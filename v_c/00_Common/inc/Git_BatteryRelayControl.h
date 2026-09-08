
#ifndef __GIT_BATTERYRELAYCONTROL_H__
#define __GIT_BATTERYRELAYCONTROL_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include <string.h>

/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/
#define MESSAGE_BATRELAYCON_QUEUE_SIZE		1000

#define CRC16_POLY							0x1021
#define CRC16_INIT_VALUE					0xFFFF
#define CRC16_ADD_VALUE						0xF800

#define MOSA_RX_TIMEOUT						55
#define MOSA_RX_CANID_SIZE					2
#define MOSA_RX_CANLENTH_SIZE				1

#define NE_PRECHRGSTA_CANID					0x0235
#define NE_MAINRLYONSTA_CNAID				0x02FA
#define NE_BATPACP_VOLT_DATA_SIZE			2
#define NE_BATPACP_VOLT_DATA_POSITION		13
#define NE_BATPACP_VOLT_FACTOR				0.1
#define NE_CHRG_STATE_DATA_SIZE				1
#define NE_CHRG_STATE_DATA_POSITION			3

#define OS_MONITORING_CANID					0x0595
#define OS_BATPACP_VOLT_DATA_SIZE			2
#define OS_BATPACP_VOLT_DATA_POSITION		6
#define OS_BATPACP_VOLT_FACTOR				0.1


/*----------------------------------------------------------------------
 *   struck & enum
 *--------------------------------------------------------------------*/
typedef enum {
	BatRelayConStatus_None = 0,
	BatRelayConStatus_Init,
	BatRelayConStatus_Run,
	BatRelayConStatus_Idle,
	BatRelayConStatus_Stop,
	BatRelayConStatus_Exit,
	BatRelayConStatus_Max
}eBatRelayConStatus;

typedef enum {
	BatRelayConType_None = 0,
	BatRelayConType_NEEV,
	BatRelayConType_OSEV,
	BatRelayConType_Max
}eBatRelayConType;

typedef struct {
	char* pcPreData;
	char* pcData;
	uint16_t unCycle;
	char cAliveCnt;
	uint16_t usCanId;
	uint16_t usCanLen;
}stBatRelayConData;

/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
bool StartBatRelayConThread(void);
void BatRelayConThread(void const *argument);
void BatRelayMonitorThread(void const *argument);
void BatRelayConEventListener();
void SendMSGToBatRelayCon(u16 Mode, stMsgClst *message);
unsigned short CalculateCRC16(char* pcData, unsigned int unLen);
void MakeNeevTxPacket(char* pcData, char* pcPreData, unsigned short usCanId, char cCnt);
void MakeOsEvTxPacket(char* pcData, unsigned short usCanId);
void BatRelayCon_WriteCanPacket(stBatRelayConData* pstData, uint16_t unTime, eBatRelayConType eType, uint16_t unCount);
void OSEV_SendRelayStop();
unsigned short ByteSwap(unsigned short usData);



/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/




#endif // __GIT_BATTERYRELAYCONTROL_H__

