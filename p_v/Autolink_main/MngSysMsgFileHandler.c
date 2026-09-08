/* Includes ------------------------------------------------------------------*/
#include "GIT_Util.h"
#include "ff.h"

#include "Autolink_Manager.h"
#include "MngSystem.h"
#include "MngSystemUtil.h"
#include "MngQueue.h"
#include "HdDebug.h"
#include "DebugHandler.h"
#include "MngStorage.h"
#include "SysHalFileSystem.h"

#define Trace(...)  GITDebug(DEBUG_MODULES_FILE_SYSTEM,__VA_ARGS__)

#define AUTOLINKDATA_BASE_DIR   "AutoLinkData"
#define AUTOLINK_INTERVAL       "interval_data"
#define AUTOLINK_INTERVAL_TMP   "~interval_data"

#define AUTOLINK_ALRAM_CONTROL  "alram_control_data"
#define HEADER_FILE_USE
#define HEADERFILE				"Header"

eGitFresult DeleteAutolinkFolder(char *pcarrFolderPath);

void DirList(char* strScanDirectory)
{
	eGitFresult res;
	stFileSystemDescript Dir;
#if defined(USE_GIT_FAT_FS)
	char Lfname[512];
	UINT s1=0, s2=0;
	long p1=0;
	FATFS *fs;
	FILINFO Finfo;

	memset(&Finfo, NULL, sizeof(Finfo));

	res = git_f_opendir(&Dir, strScanDirectory);

	if (res) { /*put_rc((eGitFresult)res);*/ return; }

	for(;;)
	{
#if _USE_LFN
		memset(Lfname, 0x00, sizeof(Lfname));
		Finfo.lfname = Lfname;
		Finfo.lfsize = sizeof(Lfname);
#endif
		res = git_f_readdir(&Dir, &Finfo);
		if ((res != GIT_FR_OK) || !Finfo.fname[0])
			break;
		if (Finfo.fattrib & AM_DIR)
		{
			s2++;
		}
		else
		{
			s1++; p1 += Finfo.fsize;
		}

#if _USE_LFN
		Trace("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s [%s]\r\n",
		(Finfo.fattrib & AM_DIR) ? 'D' : '-',
		(Finfo.fattrib & AM_RDO) ? 'R' : '-',
		(Finfo.fattrib & AM_HID) ? 'H' : '-',
		(Finfo.fattrib & AM_SYS) ? 'S' : '-',
		(Finfo.fattrib & AM_ARC) ? 'A' : '-',
		(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
		(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,	Finfo.fsize, Lfname, &(Finfo.fname[0]));
#else
		Trace("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s\r\n",
		(Finfo.fattrib & AM_DIR) ? 'D' : '-',
		(Finfo.fattrib & AM_RDO) ? 'R' : '-',
		(Finfo.fattrib & AM_HID) ? 'H' : '-',
		(Finfo.fattrib & AM_SYS) ? 'S' : '-',
		(Finfo.fattrib & AM_ARC) ? 'A' : '-',
		(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
		(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,	Finfo.fsize, &(Finfo.fname[0]));
		Trace("\r\n");
#endif
	}

//	Trace("\r\n%4u File(s),%10lu bytes total%4u Dir(s)", s1, p1, s2);
//
//	if (git_f_getfree(strScanDirectory, (DWORD*)&p1, &fs) == GIT_FR_OK) {
//		Trace(", %d Mbytes free\r\n\r\n", (uint32_t)((uint64_t)p1 * ((uint64_t)fs->csize * (uint64_t)512)>>20));
//	}
//	else {
//		Trace("\r\n\r\n");
//	}

	Trace("\r\n%4u File(s),%10lu bytes total%4u Dir(s)\r\n", s1, p1, s2);
	AutoLinkManagerData.wSFlashTotalSize = p1;

	if (git_f_getfree(strScanDirectory, (DWORD*)&p1, (void**)&fs) == GIT_FR_OK) {
		AutoLinkManagerData.wSFlashFreeSize = (uint32_t)((uint64_t)p1 * ((uint64_t)fs->csize * (uint64_t)4096));
		Trace(", %d bytes free\r\n\r\n", AutoLinkManagerData.wSFlashFreeSize);
	}
	else {
		Trace("\r\n\r\n");
	}

	Trace("%d bytes total\r\n", AutoLinkManagerData.wSFlashTotalSize);
	Trace("%d bytes free\r\n\r\n", AutoLinkManagerData.wSFlashFreeSize);
	
#elif defined(USE_GIT_LITTLE_FS)

	struct lfs_info Finfo;
	memset(&Finfo, NULL, sizeof(Finfo));

	res = git_f_opendir(&Dir, strScanDirectory);

	if (res != GIT_FR_OK)
	{
		printf("Error Folder Open\n");
		return;
	}

	for(;;)
	{
		res = git_f_readdir(&Dir, &Finfo);
		if( res != GIT_FR_OK )
		{
			break;
		}

		//Trace("%c] %s : %d\r\n",(Finfo.type == LFS_TYPE_REG)?'F':'D', Finfo.name, Finfo.size);
	}
	git_f_closedir(&Dir);
#endif

	return;
}

void ShowDir(char* carrPath)
{
    DirList(carrPath);
}

eGitFresult WriteFile(int iMode, char *pData, uint16_t nLen)
{
	UINT dwFileSize;
	stFileSystemDescript fpFile;

	eGitFresult ret = GIT_FR_OK;
    char path[256];

    if( iMode == FILE_NAME_REPORT ) // interval
    {
        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_INTERVAL);
        //sprintf(path,"%s/%0.2d%0.2d%0.22d",AUTOLINKDATA_BASE_DIR,stRTCTime.RTC_Hours,stRTCTime.RTC_Minutes,stRTCTime.RTC_Seconds);
    }
    else if( iMode == FILE_NAME_CONTROL )
    {

        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_ALRAM_CONTROL);
    }
	//hexdump(pData, nLen);

    ret = git_f_open(&fpFile, (const char *)path, GIT_FA_OPEN_ALWAYS | GIT_FA_WRITE);

    if ( ret != GIT_FR_OK )
    {
        uint8_t ucFileOption = 0;
        sprintf(path,"/%s",AUTOLINKDATA_BASE_DIR);
        ret = git_f_mkdir(path);

        if( iMode == FILE_NAME_REPORT ) // interval
        {
            sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_INTERVAL);
            //sprintf(path,"%s/%0.2d%0.2d%0.22d",AUTOLINKDATA_BASE_DIR,stRTCTime.RTC_Hours,stRTCTime.RTC_Minutes,stRTCTime.RTC_Seconds);
        }
        else if( iMode == FILE_NAME_CONTROL )
        {
            sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_ALRAM_CONTROL);
        }

        if( ret != GIT_FR_OK && ret != GIT_FR_EXIST)
        {
            GIT_Assert(false,eErrorCodeStgFile|eMakeFodlerError);
        }

        ucFileOption = (uint8_t)(GIT_FA_CREATE_NEW | GIT_FA_WRITE);

        if (git_f_open(&fpFile, (const char *)path, ucFileOption ) != GIT_FR_OK )
            GIT_Assert(false,eErrorCodeStgFile|eWriteOpenFail);
    }

    ret = git_f_lseek(&fpFile, git_f_size(&fpFile));

	// message 전송 시간

    int nTotalLength = nLen;
    int nTotalWriteLength = 0;
    int nCount = 0;
    while(1)
    {
        ret = git_f_write(&fpFile, (char *)pData, nLen, &dwFileSize);
        if(ret != GIT_FR_OK) {
            git_f_close(&fpFile);

            Trace("write fail!!! %d \r\n", ret);

            return GIT_FR_DISK_ERR;
        }

        nTotalWriteLength+=dwFileSize;
        nLen-=dwFileSize;

        if( nTotalLength == nTotalWriteLength )
        {
            break;
        }

        if( dwFileSize == 0 )
            break;

        nCount++;
    }

    Trace("%s] File Write Serveral time\n",__FUNCTION__);
	Trace("Success !!! %d \r\n", ret);

	git_f_close(&fpFile);

	return GIT_FR_OK;
}

