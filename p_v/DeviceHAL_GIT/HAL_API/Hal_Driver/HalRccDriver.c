/*
  ******************************************************************************
  * @file    HalRccDriver.c
  * @author  James Jean
  * @version V1.0.0
  * @date    2022-03-02
  * @brief
  *
  *
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#if defined(STM32F427X)
#include "STM32F4xx.h"
#include "STM32F4xx_rcc.h"
#include "Misc.h"
#elif defined(AT32F435VMT7)
#include "at32f435_437.h"
#include "at32f435_437_crm.h"
#endif
#include "HalHandler.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#if defined(STM32F427X)
extern uint32_t SystemCoreClock;
#elif defined(AT32F435VMT7)
extern unsigned int system_core_clock;
#endif
/* Private function prototypes -----------------------------------------------*/
void HalRccSystemClock_Init();
unsigned int HalRccSetUartClock(eHalRccClockUart eClockUart, eHalFunctionalState eRccState);
void HalRccSetSPIClock(eHalRccClockSPI eClockRcc, eHalFunctionalState eRccState);
void HalRccSetI2CClock(eHalRccClockI2C eClockRcc, eHalFunctionalState eRccState);
void HalRccSetI2CResetClock(eHalRccClockI2C eClockRcc, eHalFunctionalState eRccState);


/* Private functions ---------------------------------------------------------*/


void HalRccSystemClock_Init()
{
    stHalRCC_ClocksTypeDef RCC_ClockFreq;

#if defined(AT32F435VMT7)
    /**
    * @brief  system clock config program
    * @note   the system clock is configured as follow:
    *         - system clock        = (hext * pll_ns)/(pll_ms * pll_fr)
    *         - system clock source = pll (hext)
    *         - hext                = 8000000
    *         - sclk                = 288000000
    *         - ahbdiv              = 1
    *         - ahbclk              = 288000000
    *         - apb2div             = 2
    *         - apb2clk             = 144000000
    *         - apb1div             = 2
    *         - apb1clk             = 144000000
    *         - pll_ns              = 72
    *         - pll_ms              = 1
    *         - pll_fr              = 2
    * @param  none
    * @retval none
    */
    uint16_t pll_ns, pll_ms, pll_fr;
    
    /* enable pwc periph clock */
    crm_periph_clock_enable(CRM_PWC_PERIPH_CLOCK, TRUE);

    /* config ldo voltage */
    pwc_ldo_output_voltage_set(PWC_LDO_OUTPUT_1V3);

    /* set the flash clock divider */
    flash_clock_divider_set(FLASH_CLOCK_DIV_3);

    /* reset crm */
    crm_reset();

    crm_clock_source_enable(CRM_CLOCK_SOURCE_HEXT, TRUE);

    /* wait till hext is ready */
    while(crm_hext_stable_wait() == ERROR)
    {
    }

    /* calculate pll parameter according to the function */
    crm_pll_parameter_calculate(CRM_PLL_SOURCE_HEXT, 288000000, &pll_ms, &pll_ns, &pll_fr);

    /* config pll clock resource */
    crm_pll_config(CRM_PLL_SOURCE_HEXT, pll_ns, pll_ms, (crm_pll_fr_type)pll_fr);
//    crm_pll_config(CRM_PLL_SOURCE_HEXT, 72, 1, CRM_PLL_FR_2);

    /* enable pll */
    crm_clock_source_enable(CRM_CLOCK_SOURCE_PLL, TRUE);

    /* wait till pll is ready */
    while(crm_flag_get(CRM_PLL_STABLE_FLAG) != SET)
    {
    }

    /* config ahbclk */
    crm_ahb_div_set(CRM_AHB_DIV_1);

    /* config apb1clk */
    crm_apb1_div_set(CRM_APB1_DIV_4);

    /* config apb2clk */
    crm_apb2_div_set(CRM_APB2_DIV_2);

    /* enable auto step mode */
    crm_auto_step_mode_enable(TRUE);

    /* select pll as system clock source */
    crm_sysclk_switch(CRM_SCLK_PLL);

    /* wait till pll is used as system clock source */
    while(crm_sysclk_switch_status_get() != CRM_SCLK_PLL)
    {
    }

    /* disable auto step mode */
    crm_auto_step_mode_enable(FALSE);

    /* update system_core_clock global variable */
    system_core_clock_update();
#endif
     

#if defined(STM32F427X)
    SysTick_Config(SystemCoreClock / 1000);

    /* Enable Clock Security System(CSS): this will generate an NMI exception when HSE clock fails */
    RCC_ClockSecuritySystemCmd(ENABLE);//enable clock security system
#elif defined(AT32F435VMT7 )
    SysTick_Config(system_core_clock / 1000);
#endif
    HalDrvRccIOCtrl(eRCC_IO_GETSYS_Clock, 0, (char*)&RCC_ClockFreq, sizeof(stHalRCC_ClocksTypeDef), 0);

#if defined(FEATURE_BOOTLOADER)
#else
{
    stHalNVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = HAL_RCC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_Group_0_NVIC_IRQChannelPreemptionPriority;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_RCC_NVIC_IRQChannelSubPriority;
    NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_ENABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);
}
#endif

}

