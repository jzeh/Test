#ifndef __GIT_VCI_H__
#define __GIT_VCI_H__

/*----------------------------------------------------------------------
	includes
----------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 *   define
 *--------------------------------------------------------------------*/
#define	HIGH_CAN_CH								1
#define LOW_CAN_CH 								2
#define CAN_CHANNEL_1							1
#define CAN_CHANNEL_2							2
#define STANDARD_CAN							0x01
#define EXTENDED_CAN							0x02

#define KLine    								0
#define Highcan1  								1
#define Highcan2  								2
#define Lowcan1    								3
#define Lowcan2   								4
#define Flexray    								5

#define NormalHigh 								1
#define NormalLow  								0
#define Pulse 									1
#define Serial 									2

#define KCh07 									1
#define KCh08 									2
#define KCh11 									3
#define KCh12 									4

#define OBD1 									1
#define OBD2 									2

#define Pull510 								1
#define Pull_NC 								0

#define HIGH									(U32)0x0001
#define LOW										(U32)0x0000
#define KCh										(U32)0x0001

#define MAX_PERIODICMSG_CNT	5
#define MAX_RX_BUFF_SIZE						2000			//K라인 수신버퍼 최대 사이즈(무한수신 방지) 20180426 KKT
//========================DLC setting start====================================//
//1.CommRelay
#define OFF_RELAY								(U32)0x0000
#define ADIN_RELAY								(U32)0x1080		//AD K-LINE,L-LINE Relay Select
#define UART_RELAY								(U32)0x1000		//RS232C 상용통신 Relay Select
#define J1850_RELAY								(U32)0x1300		//J1850 Relay Select
#define J1708_RELAY								(U32)0x1200		//J1708(485) Relay Select
#define CANLOW_RELAY							(U32)0x1C00		//Can Low Relay Select
#define CANHIGH_RELAY							(U32)0x1800		//Can High Relay Select
#define CANSIG_RELAY							(U32)0x1c80		//GPEport
#define CANSIG_ON								(U32)0x2000		//GPAport
#define CANSIG_OFF								(U32)0xdfff		//GPAport
#define KLLINE_RELAY							(U32)0x0000		//K-line,L-line Relay Select

//2.KlineSelect
#define K_SERIAL								(U32)0x0000		//SERIAL(RS232C) select
#define K_PULSE									(U32)0x0001		//Pulse select

//3.KlineStatus
#define K_NORMAL								(U32)0x0010		//dlc 출력단을 high 설정
#define K_INVERSE								(U32)0x0000		//dlc 출력단을 low 설정

//4.LlineSelect
#define L_SERIAL								(U32)0x0000		//SERIAL(RS232C) select
#define L_PULSE									(U32)0x0002		//Pulse select

//5.LlineStatus
#define L_NORMAL								(U32)0x0020		//dlc 출력단을 high 설정
#define L_INVERSE								(U32)0x0000		//dlc 출력단을 low 설정

//6.KlineSwitchStatus
#define K_PULSE_HIGH							(U32)0x0004		//Pulse초기 high출력설정
#define K_PULSE_LOW								(U32)0x0000		//Pulse초기 low출력설정

//7.LlineSwitchStatus
#define L_PULSE_HIGH							(U32)0x0008		//Pulse초기 high출력설정
#define L_PULSE_LOW								(U32)0x0000		//Pulse초기 low출력설정

//8.RxLineSelect
#define RXD_L_FB								(U32)0x0000		//L-line receive
#define RXD_OBD1								(U32)0x0002		//K-line 2.54V(REF) receive
#define RXD_OBD2								(U32)0x0008		//K-line OBD-II receive
#define RXD_485									(U32)0x0003		//J1708 receive
#define RXD_RS232								(U32)0x0004		//RS232C receive
#define RXD_HIGHCAN								(U32)0x0020		//HIGH CAN 250K~1M
#define RXD_LOWCAN								(U32)0x0000		//LOW CAN ~200K
#define RXD_SIGCAN								(U32)0x0001		//Single CAN high line
#define RXD_J1850								(U32)0x0000		//J1850 receive

