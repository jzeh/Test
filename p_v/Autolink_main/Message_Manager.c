/* Includes ------------------------------------------------------------------*/
#include "Modem_Manager.h"
#include "GIT_Util.h"
#include "Modem_comm.h"
#include "Message_Manager.h"
#include "Power_Manager.h"
#include "OBD_Controller.h"
#include "Message_Make.h"
#include "FOTA_Manager.h"
#include "AutolinkConfiguration.h"
#include "MngSystem.h"
#include "HdDebug.h"
#include "DebugHandler.h"
#include "HandlerRsvEngCtrl.h"
#include "SysHalFileSystem.h"
#include "HalHandler.h"

#define Trace(...)  GITDebug(DEBUG_MODULES_MODEM,__VA_ARGS__)

//#define MAX_EVENT_MESSAGE_BUFFER_LENGTH				1500


MESSAGE_MANAGER_DATA MessageManagerData;

// 서버로 부터 수신 받은 에어 명령을 보관해 둔다.
uint8_t gb_RemoteMessageBuffer[MAX_CONTROL_REQUEST_FROM_SERVER_BUFFER_LENGTH];
uint16_t gn_RemoteMessageBufferLength;
#if defined(PROTOCOL17)
extern stURLInfo g_stTempURLInfo;
unsigned char g_SavedGUID[MAX_GUID_LENGTH+1];
#endif

extern stMsgMdm m_stLastRcvSmartKey;

REMOTE_COMMAND stRemoteCommand;

#pragma section="BKSRAM"

/* Function ------------------------------------------------------------------*/
bool ParseRemoteMessage(uint32_t* punOccurredEventTime);
extern uint32_t GetTimefromDate2(stHalRTCTypeDef stDate);
extern void GetDatefromDateArray(char* parrDateTime,stHalRTCTypeDef* pstDate);

void InitializeMessageManager(void)
{
	MessageManagerData.eMessageCommResultCode = eMESSAGE_RESULT_CODE_NONE;
	MessageManagerData.bReceivedRequestRemoteMessageFlag = eMESSAGE_REMOTE_CONTROL_STATE_NONE;
	MessageManagerData.iTimer_MSG_Common_Dly = HalTimerSetSWTimer(1000, eSWTimer_ONESHOT, NULL, false);

	return;
}

uint8_t ConversionHexDecToByte(uint8_t *pBuffer)
{
	uint8_t bData;

	bData = (uint8_t)*pBuffer;

	return bData;
}

uint16_t ConversionHexDecToWord(uint8_t *pBuffer)
{
	uint16_t nData;
	uint8_t bData;

	nData = 0x0000;
	nData = (uint8_t)*pBuffer;
	nData <<= 8;

	pBuffer++;
	bData = (uint8_t)*pBuffer;

	nData |= bData;

	return nData;
}

uint32_t ConversionHexDecToDWord(uint8_t *pBuffer)
{
	uint32_t wData;
	uint8_t bData;

	wData = 0;
	wData = (uint32_t)*pBuffer;
	wData <<= 8;

	pBuffer++;
	bData = (uint8_t)*pBuffer;
	wData |= bData;
	wData <<= 8;

	pBuffer++;
	bData = (uint8_t)*pBuffer;
	wData |= bData;
	wData <<= 8;

	pBuffer++;
	bData = (uint8_t)*pBuffer;
	wData |= bData;

	Trace("wData: %x\r\n", wData);

	return wData;
}

long long ConversionHexDecToLongLongWord(uint8_t *pBuffer)
{
	long long llData;
	uint32_t wData;
	uint8_t bData;

	llData = 0;
	llData = (long long)*pBuffer;
	llData <<= 8;

	pBuffer++;
	bData = (uint8_t)*pBuffer;
	llData |= bData;
	llData <<= 8;

	pBuffer++;
	bData = (uint8_t)*pBuffer;
	llData |= bData;
	llData <<= 8;

	pBuffer++;
	bData = (uint8_t)*pBuffer;
	llData |= bData;
	llData <<= 8;

	pBuffer++;
	bData = (uint8_t)*pBuffer;
	llData |= bData;
	llData <<= 8;

	wData = (llData >> 32) && 0xFFFFFFFF;
	Trace("llData(u): %x\r\n", wData);
	wData = (llData) && 0xFFFFFFFF;
	Trace("llData(l): %x\r\n", wData);

	return llData;
}

void ReverseString(char* s)
{
  uint16_t size = strlen(s);
  char temp;

  for (uint16_t i = 0; i < size / 2; i++) {
    temp = s[i];
    s[i] = s[(size - 1) - i];
    s[(size - 1) - i] = temp;
  }
}

char* itoa_1(long long val, char *buf, int radix)
{
	char *p = buf;

	while(val) {
		if(radix <= 10)
			*p++ = (val % radix) + '0';
		else {
			int t = val % radix;

			if (t <= 9)
				*p++ = t + '0';
			else
				*p++ = t - 10 + 'a';
		}

		val /= radix;
	}

	*p = '\0';
	ReverseString(buf);

	return buf;
}


#define CONTROL_REQUEST_COMMAND_DATA_POS			112

#define MSG_CON_CMD_REQ_POS_COMM_TYPE		(CONTROL_REQUEST_COMMAND_DATA_POS + 0)
#define MSG_CON_CMD_REQ_POS_GUID			(MSG_CON_CMD_REQ_POS_COMM_TYPE 	  + 2)
#define MSG_CON_CMD_REQ_POS_CON_TYPE		(MSG_CON_CMD_REQ_POS_GUID 		  + 64)
#define MSG_CON_CMD_REQ_POS_STARTUP_TIME	(MSG_CON_CMD_REQ_POS_CON_TYPE 	  + 2)
#define MSG_CON_CMD_REQ_POS_TEMPERATURE		(MSG_CON_CMD_REQ_POS_STARTUP_TIME + 2)
#define MSG_CON_CMD_REQ_POS_CHECK_TEMPERATURE (MSG_CON_CMD_REQ_POS_TEMPERATURE  + 4)
#define MSG_CON_CMD_REQ_POS_REAR_DEFOGGER	(MSG_CON_CMD_REQ_POS_CHECK_TEMPERATURE  + 2)
#define MSG_CON_CMD_REQ_POS_HEAD_LAMP	    (MSG_CON_CMD_REQ_POS_REAR_DEFOGGER  + 2)
#define MSG_CON_CMD_REQ_POS_REMOVE_FROST	(MSG_CON_CMD_REQ_POS_HEAD_LAMP  + 2)

//#define MSG_CON_CMD_REQ_POS_LATITUDE		(MSG_CON_CMD_REQ_POS_REMOVE_FROST + 2)
//#define MSG_CON_CMD_REQ_POS_LONGITUDE		(MSG_CON_CMD_REQ_POS_LATITUDE 	  + 10)
//#define MSG_CON_CMD_REQ_POS_BOUND_TYPE		(MSG_CON_CMD_REQ_POS_LONGITUDE 	  + 10)
//#define MSG_CON_CMD_REQ_POS_DISTANCE		(MSG_CON_CMD_REQ_POS_BOUND_TYPE   + 2)
#define MSG_CON_CMD_REQ_POS_OCCURE_TIME		(MSG_CON_CMD_REQ_POS_REMOVE_FROST + 2)
#define MSG_CON_CMN_REQ_POS_BT_CONTROLKEY   (MSG_CON_CMD_REQ_POS_OCCURE_TIME  + 12)

void CovertByteArray(char* pcarrTemp,uint8_t cSize)
{
    unsigned char carrTmp[32];
    memcpy(carrTmp,pcarrTemp,cSize);

    for(int i=0;i<cSize;i++)
    {
        pcarrTemp[i] = carrTmp[(cSize-1)-i];
    }
}

int GetGetfenceCount(char* pcarrBuffer,int size)
{
    int nGeofenceCount=0;
    for(int i=0;i<size;i++)
    {
        if( pcarrBuffer[i] == ',' )
            nGeofenceCount++;
    }

    // we added 1 because the data is split with ','
    return nGeofenceCount+1;
}

