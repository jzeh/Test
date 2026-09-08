#ifndef __HAL_ADC_DRIVER_H__
#define __HAL_ADC_DRIVER_H__


#include "common.h"
/* Exported define -----------------------------------------------------------*/
#define HAL_ADC1                    ADC1
#define HAL_ADC2                    ADC2
#define HAL_ADC2_DMA                DMA2
#define HAL_ADC2_GPIOPIN            GPIO_ADC_BAT
#define HAL_ADC2_DMA_CHANNELx       eDMA_Channel_1
#define HAL_ADC2_DMA_STREAMx        eDMA2_Stream2


// @defgroup ADC_Common_mode 
#define HAL_ADC_Mode_Independent                       ((uint32_t)0x00000000)       
#define HAL_ADC_DualMode_RegSimult_InjecSimult         ((uint32_t)0x00000001)
#define HAL_ADC_DualMode_RegSimult_AlterTrig           ((uint32_t)0x00000002)
#define HAL_ADC_DualMode_InjecSimult                   ((uint32_t)0x00000005)
#define HAL_ADC_DualMode_RegSimult                     ((uint32_t)0x00000006)
#define HAL_ADC_DualMode_Interl                        ((uint32_t)0x00000007)
#define HAL_ADC_DualMode_AlterTrig                     ((uint32_t)0x00000009)
#define HAL_ADC_TripleMode_RegSimult_InjecSimult       ((uint32_t)0x00000011)
#define HAL_ADC_TripleMode_RegSimult_AlterTrig         ((uint32_t)0x00000012)
#define HAL_ADC_TripleMode_InjecSimult                 ((uint32_t)0x00000015)
#define HAL_ADC_TripleMode_RegSimult                   ((uint32_t)0x00000016)
#define HAL_ADC_TripleMode_Interl                      ((uint32_t)0x00000017)
#define HAL_ADC_TripleMode_AlterTrig                   ((uint32_t)0x00000019)
// @defgroup ADC_Prescaler 
#define HAL_ADC_Prescaler_Div2                         ((uint32_t)0x00000000)
#define HAL_ADC_Prescaler_Div4                         ((uint32_t)0x00010000)
#define HAL_ADC_Prescaler_Div6                         ((uint32_t)0x00020000)
#define HAL_ADC_Prescaler_Div8                         ((uint32_t)0x00030000)
// @defgroup ADC_Direct_memory_access_mode_for_multi_mode 
#define HAL_ADC_DMAAccessMode_Disabled      ((uint32_t)0x00000000)     /* DMA mode disabled */
#define HAL_ADC_DMAAccessMode_1             ((uint32_t)0x00004000)     /* DMA mode 1 enabled (2 / 3 half-words one by one - 1 then 2 then 3)*/
#define HAL_ADC_DMAAccessMode_2             ((uint32_t)0x00008000)     /* DMA mode 2 enabled (2 / 3 half-words by pairs - 2&1 then 1&3 then 3&2)*/
#define HAL_ADC_DMAAccessMode_3             ((uint32_t)0x0000C000)     /* DMA mode 3 enabled (2 / 3 bytes by pairs - 2&1 then 1&3 then 3&2) */
// @defgroup ADC_delay_between_2_sampling_phases 
#define HAL_ADC_TwoSamplingDelay_5Cycles               ((uint32_t)0x00000000)
#define HAL_ADC_TwoSamplingDelay_6Cycles               ((uint32_t)0x00000100)
#define HAL_ADC_TwoSamplingDelay_7Cycles               ((uint32_t)0x00000200)
#define HAL_ADC_TwoSamplingDelay_8Cycles               ((uint32_t)0x00000300)
#define HAL_ADC_TwoSamplingDelay_9Cycles               ((uint32_t)0x00000400)
#define HAL_ADC_TwoSamplingDelay_10Cycles              ((uint32_t)0x00000500)
#define HAL_ADC_TwoSamplingDelay_11Cycles              ((uint32_t)0x00000600)
#define HAL_ADC_TwoSamplingDelay_12Cycles              ((uint32_t)0x00000700)
#define HAL_ADC_TwoSamplingDelay_13Cycles              ((uint32_t)0x00000800)
#define HAL_ADC_TwoSamplingDelay_14Cycles              ((uint32_t)0x00000900)
#define HAL_ADC_TwoSamplingDelay_15Cycles              ((uint32_t)0x00000A00)
#define HAL_ADC_TwoSamplingDelay_16Cycles              ((uint32_t)0x00000B00)
#define HAL_ADC_TwoSamplingDelay_17Cycles              ((uint32_t)0x00000C00)
#define HAL_ADC_TwoSamplingDelay_18Cycles              ((uint32_t)0x00000D00)
#define HAL_ADC_TwoSamplingDelay_19Cycles              ((uint32_t)0x00000E00)
#define HAL_ADC_TwoSamplingDelay_20Cycles              ((uint32_t)0x00000F00)
// @defgroup ADC_external_trigger_edge_for_regular_channels_conversion 
#define HAL_ADC_ExternalTrigConvEdge_None          ((uint32_t)0x00000000)
#define HAL_ADC_ExternalTrigConvEdge_Rising        ((uint32_t)0x10000000)
#define HAL_ADC_ExternalTrigConvEdge_Falling       ((uint32_t)0x20000000)
#define HAL_ADC_ExternalTrigConvEdge_RisingFalling ((uint32_t)0x30000000)
// @defgroup ADC_resolution 
#define HAL_ADC_Resolution_12b                         ((uint32_t)0x00000000)
#define HAL_ADC_Resolution_10b                         ((uint32_t)0x01000000)
#define HAL_ADC_Resolution_8b                          ((uint32_t)0x02000000)
#define HAL_ADC_Resolution_6b                          ((uint32_t)0x03000000)
// @defgroup ADC_external_trigger_edge_for_regular_channels_conversion 
#define HAL_ADC_ExternalTrigConvEdge_None          ((uint32_t)0x00000000)
#define HAL_ADC_ExternalTrigConvEdge_Rising        ((uint32_t)0x10000000)
#define HAL_ADC_ExternalTrigConvEdge_Falling       ((uint32_t)0x20000000)
#define HAL_ADC_ExternalTrigConvEdge_RisingFalling ((uint32_t)0x30000000)
// @defgroup ADC_extrenal_trigger_sources_for_regular_channels_conversion 
#define HAL_ADC_ExternalTrigConv_T1_CC1                ((uint32_t)0x00000000)
#define HAL_ADC_ExternalTrigConv_T1_CC2                ((uint32_t)0x01000000)
#define HAL_ADC_ExternalTrigConv_T1_CC3                ((uint32_t)0x02000000)
#define HAL_ADC_ExternalTrigConv_T2_CC2                ((uint32_t)0x03000000)
#define HAL_ADC_ExternalTrigConv_T2_CC3                ((uint32_t)0x04000000)
#define HAL_ADC_ExternalTrigConv_T2_CC4                ((uint32_t)0x05000000)
#define HAL_ADC_ExternalTrigConv_T2_TRGO               ((uint32_t)0x06000000)
#define HAL_ADC_ExternalTrigConv_T3_CC1                ((uint32_t)0x07000000)
#define HAL_ADC_ExternalTrigConv_T3_TRGO               ((uint32_t)0x08000000)
#define HAL_ADC_ExternalTrigConv_T4_CC4                ((uint32_t)0x09000000)
#define HAL_ADC_ExternalTrigConv_T5_CC1                ((uint32_t)0x0A000000)
#define HAL_ADC_ExternalTrigConv_T5_CC2                ((uint32_t)0x0B000000)
#define HAL_ADC_ExternalTrigConv_T5_CC3                ((uint32_t)0x0C000000)
#define HAL_ADC_ExternalTrigConv_T8_CC1                ((uint32_t)0x0D000000)
#define HAL_ADC_ExternalTrigConv_T8_TRGO               ((uint32_t)0x0E000000)
#define HAL_ADC_ExternalTrigConv_Ext_IT11              ((uint32_t)0x0F000000)
// @defgroup ADC_data_align 
#define HAL_ADC_DataAlign_Right                        ((uint32_t)0x00000000)
#define HAL_ADC_DataAlign_Left                         ((uint32_t)0x00000800)
// @defgroup ADC_channels 
#define HAL_ADC_Channel_0                               ((uint8_t)0x00)
#define HAL_ADC_Channel_1                               ((uint8_t)0x01)
#define HAL_ADC_Channel_2                               ((uint8_t)0x02)
#define HAL_ADC_Channel_3                               ((uint8_t)0x03)
#define HAL_ADC_Channel_4                               ((uint8_t)0x04)
#define HAL_ADC_Channel_5                               ((uint8_t)0x05)
#define HAL_ADC_Channel_6                               ((uint8_t)0x06)
#define HAL_ADC_Channel_7                               ((uint8_t)0x07)
#define HAL_ADC_Channel_8                               ((uint8_t)0x08)
#define HAL_ADC_Channel_9                               ((uint8_t)0x09)
#define HAL_ADC_Channel_10                              ((uint8_t)0x0A)
#define HAL_ADC_Channel_11                              ((uint8_t)0x0B)
#define HAL_ADC_Channel_12                              ((uint8_t)0x0C)
#define HAL_ADC_Channel_13                              ((uint8_t)0x0D)
#define HAL_ADC_Channel_14                              ((uint8_t)0x0E)
#define HAL_ADC_Channel_15                              ((uint8_t)0x0F)
#define HAL_ADC_Channel_16                              ((uint8_t)0x10)
#define HAL_ADC_Channel_17                              ((uint8_t)0x11)
#define HAL_ADC_Channel_18                              ((uint8_t)0x12)

