/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __MESSAGE_MAKE_H__
#define __MESSAGE_MAKE_H__

#include <time.h>
#include "GIT_OemInterface.h"
#include "Message_Manager.h"
#include "AutolinkMessage.h"
#include "GIT_Util.h"
#include "MngSystem.h"


/* Define ------------------------------------------------------------------*/


bool ApplyEncryption(uint8_t *arrPlainText, uint8_t *arrEncryptionText, uint16_t *nEncryptionTextLen, uint16_t totalLength);
uint16_t ApplyDecryption(uint8_t *pMessage, uint16_t nLen);
uint16_t ConversionLongLongToHexDec_6(long long llValue, uint16_t *pnPos, uint8_t *ptrBuffer);
uint16_t ConversionLongLongToHexDec_7(long long llValue, uint16_t *pnPos, uint8_t *ptrBuffer);
uint16_t ConversionFloatToHexDec_2(float fValue, uint16_t *pnPos, uint8_t *ptrBuffer);
uint16_t ConversionFloatToHexDec_3(float fValue, uint16_t *pnPos, uint8_t *ptrBuffer);
uint16_t ConversionIntToHexDec_1(uint16_t nValue, uint16_t *pnPos, uint8_t *ptrBuffer);
uint16_t ConversionIntToHexDec_2(uint16_t nValue, uint16_t *pnPos, uint8_t *ptrBuffer);
uint16_t ConversionDWordToHexDec_3(uint32_t wValue, uint16_t *pnPos, uint8_t *ptrBuffer);
uint16_t ConversionDWordToHexDec_4(uint32_t wValue, uint16_t *pnPos, uint8_t *ptrBuffer);
uint16_t ConversionDWordToHexDec_8(uint64_t wValue, uint16_t *pnPos, uint8_t *ptrBuffer);
uint16_t ConversionGPSDataToHexDec(float fValue, uint16_t *pnPos, uint8_t *ptrBuffer);
uint16_t ConversionMultiByteToHexDec_n(uint8_t *pMultiByte, uint16_t n, uint16_t *pnPos, uint8_t *ptrBuffer);
void ConversionHexStringToHex(char* pIn, char *pOut, int nInLength, int nRadix);//mod.pdh 2021.10.27 
uint16_t MakeEventMessageHeader2(eMESSAGE_TYPE eMsgType, uint8_t *strMessageBuffer, stMsgHeader* pstMessageHeader);
boolean_t IsInitializeTime(unsigned int unLocalTime);
#if defined(PROTOCOL17)
void MakeReqSetURL(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer,stReportSmartKey *pstSmartKey);
void MakeReqSetURLComplete(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer,stCarReport *pstCarReport);
#endif
#if defined(PROTOCOL24)
void MakeReportNetworkStatus(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer,stReportModemStatus *pstReportModemStatus);
#endif

void MakeRequestTrackingInfo(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer, stMsgMdm* pstMsgMdm);
void MakeResponseTrackingInfo(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer, stMsgMdm* pstMsgMdm);
void MakeSendTrackingInfo(uint16_t *pnDataLength, uint16_t *pnPos, uint8_t *paMessageBuffer, stMsgMdm* pstMsgMdm);
bool MakeEventMessageContent(eMESSAGE_TYPE eMsgType, uint8_t *strMessageBuffer);
extern FIRMWARE_INFO 		g_FirmwareInfo;
#endif
