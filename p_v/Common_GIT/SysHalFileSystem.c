/**
  ******************************************************************************
  * @file    SysHalFileSystem.c
  * @author  GIT Application Team by hkmoon
  * @version V1.1.0
  * @date    05-SEP-2013
  * @brief   Header for SysHalFileSystem.c module
  ******************************************************************************
 **/
#include "SysHalFileSystem.h"
#include "GIT_Util.h"
#include "HdDebug.h"
#include "HalHandler.h"

#define Trace(...)  GITDebug(DEBUG_MODULES_UTIL,__VA_ARGS__)

#ifdef USE_GIT_FAT_FS
extern FATFS g_Fatfs[_VOLUMES];
#endif

#ifdef USE_GIT_LITTLE_FS
extern stFileSystemInfo m_stFileSystemInfo;
#endif //#ifdef USE_GIT_LITTLE_FS

extern void directory_list(void);

boolean_t InitFileSystem(void)
{
	// mount little file systems.

#if defined(USE_GIT_FAT_FS)
	Trace("FS: Start File System\n");
	if (f_mount(FAT_VOLUME_DRV, &g_Fatfs[0]) == FR_OK)
	{
		Trace("FS: Fat File System mount Ok\r\n");

		if ( f_chdir(DIR_ROOT) == FR_OK )
		{
			Trace("FS: disk initial OK\r\n");
			Trace("FS: Change Directory root Ok\r\n");
			directory_list();
			return TRUE;
		}
		else
		{
			Trace("FS: format start\r\n");

			if ( f_mkfs(FAT_VOLUME_DRV, 0, 4096) == FR_OK )
			{
				Trace("FS: format success\r\n");

				if ( f_chdir(DIR_ROOT) == FR_OK )
				{
					Trace("FS: Create default foler\r\n");
					f_mkdir(DIR_LOG);
					f_mkdir(DIR_FIRMWARE);
					f_chmod(DIR_FIRMWARE, AM_HID, AM_HID);
					f_mkdir(DIR_INFORMATION);
					directory_list();
					return TRUE;
				}
				else
				{
					Trace("FS: FR_NOK\r\n");
				}
			}
			else
			{
				Trace("FS: format fail\r\n");
			}
		}
	}
	else
	{
		if ( f_mkfs(FAT_VOLUME_DRV, 0, 4096) == FR_OK )
		{
			Trace("FS: format success\r\n");

			if ( f_chdir(DIR_ROOT) == FR_OK )
			{
				Trace("FS: Create default foler\r\n");
				f_mkdir(DIR_LOG);
				f_mkdir(DIR_FIRMWARE);
				f_mkdir(DIR_INFORMATION);
				directory_list();
				return TRUE;
			}
			else
			{
				Trace("FS: FR_NOK\r\n");
			}
		}
		else
		{
			Trace("FS: format fail\r\n");
		}
	}

	return FALSE;

#elif defined(USE_GIT_LITTLE_FS)

	// initialize little file systems
	if( LfsInit() == false )
	{
		printf("error mount file system\r\n");
		return GIT_FR_DISK_ERR;
	}

	return GIT_FR_OK;
#endif
}


boolean_t DeInitFileSystem(void)
{
	// mount little file systems.

#if defined(USE_GIT_FAT_FS)
	Trace("FS: Start File System\r\n");
	return true;

#elif defined(USE_GIT_LITTLE_FS)

	git_f_umount((uint32_t)NULL,(void*)NULL);

	return true;
#endif
}

eGitFresult git_disk_write(
	uint32_t drv,			/* Physical drive number (0) */
	const uint8_t *buff,	/* Pointer to the data to be written */
	uint32_t sector,		/* Start sector number (LBA) */
	uint32_t count			/* Sector count (1..255) */
)
{
	for (int i = 1; i <= count; i++)
	{
		//HalHandlerIOCtrl(DRV_ID_SPI, eSpiEraseSubSector, (sector * i * SFLASH_SECTOR_SIZE),NULL,0,0);
		sFLASH_EraseSubSector(sector * i * SFLASH_SECTOR_SIZE);
	}

	sFLASH_WriteBuffer((uint8_t *) buff, sector * SFLASH_SECTOR_SIZE, count * SFLASH_SECTOR_SIZE);
	//HalDrvSPIWrite(DRV_ID_SPI, (int)(sector * SFLASH_SECTOR_SIZE), 0,(char *)buff, (int)(count * SFLASH_SECTOR_SIZE),0);

	return GIT_FR_OK;
}

