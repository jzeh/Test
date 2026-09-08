/**
******************************************************************************
* @file    HalHandler.c
* @author  HyeonKeol.Moon
* @version V1
* @date    1-Feb-2019

* @Editor  James jean
* @version V2
* @date    28-Feb-2022
* @Modify  1. Search is unnecessarily modified in registed handlers

* @brief   Main program body
******************************************************************************

******************************************************************************
*/
/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "HalHandler.h"
#if !defined(FEATURE_BOOTLOADER)
#include "GIT_Gps.h"
#include "GIT_OemInterface.h"
#include "Modem_Manager.h"
#endif


/* Definiton -----------------------------------------------------------------*/
#define NOT_REGISTERED_ID_VALUE (0x00)

/* Exported constants --------------------------------------------------------*/

/* Internal functions ------------------------------------------------------- */
/*
int HalDriverRegister(int nDrvId, stHalInterfaceFunc funcs);
int HalDriverUnregister(int nDrvId);
*/
#if !defined(FEATURE_BOOTLOADER)
extern MODEM_MANAGER_DATA ModemManagerData;
#endif

stHalInterface g_tbHalInterface[MAX_HAL_INTERFACE] = { 
#if defined(FEATURE_BOOTLOADER)
    {DRV_ID_MISC,HalDrvMiscOpen, HalDrvMiscClose, HalDrvMiscRead, 
                        HalDrvMiscWrite, HalDrvMiscIOCtrl, NULL/*HalDrvMiscEvent*/},
    {DRV_ID_RCC, HalDrvRccOpen, HalDrvRccClose, HalDrvRccRead, 
                        HalDrvRccWrite, HalDrvRccIOCtrl, NULL/*HalDrvRccEvent*/},
    {DRV_ID_TIMER, HalDrvTimerOpen, HalDrvTimerClose, HalDrvTimerRead, 
                        HalDrvTimerWrite, HalDrvTimerIOCtrl, NULL/*HalDrvTimerEvent*/},
    {DRV_ID_GPIO, HalDrvGpioOpen, HalDrvGpioClose, HalDrvGpioRead, 
                        HalDrvGpioWrite, HalDrvGpioIOCtrl, NULL/*HalDrvGpioEvent*/},
    {DRV_ID_UART, HalDrvUartOpen, HalDrvUartClose, HalDrvUartRead, 
                       HalDrvUartWrite, HalDrvUartIOCtrl, NULL/*HalDrvUartEvent*/},
    {DRV_ID_FLASH, HalDrvFlashOpen, HalDrvFlashClose, HalDrvFlashRead, 
                       HalDrvFlashWrite, HalDrvFlashIOCtrl, NULL/*HalDrvFlashEvent*/},
    {DRV_ID_LED, HalDrvLedOpen, HalDrvLedClose, HalDrvLedRead, 
                       HalDrvLedWrite, HalDrvLedIOCtrl, NULL/*HalDrvLedEvent*/},
#else
    {DRV_ID_MISC,HalDrvMiscOpen, HalDrvMiscClose, HalDrvMiscRead, 
                        HalDrvMiscWrite, HalDrvMiscIOCtrl, NULL/*HalDrvMiscEvent*/},
    {DRV_ID_RCC, HalDrvRccOpen, HalDrvRccClose, HalDrvRccRead, 
                        HalDrvRccWrite, HalDrvRccIOCtrl, NULL/*HalDrvRccEvent*/},
    {DRV_ID_TIMER, HalDrvTimerOpen, HalDrvTimerClose, HalDrvTimerRead, 
                        HalDrvTimerWrite, HalDrvTimerIOCtrl, NULL/*HalDrvTimerEvent*/},
    {DRV_ID_POWER, HalDrvPowerOpen, HalDrvPowerClose, HalDrvPowerRead, 
                       HalDrvPowerWrite, HalDrvPowerIOCtrl, NULL/*HalDrvPowerEvent*/},
    {DRV_ID_GPIO, HalDrvGpioOpen, HalDrvGpioClose, HalDrvGpioRead, 
                        HalDrvGpioWrite, HalDrvGpioIOCtrl, NULL/*HalDrvGpioEvent*/},
    {DRV_ID_UART, HalDrvUartOpen, HalDrvUartClose, HalDrvUartRead, 
                       HalDrvUartWrite, HalDrvUartIOCtrl, NULL/*HalDrvUartEvent*/},
    {DRV_ID_RTC, HalDrvRtcOpen, HalDrvRtcClose, HalDrvRtcRead, 
                       HalDrvRtcWrite, HalDrvRtcIOCtrl, NULL/*HalDrvRtcEvent*/},
    {DRV_ID_FLASH, HalDrvFlashOpen, HalDrvFlashClose, HalDrvFlashRead, 
                       HalDrvFlashWrite, HalDrvFlashIOCtrl, NULL/*HalDrvFlashEvent*/},
    {DRV_ID_SPI, HalDrvSPIOpen, HalDrvSPIClose, HalDrvSPIRead, 
                       HalDrvSPIWrite, HalDrvSPIIOCtrl, NULL/*HalDrvSPIEvent*/},
    {DRV_ID_I2C, HalDrvI2COpen, HalDrvI2CClose, HalDrvI2CRead, 
                       HalDrvI2CWrite, HalDrvI2CIOCtrl, NULL/*HalDrvI2CEvent*/},
    {DRV_ID_ADC, HalDrvAdcOpen, HalDrvAdcClose, HalDrvAdcRead, 
                       HalDrvAdcWrite, HalDrvAdcIOCtrl, NULL/*HalDrvAdcEvent*/},
    {DRV_ID_LED, HalDrvLedOpen, HalDrvLedClose, HalDrvLedRead, 
                       HalDrvLedWrite, HalDrvLedIOCtrl, NULL/*HalDrvLedEvent*/},
    {DRV_ID_CAN, HalDrvCanOpen, HalDrvCanClose, HalDrvCanRead, 
                       HalDrvCanWrite, HalDrvCanIOCtrl, NULL/*HalDrvCanEvent*/},
    {DRV_ID_EXTI, HalDrvExIntOpen, HalDrvExIntClose, HalDrvExIntRead, 
                       HalDrvExIntWrite, HalDrvExIntIOCtrl, NULL/*HalDrvCanEvent*/},
#if defined(FEATURE_USE_USB_DRIVE)
    {DRV_ID_USB, HalDrvUsbOpen, HalDrvUsbClose, HalDrvUsbRead, 
                       HalDrvUsbWrite, HalDrvUsbIOCtrl, NULL/*HalDrvUsbEvent*/},
#endif
#endif //defined(FEATURE_BOOTLOADER)
};


