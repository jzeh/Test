#ifndef __SYS_PS_EXTEND_H__
#define __SYS_PS_EXTEND_H__

#if defined(FEATURE_EXTENSION_BOARD)

#include "Autolink_Manager.h"
#include "Git_Interprotocol.h"

typedef enum __SysPsExtendState{
    STATE_SYSPS_EXTEND_INIT = 0,
    STATE_SYSPS_EXTEND_SLEEP,
    STATE_SYSPS_EXTEND_ERROR,
    STATE_SYSPS_EXTEND_IDLE,
    STATE_SYSPS_EXTEND_RUN,
	STATE_SYSPS_EXTEND_RESET,
    STATE_SYSPS_EXTEND_FOTA,
    STATE_SYSPS_EXTEND_CANNOTRECOVERY,
}SysPsExtendState;

enum {
	eCONTROL_IDLE = 0,
	eCONTROL_WATTING
};

typedef __packed struct _stPLUS_MODULE_STATE
{
	INT8U	m_ucAccState;				//0x00:OFF 0x01:ACC ON(시동전) 0x02:EV-ON(시동후) 0x03:GAS-ON(시동후)
	INT8U 	m_ucDoorOpenState;		//하위4bit DoorState FL/FR/RL/RR	ex)00000010b   FROPEN
	INT8U 	m_ucUnlockState;			//하위4bit	DoorUnlockState FL/FR/RL/RR		0x10-TrunkOpen
	INT8U	m_ucLampState;			//0x00:OFF 0x01:ON
}stPLUS_MODULE_STATE;

typedef __packed struct _stExtBD_Version_Info
{
	uint16_t usExtVersionBoot;
	uint16_t usExtVersionApp;
}stExtBDVersionInfo;

typedef __packed struct __BR_EXTBD_Info {
	uint16_t usEXTBDVersionBoot;
	uint16_t usEXTBDVersionApp;
}BR_EXTBDInfo;

void SaveHiPassCardStatus(uint8_t cHiPassCardStatus);
uint8_t GetHiPassCardStatus();

void SetExtBDVersionInfo(uint8_t * ucExtBDVersion);
void GetExtDBVersionInfo(stExtBDVersionInfo * stExtBDFWFileInfo);

void SysExtendBoardProcess();

void SetSysPsExtendStatus(SysPsExtendState eState);
SysPsExtendState GetSysPsExtendStatus();
void SysPsExtendEventHandler(stMsgExtend* pstReport);
boolean_t GetExtendBoardWakeupState();
void SendExtendBoardSysPsState(int8_t cNewState);

void SaveControlEvtMsg(void* pstEventMsg);
void GetSavedControlEvtMsg(void* pstEventMsg);

int SysPsExtend_EvtNone(void* pstEventMsg);
int SysPsExtend_EvtSysSystem(void* pstEventMsg);
int SysPsExtend_EvtSysSystemMsg(void* pstEventMsg);
int SysPsExtend_EvtSysSensor(void* pstEventMsg);
int SysPsExtend_EvtSysBluetooth(void* pstEventMsg);
int SysPsExtend_EvtSysModem(void* pstEventMsg);
int SysPsExtend_EvtALObd(void* pstEventMsg);
int SysPsExtend_EvtALData(void* pstEventMsg);
int SysPsExtend_EvtALAutoLink(void* pstEventMsg);
int SysPsExtend_EvtProductionTest(void* pstEventMsg);
int SysPsExtend_EvtExtendBoard(void* pstEventMsg);
void HandlerSaveMsg(stMsgExtend stEventMsg);


void SetHighPassPower(U8 ucIsPowerOn);
void LoadConfigHipassPowerValue();
U8 GetHighPassPower();

// Basic Plus org
// void SysPsExtensionInit(eSYSTEM_RESET_MODE eSysResetMode,eAutoLinkServiceMode eServiceMode);
void SysPsExtensionInit();
void ProcessExtendInterProtocol(stGIT_PTCL_PAYLOAD *pInterPtcl, eCommType eInCommType);


void ExtendBoard3secReport(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardGetModuleStateRes(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardModuleWakeupNoti(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardFWUpdateStartRes(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardFWUpdateDataRes(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardFWUpdateEndRes(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardSerialRes(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardSleepRes(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardFWInfoRes(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardGetHipassInfoRes(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardHiPassStateRes(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardGetHiPassPaymentCountRes(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardGetHipassPaymentDataRes(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardHiPassPowerOnRes(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardHiPassPowerOffRes(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardFobKeyPowerControlResultRes(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardFobKeyPowerStatusRes(void *pInterPtcl, unsigned int eInCommType);
void ExtendBoardGetRFIDInfoRes(void *pInterPtcl, unsigned int eInCommType);

void RSWakeupComplete(uint8_t* pBuffer, int32_t nLength);
void ResGetVehicleStatusFromRS(uint8_t* pBuffer, int32_t nLength);
void ResGetRSVersion(uint8_t* pBuffer, int32_t nLength);

void SysExtendBoardCheckAntiThief(boolean_t bAntiThiefFlag, boolean_t bFOBOnStatus);

void SetAntiThiefFlagSetting(bool bAntiThiefFlag);
bool GetAntiThiefFlagSetting();

#endif //#if defined(FEATURE_EXTENSION_BOARD)

#endif