// @defgroup ADC_injected_channel_selection 
#define HAL_ADC_InjectedChannel_1                       ((uint8_t)0x14)
#define HAL_ADC_InjectedChannel_2                       ((uint8_t)0x18)
#define HAL_ADC_InjectedChannel_3                       ((uint8_t)0x1C)
#define HAL_ADC_InjectedChannel_4                       ((uint8_t)0x20)
// @defgroup ADC_sampling_times 
#define HAL_ADC_SampleTime_3Cycles                    ((uint8_t)0x00)
#define HAL_ADC_SampleTime_15Cycles                   ((uint8_t)0x01)
#define HAL_ADC_SampleTime_28Cycles                   ((uint8_t)0x02)
#define HAL_ADC_SampleTime_56Cycles                   ((uint8_t)0x03)
#define HAL_ADC_SampleTime_84Cycles                   ((uint8_t)0x04)
#define HAL_ADC_SampleTime_112Cycles                  ((uint8_t)0x05)
#define HAL_ADC_SampleTime_144Cycles                  ((uint8_t)0x06)
#define HAL_ADC_SampleTime_480Cycles                  ((uint8_t)0x07)
// @defgroup ADC_interrupts_definition 
#define HAL_ADC_IT_EOC                                 ((uint16_t)0x0205)  
#define HAL_ADC_IT_AWD                                 ((uint16_t)0x0106)  
#define HAL_ADC_IT_JEOC                                ((uint16_t)0x0407)  
#define HAL_ADC_IT_OVR                                 ((uint16_t)0x201A)  
// @defgroup ADC_flags_definition 
#define HAL_ADC_FLAG_AWD                               ((uint8_t)0x01)
#define HAL_ADC_FLAG_EOC                               ((uint8_t)0x02)
#define HAL_ADC_FLAG_JEOC                              ((uint8_t)0x04)
#define HAL_ADC_FLAG_JSTRT                             ((uint8_t)0x08)
#define HAL_ADC_FLAG_STRT                              ((uint8_t)0x10)
#define HAL_ADC_FLAG_OVR                               ((uint8_t)0x20)   
// @defgroup ADC_interrupts_definition
#define HAL_ADC_IT_EOC                                 ((uint16_t)0x0205)  
#define HAL_ADC_IT_AWD                                 ((uint16_t)0x0106)  
#define HAL_ADC_IT_JEOC                                ((uint16_t)0x0407)  
#define HAL_ADC_IT_OVR                                 ((uint16_t)0x201A)  

