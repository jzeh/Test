#ifndef __HAL_EX_INT_DRIVER_H__
#define __HAL_EX_INT_DRIVER_H__


/* Includes ------------------------------------------------------------------*/
#include "common.h"

/* Exported define -----------------------------------------------------------*/

//---------------------------------------------------------------------------------------
// @defgroup EXTI_Lines 
#define HAL_EXTI_Line0       ((uint32_t)0x00001)     /*!< External interrupt line 0 */
#define HAL_EXTI_Line1       ((uint32_t)0x00002)     /*!< External interrupt line 1 */
#define HAL_EXTI_Line2       ((uint32_t)0x00004)     /*!< External interrupt line 2 */
#define HAL_EXTI_Line3       ((uint32_t)0x00008)     /*!< External interrupt line 3 */
#define HAL_EXTI_Line4       ((uint32_t)0x00010)     /*!< External interrupt line 4 */
#define HAL_EXTI_Line5       ((uint32_t)0x00020)     /*!< External interrupt line 5 */
#define HAL_EXTI_Line6       ((uint32_t)0x00040)     /*!< External interrupt line 6 */
#define HAL_EXTI_Line7       ((uint32_t)0x00080)     /*!< External interrupt line 7 */
#define HAL_EXTI_Line8       ((uint32_t)0x00100)     /*!< External interrupt line 8 */
#define HAL_EXTI_Line9       ((uint32_t)0x00200)     /*!< External interrupt line 9 */
#define HAL_EXTI_Line10      ((uint32_t)0x00400)     /*!< External interrupt line 10 */
#define HAL_EXTI_Line11      ((uint32_t)0x00800)     /*!< External interrupt line 11 */
#define HAL_EXTI_Line12      ((uint32_t)0x01000)     /*!< External interrupt line 12 */
#define HAL_EXTI_Line13      ((uint32_t)0x02000)     /*!< External interrupt line 13 */
#define HAL_EXTI_Line14      ((uint32_t)0x04000)     /*!< External interrupt line 14 */
#define HAL_EXTI_Line15      ((uint32_t)0x08000)     /*!< External interrupt line 15 */
#define HAL_EXTI_Line16      ((uint32_t)0x10000)     /*!< External interrupt line 16 Connected to the PVD Output */
#define HAL_EXTI_Line17      ((uint32_t)0x20000)     /*!< External interrupt line 17 Connected to the RTC Alarm event */
#define HAL_EXTI_Line18      ((uint32_t)0x40000)     /*!< External interrupt line 18 Connected to the USB OTG FS Wakeup from suspend event */                                    
#define HAL_EXTI_Line19      ((uint32_t)0x80000)     /*!< External interrupt line 19 Connected to the Ethernet Wakeup event */
#define HAL_EXTI_Line20      ((uint32_t)0x00100000)  /*!< External interrupt line 20 Connected to the USB OTG HS (configured in FS) Wakeup event  */
#define HAL_EXTI_Line21      ((uint32_t)0x00200000)  /*!< External interrupt line 21 Connected to the RTC Tamper and Time Stamp events */                                               
#define HAL_EXTI_Line22      ((uint32_t)0x00400000)  /*!< External interrupt line 22 Connected to the RTC Wakeup event */
#define HAL_EXTI_Line23      ((uint32_t)0x00800000)  /*!< External interrupt line 23 Connected to the LPTIM Wakeup event */
// @defgroup SYSCFG_EXTI_Port_Sources 
#define HAL_EXTI_PortSourceGPIOA       ((uint8_t)0x00)
#define HAL_EXTI_PortSourceGPIOB       ((uint8_t)0x01)
#define HAL_EXTI_PortSourceGPIOC       ((uint8_t)0x02)
#define HAL_EXTI_PortSourceGPIOD       ((uint8_t)0x03)
#define HAL_EXTI_PortSourceGPIOE       ((uint8_t)0x04)
#define HAL_EXTI_PortSourceGPIOF       ((uint8_t)0x05)
#define HAL_EXTI_PortSourceGPIOG       ((uint8_t)0x06)
#define HAL_EXTI_PortSourceGPIOH       ((uint8_t)0x07)
#define HAL_EXTI_PortSourceGPIOI       ((uint8_t)0x08)
#define HAL_EXTI_PortSourceGPIOJ       ((uint8_t)0x09)
#define HAL_EXTI_PortSourceGPIOK       ((uint8_t)0x0A)
// @defgroup SYSCFG_EXTI_Pin_Sources 
#define HAL_EXTI_PinSource0            ((uint8_t)0x00)
#define HAL_EXTI_PinSource1            ((uint8_t)0x01)
#define HAL_EXTI_PinSource2            ((uint8_t)0x02)
#define HAL_EXTI_PinSource3            ((uint8_t)0x03)
#define HAL_EXTI_PinSource4            ((uint8_t)0x04)
#define HAL_EXTI_PinSource5            ((uint8_t)0x05)
#define HAL_EXTI_PinSource6            ((uint8_t)0x06)
#define HAL_EXTI_PinSource7            ((uint8_t)0x07)
#define HAL_EXTI_PinSource8            ((uint8_t)0x08)
#define HAL_EXTI_PinSource9            ((uint8_t)0x09)
#define HAL_EXTI_PinSource10           ((uint8_t)0x0A)
#define HAL_EXTI_PinSource11           ((uint8_t)0x0B)
#define HAL_EXTI_PinSource12           ((uint8_t)0x0C)
#define HAL_EXTI_PinSource13           ((uint8_t)0x0D)
#define HAL_EXTI_PinSource14           ((uint8_t)0x0E)
#define HAL_EXTI_PinSource15           ((uint8_t)0x0F)

typedef enum __eHalExIntMode{
    eEXTI_IO_REG,
    eEXTI_IO_INIT,
    eEXTI_IO_GetItStatus,
    eEXTI_IO_ClearItStatus,
}eHalExInt_IOCtlMode;

typedef enum __HalEXTIMode_TypeDef
{
  eEXTI_Mode_Interrupt = 0x00,
  eEXTI_Mode_Event = 0x04
}eHalEXTIMode_TypeDef;

typedef enum __HalEXTITrigger_TypeDef
{
  eEXTI_Trigger_Rising = 0x08,
  eEXTI_Trigger_Falling = 0x0C,  
  eEXTI_Trigger_Rising_Falling = 0x10
}eHalEXTITrigger_TypeDef;


typedef __packed struct _stHalEXTI_InitTypeDef
{
  uint32_t EXTI_Line;               /*!< Specifies the EXTI lines to be enabled or disabled.
                                         This parameter can be any combination value of @ref EXTI_Lines */
   
  eHalEXTIMode_TypeDef EXTI_Mode;       /*!< Specifies the mode for the EXTI lines.
                                         This parameter can be a value of @ref EXTIMode_TypeDef */

  eHalEXTITrigger_TypeDef EXTI_Trigger; /*!< Specifies the trigger signal active edge for the EXTI lines.
                                         This parameter can be a value of @ref EXTITrigger_TypeDef */

  eHalFunctionalState EXTI_LineCmd;     /*!< Specifies the new state of the selected EXTI lines.
                                         This parameter can be set either to ENABLE or DISABLE */ 
}stHalEXTI_InitTypeDef;


/* Exported constants --------------------------------------------------------*/

/* Exported macro & function prototypes --------------------------------------*/
int HalDrvExIntOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvExIntRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvExIntWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvExIntIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvExIntClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);

#endif //__HAL_EX_INT_DRIVER_H__
