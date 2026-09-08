/**
  ******************************************************************************
  * @file    usbd_bulk_core.h
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   header file for the usbd_bulk_core.c file.
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

#ifndef __USB_BULK_CORE_H_
#define __USB_BULK_CORE_H_


#include  "usbd_ioreq.h"
#include "usb_dcd_int.h"
#include "usbd_conf.h"

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */

/** @defgroup usbd_bulk
  * @brief This file is the Header file for USBD_bulk.c
  * @{
  */


/** @defgroup usbd_bulk_Exported_Defines
  * @{
  */

#define USB_BULK_CONFIG_DESC_SIZ                (32)//(67)
#define USB_BULK_DESC_SIZ                       (32-9)//(67-9)

#define USB_USR_RXBUF_SIZE  1024    // HOST->DEVICE
#define USB_USR_TXBUF_SIZE  1024    // DEVICE->HOST
typedef struct{
vu8  txbuf[USB_USR_TXBUF_SIZE];
vu16 tx_rdindex;
vu16 tx_wrindex;
vu16 txcnt;

vu8 rxbuf[USB_USR_RXBUF_SIZE];
vu16 rx_rdindex;
vu16 rx_wrindex;
vu16 rxcnt;
vu8 tx_status;
vu8 err;
}_USB_APP_STRUCT;

enum{
    USB_USER_NOERR=0,
    USB_USER_RXBUF_OVER=0x01

};

extern _USB_APP_STRUCT UsbUser;


//#define USB_BULK_CONFIG_DESC_SIZ                (67)
//#define USB_BULK_DESC_SIZ                       (67-9)

#define BULK_DESCRIPTOR_TYPE                     0x21

//#define DEVICE_CLASS_BULK                        0x02
//#define DEVICE_SUBCLASS_BULK                     0x00


#define USB_DEVICE_DESCRIPTOR_TYPE              0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define USB_STRING_DESCRIPTOR_TYPE              0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05

#define STANDARD_ENDPOINT_DESC_SIZE             0x09

#define BULK_DATA_IN_PACKET_SIZE                BULK_DATA_MAX_PACKET_SIZE

#define BULK_DATA_OUT_PACKET_SIZE               BULK_DATA_MAX_PACKET_SIZE

extern __ALIGN_BEGIN uint8_t USB_Tx_Buffer   [BULK_OUT_PACKET_SZE] __ALIGN_END ;

/*---------------------------------------------------------------------*/
/*  BULK definitions                                                    */
/*---------------------------------------------------------------------*/

/**************************************************/
/* BULK Requests                                   */
/**************************************************/
#define SEND_ENCAPSULATED_COMMAND               0x00
#define GET_ENCAPSULATED_RESPONSE               0x01
#define SET_COMM_FEATURE                        0x02
#define GET_COMM_FEATURE                        0x03
#define CLEAR_COMM_FEATURE                      0x04
#define SET_LINE_CODING                         0x20
#define GET_LINE_CODING                         0x21
#define SET_CONTROL_LINE_STATE                  0x22
#define SEND_BREAK                              0x23
#define NO_CMD                                  0xFF

/**
  * @}
  */


/** @defgroup USBD_CORE_Exported_TypesDefinitions
  * @{
  */
/**
  * @}
  */



/** @defgroup USBD_CORE_Exported_Macros
  * @{
  */

/**
  * @}
  */

/** @defgroup USBD_CORE_Exported_Variables
  * @{
  */

extern USBD_Class_cb_TypeDef  USBD_BULK_cb;
/**
  * @}
  */

/** @defgroup USB_CORE_Exported_Functions
  * @{
  */
/**
  * @}
  */

#endif  // __USB_BULK_CORE_H_
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
