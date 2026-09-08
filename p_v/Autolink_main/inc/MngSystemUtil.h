/**
  ******************************************************************************
  * @file    ManagerSystemUtil.h
  * @author  GIT Connectivity Development 2 Team
  * @version V1.0.0
  * @date    20-Dec-2017
  * @brief   Header for ManagerSystem.c module
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MANAGER_SYSTEM_UTIL_H__
#define __MANAGER_SYSTEM_UTIL_H__

#include "common.h"

#include "AutolinkMessage.h"
#include "MngSystem.h"

#include "HalHandler.h"


enum
{
    eGetStringEvent,
    eGetStringSubEvent
};

// system util definition
void Send2MngSysSub(uint16_t iId, int32_t iEvent,int32_t iSubEvent, stCarReport* pstReport,uint32_t unTraceMng);
void Send2MngModem(uint16_t iId, int32_t iEvent,int32_t iSubEvent, stCarReport* pstReport, uint32_t unTraceMng);
void Send2MngModem2(stMsgMdm* pstMsg);
void Send2MngModem3(uint16_t iId, int32_t iEvent,int32_t iSubEvent,int32_t iResult, stCarReport* pstReport, uint32_t unTraceMng);
void Send2MngSensor(uint16_t iId, int32_t iEvent,int32_t iSubEvent, stCarReport* pstReport,uint32_t unTraceMng);
void Send2MngObd(uint16_t iId, int32_t iEvent,int32_t iSubEvent, stCarReport* pstReport,uint32_t unTraceMng);
void Send2MngObd2(stMsgObd* pstMsgObd);
void Send2MngStorage(uint16_t iId, int32_t iEvent,int32_t iSubEvent, stCarReport* pstReport,uint32_t unTraceMng);
void Send2MngStorage2(stMsgStorage* pstMsgStorage);
void Send2MngSysMsg(uint16_t iId, int32_t iEvent,int32_t iSubEvent, stCarReport* pstReport,uint32_t unTraceMng);
void Send2MngSysMsg2(uint16_t iId, int32_t iEvent,int32_t iSubEvent,int32_t iResult, stCarReport* pstReport,uint32_t unTraceMng);
void Send2MngSysMsg3(stMsgSysMsg* pstMsgSysMsg);
void Send2MngSysSensor(uint16_t iId, int32_t iEvent,int32_t iSubEvent, stCarReport* pstReport,uint32_t unTraceMng);
void Send2MngSys(uint16_t iId, int32_t iEvent,int32_t iSubEvent, stCarReport* pstReport,uint32_t unTraceMng);

#if defined(FEATURE_EXTENSION_BOARD)
void Send2SysExtend(uint16_t iId, int32_t iEvent,int32_t iSubEvent,int32_t iResult, stMsgExtend* pstExtend,uint32_t unTraceMng);
#endif

void ConfigforAlram(boolean_t bWakeupSoon, boolean_t bReqLongSleep);
uint32_t RTC_DateToBinary(stHalRTC_TimeTypeDef stTime, stHalRTC_DateTypeDef stDate);
boolean_t CheckAlramInterrupt();
void EnableSystemMessageTimer(int32_t* iId,int32_t iValue, int32_t iMode, fnSWCallBack callback);
void DisableSystemMessageTimer(int32_t iId);
void GetLocalTimeforDate(stHalRTCTypeDef* pstDate);
uint32_t ConvertRTC2Seconds(stHalRTC_TimeTypeDef stTime,stHalRTC_DateTypeDef stDate);

// related with geofence
boolean_t CheckGeoFence(stGeofenceUnit stCurrentUnit,stGeofenceUnit* pstSettingUnit,boolean_t bActive,boolean_t bReport);
void SetGeofenceSetting(stGeoFenceSetting stGeofenceValue);
void GetGeofenceSetting(stGeoFenceSetting* pstGeofenceSetting);

void CheckPolygonGeoFence();
void SetPolygonGeofenceSetting(stPolygonGeoFenceSetting stPolygonGeofenceValue);
boolean_t GetPolygonGeofenceActive();

void DisplayReport(int8_t* buffer, stMsgSysMsg* pstMessage);

void GetAutolinkConfigProperty(uint8_t cIndex,void* pvValue);
void SetAutolinkConfigProperty(uint8_t cIndex,void* pvValue);

stGeoFenceSetting* GetGeofenceSettingPointerAddress();

void DisplayTime(char* pstr, uint32_t unLocalTime);
uint32_t GetUtcTimefromTime(uint32_t unLocalTime);

boolean_t CheckInterruptSignal();
uint32_t GetTimefromDate2(stHalRTCTypeDef stDate);
uint32_t GetLocalTimefromTime(uint32_t unUTCTime);
void GetDatefromTime2(stHalRTCTypeDef* stDate,uint32_t nTime);
void ClearAppVersion();

#if defined (GLOBAL_SHARE_QUEUE)
void SendSysHdShareQueueMessage(uint16_t usId, uint8_t* pSrcHeader, uint32_t unSrcHeaderLength, uint8_t* pSrcBody, uint32_t unSrcBodyLength);
boolean_t GetSysHdShareQueueMessage(uint16_t usId, uint8_t* pDstHeader, uint32_t unDstHeaderLength, uint8_t* pDstBody, uint32_t unDstBodyLength);
#endif
#endif //__MANAGER_SYSTEM_UTIL_H__

/***************************** END OF FILE ****/
