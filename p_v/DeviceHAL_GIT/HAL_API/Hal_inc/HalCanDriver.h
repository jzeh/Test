#ifndef __HAL_CAN_DRIVER_H__
#define __HAL_CAN_DRIVER_H__


#include "common.h"


/* Exported define -----------------------------------------------------------*/
#define HAL_CAN_FRAME_DATA_SIZE     0X08
#define HAL_CAN_MAX_MASK_CNT        14

#define FEATURE_CAN1_USE
#define FEATURE_CAN2_USE


#if defined(FEATURE_CAN1_USE)
#define HAL_CAN1            CAN1
#define HAL_CAN1_RX_PIN     GPIO_CAN1_RX
#define HAL_CAN1_TX_PIN     GPIO_CAN1_TX
#define HAL_CAN1_TXBUF_LEN  20
#define HAL_CAN1_RXBUF_LEN  500
#endif

#if defined(FEATURE_CAN2_USE)
#define HAL_CAN2            CAN2
#define HAL_CAN2_RX_PIN     GPIO_LOW_HIGH_CAN_RX
#define HAL_CAN2_TX_PIN     GPIO_LOW_HIGH_CAN_TX
#define HAL_CAN2_TXBUF_LEN  20
#define HAL_CAN2_RXBUF_LEN  500
#endif

//-----------------------------------------------------------
// @defgroup CAN_identifier_type 
#define HAL_CAN_ID_STD             ((uint32_t)0x00000000)  /*!< Standard Id */
#define HAL_CAN_ID_EXT             ((uint32_t)0x00000004)  /*!< Extended Id */

// @defgroup CAN_remote_transmission_request 
#define HAL_CAN_RTR_DATA            ((uint32_t)0x00000000)  /*!< Data frame */      
#define HAL_CAN_RTR_REMOTE          ((uint32_t)0x00000002)  /*!< Remote frame */

/* Mailboxes definition */
#define HAL_CAN_TXMAILBOX_0         ((uint8_t)0x00)
#define HAL_CAN_TXMAILBOX_1         ((uint8_t)0x01)
#define HAL_CAN_TXMAILBOX_2         ((uint8_t)0x02) 

#define HAL_CAN_FIFO0                 ((uint8_t)0x00) /*!< CAN FIFO 0 used to receive */
#define HAL_CAN_FIFO1                 ((uint8_t)0x01) /*!< CAN FIFO 1 used to receive */

// @defgroup CAN_transmit_constants 
#define HAL_CAN_TxStatus_Failed         ((uint8_t)0x00)/*!< CAN transmission failed */
#define HAL_CAN_TxStatus_Ok             ((uint8_t)0x01) /*!< CAN transmission succeeded */
#define HAL_CAN_TxStatus_Pending        ((uint8_t)0x02) /*!< CAN transmission pending */
#define HAL_CAN_TxStatus_NoMailBox      ((uint8_t)0x04) /*!< CAN cell did not provide */


