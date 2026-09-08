/************************************************************************************************************\
* Description :  Definitions for the PassThru API                                                            *
**************************************************************************************************************
* Project     :  PassThru API DLL                                                                            *
* File        :  GIT_PassThruDefines.h                                                                       *
* Version     :  1.00                                                                                        *
* Reference   :  SAE J2534 Februar 2002 Final = 02.02                                                        *
**************************************************************************************************************
* Company     :  I+ME Actia GmbH                                                                             *
\************************************************************************************************************/
#include "typedef.h"

#ifndef _GIT_PASSTHRU_DEFINES_H
#define _GIT_PASSTHRU_DEFINES_H

///////////////////////////////////////////////////////////////////////////////
//PassThruConnect ProtocolID Values
#define PROTOCOL_OVERLAPPED                     ((unsigned long)0x00)
//#define J1850VPW	                            ((unsigned long)0x01) /**< GM / DaimlerChrysler CLASS2 */		//개발삭제
//#define J1850PWM	                            ((unsigned long)0x02) /**< Ford SCP */		//개발삭제
#define ISO9141 	                            ((unsigned long)0x03) /**< ISO9141 and ISO9141-2 */
#define ISO14230                                ((unsigned long)0x04) /**< Keyword Protocol 2000 (ISO 14230-4) */
#define ISO14230_ETC                            ((unsigned long)0x44) /**< Keyword Protocol 2000 REPROGRAM에 사용*/
#define PID_CAN	                                ((unsigned long)0x05) /**< Raw CAN (flow control not handled automatically by interface) */
#define ISO15765	                            ((unsigned long)0x06) /**< ISO15765-2 flow control enabled (see Appendix A for high level description) */
#define ISO15765_CUBIS                          ((unsigned long)0x16) /**< ISO15765-2 flow control enabled (see Appendix A for high level description) */
#define ISO15765_SINGLE                         ((unsigned long)0x17) /**< ISO15765 TX,RX SINGLE FRAME ONLY*/
//#define ISO15765_GM		                    ((unsigned long)0x18) /* PDI 불필요 프로토콜 */
#define ISO14229		                        ((unsigned long)0x19) /**< uds protocol*/
#define ISO14229_UDS	                        ((unsigned long)0x20) /**< uds protocol*/
#define ISO15765_NEW                            ((unsigned long)0x21) /**< can 송신데이터 갯수가 255이상인경우 예외처리 프로토콜*/
#define ISO15765_SINGLE_SMK                     ((unsigned long)0x22) /**< smk 길이정보 없이 보내는 싱글프레임 프로토콜 추가 - 2010.07.28 황제욱 */
//#define ISO15765_GM_100K	                    ((unsigned long)0x23) /* PDI 불필요 프로토콜 */
//#define ISO15765_SM_MULTI   					((unsigned long)0x24) /* PDI 불필요 프로토콜 */
#define ISO15765_SINGLE_PODS                    ((unsigned long)0x25) /**< ISO15765_SINGLE 과 동일하나 TX전에 RX 버퍼 클리어 루틴 추가 */
//#define ISO15765_SID36		                ((unsigned long)0x26) /*PDI 불필요 프로토콜*/
#define ISO15765_NORMAL							((unsigned long)0x27) /**< ISO15765 에서 팬딩처리를 하지않고 PC로 그냥 송신하게 처리 */
#define ISO15765_CAN_HWSET_DB					((unsigned long)0x28) /**< ISO15765_NORMAL 에서 CAN 종류(high,low,single) 및 속도, 채널 셋팅을 DB에서 할 수 있게 처리 */
#define ISO15765_CAN_HWSET_DB_NEW				((unsigned long)0x2D) /**< ISO15765_CAN_HWSET_DB에서 ISO15765_NEW 도 같이 처리될 수 있게 추가 150615*/ 
#define ISO15765_EXCEPT              			((unsigned long)0x29) /**< ISO15765 와 같으나 중복되거나 누락된 프레임 있으면 데이터 생략시킴 110715 LWH */
#define ISO15765_CAN_HWSET_DB_SINGLE			((unsigned long)0x2A) /**< ISO15765_CAN_HWSET_DB 에서 ISO15765_SMK_NEW 처럼 TX 처리함*/
#define ISO15765_ACU_SINGLE						((unsigned long)0x2B) /**< ISO15765_SINGLE_PODS에서 Rx data 부분만 CUBIS */
#define ISO14229_UDS_HWSET_DB					((unsigned long)0x2C) /**< ISO15765_LOW_CAN_DIMS 2013-09-03 김연수씨 요청으로 추가 */ 

