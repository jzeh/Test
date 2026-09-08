#ifndef __HAL_FLASH_DRIVER_H__
#define __HAL_FLASH_DRIVER_H__


#include "common.h"
/* Exported define -----------------------------------------------------------*/
#define HAL_TYPEPROGRAM_BYTE        0x00000001U  /*!< Program byte (8-bit) at a specified address           */
#define HAL_TYPEPROGRAM_HALFWORD    0x00000002U  /*!< Program a half-word (16-bit) at a specified address   */
#define HAL_TYPEPROGRAM_WORD        0x00000004U  /*!< Program a word (32-bit) at a specified address        */
#define HAL_TYPEPROGRAM_DOUBLEWORD  0x00000008U  /*!< Program a double word (64-bit) at a specified address */


/* End of the Flash address */
#if defined(STM32F427X)
#define HAL_USER_FLASH_END_ADDRESS        0x081FFFFF
#elif defined(AT32F435VMT7)
#define HAL_FLASH_SECOTR_SIZE             4096
#define HAL_USER_FLASH_END_ADDRESS        0x083EFFFF
#endif
/* Define the user application size */
#define HAL_USER_FLASH_SIZE   (HAL_USER_FLASH_END_ADDRESS - APPLICATION_ADDRESS + 1)

/* Base address of the Flash sectors */
#define HAL_ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base address of Sector 0, 16 Kbytes   */
#define HAL_ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base address of Sector 1, 16 Kbytes   */
#define HAL_ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base address of Sector 2, 16 Kbytes   */
#define HAL_ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base address of Sector 3, 16 Kbytes   */
#define HAL_ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base address of Sector 4, 64 Kbytes   */
#define HAL_ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base address of Sector 5, 128 Kbytes  */
#define HAL_ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base address of Sector 6, 128 Kbytes  */
#define HAL_ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base address of Sector 7, 128 Kbytes  */
#define HAL_ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base address of Sector 8, 128 Kbytes  */
#define HAL_ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base address of Sector 9, 128 Kbytes  */
#define HAL_ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base address of Sector 10, 128 Kbytes */
#define HAL_ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base address of Sector 11, 128 Kbytes */
#define HAL_ADDR_FLASH_SECTOR_12     ((uint32_t)0x08100000) /* Base address of Sector 12, 16 Kbytes  */
#define HAL_ADDR_FLASH_SECTOR_13     ((uint32_t)0x08104000) /* Base address of Sector 13, 16 Kbytes  */
#define HAL_ADDR_FLASH_SECTOR_14     ((uint32_t)0x08108000) /* Base address of Sector 14, 16 Kbytes  */
#define HAL_ADDR_FLASH_SECTOR_15     ((uint32_t)0x0810C000) /* Base address of Sector 15, 16 Kbytes  */
#define HAL_ADDR_FLASH_SECTOR_16     ((uint32_t)0x08110000) /* Base address of Sector 16, 64 Kbytes  */
#define HAL_ADDR_FLASH_SECTOR_17     ((uint32_t)0x08120000) /* Base address of Sector 17, 128 Kbytes */
#define HAL_ADDR_FLASH_SECTOR_18     ((uint32_t)0x08140000) /* Base address of Sector 18, 128 Kbytes */
#define HAL_ADDR_FLASH_SECTOR_19     ((uint32_t)0x08160000) /* Base address of Sector 19, 128 Kbytes */
#define HAL_ADDR_FLASH_SECTOR_20     ((uint32_t)0x08180000) /* Base address of Sector 20, 128 Kbytes */
#define HAL_ADDR_FLASH_SECTOR_21     ((uint32_t)0x081A0000) /* Base address of Sector 21, 128 Kbytes */
#define HAL_ADDR_FLASH_SECTOR_22     ((uint32_t)0x081C0000) /* Base address of Sector 22, 128 Kbytes */
#define HAL_ADDR_FLASH_SECTOR_23     ((uint32_t)0x081E0000) /* Base address of Sector 23, 128 Kbytes */

