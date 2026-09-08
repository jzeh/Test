/*----------------------------------------------------------------------
 *   Ethernet Control
 *--------------------------------------------------------------------*/
#ifndef	__GIT_ETH_H__
#define	__GIT_ETH_H__

/*----------------------------------------------------------------------
 *   Include
 *--------------------------------------------------------------------*/
#ifdef VCI3_DIAG
#include "error.h"
#endif
#include "core/net.h"

/*----------------------------------------------------------------------
 *   Defines
 *--------------------------------------------------------------------*/

#define TCP_LOCL_PORT										52081			//ADAS_DRV 2

#define ETH_MAX_DATA_SIZE									1500

//First Ethernet interface configuration
#define APP_IF1_NAME										"VCI3"
#define APP_IF1_HOST_NAME									"ethdiag-server-demo-1"
#define APP_IF1_MAC_ADDR									"02-00-00-00-05-00"
#define APP_VLAN_ID											0x081

#if defined (KKT_TEST)
#define APP_IF1_IPV4_HOST_ADDR								"192.168.0.77"
#define APP_IF1_IPV4_SUBNET_MASK							"255.255.255.0"
#define APP_IF1_IPV4_DEFAULT_GATEWAY						"192.168.0.1"
#define APP_IF1_IPV4_PRIMARY_DNS							"8.8.8.8"
#define APP_IF1_IPV4_SECONDARY_DNS							"8.8.4.4"

#define TCP_TARGET_PORT										13402
#define TCP_TARGET_NAME										"192.168.0.100"
#else
#define APP_IF1_IPV4_HOST_ADDR								"10.0.5.0"
#define APP_IF1_IPV4_SUBNET_MASK							"255.0.0.0"
#define APP_IF1_IPV4_DEFAULT_GATEWAY						"10.0.128.1"
//#define APP_IF1_IPV4_DEFAULT_GATEWAY						"192.168.0.1"
#define APP_IF1_IPV4_PRIMARY_DNS							"8.8.8.8"
#define APP_IF1_IPV4_SECONDARY_DNS							"8.8.4.4"

#define TCP_TARGET_PORT										13402
#define TCP_TARGET_NAME										"10.32.0.0"
#endif

#define ETH_PROTOCOL_VER_ICU								0xCC
#define ETH_PROTOCOL_VER_CCU								0xCA
#define ETH_INV_PROTOCOL_VER_ICU							0x33
#define ETH_INV_PROTOCOL_VER_CCU							0x35

#define ETH_PAYLOAD_TYPE_ALIVE								0x0007
#define ETH_PAYLOAD_TYPE_DIAG								0x8001
#define ETH_PAYLOAD_TYPE_MASS								0xFCBC

#define SOCKET_TEST											1				// ?

/*----------------------------------------------------------------------
 *   Typedef
 *--------------------------------------------------------------------*/
typedef struct _stETHPkt
{
	uint16_t	mTimeStamp;
	uint32_t	mLen;
	uint8_t		mData[1];
} stETHPkt;

typedef enum
{
	eSOCK_STATE_UNUSE,
	eSOCK_STATE_CLOSE,
	eSOCK_STATE_INIT,
	eSOCK_STATE_OPEN,
	eSOCK_STATE_BIND,
	eSOCK_STATE_CONNECT,
	eSOCK_STATE_CONNECTED,
	eSOCK_STATE_SHUTDOWN,
	eSOCK_STATE_FAIL,
//	eSOCK_STATE_CLIENT_HANDSHAKE,
//	eSOCK_STATE_SERVER_HANDSHAKE,
//	eSOCK_STATE_SERVER_RESP_BODY,
//	eSOCK_STATE_OPEN,
//	eSOCK_STATE_CLOSING_TX,
//	eSOCK_STATE_CLOSING_RX,
} eSocketState;

typedef struct _stEthDiagSocket
{
	eSocketState	state;
	Socket			*socket;
} stEthDiagSocket_t;

typedef struct _stEthDiagHeader
{
	uint8_t 	ucProtocolVer;
	uint8_t 	ucInvProtocolVer;
	uint8_t 	usPayloadType[2];
	uint8_t 	uiPayloadLen[4];
} stEthDiagHeader_t;

typedef struct _stMsgEthDiag_t
{
	SocketEvent		eEvent;
	void			*pPacket;
} stMsgEthDiag_t;

typedef struct _stEthDiagPkt
{
	stEthDiagHeader_t	stHeader;
	uint8_t				ucPlayload[ETH_MAX_DATA_SIZE];
} stEthDiagPkt_t;

/*----------------------------------------------------------------------
 *   Global Functions
 *--------------------------------------------------------------------*/
extern int32_t		InitEthernet( void );
extern error_t		eth1InterfaceInit(void);
extern eSocketState	ethTCPConnect(stEthDiagSocket_t *ethSocket);
extern void			setEthSocketState(stEthDiagSocket_t *socket, eSocketState state);
extern eSocketState	getEthSocketState(stEthDiagSocket_t *socket);

extern void			eth_SetIPAddressStatic( uint32_t ip_addr, uint32_t network_mask, uint32_t gateway );

/*----------------------------------------------------------------------
 *   Global Variables
 *--------------------------------------------------------------------*/
extern stEthDiagSocket_t g_ethSocket;

#endif // __GIT_ETH_H__
