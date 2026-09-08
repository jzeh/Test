#ifndef __HAL_I2C_DRIVER_H__
#define __HAL_I2C_DRIVER_H__


/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "HalGpioDriver.h"
/* Exported define -----------------------------------------------------------*/


#define HAL_I2C_1                           I2C1
#define HAL_I2C_SCL_PIN                     GPIO_I2C1_SCL
#define HAL_I2C_SDA_PIN                     GPIO_I2C1_SDA
#define HAL_I2Cx_OWN_ADDRESS                0xA0

#if defined(STM32F427X)
#define HAL_I2C_SPEED                       200000 //400K->200K ALPU-MP Stable Speed
#elif defined(AT32F435VMT7)
//#define HAL_I2C_SPEED                     0xB170FFFF   //10K
//#define HAL_I2C_SPEED                     0xC0E06969   //50K
//#define HAL_I2C_SPEED                     0x80504C4E   //100K
//#define HAL_I2C_SPEED                       0x30F03C6B   //200K
#define HAL_I2C_SPEED                       0x10F03C64 //<--288MHz:0x10F03C64 , 280MHz:0x10F03A67   //200K

#endif

#define HAL_GYRO_ACCELER_SLAVE_ADDR			0xD0	// MPU6515
#define HAL_GYRO_MAGNETOMETER_SLAVE_ADDR	0x18	// MXG2320
#define HAL_PRESSURE_SLAVE_ADDR				0xEC	// BMP280
//#define PRESSURE_SLAVE_ADDR				0xEE	// BMP180

// -----------------------------------------------------------------------------------
// @brief  Communication start
/* Master RECEIVER mode -----------------------------*/ 
#define HAL_I2C_EVENT_MASTER_MODE_SELECT                ((unsigned int)0x00030001)  /* BUSY, MSL and SB flag */
#define HAL_I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED  ((unsigned int)0x00070082)  /* BUSY, MSL, ADDR, TXE and TRA flags */
#define HAL_I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED     ((unsigned int)0x00030002)  /* BUSY, MSL and ADDR flags */
#define HAL_I2C_EVENT_MASTER_MODE_ADDRESS10             ((unsigned int)0x00030008)  /* BUSY, MSL and ADD10 flags */
#define HAL_I2C_EVENT_MASTER_BYTE_RECEIVED              ((unsigned int)0x00030040)  /* BUSY, MSL and RXNE flags */
/* Master TRANSMITTER mode --------------------------*/
#define HAL_I2C_EVENT_MASTER_BYTE_TRANSMITTING          ((unsigned int)0x00070080) /* TRA, BUSY, MSL, TXE flags */
#define HAL_I2C_EVENT_MASTER_BYTE_TRANSMITTED           ((unsigned int)0x00070084)  /* TRA, BUSY, MSL, TXE and BTF flags */


// @defgroup I2C_mode 
#define HAL_I2C_Mode_I2C                    ((unsigned short)0x0000)
#define HAL_I2C_Mode_SMBusDevice            ((unsigned short)0x0002)  
#define HAL_I2C_Mode_SMBusHost              ((unsigned short)0x000A)
// @defgroup I2C_duty_cycle_in_fast_mode 
#define HAL_I2C_DutyCycle_16_9              ((unsigned short)0x4000) /*!< I2C fast mode Tlow/Thigh = 16/9 */
#define HAL_I2C_DutyCycle_2                 ((unsigned short)0xBFFF) /*!< I2C fast mode Tlow/Thigh = 2 */
// @defgroup I2C_acknowledgement
#define HAL_I2C_Ack_Enable                  ((unsigned short)0x0400)
#define HAL_I2C_Ack_Disable                 ((unsigned short)0x0000)
// @defgroup I2C_transfer_direction 
#define HAL_I2C_Direction_Transmitter       ((unsigned char)0x00)
#define HAL_I2C_Direction_Receiver          ((unsigned char)0x01)
// @defgroup I2C_acknowledged_address 
#define HAL_I2C_AcknowledgedAddress_7bit    ((unsigned short)0x4000)
#define HAL_I2C_AcknowledgedAddress_10bit   ((unsigned short)0xC000)

