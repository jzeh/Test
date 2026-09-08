/**
  ******************************************************************************
  * @file    usbd_bulk_core.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   This file provides the high layer firmware functions to manage the
  *          following functionalities of the USB BULK Class:
  *           - Initialization and Configuration of high and low layer
  *           - Enumeration as BULK Device (and enumeration for each implemented memory interface)
  *           - OUT/IN data transfer
  *           - Command IN transfer (class requests management)
  *           - Error management
  *
  *  @verbatim
  *
  *          ===================================================================
  *                                BULK Class Driver Description
  *          ===================================================================
  *           This driver manages the "Universal Serial Bus Class Definitions for Communications Devices
  *           Revision 1.2 November 16, 2007" and the sub-protocol specification of "Universal Serial Bus
  *           Communications Class Subclass Specification for PSTN Devices Revision 1.2 February 9, 2007"
  *           This driver implements the following aspects of the specification:
  *             - Device descriptor management
  *             - Configuration descriptor management
  *             - Enumeration as BULK device with 2 data endpoints (IN and OUT) and 1 command endpoint (IN)
  *             - Requests management (as described in section 6.2 in specification)
  *             - Abstract Control Model compliant
  *             - Union Functional collection (using 1 IN endpoint for control)
  *             - Data interface class

  *           @note
  *             For the Abstract Control Model, this core allows only transmitting the requests to
  *             lower layer dispatcher (ie. usbd_bulk_vcp.c/.h) which should manage each request and
  *             perform relative actions.
  *
  *           These aspects may be enriched or modified for a specific user application.
  *
  *            This driver doesn't implement the following aspects of the specification
  *            (but it is possible to manage these features with some modifications on this driver):
  *             - Any class-specific aspect relative to communication classes should be managed by user application.
  *             - All communication classes other than PSTN are not managed
  *
  *  @endverbatim
  *
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
#include "usbd_bulk_core.h"
#include "usbd_desc.h"
#include "usbd_req.h"

vu16 g_Checktxcnt = 0;

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */


/** @defgroup usbd_bulk
  * @brief usbd core module
  * @{
  */

/** @defgroup usbd_bulk_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup usbd_bulk_Private_Defines
  * @{
  */
/**
  * @}
  */


/** @defgroup usbd_bulk_Private_Macros
  * @{
  */
/**
  * @}
  */


/** @defgroup usbd_bulk_Private_FunctionPrototypes
  * @{
  */

_USB_APP_STRUCT UsbUser;


/*********************************************
   BULK Device library callbacks
 *********************************************/
static uint8_t  usbd_bulk_Init        (void  *pdev, uint8_t cfgidx);
static uint8_t  usbd_bulk_DeInit      (void  *pdev, uint8_t cfgidx);
static uint8_t  usbd_bulk_Setup       (void  *pdev, USB_SETUP_REQ *req);
static uint8_t  usbd_bulk_EP0_RxReady  (void *pdev);
static uint8_t  usbd_bulk_DataIn      (void *pdev, uint8_t epnum);
static uint8_t  usbd_bulk_DataOut     (void *pdev, uint8_t epnum);
static uint8_t  usbd_bulk_SOF         (void *pdev);

/*********************************************
   BULK specific management functions
 *********************************************/
static void Handle_USBAsynchXfer  (void *pdev);
static uint8_t  *USBD_bulk_GetCfgDesc (uint8_t speed, uint16_t *length);
#ifdef USE_USB_OTG_HS
__IO uint32_t remote_wakeup =0;
static uint8_t  *USBD_bulk_GetOtherCfgDesc (uint8_t speed, uint16_t *length);
#endif
/**
  * @}
  */

/** @defgroup usbd_bulk_Private_Variables
  * @{
  */
//extern BULK_IF_Prop_TypeDef  APP_FOPS;
extern uint8_t USBD_BULK_DeviceDesc   [USB_SIZ_DEVICE_DESC];

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN uint8_t usbd_bulk_CfgDesc  [USB_BULK_CONFIG_DESC_SIZ] __ALIGN_END ;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN uint8_t usbd_bulk_OtherCfgDesc  [USB_BULK_CONFIG_DESC_SIZ] __ALIGN_END ;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN static __IO uint32_t  usbd_bulk_AltSet  __ALIGN_END = 0;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN uint8_t USB_BULK_Rx_Buffer   [BULK_OUT_PACKET_SZE] __ALIGN_END ;
__ALIGN_BEGIN uint8_t USB_Tx_Buffer   [BULK_OUT_PACKET_SZE] __ALIGN_END ;
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
//__ALIGN_BEGIN uint8_t APP_Rx_Buffer   [APP_RX_DATA_SIZE] __ALIGN_END ;


#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */


