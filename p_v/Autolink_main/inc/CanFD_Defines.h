/* CANFD Controller */
#ifndef __CANFD_DEFINES_H__
#define __CANFD_DEFINES_H__

#include "common.h"


#define CANFD_ANALOG_SWITCH_FD4		0
#define CANFD_ANALOG_SWITCH_FD5		1


#define CANMAIN_HIGHCAN2	202
#define CANMAIN_HIGHCAN3	203


#define CANFD_SPI_DEFAULT	220


#define CANFD_SPI1_ArrIdx	0
#define CANFD_SPI2_ArrIdx	1
#define CANFD_SPI3_ArrIdx	2
#define CANFD_SPI4_ArrIdx	3
#define CANFD_SPI5_ArrIdx	3






#define CANFD_SPI_1			221
#define CANFD_SPI_2			222
#define CANFD_SPI_3			223
#define CANFD_SPI_4			224
#define CANFD_SPI_5			225



//////////////////////////////////////////////////
//#define 정의
#define MAX_CFD_BUFFER		8
#define MAX_CFD_CAN_LINE	5
#define MAX_SPI_COUNT   	4

#define LENGTH_CANFRAME_DATA	8

#define CANMASKING_SW			0
#define CANMASKING_HW			1
#define CANMASKING_HW_MAXCOUNT	30


#define CANMASKING_MAXCOUNT		3


#define CANFD_CANMASKING_RESPONSE_VERSION   0x207
#define CANFD_SLEEP_RESPONSE_VERSION 0x20B
#define CANFD_BYPASS_OFF_LINE_SET_VERSION 0x20C

#define CFD_ARTERY_MCU_VERSION              0x5000


typedef enum _eCanFD_STATE
{
	eCanFD_NONE,
	eCanFD_INIT,
	eCanFD_FOTA,	
	eCanFD_VARIFYMASK,
	eCanFD_MASK,
	eCanFD_COMPLETE,
	eCanFD_FINISH,
	eCanFD_NORSP,
	eCanFD_SLEEP,
	eCanFD_State_Max,
}eCanFD_STATE;	

typedef enum __eCanFDInitType{
    eCanFDInit_Waiting	= 0,
	eCanFDInit_Dummy,
	eCanFDInit_Max
}eCanFDInitType;

typedef enum __eCanLineMode{
    eCanFDLineMode_Default=0,
    eCanFDLineMode_ALLCAN1,
	eCanFDLineMode_ALLCAN2,
	eCanFDLineMode_Max
}eCanLineMode;

//////////////////////////////////////////////////
//eNum

typedef enum __eCanFDTxCmd{
    eCanFD_GetVerion 		 	= 0x0700,
    eCanFD_SetVerion 		 	= 0x0701,		
    eCanFD_FWUpdate_StartCmd 	= 0x0702,
    eCanFD_FWUpdate_DataCmd  	= 0x0703,
    eCanFD_FWUpdate_EndCmd 	 	= 0x0704,
    eCanFD_Reset 			 	= 0x0705,
    eCanFD_Sleep 			 	= 0x0706,
    eCanFD_Serial_Write 	 	= 0x0707,    
    eCanFD_Serial_Read 		 	= 0x0708,
    eCanFD_Battery_Read 	 	= 0x0709,
    eCanFD_Jump_Verification 	= 0x070A,
    eCanFD_Set_ByPassFlag 	 	= 0x070B,
    eCanFD_Get_ByPassFlag 	 	= 0x070C,    
    eCanFD_Set_Transceiver 	 	= 0x070D,
    eCanFD_Set_LedOnOff 	 	= 0x070E,
    eCanFD_Multi_Start	 	 	= 0x0710,
    eCanFD_Multi_Data	 	 	= 0x0711,
    eCanFD_CommandPortChange	= 0x0714,
    eCanFD_HW_Channel	 	 	= 0x0721,
    eCanFD_HW_Resistance 	 	= 0x0722,
    eCanFD_HW_Set_BaudRate 	 	= 0x0723,
    eCanFD_HW_Get_BaudRate 	 	= 0x0724,    
    eCanFD_HW_Set_Masking 	 	= 0x0725,
    eCanFD_HW_Get_MaskingInfo	= 0x0726,
    eCanFD_HW_Set_MaskingClear 	= 0x0727,
    eCanFD_HW_Set_AnalogSwitch	= 0x0728,
    eCanFD_HW_Get_AnalogSwitch	= 0x0729,
    eCanFD_HW_Set_CanDataLine	= 0x072A,
    eCanFD_HW_Get_CanDataLine	= 0x072B,    
    eCanFD_HW_Set_SpiController	= 0x0740,  
    eCanFD_UTIL_DisplayConfig	= 0x0741,  
    eCanFD_UTIL_ActiveLogData	= 0x0750,      
    eCanFD_DummyControl        	= 0x07FF,  
	eCanFD_Max
}eCanFDTxCmd;