//0x0100 ~ 0x01FF ES95486 스펙 영역으로 사용 
//#define ISO14229_ES95486_170                  ((unsigned long)0x0100) /* ISO15765_CAN_HWSET_DB 에서 3002 에서 P3_MAX 사용, CAN RX FAIL 시 NULL RETURN */
//#define ISO14229_ES95486_170_MMCAN            ((unsigned long)0x0101) /* ISO15765_CAN_HWSET_DB 에서 3002 에서 P3_MAX 사용, CAN RX FAIL 시 NULL RETURN */
//#define ISO14230_ES95486_170                  ((unsigned long)0x0110) /* ISO15765_CAN_HWSET_DB 에서 3002 에서 P3_MAX 사용, CAN RX FAIL 시 NULL RETURN */
//#define ISO14230_ES95486_170_MMCAN            ((unsigned long)0x0111) /* ISO15765_CAN_HWSET_DB 에서 3002 에서 P3_MAX 사용, CAN RX FAIL 시 NULL RETURN */
#define ISO14229_ES95486_02_100                 ((unsigned long)0x0100)	//APP에서 P3MIN값에 +30ms적용하여 전달함, CANID 마스킹영역 0x580~0x5FF 추가
#define ISO14229_ES95486_02_100_NEW				((unsigned long)0x0101)	//20190918 Jay ES95486개정사항 적용 + 255byte 이상 송신
#define ISO14229_ES95486_02_102                 ((unsigned long)0x0102)	//0x0100 과 동일처리
#define ISO14229_ES95486_02_103                 ((unsigned long)0x0103)	//0x0100 과 동일처리
#define ISO14229_ES95486_02_104                 ((unsigned long)0x0104)	//0x0100 과 동일처리
#define ISO14229_ES95486_02_105                 ((unsigned long)0x0105)	//0x0100 과 동일처리
#define ISO14229_ES95486_02_106                 ((unsigned long)0x0106)	//0x0100 과 동일처리
#define ISO14229_ES95486_02_107                 ((unsigned long)0x0107)	//0x0100 과 동일처리
#define ISO14229_ES95486_02_108                 ((unsigned long)0x0108)	//0x0100 과 동일처리
#define ISO14229_ES95486_02_109                 ((unsigned long)0x0109)	//0x0100 과 동일처리
#define ISO14229_ES95486_02_10A                 ((unsigned long)0x010A)	//ES95486개정사항 적용 + 255byte 이상 송신 CAN ID +8 필터링 미적용
#define ISO14229_ES95486_02_10B                 ((unsigned long)0x010B)	//0x0100 과 동일처리
#define ISO14229_ES95486_02_10C                 ((unsigned long)0x010C)	//0x0100 과 동일처리
#define ISO14229_ES95486_02_10D                 ((unsigned long)0x010D)	//0x0100 과 동일처리
#define ISO14229_ES95486_02_10E                 ((unsigned long)0x010E)	//0x0100 과 동일처리
#define ISO14229_ES95486_02_10F                 ((unsigned long)0x010F)	//0x0100 과 동일처리
#define ISO14229_ES95486_170                	((unsigned long)0x0110)	//FCS에서 신규프로토콜로 변경할 때 사용(GDSM, G3에서만 사용)
#define ISO14229_ES95486_170_MMCAN              ((unsigned long)0x0111)	//0x0110 과 동일처리
#define ISO14230_ES95486_170                	((unsigned long)0x0112)	//0x0110 과 동일처리
#define ISO14230_ES95486_170_MMCAN              ((unsigned long)0x0113)	//0x0110 과 동일처리

#define ISO14230_ES95486_DOIP_120               ((unsigned long)0x0120)	//0x0100 과 동일처리
#define ISO14230_ES95486_DOIP_121               ((unsigned long)0x0121)	//0x0100 과 동일처리 + 255byte 이상 송신
#define ISO14230_ES95486_DOIP_122               ((unsigned long)0x0122)	//0x0100 과 동일처리
#define ISO14230_ES95486_DOIP_123               ((unsigned long)0x0123)	//0x0100 과 동일처리
#define ISO14230_ES95486_DOIP_124               ((unsigned long)0x0124)	//0x0100 과 동일처리
#define ISO14230_ES95486_DOIP_125               ((unsigned long)0x0125)	//0x0100 과 동일처리
#define ISO14230_ES95486_DOIP_126               ((unsigned long)0x0126)	//0x0100 과 동일처리
#define ISO14230_ES95486_DOIP_127               ((unsigned long)0x0127)	//0x0100 과 동일처리
#define ISO14230_ES95486_DOIP_128               ((unsigned long)0x0128)	//0x0100 과 동일처리
#define ISO14230_ES95486_DOIP_129               ((unsigned long)0x0129)	//0x0100 과 동일처리
#define ISO14230_ES95486_DOIP_12A               ((unsigned long)0x012A)	//0x0100 과 동일처리
#define ISO14230_ES95486_DOIP_12B               ((unsigned long)0x012B)	//0x0100 과 동일처리
#define ISO14230_ES95486_DOIP_12C               ((unsigned long)0x012C)	//0x0100 과 동일처리
#define ISO14230_ES95486_DOIP_12D               ((unsigned long)0x012D)	//0x0100 과 동일처리
#define ISO14230_ES95486_DOIP_12E               ((unsigned long)0x012E)	//0x0100 과 동일처리
#define ISO14230_ES95486_DOIP_12F               ((unsigned long)0x012F)	//0x0100 과 동일처리
#ifdef NEW_29BIT_CAN
#define ISO15765_ES95486_29bit					((unsigned long)0x0150) //29bit 승용CAN
#define ISO15765_ES95486_DOIP_29BIT				((unsigned long)0x0170) //29bit 승용DOIP on CAN
#endif
#ifdef HOTA
#define ISO14229_ES95486_02_HOTA                ((unsigned long)0x0130) // Single/Multi Frame 2Byte Length
#endif

#ifdef CANFD_PROTOCOL
#define ISO14229_ES95486_02_130_CANFD           ((unsigned long)0x0130)	//0x0100 과 동일처리
#define ISO14229_ES95486_02_131_CANFD           ((unsigned long)0x0131)	//0x0100 과 동일처리 + 255byte 이상 송신
#define ISO15765_ES95486_135_29bit_CANFD        ((unsigned long)0x0135)	//0x0150 과 동일처리
#define ISO15765_ES95486_136_29bit_CANFD        ((unsigned long)0x0136)	//0x0150 과 동일처리 + 255byte 이상 송신
#endif