//__ALIGN_BEGIN uint8_t buldBuff[BULK_CMD_PACKET_SZE] __ALIGN_END;
//static uint32_t bulkCmd = 0xFF;
//static uint32_t bulkLen = 0;

/* BULK interface class callbacks structure */
USBD_Class_cb_TypeDef  USBD_BULK_cb =
{
	usbd_bulk_Init,
	usbd_bulk_DeInit,
	usbd_bulk_Setup,
	NULL,                 /* EP0_TxSent, */
	usbd_bulk_EP0_RxReady,
	usbd_bulk_DataIn,
	usbd_bulk_DataOut,
	usbd_bulk_SOF,
	NULL,
	NULL,
	USBD_bulk_GetCfgDesc,
#ifdef USE_USB_OTG_HS
	USBD_bulk_GetOtherCfgDesc, // USBD_bulk_GetOtherCfgDesc, /* use same cobfig as per FS */
#endif /* USE_USB_OTG_HS  */
};

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
/* USB BULK device Configuration Descriptor */
__ALIGN_BEGIN uint8_t usbd_bulk_CfgDesc[USB_BULK_CONFIG_DESC_SIZ]  __ALIGN_END =
{
	/*Configuration Descriptor*/
	0x09,   								/* bLength: Configuration Descriptor size */
	USB_CONFIGURATION_DESCRIPTOR_TYPE,		/* bDescriptorType: Configuration */
	LOBYTE(USB_BULK_CONFIG_DESC_SIZ),		/* wTotalLength:no of returned bytes */
	HIBYTE(USB_BULK_CONFIG_DESC_SIZ),
	0x01,   								/* bNumInterfaces: 2 interface */
	0x01,									/* bConfigurationValue: Configuration value */
	0x00,									/* iConfiguration: Index of string descriptor describing the configuration */
	0xC0,									/* bmAttributes: self powered */
	0x32,// 0x19							/* MaxPower 0 mA ->100mA */

	/*---------------------------------------------------------------------------*/

	/*Interface Descriptor */
	0x09,									/* bLength: Interface Descriptor size */
	USB_INTERFACE_DESCRIPTOR_TYPE,			/* bDescriptorType: Interface */
	/* Interface descriptor type */
	0x00,									/* bInterfaceNumber: Number of Interface */
	0x00,									/* bAlternateSetting: Alternate setting */
	0x02,									/* bNumEndpoints: One endpoints used */
	0xff,									/* bInterfaceClass: Communication Interface Class */
	0x00,									/* bInterfaceSubClass: Abstract Control Model */
	0x00,									/* bInterfaceProtocol: Common AT commands */
	0x00,									/* iInterface: */
	/*---------------------------------------------------------------------------*/

	/*Endpoint 1 Descriptor*/
	0x07,									 /* bLength: Endpoint Descriptor size */
	USB_ENDPOINT_DESCRIPTOR_TYPE,			/* bDescriptorType: Endpoint */
	BULK_IN_EP,								/* bEndpointAddress */
	0x02,									/* bmAttributes: Interrupt */
	LOBYTE(BULK_IN_PACKET_SZE),				/* wMaxPacketSize: */
	HIBYTE(BULK_IN_PACKET_SZE),
	0x00,									/* bInterval: */


	/*Endpoint 2 Descriptor*/
	0x07,									/* bLength: Endpoint Descriptor size */
	USB_ENDPOINT_DESCRIPTOR_TYPE,			/* bDescriptorType: Endpoint */
	BULK_OUT_EP,							/* bEndpointAddress */
	0x02,									/* bmAttributes: Interrupt */
	LOBYTE(BULK_OUT_PACKET_SZE),			/* wMaxPacketSize: */
	HIBYTE(BULK_OUT_PACKET_SZE),
	0x00,//0x01,							/* bInterval: */
} ;