//9.RxLineStatus
#define RXD_NORMAL								(U32)0x0000		//RXD 입력단을 high 설정
#define RXD_INVERSE								(U32)0x0040		//RXD 입력단을 LOW 설정

//10.PullupRelay
#define Pullup									(U32)0x0000		//Pull-up
#define Pulldown								(U32)0x0010		//Pull-down

//11.KlinePullup
#define K_Pullup_Off							(U32)0x0000
#define K_Pullup_510							(U32)0x0001		//k-line 510 select(+Batt)
#define K_Pullup_47k							(U32)0x0004		//k-line 4.7k select(+Batt)
#define K_Pullup_2k								(U32)0x0020		//k-line 2k select(+5V)

//12.LlinePullup
#define L_Pullup_Off							(U32)0x0000
#define L_Pullup_510							(U32)0x0002		//L-line 510 select(+Batt)
#define L_Pullup_47k							(U32)0x0008		//L-line 4.7k select(+Batt)
#define L_Pullup_2k								(U32)0x0040		//L-line 2k select(+5V)

//13.LineGnd
#define L_GND_ON								(U32)0x0080
#define L_GND_OFF								(U32)0x0000

///////////////////////////////////////////////////
// Use a auto flow control(cts,rts)
//14.K-Line CH
#define K_ChOff									(U32)0x0000			//K-Line CH all clear(Open)
#define K_Ch00									(U32)0x0080			//GPCport	//1pin
#define K_Ch01									(U32)0x0081	//2pin
#define K_Ch02									(U32)0x00C0	//3pin
#define K_Ch03									(U32)0x00C1	//6pin
#define K_Ch04									(U32)0x0090	//7pin
#define K_Ch05									(U32)0x0092	//8pin
#define K_Ch06									(U32)0x00D0	//9pin
#define K_Ch07									(U32)0x00D2	//10pin
#define K_Ch08									(U32)0x00A0	//11pin
#define K_Ch09									(U32)0x00A4	//12pin
#define K_Ch10									(U32)0x00E0	//13pin
#define K_Ch11									(U32)0x00E4	//14pin
#define K_Ch12									(U32)0x0040	//15pin
#define K_Ch13									(U32)0x0048	//16pin

//15.L-Line CH
#define L_ChOff									(U32)0x0000			//L-Line CH all clear(Open)
#define L_Ch00									(U32)0x0080			//GPDport	//1pin
#define L_Ch01									(U32)0x0081	//2pin
#define L_Ch02									(U32)0x00C0	//3pin
#define L_Ch03									(U32)0x00C1	//6pin
#define L_Ch04									(U32)0x0090	//7pin
#define L_Ch05									(U32)0x0092	//8pin
#define L_Ch06									(U32)0x00D0	//9pin
#define L_Ch07									(U32)0x00D2	//10pin
#define L_Ch08									(U32)0x00A0	//11pin
#define L_Ch09									(U32)0x00A4	//12pin
#define L_Ch10									(U32)0x00E0	//13pin
#define L_Ch11									(U32)0x00E4	//14pin
#define L_Ch12									(U32)0x0040	//15pin
#define L_Ch13									(U32)0x0048	//16pin

//==================================================
//16.Rprogram CH
#define R_ChOff									(U32)0x0000			//Reprogram all clear(Open)
#define R_Ch03									(U32)0x3800			//GPFport
#define R_Ch06									(U32)0x3900
#define R_Ch08									(U32)0x3400
#define R_Ch09									(U32)0x3600
#define R_Ch10									(U32)0x3C00
#define R_Ch11									(U32)0x3E00
#define R_Ch02									(U32)0x3100

