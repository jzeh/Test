/*
  ******************************************************************************
  * @file    HalCanDriver.c
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
#include "stm32f4xx_can.h"
#elif defined(AT32F435VMT7)
#include "at32f435_437.h"
#include "at32f435_437_can.h"
#endif
#include "HalHandler.h"
#include "HalCanDriver.h"

#include "HdDebug.h"

#include "GIT_InterProtocol.h"
#include "GIT_OemInterface.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define Trace(...)  GITDebug(DEBUG_MODULES_CAN,__VA_ARGS__)

#define HAL_INAK_TIMEOUT      		((uint32_t)0x0000FFFF)
#define HAL_CAN_TME_CHECK_TIME  	10

#if defined(AT32F435VMT7)
#define CAN1_SE_IRQn        CAN1_SCE_IRQn
#define CAN2_SE_IRQn        CAN2_SCE_IRQn
#define CAN1_SCE_IRQHandler CAN1_SE_IRQHandler
#define CAN2_SCE_IRQHandler CAN2_SE_IRQHandler

#endif

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#if defined(FEATURE_CAN1_USE)
stHalCANTX_STRUCT   g_CAN1_TxBuffCtrl;
stHalCanTxMsg       g_CAN1_TxMsgBuff[HAL_CAN1_TXBUF_LEN];

stHalCANRX_STRUCT   g_CAN1_RxBuffCtrl;
stHalCanRxMsg       g_CAN1_RxMsgBuff[HAL_CAN1_RXBUF_LEN];
#endif

#if defined(FEATURE_CAN2_USE)
stHalCANTX_STRUCT   g_CAN2_TxBuffCtrl;
stHalCanTxMsg       g_CAN2_TxMsgBuff[HAL_CAN2_TXBUF_LEN];

stHalCANRX_STRUCT   g_CAN2_RxBuffCtrl;
stHalCanRxMsg       g_CAN2_RxMsgBuff[HAL_CAN2_RXBUF_LEN];
#endif

unsigned char   g_ucCanOverFlowFlag = 0;
unsigned int    g_uiCanOverFlowTimer = 0;
unsigned char   g_ucCanOverFlowFlag2 = 0;
unsigned int    g_uiCanOverFlowTimer2 = 0;
volatile unsigned char g_ucCANTxBuffChkCount = 0;	//cantxbuffer? ??? ?? ????
extern boolean_t m_bBlockStorage;


#if defined(AT32F435VMT7)
BOOL g_bNeedtoCanReinit_CAN1 = FALSE;
BOOL g_bNeedtoCanReinit_CAN2 = FALSE;
#endif


/* Private function prototypes -----------------------------------------------*/
eHalFlagStatus HalDrvCanGetFlagStatus(int unCANRegAddr, unsigned int unFlag);
void HalDrvCanClearFlagStatus(int unCANRegAddr, unsigned int unFlag);
eHalFlagStatus HalDrvCanGetITFlagStatus(int unCANRegAddr, unsigned int unITFlag);
void HalDrvCanClearITStatus(int unCANRegAddr, unsigned int unITFlag);
int HalDrvCanTransmitStatus(int unCANRegAddr, unsigned char ucMailBox);
void HalCan_Tx_Irq(stHalCANTX_STRUCT *cantx);

/* Private functions ---------------------------------------------------------*/

void HalCan_Initial(void)
{
    stHalGPIO_InitTypeDef  GPIO_InitStructure;
    stHalNVIC_InitTypeDef  NVIC_InitStructure;
    stHalCAN_InitTypeDef   CAN_InitStructure;
    
#if defined(FEATURE_CAN1_USE)
    //------------------------------------------------------------------------------
    //  CAN1
    //------------------------------------------------------------------------------
    Trace("Can_Init\r\n");
    g_CAN1_TxBuffCtrl.pTxMsg    = g_CAN1_TxMsgBuff;
    g_CAN1_TxBuffCtrl.unCanReg  = (unsigned int)HAL_CAN1;
    g_CAN1_RxBuffCtrl.pRxMsg    = g_CAN1_RxMsgBuff;

    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOD_GROUP, NULL, 0, HAL_ENABLE);

    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, HAL_CAN1_RX_PIN, NULL, 0, HAL_GPIO_AF_CAN1);
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, HAL_CAN1_TX_PIN, NULL, 0, HAL_GPIO_AF_CAN1);

    GPIO_InitStructure.GPIO_DS = eGPIO_DRIVE_STRENGTH_STRONGER;
    GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = eGPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = eGPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = eGPIO_PuPd_UP;//GPIO_PuPd_NOPULL;//GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Pin = HAL_CAN1_RX_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(stHalGPIO_InitTypeDef), 0);

    GPIO_InitStructure.GPIO_Pin = HAL_CAN1_TX_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(stHalGPIO_InitTypeDef), 0);

    /* CAN configuration **************************/
    HalDrvRccIOCtrl(eRCC_IO_CAN_Clock, eRCC_Clock_CAN1, NULL, 0, HAL_ENABLE);

    HalDrvCanIOCtrl(eCAN_IO_DeInit, (int)HAL_CAN1, NULL, 0, 0);

    /* CAN cell init */
    CAN_InitStructure.CAN_TTCM = HAL_DISABLE;
    CAN_InitStructure.CAN_ABOM = HAL_ENABLE;
    CAN_InitStructure.CAN_AWUM = HAL_DISABLE;
    CAN_InitStructure.CAN_NART = HAL_DISABLE;
    CAN_InitStructure.CAN_RFLM = HAL_DISABLE;
    CAN_InitStructure.CAN_TXFP = HAL_DISABLE;
    CAN_InitStructure.CAN_Mode = HAL_CAN_Mode_Normal; //HAL_CAN_Mode_Normal;//CAN_Mode_LoopBack;//CAN_Mode_Normal;

#if defined(STM32F427X)
    /* CAN Baudrate = 500KBps (CAN clocked at 45 MHz) */
    CAN_InitStructure.CAN_BS1 = HAL_CAN_BS1_11tq;
    CAN_InitStructure.CAN_BS2 = HAL_CAN_BS2_3tq;
    CAN_InitStructure.CAN_Prescaler = 6;
    CAN_InitStructure.CAN_SJW = HAL_CAN_SJW_1tq;
#elif defined(AT32F435VMT7)
    /* CAN Baudrate = 500KBps - 75% (CAN clocked at 72 MHz) */
    CAN_InitStructure.CAN_BS1 = HAL_CAN_BS1_5tq;
    CAN_InitStructure.CAN_BS2 = HAL_CAN_BS2_2tq;
    CAN_InitStructure.CAN_Prescaler = 18;
    CAN_InitStructure.CAN_SJW = HAL_CAN_SJW_2tq;
#endif
    HalDrvCanIOCtrl(eCAN_IO_Init, (int)HAL_CAN1, (char*)&CAN_InitStructure, sizeof(stHalCAN_InitTypeDef), 0);


    /* Enable FIFO 0 message pending Interrupt */
    
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN1, NULL, HAL_CAN_IT_TME, HAL_ENABLE);
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN1, NULL, HAL_CAN_IT_FMP0, HAL_ENABLE);/*!< FIFO 0 message pending Interrupt*/
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN1, NULL, HAL_CAN_IT_EWG, HAL_ENABLE);/*!< Error warning Interrupt*/
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN1, NULL, HAL_CAN_IT_EPV, HAL_ENABLE);/*!< Error passive Interrupt*/
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN1, NULL, HAL_CAN_IT_BOF, HAL_ENABLE);/*!< Bus-off Interrupt*/
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN1, NULL, HAL_CAN_IT_LEC, HAL_ENABLE);/*!< Last error code Interrupt*/
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN1, NULL, HAL_CAN_IT_ERR, HAL_ENABLE);/* Error Globale Interrupt*/

    NVIC_InitStructure.NVIC_IRQChannel = HAL_CAN1_TX_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_CAN1_NVIC_IRQChannelPreemptionPriority;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_CAN1_TX_NVIC_IRQChannelSubPriority;
    NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_ENABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);

    NVIC_InitStructure.NVIC_IRQChannel = HAL_CAN1_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_CAN1_NVIC_IRQChannelPreemptionPriority;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_CAN1_RX0_NVIC_IRQChannelSubPriority;
    NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_ENABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);

    NVIC_InitStructure.NVIC_IRQChannel = HAL_CAN1_SCE_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_CAN1_NVIC_IRQChannelPreemptionPriority;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_CAN1_SCE_NVIC_IRQChannelSubPriority;
    NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_ENABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);
#endif

