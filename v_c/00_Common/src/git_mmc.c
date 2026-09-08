/*************************************************************
 * NOTE : git_mmc.c
 *      mmc control( SDMMC, EMMC ... )
 * Author : Lee junho
 * Since : 2019.08.13
**************************************************************/
#include "main.h"
#include "string.h"
//#include "rtc.h"
#include "sdmmc.h"

#include "common.h"
#include "led.h"
#include "typedef.h"
#include "buzzer.h"
#include "sw_timer.h"
//#include "git_can.h"
//#include "git_protocol.h"
#include "git_ioctl.h"
#include "firmware.h"
#include "flash_if.h"

#include "git_fsutil.h"
#include "git_mmc.h"
#include "git_rtc.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#define FILE_DATA_BUFFER_SIZE								4096

#define SDCARD_WRITE_READ_SPEED_TEST_ENABLE					0

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
int32_t	initMMC( void );
void	deinitMMC( void );

FRESULT mountFatFS( void );
FRESULT unmountFatFS( void );
uint8_t getMountStatus( void );
FRESULT formatEmmc( void );

FRESULT scanDir( char *path );

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
static uint8_t	g_IsMounted			= 0;					// check fs mounted

uint8_t	workBuffer[ 2 * _MAX_SS ];

ALIGN_32BYTES(char g_FsReadBuf[FILE_DATA_BUFFER_SIZE]);
ALIGN_32BYTES(char g_FsWriteBuf[FILE_DATA_BUFFER_SIZE]);

FILINFO	gFileInfo[FILE_INFO_MAX];

// for saving CAN log
uint8_t 	g_STORE_CANLog_FILE_NAME[20]={"CAN_log.dat"};
FIL 		f_TempFilepnt;
uint32_t 	g_uiLogSaveTimer_Timeout = 0;
uint8_t		g_ucTempFileCnt = 0;

// for mmc read/write speed test
uint32_t		g20usMmcTick		= 0;					// 20us tick count

char FR_Table[][24] =
{
	"FR_OK",							/* (0) Succeeded */
	"FR_DISK_ERR",						/* (1) A hard error	occurred in	the	low	level disk I/O layer */
	"FR_INT_ERR",						/* (2) Assertion failed	*/
	"FR_NOT_READY",						/* (3) The physical	drive cannot work */
	"FR_NO_FILE",						/* (4) Could not find the file */
	"FR_NO_PATH",						/* (5) Could not find the path */
	"FR_INVALID_NAME",					/* (6) The path	name format	is invalid */
	"FR_DENIED",						/* (7) Access denied due to	prohibited access or directory full	*/
	"FR_EXIST",							/* (8) Access denied due to	prohibited access */
	"FR_INVALID_OBJECT",				/* (9) The file/directory object is	invalid	*/
	"FR_WRITE_PROTECTED",				/* (10)	The	physical drive is write	protected */
	"FR_INVALID_DRIVE",					/* (11)	The	logical	drive number is	invalid	*/
	"FR_NOT_ENABLED",					/* (12)	The	volume has no work area	*/
	"FR_NO_FILESYSTEM",					/* (13)	There is no	valid FAT volume */
	"FR_MKFS_ABORTED",					/* (14)	The	f_mkfs() aborted due to	any	problem	*/
	"FR_TIMEOUT",						/* (15)	Could not get a	grant to access	the	volume within defined period */
	"FR_LOCKED",						/* (16)	The	operation is rejected according	to the file	sharing	policy */
	"FR_NOT_ENOUGH_CORE",				/* (17)	LFN	working	buffer could not be	allocated */
	"FR_TOO_MANY_OPEN_FILES",			/* (18)	Number of open files > _FS_LOCK	*/
	"FR_INVALID_PARAMETER"				/* (19) Given parameter is invalid */
};

/*----------------------------------------------------------------------
 *   Extern Variables
 *--------------------------------------------------------------------*/
extern uint8_t	retUSER;			/* Return value for USER */
extern char		USERPath[4];		/* USER logical drive path */
extern FATFS	USERFatFS;			/* File system object for USER logical drive */
extern FIL		USERFile;			/* File object for USER */