//Veh (Speed) CH
#define V_ChOff									(U32)0x0000			//all clear(Open)
#define V_Ch03									(U32)0x2800			//GPFport
#define V_Ch06									(U32)0x2900
#define V_Ch08									(U32)0x2400
#define V_Ch09									(U32)0x2600
#define V_Ch10									(U32)0x2C00
#define V_Ch11									(U32)0x2E00
#define V_Ch02									(U32)0x2100
//========================END====================================//

//ReproLineStatus
#define Vss_Output								(U32)0x0040		//Veh signal output
#define Clk_Output								(U32)0x0080		//Baudrate cleck output
#define REPROG_OUT	 							(U32)0x0400			//Reprogram Output ON


#define HIGH									(U32)0x0001		//high status
#define LOW										(U32)0x0000		//low status
#define PULSE									(U32)0x0001		//pulse status
#define SERIAL									(U32)0x0000		//serial status

#define KCh										(U32)0x0001
#define LCh										(U32)0x0002
#define KLCh									(U32)0x0000

//#define KPulse									(U32)0x0001
//#define LPulse									(U32)0x0002

#define NOTUSE									(U32)0x0000
#define M40										(U32)0x0000
#define M50										(U32)0x0000		//50.7MHz

//BPS
#define	BAUD_61									51945	//(50.7MHz/(BAUD*16))-1
#define	BAUD_66									48010
#define	BAUD_244								12985
#define	BAUD_250								12674
#define	BAUD_488								6392
#define	BAUD_1000								3167
#define	BAUD_1200								2439
#define	BAUD_1953								1621
#define	BAUD_2400								1319
#define	BAUD_3600								879
#define	BAUD_4800								659
#define	BAUD_7200								439
#define	BAUD_7812								404
#define	BAUD_8193								385
#define	BAUD_9600								329
#define	BAUD_10400								303

#define	BAUD_10416								303
#define	BAUD_10285								307
#define	BAUD_12480								252
#define	BAUD_14400								219
#define	BAUD_15600								202
#define	BAUD_19200								164
#define	BAUD_28800								109
#define	BAUD_38400								81
#define	BAUD_41600								75
#define	BAUD_57600								54
#define	BAUD_62500								49
#define	BAUD_112000								25
#define	BAUD_128000								23

#define	SM9600									165
#define	SM10416									152

#if defined(HS_MODIFY)
	#define R_ChOff								(U32)0x0000			//Reprogram all clear(Open)
	#define R_Ch03								(U32)0x3800			//GPFport
	#define R_Ch06								(U32)0x3900
	#define R_Ch08								(U32)0x3400
	#define R_Ch09								(U32)0x3600
	#define R_Ch10								(U32)0x3C00
	#define R_Ch11								(U32)0x3E00
	#define R_Ch02								(U32)0x3100
#endif
//////////////////////////////////////////////////////////////////////////////////////////

/*----------------------------------------------------------------------
 *   typedef
 *--------------------------------------------------------------------*/