eGitFresult git_disk_read(
	uint32_t drv,			/* Physical drive number (0) */
	uint8_t *buff,			/* Pointer to the data buffer to store read data */
	uint32_t sector,		/* Start sector number (LBA) */
	uint32_t count			/* Sector count (1..255) */
)
{
	sFLASH_ReadBuffer(buff, sector * SFLASH_SECTOR_SIZE, count * SFLASH_SECTOR_SIZE);
	//HalDrvSPIRead(DRV_ID_SPI, (sector * SFLASH_SECTOR_SIZE), 0, (char *)buff, (count * SFLASH_SECTOR_SIZE),0);

	return GIT_FR_OK;
}

eGitFresult git_f_mount (
	uint32_t vol,		/* Logical drive number to be mounted/unmounted */
	void *fs		/* Pointer to new file system object (NULL for unmount)*/
)
{
#if defined(USE_GIT_FAT_FS)

	return (eGitFresult)f_mount(vol,(FATFS*)fs);

#elif defined(USE_GIT_LITTLE_FS)

	if( lfs_mount(&m_stFileSystemInfo.stLfs.fs, &m_stFileSystemInfo.stLfs.cfg) < 0 )
		return GIT_FR_DISK_ERR;

	return GIT_FR_OK;
#endif
}

eGitFresult git_f_umount (
	uint32_t vol,		/* Logical drive number to be mounted/unmounted */
	void *fs		/* Pointer to new file system object (NULL for unmount)*/
)
{
#if defined(USE_GIT_FAT_FS)

	return GIT_FR_NOT_SUPPORT;

#elif defined(USE_GIT_LITTLE_FS)

	return (eGitFresult)lfs_unmount(GetLittleFSDescript());
#endif
}



eGitFresult git_f_format(
	uint32_t vol,		/* Logical drive number to be mounted/unmounted */
	void *fs		/* Pointer to new file system object (NULL for unmount)*/
)
{
#if defined(USE_GIT_FAT_FS)

	return (eGitFresult)f_mount(vol,(FATFS*)fs);

#elif defined(USE_GIT_LITTLE_FS)

	if( lfs_format(&m_stFileSystemInfo.stLfs.fs, &m_stFileSystemInfo.stLfs.cfg) < 0 )
		return GIT_FR_DISK_ERR;

	return GIT_FR_OK;
#endif
}

eGitFresult git_f_mkfs(
		uint32_t drv,		/* Logical drive number */
		uint32_t sfd,		/* Partitioning rule 0:FDISK, 1:SFD */
		uint32_t au 		/* Allocation unit size [bytes] */
)
{
#if defined(USE_GIT_FAT_FS)

	return (eGitFresult)f_mkfs(drv,sfd,au);
#elif defined(USE_GIT_LITTLE_FS)

		if (lfs_format(&m_stFileSystemInfo.stLfs.fs, &m_stFileSystemInfo.stLfs.cfg) < 0)
		{
			printf("format fail\r\n");

			return false;
		}
		

		if (lfs_mount(&m_stFileSystemInfo.stLfs.fs, &m_stFileSystemInfo.stLfs.cfg) < 0)
		{
			printf("mount fail\r\n");
			return false;
		}
	return GIT_FR_OK;
#endif
}

eGitFresult git_f_chdir (
	const TCHAR *path	/* Pointer to the directory path */
)
{
#if defined(USE_GIT_FAT_FS)
	return (eGitFresult)f_chdir(path);
#elif defined(USE_GIT_LITTLE_FS)
//	stLittleFsFileInfo Dir;
	

//	lfs_dir_open(GetLittleFSDescript(), (lfs_dir_t*)&Dir.dir, path);
	
//	lfs_dir_read(GetLittleFSDescript(), (lfs_dir_t*)&Dir.dir, NULL);
	return GIT_FR_OK;
#endif

}

