/*----------------------------------------------------------------------
 *   IO Control
 *--------------------------------------------------------------------*/
#ifndef	__GIT_IOCTL_H__
#define	__GIT_IOCTL_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "main.h"

/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/
#define IO_CONTROL_HIGH( __name )							HAL_GPIO_WritePin( __name ## _GPIO_Port, __name ## _Pin, GPIO_PIN_SET )
#define IO_CONTROL_LOW( __name )							HAL_GPIO_WritePin( __name ## _GPIO_Port, __name ## _Pin, GPIO_PIN_RESET )
#define IO_CONTROL_TOGGLE( __name )						HAL_GPIO_TogglePin( __name ## _GPIO_Port, __name ## _Pin )
#define IO_CONTROL_GET( __name )							HAL_GPIO_ReadPin( __name ## _GPIO_Port, __name ## _Pin )

// KL Line
#define KL_LINE1_CONNECT_CH01								0x01
#define KL_LINE1_CONNECT_CH02								0x02
#define KL_LINE1_CONNECT_CH03								0x03
#define KL_LINE1_CONNECT_CH06								0x06
#define KL_LINE1_CONNECT_CH07								0x07
#define KL_LINE1_CONNECT_CH08								0x08
#define KL_LINE1_CONNECT_CH13								0x0d
#define KL_LINE1_CONNECT_CH15								0x0f
#define KL_LINE1_CONNECT_CH01_CH08							0x81
#define KL_LINE1_CONNECT_CH02_CH08							0x82
#define KL_LINE1_CONNECT_CH03_CH08							0x83
#define KL_LINE1_CONNECT_CH06_CH08							0x86
#define KL_LINE1_CONNECT_CH07_CH08							0x87
#define KL_LINE1_CONNECT_CH13_CH08							0x8d
#define KL_LINE1_CONNECT_CH15_CH08							0x8f

#define KL_LINE2_CONNECT_CH08								0x08
#define KL_LINE2_CONNECT_CH09								0x09
#define KL_LINE2_CONNECT_CH10								0x0a
#define KL_LINE2_CONNECT_CH11								0x0b
#define KL_LINE2_CONNECT_CH12								0x0c
#define KL_LINE2_CONNECT_CH14								0x0e
#define KL_LINE2_CONNECT_CH15								0x0f
#define KL_LINE2_CONNECT_CH08_CH15							0xf8
#define KL_LINE2_CONNECT_CH09_CH15							0xf9
#define KL_LINE2_CONNECT_CH10_CH15							0xfa
#define KL_LINE2_CONNECT_CH11_CH15							0xfb
#define KL_LINE2_CONNECT_CH12_CH15							0xfc
#define KL_LINE2_CONNECT_CH14_CH15							0xfe

#define KL_LINE1_CONNECT_2K								do{ IO_CONTROL_HIGH( TXD1_2K_EN );	IO_CONTROL_LOW(  TXD1_47K_EN );	IO_CONTROL_LOW(  TXD1_510_EN ); IO_CONTROL_HIGH( KL_RXD1_SEL );	 IO_CONTROL_HIGH( KL_RXD2_SEL );}while(0)
#define KL_LINE1_CONNECT_47K								do{ IO_CONTROL_LOW(  TXD1_2K_EN );	IO_CONTROL_HIGH( TXD1_47K_EN );	IO_CONTROL_LOW(  TXD1_510_EN ); }while(0)
#define KL_LINE1_CONNECT_510								do{ IO_CONTROL_LOW(  TXD1_2K_EN );	IO_CONTROL_LOW(  TXD1_47K_EN );	IO_CONTROL_HIGH( TXD1_510_EN ); }while(0)
#define KL_LINE1_DISCONNECT_PULLUP						do{ IO_CONTROL_LOW(  TXD1_2K_EN );	IO_CONTROL_LOW(  TXD1_47K_EN );	IO_CONTROL_LOW(  TXD1_510_EN ); }while(0)

#define KL_LINE2_CONNECT_2K								do{ IO_CONTROL_HIGH( TXD2_2K_EN );	IO_CONTROL_LOW(  TXD2_47K_EN );	IO_CONTROL_LOW(  TXD2_510_EN ); IO_CONTROL_HIGH( KL_RXD1_SEL );	 IO_CONTROL_HIGH( KL_RXD2_SEL );}while(0)
#define KL_LINE2_CONNECT_47K								do{ IO_CONTROL_LOW(  TXD2_2K_EN );	IO_CONTROL_HIGH( TXD2_47K_EN );	IO_CONTROL_LOW(  TXD2_510_EN ); }while(0)
#define KL_LINE2_CONNECT_510								do{ IO_CONTROL_LOW(  TXD2_2K_EN );	IO_CONTROL_LOW(  TXD2_47K_EN );	IO_CONTROL_HIGH( TXD2_510_EN ); }while(0)
#define KL_LINE2_DISCONNECT_PULLUP						do{ IO_CONTROL_LOW(  TXD2_2K_EN );	IO_CONTROL_LOW(  TXD2_47K_EN );	IO_CONTROL_LOW(  TXD2_510_EN ); }while(0)

// Reprogram Voltage
#define	REPG_LINE_CH03										3
#define	REPG_LINE_CH06										6
#define	REPG_LINE_CH09										9
#define	REPG_LINE_CH11										11
#define	REPG_LINE_CH12										12
#define	REPG_LINE_CH13										13
#define	REPG_LINE_CH14		 								14

