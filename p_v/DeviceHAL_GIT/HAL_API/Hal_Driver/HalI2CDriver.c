/*
  ******************************************************************************
  * @file    HalI2CDriver.c
  * @author  James Jean
  * @version V1.0.0
  * @date    2022-03-02
  * @brief
  *
  *
  ******************************************************************************
*/
/* Includes ------------------------------------------------------------------*/
#include "HalHandler.h"
#include "HalI2CDriver.h"
#if defined(STM32F427X)
#include "STM32F4xx.h"
#include "STM32F4xx_i2c.h"
#elif defined(AT32F435VMT7)
#include "at32f435_437.h"
#include "at32f435_437_i2c.h"
#endif

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define I2C_RW_TIMEOUT_CNT	(0x5000)

#if defined(AT32F435VMT7)
/////////////////////////////////////////////////////////////////////////////////////////
// 기존 STM의 I2C read/Write 구조로 ARTERY I2C 동작 불가로 인한 ARTERY용 별도 함수 적용// 
/////////////////////////////////////////////////////////////////////////////////////////
#define I2C_EVENT_CHECK_NONE             ((uint32_t)0x00000000)    /*!< check flag none */
#define I2C_EVENT_CHECK_ACKFAIL          ((uint32_t)0x00000001)    /*!< check flag ackfail */
#define I2C_EVENT_CHECK_STOP             ((uint32_t)0x00000002)    /*!< check flag stop */
#endif
/* Private macro -------------------------------------------------------------*/
typedef enum {HAL_I2C_RETURN_SUCCESS = 1, HAL_I2C_RETURN_FAIL = 0} eHalI2cRetStatus;
#if defined(AT32F435VMT7)
/////////////////////////////////////////////////////////////////////////////////////////
// 기존 STM의 I2C read/Write 구조로 ARTERY I2C 동작 불가로 인한 ARTERY용 별도 함수 적용// 
/////////////////////////////////////////////////////////////////////////////////////////
// @defgroup I2C_library_transmission_mode
typedef enum 
{
  I2C_INT_MA_TX = 0,
  I2C_INT_MA_RX,
  I2C_INT_SLA_TX,
  I2C_INT_SLA_RX,
  I2C_DMA_MA_TX,
  I2C_DMA_MA_RX,
  I2C_DMA_SLA_TX,
  I2C_DMA_SLA_RX,  
} i2c_mode_type;
// @defgroup I2C_library_status_code
typedef enum 
{
  I2C_OK = 0,          /*!< no error */
  I2C_ERR_STEP_1,      /*!< step 1 error */
  I2C_ERR_STEP_2,      /*!< step 2 error */
  I2C_ERR_STEP_3,      /*!< step 3 error */
  I2C_ERR_STEP_4,      /*!< step 4 error */
  I2C_ERR_STEP_5,      /*!< step 5 error */
  I2C_ERR_STEP_6,      /*!< step 6 error */
  I2C_ERR_STEP_7,      /*!< step 7 error */
  I2C_ERR_STEP_8,      /*!< step 8 error */
  I2C_ERR_STEP_9,      /*!< step 9 error */
  I2C_ERR_STEP_10,     /*!< step 10 error */
  I2C_ERR_STEP_11,     /*!< step 11 error */
  I2C_ERR_STEP_12,     /*!< step 12 error */
  I2C_ERR_TCRLD,       /*!< tcrld error */
  I2C_ERR_TDC,         /*!< tdc error */
  I2C_ERR_ADDR,        /*!< addr error */
  I2C_ERR_STOP,        /*!< stop error */
  I2C_ERR_ACKFAIL,     /*!< ackfail error */
  I2C_ERR_TIMEOUT,     /*!< timeout error */
  I2C_ERR_INTERRUPT,   /*!< interrupt error */
} i2c_status_type;

typedef struct 
{
  i2c_type                               *i2cx;                   /*!< i2c registers base address      */
  uint8_t                                *pbuff;                  /*!< pointer to i2c transfer buffer  */
  __IO uint16_t                          psize;                   /*!< i2c transfer size               */
  __IO uint16_t                          pcount;                  /*!< i2c transfer counter            */
  __IO uint32_t                          mode;                    /*!< i2c communication mode          */
  __IO uint32_t                          status;                  /*!< i2c communication status        */
  __IO i2c_status_type                   error_code;              /*!< i2c error code                  */
  dma_channel_type                       *dma_tx_channel;         /*!< dma transmit channel            */
  dma_channel_type                       *dma_rx_channel;         /*!< dma receive channel             */
  dma_init_type                          dma_init_struct;         /*!< dma init parameters             */
} i2c_handle_type;

#endif

/* Private variables ---------------------------------------------------------*/
#if defined(AT32F435VMT7)
/////////////////////////////////////////////////////////////////////////////////////////
// 기존 STM의 I2C read/Write 구조로 ARTERY I2C 동작 불가로 인한 ARTERY용 별도 함수 적용// 
/////////////////////////////////////////////////////////////////////////////////////////
i2c_handle_type g_hI2Cx;
#endif

/* Private function prototypes -----------------------------------------------*/
void HalI2cInitial(void);
void HalI2C_GenerateStart(unsigned int nI2CChAddr, eHalFunctionalState eHalFState);
void HalI2C_GenerateStop(unsigned int nI2CChAddr, eHalFunctionalState eHalFState);
void HalI2C_SetTransmitAddr(unsigned int nI2CChAddr, unsigned int unI2CAddr, unsigned char unI2CDirection);
void HalI2C_AcknowlegeConfig(unsigned int nI2CChAddr, eHalFunctionalState eHalFState);
eHalFlagStatus HalI2C_GetFlagStatus(unsigned int nI2CChAddr, unsigned int unCheckFlag);
eHalErrorStatus HalI2C_CheckEvent(unsigned int nI2CChAddr, unsigned int unCheckFlag);


