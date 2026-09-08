/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MESSAGE_MANAGER_H__
#define __MESSAGE_MANAGER_H__

#if defined(USE_GIT_FAT_FS)
#include "ff.h"
#endif

//************************************************************************************************
#include "AutolinkMessage.h"
#include "AutolinkConfiguration.h"
#include "SysHalFileSystem.h"

#if defined(PROTOCOL17)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//한국 운영     		X
//한국 개발     		"devapiautolink-premium.gitauto.com  		/api/dcs/"
//호주 현대운영   	"autolinkapi-premium.hmca.com.au 		  /api/dcs/"   																  // https://gds.hyundai-motor.com/autolink_premium/Autolink-premium
//호주 현대개발   	"devautolinkapi-premium.hmca.com.au  	/api/dcs/"   												  				// https://gds.hyundai-motor.com/autolink_dev_premium/Autolink-premium
//호주 현대플릿운영 "fleet-premium-api.hmca.com.au    		/api/dcs/"   														 	 		// https://gds.hyundai-motor.com/autolink_premium/Autolink-premium
//호주 현대플릿개발 "fleet-premium-api-dev.hmca.com.au    /api/dcs/"   																	// https://gds.hyundai-motor.com/autolink_dev_premium/Autolink-premium
//국내 현대플릿개발 "devautolink.gitauto.com							/AutoLink_Fleet_Advance/ServerAPI /api/dcs/ 	// 국내 FOTA  https://gds.hyundai-motor.com/AUTOLINK_Dev_KR/AUTOLINK-Premium/
//호주 기아플릿운영 ""                																																	// https://gds.kia.co.kr/autolink_dev_premium/Autolink-premium
//호주 기아플릿개발 ""                																																	// https://gds.kia.co.kr/autolink_premium/Autolink-premium
//국내 기아플릿개발 "devautolink.gitauto.com							/KiaLink_Fleet_ServerAPI/api/dcs/"						// 국내 FOTA  https://gds.hyundai-motor.com/AUTOLINK_Dev_KR/AUTOLINK-Premium/
//뉴질 운영    		  "apiautolink-premium.hyundai.co.nz  	/api/dcs/"      															// https://gds.hyundai-motor.com/autolink_premium/Autolink-premium
//뉴질 개발    		  "devapiautolink-premium.hyundai.co.nz /api/dcs/"  							 				 						// https://gds.hyundai-motor.com/autolink_dev_premium/Autolink-premium
	//국내 개발
	#define	KR_HTTP_RETAIL_DEV_MESSAGE_URL		"devautolink-premium.gitauto.com"
    #define KR_HTTP_RETAIL_DEV_MESSAGE_URL2      "devapiautolink-premium-v2.gitauto.com"
	#define	HTTP__MESSAGE_PATH							"api/dcs/"
//	#define	KR_HTTP_RETAIL_DEV_MESSAGE_PATH		"api/dcs/"
	//호주 현대 운영
#ifndef USE_KSA_URL_PORT
	#define	AU_HTTP_RETAIL_MESSAGE_URL				"autolinkapi-premium.hmca.com.au"
#else
	#define	AU_HTTP_RETAIL_MESSAGE_URL				"devksa.gitauto.com"
#endif

//	#define	AU_HTTP_RETAIL_MESSAGE_PATH			"api/dcs/"
	//호주 현대 개발
	#define	AU_HTTP_RETAIL_DEV_MESSAGE_URL		"devautolinkapi-premium.hmca.com.au"
//	#define	AU_HTTP_RETAIL_DEV_MESSAGE_PATH		"api/dcs/"
	//호주 현대 플릿 운영
	#define	AU_HTTP_FLEET_MESSAGE_URL				"fleet-premium-api.hmca.com.au"
//	#define	AU_HTTP_FLEET_MESSAGE_PATH				"api/dcs/"
	//호주 현대 플릿 개발
	#define	AU_HTTP_FLEET_DEV_MESSAGE_URL		"fleet-premium-api-dev.hmca.com.au"
//	#define	AU_HTTP_FLEET_DEV_MESSAGE_PATH		"api/dcs/"
	//국내 현대 플릿 개발
