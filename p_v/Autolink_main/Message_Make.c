/* Includes ------------------------------------------------------------------*/

#include <stdlib.h>
#include <time.h>

#include "AutolinkConfig.h"

#include "Modem_Manager.h"
#include "FOTA_Manager.h"
#include "GIT_Util.h"
#include "Modem_comm.h"
#include "HalLedDriver.h"
#include "Message_Manager.h"
#include "GIT_base64.h"
#include "Power_Manager.h"
#include "GIT_AesEncrypt.h"
#include "OBD_Controller.h"
#include "Message_Make.h"

#include "MngModem.h"
#include "MngSystem.h"
#include "MngQueue.h"
#include "HdDebug.h"

#include "HalHandler.h"
#include "aes.h"
#include "dukpt_algo.h"
#include "base64_penta.h"
#include "MngSystemUtil.h"

#include "HandlerRsvEngCtrl.h"
#include "CanFD_Defines.h"

//#define ENABL_KMS_LOG
//#define ENABLE_CHECK_KMS_ENC

#define Trace(...)  GITDebug(DEBUG_MODULES_MODEM,__VA_ARGS__)

extern BR_SystemInfo BkSram_SystemInfo;
extern void HandlerIpek(stMsgSysMsg* pstMsgSysMsg);
extern void GetIpekProperties(stReportIpek* pstIpek);
extern void Up_periodMessage_counter(void);
extern stCFDControl m_stCFDCtrl;
extern stUserActionSetting m_stUserActionSetting;

/* Define -------------------------------------------------------------------*/
#define MSG_STR_STX							'2'
#define MSG_STR_ETX							'3'

#define OFFSET_POS_DATA_LENGTH				(108)				// STX(1) + VIN(18) + CCID(20) + SOURCE(1) + DESTINATION(1) + MESSAGEDATE(6) + UTC(6) + OPCODE(1) = 54 -> 두배108

//#define MAX_EVENT_MESSAGE_BUFFER_LENGTH				1500

/* Variable -----------------------------------------------------------------*/
#if 1 // James Jean 2018/11/18
#include "OBD_Manager.h"
extern eFUEL_TYPE g_eFuel_Type;
extern stMsgMdm m_stLastRcvSmartKey;
#endif

/* Function -----------------------------------------------------------------*/
extern void GetAutolinkConfigProperty(uint8_t cIndex,void* pvValue);
extern void GetLastSmartMessageInfomation(stMsgMdm* pstModemMessage);
extern void GetDatefromTime2(stHalRTCTypeDef* pstDate,uint32_t unTime);
extern uint32_t GetUtcTimefromTime(uint32_t unLocalTime);
extern uint32_t GetLocalTimefromTime(uint32_t unUTCTime);
extern uint32_t GetUTCTime(); //mod.kks 21.11.05 to warnning check
/* --------------------------------------------------------------------------*/

uint16_t ConversionLongLongToHexDec_5(long long llValue, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	unsigned long int lValueH;
	unsigned long int lValueL;

	lValueH = (long)(llValue >> 32);
	lValueL = (long)(llValue);
//			Trace("time key: %04x%08x\r\n", lValueH, lValueL);
	sprintf((char *)ptrBuffer, "%02X%08X\x00", lValueH, lValueL);
	*pnPos += 10;

	return 5;
}

uint16_t ConversionLongLongToHexDec_6(long long llValue, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	unsigned long int lValueH;
	unsigned long int lValueL;

	lValueH = (long)(llValue >> 32);
	lValueL = (long)(llValue);
	//Trace("time key: %04x%08x\r\n", lValueH, lValueL);
	sprintf((char *)ptrBuffer, "%04X%08X\x00", lValueH, lValueL);
	*pnPos += 12;

	return 6;
}

uint16_t ConversionUnsignedIntToHexDec_6(uint32_t nValue, uint16_t *pnPos, uint8_t *ptrBuffer)
{
    long long llValue;
    char arrTemp[64];
	unsigned long int lValueH;
	unsigned long int lValueL;

    stHalRTCTypeDef stDate;
    GetDatefromTime2(&stDate, nValue);

	GetDateTimeStamp((uint8_t *)arrTemp, stDate.RtcDate, stDate.RtcTime);
	llValue = atoll((char *)arrTemp);

	lValueH = (long)(llValue >> 32);
	lValueL = (long)(llValue);
	//Trace("time key: %04x%08x\n", lValueH, lValueL);
	sprintf((char *)ptrBuffer, "%04X%08X\x00", lValueH, lValueL);
	*pnPos += 12;

	return 6;
}


uint16_t ConversionLongLongToHexDec_6_key(unsigned long int lValueH,unsigned long int lValueL, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	//Trace("%s] gen time key: %04x%08x\n",__FUNCTION__, lValueH, lValueL);
    sprintf((char *)ptrBuffer, "%04X%08X\x00", lValueH, lValueL);
    *pnPos += 12;


	return 6;
}


uint16_t key_display(long long llValue)
{
	unsigned long int lValueH;
	unsigned long int lValueL;

    lValueH = llValue>>32;
    lValueL = llValue;


	Trace("%s] gen time key: %04x%08x\r\n",__FUNCTION__, lValueH, lValueL);
	return 0;
}



uint16_t ConversionLongLongToHexDec_7(long long llValue, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	unsigned long int lValueH;
	unsigned long int lValueL;

	lValueH = (long)(llValue >> 32);
	lValueL = (long)(llValue);
//			Trace("time key: %04x%08x\r\n", lValueH, lValueL);
	sprintf((char *)ptrBuffer, "%04X%08X\x00", lValueH, lValueL);
	*pnPos += 14;

	return 7;
}

uint16_t ConversionFloatToHexDec_2(float fValue, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	uint32_t wTemp;

	wTemp = (uint32_t)(fValue);
	sprintf((char *)ptrBuffer, "%04X", wTemp);
	*pnPos += 4;

	return 2;
}

uint16_t ConversionFloatToHexDec_3(float fValue, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	uint32_t wTemp;

	wTemp = (uint32_t)(fValue);
	sprintf((char *)ptrBuffer, "%06X", wTemp);
	*pnPos += 6;

	return 3;
}

uint16_t ConversionFloatToHexDec_4(float fValue, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	uint32_t wTemp;

	wTemp = (uint32_t)(fValue);
	sprintf((char *)ptrBuffer, "%08X", wTemp);
	*pnPos += 8;

	return 4;
}


uint16_t ConversionIntToHexDec_1(uint16_t nValue, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	sprintf((char *)ptrBuffer, "%02X", nValue);
	*pnPos += 2;

	return 1;
}

uint16_t ConversionIntToHexDec_2(uint16_t nValue, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	sprintf((char *)ptrBuffer, "%04X", nValue);
	*pnPos += 4;

	return 2;
}

uint16_t ConversionDWordToHexDec_3(uint32_t wValue, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	sprintf((char *)ptrBuffer, "%06X", wValue);
	*pnPos += 6;

	return 3;
}

uint16_t ConversionDWordToHexDec_4(uint32_t wValue, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	sprintf((char *)ptrBuffer, "%08X", wValue);
	*pnPos += 8;

	return 4;
}

uint16_t ConversionDWordToHexDec_8(uint64_t wValue, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	unsigned long int lValueH;
	unsigned long int lValueL;

	lValueH = (long)(wValue >> 32);
	lValueL = (long)(wValue);

	//Trace("lValueH:%X, lValueL:%X\r\n",lValueH,lValueL);
	sprintf((char *)ptrBuffer, "%08X%08X", lValueH, lValueL);
	*pnPos += 16;

	return 8;
}

uint16_t ConversionGPSDataToHexDec(float fValue, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	long long slTemp;
	unsigned long int lValueH;
	unsigned long int lValueL;
	uint8_t arrTemp[64]={0};

	slTemp = (long long)(fValue * 1000000);
//	hexdump((char *)&slTemp, 8);

	lValueH = (long)(slTemp >> 32);
	lValueL = (long)(slTemp);
//	Trace(": %08X%08X\n", lValueH, lValueL);

	sprintf((char *)arrTemp, "%08X%08X\x00", lValueH, lValueL);
	memcpy((char *)ptrBuffer, (char *)&arrTemp[6], 10);
	*pnPos += 10;

	return 5;
}

uint16_t ConversionMultiByteToHexDec_n(uint8_t *pMultiByte, uint16_t n, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	uint16_t i;

	for(i = 0; i < n; i++) {
		sprintf((char *)ptrBuffer, "%02X", *pMultiByte);
		ptrBuffer++;
		ptrBuffer++;
		pMultiByte++;
	}

	*pnPos += (n * 2);

	return n;
}

uint16_t ConversionMultiByteToHexDec_n2(uint8_t *pMultiByte, uint16_t n, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	uint16_t i;
    uint8_t ucTemp;
	for(i = 0; i < n; i++) {
        ucTemp = *pMultiByte;
		sprintf((char *)ptrBuffer, "%02X",ucTemp);
		ptrBuffer++;
		ptrBuffer++;
		pMultiByte++;
	}

	*pnPos += (n * 2);

	return n;
}


uint16_t ConversionMultiByteToHexStr_n3(uint8_t *pMultiByte, uint16_t n, uint16_t *pnPos, uint8_t *ptrBuffer)
{
    int index=0;
	uint16_t i;
    uint16_t unSize=0;
    uint16_t unLength=0;
    uint8_t ucBuff[128]={0,};
    uint8_t ucBuff1[128]={0,};

	for(i = 0; i < n; i++) {
        memset(ucBuff,0,sizeof(ucBuff));

        memcpy((char*)&unSize,&pMultiByte[i*2],2);
		sprintf((char*)ucBuff, "%d%c", unSize,',');
        unLength = strlen((char const*)ucBuff);

        memcpy(&ucBuff1[index],ucBuff,unLength);
        index+=unLength;
	}

    ConvertCharArrayToHex(index-1, (char*)ucBuff1, (char*)ptrBuffer, 0);

    ptrBuffer[index] = 0;
    // we extract last ','
	*pnPos += ((index - 1)*2);

	return ((index - 1));
}


uint16_t ConversionMultiByteToHexStr_n(uint8_t *pMultiByte, uint16_t n, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	uint16_t i;

	for(i = 0; i < n; i++) {
		sprintf((char *)ptrBuffer, "%02X", *pMultiByte);
		ptrBuffer++;
        ptrBuffer++;
		pMultiByte++;
	}

	*pnPos += (n * 2);

	return n;
}

uint16_t ConversionMultiByteToHexStr_n2(uint8_t nRealNum,uint8_t *pMultiByte, uint16_t n, uint16_t *pnPos, uint8_t *ptrBuffer)
{
	uint16_t i;

	for(i = 0; i < n; i++) {
        if(nRealNum > i)
        {
		    sprintf((char *)ptrBuffer, "%02X", *pMultiByte);
        }
        else
        {
            sprintf((char *)ptrBuffer, "%02X", ' ');
        }

		ptrBuffer++;
        ptrBuffer++;
		pMultiByte++;
	}

	*pnPos += (n*2);

	return n;
}

uint16_t ConversionMultiByteToHexStr_n4(uint8_t *ptrBuffer,uint16_t *pnPos, uint8_t *pMultiByte, uint16_t n)
{
	uint16_t i;

	for(i = 0; i < n; i++) {
        sprintf((char *)ptrBuffer, "%02X", *pMultiByte);
		ptrBuffer++;
        ptrBuffer++;
		pMultiByte++;
	}

	*pnPos += (n*2);

	return n;
}

//mod.pdh 2021.10.27
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

uint16_t MakeEventMessageHeader2(eMESSAGE_TYPE eMsgType, uint8_t *strMessageBuffer, stMsgHeader* pstMessageHeader)
{
	uint16_t nPos;
	uint8_t arrTemp[64];
	uint16_t i;
	uint16_t n;
//	long long llValue;
	uint32_t nTemp;
    uint32_t wTemp;

    char carrBuff[128]={0};

	memset((char *)arrTemp, 0x00, 64);

	nPos = 0;
	//********************************
	// Head
	//********************************
	// STX
	nTemp = MSG_STR_STX;
	ConversionIntToHexDec_1(nTemp, &nPos, &strMessageBuffer[nPos]);

	//VIN
    GetAutolinkConfigProperty(eAutoLinkConfig_Vin,(void*)carrBuff);
	n = strlen((char *)carrBuff);
	memset((char *)arrTemp, ' ', 64);
	memcpy((char *)arrTemp, carrBuff, n);

	for(i = 0; i < 18; i++) {
		sprintf((char *)&strMessageBuffer[nPos], "%02X", arrTemp[i]);
		nPos += 2;
	}

	// SIM No
	memset((char *)arrTemp, ' ', 64);
	memcpy((char *)arrTemp, BkSram_ModemInfo.aCCID, strlen((char *)BkSram_ModemInfo.aCCID));
	for(i = 0; i < 20; i++) {
		sprintf((char *)&strMessageBuffer[nPos], "%02X", arrTemp[i]);
		nPos += 2;
	}

	// 송신부
	nTemp = 3;
	ConversionIntToHexDec_1(nTemp, &nPos, &strMessageBuffer[nPos]);

	// 수신부
	nTemp = 1;
	ConversionIntToHexDec_1(nTemp, &nPos, &strMessageBuffer[nPos]);

    stHalRTCTypeDef stHalRtcDateTime;
    APP_TimeShow(&stHalRtcDateTime);

	if(eMsgType == eMESSAGE_TYPE_TRIP_REPORTING) {
        //MONI 2018-1-22
        // trip time will be save in message of system manager
        // becuse all message is handled in message of system manager
        wTemp = GetLocalTimefromTime(GetUTCTime());
        ConversionUnsignedIntToHexDec_6(wTemp,&nPos, &strMessageBuffer[nPos]);

        wTemp = GetUTCTime();
        ConversionUnsignedIntToHexDec_6(wTemp,&nPos, &strMessageBuffer[nPos]);

		// 통신 에러가 발생할 경우, 외장 serial flash memory에 전송하지 못한 message를 저장하기 위해서 만들어 둔다.
		// message 생성 시간과 message 구분자를 각각 저장한다.
		// MONI 2018-03-06
		// Not used code
		//sprintf((char *)MessageManagerData.aSaveDataTimeStampForError, "%s-%02d:\x00", arrTemp, eMsgType);
	}
	else {
		// 발생시간(Local)
		wTemp = GetLocalTimefromTime(GetUTCTime());
        ConversionUnsignedIntToHexDec_6(wTemp,&nPos, &strMessageBuffer[nPos]);

        wTemp = GetUTCTime();
        ConversionUnsignedIntToHexDec_6(wTemp,&nPos, &strMessageBuffer[nPos]);
		// 통신 에러가 발생할 경우, 외장 serial flash memory에 전송하지 못한 message를 저장하기 위해서 만들어 둔다.
		// message 생성 시간과 message 구분자를 각각 저장한다.
		// MONI 2018-03-06
		// Not used code
		//sprintf((char *)MessageManagerData.aSaveDataTimeStampForError, "%s-%02d:\x00", arrTemp, eMsgType);
	}

	// 명령 구분
	if(eMsgType == eMESSAGE_TYPE_REMOTE_CONTROL_REQUEST)
	{
		nTemp = (uint16_t)eMESSAGE_TYPE_REMOTE_CONTROL_REPORT;
	}
#if defined(PROTOCOL17)
	else if(eMsgType == eMESSAGE_TYPE_RESPONSE_SETURL)
	{
		nTemp = (uint16_t)eMESSAGE_TYPE_REQUEST_SETURL;
	}
#endif
	else {
		nTemp = (uint16_t)eMsgType;
	}
	ConversionIntToHexDec_1(nTemp, &nPos, &strMessageBuffer[nPos]);

	// 데이터 길이(여기서는 계산을 하지 않는다. 전문을 다 채워두고 계산한다.)
	nTemp = 0;
	ConversionIntToHexDec_2(nTemp, &nPos, &strMessageBuffer[nPos]);

	return nPos;
}


void MakeReqSettingInfo(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer)
{
}

#if defined(PROTOCOL12)
void MakeReqModemActive(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer)
{
    uint16_t i;

	Trace("@MakeReqModemActive()\r\n");
    hexdump(MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);

	// GUID
	for(i = 0; i < MAX_GUID_LENGTH; i++) {
		sprintf((char *)&paMessageBuffer[*pnPos], "%x", MessageManagerData.aRemoteControlGUID[i]);
		*pnPos += 2;
	}
	*pnDataLength += MAX_GUID_LENGTH / 2;
}

void MakeRspModemActive(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer,stReportSetting* pstReportSetting)
{
	uint16_t i;
	uint32_t nTemp;
	uint32_t wTemp;
    stMsgMdm stMessage;

    Trace("@MakeRspModemActive()\r\n");
#if false
    Trace("CarReportGUID:\r\n");
	hexdump(pstSmartKey->Request.Guid, MAX_GUID_LENGTH);
    Trace("MessageDataGUID:\r\n");
    hexdump(MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);
#endif

	// MONI 20200211 changed guid info
	// sometimes guid will be changed with other request.
	// GUID
	//for(i = 0; i < MAX_GUID_LENGTH; i++) {
	//	sprintf((char *)&paMessageBuffer[*pnPos], "%x", MessageManagerData.aRemoteControlGUID[i]);
	//	*pnPos += 2;
	//}
	for(i = 0; i < MAX_GUID_LENGTH; i++) {
		sprintf((char *)&paMessageBuffer[*pnPos], "%x", pstReportSetting->UserSetting.stUserActionSetting.ModemActivate.carrRequestGUID[i]);
		*pnPos += 2;
	}

	*pnDataLength += MAX_GUID_LENGTH / 2;

	// 발생시간
    wTemp = pstReportSetting->UserSetting.stUserActionSetting.ModemActivate.unEventTime;
	*pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

    GetLastSmartMessageInfomation(&stMessage);
    // sms rcv time
    wTemp = stMessage.carReport.rpSetting.UserSetting.stUserActionSetting.ModemActivate.unSmsRcvTime;

    //*pnDataLength += ConversionUnsignedIntToHexDec_6(pstSmartKey->Request.OccurredEventTime, pnPos, &paMessageBuffer[*pnPos]);
    *pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 결과값
	nTemp = pstReportSetting->UserSetting.stUserActionSetting.ModemActivate.ucResult;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 실패원인
	nTemp = pstReportSetting->UserSetting.stUserActionSetting.ModemActivate.usReason;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	return;
}
#endif

