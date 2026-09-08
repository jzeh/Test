#ifndef __TASK_HWCONTROL_H
#define __TASK_HWCONTROL_H

/* Includes -----------------------------------------------------------*/
#include "../Inc/sys-common.h"

/* Functions -----------------------------------------------*/
extern void HWCONTROL_init(void);
extern void HWCONTROL_Update(void);

extern void StartHwConTask(void *argument);
extern void RELAY_process(SystemMode_t state);
extern void IO_AD_RELAY_control(int type, int ch, bool enable);
extern void IO_EXT_RLY_control(int ch, bool enable);
extern void IO_ALL_OFF_control(void);
extern void IO_Resistance_Relay_Control(int ohm, bool enable);
extern void IO_SS_control(int ch, bool enable);

extern void CP_PWM_control(bool enable, int curr);
extern uint16_t CP_CLI_ReadADC(void);

/* 0x12 DataConfig 의 CP 전류 반영 로직 (git-functionlist.c 에서 이동, 동작 동일) */
extern void HWCON_ApplyChargeCurrent(void);

extern void EMMC_Enable(void);
extern void FAN_CooldownArm(void);

#endif /* __TASK_HWCONTROL_H */