#if defined(PROTOCOL19)  //호주 플릿 개발용 서버 주소. 개발 설치용 앱이 바라보는 주소
	//#define	KR_HTTP_FLEET_DEV_MESSAGE_URL         "devautolink.gitauto.com/AutoLink_Fleet_AU/ServerAPI"
	#define	KR_HTTP_FLEET_DEV_MESSAGE_URL          "devautolink.gitauto.com/AutoLink_TD/ServerAPI"
#else
	#define	KR_HTTP_FLEET_DEV_MESSAGE_URL		"devautolink.gitauto.com/AutoLink_Fleet_Advance/ServerAPI"
#endif
//	#define	KR_HTTP_FLEET_DEV_MESSAGE_PATH		"api/dcs/"
	//뉴질 현대 운영
	#define	NZ_HTTP_RETAIL_MESSAGE_URL					"apiautolink-premium.hyundai.co.nz"
	//뉴질 현대 플릿운영
	#define	NZ_HTTP_FLEET_MESSAGE_URL					"autolinkfleet.hyundai.co.nz/api/Premium"
//	#define	NZ_HTTP_RETAIL_MESSAGE_PATH				"api/dcs/"
	//뉴질 현대 개발
	#define	NZ_HTTP_RETAIL_DEV_MESSAGE_URL			"devapiautolink-premium.hyundai.co.nz"
//	#define	NZ_HTTP_RETAIL_DEV_MESSAGE_PATH			"api/dcs/"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//국내 기아 개발 190409기준 미존재로 현대 개발로 적용
	#define	KR_HTTP_RETAIL_DEV_MESSAGE_URL_KIA		"demoapiautolink-premium.gitauto.com"
//	#define	KR_HTTP_RETAIL_DEV_MESSAGE_PATH_KIA	"api/dcs/"
	//호주 기아 운영
	#define	AU_HTTP_RETAIL_MESSAGE_URL_KIA			"devautolink.gitauto.com/KiaLink_Fleet_ServerAPI"
//	#define	AU_HTTP_RETAIL_MESSAGE_PATH_KIA			"api/dcs/"
	//호주 기아 개발
	#define	AU_HTTP_RETAIL_DEV_MESSAGE_URL_KIA		"devautolink.gitauto.com/KiaLink_Fleet_ServerAPI"
//	#define	AU_HTTP_RETAIL_DEV_MESSAGE_PATH_KIA	"api/dcs/"
	//호주 기아 플릿 운영
	#define	AU_HTTP_FLEET_MESSAGE_URL_KIA				"devautolink.gitauto.com/KiaLink_Fleet_ServerAPI"
//	#define	AU_HTTP_FLEET_MESSAGE_PATH_KIA			"api/dcs/"
	//호주 기아 플릿 개발
	#define	AU_HTTP_FLEET_DEV_MESSAGE_URL_KIA		"devautolink.gitauto.com/KiaLink_Fleet_ServerAPI"
//	#define	AU_HTTP_FLEET_DEV_MESSAGE_PATH_KIA	"api/dcs/"
	//국내 기아 플릿 개발
	#define	KR_HTTP_FLEET_DEV_MESSAGE_URL_KIA		"devautolink.gitauto.com/KiaLink_Fleet_ServerAPI"
//	#define	KR_HTTP_FLEET_DEV_MESSAGE_PATH_KIA	"api/dcs/"
	//베트남
	
	#define	VN_HTTP_RETAIL_MESSAGE_URL_KIA		"link.thaco.com.vn" 
	#define	VN_HTTP_RETAIL_DEV_MESSAGE_URL_KIA	"linktest.thaco.com.vn" 
	//싱가폴
	#define	SG_HTTP_RETAIL_MESSAGE_URL_KIA		 "kialink.cyclecarriage.com"
	#define	SG_HTTP_RETAIL_DEV_MESSAGE_URL_KIA	 "uat-kialink.cyclecarriage.com"
    #define	SG_HTTP_DEV_MESSAGE_URL_KIA	 "101.1.36.39"

	//싱가포르 파주 개발 
	#define	SG_HTTP_RETAIL_DEV2_MESSAGE_URL_KIA "devksa.gitauto.com:8443"

	#define	RU_HTTP_RETAIL_DEV_MESSAGE_URL_KIA  "devmykia.kia.ru"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // GIT FLEET
