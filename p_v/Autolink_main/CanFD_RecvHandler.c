/**
******************************************************************************
* @file    CanFD_RecvHandler.c
* @author  Cho Sang Jun
* @version V1
* @date    11-May-2020
* @brief   CanFD Receive Data Handler
******************************************************************************
**/



/* CanFD_RecvHandler.c */

#include "CanFD_RecvHandler.h"
#include "CanFD_Defines.h"
#include "OBD_Manager.h"
#include "GIT_InterProtocol.h"
#include "GIT_CanParsingProc.h"
#include "CanFD_Manager.h"
#include "HdDebug.h"
#include "GIT_Util.h"

#define Trace(...)	GITDebug(DEBUG_MODULES_OBD_CANFD,__VA_ARGS__);



extern stCFDControl m_stCFDCtrl;
extern stCanIDCheckList g_stCanIDCheckList;

extern void CFD_SetCanFDSleepStatus(boolean_t bSleepStatus);


void CFD_RecvMultiDataCmd731(unsigned char* pCanData);
void CFD_RecvMultiDataCmd732(unsigned char* pCanData);
void CFD_RecvMultiDataCmd733(unsigned char* pCanData);
void CFD_RecvMultiDataCmd734(unsigned char* pCanData);
void CFD_RecvMultiDataCmd735(unsigned char* pCanData);
void CFD_RecvMultiDataCmd736(unsigned char* pCanData);
void CFD_RecvMultiDataCmd737(unsigned char* pCanData);
void CFD_RecvMultiDataCmd738(unsigned char* pCanData);


typedef struct __stFunctionIndexConvert{
	uint16_t usFuncIndex;
	eInitCanFDSequence eCanRequest;
	eInitCanFDSequence eCanResponse;
	fnpEventHanderList fnpProcess;	
}stFunctionIndexConvert;

stFunctionIndexConvert m_stFunctionIndexConvert[] = 
{
    /*
    SetCanFDRequest(eFDDummySignalRsp);
    */
	{0x0700,eFDGetVersionReq,eFDGetVersionRsp,CFD_RecvGetVersionData},
	{0x0702,eFDUpdateStartReq,eFDUpdateStartRsp,NULL},
	{0x0703,eFDUpdateTransmitReq,eFDUpdateTransmitRsp,CFD_RecvGetTransmitResponse},
	{0x0704,eFDUpdateEndReq,eFDUpdateEndRsp,CFD_RecvGetEndResponse},
	{0x0705,eFDUpdateAfterResetReq,eFDUpdateAfterResetRsp,NULL},
	{0x0706,eFDSetSleepReq,eFDSetSleepRsp,CFD_RecvSleepResponse},
	{0x070B,eFDSetCanByPassModeReq,eFDSetCanByPassModeRsp,CFD_RecvByPassResponse},	
	{0x0723,eFDSetCanBaudRateReq,eFDSetCanBaudRateRsp,NULL},	
	{0x0724,eFDGetCanBaudRateReq,eFDGetCanBaudRateRsp,CFD_RecvGetBaudRateData},	
	{0x0725,eFDSetMaskingInfoReq,eFDSetMaskingInfoRsp,NULL},	
	{0x0726,eFDGetMaskingInfoReq,eFDGetMaskingInfoRsp,CFD_RecvGetMaskingData},
	{0x0727,eFDSetMaskingClearReq,eFDSetMaskingClearRsp,NULL},
	{0x0728,eFDSetAnalogSwitchReq,eFDSetAnalogSwitchRsp,NULL},
	{0x0729,eFDGetAnalogSwitchReq,eFDGetAnalogSwitchRsp,CFD_RecvGetAnalogSwitch},
	{0x072A,eFDSetCanLineReq,eFDSetCanLineRsp,CFD_RecvSetCanLineData},					
	{0x072B,eFDGetCanLineReq,eFDGetCanLineRsp,CFD_RecvGetCanLineData},		
	{0x0730,eFDSetMultiDataControlReq,eFDSetMultiDataControlRsp,CFD_RecvMultiStartCmd},
	{0x0731,eFDSetMultiDataReq,eFDSetMultiDataRsp,CFD_RecvMultiDataCmd731},
	{0x0732,eFDSetMultiDataReq,eFDSetMultiDataRsp,CFD_RecvMultiDataCmd732},
	{0x0733,eFDSetMultiDataReq,eFDSetMultiDataRsp,CFD_RecvMultiDataCmd733},
	{0x0734,eFDSetMultiDataReq,eFDSetMultiDataRsp,CFD_RecvMultiDataCmd734},
	{0x0735,eFDSetMultiDataReq,eFDSetMultiDataRsp,CFD_RecvMultiDataCmd735},
	{0x0736,eFDSetMultiDataReq,eFDSetMultiDataRsp,CFD_RecvMultiDataCmd736},
	{0x0737,eFDSetMultiDataReq,eFDSetMultiDataRsp,CFD_RecvMultiDataCmd737},
	{0x0738,eFDSetMultiDataReq,eFDSetMultiDataRsp,CFD_RecvMultiDataCmd738},
	{0x0712,eFDNotifyCanFDWakeUp,eFDNotifyCanFDWakeUp,CFD_RecvNotifyWakeUpSignal},
	{0x0750,eFDLogShowReq,eFDLogShowRsp,NULL},
	{0x07FF,eFDNotifyCanFDWakeUp,eFDNotifyCanFDWakeUp,CFD_RecvUnKnownMsg},
};