eGitFresult git_f_mkdir (
	const TCHAR *path		/* Pointer to the directory path */
)
{
#if defined(USE_GIT_FAT_FS)
	return (eGitFresult)f_mkdir(path);

#elif defined(USE_GIT_LITTLE_FS)

	int nRet = lfs_mkdir(GetLittleFSDescript(), path);

	if( nRet == 0 )
		return GIT_FR_OK;


	return (eGitFresult)nRet;

#endif
}


eGitFresult git_f_chmod (
	const TCHAR *path,	/* Pointer to the file path */
	uint32_t value,			/* Attribute bits */
	uint32_t mask			/* Attribute mask to change */
)
{
#if defined(USE_GIT_FAT_FS)
	return (eGitFresult)f_chmod(path,value,mask);
#elif defined(USE_GIT_LITTLE_FS)
	return GIT_FR_OK;
#endif
}

void git_directory_list()
{
#if defined(USE_GIT_FAT_FS)
	directory_list();
#elif defined(USE_GIT_LITTLE_FS)
	
	stLittleFsFileInfo Dir;
	eGitFresult res;
	struct lfs_info Finfo;
	UINT s1 = 0, s2 = 0;
	long p1 = 0;
	//FATFS *fs;
	char* strScanDirectory = DIR_ROOT;

	memset(&Finfo, (int) NULL, sizeof(Finfo));

	res = git_f_opendir(&Dir, strScanDirectory);

	if (res) {
		return;
	}

	printf("\r\n");
	//printf("fs: current directory - [%s]\r\n", DIR_ROOT);
	for(;;) {
		res = git_f_readdir(&Dir, &Finfo);
		if ((res != GIT_FR_OK) || !Finfo.name[0])
			break;
		if (Finfo.type & LFS_TYPE_DIR) {
			s2++;
		}
		else {
			s1++;
			p1 += Finfo.size;
		}
		printf("%c %9lu  %s ",  (Finfo.type & LFS_TYPE_DIR) ? 'D' : '-',
								Finfo.size, &(Finfo.name[0]));
		printf("\r\n");
	}
	printf("\r\n%4u File(s),%10lu bytes total%4u Dir(s)\r\n", s1, p1, s2);
	git_f_closedir(&Dir);
#if 0
	char *InBuff=0;
	uint16_t iBuffLength = 0;
	UINT dwFileSize2;
	eGitFresult ret = GIT_FR_OK;
	stFileSystemDescript fpVer;
	if ( (ret = git_f_open(&fpVer, "test2.txt", GIT_FA_EXIST | GIT_FA_RD_WR)) == GIT_FR_OK ) {
		if ((ret = git_f_read(&fpVer, InBuff, (UINT)iBuffLength, (UINT *)&dwFileSize2)) == GIT_FR_OK ) {
			git_f_close(&fpVer);
			Trace("SYS: read size: %d\n", dwFileSize2);
			if(iBuffLength == dwFileSize2) {
				return;
			}
			else {
				return;
			}
		}
		else {
			git_f_close(&fpVer);
			Trace("SYS: %s File Read fail %d\r\n", "test2.txt", ret);
			return;
		}
	}
#endif
	return;
#endif
}
void git_directory_select(char* strScanDirectory)
{
#if defined(USE_GIT_FAT_FS)
	DirectoryList(strScanDirectory);
#elif defined(USE_GIT_LITTLE_FS)
	stLittleFsFileInfo Dir;
	eGitFresult res;
	struct lfs_info Finfo;
	UINT s1 = 0, s2 = 0;
	long p1 = 0;
	memset(&Finfo, (int) NULL, sizeof(Finfo));
	res = git_f_opendir(&Dir, strScanDirectory);
	if (res) {
		return;
	}
	printf("\r\n");
	//printf("fs: current directory - [%s]\r\n", strScanDirectory);

	for(;;) {
		res = git_f_readdir(&Dir, &Finfo);
		if ((res != GIT_FR_OK) || !Finfo.name[0])
			break;

		if (Finfo.type & LFS_TYPE_DIR) {
			s2++;
		}
		else {
			s1++;
			p1 += Finfo.size;
		}

		printf("%c %9lu  %s ",  (Finfo.type & LFS_TYPE_DIR) ? 'D' : '-',
								Finfo.size, &(Finfo.name[0]));
		printf("\r\n");
	}

	printf("\r\n%4u File(s),%10lu bytes total%4u Dir(s)\r\n", s1, p1, s2);
	git_f_closedir(&Dir);
	
#if 0
	char *InBuff=0;
	uint16_t iBuffLength = 0;
	UINT dwFileSize2;
	eGitFresult ret = GIT_FR_OK;
	stFileSystemDescript fpVer;

	if ( (ret = git_f_open(&fpVer, "test2.txt", GIT_FA_EXIST | GIT_FA_RD_WR)) == GIT_FR_OK ) {
		if ((ret = git_f_read(&fpVer, InBuff, (UINT)iBuffLength, (UINT *)&dwFileSize2)) == GIT_FR_OK ) {
			git_f_close(&fpVer);

			Trace("SYS: read size: %d\n", dwFileSize2);
			if(iBuffLength == dwFileSize2) {
				return;
			}
			else {
				return;
			}
		}
		else {
			git_f_close(&fpVer);
			Trace("SYS: %s File Read fail %d\r\n", "test2.txt", ret);

			return;
		}
	}

	
#endif
	
	return;

#endif
}


