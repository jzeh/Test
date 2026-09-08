#ifndef __HAL_DMA_DRIVER_H__
#define __HAL_DMA_DRIVER_H__


#include "common.h"
/* Exported define -----------------------------------------------------------*/
#define HAL_DMA_RX_MODE                         0
#define HAL_DMA_TX_MODE                         1
//@defgroup DMA_channel 
#define HAL_DMA_Channel_0                     ((uint32_t)0x00000000)
#define HAL_DMA_Channel_1                     ((uint32_t)0x02000000)
#define HAL_DMA_Channel_2                     ((uint32_t)0x04000000)
#define HAL_DMA_Channel_3                     ((uint32_t)0x06000000)
#define HAL_DMA_Channel_4                     ((uint32_t)0x08000000)
#define HAL_DMA_Channel_5                     ((uint32_t)0x0A000000)
#define HAL_DMA_Channel_6                     ((uint32_t)0x0C000000)
#define HAL_DMA_Channel_7                     ((uint32_t)0x0E000000)
// @defgroup DMA_data_transfer_direction 
#define HAL_DMA_DIR_PeripheralToMemory        ((uint32_t)0x00000000)
#define HAL_DMA_DIR_MemoryToPeripheral        ((uint32_t)0x00000040) 
#define HAL_DMA_DIR_MemoryToMemory            ((uint32_t)0x00000080)
// @defgroup DMA_peripheral_incremented_mode 
#define HAL_DMA_PeripheralInc_Enable          ((uint32_t)0x00000200)
#define HAL_DMA_PeripheralInc_Disable         ((uint32_t)0x00000000)
// @defgroup DMA_memory_incremented_mode 
#define HAL_DMA_MemoryInc_Enable              ((uint32_t)0x00000400)
#define HAL_DMA_MemoryInc_Disable             ((uint32_t)0x00000000)
// @defgroup DMA_peripheral_data_size 
#define HAL_DMA_PeripheralDataSize_Byte       ((uint32_t)0x00000000) 
#define HAL_DMA_PeripheralDataSize_HalfWord   ((uint32_t)0x00000800) 
#define HAL_DMA_PeripheralDataSize_Word       ((uint32_t)0x00001000)
// @defgroup DMA_memory_data_size 
#define HAL_DMA_MemoryDataSize_Byte           ((uint32_t)0x00000000) 
#define HAL_DMA_MemoryDataSize_HalfWord       ((uint32_t)0x00002000) 
#define HAL_DMA_MemoryDataSize_Word           ((uint32_t)0x00004000)
// @defgroup DMA_circular_normal_mode 
#define HAL_DMA_Mode_Normal                   ((uint32_t)0x00000000) 
#define HAL_DMA_Mode_Circular                 ((uint32_t)0x00000100)
// @defgroup DMA_priority_level 
#define HAL_DMA_Priority_Low                  ((uint32_t)0x00000000)
#define HAL_DMA_Priority_Medium               ((uint32_t)0x00010000) 
#define HAL_DMA_Priority_High                 ((uint32_t)0x00020000)
#define HAL_DMA_Priority_VeryHigh             ((uint32_t)0x00030000)
// @defgroup DMA_fifo_direct_mode 
#define HAL_DMA_FIFOMode_Disable              ((uint32_t)0x00000000) 
#define HAL_DMA_FIFOMode_Enable               ((uint32_t)0x00000004)
// @defgroup DMA_fifo_threshold_level 
#define HAL_DMA_FIFOThreshold_1QuarterFull    ((uint32_t)0x00000000)
#define HAL_DMA_FIFOThreshold_HalfFull        ((uint32_t)0x00000001) 
#define HAL_DMA_FIFOThreshold_3QuartersFull   ((uint32_t)0x00000002)
#define HAL_DMA_FIFOThreshold_Full            ((uint32_t)0x00000003)
// @defgroup DMA_memory_burst 
#define HAL_DMA_MemoryBurst_Single            ((uint32_t)0x00000000)
#define HAL_DMA_MemoryBurst_INC4              ((uint32_t)0x00800000)  
#define HAL_DMA_MemoryBurst_INC8              ((uint32_t)0x01000000)
#define HAL_DMA_MemoryBurst_INC16             ((uint32_t)0x01800000)
// @defgroup DMA_peripheral_burst 
#define HAL_DMA_PeripheralBurst_Single        ((uint32_t)0x00000000)
#define HAL_DMA_PeripheralBurst_INC4          ((uint32_t)0x00200000)  
#define HAL_DMA_PeripheralBurst_INC8          ((uint32_t)0x00400000)
#define HAL_DMA_PeripheralBurst_INC16         ((uint32_t)0x00600000)
// @defgroup DMA_fifo_status_level 
#define HAL_DMA_FIFOStatus_Less1QuarterFull   ((uint32_t)0x00000000 << 3)
#define HAL_DMA_FIFOStatus_1QuarterFull       ((uint32_t)0x00000001 << 3)
#define HAL_DMA_FIFOStatus_HalfFull           ((uint32_t)0x00000002 << 3) 
#define HAL_DMA_FIFOStatus_3QuartersFull      ((uint32_t)0x00000003 << 3)
#define HAL_DMA_FIFOStatus_Empty              ((uint32_t)0x00000004 << 3)
#define HAL_DMA_FIFOStatus_Full               ((uint32_t)0x00000005 << 3)
// @defgroup DMA_flags_definition 
#define HAL_DMA_FLAG_FEIF0                    ((uint32_t)0x10800001)
#define HAL_DMA_FLAG_DMEIF0                   ((uint32_t)0x10800004)
#define HAL_DMA_FLAG_TEIF0                    ((uint32_t)0x10000008)
#define HAL_DMA_FLAG_HTIF0                    ((uint32_t)0x10000010)
#define HAL_DMA_FLAG_TCIF0                    ((uint32_t)0x10000020)
#define HAL_DMA_FLAG_FEIF1                    ((uint32_t)0x10000040)
#define HAL_DMA_FLAG_DMEIF1                   ((uint32_t)0x10000100)
#define HAL_DMA_FLAG_TEIF1                    ((uint32_t)0x10000200)
#define HAL_DMA_FLAG_HTIF1                    ((uint32_t)0x10000400)
#define HAL_DMA_FLAG_TCIF1                    ((uint32_t)0x10000800)
#define HAL_DMA_FLAG_FEIF2                    ((uint32_t)0x10010000)
#define HAL_DMA_FLAG_DMEIF2                   ((uint32_t)0x10040000)
#define HAL_DMA_FLAG_TEIF2                    ((uint32_t)0x10080000)
#define HAL_DMA_FLAG_HTIF2                    ((uint32_t)0x10100000)
#define HAL_DMA_FLAG_TCIF2                    ((uint32_t)0x10200000)
#define HAL_DMA_FLAG_FEIF3                    ((uint32_t)0x10400000)
#define HAL_DMA_FLAG_DMEIF3                   ((uint32_t)0x11000000)
#define HAL_DMA_FLAG_TEIF3                    ((uint32_t)0x12000000)
#define HAL_DMA_FLAG_HTIF3                    ((uint32_t)0x14000000)
#define HAL_DMA_FLAG_TCIF3                    ((uint32_t)0x18000000)
#define HAL_DMA_FLAG_FEIF4                    ((uint32_t)0x20000001)
#define HAL_DMA_FLAG_DMEIF4                   ((uint32_t)0x20000004)
#define HAL_DMA_FLAG_TEIF4                    ((uint32_t)0x20000008)
#define HAL_DMA_FLAG_HTIF4                    ((uint32_t)0x20000010)
#define HAL_DMA_FLAG_TCIF4                    ((uint32_t)0x20000020)
#define HAL_DMA_FLAG_FEIF5                    ((uint32_t)0x20000040)
#define HAL_DMA_FLAG_DMEIF5                   ((uint32_t)0x20000100)
#define HAL_DMA_FLAG_TEIF5                    ((uint32_t)0x20000200)
#define HAL_DMA_FLAG_HTIF5                    ((uint32_t)0x20000400)
#define HAL_DMA_FLAG_TCIF5                    ((uint32_t)0x20000800)
#define HAL_DMA_FLAG_FEIF6                    ((uint32_t)0x20010000)
#define HAL_DMA_FLAG_DMEIF6                   ((uint32_t)0x20040000)
#define HAL_DMA_FLAG_TEIF6                    ((uint32_t)0x20080000)
#define HAL_DMA_FLAG_HTIF6                    ((uint32_t)0x20100000)
#define HAL_DMA_FLAG_TCIF6                    ((uint32_t)0x20200000)
#define HAL_DMA_FLAG_FEIF7                    ((uint32_t)0x20400000)
#define HAL_DMA_FLAG_DMEIF7                   ((uint32_t)0x21000000)
#define HAL_DMA_FLAG_TEIF7                    ((uint32_t)0x22000000)
#define HAL_DMA_FLAG_HTIF7                    ((uint32_t)0x24000000)
#define HAL_DMA_FLAG_TCIF7                    ((uint32_t)0x28000000)
// @defgroup DMA_interrupt_enable_definitions 
#define HAL_DMA_IT_TC                         ((uint32_t)0x00000010)
#define HAL_DMA_IT_HT                         ((uint32_t)0x00000008)
#define HAL_DMA_IT_TE                         ((uint32_t)0x00000004)
#define HAL_DMA_IT_DME                        ((uint32_t)0x00000002)
#define HAL_DMA_IT_FE                         ((uint32_t)0x00000080)
// @defgroup DMA_interrupts_definitions 
#define HAL_DMA_IT_FEIF0                      ((uint32_t)0x90000001)
#define HAL_DMA_IT_DMEIF0                     ((uint32_t)0x10001004)
#define HAL_DMA_IT_TEIF0                      ((uint32_t)0x10002008)
#define HAL_DMA_IT_HTIF0                      ((uint32_t)0x10004010)
#define HAL_DMA_IT_TCIF0                      ((uint32_t)0x10008020)
#define HAL_DMA_IT_FEIF1                      ((uint32_t)0x90000040)
#define HAL_DMA_IT_DMEIF1                     ((uint32_t)0x10001100)
#define HAL_DMA_IT_TEIF1                      ((uint32_t)0x10002200)
#define HAL_DMA_IT_HTIF1                      ((uint32_t)0x10004400)
#define HAL_DMA_IT_TCIF1                      ((uint32_t)0x10008800)
#define HAL_DMA_IT_FEIF2                      ((uint32_t)0x90010000)
#define HAL_DMA_IT_DMEIF2                     ((uint32_t)0x10041000)
#define HAL_DMA_IT_TEIF2                      ((uint32_t)0x10082000)
#define HAL_DMA_IT_HTIF2                      ((uint32_t)0x10104000)
#define HAL_DMA_IT_TCIF2                      ((uint32_t)0x10208000)
#define HAL_DMA_IT_FEIF3                      ((uint32_t)0x90400000)
#define HAL_DMA_IT_DMEIF3                     ((uint32_t)0x11001000)
#define HAL_DMA_IT_TEIF3                      ((uint32_t)0x12002000)
#define HAL_DMA_IT_HTIF3                      ((uint32_t)0x14004000)
#define HAL_DMA_IT_TCIF3                      ((uint32_t)0x18008000)
#define HAL_DMA_IT_FEIF4                      ((uint32_t)0xA0000001)
#define HAL_DMA_IT_DMEIF4                     ((uint32_t)0x20001004)
#define HAL_DMA_IT_TEIF4                      ((uint32_t)0x20002008)
#define HAL_DMA_IT_HTIF4                      ((uint32_t)0x20004010)
#define HAL_DMA_IT_TCIF4                      ((uint32_t)0x20008020)
#define HAL_DMA_IT_FEIF5                      ((uint32_t)0xA0000040)
#define HAL_DMA_IT_DMEIF5                     ((uint32_t)0x20001100)
#define HAL_DMA_IT_TEIF5                      ((uint32_t)0x20002200)
#define HAL_DMA_IT_HTIF5                      ((uint32_t)0x20004400)
#define HAL_DMA_IT_TCIF5                      ((uint32_t)0x20008800)
#define HAL_DMA_IT_FEIF6                      ((uint32_t)0xA0010000)
#define HAL_DMA_IT_DMEIF6                     ((uint32_t)0x20041000)
#define HAL_DMA_IT_TEIF6                      ((uint32_t)0x20082000)
#define HAL_DMA_IT_HTIF6                      ((uint32_t)0x20104000)
#define HAL_DMA_IT_TCIF6                      ((uint32_t)0x20208000)
#define HAL_DMA_IT_FEIF7                      ((uint32_t)0xA0400000)
#define HAL_DMA_IT_DMEIF7                     ((uint32_t)0x21001000)
#define HAL_DMA_IT_TEIF7                      ((uint32_t)0x22002000)
#define HAL_DMA_IT_HTIF7                      ((uint32_t)0x24004000)
#define HAL_DMA_IT_TCIF7                      ((uint32_t)0x28008000)
// @defgroup DMA_peripheral_increment_offset 
#define HAL_DMA_PINCOS_Psize                  ((uint32_t)0x00000000)
#define HAL_DMA_PINCOS_WordAligned            ((uint32_t)0x00008000)
// @defgroup DMA_flow_controller_definitions 
#define HAL_DMA_FlowCtrl_Memory               ((uint32_t)0x00000000)
#define HAL_DMA_FlowCtrl_Peripheral           ((uint32_t)0x00000020)
// @defgroup DMA_memory_targets_definitions 
#define HAL_DMA_Memory_0                      ((uint32_t)0x00000000)
#define HAL_DMA_Memory_1                      ((uint32_t)0x00080000)