typedef __packed struct
{
	uint32_t	nCommRelay;
	uint32_t	nKlineSelect;
	uint32_t	nKlineStatus;
	uint32_t	nLlineSelect;
	uint32_t	nLlineStatus;
	uint32_t	nKlineSwitchStatus;
	uint32_t	nLlineSwitchStatus;
	uint32_t	nRxLineSelect;
	uint32_t	nRxLineStatus;
	uint32_t	nPullupRelay;
	uint32_t	nKlinePullup;
	uint32_t	nLlinePullup;
	uint32_t	nLlineGnd;
	uint32_t	nKlineCh;
	uint32_t	nLlineCh;
	uint32_t	nRProgramCh;
	uint32_t	nEtc1;
	uint32_t	nEtc2;
	uint32_t	nEtc3;
	uint32_t	nEtc4;
	uint32_t	nEtc5;
	uint8_t		ackmessage[20];
} stRECORD_HW_SET, tagHARDWARESET;
typedef struct
{
	unsigned long	nCommRelay;	
	unsigned long	nSourceIP_T1;	
	unsigned long	nSourcePort_T1;	
	unsigned char	nSourceMac_T1[6];	
	unsigned long	nDestinationIp_T1;	
	unsigned long	nDestinationPort_T1;	
	unsigned long	nSourceIp_Tx;	
	unsigned long	nSourcePort_Tx;	
	unsigned char	nSourceMac_Tx[6];	
	unsigned long	nDestinationIp_Tx;	
	unsigned long	nDestinationPort_Tx;	
	unsigned long	nTxLineSelect1;	
	unsigned long	nTxLineSelect2;	
	unsigned long	nTxLineSelect3;	
	unsigned long	nTxLineSelect4;	
	unsigned long	nEtc1;	
	unsigned long	nEtc2;	
	unsigned long	nEtc3;	
	unsigned long	nEtc4;	
	unsigned long	nEtc5;
} stRECORD_HW_SET_ETH, tagHARDWARESETETH;
typedef __packed struct
{										// IOCTL Config Parameters in J2534 (Figure 23 in J2534)
	uint32_t nDataRate;					// desired baud rate (5~500000, no default value)
	uint32_t nLoopBack;					// 수신 메시지에 대한 에코 여부 (0:off(default), 1:on)
	uint32_t nNodeAddress;				// J1850PWM에서의 node address 지정 (00~FF 사이의 값)
	uint32_t nNetworkLine;				// J1850PWM에서의 통신중 active한 network line 지정 (0:bus normal(default), 1:bus plus, 2:bus minus)
	uint32_t nP1Min;					// ISO9141에서의 ECU응답의 uint32_ter-byte time의 최소값					(0~FFFF, 단위: milli-seconds) 기본값: 0
	uint32_t nP1Max;					// ISO9141에서의 ECU응답의 uint32_ter-byte time의 최대값					(0~FFFF, 단위: milli-seconds) 기본값: 20
	uint32_t nP2Min;					// ISO9141에서의 tester request와 ECU응답 사이 time의 최소값						(0~FFFF, 단위: milli-seconds) 기본값: 25
	uint32_t nP2Max;					// ISO9141에서의 tester request와 ECU응답 사이 time의 최대값						(0~FFFF, 단위: milli-seconds) 기본값: 50
	uint32_t nP3Min;					// ISO9141에서의 ECU응답의 끝과 다음 tester request 시작 사이 time의 최소값			(0~FFFF, 단위: milli-seconds) 기본값: 55
	uint32_t nP3Max;					// ISO9141에서의 ECU응답의 끝과 다음 tester request 시작 사이 time의 최대값			(0~FFFF, 단위: milli-seconds) 기본값: 5000
	uint32_t nP4Min;					// ISO9141에서의 tester request의 uint32_ter-byte time의 최소값			(0~FFFF, 단위: milli-seconds) 기본값: 5
	uint32_t nP4Max;					// ISO9141에서의 tester request의 uint32_ter-byte time의 최대값			(0~FFFF, 단위: milli-seconds) 기본값: 20
//	uint32_t nW0;						// ISO9141에서의 address byte 끝 ~ synchronization pattern 시작 사이의 시간 최대값	(0~FFFF, 단위: milli-seconds) 기본값: 300
	uint32_t nW1;						// ISO9141에서의 address byte 끝 ~ synchronization pattern 시작 사이의 시간 최대값	(0~FFFF, 단위: milli-seconds) 기본값: 300
	uint32_t nW2;						// ISO9141에서의 synchronization pattern 끝 ~ key byte 1 시작 사이의 시간 최대값	(0~FFFF, 단위: milli-seconds) 기본값: 20
	uint32_t nW3;						// ISO9141에서의 key byte 1 ~ key byte 2 사이의 시간 최대값							(0~FFFF, 단위: milli-seconds) 기본값: 20
	uint32_t nW4;						// ISO9141에서의 key byte 2 ~ tester로부터의 inversion 사이의 시간 최대값			(0~FFFF, 단위: milli-seconds) 기본값: 50
	uint32_t nW5;						// ISO9141에서의 address byte 전송을 위한 tester 시작 전에 필요한 최소 시간값		(0~FFFF, 단위: milli-seconds) 기본값: 300
	uint32_t nTIdle;					// ISO9141에서의 Fast Init. 시작을 위하여 필요한 bus idle time						(0~FFFF, 단위: milli-seconds) 기본값: nW5의 값
	uint32_t nTInil;					// ISO9141에서의 Fast Init.에서 low pulse의 작동시간								(0~FFFF, 단위: milli-seconds) 기본값: 25
	uint32_t nTWUp;						// ISO9141에서의 Fast Init.에서 Wake-up pulse의 작동시간							(0~FFFF, 단위: milli-seconds) 기본값: 50
	uint32_t nParity;					// ISO9141에서의 Parity방식 (0:No Parity(default), 1:Odd Parity, 2:Even Parity)
	uint32_t nBitSamplePoint;			// desired bit samle pouint32_t를 표현하는 bit time의 percentage (0~100 (%), 기본값: 80)
	uint32_t nSyncJumpWidth;			// desired Synchronization jump width를 표현하는 bit time의 percentage (0~100 (%), 기본값: 15)
	uint32_t nT1Max;					// SCI Type에서의 uint32_ter-frame resonse delay의 최대값 (0~FFFF, 단위: milli-seconds) 기본값: 20
	uint32_t nT2Max;					// SCI Type에서의 uint32_ter-frame request delay의 최대값 (0~FFFF, 단위: milli-seconds) 기본값: 100
	uint32_t nT3Max;					// --- unused
	uint32_t nT4Max;					// SCI Type에서의 uint32_ter-message resonse delay의 최대값 (0~FFFF, 단위: milli-seconds) 기본값: 20
	uint32_t nT5Max;					// SCI Type에서의 uint32_ter-message request delay의 최대값 (0~FFFF, 단위: milli-seconds) 기본값: 100
	uint32_t nIso15765BS;				// ISO15765에서의 segmented transfer을 위한 block size (기본값: 0)
	uint32_t nIso15765STMin;			// ISO15765에서의 segmented transfer에서의 separation time  (기본값: 0)
	uint32_t nBSTx;						// --- 0~FF, FFFF (ISO15765-2 참조)
	uint32_t nSTMinTx;					// --- 0~FF, FFFF (ISO15765-2 참조)
	uint32_t nDataBits;					// --- 0:8 data bit, 1: 7 data bit
	uint32_t nFiveBaudMod;				// --- 0~3 (별도표 참조)
	uint32_t nToolManufacturerSpec;		//
	uint32_t nEtc1;						// can delay 변수로 사용함.
	uint32_t nEtc2;						// can ID offset 변수로 사용함.
	uint32_t nEtc3;						//
	uint32_t nEtc4;
	uint32_t nEtc5;						// can ID mask 변수로 사용함.
} stGITSetConfig, tagJ2534SETCONFIG;