#if defined(GIT_FLEET)
    #define KR_HTTP_FLEET_MESSAGE_URL_GIT       "devautolink.gitauto.com/AutoLInk_Fleet_Advance/ServerAPI"
#endif
#if defined(QA_FIFA)
	#define QA_HTTP_RETAIL_DEV_MESSAGE_URL      "hyundaifms-qat.com"
#endif
#else
	// use korea server
	#define	KR_HTTP_RETAIL_MESSAGE_URL			"autolinkapi-premium.hmca.com.au"//"autolink.gitauto.com"
	#define	KR_HTTP_RETAIL_MESSAGE_PATH			"api/dcs/"//"Australia_Premium/ServerAPI/api/dcs/"
	#define	KR_HTTP_RETAIL_DEV_MESSAGE_URL		"devautolinkapi-premium.hmca.com.au"//"autolink.gitauto.com"
	#define	KR_HTTP_RETAIL_DEV_MESSAGE_PATH		"api/dcs/"//"Australia_Premium/ServerAPI/api/dcs/"
	// user australia server
	#define	AU_HTTP_RETAIL_MESSAGE_URL			"autolinkapi-premium.hmca.com.au"
	#define	AU_HTTP_RETAIL_MESSAGE_PATH			"api/dcs/"
	#define	AU_HTTP_RETAIL_DEV_MESSAGE_URL		"devautolinkapi-premium.hmca.com.au"
	#define	AU_HTTP_RETAIL_DEV_MESSAGE_PATH		"api/dcs/"
	#define	AU_HTTP_RETAIL_MESSAGE_URL_KIA			"autolinkapi-premium.hmca.com.au"
	#define	AU_HTTP_RETAIL_MESSAGE_PATH_KIA			"api/dcs/"
	#define	AU_HTTP_RETAIL_DEV_MESSAGE_URL_KIA		"devautolinkapi-premium.hmca.com.au"
	#define	AU_HTTP_RETAIL_DEV_MESSAGE_PATH_KIA	"api/dcs/"
	// user newziland server
	#define	NZ_HTTP_RETAIL_MESSAGE_URL         "apiautolink-premium.hyundai.co.nz"
	#define	NZ_HTTP_RETAIL_MESSAGE_PATH			"api/dcs/"
	#define	NZ_HTTP_RETAIL_DEV_MESSAGE_URL         "devautolinkapi-premium.hmca.com.au"
	#define	NZ_HTTP_RETAIL_DEV_MESSAGE_PATH			"api/dcs/"

	// fleet information
	//https://fleet-premium-api.hmca.com.au/toss
	//dev url information
	//https://fleet-premium-api-dev.hmca.com.au/toss
	#define	HTTP_FLEET_MESSAGE_URL			"fleet-premium-api.hmca.com.au"
	#define	HTTP_FLEET_MESSAGE_PATH			"api/dcs/"

	#define	HTTP_FLEET_DEV_MESSAGE_URL		"fleet-premium-api-dev.hmca.com.au"
	#define	HTTP_FLEET_DEV_MESSAGE_PATH		"api/dcs/"
#endif
// ublox apgs server
//http://offline-live1.services.u-blox.com/GetOfflineData.ashx?token=_e_O3Zx1e0y8kwzFuy-Inw;gnss=gps;period=1;resolution=1;days=14;
#if defined(USE_UBLOX_GPS)
#define	HTTP_AGPS_MESSAGE_URL			"offline-live1.services.u-blox.com"
#define	HTTP_AGPS_MESSAGE_PATH			"GetOfflineData.ashx?token=_e_O3Zx1e0y8kwzFuy-Inw&gnss=gps&period=2&resolution=1&days=1;"
#else	//Not USE_UBLOX_GPS
#define	HTTP_AGPS_MESSAGE_URL			"xtrapath4.izatcloud.net"
#define	HTTP_AGPS_MESSAGE_PATH			"xtra2.bin"
#endif	//USE_UBLOX_GPS


#define MAX_CONTROL_REQUEST_FROM_SERVER_BUFFER_LENGTH				2048+128

#define MAX_ERROR_MESSAGE_BUFFER_COUNT				15
#define MAX_ERROR_MESSAGE_BUFFER_LENGTH				512

//************************************************************************************************

