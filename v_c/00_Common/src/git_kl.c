/*************************************************************
 * NOTE : git_kl.c
 *      K-Line, L-line Control
 * Author : Lee junho
 * Since : 2021.11.15
**************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "gpio.h"
#include "cmsis_os.h"

#include "main.h"
#include "usart.h"

#include "common.h"
#include "typedef.h"
#include "git_ioctl.h"

#include "git_kl.h"
#include "git_vci.h"

#include "sw_timer.h"
#include "git_OBDcomm.h"
/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#define KL_LINE_UART_PORT1									huart1
#define KL_LINE_UART_PORT2									huart3

#define KL_LINE_UART_PORT1_IRQ								USART1_IRQn
#define KL_LINE_UART_PORT2_IRQ								USART3_IRQn

#define KL_LINE_STREAM_BUFFER_SIZE							200

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
uint32_t getKLReceiveData_9141( uint8_t line, uint8_t *data, uint32_t len, uint32_t timeout);
/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
StreamBufferHandle_t	hSBKLRx1;
StreamBufferHandle_t	hSBKLRx2;

uint8_t				gucKL1RxDummy;
uint8_t				gucKL2RxDummy;
uint8_t g_ucTxLen;
uint8_t g_ucTxbuff[255];

extern u32	intDlccomCount;
extern BOOL	g_bAckflag;
BOOL g_bRxIng = true;
extern BOOL g_bIsAutoVIN;

/*----------------------------------------------------------------------
 *   Functions definition
 *--------------------------------------------------------------------*/
int32_t InitKL( void )
{
	// create streamBuffer
	hSBKLRx1 = xStreamBufferCreate( KL_LINE_STREAM_BUFFER_SIZE, 100 );
	if( hSBKLRx1 == NULL )
	{
		return INIT_FAIL;
	}

	hSBKLRx2 = xStreamBufferCreate( KL_LINE_STREAM_BUFFER_SIZE, 100 );
	if( hSBKLRx2 == NULL )
	{
		return INIT_FAIL;
	}

	Disable_KL_Interrupt( KL_LINE1 );
	Disable_KL_Interrupt( KL_LINE2 );

	HAL_UART_Receive_IT( &KL_LINE_UART_PORT1, &gucKL1RxDummy, 1 );
	HAL_UART_Receive_IT( &KL_LINE_UART_PORT2, &gucKL2RxDummy, 1 );

	return INIT_OK;
}

void Enable_KL_Interrupt( uint8_t line )
{
	if( line == KL_LINE1 )			HAL_NVIC_EnableIRQ(KL_LINE_UART_PORT1_IRQ);
	else							HAL_NVIC_EnableIRQ(KL_LINE_UART_PORT2_IRQ);
}

void Disable_KL_Interrupt( uint8_t line )
{
	if( line == KL_LINE1 )			HAL_NVIC_DisableIRQ(KL_LINE_UART_PORT1_IRQ);
	else							HAL_NVIC_DisableIRQ(KL_LINE_UART_PORT2_IRQ);
}

void transmitKL( uint8_t line, uint8_t *data, uint8_t len )
{
	UART_HandleTypeDef huart;

	if( line == KL_LINE1 )			huart = KL_LINE_UART_PORT1;
	else							huart = KL_LINE_UART_PORT2;

//	huart.RxState = HAL_UART_STATE_READY;				// // Uart Receive disable

#ifdef CANTEST_LOG
	GLogN("KL Line TX: ");
	for( uint8_t i = 0; i < len; i++ )
	{
		GLogN("%02x ",data[i]);
		if( HAL_UART_Transmit( &huart, (uint8_t *)&data[i], 1, 0xFFFF ) != HAL_OK )
		{
			GLogEE( "Fail... Transmit Uart\r\n" );
		}
	}
	GLogN("\r\n");
#endif

	for( uint8_t i = 0; i < len; i++ )
	{
		if( HAL_UART_Transmit( &huart, (uint8_t *)&data[i], 1, 0xFFFF ) != HAL_OK )
		{
			GLogEE( "Fail... Transmit Uart\r\n" );
		}
	}

//	if( line == KL_LINE1 )			HAL_UART_Receive_IT( &KL_LINE_UART_PORT1, &gucKL1RxDummy, 1 );
//	else							HAL_UART_Receive_IT( &KL_LINE_UART_PORT2, &gucKL2RxDummy, 1 );
}

uint8_t g_ucBoschBlockCnt = 0;

bool transmitKL_ByteTime_bosch( uint8_t line, uint8_t *data, uint8_t len, uint32_t P4time )
{
	UART_HandleTypeDef huart;
	uint8_t i = 0;
	uint8_t rx_len = 0;
	uint8_t rx_buff[2] = {0,};

	if( line == KL_LINE1 )			huart = KL_LINE_UART_PORT1;
	else							huart = KL_LINE_UART_PORT2;
	
	osDelay(3);
	
	data[1] = g_ucBoschBlockCnt+1;

	for( i = 0; i < data[0]; i++ )
	{
		if( HAL_UART_Transmit( &huart, (uint8_t *)&data[i], 1, 0xFFFF ) != HAL_OK )
		{
			GLogEE( "Fail... Transmit Uart\r\n" );
		}
		osDelay(3);
		rx_len = getKLReceiveData_9141(line, &rx_buff[0], 2, 1000);
		if( rx_len != 2) return false;
		g_ucTxbuff[i]=data[i];
	}
	osDelay(3);
	HAL_UART_Transmit( &huart, (uint8_t *)&data[i], 1, 0xFFFF );
	g_ucTxLen = len;
	return true;
}


void transmitKL_ByteTime( uint8_t line, uint8_t *data, uint8_t len, uint32_t P4time )
{
	UART_HandleTypeDef huart;

	if( line == KL_LINE1 )			huart = KL_LINE_UART_PORT1;
	else							huart = KL_LINE_UART_PORT2;

	for( uint8_t i = 0; i < len; i++ )
	{
		if( HAL_UART_Transmit( &huart, (uint8_t *)&data[i], 1, 0xFFFF ) != HAL_OK )
		{
			GLogEE( "Fail... Transmit Uart\r\n" );
		}
		//if(!(len==i+1)) HAL_Delay(P4time);
		if(!(len==i+1)) osDelay(P4time);
		g_ucTxbuff[i]=data[i];
	}
	g_ucTxLen=len;
}

