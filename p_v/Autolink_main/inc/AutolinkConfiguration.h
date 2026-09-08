/**
  ******************************************************************************
  * @file    AutolinkConfiguration.h
  * @author  GIT Connectivity Development 2 Team
  * @version V1.0.0
  * @date    06-Feb-2018
  * @brief   Header for AutolinkConfiguration.c module
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AUTOLINK_CONFIGURATION_H__
#define __AUTOLINK_CONFIGURATION_H__

#include "common.h"

// related with modem
#define KTF_APN_URL				    "lte.ktfwing.com"
#define SKT_APN_URL				    "lte.sktelecom.com"
#define SKT_ROAMING_APN_URL			"skt.iot.kr"
#define SKT_ROAMING_APN_URL_2		"m2mvf.skt.gdsp"
#define UPLUS_APN_URL               "internet.lguplus.co.kr"
#define TELSTRA_APN_URL             "telstra.m2m"
#define TELSTRA_COMERCIAL_APN_URL   "telstra.wap"

#define DEFAULT_APN_URL		        TELSTRA_APN_URL

// if me apply ksa use this define
//#define USE_KSA_URL_PORT

// server related
#define ENABLE_URL_HTTPS    // if define ENABLE_URL_HTTPS, use HTTPS
//#define ENABLE_URL_HTTP     // if define ENABLE_URL_HTTP, use HTTP

// related with server for dcs
//#define DCS_KOREA_SERVER
//#define DCS_AUS_SERVER
//#define DCS_NZ_SERVER

// related with file

// related with message
#define DEFAULT_TEST_VIN_4				"KM123456784"	//010-2022-4271	//??
#define DEFAULT_TEST_VIN_5				"KM123456785"
#define DEFAULT_TEST_VIN_6				"KMTG241ABJU005612" //"KMTG241ABJU005612" //G70 "KM123456786"	//010-4130-2189	//보관
//#define DEFAULT_TEST_VIN_6				"K12GL4EE0H01234567" // G80 "KM123456786"	//010-4130-2189	//보관

#define DEFAULT_TEST_VIN_7				"KM123456787"
#define DEFAULT_TEST_VIN_8				"KM123456788"	//010-8577-2189	//문현걸
#define DEFAULT_TEST_VIN_9				"KM123456789"	//010-2107-8510	//kim younsu
#define DEFAULT_TEST_VIN_10				"KM123456790"	//010-2022-8507	//김철훈
#define DEFAULT_TEST_VIN_11				"KM123456791"	//010-2102-8590	//hkmoon
#define DEFAULT_TEST_VIN_12				"KM123456792"	//010-2376-3704  //연수씨
#define DEFAULT_TEST_VIN_13				"KM123456793"   //+61436348199  // AU Telstar
#define DEFAULT_TEST_VIN_14				"KM123456794"   //+61455793961  // AU Telstar
#define DEFAULT_TEST_VIN_15				"KM123456795"   //+61455793969  // AU Telstar

#define STR_DEFAULT_DEV_VIN		        DEFAULT_TEST_VIN_15
#define STR_DEFAULT_VIN                 "GITDEFAUTOVIN0000"

//#define FIX_VIN
#ifdef FIX_VIN
#define FIX_VIN_NUMBER 					"KNAF3416BN5118622"
#endif
// enable to use develupment vin
//#define ENABLE_DEV_VIN

// related with system
#define ONE_SECOND                          (1*1000)
#define ONE_MINUTE                          (60*ONE_SECOND)
#define ONE_MINUTE_SEC                 		(60)
#if defined(QA_FIFA)
#define TIMER_DRIVING_INTERVAL_VALUE        (5*ONE_SECOND)
#else
#define TIMER_DRIVING_INTERVAL_VALUE        (30*ONE_SECOND)
#endif
//#define TIMER_DRIVING_INTERVAL_VALUE        (10*ONE_SECOND)

#define TIMER_SYSTEM_TIMEOUT_INTERVAL_VALUE (10*ONE_SECOND)

// related with rtc alram
// unit : second
#define RTC_SET_WAKEUP_ALRAM_TIME           (30*60)

//4 hours TIME_SET_ALRAM(every wake up time) * this count
#define RTC_SET_UPDATE_FIRMWARE_ALRAM_TIME  (4*60*60)

#define DTC_DIVIDE_MEMORY_SIZE          1000

// OBD_Manager

#if defined(FEATURE_EXTENSION_BOARD)
#define OBD_SLEEP_TIME					(20*ONE_SECOND)	//슬립플래그를 띄우는 시간
#else
#define OBD_SLEEP_TIME					(60*ONE_SECOND)	//슬립플래그를 띄우는 시간
#endif
#define CV_FCS_TIMEOUT                  (10*ONE_SECOND)
#define FCS_TIMEOUT						(30*ONE_SECOND) //FCS시작후 30초이상 지나면 ERROR간주 TIMEOUT BASIC루틴 적용 프리미엄에서 본적없음
#define AUTOVIN_TIMEOUT				(10*ONE_SECOND) //AUTOVIN 시작후 30초이상 지나면 ERROR간주 TIMEOUT BASIC루틴 적용 프리미엄에서 본적없음
#define FUELLEVEL_CHECK_TIME			(2*ONE_SECOND)	//엔진Run후 빈을 얼마후에 할것인지
#define SOH_CHECK_TIME					(3*ONE_SECOND)	//엔진Run후 빈을 얼마후에 할것인지
#define AUTOVIN_START_TIME				(4*ONE_SECOND)	//엔진Run후 빈을 얼마후에 할것인지
#define FCS_START_TIME					(7*ONE_SECOND)	//엔진Run후 FCS을 얼마후에 할것인지
#define ACTUATOR_MAX_REQ_CNT			40		//개당 118byte_200727
#define ACTUATOR_MAX_CONVERT_CNT 	    10
#define ACTUATOR_MAX_RES_CNT			60
#define ACTUATOR_MAX_READY_CNT			50
#define ACTUATOR_CHECK_WAIT_TIME		(3*ONE_SECOND)	//1000ms 밑으로 내리면 체크못할경우존재
#define ACTUATOR_CHECK_BASE_TIME		(100)
#define ACTUATOR_MAX_ENGONTIME	        (600*ONE_SECOND)
#define ACTUATOR_MAX_AIRCONTIME             (10*ONE_SECOND) // EVNONEREADY TYPE 에서  체크하지 못하는 경우 존재 

#define ACTUATOR_MAX_AIR_CNT            40   // dahae
#define ACTUATOR_MAX_CTRL_CNT           30   // dahae

#define IND_CANOFF_CHECK_TIME			(ONE_SECOND)
#define IND_LASTON_CHECK_TIME			(2*ONE_SECOND)
#define IND_LASTOFF_CHECK_TIME			(2*ONE_SECOND)

#define ACTUATORRESINFOLISTCNT			20

// OBD_Controller
#define DECIDE_ENGINE_STARTING_RPM 			300		/*50 RPM*/
#define DECIDE_ENGINE_STOP_RPM   			0		/*0 RPM*/


