#ifndef __HAL_RTC_DRIVER_H__
#define __HAL_RTC_DRIVER_H__

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "common.h"

/* Exported define -----------------------------------------------------------*/
#define HAL_BKP_DR0_RTC_WAKEUP_VALUE    (0x32F2)

#define HAL_RTC_Alarm_A                    ((unsigned int)0x00000100)
#define HAL_RTC_Alarm_B                    ((unsigned int)0x00000200)

#define HAL_RTC_H24H                       ((unsigned char)0x00)
#define HAL_RTC_H12_AM                     ((unsigned char)0x00)
#define HAL_RTC_H12_PM                     ((unsigned char)0x40)
#define HAL_RTC_HourFormat_24              ((unsigned int)0x00000000)
#define HAL_RTC_HourFormat_12              ((unsigned int)0x00000040)
// @defgroup RTC_WeekDay_Definitions 
#define HAL_RTC_Weekday_Monday             ((uint8_t)0x01)
#define HAL_RTC_Weekday_Tuesday            ((uint8_t)0x02)
#define HAL_RTC_Weekday_Wednesday          ((uint8_t)0x03)
#define HAL_RTC_Weekday_Thursday           ((uint8_t)0x04)
#define HAL_RTC_Weekday_Friday             ((uint8_t)0x05)
#define HAL_RTC_Weekday_Saturday           ((uint8_t)0x06)
#define HAL_RTC_Weekday_Sunday             ((uint8_t)0x07)
// @defgroup RTC_Flags_Definitions 
#define HAL_RTC_FLAG_RECALPF                  ((unsigned int)0x00010000)
#define HAL_RTC_FLAG_TAMP1F                   ((unsigned int)0x00002000)
#define HAL_RTC_FLAG_TSOVF                    ((unsigned int)0x00001000)
#define HAL_RTC_FLAG_TSF                      ((unsigned int)0x00000800)
#define HAL_RTC_FLAG_WUTF                     ((unsigned int)0x00000400)
#define HAL_RTC_FLAG_ALRBF                    ((unsigned int)0x00000200)
#define HAL_RTC_FLAG_ALRAF                    ((unsigned int)0x00000100)
#define HAL_RTC_FLAG_INITF                    ((unsigned int)0x00000040)
#define HAL_RTC_FLAG_RSF                      ((unsigned int)0x00000020)
#define HAL_RTC_FLAG_INITS                    ((unsigned int)0x00000010)
#define HAL_RTC_FLAG_SHPF                     ((unsigned int)0x00000008)
#define HAL_RTC_FLAG_WUTWF                    ((unsigned int)0x00000004)
#define HAL_RTC_FLAG_ALRBWF                   ((unsigned int)0x00000002)
#define HAL_RTC_FLAG_ALRAWF                   ((unsigned int)0x00000001)

// @defgroup RTC_Interrupts_Definitions 
#define HAL_RTC_IT_TS                         ((unsigned int)0x00008000)
#define HAL_RTC_IT_WUT                        ((unsigned int)0x00004000)
#define HAL_RTC_IT_ALRB                       ((unsigned int)0x00002000)
#define HAL_RTC_IT_ALRA                       ((unsigned int)0x00001000)
#define HAL_RTC_IT_TAMP                       ((unsigned int)0x00000004) /* Used only to Enable the Tamper Interrupt */
#define HAL_RTC_IT_TAMP1                      ((unsigned int)0x00020000)

