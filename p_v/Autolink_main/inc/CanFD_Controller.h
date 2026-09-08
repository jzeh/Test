/* CANFD Controller */
#ifndef __CANFD_CONTROLLER_H__
#define __CANFD_CONTROLLER_H__

#include "common.h"
#include "CanFD_Defines.h"


boolean_t CFD_GetCanFDAdapter();
boolean_t CFD_GetInitComplete();
void CFD_SetInitComplete(boolean_t bIsComplete);
void CFD_SetCanFDAdapter(boolean_t bIsUse);
boolean_t CFD_GetActiveAnalogSwitch();
boolean_t CFD_SelectedAnalogSwitch();
boolean_t CFD_GetCanFDAdapterControlMode();

void SetCanPacket(eCanFDTxCmd eTxCmd ,unsigned char* pucWriteData);
void SetCanPacket_VariableSize(eCanFDTxCmd eTxCmd ,unsigned char* pucWriteData, unsigned char ucDLC);


//Function List
void DeFaultCanSetting();
void CFD_GetCanFDVersion();
void CFD_SetCanFDVersion(unsigned short usBlVer,unsigned short usAppVer,unsigned short usVerfVer,unsigned short usBUVer);
void CFD_SetFWUpdateStart(unsigned short usVersion , int32_t nSize , unsigned short usTarget);
void CFD_SetFWUpdateTransmit(unsigned char *pucFWUpdateFileData);
void CFD_SetFWUpdateTransmit2(unsigned char *pucFWUpdateFileData, unsigned char ucSize);
void CFD_SetFWUpdateEnd(unsigned short usCheckSum);
void CFD_Reset();
void CFD_Sleep();
void CFD_WriteSerial(unsigned char *pucAdapterSerial);
void CFD_ReadSerial();
void CFD_ReadBatteryVoltage();
void CFD_JumpVerification();
void CFD_SetByPassFlag(unsigned char *pucByPassFlag);
void CFD_SetByPassFlagAllOff();
void CFD_SetByPassFlagAllOn();
void CFD_GetByPassFlag();
void CFD_SetTransceiverMode(unsigned char ucSpiIndex , unsigned char ucTxTransceiverSet);
void CFD_SendTransceiverMode(unsigned char *pucTransceiverMode);
void CFD_SetLedControl(eCanFDLed eSelectLed , unsigned char ucLedOnOff);
void CFD_SetCanBaudRate(unsigned char *pucCanBaudRate);
void CFD_GetCanBaudRate();
void CFD_Channel_Masket_Set(unsigned char ucSpiNum,unsigned char ucMaskNum,unsigned char ucMaskType, unsigned int *pStartMaskValue, unsigned int *pEndMaskValue);
void CFD_Channel_Masket_Set_Manual(unsigned char ucSpiNum,unsigned char ucMaskNum,unsigned char ucMaskType, unsigned int *pStartMaskValue, unsigned int *pEndMaskValue,unsigned char *pLineValue);
void CFD_SetCanMasking(unsigned char ucSpiNum,unsigned char ucMode ,unsigned char ucCount, unsigned char* pucMaskingID);
void CFD_GetCanMaskingInfo(unsigned char ucSpiNum);
void CFD_SetClearCanMasking(unsigned char ucSpiNum);
void CFD_SetCanFDConfig(unsigned char ucFDCanLine ,unsigned char ucFDCanSpeed);
void CFD_SetAnalogSwitch(unsigned char ucSelectSwitch);
void CFD_GetAnalogSwitch();
void CFD_SetTxSpiNumber(unsigned char ucMainCanLine , unsigned char ucSpiNumber);
void CFD_SetCanLineMode(eCanLineMode eLineMode);
void CFD_SendCanLine(unsigned char* pucCanLine);
void CFD_GetCanLine();
void CFD_DummyController();
void CFD_SendMultiDataCmd(unsigned char* pucCanLine ,unsigned char ucCanDataSize);
void CFD_SendMultiStartCmd(unsigned int nCanID ,unsigned char ucCanLength);
void CFD_SendMultiAllCmd(unsigned int nCanID,unsigned char ucCanLength);
void CanFD_Initial_Channel(unsigned char ucCanFDLine,  unsigned char ucCanFDBps);
void CanFD_ChannelSet();
void CFD_DisplayConfig();
void CFD_DisplayLog();
void CFD_DisplayLogOff();
void CFD_SetByPassFlagDirect(unsigned char *pucByPassFlag);

void CFD_SetCanFDSleepStatus(boolean_t bSleepStatus);
boolean_t CFD_GetCanFDSleepStatus(void);


#endif /* __CANFD_CONTROLLER_H__ */

