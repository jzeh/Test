/*************************************************************
 * NOTE : git_fsutil.c
 *      file system utillity
 * Author : whlee
 * Since : 2019.09.23
**************************************************************/
/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "common.h"
#include "git_mmc.h"
#include "typedef.h"
#include "git_fsutil.h"
#include "flash_if.h"
#include "firmware.h"
//#include "git_hsm.h"
#include "git_vci.h"
#if defined(VCI3_DIAG) && defined(VCI3_RECORD)
#include "git_global.h"
#include "git_cli.h"
#endif
////////////////////////////////////////////////////////////////////////////////
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
#include "GIT_PassThruDefines.h"

stRDBIInfo		*g_pstRDBIInfo = NULL;
stRunRepro		*g_pstRunReproInfo = NULL;
BYTE			g_ucCommRDBIIdx = 0;
BYTE			g_arrRecvPartNo[32];
BYTE			g_arrRecvSwVer[32];
//stReproResult g_stReproResultInfo;
BOOL			g_bWaitForResPacket_InterAnalysis = FALSE;

extern void		FL_GitPassThruWriteMsgs( void *pInterPtcl, uint32_t eInCommType, uint32_t usLength  );
extern uint32_t	GetUnixTime( void );

#endif

//static FATFS g_Fatfs[_VOLUMES];
FIL				g_TempFilepnt,g_SaveTempFilepnt;
bool			g_bDeleteFileFlag=false;

uint32_t		g_u32UpdateFileSize = 0;
char			g_strDownloadFW_FileName[25];
static char		g_strTemp_FileName[100];
uint8_t			g_ucDownloadFW_FileNo = 0;
//U8  g_temp[4000] @ "CCMRAM";

//U8  g_temp[4000];

// Datalog_EMMCFile
char			g_emmcLogFileName[EMMC_FILE_NAME_MAX];


FIL				fpFast;
DWORD			dwFileSizeFast;
char			Flashdata[30];
U8				StringCompareAce89(U8 *From, U8 *Dest, U16 Length);

extern			U8 gHSM_Info_Name[18];

#define BULKSIZE 500
extern uint32_t g_CRCval; 

U8 IsConfigFile( void )
{
	U8 config_buff[2];
    FIL CFilepnt;

	GLogI( ">>> Start %s\r\n", __FUNCTION__ );

	f_chdir(DIR_RECORD);

	memset(config_buff,0x00,sizeof(config_buff));
	if (f_open(&CFilepnt, "CONFIG.DAT", FA_OPEN_EXISTING | FA_READ) == FR_OK )
	{
        if(f_size(&CFilepnt) != 0)
        {
            GLogN( "\n\r %s File Open Success\r\n", "CONFIG.DAT");
            f_close(&CFilepnt);



			f_chdir(DIR_ROOT);


            return 1;
        }
        else
        {
            GLogE( "\n\r %s File Size is zero \r\n", "CONFIG.DAT");
            f_close(&CFilepnt);

			f_chdir(DIR_ROOT);


            return 0;
        }
	}

	f_chdir(DIR_ROOT);

	GLogE( "\n\r %s File Open Fail \r\n", "CONFIG.DAT");
	return 0;

}

BOOL File_Write(char* strOpenFileName, U8 mode, U32 lseek, U8* InBuff, U32 InBuffSize, U32 *iBuffLength)
{
	FIL fpVer;

	if(f_open(&fpVer, (char const*)strOpenFileName,mode)== FR_OK)
	{
		if(f_lseek(&fpVer, lseek )== FR_OK)
		{
			f_write(&fpVer, InBuff, InBuffSize, iBuffLength);
		}
		f_close(&fpVer);
	}
	else
	{
		GLogE( "%s : %s File Open fail\r\n", __FUNCTION__, strOpenFileName);
		f_close(&fpVer);
		return 0;
	}

	return 1;
}

BOOL File_Read(char* strOpenFileName, U8 mode, U32 lseek, U8* InBuff, U32 InBuffSize, U32 *iBuffLength)
{
	FIL fpVer;

	if ( f_open(&fpVer, strOpenFileName, mode) == FR_OK )
	{
        if(f_size(&fpVer) != 0)
        {
            if(f_lseek(&fpVer,lseek) == FR_OK)
            {
                if ( f_read(&fpVer, InBuff, InBuffSize, iBuffLength) == FR_OK )
                {
                    f_close(&fpVer);
                    return TRUE;
                }
            }
        }
		f_close(&fpVer);
	}
	else
		GLogE( "%s : %s File Open fail\r\n", __FUNCTION__, strOpenFileName);

	return FALSE;
}

BOOL File_Rename(char* strOpenFileName, U8 mode, char* strReFileName)
{
	FIL fpVer;

	f_unlink((char const*)strReFileName);
	if ( f_open(&fpVer, strOpenFileName, mode) == FR_OK )
	{
        if( f_rename((char const*)strOpenFileName,(char const*)strReFileName) == FR_OK)
        {
			f_close(&fpVer);
			return TRUE;
        }
		f_close(&fpVer);
	}

	GLogE( "%s : %s File Rename %s fail\r\n", __FUNCTION__, strOpenFileName, strReFileName);

	return FALSE;
}

U32 File_Size(char* strOpenFileName, U8 mode)
{
	FIL fpVer;

	if ( f_open(&fpVer, strOpenFileName, mode) == FR_OK )
	{
		f_close(&fpVer);
		return f_size(&fpVer);
	}
	else
		GLogE( "%s : %s File Open fail\r\n", __FUNCTION__, strOpenFileName);

	return 0;
}

BOOL GetVCI2FileRead(char* strOpenFileName, U8* InBuff, U32 *iBuffLength)
{
	DWORD dwFileSize;
	FIL fpVer;

	GLogI( ">>> Start %s\r\n", __FUNCTION__ );

	if ( f_open(&fpVer, strOpenFileName, FA_OPEN_EXISTING | FA_READ) == FR_OK )
	{
		//UartPrintf(&U1,"%s File Open Success\r\n", strOpenFileName);
		dwFileSize = f_size(&fpVer);

		if ( f_read(&fpVer, InBuff, dwFileSize, iBuffLength) == FR_OK )
		{
			f_close(&fpVer);
			return TRUE;
		}
	}
	else
		GLogE( "%s File Open fail\r\n", strOpenFileName);

	return FALSE;
}

BOOL GetVCI2FileWrite(char* strOpenFileName,U8 *buff, UINT iBuffLength)
{
	FIL Filepnt;
	UINT uiWriteNum;
	U8 ret=0;

	ret|=f_unlink(strOpenFileName);
	ret|=f_open(&Filepnt, strOpenFileName, FA_CREATE_NEW | FA_WRITE);
	ret|=f_truncate(&Filepnt);
	ret|=f_write(&Filepnt, buff, iBuffLength, &uiWriteNum);
	ret|=f_close(&Filepnt);

	if(ret != 0)  return 1;
	return 0;
}

BOOL LogFileWrite(char* strOpenFileName,U8 *buff, UINT iBuffLength)
{
	FIL Filepnt;
	UINT uiWriteNum;
	U8 ret=0;

	ret|=f_unlink(strOpenFileName);
	ret|=f_open(&Filepnt, strOpenFileName, FA_CREATE_ALWAYS| FA_WRITE);
	ret|=f_truncate(&Filepnt);
	ret|=f_write(&Filepnt, buff, iBuffLength, &uiWriteNum);
	ret|=f_close(&Filepnt);

	if(ret != 0)  return 1;
	return 0;
}


void put_rc( FRESULT rc )
{
	switch( rc )
	{
		case FR_OK					:	GLogN( "rc = %u FR_OK\r\n", rc );					break;			/* (0) Succeeded */
		case FR_DISK_ERR			:	GLogN( "rc = %u FR_DISK_ERR\r\n", rc );			    break;			/* (1) A hard error occurred in the low level disk I/O layer */
		case FR_INT_ERR				:	GLogN( "rc = %u FR_INT_ERR\r\n", rc );				break;			/* (2) Assertion failed */
		case FR_NOT_READY			:	GLogN( "rc = %u FR_NOT_READY\r\n", rc );			break;			/* (3) The physical drive cannot work */
		case FR_NO_FILE				:	GLogN( "rc = %u FR_NO_FILE\r\n", rc );				break;			/* (4) Could not find the file */
		case FR_NO_PATH				:	GLogN( "rc = %u FR_NO_PATH\r\n", rc );				break;			/* (5) Could not find the path */
		case FR_INVALID_NAME		:	GLogN( "rc = %u FR_INVALID_NAME\r\n", rc );		    break;			/* (6) The path name format is invalid */
		case FR_DENIED				:	GLogN( "rc = %u FR_DENIED\r\n", rc );				break;			/* (7) Access denied due to prohibited access or directory full */
		case FR_EXIST				:	GLogN( "rc = %u FR_EXIST\r\n", rc );				break;			/* (8) Access denied due to prohibited access */
		case FR_INVALID_OBJECT		:	GLogN( "rc = %u FR_INVALID_OBJECT\r\n", rc );		break;			/* (9) The file/directory object is invalid */
		case FR_WRITE_PROTECTED		:	GLogN( "rc = %u FR_WRITE_PROTECTED\r\n", rc );		break;			/* (10) The physical drive is write protected */
		case FR_INVALID_DRIVE		:	GLogN( "rc = %u FR_INVALID_DRIVE\r\n", rc );		break;			/* (11) The logical drive number is invalid */
		case FR_NOT_ENABLED			:	GLogN( "rc = %u FR_NOT_ENABLED\r\n", rc );			break;			/* (12) The volume has no work area */
		case FR_NO_FILESYSTEM		:	GLogN( "rc = %u FR_NO_FILESYSTEM\r\n", rc );		break;			/* (13) There is no valid FAT volume */
		case FR_MKFS_ABORTED		:	GLogN( "rc = %u FR_MKFS_ABORTED\r\n", rc );	    	break;			/* (14) The f_mkfs() aborted due to any problem */
		case FR_TIMEOUT				:	GLogN( "rc = %u FR_TIMEOUT\r\n", rc );				break;			/* (15) Could not get a grant to access the volume within defined period */
		case FR_LOCKED				:	GLogN( "rc = %u FR_LOCKED\r\n", rc );				break;			/* (16) The operation is rejected according to the file sharing policy */
		case FR_NOT_ENOUGH_CORE		:	GLogN( "rc = %u FR_NOT_ENOUGH_CORE\r\n", rc );		break;			/* (17) LFN working buffer could not be allocated */
		case FR_TOO_MANY_OPEN_FILES	:	GLogN( "rc = %u FR_TOO_MANY_OPEN_FILES\r\n", rc );	break;			/* (18) Number of open files > _FS_LOCK */
		case FR_INVALID_PARAMETER	:	GLogN( "rc = %u FR_INVALID_PARAMETER\r\n", rc );	break;			/* (19) Given parameter is invalid */
		default						:	GLogN( "rc = %u UNKNOWN\r\n", rc );					break;
	}
}