void FindCanFDResponse(eCanFDTxCmd eTxCmd , unsigned char* pCanData)
{
	/*
	if(eTxCmd == 0x0710 || eTxCmd == 0x0711)
	{
		Trace(" ---- Cmd :  %x -----\n",eTxCmd);
		hexdump(pCanData,8);
	}
	*/
	
	for(int i=0;i<(sizeof(m_stFunctionIndexConvert)/sizeof(stFunctionIndexConvert));i++)
	{
		if( m_stFunctionIndexConvert[i].usFuncIndex == eTxCmd )
		{
			if( m_stFunctionIndexConvert[i].fnpProcess != NULL )
			{
				m_stFunctionIndexConvert[i].fnpProcess(pCanData);
			}

			//if(m_stFunctionIndexConvert[i].eCanResponse != 0x0710 && m_stFunctionIndexConvert[i].eCanResponse != 0x0711)
			if(GetCanFDRequest() == m_stFunctionIndexConvert[i].eCanResponse)
				SetCanFDResponse(m_stFunctionIndexConvert[i].eCanResponse);
			break;
		}
	}
}


void SetCanFDResponse(eInitCanFDSequence eCanFDRequest)
{
	m_stCFDCtrl.stStateCtrl.eCanFdResponse = eCanFDRequest;
}

void CFD_RecvGetVersionData(unsigned char* pCanData)
{
	m_stCFDCtrl.stVer.usBlVer = pCanData[1]|(pCanData[0]<<8);
	m_stCFDCtrl.stVer.usAppVer = pCanData[3]|(pCanData[2]<<8);
	m_stCFDCtrl.stVer.usVerfVer = pCanData[5]|(pCanData[4]<<8);
	m_stCFDCtrl.stVer.usBUVer = pCanData[7]|(pCanData[6]<<8);
}

void CFD_RecvGetTransmitResponse(unsigned char* pCanData)
{
  	m_stCFDCtrl.eFotaResult = (eCanFDFotaResult)pCanData[0];
}

void CFD_RecvGetEndResponse(unsigned char* pCanData)
{
  	m_stCFDCtrl.eFotaResult = (eCanFDFotaResult)pCanData[0];
}

void CFD_RecvGetBaudRateData(unsigned char* pCanData)
{
	memcpy(&m_stCFDCtrl.stConfig.ucCanBaudRate,pCanData,sizeof(int8_t)*4);
}