bool transmitKL_ByteTime_wabco_abs( uint8_t line, uint8_t *data, uint8_t len, uint32_t P4time )
{
  	UART_HandleTypeDef huart;
	uint8_t rx_buff[200] = {0,};
	uint8_t rx_len = 0;
	uint8_t ack = 0x80;

	if( line == KL_LINE1 )			huart = KL_LINE_UART_PORT1;
	else							huart = KL_LINE_UART_PORT2;
	
    if(data[0]==0x83) len=5;
    if(data[0]==0x00 ||
	   data[0]==0x04 ||   // WABCO ECAS에서 ACK 04 80 에 대한 처리 누락부분 추가	120723 LWH
       data[0]==0x11 ||
       data[0]==0x41 ||
       data[0]==0x42 ||
       data[0]==0x43 ||
       data[0]==0x44 ||
       data[0]==0x45 ||
       data[0]==0x46 ||
       data[0]==0x47 ||
       data[0]==0x48 ||
       data[0]==0x49) len=2;

	for( uint8_t i = 0; i < (len-1); i++ )
	{
		if( HAL_UART_Transmit( &huart, (uint8_t *)&data[i], 1, 0xFFFF ) != HAL_OK )
		{
			GLogEE( "Fail... Transmit Uart\r\n" );
		}
		osDelay(2);
		rx_len = getKLReceiveData_9141(line, &rx_buff[0], 2, 64);
		//if( rx_len != 2) return false;
        if( rx_len == 0) return false;
		g_ucTxbuff[i]=data[i];
		intDlccomCount = g_uiAckTiming;
		osDelay(1);
	}
	
	if( HAL_UART_Transmit( &huart, &ack, 1, 0xFFFF ) != HAL_OK )
	{
		GLogEE( "Fail... Transmit Uart\r\n" );
	}
	g_ucTxLen=len;
	
	return true;
}

uint32_t getBoschRxBlock( uint8_t ucLine, uint8_t *ucData )
{
  	uint8_t 	ucRxData[2] = {0,};
	uint8_t 	ucRxLen = 0;
	uint8_t		ucTxData = 0;
	uint8_t 	i = 0;
	uint8_t		ucData_Len = 0;
	
  	ucRxLen = getKLReceiveData_9141(ucLine, ucRxData, 2, 1000);
	if(ucRxLen < 2)	return 0;
	
	ucData[0] = ucRxData[1];
	ucTxData = 0xFF - ucRxData[1];
	osDelay(3);
	transmitKL( ucLine, &ucTxData, 1 );
	ucData_Len = ucRxData[1];
	
	for( i = 1; i < ucData_Len; i++ )
	{
	  	ucRxLen = getKLReceiveData_9141(ucLine, ucRxData, 2, 1000);
		if(ucRxLen < 2)	return 0;
		
		ucData[i] = ucRxData[1];
		osDelay(3);
		ucTxData = 0xFF - ucData[i];
		transmitKL( ucLine, &ucTxData, 1 );
	}
	
	ucRxLen = getKLReceiveData_9141(ucLine, ucRxData, 2, 1000);
	if(ucRxLen < 2)	return 0;
	
	ucData[i] = ucRxData[1];
	g_ucBoschBlockCnt = ucData[1];
	
	return i+1;
}

uint32_t getWabcoAbsRxBlock( uint8_t ucLine, uint8_t *ucData )
{
	uint8_t ucRxData[2] = {0,};
	uint8_t ucRxData2[2] = {0,};
	uint8_t ucRxTemp = 0;
	uint8_t ucRxLen = 0;
	uint8_t i = 0;
	
	ucRxLen = getKLReceiveData_9141(ucLine, ucRxData, 2, 35);
	
	//if(ucRxLen < 2)
    if(ucRxLen == 0)
		ucRxLen = getKLReceiveData_9141(ucLine, ucRxData, 2, 30);
	
	//if(ucRxLen < 2)
	//	return 0;
	
	//ucData[0] = ucRxData[1];
    ucData[0] = ucRxData[0];
	
	osDelay(2);
	transmitKL( ucLine, &ucData[0], 1 );
	
	if(ucData[0] < 0x80)
	{
		if(ucData[0] == 0x01)
		{
			getKLReceiveData_9141(ucLine, ucRxData2, 1, 30);
			osDelay(2);
			getKLReceiveData_9141(ucLine, ucRxData, 1, 30);
			getKLReceiveData_9141(ucLine, ucRxData, 1, 60);
			if(ucRxData[0] == 0x01)
			{
				osDelay(2);
				transmitKL( ucLine, &ucData[0], 1 );
				getKLReceiveData_9141(ucLine, ucRxData2, 1, 30);
				osDelay(2);
				getKLReceiveData_9141(ucLine, ucRxData, 1, 30);
				getKLReceiveData_9141(ucLine, ucRxData, 1, 60);
			}
			ucData[0] = ucRxData[0];
			osDelay(2);
			transmitKL( ucLine, &ucData[0], 1 );
		}
		if(ucRxData[0] < 0x80)
		{
			ucRxTemp = ucData[0];
			ucData[0] = 0x80;
		}
	}
	for(i = 1; i<(ucData[0]-0x7F); i++)
	{
		getKLReceiveData_9141(ucLine, ucRxData2, 2, 30);
		ucData[i] = ucRxData2[1];
		osDelay(2);
		transmitKL( ucLine, &ucData[i], 1 );
		
	}
	getKLReceiveData_9141(ucLine, ucRxData2, 2, 30);
	ucData[i] = ucRxData2[1];
	
	ucData[100] = (ucData[0]-0x7F);
	
	if(ucRxTemp==0x04) ucData[0] = 0x04;
	osDelay(12);
	
  	return i;
}

void clearKLReceiveData( uint8_t line )
{
	if( line == KL_LINE1 )			xStreamBufferReset( hSBKLRx1 );
	else							xStreamBufferReset( hSBKLRx2 );
}

uint32_t getKLReceiveDataSize( uint8_t line )
{
	if( line == KL_LINE1 )			return (uint32_t)xStreamBufferBytesAvailable( hSBKLRx1 );
	else							return (uint32_t)xStreamBufferBytesAvailable( hSBKLRx2 );
}


