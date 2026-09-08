/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __OBD_MANAGER_H__
#define __OBD_MANAGER_H__

#include "main.h"
#include "MngQueue.h"
#include "AutolinkConfig.h"
#include "Message_Manager.h"
#include "AutolinkConfiguration.h"
#include "GIT_PassthruDefines.h"
#include "OBD_Controller.h"

//#define DRIVE_TEST		//확장보드없이 실차 검증시 해당 define만 풀면 된다

#define STATISTICS		//통계화면에서 사용되는 data로 GCS 출시 이후에 추가개발된 사항들..  GCS도 같이 적용된다
#define FCS_MODE
#define CARB_ODO
//#define		 FINE_OBD

#define INVALID_INDEX									255

#define DCS_GEAR_POS						"A01"		//기어정보(단수)
#define DCS_GEAR_LEVER						"A02"		//기어정보(레버위치)
#define DCS_OILTEMP_TCU						"A03"		//미션오일온도
#define DCS_GEAR_SBW_P						"A04"		//SBW P 단
#define DCS_GEAR_SBW_R						"A05"		//SBW R 단
#define DCS_GEAR_SBW_N						"A06"		//SBW N 단
#define DCS_GEAR_SBW_D						"A07"		//SBW D단
#define DCS_GEAR_SBW_B						"A08"		//SBW B단


#define DCS_ODOMETER_SUPPORTED				"CP0"		//주행거리(odometer) Supported Check Code
#define DCS_ODOMETER						"C01"		//주행거리(odometer)
#define DCS_ODOMETER_SUB					"C02"		//주행거리(odometer) - mile단위(HI)
#define DCS_FUEL_EFFICIENCY_K				"C03"		//순간연비 - km/l
#define DCS_FUEL_EFFICIENCY_M				"C04"		//순간연비 - mi/l
#define DCS_AVR_FUEL_EFFICIENCY_K			"C05"		//평균연비 - km/l
#define DCS_AVR_FUEL_EFFICIENCY_M			"C06"		//평균연비 - mi/l
#define DCS_DRIVING_DISTANCE_K				"C07"		//주행가능거리 - km/l
#define DCS_DRIVING_DISTANCE_M				"C08"		//주행가능거리 - mi/l
#define DCS_DRIVING_TIME					"C09"		//주행시간
#define DCS_DISTANCE_CLEAR_DTC				"C0C"		//Carb - 고장코드소거후주행거리


#define DCS_BATTERY_V						"E01"		//배터리전압
#define DCS_RPM								"E02"		//RPM
#define DCS_FUEL_LEVEL						"E03"		//연료잔량
#define DCS_SPEED							"E04"		//차속
#define DCS_MIL_LAMP						"E05"		//엔진경고등
#define DCS_FUELCUT							"E06"		//퓨얼컷
#define DCS_LOWBATTERY_SOC					"E07"		//배터리충전상태
#define DCS_BATTERY_HEALTH					"E08"		//배터리노화진행률
#define DCS_BATTERY_CAPACITY				"E09"		//배터리용량
#define DCS_BATTERY_CURRENT					"E0A"		//배터리전류
#define DCS_ACCEL_POS						"E0B"		//가속페달위치
#define DCS_FUEL_CONSUM						"E0C"		//연료소모량
#define DCS_BRAKE_SW						"E0D"		//브레이크페달스위치
#define DCS_COOLANT_TEMP					"E0E"		//냉각수온
#define DCS_THROTTLE_CLOSE					"E0F"		//THROTTLE 완전닫힘 상태
#define DCS_CRUIES							"E10"		//크루즈컨트롤상태
#define DCS_ISG								"E11"		//ISG 상태
#define DCS_HOOD							"E12"		//보닛 상태
#define DCS_ENGINEOIL_TEMP					"E13"    	//엔진오일온도
#define DCS_KEY_STATUS						"E14"      	//KEY STATUS
#define DCS_ENGINE_LOAD						"E15"      	//엔진부하(%)
#define DCS_INJECTOR_QUANTITY				"E16"       //인젝터 분사량
#define DCS_MAIN_QUANTITY  					"E17"       //메인 분사량
#define DCS_PILOT1_QUANTITY  				"E18"       //파일럿1 분사량
#define DCS_PILOT2_QUANTITY  				"E19"       //파일럿2 분사량
#define DCS_PILOT3_QUANTITY  				"E20"       //파일럿3 분사량
#define DCS_POST1_QUANTITY  				"E21"       //포스트1 분사량
#define DCS_POST2_QUANTITY  				"E22"       //포스트2 분사량
#define DCS_AIRCON_TEMPERATURE				"E26"       //DCS_AIRCON_TEMPERATURE
#define DCS_VENT_STATUS  					"E27"       //VENT STATUS
#define DCS_FATC_STATUS						"E28"		//FATC ON/OFF 상태
#define DCS_OUTSIDE_TEMPERATURE				"E29"    	//외기온도
#define DCS_INSIDE_TEMPERATURE				"E2A"		//내기 온도
#define DCS_ATMOSPHERIC_PRESS				"E2B"		//대기압
#define DCS_FAHRENHEIT_TEMP					"E2C"		//Fahrenheit Temp
#define DCS_CELSIUS							"E2D"		//Celsius
#define DCS_FAHRENHEIT						"E2E"		//Fahrenheit
#define DCS_ABS_INDICATOR					"E34"		//ABS 경고등
#define DCS_ISG_INDICATOR					"E35"		//ISG 경고등
#define DCS_EPB_INDICATOR					"E36"		//EPB 경고등
#define DCS_BRAKEJUDDER_VALUE				"E37"		//BRAKE JUDDER Value
#define DCS_BMS_INDICATOR					"E38"		//BMS 경고등
#define DCS_ENGOILPRESS_INDICATOR			"E39"		//ENG OIL PRESS 경고등


#define DCS_DOOR_RL_OPEN					"S01"		//뒤좌측도어
#define DCS_DOOR_RR_OPEN					"S02"		//뒤우측도어
#define DCS_DOOR_FR_OPEN					"S03"		//FR 도어
#define DCS_DOOR_FL_OPEN					"S04"		//FL 도어
#define DCS_HEADLAMP						"S05"		//전조등
#define DCS_TRUNK							"S06"		//트렁크
#define DCS_WEAK_LAMP						"S07"		//미등
#define DCS_DOOR_RL_UNLOCK					"S08"		//뒤 좌측 언락
#define DCS_DOOR_RR_UNLOCK					"S09"		//뒤우측 언락
#define DCS_DOOR_FR_UNLOCK					"S0A"		//FR 언락
#define DCS_DOOR_FL_UNLOCK					"S0B"		//FL 언락
#define DCS_FOG_LAMP						"S0C"		//안개등
#define DCS_BRAKE_LAMP						"S0D"		//정지등
#define DCS_HIGHLIGHT_LAMP					"S0E"		//상향등
#define DCS_HAZARD_LAMP						"S0F"		//비상등
#define DCS_WIPER_STATUS					"S10"		//와이퍼 정지 상태
#define DCS_TURN_RIGHT_LAMP					"S11"		//우측방향지시등
#define DCS_TURN_LEFT_LAMP					"S12"		//좌측방향지시등


#define DCS_TIRE_PRESS_FL					"T01"		//TPMS_타이어공기압(앞좌)
#define DCS_TIRE_PRESS_FR					"T02"		//TPMS_타이어공기압(앞우)
#define DCS_TIRE_PRESS_RR					"T03"		//TPMS_타이어공기압(뒤우)
#define DCS_TIRE_PRESS_RL					"T04"		//TPMS_타이어공기압(뒤좌)
#define DCS_TIRE_TEMP_FL					"T05"		//TPMS_타이어공기온도(앞좌)
#define DCS_TIRE_TEMP_FR					"T06"		//TPMS_타이어공기온도(앞우)
#define DCS_TIRE_TEMP_RR					"T07"		//TPMS_타이어공기온도(뒤우)
#define DCS_TIRE_TEMP_RL					"T08"		//TPMS_타이어공기온도(뒤좌)
#define DCS_TIRE_CONVERT					"T09"		//TPMS 단위

#if 0
#define DCS_PURGE_OC_SENSOR					"F01"		//퍼지밸브 개폐감지 센서 이상
#define DCS_HYDROGEN_VALVE					"F02"		//수소공급밸브 이상
#define DCS_PURGE_VALVE						"F03"		//퍼지밸브 이상
#define DCS_DRAIN_VALVE						"F04"		//드레인밸브 이상
#define DCS_KEY_START						"F08"		//키 스타트
#endif

#define DCS_FUEL_CELL_START_STOP_STATE		"F01"		//수소 FC Start Stop State
#define DCS_FUEL_CELL_CTRL_STATE			"F02"		//수소 Fuel Cell Control State
#define DCS_FUEL_CELL_CURRENT				"F03"		//수소 Fuel Cell Current
#define DCS_STACK_TOTAL_VOLTAGE_SUMMATION	"F04"		//수소 Fuel Cell
#define DCS_HYDROGEN_CUR_PRES				"F05"		//수소탱크 현재 압력값
#define DCS_HYDROGEN_CUR_TEMP				"F06"		//수소탱크 현재 온도값
#define DCS_HYDROGEN_CUR_STOR				"F07"		//수소 탱크 연료 저장량
#define DCS_AIR_BLOWER_RPM_COMMAND			"F08"		//수소
#define DCS_HYDROGEN_LEAK					"F10"		//수소리크 감지 신호
#define DCS_HYDROGEN_TANK_PRES				"F11"		//수소탱크 압력
#define DCS_HYDROGEN_CHARGE_CNT         	"F12"       //수소탱크 연료 주입 횟수
#define DCS_H2_CONSUM						"F16"   	// 수소 순간 연료사용량
#define DCS_FUELTERM_PERCENT				"F21"       //Fuel Term
#define DCS_MAP_SENSOR   	      	 		"F30"       //흡기압
#define DCS_IAT_SENSOR  	           		"F60"       //흡기온도
#define DCS_LAMBDA_SENSOR   	   		 	"F20"       //람다
#define DCS_MAF_SENSOR	   	       			"F31"       //흡입공기량
#define DCS_BATTERY_CELL_MIN				"F32"       // BATTERY CELL VOLTAGE MIN
#define DCS_BATTERY_CELL_MAX				"F33"       // BATTERY CELL VOLTAGE MAX
#define DCS_HYDROGEN_FUEL_LEVEL				"F36"		//수소 연료 레벨 %
#define DCS_AIR_PURIFICATION				"F37"		//공기 정화량 KL
#define DCS_CO2_REDUCTION					"F38"		//CO2 감축량 Kg