void CFD_RecvGetAnalogSwitch(unsigned char* pCanData)
{
    if(pCanData[0] == 0)       
    {
         //CanFD 4 Active
         m_stCFDCtrl.stConfig.ucActiveSwitchingLine = CANFD_ANALOG_SWITCH_FD4;
    }
    else //if(pCanData[0] == 1)       
    {
         //CanFD 5 Active           
         m_stCFDCtrl.stConfig.ucActiveSwitchingLine = CANFD_ANALOG_SWITCH_FD5;         
    }
}


void CFD_RecvGetMaskingData(unsigned char* pCanData)
{
	Trace("CFD_RecvGetMaskingData Start\r\n");
		
	stMaskingInfo stTempRecvMaskinfo;
	if( m_stCFDCtrl.stVer.usAppVer >= 0x207 )
	{
	    if(pCanData[0] == 3)        //SPI Number 3 에 대한 정의
	    {
	        m_stCFDCtrl.stMask.ucSpiMaskCertify[eCanFD_Number_3] = true;
	    }
	    else if((pCanData[0] >= 1) && (pCanData[0] <= 5))
		{
			stTempRecvMaskinfo.ucCanFdSpiNum = pCanData[0];
			if(pCanData[1] == CANMASKING_SW)
			{
				stTempRecvMaskinfo.stSwSector.ucMaskingCount 	= pCanData[2];
				stTempRecvMaskinfo.stSwSector.ucMaskingCheckSum = pCanData[3];
				if(m_stCFDCtrl.stCanIDMerge.ucMaskCount >= CANMASKING_HW_MAXCOUNT)
				{
					if((GetMaskingCountForDB() == stTempRecvMaskinfo.stSwSector.ucMaskingCount) &&
					   (GetMaskingCheckSumForDB() == stTempRecvMaskinfo.stSwSector.ucMaskingCheckSum))
					{
						Trace("[%s] CANMASKING_SW Certify Success \r\n",__FUNCTION__);
						m_stCFDCtrl.stMask.ucSpiMaskCertify[stTempRecvMaskinfo.ucCanFdSpiNum-1] = true;
					}
					else
					{
						Trace("[%s] CANMASKING_SW Certify Fail \r\n",__FUNCTION__);
						m_stCFDCtrl.stMask.ucSpiMaskCertify[stTempRecvMaskinfo.ucCanFdSpiNum-1] = false;
						Trace("Fail Result1 GetCount %d , RecvCount %d \r\n",GetMaskingCountForDB(),stTempRecvMaskinfo.stSwSector.ucMaskingCount);				
						Trace("Fail Result2 GetCheckSum %d , RecvCheckSum %d \r\n",GetMaskingCheckSumForDB(),stTempRecvMaskinfo.stSwSector.ucMaskingCheckSum);				
						//hexdump(pCanData,8);
					}				
				}
			}
			if(pCanData[4] == CANMASKING_HW)
			{
				stTempRecvMaskinfo.stHwSector.ucMaskingCount 	= pCanData[5];
				stTempRecvMaskinfo.stHwSector.ucMaskingCheckSum = pCanData[6];
				if(m_stCFDCtrl.stCanIDMerge.ucMaskCount < CANMASKING_HW_MAXCOUNT)
				{
					if((GetMaskingCountForDB() == stTempRecvMaskinfo.stHwSector.ucMaskingCount) &&
					   (GetMaskingCheckSumForDB() == stTempRecvMaskinfo.stHwSector.ucMaskingCheckSum))
					{
						Trace("[%s] Certify Success \r\n",__FUNCTION__);
						m_stCFDCtrl.stMask.ucSpiMaskCertify[stTempRecvMaskinfo.ucCanFdSpiNum-1] = true;
					}
					else
					{
						Trace("[%s] Certify Fail \r\n",__FUNCTION__);
						//hexdump(pCanData,8);
						m_stCFDCtrl.stMask.ucSpiMaskCertify[stTempRecvMaskinfo.ucCanFdSpiNum-1] = false;
					}
				}
			}
		}
	}
	else
	{
		if(pCanData[0] == 3)        //SPI Number 3 에 대한 정의
		{
			m_stCFDCtrl.stMask.ucSpiMaskCertify[eCanFD_Number_3] = true;
	    }
	    else if((pCanData[0] >= 1) && (pCanData[0] <= 5))
		{
		stTempRecvMaskinfo.ucCanFdSpiNum = pCanData[0];
	
		if(pCanData[1] == CANMASKING_SW)
		{
			stTempRecvMaskinfo.stSwSector.ucMaskingCount 	= pCanData[2];
			stTempRecvMaskinfo.stSwSector.ucMaskingCheckSum = pCanData[3];
			if(m_stCFDCtrl.stCanIDMerge.ucMaskCount >= CANMASKING_HW_MAXCOUNT)
			{
				if((GetMaskingCountForDB() == stTempRecvMaskinfo.stSwSector.ucMaskingCount) &&
				   (GetMaskingCheckSumForDB() == stTempRecvMaskinfo.stSwSector.ucMaskingCheckSum))
				{
					Trace("[%s] CANMASKING_SW Certify Success \r\n",__FUNCTION__);
					m_stCFDCtrl.stMask.ucSpiMaskCertify[stTempRecvMaskinfo.ucCanFdSpiNum-1] = true;
				}
				else
				{
					printf("[%s] CANMASKING_SW Certify Fail \r\n",__FUNCTION__);				
					m_stCFDCtrl.stMask.ucSpiMaskCertify[stTempRecvMaskinfo.ucCanFdSpiNum-1] = false;
					printf("Fail Result1 GetCount %d , RecvCount %d \r\n",GetMaskingCountForDB(),stTempRecvMaskinfo.stSwSector.ucMaskingCount);				
					printf("Fail Result2 GetCheckSum %d , RecvCheckSum %d \r\n",GetMaskingCheckSumForDB(),stTempRecvMaskinfo.stSwSector.ucMaskingCheckSum);
					//hexdump(pCanData,8);
				}				
			}
		}
		else if(pCanData[1] == CANMASKING_HW)
		{
			stTempRecvMaskinfo.stHwSector.ucMaskingCount 	= pCanData[2];
			stTempRecvMaskinfo.stHwSector.ucMaskingCheckSum = pCanData[3];
			
			if(m_stCFDCtrl.stCanIDMerge.ucMaskCount < CANMASKING_HW_MAXCOUNT)
			{
				if((GetMaskingCountForDB() == stTempRecvMaskinfo.stHwSector.ucMaskingCount) &&
				   (GetMaskingCheckSumForDB() == stTempRecvMaskinfo.stHwSector.ucMaskingCheckSum))
				{
					//Certify Success
					printf("[%s] Certify Success \r\n",__FUNCTION__);
					m_stCFDCtrl.stMask.ucSpiMaskCertify[stTempRecvMaskinfo.ucCanFdSpiNum-1] = true;
				}
				else
				{
					//Certify Fail				
					printf("[%s] Certify Fail \r\n",__FUNCTION__);
					//hexdump(pCanData,8);
					m_stCFDCtrl.stMask.ucSpiMaskCertify[stTempRecvMaskinfo.ucCanFdSpiNum-1] = false;
				}
			}
		}

		// 기존 Struct Compare Process And Certify Process
		}
	}
	
	Trace("CFD_RecvGetMaskingData End\r\n");
}

