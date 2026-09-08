/**
  ******************************************************************************
  * @file    DebugHandler.h
  * @author  GIT Connectivity Development 2 Team
  * @version V1.0.0
  * @date    06-Feb-2018
  * @brief   Header for DebugHandler.c module
  ******************************************************************************
 **/

#include "common.h"
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __DEBUG_HANDLER_H__
#define __DEBUG_HANDLER_H__

enum{
    eErrorCodeSys = 0x01000000,
	eErrorCodeSysHd = 0x02000000,
    eErrorCodeCfg = 0x03000000,
    eErrorCodeStg = 0x04000000,
	eErrorCodeStgFile = 0x05000000,
    eErrorCodeMdm = 0x06000000,
    eErrorCodeObd = 0x07000000,
    eErrorCodeQue = 0x08000000,
    eErrorCodeMsg = 0x09000000,
	eErrorCodeSns = 0x0A000000,
	eErrorCodeApp = 0x0B000000,
};


// related App
enum{
	eUnknownId,	
	eEventUnknown,
	eSubEventUnknown,
	eFotaEventUnknown,
	eWriteOpenFail,
	eWriteFail,
	eWriteDataQueueFail,
	eWriteMsgQueueFail,
	eSizeLimit,
	eWriteError,
	eWriteSizeError,
	eReadError,
	eReadSizeError,
	eMakeFodlerError,
	eDeleteFileError,
	eDeleteFolderError,
	eUnknownControl,
	eCloseError,
	eSeekError,
	eRenameError,
	eUnknownStateError,
	eModeError,
	eUnknownModeError,
	eNotSupportCommand,
    eMemoryLimit,
    eIpekGetFail,
    eHeaderSizeLimit,
    ePayloadSizeLimit,
    eIpekSrandSizeLimit,
    eIpekSrandLimit,    
    eIpekHashSizeLimit,
    eIpekHashLimit,
    eIpekSignSizeLimit,
    eIpekSignLimit,
    eIpekSizeLimit,
    eIpekLimit,	
    eNoSerial,
};


int32_t WriteErrorCode(int32_t nErrorCode);
void ReadErrorCode();
void ClearErrorCode();

#endif //__DEBUG_HANDLER_H__

/***************************** END OF FILE ****/