#define DCS_MOTOR_REVOL						"H01"     	//모터 회전수
#define DCS_HYDROGEN_VOLUME					"H02"		//수소탱크 부피


#define DCS_SOC_STATE						"B01"     	//SOC상태 패터리팩 잔량
#define DCS_BATTERY_PACK_C					"B02"     	//배터리 팩 전류
#define DCS_BATTERY_PACK_V					"B03"     	//배터리 팩 전압
#define DCS_CHARGE_STATE					"B08"     	//충전커넥터체결상태
#define DCS_POWER_ACC						"B09"     	//ACC전원 상태
#define DCS_POWER_IG1						"B0A"     	//IG1전원 상태
#define DCS_POWER_IG2						"B0B"     	//IG2전원 상태
#define DCS_POWER_READY						"B0C"     	//EV READY 상태
#define DCS_CRASH_SIGNAL  					"B0D"       //충돌신호
#define DCS_HORN_STATUS  					"B0F"       //HORN STATUS
#define DCS_USER_CTRL_STATUS				"B10"       //사용자 권한이동
#define DCS_REAR_DEFOG						"B18"		//RearDefog 상태
#define DCS_POWER_IG3						"B1D"     	//IG3전원 상태
#define DCS_SLOPE_TCU						"B1E"		//PCU의 경사도
#define DCS_SOH_STATE						"B20"     	//SOH상태 배터리팩 노화진행률
#define DCS_CHARGE_COUNT					"B21"     	//완속충전수행적산횟수
#define DCS_CHARGE_TIME						"B22"     	//완속충전수행적산시간
#define DCS_EXTRA_DISTANCE					"B23"     	//사용가능주행거리
#define DCS_HIGHBATT_TEMP					"B24"     	//고전압배터리온도 MIN
#define DCS_EXTRA_CHARGE_TIME				"B25"		//남은충전시간
#if defined(PROTOCOL18)
#define DCS_CHARGING_STATE              	"B28"       //충전중 상태
#define DCS_INSULATION_RESISTANCE			"B2A"       //절연 저항 
#define DCS_HIGHBATT_TEMP_MAX               "B30"     	// 고전압배터리온도 MAX //dahae
#endif
#define DCS_REAR_SEAT_SET_STATE				"B31"		//후석 승객 알람 설정여부
#define DCS_REAR_SEAT_OCCURED				"B32"		//후석 알람
#define DCS_BATTERY_CELL_VOL_MAX            "B36"       //배터리 셀 전압 MAX
#define DCS_BATTERY_CELL_VOL_MIN			"B37"       //배터리 셀 전압 MIN
#if defined(PROTOCOL21)
#define DCS_ENG_OILLIFE_RATIO               "E31"       //엔진오일 수명 잔량
#define DCS_ENG_OILLIFE_ENA                 "E32"       //엔진오일 센서 데이터 지원 여부
#define DCS_ENG_OILLIFE_WARN                "E33"       //엔진오일 교체 알림
#endif 


#if 0
#define DCS_ACU1										"U01"		//ACU
#define DCS_ACU2										"U02"		//ACU
#define DCS_ACU3										"U03"		//ACU
#define DCS_ACU4										"U04"		//ACU
#endif


#define DCS_WARNNING_LIST						"IND"		//DB개발자 편의상 추가
#define DCS_4WD_IND_0							"!01"
#define DCS_4WD_IND_1							"!02"
#define DCS_4WD_IND_2							"!03"
#define DCS_4WD_IND_3							"!04"
#define DCS_4WD_IND_4							"!05"
#define DCS_4WD_IND_5							"!06"
#define DCS_4WD_IND_6							"!07"
#define DCS_4WD_IND_7							"!08"


#define DCS_ABS_IND_0							"!10"
#define DCS_ABS_IND_1							"!11"


#define DCS_ACU_IND_0							"!20"
#define DCS_ACU_IND_1							"!21"
#define DCS_ACU_IND_2							"!22"
#define DCS_ACU_IND_3							"!23"
#define DCS_ACU_IND_4							"!24"


#define DCS_AUTOHOLD_IND_0						"!30"
#define DCS_AUTOHOLD_IND_1						"!31"
#define DCS_AUTOHOLD_IND_2						"!32"


#define DCS_BATTCHARGE_IND_0					"!40"
#define DCS_BATTCHARGE_IND_1					"!41"


#define DCS_CHECKENGINE_IND_0					"!50"
#define DCS_CHECKENGINE_IND_1					"!51"
#define DCS_CHECKENGINE_IND_2					"!52"
#define DCS_CHECKENGINE_IND_3					"!53"


#define DCS_DBCWARNNING_IND						"!60"


#define DCS_DPFGPF_IND							"!70"


#define DCS_SCR_IND								"!80"


#define DCS_EPB_IND_0							"!90"
#define DCS_EPB_IND_1							"!91"


#define DCS_TCS_IND_0							"!A0"
#define DCS_TCS_IND_1							"!A1"


#define DCS_ISG_IND_0							"!B0"
#define DCS_ISG_IND_1							"!B1"
#define DCS_ISG_IND_2							"!B2"


#define DCS_MDPS_IND_0							"!C0"
#define DCS_MDPS_IND_1							"!C1"


#define DCS_OILLEVEL_IND						"!D0"


#define DCS_OILPRESS_IND						"!E0"


#define DCS_PARKINGBRAKE_IND_0					"!F0"
#define DCS_PARKINGBRAKE_IND_1					"!F1"
#define DCS_PARKINGBRAKE_IND_2					"!F2"


#define DCS_POWERDOWN_IND_0						"!G0"
#define DCS_POWERDOWN_IND_1						"!G1"
//#define DCS_POWERDOWN_IND_2					"!G2"


#define DCS_AHB_IND								"!H0"
//#define DCS_AHB_IND							"!H1"


#define DCS_HEV_IND_0							"!I0"
#define DCS_HEV_IND_1							"!I1"
#define DCS_HEV_IND_2							"!I2"
#define DCS_HEV_IND_3							"!I3"
#define DCS_HEV_IND_4							"!I4"
#define DCS_HEV_IND_5							"!I5"
#define DCS_HEV_IND_6							"!I6"
#define DCS_HEV_IND_7							"!I7"


#define DCS_EV_IND_0							"!J0"
#define DCS_EV_IND_1							"!J1"
#define DCS_EV_IND_2							"!J2"
#define DCS_EV_IND_3							"!J3"
#define DCS_EV_IND_4							"!J4"
#define DCS_EV_IND_5							"!J5"


#define DCS_FCEV_IND_0							"!K0"
#define DCS_FCEV_IND_1							"!K1"
#define DCS_FCEV_IND_2							"!K2"
#define DCS_FCEV_IND_3							"!K3"
#define DCS_FCEV_IND_4							"!K4"
//#define DCS_FCEV_IND_4						"!K5"

#define DCS_TPMS_IND_0							"!L0"
#define DCS_TPMS_IND_1							"!L1"
#define DCS_TPMS_IND_2							"!L2"



#define DCS_ODO_MILE_CONVERSION		1.609344
#define DCS_ODO_ERROR				300				//약 시속300km/h

#define DTC_DATA_MAX 				100
#define SLAVE_DATA_MAX  			120
#define SLAVE_DTC_DATA_MAX  		40
#define FREEZE_FRAME_SYS_CNT_MAX 	14
#define FREEZE_FRAME_CNT_MAX 	10
#define FREEZE_FRAME_DATA_MAX 	150
#define SLAVE_CURRENT_FIELD  		17
#define SLAVE_DTC_FIELD  			15
#define MASTER_DATA_MAX  			130
#define MASTER_DATA_FIELD  			6
#define MAX_ECU_ID_SIZE				4
#define MAX_REF_TABLE_SIZE			80
#define MAX_REF_INDEX_SIZE			30

#define	eParse_Index_IDX			0
#define	eParse_Request_IDX			1
#define	eParse_Response_IDX			2
#define	eParse_StartPos_IDX			3
#define	eParse_RealPos_IDX			4
#define	eParse_DataSize_IDX			5
#define	eParse_DataType_IDX			6
#define	eParse_Unit_IDX				7
#define	eParse_ConvRule_IDX			8
#define	eParse_A_IDX				9
#define	eParse_B_IDX				10
#define	eParse_C_IDX				11
#define	eParse_D_IDX				12
#define	eParse_E_IDX				13
#define	eParse_F_IDX				14
#define	eParse_LUT_IDX				15
#define	eParse_CommType_IDX			16
#define	eParse_NONE_IDX				17

#define	INDEX_REQ_FUNCTIONINDEX		0
#define	INDEX_REQ_WAKEUPUSED			1
#define	INDEX_REQ_RETRY					2
#define	INDEX_REQ_CANLINE				3
#define	INDEX_REQ_CANSPEED				4
#define	INDEX_REQ_REQVAL					5
#define	INDEX_REQ_RESVAL					6
#define	INDEX_REQ_LENGTH					7
#define	INDEX_REQ_DATA						8
#define	INDEX_REQ_TIMING					9
#define	INDEX_REQ_TIMES					10
#define	INDEX_REQ_MIN						11
#define	INDEX_REQ_MAX						12
#define	INDEX_REQ_CONV						13
#define	INDEX_REQ_FDCANLINE					14
#define	INDEX_REQ_FDCANSPEED				15
#define	INDEX_REQ_FDCANFRAME				16
#define	INDEX_REQ_TOTALCNT					17