// Reprogram
#define REPG_LINE_DISABLE		do{IO_CONTROL_LOW( REPG_CH3 ); IO_CONTROL_LOW( REPG_CH6  ); IO_CONTROL_LOW( REPG_CH9  );	IO_CONTROL_LOW( REPG_CH11 ); IO_CONTROL_LOW( REPG_CH12 ); IO_CONTROL_LOW( REPG_CH13 ); IO_CONTROL_LOW( REPG_CH14 );}while(0)

// SPI NSS
#define CS_ENABLE1											IO_CONTROL_LOW(  SPI2_NSS )
#define CS_DISABLE1										IO_CONTROL_HIGH( SPI2_NSS )

#define CS_ENABLE5											IO_CONTROL_LOW(  SPI5_NSS )
#define CS_DISABLE5										IO_CONTROL_HIGH( SPI5_NSS )

// LAN
#define ETH_PWR_EN_ENABLE									IO_CONTROL_HIGH(ETH_PWR_EN)
#define ETH_PWR_EN_DISENABLE								IO_CONTROL_LOW(ETH_PWR_EN)

// USB
#define USB_ETH_NRST_ENABLE								IO_CONTROL_LOW(USB_ETH_NRST)
#define USB_ETH_NRST_DISABLE								IO_CONTROL_HIGH(USB_ETH_NRST)

#define USB_PHY_RST_ENABLE								IO_CONTROL_HIGH(USB_PHY_RST)
#define USB_PHY_RST_DISABLE 								IO_CONTROL_LOW(USB_PHY_RST)

#define USB_HUB_RST_ENABLE								IO_CONTROL_LOW(USB_HUB_RST)
#define USB_HUB_RST_DISABLE								IO_CONTROL_HIGH(USB_HUB_RST)


/*mod.kks 2023.03.22*/
#define USB_HUB_PWR_ENABLE								IO_CONTROL_HIGH(USB_HUB_PWR_EN)
#define USB_HUB_PWR_DISABLE								IO_CONTROL_LOW(USB_HUB_PWR_EN)
/*end*/

// Can2 120Ohm
#define HIGHCAN2_120OHM_ENABLE							IO_CONTROL_HIGH( HIGHCAN_EN614 )
#define HIGHCAN2_120OHM_DISABLE							IO_CONTROL_LOW(  HIGHCAN_EN614 )

// IG On Detect
#define IG_ON_DETECT_ENABLE								IO_CONTROL_HIGH( IG_ON_EN )
#define IG_ON_DETECT_DISABLE								IO_CONTROL_LOW(  IG_ON_EN )

// Tirgger
#define TRIGGER_LED_ON									IO_CONTROL_HIGH( TRIG_LED_EN )
#define TRIGGER_LED_OFF									IO_CONTROL_LOW(  TRIG_LED_EN )

//LOW_CAN_NSTB
#define LOW_CAN_NSTB_ENABLE								IO_CONTROL_HIGH(LOW_CAN_NSTB)
#define LOW_CAN_NSTB_DISABLE								IO_CONTROL_LOW(LOW_CAN_NSTB)

// KL Line Inverse
#define KL_TXD1_INV_ENABLE								IO_CONTROL_LOW(  KL_TXD1_INV )
#define KL_TXD1_INV_DISABLE								IO_CONTROL_HIGH( KL_TXD1_INV )

#define KL_TXD2_INV_ENABLE								IO_CONTROL_LOW(  KL_TXD2_INV )
#define KL_TXD2_INV_DISABLE								IO_CONTROL_HIGH( KL_TXD2_INV )

#define KL_RXD1_INV_ENABLE								IO_CONTROL_HIGH( KL_RXD1_INV )
#define KL_RXD1_INV_DISABLE								IO_CONTROL_LOW(  KL_RXD1_INV )

#define KL_RXD2_INV_ENABLE								IO_CONTROL_HIGH( KL_RXD2_INV )
#define KL_RXD2_INV_DISABLE								IO_CONTROL_LOW(  KL_RXD2_INV )

#define KL_RXD1_REF_BATTERY								IO_CONTROL_HIGH( KL_RXD1_SEL )
#define KL_RXD1_REF_5V									IO_CONTROL_LOW(  KL_RXD1_SEL )

#define KL_RXD2_REF_BATTERY								IO_CONTROL_HIGH( KL_RXD2_SEL )
#define KL_RXD2_REF_5V									IO_CONTROL_LOW(  KL_RXD2_SEL )

/*----------------------------------------------------------------------
 *   typedef
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
extern void InitIOCTL( void );

extern void EnableEmmc( void );
extern void DisableEmmc( void );

extern void EnableRS9116( void );
extern void DisableRS9116( void );

#ifdef LAN_9514
extern void EnableUSB3300(void);
extern void DisableUSB3300(void);
extern void EnableLAN9514(void);
extern void DisableLAN9514(void);

/*mod.kks 2023.03.23*/
extern void EnableGL850G(void);
extern void DisableGL850G(void);
/*end*/

#else
extern void EnableLAN9371( void );
extern void DisableLAN9371( void );
#endif

extern void EnableEthDiag( void );
extern void DisableEthDiag( void );

extern void EnableHSM( void );
extern void DisableHSM( void );

extern void SetKL_Line( uint8_t line1, uint8_t line2 );
extern void SetReprogramVol( uint8_t line );

 extern void EnableHighCan1( void );
extern void DisableHighCan1( void );
extern void EnableHighCan2( void );
extern void DisableHighCan2( void );
extern void EnableLowCan2( void );
extern void DisableLowCan2( void );
extern void IOCanSetLoopback( void );
extern void SetWakeUpPin_Input( void );
extern void SetWakeUpPin_AF( void );
extern void EnableCan2OSC( void );
extern void DisableCan2OSC( void );
/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/

#endif // __GIT_IOCTL_H__