boolean_t GetNextGeoFenceToken(char* pCur,char cToken, char** pcNext, char* pcValue)
{
    char* ptr;
    
    ptr = strchr((const char*)pCur,cToken);
    if( ptr != NULL )
    {
        // we need add 1 address because when we use GetNextGeoFenceToken, cur address is same with token
        *pcNext = ptr+1;

        if( ptr - pCur > 0 )
            memcpy((char *)pcValue, (char *)pCur, ptr - pCur);

        return true;
    }

    pcNext = NULL;

    return false;
}


int GetPolygonGeofenceListCount(char* pcarrBuffer,int size, uint16_t* pusPolygonListSize)
{
    int nPolygonGeofenceCount= GetGetfenceCount(pcarrBuffer,size);
    char carrBuffer[512] = {0,};
    char carrValue[64] = {0,};
    char* pcCur = carrBuffer;
    char* pcNext;

    if( size == 0 )
        return 0;
    if( size > 512 )
        return 0;

    // to check last value ME added force token.
    memcpy(carrBuffer,pcarrBuffer,size);
    carrBuffer[size]=',';
    
    // calculate last list count
    for(int k=0;k<nPolygonGeofenceCount;k++)
    {
        if( GetNextGeoFenceToken(pcCur,',',&pcNext,carrValue))
        {
            pusPolygonListSize[k]= atoi(carrValue);
            //hexdump(carrValue,strlen(carrValue));
            
            memset(carrValue,0,sizeof(carrValue));

            pcCur = pcNext;
            pcNext= NULL;
        }
    }
    
    // we added 1 because the data is split with ','
    return nPolygonGeofenceCount;
}

bool ParseRemoteMessage(uint32_t* punOccurredEventTime)
{
    stHalRTCTypeDef stDate;
	long long llTemp;
    //MONI increase buffer size for geo fence.
	uint8_t aTempBuffer[256]={0,};
	uint8_t aTemp[64]={0,};
	Trace("@ParseRemoteMessage()\r\n");

	//hexdump(&gb_RemoteMessageBuffer[112], gn_RemoteMessageBufferLength-112);
	//hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);

	//************************************************************************
	// STX: 1
	// VIN: 18
	// ID: 20
	// SOURCE: 1
	// DESTINATION: 1
	// MSG DATE/TIME: 6
	// UTC MSG DATE/TIME: 6
	// OPCODE: 1
	// LENGTH: 2
	// --> 여기까지 56이다.(따라서, 112)

	// DATA

	// CHECKSUM: 1
	// ETX: 1
	// --> 실제 갯수는 HEX ASCII 이므로 2배이다.
	//************************************************************************

	memset((char *)&stRemoteCommand, 0x00, sizeof(REMOTE_COMMAND));

	// 명령구분: 1
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[MSG_CON_CMD_REQ_POS_COMM_TYPE], 1);
	stRemoteCommand.eCommandType = (eREMOTE_CON_CMD_TYPE)aTempBuffer[0];

	// GUID: 32
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[MSG_CON_CMD_REQ_POS_GUID], MAX_GUID_LENGTH);
	//Trace("GUID:\r\n"); hexdump(aTempBuffer, MAX_GUID_LENGTH);
	if(memcmp((char *)aTempBuffer, MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH) != 0) {
		Trace("MSG: mismatch GUID!!!\r\n");
        //Trace("Request GUID:\r\n"); hexdump(MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);
		return false;
	}
    memcpy((char*)&stRemoteCommand.guid,(char*)&aTempBuffer,32);

    //hexdump(&gb_RemoteMessageBuffer[MSG_CON_CMD_REQ_POS_CON_TYPE], gn_RemoteMessageBufferLength-MSG_CON_CMD_REQ_POS_CON_TYPE);

	// 제어 구분: 1
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[MSG_CON_CMD_REQ_POS_CON_TYPE], 1);
	stRemoteCommand.bControlType = aTempBuffer[0];

	// 시동유지시간: 1
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[MSG_CON_CMD_REQ_POS_STARTUP_TIME], 1);
	stRemoteCommand.bStartUpTime = aTempBuffer[0];

    // display temperature value and check value
    hexdump(&gb_RemoteMessageBuffer[MSG_CON_CMD_REQ_POS_TEMPERATURE],16);

	// 온도: 2
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[MSG_CON_CMD_REQ_POS_TEMPERATURE], 2);
	//memcpy((char *)&stRemoteCommand.nTemperature,(char *)aTempBuffer,2);
	stRemoteCommand.nTemperature = aTempBuffer[1]<<8|aTempBuffer[0];

    // check valur for temperature : 1
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[MSG_CON_CMD_REQ_POS_CHECK_TEMPERATURE], 1);
    stRemoteCommand.unCheckTemperature = aTempBuffer[0];

#if defined(PROTOCOL13)
    // rear defogger
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[MSG_CON_CMD_REQ_POS_REAR_DEFOGGER], 1);
    stRemoteCommand.bRearDefogger = aTempBuffer[0];

    // head lamp
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[MSG_CON_CMD_REQ_POS_HEAD_LAMP], 1);
    stRemoteCommand.bHighBeam = aTempBuffer[0];
#endif //#if defined(PROTOCOL13)

	// 성에제거 유무: 1
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[MSG_CON_CMD_REQ_POS_REMOVE_FROST], 1);
	stRemoteCommand.bRemoveFrost = aTempBuffer[0];

	// 발생시간: 6
	memset(aTempBuffer,0,sizeof(aTempBuffer));
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[MSG_CON_CMD_REQ_POS_OCCURE_TIME], 6);
	llTemp = 0;
	llTemp |= aTempBuffer[0];
	llTemp <<= 8;
	llTemp |= aTempBuffer[1];
	llTemp <<= 8;
	llTemp |= aTempBuffer[2];
	llTemp <<= 8;
	llTemp |= aTempBuffer[3];
	llTemp <<= 8;
	llTemp |= aTempBuffer[4];
	llTemp <<= 8;
	llTemp |= aTempBuffer[5];

    //itoa_1(llTemp, (char *)stRemoteCommand.arrDateTime, 10);
    sprintf((char *)aTempBuffer,"%lld\x00",llTemp);
    printf("Occurred Event Time : %s\r\n",aTempBuffer);
    GetDatefromDateArray((char *)aTempBuffer,&stDate);
    stRemoteCommand.unOccurredDateTime = GetTimefromDate2(stDate);
	
    // return occurred event time
    *punOccurredEventTime = stRemoteCommand.unOccurredDateTime;
    
    	
#if defined(PROTOCOL18)
	memset(aTempBuffer,0,sizeof(aTempBuffer));
	memset(aTemp,0,sizeof(aTemp));
		
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[MSG_CON_CMN_REQ_POS_BT_CONTROLKEY], MAX_BT_CONTROLKEY_LENGTH*2);
			
	if( (aTempBuffer[0]==' ') || (strncmp((char *)aTempBuffer[0],"null",4)==0 ))
	{
		memset(stRemoteCommand.BTControlKey, 0x20, MAX_BT_CONTROLKEY_LENGTH);
	}
	else
	{
		Convert_HexString_To_Bin((char *)aTemp, (char *)aTempBuffer, MAX_BT_CONTROLKEY_LENGTH);
		memcpy((char*)&stRemoteCommand.BTControlKey,(char*)&aTemp, MAX_BT_CONTROLKEY_LENGTH);
	}
#endif

    return true;
}

#if defined(PROTOCOL17)
void ParseSetURLMessage(stMsgSysMsg* pstMessage)
{
	uint8_t aTempBuffer[MAX_SERVER_URL_LENGTH*3];
    int32_t nBufPosition = CONTROL_REQUEST_COMMAND_DATA_POS;
	//int nLength=0;

    stUserSetting stUserSettingMessage;
    memset((char*)&stUserSettingMessage,0,sizeof(stUserSetting));

	Trace("@ParseSetURLMessage()\r\n");

	//hexdump(&gb_RemoteMessageBuffer[112], gn_RemoteMessageBufferLength-112);
	//hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);

    // GUID: 32
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], MAX_GUID_LENGTH);
    //Trace("GUID:\r\n"); hexdump(aTempBuffer, MAX_GUID_LENGTH);
    memcpy((char*)&pstMessage->carReport.rpSetting.UserSetting.RequestGUID,(char*)&aTempBuffer,MAX_GUID_LENGTH);
	memset((char*)g_SavedGUID,0x00,MAX_GUID_LENGTH+1);
    memcpy((char*)g_SavedGUID,(char*)&aTempBuffer,MAX_GUID_LENGTH);
    nBufPosition+=(MAX_GUID_LENGTH*2);

	//URL COPY
	memset(&g_stTempURLInfo,0,sizeof(stURLInfo));
	g_stTempURLInfo.nLength = Convert_HexString_To_Bin_URL((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], MAX_SERVER_URL_LENGTH);
	memcpy(&g_stTempURLInfo.strUrl,(char*)&aTempBuffer,g_stTempURLInfo.nLength);
    nBufPosition+=(MAX_SERVER_URL_LENGTH*2);
	
