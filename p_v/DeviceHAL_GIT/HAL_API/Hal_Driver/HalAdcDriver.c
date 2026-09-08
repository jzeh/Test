/*
  ******************************************************************************
  * @file    HalAdcDriver.c
  * @author  James Jean
  * @version V1.0.0
  * @date    2022-03-02
  * @brief
  *
  *
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#if defined(STM32F427X)
#include "STM32F4xx.h"
#include "stm32f4xx_adc.h"
#elif defined(AT32F435VMT7)
#include "at32f435_437.h"
#include "at32f435_437_adc.h"
#endif

#include "HalAdcDriver.h"
#include "HalHandler.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
//#define ADC_POLLING
#define ADC_DMA

#if defined(STM32F427X)
#define HAL_ADC_DMA_ADDR(X)    (uint32_t)(&(X->DR))
#elif defined(AT32F435VMT7)
#define HAL_ADC_DMA_ADDR(X)    (uint32_t)(&(X->odt))
#endif
//******************************************************************************
// Dev Selftest
//******************************************************************************
#if defined(ADC_DMA)
#define HAL_ADC_CNT  10
__IO uint16_t 		g_usADCBattVoltage[HAL_ADC_CNT];
#endif

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
float 			g_fBatteryVoltage = 0;
volatile __IO   uint16_t ADCConvertedValue[2];

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
void HalDrvGpio_InitGpioADC(void)
{
	stHalGPIO_InitTypeDef  GPIO_InitStructure;

    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOA_GROUP | GPIOC_GROUP, NULL, 0, HAL_ENABLE);
	GPIO_InitStructure.GPIO_DS = eGPIO_DRIVE_STRENGTH_STRONGER;
	GPIO_InitStructure.GPIO_OType = eGPIO_OType_OD;
	GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Pin = HAL_ADC2_GPIOPIN;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);
}


#if defined(ADC_POLLING)
void HalADC_Init(void)
{
	stHalADC_InitTypeDef		ADC_InitStructure;
	stHalADC_CommonInitTypeDef	ADC_CommonInitStructure;

	HalDrvGpio_InitGpioADC();

    HalDrvRccIOCtrl(eRCC_IO_ADC_Clock, eRCC_Clock_ADC1, NULL, 0, HAL_ENABLE);
    
	ADC_CommonInitStructure.ADC_Mode = HAL_ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler = HAL_ADC_Prescaler_Div2;
	ADC_CommonInitStructure.ADC_DMAAccessMode = HAL_ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = HAL_ADC_TwoSamplingDelay_5Cycles;
    HalDrvAdcIOCtrl(eADC_IO_CommonInit, 0, (char*)&ADC_CommonInitStructure, sizeof(ADC_CommonInitStructure), 0);

	ADC_InitStructure.ADC_Resolution = HAL_ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = HAL_ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = HAL_ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = HAL_ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_ExternalTrigConv = HAL_ADC_ExternalTrigConv_T1_CC1;
	ADC_InitStructure.ADC_DataAlign = HAL_ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 1;
    HalDrvAdcIOCtrl(eADC_IO_Init, (int)HAL_ADC1, (char*)&ADC_InitStructure, sizeof(ADC_InitStructure), 0);
    HalDrvAdcIOCtrl(eADC_IO_Init, (int)HAL_ADC2, (char*)&ADC_InitStructure, sizeof(ADC_InitStructure), 0);

    HalADC_SetAdc_Chnnel((unsigned int)HAL_ADC2, 1);
    HalADC_SetAdc_Chnnel((unsigned int)HAL_ADC1, 1);

    HalDrvAdcIOCtrl(eADC_IO_PortEnable, (int)HAL_ADC2, NULL, 0, HAL_ENABLE);
    HalDrvAdcIOCtrl(eADC_IO_PortEnable, (int)HAL_ADC1, NULL, 0, HAL_ENABLE);
}
#endif


#if defined(ADC_DMA)
void HalADC_Init(void)
{
	stHalDMA_InitTypeDef DMA_InitStructure;
    stHalADC_InitTypeDef ADC_InitStructure;
	stHalADC_CommonInitTypeDef ADC_CommonInitStructure;
    stHalGPIO_InitTypeDef  GPIO_InitStructure;

    HalDrvRccIOCtrl(eRCC_IO_ADC_Clock, eRCC_Clock_ADC2, NULL, 0, HAL_ENABLE);
    HalDrvRccIOCtrl(eRCC_IO_GPIO_Clock, GPIOA_GROUP, NULL, 0, HAL_ENABLE);

    GPIO_InitStructure.GPIO_DS = eGPIO_DRIVE_STRENGTH_STRONGER;
	GPIO_InitStructure.GPIO_Pin = HAL_ADC2_GPIOPIN;
    GPIO_InitStructure.GPIO_Mode = eGPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = eGPIO_PuPd_NOPULL;
    HalDrvGpioIOCtrl(eGPIO_IO_INIT, 0, (char*)&GPIO_InitStructure, sizeof(GPIO_InitStructure), 0);

	/* Enable DMA2 clock */
    HalDrvRccIOCtrl(eRCC_IO_DMA_Reset_Clock, eRCC_Clock_DMA2, NULL, 0, HAL_ENABLE);    
    HalDrvRccIOCtrl(eRCC_IO_DMA_Clock, eRCC_Clock_DMA2, NULL, 0, HAL_ENABLE);

	/* DMA2 Stream1 channel2 configuration **************************************/
    HalDrvDmaIOCtrl(eDMA_IO_DeInit, HAL_ADC2_DMA_STREAMx, NULL, 0, 0);


	DMA_InitStructure.DMA_Channel 				= HAL_ADC2_DMA_CHANNELx;
	DMA_InitStructure.DMA_PeripheralBaseAddr	= HAL_ADC_DMA_ADDR(HAL_ADC2);//(uint32_t)HAL_ADC2_DR_ADDRESS;
	DMA_InitStructure.DMA_Memory0BaseAddr 		= (uint32_t)&g_usADCBattVoltage;
	DMA_InitStructure.DMA_DIR 					= HAL_DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize 			= HAL_ADC_CNT;
	DMA_InitStructure.DMA_PeripheralInc 		= HAL_DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc 			= HAL_DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize	= HAL_DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize 		= HAL_DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode 					= HAL_DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority 				= HAL_DMA_Priority_Low;		//DMA_Priority_High
	DMA_InitStructure.DMA_FIFOMode 				= HAL_DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold 		= HAL_DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst 			= HAL_DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst 		= HAL_DMA_PeripheralBurst_Single;

    HalDrvDmaIOCtrl(eDMA_IO_Init, HAL_ADC2_DMA_STREAMx, (char*)&DMA_InitStructure, sizeof(DMA_InitStructure), HAL_ENABLE);
    HalDrvDmaIOCtrl(eDMA_IO_MuxEnable, (int)HAL_ADC2_DMA, NULL, 0, HAL_ENABLE);
    HalDrvDmaIOCtrl(eDMA_IO_MuxInit, (int)HAL_ADC2, NULL, HAL_ADC2_DMA_STREAMx, HAL_DMA_RX_MODE);

    HalDrvDmaIOCtrl(eDMA_IO_ChannelEnable, HAL_ADC2_DMA_STREAMx, NULL, 0, HAL_ENABLE);

	/* ADC Common Init **********************************************************/
	ADC_CommonInitStructure.ADC_Mode 			= HAL_ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler 		= HAL_ADC_Prescaler_Div2;
	ADC_CommonInitStructure.ADC_DMAAccessMode 	= HAL_ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = HAL_ADC_TwoSamplingDelay_20Cycles/*ADC_TwoSamplingDelay_5Cycles*/;
    HalDrvAdcIOCtrl(eADC_IO_CommonInit, 0, (char*)&ADC_CommonInitStructure, sizeof(ADC_CommonInitStructure), 0);

	/* ADC2 Init ****************************************************************/
	ADC_InitStructure.ADC_Resolution = HAL_ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = HAL_ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = HAL_ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = HAL_ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_ExternalTrigConv = HAL_ADC_ExternalTrigConv_T1_CC1;
	ADC_InitStructure.ADC_DataAlign = HAL_ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 1;
    HalDrvAdcIOCtrl(eADC_IO_Init, (int)HAL_ADC2, (char*)&ADC_InitStructure, sizeof(ADC_InitStructure), 0);

	/* ADC2 regular channel2 configuration **************************************/
    HalDrvAdcIOCtrl(eADC_IO_ChConfig, (int)HAL_ADC2, NULL, HAL_ADC_Channel_2, HAL_ADC_SampleTime_480Cycles);

	/* Enable DMA request after last transfer (Single-ADC mode) */
    HalDrvAdcIOCtrl(eADC_IO_DMARequestRepeatEnable, (int)HAL_ADC2, NULL, 0, HAL_ENABLE);

    /* config voltage_monitoring */
    HalDrvAdcIOCtrl(eADC_IO_Voltage_Config, (int)HAL_ADC2, NULL, 0, HAL_ADC_Channel_2);

	/* Enable ADC2 DMA */
    HalDrvAdcIOCtrl(eADC_IO_DMAEnable, (int)HAL_ADC2, NULL, 0, HAL_ENABLE);

	/* Enable ADC2 */
    HalDrvAdcIOCtrl(eADC_IO_PortEnable, (int)HAL_ADC2, NULL, 0, HAL_ENABLE);

    /* adc calibration */
    HalDrvAdcIOCtrl(eADC_IO_Calbration_Config, (int)HAL_ADC2, NULL, 0, HAL_ENABLE);
    
    HalDrvAdcIOCtrl(eADC_IO_SoftwareStartConv, (int)HAL_ADC2, NULL, 0, HAL_ENABLE);// Start ADC1 conversion
}
#endif			//#if defined(ADC_DMA)