unsigned int HalRccSetUartClock(eHalRccClockUart eClockUart, eHalFunctionalState eRccState)
{
    unsigned int unPeriphUartNum;
#if defined(STM32F427X)
    if      ( eClockUart == eRCC_Clock_Uart1 ) unPeriphUartNum = RCC_APB2Periph_USART1;
    else if ( eClockUart == eRCC_Clock_Uart2 ) unPeriphUartNum = RCC_APB1Periph_USART2;
    else if ( eClockUart == eRCC_Clock_Uart3 ) unPeriphUartNum = RCC_APB1Periph_USART3;
    else if ( eClockUart == eRCC_Clock_Uart4 ) unPeriphUartNum = RCC_APB1Periph_UART4;
    else if ( eClockUart == eRCC_Clock_Uart5 ) unPeriphUartNum = RCC_APB1Periph_UART5;
    else if ( eClockUart == eRCC_Clock_Uart6 ) unPeriphUartNum = RCC_APB2Periph_USART6;
    else if ( eClockUart == eRCC_Clock_Uart7 ) unPeriphUartNum = RCC_APB1Periph_UART7;
    else if ( eClockUart == eRCC_Clock_Uart8 ) unPeriphUartNum = RCC_APB1Periph_UART8;
#elif defined(AT32F435VMT7)
    if      ( eClockUart == eRCC_Clock_Uart1 ) unPeriphUartNum = CRM_USART1_PERIPH_CLOCK;
    else if ( eClockUart == eRCC_Clock_Uart2 ) unPeriphUartNum = CRM_USART2_PERIPH_CLOCK;
    else if ( eClockUart == eRCC_Clock_Uart3 ) unPeriphUartNum = CRM_USART3_PERIPH_CLOCK;
    else if ( eClockUart == eRCC_Clock_Uart4 ) unPeriphUartNum = CRM_UART4_PERIPH_CLOCK;
    else if ( eClockUart == eRCC_Clock_Uart5 ) unPeriphUartNum = CRM_UART5_PERIPH_CLOCK;
    else if ( eClockUart == eRCC_Clock_Uart6 ) unPeriphUartNum = CRM_USART6_PERIPH_CLOCK;
    else if ( eClockUart == eRCC_Clock_Uart7 ) unPeriphUartNum = CRM_UART7_PERIPH_CLOCK;
    else if ( eClockUart == eRCC_Clock_Uart8 ) unPeriphUartNum = CRM_UART8_PERIPH_CLOCK;
#endif
    return unPeriphUartNum;
}

void HalRccSetSPIClock(eHalRccClockSPI eClockRcc, eHalFunctionalState eRccState)
{
    unsigned int unSpiSelClock;
#if defined(STM32F427X)
    if      ( eClockRcc == eRCC_Clock_SPI1 ) unSpiSelClock = RCC_APB2Periph_SPI1;
    else if ( eClockRcc == eRCC_Clock_SPI2 ) unSpiSelClock = RCC_APB1Periph_SPI2;
    else if ( eClockRcc == eRCC_Clock_SPI3 ) unSpiSelClock = RCC_APB1Periph_SPI3;
    else if ( eClockRcc == eRCC_Clock_SPI4 ) unSpiSelClock = RCC_APB2Periph_SPI4;
    else if ( eClockRcc == eRCC_Clock_SPI5 ) unSpiSelClock = RCC_APB2Periph_SPI5;
    else if ( eClockRcc == eRCC_Clock_SPI6 ) unSpiSelClock = RCC_APB2Periph_SPI6;

    if ( (eClockRcc == eRCC_Clock_SPI2) || (eClockRcc == eRCC_Clock_SPI3) )
        RCC_APB1PeriphClockCmd(unSpiSelClock, (FunctionalState)eRccState);
    else
        RCC_APB2PeriphClockCmd(unSpiSelClock, (FunctionalState)eRccState);    
#elif defined(AT32F435VMT7)
    if      ( eClockRcc == eRCC_Clock_SPI1 ) unSpiSelClock = CRM_SPI1_PERIPH_CLOCK;
    else if ( eClockRcc == eRCC_Clock_SPI2 ) unSpiSelClock = CRM_SPI2_PERIPH_CLOCK;
    else if ( eClockRcc == eRCC_Clock_SPI3 ) unSpiSelClock = CRM_SPI3_PERIPH_CLOCK;
    else if ( eClockRcc == eRCC_Clock_SPI4 ) unSpiSelClock = CRM_SPI4_PERIPH_CLOCK;

    crm_periph_clock_enable((crm_periph_clock_type)unSpiSelClock, (confirm_state)eRccState);    
#endif
}