#define	INDEX_CONV_FUNCTIONINDEX	0
#define	INDEX_CONV_SEPARATOR			1
#define	INDEX_CONV_ON						2
#define	INDEX_CONV_OFF						3
#define	INDEX_CONV_TOTALCNT			4

#define	INDEX_FREEZE_SYSTEM				0
#define	INDEX_FREEZE_ECUID					1
#define	INDEX_FREEZE_FUNCTIONINDEX	2
#define	INDEX_FREEZE_OPEN_REQ			3
#define	INDEX_FREEZE_OPEN_RES			4
#define	INDEX_FREEZE_FRAME_REQ			5
#define	INDEX_FREEZE_FRAME_RES			6
#define	INDEX_FREEZE_CLOSE_REQ			7
#define	INDEX_FREEZE_CLOSE_RES			8
#define	INDEX_FREEZE_TOTALCNT			9

#define	INDEX_CONFIG_AVN						0
#define	INDEX_CONFIG_AVN_VERSION		1
#define	INDEX_CONFIG_TYPE					2
#define	INDEX_CONFIG_TYPE_VALUE			3
#define	INDEX_CONFIG_ENGSTOP				4
#define	INDEX_CONFIG_ENGSTOP_VALUE	5
#define	INDEX_CONFIG_WAITTIME				6
#define	INDEX_CONFIG_WAITTIME_VALUE	7
#define	INDEX_CONFIG_TOTALCNT			8

#define	INDEX_RES_FUNCTIONINDEX		0
#define	INDEX_RES_CANLINE					1
#define	INDEX_RES_CANSPEED				2
#define	INDEX_RES_INDEX						3
#define	INDEX_RES_REQVAL					4
#define	INDEX_RES_RESVAL					5
#define	INDEX_RES_STARTPOS				6
#define	INDEX_RES_REALPOS				7
#define	INDEX_RES_DATASIZE				8
#define	INDEX_RES_MSBLSB					9
#define	INDEX_RES_MASKVAL				10
#define	INDEX_RES_CONVRULE				11
#define	INDEX_RES_A							12
#define	INDEX_RES_B							13
#define	INDEX_RES_C							14
#define	INDEX_RES_D							15
#define	INDEX_RES_E							16
#define	INDEX_RES_F							17
#define	INDEX_RES_LUT						18
#define	INDEX_RES_COMPARETYPE		19
#define	INDEX_RES_COMPAREVALUE		20
#define	INDEX_RES_FDCANLINE					21
#define	INDEX_RES_FDCANSPEED				22
#define	INDEX_RES_FDCANFRAME				23
#define	INDEX_RES_TOTALCNT					24

#define	INDEX_VEHICLESTATUS_FUNCTIONINDEX	0
#define	INDEX_VEHICLESTATUS_WAKEUPUSED		1
#define	INDEX_VEHICLESTATUS_CANLINE				2
#define	INDEX_VEHICLESTATUS_CANSPEED			3
#define	INDEX_VEHICLESTATUS_INDEX					4
#define	INDEX_VEHICLESTATUS_REQVAL				5
#define	INDEX_VEHICLESTATUS_RESVAL				6
#define	INDEX_VEHICLESTATUS_STARTPOS			7
#define	INDEX_VEHICLESTATUS_REALPOS				8
#define	INDEX_VEHICLESTATUS_DATASIZE			9
#define	INDEX_VEHICLESTATUS_MSBLSB				10
#define	INDEX_VEHICLESTATUS_MASKVAL				11
#define	INDEX_VEHICLESTATUS_CONVRULE			12
#define	INDEX_VEHICLESTATUS_A							13
#define	INDEX_VEHICLESTATUS_B							14
#define	INDEX_VEHICLESTATUS_C							15
#define	INDEX_VEHICLESTATUS_D							16
#define	INDEX_VEHICLESTATUS_E							17
#define	INDEX_VEHICLESTATUS_F							18
#define	INDEX_VEHICLESTATUS_LUT						19
#define	INDEX_VEHICLESTATUS_CANFDLINE					20
#define	INDEX_VEHICLESTATUS_CANFDSPEED					21
#define	INDEX_VEHICLESTATUS_CANFDFRAME					22
#define	INDEX_VEHICLESTATUS_TOTALCNT					23




//#define	INDEX_VEHICLESTATUS_COMPAREVALUE	20
//#define	INDEX_VEHICLESTATUS_USEFUNCTION		20

//dahae
#define INDEX_AIR_FUNCTIONINDEX              0
#define INDEX_AIR_VALUE1                     1
#define INDEX_AIR_VALUE2                     2
#define INDEX_AIR_VALUE3                     3
#define INDEX_AIR_CS                         4
#define INDEX_AIR_TOTALCNT                   5

#define INDEX_CTRL_TOTALCNT                  3
#define INDEX_CTRL_FUNCTIONINDEX             0
#define INDEX_CTRL_FUNCTION                  1
#define INDEX_CTRL_SUPPORT                   2

#define	eParse_DTC_ID_IDX			1
#define	eParse_DTC_INDEX_IDX		2
#define	eParse_DTC_RRQ_TIME_IDX		3
#define	eParse_DTC_SEARCH_TYPE_IDX	4
#define	eParse_DTC_STARTPOS_IDX		5
#define	eParse_DTC_READ_NO_IDX		6
#define	eParse_DTC_SKIP_NO_IDX		7
#define	eParse_DTC_MASKING_IDX		8
#define	eParse_DTC_OPENREQ_IDX		9
#define	eParse_DTC_OPENRES_IDX		10
#define	eParse_DTC_REQ_IDX			11
#define	eParse_DTC_RES_IDX			12
#define	eParse_DTC_CLOSEREQ_IDX		13
#define	eParse_DTC_CLOSERES_IDX		14

#define	eParse_DISPLACE_NONE_IDX				0
#define	eParse_DISPLACE_FUELTYPE_IDX			1
#define	eParse_DISPLACE_DISPLACEMENT_IDX	2
#define	eParse_DISPLACE_CYLINDER_IDX			3
#define	eParse_DISPLACE_CGW_IDX					4	//베이직과 동기화를 위해 추가 Premium에서는 미사용
#define  eParse_DISPLACE_SLEEP_IDX				5	//베이직과 동기화를 위해 추가 Premium에서는 미사용
#define  eParse_DISPLACE_CANLINE_IDX				6
#define  eParse_DISPLACE_FUEL_LITER_IDX		7
#define  eParse_DISPLACE_INDICATOR_IDX			8
#define  eParse_DISPLACE_KEYTYPEDB_IDX		9
#define eParse_DISPLACE_PACV_SEPERATOR_IDX       10 
#define  eParse_DISPLACE_FUELTANKSIZE_IDX		12
#define  eParse_DISPLACE_CANFDADAPTER_IDX		13



#define eParse_FUNCTION_SOHDIAG_IDX  0

#define	eParse_TPMSALRAM_NONE_IDX				0
#define	eParse_TPMSALRAM_VALUE_IDX				1

#define ACTREQDATA_RESERVED_CNT		1
#define ACTCONVDATA_RESERVED_CNT	1
#define ACTRESDATA_RESERVED_CNT		1
#define ACTREADYDATA_RESERVED_CNT	1

#define	FUEL_TYPE	10
#define	ELECTRONIC_TYPE	11

#define CAN_ID_CHECK_LIST_MAX_CNT		50

#define ACCON_MASK				(uint8_t)(1 << 0)
#define IGON_MASK              	(uint8_t)(1 << 1)
#define ENGON_MASK				(uint8_t)(1 << 2)
#define ACCOFF_MASK				(uint8_t)(1 << 3)

#define ISACCON(x)  ((x) & ACCON_MASK)
#define ISIGON(x) 		 ((x) & IGON_MASK)
#define ISENGON(x)  ((x) & ENGON_MASK)
#define ISACCOFF(x)  ((x) & ACCOFF_MASK)

#define ARRAY_VSS_MAX  2
#define ARRAY_VSS_CNT  (ARRAY_VSS_MAX-1)

#define HIGHCAN1	201
#define HIGHCAN2	202
#define HIGHCAN3	203
#define LOWCAN1	204

typedef enum _ePOS_INDICATOR_TIMER
{
	ePOS_4WD_IND_0 = 0,
	ePOS_4WD_IND_1,
	ePOS_4WD_IND_2,
	ePOS_4WD_IND_3,
	ePOS_4WD_IND_4,
	ePOS_4WD_IND_5,	//5
	ePOS_4WD_IND_6,
	ePOS_4WD_IND_7,
	ePOS_ABS_IND_0,
	ePOS_ABS_IND_1,
	ePOS_ACU_IND_0,	//10
	ePOS_ACU_IND_1,
	ePOS_ACU_IND_2,
	ePOS_ACU_IND_3,
	ePOS_ACU_IND_4,
	ePOS_AUTOHOLD_IND_0,	//15
	ePOS_AUTOHOLD_IND_1,
	ePOS_AUTOHOLD_IND_2,
	ePOS_BATTCHARGE_IND_0,
	ePOS_BATTCHARGE_IND_1,
	ePOS_CHECKENGINE_IND_0,	//20
	ePOS_CHECKENGINE_IND_1,
	ePOS_CHECKENGINE_IND_2,
	ePOS_CHECKENGINE_IND_3,
	ePOS_DBCWARNNING_IND,
	ePOS_DPFGPF_IND,					//25
	ePOS_SCR_IND,
	ePOS_EPB_IND_0,
	ePOS_EPB_IND_1,
	ePOS_TCS_IND_0	,
	ePOS_TCS_IND_1,	//30
	ePOS_ISG_IND_0,
	ePOS_ISG_IND_1,
	ePOS_ISG_IND_2,
	ePOS_MDPS_IND_0,
	ePOS_MDPS_IND_1,	//35
	ePOS_OILLEVEL_IND,
	ePOS_OILPRESS_IND,
	ePOS_PARKINGBRAKE_IND_0,
	ePOS_PARKINGBRAKE_IND_1,
	ePOS_PARKINGBRAKE_IND_2,	//40
	ePOS_POWERDOWN_IND_0,
	ePOS_POWERDOWN_IND_1,
	ePOS_AHB_IND,
	ePOS_HEV_IND_0,
	ePOS_HEV_IND_1,	//45
	ePOS_HEV_IND_2,
	ePOS_HEV_IND_3,
	ePOS_HEV_IND_4,
	ePOS_HEV_IND_5,
	ePOS_HEV_IND_6,	//50
	ePOS_HEV_IND_7,
	ePOS_EV_IND_0,
	ePOS_EV_IND_1,
	ePOS_EV_IND_2,
	ePOS_EV_IND_3,	//55
	ePOS_EV_IND_4,
	ePOS_EV_IND_5,
	ePOS_FCEV_IND_0,
	ePOS_FCEV_IND_1,
	ePOS_FCEV_IND_2,	//60
	ePOS_FCEV_IND_3,
	ePOS_FCEV_IND_4,
	ePOS_TPMS_IND_0,
	ePOS_TPMS_IND_1,
	ePOS_TPMS_IND_2,	//65
	ePOS_INDICATOR_TIMER_MAX,
}ePOS_INDICATOR_TIMER;