// @defgroup FLASH_Flags 
#define HAL_FLASH_FLAG_EOP                 ((uint32_t)0x00000001)  /*!< FLASH End of Operation flag               */
#define HAL_FLASH_FLAG_OPERR               ((uint32_t)0x00000002)  /*!< FLASH operation Error flag                */
#define HAL_FLASH_FLAG_WRPERR              ((uint32_t)0x00000010)  /*!< FLASH Write protected error flag          */
#define HAL_FLASH_FLAG_PGAERR              ((uint32_t)0x00000020)  /*!< FLASH Programming Alignment error flag    */
#define HAL_FLASH_FLAG_PGPERR              ((uint32_t)0x00000040)  /*!< FLASH Programming Parallelism error flag  */
#define HAL_FLASH_FLAG_PGSERR              ((uint32_t)0x00000080)  /*!< FLASH Programming Sequence error flag     */
#define HAL_FLASH_FLAG_RDERR               ((uint32_t)0x00000100)  /*!< Read Protection error flag (PCROP)        */
#define HAL_FLASH_FLAG_BSY                 ((uint32_t)0x00010000)  /*!< FLASH Busy flag                           */ 
// @defgroup FLASH_Sectors
#define HAL_FLASH_Sector_0     ((uint16_t)0x0000) /*!< Sector Number 0   */
#define HAL_FLASH_Sector_1     ((uint16_t)0x0008) /*!< Sector Number 1   */
#define HAL_FLASH_Sector_2     ((uint16_t)0x0010) /*!< Sector Number 2   */
#define HAL_FLASH_Sector_3     ((uint16_t)0x0018) /*!< Sector Number 3   */
#define HAL_FLASH_Sector_4     ((uint16_t)0x0020) /*!< Sector Number 4   */
#define HAL_FLASH_Sector_5     ((uint16_t)0x0028) /*!< Sector Number 5   */
#define HAL_FLASH_Sector_6     ((uint16_t)0x0030) /*!< Sector Number 6   */
#define HAL_FLASH_Sector_7     ((uint16_t)0x0038) /*!< Sector Number 7   */
#define HAL_FLASH_Sector_8     ((uint16_t)0x0040) /*!< Sector Number 8   */
#define HAL_FLASH_Sector_9     ((uint16_t)0x0048) /*!< Sector Number 9   */
#define HAL_FLASH_Sector_10    ((uint16_t)0x0050) /*!< Sector Number 10  */
#define HAL_FLASH_Sector_11    ((uint16_t)0x0058) /*!< Sector Number 11  */
#define HAL_FLASH_Sector_12    ((uint16_t)0x0080) /*!< Sector Number 12  */
#define HAL_FLASH_Sector_13    ((uint16_t)0x0088) /*!< Sector Number 13  */
#define HAL_FLASH_Sector_14    ((uint16_t)0x0090) /*!< Sector Number 14  */
#define HAL_FLASH_Sector_15    ((uint16_t)0x0098) /*!< Sector Number 15  */
#define HAL_FLASH_Sector_16    ((uint16_t)0x00A0) /*!< Sector Number 16  */
#define HAL_FLASH_Sector_17    ((uint16_t)0x00A8) /*!< Sector Number 17  */
#define HAL_FLASH_Sector_18    ((uint16_t)0x00B0) /*!< Sector Number 18  */
#define HAL_FLASH_Sector_19    ((uint16_t)0x00B8) /*!< Sector Number 19  */
#define HAL_FLASH_Sector_20    ((uint16_t)0x00C0) /*!< Sector Number 20  */
#define HAL_FLASH_Sector_21    ((uint16_t)0x00C8) /*!< Sector Number 21  */
#define HAL_FLASH_Sector_22    ((uint16_t)0x00D0) /*!< Sector Number 22  */
#define HAL_FLASH_Sector_23    ((uint16_t)0x00D8) /*!< Sector Number 23  */

/* Exported types - Structure, Enumeration -----------------------------------*/
// @brief FLASH Status  
#if defined(STM32F427X)
typedef enum __eHalFLASH_Status
{ 
  eHalFLASH_BUSY = 1,
  eHalFLASH_ERROR_RD,
  eHalFLASH_ERROR_PGS,
  eHalFLASH_ERROR_PGP,
  eHalFLASH_ERROR_PGA,
  eHalFLASH_ERROR_WRP,
  eHalFLASH_ERROR_PROGRAM,
  eHalFLASH_ERROR_OPERATION,
  eHalFLASH_COMPLETE
}eHalFLASH_Status;
#elif defined(AT32F435VMT7)
typedef enum __eHalFLASH_Status
{ 
  eHalFLASH_BUSY = 0,
  eHalFLASH_ERROR_PROGRAM,
  eHalFLASH_EPP_ERROR,
  eHalFLASH_COMPLETE,
  eHalFLASH_OPERATE_TIMEOUT,
}eHalFLASH_Status;
#endif

/* Exported constants --------------------------------------------------------*/
/* Exported macro & function prototypes --------------------------------------*/

int HalDrvFlashOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvFlashRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvFlashReadByteCallByRef(uint32_t* FlashAddress, uint8_t* Data ,uint16_t DataLength);
int HalDrvFlashWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvFlashWriteByteCallByRef(uint32_t* FlashAddress, uint8_t* Data ,uint16_t DataLength);
int HalDrvFlashIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvFlashClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);

int HalDrvFlashErase(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);

#endif //__HAL_FLASH_DRIVER_H__
