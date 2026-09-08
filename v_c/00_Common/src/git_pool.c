/*************************************************************
 * NOTE : git_can.c
 *      FDCAN control
 * Author : Lee junho
 * Since : 2019.09.03
**************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "gpio.h"
#include "cmsis_os.h"

#include "common.h"
#include "typedef.h"
#include "git_can.h"

#include "git_pool.h"
#include "git_protocol.h"
#include "GIT_PassThruDefines.h"
#ifdef VCI3_DIAG
#include "git_mcp2518fd.h"
#endif

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/

#ifdef OS_POOL_ID_PRINT
stOS_POOL_ID_INFO stOsPoolIdInfo[10];
int8_t g_ucOSPoolInfoIndex=0;
#endif

#if defined(__ICCARM__)
    #define SRAM2_LOC   _Pragma("location=\".sram2_pool\"") __root
#else
    #define SRAM2_LOC   __attribute__((section(".sram2_pool")))
#endif

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
int32_t InitCanPool( void );
int32_t InitMessagePool( void );
int32_t InitSpiCanPool( void );


/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
// Can
osPoolId		hFdcanPktPool;
osPoolDef( canpktpool, CAN_PACKET_POOL_SIZE, stFdcanPkt );
osPoolId		hFdcanMsgPool;
osPoolDef( fdmsgpool, FDCAN_POOL_SIZE, stMsgClst );
#ifdef VCI3_DIAG
// Spi Can
osPoolId		hSpiCanPktPool;
osPoolDef( spicanpktpool, SPI_CAN_PACKET_POOL_SIZE, SpiCanPkt_t );
#endif
// Communication
osPoolId		hCommPKPool;
osPoolDef( commpktpool, COMM_PACKET_POOL_SIZE, stCommPkt );

// MQTT Compacket
osPoolId		hMqttPKPool;
osPoolDef( mqttpktpool, MQTT_PACKET_POOL_SIZE, stCommPkt );

// Message
osPoolId		hMsgPool;
osPoolDef( msgpool, MESSAGE_POOL_SIZE, stMsgClst );

// Ethernet Packet
osPoolId		hTCPPktPool;
osPoolDef( tcppktpool, MESSAGE_DIAGNOSTIC_QUEUE_SIZE, stMsgClst );

// Ethernet Message
osPoolId		hTCPMsgPool;
osPoolDef( tcpmsgpool, MESSAGE_DIAGNOSTIC_QUEUE_SIZE, stMsgClst );

// Diag Message
osPoolId		hDiagPool;
osPoolDef( diagpool, MESSAGE_DIAGNOSTIC_QUEUE_SIZE, MsgDiag_t );

// Diag Packet
osPoolId		hPTPKPool;
osPoolDef( ptpkpool, MESSAGE_DIAGNOSTIC_QUEUE_SIZE, PTmsgPkt_t );

#ifdef USE_RELAY_MOSA
// Battery Relay Control Message
osPoolId		hBatRelayConPool;
osPoolDef( batpool, MESSAGE_DIAGNOSTIC_QUEUE_SIZE, MsgDiag_t );

// Battery Relay Control Packet
osPoolId		hBatRelayConPKPool;
osPoolDef( batpkpool, MESSAGE_DIAGNOSTIC_QUEUE_SIZE, PTmsgPkt_t );
#endif

/* Pool Semaphore */
osSemaphoreId	hMqttPKPoolSemaphore;
osSemaphoreDef( MqttPktPoolSem );

osSemaphoreId   hpairflagSemaphore;
osSemaphoreDef( pairflagSem );


/*----------------------------------------------------------------------
 *   Functions
 *--------------------------------------------------------------------*/
