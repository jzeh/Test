/*************************************************************
 * NOTE : git_record.c
 *      
 * Author : 
 * Since : 2019.12.18
**************************************************************/
#include <string.h>

#include "git_PassthruDefines.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

#include "common.h"
#include "firmware.h"
#include "git_protocol.h"
#include "sw_timer.h"
#include "buzzer.h"
#include "git_fsutil.h"
#include "git_rtc.h"
#include "git_ioctl.h"

#include "git_record.h"
#include "git_record_file.h"

#include "ff.h"

//MONI #include "PassThruStruct.h"

#define MAX_RECORED_HEADER_SIZE (4)

#define TMP_BUFF_SIZE (1024)

#define MAX_FILE_TMP_BUFF_SIZE	(1024)

#define FATS_1G_MARGINE_SIZE 	(1*1024*1024*1024)
#define FATS_50M_MARGINE_SIZE 	(50*1024*1024)
#define FATS_10M_MARGINE_SIZE 	(10*1024*1024)
#define FATS_5M_MARGINE_SIZE 	(5*1024*1024)
#define FATS_3M_MARGINE_SIZE 	(3*1024*1024)
#define FATS_1M_MARGINE_SIZE 	(1*1024*1024)

//#define FATS_100K_MARGINE_SIZE 	(100*1024)



/*----------------------------------------------------------------------*/



extern stRecordingControl m_stRecordingControl;
extern FRESULT formatEmmc( void );


uint32_t ReadFile(uint8_t* pcFileName, uint8_t* pcBuffer, uint32_t unAdjustAddress, uint32_t unLength)
{
	FIL CFilepnt;
	uint32_t unReadCnt=0;
	uint8_t uRet=0;

	if( pcFileName == NULL )
	{
		GLogE("error : Read file name is null\r\n");
		return 0;
	}

#if false
	
	FIL stRecFile;	
	FIL stTempFile;
	FILINFO fno;
	

	if( f_stat(pcFileName,&fno) == FR_OK )
	{		
		printf("%s] config file size : %d\r\n", __func__, fno.fsize);
	}
	
#endif
	uRet = f_open(&CFilepnt, (char const*)pcFileName, FA_OPEN_EXISTING | FA_READ);
	//if (f_open(&CFilepnt, (char const*)pcFileName, FA_OPEN_EXISTING | FA_READ) == FR_OK )
	if ( uRet == FR_OK )
	{
		GLogI("%s File Open Success\r\n", pcFileName);
		
		if(f_lseek(&CFilepnt, unAdjustAddress) == FR_OK)
		{
			if((f_read(&CFilepnt, (void*)pcBuffer, unLength, &unReadCnt)) != FR_OK)
			{
				GLogE("error read file\r\n");
			}
		}
		else
		{
			GLogE("error lseek file\r\n");
		}
		
		f_close(&CFilepnt);
	}
	else
	{
		GLogE("error open file[%s],%d\r\n",pcFileName,uRet);
	}

	return unReadCnt;
}

void file_hex_dump(FIL* pstRecFile)
{
	uint32_t unReadCount;
	U8	stTempBuff[TMP_BUFF_SIZE] = {0,};
	
	f_lseek(pstRecFile, 0);

	int nQ = f_size(pstRecFile)/TMP_BUFF_SIZE;
	int nR = f_size(pstRecFile)%TMP_BUFF_SIZE;

	GLogI("%s] file size : %d\r\n", __func__, f_size(pstRecFile));

	for(int i=0; i<nQ;i++)
	{
		if(f_read(pstRecFile, (void*)stTempBuff, TMP_BUFF_SIZE , &unReadCount) == FR_OK)
		{
			hexdump(stTempBuff, unReadCount);
		}
	}

	if(f_read(pstRecFile, (void*)stTempBuff, nR , &unReadCount) == FR_OK)
	{
		hexdump(stTempBuff, unReadCount);
	}
}

uint8_t m_stTempBuff[TMP_BUFF_SIZE];