void HalADC_SetAdc_Chnnel(unsigned int unAdcAddr, unsigned char ucSequence)
{
#if defined(STM32F427X)
    unsigned char ucAdcChannel;
	if(unAdcAddr == (unsigned int)HAL_ADC1)     ucAdcChannel = ADC_Channel_1;
    else                                        ucAdcChannel = ADC_Channel_2;

    ADC_InjectedChannelConfig((ADC_TypeDef*)unAdcAddr, (uint8_t)ucAdcChannel, 1, HAL_ADC_SampleTime_480Cycles);
    ADC_InjectedSequencerLengthConfig((ADC_TypeDef*)unAdcAddr, ucSequence);
#elif defined(AT32F435VMT7)
/*
    unsigned char ucAdcChannel;
    adc_ordinary_trig_select_type adc_ordinary_trig;

    if(unAdcAddr == (unsigned int)HAL_ADC1)
    {
        ucAdcChannel = ADC_CHANNEL_1;
        adc_ordinary_trig = ADC_ORDINARY_TRIG_TMR1CH1;
    }
    else   
    {
        ucAdcChannel = ADC_CHANNEL_2;
        adc_ordinary_trig = ADC_ORDINARY_TRIG_TMR1CH2;
    }

//   config ordinary trigger source and trigger edge
    adc_ordinary_conversion_trigger_set((adc_type*)unAdcAddr, 
                                        adc_ordinary_trig, 
                                        ADC_ORDINARY_TRIG_EDGE_NONE);

*/
#endif
}

