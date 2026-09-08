/**
  ******************************************************************************
  * @file    main.c
  * @author  system Dev Team
  * @version V1.1.0
  * @date    08-April-2022
  * @brief   Main program body
  ******************************************************************************
**/

/* Includes ------------------------------------------------------------------*/
#if !defined(FEATURE_BOOTLOADER)
	#include "main.h"
	#include "Autolink_Manager.h"
	#include "app_cli.h"
#endif

#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
#include "usbd_cdc_vcp.h"
#include "usbd_usr.h"
#endif

#include "GIT_Util.h"
#include "HdDebug.h"
#include "GIT_OemInterface.h"
#include "GIT_BluetoothLowEnergy.h"

#include "HalHandler.h"


#define Trace(...)  //GITDebug(DEBUG_MODULES_INIT,__VA_ARGS__)

#if defined(FEATURE_BOOTLOADER)
void BootloaderProc(void)
{
	uint32_t flashdestination;
	stHalNVIC_InitTypeDef   NVIC_InitStructure;

	//********************************************************************
	// bootloader 사용 시, UART interrupt는 사용하지 말것.
	//********************************************************************
   
	printf("\n\n\n\n");
	printf("*********************************\r\n");
	printf("*       Start Bootloader        *\r\n");
	printf("*********************************\r\n\r\n");

	printf("[AutolinkP Bootloader start: built - %s %s]\r\n\r\n", __DATE__, __TIME__);

	InitFirmwareInfo();

	flashdestination = Appl_Mode_Switch();	// FlashRom에서 ModeSwitch.ini 정보를 읽어 와서 모드 전환 한다.

	printf("\r\n\r\n");
	printf("**********************************************\r\n");
	printf("*       Leave Bootloader                     *\r\n");
	printf("*       Go to Application(0x%x)              *\r\n", flashdestination);
	printf("**********************************************\r\n");

    while(HalUartGetFlagStatus((int)g_stUart7.pUARTreg, HAL_USART_FLAG_TC)== HAL_RESET);
    NVIC_InitStructure.NVIC_IRQChannel = HAL_UART7_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = HAL_Group_0_NVIC_IRQChannelPreemptionPriority;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = HAL_UART_DEBUG_IRQChannelSubPriority;
    NVIC_InitStructure.NVIC_IRQChannelEnable = HAL_DISABLE;
    HalDrvMiscIOCtrl(eMISC_IO_RegIRQ, 0, (char*)&NVIC_InitStructure, sizeof(stHalNVIC_InitTypeDef), 0);

	if(((*(__IO uint32_t*)flashdestination) & 0x2FF00000) == 0x20000000) {
		JumpToApp(flashdestination);
	}
	else {
		printf("\r\nNot Install Application!!!\r\n");
		printf("Bootloader Halt...\n\r");

		while(1);
	}

	return;
}
#endif

int main(void)
{
    HalHandlerInit();

#if defined(FEATURE_BOOTLOADER)
	BootloaderProc();
#else
    GITDebugPrintf("[----- DCS Premium : built %s %s------]\r\n", __DATE__, __TIME__);

#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
    InitUSBDeviceClassType();
#endif
    
    BT_Initialize();

    InitFileSystem();

    LoadSettingInfoFile(0);

#ifdef USE_ALPU_CHIPSET
	AlpuCheck();
#endif
	AutoLink_MainProc();
#endif
}



#ifdef  USE_FULL_ASSERT
/**
* @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: Trace("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