typedef __packed struct
{										// IOCTL Config Parameters in J2534-2 (Figure 104 in J2534-2)
	uint32_t nCanMixedFormat;			// 0(FORMAT_OFF): ISO 15765 ONLY, 1(FORMAT_ON): ISO 15765 OR UNFORMATTED CAN, 2(ALL_FRAME):ISO 15765, UNFORMATTED CAN, OR BOTH, DEFAULT 0
	uint32_t nJ1962_Pins;				// 0x0000PPSS  PP: 1ST PIN, SS: 2ND PIN, DEFAULT : 0X0000
	uint32_t nSWCanDataRate;
	uint32_t nSWCanSpeedChange;
	uint32_t nSWCanResSwitch;
	uint32_t nActiveChannel;
	uint32_t nSampleRate;
	uint32_t nSamplePerReading;
	uint32_t nReadingPerMsg;
	uint32_t nAverageMethod;
	uint32_t nSampleResolution;
	uint32_t nInputRangeLow;
	uint32_t nInputRangeHigh;
	uint32_t nUEB_T0_Min;
	uint32_t nUEB_T1_Max;
	uint32_t nUEB_T2_Max;
	uint32_t nUEB_T3_Max;
	uint32_t nUEB_T4_Min;
	uint32_t nUEB_T5_Max;
	uint32_t nUEB_T6_Max;
	uint32_t nUEB_T7_Min;
	uint32_t nUEB_T7_Max;
	uint32_t nUEB_T9_Min;
	uint32_t nJ1939_Pins;
	uint32_t nJ1708_Pins;
	uint32_t nJ1939_T1;
	uint32_t nJ1939_T2;
	uint32_t nJ1939_T3;
	uint32_t nJ1939_T4;
	uint32_t nJ1939_Brdcst_MinDelay;
	uint32_t nTP20_T_BR_Int;
	uint32_t nTP20_T_E;
	uint32_t nTP20_MNTC;
	uint32_t nTP20_T_Cta;
	uint32_t nTP20_MNCT;
	uint32_t nTP20_MNTB;
	uint32_t nTP20_MNT;
	uint32_t nTP20_T_Wait;
	uint32_t nTP20_T1;
	uint32_t nTP20_T3;
	uint32_t nTP20_Identifer;
	uint32_t nTP20_RxIdPassive;
} stGITSetConfig_2, tagJ2534_2SETCONFIG;

