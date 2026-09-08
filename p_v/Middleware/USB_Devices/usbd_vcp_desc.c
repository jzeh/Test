/**
  ******************************************************************************
  * @file    usbd_desc.c
  * @author  MCD Application Team
  * @version V1.2.0
  * @date    09-November-2015
  * @brief   This file provides the USBD descriptors and string formating method.
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
#include "usbd_vcp_desc.h"

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

#define USBD_VCP_VID                   		0x0483
#define USBD_VCP_PID                   		0x5740

#define USBD_VCP_LANGID_STRING         		0x409
#define USBD_VCP_MANUFACTURER_STRING        	"STMicroelectronics"
#define USBD_VCP_PRODUCT_HS_STRING          	"STM32 Virtual ComPort in HS mode"
#define USBD_VCP_PRODUCT_FS_STRING          	"STM32 Virtual ComPort in FS Mode"
#define USBD_VCP_CONFIGURATION_HS_STRING    	"VCP Config"
#define USBD_VCP_INTERFACE_HS_STRING        	"VCP Interface"
#define USBD_VCP_CONFIGURATION_FS_STRING    	"VCP Config"
#define USBD_VCP_INTERFACE_FS_STRING        	"VCP Interface"
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

USBD_DEVICE USR_VCP_desc =
{
  USBD_USR_VCP_DeviceDescriptor,
  USBD_USR_VCP_LangIDStrDescriptor,
  USBD_USR_VCP_ManufacturerStrDescriptor,
  USBD_USR_VCP_ProductStrDescriptor,
  USBD_USR_VCP_SerialStrDescriptor,
  USBD_USR_VCP_ConfigStrDescriptor,
  USBD_USR_VCP_InterfaceStrDescriptor,
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN uint8_t USBD_VCP_DeviceDesc[USB_SIZ_DEVICE_DESC] __ALIGN_END =
{
  0x12,                       /*bLength */
  USB_DEVICE_DESCRIPTOR_TYPE, /*bDescriptorType*/
  0x00,                       /*bcdUSB */
  0x02,
  0x00,                       /*bDeviceClass*/
  0x00,                       /*bDeviceSubClass*/
  0x00,                       /*bDeviceProtocol*/
  USB_OTG_MAX_EP0_SIZE,      /*bMaxPacketSize*/
  LOBYTE(USBD_VCP_VID),           /*idVendor*/
  HIBYTE(USBD_VCP_VID),           /*idVendor*/
  LOBYTE(USBD_VCP_PID),           /*idVendor*/
  HIBYTE(USBD_VCP_PID),           /*idVendor*/
  0x00,                       /*bcdDevice rel. 2.00*/
  0x02,
  USBD_IDX_MFC_STR,           /*Index of manufacturer  string*/
  USBD_IDX_PRODUCT_STR,       /*Index of product string*/
  USBD_IDX_SERIAL_STR,        /*Index of serial number string*/
  USBD_CFG_MAX_NUM            /*bNumConfigurations*/
} ; /* USB_DeviceDescriptor */

///* USB Standard Device Descriptor */
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
__ALIGN_BEGIN uint8_t USBD_VCP_LangIDDesc[USB_SIZ_STRING_LANGID] __ALIGN_END =
{
  USB_SIZ_STRING_LANGID,
  USB_DESC_TYPE_STRING,
  LOBYTE(USBD_VCP_LANGID_STRING),
  HIBYTE(USBD_VCP_LANGID_STRING),
};

uint8_t USBD_VCP_StringSerial[USB_SIZ_STRING_SERIAL] =
{
  USB_SIZ_STRING_SERIAL,
  USB_DESC_TYPE_STRING,
};

//__ALIGN_BEGIN uint8_t USBD_StrDesc[USB_MAX_STR_DESC_SIZ] __ALIGN_END ;

/**
  * @}
  */


/** @defgroup USBD_DESC_Private_FunctionPrototypes
  * @{
  */
void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len);
void Get_SerialNum_VCP(void);
/**
  * @}
  */


/** @defgroup USBD_DESC_Private_Functions
  * @{
  */

