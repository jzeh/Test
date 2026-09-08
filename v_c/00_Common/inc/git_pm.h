/*----------------------------------------------------------------------
 *   Power Management
 *--------------------------------------------------------------------*/
#ifndef	__GIT_PM_H__
#define	__GIT_PM_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/
#define PWR_WAKEUP_PIN_FLAGS 						(PWR_WAKEUP_FLAG1 | \
													 PWR_WAKEUP_FLAG2 | \
													 PWR_WAKEUP_FLAG3 | \
													 PWR_WAKEUP_FLAG4 | \
													 PWR_WAKEUP_FLAG5 | \
													 PWR_WAKEUP_FLAG6)

#define WAKEUP_SOURCE_IG							(0x00000001U)
#define WAKEUP_SOURCE_SENSOR						(0x00000002U)
#define WAKEUP_SOURCE_HCAN1							(0x00000004U)
#define WAKEUP_SOURCE_HCAN2							(0x00000008U)
#define WAKEUP_SOURCE_LCAN							(0x00000010U)
#define WAKEUP_SOURCE_TRG							(0x00000020U)
#define WAKEUP_SOURCE_12V_DET						(0x00000040U)
#define WAKEUP_SOURCE_24V_DET						(0x00000080U)
#define	WAKEUP_SOURCE_ALL_DOWN						(0x00000100U)
#ifdef ADDTOLATCH_WIFI // mod.kks 22.05.01
#define	LAT_WIFI_BT_PWR_EN							(0x00000200U)
#endif
#define WAKEUP_SOURCE_ALL							(0xFFFFFFFFU)

/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
extern void initPowerManagement( void );
extern void gotoStandbyMode( uint32_t flag );
extern void setWakeUpSource( uint32_t flag );

/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/
extern uint8_t	gStanbyExitFlag;
extern uint32_t	gPMFlag;

#endif // __GIT_PM_H__