void directory_list()
{
	DIR Dir;
	FRESULT res;
	FILINFO Finfo;
	UINT s1 = 0, s2 = 0;
	long p1;
	FATFS *fs;
	char* strScanDirectory = "/";

	memset(&Finfo, NULL, sizeof(Finfo));

	res = f_opendir(&Dir, strScanDirectory);

	if (res) { put_rc((FRESULT)res); return; }

	for(;;)
	{
		res = f_readdir(&Dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0])
			break;
		GLogN( "\r\n%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s",
		(Finfo.fattrib & AM_DIR) ? 'D' : '-',
		(Finfo.fattrib & AM_RDO) ? 'R' : '-',
		(Finfo.fattrib & AM_HID) ? 'H' : '-',
		(Finfo.fattrib & AM_SYS) ? 'S' : '-',
		(Finfo.fattrib & AM_ARC) ? 'A' : '-',
		(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
		(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
		Finfo.fsize, &(Finfo.fname[0]));
		GLogN( "\n");
		if (Finfo.fattrib & AM_DIR)
		{
			s2++;
		}
		else
		{
			s1++; p1 += Finfo.fsize;
		}
	}

	GLogN( "\r\n%4u File(s),%10lu bytes total%4u Dir(s)", s1, p1, s2);

	if (f_getfree(strScanDirectory, (DWORD*)&p1, &fs) == FR_OK)
		GLogN( ", %d Mbytes free\r\n", (uint32_t)((uint64_t)p1 * ((uint64_t)fs->csize * (uint64_t)512)>>20));

}
void directory_listWithPath(char *DirPath)
{
	DIR Dir;
	FRESULT res;
	FILINFO Finfo;
	UINT s1 = 0, s2 = 0;
	long p1;
	FATFS *fs;
	char* strScanDirectory = DirPath;

	memset(&Finfo, NULL, sizeof(Finfo));

	res = f_opendir(&Dir, strScanDirectory);

	if (res) { put_rc((FRESULT)res); return; }

	for(;;)
	{
		res = f_readdir(&Dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0])
			break;
		GLogN( "\r\n%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s",
		(Finfo.fattrib & AM_DIR) ? 'D' : '-',
		(Finfo.fattrib & AM_RDO) ? 'R' : '-',
		(Finfo.fattrib & AM_HID) ? 'H' : '-',
		(Finfo.fattrib & AM_SYS) ? 'S' : '-',
		(Finfo.fattrib & AM_ARC) ? 'A' : '-',
		(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
		(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
		Finfo.fsize, &(Finfo.fname[0]));
		GLogN( "\n");
		if (Finfo.fattrib & AM_DIR)
		{
			s2++;
		}
		else
		{
			s1++; p1 += Finfo.fsize;
		}
	}

	GLogN( "\r\n%4u File(s),%10lu bytes total%4u Dir(s)", s1, p1, s2);

	if (f_getfree(strScanDirectory, (DWORD*)&p1, &fs) == FR_OK)
		GLogN( ", %d Mbytes free\r\n", (uint32_t)((uint64_t)p1 * ((uint64_t)fs->csize * (uint64_t)512)>>20));

}

void directory_list2(unsigned char *DirPath, unsigned char *FileInfoToPC, int *SizeInfoToPC)
{
  DIR Dir;
  FRESULT res;
  FILINFO Finfo;
  char Lfname[512];
  UINT s1 = 0, s2 = 0;
  long p1=0;
  FATFS *fs;
  char* strScanDirectory = (char*)DirPath; //= "Record/Record";
  char FileLen = 0;
  char ReadBuff[500] = {0x00, };

  unsigned char FileNameSize = 0;
  int FileCnt = 0;
  unsigned char FileSize[4] = {0x00, };
  unsigned char arrFileCS[4] = {0x00, };

  FIL temp;
  char Fname[20] = {0x00, };
  int FileCS = 0;
  int readsize = 0;
  int N = 0;
  int SEEK = 0;
  char FileEA = 0;
  char FF_Flag = 0;

  memset(&Finfo, NULL, sizeof(Finfo));
  memset(FileInfoToPC, 0x00, sizeof(FileInfoToPC));

  res = f_opendir(&Dir, strScanDirectory);
  if(f_chdir(strScanDirectory) == FR_OK) GLogN("\r\n--------------- PATH OPEN SUCCESS ---------------\r\n");

  if (res)
  {
    memset(&FileInfoToPC[0], 0x01, 1);
    memset(&FileInfoToPC[1], res, 1);
    *SizeInfoToPC = 2;
    put_rc((FRESULT)res);
    return;
  }

  for(;;)
  {
    res = f_readdir(&Dir, &Finfo);
    if(res != FR_OK)
    {

      memset(&FileInfoToPC[0], 0x01, 1);
      memset(&FileInfoToPC[1], res, 1);
      *SizeInfoToPC = 2;
      return;
    }

    FileLen = strlen(Finfo.fname);
    memcpy(Lfname, Finfo.fname, FileLen);

    if ((res != FR_OK) || !Finfo.fname[0])
    {
      break;
    }

    if (Finfo.fattrib & AM_DIR)
    {
      s2++; FF_Flag=1;
    }
    else
    {
      s1++; p1 += Finfo.fsize;
    }

    FileNameSize = strlen(Finfo.fname);

    memset(Fname, 0x00, sizeof(Fname));
    memset(ReadBuff, 0x00, sizeof(ReadBuff));
    FileCS = 0;
    sprintf(Fname, "%s", Finfo.fname);
    readsize = 0;
    N = 0;
    SEEK = 0;

    if(f_open(&temp, Fname, FA_OPEN_EXISTING | FA_READ) == FR_OK)
    {
      if(Finfo.fsize < 500)
      {
        if(f_read(&temp, ReadBuff, Finfo.fsize, (UINT*)&readsize) == FR_OK)
        {
          for(int j=0; j<sizeof(ReadBuff); j++)
          {
            FileCS += ReadBuff[j];
          }
        }
      }

      else
      {
        for(int i=0; i<((Finfo.fsize)/500)+1; i++)
        {
          memset(ReadBuff, 0x00, sizeof(ReadBuff));
          if( f_lseek( &temp, SEEK ) == FR_OK )
          {
            if(i == ((Finfo.fsize)/500))
            {N = (Finfo.fsize)%500;}
            else N = 500;

            if(f_read(&temp, ReadBuff, N, (UINT*)&readsize) == FR_OK)
            {
              //if(N != 500) UartPrintf(&U1, "CS :  ");
              for(int j=0; j<sizeof(ReadBuff); j++)
              {
                FileCS += ReadBuff[j];
                //if(N != 500) UartPrintf(&U1, "%02X ",ReadBuff[j]);
              }
              SEEK += sizeof(ReadBuff);
            }
          }
        }
      }
    }

    memset(&FileInfoToPC[2], s1, 1);
    memset(&FileInfoToPC[3], s2, 1);
    memset(&FileInfoToPC[4+FileCnt], s1+s2, 1);
    memset(&FileInfoToPC[5+FileCnt], FF_Flag, 1);
    memset(&FileInfoToPC[6+FileCnt], FileNameSize, 1); //Fiile Name Size
    memcpy(&FileInfoToPC[7+FileCnt], Finfo.fname, FileNameSize); //File Name

    memcpy(&FileSize[0], &Finfo.fsize, 4);
    memcpy(&FileInfoToPC[7+FileCnt+FileNameSize], &FileSize[0]+3, 1); //FileSize
    memcpy(&FileInfoToPC[7+FileCnt+FileNameSize]+1, &FileSize[0]+2, 1); //FileSize
    memcpy(&FileInfoToPC[7+FileCnt+FileNameSize]+2, &FileSize[0]+1, 1); //FileSize
    memcpy(&FileInfoToPC[7+FileCnt+FileNameSize]+3, &FileSize[0], 1); //FileSize

    memcpy(&arrFileCS[0], &FileCS, 4);
    memcpy(&FileInfoToPC[7+FileCnt+FileNameSize+4], &arrFileCS[0]+1, 1); //CS
    memcpy(&FileInfoToPC[7+FileCnt+FileNameSize+4+1], &arrFileCS[0], 1); //CS


    FileCnt += 4+FileNameSize+4+2 -1; //Value for calculating the size of data to be sent to PC
    FileEA += 1;
    FF_Flag = 0;

    GLogN("\r %s --> NameSize : %d  FileSize : %lu  CS : %02X %02X \n\n",Finfo.fname, FileNameSize, Finfo.fsize, arrFileCS[1],arrFileCS[0]);

    char closeRes = 0;
    closeRes = (f_close(&temp));
    if(closeRes == FR_OK) GLogN("FILE CLOSE SUCCESS");
    else GLogN("FILE CLOSE FAIL(%02x)", closeRes);

  }
  *SizeInfoToPC = FileCnt + 4;
  memset(&FileInfoToPC[0], 0x00, 1); // Success
  memset(&FileInfoToPC[1], FileEA, 1);

  if (f_getfree(strScanDirectory, (DWORD*)&p1, &fs) == FR_OK)
    GLogN(", %d Mbytes free\r\n", (uint32_t)((uint64_t)p1 * ((uint64_t)fs->csize * (uint64_t)512)>>20));
}

U8 VCI_BlockBox_Fat_Info(U8 *DirBuff, U32 *DirBuffSize)
{
	DIR Dir;
	FRESULT res;
	FILINFO Finfo;
	UINT s1=0, s2=0;//,s3=0;
	long p1;
	FATFS *fs;
	//U8 filename[20][25];
	U8 Configname[12],Configname2[12];
	U8 *filename = NULL;//, *filename_tmp = NULL;
	U16 i, usfilecnt=0,cnt=0, j;
	char* strScanDirectory = "03_Record";

	sprintf((char*)Configname,"CONFIG.DAT");
	sprintf((char*)Configname2,"config.dat");

	memset(&Finfo, NULL, sizeof(Finfo));

	res = f_opendir(&Dir, strScanDirectory);

	if (res) { put_rc((FRESULT)res); return res; }

    while(1)
    {
	    res = f_readdir(&Dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0])
			break;
		if (Finfo.fattrib & AM_DIR){}
        else usfilecnt++;
    }

    if(usfilecnt>FILE_NUM_MAX) usfilecnt=FILE_NUM_MAX;
    //if(usfilecnt>FILE_CNT_MAX) usfilecnt=FILE_CNT_MAX;

	filename     = (U8*)malloc(usfilecnt*25);
	//filename_tmp = (U8*)malloc(usfilecnt*25);
    if( filename == NULL ) return 0xff;

	memset(filename, NULL, usfilecnt*25);
	//memset(filename_tmp, NULL, usfilecnt*25);

	res = f_opendir(&Dir, strScanDirectory);

	if (res) { put_rc((FRESULT)res); return res; }

	for(i=0; i<usfilecnt;)
	{
		res = f_readdir(&Dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0])
			break;

		if (Finfo.fattrib & AM_DIR)
		{
			s2++;
		}
		else
		{
			if((strncmp ((char const*)&Configname[0],&Finfo.fname[0],10)  == 0) || (strncmp ((char const*)&Configname2[0],&Finfo.fname[0],10)  == 0))	//strupr() �ȸԾ �빮�ڰ��,�ҹ��ڰ�� �ΰ�츸 ó�� 160404_KCH(LWH����)
			{
				memcpy(&DirBuff[0], Finfo.fname, 20);
				memcpy(&DirBuff[20], &Finfo.fsize, sizeof(Finfo.fsize));
				//i++;
			}
			else if( (strncmp(&Finfo.fname[15],".REF",4)!=0) && (strncmp(&Finfo.fname[0],"TEMP",4)!=0) )		//���ڵ����� �߿� .REF������ ���ڵ����Ϸ� ������� �ʴ´�	150107 LWH
			{
#if _USE_LFN
				if(*Finfo.fname!=0)
				{
					memcpy(&filename[i*25], Finfo.fname, 20);
					memcpy(&filename[i*25+20], &Finfo.fsize, sizeof(Finfo.fsize));
				}
				else
				{
					memcpy(&filename[i*25],  Finfo.fname, sizeof(Finfo.fname));
					memcpy(&filename[i*25+20], &Finfo.fsize, sizeof(Finfo.fsize));
				}
#else
				memcpy(&filename[i*25],  Finfo.fname, sizeof(Finfo.fname));
				memcpy(&filename[i*25+20], &Finfo.fsize, sizeof(Finfo.fsize));
#endif
				i++;
			}
			s1++; p1 += Finfo.fsize;
		}

		GLogN( "\r\n%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s",
		(Finfo.fattrib & AM_DIR) ? 'D' : '-',
		(Finfo.fattrib & AM_RDO) ? 'R' : '-',
		(Finfo.fattrib & AM_HID) ? 'H' : '-',
		(Finfo.fattrib & AM_SYS) ? 'S' : '-',
		(Finfo.fattrib & AM_ARC) ? 'A' : '-',
		(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
		(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
		Finfo.fsize, &(Finfo.fname[0]));
	}
	GLogN( "\r\n%4u File(s),%10lu bytes total%4u Dir(s)", s1, p1, s2);

	if (f_getfree(strScanDirectory, (DWORD*)&p1, &fs) == FR_OK)
		GLogN( ", %d Mbytes free\r\n", (uint32_t)((uint64_t)p1 * ((uint64_t)fs->csize * (uint64_t)512)>>20));

	usfilecnt =i+1;	//150107 LWH config.dat도 갯수 포함이라 +1
    //최신것 부터 옛날로
    cnt=usfilecnt;
    if(usfilecnt>FILE_CNT_MAX) cnt=FILE_CNT_MAX;      //30개 다 가져와 놓고 그 중에서 최신걸로 가져간다

    for(i=1; i<cnt; i++)	//20151228 LWH
    {
        for(j=0;j<25;j++)
        {
            DirBuff[i*25+j] =  filename[(usfilecnt-i-1)*25+j];
        }
    }

	*DirBuffSize = (U32)(i*25);
    //DirBuff = filename;                          //이 라인과 아래 라인 중 하나를 했을 때 free 에러 발생하기도 한다
	//memcpy(DirBuff, filename, *DirBuffSize);

    //DirBuff = filename_tmp;
	//memcpy(DirBuff, filename_tmp, *DirBuffSize);

	free(filename);
	//free(filename_tmp);
	return 0;
}

void GetRecordFileList()
{
	u8 a[4000];	//파일 저장 갯수 재정의 필요
	u32 b;

    VCI_BlockBox_Fat_Info(a,&b);
}

void BlockBox_FAT_Erase(void)
{
	U8 ret;
	DIR Dir;
	FRESULT res;
	FILINFO Finfo;
	char* strScanDirectory = "/03_Record";
	char* strScanDirectoryRef = "/03_Record/Ref";

	memset(&Finfo, NULL, sizeof(Finfo));

	f_chdir(DIR_ROOT);
	res = f_opendir(&Dir, strScanDirectory);

	if (res) { put_rc((FRESULT)res); return; }

	for(;;)
	{
		res = f_readdir(&Dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0])
			break;


		//MONI need to check
		f_chdir(DIR_RECORD);


		ret=f_unlink(Finfo.fname);
		GLogN( "f_unlink go######## %d @@@@@@   %s !!!!!!!", ret, Finfo.fname);
	}
	f_chdir(DIR_ROOT);

	memset(&Finfo, NULL, sizeof(Finfo));

	f_chdir("Ref");
	res = f_opendir(&Dir, strScanDirectoryRef);

	if (res) { put_rc((FRESULT)res); GLogN( "NONONONO!!!!!!!");return; }

	for(;;)
	{
		res = f_readdir(&Dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0])
			break;

		//MONI need to check
		f_chdir(DIR_RECORD);

		ret=f_unlink(Finfo.fname);
		GLogN( "f_unlink go222--######## %d @@@@@@   %s !!!!!!!", ret, Finfo.fname);
	}
	f_chdir(DIR_ROOT);
}

U8 VciDirFile(U8 *DirBuff, U32 *DirBuffSize)
{
	return VCI_BlockBox_Fat_Info(DirBuff, DirBuffSize);
	//return 0;
}

U8 VciRecvFileOpen(char *FileName, U32 FileSize)
{
    FRESULT result = FR_OK;
	g_u32UpdateFileSize = 0;

	memset(g_strDownloadFW_FileName, NULL, sizeof(g_strDownloadFW_FileName));
	memcpy(g_strDownloadFW_FileName, FileName,sizeof(g_strDownloadFW_FileName));
#if 1	//��������
    result = f_chdir(DIR_RECORD);
	if ( result == FR_OK )
	{
		GLogN( "Change Directory Record Ok\r\n");
	}
	else	return result;

	memset(&g_TempFilepnt, 0x0, sizeof(g_TempFilepnt));
    result = f_open(&g_TempFilepnt, DOWNLOAD_FW_TEMP_FILE_NAME, FA_CREATE_ALWAYS | FA_WRITE | FA_READ );
	if ( result == FR_OK )
	{
		GLogN( "%s File Open Success\r\n", DOWNLOAD_FW_TEMP_FILE_NAME);
		f_truncate(&g_TempFilepnt);
    	f_close(&g_TempFilepnt);
		return FR_OK;
	}
	else
		GLogE( "%s File Open fail\r\n", DOWNLOAD_FW_TEMP_FILE_NAME);

    f_close(&g_TempFilepnt);
#else	//신규로직	//파일 오픈 한번만 하는 로직
	memset(&g_TempFilepnt, 0x0, sizeof(g_TempFilepnt));
    result = f_open(&g_TempFilepnt, DOWNLOAD_FW_TEMP_FILE_NAME, FA_CREATE_ALWAYS | FA_WRITE | FA_READ );
	if ( result == FR_OK )
	{
		GLogN( "%s File Open Success\r\n", DOWNLOAD_FW_TEMP_FILE_NAME);
		f_truncate(&g_TempFilepnt);
		return FR_OK;
	}
	else
		GLogE( "%s File Open fail\r\n", DOWNLOAD_FW_TEMP_FILE_NAME);
#endif
	return result;
}
U8 VciRecvFileOpenWithPath(U8 FileOpenOption, char *DirPath, char *FileName, U32 FileSize)
{
    FRESULT result = FR_OK;
	g_u32UpdateFileSize = 0;
	char* strScanDirectory = DirPath; //= "Record/Record";

	//memset(g_strTemp_FileName, NULL, sizeof(g_strTemp_FileName));
	memcpy(g_strTemp_FileName, FileName,sizeof(g_strTemp_FileName));
	//파일 오픈 한번만 하는 로직
  	result = f_chdir(strScanDirectory);
	if ( result == FR_OK )
	{
		GLogN( "Change Directory %s Ok\r\n", strScanDirectory);
	}
	else
	{	
		f_chdir(DIR_ROOT);
		return FALSE;
	}
	if ( result == FR_OK )
	{
		GLogN( "Change Directory Record Ok\r\n");
	}
	else	return result;

	memset(&g_TempFilepnt, 0x0, sizeof(g_TempFilepnt));
    //result = f_open(&g_TempFilepnt, DOWNLOAD_FW_TEMP_FILE_NAME, FA_CREATE_ALWAYS | FA_WRITE | FA_READ );
    result = f_open(&g_TempFilepnt, DOWNLOAD_FW_TEMP_FILE_NAME, FileOpenOption | FA_WRITE | FA_READ );
	if ( result == FR_OK )
	{
		GLogN( "%s File Open Success\r\n", DOWNLOAD_FW_TEMP_FILE_NAME);
		f_truncate(&g_TempFilepnt);
    	//f_close(&g_TempFilepnt);
		return FR_OK;
	}
	else
	{
		GLogE( "%s File Open fail\r\n", DOWNLOAD_FW_TEMP_FILE_NAME);
    	f_close(&g_TempFilepnt);
	}

	return result;
}


U8 VciRecvFileReceive(U8 *buff, U16 Size)
{
    FRESULT result = FR_OK;
    UINT uiWriteNum;
#if 1	//��������
    result = f_open(&g_TempFilepnt, DOWNLOAD_FW_TEMP_FILE_NAME, FA_WRITE);
    if ( result == FR_OK )
    {
        //GLogN( "1");
        result = f_lseek(&g_TempFilepnt,g_u32UpdateFileSize);
        if( result == FR_OK )
        {
            result = f_write(&g_TempFilepnt, buff, Size, &uiWriteNum);
            if( result == FR_OK )
            {
                g_u32UpdateFileSize += Size;
            }
            else    GLogE( "%s File Seek fail\r\n", DOWNLOAD_FW_TEMP_FILE_NAME);
        }
        else
        {
            GLogE( "%s File Seek fail\r\n", DOWNLOAD_FW_TEMP_FILE_NAME);
        }
    }
    else
      GLogE( "%s File Open fail\r\n", DOWNLOAD_FW_TEMP_FILE_NAME);

	result = f_close(&g_TempFilepnt);
	return result;
#else	//�űԷ���	//���� ���� �ѹ��� �ϴ� ����
	U32 i;
    result = f_write(&g_TempFilepnt, (void*)buff, Size, &uiWriteNum);
    if( result != FR_OK )
    {
        for ( i=0; i<3; i++ )
		{
			// retry to write file
            result = f_write(&g_TempFilepnt, (void*)buff, Size, &uiWriteNum);
			if ( result != FR_OK )
			{
				GLogE("-file rewrite fail[retry cnt %d]\r\n", i);
			}
			else
			{
				g_u32UpdateFileSize += Size;
				break;
			}
		}
    }
    else
	{
		g_u32UpdateFileSize += Size;
		GLogN( "1");
    }
	return result;
#endif


}