//    // 발생시간: 6
//    memset(aTempBuffer,0,sizeof(aTempBuffer));
//    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 6);
//
//    llTemp = 0;
//    llTemp |= aTempBuffer[0];
//    llTemp <<= 8;
//    llTemp |= aTempBuffer[1];
//    llTemp <<= 8;
//    llTemp |= aTempBuffer[2];
//    llTemp <<= 8;
//    llTemp |= aTempBuffer[3];
//    llTemp <<= 8;
//    llTemp |= aTempBuffer[4];
//    llTemp <<= 8;
//    llTemp |= aTempBuffer[5];
//
//    // not used
//    //itoa_1(llTemp, (char *)pstGeofenceValue->unDateTime, 14);
//    sprintf((char *)aTempBuffer,"%lld\x00",llTemp);
//    Trace("Occurred Event Time : %s\r\n",aTempBuffer);
//
//    stHalRTCTypeDef stDate;
//    GetDatefromDateArray((char *)aTempBuffer,&stDate);
//    stModemMessage.unEventTime = GetTimefromDate2(stDate/*,pstGeofenceValue->unDateTime*/);
//
//    // copy all contents
//    memcpy((char*)&pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.ModemActivate,
//            (char*)&stModemMessage,sizeof(stModemActivateSetting));
//
//    Trace("=================================================\r\n");
//    Trace("Modem Activate\r\n");
//    Trace("GUID : ");hexdump(pstMessage->carReport.rpSetting.UserSetting.RequestGUID,MAX_GUID_LENGTH);
//    //Trace("VIN : %s\r\n",stModemMessage.carrRequestVIN);
//    Trace("Odometer : %d\r\n",stModemMessage.unOdometer);
}
#endif

int parseRsvEngCtrlSubValue(char* pcarrBuffer,stRsvEngCntorlInfo* pststRsvEngCntorlValue)
{
    int nIndexPosition = 0;
    long long llTemp=0;
    char aTempBuffer[128]={0};
	int i;

    memset((char*)pststRsvEngCntorlValue,0,sizeof(stRsvEngCntorlInfo));

	// GUID : 32
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 32);
	memcpy(pststRsvEngCntorlValue->ucGUID, (char*)&aTempBuffer[nIndexPosition], 32);
	nIndexPosition += 64;
	
	// 설정 유무 : 1
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 1);
	pststRsvEngCntorlValue->bRsvEngCtrl = aTempBuffer[0];
	nIndexPosition += 2;

	// 반복 유무     : 1
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 1);
	pststRsvEngCntorlValue->bRepeat = aTempBuffer[0];
	nIndexPosition += 2;

	// 예약 시간     : 2
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 4);

	for(i=0; i< 4; i++)
	{
		aTempBuffer[i] -= 48;
	}

	pststRsvEngCntorlValue->nRsvTime = (((aTempBuffer[0]*10)+aTempBuffer[1])*3600) + (((aTempBuffer[2]*10)+aTempBuffer[3])*60);
	nIndexPosition += 8;

	// 예약   요일   : 1
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 1);
	pststRsvEngCntorlValue->ucRsvDayoftheweek = aTempBuffer[0];
	nIndexPosition += 2;

	// 예약 날짜 : 6
	memset(aTempBuffer,0,sizeof(aTempBuffer));
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 6);

    llTemp = 0;
    llTemp |= aTempBuffer[0];
    llTemp <<= 8;
    llTemp |= aTempBuffer[1];
    llTemp <<= 8;
    llTemp |= aTempBuffer[2];
    llTemp <<= 8;
    llTemp |= aTempBuffer[3];
    llTemp <<= 8;
    llTemp |= aTempBuffer[4];
    llTemp <<= 8;
    llTemp |= aTempBuffer[5];

	sprintf((char *)aTempBuffer,"%lld\x00",llTemp);

	stHalRTCTypeDef stDate;
	
    memset(&stDate,0x00,sizeof(stHalRTCTypeDef));
	GetDatefromDateArray((char *)aTempBuffer,&stDate);
	pststRsvEngCntorlValue->nRsvDate = GetTimefromDate2(stDate);
	
	nIndexPosition += 12;

	// 시동유지시간
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 1);
	pststRsvEngCntorlValue->ucDurationTime = aTempBuffer[0];
	nIndexPosition += 2;

	// 온도 : 2
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 2);
	pststRsvEngCntorlValue->usTemperature = aTempBuffer[1]<<8|aTempBuffer[0];
	nIndexPosition += 4;

	// check : 1
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 1);
	pststRsvEngCntorlValue->ucCheckTemperature = aTempBuffer[0];
	nIndexPosition += 2;

	// 후방열선 : 1
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 1);
	pststRsvEngCntorlValue->bRearDefog = aTempBuffer[0];
	nIndexPosition += 2;

	// 헤드램프: 1
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 1);
	pststRsvEngCntorlValue->bHeadLamp = aTempBuffer[0];
	nIndexPosition += 2;

	// 성애제거 : 1
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 1);
	pststRsvEngCntorlValue->bDeperost = aTempBuffer[0];
	nIndexPosition += 2;

	return nIndexPosition;

}

void parseGeofenceSubValue(char* pcarrBuffer,stGeofenceUnit* pstGeoFenceValue)
{
    int nIndexPosition = 0;
    long long llTemp=0;
    char aTempBuffer[128]={0};

    memset((char*)pstGeoFenceValue,0,sizeof(stGeofenceUnit));

	// 위도: 5
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 5);
    CovertByteArray((char *)aTempBuffer,5);
    memcpy(&llTemp,aTempBuffer,5);

    // this bit operation for the minus value.
    // 00 00 00 FF FD FB 72 00 this is minus values
    // FF FD FB 72 00 00 00 00 llTemp<<=(3*8);
    // FF FF FF FF FD FB 72 00 llTemp>>=(3*8);
    llTemp<<=(3*8);
    llTemp>>=(3*8);
	pstGeoFenceValue->GpsLatitude = (float)(llTemp / 1000000.0);

    // latitude size
    nIndexPosition+=10;

    memset((char *)&aTempBuffer, 0x00, sizeof(aTempBuffer));
	// 경도: 5
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 5);
    CovertByteArray((char *)aTempBuffer,5);
    memcpy((char *)&llTemp,(char *)aTempBuffer,5);

    llTemp<<=(3*8);
    llTemp>>=(3*8);
	pstGeoFenceValue->GpsLongitude = (float)(llTemp / 1000000.0);

    // longitude size
    nIndexPosition+=10;

	// 바운드타입: 1
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 1);
	if(aTempBuffer[0] == 'I') {
		// 인바운드
		pstGeoFenceValue->cFenceType = eREMOTE_CON_BOUNDTYPE_IN;
	}
	else if(aTempBuffer[0] == 'O') {
		// 아웃바운드
		pstGeoFenceValue->cFenceType = eREMOTE_CON_BOUNDTYPE_OUT;
	}
    else if(aTempBuffer[0] == 'B') {
		// BOTH (IN/OUT)
		pstGeoFenceValue->cFenceType = eREMOTE_CON_BOUNDTYPE_BOTH;
	}
	else {
		pstGeoFenceValue->cFenceType = eREMOTE_CON_BOUNDTYPE_NONE;
	}

    // bound type size
    nIndexPosition+=2;

	// 거리: 3
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 3);
	pstGeoFenceValue->unDistance =(aTempBuffer[0]<<16)|(aTempBuffer[1]<<8)|(aTempBuffer[2])*1000;

    // distance
    nIndexPosition+=6;

    // enable / disable
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&pcarrBuffer[nIndexPosition], 1);
    pstGeoFenceValue->bActive = ((aTempBuffer[0] == 'Y') ? true:false);
}

