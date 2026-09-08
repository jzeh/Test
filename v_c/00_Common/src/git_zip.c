#include "zlib.h"
#include "fatfs.h"
#include "FreeRTOS.h"
#include "common.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdlib.h>
#include "git_protocol.h"
#include "git_mmc.h"
#include "git_fsutil.h"
#include "git_function_list.h"
#include "git_pool.h"

#pragma location = ".dtcm"
static z_stream strm __attribute__((section(".dtcm")));

#pragma location = ".dtcm"
static uint8_t g_dtcm_pool[40 * 1024] __attribute__((section(".dtcm")));

static size_t g_dtcm_off = 0u;

bool decompressFile(const char* compressedFilename, const char* decompressedFilename);
void *zlib_alloc(void *opaque, unsigned int items, unsigned int size);
void zlib_free(void *opaque, void *address);
static void *zlib_dtcm_alloc(void *opaque, unsigned int items, unsigned int size);
static void zlib_dtcm_free(void *opaque, void *addr);

extern void deinitRS9116();
extern osThreadId		hOBDcanTxTh;
extern osThreadId		hOBDcanRxTh;
extern osThreadId		hTransmitFDTh;
extern osThreadId       defaultTaskHandle;
extern osThreadId       hParsingTh;
extern void uartCliThread();
extern osThreadId		hTransmitFDTh;
extern void parsingThread( void const *argument );
extern char         g_strDownloadFW_FileName[25];

osMutexId hDecompressMutex;
osMessageQId    hDecompressionMsg;

#define CHUNK 4096

unsigned char in[CHUNK];
unsigned char out[CHUNK];

bool decompressFile(const char* compressedFilename, const char* decompressedFilename) {
    //GLogN( "Start %s\r\n", __FUNCTION__ );
    
    FIL compressedFile;
    int ret;
    
    ret = f_open( &compressedFile, compressedFilename, FA_OPEN_EXISTING | FA_READ);
    if (ret != FR_OK) {
        GLogN("Error opening compressed file.\n\r");
        return false;
    }

    FIL decompressedFile;
    ret = f_open( &decompressedFile, decompressedFilename, FA_WRITE | FA_READ | FA_CREATE_ALWAYS);
    if (ret != FR_OK) {
        GLogN("Error opening decompressed file.\n\r");
        f_close( &compressedFile );
        return false;
    }

    /* Set Buffer size, if you set large size, you need to set more memory. *
     * However, if you set small size, you need to more time decompressing. */
    
    unsigned have;

    /* zlib structure initializing. */
    strm.zalloc = zlib_dtcm_alloc;
    strm.zfree = zlib_dtcm_free;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    
    uint32_t bytesRead, bytesWritten=0;

    int decompressCnt = 0;

    /* Inflate Fuction initializing.                    *
     * you need to have 40kByte memory in heap area.    *
     * Because It requset 32k buffer size.              */
    ret = inflateInit( &strm );
    //ret = inflateInit2(&strm, -MAX_WBITS);
    if (ret != Z_OK) {
        GLogN("Not Zip File\n\r");
        f_close( &compressedFile );
        f_close( &decompressedFile );
        return true;
    }

    /* Read data from File. And, Do inflate Algorithm. */
    do {
        strm.avail_in = f_read( &compressedFile, in, CHUNK, &bytesRead );
        if ( f_error( &compressedFile ) ) {
            inflateEnd(&strm);
            f_close( &compressedFile );
            f_close( &decompressedFile );
            return false;
        }
        strm.avail_in = bytesRead;
        
        
        if (strm.avail_in == 0) break;
        if( decompressCnt == 100)
        {
            GLogN(".");
            decompressCnt = 0;
        }
        strm.next_in = in;
        decompressCnt++;

        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            if (ret == Z_STREAM_ERROR) {
                inflateEnd(&strm);
                f_close( &compressedFile );
                f_close( &decompressedFile );
                GLogN("Error, While decompressing...\n\r");
                return false;
            }
            switch (ret) {
                case Z_NEED_DICT:
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    inflateEnd(&strm);
                    f_close( &compressedFile );
                    f_close( &decompressedFile );
                    return false;
            }
            have = CHUNK - strm.avail_out;
            f_write( &decompressedFile, out, have, &bytesWritten);
            if ( bytesWritten != have || f_error( &decompressedFile ) ) {
                inflateEnd(&strm);
                f_close( &compressedFile );
                f_close( &decompressedFile );
                return false;
            }
        } while (strm.avail_out == 0);
    } while (ret != Z_STREAM_END);
    /* End of inflate */
    inflateEnd(&strm);

    
    int ReadByte = 0;
    FRESULT fResult;
    uint16_t Checksum16value = 0;
    uint32_t Filesize = 0;

    Filesize = f_size(&decompressedFile);

    if ( f_lseek(&decompressedFile, 0) == FR_OK )
    {
        for(int i=0; i<Filesize; i =  i + 1024)
        {
            fResult = f_read(&decompressedFile, (void*)&in, 1024, &ReadByte);
            if ( fResult != FR_OK ) GLogE( "File Read fail\r\n");
                
            for(int j = 0; j < ReadByte; ++j) {
                Checksum16value += in[j];
            }
        }
    }

    extern  uint32_t g_u32UpdateFileSize;
    extern uint32_t g_u32CheckSumTemp;
    g_u32CheckSumTemp = Checksum16value;
    g_u32UpdateFileSize = Filesize;
    
    f_close( &compressedFile );
    f_close( &decompressedFile );

    if (ret != Z_STREAM_END) {
        GLogN("Error during decompression.\n\r");
        return false;
    }

    GLogN("\r\nDecompression complete. Results saved to %s\n\r", decompressedFilename);
    
    f_unlink(compressedFilename);
    f_rename(decompressedFilename, compressedFilename);
    GLogN( "%s rename %s\r\n", decompressedFilename, compressedFilename);

    return true;
}