void DistributionDataFileofRecoredingTime(bool bForce)
{
	// save backup data every 4*1024 bytes or remained backup data
	FIL stRecFile;	
	FIL stTempFile;
	FILINFO fno;
	

	uint32_t unMaxDataCnt=0;
	uint32_t i=0,j=0,k=0;
	uint32_t unReadCount=0,unReadCount2=0;

	GLogI("start\r\n");

	if( f_stat(STORE_REC_TEMP_FILE_NAME,&fno) == FR_OK )
	{		
		GLogI("file size : %d \r\n", fno.fsize);

		// error over 500MB
		if( fno.fsize > FATS_1G_MARGINE_SIZE )
		{
			f_unlink(STORE_REC_TEMP_FILE_NAME);

			ClearBkRam();

			GLogI("exception case file was deleted, initialize index\r\n");
		}
	}
	else
	{
		GLogE("error file didn't exist\r\n");
		return;
	}


#ifdef ENABLE_TEST_SHORT_RECORD
	if( fno.fsize >= 3*1024 )	
#else
	//if( fno.fsize >= FATS_10M_MARGINE_SIZE || bForce == true)
	if( fno.fsize >= FATS_5M_MARGINE_SIZE || bForce == true)
#endif
	{
		GLogI("start file migration\r\n");
		GLogI("used time : %d, diff time : %d\r\n", m_stRecordingControl.stDataCtrl.unUsedRecTime/1000, (Get_Tmr() - m_stRecordingControl.stDataCtrl.unUsedRecTime)/1000);

		int nMargineTime = 2;
		int nMaxMargineTime = 10;
		int nDiffTimeSecond = (int)((Get_Tmr()-m_stRecordingControl.stDataCtrl.unUsedRecTime)/1000);

		//MONI 20230419 static analysis num : 34 / for checking devide by 0
		if( nDiffTimeSecond == 0 )
		{
			// set defualt 1 seconds.
			nDiffTimeSecond = 1;
		}

		
		// 3seconds margine measure data value
		// record file size : 1256536, recorded time : 637, record count for 1 seconds : 1972
		// adjust data count : 1183200  --> 10mins
		// max margin size 5Mbytes for 30mins

		// 2seconds margine measure data value with optimizing trigger delay
		// record file size : 1091200, recorded time : 588, record count for 1 seconds : 1855
		// adjust data count : 1113000  --> 10mins
		// max margin size 5Mbytes for 30mins
//MONI 20230106 block adjust value for stable data size.
#if false
		// adjust margin time beccause of dtc and waiting time //// 3 seconds every 10 seconds 
		nDiffTimeSecond = nDiffTimeSecond - ((nDiffTimeSecond/nMaxMargineTime)*nMargineTime);		
#endif

		//1ms * recording time from config 
		unMaxDataCnt = (float)((fno.fsize/nDiffTimeSecond)*m_stRecordingControl.stDataCtrl.unMaxRecordingTime);

		GLogE("record file size : %d, ori recorded time : %d, adj recorded time : %d, record count for 1 seconds : %d\r\n", fno.fsize, (int)((Get_Tmr()-m_stRecordingControl.stDataCtrl.unUsedRecTime)/1000), nDiffTimeSecond, fno.fsize/nDiffTimeSecond);
		GLogE("adjust data count : %d\r\n", unMaxDataCnt);

		
		if (f_open(&stRecFile, STORE_REC_TEMP_FILE_NAME, FA_OPEN_EXISTING | FA_READ | FA_WRITE ) == FR_OK )
		{		
			if(f_open(&stTempFile, STORE_REC_NEW_TEMP_FILE_NAME, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
			{
#ifdef ENABLE_TEST_SHORT_RECORD				
				unMaxDataCnt = 2*1123;
#endif

				//** adjust recording time becuase of file size was reduced. **
				m_stRecordingControl.stDataCtrl.unUsedRecTime = Get_Tmr()-(m_stRecordingControl.stDataCtrl.unMaxRecordingTime*1000);
				printf("%s] adjust time : %d, diff time : %d\r\n", __func__, m_stRecordingControl.stDataCtrl.unUsedRecTime/1000, nDiffTimeSecond);


				if( fno.fsize < unMaxDataCnt )
					unMaxDataCnt = fno.fsize;
				
				i = unMaxDataCnt/TMP_BUFF_SIZE;
				j = unMaxDataCnt%TMP_BUFF_SIZE;

				if(f_lseek(&stRecFile, fno.fsize - unMaxDataCnt) != FR_OK)
				{
					GLogE("error f_lssek : %s\r\n", STORE_REC_TEMP_FILE_NAME);
				}
				
				if(f_lseek(&stTempFile, 0) != FR_OK)
				{
					GLogE("error f_lssek : %s\r\n", STORE_REC_NEW_TEMP_FILE_NAME);
				}

				// copy all buffer size data
				for(k=0; k<i; k++)
				{
					if(f_read(&stRecFile, (void*)m_stTempBuff, TMP_BUFF_SIZE , &unReadCount) == FR_OK)
					{
						if ( f_write(&stTempFile, (void*)m_stTempBuff, TMP_BUFF_SIZE, &unReadCount2) != FR_OK )
						{	
							GLogI("Write Fail..r\n");	
						}
					}

					//hexdump(m_stTempBuff,unReadCount);

					if( unReadCount != unReadCount2 || unReadCount != TMP_BUFF_SIZE )
					{
						GLogI("Step#1 Write Fail..r\n"); 
					}
				}

				//GLogI("%s] remained size : %d\r\n", __func__, j);

				// copy remained size data
				if(f_read(&stRecFile, (void*)m_stTempBuff, j, &unReadCount) == FR_OK)
				{
					if ( f_write(&stTempFile, (void*)m_stTempBuff, j, &unReadCount2) != FR_OK )
					{	
						GLogI("Write Fail..\r\n");	
					}

					//hexdump(m_stTempBuff,unReadCount);

					if( unReadCount != unReadCount2 || unReadCount != j )
					{
						GLogI("Step#2 Write Fail..r\n"); 
					}
				}
				
				if((f_close(&stRecFile)!=FR_OK)||(f_close(&stTempFile)!=FR_OK))
				{	
					GLogI("File Close FAIL!\r\n");	
				}

				
				f_unlink(STORE_REC_TEMP_FILE_NAME);
				//scan_files("/");

				// rename temp file to recording file
				if(f_rename(STORE_REC_NEW_TEMP_FILE_NAME, STORE_REC_TEMP_FILE_NAME)==FR_OK)
				{
					GLogI("2 %s File Create OK!\r\n",STORE_REC_TEMP_FILE_NAME);
					f_unlink(STORE_REC_NEW_TEMP_FILE_NAME);
				}
				else
				{
					GLogI("2%s File Creat FAIL!","New_TEMP.REC\r\n");
				}

				//scan_files("/");
			}
			else
			{
				GLogI("NEW_TEMP.REC FILE OPEN FAIL!!\r\n");
			}
		}
		else
		{
			GLogI("TEMP.REC FILE OPEN FAIL!!\r\n");
		}
	}
}

void DisplayFileData(char* pFileName)
{
	// save backup data every 4*1024 bytes or remained backup data
	FIL stRecFile;
  	uint32_t unReadCount=1;
	FILINFO fno;

	GLogI("%s] display file : %s\r\n", __func__, pFileName);

	f_chdir(DIR_RECORD);

	if( f_stat(pFileName,&fno) == FR_OK )
	{		
		GLogI("%s] file size : %d \r\n", __func__, fno.fsize);

		if( fno.fsize > 100*1024*1024 )
		{
			f_unlink(pFileName);

			formatEmmc();

			GLogI("%s] exception case file was deleted, initialize index\r\n", __func__);
		}
	}

	
	if (f_open(&stRecFile, pFileName, FA_OPEN_EXISTING | FA_READ) != FR_OK )
	{
		GLogI("%s] error file open : %s\r\n", __func__, pFileName);
		return;
	}

	char buffer[512];
	
	
	if(f_lseek(&stRecFile,0) == FR_OK)
	{
		while(unReadCount!=0)
		{
			if ( f_read(&stRecFile, (void*)buffer, 512, &unReadCount) != FR_OK )
			{	
				GLogI("Write Fail..\r\n");	
			}
			
			if( unReadCount < 512 )
				hexdump((uint8_t*)buffer,unReadCount);
		}
		
	}
	
	if(f_close(&stRecFile)!= FR_OK)
	{	
		GLogI("File Close FAIL!\r\n");	
	}
}

void SaveBackupData2File(bool bForce)
{
	// save backup data every 4*1024 bytes or remained backup data
	FIL stRecFile;
  	uint32_t unReadCount;
	FRESULT stResult;
	
	if( isAvailableBkRam() == true && bForce == false )
		return;

	//GLogI("%s] save backup buffer to temporary file\r\n", __func__);
        
	f_chdir(DIR_RECORD);

	stResult = f_open(&stRecFile, STORE_REC_TEMP_FILE_NAME, FA_OPEN_EXISTING | FA_WRITE);
	
	if( stResult != FR_OK )
	{
		GLogE("%s] error file broken : %d, name : %s\r\n", __func__, stResult, STORE_REC_TEMP_FILE_NAME);
				// file broken;
		f_unlink(STORE_REC_TEMP_FILE_NAME);

		stResult = f_open(&stRecFile, STORE_REC_TEMP_FILE_NAME, FA_CREATE_NEW | FA_WRITE);
		if( stResult != FR_OK )
		{
			GLogE("%s] error file open :%d, name : %s\r\n", __func__, stResult, STORE_REC_TEMP_FILE_NAME);
			GLogE("%s] error file open :%d, name : %s\r\n", __func__, stResult, STORE_REC_TEMP_FILE_NAME);
			GLogE("%s] error file open :%d, name : %s\r\n", __func__, stResult, STORE_REC_TEMP_FILE_NAME);

			// file broken;
			f_unlink(STORE_REC_TEMP_FILE_NAME);
			
			// test code for stable
			deinitMMC();
			osDelay(10);
			initMMC();

			GLogI("%s] exception case file was created initialize index1\r\n", __func__);

			return;
		}
		else
		{
			// MONI exception case 
			GLogI("%s] exception case file was created initialize index2\r\n", __func__);
		}
	}
	
	if(f_lseek(&stRecFile, f_size(&stRecFile)) == FR_OK)
	{
		if ( f_write(&stRecFile, (void*)m_stRecordingControl.stDataCtrl.ucBackupData, m_stRecordingControl.stDataCtrl.usBackupDataPosition, &unReadCount) != FR_OK )
		{	
			GLogI("Write Fail..\r\n");	
		}

		if( m_stRecordingControl.stDataCtrl.usBackupDataPosition != unReadCount )
		{
			GLogE("%s] error file wirte\r\n", __func__);
		}
	}

	GLogI("%s] file size : %d\r\n", __func__, f_size(&stRecFile));
	
	if(f_close(&stRecFile)!= FR_OK)
	{	
		GLogI("File Close FAIL!\r\n");	
	}

	ClearBkRam();
}


void MakeFlightRecordFile()
{
	uint8_t ucSaveFileName[64]={0,};
	FRESULT nResult;
	FILINFO fno;
	
	f_chdir(DIR_RECORD);
	
	ConvertFRRtc2FileName(ucSaveFileName);

	GLogI("%s] recored file name : %s\r\n", __func__, ucSaveFileName);


	//RecordingTime : 10min(600000)  intRecCount : Timer 변수 
	if( m_stRecordingControl.stDataCtrl.unMaxRecordingTime <= (uint32_t)((Get_Tmr()-m_stRecordingControl.stDataCtrl.unUsedRecTime)/1000) )
	{
		//splite the recording file according with record time because file is large  better then recording time
		DistributionDataFileofRecoredingTime(true);
	}

	// make recording file -> rename temp recording file to current time recording file.
	if((nResult = f_rename(STORE_REC_TEMP_FILE_NAME,(char const*)ucSaveFileName))==FR_OK)
	{
		GLogI("%s File Create OK!\r\n", ucSaveFileName);
	}
	else
	{
		GLogI("%s File Write FAIL! %d\r\n", ucSaveFileName, nResult);
	}

	if( f_stat(ucSaveFileName,&fno) != FR_OK )
	{		
		GLogE("%s] error f_stat\r\n", __func__);
	}


	GLogI("%s] get_tmr : %d, used record time : %d\r\n", __func__, Get_Tmr(), m_stRecordingControl.stDataCtrl.unUsedRecTime);

	GLogI("[1.Recording Time   =%d s]\r\n", (uint32_t)((Get_Tmr()-m_stRecordingControl.stDataCtrl.unUsedRecTime)/1000));
	GLogI("[1.Record Max Time  =%d s]\r\n", m_stRecordingControl.stDataCtrl.unMaxRecordingTime);
	GLogI("[1.Record Size      =%d bytes]\r\n", fno.fsize);	

	f_unlink(STORE_REC_TEMP_FILE_NAME);
	
	// show temp data
	//DisplayFileData((char*)ucSaveFileName);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////

bool UpdateRecordHeader(uint8_t* pucHeader, uint16_t usLength, uint32_t* punDataSize)
{
	if( pucHeader == NULL )
	{
		GLogI("%s] error buffer is null\r\n", __func__);
		return false;
	}

	pucHeader[0] = 0xF5;
	pucHeader[1] = 0xF6;
	pucHeader[2] = 0xF7;
	
	if(m_stRecordingControl.bIsMultiRecord == TRUE)
	{
		//151116 lwh Rxdata Size(+3는 index)
	  	pucHeader[3] = (uint8_t)usLength+3;
	}
	else
	{
		// CHJ 2013-09-25 Rxdata Size(+2는 index)
		pucHeader[3] = (uint8_t)usLength+2;
	}
	
	// adjust header index
	if(m_stRecordingControl.stCanCtrl.ucCurCommIndex == 0)
	{
		//first index add header prefix
		*punDataSize = MAX_RECORED_HEADER_SIZE;
	}
	else
	{
		//over seconds index add just size in header.
		*punDataSize = 1;
		// save only size
		pucHeader[0] = pucHeader[3];
		pucHeader[1] = 0x00;
		pucHeader[2] = 0x00;
		pucHeader[3] = 0x00;
	}

	return true;	
}

void UpdateRecordIndex(uint8_t* pucIndex, uint32_t* punLength)
{
	uint32_t unRecvSize = m_stRecordingControl.stCanCtrl.stTxPacket.DataSize;
	
	// update index
	if( m_stRecordingControl.bIsMultiRecord == true )
	{
		pucIndex[0] = m_stRecordingControl.stCanCtrl.stTxPacket.pData[unRecvSize-3];
		pucIndex[1] = m_stRecordingControl.stCanCtrl.stTxPacket.pData[unRecvSize-2];
		pucIndex[2] = m_stRecordingControl.stCanCtrl.stTxPacket.pData[unRecvSize-1];
		//pucIndex[0] = SBYTE_ARRAYBuff_test[DlcTxBlock].BytePtr[unRecvSize-3];		// INDEX 1
		//pucIndex[1] = SBYTE_ARRAYBuff_test[DlcTxBlock].BytePtr[unRecvSize-2];		// INDEX 2
		//pucIndex[2] = SBYTE_ARRAYBuff_test[DlcTxBlock].BytePtr[unRecvSize-1];		// INDEX 3
		*punLength = 3;
	}
	else
	{
		pucIndex[0] = m_stRecordingControl.stCanCtrl.stTxPacket.pData[unRecvSize-2];
		pucIndex[1] = m_stRecordingControl.stCanCtrl.stTxPacket.pData[unRecvSize-1];
		//pucIndex[0] = SBYTE_ARRAYBuff_test[DlcTxBlock].BytePtr[unRecvSize-2];		// INDEX 1
		//pucIndex[1] = SBYTE_ARRAYBuff_test[DlcTxBlock].BytePtr[unRecvSize-1];		// INDEX 2
		*punLength = 2;
	}
}

void MakeRecordPacket(uint8_t* pucPacket, uint32_t* punLength, uint8_t* pucRxBuffer, uint8_t unRxLength)
{
	uint8_t ucTmpBuffer[8]={0x00,};
	uint32_t unPacketLength = 0;
	uint32_t unTemLength;
	
	//update Record header
	UpdateRecordHeader(ucTmpBuffer, unRxLength, &unPacketLength);

	// header
	memcpy(pucPacket,ucTmpBuffer, unPacketLength);


	// data
	memcpy(&pucPacket[unPacketLength], pucRxBuffer, unRxLength);
	unPacketLength+=unRxLength;

	
	memset(ucTmpBuffer,0,sizeof(ucTmpBuffer));
	
	// tx size
	UpdateRecordIndex(ucTmpBuffer, &unTemLength);

	// data
	memcpy(&pucPacket[unPacketLength], ucTmpBuffer, unTemLength);
	unPacketLength+=unTemLength;

	// update packet length
	*punLength = unPacketLength;	
}

void ConvertFRRtc2FileName(uint8_t* pcFileName)
{
	uint8_t RTCData[15]={0,};
	
	Get_RTCData(RTCData);
#if false
	// year high
	AsciiPtr = DecToHexToAscii(20);
	pcFileName[ 0] = *AsciiPtr;
	pcFileName[ 1] = *(AsciiPtr+1);

	// year low
	DecToHexToAscii(RTCData[ 1]);
	pcFileName[ 2] = *AsciiPtr;
	pcFileName[ 3] = *(AsciiPtr+1);
	
	// month
	DecToHexToAscii(RTCData[ 2]);	
	pcFileName[ 4] = *AsciiPtr;		
	pcFileName[ 5] = *(AsciiPtr+1);
	
	// day
	DecToHexToAscii(RTCData[ 3]);	
	pcFileName[ 6] = *AsciiPtr;
	pcFileName[ 7] = *(AsciiPtr+1);

	pcFileName[ 8] = '-';

	// hour
	DecToHexToAscii(RTCData[4]);
	
	if(*AsciiPtr+8 >= 0x3A)
	{
		*AsciiPtr = *AsciiPtr+0x0F;
		pcFileName[ 9] = *AsciiPtr;				
	}
	else
	{	
		pcFileName[ 9] = *AsciiPtr+8;	
	}
		
	pcFileName[10] = *(AsciiPtr+1);

	// min
	DecToHexToAscii(RTCData[ 5]);		
	pcFileName[11] = *AsciiPtr;
	pcFileName[12] = *(AsciiPtr+1);

	// sec
	DecToHexToAscii(RTCData[ 6]);		
	pcFileName[13] = *AsciiPtr;
	pcFileName[14] = *(AsciiPtr+1);

#else

	// year high
	DecToAscii(20, &pcFileName[0]);
	// year low
	DecToAscii(RTCData[1], &pcFileName[2]);	
	// month
	DecToAscii(RTCData[2], &pcFileName[4]);	
	// day
	DecToAscii(RTCData[3], &pcFileName[6]);	
	pcFileName[8] = '-';

	// hour
	DecToAscii(RTCData[4], &pcFileName[9]);
	
	if( (pcFileName[9] + 8) >= 0x3A)
	{
		pcFileName[9] = pcFileName[9] + 0x0F;
	}
	else
	{	
		pcFileName[ 9] = pcFileName[9] + 8;	
	}
		
	// min
	DecToAscii(RTCData[5], &pcFileName[11]);
	// sec
	DecToAscii(RTCData[6], &pcFileName[13]);

#endif
	
	pcFileName[15] = '.';
	pcFileName[16] = 'R';
	pcFileName[17] = 'E';
	pcFileName[18] = 'C';
		
}


void InitializeTempFile()
{
	// save backup data every 4*1024 bytes or remained backup data
	FIL stRecFile;
	FRESULT stResult;
	FILINFO fno;
	
	memset(&fno, 0, sizeof(FILINFO));

	f_chdir(DIR_RECORD);

	if( f_stat(STORE_REC_TEMP_FILE_NAME,&fno) == FR_OK )
	{
	  	f_chdir(DIR_ROOT);
	  	return;
	}
	else
	{	
		GLogE("%s] file didn't exist\r\n", __func__);

		f_unlink(STORE_REC_TEMP_FILE_NAME);

		stResult = f_open(&stRecFile, STORE_REC_TEMP_FILE_NAME, FA_CREATE_NEW | FA_WRITE);
		if( stResult != FR_OK )
		{
			GLogE("%s] error file open :%d, name : %s\r\n", __func__, stResult, STORE_REC_TEMP_FILE_NAME);
			GLogE("%s] error file open :%d, name : %s\r\n", __func__, stResult, STORE_REC_TEMP_FILE_NAME);
			GLogE("%s] error file open :%d, name : %s\r\n", __func__, stResult, STORE_REC_TEMP_FILE_NAME);

			// file broken;
			f_unlink(STORE_REC_TEMP_FILE_NAME);
			
			// test code for stable
			deinitMMC();
			osDelay(10);
			initMMC();

			GLogI("%s] exception case file was created initialize index3\r\n", __func__);
			
			f_chdir(DIR_ROOT);
			return;
		}
		else
		{
			// MONI exception case 
			GLogI("%s] exception case file was created initialize index4\r\n", __func__);
		}
		
		if(f_close(&stRecFile)!= FR_OK)
		{	
			GLogI("File Close FAIL!\r\n");	
		}
	}
	
	f_chdir(DIR_ROOT);
}

FRESULT scan_files(char* path)
{
    FRESULT res;
    FILINFO fno;
    DIR dir;
    int i;
    char *fn;
	long p1 = 0;
	UINT s1=0,s2=0;
	FATFS *fs;

    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        i = strlen(path);
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) 
			{
			  	f_closedir(&dir);
				break;  /* Break on error or end of dir */
			}
            if (fno.fname[0] == '.') continue;             /* Ignore dot entry */
			if (fno.fname[0] == '..') continue;             /* Ignore dot entry */
            fn = fno.fname;
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                //sprintf(&path[i], "/%s", fn);
				memcpy(&path[i], fn, strlen(fn)+1);
                res = scan_files(path);
                if (res != FR_OK) break;
                //path[i] = 0;
				s2++;
            } else {                                       /* It is a file. */
                //GLogI("%s/%s\r\n", path, fn);
				s1++; 
				p1 += fno.fsize;
            }

			GLogI("%s] %c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s/%s\r\n", __func__,
				(fno.fattrib & AM_DIR) ? 'D' : '-',
				(fno.fattrib & AM_RDO) ? 'R' : '-',
				(fno.fattrib & AM_HID) ? 'H' : '-',
				(fno.fattrib & AM_SYS) ? 'S' : '-',
				(fno.fattrib & AM_ARC) ? 'A' : '-',
				(fno.fdate >> 9) + 1980, (fno.fdate >> 5) & 15, fno.fdate & 31,
				(fno.ftime >> 11), (fno.ftime >> 5) & 63,	fno.fsize, path, &(fno.fname[0]));
			GLogI("\r\n");
        }
    }

	GLogI("\r\n%4u File(s),%10lu bytes total%4u Dir(s)", s1, p1, s2);


	//formatEmmc();

    return res;
}

