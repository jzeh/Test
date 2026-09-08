/**
  ******************************************************************************
  * @file    LfsPorting.h
  * @author  GIT Application Team by hkmoon
  * @version V1.1.0
  * @date    05-SEP-2013
  * @brief   Header for LfsPorting.h module
  ******************************************************************************
 **/
#ifndef __LFS_PORTING_H__
#define __LFS_PORTING_H__

#include "common.h"

#include "lfs.h"
#include "lfs_util.h"


typedef struct __stLittleFS{
	lfs_t fs;
	struct lfs_config cfg;
}stLittleFS;

typedef struct __stFileSystemInfo{
	stLittleFS stLfs;
	bool bActive;
}stFileSystemInfo;

lfs_t* GetLittleFSDescript();



bool LfsInit();

#endif // __LFS_PORTING_H__
