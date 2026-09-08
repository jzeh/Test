#ifndef __HAL_GPIO_DRIVER_H__
#define __HAL_GPIO_DRIVER_H__


/* Includes ------------------------------------------------------------------*/
#include "common.h"

/* Exported define -----------------------------------------------------------*/

#define GPIOA_GROUP     ((unsigned int)0x00010000)
#define GPIOB_GROUP     ((unsigned int)0x00020000)
#define GPIOC_GROUP     ((unsigned int)0x00040000)
#define GPIOD_GROUP     ((unsigned int)0x00080000)
#define GPIOE_GROUP     ((unsigned int)0x00100000)
#define GPIO_ALL_GROUP  (GPIOA_GROUP|GPIOB_GROUP|GPIOC_GROUP|GPIOD_GROUP|GPIOE_GROUP)

#define HAL_GPIO_PIN_0                 ((unsigned short)0x0001)  /* Pin 0 selected */
#define HAL_GPIO_PIN_1                 ((unsigned short)0x0002)  /* Pin 1 selected */
#define HAL_GPIO_PIN_2                 ((unsigned short)0x0004)  /* Pin 2 selected */
#define HAL_GPIO_PIN_3                 ((unsigned short)0x0008)  /* Pin 3 selected */
#define HAL_GPIO_PIN_4                 ((unsigned short)0x0010)  /* Pin 4 selected */
#define HAL_GPIO_PIN_5                 ((unsigned short)0x0020)  /* Pin 5 selected */
#define HAL_GPIO_PIN_6                 ((unsigned short)0x0040)  /* Pin 6 selected */
#define HAL_GPIO_PIN_7                 ((unsigned short)0x0080)  /* Pin 7 selected */
#define HAL_GPIO_PIN_8                 ((unsigned short)0x0100)  /* Pin 8 selected */
#define HAL_GPIO_PIN_9                 ((unsigned short)0x0200)  /* Pin 9 selected */
#define HAL_GPIO_PIN_10                ((unsigned short)0x0400)  /* Pin 10 selected */
#define HAL_GPIO_PIN_11                ((unsigned short)0x0800)  /* Pin 11 selected */
#define HAL_GPIO_PIN_12                ((unsigned short)0x1000)  /* Pin 12 selected */
#define HAL_GPIO_PIN_13                ((unsigned short)0x2000)  /* Pin 13 selected */
#define HAL_GPIO_PIN_14                ((unsigned short)0x4000)  /* Pin 14 selected */
#define HAL_GPIO_PIN_15                ((unsigned short)0x8000)  /* Pin 15 selected */
#define HAL_GPIO_PIN_All               ((unsigned short)0xFFFF)  /* All pins selected */


