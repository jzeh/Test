/*----------------------------------------------------------------------
 *   LED Control
 *--------------------------------------------------------------------*/
#ifndef __LED_CONTOL_H__
#define __LED_CONTOL_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "main.h"
#include "git_ioctl.h"

/*----------------------------------------------------------------------
 *   LED State
 *--------------------------------------------------------------------*/
// Flags
#define	LED_STATE_TXWAIT						(0x80000000U)			// 31
#define	LED_STATE_UPDATE						(0x40000000U)			// 30
#define	LED_STATE_BYPASS						(0x20000000U)			// 29
#define LED_STATE_EXTON							(0x10000000U)			// 28
#define LED_STATE_EXTOFF						(0x08000000U)			// 27
#define LED_STATE_DUMMY26						(0x04000000U)			// 26
#define LED_STATE_DUMMY25						(0x02000000U)			// 25
#define LED_STATE_DUMMY24						(0x01000000U)			// 24
#define LED_STATE_DUMMY23						(0x00800000U)			// 23
#define LED_STATE_DUMMY22						(0x00400000U)			// 22
#define LED_STATE_DUMMY21						(0x00200000U)			// 21
#define LED_STATE_DUMMY20						(0x00100000U)			// 20
#define LED_STATE_DUMMY19						(0x00080000U)			// 19
#define LED_STATE_DUMMY18						(0x00040000U)			// 18
#define LED_STATE_DUMMY17						(0x00020000U)			// 17
#define LED_STATE_DUMMY16						(0x00010000U)			// 16
#define LED_STATE_DUMMY15						(0x00008000U)			// 15
#define LED_STATE_DUMMY14						(0x00004000U)			// 14
#define LED_STATE_DUMMY13						(0x00002000U)			// 13
#define LED_STATE_DUMMY12						(0x00001000U)			// 12
#define LED_STATE_DUMMY11						(0x00000800U)			// 11
#define LED_STATE_DUMMY10						(0x00000400U)			// 10
#define LED_STATE_DUMMY09						(0x00000200U)			// 09
#define LED_STATE_DUMMY08						(0x00000100U)			// 08
#define LED_STATE_DUMMY07						(0x00000080U)			// 07
#define LED_STATE_DUMMY06						(0x00000040U)			// 06
#define LED_STATE_DUMMY05						(0x00000020U)			// 05
#define LED_STATE_DUMMY04						(0x00000010U)			// 04
#define LED_STATE_DUMMY03						(0x00000008U)			// 03
#define LED_STATE_DUMMY02						(0x00000004U)			// 02
#define LED_STATE_DUMMY01						(0x00000002U)			// 01
#define	LED_STATE_NORMAL						(0x00000001U)			// 00

typedef struct
{
	union
	{
		struct
		{
			uint32_t normal	: 1;				// 0
			uint32_t dymmy	: 26;
			uint32_t extoff	: 1;				// 27
			uint32_t exton	: 1;				// 28
			uint32_t bypass	: 1;				// 29
			uint32_t update	: 1;				// 30
			uint32_t txwait	: 1;				// 31
		};

		uint32_t flags;
	};
} LED_State_t;

/*----------------------------------------------------------------------
 *   LED Control
 *--------------------------------------------------------------------*/
#define	LED_RED_ON								IO_CONTROL_HIGH( RED_LED_EN )
#define	LED_RED_OFF								IO_CONTROL_LOW( RED_LED_EN )
#define	LED_RED_TOGGLE							IO_CONTROL_TOGGLE( RED_LED_EN )

#define	LED_GREEN_ON							IO_CONTROL_HIGH( GREEN_LED_EN )
#define	LED_GREEN_OFF							IO_CONTROL_LOW( GREEN_LED_EN )
#define	LED_GREEN_TOGGLE						IO_CONTROL_TOGGLE( GREEN_LED_EN )

#define	LED_BLUE_ON								IO_CONTROL_HIGH( BLUE_LED_EN )
#define	LED_BLUE_OFF							IO_CONTROL_LOW( BLUE_LED_EN )
#define	LED_BLUE_TOGGLE							IO_CONTROL_TOGGLE( BLUE_LED_EN )

#define	LED_YELLOW_ON							do{ LED_RED_ON;		LED_GREEN_ON;		LED_BLUE_OFF; }while(0)
#define	LED_YELLOW_OFF							do{ LED_RED_OFF;	LED_GREEN_OFF;		LED_BLUE_OFF; }while(0)
#define	LED_YELLOW_TOGGLE						do{ LED_RED_TOGGLE;	LED_GREEN_TOGGLE;	LED_BLUE_OFF; }while(0)

#define	LED_CYAN_ON								do{ LED_RED_OFF;	LED_GREEN_ON;		LED_BLUE_ON;	}while(0)
#define	LED_CYAN_OFF							do{ LED_RED_OFF;	LED_GREEN_OFF;		LED_BLUE_OFF;	}while(0)
#define	LED_CYAN_TOGGLE							do{ LED_RED_OFF;	LED_GREEN_TOGGLE;	LED_BLUE_TOGGLE;}while(0)

#define	LED_MAGENTA_ON							do{ LED_RED_ON;		LED_GREEN_OFF;		LED_BLUE_ON;	}while(0)
#define	LED_MAGENTA_OFF							do{ LED_RED_OFF;	LED_GREEN_OFF;		LED_BLUE_OFF;	}while(0)
#define	LED_MAGENTA_TOGGLE						do{ LED_RED_TOGGLE;	LED_GREEN_OFF;		LED_BLUE_TOGGLE;}while(0)

#define	LED_WHITE_ON							do{ LED_RED_ON;		LED_GREEN_ON;		LED_BLUE_ON;	}while(0)
#define	LED_WHITE_OFF							do{ LED_RED_OFF;	LED_GREEN_OFF;		LED_BLUE_OFF;	}while(0)
#define	LED_WHITE_TOGGLE						do{ LED_RED_TOGGLE;	LED_GREEN_TOGGLE;	LED_BLUE_TOGGLE;}while(0)

#define	LED_TRIG_ON								IO_CONTROL_HIGH( TRIG_LED_EN )
#define	LED_TRIG_OFF							IO_CONTROL_LOW( TRIG_LED_EN )
#define	LED_TRIG_TOGGLE							IO_CONTROL_TOGGLE( TRIG_LED_EN )

#define	LED_ALL_ON								do{ LED_RED_ON;		LED_GREEN_ON;		LED_BLUE_ON;		LED_TRIG_ON;	}while(0)
#define	LED_ALL_OFF								do{ LED_RED_OFF;	LED_GREEN_OFF;		LED_BLUE_OFF;		LED_TRIG_OFF;	}while(0)
#define LED_ALL_TOGGLE							do{ LED_RED_TOGGLE;	LED_GREEN_TOGGLE;	LED_BLUE_TOGGLE;	LED_TRIG_TOGGLE;}while(0)

#endif // __LED_CONTOL_H__