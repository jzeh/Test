/**
******************************************************************************
* @file    CanFD_Controller.c
* @author  Cho Sang Jun
* @version V1
* @date    6-May-2020
* @brief   CanFD Communication Logic Ba
******************************************************************************
**/

#include "HalHandler.h"
#include "HalCanDriver.h"
/* CanFD Controller */

#include "CanFD_Controller.h"
#include "GIT_VCI.h"
#include "GIT_Util.h"
#include "GIT_CanParsingProc.h"
#include "GIT_OemInterface.h"
#include "CanFD_RecvHandler.h"


stCFDControl m_stCFDCtrl;


boolean_t CFD_GetCanFDAdapter()
{
	return m_stCFDCtrl.bActive;
}

boolean_t CFD_GetInitComplete()
{
	return m_stCFDCtrl.bInitProcessComplete;
}

void CFD_SetInitComplete(boolean_t bIsComplete)
{
	m_stCFDCtrl.bInitProcessComplete = bIsComplete;
}


boolean_t CFD_GetCanFDAdapterControlMode()
{
	return m_stCFDCtrl.bFDControl;
}

void CFD_SetCanFDAdapterControlMode()
{
	m_stCFDCtrl.bFDControl = true;
}

void CFD_SetCanFDAdapter(boolean_t bIsUse)
{
	m_stCFDCtrl.bActive = bIsUse;
}

void CFD_SetCanFDSleepStatus(boolean_t bSleepStatus)
{
	m_stCFDCtrl.bCanFDSleepStatus = bSleepStatus;
}

boolean_t CFD_GetCanFDSleepStatus()
{
	return m_stCFDCtrl.bCanFDSleepStatus;
}
void CFD_SetCanFDConfig(unsigned char ucFDCanLine ,unsigned char ucFDCanSpeed)
{
	switch(ucFDCanLine)
	{
		case CANFD_SPI_1:
			if(m_stCFDCtrl.stConfig.ucCanBaudRate[ucFDCanLine%CANFD_SPI_1] != ucFDCanSpeed)
			{
				m_stCFDCtrl.stConfig.bActvieLine[ucFDCanLine%CANFD_SPI_1] = true;
				m_stCFDCtrl.stConfig.ucCanBaudRate[ucFDCanLine%CANFD_SPI_1] = ucFDCanSpeed;
			}
			break;
		case CANFD_SPI_2:
			if(m_stCFDCtrl.stConfig.ucCanBaudRate[ucFDCanLine%CANFD_SPI_1] != ucFDCanSpeed)
			{
				m_stCFDCtrl.stConfig.bActvieLine[ucFDCanLine%CANFD_SPI_1] = true;
				m_stCFDCtrl.stConfig.ucCanBaudRate[ucFDCanLine%CANFD_SPI_1] = ucFDCanSpeed;
			}
			break;			
		case CANFD_SPI_3:
			if(m_stCFDCtrl.stConfig.ucCanBaudRate[ucFDCanLine%CANFD_SPI_1] != ucFDCanSpeed)		
			{
				m_stCFDCtrl.stConfig.bActvieLine[ucFDCanLine%CANFD_SPI_1] = true;
				m_stCFDCtrl.stConfig.ucCanBaudRate[ucFDCanLine%CANFD_SPI_1] = ucFDCanSpeed;
			}
			
			break;
		case CANFD_SPI_4:
		case CANFD_SPI_5:
			if(m_stCFDCtrl.stConfig.ucCanBaudRate[CANFD_SPI_4%CANFD_SPI_1] != ucFDCanSpeed)		
			{
				m_stCFDCtrl.stConfig.bActvieLine[CANFD_SPI_4%CANFD_SPI_1] = true;
				m_stCFDCtrl.stConfig.ucCanBaudRate[CANFD_SPI_4%CANFD_SPI_1] = ucFDCanSpeed;
			}
			
			if(ucFDCanLine == CANFD_SPI_4)
				m_stCFDCtrl.stConfig.bSelectedAnalogSwitch = false;
			else
				m_stCFDCtrl.stConfig.bSelectedAnalogSwitch = true;
			break;						
	}
}