#if defined(FEATURE_CAN2_USE)
    //------------------------------------------------------------------------------
    //  CAN2
    //------------------------------------------------------------------------------
    //printf("CAN2: Init CAN2(Bit Rate: 100K, Low CAN)\n");
    g_CAN2_TxBuffCtrl.pTxMsg    = g_CAN2_TxMsgBuff;
    g_CAN2_TxBuffCtrl.unCanReg  = (unsigned int)HAL_CAN2;
    g_CAN2_RxBuffCtrl.pRxMsg    = g_CAN2_RxMsgBuff;

    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOB_GROUP, NULL, 0, HAL_ENABLE);

    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, HAL_CAN2_RX_PIN, NULL, 0, HAL_GPIO_AF_CAN2);
    HalDrvGpioIOCtrl(eGPIO_IO_AF_MAPPING, HAL_CAN2_TX_PIN, NULL, 0, HAL_GPIO_AF_CAN2);

    //tx,rx
    GPIO_InitStructure.GPIO_DS      = eGPIO_DRIVE_STRENGTH_STRONGER;
    GPIO_InitStructure.GPIO_Mode    = eGPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed   = eGPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType   = eGPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd    = eGPIO_PuPd_UP;
    
    GPIO_InitStructure.GPIO_Pin     =  HAL_CAN2_RX_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(stHalGPIO_InitTypeDef), 0);

    GPIO_InitStructure.GPIO_Pin = HAL_CAN2_TX_PIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(stHalGPIO_InitTypeDef), 0);

    /* CAN configuration **************************/
    HalDrvRccIOCtrl(eRCC_IO_CAN_Clock, eRCC_Clock_CAN2, NULL, 0, HAL_ENABLE);

    HalDrvCanIOCtrl(eCAN_IO_DeInit, (int)HAL_CAN2, NULL, 0, 0);

    /* CAN cell init */
    CAN_InitStructure.CAN_TTCM = HAL_DISABLE;
    CAN_InitStructure.CAN_ABOM = HAL_ENABLE;
    CAN_InitStructure.CAN_AWUM = HAL_DISABLE;
    CAN_InitStructure.CAN_NART = HAL_DISABLE;
    CAN_InitStructure.CAN_RFLM = HAL_DISABLE;
    CAN_InitStructure.CAN_TXFP = HAL_DISABLE;
    CAN_InitStructure.CAN_Mode = HAL_CAN_Mode_Normal;// HAL_CAN_Mode_LoopBack;//CAN_Mode_LoopBack;//CAN_Mode_Normal;

#if defined(STM32F427X)
    /* CAN Baudrate = 100KBps (CAN clocked at 45 MHz) */
    // 80%
//    CAN_InitStructure.CAN_BS1 = HAL_CAN_BS1_11tq;
//    CAN_InitStructure.CAN_BS2 = HAL_CAN_BS2_3tq;
//    CAN_InitStructure.CAN_Prescaler = 24;
    CAN_InitStructure.CAN_BS1 = HAL_CAN_BS1_14tq;
    CAN_InitStructure.CAN_BS2 = HAL_CAN_BS2_5tq;
    CAN_InitStructure.CAN_Prescaler = 21;
    CAN_InitStructure.CAN_SJW = HAL_CAN_SJW_1tq;
#elif defined(AT32F435VMT7)
    /* CAN Baudrate = 100KBps - 75% (CAN clocked at 72 MHz) */
    CAN_InitStructure.CAN_BS1 = HAL_CAN_BS1_5tq;
    CAN_InitStructure.CAN_BS2 = HAL_CAN_BS2_2tq;
    CAN_InitStructure.CAN_Prescaler = 90;
    CAN_InitStructure.CAN_SJW = HAL_CAN_SJW_2tq;
#endif
    HalDrvCanIOCtrl(eCAN_IO_Init, (int)HAL_CAN2, (char*)&CAN_InitStructure, sizeof(stHalCAN_InitTypeDef), 0);

    /* Enable FIFO 0 message pending Interrupt */
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN2, NULL, HAL_CAN_IT_TME, HAL_ENABLE);
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN2, NULL, HAL_CAN_IT_FMP0, HAL_ENABLE);/*!< FIFO 0 message pending Interrupt*/
	HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN2, NULL, HAL_CAN_IT_EWG, HAL_ENABLE);/*!< Error warning Interrupt*/
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN2, NULL, HAL_CAN_IT_EPV, HAL_ENABLE);/*!< Error passive Interrupt*/
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN2, NULL, HAL_CAN_IT_BOF, HAL_ENABLE);/*!< Bus-off Interrupt*/
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN2, NULL, HAL_CAN_IT_LEC, HAL_ENABLE);/*!< Last error code Interrupt*/
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN2, NULL, HAL_CAN_IT_ERR, HAL_ENABLE);/* Error Globale Interrupt*/

    NVIC_InitStructure.NVIC_IRQChannel = HAL_CAN2_TX_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_CAN2_NVIC_IRQChannelPreemptionPriority;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_CAN2_TX_NVIC_IRQChannelSubPriority;
    NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_ENABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);

    NVIC_InitStructure.NVIC_IRQChannel = HAL_CAN2_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_CAN2_NVIC_IRQChannelPreemptionPriority;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_CAN2_RX0_NVIC_IRQChannelSubPriority;
    NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_ENABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);

    NVIC_InitStructure.NVIC_IRQChannel = HAL_CAN2_SCE_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_CAN2_NVIC_IRQChannelPreemptionPriority;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_CAN2_SCE_NVIC_IRQChannelSubPriority;
    NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_ENABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);
#endif

	return;
}

void HalCan_makeTxHeader( stHalCANTX_STRUCT *cantx, unsigned int inCANId, unsigned char ucCANtype, unsigned char ucLen )
{
	if( ucCANtype == HAL_CAN_ID_EXT )							// 29byte identifier
	{
		cantx->pTxMsg[cantx->wrindex].ExtId	= inCANId;
		cantx->pTxMsg[cantx->wrindex].IDE   = HAL_CAN_ID_EXT;
	}
	else												// 11byte identifier
	{
		cantx->pTxMsg[cantx->wrindex].StdId	= inCANId;
		cantx->pTxMsg[cantx->wrindex].IDE	= HAL_CAN_ID_STD;
	}

	cantx->pTxMsg[cantx->wrindex].RTR = HAL_CAN_RTR_DATA;
	cantx->pTxMsg[cantx->wrindex].DLC = ucLen;
}

stHalCANTX_STRUCT* HalCan_GetTxStructAddr(unsigned int nCANChAddr)
{
    if ( nCANChAddr == (unsigned int)HAL_CAN1 ) return &g_CAN1_TxBuffCtrl;
    else                                        return &g_CAN2_TxBuffCtrl;    
}

//-----------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//      u8 HalCanSet_Std(stHalCANTX_STRUCT *cantx,u32 stdid)
//      return  1: ok
//              0: tx doing
//------------------------------------------------------------------------------
unsigned char HalCanSet_Std(stHalCANTX_STRUCT *cantx, u32 stdid)
{
    cantx->pTxMsg[cantx->wrindex].StdId = stdid;    
    cantx->pTxMsg[cantx->wrindex].RTR = HAL_CAN_RTR_DATA;
    cantx->pTxMsg[cantx->wrindex].IDE = HAL_CAN_ID_STD;

	return 1;
}

//------------------------------------------------------------------------------
//      u8 HalCanSet_Ext(stHalCANTX_STRUCT *cantx,u32 stdid,extid)
//      return  1: ok
//              0: tx doing
//------------------------------------------------------------------------------
unsigned char HalCanSet_Ext(stHalCANTX_STRUCT *cantx, u32 extid)
{
    cantx->pTxMsg[cantx->wrindex].ExtId = extid;
    cantx->pTxMsg[cantx->wrindex].RTR = HAL_CAN_RTR_DATA;
    cantx->pTxMsg[cantx->wrindex].IDE = HAL_CAN_ID_EXT;

	return 1;
}

//------------------------------------------------------------------------------
//  unsigned short HalCan_GetRxQueueCount(stHalCANRX_STRUCT *CanRxCtrl)
//      return  queue count
//------------------------------------------------------------------------------
unsigned short HalCan_GetRxQueueCount(stHalCANRX_STRUCT *CanRxCtrl)
{
    unsigned short usRet = 0;
    unsigned short usMaxBufSize;

    if ( CanRxCtrl == &g_CAN1_RxBuffCtrl )  usMaxBufSize = HAL_CAN1_RXBUF_LEN;
    else                                    usMaxBufSize = HAL_CAN2_RXBUF_LEN;

    if ( CanRxCtrl->wrindex >= CanRxCtrl->rdindex )
       usRet = CanRxCtrl->wrindex - CanRxCtrl->rdindex;
    else
       usRet = usMaxBufSize + CanRxCtrl->wrindex - CanRxCtrl->rdindex; 

    return usRet;
}

//------------------------------------------------------------------------------
//  unsigned short HalCan_GetTxQueueCount(stHalCANTX_STRUCT *CanTxCtrl)
//      return  queue count
//------------------------------------------------------------------------------
unsigned short HalCan_GetTxQueueCount(stHalCANTX_STRUCT *CanTxCtrl)
{
    unsigned short usRet;
    unsigned short usMaxBufSize;

    if ( CanTxCtrl == &g_CAN1_TxBuffCtrl )  usMaxBufSize = HAL_CAN1_TXBUF_LEN;
    else                                    usMaxBufSize = HAL_CAN2_TXBUF_LEN;

    if ( CanTxCtrl->wrindex >= CanTxCtrl->rdindex )
       usRet = CanTxCtrl->wrindex - CanTxCtrl->rdindex;
    else
       usRet = usMaxBufSize + CanTxCtrl->wrindex - CanTxCtrl->rdindex; 

    return usRet;
}

