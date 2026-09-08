/*****************************************************************************
* @file    HalGpioDriver.c
* @author  James Jean
* @version V1
* @date    2022-03-02
* @brief   
******************************************************************************

******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "common.h"

#include "HalHandler.h"

/////////////////////////////////////////////////////////////////////////////
#include "GIT_OemInterface.h" 
#include "Autolink_Manager.h"
/////////////////////////////////////////////////////////////////////////////


#if defined(STM32F427X)
#include "STM32F4xx.h"
#include "STM32F4xx_gpio.h"
#include "STM32F4xx_exti.h"

#elif defined(AT32F435VMT7)
#include "at32f435_437.h"
#include "at32f435_437_gpio.h"
#include "at32f435_437_exint.h"


#endif

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define GET_GPIO_GROUP(PIN)        ((unsigned int)(PIN&0xFFFF0000))
#define GET_GPIO_PIN_NUM(PIN)      ((unsigned int)(PIN&0x0000FFFF))
#define GET_GPIO_PIN_CLK(PIN)      (GET_GPIO_GROUP(PIN)>>4)


/* Private macro -------------------------------------------------------------*/
/*inline*/ int GET_GPIO_PIN_SRC(unsigned int PIN) 
{
    int nBitCnt;
    int nPinNum = GET_GPIO_PIN_NUM(PIN);
    for (nBitCnt=0; nBitCnt<0x0F; nBitCnt++ )
    {                                        
        if ( (nPinNum>>nBitCnt) & 0x01 ) break;         
    }                                        
    return nBitCnt;                                 
}

/* Private variables ---------------------------------------------------------*/
__no_init bool g_bBTNotNeedtoInit;

/* Private function prototypes -----------------------------------------------*/
void HalGpioInit_Premium(void);


/* Private functions ---------------------------------------------------------*/


void HalGpioInitMainPowerEnable(void)
{
    stHalGPIO_InitTypeDef GPIO_InitStructure;

    // GPIO 클럭 인가
    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOC_GROUP, NULL, 0, HAL_ENABLE);
    HalGPIOSetVaule(GPIO_MA_PWEN, eBIT_SET);// 먼저 SET으로 적용할 것    

    GPIO_InitStructure.GPIO_Pin = GPIO_MA_PWEN;
    GPIO_InitStructure.GPIO_DS  = eGPIO_DRIVE_STRENGTH_STRONGER;
    GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_NOPULL;                // No Pull-up으로 설정하자.
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

    HalGPIOSetVaule(GPIO_MA_PWEN, eBIT_SET);
}

void HalGpioInit_Led(void)
{
	stHalGPIO_InitTypeDef  GPIO_InitStructure;

    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOA_GROUP | GPIOE_GROUP, NULL, 0, HAL_ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_LTE_LED;
    GPIO_InitStructure.GPIO_DS  = eGPIO_DRIVE_STRENGTH_STRONGER;
    GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);


	GPIO_InitStructure.GPIO_Pin = GPIO_CAN_LED;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	GPIO_InitStructure.GPIO_Pin = GPIO_GPS_LED;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
    
	SetLedOnOffCtl(LED_ON, eLED_GPS);
	SetLedOnOffCtl(LED_ON, eLED_SERVER);
	SetLedOnOffCtl(LED_ON, eLED_CAN);
	//	 변경사항- 현재 상태가 슬립때 ACC온하면 메인 LED가 안들어와서 LED다시 들어오게 롤백함 이작업으로 인해 1초웨이크업시 LED 깜빡하는 문제나옴
}

void HalGpioGenerateLatchClock(void)
{
	HalDrvGpioWrite(GPIO_LATCH_CLK, eBIT_SET, NULL, 0, 0);
    APP_Delay(2);
    HalDrvGpioWrite(GPIO_LATCH_CLK, eBIT_RESET, NULL, 0, 0);
    APP_Delay(2);
}


void HalGpioInit_Premium(void)
{
	stHalGPIO_InitTypeDef  GPIO_InitStructure;

    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIO_ALL_GROUP, NULL, 0, HAL_ENABLE);

	// Bluetooth State : connect or Disconnect
    GPIO_InitStructure.GPIO_Pin = GPIO_BT_STATUS;
    GPIO_InitStructure.GPIO_DS  = eGPIO_DRIVE_STRENGTH_STRONGER;
    GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_DOWN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

    g_bBTNotNeedtoInit = HalDrvGpioRead(GPIO_BT_STATUS, 0, NULL, 0, 0);

    