#pragma pack(push, 1)

#if 1 // James Jean 2018/11/14
typedef enum _eAutoVinState
{
  eAUTOVIN_ELECTIC_UDS_REQ,
  eAUTOVIN_ENGINE_REQ,	
  eAUTOVIN_ELECTIC_REQ,
  eAUTOVIN_HYDRO_REQ,
  eAUTOVIN_REQ_MAX,
}eAUTOVIN_STATE;
#endif
typedef enum _eCVAutoVinState
{
  eAUTOVIN_CV_UNIVERSE_REQ,
  eAUTOVIN_CV_ELECCITY_REQ,
  eAUTOVIN_CV_REQ_MAX,
}eCV_AUTOVIN_STATE;

typedef enum _eOBD_STATE
{
	eOBD_NONE,
	eOBD_Error,
	eConfiguration_Error,
	eOBD_DB_Download_Error,
	eOBD_CheckCopyProtect,
	eOBD_Initialize,		//5
	eOBD_Initialized,
	eOBD_GetAutoVIN,
	eOBD_GetAutoVIN_Fail,
	eOBD_GetAutoVIN_Success,
	eOBD_Server_Waiting,				//10
	eOBD_Server_Connected,
	eOBD_Server_Communication,
	eOBD_Running_Start,
	eOBD_Running_Info_Mode,
	eOBD_Actuator_Mode,			//15
	eOBD_Running_Info_Mode_ControlDB,
	eOBD_FCS_Start,
	eOBD_FreezeFrame,
	eOBD_WriteThru_Mode,
	eOBD_FW_DB_Update_Mode,	//20
	eOBD_Sleep_Ready,
	eOBD_Sleep_Complete,
	eOBD_Power_Check,
	eOBD_Idle,
	eOBD_Comm_Finishing,			//25
	eOBD_Comm_Finished,
	eOBD_Selftest,
	eOBD_Selftest_FW_Update_Mode,
	eOBD_FOTA,
	eOBD_GetFuelLevelCheck,
	eOBD_GetSOH,
	eOBD_InitCanFD,
	eOBD_GetBrakeJudder,
#ifdef CGW_SECURITY
	eOBD_CGWAlgorithm,
#endif
	eOBD_State_Max,
}eOBD_STATE;	//eDLOGGER_STATE

typedef enum _eFREEZE_FRAME_STATE
{
	eFREEZE_NONE,
	eFREEZE_OPEN,
	eFREEZE_CLOSE,
	eFREEZE_REQ,
}eFREEZE_FRAME_STATE;


typedef enum _eCAN_COMM_STATE
{
	eCAN_COMM_NONE,			//0
	eCAN_COMM_TX,				//1
	eCAN_COMM_RX_ING,			//2
	eCAN_COMM_RX_DONE,		//3
	eCAN_COMM_RX_COMPLETE,		// 옆으로 전부 다 받았다s
	eCAN_COMM_RX_FAIL,		//3
	eCAN_COMM_MAX,
} eCAN_COMM_STATE;

typedef enum _eSET_OFFSET_STATE
{
	NONE_OFFSET,
	GET_DISTANCE,
	GET_MILEAGE,
	GET_OFFSET,
	GETED_OFFSET,
	END_OFFSET,
}eSET_OFFSET_STATE;

typedef enum _eDtcType
{
	eDTC_NONE,
	eDTC_UDS ,
	eDTC_CAN,
	eDTC_MAX
}eDtcType;

typedef enum _eFuelConsumptionType
{
	eConsum_NONE,
	eConsum_Instant ,
	eConsum_Total,
	eConsum_MAX
}eConsumptionType;

typedef enum _eFCS_STATE
{
	eFCS_NONE,
	eFCS_Error,
	eFCS_Ing,
	eFCS_Complete,
	eFCS_SEND,
}eFCS_STATE;

typedef enum _eODO_SUPP_STATE
{
	ODO_SUPP_NONE,
	ODO_SUPP_READY,
	ODO_SUPP_TRUE,
	ODO_SUPP_FALSE,
	ODO_SUPP_COMPLETE,
	ODO_SUPP_HI,
}eODO_SUPP_STATE;

typedef enum _eODO_TYPE
{
	ODO_TYPE_NONE,
	ODO_TYPE_SUPP_DCS_ODOMETER,
	ODO_TYPE_SUPP_DCS_ODOMETER_SUB,
	ODO_TYPE_CARB,
	ODO_TYPE_NOT_SUPP_TYPE,
}eODO_TYPE;

typedef enum _eACTUATOR_TYPE
{
	ACTUATOR_TYPE_NONE			= 0,
	ACTUATOR_TYPE_WAKEUP		= 101,
	ACTUATOR_TYPE_DOORLOCK	= 201,
	ACTUATOR_TYPE_DOORUNLOCK= 301,
	ACTUATOR_TYPE_LAMP     		= 401,
	ACTUATOR_TYPE_LAMPHORN	= 501,
	ACTUATOR_TYPE_ENGINERUN	= 601,
	ACTUATOR_TYPE_ENGINERUNCO	= 602,
	ACTUATOR_TYPE_ENGINESTOP	= 701,
	ACTUATOR_TYPE_AIRCON			= 801,
	ACTUATOR_TYPE_VENT			= 802,
	ACTUATOR_TYPE_VEHICLE_CHECK = 901,
	ACTUATOR_TYPE_REARDEFOG	= 1001,
	ACTUATOR_TYPE_AIRCON_STOP= 1002,
	ACTUATOR_TYPE_IG3ON			= 1003,
	ACTUATOR_TYPE_HIGHBEAM		= 1101,
	ACTUATOR_TYPE_DTC				= 6666,
	ACTUATOR_TYPE_SUCCESS		= 7777,
	ACTUATOR_TYPE_FAIL				= 8888,
	ACTUATOR_TYPE_MAX				= 9999,
}eACTUATOR_TYPE;

typedef enum _eACTUATOR_STATUS
{
	ACTUATOR_STATUS_NONE,					//0
	ACTUATOR_STATUS_CTRL_RUNNING,					//제어
	ACTUATOR_STATUS_CTRL_CHECKING,					//제어
	ACTUATOR_STATUS_CTRL_SUCCESS,	//3			//제어
//	ACTUATOR_STATUS_WAKE_SUCCESS,	//제어
//	ACTUATOR_STATUS_VEHICLE_CHECK_SUCCESS,	//제어
	ACTUATOR_STATUS_CTRL_RETRY,						//제어
	ACTUATOR_STATUS_CTRL_FAIL,			//5			//제어
	ACTUATOR_STATUS_CHECK_SUCCESS,	//6			//제어후에 상태확인
	ACTUATOR_STATUS_CHECK_FAIL,							//제어후에 상태확인
	ACTUATOR_STATUS_CHECK_RUNNING,					//제어후에 상태확인
	ACTUATOR_STATUS_FAIL5,
	ACTUATOR_STATUS_FAIL6,
	ACTUATOR_STATUS_FAIL7,
	ACTUATOR_STATUS_NONEDB,
	ACTUATOR_RESULT_MAX,
}eACTUATOR_STATUS;

typedef enum _eCanFDByPass
{
	CANFD_BYPASS_REQUEST = 0,
	CANFD_BYPASS_RESPONSE,
	CANFD_BYPASS_NEXT	
}eCanFDByPass;

typedef enum _eCOMMSET_TYPE
{
	TYPE_NONE,
	REQ_TYPE,
	RES_TYPE,
}eCOMMSET_TYPE;

typedef enum _eCLEAR_TYPE
{
	eCLEAR_TYPE_ALL,
	eCLEAR_TYPE_CAN1,
	eCLEAR_TYPE_CAN2,
	eCLEAR_TYPE_MAX,
}eCLEAR_TYPE;