#define DECIDE_BATTERY_HIGH_VOLTAGE_ON 		165		/* 16.5 V */
#define DECIDE_BATTERY_HIGH_VOLTAGE_OFF 	150		/* 15.0 V */

#define DECIDE_BATTERY_LOW_VOLTAGE_ON 		117		/* 11.7 V */
#define DECIDE_BATTERY_LOW_VOLTAGE_OFF 		120		/* 12.0 V */

#define DECIDE_ENGINE_OVER_TEMPERATURE_ON 	115		/* 115 'c */
#define DECIDE_ENGINE_OVER_TEMPERATURE_OFF 	95		/* 95 'c */

#define DECIDE_FUEL_RUN_OUT_ON 				10		/* 10 % */
#define DECIDE_FUEL_RUN_OUT_OFF 			20		/* 20 % */

//#define DECIDE_OVER_TIRE_PRESSURE 			30		/* 30 PSI */
#define DECIDE_UNDER_TIRE_PRESSURE 			172		/* 25 PSI  = 172.368332 kPa*/

#define DECIDE_OVER_SPEED_ON 				160 	/* 160 km/h */
#define DECIDE_OVER_SPEED_OFF 				150 	/* 160 km/h */

#define DECIDE_OVER_RPM_ON 					5000 	/* 5000 RPM */
#define DECIDE_OVER_RPM_OFF 				4500 	/* 4500 RPM */

#define DECIDE_OVER_MOTOR_RPM_ON 			10000 	/* 5000 RPM */
#define DECIDE_OVER_MOTOR_RPM_OFF			9500 	/* 4500 RPM */

#define BT_STATE_CONNECTION_ON          0x01  /*BT Connection */
#define BT_STATE_CONNECTION_OFF         0x00 /*BT DisConnection */


// if disable this define, system won't go to sleep.
#define ENABLE_SYSTEM_SLEEP
#define USE_GEMALTO_MODEM
// this define for modem sleep when user didn't use during 96hours.
#define USE_96_HOURS_POWER_OFF

// if use this define, fota will be tested whenever the device go to sleep.
//#define ENABLE_FOTA_TEST
// this define just test download
//#define ENABLE_FOTA_DOWNLOAD_TEST

#define FW_VERSION_MAJOR    (0x00)
#define FW_VERSION_MINOR    (0x56)
#define FW_VERSION() 	    (FW_VERSION_MAJOR<<8|FW_VERSION_MINOR)

#define PROTOCOL12
#define PROTOCOL13
#define PROTOCOL14
#define PROTOCOL15
#define PROTOCOL16
#define PROTOCOL17
#define PROTOCOL18
#define PROTOCOL19

#define PROTOCOL21
#define PROTOCOL22
#define PROTOCOL23
#define PROTOCOL24
#define PROTOCOL25

#define GetServerProtocolVersion() "2.5"

// this is sampling time in obd manager
#define MAX_GPS_SAMPLE_TIME 5 // unit : seconds

#ifdef USE_UBLOX_GPS
#define ENABLE_UBLOX_GPS_DATA
#endif

#endif //__AUTOLINK_CONFIGURATION_H__

/***************************** END OF FILE ****/