void parsePolygonGeofenceSubValue(char* pcarrBuffer,int nSize, stPolygonGeofenceUnit* pstGeoFenceValue)
{
    char aTempBuffer[512]={0,};
    char arrTempValue[64]={0,};
    uint32_t unGeofenceID = 0;
    int nGeofenceCount = 0;
    char cInoutType;
    char cEnable;
    double dlLat=0,dlLon=0;

    memset((char*)pstGeoFenceValue,0,sizeof(stGeofenceUnit));

    // set up temp buffer with one sub body
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)pcarrBuffer,nSize);

    unGeofenceID = aTempBuffer[nSize-6]<<24|aTempBuffer[nSize-5]<<16|aTempBuffer[nSize-4]<<8|aTempBuffer[nSize-3];
    cInoutType = aTempBuffer[nSize-2];
    cEnable = aTempBuffer[nSize-1];
    
    // get point list count
    // we should minus 2 because boundary type and enable.
    nGeofenceCount = GetGetfenceCount((char *)aTempBuffer,nSize-2-4);

    // We added this token to calculate last message.
    aTempBuffer[nSize-6] = '|';
    aTempBuffer[nSize-5] = 0;

    char* pCur = aTempBuffer;
    char* pNext;
    int nReadCount;

    // save list count
    pstGeoFenceValue->ucPointListCount = nGeofenceCount-1;
    
    for(int i=0;i<nGeofenceCount-1;i++)
    {
        if( GetNextGeoFenceToken(pCur,'|',&pNext,arrTempValue) == true )
        {
            nReadCount = ssscanf((char *)arrTempValue, strlen(arrTempValue),"%f,%f",&dlLat,&dlLon);
            
            pCur=pNext;

            // read success
            if( nReadCount == 2 )
            {
                pstGeoFenceValue->stPointList[i].GpsLatitude = dlLat;
                pstGeoFenceValue->stPointList[i].GpsLongitude = dlLon;
                
                //Trace("index : %d, lat : %f, lon : %f\r\n", i, 
                //    pstGeoFenceValue->stPointList[i].GpsLatitude,
                //    pstGeoFenceValue->stPointList[i].GpsLongitude);
                
                memset(arrTempValue,0,sizeof(arrTempValue));
                dlLat = 0;
                dlLon = 0;
            }
        }
        else
        {
            Trace("Polygon Geofence Data is not valid. break;\r\n");
            break;
        }
    }

    pstGeoFenceValue->unGeofenceID = unGeofenceID;

	// 바운드타입: 1
	if(cInoutType == 'I') {
		// 인바운드
		pstGeoFenceValue->cFenceType = eREMOTE_CON_BOUNDTYPE_IN;
	}
	else if(cInoutType == 'O') {
		// 아웃바운드
		pstGeoFenceValue->cFenceType = eREMOTE_CON_BOUNDTYPE_OUT;
	}
    else if(cInoutType == 'B') {
		// BOTH (IN / OUT)
		pstGeoFenceValue->cFenceType = eREMOTE_CON_BOUNDTYPE_BOTH;
	}
	else {
		pstGeoFenceValue->cFenceType = eREMOTE_CON_BOUNDTYPE_NONE;
	}

    // enable / disable
    pstGeoFenceValue->bActive = ((cEnable == 'Y') ? true:false);
}

typedef __packed struct __SettingInfo{
    uint16_t AlramMask;
    uint32_t ParkingInterval;
    uint32_t DrivingInterval;
    uint32_t ValetAlramDistance;
    uint32_t ValetAlramTime;
    uint32_t TowingAlramDistance;
    uint32_t TowingAlramTime;
}SettingInfo;

bool ParseSettingInfoMessage()
{
    //stHalRTCTypeDef stDate;
	//long long llTemp;
    //MONI increase buffer size for geo fence.
	uint8_t aTempBuffer[256];

    int32_t nBufPosition = CONTROL_REQUEST_COMMAND_DATA_POS;

    SettingInfo stSettingInfo;
    memset((char*)&stSettingInfo,0,sizeof(stSettingInfo));

    if( gn_RemoteMessageBufferLength < CONTROL_REQUEST_COMMAND_DATA_POS )
    {
        Trace("Size Mismatched Error\r\n");
        return false;
    }
    
	Trace("@ParseSettingInfoMessage(), gn_RemoteMessageBufferLength %d\r\n", gn_RemoteMessageBufferLength);

//	hexdump(&gb_RemoteMessageBuffer[112], gn_RemoteMessageBufferLength-112);
//	hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);

	//************************************************************************
	// STX: 1
	// VIN: 18
	// ID: 20
	// SOURCE: 1
	// DESTINATION: 1
	// MSG DATE/TIME: 6
	// UTC MSG DATE/TIME: 6
	// OPCODE: 1
	// LENGTH: 2
	// --> 여기까지 56이다.(따라서, 112)

	// DATA

	// CHECKSUM: 1
	// ETX: 1
	// --> 실제 갯수는 HEX ASCII 이므로 2배이다.
	//************************************************************************

	// Alram Mask Setting :2
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 2);
	stSettingInfo.AlramMask = aTempBuffer[0]<<8|aTempBuffer[1];

    nBufPosition+=4;

	// Parking Report Interval :3
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 3);
	stSettingInfo.ParkingInterval = aTempBuffer[0]<<16|aTempBuffer[1]<<8|aTempBuffer[2];

    nBufPosition+=6;

	// Driving Report Interval :3
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 3);
	stSettingInfo.DrivingInterval = aTempBuffer[0]<<16|aTempBuffer[1]<<8|aTempBuffer[2];

    nBufPosition+=6;

    // Valet Alram Distance :3
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 3);
	stSettingInfo.ValetAlramDistance = aTempBuffer[0]<<16|aTempBuffer[1]<<8|aTempBuffer[2];

    nBufPosition+=6;

	// Valet Alram Time :3
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 3);
	stSettingInfo.ValetAlramTime = aTempBuffer[0]<<16|aTempBuffer[1]<<8|aTempBuffer[2];

    nBufPosition+=6;

    // Towing Alram Disntance :3
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 3);
	stSettingInfo.TowingAlramDistance = aTempBuffer[0]<<16|aTempBuffer[1]<<8|aTempBuffer[2];

    nBufPosition+=6;

	// Towing Alram Time :3
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 3);
	stSettingInfo.TowingAlramTime = aTempBuffer[0]<<16|aTempBuffer[1]<<8|aTempBuffer[2];

    Trace("=================================================\r\n");
    Trace("Setting Info\r\n");
    Trace("AlramMask           : %d\r\n",stSettingInfo.AlramMask);
    Trace("ParkingInterval     : %d\r\n",stSettingInfo.ParkingInterval);
    Trace("DrivingInterval     : %d\r\n",stSettingInfo.DrivingInterval);
    Trace("ValetAlramDistance  : %d\r\n",stSettingInfo.ValetAlramDistance);
    Trace("ValetAlramTime      : %d\r\n",stSettingInfo.ValetAlramTime);
    Trace("TowingAlramDistance : %d\r\n",stSettingInfo.TowingAlramDistance);
    Trace("TowingAlramTime     : %d\r\n",stSettingInfo.TowingAlramTime);
    
    return true;
}

#if defined(PROTOCOL12)
bool ParseModemActivateMessage(stMsgSysMsg* pstMessage)
{
    long long llTemp;
	uint8_t aTempBuffer[256];
    int32_t nBufPosition = 112;

    stModemActivateSetting stModemMessage;
    memset((char*)&stModemMessage,0,sizeof(stModemActivateSetting));

	Trace("@ParseModemActivateMessage()\r\n");

	//hexdump(&gb_RemoteMessageBuffer[112], gn_RemoteMessageBufferLength-112);
	//hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);

    // GUID: 32
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], MAX_GUID_LENGTH);
    //Trace("GUID:\r\n"); hexdump(aTempBuffer, MAX_GUID_LENGTH);
    if(memcmp((char *)aTempBuffer, MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH) != 0) {
        Trace("MSG: mismatch GUID!!!\r\n");
        //Trace("Request GUID:\r\n"); hexdump(MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);
        return false;
    }

    memcpy((char*)&pstMessage->carReport.rpSetting.UserSetting.RequestGUID,(char*)&aTempBuffer,MAX_GUID_LENGTH);

    nBufPosition+=64;