typedef enum _eMESSAGE_TYPE
{
	eMESSAGE_TYPE_NONE 							= 0,
	eMESSAGE_APGS_NONE							= eMESSAGE_TYPE_NONE,
	eMESSAGE_APGS_DATA							= 1,
	eMESSAGE_TYPE_PERIOD_INFORMATION 			= 10,		/* 운행중 정보 */  /* use this */
	eMESSAGE_TYPE_TRIP_REPORTING 				= 20,		/* 주행완료 후 보고 */ /* Trip */  /* use this */
	eMESSAGE_TYPE_ALARM_EVENT					= 30,		/* 차량 상태 이벤트 */  /* Use this */
	eMESSAGE_TYPE_REMOTE_CONTROL_REPORT	 		= 40,		/* 원격제어 결과 보고 */
	eMESSAGE_TYPE_REMOTE_CONTROL_REQUEST	 	= 41,		/* 원격제어 요청 */
	eMESSAGE_TYPE_CURRENT_VEHICLE_STATUS 		= 70,		/* 현재 차량의 정보 요청 */
	eMESSAGE_TYPE_ALARM_MASKING 				= 51,		/* 알람 마스킹 */ /* wakeup */
	eMESSAGE_TYPE_ENGINE_START	 				= 50,		/* 주행전 보고 */
	eMESSAGE_TYPE_DTC_ERROR 					= 80,		/* DTC 고장 */
	eMESSAGE_TYPE_PERIOD_INFORMATION_SLEEP		= 60,		/* 운행 정지 중 정보(시동 OFF) */
    eMESSAGE_TYPE_SETTING_GEOFENCE				= 65,		// Geofence setting
    eMESSAGE_TYPE_SETTING_POLYGON_GEOFENCE		= 66,		// Polygon Geofence setting
    eMESSAGE_TYPE_SMS_TEST				        = 39,		// sms test event that used at app install.
    eMESSAGE_TYPE_RESPONSE_MODEM_ACTIVATE       = 90,       // modem activate response
    eMESSAGE_TYPE_REQUEST_MODEM_ACTIVATE        = 91,       // modem activate request
    eMESSAGE_TYPE_RESPONSE_SENSOR_INITIALIZE    = 92,       // sensor initalize response
    eMESSAGE_TYPE_REQUEST_SENSOR_INITIALIZE     = 93,       // sensor initalize request
    eMESSAGE_TYPE_IPEK_PHASE1                   = 191,      // ipek phase1
    eMESSAGE_TYPE_IPEK_PHASE2                   = 192,      // ipek phase2
#if defined(PROTOCOL17)
	eMESSAGE_TYPE_REQUEST_SETURL         = 45,
	eMESSAGE_TYPE_RESPONSE_SETURL			= 46,
	eMESSAGE_TYPE_RESPONSE_SETURL_COMPLETE = 81,
	eMESSAGE_TYPE_RESPONSE_INITURL_COMPLETE = 82,
#endif
//#if defined(PROTOCOL18)
	eMESSAGE_TYPE_BT_REMOTE_CONTROL_REPORT = 44, //dahae
//#endif
	eMESSAGE_TYPE_TRACKING_REQUEST = 85,
	eMESSAGE_TYPE_TRACKING_RESPONSE = 86,
	eMESSAGE_TYPE_TRACKING_DATA = 87,
#if defined(PROTOCOL18)
	eMESSAGE_TYPE_REQUEST_RESERVATION_ENGINE_CONTROL_SETTING = 95,
	eMESSAGE_TYPE_RESPONSE_RESERVATION_ENGINE_CONTROL_SETTING = 96,
	eMESSAGE_TYPE_RESPONSE_RESERVATION_ENGINE_CONTROL_RESULT = 97,
	eMESSAGE_TYPE_CHARGING_REPORT = 98,
#endif
#if defined(PROTOCOL24)
	eMESSAGE_TYPE_MODEM_STATUS_REPORT       = 33,
#endif
#if defined(PROTOCOL25)
	eMESSAGE_TYPE_INSTALLATION_NETWORK_CHECK_TEST         = 49,
	eMESSAGE_TYPE_INSTALLATION_SMS_CHECK_TEST         = 59,
#endif

} eMESSAGE_TYPE;