typedef enum _eRETURNFAIL_TYPE
{
	eRETURNFAIL_TYPE_NONE						= 0x00,
	eRETURNFAIL_TYPE_FL 							= 0x01,
	eRETURNFAIL_TYPE_FR 							= 0x02,
	eRETURNFAIL_TYPE_RL 							= 0x04,
	eRETURNFAIL_TYPE_RR 							= 0x08,
	eRETURNFAIL_TYPE_TRUNKOPEN 				= 0x10,
	eRETURNFAIL_TYPE_COMMFAIL 				= 0x20,
	eRETURNFAIL_TYPE_LAMPOFF 					= 0x40,
	eRETURNFAIL_TYPE_HORNOFF 					= 0x80,
	eRETURNFAIL_TYPE_ENGON_AIRCONFAIL 	= 0x100,
	eRETURNFAIL_TYPE_ENGON_VENTFAIL 		= 0x200,
	eRETURNFAIL_TYPE_ENGON_FAIL 				= 0x400,
	eRETURNFAIL_TYPE_ENGOFF_FAIL 			= 0x800,
	eRETURNFAIL_TYPE_PARSINGFAIL				= 0x1000,
	eRETURNFAIL_TYPE_ALREADY_RUNNING 	= 0x2000,
	eRETURNFAIL_TYPE_DRIVING 					= 0x4000,
	eRETURNFAIL_TYPE_WAKEUPFAIL 				= 0x8000,
	eRETURNFAIL_TYPE_DOOROPEN 				= 0x10000,
	eRETURNFAIL_TYPE_IGOFF						= 0x20000,
	eRETURNFAIL_TYPE_ANOTHERFUNCTION 	= 0x40000,
	eRETURNFAIL_TYPE_ALREADY_ENGRUN 	= 0x80000,
	eRETURNFAIL_TYPE_USERCTRL 				= 0x100000,
	eRETURNFAIL_TYPE_ALREADY_ENGSTOP 	= 0x200000,
	eRETURNFAIL_TYPE_MODEM_DISABLE 		= 0x400000,
	eRETURNFAIL_TYPE_DOORUNLOCK			= 0x1000000,
	eRETURNFAIL_TYPE_REARDEFOG_FAIL		= 0x2000000,
	eRETURNFAIL_TYPE_HOODOPEN				= 0x4000000,
	eRETURNFAIL_TYPE_AIRCON_FAIL				= 0x8000000,
	eRETURNFAIL_TYPE_IG3_FAIL					= 0x10000000,
	eRETURNFAIL_NOT_ALLOWED_COMMAND         = 0x20000000,
    eRETURNFAIL_TYPE_BLOCKED                = 0x40000000,
    eRETURNFAIL_OBD_BUSY                    = 0x80000000,
    eRETURNFAIL_NO_DB_TEMPERATURE           = 0x0000000100000000,
    eRETURNFAIL_NO_DB                       = 0x0000000200000000,
    eRETURNFAIL_NOT_MATCH_TEMP_UNIT         = 0x0000000400000000,
    eRETURNFAIL_UNKNOWN					  	= 0x0000000800000000,
    eRETURNFAIL_URL_NOT_VALIED              = 0x0000001000000000,
    eRETURNFAIL_INVALIED_PARAM              = 0x0000002000000000,
    eRETURNFAIL_ALREADY_FINISHED_COMMAND    = 0x0000004000000000,
}eRETURNFAIL_TYPE;

typedef struct _stFastFunc{
	INT8U 	m_ucSpeed;			/* 현재 속도 */
	INT16U 	m_usRpm;				/* 현재 RPM */
	INT32U 	m_usEngine_run_t;	/* 운행 시간 */
	float 	m_nMileage_a;		/* 평균 연비 */					// (float)
	float 	m_nFuel_i;				/* 순간 연료소모량 */			// (float)
	float 	m_nFuel_t;				/* 연료소모량 */					// (float)
	float 	m_nFuel_r;			/* 예약자 연료소모량 */		// (float)
	float 	m_nFuel_vehicle;	/* 총 연료소모량 */				// (float)
	INT32U 	m_nOdometer_a;	/* 시동 후 이동 거리 */
	INT32U 	m_nOdometer_t;		/* 총이동 거리(TRIP) */
	INT32U 	m_nOdometer_r;	/* 사용자 이동거리 */
	INT8U 	m_ucFoot_brake;	/* 브레이크 상태 */
	INT8U	m_ucGearPos;		/* 현재 기어 단수*/
	INT8U	m_ucAuto_gear;		/* 현재 기어 레버 */
	INT8U 	m_ucFuel_cut; 		/* 퓨얼컷 상태 */
	INT8U  	m_ucFuel_lev;		/* 연료 잔량 */
	INT8U 	m_ucAcc_pos;		/* 가속페달 위치 */
	INT8U 	m_ucEngine_load;		/* 엔진부하(%) */
}stFastFunc;

typedef struct _stSlow1Func{
	INT8U	m_ucACU1;					/* 에어백1 */
	INT8U	m_ucACU2;					/* 에어백2 */
	INT8U	m_ucACU3;					/* 에어백3 */
	INT8U	m_ucACU4;					/* 에어백4 */
	INT8U 	m_ucSpeed_max;			/* 최고 속도 */
	INT8U 	m_ucSpeed_aver;			/* 평균 속도 */
	INT32U 	m_usSpd_section_t0;		/* 속도구간 누적시간 */
	INT32U 	m_usSpd_section_t1;		/* 속도구간 누적시간 */
	INT32U 	m_usSpd_section_t2;		/* 속도구간 누적시간 */
	INT32U 	m_usSpd_section_t3;		/* 속도구간 누적시간 */
	INT32U 	m_usSpd_section_t4;		/* 속도구간 누적시간 */
#if defined(STATISTICS)
	INT32U 	m_usSpd_section_t5;	/* 속도구간 누적 시간 */
	INT32U 	m_usSpd_section_t6;	/* 속도구간 누적 시간 */
	INT32U 	m_usSpd_section_t7;	/* 속도구간 누적 시간 */
	INT32U 	m_usSpd_section_t8;	/* 속도구간 누적 시간 */
	INT32U 	m_usSpd_section_t9;	/* 속도구간 누적 시간 */
	INT32U 	m_usSpd_section_t10;	/* 속도구간 누적 시간 */
	INT32U 	m_usSpd_section_t11;	/* 속도구간 누적 시간 */
	INT32U 	m_usSpd_section_t12;	/* 속도구간 누적 시간 */
	INT32U 	m_usSpd_section_t13;	/* 속도구간 누적 시간 */
	INT32U 	m_usSpd_section_t14;	/* 속도구간 누적 시간 */
#endif
	INT32U	m_usEngine_idle_t;			/* 공회전 누적 시간 */
	INT32U	m_usStop_t;					/* 정차 누적 시간 */
	INT32U	m_usBrake_t;					/* 제동 누적 시간 */
	INT32U	m_usInertia_t;				/* 관성주행 누적 시간 */
	INT32U	m_usGeneral_t;				/* 일반주행 누적 시간 */
}stSlow1Func;

typedef struct _stSlow2Func{
	float 	m_nBatt_v;                     	/* 현재 배터리 전압 */			// (float)
	float 	m_nBatt_c;                     	/* 현재 배터리 전류 */			// (float)
	INT8U 	m_ucBatt_Charge;				/* 현재 배터리 충전상태 */
	INT8U 	m_ucBatt_Health;             	/* 배터리 노화진행률 */
	INT8U 	m_ucBatt_Capacity;         	/* 배터리 용량 */
	float 	m_usPressure_FL;				/* 타이어 공기압 FL */			// (float)
	float 	m_usPressure_FR;				/* 타이어 공기압 FR */			// (float)
	float 	m_usPressure_RL;				/* 타이어 공기압 RL */			// (float)
	float 	m_usPressure_RR;				/* 타이어 공기압 RR */		// (float)
	INT8U	m_ucMalfunction;				/* 엔진 경고등 상태 */
	INT8U	m_ucHeadlamp;					/* 전조등 상태 */
	INT8U	m_ucBrakelamp_L;				/* 좌측 브레이크등 상태 */
	INT8U	m_ucBrakelamp_R;				/* 우측 브레이크등 상태 */
	INT8U	m_ucTailgate;					/* 트렁크 상태 */
	INT8U	m_ucDoor_FL;					/* 운전석 도어 상태 */
	INT8U	m_ucDoor_FR;					/* 조수석 도어 상태 */
	INT8U	m_ucDoor_RL;					/* 좌측 뒤 도어 상태 */
	INT8U	m_ucDoor_RR;					/* 우측 뒤 도어 상태 */
	INT8S	m_ucCoolant_Temp;			/* 냉각수 온도 */
	INT8U	m_ucBonnet;						/* 보닛 상태 */
	float 	m_usTire_Temp_FL;				/* 타이어 공기온도 FL */			// (float)
	float 	m_usTire_Temp_FR;				/* 타이어 공기온도 FR */			// (float)
	float 	m_usTire_Temp_RL;				/* 타이어 공기온도 RL */			// (float)
	float 	m_usTire_Temp_RR;				/* 타이어 공기온도 RR */		// (float)
}stSlow2Func;

typedef struct _stSlow3Func{
	INT8U	m_ucKey_State;					/* 키 상태 */
	//INT8U	m_ucSpeed_max;				/* 최고 속도 */
	//INT8U	m_ucSpeed_aver;				/* 평균 속도 */
	INT16U	m_ucRPM_max;					/* 최고 RPM */
	INT16U 	m_ucRPM_aver;					/* 평균 RPM */
	INT16U	m_usSpd_Acceleration_t;				/* 급가속 횟수 */
	INT16U 	m_usSpd_Retardation_t;				/* 급감속 횟수 */
	INT32U	m_usWarmup_t;					/* 워밍업 시간 */
	INT16U	m_usMileage_idx;				/* 연비 지수 */
	INT8S	m_ucEngineOil_Temp;		/* 엔진오일온도 */
	INT8S	m_ucMissionOil_Temp;		/* 미션오일온도 */
	INT8U	m_ucEV_Charge;				/* EV 충전기 탈착 여부 */
	INT8U	m_ucFuel_type;					/* 연료형태 */
	INT8U	m_ucWeak_Lamp;					/* 미등상태 */
	INT8U 	m_ucDoor_Lock_FL;				/* 도어 락 상태 */
	INT8U 	m_ucDoor_Lock_FR;				/* 도어 락 상태 */
	INT8U 	m_ucDoor_Lock_RL;				/* 도어 락 상태 */
	INT8U 	m_ucDoor_Lock_RR;				/* 도어 락 상태 */
	INT8U 	m_ucPower_Switch;				/* 도어 락 상태 */
	INT8U 	m_ucSOC_State;					/* SCO상태 */
}stSlow3Func;