//------------------------------------------------------------------------------
//  aram  GPIOx: where x can be (A..I) to select the GPIO peripheral.
//  @param  *buf : adc store buf
//
//  @retval  0: busy 1: complete
//
//------------------------------------------------------------------------------

u8 HalADC_Read2(u16 *buf)
{
#if defined(ADC_POLLING)
    static u8 step = 0;
	u16 usTemp;

    switch(step)
    {
    case 0: 
        HalDrvAdcIOCtrl(eADC_IO_ClearFlagStatus, (int)HAL_ADC2, NULL, 0, HAL_ADC_FLAG_JEOC);
        HalDrvAdcIOCtrl(eADC_IO_SoftwareStartInjectedConv, (int)HAL_ADC2, NULL, 0, 0);
        step++;
		break;
    case 1:
        
        usTemp = HalDrvAdcIOCtrl(eADC_IO_GetFlagStatus, (int)HAL_ADC2, NULL, 0, HAL_ADC_FLAG_JEOC);
        if ( usTemp)
        {
            HalDrvAdcIOCtrl(eADC_IO_ClearFlagStatus, (int)HAL_ADC2, NULL, 0, HAL_ADC_FLAG_JEOC);
            usTemp = HalDrvAdcIOCtrl(eADC_IO_GetSoftwareInjectedConvValue, (int)HAL_ADC2, NULL, 0, HAL_ADC_InjectedChannel_1);
            step = 0;
            *buf = (u16)(usTemp - ((usTemp-1000)*0.1)); 
            return 1;
        }
        break;
    }
    return 0;
#elif defined(ADC_DMA)
	U32 usAdcSum=0;
	float f;

	//*buf =  (u16)(g_usADCBattVoltage - ((g_usADCBattVoltage-1000)*0.1)); /* (*10);*/
	for(U8 i=0; i<HAL_ADC_CNT; i++)
	{
		f = (float)g_usADCBattVoltage[i];
		f = (f - 1000) * 0.1;
		usAdcSum += (u16)(g_usADCBattVoltage[i] - (u16)f); /* (*10);*/

#if defined(AT32F435VMT7)
		usAdcSum -= 45; 
#endif
		*buf = usAdcSum /(i+1);	
    }
	return 1;
#endif
}
u8 HalADC_Read1(u16 *buf)
{
#if defined(ADC_POLLING)
	static u8 step = 0;
	u16 usTemp;

	switch(step)
    {
	case 0:
        HalDrvAdcIOCtrl(eADC_IO_ClearFlagStatus, (int)HAL_ADC1, NULL, 0, HAL_ADC_FLAG_JEOC);
        HalDrvAdcIOCtrl(eADC_IO_SoftwareStartInjectedConv, (int)HAL_ADC1, NULL, 0, 0);
		step++;
		break;

	case 1:
        usTemp = HalDrvAdcIOCtrl(eADC_IO_GetFlagStatus, (int)HAL_ADC1, NULL, 0, HAL_ADC_FLAG_JEOC);
		if (usTemp)
        {
            HalDrvAdcIOCtrl(eADC_IO_ClearFlagStatus, (int)HAL_ADC1, NULL, 0, HAL_ADC_FLAG_JEOC);
            usTemp = HalDrvAdcIOCtrl(eADC_IO_GetSoftwareInjectedConvValue, (int)HAL_ADC1, NULL, 0, HAL_ADC_InjectedChannel_1);
			step = 0;
			*buf = (u16)(usTemp - ((usTemp-1000)*0.1)); /* (*10);*/

			return 1;
		}
	}

	return 0;
#elif defined(ADC_DMA)
	U32 usAdcSum=0;
	float f;

	//*buf =  (u16)(g_usADCBattVoltage - ((g_usADCBattVoltage-1000)*0.1)); /* (*10);*/
	for(U8 i = 0; i < HAL_ADC_CNT; i++)
	{
		f = ((g_usADCBattVoltage[i] - 1000) * 0.1);
		usAdcSum += (u16)(g_usADCBattVoltage[i] - (U32)f);

		*buf = usAdcSum /(i+1);
	}

	return 1;
#endif
}