#define SCI_A_ENGINE	                        ((unsigned long)0x07) /**< SAE J2610 (DaimlerChrysler SCI) configuration A for engine */
#define SCI_A_TRANS	                            ((unsigned long)0x08) /**< SAE J2610 (DaimlerChrysler SCI) configuration A for transmission */
#define SCI_B_ENGINE	                        ((unsigned long)0x09) /**< SAE J2610 (DaimlerChrysler SCI) configuration B for engine */
#define SCI_B_TRANS		                        ((unsigned long)0x0A) /**< SAE J2610 (DaimlerChrysler SCI) configuration B for transmission */
#define ISO14230_LLINE_LOW                      ((unsigned long)0x0C) /**< Keyword Protocol 2000 L-line Low */
#define ISO15765_SMK                            ((unsigned long)0x0D) /* PDI 불필요 프로토콜 */
//#define ISO15765_SMK_NEW                        ((unsigned long)0x0E) /* PDI 불필요 프로토콜 */

//Reserved for SAE use	                        0x0B - 0xFFFF
//Tool manufacturer specific	                0x10000 - 0xFFFFFFFF
#define CAN_2                                   ((unsigned long)0x010000) /**< Raw CAN2 (flow control not handled automatically by interface) */
#define ISO9141_2                               ((unsigned long)0x010001) /**< ISO9141 and ISO9141-2 */
#define ISO14230_2                              ((unsigned long)0x010002) /**< Keyword Protocol 2000 (ISO 14230-4) */
#define ISO9141_2_5BPSTXONLY                    ((unsigned long)0x010003) /**< TCU3 5BPS TX ONLY NO  SYNC    */
#define ISO9141_2_DW_SIEMENSE                   ((unsigned long)0x010004) /* DW SIEMENSE*/
#define ISO9141_2_DW_ABS						((unsigned long)0x010005) /* DW ABS*/
#define ISO9141_2_SyncTime						((unsigned long)0x010006) /* atoz sync time ~50ms */
#define ISO9141_2_00D1							((unsigned long)0x010007) /* 2009.10.06 에쿠스(lz) 오토에어콘 김동현 추가 요청(qir)*/
#define UDSonFlexRay_EOB						((unsigned long)0x010008) /* FlexRay LWH 130130 - CONSECUTIVE_EOB를 사용 함*/
#define UDSonFlexRay_NONEOB						((unsigned long)0x010009) /* FlexRay LWH 131210 - CONSECUTIVE_EOB를 사용하지 않음*/

//Define HMC/KMC Protocol ID
#define SIEMENS_SIMPLEX							((unsigned long)0x100000) /**< Siemens simpplex type ex) Accel     */
#define SIEMENS_DUPLEX							((unsigned long)0x100001) /**< Siemens dupplex type  ex) Accent    */
#define ISO9141_LUCAS							((unsigned long)0x100002) /**< ISO 9141 LUCAS ABS Type             */
#define MELCO_OLDTYPE							((unsigned long)0x100003) /**< ISO 9141 MELCO OLD Type             */
//#define PROTON_MELCO							((unsigned long)0x100004) /**< ISO 9141 PROTON MELCO Type          */		//개발삭제 L-line 사용
#define ISO9141_BOSCH							((unsigned long)0x100005) /**< ISO 9141 BOSCH OLD Type             */
#define BENDIX_ABS								((unsigned long)0x100006) /**< BENDIX ABS K, L-LINE dupplex comm.  */
#define NIKKI_LPG								((unsigned long)0x100007) /**< KIA NIKKI LPG KARENS & POTENSHA     */
#define IMMO_SINCHANG							((unsigned long)0x100008) /**< IMMO SINCHANG	    			   */
#define TRW_AIRBAG1								((unsigned long)0x10000C) /**< KIA TRW AIR BAG                     */
#define KEYLESS_SHINCHANG                       ((unsigned long)0x10000D) /**< KEYLESS SHINCHANG                   */
#define KEYLESS_OMRON                           ((unsigned long)0x10000E) /**< KEYLESS OMRON                       */
#define KEYLESS_BOSCH                           ((unsigned long)0x10000F) /**< KEYLESS BOSCH                       */
#define KEYLESS_SHORT_HMC                       ((unsigned long)0x100010) /**< KEYLESS SHORT HMC                   */
#define DELPHI_AIRBAG							((unsigned long)0x100011) /**< XD DELPHI AIR BAG                   */
#define ISO9141_BOSCH_AIRBAG					((unsigned long)0x100012) /**< KIA KB05 BOSCH AIR BAG              */
#define KEYLESS_SHORT_KMC                       ((unsigned long)0x100013) /**< KEYLESS SHORT KMC                   */
#define CANIVAL_ABS                             ((unsigned long)0x100014) /**< CANIVAL ABS 2K PULL UP              */
#define SIEMENS_SIMPLEX_KMC						((unsigned long)0x100016) /**< Siemens simpplex type ex) PRIDE     */
#define ISO9141_ZEXEL							((unsigned long)0x100017) /**< Siemens simpplex type ex) PRIDE     */
#define ISO9141_KIA_IMMO_5BPS					((unsigned long)0x100018) /**< Siemens simpplex type ex) PRIDE     */
#define ISO9141_2_LAN1							((unsigned long)0x100019) /**< Siemens simpplex type ex) PRIDE     */
#define KIA_NIPPONDENSO_OLD_TYPE				((unsigned long)0x10001A) /**< MILTYPE_EMS3_DTC AND TXRX_CURRENT   */
/* 100020~10003F 까지 MIL 타입으로 정의 되어 있음 */
#define MILTYPE_EMS1                            ((unsigned long)0x100020) /**< MIL TYPE DIAGNOSIS NORMAL LOW       */
                                                                          /**< Interval Time : 3.0 Sec             */
                                                                          /**< Code Separation Time : 2 Sec        */
                                                                          /**< Digit Separation Time : 0.5 Sec     */
                                                                          /**< Low Max Time : 5 Sec                */
                                                                          /**< High Max Time : 3 Sec               */
                                                                          /**< Ten Digit Time : 1.5 Sec            */
                                                                          /**< One Digit Time : 0.5 Sec            */