typedef enum _eMESSAGE_RESULT_CODES
{
	eMESSAGE_RESULT_CODE_NONE = 0,
	eMESSAGE_RESULT_CODE_SUCCESS,
	eMESSAGE_RESULT_CODE_FAIL_SERVER_MESSAGE,
	eMESSAGE_RESULT_CODE_NOT_NWETWORK_REGISTERED,
	eMESSAGE_RESULT_CODE_ERROR_HTTP_COMM_FAIL,
	eMESSAGE_RESULT_CODE_CME_ERROR,
	eMESSAGE_RESULT_CODE_NO_RESP,
	eMESSAGE_RESULT_CODE_MODEM_CMD_FAIL,
	eMESSAGE_RESULT_CODE_MODEM_WARNING,
	eMESSAGE_RESULT_CODE_MODEM_LOW_SIGNAL,
	eMESSAGE_RESULT_CODE_SERVER_ABORTED,
	eMESSAGE_RESULT_CODE_SERVER_NOT_STABLE,
	eMESSAGE_RESULT_CODE_SERVER_NOT_FOUND,
	eMESSAGE_RESULT_CODE_WAIT,
	eMESSAGE_RESULT_CODE_DAMO_ERROR,
} eMESSAGE_RESULT_CODES;
/*
typedef enum _eAPGS_TYPE
{
    eMESSAGE_APGS_NONE = 0,
    eMESSAGE_APGS_DATA = 1,
}eAPGS_TYPE;
*/
/*			< Alarm Event define >
eventKey    eventName   					eventValue
======================================================================================
1			Over battery            0: under 15V			1: over 16.5V
2			Low battery				0: over 12V				1: 10.5V
3			Door Lock				0: lock						1: open
4			Door Open				0: all closed			1: no all closed
5			engine Temperature		0: under 95'c			1: over 115'c
6			Fuel urn out			0: over 20%				1: under 10%
7			Tire pressure			0: over 30psi			2: under 25psi
8			Over Speed				0: under 150km/h	    1: over 160km/h
9			Over RPM				0: under 4500rpm	    1: over 5000rpm
10
11			engine Start			0: off					1: on
12			tail Lamp				0: off					1: on
13
14			impacting				0: off					1: on
15			MIL Lamp				0: off					1: on
16			fota					version 				"use InformFotaCompleteToServer()"
*/

typedef enum _eMESSAGE_EVENT_KEY
{
	eMESSAGE_EVENT_KEY_NONE								= 0,
	eMESSAGE_EVENT_KEY_OVER_VOLTAGE_ALARM 				= 1,
	eMESSAGE_EVENT_KEY_LOW_VOLTAGE_ALARM 				= 2,
	eMESSAGE_EVENT_KEY_DOOR_LOCK_ALARM 					= 3,
	eMESSAGE_EVENT_KEY_DOOR_OPEN_ALARM 					= 4,
	eMESSAGE_EVENT_KEY_OVER_ENGINE_TEMP_ALARM 			= 5,
	eMESSAGE_EVENT_KEY_FUEL_RUN_OUT 					= 6,
	eMESSAGE_EVENT_KEY_TIRE_PRESSURE 					= 7, /* TPMS */
	eMESSAGE_EVENT_KEY_VEHICLE_OVER_SPEED_ALARM 		= 8,
	eMESSAGE_EVENT_KEY_VEHICLE_OVER_RPM_ALARM 			= 9,
	eMESSAGE_EVENT_KEY_GEO_FENCE_ALRAM                  = 10,
	eMESSAGE_EVENT_KEY_VEHICLE_ENGINE_START_ALARM 	    = 11, /* 11 */
	eMESSAGE_EVENT_KEY_TAIL_LAMP_ALARM 					= 12, /* 12 */
	eMESSAGE_EVENT_KEY_PARKING_IMPACT					= 13, /* 14 */
	eMESSAGE_EVENT_KEY_MIL_LAMP_ON 						= 14, /* 15 */
	eMESSAGE_EVENT_KEY_FOTA_COMPLETE_ALRAM 				= 15, /* 16 */
	eMESSAGE_EVENT_KEY_AIRBAG_ALRAM                     = 16,
    eMESSAGE_EVENT_KEY_TOWING_ALRAM                     = 17,
    eMESSAGE_EVENT_KEY_VALET_ALRAM                      = 18,
	eMESSAGE_EVENT_KEY_ENGKEEP_TIMEOVER_ALRAM           = 19,
	eMESSAGE_EVENT_KEY_MODEM_POWER_OFF                  = 20,
	eMESSAGE_EVENT_KEY_INDICATOR_ALRAM                  = 21,
	eMESSAGE_EVENT_KEY_CHARGE_ALRAM                     = 22,
	eMESSAGE_EVENT_KEY_SMSEXPIRE_ALRAM                  = 23,
	eMESSAGE_EVENT_KEY_STATE_ALRAM                      = 24,
	eMESSAGE_EVENT_KEY_REARSEAT_ALRAM                   = 25,
	eMESSAGE_EVENT_KEY_EXTRA_CHARGE_TIME                = 26,
	eMESSAGE_EVENT_KEY_CHARGING_STATE                   = 27,
	eMESSAGE_EVENT_KEY_FAHRENHEIT_ALRAM					= 29,
	eMESSAGE_EVENT_KEY_SECURITY_ALRAM                   = 30,
	eMESSAGE_EVENT_KEY_FOB_STATUS_ALRAM                 = 31,
	eMESSAGE_EVENT_KEY_ABSINDICATOR_ALRAM               = 32,
	eMESSAGE_EVENT_KEY_EPBINDICATOR_ALRAM               = 33,
	eMESSAGE_EVENT_KEY_ISGINDICATOR_ALRAM               = 34,
	eMESSAGE_EVENT_KEY_BMSINDICATOR_ALRAM               = 35,
	eMESSAGE_EVENT_KEY_BRAKEJUDDERINDICATOR_ALRAM       = 36,
	eMESSAGE_EVENT_KEY_ENGOILINDICATOR_ALRAM            = 37,
	eMESSAGE_EVENT_KEY_CANFD_CANNOT_COMMUNICATION       = 40,   //210711 PDH CANFD 통신불가시 알람
    eMESSAGE_EVENT_KEY_MAX,
} eMESSAGE_EVENT_KEY;