#define HAL_MASK32_ID   0
#define HAL_MASK32_MK   1
#define HAL_MASK16_ID1  0
#define HAL_MASK16_MK1  1
#define HAL_MASK16_ID2  2
#define HAL_MASK16_MK2  3
// @defgroup CAN_InitStatus 
#define HAL_CAN_InitStatus_Failed              ((uint8_t)0x00) /*!< CAN initialization failed */
#define HAL_CAN_InitStatus_Success             ((uint8_t)0x01) /*!< CAN initialization OK */
// @defgroup CAN_operating_mode 
#define HAL_CAN_Mode_Normal             ((uint8_t)0x00)  /*!< normal mode */
#define HAL_CAN_Mode_LoopBack           ((uint8_t)0x01)  /*!< loopback mode */
#define HAL_CAN_Mode_Silent             ((uint8_t)0x02)  /*!< silent mode */
#define HAL_CAN_Mode_Silent_LoopBack    ((uint8_t)0x03)  /*!< loopback combined with silent mode */
// @defgroup CAN_operating_mode 
#define HAL_CAN_OperatingMode_Initialization  ((uint8_t)0x00) /*!< Initialization mode */
#define HAL_CAN_OperatingMode_Normal          ((uint8_t)0x01) /*!< Normal mode */
#define HAL_CAN_OperatingMode_Sleep           ((uint8_t)0x02) /*!< sleep mode */
#define HAL_CAN_SJW_1tq                 ((uint8_t)0x00)  /*!< 1 time quantum */
#define HAL_CAN_SJW_2tq                 ((uint8_t)0x01)  /*!< 2 time quantum */
#define HAL_CAN_SJW_3tq                 ((uint8_t)0x02)  /*!< 3 time quantum */
#define HAL_CAN_SJW_4tq                 ((uint8_t)0x03)  /*!< 4 time quantum */
// @defgroup CAN_time_quantum_in_bit_segment_1 
#define HAL_CAN_BS1_1tq                 ((uint8_t)0x00)  /*!< 1 time quantum */
#define HAL_CAN_BS1_2tq                 ((uint8_t)0x01)  /*!< 2 time quantum */
#define HAL_CAN_BS1_3tq                 ((uint8_t)0x02)  /*!< 3 time quantum */
#define HAL_CAN_BS1_4tq                 ((uint8_t)0x03)  /*!< 4 time quantum */
#define HAL_CAN_BS1_5tq                 ((uint8_t)0x04)  /*!< 5 time quantum */
#define HAL_CAN_BS1_6tq                 ((uint8_t)0x05)  /*!< 6 time quantum */
#define HAL_CAN_BS1_7tq                 ((uint8_t)0x06)  /*!< 7 time quantum */
#define HAL_CAN_BS1_8tq                 ((uint8_t)0x07)  /*!< 8 time quantum */
#define HAL_CAN_BS1_9tq                 ((uint8_t)0x08)  /*!< 9 time quantum */
#define HAL_CAN_BS1_10tq                ((uint8_t)0x09)  /*!< 10 time quantum */
#define HAL_CAN_BS1_11tq                ((uint8_t)0x0A)  /*!< 11 time quantum */
#define HAL_CAN_BS1_12tq                ((uint8_t)0x0B)  /*!< 12 time quantum */
#define HAL_CAN_BS1_13tq                ((uint8_t)0x0C)  /*!< 13 time quantum */
#define HAL_CAN_BS1_14tq                ((uint8_t)0x0D)  /*!< 14 time quantum */
#define HAL_CAN_BS1_15tq                ((uint8_t)0x0E)  /*!< 15 time quantum */
#define HAL_CAN_BS1_16tq                ((uint8_t)0x0F)  /*!< 16 time quantum */
// @defgroup CAN_time_quantum_in_bit_segment_2 
#define HAL_CAN_BS2_1tq                 ((uint8_t)0x00)  /*!< 1 time quantum */
#define HAL_CAN_BS2_2tq                 ((uint8_t)0x01)  /*!< 2 time quantum */
#define HAL_CAN_BS2_3tq                 ((uint8_t)0x02)  /*!< 3 time quantum */
#define HAL_CAN_BS2_4tq                 ((uint8_t)0x03)  /*!< 4 time quantum */
#define HAL_CAN_BS2_5tq                 ((uint8_t)0x04)  /*!< 5 time quantum */
#define HAL_CAN_BS2_6tq                 ((uint8_t)0x05)  /*!< 6 time quantum */
#define HAL_CAN_BS2_7tq                 ((uint8_t)0x06)  /*!< 7 time quantum */
#define HAL_CAN_BS2_8tq                 ((uint8_t)0x07)  /*!< 8 time quantum */
// @defgroup CAN_filter_mode 
#define HAL_CAN_FilterMode_IdMask       ((uint8_t)0x00)  /*!< identifier/mask mode */
#define HAL_CAN_FilterMode_IdList       ((uint8_t)0x01)  /*!< identifier list mode */
// @defgroup CAN_filter_scale 
#define HAL_CAN_FilterScale_16bit       ((uint8_t)0x00) /*!< Two 16-bit filters */
#define HAL_CAN_FilterScale_32bit       ((uint8_t)0x01) /*!< One 32-bit filter */
// @defgroup CAN_filter_FIFO
#define HAL_CAN_Filter_FIFO0             ((uint8_t)0x00)  /*!< Filter FIFO 0 assignment for filter x */
#define HAL_CAN_Filter_FIFO1             ((uint8_t)0x01)  /*!< Filter FIFO 1 assignment for filter x */
/* Transmit Flags */
#define HAL_CAN_FLAG_RQCP0             ((uint32_t)0x38000001) /*!< Request MailBox0 Flag */
#define HAL_CAN_FLAG_RQCP1             ((uint32_t)0x38000100) /*!< Request MailBox1 Flag */
#define HAL_CAN_FLAG_RQCP2             ((uint32_t)0x38010000) /*!< Request MailBox2 Flag */
/* Receive Flags */
#define HAL_CAN_FLAG_FMP0              ((uint32_t)0x12000003) /*!< FIFO 0 Message Pending Flag */
#define HAL_CAN_FLAG_FF0               ((uint32_t)0x32000008) /*!< FIFO 0 Full Flag            */
#define HAL_CAN_FLAG_FOV0              ((uint32_t)0x32000010) /*!< FIFO 0 Overrun Flag         */
#define HAL_CAN_FLAG_FMP1              ((uint32_t)0x14000003) /*!< FIFO 1 Message Pending Flag */
#define HAL_CAN_FLAG_FF1               ((uint32_t)0x34000008) /*!< FIFO 1 Full Flag            */
#define HAL_CAN_FLAG_FOV1              ((uint32_t)0x34000010) /*!< FIFO 1 Overrun Flag         */
/* Operating Mode Flags */
#define HAL_CAN_FLAG_WKU               ((uint32_t)0x31000008) /*!< Wake up Flag */
#define HAL_CAN_FLAG_SLAK              ((uint32_t)0x31000012) /*!< Sleep acknowledge Flag */
/* Error Flags */
#define HAL_CAN_FLAG_EWG               ((uint32_t)0x10F00001) /*!< Error Warning Flag   */
#define HAL_CAN_FLAG_EPV               ((uint32_t)0x10F00002) /*!< Error Passive Flag   */
#define HAL_CAN_FLAG_BOF               ((uint32_t)0x10F00004) /*!< Bus-Off Flag         */
#define HAL_CAN_FLAG_LEC               ((uint32_t)0x30F00070) /*!< Last error code Flag */
// @defgroup CAN_interrupts 
#define HAL_CAN_IT_TME                  ((uint32_t)0x00000001) /*!< Transmit mailbox empty Interrupt*/
/* Receive Interrupts */
#define HAL_CAN_IT_FMP0                 ((uint32_t)0x00000002) /*!< FIFO 0 message pending Interrupt*/
#define HAL_CAN_IT_FF0                  ((uint32_t)0x00000004) /*!< FIFO 0 full Interrupt*/
#define HAL_CAN_IT_FOV0                 ((uint32_t)0x00000008) /*!< FIFO 0 overrun Interrupt*/
#define HAL_CAN_IT_FMP1                 ((uint32_t)0x00000010) /*!< FIFO 1 message pending Interrupt*/
#define HAL_CAN_IT_FF1                  ((uint32_t)0x00000020) /*!< FIFO 1 full Interrupt*/
#define HAL_CAN_IT_FOV1                 ((uint32_t)0x00000040) /*!< FIFO 1 overrun Interrupt*/
/* Operating Mode Interrupts */
#define HAL_CAN_IT_WKU                  ((uint32_t)0x00010000) /*!< Wake-up Interrupt*/
#define HAL_CAN_IT_SLK                  ((uint32_t)0x00020000) /*!< Sleep acknowledge Interrupt*/
/* Error Interrupts */
#define HAL_CAN_IT_EWG                  ((uint32_t)0x00000100) /*!< Error warning Interrupt*/
#define HAL_CAN_IT_EPV                  ((uint32_t)0x00000200) /*!< Error passive Interrupt*/
#define HAL_CAN_IT_BOF                  ((uint32_t)0x00000400) /*!< Bus-off Interrupt*/
#define HAL_CAN_IT_LEC                  ((uint32_t)0x00000800) /*!< Last error code Interrupt*/
#define HAL_CAN_IT_ERR                  ((uint32_t)0x00008000) /*!< Error Interrupt*/
/* GIT custom */
#define HAL_CAN_IT_NO_TRANMIT           ((uint32_t)0x00100000) /*!< No tranmit interrupt status*/
/*******************  Bit definition for CAN_TSR register  ********************/
#define HAL_CAN_TSR_TME                         ((uint32_t)0x1C000000)        /*!<TME[2:0] bits */
#define HAL_CAN_TSR_TME0                        ((uint32_t)0x04000000)        /*!<Transmit Mailbox 0 Empty */
#define HAL_CAN_TSR_TME1                        ((uint32_t)0x08000000)        /*!<Transmit Mailbox 1 Empty */
#define HAL_CAN_TSR_TME2                        ((uint32_t)0x10000000)        /*!<Transmit Mailbox 2 Empty */
/*******************  Bit definition for CAN_RF0R register  *******************/
#define HAL_CAN_RF0R_FMP0               ((uint8_t)0x03)               /*!<FIFO 0 Message Pending */
#define HAL_CAN_RF0R_FULL0              ((uint8_t)0x08)               /*!<FIFO 0 Full */
#define HAL_CAN_RF0R_FOVR0              ((uint8_t)0x10)               /*!<FIFO 0 Overrun */
#define HAL_CAN_RF0R_RFOM0              ((uint8_t)0x20)               /*!<Release FIFO 0 Output Mailbox */