//------------------------------------------------------------------------------
//  signed char HalCan_Tx(stHalCANTX_STRUCT *cantx,u8 *buf,u16 size)
//      return  2: buf over
//              1: ok
//              0: tx doing
//------------------------------------------------------------------------------
signed char HalCan_Tx(stHalCANTX_STRUCT *cantx, u8 *buf, u16 size)
{
        unsigned char* p;
    unsigned short usMaxBufSize;

    if ( cantx == &g_CAN1_TxBuffCtrl )  usMaxBufSize = HAL_CAN1_TXBUF_LEN;
    else                                usMaxBufSize = HAL_CAN2_TXBUF_LEN;
        
    if ( HalCan_GetTxQueueCount(cantx)+1 >= usMaxBufSize )     
    {
        printf("\r\n\r\n!!! CAN Tx Buffer is overflow, Buffer size %d, wr %d, rd %d !!!\r\n\r\n", usMaxBufSize, cantx->wrindex, cantx->rdindex);
        //return 0;
    }
    if  ( size < HAL_CAN_FRAME_DATA_SIZE )  cantx->pTxMsg[cantx->wrindex].DLC = size;
    else                                    cantx->pTxMsg[cantx->wrindex].DLC = HAL_CAN_FRAME_DATA_SIZE; 
    
    p = cantx->pTxMsg[cantx->wrindex].Data;
    memcpy(p, buf, cantx->pTxMsg[cantx->wrindex].DLC); 
    
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)cantx->unCanReg, NULL, HAL_CAN_IT_TME, HAL_DISABLE);
    cantx->wrindex = (cantx->wrindex+1) % usMaxBufSize;
    
    // 2023-03-17 lee1008, can tranmit °¡ Àü¼Û»óÅÂ°¡ ¾Æ´Ò °æ¿ì¿¡ È£Ãâ µÇµµ·Ï º¯°æ(mailbox°¡ ¸ðµÎ ºñ¿©ÀÌ°í complete flag°¡ ¸ðµÎ clear µÈ »óÅÂ)
    // 2023-03-23 lee1008, Áõ»ó ¹ß»ý(ÁßÃ¸µÇ´Â)ÇÏ¿© TX ¿äÃ» Áß¿¡´Â interrupt disable ÇÏµµ·Ï º¯°æ. TX ¿äÃ» µé¾î°¡´Â Á¶°Ç ¿ÏÈ­(Mailbox empty¸¸ È®ÀÎ,TMxTCF È®ÀÎÇÏÁö ¾ÊÀ½)
    //if( HalCan_GetTxQueueCount(cantx) == 1 ) // ï¿½ï¿½ï¿½ï¿½ 1È¸CAN writeï¿½ï¿½ ï¿½Ø¾ï¿½ HAL_CAN_IT_TMEï¿½ï¿½ enable ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ Interruptï¿½ï¿½ ï¿½ß»ï¿½ï¿½Ñ´ï¿½.
    if ( HalDrvCanGetITFlagStatus((int)cantx->unCanReg, HAL_CAN_IT_NO_TRANMIT ) == HAL_SET )  // Àü¼Û »óÅÂ°¡ ¾Æ´Ï¸é 
	    HalCan_Tx_Irq(cantx);

    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)cantx->unCanReg, NULL, HAL_CAN_IT_TME, HAL_ENABLE);

    return 1;
}

//------------------------------------------------------------------------------
//      Can Tx ISR
//
//
//------------------------------------------------------------------------------

void HalCan_Tx_Irq(stHalCANTX_STRUCT *cantx)
{
	unsigned char ret;
    unsigned short usMaxBufSize;;
    if ( cantx == &g_CAN1_TxBuffCtrl )  usMaxBufSize = HAL_CAN1_TXBUF_LEN;
    else                                usMaxBufSize = HAL_CAN2_TXBUF_LEN;

	if (!HalCan_GetTxQueueCount(cantx)){
        if ( HalDrvCanGetITFlagStatus((int)cantx->unCanReg, HAL_CAN_IT_TME ) == HAL_SET ) 
                     HalDrvCanClearITStatus((int)cantx->unCanReg, HAL_CAN_IT_TME);
        HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)cantx->unCanReg, NULL, HAL_CAN_IT_TME, HAL_DISABLE);
    }
	else
    {
        ret = HalDrvCanWrite((int)cantx->unCanReg, 0, (char*)&cantx->pTxMsg[cantx->rdindex], sizeof(stHalCanTxMsg), 0);

        if(ret != HAL_CAN_TxStatus_NoMailBox)
        {
            cantx->rdindex = (cantx->rdindex+1) % usMaxBufSize;
        }
	}
}

#if defined(FEATURE_CAN1_USE)
void CAN1_TX_IRQHandler(void)
{
	HalCan_Tx_Irq(&g_CAN1_TxBuffCtrl);
}
#endif

#if defined(FEATURE_CAN2_USE)
void CAN2_TX_IRQHandler(void)
{
	HalCan_Tx_Irq(&g_CAN2_TxBuffCtrl);
}
#endif

//------------------------------------------------------------------------------
//  return 0: no msg
//         1: read msg ok
//------------------------------------------------------------------------------
unsigned char HalCan1_Read( stHalCanRxMsg* rxmsg )
{
    if ( HalCan_GetRxQueueCount(&g_CAN1_RxBuffCtrl) == 0 ) return 0;

    memcpy(rxmsg, &g_CAN1_RxBuffCtrl.pRxMsg[g_CAN1_RxBuffCtrl.rdindex], sizeof(stHalCanRxMsg));
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (unsigned int)HAL_CAN1, NULL, HAL_CAN_IT_FMP0, HAL_DISABLE);
    g_CAN1_RxBuffCtrl.rdindex = (g_CAN1_RxBuffCtrl.rdindex+1) % HAL_CAN1_RXBUF_LEN;

    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (unsigned int)HAL_CAN1, NULL, HAL_CAN_IT_FMP0, HAL_ENABLE);
    return 1;
}

void HalCan_DisableRxInterrupt(unsigned int CANxAddr)
{
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, CANxAddr, NULL, HAL_CAN_IT_FMP0, HAL_DISABLE);
}

void HalCan_EnableRxInterrupt(unsigned int CANxAddr)
{
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, CANxAddr, NULL, HAL_CAN_IT_FMP0, HAL_ENABLE);
}

unsigned char HalCan_ClearBuffer(unsigned int wParam)
{
	if( wParam == eCOMM_TYPE_CAN1)
	{
		memset(&g_CAN1_TxBuffCtrl,0x00,sizeof(g_CAN1_TxBuffCtrl));
        g_CAN1_TxBuffCtrl.pTxMsg = g_CAN1_TxMsgBuff;
        g_CAN1_TxBuffCtrl.unCanReg = (unsigned int)HAL_CAN1;
        memset(&g_CAN1_RxBuffCtrl,0x00,sizeof(g_CAN1_RxBuffCtrl));
		g_CAN1_RxBuffCtrl.pRxMsg = g_CAN1_RxMsgBuff;
		return 1;
	}
	else
	{
        memset(&g_CAN2_TxBuffCtrl,0x00,sizeof(g_CAN2_TxBuffCtrl));
        g_CAN2_TxBuffCtrl.pTxMsg = g_CAN2_TxMsgBuff;
        g_CAN2_TxBuffCtrl.unCanReg = (unsigned int)HAL_CAN2;
		memset(&g_CAN2_RxBuffCtrl,0x00,sizeof(g_CAN2_RxBuffCtrl));
		g_CAN2_RxBuffCtrl.pRxMsg = g_CAN2_RxMsgBuff;
		return 1;
	}
}

unsigned char HalCan2_Read( stHalCanRxMsg* rxmsg )
{
    if ( HalCan_GetRxQueueCount(&g_CAN2_RxBuffCtrl) == 0 ) return 0;

    memcpy(rxmsg, &g_CAN2_RxBuffCtrl.pRxMsg[g_CAN2_RxBuffCtrl.rdindex], sizeof(stHalCanRxMsg));
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN2, NULL, HAL_CAN_IT_FMP0, HAL_DISABLE);
    g_CAN2_RxBuffCtrl.rdindex = (g_CAN2_RxBuffCtrl.rdindex+1) % HAL_CAN2_RXBUF_LEN;
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN2, NULL, HAL_CAN_IT_FMP0, HAL_ENABLE);
    return 1;
}

//------------------------------------------------------------------------------
//      Can Rx ISR
//
//
//------------------------------------------------------------------------------