/*
    memset(aTempBuffer,0,sizeof(aTempBuffer));
    // VIN : 18
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], MAX_VIN_SIZE);
    memcpy((char*)&stModemMessage.carrRequestVIN,(char*)&aTempBuffer,MAX_VIN_SIZE);

    nBufPosition+=36;
*/
	// obometer :3
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 3);
	stModemMessage.unOdometer = aTempBuffer[0]<<16|aTempBuffer[1]<<8|aTempBuffer[2];

    nBufPosition+=6;

    // 발생시간: 6
    memset(aTempBuffer,0,sizeof(aTempBuffer));
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 6);

    llTemp = 0;
    llTemp |= aTempBuffer[0];
    llTemp <<= 8;
    llTemp |= aTempBuffer[1];
    llTemp <<= 8;
    llTemp |= aTempBuffer[2];
    llTemp <<= 8;
    llTemp |= aTempBuffer[3];
    llTemp <<= 8;
    llTemp |= aTempBuffer[4];
    llTemp <<= 8;
    llTemp |= aTempBuffer[5];

    // not used
    //itoa_1(llTemp, (char *)pstGeofenceValue->unDateTime, 14);
    sprintf((char *)aTempBuffer,"%lld\x00",llTemp);
    Trace("Occurred Event Time : %s\r\n",aTempBuffer);

    stHalRTCTypeDef stDate;
    GetDatefromDateArray((char *)aTempBuffer,&stDate);
    stModemMessage.unEventTime = GetTimefromDate2(stDate/*,pstGeofenceValue->unDateTime*/);

    // copy all contents
    memcpy((char*)&pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.ModemActivate,
            (char*)&stModemMessage,sizeof(stModemActivateSetting));

    Trace("=================================================\r\n");
    Trace("Modem Activate\r\n");
    Trace("GUID : ");hexdump(pstMessage->carReport.rpSetting.UserSetting.RequestGUID,MAX_GUID_LENGTH);
    //Trace("VIN : %s\r\n",stModemMessage.carrRequestVIN);
    Trace("Odometer : %d\r\n",stModemMessage.unOdometer);
    
    return true;
}
#endif

bool ParseRsvEngCtrlMessage(stCarReport* pstRsvEngCtrl)
{
    stRsvEngCntorl* pstRsvEngCtrlValue = &pstRsvEngCtrl->rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl;

    memset((char*)pstRsvEngCtrlValue,0,sizeof(stRsvEngCntorl));
	//long long llTemp;
    //MONI increase buffer size for geo fence.
	uint8_t aTempBuffer[512];

	Trace("@ParseRsvEngCtrlMessage()\r\n");

	//hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);
	hexdump(gb_RemoteMessageBuffer, 16);

	if(IsValidRsvEngCtrlGUID(gb_RemoteMessageBuffer) == false)
	{
		return false;
	}

    int32_t nBufPosition = CONTROL_REQUEST_COMMAND_DATA_POS;

	// GUID: 32
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], MAX_GUID_LENGTH);
	Trace("Received GUID:\r\n");
	hexdump(aTempBuffer, MAX_GUID_LENGTH);
#if false
	if(memcmp((char *)aTempBuffer, MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH) != 0) {
		Trace("MSG: mismatch GUID!!!\r\n");
        Trace("Request GUID:\r\n");
        hexdump(MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);
		return false;
	}
#endif
    // copy guid
    memcpy((char*)pstRsvEngCtrl->rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.ucGroupGUID ,(char*)&aTempBuffer,32);

    nBufPosition+=64;

	// Data CNT
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 1);
	pstRsvEngCtrlValue->ucDataCnt = aTempBuffer[0];
	nBufPosition+=2;

	// legnth size : 2bytes
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 2);
    pstRsvEngCtrlValue->usLengthSize = aTempBuffer[0]<<8|aTempBuffer[1];

    nBufPosition+=4;

    if(pstRsvEngCtrlValue->usLengthSize > sizeof(aTempBuffer)*2)
    {
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!Convert_HexString_To_Bin fail!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        memset(pstRsvEngCtrlValue,0x00,sizeof(stRsvEngCntorl));
        return false;
    }
    else
    {
        Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition],pstRsvEngCtrlValue->usLengthSize);
    }
   // int nRsvEngCtrlCount = 0;
   // nRsvEngCtrlCount = GetGetfenceCount((char *)aTempBuffer,pstRsvEngCtrlValue->usLengthSize);
	
    nBufPosition+=(pstRsvEngCtrlValue->usLengthSize*2);

    // parse length count
    for(int i=0;i<pstRsvEngCtrlValue->ucDataCnt;i++)
    {
    	int temp = 0;
        temp = parseRsvEngCtrlSubValue((char*)&gb_RemoteMessageBuffer[nBufPosition],&pstRsvEngCtrlValue->stRsvEngCtrlInfo[i]);

		nBufPosition+=temp;
    }

	pstRsvEngCtrlValue->ucWakeupIndexFlag  = 0x00;

	pstRsvEngCtrlValue->unRcvSMSTime = 
	m_stLastRcvSmartKey.carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.unRcvSMSTime;

	pstRsvEngCtrl->rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.unRcvSMSTime = m_stLastRcvSmartKey.carReport.rpSetting.UserSetting.stUserActionSetting.RsvEngCtrl.unRcvSMSTime;
	
	return true;
}

bool ParseGeoFenceMessage(stCarReport* pstGeoFenceSetting)
{
    stGeoFenceSetting* pstGeofenceValue = &pstGeoFenceSetting->rpSetting.UserSetting.stUserActionSetting.Geofence;

    memset((char*)pstGeofenceValue,0,sizeof(stGeoFenceSetting));
	long long llTemp;
    //MONI increase buffer size for geo fence.
	uint8_t aTempBuffer[512];

	Trace("@ParseGeoFenceMessage()\r\n");

	//hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);
	hexdump(gb_RemoteMessageBuffer, 16);

	//************************************************************************
	// STX: 1
	// VIN: 18
	// ID: 20
	// SOURCE: 1
	// DESTINATION: 1
	// MSG DATE/TIME: 6
	// UTC MSG DATE/TIME: 6
	// OPCODE: 1
	// LENGTH: 2
	// --> 여기까지 56이다.(따라서, 112)

	// DATA

	// CHECKSUM: 1
	// ETX: 1
	// --> 실제 갯수는 HEX ASCII 이므로 2배이다.
	//************************************************************************

    int32_t nBufPosition = CONTROL_REQUEST_COMMAND_DATA_POS;

/*
	// command type: 1
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 1);
	stGeofenceValue.cCommand = (eREMOTE_CON_CMD_TYPE)aTempBuffer[0];

    nBufPosition+=2;
*/
	// GUID: 32
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], MAX_GUID_LENGTH);
	Trace("Received GUID:\r\n");
	hexdump(aTempBuffer, MAX_GUID_LENGTH);

	if(memcmp((char *)aTempBuffer, MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH) != 0) {
		Trace("MSG: mismatch GUID!!!\r\n");
        Trace("Request GUID:\r\n");
        hexdump(MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);
		return false;
	}

    // copy guid
    memcpy((char*)pstGeoFenceSetting->rpSetting.UserSetting.RequestGUID,(char*)&aTempBuffer,32);

    nBufPosition+=64;

	// enable flag of all at a one time : 1byte
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 1);
	pstGeofenceValue->bEnableAllGeofence = ((aTempBuffer[0]=='Y')?true:false);

    nBufPosition+=2;

	// legnth size : 2bytes
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 2);
    pstGeofenceValue->usLengthSize = aTempBuffer[0]<<8|aTempBuffer[1];

    nBufPosition+=4;

    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition],pstGeofenceValue->usLengthSize);

    //uint8_t x1,x2,x3,x4,x5,x6,x7,x8;
	// length and count according to the length size : variables
    //ssscanf((char *)aTempBuffer, stGeofenceValue.usLengthSize, "%x,%x,%x,%x,%x,%x,%x,%x", &x1,&x2,&x3,&x4,&x5,&x6,&x7,&x8);
    int nGeofenceCount = 0;
    nGeofenceCount = GetGetfenceCount((char *)aTempBuffer,pstGeofenceValue->usLengthSize);

    nBufPosition+=(pstGeofenceValue->usLengthSize*2);

    // parse length count
    for(int i=0;i<nGeofenceCount;i++)
    {
        parseGeofenceSubValue((char*)&gb_RemoteMessageBuffer[nBufPosition],&pstGeofenceValue->stGeofencePoint[i]);

        // because 1 geofence setting is 15bytes
        nBufPosition+=30;
    }

	// 발생시간: 6
	memset(aTempBuffer,0,sizeof(aTempBuffer));
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 6);

	llTemp = 0;
	llTemp |= aTempBuffer[0];
	llTemp <<= 8;
	llTemp |= aTempBuffer[1];
	llTemp <<= 8;
	llTemp |= aTempBuffer[2];
	llTemp <<= 8;
	llTemp |= aTempBuffer[3];
	llTemp <<= 8;
	llTemp |= aTempBuffer[4];
	llTemp <<= 8;
	llTemp |= aTempBuffer[5];

    // not used
	//itoa_1(llTemp, (char *)pstGeofenceValue->unDateTime, 14);
    sprintf((char *)aTempBuffer,"%lld\x00",llTemp);
    Trace("Occurred Event Time : %s\r\n",aTempBuffer);

	stHalRTCTypeDef stDate;
    GetDatefromDateArray((char *)aTempBuffer,&stDate);
    GetTimefromDate2(stDate/*,pstGeofenceValue->unDateTime*/);

    // not used this direct setting value
    // we will send setting value to system
    //SetGeofenceSetting(stGeofenceValue);

	return true;
}

