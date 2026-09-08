/* Includes ------------------------------------------------------------------*/
#if defined(USE_GIT_FAT_FS)
#include "ff.h"
#endif

#include "HdDebug.h"
#include "DebugHandler.h"
#include "Git_Util.h"
#include "AutolinkConfig.h"
#include "SysHalFileSystem.h"
#include "SPIDriverMicron.h"

#define SFLASH_SECTOR_SIZE (4*1024)

#define Trace(...)  GITDebug(DEBUG_MODULES_SYSTEM,__VA_ARGS__)

typedef struct __stAutolinkHeader
{
    int32_t nItemCount;
}stAutolinkHeader;

typedef struct __stAutolinkErrorCodeInfo
{
    uint32_t nErrorCode;
}stAutolinkErrorCodeInfo;

typedef struct __stAutolinkDebug
{
    stAutolinkHeader header;
    stAutolinkErrorCodeInfo errorcode;
}stAutolinkDebug;

stAutolinkDebug m_stAutolinkDebug;

int32_t WriteErrorCode(int32_t nErrorCode)
{
    int32_t nReadErrorCode;
    char carrBuf[SFLASH_SECTOR_SIZE] = {0,};
    //git_disk_read(FAT_VOLUME_DRV, (BYTE*)carrBuf, SECTOR_DEBUG_STORAGE_BACKUP, 1);    
	sFLASH_ReadBuffer((uint8_t*)&carrBuf, SECTOR_DEBUG_STORAGE_BACKUP * SFLASH_SECTOR_SIZE, sizeof(carrBuf));

    for(int i=0;i<(SFLASH_SECTOR_SIZE/4);i++)
    {
        memcpy((char*)&nReadErrorCode,(char*)&carrBuf[i*4],4);
        if( nReadErrorCode == -1 )
        {
            sFLASH_WriteBuffer((uint8_t*)&nErrorCode, (SECTOR_DEBUG_STORAGE_BACKUP * SFLASH_SECTOR_SIZE)+i*4, 4);
            return i;
        }
    }

    return -1;
}

void ReadErrorCode()
{
    int32_t i;
    int32_t nReadErrorCode;
    char carrBuf[SFLASH_SECTOR_SIZE] = {0,};
    //git_disk_read(FAT_VOLUME_DRV, (BYTE*)carrBuf, SECTOR_DEBUG_STORAGE_BACKUP, 1);    
	sFLASH_ReadBuffer((uint8_t*)&carrBuf, SECTOR_DEBUG_STORAGE_BACKUP * SFLASH_SECTOR_SIZE, sizeof(carrBuf));

    Trace("================================================\r\n");
    Trace(" Error Code List \n");
    Trace("================================================\r\n");
    for(i=0;i<(SFLASH_SECTOR_SIZE/4);i++)
    {        
        memcpy((char*)&nReadErrorCode,(char*)&carrBuf[i*4],4);
        Trace("Error Code : %x\n",nReadErrorCode);        
        if( nReadErrorCode == -1 )
        {
            break;
        }
    }

    Trace("Error Code Count : %d\n",i);
}

void ClearErrorCode()
{    
    int i,count = 1;

    for (i = 1; i <= count; i++)
    {
        sFLASH_EraseSubSector(SECTOR_DEBUG_STORAGE_BACKUP * i * SFLASH_SECTOR_SIZE);
    }
}
