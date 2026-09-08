/**
  ******************************************************************************
  * @file    stm32fxxx_it.c
  * @author  MCD Application Team
  * @version V1.2.0
  * @date    09-November-2015
  * @brief   Main Interrupt Service Routines.
  *          This file provides all exceptions handler and peripherals interrupt
  *          service routine.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "main.h"
#include "Power_Manager.h"
#include "HalHandler.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
	extern USB_OTG_CORE_HANDLE  USB_OTG_dev;
#endif

/* Private function prototypes -----------------------------------------------*/
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
extern uint32_t USBD_OTG_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);
#endif

/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*             Cortex-M Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
	HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
	NVIC_SystemReset();
	while (1)
	{
	}
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */

void HardFault_Handler(void)
{
	HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
//	Oem_GIT_mDelay(1000);				// hardfault에 진입했는데, system tick interrupt가 살아 있을까?
	NVIC_SystemReset();
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */

#if true    
    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
    NVIC_SystemReset();
#endif

  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */

#if true
    HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
    NVIC_SystemReset();
#endif

  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */

#if true
  HalGPIOSetVaule(GPIO_MODEM_RST, eBIT_SET);
  NVIC_SystemReset();
#endif

  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}


/**
  * @brief  This function handles OTG_HS Handler.
  * @param  None
  * @retval None
  */
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
void OTG_FS_IRQHandler(void)
{
  USBD_OTG_ISR_Handler (&USB_OTG_dev);
}
#endif

/******************************************************************************/
/*                 STM32Fxxx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32fxxx.s).                                               */
/******************************************************************************/

#include "MngSystem.h"
#include "MngQueue.h"

/**
  * @brief  This function handles External lines 9 to 5 interrupt request.
  * @param  None
  * @retval None
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
