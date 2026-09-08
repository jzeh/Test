/*
  ******************************************************************************
  * @file    MngQueue.c
  * @author
  * @version V1.0.0
  * @date    2018-01-15
  * @brief
  *
  *
  ******************************************************************************
*/

#include <math.h>

#include "HdDebug.h"
#include "MngQueue.h"
#include "GIT_Util.h"
#include "AutolinkMessage.h"

#define Trace(...)  GITDebug(DEBUG_MODULES_QUEUE,__VA_ARGS__);
//#define MONI_DEBUG

#if !defined(GLOBAL_SHARE_QUEUE)
//////////////////////////////////////////////////////////////////
// these struct is related with car report
#define private
#define GLOBAL_VARIABLE_QUEUE
//#define MALLOC_QUEUE

boolean_t m_bQueueUsed = false;
void SetQueueStatus(boolean_t used);

#if defined(GLOBAL_VARIABLE_QUEUE)

// header and buffer : 8 is header
#define MSG_QUEUE_BUFFER_SIZE (sizeof(stMsgHeader) + sizeof(stCarReport))

typedef __packed struct __stMngQueueData
{
    //int8_t buffer[MSG_QUEUE_BUFFER_SIZE];
    char* buffer;
}stMngQueueData;

// buffer size
#define MAX_MNG_QUEUE_SIZE 4
#define MAX_MNG_QUEUE_DATA_SIZE 10
#define MAX_MNG_QUEUE_SENSOR_SIZE 4
#define MAX_MNG_QUEUE_STORAGE_SIZE (MAX_MNG_QUEUE_SIZE*3)
#define MAX_MNG_QUEUE_MESSAGE_SIZE (MAX_MNG_QUEUE_SIZE*2)


typedef __packed struct __stMngQueue
{
    uint32_t count;
    uint32_t max;
    uint32_t cur;
    uint32_t front;
    uint32_t rear;
    uint32_t datasize;
//#if true
    stMngQueueData queue[MAX_MNG_QUEUE_SIZE*3];
//#else
//    stMngQueueData* queue;
//#endif
}stMngQueue;

private stMngQueue m_starrMngQueueList[ID_MNG_QUEUE_MAX];

char m_carrMngSysQueue[sizeof(stMsgSys)*MAX_MNG_QUEUE_SIZE];
char m_carrMngSysSubQueue[sizeof(stMsgSysSub)*MAX_MNG_QUEUE_SIZE];
char m_carrMngSysMsgQueue[sizeof(stMsgSysMsg)*MAX_MNG_QUEUE_MESSAGE_SIZE];
char m_carrMngSysSensorQueue[sizeof(stMsgSysSensor)*MAX_MNG_QUEUE_SENSOR_SIZE];
char m_carrMngMdmQueue[sizeof(stMsgMdm)*MAX_MNG_QUEUE_SIZE];
char m_carrMngStorageQueue[sizeof(stMsgStorage)*MAX_MNG_QUEUE_STORAGE_SIZE];
char m_carrMngObdQueue[sizeof(stMsgObd)*MAX_MNG_QUEUE_SIZE];
char m_carrMngSensorQueue[sizeof(stMsgSensor)*MAX_MNG_QUEUE_SENSOR_SIZE];
char m_carrMngDataQueue[sizeof(stMsgData)*MAX_MNG_QUEUE_DATA_SIZE];
char m_carrMngBtQueue[sizeof(stMsgBt)*MAX_MNG_QUEUE_SIZE];
char m_carrMngMdmhQueue[sizeof(stMsgMdmHandler)*MAX_MNG_QUEUE_SIZE];
#if defined(FEATURE_EXTENSION_BOARD)
char m_carrMngSysExtendedQueue[sizeof(stMsgExtend)*MAX_MNG_QUEUE_SIZE];
#endif

extern bool CheckDebugOption(unsigned int option);

void InitQueueBuffer();