#ifdef USE_USB_OTG_HS
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN uint8_t usbd_bulk_OtherCfgDesc[USB_BULK_CONFIG_DESC_SIZ]  __ALIGN_END =
{
	0x09,	/* bLength: Configuation Descriptor size */
	USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION,
	LOBYTE(USB_BULK_CONFIG_DESC_SIZ),		/* wTotalLength:no of returned bytes */
	HIBYTE(USB_BULK_CONFIG_DESC_SIZ),
	0x01,									/* bNumInterfaces: 2 interface */
	0x01,									/* bConfigurationValue: Configuration value */
	0x00,									/* iConfiguration: Index of string descriptor describing the configuration */
	0xC0,									/* bmAttributes: self powered */
	0x32,// 0x19							/* MaxPower 0 mA ->100mA */

	/*---------------------------------------------------------------------------*/

	/*Interface Descriptor */
	0x09,	/* bLength: Interface Descriptor size */
	USB_INTERFACE_DESCRIPTOR_TYPE,	/* bDescriptorType: Interface */
	/* Interface descriptor type */
	0x00,	/* bInterfaceNumber: Number of Interface */
	0x00,	/* bAlternateSetting: Alternate setting */
	0x02,									/* bNumEndpoints: One endpoints used */
	0xff,									/* bInterfaceClass: Communication Interface Class */
	0x00,									/* bInterfaceSubClass: Abstract Control Model */
	0x00,									/* bInterfaceProtocol: Common AT commands */
	0x00,	/* iInterface: */
	/*---------------------------------------------------------------------------*/

	/*Endpoint 1 Descriptor*/
	0x07,							/* bLength: Endpoint Descriptor size */
	USB_ENDPOINT_DESCRIPTOR_TYPE,	/* bDescriptorType: Endpoint */
	BULK_IN_EP, 							/* bEndpointAddress */
	0x02,									/* bmAttributes: Interrupt */
	LOBYTE(BULK_IN_PACKET_SZE), 			/* wMaxPacketSize: */
	HIBYTE(BULK_IN_PACKET_SZE),
	0x00,									/* bInterval: */


	/*Endpoint 2 Descriptor*/
	0x07,	/* bLength: Endpoint Descriptor size */
	USB_ENDPOINT_DESCRIPTOR_TYPE,	   /* bDescriptorType: Endpoint */
	BULK_OUT_EP,						/* bEndpointAddress */
	0x02,									/* bmAttributes: Interrupt */
	LOBYTE(BULK_OUT_PACKET_SZE),			/* wMaxPacketSize: */
	HIBYTE(BULK_OUT_PACKET_SZE),
	0x00,//0x01,							/* bInterval: */
};
#endif /* USE_USB_OTG_HS  */

/**
  * @}
  */

/** @defgroup usbd_bulk_Private_Functions
  * @{
  */

/**
  * @brief  usbd_bulk_Init
  *         Initilaize the BULK interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  usbd_bulk_Init (void  *pdev,
                               uint8_t cfgidx)
{
	DCD_EP_Open(pdev,BULK_IN_EP,BULK_IN_PACKET_SZE,USB_OTG_EP_BULK);

	DCD_EP_Open(pdev,BULK_OUT_EP,BULK_OUT_PACKET_SZE,USB_OTG_EP_BULK);

	/* Prepare Out endpoint to receive next packet */
	DCD_EP_PrepareRx(pdev,
	               BULK_OUT_EP,
	               (uint8_t*)(USB_BULK_Rx_Buffer),
	               BULK_OUT_PACKET_SZE);

  return USBD_OK;
}

/**
  * @brief  usbd_bulk_Init
  *         DeInitialize the BULK layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  usbd_bulk_DeInit (void  *pdev,
                                 uint8_t cfgidx)
{
    /* Open EP IN */
  DCD_EP_Close(pdev,
              BULK_IN_EP);

  /* Open EP OUT */
  DCD_EP_Close(pdev,
              BULK_OUT_EP);

  return USBD_OK;
}

/**
  * @brief  usbd_bulk_Setup
  *         Handle the BULK specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  usbd_bulk_Setup (void  *pdev,
                                USB_SETUP_REQ *req)
{
	uint16_t len=USB_BULK_DESC_SIZ;
	uint8_t  *pbuf=usbd_bulk_CfgDesc + 9;

	switch (req->bmRequest & USB_REQ_TYPE_MASK)
	{
	/* Standard Requests -------------------------------*/
	case USB_REQ_TYPE_STANDARD:
		switch (req->bRequest)
		{
			case USB_REQ_GET_DESCRIPTOR:
				if( (req->wValue >> 8) == BULK_DESCRIPTOR_TYPE)
				{
					#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
						//pbuf = usbd_bulk_Desc;
						pbuf = usbd_bulk_CfgDesc;
					#else
						pbuf = usbd_bulk_CfgDesc + 9 + (9 * USBD_ITF_MAX_NUM);
					#endif
					len = MIN(USB_BULK_DESC_SIZ , req->wLength);
				}

				USBD_CtlSendData (pdev, pbuf, len);
				break;

			case USB_REQ_GET_INTERFACE :
				USBD_CtlSendData (pdev, (uint8_t *)&usbd_bulk_AltSet, 1);
				break;

			case USB_REQ_SET_INTERFACE :
				if ((uint8_t)(req->wValue) < USBD_ITF_MAX_NUM)
				{
					usbd_bulk_AltSet = (uint8_t)(req->wValue);
				}
				else
				{
					/* Call the error management function (command will be nacked */
					USBD_CtlError (pdev, req);
				}
				break;
/*
		case USB_REQ_CLEAR_FEATURE:
			usbd_bulk_DeInit(pdev, 0);
			usbd_bulk_Init(pdev, 0);
			break;
*/
		}
	}
	return USBD_OK;
}