typedef enum _eMESSAGE_REMOTE_CONTROL_STATE
{
	eMESSAGE_REMOTE_CONTROL_STATE_NONE = 0,
	eMESSAGE_REMOTE_CONTROL_STATE_OCCURE,
	eMESSAGE_REMOTE_CONTROL_STATE_PROCESS,
	eMESSAGE_REMOTE_CONTROL_STATE_WAITING,
	eMESSAGE_REMOTE_CONTROL_STATE_FAIL,
	eMESSAGE_REMOTE_CONTROL_STATE_SUCCESS,
} eMESSAGE_REMOTE_CONTROL_STATE;

typedef enum _eREMOTE_CON_CMD_TYPE
{
	eREMOTE_CON_CMD_TYPE_NONE = 0,
	eREMOTE_CON_CMD_TYPE_STARTING,
	eREMOTE_CON_CMD_TYPE_DOOR,
	eREMOTE_CON_CMD_TYPE_FOTA,
	eREMOTE_CON_CMD_TYPE_EMERGENCY_LIGHT,
	eREMOTE_CON_CMD_TYPE_HORN_EMERGENCY_LIGHT, //5
	eREMOTE_CON_CMD_TYPE_RESET,
	eREMOTE_CON_CMD_TYPE_GEO_FENCE,
	eREMOTE_CON_CMD_TYPE_SENSOR_SENSITIVITY,
	eREMOTE_CON_CMD_TYPE_VALET,
	eREMOTE_CON_CMD_TYPE_DTC_STATUS, //10
	eREMOTE_CON_CMD_TYPE_TOWING,
	eREMOTE_CON_CMD_TYPE_GUARD,
	eREMOTE_CON_CMD_TYPE_DATA,
	eREMOTE_CON_CMD_TYPE_MODEM_ACTIVE,// new odometer activation
	eREMOTE_CON_CMD_TYPE_SERVICE_TYPE,// 15
	eREMOTE_CON_CMD_TYPE_ENCRYPT_TYPE,
	eREMOTE_CON_CMD_TYPE_POLYGON_GEO_FENCE,
	eREMOTE_CON_CMD_TYPE_SENSOR_INITIALIZE,
#if defined(PROTOCOL17)
#endif
#if defined(PROTOCOL18)
	eREMOTE_CON_CMD_TYPE_SETTING_RSV_ENG_CTRL,
#endif
#if defined(FEATURE_EXTENSION_BOARD)
	eREMOTE_CON_CMD_TYPE_SETTING_ANTI_THIEF, // 20
#endif


} eREMOTE_CON_CMD_TYPE;