bool ParsePolygonGeoFenceMessage(stCarReport* pstPolygonGeoFenceSetting)
{
    uint16_t usPointListSize[MAX_POLYGON_LIST]={0,};
    stPolygonGeoFenceSetting* pstPolygonGeofenceValue = &pstPolygonGeoFenceSetting->rpSetting.UserSetting.stUserActionSetting.PolygonGeofence;

    memset((char*)pstPolygonGeofenceValue,0,sizeof(stPolygonGeoFenceSetting));
	long long llTemp;
    //MONI increase buffer size for geo fence.
	uint8_t aTempBuffer[512];

	Trace("@ParsePolygonGeoFenceMessage()\r\n");

	//hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);
	if( gn_RemoteMessageBufferLength-CONTROL_REQUEST_COMMAND_DATA_POS < MAX_CONTROL_REQUEST_FROM_SERVER_BUFFER_LENGTH )
		hexdump(&gb_RemoteMessageBuffer[CONTROL_REQUEST_COMMAND_DATA_POS], gn_RemoteMessageBufferLength-CONTROL_REQUEST_COMMAND_DATA_POS);

	//************************************************************************
	// STX: 1
	// VIN: 18
	// ID: 20
	// SOURCE: 1
	// DESTINATION: 1
	// MSG DATE/TIME: 6
	// UTC MSG DATE/TIME: 6
	// OPCODE: 1
	// LENGTH: 2
	// --> 여기까지 56이다.(따라서, 112)

	// DATA

	// CHECKSUM: 1
	// ETX: 1
	// --> 실제 갯수는 HEX ASCII 이므로 2배이다.
	//************************************************************************

    int32_t nBufPosition = CONTROL_REQUEST_COMMAND_DATA_POS;

/*
	// command type: 1
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 1);
	stGeofenceValue.cCommand = (eREMOTE_CON_CMD_TYPE)aTempBuffer[0];

    nBufPosition+=2;
*/
	// GUID: 32
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], MAX_GUID_LENGTH);
	Trace("Received GUID:\r\n");
	hexdump(aTempBuffer, MAX_GUID_LENGTH);

	if(memcmp((char *)aTempBuffer, MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH) != 0) {
		Trace("MSG: mismatch GUID!!!\r\n");
        Trace("Request GUID:\r\n");
        hexdump(MessageManagerData.aRemoteControlGUID, MAX_GUID_LENGTH);
		return false;
	}

    //copy guid
    memcpy((char*)pstPolygonGeoFenceSetting->rpSetting.UserSetting.RequestGUID,(char*)&aTempBuffer,32);

    nBufPosition+=64;

	// enable flag of all at a one time : 1byte
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 1);
	pstPolygonGeofenceValue->bEnableAllGeofence = ((aTempBuffer[0]=='Y')?true:false);

    nBufPosition+=2;

    // 발생시간: 6
    memset(aTempBuffer,0,sizeof(aTempBuffer));
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 6);

    llTemp = 0;
    llTemp |= aTempBuffer[0];
    llTemp <<= 8;
    llTemp |= aTempBuffer[1];
    llTemp <<= 8;
    llTemp |= aTempBuffer[2];
    llTemp <<= 8;
    llTemp |= aTempBuffer[3];
    llTemp <<= 8;
    llTemp |= aTempBuffer[4];
    llTemp <<= 8;
    llTemp |= aTempBuffer[5];

    // not used
    //itoa_1(llTemp, (char *)pstGeofenceValue->unDateTime, 14);
    sprintf((char *)aTempBuffer,"%lld\x00",llTemp);
    Trace("Occurred Event Time : %s\r\n",aTempBuffer);

    stHalRTCTypeDef stDate;
    GetDatefromDateArray((char *)aTempBuffer,&stDate);
    GetTimefromDate2(stDate/*,pstGeofenceValue->unDateTime*/);

    nBufPosition+=12;

	// total list legnth size : 2bytes
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 2);
    pstPolygonGeofenceValue->usLengthSize = aTempBuffer[0]<<8|aTempBuffer[1];

    nBufPosition+=4;

    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition],pstPolygonGeofenceValue->usLengthSize);

    // this is total polygon list count
    pstPolygonGeofenceValue->ucPolygonGeofenceCount = GetPolygonGeofenceListCount((char *)aTempBuffer,pstPolygonGeofenceValue->usLengthSize,usPointListSize);

    nBufPosition+=(pstPolygonGeofenceValue->usLengthSize*2);

    Trace("polygon enable : %d\r\n",pstPolygonGeofenceValue->bEnableAllGeofence);
    Trace("polygon total size : %d\r\n",pstPolygonGeofenceValue->usLengthSize);
    Trace("polygon list count : %d\r\n",pstPolygonGeofenceValue->ucPolygonGeofenceCount);

    if( pstPolygonGeofenceValue->ucPolygonGeofenceCount > MAX_POLYGON_LIST )
    {
        printf("Error / Recevied Polygon Count Over\r\n");
        pstPolygonGeofenceValue->ucPolygonGeofenceCount = MAX_POLYGON_LIST;
    }

    // this is sub body polygon list count    
    for(int i=0;i<pstPolygonGeofenceValue->ucPolygonGeofenceCount;i++)
    {
        int nTempSubBodySize=usPointListSize[i];

        // parse length count
        parsePolygonGeofenceSubValue((char*)&gb_RemoteMessageBuffer[nBufPosition],nTempSubBodySize,&pstPolygonGeofenceValue->stPolygonGeofenceList[i]);
 
        nBufPosition+=(usPointListSize[i]*2);
    }

    for(int i=0;i<pstPolygonGeofenceValue->ucPolygonGeofenceCount;i++)
    {
        Trace("#%d] Enable : %d\r\n",i,pstPolygonGeofenceValue->stPolygonGeofenceList[i].bActive);
        Trace("#%d] GeofenceID : %x\r\n",i,pstPolygonGeofenceValue->stPolygonGeofenceList[i].unGeofenceID);
        Trace("#%d] Type : %d\r\n",i,pstPolygonGeofenceValue->stPolygonGeofenceList[i].cFenceType);
        Trace("#%d] Point List Count : %d\r\n",i,pstPolygonGeofenceValue->stPolygonGeofenceList[i].ucPointListCount);
        for(int j=0;j<pstPolygonGeofenceValue->stPolygonGeofenceList[i].ucPointListCount;j++)
        {
            Trace("#%d] Lat : %f, Lon : %f\r\n",i,pstPolygonGeofenceValue->stPolygonGeofenceList[i].stPointList[j].GpsLatitude,
                pstPolygonGeofenceValue->stPolygonGeofenceList[i].stPointList[j].GpsLongitude);
        }
        
    }
    // not used this direct setting value
    // we will send setting value to system
    //SetGeofenceSetting(stGeofenceValue);
	return true;
}