/**
  * @brief  usbd_bulk_EP0_RxReady
  *         Data received on control endpoint
  * @param  pdev: device device instance
  * @retval status
  */
static uint8_t  usbd_bulk_EP0_RxReady (void  *pdev)
{
	return USBD_OK;
}

/**
  * @brief  usbd_audio_DataIn
  *         Data sent on non-control IN endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  usbd_bulk_DataIn (void *pdev, uint8_t epnum)
{
	u16 cnt = 0;

	if (UsbUser.tx_status){
	    if (UsbUser.txcnt == 0){
	        UsbUser.tx_status=0;
			if ( g_Checktxcnt == BULK_DATA_IN_PACKET_SIZE )
			{
			 DCD_EP_Tx (pdev,
	               BULK_IN_EP,
	               USB_Tx_Buffer,
	               cnt);
			}
	    }else{
	        while(cnt<(BULK_DATA_IN_PACKET_SIZE)){
	            USB_Tx_Buffer[cnt++]=UsbUser.txbuf[UsbUser.tx_rdindex++];
	            if (UsbUser.tx_rdindex==USB_USR_TXBUF_SIZE)
	                UsbUser.tx_rdindex=0;

	            if (!--UsbUser.txcnt)
	                break;
	        }
	        DCD_EP_Tx (pdev,
	               BULK_IN_EP,
	               USB_Tx_Buffer,
	               cnt);
	   }
	}

	return USBD_OK;
}

/**
  * @brief  usbd_bulk_DataOut
  *         Data received on non-control Out endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  usbd_bulk_DataOut (void *pdev, uint8_t epnum)
{
	uint16_t USB_Rx_Cnt;
	u16 tcnt=0;

	/* Get the received data buffer and update the counter */
	USB_Rx_Cnt = ((USB_OTG_CORE_HANDLE*)pdev)->dev.out_ep[epnum].xfer_count;

	/* USB data will be immediately processed, this allow next USB traffic being
	NAKed till the end of the application Xfer */

	while(USB_Rx_Cnt--){
		if (UsbUser.rxcnt==USB_USR_RXBUF_SIZE){
	        UsbUser.err=USB_USER_RXBUF_OVER;
	        break;
		}
		UsbUser.rxbuf[UsbUser.rx_wrindex++]=USB_BULK_Rx_Buffer[tcnt++];
		if (UsbUser.rx_wrindex==USB_USR_RXBUF_SIZE)
			UsbUser.rx_wrindex=0;
		UsbUser.rxcnt++;
	}

	/* Prepare Out endpoint to receive next packet */
	DCD_EP_PrepareRx(pdev,
	               BULK_OUT_EP,
	               (uint8_t*)(USB_BULK_Rx_Buffer),
	               BULK_DATA_OUT_PACKET_SIZE);

	return USBD_OK;
}

/**
  * @brief  usbd_audio_SOF
  *         Start Of Frame event management
  * @param  pdev: instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t  usbd_bulk_SOF (void *pdev)
{
	static uint32_t FrameCount = 0;

	if (FrameCount++ == BULK_IN_FRAME_INTERVAL)
	{
		/* Reset the frame counter */
		FrameCount = 0;

		/* Check the data to be sent through IN pipe */
		Handle_USBAsynchXfer(pdev);
	}

	return USBD_OK;
}

/**
  * @brief  Handle_USBAsynchXfer
  *         Send data to USB
  * @param  pdev: instance
  * @retval None
  */
static void Handle_USBAsynchXfer (void *pdev)
{
	u16 cnt=0;

	if (!UsbUser.tx_status)
	{
	    if (UsbUser.txcnt != 0)
		{
			UsbUser.tx_status=1;
			g_Checktxcnt = UsbUser.txcnt;

			while(cnt<(BULK_DATA_IN_PACKET_SIZE))
			{
				USB_Tx_Buffer[cnt++]=UsbUser.txbuf[UsbUser.tx_rdindex++];
				if (UsbUser.tx_rdindex==USB_USR_TXBUF_SIZE)
					UsbUser.tx_rdindex=0;

				if (!--UsbUser.txcnt)
					break;
			}

			DCD_EP_Tx (pdev, BULK_IN_EP, USB_Tx_Buffer,	cnt);
		}
	}
}

/**
  * @brief  USBD_bulk_GetCfgDesc
  *         Return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_bulk_GetCfgDesc (uint8_t speed, uint16_t *length)
{
  *length = sizeof (usbd_bulk_CfgDesc);
  return usbd_bulk_CfgDesc;
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