void HalRccSetI2CClock(eHalRccClockI2C eClockRcc, eHalFunctionalState eRccState)
{
    unsigned int unI2CSelClock = 0;
#if defined(STM32F427X)
    if      ( eClockRcc == eRCC_Clock_I2C1 ) unI2CSelClock = RCC_APB1Periph_I2C1;
    else if ( eClockRcc == eRCC_Clock_I2C2 ) unI2CSelClock = RCC_APB1Periph_I2C2;
    else if ( eClockRcc == eRCC_Clock_I2C3 ) unI2CSelClock = RCC_APB1Periph_I2C3;

    RCC_APB1PeriphClockCmd(unI2CSelClock, (FunctionalState)eRccState);

#elif defined(AT32F435VMT7)
    if      ( eClockRcc == eRCC_Clock_I2C1 ) unI2CSelClock = CRM_I2C1_PERIPH_CLOCK;
    else if ( eClockRcc == eRCC_Clock_I2C2 ) unI2CSelClock = CRM_I2C2_PERIPH_CLOCK;
    else if ( eClockRcc == eRCC_Clock_I2C3 ) unI2CSelClock = CRM_I2C3_PERIPH_CLOCK;

    crm_periph_clock_enable((crm_periph_clock_type)unI2CSelClock, (confirm_state)eRccState);    
#endif

}

void HalRccSetI2CResetClock(eHalRccClockI2C eClockRcc, eHalFunctionalState eRccState)
{
    unsigned int unI2CSelResetClock;
#if defined(STM32F427X)
    if      ( eClockRcc == eRCC_Clock_I2C1 ) unI2CSelResetClock = RCC_APB1Periph_I2C1;
    else if ( eClockRcc == eRCC_Clock_I2C2 ) unI2CSelResetClock = RCC_APB1Periph_I2C2;
    else if ( eClockRcc == eRCC_Clock_I2C3 ) unI2CSelResetClock = RCC_APB1Periph_I2C3;

    RCC_APB1PeriphResetCmd(unI2CSelResetClock, (FunctionalState)eRccState);

#elif defined(AT32F435VMT7)
    if      ( eClockRcc == eRCC_Clock_I2C1 ) unI2CSelResetClock = CRM_I2C1_PERIPH_RESET;
    else if ( eClockRcc == eRCC_Clock_I2C2 ) unI2CSelResetClock = CRM_I2C2_PERIPH_RESET;
    else if ( eClockRcc == eRCC_Clock_I2C3 ) unI2CSelResetClock = CRM_I2C3_PERIPH_RESET;

    crm_periph_reset((crm_periph_reset_type)unI2CSelResetClock, (confirm_state)eRccState);    
#endif

}

void HalRcc_printSystemCLKs( void )
{
    stHalRCC_ClocksTypeDef RCC_ClockFreq;
    HalDrvRccIOCtrl(eRCC_IO_GETSYS_Clock, 0, (char*)&RCC_ClockFreq, sizeof(stHalRCC_ClocksTypeDef), 0);
	printf( "==================================================\r\n" );
    //  printf( "CORE      = %9d, %6.2lf MHz\r\n", SystemCoreClock, (double)SystemCoreClock*1e-6 );
	printf( "CORE      = %9d, %6.2lf MHz\r\n", RCC_ClockFreq.SYSCLK_Frequency, (double)RCC_ClockFreq.SYSCLK_Frequency*1e-6 );
	printf( "HCLK      = %9d, %6.2lf MHz\r\n", RCC_ClockFreq.HCLK_Frequency, (double)RCC_ClockFreq.HCLK_Frequency*1e-6 );
	printf( "APB1      = %9d, %6.2lf MHz\r\n", RCC_ClockFreq.PCLK1_Frequency, (double)RCC_ClockFreq.PCLK1_Frequency*1e-6 );
	printf( "APB2      = %9d, %6.2lf MHz\r\n", RCC_ClockFreq.PCLK2_Frequency, (double)RCC_ClockFreq.PCLK2_Frequency*1e-6 );
	printf( "==================================================\r\n" );
}


//---------------------------------------------------------------------------------//
int HalDrvRccOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    HalRccSystemClock_Init();

    return HAL_RETURN_SUCCESS;
}

int HalDrvRccRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

int HalDrvRccWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

int HalDrvRccIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{   
    // nTargetClock ==> Clock Target -> see eHalRccClock
    // nOverlap      ==> HAL_ENABLE or HAL_DISABLE -> see HalHandler.h
    int nTargetClock = nLparam;
    int nClockEnable = nOverlap;

    switch( nTargetClock )
    {
        case eRCC_IO_GPIO_Clock:
        {
            // nRparam ==> GPIO GROUP  -> see in HalGPIODriver.h
            int nGpioGROUP   = nRparam;
            unsigned int nPeriPh_Clock = 0; 

#if defined(STM32F427X)
                if ( nGpioGROUP & GPIOA_GROUP ) nPeriPh_Clock += RCC_AHB1Periph_GPIOA;
                if ( nGpioGROUP & GPIOB_GROUP ) nPeriPh_Clock += RCC_AHB1Periph_GPIOB;
                if ( nGpioGROUP & GPIOC_GROUP ) nPeriPh_Clock += RCC_AHB1Periph_GPIOC;
                if ( nGpioGROUP & GPIOD_GROUP ) nPeriPh_Clock += RCC_AHB1Periph_GPIOD;
                if ( nGpioGROUP & GPIOE_GROUP ) nPeriPh_Clock += RCC_AHB1Periph_GPIOE;

                RCC_AHB1PeriphClockCmd(nPeriPh_Clock, (FunctionalState)nClockEnable);                
#elif defined(AT32F435VMT7)
                if ( nGpioGROUP & GPIOA_GROUP )
                {
                    nPeriPh_Clock = CRM_GPIOA_PERIPH_CLOCK;
                    crm_periph_clock_enable((crm_periph_clock_type)nPeriPh_Clock, (confirm_state)nClockEnable);
                }
                if ( nGpioGROUP & GPIOB_GROUP ) 
                {
                    nPeriPh_Clock = CRM_GPIOB_PERIPH_CLOCK;
                    crm_periph_clock_enable((crm_periph_clock_type)nPeriPh_Clock, (confirm_state)nClockEnable);
                }
                if ( nGpioGROUP & GPIOC_GROUP ) 
                {
                    nPeriPh_Clock = CRM_GPIOC_PERIPH_CLOCK;
                    crm_periph_clock_enable((crm_periph_clock_type)nPeriPh_Clock, (confirm_state)nClockEnable);
                }
                if ( nGpioGROUP & GPIOD_GROUP )
                {
                    nPeriPh_Clock = CRM_GPIOD_PERIPH_CLOCK;
                    crm_periph_clock_enable((crm_periph_clock_type)nPeriPh_Clock, (confirm_state)nClockEnable);
                }
                if ( nGpioGROUP & GPIOE_GROUP )
                {
                    nPeriPh_Clock = CRM_GPIOE_PERIPH_CLOCK;
                    crm_periph_clock_enable((crm_periph_clock_type)nPeriPh_Clock, (confirm_state)nClockEnable);
                }
#endif
        }
            break;
        case eRCC_IO_UART_Clock:
        {
            eHalRccClockUart eClockUart = (eHalRccClockUart)nRparam;
            unsigned int unPeriphUartNum= HalRccSetUartClock(eClockUart, (eHalFunctionalState)nClockEnable);
#if defined(STM32F427X)
            if ( (eClockUart == eRCC_Clock_Uart1) || (eClockUart == eRCC_Clock_Uart6) )
                RCC_APB2PeriphClockCmd(unPeriphUartNum, (FunctionalState)nClockEnable);
            else
                RCC_APB1PeriphClockCmd(unPeriphUartNum, (FunctionalState)nClockEnable);
#elif defined(AT32F435VMT7)
                crm_periph_clock_enable((crm_periph_clock_type)unPeriphUartNum, (confirm_state)nClockEnable);
#endif
        }
            break;
        case eRCC_IO_UART_ResetClock:
        {
            eHalRccClockUart eClockUart = (eHalRccClockUart)nRparam;
            unsigned int unPeriphUartNum= HalRccSetUartClock(eClockUart, (eHalFunctionalState)nClockEnable);
#if defined(STM32F427X)
            if ( (eClockUart == eRCC_Clock_Uart1) || (eClockUart == eRCC_Clock_Uart6) )
                RCC_AHB2PeriphResetCmd(unPeriphUartNum, (FunctionalState)nClockEnable);
            else
                RCC_AHB1PeriphResetCmd(unPeriphUartNum, (FunctionalState)nClockEnable);
#elif defined(AT32F435VMT7)
            crm_periph_reset((crm_periph_reset_type)unPeriphUartNum, (confirm_state)nClockEnable);
#endif
        }
            break;
        case eRCC_IO_SYSCFG_Clock:

#if defined(STM32F427X)
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, (FunctionalState)nClockEnable);
#elif defined(AT32F435VMT7)
            crm_periph_clock_enable(CRM_SCFG_PERIPH_CLOCK, (confirm_state)nClockEnable);
#endif
            break;
        case eRCC_IO_GETSYS_Clock:
        {
            stHalRCC_ClocksTypeDef* pRCC_ClockFreq = (stHalRCC_ClocksTypeDef*)pBuffer;
#if defined(STM32F427X)
            RCC_ClocksTypeDef RCC_ClockFreq;
            /* This function fills the RCC_ClockFreq structure with the current frequencies of different on chip clocks (for debug purpose) */
            RCC_GetClocksFreq(&RCC_ClockFreq);

            memcpy((char*)pRCC_ClockFreq, (char*)&RCC_ClockFreq, sizeof(RCC_ClocksTypeDef));
#elif defined(AT32F435VMT7)
            crm_clocks_freq_type clocks_freq;
            crm_clocks_freq_get(&clocks_freq);
            pRCC_ClockFreq->SYSCLK_Frequency = clocks_freq.sclk_freq;
            pRCC_ClockFreq->HCLK_Frequency   = clocks_freq.ahb_freq;
            pRCC_ClockFreq->PCLK1_Frequency  = clocks_freq.apb1_freq;
            pRCC_ClockFreq->PCLK2_Frequency  = clocks_freq.apb2_freq;
#endif
        }
            break;
        case eRCC_IO_SetSourceClock:
        {
#if defined(STM32F427X)
            RCC_SYSCLKConfig(nRparam);
#elif defined(AT32F435VMT7)
            crm_sclk_type sourceClock = CRM_SCLK_PLL;
            if (nRparam == HAL_RCC_SYSCLKSource_PLLCLK ) sourceClock = CRM_SCLK_PLL;
            
            /* enable auto step mode */
            crm_auto_step_mode_enable(TRUE);
            /* select pll as system clock source */
            crm_sysclk_switch(sourceClock);
#endif
        }
            break;
        case eRCC_IO_GetSourceClock:
        {
#if defined(STM32F427X)
/*              - 0x00: HSI used as system clock
*              - 0x04: HSE used as system clock
*              - 0x08: PLL used as system clock (PLL P for STM32F446xx devices)
*              - 0x0C: PLL R used as system clock (only for STM32F446xx devices)
*/          
            unsigned char ucRet = RCC_GetSYSCLKSource();
            if      ( ucRet == 0x00 ) ucRet = HAL_RCC_SYSCLKSource_HSI;
            else if ( ucRet == 0x04 ) ucRet = HAL_RCC_SYSCLKSource_HSE;
            else if ( ucRet == 0x08 ) ucRet = HAL_RCC_SYSCLKSource_PLLCLK;
            return ucRet;
#elif defined(AT32F435VMT7)
/*         - CRM_SCLK_HICK
*         - CRM_SCLK_HEXT
*         - CRM_SCLK_PLL
*/
            return crm_sysclk_switch_status_get();
#endif
        }
            break;

        case eRCC_IO_SET_LSE_CONFIG:
        {
#if defined(STM32F427X)
            RCC_LSEConfig((unsigned char)nRparam);
#elif defined(AT32F435VMT7)
            if ( nRparam == HAL_RCC_LSE_Bypass ) 
            {
                // bypass
                crm_lext_bypass((confirm_state)nClockEnable);
            }
            else
            {
                // HAL_RCC_LSE_ON, HAL_RCC_LSE_OFF
                crm_clock_source_enable(CRM_CLOCK_SOURCE_LEXT, (confirm_state)nClockEnable);
            }            
#endif
        }
            break;
        case eRCC_IO_SET_HSE_CONFIG:
        {
#if defined(STM32F427X)
            RCC_HSEConfig((unsigned char)nRparam);
#elif defined(AT32F435VMT7)
            if ( nRparam == HAL_RCC_HSE_Bypass ) 
            {

                // bypass
                crm_hext_bypass((confirm_state)nClockEnable);
            }
            else
            {
                // HAL_RCC_HSE_ON, HAL_RCC_HSE_OFF
                crm_clock_source_enable(CRM_CLOCK_SOURCE_HEXT, (confirm_state)nClockEnable);
            }            
#endif
        }
            break;
        case eRCC_IO_SET_RTC_Clock_CONFIG:
        {
           unsigned int unRtcClockSource = nRparam;
#if defined(STM32F427X)
            RCC_RTCCLKConfig(unRtcClockSource);
#elif defined(AT32F435VMT7)
            
            if      ( unRtcClockSource == HAL_RCC_RTCCLKSource_LSE ) unRtcClockSource = CRM_ERTC_CLOCK_LEXT;
            else if ( unRtcClockSource == HAL_RCC_RTCCLKSource_LSI ) unRtcClockSource = CRM_ERTC_CLOCK_LICK;
            else if ( unRtcClockSource == HAL_RCC_RTCCLKSource_HSE_Div2 ) unRtcClockSource = CRM_ERTC_CLOCK_HEXT_DIV_2;
            else if ( unRtcClockSource == HAL_RCC_RTCCLKSource_HSE_Div3 ) unRtcClockSource = CRM_ERTC_CLOCK_HEXT_DIV_3;
            else if ( unRtcClockSource == HAL_RCC_RTCCLKSource_HSE_Div4 ) unRtcClockSource = CRM_ERTC_CLOCK_HEXT_DIV_4;

            crm_ertc_clock_select((crm_ertc_clock_type)unRtcClockSource);
 #endif
        }
            break;
        case eRCC_IO_SET_RTC_Clock_ENABLE:
#if defined(STM32F427X)
            RCC_RTCCLKCmd((FunctionalState)nClockEnable);
#elif defined(AT32F435VMT7)
            crm_ertc_clock_enable((confirm_state)nClockEnable);
#endif
            break;
            
        case eRCC_IO_GET_FLAG_STATUS:
        {
#if defined(STM32F427X)
            return RCC_GetFlagStatus((unsigned char)nRparam);
#elif defined(AT32F435VMT7)
            unsigned int unRCCFlag = nRparam; 

            if      ( unRCCFlag == HAL_RCC_FLAG_HSIRDY  ) unRCCFlag = CRM_HICK_STABLE_FLAG;
            else if ( unRCCFlag == HAL_RCC_FLAG_HSERDY  ) unRCCFlag = CRM_HEXT_STABLE_FLAG;
            else if ( unRCCFlag == HAL_RCC_FLAG_PLLRDY  ) unRCCFlag = CRM_PLL_STABLE_FLAG;
            else if ( unRCCFlag == HAL_RCC_FLAG_LSERDY  ) unRCCFlag = CRM_LEXT_STABLE_FLAG;
            else if ( unRCCFlag == HAL_RCC_FLAG_LSIRDY  ) unRCCFlag = CRM_LICK_STABLE_FLAG;
            else if ( unRCCFlag == HAL_RCC_FLAG_PORRST  ) unRCCFlag = CRM_POR_RESET_FLAG;
            else if ( unRCCFlag == HAL_RCC_FLAG_SFTRST  ) unRCCFlag = CRM_SW_RESET_FLAG;
            else if ( unRCCFlag == HAL_RCC_FLAG_IWDGRST ) unRCCFlag = CRM_WDT_RESET_FLAG;
            else if ( unRCCFlag == HAL_RCC_FLAG_WWDGRST ) unRCCFlag = CRM_WWDT_RESET_FLAG;
            else if ( unRCCFlag == HAL_RCC_FLAG_LPWRRST ) unRCCFlag = CRM_LOWPOWER_RESET_FLAG;
//          else if ( unRCCFlag == HAL_RCC_FLAG_PLLI2SRDY)unRCCFlag = CRM_HICK_STABLE_FLAG;
//          else if ( unRCCFlag == HAL_RCC_FLAG_PLLSAIRDY)unRCCFlag = CRM_HICK_STABLE_FLAG;
//          else if ( unRCCFlag == HAL_RCC_FLAG_BORRST  ) unRCCFlag = CRM_HICK_STABLE_FLAG;
//          else if ( unRCCFlag == HAL_RCC_FLAG_PINRST  ) unRCCFlag = CRM_HICK_STABLE_FLAG;

            return crm_flag_get(unRCCFlag);
#endif
        }
            break;
        case eRCC_IO_Clear_FLAG_STATUS:
#if defined(STM32F427X)
            RCC_ClearFlag();
#elif defined(AT32F435VMT7)
#endif
            break;
        case eRCC_IO_SET_PLL_ENABLE:
        {
#if defined(STM32F427X)
            RCC_PLLCmd((FunctionalState)nClockEnable);
#elif defined(AT32F435VMT7)
            crm_clock_source_enable(CRM_CLOCK_SOURCE_PLL, (confirm_state)nClockEnable);
#endif
        }
            break;
        case eRCC_IO_SPI_Clock:
            HalRccSetSPIClock((eHalRccClockSPI)nRparam, (eHalFunctionalState)nClockEnable);
            break;
        case eRCC_IO_MCOX_Clock_config:
#if defined(STM32F427X)
            if ( nRparam == HAL_RCC_MCO1Source_PLLCLK )
                RCC_MCO1Config(nRparam, nClockEnable);
            else
                RCC_MCO2Config(nRparam, nClockEnable);
#elif defined(AT32F435VMT7)
#endif
            break;
        case eRCC_IO_I2C_Clock:
            HalRccSetI2CClock((eHalRccClockI2C)nRparam, (eHalFunctionalState)nClockEnable);
            break;
        case eRCC_IO_I2C_RESET_Clock:
            HalRccSetI2CResetClock((eHalRccClockI2C)nRparam, (eHalFunctionalState)nClockEnable);
            break;
        case eRCC_IO_Power_Clock:
#if defined(STM32F427X)
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, (FunctionalState)nClockEnable);
#elif defined(AT32F435VMT7)
            crm_periph_clock_enable(CRM_PWC_PERIPH_CLOCK, (confirm_state)nClockEnable);