#if 0
uint32_t getKLReceiveData( uint8_t line, uint8_t *data, uint32_t len )
{
	//if( line == KL_LINE1 )			return xStreamBufferReceive( hSBKLRx1, (void *)data, len, pdMS_TO_TICKS( 1000 ) );			// wait time 3S
	//else							return xStreamBufferReceive( hSBKLRx2, (void *)data, len, pdMS_TO_TICKS( 1000 ) );			// wait time 3S
	if( line == KL_LINE1 )			return xStreamBufferReceive( hSBKLRx1, (void *)data, len, pdMS_TO_TICKS( g_stGITSetConfig.nP3Min ) );			// wait time 3S
	else							return xStreamBufferReceive( hSBKLRx2, (void *)data, len, pdMS_TO_TICKS( g_stGITSetConfig.nP3Min ) );			// wait time 3S
}
#else
uint32_t getKLReceiveData( uint8_t line, uint8_t *data, uint32_t len )
{
	//printf("%s] tick : %d --> %d\r\n", __func__, g_stGITSetConfig.nP3Min, pdMS_TO_TICKS( g_stGITSetConfig.nP3Min ));
	uint32_t P3MinTemp=0;

	StreamBufferHandle_t xStreamBuffer = NULL;

	if(g_bIsAutoVIN==TRUE)
	{
		P3MinTemp=g_stGITSetConfig.nP3Min;
		g_stGITSetConfig.nP3Min=70;
	}

	if( line == KL_LINE1 )
		xStreamBuffer = hSBKLRx1;
	else
		xStreamBuffer = hSBKLRx2;

	if( len == 1 )
	{				
		int nLength = 0;
		int nTimeout = Get_Tmr();
		do 
		{
			nLength = xStreamBufferBytesAvailable(xStreamBuffer);
	
			if( (Get_Tmr() - nTimeout) >  (g_stGITSetConfig.nP3Min-1) )
			{
				break;
			}

			if( nLength == 0 ) osDelay(1);
		}
		while( nLength == 0 );

		if(g_bIsAutoVIN==TRUE)
		{
			g_stGITSetConfig.nP3Min=P3MinTemp;
			g_bIsAutoVIN=FALSE;
		}
		return xStreamBufferReceive( xStreamBuffer, (void *)data, len, 1);			// wait time 3S
	}

	if(g_bIsAutoVIN==TRUE)
	{
		g_stGITSetConfig.nP3Min=P3MinTemp;
		g_bIsAutoVIN=FALSE;
	}
	return xStreamBufferReceive( xStreamBuffer, (void *)data, len, g_stGITSetConfig.nP3Min);			// wait time 3S	
}

uint32_t getKLReceiveData_Timeout( uint8_t line, uint8_t *data, uint32_t len, uint32_t uiTimeout )
{
	//printf("%s] tick : %d --> %d\r\n", __func__, g_stGITSetConfig.nP3Min, pdMS_TO_TICKS( g_stGITSetConfig.nP3Min ));
	uint32_t P3MinTemp;

	StreamBufferHandle_t xStreamBuffer = NULL;

	if(g_bIsAutoVIN==TRUE)
	{
		uiTimeout=70;
	}

	if( line == KL_LINE1 )
		xStreamBuffer = hSBKLRx1;
	else
		xStreamBuffer = hSBKLRx2;

	if( len == 1 )
	{				
		int nLength = 0;
		int nTimeout = Get_Tmr();
		do 
		{
			nLength = xStreamBufferBytesAvailable(xStreamBuffer);
	
			if( (Get_Tmr() - nTimeout) >  (uiTimeout) )
			{
				break;
			}

			if( nLength == 0 ) osDelay(1);
		}
		while( nLength == 0 );

		if(g_bIsAutoVIN==TRUE)
		{
			g_bIsAutoVIN=FALSE;
		}
		return xStreamBufferReceive( xStreamBuffer, (void *)data, len, 1);			// wait time 3S
	}

	if(g_bIsAutoVIN==TRUE)
	{
		g_bIsAutoVIN=FALSE;
	}
	return xStreamBufferReceive( xStreamBuffer, (void *)data, len, g_stGITSetConfig.nP3Min);			// wait time 3S	
}

#endif

uint32_t getKLReceiveData_9141( uint8_t line, uint8_t *data, uint32_t len, uint32_t timeout)
{
	//printf("%s] tick : %d --> %d\r\n", __func__, g_stGITSetConfig.nP3Min, pdMS_TO_TICKS( g_stGITSetConfig.nP3Min ));

	StreamBufferHandle_t xStreamBuffer = NULL;
	int nLength = 0;
	int nTimeout = Get_Tmr();

	if( line == KL_LINE1 )
		xStreamBuffer = hSBKLRx1;
	else
		xStreamBuffer = hSBKLRx2;
	
	do 
	{
		nLength = xStreamBufferBytesAvailable(xStreamBuffer);

		if( (Get_Tmr() - nTimeout) >=  timeout )
		{
			break;
		}

		//if( nLength == 0 ) osDelay(1);
	}
	while( nLength < len );

	return xStreamBufferReceive( xStreamBuffer, (void *)data, len, 1);			// wait time 3S
}