// @ GPIO_Alternat_function_selection_define 
// @brief   AF 0 selection  
#define HAL_GPIO_AF_RTC_50Hz      ((unsigned char)0x00)  /* RTC_50Hz Alternate Function mapping */
#define HAL_GPIO_AF_MCO           ((unsigned char)0x00)  /* MCO (MCO1 and MCO2) Alternate Function mapping */
#define HAL_GPIO_AF_TAMPER        ((unsigned char)0x00)  /* TAMPER (TAMPER_1 and TAMPER_2) Alternate Function mapping */
#define HAL_GPIO_AF_SWJ           ((unsigned char)0x00)  /* SWJ (SWD and JTAG) Alternate Function mapping */
#define HAL_GPIO_AF_TRACE         ((unsigned char)0x00)  /* TRACE Alternate Function mapping */
// @brief   AF 1 selection  
#define HAL_GPIO_AF_TIM1          ((unsigned char)0x01)  /* TIM1 Alternate Function mapping */
#define HAL_GPIO_AF_TIM2          ((unsigned char)0x01)  /* TIM2 Alternate Function mapping */
//@brief   AF 2 selection  
#define HAL_GPIO_AF_TIM3          ((unsigned char)0x02)  /* TIM3 Alternate Function mapping */
#define HAL_GPIO_AF_TIM4          ((unsigned char)0x02)  /* TIM4 Alternate Function mapping */
#define HAL_GPIO_AF_TIM5          ((unsigned char)0x02)  /* TIM5 Alternate Function mapping */
// @brief   AF 3 selection  
#define HAL_GPIO_AF_TIM8          ((unsigned char)0x03)  /* TIM8 Alternate Function mapping */
#define HAL_GPIO_AF_TIM9          ((unsigned char)0x03)  /* TIM9 Alternate Function mapping */
#define HAL_GPIO_AF_TIM10         ((unsigned char)0x03)  /* TIM10 Alternate Function mapping */
#define HAL_GPIO_AF_TIM11         ((unsigned char)0x03)  /* TIM11 Alternate Function mapping */
// @brief   AF 4 selection  
#define HAL_GPIO_AF_I2C1          ((unsigned char)0x04)  /* I2C1 Alternate Function mapping */
#define HAL_GPIO_AF_I2C2          ((unsigned char)0x04)  /* I2C2 Alternate Function mapping */
#define HAL_GPIO_AF_I2C3          ((unsigned char)0x04)  /* I2C3 Alternate Function mapping */
// @brief   AF 5 selection  
#define HAL_GPIO_AF_SPI1          ((unsigned char)0x05)  /* SPI1/I2S1 Alternate Function mapping */
#define HAL_GPIO_AF_SPI2          ((unsigned char)0x05)  /* SPI2/I2S2 Alternate Function mapping */
#define HAL_GPIO_AF5_SPI3         ((unsigned char)0x05)  /* SPI3/I2S3 Alternate Function mapping (Only for STM32F411xE Devices) */
#define HAL_GPIO_AF_SPI4          ((unsigned char)0x05)  /* SPI4/I2S4 Alternate Function mapping */
#define HAL_GPIO_AF_SPI5          ((unsigned char)0x05)  /* SPI5 Alternate Function mapping      */
#define HAL_GPIO_AF_SPI6          ((unsigned char)0x05)  /* SPI6 Alternate Function mapping      */
// @brief   AF 6 selection  
#define HAL_GPIO_AF_SPI3          ((unsigned char)0x06)  /* SPI3/I2S3 Alternate Function mapping */
#define HAL_GPIO_AF6_SPI1         ((unsigned char)0x06)  /* SPI1 Alternate Function mapping (Only for STM32F410xx Devices) */
#define HAL_GPIO_AF6_SPI2         ((unsigned char)0x06)  /* SPI2 Alternate Function mapping (Only for STM32F410xx/STM32F411xE Devices) */
#define HAL_GPIO_AF6_SPI4         ((unsigned char)0x06)  /* SPI4 Alternate Function mapping (Only for STM32F411xE Devices) */
#define HAL_GPIO_AF6_SPI5         ((unsigned char)0x06)  /* SPI5 Alternate Function mapping (Only for STM32F410xx/STM32F411xE Devices) */
#define HAL_GPIO_AF_SAI1          ((unsigned char)0x06)  /* SAI1 Alternate Function mapping      */
// @brief   AF 7 selection  
#define HAL_GPIO_AF_USART1         ((unsigned char)0x07)  /* USART1 Alternate Function mapping  */
#define HAL_GPIO_AF_USART2         ((unsigned char)0x07)  /* USART2 Alternate Function mapping  */
#define HAL_GPIO_AF_USART3         ((unsigned char)0x07)  /* USART3 Alternate Function mapping  */
#define HAL_GPIO_AF7_SPI3          ((unsigned char)0x07)  /* SPI3/I2S3ext Alternate Function mapping */
// @brief   AF 7 selection Legacy 
#define HAL_GPIO_AF_I2S3ext         GPIO_AF7_SPI3
// @brief   AF 8 selection  
#define HAL_GPIO_AF_UART4         ((unsigned char)0x08)  /* UART4 Alternate Function mapping  */
#define HAL_GPIO_AF_UART5         ((unsigned char)0x08)  /* UART5 Alternate Function mapping  */
#define HAL_GPIO_AF_USART6        ((unsigned char)0x08)  /* USART6 Alternate Function mapping */
#define HAL_GPIO_AF_UART7         ((unsigned char)0x08)  /* UART7 Alternate Function mapping  */
#define HAL_GPIO_AF_UART8         ((unsigned char)0x08)  /* UART8 Alternate Function mapping  */
// @brief   AF 9 selection 
#define HAL_GPIO_AF_CAN1          ((unsigned char)0x09)  /* CAN1 Alternate Function mapping  */
#define HAL_GPIO_AF_CAN2          ((unsigned char)0x09)  /* CAN2 Alternate Function mapping  */
#define HAL_GPIO_AF_TIM12         ((unsigned char)0x09)  /* TIM12 Alternate Function mapping */
#define HAL_GPIO_AF_TIM13         ((unsigned char)0x09)  /* TIM13 Alternate Function mapping */
#define HAL_GPIO_AF_TIM14         ((unsigned char)0x09)  /* TIM14 Alternate Function mapping */
#define HAL_GPIO_AF9_I2C2         ((unsigned char)0x09)  /* I2C2 Alternate Function mapping (Only for STM32F401xx/STM32F410xx/STM32F411xE Devices) */
#define HAL_GPIO_AF9_I2C3         ((unsigned char)0x09)  /* I2C3 Alternate Function mapping (Only for STM32F401xx/STM32F411xE Devices) */
// @brief   AF 10 selection  
#define HAL_GPIO_AF_OTG_FS         ((unsigned char)0xA)  /* OTG_FS Alternate Function mapping */
#define HAL_GPIO_AF_OTG_HS         ((unsigned char)0xA)  /* OTG_HS Alternate Function mapping */
// @brief   AF 11 selection  
#define HAL_GPIO_AF_ETH             ((unsigned char)0x0B)  /* ETHERNET Alternate Function mapping */
// @brief   AF 12 selection  
#define HAL_GPIO_AF_FMC              ((unsigned char)0xC)  /* FMC Alternate Function mapping                      */
#define HAL_GPIO_AF_OTG_HS_FS        ((unsigned char)0xC)  /* OTG HS configured in FS, Alternate Function mapping */
#define HAL_GPIO_AF_SDIO             ((unsigned char)0xC)  /* SDIO Alternate Function mapping                     */
// @brief   AF 13 selection  
#define HAL_GPIO_AF_DCMI          ((unsigned char)0x0D)  /* DCMI Alternate Function mapping */
// @brief   AF 14 selection  
#define HAL_GPIO_AF_LTDC          ((unsigned char)0x0E)  /* LCD-TFT Alternate Function mapping */
// @brief   AF 15 selection  
#define HAL_GPIO_AF_EVENTOUT      ((unsigned char)0x0F)  /* EVENTOUT Alternate Function mapping */

