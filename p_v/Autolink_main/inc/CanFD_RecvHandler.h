/* CANFD_RECVHANDLER */
#ifndef __CANFD_RECVHANDLER_H__
#define __CANFD_RECVHANDLER_H__
         
#include "common.h"
#include "CanFD_Defines.h"

//
typedef void (*fnpEventHanderList)(unsigned char* pCanData);


//////////////////////////////////////////////////
void FindCanFDResponse(eCanFDTxCmd eTxCmd,unsigned char* pCanData);

//////////////////////////////////////////////////
//ID
void SetCanFDResponse(eInitCanFDSequence eCanFDRequest);

//Data
void CFD_RecvGetVersionData(unsigned char* pCanData);
void CFD_RecvGetBaudRateData(unsigned char* pCanData);
void CFD_RecvGetAnalogSwitch(unsigned char* pCanData);


//CanFD Masking
void CFD_RecvGetMaskingData(unsigned char* pCanData);
int8_t GetMaskingCheckSumForDB();
int8_t GetMaskingCountForDB();

//CanFD CanLine
void CFD_RecvGetCanLineData(unsigned char* pCanData);

//Multi Start Cmd
void CFD_RecvMultiStartCmd(unsigned char* pCanData);

//Multi Data Cmd
void CFD_RecvMultiDataCmd(unsigned char* pCanData);

//Multi Data Clear
void CFD_RecvMultiDataClear();
void CFD_ShowMultiData();
void CFD_PassThruReadMsgs();
void CFD_RecvUnKnownMsg(unsigned char* pCanData);





//Notify CanFD Module WakeUp
void CFD_RecvNotifyWakeUpSignal(unsigned char* pCanData);

void CFD_RecvGetEndResponse(unsigned char* pCanData);
void CFD_RecvGetTransmitResponse(unsigned char* pCanData);
void CFD_RecvByPassResponse(unsigned char* pCanData);
void CFD_RecvSleepResponse(unsigned char* pCanData);
void CFD_RecvSetCanLineData(unsigned char* pCanData);

#endif /* __CANFD_RECVHANDLER_H__ */