#ifdef BNCOM_TEMP //22.01.06 mod.kks todo check
    if(!g_bBTNotNeedtoInit) // Not Connect
   {        
        GPIO_InitStructure.GPIO_Pin = GPIO_BT_PWEN;
        GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
        GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
        GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

        HalDrvGpioWrite(GPIO_BT_PWEN, eBIT_RESET, NULL, 0, 0);
    }
#endif

	//********************************************************
	// PA7: MODEM Power Key
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_POWER_KEY;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

#ifdef RF_COMMON_MODEM
    HalDrvGpioWrite(GPIO_POWER_KEY, eBIT_SET, NULL, 0, 0);
    APP_Delay(100);
    HalDrvGpioWrite(GPIO_POWER_KEY, eBIT_RESET, NULL, 0, 0);
    APP_Delay(800);
	/*endif*/
#endif

    HalDrvGpioWrite(GPIO_POWER_KEY, eBIT_SET, NULL, 0, 0);
	//********************************************************

#ifdef BNCOM //mod.kks 21.01.06

	if(!g_bBTNotNeedtoInit) //mod.kks 22.01.10
	{
		// GPIO_BT_RESET
		GPIO_InitStructure.GPIO_Pin = GPIO_BT_RESET;
		GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
		GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
        HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

        HalDrvGpioWrite(GPIO_BT_RESET, eBIT_RESET, NULL, 0, 0);
	}
#endif

	//********************************************************
	// 처음부팅시 BT파워가 플로팅상태에서 오래있다가(RTC초기화가오래걸림)
	// BT Init에서 3.3V를 넣어주면 BT NOTI가 두번발생해서 처음부팅할때 강제로 Low로 땡겨줌
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_BT_PWEN;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
    HalDrvGpioWrite(GPIO_BT_PWEN, eBIT_SET, NULL, 0, 0);

	//********************************************************

	//********************************************************
	// PC0: GPIO_LATCH_CLK - output
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_LATCH_CLK;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	HalDrvGpioWrite(GPIO_LATCH_CLK, eBIT_RESET, NULL, 0, 0);
    
	//********************************************************

	//********************************************************
	// PA3: GPIO_MCU_LNA_EN - output
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_MCU_LNA_EN;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

    HalDrvGpioWrite(GPIO_MCU_LNA_EN, eBIT_SET, NULL, 0, 0);
    
	//********************************************************

	//********************************************************
	// initialize wakeup signal monitor gpio
	//********************************************************
	//********************************************************
	// PC3: GPIO_MO_WAKE - input
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_MO_WAKE;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_DOWN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
    
	//********************************************************

	//********************************************************
	// PB1: GPIO_IG_ON_DET - input
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_IG_ON_DET;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_DOWN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
    
	//********************************************************

	//********************************************************
	// PB10: GPIO_ACC_DET - input
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_ACC_DET;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_DOWN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
	//********************************************************

	//********************************************************
	// PB14: GPIO_LOW_HIGH_CAN_RX_MON - input
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_LOW_HIGH_CAN_RX_MON;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_DOWN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
	//********************************************************

	//********************************************************
	// PB15: GPIO_CAN_RX_MON - input
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_CAN_RX_MON;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_DOWN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
	//********************************************************

	//********************************************************
	// Latch 제어로 동작 가능한 포트
	//********************************************************
	//********************************************************
	// PA5: GPIO_LOW_CAN_RX_CTL - output
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_LOW_CAN_RX_CTL;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	HalDrvGpioWrite(GPIO_LOW_CAN_RX_CTL, eBIT_RESET, NULL, 0, 0);
    //********************************************************

	//********************************************************
	// PB8: GPIO_BT_MON_CTL - output
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_BT_MON_CTL;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	HalDrvGpioWrite(GPIO_BT_MON_CTL, eBIT_RESET, NULL, 0, 0);
	//********************************************************

	//********************************************************
	// PB11: GPIO_CAN_RX_CTL - output
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_CAN_RX_CTL;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	HalDrvGpioWrite(GPIO_CAN_RX_CTL, eBIT_RESET, NULL, 0, 0);
	//********************************************************

	//********************************************************
	// PB11: GPIO_MO_WAKE_CTL - output
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_MO_WAKE_CTL;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	HalDrvGpioWrite(GPIO_MO_WAKE_CTL, eBIT_RESET, NULL, 0, 0);
	//********************************************************

	//********************************************************
	// PC7: GPIO_IG_ACC_DET_CTL - output
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_IG_ACC_DET_CTL;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	HalDrvGpioWrite(GPIO_IG_ACC_DET_CTL, eBIT_RESET, NULL, 0, 0);
	//********************************************************

	//********************************************************
	// PE15: GPIO_BATT_CTL - output (기능 사용안함)
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_BATT_CTL;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_DOWN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	HalDrvGpioWrite(GPIO_BATT_CTL, eBIT_SET, NULL, 0, 0);
	//********************************************************

	//********************************************************
	// SN74LVC374 latch에 clock을 인가한다.	Dx --> Qx 출력됨.
	//********************************************************
	HalGpioGenerateLatchClock();
	//********************************************************