#define MILTYPE_EMS2                            ((unsigned long)0x100021) /**< MIL TYPE DIAGNOSIS : EST            */
#define MILTYPE_EMS3                            ((unsigned long)0x100022) /**< MIL TYPE DIAGNOSIS : ka0s           */
                                                                          /**< Interval Time : 3.0 Sec             */
                                                                          /**< Code Separation Time : 0.5 Sec      */
                                                                          /**< Digit Separation Time : 0.5 Sec     */
                                                                          /**< Low Max Time : 4 Sec                */
                                                                          /**< High Max Time : 2 Sec               */
                                                                          /**< Ten Digit Time : 1.0 Sec            */
                                                                          /**< One Digit Time : 0.5 Sec            */
#define MILTYPE_EMS4                            ((unsigned long)0x100023) /**< MIL TYPE DIAGNOSIS : TCU5 STAREKS   */                                                                          
#define MILTYPE_EMS5                            ((unsigned long)0x100024) /**< MIL TYPE DIAGNOSIS : TCU6           */  
#define MILTYPE_EMS6                            ((unsigned long)0x100029) /**< MIL TYPE DIAGNOSIS : ABSA  0x0055   */
#define MILTYPE_EMS7                            ((unsigned long)0x10002a) /**< MIL TYPE DIAGNOSIS : NOT DEFINE     */
#define MILTYPE_EMS8                            ((unsigned long)0x100027) /**< MIL TYPE DIAGNOSIS : ABS2,ABS3      */
                                                                          /**< Interval Time         : 2.5 Sec     */
                                                                          /**< Code Separation Time  : 2.5 Sec     */
                                                                          /**< Digit Separation Time : 1.5 Sec     */
                                                                          /**< Low Max Time          : 4 Sec       */
                                                                          /**< High Max Time         : 3 Sec       */
                                                                          /**< Ten Digit Time        : 1.7 Sec     */
                                                                          /**< One Digit Time        : 0.5 Sec     */   
                                                                   
#define ISO15765_CARB	                        ((unsigned long)0x100040) /**< ISO15765 USED CARD OBD-II           */
#define ISO15765_CARB_29BIT	                    ((unsigned long)0x100043) /**< ISO15765 USED CARD OBD-II           */// 번호 변경:이종흥 요청
#define ISO15765_CARB_NEW                       ((unsigned long)0x300)	  //LJH CARB CAN 로직 개선(멀티스레드 기능)
#define ISO15765_CARB_NEW_LENGTH                ((unsigned long)0x301) 	  // LJH 길이 포함
#define ISO15765_CARB_29BIT_NEW                 ((unsigned long)0x302)    // LJH 길이 포함
#define J1939_23_CARB_NEW_LENGTH                ((unsigned long)0x311) 	  // LJH 길이 포함
#define J1939_23_CARB_29BIT_NEW                 ((unsigned long)0x312)    // LJH 길이 포함

#define ISO15765_REPRO_PENNIMG	                ((unsigned long)0x100050) /**< ISO15765 repro  USED                */
#define KEYLESS_SHINCHANG_HP                    ((unsigned long)0x100051) /**< KEYLESS SHINCHANG                   */
#define ISO15765_REPRO_PENNIMG_TIME             ((unsigned long)0x100052) /**< 리프로그램 펜딩시 응답타임아웃 P3MAX 값으로 하고 3초간 ACK 송신 */

#define ISO14230_POWERTEC                    	((unsigned long)0x100100) /**< Powertec kyc 2007.06.19             */
#define ISO15765_29BIT                    		((unsigned long)0x100101) /**< CARGO VEHICLE kyc 2007.06.19        */
#define J1939		                    		((unsigned long)0x100102) /**< J1939      kyc 2007.06.19           */
#define RS232		                    		((unsigned long)0x100103) /**< RS232      kyc 2007.06.19           */
#define IS09141_2_ALLISON_ATM		            ((unsigned long)0x100104) /**< TCU4       AJJOOLEE 070816        */
#define WABCO_ABS					            ((unsigned long)0x100106) /**< TCU4       AJJOOLEE 070816        */
#define RS232_MCU					            ((unsigned long)0x100107) /**< HRDT MCU 2.0                      */
#define RS232_MCU_REPRO				            ((unsigned long)0x100108) /**< HRDT MCU 2.0                      */
#define ISO9141_2_DW_SIEMENS		            ((unsigned long)0x100109) /**< DAWOO SIEMES AIRBAG                      */
#define ISO9141_2_NAG				            ((unsigned long)0x100110) /**< SSANGYONG CHAIRMATN TCU NAG               */
#define SMK_CAN					            	((unsigned long)0x100111) /**< SL SMK PROTOCOL ADD 2010.07.13 FROM 황제욱              */
#define ISO15765_29BIT_EXCEPT              		((unsigned long)0x10010a) /**< ISO15765_29BIT 와 같으나 중복되거나 누락된 프레임 있으면 데이터 생략시킴 110609 LWH */
#define J1939_4PGN	                    		((unsigned long)0x10010b) /**< J1939에서 PGN을 4BYTE로 처리하고, TX는 하지 않고 받기만 하는 프로토콜 120925 LWH  */
#define ISO15765_29BIT_REPRO_PENNIMG_TIME       ((unsigned long)0x10010c) /**< 리프로그램 펜딩시 응답타임아웃 P3MAX 값으로 하고 송신 140220 LWH*/