/* Exported types - Structure, Enumeration -----------------------------------*/

typedef enum __eHalCAN_IOCtlMode{
    eCAN_IO_Init,
    eCAN_IO_DeInit,
    eCAN_IO_FilterInit,
    eCAN_IO_IntEnable,
    eCAN_IO_GetFlagStatus,
    eCAN_IO_ClearFlagStatus,
    eCAN_IO_GetITFlagStatus,
    eCAN_IO_ClearITFlagStatus,
    eCAN_IO_GetTransmitStatus,
}eHalCAN_IOCtlMode;


// @brief  CAN init structure definition
typedef __packed struct __stHalCAN_InitTypeDef
{
  uint16_t CAN_Prescaler;   /*!< Specifies the length of a time quantum. 
                                 It ranges from 1 to 1024. */
  
  uint8_t CAN_Mode;         /*!< Specifies the CAN operating mode.
                                 This parameter can be a value of @ref CAN_operating_mode */

  uint8_t CAN_SJW;          /*!< Specifies the maximum number of time quanta 
                                 the CAN hardware is allowed to lengthen or 
                                 shorten a bit to perform resynchronization.
                                 This parameter can be a value of @ref CAN_synchronisation_jump_width */

  uint8_t CAN_BS1;          /*!< Specifies the number of time quanta in Bit 
                                 Segment 1. This parameter can be a value of 
                                 @ref CAN_time_quantum_in_bit_segment_1 */

  uint8_t CAN_BS2;          /*!< Specifies the number of time quanta in Bit Segment 2.
                                 This parameter can be a value of @ref CAN_time_quantum_in_bit_segment_2 */
  
  eHalFunctionalState CAN_TTCM; /*!< Enable or disable the time triggered communication mode.
                                This parameter can be set either to ENABLE or DISABLE. */
  
  eHalFunctionalState CAN_ABOM;  /*!< Enable or disable the automatic bus-off management.
                                  This parameter can be set either to ENABLE or DISABLE. */

  eHalFunctionalState CAN_AWUM;  /*!< Enable or disable the automatic wake-up mode. 
                                  This parameter can be set either to ENABLE or DISABLE. */

  eHalFunctionalState CAN_NART;  /*!< Enable or disable the non-automatic retransmission mode.
                                  This parameter can be set either to ENABLE or DISABLE. */

  eHalFunctionalState CAN_RFLM;  /*!< Enable or disable the Receive FIFO Locked mode.
                                  This parameter can be set either to ENABLE or DISABLE. */

  eHalFunctionalState CAN_TXFP;  /*!< Enable or disable the transmit FIFO priority.
                                  This parameter can be set either to ENABLE or DISABLE. */
}stHalCAN_InitTypeDef;