U8 VciRecvFileClose(void)
{
    FRESULT result = FR_OK;

    f_close(&g_TempFilepnt);
	f_unlink(g_strDownloadFW_FileName);
	result |= f_rename(DOWNLOAD_FW_TEMP_FILE_NAME, g_strDownloadFW_FileName);
	memset(&g_TempFilepnt, 0x0, sizeof(g_TempFilepnt));

	GLogN( "\r\n%s %s rename %s\r\n",
			   __FUNCTION__, DOWNLOAD_FW_TEMP_FILE_NAME, g_strDownloadFW_FileName);

    result |= f_chdir(DIR_ROOT);

	if ( result == FR_OK )
	{
		GLogN( "Change Directory root Ok\r\n");
		directory_list();
	}
	return result;
}
U8 VciRecvFileCloseBigName(void)
{
    FRESULT result = FR_OK;

    f_close(&g_TempFilepnt);
	f_unlink(g_strTemp_FileName);
	result |= f_rename(DOWNLOAD_FW_TEMP_FILE_NAME, g_strTemp_FileName);
	memset(&g_TempFilepnt, 0x0, sizeof(g_TempFilepnt));

	GLogN( "\r\n%s %s rename %s\r\n",
			   __FUNCTION__, DOWNLOAD_FW_TEMP_FILE_NAME, g_strTemp_FileName);

    result |= f_chdir(DIR_ROOT);

	if ( result == FR_OK )
	{
		GLogN( "Change Directory root Ok\r\n");
	}
	return result;
}
U8 VciSendFileOpenWithPath(char *DirPath,  char *FileName, U32 *FileSize)
{
	FRESULT result = FR_OK;
	char* strScanDirectory = DirPath;

	g_u32UpdateFileSize =0;
	//memset(g_strTemp_FileName, NULL, sizeof(g_strTemp_FileName));
	memcpy(g_strTemp_FileName, FileName,sizeof(g_strTemp_FileName));
	//파일 오픈 한번만 하는 로직
  	result = f_chdir(strScanDirectory);
	if ( result == FR_OK )
	{
		GLogN( "Change Directory %s Ok\r\n", strScanDirectory);
	}
	else
	{	
		f_chdir(DIR_ROOT);
		return FALSE;
	}
	if ( result == FR_OK )
	{
		GLogN( "Change Directory Record Ok\r\n");
	}
	else	return result;

	memset(&g_TempFilepnt, 0x0, sizeof(g_TempFilepnt));

	result = f_open(&g_TempFilepnt, g_strTemp_FileName, FA_OPEN_EXISTING | FA_READ);
		
	if ( result != FR_OK )
	{
		GLogE( "%s File Open fail\r\n", g_strTemp_FileName); 
		*FileSize=0;
		return result;
	}
	//time2 = Get_TmrDelta(Get_Tmr(),time);
	//GLogN( "t1: %d ",time2);
	//time = Get_Tmr();
	*FileSize = dwFileSizeFast = f_size(&g_TempFilepnt);	//����ũ��
	//time2 = Get_TmrDelta(Get_Tmr(),time);
	//GLogN( "t2: %d ",time2);

	return result;
}
U8 VciReadFile_Send(U32 ReadSize, U8 *FileBuff, U32 *FileSize, U32 uiSequence)
{
	uint32_t	bytes		= 0;
	FRESULT result = FR_OK;
	//f_chdir(DIR_RECORD);
	GLogN( ",");

	result = f_lseek(&g_TempFilepnt,(uiSequence*ReadSize));
	if(result == FR_OK)
	{
		if(((uiSequence+1)*ReadSize) >= dwFileSizeFast)	ReadSize= dwFileSizeFast - (uiSequence*ReadSize);
		if ( f_read(&g_TempFilepnt, FileBuff, ReadSize, (void *)&bytes) == FR_OK )    *FileSize = ReadSize;
	}
	else return result;

	if(ReadSize==0) *FileSize = ReadSize; 	//150109 LWH

	return result;
}

U8 VciSendFileOpen(unsigned char FileName[25], U32 *FileSize)
{
	U8 DirBuff[FILE_CNT_MAX][25], i;  //���� ���� ���� ������ �ʿ�
	U32 DirBuffSize, uiFileTotalSize;

	g_u32UpdateFileSize =0;
	memset(g_strDownloadFW_FileName, NULL, sizeof(g_strDownloadFW_FileName));
	memcpy(&g_strDownloadFW_FileName[0], FileName, 25);

	uiFileTotalSize =  (U32)FileName[20];
	uiFileTotalSize += (U32)FileName[21]<<8;
	uiFileTotalSize += (U32)FileName[22]<<16;
	uiFileTotalSize += (U32)FileName[23]<<24;

	memset(DirBuff, NULL, sizeof(DirBuff)/sizeof(DirBuff[0]));
    VCI_BlockBox_Fat_Info(&DirBuff[0][0], &DirBuffSize);

	for(i=0;i<FILE_CNT_MAX ; i++)
	{
		if (DirBuff[i][0] == 0) //����.
		{
			break;
		}
		else
		{
			//�ִ�.
			if (strncmp ((char const*)&DirBuff[i][1], (char const*)&FileName[1], 11)  == 0)
			{
				break;
			}
		}
	}
	*FileSize = uiFileTotalSize;
	return 0;
}
U8 VciEraseFileWithPath(char *DirPath,  char *FileName, U8 EraseOption)
{
	FRESULT result = FR_OK;
	char* strScanDirectory = DirPath;
	//FRESULT res;
	FILINFO Finfo;
	U32 i;
	DIR Dir;

	f_chdir(DIR_ROOT);
	if(EraseOption==0)
	{
	  	result = f_chdir(strScanDirectory);
		if ( result == FR_OK )
		{
			GLogN( "Change Directory %s Ok\r\n", strScanDirectory);
		}
		else
		{	
			f_chdir(DIR_ROOT);
			return result;
		}
		result=f_unlink(FileName);
	}
	else if(EraseOption==1)
	{
		memset(&Finfo, NULL, sizeof(Finfo));

		result = f_opendir(&Dir, strScanDirectory);

		if (result) { put_rc((FRESULT)result); return result; }
	  	for(i=0; i<10000; i++)
		{
			result = f_readdir(&Dir, &Finfo);
			if ((result != FR_OK) || !Finfo.fname[0])
				break;


			//MONI need to check
			f_chdir(strScanDirectory);


			result=f_unlink(Finfo.fname);
			GLogN( "f_unlink #%d##  %s  !!!!!!!", result, Finfo.fname);
		}
	}
	f_chdir(DIR_ROOT);

	return result;
}
U8 VciEraseFileOrDir(char *DirPath)
{
	FRESULT result = FR_OK;
	char* strScanDirectory = DirPath;

	result = f_chdir(DIR_ROOT);
	if(result != FR_OK) return result;
	result = f_unlink(strScanDirectory);
	if(result != FR_OK) return result;
	result = f_chdir(DIR_ROOT);

	return result;
}

U8 VciMakeDirWithPath(char *DirPath)
{
	FRESULT result = FR_OK;
	char* strScanDirectory = DirPath;
	
	result = f_chdir(DIR_ROOT);
	if(result != FR_OK) return result;
	result = f_mkdir(strScanDirectory);
	if(result != FR_OK) return result;
	result = f_chdir(DIR_ROOT);
	
	return result;
}


U8 VciReadFile_Slave(U32 ReadSize, U8 *FileBuff, U32 *FileSize, U32 uiSequence)
{
	uint32_t	bytes		= 0;
	f_chdir(DIR_RECORD);
	if(0==uiSequence)	// 1st Sequence -> file open
	{
		//time = Get_Tmr();
		if ( f_open(&fpFast, g_strDownloadFW_FileName, FA_OPEN_EXISTING | FA_READ) != FR_OK )
		{
			GLogE( "%s File Open fail\r\n", g_strDownloadFW_FileName); return 1;
		}
		//time2 = Get_TmrDelta(Get_Tmr(),time);
		//GLogN( "t1: %d ",time2);
		//time = Get_Tmr();
		dwFileSizeFast = f_size(&fpFast);	//����ũ��
		//time2 = Get_TmrDelta(Get_Tmr(),time);
		//GLogN( "t2: %d ",time2);
	}
	//time = Get_Tmr();
	GLogN( ",");

	if(f_lseek(&fpFast,((uiSequence*ReadSize))) == FR_OK)
	{
		//time2 = Get_TmrDelta(Get_Tmr(),time);
		//GLogN( "t3: %d ",time2);
		if(((uiSequence+1)*ReadSize) >= dwFileSizeFast)	ReadSize= dwFileSizeFast - (uiSequence*ReadSize);

		//time = Get_Tmr();
		if ( f_read(&fpFast, FileBuff, ReadSize, (void *)&bytes) == FR_OK )    *FileSize = ReadSize;
		//if ( f_read(&fpFast, g_temp, ReadSize, (void *)&bytes) == FR_OK )    *FileSize = ReadSize;
		//time2 = Get_TmrDelta(Get_Tmr(),time);
		//GLogN( "t4: %d ",time2);
        //osDelay(1);
	}

	//if(ReadSize<4000)	// Last Sequence -> file close	//VciSendFileClose() �̵�
	//{
	//	f_close(&fpFast);
	//}
	if(ReadSize==0) *FileSize = ReadSize; 	//150109 LWH

	return 0;
}

U8 VciSendFileClose(void)
{
	f_close(&fpFast);

	GLogN( "%s File Close Success\r\n", g_strDownloadFW_FileName);
    f_chdir(DIR_ROOT);
 	return 0;
}
U8 VciSendFileCloseBigName(void)
{
	FRESULT result = FR_OK;

	result = f_close(&g_TempFilepnt);
	if(result == FR_OK)
	{
		GLogN( "%s File Close Success\r\n", g_strTemp_FileName);
	}
	else GLogE( "%s File Close Fail !!\r\n", g_strTemp_FileName); 
    f_chdir(DIR_ROOT);
 	return result;
}


U8 VciEraseFile(void)
{
	BlockBox_FAT_Erase();
	return 0;
}

U8 GetUpgradeFileInfo(U8 *pData)
{
    FRESULT result = FR_OK;
	int nTokenCnt = 0;
	//char seps[] = " ,:;\t\r\n", *token;
	char seps[] = " ,:;\t\r\n.", *token;
	char *ptr;
	char strTempFileName[25]={0,};

	token = strtok_r((char*)pData, seps, &ptr);
	while ( token != NULL )
	{
		switch ( nTokenCnt )
		{
		case 0:
		  	g_ucDownloadFW_FileNo = atoi(token);
			GLogN( "Download Start -> App Number %d,", g_ucDownloadFW_FileNo);
			nTokenCnt = 1;
			break;
		case 1:
		{
			if( g_ucDownloadFW_FileNo == eApp_bootloader )	// bootloader: always save as no-prefix name (match flash path)
			{
				strcpy(g_strDownloadFW_FileName, "vci3_bootloader.bin");
			}
			else
			{
				strcpy(g_strDownloadFW_FileName,token);
				strcat(g_strDownloadFW_FileName, ".bin");
			}
			GLogN( " App Name %s\r\n", g_strDownloadFW_FileName);
			nTokenCnt = 2;
			break;
		}
		case 2:
		default:
			nTokenCnt = 0;
			break;
		}
		token = strtok_r(NULL, seps, &ptr);
        if(nTokenCnt == 0) break;       //�ڿ� �����Ⱑ �پ��־ 2������ �ϰ� ���������°� �´�
	}

//#if JAY_TEST
//#else
//	if(strncmp(g_strDownloadFW_FileName, "vci3", sizeof("vci3")) ||  strncmp(g_strDownloadFW_FileName, "Repro", sizeof("Repro")))
//	  return FR_INVALID_NAME;
//
//#endif
	g_u32UpdateFileSize = 0;

	memset(strTempFileName,0x00,sizeof(strTempFileName));
	strncpy(strTempFileName,g_strDownloadFW_FileName,sizeof(strTempFileName));
	strTempFileName[24]=0x00;

	for ( int index= 0; index < strlen( strTempFileName); index++)
	{
		strTempFileName[index] = (char)toupper( strTempFileName[index]);
	}

	
	//VCI2 file filter
    if((strncmp(strTempFileName,"DOWNLOADER.BIN",14)==0) 
		|| (strncmp(strTempFileName,"ECUUPCAN.BIN",12)==0)
		|| (strncmp(strTempFileName,"ECUUPCCP.BIN",12)==0)
		|| (strncmp(strTempFileName,"ECUUPCV.BIN",11)==0)
		|| (strncmp(strTempFileName,"ECUUPCVKWP.BIN",14)==0)
		|| (strncmp(strTempFileName,"ECUUPDOWNLOADER.BIN",19)==0)
		|| (strncmp(strTempFileName,"ECUUPKWP.BIN",12)==0)
		|| (strncmp(strTempFileName,"INSIDE.BIN",10)==0)
		|| (strncmp(strTempFileName,"INSIDE2.BIN",11)==0)
		|| (strncmp(strTempFileName,"RECOVERY.BIN",12)==0)
		|| (strncmp(strTempFileName,"SELFTEST.BIN",12)==0)
		|| (strncmp(strTempFileName,"VCI_2.BIN",9)==0)
		|| (strncmp(strTempFileName,"VCI_BOOTLOADER.BIN",18)==0)
		|| (strncmp(strTempFileName,"VCI_II_PDI.BIN",14)==0)
		//|| (strncmp(strTempFileName,"TM_BOOTLOADER.BIN",17)==0)	//VCI3 same file
		//|| (strncmp(strTempFileName,"TM_DOWNLOADER.BIN",17)==0)	//VCI3 same file
		//|| (strncmp(strTempFileName,"TRIGGER_MODULE.BIN",18)==0)	//VCI3 same file
		)
    {
    	GLogE( "file name check : %s\r\n",strTempFileName);
		return FR_INVALID_NAME;
		
    }
	else //if((strncmp(strTempFileName,"REPRO_CAN.BIN",13)==0)
			//|| (strncmp(strTempFileName,"REPRO_CCP.BIN",13)==0)
			//|| (strncmp(strTempFileName,"REPRO_DOWN.BIN",14)==0)
			//|| (strncmp(strTempFileName,"REPRO_KWP.BIN",13)==0)
			//|| (strncmp(strTempFileName,"TM_BOOTLOADER.BIN",17)==0)	//VCI3 same file
			//|| (strncmp(strTempFileName,"TM_DOWNLOADER.BIN",17)==0)	//VCI3 same file
			//|| (strncmp(strTempFileName,"TRIGGER_MODULE.BIN",18)==0)	//VCI3 same file
			//|| (strncmp(strTempFileName,"VCI3_BOOTLOADER.BIN",19)==0)
			//|| (strncmp(strTempFileName,"VCI3_MAIN.BIN",13)==0)
			//|| (strncmp(strTempFileName,"VCI3_PDI.BIN",12)==0)
			//|| (strncmp(strTempFileName,"RS9116FW.BIN",12)==0)
			//)
	{
		GLogN( "file name check : %s\r\n",strTempFileName);
	}

	if( g_bDeleteFileFlag == true )
	{
		g_bDeleteFileFlag = false;
		f_close(&g_SaveTempFilepnt);
	}
	f_close(&g_TempFilepnt);

	memset(&g_TempFilepnt, 0x0, sizeof(g_TempFilepnt));
	result = f_chdir(DIR_ROOT);
	if ( result == FR_OK )
	{
		GLogN( "Change Directory DIR_ROOT Ok\r\n");
	}
	else
	{
		GLogN("result:%d\r\n",result);
		return result;
	}

	result = f_chdir(DIR_APP);
	if ( result == FR_OK )
	{
		GLogN( "Change Directory Application Ok\r\n");
	}
	else
	{
		GLogN("result2:%d\r\n",result);
		return result;
	}

    result = f_open(&g_TempFilepnt, DOWNLOAD_FW_TEMP_FILE_NAME, FA_CREATE_ALWAYS | FA_WRITE | FA_READ );
	if ( result == FR_OK )
	{
		GLogN( "%s File Open Success\r\n", DOWNLOAD_FW_TEMP_FILE_NAME);
        result = f_truncate(&g_TempFilepnt);
		if ( result == FR_OK )
			GLogN( "%s File Truncate Success\r\n", DOWNLOAD_FW_TEMP_FILE_NAME);
	}
	else
	{
	  	f_close(&g_TempFilepnt);
		GLogE( "%s File Open fail,%d\r\n", DOWNLOAD_FW_TEMP_FILE_NAME,result);
	}

    return result;
}

