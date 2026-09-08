/*************************************************************
 * NOTE : git_record.c
 *      
 * Author : 
 * Since : 2019.12.18
**************************************************************/
#include <string.h>

#include "typedef.h"

#include "git_PassthruDefines.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

#include "common.h"
#include "firmware.h"
#include "git_protocol.h"
#include "sw_timer.h"
#include "buzzer.h"
#include "git_fsutil.h"
#include "git_rtc.h"
#include "git_ioctl.h"

#include "git_function_list.h"


#include "git_record.h"
#include "git_record_file.h"
#include "git_trigger.h"

#include "ff.h"


//MONI #include "PassThruStruct.h"

extern void hexdump(uint8_t* pucBuffer, int32_t nLength);


typedef struct __stTriggerBluetoothControl{
	uint8_t bBtConnected;
}stTriggerBluetoothControl;

stTriggerBluetoothControl m_stTriBtCtrl;


void FunctionID_0xD001(void *pInterPtcl, uint32_t eInCommType, uint32_t usLength )
{
	//GLogI("%s] rcv message\r\n", __func__);

	SetCurTriggerModuleLedObdStatus();

	SendSignalBluetoothAlive();
}

void FunctionID_0xD002(void *pInterPtcl, uint32_t eInCommType, uint32_t usLength )
{
	GLogI("%s] rcv message\r\n", __func__);


	SetCurTriggerModuleButtonStatus((uint8_t*)pInterPtcl);
}

void FunctionID_0xD004(void *pInterPtcl, uint32_t eInCommType, uint32_t usLength )
{
	U8 *pPayloadPtcl = (U8*)pInterPtcl;
	GLogI("%s] rcv message size : %d\r\n", __func__, usLength);
	//hexdump(pPayloadPtcl,usLength);

	SetCurTriggerModuleConnection();
}


void FunctionID_0xD005(void *pInterPtcl, uint32_t eInCommType, uint32_t usLength )
{	
	GLogI("%s] rcv message\r\n", __func__);
#if false	
	if(strCommDataBuff->Data[0]==0x00)	TM_VersionStep=4;
	else TM_VersionStep=98;
#endif	
}

void FunctionID_0xD006(void *pInterPtcl, uint32_t eInCommType, uint32_t usLength )
{	
	GLogI("%s] rcv message\r\n", __func__);
#if false	
	if(strCommDataBuff->Data[0]==0x00)
	{
		if(TM_VersionStep==97) TM_VersionStep=4;	//반복 전송
		if(TM_VersionStep==99) TM_VersionStep=5;	//마지막 전송
	}
	else TM_VersionStep=98;
#endif
}

void FunctionID_0xD007(void *pInterPtcl, uint32_t eInCommType, uint32_t usLength )
{	
	GLogI("%s] rcv message\r\n", __func__);
#if false	
	U16 TM_CS;
	FRESULT fsResult;
	FIL Filepnt;

	U16 returnValue = 0;
	U32 i, iReadByte;
	U8 cBuff = 0;
	
	memcpy(&TM_CS,strCommDataBuff->Data,sizeof(TM_CS));
	
	if( f_open(&Filepnt, strOpenFileName, FA_OPEN_EXISTING | FA_WRITE | FA_READ) == FR_OK )
	{
		if ( f_lseek(&Filepnt, 0) == FR_OK )
		{
			for(i=0; i<TMFileCounter; i++)
			{
				fsResult = f_read(&Filepnt, (void*)&cBuff, 1, &iReadByte);
				if (  fsResult == FR_OK )
					returnValue += cBuff;
				else
					GLogI(&U1, "CRC Read fail\r\n");
			}
		}
		
		GLogI(&U1, "\r\n%s: checksum %d\r\n", __FUNCTION__, returnValue);	
		f_close(&Filepnt);
	}
	else
	{
		TM_VersionStep=3;
		GLogI(&U1, "%s File Open fail\r\n", strOpenFileName);
		return;
	}
	
	if(returnValue!= TM_CS)
	{
		for(i=0; i<3;i++)
		{
			strTempFileName = &TMdata[i][2];
			//memcpy(strTempFileName,TMdata[i+2],20);
			if( strcmp(strOpenFileName, strTempFileName)==0) TMdata[i][27]=1;
		}
		TM_VersionStep=3;
	}
	else TM_VersionStep=6;
#endif
}

void FunctionID_0xD008(void *pInterPtcl, uint32_t eInCommType, uint32_t usLength )
{
	GLogI("%s] rcv message\r\n", __func__);
#if false	
	TM_VersionStep=3;
#endif
}

void FunctionID_0xD009(void *pInterPtcl, uint32_t eInCommType, uint32_t usLength )
{
	GLogI("%s] rcv message\r\n", __func__);
#if false	
	TM_VersionStep=8;
#endif
}