typedef enum __eCanFDNumber{
    eCanFD_Number_1		= 0,
    eCanFD_Number_2,
    eCanFD_Number_3,    
    eCanFD_Number_4,    
    eCanFD_Number_5,    
	eCanFD_Number_Max
}eCanFDNumber;

typedef enum __eCanFDSpiNumber{
    eCanFD_Spi_Number_1		= 1,
    eCanFD_Spi_Number_2,
    eCanFD_Spi_Number_3,    
    eCanFD_Spi_Number_4,    
	eCanFD_Spi_Number_Max
}eCanFDSpiNumber;



typedef enum __eCanFDBaudRate{
    eCAN_500K_1M = 0x00,
    eCAN_500K_2M,//Default
    eCAN_500K_3M,
    eCAN_500K_4M,
    eCAN_500K_5M, 
    eCAN_500K_6M7,
    eCAN_500K_8M, 	//0x06
    eCAN_500K_10M,
    eCAN_250K_500K, 	//0x08
    eCAN_250K_833K,
    eCAN_250K_1M,
    eCAN_250K_1M5,
    eCAN_250K_2M,
    eCAN_250K_3M,
    eCAN_250K_4M,
    eCAN_1000K_4M, // 0x0f
    eCAN_1000K_8M, // 
    eCAN_125K_500K 	
}eCanFDBaudRate;

typedef enum __eCanFDLed{
    eCanFD_Red		= 0,
    eCanFD_Green,
    eCanFD_ALL,    
	eCanFD_LedControl_Max
}eCanFDLed;

typedef enum __eCanFDLedStatus{
    eCanFD_SelectLed		= 0,
    eCanFD_Control,
	eCanFD_Index_Max
}eCanFDLedStatus;

typedef enum __eCanFDLedControl{
    eCanFD_Control_Off		= 0,
    eCanFD_Control_On,
	eCanFD_Control_Max
}eCanFDLedControl;
typedef enum __eInitCanFDSequence{
	eFDDefaultValue = 0,
    eFDWaitSignal,
    eFDDummySignalReq,
    eFDDummySignalRsp,
    eFDGetVersionReq,
    eFDGetVersionRsp,			//5
    eFDGetMaskingInfoReq,
    eFDGetMaskingInfoRsp,
    eFDGetAnalogSwitchReq,
    eFDGetAnalogSwitchRsp,
    eFDGetCanLineReq,			//10
    eFDGetCanLineRsp,
    eFDGetCanBaudRateReq,
    eFDGetCanBaudRateRsp,
    eFDSetMaskingInfoReq,
    eFDSetMaskingInfoRsp,		//15
    eFDSetCanLineReq,
    eFDSetCanLineRsp,
    eFDSetCanByPassModeReq,
    eFDSetCanByPassModeRsp,           
    eFDSetCanBaudRateReq,	//20
    eFDSetCanBaudRateRsp,
    eFDSetMaskingClearReq,
    eFDSetMaskingClearRsp,
	eFDSetAnalogSwitchReq,      
	eFDSetAnalogSwitchRsp,	//25
	eFDSetMultiDataControlReq,
	eFDSetMultiDataControlRsp,
	eFDSetMultiDataReq,
	eFDSetMultiDataRsp,         
	eFDNotifyCanFDWakeUp,	//30
	eFDUpdateStartReq,
	eFDUpdateStartRsp,			
	eFDUpdateTransmitReq,
	eFDUpdateTransmitRsp,
	eFDUpdateEndReq,		//35
	eFDUpdateEndRsp,
	eFDUpdateAfterResetReq,
	eFDUpdateAfterResetRsp,		
	eFDUpdateWaitBooting,
	eFDSetSleepReq,			//40
    eFDSetSleepRsp,
    eFDLogShowReq,
    eFDLogShowRsp,
    eFDTxWait,                  
}eInitCanFDSequence;

typedef enum __eCanFDHandlerRsp{
    eCanFdhdResNone = 0,
    eCanFdhdResSuccess,
    eCanFdhdResFail,
    eCanFDhdResMax
}eCanFDHandlerRsp;

typedef enum __eCanFDSubControlResult{
    eCanFdSubControl_None = 0,
    eCanFdSubControl_Success,
    eCanFdSubControl_Fail,
    eCanFdSubControl_SetMask,
    eCanFdSubControl_Finish,
    eCanFdSubControl_Next,
    eCanFdSubControl_UpdateComplete,
	eCanFdSubControl_UpdateFail,
    eCanFdSubControl_Max
}eCanFDSubControlResult;