U8 WriteUpdateDataTemp(U8 *buff, U16 Size)
{
	U32 i;
	UINT uiFileReadNum;
    FRESULT result = FR_OK;

	//g_u32UpdateFileSize += Size;

	//if ( g_TempFilepnt.fs != NULL )
	{
        result = f_write(&g_TempFilepnt, (void*)buff, Size, &uiFileReadNum);
		if( result != FR_OK )
		{
			// Occur to Error at 89th of Sequence number
			// have to check this Error
			GLogE("-file rewrite fail [retry cnt %d]\r\n", result);
			for ( i=0; i<3; i++ )
			{
				// retry to write file
                result = f_write(&g_TempFilepnt, (void*)buff, Size, &uiFileReadNum);
				if ( result != FR_OK )
				{
					GLogE("-file rewrite fail[retry cnt %d,%d]\r\n", i,result);
				}
				else
				{
					g_u32UpdateFileSize += Size;
					break;
				}
			}
		}
		else
		{
			g_u32UpdateFileSize += Size;
			//GLogN( "2");
		}
	}
	//else
	//	GLogE( "file not opend\r\n");

    return result;
}
#if 1
U16 CheckVCIFWCheckSum(void)
{
	FRESULT fsResult;

	U16 returnValue = 0;
	U32 i, iReadByte;
	U8 cBuff[1024] = {0, };
    U8 fBuff = 0;
	//if ( f_open(&g_TempFilepnt, DOWNLOAD_FW_TEMP_FILE_NAME, FA_WRITE | FA_READ) == FR_OK )
	{
        /*
		if ( f_lseek(&g_TempFilepnt, 0) == FR_OK )
		{
            for(i=0; i<g_u32UpdateFileSize; i =  i + 1024)
            {
                fsResult = f_read(&g_TempFilepnt, (void*)&cBuff, 1024, &iReadByte);
                if ( fsResult != FR_OK ) GLogE( "CRC Read fail\r\n");
                    
                for(int j = 0; j < iReadByte; ++j) {
                    returnValue += cBuff[j];
                }
            }

			//f_write(&g_TempFilepnt, (char*)&returnValue, sizeof(returnValue), &iReadByte); // checksum write
		}
        */

        if ( f_lseek(&g_TempFilepnt, 0) == FR_OK )
		{
			for(i=0; i<g_u32UpdateFileSize; i++)
			{
				fsResult = f_read(&g_TempFilepnt, (void*)&fBuff, 1, &iReadByte);
				if (  fsResult == FR_OK )
					returnValue += fBuff;
				else
					GLogE( "CRC Read fail\r\n");
			}

			//f_write(&g_TempFilepnt, (char*)&returnValue, sizeof(returnValue), &iReadByte); // checksum write
		}
	}

	////move to 'FL_Git_FWUpdate_End()'
	//gsFwInfo.msAppInfo[g_ucDownloadFW_FileNo].mCheckSum = returnValue;
	//gsFwInfo.msAppInfo[g_ucDownloadFW_FileNo].mSize = g_u32UpdateFileSize;
	//gsFwInfo.msAppInfo[g_ucDownloadFW_FileNo].mSectorCount = (g_u32UpdateFileSize/131072 + 1);	

	GLogN( "\r\n%s: checksum %X size %d\r\n", __FUNCTION__, returnValue, g_u32UpdateFileSize);

	return returnValue;
}
#else
U16 CheckVCIFWCheckSum(void)
  {
      FRESULT fsResult;
      U16 returnValue = 0;
      U32 i, iReadByte;
      U8 cBuff[2048];
      U32 sum = 0;

      if(f_lseek(&g_TempFilepnt, 0) == FR_OK)
      {
          for(i = 0; i < g_u32UpdateFileSize; i += 2048)
          {
              fsResult = f_read(&g_TempFilepnt, (void*)cBuff, 2048, &iReadByte);
              if(fsResult != FR_OK)
              {
                  GLogE("CRC Read fail\r\n");
                  break;
              }

              U32 j;
              U32 aligned = iReadByte & ~3U;
			  
              for(j = 0; j < aligned; j += 4)
              {
                  sum += cBuff[j];
                  sum += cBuff[j + 1];
                  sum += cBuff[j + 2];
                  sum += cBuff[j + 3];
              }
			  
              for(; j < iReadByte; j++)
              {
                  sum += cBuff[j];
              }
          }
      }

      returnValue = (U16)sum;

      GLogN("\r\n%s: checksum %X size %d\r\n", __FUNCTION__, returnValue, g_u32UpdateFileSize);

      return returnValue;
  }
#endif

U8 DownloadClose()
{
    FRESULT result = FR_OK;

	result |= f_close(&g_TempFilepnt);
#ifdef VCI3_RECOVERY
		f_unlink(g_strDownloadFW_FileName);
#else
		result |= f_unlink(g_strDownloadFW_FileName);
#endif

	result |= f_rename(DOWNLOAD_FW_TEMP_FILE_NAME, g_strDownloadFW_FileName);
	memset(&g_TempFilepnt, 0x0, sizeof(g_TempFilepnt));

	GLogN( "\r\n%s %s rename %s\r\n",
			   __FUNCTION__, DOWNLOAD_FW_TEMP_FILE_NAME, g_strDownloadFW_FileName);

	//result = f_chdir(DIR_ROOT);

	if ( result == FR_OK )
	{
		//GLogN( "Change Directory Root Ok\r\n");
	}

    return result;
}

///////////////////////////////////////////////////////////////////////////
//     GetFlashUpdate(U8 *buff, U16 Size) �׽�Ʈ �׸�  14.03.26 LWH   /////
//																		 //
// 1. vci2fw.lst �� boot�� down�� ��� ���� �ϴ� ��� -> �Ϲ����� ȯ��   //
//		1) ������ ���� ��� : OK										 //
//		2) ������ �ϳ��� �ٸ� ��� : OK									 //
//      3) ������ ��� �ٸ� ��� : OK									 //
//																		 //
// 2. vci2fw.lst ��ü�� ���� ��� -> eMMC ���˵� ȯ�� : OK				 //
//																		 //
// 3. RECOVERY �������� ������Ʈ �ϴ� ��� : OK							 //
//																		 //
// 4. vci2fw.lst �� boot�� down�� ���� ��� : OK						 //
//																		 //
// 5. vci2fw.lst �� �� �� �ϳ��� �ִ� ��� : OK							 //
//																		 //
///////////////////////////////////////////////////////////////////////////

BOOL GetFlashUpdate(U8 *buff, U16 Size)
{
  	U8 arrTemp[1000] = {0,};
	uint32_t iLength;
	char seps[] = " ,:;\t\r\n",*token ;
	int iTokenCnt = 0 ;
	U8 i=0,temp;
	int iRetry;
	TCHAR		FWPath[30]		= { "\0", };
	uint16_t	cs				= 0;
	uint32_t	size			= 0;

	if ( GetUpdateAppListFromEMMC((BYTE*)arrTemp, &iLength) == TRUE ) //�����̸��� ������ ã�Ƴ��� �迭�� ����
	{
		token = strtok((char*)arrTemp, seps);
		while ( token != NULL )
		{
			if ( iTokenCnt%3 == 0 )
			{
                temp = atoi(token);
				//Flashdata[i][0]= atoi(token);
				Flashdata[1]= ',';
				//j++;
			}
			else if ( iTokenCnt%3 == 1 )
			{
				if( temp == eApp_bootloader )	//applist�� C_ ���ξ ���Ե��� ��ȣ(10)�� �ν�
				{
                    Flashdata[0] = temp;
					strcpy(&Flashdata[2],"vci3_bootloader.bin");	//�׻� ������ ���� ���Ϸ� ���÷���
	                GLogN("\r\n V");
				}
			}
			else //MONI 20230307 [suresoft]85) : block this code for misra c if ( iTokenCnt%3 == 2 )
			{
				if(temp == Flashdata[0])   { strcpy(&Flashdata[22],token);	 break; }
			}
			token = strtok(NULL, seps);
			iTokenCnt++;
		}
	}

    if( Flashdata[0] != 10 )		//applist�� bin ������ ���� ���
    {
        Flashdata[0]=10;

        strcpy(&Flashdata[1],",vci3_bootloader.bin");
    }

	i=0;
	iTokenCnt=0;
	memcpy(arrTemp , buff, Size);

	token = strtok((char*)arrTemp, seps);
	while ( token != NULL )                 //����� �迭�� �е�κ��� ���� ������ �ٸ��� üũ
	{
		if ( iTokenCnt%3 == 0 )
		{
			temp = atoi(token);
		}
		else if ( iTokenCnt%3 == 1 )
		{
			if		( temp == eApp_bootloader ) 	i++; //10byte = 'appnumber' ',' name ',' 'ver' 'ent' (��ȣ 10���� �ν�)
		}
		else ////MONI 20230307 [suresoft]85) : block this code for misra c if ( iTokenCnt%3 == 2 )
		{
			if((i!=0)&&(temp == Flashdata[0])&& (strcmp(token,&Flashdata[22])!=0)) //������ ���� ���� �� flag ����
			{
				Flashdata[27]= 1;				// update�ؾ��� bin flag set
				//memcpy(&arrTempS[TMfwlst_length-7],&Flashdata[j][22],5);  // �ֽŹ����� ���ۿ� ����
				GLogN("diff %s ",token);
			}
		}
		token = strtok(NULL, seps);
		iTokenCnt++;
	}

	GLogN("\r\n F %02X ",Flashdata[27]);
	GLogN("\r\n A %02X ",Flashdata[26]);


	if(Flashdata[27] == 1)	//eApp_bootloader
	{
		//gsFwInfo.mucCurrentMode				= gsFwInfo.mucBootMode;
		gsFwInfo.mucBootMode = (AppName)Flashdata[0];

		for ( iRetry = 0; iRetry < 3 ; iRetry++ )
		{
			size = gsFwInfo.msAppInfo[Flashdata[0]].mSize;
			cs = gsFwInfo.msAppInfo[Flashdata[0]].mCheckSum;

			sprintf( FWPath, "%s/%s", EMMC_APPLICATION_FOLDER, &Flashdata[2]);

			if ( RestoreApplication( FWPath, size, cs ) == 0 )
			{
				GLogN("RestoreApplication Success\r\n");
				break;
			}
			else
			  return FALSE;
		}
	}
    return TRUE;
}

FRESULT SaveAppListToEMMC(U8 *buff, U16 Size)
{
  	FIL Filepnt;
	UINT uiWriteNum;
  	FRESULT result = FR_OK;

  	result = f_chdir(DIR_APP);
	if ( result == FR_OK )
	{
		GLogN( "Change Directory %s Ok\r\n", DIR_APP);
	}
	else	return FALSE;

	result |= f_unlink(FILENAME_APP_LIST_INI);
	result |= f_open(&Filepnt, FILENAME_APP_LIST_INI, FA_CREATE_NEW | FA_WRITE);
	result |= f_truncate(&Filepnt);
	result |= f_write(&Filepnt, buff, Size, &uiWriteNum);
	result |= f_close(&Filepnt);

	f_chdir(DIR_ROOT);

  	return result;
}

BOOL UpdateAppListFromRemoteDevice(U8 *buff, U16 Size)
{

	//FRESULT result = FR_OK;

	GLogI( ">>> Start %s\r\n", __FUNCTION__ );

	if( !GetFlashUpdate(buff, Size) )
	  return FALSE;

	if( SaveAppListToEMMC(buff, Size) != FR_OK )
	  return FALSE;

	UpdateFWVersion();

	return TRUE;
}

BOOL GetUpdateAppListFromEMMC(uint8_t *buff, uint32_t *Size)
{
  	FIL Filepnt;
	DWORD dwFileSize;
	uint32_t uiReadNum;
	FRESULT result = FR_OK;
	uint8_t retry;
	BOOL bChdirSuccess = FALSE;

	/* Try chdir with retry after InitEmmcFolder if failed */
	for (retry = 0U; retry < 2U; retry++)
	{
		result = f_chdir(DIR_ROOT);
		if (result != FR_OK)
		{
			if (retry == 0U)
			{
				GLogN("Root chdir failed(%d), init EMMC folders...\r\n", result);
				(void)InitEmmcFolder();    
                // FW List Init.
                InitFwListFile();
                
                // FW Info Init.
                memset( &gsFwInfo, 0x00, sizeof( SFwInfo ) );
            //  gsFwInfo.mucEmmcFormat	= TRUE;
                initFirmwareInfo();
                gsFwInfo.mucBootMode = eApp_VCI_2;
                gsFwInfo.mucChanged = true;
                // Recover Serial
                //RecoverSerial();
                
                // Save Firmware Info.
                saveFirmwareInfo_EMMC(true);
				continue;
			}
			break;
		}

		result = f_chdir(DIR_APP);
		if (result != FR_OK)
		{
			if (retry == 0U)
			{
				GLogN("APP chdir failed(%d), init EMMC folders...\r\n", result);
				(void)InitEmmcFolder();    
                // FW List Init.
                InitFwListFile();
                
                // FW Info Init.
                memset( &gsFwInfo, 0x00, sizeof( SFwInfo ) );
            //  gsFwInfo.mucEmmcFormat	= TRUE;
                initFirmwareInfo();
                gsFwInfo.mucBootMode = eApp_VCI_2;
                gsFwInfo.mucChanged = true;
                // Recover Serial
                //RecoverSerial();
                
                // Save Firmware Info.
                saveFirmwareInfo_EMMC(true);
				continue;
			}
			break;
		}

		/* Both chdir succeeded */
		GLogN("Root Directory %s Ok\r\n", DIR_ROOT);
		GLogN("Change Directory %s Ok\r\n", DIR_APP);
		bChdirSuccess = TRUE;
		break;
	}

	if (bChdirSuccess == FALSE)
	{
		return FALSE;
	}


	if ( f_open(&Filepnt, FILENAME_APP_LIST_INI, FA_OPEN_EXISTING | FA_READ) != FR_OK )
	{
		f_open(&Filepnt, FILENAME_APP_LIST_INI, FA_CREATE_NEW | FA_WRITE);
		f_write(&Filepnt, "1,C_vci3_main.bin,00.00\r\n", 			strlen("1,C_vci3_main.bin,00.00\r\n"),			&uiReadNum);
		f_write(&Filepnt, "2,C_Repro_CAN.bin,00.00\r\n",			strlen("2,C_Repro_CAN.bin,00.00\r\n"), 			&uiReadNum);
		f_write(&Filepnt, "3,C_Repro_KWP.bin,00.00\r\n",	 		strlen("3,C_Repro_KWP.bin,00.00\r\n"),			&uiReadNum);
		f_write(&Filepnt, "6,TM_Bootloader.bin,00.00\r\n", 		strlen("6,TM_Bootloader.bin,00.00\r\n"), 		&uiReadNum);
		f_write(&Filepnt, "7,TM_Downloader.bin,00.00\r\n",		strlen("7,TM_Downloader.bin,00.00\r\n"),		&uiReadNum);
		f_write(&Filepnt, "8,Trigger_Module.bin,00.00\r\n", 	strlen("8,Trigger_Module.bin,00.00\r\n"),		&uiReadNum);
		f_write(&Filepnt, "10,vci3_bootloader.bin,00.00\r\n", 	strlen("10,vci3_bootloader.bin,00.00\r\n"),		&uiReadNum);
		f_write(&Filepnt, "11,C_vci3_recovery.bin,00.00\r\n", 	strlen("11,C_vci3_recovery.bin,00.00\r\n"),		&uiReadNum);
		f_write(&Filepnt, "13,C_Repro_CCP.bin,00.00\r\n", 		strlen("13,C_Repro_CCP.bin,00.00\r\n"), 			&uiReadNum);
		f_write(&Filepnt, "16,C_Repro_DOWN.bin,00.00\r\n", 		strlen("16,C_Repro_DOWN.bin,00.00\r\n"), 			&uiReadNum);
		f_write(&Filepnt, "18,RS9116fw.bin,00.00\r\n", 			strlen("18,RS9116fw.bin,00.00\r\n"), 			&uiReadNum);
		f_write(&Filepnt, "19,C_vci3_pdi.bin,00.00\r\n", 			strlen("19,C_vci3_pdi.bin,00.00\r\n"), 			&uiReadNum);
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
		f_write(&Filepnt, "22,C_vci3_reprocommon.bin,00.01\r\n", 	strlen("22,C_vci3_reprocommon.bin,00.01\r\n"),	&uiReadNum);
		f_write(&Filepnt, "23,C_Repro_ST_CAN.bin,00.01\r\n", 		strlen("23,C_Repro_ST_CAN.bin,00.01\r\n"), 		&uiReadNum);
#endif
		f_write(&Filepnt, "FF,TOTAL_VERSION,00.00\r\n", 		strlen("FF,TOTAL_VERSION,00.00\r\n"),			&uiReadNum);
/*		f_write(&Filepnt, "0,Downloader.bin,00.00\r\n", strlen("0,Downloader.bin,00.00\r\n"), 	&uiReadNum);
		f_write(&Filepnt, "1,VCI_2.bin,00.00\r\n",		strlen("1,VCI_2.bin,00.00\r\n"), 		&uiReadNum);
		f_write(&Filepnt, "2,ECUUpCAN.bin,00.00\r\n", 	strlen("2,ECUUpCAN.bin,00.00\r\n"), 	&uiReadNum);
		f_write(&Filepnt, "3,ECUUpKWP.bin,00.00\r\n", 	strlen("3,ECUUpKWP.bin,00.00\r\n"), 	&uiReadNum);
		f_write(&Filepnt, "4,ECUUpCV.bin,00.00\r\n",	strlen("4,ECUUpCV.bin,00.00\r\n"), 		&uiReadNum);
		f_write(&Filepnt, "5,Inside.bin,00.00\r\n", 	strlen("5,Inside.bin,00.00\r\n"), 		&uiReadNum);
		f_write(&Filepnt, "FF,TOTAL_VERSION,00.00\r\n", strlen("FF,TOTAL_VERSION,00.00\r\n"), 	&uiReadNum);*/
		f_close(&Filepnt);
	}
	else
	{
		dwFileSize = f_size(&Filepnt);
		if( dwFileSize == 0)
		{
			f_close(&Filepnt);
			if ( f_open(&Filepnt, FILENAME_APP_LIST_INI, FA_WRITE) == FR_OK )
			{
				f_write(&Filepnt, "1,C_vci3_main.bin,00.00\r\n", 			strlen("1,C_vci3_main.bin,00.00\r\n"),		&uiReadNum);
				f_write(&Filepnt, "2,C_Repro_CAN.bin,00.00\r\n",			strlen("2,C_Repro_CAN.bin,00.00\r\n"), 		&uiReadNum);
				f_write(&Filepnt, "3,C_Repro_KWP.bin,00.00\r\n",	 		strlen("3,C_Repro_KWP.bin,00.00\r\n"), 		&uiReadNum);
				f_write(&Filepnt, "6,TM_Bootloader.bin,00.00\r\n", 		strlen("6,TM_Bootloader.bin,00.00\r\n"), 	&uiReadNum);
				f_write(&Filepnt, "7,TM_Downloader.bin,00.00\r\n",		strlen("7,TM_Downloader.bin,00.00\r\n"),	&uiReadNum);
				f_write(&Filepnt, "8,Trigger_Module.bin,00.00\r\n", 	strlen("8,Trigger_Module.bin,00.00\r\n"),	&uiReadNum);
				f_write(&Filepnt, "10,vci3_bootloader.bin,00.00\r\n", 	strlen("10,vci3_bootloader.bin,00.00\r\n"),	&uiReadNum);
				f_write(&Filepnt, "11,C_vci3_recovery.bin,00.00\r\n", 	strlen("11,C_vci3_recovery.bin,00.00\r\n"),	&uiReadNum);
				f_write(&Filepnt, "13,C_Repro_CCP.bin,00.00\r\n", 		strlen("13,C_Repro_CCP.bin,00.00\r\n"), 		&uiReadNum);
				f_write(&Filepnt, "16,C_Repro_DOWN.bin,00.00\r\n", 		strlen("16,C_Repro_DOWN.bin,00.00\r\n"), 		&uiReadNum);
				f_write(&Filepnt, "18,RS9116fw.bin,00.00\r\n", 			strlen("18,RS9116fw.bin,00.00\r\n"), 		&uiReadNum);
				f_write(&Filepnt, "19,C_vci3_pdi.bin,00.00\r\n", 			strlen("19,C_vci3_pdi.bin,00.00\r\n"), 		&uiReadNum);
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
		f_write(&Filepnt, "22,C_vci3_reprocommon.bin,00.01\r\n", 	strlen("22,C_vci3_reprocommon.bin,00.01\r\n"), 		&uiReadNum);
		f_write(&Filepnt, "23,C_Repro_ST_CAN.bin,00.01\r\n", 		strlen("23,C_Repro_ST_CAN.bin,00.01\r\n"), 			&uiReadNum);
#endif
				f_write(&Filepnt, "FF,TOTAL_VERSION,00.00\r\n", 		strlen("FF,TOTAL_VERSION,00.00\r\n"),	 	&uiReadNum);
			}
		}
	  	f_close(&Filepnt);
	}

	if ( f_open(&Filepnt, FILENAME_APP_LIST_INI, FA_OPEN_EXISTING | FA_READ) == FR_OK )
	{
		GLogN("%s File Open Success\r\n", FILENAME_APP_LIST_INI);
		dwFileSize = f_size(&Filepnt);

		if ( f_read(&Filepnt, buff, dwFileSize, Size) == FR_OK )
		{
			f_close(&Filepnt);
			f_chdir(DIR_ROOT);
			return TRUE;
		}
	}
	else
		GLogN("%s File Open fail\r\n", FILENAME_APP_LIST_INI);

	f_chdir(DIR_ROOT);
	return FALSE;
}