void FunctionID_0xD00A(void *pInterPtcl, uint32_t eInCommType, uint32_t usLength )
{
	GLogI("%s] rcv message\r\n", __func__);
#if false	
	U8 *arrTemp,*arrTempS;
	UINT iLength;	
	char seps[] = " ,:;\t\r\n",*token ;
	int iTokenCnt = 0 ;
	U8 i=0,j=0,temp;
	U16 TMfwlst_length=0;
	
	arrTemp = (U8*)malloc(1000);
	
	memset(arrTemp, 0x0, 500);
	if ( GetVCI2FWAppList(arrTemp, &iLength) == TRUE ) //파일이름이 같은지 찾아내어 배열에 저장
	{
		token = strtok((char*)arrTemp, seps);
		while ( token != NULL )
		{
			if ( iTokenCnt%3 == 0 )
			{
				TMdata[i][0]= *token;
				TMdata[i][1]= ',';
				//j++;
			}
			else if ( iTokenCnt%3 == 1 )
			{
				if ( strcmp(token, "TM_Bootloader.bin") == 0 )
				{
					strcpy(&TMdata[i][2],token);
					i++;
				}
				else if ( strcmp(token, "TM_Downloader.bin") == 0 )
				{
					strcpy(&TMdata[i][2],token);
					i++;
				}
				else if ( strcmp(token, "Trigger_Module.bin") == 0 )
				{
					strcpy(&TMdata[i][2],token);
					i++;
				}
				//j++;
			}
			else if ( iTokenCnt%3 == 2 )
			{
				if(i==1) strcpy(&TMdata[0][22],token);
				if(i==2) strcpy(&TMdata[1][22],token);
				if(i==3){ strcpy(&TMdata[2][22],token);	 break; }
				//j=0;
			}
			token = strtok(NULL, seps);
			iTokenCnt++;
		}
	}
	free(arrTemp);
	
	i=0;
	iTokenCnt=0;
	arrTemp = (U8*)malloc(strCommDataBuff->DataLength-8);
	arrTempS = (U8*)malloc(strCommDataBuff->DataLength-8);
	memcpy(arrTemp , strCommDataBuff->Data, strCommDataBuff->DataLength-8);
	memcpy(arrTempS, strCommDataBuff->Data, strCommDataBuff->DataLength-8);	
		
	token = strtok((char*)arrTemp, seps);
	while ( token != NULL )
	{
		if ( iTokenCnt%3 == 0 )
		{
			temp = *token;
			//j++;
		}
		else if ( iTokenCnt%3 == 1 )
		{
			if		( strcmp(token, "TM_Bootloader.bin") == 0 )  {TMfwlst_length += strlen(token)+10;	 i++;} //10byte = 'appnumber' ',' name ',' 'ver' 'ent'
			else if ( strcmp(token, "TM_Downloader.bin") == 0 )  {TMfwlst_length += strlen(token)+10;	 i++;}
			else if ( strcmp(token, "Trigger_Module.bin") == 0 ) {TMfwlst_length += strlen(token)+10;	 i++;}
			//j++;
		}
		else if ( iTokenCnt%3 == 2 )
		{
			for(j=0;j<3;j++)
			{
				if((i!=0)&&(temp == TMdata[j][0])&& (strcmp(token,&TMdata[j][22])!=0)) //변경할 버젼 저장 및 flag 셋팅
				{
					TMdata[i-1][27]= 1;				// update해야할 bin flag set
					TM_VersionStep=1;
					//&arrTemp[TMfwlst_length-7] = &TMdata[j][22];  // 최신버젼을 버퍼에 갱신
					//strcpy(&arrTempS[TMfwlst_length-7],&TMdata[j][22]);  // 최신버젼을 버퍼에 갱신
					memcpy(&arrTempS[TMfwlst_length-7],&TMdata[j][22],5);  // 최신버젼을 버퍼에 갱신
					GLogI(&U1,"diff %s ",token);
				}
			}
		}
		token = strtok(NULL, seps);
		iTokenCnt++;
	}
	
//	sprintf((char*)InBuff, "%d,%s,%s\r\n%d,%s,%s\r\n%d,%s,%s\r\n%X,%s,%s\r\n", 
//		DEFAULT_BOOTLOADER, g_tbFWAppInfo[DEFAULT_BOOTLOADER].strFWName, g_FirmwareInfo.AppProperty[DEFAULT_BOOTLOADER].arrAppFWVersion,
//		DEFAULT_DOWNLOADER, g_tbFWAppInfo[DEFAULT_DOWNLOADER].strFWName, g_FirmwareInfo.AppProperty[DEFAULT_DOWNLOADER].arrAppFWVersion,
//		DEFAULT_APPLICATION, g_tbFWAppInfo[DEFAULT_APPLICATION].strFWName, g_FirmwareInfo.AppProperty[DEFAULT_APPLICATION].arrAppFWVersion,
//		0xFF, "TOTAL_VERSION", g_FirmwareInfo.arrFirmwareVersion);
//
//	*iBuffLength = strlen((char*)InBuff);
	
	if(TM_VersionStep!=1) TM_VersionStep=98;	//버전이 다른게 없으므로 종료
	
	f_chdir(DIR_ROOT);
	GetVCI2FileWrite("TMfw.lst",arrTempS, strCommDataBuff->DataLength-8);
	
	free(arrTempS);
	free(arrTemp);
#endif	
}


