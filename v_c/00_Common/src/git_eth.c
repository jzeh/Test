/*************************************************************
 * NOTE : git_eth.c
 *      Ethernet control(With CycloneTCP)
 * Author : Lee junho
 * Since : 2021.02.01
**************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32h743xx.h"

#include "main.h"
#include "common.h"
#include "typedef.h"
#include "git_vci.h"
#include "git_protocol.h"
#include "git_pool.h"
#ifdef VCI3_DIAG
#include "git_eth.h"
#include "core/net.h" //mod.kks test
#endif
#include "drivers/switch/lan937x_driver.h"
#include "drivers/mac/stm32h7xx_eth_driver.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/
#define	DEBUG_SPI_RX_ENABLE								0
#define	DEBUG_SPI_TX_ENABLE								0

#define APP_HTTP_MAX_CONNECTIONS 4

#define HTTP_PORT										80									//HTTP port number
#define HTTPS_PORT										443									//HTTPS port number (HTTP over TLS)

#define SOCKET_BIND_TIMEOUT								3000

/*----------------------------------------------------------------------
 *   Functions declaration
 *--------------------------------------------------------------------*/
uint8_t initTCP( void );
void TCPTrasnmitThread( void const *argument);
void TCPReceiveThread( void const *argument);

/*----------------------------------------------------------------------
 *   Variables
 *--------------------------------------------------------------------*/
osMessageQId	hTCPTxMessage;
osMessageQId	hTCPRxMessage;
osMessageQId	hTCPHandleMessage;

osMessageQDef( tcphandlequeue, MESSAGE_DIAGNOSTIC_QUEUE_SIZE, int );
osMessageQDef( tcptxqueue, MESSAGE_DIAGNOSTIC_QUEUE_SIZE, int );
osMessageQDef( tcprxqueue, MESSAGE_DIAGNOSTIC_QUEUE_SIZE, int );

osThreadId		hTCPTxTh;
osThreadId		hTCPRxTh;
osThreadDef( TCPTxTh,	TCPTrasnmitThread,	osPriorityNormal, 0, 1100 );
osThreadDef( TCPRxTh,	TCPReceiveThread,	osPriorityNormal, 0, 1100 );

stEthDiagSocket_t g_ethSocket;

#if( VCI_III_ASING_MODE )
extern uint8_t		g_TriggerKey;
#endif
#ifdef PRINT_MESSAGE_ID
	extern stMESSAGE_ID_INFO stMessageIdInfo[20];
 	extern uint8_t ucMessageIdInfoCnt;
#endif

/*----------------------------------------------------------------------
 *   Functions definition
 *--------------------------------------------------------------------*/
int32_t InitEthernet( void )
{
	//Configure the first Ethernet interface
	eth1InterfaceInit();

	return 0;
}

error_t eth1InterfaceInit(void)
{
	error_t			error;
	NetInterface	*interface;
	MacAddr			macAddr;

	//TCP/IP stack initialization
	error = netInit();
	if(error)
	{
		//Debug message
		GLogEE( "Failed to initialize TCP/IP stack!(%d)\r\n", error );
		return error;
	}

	//Configure the first network interface
	interface = &netInterface[0];

	netSetInterfaceName(interface, APP_IF1_NAME);						// Set interface name
	netSetHostname(interface, APP_IF1_HOST_NAME);						// Set host name
	macStringToAddr(APP_IF1_MAC_ADDR, &macAddr);						// Set host MAC address
	netSetMacAddr(interface, &macAddr);
	netSetDriver(interface, &stm32h7xxEthDriver);						// Select the relevant MAC driver
	netSetSwitchDriver(interface, &lan937xSwitchDriver);				// Select the relevant switch driver
	netSetSwitchPort(interface, LAN937x_PORT1);							// Enable special VLAN tagging mode

#if( VCI_III_ASING_MODE )
	if( g_TriggerKey == 1 )		netSetVlanId(interface, APP_VLAN_ID);	//Set VLAN identifier
#else
	netSetVlanId(interface, APP_VLAN_ID);								//Set VLAN identifier
#endif

	//Initialize network interface
	error = netConfigInterface(interface);
	if(error)
	{
		GLogEE( "Failed to configure interface %s(%d)!\r\n", interface->name, error );
		return error;
	}

	return NO_ERROR;
}