/* Exported types - Structure, Enumeration -----------------------------------*/
typedef enum __eHalADC_IOCtlMode{
    eADC_IO_Init,
    eADC_IO_CommonInit,
    eADC_IO_PortEnable,
    eADC_IO_ChConfig,
    eADC_IO_DeInit,
    eADC_IO_GetFlagStatus,
    eADC_IO_ClearFlagStatus,
    eADC_IO_GetITFlagStatus,
    eADC_IO_ClearITStatus,
    eADC_IO_IntEnable,
    eADC_IO_DMAEnable,
    eADC_IO_DMARequestRepeatEnable,
    eADC_IO_Voltage_Config,
    eADC_IO_Calbration_Config,
    eADC_IO_SoftwareStartConv,
    eADC_IO_SoftwareStartInjectedConv,
    eADC_IO_GetSoftwareInjectedConvValue,

}eHalADC_IOCtlMode;


typedef __packed struct __stHalADC_InitTypeDef
{
  uint32_t ADC_Resolution;                /*!< Configures the ADC resolution dual mode. 
                                               This parameter can be a value of @ref ADC_resolution */                                   
  eHalFunctionalState ADC_ScanConvMode;       /*!< Specifies whether the conversion 
                                               is performed in Scan (multichannels) 
                                               or Single (one channel) mode.
                                               This parameter can be set to ENABLE or DISABLE */ 
  eHalFunctionalState ADC_ContinuousConvMode; /*!< Specifies whether the conversion 
                                               is performed in Continuous or Single mode.
                                               This parameter can be set to ENABLE or DISABLE. */
  uint32_t ADC_ExternalTrigConvEdge;      /*!< Select the external trigger edge and
                                               enable the trigger of a regular group. 
                                               This parameter can be a value of 
                                               @ref ADC_external_trigger_edge_for_regular_channels_conversion */
  uint32_t ADC_ExternalTrigConv;          /*!< Select the external event used to trigger 
                                               the start of conversion of a regular group.
                                               This parameter can be a value of 
                                               @ref ADC_extrenal_trigger_sources_for_regular_channels_conversion */
  uint32_t ADC_DataAlign;                 /*!< Specifies whether the ADC data  alignment
                                               is left or right. This parameter can be 
                                               a value of @ref ADC_data_align */
  uint8_t  ADC_NbrOfConversion;           /*!< Specifies the number of ADC conversions
                                               that will be done using the sequencer for
                                               regular channel group.
                                               This parameter must range from 1 to 16. */
}stHalADC_InitTypeDef;