//-----------------------------------------------------------------------------------------

//**********************************************************************
// PA GROUP
//**********************************************************************
#define GPIO_WAKEUP			(GPIOA_GROUP|HAL_GPIO_PIN_0)
#define GPIO_MODEM_SLEEP	(GPIOA_GROUP|HAL_GPIO_PIN_1)
#define GPIO_ADC_BAT		(GPIOA_GROUP|HAL_GPIO_PIN_2)
#define GPIO_MCU_LNA_EN		(GPIOA_GROUP|HAL_GPIO_PIN_3)
#define GPIO_CAN_LED     	(GPIOA_GROUP|HAL_GPIO_PIN_4)
#define GPIO_LOW_CAN_RX_CTL	(GPIOA_GROUP|HAL_GPIO_PIN_5)
#define GPIO_GPS_LED     	(GPIOA_GROUP|HAL_GPIO_PIN_6)
#ifdef USEBUZZER
#define GPIO_BUZZER     	(GPIOA_GROUP|HAL_GPIO_PIN_7)
#endif
#define GPIO_LOW_CAN_NERR	(GPIOA_GROUP|HAL_GPIO_PIN_8)
#define GPIO_USB_FS_VBUS	(GPIOA_GROUP|HAL_GPIO_PIN_9)
#define GPIO_USB_FS_ID		(GPIOA_GROUP|HAL_GPIO_PIN_10)
#define GPIO_USB_FS_DM		(GPIOA_GROUP|HAL_GPIO_PIN_11)
#define GPIO_USB_FS_DP		(GPIOA_GROUP|HAL_GPIO_PIN_12)
#define GPIO_JTAG_TMS		(GPIOA_GROUP|HAL_GPIO_PIN_13)
#define GPIO_JTAG_TCK		(GPIOA_GROUP|HAL_GPIO_PIN_14)
#define GPIO_JTAG_TDI   	(GPIOA_GROUP|HAL_GPIO_PIN_15)