void InitQueueBuffer()
{ 
    for(int i=0;i<MAX_MNG_QUEUE_SIZE;i++)
    {
        m_starrMngQueueList[ID_MNG_QUEUE_SYS].queue[i].buffer = &m_carrMngSysQueue[sizeof(stMsgSys)*i];
        m_starrMngQueueList[ID_MNG_QUEUE_SYS_SUB].queue[i].buffer = &m_carrMngSysSubQueue[sizeof(stMsgSysSub)*i];
        //m_starrMngQueueList[ID_MNG_QUEUE_SYS_MSG].queue[i].buffer = &m_carrMngSysMsgQueue[sizeof(stMsgSysMsg)*i];
        //m_starrMngQueueList[ID_MNG_QUEUE_SYS_SENSOR].queue[i].buffer = &m_carrMngSysSensorQueue[sizeof(stMsgSysSensor)*i];
        m_starrMngQueueList[ID_MNG_QUEUE_MDM].queue[i].buffer = &m_carrMngMdmQueue[sizeof(stMsgMdm)*i];
        //m_starrMngQueueList[ID_MNG_QUEUE_STORAGE].queue[i].buffer = &m_carrMngStorageQueue[sizeof(stMsgStorage)*i];
        m_starrMngQueueList[ID_MNG_QUEUE_OBD].queue[i].buffer = &m_carrMngObdQueue[sizeof(stMsgObd)*i];
        //m_starrMngQueueList[ID_MNG_QUEUE_SENSOR].queue[i].buffer = &m_carrMngSensorQueue[sizeof(stMsgSensor)*i];
        //m_starrMngQueueList[ID_MNG_QUEUE_DATA].queue[i].buffer = &m_carrMngDataQueue[sizeof(stMsgData)*i];
        m_starrMngQueueList[ID_MNG_QUEUE_BT].queue[i].buffer = &m_carrMngBtQueue[sizeof(stMsgBt)*i];
        m_starrMngQueueList[ID_MNG_QUEUE_MDMH].queue[i].buffer = &m_carrMngMdmhQueue[sizeof(stMsgMdmHandler)*i];
#if defined(FEATURE_EXTENSION_BOARD)
        m_starrMngQueueList[ID_MNG_QUEUE_EXTEND].queue[i].buffer = &m_carrMngSysExtendedQueue[sizeof(stMsgExtend)*i];
#endif

    }

    for(int i=0;i<MAX_MNG_QUEUE_MESSAGE_SIZE;i++)
    {
        m_starrMngQueueList[ID_MNG_QUEUE_SYS_MSG].queue[i].buffer = &m_carrMngSysMsgQueue[sizeof(stMsgSysMsg)*i];
    }

    for(int i=0;i<MAX_MNG_QUEUE_STORAGE_SIZE;i++)
    {
        m_starrMngQueueList[ID_MNG_QUEUE_STORAGE].queue[i].buffer = &m_carrMngStorageQueue[sizeof(stMsgStorage)*i];
    }
	for(int i=0;i<MAX_MNG_QUEUE_DATA_SIZE;i++)
    {
	    m_starrMngQueueList[ID_MNG_QUEUE_DATA].queue[i].buffer = &m_carrMngDataQueue[sizeof(stMsgData)*i];
    }
	for(int i=0;i<MAX_MNG_QUEUE_SENSOR_SIZE;i++)
    {
        m_starrMngQueueList[ID_MNG_QUEUE_SYS_SENSOR].queue[i].buffer = &m_carrMngSysSensorQueue[sizeof(stMsgSysSensor)*i];
		m_starrMngQueueList[ID_MNG_QUEUE_SENSOR].queue[i].buffer = &m_carrMngSensorQueue[sizeof(stMsgSensor)*i];
    }
}

/**
 * @author HyeonKeol.Moon
 * @brief Initialize message queue
 * @param None
 * @return None
 */
private void InitializeQueue(int32_t id, int32_t iQqueueSize, int32_t iBufferSize)
{
    m_starrMngQueueList[id].count = 0;
    m_starrMngQueueList[id].front= 0;
    m_starrMngQueueList[id].rear = 0;
    m_starrMngQueueList[id].datasize = iBufferSize;
    m_starrMngQueueList[id].max = iQqueueSize;
//#if true    
//    m_starrMngQueueList[id].queue = (stMngQueueData*)malloc(sizeof(stMngQueueData)*iQqueueSize);
//#endif
}

