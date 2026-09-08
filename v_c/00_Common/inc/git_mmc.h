/*----------------------------------------------------------------------
 *   MMC Control( SDMMC, EMMC... )
 *--------------------------------------------------------------------*/
#ifndef	__GIT_MMC_H__
#define	__GIT_MMC_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#include "fatfs.h"
#include "firmware.h"

/*----------------------------------------------------------------------
 *   Define
 *--------------------------------------------------------------------*/
#define FILE_INFO_MAX									10

#define FA_SEEKEND										0x20					/* Seek to end of the file on file open */
   
//#define SAVE_CAN_LOG

/*----------------------------------------------------------------------
 *   typedef
 *--------------------------------------------------------------------*/
typedef enum								// sdcard protected status
{
	CARD_PROTECTED			= 0,
	CARD_NOT_PROTECTED		= 1
} protectedTypeDef;

typedef enum								// sdcard connection status
{
	CARD_CONNECTED			= 0,
	CARD_DISCONNECTED		= 1,
	CARD_STATUS_CHANGED		= 2
} ConnectionStateTypeDef;

typedef enum
{
	FILE_WRITE_OPEN	= 0,
	FILE_READ_OPEN,
	FILE_CLOSE,
	FILE_WRITE,
	FILE_READ,
	FILE_REMOVE,
	FILE_LIST,
	FILE_SYNC,
	FILE_FS_FORMAT
} OperationStateTypeDef;

typedef enum
{
	ERROR_CODE_NONE_ERROR		= 0,
	ERROR_CODE_EMMC_MOUNT		= 1,
	ERROR_CODE_EMMC_OPEN		= 2,
	ERROR_CODE_EMMC_WRITE		= 3,
	ERROR_CODE_EMMC_READ		= 4,
	ERROR_CODE_EMMC_ERASE		= 5,
	ERROR_CODE_EMMC_COMPARE		= 6,
} eEMMC_ERROR_CODE;

/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
extern int32_t	initMMC( void );
extern void		deinitMMC( void );

extern FRESULT	mountFatFS( void );
extern FRESULT	unmountFatFS( void );
extern uint8_t	getMountStatus( void );
extern FRESULT	formatEmmc( void );
extern uint8_t	InitEmmcFolder( void );

extern FRESULT	scanDir( char *path );

extern uint8_t	WriteFileTest(void);

extern uint8_t	UploadFlashfromFile( const TCHAR* path, uint32_t destAddress, uint32_t *size, uint16_t *cs, uint8_t sector, SAppInfo* stFWAppInfo);
extern uint8_t	DownloadFilefromFlash( const TCHAR* path, SAppInfo *source );

extern uint8_t	Save_ServerInfo_EMMC(void);
extern uint8_t	Load_ServerInfo_EMMC(void);
extern uint8_t	Delete_ServerInfo_EMMC(void);

#if defined ( SAVE_CAN_LOG )
extern void CreateTempLogFile( uint8_t number );
extern void WriteCANLog ( uint32_t pCANID, uint8_t *pTxData, uint16_t uiSize );
extern void WriteKlLog( uint8_t *pTxData, uint16_t uiSize );
extern void MakeCANLogFile (void);
//extern void SendLogFile(uint8_t *buff, uint32_t readSize, uint32_t curSeqNo, uint32_t maxSeqNo);
extern void SendLogFile(uint8_t *buff, uint32_t readSize);
#endif
/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/
extern FILINFO	gFileInfo[FILE_INFO_MAX];
extern uint8_t	g_ucTempFileCnt;

#endif // __GIT_MMC_H__