eGitFresult CheckFile(int iMode,int* pnFileSize)
{
	stFileSystemDescript Dir;
	eGitFresult res,res1 = GIT_FR_NO_FILE;
#if defined(USE_GIT_FAT_FS)
	FILINFO Finfo;
	char Lfname[512];
	UINT s1=0, s2=0;
	long p1=0;

    char path[256];

    if( iMode == FILE_NAME_REPORT ) // interval
    {
        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_INTERVAL);
        //sprintf(path,"%s/%0.2d%0.2d%0.22d",AUTOLINKDATA_BASE_DIR,stRTCTime.RTC_Hours,stRTCTime.RTC_Minutes,stRTCTime.RTC_Seconds);
    }
    else if( iMode == FILE_NAME_CONTROL )
    {

        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_ALRAM_CONTROL);
    }

	memset(&Finfo, NULL, sizeof(Finfo));

	res = git_f_opendir(&Dir, AUTOLINKDATA_BASE_DIR);

	if (res) { /*put_rc((eGitFresult)res);*/ return false; }

	for(;;)
	{
#if _USE_LFN
		memset(Lfname, 0x00, sizeof(Lfname));
		Finfo.lfname = Lfname;
		Finfo.lfsize = sizeof(Lfname);
#endif
		res = git_f_readdir(&Dir, &Finfo);
		if ((res != GIT_FR_OK) || !Finfo.fname[0])
			break;
		if (Finfo.fattrib & AM_DIR)
		{
			s2++;
		}
		else
		{
			s1++; p1 += Finfo.fsize;
		}

        if( strncmp(Finfo.lfname,AUTOLINK_INTERVAL,strlen(AUTOLINK_INTERVAL)) == 0)
        {
            if( Finfo.fsize> 0 )
            {
                *pnFileSize = Finfo.fsize;
                res1 = GIT_FR_OK;
            }
        }
	}

    if( res1 == GIT_FR_OK )
        return true;

    return false;
#elif defined(USE_GIT_LITTLE_FS)
	struct lfs_info Finfo;

    char path[256];

    if( iMode == FILE_NAME_REPORT ) // interval
    {
        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_INTERVAL);
        //sprintf(path,"%s/%0.2d%0.2d%0.22d",AUTOLINKDATA_BASE_DIR,stRTCTime.RTC_Hours,stRTCTime.RTC_Minutes,stRTCTime.RTC_Seconds);
    }
    else if( iMode == FILE_NAME_CONTROL )
    {

        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_ALRAM_CONTROL);
    }

	memset(&Finfo, NULL, sizeof(Finfo));
	res = git_f_opendir(&Dir, path);

	if (res != GIT_FR_OK)
	{
		printf("Error Folder Open\n");
		return GIT_FR_DISK_ERR;
	}

	for(;;)
	{
		res = git_f_readdir(&Dir, &Finfo);
		if( res != GIT_FR_OK )
		{
			break;
		}

		//Trace("%c] %s : %d\r\n",(Finfo.type == LFS_TYPE_REG)?'F':'D', Finfo.name, Finfo.size);

		if( strncmp(Finfo.name,AUTOLINK_INTERVAL,strlen(AUTOLINK_INTERVAL)) == 0)
        {
            if( Finfo.size > 0 )
            {
                *pnFileSize = Finfo.size;
                res1 = GIT_FR_OK;
            }
        }
	}
	git_f_closedir(&Dir);
	
    if( res1 == GIT_FR_OK )
        return true;

    return false;
#endif
}


eGitFresult IsFileEmpty(char * pcarrFullFilePath)
{
	stFileSystemDescript fpFile;

    eGitFresult ret = git_f_open(&fpFile, (const char *)pcarrFullFilePath, GIT_FA_READ); //20210412 확인필요
    if( ret == GIT_FR_OK )
    {
        int nSize = git_f_size(&fpFile);
        git_f_close(&fpFile);

        if( nSize  > 0 )
        {
            return false;
        }
    }

    return true;
}


#define MAX_AUTOLINK_FOLDER_SIZE 6
// example : YYYYMM


bool CheckFolderFormat(char* pcFileName,int size)
{
    for(int i=0;i<size;i++)
    {
        if( pcFileName[i] < '0' || pcFileName[i] > '9' )
            return false;
    }

    return true;
}

bool GetDecFromStr(char* fileName,long long* pullDate, int size)
{
    if( CheckFolderFormat(fileName, size )== true )
    {
        for(int i=0;i<size;i++)
        {
            *pullDate |= (fileName[i]<<((size-i-1)*8));
        }

        return true;
    }

    return false;
}

boolean_t ReadFileHeader(void* fpFinfo, stSaveFileHeader* pstFileHeader)
{
    char carrBuf[64];
	UINT dwFileSize;
    
    int nTotalReadLength = 0;
    int nReadSize;
    int nReadLength;

	eGitFresult eRet=GIT_FR_OK ;
    
	if( fpFinfo == NULL )
	{
		printf("%s] file pointer  is null\n",__FUNCTION__);
		return false;
	}
	
    // move to frist to read file header
    eRet = git_f_lseek(fpFinfo, 0);	// 헤더 파일크기가 작아서 seek 사용한다.
	if( eRet != GIT_FR_OK )
	{
		printf("ReadFileHeader fail2, %d\r\n",eRet);
		return false;
	}

	//printf("file size:%d\r\n",git_f_size(fpFinfo));
    if( git_f_size(fpFinfo) >= sizeof(stSaveFileHeader) )        
    {
        nTotalReadLength = 0;
        nReadSize = sizeof(stSaveFileHeader);
        nReadLength = sizeof(stSaveFileHeader);
        
        while(1)
        {
            if(git_f_read(fpFinfo, &carrBuf[nTotalReadLength], nReadSize, (UINT *)&dwFileSize)!= GIT_FR_OK) {
				printf("git_f_read fail\r\n");
                return false;
            }

            nReadSize-=dwFileSize;
            nTotalReadLength+=dwFileSize;

            if( nTotalReadLength == nReadLength || dwFileSize == 0 )
            {
                break;
            }            
        }
        
        // already exist header
        memcpy((char*)pstFileHeader,carrBuf,nReadLength);
    }
    else
    {
#ifdef FILE_LOG
    	printf("ReadFileHeader~~~\r\n");
#endif
        // new file
        memset((char*)pstFileHeader,0,sizeof(stSaveFileHeader));
    }

    return true;
}