boolean_t CFD_GetActiveAnalogSwitch()
{
	boolean_t bIsResult = false;
	
	if(m_stCFDCtrl.stConfig.ucActiveSwitchingLine)
	{
		bIsResult = true;
	}
	
	return bIsResult;
}

boolean_t CFD_SelectedAnalogSwitch()
{
	boolean_t bIsResult = m_stCFDCtrl.stConfig.bSelectedAnalogSwitch;
	return bIsResult;
}

void DeFaultCanSetting()
{
	//stCanPacket OutCanPacket;
	uint32_t uiCanChannel = 0;
	uint32_t uiStartMask[HAL_CAN_MAX_MASK_CNT];
	uint32_t uiEndMask[HAL_CAN_MAX_MASK_CNT];

	uiStartMask[0] = 0x0700;
	uiEndMask[0]   = 0x07FF;
	
	//Default Set
	Oem_CAN_Initial_CH1(Highcan2, eCAN_1MBPS); //init
	uiCanChannel = CAN_CHANNEL_1;
	Oem_CAN_Channel_Masket_Set(uiCanChannel, STANDARD_CAN, 1, uiStartMask, uiEndMask);

	Oem_CAN_Initial_CH2(Highcan3, eCAN_1MBPS); //init
	uiCanChannel = CAN_CHANNEL_2;
	Oem_CAN_Channel_Masket_Set(uiCanChannel, STANDARD_CAN, 1, uiStartMask, uiEndMask);
}

void SetCanPacket(eCanFDTxCmd eTxCmd ,unsigned char* pucWriteData)
{
	stCanPacket OutCanPacket;
	unsigned char ucCanData[10];
	uint32_t uiCanChannel = 0;
	//uint32_t uiStartMask[HAL_CAN_MAX_MASK_CNT];
	//uint32_t uiEndMask[HAL_CAN_MAX_MASK_CNT];

	memset(&ucCanData,0x00,sizeof(ucCanData));

	//Default Set
	if(m_stCFDCtrl.unChFinalCanLine != Highcan2)
	{
//		printf(" m_stCFDCtrl.unChFinalCanLine != Highcan2 \n");
		Oem_CAN_Initial_CH1(Highcan2, eCAN_1MBPS); //init
	}
	uiCanChannel = CAN_CHANNEL_1;

	//uiStartMask[0] = 0x0700;
	//uiEndMask[0]   = 0x07FF;

	//DefaultMaskSet(uiStartMask,uiEndMask,8);
		
	OutCanPacket.stNormalPacket.ucSOF = CAN_FRAME_SOF;
	OutCanPacket.stNormalPacket.us11BitID = eTxCmd;	
	OutCanPacket.stNormalPacket.ucRTR = 0;
	OutCanPacket.stNormalPacket.ucIDE = CAN_FRAME_STANDARD_IDE;
	OutCanPacket.stNormalPacket.ucReserved = 0;
	OutCanPacket.stNormalPacket.ucDLC = 8;
	OutCanPacket.stNormalPacket.usCRC = 0;
	OutCanPacket.stNormalPacket.ucCRCDelimiter = 1;
	OutCanPacket.stNormalPacket.ucACK = 1;
	OutCanPacket.stNormalPacket.ucACKDelimiter = 1;
	OutCanPacket.stNormalPacket.ucEOF = CAN_FRAME_EOF;

	if(pucWriteData != NULL)
	{
		memcpy(&OutCanPacket.stNormalPacket.arrDataFields[0],pucWriteData,LENGTH_CANFRAME_DATA);	
	}
	else
	{
		memcpy(&OutCanPacket.stNormalPacket.arrDataFields[0],ucCanData,LENGTH_CANFRAME_DATA);	
	}

	//Oem_CAN_Channel_Masket_Set(uiCanChannel, STANDARD_CAN, 8, uiStartMask, uiEndMask);

	if(FineWriteCanBuff((unsigned char*)&OutCanPacket, sizeof(OutCanPacket), NULL, uiCanChannel) > 0 ) 
	{
		DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
		LCAN_SET_COMM_STATE(eLCAN_RX_BLOCK);
		CFD_SetCanFDAdapterControlMode();
	}
        
	//if(FineWriteCanBuff((unsigned char*)&OutCanPacket, sizeof(OutCanPacket), NULL, uiCanChannel);
}