int32_t InitCanPool( void )
{
	// packet memory poll
	hFdcanPktPool = osPoolCreate( osPool( canpktpool ) );
	if( hFdcanPktPool == NULL )
	{
		return INIT_FAIL;
	}
#ifdef OS_POOL_ID_PRINT
	GLogN( "hFdcanPktPool:0x%X\r\n", hFdcanPktPool );
	stOsPoolIdInfo[g_ucOSPoolInfoIndex].uiID=(uint32_t)hFdcanPktPool;
	memcpy(stOsPoolIdInfo[g_ucOSPoolInfoIndex++].ucIdName,"hFdcanPktPool",sizeof("hFdcanPktPool"));
#endif

	hFdcanMsgPool = osPoolCreate( osPool( fdmsgpool ) );
	if( hFdcanMsgPool == NULL )
	{
		return INIT_FAIL;
	}
#ifdef OS_POOL_ID_PRINT
	GLogN( "hFdcanMsgPool:0x%X\r\n", hFdcanMsgPool );
	stOsPoolIdInfo[g_ucOSPoolInfoIndex].uiID=(uint32_t)hFdcanMsgPool;
	memcpy(stOsPoolIdInfo[g_ucOSPoolInfoIndex++].ucIdName,"hFdcanMsgPool",sizeof("hFdcanMsgPool"));
#endif

	return INIT_OK;
}
#ifdef VCI3_DIAG
int32_t InitSpiCanPool( void )
{
#ifdef MCP2518_LOG
	GLogN( "[MCP2518] +%s\r\n", __FUNCTION__ );
#endif
	// packet memory poll
	hSpiCanPktPool = osPoolCreate( osPool( spicanpktpool ) );
	if( hSpiCanPktPool == NULL )
	{
		return INIT_FAIL;
	}
#ifdef OS_POOL_ID_PRINT
	GLogN( "hSpiCanPktPool:0x%X\r\n", hSpiCanPktPool );
	stOsPoolIdInfo[g_ucOSPoolInfoIndex].uiID=(uint32_t)hSpiCanPktPool;
	memcpy(stOsPoolIdInfo[g_ucOSPoolInfoIndex++].ucIdName,"hSpiCanPktPool",sizeof("hSpiCanPktPool"));
#endif

	return INIT_OK;
}
#endif
int32_t InitCommPool( void )
{
  	// packet memory poll
	hMqttPKPool = osPoolCreate( osPool( mqttpktpool ) );
	if( hMqttPKPool == NULL )
	{
		return INIT_FAIL;
	}
	// packet memory poll
	hCommPKPool = osPoolCreate( osPool( commpktpool ) );
	if( hCommPKPool == NULL )
	{
		return INIT_FAIL;
	}
#ifdef OS_POOL_ID_PRINT
	GLogN( "hCommPKPool:0x%X\r\n", hCommPKPool );
	stOsPoolIdInfo[g_ucOSPoolInfoIndex].uiID=(uint32_t)hCommPKPool;
	memcpy(stOsPoolIdInfo[g_ucOSPoolInfoIndex++].ucIdName,"hCommPKPool",sizeof("hCommPKPool"));
#endif

	return INIT_OK;
}

int32_t InitMessagePool( void )
{
	// message memory poll
	hMsgPool = osPoolCreate( osPool( msgpool ) );
	if( hMsgPool == NULL )
	{
                GLogE( "error... osPoolCreate hMsgPool\r\n" );
	        return INIT_FAIL;
	}
#ifdef OS_POOL_ID_PRINT
	GLogN( "hMsgPool:0x%X\r\n", hMsgPool );
	stOsPoolIdInfo[g_ucOSPoolInfoIndex].uiID=(uint32_t)hMsgPool;
	memcpy(stOsPoolIdInfo[g_ucOSPoolInfoIndex++].ucIdName,"hMsgPool",sizeof("hMsgPool"));
#endif

	return INIT_OK;
}

int32_t	InitEthPool( void )
{
	hTCPMsgPool = osPoolCreate( osPool( tcpmsgpool ) );
	if( hTCPMsgPool == NULL )
	{
		GLogE( "error... osPoolCreate hTCPMsgPool\r\n" );
		return INIT_FAIL;
	}
#ifdef OS_POOL_ID_PRINT
	GLogN( "hTCPMsgPool:0x%X\r\n", hTCPMsgPool );
	stOsPoolIdInfo[g_ucOSPoolInfoIndex].uiID=(uint32_t)hTCPMsgPool;
	memcpy(stOsPoolIdInfo[g_ucOSPoolInfoIndex++].ucIdName,"hTCPMsgPool",sizeof("hTCPMsgPool"));
#endif

	hTCPPktPool = osPoolCreate( osPool( tcppktpool ) );
	if( hTCPPktPool == NULL )
	{
		GLogE( "error... osPoolCreate hTCPPktPool\r\n" );
		return INIT_FAIL;
	}
#ifdef OS_POOL_ID_PRINT
	GLogN( "hTCPPktPool:0x%X\r\n", hTCPPktPool );
	stOsPoolIdInfo[g_ucOSPoolInfoIndex].uiID=(uint32_t)hTCPPktPool;
	memcpy(stOsPoolIdInfo[g_ucOSPoolInfoIndex++].ucIdName,"hTCPPktPool",sizeof("hTCPPktPool"));
#endif

	return INIT_OK;
}