eGitFresult git_f_open (
	void *fp,			/* Pointer to the blank file object */
	const TCHAR *path,	/* Pointer to the file name */
	uint32_t mode			/* Access mode and file open mode flags */
)
{
#if defined(USE_GIT_FAT_FS)
  
	stFatFsFileInfo* pstFp = fp;
  
	return (eGitFresult)f_open((FIL*)&pstFp->file,path,mode);
#elif defined(USE_GIT_LITTLE_FS)

	stLittleFsFileInfo* pstFp = fp;
#ifdef FILE_LOG
	printf("git_f_open: %s\r\n",path);
#endif
	int32_t nRet = lfs_file_open(GetLittleFSDescript(), (lfs_file_t*)&pstFp->file, path, mode);

	if( nRet == 0 )
		return GIT_FR_OK;

	//printf("%s] filename :%s, error nRet : %d\r\n", __FUNCTION__, path, nRet);

	return (eGitFresult)nRet;
#endif
}

eGitFresult git_f_read (
	void *fp, 		/* Pointer to the file object */
	void *buff,		/* Pointer to data buffer */
	uint32_t btr,		/* Number of bytes to read */
	uint32_t *br		/* Pointer to number of bytes read */
)
{
#if defined(USE_GIT_FAT_FS)
	stFatFsFileInfo* pstFp = fp;
  
	return (eGitFresult)f_read((FIL*)&pstFp->file,buff,btr,br);
#elif defined(USE_GIT_LITTLE_FS)

	stLittleFsFileInfo* pstFp = fp;

	*br = lfs_file_read(GetLittleFSDescript(), (lfs_file_t*)&pstFp->file, buff, btr);

	if( *br == btr )	return GIT_FR_OK;
	else				return (eGitFresult)*br;
#endif
}

eGitFresult git_f_write (
	void *fp,			/* Pointer to the file object */
	const void *buff,	/* Pointer to the data to be written */
	uint32_t btw,			/* Number of bytes to write */
	uint32_t *bw			/* Pointer to number of bytes written */
)
{
#if defined(USE_GIT_FAT_FS)
	stFatFsFileInfo* pstFp = fp;
	
	return (eGitFresult)f_write((FIL*)&pstFp->file,buff,btw,bw);

#elif defined(USE_GIT_LITTLE_FS)

	stLittleFsFileInfo* pstFp = fp;

	*bw = lfs_file_write(GetLittleFSDescript(), (lfs_file_t*)&pstFp->file, buff, btw);

	if( *bw == btw )	return GIT_FR_OK;
	else				return (eGitFresult)*bw ;
#endif
}


eGitFresult git_f_close (
	void *fp 	/* Pointer to the file object to be closed */
)
{
#if defined(USE_GIT_FAT_FS)
	stFatFsFileInfo* pstFp = fp;

	return (eGitFresult)f_close((FIL*)&pstFp->file);

#elif defined(USE_GIT_LITTLE_FS)

	stLittleFsFileInfo* pstFp = fp;

	int32_t nRet = lfs_file_close(GetLittleFSDescript(), (lfs_file_t*)&pstFp->file);

	if( nRet == 0 )
		return GIT_FR_OK;

	return (eGitFresult)nRet;
#endif
}

