/**
  ******************************************************************************
  * @file    SysHalFileSystem.h
  * @author  GIT Application Team by hkmoon
  * @version V1.1.0
  * @date    05-SEP-2013
  * @brief   Header for SysHalFileSystem.h module
  ******************************************************************************
 **/
#ifndef __SYS_HAL_FILE_SYSTEM__
#define __SYS_HAL_FILE_SYSTEM__

#if !defined(FEATURE_BOOTLOADER)
#include "LfsPorting.h"
#endif

//#ifdef USE_GIT_FAT_FS
#include "ff.h"
//#endif

#include "MngStorage.h"
#include "common.h"



//#define MAX_FILE_NAME_LENGTH 32


//#define USE_GIT_LITTLE_FS
//#define USE_GIT_FAT_FS


#ifdef USE_GIT_LITTLE_FS


typedef enum eAutolinkFileOpenMode{
	GIT_FA_READ 		= LFS_O_RDONLY,
	GIT_FA_WRITE 		= LFS_O_WRONLY,
	GIT_FA_RD_WR 		= LFS_O_RDWR,
	GIT_FA_CREATE_NEW 	= LFS_O_CREAT|LFS_O_EXCL,
	GIT_FA_CREATE_ALWAYS= LFS_O_CREAT|LFS_O_TRUNC,
	GIT_FA_OPEN_ALWAYS 	= LFS_O_CREAT,		//LFS_O_CREAT|LFS_O_APPEND,
	GIT_FA_EXIST		= 0,
	GIT_FA_TRUNC 		= LFS_O_TRUNC,
	GIT_FA_APPEND 		= LFS_O_APPEND,
}eAutolinkFileOpenMode;


// File seek flags
typedef enum eAutolinkSeekFlag{
    GIT_FS_SEEK_SET = LFS_SEEK_SET,
    GIT_FS_SEEK_CUR = LFS_SEEK_CUR,
    GIT_FS_SEEK_END = LFS_SEEK_END,
}eAutolinkSeekFlag;

/* Type of path name strings on FatFs API */

#if _LFN_UNICODE			/* Unicode string */
#if !_USE_LFN
#error _LFN_UNICODE must be 0 in non-LFN cfg.
#endif

#if false 
#ifndef _INC_TCHAR
typedef WCHAR TCHAR;
#define _T(x) L ## x
#define _TEXT(x) L ## x
#endif

#else						/* ANSI/OEM string */
#ifndef _INC_TCHAR
typedef char TCHAR;
#define _T(x) x
#define _TEXT(x) x
#endif

#endif
#endif 

#endif //#ifdef USE_GIT_LITTLE_FS



#ifdef USE_GIT_FAT_FS

typedef enum eAutolinkFileOpenMode{
	GIT_FA_READ 		= FA_READ,				// Specifies read access to the object. Data can be read from the file.
	GIT_FA_WRITE 		= FA_WRITE	,			// Specifies write access to the object. Data can be written to the file. Combine with FA_READ for read-write access.
	GIT_FA_RD_WR 		= LFS_O_RDWR,
	GIT_FA_CREATE_NEW 	= FA_CREATE_NEW,		// Creates a new file. The function fails with FR_EXIST if the file is existing.
	GIT_FA_CREATE_ALWAYS= FA_CREATE_ALWAYS,		// Creates a new file. If the file is existing, it will be truncated and overwritten.
	GIT_FA_OPEN_ALWAYS 	= FA_OPEN_ALWAYS,		// Opens the file if it is existing. If not, a new file will be created.
	GIT_FA_EXIST 		= FA_OPEN_EXISTING,
	GIT_FA_TRUNC 		= LFS_O_TRUNC,
	GIT_FA_APPEND 		= LFS_O_APPEND,
};


// File seek flags
typedef enum eAutolinkSeekFlag{
    GIT_FS_SEEK_SET = LFS_SEEK_SET,
    GIT_FS_SEEK_CUR = LFS_SEEK_CUR,
    GIT_FS_SEEK_END = LFS_SEEK_END,
};
#endif //#ifdef USE_GIT_FAT_FS