#if defined(FEATURE_CAN1_USE)
void CAN1_RX0_IRQHandler(void)
{
    if ( HalDrvCanGetITFlagStatus((int)HAL_CAN1, HAL_CAN_IT_FOV0 ) == HAL_SET ) 
    {
        HalDrvCanClearITStatus((int)HAL_CAN1, HAL_CAN_IT_FOV0);
    }
    if ( HalDrvCanGetITFlagStatus((int)HAL_CAN1, HAL_CAN_IT_FF0 ) == HAL_SET ) 
    {
        HalDrvCanClearITStatus((int)HAL_CAN1, HAL_CAN_IT_FF0);
    }
    if ( HalDrvCanGetITFlagStatus((int)HAL_CAN1, HAL_CAN_IT_FMP0 ) == HAL_SET )
    {
#if defined(AT32F435VMT7)
        if ( g_bNeedtoCanReinit_CAN1 == TRUE ) // Workaround
        {
            stHalCanRxMsg CANRXMsg;
            HalDrvCanRead((int)HAL_CAN1, HAL_CAN_FIFO0, (char*)&CANRXMsg, sizeof(stHalCanRxMsg), 0);
            g_bNeedtoCanReinit_CAN1 = FALSE;
        }
        else
#endif
        {
            HalDrvCanRead((int)HAL_CAN1, HAL_CAN_FIFO0, (char*)&g_CAN1_RxBuffCtrl.pRxMsg[g_CAN1_RxBuffCtrl.wrindex], sizeof(stHalCanRxMsg), 0);
            g_CAN1_RxBuffCtrl.wrindex = (g_CAN1_RxBuffCtrl.wrindex+1) % HAL_CAN1_RXBUF_LEN;
        }
    }

#if 0 // not used : need to interrupt active
    if ( HalDrvCanGetITFlagStatus((int)HAL_CAN1, HAL_CAN_IT_FMP1) == HAL_SET )
    {
        if ( g_bNeedtoCanReinit_CAN1 == TRUE ) // Workaround
        {
//            Trace("#2 g_bNeedtoCanReinit_CAN1, cnt %d, rd %d, wr %d\r\n", 
//                g_CAN1_RxBuffCtrl.cnt, g_CAN1_RxBuffCtrl.rdindex, g_CAN1_RxBuffCtrl.wrindex);
            stHalCanRxMsg CANRXMsg;
            HalDrvCanRead((int)HAL_CAN1, HAL_CAN_FIFO1, (char*)&CANRXMsg, sizeof(stHalCanRxMsg), 0);
            g_bNeedtoCanReinit_CAN1 = FALSE;
        }
        else
        {
            HalDrvCanRead((int)HAL_CAN1, HAL_CAN_FIFO1, (char*)&g_CAN1_RxBuffCtrl.pRxMsg[g_CAN1_RxBuffCtrl.wrindex], sizeof(stHalCanRxMsg), 0);
            g_CAN1_RxBuffCtrl.wrindex = (g_CAN1_RxBuffCtrl.wrindex+1) % HAL_CAN1_RXBUF_LEN;
        }
    }
#endif
}
#endif

#if defined(FEATURE_CAN2_USE)
void CAN2_RX0_IRQHandler(void)
{
    if ( HalDrvCanGetITFlagStatus((int)HAL_CAN2, HAL_CAN_IT_FOV0 ) == HAL_SET ) 
    {
        HalDrvCanClearITStatus((int)HAL_CAN2, HAL_CAN_IT_FOV0);
    }
    if ( HalDrvCanGetITFlagStatus((int)HAL_CAN2, HAL_CAN_IT_FF0 ) == HAL_SET ) 
    {
        HalDrvCanClearITStatus((int)HAL_CAN2, HAL_CAN_IT_FF0);
    }
    if ( HalDrvCanGetITFlagStatus ((int)HAL_CAN2, HAL_CAN_IT_FMP0 ) == HAL_SET )
    {
#if defined(AT32F435VMT7)
        if ( g_bNeedtoCanReinit_CAN2 == TRUE ) // Workaround
        {
//            Trace("#1 g_bNeedtoCanReinit_CAN2, cnt %d, rd %d, wr %d\r\n", 
//                HalCan_GetRxQueueCount(&g_CAN2_RxBuffCtrl), g_CAN2_RxBuffCtrl.rdindex, g_CAN2_RxBuffCtrl.wrindex);
            stHalCanRxMsg CANRXMsg;
            HalDrvCanRead((int)HAL_CAN2, HAL_CAN_FIFO0, (char*)&CANRXMsg, sizeof(stHalCanRxMsg), 0);
            g_bNeedtoCanReinit_CAN2 = FALSE;
        }
        else
#endif
        {
            HalDrvCanRead((int)HAL_CAN2, HAL_CAN_FIFO0, (char*)&g_CAN2_RxBuffCtrl.pRxMsg[g_CAN2_RxBuffCtrl.wrindex], sizeof(stHalCanRxMsg), 0);
            g_CAN2_RxBuffCtrl.wrindex = (g_CAN2_RxBuffCtrl.wrindex+1) % HAL_CAN2_RXBUF_LEN;
        }
    }
}
#endif

//------------------------------------------------------------------------------
//      Can Err ISR
//------------------------------------------------------------------------------
#if defined(FEATURE_CAN1_USE)
void CAN1_SCE_IRQHandler(void)
{
	if (HalDrvCanGetITFlagStatus((int)HAL_CAN1, HAL_CAN_IT_EWG)){
		HalDrvCanClearITStatus((int)HAL_CAN1, HAL_CAN_IT_EWG);
	}

	if (HalDrvCanGetITFlagStatus((int)HAL_CAN1, HAL_CAN_IT_EPV)){
		HalDrvCanClearITStatus((int)HAL_CAN1, HAL_CAN_IT_EPV);
	}

	if (HalDrvCanGetITFlagStatus((int)HAL_CAN1, HAL_CAN_IT_BOF)){
		HalDrvCanClearITStatus((int)HAL_CAN1, HAL_CAN_IT_BOF);
	}

	if (HalDrvCanGetITFlagStatus((int)HAL_CAN1, HAL_CAN_IT_LEC) == HAL_SET){
#if defined(STM32F427X)
        HalDrvCanClearITStatus((int)HAL_CAN1, HAL_CAN_IT_LEC);
#elif defined(AT32F435VMT7)
        // errorta Àû¿ë 
        __IO unsigned int unErrIndex = 0;
        unErrIndex = CAN1->ests & 0x70;
		HalDrvCanClearITStatus((int)HAL_CAN1, HAL_CAN_IT_LEC);
        if ( unErrIndex == 0x00000010 )
        {
//Trace("1 ~~~~~~~~~~~~!~!~!~!~!~!~!~!~!~!~!\r\n");
//           HalDrvRccIOCtrl(eRCC_IO_CAN_Clock, eRCC_Clock_CAN1, NULL, 0, HAL_DISABLE);
//           HalCan_Initial();
            //can_reset(CAN1);
            /* Call CAN initialization function */
            g_bNeedtoCanReinit_CAN1 = TRUE;// Workaround
        }
#endif
	}

	if (HalDrvCanGetITFlagStatus((int)HAL_CAN1, HAL_CAN_IT_ERR)){
		HalDrvCanClearITStatus((int)HAL_CAN1, HAL_CAN_IT_ERR);
	}
}
#endif

#if defined(FEATURE_CAN2_USE)
void CAN2_SCE_IRQHandler(void)
{
	if (HalDrvCanGetITFlagStatus((int)HAL_CAN2, HAL_CAN_IT_EWG)) {
//		printf("CAN2_IT_EWG\n");
		HalDrvCanClearITStatus((int)HAL_CAN2, HAL_CAN_IT_EWG);
	}

	if (HalDrvCanGetITFlagStatus((int)HAL_CAN2, HAL_CAN_IT_EPV)) {
//		printf("CAN2_IT_EPV\n");
		HalDrvCanClearITStatus((int)HAL_CAN2, HAL_CAN_IT_EPV);
	}

	if (HalDrvCanGetITFlagStatus((int)HAL_CAN2, HAL_CAN_IT_BOF)) {
//		printf("CAN2_IT_BOF\n");
		HalDrvCanClearITStatus((int)HAL_CAN2, HAL_CAN_IT_BOF);
	}

	if (HalDrvCanGetITFlagStatus((int)HAL_CAN2, HAL_CAN_IT_LEC)) {
#if defined(STM32F427X)
		HalDrvCanClearITStatus((int)HAL_CAN2, HAL_CAN_IT_LEC);
#elif defined(AT32F435VMT7)
        // errorta Àû¿ë 
        __IO unsigned int unErrIndex = 0;
        unErrIndex = CAN2->ests & 0x70;
        HalDrvCanClearITStatus((int)HAL_CAN2, HAL_CAN_IT_LEC);
        if ( unErrIndex == 0x00000010 )
        {
            //can_reset(CAN2);
//        Trace("2 ~~~~~~~~~~~~!~!~!~!~!~!~!~!~!~!~!\r\n");
//            HalDrvRccIOCtrl(eRCC_IO_CAN_Clock, eRCC_Clock_CAN2, NULL, 0, HAL_DISABLE);
//            HalCan_Initial();
            /* Call CAN initialization function */
            g_bNeedtoCanReinit_CAN2 = TRUE;// Workaround
        }
#endif
	}

	if (HalDrvCanGetITFlagStatus((int)HAL_CAN2, HAL_CAN_IT_ERR)) {
//		printf("CAN2_IT_ERR\n");
		HalDrvCanClearITStatus((int)HAL_CAN2, HAL_CAN_IT_ERR);
	}
}
#endif

//------------------------------------------------------------------------------
//  in param1 : CAN1 / CAN2
//  in param2 : CAN_1MBPS/CAN_500KBPS/CAN_250KBPS/CAN_100KBPS
//  return : 0 -fail, 1- ok
//------------------------------------------------------------------------------