typedef struct _stAdviceFunc{
	INT8U 	m_ucCruies_State;				/* CRUIES 상태 */
	INT8U 	m_ucThrottle_State;            /* throttle 닫힘 상태 */
	INT8U 	m_ucIsg_State;         			/* ISG 상태 */
}stAdviceFunc;

typedef struct _stLMFCEVFunc{
    INT8U   m_ucPurge_OpenClose;					/* 퍼지밸브 개폐감지*/
    INT8U   m_ucHydrogen_State;						/*수소공급 밸브 상태*/
    INT8U   m_ucPurge_State;						/*퍼지밸브 이상*/
    INT8U   m_ucDrain_State;						/*드레인밸브 이상*/
    INT16U  m_ucHydrogen_Press;						/*수소탱크 현재 압력값*/
    INT8U   m_ucHydrogen_Temp;						/*수소탱크 현재 온도값*/
    INT8U   m_ucHydrogen_Storage;					/*수소 탱크 연료 저장량*/
    INT8U   m_ucKey_State;							/*키 스타트*/
    INT8U   m_ucMotor_State;						/*모터_ 인버터 과열 경고*/
    INT8U   m_ucHydrogen_Leak;						/*수소리크 감지 신호*/
    INT16U  m_ucHydrogen_Tank_Press;				/*수소탱크 압력*/
    INT16U  m_ucBattery_Voltage;					/*고전압배터리 전압*/
    INT16U  m_ucBattery_Current;					/*고전압배터리 전류*/
    INT8U   m_ucBattery_SOC;						/*고전압배터리 SOC 값*/
    INT16U m_ucHydrogen_Vol;						/*수소탱크 부피*/
    INT8U   m_ucH2Vehicle_Ready;					/*수소차 레디*/
}stLMFCEVFunc;

typedef struct _stAEHEVFunc{
	INT8U	m_ucEngineOil_Temp;						/* 엔진오일온도 */
	INT16U  m_usMotor_Revolution;       			/* 모터 회전수 */
	INT8U 	m_ucSOC_State;							/* SCO상태 */
	INT16S 	m_usBattPack_Current;   		        /* 배터리팩 전류 */
	INT16U 	m_usBattPack_Voltage;       		    /* 배터리팩 전압 */
	INT8S 	m_ucBatt_Max_Temp;						/* 배터리 최대 온도 */
    INT8S 	m_ucBatt_Min_Temp;						/* 배터리 최소 온도 */
    INT32U  m_usMotor_Time;							/* 모터주행시간 */
    INT32U  m_usEngine_Time;						/* 엔진주행시간 */
    INT8U   m_ucHCU_Ready;							/* HCU 준비 */
    INT8U   m_ucBMS_Safety;							/* BMS 과충전 보호상태 */
}stAEHEVFunc;

typedef struct _stStaticFunc{
	unsigned long s_speed_t;
	unsigned long s_ulSpeedCnt;
	unsigned long s_ulRpmCnt;
	unsigned long s_rpm_t;
	float 				g_fFuelConsumption;						//순간소모량
	float 				g_fFuelConsumptionT;					//시동후누적소모량(총)
	U32					s_EngineRunningTime;
	BOOL 				g_bMilFCS;
	U8 					g_MilCount;
	U8					g_ucFcsTime;
	U32 					s_uiStartOdoMeter;
	U32					s_uiOldOdoMeter;
	BOOL 				s_bStartState;
	float 				g_fOdometer_run;
	int 					g_nOffset2;
	int					g_nOffset;										//시작 distance 보정값
	U8 					g_ucWarmupState;							//워밍업 시간
	INT32U 				g_VSSOver110;
	unsigned int 	s_uiVSSOldTime;
	bool 				g_bOdometerRcv;
	U32 					g_usOldDistance;
#if defined(STATISTICS)
	U8 					s_OldVSS;
	U8 					s_ucDiffVss;
	U8 					s_oldRetardcnt;
	U8					s_oldAccelcnt;
	U8 					s_arrVSSold[ARRAY_VSS_MAX];
	U8 					s_tempVSSoldPosition;
	U8 					s_thisposition;
#endif
}stStaticFunc;

typedef struct _stDistanceInfo
{
	float					m_fOdometer_run;
	int					m_nOffset2;
	int					m_nOffset;										//시작 distance 보정값
	unsigned int	m_uiOldDistance;
	bool					m_bOdometerRcv;
	bool					m_bStartState;
}stDistanceInfo;

typedef struct _stAutoVin
{
	bool bRxStatus;
	unsigned char ucSize;
	unsigned char strVinCode[DCS_AUTOVIN_SIZE+1];
}stAutoVin;

typedef struct _stControlConfig{
	unsigned char m_ucAVNVersion;
	unsigned char m_ucAVNType;
	unsigned char m_ucEngStopType;
	unsigned int m_uiWaitTime;
}stControlConfig;

typedef enum _eFUEL_TYPE
{
	GASOLINE,
	LPG,
	DIESEL,
	GASOLINE_HEV,	//일반 HEV
	ELECTRONIC,
	BI_FUEL,
	DIESEL_HEV,
	PLUGIN_HEV,
	FCEV,
	FFV,
	EV_NONE_READY,
	ETC
}eFUEL_TYPE;

typedef enum _ePACV_TYPE
{
	PA,               //0
	CV,
}ePACV_TYPE;
typedef enum _eFUNCTION_COMM_STATE
{
	eCOMM_NONE,							//0
	eCOMM_Get_AutoVIN,				//1
	eCOMM_DTC_FUNCTION,			//2
	eCOMM_FREEZE_FRAME,			//3
	eCOMM_FAST_FUNCTION,			//4
	eCOMM_SLOW1_FUNCTION,		//5
	eCOMM_SLOW2_FUNCTION,		//6
	eCOMM_SLOW3_FUNCTION,		//7
	eCOMM_UDS_OPEN,					//8
	eCOMM_CARB_OPEN,					//9
#ifdef CGW_SECURITY
	eCOMM_UDS_CARBOPEN,					//10
	eCOMM_UDS_EXTOPEN,					//11
	eCOMM_CGW_SECURITY1,					//12
	eCOMM_CGW_SECURITY2,					//13
#endif
	eCOMM_MAX,
} eFUNCTION_COMM_STATE;

typedef enum _eFCSMode
{
	eFCS_MODE_NONE,
	eFCS_MODE_AUTO,
	eFCS_MODE_MANUAL,
	eFCS_MODE_MANUAL_END
}eFCSMode;

typedef enum _eCmdInput
{
	eCMD_NONE,
	eCMD_AUTOVIN_1105,
	eCMD_MAX
}eCmdInput;

typedef enum _eMapMaf
{
	eMAPMAF_NONE,
	eMAPMAF_MAF,
	eMAPMAF_MAP,
	eMAPMAF_MAX
}eMapMaf;


typedef __packed struct _stMasterData{
	char m_cIndex[2];
	char m_cFuncType;
	char m_cRef[20];
	char m_cFloatRange;
	char m_cDescUnit;
	char m_cDataSize;
}stMasterData;

//typedef struct _stActuatorReqList{
//	unsigned int m_uiTiming;
//	unsigned int m_uiTimes;
//	char m_ucData[10];
//}stActuatorReqList;

typedef __packed struct _stActuatorReqInfo{
	unsigned int m_uiFuncIndex;
	unsigned char m_ucIndexPos;
	unsigned int m_uiCount;	//해당펑션ID의 라인수
	//unsigned char m_ucWakeupUsed;
	//unsigned char m_ucRetry;
	//unsigned char m_ucCanLine;
	//unsigned int m_uiCanSpeed;
	//stActuatorReqList m_stDataList[10];
}stActuatorReqInfo;

//typedef struct _stActuatorResIDList{
//	char m_cIndex[4];
//	unsigned int m_uiID;
//}stActuatorResIDList;

typedef __packed struct _stSlaveHWSetInfo{
	unsigned char m_ucCanCH;
	unsigned char m_ucCanChip;
	unsigned char m_ucCanSpeed;
	unsigned int  m_uiMaskCount;
	unsigned int m_uiStartMask[14];
	unsigned int m_uiEndMask[14];
}stSlaveHWSetInfo;

typedef __packed struct _stControlHWSetInfo{
	unsigned char m_ucCanCH;
	unsigned char m_ucCanChip;
	unsigned char m_ucCanSpeed;
	unsigned int  m_uiMaskCount;
	unsigned int m_uiStartMask[14];
	unsigned int m_uiEndMask[14];
}stControlHWSetInfo;

typedef __packed struct _stCanIDCheckList{
	unsigned int  m_uiMaskCount;
	unsigned int m_uiStartMask[CAN_ID_CHECK_LIST_MAX_CNT];
	unsigned int m_uiEndMask[CAN_ID_CHECK_LIST_MAX_CNT];
}stCanIDCheckList;
//0101
//3
//201
//500
//[0136][0134][0548]
typedef __packed struct _stActuatorResInfo{
	unsigned int m_uiFuncIndex;
	unsigned char m_ucIndexPos;
	unsigned int m_uiCount;	//해당펑션ID의 라인수
	//unsigned int m_uiCanSpeed;
	//stActuatorResIDList stResIDList[ACTUATORRESINFOLISTCNT];
}stActuatorResInfo;

typedef __packed struct _stActuatorReadyInfo{
	unsigned int m_uiFuncIndex;
	unsigned int m_uiCount;
}stActuatorReadyInfo;

typedef __packed struct _stActuatorConvertInfo{
	unsigned int m_uiFuncIndex;
	unsigned int m_uiCount;
}stActuatorConvertInfo;


typedef __packed struct _stFreezeFrame{
	uint16_t		m_usFreezeFuncType;		//1120,1330...
	uint8_t		m_ucState;						//0:fail, 1:success
	uint16_t		m_usLength;
	uint8_t		m_ucFreezeFrameData[FREEZE_FRAME_DATA_MAX];
}stFreezeFrame;