/* Exported types - Structure, Enumeration -----------------------------------*/
typedef enum __eHalDmaChannel{
    eDMA_Channel_0,
    eDMA_Channel_1,
    eDMA_Channel_2,
    eDMA_Channel_3,
    eDMA_Channel_4,
    eDMA_Channel_5,
    eDMA_Channel_6,
    eDMA_Channel_7,
}eHalDmaChannel;

typedef enum __eHalDmaStream{
    eDMA1_Stream0,
    eDMA1_Stream1,
    eDMA1_Stream2,
    eDMA1_Stream3,
    eDMA1_Stream4,
    eDMA1_Stream5,
    eDMA1_Stream6,

    eDMA2_Stream0,
    eDMA2_Stream1,
    eDMA2_Stream2,
    eDMA2_Stream3,
    eDMA2_Stream4,
    eDMA2_Stream5,
    eDMA2_Stream6,
}eHalDmaStream;

typedef enum __eHalDmaIoCtlMode{
    eDMA_IO_Init,
    eDMA_IO_DeInit,
    eDMA_IO_MuxInit,
    eDMA_IO_MuxEnable,
    eDMA_IO_ChannelEnable,
    eDMA_IO_INT_Enable,
    eDMA_IO_GetDMACount,
    
}eHalDmaIoCtlMode;

