#include "usb_trx.h"

/* Enable the Selected IRQ Channels --------------------------------------*/

#if defined(USE_USB_OTG_HS)
#define USB_ENISR    NVIC_EnableIRQ(OTG_HS_IRQn)			//NVIC->ISER[OTG_HS_IRQn >> 0x05] =(uint32_t)0x01 << (OTG_HS_IRQn & (uint8_t)0x1F);
#define USB_DSISR    NVIC_DisableIRQ(OTG_HS_IRQn)			//NVIC->ICER[OTG_HS_IRQn >> 0x05] =(uint32_t)0x01 << (OTG_HS_IRQn & (uint8_t)0x1F);
#else
#define USB_ENISR    NVIC_EnableIRQ(OTG_FS_IRQn)			//NVIC->ISER[OTG_FS_IRQn >> 0x05] =(uint32_t)0x01 << (OTG_FS_IRQn & (uint8_t)0x1F);
#define USB_DSISR    NVIC_DisableIRQ(OTG_FS_IRQn)			//NVIC->ICER[OTG_FS_IRQn >> 0x05] =(uint32_t)0x01 << (OTG_FS_IRQn & (uint8_t)0x1F);
#endif

u16 Usb_Read(u8 *buf,u16 len)
{
	u16 cnt=0;

	if (!UsbUser.rxcnt)
		return 0;

	while(cnt!=len) {
		buf[cnt++] = UsbUser.rxbuf[UsbUser.rx_rdindex++];

		if (UsbUser.rx_rdindex == USB_USR_RXBUF_SIZE)
			UsbUser.rx_rdindex = 0;

		USB_DSISR;
		if ( !--UsbUser.rxcnt )
			break;
		USB_ENISR;
	}

	USB_ENISR;

	return cnt;
}

u16 ChkUsbRx_Buf(void)
{
    return (UsbUser.rxcnt);
}

u16 ChkUsbTx_Buf(void)
{
    return (USB_USR_TXBUF_SIZE-UsbUser.txcnt);
}


u16 Usb_Write(u8 *buf, u16 len)
{
	u16 cnt = 0;

	if ( UsbUser.txcnt == USB_USR_TXBUF_SIZE ) {				// USB_USR_TXBUF_SIZE = 2048
//		printf("usb wr ret: 0\n");
		return 0;
	}

	while( cnt != len ) {
		UsbUser.txbuf[UsbUser.tx_wrindex++] = buf[cnt++];

		if (UsbUser.tx_wrindex == USB_USR_TXBUF_SIZE)
			UsbUser.tx_wrindex = 0;

		USB_DSISR;

		if (++UsbUser.txcnt == USB_USR_TXBUF_SIZE)
			break;

		USB_ENISR;
	}
	USB_ENISR;

//	printf("usb wr ret: %d\n", cnt);
	return cnt;
}
