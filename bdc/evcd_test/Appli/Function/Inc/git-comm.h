/* Includes ------------------------------------------------------------*/
#include "../Inc/sys-common.h"

/* Variables -----------------------------------------------------------*/

/* Functions -----------------------------------------------------------*/

// Task Functions
extern void InitCommTask(void);
extern void StartCommTask(void *argument);
extern bool COMM_IsBTConnected(void);

// UART1 Polling Mode Functions 
extern unsigned int UART1_Transmit_Polling(const uint8_t *pData, uint16_t Size);
extern unsigned int UART1_Receive_Interrupt(uint8_t *pData, uint16_t Size);
extern void UART1_Init_IT(void);
extern unsigned int UART1_read_from_ring(uint8_t *buf, unsigned int maxlen);

// UART2 Polling Mode Functions 
extern unsigned int UART2_Transmit_Polling(const uint8_t *pData, uint16_t Size);
extern unsigned int UART2_Receive_Interrupt(uint8_t *pData, uint16_t Size);  // Interrupt 기반
extern void UART2_Process_GitProtocol(void);
extern unsigned int UART2_read_from_ring(uint8_t *buf, unsigned int maxlen);
extern unsigned int UART2_read_buff(void);



// // UART1 Test Functions
// extern unsigned int UART1_write_buff_test(void);
// extern unsigned int UART1_read_buff_test(void);

// // UART2 Test Functions
// extern unsigned int UART2_write_buff_test(void);