//////////#define DAIHATSU_NON_KWP              ((unsigned long)0x100041) /**< ISO15765 USED CARD OBD-II           */                                                                    
#define DAIHATSU_NON_KWP                        ((unsigned long)0x100041) /**< ISO15765 USED CARD OBD-II           */
#define SUBARU_TxRx_1953	                    ((unsigned long)0x200005) /**< A/M JAPNA INS                   */    
//#define NISSAN_TxRx	                    	((unsigned long)0x200006) /**< A/M JAPNA INS>*/					//개발삭제
#define TOYOTA_CAN	                    		((unsigned long)0x200007) /**< A/M JAPNA INS                  */
//#define ISUZU_J1850VPW						((unsigned long)0x200022) /**< ISUZU ECU, TCU, ABS>*/			//개발삭제
//#define Single_CAN							((unsigned long)0x200024) /**< DW single can*/					//개발삭제
//#define Single_CAN_GM							((unsigned long)0x200026) /**< DW single can 센서출력이 틀리다.*///개발삭제
#define J1939_AM								((unsigned long)0x200033) /**< 상용 OBD를 위해 AM에서 가져옴 130325 LWH*/

#ifdef USE_RELAY_MOSA
#define BAT_RELAY_CON							((unsigned long)0x200035) /** EV Battery Relay Control*/
#define BAT_FD_RELAY_CON						((unsigned long)0x200036) /** FD EV Battery Relay Control*/
#endif


///////////////////////////////////////////////////////////////////////////////
//Ioctl ID Values
#define GET_CONFIG                              ((unsigned long)0x01) /**< To get the vehicle network configuration of the pass-thru device */
#define SET_CONFIG                              ((unsigned long)0x02) /**< To set the vehicle network configuration of the pass-thru device */
#define READ_VBATT                              ((unsigned long)0x03) /**< To direct the pass-thru device to read the voltage on pin 16 of the J1962 connector */
#define FIVE_BAUD_INIT                          ((unsigned long)0x04) /**< To direct the pass-thru device to initiate a 5 baud initialization sequence */
#define FAST_INIT                               ((unsigned long)0x05) /**< To direct the pass-thru device to initiate a fast initialization sequence*/
#define CLEAR_TX_BUFFER                         ((unsigned long)0x07) /**< To direct the pass-thru device to clear all messages in its transmit queze */
#define CLEAR_RX_BUFFER                         ((unsigned long)0x08) /**< To direct the pass-thru device to clear all messages in its receive queue */
#define CLEAR_PERIODIC_MSGS                     ((unsigned long)0x09) /**< To direct the pass-thru device to clear all periodic messages, thus stopping all periodic message transmission */
#define CLEAR_MSG_FILTERS                       ((unsigned long)0x0A) /**< To direct the pass-thru device to clear all message filters, thus stopping all filtering */
#define CLEAR_FUNCT_MSG_LOOKUP_TABLE            ((unsigned long)0x0B) /**< To direct the pass-thru device to clear the Functional Message Look-up Table */
#define ADD_TO_FUNCT_MSG_LOOKUP_TABLE           ((unsigned long)0x0C) /**< To direct the pass-thru device to add a functional address to the Functional Message Look-up Table */
#define DELETE_FROM_FUNCT_MSG_LOOKUP_TABLE		((unsigned long)0x0D) /**< To direct the pass-thru device to delete a functional address from the Functional Message Look-up Table */
#define READ_PROG_VOLTAGE                       ((unsigned long)0x0E) /**< To direct the pass-thru device to read the feedback of the programmable voltage set by PassThruSetProgrammingVoltage */

#define ACK_DISABLE								((unsigned long)0x11000) /**< To direct the pass-thru device to read the feedback of the programmable voltage set by PassThruSetProgrammingVoltage */
#define ACK_ENABLE								((unsigned long)0x11001) /**< To direct the pass-thru device to read the feedback of the programmable voltage set by PassThruSetProgrammingVoltage */
#define ACK_TIMING_CONTROL						((unsigned long)0x11002) /**< ACKTIMING CONTROL  CHJ */

#define FW_EZDSMode_ENABLE						((unsigned long)0x11003) /**< To direct the pass-thru device to read the feedback of the programmable voltage set by PassThruSetProgrammingVoltage */
#define FW_EZDSMode_DISABLE						((unsigned long)0x11004) /**< To direct the pass-thru device to read the feedback of the programmable voltage set by PassThruSetProgrammingVoltage */
#define TRIGG_BAT_CHK							((unsigned long)0x11005) /**< trigger battery status check */
#define LPG_DUTY								((unsigned long)0x11006) /**< DAEWOO LPG DUTY CALCULATE ADD (0:DLC CHECK START, 64:DUTY VALUE) */
//#define USBPort_FIX_ENABLE					((unsigned long)0x11007) /**< only use usb port enable  */
//#define USBPort_FIX_DISABLE					((unsigned long)0x11008) /**< only use usb port disable */
#define CARB_ECU_CNT_CONTROL					((unsigned long)0x11009) /**< CARB Struct Improve for Multi Response 20130717 CHJ */ 
//#define DEBUG_SET_FLAG						((unsigned long)0x1100A) /**< Data Packet & Law Data Debug Flag	20130717 CHJ */
#define CAN_CH_CONTROL							((unsigned long)0x1100B) /**< DLogger - channel setting 20150326 seo  */ 
//#define LINE_CHANNEL_PULSE_CONTROL			((unsigned long)0x1100C) /**< Any line High or Low	150617 LWH*/
#define PERIODIC_MSG_CONTROL					((unsigned long)0x1100D) /**< Periodic Message Control 150811 LWH*/
#define CGW_PASS_ALGORITHM						((unsigned long)0x1100E) /**< 160212 LWH*/

