#ifndef __HAL_HANDLER__H__
#define __HAL_HANDLER__H__
/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "common.h"

#if defined(STM32F427X)
#include "STM32F4xx.h"
#elif defined(AT32F435VMT7)
#include "AT32F435_437.h"
#include "at32f435_437_i2c.h"
#endif

#include "HalRccDriver.h"
#include "HalAdcDriver.h"
#include "HalCanDriver.h"
#include "HalFlashDriver.h"
#include "HalGpioDriver.h"
#include "HalI2CDriver.h"
#include "HalLedDriver.h"
#include "HalPowerDriver.h"
#include "HalRtcDriver.h"
#include "HalSPIDriver.h"
#include "HalMiscDriver.h"
#include "HalUartDriver.h"
#include "HalExIntDriver.h"
#include "HalDmaDriver.h"
#include "HalTimerDriver.h"
#include "HalTimerWatchDoc.h"
//#include "HalUSBDriver.h"



/* Definiton -----------------------------------------------------------------*/
#if defined(FEATURE_BOOTLOADER)

#define DRV_ID_MISC				(0x00100000)    // NVIC
#define DRV_ID_RCC				(0x00100001)
#define DRV_ID_TIMER            (0x00100002)    // HW & SW timer
#define DRV_ID_GPIO				(0x00100003)
#define DRV_ID_UART				(0x00100004)
#define DRV_ID_FLASH			(0x00100005)
#define DRV_ID_LED              (0x00100006)
#define MAX_HAL_INTERFACE       ((DRV_ID_LED&(0x0000000F))+1)

#else

#define DRV_ID_MISC				(0x00100000)    // NVIC
#define DRV_ID_RCC				(0x00100001)
#define DRV_ID_TIMER            (0x00100002)    // HW & SW timer
#define DRV_ID_POWER			(0x00100003)
#define DRV_ID_GPIO				(0x00100004)
#define DRV_ID_UART				(0x00100005)
#define DRV_ID_RTC				(0x00100006)
#define DRV_ID_FLASH			(0x00100007)
#define DRV_ID_SPI				(0x00100008)
#define DRV_ID_I2C				(0x00100009)
#define DRV_ID_ADC			    (0x0010000A)
#define DRV_ID_LED              (0x0010000B)
#define DRV_ID_CAN              (0x0010000C)
#define DRV_ID_EXTI             (0x0010000D)

#if defined(FEATURE_USE_USB_DRIVE)
#define DRV_ID_USB  			(0x0010000E)
#define MAX_HAL_INTERFACE       ((DRV_ID_USB&(0x0000000F))+1)
#else
#define MAX_HAL_INTERFACE       ((DRV_ID_EXTI&(0x0000000F))+1)
#endif

#endif  // defined(FEATURE_BOOTLOADER)


/* Exported types - Structure, Enumeration -----------------------------------*/

typedef int (*fpDrvfp)(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);

typedef __packed struct _stHalInterfaceFunc {
	fpDrvfp     Open;
	fpDrvfp     Close;
	fpDrvfp     Write;
	fpDrvfp     Read;
	fpDrvfp     IOCtrl;
	fpDrvfp     CallBackEvt;
}stHalInterfaceFunc;

typedef __packed struct _stHalInterface {
	int nDrvID;
	stHalInterfaceFunc interfaceFunc;
}stHalInterface;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/


/* Exported functions ------------------------------------------------------- */

int HalHandlerInit();
int HalHandlerDeInit();


//////////////////////////////////////////////////////////////////////////////////
// Hal GPIO API 
//Direct GPIO functions
BOOL HalGPIOGetStatus(uint32_t nGPio_Pin);
void HalGPIOSetVaule(uint32_t nGPio_Pin, bool bHigh);
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
// Hal RTC API 
void APP_TimeShow(stHalRTCTypeDef* pstHalRtc);
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
// Hal GPS API 
void HalAPI_ShowGpsInfoEvery5Sec();
//////////////////////////////////////////////////////////////////////////////////

#endif  //__HAL_HANDLER__H__