#if !defined(FEATURE_BOOTLOADER)
	//********************************************************
	// U202: BL8568
	// 4.0V --> 3.3V
	//********************************************************
	// OUTPUT - 3.3V 전원인가 용도(HIGH)
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_ETC_PWEN;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	HalDrvGpioWrite(GPIO_ETC_PWEN, eBIT_SET, NULL, 0, 0);
	//********************************************************

	//********************************************************
	// OUTPUT - GPS POWER ENABLE Port
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_GPS_PWEN;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	//**************************************************
	// GPS Power On
	//**************************************************
	HalDrvGpioWrite(GPIO_GPS_PWEN, eBIT_SET, NULL, 0, 0);    // GPS Power On
	

	//********************************************************

	//********************************************************
	// 모뎀제어 WAKE - high,  SLEEP  - low
	//********************************************************
    GPIO_InitStructure.GPIO_Pin = GPIO_MODEM_SLEEP;
    GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

    HalDrvGpioWrite(GPIO_MODEM_SLEEP, eBIT_SET, NULL, 0, 0);
	//********************************************************

	//********************************************************
	// 모뎀제어 RST
	//********************************************************
    GPIO_InitStructure.GPIO_Pin = GPIO_MODEM_RST;
    GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	if(AutoLinkManagerData.eSystemResetMode == eSYSTEM_RESET_MODE_POWER_ON) {
		// modem rst 단에 NPN TR이 적용되어있다. 따라서, High/Low 신호를 반대로 해야 한다.
		// uart드라이버 로딩 이후 프린트를 해야한다.
//		printf(" Modem H/W Reset\n");
//		printf("PowerOnGemaltoModem_Modem_Reset_Rst@@");
		HalDrvGpioWrite(GPIO_MODEM_RST, eBIT_RESET, NULL, 0, 0);
#ifdef RF_COMMON_MODEM  //mod.kks 21.10.25
		APP_Delay(200);
#else
		APP_Delay(5);
#endif
		HalDrvGpioWrite(GPIO_MODEM_RST, eBIT_SET, NULL, 0, 0);

#ifdef RF_COMMON_MODEM  //mod.kks 21.10.25
		APP_Delay(200);
#else
		APP_Delay(5);
#endif
		HalDrvGpioWrite(GPIO_MODEM_RST, eBIT_RESET, NULL, 0, 0);
	}
	else {
	}
	//********************************************************
#endif

#if defined(FEATURE_EXTENSION_BOARD)
    GPIO_InitStructure.GPIO_Pin = GPIO_USB_FS_DM|GPIO_USB_FS_DP;
    GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
#endif
}