typedef __packed struct __stHalCanRxMsg
{
  uint32_t StdId;  /*!< Specifies the standard identifier.
                        This parameter can be a value between 0 to 0x7FF. */

  uint32_t ExtId;  /*!< Specifies the extended identifier.
                        This parameter can be a value between 0 to 0x1FFFFFFF. */

  uint8_t IDE;     /*!< Specifies the type of identifier for the message that 
                        will be received. This parameter can be a value of 
                        @ref CAN_identifier_type */

  uint8_t RTR;     /*!< Specifies the type of frame for the received message.
                        This parameter can be a value of 
                        @ref CAN_remote_transmission_request */

  uint8_t DLC;     /*!< Specifies the length of the frame that will be received.
                        This parameter can be a value between 0 to 8 */

  uint8_t Data[HAL_CAN_FRAME_DATA_SIZE]; /*!< Contains the data to be received. It ranges from 0 to 
                        0xFF. */

  uint8_t FMI;     /*!< Specifies the index of the filter the message stored in 
                        the mailbox passes through. This parameter can be a 
                        value between 0 to 0xFF */
}stHalCanRxMsg;

typedef __packed struct __stHalCanTxMsg
{
  uint32_t StdId;  /*!< Specifies the standard identifier.
                        This parameter can be a value between 0 to 0x7FF. */

  uint32_t ExtId;  /*!< Specifies the extended identifier.
                        This parameter can be a value between 0 to 0x1FFFFFFF. */

  uint8_t IDE;     /*!< Specifies the type of identifier for the message that 
                        will be transmitted. This parameter can be a value 
                        of @ref CAN_identifier_type */

  uint8_t RTR;     /*!< Specifies the type of frame for the message that will 
                        be transmitted. This parameter can be a value of 
                        @ref CAN_remote_transmission_request */

  uint8_t DLC;     /*!< Specifies the length of the frame that will be 
                        transmitted. This parameter can be a value between 
                        0 to 8 */

  uint8_t Data[HAL_CAN_FRAME_DATA_SIZE]; /*!< Contains the data to be transmitted. It ranges from 0 
                        to 0xFF. */
}stHalCanTxMsg;