extern MMC_HandleTypeDef hsd1;

/*----------------------------------------------------------------------
 *   Functions implementation
 *--------------------------------------------------------------------*/
int32_t initMMC( void )
{
	EnableEmmc();

	MX_SDMMC1_MMC_Init();
	MX_FATFS_Init();

	HAL_NVIC_SetPriority(SysTick_IRQn, 0x0E ,0);

	if( retUSER == 0 )		//  fatfs.c FATFS_LinkDriver(&USER_Driver, USERPath);
	{
		if( mountFatFS() != FR_OK )			return -1;
	}
	else
	{
		return -2;
	}

	return 0;
}

void deinitMMC( void )
{
	unmountFatFS();

	DisableEmmc();
}

FRESULT mountFatFS( void )
  {
      FRESULT res = FR_OK;

      if( g_IsMounted != 1 )
      {
          for(uint8_t i = 0; i < 3; i++)
          {
              res = f_mount( &USERFatFS, (TCHAR const*)USERPath, 0 );

              if( res == FR_OK )
              {
                  GLogN( "Success... Mount(%s)!!!\r\n", USERPath );
                  g_IsMounted = 1;
                  return res;
              }
              else if( res == FR_NO_FILESYSTEM )
              {
                  GLogN( "No filesystem, formatting...\r\n" );
                  formatEmmc();
                  continue;
              }
              else
              {
                  GLogE( "Mount failed(%s), retry %d...\r\n", FR_Table[res], i+1 );
                  HAL_Delay(100);
              }
          }

          GLogE( "Error... fail mount(%s)!!!\r\n", FR_Table[res] );
          g_IsMounted = 0;
      }
      else
      {
          GLogI( "Already mounted...\r\n" );
      }

      return res;
  }

FRESULT unmountFatFS( void )
{
	FRESULT	res = FR_OK;

	if( g_IsMounted == 1 )
	{
		g_IsMounted = 0;
		res = f_mount(NULL, (TCHAR const*)"", 0);
		if( res == FR_OK )
		{
			g_IsMounted = 0;
			GLogI( "Success... Unmount!!!\r\n" );
		}
		else
		{
			GLogEE( "Failed... Unmount!!!(%s)\r\n", FR_Table[res] );
		}
	}
	else
	{
		GLogI( "Already unmounted!!!\r\n" );
	}

	return res;
}

uint8_t getMountStatus( void )
{
	return g_IsMounted;
}

FRESULT formatEmmc( void )
{
	FRESULT		res;

//	res = f_mkfs( USERPath, FM_ANY, 0, workBuffer, sizeof( workBuffer ) );
	res = f_mkfs( USERPath, FM_FAT32, 32 * 1024, workBuffer, sizeof( workBuffer ) );
	if( res != FR_OK )
	{
		GLogE( "Error... fail format filesystem(%d)!!!\r\n", res );
	}
	else
	{
		GLogN( "Success... f_mkfs...\r\n" );
	}

	return res;
}

uint8_t InitEmmcFolder( void )
{
	FRESULT		res;
	char		path[20] = { '\0', };

	// Check mount
	if( g_IsMounted == 0 )			mountFatFS();

	// 1. make Application folder
	sprintf( path, "%s%s", USERPath, EMMC_APPLICATION_FOLDER );
	res = f_mkdir( path );
	if( ( res != FR_OK ) && ( res != FR_EXIST ) )
	{
		GLogEE("Failed...(%s, %s)\r\n", path, FR_Table[res] );
		return 1;
	}

	// 2. make Backup folder
	memset( path, '\0', sizeof( path ) );
	sprintf( path, "%s%s", USERPath, EMMC_BACKUP_FOLDER );
	res = f_mkdir( path );
	if( ( res != FR_OK ) && ( res != FR_EXIST ) )
	{
		GLogEE("Failed...(%s, %s)\r\n", path, FR_Table[res] );
		return 2;
	}
	
	// 3. make Record folder
	memset( path, '\0', sizeof( path ) );
	sprintf( path, "%s%s", USERPath, EMMC_RECORD_FOLDER );
	res = f_mkdir( path );
	if( ( res != FR_OK ) && ( res != FR_EXIST ) )
	{
		GLogEE("Failed...(%s, %s)\r\n", path, FR_Table[res] );
		return 3;
	}

	return 0;
}

