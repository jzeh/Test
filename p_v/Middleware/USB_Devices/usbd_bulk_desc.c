/**
  ******************************************************************************
  * @file    usbd_desc.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   This file provides the USBD descriptors and string formating method.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
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
#include "usbd_desc.h"

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_DESC
  * @brief USBD descriptors module
  * @{
  */

/** @defgroup USBD_DESC_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_DESC_Private_Defines
  * @{
  */

/** @defgroup USB_String_Descriptors
  * @{
  */

#define USBD_BULK_VID                        0x5345//0x0483
#define USBD_BULK_PID                        0x1234//0x5740

#define USBD_BULK_LANGID_STRING              0x409
#define USBD_BULK_MANUFACTURER_STRING        "GIT AUTO"
#define USBD_BULK_PRODUCT_HS_STRING          "GIT HS  BULK"
#define USBD_BULK_SERIALNUMBER_HS_STRING     "00000000050B"
#define USBD_BULK_PRODUCT_FS_STRING          "GIT FS  BULK"
#define USBD_BULK_SERIALNUMBER_FS_STRING     "00000000050C"
#define USBD_BULK_CONFIGURATION_HS_STRING    "DCS Config"
#define USBD_BULK_INTERFACE_HS_STRING        "DCS Interface"
#define USBD_BULK_CONFIGURATION_FS_STRING    "DCS Config"
#define USBD_BULK_INTERFACE_FS_STRING        "DCS Interface"

/**
  * @}
  */


/** @defgroup USBD_DESC_Private_Macros
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_DESC_Private_Variables
  * @{
  */

USBD_DEVICE USR_BULK_desc =
{
  USBD_USR_BULK_DeviceDescriptor,
  USBD_USR_BULK_LangIDStrDescriptor,
  USBD_USR_BULK_ManufacturerStrDescriptor,
  USBD_USR_BULK_ProductStrDescriptor,
  USBD_USR_BULK_SerialStrDescriptor,
  USBD_USR_BULK_ConfigStrDescriptor,
  USBD_USR_BULK_InterfaceStrDescriptor,
};

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
/* USB Standard Device Descriptor */
__ALIGN_BEGIN uint8_t USBD_BULK_DeviceDesc[USB_SIZ_DEVICE_DESC] __ALIGN_END =
  {
    0x12,							/*bLength */
    USB_DEVICE_DESCRIPTOR_TYPE, 	/*bDescriptorType*/
    0x00,							/*bcdUSB */
    0x02,
    0xff,							/*bDeviceClass*/
    0x00,							/*bDeviceSubClass*/
    0x00,							/*bDeviceProtocol*/
    USB_OTG_MAX_EP0_SIZE,			/*bMaxPacketSize*/
    LOBYTE(USBD_BULK_VID),				/*idVendor*/
    HIBYTE(USBD_BULK_VID),				/*idVendor*/
    LOBYTE(USBD_BULK_PID),				/*idVendor*/
    HIBYTE(USBD_BULK_PID),				/*idVendor*/
    0x00,							/*bcdDevice rel. 2.00*/
    0x02,
    USBD_IDX_MFC_STR,				/*Index of manufacturer  string*/
    USBD_IDX_PRODUCT_STR,			/*Index of product string*/
    0x00,//USBD_IDX_SERIAL_STR,		/*Index of serial number string*/
    USBD_CFG_MAX_NUM				/*bNumConfigurations*/
  } ; /* USB_DeviceDescriptor */

//#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
//  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
//    #pragma data_alignment=4
//  #endif
//#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
/* USB Standard Device Descriptor */
//__ALIGN_BEGIN uint8_t USBD_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
//{
//  USB_LEN_DEV_QUALIFIER_DESC,
//  USB_DESC_TYPE_DEVICE_QUALIFIER,
//  0x00,
//  0x02,
//  0x00,
//  0x00,
//  0x00,
//  0x40,
//  0x01,
//  0x00,
//};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN uint8_t USBD_BULK_LangIDDesc[USB_SIZ_STRING_LANGID] __ALIGN_END =
{
     USB_SIZ_STRING_LANGID,
     USB_DESC_TYPE_STRING,
     LOBYTE(USBD_BULK_LANGID_STRING),
     HIBYTE(USBD_BULK_LANGID_STRING),
};
/**
  * @}
  */


/** @defgroup USBD_DESC_Private_FunctionPrototypes
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_DESC_Private_Functions
  * @{
  */

/**
* @brief  USBD_USR_BULK_DeviceDescriptor
*         return the device descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_BULK_DeviceDescriptor( uint8_t speed , uint16_t *length)
{
  *length = sizeof(USBD_BULK_DeviceDesc);
  return USBD_BULK_DeviceDesc;
}

/**
* @brief  USBD_USR_BULK_LangIDStrDescriptor
*         return the LangID string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_BULK_LangIDStrDescriptor( uint8_t speed , uint16_t *length)
{
  *length =  sizeof(USBD_LangIDDesc);
  return USBD_BULK_LangIDDesc;
}


/**
* @brief  USBD_USR_BULK_ProductStrDescriptor
*         return the product string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_BULK_ProductStrDescriptor( uint8_t speed , uint16_t *length)
{
  if(speed == 0)
  {
    USBD_GetString (USBD_BULK_PRODUCT_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString (USBD_BULK_PRODUCT_FS_STRING, USBD_StrDesc, length);
  }

  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_BULK_ManufacturerStrDescriptor
*         return the manufacturer string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_BULK_ManufacturerStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString (USBD_BULK_MANUFACTURER_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_BULK_SerialStrDescriptor
*         return the serial number string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_BULK_SerialStrDescriptor( uint8_t speed , uint16_t *length)
{
  if(speed  == USB_OTG_SPEED_HIGH)
  {
    USBD_GetString (USBD_BULK_SERIALNUMBER_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString (USBD_BULK_SERIALNUMBER_FS_STRING, USBD_StrDesc, length);
  }
  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_BULK_ConfigStrDescriptor
*         return the configuration string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_BULK_ConfigStrDescriptor( uint8_t speed , uint16_t *length)
{
  if(speed  == USB_OTG_SPEED_HIGH)
  {
    USBD_GetString (USBD_BULK_CONFIGURATION_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString (USBD_BULK_CONFIGURATION_FS_STRING, USBD_StrDesc, length);
  }
  return USBD_StrDesc;
}


/**
* @brief  USBD_USR_BULK_InterfaceStrDescriptor
*         return the interface string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_BULK_InterfaceStrDescriptor( uint8_t speed , uint16_t *length)
{
  if(speed == 0)
  {
    USBD_GetString (USBD_BULK_INTERFACE_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString (USBD_BULK_INTERFACE_FS_STRING, USBD_StrDesc, length);
  }
  return USBD_StrDesc;
}

/**
  * @}
  */


/**
  * @}
  */


/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