bool InitFwListFile (void)
{
    FIL Filepnt;
	uint32_t uiReadNum;
	FRESULT result = FR_OK;
    char* Dir = "/01_Application";
    
    result = f_chdir(Dir);
    if ( result == FR_OK )
	{
        GLogN( "Open Directory : %s Ok\r\n", Dir);
	}
    else	return FALSE;
    
    result = f_unlink((char const*)APPLICATION_INFO_FILE_NAME);
    if ( result == FR_OK || result == FR_NO_FILE )
    {
        GLogN( "Remove %s Ok\r\n", APPLICATION_INFO_FILE_NAME);
    }
    else    return FALSE;
    
    if( f_open(&Filepnt, FILENAME_APP_LIST_INI, FA_CREATE_NEW | FA_WRITE) == FR_OK )
    {  
        f_write(&Filepnt, "1,C_vci3_main.bin,00.00\r\n", 			strlen("1,C_vci3_main.bin,00.00\r\n"), 	&uiReadNum);
        f_write(&Filepnt, "2,C_Repro_CAN.bin,00.00\r\n",			strlen("2,C_Repro_CAN.bin,00.00\r\n"), 		&uiReadNum);
        f_write(&Filepnt, "3,C_Repro_KWP.bin,00.00\r\n",	 		strlen("3,C_Repro_KWP.bin,00.00\r\n"), 	&uiReadNum);
        f_write(&Filepnt, "6,TM_Bootloader.bin,00.00\r\n", 		strlen("6,TM_Bootloader.bin,00.00\r\n"), 	&uiReadNum);
        f_write(&Filepnt, "7,TM_Downloader.bin,00.00\r\n",		strlen("7,TM_Downloader.bin,00.00\r\n"), 		&uiReadNum);
        f_write(&Filepnt, "8,Trigger_Module.bin,00.00\r\n", 	strlen("8,Trigger_Module.bin,00.00\r\n"), 		&uiReadNum);
        f_write(&Filepnt, "10,vci3_bootloader.bin,00.00\r\n", 	strlen("10,vci3_bootloader.bin,00.00\r\n"), 		&uiReadNum);
        f_write(&Filepnt, "11,C_vci3_recovery.bin,00.00\r\n", 	strlen("11,C_vci3_recovery.bin,00.00\r\n"), 		&uiReadNum);
        f_write(&Filepnt, "13,C_Repro_CCP.bin,00.00\r\n", 		strlen("13,C_Repro_CCP.bin,00.00\r\n"), 		&uiReadNum);
        f_write(&Filepnt, "16,C_Repro_DOWN.bin,00.00\r\n", 		strlen("16,C_Repro_DOWN.bin,00.00\r\n"), 		&uiReadNum);
        f_write(&Filepnt, "18,RS9116fw.bin,00.00\r\n", 			strlen("18,RS9116fw.bin,00.00\r\n"), 		&uiReadNum);
        f_write(&Filepnt, "19,C_vci3_pdi.bin,00.00\r\n", 			strlen("19,C_vci3_pdi.bin,00.00\r\n"), 		&uiReadNum);
    #if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        f_write(&Filepnt, "22,C_vci3_reprocommon.bin,00.01\r\n", 	strlen("22,C_vci3_reprocommon.bin,00.01\r\n"), 		&uiReadNum);
        f_write(&Filepnt, "23,C_Repro_ST_CAN.bin,00.01\r\n", 		strlen("23,C_Repro_ST_CAN.bin,00.01\r\n"), 		&uiReadNum);
    #endif
        f_write(&Filepnt, "FF,TOTAL_VERSION,00.00\r\n", 		strlen("FF,TOTAL_VERSION,00.00\r\n"), 	&uiReadNum);
    /*	f_write(&Filepnt, "0,Downloader.bin,00.00\r\n", strlen("0,Downloader.bin,00.00\r\n"), 	&uiReadNum);
        f_write(&Filepnt, "1,VCI_2.bin,00.00\r\n",		strlen("1,VCI_2.bin,00.00\r\n"), 		&uiReadNum);
        f_write(&Filepnt, "2,ECUUpCAN.bin,00.00\r\n", 	strlen("2,ECUUpCAN.bin,00.00\r\n"), 	&uiReadNum);
        f_write(&Filepnt, "3,ECUUpKWP.bin,00.00\r\n", 	strlen("3,ECUUpKWP.bin,00.00\r\n"), 	&uiReadNum);
        f_write(&Filepnt, "4,ECUUpCV.bin,00.00\r\n",	strlen("4,ECUUpCV.bin,00.00\r\n"), 		&uiReadNum);
        f_write(&Filepnt, "5,Inside.bin,00.00\r\n", 	strlen("5,Inside.bin,00.00\r\n"), 		&uiReadNum);
        f_write(&Filepnt, "FF,TOTAL_VERSION,00.00\r\n", strlen("FF,TOTAL_VERSION,00.00\r\n"), 	&uiReadNum);*/
        f_close(&Filepnt);
    }
    else return FALSE;
    
    
    
    return TRUE;
}

#if 0
bool CopyFiles( U8 *strSourceFileName, BYTE SourceFileMode, U32 uilSourceSeekPoint, U8 *strTargetFileName, BYTE TargetFileMode, U32 uilTargetSeekPoint)
{
	return CopyFiletoFile( strSourceFileName, SourceFileMode, uilSourceSeekPoint, strTargetFileName, TargetFileMode, uilTargetSeekPoint);
}
#endif

#define READ_DATA_BUFFER_SIZE  4000
bool CopyFiletoFile( FIL fSourceFile, U8 *strSourceFileName, BYTE SourceFileMode, U32 uilSourceSeekPoint, FIL fTargetFile, U8 *strTargetFileName, BYTE TargetFileMode, U32 uilTargetSeekPoint)
{
	bool	bRet		= TRUE;
	U32		i			= 0;
	U32		j			= 0;
	U32		k			= 0;
	U32		uiEmmcCnt1	= 0;
	U32		uiEmmcCnt2	= 0;
	U32		uiTempSize	= 0;
	U8		ucFR_DataBuff[READ_DATA_BUFFER_SIZE];
	int		res = FR_OK;

	if( f_open( &fSourceFile, (char const*)strSourceFileName, SourceFileMode ) == FR_OK )							// TEMP_RENAME ���� �б�
	{
		if( f_open( &fTargetFile, (char const*)strTargetFileName, TargetFileMode) == FR_OK )
		{
			if( uilSourceSeekPoint < f_size( &fSourceFile ) )														//������ ���� ũ�Ⱑ ù��° ���� ������������
			{
				uiTempSize	= f_size( &fSourceFile ) - uilSourceSeekPoint;

				i	= uiTempSize / (U32)sizeof( ucFR_DataBuff );
				j	= uiTempSize % (U32)sizeof( ucFR_DataBuff );

				for( k = 0; k < i; k++ )
				{
					if( f_lseek( &fSourceFile, uilSourceSeekPoint + ( k * READ_DATA_BUFFER_SIZE ) ) == FR_OK )    //�� ����
					{
						if( f_read( &fSourceFile, (void*)ucFR_DataBuff, sizeof(ucFR_DataBuff), &uiEmmcCnt1 ) == FR_OK )
						{
							if( f_lseek( &fTargetFile, uilTargetSeekPoint + ( k * READ_DATA_BUFFER_SIZE ) ) == FR_OK )
							{
								if( f_write( &fTargetFile, (void*)ucFR_DataBuff, (int)sizeof(ucFR_DataBuff), &uiEmmcCnt2) != FR_OK )
								{
									GLogE( "\r\n Write FAIL %d",res);
									bRet = FALSE;
									break;
								}
							}
							else
							{
								GLogE( "\r\n TargetSeek FAIL %d",res);
								bRet = FALSE;
								break;
							}
						}
						else
						{
							GLogE( "\r\n Read FAIL %d",res);
							bRet = FALSE;
							break;
						}
					}
					else
					{
						GLogE( "\r\n SourceSeek FAIL %d",res);
						bRet = FALSE;
						break;
					}
				}

				if( f_lseek( &fSourceFile, uilSourceSeekPoint + ( k * READ_DATA_BUFFER_SIZE ) ) == FR_OK )        //������ ����
				{
					if( f_read( &fSourceFile, (void*)ucFR_DataBuff, j, &uiEmmcCnt1 ) == FR_OK )
					{
						if( f_lseek( &fTargetFile, uilTargetSeekPoint + ( k * READ_DATA_BUFFER_SIZE ) ) == FR_OK )
						{
							if( f_write( &fTargetFile, (void*)ucFR_DataBuff, j, &uiEmmcCnt2 ) != FR_OK )
							{
								GLogE( "\r\n Write Fail..%d",res);
								bRet = FALSE;
							}
						}
						else
						{
							GLogE( "\r\n TargetSeek FAIL %d",res);
							bRet = FALSE;
						}
					}
					else
					{
						GLogE( "\r\n Read FAIL %d",res);
						bRet = FALSE;
					}
				}
				else
				{
					GLogE( "\r\n Seek FAIL %d",res);
					bRet = FALSE;
				}

				if( f_close( &fSourceFile ) != FR_OK )
				{
					GLogE( "\r\n Source File Close FAIL!%d", res);
					bRet = FALSE;
				}

				if( f_close( &fTargetFile ) != FR_OK )
				{
					GLogE( "\r\n Target File Close FAIL!%d", res);
					bRet = FALSE;
				}
			}
			else
			{
				if( f_close( &fSourceFile ) != FR_OK )
				{
					GLogE( "\r\n Source File Close FAIL!%d", res);
					bRet = FALSE;
				}

				if( f_close( &fTargetFile ) != FR_OK )
				{
					GLogE( "\r\n Target File Close FAIL!%d", res);
					bRet = FALSE;
				}
			}
		}
		else
		{
			f_close( &fSourceFile );
			f_close( &fTargetFile );
			GLogE( "\r\n FNAME.REC FILE OPEN FAIL 9:%d!! ",res);
			return FALSE;
		}
	}
	else
	{
	  	f_close( &fSourceFile );
		GLogE( "\r\n TEMP.REC FILE OPEN FAIL 10:%d!! ",res);
		return FALSE;
	}

	return bRet;
}

U8 StringCompareAce89(U8 *From, U8 *Dest, U16 Length)
{
	U16 i;
	U8 IsSame = 1;
	for(i=0; i<Length; i++)
	{
		if (From[i] != Dest[i])
		{
			IsSame = 0;
			break;
		}
	}
	return IsSame;
}

int8_t HSM_File_SizeInfo( int *FileSize )
{
	DIR Dir;
	FRESULT res;
	FILINFO Finfo;
	//long p1;
	//FATFS *fs;
	//char* strScanDirectory = "DIR_APP";
	U8 ReturnValue = 0;

	memset(&Finfo, NULL, sizeof(Finfo));
    if((res = f_chdir("/")) == FR_OK)
    {
        if(res = f_opendir(&Dir, "01_Application")== FR_OK)
        {
            res = f_chdir("01_Application");
        }
    }

	if (res) { put_rc((FRESULT)res); return ReturnValue; }

	for(;;)
	{
		res = f_readdir(&Dir, &Finfo);
		if((res == FR_OK) && (strncmp(Finfo.fname, (char const*)gHSM_Info_Name, 18) == 0))
		{
			*FileSize = Finfo.fsize;
			ReturnValue = 1;
		}
		else if ((res != FR_OK) || !Finfo.fname[0])
        {
        	if(res != FR_OK)
        	{
            	GLogE("\r\nFailed to read HSM file\r\n");
        	}
			break;
        }
	}

	/*if (f_getfree(strScanDirectory, (DWORD*)&p1, &fs) == FR_OK)
		GLogN( "FREE (01_Application)" );*/

	return ReturnValue;

}

