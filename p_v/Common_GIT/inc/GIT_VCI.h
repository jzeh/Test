/**
  ******************************************************************************
  * @file    GIT_VCI.h
  * @author  GIT Application Team by james jean
  * @version V 1.0
  * @date    26-MAR-2014
  * @brief   Header for GIT_VCI.h module
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GIT_VCI_H__
#define __GIT_VCI_H__

#include <string.h>
#include "common.h"
#include "GIT_PassthruDefines.h"
//#include "J2534_1_define.h"
//#include "J2534_2_define.h"
#include "Autolink_Manager.h"

//#define FIRMWARE_VERSION_STR "Ver %.2f, " __DATE__ " " __TIME__
#define FIRMWARE_VERSION_STR __DATE__ " " __TIME__
#define FIRMWAER_VERSION_NUMBER 2.16

#define COUNT_AUTOVIN_REQ_PACKET        4   

#define	HIGH_CAN_CH				1
#define LOW_CAN_CH 				2
#define CAN_CHANNEL_1			1
#define CAN_CHANNEL_2			2

#define STANDARD_CAN			0x01
#define EXTENDED_CAN			0x02
#define STANDARD_EXTENDED_CAN   0x03

#define Highcan1  			1
#define Highcan2  			2
#define Highcan3    		3
#define Lowcan1   			4
#define Flexray    			5

#define NormalHigh 			1
#define NormalLow  			0
#define Pulse 				1
#define Serial 				2

#define KCh07 				1
#define KCh08 				2
#define KCh11 				3
#define KCh12 				4

#define OBD1 				1
#define OBD2 				2

#define Pull510 			1
#define Pull_NC 			0

#define HIGH			(U32)0x0001		//high status 130806 LWH 추가
#define LOW				(U32)0x0000		//low status  130806 LWH 추가
#define KCh				(U32)0x0001		//130715 LWH 추가

#if 1//defined(FEATURE_GDS_VCI_AM)
//========================DLC setting start====================================//
//1.CommRelay
#define OFF_RELAY		(U32)0x0000
#define ADIN_RELAY		(U32)0x1080		//AD K-LINE,L-LINE Relay Select
#define UART_RELAY		(U32)0x1000		//RS232C 상용통신 Relay Select
#define J1850_RELAY		(U32)0x1300		//J1850 Relay Select
#define J1708_RELAY		(U32)0x1200		//J1708(485) Relay Select
#define CANLOW_RELAY	(U32)0x1C00		//Can Low Relay Select
#define CANHIGH_RELAY	(U32)0x1800		//Can High Relay Select
#define CANSIG_RELAY	(U32)0x1c80		//GPEport
#define CANSIG_ON		(U32)0x2000		//GPAport
#define CANSIG_OFF		(U32)0xdfff		//GPAport
#define KLLINE_RELAY	(U32)0x0000		//K-line,L-line Relay Select

//2.KlineSelect
#define K_SERIAL		(U32)0x0000		//SERIAL(RS232C) select
#define K_PULSE			(U32)0x0001		//Pulse select
//3.KlineStatus
#define K_NORMAL		(U32)0x0010		//dlc 출력단을 high 설정
#define K_INVERSE		(U32)0x0000		//dlc 출력단을 low 설정
//4.LlineSelect
#define L_SERIAL		(U32)0x0000		//SERIAL(RS232C) select
#define L_PULSE			(U32)0x0002		//Pulse select
//5.LlineStatus
#define L_NORMAL		(U32)0x0020		//dlc 출력단을 high 설정
#define L_INVERSE		(U32)0x0000		//dlc 출력단을 low 설정
//6.KlineSwitchStatus
#define K_PULSE_HIGH	(U32)0x0004		//Pulse초기 high출력설정
#define K_PULSE_LOW		(U32)0x0000		//Pulse초기 low출력설정
//7.LlineSwitchStatus
#define L_PULSE_HIGH	(U32)0x0008		//Pulse초기 high출력설정
#define L_PULSE_LOW		(U32)0x0000		//Pulse초기 low출력설정
//8.RxLineSelect
#define RXD_L_FB		(U32)0x0000		//L-line receive
#define RXD_OBD1		(U32)0x0002		//K-line 2.54V(REF) receive
#define RXD_OBD2		(U32)0x0008		//K-line OBD-II receive
#define RXD_485			(U32)0x0003		//J1708 receive
#define RXD_RS232		(U32)0x0004		//RS232C receive
#define RXD_HIGHCAN		(U32)0x0020		//HIGH CAN 250K~1M
#define RXD_LOWCAN		(U32)0x0000		//LOW CAN ~200K
#define RXD_SIGCAN		(U32)0x0001		//Single CAN high line
#define RXD_J1850		(U32)0x0000		//J1850 receive
//9.RxLineStatus
#define RXD_NORMAL		(U32)0x0000		//RXD 입력단을 high 설정
#define RXD_INVERSE		(U32)0x0040		//RXD 입력단을 LOW 설정
//10.PullupRelay
#define Pullup			(U32)0x0000		//Pull-up
#define Pulldown		(U32)0x0010		//Pull-down
//11.KlinePullup
#define K_Pullup_Off	(U32)0x0000
#define K_Pullup_510	(U32)0x0001		//k-line 510 select(+Batt)
#define K_Pullup_47k	(U32)0x0004		//k-line 4.7k select(+Batt)
#define K_Pullup_2k		(U32)0x0020		//k-line 2k select(+5V)
//12.LlinePullup
#define L_Pullup_Off	(U32)0x0000
#define L_Pullup_510	(U32)0x0002		//L-line 510 select(+Batt)
#define L_Pullup_47k	(U32)0x0008		//L-line 4.7k select(+Batt)
#define L_Pullup_2k		(U32)0x0040		//L-line 2k select(+5V)
//13.LineGnd
//#define L_GND_ON		(U32)0x0080
//#define L_GND_OFF		(U32)0x0000

///////////////////////////////////////////////////
	// Use a auto flow control(cts,rts)

	#define DLC_VERSION_ES2     1	//ES1 =0
///////////////////////////////////////////////////

#if DLC_VERSION_ES2

//===================(ES2 DEFINE)================
	//14.K-Line CH
	#define K_ChOff		(U32)0x0000			//K-Line CH all clear(Open)
	#define K_Ch00		(U32)0x0080			//GPCport
	#define K_Ch01		(U32)0x0081
	#define K_Ch02		(U32)0x00C0
	#define K_Ch03		(U32)0x00C1
	#define K_Ch04		(U32)0x0090
	#define K_Ch05		(U32)0x0092
	#define K_Ch06		(U32)0x00D0
	#define K_Ch07		(U32)0x00D2
	#define K_Ch08		(U32)0x00A0
	#define K_Ch09		(U32)0x00A4
	#define K_Ch10		(U32)0x00E0
	#define K_Ch11		(U32)0x00E4
	#define K_Ch12		(U32)0x0040
	#define K_Ch13		(U32)0x0048

	//15.L-Line CH
	#define L_ChOff		(U32)0x0000			//L-Line CH all clear(Open)
	#define L_Ch00		(U32)0x0080			//GPDport
	#define L_Ch01		(U32)0x0081
	#define L_Ch02		(U32)0x00C0
	#define L_Ch03		(U32)0x00C1
	#define L_Ch04		(U32)0x0090
	#define L_Ch05		(U32)0x0092
	#define L_Ch06		(U32)0x00D0
	#define L_Ch07		(U32)0x00D2
	#define L_Ch08		(U32)0x00A0
	#define L_Ch09		(U32)0x00A4
	#define L_Ch10		(U32)0x00E0
	#define L_Ch11		(U32)0x00E4
	#define L_Ch12		(U32)0x0040
	#define L_Ch13		(U32)0x0048

#else
	//===================(ES1 DEFINE)================
	//14.K-Line CH
	#define K_ChOff		(U32)0x0080			//K-Line CH all clear(Open)
	#define K_Ch00		(U32)0x0000			//GPCport
	#define K_Ch01		(U32)0x0001
	#define K_Ch02		(U32)0x0040
	#define K_Ch03		(U32)0x0041
	#define K_Ch04		(U32)0x0010
	#define K_Ch05		(U32)0x0012
	#define K_Ch06		(U32)0x0050
	#define K_Ch07		(U32)0x0052
	#define K_Ch08		(U32)0x0020
	#define K_Ch09		(U32)0x0024
	#define K_Ch10		(U32)0x0060
	#define K_Ch11		(U32)0x0064
	#define K_Ch12		(U32)0x00C0
	#define K_Ch13		(U32)0x00C8

	//15.L-Line CH
	#define L_ChOff		(U32)0x0080			//L-Line CH all clear(Open)
	#define L_Ch00		(U32)0x0000			//GPDport
	#define L_Ch01		(U32)0x0001
	#define L_Ch02		(U32)0x0040
	#define L_Ch03		(U32)0x0041
	#define L_Ch04		(U32)0x0010
	#define L_Ch05		(U32)0x0012
	#define L_Ch06		(U32)0x0050
	#define L_Ch07		(U32)0x0052
	#define L_Ch08		(U32)0x0020
	#define L_Ch09		(U32)0x0024
	#define L_Ch10		(U32)0x0060
	#define L_Ch11		(U32)0x0064
	#define L_Ch12		(U32)0x00C0
	#define L_Ch13		(U32)0x00C8
#endif
//==================================================
//16.Rprogram CH
#define R_ChOff		(U32)0x0000			//Reprogram all clear(Open)
#define R_Ch03		(U32)0x3800			//GPFport
#define R_Ch06		(U32)0x3900
#define R_Ch08		(U32)0x3400
#define R_Ch09		(U32)0x3600
#define R_Ch10		(U32)0x3C00
#define R_Ch11		(U32)0x3E00
#define R_Ch02		(U32)0x3100

//Veh (Speed) CH
#define V_ChOff		(U32)0x0000			//all clear(Open)
#define V_Ch03		(U32)0x2800			//GPFport
#define V_Ch06		(U32)0x2900
#define V_Ch08		(U32)0x2400
#define V_Ch09		(U32)0x2600
#define V_Ch10		(U32)0x2C00
#define V_Ch11		(U32)0x2E00
#define V_Ch02		(U32)0x2100
//========================END====================================//
//ReproLineStatus
#define Vss_Output		(U32)0x0040		//Veh signal output
#define Clk_Output		(U32)0x0080		//Baudrate cleck output
#define REPROG_OUT	 	(U32)0x0400			//Reprogram Output ON


#define HIGH			(U32)0x0001		//high status
#define LOW				(U32)0x0000		//low status
#define PULSE			(U32)0x0001		//pulse status
#define SERIAL			(U32)0x0000		//serial status

#define KCh				(U32)0x0001
#define LCh				(U32)0x0002
#define KLCh			(U32)0x0000

//#define KPulse			(U32)0x0001
//#define LPulse			(U32)0x0002

#define NOTUSE			(U32)0x0000
#define M40				(U32)0x0000
#define M50				(U32)0x0000		//50.7MHz

//BPS
#define BAUD_61         51945	//(50.7MHz/(BAUD*16))-1
#define BAUD_66         48010
#define BAUD_244        12985
#define BAUD_250        12674
#define BAUD_488        6392
#define BAUD_1000       3167
#define BAUD_1200       2439
#define BAUD_1953       1621
#define BAUD_2400       1319
#define BAUD_3600       879
#define BAUD_4800       659
#define BAUD_7200       439
#define BAUD_7812       404
#define BAUD_8193       385
#define BAUD_9600       329
#define BAUD_10400      303

#define BAUD_10416      303
#define BAUD_10285      307
#define BAUD_12480      252
#define BAUD_14400      219
#define BAUD_15600		202
#define BAUD_19200      164
#define BAUD_28800      109
#define BAUD_38400      81
#define BAUD_41600      75
#define BAUD_57600      54
#define BAUD_62500      49
#define BAUD_112000     25
#define BAUD_128000     23

#define SM9600         165
#define SM10416        152

#endif	// defined(STM32F2XX)

#if defined(HS_MODIFY)
#define R_ChOff		(U32)0x0000			//Reprogram all clear(Open)
#define R_Ch03		(U32)0x3800			//GPFport
#define R_Ch06		(U32)0x3900
#define R_Ch08		(U32)0x3400
#define R_Ch09		(U32)0x3600
#define R_Ch10		(U32)0x3C00
#define R_Ch11		(U32)0x3E00
#define R_Ch02		(U32)0x3100
#endif
//////////////////////////////////////////////////////////////////////////////////////////

////////////////// Host Com Parsing & Data Process Value Defined ///////////////
#define HOST_BUFFER_EMPTY											0x00
#define HOST_BUFFER_IS												0x01
#define HOST_FRAME_COMPLETED										0x02
#define HOST_FRAME_REMAINED										0x03
//////////////////////////////////////////////////////////////////////////


//////////////////  Error Code Value Defined  /////////////////////////////////
#define WLAN_DATA_IS_VALID										0x00
#define RESPONSE_DATA_ALREADY_DONE								0x00
#define WORK_SUCCESS												0x01
#define HOST_CHECKSUM_ERROR										0x02
#define HOST_DATA_WRONG											0x03
#define CAN_MASK_SET_ERROR										0x04
#define FW_ERASER_MEM_ERROR										0x05			// MCU Internal Flash Memory Eraser Fail Error
#define FW_WRITING_MEM_ERROR										0x06
#define FW_VERIFY_MEM_ERROR										0x07
#define VEHICLE_COM_HW_SET_ERROR									0x08
#define HOST_DATA_LENGTH_ERROR									0x09
#define WLAN_BT_SPI_RX_TIMEOUT									0x0A
#define WLAN_BT_SPI_TX_ERROR										0x0B
#define WLAN_BT_SPI_RX_ERROR										0x0C
#define WLAN_BT_SPI_RX_CMD_ERROR									0x0D
#define WLAN_BT_SPI_FRAME_ERROR									0x0E
#define WLAN_BT_SPI_CHECKSUM_ERROR								0x0F
#define WLAN_BT_SPI_RX_LENGTH_ERROR								0x10
#define WLAN_NAK_RESPONSE											0x11
#define WLAN_DATA_ERROR											0x12
#define DLC_ADC_CONV_TIMEOUT										0x13
#define CAN_TX_ERROR												0x14
////////////////////////////////////////////////////////////////////////////

//////////////////  VCI Interface Header Value Defined ////////////////////////
#define HOST_HEADER				0xF1
#define WLANBT_HEADER				0xF3
#define SBOXII_HEADER				0xF7
//////////////////////////////////////////////////////////////////////////////


//////////////  Wireless LAN Setting & Communication Command Defined //////////////////////
#define WLAN_ALL_IP_SET							0x30
#define WLAN_SERVER_IP_SET							0x31
#define WLAN_SSID_SET								0x32
#define WLAN_MAC_ADDR_SET							0x33
#define WLAN_SECURITY_SET							0x34
#define WLAN_OPERATION_SET						0x35
#define WLAN_BAND_SET								0x36
#define WLAN_SERVER_CONNECT_SET					0x37
#define WLAN_SLEEP_MODE_SET						0x38
#define WLAN_SOFTWARE_RESET						0x39
#define WLAN_WIRELESS_SEARCH_INQUIRY				0x3A
#define WLAN_STATUS_INQUIRY						0x3B
/////////////////////////////////////////////////////////////////////////////////////



/////// SmartBox-II to Tobelco Wireless LAN Setting Protocol Command Defined //////////////////////
#define SPI_WLAN_ALL_IP_SET							0xC010
#define SPI_WLAN_SERVER_IP_SET						0xC011
#define SPI_WLAN_SSID_SET								0xC012
#define SPI_WLAN_MAC_ADDR_SET						0xC013
#define SPI_WLAN_SECURITY_SET							0xC014
#define SPI_WLAN_OPERATION_SET						0xC015
#define SPI_WLAN_BAND_SET								0xC016
#define SPI_WLAN_SERVER_CONNECT_SET					0xC017
#define SPI_WLAN_SLEEP_MODE_SET						0xC118
#define SPI_WLAN_WIRELESS_SEARCH_INQUIRY			0xC020
#define SPI_WLAN_WIRELESS_STATUS_INQUIRY			0xC021
#define SPI_WLAN_SOFTWARE_RESET						0xC110

#define ACK_RESPONSE					0x0A
#define NAK_RESPONSE					0x09

//////////////////  DVT.c Used Defined  ////////////////////////////////////////
#define HOST_COM_CHECK_COUNTOUT				3000
#define HOST_COM_TIMEOUT_VALUE				1000				// about 1000 ms
#define MAX_HOST_COM_BUFF_SIZE				0x1000		// 4Kbyte
#define MAX_HOST_COM_BUFF_SCALE				MAX_HOST_COM_BUFF_SIZE-1

#define MAX_DLC_COM_BUFF_SIZE					0x1000			// 4096
//////////////////-/////////////////////////////////////////////////////////

//// 2534comm.c 에도 사용하기 위해 userDefine.h로 옮김	130607 LWH
#define WLAN_BT_FRAME_FIXED_LENGTH				252//256
#define WLAN_BT_SET_BUFF_MAX_LENGTH				256
#define WLAN_BT_SET_TIMEOUT_VALUE				1000

#define COMM_UART1		1
#define COMM_UART2		2  //Wlan
#define COMM_USB		3  //USB Code Change


#define PASSTHRUMSG_HEADER_SIZE				(24) //sizeof(unsigned long) * 6) /* ProtocolID + RxStatus + TxFlags + Timestamp + DataSize + ExtraDataIndex */