/**
* @brief  USBD_USR_DeviceDescriptor
*         return the device descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_VCP_DeviceDescriptor( uint8_t speed , uint16_t *length)
{
  *length = sizeof(USBD_VCP_DeviceDesc);
  return (uint8_t*)USBD_VCP_DeviceDesc;
}

/**
* @brief  USBD_USR_LangIDStrDescriptor
*         return the LangID string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_VCP_LangIDStrDescriptor( uint8_t speed , uint16_t *length)
{
  *length =  sizeof(USBD_LangIDDesc);
  return (uint8_t*)USBD_VCP_LangIDDesc;
}


/**
* @brief  USBD_USR_ProductStrDescriptor
*         return the product string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_VCP_ProductStrDescriptor( uint8_t speed , uint16_t *length)
{
  if(speed == 0)
  {
    USBD_GetString((uint8_t *)(uint8_t *)USBD_VCP_PRODUCT_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString((uint8_t *)(uint8_t *)USBD_VCP_PRODUCT_FS_STRING, USBD_StrDesc, length);
  }
  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_ManufacturerStrDescriptor
*         return the manufacturer string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_VCP_ManufacturerStrDescriptor( uint8_t speed , uint16_t *length)
{
  USBD_GetString((uint8_t *)(uint8_t *)USBD_VCP_MANUFACTURER_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}

/**
* @brief  USBD_USR_SerialStrDescriptor
*         return the serial number string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_VCP_SerialStrDescriptor( uint8_t speed , uint16_t *length)
{
  *length = USB_SIZ_STRING_SERIAL;

  /* Update the serial number string descriptor with the data from the unique ID*/
  Get_SerialNum_VCP();

  return (uint8_t*)USBD_VCP_StringSerial;
}

/**
* @brief  USBD_USR_ConfigStrDescriptor
*         return the configuration string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_VCP_ConfigStrDescriptor( uint8_t speed , uint16_t *length)
{
  if(speed  == USB_OTG_SPEED_HIGH)
  {
    USBD_GetString((uint8_t *)(uint8_t *)USBD_VCP_CONFIGURATION_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString((uint8_t *)(uint8_t *)USBD_VCP_CONFIGURATION_FS_STRING, USBD_StrDesc, length);
  }
  return USBD_StrDesc;
}


/**
* @brief  USBD_USR_InterfaceStrDescriptor
*         return the interface string descriptor
* @param  speed : current device speed
* @param  length : pointer to data length variable
* @retval pointer to descriptor buffer
*/
uint8_t *  USBD_USR_VCP_InterfaceStrDescriptor( uint8_t speed , uint16_t *length)
{
  if(speed == 0)
  {
    USBD_GetString((uint8_t *)(uint8_t *)USBD_VCP_INTERFACE_HS_STRING, USBD_StrDesc, length);
  }
  else
  {
    USBD_GetString((uint8_t *)(uint8_t *)USBD_VCP_INTERFACE_FS_STRING, USBD_StrDesc, length);
  }
  return USBD_StrDesc;
}

/**
  * @brief  Create the serial number string descriptor
  * @param  None
  * @retval None
  */
void Get_SerialNum_VCP(void)
{
  uint32_t deviceserial0, deviceserial1, deviceserial2;

  deviceserial0 = *(uint32_t*)DEVICE_ID1;
  deviceserial1 = *(uint32_t*)DEVICE_ID2;
  deviceserial2 = *(uint32_t*)DEVICE_ID3;

  deviceserial0 += deviceserial2;

  if (deviceserial0 != 0)
  {
    IntToUnicode (deviceserial0, &USBD_VCP_StringSerial[2] ,8);
    IntToUnicode (deviceserial1, &USBD_VCP_StringSerial[18] ,4);
  }
}

///**
//  * @brief  Convert Hex 32Bits value into char
//  * @param  value: value to convert
//  * @param  pbuf: pointer to the buffer
//  * @param  len: buffer length
//  * @retval None
//  */
//void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len)
//{
//  uint8_t idx = 0;
//
//  for( idx = 0 ; idx < len ; idx ++)
//  {
//    if( ((value >> 28)) < 0xA )
//    {
//      pbuf[ 2* idx] = (value >> 28) + '0';
//    }
//    else
//    {
//      pbuf[2* idx] = (value >> 28) + 'A' - 10;
//    }
//
//    value = value << 4;
//
//    pbuf[ 2* idx + 1] = 0;
//  }
//}


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

