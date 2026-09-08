/**
  **************************************************************************
  * @file     main.c
  * @version  v2.0.4
  * @date     2021-12-31
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

#include "at32f435_437_board.h"
#include "at32f435_437_clock.h"

#include "HalMiscDriver.h"
/** @addtogroup AT32F435_periph_examples
  * @{
  */
  
/** @addtogroup 435_USART_transfer_by_dma_polling USART_transfer_by_dma_polling
  * @{
  */

#define COUNTOF(a)                       (sizeof(a) / sizeof(*(a)))
#define USART2_TX_BUFFER_SIZE            1800//(COUNTOF(usart2_tx_buffer) - 1)
#define USART3_TX_BUFFER_SIZE            (COUNTOF(usart3_tx_buffer) - 1)

uint8_t usart2_tx_buffer[] = "usart transfer by dma interrupt: usart2 -> usart3 using dma";
uint8_t usart3_tx_buffer[] = "usart transfer by dma interrupt: usart3 -> usart2 using dma";
uint8_t usart2_rx_buffer[USART3_TX_BUFFER_SIZE];
uint8_t usart3_rx_buffer[USART2_TX_BUFFER_SIZE];

extern uint8_t g_aUART2DMARxBuffer[USART2_TX_BUFFER_SIZE];

/**
  * @brief  config usart   
  * @param  none
  * @retval none
  */
void usart_configuration(void)
{
  gpio_init_type gpio_init_struct;
  
  /* enable the usart2 and gpio clock */
  crm_periph_clock_enable(CRM_USART2_PERIPH_CLOCK, TRUE);  
  crm_periph_clock_enable(CRM_GPIOD_PERIPH_CLOCK, TRUE);
  
  /* enable the usart3 and gpio clock */  
//  crm_periph_clock_enable(CRM_USART3_PERIPH_CLOCK, TRUE);  
//  crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);    

  gpio_default_para_init(&gpio_init_struct);
  /* configure the usart2 tx, rx pin */
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type  = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = GPIO_PINS_5 | GPIO_PINS_6;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOD, &gpio_init_struct);
  
  gpio_init_struct.gpio_out_type  = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_pins = GPIO_PINS_3 | GPIO_PINS_4;
  gpio_init(GPIOD, &gpio_init_struct);
  
  gpio_bits_write((gpio_type*)GPIOD, GPIO_PINS_3, (confirm_state)FALSE);
  gpio_bits_write((gpio_type*)GPIOD, GPIO_PINS_3, (confirm_state)FALSE);
  
  gpio_pin_mux_config(GPIOD, GPIO_PINS_SOURCE5, GPIO_MUX_7);
  gpio_pin_mux_config(GPIOD, GPIO_PINS_SOURCE6, GPIO_MUX_7);

  
   /* config usart nvic interrupt */
  nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
  nvic_irq_enable(USART2_IRQn, HAL_UART_NVIC_IRQChannelPreemptionPriority, HAL_UART2_MODEM_NVIC_IRQChannelSubPriority);
 
  
  /* configure usart2 param */
#ifdef RF_COMMON_MODEM	 //mod.kks 21.10.25  need to set the initial value on the factory line.
  usart_init(USART2, 115200, USART_DATA_8BITS, USART_STOP_1_BIT);
#else
  usart_init(USART2, 921600, USART_DATA_8BITS, USART_STOP_1_BIT);
#endif
  usart_transmitter_enable(USART2, TRUE);
  usart_receiver_enable(USART2, TRUE);
  usart_dma_transmitter_enable(USART2, FALSE);
  usart_dma_receiver_enable(USART2, TRUE);  
  
  
  
  
  usart_enable(USART2, TRUE);
  usart_interrupt_enable(USART2, USART_RDBF_INT, FALSE);
    usart_interrupt_enable(USART2, USART_TDBE_INT, TRUE);
}

/**
  * @brief  config dma for usart2 and usart3   
  * @param  none
  * @retval none
  */
void dma_configuration(void)
{
  dma_init_type dma_init_struct;  
  
  /* enable dma1 clock */
  crm_periph_clock_enable(CRM_DMA1_PERIPH_CLOCK, TRUE);  

  dmamux_enable(DMA1, TRUE);

  
  /* dma1 channel2 for usart2 rx configuration */
  dma_reset(DMA1_CHANNEL6);
  dma_default_para_init(&dma_init_struct);  
  dma_init_struct.buffer_size = USART3_TX_BUFFER_SIZE;
  dma_init_struct.direction = DMA_DIR_PERIPHERAL_TO_MEMORY;
  dma_init_struct.memory_base_addr = (uint32_t)g_aUART2DMARxBuffer;
  dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_BYTE;
  dma_init_struct.memory_inc_enable = TRUE;
  dma_init_struct.peripheral_base_addr = (uint32_t)&USART2->dt;
  dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_BYTE;
  dma_init_struct.peripheral_inc_enable = FALSE;
  dma_init_struct.priority = DMA_PRIORITY_MEDIUM;
  dma_init_struct.loop_mode_enable = FALSE;
  dma_init(DMA1_CHANNEL6, &dma_init_struct);
  
  /* config flexible dma for usart2 rx */
  dmamux_init(DMA1MUX_CHANNEL6, DMAMUX_DMAREQ_ID_USART2_RX);

  
  dma_channel_enable(DMA1_CHANNEL6, TRUE); /* usart2 rx begin dma receiving */
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

#if 0
/**
  * @brief  main function.
  * @param  none
  * @retval none
  */
int main(void)
{
  //system_clock_config();
  //  system_clock_config_GIT();
  //at32_board_init();
//HalDrvGpioOpen(0,0,NULL,0,0);
HalHandlerInit();
  usart_configuration();
  dma_configuration();
        
  /* wait dma transmission complete */  
//  while((dma_flag_get(DMA1_FDT1_FLAG) == RESET) || (dma_flag_get(DMA1_FDT2_FLAG) == RESET) || \
//        (dma_flag_get(DMA1_FDT3_FLAG) == RESET) || (dma_flag_get(DMA1_FDT4_FLAG) == RESET));
  
  
  while(1)
  { 
    /* compare data buffer */ 
    if(buffer_compare(usart2_tx_buffer, usart3_rx_buffer, USART2_TX_BUFFER_SIZE) && \
       buffer_compare(usart3_tx_buffer, usart2_rx_buffer, USART3_TX_BUFFER_SIZE))
    {
//      at32_led_toggle(LED2);
//      at32_led_toggle(LED3);
//      at32_led_toggle(LED4);      
//      delay_sec(1);
    }
  }
}
/*
void USART2_IRQHandler(void)
{
  if(usart_flag_get(USART2, USART_RDBF_FLAG) != RESET)
  {
    usart_interrupt_enable(USART2, USART_RDBF_INT, FALSE);
  }

  if(usart_flag_get(USART2, USART_TDBE_FLAG) != RESET)
  {
      usart_interrupt_enable(USART2, USART_TDBE_INT, FALSE);
  }
}
*/
#endif