typedef __packed struct _stRECORD_HW_SET
{
	unsigned long	nCommRelay;
	unsigned long	nKlineSelect;
	unsigned long	nKlineStatus;
	unsigned long	nLlineSelect;
	unsigned long	nLlineStatus;
	unsigned long	nKlineSwitchStatus;
	unsigned long	nLlineSwitchStatus;
	unsigned long	nRxLineSelect;
	unsigned long	nRxLineStatus;
	unsigned long	nPullupRelay;
	unsigned long	nKlinePullup;
	unsigned long	nLlinePullup;
	unsigned long	nLlineGnd;
	unsigned long	nKlineCh;
	unsigned long	nLlineCh;
	unsigned long	nRProgramCh;
	unsigned long	nEtc1;
	unsigned long	nEtc2;
	unsigned long	nEtc3;
	unsigned long	nEtc4;
	unsigned long	nEtc5;
	unsigned char   ackmessage[20];
} stRECORD_HW_SET, tagHARDWARESET;

typedef __packed struct
{				// IOCTL Config Parameters in J2534 (Figure 23 in J2534)
	unsigned long nDataRate;				// desired baud rate (5~500000, no default value)
	unsigned long nLoopBack;				// 수신 메시지에 대한 에코 여부 (0:off(default), 1:on)
	unsigned long nNodeAddress;				// J1850PWM에서의 node address 지정 (00~FF 사이의 값)
	unsigned long nNetworkLine;				// J1850PWM에서의 통신중 active한 network line 지정 (0:bus normal(default), 1:bus plus, 2:bus minus)
	unsigned long nP1Min;					// ISO9141에서의 ECU응답의 unsigned longer-byte time의 최소값					(0~FFFF, 단위: milli-seconds) 기본값: 0
	unsigned long nP1Max;					// ISO9141에서의 ECU응답의 unsigned longer-byte time의 최대값					(0~FFFF, 단위: milli-seconds) 기본값: 20
	unsigned long nP2Min;					// ISO9141에서의 tester request와 ECU응답 사이 time의 최소값						(0~FFFF, 단위: milli-seconds) 기본값: 25
	unsigned long nP2Max;					// ISO9141에서의 tester request와 ECU응답 사이 time의 최대값						(0~FFFF, 단위: milli-seconds) 기본값: 50
	unsigned long nP3Min;					// ISO9141에서의 ECU응답의 끝과 다음 tester request 시작 사이 time의 최소값			(0~FFFF, 단위: milli-seconds) 기본값: 55
	unsigned long nP3Max;					// ISO9141에서의 ECU응답의 끝과 다음 tester request 시작 사이 time의 최대값			(0~FFFF, 단위: milli-seconds) 기본값: 5000
	unsigned long nP4Min;					// ISO9141에서의 tester request의 unsigned longer-byte time의 최소값			(0~FFFF, 단위: milli-seconds) 기본값: 5
	unsigned long nP4Max;					// ISO9141에서의 tester request의 unsigned longer-byte time의 최대값			(0~FFFF, 단위: milli-seconds) 기본값: 20
//	unsigned long nW0;						// ISO9141에서의 address byte 끝 ~ synchronization pattern 시작 사이의 시간 최대값	(0~FFFF, 단위: milli-seconds) 기본값: 300
	unsigned long nW1;						// ISO9141에서의 address byte 끝 ~ synchronization pattern 시작 사이의 시간 최대값	(0~FFFF, 단위: milli-seconds) 기본값: 300
	unsigned long nW2;						// ISO9141에서의 synchronization pattern 끝 ~ key byte 1 시작 사이의 시간 최대값	(0~FFFF, 단위: milli-seconds) 기본값: 20
	unsigned long nW3;						// ISO9141에서의 key byte 1 ~ key byte 2 사이의 시간 최대값							(0~FFFF, 단위: milli-seconds) 기본값: 20
	unsigned long nW4;						// ISO9141에서의 key byte 2 ~ tester로부터의 inversion 사이의 시간 최대값			(0~FFFF, 단위: milli-seconds) 기본값: 50
	unsigned long nW5;						// ISO9141에서의 address byte 전송을 위한 tester 시작 전에 필요한 최소 시간값		(0~FFFF, 단위: milli-seconds) 기본값: 300
	unsigned long nTIdle;					// ISO9141에서의 Fast Init. 시작을 위하여 필요한 bus idle time						(0~FFFF, 단위: milli-seconds) 기본값: nW5의 값
	unsigned long nTInil;					// ISO9141에서의 Fast Init.에서 low pulse의 작동시간								(0~FFFF, 단위: milli-seconds) 기본값: 25
	unsigned long nTWUp;					// ISO9141에서의 Fast Init.에서 Wake-up pulse의 작동시간							(0~FFFF, 단위: milli-seconds) 기본값: 50
	unsigned long nParity;					// ISO9141에서의 Parity방식 (0:No Parity(default), 1:Odd Parity, 2:Even Parity)
	unsigned long nBitSamplePoint;			// desired bit samle pounsigned long를 표현하는 bit time의 percentage (0~100 (%), 기본값: 80)
	unsigned long nSyncJumpWidth;			// desired Synchronization jump width를 표현하는 bit time의 percentage (0~100 (%), 기본값: 15)
	unsigned long nT1Max;					// SCI Type에서의 unsigned longer-frame resonse delay의 최대값 (0~FFFF, 단위: milli-seconds) 기본값: 20
	unsigned long nT2Max;					// SCI Type에서의 unsigned longer-frame request delay의 최대값 (0~FFFF, 단위: milli-seconds) 기본값: 100
	unsigned long nT3Max;					// --- unused
	unsigned long nT4Max;					// SCI Type에서의 unsigned longer-message resonse delay의 최대값 (0~FFFF, 단위: milli-seconds) 기본값: 20
	unsigned long nT5Max;					// SCI Type에서의 unsigned longer-message request delay의 최대값 (0~FFFF, 단위: milli-seconds) 기본값: 100
	unsigned long nIso15765BS;				// ISO15765에서의 segmented transfer을 위한 block size (기본값: 0)
	unsigned long nIso15765STMin;			// ISO15765에서의 segmented transfer에서의 separation time  (기본값: 0)
	unsigned long nBSTx;					// --- 0~FF, FFFF (ISO15765-2 참조)
	unsigned long nSTMinTx;					// --- 0~FF, FFFF (ISO15765-2 참조)
	unsigned long nDataBits;				// --- 0:8 data bit, 1: 7 data bit
	unsigned long nFiveBaudMod;				// --- 0~3 (별도표 참조)
	unsigned long nToolManufacturerSpec;	//
	unsigned long nEtc1;					// can delay 변수로 사용함.
	unsigned long nEtc2;					// can ID offset 변수로 사용함.
	unsigned long nEtc3;					//
	unsigned long nEtc4;
	unsigned long nEtc5;					// can ID mask 변수로 사용함.

} stGITSetConfig, tagJ2534SETCONFIG;

