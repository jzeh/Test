#ifndef __GIT_FUNCTIONLIST_H
#define __GIT_FUNCTIONLIST_H

/* Includes  -----------------------------------------------------------*/
#include "sys-common.h"


/* Variables  ----------------------------------------------------------*/

/* Typedef  ----------------------------------------------------------*/

// Function Def
typedef void	(*pfnCommandLoadCB)( void *pInterPtcl, uint32_t uiInCommType, uint32_t uiLength  );

typedef struct
{
	uint32_t			uiFunctionID;
	pfnCommandLoadCB	fnPayloadCB;
} stFunctionList;


/*
	Protocol Def
*/
/********************************************************************************************************/
/*   Frame Struct                                                                                       */
/********************************************************************************************************/
/*    |     0    |   1~2   |     3     |     ...   |  n-3~n-2  |    n-1    |           */
/*    |    SOF   | Len(2B) |   FuncID  |  Payload  |  CRC16(2B)|    EOF    |           */
/********************************************************************************************************/
/*   SOF : 0xA0   EOF : 0xB0                                                                            */
/*   Len : 2 bytes (Little Endian), 0x0000 ~ 0xFFFF                                                     */
/*   Len = FuncID(1) + Payload(n) + CRC16(2) + EOF(1) = n + 4                                           */
/*   CRC16 : CRC-16/IBM (poly=0xA001 reflected), 계산 범위 = SOF ~ Payload (CRC 앞까지 전체)            */

#define GITPACKET_LEN_MIN				0x0000	// FUNCID + PAYLOAD + CRC16 + EOF
#define GITPACKET_LEN_MAX				0xFFFF
#define GITPACKET_PAYLOAD_LEN_MIN		0x00
#define GITPACKET_PAYLOAD_LEN_MAX		0x200	// 512 bytes
#define GITPACKET_RESPONSE_LEN			0x05	// FUNCID + ACK/NAK + CRC16(2) + EOF

#define GITPACKET_SOF					0xA0
#define GITPACKET_EOF					0xB0
#define GITPACKET_ACK					0x00
#define GITPACKET_NAK					0x01

#define GITPACKET_SOF_IDX				0
#define GITPACKET_LEN_IDX				1		// 2 bytes LE (Low byte at [1], High byte at [2])
#define GITPACKET_FUNCID_IDX			3
#define GITPACKET_PAYLOAD_IDX			4

typedef struct _stProtocolFunc
{
	uint32_t 		uiFuncID;
	uint32_t 		uiDataLen;
	char 			cPayloadData[GITPACKET_PAYLOAD_LEN_MAX];
	uint32_t 		ucChecksum;
} stProtocolFunc;




/* Functions -----------------------------------------------------------*/
// Function List
extern stFunctionList g_Functions_GDS[];
extern stFunctionList g_Functions_PLC[];
extern stFunctionList g_Functions_LCD[];

// 0x51 periodic TX (main loop 또는 task에서 주기적으로 호출)
extern void FL_GDS_Display_Data_Periodic(void);
extern bool g_bDisplayActive;

// 0x53 VCI3 수집 데이터
extern uint8_t g_vci3_soc;

/* 0x81 Error Code periodic TX
 *  g_error_code != 0 이고 BT 연결된 경우,
 *  앱 요청 없이도 주기적으로 에러 코드를 앱으로 전송 (comm task loop에서 호출)
 */
extern void    FL_GDS_Send_Error_Code(void);
extern uint8_t g_error_code;

/* 0x91 STOP과 동일한 정지 시퀀스를 팝업 없이 수행 */
extern void FL_GDS_StopDevice_WithoutPopup(void);

/* R/Y/G LED 상태(3초 cooldown 타이머, sticky 플래그)는 led-indicator.h 로 통합됨 */


// 2) PLC Protocol Functions
// ...
// 3) LCD Protocol Functions


#endif /* __GIT_FUNCTIONLIST_H */