typedef __packed struct _stActuatorFreezeInfo{
	unsigned char m_ucIndex;
	unsigned char m_ucDTCIndex;
	unsigned char m_ucDTCCount;
	stFreezeFrame m_stFreezeFrame[FREEZE_FRAME_CNT_MAX];
}stActuatorFreezeInfo;

typedef __packed struct _stActuatorAirConData{ // dahae
	unsigned short m_usTemp;
	unsigned char m_ucValue1;
	unsigned char m_ucValue2;
	unsigned char m_ucValue3;
	unsigned char m_ucCS;
}stActuatorAirConData;

typedef __packed struct _stActuatorCtrlData{ //dahae
	unsigned int   m_uiFuncIndex;
	char           m_ucCtrlFunction[32];
	unsigned char  m_ucSupport;
}stActuatorCtrlData;

typedef __packed struct _stActuatorReqData{
	unsigned int m_uiFuncIndex;
	unsigned char m_ucWakeupUsed;
	unsigned char m_ucRetry;
	unsigned char m_ucCanLine;
	unsigned int m_uiCanSpeed;
//	char m_cReqVal[2];
//	char m_cResVal[2];
	unsigned int m_uiReqVal;
	unsigned int m_uiResVal;
	unsigned char m_ucLength;
	char m_ucData[66];				//!!!!!!!!!!!!!!!!!!상준책임 확인필요!!!!!!!!!!!!!!!!!
	unsigned int m_uiTiming;
	unsigned int m_uiTimes;
	int				 m_uiMin;
	int				 m_uiMax;
	float 			 m_fConvert;
	unsigned char m_ucFDCanLine;
	unsigned char m_ucFDCanBaudRate;
	unsigned char m_ucFDCanFrame;
	unsigned char reserved[ACTREQDATA_RESERVED_CNT];
}stActuatorReqData;

typedef __packed struct _stActuatorConvertData{
	unsigned int m_uiFuncIndex;
	unsigned char m_ucSeparator;
	unsigned char m_ucOn;
	unsigned char m_ucOff;
	unsigned char reserved[ACTCONVDATA_RESERVED_CNT];
}stActuatorConvertData;

typedef __packed struct _stActuatorResData{
	unsigned int m_uiFuncIndex;
	unsigned char m_ucCanLine;
	unsigned int m_uiCanSpeed;
	char m_cIndex[4];
	unsigned int m_uiReqVal;
	unsigned int m_uiResVal;
	unsigned char m_ucStartPos;
	unsigned char m_ucRealPos;
	unsigned char m_ucDataSize;
	unsigned char m_ucMsbLsb;
	unsigned int m_uiMaskVal;
	unsigned char m_ucConvrule;
	float m_fA;
	float m_fB;
	float m_fC;
	char m_cD;
	char m_cE;
	short m_cF;
	float	m_fData;
	//unsigned char m_ucLut[75];
	unsigned char* pucArrLut;
	char m_cCompType;
	unsigned int m_uiCompVal;
	unsigned char m_ucFDCanLine;
	unsigned char m_ucFDCanBaudRate;
	unsigned char m_ucFDCanFrame;
	unsigned char reserved[ACTRESDATA_RESERVED_CNT];
}stActuatorResData;

typedef __packed struct _stActuatorReadyData{
	unsigned int m_uiFuncIndex;
	unsigned char m_ucWakeupUsed;
	unsigned char m_ucCanLine;
	unsigned int m_uiCanSpeed;
	char m_cIndex[4];
	unsigned int m_uiReqVal;
	unsigned int m_uiResVal;
	unsigned char m_ucStartPos;
	unsigned char m_ucRealPos;
	unsigned char m_ucDataSize;
	unsigned char m_ucMsbLsb;
	unsigned int m_uiMaskVal;
	unsigned char m_ucConvrule;
	float m_fA;
	float m_fB;
	float m_fC;
	char m_cD;
	char m_cE;
	short m_cF;
	float	m_fData;
	//unsigned char m_ucLut[75];
	unsigned char* pucArrLut;
	unsigned char m_ucFDCanLine;
	unsigned char m_ucFDCanBaudRate;
	unsigned char m_ucFDCanFrame;
	unsigned char reserved[ACTREADYDATA_RESERVED_CNT];
}stActuatorReadyData;


typedef __packed struct _stActuatorFreezeData{	//DB파싱한 데이터
	char m_cSystem[7];
	char m_cEcuid[4];
	unsigned int m_uiFuncIndex;
	char m_ucOpenReq[20];
	char m_ucOpenRes[20];
	char m_ucFreezeReq[20];
	char m_ucFreezeRes[20];
	char m_ucCloseReq[20];
	char m_ucCloseRes[20];
}stActuatorFreezeData;


typedef __packed struct _stSlaveData{
	char 	m_cIndexFine[4];
	char 	m_cReqNode[18];
	char 	m_cResNode[18];
	U8 		m_cStartPos;
	U8 		m_cRealPos;
	U8 		m_cDataSize;
	U8 		m_cDataType;
	unsigned short m_usUnit;	//U8 m_cUnit;
	U8 		m_cCunvRule;
	float 	m_cA;
	float 	m_cB;
	float 	m_cC;
	char 	m_cD;
	char 	m_cE;
	short 	m_cF;
	float	m_fData;
	char	m_cLut[80];
	char	m_cCommType;
	unsigned char m_ucCanLine;
}stSlaveData;

typedef __packed struct _stSlaveDTCData{
	char 	m_cEcuID[5];
	U16 	m_cFunctiontype;
	U8 		m_cStartPos;
	U8 		m_cReadno;
	U8 		m_cSkipno;
	U8 		m_cTotalmasking[8];
#if 1 // //상용 CAN 적용으로 인한 Size up
	char 	m_cOpenReqNode[20];
	char 	m_cOpenResNode[20];
	char 	m_cReqNode[25];
	char 	m_cResNode[25];
	char 	m_cCloseReqNode[20];
	char 	m_cCloseResNode[20];
#else
	char 	m_cOpenReqNode[15];
	char 	m_cOpenResNode[15];
	char 	m_cReqNode[20];
	char 	m_cResNode[20];
	char 	m_cCloseReqNode[15];
	char 	m_cCloseResNode[15];
#endif
	U8 		m_cRequesttime;
	U8 		m_cSearchtype;
	U8 		m_cIndex;
}stSlaveDTCData;

typedef __packed struct _stDTCFunc{
	uint64_t		m_uiOccurredEventTime;	//발생시간
	uint16_t		m_usLength;						//전체길이
	uint8_t		m_ucDTCType;					//1:alarm 2:control
	uint8_t		m_ucSystemCnt;				//시스탬갯수
	uint16_t		m_usSystemLength;			//시스탬세트의길이

	uint8_t		m_ucECU_ID[7];				//ECUID
	uint16_t		m_usFuncType;					//250,251
	uint8_t		m_ucState;						//0:fail, 1:NODTC 2:DTC
	uint8_t		m_ucUDSFlag;					//1:UDS 2:CAN
	uint32_t		m_nOdometer;
	uint8_t		m_ucFCSTime[7];
	uint8_t		m_ucDTCCount;					//DTC 갯수
	uint8_t		m_ucDTCCode[DTC_DATA_MAX];
}stDTCFunc;	//시스템단위

typedef __packed struct _stReferenceTable{
	char m_cIndexFine[3];
	char m_cCurrNumber;
	char m_cNaviNumber[10];
	char m_cNaviCounter;
}stReferenceTable;

typedef __packed struct _stFuntionRequestCnt{
	char m_cRequestCnt;
} stFunctionRequestCnt;

typedef __packed struct _stFunctionParameter{
	char ucFunctionType;
	char ucParaTotalCnt;
}stFunctionParameter;

typedef __packed struct _stSaveOBDInfo
{
	INT32U 	m_nOdometer_t;		/* 총이동 거리(TRIP) */
	INT32U 	m_nOdometer_rev;		/* 예약자 이동거리 */
	float 	m_fFuel_vehicle;		/* 총 연료소모량 */
	float 	m_fFuel_rev;				/* 예약자 연료소모량 */
}stSaveOBDInfo;

typedef __packed struct _stSlaveDBCount
{
	unsigned char m_uiFastCnt;
	unsigned char m_ucSlow1Cnt;
	unsigned char m_ucSlow2Cnt;
	unsigned char m_ucSlow3Cnt;
	unsigned char m_ucTotalCnt;
}stSlaveDBCount;

typedef __packed struct _stDieselFuelConsume
{
	float m_fInjectionQuantity;
	float m_fMI;
	float m_fPIL1;
	float m_fPIL2;
	float m_fPIL3;
	float m_fPOL1;
	float m_fPOL2;
}stDieselFuelConsume;

typedef __packed struct _stIndicatorCheck{
	unsigned int m_uiCanStopTimer;
	unsigned int m_uiTimerFromLastON;
	unsigned int m_uiTimerFromLastOFF;
}stIndicatorCheck;


typedef __packed struct _stEvAirConControl
{
	bool m_bSetIG3On;
	bool m_bSetAirCon;
}stEvAirConControl;

typedef __packed struct _stBrakeJudder
{
	unsigned int uiValue;
	unsigned char ucCheckType;
	bool bValid;
}stBrakeJudder;

typedef enum _eBrakeJudder
{
	eMorethan =0,
	eBelow,
	eExcess,
	eUnder,
	eEqual
}eBrakeJudder;


typedef enum _eEvReadySet
{
	eIGONSet,
	eAirConSet,
	eEvReadyMax
}eEvReadySet;

#define FUELTYPE_GET_STATE() (g_eFuel_Type)
#define FUELTYPE_SET_STATE(X) (g_eFuel_Type = X)

#define FCSMODE_GET_STATE() (g_eFCSMode)
#define FCSMODE_SET_STATE(X) (g_eFCSMode = X)