eGitFresult git_f_closedir(
	void *dj 	/* Pointer to the file object to be closed */
)
{
#if defined(USE_GIT_FAT_FS)

	return GIT_FR_NOT_SUPPORT;


#elif defined(USE_GIT_LITTLE_FS)


	stLittleFsFileInfo* pstFp = dj;

	int32_t nRet = lfs_dir_close(GetLittleFSDescript(), (lfs_dir_t*)&pstFp->dir);

	if( nRet == 0 )
		return GIT_FR_OK;

	return (eGitFresult)nRet;
#endif
}



eGitFresult git_f_truncate (
	void *fp,		/* Pointer to the file object */
	int32_t nSize
)
{
#if defined(USE_GIT_FAT_FS)
	stFatFsFileInfo* pstFp = fp;

	return (eGitFresult)f_truncate((FIL*)&pstFp->file);

#elif defined(USE_GIT_LITTLE_FS)

	stLittleFsFileInfo* pstFp = fp;

	lfs_file_truncate(GetLittleFSDescript(), (lfs_file_t*)&pstFp->file, nSize);


	return GIT_FR_OK;
#endif
}


eGitFresult git_f_unlink (
	const TCHAR *path		/* Pointer to the file or directory path */
)
{
#if defined(USE_GIT_FAT_FS)
	return (eGitFresult)f_unlink(path);
#elif defined(USE_GIT_LITTLE_FS)

	int32_t nRet = lfs_remove(GetLittleFSDescript(), path);

	if( nRet == 0 )
		return GIT_FR_OK;

	//printf("%s] Error : %d\r\n", __FUNCTION__, nRet);

	return (eGitFresult)nRet;
#endif
}

eGitFresult git_f_opendir (
	void *dj,			/* Pointer to directory object to create */
	const TCHAR *path	/* Pointer to the directory path */
)
{
#if defined(USE_GIT_FAT_FS)
	
	stFatFsFileInfo* pstFp = dj;
	
	return (eGitFresult)f_opendir((DIR*)&pstFp->dir,path);

#elif defined(USE_GIT_LITTLE_FS)

	stLittleFsFileInfo* pstFp = dj;

	int32_t nRet = lfs_dir_open(GetLittleFSDescript(), (lfs_dir_t*)&pstFp->dir, path);

	if( nRet == 0 )
		return GIT_FR_OK;

	//printf("%s] error nRet : %d\r\n", __FUNCTION__, nRet);

	return (eGitFresult)nRet;
#endif
}


eGitFresult git_f_readdir (
	void *dj,			/* Pointer to the open directory object */
	void *fno		/* Pointer to file information to return */
)
{
#if defined(USE_GIT_FAT_FS)
	stFatFsFileInfo* pstFp = dj;

	return (eGitFresult)f_readdir((DIR*)&pstFp->dir,(FILINFO*)fno);

#elif defined(USE_GIT_LITTLE_FS)

	stLittleFsFileInfo* pstFp = dj;

	int nRet = lfs_dir_read(GetLittleFSDescript(), (lfs_dir_t*)&pstFp->dir, (struct lfs_info*)fno);


	if( nRet == true )
		return GIT_FR_OK;


	if( nRet == 0 )
		nRet = GIT_FR_NO_PATH;
	else
		printf("%s] Error Ret : %d\r\n", __FUNCTION__, nRet);

	return (eGitFresult)nRet;

#endif

}


eGitFresult git_f_getfree (
	const TCHAR *path,	/* Pointer to the logical drive number (root dir) */
	int32_t *nclst,		/* Pointer to the variable to return number of free clusters */
	void **fatfs		/* Pointer to pointer to corresponding file system object to return */
)
{
#if defined(USE_GIT_FAT_FS)
	return (eGitFresult)f_getfree(path, (DWORD*)&nclst, (FATFS**)&fatfs);
#elif defined(USE_GIT_LITTLE_FS)
	return GIT_FR_OK;
#endif
}