/**
 * @author HyeonKeol.Moon
 * @brief Initialize message queue list
 * @param None
 * @return None
 */
boolean_t MngQueueInit()
{    
    // Message Queue between modem manager and message manager
    InitializeQueue(ID_MNG_QUEUE_SYS,MAX_MNG_QUEUE_SIZE,sizeof(stMsgSys));

    // Message Queue between modem manager and message manager
    InitializeQueue(ID_MNG_QUEUE_SYS_SUB,MAX_MNG_QUEUE_SIZE,sizeof(stMsgSysSub));

    // Message Queue between modem manager and message manager
    InitializeQueue(ID_MNG_QUEUE_SYS_MSG,MAX_MNG_QUEUE_MESSAGE_SIZE,sizeof(stMsgSysMsg));

    // Message Queue between modem manager and message manager
    InitializeQueue(ID_MNG_QUEUE_SYS_SENSOR,MAX_MNG_QUEUE_SENSOR_SIZE,sizeof(stMsgSysSensor));

    // Message Queue between modem manager and message manager
    InitializeQueue(ID_MNG_QUEUE_MDM,MAX_MNG_QUEUE_SIZE,sizeof(stMsgMdm));

    // Storage Queue between storage mananger and message manager
    InitializeQueue(ID_MNG_QUEUE_STORAGE,MAX_MNG_QUEUE_STORAGE_SIZE,sizeof(stMsgStorage));

    // Obd queue between obd manager and message manager
    InitializeQueue(ID_MNG_QUEUE_OBD,MAX_MNG_QUEUE_SIZE,sizeof(stMsgObd));

    // sensor queue between sensor manager and message manager
    InitializeQueue(ID_MNG_QUEUE_SENSOR,MAX_MNG_QUEUE_SENSOR_SIZE,sizeof(stMsgSensor));

    // data queue between modem manager and modem manager
    InitializeQueue(ID_MNG_QUEUE_DATA,MAX_MNG_QUEUE_DATA_SIZE,sizeof(stMsgMdm));

    // data queue between bt manager and message manager
    InitializeQueue(ID_MNG_QUEUE_BT,MAX_MNG_QUEUE_SIZE,sizeof(stMsgBt));   
    
    // data queue between modem handler manager and message manager
    InitializeQueue(ID_MNG_QUEUE_MDMH,MAX_MNG_QUEUE_SIZE,sizeof(stMsgMdmHandler));   

#if defined(FEATURE_EXTENSION_BOARD)
    InitializeQueue(ID_MNG_QUEUE_EXTEND,MAX_MNG_QUEUE_SIZE, sizeof(stMsgExtend));
#endif

    InitQueueBuffer();

    return 0;
}

#endif


#if defined(MALLOC_QUEUE)

typedef __packed struct __stMngQueueData
{
    int8_t* buffer;
}stMngQueueData;


typedef __packed struct __stMngQueue
{
    uint32_t count;
    uint32_t max;
    uint32_t cur;
    uint32_t front;
    uint32_t rear;
    uint32_t datasize;
    stMngQueueData* queue;
}stMngQueue;

private stMngQueue m_starrMngQueueList[ID_MNG_QUEUE_MAX];

private void InitializeQueue(int32_t id, int32_t iQqueueSize, int32_t iBufferSize)
{
    m_starrMngQueueList[id].count = 0;
    m_starrMngQueueList[id].front= 0;
    m_starrMngQueueList[id].rear = 0;
    m_starrMngQueueList[id].datasize = iBufferSize;
    m_starrMngQueueList[id].max = iQqueueSize;
    m_starrMngQueueList[id].queue = (stMngQueueData*)malloc(sizeof(stMngQueueData)*iBufferSize);

    for(int32_t i=0;i<iBufferSize;i++)
        m_starrMngQueueList[id].queue[i].buffer = (int8_t*)malloc(sizeof(int8_t)*iBufferSize);
}