#if( SDCARD_WRITE_READ_SPEED_TEST_ENABLE )
#define TEST_FILE_LEN					(uint32_t)(1 * 1024 *1024)		// 1 MB
uint32_t testWriteSpeed( uint32_t size )
{
	FRESULT		res;
	FIL			TestFile;
	uint32_t	bytes;
	uint32_t	times;
	uint32_t	count;
	uint32_t	i;
	uint32_t	currentTick;

	memset( g_FsWriteBuf, 0xAA, FILE_DATA_BUFFER_SIZE );

	res = f_open( &TestFile, (TCHAR const *)"test.txt", FA_CREATE_ALWAYS | FA_WRITE );
	if( res != FR_OK )
	{
		GLogE( "Error... fail open file(%d)!!!\r\n", res );
		return 0;
	}

	currentTick	= HAL_GetTick();

	count = TEST_FILE_LEN / size;
	for( i = 0; i < count; i++ )
	{
		res = f_write( &TestFile, g_FsWriteBuf, size, (void *)&bytes );
		if( res != FR_OK )
		{
			GLogE( "Error... fail write Data(%d)!!!\r\n", res );
			break;
		}
	}

	times = HAL_GetTick() - currentTick;

	f_close( &TestFile );

	return ( TEST_FILE_LEN / 1024 ) * 1000 / times;
}

uint32_t testReadSpeed( uint32_t size )
{
	FRESULT		res;
	FIL			TestFile;
	uint32_t	bytes;
	uint32_t	times;
	uint32_t	count;
	uint32_t	i;
	uint32_t	currentTick;

	res = f_open( &TestFile, (TCHAR const *)"test.txt", FA_READ );
	if( res != FR_OK )
	{
		GLogE( "Error... fail open file(%d)!!!\r\n", res );
		return 0;
	}

	currentTick	= HAL_GetTick();

	count = TEST_FILE_LEN / size;
	for( i = 0; i < count; i++ )
	{
		res = f_read( &TestFile, g_FsWriteBuf, size, (void *)&bytes );
		if( ( res != FR_OK ) || ( bytes != size ) )
		{
			GLogE( "Error... fail read Data(%d)!!!\r\n", bytes );
			break;
		}
	}

	times = HAL_GetTick() - currentTick;

	f_close( &TestFile );

	return ( TEST_FILE_LEN / 1024 ) * 1000 / times;
}

void testEmmcSpeed( void )
{
	GLogI( "==================================================\r\n" );
	GLogI( "   EMMC Write Speed Test(File Size : %dMByte)!!!\r\n", TEST_FILE_LEN / 1024 / 1024 );
	GLogI( "==================================================\r\n" );
//	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n",    1, testWriteSpeed(    1 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n",    8, testWriteSpeed(    8 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n",   32, testWriteSpeed(   32 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n",   64, testWriteSpeed(   64 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n",  128, testWriteSpeed(  128 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n",  256, testWriteSpeed(  256 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n",  512, testWriteSpeed(  512 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n", 1024, testWriteSpeed( 1024 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n", 2048, testWriteSpeed( 2048 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n", 4096, testWriteSpeed( 4096 ) );
	GLogI( "==================================================\r\n" );
	GLogI( "   EMMC Read Speed Test(File Size : %dMByte)!!!\r\n", TEST_FILE_LEN / 1024 / 1024 );
	GLogI( "==================================================\r\n" );
//	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n",    1, testReadSpeed(    1 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n",    8, testReadSpeed(    8 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n",   32, testReadSpeed(   32 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n",   64, testReadSpeed(   64 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n",  128, testReadSpeed(  128 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n",  256, testReadSpeed(  256 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n",  512, testReadSpeed(  512 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n", 1024, testReadSpeed( 1024 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n", 2048, testReadSpeed( 2048 ) );
	GLogN( " Unit : %4dByte   Speed : %5dKB/S\r\n", 4096, testReadSpeed( 4096 ) );
	GLogI( "==================================================\r\n" );
}
#endif	// SDCARD_WRITE_READ_SPEED_TEST_ENABLE

