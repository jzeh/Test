/*----------------------------------------------------------------------
 *   Ethernet Control
 *--------------------------------------------------------------------*/
#ifndef	__GIT_FSUTIL_H_
#define	__GIT_FSUTIL_H_

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "ff.h"

/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/
#define DIR_ROOT								"/"
#define DIR_APP									"/01_Application"
#define DIR_INFO								"INFO"
#define DIR_RECORD								"03_Record"
#define DIR_REF									"Ref"
#define DIR_LOG									"LOG"
#define DOWNLOAD_FW_TEMP_FILE_NAME	            "DownloadTemp.bin"
#define HSM_STATUS_FILE_NAME	                "HSM_Status.log"
#define HSM_UPDATE_FAIL_INFO	                "HSM_Update_Fail.inf"
#define HSM_UPDATE_MAX_COUNT	                3
#define HSM_UPDATE_ERROR_NUM	                4
#define HSM_UPDATE_FAIL_STATUS	                0x88
#define FILE_CNT_MAX                            20
//#define FILE_CNT_MAX                            25//추후 업데이트 //.evt 파일 18byte 파일명에서 22byte 파일명으로 변경되어 25바이트까지 전닳할수 있도록 수정
#define FILE_NUM_MAX                            160
//#define FW_WAKEUP_DATA				            "FWWakeup.dat"
//#define FILENAME_APP_LIST_INI					"fw_cvci301.lst"

//For Portable ECU Update
#define CERT_SHA1_FILENAME						"sdcrt.dat"
#define CERT_SHA2_FILENAME						"sdcrt2.dat"
#define CERT_MAX_FILE_SIZE						610
#define DATE_LENGTH								6
#define EMMC_FILE_NAME_MAX						64
	 
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
#define SIZE_VIN_NUM			                17

#define DIR_ECU_TO_UPGRADE_DEPTH1               "Crux"
#define DIR_ECU_TO_UPGRADE_DEPTH2               "S_ETC"

#define FILE_RUNREPRO                           "RUNREPRO.INI"
#define FILE_RDBI                               "RDBI.INI"
#define DIR_REPRO_RESULT_DEPTH2                 "S_LOG"
#define FILE_REPRO_RESULT                       "Repro_Result.log"
#define FILE_REPRO_RESULT_BK                    "Repro_Result.bk"


#endif
/*----------------------------------------------------------------------
 *   typedef
 *--------------------------------------------------------------------*/
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
#pragma pack(push, 1) 
typedef struct _stRDBI
{
    u16 unECUCount;
    u8  ucRuleDBFileNameSize;
    u8  *pRuleDBFileName;
    u8  ucRomBinFileNameSize;
    u8  *pRomBinFileName;
    u8  ucPartLidSize;
    u8  *pPartLid;
    u8  ucPartDty;      // 0:ascii, 1:hex
    u8  ucPartByteSize;
    u8  *pPartByte;
    u8  ucPartNoCnt;
    u8  **parrPartNo;  
    u8  ucSwVerLidSize;
    u8  *pSwVerLid;
    u8  ucSwVerDty;      // 0:ascii, 1:hex
    u8  ucSwVerByteSize;
    u8  *pSwVerByte;
    u8  *pSwVer;    //Length = ucSwVerByteSize
    u32 unCanTxID;  // 앞 2byte는  0으로 채움
    u32 unCanRxID;
    BOOL bIsUpgradeTarget;
}stRDBIInfo;

typedef struct _stRunRepro
{
    u16 unECUCount;
    u8  ucRomBinFileNameSize;
    u8  *pRomBinFileName;
    u8  ucRuleDBFileNameSize;
    u8  *pRuleDBFileName;
    u8  ucRetryCount;           // if 0xFF, success
    u8  arrVinCode[SIZE_VIN_NUM];
}stRunRepro;
/*
typedef struct _stReproResult
{
}stReproResult;
*/
#pragma pack(pop,1) 