void SetCanPacket_VariableSize(eCanFDTxCmd eTxCmd ,unsigned char* pucWriteData, unsigned char ucDLC)
{
	stCanPacket OutCanPacket;
	unsigned char ucCanData[10];
	uint32_t uiCanChannel = 0;
	memset(&ucCanData,0x00,sizeof(ucCanData));
	//Default Set
	Oem_CAN_Initial_CH1(Highcan2, eCAN_1MBPS); //init

        uiCanChannel = CAN_CHANNEL_1;

	OutCanPacket.stNormalPacket.ucSOF = CAN_FRAME_SOF;
	OutCanPacket.stNormalPacket.us11BitID = eTxCmd;	
	OutCanPacket.stNormalPacket.ucRTR = 0;
	OutCanPacket.stNormalPacket.ucIDE = CAN_FRAME_STANDARD_IDE;
	OutCanPacket.stNormalPacket.ucReserved = 0;
	OutCanPacket.stNormalPacket.ucDLC = ucDLC;
    OutCanPacket.stNormalPacket.usCRC = 0;
	OutCanPacket.stNormalPacket.ucCRCDelimiter = 1;
	OutCanPacket.stNormalPacket.ucACK = 1;
	OutCanPacket.stNormalPacket.ucACKDelimiter = 1;
	OutCanPacket.stNormalPacket.ucEOF = CAN_FRAME_EOF;

	if(pucWriteData != NULL)
	{
		memcpy(&OutCanPacket.stNormalPacket.arrDataFields[0],pucWriteData,LENGTH_CANFRAME_DATA);	
	}
	else
	{
		memcpy(&OutCanPacket.stNormalPacket.arrDataFields[0],ucCanData,LENGTH_CANFRAME_DATA);	
	}

//Oem_CAN_Channel_Masket_Set(uiCanChannel, STANDARD_CAN, 8, uiStartMask, uiEndMask);

	if(FineWriteCanBuff((unsigned char*)&OutCanPacket, sizeof(OutCanPacket), NULL, uiCanChannel) > 0 ) 
	{
		DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
		CFD_SetCanFDAdapterControlMode();
	}
        
	//if(FineWriteCanBuff((unsigned char*)&OutCanPacket, sizeof(OutCanPacket), NULL, uiCanChannel);
}


void CFD_GetCanFDVersion()
{
	SetCanPacket(eCanFD_GetVerion,NULL);
}

void CFD_SetCanFDVersion(unsigned short usBlVer,unsigned short usAppVer,unsigned short usVerfVer,unsigned short usBUVer)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));

//	memcpy(&ucCanData[0],&usBlVer,sizeof(unsigned short));
//	memcpy(&ucCanData[2],&usAppVer,sizeof(unsigned short));
//	memcpy(&ucCanData[4],&usVerfVer,sizeof(unsigned short));
//	memcpy(&ucCanData[6],&usBUVer,sizeof(unsigned short)); 	
	
	ucCanData[0] = (usBlVer>>8)&0xFF;
	ucCanData[1] = usBlVer&0xFF;
	ucCanData[2] = (usAppVer>>8)&0xFF;
	ucCanData[3] = usAppVer&0xFF;
	ucCanData[4] = (usVerfVer>>8)&0xFF;
	ucCanData[5] = usVerfVer&0xFF;
	ucCanData[6] = (usBUVer>>8)&0xFF;
	ucCanData[7] = usBUVer&0xFF;
	
	SetCanPacket(eCanFD_SetVerion,ucCanData);
}

void CFD_SetFWUpdateStart(unsigned short usVersion , int32_t nSize , unsigned short usTarget)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));