void HalDrvAdcConvertCommonType(stHalADC_CommonInitTypeDef* pADC_CommonInitType, void* pAdc_common_struct)
{
#if defined(STM32F427X)
    ADC_CommonInitTypeDef* pstADCCommonInit = (ADC_CommonInitTypeDef*)pAdc_common_struct;

    pstADCCommonInit->ADC_Mode              = pADC_CommonInitType->ADC_Mode;
    pstADCCommonInit->ADC_Prescaler         = pADC_CommonInitType->ADC_Prescaler;
    pstADCCommonInit->ADC_DMAAccessMode     = pADC_CommonInitType->ADC_DMAAccessMode;
    pstADCCommonInit->ADC_TwoSamplingDelay = pADC_CommonInitType->ADC_TwoSamplingDelay;
#elif defined(AT32F435VMT7)
    adc_common_config_type* pstADCCommonInit = (adc_common_config_type*)pAdc_common_struct;

    pstADCCommonInit->combine_mode = (adc_combine_mode_type)pADC_CommonInitType->ADC_Mode;

    if ( pADC_CommonInitType->ADC_Prescaler == HAL_ADC_Prescaler_Div2 )
        pstADCCommonInit->div = ADC_HCLK_DIV_2;
    else if ( pADC_CommonInitType->ADC_Prescaler == HAL_ADC_Prescaler_Div4 )
        pstADCCommonInit->div = ADC_HCLK_DIV_4;
    else if ( pADC_CommonInitType->ADC_Prescaler == HAL_ADC_Prescaler_Div6 )
        pstADCCommonInit->div = ADC_HCLK_DIV_6;
    else if ( pADC_CommonInitType->ADC_Prescaler == HAL_ADC_Prescaler_Div8 )
        pstADCCommonInit->div = ADC_HCLK_DIV_8;
    
    if ( pADC_CommonInitType->ADC_DMAAccessMode == HAL_ADC_DMAAccessMode_Disabled )
        pstADCCommonInit->common_dma_mode = ADC_COMMON_DMAMODE_DISABLE;
    else if ( pADC_CommonInitType->ADC_DMAAccessMode == HAL_ADC_DMAAccessMode_1 )
        pstADCCommonInit->common_dma_mode = ADC_COMMON_DMAMODE_1;
    else if ( pADC_CommonInitType->ADC_DMAAccessMode == HAL_ADC_DMAAccessMode_2 )
        pstADCCommonInit->common_dma_mode = ADC_COMMON_DMAMODE_2;
    else if ( pADC_CommonInitType->ADC_DMAAccessMode == HAL_ADC_DMAAccessMode_3 )
        pstADCCommonInit->common_dma_mode = ADC_COMMON_DMAMODE_3;

    if ( pADC_CommonInitType->ADC_TwoSamplingDelay == HAL_ADC_TwoSamplingDelay_5Cycles )
        pstADCCommonInit->sampling_interval = ADC_SAMPLING_INTERVAL_5CYCLES;
    else if ( pADC_CommonInitType->ADC_TwoSamplingDelay == HAL_ADC_TwoSamplingDelay_6Cycles )
        pstADCCommonInit->sampling_interval = ADC_SAMPLING_INTERVAL_6CYCLES;
    else if ( pADC_CommonInitType->ADC_TwoSamplingDelay == HAL_ADC_TwoSamplingDelay_7Cycles )
        pstADCCommonInit->sampling_interval = ADC_SAMPLING_INTERVAL_7CYCLES;
    else if ( pADC_CommonInitType->ADC_TwoSamplingDelay == HAL_ADC_TwoSamplingDelay_8Cycles )
        pstADCCommonInit->sampling_interval = ADC_SAMPLING_INTERVAL_8CYCLES;
    else if ( pADC_CommonInitType->ADC_TwoSamplingDelay == HAL_ADC_TwoSamplingDelay_9Cycles )
        pstADCCommonInit->sampling_interval = ADC_SAMPLING_INTERVAL_9CYCLES;
    else if ( pADC_CommonInitType->ADC_TwoSamplingDelay == HAL_ADC_TwoSamplingDelay_10Cycles )
        pstADCCommonInit->sampling_interval = ADC_SAMPLING_INTERVAL_10CYCLES;
    else if ( pADC_CommonInitType->ADC_TwoSamplingDelay == HAL_ADC_TwoSamplingDelay_11Cycles )
        pstADCCommonInit->sampling_interval = ADC_SAMPLING_INTERVAL_11CYCLES;
    else if ( pADC_CommonInitType->ADC_TwoSamplingDelay == HAL_ADC_TwoSamplingDelay_12Cycles )
        pstADCCommonInit->sampling_interval = ADC_SAMPLING_INTERVAL_12CYCLES;
    else if ( pADC_CommonInitType->ADC_TwoSamplingDelay == HAL_ADC_TwoSamplingDelay_13Cycles )
        pstADCCommonInit->sampling_interval = ADC_SAMPLING_INTERVAL_13CYCLES;
    else if ( pADC_CommonInitType->ADC_TwoSamplingDelay == HAL_ADC_TwoSamplingDelay_14Cycles )
        pstADCCommonInit->sampling_interval = ADC_SAMPLING_INTERVAL_14CYCLES;
    else if ( pADC_CommonInitType->ADC_TwoSamplingDelay == HAL_ADC_TwoSamplingDelay_15Cycles )
        pstADCCommonInit->sampling_interval = ADC_SAMPLING_INTERVAL_15CYCLES;
    else if ( pADC_CommonInitType->ADC_TwoSamplingDelay == HAL_ADC_TwoSamplingDelay_16Cycles )
        pstADCCommonInit->sampling_interval = ADC_SAMPLING_INTERVAL_16CYCLES;
    else if ( pADC_CommonInitType->ADC_TwoSamplingDelay == HAL_ADC_TwoSamplingDelay_17Cycles )
        pstADCCommonInit->sampling_interval = ADC_SAMPLING_INTERVAL_17CYCLES;
    else if ( pADC_CommonInitType->ADC_TwoSamplingDelay == HAL_ADC_TwoSamplingDelay_18Cycles )
        pstADCCommonInit->sampling_interval = ADC_SAMPLING_INTERVAL_18CYCLES;
    else if ( pADC_CommonInitType->ADC_TwoSamplingDelay == HAL_ADC_TwoSamplingDelay_19Cycles )
        pstADCCommonInit->sampling_interval = ADC_SAMPLING_INTERVAL_19CYCLES;
    else if ( pADC_CommonInitType->ADC_TwoSamplingDelay == HAL_ADC_TwoSamplingDelay_20Cycles )
        pstADCCommonInit->sampling_interval = ADC_SAMPLING_INTERVAL_20CYCLES;

    pstADCCommonInit->common_dma_request_repeat_state = FALSE;
    pstADCCommonInit->tempervintrv_state = FALSE;
    pstADCCommonInit->vbat_state = FALSE;
#endif
}


