/*
  ******************************************************************************
  * @file    HalFlashDriver.c
  * @author  James Jean
  * @version V1.0.0
  * @date    2022-03-02
  * @brief
  *
  *
  ******************************************************************************
*/
#include "HalFlashDriver.h"
/* Includes ------------------------------------------------------------------*/
#include "HalHandler.h"
#include <intrinsics.h>

#if defined(STM32F427X)
#include "STM32F4xx.h"
#include "STM32F4xx_flash.h"
#elif defined(AT32F435VMT7)
#include "at32f435_437.h"
#include "at32f435_437_flash.h"
#endif

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#if defined(AT32F435VMT7)
#define FLASH_Lock              flash_lock
#define FLASH_Unlock            flash_unlock
#define FLASH_ProgramByte       flash_byte_program
#define FLASH_EraseSector(X,Y)  flash_sector_erase(X)
#endif

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
void HalFLASH_If_Init(void);
void HalSetRDPofFlash();
unsigned int HalFlashGetSector(uint32_t Address);


/* Private functions ---------------------------------------------------------*/

int HalDrvFlashOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    HalFLASH_If_Init();
#if !defined(FEATURE_BOOTLOADER)
    HalSetRDPofFlash();
#endif
    return HAL_RETURN_SUCCESS;
}

int HalDrvFlashRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    int nFlashAddr = nLparam;    // FlashAddress
    int i = 0, nJumpCnt = nOverlap;

    if ( pBuffer == NULL ) return HAL_RETURN_FAIL;    
    if ( nJumpCnt == 0 ) nJumpCnt = HAL_TYPEPROGRAM_BYTE;

    for ( i=0; (i < nLength) && (nFlashAddr <= (HAL_USER_FLASH_END_ADDRESS-4)); i+=nJumpCnt )
    {
    	pBuffer[i] = *(unsigned char*)(nFlashAddr);
        nFlashAddr += nJumpCnt;
    }

    return HAL_RETURN_SUCCESS;
}

int HalDrvFlashReadByteCallByRef(uint32_t* FlashAddress, uint8_t* Data ,uint16_t DataLength)
{
    uint32_t i = 0;

	for ( i=0; (i < DataLength) && (*FlashAddress <= (HAL_USER_FLASH_END_ADDRESS-4)); i++ )
	{
		Data[i] = *(uint8_t*)*(FlashAddress);
		/* Increment FLASH destination address */
		*FlashAddress += 1;
	}

	return HAL_RETURN_SUCCESS;
}

int HalDrvFlashWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    int i = 0, nJumpCnt = nOverlap;
    int nFlashAddr = nLparam;    // FlashAddress

    if ( pBuffer == NULL ) return HAL_RETURN_FAIL;    
    if ( nJumpCnt == 0 ) nJumpCnt = HAL_TYPEPROGRAM_BYTE;
    
    __disable_interrupt();
    FLASH_Unlock();
	for (i = 0; (i < nLength) && (nFlashAddr <= (HAL_USER_FLASH_END_ADDRESS-4)); i+=nJumpCnt)
	{
		if (FLASH_ProgramByte(nFlashAddr, *(unsigned char*)(pBuffer+i)) == (int)eHalFLASH_COMPLETE)
		{
			/* Check the written value */
			if (*(unsigned char*)nFlashAddr != *(unsigned char*)(pBuffer+i))
			{
				/* Flash content doesn't match SRAM content */
				printf("Flash content doesn't match SRAM content\n");
				FLASH_Lock();
				__enable_interrupt();
				return(2);
			}
			/* Increment FLASH destination address */
			nFlashAddr += nJumpCnt;
//			printf(".");
		}
		else
		{
			/* Error occurred while writing data in Flash memory */
			printf("Error occurred while writing data in Flash memory_m\n");
			FLASH_Lock();
			__enable_interrupt();
			return (1);
		}
	}

    FLASH_Lock();

    __enable_interrupt();

    return HAL_RETURN_SUCCESS;
}