#endif
/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
BOOL GetVCI2FileWrite(char* strOpenFileName,U8 *buff, U32 iBuffLength);
BOOL LogFileWrite(char* strOpenFileName,U8 *buff, U32 iBuffLength);
BOOL GetVCI2FileRead(char* strOpenFileName, U8* InBuff, U32 *iBuffLength);
BOOL GetCrlFileRead(char* strOpenFileName, U8* InBuff, U32 *iBuffLength);
BOOL File_Read(char* strOpenFileName, U8 mode, U32 lseek, U8* InBuff, U32 InBuffSize, U32 *iBuffLength);
BOOL File_Write(char* strOpenFileName, U8 mode, U32 lseek, U8* InBuff, U32 InBuffSize, U32 *iBuffLength);
BOOL File_Rename(char* strOpenFileName, U8 mode, char* strReFileName);
U32  File_Size(char* strOpenFileName, U8 mode);
BOOL GetUpdateAppListFromEMMC(uint8_t *buff, uint32_t *Size);
U8 IsConfigFile( void );
void directory_list( void );
void directory_listWithPath(char *DirPath);
void directory_list2(unsigned char *DirPath, unsigned char *FileInfoToPC, int *SizeInfoToPC);

extern U8 	VciDirFile(U8 *DirBuff, U32 *DirBuffSize);
extern U8	VciEraseFile(void);
extern U8	VciSendFileOpen(unsigned char FileName[25], U32 *FileTotalSize);
extern U8	VciSendFileOpenWithPath(char *DirPath,  char *FileName, U32 *FileSize);
extern U8	VciReadFile_Slave(U32 ReadSize, U8 *FileBuff, U32 *FileSize, U32 uiSequence);
extern U8	VciReadFile_Send(U32 ReadSize, U8 *FileBuff, U32 *FileSize, U32 uiSequence);
extern U8	VciSendFileClose(void);
extern U8	VciRecvFileOpen(char *FileName, U32 FileSize);
extern U8	VciRecvFileOpenWithPath(U8 FileOpenOption, char *DirPath, char *FileName, U32 FileSize);
extern U8	VciRecvFileReceive(U8 *buff, U16 Size);
extern U8	VciRecvFileClose(void);
extern U8	VciRecvFileCloseBigName(void);
extern U8	VciEraseFileWithPath(char *DirPath,  char *FileName, U8 EraseOption);
extern U8	VciMakeDirWithPath(char *DirPath);
extern U8	VciEraseFileOrDir(char *DirPath);

extern U16	CheckVCIFWCheckSum(void);
extern BOOL UpdateAppListFromRemoteDevice(U8 *buff, U16 Size);
extern U8   DownloadClose();
extern U8	WriteUpdateDataTemp(U8 *buff, U16 Size);
extern U8   GetUpgradeFileInfo(U8 *pData);
extern bool InitFwListFile (void);

extern bool CopyFiletoFile( FIL fSourceFile, U8 *strSourceFileName, BYTE SourceFileMode, U32 uilSourceSeekPoint, FIL fTargetFile, U8 *strTargetFileName, BYTE TargetFileMode, U32 uilTargetSeekPoint);
extern bool CopyFiles( U8 *strSourceFileName, BYTE SourceFileMode, U32 uilSourceSeekPoint, U8 *strTargetFileName, BYTE TargetFileMode, U32 uilTargetSeekPoint);

extern int8_t Send_HSM_File( U8 *Data_Addr, int Count, int length );
extern FRESULT StoreCrlEmmc(U8 crl_no, U8 *buff, U16 Size);
extern BOOL GetCrlEmmc(U8 crl_no, U8* InBuff, U32 *iBuffLength);
extern FRESULT StoreRdbiEmmc(U8 *buff, U16 Size);//for test
extern FRESULT StoreSTDA_ReproEmmc(U8 *buff, U16 Size);//for test

extern U8 CRT_CRL_Check(U8 *Data, U8 CertNum);
extern FRESULT FileCount(U8 *ucPath, U16 usPathLen, U16 *usFileCount, U16 *usFolderCount, U16 *usPacketCount);
extern FRESULT FileParsing(U8 *ucPath, U16 usPathLen, U16 Sequence, U8 *ucData, U32 *uiDataLen);
extern FRESULT FileChecksumCheck(U8 *ucPath, U16 usPathLen, U8 *ucFilename, U16 ucFilenameLen, U32 *uiChecksum);
extern FRESULT FileCRC32Check(U8 *ucPath, U16 usPathLen, U8 *ucFilename, U16 ucFilenameLen, U32 *uiChecksum);
/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/
   
extern char			Flashdata[30];
extern eMain_State	g_eMainState;
extern FIL			g_logEmmcFile;
extern char			g_emmcLogFileName[EMMC_FILE_NAME_MAX];
#endif // __GIT_ETH_H_