/*----------------------------------------------------------------------
 *   FatFS utils
 *--------------------------------------------------------------------*/
FRESULT scanDir( char *path )
{
	FRESULT			res;
	DIR				dir;
	static FILINFO	fno;
	u32				count = 0;

	res = f_opendir( &dir, (TCHAR const *)path );
	if( res == FR_OK )
	{
		GLogN( "Open dir( %s )\r\n", path );

		for(;;)
		{
			res = f_readdir( &dir, &fno );
			if( ( res != FR_OK ) || ( fno.fname[0] == 0 ) )				break;		// Break on error or end of dir

			if( fno.fattrib & AM_DIR )
			{
				u32 i;

				i = strlen( path );
				sprintf( &path[i], "/%s", fno.fname );
				res = scanDir( path );
				if( res != FR_OK )		break;
				path[i] = 0;
			}
			else
			{
				GLogN( "%s/%s\t\t%10d\r\n", path, fno.fname, fno.fsize );
				if( count >= FILE_INFO_MAX )
				{
					GLogE( "Error... Overflow File list!!!\r\n" );
					count = 0;
				}
				memcpy( &gFileInfo[count++], &fno, sizeof( FILINFO ) );
			}
		}

		f_closedir( &dir );
	}

	return res;
}

uint8_t WriteFileTest(void)
{
	FRESULT	res;
	char path[64];
	uint32_t bw;

	GLogI( "==================================================\r\n" );
	GLogI( "   Start EMMC File Test...\r\n" );
	GLogI( "==================================================\r\n" );

	GLogN( " 1. Mount      : " );
	if( getMountStatus() == false )
	{
		res = mountFatFS();
		if( res == FR_OK )
		{
			GLogN( "Success...\r\n" );
		}
		else
		{
			GLogEE( "Failed...\r\n" );
			return ERROR_CODE_EMMC_MOUNT;
		}
	}
	else
	{
		GLogN( "Success...\r\n" );
	}

	do
	{
		GLogN( " 2. File Open  : " );
		sprintf( path, "%sTest.txt", USERPath );
		res = f_open( &USERFile, path, FA_CREATE_ALWAYS | FA_WRITE );
		if( res == FR_OK )
		{
			GLogN("Success...(%s)!!!\r\n", path);
		}
		else
		{
			GLogEE("Failed...(%s, %s)\r\n", path, FR_Table[res] );

			if( res == FR_NO_FILESYSTEM )
			{
				formatEmmc();
				continue;
			}

			return ERROR_CODE_EMMC_OPEN;
		}
	} while( res != FR_OK );

	GLogN( " 3. File Write : " );
	sprintf( g_FsWriteBuf, "0123456789abcdefghijklmnopqrstuvwxyz\r\n" );
	res = f_write( &USERFile, g_FsWriteBuf, sizeof(g_FsWriteBuf), &bw );
	if( res == FR_OK )
	{
		GLogN( "Success...!!!\r\n");
	}
	else
	{
		GLogEE("Failed...(%s, %s)\r\n", path, FR_Table[res] );
		return ERROR_CODE_EMMC_WRITE;
	}

	f_close(&USERFile);        /*  Close file */

	GLogN( " 4. File Open  : " );
	res = f_open( &USERFile, path, FA_OPEN_EXISTING | FA_READ );
	if( res == FR_OK )
	{
		GLogN("Success...(%s)!!!\r\n", path);
	}
	else
	{
		GLogEE("Failed...(%s, %s)\r\n", path, FR_Table[res] );
		return ERROR_CODE_EMMC_OPEN;
	}

	GLogN( " 5. File Read  : " );
	memset( g_FsReadBuf, 0, sizeof( g_FsReadBuf ) );
	res = f_read( &USERFile, g_FsReadBuf, sizeof(g_FsReadBuf), &bw );
	if( res == FR_OK )
	{
		GLogN( "Success...!!!\r\n");
	}
	else
	{
		GLogEE("Failed...(%s, %s)\r\n", path, FR_Table[res] );
		return ERROR_CODE_EMMC_READ;
	}

	/*  Close file */
	f_close(&USERFile);
/*
	GLogN( " 6. File Erase : " );
	res = f_unlink( path );
	if( res == FR_OK )
	{
		GLogN("Success...(%s)!!!\r\n", path);
	}
	else
	{
		GLogEE("Failed...(%s, %s)\r\n", path, FR_Table[res] );
		return ERROR_CODE_EMMC_ERASE;
	}
*/
	GLogI( "==================================================\r\n\n" );

	if( strcmp( g_FsWriteBuf, g_FsReadBuf ) == 0 )
	{
		GLogN( "Test Pass...\r\n" );
		return ERROR_CODE_NONE_ERROR;
	}
	else
	{
		GLogEE( "Test Fail...\r\n" );
		return ERROR_CODE_EMMC_COMPARE;
	}
}