unsigned char HalCanSetBaudrate(int nCANChAddr, eCanBaudrate eBaudrate)
{
#if defined(AT32F435VMT7)
	u32 bs1, bs2, prec;
    stHalCAN_InitTypeDef   CAN_InitStructure;

    HalDrvCanIOCtrl(eCAN_IO_IntEnable, nCANChAddr, NULL, HAL_CAN_IT_TME, HAL_DISABLE);
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, nCANChAddr, NULL, HAL_CAN_IT_FMP0, HAL_DISABLE);
    HalDrvCanIOCtrl(eCAN_IO_DeInit, nCANChAddr, NULL, 0, 0);

	switch (eBaudrate) {
    	case eCAN_1MBPS:
#if defined(STM32F427X)
            //½Ì°¡Æú º£ÀÌ½º ¼Ò½º¿¡¼­ 85.7%·Î ¼³Á¤µÊ. CanFD´Â 87.5%
            // 87.5% - 40MHz 
    		bs1 = HAL_CAN_BS1_6tq;
			bs2 = HAL_CAN_BS2_1tq;
			prec = 5;
            CAN_InitStructure.CAN_SJW = HAL_CAN_SJW_1tq;                

#elif defined(AT32F435VMT7)
/*
            // 80% - 72M
            bs1 = HAL_CAN_BS1_8tq;
            bs2 = HAL_CAN_BS2_3tq;
            prec = 6;
            CAN_InitStructure.CAN_SJW = HAL_CAN_SJW_3tq;
*/

            // 87.5%
            bs1 = HAL_CAN_BS1_6tq;
            bs2 = HAL_CAN_BS2_1tq;
            prec = 9;
            CAN_InitStructure.CAN_SJW = HAL_CAN_SJW_1tq;

#endif
			break;
        case eCAN_500KBPS:
#if defined(STM32F427X)
            // 75% - 40MHz
            bs1 = HAL_CAN_BS1_5tq;
            bs2 = HAL_CAN_BS2_2tq;
            prec = 10;
            CAN_InitStructure.CAN_SJW = HAL_CAN_SJW_2tq;                
#elif defined(AT32F435VMT7)
            // 75% - 500000
            bs1 = HAL_CAN_BS1_5tq;
            bs2 = HAL_CAN_BS2_2tq;
            prec = 18;
            CAN_InitStructure.CAN_SJW = HAL_CAN_SJW_2tq;                
#endif
            break;

		case eCAN_250KBPS:
#if defined(STM32F427X)
			//75%
			bs1 = CAN_BS1_5tq;
			bs2 = CAN_BS2_2tq;
			prec = 20;
            CAN_InitStructure.CAN_SJW = HAL_CAN_SJW_2tq;                
#elif defined(AT32F435VMT7)
            // 75% - 250000
            bs1 = HAL_CAN_BS1_5tq;
            bs2 = HAL_CAN_BS2_2tq;
            prec = 36;
            CAN_InitStructure.CAN_SJW = HAL_CAN_SJW_2tq;                
#endif
            break;
		case eCAN_125KBPS:
#if defined(STM32F427X)
			//75%
			bs1 = CAN_BS1_5tq;
			bs2 = CAN_BS2_2tq;
			prec = 40;
            CAN_InitStructure.CAN_SJW = HAL_CAN_SJW_2tq;                
#elif defined(AT32F435VMT7)
            // 75% - 125000
            bs1 = HAL_CAN_BS1_5tq;
            bs2 = HAL_CAN_BS2_2tq;
            prec = 72;
            CAN_InitStructure.CAN_SJW = HAL_CAN_SJW_2tq;                
#endif
			break;
		case eCAN_100KBPS:
#if defined(STM32F427X)
            // 75%
			bs1 = HAL_CAN_BS1_5tq;
			bs2 = HAL_CAN_BS2_2tq;
			prec= 50;
            CAN_InitStructure.CAN_SJW = HAL_CAN_SJW_2tq;                
#elif defined(AT32F435VMT7)
            //75%
            bs1 = HAL_CAN_BS1_5tq;
            bs2 = HAL_CAN_BS2_2tq;
            prec = 90;
            CAN_InitStructure.CAN_SJW = HAL_CAN_SJW_2tq;
#endif
            break;
		default:
			return 0;
	}

    CAN_InitStructure.CAN_TTCM = HAL_DISABLE;
    CAN_InitStructure.CAN_ABOM = HAL_ENABLE;
    CAN_InitStructure.CAN_AWUM = HAL_DISABLE;
    CAN_InitStructure.CAN_NART = HAL_DISABLE;
    CAN_InitStructure.CAN_RFLM = HAL_DISABLE;
    CAN_InitStructure.CAN_TXFP = HAL_DISABLE;
    CAN_InitStructure.CAN_Mode = HAL_CAN_Mode_Normal;    //HAL_CAN_Mode_Normal;// CAN_Mode_Silent; //CAN_Mode_LoopBack;
    CAN_InitStructure.CAN_BS1 = bs1;
    CAN_InitStructure.CAN_BS2 = bs2;    
    CAN_InitStructure.CAN_Prescaler = prec;
    HalDrvCanIOCtrl(eCAN_IO_Init, nCANChAddr, (char*)&CAN_InitStructure, sizeof(stHalCAN_InitTypeDef), 0);


    HalDrvCanIOCtrl(eCAN_IO_IntEnable, nCANChAddr, NULL, HAL_CAN_IT_TME, HAL_ENABLE);
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, nCANChAddr, NULL, HAL_CAN_IT_FMP0, HAL_ENABLE);

	return 1;


#elif defined(STM32F427X) // #if defined(AT32F435VMT7)
	uint32_t wait_ack = 0x00000000;
	u32 bs1, bs2, prec;
	CAN_TypeDef* CANx = (CAN_TypeDef*)nCANChAddr;

	switch (eBaudrate) {
    	case eCAN_1MBPS:
            //½Ì°¡Æú º£ÀÌ½º ¼Ò½º¿¡¼­ 85.7%·Î ¼³Á¤µÊ. CanFD´Â 87.5%
            // 87.5% - 40MHz 
    		bs1 = HAL_CAN_BS1_6tq;
			bs2 = HAL_CAN_BS2_1tq;
			prec = 5;
			break;    	
		case eCAN_250KBPS:
			//75%
			bs1 = CAN_BS1_5tq;
			bs2 = CAN_BS2_2tq;
			prec = 20;
            break;
		case eCAN_125KBPS:
			//75%
			bs1 = CAN_BS1_5tq;
			bs2 = CAN_BS2_2tq;
			prec = 40;
			break;
		case eCAN_500KBPS:
    	Trace("500K\n");
            // 75% - 40MHz
            bs1 = HAL_CAN_BS1_5tq;
            bs2 = HAL_CAN_BS2_2tq;
            prec = 10;

			break;
		case eCAN_100KBPS:
        	Trace("100K\n");
            bs1 = HAL_CAN_BS1_5tq;
            bs2 = HAL_CAN_BS2_2tq;
            prec= 50;
			break;
		default:
			return 0;
	}

	/* Request initialisation */
	CANx->MCR |= CAN_MCR_INRQ ;

	/* Wait the acknowledge */
	while (((CANx->MSR & CAN_MSR_INAK) != CAN_MSR_INAK) && (wait_ack != HAL_INAK_TIMEOUT)){
		wait_ack++;
	}

	if ((CANx->MSR & CAN_MSR_INAK) != CAN_MSR_INAK){
		return 0;
	}

	CANx->BTR=CANx->BTR&(0xff80fc00);
	CANx->BTR |= ((bs1 << 16)|(bs2 << 20)|(prec-1));

	/* Request leave initialisation */
	CANx->MCR &= ~(uint32_t)CAN_MCR_INRQ;

	/* Wait the acknowledge */
	wait_ack = 0;

	while (((CANx->MSR & CAN_MSR_INAK) == CAN_MSR_INAK) && (wait_ack != HAL_INAK_TIMEOUT)){
		wait_ack++;
	}

	if ((CANx->MSR & CAN_MSR_INAK) == CAN_MSR_INAK){
		return 0;
	}

	return 1;

#endif//// #if defined(AT32F435VMT7)
}

