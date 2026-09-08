/**
 * @file		git_ndlogger.h
 * @version		0.1
 *
 */
 
#ifndef GIT_NDLOGGER_H
#define GIT_NDLOGGER_H

//! To enable concurrent mode
#define GIT_CONCURRENT_MODE							RSI_DISABLE

////////////////////////////////////////////////////////
//! opermode command paramaters
/*=======================================================================*/
//! To set wlan feature select bit map
// WiFi AP - FEAT_SECURITY_PSK
#define GIT_FEATURE_BIT_MAP							(FEAT_SECURITY_OPEN)  //! FEAT_SECURITY_OPEN - BIT(0)

//! TCP IP BYPASS feature check
#define GIT_TCP_IP_BYPASS							RSI_DISABLE

//! TCP/IP feature select bitmap for selecting TCP/IP features
// WiFi Station - (TCP_IP_FEAT_DHCPV4_CLIENT|TCP_IP_FEAT_ICMP), WiFi AP - TCP_IP_FEAT_DHCPV4_SERVER
#define GIT_TCP_IP_FEATURE_BIT_MAP					(TCP_IP_FEAT_DHCPV4_CLIENT|TCP_IP_FEAT_SSL)

//! To set custom feature select bit map 
// WiFi Station - FEAT_CUSTOM_FEAT_EXTENTION_VALID
#define GIT_CUSTOM_FEATURE_BIT_MAP					RSI_DISABLE 
//#define GIT_CUSTOM_FEATURE_BIT_MAP					FEAT_CUSTOM_FEAT_EXTENTION_VALID

//! To set Extended custom feature select bit map 
#define GIT_EXT_CUSTOM_FEATURE_BIT_MAP				EXT_FEAT_256K_MODE    

//! Band command paramters
/*=======================================================================*/
//! RSI_BAND_2P4GHZ(2.4GHz) or RSI_BAND_5GHZ(5GHz) or RSI_DUAL_BAND
#define GIT_BAND									RSI_DUAL_BAND

//! Timeout for ping request
/*=======================================================================*/
#define GIT_PING_REQ_TIMEOUT						1

//! Join command parameters
/*=======================================================================*/
//! RSI_JOIN_FEAT_STA_BG_ONLY_MODE_ENABLE or RSI_JOIN_FEAT_LISTEN_INTERVAL_VALID
#define GIT_JOIN_FEAT_BIT_MAP						0

#endif