void HalDrvAdcConvertIntiType(stHalADC_InitTypeDef* pADC_InitType, void* pAdc_base_struct)
{
#if defined(STM32F427X)
    ADC_InitTypeDef*  pADCInitStru = (ADC_InitTypeDef*)pAdc_base_struct;
    // __packed 이슈로 인해 각각 할당 필요.
    pADCInitStru->ADC_Resolution            = pADC_InitType->ADC_Resolution;
    pADCInitStru->ADC_ScanConvMode          = (FunctionalState)pADC_InitType->ADC_ScanConvMode;
    pADCInitStru->ADC_ContinuousConvMode    = (FunctionalState)pADC_InitType->ADC_ContinuousConvMode;
    pADCInitStru->ADC_ExternalTrigConvEdge  = pADC_InitType->ADC_ExternalTrigConvEdge;
    pADCInitStru->ADC_ExternalTrigConv      = pADC_InitType->ADC_ExternalTrigConv;
    pADCInitStru->ADC_DataAlign             = pADC_InitType->ADC_DataAlign;
    pADCInitStru->ADC_NbrOfConversion       = pADC_InitType->ADC_NbrOfConversion;
#elif defined(AT32F435VMT7)
    adc_base_config_type*  pADCInitStru = (adc_base_config_type*)pAdc_base_struct;

    pADCInitStru->sequence_mode  = (confirm_state)pADC_InitType->ADC_ScanConvMode;
    pADCInitStru->repeat_mode    = (confirm_state)pADC_InitType->ADC_ContinuousConvMode;

    if ( pADC_InitType->ADC_DataAlign == HAL_ADC_DataAlign_Right )
        pADCInitStru->data_align  = ADC_RIGHT_ALIGNMENT;
    else if ( pADC_InitType->ADC_DataAlign == HAL_ADC_DataAlign_Left )
        pADCInitStru->data_align = ADC_LEFT_ALIGNMENT;
    
    pADCInitStru->ordinary_channel_length      = 1;
#endif
}

//-----------------------------------------------------------------------------------//
int HalDrvAdcOpen(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    HalADC_Init();
    return HAL_RETURN_SUCCESS;
}