#if defined(PROTOCOL15)
void MakeReqSensorInitialize(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer)
{
    uint16_t i;

	Trace("@MakeReqModemActive()\r\n");
    hexdump(MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);

	// GUID
	for(i = 0; i < MAX_GUID_LENGTH; i++) {
		sprintf((char *)&paMessageBuffer[*pnPos], "%x", MessageManagerData.aRemoteControlGUID[i]);
		*pnPos += 2;
	}
	*pnDataLength += MAX_GUID_LENGTH / 2;
}


void MakeRspSensorInitialize(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer,stReportSetting* pstReportSetting)
{
	uint16_t i;
	uint32_t nTemp;
	uint32_t wTemp;
    stMsgMdm stMessage;

    Trace("@MakeRspModemActive()\r\n");
#if false
    Trace("CarReportGUID:\r\n");
	hexdump(pstSmartKey->Request.Guid, MAX_GUID_LENGTH);
    Trace("MessageDataGUID:\r\n");
    hexdump(MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);
#endif
	// GUID
	for(i = 0; i < MAX_GUID_LENGTH; i++) {
		sprintf((char *)&paMessageBuffer[*pnPos], "%x", MessageManagerData.aRemoteControlGUID[i]);
		*pnPos += 2;
	}
	*pnDataLength += MAX_GUID_LENGTH / 2;

	// 발생시간
    wTemp = GetLocalTimefromTime(GetUTCTime());
	*pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

    GetLastSmartMessageInfomation(&stMessage);
    // sms rcv time
    wTemp = stMessage.carReport.rpSetting.UserSetting.stUserActionSetting.SensorInitialize.unSmsRcvTime;

    //*pnDataLength += ConversionUnsignedIntToHexDec_6(pstSmartKey->Request.OccurredEventTime, pnPos, &paMessageBuffer[*pnPos]);
    *pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 결과값
	nTemp = pstReportSetting->UserSetting.stUserActionSetting.SensorInitialize.ucResult;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 실패원인
	nTemp = pstReportSetting->UserSetting.stUserActionSetting.SensorInitialize.ucReason;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// X
	nTemp = pstReportSetting->UserSetting.stUserActionSetting.SensorInitialize.sX;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// Y
	nTemp = pstReportSetting->UserSetting.stUserActionSetting.SensorInitialize.sY;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// Z
	nTemp = pstReportSetting->UserSetting.stUserActionSetting.SensorInitialize.sZ;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	return;
}
#endif

#if defined(PROTOCOL17)
void MakeReqSetURL(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer,stReportSmartKey *pstSmartKey)
{
	uint16_t i;
	uint32_t nTemp;
	uint32_t wTemp;
    stMsgMdm stMessage;

    Trace("@MakeRspSetURL()\r\n");

	// GUID
	for(i = 0; i < MAX_GUID_LENGTH; i++) {
		sprintf((char *)&paMessageBuffer[*pnPos], "%x", MessageManagerData.aRemoteControlGUID[i]);
		*pnPos += 2;
	}
	*pnDataLength += MAX_GUID_LENGTH / 2;

	// 발생시간
    wTemp = GetLocalTimefromTime(GetUTCTime());
	*pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

    GetLastSmartMessageInfomation(&stMessage);
    // sms rcv time
    wTemp = stMessage.carReport.rpSetting.UserSetting.stUserActionSetting.SensorInitialize.unSmsRcvTime;
    *pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 결과값
	nTemp = 1;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	return;
}
void MakeReqSetURLComplete(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer,stCarReport *pstCarReport)
{
	uint16_t i;
	//uint32_t nTemp;
	uint32_t wTemp;
    //stMsgMdm stMessage;

    Trace("@MakeReqSetURLComplete()\r\n");

	// GUID
	for(i = 0; i < MAX_GUID_LENGTH; i++) {
		sprintf((char *)&paMessageBuffer[*pnPos], "%x", pstCarReport->rpSetting.UserSetting.RequestGUID[i]);
		*pnPos += 2;
	}
	*pnDataLength += MAX_GUID_LENGTH / 2;

	// 발생시간
    wTemp = pstCarReport->rpSetting.UserSetting.stUserActionSetting.SetUrl.unEventTime;
	*pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	return;
}
#endif

#if defined(PROTOCOL24)
void MakeReportNetworkStatus(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer,stReportModemStatus *pstReportModemStatus)
{
	uint32_t nTemp=0, wTemp=0, nLength=0;
	float fTemp = 0;

    Trace("@MakeReportNetworkStatus()\n");
	
	// 위도: -33.915						--> FFFDFA7F88: 위도
	fTemp = pstReportModemStatus->Latitude;
	*pnDataLength += ConversionGPSDataToHexDec(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 경도: 151.212						--> 0009034FE0: 경도
	fTemp = pstReportModemStatus->Longitude;
	*pnDataLength += ConversionGPSDataToHexDec(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// GPS 상태: 1									--> 01: GPS 상태
	nTemp = pstReportModemStatus->GpsValid;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportModemStatus->ModemConnectStatus;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nLength = strlen(pstReportModemStatus->ModemStatus);
	*pnDataLength +=ConversionMultiByteToHexStr_n2(nLength,pstReportModemStatus->ModemStatus, MAX_MODEM_STATUS_LENGTH, pnPos, &paMessageBuffer[*pnPos]);

	// local time
	wTemp = pstReportModemStatus->OccurredEventTime;
    *pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

    // utc time
    wTemp = pstReportModemStatus->OccurredEventUtcTime;
    *pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);


	return;
}
#endif

void MakeIpekPhase1(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer)
{
	//uint16_t i;
    uint32_t wTemp;
	uint32_t unUTCTime,unLocalTime;
	unUTCTime = GetUTCTime();
	unLocalTime = GetLocalTimefromTime(unUTCTime);

	Trace("@MakeIpekPhase1()\r\n");

    wTemp = unLocalTime;
    *pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

    wTemp = unUTCTime;
    *pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	return;
}

void MakeIpekPhase2(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer)
{
	//uint16_t i;
    uint32_t nTemp;
    //uint8_t carrTemp[64]={0,};
	Trace("@MakeIpekPhase2()\r\n");
    stReportIpek stIpekSetting;

    GetIpekProperties(&stIpekSetting);

    // ksn
    nTemp = stIpekSetting.Phase2.KsnSize;
	ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);
    *pnDataLength += ConversionMultiByteToHexDec_n(stIpekSetting.Phase2.Ksn,nTemp, pnPos, &paMessageBuffer[*pnPos]);

    // encrand
    nTemp = stIpekSetting.Phase2.EncRandSize;
    ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

    memcpy(&paMessageBuffer[*pnPos],stIpekSetting.Phase2.Enc,nTemp*2);
    //*pnDataLength += ConversionMultiByteToHexDec_n(stIpekSetting.Phase2.Enc,nTemp, pnPos, &paMessageBuffer[*pnPos]);
    *pnPos += (nTemp*2);
    *pnDataLength += (nTemp);

	return;
}

int MakeGpsList2String(stReportDrivingInfoBody* pstReportDrivingInfoBody,char* parrTemp)
{
    int nSize= 0 ;
#ifndef AGINGTEST //mod.kks 21.11.26 to test send the 2k packet data to server.
    if( pstReportDrivingInfoBody->GpsListCount > 0 )
    {
        for(int i=0;i<pstReportDrivingInfoBody->GpsListCount - 1;i++)
        {
            sprintf(&parrTemp[nSize],"%f,%f|",pstReportDrivingInfoBody->GpsLatitudeList[i],
                                              pstReportDrivingInfoBody->GpsLongitudeList[i]);
            nSize = strlen(parrTemp);
        }
        sprintf(&parrTemp[nSize],"%f,%f",pstReportDrivingInfoBody->GpsLatitudeList[pstReportDrivingInfoBody->GpsListCount-1],pstReportDrivingInfoBody->GpsLongitudeList[pstReportDrivingInfoBody->GpsListCount-1]);
        nSize = strlen(parrTemp);
    }
#else
    for(int i=0;i<5;i++)
    {
        sprintf(&parrTemp[nSize],"%f,%f|",Get_GPS_Lat(),
                                          Get_GPS_Lon());
        nSize = strlen(parrTemp);
    }
    sprintf(&parrTemp[nSize],"%f,%f",pstReportDrivingInfoBody->GpsLatitudeList[pstReportDrivingInfoBody->GpsListCount-1],pstReportDrivingInfoBody->GpsLongitudeList[pstReportDrivingInfoBody->GpsListCount-1]);
    nSize = strlen(parrTemp);
#endif
    return nSize;
}

int MakeGpsList2String2(uint8_t ucCount, double* pdlLatitudeList,double* pdlLongitudeList,char* parrTemp)
{
    int nSize= 0 ;

    if( ucCount > 0 )
    {
        for(int i=0;i<ucCount-1;i++)
        {
            sprintf(&parrTemp[nSize],"%f,%f|",pdlLatitudeList[i],pdlLongitudeList[i]);
            nSize = strlen(parrTemp);
        }
        sprintf(&parrTemp[nSize],"%f,%f",pdlLatitudeList[ucCount-1],pdlLongitudeList[ucCount-1]);
        nSize = strlen(parrTemp);

        if( nSize < 512 )
            hexdump(parrTemp,nSize);
    }
    return nSize;
}


void MakeChargingReport(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer,stReportChargingInfo* pstReportChargingInfo)
{
	//int32_t nSize=0;
	//float fTemp;
	uint32_t nTemp;
	//uint32_t wTemp;
    //char arrTemp[1024]={0,};

	//SOC
    nTemp = pstReportChargingInfo->HighBatterySOC;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// SOH
	nTemp = pstReportChargingInfo->HighBatterySOH;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 주행가능거리
	nTemp = pstReportChargingInfo->ExtraDrivingDistance;
	*pnDataLength += ConversionFloatToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 배터리 온도 MIN
	nTemp = pstReportChargingInfo->HighBatteryTemperatureMin;
	*pnDataLength += ConversionFloatToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 배터리 온도 MAX
	nTemp = pstReportChargingInfo->HighBatteryTemperatureMax;
	*pnDataLength += ConversionFloatToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);


	// 충전잔여시간
	nTemp = pstReportChargingInfo->ExtraCharingTime;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	return;

}