boolean_t MngQueueInit()
{
    // Message Queue between modem manager and message manager
    InitializeQueue(ID_MNG_QUEUE_SYS,5,sizeof(stMsgSys));
    // Message Queue between modem manager and message manager
    InitializeQueue(ID_MNG_QUEUE_SYS_SUB,3,sizeof(stMsgSysSub));
    // Message Queue between modem manager and message manager
    InitializeQueue(ID_MNG_QUEUE_SYS_MSG,3,sizeof(stMsgSysMsg));
    // Message Queue between modem manager and message manager
    InitializeQueue(ID_MNG_QUEUE_SYS_SENSOR,3,sizeof(stMsgSysSensor));
    // Message Queue between modem manager and message manager
    InitializeQueue(ID_MNG_QUEUE_MDM,5,sizeof(stMsgMdm));
    // Storage Queue between storage mananger and message manager
    InitializeQueue(ID_MNG_QUEUE_STORAGE,5,sizeof(stMsgStorage));
    // Obd queue between obd manager and message manager
    InitializeQueue(ID_MNG_QUEUE_OBD,3,sizeof(stMsgObd));
    // sensor queue between sensor manager and message manager
    InitializeQueue(ID_MNG_QUEUE_SENSOR,3,sizeof(stMsgSensor));
    // data queue between modem manager and modem manager
    InitializeQueue(ID_MNG_QUEUE_DATA,5,sizeof(stMsgMdm));
    // data queue between bt manager and message manager
    InitializeQueue(ID_MNG_QUEUE_BT,3,sizeof(stMsgBt));      
    // data queue between modem handler manager and message manager
    InitializeQueue(ID_MNG_QUEUE_MDMH,3,sizeof(stMsgMdmHandler));   

#if defined(FEATURE_EXTENSION_BOARD)
       //InitializeQueue(ID_MNG_QUEUE_EXTEND,MAX_MNG_QUEUE_SIZE, sizeof(stMsgExtend));

#endif
    
    return 0;
}
#endif //#if defined(DEFINE_MALLOC_QUEUE)

/**
 * @author HyeonKeol.Moon
 * @brief check if queue is full or not
 * @param None
 * @return None
 */
private boolean_t isQueueFull(int32_t id)
{
    if( ((m_starrMngQueueList[id].rear+1)%m_starrMngQueueList[id].max) == m_starrMngQueueList[id].front &&
        m_starrMngQueueList[id].count > 0 )
        return true;

    return false;
}

/**
 * @author HyeonKeol.Moon
 * @brief check if queue is empty or not
 * @param None
 * @return None
 */
private boolean_t isQueueEmpty(int32_t id)
{
    if( m_starrMngQueueList[id].rear == m_starrMngQueueList[id].front &&
        m_starrMngQueueList[id].count == 0)
        return true;

    return false;
}

/**
 * @author HyeonKeol.Moon
 * @brief send message to a manager that was assigned with id
 * @param id message identify
 * @param buffer message struct related with id manager
 * @param size TBD
 * @return false : error, true : success
 */
