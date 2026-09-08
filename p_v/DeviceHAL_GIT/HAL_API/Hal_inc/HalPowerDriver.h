#ifndef __HAL_POWER_DRIVER_H__
#define __HAL_POWER_DRIVER_H__


#include "common.h"

/* Exported define -----------------------------------------------------------*/
#define HAL_WAKEUP_PIN                  GPIO_WAKEUP

#define BKRAM_MAX           4095 //Byte

//190905   0 ~ 383byte 까지는 사용중
//여기서부터는 주소 역산으로 데이터 사용
#define BKRAM_SW_RESET_SIGNAL_SIZE		4
#define BKRAM_SW_RESET_SIGNAL_ADDR		BKRAM_MAX - BKRAM_SW_RESET_SIGNAL_SIZE	//4091

#define BKRAM_ODOMETER_SIZE				4
#define BKRAM_ODOMETER_ADDR				BKRAM_SW_RESET_SIGNAL_ADDR - BKRAM_ODOMETER_SIZE	//4087

#define BKRAM_CLEARODO_FLAG_SIZE		4
#define BKRAM_CLEARODO_FLAG_ADDR		BKRAM_ODOMETER_ADDR - BKRAM_CLEARODO_FLAG_SIZE	//4083

#define BKRAM_RTC_SET_SIGNAL_FLAG_SIZE  4
#define BKRAM_RTC_SET_SIGNAL_FLAG_ADDR  BKRAM_CLEARODO_FLAG_ADDR - BKRAM_RTC_SET_SIGNAL_FLAG_SIZE //4079
#define BKRAM_RESETCOUNT_SIZE			2
#define BKRAM_RESETCOUNT_ADDR			BKRAM_RTC_SET_SIGNAL_FLAG_ADDR - BKRAM_RESETCOUNT_SIZE		//4077
#define BKRAM_3G4GFLAG_SIZE				2
#define BKRAM_3G4GFLAG_ADDR				BKRAM_RESETCOUNT_ADDR - BKRAM_3G4GFLAG_SIZE		

// @defgroup PWR_Flag 
#define HAL_PWR_FLAG_WU                 ((uint32_t)0x00000001)/*!< Wakeup Flag         */     
#define HAL_PWR_FLAG_SB                 ((uint32_t)0x00000002)/*!< Standby Flag      */     
#define HAL_PWR_FLAG_PVDO               ((uint32_t)0x00000004)/*!< PVD Output      */
#define HAL_PWR_FLAG_BRR                ((uint32_t)0x00000008)/*!< Backup regulator ready    */
#define HAL_PWR_FLAG_VOSRDY             ((uint32_t)0x00004000)/*!< Regulator voltage scaling output selection ready */
#define HAL_PWR_FLAG_ODRDY              ((uint32_t)0x00010000)/*!< Over Drive generator ready*/
#define HAL_PWR_FLAG_ODSWRDY            ((uint32_t)0x00020000)/*!< Over Drive Switch ready*/   
#define HAL_PWR_FLAG_UDRDY              ((uint32_t)0x000C0000)/*!< Under Drive ready     */

// @defgroup PWR_STOP_mode_entry 
#define HAL_PWR_STOPEntry_WFI               ((uint8_t)0x01)
#define HAL_PWR_STOPEntry_WFE               ((uint8_t)0x02)
// @defgroup PWR_Regulator_state_in_STOP_mode 
#define HAL_PWR_Regulator_ON                    ((uint32_t)0x00000000)
#define HAL_PWR_Regulator_LowPower              ((uint32_t)0x00000001)
//#define HAL_PWR_MainRegulator_UnderDrive_ON     ((uint32_t)0x00000800)
//#define HAL_PWR_LowPowerRegulator_UnderDrive_ON ((uint32_t)0x00000401)

/* Exported types - Structure, Enumeration -----------------------------------*/
typedef enum {HAL_WKUP_COLDBOOT = 0, HAL_WKUP_WARMBOOT} eHalPwr_WakeUpType;
typedef enum {HAL_WKUPPIN_INT = 0, HAL_WKUPPIN_WKUP} eHalPwr_WakeUpMode;

typedef enum __eHalPwrIoMode{
    ePWR_IO_GetFlagStatus,
    ePWR_IO_SetFlagStatus,
    ePWR_IO_ClearItStatus,
    ePWR_IO_WakeupPinEnable,
    ePWR_IO_BK_PwAccessEnable,
    ePWR_IO_BK_PwDomain_Reset,
    ePWR_IO_PWR_EnterStandbyMode,
    ePWR_IO_PWR_EnterStandbySystemOnlyMode,
    ePWR_IO_PWR_EnterStopMode,
}eHalPwr_IOCtlMode;

/* Exported constants --------------------------------------------------------*/
/* Exported macro & function prototypes --------------------------------------*/

int HalDrvPowerOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvPowerRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvPowerWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvPowerIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvPowerClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);


void HalDrvPwr_WkupPin_Config(eHalPwr_WakeUpMode eState);
void HalDrvPower_ClearStandbyFlag(void);
eHalPwr_WakeUpType HalDrvPower_GetStandbyFlag(void);
void HalDrvPower_SYSCLKConfig_STOP(void);

unsigned short HalDrvRdBkRam(unsigned char *pucBuf, unsigned short usIndex, unsigned short usLen);
unsigned short HalDrvWrBkRam(unsigned char *pucBuf, unsigned short usIndex, unsigned short usLen);

#endif //__HAL_POWER_DRIVER_H__