//	memcpy(&ucCanData[0],&usVersion,sizeof(unsigned short));
//	memcpy(&ucCanData[2],&nSize,sizeof(int32_t));
//	memcpy(&ucCanData[6],&usTarget,sizeof(unsigned short));

	ucCanData[0] = (usVersion>>8)&0xFF;
	ucCanData[1] = usVersion&0xFF;
	ucCanData[2] = (nSize>>24)&0xFF;
	ucCanData[3] = (nSize>>16)&0xFF;
	ucCanData[4] = (nSize>>8)&0xFF;
	ucCanData[5] = nSize&0xFF;;
	ucCanData[6] = (usTarget>>8)&0xFF;
	ucCanData[7] = usTarget&0xFF;
	
	SetCanPacket_VariableSize(eCanFD_FWUpdate_StartCmd,ucCanData,8);
}

void CFD_SetFWUpdateTransmit(unsigned char *pucFWUpdateFileData)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	memcpy(&ucCanData[0],pucFWUpdateFileData,sizeof(ucCanData));

	SetCanPacket(eCanFD_FWUpdate_DataCmd,ucCanData);
}

void CFD_SetFWUpdateTransmit2(unsigned char *pucFWUpdateFileData, unsigned char ucSize)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	memcpy(&ucCanData[0],pucFWUpdateFileData,sizeof(ucCanData));

	SetCanPacket_VariableSize(eCanFD_FWUpdate_DataCmd,ucCanData,ucSize);
}


void CFD_SetFWUpdateEnd(unsigned short usCheckSum)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
//	memcpy(&ucCanData[0],&usCheckSum,sizeof(unsigned short));
	
	ucCanData[0] = (usCheckSum>>8)&0xFF;
	ucCanData[1] = usCheckSum&0xFF;
	
	SetCanPacket_VariableSize(eCanFD_FWUpdate_EndCmd,ucCanData,2);
}

void CFD_Reset()
{
	SetCanPacket(eCanFD_Reset,NULL);
}

void CFD_Sleep()
{
	SetCanPacket(eCanFD_Sleep,NULL);
}

void CFD_WriteSerial(unsigned char *pucAdapterSerial)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	memcpy(&ucCanData,pucAdapterSerial,sizeof(ucCanData));
	
	SetCanPacket(eCanFD_Serial_Write,ucCanData);
}

void CFD_ReadSerial()
{
	SetCanPacket(eCanFD_Serial_Read,NULL);
}

void CFD_ReadBatteryVoltage()
{
	SetCanPacket(eCanFD_Battery_Read,NULL);
}

void CFD_JumpVerification()
{
	SetCanPacket(eCanFD_Jump_Verification,NULL);
}

void CFD_SetByPassFlag(unsigned char *pucByPassFlag)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	memcpy(&ucCanData,pucByPassFlag,sizeof(ucCanData));
	
	SetCanPacket(eCanFD_Set_ByPassFlag,ucCanData);
}

void CFD_SetByPassFlagAllOff()
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	SetCanPacket(eCanFD_Set_ByPassFlag,ucCanData);
	APP_Delay(100);
}


void CFD_SetByPassFlagAllOn()
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x01,sizeof(ucCanData));
	SetCanPacket(eCanFD_Set_ByPassFlag,ucCanData);

	CFD_RecvMultiDataClear();
}