int HalDrvAdcRead(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

int HalDrvAdcWrite(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

int HalDrvAdcIOCtrl(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    eHalADC_IOCtlMode eADC_IO_Mode = (eHalADC_IOCtlMode)nLparam;

    switch ( eADC_IO_Mode )
    {
        case eADC_IO_Init:
        {
            stHalADC_InitTypeDef* pADC_InitType = (stHalADC_InitTypeDef*)pBuffer;
#if defined(STM32F427X)
            ADC_InitTypeDef ADCInitStruct;
            HalDrvAdcConvertIntiType(pADC_InitType, &ADCInitStruct);

            ADC_Init((ADC_TypeDef*)nRparam, (ADC_InitTypeDef*)&ADCInitStruct);
#elif defined(AT32F435VMT7)
            adc_base_config_type adc_base_struct;
            adc_resolution_type adc_resolution;

            HalDrvAdcConvertIntiType(pADC_InitType, &adc_base_struct);
            adc_base_config((adc_type*)nRparam, &adc_base_struct);
            
            if      ( pADC_InitType->ADC_Resolution == HAL_ADC_Resolution_12b )
                adc_resolution = ADC_RESOLUTION_12B;
            else if ( pADC_InitType->ADC_Resolution == HAL_ADC_Resolution_10b )
                adc_resolution = ADC_RESOLUTION_10B;
            else if ( pADC_InitType->ADC_Resolution == HAL_ADC_Resolution_8b )
                adc_resolution = ADC_RESOLUTION_8B;
            else if ( pADC_InitType->ADC_Resolution == HAL_ADC_Resolution_6b )
                adc_resolution = ADC_RESOLUTION_6B;

            adc_resolution_set((adc_type*)nRparam, adc_resolution);
#endif
        }
            break;
        case eADC_IO_CommonInit:
        {
            stHalADC_CommonInitTypeDef* pADC_CommonInitType = (stHalADC_CommonInitTypeDef*)pBuffer;
#if defined(STM32F427X)
            ADC_CommonInitTypeDef ADCCommonInitStruct;
            HalDrvAdcConvertCommonType(pADC_CommonInitType, &ADCCommonInitStruct);
            ADC_CommonInit((ADC_CommonInitTypeDef*)pADC_CommonInitType);
#elif defined(AT32F435VMT7)
            adc_common_config_type adc_common_struct;
            adc_common_default_para_init(&adc_common_struct);

            HalDrvAdcConvertCommonType(pADC_CommonInitType, &adc_common_struct);
            adc_common_config(&adc_common_struct);
#endif
        }
            break;
        case eADC_IO_PortEnable:
#if defined(STM32F427X)
            ADC_Cmd((ADC_TypeDef*)nRparam, (FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
            adc_enable((adc_type*)nRparam, (confirm_state)nOverlap);
            while(adc_flag_get((adc_type*)nRparam, ADC_RDY_FLAG) == RESET);
#endif
            break;
        case eADC_IO_ChConfig:
        {
            unsigned char ucAdcCH = (unsigned char)nLength;
            unsigned char ucAdcCHSampling = (unsigned char)nOverlap;
#if defined(STM32F427X)
            ADC_RegularChannelConfig((ADC_TypeDef*)nRparam, ucAdcCH, 1, ucAdcCHSampling);
#elif defined(AT32F435VMT7)
            adc_sampletime_select_type adc_sampletime;

            if ( ucAdcCHSampling == HAL_ADC_SampleTime_3Cycles  ) adc_sampletime = ADC_SAMPLETIME_2_5;
            if ( ucAdcCHSampling == HAL_ADC_SampleTime_15Cycles ) adc_sampletime = ADC_SAMPLETIME_6_5;
            if ( ucAdcCHSampling == HAL_ADC_SampleTime_28Cycles ) adc_sampletime = ADC_SAMPLETIME_12_5;
            if ( ucAdcCHSampling == HAL_ADC_SampleTime_56Cycles ) adc_sampletime = ADC_SAMPLETIME_24_5;
            if ( ucAdcCHSampling == HAL_ADC_SampleTime_84Cycles ) adc_sampletime = ADC_SAMPLETIME_47_5;
            if ( ucAdcCHSampling == HAL_ADC_SampleTime_112Cycles) adc_sampletime = ADC_SAMPLETIME_92_5;
            if ( ucAdcCHSampling == HAL_ADC_SampleTime_144Cycles) adc_sampletime = ADC_SAMPLETIME_247_5;
            if ( ucAdcCHSampling == HAL_ADC_SampleTime_480Cycles) adc_sampletime = ADC_SAMPLETIME_640_5;

            adc_ordinary_channel_set((adc_type*)nRparam, 
                                    (adc_channel_select_type)ucAdcCH, 
                                    1, // secquence
                                    adc_sampletime);
            
            /* config ordinary trigger source and trigger edge */
            adc_ordinary_conversion_trigger_set((adc_type*)nRparam,
                                                ADC_ORDINARY_TRIG_TMR1CH1, 
                                                ADC_ORDINARY_TRIG_EDGE_NONE);
#endif
        }
            break;
        case eADC_IO_DeInit:
#if defined(STM32F427X)
            ADC_DeInit();
#elif defined(AT32F435VMT7)
            adc_reset();
#endif
            break;
        case eADC_IO_GetFlagStatus:
#if defined(STM32F427X)
            return ADC_GetFlagStatus((ADC_TypeDef*)nRparam, (unsigned char)nOverlap);
#elif defined(AT32F435VMT7)
            return adc_flag_get((adc_type*)nRparam, (unsigned char)nOverlap);
#endif
            break;
        case eADC_IO_ClearFlagStatus:
#if defined(STM32F427X)
            ADC_ClearFlag((ADC_TypeDef*)nRparam, (unsigned char)nOverlap);
/*
*            @arg ADC_FLAG_AWD((uint8_t)0x01): Analog watchdog flag
*            @arg ADC_FLAG_EOC((uint8_t)0x02): End of conversion flag
*            @arg ADC_FLAG_JEOC((uint8_t)0x04): End of injected group conversion flag
*            @arg ADC_FLAG_JSTRT((uint8_t)0x08): Start of injected group conversion flag
*            @arg ADC_FLAG_STRT((uint8_t)0x10): Start of regular group conversion flag
*            @arg ADC_FLAG_OVR((uint8_t)0x20): Overrun flag                          
*/
#elif defined(AT32F435VMT7)
#if 0
#define ADC_VMOR_FLAG   ((uint8_t)0x01) /*!< voltage monitoring out of range flag */
#define ADC_OCCE_FLAG   ((uint8_t)0x02) /*!< ordinary channels conversion end flag */
#define ADC_PCCE_FLAG   (uint8_t)0x04) /*!< preempt channels conversion end flag */
#define ADC_PCCS_FLAG     ((uint8_t)0x08) /*!< preempt channel conversion start flag */
#define ADC_OCCS_FLAG    ((uint8_t)0x10) /*!< ordinary channel conversion start flag */
#define ADC_OCCO_FLAG    ((uint8_t)0x20) /*!< ordinary channel conversion overflow flag */
#define ADC_RDY_FLAG       ((uint8_t)0x40) /*!< adc ready to conversion flag */
#endif
            adc_flag_clear((adc_type*)nRparam, (unsigned int)nOverlap);
#endif
            break;
        case eADC_IO_GetITFlagStatus:
        {
            unsigned int usADC_ITFlag = nOverlap;
#if defined(STM32F427X)
            return ADC_GetITStatus((ADC_TypeDef*)nRparam, (unsigned short)usADC_ITFlag);
/*
*            @arg ADC_IT_EOC((uint16_t)0x0205)  : End of conversion interrupt mask
*            @arg ADC_IT_AWD((uint16_t)0x0106) : Analog watchdog interrupt mask
*            @arg ADC_IT_JEOC((uint16_t)0x0407): End of injected conversion interrupt mask
*            @arg ADC_IT_OVR((uint16_t)0x201A) : Overrun interrupt mask                        
*/
#elif defined(AT32F435VMT7)
            if      ( usADC_ITFlag == HAL_ADC_IT_EOC ) usADC_ITFlag = ADC_OCCE_INT;
            else if ( usADC_ITFlag == HAL_ADC_IT_EOC ) usADC_ITFlag = ADC_VMOR_INT;
            else if ( usADC_ITFlag == HAL_ADC_IT_EOC ) usADC_ITFlag = ADC_PCCE_INT;
            else if ( usADC_ITFlag == HAL_ADC_IT_EOC ) usADC_ITFlag = ADC_OCCO_INT;
            
            return adc_flag_get((adc_type*)nRparam, (unsigned char)usADC_ITFlag);
#if 0
#define ADC_OCCE_INT  ((uint32_t)0x00000020) /*!< ordinary channels conversion end interrupt */
#define ADC_VMOR_INT  ((uint32_t)0x00000040) /*!< voltage monitoring out of range interrupt */
#define ADC_PCCE_INT  ((uint32_t)0x00000080) /*!< preempt channels conversion end interrupt */
#define ADC_OCCO_INT  ((uint32_t)0x04000000) /*!< ordinary channel conversion overflow interrupt */
#endif
#endif
        }
            break;
        case eADC_IO_ClearITStatus:
        {
            unsigned int unADC_ITFlag = nOverlap;
#if defined(STM32F427X)
            ADC_ClearITPendingBit((ADC_TypeDef*)nRparam, (unsigned short)unADC_ITFlag);
#elif defined(AT32F435VMT7)
            if      ( unADC_ITFlag == HAL_ADC_IT_EOC ) unADC_ITFlag = ADC_OCCE_INT;
            else if ( unADC_ITFlag == HAL_ADC_IT_EOC ) unADC_ITFlag = ADC_VMOR_INT;
            else if ( unADC_ITFlag == HAL_ADC_IT_EOC ) unADC_ITFlag = ADC_PCCE_INT;
            else if ( unADC_ITFlag == HAL_ADC_IT_EOC ) unADC_ITFlag = ADC_OCCO_INT;
          
            adc_flag_clear((adc_type*)nRparam, (unsigned int)unADC_ITFlag);
#endif
        }
            break;

        case eADC_IO_IntEnable:
        {
            unsigned int unADC_InterrptFlag = nLength;
#if defined(STM32F427X)
            ADC_ITConfig((ADC_TypeDef*)nRparam, unADC_InterrptFlag, (FunctionalState)nOverlap);
/*
*            @arg ADC_IT_EOC((uint16_t)0x0205): End of conversion interrupt mask
*            @arg ADC_IT_AWD((uint16_t)0x0106): Analog watchdog interrupt mask
*            @arg ADC_IT_JEOC((uint16_t)0x0407): End of injected conversion interrupt mask
*            @arg ADC_IT_OVR((uint16_t)0x201A): Overrun interrupt enable                       
*/
#elif defined(AT32F435VMT7)
            if      ( unADC_InterrptFlag == HAL_ADC_IT_EOC ) unADC_InterrptFlag = ADC_OCCE_INT;
            else if ( unADC_InterrptFlag == HAL_ADC_IT_AWD ) unADC_InterrptFlag = ADC_VMOR_INT; 
            else if ( unADC_InterrptFlag == HAL_ADC_IT_JEOC) unADC_InterrptFlag = ADC_PCCE_INT; 
            else if ( unADC_InterrptFlag == HAL_ADC_IT_OVR ) unADC_InterrptFlag = ADC_OCCO_INT; 

            adc_interrupt_enable((adc_type*)nRparam, unADC_InterrptFlag, (confirm_state)nOverlap);

#if 0
#define ADC_OCCE_INT                     ((uint32_t)0x00000020) /*!< ordinary channels conversion end interrupt */
#define ADC_VMOR_INT                     ((uint32_t)0x00000040) /*!< voltage monitoring out of range interrupt */
#define ADC_PCCE_INT                     ((uint32_t)0x00000080) /*!< preempt channels conversion end interrupt */
#define ADC_OCCO_INT                     ((uint32_t)0x04000000) /*!< ordinary channel conversion overflow interrupt */
#endif
#endif
        }
            break;
        case eADC_IO_DMAEnable:
#if defined(STM32F427X)
            ADC_DMACmd((ADC_TypeDef*)nRparam, (FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
            adc_dma_mode_enable((adc_type*)nRparam, (confirm_state)nOverlap);
#endif 
            break;
        case eADC_IO_DMARequestRepeatEnable:
#if defined(STM32F427X)
            ADC_DMARequestAfterLastTransferCmd((ADC_TypeDef*)nRparam, (FunctionalState)nOverlap);
#elif defined(AT32F435VMT7)
            adc_dma_request_repeat_enable((adc_type*)nRparam, (confirm_state)nOverlap);
#endif 
            break;
        case eADC_IO_Voltage_Config:
#if defined(STM32F427X)
#elif defined(AT32F435VMT7)
/*
            // 불필요 소스 : 아이지엠(권정일 이사)
            adc_voltage_monitor_threshold_value_set((adc_type*)nRparam, 0x100, 0x000);
            adc_voltage_monitor_single_channel_select((adc_type*)nRparam, nOverlap);
            adc_voltage_monitor_enable((adc_type*)nRparam, ADC_VMONITOR_SINGLE_ORDINARY);
*/
#endif
            break;
        case eADC_IO_Calbration_Config:
#if defined(STM32F427X)
#elif defined(AT32F435VMT7)
            /* adc calibration */
            adc_calibration_init((adc_type*)nRparam);
            while(adc_calibration_init_status_get((adc_type*)nRparam));
            adc_calibration_start((adc_type*)nRparam);
            while(adc_calibration_status_get((adc_type*)nRparam));
#endif
            break;
        case eADC_IO_SoftwareStartConv:
#if defined(STM32F427X)
            ADC_SoftwareStartConv((ADC_TypeDef*)nRparam);
#elif defined(AT32F435VMT7)
            adc_ordinary_software_trigger_enable((adc_type*)nRparam, (confirm_state)nOverlap);
#endif
            break;
        case eADC_IO_SoftwareStartInjectedConv:
#if defined(STM32F427X)
            ADC_SoftwareStartInjectedConv((ADC_TypeDef*)nRparam);
#elif defined(AT32F435VMT7)
//void adc_preempt_offset_value_set(adc_type *adc_x, adc_preempt_channel_type adc_preempt_channel, uint16_t adc_offset_value)
                        
#endif 
            break;
        case eADC_IO_GetSoftwareInjectedConvValue:
        {
            unsigned char ucADC_InjectedCh = (unsigned char)nOverlap;
#if defined(STM32F427X)
            return ADC_GetInjectedConversionValue((ADC_TypeDef*)nRparam, (unsigned char)ucADC_InjectedCh);
/*
*            @arg ADC_InjectedChannel_1 ((uint8_t)0x14): Injected Channel1 selected
*            @arg ADC_InjectedChannel_2((uint8_t)0x18): Injected Channel2 selected
*            @arg ADC_InjectedChannel_3((uint8_t)0x1C): Injected Channel3 selected
*            @arg ADC_InjectedChannel_4((uint8_t)0x20): Injected Channel4 selected
*/
#elif defined(AT32F435VMT7)
            if      ( ucADC_InjectedCh == HAL_ADC_InjectedChannel_1 ) ucADC_InjectedCh = ADC_PREEMPT_CHANNEL_1;
            else if ( ucADC_InjectedCh == HAL_ADC_InjectedChannel_2 ) ucADC_InjectedCh = ADC_PREEMPT_CHANNEL_2;
            else if ( ucADC_InjectedCh == HAL_ADC_InjectedChannel_3 ) ucADC_InjectedCh = ADC_PREEMPT_CHANNEL_3;
            else if ( ucADC_InjectedCh == HAL_ADC_InjectedChannel_4 ) ucADC_InjectedCh = ADC_PREEMPT_CHANNEL_4;

            return adc_preempt_conversion_data_get((adc_type*)nRparam, (adc_preempt_channel_type)ucADC_InjectedCh);
#if 0
            ADC_PREEMPT_CHANNEL_1                  = 0x00, /*!< adc preempt channel 1 */
            ADC_PREEMPT_CHANNEL_2                  = 0x01, /*!< adc preempt channel 2 */
            ADC_PREEMPT_CHANNEL_3                  = 0x02, /*!< adc preempt channel 3 */
            ADC_PREEMPT_CHANNEL_4                  = 0x03  /*!< adc preempt channel 4 */
#endif

#endif 
        }  
            break;

        default:
            break;
    }


    return HAL_RETURN_SUCCESS;
}

int HalDrvAdcClose(int nLparam, int nRparam, char* pBuffer, int nLength, int nOverlap)
{
    return HAL_RETURN_SUCCESS;
}