void eth_SetIPAddressStatic( uint32_t ip_addr, uint32_t network_mask, uint32_t gateway )
{
	Ipv4Addr		ipv4Addr;
	NetInterface	*interface;
	interface = &netInterface[0];

	//Set IPv4 host address
	ipv4SetHostAddr(interface, ip_addr);

	//Set subnet mask
	ipv4SetSubnetMask(interface, network_mask);

	//Set default gateway
	ipv4SetDefaultGateway(interface, gateway);

#if 1
	//Set primary and secondary DNS servers
	ipv4StringToAddr(APP_IF1_IPV4_PRIMARY_DNS, &ipv4Addr);
	ipv4SetDnsServer(interface, 0, ipv4Addr);
	ipv4StringToAddr(APP_IF1_IPV4_SECONDARY_DNS, &ipv4Addr);
	ipv4SetDnsServer(interface, 1, ipv4Addr);
#endif
}

uint8_t initTCP( void )
{
	hTCPHandleMessage = osMessageCreate( osMessageQ( tcphandlequeue ), NULL );
	if( hTCPHandleMessage == NULL )
	{
		GLogE( "error... osMessageCreate hTCPHandleMessage\r\n" );
		return INIT_FAIL;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hTCPHandleMessage;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hTCPHandleMessage",sizeof("hTCPHandleMessage"));
#endif


	hTCPTxMessage = osMessageCreate( osMessageQ( tcptxqueue ), NULL );
	if( hTCPTxMessage == NULL )
	{
		GLogE( "error... osMessageCreate hTCPTxMessage\r\n" );
		return INIT_FAIL;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hTCPTxMessage;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hTCPTxMessage",sizeof("hTCPTxMessage"));
#endif


	hTCPRxMessage = osMessageCreate( osMessageQ( tcprxqueue ), NULL );
	if( hTCPRxMessage == NULL )
	{
		GLogE( "error... osMessageCreate hTCPRxMessage\r\n" );
		return INIT_FAIL;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hTCPRxMessage;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hTCPRxMessage",sizeof("hTCPRxMessage"));
#endif


	// Threads
	hTCPRxTh = osThreadCreate( osThread(TCPRxTh), NULL );
	if( hTCPRxTh == NULL )
	{
		GLogE( "Error... fail create hTCPHandleTh Thread!!!\r\n" );
		return INIT_FAIL;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hTCPRxTh;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hTCPRxTh",sizeof("hTCPRxTh"));
#endif


	hTCPTxTh = osThreadCreate( osThread(TCPTxTh), NULL );
	if( hTCPTxTh == NULL )
	{
		GLogE( "Error... fail create hTCPHandleTh Thread!!!\r\n" );
		return INIT_FAIL;
	}
#ifdef PRINT_MESSAGE_ID
	stMessageIdInfo[ucMessageIdInfoCnt].uiID=(uint32_t)hTCPTxTh;
	memcpy(stMessageIdInfo[ucMessageIdInfoCnt++].ucIdName,"hTCPTxTh",sizeof("hTCPTxTh"));
#endif


	return INIT_OK;
}

void setEthSocketState(stEthDiagSocket_t *socket, eSocketState state)
{
	socket->state = state;
}

eSocketState getEthSocketState(stEthDiagSocket_t *socket)
{
	return socket->state;
}

eSocketState ethTCPConnect(stEthDiagSocket_t *ethSocket)
{
	error_t			error;
//	IpAddr			localIpAddr;
	IpAddr			serverIpAddr;
//	uint16_t		localPort;
	uint16_t		serverPort;
//	NetInterface	*interface;

//	interface = netInterface;

	while( ethSocket->state != eSOCK_STATE_CONNECTED )
	{
		switch( ethSocket->state )
		{
			case eSOCK_STATE_INIT:
			{
//				localIpAddr.ipv4Addr	= g_stGITHWSetDataEth.nSourceIP_T1;
//				localIpAddr.length		= sizeof(g_stGITHWSetDataEth.nSourceIP_T1);
//				localPort				= g_stGITHWSetDataEth.nSourcePort_T1;

				serverIpAddr.ipv4Addr	= g_stGITHWSetDataEth.nDestinationIp_T1;
				serverIpAddr.length		= sizeof(g_stGITHWSetDataEth.nDestinationIp_T1);
				serverPort				= g_stGITHWSetDataEth.nDestinationPort_T1;

				setEthSocketState( ethSocket, eSOCK_STATE_OPEN );
				break;
			}

			case eSOCK_STATE_OPEN:
			{
				//Open a TCP socket
				ethSocket->socket = socketOpen(SOCKET_TYPE_STREAM, SOCKET_IP_PROTO_TCP);
				if( ethSocket->socket == NULL )
				{
					GLogEE( "Socket Open Error!!! \r\n" );

					setEthSocketState(ethSocket, eSOCK_STATE_FAIL);
					break;
				}

				setEthSocketState(ethSocket, eSOCK_STATE_BIND);
				break;
			}

			case eSOCK_STATE_BIND:
			{
/*
				//Associate the socket with the relevant interface
				socketBindToInterface( ethSocket->socket, interface );
				socketBind( ethSocket->socket, &localIpAddr, localPort );
*/
				error = socketSetTimeout( ethSocket->socket, SOCKET_BIND_TIMEOUT );				//Set timeout
				if( error )
				{
					GLogE( "Socket Set BIND Error!!! \r\n" );

					socketClose(ethSocket->socket);
					setEthSocketState(ethSocket, eSOCK_STATE_FAIL);
					break;
				}

				setEthSocketState(ethSocket, eSOCK_STATE_CONNECT);
				break;
			}

			case eSOCK_STATE_CONNECT:
			{
				error = socketConnect(ethSocket->socket, &serverIpAddr, serverPort);
				if(error)
				{
					GLogE( "Socket Connect Error(%d)!!! \r\n", error );
					socketClose(ethSocket->socket);

					setEthSocketState(ethSocket, eSOCK_STATE_FAIL);
					break;
				}

				setEthSocketState(ethSocket, eSOCK_STATE_CONNECTED);
				break;
			}

			case eSOCK_STATE_CLOSE:
			{
				error = NO_ERROR;
				socketClose(ethSocket->socket);
				setEthSocketState(ethSocket, eSOCK_STATE_UNUSE);
				break;
			}

			case eSOCK_STATE_SHUTDOWN:
			{
				socketShutdown(ethSocket->socket,2);
				socketClose(ethSocket->socket);
				setEthSocketState(ethSocket, eSOCK_STATE_UNUSE);
				error = NO_ERROR;
				break;
			}

			default :
			{
				GLogEE( "Unknown Socket state!!!\r\n" );
				break;
			}
		}

		if((getEthSocketState(ethSocket) == eSOCK_STATE_FAIL) || (getEthSocketState(ethSocket) == eSOCK_STATE_UNUSE))			break;
	}

	return (getEthSocketState(ethSocket));
}

void TCPTrasnmitThread(void const *argument)
{
//  	osEvent evt;
//	error_t error;
//	stMsgEthDiag_t		*message, *pEthMsg;
//	stEthDiagPkt_t		*packet, *pEthPacket;
 // 	uint32_t sendDataLen = 0;

	for(;;)
	{
/*
		evt =  osMessageGet( hTCPTxMessage, osWaitForever );
		if( evt.status == osEventMessage )
		{
		  	message	= ( stMsgEthDiag_t* )evt.value.p;
			packet	= ( stEthDiagPkt_t* )message->pPacket;

			pEthMsg = ( stMsgEthDiag_t* ) osPoolCAlloc( hTCPMsgPool );
			if( pEthMsg == NULL )
			{
			  	break;
			}
			pEthPacket = ( stEthDiagPkt_t* )osPoolCAlloc( hTCPPktPool );
			if( pEthPacket == NULL )
			{
			  	osPoolFree( hTCPMsgPool, (void *)pEthMsg );
				break;
			}

			memcpy(pEthPacket, packet, sizeof(stEthDiagPkt_t));
			memcpy(pEthMsg, message, sizeof(stMsgEthDiag_t));
			pEthMsg->pPacket = pEthPacket;

			osPoolFree( hTCPPktPool, (void *)packet );
			osPoolFree( hTCPMsgPool, (void *)message );

			for(uint8_t i = 0; i<sizeof(packet->stHeader.uiPayloadLen); i++)
			{
				sendDataLen |= (packet->stHeader.uiPayloadLen[i] << (24-(i*8)));
			}

			sendDataLen += sizeof(stEthDiagHeader_t);

			error = socketSend(g_ethSocket.socket, (uint8_t*)pEthPacket, (size_t)sendDataLen, NULL, FALSE);

			if(!error)
			{
				//DATA_G_LED_TOGGLE;
				osPoolFree( hTCPPktPool, (void *)pEthPacket );
				osPoolFree( hTCPMsgPool, (void *)pEthMsg );
			}
			else
			{
				//GLogE("%d", error);
//				if(error == ERROR_TIMEOUT)
//				{
//				  	if(osMessageAvailableSpace(hTCPRxMessage) == 0)
//					{
//						osPoolFree( gsPropertie.hTCPPktPool, (void *)pEthPacket );
//						osPoolFree( gsPropertie.hTCPMsgPool, (void *)pEthMsg );
//					}
//					else
//					{
//					  	osDelay(1);
//						osMessagePut( hTCPTxMessage, (uint32_t)pEthMsg, osWaitForever );
//					}
//				}
//				else
				{
				  	osPoolFree( hTCPPktPool, (void *)pEthPacket );
					osPoolFree( hTCPMsgPool, (void *)pEthMsg );
				}
			}
			sendDataLen = 0;
		}
*/
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        osThreadYield();
#endif
	}
}

void TCPReceiveThread( void const *argument)
{
//  	osEvent evt;
//	error_t error;
//	stMsgEthDiag_t		*message, *pEthMsg;
//	stEthDiagPkt_t		*packet, *pEthPacket;

  	for(;;)
	{
/*
		evt =  osMessageGet( hTCPHandleMessage, osWaitForever );
		if( evt.status == osEventMessage )
		{
			message	= ( stMsgEthDiag_t* )evt.value.p;
			packet	= ( stEthDiagPkt_t* )message->pPacket;

			pEthMsg = ( stMsgEthDiag_t* ) osPoolCAlloc( hTCPMsgPool );
			if( pEthMsg == NULL )
			{
			  	break;
			}
			pEthPacket = ( stEthDiagPkt_t* )osPoolCAlloc( hTCPPktPool );
			if( pEthPacket == NULL )
			{
			  	osPoolFree( hTCPMsgPool, (void *)pEthMsg );
				break;
			}

			memcpy(pEthPacket, packet, sizeof(stEthDiagPkt_t));
			memcpy(pEthMsg, message, sizeof(stMsgEthDiag_t));
			pEthMsg->pPacket = pEthPacket;

			osPoolFree( hTCPPktPool, (void *)packet );
			osPoolFree( hTCPMsgPool, (void *)message );

			error = socketReceive(g_ethSocket.socket, pEthPacket, sizeof(stEthDiagPkt_t), NULL, FALSE);

			if(!error)
			{
				pEthMsg->pPacket = pEthPacket;
				if(osMessageAvailableSpace(hTCPRxMessage) == 0)
				{
					osPoolFree( hTCPPktPool, (void *)pEthPacket );
					osPoolFree( hTCPMsgPool, (void *)pEthMsg );
				}
				else
				{
				  	//if(pEthPacket->stHeader.ucInvProtocolVer == 0x00)
					//GLogE("%02X\r\n", pEthPacket->stHeader.ucProtocolVer);
				  	osMessagePut( hTCPRxMessage, (uint32_t)pEthMsg, osWaitForever );
				}
			}
			else
			{
			  	osPoolFree( hTCPPktPool, (void *)pEthPacket );
				osPoolFree( hTCPMsgPool, (void *)pEthMsg );
			}
//			TCPStartRxThread();
		}
		//osDelay(1)
*/
#if defined(FEATURE_DETERMINE_ECU_TO_UPGRADE)
        osThreadYield();
#endif
	}
}