/* Private functions ---------------------------------------------------------*/
void HalI2cInitial(void)
{
	stHalGPIO_InitTypeDef  GPIO_InitStructure;
	stHalI2C_InitTypeDef  I2C_InitStructure;

	//----------------------------------------------------------------------------
	//  initial  I2C
	//----------------------------------------------------------------------------
    HalDrvRccIOCtrl(eRCC_IO_I2C_Clock, eRCC_Clock_I2C1, NULL, 0, HAL_ENABLE);
    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOB_GROUP, NULL, 0, HAL_ENABLE);

    HalDrvRccIOCtrl(eRCC_IO_SYSCFG_Clock, 0, NULL, 0, HAL_ENABLE);

    HalDrvRccIOCtrl(eRCC_IO_I2C_RESET_Clock, eRCC_Clock_I2C1, NULL, 0, HAL_ENABLE);
    HalDrvRccIOCtrl(eRCC_IO_I2C_RESET_Clock, eRCC_Clock_I2C1, NULL, 0, HAL_DISABLE);
    
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, HAL_I2C_SCL_PIN, NULL, 0, HAL_GPIO_AF_I2C1);
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, HAL_I2C_SDA_PIN, NULL, 0, HAL_GPIO_AF_I2C1);

	GPIO_InitStructure.GPIO_DS    = eGPIO_DRIVE_STRENGTH_STRONGER;
	GPIO_InitStructure.GPIO_Mode  = eGPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Pin   = HAL_I2C_SCL_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	GPIO_InitStructure.GPIO_OType = eGPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Pin   = HAL_I2C_SDA_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	I2C_InitStructure.I2C_Mode          = HAL_I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle     = HAL_I2C_DutyCycle_2;
	I2C_InitStructure.I2C_Ack           = HAL_I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = HAL_I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed    = HAL_I2C_SPEED;

    HalDrvI2CIOCtrl(eI2C_IO_Init, (int)HAL_I2C_1, (char*)&I2C_InitStructure, sizeof(I2C_InitStructure), 0);
    HalDrvI2CIOCtrl(eI2C_IO_Enable, (int)HAL_I2C_1, NULL, 0, HAL_ENABLE);

	return;
}

void HalDrvI2C_SetInitialForGPIO(void)
{
	stHalGPIO_InitTypeDef  GPIO_InitStructure;

	//----------------------------------------------------------------------------
	//  initial  I2C
	//----------------------------------------------------------------------------
    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOB_GROUP, NULL, 0, HAL_ENABLE);

	GPIO_InitStructure.GPIO_DS    = eGPIO_DRIVE_STRENGTH_STRONGER;
	GPIO_InitStructure.GPIO_Mode  = eGPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Pin   = HAL_I2C_SCL_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	GPIO_InitStructure.GPIO_OType = eGPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Pin   = HAL_I2C_SDA_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	return;
}

#if defined(AT32F435VMT7)
/////////////////////////////////////////////////////////////////////////////////////////
// 기존 STM의 I2C read/Write 구조로 ARTERY I2C 동작 불가로 인한 ARTERY용 별도 함수 적용// 
/////////////////////////////////////////////////////////////////////////////////////////
/** 
  * @brief  start transfer in poll mode or interrupt mode.
  * @param  hi2c: the handle points to the operation information.
  * @param  address: slave address.
  * @param  start_stop: config gen start condition mode.
  *         parameter as following values:
  *         - I2C_WITHOUT_START: transfer data without start condition.
  *         - I2C_GEN_START_READ: read data and generate start.
  *         - I2C_GEN_START_WRITE: send data and generate start.
  * @retval i2c status.
  */
void i2c_start_transfer(i2c_handle_type* hi2c, uint16_t address, i2c_start_stop_mode_type start_stop)
{
  if (hi2c->pcount > MAX_TRANSFER_CNT)
  {
    hi2c->psize = MAX_TRANSFER_CNT;
    
    i2c_transmit_set(hi2c->i2cx, address, hi2c->psize, I2C_RELOAD_MODE, start_stop);
  }
  else
  {
    hi2c->psize = hi2c->pcount;
    
    i2c_transmit_set(hi2c->i2cx, address, hi2c->psize, I2C_AUTO_STOP_MODE, start_stop);
  }
}

/** 
  * @brief  reset ctrl2 register.
  * @param  hi2c: the handle points to the operation information.
  * @retval none.
  */
void i2c_reset_ctrl2_register(i2c_handle_type* hi2c)
{
  hi2c->i2cx->ctrl2_bit.saddr   = 0;    
  hi2c->i2cx->ctrl2_bit.readh10 = 0; 
  hi2c->i2cx->ctrl2_bit.cnt     = 0; 
  hi2c->i2cx->ctrl2_bit.rlden   = 0; 
  hi2c->i2cx->ctrl2_bit.dir     = 0; 
}

/** 
  * @brief  wait for the flag.
  * @param  hi2c: the handle points to the operation information.
  * @param  flag: flag to wait.
  * @param  status: status to wait.
  * @param  event_check: flag to check while waiting for the flag.
  *         parameter as following values:    
  *         - I2C_EVENT_CHECK_NONE
  *         - I2C_EVENT_CHECK_ACKFAIL
  *         - I2C_EVENT_CHECK_STOP
  * @param  timeout: maximum waiting time.
  * @retval i2c status.
  */
i2c_status_type i2c_wait_flag(i2c_handle_type* hi2c, uint32_t flag, flag_status status, uint32_t event_check, uint32_t timeout)
{
  while(i2c_flag_get(hi2c->i2cx, flag) == status)
  {
    /* check the ack fail flag */
    if(event_check & I2C_EVENT_CHECK_ACKFAIL)
    {
      if(hi2c->i2cx->sts & I2C_ACKFAIL_FLAG)
      {
        /* clear ack fail flag */
        i2c_flag_clear(hi2c->i2cx, I2C_ACKFAIL_FLAG);
        
        hi2c->error_code = I2C_ERR_ACKFAIL;
        
        return I2C_ERR_ACKFAIL;
      }
    }

    /* check the stop flag */
    if(event_check & I2C_EVENT_CHECK_STOP)
    {
      if(hi2c->i2cx->sts & I2C_STOPF_FLAG)
      {
        /* clear stop flag */
        i2c_flag_clear(hi2c->i2cx, I2C_STOPF_FLAG);

        i2c_reset_ctrl2_register(hi2c);
        
        hi2c->error_code = I2C_ERR_STOP; 
        
        return I2C_ERR_STOP;
      }    
    }

    /* check timeout */
    if((timeout--) == 0)
    {
      hi2c->error_code = I2C_ERR_TIMEOUT; 
      
      return I2C_ERR_TIMEOUT;
    }
  }
  
  return I2C_OK;
}

/** 
  * @brief  refresh i2c register.
  * @param  hi2c: the handle points to the operation information.
  * @retval none.
  */
void i2c_refresh_txdt_register(i2c_handle_type* hi2c)
{
  /* clear tdis flag */
  if (i2c_flag_get(hi2c->i2cx, I2C_TDIS_FLAG) != RESET)
  {
    hi2c->i2cx->txdt = 0x00;
  }

  /* refresh txdt register*/
  if (i2c_flag_get(hi2c->i2cx, I2C_TDBE_FLAG) == RESET)
  {
    hi2c->i2cx->sts_bit.tdbe = 1;
  }
}

/** 
  * @brief  the master transmits data through polling mode.
  * @param  hi2c: the handle points to the operation information.
  * @param  address: slave address.
  * @param  pdata: data buffer.
  * @param  size: data size.
  * @param  timeout: maximum waiting time.
  * @retval i2c status.
  */