#if 0
uint32_t GetKlineDataTime( uint8_t line, uint8_t *data )
{
	uint8_t	i, ucRxCnt, ucGarbageCnt;
	uint32_t	uiP3min;
	U8 end=0,WriteMsgLength=0;
	
	uint32_t	oldtime;
	u16 DlcRxCount = 0;
	u16 DlcRxCountTemp = 0;

	uint8_t	rxBuff[100]	= { 0, };
	
	oldtime = Get_Tmr();
	ucRxCnt=0;
	//uiP3min=70;
	uiP3min=g_stGITSetConfig.nP3Min;
	WriteMsgLength=g_ucTxLen;

	while( Get_TmrDelta( Get_Tmr(), oldtime ) < uiP3min )
	{
        //len=getKLReceiveDataSize(KL_LINE1);
		//DlcRxCount += getKLReceiveData( KL_LINE1, rxBuff, len );
		DlcRxCount += getKLReceiveData( line, &rxBuff[ucRxCnt], 1 );
		ucRxCnt++;
		
		if(DlcRxCount!=DlcRxCountTemp)
		{
			DlcRxCountTemp=DlcRxCount;
			oldtime = Get_Tmr();
		}
	}
	ucRxCnt--;
	//송수신 라인이 같아 송신한 데이터도 수신데이터로 들어옴
	//수신데이터 중 젤앞에 쓰레기 데이터 1바이트 수신되는 경우 있음
	//송신한 데이터와 쓰레기 데이터(쓰레기 255바이트까지 버리기 가능) 제거로직
	for(ucGarbageCnt=0; ucGarbageCnt<255; ucGarbageCnt++)
	{
		if((g_ucTxbuff[0]==rxBuff[0+ucGarbageCnt])
			&&(g_ucTxbuff[1]==rxBuff[1+ucGarbageCnt])
			&&(g_ucTxbuff[2]==rxBuff[2+ucGarbageCnt])
			)
		{
			ucRxCnt=(ucRxCnt-(WriteMsgLength+ucGarbageCnt)); //ucGarbageCnt : garbage data size, WriteMsgLength : tx data size
			for(i=0; i<ucRxCnt; i++)
			{
				rxBuff[i]=rxBuff[i+WriteMsgLength+ucGarbageCnt];
			}
			break;
		}
	}
#if 1
	if(ucRxCnt>0)
	{
		//GLogN("\n\r rxBuff1(%d) :",ucRxCnt);
		for(i=0; i<ucRxCnt; i++)
		{
			data[i]=rxBuff[i];
			//GLogN(" %02X",rxBuff[i]);
		}
	}
#endif
	return ucRxCnt;
}
#else
uint32_t GetKlineDataTime( uint8_t line, uint8_t *data )
{
	uint32_t	i, ucRxCnt;//, ucGarbageCnt;
	//uint32_t	uiP3min;
	//U8 end=0,WriteMsgLength=0;
	
	//uint32_t	oldtime;
	u16 DlcRxCount = 0;
	u16 DlcRxCountTemp = 0;
	u32	ulTimeout = g_stGITSetConfig.nP3Min;
	static bool bPendingFlag = false;

	uint8_t	rxBuff[K_LINE_BUFF_SIZE]	= { 0, };
	
	//oldtime = Get_Tmr();
	ucRxCnt=0;
	//uiP3min=70;
	//uiP3min=g_stGITSetConfig.nP3Min;
	//WriteMsgLength=g_ucTxLen;
#if 0 //byte time : uiP3min
	while( Get_TmrDelta( Get_Tmr(), oldtime ) < uiP3min )
	{
        DlcRxCount += getKLReceiveData( line, &rxBuff[ucRxCnt], 1 );
		ucRxCnt++;
		
		if(DlcRxCount!=DlcRxCountTemp)
		{
			DlcRxCountTemp=DlcRxCount;
			oldtime = Get_Tmr();
		}
	}
	ucRxCnt--;
#else //
	//GLogN("\n\r rxBuff0 :%d:",ulTimeout);
	while(1)
	{
		if( ucRxCnt == 0 )
        {
            if( bPendingFlag == true )  ulTimeout = g_stGITSetConfig.nP3Max;
            else                        ulTimeout = g_stGITSetConfig.nP3Min;
            //GLogN("\n\r rxBuff0 :%d:",ulTimeout);
        }
        else
        {
            ulTimeout = g_stGITSetConfig.nP4Max;
        }
		DlcRxCount += getKLReceiveData_Timeout( line, &rxBuff[ucRxCnt], 1 ,ulTimeout);
		ucRxCnt++;
		//ulTimeout = g_stGITSetConfig.nP4Max;
		
		if(DlcRxCount!=DlcRxCountTemp)
		{
			//GLogN(" %02X",rxBuff[ucRxCnt-1]);
			DlcRxCountTemp=DlcRxCount;
			intDlccomCount = g_uiAckTiming;
		}
		else break;
		if(ucRxCnt>1900) break; //infinite reception processing
	}
	ucRxCnt--;
#endif
#if 0
	//송수신 라인이 같아 송신한 데이터도 수신데이터로 들어옴
	//수신데이터 중 젤앞에 쓰레기 데이터 1바이트 수신되는 경우 있음
	//송신한 데이터와 쓰레기 데이터(쓰레기 255바이트까지 버리기 가능) 제거로직
	for(ucGarbageCnt=0; ucGarbageCnt<255; ucGarbageCnt++)
	{
		if((g_ucTxbuff[0]==rxBuff[0+ucGarbageCnt])
			&&(g_ucTxbuff[1]==rxBuff[1+ucGarbageCnt])
			&&(g_ucTxbuff[2]==rxBuff[2+ucGarbageCnt])
			)
		{
			ucRxCnt=(ucRxCnt-(WriteMsgLength+ucGarbageCnt)); //ucGarbageCnt : garbage data size, WriteMsgLength : tx data size
			for(i=0; i<ucRxCnt; i++)
			{
				rxBuff[i]=rxBuff[i+WriteMsgLength+ucGarbageCnt];
			}
			break;
		}
	}
#endif
#if 0	//rx debug print
	if(DlcRxCount>0)
	{
		GLogN("\n\r rxBuff0(%d) :",ucRxCnt);
		for(i=0; i<DlcRxCount; i++)
		{
			data[i]=rxBuff[i];
			GLogN(" %02X",rxBuff[i]);
		} 
	}
#else
	if(DlcRxCount>0 )
	{
		for(i=0; i<DlcRxCount; i++)
		{
			if(DlcRxCount<K_LINE_BUFF_SIZE)	data[i]=rxBuff[i];
			else							break;
		}
	}
#endif
    if( (rxBuff[3]==0x7F && rxBuff[5]==0x78) || (rxBuff[4]==0x7F && rxBuff[6]==0x78) )	bPendingFlag = true;
    else																			    bPendingFlag = false;

	return DlcRxCount;
}
#endif
#define KL_TEST_DATA_LENGTH			7

static uint8_t	txBuff[KL_TEST_DATA_LENGTH]	= "KL TEST";
static uint8_t	rxBuff[KL_TEST_DATA_LENGTH]	= { 0, };

