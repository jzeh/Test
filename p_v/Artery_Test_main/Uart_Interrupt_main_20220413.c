/**
  **************************************************************************
  * @file     main.c
  * @version  v2.0.5
  * @date     2022-02-11
  * @brief    main program
  **************************************************************************
  *                       Copyright notice & Disclaimer
  *
  * The software Board Support Package (BSP) that is made available to
  * download from Artery official website is the copyrighted work of Artery.
  * Artery authorizes customers to use, copy, and distribute the BSP
  * software and its related documentation for the purpose of design and
  * development in conjunction with Artery microcontrollers. Use of the
  * software is governed by this copyright notice and the following disclaimer.
  *
  * THIS SOFTWARE IS PROVIDED ON "AS IS" BASIS WITHOUT WARRANTIES,
  * GUARANTEES OR REPRESENTATIONS OF ANY KIND. ARTERY EXPRESSLY DISCLAIMS,
  * TO THE FULLEST EXTENT PERMITTED BY LAW, ALL EXPRESS, IMPLIED OR
  * STATUTORY OR OTHER WARRANTIES, GUARANTEES OR REPRESENTATIONS,
  * INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
  *
  **************************************************************************
  */

//#include "at32f435_437_board.h"
//#include "at32f435_437_clock.h"

/** @addtogroup AT32F435_periph_examples
  * @{
  */

/** @addtogroup 435_USART_interrupt USART_interrupt
  * @{
  */
#include "HalHandler.h"
#include "at32f435_437_usart.h"
#include "at32f435_437_gpio.h"
#include "at32f435_437_crm.h"
      

#define USART7_BUFFER_SIZE            100

unsigned char usart7_tx_buffer[USART7_BUFFER_SIZE];
unsigned char usart7_rx_buffer[USART7_BUFFER_SIZE];

volatile unsigned char usart7_tx_counter = 0x00;
volatile unsigned char usart7_rx_counter = 0x00;

/**
  * @brief  config usart
  * @param  none
  * @retval none
  */
void usart_configuration(void)
{
  gpio_init_type gpio_init_struct;

  /* enable the usart7 and gpio clock */
  crm_periph_clock_enable(CRM_UART7_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_GPIOE_PERIPH_CLOCK, TRUE);

  gpio_default_para_init(&gpio_init_struct);

  /* configure the usart7 tx, rx pin */
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type  = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = GPIO_PINS_7 | GPIO_PINS_8;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOE, &gpio_init_struct);
  gpio_pin_mux_config(GPIOE, GPIO_PINS_SOURCE7, GPIO_MUX_8);
  gpio_pin_mux_config(GPIOE, GPIO_PINS_SOURCE8, GPIO_MUX_8);

  /* config usart nvic interrupt */
  nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
  nvic_irq_enable(UART7_IRQn, 0, 0);

  /* configure usart7 param */
  usart_init(UART7, 115200, USART_DATA_8BITS, USART_STOP_1_BIT);
  usart_transmitter_enable(UART7, TRUE);
  usart_receiver_enable(UART7, TRUE);

  /* enable usart7 interrupt */
  usart_interrupt_enable(UART7, USART_RDBF_INT, TRUE);
  usart_interrupt_enable(UART7, USART_TDBE_INT, TRUE);
  
  usart_enable(UART7, TRUE);
}

/**
  * @brief  compares two buffers.
  * @param  pbuffer1, pbuffer2: buffers to be compared.
  * @param  buffer_length: buffer's length
  * @retval 1: pbuffer1 identical to pbuffer2
  *         0: pbuffer1 differs from pbuffer2
  */
uint8_t buffer_compare(uint8_t* pbuffer1, uint8_t* pbuffer2, uint16_t buffer_length)
{
  while(buffer_length--)
  {
    if(*pbuffer1 != *pbuffer2)
    {
      return 0;
    }
    pbuffer1++;
    pbuffer2++;
  }
  return 1;
}

/**
  * @brief  main function.
  * @param  none
  * @retval none
  */

void RCC_SystemClock_Init()
{
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
    crm_pll_parameter_calculate(CRM_PLL_SOURCE_HEXT, 200000000, &pll_ms, &pll_ns, &pll_fr);

    /* config pll clock resource */
    crm_pll_config(CRM_PLL_SOURCE_HEXT, pll_ns, pll_ms, (crm_pll_fr_type)pll_fr);
//    /* config pll clock resource */
//    crm_pll_config(CRM_PLL_SOURCE_HEXT, 72, 1, CRM_PLL_FR_2);

    /* enable pll */
    crm_clock_source_enable(CRM_CLOCK_SOURCE_PLL, TRUE);

    /* wait till pll is ready */
    while(crm_flag_get(CRM_PLL_STABLE_FLAG) != SET)
    {
    }

    /* config ahbclk */
    crm_ahb_div_set(CRM_AHB_DIV_1);

    /* config apb2clk */
    crm_apb2_div_set(CRM_APB2_DIV_2);

    /* config apb1clk */
    crm_apb1_div_set(CRM_APB1_DIV_2);

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

    SysTick_Config(system_core_clock / 1000);
}


int main(void)
{
    int ntest =0;
    char *pTx = "I hope this problem will be resolved quickly.";
    strncpy((char*)usart7_tx_buffer, pTx, strlen(pTx)); 
    
    RCC_SystemClock_Init();
    //HalDrvGpioOpen(0,0,NULL,0,0); // 보드 gpio 초기화 부분
    
  usart_configuration();

  while(1)
  {
      ntest++;
  }
}

void UART7_IRQHandler(void)
{
  //if(usart_flag_get(UART7, USART_RDBF_FLAG) != RESET)
  if( (usart_flag_get(UART7, USART_RDBF_FLAG) != RESET) & (UART7->ctrl1_bit.rdbfien == 1) )
  {
    // read one byte from the receive data register 
    usart7_rx_buffer[usart7_rx_counter++] = usart_data_receive(UART7);
    usart7_rx_counter %= USART7_BUFFER_SIZE;
  }
 
  //if(usart_flag_get(UART7, USART_TDBE_FLAG) != RESET)
  if( (usart_flag_get(UART7, USART_TDBE_FLAG) != RESET) & (UART7->ctrl1_bit.tdbeien == 1) )
  {
    // write one byte to the transmit data register 
    if ( usart7_tx_counter >= strlen((char*)usart7_tx_buffer) )
    {
         usart_interrupt_enable(UART7, USART_TDBE_INT, FALSE);
    }
    else
        usart_data_transmit(UART7, usart7_tx_buffer[usart7_tx_counter++]);

  }

}
