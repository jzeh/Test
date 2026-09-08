/*----------------------------------------------------------------------
 *   FDCAN Control
 *--------------------------------------------------------------------*/
#ifndef	__GIT_POOL_H__
#define	__GIT_POOL_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/
#define	CAN_PACKET_POOL_SIZE								300//100
#define COMM_PACKET_POOL_SIZE								5
#define MQTT_PACKET_POOL_SIZE								2
#define	MESSAGE_POOL_SIZE									1000
#define	SPI_CAN_PACKET_POOL_SIZE							300
#define OBD_ListDiag_END    								(1U << 0)
/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
extern int32_t	InitCanPool( void );
extern int32_t	InitSpiCanPool( void );
extern int32_t	InitCommPool( void );
extern int32_t	InitMessagePool( void );
extern int32_t	InitEthPool( void );
extern int32_t	InitDiagPool( void );
extern int32_t	InitSemaphore(void);
extern int32_t	InitEventFlags(void);
/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/
extern osPoolId			hFdcanPktPool;
extern osPoolId			hCommPKPool;
extern osPoolId			hMqttPKPool;
extern osPoolId			hMsgPool;
extern osPoolId			hTCPPktPool;
extern osPoolId			hTCPMsgPool;
// spi Can
extern osPoolId			hSpiCanPktPool;
extern osSemaphoreId	hMqttPKPoolSemaphore;
extern osSemaphoreId	hpairflagSemaphore;

#ifdef OS_POOL_ID_PRINT
typedef struct _stOS_POOL_ID_INFO
{
	uint32_t uiID;
	uint8_t ucIdName[15];
}stOS_POOL_ID_INFO;
char* GetOsIDName(uint32_t uiIndex);
#endif

#ifdef PRINT_MESSAGE_ID
typedef struct _stMESSAGE_ID_INFO
{
	uint32_t uiID;
	uint8_t ucIdName[25];
}stMESSAGE_ID_INFO;
char* GetMessageIDName(uint32_t uiIndex);
#endif

#endif // __GIT_POOL_H__