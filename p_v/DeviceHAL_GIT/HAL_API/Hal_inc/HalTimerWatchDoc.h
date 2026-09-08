#ifndef __HAL_TIMER_WATCH_DOC_H__
#define __HAL_TIMER_WATCH_DOC_H__

#include "common.h"
/* Exported define -----------------------------------------------------------*/
// @defgroup IWDG_WriteAccess
#define HAL_IWDG_WriteAccess_Enable     ((uint16_t)0x5555)
#define HAL_IWDG_WriteAccess_Disable    ((uint16_t)0x0000)
// @defgroup IWDG_prescaler 
#define HAL_IWDG_Prescaler_4            ((uint8_t)0x00)
#define HAL_IWDG_Prescaler_8            ((uint8_t)0x01)
#define HAL_IWDG_Prescaler_16           ((uint8_t)0x02)
#define HAL_IWDG_Prescaler_32           ((uint8_t)0x03)
#define HAL_IWDG_Prescaler_64           ((uint8_t)0x04)
#define HAL_IWDG_Prescaler_128          ((uint8_t)0x05)
#define HAL_IWDG_Prescaler_256          ((uint8_t)0x06)

/* Exported types - Structure, Enumeration -----------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro & function prototypes --------------------------------------*/

extern void Set_Wdt(u16 mscnt);
extern u8 Chk_WdtReset(void);
extern void SetWDTReset(uint16_t msDelay);


#endif //__HAL_TIMER_WATCH_DOC_H__