boolean_t WriteFileHeader(void* fpFinfo, stSaveFileHeader* stFileHeader)
{
    char carrBuf[64]={0,};
	UINT dwFileSize;
    
    int nTotalWriteLength = 0;
    int nWriteSize;
    int nWriteLength;

	eGitFresult eRet=GIT_FR_OK;
    
	if( fpFinfo == NULL )
	{
		printf("%s] file pointer is null\n", __FUNCTION__);
		return false;
	}
	
    // move to frist to read file header
    git_f_lseek(fpFinfo, 0);

    stFileHeader->unCheckSum = stFileHeader->readedCount + stFileHeader->writedCount;
	
    // copy to buffer
    memset(carrBuf,0x00,sizeof(carrBuf));
    memcpy(carrBuf,(char*)stFileHeader,sizeof(stSaveFileHeader));

    nTotalWriteLength = 0;
    nWriteSize = sizeof(stSaveFileHeader);
    nWriteLength = sizeof(stSaveFileHeader);

#ifdef FILE_LOG
	printf("!!!");
	for(int i=0;i<sizeof(carrBuf);i++)
	{
    	printf("%02X ",carrBuf[i]);
	}
	printf("\r\n");
#endif
	
    while(1)
    {
    	eRet = git_f_write(fpFinfo, &carrBuf[nTotalWriteLength], nWriteSize, (UINT *)&dwFileSize);
        if( eRet != GIT_FR_OK) {
			printf("WriteFileHeader_%d\r\n",eRet);
            return false;
        }

        nWriteSize-=dwFileSize;
        nTotalWriteLength+=dwFileSize;

        if( nTotalWriteLength == nWriteLength || dwFileSize == 0 )
        {
            break;
        }            
    }
        
    return true;
}
#ifdef HEADER_FILE_USE
eGitFresult WriteAutoLinkData(char *pData, uint16_t nLen, char *pPath)
{
	stFileSystemDescript fpHeader;
	stFileSystemDescript fpFile;
	eGitFresult ret = GIT_FR_OK;    
    char path[256];
	UINT dwFileSize;
    int nTotalWriteLength = 0;
    int nWriteSize;
    int nWriteLength;
    stSaveFileHeader stFileHeader;

	eGitFresult eRet=GIT_FR_OK;

    stHalRTCTypeDef stDate;
    GetLocalTimeforDate(&stDate);
	memset(path,0x00,sizeof(path));
    sprintf(path,"/%s/%04d%02d/%02d%02d%s\x00",AUTOLINKDATA_BASE_DIR,
                        stDate.RtcDate.RTC_Year,
                        stDate.RtcDate.RTC_Month,
                        stDate.RtcDate.RTC_Date,
                        stDate.RtcTime.RTC_Hours,
                        HEADERFILE);
	eRet = git_f_open(&fpHeader, (const char *)path, GIT_FA_OPEN_ALWAYS | GIT_FA_READ | GIT_FA_WRITE);
    if( eRet != GIT_FR_OK )
    {

        sprintf(path,"/%s\x00",AUTOLINKDATA_BASE_DIR);
#ifdef FILE_LOG
		printf("git_f_mkdir: %s\r\n",path);
#endif
        git_f_mkdir(path);
        sprintf(path,"/%s/%04d%02d\x00",AUTOLINKDATA_BASE_DIR,
                        stDate.RtcDate.RTC_Year,
                        stDate.RtcDate.RTC_Month);
#ifdef FILE_LOG
		printf("git_f_mkdir2: %s\r\n",path);
#endif
        git_f_mkdir(path);

        sprintf(path,"/%s/%04d%02d/%02d%02d%s\x00",AUTOLINKDATA_BASE_DIR,
                            stDate.RtcDate.RTC_Year,
                            stDate.RtcDate.RTC_Month,
	                        stDate.RtcDate.RTC_Date,
	                        stDate.RtcTime.RTC_Hours,
                            HEADERFILE);
		eRet = git_f_open(&fpHeader, (const char *)path, GIT_FA_CREATE_NEW | GIT_FA_READ| GIT_FA_WRITE);
        if( eRet != GIT_FR_OK )
        {
            // error
            printf("git_f_open fail__%d\r\n",eRet);
            GIT_Assert(false,eErrorCodeStgFile|eWriteOpenFail);
        }
    }
    if( ReadFileHeader(&fpHeader,&stFileHeader) == true )
    {
        stFileHeader.writedCount++;
        Trace("Write : readed1 count : %d, writed count : %d\r\n",
                stFileHeader.readedCount,stFileHeader.writedCount);
        if( WriteFileHeader(&fpHeader,&stFileHeader) == true )
        {
            nTotalWriteLength = 0;
            nWriteSize = nLen;
			nWriteLength = nLen;
			git_f_close(&fpHeader);
			sprintf(path,"/%s/%04d%02d/%02d%02d\x00",AUTOLINKDATA_BASE_DIR,
								stDate.RtcDate.RTC_Year,
								stDate.RtcDate.RTC_Month,
								stDate.RtcDate.RTC_Date,
								stDate.RtcTime.RTC_Hours);
			
			eRet = git_f_open(&fpFile, (const char *)path, GIT_FA_OPEN_ALWAYS | GIT_FA_APPEND  | GIT_FA_WRITE);
			
			if( eRet != GIT_FR_OK )
			{
				//printf("WriteFileHeader path:%s\r\n",path);
				eRet = git_f_open(&fpFile, (const char *)path, GIT_FA_CREATE_NEW | GIT_FA_WRITE);
				//printf("open fail_22!%d\r\n",eRet);
				if( eRet != GIT_FR_OK )
				{
					// error
					printf("open fail_33!%d\r\n",eRet);
					GIT_Assert(false,eErrorCodeStgFile|eWriteOpenFail);
				}
			}
            while(1)
            {
            	//printf("WriteAutoLinkDatafile: %d,%d,%d\r\n",fpFile.stLittleFsInfo.file.pos,fpFile.stLittleFsInfo.file.ctz.size,fpFile.stLittleFsInfo.file.ctz.head);
				//printf("WFile:%d,%d\r\n",fpFile.stLittleFsInfo.file.ctz.size-fpFile.stLittleFsInfo.file.pos,(fpFile.stLittleFsInfo.file.ctz.size-fpFile.stLittleFsInfo.file.pos)/1629);
			
            	eRet = git_f_write(&fpFile, (char *)&pData[nTotalWriteLength],nWriteSize, &dwFileSize);
				
				//printf("WriteAutoLinkDatafile@: %d,%d,%d\r\n",fpFile.stLittleFsInfo.file.pos,fpFile.stLittleFsInfo.file.ctz.size,fpFile.stLittleFsInfo.file.ctz.head);
				//printf("WFile@:%d,%d\r\n",fpFile.stLittleFsInfo.file.pos-fpFile.stLittleFsInfo.file.ctz.size,(fpFile.stLittleFsInfo.file.pos)/1629);
				
                if( eRet != GIT_FR_OK) {
					printf("git_f_write_%d\r\n",eRet);
                    GIT_Assert(false,eErrorCodeStgFile|eWriteError);
					break;
                }
                nTotalWriteLength+=dwFileSize;
                nWriteSize-=dwFileSize;
                if( nWriteLength == nTotalWriteLength || dwFileSize == 0 )
                {
                    break;
                }
            }
        	Trace("Success !!!\r\n",ret);
            sprintf(path,"/%s/%04d%02d\x00",AUTOLINKDATA_BASE_DIR,
                                    stDate.RtcDate.RTC_Year,
                                    stDate.RtcDate.RTC_Month);
            memcpy((char*)pPath,(char*)path,strlen(path));
#ifdef FILE_LOG
			printf("pPath: %s\r\n",path);
#endif
        }
        else
        {
        	printf("git_f_write22_\r\n");
           GIT_Assert(false,eErrorCodeStgFile|eWriteError);
        }
    }
    else
    {
    	printf("ReadFileHeader33_\r\n");
        GIT_Assert(false,eErrorCodeStgFile|eReadError);
    }
	git_f_close(&fpFile);

	return eRet;
}
#else
eGitFresult WriteAutoLinkData(char *pData, uint16_t nLen, char *pPath)
{
	stFileSystemDescript fpFile;
	eGitFresult ret = GIT_FR_OK;    
    char path[256];
	UINT dwFileSize;
    
    int nTotalWriteLength = 0;
    int nWriteSize;
    int nWriteLength;
    
    stSaveFileHeader stFileHeader;

    stHalRTCTypeDef stDate;
    GetLocalTimeforDate(&stDate);
    sprintf(path,"/%s/%04d%02d/%02d%02d\x00",AUTOLINKDATA_BASE_DIR,
                        stDate.RtcDate.RTC_Year,
                        stDate.RtcDate.RTC_Month,
                        stDate.RtcDate.RTC_Date,
                        stDate.RtcTime.RTC_Hours);
#ifdef FILE_LOG
	printf("WriteAutoLinkData: %s\r\n",path);
#endif

    if( git_f_open(&fpFile, (const char *)path, GIT_FA_OPEN_ALWAYS | GIT_FA_READ | GIT_FA_WRITE) != GIT_FR_OK )
    {
        // make directory
        sprintf(path,"/%s\x00",AUTOLINKDATA_BASE_DIR);
#ifdef FILE_LOG
		printf("git_f_mkdir: %s\r\n",path);
#endif
        git_f_mkdir(path);
        sprintf(path,"/%s/%04d%02d\x00",AUTOLINKDATA_BASE_DIR,
                        stDate.RtcDate.RTC_Year,
                        stDate.RtcDate.RTC_Month);
#ifdef FILE_LOG
		printf("git_f_mkdir2: %s\r\n",path);
#endif
        git_f_mkdir(path);

        sprintf(path,"/%s/%04d%02d/%02d%02d\x00",AUTOLINKDATA_BASE_DIR,
                            stDate.RtcDate.RTC_Year,
                            stDate.RtcDate.RTC_Month,
                            stDate.RtcDate.RTC_Date,
                            stDate.RtcTime.RTC_Hours);

        if( git_f_open(&fpFile, (const char *)path, GIT_FA_CREATE_NEW | GIT_FA_READ| GIT_FA_WRITE) != GIT_FR_OK )
        {
            // error
            GIT_Assert(false,eErrorCodeStgFile|eWriteOpenFail);
        }
    }

    // read header
    if( ReadFileHeader(&fpFile,&stFileHeader) == true )
    {            
        // increase write count
        stFileHeader.writedCount++;

        Trace("Write : readed2 count : %d, writed count : %d\r\n",
                stFileHeader.readedCount,stFileHeader.writedCount);

        if( WriteFileHeader(&fpFile,&stFileHeader) == true )
        {
            // move to last position    
            ret = git_f_lseek(&fpFile, git_f_size(&fpFile));

            nTotalWriteLength = 0;
            nWriteSize = nLen;
            nWriteLength = nLen;

            // write data
            while(1)
            {
                if( git_f_write(&fpFile, (char *)&pData[nTotalWriteLength],nWriteSize, &dwFileSize) != GIT_FR_OK) {
                    GIT_Assert(false,eErrorCodeStgFile|eWriteError);
                }

                nTotalWriteLength+=dwFileSize;
                nWriteSize-=dwFileSize;

                if( nWriteLength == nTotalWriteLength || dwFileSize == 0 )
                {
                    break;
                }
            }

        	Trace("Success !!!\r\n",ret);
            
            sprintf(path,"/%s/%04d%02d\x00",AUTOLINKDATA_BASE_DIR,
                                    stDate.RtcDate.RTC_Year,
                                    stDate.RtcDate.RTC_Month);
            memcpy((char*)pPath,(char*)path,strlen(path));
#ifdef FILE_LOG
			printf("pPath: %s\r\n",path);
#endif
        }
        else
        {
           GIT_Assert(false,eErrorCodeStgFile|eWriteError);
        }
    }
    else
    {
        GIT_Assert(false,eErrorCodeStgFile|eReadError);
		printf("ReadFileHeader fail\r\n");
		return GIT_FR_NOT_SUPPORT;
    }
	
	git_f_close(&fpFile);


	return GIT_FR_OK;
}
#endif

