#ifndef __HAL_LED_DRIVER_H__
#define __HAL_LED_DRIVER_H__

#include "common.h"
#include "HalHandler.h"
/* Exported define -----------------------------------------------------------*/
/* Exported types - Structure, Enumeration -----------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro & function prototypes --------------------------------------*/

#define LED_OFF				0
#define LED_ON				1

#if defined(NO_DELAY_SEND_TEST)
    #define MAX_BT_LED_DELAY_CNT 5
#else
    #define MAX_BT_LED_DELAY_CNT 1500
#endif

#define MAX_CAN_LED_DELAY_CNT 10

typedef enum _eLEDtype
{
	eLED_TYPE_NONE	= 0x0000,
	eLED_GPS		= 0x0001,	//GREEN
	eLED_SERVER		= 0x0002,	//RED
	eLED_CAN		= 0x0004,	//BLUE
	eLED_ALL		= 0xFFFF,
}eLEDType;



int HalDrvLedOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvLedRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvLedWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvLedIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvLedClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);

void SetLedOnOffCtl(BOOL bOn, eLEDType eLedType);
void SetLedOnOff(BOOL bOn, UINT uiGPIONum);

void SetALLTransmitLedOnOff();
void SetBTTransmitLedOnOff();
void SetCANTransmitLedOnOff();

void SetALLLedOnOff();

#endif //__HAL_LED_DRIVER_H__