int HalDrvFlashWriteByteCallByRef(uint32_t* FlashAddress, uint8_t* Data ,uint16_t DataLength)
{
    __disable_interrupt();

	unsigned int i = 0;

    FLASH_Unlock();
	for (i = 0; (i < DataLength) && (*FlashAddress <= (HAL_USER_FLASH_END_ADDRESS-4)); i++)
	{
		if (FLASH_ProgramByte(*FlashAddress, *(unsigned char*)(Data+i)) == (int)eHalFLASH_COMPLETE)
		{
			/* Check the written value */
			if (*(unsigned char*)*FlashAddress != *(unsigned char*)(Data+i))
			{
				/* Flash content doesn't match SRAM content */
				printf("Flash content doesn't match SRAM content\n");
				FLASH_Lock();
				__enable_interrupt();
				return(2);
			}
			/* Increment FLASH destination address */
			*FlashAddress += 1;
//			printf(".");
		}
		else
		{
			/* Error occurred while writing data in Flash memory */
			printf("Error occurred while writing data in Flash memory_m\n");
			FLASH_Lock();
			__enable_interrupt();
			return (1);
		}
	}

    FLASH_Lock();

    __enable_interrupt();

    return HAL_RETURN_SUCCESS;
}

int HalDrvFlashIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

int HalDrvFlashErase(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    unsigned int unUserStartSector, unUserEndSector, i = 0;

    __disable_interrupt();
    FLASH_Unlock();
    HalFLASH_If_Init();

    unUserStartSector = HalFlashGetSector(nLparam);
    unUserEndSector = HalFlashGetSector(nRparam);

#if defined(STM32F427X)
    for(i = unUserStartSector; i <= unUserEndSector; )
#elif defined(AT32F435VMT7)
    for(i = unUserStartSector; i <= unUserEndSector; i+=HAL_FLASH_SECOTR_SIZE)
#endif
    {
        if (FLASH_EraseSector(i, VoltageRange_3) != (int)eHalFLASH_COMPLETE)
        {
            /* Error occurred while page erase */
            FLASH_Lock();

            printf("Flash erase error\r\n");
            __enable_interrupt();
            return (1);
        }
#if defined(STM32F427X)
        if (i == HAL_FLASH_Sector_11)
        {
            i += 40;
        }
        else
        {
            i += 8;
        }
#endif
    }

    FLASH_Lock();

    __enable_interrupt();
    
    return HAL_RETURN_SUCCESS;
}


int HalDrvFlashClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}


void HalFLASH_If_Init(void)
{
#if defined(STM32F427X)
    /* Clear pending flags (if any) */
    FLASH_ClearFlag(HAL_FLASH_FLAG_EOP | HAL_FLASH_FLAG_OPERR | HAL_FLASH_FLAG_WRPERR |
                  HAL_FLASH_FLAG_PGAERR | HAL_FLASH_FLAG_PGPERR|HAL_FLASH_FLAG_PGSERR);
#elif defined(AT32F435VMT7)
    flash_flag_clear( //FLASH_ODF_FLAG | FLASH_PRGMERR_FLAG | FLASH_EPPERR_FLAG |
                    FLASH_BANK1_OBF_FLAG |FLASH_BANK1_ODF_FLAG | FLASH_BANK1_PRGMERR_FLAG | FLASH_BANK1_EPPERR_FLAG |
                    FLASH_BANK2_ODF_FLAG | FLASH_BANK2_PRGMERR_FLAG | FLASH_BANK2_EPPERR_FLAG);
#endif
}

void HalSetRDPofFlash()
{
#if defined(STM32F427X)
#ifdef ENABLE_RDP_LEVEL_1
	if(!FLASH_OB_GetRDP())
	{
		FLASH_OB_Unlock();

		FLASH_OB_RDPConfig(OB_RDP_Level_1);
		FLASH_OB_Launch();
		FLASH_OB_Lock();

		NVIC_SystemReset();
	}
#endif //ENABLE_RDP_LEVEL_1 
#elif defined(AT32F435VMT7)
#endif

}