void CFD_SetByPassFlagDirect(unsigned char *pucByPassFlag)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	memcpy(&ucCanData,pucByPassFlag,sizeof(ucCanData));
	
	stCanPacket OutCanPacket;
	uint32_t uiCanChannel = CAN_CHANNEL_1;
		
	OutCanPacket.stNormalPacket.ucSOF = CAN_FRAME_SOF;
	OutCanPacket.stNormalPacket.us11BitID = eCanFD_Set_ByPassFlag; 
	OutCanPacket.stNormalPacket.ucRTR = 0;
	OutCanPacket.stNormalPacket.ucIDE = CAN_FRAME_STANDARD_IDE;
	OutCanPacket.stNormalPacket.ucReserved = 0;
	OutCanPacket.stNormalPacket.ucDLC = 8;
	OutCanPacket.stNormalPacket.usCRC = 0;
	OutCanPacket.stNormalPacket.ucCRCDelimiter = 1;
	OutCanPacket.stNormalPacket.ucACK = 1;
	OutCanPacket.stNormalPacket.ucACKDelimiter = 1;
	OutCanPacket.stNormalPacket.ucEOF = CAN_FRAME_EOF;

	if(ucCanData != NULL)
	{
		memcpy(&OutCanPacket.stNormalPacket.arrDataFields[0],ucCanData,LENGTH_CANFRAME_DATA);	
	}

	if(FineWriteCanBuff((unsigned char*)&OutCanPacket, sizeof(OutCanPacket), NULL, uiCanChannel) > 0 ) 
	{
		DCAN_SET_COMM_STATE(eCAN_RX_BLOCK);
		CFD_SetCanFDAdapterControlMode();
	}
}


void CFD_GetByPassFlag()
{
	SetCanPacket(eCanFD_Get_ByPassFlag,NULL);
}

void CFD_SetTransceiverMode(unsigned char ucSpiIndex , unsigned char ucTxTransceiverSet)
{
	boolean_t bNeedSet = false;
	switch(ucSpiIndex)
	{
		case CANFD_SPI_1:
		case CANFD_SPI_2:
		case CANFD_SPI_3:		
			if(m_stCFDCtrl.stConfig.bTxTransceiverSet[ucSpiIndex%CANFD_SPI_1] != ucTxTransceiverSet)
			{
				m_stCFDCtrl.stConfig.bTxTransceiverSet[ucSpiIndex%CANFD_SPI_1] = ucTxTransceiverSet;
				bNeedSet = true;
			}
			break;
		case CANFD_SPI_4:
		case CANFD_SPI_5:
			if(m_stCFDCtrl.stConfig.bTxTransceiverSet[CANFD_SPI_4%CANFD_SPI_1] != ucTxTransceiverSet)
			{
				m_stCFDCtrl.stConfig.bTxTransceiverSet[CANFD_SPI_4%CANFD_SPI_1] = ucTxTransceiverSet;
				bNeedSet = true;				
			}			
			break;
	}

	if(bNeedSet)
	{
		CFD_SendTransceiverMode(m_stCFDCtrl.stConfig.bTxTransceiverSet);
	}
}

void CFD_SendTransceiverMode(unsigned char *pucTransceiverMode)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	memcpy(&ucCanData,pucTransceiverMode,sizeof(ucCanData));

	SetCanPacket(eCanFD_Set_Transceiver,ucCanData);
}

void CFD_SetLedControl(eCanFDLed eSelectLed , unsigned char ucLedOnOff)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	//memset(&ucCanData[0],pucTransceiverMode,sizeof(ucCanData));
	ucCanData[eCanFD_SelectLed] = eSelectLed;
	ucCanData[eCanFD_Control] = ucLedOnOff;

	SetCanPacket(eCanFD_Set_LedOnOff,ucCanData);
}

void CFD_SetCanBaudRate(unsigned char *pucCanBaudRate)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	memcpy(&ucCanData,pucCanBaudRate,sizeof(ucCanData));
	
	SetCanPacket(eCanFD_HW_Set_BaudRate,ucCanData);
}

/*
typedef __packed struct __stAdapterConfig{
	boolean_t bActiveMasking[MAX_CFD_CAN_LINE];
	boolean_t bActvieLine[MAX_CFD_CAN_LINE];			//Active 되어진 CanFD 설정 라인
	boolean_t bSendCanLine[MAX_CFD_CAN_LINE];			//CanFD -> Can Module의 Can Line 설정 ( Can1 / Can2 ) , Can1 : False , Can2 : True
	boolean_t bTxTransceiverSet[MAX_CFD_CAN_LINE];		//CanFD : True / Can : False 
	boolean_t bSelectedAnalogSwitch;					//CanFD SPI3번에 대한 Transceiver 에 대한 설정
	int8_t    ucActiveSwitchingLine;					//활성화 여부 
	int8_t    ucCanBaudRate[MAX_CFD_CAN_LINE];
}stAdapterConfig;


typedef __packed struct __stCFDControl{         
	stCFDStateControl 	stStateCtrl;
	stCFDVersion 		stVer;
	stAdapterConfig	 	stConfig;
	boolean_t 			bActive;		// FD Control 이 활성화가 되어져 있는지 ?
	boolean_t 			bFDControl;
}stCFDControl;

*/