#if( VCI_III_ASING_MODE )
int32_t SelfTest_KLINE( void )
{
	uint8_t	count		= 0;

	uint8_t	i, j=0, k;

#if 0
	InitIOCTL();
#else
	SetKL_Line( 0, 0 );				// set KL line All off
	osDelay( 1 );						//추후 확인 필요 jkc
	SetReprogramVol( 0 );			// set Reprogram All off
	
	//DisableHighCan1();
	//DisableHighCan2();
	DisableEthDiag();				// Eth Diag All off  

	KL_TXD1_INV_DISABLE;
	KL_TXD2_INV_DISABLE;
	KL_RXD1_INV_DISABLE;
	KL_RXD2_INV_DISABLE;

	KL_LINE1_DISCONNECT_PULLUP;
	KL_LINE2_DISCONNECT_PULLUP;

	HIGHCAN2_120OHM_DISABLE;
	IG_ON_DETECT_ENABLE;
	TRIGGER_LED_OFF;
#endif
	

	Enable_KL_Interrupt( KL_LINE1 );
	Enable_KL_Interrupt( KL_LINE2 );

	
	
	// 1. KL Line 1 Tx, KL_Line 2 Rx
	for( i = 0; i < 2; i++ )
	{
		if( i == 0 )
		{
			IO_CONTROL_HIGH( KL_TXD1_INV );
			IO_CONTROL_HIGH( KL_TXD2_INV );
			IO_CONTROL_LOW( KL_RXD2_SEL );
			IO_CONTROL_LOW( KL_RXD2_INV );
		}
		else
		{
			IO_CONTROL_LOW( KL_TXD1_INV );
			IO_CONTROL_HIGH( KL_TXD2_INV );
			IO_CONTROL_HIGH( KL_RXD2_SEL );
			IO_CONTROL_HIGH( KL_RXD2_INV );
		}

		KL_LINE1_CONNECT_47K;

		for( k = 0; k < 4; k++ )
		{
			memset( rxBuff, 0, KL_TEST_DATA_LENGTH );

			clearKLReceiveData( KL_LINE1 );
			clearKLReceiveData( KL_LINE2 );

			switch( k )
			{
				case 0 :		SetKL_Line( KL_LINE1_CONNECT_CH02, KL_LINE2_CONNECT_CH08 );			break;
				case 1 :		SetKL_Line( KL_LINE1_CONNECT_CH03, KL_LINE2_CONNECT_CH08 );			break;
				case 2 :		SetKL_Line( KL_LINE1_CONNECT_CH07, KL_LINE2_CONNECT_CH08 );			break;
				case 3 :		SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH08 );			break;	
			}

			osDelay( 20 );

			transmitKL( KL_LINE1, txBuff, KL_TEST_DATA_LENGTH );
			osDelay( 20 );
			count = getKLReceiveData( KL_LINE2, rxBuff, KL_TEST_DATA_LENGTH );
			
#ifdef CANTEST_LOG
			GLogN("KL Line2 RX: ",rxBuff[i]);
			for(int i=0; i<KL_TEST_DATA_LENGTH;i++)
			{
				GLogN("%02x ",rxBuff[i]);
			}
			GLogN("\r\n");
#endif
			if( count != KL_TEST_DATA_LENGTH || memcmp( txBuff, rxBuff, KL_TEST_DATA_LENGTH ) != 0 )
			{
				GLogEE( "Fail... KL Test( i : %d, j : %d, k : %d )\r\n", i, j, k );
				KL_LINE1_DISCONNECT_PULLUP;
				KL_LINE2_DISCONNECT_PULLUP;
				
				return -1;
			}
		}
	}

	KL_LINE1_DISCONNECT_PULLUP;

	// 2. KL Line 1 Rx, KL_Line 2 Tx
	for( i = 0; i < 2; i++ )
	{
		if( i == 0 )
		{
			IO_CONTROL_HIGH( KL_TXD1_INV );
			IO_CONTROL_HIGH( KL_TXD2_INV );
			IO_CONTROL_LOW( KL_RXD1_SEL );
			IO_CONTROL_LOW( KL_RXD1_INV );
		}
		else
		{
			IO_CONTROL_HIGH( KL_TXD1_INV );
			IO_CONTROL_LOW( KL_TXD2_INV );
			IO_CONTROL_HIGH( KL_RXD1_SEL );
			IO_CONTROL_HIGH( KL_RXD1_INV );
		}

		KL_LINE2_CONNECT_47K;

		for( k = 0; k < 3; k++ )
		{
			memset( rxBuff, 0, KL_TEST_DATA_LENGTH );

			clearKLReceiveData( KL_LINE1 );
			clearKLReceiveData( KL_LINE2 );

			switch( k )
			{
				case 0 :		SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH08 );			break;
				case 1 :		SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH10 );			break;
				case 2 :		SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH11 );			break;
			}

			osDelay( 20 );
			transmitKL( KL_LINE2, txBuff, KL_TEST_DATA_LENGTH );
			osDelay( 20 );
			count = getKLReceiveData( KL_LINE1, rxBuff, KL_TEST_DATA_LENGTH );
			
#ifdef CANTEST_LOG
			GLogN("KL Line1 RX: ",rxBuff[i]);
			for(int i=0; i<KL_TEST_DATA_LENGTH;i++)
			{
				GLogN("%02x ",rxBuff[i]);
			}
			GLogN("\r\n");