uint8_t UploadFlashfromFile( const TCHAR* path, uint32_t destAddress, uint32_t *size, uint16_t *cs , uint8_t sector, SAppInfo* stFWAppInfo)
{
	uint32_t ret		= 0;
	uint32_t bytes		= 0;
	uint32_t offset		= 0;
	uint32_t flashAddr	= destAddress;
	uint32_t count		= 0;
	uint32_t i			= 0;
    uint32_t info_offset = 0;

	GLogN( "Start %s(%s)!!!\r\n", __FUNCTION__, path );

	*size	= 0;
	*cs		= 0;
	if( f_open( &USERFile, path, FA_OPEN_EXISTING | FA_READ ) == FR_OK )
	{
	  	HAL_FLASH_Unlock();

		// erase destination sector
        if( (USERFile.obj.objsize) <= (sector*131072) )
        {
            ret = eraseFlash( destAddress, sector );
            if( ret )
            {
                GLogEE( "fail eraseFlash!!!\r\n" );
                ret = 1;
                goto error_upload;
            }
            
            if( f_size( &USERFile ) == 0 )		// Check File Size
            {
                ret = 2;
                goto error_upload;
            }
            
            if(stFWAppInfo != NULL)
            {
                memcpy(&g_FsReadBuf[0], stFWAppInfo, sizeof(SAppInfo));
                memcpy(&g_FsReadBuf[sizeof(SAppInfo)], &gsFwInfo.marrucSerialNo[0], SERIAL_NUMBER_SIZE);
                info_offset += FIRMWARE_RECOVERY_INFO_OFFSET;
            }
            
            while( !f_eof( &USERFile ) )
            {
                ret = f_read( &USERFile, &g_FsReadBuf[info_offset], FILE_DATA_BUFFER_SIZE-info_offset, (void *)&bytes );
                if( ( ret == FR_OK ) && ( bytes > 0 ) )
                {
    //				GLogN( "." );
                    count++;
                    if( ( count % 16 ) == 0 )
                    {
                        //Buzzer_Control( eBUZZER_ON, MSEC(50),MSEC(50), 1 );
                        //HAL_Delay( 100 );
                    }
                    
                    bytes += info_offset;

                    if( writeByteFlash( flashAddr, (uint8_t *)g_FsReadBuf, bytes ) == FLASH_OK )
                    {
                        /* Increment FLASH destination address */
                        flashAddr	+= bytes;
                        offset		+= (bytes-info_offset);
                        *size		+= (bytes-info_offset);
                    }
                    else
                    {
                        GLogEE( "fail write Flash!!!\r\n" );
                        f_close( &USERFile );
                        info_offset = 0;
                        ret = 3;
                        goto error_upload;
                    }

                    for( i = 0; i < bytes-info_offset; i++ )			*cs = *cs + g_FsReadBuf[i+info_offset];
                    info_offset = 0;
                }

                f_lseek( &USERFile, offset );
            }
        }
		f_close( &USERFile );
	}
	else
	{
		GLogEE( "Fail Open file(%s)...\r\n", path );
		ret = 4;
		goto error_upload;

	}

	GLogI( "File Size is %d\r\n", *size );
	GLogI( "File CheckSum is %d\r\n", *cs );

	ret = 0;

error_upload:
	HAL_FLASH_Lock();
	
	f_close( &USERFile );
	
	return (uint8_t)ret;
}