void MakePeriodInfomation2(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer,stReportDrivingInfoBody* pstReportDrivingInfoBody )
{
    int32_t nSize=0;
	float fTemp;
	uint32_t nTemp;
	uint32_t wTemp;
    char arrTemp[1024]={0,};

	// speed: 120.4 							--> 04B4: speed
	fTemp = pstReportDrivingInfoBody->Speed;
	*pnDataLength += ConversionFloatToHexDec_2(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// rpm: 1000 							--> 2710: rpm
	fTemp = pstReportDrivingInfoBody->Rpm;
	*pnDataLength += ConversionFloatToHexDec_2(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 최고 속도
	nTemp = pstReportDrivingInfoBody->MaxSpeed;
	*pnDataLength += ConversionFloatToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 최고 RPM
	nTemp = pstReportDrivingInfoBody->MaxRpm;
	*pnDataLength += ConversionFloatToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 연료 소비량
	fTemp = pstReportDrivingInfoBody->ConsumedFuel * 1000000;
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 이동거리
	wTemp = pstReportDrivingInfoBody->DrivingDistance;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 급가속 횟수: 3 									--> 0003: 급가속 횟수
	nTemp = pstReportDrivingInfoBody->RapidAccelCount;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 급감속 횟수: 3 									--> 0003: 급감속 횟수
	nTemp = pstReportDrivingInfoBody->RapidDecelCount;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);
#if defined(PROTOCOL17)
	// 공회전 시간
	nTemp = pstReportDrivingInfoBody->EngineIdleTime;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 워밍업 시간
	nTemp = pstReportDrivingInfoBody->WarmUpTime;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif
	// GPS 상태: 1 									--> 01: GPS 상태
	nTemp = pstReportDrivingInfoBody->GpsValid;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// GPS 위성개수: 1 									--> 01: GPS 위성개수
	nTemp = pstReportDrivingInfoBody->GpsSateliteCount;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 위도: -33.915 						--> FFFDFA7F88: 위도
	fTemp = pstReportDrivingInfoBody->GpsLatitude;
	*pnDataLength += ConversionGPSDataToHexDec(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 경도: 151.212 						--> 0009034FE0: 경도
	fTemp = pstReportDrivingInfoBody->GpsLongitude;
	*pnDataLength += ConversionGPSDataToHexDec(fTemp, pnPos, &paMessageBuffer[*pnPos]);

    // MONI 20180518
    // Added protocol version 1.1
    // added gps list
    // GPS List Count : 1
    Trace("Gps List Count / max count : %d, list count : %d\n",MAX_GPS_LIST_COUNT,pstReportDrivingInfoBody->GpsListCount);
    // gps list total size : 2
#ifndef AGINGTEST //mod.kks 21.11.26 to test send the 2k packet data to server.
    if( pstReportDrivingInfoBody->GpsListCount == 0 )
    {
        sprintf(&arrTemp[nSize],"%f,%f",pstReportDrivingInfoBody->GpsLatitude,pstReportDrivingInfoBody->GpsLongitude);
    }
    else
    {
        if( pstReportDrivingInfoBody->GpsListCount > 50 )
        {
            Trace("Gps List Count is minus : %d\r\n",pstReportDrivingInfoBody->GpsListCount);
            pstReportDrivingInfoBody->GpsListCount = 0;
            sprintf(&arrTemp[nSize],"%f,%f",pstReportDrivingInfoBody->GpsLatitude,pstReportDrivingInfoBody->GpsLongitude);
        }
        else
        {
            // added last position
            nSize = MakeGpsList2String(pstReportDrivingInfoBody ,arrTemp);
            sprintf(&arrTemp[nSize],"|%f,%f",pstReportDrivingInfoBody->GpsLatitude,pstReportDrivingInfoBody->GpsLongitude);
        }
    }
#endif

#ifdef AGINGTEST //mod.kks 21.11.26 to test send the 2k packet data to server.
    nSize = MakeGpsList2String(pstReportDrivingInfoBody ,arrTemp);
    sprintf(&arrTemp[nSize],"|%f,%f",pstReportDrivingInfoBody->GpsLatitude,pstReportDrivingInfoBody->GpsLongitude);
#endif

    nSize = strlen(arrTemp);
#ifndef AGINGTEST //mod.kks 21.11.26 to test send the 2k packet data to server.
    if( nSize < 1024 )
    {
        //hexdump(arrTemp,nSize);
    }
    else
    {
        Trace("Error Make Gps List String\r\n");
        pstReportDrivingInfoBody->GpsListCount=0;
        sprintf(&arrTemp[nSize],"%f,%f",pstReportDrivingInfoBody->GpsLatitude,pstReportDrivingInfoBody->GpsLongitude);
        nSize = strlen(arrTemp);
        //hexdump(arrTemp,nSize);
    }

    if( MAX_GPS_LIST_COUNT >= pstReportDrivingInfoBody->GpsListCount )
    {
        nTemp = pstReportDrivingInfoBody->GpsListCount+1;
        *pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
    }
    else
    {
        // not expected case
        nTemp = 1;
        pstReportDrivingInfoBody->GpsListCount = 1;
        *pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
    }
#endif // mod.kks todo remove

#ifdef AGINGTEST //mod.kks 21.11.26 to test send the 2k packet data to server.
    nTemp = 6;
    *pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif

    // gps list size
    *pnDataLength += ConversionFloatToHexDec_2(nSize, pnPos, &paMessageBuffer[*pnPos]);

    // gps list data, "lat,lon|lat,lon"
	*pnDataLength += ConversionMultiByteToHexStr_n((unsigned char*)arrTemp, nSize, pnPos, &paMessageBuffer[*pnPos]);

#if defined(PROTOCOL16)
	nTemp = (uint16_t)pstReportDrivingInfoBody->GpsDirection;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif

	// 타이어 공기압: 0 									--> 000000000000: 타이어 공기압
	// 전좌/전우/후좌/후우(12자리), 소수점을 없애기 위해서 10을 곱해준다.
	// 23.5/23.5/23.5/23.5 일 경우, 235/235/235/235 --> 2byte/2byte/2byte/2byte --> 을 최종 적으로 사용
	nTemp = pstReportDrivingInfoBody->TpmsFL;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportDrivingInfoBody->TpmsFR;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportDrivingInfoBody->TpmsRL;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportDrivingInfoBody->TpmsRR;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 밧데리 전압: 50 								--> 01F4: 밧데리 전압
	nTemp = pstReportDrivingInfoBody->CarBattery;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 모뎀 수신 레벨: -70 								--> 46: 모뎀 수신 레벨
	//nTemp = abs(ModemManagerData.ndBm);
	nTemp = pstReportDrivingInfoBody->ModemRssi;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 차량문 잠금 상태: 00010000(b) 				--> 10(0001 0000): 차량문 잠금 상태
	nTemp = pstReportDrivingInfoBody->DoorLockStatus;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 차량문 열림 상태
	nTemp = pstReportDrivingInfoBody->DoorOpenStatus;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 차량 ACC 상태: 4 									--> 04: 차량 ACC 상태
	nTemp = pstReportDrivingInfoBody->AccStatus;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 총 이동거리: 63 								--> 00003F: 총 이동거리
	nTemp = pstReportDrivingInfoBody->Odometer;
	*pnDataLength += ConversionDWordToHexDec_3(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// BLE 상태: 0 									--> 00: BLE 상태
      	nTemp = pstReportDrivingInfoBody->BleStatus;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 연료 잔량: 58 								--> 3A: 연료 잔량
	nTemp = pstReportDrivingInfoBody->RemainedFuel;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 배터리 잔량
	nTemp = pstReportDrivingInfoBody->RemainedECarBattery;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 미등
	nTemp = pstReportDrivingInfoBody->TailLanmp;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

    // interiorlamp
    nTemp = pstReportDrivingInfoBody->InteriorLamp;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

    // headlight
    nTemp = pstReportDrivingInfoBody->HeadLight;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// MIL 점등
	nTemp = pstReportDrivingInfoBody->MILLamp;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

#if defined(PROTOCOL16)
	if ( pstReportDrivingInfoBody->ElectronicEffiency > 99.9 || pstReportDrivingInfoBody->ElectronicEffiency < 0 )
		fTemp = 99.9;
	else
		fTemp = pstReportDrivingInfoBody->ElectronicEffiency * 10;	//배터리연비
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportDrivingInfoBody->RemainedECarBattery;	//배터리잔량SOC
	pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportDrivingInfoBody->SOH;	//배터리노화진행률
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportDrivingInfoBody->ExtraDrivingDistance;	//남은주행거리
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportDrivingInfoBody->HighBatteryTemperature;	//고전압배터리온도
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

#if defined(PROTOCOL18)
	nTemp = pstReportDrivingInfoBody->HighBatteryTemperatureMax;	//고전압배터리온도
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	//주행시간 (전기차 : 배터리 구동시간)
	//nTemp = GetUTCTime()-Get_DriveStartTime_UTC();
	nTemp = pstReportDrivingInfoBody->EngRunTime;
	*pnDataLength += ConversionDWordToHexDec_3(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	//전류 사용량
	fTemp = pstReportDrivingInfoBody->ConsumedBattery;
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif

	nTemp = pstReportDrivingInfoBody->OutsideTemperature;	//외기온도
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportDrivingInfoBody->MotorRpm;	//모터RPM
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportDrivingInfoBody->MaxMotorRpm;	//최고모터RPM
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	fTemp = pstReportDrivingInfoBody->ConsumedBattery*1000000;	//배터리소모량
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif //PROTOCOL16
#if defined(PROTOCOL17)
	fTemp = pstReportDrivingInfoBody->ConsumedBattery_Regen*1000000;	//배터리소모량
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif

	// 연비: 12.1 							--> 0079: 연비
    if( FUELTYPE_GET_STATE() == ELECTRONIC || FUELTYPE_GET_STATE() == EV_NONE_READY)
    {
        if ( pstReportDrivingInfoBody->FuelEffiency > 99.9 || pstReportDrivingInfoBody->FuelEffiency < 0 )
            fTemp = 99.9;
        else
            fTemp = pstReportDrivingInfoBody->FuelEffiency * 10;
    }
    else	fTemp = pstReportDrivingInfoBody->FuelEffiency * 10;
	*pnDataLength += ConversionFloatToHexDec_2(fTemp, pnPos, &paMessageBuffer[*pnPos]);

////	// 주행시간: 5000 							--> 001388: 주행시간
////	wTemp = Get_DrivingTime();
////	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);

//	// 40km/h 미만 갯수: 15 								--> 00000F: 40km/h 미만 갯수
//	wTemp = pstReportDrivingInfoBody->SpdCntUnder40km;
//	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
//
//	// 40 ~ 80km/h 갯수: 16 								--> 000010: 40 ~ 80km/h 갯수
//	wTemp = pstReportDrivingInfoBody->SpdCntUpper40kmUnder80km;
//	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
//
//	// 80 ~ 120km/h 갯수: 17 								--> 000011: 80 ~ 120km/h 갯수
//	wTemp = pstReportDrivingInfoBody->SpdCntUpper80kmUnder120km;
//	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
//
//	// 120 ~ 160km/h 갯수: 18 								--> 000012: 120 ~ 160km/h 갯수
//	wTemp = pstReportDrivingInfoBody->SpdCntUpper120kmUnder160km;
//	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
//
//	// 160km/h 이상: 0 									--> 000000: 160km/h 이상
//	wTemp = pstReportDrivingInfoBody->SpdCntUpper141km;
//	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 10km/h 미만 갯수: 15 								--> 00000F: 10km/h 미만 갯수
	nTemp = pstReportDrivingInfoBody->SpdCnt0km;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 10km/h 미만 갯수: 15 								--> 00000F: 10km/h 미만 갯수
	nTemp = pstReportDrivingInfoBody->SpdCntUpper1kmUnder10km;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 10 ~ 20km/h 갯수: 16 								--> 000010: 10 ~ 20km/h 갯수
	nTemp = pstReportDrivingInfoBody->SpdCntUpper11kmUnder20km;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 20 ~ 30km/h 갯수: 17 								--> 000011: 20 ~ 30km/h 갯수
	nTemp = pstReportDrivingInfoBody->SpdCntUpper21kmUnder30km;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 30 ~ 40km/h 갯수: 18 								--> 000012: 30 ~ 40km/h 갯수
	nTemp = pstReportDrivingInfoBody->SpdCntUpper31kmUnder40km;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 40 ~ 50km/h 갯수: 19 								--> 000013: 40 ~ 50km/h 갯수
	nTemp = pstReportDrivingInfoBody->SpdCntUpper41kmUnder50km;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 50 ~ 60km/h 갯수: 20 								--> 000014: 50 ~ 60km/h 갯수
	nTemp = pstReportDrivingInfoBody->SpdCntUpper51kmUnder60km;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 60 ~ 70km/h 갯수: 21 								--> 000015: 60 ~ 70km/h 갯수
	nTemp = pstReportDrivingInfoBody->SpdCntUpper61kmUnder70km;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 70 ~ 80km/h 갯수: 22 								--> 000016: 70 ~ 80km/h 갯수
	nTemp = pstReportDrivingInfoBody->SpdCntUpper71kmUnder80km;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 80 ~ 90km/h 갯수: 23 								--> 000017: 80 ~ 90km/h 갯수
	nTemp = pstReportDrivingInfoBody->SpdCntUpper81kmUnder90km;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 90 ~ 100km/h 갯수: 24 								--> 000018: 90 ~ 100km/h 갯수
	nTemp = pstReportDrivingInfoBody->SpdCntUpper91kmUnder100km;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 100 ~ 110km/h 갯수: 25 								--> 000019: 100 ~ 110km/h 갯수
	nTemp = pstReportDrivingInfoBody->SpdCntUpper101kmUnder110km;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 110 ~ 120km/h 갯수: 26 								--> 00001A: 110 ~ 120km/h 갯수
	nTemp = pstReportDrivingInfoBody->SpdCntUpper111kmUnder120km;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 120 ~ 130km/h 갯수: 27 								--> 00001B: 120 ~ 130km/h 갯수
	nTemp = pstReportDrivingInfoBody->SpdCntUpper121kmUnder130km;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 130 ~ 140km/h 갯수: 28 								--> 00001C: 130 ~ 140km/h 갯수
	nTemp = pstReportDrivingInfoBody->SpdCntUpper131kmUnder140km;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 141km/h 이상: 0 									--> 000000: 141km/h 이상
	nTemp = pstReportDrivingInfoBody->SpdCntUpper141km;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

    nTemp = pstReportDrivingInfoBody->IdlingCount;
    *pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

    nTemp = pstReportDrivingInfoBody->StopCount;
    *pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

    nTemp = pstReportDrivingInfoBody->BreakeCount;
    *pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

    nTemp = pstReportDrivingInfoBody->InertiaDrivingCount;
    *pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

    nTemp = pstReportDrivingInfoBody->NormalDrivingCount;
    *pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

#if defined(PROTOCOL17)
	nTemp = pstReportDrivingInfoBody->ExtraDrivingCount;
    *pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif

    // local time
    wTemp = pstReportDrivingInfoBody->OccurredEventTime;
    *pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

    // utc time
    wTemp = pstReportDrivingInfoBody->OccurredEventUtcTime;
    *pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 전송 순번: 1 									--> 0001: 전송 순번
	Up_periodMessage_counter(); /* init value is '0' */
	nTemp = pstReportDrivingInfoBody->SequnceNumber;

	Trace("MSG: Period message count: %d\r\n", nTemp);

	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

#if defined(PROTOCOL21)
	nTemp = pstReportDrivingInfoBody->EngOilLifeRatio;	//엔진오일 수명 잔량
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportDrivingInfoBody->EngOilLifeEna;	//엔진오일 센서 데이터 지원 여부
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportDrivingInfoBody->EngOilLifeWarn;	//엔진오일 교체 알림
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportDrivingInfoBody->HydrogenTemperature; // 수소탱크 현재 온도값
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	fTemp = pstReportDrivingInfoBody->HydrogenTankPress * 10; // 수소탱크 압력값
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	wTemp = pstReportDrivingInfoBody->HydrogenChargeCnt; // 수소탱크 연료 주입 횟수
	*pnDataLength += ConversionDWordToHexDec_4(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	fTemp = pstReportDrivingInfoBody->FuelcellVoltLow * 100; // 배터리 Cell 최소 전압
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	fTemp = pstReportDrivingInfoBody->FuelcellVoltHigh * 100; // 배터리 Cell 최고 전압
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	fTemp = pstReportDrivingInfoBody->HydrogenFuel * 10; // 수소 연료 레벨
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	fTemp = pstReportDrivingInfoBody->AirPurification * 10; // 공기 정화량
	*pnDataLength += ConversionDWordToHexDec_4((uint32_t)fTemp, pnPos, &paMessageBuffer[*pnPos]);

	fTemp = pstReportDrivingInfoBody->CO2Reduction * 10; // CO2 감축량
	*pnDataLength += ConversionDWordToHexDec_4((uint32_t)fTemp, pnPos, &paMessageBuffer[*pnPos]);

	wTemp = (uint32_t)(pstReportDrivingInfoBody->BMSBatteryCurr * 10); // BMS Battery Current
	*pnDataLength += ConversionDWordToHexDec_4(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	wTemp = (uint32_t)(pstReportDrivingInfoBody->BMSBatteryVolt * 10); // BMS Battery Voltage
	*pnDataLength += ConversionDWordToHexDec_4(wTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif
#if defined(PROTOCOL22)
	nTemp = pstReportDrivingInfoBody->LowBatterySOC;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif
#if defined(PROTOCOL23)
	nTemp = pstReportDrivingInfoBody->InsulationResistance; // insulation Resistance - EV Car
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif
#if defined(PROTOCOL13)
    {
    char carrBuff[128]={0,};
    char carrTemp[64]={0,};
    DCSServiceType nServiceType;

    // get service type fleet / retail
    GetAutolinkConfigProperty(eAutoLinkConfig_ServiceType,(void*)&nServiceType);

    // added protocol version 1.3
    if( nServiceType == DCS_Fleet )
    {
#if defined(PROTOCOL21) //Retail 로 수정
#elif defined(PROTOCOL19)
		nTemp = pstReportDrivingInfoBody->HydrogenTemperature; // 수소탱크 현재 온도값
		*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

		fTemp = pstReportDrivingInfoBody->HydrogenTankPress * 10; // 수소탱크 압력값
		*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);

		wTemp = pstReportDrivingInfoBody->HydrogenChargeCnt; // 수소탱크 연료 주입 횟수
		*pnDataLength += ConversionDWordToHexDec_4(wTemp, pnPos, &paMessageBuffer[*pnPos]);
 
		fTemp = pstReportDrivingInfoBody->FuelcellVoltLow * 100; // 배터리 Cell 최소 전압
		*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);

		fTemp = pstReportDrivingInfoBody->FuelcellVoltHigh * 100; // 배터리 Cell 최고 전압
		*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);

		fTemp = pstReportDrivingInfoBody->HydrogenFuel * 10; // 수소 연료 레벨
		*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);

		fTemp = pstReportDrivingInfoBody->AirPurification * 10; // 공기 정화량
		*pnDataLength += ConversionDWordToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);

		fTemp = pstReportDrivingInfoBody->CO2Reduction * 10; // CO2 감축량
		*pnDataLength += ConversionDWordToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);

		wTemp = pstReportDrivingInfoBody->BMSBatteryCurr * 10; // BMS Battery Current
		*pnDataLength += ConversionDWordToHexDec_4(wTemp, pnPos, &paMessageBuffer[*pnPos]);

		wTemp = pstReportDrivingInfoBody->BMSBatteryVolt * 10; // BMS Battery Voltage
		*pnDataLength += ConversionDWordToHexDec_4(wTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif
        // add type
        nTemp = 0xF117;
        *pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

        // add rfid id
#if defined(PROTOCOL14)
		nTemp = RFID_08C_DATA_LENGTH;
		*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

		memcpy(carrBuff,BkSram_SystemInfo.ucarrRFIDUID,RFID_08C_DATA_LENGTH);
		memset((char *)carrTemp, ' ', 64);
        memcpy((char *)carrTemp, carrBuff, RFID_08C_DATA_LENGTH);
		for(int i = 0; i < RFID_08C_DATA_LENGTH; i++) {
            sprintf((char *)&paMessageBuffer[*pnPos], "%02X", carrTemp[i]);
            *pnPos += 2;
        }

        *pnDataLength += RFID_08C_DATA_LENGTH;
#else	//PROTOCOL14
        memcpy(carrBuff,"AABBCCDD",8);
		memset((char *)carrTemp, ' ', 64);
        memcpy((char *)carrTemp, carrBuff, 8);
		for(int i = 0; i < 8; i++) {
            sprintf((char *)&paMessageBuffer[*pnPos], "%02X", carrTemp[i]);
            *pnPos += 2;
        }

        *pnDataLength += 8;
#endif	//PROTOCOL14
	}
	}
#endif //#if defined(PROTOCOL13)




	return;
}

void MakeTripReporing2(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer,stReportAfterDrivingInfo* pstReportAfterDrivingInfo )
{
	float fTemp;
	uint32_t nTemp;
	uint32_t wTemp;
//	long long llValue;

    // MONI 2018-02-26
    // changed time format with unix time stamp
	// 주행 시작 시간(Local)
	wTemp = pstReportAfterDrivingInfo->StartDrivingLocalTime;
	*pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 주행 완료 시간(Local)
	wTemp = pstReportAfterDrivingInfo->StopDrivingLocalTime;
	*pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 주행 시작 시간(UTC)
	wTemp = pstReportAfterDrivingInfo->StartDrivingUTCTime;
	*pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 주행 완료 시간(UTC)
	wTemp = pstReportAfterDrivingInfo->StopDrivingUTCTime;
	*pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// max speed 					--> 05E1
	fTemp = pstReportAfterDrivingInfo->MaxSpeed;
	*pnDataLength += ConversionFloatToHexDec_2(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// Avg speed
	nTemp = pstReportAfterDrivingInfo->AvgSpeed;
	*pnDataLength += ConversionFloatToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// max rpm 						--> C350
	fTemp = pstReportAfterDrivingInfo->MaxRpm;
	*pnDataLength += ConversionFloatToHexDec_2(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// Avg rpm
	nTemp = pstReportAfterDrivingInfo->AvgRpm;
	*pnDataLength += ConversionFloatToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 급가속 횟수 				--> 0005
	nTemp = pstReportAfterDrivingInfo->RapidAccelCount;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 급감속 횟수 				--> 0009
	nTemp = pstReportAfterDrivingInfo->RapidDecelCount;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 공회전 시간
	nTemp = pstReportAfterDrivingInfo->FakeDrivingTime;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 워밍업 시간
	nTemp = pstReportAfterDrivingInfo->WarmUpTime;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 연료소비량 				--> 0F42BB
	fTemp = pstReportAfterDrivingInfo->ConsumedFuel*1000000;
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);

    // car batter
    nTemp = pstReportAfterDrivingInfo->CarBattery;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

#if defined(PROTOCOL16)
	nTemp = pstReportAfterDrivingInfo->RemainedECarBattery;	//배터리잔량SOC
	pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportAfterDrivingInfo->SOH;	//배터리노화진행률
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportAfterDrivingInfo->ExtraDrivingDistance;	//주행가능거리
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportAfterDrivingInfo->HighBatteryTemperature;	//고전압배터리온도 MIN
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

#if defined(PROTOCOL18)
	nTemp = pstReportAfterDrivingInfo->HighBatteryTemperatureMax;	//고전압배터리온도 MAX
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif

	nTemp = pstReportAfterDrivingInfo->OutsideTemperature;	//외기온도
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportAfterDrivingInfo->AvrMotorRpm;	//평균모터RPM
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportAfterDrivingInfo->MaxMotorRpm;	//최고모터RPM
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	fTemp = pstReportAfterDrivingInfo->ConsumedBattery*1000000;	//배터리소모량
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);

#if defined(PROTOCOL17)
	fTemp = pstReportAfterDrivingInfo->ConsumedBattery_Regen*1000000;	//배터리소모량
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif

	nTemp = pstReportAfterDrivingInfo->m_usChargeCount;	//배터리충전횟수
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	fTemp = pstReportAfterDrivingInfo->m_usChargeTime;	//배터리충전시간
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportAfterDrivingInfo->m_bChargeState;	//배터리충전상태
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = (uint16_t)pstReportAfterDrivingInfo->GpsDirection;	//GPS진행방향
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif //PROTOCOL16

	// 주행시간 					--> 000E10
	wTemp = pstReportAfterDrivingInfo->DrivingTime;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 이동거리 					--> 001388
	wTemp = pstReportAfterDrivingInfo->DrivingDistance;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);

//	// 엔진오일 온도 			--> 07D5
//	fTemp = Get_CoolantTemperature();
//	*pnDataLength += ConversionFloatToHexDec_2(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 기어포지션 				--> 50
	nTemp = pstReportAfterDrivingInfo->GeerPosition;//nTemp = (uint16_t)'P';
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

//	// 미션오일 온도 			--> 05DF
//	fTemp = Get_TransmissionOil_Temperature();
//	*pnDataLength += ConversionFloatToHexDec_2(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 연료잔량 					--> 43
	nTemp = pstReportAfterDrivingInfo->RemainedFuel;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	//배터리잔량
     nTemp = pstReportAfterDrivingInfo->RemainedECarBattery;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// start GPS 상태 		--> 01
	nTemp = pstReportAfterDrivingInfo->StartGpsValid;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 시동 on 경도 			--> 000793BFC8
	fTemp = pstReportAfterDrivingInfo->PowerOnLongitude;
	*pnDataLength += ConversionGPSDataToHexDec(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 시동 on 위도 			--> 000793BFC8
	fTemp = pstReportAfterDrivingInfo->PowerOnLatitude;
	*pnDataLength += ConversionGPSDataToHexDec(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// End GPS 상태 			--> 01
	nTemp = pstReportAfterDrivingInfo->EndGpsValid;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 시동 off 경도 			--> 0007B3F450
	fTemp = pstReportAfterDrivingInfo->EndGpsLongitude;

	*pnDataLength += ConversionGPSDataToHexDec(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 시동 off 위도 			--> 0007B3F450
	fTemp = pstReportAfterDrivingInfo->EndGpsLatitude;
	*pnDataLength += ConversionGPSDataToHexDec(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 총 이동거리 				--> 000005
	wTemp = pstReportAfterDrivingInfo->Odometer;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);

#if defined(PROTOCOL16)
	//출발시 ODO
	wTemp = pstReportAfterDrivingInfo->StartOdometer;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif

//	// 40km/h 미만 갯수 	--> 00000C
//	wTemp = pstReportAfterDrivingInfo->SpdCntUnder40km;
//	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
//
//	// 40 ~ 80km/h 갯수 	--> 00000D
//	wTemp = pstReportAfterDrivingInfo->SpdCntUpper40kmUnder80km;
//	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
//
//	// 80 ~ 120km/h 갯수 	--> 00000E
//	wTemp = pstReportAfterDrivingInfo->SpdCntUpper80kmUnder120km;
//	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
//
//	// 120 ~ 160km/h 갯수 --> 00000F
//	wTemp = pstReportAfterDrivingInfo->SpdCntUpper120kmUnder160km;
//	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
//
//	// 160km/h 이상 			--> 000010
//	wTemp = pstReportAfterDrivingInfo->SpdCntUpper141km;
//	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 0km/h 갯수
	wTemp = pstReportAfterDrivingInfo->SpdCnt0km;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 10km/h 미만 갯수
	wTemp = pstReportAfterDrivingInfo->SpdCntUpper1kmUnder10km;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 10 ~ 20km/h 갯수
	wTemp = pstReportAfterDrivingInfo->SpdCntUpper11kmUnder20km;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 20 ~ 30km/h 갯수
	wTemp = pstReportAfterDrivingInfo->SpdCntUpper21kmUnder30km;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 30 ~ 40km/h 갯수
	wTemp = pstReportAfterDrivingInfo->SpdCntUpper31kmUnder40km;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 40 ~ 50km/h 갯수
	wTemp = pstReportAfterDrivingInfo->SpdCntUpper41kmUnder50km;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 50 ~ 60km/h 갯수
	wTemp = pstReportAfterDrivingInfo->SpdCntUpper51kmUnder60km;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 60 ~ 70km/h 갯수
	wTemp = pstReportAfterDrivingInfo->SpdCntUpper61kmUnder70km;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 70 ~ 80km/h 갯수
	wTemp = pstReportAfterDrivingInfo->SpdCntUpper71kmUnder80km;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 80 ~ 90km/h 갯수
	wTemp = pstReportAfterDrivingInfo->SpdCntUpper81kmUnder90km;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 90 ~ 100km/h 갯수
	wTemp = pstReportAfterDrivingInfo->SpdCntUpper91kmUnder100km;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 100 ~ 110km/h 갯수
	wTemp = pstReportAfterDrivingInfo->SpdCntUpper101kmUnder110km;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 110 ~ 120km/h 갯수
	wTemp = pstReportAfterDrivingInfo->SpdCntUpper111kmUnder120km;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 120 ~ 130km/h 갯수
	wTemp = pstReportAfterDrivingInfo->SpdCntUpper121kmUnder130km;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 130 ~ 140km/h 갯수
	wTemp = pstReportAfterDrivingInfo->SpdCntUpper131kmUnder140km;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 141km/h 이상
	wTemp = pstReportAfterDrivingInfo->SpdCntUpper141km;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);

    wTemp = pstReportAfterDrivingInfo->AccIdlingCount;
    *pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);

    wTemp = pstReportAfterDrivingInfo->AccStopCount;
    *pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);

    wTemp = pstReportAfterDrivingInfo->AccBreakeCount;
    *pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);

    wTemp = pstReportAfterDrivingInfo->AccInertiaDrivingCount;
    *pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);

    wTemp = pstReportAfterDrivingInfo->AccNormalDrivingCount;
    *pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