//Oem_CAN_Initial_CH1
//CanFD_Initial_Channel
void CanFD_Initial_Channel(unsigned char ucCanFDLine,  unsigned char ucCanFDBps)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	m_stCFDCtrl.stConfig.ucCanBaudRate[ucCanFDLine % CANFD_SPI_DEFAULT] = ucCanFDBps;
	CFD_SetCanBaudRate((unsigned char*)m_stCFDCtrl.stConfig.ucCanBaudRate);

	memset(&ucCanData,0x00,sizeof(ucCanData));
	m_stCFDCtrl.stConfig.bActvieLine[ucCanFDLine % CANFD_SPI_DEFAULT] = true;
	CFD_SendCanLine(m_stCFDCtrl.stConfig.bActvieLine);
}

void CanFD_ChannelSet()
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	memcpy(&ucCanData,m_stCFDCtrl.stConfig.ucCanBaudRate,sizeof(ucCanData));
	CFD_SetCanBaudRate(ucCanData);

	memset(&ucCanData,0x00,sizeof(ucCanData));
	memcpy(&ucCanData,m_stCFDCtrl.stConfig.bSendCanLine,sizeof(ucCanData));	
	CFD_SendCanLine(ucCanData);
}

void CFD_GetCanBaudRate()
{
	SetCanPacket(eCanFD_HW_Get_BaudRate,NULL);
}


// Oem_CAN_Channel_Masket_Set 
void CFD_Channel_Masket_Set(unsigned char ucSpiNum,unsigned char ucMaskNum,unsigned char ucMaskType, unsigned int *pStartMaskValue, unsigned int *pEndMaskValue)
{
	unsigned int uistartMaskVal, uiendMaskVal;
	unsigned char arrCanData[LENGTH_CANFRAME_DATA];
	memset(&arrCanData,0x00,sizeof(arrCanData));
	int8_t ucCount  = 0;

	for(int i = 0 ; i < ucMaskNum; i++)
	{
		uistartMaskVal = pStartMaskValue[i];
		uiendMaskVal   = pEndMaskValue[i];
		
		if(uistartMaskVal == uiendMaskVal)
		{
			arrCanData[(i%CANMASKING_MAXCOUNT)*2]		= (uistartMaskVal>>8)&0xFF;
			arrCanData[((i%CANMASKING_MAXCOUNT)*2)+1]	= (uistartMaskVal)&0xFF;	
			ucCount++;
			
			if(ucCount == CANMASKING_MAXCOUNT)
			{
				CFD_SetCanMasking(ucSpiNum,ucMaskType,ucCount,arrCanData);
				memset(&arrCanData,0x00,sizeof(arrCanData));
				ucCount = 0;
			}
		}
	}

	if(ucCount !=0)
	{
		CFD_SetCanMasking(ucSpiNum,ucMaskType,ucCount,arrCanData);
	}
}