typedef enum _eREMOTE_CON_BOUNDTYPE
{
	eREMOTE_CON_BOUNDTYPE_NONE = 0,
	eREMOTE_CON_BOUNDTYPE_IN,
	eREMOTE_CON_BOUNDTYPE_OUT,
	eREMOTE_CON_BOUNDTYPE_BOTH,
} eREMOTE_CON_BOUNDTYPE;

typedef enum _eREMOTE_CON_FOTA_TYPE
{
    eREMOTE_CON_FOTA_TYPE_UPDATE = 1,
    eREMOTE_CON_FOTA_TYPE_TEST = 2,
    eREMOTE_CON_FOTA_TYPE_BOOT = 3,
}eREMOTE_CON_FOTA_TYPE;

typedef enum _eREMOTE_CON_RESULT
{
	eREMOTE_CON_RESULT_FAIL = 0,
	eREMOTE_CON_RESULT_SUCCESS,
	eREMOTE_CON_RESULT_WAIT,
} eREMOTE_CON_RESULT;

typedef __packed struct {
	/* manager state */
	eMESSAGE_RESULT_CODES eMessageCommResultCode;
	eMESSAGE_REMOTE_CONTROL_STATE bReceivedRequestRemoteMessageFlag;

	uint16_t nErrorCodeFromMessageServer;		// event message 송수신 시, 서버로 부터 수신 받은 error code
    uint16_t nErrorCode;

	int iTimer_MSG_Common_Dly;

    uint8_t aRemoteControlGUID[MAX_GUID_LENGTH];

	uint8_t aSaveDataTimeStampForError[20];
	uint8_t aErrMsgFileName[64];
} MESSAGE_MANAGER_DATA;

typedef __packed struct {
	/* 서버로 부터 수신 받은 제어 명령 */
	eREMOTE_CON_CMD_TYPE eCommandType;						// 명령 구분
	uint8_t bControlType;						// 제어 구분
	uint8_t bStartUpTime;						// 시동 유지 시간
	uint16_t nTemperature;					// 온도
	uint8_t unCheckTemperature;
	bool bRemoveFrost;							// 성에제거 유무
#if defined(PROTOCOL13)
	bool bRearDefogger;                     // rear defogger
	bool bHighBeam;                         // head lamp
#endif //#if defined(PROTOCOL13)
	float fLatitude;							// 위도
	float fLongitude;						// 경도
	eREMOTE_CON_BOUNDTYPE eBoundType;							// 바운드 타입
	uint32_t unDistance;							// 거리
	uint32_t unOccurredDateTime; // 20171221112233
	uint8_t guid[32];
#if defined(PROTOCOL18)
	uint8_t BTControlKey[16];
#endif
} REMOTE_COMMAND;

extern MESSAGE_MANAGER_DATA MessageManagerData;
extern REMOTE_COMMAND stRemoteCommand;

void InitializeMessageManager(void);

eGitFresult MSG_WriteErrorFile(char *pData, uint16_t nLen);
eGitFresult MSG_ReadErrorFile(void);
eGitFresult MSG_DelErrorFile(void);
eGitFresult MSG_DelAllErrorFile(void);

//FRESULT MSG_WriteErrorFile(char *pData, uint16_t nLen);
//FRESULT MSG_ReadErrorFile(void);
//FRESULT MSG_DelErrorFile(void);
//FRESULT MSG_DelAllErrorFile(void);

extern void ConvertTime_LocalToUTC(stHalRTC_TimeTypeDef stLocalTime, stHalRTC_DateTypeDef stLocalDate, stHalRTC_TimeTypeDef *pUTCTime, stHalRTC_DateTypeDef *pUTCDate);
#if defined(PROTOCOL17)
void ParseSetURLMessage(stMsgSysMsg* pstMsgSysMsg);
#endif
char* itoa_1(long long val, char *buf, int radix);

#endif /* __MESSAGE_MANAGER_H__ */

/***************************** END OF FILE ****/