int32_t	InitDiagPool( void )
{
	hDiagPool = osPoolCreate( osPool( diagpool ) );
	if( hDiagPool == NULL )
	{
		GLogE( "error... osPoolCreate hDiagPool\r\n" );
		return INIT_FAIL;
	}
#ifdef OS_POOL_ID_PRINT
	GLogN( "hDiagPool:0x%X\r\n", hDiagPool );
	stOsPoolIdInfo[g_ucOSPoolInfoIndex].uiID=(uint32_t)hDiagPool;
	memcpy(stOsPoolIdInfo[g_ucOSPoolInfoIndex++].ucIdName,"hDiagPool",sizeof("hDiagPool"));
#endif

	hPTPKPool = osPoolCreate( osPool( ptpkpool ) );
	if( hPTPKPool == NULL )
	{
		GLogE( "error... osPoolCreate hPTPKPool\r\n" );
		return INIT_FAIL;
	}
#ifdef OS_POOL_ID_PRINT
	GLogN( "hPTPKPool:0x%X\r\n", hPTPKPool );
	stOsPoolIdInfo[g_ucOSPoolInfoIndex].uiID=(uint32_t)hPTPKPool;
	memcpy(stOsPoolIdInfo[g_ucOSPoolInfoIndex++].ucIdName,"hPTPKPool",sizeof("hPTPKPool"));
#endif
	
	return INIT_OK;
}

int32_t InitSemaphore(void)
{
    hMqttPKPoolSemaphore = osSemaphoreCreate( osSemaphore(MqttPktPoolSem), MQTT_PACKET_POOL_SIZE );
    if (hMqttPKPoolSemaphore == NULL)
    {
        GLogE("error... osSemaphoreCreate hMqttPKPoolSemaphore\r\n");
        return INIT_FAIL;
    }

	hpairflagSemaphore = osSemaphoreCreate( osSemaphore(pairflagSem), 1 );
    if (hpairflagSemaphore == NULL)
    {
        GLogE("error... osSemaphoreCreate hpairflagSemaphore\r\n");
        return INIT_FAIL;
    }
	
    return INIT_OK;
}

#ifdef USE_RELAY_MOSA
int32_t InitBatRelayConPool(void)
{
	hBatRelayConPool = osPoolCreate( osPool( batpool ) );
	if( hBatRelayConPool == NULL )
	{
		GLogE( "error... osPoolCreate hBatRelayConPool\r\n" );
		return INIT_FAIL;
	}
#ifdef OS_POOL_ID_PRINT
	GLogN( "hBatRelayConPool:0x%X\r\n", hBatRelayConPool );
	stOsPoolIdInfo[g_ucOSPoolInfoIndex].uiID=(uint32_t)hBatRelayConPool;
	memcpy(stOsPoolIdInfo[g_ucOSPoolInfoIndex++].ucIdName,"hBatRelayConPool",sizeof("hBatRelayConPool"));
#endif

	hBatRelayConPKPool = osPoolCreate( osPool( batpkpool ) );
	if( hBatRelayConPKPool == NULL )
	{
		GLogE( "error... osPoolCreate hBatRelayConPKPool\r\n" );
		return INIT_FAIL;
	}
#ifdef OS_POOL_ID_PRINT
	GLogN( "hBatRelayConPKPool:0x%X\r\n", hBatRelayConPKPool );
	stOsPoolIdInfo[g_ucOSPoolInfoIndex].uiID=(uint32_t)hBatRelayConPKPool;
	memcpy(stOsPoolIdInfo[g_ucOSPoolInfoIndex++].ucIdName,"hBatRelayConPKPool",sizeof("hBatRelayConPKPool"));
#endif
	
	return INIT_OK;

}
#endif

#ifdef OS_POOL_ID_PRINT
char* GetOsIDName(uint32_t uiIndex)
{
	for(int i=0;i<10;i++)
	{
		if( stOsPoolIdInfo[i].uiID == uiIndex )
			return stOsPoolIdInfo[i].ucIdName;
	}
	return "NotFind";
}
#endif