#if !defined(FEATURE_BOOTLOADER)
void HalGpioInit_Can(void)
{
	stHalGPIO_InitTypeDef GPIO_InitStructure;

    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOE_GROUP , NULL, 0, HAL_ENABLE);

	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_NOPULL;

	//*************************************************************************
	// CAN1: CAN1에는 HIGH CAN1 또는 HIGH CAN2가 연결 된다.
	//*************************************************************************
	//*************************************************************************
	// PE14: GPIO_CAN1_ENABLE, enable/disable
	//*************************************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_CAN1_ENABLE;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	Oem_CAN1_STANDBY_ACTIVE();				// standby mode, disable CAN1 input
	//*************************************************************************

	//********************************************************
	// PE9: GPIO_HI_CAN2_CHK_CTL - output
	//********************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_HI_CAN2_CHK_CTL;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	HalDrvGpioWrite(GPIO_HI_CAN2_CHK_CTL, eBIT_RESET, NULL, 0, 0);
	//********************************************************

	//*************************************************************************
	// CAN2: CAN2에는 LOW CAN 또는 HIGH CAN3가 연결 된다.
	//*************************************************************************
	//*************************************************************************
	// GPIO_LOW_CAN_NSTB
	//*************************************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_LOW_CAN_NSTB;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	HalDrvGpioWrite(GPIO_LOW_CAN_NSTB, eBIT_RESET, NULL, 0, 0);	// LOW CAN Stanby
	//*************************************************************************

	//*************************************************************************
	// GPIO_LOW_CAN_EN
	//*************************************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_LOW_CAN_EN;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	HalDrvGpioWrite(GPIO_LOW_CAN_EN, eBIT_RESET, NULL, 0, 0);
	//*************************************************************************

	//*************************************************************************
	// GPIO_LOW_CAN_NERR
	//*************************************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_LOW_CAN_NERR;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_DOWN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	//*************************************************************************
	// PE11: GPIO_HIGH_CAN3_EN - output
	//*************************************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_HIGH_CAN3_EN;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	HalDrvGpioWrite(GPIO_HIGH_CAN3_EN, eBIT_SET, NULL, 0, 0);		// Disable HIGH_CAN3
	//*************************************************************************

	//*************************************************************************
	// OUTPUT - LOW CAN / HIGH CAN3 선택 포트
	//*************************************************************************
	GPIO_InitStructure.GPIO_Pin = GPIO_LOW_HIGH_CAN_SEL;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

    HalDrvGpioWrite(GPIO_LOW_HIGH_CAN_SEL, eBIT_RESET, NULL, 0, 0);  // High: Input HIGH_CAN3
	//*************************************************************************

	//********************************************************
	// SN74LVC374 latch에 clock을 인가한다.	Dx --> Qx 출력됨.
	//********************************************************
	HalGpioGenerateLatchClock();				// GPIO_LOW_HIGH_CAN_SEL, GPIO_HI_CAN2_CHK_CTL 도 연결되어있음
	//********************************************************
}

void HalGpioInit_SFlash(void)
{
	stHalGPIO_InitTypeDef  GPIO_InitStructure;

    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOB_GROUP | GPIOC_GROUP, NULL, 0, HAL_ENABLE);

	// Serial Flash(SPI) Write Protect
	GPIO_InitStructure.GPIO_Pin = GPIO_SFLASH_WP;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	// Disable Write Protect
	HalDrvGpioWrite(GPIO_SFLASH_WP, eBIT_SET, NULL, 0, 0);

	// Serial Flash(SPI) Reset port
	GPIO_InitStructure.GPIO_Pin = GPIO_SFLASH_RST;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_UP;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	// Disable Write Protect
	HalDrvGpioWrite(GPIO_SFLASH_RST, eBIT_RESET, NULL, 0, 0);

	APP_Delay(5);

	HalDrvGpioWrite(GPIO_SFLASH_RST, eBIT_SET, NULL, 0, 0);
}

#endif

unsigned char HalGPio_GetBoardID()
{
    unsigned char ucBoardID = 0;

#if 0
#if( USE_BOARD_ID1 )
	ucBoardID  = HalGPIOGetStatus( BD_ID1_Pin ) << 1;
#endif
	ucBoardID += HalGPIOGetStatus( BD_ID0_Pin );

	switch( ucBoardID )
	{
		case 3 :			printf( "Board ID : 0x11\r\n" );			break;
		case 2 :			printf( "Board ID : 0x10\r\n" );			break;
		case 1 :			printf( "Board ID : 0x01\r\n" );			break;
		case 0 :
		default :			printf( "Board ID : 0x00\r\n" );			break;
	}
#endif
	return ucBoardID;
}

/////////////////////////////////////////////////////////////////////////////////////
int HalDrvGpioOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
  
    HalGpioInitMainPowerEnable();
    
#if !defined(FEATURE_BOOTLOADER)
    HalGpioInit_Led();
    HalGpioInit_SFlash();
    HalGpioInit_Premium();
	HalGpioInit_Can();
#endif

    return HAL_RETURN_SUCCESS;
}