int8_t GetMaskingCountForDB()
{
	//return (int8_t)g_stCanIDCheckList.m_uiMaskCount;
	return m_stCFDCtrl.stCanIDMerge.ucMaskCount;
}

int8_t GetMaskingCheckSumForDB()
{
	/*
	int8_t ucTempCheckSum = 0;
	
	for(int i = 0 ; i < GetMaskingCountForDB(); i++)
	{
		ucTempCheckSum += g_stCanIDCheckList.m_uiStartMask[i];
	}
	
	return ucTempCheckSum;
	*/

	
	int8_t ucTempCheckSum = 0;
	
	for(int i = 0 ; i < GetMaskingCountForDB(); i++)
	{
		ucTempCheckSum += m_stCFDCtrl.stCanIDMerge.nStartMask[i];
	}
	
	return ucTempCheckSum;


	
}

void CFD_RecvGetCanLineData(unsigned char* pCanData)
{
	memcpy(&m_stCFDCtrl.stConfig.bSendCanLine,pCanData,sizeof(int8_t)*6);
}

typedef struct __stCFDMuldtiDataControl
{
	uint8_t ucChannel;
	uint32_t unCanId;
	uint8_t ucDataSize;
	uint8_t ucDataCount;
	uint8_t ucCurDataCount;
	uint32_t ucStartCmdCount;	
	uint32_t ucSumCmdCount;		
	uint8_t ucData[64];
	uint16_t usCheckSum;
}stCFDMuldtiDataControl;