unsigned int HalFlashGetSector(uint32_t Address)
{
  uint32_t sector = 0;
#if defined(STM32F427X)
  if((Address < HAL_ADDR_FLASH_SECTOR_1) && (Address >= HAL_ADDR_FLASH_SECTOR_0))
  {
    sector = HAL_FLASH_Sector_0;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_2) && (Address >= HAL_ADDR_FLASH_SECTOR_1))
  {
    sector = HAL_FLASH_Sector_1;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_3) && (Address >= HAL_ADDR_FLASH_SECTOR_2))
  {
    sector = HAL_FLASH_Sector_2;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_4) && (Address >= HAL_ADDR_FLASH_SECTOR_3))
  {
    sector = HAL_FLASH_Sector_3;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_5) && (Address >= HAL_ADDR_FLASH_SECTOR_4))
  {
    sector = HAL_FLASH_Sector_4;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_6) && (Address >= HAL_ADDR_FLASH_SECTOR_5))
  {
    sector = HAL_FLASH_Sector_5;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_7) && (Address >= HAL_ADDR_FLASH_SECTOR_6))
  {
    sector = HAL_FLASH_Sector_6;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_8) && (Address >= HAL_ADDR_FLASH_SECTOR_7))
  {
    sector = HAL_FLASH_Sector_7;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_9) && (Address >= HAL_ADDR_FLASH_SECTOR_8))
  {
    sector = HAL_FLASH_Sector_8;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_10) && (Address >= HAL_ADDR_FLASH_SECTOR_9))
  {
    sector = HAL_FLASH_Sector_9;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_11) && (Address >= HAL_ADDR_FLASH_SECTOR_10))
  {
    sector = HAL_FLASH_Sector_10;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_12) && (Address >= HAL_ADDR_FLASH_SECTOR_11))
  {
    sector = HAL_FLASH_Sector_11;
  }

  else if((Address < HAL_ADDR_FLASH_SECTOR_13) && (Address >= HAL_ADDR_FLASH_SECTOR_12))
  {
    sector = HAL_FLASH_Sector_12;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_14) && (Address >= HAL_ADDR_FLASH_SECTOR_13))
  {
    sector = HAL_FLASH_Sector_13;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_15) && (Address >= HAL_ADDR_FLASH_SECTOR_14))
  {
    sector = HAL_FLASH_Sector_14;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_16) && (Address >= HAL_ADDR_FLASH_SECTOR_15))
  {
    sector = HAL_FLASH_Sector_15;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_17) && (Address >= HAL_ADDR_FLASH_SECTOR_16))
  {
    sector = HAL_FLASH_Sector_16;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_18) && (Address >= HAL_ADDR_FLASH_SECTOR_17))
  {
    sector = HAL_FLASH_Sector_17;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_19) && (Address >= HAL_ADDR_FLASH_SECTOR_18))
  {
    sector = HAL_FLASH_Sector_18;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_20) && (Address >= HAL_ADDR_FLASH_SECTOR_19))
  {
    sector = HAL_FLASH_Sector_19;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_21) && (Address >= HAL_ADDR_FLASH_SECTOR_20))
  {
    sector = HAL_FLASH_Sector_20;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_22) && (Address >= HAL_ADDR_FLASH_SECTOR_21))
  {
    sector = HAL_FLASH_Sector_21;
  }
  else if((Address < HAL_ADDR_FLASH_SECTOR_23) && (Address >= HAL_ADDR_FLASH_SECTOR_22))
  {
    sector = HAL_FLASH_Sector_22;
  }
  else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_23))*/
  {
    sector = HAL_FLASH_Sector_23;
  }
  
#elif defined(AT32F435VMT7)
  sector = Address;
#endif

  return sector;
}