//**********************************************************************
// PB GROUP
//**********************************************************************
#define GPIO_GPS_PWEN	    		(GPIOB_GROUP|HAL_GPIO_PIN_0)
#define GPIO_IG_ON_DET	    		(GPIOB_GROUP|HAL_GPIO_PIN_1)
#define GPIO_BOOT1          		(GPIOB_GROUP|HAL_GPIO_PIN_2)
#define GPIO_JTAG_TDO       		(GPIOB_GROUP|HAL_GPIO_PIN_3)
#define GPIO_JTAG_TRST      		(GPIOB_GROUP|HAL_GPIO_PIN_4)
#define GPIO_LOW_HIGH_CAN_RX	    (GPIOB_GROUP|HAL_GPIO_PIN_5)
#define GPIO_I2C1_SCL		    	(GPIOB_GROUP|HAL_GPIO_PIN_6)
#define GPIO_I2C1_SDA	    		(GPIOB_GROUP|HAL_GPIO_PIN_7)
#define GPIO_BT_MON_CTL            	(GPIOB_GROUP|HAL_GPIO_PIN_8)
#define GPIO_SFLASH_WP            	(GPIOB_GROUP|HAL_GPIO_PIN_9)
#define GPIO_ACC_DET	    		(GPIOB_GROUP|HAL_GPIO_PIN_10)
#define GPIO_CAN_RX_CTL	    		(GPIOB_GROUP|HAL_GPIO_PIN_11)
#define GPIO_MO_WAKE_CTL	    	(GPIOB_GROUP|HAL_GPIO_PIN_12)
#define GPIO_LOW_HIGH_CAN_TX    	(GPIOB_GROUP|HAL_GPIO_PIN_13)
#define GPIO_LOW_HIGH_CAN_RX_MON    (GPIOB_GROUP|HAL_GPIO_PIN_14)
#define GPIO_CAN_RX_MON				(GPIOB_GROUP|HAL_GPIO_PIN_15)

//**********************************************************************
// PC GROUP
//**********************************************************************
#define GPIO_LATCH_CLK 				(GPIOC_GROUP|HAL_GPIO_PIN_0)
#define GPIO_MODEM_RST  			(GPIOC_GROUP|HAL_GPIO_PIN_1)
//#define GPIO_PC2  				(GPIOC_GROUP|HAL_GPIO_PIN_2)
#define GPIO_MO_WAKE  				(GPIOC_GROUP|HAL_GPIO_PIN_3)
#define GPIO_MCU_3V3        	    (GPIOC_GROUP|HAL_GPIO_PIN_4)
#define GPIO_SENSOR_INT				(GPIOC_GROUP|HAL_GPIO_PIN_5)
#define GPIO_SFLASH_RST	            (GPIOC_GROUP|HAL_GPIO_PIN_6)
#define GPIO_IG_ACC_DET_CTL  	    (GPIOC_GROUP|HAL_GPIO_PIN_7)
#define GPIO_LOW_CAN_NSTB			(GPIOC_GROUP|HAL_GPIO_PIN_8)
#define GPIO_LOW_CAN_EN				(GPIOC_GROUP|HAL_GPIO_PIN_9)
#define GPIO_GPS_TX					(GPIOC_GROUP|HAL_GPIO_PIN_10)
#define GPIO_GPS_RX					(GPIOC_GROUP|HAL_GPIO_PIN_11)
#define GPIO_BT_TX_DW				(GPIOC_GROUP|HAL_GPIO_PIN_12)
#define GPIO_MA_PWEN   				(GPIOC_GROUP|HAL_GPIO_PIN_13)