void CFD_Channel_Masket_Set_Manual(unsigned char ucSpiNum,unsigned char ucMaskNum,unsigned char ucMaskType, unsigned int *pStartMaskValue, unsigned int *pEndMaskValue,unsigned char *pLineValue)
{
	unsigned int uistartMaskVal, uiendMaskVal;
	unsigned char arrCanData[LENGTH_CANFRAME_DATA];
	memset(&arrCanData,0x00,sizeof(arrCanData));
	int8_t ucCount  = 0;


	for(int i = 0 ; i < ucMaskNum; i++)
	{
		uistartMaskVal = pStartMaskValue[i];
		uiendMaskVal   = pEndMaskValue[i];
		
		if(uistartMaskVal == uiendMaskVal && ((ucSpiNum == ((unsigned int)pLineValue%CANFD_SPI_DEFAULT)) || (unsigned int)pLineValue == CANFD_SPI_DEFAULT))
		{
			arrCanData[(i%CANMASKING_MAXCOUNT)*2]		= (uistartMaskVal>>8)&0xFF;
			arrCanData[((i%CANMASKING_MAXCOUNT)*2)+1]	= (uistartMaskVal)&0xFF;	
			ucCount++;
			
			if(ucCount == CANMASKING_MAXCOUNT)
			{
				CFD_SetCanMasking(ucSpiNum,ucMaskType,ucCount,arrCanData);
				memset(&arrCanData,0x00,sizeof(arrCanData));
				ucCount = 0;
			}
		}
	}

	if(ucCount !=0)
	{
		CFD_SetCanMasking(ucSpiNum,ucMaskType,ucCount,arrCanData);
	}
}



void CFD_SetCanMasking(unsigned char ucSpiNum,unsigned char ucMode,unsigned char ucCount , unsigned char* pucMaskingID)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	ucCanData[0] = ucSpiNum;
	ucCanData[1] = (ucMode<<4) | (ucCount & 0x0F);
	memcpy(&ucCanData[2],pucMaskingID,sizeof(unsigned char)*6);

//hexdump(ucCanData,8);
	SetCanPacket(eCanFD_HW_Set_Masking,ucCanData);
}

void CFD_GetCanMaskingInfo(unsigned char ucSpiNum)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	ucCanData[0] = ucSpiNum;

	SetCanPacket(eCanFD_HW_Get_MaskingInfo,ucCanData);
}

void CFD_SetClearCanMasking(unsigned char ucSpiNum)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	ucCanData[0] = ucSpiNum;
	
	SetCanPacket(eCanFD_HW_Set_MaskingClear,ucCanData);
}

void CFD_SetAnalogSwitch(unsigned char ucSelectSwitch)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	ucCanData[0] = ucSelectSwitch;

	SetCanPacket(eCanFD_HW_Set_AnalogSwitch,ucCanData);
}

void CFD_GetAnalogSwitch()
{
	SetCanPacket(eCanFD_HW_Get_AnalogSwitch,NULL);
}

void CFD_SetTxSpiNumber(unsigned char ucMainCanLine , unsigned char ucSpiNumber)
{
	boolean_t bCanLineChange = false;
	
	if(ucMainCanLine == CANMAIN_HIGHCAN2)
	{
		if(ucSpiNumber == CANFD_SPI_5)
		{
			if(m_stCFDCtrl.stConfig.bSendCanLine[4] != CANFD_SPI_4 % CANFD_SPI_DEFAULT)
			{
				m_stCFDCtrl.stConfig.bSendCanLine[4] = CANFD_SPI_4 % CANFD_SPI_DEFAULT;
				bCanLineChange = true;
			}
		}
		else
		{
			if(m_stCFDCtrl.stConfig.bSendCanLine[4] != (ucSpiNumber % CANFD_SPI_DEFAULT))
			{
				m_stCFDCtrl.stConfig.bSendCanLine[4] = ucSpiNumber % CANFD_SPI_DEFAULT;
				bCanLineChange = true;
			}
		}
	}

	if(ucMainCanLine == CANMAIN_HIGHCAN3)
	{
		if(ucSpiNumber == CANFD_SPI_5)
		{
			if(m_stCFDCtrl.stConfig.bSendCanLine[5] != CANFD_SPI_4 % CANFD_SPI_DEFAULT)
			{
				m_stCFDCtrl.stConfig.bSendCanLine[5] = CANFD_SPI_4 % CANFD_SPI_DEFAULT;
				bCanLineChange = true;
			}
			
		}
		else
		{
			if(m_stCFDCtrl.stConfig.bSendCanLine[5] != ucSpiNumber % CANFD_SPI_DEFAULT)
			{
				m_stCFDCtrl.stConfig.bSendCanLine[5] = ucSpiNumber % CANFD_SPI_DEFAULT;	
				bCanLineChange = true;
			}
			
		}
	}

	if(bCanLineChange)
		CFD_SendCanLine(m_stCFDCtrl.stConfig.bSendCanLine);
}