bool ParseIpekPhase1Message(stReportIpek* pstIpekProperties)
{
	//long long llTemp;
    //MONI increase buffer size for geo fence.
	uint8_t aTempBuffer[512] = {0,};

	Trace("@ParseIpekPhase1Message()\r\n");

	//hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);
	hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);

	//************************************************************************
	// STX: 1
	// VIN: 18
	// ID: 20
	// SOURCE: 1
	// DESTINATION: 1
	// MSG DATE/TIME: 6
	// UTC MSG DATE/TIME: 6
	// OPCODE: 1
	// LENGTH: 2
	// --> 여기까지 56이다.(따라서, 112)

	// DATA

	// CHECKSUM: 1
	// ETX: 1
	// --> 실제 갯수는 HEX ASCII 이므로 2배이다.
	//************************************************************************

    int32_t nBufPosition = CONTROL_REQUEST_COMMAND_DATA_POS;

    // srand size : 2bytes
    /* //jkc_0230901_BEGIN -- 확인 필요
    if(gb_RemoteMessageBuffer[nBufPosition] == 0 && gb_RemoteMessageBuffer[nBufPosition+1] == 0)
        return false;
    */ //jkc_0230901_END -- 확인 필요
    
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 2);
    pstIpekProperties->Phase1.SrandSize = (aTempBuffer[0]<<8|aTempBuffer[1])*2;
    nBufPosition+=(2*2);

    GIT_Assert(nBufPosition<1500,eErrorCodeMsg|eIpekSrandSizeLimit);

	// srand : n
	//Convert_HexString_To_Bin((char *)pstIpekProperties->Phase1.Srand, (char *)&gb_RemoteMessageBuffer[nBufPosition], pstIpekProperties->Phase1.SrandSize);
    memcpy(pstIpekProperties->Phase1.Srand,&gb_RemoteMessageBuffer[nBufPosition],pstIpekProperties->Phase1.SrandSize);    
    nBufPosition+=(pstIpekProperties->Phase1.SrandSize);

    hexdump(pstIpekProperties->Phase1.Srand,pstIpekProperties->Phase1.SrandSize);

    GIT_Assert(nBufPosition<1500,eErrorCodeMsg|eIpekSrandLimit);

    // hash size : 2bytes
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 2);
    pstIpekProperties->Phase1.HashSize= (aTempBuffer[0]<<8|aTempBuffer[1])*2;
    nBufPosition+=(2*2);

    GIT_Assert(nBufPosition<1500,eErrorCodeMsg|eIpekHashSizeLimit);

	// hash : n
	//Convert_HexString_To_Bin((char *)pstIpekProperties->Phase1.Hash, (char *)&gb_RemoteMessageBuffer[nBufPosition], pstIpekProperties->Phase1.HashSize);
    memcpy(pstIpekProperties->Phase1.Hash,&gb_RemoteMessageBuffer[nBufPosition],pstIpekProperties->Phase1.HashSize);
    nBufPosition+=(pstIpekProperties->Phase1.HashSize);

    hexdump(pstIpekProperties->Phase1.Hash,pstIpekProperties->Phase1.HashSize);

    GIT_Assert(nBufPosition<1500,eErrorCodeMsg|eIpekHashLimit);

    // sign size : 2bytes
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 2);
    pstIpekProperties->Phase1.SignSize = (aTempBuffer[0]<<8|aTempBuffer[1])*2;
    nBufPosition+=(2*2);

    GIT_Assert(nBufPosition<1500,eErrorCodeMsg|eIpekSignSizeLimit);

	// sign : n
	//Convert_HexString_To_Bin((char *)pstIpekProperties->Phase1.Sign, (char *)&gb_RemoteMessageBuffer[nBufPosition], pstIpekProperties->Phase1.SignSize);
	memcpy(pstIpekProperties->Phase1.Sign, (char *)&gb_RemoteMessageBuffer[nBufPosition], pstIpekProperties->Phase1.SignSize);
    nBufPosition+=(pstIpekProperties->Phase1.SignSize);

    hexdump(pstIpekProperties->Phase1.Sign,pstIpekProperties->Phase1.SignSize);

    GIT_Assert(nBufPosition<1500,eErrorCodeMsg|eIpekSignLimit);

	return true;
}

bool ParseIpekPhase2Message(stReportIpek* pstIpekProperties)
{
	//long long llTemp;
    //MONI increase buffer size for geo fence.
	uint8_t aTempBuffer[512];

	Trace("@ParseIpekPhase2Message()\r\n");

	//hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);
	hexdump(gb_RemoteMessageBuffer, gn_RemoteMessageBufferLength);

	//************************************************************************
	// STX: 1
	// VIN: 18
	// ID: 20
	// SOURCE: 1
	// DESTINATION: 1
	// MSG DATE/TIME: 6
	// UTC MSG DATE/TIME: 6
	// OPCODE: 1
	// LENGTH: 2
	// --> 여기까지 56이다.(따라서, 112)

	// DATA

	// CHECKSUM: 1
	// ETX: 1
	// --> 실제 갯수는 HEX ASCII 이므로 2배이다.
	//************************************************************************

    int32_t nBufPosition = CONTROL_REQUEST_COMMAND_DATA_POS;

    memset((char*)&pstIpekProperties->Ipek,0,sizeof(pstIpekProperties->Ipek));
    
    // ipek size : 2bytes
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 2);
    pstIpekProperties->Ipek.IpekSize = (aTempBuffer[0]<<8|aTempBuffer[1])*2;
    nBufPosition+=(2*2);

    GIT_Assert(nBufPosition<1500,eErrorCodeMsg|eIpekSizeLimit);

	// ipek : n
	//Convert_HexString_To_Bin((char *)pstIpekProperties->Ipek.Ipek, (char *)&gb_RemoteMessageBuffer[nBufPosition], pstIpekProperties->Ipek.IpekSize);
    memcpy(pstIpekProperties->Ipek.Ipek,&gb_RemoteMessageBuffer[nBufPosition],pstIpekProperties->Ipek.IpekSize);
    nBufPosition+=pstIpekProperties->Ipek.IpekSize;
    
    Trace("Ipek : \r\n");hexdump(pstIpekProperties->Ipek.Ipek,pstIpekProperties->Ipek.IpekSize);
    
    GIT_Assert(nBufPosition<1500,eErrorCodeMsg|eIpekLimit);

    return true;
}

void ParseSetTrackingMessage(stMsgSysMsg* pstMessage)
{
	uint8_t aTempBuffer[MAX_SERVER_URL_LENGTH*3];
    int32_t nBufPosition = CONTROL_REQUEST_COMMAND_DATA_POS;
	//int nLength=0;

    stUserSetting stUserSettingMessage;
    memset((char*)&stUserSettingMessage,0,sizeof(stUserSetting));

	Trace("@ParseSetTrackingMessage()\r\n");

    // GUID: 32
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], MAX_GUID_LENGTH);
    //Trace("GUID:\r\n"); hexdump(aTempBuffer, MAX_GUID_LENGTH);
    memcpy((char*)&pstMessage->carReport.rpSetting.UserSetting.RequestGUID,(char*)&aTempBuffer,MAX_GUID_LENGTH);
	memset((char*)&pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.TrackingInfo.ucArrGUID,0x00,MAX_GUID_LENGTH+1);
    memcpy((char*)&pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.TrackingInfo.ucArrGUID,(char*)&aTempBuffer,MAX_GUID_LENGTH);
    nBufPosition+=(MAX_GUID_LENGTH*2);

	//Active
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 1);
	pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.TrackingInfo.bActive = aTempBuffer[0];
    nBufPosition+=2;

	// Interval  : sec
	Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 1);
	pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.TrackingInfo.ucDurationMinTime = aTempBuffer[0];
    nBufPosition+=2;
	
    //RunTime : Min
    Convert_HexString_To_Bin((char *)aTempBuffer, (char *)&gb_RemoteMessageBuffer[nBufPosition], 1);
	pstMessage->carReport.rpSetting.UserSetting.stUserActionSetting.TrackingInfo.ucIntervalSec = aTempBuffer[0];
    nBufPosition+=2;
}