#ifdef HEADER_FILE_USE
eGitFresult ReadAutolinkAsset(int nMode, char* pcarrPath,char* pcarrFileName, char *pData, uint16_t nLen)
{
	UINT dwFileSize;
	stFileSystemDescript fpFile;
	stFileSystemDescript fpHeader;
    int nTotalReadLength = 0;
    int nReadSize;
    int nReadLength;
    char path1[256];
    char path2[256];

	eGitFresult eRes=GIT_FR_OK;

    stSaveFileHeader stFileHeader;

	memset(&stFileHeader,0x00,sizeof(stFileHeader));
	memset(path1,0x00,sizeof(path1));
	memset(path2,0x00,sizeof(path2));
	memset(&fpFile,0x00,sizeof(fpFile));
	memset(&fpHeader,0x00,sizeof(fpHeader));
    sprintf(path1,"/%s/%s/%s",AUTOLINKDATA_BASE_DIR,pcarrPath,pcarrFileName);
    sprintf(path2,"/%s/%s/%s%s",AUTOLINKDATA_BASE_DIR,pcarrPath,pcarrFileName,HEADERFILE);
	
    if ( (pcarrPath == NULL) || (pcarrFileName == NULL) )
    {
    	printf("Path Null %s,%s\r\n",pcarrPath,pcarrFileName);
		return GIT_FR_OK;
    }

    if( nMode == FILE_MODE_DELETE )
    {
        if( git_f_unlink(path1) != GIT_FR_OK)
        {
            GIT_Assert(false,eErrorCodeStgFile|eReadError);
        }
        if( git_f_unlink(path2) != GIT_FR_OK)
        {
            GIT_Assert(false,eErrorCodeStgFile|eReadError);
        }
        return GIT_FR_OK;
    }
	if ( git_f_open(&fpHeader, (const char *)path2, GIT_FA_EXIST | GIT_FA_READ | GIT_FA_WRITE ) == GIT_FR_OK )
    {
        if( ReadFileHeader(&fpHeader,&stFileHeader) == true )
        {
			git_f_close(&fpHeader);
            if( nMode == FILE_MODE_READ )
            {
                Trace("Read : readed3 count : %d, writed count : %d\r\n",
                    stFileHeader.readedCount,stFileHeader.writedCount);
                if( stFileHeader.readedCount < stFileHeader.writedCount )
                {   
                    nTotalReadLength = 0;
                    nReadSize = nLen;
                    nReadLength = nLen;
					
				    //printf("Filerpath:%s open\r\n",path1);
					eRes = git_f_open(&fpFile, (const char *)path1, GIT_FA_EXIST | GIT_FA_READ | GIT_FA_APPEND );

					//printf("ReadAutolinkAssetfile! %d,%d,%d\r\n",fpFile.stLittleFsInfo.file.pos,fpFile.stLittleFsInfo.file.ctz.size,fpFile.stLittleFsInfo.file.ctz.head);
					//printf("ReadAutolinkAssetheader! %d,%d,%d\r\n",fpHeader.stLittleFsInfo.file.pos,fpHeader.stLittleFsInfo.file.ctz.size,fpHeader.stLittleFsInfo.file.ctz.head);
					//printf("File!:%d,%d\r\n",fpFile.stLittleFsInfo.file.ctz.size-fpFile.stLittleFsInfo.file.pos,(fpFile.stLittleFsInfo.file.ctz.size-fpFile.stLittleFsInfo.file.pos)/1629);
					//printf("header!:%d,%d\r\n",fpHeader.stLittleFsInfo.file.ctz.size-fpHeader.stLittleFsInfo.file.pos,(fpHeader.stLittleFsInfo.file.ctz.size-fpHeader.stLittleFsInfo.file.pos)/1629);
						
					if ( eRes == GIT_FR_OK )
					{
					  	eRes = git_f_lseek(&fpFile, (stFileHeader.readedCount*nLen));		//read때만 lseek 사용해야한다!!!!!!!
					  	//printf("lseek:%d\r\n",eRes);
						while(1)
						{
							//printf("ReadAutolinkAssetfile_ %d,%d,%d\r\n",fpFile.stLittleFsInfo.file.pos,fpFile.stLittleFsInfo.file.ctz.size,fpFile.stLittleFsInfo.file.ctz.head);
							//printf("ReadAutolinkAssetheader_ %d,%d,%d\r\n",fpHeader.stLittleFsInfo.file.pos,fpHeader.stLittleFsInfo.file.ctz.size,fpHeader.stLittleFsInfo.file.ctz.head);
							//printf("File_:%d,%d\r\n",fpFile.stLittleFsInfo.file.ctz.size-fpFile.stLittleFsInfo.file.pos,(fpFile.stLittleFsInfo.file.ctz.size-fpFile.stLittleFsInfo.file.pos)/1629);
							//printf("header_:%d,%d\r\n",fpHeader.stLittleFsInfo.file.ctz.size-fpHeader.stLittleFsInfo.file.pos,(fpHeader.stLittleFsInfo.file.ctz.size-fpHeader.stLittleFsInfo.file.pos)/1629);
							
							eRes = git_f_read(&fpFile, &pData[nTotalReadLength], nReadSize, (UINT *)&dwFileSize);
							if( eRes != GIT_FR_OK )
							{
                                printf("git_f_read fail44:%d\r\n",eRes);
								printf("nReadSize:%d,dwFileSize:%d,nLen:%d,nTotalReadLength%d\r\n",nReadSize,dwFileSize,nLen,nTotalReadLength);
								for(int i=0;i<sizeof(stMsgStorage);i++)
								{
									printf("%02X ",pData[i]);									
								}
								printf("\r\n");
								GIT_Assert(false,eErrorCodeStgFile|eReadError);
							}
							nReadSize -= dwFileSize;
							nTotalReadLength += dwFileSize;
							if( nReadLength == nTotalReadLength || dwFileSize == 0 )
							{
								break;
							}
						}
						git_f_close(&fpFile);
					}
					else
					{
						printf("git_f_open fail %d\r\n",eRes);
						return eRes;
					}
                    return GIT_FR_OK;
                }
                else
                {
                	printf("FILE_MODE_READ\r\n");
                    GIT_Assert(false,eErrorCodeStgFile|eReadSizeError);
                }
            }
            else if( nMode == FILE_MODE_READ_AND_DELETE )
            {                
                stFileHeader.readedCount++;
                Trace("Delete : readed4 count : %d, writed count : %d\r\n",
                    	stFileHeader.readedCount,stFileHeader.writedCount);
				if ( git_f_open(&fpHeader, (const char *)path2, GIT_FA_EXIST | GIT_FA_WRITE ) == GIT_FR_OK )
				{
					if( stFileHeader.readedCount < stFileHeader.writedCount )
					{
						if( WriteFileHeader(&fpHeader,&stFileHeader) == true )
						{
							git_f_close(&fpHeader);
							return GIT_FR_OK;
						}
						else
						{
							GIT_Assert(false,eErrorCodeStgFile|eWriteError);
						}                    
					}
					else if( stFileHeader.readedCount == stFileHeader.writedCount )
					{
						git_f_close(&fpHeader);
						if( git_f_unlink(path1) != GIT_FR_OK)
						{
							GIT_Assert(false,eErrorCodeStgFile|eDeleteFolderError);
						}
						if( git_f_unlink(path2) != GIT_FR_OK)
						{
							GIT_Assert(false,eErrorCodeStgFile|eDeleteFolderError);
						}
					}
				}
            }
            else
            {
                GIT_Assert(false,eErrorCodeStgFile|eUnknownControl);
            }
        }
        else
        {
        	printf("ReadFileHeader fail\r\n");
            GIT_Assert(false,eErrorCodeStgFile|eReadError);
        }
	}
    else
    {
        printf("read error read path : %s\n",path1);
    }
	return GIT_FR_OK;
}
#else
eGitFresult ReadAutolinkAsset(int nMode, char* pcarrPath,char* pcarrFileName, char *pData, uint16_t nLen)
{
	UINT dwFileSize;
	stFileSystemDescript fpFile;
    
    int nTotalReadLength = 0;
    int nReadSize;
    int nReadLength;

    char path[256];
    char path2[256];

    stSaveFileHeader stFileHeader;

    sprintf(path,"/%s/%s/%s",AUTOLINKDATA_BASE_DIR,pcarrPath,pcarrFileName);
    sprintf(path2,"/%s/%s/~%s",AUTOLINKDATA_BASE_DIR,pcarrPath,pcarrFileName);
#ifdef FILE_LOG
	printf("path1: %s,%d\r\n",path1,nMode);
	printf("path2: %s\r\n",path2);
#endif

    if ( (pcarrPath == NULL) || (pcarrFileName == NULL) ) return GIT_FR_OK;

    if( nMode == FILE_MODE_DELETE )
    {
        // delete file
        if( git_f_unlink(path) != GIT_FR_OK)
        {
            GIT_Assert(false,eErrorCodeStgFile|eReadError);
        }
        
        return GIT_FR_OK;
    }
    
	if ( git_f_open(&fpFile, (const char *)path, GIT_FA_EXIST | GIT_FA_READ | GIT_FA_WRITE ) == GIT_FR_OK )
    {
        // read header
        if( ReadFileHeader(&fpFile,&stFileHeader) == true )
        {
            if( nMode == FILE_MODE_READ )
            {
                Trace("Read : readed5 count : %d, writed count : %d\r\n",
                    stFileHeader.readedCount,stFileHeader.writedCount);
                
                // exist for reading
                if( stFileHeader.readedCount < stFileHeader.writedCount )
                {   
                    nTotalReadLength = 0;
                    nReadSize = nLen;
                    nReadLength = nLen;

                    // move to read position
                    git_f_lseek(&fpFile, ((stFileHeader.readedCount*nLen)+sizeof(stSaveFileHeader)));

                    while(1)
                    {
            			if( git_f_read(&fpFile, &pData[nTotalReadLength], nReadSize, (UINT *)&dwFileSize) != GIT_FR_OK )
                        {
                            GIT_Assert(false,eErrorCodeStgFile|eReadError);
            			}
                        
                        nReadSize -= dwFileSize;
                        nTotalReadLength += dwFileSize;

                        if( nReadLength == nTotalReadLength || dwFileSize == 0 )
                        {
                            break;
                        }
            		}

                    git_f_close(&fpFile);
                    return GIT_FR_OK;
                }
                else
                {
                    GIT_Assert(false,eErrorCodeStgFile|eReadSizeError);
                }
            }
            else if( nMode == FILE_MODE_READ_AND_DELETE )
            {                
                stFileHeader.readedCount++;

                Trace("Delete : readed6 count : %d, writed count : %d\r\n",
                    stFileHeader.readedCount,stFileHeader.writedCount);

                if( stFileHeader.readedCount < stFileHeader.writedCount )
                {
                    if( WriteFileHeader(&fpFile,&stFileHeader) == true )
                    {
                        git_f_close(&fpFile);
                        
                        return GIT_FR_OK;
                    }
                    else
                    {
                        GIT_Assert(false,eErrorCodeStgFile|eWriteError);
                    }                    
                }
                else if( stFileHeader.readedCount == stFileHeader.writedCount )
                {
                    git_f_close(&fpFile);
                    // delete file
                    if( git_f_unlink(path) != GIT_FR_OK)
                    {
                        GIT_Assert(false,eErrorCodeStgFile|eDeleteFolderError);
                    }
                }
            }
            else
            {
                GIT_Assert(false,eErrorCodeStgFile|eUnknownControl);
            }
        }
        else
        {
            GIT_Assert(false,eErrorCodeStgFile|eReadError);
        }
	}
    else
    {
        printf("read error read path : %s\n",path);
        //GIT_Assert(false,0x00100000);
    }

	return GIT_FR_OK;
}
#endif