stCFDMuldtiDataControl m_stCFDMultiDataCtrl;



void CFD_RecvMultiStartCmd(unsigned char* pCanData)
{
	m_stCFDMultiDataCtrl.unCanId = pCanData[0]<<8|pCanData[1];
	m_stCFDMultiDataCtrl.ucDataSize   = pCanData[2];
	m_stCFDMultiDataCtrl.ucChannel = pCanData[3];

	if(pCanData[2] % 8 == 0)
		m_stCFDMultiDataCtrl.ucDataCount = pCanData[2] / 8;
	else
		m_stCFDMultiDataCtrl.ucDataCount = (pCanData[2] / 8)+1;
	
	m_stCFDMultiDataCtrl.ucCurDataCount   = 0;
	memset(&m_stCFDMultiDataCtrl.ucData,0x00,sizeof(m_stCFDMultiDataCtrl.ucData));
	m_stCFDMultiDataCtrl.ucStartCmdCount++;
	m_stCFDMultiDataCtrl.usCheckSum = pCanData[4]<<8 | pCanData[5];
}

void CFD_RecvMultiDataCmd(unsigned char* pCanData)
{
	memcpy(&m_stCFDMultiDataCtrl.ucData[m_stCFDMultiDataCtrl.ucCurDataCount++*8],pCanData,8);

	//Trace("Sum multi can data Curr : %d All : %d\n",m_stCFDMultiDataCtrl.ucCurDataCount,m_stCFDMultiDataCtrl.ucDataCount);
	//hexdump(pCanData,8);

	if( m_stCFDMultiDataCtrl.ucCurDataCount == m_stCFDMultiDataCtrl.ucDataCount )
	{

		//if(m_stCFDMultiDataCtrl.unCanId == 0x0100 || m_stCFDMultiDataCtrl.unCanId == 0x00B5)
		//if(m_stCFDMultiDataCtrl.unCanId == 0x00B5)
		{	
			//Trace("Data === m_stCFDMultiDataCtrl.ucChannel:%d\r\n",m_stCFDMultiDataCtrl.ucChannel);
			//Trace("Sum Clear multi can data id : %x , count : %d Start : %d , Sum : %d \n",m_stCFDMultiDataCtrl.unCanId,m_stCFDMultiDataCtrl.ucDataCount,m_stCFDMultiDataCtrl.ucStartCmdCount,m_stCFDMultiDataCtrl.ucSumCmdCount);
			//hexdump(m_stCFDMultiDataCtrl.ucData,m_stCFDMultiDataCtrl.ucCurDataCount*8);
		}
		
		CFD_PassThruReadMsgs();
	}
}