// @brief  SR2 register flags  
#define HAL_I2C_FLAG_DUALF                  ((uint32_t)0x00800000)
#define HAL_I2C_FLAG_SMBHOST                ((uint32_t)0x00400000)
#define HAL_I2C_FLAG_SMBDEFAULT             ((uint32_t)0x00200000)
#define HAL_I2C_FLAG_GENCALL                ((uint32_t)0x00100000)
#define HAL_I2C_FLAG_TRA                    ((uint32_t)0x00040000)
#define HAL_I2C_FLAG_BUSY                   ((uint32_t)0x00020000)
#define HAL_I2C_FLAG_MSL                    ((uint32_t)0x00010000)
// @brief  SR1 register flags  
#define HAL_I2C_FLAG_SMBALERT               ((uint32_t)0x10008000)
#define HAL_I2C_FLAG_TIMEOUT                ((uint32_t)0x10004000)
#define HAL_I2C_FLAG_PECERR                 ((uint32_t)0x10001000)
#define HAL_I2C_FLAG_OVR                    ((uint32_t)0x10000800)
#define HAL_I2C_FLAG_AF                     ((uint32_t)0x10000400)
#define HAL_I2C_FLAG_ARLO                   ((uint32_t)0x10000200)
#define HAL_I2C_FLAG_BERR                   ((uint32_t)0x10000100)
#define HAL_I2C_FLAG_TXE                    ((uint32_t)0x10000080)
#define HAL_I2C_FLAG_RXNE                   ((uint32_t)0x10000040)
#define HAL_I2C_FLAG_STOPF                  ((uint32_t)0x10000010)
#define HAL_I2C_FLAG_ADD10                  ((uint32_t)0x10000008)
#define HAL_I2C_FLAG_BTF                    ((uint32_t)0x10000004)
#define HAL_I2C_FLAG_ADDR                   ((uint32_t)0x10000002)
#define HAL_I2C_FLAG_SB                     ((uint32_t)0x10000001)

/* Exported types - Structure, Enumeration -----------------------------------*/

typedef enum __eHali2c_address_mode_type
{
  eI2C_ADDRESS_MODE_7BIT                  = 0x00, /*!< 7bit address mode */
  eI2C_ADDRESS_MODE_10BIT                 = 0x01  /*!< 10bit address mode */
} eHali2c_address_mode_type;

typedef enum __eHalI2C_IOCtlMode{
    eI2C_IO_Init,
    eI2C_IO_DeInit,
    eI2C_IO_Enable,
    eI2C_IO_GetFlagStatus,
    eI2C_IO_GenerateStart,
    eI2C_IO_GenerateStop,
    eI2C_IO_CheckEvent,
    eI2C_IO_SetTransferAddr,
    eI2C_IO_SetAcknowlegeConfig,
}eHalI2C_IOCtlMode;


// @brief  I2C Init structure definition  
typedef __packed struct _stHalI2C_InitTypeDef
{
  unsigned int I2C_ClockSpeed;          /*!< Specifies the clock frequency.
                                         This parameter must be set to a value lower than 400kHz */

  unsigned short I2C_Mode;                /*!< Specifies the I2C mode.
                                         This parameter can be a value of @ref I2C_mode */

  unsigned short I2C_DutyCycle;           /*!< Specifies the I2C fast mode duty cycle.
                                         This parameter can be a value of @ref I2C_duty_cycle_in_fast_mode */

  unsigned short I2C_OwnAddress1;         /*!< Specifies the first device own address.
                                         This parameter can be a 7-bit or 10-bit address. */

  unsigned short I2C_Ack;                 /*!< Enables or disables the acknowledgement.
                                         This parameter can be a value of @ref I2C_acknowledgement */

  unsigned short I2C_AcknowledgedAddress; /*!< Specifies if 7-bit or 10-bit address is acknowledged.
                                         This parameter can be a value of @ref I2C_acknowledged_address */
}stHalI2C_InitTypeDef;

/* Exported constants --------------------------------------------------------*/




/* Exported macro & function prototypes --------------------------------------*/
int HalDrvI2COpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvI2CRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvI2CWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvI2CIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvI2CClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);

void HalDrvI2C_SetInitialForGPIO(void);




// ALPU & MPU export functions
unsigned char I2C_Write(unsigned char ucDeviceAddr, unsigned char ucRegAddr, unsigned char* pucData, unsigned char ucLength);
unsigned char I2C_Read(unsigned char ucDeviceAddr, unsigned char ucRegAddr, unsigned char* pucData, unsigned char ucLength);



#endif //__HAL_I2C_DRIVER_H__
