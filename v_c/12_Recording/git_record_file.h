/*----------------------------------------------------------------------
 *   record control
 *--------------------------------------------------------------------*/
#ifndef	__GIT_RECORD_FILE_H__
#define	__GIT_RECORD_FILE_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "typedef.h"
#include "ff.h"

#define FILENAME_MODE_SWITCH_INI	"ModeSwitch.ini"
#define FILENAME_APP_LIST_INI		"AppSwList.ini"
#define FILENAME_CV_ADAPTOR_INI		"CVAdaptorVer.ini"
#define FILENAME_AUTOVIN_CONFIG_DAT	"AutoVinConfig.dat"
#define FILENAME_VCI2_SERIAL_DAT	"VCI2Serial.dat"
#define STR_FW_SWITCH_COMPLETE		"FW_SWITCH_COMPLETE"
#define STR_FW_SWITCH_READY  		"FW_SWITCH_READY"
#define BlockBox_STR_DAT		"BlockBox_STR.dat"

#define DIR_ROOT					"/"
//#define DIR_APP						"APP"
#define DIR_INFO					"INFO"
//#define DIR_RECORD					"Record"

#define STORE_REC_TEMP_FILE_NAME	"TEMP.REC"
#define STORE_REC_NEW_TEMP_FILE_NAME	"New_TEMP.REC"

#define ENGINE_STALL_DATA			"EngineStall.dat"
#define CURRENT_NODE_DATA			"CurrentNode.dat"
#define FW_WAKEUP_DATA				"FWWakeup.dat"
#define CONFIG_DATA					"CONFIG.DAT"

////////////////////////////////////////////////////////////////////////////
//		config.dat address
//#define Addr_StartCommCnt				234
//#define Addr_StartCommData			235
#define Addr_ProtocolID					1384
#define Addr_InitCnt					1388
#define Addr_InitAddr					1389
#define Addr_StartCommCnt				1390
#define Addr_StartCommData				1391
#define Addr_HwSetData					1641
#define Addr_ConfigData					1745

#define Addr_TriggerMode				1905
#define Addr_RecordingTime				1906

#define Addr_DtcCommCnt					1907
#define Addr_DtcCommData				1908
#define Addr_DtcStartPos				2408
#define Addr_DtcReadNo					2409
#define Addr_DtcSkipNo					2410

#define Addr_SelectTrigTime				2422

#define Addr_RecItemCnt					2433

#define Addr_RecItems					2434
#define Addr_RecReqData					4435
#define Addr_NextConfig					0x4321
////////////////////////////////////////////////////////////////////////////


typedef struct __stFwRecordingWakeupInfo{
	uint8_t bActiveRecording;
	uint8_t ucReseard[255];
}stFwRecordingWakeupInfo;


void ConvertFRRtc2FileName(uint8_t* pcFileName);

void SaveBackupData2File(bool bForce);
uint32_t ReadFile(uint8_t* pcFileName, uint8_t* pcBuffer, uint32_t unAdjustAddress, uint32_t unLength);
void DistributionDataFileofRecoredingTime(bool bForce);

void MakeFlightRecordFile();
void MakeRecordPacket(uint8_t* pucPacket, uint32_t* punLength, uint8_t* pucRxBuffer, uint8_t unRxLength);

bool UpdateRecordHeader(uint8_t* pucHeader, uint16_t usLength, uint32_t* punDataSize);
void UpdateRecordIndex(uint8_t* pucIndex, uint32_t* punLength);

FRESULT scan_files(char* path);


#endif // __GIT_RECROD_FILE_H__