boolean_t MngQueueSendMessage(int32_t id, int8_t* buffer, int32_t size)
{
	uint32_t wTemp;

    GIT_Assert( (id >= 0) && (id < ID_MNG_QUEUE_MAX) ,eErrorCodeQue|eUnknownId);
    if( id >= 0 && id < ID_MNG_QUEUE_MAX )
    {
        GIT_Assert(size==m_starrMngQueueList[id].datasize,eErrorCodeQue|eSizeLimit);
#if false
        if( size != m_starrMngQueueList[id].datasize )
        {
            while(1)
            {
              Trace("%s] ID : %d]data size is different\n",__FUNCTION__,id);
            }
            // error
            // data struct is weired
            return false;
        }
#endif
        if( isQueueFull(id ) == true )
        {
            Trace("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\r\n");
            printf("%s] ID : %d] error data queue is full\r\n",__FUNCTION__,id);
            Trace("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\r\n");
#if false
            while(1)
            {
              Trace("%s] ID : %d]data queue is full\n",__FUNCTION__,id);
            }
#endif
            // error
            return false;
        }

        wTemp = ++(m_starrMngQueueList[id].rear);
        m_starrMngQueueList[id].rear = wTemp % m_starrMngQueueList[id].max;
        memcpy(m_starrMngQueueList[id].queue[m_starrMngQueueList[id].rear].buffer,buffer,size);
        m_starrMngQueueList[id].count++;

        SetQueueStatus(true);
        //Trace("%s] id:%d, queue]\n",__FUNCTION__,id);

		if( CheckDebugOption(DEBUG_MODULES_QUEUE) == true )
		{
			stMsgMdm *pststMsgSys;
			pststMsgSys = (void*)buffer;
			Trace("[-S-]id:%d->%d,event:%X,subevent:%X,t:%X\r\n",
			    pststMsgSys->header.id,id,pststMsgSys->header.event,pststMsgSys->header.subEvent,pststMsgSys->header.unTraceMng);
		}

#if defined(MONI_DEBUG)
        Trace("%s] id:%d, queue]\r\n",__FUNCTION__,id);
        for(int32_t i=0;i<10;i++)
        {
            Trace("%d ",m_starrMngQueueList[id].queue[m_starrMngQueueList[id].rear].buffer[i]);
        }
        Trace("\r\n");

        Trace("%s] buffer",__FUNCTION__);
        for(int32_t i=0;i<10;i++)
        {
            Trace("%d ",buffer[i]);
        }
        Trace("\r\n");
#endif
    }

    return 0;
}

/**
 * @author HyeonKeol.Moon
 * @brief get message relate with id from queue.
 * @param id message identify
 * @param buffer get message struct related with id manager
 * @param size TBD
 * @return false : error, true : success
 */
boolean_t MngQueueGetMessage(int32_t id, int8_t* buffer, int32_t size)
{
	uint32_t wTemp;

    GIT_Assert( (id >= 0) && (id < ID_MNG_QUEUE_MAX) ,eErrorCodeQue|eUnknownId);
    if( id >= 0 && id < ID_MNG_QUEUE_MAX )
    {
        if( isQueueEmpty(id) == true )
        {
            //error
            return false;
        }

        wTemp = ++(m_starrMngQueueList[id].front);
        m_starrMngQueueList[id].front = wTemp % m_starrMngQueueList[id].max;
        memcpy(buffer, m_starrMngQueueList[id].queue[m_starrMngQueueList[id].front].buffer, size);
        m_starrMngQueueList[id].count--;

        //Trace("%s] id:%d, queue] \n",__FUNCTION__,id);
        SetQueueStatus(true);
		if( CheckDebugOption(DEBUG_MODULES_QUEUE) == true )
		{
			stMsgMdm *pststMsgSys;
			pststMsgSys = (void*)buffer;
			Trace("[POP]id:%d->%d,event:%X,sub:%X,t:%X\r\n",pststMsgSys->header.id,id,pststMsgSys->header.event,pststMsgSys->header.subEvent,pststMsgSys->header.unTraceMng);
		}
		
#if defined (MONI_DEBUG)
        Trace("%s] id:%d, queue]\r\n",__FUNCTION__,id);
        for(int32_t i=0;i<10;i++)
        {
            Trace("%d ",m_starrMngQueueList[id].queue[m_starrMngQueueList[id].front].buffer[i]);
        }
        Trace("\r\n");

        Trace("%s] buffer]",__FUNCTION__);
        for(int32_t i=0;i<10;i++)
        {
            Trace("%d ",buffer[i]);
        }
        Trace("\r\n");
#endif

        return true;
    }

    return false;
}

boolean_t GetQueueState()
{
    boolean_t retValue = m_bQueueUsed;
    m_bQueueUsed = false;

    //Trace("%s] m_bQueueUsed :%d\n",__FUNCTION__,m_bQueueUsed);
    return retValue;
}

void SetQueueStatus(boolean_t used)
{
    m_bQueueUsed = used;
}