uint8_t DownloadFilefromFlash( const TCHAR* path, SAppInfo *source )
{
	uint32_t	flashAddr;
	uint32_t	ret;
	uint32_t	bw;
	uint8_t		buff[8];
	uint32_t	count;
	uint8_t		remain;
	uint32_t	i;

	count	= source->mSize / 8;
	remain	= source->mSize % 8;

	GLogN( "Start %s(%s)!!!\r\n", __FUNCTION__, path );
	if( f_open( &USERFile, path, FA_CREATE_ALWAYS | FA_WRITE ) == FR_OK )
	{
		flashAddr = FIRMWARE_MAINAPP_ADD;
		for( i = 0; i < count; i++ )
		{
			if( ( i % 4096 ) == 0 )
			{
				//Buzzer_Control( eBUZZER_ON, MSEC(50),MSEC(50), 1 );
				//HAL_Delay( 100 );
			}

			readByteFlash( flashAddr, buff, 8 );
			ret = f_write( &USERFile, buff, 8, &bw );
			if( ret != FR_OK || bw != 8 )
			{
				GLogEE( "Fail write file...\r\n" );
				return 1;
			}

			flashAddr = flashAddr + 8;
		}

		readByteFlash( flashAddr, buff, remain );
		ret = f_write( &USERFile, buff, remain, &bw );
		if( ret != FR_OK || bw != remain )
		{
			GLogEE( "Fail write file...\r\n" );
			return 2;
		}

		f_close( &USERFile );
	}
	else
	{
		GLogEE( "Fail Open file...\r\n" );
		return 3;
	}

	return 0;
}

#if defined ( SAVE_CAN_LOG )

void CreateTempLogFile( uint8_t number )
{
  	uint8_t 	ucFname[20]={0,};
	FRESULT		res1, res2, res3;
	res1 = f_chdir(DIR_ROOT);
  	sprintf( (char *)ucFname, "TEMP_%d.log", number );
	res2 = f_unlink((char *)ucFname);
	res3 = f_open(&f_TempFilepnt, (char const*)ucFname, FA_OPEN_ALWAYS | FA_WRITE);
	if( res3 == FR_OK )
	{
		GLogN( "Access CAN Log File %d Success\r\n", number);
		g_uiLogSaveTimer_Timeout = Get_Tmr();
	}
	else	
	{
	  	GLogE( "Access CAN Log File(%08X) %d Fail : dir %d / unlink %d / open %d \r\n", f_TempFilepnt, number, res1, res2, res3);
//		g_ucCANLog_Enable = 0;
//		f_close(&f_TempFilepnt);
	}
}

void WriteCANLog ( uint32_t pCANID, uint8_t *pTxData, uint16_t uiSize )
{
  	FRESULT				res;
	uint32_t			bytes = 0;
	uint8_t				buf[80] = {0,};
	uint8_t				i = 0;
	uint32_t			save_time = 0;
	static bool s_bFlag=true;

	if( s_bFlag == true )
	{
		s_bFlag = false;
		save_time = Get_Tmr();
		
		for(i = 0; i < sizeof(pCANID); i++)
		{
		  	buf[i] = (((pCANID << i*8) & 0xFF000000) >> 24) ;
		}
		
		memcpy(&buf[4], pTxData, uiSize);
		
		uiSize += sizeof(pCANID);
			   
	  	res = f_write( &f_TempFilepnt, buf, uiSize, &bytes ); 
		if( ( res != FR_OK ) || ( uiSize != bytes ) )
		{
			GLogE( "Write CAN Log Fail...%d,%d,%d\r\n", res, uiSize,bytes );
		}
		
		if( f_size( &f_TempFilepnt ) >= 1000 )
		{
			f_close(&f_TempFilepnt);
			g_ucTempFileCnt++;
			if(g_ucTempFileCnt == 4)	g_ucTempFileCnt = 0;
			CreateTempLogFile(g_ucTempFileCnt);
		}
		save_time = Get_TmrDelta(Get_Tmr(), save_time);
		if( save_time > 5 ) GLogN(" %d", save_time);
		s_bFlag = true;
	}
}

