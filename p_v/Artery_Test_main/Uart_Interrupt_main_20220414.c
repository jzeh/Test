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

/** @addtogroup AT32F435_periph_examples
  * @{
  */

/** @addtogroup 435_USART_interrupt USART_interrupt
  * @{
  */

#define COUNTOF(a)                       (sizeof(a) / sizeof(*(a)))
#define USART2_TX_BUFFER_SIZE            (COUNTOF(usart2_tx_buffer) - 1)
#define USART3_TX_BUFFER_SIZE            (COUNTOF(usart3_tx_buffer) - 1)

uint8_t usart2_tx_buffer[] = "usart transfer by interrupt: usart2 -> usart3 using interrupt";
uint8_t usart3_tx_buffer[] = "usart transfer by interrupt: usart3 -> usart2 using interrupt";
uint8_t usart2_rx_buffer[USART3_TX_BUFFER_SIZE];
uint8_t usart3_rx_buffer[USART2_TX_BUFFER_SIZE];
volatile uint8_t usart2_tx_counter = 0x00;
volatile uint8_t usart3_tx_counter = 0x00;
volatile uint8_t usart2_rx_counter = 0x00;
volatile uint8_t usart3_rx_counter = 0x00;
uint8_t usart2_tx_buffer_size = USART2_TX_BUFFER_SIZE;
uint8_t usart3_tx_buffer_size = USART3_TX_BUFFER_SIZE;

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
  crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);

  /* enable the usart3 and gpio clock */
  crm_periph_clock_enable(CRM_USART3_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);

  gpio_default_para_init(&gpio_init_struct);

  /* configure the usart2 tx, rx pin */
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type  = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pins = GPIO_PINS_2 | GPIO_PINS_3;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init(GPIOA, &gpio_init_struct);
  gpio_pin_mux_config(GPIOA, GPIO_PINS_SOURCE2, GPIO_MUX_7);
  gpio_pin_mux_config(GPIOA, GPIO_PINS_SOURCE3, GPIO_MUX_7);

  /* configure the usart3 tx, rx pin */
  gpio_init_struct.gpio_pins = GPIO_PINS_10 | GPIO_PINS_11;
  gpio_init(GPIOB, &gpio_init_struct);
  gpio_pin_mux_config(GPIOB, GPIO_PINS_SOURCE10, GPIO_MUX_7);
  gpio_pin_mux_config(GPIOB, GPIO_PINS_SOURCE11, GPIO_MUX_7);

  /* config usart nvic interrupt */
  nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
  nvic_irq_enable(USART2_IRQn, 0, 0);
  nvic_irq_enable(USART3_IRQn, 0, 0);

  /* configure usart2 param */
  usart_init(USART2, 115200, USART_DATA_8BITS, USART_STOP_1_BIT);
  usart_transmitter_enable(USART2, TRUE);
  usart_receiver_enable(USART2, TRUE);

  /* configure usart3 param */
  usart_init(USART3, 115200, USART_DATA_8BITS, USART_STOP_1_BIT);
  usart_transmitter_enable(USART3, TRUE);
  usart_receiver_enable(USART3, TRUE);

  /* enable usart2 and usart3 interrupt */
  usart_interrupt_enable(USART2, USART_RDBF_INT, TRUE);
  usart_enable(USART2, TRUE);

  usart_interrupt_enable(USART3, USART_RDBF_INT, TRUE);
  usart_enable(USART3, TRUE);

  usart_interrupt_enable(USART2, USART_TDBE_INT, TRUE);
  usart_interrupt_enable(USART3, USART_TDBE_INT, TRUE);
}

void usart_configuration_uart7(void)
{
  gpio_init_type gpio_init_struct;

  /* enable the usart7 and gpio clock */
  crm_periph_clock_enable(CRM_UART7_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_GPIOE_PERIPH_CLOCK, TRUE);

  gpio_default_para_init(&gpio_init_struct);

  /* configure the usart7 tx, rx pin */
	//PE7:UART7_RX, PE8:UART7_TX
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
int main(void)
{
  //system_clock_config();
	
    system_clock_config_GIT();
	//system_clock_config_200mhz();
	
  //at32_board_init();
	delay_init();
	
	//uart_print_init(115200);
	//printf("\r\n***F435_TEST\r\n");
	
  //usart_configuration();
	usart_configuration_uart7();
	
	//usart_interrupt_enable(UART7, USART_TDBE_INT, TRUE);
//	usart_data_transmit(UART7, 'a');
	
	
	//test
	while(1)
	{
		//usart_data_transmit(UART7, 'a');
		//delay_ms(500);
	}
	
	

  /* wait until end of transmission from usart2 to usart3 */
  while(usart3_rx_counter < usart2_tx_buffer_size);

  /* wait until end of transmission from usart3 to usart2 */
  while(usart2_rx_counter < usart3_tx_buffer_size);

  while(1)
  {
    /* compare data buffer */
    if(buffer_compare(usart2_tx_buffer, usart3_rx_buffer, USART2_TX_BUFFER_SIZE) && \
       buffer_compare(usart3_tx_buffer, usart2_rx_buffer, USART3_TX_BUFFER_SIZE))
    {
      at32_led_toggle(LED2);
      at32_led_toggle(LED3);
      at32_led_toggle(LED4);
      delay_sec(1);
    }
  }
}

/**
  * @}
  */

/**
  * @}
  */

  
  void UART7_IRQHandler(void)
  {
      uint8_t ui8Temp=0;;
      
    if(usart_flag_get(UART7, USART_RDBF_FLAG) != RESET)
    {
      // read one byte from the receive data register 
      //usart7_rx_buffer[usart7_rx_counter++] = usart_data_receive(UART7);
      //usart7_rx_counter %= USART7_RX_BUFFER_SIZE;
          
          ui8Temp = usart_data_receive(UART7);
          //usart_data_transmit(UART7, ui8Temp);
          //printf("%c",ui8Temp);
          
    }
  
    if(usart_flag_get(UART7, USART_TDBE_FLAG) != RESET)
    {
          //usart_data_transmit(UART7, 'a');
          usart_interrupt_enable(UART7, USART_TDBE_INT, FALSE);
          
      // write one byte to the transmit data register 
  /* 
        usart_data_transmit(UART7, usart7_tx_buffer[usart7_tx_counter++]);
      if ( usart7_tx_counter >= strlen(usart7_tx_buffer) )
      {
           usart_interrupt_enable(UART7, USART_TDBE_INT, FALSE);
      }
  */
    }
  }