/**
 * @author HyeonKeol.Moon
 * @brief clear queue for the id.
 * @param None
 * @return None
 */
boolean_t MngQueueClearMessage(int32_t id)
{
    m_starrMngQueueList[id].count = 0;
    m_starrMngQueueList[id].front= 0;
    m_starrMngQueueList[id].rear = 0;

    return true;
}

/**
 * @author HyeonKeol.Moon
 * @brief get data count for the id.
 * @param None
 * @return None
 */

int32_t MngQueueGetMessageCount(int32_t id)
{
     return m_starrMngQueueList[id].count;
}
#else //#if !defined(GLOBAL_SHARE_QUEUE)
boolean_t m_bQueueUsed = false;
typedef __packed struct __stMngQueueData
{
    char* buffer;
}stMngQueueData;
#define MAX_MNG_QUEUE_SIZE 8 
#define MAX_MNG_QUEUE_DATA_SIZE 8
#define MAX_MNG_QUEUE_SENSOR_SIZE 8
#define MAX_MNG_QUEUE_STORAGE_SIZE (MAX_MNG_QUEUE_SIZE*3)
#define MAX_MNG_QUEUE_MESSAGE_SIZE (MAX_MNG_QUEUE_SIZE*2)
typedef __packed struct __stSysHdQueue
{
    uint32_t count;
    uint32_t max;
    uint32_t cur;
    uint32_t front;
    uint32_t rear;
    uint32_t datasize;
    stSysHdQueueData* pstQueue;
}stSysHdQueue;
stSysHdQueue m_starrMngQueueList[ID_MNG_QUEUE_MAX];
void InitQueueBuffer()
{
    memset(&m_starrMngQueueList,0,sizeof(m_starrMngQueueList));
}
 
void InitializeQueue(int32_t id, int32_t iQqueueSize, int32_t iBufferSize)
{
	m_starrMngQueueList[id].count = 0;
	m_starrMngQueueList[id].front= 0;
	m_starrMngQueueList[id].rear = 0;
	m_starrMngQueueList[id].datasize = sizeof(stSysHdQueueData);
	m_starrMngQueueList[id].max = iQqueueSize;
	m_starrMngQueueList[id].pstQueue = (stSysHdQueueData*)malloc(sizeof(stSysHdQueueData)*iQqueueSize);
	memset(m_starrMngQueueList[id].pstQueue,0,sizeof(stSysHdQueueData)*(iQqueueSize));

}
 
