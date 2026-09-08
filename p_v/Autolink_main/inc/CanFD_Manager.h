/* CANFD Controller */
#ifndef __CANFD_MANAGER_H__
#define __CANFD_MANAGER_H__

#include "common.h"
#include "CanFD_Defines.h"
#include "AutolinkConfig.h"
#include "diskio.h"
         

//////////////////////////////////////////////////
void SetCanFDInitHandler();
uint32_t GetRetryInitCount();

void SetDefaultFDState(eInitCanFDSequence eInitType);
eCanFDHandlerRsp CanFDInitHandler();
eCanFDSubControlResult InitCanFD();
eCanFDSubControlResult JudgeCanFDMaskingSet();
eCanFDSubControlResult WriteCanFDMasking();
eCanFDSubControlResult CheckCanFDFota();
eCanFDSubControlResult InitComplete();
eCanFDSubControlResult CFAM_GotoSleep();


//////////////////////////////////////////////////
void SetFDRunState(eInitCanFDSequence eState, eInitCanFDSequence ePreState, eInitCanFDSequence eNextState, int nMaxTimeout, int nRetryCount);
void SetCanFDRequest(eInitCanFDSequence eCanFDRequest);
eInitCanFDSequence GetCanFDRequest();
eInitCanFDSequence GetCanFDResponse();
eCanFDResult CanFDCompareReqRsp();
int GetPrevState();
void SetCurrentState();
int GetCurrentState();
int GetNextState();
void SetInitCurretCount();

boolean_t ExistCanID(unsigned int nCanID, unsigned char ucLine);
boolean_t AddMaskCanID(unsigned int nCanID , unsigned char ucLineInfo);
boolean_t MakeCanIDMergeList();
void DisplayMergeCanIDList();

eCANFD_BOARD_NEED_UPDATE NeedToUpdateCheck();
eCANFD_BOARD_NEED_UPDATE NeedToUpdateCheck2();

void SetCanFDStatusForBT(eCFD_STATUS_BT eStatus);
void SetCanFDMainState(eCanFD_STATE eState);

#endif /* __CANFD_MANAGER_H__ */