// usb is based on interrupt
// so we implement this function with event
int HalDrvGpioRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    void *pGpioX = NULL;
    int nGROUP_PINNUM = nLparam;
    unsigned int unGpioGroup  = GET_GPIO_GROUP(nGROUP_PINNUM);
    unsigned int unGpioPinNum = GET_GPIO_PIN_NUM(nGROUP_PINNUM);

    if      ( unGpioGroup == GPIOA_GROUP )  pGpioX = GPIOA;
    else if ( unGpioGroup == GPIOB_GROUP )  pGpioX = GPIOB;
    else if ( unGpioGroup == GPIOC_GROUP )  pGpioX = GPIOC;
    else if ( unGpioGroup == GPIOD_GROUP )  pGpioX = GPIOD;
    else if ( unGpioGroup == GPIOE_GROUP )  pGpioX = GPIOE;
    else                                    return HAL_RETURN_FAIL;

#if defined(STM32F427X)
    return GPIO_ReadInputDataBit((GPIO_TypeDef*)pGpioX, unGpioPinNum);
#elif defined(AT32F435VMT7)
    return gpio_input_data_bit_read((gpio_type*)pGpioX, unGpioPinNum);
#endif
}

int HalDrvGpioWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    void *pGpioX = NULL;
    int nGROUP_PINNUM = nLparam;
    eHalGpioBitAction eBitAction = (eHalGpioBitAction)nRparam;
    unsigned int unGpioGroup  = GET_GPIO_GROUP(nGROUP_PINNUM);
    unsigned int unGpioPinNum = GET_GPIO_PIN_NUM(nGROUP_PINNUM);
    
    if      ( unGpioGroup == GPIOA_GROUP ) pGpioX = GPIOA;
    else if ( unGpioGroup == GPIOB_GROUP ) pGpioX = GPIOB;
    else if ( unGpioGroup == GPIOC_GROUP ) pGpioX = GPIOC;
    else if ( unGpioGroup == GPIOD_GROUP ) pGpioX = GPIOD;
    else if ( unGpioGroup == GPIOE_GROUP ) pGpioX = GPIOE;

#if defined(STM32F427X)
    GPIO_WriteBit((GPIO_TypeDef*)pGpioX, unGpioPinNum, (BitAction)eBitAction);
#elif defined(AT32F435VMT7)
    gpio_bits_write((gpio_type*)pGpioX, unGpioPinNum, (confirm_state)eBitAction);
#endif

    return HAL_RETURN_SUCCESS;
}

int HalDrvGpioIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    void *pGpioX = NULL;
    eHalGpio_IOCtlMode eIOMode = (eHalGpio_IOCtlMode)nLparam;

    switch( eIOMode )
    {
        case eGPIO_IO_INIT:
        {
            stHalGPIO_InitTypeDef *pGPIOInitType = (stHalGPIO_InitTypeDef*)pBuffer;
            unsigned int unGpioGroup  = GET_GPIO_GROUP(pGPIOInitType->GPIO_Pin);
            unsigned int unGpioPinNum = GET_GPIO_PIN_NUM(pGPIOInitType->GPIO_Pin);

            if      ( unGpioGroup == GPIOA_GROUP )  pGpioX = GPIOA;
            else if ( unGpioGroup == GPIOB_GROUP )  pGpioX = GPIOB;
            else if ( unGpioGroup == GPIOC_GROUP )  pGpioX = GPIOC;
            else if ( unGpioGroup == GPIOD_GROUP )  pGpioX = GPIOD;
            else if ( unGpioGroup == GPIOE_GROUP )  pGpioX = GPIOE;
            else                                    return HAL_RETURN_FAIL;
            
#if defined(STM32F427X)
            GPIO_InitTypeDef GPIO_InitStructure;
            GPIO_StructInit(&GPIO_InitStructure);
            GPIO_InitStructure.GPIO_Pin   = unGpioPinNum;
            GPIO_InitStructure.GPIO_Mode  = (GPIOMode_TypeDef)pGPIOInitType->GPIO_Mode;
            GPIO_InitStructure.GPIO_OType = (GPIOOType_TypeDef)pGPIOInitType->GPIO_OType;
            GPIO_InitStructure.GPIO_Speed = (GPIOSpeed_TypeDef)pGPIOInitType->GPIO_Speed;
            GPIO_InitStructure.GPIO_PuPd  = (GPIOPuPd_TypeDef)pGPIOInitType->GPIO_PuPd;
            GPIO_Init((GPIO_TypeDef*)pGpioX, &GPIO_InitStructure);

#elif defined(AT32F435VMT7)

            gpio_init_type GPIO_InitStructure;
            gpio_default_para_init(&GPIO_InitStructure);
            GPIO_InitStructure.gpio_pins  = unGpioPinNum;
            GPIO_InitStructure.gpio_mode = (gpio_mode_type)pGPIOInitType->GPIO_Mode;
            GPIO_InitStructure.gpio_out_type = (gpio_output_type)pGPIOInitType->GPIO_OType;
            GPIO_InitStructure.gpio_pull = (gpio_pull_type)pGPIOInitType->GPIO_PuPd;
            GPIO_InitStructure.gpio_drive_strength = (gpio_drive_type)pGPIOInitType->GPIO_DS;
            gpio_init((gpio_type*)pGpioX, &GPIO_InitStructure);
#endif
            }
            break;
        case eGPIO_IO_AF_MAPPING:
            {
                unsigned int unGpioAFSel  = nOverlap;
                unsigned int unGpioPinSrc = GET_GPIO_PIN_SRC(nRparam);
                unsigned int unGpioGroup  = GET_GPIO_GROUP(nRparam);
                
                if      ( unGpioGroup == GPIOA_GROUP )  pGpioX = GPIOA;
                else if ( unGpioGroup == GPIOB_GROUP )  pGpioX = GPIOB;
                else if ( unGpioGroup == GPIOC_GROUP )  pGpioX = GPIOC;
                else if ( unGpioGroup == GPIOD_GROUP )  pGpioX = GPIOD;
                else if ( unGpioGroup == GPIOE_GROUP )  pGpioX = GPIOE;
                else                                    return HAL_RETURN_FAIL;

#if defined(STM32F427X)
                GPIO_PinAFConfig((GPIO_TypeDef*)pGpioX, unGpioPinSrc, unGpioAFSel);
#elif defined(AT32F435VMT7)
                gpio_pin_mux_config((gpio_type*)pGpioX, 
                                    (gpio_pins_source_type)unGpioPinSrc, 
                                    (gpio_mux_sel_type)unGpioAFSel);
#endif
            }
            break;

        default:
            break;

    }

	return HAL_RETURN_SUCCESS;
}

int HalDrvGpioClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    stHalGPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_DS  = eGPIO_DRIVE_STRENGTH_STRONGER;
    GPIO_InitStructure.GPIO_OType = eGPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Mode  = eGPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = eGPIO_Low_Speed;

    // GPIO_BT_PWEN은 HW에서 잡고 있기 때문에 outpu 설정 불필요
    GPIO_InitStructure.GPIO_Pin   = GPIO_BT_PWEN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

    //open drain으로 설정 필요(PLS의 경우 "Do not add any voltage")
    GPIO_InitStructure.GPIO_Pin   = GPIO_POWER_KEY;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

    HalGPIOSetVaule(GPIO_USB_FS_VBUS, eBIT_RESET);
    HalGPIOSetVaule(GPIO_USB_FS_ID, eBIT_RESET);
    HalGPIOSetVaule(GPIO_USB_FS_DM, eBIT_RESET);
    HalGPIOSetVaule(GPIO_USB_FS_DP, eBIT_RESET);

    HalGPIOSetVaule(GPIO_MCU_LNA_EN, eBIT_RESET);

    // OUTPUT - 3.3V 전원인가 용도(HIGH)
#ifndef ENABLE_STANDBY_MODE
    HalGPIOSetVaule(GPIO_ETC_PWEN, eBIT_SET);
#else
    HalGPIOSetVaule(GPIO_ETC_PWEN, eBIT_RESET);
#endif

    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_RESET);

    HalGPIOSetVaule(GPIO_MODEM_SLEEP, eBIT_RESET);

    // HW에서 잡고 있기 때문에 설정 불필요(sleep시 ELS는 low, PLS는 high )
#ifdef RF_COMMON_MODEM // PLS
    HalGPIOSetVaule(GPIO_GPS_PWEN, eBIT_SET);	
    HalGPIOSetVaule(GPIO_BT_RESET, eBIT_SET);
#else // ELS
    HalGPIOSetVaule(GPIO_GPS_PWEN, eBIT_RESET);
    HalGPIOSetVaule(GPIO_BT_RESET, eBIT_RESET);
#endif

    return HAL_RETURN_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////


