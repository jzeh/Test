/**
  ******************************************************************************
  * @file    usbd_usr.c
  * @author  MCD Application Team
  * @version V1.2.0
  * @date    09-November-2015
  * @brief   This file includes the user application layer
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usbd_usr.h"
#include "GIT_InterProtocol.h"
#include <stdio.h>

#include "HdDebug.h"

#define Trace(...)  GITDebug(DEBUG_MODULES_USB,__VA_ARGS__)


__ALIGN_BEGIN USB_OTG_CORE_HANDLE    USB_OTG_dev 				__ALIGN_END;

uint8_t gbUSBCDCConnected = false;

/** @addtogroup USBD_USER
  * @{
  */

/** @addtogroup USBD_MSC_DEMO_USER_CALLBACKS
  * @{
  */

/** @defgroup USBD_USR
  * @brief    This file includes the user application layer
  * @{
  */

/** @defgroup USBD_USR_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_USR_Private_Defines
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_USR_Private_Macros
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_USR_Private_Variables
  * @{
  */
/*  Points to the DEVICE_PROP structure of current device */
/*  The purpose of this register is to speed up the execution */

USBD_Usr_cb_TypeDef USR_bulk_cb =
{
	USBD_USR_Bulk_Init,
	USBD_USR_Bulk_DeviceReset,
	USBD_USR_Bulk_DeviceConfigured,
	USBD_USR_Bulk_DeviceSuspended,
	USBD_USR_Bulk_DeviceResumed,
	USBD_USR_Bulk_DeviceConnected,
	USBD_USR_Bulk_DeviceDisconnected,
};

USBD_Usr_cb_TypeDef USR_msc_cb =
{
  USBD_USR_Msc_Init,
  USBD_USR_Msc_DeviceReset,
  USBD_USR_Msc_DeviceConfigured,
  USBD_USR_Msc_DeviceSuspended,
  USBD_USR_Msc_DeviceResumed,
  USBD_USR_Msc_DeviceConnected,
  USBD_USR_Msc_DeviceDisconnected,
};

USBD_Usr_cb_TypeDef USR_vcp_cb =
{
  USBD_USR_Vcp_Init,
  USBD_USR_Vcp_DeviceReset,
  USBD_USR_Vcp_DeviceConfigured,
  USBD_USR_Vcp_DeviceSuspended,
  USBD_USR_Vcp_DeviceResumed,
  USBD_USR_Vcp_DeviceConnected,
  USBD_USR_Vcp_DeviceDisconnected,
};

/**
  * @}
  */

/** @defgroup USBD_USR_Private_Constants
  * @{
  */

/**
  * @}
  */



/** @defgroup USBD_USR_Private_FunctionPrototypes
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_USR_Private_Functions
  * @{
  */

/*****************************************************/
/************************ VCP ************************/
/*****************************************************/

/**
* @brief  USBD_USR_Init
*         Displays the message on LCD for host lib initialization
* @param  None
* @retval None
*/
void USBD_USR_Vcp_Init(void)
{
  Trace(" USB OTG FS VCP Device\n");
  Trace("> USB device library started.\n");
  Trace("     USB Device Library v1.2.0\n" );
}

/**
* @brief  USBD_USR_DeviceReset
*         Displays the message on LCD on device Reset Event
* @param  speed : device speed
* @retval None
*/
void USBD_USR_Vcp_DeviceReset(uint8_t speed )
{
	switch (speed)
	{
		case USB_OTG_SPEED_HIGH:
			Trace ("     USB Device Library v1.2.0 [HS]\n" );
			break;

		case USB_OTG_SPEED_FULL:
			Trace ("     USB Device Library v1.2.0 [FS]\n" );
			break;

		default:
			Trace ("     USB Device Library v1.2.0 [??]\n" );
			break;
	}
}


/**
* @brief  USBD_USR_DeviceConfigured
*         Displays the message on LCD on device configuration Event
* @param  None
* @retval Status
*/
void USBD_USR_Vcp_DeviceConfigured (void)
{
  Trace("> VCP Interface configured.\n");
  gbUSBCDCConnected = true;
}

/**
* @brief  USBD_USR_DeviceSuspended
*         Displays the message on LCD on device suspend Event
* @param  None
* @retval None
*/
void USBD_USR_Vcp_DeviceSuspended(void)
{
	gbUSBCDCConnected = false;

//	Trace("> USB Device in Suspend Mode.\n");
  /* Users can do their application actions here for the USB-Reset */
}


/**
* @brief  USBD_USR_DeviceResumed
*         Displays the message on LCD on device resume Event
* @param  None
* @retval None
*/
void USBD_USR_Vcp_DeviceResumed(void)
{
	gbUSBCDCConnected = true;
	Trace("> USB Device in Idle Mode.\n");
  /* Users can do their application actions here for the USB-Reset */
}


/**
* @brief  USBD_USR_DeviceConnected
*         Displays the message on LCD on device connection Event
* @param  None
* @retval Status
*/
void USBD_USR_Vcp_DeviceConnected (void)
{
  Trace("> USB Device Connected - VCP\n");
  gbUSBCDCConnected = true;
}


/**
* @brief  USBD_USR_DeviceDisonnected
*         Displays the message on LCD on device disconnection Event
* @param  None
* @retval Status
*/
void USBD_USR_Vcp_DeviceDisconnected (void)
{
	gbUSBCDCConnected = false;
	Trace("> USB Device Disconnected - VCP\n");
}
/**
* @}
*/