//**********************************************************************
// PD GROUP
//**********************************************************************
#define GPIO_CAN1_RX			    (GPIOD_GROUP|HAL_GPIO_PIN_0)
#define GPIO_CAN1_TX			    (GPIOD_GROUP|HAL_GPIO_PIN_1)
#define GPIO_BT_RX_DW				(GPIOD_GROUP|HAL_GPIO_PIN_2)
#define GPIO_MODEM_CTS				(GPIOD_GROUP|HAL_GPIO_PIN_3)
#define GPIO_MODEM_RTS				(GPIOD_GROUP|HAL_GPIO_PIN_4)
#define GPIO_MODEM_TX				(GPIOD_GROUP|HAL_GPIO_PIN_5)
#define GPIO_MODEM_RX				(GPIOD_GROUP|HAL_GPIO_PIN_6)
#define GPIO_POWER_KEY				(GPIOD_GROUP|HAL_GPIO_PIN_7)
#define GPIO_BT_TX  				(GPIOD_GROUP|HAL_GPIO_PIN_8)
#define GPIO_BT_RX  				(GPIOD_GROUP|HAL_GPIO_PIN_9)
#define GPIO_BT_STATUS				(GPIOD_GROUP|HAL_GPIO_PIN_10)
#define GPIO_BT_CTS  				(GPIOD_GROUP|HAL_GPIO_PIN_11)
#define GPIO_BT_RTS  				(GPIOD_GROUP|HAL_GPIO_PIN_12)
#define GPIO_BT_PWEN  				(GPIOD_GROUP|HAL_GPIO_PIN_13)
#define GPIO_BT_RESET  				(GPIOD_GROUP|HAL_GPIO_PIN_14)
#define GPIO_BT_WAKE                (GPIOD_GROUP|HAL_GPIO_PIN_15)

//**********************************************************************
// PE GROUP
//**********************************************************************
#define GPIO_INT_TX					(GPIOE_GROUP|HAL_GPIO_PIN_0)
#define GPIO_INT_RX		  			(GPIOE_GROUP|HAL_GPIO_PIN_1)
#define GPIO_SPI4_SCK				(GPIOE_GROUP|HAL_GPIO_PIN_2)
#define GPIO_LTE_LED     			(GPIOE_GROUP|HAL_GPIO_PIN_3)
#define GPIO_SPI4_NSS     		    (GPIOE_GROUP|HAL_GPIO_PIN_4)
#define GPIO_SPI4_MISO				(GPIOE_GROUP|HAL_GPIO_PIN_5)
#define GPIO_SPI4_MOSI				(GPIOE_GROUP|HAL_GPIO_PIN_6)
#define GPIO_DBG_RX    	    	    (GPIOE_GROUP|HAL_GPIO_PIN_7)
#define GPIO_DBG_TX    		        (GPIOE_GROUP|HAL_GPIO_PIN_8)
#define GPIO_HI_CAN2_CHK_CTL    	(GPIOE_GROUP|HAL_GPIO_PIN_9)
#define GPIO_LOW_HIGH_CAN_SEL   	(GPIOE_GROUP|HAL_GPIO_PIN_10)
#define GPIO_HIGH_CAN3_EN	        (GPIOE_GROUP|HAL_GPIO_PIN_11)
#define GPIO_ETC_PWEN   	        (GPIOE_GROUP|HAL_GPIO_PIN_12)
//#define GPIO_PE13			   	    (GPIO_Pin_13)	//
#define GPIO_CAN1_ENABLE   	        (GPIOE_GROUP|HAL_GPIO_PIN_14)
#define GPIO_BATT_CTL   	        (GPIOE_GROUP|HAL_GPIO_PIN_15)