#if defined(PROTOCOL17)
	wTemp = pstReportAfterDrivingInfo->AccExtraDrivingCount;
    *pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif
	// 타이어 공기압: 0 									--> 000000000000: 타이어 공기압
	// 전좌/전우/후좌/후우(12자리), 소수점을 없애기 위해서 10을 곱해준다.
	// 23.5/23.5/23.5/23.5 일 경우, 235/235/235/235 --> 2byte/2byte/2byte/2byte --> 을 최종 적으로 사용
	nTemp = pstReportAfterDrivingInfo->TpmsFL;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportAfterDrivingInfo->TpmsFR;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportAfterDrivingInfo->TpmsRL;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstReportAfterDrivingInfo->TpmsRR;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

    // interiorlamp
    nTemp = pstReportAfterDrivingInfo->InteriorLamp;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

    // headlight
    nTemp = pstReportAfterDrivingInfo->HeadLight;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// Master DB version
	nTemp = g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// Slave DB version
	nTemp = g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// Driving DB version
	nTemp = g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// Bootloader version
	nTemp = g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// Application version
	nTemp = g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

//#if defined(PROTOCOL13)
    {
    char carrBuff[128]={0,};
    char carrTemp[64]={0,};
    DCSServiceType nServiceType;

#if defined(PROTOCOL16)
	memset((char *)carrTemp, ' ', sizeof(carrTemp));
	memcpy((char *)carrTemp, g_FirmwareInfo.AppProperty[eApp_SlaveDB].arrFWName, MAX_FW_DB_FILE_NAME);
	for(int i = 0; i < MAX_FW_DB_FILE_NAME; i++)
	{
		sprintf((char *)&paMessageBuffer[*pnPos], "%02X", carrTemp[i]);
		*pnPos += 2;
	}
	*pnDataLength += MAX_FW_DB_FILE_NAME;

	memset((char *)carrTemp, ' ', sizeof(carrTemp));
	memcpy((char *)carrTemp, g_FirmwareInfo.AppProperty[eApp_ControlDB].arrFWName, MAX_FW_DB_FILE_NAME);
	for(int i = 0; i < MAX_FW_DB_FILE_NAME; i++)
	{
		sprintf((char *)&paMessageBuffer[*pnPos], "%02X", carrTemp[i]);
		*pnPos += 2;
	}
	*pnDataLength += MAX_FW_DB_FILE_NAME;
#endif //PROTOCOL16

#if defined(PROTOCOL19)
	// FD Bootloader version
	nTemp = m_stCFDCtrl.stVer.usBlVer;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// FD Application version
	nTemp = m_stCFDCtrl.stVer.usAppVer;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif	// defined(PROTOCOL19)
#if defined(PROTOCOL22)
	nTemp = pstReportAfterDrivingInfo->LowBatterySOC;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif 

#if defined(PROTOCOL23)
	nTemp = pstReportAfterDrivingInfo->HydrogenTemperature; 
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);
    
	fTemp = pstReportAfterDrivingInfo->HydrogenTankPress * 10;
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);
    
	wTemp = pstReportAfterDrivingInfo->HydrogenChargeCnt; 
	*pnDataLength += ConversionDWordToHexDec_4(wTemp, pnPos, &paMessageBuffer[*pnPos]);
    
	fTemp = pstReportAfterDrivingInfo->FuelcellVoltLow * 100; // Battery Cell Voltage Min
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);
    
	fTemp = pstReportAfterDrivingInfo->FuelcellVoltHigh * 100; // Battery Cell Voltage Max
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);
    
	fTemp = pstReportAfterDrivingInfo->HydrogenFuel * 10; 
	*pnDataLength += ConversionFloatToHexDec_4(fTemp, pnPos, &paMessageBuffer[*pnPos]);
    
	fTemp = pstReportAfterDrivingInfo->AirPurification * 10; 
	*pnDataLength += ConversionDWordToHexDec_4((uint32_t)fTemp, pnPos, &paMessageBuffer[*pnPos]);
    
	fTemp = pstReportAfterDrivingInfo->CO2Reduction * 10; // CO2 
	*pnDataLength += ConversionDWordToHexDec_4((uint32_t)fTemp, pnPos, &paMessageBuffer[*pnPos]);
    
	nTemp = pstReportAfterDrivingInfo->InsulationResistance; // insulation Resistance - EV Car
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);
    
	nTemp = pstReportAfterDrivingInfo->ModemRssi;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif
    // get service type fleet / retail
    GetAutolinkConfigProperty(eAutoLinkConfig_ServiceType,(void*)&nServiceType);

    // added protocol version 1.3
    if( nServiceType == DCS_Fleet )
    {
        // add type
        nTemp = 0xF117;
        *pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

        // add rfid id
//#if defined(PROTOCOL14)
		nTemp = RFID_08C_DATA_LENGTH;
		*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

		memcpy(carrBuff,BkSram_SystemInfo.ucarrRFIDUID,RFID_08C_DATA_LENGTH);
		memset((char *)carrTemp, ' ', 64);
        memcpy((char *)carrTemp, carrBuff, RFID_08C_DATA_LENGTH);
		for(int i = 0; i < RFID_08C_DATA_LENGTH; i++) {
            sprintf((char *)&paMessageBuffer[*pnPos], "%02X", carrTemp[i]);
            *pnPos += 2;
        }

        *pnDataLength += RFID_08C_DATA_LENGTH;
//#else	//PROTOCOL14
//        memcpy(carrBuff,"AABBCCDD",8);
//		memset((char *)carrTemp, ' ', 64);
//        memcpy((char *)carrTemp, carrBuff, 8);
//		for(int i = 0; i < 8; i++) {
//            sprintf((char *)&paMessageBuffer[*pnPos], "%02X", carrTemp[i]);
//            *pnPos += 2;
//        }
//
//        *pnDataLength += 8;
//#endif	//PROTOCOL14



    }
    }
//#endif //#if defined(PROTOCOL13)

	return;
}

int8_t MakeAlramEventKeyValue(uint8_t ucEventId,uint8_t* pucarrEventKeyValue,stCarReport carReport)
{
    float fEventKeyValue;
    uint32_t unEventKeyValue;
    int8_t cSize;

    switch( ucEventId )
    {
        case eMESSAGE_EVENT_KEY_NONE:
            break;
    	case eMESSAGE_EVENT_KEY_OVER_VOLTAGE_ALARM:
        case eMESSAGE_EVENT_KEY_LOW_VOLTAGE_ALARM:
            memcpy((char *)&unEventKeyValue,(char *)carReport.rpAlram.CarStatus.EventKeyValue,4);
    	    fEventKeyValue = (float)((float)unEventKeyValue/(float)10.0);
            sprintf((char *)pucarrEventKeyValue,"%.1f\x00",fEventKeyValue);
            cSize = strlen((char *)pucarrEventKeyValue);
            break;
        case eMESSAGE_EVENT_KEY_DOOR_LOCK_ALARM:
        case eMESSAGE_EVENT_KEY_DOOR_OPEN_ALARM:
        case eMESSAGE_EVENT_KEY_FUEL_RUN_OUT:
        case eMESSAGE_EVENT_KEY_VEHICLE_ENGINE_START_ALARM:
        case eMESSAGE_EVENT_KEY_TAIL_LAMP_ALARM:
        case eMESSAGE_EVENT_KEY_MIL_LAMP_ON:
		case eMESSAGE_EVENT_KEY_SECURITY_ALRAM:
        case eMESSAGE_EVENT_KEY_OVER_ENGINE_TEMP_ALARM:
        case eMESSAGE_EVENT_KEY_VEHICLE_OVER_SPEED_ALARM:
        case eMESSAGE_EVENT_KEY_VEHICLE_OVER_RPM_ALARM:
        case eMESSAGE_EVENT_KEY_PARKING_IMPACT:
    	case eMESSAGE_EVENT_KEY_AIRBAG_ALRAM:
        case eMESSAGE_EVENT_KEY_ENGKEEP_TIMEOVER_ALRAM:
        case eMESSAGE_EVENT_KEY_MODEM_POWER_OFF:
        case eMESSAGE_EVENT_KEY_CHARGE_ALRAM:
		case eMESSAGE_EVENT_KEY_ABSINDICATOR_ALRAM:
		case eMESSAGE_EVENT_KEY_EPBINDICATOR_ALRAM:
		case eMESSAGE_EVENT_KEY_ISGINDICATOR_ALRAM:
		case eMESSAGE_EVENT_KEY_BMSINDICATOR_ALRAM:
		case eMESSAGE_EVENT_KEY_BRAKEJUDDERINDICATOR_ALRAM:
		case eMESSAGE_EVENT_KEY_ENGOILINDICATOR_ALRAM:
		case eMESSAGE_EVENT_KEY_CHARGING_STATE:
		case eMESSAGE_EVENT_KEY_EXTRA_CHARGE_TIME:
		case eMESSAGE_EVENT_KEY_REARSEAT_ALRAM:
        case eMESSAGE_EVENT_KEY_FOB_STATUS_ALRAM:
            memcpy((char *)&unEventKeyValue,(char *)carReport.rpAlram.CarStatus.EventKeyValue,4);
            sprintf((char *)pucarrEventKeyValue,"%d\x00",unEventKeyValue);
            cSize = strlen((char *)pucarrEventKeyValue);
//            printf("==================================\r\n");
//            printf("key value : %s\r\n",pucarrEventKeyValue);
            break;
        case eMESSAGE_EVENT_KEY_VALET_ALRAM:
        case eMESSAGE_EVENT_KEY_GEO_FENCE_ALRAM:
            memcpy((char *)pucarrEventKeyValue,(char *)carReport.rpAlram.CarStatus.EventKeyValue,sizeof(carReport.rpAlram.CarStatus.EventKeyValue));
            cSize = strlen((char *)pucarrEventKeyValue);
            break;
    	case eMESSAGE_EVENT_KEY_TIRE_PRESSURE:
		case eMESSAGE_EVENT_KEY_FAHRENHEIT_ALRAM:
			memcpy((char *)pucarrEventKeyValue,(char *)carReport.rpAlram.CarStatus.EventKeyValue,sizeof(carReport.rpAlram.CarStatus.EventKeyValue));
            cSize = strlen((char *)pucarrEventKeyValue);
            break;
    	case eMESSAGE_EVENT_KEY_FOTA_COMPLETE_ALRAM:
		case eMESSAGE_EVENT_KEY_INDICATOR_ALRAM:
		case eMESSAGE_EVENT_KEY_STATE_ALRAM:
		case eMESSAGE_EVENT_KEY_SMSEXPIRE_ALRAM:
		case eMESSAGE_EVENT_KEY_CANFD_CANNOT_COMMUNICATION:
            memcpy((char *)pucarrEventKeyValue,(char *)carReport.rpAlram.CarStatus.EventKeyValue,sizeof(carReport.rpAlram.CarStatus.EventKeyValue));
            cSize = strlen((char *)pucarrEventKeyValue);
            break;
        case eMESSAGE_EVENT_KEY_TOWING_ALRAM:
            break;
    }

    return cSize;
}

boolean_t IsInitializeTime(unsigned int unLocalTime)
{
    stHalRTCTypeDef stDate;
    unsigned int nInitTime; // initialize time

    stDate.RtcDate.RTC_Year = 17;
    stDate.RtcDate.RTC_Month = 12;
    stDate.RtcDate.RTC_Date = 1;
    stDate.RtcDate.RTC_WeekDay = HAL_RTC_Weekday_Friday;
    stDate.RtcTime.RTC_Hours = 0;
    stDate.RtcTime.RTC_Minutes = 0;
    stDate.RtcTime.RTC_Seconds = 0;

    nInitTime = GetTimefromDate2(stDate);

    if( (int)((int)unLocalTime - (int)nInitTime) < 0 )
	{
        return true;
	}

    return false;
}