void WriteKlLog( uint8_t *pTxData, uint16_t uiSize )
{
  	FRESULT				res;
	uint32_t			bytes = 0;
	uint8_t				buf[2000] = {0,};
	uint32_t			save_time = 0;
	static bool s_bFlag=true;

	if( s_bFlag == true )
	{
		s_bFlag = false;
		save_time = Get_Tmr();
		
		memcpy(&buf[0], pTxData, uiSize);
			   
	  	res = f_write( &f_TempFilepnt, buf, uiSize, &bytes ); 
		if( ( res != FR_OK ) || ( uiSize != bytes ) )
		{
			GLogE( "Write KLog Fail...%d,%d\r\n",res,uiSize);
		}
		
		if( f_size( &f_TempFilepnt ) >= 1000 )
		{
			f_close(&f_TempFilepnt);
			g_ucTempFileCnt++;
			if(g_ucTempFileCnt == 4)	g_ucTempFileCnt = 0;
			CreateTempLogFile(g_ucTempFileCnt);
		}
		save_time = Get_TmrDelta(Get_Tmr(), save_time);
		if( save_time > 5 ) GLogN(" %d", save_time);
		s_bFlag = true;
	}
}

void MakeCANLogFile (void)
{
	uint8_t		tempCnt;
	FIL 		LogFilepnt;
	uint8_t 	ucFname[8] = {0,};
	uint8_t		STORE_TEMP_FILE_NAME[15]={"TEMP_0.log"};
	uint32_t	uiEmmcTotalCnt=0;
	uint8_t		LOG_FILE_NAME[8]={"CAN.log"};
	
	
	f_close(&f_TempFilepnt);

	sprintf((char*)&ucFname[0],"%s", &LOG_FILE_NAME[0]);  // YY
	
	tempCnt = g_ucTempFileCnt;
	
	f_unlink((char const*)ucFname);
	
	if(f_open(&LogFilepnt,(char const*)ucFname, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)    //.REM ����
	{
		GLogE( "\r\n %s LOG FILE OPEN FAIL !! ",ucFname);
	}
	if((f_close(&LogFilepnt)!=FR_OK))
	{
		GLogE( "\r\n LOG FIL Close FAIL!");
	}
	if( tempCnt == 3) tempCnt = 0;
	else tempCnt++;
	while(1)
	{
	  	sprintf((char*)STORE_TEMP_FILE_NAME,"TEMP_%d.log", tempCnt);
	  
	  	if(f_open(&f_TempFilepnt,(char const*)STORE_TEMP_FILE_NAME, FA_OPEN_EXISTING | FA_READ) != FR_OK)
		{
		  	GLogE( "\r\n %s TEMP FILE OPEN FAIL !! ", STORE_TEMP_FILE_NAME);
		}
		else
		{
		 	uiEmmcTotalCnt = uiEmmcTotalCnt + f_size(&f_TempFilepnt);
			GLogN( "\r\n %s TEMP FILE OPEN SUCCESS : %d !! ", STORE_TEMP_FILE_NAME, uiEmmcTotalCnt);
			CopyFiletoFile(f_TempFilepnt, STORE_TEMP_FILE_NAME, FA_OPEN_EXISTING | FA_READ, 0, LogFilepnt, ucFname, FA_OPEN_EXISTING | FA_WRITE, uiEmmcTotalCnt - f_size(&f_TempFilepnt));    //�����ϴµ� 2MB�� 5�� ����  22MB 50sec, 37MB 120sec
			f_close(&f_TempFilepnt);
			f_unlink((char const*)STORE_TEMP_FILE_NAME);
		}
		
		if( tempCnt == g_ucTempFileCnt)
		{
		  	break;
		}
		else
		{
		  	if( tempCnt == 3) tempCnt = 0;
			else tempCnt++;
		}
	};
	g_ucTempFileCnt = 0;
}

//void SendLogFile(uint8_t *buff, uint32_t readSize, uint32_t curSeqNo, uint32_t maxSeqNo)
void SendLogFile(uint8_t *buff, uint32_t readSize)
{
  	//static uint32_t	seqNo  = 0;
  	//uint8_t			ucTemp[80];
	uint32_t		ret;
	//uint32_t		uiTempSize	= 0;
	uint32_t		uiEmmcCnt1	= 0;
	//uint32_t		ucReadSize = 0;
	uint8_t			LOG_FILE_NAME[8]={"CAN.log"};	
	FIL 			LogFilepnt;
	
	f_chdir(DIR_ROOT);
	
	ret = f_open( &LogFilepnt, (char const*)LOG_FILE_NAME, FA_OPEN_EXISTING | FA_READ );
	if( ret == FR_OK )
	{
		// check file size
		readSize = f_size( &LogFilepnt );

		if(readSize != 0 && readSize < sizeof(buff))
		{
			if( f_read( &LogFilepnt, buff, readSize, &uiEmmcCnt1 ) == FR_OK )
			{
				GLogN("Read CAN Log File Success...\r\n");
			}
		}
		else
		{
		  GLogN("Fail : CAN Log File Size = %d (MAX : 4096)\r\n", readSize);
		}
		
//		maxSeqNo = ((uiTempSize / sizeof(buff)) + 1);
//		
//		if((f_size( &LogFilepnt ) - uiTempSize) > sizeof(buff))
//			ucReadSize = sizeof(buff);
//		else
//			ucReadSize = f_size( &LogFilepnt ) - uiTempSize;
//		   
//		if( f_lseek( &LogFilepnt, uiTempSize ) == FR_OK )
//		{
//			if( f_read( &LogFilepnt, buff, uiTempSize, &uiEmmcCnt1 ) == FR_OK )
//			{
//				for(uint16_t h = 0; h < uiEmmcCnt1; h++)
//				{
//					GLogN("%02X", ucTemp[h]);
//				}
//				uiTempSize += uiEmmcCnt1;;
//				readSize = uiEmmcCnt1;
//				seqNo++;
//				curSeqNo = seqNo;
//				if (curSeqNo == seqNo)
//				  seqNo = 0;
//			}
//			else
//				seqNo = 0;
//		}
//		else
//		{
//			seqNo = 0;
//		}
//		GLogN("\r\n");
	}
	
	f_close( &LogFilepnt );
}

#endif

uint8_t Save_ServerInfo_EMMC(void)
{
	FIL     file;
	FRESULT res;
	UINT    bw;

	f_chdir(DIR_ROOT);

	res = f_open(&file, "server_info.dat", FA_WRITE | FA_CREATE_ALWAYS);
	if (res != FR_OK)
	{
	  	f_close(&file);
		GLogN("f_open %d\r\n", res);
		return 0;
	}

	res = f_write(&file, &msServerConnectInfo, sizeof(SServerInfo), &bw);
	if (res != FR_OK || bw != sizeof(SServerInfo))
	{
		GLogN("f_write %d\r\n", res);
		f_close(&file);
		return 0;
	}

	res = f_close(&file);
	if (res != FR_OK)
	{
		GLogN("f_close %d\r\n", res);
		return 0;
	}

	return 0;
}


uint8_t Load_ServerInfo_EMMC(void)
{
    FIL		file;
    FRESULT res;
    UINT	br;

	f_chdir(DIR_ROOT);

    res = f_open(&file, "server_info.dat", FA_READ);
    if (res != FR_OK)
    {
        return res;
    }

    res = f_read(&file, &msServerConnectInfo, sizeof(SServerInfo), &br);
	if (res != FR_OK || br != sizeof(SServerInfo))
    {
        return res;
    }

    f_close(&file);



    return res;
}

uint8_t Delete_ServerInfo_EMMC(void)
{
	FRESULT res;

	f_chdir(DIR_ROOT);

	res = f_unlink("server_info.dat");
	if (res != FR_OK && res != FR_NO_FILE)
	{
		GLogE("f_unlink server_info.dat failed(%d)\r\n", res);
		return 1;
	}

	memset(&msServerConnectInfo, 0x00, sizeof(SServerInfo));

	GLogI("Delete server_info.dat Success\r\n");

	return 0;
}