/* Exported types - Structure, Enumeration -----------------------------------*/

typedef enum __eHalGpioBitAction{
    eBIT_RESET,
    eBIT_SET,
}eHalGpioBitAction;

typedef enum __eHalGpioMode{
    eGPIO_IO_INIT,
    eGPIO_IO_AF_MAPPING,
}eHalGpio_IOCtlMode;

typedef enum _eHalGPIOMode_TypeDef
{
  eGPIO_Mode_IN   = 0x00,   /*!< gpio input mode */
  eGPIO_Mode_OUT  = 0x01,   /*!< gpio output mode */
  eGPIO_Mode_AF   = 0x02,   /*!< gpio mux function mode */
  eGPIO_Mode_AN   = 0x03    /*!< gpio analog in/out mode */
}eHalGPIOMode_TypeDef;

typedef enum _eHalGPIODriveStrength_TypeDef
{
  eGPIO_DRIVE_STRENGTH_STRONGER  = 0x01, /*!< stronger sourcing/sinking strength */
  eGPIO_DRIVE_STRENGTH_MODERATE  = 0x02  /*!< moderate sourcing/sinking strength */
}eHalGPIO_DS_TypeDef;

typedef enum _eHalGPIOOutPut_TypeDef
{
  eGPIO_OType_PP = 0x00,    /*!< output push-pull */
  eGPIO_OType_OD = 0x01     /*!< output open-drain */
}eHalGPIOOutPut_TypeDef;

typedef enum _eHalGPIOPuPd_TypeDef
{
  eGPIO_PuPd_NOPULL = 0x00, /*!< floating for input, no pull for output */
  eGPIO_PuPd_UP     = 0x01, /*!< pull-up */
  eGPIO_PuPd_DOWN   = 0x02  /*!< pull-down */
}eHalGPIOPuPd_TypeDef;

typedef enum _eHalGPIOSpeed_TypeDef
{
  eGPIO_Low_Speed     = 0x00,
  eGPIO_Medium_Speed  = 0x01,
  eGPIO_Fast_Speed    = 0x02,
  eGPIO_High_Speed    = 0x03
}eHalGPIOSpeed_TypeDef;

#define  eGPIO_Speed_2MHz    eGPIO_Low_Speed
#define  eGPIO_Speed_25MHz   eGPIO_Medium_Speed
#define  eGPIO_Speed_50MHz   eGPIO_Fast_Speed
#define  eGPIO_Speed_100MHz  eGPIO_High_Speed

/*
typedef __packed struct __stGpioControl{
    unsigned short ucGPio_Pin;
    bool bHigh;
}stHalGpioControl;
*/

typedef __packed struct _stHalGPIO_InitTypeDef
{
  int GPIO_Pin;
  eHalGPIOMode_TypeDef             GPIO_Mode;
  eHalGPIO_DS_TypeDef              GPIO_DS;    // DriveStrength
  eHalGPIOSpeed_TypeDef            GPIO_Speed;
  eHalGPIOOutPut_TypeDef           GPIO_OType;
  eHalGPIOPuPd_TypeDef             GPIO_PuPd;
}stHalGPIO_InitTypeDef;


/* Exported constants --------------------------------------------------------*/

/* Exported macro & function prototypes --------------------------------------*/
unsigned char HalGPio_GetBoardID();

//////////////////////////////////////////////////////////////////////////////////////////
int HalDrvGpioOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvGpioRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvGpioWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvGpioIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvGpioClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);

void HalGpioInitMainPowerEnable(void);
void HalGpioGenerateLatchClock(void);

#endif //__HAL_GPIO_DRIVER_H__