void CFD_PassThruReadMsgs()
{
	m_stCFDMultiDataCtrl.ucSumCmdCount++;

	//Trace("Sum Clear multi can data id : %x , count : %d Start : %d , Sum : %d \n",m_stCFDMultiDataCtrl.unCanId,m_stCFDMultiDataCtrl.ucDataCount,m_stCFDMultiDataCtrl.ucStartCmdCount,m_stCFDMultiDataCtrl.ucSumCmdCount);
	//hexdump(m_stCFDMultiDataCtrl.ucData,m_stCFDMultiDataCtrl.ucCurDataCount*8);
		
	if(m_stCFDMultiDataCtrl.usCheckSum == CalcChecksum_Short(m_stCFDMultiDataCtrl.ucData,m_stCFDMultiDataCtrl.ucDataSize) && m_stCFDMultiDataCtrl.ucDataSize > 0)
	{
		//Trace("[%s] \n",__FUNCTION__);
		
		unsigned int nMsgLength = 0;
		stPASSTHRU_MSG stReadMsg;
		nMsgLength += 2;
		nMsgLength += m_stCFDMultiDataCtrl.ucDataSize;
		//stReadMsg.pData[0] = (m_stCFDMultiDataCtrl.unCanId>>8) & 0xFF;
		memset(&stReadMsg.pData[0],(m_stCFDMultiDataCtrl.unCanId>>8) & 0xFF,sizeof(unsigned char));
		//stReadMsg.pData[1] = m_stCFDMultiDataCtrl.unCanId & 0xFF;
		memset(&stReadMsg.pData[1],m_stCFDMultiDataCtrl.unCanId & 0xFF,sizeof(unsigned char));
		memcpy(&stReadMsg.pData[2],m_stCFDMultiDataCtrl.ucData,m_stCFDMultiDataCtrl.ucDataSize);
		
		stReadMsg.RxStatus = CANFD_SPI_DEFAULT + m_stCFDMultiDataCtrl.ucChannel;
			

		if( m_stCFDMultiDataCtrl.ucChannel == 1 || m_stCFDMultiDataCtrl.ucChannel == 2 )
		{
			PassThruReadMsgs(&stReadMsg, nMsgLength, eCOMM_TYPE_CAN1, true);
		}
		else
		{
			Git_LCANReadMsgs(&stReadMsg, nMsgLength, eCOMM_TYPE_CAN2, true);
			//LCAN_SET_COMM_STATE(eLCAN_RX_DONE);
		}
	}
	else
	{
		//Trace("CR \n");
		//hexdump(m_stCFDMultiDataCtrl.ucData,m_stCFDMultiDataCtrl.ucCurDataCount*8);
		
		//Trace("Error [%s] \n",__FUNCTION__);
		//Trace("checksum : %x, cal checksum : %x\n", m_stCFDMultiDataCtrl.usCheckSum, CalcChecksum_Short(m_stCFDMultiDataCtrl.ucData,m_stCFDMultiDataCtrl.ucDataSize));
	}
}



void CFD_RecvMultiDataCmd731(unsigned char* pCanData)
{
	if( m_stCFDMultiDataCtrl.ucCurDataCount != 0 )
	{
		//Trace("1_R\n");
		// clear multi data info
		CFD_RecvMultiDataClear();
		return;
	}

	//Trace("731 \n");
	
	//memcpy(&m_stCFDMultiDataCtrl.ucData[m_stCFDMultiDataCtrl.ucCurDataCount++*8],pCanData,8);
	memcpy(&m_stCFDMultiDataCtrl.ucData[0],pCanData,8);
	m_stCFDMultiDataCtrl.ucCurDataCount++;
	if( m_stCFDMultiDataCtrl.ucCurDataCount == m_stCFDMultiDataCtrl.ucDataCount )
	{
		CFD_PassThruReadMsgs();
	}
}

void CFD_RecvMultiDataCmd732(unsigned char* pCanData)
{
	if( m_stCFDMultiDataCtrl.ucCurDataCount != 1 )
	{
		//Trace("2_R\n");
		// clear multi data info
		CFD_RecvMultiDataClear();
		return;
	}

	//Trace("732 \n");
	
	//memcpy(&m_stCFDMultiDataCtrl.ucData[m_stCFDMultiDataCtrl.ucCurDataCount++*8],pCanData,8);
	memcpy(&m_stCFDMultiDataCtrl.ucData[1*8],pCanData,8);
	m_stCFDMultiDataCtrl.ucCurDataCount++;
	if( m_stCFDMultiDataCtrl.ucCurDataCount == m_stCFDMultiDataCtrl.ucDataCount )
	{
		CFD_PassThruReadMsgs();
	}
}