void CFD_SetCanLineMode(eCanLineMode eLineMode)
{
	switch(eLineMode)
	{
		case eCanFDLineMode_Default:
			m_stCFDCtrl.stConfig.bSendCanLine[CANFD_SPI1_ArrIdx] = 0;
			m_stCFDCtrl.stConfig.bSendCanLine[CANFD_SPI2_ArrIdx] = 0;
			m_stCFDCtrl.stConfig.bSendCanLine[CANFD_SPI3_ArrIdx] = 1;
			m_stCFDCtrl.stConfig.bSendCanLine[CANFD_SPI4_ArrIdx] = 1;
			break;
		case eCanFDLineMode_ALLCAN1:
			memset(&m_stCFDCtrl.stConfig.bSendCanLine,0x00,sizeof(boolean_t)*4);
				break;
		case eCanFDLineMode_ALLCAN2:
			memset(&m_stCFDCtrl.stConfig.bSendCanLine,0x01,sizeof(boolean_t)*4);
				break;			
	}

	CFD_SendCanLine(m_stCFDCtrl.stConfig.bSendCanLine);
}

void CFD_SendCanLine(unsigned char* pucCanLine)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	memcpy(&ucCanData[0],pucCanLine,sizeof(ucCanData));

	SetCanPacket(eCanFD_HW_Set_CanDataLine,ucCanData);
}

void CFD_GetCanLine()
{
	SetCanPacket(eCanFD_HW_Get_CanDataLine,NULL);
}

void CFD_SetSpiController()
{
	SetCanPacket(eCanFD_HW_Set_SpiController,NULL);
}

void CFD_DummyController()
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	ucCanData[0] = 1;
    SetCanPacket(eCanFD_DummyControl,ucCanData);
}

void CFD_SendMultiStartCmd(unsigned int nCanID ,unsigned char ucCanLength)
{	
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	ucCanData[0] = (nCanID >>8) & 0xFF;
	ucCanData[1] = nCanID & 0xFF;
	ucCanData[2] = ucCanLength;
	SetCanPacket(eCanFD_Multi_Start,ucCanData);
}

void CFD_SendMultiDataCmd(unsigned char* pucCanLine ,unsigned char ucCanDataSize)
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	memcpy(&ucCanData[0],pucCanLine,ucCanDataSize);
	SetCanPacket_VariableSize(eCanFD_Multi_Data,ucCanData,ucCanDataSize);
}

void CFD_SendMultiAllCmd(unsigned int nCanID,unsigned char ucCanLength)
{
	unsigned char ucFDCanDataDivide   = ucCanLength / 8;
	unsigned char ucFDCanDataReminder = ucCanLength % 8;

	CFD_SendMultiStartCmd(nCanID,ucCanLength);

	for(int nCanData = 0 ; nCanData < ucFDCanDataDivide; nCanData++)
	{
		//CFD_SendMultiDataCmd(&pucCanLine[8*nCanData],8);
	}

	if(ucFDCanDataReminder >  0)
	{
		//CFD_SendMultiDataCmd(&pucCanLine[(ucFDCanDataDivide*8)+1],ucFDCanDataReminder);
	}
}

void CFD_DisplayConfig()
{
	SetCanPacket(eCanFD_UTIL_DisplayConfig,NULL);
}

void CFD_DisplayLog()
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x01,sizeof(ucCanData));
	SetCanPacket(eCanFD_UTIL_ActiveLogData,ucCanData);
}

void CFD_DisplayLogOff()
{
	unsigned char ucCanData[LENGTH_CANFRAME_DATA];
	memset(&ucCanData,0x00,sizeof(ucCanData));
	SetCanPacket(eCanFD_UTIL_ActiveLogData,ucCanData);
}




















	









