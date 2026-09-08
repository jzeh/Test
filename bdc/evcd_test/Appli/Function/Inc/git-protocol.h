/* Includes  -----------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h> 

/* Defines   -----------------------------------------------------------*/

/********************************************************************************************************/
/*   Frame Struct (git-functionlist.h 기반)                                                            */
/********************************************************************************************************/
/*         0    |   1~2   |    3    |   4~n   |  n+1~n+2 |    n+3                                     */
/*        SOF   | Len(2B) |  FuncID | Payload |  CRC16(2B)|    EOF                                     */
/********************************************************************************************************/
/*   SOF : 0xA0   EOF : 0xB0                                                                            */
/*   Len : 2 bytes Little Endian, FuncID(1) + Payload(0~N) + CRC16(2) + EOF(1) = N + 4                 */
/*   CRC16 : CRC-16/IBM (poly=0xA001 reflected), 계산 범위 = SOF ~ Payload (CRC 앞까지 전체)           */
/*   Total Frame Size = SOF(1) + Len(2) + Len_value = 3 + Len_value                                    */
/********************************************************************************************************/

#define	MAX_INTER_PROTO_DATA_LENGTH						4200
#define	GITPACKET_FRAME_SIZE_MIN						7				// SOF + Len(2) + FuncID + CRC16(2) + EOF
#define	GITPACKET_FRAME_SIZE_MAX						519				// SOF + Len(2) + FuncID + Payload(512) + CRC16(2) + EOF

// git pool
#define	CAN_PACKET_POOL_SIZE								300
#define COMM_PACKET_POOL_SIZE								5
#define	MESSAGE_POOL_SIZE									1000

#define MESSAGE_PARSING_QUEUE_SIZE							COMM_PACKET_POOL_SIZE
#define MESSAGE_TRANSMIT_QUEUE_SIZE							100

#define FDCAN_PACKET_MAX_SIZE						        64


/* Variables  ----------------------------------------------------------*/



/* Typedef  ----------------------------------------------------------*/



/* Functions -----------------------------------------------------------*/

/* Protocol Frame Functions (git-functionlist.h 기반) */
int GITPACKET_make_frame(uint16_t FunctionID, uint8_t *pPayload, uint16_t PayloadLen, uint8_t *pFrameBuff);
int GITPACKET_parse_frame(uint8_t *pFrameBuff, uint16_t FrameSize, uint8_t *pFuncID, uint8_t **ppPayload, uint16_t *pPayloadLen);
int GITPACKET_make_response(uint8_t FunctionID, uint8_t isAck, uint8_t *pFrameBuff);
int GITPACKET_send_response(uint8_t FunctionID, uint8_t isAck);

/* UART Transmit Functions */
HAL_StatusTypeDef GITPACKET_send_frame_via_uart(uint8_t *pFrameData, uint16_t frameSize);
HAL_StatusTypeDef GITPACKET_send_frame_via_uart_hex(uint8_t *pFrameData, uint16_t frameSize);

/* Test Functions */
void GITPACKET_test_example(void);
void GITPACKET_test_transmit(void);
void GITPACKET_test_receive(void);

/* Task Functions */
void GITPACKET_transmit(void);
void InitGitProtocolTasks(void);
