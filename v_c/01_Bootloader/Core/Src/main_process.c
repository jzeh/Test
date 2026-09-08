/*************************************************************
 * NOTE : main_process.c
 *      Application Main Process
 * Author : Lee junho
 * Since : 2019.04.12
**************************************************************/
#include <string.h>

#include "firmware.h"
#include "flash_if.h"
#include "typedef.h"
#include "common.h"
#include "led_control.h"
#include "buzzer.h"
#include "sw_timer.h"
#include "git_mmc.h"
#include "git_ioctl.h"
#include "firmware.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
void goNormalBooting( void );
void goReprogramBooting( void );

void jumpToApplication( uint32_t address );

extern void MX_USB_DEVICE_Init(void);

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
pFunction	Jump_To_Application;
uint32_t	JumpAddress;

/*----------------------------------------------------------------------
 *   Functions definition
 *--------------------------------------------------------------------*/
void MainProcess( void )
{
    uint8_t             ucVerifyFwInfo_Result = 0;
    eResVerifiyApp      eVerifyApplication_Result = eAppOK;
    bool                bCheckRecoveryFirmware_Result = FALSE;
    
	GLogN( "\r\n\n" );
	GLogI( "==================================================\r\n" );
	GLogI( "   Start %s Bootloader(%s)...\r\n", MODEL_NAME, __DATE__ );
	GLogI( "==================================================\r\n" );

	//getBoardID();

	LED_RED_ON;

	Buzzer_Init();					// Buzzer initialize

	//Buzzer_Control( eBUZZER_ON, MSEC(100),MSEC(100), 1 );
	
	//HAL_Delay( 100 );
	initMMC();
	
    
    // Firmware Information Validation
    ucVerifyFwInfo_Result = verifyFirmwareInfo();
	
	// Print Firmware Information
	printFirmwareInfo();
	printFWVersion();

	if( IO_CONTROL_GET( PAIR_SW ) == GPIO_PIN_RESET )
	{
		EnableUSB3300();
		EnableLAN9514();
		MX_USB_DEVICE_Init();

		Buzzer_Control( eBUZZER_SIRESOL, MSEC(200),MSEC(200), 3 );

		LED_CYAN_ON;

		while(1)
		{
		}
	}
    
    // Verify Applcation
    eVerifyApplication_Result = verifyApplication();
    
    // Checking if recovery firmware is saved.
    bCheckRecoveryFirmware_Result = checkRecoveryFirmware();
    
    // Firmware Info or Files are abnormal. (exclude eBootRecovery to avoid infinite recovery loop)
    if( (eVerifyApplication_Result != eAppOK  || ucVerifyFwInfo_Result != 0 )
        && (eVerifyApplication_Result != eBootRecovery)
        && (bCheckRecoveryFirmware_Result == TRUE) )
    {
        GLogN("Recover Firmware!!!\r\n");
        RecoverFirmware();
    }
    

	// Check Booting Mode
	switch( gsFwInfo.mucBootMode )
	{
		case eApp_Downloader		:
		case eApp_VCI_2				:
		case eApp_Inside			:
		case eApp_Inside2			:
		case eApp_Recovery			:
		case eApp_Selftest			:
		case eApp_VCI_II_PDI		:
		case eApp_ECUUpCAN			:
		case eApp_ECUUpKWP			:
		case eApp_ECUUpCV			:
		case eApp_ECUUpCCP			:
		case eApp_ECUUpFlexRay		:
		case eApp_ECUUpDownloader	:
		case eApp_ECUUpCVKWP		:
		//case eApp_ECUUP_COMMON	:
		//case eApp_ECUUP_STDA		:
			goNormalBooting();
			break;

		default				:
			goNormalBooting();
			break;
	}
}

void goNormalBooting( void )
{
	uint32_t	ret = 0;

	GLogI( "==================================================\r\n" );
	GLogI( "   Start Normal Booting..\r\n" );
	GLogI( "==================================================\r\n" );

	GLogN( "Check Main Application CS\r\n" );
	ret = checkCS( FIRMWARE_MAINAPP_ADD, &gsFwInfo.msAppInfo[gsFwInfo.mucBootMode] );					// main app check CS
	if( ret ) // Fail
	{
		gsFwInfo.mucCurrentMode		= eApp_VCI_2;
		checkApplication();
	}
	else
	{
	  	gsFwInfo.mucCurrentMode 	= gsFwInfo.mucBootMode;
	}
	
	gsFwInfo.mucChanged		= TRUE;
	saveFirmwareInfo_EMMC(false);
	
	// Jump Main Application
	jumpToApplication( FIRMWARE_MAINAPP_ADD );
}

#if (0)
static void goReprogramBooting( void )
{
	uint32_t ret = 0;

	GLogI( "==================================================\r\n" );
	GLogI( "   Start Reprogram Booting..\r\n" );
	GLogI( "==================================================\r\n" );

	GLogN( "Check Reprogram Application CS\r\n" );
	ret = checkCS( &gsFwInfo.msAppInfo[gsFwInfo.mucBootMode] );					// reprogram app check CS
	if( ret ) // Fail
	{
		goNormalBooting();
	}

	// Jump Verification Application
	jumpToApplication( gsFwInfo.msAppInfo[gsFwInfo.mucBootMode].mJumpAddress );
}
#endif

static void jumpToApplication( uint32_t address )
{
	//GLogN( "Jump Application Address..[0x%08x]\r\n", (*(__IO uint32_t*)address));
	//GLogN( "Jump Application Address..[0x%08x]\r\n", address);
	GLogN( "\r\nJump Application\r\n");

	SCB_DisableDCache();
	SCB_InvalidateDCache();

//	if( ((*(__IO uint32_t*)address) & 0x2FFE0000 ) == 0x20000000 )
	if( ((*(__IO uint32_t*)address) & 0x24f80000 ) == 0x24000000 )
	{
		__disable_irq();

		//SysTick->CTRL = 0;
		//SysTick->LOAD = 0;
		//SysTick->VAL  = 0;

		__set_MSP(*(__IO uint32_t*)address);

		SCB->VTOR = address;

		JumpAddress = *(__IO uint32_t*)(address + 4);
		Jump_To_Application = (pFunction)JumpAddress;

		GLogN( "\r\n\n" );

		Jump_To_Application();

		while(1);
	}

	Error_Handler();
}