i2c_status_type i2c_master_transmit(i2c_handle_type* hi2c, uint16_t address, uint8_t* pdata, uint16_t size, uint32_t timeout)
{  
  /* initialization parameters */
  hi2c->pbuff = pdata;
  hi2c->pcount = size;

  hi2c->error_code = I2C_OK;
   
  /* wait for the busy falg to be reset */
  if (i2c_wait_flag(hi2c, I2C_BUSYF_FLAG, SET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
  {
    return I2C_ERR_STEP_1;
  }

  /* start transfer */
  i2c_start_transfer(hi2c, address, I2C_GEN_START_WRITE);

  while (hi2c->pcount > 0)
  {
    /* wait for the tdis falg to be set */
    if(i2c_wait_flag(hi2c, I2C_TDIS_FLAG, RESET, I2C_EVENT_CHECK_ACKFAIL, timeout) != I2C_OK)
    {
      return I2C_ERR_STEP_2;
    }
    
    /* send data */
    i2c_data_send(hi2c->i2cx, *hi2c->pbuff++);
    hi2c->psize--;    
    hi2c->pcount--;

    if ((hi2c->psize == 0) && (hi2c->pcount != 0))
    {
      /* wait for the tcrld falg to be set  */
      if (i2c_wait_flag(hi2c, I2C_TCRLD_FLAG, RESET, I2C_EVENT_CHECK_ACKFAIL, timeout) != I2C_OK)
      {
        return I2C_ERR_STEP_3;
      }
      
      /* continue transfer */
      i2c_start_transfer(hi2c, address, I2C_WITHOUT_START);
    }
  }

  /* wait for the stop falg to be set  */
  if(i2c_wait_flag(hi2c, I2C_STOPF_FLAG, RESET, I2C_EVENT_CHECK_ACKFAIL, timeout) != I2C_OK)
  {
    return I2C_ERR_STEP_4;
  }

  /* clear stop flag */
  i2c_flag_clear(hi2c->i2cx, I2C_STOPF_FLAG);

  /* reset ctrl2 register */
  i2c_reset_ctrl2_register(hi2c);

  return I2C_OK;
}

/** 
  * @brief  the master receive data through polling mode.
  * @param  hi2c: the handle points to the operation information.
  * @param  address: slave address.
  * @param  pdata: data buffer.
  * @param  size: data size.
  * @param  timeout: maximum waiting time.
  * @retval i2c status.
  */
i2c_status_type i2c_master_receive(i2c_handle_type* hi2c, uint16_t address, uint8_t* pdata, uint16_t size, uint32_t timeout)
{  
  /* initialization parameters */
  hi2c->pbuff = pdata;
  hi2c->pcount = size;

  hi2c->error_code = I2C_OK;
   
  /* wait for the busy falg to be reset */
  if (i2c_wait_flag(hi2c, I2C_BUSYF_FLAG, SET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
  {
    return I2C_ERR_STEP_1;
  }

  /* start transfer */
  i2c_start_transfer(hi2c, address, I2C_GEN_START_READ);

  while (hi2c->pcount > 0)
  {
    /* wait for the rdbf falg to be set  */
    if(i2c_wait_flag(hi2c, I2C_RDBF_FLAG, RESET, I2C_EVENT_CHECK_ACKFAIL, timeout) != I2C_OK)
    {
      return I2C_ERR_STEP_2;
    }

    /* read data */
    (*hi2c->pbuff++) = i2c_data_receive(hi2c->i2cx);
    hi2c->pcount--;    
    hi2c->psize--;

    if ((hi2c->psize == 0) && (hi2c->pcount != 0))
    {
      /* wait for the tcrld falg to be set  */
      if (i2c_wait_flag(hi2c, I2C_TCRLD_FLAG, RESET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
      {
        return I2C_ERR_STEP_3;
      }
      
      /* continue transfer */
      i2c_start_transfer(hi2c, address, I2C_WITHOUT_START);
    }
  }

  /* wait for the stop falg to be set  */
  if(i2c_wait_flag(hi2c, I2C_STOPF_FLAG, RESET, I2C_EVENT_CHECK_ACKFAIL, timeout) != I2C_OK)
  {
    return I2C_ERR_STEP_4;
  }

  /* clear stop flag */
  i2c_flag_clear(hi2c->i2cx, I2C_STOPF_FLAG);

  /* reset ctrl2 register */
  i2c_reset_ctrl2_register(hi2c);

  return I2C_OK;
}

/** 
  * @brief  the slave receive data through polling mode.
  * @param  hi2c: the handle points to the operation information.
  * @param  pdata: data buffer.
  * @param  size: data size.
  * @param  timeout: maximum waiting time.
  * @retval i2c status.
  */
i2c_status_type i2c_slave_receive(i2c_handle_type* hi2c, uint8_t* pdata, uint16_t size, uint32_t timeout)
{
  /* initialization parameters */
  hi2c->pbuff = pdata;
  hi2c->pcount = size;

  hi2c->error_code = I2C_OK;
 
  /* wait for the busy falg to be reset */
  if(i2c_wait_flag(hi2c, I2C_BUSYF_FLAG, SET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
  {
    return I2C_ERR_STEP_1;
  }
  
  /* enable acknowledge */
  i2c_ack_enable(hi2c->i2cx, TRUE);

  /* wait for the addr falg to be set */
  if (i2c_wait_flag(hi2c, I2C_ADDRF_FLAG, RESET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
  { 
    return I2C_ERR_STEP_2;
  }

  /* clear addr flag */
  i2c_flag_clear(hi2c->i2cx, I2C_ADDRF_FLAG);

  /* wait for the dir falg to be reset */
  if (i2c_wait_flag(hi2c, I2C_SDIR_FLAG, SET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
  {
    /* disable acknowledge */
    i2c_ack_enable(hi2c->i2cx, FALSE);
    
    return I2C_ERR_STEP_3;
  }
  
  while (hi2c->pcount > 0)
  {
    /* wait for the rdbf falg to be set  */
    if(i2c_wait_flag(hi2c, I2C_RDBF_FLAG, RESET, I2C_EVENT_CHECK_STOP, timeout) != I2C_OK) 
    {
      /* disable acknowledge */
      i2c_ack_enable(hi2c->i2cx, FALSE);
      
      /* if data is received, read data */
      if (i2c_flag_get(hi2c->i2cx, I2C_RDBF_FLAG) == SET)
      {
        /* read data */
        (*hi2c->pbuff++) = i2c_data_receive(hi2c->i2cx);
        hi2c->pcount--;
      }
      
      return I2C_ERR_STEP_4;
    }

    /* read data */
    (*hi2c->pbuff++) = i2c_data_receive(hi2c->i2cx);
    hi2c->pcount--;
  }

  /* wait for the stop falg to be set */
  if(i2c_wait_flag(hi2c, I2C_STOPF_FLAG, RESET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
  {
    /* disable acknowledge */
    i2c_ack_enable(hi2c->i2cx, FALSE);
    
    return I2C_ERR_STEP_5;
  }

  /* clear stop flag */
  i2c_flag_clear(hi2c->i2cx, I2C_STOPF_FLAG);

  /* wait for the busy falg to be reset */
  if (i2c_wait_flag(hi2c, I2C_BUSYF_FLAG, SET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
  {
    /* disable acknowledge */
    i2c_ack_enable(hi2c->i2cx, FALSE);
    
    return I2C_ERR_STEP_6;
  }

  return I2C_OK;
}

/** 
  * @brief  the slave transmits data through polling mode.
  * @param  hi2c: the handle points to the operation information.
  * @param  pdata: data buffer.
  * @param  size: data size.
  * @param  timeout: maximum waiting time.
  * @retval i2c status.
  */
i2c_status_type i2c_slave_transmit(i2c_handle_type* hi2c, uint8_t* pdata, uint16_t size, uint32_t timeout)
{
  /* initialization parameters */
  hi2c->pbuff = pdata;
  hi2c->pcount = size;

  hi2c->error_code = I2C_OK;
   
  /* wait for the busy falg to be reset */
  if(i2c_wait_flag(hi2c, I2C_BUSYF_FLAG, SET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
  {
    return I2C_ERR_STEP_1;
  }
  
  /* enable acknowledge */
  i2c_ack_enable(hi2c->i2cx, TRUE);

  /* wait for the addr falg to be set */
  if (i2c_wait_flag(hi2c, I2C_ADDRF_FLAG, RESET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
  {
    /* disable acknowledge */
    i2c_ack_enable(hi2c->i2cx, FALSE);
    return I2C_ERR_STEP_2;
  }

  /* clear addr flag */
  i2c_flag_clear(hi2c->i2cx, I2C_ADDRF_FLAG);

  /* if 10-bit address mode is used */
  if (hi2c->i2cx->ctrl2_bit.addr10 != RESET)
  {
    /* wait for the addr falg to be set */
    if (i2c_wait_flag(hi2c, I2C_ADDRF_FLAG, RESET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
    {
      /* disable acknowledge */
      i2c_ack_enable(hi2c->i2cx, FALSE);
      
      return I2C_ERR_STEP_3;
    }

    /* clear addr flag */
    i2c_flag_clear(hi2c->i2cx, I2C_ADDRF_FLAG);
  }

  /* wait for the dir falg to be set */
  if (i2c_wait_flag(hi2c, I2C_SDIR_FLAG, RESET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
  {
    /* disable acknowledge */
    i2c_ack_enable(hi2c->i2cx, FALSE);
    
    return I2C_ERR_STEP_4;
  }

  while (hi2c->pcount > 0)
  {
    /* wait for the tdis falg to be set */
    if(i2c_wait_flag(hi2c, I2C_TDIS_FLAG, RESET, I2C_EVENT_CHECK_ACKFAIL, timeout) != I2C_OK)
    {
      /* disable acknowledge */
      i2c_ack_enable(hi2c->i2cx, FALSE);
      
      return I2C_ERR_STEP_5;
    }

    /* send data */
    i2c_data_send(hi2c->i2cx, *hi2c->pbuff++);
    hi2c->pcount--;
  }

  /* wait for the ackfail falg to be set */
  if(i2c_wait_flag(hi2c, I2C_ACKFAIL_FLAG, RESET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
  {
    return I2C_ERR_STEP_6;
  }
  
  /* clear ack fail flag */
  i2c_flag_clear(hi2c->i2cx, I2C_ACKFAIL_FLAG);
  
  /* wait for the stop falg to be set */
  if(i2c_wait_flag(hi2c, I2C_STOPF_FLAG, RESET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
  {
    /* disable acknowledge */
    i2c_ack_enable(hi2c->i2cx, FALSE);
    
    return I2C_ERR_STEP_7;
  }

  /* clear stop flag */
  i2c_flag_clear(hi2c->i2cx, I2C_STOPF_FLAG);

  /* wait for the busy falg to be reset */
  if (i2c_wait_flag(hi2c, I2C_BUSYF_FLAG, SET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
  {
    /* disable acknowledge */
    i2c_ack_enable(hi2c->i2cx, FALSE);
    
    return I2C_ERR_STEP_8;
  }

  /* refresh tx dt register */
  i2c_refresh_txdt_register(hi2c);
  
  return I2C_OK;
}

/** 
  * @brief  write data to the memory device through polling mode.
  * @param  hi2c: the handle points to the operation information.
  * @param  address: memory device address.
  * @param  mem_address: memory address.
  * @param  pdata: data buffer.
  * @param  size: data size.
  * @param  timeout: maximum waiting time.
  * @retval i2c status.
  */
i2c_status_type i2c_memory_write(i2c_handle_type* hi2c, uint16_t address, uint16_t mem_address, uint8_t* pdata, uint16_t size, uint32_t timeout)
{
  /* initialization parameters */
  hi2c->pbuff = pdata;
  hi2c->pcount = size + 1;

  hi2c->error_code = I2C_OK;
   
  /* wait for the busy falg to be reset */
  if (i2c_wait_flag(hi2c, I2C_BUSYF_FLAG, SET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
  {
    return I2C_ERR_STEP_1;
  }

  /* start transfer */
  i2c_start_transfer(hi2c, address, I2C_GEN_START_WRITE);
  
  /* wait for the tdis falg to be set */
  if(i2c_wait_flag(hi2c, I2C_TDIS_FLAG, RESET, I2C_EVENT_CHECK_ACKFAIL, timeout) != I2C_OK)
  {
    return I2C_ERR_STEP_2;
  }  

  /* send memory address */
  i2c_data_send(hi2c->i2cx, mem_address);

  hi2c->psize--;
  hi2c->pcount--;

  while (hi2c->pcount > 0)
  {
    /* wait for the tdis falg to be set */
    if(i2c_wait_flag(hi2c, I2C_TDIS_FLAG, RESET, I2C_EVENT_CHECK_ACKFAIL, timeout) != I2C_OK)
    {
      return I2C_ERR_STEP_3;
    }
    
    /* send data */
    i2c_data_send(hi2c->i2cx, *hi2c->pbuff++);
    hi2c->psize--;    
    hi2c->pcount--;

    if ((hi2c->psize == 0) && (hi2c->pcount != 0))
    {
      /* wait for the tcrld falg to be set  */
      if (i2c_wait_flag(hi2c, I2C_TCRLD_FLAG, RESET, I2C_EVENT_CHECK_ACKFAIL, timeout) != I2C_OK)
      {
        return I2C_ERR_STEP_4;
      }
      
      /* continue transfer */
      i2c_start_transfer(hi2c, address, I2C_WITHOUT_START);
    }
  }

  /* wait for the stop falg to be set  */
  if(i2c_wait_flag(hi2c, I2C_STOPF_FLAG, RESET, I2C_EVENT_CHECK_ACKFAIL, timeout) != I2C_OK)
  {
    return I2C_ERR_STEP_5;
  }

  /* clear stop flag */
  i2c_flag_clear(hi2c->i2cx, I2C_STOPF_FLAG);

  /* reset ctrl2 register */
  i2c_reset_ctrl2_register(hi2c);

  return I2C_OK;
}

/** 
  * @brief  read data from memory device through polling mode.
  * @param  hi2c: the handle points to the operation information.
  * @param  address: memory device address.
  * @param  mem_address: memory address.
  * @param  pdata: data buffer.
  * @param  size: data size.
  * @param  timeout: maximum waiting time.
  * @retval i2c status.
  */
i2c_status_type i2c_memory_read(i2c_handle_type* hi2c, uint16_t address, uint16_t mem_address, uint8_t* pdata, uint16_t size, uint32_t timeout)
{
  /* initialization parameters */
  hi2c->pbuff = pdata;
  hi2c->pcount = size;

  hi2c->error_code = I2C_OK;
   
  /* wait for the busy falg to be reset */
  if(i2c_wait_flag(hi2c, I2C_BUSYF_FLAG, SET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
  {
    return I2C_ERR_STEP_1;
  }

  /* start transfer */
  i2c_transmit_set(hi2c->i2cx, address, 1, I2C_SOFT_STOP_MODE, I2C_GEN_START_WRITE);

  /* wait for the tdis falg to be set */
  if(i2c_wait_flag(hi2c, I2C_TDIS_FLAG, RESET, I2C_EVENT_CHECK_ACKFAIL, timeout) != I2C_OK)
  {
    return I2C_ERR_STEP_2;
  }  

  /* send memory address */
  i2c_data_send(hi2c->i2cx, mem_address);

  /* wait for the tdc falg to be set */
  if (i2c_wait_flag(hi2c, I2C_TDC_FLAG, RESET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
  {
    return I2C_ERR_STEP_3;
  }

  /* start transfer */
  i2c_start_transfer(hi2c, address, I2C_GEN_START_READ); 
  
  while (hi2c->pcount > 0)
  {
    /* wait for the rdbf falg to be set  */
    if (i2c_wait_flag(hi2c, I2C_RDBF_FLAG, RESET, I2C_EVENT_CHECK_ACKFAIL, timeout) != I2C_OK)
    {
      return I2C_ERR_STEP_4;
    }

    /* read data */
    (*hi2c->pbuff++) = i2c_data_receive(hi2c->i2cx);
    hi2c->pcount--;    
    hi2c->psize--;

    if ((hi2c->psize == 0) && (hi2c->pcount != 0))
    {
      /* wait for the tcrld falg to be set  */
      if (i2c_wait_flag(hi2c, I2C_TCRLD_FLAG, RESET, I2C_EVENT_CHECK_NONE, timeout) != I2C_OK)
      {
        return I2C_ERR_STEP_5;
      }
      
      /* continue transfer */
      i2c_start_transfer(hi2c, address, I2C_WITHOUT_START);
    }
  }
  
  /* wait for the stop falg to be set  */
  if (i2c_wait_flag(hi2c, I2C_STOPF_FLAG, RESET, I2C_EVENT_CHECK_ACKFAIL, timeout) != I2C_OK)
  {
    return I2C_ERR_STEP_6;
  }

  /* clear stop flag */
  i2c_flag_clear(hi2c->i2cx, I2C_STOPF_FLAG);
  
  /* reset ctrl2 register */
  i2c_reset_ctrl2_register(hi2c);
  
  return I2C_OK;
}

#endif

void HalI2C_GenerateStart(unsigned int nI2CChAddr, eHalFunctionalState eHalFState)
{
    HalDrvI2CIOCtrl(eI2C_IO_GenerateStart, nI2CChAddr, NULL, 0, eHalFState);
}

void HalI2C_GenerateStop(unsigned int nI2CChAddr, eHalFunctionalState eHalFState)
{
    HalDrvI2CIOCtrl(eI2C_IO_GenerateStop, nI2CChAddr, NULL, 0, eHalFState);
}

eHalFlagStatus HalI2C_GetFlagStatus(unsigned int nI2CChAddr, unsigned int unCheckFlag)
{
    return (eHalFlagStatus)HalDrvI2CIOCtrl(eI2C_IO_GetFlagStatus, nI2CChAddr, NULL, 0, unCheckFlag);
}
eHalErrorStatus HalI2C_CheckEvent(unsigned int nI2CChAddr, unsigned int unCheckFlag)
{
    return (eHalErrorStatus)HalDrvI2CIOCtrl(eI2C_IO_CheckEvent, nI2CChAddr, NULL, 0, unCheckFlag);
}

void HalI2C_SetTransmitAddr(unsigned int nI2CChAddr, unsigned int unI2CAddr, unsigned char unI2CDirection)
{
    HalDrvI2CIOCtrl(eI2C_IO_SetTransferAddr, nI2CChAddr, NULL, unI2CAddr, unI2CDirection);
}

void HalI2C_AcknowlegeConfig(unsigned int nI2CChAddr, eHalFunctionalState eHalFState)
{
    HalDrvI2CIOCtrl(eI2C_IO_SetAcknowlegeConfig, nI2CChAddr, NULL, 0, eHalFState);
}

unsigned char I2C_Write(unsigned char ucDeviceAddr, unsigned char ucRegAddr, unsigned char* pucData, unsigned char ucLength)
{
    return HalDrvI2CWrite((int)HAL_I2C_1, ucDeviceAddr, (char*)pucData, ucLength, ucRegAddr);
}
unsigned char I2C_Read(unsigned char ucDeviceAddr, unsigned char ucRegAddr, unsigned char* pucData, unsigned char ucLength)
{
    return HalDrvI2CRead((int)HAL_I2C_1, ucDeviceAddr, (char*)pucData, ucLength, ucRegAddr);
}

#if defined(AT32F435VMT7)
unsigned int HalDrvI2C_ConvertFlag(unsigned int unHalI2CFlags)
{
    unsigned int unI2CCvtFlags = 0;
    
    if ( unHalI2CFlags & HAL_I2C_FLAG_BUSY     ) unI2CCvtFlags += I2C_BUSYF_FLAG;
    if ( unHalI2CFlags & HAL_I2C_FLAG_SMBALERT ) unI2CCvtFlags += I2C_ALERTF_FLAG;
    if ( unHalI2CFlags & HAL_I2C_FLAG_TIMEOUT  ) unI2CCvtFlags += I2C_TMOUT_FLAG;
    if ( unHalI2CFlags & HAL_I2C_FLAG_PECERR   ) unI2CCvtFlags += I2C_PECERR_FLAG;
    if ( unHalI2CFlags & HAL_I2C_FLAG_OVR      ) unI2CCvtFlags += I2C_OUF_FLAG;
    if ( unHalI2CFlags & HAL_I2C_FLAG_AF       ) unI2CCvtFlags += I2C_ACKFAIL_FLAG;
    if ( unHalI2CFlags & HAL_I2C_FLAG_ARLO     ) unI2CCvtFlags += I2C_ARLOST_FLAG;
    if ( unHalI2CFlags & HAL_I2C_FLAG_BERR     ) unI2CCvtFlags += I2C_BUSERR_FLAG;
    if ( unHalI2CFlags & HAL_I2C_FLAG_TXE      ) unI2CCvtFlags += I2C_TDBE_FLAG;
    if ( unHalI2CFlags & HAL_I2C_FLAG_RXNE     ) unI2CCvtFlags += I2C_RDBF_FLAG;
    if ( unHalI2CFlags & HAL_I2C_FLAG_STOPF    ) unI2CCvtFlags += I2C_STOPF_FLAG;
    if ( unHalI2CFlags & HAL_I2C_FLAG_BTF      ) unI2CCvtFlags += I2C_TDC_FLAG;
    if ( unHalI2CFlags & HAL_I2C_FLAG_ADDR     ) unI2CCvtFlags += I2C_ADDRF_FLAG;

    return unI2CCvtFlags;
/*
    if ( unHalI2CFlags & HAL_I2C_FLAG_DUALF    ) unI2CCvtFlags += ;
    if ( unHalI2CFlags & HAL_I2C_FLAG_SMBHOST  ) unI2CCvtFlags += ;
    if ( unHalI2CFlags & HAL_I2C_FLAG_SMBDEFAULT)unI2CCvtFlags += ;
    if ( unHalI2CFlags & HAL_I2C_FLAG_GENCALL  ) unI2CCvtFlags += ;
    if ( unHalI2CFlags & HAL_I2C_FLAG_TRA      ) unI2CCvtFlags += ;
    if ( unHalI2CFlags & HAL_I2C_FLAG_ADD10    ) unI2CCvtFlags += ;
    if ( unHalI2CFlags & HAL_I2C_FLAG_MSL      ) unI2CCvtFlags += ;
    if ( unHalI2CFlags & HAL_I2C_FLAG_SB       ) unI2CCvtFlags += ;
*/

/*
//  AT32F435에서 변환하지 못한 플래그
*         - I2C_TDIS_FLAG: send interrupt status.
*         - I2C_TCRLD_FLAG: transmission is complete, waiting to load data.
*         - I2C_SDIR_FLAG: slave data transmit direction.
*/
}
#endif 
//----------------------------------------------------------------------------------//
int HalDrvI2COpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    HalI2cInitial();
#if defined(AT32F435VMT7)
    /////////////////////////////////////////////////////////////////////////////////////////
    // 기존 STM의 I2C read/Write 구조로 ARTERY I2C 동작 불가로 인한 ARTERY용 별도 함수 적용// 
    /////////////////////////////////////////////////////////////////////////////////////////
    g_hI2Cx.i2cx = HAL_I2C_1;
#endif
     

    return HAL_I2C_RETURN_SUCCESS;
}

int HalDrvI2CRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    unsigned char ucDeviceAddress = nRparam;
    unsigned char   ucI2CRegAddr = nOverlap;
    unsigned short usI2Ctimeout = I2C_RW_TIMEOUT_CNT;
    
#if defined(STM32F427X)
    unsigned int unI2C_channelAddr = nLparam;

    while( HalI2C_GetFlagStatus(unI2C_channelAddr, HAL_I2C_FLAG_BUSY) == HAL_SET ) {
        if( (usI2Ctimeout--) == 0 )
        {
            printf("@");
            return HAL_I2C_RETURN_FAIL;
        }
    }

    //------------------gernerate start
    HalI2C_GenerateStart(unI2C_channelAddr, HAL_ENABLE);
    usI2Ctimeout = I2C_RW_TIMEOUT_CNT;

    while( !HalI2C_CheckEvent(unI2C_channelAddr, HAL_I2C_EVENT_MASTER_MODE_SELECT) ) {
        if ( (usI2Ctimeout--) == 0 ) return HAL_I2C_RETURN_FAIL;
    }
    //------------------transmite device addr (WR)
    HalI2C_SetTransmitAddr(unI2C_channelAddr, ucDeviceAddress, HAL_I2C_Direction_Transmitter);
    usI2Ctimeout = I2C_RW_TIMEOUT_CNT;

    while( !HalI2C_CheckEvent(unI2C_channelAddr, HAL_I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) ) {
       if ( (usI2Ctimeout--) == 0 )return HAL_I2C_RETURN_FAIL;
    }
        //-----------------transmite reg addr

#if defined(STM32F427X)
        I2C_SendData((I2C_TypeDef*)unI2C_channelAddr, ucI2CRegAddr);
#elif defined(AT32F435VMT7)
        i2c_data_send((i2c_type*)unI2C_channelAddr, ucI2CRegAddr);
#endif    

        
    usI2Ctimeout = I2C_RW_TIMEOUT_CNT;
    while( HalI2C_GetFlagStatus(unI2C_channelAddr, HAL_I2C_FLAG_BTF) == HAL_RESET ) {
       if ( (usI2Ctimeout--) == 0 )return HAL_I2C_RETURN_FAIL;
    }


    //----------------gernerate restart
    HalI2C_GenerateStart(unI2C_channelAddr, HAL_ENABLE);
    usI2Ctimeout = I2C_RW_TIMEOUT_CNT;
    while( !HalI2C_CheckEvent(unI2C_channelAddr, HAL_I2C_EVENT_MASTER_MODE_SELECT) ) {
        if ( (usI2Ctimeout--) == 0 )return HAL_I2C_RETURN_FAIL;
    }
    //------------------transmite device addr (RD)
    HalI2C_SetTransmitAddr(unI2C_channelAddr, ucDeviceAddress, HAL_I2C_Direction_Receiver);
    usI2Ctimeout=I2C_RW_TIMEOUT_CNT;

    while( !HalI2C_CheckEvent(unI2C_channelAddr, HAL_I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) ) {
        if ( (usI2Ctimeout--) == 0 )return HAL_I2C_RETURN_FAIL;
    }
    HalI2C_AcknowlegeConfig(unI2C_channelAddr, HAL_ENABLE);

    while( nLength-- ) {
        if (!nLength) 
        {
            HalI2C_AcknowlegeConfig(unI2C_channelAddr, HAL_DISABLE);

            HalI2C_GenerateStop(unI2C_channelAddr, HAL_ENABLE);
            usI2Ctimeout = I2C_RW_TIMEOUT_CNT;
            while( HalI2C_GetFlagStatus(unI2C_channelAddr, HAL_I2C_FLAG_RXNE) == HAL_RESET ) {
                if ( (usI2Ctimeout--) == 0 )
                return HAL_I2C_RETURN_FAIL;
            }
#if defined(STM32F427X)
            *pBuffer = I2C_ReceiveData((I2C_TypeDef*)unI2C_channelAddr);
#elif defined(AT32F435VMT7)
            *pBuffer = i2c_data_receive((i2c_type*)unI2C_channelAddr);
#endif  
            break;
        }
        else
        {
            usI2Ctimeout=I2C_RW_TIMEOUT_CNT;
            while( !HalI2C_CheckEvent(unI2C_channelAddr, HAL_I2C_EVENT_MASTER_BYTE_RECEIVED) ) {
                if ( (usI2Ctimeout--) == 0 )
                return HAL_I2C_RETURN_FAIL;
            }
#if defined(STM32F427X)
            *pBuffer = I2C_ReceiveData((I2C_TypeDef*)unI2C_channelAddr);
#elif defined(AT32F435VMT7)
            *pBuffer = i2c_data_receive((i2c_type*)unI2C_channelAddr);
#endif  
            pBuffer++;
        }
    }
    
    HalI2C_AcknowlegeConfig(unI2C_channelAddr, HAL_ENABLE);
#elif defined(AT32F435VMT7)
{
    if ( i2c_memory_read(&g_hI2Cx, ucDeviceAddress, 
                            (uint16_t)ucI2CRegAddr, 
                            (unsigned char*)pBuffer, nLength, usI2Ctimeout) != I2C_OK )
        return HAL_I2C_RETURN_FAIL;


//    if ( i2c_master_transmit(&g_hI2Cx, ucDeviceAddress, (unsigned char*)&nOverlap, 1, usI2Ctimeout) != I2C_OK )
//        return HAL_I2C_RETURN_FAIL;

//    if ( i2c_master_receive(&g_hI2Cx, ucDeviceAddress, (unsigned char*)pBuffer, nLength, usI2Ctimeout) != I2C_OK )
//        return HAL_I2C_RETURN_FAIL;
}
#endif

    return HAL_I2C_RETURN_SUCCESS;
}

int HalDrvI2CWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    unsigned char ucDeviceAddress = nRparam;
    unsigned char   ucI2CRegAddr = nOverlap;
    unsigned short usI2Ctimeout = I2C_RW_TIMEOUT_CNT;
#if defined(STM32F427X)
    unsigned int unI2C_channelAddr = nLparam;
        
    while( HalI2C_GetFlagStatus(unI2C_channelAddr, HAL_I2C_FLAG_BUSY) == HAL_SET ) {
        if((usI2Ctimeout--) == 0) return HAL_I2C_RETURN_FAIL;
    }

    HalI2C_GenerateStart(unI2C_channelAddr, HAL_ENABLE);

    usI2Ctimeout = I2C_RW_TIMEOUT_CNT;
    while( !HalI2C_CheckEvent(unI2C_channelAddr, HAL_I2C_EVENT_MASTER_MODE_SELECT) ) {
        if ( (usI2Ctimeout--) == 0 ) return HAL_I2C_RETURN_FAIL;
    }

    HalI2C_SetTransmitAddr(unI2C_channelAddr, ucDeviceAddress, HAL_I2C_Direction_Transmitter);
    usI2Ctimeout = I2C_RW_TIMEOUT_CNT;
    while( !HalI2C_CheckEvent(unI2C_channelAddr, HAL_I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) ) 
    {
        if ( (usI2Ctimeout--) == 0 ) {
            HalI2C_GenerateStop(unI2C_channelAddr, HAL_ENABLE);

            usI2Ctimeout = I2C_RW_TIMEOUT_CNT;

            while( HalI2C_GetFlagStatus(unI2C_channelAddr, HAL_I2C_FLAG_STOPF) == HAL_RESET ) 
            {
                if((usI2Ctimeout--) == 0) return HAL_I2C_RETURN_FAIL;
            }

            return HAL_I2C_RETURN_FAIL;
        }
    }
#if defined(STM32F427X)
    I2C_SendData((I2C_TypeDef*)unI2C_channelAddr, ucI2CRegAddr);
#elif defined(AT32F435VMT7)
    i2c_data_send((i2c_type*)unI2C_channelAddr, ucI2CRegAddr);
#endif    
    usI2Ctimeout = I2C_RW_TIMEOUT_CNT;
    while( !HalI2C_CheckEvent(unI2C_channelAddr, HAL_I2C_EVENT_MASTER_BYTE_TRANSMITTING) ) 
    {
        if ( (usI2Ctimeout--) == 0 ) {
            HalI2C_GenerateStop(unI2C_channelAddr, HAL_ENABLE);
            usI2Ctimeout = I2C_RW_TIMEOUT_CNT;

            while( HalI2C_GetFlagStatus(unI2C_channelAddr, HAL_I2C_FLAG_STOPF) == HAL_RESET ) 
            {
                if((usI2Ctimeout--) == 0) return HAL_I2C_RETURN_FAIL;
            }

            return HAL_I2C_RETURN_FAIL;
        }
    }

    do{
#if defined(STM32F427X)
        I2C_SendData((I2C_TypeDef*)unI2C_channelAddr, *pBuffer);
#elif defined(AT32F435VMT7)
        i2c_data_send((i2c_type*)unI2C_channelAddr, *pBuffer);
#endif    

        usI2Ctimeout = I2C_RW_TIMEOUT_CNT;
        while( !HalI2C_CheckEvent(unI2C_channelAddr, HAL_I2C_EVENT_MASTER_BYTE_TRANSMITTING) ) 
        {
            if ( (usI2Ctimeout--) == 0 ) {
                HalI2C_GenerateStop(unI2C_channelAddr, HAL_ENABLE);
                return HAL_I2C_RETURN_FAIL;
            }
        }
        pBuffer++;
    }while(--nLength);

    HalI2C_GenerateStop(unI2C_channelAddr, HAL_ENABLE);
    usI2Ctimeout = I2C_RW_TIMEOUT_CNT;
/*
    while( HalI2C_GetFlagStatus(unI2C_channelAddr, HAL_I2C_FLAG_STOPF) == HAL_RESET ) 
    {
        if( (usI2Ctimeout--) == 0) return HAL_I2C_RETURN_FAIL;
    }
*/
#elif defined(AT32F435VMT7)
{
/*
    unsigned char uctmp[256];
    uctmp[0] = nOverlap;
    memcpy(&uctmp[1], pBuffer, nLength);
    nLength++;
    
    if ( i2c_master_transmit(&g_hI2Cx, ucDeviceAddress, (unsigned char*)uctmp, nLength, usI2Ctimeout) != I2C_OK )
        return HAL_I2C_RETURN_FAIL;
*/
    if ( i2c_memory_write(&g_hI2Cx, ucDeviceAddress, 
                            (uint16_t)ucI2CRegAddr, 
                            (unsigned char*)pBuffer, nLength, usI2Ctimeout) != I2C_OK )
        return HAL_I2C_RETURN_FAIL;
}
#endif

    return HAL_I2C_RETURN_SUCCESS;
}

int HalDrvI2CIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    eHalI2C_IOCtlMode nIoCtlMode = (eHalI2C_IOCtlMode)nLparam;

    switch (nIoCtlMode) 
    {
        case eI2C_IO_Init:
        {
            stHalI2C_InitTypeDef* pStHalI2C = (stHalI2C_InitTypeDef*)pBuffer;
#if defined(STM32F427X)
            I2C_InitTypeDef I2C_InitStructure;
            I2C_TypeDef* pI2CPortAddr = (I2C_TypeDef*)nRparam;

            memcpy((char*)&I2C_InitStructure, pStHalI2C, sizeof(I2C_InitTypeDef));
            I2C_Init(pI2CPortAddr, &I2C_InitStructure);
#elif defined(AT32F435VMT7)
            i2c_address_mode_type i2cAddressMode;
            unsigned int unClockSpeed = pStHalI2C->I2C_ClockSpeed;
            i2c_type* pI2CPortAddr = (i2c_type*)nRparam;

            if ( pStHalI2C->I2C_AcknowledgedAddress == HAL_I2C_AcknowledgedAddress_7bit )
                i2cAddressMode = I2C_ADDRESS_MODE_7BIT;
            else
                i2cAddressMode = I2C_ADDRESS_MODE_10BIT;
            
            i2c_init(pI2CPortAddr, 0, unClockSpeed);
            i2c_own_address1_set(pI2CPortAddr, i2cAddressMode, HAL_I2Cx_OWN_ADDRESS); 
#endif
        }
            break;
            
        case eI2C_IO_Enable:
        {
#if defined(STM32F427X)
            I2C_TypeDef* pI2CPortAddr = (I2C_TypeDef*)nRparam;
            I2C_Cmd((I2C_TypeDef*)pI2CPortAddr, (FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
            i2c_type* pI2CPortAddr = (i2c_type*)nRparam;
            i2c_enable((i2c_type*)pI2CPortAddr, (confirm_state)nOverlap);
#endif
        }
            break;
        case eI2C_IO_DeInit:
        {
#if defined(STM32F427X)
            I2C_TypeDef* pI2CPortAddr = (I2C_TypeDef*)nRparam;
            I2C_DeInit((I2C_TypeDef*)pI2CPortAddr);
#elif defined(AT32F435VMT7)
            i2c_type* pI2CPortAddr = (i2c_type*)nRparam;
            i2c_reset((i2c_type*)pI2CPortAddr);
#endif
        }
            break;
        case eI2C_IO_GetFlagStatus:
        {
            unsigned int unI2CFlags = nOverlap;
 #if defined(STM32F427X)
            return I2C_GetFlagStatus((I2C_TypeDef*)nRparam, unI2CFlags);
#elif defined(AT32F435VMT7)
            unsigned int unI2CCvtFlags = HalDrvI2C_ConvertFlag(unI2CFlags);

            return i2c_flag_get((i2c_type*)nRparam, unI2CCvtFlags);
#endif
        }
            break;
        case eI2C_IO_GenerateStart:
        {
#if defined(STM32F427X)
            I2C_TypeDef* pI2CPortAddr = (I2C_TypeDef*)nRparam;
            I2C_GenerateSTART(pI2CPortAddr, (FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
            i2c_type* pI2CPortAddr = (i2c_type*)nRparam;
            i2c_start_generate(pI2CPortAddr);
#endif
        }
            break;
        case eI2C_IO_GenerateStop:
        {
#if defined(STM32F427X)
            I2C_TypeDef* pI2CPortAddr = (I2C_TypeDef*)nRparam;
            I2C_GenerateSTOP((I2C_TypeDef*)pI2CPortAddr, (FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
            i2c_type* pI2CPortAddr = (i2c_type*)nRparam;
            i2c_stop_generate(pI2CPortAddr);
#endif
        }
            break;
        case eI2C_IO_CheckEvent:
        {
#if defined(STM32F427X)
            unsigned int unI2CFlags = nOverlap;
            I2C_TypeDef* pI2CPortAddr = (I2C_TypeDef*)nRparam;
            
            return I2C_CheckEvent(pI2CPortAddr, unI2CFlags);
#elif defined(AT32F435VMT7)
//            unsigned int unI2CFlags = nOverlap;
//            i2c_type* pI2CPortAddr = (i2c_type*)nRparam;
//            unsigned int unI2CCvtFlags = HalDrvI2C_ConvertFlag(unI2CFlags);
            return HAL_I2C_RETURN_SUCCESS;
#endif
        }
            break;
        case eI2C_IO_SetTransferAddr:
        {
            unsigned char unI2CAddr = (unsigned char)nLength;
            unsigned char unI2CDirection = nOverlap;
#if defined(STM32F427X)
            I2C_TypeDef* pI2CPortAddr = (I2C_TypeDef*)nRparam;
            I2C_Send7bitAddress(pI2CPortAddr, unI2CAddr, unI2CDirection);
#elif defined(AT32F435VMT7)
            i2c_type* pI2CPortAddr = (i2c_type*)nRparam;
            //i2c_transfer_addr_set(pI2CPortAddr, unI2CAddr);
            
            if      ( unI2CDirection == HAL_I2C_Direction_Transmitter ) unI2CDirection = I2C_GEN_START_WRITE;
            else if ( unI2CDirection == HAL_I2C_Direction_Receiver    ) unI2CDirection = I2C_GEN_START_READ;
            i2c_transmit_set(pI2CPortAddr, unI2CAddr, 1, I2C_RELOAD_MODE, I2C_GEN_START_WRITE);
#endif
        }
            break;
        case eI2C_IO_SetAcknowlegeConfig:
        {
#if defined(STM32F427X)
            I2C_TypeDef* pI2CPortAddr = (I2C_TypeDef*)nRparam;
            I2C_AcknowledgeConfig(pI2CPortAddr, (FunctionalState)nOverlap);            
#elif defined(AT32F435VMT7)
            i2c_type* pI2CPortAddr = (i2c_type*)nRparam;
            i2c_ack_enable(pI2CPortAddr, (confirm_state)nOverlap);
#endif       

        }
            break;
        default:
            break;
    }
    return HAL_I2C_RETURN_SUCCESS;
}

int HalDrvI2CClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_I2C_RETURN_SUCCESS;
}