typedef __packed struct __stHalCANTX_STRUCT
{
    stHalCanTxMsg*  pTxMsg;
    u16 rdindex;
    u16 wrindex;
    vu8 err_code;
    unsigned int unCanReg;
}stHalCANTX_STRUCT;

typedef __packed struct __stHalCANRX_STRUCT{
	stHalCanRxMsg* pRxMsg;
	u16 wrindex;
	u16 rdindex;
	vu8 err_code;
}stHalCANRX_STRUCT;

typedef enum __eCanBaudrate
{
	eCAN_1MBPS = 0,
	eCAN_500KBPS,
	eCAN_250KBPS,
	eCAN_125KBPS,
	eCAN_100KBPS,
	eCAN_50KBPS,
}eCanBaudrate;

typedef enum _eCAN_TYPE
{
	eCAN_TYPE_HIGH_CAN1 = 0,
	eCAN_TYPE_HIGH_CAN2,
	eCAN_TYPE_HIGH_CAN3,
	eCAN_TYPE_LOW_CAN,
} eCAN_TYPE;


typedef __packed union _stHalCanFilter{
    u16 data[8];
    struct {
        unsigned dummy:1;
        unsigned rtr:1;
        unsigned ide:1;
        unsigned extid:18;// bit 0~bit17
        unsigned stdid:11;// bit 0~bit10
    }bits32[2];

    struct {

        unsigned extid:3; // bit 15~bit17
        unsigned ide:1;
        unsigned rtr:1;
        unsigned stdid:11;// bit 0~bit10
    }bits16[4];
}stHalCanFilter;