eCommState GetUSBCommState()
{
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
	return g_stGitCommInfo[eCOMM_TYPE_USB].eCommSate;
#else
    return -1;
#endif
}

void SetUSBCommState(eCommState eUsbState)
{
#if defined(FEATURE_USE_USB_DRIVE) // 2022/02/25 Added by James Jean
	g_stGitCommInfo[eCOMM_TYPE_USB].eCommSate = eUsbState;
#endif
}



/*****************************************************/
/************************ BULK ************************/
/*****************************************************/

/**
* @brief  USBD_USR_Bulk_Init
*         Displays the message on LCD for host lib initialization
* @param  None
* @retval None
*/
void USBD_USR_Bulk_Init(void)
{
#ifdef USE_USB_OTG_HS
	Trace("USB OTG High Speed BULK Device\r\n");
#else
	Trace("USB OTG Full Speed BULK Device\r\n");
#endif

	Trace("> USB device library started.\n");

	SetUSBCommState(eCOMM_STATE_INIT);
}

/**
* @brief  USBD_USR_Bulk_DeviceReset
*         Displays the message on LCD on device Reset Event
* @param  speed : device speed
* @retval None
*/
void USBD_USR_Bulk_DeviceReset(uint8_t speed )
{
	switch (speed)
	{
	case USB_OTG_SPEED_HIGH:
		Trace("USB Device [High Speed]\r\n");
		break;

	case USB_OTG_SPEED_FULL:
		Trace("USB Device [Full Speed]\r\n");

		break;
	default:
		Trace("USB Device [??]\r\n");
		break;
	}

	SetUSBCommState(eCOMM_STATE_DISCONNECTED);
}


/**
* @brief  USBD_USR_Bulk_DeviceConfigured
*         Displays the message on LCD on device configuration Event
* @param  None
* @retval Staus
*/
void USBD_USR_Bulk_DeviceConfigured (void)
{
	Trace("> BULK Interface configured.\r\n");

	SetUSBCommState(eCOMM_STATE_CONNECTED);
}

/**
* @brief  USBD_USR_Bulk_DeviceSuspended
*         Displays the message on LCD on device suspend Event
* @param  None
* @retval None
*/
void USBD_USR_Bulk_DeviceSuspended(void)
{
//	Trace("> USB Device in Suspend Mode.\r\n");
}


/**
* @brief  USBD_USR_Bulk_DeviceResumed
*         Displays the message on LCD on device resume Event
* @param  None
* @retval None
*/
void USBD_USR_Bulk_DeviceResumed(void)
{
	Trace("> USB Device in Idle Mode.\r\n");
}


/**
* @brief  USBD_USR_Bulk_DeviceConnected
*         Displays the message on LCD on device connection Event
* @param  None
* @retval Staus
*/
void USBD_USR_Bulk_DeviceConnected (void)
{
	Trace("> USB Device Connected.\r\n");
	SetUSBCommState(eCOMM_STATE_CONNECTING);
}


/**
* @brief  USBD_USR_DeviceDisonnected
*         Displays the message on LCD on device disconnection Event
* @param  None
* @retval Staus
*/
void USBD_USR_Bulk_DeviceDisconnected (void)
{
	Trace("> USB Device Disconnected.\r\n");

	SetUSBCommState(eCOMM_STATE_DISCONNECTING);
}
/**
* @}
*/

/*****************************************************/
/************************ MSC ************************/
/*****************************************************/

/** @defgroup USBD_USR_Private_Functions
  * @{
  */

/**
* @brief  Displays the message on LCD on device lib initialization
* @param  None
* @retval None
*/
void USBD_USR_Msc_Init(void)
{
#ifdef USE_USB_OTG_HS
  Trace(" USB OTG HS MSC Device\n");
#else
  Trace(" USB OTG FS MSC Device\n");
#endif
  Trace("> USB device library started.\n");
  Trace ("     USB Device Library v1.2.0\n" );
}

/**
* @brief  Displays the message on LCD on device reset event
* @param  speed : device speed
* @retval None
*/
void USBD_USR_Msc_DeviceReset (uint8_t speed)
{
 switch (speed)
 {
   case USB_OTG_SPEED_HIGH:
     Trace ("     USB Device Library v1.2.0  [HS]\n" );
     break;

  case USB_OTG_SPEED_FULL:
     Trace ("     USB Device Library v1.2.0  [FS]\n" );
     break;
 default:
     Trace ("     USB Device Library v1.2.0  [??]\n" );

 }
}


/**
* @brief  Displays the message on LCD on device config event
* @param  None
* @retval Status
*/
void USBD_USR_Msc_DeviceConfigured (void)
{
  Trace("> MSC Interface started.\n");

}
/**
* @brief  Displays the message on LCD on device suspend event
* @param  None
* @retval None
*/
void USBD_USR_Msc_DeviceSuspended(void)
{
    Trace("> Device In suspend mode.\n");
}


/**
* @brief  Displays the message on LCD on device resume event
* @param  None
* @retval None
*/
void USBD_USR_Msc_DeviceResumed(void)
{

}

/**
* @brief  USBD_USR_DeviceConnected
*         Displays the message on LCD on device connection Event
* @param  None
* @retval Status
*/
void USBD_USR_Msc_DeviceConnected (void)
{
  Trace("> USB Device Connected.\n");
}


/**
* @brief  USBD_USR_DeviceDisonnected
*         Displays the message on LCD on device disconnection Event
* @param  None
* @retval Status
*/
void USBD_USR_Msc_DeviceDisconnected (void)
{
  Trace("> USB Device Disconnected.\n");
}


/**
* @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