void FunctionID_0xD00B(void *pInterPtcl, uint32_t eInCommType, uint32_t usLength )
{
	GLogI("%s] rcv message\r\n", __func__);

	//hexdump((uint8_t*)pInterPtcl,usLength);
}


// [0] : current mode, 
// [1] : current device info
// [2-14] : app f/w version info 12array
void FunctionID_0xD00C(void *pInterPtcl, uint32_t eInCommType, uint32_t usLength )
{
	GLogI("%s] rcv message\r\n", __func__);

	//hexdump((uint8_t*)pInterPtcl,usLength);

	//[0] : current mode
	//[1] : module type
	//[2] : app f/w version(12bytes)
	SetCurTriggerModuleInfo((uint8_t*)pInterPtcl);
}

void FunctionID_0xD00D(void *pInterPtcl, uint32_t eInCommType, uint32_t usLength )
{
	GLogI("%s] rcv message\r\n", __func__);
#if false	
  if     (TM_VersionStep==96)  TM_VersionStep=2;	
  else if(TM_VersionStep==95)  TM_VersionStep=9;
#endif  
}

void FunctionID_0xD00E(void *pInterPtcl, uint32_t eInCommType, uint32_t usLength )
{
	GLogI("%s] rcv message\r\n", __func__);
	SetCurTriggerModuleTriggerButton((uint8_t*)pInterPtcl);
}


void FunctionID_0xC025(void *pInterPtcl, uint32_t eInCommType, uint32_t usLength )
{
	GLogI("%s] rcv message\r\n", __func__);
#if false  
    U8 uWlanFw[2];  // 현재 무선보드 FW 버전
    U8 uVciFw[2] = {24, 39};   // 비교 무선보드 FW 버전 '18.27'을 나타내는 16진수임. 24 39
    
    memcpy(&uWlanFw, &strCommDataBuff->Data, strCommDataBuff->DataLength-8);
    
    if(m_stTriBtCtrl.bBtConnected == false)
    {   
        if(uWlanFw[0] > uVciFw[0])
        {
            gWlanFlag = FALSE;
            //GLogI(&U1, "\r\n1.gWlanFlag: %d\n", gWlanFlag); 
        }
        else if(uWlanFw[0] == uVciFw[0])
        {
            if(uWlanFw[1] >= uVciFw[1])
            {
                gWlanFlag = FALSE;
                //GLogI(&U1, "\r\n2.gWlanFlag: %d\n", gWlanFlag); 
            }
            else
            {
                gWlanFlag = TRUE;
                //GLogI(&U1, "\r\n3.gWlanFlag: %d\n", gWlanFlag); 
            }
        }
        else
        {
             gWlanFlag = TRUE;
             //GLogI(&U1, "\r\n4.gWlanFlag: %d\n", gWlanFlag); 
        }
        
        m_stTriBtCtrl.bBtConnected = true;
        
        //FunctionID_0x1208();		//151228 lwh 막음
   }
   else
   {	 
        J2534CommPort = J2534CommPortSever;	// USB로 받아서 SPI 송신 그리고 SPI 수신 후 다시 USB로 송신하기 위함 140409 LWH
        
        memcpy(&strSendCommDataBuff, strCommDataBuff, sizeof(strCommData));
        //strSendCommDataBuff.DataLength 		= 8+sizeof(strCommDataBuff);
        //strSendCommDataBuff.FunctionID 		= 0xC025;
        //strSendCommDataBuff.CurrentFrame 	= 0;
        //strSendCommDataBuff.CheckSum		= 0;
        strSendCommDataBuff.CheckSum 		= J2534CheckFrameCheckSum(&strSendCommDataBuff);
    
        J2534SendDataToPC(&strSendCommDataBuff);
	}   
    
#endif    
}



