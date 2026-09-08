/**
  ******************************************************************************
  * @file    ManagerSystem.h
  * @author  GIT Connectivity Development 2 Team
  * @version V1.0.0
  * @date    20-Dec-2017
  * @brief   Header for ManagerSystem.c module
  ******************************************************************************
 **/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MANAGER_STORAGE_H__
#define __MANAGER_STORAGE_H__

#define MAX_FILE_NAME_LENGTH 256
//#define FILE_LOG

typedef struct _stAutolinkFileInfo{
    char m_carrLatestFullFilePath[MAX_FILE_NAME_LENGTH];
    char m_carrLatestFilePath[MAX_FILE_NAME_LENGTH];
    char m_carrLatestFolderPath[MAX_FILE_NAME_LENGTH];
}stAutolinkFileInfo;

enum{
    FILE_MODE_READ,
    FILE_MODE_READ_AND_DELETE,
    FILE_MODE_FIND_FOLDER,
    FILE_MODE_FIND_FILE,
    FILE_MODE_DELETE,
};

int32_t MngStorage(/*int lparam, int rparam*/);

#endif //__MANAGER_STORAGE_H__

/***************************** END OF FILE ****/