#endif
			if( count != KL_TEST_DATA_LENGTH || memcmp( txBuff, rxBuff, KL_TEST_DATA_LENGTH ) != 0 )
			{
				GLogEE( "Fail... KL Test( i : %d, j : %d, k : %d )\r\n", i, j, k );
				return -1;
			}
		}
	}

	KL_LINE1_DISCONNECT_PULLUP;
	KL_LINE2_DISCONNECT_PULLUP;

	IO_CONTROL_HIGH( KL_TXD1_INV );
	IO_CONTROL_HIGH( KL_TXD2_INV );
	IO_CONTROL_LOW( KL_RXD1_SEL );
	IO_CONTROL_LOW( KL_RXD1_INV );
	IO_CONTROL_LOW( KL_RXD2_SEL );
	IO_CONTROL_LOW( KL_RXD2_INV );

	Disable_KL_Interrupt( KL_LINE1 );
	Disable_KL_Interrupt( KL_LINE2 );

	return 0;
}
#else
int32_t SelfTest_KLINE( void )
{
//	uint8_t	txBuff[KL_TEST_DATA_LENGTH]	= "KL TEST";
//	uint8_t	rxBuff[KL_TEST_DATA_LENGTH]	= { 0, };
	uint8_t	count		= 0;

	uint8_t	i, j, k;
	
	KL_LINE1_DISCONNECT_PULLUP;
	KL_LINE2_DISCONNECT_PULLUP;
	
	Enable_KL_Interrupt( KL_LINE1 );
	Enable_KL_Interrupt( KL_LINE2 );

	// 1. KL Line 1 Tx, KL_Line 2 Rx
	for( i = 0; i < 2; i++ )
	{
		if( i == 0 )
		{
			IO_CONTROL_HIGH( KL_TXD1_INV );
			IO_CONTROL_HIGH( KL_TXD2_INV );
			IO_CONTROL_LOW( KL_RXD2_SEL );
			IO_CONTROL_LOW( KL_RXD2_INV );
		}
		else
		{
			IO_CONTROL_LOW( KL_TXD1_INV );
			IO_CONTROL_HIGH( KL_TXD2_INV );
			IO_CONTROL_HIGH( KL_RXD2_SEL );
			IO_CONTROL_HIGH( KL_RXD2_INV );
		}

		for( j = 0; j < 3; j++ )
		{
			switch( j )
			{
				case 0 :		GLogN( "KL1 Pull Up is 510\r\n" );		KL_LINE1_CONNECT_510;			break;
				case 1 :		GLogN( "KL1 Pull Up is 2K\r\n" );		KL_LINE1_CONNECT_2K;			break;
				case 2 :		GLogN( "KL1 Pull Up is 4.7K\r\n" );		KL_LINE1_CONNECT_47K;			break;
			}

			for( k = 0; k < 7; k++ )
			{
				memset( rxBuff, 0, KL_TEST_DATA_LENGTH );

				clearKLReceiveData( KL_LINE1 );
				clearKLReceiveData( KL_LINE2 );

				switch( k )
				{
					case 0 :		SetKL_Line( KL_LINE1_CONNECT_CH01, KL_LINE2_CONNECT_CH08 );			break;
					case 1 :		SetKL_Line( KL_LINE1_CONNECT_CH02, KL_LINE2_CONNECT_CH08 );			break;
					case 2 :		SetKL_Line( KL_LINE1_CONNECT_CH03, KL_LINE2_CONNECT_CH08 );			break;
					case 3 :		SetKL_Line( KL_LINE1_CONNECT_CH06, KL_LINE2_CONNECT_CH08 );			break;
					case 4 :		SetKL_Line( KL_LINE1_CONNECT_CH07, KL_LINE2_CONNECT_CH08 );			break;
					case 5 :		SetKL_Line( KL_LINE1_CONNECT_CH13, KL_LINE2_CONNECT_CH08 );			break;
					case 6 :		SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH08 );			break;
				}

				osDelay( 20 );

				transmitKL( KL_LINE1, txBuff, KL_TEST_DATA_LENGTH );

				osDelay( 20 );

				count = getKLReceiveData( KL_LINE2, rxBuff, KL_TEST_DATA_LENGTH );
				if( count != KL_TEST_DATA_LENGTH || memcmp( txBuff, rxBuff, KL_TEST_DATA_LENGTH ) != 0 )
				{
					GLogEE( "Fail... KL Test( i : %d, j : %d, k : %d )\r\n", i, j, k );
					KL_LINE1_DISCONNECT_PULLUP;
					KL_LINE2_DISCONNECT_PULLUP;
					
					return -1;
				}
			}
		}
	}

	KL_LINE1_DISCONNECT_PULLUP;

	// 2. KL Line 1 Rx, KL_Line 2 Tx
	for( i = 0; i < 2; i++ )
	{
		if( i == 0 )
		{
			IO_CONTROL_HIGH( KL_TXD1_INV );
			IO_CONTROL_HIGH( KL_TXD2_INV );
			IO_CONTROL_LOW( KL_RXD1_SEL );
			IO_CONTROL_LOW( KL_RXD1_INV );
		}
		else
		{
			IO_CONTROL_HIGH( KL_TXD1_INV );
			IO_CONTROL_LOW( KL_TXD2_INV );
			IO_CONTROL_HIGH( KL_RXD1_SEL );
			IO_CONTROL_HIGH( KL_RXD1_INV );
		}

		for( j = 0; j < 3; j++ )
		{
			switch( j )
			{
				case 0 :		GLogN( "KL2 Pull Up is 510\r\n" );		KL_LINE2_CONNECT_510;			break;
				case 1 :		GLogN( "KL2 Pull Up is 2K\r\n" );		KL_LINE2_CONNECT_2K;			break;
				case 2 :		GLogN( "KL2 Pull Up is 4.7K\r\n" );		KL_LINE2_CONNECT_47K;			break;
			}

			for( k = 0; k < 6; k++ )
			{
				memset( rxBuff, 0, KL_TEST_DATA_LENGTH );

				clearKLReceiveData( KL_LINE1 );
				clearKLReceiveData( KL_LINE2 );

				switch( k )
				{
				//	case 0 :		SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH09_CH08 );			break;
				//	case 1 :		SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH10_CH08 );			break;
				//	case 2 :		SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH11_CH08 );			break;
				//	case 3 :		SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH12_CH08 );			break;
				//	case 4 :		SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH14_CH08 );			break;
				//	case 5 :		SetKL_Line( KL_LINE1_CONNECT_CH08, KL_LINE2_CONNECT_CH15_CH08 );			break;
				}

				osDelay( 20 );

				transmitKL( KL_LINE2, txBuff, KL_TEST_DATA_LENGTH );

				osDelay( 20 );

				count = getKLReceiveData( KL_LINE1, rxBuff, KL_TEST_DATA_LENGTH );
				if( count != KL_TEST_DATA_LENGTH || memcmp( txBuff, rxBuff, KL_TEST_DATA_LENGTH ) != 0 )
				{
					GLogEE( "Fail... KL Test( i : %d, j : %d, k : %d )\r\n", i, j, k );

					KL_LINE1_DISCONNECT_PULLUP;
					KL_LINE2_DISCONNECT_PULLUP;
					
					return -1;
				}
			}
		}
	}

	KL_LINE1_DISCONNECT_PULLUP;
	KL_LINE2_DISCONNECT_PULLUP;

	IO_CONTROL_HIGH( KL_TXD1_INV );
	IO_CONTROL_HIGH( KL_TXD2_INV );

	Disable_KL_Interrupt( KL_LINE1 );
	Disable_KL_Interrupt( KL_LINE2 );

	return 0;
}
#endif	// VCI_III_ASING_MODE
int32_t SelfTest_KLINE_pullup( uint8_t ucPullup )
{
//	uint8_t txBuff[KL_TEST_DATA_LENGTH] = "KL TEST";
//	uint8_t rxBuff[KL_TEST_DATA_LENGTH] = { 0, };
	uint8_t count	= 0;

	uint8_t i, k;

	InitIOCTL();
	DisableEthDiag();
	
	Enable_KL_Interrupt( KL_LINE1 );
	Enable_KL_Interrupt( KL_LINE2 );

	// 1. KL Line 1 Tx, KL_Line 2 Rx
	for( i = 0; i < 2; i++ )
	{
		if( i == 0 )
		{
			IO_CONTROL_HIGH( KL_TXD1_INV );
			IO_CONTROL_HIGH( KL_TXD2_INV );
			IO_CONTROL_LOW( KL_RXD2_SEL );
			IO_CONTROL_LOW( KL_RXD2_INV );
		}
		else
		{
			IO_CONTROL_LOW( KL_TXD1_INV );
			IO_CONTROL_HIGH( KL_TXD2_INV );
			IO_CONTROL_HIGH( KL_RXD2_SEL );
			IO_CONTROL_HIGH( KL_RXD2_INV );
		}
		
		if(ucPullup==0)
		{
			GLogN( "KL1 Pull Up is 510\r\n" );			KL_LINE1_CONNECT_510;
		}
		else if(ucPullup==1)
		{
			GLogN( "KL1 Pull Up is 2K\r\n" );			KL_LINE1_CONNECT_2K;
			//i = 1; // mod.kks except the 2K. HW Spec.
		}
		else if(ucPullup==2)
		{
			GLogN( "KL1 Pull Up is 4.7K\r\n" ); 	 	KL_LINE1_CONNECT_47K;
			IO_CONTROL_HIGH( KL_RXD2_SEL );
		}
			
		for( k = 0; k < 7; k++ )
		{
			memset( rxBuff, 0, KL_TEST_DATA_LENGTH );
			
			clearKLReceiveData( KL_LINE1 );
			clearKLReceiveData( KL_LINE2 );

			switch( k )
			{
				case 0 :		SetKL_Line( KL_LINE1_CONNECT_CH01, KL_LINE2_CONNECT_CH08);			break;
				case 1 :		SetKL_Line( KL_LINE1_CONNECT_CH02, KL_LINE2_CONNECT_CH08); 		break;
				case 2 :		SetKL_Line( KL_LINE1_CONNECT_CH03, KL_LINE2_CONNECT_CH08); 		break;
				case 3 :		SetKL_Line( KL_LINE1_CONNECT_CH06, KL_LINE2_CONNECT_CH08); 		break;
				case 4 :		SetKL_Line( KL_LINE1_CONNECT_CH07, KL_LINE2_CONNECT_CH08); 		break;
				case 5 :		SetKL_Line( KL_LINE1_CONNECT_CH13, KL_LINE2_CONNECT_CH08); 		break;
				case 6 :		SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH08); 		break;
			}

			osDelay( 20 );

			transmitKL( KL_LINE1, txBuff, KL_TEST_DATA_LENGTH );

			osDelay( 20 );

			count = getKLReceiveData( KL_LINE2, rxBuff, KL_TEST_DATA_LENGTH );

			if( count != KL_TEST_DATA_LENGTH || memcmp( txBuff, rxBuff, KL_TEST_DATA_LENGTH ) != 0 )
			{
				GLogEE( "Fail... KL Test( i : %d, k : %d )\r\n", i, k );

		KL_LINE1_DISCONNECT_PULLUP;
		KL_LINE2_DISCONNECT_PULLUP;

				return k;
			}
		}
	}

	KL_LINE1_DISCONNECT_PULLUP;
	KL_LINE2_DISCONNECT_PULLUP;

	// 2. KL Line 1 Rx, KL_Line 2 Tx
	for( i = 0; i < 2; i++ )
	{
		if( i == 0 )
		{
			IO_CONTROL_HIGH( KL_TXD1_INV );
			IO_CONTROL_HIGH( KL_TXD2_INV );
			IO_CONTROL_LOW( KL_RXD1_SEL );
			IO_CONTROL_LOW( KL_RXD1_INV );
		}
		else
		{
			IO_CONTROL_HIGH( KL_TXD1_INV );
			IO_CONTROL_LOW( KL_TXD2_INV );
			IO_CONTROL_HIGH( KL_RXD1_SEL );
			IO_CONTROL_HIGH( KL_RXD1_INV );
		}
		
		if(ucPullup==0)
		{
			GLogN( "KL2 Pull Up is 510\r\n" );		KL_LINE2_CONNECT_510;
		}
		else if(ucPullup==1)
		{
			GLogN( "KL2 Pull Up is 2K\r\n" );		KL_LINE2_CONNECT_2K;
			//i = 1; //mod.kks to except the HW Spec.
		}
		else if(ucPullup==2)
		{
			GLogN( "KL2 Pull Up is 4.7K\r\n" ); 	KL_LINE2_CONNECT_47K;
		}

		for( k = 7; k < 13; k++ )
		{
			switch( k )
			{
			case 7 :		SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH08);				break;
			case 8 :		SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH09);				break;
			case 9 :		SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH10);				break;
			case 10 :		SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH11);				break;
			case 11 :		SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH12); 			break;
			case 12 :		SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH14); 			break;

			}

			osDelay( 20 );

			memset( rxBuff, 0, KL_TEST_DATA_LENGTH );

			clearKLReceiveData( KL_LINE1 );
			clearKLReceiveData( KL_LINE2 );

			transmitKL( KL_LINE2, txBuff, KL_TEST_DATA_LENGTH );

			osDelay( 20 );

			count = getKLReceiveData( KL_LINE1, rxBuff, KL_TEST_DATA_LENGTH );

			if( count != KL_TEST_DATA_LENGTH || memcmp( txBuff, rxBuff, KL_TEST_DATA_LENGTH ) != 0 )
			{
				GLogEE( "Fail... KL Test( i : %d, k : %d )\r\n", i, k );
				
		KL_LINE1_DISCONNECT_PULLUP;
		KL_LINE2_DISCONNECT_PULLUP;

				return k;
			}
		}
	}

	KL_LINE1_DISCONNECT_PULLUP;
	KL_LINE2_DISCONNECT_PULLUP;

	IO_CONTROL_HIGH( KL_TXD1_INV );
	IO_CONTROL_HIGH( KL_TXD2_INV );

	Disable_KL_Interrupt( KL_LINE1 );
	Disable_KL_Interrupt( KL_LINE2 );

	return 20;
}
int32_t SelfTest_KLINE_OBD_Connect( void )
{
//	uint8_t	txBuff[KL_TEST_DATA_LENGTH]	= "KL TEST";
//	uint8_t	rxBuff[KL_TEST_DATA_LENGTH]	= { 0, };
	uint8_t count		= 0;

	uint8_t	 i,j, k;

	InitIOCTL();
	DisableEthDiag();
	
	Enable_KL_Interrupt( KL_LINE1 );
	Enable_KL_Interrupt( KL_LINE2 );

	// 1. KL Line 1 Tx, KL_Line 2 Rx
	for( i = 0; i < 2; i++ )
	{
		if( i == 0 )
		{
			IO_CONTROL_HIGH( KL_TXD1_INV );
			IO_CONTROL_HIGH( KL_TXD2_INV );
			IO_CONTROL_LOW( KL_RXD2_INV );
			IO_CONTROL_LOW( KL_RXD2_SEL );
		}
		else
		{
			IO_CONTROL_LOW( KL_TXD1_INV );
			IO_CONTROL_HIGH( KL_TXD2_INV );
			IO_CONTROL_HIGH( KL_RXD2_SEL );
			IO_CONTROL_HIGH( KL_RXD2_INV );
		}
		for( j = 0; j < 3; j++ )
		{
			switch( j )
			{
				case 0 :		GLogN( "KL1 Pull Up is 510\r\n" );		KL_LINE1_CONNECT_510;			break;
				case 1 :		GLogN( "KL1 Pull Up is 2K\r\n" );			KL_LINE1_CONNECT_2K;			break;
				case 2 :		GLogN( "KL1 Pull Up is 4.7K\r\n" );		KL_LINE1_CONNECT_47K;			break;
			}

			for( k = 0; k < 7; k++ )
			{
				memset( rxBuff, 0, KL_TEST_DATA_LENGTH );

				clearKLReceiveData( KL_LINE1 );
				clearKLReceiveData( KL_LINE2 );

				switch( k )
				{
					case 0 :		SetKL_Line( KL_LINE1_CONNECT_CH01, KL_LINE2_CONNECT_CH08 );			break;
					case 1 :		SetKL_Line( KL_LINE1_CONNECT_CH02, KL_LINE2_CONNECT_CH08);			break;
					case 2 :		SetKL_Line( KL_LINE1_CONNECT_CH03, KL_LINE2_CONNECT_CH08);			break;
					case 3 :		SetKL_Line( KL_LINE1_CONNECT_CH06, KL_LINE2_CONNECT_CH08);			break;
					case 4 :		SetKL_Line( KL_LINE1_CONNECT_CH07, KL_LINE2_CONNECT_CH08);			break;
					case 5 :		SetKL_Line( KL_LINE1_CONNECT_CH13, KL_LINE2_CONNECT_CH08);			break;
					case 6 :		SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH08);			break;
				}

				osDelay( 20 );

				transmitKL( KL_LINE1, txBuff, KL_TEST_DATA_LENGTH );

				osDelay( 20 );

				count = getKLReceiveData( KL_LINE2, rxBuff, KL_TEST_DATA_LENGTH );
				if( count != KL_TEST_DATA_LENGTH || memcmp( txBuff, rxBuff, KL_TEST_DATA_LENGTH ) != 0 )
				{
					GLogEE( "Fail... KL Test( k : %d )\r\n", k );
					
					KL_LINE1_DISCONNECT_PULLUP;
					KL_LINE2_DISCONNECT_PULLUP;

					return k;
				}
				else GLogN( "OK... KL Test( k : %d )\r\n", k );
			}
		}
	}

	KL_LINE1_DISCONNECT_PULLUP;
	KL_LINE2_DISCONNECT_PULLUP;
	
	// 2. KL Line 1 Rx, KL_Line 2 Tx
	for( i = 0; i < 2; i++ )
	{
		if( i == 0 )
		{
			IO_CONTROL_HIGH( KL_TXD1_INV );
			IO_CONTROL_HIGH( KL_TXD2_INV );
			IO_CONTROL_LOW( KL_RXD1_SEL );
			IO_CONTROL_LOW( KL_RXD1_INV );
		}
		else
		{
			IO_CONTROL_HIGH( KL_TXD1_INV );
			IO_CONTROL_LOW( KL_TXD2_INV );
			IO_CONTROL_HIGH( KL_RXD1_SEL );
			IO_CONTROL_HIGH( KL_RXD1_INV );
		}

		for( j = 0; j < 3; j++ )
		{
			switch( j )
			{
				case 0 :		GLogN( "KL2 Pull Up is 510\r\n" );		KL_LINE2_CONNECT_510;			break;
				case 1 :		GLogN( "KL2 Pull Up is 2K\r\n" );			KL_LINE2_CONNECT_2K;			break;
				case 2 :		GLogN( "KL2 Pull Up is 4.7K\r\n" );		KL_LINE2_CONNECT_47K;			break;
			}
			
			for( k = 7; k < 13; k++ )
			{
				memset( rxBuff, 0, KL_TEST_DATA_LENGTH );

				clearKLReceiveData( KL_LINE1 );
				clearKLReceiveData( KL_LINE2 );

				switch( k )
				{
					case 7 :			SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH08);		break;
					case 8 :			SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH09 );		break;
					case 9 :			SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH10 );		break;
					case 10 :		SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH11 );		break;
					case 11 :		SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH12);		break;
					case 12 :		SetKL_Line( KL_LINE1_CONNECT_CH15, KL_LINE2_CONNECT_CH14);		break;
					
				}

				osDelay( 20 );

				transmitKL( KL_LINE2, txBuff, KL_TEST_DATA_LENGTH );

				osDelay( 20 );

				count = getKLReceiveData( KL_LINE1, rxBuff, KL_TEST_DATA_LENGTH );
				if( count != KL_TEST_DATA_LENGTH || memcmp( txBuff, rxBuff, KL_TEST_DATA_LENGTH ) != 0 )
				{
					GLogEE( "Fail... KL Test( k : %d )\r\n", k );
					
					KL_LINE1_DISCONNECT_PULLUP;
					KL_LINE2_DISCONNECT_PULLUP;

					return k;
				}
				else GLogN( "OK... KL Test( k : %d )\r\n", k );
			}
		}
	}

	KL_LINE1_DISCONNECT_PULLUP;
	KL_LINE2_DISCONNECT_PULLUP;
	
	IO_CONTROL_HIGH( KL_TXD1_INV );
	IO_CONTROL_HIGH( KL_TXD2_INV );

	Disable_KL_Interrupt( KL_LINE1 );
	Disable_KL_Interrupt( KL_LINE2 );

	return 20;
}