void CFD_RecvMultiDataCmd733(unsigned char* pCanData)
{
	if( m_stCFDMultiDataCtrl.ucCurDataCount != 2 )
	{
		//Trace("3_R\n");
		// clear multi data info
		CFD_RecvMultiDataClear();
		return;
	}

	//Trace("733 \n");

	//memcpy(&m_stCFDMultiDataCtrl.ucData[m_stCFDMultiDataCtrl.ucCurDataCount++*8],pCanData,8);
	memcpy(&m_stCFDMultiDataCtrl.ucData[2*8],pCanData,8);
	m_stCFDMultiDataCtrl.ucCurDataCount++;
	if( m_stCFDMultiDataCtrl.ucCurDataCount == m_stCFDMultiDataCtrl.ucDataCount )
	{
		CFD_PassThruReadMsgs();
	}
}


void CFD_RecvMultiDataCmd734(unsigned char* pCanData)
{
	if( m_stCFDMultiDataCtrl.ucCurDataCount != 3 )
	{
		//Trace("4_R\n");
		// clear multi data info
		CFD_RecvMultiDataClear();
		return;
	}

	//Trace("734 \n");

	//memcpy(&m_stCFDMultiDataCtrl.ucData[m_stCFDMultiDataCtrl.ucCurDataCount++*8],pCanData,8);
	memcpy(&m_stCFDMultiDataCtrl.ucData[3*8],pCanData,8);
	m_stCFDMultiDataCtrl.ucCurDataCount++;

	if( m_stCFDMultiDataCtrl.ucCurDataCount == m_stCFDMultiDataCtrl.ucDataCount )
	{
		CFD_PassThruReadMsgs();
	}
}

void CFD_RecvMultiDataCmd735(unsigned char* pCanData)
{
	if( m_stCFDMultiDataCtrl.ucCurDataCount != 4 )
	{
		//Trace("5_R\n");
		// clear multi data info
		CFD_RecvMultiDataClear();
		return;
	}

	//Trace("735 \n");
	
	//memcpy(&m_stCFDMultiDataCtrl.ucData[m_stCFDMultiDataCtrl.ucCurDataCount++*8],pCanData,8);
	memcpy(&m_stCFDMultiDataCtrl.ucData[4*8],pCanData,8);
	m_stCFDMultiDataCtrl.ucCurDataCount++;

	
	if( m_stCFDMultiDataCtrl.ucCurDataCount == m_stCFDMultiDataCtrl.ucDataCount )
	{
		CFD_PassThruReadMsgs();
	}
}

void CFD_RecvMultiDataCmd736(unsigned char* pCanData)
{
	if( m_stCFDMultiDataCtrl.ucCurDataCount != 5 )
	{
		//Trace("6_R\n");
		// clear multi data info
		CFD_RecvMultiDataClear();
		return;
	}

	//Trace("736 \n");
	
	//memcpy(&m_stCFDMultiDataCtrl.ucData[m_stCFDMultiDataCtrl.ucCurDataCount++*8],pCanData,8);
	memcpy(&m_stCFDMultiDataCtrl.ucData[5*8],pCanData,8);
	m_stCFDMultiDataCtrl.ucCurDataCount++;
	
	if( m_stCFDMultiDataCtrl.ucCurDataCount == m_stCFDMultiDataCtrl.ucDataCount )
	{
		CFD_PassThruReadMsgs();
	}
}