eGitFresult FindAutoLinkInfo(int iMode,char* pcPath, char* pcFileName)
{
#if defined(USE_GIT_FAT_FS)
	stFileSystemDescript Dir;
	eGitFresult res;
	FILINFO Finfo;
	char Lfname[512];
    char carrPath[512];

    sprintf(carrPath,"/%s",pcPath);

	memset(&Finfo, NULL, sizeof(Finfo));
	res = git_f_opendir(&Dir, carrPath);
	if (res) { /*put_rc((eGitFresult)res);*/ return false; }

    unsigned long long ullDate = 0xffffffffffffffff;
    unsigned long long ullCurDate = 0;

	for(;;)
	{
#if _USE_LFN
		memset(Lfname, 0x00, sizeof(Lfname));
		Finfo.lfname = Lfname;
		Finfo.lfsize = sizeof(Lfname);
#endif
		res = git_f_readdir(&Dir, &Finfo);
		if((res != GIT_FR_OK) || !Finfo.fname[0])
			break;

        // initialize
        ullCurDate = 0;

		if(Finfo.fattrib & AM_DIR)
		{
            // find directory
            if( iMode == FILE_MODE_FIND_FOLDER )
            {
                if( GetDecFromStr(Finfo.fname,(long long *)&ullCurDate,6) == true )
                {
                    if( ullCurDate < ullDate )
                    {
                        // new folder
                        ullDate = ullCurDate;
                        memcpy(pcFileName,Finfo.fname,6);
                        pcFileName[6]=NULL;
                    }
                }
            }
		}
        else
        {
            // find files
            if( iMode == FILE_MODE_FIND_FILE )
            {
                if( GetDecFromStr(Finfo.fname,(long long *)&ullCurDate,4) == true )
                {
                    if( ullCurDate < ullDate )
                    {
                        // new folder
                        ullDate = ullCurDate;
                        memcpy(pcFileName,Finfo.fname,4);
                        pcFileName[4]=NULL;
                    }
                }
            }
        }
	}

    if( ullDate == 0xffffffffffffffff )
    {
        return false;
    }
    return true;

#elif defined(USE_GIT_LITTLE_FS)

	struct lfs_info Finfo;
	stFileSystemDescript Dir;
	eGitFresult res;
    char carrPath[512]={0,};

    sprintf(carrPath,"/%s",pcPath);
#ifdef FILE_LOG
	printf("FindAutoLinkInfo carrPath : %s\r\n",carrPath);
#endif

	memset(&Finfo, NULL, sizeof(Finfo));
	res = git_f_opendir(&Dir, carrPath);
	if (res != GIT_FR_OK)
	{
		printf("Error Folder Open\n");
		return res;
	}

    unsigned long long ullDate = 0xffffffffffffffff;
    unsigned long long ullCurDate = 0;

	for(;;)
	{
		res = git_f_readdir(&Dir, &Finfo);
		if( res != GIT_FR_OK )
			break;

		//Trace("%c] %s : %d\r\n",(Finfo.type == LFS_TYPE_REG)?'F':'D', Finfo.name, Finfo.size);
		
        // initialize
        ullCurDate = 0;

		if( Finfo.type == LFS_TYPE_DIR )
		{
            // find directory
            if( iMode == FILE_MODE_FIND_FOLDER )
            {
                if( GetDecFromStr(Finfo.name,(long long *)&ullCurDate,6) == true )
                {
                    if( ullCurDate < ullDate )
                    {
                        // new folder
                        ullDate = ullCurDate;
                        memcpy(pcFileName,Finfo.name,6);
                        pcFileName[6]=NULL;
                    }
                }
            }
		}
        else
        {
            // find files
            if( iMode == FILE_MODE_FIND_FILE )
            {
                if( GetDecFromStr(Finfo.name,(long long *)&ullCurDate,4) == true )
                {
                    if( ullCurDate < ullDate )
                    {
                        // new folder
                        ullDate = ullCurDate;
                        memcpy(pcFileName,Finfo.name,4);
                        pcFileName[4]=NULL;
                    }
                }
            }
        }
	}
	git_f_closedir(&Dir);
	
    if( ullDate == 0xffffffffffffffff )
    {
        return false;
    }
    return true;

#endif
}