eGitFresult MSG_WriteErrorFile(char *pData, uint16_t nLen)
{
	UINT dwFileSize;
	stFileSystemDescript fpFile;
	eGitFresult ret = GIT_FR_OK;

	Trace("MSG: store failed message - len: %d\r\n", nLen);
	hexdump(pData, nLen);

	if ( (ret = git_f_open(&fpFile, (const char *)MessageManagerData.aErrMsgFileName, GIT_FA_OPEN_ALWAYS | GIT_FA_WRITE)) == GIT_FR_OK ) {
		ret = git_f_lseek(&fpFile, git_f_size(&fpFile));

		// message 전송 시간
		ret = git_f_write(&fpFile, (char *)MessageManagerData.aSaveDataTimeStampForError, strlen((char *)MessageManagerData.aSaveDataTimeStampForError), &dwFileSize);
		if(ret != GIT_FR_OK) {
			git_f_close(&fpFile);

			Trace("@%s(), %s write fail!!! %d \r\n", __FUNCTION__, MessageManagerData.aErrMsgFileName, ret);

			return GIT_FR_DISK_ERR;
		}

		ret = git_f_write(&fpFile, (char *)pData, nLen, &dwFileSize);
		if(ret != GIT_FR_OK) {
			git_f_close(&fpFile);

			Trace("@%s(), %s write fail!!! %d \r\n", __FUNCTION__, MessageManagerData.aErrMsgFileName, ret);

			return GIT_FR_DISK_ERR;
		}

		ret = git_f_write(&fpFile, (char *)"\r\n", 2, &dwFileSize);
		if(ret != GIT_FR_OK) {
			git_f_close(&fpFile);

			Trace("@%s(), %s write fail!!! %d \r\n", __FUNCTION__, MessageManagerData.aErrMsgFileName, ret);

			return GIT_FR_DISK_ERR;
		}
	}
	else {
		Trace("@%s(), %s open fail!!! %d \r\n", __FUNCTION__, MessageManagerData.aErrMsgFileName, ret);

		return GIT_FR_DISK_ERR;
	}

	Trace("@%s(), %s Success !!! %d \r\n\r\n\r\n", __FUNCTION__, MessageManagerData.aErrMsgFileName, ret);

	git_f_close(&fpFile);

	return GIT_FR_OK;
}

eGitFresult MSG_ReadErrorFile(void)
{
	UINT dwFileSize2;
	stFileSystemDescript fpFile;
	eGitFresult ret = GIT_FR_OK;
	uint8_t arrTempBuffer[512];
	uint16_t i;

	if ( (ret = git_f_open(&fpFile, (const char *)MessageManagerData.aErrMsgFileName, GIT_FA_EXIST | GIT_FA_READ)) == GIT_FR_OK ) {
		while(1) {
			if ((ret = git_f_read(&fpFile, arrTempBuffer, (UINT)sizeof(arrTempBuffer), (UINT *)&dwFileSize2)) == GIT_FR_OK ) {
				if(dwFileSize2 > 0) {
					//hexdump(arrTempBuffer, dwFileSize2);
					for(i = 0; i < dwFileSize2; i++) {
						Trace("%c", arrTempBuffer[i]);
					}
				}
				else {
					break;
				}
			}
			else {
				break;
			}
		}

		git_f_close(&fpFile);
	}
	else {
		Trace("@%s(), %s open fail!!! %d \r\n", __FUNCTION__, MessageManagerData.aErrMsgFileName, ret);

		return GIT_FR_DISK_ERR;
	}

	return GIT_FR_OK;
}

eGitFresult MSG_DelErrorFile(void)
{
	eGitFresult ret = GIT_FR_OK;

	if ( (ret = git_f_unlink((const char *)MessageManagerData.aErrMsgFileName)) == GIT_FR_OK ) {
	}
	else {
		Trace("@%s(), %s delete fail!!! %d \r\n", __FUNCTION__, MessageManagerData.aErrMsgFileName, ret);

		return GIT_FR_DISK_ERR;
	}

	return GIT_FR_OK;
}

eGitFresult MSG_DelAllErrorFile(void)
{
	eGitFresult res;
	stFileSystemDescript  Dir;
#if defined(USE_GIT_FAT_FS)
	FILINFO Finfo;
	char Lfname[128];
	UINT s1 = 0, s2 = 0;
	long p1 = 0;
	char* strScanDirectory = DIR_ROOT;

	memset(&Finfo, NULL, sizeof(Finfo));

	res = git_f_opendir(&Dir, strScanDirectory);

	if (res) {
		return res;
	}

	Trace("\r\n");
	Trace("fs: current directory - [%s]\r\n", DIR_ROOT);

	for(;;) {
#if _USE_LFN
		Finfo.lfname = Lfname;
		Finfo.lfsize = sizeof(Lfname);
		Finfo.fsize = 0;
#endif
		res = git_f_readdir(&Dir, &Finfo);
		if ((res != GIT_FR_OK) || !Finfo.fname[0])
			break;

		if (Finfo.fattrib & AM_DIR) {
			s2++;
			continue;
		}
		else {
			s1++;
			p1 += Finfo.fsize;
		}

		Trace("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s ",
										(Finfo.fattrib & AM_DIR) ? 'D' : '-',
										(Finfo.fattrib & AM_RDO) ? 'R' : '-',
										(Finfo.fattrib & AM_HID) ? 'H' : '-',
										(Finfo.fattrib & AM_SYS) ? 'S' : '-',
										(Finfo.fattrib & AM_ARC) ? 'A' : '-',
										(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
										(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
										Finfo.fsize, &(Finfo.fname[0]));
#if _USE_LFN
		if(strlen((const char *)Lfname) > 0) {
			Trace("LFN: [%s]\r\n", Lfname);
		}
		else {
			Trace("\r\n");
		}
#else
		Trace("\r\n");
#endif

		if(memcmp((char *)Lfname, "ErrMsg_", 7) == 0) {
			if ( (res = git_f_unlink((const char *)Lfname)) == GIT_FR_OK ) {
				Trace("FS: delete file [%s]\r\n", Lfname);
			}
			else {
				Trace("@%s(), [%s] delete fail!!! %d \r\n", __FUNCTION__, Lfname, res);
			}
		}
	}
	
#elif defined(USE_GIT_LITTLE_FS)
	struct lfs_info Finfo;
	char Lfname[128];
	UINT s1 = 0, s2 = 0;
	long p1 = 0;
	char* strScanDirectory = DIR_ROOT;

	memset(&Finfo, NULL, sizeof(Finfo));

	res = git_f_opendir(&Dir, strScanDirectory);

	if (res) {
		return res;
	}

	Trace("\r\n");
	Trace("fs: current directory - [%s]\r\n", DIR_ROOT);

	for(;;) {
#if 0 //_USE_LFN mod.pdh temp...
		Finfo.lfname = Lfname;
		Finfo.lfsize = sizeof(Lfname);
		Finfo.fsize = 0;
#endif
		res = git_f_readdir(&Dir, &Finfo);
		if ((res != GIT_FR_OK) || !Finfo.name[0])
			break;

		if (Finfo.type & LFS_TYPE_DIR) {
			s2++;
			continue;
		}
		else {
			s1++;
			p1 += Finfo.size;
		}

		Trace("%c %9lu  %s ",		(Finfo.type & LFS_TYPE_DIR) ? 'D' : '-',
									Finfo.size, &(Finfo.name[0]));
#if _USE_LFN
		if(strlen((const char *)Lfname) > 0) {
			Trace("LFN: [%s]\r\n", Lfname);
		}
		else {
			Trace("\r\n");
		}
#else
		Trace("\r\n");
#endif

		if(memcmp((char *)Lfname, "ErrMsg_", 7) == 0) {
			if ( (res = git_f_unlink((const char *)Lfname)) == GIT_FR_OK ) {
				Trace("FS: delete file [%s]\r\n", Lfname);
			}
			else {
				Trace("@%s(), [%s] delete fail!!! %d \r\n", __FUNCTION__, Lfname, res);
			}
		}
	}
	
	git_f_closedir(&Dir);
#endif

	return GIT_FR_OK;
}