void CFD_RecvMultiDataCmd737(unsigned char* pCanData)
{
	if( m_stCFDMultiDataCtrl.ucCurDataCount != 6 )
	{
		//Trace("7_R\n");
		// clear multi data info
		CFD_RecvMultiDataClear();
		return;
	}
	
	//Trace("737 \n");
	
	//memcpy(&m_stCFDMultiDataCtrl.ucData[m_stCFDMultiDataCtrl.ucCurDataCount++*8],pCanData,8);
	memcpy(&m_stCFDMultiDataCtrl.ucData[6*8],pCanData,8);
	m_stCFDMultiDataCtrl.ucCurDataCount++;

	
	if( m_stCFDMultiDataCtrl.ucCurDataCount == m_stCFDMultiDataCtrl.ucDataCount )
	{
		CFD_PassThruReadMsgs();
	}
}

void CFD_RecvMultiDataCmd738(unsigned char* pCanData)
{
	if( m_stCFDMultiDataCtrl.ucCurDataCount != 7)
	{
		//Trace("8_R\n");
		// clear multi data info
		CFD_RecvMultiDataClear();
		return;
	}

	//Trace("738 \n");
	
	//memcpy(&m_stCFDMultiDataCtrl.ucData[m_stCFDMultiDataCtrl.ucCurDataCount++*8],pCanData,8);
	memcpy(&m_stCFDMultiDataCtrl.ucData[7*8],pCanData,8);
	m_stCFDMultiDataCtrl.ucCurDataCount++;
	
	if( m_stCFDMultiDataCtrl.ucCurDataCount == m_stCFDMultiDataCtrl.ucDataCount )
	{
		CFD_PassThruReadMsgs();
	}
}

void CFD_RecvMultiDataClear()
{
	//memset(&m_stCFDMultiDataCtrl,0x00,sizeof(m_stCFDMultiDataCtrl));

	m_stCFDMultiDataCtrl.ucDataSize  = 0;
	m_stCFDMultiDataCtrl.ucDataCount = 0;
	m_stCFDMultiDataCtrl.unCanId = 0;
	m_stCFDMultiDataCtrl.ucCurDataCount = 0;
}

void CFD_ShowMultiData()
{
	//hexdump(m_stCFDMultiDataCtrl.ucData,m_stCFDMultiDataCtrl.ucCurDataCount*8);
}

void CFD_RecvUnKnownMsg(unsigned char* pCanData)
{
	//Trace("[%s] \n",__FUNCTION__);
    //hexdump(pCanData,8);
}


void CFD_RecvNotifyWakeUpSignal(unsigned char* pCanData)
{
    if(pCanData[0] == 0)   
    {
        printf(" [%s] Cold Booting \r\n",__FUNCTION__);
		memset(&m_stCFDCtrl.stConfig.bTxTransceiverSet,0x00,sizeof(m_stCFDCtrl.stConfig.bTxTransceiverSet));
    }
    else 
        printf(" [%s] Warm Booting \r\n",__FUNCTION__);

	if( GetPrevState() == eFDUpdateAfterResetReq )
	{
	  	SetCurrentState(eFDUpdateAfterResetRsp);
	}
	else if(GetOBDState() == eOBD_Running_Info_Mode)
	{
		SetOBDState(eOBD_InitCanFD);
		SetCanFDInitHandler();
	}

    m_stCFDCtrl.bActive = true;
}

void CFD_RecvByPassResponse(unsigned char* pCanData)
{
  	printf(" [%s] ByPass Response \r\n",__FUNCTION__);
	m_stCFDCtrl.bCanFDByPassOffStatus = 1;
}

void CFD_RecvSleepResponse(unsigned char* pCanData)
{
	if(m_stCFDCtrl.stVer.usAppVer >= CANFD_SLEEP_RESPONSE_VERSION)
	{
		printf(" [%s] Sleep Response : %d \r\n",__FUNCTION__,pCanData[0]);
		CFD_SetCanFDSleepStatus(pCanData[0]);
	}else
	{
  		printf(" [%s] Sleep Response \n",__FUNCTION__);
	}
}

void CFD_RecvSetCanLineData(unsigned char* pCanData)
{
  	printf(" [%s] Can Data Line Response \r\n",__FUNCTION__);
}