eHalReturnStatus HalCan_SetCanFilter(unsigned char nChannel, unsigned char nCANIDType, 
                            unsigned char nMaskNum, unsigned int *pStartMaskValue, unsigned int *pEndMaskValue, unsigned int unFIFOAssignNo)
{
	unsigned char i, startFilterNum, endFilterNum;
	unsigned int startMaskVal, endMaskVal;
	stHalCAN_FilterInitTypeDef  CAN_Ch_FilterInitStructure;
    unsigned int unCanChAddress;
	if(nMaskNum > HAL_CAN_MAX_MASK_CNT)
	{
		return HAL_RETURN_FAIL;
	}

	if(nChannel == CAN_CHANNEL_1)		//CH1Àº 0ºÎÅÍ CH2´Â 14ºÎÅÍ ½ÃÀÛÇÏ¸ç ÇÊÅÍ°¹¼ö¸¸Å­ ¸¶½ºÅ·À» ´õ ÇÒ ¼ö ÀÖÁö¸¸, ¿ì¸® DB»óÀÇ ÇÑ°è·Î 1°³¹Û¿¡ ¾²Áö ¸øÇÑ´Ù
	{
		startFilterNum = 0;
		endFilterNum = nMaskNum;
        unCanChAddress = (unsigned int)HAL_CAN1;
	}
	else
	{
		startFilterNum = HAL_CAN_MAX_MASK_CNT;				// channel 2ÀÌ¸é 14ºÎÅÍ ½ÃÀÛ
		endFilterNum = HAL_CAN_MAX_MASK_CNT + nMaskNum;
        unCanChAddress = (unsigned int)HAL_CAN2;
	}
	// Bit 2 IDE: Identifier extension
	// This bit defines the identifier type of message in the mailbox.
	// 0: Standard identifier.
	// 1: Extended identifier.
	// Bit 1 RTR: Remote transmission request
	// 0: Data frame
	// 1: Remote frame
	// IDList ¸ðµå ¿¡´Â standard ÀÏ ´ë´Â ÀÎµ¦½º°¡ 4°³ ÀÌ´Ù.
	// Mask ¸ðµå¿¡´Â standard ÀÏ ´ë´Â ÀÎµ¦½º°¡ 2°³ÀÌ´Ù.
	for(i = startFilterNum; i < endFilterNum; i++) 
    {
		startMaskVal = pStartMaskValue[i-startFilterNum];
		endMaskVal = pEndMaskValue[i-startFilterNum];

		if(nCANIDType == STANDARD_EXTENDED_CAN)
		{
			if((startMaskVal>>16) != 0) nCANIDType = EXTENDED_CAN;
			else						nCANIDType = STANDARD_CAN;
		}
		
		if(nCANIDType == STANDARD_CAN)
		{
			CAN_Ch_FilterInitStructure.CAN_FilterNumber = i;
			CAN_Ch_FilterInitStructure.CAN_FilterScale = HAL_CAN_FilterScale_16bit;
			
			if(startMaskVal == endMaskVal) {									//IdListMode¿¡¼­´Â ÀÔ·ÂµÈ CAN ID¸¸ ¹ÞÀ» ¼ö ÀÖ´Ù.
				CAN_Ch_FilterInitStructure.CAN_FilterMode = HAL_CAN_FilterMode_IdList;	//CAN_FilterMode_IdList:0x01
				CAN_Ch_FilterInitStructure.CAN_FilterIdLow = ((startMaskVal&0x0000FFFF)<<5) & 0xFFE0;
				// Force Diable setting
				CAN_Ch_FilterInitStructure.CAN_FilterMaskIdLow = 0xFFF0;
				CAN_Ch_FilterInitStructure.CAN_FilterIdHigh = 0xFFF0;
				CAN_Ch_FilterInitStructure.CAN_FilterMaskIdHigh = 0xFFF0;
			}
			else {
				//FilterMaskIdLowÀÇ °¢ bit °ªÀÌ 1ÀÌ¸é FilterIdLowIdlowÀÇ bit°ª°ú µ¿ÀÏÇØ¾ßÇÏ°í 0ÀÌ¸é don't care
				//FilterMaskIdLowÀÇ °¢ bit °ªÀÌ 1ÀÌ¸é FilterIdLowIdlowÀÇ bit°ª°ú µ¿ÀÏÇØ¾ßÇÏ°í 0ÀÌ¸é don't care
				CAN_Ch_FilterInitStructure.CAN_FilterMode = HAL_CAN_FilterMode_IdMask;
				CAN_Ch_FilterInitStructure.CAN_FilterIdLow = (((startMaskVal&0x0000FFFF)<<5)&((endMaskVal&0x0000FFFF)<<5)) & 0xFFE0;
				CAN_Ch_FilterInitStructure.CAN_FilterMaskIdLow = (~(((startMaskVal&0x0000FFFF)<<5)^((endMaskVal&0x0000FFFF)<<5)))& 0xFFE0;
				// Force Diable setting
				CAN_Ch_FilterInitStructure.CAN_FilterIdHigh = 0xFFF0;
				CAN_Ch_FilterInitStructure.CAN_FilterMaskIdHigh = 0xFFF0;
			}
		}
		else if(nCANIDType == EXTENDED_CAN)
		{
			CAN_Ch_FilterInitStructure.CAN_FilterNumber = i;
			CAN_Ch_FilterInitStructure.CAN_FilterScale = HAL_CAN_FilterScale_32bit;

			if(startMaskVal == endMaskVal)
			{
				CAN_Ch_FilterInitStructure.CAN_FilterMode = HAL_CAN_FilterMode_IdList;
				CAN_Ch_FilterInitStructure.CAN_FilterIdHigh = ((startMaskVal&0x1FFFE000)>>13);
				CAN_Ch_FilterInitStructure.CAN_FilterIdLow = ((startMaskVal&0x00001FFF)<<3) | 0x04;
				CAN_Ch_FilterInitStructure.CAN_FilterMaskIdHigh = 0xFFFF;    
				CAN_Ch_FilterInitStructure.CAN_FilterMaskIdLow = 0xFFFC;    
			}
			else
			{
				CAN_Ch_FilterInitStructure.CAN_FilterMode = HAL_CAN_FilterMode_IdMask;
				CAN_Ch_FilterInitStructure.CAN_FilterIdHigh = ((endMaskVal&0x1FFFE000)>>13);
				CAN_Ch_FilterInitStructure.CAN_FilterIdLow = ((endMaskVal&0x00001FFF)<<3) | 0x04;
				CAN_Ch_FilterInitStructure.CAN_FilterMaskIdHigh = ((startMaskVal&0x1FFFE000)>>13);
				CAN_Ch_FilterInitStructure.CAN_FilterMaskIdLow = ((startMaskVal&0x00001FFF)<<3) | 0x04;
			}
		}
		else 
		{
			return HAL_RETURN_FAIL;
		}
		
		/* CAN filter init */
		CAN_Ch_FilterInitStructure.CAN_FilterFIFOAssignment = unFIFOAssignNo;
		CAN_Ch_FilterInitStructure.CAN_FilterActivation = HAL_ENABLE;
        
        HalDrvCanIOCtrl(eCAN_IO_FilterInit, unCanChAddress, 
                        (char*)&CAN_Ch_FilterInitStructure, 
                        sizeof(CAN_Ch_FilterInitStructure), 0);
	}

	return HAL_RETURN_SUCCESS;
}


eHalFlagStatus HalDrvCanGetFlagStatus(int unCANRegAddr, unsigned int unFlag)
{
    return (eHalFlagStatus)HalDrvCanIOCtrl(eCAN_IO_GetFlagStatus, unCANRegAddr, NULL, 0, unFlag);
}

void HalDrvCanClearFlagStatus(int unCANRegAddr, unsigned int unFlag)
{
    HalDrvCanIOCtrl(eCAN_IO_ClearFlagStatus, unCANRegAddr, NULL, 0, unFlag);
}

eHalFlagStatus HalDrvCanGetITFlagStatus(int unCANRegAddr, unsigned int unITFlag)   
{
    return (eHalFlagStatus)HalDrvCanIOCtrl(eCAN_IO_GetITFlagStatus, unCANRegAddr, NULL, 0, unITFlag);
}

void HalDrvCanClearITStatus(int unCANRegAddr, unsigned int unITFlag)
{
    HalDrvCanIOCtrl(eCAN_IO_ClearITFlagStatus, unCANRegAddr, NULL, 0, unITFlag);
}

int HalDrvCanTransmitStatus(int unCANRegAddr, unsigned char ucMailBox)
{
    return HalDrvCanIOCtrl(eCAN_IO_GetTransmitStatus, unCANRegAddr, NULL, 0, ucMailBox);
}