int HalHandlerInit()
{
    int i;
 
    for ( i=0; i<MAX_HAL_INTERFACE; i++ )
    {
        if( g_tbHalInterface[i].interfaceFunc.Open != NULL )
            g_tbHalInterface[i].interfaceFunc.Open(0, 0, NULL, 0, 0);
    }

    return HAL_RETURN_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////////
// Hal GPIO API 
BOOL HalGPIOGetStatus(uint32_t nGPio_Pin)
{
    return HalDrvGpioRead(nGPio_Pin, 0, NULL, 0, 0);
}

void HalGPIOSetVaule(uint32_t nGPio_Pin, bool bHigh)
{
    HalDrvGpioWrite(nGPio_Pin, bHigh, NULL, 0, 0);
}


//////////////////////////////////////////////////////////////////////////////////
// Hal RTC API 

/**
  * @brief  Display the current time.
  * @param  None
  * @retval None
  */
void APP_TimeShow(stHalRTCTypeDef* pstHalRtc)
{   
    eHalReturnStatus eRetStatus;

    eRetStatus = (eHalReturnStatus)HalDrvRtcRead(eRtcBin, eRtcAll, (char*)pstHalRtc, sizeof(stHalRTCTypeDef), 0);

    if ( eRetStatus == HAL_RETURN_SUCCESS )
    {
        printf(" - RTC_Date/Time(UTC) %04d-%02d-%02d(WeekDay %d) %02d:%02d:%02d(RTC AM/PM %d)\r\n", 
            pstHalRtc->RtcDate.RTC_Year+2000,
            pstHalRtc->RtcDate.RTC_Month,
            pstHalRtc->RtcDate.RTC_Date,
            pstHalRtc->RtcDate.RTC_WeekDay,
            pstHalRtc->RtcTime.RTC_Hours,
            pstHalRtc->RtcTime.RTC_Minutes,
            pstHalRtc->RtcTime.RTC_Seconds, 
            pstHalRtc->RtcTime.RTC_H12);

    }
}
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Hal GPS API 

int HalHandlerDeInit()
{
    int i;
 
    for ( i=MAX_HAL_INTERFACE-1; i>=0; i-- )
    {
        if( g_tbHalInterface[i].interfaceFunc.Close != NULL )
            g_tbHalInterface[i].interfaceFunc.Close(0, 0, NULL, 0, 0);
    }

    return HAL_RETURN_SUCCESS;
}

#if !defined(FEATURE_BOOTLOADER)
void HalAPI_ShowGpsInfoEvery5Sec()
{
    stHalRTCTypeDef stHalRtcDateTime;
    static unsigned long m_ulShowTime = 0;
    unsigned long ulDiffTmr;
    
	ulDiffTmr = OemGetTmrDelta(OemGetTmr(), m_ulShowTime);

    if ( ulDiffTmr > 5000 )
    {
        // 네트워크도 출력    
        GITDebugPrintf("\r\n==================================================================\r\n");
        APP_TimeShow(&stHalRtcDateTime); // 시간 출력 
        GITDebugPrintf(" - GPS lat : %f, lon : %f, valid : %d, inuse : %d\r\n", 
                    Get_GPS_Lat(), Get_GPS_Lon(), g_GPSInfo.sig, g_GPSInfo.satinfo.inuse);
        GITDebugPrintf(" - RSSI: %d, %d dBm\r\n", ModemManagerData.nRSSI, ModemManagerData.ndBm);
        GITDebugPrintf("==================================================================\r\n\r\n");

        m_ulShowTime = OemGetTmr();
    }
}
#endif