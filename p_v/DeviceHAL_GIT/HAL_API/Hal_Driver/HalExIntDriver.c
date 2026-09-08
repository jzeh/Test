/*****************************************************************************
* @file    HalExIntDriver.c
* @author  James Jean
* @version V1
* @date    2022-03-02
* @brief   
******************************************************************************

******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>

#include "common.h"

#include "HalHandler.h"
#include "Power_Manager.h"

#if defined(STM32F427X)
#include "STM32F4xx.h"
#include "STM32F4xx_gpio.h"
#include "STM32F4xx_exti.h"

#elif defined(AT32F435VMT7)
#include "at32f435_437.h"
#include "at32f435_437_gpio.h"
#include "at32f435_437_exint.h"

#define EXTI9_5_IRQHandler EXINT9_5_IRQHandler

#endif

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/////////////////////////////////////////////////////////////////////////////////////
int HalDrvExIntOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    // NOT USED 
    //EXTILine9_5_Config();
    return HAL_RETURN_SUCCESS;
}

int HalDrvExIntRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

int HalDrvExIntWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

int HalDrvExIntIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    int nIOMode = nLparam;
    unsigned int unExtLine = nRparam;

    switch( nIOMode )
    {
        case eEXTI_IO_REG:
        {
            unsigned char ucExtiPinSourceGroup = (unsigned char)nRparam;
            unsigned char ucExtiPinSource = (unsigned char)nOverlap;
#if defined(STM32F427X)
            SYSCFG_EXTILineConfig(ucExtiPinSourceGroup, ucExtiPinSource);
#elif defined(AT32F435VMT7)
            scfg_exint_line_config((scfg_port_source_type)ucExtiPinSourceGroup, (scfg_pins_source_type)ucExtiPinSource);
#endif
        }
            break;
        case eEXTI_IO_INIT:
        {
            stHalEXTI_InitTypeDef* pHalEXTI = (stHalEXTI_InitTypeDef*)pBuffer;

#if defined(STM32F427X)
            EXTI_InitTypeDef EXTI_InitStructure;
            memcpy(&EXTI_InitStructure, pHalEXTI, sizeof(EXTI_InitTypeDef));

            EXTI_Init(&EXTI_InitStructure);
#elif defined(AT32F435VMT7)
            exint_init_type stExtiInitType;

            if ( pHalEXTI->EXTI_Mode == eEXTI_Mode_Interrupt )
                stExtiInitType.line_mode = EXINT_LINE_INTERRUPUT;
            else if ( pHalEXTI->EXTI_Mode == eEXTI_Mode_Event )
                stExtiInitType.line_mode = EXINT_LINE_EVENT;
            
            if ( pHalEXTI->EXTI_Trigger == eEXTI_Trigger_Rising )
                stExtiInitType.line_polarity = EXINT_TRIGGER_RISING_EDGE;
            else if ( pHalEXTI->EXTI_Trigger == eEXTI_Trigger_Falling )
                stExtiInitType.line_polarity = EXINT_TRIGGER_FALLING_EDGE;
            else if ( pHalEXTI->EXTI_Trigger == eEXTI_Trigger_Rising_Falling )
                stExtiInitType.line_polarity = EXINT_TRIGGER_BOTH_EDGE;

            stExtiInitType.line_select = pHalEXTI->EXTI_Line;
            stExtiInitType.line_enable = (confirm_state)pHalEXTI->EXTI_LineCmd;
            exint_init(&stExtiInitType);
#endif
        }
            break;
        case eEXTI_IO_GetItStatus:
            
#if defined(STM32F427X)
                return EXTI_GetITStatus(unExtLine);
#elif defined(AT32F435VMT7)
                return exint_flag_get(unExtLine);
#endif
        case eEXTI_IO_ClearItStatus:
#if defined(STM32F427X)
                EXTI_ClearITPendingBit(unExtLine);
#elif defined(AT32F435VMT7)
                exint_flag_clear(unExtLine);
#endif
            break;
        default:
            break;

    }

	return HAL_RETURN_SUCCESS;
}

int HalDrvExIntClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////


/**
  * @brief  Configures EXTI Line15 (connected to PG15 pin) in interrupt mode
  * @param  None
  * @retval None
  */
#if 0 //// NOT USED 
static void EXTILine9_5_Config(void)
{
    stHalEXTI_InitTypeDef   EXTI_InitStructure;
    stHalGPIO_InitTypeDef   GPIO_InitStructure;
    stHalNVIC_InitTypeDef   NVIC_InitStructure;

    /* Enable GPIOC clock */
    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOC_GROUP, NULL, 0, HAL_ENABLE);
    /* Enable SYSCFG clock */
    HalDrvRccIOCtrl(eRCC_IO_SYSCFG_Clock, 0, NULL, 0, HAL_ENABLE);


    /* Configure PC5 pin as input floating */
    GPIO_InitStructure.GPIO_DS   = eGPIO_DRIVE_STRENGTH_STRONGER;
    GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Pin  = GPIO_SENSOR_INT;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

    /* Connect EXTI Line5 to PC5 pin */
    HalDrvExIntIOCtrl(eEXTI_IO_REG, HAL_EXTI_PortSourceGPIOC, NULL, 0, HAL_EXTI_PinSource5);

    /* Configure EXTI Line5 */
    EXTI_InitStructure.EXTI_Line = HAL_EXTI_Line5;
    EXTI_InitStructure.EXTI_Mode = eEXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = eEXTI_Trigger_Rising;				// L --> H
    EXTI_InitStructure.EXTI_LineCmd = HAL_ENABLE;

    HalDrvExIntIOCtrl(eEXTI_IO_INIT, 0, (char*)&EXTI_InitStructure, sizeof(EXTI_InitStructure), 0);

    /* Enable and set EXTI9_5_IRQn Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = HAL_EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_Group_0_NVIC_IRQChannelPreemptionPriority;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_EXTI9_5_NVIC_IRQChannelSubPriority;
    NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_ENABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);
}
#endif

//------------------------------------------------------------------
// Interrup handler 
void EXTI9_5_IRQHandler(void)
{
    if(HalDrvExIntIOCtrl(eEXTI_IO_GetItStatus, HAL_EXTI_Line5, NULL, 0, 0) != RESET)
    {
        //printf("\nEXTI: Sensor Interrupt(PC5)\n\n");
#if !defined(FEATURE_BOOTLOADER)
        if(g_bYUJINSelftestFlag == true)
        {
            if(GetWakeupPA0PinState() == true)                    g_uiWakeupPinToggleCnt++;
            if(GetWakeupDetectPinState(eWAKE_PIN_SENSOR) == true)	g_uiSensorWakeupPinToggleCnt++;
        }
#endif

        /* Clear the EXTI line 5 pending bit */
        HalDrvExIntIOCtrl(eEXTI_IO_ClearItStatus, HAL_EXTI_Line5, NULL, 0, 0);
    }
}