typedef __packed struct __stHalADC_CommonInitTypeDef
{
  uint32_t ADC_Mode;                      /*!< Configures the ADC to operate in 
                                               independent or multi mode. 
                                               This parameter can be a value of @ref ADC_Common_mode */                                              
  uint32_t ADC_Prescaler;                 /*!< Select the frequency of the clock 
                                               to the ADC. The clock is common for all the ADCs.
                                               This parameter can be a value of @ref ADC_Prescaler */
  uint32_t ADC_DMAAccessMode;             /*!< Configures the Direct memory access 
                                              mode for multi ADC mode.
                                               This parameter can be a value of 
                                               @ref ADC_Direct_memory_access_mode_for_multi_mode */
  uint32_t ADC_TwoSamplingDelay;          /*!< Configures the Delay between 2 sampling phases.
                                               This parameter can be a value of 
                                               @ref ADC_delay_between_2_sampling_phases */
  
}stHalADC_CommonInitTypeDef;

/* Exported constants --------------------------------------------------------*/
/* Exported macro & function prototypes --------------------------------------*/

int HalDrvAdcOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvAdcRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvAdcWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvAdcIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);
int HalDrvAdcClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap);


void ADCHandlerInit();
void ADCHandlerDeInit();



u8 HalADC_Read1(u16 *buf);
u8 HalADC_Read2(u16 *buf);
void HalADC_SetAdc_Chnnel(unsigned int unAdcAddr, unsigned char ucSequence);
void HalADC_Init(void);

#endif //__ADC_DRIVER_H__