int SearchLatestFile(stAutolinkFileInfo* pstLatestReadFileInfo)
{
    char carrFileFullPath[256];
    char carrFolderPath[256];
    char carrFilePath[256];
    char carrTmpPath[256];

    if( FindAutoLinkInfo(FILE_MODE_FIND_FOLDER,AUTOLINKDATA_BASE_DIR,carrFolderPath) == true )
    {
        sprintf(carrTmpPath,"%s/%s\x00",AUTOLINKDATA_BASE_DIR,carrFolderPath);

        if( FindAutoLinkInfo(FILE_MODE_FIND_FILE,carrTmpPath,carrFilePath) == true )
        {
            sprintf(carrFileFullPath,"/%s/%s/%s\x00",AUTOLINKDATA_BASE_DIR,
                                        carrFolderPath,
                                        carrFilePath);

            memcpy(pstLatestReadFileInfo->m_carrLatestFilePath,carrFilePath,strlen(carrFilePath));
            memcpy(pstLatestReadFileInfo->m_carrLatestFolderPath,carrFolderPath,strlen(carrFolderPath));
            memcpy(pstLatestReadFileInfo->m_carrLatestFullFilePath,carrFileFullPath,strlen(carrFileFullPath));

            Trace("find file path : %s\r\n",pstLatestReadFileInfo->m_carrLatestFullFilePath);
            return true;
        }

        // delete empty folder because there is no file in the folder
        DeleteAutolinkFolder(carrFolderPath);
    }

    //if( IsExistData() == true )
    //{
    //    Send2MngSysMsg(eMngStorage,eRspDataExist,IsExistData(),(stCarReport*)NULL,0);
    //}

    memset((char*)pstLatestReadFileInfo->m_carrLatestFilePath,0,strlen(carrFilePath));
    memset((char*)pstLatestReadFileInfo->m_carrLatestFolderPath,0,strlen(carrFolderPath));
    memset((char*)pstLatestReadFileInfo->m_carrLatestFullFilePath,0,strlen(carrFileFullPath));


    return false;
}

int CheckFile2(int iMode)
{
	stFileSystemDescript fpFile;

    char path[256];

    if( iMode == FILE_NAME_REPORT ) // interval
    {
        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_INTERVAL);
    }
    else if( iMode == FILE_NAME_CONTROL )
    {

        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_ALRAM_CONTROL);
    }

    int nFileSize = -1;
    if ( (git_f_open(&fpFile, (const char *)path, GIT_FA_EXIST | GIT_FA_READ)) == GIT_FR_OK )
    {
        nFileSize =  git_f_size(&fpFile);

        if( git_f_close(&fpFile) != GIT_FR_OK )
            GIT_Assert(false,eErrorCodeStgFile|eCloseError);
	}

    return nFileSize;
}


eGitFresult ReadFile(int iMode, char *pData, uint16_t nLen)
{
	UINT dwFileSize2;
    eGitFresult ret = GIT_FR_OK;
	stFileSystemDescript fpFile;

    char path[256];
    char path2[256];

    if( iMode == FILE_NAME_REPORT ) // interval
    {
        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_INTERVAL);
        sprintf(path2,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_INTERVAL_TMP);
        //sprintf(path,"%s/%0.2d%0.2d%0.22d",AUTOLINKDATA_BASE_DIR,stRTCTime.RTC_Hours,stRTCTime.RTC_Minutes,stRTCTime.RTC_Seconds);
    }
    else if( iMode == FILE_NAME_CONTROL )
    {

        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_ALRAM_CONTROL);
    }

    dwFileSize2=0;
    int totalSize = 0;
    int readSize = nLen;
	if ( (ret = git_f_open(&fpFile, (const char *)path, GIT_FA_EXIST | GIT_FA_READ)) == GIT_FR_OK )
    {
		while(1)
        {
			if ((ret = git_f_read(&fpFile, &pData[totalSize], readSize, (UINT *)&dwFileSize2)) == GIT_FR_OK )
            {
                if(dwFileSize2 > 0)
                {
                    if( readSize == dwFileSize2 )
                    {
                        break;
                    }

                    readSize = readSize - dwFileSize2;
                    totalSize = totalSize+dwFileSize2;
                }
                else
                {
                    break;
                }

			}
			else
            {
				break;
			}
		}

		git_f_close(&fpFile);
	}
	else {
		Trace("open fail!!! %d \r\n", ret);

		return GIT_FR_DISK_ERR;
	}

	return GIT_FR_OK;
}


