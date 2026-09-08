/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HANDLER_RSV_ENG_CTRL_H__
#define __HANDLER_RSV_ENG_CTRL_H__

boolean_t IsValidRsvEngCtrlGUID(uint8_t* pBuff);
void ShowPosixToGeneralTime(long lPosix);

void GetRsvEngCtrlSetting(stRsvEngCntorl* pstRsvEngCtrlSetting);
void SetRsvEngCtrlSetting(stRsvEngCntorl stRsvEngCtrlValue);
uint32_t GetSecUntiltDay();
uint8_t HandlerRsvEngCtrl();
boolean_t CheckRsvEngCtrlDate(stRsvEngCntorlInfo stRsvEngCtrlInfo);
void CheckRsvEngControlInfo(stHalRTCTypeDef * pstDateTime, uint32_t *punNextWakeupTime, stRsvEngCntorl * pstRsvEngCtrl);



#endif /* __HANDLER_RSV_ENG_CTRL_H__ */

/***************************** END OF FILE ****/

