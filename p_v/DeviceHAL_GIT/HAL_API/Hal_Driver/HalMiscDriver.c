/*
  ******************************************************************************
  * @file    HalMiscDriver.c
  * @author  James Jean
  * @version V1.0.0
  * @date    2022-03-02
  * @brief
  *
  *
  ******************************************************************************
*/
/* Includes ------------------------------------------------------------------*/
#include "HalMiscDriver.h"
#if defined(STM32F427X)
#include "STM32F4xx.h"
#include "Misc.h"
#elif defined(AT32F435VMT7)
#include "at32f435_437.h"
#include "at32f435_437_misc.h"
#endif

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

// stHalNVIC_InitTypeDef

int HalDrvMiscOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
#if defined(STM32F427X)
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
#elif defined(AT32F435VMT7)
    nvic_priority_group_config(NVIC_PRIORITY_GROUP_2);
#endif
    return HAL_RETURN_SUCCESS;
}

int HalDrvMiscRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

int HalDrvMiscWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{

    return HAL_RETURN_SUCCESS;
}

int HalDrvMiscIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    // nLparam : IoCtlMode
    eHalMiscIoCtlMode nIoCtlMode = (eHalMiscIoCtlMode)nLparam;
    stHalNVIC_InitTypeDef* pstHalNVIC = (stHalNVIC_InitTypeDef*)pBuffer ;
    
    switch ( nIoCtlMode )
    {
        case eMISC_IO_RegIRQ:
        {
#if defined(STM32F427X)
            NVIC_InitTypeDef NVIC_InitVar;
            memcpy((char*)&NVIC_InitVar, (char*)pstHalNVIC, sizeof(NVIC_InitTypeDef));
            NVIC_Init(&NVIC_InitVar);
#elif defined(AT32F435VMT7)
            int nIRQ_Number = pstHalNVIC->NVIC_IRQChannel;
            BOOL bIrqEnable = pstHalNVIC->NVIC_IRQChannelEnable;
            unsigned char ucPreemptionP = pstHalNVIC->NVIC_IRQChannelPreemptionPriority;
            unsigned char ucSubP = pstHalNVIC->NVIC_IRQChannelSubPriority;

            if ( bIrqEnable == TRUE )
                nvic_irq_enable((IRQn_Type)nIRQ_Number, ucPreemptionP, ucSubP);
            else
                nvic_irq_disable((IRQn_Type)nIRQ_Number);
#endif
        }
            break;
		default:
			break;
    }

    return HAL_RETURN_SUCCESS;
}

int HalDrvMiscClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
	/*
	stHalNVIC_InitTypeDef   NVIC_InitStructure;

	for ( 	
    NVIC_InitStructure.NVIC_IRQChannel = HAL_UART7_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_DISABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);
*/
    return HAL_RETURN_SUCCESS;
}