typedef enum __eCanFDResult{
    eCanFdResult_None = 0,
	eCanFdResult_YES,    
	eCanFdResult_NO,    
    eCanFdResult_Max
}eCanFDResult;

typedef enum __eCanFDFotaResult{
    eCanFDFotaResult_None = 0,
	eCanFDFotaResult_Success,
	eCanFDFotaResult_Size,
	eCanFDFotaResult_CheckSum,
	eCanFDFotaResult_Copy,
	eCanFDFotaResult_Target,
	eCanFDFotaResult_Erase,
    eCanFDFotaResult_Max
}eCanFDFotaResult;

typedef enum _eCFD_STATUS_BT
{	
	eCFD_STATUS_BT_NONE = 0,
	eCFD_STATUS_BT_CHECKING,
	eCFD_STATUS_BT_UPDATING,
	eCFD_STATUS_BT_WORKING,
	eCFD_STATUS_BT_NOMODULE,
	eCFD_STATUS_BT_MAX
}eCFD_STATUS_BT;

typedef __packed struct __stCFDStateControl{
    eCanFD_STATE eCanFDState;
    int nInitTryCount;
	int nCurrentState;
	int nPrevState; 
	int nNextState;
	unsigned long ulTimeout;
	int nMaxTimeout;
	int nRetryCount;
	int nCurrentCount;
	eInitCanFDSequence eCanFdRequest;
	eInitCanFDSequence eCanFdResponse;
}stCFDStateControl;

typedef __packed struct __stCFDVersion{
	unsigned short usBlVer;
	unsigned short usAppVer;
	unsigned short usVerfVer;
	unsigned short usBUVer;	
}stCFDVersion;


//
//		Can[0]			Can[1]			Can[2]			Can[3]				Can[4]
//      =SPI1			=SPI2			=SPI4			=SPI4				=SPI3 
//										Analog Switch						(Option)
//		=CanFD 1		=CanFD 2		=CanFD 4		=CanFD 5			=CanFD 3

typedef __packed struct __stMaskingSector{
	int8_t 		ucMaskingCount;
	int8_t 		ucMaskingCheckSum;
}stMaskingSector;

typedef __packed struct __stMaskingInfo{
	int8_t 				ucCanFdSpiNum;
	stMaskingSector 	stSwSector;
	stMaskingSector 	stHwSector;
}stMaskingInfo;

typedef __packed struct __stMasking{
	int8_t			ucSpiMaskCertify[MAX_CFD_CAN_LINE];
	stMaskingInfo	stSpiMaskInfo[MAX_CFD_CAN_LINE];
}stMasking;



typedef __packed struct __stAdapterConfig{
	boolean_t bActiveMasking[MAX_CFD_CAN_LINE];
	boolean_t bActvieLine[MAX_CFD_CAN_LINE];			//Active 되어진 CanFD 설정 라인
	boolean_t bSendCanLine[MAX_CFD_BUFFER];			//CanFD -> Can Module의 Can Line 설정 ( Can1 / Can2 ) , Can1 : False , Can2 : True
	boolean_t bTxTransceiverSet[MAX_CFD_CAN_LINE];		//CanFD : True / Can : False 
	boolean_t bSelectedAnalogSwitch;					//CanFD SPI3번에 대한 Transceiver 에 대한 설정
	int8_t    ucActiveSwitchingLine;					//활성화 여부 
	int8_t    ucCanBaudRate[MAX_CFD_CAN_LINE];
}stAdapterConfig;


#define CAN_ID_MERGE_MAX_CNT	100	
typedef __packed struct _stCanIDMergeList{
	int8_t  	 ucMaskCount;
	unsigned int nStartMask[CAN_ID_MERGE_MAX_CNT];
	unsigned int nEndMask[CAN_ID_MERGE_MAX_CNT];
	unsigned char ucLineIndex[CAN_ID_MERGE_MAX_CNT];
}stCanIDMergeList;


typedef __packed struct __stCFDControl{         
	stCFDStateControl 	stStateCtrl;
	stCFDVersion 		stVer;
	stAdapterConfig	 	stConfig;
	stMasking			stMask;
	stCanIDMergeList	stCanIDMerge;
	eCanFDFotaResult	eFotaResult;
	boolean_t 			bActive;		// FD Control 이 활성화가 되어져 있는지 ?
	boolean_t 			bFDControl;
	uint32_t			unCh1CanLine;
	uint32_t			unCh2CanLine;
	uint32_t			unChFinalCanLine;
	boolean_t 			bInitProcessComplete;		
	eCFD_STATUS_BT eCanFDStatusForBT;
	boolean_t           bCanFDSleepStatus;
	boolean_t			bCanFDByPassOffStatus;
}stCFDControl;

#endif /* __CANFD_DEFINES_H__ */