#define WORK_SUCCESS												0x01
#define CAN_MASK_SET_ERROR										0x04



extern stGITSetConfig g_stGITSetConfig;


//////////////////  Error Code Value Defined  /////////////////////////////////


/**(VCI 동작 함수)*****************************************************************/
void VCI_Initialize(void);
void VCI_Clear_DLC_HW(void);
//void VCI_HW_Setting(unsigned long uiPID);

void VCI_DLC_HW_Set(U8 CommRelay,U8 KlineSelect,U8 KlineStatus,U8 RxLineStatus,U8 KlinePullup,U8 KlineChSet);
bool VCI_FunctionClassification(U8 FunctionType, U8 ParaCounter);
bool VCI_ProtocolClassification(unsigned char *pData, unsigned int uiDataLen);

void VCI_SendtoECUCommManager(void);

void VCI_GetRtcTime(unsigned char *pSetDate);

/**(Passthru 제어 함수 )*****************************************************************/
void VCI_SetPassThruDeviceID(unsigned char *pData);
unsigned long VCI_GetPassThruDeviceID(void);
void VCI_SetPassThruProtocolID(unsigned char *pData);
unsigned long VCI_GetPassThruProtocolID(void);
void VCI_SetPassThruConnectFlags(unsigned char *pData);
unsigned long VCI_GetPassThruConnectFlags(void);
void VCI_SetPassThruBaudrate(unsigned char *pData);
unsigned long VCI_GetPassThruBaudrate(void);
void VCI_REINTI_COMM_STATE(unsigned long ulPID);


void VCI_SetPassThruWirteMsgTimeout(unsigned char *pData, unsigned int uiDataLen);

//void VCI_SW_Reset(unsigned short mscnt);

unsigned long VCI_SetProVoltage(unsigned long ulPinNumber, unsigned long ulVoltage);

#endif /* __GIT_VCI_H__ */

/***************************** END OF FILE ****/