#define ETHERNET_ENABLE                         ((unsigned long)0x11010) /**< ETHERNET ACTIVATION ENABLE*/	
#define ETHERNET_DISABLE						((unsigned long)0x11011) /**< ETHERNET ACTIVATION DISABLE*/	

#define USB_DEVICE_TYPE_MSC						((unsigned long)0x12000) /**<DLogger - USB setting  190509 LWH */

//Reserved for SAE                              0x0F - 0xFFFF
//Tool manufacturer specific                    0x10000 - 0xFFFFFFFF
#define K_LINE_MULTIPLEXER_CONTROL              ((unsigned long)0x10000) /**< To direct the pass-thru device to drive the I+ME K-Line Multiplexer*/
#define READ_VBATT_PIN_1			            ((unsigned long)0x10001) /**< Special for BMW to read the Voltage on OBD Pin 1*/
#define SELFTEST					            ((unsigned long)0x10002) /**< To direct the pass-thru device to do some hardware test operations*/
#define CAN_SET_BTR					            ((unsigned long)0x10003) /**< To direct the pass-thru device to select the can parameters by the BTR register of the CAN-Chip*/
#define CAN_MULTIPLEXER_CONTROL		            ((unsigned long)0x10004) /**< To direct the pass-thru device to select the can physical layer*/