int8_t HSM_File_VersionInfo( void )
{
	DIR Dir;
	FIL Filepnt;
	FRESULT res;
	FILINFO Finfo;
	//FATFS *fs;
	//long p1;
	char* strScanDirectory = "/01_Application";//DIR_APP;
	char ReadBuff[500] = {0x00, };
	U32 readsize = 0;

	U8* HSM_Info_Addr;
	U8 HSM_Read_ver[5] = {0x00, };

	memset(&Finfo, NULL, sizeof(Finfo));

	if((res = f_opendir(&Dir, strScanDirectory)) == FR_OK)
	{
    	if((res = f_readdir(&Dir, &Finfo)) == FR_OK)
    	{
			if((res = f_chdir(strScanDirectory)) == FR_OK)
			{
				GLogN("\r\n--------------- PATH OPEN SUCCESS ---------------\r\n");

				if ( (res = f_open(&Filepnt, FILENAME_APP_LIST_INI, FA_OPEN_EXISTING | FA_READ)) == FR_OK )
				{
					if((res = f_read(&Filepnt, ReadBuff, Finfo.fsize, &readsize)) == FR_OK)
			        {
			        	HSM_Info_Addr = strstr(ReadBuff, (char const*)gHSM_Info_Name);
                        if(HSM_Info_Addr == NULL)
                        {
                            GLogE("There is no HSM Version Info");
                            return 0;
                        }
                        else
                        {
                            HSM_Info_Addr += sizeof(gHSM_Info_Name) + 1;

                            memcpy(HSM_Read_ver, HSM_Info_Addr, 5);

                            f_close(&Filepnt);
                            f_closedir( &Dir );
                            return ((HSM_Read_ver[3]-0x30)*10) + (HSM_Read_ver[4]-0x30);
                        }
			        }
				}
				else
				{
					GLogN("\r\n----------------- FILE OPEN FAIL ----------------\r\n");
				}
				f_close(&Filepnt);
			}
			//else
			{
				GLogN("\r\n----------------- PATH OPEN FAIL ----------------\r\n");
			}
    	}
		f_closedir( &Dir );
	}

	/*if (f_getfree(strScanDirectory, (DWORD*)&p1, &fs) == FR_OK)
		GLogN( "FREE (01_Application)" );*/

	if (res) { put_rc((FRESULT)res); return NULL; }
    
    return res;
}

int8_t Send_HSM_File( U8 *Data_Addr, int Count, int length )
{
	//DIR Dir;
	FIL Filepnt;
	FRESULT res;
	FILINFO Finfo;
	//FATFS *fs;
	//char* strScanDirectory = DIR_APP;
	U32 readsize = 0;

	memset(&Finfo, NULL, sizeof(Finfo));

	if ( (res = f_open(&Filepnt, (char const*)gHSM_Info_Name, FA_OPEN_EXISTING | FA_READ)) == FR_OK )
	{
		if( (res = f_lseek( &Filepnt, Count*length )) == FR_OK )
		{
			if((res = f_read(&Filepnt, Data_Addr, length, &readsize)) == FR_OK)
			{
				f_close(&Filepnt);
                return 0;
			}
		}
	}
	else
	{
		GLogN("\r\n----------------- FILE OPEN FAIL ----------------\r\n");
	  	f_close(&Filepnt);
	}

	if (res) { put_rc((FRESULT)res); return 1; }
    
    return res;
}

FRESULT StoreCrlEmmc(U8 crl_no, U8 *buff, U16 Size)
{
  	FIL Filepnt;
	UINT uiWriteNum;
  	FRESULT result = FR_OK;
	char crl_filename[32];

	sprintf(crl_filename, "crl_%d.bin", crl_no);

	f_chdir(DIR_ROOT);
  	result = f_chdir(DIR_APP);
	if ( result == FR_OK )
	{
		GLogN( "Change Directory %s Ok\r\n", DIR_APP);
	}
	else
	{
		f_chdir(DIR_ROOT);
		return FALSE;
	}

	result |= f_open(&Filepnt, crl_filename, FA_CREATE_ALWAYS | FA_WRITE);
	result |= f_truncate(&Filepnt);
	result |= f_write(&Filepnt, buff, Size, &uiWriteNum);
	result |= f_close(&Filepnt);

	f_chdir(DIR_ROOT);

  	return result;
}

BOOL GetCrlEmmc(U8 crl_no, U8* InBuff, U32 *iBuffLength)
{
	char crl_filename[32];
	sprintf(crl_filename, "crl_%d.bin", crl_no);
	return GetCrlFileRead(crl_filename, InBuff, iBuffLength);
}

#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
FRESULT StoreRdbiEmmc(U8 *buff, U16 Size)
{
  	FIL Filepnt;
	UINT uiWriteNum;
  	FRESULT result = FR_OK;

	result = f_chdir(DIR_ROOT);
  	result = f_chdir(DIR_ECU_TO_UPGRADE_DEPTH1);
    result = f_chdir(DIR_ECU_TO_UPGRADE_DEPTH2);
	if ( result == FR_OK )
	{
		GLogN( "Change Directory /Crux/S_ETC Ok\r\n");
	}
	else
	{	
		f_chdir(DIR_ROOT);
		return FALSE;
	}

	result |= f_unlink(FILE_RDBI);
	result |= f_open(&Filepnt, FILE_RDBI, FA_CREATE_NEW | FA_WRITE);
	result |= f_truncate(&Filepnt);
	result |= f_write(&Filepnt, buff, Size, &uiWriteNum);
	result |= f_close(&Filepnt);

	f_chdir(DIR_ROOT);

  	return result;
}
#endif

FRESULT StoreSTDA_ReproEmmc(U8 *buff, U16 Size)
{
	FIL Filepnt;
	UINT uiWriteNum;
	FRESULT result = FR_OK;

	result = f_chdir(DIR_ROOT);
	//result = f_chdir("/30_S_Repro");
	if ( result == FR_OK )
	{
		GLogN( "Change Directory Root Ok\r\n");
	}
	else
	{	
		f_chdir(DIR_ROOT);
		//return FALSE;
	}

	result |= f_unlink(S_REPRO_JUMP_FILE_NAME);
	result |= f_open(&Filepnt, S_REPRO_JUMP_FILE_NAME, FA_CREATE_NEW | FA_WRITE);
	result |= f_truncate(&Filepnt);
	result |= f_write(&Filepnt, buff, Size, &uiWriteNum);
	result |= f_close(&Filepnt);

	f_chdir(DIR_ROOT);

	return result;
}
FRESULT StoreLstEmmc(U8 *buff, U16 Size)
{
	FIL Filepnt;
	UINT uiWriteNum;
	FRESULT result = FR_OK;

	result = f_chdir(DIR_ROOT);
	result = f_chdir(DIR_APP);
	if ( result == FR_OK )
	{
		GLogN( "Change Directory Root Ok\r\n");
	}
	else
	{	
		f_chdir(DIR_ROOT);
		return FALSE;
	}

	result |= f_unlink("AppSwList.ini");
	result |= f_open(&Filepnt, "AppSwList.ini", FA_CREATE_NEW | FA_WRITE);
	result |= f_truncate(&Filepnt);
	result |= f_write(&Filepnt, buff, Size, &uiWriteNum);
	result |= f_close(&Filepnt);

	f_chdir(DIR_ROOT);

	return result;
}

BOOL GetCrlFileRead(char* strOpenFileName, U8* InBuff, U32 *iBuffLength)
{
	DWORD dwFileSize;
	FIL fpVer;
	FRESULT result = FR_OK;

	GLogI( ">>> Start %s\r\n", __FUNCTION__ );

	result = f_chdir(DIR_ROOT);
	result = f_chdir(DIR_APP);
	if ( result == FR_OK )
	{
		GLogN( "Change Directory %s Ok\r\n", DIR_APP);
	}
	if ( f_open(&fpVer, strOpenFileName, FA_OPEN_EXISTING | FA_READ) == FR_OK )
	{
		//UartPrintf(&U1,"%s File Open Success\r\n", strOpenFileName);
		dwFileSize = f_size(&fpVer);

		if ( f_read(&fpVer, InBuff, dwFileSize, iBuffLength) == FR_OK )
		{
			f_close(&fpVer);
			f_chdir(DIR_ROOT);
			return TRUE;
		}
	}
	else
		GLogE( "%s File Open fail\r\n", strOpenFileName);

	f_chdir(DIR_ROOT);

	return FALSE;
}

U8 CRT_CRL_Check(U8 *Data, U8 CertNum)
{
    DIR Dir;
    FRESULT res;
    FILINFO Finfo;
    char strScanDirectory[] = "/Crux/ETC";
    uint8_t size = 0;
    char strfilename[20];
    char strfullpath[50];
    char ReadBuff[10];
    
    memset(&Finfo, 0, sizeof(Finfo));
    memset(strfilename, 0, sizeof(strfilename));
    memset(strfullpath, 0, sizeof(strfullpath));
    memset(ReadBuff, 0, sizeof(ReadBuff));
    
    if( CertNum == 0x01/*PA_SHA1*/ || CertNum == 0 ) memcpy(strfilename, CERT_SHA1_FILENAME, strlen(CERT_SHA1_FILENAME));
    else if( CertNum == 0x02/*PA_SHA2*/ ) memcpy(strfilename, CERT_SHA2_FILENAME, strlen(CERT_SHA2_FILENAME));
    else if( CertNum == 0x03/*CO_SHA1*/ ) {}
    else if( CertNum == 0x04/*CO_SHA2*/ ) {}
    else return 0;

    res = f_opendir(&Dir, strScanDirectory);
    if (res != FR_OK) 
    {
        put_rc((FRESULT)res);
        return 0;
    }

    while (1) 
    {
        res = f_readdir(&Dir, &Finfo);
        if (res != FR_OK || !Finfo.fname[0]) 
        {
            break;
        }

        if (!(Finfo.fattrib & AM_DIR) && strncmp(Finfo.fname, strfilename, strlen(strfilename)) == 0) 
        {
            FIL temp;
            
            strcpy(strfullpath, strScanDirectory);
            strcat(strfullpath, "/");
            strcat(strfullpath, strfilename);
            
            if (f_open(&temp, strfullpath, FA_OPEN_EXISTING | FA_READ) == FR_OK) 
            {
                if (f_lseek(&temp, 15) == FR_OK && f_read(&temp, ReadBuff, DATE_LENGTH, NULL) == FR_OK) 
                {
                    memcpy(&Data[size], ReadBuff, DATE_LENGTH);
                    size += DATE_LENGTH;
                }

                if (f_lseek(&temp, 689) == FR_OK && Finfo.fsize > CERT_MAX_FILE_SIZE) 
                {
                    if (f_read(&temp, ReadBuff, DATE_LENGTH, NULL) == FR_OK) 
                    {
                        memcpy(&Data[size], ReadBuff, DATE_LENGTH);
                        size += DATE_LENGTH;
                    }
                }
                f_close(&temp);
                break;
            }
        }
    }

    return size;
}

FRESULT FileCount(U8 *ucPath, U16 usPathLen, U16 *usFileCount, U16 *usFolderCount, U16 *usPacketCount)
{
    DIR Dir;
    FRESULT res;
    FILINFO Finfo;
    char *strScanDirectory = (char *)malloc(usPathLen + 1);
    U32 uiSizeCheck = 4; //"uiSizeCheck" starts at 4 (Result(1) + SID(1) + Sequence(2))

    if (strScanDirectory == NULL) 
    {
        return FR_NOT_ENOUGH_CORE; // Not Enough Memory
    }

    memset(strScanDirectory, 0, usPathLen + 1);
    memcpy(strScanDirectory, ucPath, usPathLen); // Copy Path

    *usPacketCount = 1; // "usPacketCount" starts at 1
    *usFolderCount = 0; // initializing usFolderCount
    *usFileCount = 0; // initializing usFileCount

    res = f_opendir(&Dir, strScanDirectory);
    if (res != FR_OK) 
    {
        free(strScanDirectory);
        return res;
    }

    while (1) 
    {
        res = f_readdir(&Dir, &Finfo);
        if (res != FR_OK || !Finfo.fname[0]) 
        {
            break;
        }

        if (Finfo.fattrib & AM_DIR) 
        {
            (*usFolderCount)++; // Folder Count
        } 
        else 
        {
            (*usFileCount)++; // File Count
        }
        
        uiSizeCheck += (strlen(Finfo.fname) + 8);
        if (uiSizeCheck > 4000) 
        {
            (*usPacketCount)++;
            uiSizeCheck = 0;
        }
    }

    free(strScanDirectory);
    return res;
}

FRESULT FileParsing(U8 *ucPath, U16 usPathLen, U16 Sequence, U8 *ucData, U32 *uiDataLen)
{
    DIR Dir;
    FRESULT res;
    FILINFO Finfo;
    char *strScanDirectory = (char *)malloc(usPathLen + 1);
    U16 uiPacketCount = 1; //"uiPacketCount" starts at 1
    U32 uiSizeCheck = 4; //"uiSizeCheck" starts at 4 (Result(1) + SID(1) + Sequence(2))
    U16 uiOrder = 0;
    U16 uiFileCount = 0;
    U16 uiFolderCount = 0;

    if (strScanDirectory == NULL) 
    {
        return FR_NOT_ENOUGH_CORE; // Not Enough Memory
    }

    memset(strScanDirectory, 0, usPathLen + 1);
    memcpy(strScanDirectory, ucPath, usPathLen); // Copy Path
    
    res = f_opendir(&Dir, strScanDirectory);
    if (res != FR_OK) 
    {
        free(strScanDirectory);
        return res;
    }

    while (1) 
    {
        res = f_readdir(&Dir, &Finfo);
        if (res != FR_OK || !Finfo.fname[0]) 
        {
            *uiDataLen = uiSizeCheck;
            break;
        }

        if ((uiSizeCheck + strlen(Finfo.fname)) > 4000) 
        {
            *uiDataLen = uiSizeCheck;
            
            uiPacketCount++; // Check the number of packets
            uiSizeCheck = 4; // initializing uiSizeCheck
            uiFolderCount = 0; // initializing uiFolderCount
            uiFileCount = 0; // initializing uiFileCount
        }
        
        if (Finfo.fattrib & AM_DIR) 
        {
            uiFolderCount++; // Folder Count
        } 
        else 
        {
            uiFileCount++; // File Count
        }

        if (uiPacketCount == Sequence) 
        {
            uiOrder++; // Order Count

            little_to_big_endian(&ucData[0], uiPacketCount, sizeof(U16)); // Sequence
            little_to_big_endian(&ucData[2], uiFolderCount + uiFileCount, sizeof(U16)); // Total Folders & Files
            little_to_big_endian(&ucData[uiSizeCheck], uiOrder, sizeof(U16)); // Order
            ucData[uiSizeCheck + 2] = (Finfo.fattrib & AM_DIR) ? 0x01 : 0x00; // File or Folder
            ucData[uiSizeCheck + 3] = strlen(Finfo.fname); // Filename Length
            little_to_big_endian(&ucData[uiSizeCheck + 4], Finfo.fsize, sizeof(int)); // File Size
            memcpy(&ucData[uiSizeCheck + 8], Finfo.fname, strlen(Finfo.fname)); // File Name
        }
        else if(uiPacketCount > Sequence) break;

        uiSizeCheck += (strlen(Finfo.fname) + 8); // FileName + Order(2) + File/Folder Info(1) + FileNameSize(1) + Filesize(4)
    }
    
    free(strScanDirectory);
    return res;
}

