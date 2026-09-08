/**
  ******************************************************************************
  * @file    GIT_OemInterface.h
  * @author  GIT Application Team by james jean
  * @version V 1.0
  * @date    18-MAR-2014
  * @brief   Header for GIT_OemInterface.h module
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GIT_OEM_INTERFACE_H__
#define __GIT_OEM_INTERFACE_H__

#include "common.h"
#include "HalHandler.h"

void GITDebugPrintf(char *fmt,...);

#define OemWriteUartModemBuff 		OemWriteUart2Buff
#if defined(FEATURE_USE_UART_RX_DMA)
#define OemReadUartModemBuff 		OemReadUartDMA2Buff
#else
#define OemReadUartModemBuff 		OemReadUart2Buff
#endif
#define OemParsingModemUart 		OemParsingUart2

#define OemWriteUartBTBuff 			OemWriteUart3Buff
#if defined(FEATURE_USE_UART_RX_DMA)
#define OemReadUartBTBuff 			OemReadUartDMA3Buff
#else
#define OemReadUartBTBuff 			OemReadUart3Buff
#endif
#define OemParsingBTUart 			OemParsingUart3

#define OemWriteUartGPSBuff 		OemWriteUart4Buff
#if defined(FEATURE_USE_UART_RX_DMA)
#define OemReadUartGPSBuff 			OemReadUartDMA4Buff
#else
#define OemReadUartGPSBuff 			OemReadUart4Buff
#endif
#define OemParsingGPSUart 			OemParsingUart4


#define OemWriteUartSelftestBuff 		OemWriteUart8Buff
#if defined(FEATURE_USE_UART_RX_DMA)
#define OemReadUartSelftestBuff 		OemReadUartDMA8Buff
#else
#define OemReadUartSelftestBuff 		OemReadUart8Buff
#endif
#define OemParsingSelftestUart 			OemParsingUart8

//#define OemWriteUartPLUSBuff 		OemWriteUart8Buff
//#define OemReadUartPLUSBuff 		OemReadUart8Buff
//#define OemParsingPLUSUart 			OemParsingUart8


#define	Oem_GIT_mDelay(a)				APP_Delay(a)				// blocking delay

////////////////////////////////////////////////////////////////////////////////
unsigned int OemWriteUart1Buff(unsigned char* pBuff, unsigned int 	nCount, void* lParam, unsigned int wParam);
unsigned int OemReadUart1Buff(unsigned char* pBuff, unsigned int 	nCount, void* lParam, unsigned int wParam);
unsigned int OemParsingUart1(unsigned char* pBuff, unsigned int 	nCount, void* lParam, unsigned int wParam);

unsigned int OemWriteUart2Buff(unsigned char* pBuff, unsigned int 	nCount, void* lParam, unsigned int wParam);
unsigned int OemReadUart2Buff(unsigned char* pBuff, unsigned int 	nCount, void* lParam, unsigned int wParam);
unsigned int OemReadUartDMA2Buff(unsigned char* pBuff, unsigned int 	nCount, void* lParam, unsigned int wParam);
unsigned int OemParsingUart2(unsigned char* pBuff, unsigned int 	nCount, void* lParam, unsigned int wParam);

unsigned int OemWriteUart3Buff(unsigned char* pBuff, unsigned int 	nCount, void* lParam, unsigned int wParam);
unsigned int OemReadUart3Buff(unsigned char* pBuff, unsigned int 	nCount, void* lParam, unsigned int wParam);
unsigned int OemReadUartDMA3Buff(unsigned char* pBuff, unsigned int 	nCount, void* lParam, unsigned int wParam);
unsigned int OemParsingUart3(unsigned char* pBuff, unsigned int 	nCount, void* lParam, unsigned int wParam);

unsigned int OemWriteUart4Buff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
unsigned int OemReadUart4Buff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
unsigned int OemReadUartDMA4Buff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
unsigned int OemParsingUart4(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);

unsigned int OemWriteUart8Buff(unsigned char* pBuff, unsigned int 	nCount, void* lParam, unsigned int wParam);
unsigned int OemReadUart8Buff(unsigned char* pBuff, unsigned int 	nCount, void* lParam, unsigned int wParam);
unsigned int OemReadUartDMA8Buff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
unsigned int OemParsingUart8(unsigned char* pBuff, unsigned int 	nCount, void* lParam, unsigned int wParam);

unsigned int OemReadCanBuff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
unsigned int OemClearCanBuff(unsigned int wParam);
unsigned int OemParsingCan(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
unsigned int OemWriteCanBuff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
unsigned int OemWriteUsbBuff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
unsigned int OemReadUsbBuff(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
unsigned int OemParsingUsb(unsigned char* pBuff, unsigned int nCount, void* lParam, unsigned int wParam);
unsigned int OemCheckUsbReadBuff(void);
#endif
// Concern abort GPIO

//-----------------------------Can1
void Oem_CAN1_STANDBY_INACTIVE(void);
void Oem_CAN1_STANDBY_ACTIVE(void);

void Oem_CAN1_SEL_HIGH_CAN1(void);
void Oem_CAN1_SEL_HIGH_CAN2(void);

//-----------------------------Can2
void Oem_CAN2_STANDBY_ACTIVE(void);
void Oem_CAN2_HIGH_CAN3_ENABLE(void);
void Oem_CAN2_HIGH_CAN3_DISABLE(void);
void Oem_CAN2_LOW_CAN_ENABLE(void);
void Oem_CAN2_LOW_CAN_DISABLE(void);
void Oem_CAN2_SEL_HIGH_CAN3(void);
void Oem_CAN2_SEL_LOW_CAN(void);


////////////////////////////////////////////////////////////////////////////////
// Concern abort Functions
void OemGetRTCTime(unsigned char* pBuff);
void Oem_GIT_uDelay(const uint32_t usec);
void Oem_GIT_mDelay(const uint32_t msec);
unsigned long OemGetTmr(void);
unsigned long OemGetTmrDelta(unsigned int uiNewTick, unsigned int uiOldTick);

////////////////////////////////////////////////////////////////////////////////
// CAN
unsigned char Oem_CAN_Channel_Masket_Set(unsigned char nChannel, unsigned char nCANIDType, unsigned char nMaskNum, unsigned int *pStartMaskValue, unsigned int *pEndMaskValue);
void Oem_CAN_Channel_Initialize(unsigned char usCanChannel, unsigned char ucComPortRelay,  unsigned long usCanBPS);
void Oem_CAN_Initial_CH1(unsigned char usHighCan1, unsigned char usCan1BPS);
void Oem_CAN_Initial_CH2(unsigned char usHighCan2, unsigned char usCan2BPS);
void Oem_CAN_None_Receive_Set();
bool PassCanIDCheck(stHalCanRxMsg rxmsg);
bool PassCanFDData(unsigned short usInputID , uint16_t usCanLine);

////////////////////////////////////////////////////////////////////////////////



unsigned long Oem_GetBattVoltage(char adcx);

#endif

/***************************** END OF FILE ****/
