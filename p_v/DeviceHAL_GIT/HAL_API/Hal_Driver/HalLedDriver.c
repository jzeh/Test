/*
  ******************************************************************************
  * @file    HalLedDriver.c
  * @author  James Jean
  * @version V1.0.0
  * @date    2022-03-02
  * @brief
  *
  *
  ******************************************************************************
*/
#include "HalLedDriver.h"
#include "GIT_BluetoothLowEnergy.h"

/* Includes ------------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
int g_nBTTransmitLedDelayCount = 0;
int g_nCANTransmitLedDelayCount = 0;
int g_nGPSTransmitLedDelayCount = 0;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/


void SetLedOnOff(BOOL bOn, UINT uiGPIONum)
{
	if ( bOn )
		HalGPIOSetVaule(uiGPIONum, TRUE); // ON
	else
		HalGPIOSetVaule(uiGPIONum, FALSE); // off
}

void SetLedOnOffCtl(BOOL bOn, eLEDType eLedType)
{
	if(eLedType & eLED_GPS)
		SetLedOnOff(bOn, GPIO_GPS_LED);

	if(eLedType & eLED_SERVER)
		SetLedOnOff(bOn, GPIO_LTE_LED);

	if(eLedType & eLED_CAN)
		SetLedOnOff(bOn, GPIO_CAN_LED);
}

void SetBTTransmitLedOnOff()
{
	static BOOL bToggle = LED_ON;
	BOOL bBTConnectStatus = FALSE;

	bBTConnectStatus = BTGetConnectStatus();

	if ( bBTConnectStatus == FALSE ) {
		SetLedOnOffCtl(LED_OFF, eLED_SERVER);
	}
	else {
		if ( (g_nBTTransmitLedDelayCount != 0) ) {
			g_nBTTransmitLedDelayCount--;
			if ( g_nBTTransmitLedDelayCount == 0 ) {
				if ( bToggle == LED_ON )
					bToggle = LED_OFF;
				else
					bToggle = LED_ON;
			}
			SetLedOnOffCtl(bToggle, eLED_SERVER);
		}
		else {
			if ( BTGetConnectStatus() )
				SetLedOnOffCtl(LED_ON, eLED_SERVER);
		}
	}
}

void SetCANTransmitLedOnOff()
{
	static BOOL bToggle = LED_ON;

	if ( (g_nCANTransmitLedDelayCount != 0) ) {
		g_nCANTransmitLedDelayCount--;
		if ( g_nCANTransmitLedDelayCount == 0 ) {
			if ( bToggle == LED_ON )
				bToggle = LED_OFF;
			else
				bToggle = LED_ON;
		}
	}

	SetLedOnOffCtl(bToggle, eLED_CAN);
}

void SetGPSTransmitLedOnOff()
{
	static BOOL bToggle = LED_ON;

	if ( (g_nGPSTransmitLedDelayCount != 0) )
	{
		g_nGPSTransmitLedDelayCount--;
		if ( g_nGPSTransmitLedDelayCount == 0 ) {
			if ( bToggle == LED_ON )
				bToggle = LED_OFF;
			else
				bToggle = LED_ON;
		}
	}

	SetLedOnOffCtl(bToggle, eLED_GPS);
}

void SetALLTransmitLedOnOff()
{
	static BOOL bToggle = LED_ON;

	if ( (g_nCANTransmitLedDelayCount != 0) ) {
		g_nCANTransmitLedDelayCount--;
		if ( g_nCANTransmitLedDelayCount == 0 ) {
			if ( bToggle == LED_ON )
				bToggle = LED_OFF;
			else
				bToggle = LED_ON;
		}
	}

	SetLedOnOffCtl(bToggle, eLED_ALL);
}

void SetALLLedOnOff()
{
	static BOOL bToggle = LED_ON;	
	
	if ( bToggle == LED_ON )
		bToggle = LED_OFF;
	else
		bToggle = LED_ON;
		
	SetLedOnOffCtl(bToggle, eLED_ALL);
}

//---------------------------------------------------------------------------------//
int HalDrvLedOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return 0;
}

int HalDrvLedRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return 0;
}

int HalDrvLedWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return 0;
}

int HalDrvLedIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return 0;
}

int HalDrvLedClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
	SetLedOnOffCtl(LED_OFF, eLED_GPS);
	SetLedOnOffCtl(LED_OFF, eLED_SERVER);
	SetLedOnOffCtl(LED_OFF, eLED_CAN);

    return 0;
}