FRESULT FileChecksumCheck(U8 *ucPath, U16 usPathLen, U8 *ucFilename, U16 ucFilenameLen, U32 *uiChecksum)
{
    DIR Dir;
    FIL File;
    FRESULT res;
    FILINFO Finfo;
    char *strScanDirectory = (char *)malloc(usPathLen + 1 + ucFilenameLen + 1);
    U8 ucDataBuf = 0;
    U32 uiReadSize = 0;
    
    if (strScanDirectory == NULL) 
    {
        free(strScanDirectory);
        return FR_NOT_ENOUGH_CORE; // Not Enough Memory
    }
    
    (*uiChecksum) = 0;
    memset(strScanDirectory, 0, sizeof(strScanDirectory));
    memset(&Finfo, 0, sizeof(Finfo));

    memcpy(strScanDirectory, ucPath, usPathLen);
    strScanDirectory[usPathLen] = NULL;

    res = f_opendir(&Dir, strScanDirectory);
    if (res != FR_OK) 
    {
        put_rc((FRESULT)res);
        free(strScanDirectory);
        return res;
    }

    while (1) 
    {
        res = f_readdir(&Dir, &Finfo);
        if (res != FR_OK || !Finfo.fname[0]) 
        {
            break;
        }

        if (!strcmp(Finfo.fname, (char*)ucFilename)) 
        {
            strncpy(&strScanDirectory[usPathLen], "/", 1);
            memcpy(&strScanDirectory[usPathLen+1], ucFilename, ucFilenameLen);
            strScanDirectory[usPathLen+1+ucFilenameLen] = 0;
            
            res = f_open(&File, strScanDirectory, FA_OPEN_EXISTING | FA_READ);
            if (res == FR_OK) 
            {
                for (int i = 0; i < Finfo.fsize; i++) 
                {
                    res = f_read(&File, &ucDataBuf, 1, &uiReadSize);
                    if (res == FR_OK && uiReadSize == 1) 
                    {
                        *uiChecksum += ucDataBuf; // Calculate CS
                    } 
                    else 
                    {
                        f_close(&File);
                        free(strScanDirectory);
                        return res;
                    }
                }
                f_close(&File);
            }
            break;
        } 
        else 
        {
            memset(&Finfo, 0, sizeof(Finfo));
        }
    }

    free(strScanDirectory);
    return res;
}

FRESULT FileCRC32Check(U8 *ucPath, U16 usPathLen, U8 *ucFilename, U16 ucFilenameLen, U32 *uiChecksum)
{
    DIR Dir;
    FIL File;
    FRESULT res;
    FILINFO Finfo;
    char *strScanDirectory = (char *)malloc(usPathLen + 1 + ucFilenameLen + 1);
    U8 ucDataBuf[BULKSIZE] = {0x00, };
    UINT uiReadSize = 0;
    U32 uiBulkNum = 0;
    U32 uiCalcSize = 0;
    
    g_CRCval = 0xFFFFFFFF; //Initialize CRC Value
    
    if (strScanDirectory == NULL) 
    {
        free(strScanDirectory);
        return FR_NOT_ENOUGH_CORE; // Not Enough Memory
    }
    
    (*uiChecksum) = 0;
    memset(strScanDirectory, 0, sizeof(strScanDirectory));
    memset(&Finfo, 0, sizeof(Finfo));

    memcpy(strScanDirectory, ucPath, usPathLen);
    strScanDirectory[usPathLen] = NULL;

    res = f_opendir(&Dir, strScanDirectory);
    if (res != FR_OK) 
    {
        put_rc((FRESULT)res);
        free(strScanDirectory);
        return res;
    }

    while (1) 
    {
        res = f_readdir(&Dir, &Finfo);
        if (res != FR_OK || !Finfo.fname[0]) 
        {
            break;
        }

        if (!strcmp(Finfo.fname, (char*)ucFilename)) //Find File
        {
            //Setting the File Path
            strncpy(&strScanDirectory[usPathLen], "/", 1);
            memcpy(&strScanDirectory[usPathLen+1], ucFilename, ucFilenameLen);
            strScanDirectory[usPathLen+1+ucFilenameLen] = 0;
            
            res = f_open(&File, strScanDirectory, FA_OPEN_EXISTING | FA_READ);
            if (res == FR_OK) 
            {
                if((Finfo.fsize%BULKSIZE) == 0)
                {
                    uiBulkNum = Finfo.fsize/BULKSIZE;
                }
                else
                {
                    uiBulkNum = (Finfo.fsize/BULKSIZE)+1;
                }
                
                for (int i = 0; i < uiBulkNum; i++) 
                {
                    //Setting the size to read
                    if(i!=(uiBulkNum-1)) 
                    {
                        uiCalcSize = BULKSIZE;
                    }
                    else
                    {
                        uiCalcSize = Finfo.fsize%BULKSIZE;
                    }
                    
                    res = f_read(&File, &ucDataBuf, uiCalcSize, &uiReadSize);
                    if (res == FR_OK) 
                    {
                        vcicrc32(ucDataBuf, uiReadSize);
                    } 
                    else 
                    {
                        f_close(&File);
                        free(strScanDirectory);
                        return res;
                    }
                }
                f_close(&File);
                
                *uiChecksum = ~g_CRCval;
            }
            break;
        } 
        else 
        {
            memset(&Finfo, 0, sizeof(Finfo));
        }
    }

    free(strScanDirectory);
    return res;
}

#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
BOOL LoadRunRepro_Info()
{
    BOOL bRet = FALSE;
    DIR Dir;
    FIL fpVer;
    FRESULT fResult;
    unsigned int nReadenLength;
    unsigned short int usECUCnt;
    
    f_chdir(DIR_ROOT);
    // Check folder
    fResult = f_opendir(&Dir, DIR_ECU_TO_UPGRADE_DEPTH1);
    if ( fResult != FR_OK ) f_mkdir(DIR_ECU_TO_UPGRADE_DEPTH1);
    f_chdir(DIR_ECU_TO_UPGRADE_DEPTH1);
    
    fResult = f_opendir(&Dir, DIR_ECU_TO_UPGRADE_DEPTH2);
    if ( fResult != FR_OK ) f_mkdir(DIR_ECU_TO_UPGRADE_DEPTH2);
    f_chdir(DIR_ECU_TO_UPGRADE_DEPTH2);

    // Load file 
    fResult = f_open(&fpVer, FILE_RUNREPRO, FA_OPEN_EXISTING|FA_READ);
    
    if ( fResult == FR_OK )
    {
        fResult = f_read(&fpVer, &usECUCnt, sizeof(usECUCnt), &nReadenLength);

        if ( fResult == FR_OK && usECUCnt > 0 )
        {
            g_pstRunReproInfo = pvPortMalloc(sizeof(stRunRepro)*usECUCnt);
        
            if ( g_pstRunReproInfo != NULL )
            {
                for ( int i=0; i<usECUCnt; i++ )
                {
                    g_pstRunReproInfo[i].unECUCount = usECUCnt;
                    
                    // read Rom file name // osPoolCAlloc
                    f_read(&fpVer, &g_pstRunReproInfo[i].ucRomBinFileNameSize, sizeof(g_pstRunReproInfo[i].ucRomBinFileNameSize), &nReadenLength);
                    g_pstRunReproInfo[i].pRomBinFileName = pvPortMalloc(g_pstRunReproInfo[i].ucRomBinFileNameSize);
                    f_read(&fpVer, g_pstRunReproInfo[i].pRomBinFileName, g_pstRunReproInfo[i].ucRomBinFileNameSize, &nReadenLength);
                    
                    // read RuleDB file name 
                    f_read(&fpVer, &g_pstRunReproInfo[i].ucRuleDBFileNameSize, sizeof(g_pstRunReproInfo[i].ucRuleDBFileNameSize), &nReadenLength);
                    g_pstRunReproInfo[i].pRuleDBFileName = pvPortMalloc(g_pstRunReproInfo[i].ucRuleDBFileNameSize);
                    f_read(&fpVer, g_pstRunReproInfo[i].pRuleDBFileName, g_pstRunReproInfo[i].ucRuleDBFileNameSize, &nReadenLength);
                    
                    f_read(&fpVer, &g_pstRunReproInfo[i].ucRetryCount, sizeof(g_pstRunReproInfo[i].ucRetryCount), &nReadenLength);
                    
                    GLogN( "%s : RUNREPRO.INI ECU Idx %d, Retry Cnt %d, \r\n", __FUNCTION__, i, g_pstRunReproInfo[i].ucRetryCount);
                }
                f_read(&fpVer, &g_pstRunReproInfo[0].arrVinCode, SIZE_VIN_NUM, &nReadenLength);

                bRet = TRUE;
            }
        }

        f_close(&fpVer);
    }
    else
        GLogN( "There is no %s file\r\n", FILE_RUNREPRO);

    f_chdir(DIR_ROOT);
    
    return bRet;
}

BOOL LoadRDBI_Info()
{
    // ��õ� ī���͸� üũ�Ͽ�, ���� �����ϰų� ecu���׷��̵�� ���� 
    BOOL bRet = FALSE;

    DIR Dir;
    FIL fpVer;
    FRESULT fResult;
    unsigned int nReadenLength;
    unsigned short int usECUCnt;

    f_chdir(DIR_ROOT);

    // Check folder
    
    fResult = f_opendir(&Dir, DIR_ECU_TO_UPGRADE_DEPTH1);
    if ( fResult != FR_OK ) f_mkdir(DIR_ECU_TO_UPGRADE_DEPTH1);
    f_chdir(DIR_ECU_TO_UPGRADE_DEPTH1);

    fResult = f_opendir(&Dir, DIR_ECU_TO_UPGRADE_DEPTH2);
    if ( fResult != FR_OK ) f_mkdir(DIR_ECU_TO_UPGRADE_DEPTH2);
    f_chdir(DIR_ECU_TO_UPGRADE_DEPTH2);


    fResult = f_open(&fpVer, FILE_RDBI, FA_READ);
    
    if ( fResult == FR_OK )
    {
        //unsigned char *pucDest, *pucSrc;
        //unsigned int unTmp;
        
        fResult = f_read(&fpVer, &usECUCnt, sizeof(usECUCnt), &nReadenLength);

        if ( fResult == FR_OK && usECUCnt > 0 )
        {
            g_pstRDBIInfo = pvPortMalloc(sizeof(stRDBIInfo)*usECUCnt);
            
            if ( g_pstRDBIInfo != NULL )
            {
                for ( int i=0; i<usECUCnt; i++ )
                {
                    g_pstRDBIInfo[i].unECUCount = usECUCnt;

                     // read RuleDB file name 
                    f_read(&fpVer, &g_pstRDBIInfo[i].ucRuleDBFileNameSize, sizeof(g_pstRDBIInfo[i].ucRuleDBFileNameSize), &nReadenLength);
                    g_pstRDBIInfo[i].pRuleDBFileName = pvPortMalloc(g_pstRDBIInfo[i].ucRuleDBFileNameSize);
                    f_read(&fpVer, g_pstRDBIInfo[i].pRuleDBFileName, g_pstRDBIInfo[i].ucRuleDBFileNameSize, &nReadenLength);

                    // read Rom file name 
                    f_read(&fpVer, &g_pstRDBIInfo[i].ucRomBinFileNameSize, sizeof(g_pstRDBIInfo[i].ucRomBinFileNameSize), &nReadenLength);
                    g_pstRDBIInfo[i].pRomBinFileName = pvPortMalloc(g_pstRDBIInfo[i].ucRomBinFileNameSize);
                    f_read(&fpVer, g_pstRDBIInfo[i].pRomBinFileName, g_pstRDBIInfo[i].ucRomBinFileNameSize, &nReadenLength);

                    // part Lid
                    f_read(&fpVer, &g_pstRDBIInfo[i].ucPartLidSize, sizeof(g_pstRDBIInfo[i].ucPartLidSize), &nReadenLength);
                    g_pstRDBIInfo[i].pPartLid = pvPortMalloc(g_pstRDBIInfo[i].ucPartLidSize+32);
                    f_read(&fpVer, g_pstRDBIInfo[i].pPartLid, g_pstRDBIInfo[i].ucPartLidSize, &nReadenLength);

                    // PartDty
                    f_read(&fpVer, &g_pstRDBIInfo[i].ucPartDty, sizeof(g_pstRDBIInfo[i].ucPartDty), &nReadenLength);

                    // PartByteSize
                    f_read(&fpVer, &g_pstRDBIInfo[i].ucPartByteSize, sizeof(g_pstRDBIInfo[i].ucPartByteSize), &nReadenLength);
                    g_pstRDBIInfo[i].pPartByte = pvPortMalloc(g_pstRDBIInfo[i].ucPartByteSize);
                    f_read(&fpVer, g_pstRDBIInfo[i].pPartByte, g_pstRDBIInfo[i].ucPartByteSize, &nReadenLength);

                    // PartNoCnt & PartNo
                    f_read(&fpVer, &g_pstRDBIInfo[i].ucPartNoCnt, sizeof(g_pstRDBIInfo[i].ucPartNoCnt), &nReadenLength);
                    g_pstRDBIInfo[i].parrPartNo = pvPortMalloc(g_pstRDBIInfo[i].ucPartNoCnt);
                    for ( int j=0; j<g_pstRDBIInfo[i].ucPartNoCnt; j++ )
                    {
                        g_pstRDBIInfo[i].parrPartNo[j] = pvPortMalloc(g_pstRDBIInfo[i].ucPartByteSize);    
                        f_read(&fpVer, g_pstRDBIInfo[i].parrPartNo[j], g_pstRDBIInfo[i].ucPartByteSize, &nReadenLength);
                    }
                    
                    // SwVerLidSize
                    f_read(&fpVer, &g_pstRDBIInfo[i].ucSwVerLidSize, sizeof(g_pstRDBIInfo[i].ucSwVerLidSize), &nReadenLength);
                    
                    // SwVerLid
                    g_pstRDBIInfo[i].pSwVerLid = pvPortMalloc(g_pstRDBIInfo[i].ucSwVerLidSize);
                    f_read(&fpVer, g_pstRDBIInfo[i].pSwVerLid, g_pstRDBIInfo[i].ucSwVerLidSize, &nReadenLength);

                    // SwVerDty
                    f_read(&fpVer, &g_pstRDBIInfo[i].ucSwVerDty, sizeof(g_pstRDBIInfo[i].ucSwVerDty), &nReadenLength);

                    // SwVerByteSize
                    f_read(&fpVer, &g_pstRDBIInfo[i].ucSwVerByteSize, sizeof(g_pstRDBIInfo[i].ucSwVerByteSize), &nReadenLength);

                    // SwVerByte
                    g_pstRDBIInfo[i].pSwVerByte = pvPortMalloc(g_pstRDBIInfo[i].ucSwVerByteSize);
                    f_read(&fpVer, g_pstRDBIInfo[i].pSwVerByte, g_pstRDBIInfo[i].ucSwVerByteSize, &nReadenLength);
   
                    // SwVer
                    g_pstRDBIInfo[i].pSwVer = pvPortMalloc(g_pstRDBIInfo[i].ucSwVerByteSize);
                    f_read(&fpVer, g_pstRDBIInfo[i].pSwVer, g_pstRDBIInfo[i].ucSwVerByteSize, &nReadenLength);
/*
                    // CAN TX ID
                    pucSrc = (unsigned char*)&unTmp;
                    pucDest = (unsigned char*)&g_pstRDBIInfo[i].unCanTxID;
                    f_read(&fpVer, &unTmp, sizeof(g_pstRDBIInfo[i].unCanTxID), &nReadenLength);
                    pucDest[0] = pucSrc[3];pucDest[1] = pucSrc[2];pucDest[2] = pucSrc[1];pucDest[3] = pucSrc[0]; 
                    
                    pucDest = (unsigned char*)&g_pstRDBIInfo[i].unCanRxID;
                    f_read(&fpVer, &unTmp, sizeof(g_pstRDBIInfo[i].unCanRxID), &nReadenLength);
                    pucDest[0] = pucSrc[3];pucDest[1] = pucSrc[2];pucDest[2] = pucSrc[1];pucDest[3] = pucSrc[0]; 
*/
                    // CAN TX ID
                    f_read(&fpVer, &g_pstRDBIInfo[i].unCanTxID, sizeof(g_pstRDBIInfo[i].unCanTxID), &nReadenLength);
                    // CAN RX ID
                    f_read(&fpVer, &g_pstRDBIInfo[i].unCanRxID, sizeof(g_pstRDBIInfo[i].unCanRxID), &nReadenLength);
                }

                bRet = TRUE;
            }
        }

        f_close(&fpVer);
    }
    else
        GLogN( "There is no %s file\r\n", FILE_RDBI);
    f_chdir(DIR_ROOT);
    
    return bRet;
}