void FunctionID_0xC053(void *pInterPtcl, uint32_t eInCommType, uint32_t usLength )
{	
	GLogI("%s] rcv message\r\n", __func__);
#if false  
	U8 Remote_Name[16], Address[6],TRIG[5]="TRIG_",trig[5]="trig_",ret=0;
	UINT iLength;
//	static char s_cWakeupFlag=1;
	//eWakeupStatus eWakeupModeStatus;
     
    static U8 BTConnectFlag=0;
    
	BTConnect = strCommDataBuff->Data[0];

    if(BTConnect==1)
    {
		memcpy(&Remote_Name, &strCommDataBuff->Data[1], sizeof(Remote_Name));
		memcpy(&Address, &strCommDataBuff->Data[17], sizeof(Address));
    	
		ret=StringCompareAce89(Remote_Name,TRIG,5);
		if(ret!=1)
		{
			ret=StringCompareAce89(Remote_Name,trig,5);
			if(ret==0) BTConnect=3;		//대문자든 소문자든 트리거랑 연결되었는지 알 수 없다면 알수 없음으로 처리
		}
    }


    if(BTConnect==1)
    {
      if(BTConnectFlag==0)//최초 연결 완료시 멜로디
      {
        if((ucTrigMode&0x01)==0x00){}   //수동트리거 체크안됐을때 눌러도 저장안함
        else
        {
            BTConnectFlag=1;

#if false			
            intLEDCount=0;
            LedStaus=10;
#endif		
			osDelay(700);			
        }
      }
    }
    else
    {
        BTConnectFlag=0;
    }     
	
	// Wakeup 기능 추가
	//BT 연결 완료되면 삭제하고 초기화
	if(BTConnect==1)
	{
		if(TM_VersionStep==0)	
			RES0xC051_4_N();				
	}
	else
	{
		//일반 상태		
	}
	
#endif
}




void RES0xC025(void)    // 20150305 seo 무선 전송 속도 개선 때문에 변경
{
	GLogI("%s] rcv message\r\n", __func__);
#if false  
	strCommData		strSendCommDataBuff;
    U8 gPortTempWlan;    // 20150313 seo
    
    gPortTempWlan = J2534CommPort;

	J2534CommPort = COMM_UART2;
 
	strSendCommDataBuff.DataLength 		= 8;
	strSendCommDataBuff.FunctionID 		= 0xC025;
	strSendCommDataBuff.CurrentFrame 	= 0;
	strSendCommDataBuff.CheckSum		= 0;
	strSendCommDataBuff.CheckSum 		= J2534CheckFrameCheckSum(&strSendCommDataBuff);
	//J2534SendDataToPC(&strSendCommDataBuff)
	J2534SendDataToPC_WLAN(&strSendCommDataBuff);
    
#if 0	//151228 lwh
    if(gPortTempWlan != COMM_UART2)
    {
         g_uChTarget = FALSE;
    }
    else
    {
         g_uChTarget = TRUE;
    } 
#endif
    
    
    
#endif    
}



void RES0xC053(void)
{
	GLogI("%s] rcv message\r\n", __func__);
#if false  
	WLanSendTimeout=5000;
	strCommData		strSendCommDataBuff;
	
	J2534CommPort = COMM_UART2;
	
	strSendCommDataBuff.DataLength 		= 8;
	strSendCommDataBuff.FunctionID 		= 0xC053;
	strSendCommDataBuff.CurrentFrame 	= 0;
	strSendCommDataBuff.CheckSum		= 0;
	strSendCommDataBuff.CheckSum 		= J2534CheckFrameCheckSum(&strSendCommDataBuff);
	J2534SendDataToPC_WLAN(&strSendCommDataBuff);
#endif        
}



void RES0xD00D(U8 cSwitchAppNumber)	//TM_VersionStep=1,8;
{
	GLogI("%s] rcv message\r\n", __func__);
#if false  
	if(cSwitchAppNumber==0x07) TM_VersionStep=96;
	if(cSwitchAppNumber==0x08) TM_VersionStep=95;
	strCommData		strSendCommDataBuff;
	
	cTMAppNumber = cSwitchAppNumber;
	
	J2534CommPort = COMM_UART2;
	
	strSendCommDataBuff.DataLength 		= 8+1;
	strSendCommDataBuff.FunctionID 		= 0xD00D;
	strSendCommDataBuff.CurrentFrame 	= 0;
	strSendCommDataBuff.CheckSum		= 0;
	strSendCommDataBuff.Data[0] 		= cSwitchAppNumber;		//0x06 TM_Bootloader.bin  0x07 TM_Downloader.bin  0x08 Trigger_Module.bin
	strSendCommDataBuff.CheckSum 		= J2534CheckFrameCheckSum(&strSendCommDataBuff);
	J2534SendDataToPC(&strSendCommDataBuff);
#endif        
}