#define CMD_INPUT_GET_STATE() (eCommanddInput)
#define CMD_INPUT_SET_STATE(X) (eCommanddInput = X)

#define CANCOMM_GET_STATE() (g_eCanTxRxState)
#define CANCOMM_SET_STATE(X) (g_eCanTxRxState = X)

#define FUNCTION_GET_STATE() (g_Function_State)
#define FUNCTION_SET_STATE(X) (g_Function_State = X)

#define OFFSET_GET_STATE() (g_eSET_OFFSET_STATE)
#define OFFSET_SET_STATE(X) (g_eSET_OFFSET_STATE = X)

#define FCS_GET_STATE() (g_eFCSState)
#define FCS_SET_STATE(X) (g_eFCSState = X)

#define ODO_SUPP_GET_STATE() (g_eOdoSuppState)
#define ODO_SUPP_SET_STATE(X) (g_eOdoSuppState = X)

#define ODO_TYPE_GET_STATE() (g_eOdoType)
#define ODO_TYPE_SET_STATE(X) (g_eOdoType = X)

//#define DTC_TYPE_GET_STATE() (g_eDTCType)
//#define DTC_TYPE_SET_STATE(X) (g_eDTCType = X)




//extern stSlow3Func				g_stSlow3FuncData;                	//DATA STRUCT
extern eCAN_COMM_STATE  g_eCanTxRxState;
//extern unsigned long g_ulObdRunningTime;		//OBD 구동시간

extern U8 *g_pDTCRequest;
extern U8 *g_pFastRequest;
extern U8 *g_pSlow1Request;
extern U8 *g_pSlow2Request;
extern U8 *g_pSlow3Request;
extern U8 *g_pSuppRequest;

extern BOOL			*g_pDTCState;
extern stDTCFunc		*g_stDTCFuncData;           //DTC DATA STRUCT
extern stFastFunc g_stFastFuncData;               //FINE DATA STRUCT

extern eFUNCTION_COMM_STATE 	g_Function_State;
extern eODO_SUPP_STATE				g_eOdoSuppState;

extern FIRMWARE_INFO	g_FirmwareInfo;

//void InitializeOBDManager(void);
void OBDManager(void);
BOOL DBParser();
bool DBParser_Actuator();
bool GetDataBaseParsing_Master();
bool GetDataBaseParsing_Slave();
bool GetDataBaseParsing_Reference();
bool GetDataBaseParsing_Actuator();
bool GetMasterData(U8 *cMassage, char message[], char endMessage[], U16 *nCnt);
bool GetSlaveData(U8 *cVehicleMassage, U16 *nCnt);
bool GetSlaveData_Modify(U8 *cVehicleMassage, U16 *nCnt);
bool GetSlaveDTCData(U8 *cVehicleMassage, U8 *nCnt);
bool GetActuatorData(U8 *p_ucSavedData);
void GetReqData(char *,unsigned char);
void GetResData(char *,unsigned char);
void GetVehicleStatusData(char *,unsigned char);
void GetConvertData(char *,unsigned char);
void GetFreezeData(char *,unsigned char);

#if defined(PROTOCOL18)
void GetAirConData(char *,unsigned char); //dahae
void GetCtrlData(char *,unsigned char);//dahae
#endif

void GetConfigData(char *,unsigned char);
void GetUseFunction(char *,unsigned char);
char* LUT_20_Parsing(char lut_buff[]);
bool HardwareSetData(U8 *cMassage, char message[]);
bool DisplaceSetData(U8 *cMassage, char message[]);
bool FunctionSetData(U8 *cMassage, char message[]);
bool TpmsAlramSetData(U8 *cMassage, char message[]);
int SetRequest(stReferenceTable *Table, int nTableCnt, eFUNCTION_COMM_STATE eFunction);
stReferenceTable* SetReferenceTable(stMasterData *NaviData, stSlaveData *CurrData,U16 nReqCnt, U16 nCurrCnt, U16 *nCnt);
BOOL CurrentCalculate(int CurrNum, int *data, U8 *Response);
BOOL CurrentCalculate_V(int CurrNum, int *data, U8 *Response);
BOOL CurrentCalculate_V2(int CurrNum, int *data, U8 *Response);
int8_t Compute_Data(stSlaveData *pCurrDataBase, int data);
int8_t Compute_Data_V(stActuatorReadyData*, int);
int8_t Compute_Data_V2(stActuatorResData*, int);
int LUT_Data_compute( int lut_no, int order, int int_data, char lut_buff[]);
int LUT_Data_compute_20( int lut_no, int order, int int_data, char lut_buff[]);
U8 getSlaveTableNum(char index[]);
void freeitems();
void AutoVIN(void);
void SetOBDState(eOBD_STATE state);
void GetSavedOBDdata(U8 ucGetStatus);
void SetSavedOBDdata();
char* LUT_20_Parsing(char lut_buff[]);
void PassThruReadMsgs(stPASSTHRU_MSG *pReadMsg, unsigned int uiReadMsgLen, unsigned int eInCommType, boolean_t bIsStandard);
eOBD_STATE GetOBDState(void);
eOBD_STATE GetNextOBDState(void);
void Mileage(unsigned char ucType);
float DCS_FuncType(float nDataA, float nDataB, unsigned long ulCnt, int funcType);
void CalOdoMeter(void);
void CalFuelConsumption(double data, U8 type);
float GetFuelConsumption(eConsumptionType eFuelConsumMode);
U16 CalGearInfo(U8 ucGearPos, U8 ucGearLever);
void SetOffset();
void Distance(void);
void VSS_1SecCallback();
void GitCANReadMsgs(stPASSTHRU_MSG *pReadMsg, unsigned int uiReadMsgLen, unsigned int eInCommType, boolean_t bIsStandard);
void Git_LCANReadMsgs(stPASSTHRU_MSG *pReadMsg, unsigned int uiReadMsgLen, unsigned int eInCommType, boolean_t bIsStandard);
bool Can_Actuator_Set( eACTUATOR_TYPE , eCOMMSET_TYPE);
void Set_Actuator( eACTUATOR_TYPE );
void OBDActuatorModeState( eACTUATOR_TYPE );
void OBDRunningInfo();
void OBDEngineRun();
void OBDEngineStop();
void OBDDoorLock();
void OBDDoorUnlock();
void OBDLamp();
void OBDLampHorn();
void OBDAircon();
void OBDVent();
void OBDRearDefog();
void OBDHighBeam();
void OBDVehicleCheck();
void OBDWakeUp();
void OBDAutoVIN();
void OBDFuelLevelCheck();
void OBDSOHCheck();
void OBDBrakeJudderCheck();
void OBDCommFinishing();
void OBDActuator_Req();
void OBDActuatorModeManager();
void OBDIG3On();
void OBDFATCStop();
unsigned int OBDActuator_Res();
void SetActuatorStatus( eACTUATOR_STATUS );
eACTUATOR_STATUS GetActuatorStatus();
bool StatusDecision(unsigned char , unsigned int );
bool StatusDecision_char(unsigned char , unsigned char );
unsigned char ReplaceData(unsigned int , unsigned char , unsigned char );
void DefaultMaskSet(unsigned int  *StartMask,unsigned int  *EndMask, char Count);
bool Can_Actuator_Set_Wakeup( char cNum);	//추후 함수 수정필요 현재 DB상관없이 2번만 돌도록 되어있음
void ActRelationInit();
void DefaultAllCanMaskSet();
void DefaultCanMaskSetDCAN();
void OBDRunningInfo_ControlDB();
void OBDFCS_Mode();
void OBDFreezeFrame_Mode();
int ReportAlramStatus(eMESSAGE_EVENT_KEY,int);
int ReportAfterDriving();
int ReportBeforeDriving();
void SendEngContinueCode();
void ExtractFreezeIndex();
eFREEZE_FRAME_STATE GetFreezeFrameState();
void SetFreezeFrameState(eFREEZE_FRAME_STATE);
bool Freeze_FunctionClassification(eFREEZE_FRAME_STATE eState, U8 ParaCounter );
void CH2_Default_MaskID_Set();
void FreezeFrameReport();
bool VIN_ValidCheck();
int CheckInitializeTime(unsigned int uiNewTime, unsigned int uiOldTime, unsigned short usNewTimezone);

void fuelConsume();
void energyConsume();

int ReportDirectAlramStatus(int32_t, eMESSAGE_EVENT_KEY eEVENT, int iValue);
int ReportAlramStatus(eMESSAGE_EVENT_KEY eEVENT, int iValue);
int ReportAlramTPMS(eMESSAGE_EVENT_KEY eEVENT, char* iValue);
int ReportDriving();
int ReportNoDriving();

#if defined(PROTOCOL18)
int ReportChargingStatus();
#endif

int ReportAlramDtcStatus();
int ReportAlramCarStatus();
int ReportVehicleStatus(stCarReport carReport);
#if defined(REASON_8BYTE)
	int ResponseSmartKeyResult(int iActuatorType , int iResult, uint64_t iReason);
#else
    int ResponseSmartKeyResult(int iActuatorType , int iResult, int iReason);
#endif

void ShowSmartkeyResponse(stCarReport* report);
void ShowSmartkeyActionRsp(stCarReport* report);
void IndicatorTimeoutCheck();

boolean_t IsDBCorrect();


void SetEvAirControl(eEvReadySet eEvSet);
void SetClearAirControl();
bool IsNeedAirConOff();
void OBDManagerInit();
bool CanBufferClear(eCLEAR_TYPE);
char *strsep(char **stringp, const char *delim);
void CheckFahrenheit();
boolean_t GetObdRcvSleepFlag();
void SetObdRcvSleepFlag(boolean_t bRcvFlag);

void SetPACVType(ePACV_TYPE PACVType);
ePACV_TYPE GetPACVType(void);
void SetCANBaudrate(eCanBaudrate CanBaudrate);
eCanBaudrate GetCANBaudrate(void);

void ProcessSecurityAlarm();

#endif /* __OBD_MANAGER_H__ */

/***************************** END OF FILE ****/