#endif
            break;
        case eRCC_IO_BKRam_Clock:
#if defined(STM32F427X)
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_BKPSRAM, (FunctionalState)nClockEnable);
#elif defined(AT32F435VMT7)
#endif
            break;
        case eRCC_IO_BKRam_Clock_Reset:
#if defined(STM32F427X)
            RCC_BackupResetCmd((FunctionalState)nClockEnable);
#elif defined(AT32F435VMT7)
#endif
            break;
        case eRCC_IO_CAN_Clock:
        {
            eHalRccClockCAN eCanCh = (eHalRccClockCAN)nRparam;
            unsigned int unCanPeriph;
#if defined(STM32F427X)
            if      ( eCanCh == eRCC_Clock_CAN1 ) unCanPeriph = RCC_APB1Periph_CAN1;
            else if ( eCanCh == eRCC_Clock_CAN2 ) unCanPeriph = RCC_APB1Periph_CAN2;

            RCC_APB1PeriphClockCmd(unCanPeriph, (FunctionalState)nClockEnable);
#elif defined(AT32F435VMT7)
            if      ( eCanCh == eRCC_Clock_CAN1 ) unCanPeriph = CRM_CAN1_PERIPH_CLOCK;
            else if ( eCanCh == eRCC_Clock_CAN2 ) unCanPeriph = CRM_CAN2_PERIPH_CLOCK;

            crm_periph_clock_enable((crm_periph_clock_type)unCanPeriph, (confirm_state)nClockEnable);
#endif
        }
            break;
        case eRCC_IO_ADC_Clock:
        {
            eHalRccClockADC eADCCh = (eHalRccClockADC)nRparam;
            unsigned int unADCPeriph;
#if defined(STM32F427X)
            if      ( eADCCh == eRCC_Clock_ADC1 ) unADCPeriph = RCC_APB2Periph_ADC1;
            else if ( eADCCh == eRCC_Clock_ADC2 ) unADCPeriph = RCC_APB2Periph_ADC2;

            RCC_APB2PeriphClockCmd((unsigned int)unADCPeriph, (FunctionalState)nClockEnable);
#elif defined(AT32F435VMT7)
            if      ( eADCCh == eRCC_Clock_ADC1 ) unADCPeriph = CRM_ADC1_PERIPH_CLOCK;
            else if ( eADCCh == eRCC_Clock_ADC2 ) unADCPeriph = CRM_ADC2_PERIPH_CLOCK;

            crm_periph_clock_enable((crm_periph_clock_type)unADCPeriph, (confirm_state)nClockEnable);
#endif
        }
            break;
        case eRCC_IO_DMA_Clock:
        {
            eHalRccClockDMA eDMAch = (eHalRccClockDMA)nRparam;
            unsigned int unDMAClock;
#if defined(STM32F427X)
            if      ( eDMAch == eRCC_Clock_DMA1 ) unDMAClock = RCC_AHB1Periph_DMA1;
            else if ( eDMAch == eRCC_Clock_DMA2 ) unDMAClock = RCC_AHB1Periph_DMA2;
            else if ( eDMAch == eRCC_Clock_DMA2D) unDMAClock = RCC_AHB1Periph_DMA2D;
            
            RCC_AHB1PeriphClockCmd((unsigned int)unDMAClock, (FunctionalState)nClockEnable);
#elif defined(AT32F435VMT7)
            if      ( eDMAch == eRCC_Clock_DMA1 ) unDMAClock = CRM_DMA1_PERIPH_CLOCK;
            else if ( eDMAch == eRCC_Clock_DMA2 ) unDMAClock = CRM_DMA2_PERIPH_CLOCK;
            else if ( eDMAch == eRCC_Clock_DMA2D) unDMAClock = CRM_EDMA_PERIPH_CLOCK;

            crm_periph_clock_enable((crm_periph_clock_type)unDMAClock, (confirm_state)nClockEnable);
#endif
        }
            break;
        case eRCC_IO_DMA_Reset_Clock:
        {
            eHalRccClockDMA eDMAch = (eHalRccClockDMA)nRparam;
            unsigned int unDMAClock;
#if defined(STM32F427X)
            if      ( eDMAch == eRCC_Clock_DMA1 ) unDMAClock = RCC_AHB1Periph_DMA1;
            else if ( eDMAch == eRCC_Clock_DMA2 ) unDMAClock = RCC_AHB1Periph_DMA2;
            else if ( eDMAch == eRCC_Clock_DMA2D) unDMAClock = RCC_AHB1Periph_DMA2D;
            
            RCC_APB1PeriphResetCmd((unsigned int)unDMAClock, (FunctionalState)nClockEnable);
#elif defined(AT32F435VMT7)
            if      ( eDMAch == eRCC_Clock_DMA1 ) unDMAClock = CRM_DMA1_PERIPH_CLOCK;
            else if ( eDMAch == eRCC_Clock_DMA2 ) unDMAClock = CRM_DMA2_PERIPH_CLOCK;
            else if ( eDMAch == eRCC_Clock_DMA2D) unDMAClock = CRM_EDMA_PERIPH_CLOCK;

            crm_periph_reset((crm_periph_reset_type)unDMAClock, (confirm_state)nClockEnable);
#endif
        }
            break;
        case eRCC_IO_TIM_Clock:
        {
            eHalRccClockTIM eTimch = (eHalRccClockTIM)nRparam;
            unsigned int unTIMClock;
            
#if defined(STM32F427X)
    if      ( eTimch == eRCC_Clock_TIM1 ) unTIMClock = RCC_APB2Periph_TIM1;
    else if ( eTimch == eRCC_Clock_TIM2 ) unTIMClock = RCC_APB1Periph_TIM2;
    else if ( eTimch == eRCC_Clock_TIM3 ) unTIMClock = RCC_APB1Periph_TIM3;
    else if ( eTimch == eRCC_Clock_TIM4 ) unTIMClock = RCC_APB1Periph_TIM4;
    else if ( eTimch == eRCC_Clock_TIM5 ) unTIMClock = RCC_APB1Periph_TIM5;
    else if ( eTimch == eRCC_Clock_TIM6 ) unTIMClock = RCC_APB1Periph_TIM6;
    else if ( eTimch == eRCC_Clock_TIM7 ) unTIMClock = RCC_APB1Periph_TIM7;
    else if ( eTimch == eRCC_Clock_TIM8 ) unTIMClock = RCC_APB2Periph_TIM8;
    else if ( eTimch == eRCC_Clock_TIM9 ) unTIMClock = RCC_APB2Periph_TIM9;
    else if ( eTimch == eRCC_Clock_TIM10) unTIMClock = RCC_APB2Periph_TIM10;
    else if ( eTimch == eRCC_Clock_TIM11) unTIMClock = RCC_APB2Periph_TIM11;
    else if ( eTimch == eRCC_Clock_TIM12) unTIMClock = RCC_APB1Periph_TIM12;
    else if ( eTimch == eRCC_Clock_TIM13) unTIMClock = RCC_APB1Periph_TIM13;
    else if ( eTimch == eRCC_Clock_TIM14) unTIMClock = RCC_APB1Periph_TIM14;

    if ( (eTimch == eRCC_Clock_TIM1) || (eTimch == eRCC_Clock_TIM8) || (eTimch == eRCC_Clock_TIM9) ||
         (eTimch == eRCC_Clock_TIM10) || (eTimch == eRCC_Clock_TIM11) )
        RCC_APB2PeriphClockCmd(unTIMClock, (FunctionalState)nClockEnable);
    else 
        RCC_APB1PeriphClockCmd(unTIMClock, (FunctionalState)nClockEnable);
    
#elif defined(AT32F435VMT7)
    if      ( eTimch == eRCC_Clock_TIM1 ) unTIMClock = CRM_TMR1_PERIPH_CLOCK;
    else if ( eTimch == eRCC_Clock_TIM2 ) unTIMClock = CRM_TMR2_PERIPH_CLOCK;
    else if ( eTimch == eRCC_Clock_TIM3 ) unTIMClock = CRM_TMR2_PERIPH_CLOCK;
    else if ( eTimch == eRCC_Clock_TIM4 ) unTIMClock = CRM_TMR2_PERIPH_CLOCK;
    else if ( eTimch == eRCC_Clock_TIM5 ) unTIMClock = CRM_TMR2_PERIPH_CLOCK;
    else if ( eTimch == eRCC_Clock_TIM6 ) unTIMClock = CRM_TMR2_PERIPH_CLOCK;
    else if ( eTimch == eRCC_Clock_TIM7 ) unTIMClock = CRM_TMR2_PERIPH_CLOCK;
    else if ( eTimch == eRCC_Clock_TIM8 ) unTIMClock = CRM_TMR8_PERIPH_CLOCK;
    else if ( eTimch == eRCC_Clock_TIM9 ) unTIMClock = CRM_TMR9_PERIPH_CLOCK;
    else if ( eTimch == eRCC_Clock_TIM10) unTIMClock = CRM_TMR10_PERIPH_CLOCK;
    else if ( eTimch == eRCC_Clock_TIM11) unTIMClock = CRM_TMR11_PERIPH_CLOCK;
    else if ( eTimch == eRCC_Clock_TIM12) unTIMClock = CRM_TMR12_PERIPH_CLOCK;
    else if ( eTimch == eRCC_Clock_TIM13) unTIMClock = CRM_TMR13_PERIPH_CLOCK;
    else if ( eTimch == eRCC_Clock_TIM14) unTIMClock = CRM_TMR14_PERIPH_CLOCK;

    crm_periph_clock_enable((crm_periph_clock_type)unTIMClock, (confirm_state)nClockEnable);

#endif
        }
            break;
            
        default:
            break;
    }
    return HAL_RETURN_SUCCESS;
}

int HalDrvRccClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