void *zlib_alloc(void *opaque, unsigned int items, unsigned int size) {
    return pvPortMalloc(items * size);
}

void zlib_free(void *opaque, void *address) {
    vPortFree(address);
}

static void *zlib_dtcm_alloc(void *opaque, unsigned int items, unsigned int size)
{
    size_t need = (size_t)items * size;
    need = (need + 7u) & ~7u;

    if (g_dtcm_off + need > sizeof(g_dtcm_pool))
    {
		GLogE("%d, %d : inflateInit memory pool fail!!", g_dtcm_off + need, sizeof(g_dtcm_pool));
        return NULL;
    }

    void *p = &g_dtcm_pool[g_dtcm_off];
    g_dtcm_off += need;
    return p;
}

static void zlib_dtcm_free(void *opaque, void *addr)
{
    (void)opaque;
    (void)addr;
	g_dtcm_off = 0;
}


void DecompressionThread( void const *argument )
{
    //const char* compressedFilename = "cvci3_Bootloader.bin";
    //const char* decompressedFilename = "result.bin";
    osEvent	evt;
    //UBaseType_t uxHighWaterMark;    /* Inspect our own high water mark on entering the task. */   
    U32 uiOldtime;
    bool bRet = false;
    bool bFlagDeleteThread = false;
    U16         u16Temp=0;
    FRESULT     fResult;
    
    osMutexDef( DecompressMutex );
    hDecompressMutex = osMutexCreate( osMutex( DecompressMutex ) );

    osMessageQDef( decompression, MESSAGE_DECOMPRESSION_QUEUE_SIZE, int );
    hDecompressionMsg = osMessageCreate( osMessageQ( decompression ), NULL );

	for( ;; )
	{
        /* Test zip */
        evt = osMessageGet( hDecompressionMsg, osWaitForever );
        if( evt.status == osEventMessage )
		{
            if(osMutexWait(hDecompressMutex, osWaitForever) == osOK)
            {
//                if(!bFlagDeleteThread)
//                {
//                    /* Task Kill */
//                    extern osThreadId       hOBDcanTxTh;
//                    extern osThreadId       hOBDcanRxTh;
//                    extern osThreadId       hTransmitFDTh;
//                    extern osThreadId       defaultTaskHandle;
//                    extern osThreadId       uartCliTh;
//                    extern osThreadId       hTCPTxTh;
//                    extern osThreadId       hTCPRxTh;
//                    extern osThreadId       hListDiagTh;
//                    //extern void DecompressionThread( void const *argument );
//                    
//                    GLogN("Delete Threads...\r\n");
//                    if( osThreadGetState( uartCliTh ) != osThreadDeleted )                 osThreadTerminate(uartCliTh);
//                    if( osThreadGetState( hOBDcanTxTh ) != osThreadDeleted )               osThreadTerminate(hOBDcanTxTh);
//                    if( osThreadGetState( hOBDcanRxTh ) != osThreadDeleted )               osThreadTerminate(hOBDcanRxTh);
//                    if( osThreadGetState( defaultTaskHandle ) != osThreadDeleted )         osThreadTerminate(defaultTaskHandle);
//                    if( osThreadGetState( hTransmitFDTh ) != osThreadDeleted )             osThreadTerminate(hTransmitFDTh);
//
//                    osDelay(30); //When Threads do osThreadTerminate, Need to Memory Setting Time.
//                    bFlagDeleteThread = true;
//                    /* End of Task Kill */
//                }
            
                GLogN( "Start %s\r\n", __FUNCTION__ );

                extern int myrecvDeltaTime;
                GLogN("@@DeltaTime : %d", Get_TmrDelta( Get_Tmr(), myrecvDeltaTime ));
            
                //osThreadTerminate(hParsingTh);
                osDelay(30); //When Threads do osThreadTerminate, Need to Memory Setting Time.

                /* D060 Git_FWUpdate_Check Function */
                uiOldtime = Get_Tmr();
                GLogN("Checksum DeltaTime : %d \r\n", Get_TmrDelta( Get_Tmr(), uiOldtime ) );
                uiOldtime = Get_Tmr();
                extern uint32_t CalCRC32(void);
                U32 u32Temp = CalCRC32();
                GLogN("CRC32 DeltaTime : %d \r\n", Get_TmrDelta( Get_Tmr(), uiOldtime ) );
                fResult = DownloadClose();
                
                uiOldtime = Get_Tmr();
                
                bRet = decompressFile(g_strDownloadFW_FileName, "DecompressTemp.bin");
                
                /* Retry */
                if(!bRet)
                {
                    bRet = decompressFile(g_strDownloadFW_FileName, "DecompressTemp.bin");

                    if(!bRet)
                    {
                        FIL errorFile;
                        f_open( &errorFile, "errorzipFile", FA_CREATE_ALWAYS | FA_WRITE);
                        f_close( &errorFile );
                    }
                }
                
                GLogN("Decompress DeltaTime : %d \r\n", Get_TmrDelta( Get_Tmr(), uiOldtime ) );

                stCommPkt   *pkt;
                stMsgClst   *message;
                extern BOOL g_bWifiAutoConDisable;
                extern BOOL		g_bCsDiffFlag;
                extern eLockState 	g_eLockStatus;
            	unsigned char cResult = 0x00;
                if(LOCK_GET_STATE()!=eLOCK_STATE_UNLOCK) return;

                g_bWifiAutoConDisable=0;

            	message	= ( stMsgClst* )osPoolCAlloc( hMsgPool );
            	if( message == NULL )
            	{
            		return;
            	}
            	pkt	= ( stCommPkt* )osPoolCAlloc( hCommPKPool );
            	if( pkt == NULL )
            	{
            		osPoolFree( hMsgPool, (void *)message );
            		return;
            	}
                memset( pkt, 0x00, sizeof(stCommPkt) );
                memset( message, 0x00, sizeof(stMsgClst) );

                //pkt = &mpkt;

                pkt->mLen	= 4;
                pkt->pTarget	= &evt.value.v;
                pkt->mFuncID = 0x0456;

                pkt->mData[3] = u32Temp >> 24 & 0xFF;
                pkt->mData[2] = u32Temp >> 16 & 0xFF;
                pkt->mData[1] = u32Temp >> 8 & 0xFF;
                pkt->mData[0] = u32Temp & 0xFF;
                //memcpy(pkt->mData, &u32Temp, sizeof(U32));

            	pkt->mCS 	= CalcChecksumGITPtclPayloadFrame(pkt);
                
            	message->mPktType	= (ePKT_TD)evt.value.v;
            	message->pPacket	= (void *)pkt;

            	if(osMessageAvailableSpace(hTransmitMsg) == 0)
            	{
            		osPoolFree( hCommPKPool, (void *)pkt );
            		osPoolFree( hMsgPool, (void *)message );
            	}
            	else
            	{
            		osMessagePut( hTransmitMsg, (uint32_t)message, osWaitForever );
            	}

                f_chdir(DIR_ROOT);
                osMutexRelease(hDecompressMutex);
            }
        }
	}
}