#if defined(AT32F435VMT7)
unsigned int HalDrvCan_ConvertHalFlagStatus(unsigned int nHalCanFlag)
{
#if defined(STM32F427X)
#if 0
#define CAN_FLAG_RQCP0             ((uint32_t)0x38000001) /*!< Request MailBox0 Flag */
#define CAN_FLAG_RQCP1             ((uint32_t)0x38000100) /*!< Request MailBox1 Flag */
#define CAN_FLAG_RQCP2             ((uint32_t)0x38010000) /*!< Request MailBox2 Flag */
#define CAN_FLAG_FMP0              ((uint32_t)0x12000003) /*!< FIFO 0 Message Pending Flag */
#define CAN_FLAG_FF0               ((uint32_t)0x32000008) /*!< FIFO 0 Full Flag            */
#define CAN_FLAG_FOV0              ((uint32_t)0x32000010) /*!< FIFO 0 Overrun Flag         */
#define CAN_FLAG_FMP1              ((uint32_t)0x14000003) /*!< FIFO 1 Message Pending Flag */
#define CAN_FLAG_FF1               ((uint32_t)0x34000008) /*!< FIFO 1 Full Flag            */
#define CAN_FLAG_FOV1              ((uint32_t)0x34000010) /*!< FIFO 1 Overrun Flag         */
#define CAN_FLAG_WKU               ((uint32_t)0x31000008) /*!< Wake up Flag */
#define CAN_FLAG_SLAK              ((uint32_t)0x31000012) /*!< Sleep acknowledge Flag */
#define CAN_FLAG_EWG               ((uint32_t)0x10F00001) /*!< Error Warning Flag   */
#define CAN_FLAG_EPV               ((uint32_t)0x10F00002) /*!< Error Passive Flag   */
#define CAN_FLAG_BOF               ((uint32_t)0x10F00004) /*!< Bus-Off Flag         */
#define CAN_FLAG_LEC               ((uint32_t)0x30F00070) /*!< Last error code Flag */
#endif
#elif defined(AT32F435VMT7)
#if 0      
#define CAN_EAF_FLAG                     ((uint32_t)0x01) /*!< error active flag */
#define CAN_EPF_FLAG                     ((uint32_t)0x02) /*!< error passive flag */
#define CAN_BOF_FLAG                     ((uint32_t)0x03) /*!< bus-off flag */
#define CAN_ETR_FLAG                     ((uint32_t)0x04) /*!< error type record flag */
#define CAN_EOIF_FLAG                    ((uint32_t)0x05) /*!< error occur interrupt flag */
#define CAN_TM0TCF_FLAG                  ((uint32_t)0x06) /*!< transmit mailbox 0 transmission completed flag */
#define CAN_TM1TCF_FLAG                  ((uint32_t)0x07) /*!< transmit mailbox 1 transmission completed flag */
#define CAN_TM2TCF_FLAG                  ((uint32_t)0x08) /*!< transmit mailbox 2 transmission completed flag */
#define CAN_RF0MN_FLAG                   ((uint32_t)0x09) /*!< receive fifo 0 message num flag */
#define CAN_RF0FF_FLAG                   ((uint32_t)0x0A) /*!< receive fifo 0 full flag */
#define CAN_RF0OF_FLAG                   ((uint32_t)0x0B) /*!< receive fifo 0 overflow flag */
#define CAN_RF1MN_FLAG                   ((uint32_t)0x0C) /*!< receive fifo 1 message num flag */
#define CAN_RF1FF_FLAG                   ((uint32_t)0x0D) /*!< receive fifo 1 full flag */
#define CAN_RF1OF_FLAG                   ((uint32_t)0x0E) /*!< receive fifo 1 overflow flag */
#define CAN_QDZIF_FLAG                   ((uint32_t)0x0F) /*!< quit doze mode interrupt flag */
#define CAN_EDZC_FLAG                    ((uint32_t)0x10) /*!< enter doze mode confirm flag */
#define CAN_TMEF_FLAG                    ((uint32_t)0x11) /*!< transmit mailbox empty flag */
#endif
#endif
    unsigned int unRetFlag;  

    if      ( nHalCanFlag == HAL_CAN_FLAG_RQCP0 ) unRetFlag = CAN_TM0TCF_FLAG; 
    else if ( nHalCanFlag == HAL_CAN_FLAG_RQCP1 ) unRetFlag = CAN_TM1TCF_FLAG;  
    else if ( nHalCanFlag == HAL_CAN_FLAG_RQCP2 ) unRetFlag = CAN_TM2TCF_FLAG;  
    else if ( nHalCanFlag == HAL_CAN_FLAG_FMP0  ) unRetFlag = CAN_RF0MN_FLAG;  
    else if ( nHalCanFlag == HAL_CAN_FLAG_FF0   ) unRetFlag = CAN_RF0FF_FLAG;  
    else if ( nHalCanFlag == HAL_CAN_FLAG_FOV0  ) unRetFlag = CAN_RF0OF_FLAG;  
    else if ( nHalCanFlag == HAL_CAN_FLAG_FMP1  ) unRetFlag = CAN_RF1MN_FLAG;  
    else if ( nHalCanFlag == HAL_CAN_FLAG_FF1   ) unRetFlag = CAN_RF1FF_FLAG;  
    else if ( nHalCanFlag == HAL_CAN_FLAG_FOV1  ) unRetFlag = CAN_RF1OF_FLAG;  
    else if ( nHalCanFlag == HAL_CAN_FLAG_EWG   ) unRetFlag = CAN_EAF_FLAG;  
    else if ( nHalCanFlag == HAL_CAN_FLAG_EPV   ) unRetFlag = CAN_EPF_FLAG;  
    else if ( nHalCanFlag == HAL_CAN_FLAG_BOF   ) unRetFlag = CAN_BOF_FLAG;  
    else if ( nHalCanFlag == HAL_CAN_FLAG_LEC   ) unRetFlag = CAN_ETR_FLAG;  
    //    else if ( nHalCanFlag == HAL_CAN_FLAG_WKU   ) unRetFlag = CAN_TM0TCF_FLAG;  
    //    else if ( nHalCanFlag == HAL_CAN_FLAG_SLAK  ) unRetFlag = CAN_TM0TCF_FLAG;  
    
    return unRetFlag;
}

unsigned int HalDrvCan_ConvertHalITFlagStatus(unsigned int nHalITCanFlag)
{
    unsigned int nCovtITFlag = 0;
    
    if      ( nHalITCanFlag == HAL_CAN_IT_TME     ) nCovtITFlag = CAN_TMEF_FLAG;
    else if ( nHalITCanFlag == HAL_CAN_IT_FMP0    ) nCovtITFlag = CAN_RF0MN_FLAG;
    else if ( nHalITCanFlag == HAL_CAN_IT_FF0     ) nCovtITFlag = CAN_RF0FF_FLAG;
    else if ( nHalITCanFlag == HAL_CAN_IT_FOV0    ) nCovtITFlag = CAN_RF0OF_FLAG;
    else if ( nHalITCanFlag == HAL_CAN_IT_FMP1    ) nCovtITFlag = CAN_RF1MN_FLAG;
    else if ( nHalITCanFlag == HAL_CAN_IT_FF1     ) nCovtITFlag = CAN_RF1FF_FLAG;
    else if ( nHalITCanFlag == HAL_CAN_IT_FOV1    ) nCovtITFlag = CAN_RF1OF_FLAG;
//    else if ( nHalITCanFlag == HAL_CAN_IT_WKU     ) nCovtITFlag = CAN_TCIEN_INT;
//    else if ( nHalITCanFlag == HAL_CAN_IT_SLK     ) nCovtITFlag = CAN_TCIEN_INT;
    else if ( nHalITCanFlag == HAL_CAN_IT_EWG     ) nCovtITFlag = CAN_EAF_FLAG;
    else if ( nHalITCanFlag == HAL_CAN_IT_EPV     ) nCovtITFlag = CAN_EPF_FLAG;
    else if ( nHalITCanFlag == HAL_CAN_IT_BOF     ) nCovtITFlag = CAN_BOF_FLAG;
    else if ( nHalITCanFlag == HAL_CAN_IT_LEC     ) nCovtITFlag = CAN_ETR_FLAG;
    else if ( nHalITCanFlag == HAL_CAN_IT_ERR     ) nCovtITFlag = CAN_EOIF_FLAG;
    else if ( nHalITCanFlag == HAL_CAN_IT_NO_TRANMIT) nCovtITFlag = CAN_NO_TRANSMIT_FLAG;

    return nCovtITFlag;
}
#endif

//-----------------------------------------------------------------------------------------//
int HalDrvCanOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    HalCan_Initial();

    return HAL_RETURN_SUCCESS;
}

int HalDrvCanRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
	stHalCanRxMsg *pstHalCanRxMsg = (stHalCanRxMsg*)pBuffer;
    unsigned char ucFiFoNo = nRparam;
#if defined(STM32F427X)
    CanRxMsg rxCanMsg;
    CAN_Receive((CAN_TypeDef*)nLparam, ucFiFoNo, &rxCanMsg);

	pstHalCanRxMsg->StdId = rxCanMsg.StdId;
	pstHalCanRxMsg->ExtId = rxCanMsg.ExtId;
	pstHalCanRxMsg->IDE = rxCanMsg.IDE;
	pstHalCanRxMsg->RTR = rxCanMsg.RTR;
	pstHalCanRxMsg->DLC = rxCanMsg.DLC;
	memcpy(pstHalCanRxMsg->Data, rxCanMsg.Data, rxCanMsg.DLC);
	pstHalCanRxMsg->FMI = rxCanMsg.FMI;
#elif defined(AT32F435VMT7)
    can_rx_message_type rxCanMsg;
    can_message_receive((can_type*)nLparam, (can_rx_fifo_num_type)ucFiFoNo, &rxCanMsg);
	pstHalCanRxMsg->StdId = rxCanMsg.standard_id;
	pstHalCanRxMsg->ExtId = rxCanMsg.extended_id;
	pstHalCanRxMsg->IDE = (rxCanMsg.id_type==HAL_CAN_ID_STD) ? HAL_CAN_ID_STD:HAL_CAN_ID_EXT;
	pstHalCanRxMsg->RTR = rxCanMsg.frame_type;
	pstHalCanRxMsg->DLC = rxCanMsg.dlc;
	memcpy(pstHalCanRxMsg->Data, rxCanMsg.data, rxCanMsg.dlc);
	pstHalCanRxMsg->FMI = rxCanMsg.filter_index;
#endif
    return HAL_RETURN_SUCCESS;
}

int HalDrvCanWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    unsigned int unCANchAddr = (unsigned int)nLparam;

    stHalCanTxMsg* pstCANTxMsg = (stHalCanTxMsg*)pBuffer;
#if defined(STM32F427X)
    return CAN_Transmit((CAN_TypeDef*)unCANchAddr, (CanTxMsg*)pstCANTxMsg);
#elif defined(AT32F435VMT7)
    can_tx_message_type tx_message_struct;
    tx_message_struct.standard_id = pstCANTxMsg->StdId;
    tx_message_struct.extended_id = pstCANTxMsg->ExtId;
    if ( pstCANTxMsg->IDE == HAL_CAN_ID_STD )
        tx_message_struct.id_type = CAN_ID_STANDARD;
    if ( pstCANTxMsg->IDE == HAL_CAN_ID_EXT )
        tx_message_struct.id_type = CAN_ID_EXTENDED;

    if ( pstCANTxMsg->RTR == HAL_CAN_RTR_DATA )
        tx_message_struct.frame_type = CAN_TFT_DATA;
    if ( pstCANTxMsg->RTR == HAL_CAN_RTR_REMOTE )
        tx_message_struct.frame_type = CAN_TFT_REMOTE;

    tx_message_struct.dlc = pstCANTxMsg->DLC;
    memcpy(tx_message_struct.data, pstCANTxMsg->Data, tx_message_struct.dlc);

    return can_message_transmit((can_type*)unCANchAddr, (can_tx_message_type*)&tx_message_struct);
#endif
}

int HalDrvCanIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    eHalCAN_IOCtlMode nIoCtlMode = (eHalCAN_IOCtlMode)nLparam;

    switch (nIoCtlMode) 
    {
        case eCAN_IO_Init:
        {
            stHalCAN_InitTypeDef* pHalCanInit = (stHalCAN_InitTypeDef*)pBuffer;
        
#if defined(STM32F427X)
            CAN_InitTypeDef stCANInit;
            stCANInit.CAN_Prescaler = pHalCanInit->CAN_Prescaler;
            stCANInit.CAN_Mode = pHalCanInit->CAN_Mode;
            stCANInit.CAN_SJW = pHalCanInit->CAN_SJW;
            stCANInit.CAN_BS1 = pHalCanInit->CAN_BS1;
            stCANInit.CAN_BS2 = pHalCanInit->CAN_BS2;
            stCANInit.CAN_TTCM = (FunctionalState)pHalCanInit->CAN_TTCM;
            stCANInit.CAN_ABOM = (FunctionalState)pHalCanInit->CAN_ABOM;
            stCANInit.CAN_AWUM = (FunctionalState)pHalCanInit->CAN_AWUM;
            stCANInit.CAN_NART = (FunctionalState)pHalCanInit->CAN_NART;
            stCANInit.CAN_RFLM = (FunctionalState)pHalCanInit->CAN_RFLM;
            stCANInit.CAN_TXFP = (FunctionalState)pHalCanInit->CAN_TXFP;
            
            CAN_Init((CAN_TypeDef*)nRparam, (CAN_InitTypeDef*)&stCANInit);
#elif defined(AT32F435VMT7)
            can_base_type can_base_struct;
            can_baudrate_type can_baudrate_struct;

            can_base_struct.mode_selection  = (can_mode_type)pHalCanInit->CAN_Mode;
            can_base_struct.ttc_enable      = (confirm_state)pHalCanInit->CAN_TTCM;
            can_base_struct.aebo_enable     = (confirm_state)pHalCanInit->CAN_ABOM;
            can_base_struct.aed_enable      = (confirm_state)pHalCanInit->CAN_AWUM;
            can_base_struct.prsf_enable     = (confirm_state)pHalCanInit->CAN_RFLM;
            can_base_struct.mdrsel_selection= CAN_DISCARDING_FIRST_RECEIVED;
            can_base_struct.mmssr_selection = CAN_SENDING_BY_ID;

            can_base_init((can_type*)nRparam, &can_base_struct);
            
            can_baudrate_struct.baudrate_div = pHalCanInit->CAN_Prescaler;
            can_baudrate_struct.rsaw_size = (can_rsaw_type)pHalCanInit->CAN_SJW;
            can_baudrate_struct.bts1_size = (can_bts1_type)pHalCanInit->CAN_BS1; 
            can_baudrate_struct.bts2_size = (can_bts2_type)pHalCanInit->CAN_BS2;
            can_baudrate_set((can_type*)nRparam, &can_baudrate_struct);
#endif
        }
            break;
            
        case eCAN_IO_DeInit:
#if defined(STM32F427X)
            CAN_DeInit((CAN_TypeDef*)nRparam);
#elif defined(AT32F435VMT7)
            can_reset((can_type*)nRparam);
#endif
            break;
            
        case eCAN_IO_FilterInit:
        {
            stHalCAN_FilterInitTypeDef *pstHalFilterInitType = (stHalCAN_FilterInitTypeDef*)pBuffer;
#if defined(STM32F427X)
            CAN_FilterInitTypeDef stFilterInfo;

            stFilterInfo.CAN_FilterIdHigh       = pstHalFilterInitType->CAN_FilterIdHigh;
            stFilterInfo.CAN_FilterIdLow        = pstHalFilterInitType->CAN_FilterIdLow;
            stFilterInfo.CAN_FilterMaskIdHigh   = pstHalFilterInitType->CAN_FilterMaskIdHigh;
            stFilterInfo.CAN_FilterMaskIdLow    = pstHalFilterInitType->CAN_FilterMaskIdLow;
            stFilterInfo.CAN_FilterFIFOAssignment = pstHalFilterInitType->CAN_FilterFIFOAssignment;
            stFilterInfo.CAN_FilterNumber       = pstHalFilterInitType->CAN_FilterNumber;
            stFilterInfo.CAN_FilterMode         = pstHalFilterInitType->CAN_FilterMode;
            stFilterInfo.CAN_FilterScale        = pstHalFilterInitType->CAN_FilterScale;
            stFilterInfo.CAN_FilterActivation   = (FunctionalState)pstHalFilterInitType->CAN_FilterActivation;
             
            CAN_FilterInit(&stFilterInfo);
#elif defined(AT32F435VMT7)
            can_filter_init_type can_filter_init_struct;
            
            can_filter_init_struct.filter_activate_enable = (confirm_state)TRUE;
            can_filter_init_struct.filter_mode      = (can_filter_mode_type)pstHalFilterInitType->CAN_FilterMode;
            can_filter_init_struct.filter_fifo      = (can_filter_fifo_type)pstHalFilterInitType->CAN_FilterFIFOAssignment;
            can_filter_init_struct.filter_number    = pstHalFilterInitType->CAN_FilterNumber;
            can_filter_init_struct.filter_bit       = (can_filter_bit_width_type)pstHalFilterInitType->CAN_FilterScale;
            can_filter_init_struct.filter_id_high   = pstHalFilterInitType->CAN_FilterIdHigh;
            can_filter_init_struct.filter_id_low    = pstHalFilterInitType->CAN_FilterIdLow;
            can_filter_init_struct.filter_mask_high = pstHalFilterInitType->CAN_FilterMaskIdHigh;
            can_filter_init_struct.filter_mask_low  = pstHalFilterInitType->CAN_FilterMaskIdLow;
            
            can_filter_init((can_type*)nRparam, &can_filter_init_struct);
#endif
        }
            break;

        case eCAN_IO_IntEnable:
        {
            unsigned int unITBit = (unsigned int)nLength;
#if defined(STM32F427X)
            CAN_ITConfig((CAN_TypeDef*)nRparam, unITBit, (FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
            can_interrupt_enable((can_type*)nRparam, unITBit, (confirm_state)nOverlap);
#endif
        }
            break;

        case eCAN_IO_GetFlagStatus:
        {
#if defined(STM32F427X)
            return CAN_GetFlagStatus((CAN_TypeDef*)nRparam, (unsigned int)nOverlap);
#elif defined(AT32F435VMT7)
            unsigned int unATCanFlag = HalDrvCan_ConvertHalFlagStatus((unsigned int)nOverlap);
            return can_flag_get((can_type*)nRparam, unATCanFlag);
#endif
        }
            break;
        case eCAN_IO_ClearFlagStatus:
        {
#if defined(STM32F427X)
            CAN_ClearFlag((CAN_TypeDef*)nRparam, (unsigned int)nOverlap);
#elif defined(AT32F435VMT7)
            unsigned int unATCanFlag = HalDrvCan_ConvertHalFlagStatus((unsigned int)nOverlap);
            can_flag_clear((can_type*)nRparam, unATCanFlag);
#endif
        }
            break;
            
        case eCAN_IO_GetITFlagStatus:
        {
#if defined(STM32F427X)
            return CAN_GetITStatus((CAN_TypeDef*)nRparam, (unsigned int)nOverlap);
#elif defined(AT32F435VMT7)
            unsigned int unITBit = HalDrvCan_ConvertHalITFlagStatus((unsigned int)nOverlap);

            return can_flag_get((can_type*)nRparam, unITBit);
#endif
        }
            break;

        case eCAN_IO_ClearITFlagStatus:
        {
#if defined(STM32F427X)
            CAN_ClearITPendingBit((CAN_TypeDef*)nRparam, (unsigned int)nOverlap);
#elif defined(AT32F435VMT7)
            unsigned int unITBit = HalDrvCan_ConvertHalITFlagStatus((unsigned int)nOverlap);

            can_flag_clear((can_type*)nRparam, unITBit);
#endif
        }
            break;

        case eCAN_IO_GetTransmitStatus:
#if defined(STM32F427X)
            return CAN_TransmitStatus((CAN_TypeDef*)nRparam, (unsigned char)nOverlap);
#elif defined(AT32F435VMT7)
            return can_transmit_status_get((can_type*)nRparam, (can_tx_mailbox_num_type)nOverlap);
#endif
            break;
        default:
            break;
    }

    return 0;
}

int HalDrvCanClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
	//ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ CAN2 Rxï¿½ï¿½ Lowï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½Å©ï¿½ï¿½ ï¿½ï¿½È£ï¿½ï¿½ ï¿½ï¿½ï¿?Highï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ß»ï¿½ï¿½ï¿½ï¿½ï¿½
	Oem_CAN1_STANDBY_ACTIVE();		//HIGHCAN1 ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?1,2ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ 1042Ä¨
	Oem_CAN2_HIGH_CAN3_DISABLE();	//HIGHCAN3 ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?2ï¿½ï¿½Â° 1042Ä¨
	Oem_CAN2_LOW_CAN_DISABLE(); 	//LOWCAN ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?1055Ä¨

    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN1, NULL, HAL_CAN_IT_TME, HAL_DISABLE);
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN2, NULL, HAL_CAN_IT_TME, HAL_DISABLE);
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN1, NULL, HAL_CAN_IT_FMP0, HAL_DISABLE);
    HalDrvCanIOCtrl(eCAN_IO_IntEnable, (int)HAL_CAN2, NULL, HAL_CAN_IT_FMP0, HAL_DISABLE);

	HalDrvCanIOCtrl(eCAN_IO_DeInit, (int)HAL_CAN1, NULL, 0, 0);
	HalDrvCanIOCtrl(eCAN_IO_DeInit, (int)HAL_CAN2, NULL, 0, 0);

    return 0;
}