// @defgroup RTC_Backup_Registers_Definitions 
#define HAL_RTC_BKP_DR0                       ((unsigned int)0x00000000)
#define HAL_RTC_BKP_DR1                       ((unsigned int)0x00000001)
#define HAL_RTC_BKP_DR2                       ((unsigned int)0x00000002)
#define HAL_RTC_BKP_DR3                       ((unsigned int)0x00000003)
#define HAL_RTC_BKP_DR4                       ((unsigned int)0x00000004)
#define HAL_RTC_BKP_DR5                       ((unsigned int)0x00000005)
#define HAL_RTC_BKP_DR6                       ((unsigned int)0x00000006)
#define HAL_RTC_BKP_DR7                       ((unsigned int)0x00000007)
#define HAL_RTC_BKP_DR8                       ((unsigned int)0x00000008)
#define HAL_RTC_BKP_DR9                       ((unsigned int)0x00000009)
#define HAL_RTC_BKP_DR10                      ((unsigned int)0x0000000A)
#define HAL_RTC_BKP_DR11                      ((unsigned int)0x0000000B)
#define HAL_RTC_BKP_DR12                      ((unsigned int)0x0000000C)
#define HAL_RTC_BKP_DR13                      ((unsigned int)0x0000000D)
#define HAL_RTC_BKP_DR14                      ((unsigned int)0x0000000E)
#define HAL_RTC_BKP_DR15                      ((unsigned int)0x0000000F)
#define HAL_RTC_BKP_DR16                      ((unsigned int)0x00000010)
#define HAL_RTC_BKP_DR17                      ((unsigned int)0x00000011)
#define HAL_RTC_BKP_DR18                      ((unsigned int)0x00000012)
#define HAL_RTC_BKP_DR19                      ((unsigned int)0x00000013)
// @defgroup RTC_Input_parameter_format_definitions 
#define HAL_RTC_Format_BIN                    ((uint32_t)0x000000000)
#define HAL_RTC_Format_BCD                    ((uint32_t)0x000000001)
// @defgroup RTC_AlarmDateWeekDay_Definitions 
#define HAL_RTC_AlarmDateWeekDaySel_Date      ((uint32_t)0x00000000)
#define HAL_RTC_AlarmDateWeekDaySel_WeekDay   ((uint32_t)0x40000000)
// @defgroup RTC_AlarmMask_Definitions 
#define HAL_RTC_AlarmMask_None                ((uint32_t)0x00000000)
#define HAL_RTC_AlarmMask_DateWeekDay         ((uint32_t)0x80000000)
#define HAL_RTC_AlarmMask_Hours               ((uint32_t)0x00800000)
#define HAL_RTC_AlarmMask_Minutes             ((uint32_t)0x00008000)
#define HAL_RTC_AlarmMask_Seconds             ((uint32_t)0x00000080)
#define HAL_RTC_AlarmMask_All                 ((uint32_t)0x80808080)
// @defgroup RTC_Wakeup_Timer_Definitions 
#define HAL_RTC_WakeUpClock_RTCCLK_Div16        ((uint32_t)0x00000000)
#define HAL_RTC_WakeUpClock_RTCCLK_Div8         ((uint32_t)0x00000001)
#define HAL_RTC_WakeUpClock_RTCCLK_Div4         ((uint32_t)0x00000002)
#define HAL_RTC_WakeUpClock_RTCCLK_Div2         ((uint32_t)0x00000003)
#define HAL_RTC_WakeUpClock_CK_SPRE_16bits      ((uint32_t)0x00000004)
#define HAL_RTC_WakeUpClock_CK_SPRE_17bits      ((uint32_t)0x00000006)

/* Exported types - Structure, Enumeration -----------------------------------*/
typedef enum __eRtcType{
    eRtcTime = 0x01,
    eRtcDate = 0x02,
    eRtcAll  = 0x03, //(eRtcTime|eRtcDate),
}eHalRtcType;

typedef enum __eRtcReadType{
    eRtcBin,
    eRtcHex,
}eHalRtcReadType;

typedef enum __eHalRtc_IOCtlMode{
    eRtc_IO_Init,
    eRtc_IO_DeInit,
    eRtc_IO_ShowAppTime,
    eRtc_IO_SetTime,
    eRtc_IO_GetAlarmTime,
    eRtc_IO_SetAlarmTime,
    eRtc_IO_AlarmEnable,
    eRtc_IO_GetBackupReg,
    eRtc_IO_SetBackupReg,
    eRtc_IO_WaitForSync,
    eRtc_IO_BypassEnable,
    eRtc_IO_GetFlagStatus,
    eRtc_IO_ClearFlagStatus,
    eRtc_IO_GetITFlagStatus,
    eRtc_IO_ClearITFlagStatus,
    eRtc_IO_IntEnable,
    eRtc_IO_WakeupClockConfig,
    eRtc_IO_SetWakupCounter,
    eRtc_IO_WakeupEnable,
}eHalRtc_IOCtlMode;