// @brief  CAN filter init structure definition
typedef __packed struct __stHalCAN_FilterInitTypeDef
{
  uint16_t CAN_FilterIdHigh;         /*!< Specifies the filter identification number (MSBs for a 32-bit
                                              configuration, first one for a 16-bit configuration).
                                              This parameter can be a value between 0x0000 and 0xFFFF */

  uint16_t CAN_FilterIdLow;          /*!< Specifies the filter identification number (LSBs for a 32-bit
                                              configuration, second one for a 16-bit configuration).
                                              This parameter can be a value between 0x0000 and 0xFFFF */

  uint16_t CAN_FilterMaskIdHigh;     /*!< Specifies the filter mask number or identification number,
                                              according to the mode (MSBs for a 32-bit configuration,
                                              first one for a 16-bit configuration).
                                              This parameter can be a value between 0x0000 and 0xFFFF */

  uint16_t CAN_FilterMaskIdLow;      /*!< Specifies the filter mask number or identification number,
                                              according to the mode (LSBs for a 32-bit configuration,
                                              second one for a 16-bit configuration).
                                              This parameter can be a value between 0x0000 and 0xFFFF */

  uint16_t CAN_FilterFIFOAssignment; /*!< Specifies the FIFO (0 or 1) which will be assigned to the filter.
                                              This parameter can be a value of @ref CAN_filter_FIFO */
  
  uint8_t CAN_FilterNumber;          /*!< Specifies the filter which will be initialized. It ranges from 0 to 13. */

  uint8_t CAN_FilterMode;            /*!< Specifies the filter mode to be initialized.
                                              This parameter can be a value of @ref CAN_filter_mode */

  uint8_t CAN_FilterScale;           /*!< Specifies the filter scale.
                                              This parameter can be a value of @ref CAN_filter_scale */

  eHalFunctionalState CAN_FilterActivation; /*!< Enable or disable the filter.
                                              This parameter can be set either to ENABLE or DISABLE. */
} stHalCAN_FilterInitTypeDef;


/* Exported constants --------------------------------------------------------*/
/* Exported macro & function prototypes --------------------------------------*/



//void HalDrvCanEvent(char* pcEventHandle);

void HalCan_Initial(void);
void HalCan_makeTxHeader( stHalCANTX_STRUCT *cantx, unsigned int inCANId, unsigned char ucCANtype, unsigned char ucLen );
unsigned char HalCanSet_Std(stHalCANTX_STRUCT *cantx, u32 stdid);
unsigned char HalCanSet_Ext(stHalCANTX_STRUCT *cantx, u32 extid);
signed char HalCan_Tx(stHalCANTX_STRUCT *cantx, u8 *buf, u16 size);
unsigned char HalCan1_Read( stHalCanRxMsg* rxmsg ); 
unsigned char HalCan2_Read( stHalCanRxMsg* rxmsg );
void HalCan_DisableRxInterrupt(unsigned int CANxAddr);
void HalCan_EnableRxInterrupt(unsigned int CANxAddr);
unsigned char HalCan_ClearBuffer(unsigned int wParam);

unsigned char HalCanSetBaudrate(int nCANChAddr, eCanBaudrate eBaudrate);

eHalReturnStatus HalCan_SetCanFilter(unsigned char nChannel, unsigned char nCANIDType, 
                            unsigned char nMaskNum, unsigned int *pStartMaskValue, 
                            unsigned int *pEndMaskValue, unsigned int unFIFOAssignNo);
void HalCan_makeTxHeader( stHalCANTX_STRUCT *cantx, unsigned int inCANId, unsigned char ucCANtype, unsigned char ucLen );
stHalCANTX_STRUCT* HalCan_GetTxStructAddr(unsigned int nCANChAddr);

unsigned short HalCan_GetRxQueueCount(stHalCANRX_STRUCT *CanRxCtrl);
unsigned short HalCan_GetTxQueueCount(stHalCANTX_STRUCT *CanTxCtrl);

////////////////////////////////////////////////////////////////////////////////////////
int HalDrvCanOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvCanRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvCanWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvCanIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvCanClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
////////////////////////////////////////////////////////////////////////////////////////



#endif //__HAL_CAN_DRIVER_H__