boolean_t MngQueueInit()
{
    InitQueueBuffer();

    InitializeQueue(ID_MNG_QUEUE_SYS,MAX_MNG_QUEUE_SIZE,sizeof(stMsgSys));
#ifdef CHECK_MALLOC
__iar_dlmalloc_stats();
#endif
    InitializeQueue(ID_MNG_QUEUE_SYS_SUB,MAX_MNG_QUEUE_SIZE,sizeof(stMsgSysSub));
#ifdef CHECK_MALLOC
__iar_dlmalloc_stats();
#endif
    InitializeQueue(ID_MNG_QUEUE_SYS_MSG,MAX_MNG_QUEUE_MESSAGE_SIZE,sizeof(stMsgSysMsg));
#ifdef CHECK_MALLOC
__iar_dlmalloc_stats();
#endif
    InitializeQueue(ID_MNG_QUEUE_SYS_SENSOR,MAX_MNG_QUEUE_SENSOR_SIZE,sizeof(stMsgSysSensor));
#ifdef CHECK_MALLOC
__iar_dlmalloc_stats();
#endif
    InitializeQueue(ID_MNG_QUEUE_MDM,MAX_MNG_QUEUE_SIZE,sizeof(stMsgMdm));
#ifdef CHECK_MALLOC
__iar_dlmalloc_stats();
#endif
    InitializeQueue(ID_MNG_QUEUE_STORAGE,MAX_MNG_QUEUE_STORAGE_SIZE,sizeof(stMsgStorage));
#ifdef CHECK_MALLOC
__iar_dlmalloc_stats();
#endif
    InitializeQueue(ID_MNG_QUEUE_OBD,MAX_MNG_QUEUE_SIZE,sizeof(stMsgObd));
#ifdef CHECK_MALLOC
__iar_dlmalloc_stats();
#endif
    InitializeQueue(ID_MNG_QUEUE_SENSOR,MAX_MNG_QUEUE_SENSOR_SIZE,sizeof(stMsgSensor));
#ifdef CHECK_MALLOC
__iar_dlmalloc_stats();
#endif
    InitializeQueue(ID_MNG_QUEUE_DATA,MAX_MNG_QUEUE_DATA_SIZE,sizeof(stMsgMdm));
#ifdef CHECK_MALLOC
__iar_dlmalloc_stats();
#endif
    InitializeQueue(ID_MNG_QUEUE_BT,MAX_MNG_QUEUE_SIZE,sizeof(stMsgBt));
#ifdef CHECK_MALLOC
__iar_dlmalloc_stats();
#endif
    InitializeQueue(ID_MNG_QUEUE_MDMH,MAX_MNG_QUEUE_SIZE,sizeof(stMsgMdmHandler));
#ifdef CHECK_MALLOC
__iar_dlmalloc_stats();
#endif
    return 0;
}
boolean_t isQueueFull(int32_t id)
{
    if( ((m_starrMngQueueList[id].rear+1)%m_starrMngQueueList[id].max) == m_starrMngQueueList[id].front &&
        m_starrMngQueueList[id].count > 0 )
        return true;
    return false;
}
boolean_t isQueueEmpty(int32_t id)
{
    if( m_starrMngQueueList[id].rear == m_starrMngQueueList[id].front &&
        m_starrMngQueueList[id].count == 0)
        return true;
    return false;
}
boolean_t SendSysHdQueueMessage(int32_t id, int8_t* buffer, int32_t size)
{
	uint32_t wTemp;
    if( id >= 0 && id < ID_MNG_QUEUE_MAX )
    {
        if( isQueueFull(id ) == true )
        {
            printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
            printf("%s] ID : %d] error data queue is full\n",__FUNCTION__,id);
            Trace("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
#if false
            while(1)
            {
              Trace("%s] ID : %d]data queue is full\n",__FUNCTION__,id);
            }
#endif
            return false;
        }
        wTemp = ++(m_starrMngQueueList[id].rear);
        m_starrMngQueueList[id].rear = wTemp % m_starrMngQueueList[id].max;
        memcpy(&m_starrMngQueueList[id].pstQueue[m_starrMngQueueList[id].rear],buffer,size);
        m_starrMngQueueList[id].count++;
        SetSysHdQueueStatus(true);
#if QUEUE_DATA_LOG
		stMsgSys *pststMsgSys;
		pststMsgSys = (void*)buffer;
        printf("[-S-]id:eMng%d->ID_MNG_%d,event:%X,sub:%X\n",pststMsgSys->header.id,id,pststMsgSys->header.event,pststMsgSys->header.subEvent);
#endif
    }
    return true;
}
boolean_t GetSysHdQueueMessage(int32_t id, int8_t* buffer, int32_t size)
{
	uint32_t wTemp;
    if( id >= 0 && id < ID_MNG_QUEUE_MAX )
    {
        if( isQueueEmpty(id) == true )
        {
            return false;
        }
        wTemp = ++(m_starrMngQueueList[id].front);
        m_starrMngQueueList[id].front = wTemp % m_starrMngQueueList[id].max;
        memcpy(buffer, &m_starrMngQueueList[id].pstQueue[m_starrMngQueueList[id].front], size);
        m_starrMngQueueList[id].count--;
        SetSysHdQueueStatus(true);
#if QUEUE_DATA_LOG
		stMsgSys *pststMsgSys;
		pststMsgSys = (void*)buffer;
		printf("[POP]id:eMng%d->ID_MNG_%d,event:%X,sub:%X\n",pststMsgSys->header.id,id,pststMsgSys->header.event,pststMsgSys->header.subEvent);
#endif
        return true;
    }
    return false;
}
boolean_t GetQueueState()
{
    boolean_t retValue = m_bQueueUsed;
    m_bQueueUsed = false;
    return retValue;
}
void SetQueueStatus(boolean_t used)
{
    m_bQueueUsed = used;
}
boolean_t MngQueueClearMessage(int32_t id)
{
    m_starrMngQueueList[id].count = 0;
    m_starrMngQueueList[id].front= 0;
    m_starrMngQueueList[id].rear = 0;
    return true;
}
int32_t MngQueueGetMessageCount(int32_t id)
{
     return m_starrMngQueueList[id].count;
}
boolean_t GetSysHdQueueState()
{
    boolean_t retValue = m_bQueueUsed;
    m_bQueueUsed = false;
    return retValue;
}
void SetSysHdQueueStatus(boolean_t used)
{
    m_bQueueUsed = used;
}
boolean_t ClearSysHdQueueMessage(int32_t id)
{
    m_starrMngQueueList[id].count = 0;
    m_starrMngQueueList[id].front= 0;
    m_starrMngQueueList[id].rear = 0;
    return true;
}
int32_t GetSysHdQueueMessageCount(int32_t id)
{
     return m_starrMngQueueList[id].count;
}
#define MAX_MESSAGE_SHARE_QUEUE_LIST_COUNT 10
typedef __packed struct __stSystemShareQueue{
	boolean_t bActive;
	uint32_t nSize;
	uint8_t unArrBuffer[sizeof(stCarReport)];
}stSystemShareQueue;
stSystemShareQueue m_cArrSystemQueueList[MAX_MESSAGE_SHARE_QUEUE_LIST_COUNT];
uint8_t* GetMessageQueueBuffer(int16_t sBufferIndex)
{
	if( sBufferIndex >=0 && sBufferIndex < MAX_MESSAGE_SHARE_QUEUE_LIST_COUNT )
	{
		return &m_cArrSystemQueueList[sBufferIndex].unArrBuffer[0];
	}
	else
	{
		printf("%s] ERROR Share Queue was FULL\n", __FUNCTION__);
		printf("%s] ERROR Share Queue was FULL\n", __FUNCTION__);
		printf("%s] ERROR Share Queue was FULL\n", __FUNCTION__);
		printf("%s] ERROR Share Queue was FULL\n", __FUNCTION__);
	}
	return (uint8_t*)NULL;
}
boolean_t GetMessageQueueIndex(int16_t* psQueueIndex, uint8_t** ppcBufferAddress)
{
	for(int i=0;i<MAX_MESSAGE_SHARE_QUEUE_LIST_COUNT;i++)
	{
		if( m_cArrSystemQueueList[i].bActive == false )
		{
			*ppcBufferAddress = &m_cArrSystemQueueList[i].unArrBuffer[0];
			*psQueueIndex = i;
			m_cArrSystemQueueList[i].bActive = true;
			return true;
		}
	}
	printf("%s] ERROR Share Queue was FULL\n", __FUNCTION__);
	printf("%s] ERROR Share Queue was FULL\n", __FUNCTION__);
	printf("%s] ERROR Share Queue was FULL\n", __FUNCTION__);
	printf("%s] ERROR Share Queue was FULL\n", __FUNCTION__);
	*psQueueIndex = -1;
	return false;
}
void ClearMessageShareQueueIndex(int16_t sQueueIndex)
{
	if( sQueueIndex >=0 && sQueueIndex < MAX_MESSAGE_SHARE_QUEUE_LIST_COUNT )
	{
		m_cArrSystemQueueList[sQueueIndex].bActive = false;
		memset(m_cArrSystemQueueList[sQueueIndex].unArrBuffer,0,sizeof(stCarReport));
	}
	else
	{
		printf("%s] ERROR Share Queue was FULL\n", __FUNCTION__);
		printf("%s] ERROR Share Queue was FULL\n", __FUNCTION__);
		printf("%s] ERROR Share Queue was FULL\n", __FUNCTION__);
		printf("%s] ERROR Share Queue was FULL\n", __FUNCTION__);
	}
}
#endif//#if !defined(GLOBAL_SHARE_QUEUE)