typedef enum __eGitFresult{
	GIT_FR_OK = 0,				/* (0) Succeeded */
	GIT_FR_DISK_ERR,			/* (1) A hard error occured in the low level disk I/O layer */
	GIT_FR_INT_ERR,				/* (2) Assertion failed */
	GIT_FR_NOT_READY,			/* (3) The physical drive cannot work */
	GIT_FR_NO_FILE,				/* (4) Could not find the file */
	GIT_FR_NO_PATH,				/* (5) Could not find the path */
	GIT_FR_INVALID_NAME,		/* (6) The path name format is invalid */
	GIT_FR_DENIED,				/* (7) Acces denied due to prohibited access or directory full */
	GIT_FR_EXIST,				/* (8) Acces denied due to prohibited access */
	GIT_FR_INVALID_OBJECT,		/* (9) The file/directory object is invalid */
	GIT_FR_WRITE_PROTECTED,		/* (10) The physical drive is write protected */
	GIT_FR_INVALID_DRIVE,		/* (11) The logical drive number is invalid */
	GIT_FR_NOT_ENABLED,			/* (12) The volume has no work area */
	GIT_FR_NO_FILESYSTEM,		/* (13) There is no valid FAT volume on the physical drive */
	GIT_FR_MKFS_ABORTED,		/* (14) The f_mkfs() aborted due to any parameter error */
	GIT_FR_TIMEOUT,				/* (15) Could not get a grant to access the volume within defined period */
	GIT_FR_LOCKED,				/* (16) The operation is rejected according to the file shareing policy */
	GIT_FR_NOT_ENOUGH_CORE,		/* (17) LFN working buffer could not be allocated */
	GIT_FR_TOO_MANY_OPEN_FILES,	/* (18) Number of open files > _FS_SHARE */
	GIT_FR_NOT_SUPPORT,
} eGitFresult;

typedef  __packed struct __stSaveFileHeader
{
    uint32_t readedCount;
    uint32_t writedCount;
    uint32_t unCheckSum;
}stSaveFileHeader;

typedef __packed struct __stGitFileDescript
{
	stSaveFileHeader stFileHeader;
	int32_t nFileId;
	uint8_t ucArrFileName[MAX_FILE_NAME_LENGTH];
	boolean_t bActive;
	void* vpFileDescript;
}stGitFileDescript;

typedef __packed struct __stGitFileInfo
{
	uint32_t unFileId;
	uint32_t unSize;
	boolean_t bActive;
}stGitFileInfo;

typedef __packed struct __stGitFileSystemDescript
{
	uint8_t ucDrive;
	uint32_t unFileSytemId;
	boolean_t bActive;
}stGitFileSystemDescript;

typedef __packed struct __stGitDirDescript
{
	uint32_t unFileId;
	uint8_t ucArrFileName[MAX_FILE_NAME_LENGTH];
	boolean_t bActive;
}stGitDirDescript;

#ifdef USE_GIT_LITTLE_FS
typedef __packed struct __stLittleFsFileInfo{
	lfs_file_t file;
	lfs_dir_t dir;
}stLittleFsFileInfo;
#endif

#ifdef USE_GIT_FAT_FS
typedef __packed struct __stFatFsFileInfo{
	FIL file;
	DIR dir;
}stFatFsFileInfo;
#endif

#if !defined(FEATURE_BOOTLOADER)
typedef __packed union __stFileSystemDescript
{
#ifdef USE_GIT_FAT_FS
stFatFsFileInfo stFatFsInfo;
#endif
#ifdef USE_GIT_LITTLE_FS
stLittleFsFileInfo stLittleFsInfo;
#endif
}stFileSystemDescript;
#endif



boolean_t InitFileSystem(void);