typedef __packed struct __stHalRTC_InitTypeDef
{
  unsigned int RTC_HourFormat;   /*!< Specifies the RTC Hour Format.
                             This parameter can be a value of @ref RTC_Hour_Formats */
  
  unsigned int RTC_AsynchPrediv; /*!< Specifies the RTC Asynchronous Predivider value.
                             This parameter must be set to a value lower than 0x7F */
  
  unsigned int RTC_SynchPrediv;  /*!< Specifies the RTC Synchronous Predivider value.
                             This parameter must be set to a value lower than 0x7FFF */
}stHalRTC_InitTypeDef;


typedef __packed struct _stHalRTC_DateTypeDef
{
  unsigned char RTC_WeekDay; /*!< Specifies the RTC Date WeekDay.
                        This parameter can be a value of @ref RTC_WeekDay_Definitions */
  unsigned char RTC_Month;   /*!< Specifies the RTC Date Month (in BCD format).
                        This parameter can be a value of @ref RTC_Month_Date_Definitions */
  unsigned char RTC_Date;     /*!< Specifies the RTC Date.
                        This parameter must be set to a value in the 1-31 range. */
  unsigned char RTC_Year;     /*!< Specifies the RTC Date Year.
                        This parameter must be set to a value in the 0-99 range. */
}stHalRTC_DateTypeDef;

/**
  * @brief  RTC Time structure definition
  */
typedef __packed struct _stHalRTC_TimeTypeDef
{
  unsigned char RTC_Hours;    /*!< Specifies the RTC Time Hour.
                        This parameter must be set to a value in the 0-12 range
                        if the RTC_HourFormat_12 is selected or 0-23 range if
                        the RTC_HourFormat_24 is selected. */
  unsigned char RTC_Minutes;  /*!< Specifies the RTC Time Minutes.
                        This parameter must be set to a value in the 0-59 range. */
  unsigned char RTC_Seconds;  /*!< Specifies the RTC Time Seconds.
                        This parameter must be set to a value in the 0-59 range. */
  unsigned char RTC_H12;      /*!< Specifies the RTC AM/PM Time.
                        This parameter can be a value of @ref RTC_AM_PM_Definitions */
}stHalRTC_TimeTypeDef;

typedef __packed struct _stHalRTCTypeDef
{
  stHalRTC_DateTypeDef RtcDate;
  stHalRTC_TimeTypeDef RtcTime;
}stHalRTCTypeDef;

// @brief  RTC Alarm structure definition  
typedef __packed struct __stHalRTC_AlarmTypeDef
{
  stHalRTC_TimeTypeDef RTC_AlarmTime;     /*!< Specifies the RTC Alarm Time members. */

  unsigned int RTC_AlarmMask;            /*!< Specifies the RTC Alarm Masks.
                                     This parameter can be a value of @ref RTC_AlarmMask_Definitions */

  unsigned int RTC_AlarmDateWeekDaySel;  /*!< Specifies the RTC Alarm is on Date or WeekDay.
                                     This parameter can be a value of @ref RTC_AlarmDateWeekDay_Definitions */
  
  unsigned char RTC_AlarmDateWeekDay;      /*!< Specifies the RTC Alarm Date/WeekDay.
                                     If the Alarm Date is selected, this parameter
                                     must be set to a value in the 1-31 range.
                                     If the Alarm WeekDay is selected, this 
                                     parameter can be a value of @ref RTC_WeekDay_Definitions */
}stHalRTC_AlarmTypeDef;
  

/* Exported constants --------------------------------------------------------*/
/* Exported macro & function prototypes --------------------------------------*/

int HalDrvRtcOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvRtcRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvRtcWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvRtcClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvRtcIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
void HalDrvRtc_SetAlarmTime(stHalRTCTypeDef *pRTCDateTime);

#endif //__HAL_RTC_DRIVER_H__

