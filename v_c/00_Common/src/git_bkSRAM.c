#include "git_bkSRAM.h"
#include "stm32h7xx_hal.h"

void bkSRAM_Init(void)
{
	HAL_FLASH_Unlock();

	/*DBP : Enable access to Backup domain */
	HAL_PWR_EnableBkUpAccess();
	__HAL_RCC_BKPRAM_CLK_ENABLE();

	/*BRE : Enable backup regulator
	BRR : Wait for backup regulator to stabilize */

	HAL_PWREx_EnableBkUpReg();


	/*DBP : Disable access to Backup domain */


	__HAL_RCC_BKPRAM_CLK_DISABLE();

	HAL_PWR_DisableBkUpAccess();

	HAL_FLASH_Lock();
}

void bkSRAM_WriteVariables(uint16_t start_address, uint8_t *data, uint16_t length)
{
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_BKPRAM_CLK_ENABLE();

    for(uint16_t i = 0; i < length; i++)
    {
        *(__IO uint8_t*)(0x38800000 + start_address + i) = data[i];
    }

    SCB_CleanDCache_by_Addr((uint32_t *)(0x38800000 + start_address), length);

    __HAL_RCC_BKPRAM_CLK_DISABLE();
    HAL_PWR_DisableBkUpAccess();
}

void bkSRAM_ReadVariables(uint16_t start_address, uint8_t *read_data, uint16_t length)
{
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_BKPRAM_CLK_ENABLE();

    for(uint16_t i = 0; i < length; i++)
    {
        read_data[i] = *(__IO uint8_t *)(0x38800000 + start_address + i);
    }

    __HAL_RCC_BKPRAM_CLK_DISABLE();
    HAL_PWR_DisableBkUpAccess();
}