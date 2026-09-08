
/**
  ******************************************************************************
  * @file    LfsPorting.c
  * @author  GIT Application Team by hkmoon
  * @version V1.1.0
  * @date    05-SEP-2013
  * @brief   Header for LfsPorting.c module
  ******************************************************************************
 **/
#include "LfsPorting.h"
#include "GIT_Util.h"
#include "AutolinkConfig.h"
#include "SysHalFileSystem.h"
#include "HalHandler.h"



int lfs_flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
	//printf("read block : %d, blcok size \n",block,c->block_size);
	//printf("read addr : %x, read size : %d\n",( c->block_size * block + off),size);
	//HalHalDrvSPIRead(DRV_ID_SPI, ( c->block_size * block + off), 0, (char *)buffer, size,0);
	sFLASH_ReadBuffer(buffer, ( c->block_size * block + off), size);
	//hexdump((char*)buffer,size);
	//memcpy(buffer, &s_flashmem[0] + c->block_size * block + off, size);
	return 0;
}

int lfs_flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
	//printf("write block : %d, blcok size \n",block,c->block_size);
	//hexdump((char*)buffer,size);
	//HalHalDrvSPIWrite(DRV_ID_SPI, (int)( block * c->block_size + off), 0,(char *)buffer, size,0);
	sFLASH_WriteBuffer((uint8_t *) buffer, (int)( block * c->block_size + off), size);
	//memcpy(&s_flashmem[0] + block * c->block_size + off, buffer, size);
	return 0;
}

int lfs_flash_erase(const struct lfs_config *c, lfs_block_t block)
{
	//printf("erase block : %d, blcok size \n",block,c->block_size);
	//printf("erase addr : %x\n",block * c->block_size);
    //HalHandlerIOCtrl(DRV_ID_SPI, eSpiEraseSubSector, (block * c->block_size),NULL,0,0);
	sFLASH_EraseSubSector(block * c->block_size);
	//memset(&s_flashmem[0] + block * c->block_size, 0, c->block_size);
	return 0;
}

int lfs_flash_sync(const struct lfs_config *c) {
  (void) c;
  return 0;
}


stFileSystemInfo m_stFileSystemInfo;

//#define LITTLE_FS_BLOCK_SIZE SFLASH_SECTOR_SIZE
#define LITTLE_FS_BLOCK_SIZE 256

#define s_flashSize 8*1024*1024


bool LfsInit()
{
	//lfs_file_t file;
	//char buff[5];

	m_stFileSystemInfo.bActive = false;

	memset(&m_stFileSystemInfo.stLfs.fs, 0, sizeof(lfs_t));
	memset(&m_stFileSystemInfo.stLfs.cfg, 0, sizeof(struct lfs_config));
	m_stFileSystemInfo.stLfs.cfg.read	= lfs_flash_read;
	m_stFileSystemInfo.stLfs.cfg.prog	= lfs_flash_prog;
	m_stFileSystemInfo.stLfs.cfg.erase = lfs_flash_erase;
	m_stFileSystemInfo.stLfs.cfg.sync	= lfs_flash_sync;

	m_stFileSystemInfo.stLfs.cfg.read_size = LITTLE_FS_BLOCK_SIZE;
	m_stFileSystemInfo.stLfs.cfg.prog_size = LITTLE_FS_BLOCK_SIZE;
	m_stFileSystemInfo.stLfs.cfg.block_size = SFLASH_SECTOR_SIZE;
	m_stFileSystemInfo.stLfs.cfg.block_count = ((s_flashSize-MAX_CONFIG_STORAGE_SIZE) / (SFLASH_SECTOR_SIZE));
	//s_cfg.lookahead = 128; // v1
	m_stFileSystemInfo.stLfs.cfg.cache_size = LITTLE_FS_BLOCK_SIZE; //v2
	m_stFileSystemInfo.stLfs.cfg.lookahead_size = LITTLE_FS_BLOCK_SIZE; // v2
	m_stFileSystemInfo.stLfs.cfg.block_cycles = 500;

	//sFLASH_EraseBulk();

	unsigned long s_ulTimeCheck = Get_Tmr();

	if (lfs_mount(&m_stFileSystemInfo.stLfs.fs, &m_stFileSystemInfo.stLfs.cfg) < 0)
	{
		printf("mount fail\n");

		if (lfs_format(&m_stFileSystemInfo.stLfs.fs, &m_stFileSystemInfo.stLfs.cfg) < 0)
		{
			printf("format fail\n");

			return false;
		}

		if (lfs_mount(&m_stFileSystemInfo.stLfs.fs, &m_stFileSystemInfo.stLfs.cfg) < 0)
		{
			return false;
		}

		//if ( git_f_chdir(DIR_ROOT) == FR_OK )
		{
			printf("FS: Create default foler\r\n");
			git_f_mkdir(DIR_LOG);
			git_f_mkdir(DIR_FIRMWARE);
			git_f_mkdir(DIR_INFORMATION);
			git_f_mkdir(AUTOLINKDATA_BASE_DIR);
			git_directory_list();
		}
	}
	printf("lfs_mount time : %d\n", Get_Tmr()-s_ulTimeCheck);
	s_ulTimeCheck = Get_Tmr();

	m_stFileSystemInfo.bActive = true;

//	git_f_mkdir(DIR_LOG);
//	git_f_mkdir(DIR_FIRMWARE);
//	git_f_mkdir(DIR_INFORMATION);
//	git_f_mkdir(AUTOLINKDATA_BASE_DIR);
	git_directory_list();

	return true;
}

lfs_t* GetLittleFSDescript()
{
	if( m_stFileSystemInfo.bActive == true )
		return &m_stFileSystemInfo.stLfs.fs;

	return (lfs_t*)NULL;
}
