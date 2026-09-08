/**
  ******************************************************************************
  * @file    ManagerSystem.h
  * @author  GIT Connectivity Development 2 Team
  * @version V1.0.0
  * @date    20-Dec-2017
  * @brief   Header for ManagerSystem.c module
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MANAGER_QUEUE_H__
#define __MANAGER_QUEUE_H__

#include "AutolinkMessage.h"


boolean_t MngQueueInit();
#ifndef GLOBAL_SHARE_QUEUE
boolean_t MngQueueSendMessage(int id, int8_t* buffer, int size);
#endif
boolean_t MngQueueGetMessage(int id, int8_t* buffer, int size);
boolean_t GetQueueState();
int32_t MngQueueGetMessageCount(int32_t id);

void SetSysHdQueueStatus(boolean_t used);
boolean_t GetSysHdQueueState();
boolean_t ClearSysHdQueueMessage(int32_t id);
boolean_t GetSysHdQueueMessage(int32_t id, int8_t* buffer, int32_t size);
int32_t GetSysHdQueueMessageCount(int32_t id);
boolean_t SendSysHdQueueMessage(int32_t id, int8_t* buffer, int32_t size);
void SetQueueStatus(boolean_t used);
boolean_t MngQueueClearMessage(int32_t id);

#ifdef GLOBAL_SHARE_QUEUE

typedef __packed struct __stSysHdQueueData
{
    stMsgHeader header;
	int16_t sBufferIndex;
    char* buffer;
}stSysHdQueueData;

#define MSG_QUEUE_BUFFER_SIZE (sizeof(stMsgHeader) + sizeof(char*))


uint8_t* GetMessageQueueBuffer(int16_t sBufferIndex);
boolean_t GetMessageQueueIndex(int16_t* psQueueIndex, uint8_t** ppcBufferAddress);
void ClearMessageShareQueueIndex(int16_t sQueueIndex);
#endif //#ifdef GLOBAL_SHARE_QUEUE

#endif //__MANAGER_QUEUE_H__

/***************************** END OF FILE ****/