typedef __packed struct __stHalDMA_InitTypeDef
{
  uint32_t DMA_Channel;            /*!< Specifies the channel used for the specified stream. 
                                        This parameter can be a value of @ref DMA_channel */
 
  uint32_t DMA_PeripheralBaseAddr; /*!< Specifies the peripheral base address for DMAy Streamx. */

  uint32_t DMA_Memory0BaseAddr;    /*!< Specifies the memory 0 base address for DMAy Streamx. 
                                        This memory is the default memory used when double buffer mode is
                                        not enabled. */

  uint32_t DMA_DIR;                /*!< Specifies if the data will be transferred from memory to peripheral, 
                                        from memory to memory or from peripheral to memory.
                                        This parameter can be a value of @ref DMA_data_transfer_direction */

  uint32_t DMA_BufferSize;         /*!< Specifies the buffer size, in data unit, of the specified Stream. 
                                        The data unit is equal to the configuration set in DMA_PeripheralDataSize
                                        or DMA_MemoryDataSize members depending in the transfer direction. */

  uint32_t DMA_PeripheralInc;      /*!< Specifies whether the Peripheral address register should be incremented or not.
                                        This parameter can be a value of @ref DMA_peripheral_incremented_mode */

  uint32_t DMA_MemoryInc;          /*!< Specifies whether the memory address register should be incremented or not.
                                        This parameter can be a value of @ref DMA_memory_incremented_mode */

  uint32_t DMA_PeripheralDataSize; /*!< Specifies the Peripheral data width.
                                        This parameter can be a value of @ref DMA_peripheral_data_size */

  uint32_t DMA_MemoryDataSize;     /*!< Specifies the Memory data width.
                                        This parameter can be a value of @ref DMA_memory_data_size */

  uint32_t DMA_Mode;               /*!< Specifies the operation mode of the DMAy Streamx.
                                        This parameter can be a value of @ref DMA_circular_normal_mode
                                        @note The circular buffer mode cannot be used if the memory-to-memory
                                              data transfer is configured on the selected Stream */

  uint32_t DMA_Priority;           /*!< Specifies the software priority for the DMAy Streamx.
                                        This parameter can be a value of @ref DMA_priority_level */

  uint32_t DMA_FIFOMode;          /*!< Specifies if the FIFO mode or Direct mode will be used for the specified Stream.
                                        This parameter can be a value of @ref DMA_fifo_direct_mode
                                        @note The Direct mode (FIFO mode disabled) cannot be used if the 
                                               memory-to-memory data transfer is configured on the selected Stream */

  uint32_t DMA_FIFOThreshold;      /*!< Specifies the FIFO threshold level.
                                        This parameter can be a value of @ref DMA_fifo_threshold_level */

  uint32_t DMA_MemoryBurst;        /*!< Specifies the Burst transfer configuration for the memory transfers. 
                                        It specifies the amount of data to be transferred in a single non interruptable 
                                        transaction. This parameter can be a value of @ref DMA_memory_burst 
                                        @note The burst mode is possible only if the address Increment mode is enabled. */

  uint32_t DMA_PeripheralBurst;    /*!< Specifies the Burst transfer configuration for the peripheral transfers. 
                                        It specifies the amount of data to be transferred in a single non interruptable 
                                        transaction. This parameter can be a value of @ref DMA_peripheral_burst
                                        @note The burst mode is possible only if the address Increment mode is enabled. */  
}stHalDMA_InitTypeDef;


/* Exported constants --------------------------------------------------------*/
/* Exported macro & function prototypes --------------------------------------*/

int HalDrvDmaOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvDmaRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvDmaWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvDmaIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvDmaClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);

unsigned int HalDrvDma_ConvertDMAChannel(eHalDmaChannel eDmaChNo);

#endif //__HAL_DMA_DRIVER_H__