///////////////////////////////////////////////////////////////////////////////
//Ioctl GET_CONFIG / SET_CONFIG Parameter ID Values
#define DATA_RATE                               ((unsigned long)0x01) /**< 5-500000	<br> Represents the desired baud rate. There is no default value. */
#define LOOPBACK                                ((unsigned long)0x03) /**< 0(OFF)-1(ON) <br> 0 = Don't echo transmitted message in the receive queue. 1 = Echo transmitted messages in the receive queue. The default value is OFF. */
#define NODE_ADDRESS                            ((unsigned long)0x04) /**< 0x00-0xFF <br> For a protocol ID of J1850PWM, this sets the node address in the pysical layer of the vehicle network. */
#define NETWORK_LINE                            ((unsigned long)0x05) /**< 0(BUS_NORMAL) 1(BUS_PLUS) 2(BUS_MINUS) <br> For a protocol ID of J1850PWM, this sets the network line(s) that are active during communication (for cases where the pysical layer allows this). The default value is BUS_NORMAL. */
#define P1_MIN                                  ((unsigned long)0x06) /**< 0x0000-0xFFFF <br> For protocol ID of ISO9141, this sets the minimum inter-byte time (in milli-seconds) for ECU respinses. The default value is 0 milli-seconds. */
#define P1_MAX                                  ((unsigned long)0x07) /**< 0x0000-0xFFFF <br> For protocol ID of ISO9141, this sets the maximum inter-byte time (in milli-seconds) for ECU respinses. The default value is 20 milli-seconds. */
#define P2_MIN                                  ((unsigned long)0x08) /**< 0x0000-0xFFFF <br> For protocol ID of ISO9141, this sets the minimum time (in milli-seconds) between tester request and ECU response or two ECU responses. The default value is 25 milli-seconds. */
#define P2_MAX                                  ((unsigned long)0x09) /**< 0x0000-0xFFFF <br> For protocol ID of ISO9141, this sets the maximum time (in milli-seconds) between tester request and ECU response or two ECU responses. The default value is 50 milli-seconds. */
#define P3_MIN                                  ((unsigned long)0x0A) /**< 0x0000-0xFFFF <br> For protocol ID of ISO9141, this sets the minimum time (in milli-seconds) between end of ECU response and start of new tester request. The default value is 55 milli-seconds. */
#define P3_MAX                                  ((unsigned long)0x0B) /**< 0x0000-0xFFFF <br> For protocol ID of ISO9141, this sets the maximum time (in milli-seconds) between end of ECU response and start of new tester request. The default value is 5000 milli-seconds. */
#define P4_MIN                                  ((unsigned long)0x0C) /**< 0x0000-0xFFFF <br> For protocol ID of ISO9141, this sets the minimum inter-byte time (in milli-seconds) for a tester request. The default value is 5 milli-seconds. */
#define P4_MAX                                  ((unsigned long)0x0D) /**< 0x0000-0xFFFF <br> For protocol ID of ISO9141, this sets the maximum inter-byte time (in milli-seconds) for a tester request. The default value is 20 milli-seconds. */
#define W1                                      ((unsigned long)0x0E) /**< 0x0000-0xFFFF <br> For protocol ID of ISO9141, this sets the maximum time (in milli-seconds) from the end of the address byte to the start of the synchronization pattern. The default value is 300 milli-seconds. */
#define W2                                      ((unsigned long)0x0F) /**< 0x0000-0xFFFF <br> For protocol ID of ISO9141, this sets the maximum time (in milli-seconds) from the end of the synchronization pattern to the start of key byte 1. The default value is 20 milli-seconds. */
#define W3                                      ((unsigned long)0x10) /**< 0x0000-0xFFFF <br> For protocol ID of ISO9141, this sets the maximum time (in milli-seconds) between key byte 1 and key byte 2. The default value is 20 milli-seconds. */
#define W4                                      ((unsigned long)0x11) /**< 0x0000-0xFFFF <br> For protocol ID of ISO9141, this sets the maximum time (in milli-seconds) between key byte 2 and its inversion fron the tester. The default value is 50 milli-seconds. */
#define W5                                      ((unsigned long)0x12) /**< 0x0000-0xFFFF <br> For protocol ID of ISO9141, this sets the minimum time (in milli-seconds) before the tester start to transmit the address byte. The default value is 300 milli-seconds. */
#define TIDLE                                   ((unsigned long)0x13) /**< 0x0000-0xFFFF <br> For protocol ID of ISO9141, this sets the amount of bus idle time that is needed before a fast initialization sequence will begin. The default is the value of W5. */
#define TINIL                                   ((unsigned long)0x14) /**< 0x0000-0xFFFF <br> For protocol ID of ISO9141, this sets the duration (in milli-seconds) for the low pulse in fast initialization. The default value is 25 milli-seconds. */
#define TWUP                                    ((unsigned long)0x15) /**< 0x0000-0xFFFF <br> For protocol ID of ISO9141, this sets the duration (in milli-seconds) of the wake-up pulse in fast initialization. Thr default value is 50 milli-seconds. */
#define PARITY                                  ((unsigned long)0x16) /**< 0(NO_PARITY) 1(ODD_PARITY) 2(EVEN_PARITY) <br> For protocol ID of ISO9141 only. The default value is NO_PARITY. */
#define BIT_SAMPLE_POINT                        ((unsigned long)0x17) /**< 0-100 <br> For a protocol ID of CAN, this sets the desired bit sample point as a percentage of the bit time. The default is 80%. */
#define SYNC_JUMP_WIDTH                         ((unsigned long)0x18) /**< 0-100 <br> For a protocol ID of CAN, this sets the desired synchronization jump width as a percentage of the bit time. The default is 15%. */
//Reserved for SAE                              ((unsigned long)0x19)
#define T1_MAX                                  ((unsigned long)0x1A) /**< 0x0000-0xFFFF <br> For protocol ID of SCI_A_ENGINE, SCI_A_TRANS, SCI_B_ENGINE or SCI_B_TRANS, this sets the maximum interframe response delay. The default value is 20 milli-seconds. */
#define T2_MAX                                  ((unsigned long)0x1B) /**< 0x0000-0xFFFF <br> For protocol ID of SCI_A_ENGINE, SCI_A_TRANS, SCI_B_ENGINE or SCI_B_TRANS, this sets the maximum interframe request delay. The default value is 100 milli-seconds. */
#define T4_MAX                                  ((unsigned long)0x1C) /**< 0x0000-0xFFFF <br> For protocol ID of SCI_A_ENGINE, SCI_A_TRANS, SCI_B_ENGINE or SCI_B_TRANS, this sets the maximum intermessage response delay. The default value is 20 milli-seconds. */
#define T5_MAX                                  ((unsigned long)0x1D) /**< 0x0000-0xFFFF <br> For protocol ID of SCI_A_ENGINE, SCI_A_TRANS, SCI_B_ENGINE or SCI_B_TRANS, this sets the maximum intermessage request delay. The default value is 100 milli-seconds. */
#define ISO15765_BS                             ((unsigned long)0x1E) /**< 0x0000-0xFF <br> For protocol ID of ISO15765, this sets the block size for segmented transfers. The default value is 0. Default value or value set by the application may be overridden by interface to match the capabilities of the interface. */
#define ISO15765_STMIN                          ((unsigned long)0x1F) /**< 0x0000-0xFF <br> For protocol ID of ISO15765, this sets the separation time for segmented transfers. The default value is 0. Default value or value set by the application may be overridden by interface to match the capabilities of the interface. */
#define BSTX									((unsigned long)0x20) /**< LWH 추가 */
#define STMINTX									((unsigned long)0x21) /**< LWH 추가 */
#define DATABITS								((unsigned long)0x22) /**< LWH 추가 */
#define FIVEBAUDMOD								((unsigned long)0x23) /**< LWH 추가 */
#define TOOLMANUFACTURERSPEC					((unsigned long)0x24) /**< LWH 추가 */
#define ETC1									((unsigned long)0x25) /**< LWH 추가 */
#define ETC2									((unsigned long)0x26) /**< LWH 추가 */
#define ETC3									((unsigned long)0x27) /**< LWH 추가 */
#define ETC4									((unsigned long)0x28) /**< LWH 추가 */
#define ETC5									((unsigned long)0x29) /**< LWH 추가 */
#define PENDING_ACK								((unsigned long)0x2A) /**< LWH 추가 - DB j2534setconfig 에 없는 항목  */
//Reserved for SAE                              0x20 - 0xFFFF
//Tool manufacturer specific                    0x10000 - 0xFFFFFFFF


//Ioctl parameter PARITY values
#define NO_PARITY                               0 /**< value for PassThruIoctl subfunction GET_CONFIG / SET_CONFIG parameter PARITY */
#define ODD_PARITY                              1 /**< value for PassThruIoctl subfunction GET_CONFIG / SET_CONFIG parameter PARITY */
#define EVEN_PARITY                             2 /**< value for PassThruIoctl subfunction GET_CONFIG / SET_CONFIG parameter PARITY */