eGitFresult ReadFile2(int iMode, char *pData, uint16_t nLen)
{
	UINT dwFileSize2;
	stFileSystemDescript fpFile;
	stFileSystemDescript fpTemp;

	eGitFresult ret = GIT_FR_OK;

    char path[256];
    char path2[256];

    if( iMode == FILE_NAME_REPORT ) // interval
    {
        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_INTERVAL);
        sprintf(path2,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_INTERVAL_TMP);
        //sprintf(path,"%s/%0.2d%0.2d%0.22d",AUTOLINKDATA_BASE_DIR,stRTCTime.RTC_Hours,stRTCTime.RTC_Minutes,stRTCTime.RTC_Seconds);
    }
    else if( iMode == FILE_NAME_CONTROL )
    {

        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_ALRAM_CONTROL);
    }

    dwFileSize2=0;
    int totalSize = 0;
    int readSize = nLen;
	if ( (ret = git_f_open(&fpFile, (const char *)path, GIT_FA_EXIST | GIT_FA_READ)) == GIT_FR_OK )
    {
		while(1)
        {
			if ((ret = git_f_read(&fpFile, &pData[totalSize], readSize, (UINT *)&dwFileSize2)) == GIT_FR_OK )
            {
                if(dwFileSize2 > 0)
                {
                    if( readSize == dwFileSize2 )
                    {
                        break;
                    }

                    readSize = readSize - dwFileSize2;
                    totalSize = totalSize+dwFileSize2;
                }
                else
                {
                    break;
                }

			}
			else
            {
				break;
			}
		}

        if( git_f_size(&fpFile) >= nLen )
        {
            if ( (git_f_open(&fpTemp, (const char *)path2, GIT_FA_OPEN_ALWAYS | GIT_FA_WRITE)) == GIT_FR_OK )
            {
                ret = git_f_lseek(&fpFile, nLen);
                if( ret != GIT_FR_OK )
                {
                    GIT_Assert(false,eErrorCodeStgFile|eSeekError);
                }

                unsigned char tmpData;
                int iWriteSize = nLen;
                int iOriginalSize = git_f_size(&fpFile);

                // write data to temp file
        		while(1)
                {
        			if ((ret = git_f_read(&fpFile, &tmpData, 1, (UINT *)&dwFileSize2)) == GIT_FR_OK )
                    {
                        if(dwFileSize2 > 0)
                        {
                            ret = git_f_write(&fpTemp, (char *)&tmpData, 1, &dwFileSize2);
                            if(ret != GIT_FR_OK) {
                                git_f_close(&fpTemp);
                                GIT_Assert(false,eErrorCodeStgFile|eWriteError);
                            }

                            iWriteSize++;

                        }
                        else
                        {
                            break;
                        }

        			}
        			else
                    {
        				break;
        			}
        		}

                // check assert all status for test
                if( iWriteSize != iOriginalSize )
                {
                    GIT_Assert(false,eErrorCodeStgFile|eWriteSizeError);
                }

                if( git_f_close(&fpTemp) )
                {
                    GIT_Assert(false,eErrorCodeStgFile|eCloseError);
                }

                // delete old file
                if( git_f_unlink(path) )
                {
                    GIT_Assert(false,eErrorCodeStgFile|eDeleteFileError);
                }

                // rename new file to our log file name
                if( git_f_rename(path2,path) )
                {
                    GIT_Assert(false,eErrorCodeStgFile|eRenameError);
                }
            }
        }

		git_f_close(&fpFile);
	}
	else {
		Trace("open fail!!! %d \r\n", ret);

		return GIT_FR_DISK_ERR;
	}

	return GIT_FR_OK;
}

eGitFresult DeleteAutolinkFile(char *pcarrFullPath)
{
	eGitFresult ret;

	if ( (ret = git_f_unlink((const char *)pcarrFullPath)) == GIT_FR_OK ) {
        Trace("[%s] delete success!!\r\n", pcarrFullPath);
	}
	else {
		Trace("[%s] delete fail!!! : %d \r\n", pcarrFullPath, ret);

		return GIT_FR_DISK_ERR;
	}

    return GIT_FR_OK;
}

boolean_t IsFileEmpty2(char* pstrFolder, char* pstrFileName)
{
	stFileSystemDescript Dir;
	eGitFresult res;
#if defined(USE_GIT_FAT_FS)
	FILINFO Finfo;
	char Lfname[512];
//	UINT s1=0, s2=0;
//	long p1;
//	FATFS *fs;

	memset(&Finfo, NULL, sizeof(Finfo));

	res = git_f_opendir(&Dir, pstrFolder);

	if (res) { /*put_rc((eGitFresult)res);*/ return true; }

	for(;;)
	{
#if _USE_LFN
		memset(Lfname, 0x00, sizeof(Lfname));
		Finfo.lfname = Lfname;
		Finfo.lfsize = sizeof(Lfname);
#endif
		res = git_f_readdir(&Dir, &Finfo);
		if ((res != GIT_FR_OK) || !Finfo.fname[0])
			break;

		if( strncmp(Finfo.fname,pstrFileName,strlen(pstrFileName))== 0)
            if( Finfo.fsize > 0 )
            {
                git_f_chdir("/");
				git_f_closedir(&Dir);
                return false;
            }
	}
    git_f_chdir("/");

    return true;
	
#elif defined(USE_GIT_LITTLE_FS)
	struct lfs_info Finfo;
	
	memset(&Finfo, NULL, sizeof(Finfo));

	res = git_f_opendir(&Dir, pstrFolder);

	if (res) { /*put_rc((eGitFresult)res);*/ return true; }

	for(;;)
	{
		res = git_f_readdir(&Dir, &Finfo);
		if (res != GIT_FR_OK)
			break;

		if( strncmp(Finfo.name,pstrFileName,strlen(pstrFileName))== 0)
            if( Finfo.size > 0 )
            {
                git_f_chdir("/");
				git_f_closedir(&Dir);
                return false;
            }
	}
    git_f_chdir("/");

	git_f_closedir(&Dir);
    return true;
	
	
#endif
}