eGitFresult git_f_lseek (
	void *fp,		/* Pointer to the file object */
	uint32_t ofs		/* File pointer from top of file */
)
{
#if defined(USE_GIT_FAT_FS)
	stFatFsFileInfo* pstFp = fp;
	return (eGitFresult)f_lseek((FIL*)&pstFp->file,ofs);
#elif defined(USE_GIT_LITTLE_FS)

	//f_read 일때만 git_f_lseek 사용해야 한다. littlefs 특성상 write시 seek 사용하면 seek 시점부터 파일끝까지 다시 다 쓴다. 느려진다
	stLittleFsFileInfo* pstFp = fp;


	int32_t nOffset = lfs_file_seek(GetLittleFSDescript(),(lfs_file_t*)&pstFp->file,ofs,0);

	if( nOffset == ofs )
	{
		return GIT_FR_OK;
	}

	//printf("%s] Error : %d\r\n", __FUNCTION__, nOffset);
	return (eGitFresult)nOffset;
#endif
}

eGitFresult git_f_sync (
	void *fp		/* Pointer to the file object */
)
{
#if defined(USE_GIT_FAT_FS)
	stFatFsFileInfo* pstFp = fp;
	return (eGitFresult)f_sync((FIL*)&pstFp->file);
#elif defined(USE_GIT_LITTLE_FS)

	stLittleFsFileInfo* pstFp = fp;

	int32_t nRet = lfs_file_sync(GetLittleFSDescript(), (lfs_file_t*)&pstFp->file);
	if( nRet == 0 )
		return GIT_FR_OK;

	return (eGitFresult)nRet;
#endif
}

eGitFresult git_f_rename (
	const TCHAR *path_old,	/* Pointer to the old name */
	const TCHAR *path_new	/* Pointer to the new name */
)
{
#if defined(USE_GIT_FAT_FS)
	return (eGitFresult)f_rename(path_old,path_new);
#elif defined(USE_GIT_LITTLE_FS)

	//lfs_file_t file;
	//lfs_file_open(GetLittleFSDescript(), &file, path_old, LFS_O_RDONLY|LFS_O_EXCL);
	lfs_rename(GetLittleFSDescript(), path_old, path_new);

	return GIT_FR_OK;
#endif
}

int32_t git_f_size (
	void *fp
)
{
#if defined(USE_GIT_FAT_FS)
	stFatFsFileInfo* pstFp = fp;

	return f_size((FIL*)&pstFp->file);

#elif defined(USE_GIT_LITTLE_FS)

	stLittleFsFileInfo* pstFp = fp;

	return lfs_file_size(GetLittleFSDescript(), (lfs_file_t*)&pstFp->file);

#endif
}



#if defined(USE_GIT_FAT_FS)
extern FATFS g_Fatfs[_VOLUMES];
#endif
void TestFormat()
{

#if defined(USE_GIT_FAT_FS)
	if (f_mount(FAT_VOLUME_DRV, &g_Fatfs[0]) == FR_OK)
	{
		Trace("FS: Fat File System mount Ok\r\n");
	}
    else
    {
        Trace("FS: Error Fat File System mount Ok\r\n");
    }

    if ( f_mkfs(FAT_VOLUME_DRV, 0, 4096) == FR_OK )
    {
        Trace("FS: format success\r\n");

        if ( f_chdir(DIR_ROOT) == FR_OK )
        {
            Trace("FS: Create default folder\r\n");
            f_mkdir(DIR_LOG);
            f_mkdir(DIR_FIRMWARE);
            f_mkdir(DIR_INFORMATION);
            directory_list();
        }
        else
        {
            Trace("FS: FR_NOK\r\n");
        }
    }
    else
    {
        Trace("FS: format fail\r\n");
    }
	
#elif defined(USE_GIT_LITTLE_FS)

	#include "LfsPorting.h"

	extern stFileSystemInfo m_stFileSystemInfo;

	if( git_f_format((uint32_t)NULL,&m_stFileSystemInfo.stLfs) == GIT_FR_OK )
	{
		printf("format success\r\n");
	}

	if( git_f_mount((uint32_t)NULL,&m_stFileSystemInfo.stLfs) == GIT_FR_OK )
	{
	}

	git_f_mkdir(DIR_LOG);
	git_f_mkdir(DIR_FIRMWARE);
	git_f_mkdir(DIR_INFORMATION);
	git_f_mkdir(AUTOLINKDATA_BASE_DIR);
	git_directory_list();


#endif

}