void MakeAlarmEvent2(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer,stMsgMdm* pstMsgMdm)
{
//	long long llValue;
	uint8_t arrTemp[128];
    int8_t cSize;
    uint32_t nTemp;
    uint32_t wTemp;

    if( IsInitializeTime(pstMsgMdm->carReport.rpAlram.CarStatus.OccurredEventTime) == true )
    {
        Trace("========================================\r\n");
        Trace("event time is wrong\r\n");

		pstMsgMdm->carReport.rpAlram.CarStatus.OccurredEventTime = GetLocalTimefromTime(GetUTCTime());
        pstMsgMdm->carReport.rpAlram.CarStatus.OccurredEventUtcTime = GetUTCTime();
    }

	// 발생시간 : 6
	//GetDateTimeStamp(arrTemp, pstMsgMdm->header.stDate.date, pstMsgMdm->header.stDate.time);
	//llValue = atoll((char *)arrTemp);
	//*pnDataLength += ConversionLongLongToHexDec_6(llValue, pnPos, &paMessageBuffer[*pnPos]);
	wTemp = pstMsgMdm->carReport.rpAlram.CarStatus.OccurredEventTime;
	*pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

    // MONI 2018-03-14
    // utc
    wTemp = pstMsgMdm->carReport.rpAlram.CarStatus.OccurredEventUtcTime;
	*pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 이벤트 키 : 2
	nTemp = pstMsgMdm->carReport.rpAlram.CarStatus.EventKey;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

#if defined(PROTOCOL16)
	// 이벤트 값 : 42
	memset(arrTemp,0,sizeof(arrTemp));
    cSize = MakeAlramEventKeyValue(nTemp,arrTemp,pstMsgMdm->carReport);
	*pnDataLength += ConversionMultiByteToHexStr_n2(cSize,arrTemp, MAX_EVENT_KEY_VALUE, pnPos, &paMessageBuffer[*pnPos]);
#else
	// 이벤트 값 : 28
	memset(arrTemp,0,28*2);
    cSize = MakeAlramEventKeyValue(nTemp,arrTemp,pstMsgMdm->carReport);
	*pnDataLength += ConversionMultiByteToHexStr_n2(cSize,arrTemp, 28, pnPos, &paMessageBuffer[*pnPos]);
#endif

#if defined(PROTOCOL15)
	// odometer
	nTemp = pstMsgMdm->carReport.rpAlram.CarStatus.Odometer;
	*pnDataLength += ConversionDWordToHexDec_3(nTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif

    // current latitude :5
	float fTemp = pstMsgMdm->carReport.rpAlram.CarStatus.GpsCurLatitude;//pstReportAfterDrivingInfo->EndGpsLatitude/100;
	*pnDataLength += ConversionGPSDataToHexDec(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// current longitude :5
	fTemp = pstMsgMdm->carReport.rpAlram.CarStatus.GpsCurLongitude;//pstReportAfterDrivingInfo->EndGpsLongitude/100;
	*pnDataLength += ConversionGPSDataToHexDec(fTemp, pnPos, &paMessageBuffer[*pnPos]);

    // event latitude :5
    fTemp = pstMsgMdm->carReport.rpAlram.CarStatus.GpsSetLatitude;//pstReportAfterDrivingInfo->EndGpsLongitude/100;
	*pnDataLength += ConversionGPSDataToHexDec(fTemp, pnPos, &paMessageBuffer[*pnPos]);

    // event longitude : 5
    fTemp = pstMsgMdm->carReport.rpAlram.CarStatus.GpsSetLongitude;//pstReportAfterDrivingInfo->EndGpsLongitude/100;
	*pnDataLength += ConversionGPSDataToHexDec(fTemp, pnPos, &paMessageBuffer[*pnPos]);

    // bound type : 1
	if(pstMsgMdm->carReport.rpAlram.CarStatus.Boundtype == eREMOTE_CON_BOUNDTYPE_IN ){
		nTemp = 'I';
	}
	else if(pstMsgMdm->carReport.rpAlram.CarStatus.Boundtype == eREMOTE_CON_BOUNDTYPE_OUT){
		nTemp = 'O';
	}
    else if(pstMsgMdm->carReport.rpAlram.CarStatus.Boundtype == eREMOTE_CON_BOUNDTYPE_BOTH){
		nTemp = 'B';
	}
    else
    {
        printf("Not Support Geofence Type\r\n");
        nTemp = 'I';
    }

	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

    // distance : 3
    arrTemp[0] = (pstMsgMdm->carReport.rpAlram.CarStatus.Distance>>16)&0xff;
    arrTemp[1] = (pstMsgMdm->carReport.rpAlram.CarStatus.Distance>>8)&0xff;
    arrTemp[2] = pstMsgMdm->carReport.rpAlram.CarStatus.Distance&0xff;
    //memcpy((char*)arrTemp,(char*)&pstMsgMdm->carReport.rpAlram.CarStatus.Distance,4);
    *pnDataLength += ConversionMultiByteToHexDec_n(arrTemp,3,pnPos,&paMessageBuffer[*pnPos]);
#if defined(PROTOCOL23)
	nTemp = pstMsgMdm->carReport.rpAlram.CarStatus.ModemRssi;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif
#if defined(PROTOCOL13)
    {
    char carrBuff[128]={0,};
    char carrTemp[64]={0,};
    DCSServiceType nServiceType;

    // get service type fleet / retail
    GetAutolinkConfigProperty(eAutoLinkConfig_ServiceType,(void*)&nServiceType);

    // added protocol version 1.3
    if( nServiceType == DCS_Fleet )
    {
        // add type
        nTemp = 0xF117;
        *pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

        // add rfid id
#if defined(PROTOCOL14)
		nTemp = RFID_08C_DATA_LENGTH;
		*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

		memcpy(carrBuff,BkSram_SystemInfo.ucarrRFIDUID,RFID_08C_DATA_LENGTH);
		memset((char *)carrTemp, ' ', 64);
        memcpy((char *)carrTemp, carrBuff, RFID_08C_DATA_LENGTH);
		for(int i = 0; i < RFID_08C_DATA_LENGTH; i++) {
            sprintf((char *)&paMessageBuffer[*pnPos], "%02X", carrTemp[i]);
            *pnPos += 2;
        }

        *pnDataLength += RFID_08C_DATA_LENGTH;
#else	//PROTOCOL14
        memcpy(carrBuff,"AABBCCDD",8);
		memset((char *)carrTemp, ' ', 64);
        memcpy((char *)carrTemp, carrBuff, 8);
		for(int i = 0; i < 8; i++) {
            sprintf((char *)&paMessageBuffer[*pnPos], "%02X", carrTemp[i]);
            *pnPos += 2;
        }

        *pnDataLength += 8;
#endif	//PROTOCOL14

    }
    }
#endif //#if defined(PROTOCOL13)

#if defined(DEBUG_CAN_PARSING)
    Trace("!!!!!!!!!!!!!!!!!!!!!!!!!!  EVENT  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
    Trace("1. EVENT TIME       :  %lld\r\n", llValue);
    Trace("2. eEventKey           : %d \r\n", MessageEventData.eEventKey);
    Trace("3. llEeventValue       : %lld \r\n",  GetVehicleEventKey());
    Trace("!!!!!!!!!!!!!!!!!!!!!!!!!!  finish  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
    Trace("0. Get_DrivingKey       : %lld \r\n", Get_DrivingKey());
    Trace("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
#endif

	return;
}


void MakeRequestVehicleStatus(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer, stReportSmartKey* pstSmartKey)
{
	//float fTemp;
//	long long llValue;
//	uint8_t arrTemp[64];
	uint16_t i;
	uint32_t nTemp;
    uint32_t wTemp;
    //uint64_t dllTemp;
    double dlTemp;
    stMsgMdm stMessage;

	Trace("@MakeRequestVehicleStatus()\r\n");
	hexdump(MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);
    GetLastSmartMessageInfomation(&stMessage);
    hexdump(stMessage.carReport.rpSmartKey.Request.Guid,MAX_GUID_LENGTH);

	// GUID
	for(i = 0; i < MAX_GUID_LENGTH; i++) {
		sprintf((char *)&paMessageBuffer[*pnPos], "%x", stMessage.carReport.rpSmartKey.Request.Guid[i]);
		*pnPos += 2;
	}
	*pnDataLength += MAX_GUID_LENGTH / 2;

    // battery
	nTemp = pstSmartKey->Status.CarBattery;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// interior lamp
	nTemp = 0;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 헤드라이트
	nTemp = pstSmartKey->Status.HeadLight;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// Door Lock Status
	nTemp = pstSmartKey->Status.DoorLockStatus;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// Door Open Status
	nTemp = pstSmartKey->Status.DoorOpenStatus;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// ACC Status
	nTemp = pstSmartKey->Status.AccStatus;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 위도
	dlTemp = pstSmartKey->Status.Latitude;
	*pnDataLength += ConversionGPSDataToHexDec(dlTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 경도
	dlTemp = pstSmartKey->Status.Longitude;
	*pnDataLength += ConversionGPSDataToHexDec(dlTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 발생시간
	wTemp = GetLocalTimefromTime(GetUTCTime());
	*pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	return;
}

void MakeRemoteControlRequest2(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer, stReportSmartKey* pstSmartKey)
{
    stMsgMdm stMessage;
//	long long llValue;
//	uint8_t arrTemp[64];
	uint16_t i;
	uint32_t nTemp;
	uint32_t wTemp;
	char arrTemp[100]={0,};
	//uint16_t nSize=0;

    Trace("@MakeRemoteControlRequest()\r\n");
#if false
    Trace("CarReportGUID:\r\n");
	hexdump(pstSmartKey->Request.Guid, MAX_GUID_LENGTH);
    Trace("MessageDataGUID:\r\n");
    hexdump(MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);
#endif
	// GUID
	for(i = 0; i < MAX_GUID_LENGTH; i++) {
		sprintf((char *)&paMessageBuffer[*pnPos], "%x", pstSmartKey->Request.Guid[i]);
		*pnPos += 2;
	}
	*pnDataLength += MAX_GUID_LENGTH / 2;

	// 발생시간
	wTemp = GetLocalTimefromTime(GetUTCTime());
	*pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

    GetLastSmartMessageInfomation(&stMessage);

    // sms rcv time
    wTemp = stMessage.carReport.rpSmartKey.Request.OccurredEventTime;
    //*pnDataLength += ConversionUnsignedIntToHexDec_6(pstSmartKey->Request.OccurredEventTime, pnPos, &paMessageBuffer[*pnPos]);
    *pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 결과값
	nTemp = 1;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	// 실패원인
	wTemp = 0;
	*pnDataLength += ConversionDWordToHexDec_4(wTemp, pnPos, &paMessageBuffer[*pnPos]);

#if defined(PROTOCOL18)
	for(i = 0; i < 32 ; i++) {
		arrTemp[i]=' ';
		sprintf((char *)&paMessageBuffer[*pnPos], "%x", arrTemp[i]);
		*pnPos += 2;
	}
	*pnDataLength += 64;
#endif

	return;
}


void MakeRemoteControlReport2(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer, stReportSmartKey* pstReport)
{
    stMsgMdm stModemMessage;
//	long long llValue;
	uint8_t arrTemp[64]={0,};
	uint16_t i;
    uint32_t wTemp;

    // MONI 20180810 Bug Fixed.
    // copy request guid to send to server.
	//for(i = 0; i < MAX_GUID_LENGTH; i++) {
	//	sprintf((char *)&paMessageBuffer[*pnPos], "%x", MessageManagerData.aRemoteControlGUID[i]);
	//	*pnPos += 2;
	//}
	for(i = 0; i < MAX_GUID_LENGTH; i++) {
		sprintf((char *)&paMessageBuffer[*pnPos], "%x", pstReport->Response.Guid[i]);
		*pnPos += 2;
	}

	*pnDataLength += MAX_GUID_LENGTH / 2;

	// 발생시간
	wTemp = GetLocalTimefromTime(GetUTCTime());
	*pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

//#warning "we need to add check guid between last smart key message and response message"
    GetLastSmartMessageInfomation(&stModemMessage);

    // 발생시간
    // MONI 2018-02-26
    // changed time format with unix time stamp
    wTemp = pstReport->Response.OccurredEventTime;
    *pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 결과값
	*pnDataLength += ConversionIntToHexDec_1(pstReport->Response.Result, pnPos, &paMessageBuffer[*pnPos]);

#if defined(REASON_8BYTE)
	*pnDataLength += ConversionDWordToHexDec_8(pstReport->Response.Reason, pnPos, &paMessageBuffer[*pnPos]);	//reason type change 210824
#else
    *pnDataLength += ConversionDWordToHexDec_4(pstReport->Response.Reason, pnPos, &paMessageBuffer[*pnPos]);
#endif


#if defined(PROTOCOL18)
	if(pstReport->Response.BTControlKey[0] ==' ') // BT Key 값이 빈값일 경우
	{
		for(i = 0; i < 32 ; i++) {
			arrTemp[i]=' ';
			sprintf((char *)&paMessageBuffer[*pnPos], "%x", arrTemp[i]);
			*pnPos += 2;
		}
		*pnDataLength += 64;
	}
	else
	{
		ConversionMultiByteToHexStr_n2(16,&pstReport->Response.BTControlKey[0], 16, 0, arrTemp);
		*pnDataLength += ConversionMultiByteToHexStr_n2(32,&arrTemp[0], 32, pnPos, &paMessageBuffer[*pnPos]);
	}
#endif

//endif
	return;
}

void MakeBTRemoteControlReport(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer, stReportSmartKey* pstReport)
{
    stMsgMdm stModemMessage;
//	long long llValue;
	uint8_t arrTemp[64];
//	uint16_t i;
    uint32_t wTemp;

    // MONI 20180810 Bug Fixed.
    // copy request guid to send to server.
	//for(i = 0; i < MAX_GUID_LENGTH; i++) {
	//	sprintf((char *)&paMessageBuffer[*pnPos], "%x", MessageManagerData.aRemoteControlGUID[i]);
	//	*pnPos += 2;
	//}

	//BT 제어키
	ConversionMultiByteToHexStr_n2(16,&pstReport->Response.BTControlKey[0], 16, 0, arrTemp);
	*pnDataLength += ConversionMultiByteToHexStr_n2(32,&arrTemp[0], 32, pnPos, &paMessageBuffer[*pnPos]);

	//명령 구분
	*pnDataLength += ConversionIntToHexDec_1(pstReport->Response.CommandType, pnPos, &paMessageBuffer[*pnPos]);
	//제어 구분
	*pnDataLength += ConversionIntToHexDec_1(pstReport->Response.ControlType, pnPos, &paMessageBuffer[*pnPos]);

	*pnDataLength += ConversionIntToHexDec_1(pstReport->Response.KeepPowerOnTime, pnPos, &paMessageBuffer[*pnPos]);
	*pnDataLength += ConversionIntToHexDec_2(pstReport->Response.Temperature, pnPos, &paMessageBuffer[*pnPos]);
	*pnDataLength += ConversionIntToHexDec_1(pstReport->Response.CheckTemperature, pnPos, &paMessageBuffer[*pnPos]);
	*pnDataLength += ConversionIntToHexDec_1(pstReport->Response.Defrost, pnPos, &paMessageBuffer[*pnPos]);

//#warning "we need to add check guid between last smart key message and response message"
    GetLastSmartMessageInfomation(&stModemMessage);

    // 발생시간
    // MONI 2018-02-26
    // changed time format with unix time stamp
    wTemp = pstReport->Response.OccurredEventTime;
    *pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 결과값
	*pnDataLength += ConversionIntToHexDec_1(pstReport->Response.Result, pnPos, &paMessageBuffer[*pnPos]);
#if defined(REASON_8BYTE)
	*pnDataLength += ConversionDWordToHexDec_8(pstReport->Response.Reason, pnPos, &paMessageBuffer[*pnPos]);	//reason type change 210824
#else
	*pnDataLength += ConversionDWordToHexDec_4(pstReport->Response.Reason, pnPos, &paMessageBuffer[*pnPos]);
#endif

	return;
}

//#define ENABLE_DTC_LOG

extern uint8_t g_ucFreezeFrameData[];

void MakeDTCError(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer,stReportDtc* pstDtc)
{
    //uint16_t unTempSize;
    uint16_t unDtcPos;
    uint8_t ucSystemCount;
    //uint16_t unDtcSize;
    uint32_t nTemp;
    uint32_t wTemp;
    //long long llTemp;
    //long long llValue;
    //uint8_t arrTemp[64];
    uint8_t ucDtcFreezeCount;
    //uint16_t unDtcFreezeSize;
    uint16_t unDtcFreezeLength;

    // event time
    wTemp = pstDtc->OccurredEventTime;
    *pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// MIL Status
    nTemp = pstDtc->MILStatus;
    *pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

#if defined(PROTOCOL13)
    {
    char carrBuff[128]={0,};
    char carrTemp[64]={0,};
    DCSServiceType nServiceType;

    // get service type fleet / retail
    GetAutolinkConfigProperty(eAutoLinkConfig_ServiceType,(void*)&nServiceType);

    // added protocol version 1.3
    if( nServiceType == DCS_Fleet )
    {
        // add type
        nTemp = 0xF117;
        *pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

        // add rfid id
#if defined(PROTOCOL14)
		nTemp = RFID_08C_DATA_LENGTH;
		*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

		memcpy(carrBuff,BkSram_SystemInfo.ucarrRFIDUID,RFID_08C_DATA_LENGTH);
		memset((char *)carrTemp, ' ', 64);
        memcpy((char *)carrTemp, carrBuff, RFID_08C_DATA_LENGTH);
		for(int i = 0; i < RFID_08C_DATA_LENGTH; i++) {
            sprintf((char *)&paMessageBuffer[*pnPos], "%02X", carrTemp[i]);
            *pnPos += 2;
        }

        *pnDataLength += RFID_08C_DATA_LENGTH;
#else	//PROTOCOL14
        memcpy(carrBuff,"AABBCCDD",8);
		memset((char *)carrTemp, ' ', 64);
        memcpy((char *)carrTemp, carrBuff, 8);
		for(int i = 0; i < 8; i++) {
            sprintf((char *)&paMessageBuffer[*pnPos], "%02X", carrTemp[i]);
            *pnPos += 2;
        }

        *pnDataLength += 8;
#endif	//PROTOCOL14
    }
    }
#endif //#if defined(PROTOCOL13)

    // dtc response type
    nTemp = pstDtc->ResponseType;
    *pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

    unDtcPos = (pstDtc->Index * DTC_DIVIDE_MEMORY_SIZE);

    // total length
    memcpy((char*)&nTemp,&g_ucFreezeFrameData[unDtcPos],2);
    *pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

    unDtcPos += 2;

    // system count
    ucSystemCount = g_ucFreezeFrameData[unDtcPos];
    *pnDataLength += ConversionIntToHexDec_1(ucSystemCount, pnPos, &paMessageBuffer[*pnPos]);

    unDtcPos += 1;

#ifdef ENABLE_DTC_LOG
    Trace("DTC System Count : %d\r\n",ucSystemCount);
#endif
    for(int i=0;i<ucSystemCount;i++)
    {
    	// ECUID
    	g_ucFreezeFrameData[unDtcPos+4]=' ';
        g_ucFreezeFrameData[unDtcPos+5]=' ';
        g_ucFreezeFrameData[unDtcPos+6]=' ';
     	*pnDataLength += ConversionMultiByteToHexDec_n(&g_ucFreezeFrameData[unDtcPos], 7, pnPos, &paMessageBuffer[*pnPos]);
#ifdef ENABLE_DTC_LOG
        hexdump(&paMessageBuffer[*pnPos-14],16);
#endif
        unDtcPos+=7;

        // function type
        memcpy((char*)&nTemp,&g_ucFreezeFrameData[unDtcPos],2);
        *pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

#ifdef ENABLE_DTC_LOG
        Trace("function type : %d\r\n",nTemp);
#endif
        unDtcPos+=2;

    	// Status
    	nTemp = g_ucFreezeFrameData[unDtcPos];
    	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

#ifdef ENABLE_DTC_LOG
        Trace("status : %d\r\n",nTemp);
#endif
        unDtcPos+=1;

        // can uds flag
        nTemp = g_ucFreezeFrameData[unDtcPos];
    	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

#ifdef ENABLE_DTC_LOG
        Trace("can uds flag : %d\r\n",nTemp);
#endif
        unDtcPos+=1;

    	// odometer
    	memcpy((char*)&wTemp,&g_ucFreezeFrameData[unDtcPos],4);
    	*pnDataLength += ConversionDWordToHexDec_4(wTemp, pnPos, &paMessageBuffer[*pnPos]);

#ifdef ENABLE_DTC_LOG
        Trace("odometer : %d\r\n",wTemp);
#endif
        unDtcPos+=4;

#ifdef ENABLE_DTC_LOG
        Trace("Dtc Rtc Time\r\n");
        hexdump(&paMessageBuffer[*pnPos],32);
#endif
        // dtc occurred time
        //MONI 20180430
        // we should check obd dtc time this is temporary
        // because in this time, rtc time is 0 so rtc time will be sent default weired value.
        //wTemp = GetTimefromDate2(stDate);
        wTemp = GetLocalTimefromTime(GetUTCTime());
        *pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

        unDtcPos+=6;

        // freeze count
        ucDtcFreezeCount = g_ucFreezeFrameData[unDtcPos];
        *pnDataLength += ConversionIntToHexDec_1(ucDtcFreezeCount, pnPos, &paMessageBuffer[*pnPos]);

#ifdef ENABLE_DTC_LOG
        Trace("freeze count : %d\r\n",ucDtcFreezeCount);
#endif
        unDtcPos+=1;

//        Trace("DTC System Dtc Count : %d\r\n",ucDtcFreezeCount);
        for(int j=0;j<ucDtcFreezeCount;j++)
        {
            //hexdump(&g_ucFreezeFrameData[unDtcPos],16);

            //dtc
            *pnDataLength += ConversionMultiByteToHexDec_n2(&g_ucFreezeFrameData[unDtcPos],4, pnPos, &paMessageBuffer[*pnPos]);
//            hexdump(&paMessageBuffer[*pnPos],32);

            unDtcPos+=4;

            // function type
            memcpy((char*)&nTemp,&g_ucFreezeFrameData[unDtcPos],2);
            *pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

#ifdef ENABLE_DTC_LOG
            Trace("function type : %d\r\n",nTemp);
#endif
            unDtcPos+=2;

            // Status
            *pnDataLength += ConversionIntToHexDec_1(g_ucFreezeFrameData[unDtcPos], pnPos, &paMessageBuffer[*pnPos]);

#ifdef ENABLE_DTC_LOG
            Trace("status : %d\r\n",g_ucFreezeFrameData[unDtcPos]);
#endif
            unDtcPos+=1;

            // freeze frame length
            memcpy((char*)&unDtcFreezeLength,&g_ucFreezeFrameData[unDtcPos],2);
            *pnDataLength += ConversionIntToHexDec_2(unDtcFreezeLength, pnPos, &paMessageBuffer[*pnPos]);

#ifdef ENABLE_DTC_LOG
            Trace("freeze frame length : %d\r\n",unDtcFreezeLength);
#endif
            unDtcPos+=2;

            // real freeze frame value
            *pnDataLength += ConversionMultiByteToHexDec_n(&g_ucFreezeFrameData[unDtcPos],unDtcFreezeLength, pnPos, &paMessageBuffer[*pnPos]);

            unDtcPos += unDtcFreezeLength;
        }
    }

	return;
}

void MakeEngineStart(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer)
{
#if false
    uint32_t nTemp;
	uint32_t wTemp;

	// 알람마스크 세팅값
//#warning "MSG: 알람마스크 세팅값"
	nTemp = 1;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 주기 보고 간격
//#warning "MSG: 주기 보고 간격"
	wTemp = 60;				// 엔진 정지 시, 주기 보고 간격, 단위: 초
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 고장 주행거리
//#warning "MSG: 엔진 스타트 이후의 주기 보고 간격, 단위: 초"
	wTemp = 60;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// Valet alarm 거리
//#warning "MSG: Valet alarm 거리"
	wTemp = 4;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// Valet alarm 엔진 시동시간
//#warning "MSG: Valet alarm 엔진 시동시간"
	wTemp = 5;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// Towing alarm 거리
//#warning "MSG: Towing alarm 거리"
	wTemp = 6;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);

	// Towing alarm 시동시간 값
//#warning "MSG: Towing alarm 시동시간 값"
	wTemp = 7;
	*pnDataLength += ConversionDWordToHexDec_3(wTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif
	return;
}

void MakeGetGeofence(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer)
{
	uint16_t i;

	Trace("@MakeGetGeofece()\r\n");
    hexdump(MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);

	// GUID
	for(i = 0; i < MAX_GUID_LENGTH; i++) {
		sprintf((char *)&paMessageBuffer[*pnPos], "%x", MessageManagerData.aRemoteControlGUID[i]);
		*pnPos += 2;
	}
	*pnDataLength += MAX_GUID_LENGTH / 2;

	return;
}

void MakeRsvEngCtrl(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer, stMsgMdm* pstMsgMdm)
{
//	stMsgMdm stModemMessage;
//	long long llValue;
//	uint8_t arrTemp[64]={0,};
	uint16_t i;
	uint32_t wTemp;
	stRsvEngCntorl RsvEngCntorlGUID;
	// MONI 20180810 Bug Fixed.
	// copy request guid to send to server.
	//for(i = 0; i < MAX_GUID_LENGTH; i++) {
	//	sprintf((char *)&paMessageBuffer[*pnPos], "%x", MessageManagerData.aRemoteControlGUID[i]);
	//	*pnPos += 2;
	//}
	memset(&RsvEngCntorlGUID, 0x00, sizeof(stRsvEngCntorl));

	GetRsvEngCtrlSetting(&RsvEngCntorlGUID);

	if (memcmp(pstMsgMdm->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.ucGroupGUID,RsvEngCntorlGUID.ucGroupGUID,MAX_GUID_LENGTH) != 0) //recent GUID Compare
	{
		memcpy(pstMsgMdm->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.ucGroupGUID, RsvEngCntorlGUID.ucGroupGUID,MAX_GUID_LENGTH);        
		pstMsgMdm->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.unRcvSMSTime = RsvEngCntorlGUID.unRcvSMSTime;
	}
	
	if(IsValidRsvEngCtrlGUID(pstMsgMdm->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.ucGroupGUID)== false)
	//if(IsValidRsvEngCtrlGUID(m_stUserActionSetting.RsvEngCtrl.ucGroupGUID)== false)
	{
        // 예약시동이 없는 경우에는 GUID값에 빈 GUID를 넣어줘야 함.
        // 0x20으로 GUID를 체워서 보내야지 빈 GUID로 보내는것으로 서버에서 인식 함.
		for(i = 0; i < MAX_GUID_LENGTH; i++) {
			sprintf((char *)&paMessageBuffer[*pnPos], "%x", 0x20);
			*pnPos += 2;
		}
		*pnDataLength += MAX_GUID_LENGTH / 2;

		// rsv time
		wTemp = 0;
		*pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

		// result
		*pnDataLength += ConversionIntToHexDec_1(0, pnPos, &paMessageBuffer[*pnPos]);

		// reason
		*pnDataLength += ConversionIntToHexDec_1(0, pnPos, &paMessageBuffer[*pnPos]);
	}
	else
	{
		for(i = 0; i < MAX_GUID_LENGTH; i++) 
		{
			sprintf((char *)&paMessageBuffer[*pnPos], "%x",
			pstMsgMdm->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.ucGroupGUID[i]);
			*pnPos += 2;
		}
		*pnDataLength += MAX_GUID_LENGTH / 2;

		// rsv time
		wTemp = pstMsgMdm->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.unRcvSMSTime;
		*pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);
		// result
		*pnDataLength += ConversionIntToHexDec_1(pstMsgMdm->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.Result, pnPos, &paMessageBuffer[*pnPos]);
		// reason
		*pnDataLength += ConversionIntToHexDec_1(pstMsgMdm->carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.Reason, pnPos, &paMessageBuffer[*pnPos]);
	}
	return;
}

void MakeRsvCtrtConductResult(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer, stMsgMdm* pstMsgMdm)
{
//	stMsgMdm stModemMessage;
//	long long llValue;
//	uint8_t arrTemp[64]={0,};
	uint16_t i;
//	uint32_t wTemp;

	for(i = 0; i < MAX_GUID_LENGTH; i++) {
		sprintf((char *)&paMessageBuffer[*pnPos], "%x",
		pstMsgMdm->carReport.rpSmartKey.Response.Guid[i]);
		*pnPos += 2;
	}

	*pnDataLength += MAX_GUID_LENGTH / 2;

	// result
	*pnDataLength += ConversionIntToHexDec_1(pstMsgMdm->carReport.rpSmartKey.Response.Result, pnPos, &paMessageBuffer[*pnPos]);

	// reason
#if defined(REASON_8BYTE)
	*pnDataLength += ConversionDWordToHexDec_8(pstMsgMdm->carReport.rpSmartKey.Response.Reason, pnPos, &paMessageBuffer[*pnPos]);	//reason type change 210824
#else
	*pnDataLength += ConversionDWordToHexDec_4(pstMsgMdm->carReport.rpSmartKey.Response.Reason, pnPos, &paMessageBuffer[*pnPos]);
#endif

	return;
}

void MakeSmsTest(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer)
{
	uint16_t i;

	Trace("@MakeSmsTest()\r\n");
    hexdump(MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);

	// GUID
	for(i = 0; i < MAX_GUID_LENGTH; i++) {
		sprintf((char *)&paMessageBuffer[*pnPos], "%x", MessageManagerData.aRemoteControlGUID[i]);
		*pnPos += 2;
	}
	*pnDataLength += MAX_GUID_LENGTH / 2;

	return;
}

void MakeAlarmMasking(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer)
{
	//#warning "MSG: 알람 마스킹 전문 제작 루틴 추가 할 것"

	return;
}

void MakePeriodInfomationSleep2(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer, stReportNoDrivingInfo* pstNoDrivingInfo)
{
	float fTemp;
	uint32_t nTemp;
    uint32_t wTemp;

	// GPS Validation
	nTemp = pstNoDrivingInfo->GpsValid;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 위도
	fTemp = pstNoDrivingInfo->GpsLatitude;
	*pnDataLength += ConversionGPSDataToHexDec(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 경도
	fTemp = pstNoDrivingInfo->GpsLongitude;
	*pnDataLength += ConversionGPSDataToHexDec(fTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 밧데리 전압
	nTemp = pstNoDrivingInfo->CarBattery;
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 총 이동거리: 63 								--> 00003F: 총 이동거리
	nTemp = pstNoDrivingInfo->Odometer;
	*pnDataLength += ConversionDWordToHexDec_3(nTemp, pnPos, &paMessageBuffer[*pnPos]);

#if defined(PROTOCOL18)
	nTemp = pstNoDrivingInfo->ExtraDrivingDistance;	//남은주행거리
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstNoDrivingInfo->RemainedECarBattery;	//배터리잔량SOC
	pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstNoDrivingInfo->HighBatteryTemperature;	//고전압배터리온도
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstNoDrivingInfo->HighBatteryTemperatureMax;	//고전압배터리온도
	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif

    // 모뎀 수신 레벨: -70                              --> 46: 모뎀 수신 레벨
    //nTemp = abs(ModemManagerData.ndBm);
    nTemp = pstNoDrivingInfo->ModemRssi;
    *pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 차량문 잠금 상태: 00010000(b) 				--> 10(0001 0000): 차량문 잠금 상태
	nTemp = pstNoDrivingInfo->DoorLockStatus;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	// 차량문 열림 상태
	nTemp = pstNoDrivingInfo->DoorOpenStatus;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

#if defined(PROTOCOL12)
	//ACC 상태(차량상태)
	nTemp = pstNoDrivingInfo->VehicleStatus;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
#endif
    // headlight
    nTemp = pstNoDrivingInfo->HeadLight;
    *pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

    // local time
    wTemp = pstNoDrivingInfo->OccurredEventTime;
    *pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

    // utc time
    wTemp = pstNoDrivingInfo->OccurredEventUtcTime;
    *pnDataLength += ConversionUnsignedIntToHexDec_6(wTemp, pnPos, &paMessageBuffer[*pnPos]);

#if defined(PROTOCOL13)
    {
    char carrBuff[128]={0,};
    char carrTemp[64]={0,};
    DCSServiceType nServiceType;

    // get service type fleet / retail
    GetAutolinkConfigProperty(eAutoLinkConfig_ServiceType,(void*)&nServiceType);

    // added protocol version 1.3
    if( nServiceType == DCS_Fleet )
    {
        // add type
        nTemp = 0xF117;
        *pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);

        // add rfid id
#if defined(PROTOCOL14)
		nTemp = RFID_08C_DATA_LENGTH;
		*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

		memcpy(carrBuff,BkSram_SystemInfo.ucarrRFIDUID,RFID_08C_DATA_LENGTH);
		memset((char *)carrTemp, ' ', 64);
        memcpy((char *)carrTemp, carrBuff, RFID_08C_DATA_LENGTH);
		for(int i = 0; i < RFID_08C_DATA_LENGTH; i++) {
            sprintf((char *)&paMessageBuffer[*pnPos], "%02X", carrTemp[i]);
            *pnPos += 2;
        }

        *pnDataLength += RFID_08C_DATA_LENGTH;
#else	//PROTOCOL14
        memcpy(carrBuff,"AABBCCDD",8);
		memset((char *)carrTemp, ' ', 64);
        memcpy((char *)carrTemp, carrBuff, 8);
		for(int i = 0; i < 8; i++) {
            sprintf((char *)&paMessageBuffer[*pnPos], "%02X", carrTemp[i]);
            *pnPos += 2;
        }

        *pnDataLength += 8;
#endif	//PROTOCOL14
    }
    }
#endif //#if defined(PROTOCOL13)

    // MONI 2018-03-07
    // added some properties and deleted some properties
    //  DisplayFirmWareInfo();
	// Master DB Version
//	nTemp = g_FirmwareInfo.AppProperty[eApp_MasterDB].nAppFWVersion;
//	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);
//	// Slave DB Version
//	nTemp = g_FirmwareInfo.AppProperty[eApp_SlaveDB].nAppFWVersion;
//	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);
//	// Control DB Version
//	nTemp = g_FirmwareInfo.AppProperty[eApp_ControlDB].nAppFWVersion;
//	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);
//	// bootloader Version
//	nTemp = g_FirmwareInfo.AppProperty[eApp_Bootloader].nAppFWVersion;
//	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);
//	// Application Version
//	nTemp = g_FirmwareInfo.AppProperty[eApp_Application].nAppFWVersion;
//	*pnDataLength += ConversionIntToHexDec_2(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	return;
}

//#define ENABLE_AES_DEBUG

bool ApplyEncryption(uint8_t *arrPlainText, uint8_t *arrEncryptionText, uint16_t *nEncryptionTextLen, uint16_t totalLength)
{
	DWORD key_schedule[60];
	BYTE Paddingtext[AES_TEXT_SIZE]={0,};
	uint8_t enc_buf[AES_TEXT_SIZE]={0,};
	BYTE iv[1][16] = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
	BYTE key[1][32] = {{'9','f','e','s','0','0','9','9','s','5','8','1','e','s','q','e','1','1','1','f','2','3','0','7','q','e','s','7','e','s','q','e'}};
	uint16_t nLen;
	uint16_t nBase64Len;

#ifdef       ENABLE_AES_DEBUG
	Trace("Enter ApplyEncryption()\r\n");
#endif

	//*******************************************************************************************************************
	// 원문
	//*******************************************************************************************************************
	//nLen = strlen((char *)arrPlainText);
	nLen = totalLength;
#ifdef       ENABLE_AES_DEBUG
	Trace("arrPlainText length: %d\r\n", nLen);
#endif
	if(nLen >= AES_TEXT_SIZE) {
		Trace("Over size Plain Text length\r\n");
		return false;
	}

#ifdef       ENABLE_AES_DEBUG
	Trace("Plaintext:\r\n");
	hexdump(arrPlainText, nLen);
#endif
	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// Key 값 세팅
	// 원래 키값은 "sef99900185seqsef11170327seqeqse" 이다.
	// 하지만, aes_key_setup() 함수에 입력 되는 key 배열에는
	// "9fes0099s581esqe111f2307qes7esq" 와 같이 4바이트 배열을 바꾸어서 적용해야 한다. big endian, little endian 변환
	//*******************************************************************************************************************
	aes_key_setup_new(key[0], key_schedule, 256);

#ifdef       ENABLE_AES_DEBUG
	Trace("Key:\r\n");
	hexdump(key[0], 32);
#endif

#ifdef       ENABLE_AES_DEBUG
	Trace("key_schedule :\r\n");
	hexdump((char *)key_schedule, 240);
#endif
	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// Init Vector
	//*******************************************************************************************************************
#ifdef       ENABLE_AES_DEBUG
	Trace("IV:\r\n");
	hexdump(iv[0], 16);
#endif
	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// padding 적용
	//*******************************************************************************************************************
	nLen = pkcs7_pad_write((char *)arrPlainText, 16, (char *)Paddingtext);

#ifdef       ENABLE_AES_DEBUG
	Trace("padding data nLen: %d\r\n", nLen);
	hexdump(Paddingtext, nLen);
#endif
	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// AES 암호화
	//*******************************************************************************************************************
	aes_encrypt_cbc_new(Paddingtext, nLen, enc_buf, key_schedule, 256, iv[0]);

#ifdef       ENABLE_AES_DEBUG
	Trace("-encrypted to:\r\n");
	hexdump(enc_buf, nLen);
#endif
	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// Encode base64
	//*******************************************************************************************************************
	if( mbedtls_base64_encode( arrEncryptionText, (size_t)AES_TEXT_SIZE, (size_t *)&nBase64Len, enc_buf, nLen ) != 0) {
		Trace( "Base64 Encode failed\r\n" );

		return false;
	}

	arrEncryptionText[nBase64Len] = 0x00;
	*nEncryptionTextLen = nBase64Len;

#ifdef       ENABLE_AES_DEBUG
	Trace("-base64 to:\r\n");
	hexdump(arrEncryptionText, nBase64Len);
#endif

	return true;
}

#define MAX_ENCRYPT_KEY_LENGTH 16
bool ApplyDecryption2(uint8_t *arrDecryptionText, uint16_t *nDecryptionTextLen, uint8_t *arrEncryptionText, uint16_t nEncryptionTextLen,DukptPinEntry* pstEncryptKeyEntry);

bool ApplyEncryption2(uint8_t *arrPlainText,uint16_t nPlainTextLength, uint8_t *arrEncryptionText, uint16_t *nEncryptionTextLen, DukptPinEntry* pstEncryptKeyEntry)
{
	uint8_t enc_buf[AES_TEXT_SIZE];
#ifdef ENABLE_CHECK_KMS_ENC
    uint8_t dec_buf[AES_TEXT_SIZE];
    uint16_t nLen2;
#endif
	BYTE iv[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint16_t nLen;

    // we will apply encryp process after we apply damo encryption process.
    int nEncryptResult = DAMO_CRYPT_AES_EncryptEx((unsigned char *)enc_buf, (size_t*)&nLen,
        (const unsigned char*)arrPlainText, nPlainTextLength,
        pstEncryptKeyEntry->req_enc_key, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, iv);

    if( nEncryptResult != 0 )
    {
        // erro encrypt
        Trace("Encrypt error : %d\r\n",nEncryptResult);
    }

//#define ENABLE_CHECK_KMS_ENC
#ifdef ENABLE_CHECK_KMS_ENC
    // we will apply encryp process after we apply damo encryption process.
    nEncryptResult = DAMO_CRYPT_AES_DecryptEx((unsigned char *)dec_buf, (size_t*)&nLen2,
        (const unsigned char*)enc_buf, nLen,
        pstEncryptKeyEntry->req_enc_key, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, iv);

    if( nEncryptResult != 0 )
    {
        // erro decrypt
        Trace("Decrpyt error : %d\r\n",nEncryptResult);
    }
#endif

    nEncryptResult = DAMO_CRYPT_Base64_Encode(arrEncryptionText,(size_t*)nEncryptionTextLen,enc_buf,nLen);

    if( nEncryptResult != 0 )
    {
        Trace("Base64 encrypt error : %d\r\n",nEncryptResult);
    }

#ifdef ENABLE_CHECK_KMS_ENC

    hexdump(arrEncryptionText,*nEncryptionTextLen);

    memset(dec_buf,0,sizeof(dec_buf));
    nEncryptResult = DAMO_CRYPT_Base64_Decode(dec_buf,(size_t*)&nLen,arrEncryptionText,(size_t)*nEncryptionTextLen);

    if( nEncryptResult != 0 )
    {
        Trace("Base64 decrypt error : %d\r\n",nEncryptResult);
    }

/*
    nEncryptResult = ApplyDecryption2(dec_buf,&nLen2,arrEncryptionText,*nEncryptionTextLen,pstEncryptKeyEntry);

    if( nEncryptResult != 0 )
    {
        Trace("decrypt error : %d\r\n",nEncryptResult);
    }
*/
#endif

    hexdump(arrEncryptionText, *nEncryptionTextLen);

	return true;
}

bool ApplyDecryption2(uint8_t *arrDecryptionText, uint16_t *nDecryptionTextLen, uint8_t *arrEncryptionText, uint16_t nEncryptionTextLen,DukptPinEntry* pstEncryptKeyEntry)
{
    int nEncryptResult;
	uint8_t dec_buf[AES_TEXT_SIZE];
	BYTE iv[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint16_t nLen;

    nEncryptResult = DAMO_CRYPT_Base64_Decode(dec_buf,(size_t*)&nLen,arrEncryptionText,nEncryptionTextLen);

    if( nEncryptResult != 0 )
    {
        Trace("Base64 decrypt error : %d\r\n",nEncryptResult);
    }

    // we will apply encryp process after we apply damo encryption process.
    nEncryptResult = DAMO_CRYPT_AES_DecryptEx((unsigned char *)arrDecryptionText, (size_t*)&nDecryptionTextLen,
        (const unsigned char*)dec_buf, nLen,
        pstEncryptKeyEntry->req_enc_key, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, iv);

    if( nEncryptResult != 0 )
    {
        // erro decrypt
        Trace("Decrpyt error : %d\r\n",nEncryptResult);
    }

	hexdump(arrDecryptionText, *nDecryptionTextLen);

	return true;
}

DukptPinEntry m_stEncryptKeyEntry;


void SetLastUsedDukptPinEntry(DukptPinEntry* pstEncryptKeyEntry)
{
    memcpy((char*)&m_stEncryptKeyEntry,(char*)pstEncryptKeyEntry,sizeof(DukptPinEntry));
}

void GetLastUsedDukptPinEntry(DukptPinEntry* pstEncryptKeyEntry)
{
    memcpy((char*)pstEncryptKeyEntry,(char*)&m_stEncryptKeyEntry,sizeof(DukptPinEntry));
}

bool ApplyEncryption3(uint8_t *arrPlainText,uint16_t nPlainTextLength, uint8_t *arrEncryptionText, uint16_t *nEncryptionTextLen, DukptPinEntry* pstEncryptKeyEntry)
{
	uint8_t enc_buf[AES_TEXT_SIZE];
#ifdef ENABLE_CHECK_KMS_ENC
    uint8_t dec_buf[AES_TEXT_SIZE];
    uint8_t dec_buf2[AES_TEXT_SIZE];
#endif
	uint32_t nLen;
    uint32_t nBase64EncryptLen;

    //uint16_t nLen2;

    // we will apply encryp process after we apply damo encryption process.
    /*
    int nEncryptResult = DAMO_CRYPT_AES_EncryptEx((unsigned char *)arrEncryptionText, (size_t*)nEncryptionTextLen,
        (const unsigned char*)arrPlainText, nPlainTextLength,
        pstEncryptKeyEntry->req_enc_key, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, NULL);
    */

#ifdef ENABL_KMS_LOG
    Trace("DEK Key(req_enc_key) :\n");
    hexdump(pstEncryptKeyEntry->req_enc_key,16);
	Trace("KSN :\n");
    hexdump(pstEncryptKeyEntry->ksn,10);
	Trace("enc_pin_block :\n");
    hexdump(pstEncryptKeyEntry->enc_pin_block,8);
	Trace("pin_enc_key :\n");
    hexdump(pstEncryptKeyEntry->pin_enc_key,16);
	Trace("req_mac_key :\n");
    hexdump(pstEncryptKeyEntry->req_mac_key,16);
	Trace("res_mac_key :\n");
    hexdump(pstEncryptKeyEntry->res_mac_key,16);
	Trace("res_enc_key :\n");
    hexdump(pstEncryptKeyEntry->res_enc_key,16);
	printf("arrPlainText:%d\r\n",nPlainTextLength);
	hexdump(arrPlainText,nPlainTextLength);
#endif //#ifdef ENABL_KMS_LOG

#ifdef USE_KMS_BASE64_ENCRYPT
    int nEncryptResult = DAMO_CRYPT_EncryptEx((unsigned char *)enc_buf, (size_t*)&nLen,
        (const unsigned char*)arrPlainText, nPlainTextLength,
        pstEncryptKeyEntry->req_enc_key, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, NULL, 0);

    if( nEncryptResult != 0 )
    {
        // erro encrypt
        Trace("Encrypt error : %d\r\n",nEncryptResult);
    }
#ifdef ENABL_KMS_LOG
		printf("Encrypted_Plantext:%d\r\n",nLen);
		hexdump(enc_buf,nLen);
#endif //#ifdef ENABL_KMS_LOG

    nEncryptResult = DAMO_CRYPT_Base64_Encode( arrEncryptionText, (size_t *)&nBase64EncryptLen, enc_buf, nLen);

#ifdef ENABL_KMS_LOG
		printf("Encrypted_Base64:%d\r\n",nBase64EncryptLen);
		hexdump(arrEncryptionText,nBase64EncryptLen);
#endif //#ifdef ENABL_KMS_LOG

    // copy size
    *nEncryptionTextLen = (uint16_t)nBase64EncryptLen;

	//nEncryptResult = mbedtls_base64_encode( arrEncryptionText, (size_t)AES_TEXT_SIZE, (size_t *)nEncryptionTextLen, enc_buf, nLen );

    if( nEncryptResult != 0 )
    {
        // erro decrypt
        Trace("Base 64 Encrypt error : %d\r\n",nEncryptResult);
    }

#else //#ifdef USE_KMS_BASE64_ENCRYPT
    int nEncryptResult = DAMO_CRYPT_EncryptEx((unsigned char *)arrEncryptionText, (size_t*)nEncryptionTextLen,
        (const unsigned char*)arrPlainText, nPlainTextLength,
        pstEncryptKeyEntry->req_enc_key, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, NULL, 0);

    if( nEncryptResult != 0 )
    {
        // erro encrypt
        Trace("Encrypt error : %d\r\n",nEncryptResult);
    }

#endif //#ifdef USE_KMS_BASE64_ENCRYPT

#ifdef ENABLE_CHECK_KMS_ENC
    // we will apply encryp process after we apply damo encryption process.
    /*
    nEncryptResult = DAMO_CRYPT_AES_DecryptEx((unsigned char *)dec_buf, (size_t*)&nLen,
        (const unsigned char*)arrEncryptionText, *nEncryptionTextLen,
        pstEncryptKeyEntry->req_enc_key, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, NULL);
    */
    nEncryptResult = DAMO_CRYPT_Base64_Decode(dec_buf,(size_t*)&nLen,arrEncryptionText,(size_t)*nEncryptionTextLen);
	
	printf("Decrypted_Base64:%d\r\n",nLen);
	hexdump(dec_buf,nLen);
	
    *nEncryptionTextLen = nLen;
    nEncryptResult = DAMO_CRYPT_DecryptEx((unsigned char *)dec_buf2, (size_t*)&nLen,
        (const unsigned char*)dec_buf, *nEncryptionTextLen,
        pstEncryptKeyEntry->req_enc_key, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, NULL,0);

    if( nEncryptResult != 0 )
    {
        // erro decrypt
        Trace("Decrpyt error : %d\r\n",nEncryptResult);
    }

    Trace("Decrypt Data :%d\n",nLen);
    hexdump(dec_buf2, nLen);
	*nEncryptionTextLen = nBase64EncryptLen;
#endif

#ifdef ENABL_KMS_LOG
    Trace("Encrypt Data %d:\n",nBase64EncryptLen);
    hexdump(arrEncryptionText, nBase64EncryptLen);
#endif //#ifdef ENABL_KMS_LOG

	return true;
}

bool ApplyDecryption3(uint8_t *arrDecryptionText, uint16_t *nDecryptionTextLen, uint8_t *arrEncryptionText, uint16_t nEncryptionTextLen,DukptPinEntry* pstEncryptKeyEntry)
{
    int nEncryptResult;
	uint8_t dec_buf[AES_TEXT_SIZE];
	uint32_t nLen;
    uint32_t nDecryptLen;

#ifdef USE_KMS_BASE64_ENCRYPT
    nLen = AES_TEXT_SIZE;
    nEncryptResult = DAMO_CRYPT_Base64_Decode( dec_buf, (size_t *)&nLen, arrEncryptionText, nEncryptionTextLen);

	//nEncryptResult = mbedtls_base64_decode( dec_buf, sizeof( dec_buf ), (size_t *)&nLen, arrEncryptionText, nEncryptionTextLen );

    if( nEncryptResult != 0 )
    {
        // erro decrypt
        Trace("Base 64 Decrypt error : %d\r\n",nEncryptResult);
		return false;
    }
#endif //#ifdef USE_KMS_BASE64_ENCRYPT

    // we will apply encryp process after we apply damo encryption process.
    /*
    nEncryptResult = DAMO_CRYPT_AES_DecryptEx((unsigned char *)arrDecryptionText, (size_t*)nDecryptionTextLen,
        (const unsigned char*)arrEncryptionText, nEncryptionTextLen,
        pstEncryptKeyEntry->req_enc_key, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, NULL);
    */

#ifdef USE_KMS_BASE64_ENCRYPT
    nEncryptResult = DAMO_CRYPT_DecryptEx((unsigned char *)arrDecryptionText, (size_t*)&nDecryptLen,
            (const unsigned char*)dec_buf, nLen,
            pstEncryptKeyEntry->req_enc_key, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, NULL, 0);

    // copy size
    *nDecryptionTextLen = (uint16_t)nDecryptLen;

#else //#ifdef USE_KMS_BASE64_ENCRYPT
    nEncryptResult = DAMO_CRYPT_DecryptEx((unsigned char *)arrDecryptionText, (size_t*)nDecryptionTextLen,
            (const unsigned char*)arrEncryptionText, nEncryptionTextLen,
            pstEncryptKeyEntry->req_enc_key, MAX_ENCRYPT_KEY_LENGTH, AES_128, CBC_MODE, NULL, 0);
#endif //#ifdef USE_KMS_BASE64_ENCRYPT
    if( nEncryptResult != 0 )
    {
        // erro decrypt
        Trace("Decrpyt error : %d\r\n",nEncryptResult);
		return false;
    }
#ifdef ENABL_KMS_LOG
	hexdump(arrDecryptionText, *nDecryptionTextLen);
#endif //#ifdef ENABL_KMS_LOG

	return true;
}


uint16_t ApplyDecryption(uint8_t *pMessage, uint16_t nLen)
{
    uint8_t Base64Text[AES_TEXT_SIZE];
    uint16_t nBase64Len;
    uint8_t Decryptedtext[AES_TEXT_SIZE];
    uint16_t nEncryptedTextLen;

	DWORD key_schedule[60];
	BYTE iv[1][16] = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
	BYTE key[1][32] = {{'9','f','e','s','0','0','9','9','s','5','8','1','e','s','q','e','1','1','1','f','2','3','0','7','q','e','s','7','e','s','q','e'}};

#ifdef       ENABLE_AES_DEBUG
	Trace("Enter ApplyDecryption()\r\n");
#endif
	//*******************************************************************************************************************
	// 데이터를 Decode base64 한다.
	//*******************************************************************************************************************
  memset((char *)Base64Text, 0x00, AES_TEXT_SIZE);

	if(nLen <= 11) {
		Trace("MSG: error message length(%d)\r\n", nLen);
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

#ifdef       ENABLE_AES_DEBUG
	Trace("Rcv Message:\r\n");
	hexdump(Base64Text, nBase64Len);
#endif

#ifdef       ENABLE_AES_DEBUG
	Trace("Decode Base64(server)\r\n");
#endif
	if( mbedtls_base64_decode( Decryptedtext, sizeof( Decryptedtext ), (size_t *)&nEncryptedTextLen, Base64Text, nBase64Len ) != 0) {
		Trace( "Base64 Decode failed\r\n" );

		return 0;
	}

#ifdef       ENABLE_AES_DEBUG
	hexdump((char *)Decryptedtext, nEncryptedTextLen);
#endif
	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// Key 값 세팅
	// 원래 키값은 "sef99900185seqsef11170327seqeqse" 이다.
	// 하지만, aes_key_setup() 함수에 입력 되는 key 배열에는
	// "9fes0099s581esqe111f2307qes7esq" 와 같이 4바이트 배열을 바꾸어서 적용해야 한다. big endian, little endian 변환
	//*******************************************************************************************************************
	memset((char *)key_schedule, 0x00, 240);
	aes_key_setup_new(key[0], key_schedule, 256);

#ifdef       ENABLE_AES_DEBUG
	Trace("Key          :\r\n");
	hexdump(key[0], 32);

	Trace("key_schedule :\r\n");
	hexdump((char *)key_schedule, 240);
#endif
	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// 서버에서 받은 암호화 데이터를 복호화
	//*******************************************************************************************************************
	aes_decrypt_cbc_new(Decryptedtext, nEncryptedTextLen, pMessage, key_schedule, 256, iv[0]);

#ifdef       ENABLE_AES_DEBUG
	Trace("Plaintext(server):\r\n");
	hexdump(Decryptedtext, nEncryptedTextLen);
#endif

  nEncryptedTextLen = pkcs7_pad_read((char *)pMessage, nEncryptedTextLen);

#ifdef       ENABLE_AES_DEBUG
	Trace("Plaintext(pkcs7):\r\n");
	hexdump(pMessage, nEncryptedTextLen);
#endif

	return nEncryptedTextLen;
}

int MakePayload(eMESSAGE_TYPE eMsgType,uint16_t nPayloadPos, uint8_t *strMessageBuffer, stMsgMdm* pstMsgMdm)
{
    uint8_t CheckSum = 0;
    uint16_t nPos = nPayloadPos;
    uint16_t nDataLength =0;
    uint8_t arrTemp[64];
    uint32_t nTemp;
    uint16_t i;
	//********************************
	// Data - Body
	//********************************
	// 운행키: 운행 키는 시스템 power on 시에 망접속 이후에 현재 읽어 오는 명령인 AT^SIND=, AT+CCLK? 처리할때 저장해 둔다.
	// Modem_comm.c의 1122 행에서 저장한다.
	// 이때 전송 순번도 clear 해둔다.
	switch(eMsgType) {
		case eMESSAGE_TYPE_PERIOD_INFORMATION:
            //Trace("%s] report Body1 Sequence Num ::%d\n",__FUNCTION__,pstMsgMdm->carReport.rpInterval.DrivingInfo.DrivingInfoB1.SequnceNumber);
		case eMESSAGE_TYPE_TRIP_REPORTING:
        case eMESSAGE_TYPE_ALARM_EVENT:
            //Trace("%s] report Driving key  : %x\n",__FUNCTION__,pstMsgMdm->header.drivingKey);

            //key_display(Get_DrivingKey());
            //key_display(pstMsgMdm->header.drivingKey);
            //nDataLength += ConversionLongLongToHexDec_6(pstMsgMdm->header.drivingKey, &nPos, &strMessageBuffer[nPos]);
            nDataLength += ConversionLongLongToHexDec_6_key((unsigned long int)(pstMsgMdm->header.drivingKey>>32),
                                                    (unsigned long int)(pstMsgMdm->header.drivingKey),
                                                    &nPos, &strMessageBuffer[nPos]);
			break;
        case eMESSAGE_TYPE_DTC_ERROR:			// DTC 고장
            // MONI 2018-08-24
            // added dirving key in dtc structure.
            nDataLength += ConversionLongLongToHexDec_6_key((unsigned long int)(pstMsgMdm->carReport.rpAlram.Dtc.OperationKey>>32),
                                                    (unsigned long int)(pstMsgMdm->carReport.rpAlram.Dtc.OperationKey),
                                                    &nPos, &strMessageBuffer[nPos]);
            break;
		case eMESSAGE_TYPE_REMOTE_CONTROL_REQUEST:
		case eMESSAGE_TYPE_REMOTE_CONTROL_REPORT:
#if defined(PROTOCOL18)
		case eMESSAGE_TYPE_BT_REMOTE_CONTROL_REPORT:
#endif
		case eMESSAGE_TYPE_CURRENT_VEHICLE_STATUS:
		case eMESSAGE_TYPE_ALARM_MASKING:			// 알람 마스킹
		case eMESSAGE_TYPE_PERIOD_INFORMATION_SLEEP:
		case eMESSAGE_TYPE_ENGINE_START:
        case eMESSAGE_TYPE_SETTING_GEOFENCE:
        case eMESSAGE_TYPE_SETTING_POLYGON_GEOFENCE:
        case eMESSAGE_TYPE_SMS_TEST:
        case eMESSAGE_TYPE_REQUEST_MODEM_ACTIVATE:
        case eMESSAGE_TYPE_RESPONSE_MODEM_ACTIVATE:
        case eMESSAGE_TYPE_IPEK_PHASE1:
        case eMESSAGE_TYPE_IPEK_PHASE2:
        case eMESSAGE_TYPE_REQUEST_SENSOR_INITIALIZE:
        case eMESSAGE_TYPE_RESPONSE_SENSOR_INITIALIZE:
#if defined(PROTOCOL17)
		case eMESSAGE_TYPE_REQUEST_SETURL:
		case eMESSAGE_TYPE_RESPONSE_SETURL:
		case eMESSAGE_TYPE_RESPONSE_SETURL_COMPLETE:
		case eMESSAGE_TYPE_RESPONSE_INITURL_COMPLETE:
#endif
#if defined(PROTOCOL18)
		case eMESSAGE_TYPE_REQUEST_RESERVATION_ENGINE_CONTROL_SETTING:
		case eMESSAGE_TYPE_RESPONSE_RESERVATION_ENGINE_CONTROL_SETTING:
		case eMESSAGE_TYPE_RESPONSE_RESERVATION_ENGINE_CONTROL_RESULT:
		case eMESSAGE_TYPE_CHARGING_REPORT:
#endif
#if defined(PROTOCOL24)
        case eMESSAGE_TYPE_MODEM_STATUS_REPORT:
#endif
#if defined(PROTOCOL25)
        case eMESSAGE_TYPE_INSTALLATION_NETWORK_CHECK_TEST:
        case eMESSAGE_TYPE_INSTALLATION_SMS_CHECK_TEST:
#endif
        default:
			break;
	}

	switch(eMsgType) {
		case eMESSAGE_TYPE_PERIOD_INFORMATION:
			MakePeriodInfomation2(&nDataLength, &nPos, strMessageBuffer, &pstMsgMdm->carReport.rpInterval.DrivingInfo.DrivingInfoB1);
			break;
		case eMESSAGE_TYPE_TRIP_REPORTING:
			MakeTripReporing2(&nDataLength, &nPos, strMessageBuffer,&pstMsgMdm->carReport.rpInterval.AfterDrivingInfo);
			break;
		case eMESSAGE_TYPE_ALARM_EVENT:
			MakeAlarmEvent2(&nDataLength, &nPos, strMessageBuffer,pstMsgMdm);
			break;
		case eMESSAGE_TYPE_REMOTE_CONTROL_REQUEST:
			MakeRemoteControlRequest2(&nDataLength, &nPos, strMessageBuffer, &pstMsgMdm->carReport.rpSmartKey);
			break;
		case eMESSAGE_TYPE_CURRENT_VEHICLE_STATUS:
			MakeRequestVehicleStatus(&nDataLength, &nPos, strMessageBuffer, &pstMsgMdm->carReport.rpSmartKey);
			break;
		case eMESSAGE_TYPE_REMOTE_CONTROL_REPORT:
			MakeRemoteControlReport2(&nDataLength, &nPos, strMessageBuffer, &pstMsgMdm->carReport.rpSmartKey);
			break;
#if defined(PROTOCOL18)
		case eMESSAGE_TYPE_BT_REMOTE_CONTROL_REPORT: // PDH
			MakeBTRemoteControlReport(&nDataLength, &nPos, strMessageBuffer, &pstMsgMdm->carReport.rpSmartKey);
			break;
		case eMESSAGE_TYPE_REQUEST_RESERVATION_ENGINE_CONTROL_SETTING: // PWB
			break;
		case eMESSAGE_TYPE_RESPONSE_RESERVATION_ENGINE_CONTROL_SETTING: // PWB
			MakeRsvEngCtrl(&nDataLength, &nPos, strMessageBuffer,pstMsgMdm);
			break;
		case eMESSAGE_TYPE_RESPONSE_RESERVATION_ENGINE_CONTROL_RESULT:
			MakeRsvCtrtConductResult(&nDataLength, &nPos, strMessageBuffer, pstMsgMdm);
			break;
		case eMESSAGE_TYPE_CHARGING_REPORT: // PDH
			MakeChargingReport(&nDataLength, &nPos, strMessageBuffer, &pstMsgMdm->carReport.rpInterval.ChargingInfo);
			break;
#endif
		case eMESSAGE_TYPE_DTC_ERROR:			// DTC 고장
			MakeDTCError(&nDataLength, &nPos, strMessageBuffer, &pstMsgMdm->carReport.rpAlram.Dtc);
			break;
		case eMESSAGE_TYPE_ALARM_MASKING:			// 알람 마스킹
			MakeAlarmMasking(&nDataLength, &nPos, strMessageBuffer);
			break;
		//case eMESSAGE_TYPE_ENGINE_START:			// engine start
		//	MakeEngineStart(&nDataLength, &nPos, strMessageBuffer);
		//	break;
		case eMESSAGE_TYPE_PERIOD_INFORMATION_SLEEP:
			MakePeriodInfomationSleep2(&nDataLength, &nPos, strMessageBuffer, &pstMsgMdm->carReport.rpInterval.NoDrivingInfo);
			break;
        case eMESSAGE_TYPE_SETTING_GEOFENCE:
            MakeGetGeofence(&nDataLength, &nPos, strMessageBuffer);
            break;
        case eMESSAGE_TYPE_SETTING_POLYGON_GEOFENCE:
            MakeGetGeofence(&nDataLength, &nPos, strMessageBuffer);
            break;
        case eMESSAGE_TYPE_SMS_TEST:
            MakeSmsTest(&nDataLength, &nPos, strMessageBuffer);
            break;
        case eMESSAGE_TYPE_IPEK_PHASE1:
            MakeIpekPhase1(&nDataLength, &nPos, strMessageBuffer);
            break;
        case eMESSAGE_TYPE_IPEK_PHASE2:
            MakeIpekPhase2(&nDataLength, &nPos, strMessageBuffer);
            break;
        case eMESSAGE_TYPE_ENGINE_START:
            MakeReqSettingInfo(&nDataLength, &nPos, strMessageBuffer);
            break;
#if defined(PROTOCOL12)
        case eMESSAGE_TYPE_REQUEST_MODEM_ACTIVATE:
            MakeReqModemActive(&nDataLength, &nPos, strMessageBuffer);
            break;
        case eMESSAGE_TYPE_RESPONSE_MODEM_ACTIVATE:
            MakeRspModemActive(&nDataLength, &nPos, strMessageBuffer,&pstMsgMdm->carReport.rpSetting);
            break;
#endif

#if defined(PROTOCOL15)
        case eMESSAGE_TYPE_REQUEST_SENSOR_INITIALIZE:
            MakeReqSensorInitialize(&nDataLength, &nPos, strMessageBuffer);
            break;
        case eMESSAGE_TYPE_RESPONSE_SENSOR_INITIALIZE:
            MakeRspSensorInitialize(&nDataLength, &nPos, strMessageBuffer,&pstMsgMdm->carReport.rpSetting);
            break;
#endif
#if defined(PROTOCOL17)
		case eMESSAGE_TYPE_REQUEST_SETURL:
		case eMESSAGE_TYPE_RESPONSE_SETURL:
			MakeReqSetURL(&nDataLength, &nPos, strMessageBuffer, &pstMsgMdm->carReport.rpSmartKey);
			break;
		case eMESSAGE_TYPE_RESPONSE_SETURL_COMPLETE:
		case eMESSAGE_TYPE_RESPONSE_INITURL_COMPLETE:
			MakeReqSetURLComplete(&nDataLength, &nPos, strMessageBuffer, &pstMsgMdm->carReport);
			break;
#endif
		case eMESSAGE_TYPE_TRACKING_REQUEST:
			MakeRequestTrackingInfo(&nDataLength, &nPos, strMessageBuffer, pstMsgMdm);
			break;
		case eMESSAGE_TYPE_TRACKING_RESPONSE:
			MakeResponseTrackingInfo(&nDataLength, &nPos, strMessageBuffer, pstMsgMdm);
			break;
		case eMESSAGE_TYPE_TRACKING_DATA:
			MakeSendTrackingInfo(&nDataLength, &nPos, strMessageBuffer, pstMsgMdm);
			break;
#if defined(PROTOCOL24)
	   case eMESSAGE_TYPE_MODEM_STATUS_REPORT:
	   		MakeReportNetworkStatus(&nDataLength, &nPos, strMessageBuffer, &pstMsgMdm->carReport.rpModemStatus);
	   		break;
#endif
#if defined(PROTOCOL25)
       case eMESSAGE_TYPE_INSTALLATION_NETWORK_CHECK_TEST:
            MakeSmsTest(&nDataLength, &nPos, strMessageBuffer);
            break;
       case eMESSAGE_TYPE_INSTALLATION_SMS_CHECK_TEST:
            MakeSmsTest(&nDataLength, &nPos, strMessageBuffer);
            break;
#endif
	}

    //Trace("MSG: data length: %d\n", nDataLength);

    // set body length to header
	sprintf((char *)arrTemp, "%04X", nDataLength);
	memcpy((char *)&strMessageBuffer[OFFSET_POS_DATA_LENGTH], arrTemp, 4);

    // checksum (STX ~ 전송 순번까지 xor 한다.)
    for(i = 0; i < nDataLength; i++) {
        CheckSum = CheckSum ^ strMessageBuffer[OFFSET_POS_DATA_LENGTH + i];
    }

    nTemp = CheckSum;
    ConversionIntToHexDec_1(nTemp, &nPos, &strMessageBuffer[nPos]);

    // 33: 전문끝
    nTemp = MSG_STR_ETX;
    ConversionIntToHexDec_1(nTemp, &nPos, &strMessageBuffer[nPos]);

    strMessageBuffer[nPos] = 0x00; /* NULL */

//    hexdump(strMessageBuffer, nPos);

    return nPos;
}

void MakeRequestTrackingInfo(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer, stMsgMdm* pstMsgMdm)
{
	uint16_t i;

	for(i = 0; i < MAX_GUID_LENGTH; i++) {
		sprintf((char *)&paMessageBuffer[*pnPos], "%x",
		pstMsgMdm->carReport.rpSmartKey.Request.Guid[i]);
		*pnPos += 2;
	}

	hexdump(pstMsgMdm->carReport.rpSmartKey.Request.Guid,MAX_GUID_LENGTH);
	*pnDataLength += MAX_GUID_LENGTH / 2;
}

void MakeResponseTrackingInfo(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer, stMsgMdm* pstMsgMdm)
{
	uint16_t i;
	uint32_t nTemp=0;

	for(i = 0; i < MAX_GUID_LENGTH; i++) {
		sprintf((char *)&paMessageBuffer[*pnPos], "%x",
		pstMsgMdm->carReport.rpSmartKey.Response.Guid[i]);
		*pnPos += 2;
	}

	hexdump(pstMsgMdm->carReport.rpSmartKey.Response.Guid,MAX_GUID_LENGTH);
	*pnDataLength += MAX_GUID_LENGTH / 2;

	nTemp = pstMsgMdm->carReport.rpSmartKey.Response.Result;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	nTemp = pstMsgMdm->carReport.rpSmartKey.Response.Reason;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);
	//*pnDataLength += ConversionDWordToHexDec_4(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	return;
}

void MakeSendTrackingInfo(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer, stMsgMdm* pstMsgMdm)
{
    uint16_t i;
    uint32_t nTemp=0;
    float fTemp=0;

    for(i = 0; i < MAX_GUID_LENGTH; i++) {
		sprintf((char *)&paMessageBuffer[*pnPos], "%x",
		pstMsgMdm->carReport.rpSetting.UserSetting.stUserActionSetting.TrackingInfo.ucArrGUID[i]);
		*pnPos += 2;
	}
	*pnDataLength += MAX_GUID_LENGTH / 2;

	nTemp = pstMsgMdm->carReport.rpSetting.UserSetting.stUserActionSetting.TrackingInfo.ucDataIndex;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

	nTemp = pstMsgMdm->carReport.rpSetting.UserSetting.stUserActionSetting.TrackingInfo.ucGpsValid;
	*pnDataLength += ConversionIntToHexDec_1(nTemp, pnPos, &paMessageBuffer[*pnPos]);

    fTemp = pstMsgMdm->carReport.rpSetting.UserSetting.stUserActionSetting.TrackingInfo.dLatitude;
    *pnDataLength += ConversionGPSDataToHexDec(fTemp, pnPos, &paMessageBuffer[*pnPos]);

    fTemp = pstMsgMdm->carReport.rpSetting.UserSetting.stUserActionSetting.TrackingInfo.dLongitude;
    *pnDataLength += ConversionGPSDataToHexDec(fTemp, pnPos, &paMessageBuffer[*pnPos]);
}

uint16_t MakeAutoLinkPacket(eMESSAGE_TYPE eMode, uint8_t *paMessageBuffer, stMsgMdm* pstMsgMdm)
{
    uint16_t nPos;
    uint16_t nMaxSize = 1024+256;

    // make header
    nPos = MakeEventMessageHeader2(eMode, paMessageBuffer, &pstMsgMdm->header);
    GIT_Assert(nPos < nMaxSize,eErrorCodeMsg|eHeaderSizeLimit);

    // make body
    nPos = MakePayload(eMode,nPos,paMessageBuffer,pstMsgMdm);

    GIT_Assert(nPos < nMaxSize,eErrorCodeMsg|ePayloadSizeLimit);

    return nPos;
}

#if				0
int aes_cbc_test(void)
{
	DWORD key_schedule[60];
	BYTE enc_buf[AES_TEXT_SIZE];
	BYTE plaintext[1][AES_TEXT_SIZE];
	BYTE Paddingtext[AES_TEXT_SIZE];
	BYTE iv[1][16] = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
	BYTE key[1][32] = {{'9','f','e','s','0','0','9','9','s','5','8','1','e','s','q','e','1','1','1','f','2','3','0','7','q','e','s','7','e','s','q','e'}};
	uint16_t nLen;
	uint8_t Base64Text[AES_TEXT_SIZE];
	uint8_t Decryptedtext[AES_TEXT_SIZE];
	uint16_t nBase64Len;

	//*******************************************************************************************************************
	// 원문
	//*******************************************************************************************************************
	strcpy(plaintext[0], "324B4D313233343536373830202020202020200000000048F07CC90301125867FEA6370A0000125867FEA63704B42710000300030101FFFDFA7F880009034FE00001F4461004003F003A007900138800000F00001000001100001200000000016833");
	nLen = strlen((char *)plaintext[0]);
	Trace("plaintext[0] len: %d\r\n", nLen);
	Trace("Plaintext    :\r\n");
	hexdump(plaintext[0], nLen);
	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// Key 값 세팅
	// 원래 키값은 "sef99900185seqsef11170327seqeqse" 이다.
	// 하지만, aes_key_setup() 함수에 입력 되는 key 배열에는
	// "9fes0099s581esqe111f2307qes7esq" 와 같이 4바이트 배열을 바꾸어서 적용해야 한다. big endian, little endian 변환
	//*******************************************************************************************************************
	memset((char *)key_schedule, 0x00, 240);
	aes_key_setup_new(key[0], key_schedule, 256);

	Trace("Key          :\r\n");
	hexdump(key[0], 32);

	Trace("key_schedule :\r\n");
	hexdump((char *)key_schedule, 240);
	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// Init Vector
	//*******************************************************************************************************************
	Trace("IV           :\r\n");
	hexdump(iv[0], 16);
	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// padding 적용
	//*******************************************************************************************************************
	memset(Paddingtext, 0x00, AES_TEXT_SIZE);
	nLen = pkcs7_pad_write(plaintext[0], 16, Paddingtext);
	nLen = strlen(Paddingtext);
	Trace("nLen: %d\r\n", nLen);
	hexdump(Paddingtext, nLen);
	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// AES 암호화
	//*******************************************************************************************************************
	aes_encrypt_cbc_new(Paddingtext, nLen, enc_buf, key_schedule, 256, iv[0]);

	Trace("-encrypted to:\r\n");
	hexdump(enc_buf, nLen);
	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// Encode base64
	//*******************************************************************************************************************
	if( mbedtls_base64_encode( Base64Text, sizeof( Base64Text ), &nBase64Len, enc_buf, nLen ) != 0) {
		Trace( "Base64 Encode failed\r\n" );

		return false;
	}

	Trace("Encode Base64\r\n");
	Base64Text[nBase64Len] = 0;
	hexdump((char *)Base64Text, nBase64Len);
	Trace("1. [%s]\n", Base64Text);
	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// Decode base64
	//*******************************************************************************************************************
	memset(Decryptedtext, 0x00, AES_TEXT_SIZE);
	if( mbedtls_base64_decode( Decryptedtext, sizeof( Decryptedtext ), &nLen, Base64Text, nBase64Len ) != 0) {
		Trace( "Base64 Decode failed\r\n" );

		return false;
	}

	hexdump((char *)Decryptedtext, nLen);
	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// AES 복호화
	//*******************************************************************************************************************
	aes_decrypt_cbc_new(Decryptedtext, nLen, enc_buf, key_schedule, 256, iv[0]);

	Trace("-decrypted to:\r\n");
	hexdump(Decryptedtext, nLen);

	Trace("Plaintext   :\r\n");
	hexdump(enc_buf, nLen);

	//*******************************************************************************************************************
	// 결과값 비교를 위해서 서버에서 생성된 암호화 데이터를 Decode base64 한다.
	//*******************************************************************************************************************
	Trace("Decode Base64(server)\r\n");
	nBase64Len = strlen("S0Ktff4w6ZI/pkxlePREADOnQWckz2uRR3YSpKEQVMFdnG9ziVr2cpPN4C5JjmcxF+ZQaRwy6hTMoNvtUvmbRjCfJLQ+FusFZ1PLiraFzzB3WdrRSFMGgsjgIgJi3wfcWMXj5Q4aDxiKaaU8/M75bg1TujZwiV4SzHKWq2IYAnHjZRjxnLE+xkLOKb+K/Yyns4Xa4+M1FDBmk+hhZ8xp1bHQWuQFKJjZl6xb6uYzgdjwNT7Rcze8Fa5N7CPbe3813W/7ehNWEelCPqjy0ATz+w==");
	memcpy(Base64Text, "S0Ktff4w6ZI/pkxlePREADOnQWckz2uRR3YSpKEQVMFdnG9ziVr2cpPN4C5JjmcxF+ZQaRwy6hTMoNvtUvmbRjCfJLQ+FusFZ1PLiraFzzB3WdrRSFMGgsjgIgJi3wfcWMXj5Q4aDxiKaaU8/M75bg1TujZwiV4SzHKWq2IYAnHjZRjxnLE+xkLOKb+K/Yyns4Xa4+M1FDBmk+hhZ8xp1bHQWuQFKJjZl6xb6uYzgdjwNT7Rcze8Fa5N7CPbe3813W/7ehNWEelCPqjy0ATz+w==", nBase64Len);
	Base64Text[nBase64Len] = 0;
	Trace("2. [%s]\r\n", Base64Text);

	if( mbedtls_base64_decode( Decryptedtext, sizeof( Decryptedtext ), &nLen, Base64Text, nBase64Len ) != 0) {
		Trace( "Base64 Decode failed\r\n" );

		return false;
	}

	hexdump((char *)Decryptedtext, nLen);
	//*******************************************************************************************************************

	//*******************************************************************************************************************
	// 서버에서 생성된 함호화 데이터를 복호화
	//*******************************************************************************************************************
	aes_decrypt_cbc_new(Decryptedtext, nLen, enc_buf, key_schedule, 256, iv[0]);

	Trace("-decrypted to(server):\r\n");
	hexdump(Decryptedtext, nLen);

	Trace("Plaintext(server):\r\n");
	hexdump(enc_buf, nLen);

	return(1);
}
#endif