/*----------------------------------------------------------------------
	 functions
----------------------------------------------------------------------*/
extern void         VCI_ACK(void);
extern void			VCI_Initialize(void);
extern void			VCI_Clear_DLC_HW(void);
extern void			VCI_HWSetParameter(uint8_t* pData);
extern void			VCI_SetSConfigList(uint8_t* pData, BOOL bIsStructCopy);
extern void			VCI_SetPassThruProtocolID(uint8_t *pData);
extern void			VCI_SetPassThruConnectFlags(uint8_t *pData);
extern void 		VCI_REINTI_COMM_STATE(uint32_t ulPID);
extern uint32_t 	VCI_GetPassThruProtocolID(void);
extern void			VCI_HW_Setting(uint32_t uiPID);
extern void			VCI_SetRtcTime(uint8_t *pSetDate);
extern uint32_t 	VCI_GetPassThruConnectFlags(void);
extern void			VCI_SetPassThruWirteMsgTimeout(uint8_t *pData, uint32_t uiDataLen);
extern u32			VCI_ProtocolClassification();
extern void			VCI_SetSByteArray(uint8_t* pData);
extern void 		VCI_FAST_Init_Delay(uint16_t ms);
extern uint8_t 		VCI_SetCANLog( uint8_t mode, uint8_t* rec_packet, uint8_t* send_buff );
extern void 		VCI_5bpsInit(void);
extern void 		DLC_RX_BUFF_CLEAR(void);
extern void 		VCI_GetRtcTime(uint8_t *pSetDate);
extern void 		DLC_CH_Set(U32 KlineChSet,U32 LlineChSet);
extern void 		ClearAllPeriodicMsg( void );
extern void         little_to_big_endian(U8 *ucData, U32 uiNum, U8 ucSize);
extern void         ContinueFunctionalPeriodicMsg( void );
extern void         StopFunctionalPeriodicMsg( void );
extern void         vcicrc32(uint8_t *data, size_t length);
extern void         KW_fast_init_Powertec (uint32_t FastTime, uint32_t FastTime1);
extern void         KW_fast_init (uint32_t FastTime,uint32_t FastTime1);
extern void         RxCANID_Set(U16 CanTxId1, U16 CanTxId2);

/*----------------------------------------------------------------------
 *   Gloval Variables
 *--------------------------------------------------------------------*/
extern stGITSetConfig		g_stGITSetConfig;
extern stRECORD_HW_SET_ETH	g_stGITHWSetDataEth;
extern U8 					g_ucAUTOVIN[60];
#endif	// __GIT_VCI_H__
