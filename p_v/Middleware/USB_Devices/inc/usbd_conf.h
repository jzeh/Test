/**
  ******************************************************************************
  * @file    usbd_conf.h
  * @author  MCD Application Team
  * @version V1.2.0
  * @date    09-November-2015
  * @brief   USB Device configuration file
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_CONF__H__
#define __USBD_CONF__H__

/* Includes ------------------------------------------------------------------*/
#include "usb_conf.h"

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */

/** @defgroup USBD_CONF
  * @brief This file is the device library configuration file
  * @{
  */

/** @defgroup USBD_CONF_Exported_Defines
  * @{
  */


#define USBD_CFG_MAX_NUM           1
#define USBD_ITF_MAX_NUM           1
#define USB_MAX_STR_DESC_SIZ       64

#define USBD_SELF_POWERED

/* Class Layer Parameter */

/** @defgroup USB_VCP_Class_Layer_Parameter
  * @{
  */
#define CDC_IN_EP                       0x81  /* EP1 for data IN */
#define CDC_OUT_EP                      0x01  /* EP1 for data OUT */
#define CDC_CMD_EP                      0x82  /* EP2 for CDC commands */

#define CDC_DATA_MAX_PACKET_SIZE       64   /* Endpoint IN & OUT Packet size */
#define CDC_CMD_PACKET_SZE             8    /* Control Endpoint Packet size */

#define CDC_IN_FRAME_INTERVAL          5    /* Number of frames between IN transfers */
#define APP_RX_DATA_SIZE               1024 /* Total size of IN buffer:
                                              APP_RX_DATA_SIZE*8/MAX_BAUDARATE*1000 should be > CDC_IN_FRAME_INTERVAL */

#define APP_FOPS                        VCP_fops


/** @defgroup USB_BULK_Class_Layer_Parameter
  * @{
  */

#define BULK_IN_EP							0x81	/* EP1 for data IN */
#define BULK_OUT_EP							0x03	/* EP3 for data OUT */

#define BULK_CMD_EP                 0x82	/* EP2 for CDC commands */

#define BULK_OUT_PACKET_SZE					64		/* Control Endpoint Packet size */
#define BULK_IN_PACKET_SZE          64		/* Control Endpoint Packet size */
																					/* BULK Endpoints parameters: you can fine tune these values depending on the needed baudrates and performance. */

#define BULK_DATA_MAX_PACKET_SIZE			64		/* Endpoint IN & OUT Packet size */
#define BULK_CMD_PACKET_SZE					8		/* Control Endpoint Packet size */

#define BULK_IN_FRAME_INTERVAL				5		/* Number of frames between IN transfers */


/** @defgroup USB_MSC_Class_Layer_Parameter
  * @{
  */

#define MSC_IN_EP                    0x81
#define MSC_OUT_EP                   0x01
#define MSC_MAX_PACKET                64

#define MSC_MEDIA_PACKET             4096

/**
  * @}
  */


/** @defgroup USB_CONF_Exported_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USB_CONF_Exported_Macros
  * @{
  */
/**
  * @}
  */

/** @defgroup USB_CONF_Exported_Variables
  * @{
  */
/**
  * @}
  */

/** @defgroup USB_CONF_Exported_FunctionsPrototype
  * @{
  */
/**
  * @}
  */

#endif //__USBD_CONF__H__

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