boolean_t IsFolderEmpty(char* pcarrFolderFullPath)
{
	stFileSystemDescript Dir;
	eGitFresult res;
#if defined(USE_GIT_FAT_FS)
	FILINFO Finfo;
	char Lfname[512];
	UINT s1=0, s2=0;
	long p1 = 0;
//	FATFS *fs;

	memset(&Finfo, NULL, sizeof(Finfo));

	res = git_f_opendir(&Dir, pcarrFolderFullPath);

	if (res) { /*put_rc((eGitFresult)res);*/ return false; }

	for(;;)
	{
#if _USE_LFN
		memset(Lfname, 0x00, sizeof(Lfname));
		Finfo.lfname = Lfname;
		Finfo.lfsize = sizeof(Lfname);
#endif
		res = git_f_readdir(&Dir, &Finfo);
		if ((res != GIT_FR_OK) || !Finfo.fname[0])
			break;
		if (Finfo.fattrib & AM_DIR)
		{
			s2++;
		}
		else
		{
			s1++; p1 += Finfo.fsize;
		}

#if false
#if _USE_LFN
		Trace("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s [%s]\r\n",
		(Finfo.fattrib & AM_DIR) ? 'D' : '-',
		(Finfo.fattrib & AM_RDO) ? 'R' : '-',
		(Finfo.fattrib & AM_HID) ? 'H' : '-',
		(Finfo.fattrib & AM_SYS) ? 'S' : '-',
		(Finfo.fattrib & AM_ARC) ? 'A' : '-',
		(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
		(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,	Finfo.fsize, Lfname, &(Finfo.fname[0]));
#else
		Trace("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s\r\n",
		(Finfo.fattrib & AM_DIR) ? 'D' : '-',
		(Finfo.fattrib & AM_RDO) ? 'R' : '-',
		(Finfo.fattrib & AM_HID) ? 'H' : '-',
		(Finfo.fattrib & AM_SYS) ? 'S' : '-',
		(Finfo.fattrib & AM_ARC) ? 'A' : '-',
		(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
		(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,	Finfo.fsize, &(Finfo.fname[0]));
		Trace("\r\n");
#endif
#endif
	}

	if( p1 > 0 )
        return false;

    return true;
#elif defined(USE_GIT_LITTLE_FS)
	struct lfs_info Finfo;
	long p1=0;

	memset(&Finfo, NULL, sizeof(Finfo));

	res = git_f_opendir(&Dir, pcarrFolderFullPath);

	if (res != GIT_FR_OK)
	{
		printf("Error Folder Open\n");
		return GIT_FR_DISK_ERR;
	}

	for(;;)
	{

		res = git_f_readdir(&Dir, &Finfo);
		if( res != GIT_FR_OK )
		{
			break;
		}
		if (Finfo.type == LFS_TYPE_REG)
		{
			p1 += Finfo.size;
		}
		//Trace("%c] %s : %d\n",(Finfo.type == LFS_TYPE_REG)?'F':'D', Finfo.name, Finfo.size);

	}

	git_f_closedir(&Dir);
	if( p1 > 0 )
        return false;

    return true;
#endif
}


eGitFresult DeleteAutolinkFolder(char *pcarrFolderPath)
{
    eGitFresult ret;
    char carrTmpPath[256];
    char carrTmpPath1[256];

    sprintf(carrTmpPath,"/%s/%s\x00",AUTOLINKDATA_BASE_DIR,pcarrFolderPath);
    sprintf(carrTmpPath1,"/%s\x00",AUTOLINKDATA_BASE_DIR);

    if( IsFolderEmpty(carrTmpPath) == true )
    {
        git_f_chdir(carrTmpPath1);
        //delete folder
        ret = git_f_unlink((const char *)carrTmpPath);
        if ( ret == GIT_FR_OK ) {
            Trace("[%s] delete Success!!! %d \r\n",carrTmpPath, ret);
        }
        else {
            Trace("[%s] delete fail!!! %d \r\n",carrTmpPath, ret);

            return GIT_FR_DISK_ERR;
        }
    }


    return GIT_FR_OK;
}


eGitFresult DeleteFileTemp(int iMode, char *pData, uint16_t nLen)
{
	eGitFresult ret = GIT_FR_OK;
    char path[256];

    if( iMode == FILE_NAME_REPORT ) // interval
    {
        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_INTERVAL_TMP);
        //sprintf(path,"%s/%0.2d%0.2d%0.22d",AUTOLINKDATA_BASE_DIR,stRTCTime.RTC_Hours,stRTCTime.RTC_Minutes,stRTCTime.RTC_Seconds);
    }
    else if( iMode == FILE_NAME_CONTROL)
    {

        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_ALRAM_CONTROL);
    }


	if ( (ret = git_f_unlink((const char *)path)) == GIT_FR_OK ) {
        Trace("[%s] delete success!!!\r\n", path, ret);
	}
	else {
		Trace("[%s] delete fail!!! : %d \r\n", path, ret);

		return GIT_FR_DISK_ERR;
	}

	return GIT_FR_OK;
}

eGitFresult DeleteFile(int iMode, char *pData, uint16_t nLen)
{
	eGitFresult ret = GIT_FR_OK;
    char path[256];

    if( iMode == FILE_NAME_REPORT ) // interval
    {
        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_INTERVAL);
        //sprintf(path,"%s/%0.2d%0.2d%0.22d",AUTOLINKDATA_BASE_DIR,stRTCTime.RTC_Hours,stRTCTime.RTC_Minutes,stRTCTime.RTC_Seconds);
    }
    else if( iMode == FILE_NAME_CONTROL)
    {

        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_ALRAM_CONTROL);
    }


	if ( (ret = git_f_unlink((const char *)path)) == GIT_FR_OK ) {
        Trace("[%s] delete success!!! \r\n", path);
	}
	else {
		Trace("[%s] delete fail!!! : %d \r\n", path, ret);

		return GIT_FR_DISK_ERR;
	}

	return GIT_FR_OK;
}

eGitFresult Rename()
{
    int iMode = FILE_NAME_REPORT;
    char path[256];
    char path2[256];

    if( iMode == FILE_NAME_REPORT ) // interval
    {
        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_INTERVAL);
        sprintf(path2,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_INTERVAL_TMP);
        //sprintf(path,"%s/%0.2d%0.2d%0.22d",AUTOLINKDATA_BASE_DIR,stRTCTime.RTC_Hours,stRTCTime.RTC_Minutes,stRTCTime.RTC_Seconds);
    }
    else if( iMode == FILE_NAME_CONTROL )
    {

        sprintf(path,"/%s/%s",AUTOLINKDATA_BASE_DIR,AUTOLINK_ALRAM_CONTROL);
    }

    if( git_f_rename(path2,path) == GIT_FR_OK )
    {
    }
    else
    {
        Trace("Error Rename\n");
    }

    return GIT_FR_OK;
}

eGitFresult DelAllFile(void)
{
	git_directory_list();
	
	/*
	if(memcmp((char *)Lfname, "ErrMsg_", 7) == 0) {
		if ( (res = git_f_unlink((const char *)MessageManagerData.aErrMsgFileName)) == GIT_FR_OK ) {
			Trace("FS: delete file [%s]\n", Finfo.fname);
		}
		else {
			Trace("@%s(), %s delete fail!!! %d \r\n", __FUNCTION__, MessageManagerData.aErrMsgFileName, res);
		}
	}
	*/


	return GIT_FR_OK;
}

eGitFresult DelAllAsset(char* pcarrPath)
{
	stFileSystemDescript Dir;
	eGitFresult res;
#if defined(USE_GIT_FAT_FS)
	FILINFO Finfo;
	char Lfname[128];
	UINT s1 = 0, s2 = 0;
	long p1 = 0;

	memset(&Finfo, NULL, sizeof(Finfo));

    sprintf(Lfname,"/%s",AUTOLINKDATA_BASE_DIR);

	res = git_f_opendir(&Dir, Lfname);

	if (res) {
		return res;
	}

	Trace("\r\n");
	Trace("fs: current directory - [%s]\r\n", DIR_ROOT);

	for(;;) {
#if _USE_LFN
		Finfo.lfname = Lfname;
		Finfo.lfsize = sizeof(Lfname);
		Finfo.fsize = 0;
#endif
		res = git_f_readdir(&Dir, &Finfo);
		if ((res != GIT_FR_OK) || !Finfo.fname[0])
			break;

		if (Finfo.fattrib & AM_DIR) {
			s2++;
		}
		else {
			s1++;
			p1 += Finfo.fsize;
		}

		Trace("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s ",
										(Finfo.fattrib & AM_DIR) ? 'D' : '-',
										(Finfo.fattrib & AM_RDO) ? 'R' : '-',
										(Finfo.fattrib & AM_HID) ? 'H' : '-',
										(Finfo.fattrib & AM_SYS) ? 'S' : '-',
										(Finfo.fattrib & AM_ARC) ? 'A' : '-',
										(Finfo.fdate >> 9) + 1980, (Finfo.fdate >> 5) & 15, Finfo.fdate & 31,
										(Finfo.ftime >> 11), (Finfo.ftime >> 5) & 63,
										Finfo.fsize, &(Finfo.fname[0]));
#if _USE_LFN
		if(strlen((const char *)Lfname) > 0) {
			Trace("[%s]\r\n", Lfname);
		}
		else {
			Trace("\r\n");
		}
#else
		Trace("\r\n");
#endif
        if( Finfo.fname[0] != '.' )
        {
            res = git_f_unlink((const char *)Finfo.fname);
            Trace("%d\n",res);
            res = git_f_unlink((const char *)Finfo.lfname);
            Trace("%d\n",res);
        }
	}

	return GIT_FR_OK;
#elif defined(USE_GIT_LITTLE_FS)
	struct lfs_info Finfo;
	char Lfname[128];
	
	memset(&Finfo, NULL, sizeof(Finfo));

    sprintf(Lfname,"/%s",AUTOLINKDATA_BASE_DIR);

	res = git_f_opendir(&Dir, Lfname);

	if (res) {
		return res;
	}

	Trace("\r\n");
	Trace("fs: current directory - [%s]\r\n", DIR_ROOT);

	for(;;) {
		res = git_f_readdir(&Dir, &Finfo);
		if( res != GIT_FR_OK )
		{
			break;
		}

		//Trace("%c] %s : %d\n",(Finfo.type == LFS_TYPE_REG)?'F':'D', Finfo.name, Finfo.size);

		//Trace("\r\n");
		
        if( Finfo.name[0] != '.' )
        {
            res = git_f_unlink((const char *)Finfo.name);
            Trace("%d\n",res);
        }
	}
	git_f_closedir(&Dir);

	return GIT_FR_OK;
	
	
#endif
}