eGitFresult git_disk_write(
	uint32_t drv,			/* Physical drive number (0) */
	const uint8_t *buff,	/* Pointer to the data to be written */
	uint32_t sector,		/* Start sector number (LBA) */
	uint32_t count			/* Sector count (1..255) */
);

eGitFresult git_disk_read(
	uint32_t drv,			/* Physical drive number (0) */
	uint8_t *buff,			/* Pointer to the data buffer to store read data */
	uint32_t sector,		/* Start sector number (LBA) */
	uint32_t count			/* Sector count (1..255) */
);

eGitFresult git_f_mount (
	uint32_t vol,		/* Logical drive number to be mounted/unmounted */
	void *fs		/* Pointer to new file system object (NULL for unmount)*/
);

eGitFresult git_f_umount (
	uint32_t vol,		/* Logical drive number to be mounted/unmounted */
	void *fs		/* Pointer to new file system object (NULL for unmount)*/
);

eGitFresult git_f_format(
	uint32_t vol,		/* Logical drive number to be mounted/unmounted */
	void *fs		/* Pointer to new file system object (NULL for unmount)*/
);

eGitFresult git_f_mkfs(
		uint32_t drv,		/* Logical drive number */
		uint32_t sfd,		/* Partitioning rule 0:FDISK, 1:SFD */
		uint32_t au 		/* Allocation unit size [bytes] */
);

eGitFresult git_f_chdir (
	const TCHAR *path	/* Pointer to the directory path */
);

eGitFresult git_f_mkdir (
	const TCHAR *path		/* Pointer to the directory path */
);


eGitFresult git_f_chmod (
	const TCHAR *path,	/* Pointer to the file path */
	uint32_t value,			/* Attribute bits */
	uint32_t mask			/* Attribute mask to change */
);

void git_directory_list();

eGitFresult git_f_open (
	void *fp,			/* Pointer to the blank file object */
	const TCHAR *path,	/* Pointer to the file name */
	uint32_t mode			/* Access mode and file open mode flags */
);

eGitFresult git_f_read (
	void *fp, 		/* Pointer to the file object */
	void *buff,		/* Pointer to data buffer */
	uint32_t btr,		/* Number of bytes to read */
	uint32_t *br		/* Pointer to number of bytes read */
);

eGitFresult git_f_write (
	void *fp,			/* Pointer to the file object */
	const void *buff,	/* Pointer to the data to be written */
	uint32_t btw,			/* Number of bytes to write */
	uint32_t *bw			/* Pointer to number of bytes written */
);

eGitFresult git_f_close (
	void *fp 	/* Pointer to the file object to be closed */
);

eGitFresult git_f_truncate (
	void *fp,		/* Pointer to the file object */
	int32_t nSize
);

eGitFresult git_f_unlink (
	const TCHAR *path		/* Pointer to the file or directory path */
);


eGitFresult git_f_opendir (
	void *dj,			/* Pointer to directory object to create */
	const TCHAR *path	/* Pointer to the directory path */
);

eGitFresult git_f_readdir (
	void *dj,			/* Pointer to the open directory object */
	void *fno		/* Pointer to file information to return */
);

eGitFresult git_f_closedir (
	void *dj			/* Pointer to the open directory object */
);

eGitFresult git_f_getfree (
	const TCHAR *path,	/* Pointer to the logical drive number (root dir) */
	int32_t *nclst,		/* Pointer to the variable to return number of free clusters */
	void **fatfs		/* Pointer to pointer to corresponding file system object to return */
);

eGitFresult git_f_lseek (
	void *fp,		/* Pointer to the file object */
	uint32_t ofs		/* File pointer from top of file */
);

eGitFresult git_f_sync (
	void *fp		/* Pointer to the file object */
);

eGitFresult git_f_rename (
	const TCHAR *path_old,	/* Pointer to the old name */
	const TCHAR *path_new	/* Pointer to the new name */
);

int32_t git_f_size (
	void *fp
);

void TestFormat();

#endif // __SYS_HAL_FILE_SYSTEM__