///////////////////////////////////////////////////////////////////////////////
//Ioctl parameter LOOPBACK values
#define OFF                                     0 /**< value for PassThruIoctl subfunction GET_CONFIG / SET_CONFIG parameter LOOPBACK */
#define ON                                      1 /**< value for PassThruIoctl subfunction GET_CONFIG / SET_CONFIG parameter LOOPBACK */



/*******************************/
/* Configuration Parameter IDs */
/*******************************/
#define CONF_ID_DATA_RATE					0x01
// unused							0x02
#define CONF_ID_LOOPBACK					0x03
#define CONF_ID_NODE_ADDRESS				0x04
#define CONF_ID_NETWORK_LINE				0x05
#define CONF_ID_P1_MIN						0x06 // Don't use
#define CONF_ID_P1_MAX						0x07
#define CONF_ID_P2_MIN						0x08 // Don't use
#define CONF_ID_P2_MAX						0x09 // Don't use
#define CONF_ID_P3_MIN						0x0A
#define CONF_ID_P3_MAX						0x0B // Don't use
#define CONF_ID_P4_MIN						0x0C
#define CONF_ID_P4_MAX						0x0D // Don't use
// See W0 = 0x19
#define CONF_ID_W1							0x0E
#define CONF_ID_W2							0x0F
#define CONF_ID_W3							0x10
#define CONF_ID_W4							0x11
#define CONF_ID_W5							0x12
#define CONF_ID_TIDLE						0x13
#define CONF_ID_TINIL						0x14
#define CONF_ID_TWUP						0x15
#define CONF_ID_PARITY						0x16
#define CONF_ID_BIT_SAMPLE_POINT			0x17
#define CONF_ID_SYNC_JUMP_WIDTH				0x18
#define CONF_ID_W0							0x19
#define CONF_ID_T1_MAX						0x1A
#define CONF_ID_T2_MAX						0x1B
// See T3_MAX						0x24
#define CONF_ID_T4_MAX						0x1C
#define CONF_ID_T5_MAX						0x1D
#define CONF_ID_ISO15765_BS					0x1E
#define CONF_ID_ISO15765_STMIN				0x1F
#define CONF_ID_DATA_BITS					0x20
#define CONF_ID_FIVE_BAUD_MOD				0x21
#define CONF_ID_BS_TX						0x22
#define CONF_ID_STMIN_TX					0x23
#define CONF_ID_T3_MAX						0x24
#define CONF_ID_ISO15765_WFT_MAX			0x25
// Future -1 values (not approved yet)
#define CONF_ID_N_BR_MIN			        0x2A
#define CONF_ID_ISO15765_PAD_VALUE          0x2B
#define CONF_ID_N_AS_MAX			        0x2C
#define CONF_ID_N_AR_MAX			        0x2D
#define CONF_ID_N_BS_MAX			        0x2E
#define CONF_ID_N_CR_MAX			        0x2F
#define CONF_ID_N_CS_MIN			        0x30


/*********************/
/* Message Structure */
/*********************/
#define SIZE_PASSTHRU_HEADER			(sizeof(unsigned long) * 6)

#define MAX_PASSTHRUMSG_DATA_SIZE			4128

typedef struct _stPASSTHRU_MSG
{
    unsigned long ProtocolID; 	/**< Protocol type */
    unsigned long RxStatus; 	/**< Receive message status - See RxStatus in "Message Flags and Status Definition" section */
    unsigned long TxFlags; 		/**< Transmit message flags - See TxFlags in "Message Flags and Status Definition" section */
    unsigned long Timestamp; 	/**< Received message timestamp (microseconds) */
    unsigned long DataSize; 	/**< Data size in bytes */
    unsigned long ExtraDataIndex; /**< Start position of extra data in received message (e.g., IFR; CRC; checksum, ...). The extra data bytes follow the body bytes in the Data array. The index is zero-based. */
	unsigned char pData[MAX_PASSTHRUMSG_DATA_SIZE]; /**< Array of data bytes. */
	UUID_Struct UUID;
} stPASSTHRU_MSG, PTmsgPkt_t;

/********************/
/* IOCTL Structures */
/********************/
typedef struct _stSCONFIG
{
    unsigned long Parameter;    /**< name of parameter */
    unsigned long Value;        /**< value of the parameter */
}stSCONFIG, SCONFIG;

typedef struct _stSCONFIG_LIST
{
    unsigned long NumOfParams;     /**< number of SCONFIG elements */
    SCONFIG *ConfigPtr;            /**< array of SCONFIG */
}stSCONFIG_LIST, SCONFIG_LIST;		//Get_Config, Set_Config, 

typedef struct _stSBYTE_ARRAY
{
     unsigned long NumOfBytes;      /**< number of bytes in the array */
     unsigned char *BytePtr;        /**< array of bytes */
}stSBYTE_ARRAY, SBYTE_ARRAY;		//Five Baud Init, Add_To_Funct_MSG_LookUP_TABLE, Delete_From_Funct_MSG_LookUP_TABLE

typedef struct
{
	unsigned long Parameter;
	unsigned long Value;
    unsigned long Supported;
} stSPARAM, SPARAM;

typedef struct
{
	unsigned long NumOfParams;
	SPARAM *ParamPtr;
} stSPARAM_LIST, SPARAM_LIST;





#endif //_GIT_PASSTHRU_DEFINES_H

