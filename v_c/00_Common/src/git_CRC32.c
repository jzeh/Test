#include "fatfs.h"
#include "FreeRTOS.h"
#include "common.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdlib.h>
#include "git_protocol.h"
#include "git_mmc.h"
#include "git_fsutil.h"
#include "git_function_list.h"
#include "git_pool.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_crc.h"

#define BUFFER_SIZE 1024

extern FIL g_TempFilepnt;
extern uint32_t g_u32UpdateFileSize;

CRC_HandleTypeDef hcrc;

void InitCRC32Module(void)
{
    __HAL_RCC_CRC_CLK_ENABLE();
    hcrc.Instance = CRC;
    hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
    hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
    hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_BYTE;
    hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_ENABLE;
    hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;

    if (HAL_CRC_Init(&hcrc) != HAL_OK)
    {
        Error_Handler();
    }

    __HAL_CRC_DR_RESET(&hcrc);
}

void DeinitCRC32Module(void)
{
    HAL_CRC_DeInit(&hcrc);
    __HAL_RCC_CRC_CLK_DISABLE();
}

uint32_t CalCRC32(void)
{
    uint32_t CRCresult = 0;

    U32 returnValue, ReadByte = 0;
    U8 FileBuff[BUFFER_SIZE] = {0, };
    uint32_t FileSize = f_size(&g_TempFilepnt);
    FRESULT fsResult;
    GLogN("Filesize : %d \r\n", FileSize);

    if(FileSize != g_u32UpdateFileSize)
    {
        GLogE("File size is not matched\r\n");
        DeinitCRC32Module();
        return 0;
    }

    InitCRC32Module();

    if ( f_lseek(&g_TempFilepnt, 0) == FR_OK )
    {
        for(int i=0; i < g_u32UpdateFileSize; i =  i + BUFFER_SIZE)
        {
            fsResult = f_read(&g_TempFilepnt, (void*)&FileBuff, BUFFER_SIZE, &ReadByte);
            if ( fsResult != FR_OK )
            {
                GLogE( "CRC Read fail\r\n");
                DeinitCRC32Module();
                return 0;
            }
            
            CRCresult = HAL_CRC_Accumulate(&hcrc, (uint32_t *)FileBuff, ReadByte);
        }

        CRCresult ^= 0xFFFFFFFF;
        printf("CRC Result : %x\r\n", CRCresult);
    }

    DeinitCRC32Module();
    return CRCresult;
}