void CreateRunRepro_Info()
{
    FIL fpVer;
    FRESULT fResult;
    int nWritenLen;
    unsigned short usEcuCnt;
    unsigned char *pActuralVINIdx;
    const unsigned char ucRtyCnt = 0;

    char arrFilePath[255];
    sprintf(arrFilePath, "%s%s/%s/%s", DIR_ROOT, DIR_ECU_TO_UPGRADE_DEPTH1, DIR_ECU_TO_UPGRADE_DEPTH2, FILE_RUNREPRO);
    
    fResult = f_open(&fpVer, arrFilePath, FA_CREATE_ALWAYS|FA_WRITE);
    
    if ( fResult == FR_OK )
    {
        usEcuCnt = g_pstRDBIInfo[0].unECUCount;
        GLogN( "%s : %s, EcuCount %d\r\n", __FUNCTION__, arrFilePath, usEcuCnt);
        // ECU Count
        f_write(&fpVer, &usEcuCnt, sizeof(usEcuCnt), &nWritenLen);
        
        for ( int i=0; i<usEcuCnt; i++ )
        {
            if ( g_pstRDBIInfo[i].bIsUpgradeTarget == TRUE )
            {
                // Rom File Name Size
                f_write(&fpVer, &g_pstRDBIInfo[i].ucRomBinFileNameSize, sizeof(g_pstRDBIInfo[i].ucRomBinFileNameSize), &nWritenLen);
                // Rom File Name
                f_write(&fpVer, g_pstRDBIInfo[i].pRomBinFileName, g_pstRDBIInfo[i].ucRomBinFileNameSize, &nWritenLen);
            
                // RuleDB file name size
                f_write(&fpVer, &g_pstRDBIInfo[i].ucRuleDBFileNameSize, sizeof(g_pstRDBIInfo[i].ucRuleDBFileNameSize), &nWritenLen);

                // RuleDB File name
                f_write(&fpVer, g_pstRDBIInfo[i].pRuleDBFileName, g_pstRDBIInfo[i].ucRuleDBFileNameSize, &nWritenLen);

                // Retry Count
                f_write(&fpVer, &ucRtyCnt, sizeof(ucRtyCnt), &nWritenLen);
            }
        }

        if      ( g_ucAUTOVIN[0] == 0x01 )       pActuralVINIdx = &g_ucAUTOVIN[7];  // CAN RX
        else if ( g_ucAUTOVIN[0] == 0x02 )       pActuralVINIdx = &g_ucAUTOVIN[2];  // KWP RX
        else                                     pActuralVINIdx = &g_ucAUTOVIN[7];

        f_write(&fpVer, pActuralVINIdx, SIZE_VIN_NUM, &nWritenLen);

        f_close(&fpVer);
    }
}

void DeleteRunRepro_Info()
{
    char arrFilePath[255];
    sprintf(arrFilePath, "%s%s/%s/%s", DIR_ROOT, DIR_ECU_TO_UPGRADE_DEPTH1, DIR_ECU_TO_UPGRADE_DEPTH2, FILE_RUNREPRO);
    f_unlink(arrFilePath);
}

void VCI_DataSniffering(stMsgClst *msg, stCommPkt *pkt, ePKT_TD type)
{
//    if ( msg->mPktType != PACKET_INTER_ANALYSIS ) return;
    switch( g_eMainState )
    {
/*
        case eMain_AutoVin:
            g_eMainState = eMain_LoadFile_RunRepro;
            break;
*/
        case eMain_GET_ECU_PART_NO:
            if ( (unsigned int)*pkt->mData == ISO15765 )
            {
                if ( memcmp(g_pstRDBIInfo[g_ucCommRDBIIdx].pPartLid, 
                            pkt->mData+27, g_pstRDBIInfo[g_ucCommRDBIIdx].ucPartLidSize) == 0 )
                {
                    for ( int i=0; i<g_pstRDBIInfo[g_ucCommRDBIIdx].ucPartNoCnt; i++ )
                    {
                        memcpy(g_arrRecvPartNo, pkt->mData+29, g_pstRDBIInfo[g_ucCommRDBIIdx].ucPartByteSize);
                    }

                    g_bWaitForResPacket_InterAnalysis = FALSE;
                    g_eMainState = eMain_GET_ECU_SW_VERION;
                }
            }
            break;
        case eMain_GET_ECU_SW_VERION:
            if ( (unsigned int)*pkt->mData == ISO15765 )
            {
                if ( memcmp(g_pstRDBIInfo[g_ucCommRDBIIdx].pSwVerLid, 
                            pkt->mData+27, g_pstRDBIInfo[g_ucCommRDBIIdx].ucSwVerLidSize) == 0 )
                {
                    memcpy(g_arrRecvSwVer, pkt->mData+29, g_pstRDBIInfo[g_ucCommRDBIIdx].ucSwVerByteSize);

                    if ( g_ucCommRDBIIdx+1 == g_pstRDBIInfo[0].unECUCount )
                    {
                        g_eMainState = eMain_Determin_ECU_To_Upgrade;
                    }
                    else
                    {
                        g_ucCommRDBIIdx++;
                        g_eMainState = eMain_GET_ECU_SESSION_OPEN_1;
                    }
                    g_bWaitForResPacket_InterAnalysis = FALSE;
                }
            }
            break;
        default:
            GLogN( "VCI_DataSniffering : not support\r\n");
            break;
    }
}

BOOL VCI_SendECUInfoPacket(eMain_State eMainState, BOOL bWaitForResPacket)
{
    BOOL bSentRet = FALSE;
    const int nWriteStampSize = 4;
    unsigned int unArrIdx = 0, unPacketTimeout;
    PTmsgPkt_t ECUInfoPacket;
    unsigned char *pusPointer;
    
    if ( g_bWaitForResPacket_InterAnalysis == TRUE ) return bSentRet;
    g_bWaitForResPacket_InterAnalysis = bWaitForResPacket;

    ECUInfoPacket.ProtocolID = ISO15765; // ISO14230�� �ʿ��ϸ� RDBI�� �������� �������� �ؾ���.

    if ( ECUInfoPacket.ProtocolID == ISO15765 )
    {
        memset(ECUInfoPacket.pData, 0x00, 8);
        ECUInfoPacket.RxStatus        = 0;
        ECUInfoPacket.TxFlags         = 0;
        ECUInfoPacket.Timestamp       = GetUnixTime();
    }
    
    switch( eMainState )
    {
        case eMain_GET_ECU_SESSION_OPEN_1:
            ECUInfoPacket.DataSize        = 4;
            ECUInfoPacket.ExtraDataIndex  = 3;

            ECUInfoPacket.pData[unArrIdx++] = 0x07;
            ECUInfoPacket.pData[unArrIdx++] = 0xDF;
            ECUInfoPacket.pData[unArrIdx++] = 0x02;
            ECUInfoPacket.pData[unArrIdx++] = 0x10;
            ECUInfoPacket.pData[unArrIdx++] = 0x81;
            break;
            
        case eMain_GET_ECU_SESSION_OPEN_2:
            ECUInfoPacket.DataSize        = 4;
            ECUInfoPacket.ExtraDataIndex  = 3;

            ECUInfoPacket.pData[unArrIdx++] = 0x07;
            ECUInfoPacket.pData[unArrIdx++] = 0xDF;
            ECUInfoPacket.pData[unArrIdx++] = 0x02;
            ECUInfoPacket.pData[unArrIdx++] = 0x10;
            ECUInfoPacket.pData[unArrIdx++] = 0x83;
            break;
            
        case eMain_GET_ECU_PART_NO:
            {
            ECUInfoPacket.DataSize        = 4+g_pstRDBIInfo[g_ucCommRDBIIdx].ucPartLidSize, g_pstRDBIInfo[g_ucCommRDBIIdx].ucPartLidSize;
            ECUInfoPacket.ExtraDataIndex  = 2+g_pstRDBIInfo[g_ucCommRDBIIdx].ucPartLidSize, g_pstRDBIInfo[g_ucCommRDBIIdx].ucPartLidSize;
            pusPointer = (unsigned char*)&g_pstRDBIInfo[g_ucCommRDBIIdx].unCanTxID;
            ECUInfoPacket.pData[unArrIdx++] = *(pusPointer+2);
            ECUInfoPacket.pData[unArrIdx++] = *(pusPointer+3);
            ECUInfoPacket.pData[unArrIdx++] = 0x03;
            ECUInfoPacket.pData[unArrIdx++] = 0x22;
            memcpy(ECUInfoPacket.pData+unArrIdx, g_pstRDBIInfo[g_ucCommRDBIIdx].pPartLid, g_pstRDBIInfo[g_ucCommRDBIIdx].ucPartLidSize);
            }
            break;
            
        case eMain_GET_ECU_SW_VERION:
            ECUInfoPacket.DataSize        = 4+g_pstRDBIInfo[g_ucCommRDBIIdx].ucPartLidSize, g_pstRDBIInfo[g_ucCommRDBIIdx].ucSwVerLidSize;
            ECUInfoPacket.ExtraDataIndex  = 2+g_pstRDBIInfo[g_ucCommRDBIIdx].ucPartLidSize, g_pstRDBIInfo[g_ucCommRDBIIdx].ucSwVerLidSize;

            ECUInfoPacket.pData[unArrIdx++] = *(pusPointer+2);
            ECUInfoPacket.pData[unArrIdx++] = *(pusPointer+3);
            ECUInfoPacket.pData[unArrIdx++] = 0x03;
            ECUInfoPacket.pData[unArrIdx++] = 0x22;
            memcpy(ECUInfoPacket.pData+unArrIdx, g_pstRDBIInfo[g_ucCommRDBIIdx].pSwVerLid, g_pstRDBIInfo[g_ucCommRDBIIdx].ucSwVerLidSize);
            break;
            
        default:
            break;
    }
    
    unPacketTimeout = GetUnixTime();
    memcpy(g_arrOutputGITPtclBuff, (void*)&unPacketTimeout, sizeof(unsigned int));
    memcpy(g_arrOutputGITPtclBuff+4, (void*)&ECUInfoPacket, sizeof(ECUInfoPacket));
    FL_GitPassThruWriteMsgs(g_arrOutputGITPtclBuff, PACKET_INTER_ANALYSIS, ECUInfoPacket.DataSize+nWriteStampSize);

    if ( ECUInfoPacket.ProtocolID == ISO15765 )
        osDelay(CAN_GetRxP3MinTimeOutValue()+5);    // interval between packet 1st and packet (Nth)
                                                     // Req�� ���� ó�� �ð��� �ʿ���.
    else
        osDelay(300);
    
    bSentRet = TRUE;
    return bSentRet;
}

void Backup_Repro_Result()
{
    DIR Dir;
    FIL fpVer, fpBackup;
    FILINFO FileInfo, FileInfoBK;
    FRESULT fResult, fResultBackup;
    unsigned int nWritenLen, nReadLen, nTotalReadLen, nFileSize = 0, nFileSizeBK = 0, nBKSize;
    unsigned char arrBuf[256] = {0,};
    char *pSrcFileName, *pDestFileName;

    f_chdir(DIR_ROOT);
    fResult = f_opendir(&Dir, DIR_ECU_TO_UPGRADE_DEPTH1);
    if ( fResult != FR_OK ) f_mkdir(DIR_ECU_TO_UPGRADE_DEPTH1);
    f_chdir(DIR_ECU_TO_UPGRADE_DEPTH1);

    fResult = f_opendir(&Dir, DIR_REPRO_RESULT_DEPTH2);
    if ( fResult != FR_OK ) f_mkdir(DIR_REPRO_RESULT_DEPTH2);
    f_chdir(DIR_REPRO_RESULT_DEPTH2);

    if ( f_stat(FILE_REPRO_RESULT, &FileInfo) == FR_OK ) nFileSize = FileInfo.fsize;
    if ( f_stat(FILE_REPRO_RESULT_BK, &FileInfoBK) == FR_OK ) nFileSizeBK = FileInfoBK.fsize;

    if ( nFileSize > nFileSizeBK )
    {
        nBKSize = nFileSize;
        pSrcFileName = FILE_REPRO_RESULT;
        pDestFileName = FILE_REPRO_RESULT_BK;
    }
    else if ( nFileSize < nFileSizeBK )
    {
         nBKSize = nFileSizeBK;
       pSrcFileName = FILE_REPRO_RESULT_BK;
        pDestFileName = FILE_REPRO_RESULT;
    }
    else
    {
        f_chdir(DIR_ROOT);
        return;
    }
        
    fResult = f_open(&fpVer, pSrcFileName, FA_OPEN_EXISTING|FA_READ);
    fResultBackup = f_open(&fpBackup, pDestFileName, FA_CREATE_ALWAYS|FA_WRITE);
    
    if ( fResult == FR_OK && fResultBackup == FR_OK )
    {
        nTotalReadLen = 0;
        
        while ( nBKSize > nTotalReadLen )
        {
            f_read(&fpVer, arrBuf, sizeof(arrBuf), &nReadLen);
            f_write(&fpBackup, arrBuf, nReadLen, &nWritenLen);
            nTotalReadLen += nReadLen;
        }
        f_close(&fpVer);
        f_close(&fpBackup);
    }
    f_chdir(DIR_ROOT);
}

void SaveRepro_Result(unsigned short usEcuIndex)
{
    BOOL bRet = FALSE;
    DIR Dir;
    FIL fpVer;
    FILINFO FileInfo;
    FRESULT fResult;
    unsigned char *pActuralVINIdx;
    unsigned int nWritenLen, nFileSize = 0;
    unsigned char arrWriteBuf[256] = {0,};
    unsigned char arrEncryptVin[128] = "Default_VIN";
    unsigned char strRomFilename[128] = {0,}, strRuleDBFilename[128] = {0,};
    unsigned char aes_iv_cb[16] = { 0x44, 0x69, 0x61, 0x67,0x6E, 0x6F, 0x73, 0x74,
                                    0x69, 0x63, 0x53, 0x67, 0x54, 0x65, 0x61, 0x6D };

    unsigned char aes_key_cbc[16] = { 0x44, 0x69, 0x61, 0x67,0x6E, 0x6F, 0x73, 0x74,
                                    0x69, 0x63, 0x53, 0x67, 0x54, 0x65, 0x61, 0x6D };


    if      ( g_ucAUTOVIN[0] == 0x01 )       pActuralVINIdx = &g_ucAUTOVIN[7];  // CAN RX
    else if ( g_ucAUTOVIN[0] == 0x02 )       pActuralVINIdx = &g_ucAUTOVIN[2];  // KWP RX
    else                                     pActuralVINIdx = &g_ucAUTOVIN[7];

#if 0 // ��ȣ���� �����
    memcpy(&aes_iv_cb[8], gsFwInfo.marrucSerialNo, SERIAL_NUMBER_SIZE);
    memcpy(&aes_key_cbc[8], gsFwInfo.marrucSerialNo, SERIAL_NUMBER_SIZE);

    if ( EscryptAesCbc_Encrypt(arrEncryptVin, pActuralVINIdx, SIZE_VIN_NUM, aes_iv_cb, aes_key_cbc) == FALSE )
    {
        GLogE( "Vin Encrption Error\r\n");
        return;
    }
#else
    if ( g_ucAUTOVIN[0] == 0x00 )
        sprintf(arrEncryptVin, "%s", "Default_VIN");
    else
        sprintf(arrEncryptVin, "%s", pActuralVINIdx);
#endif

    f_chdir(DIR_ROOT);
    fResult = f_opendir(&Dir, DIR_ECU_TO_UPGRADE_DEPTH1);
    if ( fResult != FR_OK ) f_mkdir(DIR_ECU_TO_UPGRADE_DEPTH1);
    f_chdir(DIR_ECU_TO_UPGRADE_DEPTH1);

    fResult = f_opendir(&Dir, DIR_REPRO_RESULT_DEPTH2);
    if ( fResult != FR_OK ) f_mkdir(DIR_REPRO_RESULT_DEPTH2);
    f_chdir(DIR_REPRO_RESULT_DEPTH2);

    if ( f_stat(FILE_REPRO_RESULT, &FileInfo) == FR_OK ) nFileSize = FileInfo.fsize;

    fResult = f_open(&fpVer, FILE_REPRO_RESULT, FA_OPEN_APPEND|FA_WRITE);
    
    if ( fResult == FR_OK )
    {
        memcpy(strRomFilename, g_pstRDBIInfo[usEcuIndex].pRomBinFileName, g_pstRDBIInfo[usEcuIndex].ucRomBinFileNameSize);
        strRomFilename[g_pstRDBIInfo[usEcuIndex].ucRomBinFileNameSize] = '\0';
        memcpy(strRuleDBFilename, g_pstRDBIInfo[usEcuIndex].pRuleDBFileName, g_pstRDBIInfo[usEcuIndex].ucRuleDBFileNameSize);
        strRuleDBFilename[g_pstRDBIInfo[usEcuIndex].ucRuleDBFileNameSize] = '\0';
        
        sprintf(arrWriteBuf, "%s %s_%s %s %s\r\n", 
                arrEncryptVin, g_arrRecvPartNo, g_arrRecvSwVer, strRomFilename, strRuleDBFilename);
        f_write(&fpVer, arrWriteBuf, strlen(arrWriteBuf), &nWritenLen);
        f_close(&fpVer);
    }
    f_chdir(DIR_ROOT);
}

#endif

/*****************************END OF FILE****/