/**
 * - KL LINE Baudrate change function. by KKT
 * - VCI1,2 : same function name
 **/
int32_t uartSetBaudRate(uint8_t Line, uint32_t Baudrate )
{
	if(Line==KL_LINE1)
	{
		KL_LINE_UART_PORT1.Instance = USART1;
		KL_LINE_UART_PORT1.Init.BaudRate = Baudrate;
		KL_LINE_UART_PORT1.Init.WordLength = UART_WORDLENGTH_8B;
		KL_LINE_UART_PORT1.Init.StopBits = UART_STOPBITS_1;
		KL_LINE_UART_PORT1.Init.Parity = UART_PARITY_NONE;
		KL_LINE_UART_PORT1.Init.Mode = UART_MODE_TX_RX;
		KL_LINE_UART_PORT1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
		KL_LINE_UART_PORT1.Init.OverSampling = UART_OVERSAMPLING_16;
		KL_LINE_UART_PORT1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
		KL_LINE_UART_PORT1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
		KL_LINE_UART_PORT1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
		if (HAL_UART_Init(&KL_LINE_UART_PORT1) != HAL_OK)
		{
			return -1;
		}
		HAL_UART_Receive_IT( &KL_LINE_UART_PORT1, &gucKL1RxDummy, 1 );
		return 0;
	}
	else// if(Line==KL_LINE2)
	{
		KL_LINE_UART_PORT2.Instance = USART3;
		KL_LINE_UART_PORT2.Init.BaudRate = Baudrate;
		KL_LINE_UART_PORT2.Init.WordLength = UART_WORDLENGTH_8B;
		KL_LINE_UART_PORT2.Init.StopBits = UART_STOPBITS_1;
		KL_LINE_UART_PORT2.Init.Parity = UART_PARITY_NONE;
		KL_LINE_UART_PORT2.Init.Mode = UART_MODE_TX_RX;
		KL_LINE_UART_PORT2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
		KL_LINE_UART_PORT2.Init.OverSampling = UART_OVERSAMPLING_16;
		KL_LINE_UART_PORT2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
		KL_LINE_UART_PORT2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
		KL_LINE_UART_PORT2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
		if (HAL_UART_Init(&KL_LINE_UART_PORT2) != HAL_OK)
		{
			return -1;
		}
		HAL_UART_Receive_IT( &KL_LINE_UART_PORT2, &gucKL2RxDummy, 1 );
		return 0;
	}
}

/*----------------------------------------------------------------------
 *   Thread
 *--------------------------------------------------------------------*/
