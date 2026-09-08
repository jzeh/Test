/**
 * @file lan937x_driver.c
 * @brief LAN937x 3-port Ethernet switch driver
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2021 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneTCP Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.2
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL			NIC_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "drivers/switch/lan937x_driver.h"
#include "debug.h"

#include "git_lan9371.h"

/**
 * @brief LAN937x Ethernet switch driver
 **/
const SwitchDriver lan937xSwitchDriver =
{
	lan937xInit,
	lan937xTick,
	lan937xEnableIrq,
	lan937xDisableIrq,
	lan937xEventHandler,
	lan937xTagFrame,
	lan937xUntagFrame,
	lan937xGetLinkState,
	lan937xGetLinkSpeed,
	lan937xGetDuplexMode,
	lan937xSetPortState,
	lan937xGetPortState,
	lan937xSetAgingTime,
	lan937xEnableRsvdMcastTable,
	lan937xAddStaticFdbEntry,
	lan937xDeleteStaticFdbEntry,
	lan937xGetStaticFdbEntry,
	lan937xFlushStaticFdbTable,
	lan937xGetDynamicFdbEntry,
	lan937xFlushDynamicFdbTable
};

/**
 * @brief LAN937x Ethernet switch initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/
error_t lan937xInit(NetInterface *interface)
{
	uint32_t	value;
	uint16_t	t1_tx_enable;
	uint16_t	status;

	//Initialize serial management interface
	if(interface->smiDriver != NULL)
	{
		interface->smiDriver->init();
	}

	//The completion of the entire chip-level reset must then be determined
	//by polling the READY bit of the HW_CFG register
	do
	{
		//Read HW_CFG register
		value = lan937xReadSysReg(interface, LAN937x_HW_CFG);
	} while((value & LAN937x_HW_CFG_DEVICE_READY) == 0);

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
	//Enable special VLAN tagging mode
	lan937xWriteSwitchReg(interface, LAN937x_SWE_INGRSS_PORT_TYP,
	LAN937x_SWE_INGRSS_PORT_TYP_PORT0);

	//Configure egress VLAN tagging rules
	lan937xWriteSwitchReg(interface, LAN937x_BM_EGRSS_PORT_TYPE,
	LAN937x_BM_EGRSS_PORT_TYPE_PORT0_CPU);
#else
	//Disable special VLAN tagging mode
	lan937xWriteSwitchReg(interface, LAN937x_SWE_INGRSS_PORT_TYP, 0);

	//Revert to default configuration
	lan937xWriteSwitchReg(interface, LAN937x_BM_EGRSS_PORT_TYPE, 0);
#endif

	//port =1; //mod.kks step 1. vphy setting
	//Read PHY special control/status register
	status = lan937xReadPhyReg(interface, 1, LAN937x_PSCSR);
	t1_tx_enable = status;

	t1_tx_enable |= (1 << 13);  // Enable

	lan937xWritePhyReg(interface, 1, LAN937x_PSCSR, t1_tx_enable);

	t1_tx_enable = 0;

	t1_tx_enable = lan937xReadPhyReg(interface, 2, 9); // Master Mode (PORT2 : T1)

	/*Master Mode*/
	t1_tx_enable |= (1 << 11);	// 1:Master,  0:Slave

	lan937xWritePhyReg(interface, 2, 9, t1_tx_enable);

    osDelay(50);	//for stabilize the power sequence

#ifdef LINKUP_TEST_KKS // for debug mod.kks
	t1_tx_enable = lan937xReadPhyReg(interface, 2, 9);
	uint_t j;
	for(j=0; j<10; j++)
	{
		t1_tx_enable = lan937xReadPhyReg(interface, j, 1);
		printf("xkkstest index : %d, %x \n\r", j, t1_tx_enable);
	}
#endif

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
	//Port separation mode?
	if(interface->port != 0)
	{
		//Enable port mirroring
		lan937xWriteSwitchReg(interface, LAN937x_SWE_PORT_MIRROR,
		LAN937x_SWE_PORT_MIRROR_RX_MIRRORING_FILT_EN |
		LAN937x_SWE_PORT_MIRROR_SNIFFER_PORT0 |
		LAN937x_SWE_PORT_MIRROR_MIRRORED_PORT2 |
		LAN937x_SWE_PORT_MIRROR_MIRRORED_PORT1 |
		LAN937x_SWE_PORT_MIRROR_RX_MIRRORING_EN);

		//Configure port state
		lan937xWriteSwitchReg(interface, LAN937x_SWE_PORT_STATE,
		LAN937x_SWE_PORT_STATE_PORT2_LISTENING |
		LAN937x_SWE_PORT_STATE_PORT1_LISTENING |
		LAN937x_SWE_PORT_STATE_PORT0_FORWARDING);
	}
	else
#endif
	{
		//Disable port mirroring
		lan937xWriteSwitchReg(interface, LAN937x_SWE_PORT_MIRROR, 0);

		//Configure port state
		lan937xWriteSwitchReg(interface, LAN937x_SWE_PORT_STATE,
		LAN937x_SWE_PORT_STATE_PORT2_FORWARDING |
		LAN937x_SWE_PORT_STATE_PORT1_FORWARDING |
		LAN937x_SWE_PORT_STATE_PORT0_FORWARDING);
	}

	//Configure port 0 receive parameters
	lan937xWriteSwitchReg(interface, LAN937x_MAC_RX_CFG(0),
	LAN937x_MAC_RX_CFG_REJECT_MAC_TYPES | LAN937x_MAC_RX_CFG_RX_EN);

	//Configure port 0 transmit parameters
	lan937xWriteSwitchReg(interface, LAN937x_MAC_TX_CFG(0),
	LAN937x_MAC_TX_CFG_IFG_CONFIG_DEFAULT | LAN937x_MAC_TX_CFG_TX_PAD_EN |
	LAN937x_MAC_TX_CFG_TX_EN);

#if 0	// Test Register Dump
	uint_t		port;

	//Loop through the ports
	for(port = LAN937x_PORT1; port <= LAN937x_PORT2; port++)
	{
		//Debug message
		TRACE_DEBUG("Port %u:\r\n", port);
		//Dump PHY registers for debugging purpose
		lan937xDumpPhyReg(interface, port);
	}
#endif

	//Force the TCP/IP stack to poll the link state at startup
	interface->phyEvent = TRUE;
	//Notify the TCP/IP stack of the event
	osSetEvent(&netEvent);

	//Successful initialization
	return NO_ERROR;
}

#if( VCI_III_ASING_MODE )
error_t lan937xReInit(NetInterface *interface)
{
	uint32_t	value;
	uint16_t	t1_tx_enable;
	uint16_t	status;

	//Initialize serial management interface
	if(interface->smiDriver != NULL)
	{
		interface->smiDriver->init();
	}

	//The completion of the entire chip-level reset must then be determined
	//by polling the READY bit of the HW_CFG register
	do
	{
		//Read HW_CFG register
		value = lan937xReadSysReg(interface, LAN937x_HW_CFG);
	} while((value & LAN937x_HW_CFG_DEVICE_READY) == 0);

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
	//Enable special VLAN tagging mode
	lan937xWriteSwitchReg(interface, LAN937x_SWE_INGRSS_PORT_TYP,
	LAN937x_SWE_INGRSS_PORT_TYP_PORT0);

	//Configure egress VLAN tagging rules
	lan937xWriteSwitchReg(interface, LAN937x_BM_EGRSS_PORT_TYPE,
	LAN937x_BM_EGRSS_PORT_TYPE_PORT0_CPU);
#else
	//Disable special VLAN tagging mode
	lan937xWriteSwitchReg(interface, LAN937x_SWE_INGRSS_PORT_TYP, 0);

	//Revert to default configuration
	lan937xWriteSwitchReg(interface, LAN937x_BM_EGRSS_PORT_TYPE, 0);
#endif

	//port =1; //mod.kks step 1. vphy setting
	//Read PHY special control/status register
	status = lan937xReadPhyReg(interface, 1, LAN937x_PSCSR);
	t1_tx_enable = status;

	t1_tx_enable |= (1 << 13);  // Enable

	lan937xWritePhyReg(interface, 1, LAN937x_PSCSR, t1_tx_enable);

	t1_tx_enable = 0;

	t1_tx_enable = lan937xReadPhyReg(interface, 2, 9); // Master Mode (PORT2 : T1)

	/*Master Mode*/
	t1_tx_enable |= (1 << 11);	// 1:Master,  0:Slave

	lan937xWritePhyReg(interface, 2, 9, t1_tx_enable);

#ifdef LINKUP_TEST_KKS // for debug mod.kks
	t1_tx_enable = lan937xReadPhyReg(interface, 2, 9);
	uint_t j;
	for(j=0; j<10; j++)
	{
		t1_tx_enable = lan937xReadPhyReg(interface, j, 1);
		printf("xkkstest index : %d, %x \n\r", j, t1_tx_enable);
	}
#endif

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
	//Port separation mode?
	if(interface->port != 0)
	{
		//Enable port mirroring
		lan937xWriteSwitchReg(interface, LAN937x_SWE_PORT_MIRROR,
		LAN937x_SWE_PORT_MIRROR_RX_MIRRORING_FILT_EN |
		LAN937x_SWE_PORT_MIRROR_SNIFFER_PORT0 |
		LAN937x_SWE_PORT_MIRROR_MIRRORED_PORT2 |
		LAN937x_SWE_PORT_MIRROR_MIRRORED_PORT1 |
		LAN937x_SWE_PORT_MIRROR_RX_MIRRORING_EN);

		//Configure port state
		lan937xWriteSwitchReg(interface, LAN937x_SWE_PORT_STATE,
		LAN937x_SWE_PORT_STATE_PORT2_LISTENING |
		LAN937x_SWE_PORT_STATE_PORT1_LISTENING |
		LAN937x_SWE_PORT_STATE_PORT0_FORWARDING);
	}
	else
#endif
	{
		//Disable port mirroring
		lan937xWriteSwitchReg(interface, LAN937x_SWE_PORT_MIRROR, 0);

		//Configure port state
		lan937xWriteSwitchReg(interface, LAN937x_SWE_PORT_STATE,
		LAN937x_SWE_PORT_STATE_PORT2_FORWARDING |
		LAN937x_SWE_PORT_STATE_PORT1_FORWARDING |
		LAN937x_SWE_PORT_STATE_PORT0_FORWARDING);
	}

	//Configure port 0 receive parameters
	lan937xWriteSwitchReg(interface, LAN937x_MAC_RX_CFG(0),
	LAN937x_MAC_RX_CFG_REJECT_MAC_TYPES | LAN937x_MAC_RX_CFG_RX_EN);

	//Configure port 0 transmit parameters
	lan937xWriteSwitchReg(interface, LAN937x_MAC_TX_CFG(0),
	LAN937x_MAC_TX_CFG_IFG_CONFIG_DEFAULT | LAN937x_MAC_TX_CFG_TX_PAD_EN |
	LAN937x_MAC_TX_CFG_TX_EN);

#if 0	// Test Register Dump
	uint_t		port;

	//Loop through the ports
	for(port = LAN937x_PORT1; port <= LAN937x_PORT2; port++)
	{
		//Debug message
		TRACE_DEBUG("Port %u:\r\n", port);
		//Dump PHY registers for debugging purpose
		lan937xDumpPhyReg(interface, port);
	}
#endif

	//Successful initialization
	return NO_ERROR;
}
#endif //#if( VCI_III_ASING_MODE )
/**
 * @brief LAN937x timer handler
 * @param[in] interface Underlying network interface
 **/
void lan937xTick(NetInterface *interface)
{
   uint_t port;
   bool_t linkState;

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Port separation mode?
   if(interface->port != 0)
   {
      uint_t i;
      NetInterface *virtualInterface;

      //Loop through network interfaces
      for(i = 0; i < NET_INTERFACE_COUNT; i++)
      {
         //Point to the current interface
         virtualInterface = &netInterface[i];

         //Check whether the current virtual interface is attached to the
         //physical interface
         if(virtualInterface == interface ||
            virtualInterface->parent == interface)
         {
            //Retrieve current link state
            linkState = lan937xGetLinkState(interface, virtualInterface->port);

            //Link up or link down event?
            if(linkState != virtualInterface->linkState)
            {
               //Set event flag
               interface->phyEvent = TRUE;
               //Notify the TCP/IP stack of the event
               osSetEvent(&netEvent);
            }
         }
      }
   }
   else
#endif
   {
      //Initialize link state
      linkState = FALSE;

      //Loop through the ports
      for(port = LAN937x_PORT1; port <= LAN937x_PORT2; port++)
      {
         //Retrieve current link state
         if(lan937xGetLinkState(interface, port))
         {
            linkState = TRUE;
         }
      }

      //Link up or link down event?
      if(linkState != interface->linkState)
      {
         //Set event flag
         interface->phyEvent = TRUE;
         //Notify the TCP/IP stack of the event
         osSetEvent(&netEvent);
      }
   }
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void lan937xEnableIrq(NetInterface *interface)
{
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void lan937xDisableIrq(NetInterface *interface)
{
}


/**
 * @brief LAN937x event handler
 * @param[in] interface Underlying network interface
 **/

void lan937xEventHandler(NetInterface *interface)
{
   uint_t port;
   bool_t linkState;
   uint16_t t1_tx_enable; //mod.kks
   uint16_t status; //mod.kks

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Port separation mode?
   if(interface->port != 0)
   {
      uint_t i;
      NetInterface *virtualInterface;

      //Loop through network interfaces
      for(i = 0; i < NET_INTERFACE_COUNT; i++)
      {
         //Point to the current interface
         virtualInterface = &netInterface[i];

         //Check whether the current virtual interface is attached to the
         //physical interface
         if(virtualInterface == interface ||
            virtualInterface->parent == interface)
         {
            //Get the port number associated with the current interface
            port = virtualInterface->port;

            //Valid port?
            if(port >= LAN937x_PORT1 && port <= LAN937x_PORT2)
            {
               //Retrieve current link state
               linkState = lan937xGetLinkState(interface, port);

#ifdef LINKUP_TEST_KKS // for debug mod.kks
              uint_t j;
              for(j=0; j<10; j++)
              {
                t1_tx_enable = lan937xReadPhyReg(interface, j, 1);
                printf("kkstest index : %d, %x \n\r", j, t1_tx_enable);
              }
#endif
               //Link up event?
              if(linkState && !virtualInterface->linkState)
               {
                  //Adjust MAC configuration parameters for proper operation
                  interface->linkSpeed = NIC_LINK_SPEED_100MBPS;
                  interface->duplexMode = NIC_FULL_DUPLEX_MODE;
                  interface->nicDriver->updateMacConfig(interface);

                  //Read PHY special control/status register
                  status = lan937xReadPhyReg(interface, port, LAN937x_PSCSR);
                  /*mod.kks to enable the Tx, T1 */
                  t1_tx_enable = status;

                  t1_tx_enable |= (1 << 13);  // Enable

                  lan937xWritePhyReg(interface, port, LAN937x_PSCSR, t1_tx_enable);

                  t1_tx_enable = 0;

                  t1_tx_enable = lan937xReadPhyReg(interface, 1, 9); //Master Mode

                  /*Master Mode*/
                  t1_tx_enable |= (1 << 11);

                  lan937xWritePhyReg(interface, 1, 9, t1_tx_enable);

                  //Check current operation mode
                  switch(status & LAN937x_PSCSR_SPEED)
                  {
                  //10BASE-T half-duplex
                  case LAN937x_PSCSR_SPEED_10BT_HD:
                     virtualInterface->linkSpeed = NIC_LINK_SPEED_10MBPS;
                     virtualInterface->duplexMode = NIC_HALF_DUPLEX_MODE;
                     break;

                  //10BASE-T full-duplex
                  case LAN937x_PSCSR_SPEED_10BT_FD:
                     virtualInterface->linkSpeed = NIC_LINK_SPEED_10MBPS;
                     virtualInterface->duplexMode = NIC_FULL_DUPLEX_MODE;
                     break;

                  //100BASE-TX half-duplex
                  case LAN937x_PSCSR_SPEED_100BTX_HD:
                     virtualInterface->linkSpeed = NIC_LINK_SPEED_100MBPS;
                     virtualInterface->duplexMode = NIC_HALF_DUPLEX_MODE;
                     break;

                  //100BASE-TX full-duplex
                  case LAN937x_PSCSR_SPEED_100BTX_FD:
                     virtualInterface->linkSpeed = NIC_LINK_SPEED_100MBPS;
                     virtualInterface->duplexMode = NIC_FULL_DUPLEX_MODE;
                     break;

                  //Unknown operation mode
                  default:
                     //Debug message
                     TRACE_WARNING("Invalid operation mode!\r\n");
                     break;
                  }

                  //Update link state
                  virtualInterface->linkState = TRUE;

                  //Process link state change event
                  nicNotifyLinkChange(virtualInterface);
               }
               //Link down event
               else if(!linkState && virtualInterface->linkState)
               {
                  //Update link state
                  virtualInterface->linkState = FALSE;

                  //Process link state change event
                  nicNotifyLinkChange(virtualInterface);
               }
            }
         }
      }
   }
   else
#endif
   {
      //Initialize link state
      linkState = FALSE;

      //Loop through the ports
      for(port = LAN937x_PORT1; port <= LAN937x_PORT2; port++)
      {
         //Retrieve current link state
         if(lan937xGetLinkState(interface, port))
         {
            linkState = TRUE;
         }

      }

      //Link up event?
      if(linkState)
      {
#if 1 //mod.kks todo check master mode
		status = lan937xReadPhyReg(interface, 1, LAN937x_PSCSR);
		t1_tx_enable = status;

		t1_tx_enable |= (1 << 13);  // Enable

		lan937xWritePhyReg(interface, 1, LAN937x_PSCSR, t1_tx_enable);

		t1_tx_enable = 0;

		t1_tx_enable = lan937xReadPhyReg(interface, 2, 9); // Master Mode (PORT2 : T1)

		/*Master Mode*/
		t1_tx_enable |= (1 << 11);	// 1:Master,  0:Slave

		lan937xWritePhyReg(interface, 2, 9, t1_tx_enable);

		t1_tx_enable = 0;

		t1_tx_enable = lan937xReadPhyReg(interface, 8, 9); // Master Mode (PORT2 : T1)

		/*Master Mode*/
		t1_tx_enable |= (1 << 11);	// 1:Master,  0:Slave

		lan937xWritePhyReg(interface, 8, 9, t1_tx_enable);

#endif

         //Adjust MAC configuration parameters for proper operation
         interface->linkSpeed = NIC_LINK_SPEED_100MBPS;
         interface->duplexMode = NIC_FULL_DUPLEX_MODE;
         interface->nicDriver->updateMacConfig(interface);

         //Update link state
         interface->linkState = TRUE;


#ifdef LINKUP_TEST_KKS // for debug mod.kks
          uint_t j;
          for(j=0; j<10; j++)
          {
            t1_tx_enable = lan937xReadPhyReg(interface, j, 1);
            printf("kkstest index : %d, %x \n\r", j, t1_tx_enable);
          }
#endif
      }
      else
      {
         //Update link state
         interface->linkState = FALSE;
      }

      //Process link state change event
      nicNotifyLinkChange(interface);
   }
}


/**
 * @brief Add special VLAN tag to Ethernet frame
 * @param[in] interface Underlying network interface
 * @param[in] buffer Multi-part buffer containing the payload
 * @param[in,out] offset Offset to the first payload byte
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t lan937xTagFrame(NetInterface *interface, NetBuffer *buffer,
   size_t *offset, NetTxAncillary *ancillary)
{
   error_t error;

   //Initialize status code
   error = NO_ERROR;

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Valid port?
   if(ancillary->port <= LAN937x_PORT2)
   {
      //Is there enough space for the VLAN tag?
      if(*offset >= sizeof(VlanTag))
      {
         EthHeader *header;
         VlanTag *vlanTag;

         //Make room for the special VLAN tag
         *offset -= sizeof(VlanTag);
         //Point to the beginning of the frame
         header = netBufferAt(buffer, *offset);

         //Move the Ethernet header to make room for the special VLAN tag
         osMemmove(header, (uint8_t *) header + sizeof(VlanTag),
            sizeof(EthHeader));

         //The special VLAN tag is a normal VLAN tag where the VID field is
         //used as the destination port indicator
         vlanTag = (VlanTag *) header->data;

         //Default port number?
         if(ancillary->port == 0)
         {
            //If VID bit 3 is one, then the normal ALR lookup is performed
            vlanTag->tci = htons(LAN937x_VID_ALR_LOOKUP);
         }
         else
         {
            //VID bits 0 and 1 specify the destination port
            vlanTag->tci = htons(LAN937x_VID_STP_OVERRIDE | ancillary->port);
         }

         //The EtherType field indicates which protocol is encapsulated in
         //the payload
         vlanTag->type = header->type;

         //A distinct Ethertype has been allocated for use in the TPID field
         header->type = HTONS(ETH_TYPE_VLAN);
      }
      else
      {
         //Report an error
         error = ERROR_INVALID_PARAMETER;
      }
   }
   else
   {
      //The port number is not valid
      error = ERROR_INVALID_PORT;
   }
#endif

   //Return status code
   return error;
}


/**
 * @brief Decode special VLAN tag from incoming Ethernet frame
 * @param[in] interface Underlying network interface
 * @param[in,out] frame Pointer to the received Ethernet frame
 * @param[in,out] length Length of the frame, in bytes
 * @param[in,out] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t lan937xUntagFrame(NetInterface *interface, uint8_t **frame,
   size_t *length, NetRxAncillary *ancillary)
{
   error_t error;
#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   EthHeader *header;
   VlanTag *vlanTag;
#endif

   //Initialize status code
   error = NO_ERROR;

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Point to the beginning of the frame
   header = (EthHeader *) *frame;

   //Check whether VLAN tagging is used
   if(ntohs(header->type) == ETH_TYPE_VLAN)
   {
      //Valid Ethernet frame received?
      if(*length >= (sizeof(EthHeader) + sizeof(VlanTag)))
      {
         //The special VLAN tag is a normal VLAN tag where the VID field is
         //used as a source port indicator
         vlanTag = (VlanTag *) header->data;

         //VID bits 0 and 1 specify the source port
         ancillary->port = ntohs(vlanTag->tci) & LAN937x_VID_SRC_PORT;
         //Fix the value of the EtherType field
         header->type = vlanTag->type;

         //Strip the VLAN tag from the Ethernet frame
         osMemmove(*frame + sizeof(VlanTag), *frame, sizeof(EthHeader));

         //Point to the Ethernet frame header
         *frame += sizeof(VlanTag);
         //Retrieve the length of the original frame
         *length -= sizeof(VlanTag);

         //Successful processing
         error = NO_ERROR;
      }
      else
      {
         //Drop the received frame
         error = ERROR_INVALID_LENGTH;
      }
   }
   else
   {
      //If the interface is configured to accept VLAN-tagged frames, then
      //drop the incoming Ethernet frame
      error = ERROR_WRONG_IDENTIFIER;
   }
#endif

   //Return status code
   return error;
}


/**
 * @brief Get link state
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @return Link state
 **/

bool_t lan937xGetLinkState(NetInterface *interface, uint8_t port)
{
   uint16_t status;
   bool_t linkState;

   //Check port number
   if(port >= LAN937x_PORT1 && port <= LAN937x_PORT2)
   {
      //Read status register
      status = lan937xReadPhyReg(interface, port, LAN937x_BMSR);

      //Retrieve current link state
      linkState = (status & LAN937x_BMSR_LINK_STATUS) ? TRUE : FALSE;
   }
   else
   {
      //The specified port number is not valid
      linkState = FALSE;
   }

   //Return link status
   return linkState;
}


/**
 * @brief Get link speed
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @return Link speed
 **/

uint32_t lan937xGetLinkSpeed(NetInterface *interface, uint8_t port)
{
   uint16_t status;
   uint32_t linkSpeed;

   //Check port number
   if(port >= LAN937x_PORT1 && port <= LAN937x_PORT2)
   {
      //Read PHY special control/status register
      status = lan937xReadPhyReg(interface, port, LAN937x_PSCSR);

      //Check current operation mode
      switch(status & LAN937x_PSCSR_SPEED)
      {
      //10BASE-T
      case LAN937x_PSCSR_SPEED_10BT_HD:
      case LAN937x_PSCSR_SPEED_10BT_FD:
         linkSpeed = NIC_LINK_SPEED_10MBPS;
         break;

      //100BASE-TX
      case LAN937x_PSCSR_SPEED_100BTX_HD:
      case LAN937x_PSCSR_SPEED_100BTX_FD:
         linkSpeed = NIC_LINK_SPEED_100MBPS;
         break;

      //Unknown operation mode
      default:
         linkSpeed = NIC_LINK_SPEED_UNKNOWN;
         break;
      }
   }
   else
   {
      //The specified port number is not valid
      linkSpeed = NIC_LINK_SPEED_UNKNOWN;
   }

   //Return link status
   return linkSpeed;
}


/**
 * @brief Get duplex mode
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @return Duplex mode
 **/

NicDuplexMode lan937xGetDuplexMode(NetInterface *interface, uint8_t port)
{
   uint16_t status;
   NicDuplexMode duplexMode;

   //Check port number
   if(port >= LAN937x_PORT1 && port <= LAN937x_PORT2)
   {
      //Read PHY special control/status register
      status = lan937xReadPhyReg(interface, port, LAN937x_PSCSR);

      //Check current operation mode
      switch(status & LAN937x_PSCSR_SPEED)
      {
      //10BASE-T or 100BASE-TX half-duplex
      case LAN937x_PSCSR_SPEED_10BT_HD:
      case LAN937x_PSCSR_SPEED_100BTX_HD:
         duplexMode = NIC_HALF_DUPLEX_MODE;
         break;

      //10BASE-T or 100BASE-TX full-duplex
      case LAN937x_PSCSR_SPEED_100BTX_FD:
      case LAN937x_PSCSR_SPEED_10BT_FD:
         duplexMode = NIC_FULL_DUPLEX_MODE;
         break;

      //Unknown operation mode
      default:
         duplexMode = NIC_UNKNOWN_DUPLEX_MODE;
         break;
      }
   }
   else
   {
      //The specified port number is not valid
      duplexMode = NIC_UNKNOWN_DUPLEX_MODE;
   }

   //Return duplex mode
   return duplexMode;
}


/**
 * @brief Set port state
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @param[in] state Port state
 **/

void lan937xSetPortState(NetInterface *interface, uint8_t port,
   SwitchPortState state)
{
   uint32_t temp;

   //Read port state register
   temp = lan937xReadSwitchReg(interface, LAN937x_SWE_PORT_STATE);

   //Check port number
   if(port == LAN937x_PORT1)
   {
      //Clear current configuration
      temp &= ~LAN937x_SWE_PORT_STATE_PORT1;

      //Update port 1 state
      switch(state)
      {
      case SWITCH_PORT_STATE_BLOCKING:
         temp |= LAN937x_SWE_PORT_STATE_PORT1_LISTENING;
         break;
      case SWITCH_PORT_STATE_LEARNING:
         temp |= LAN937x_SWE_PORT_STATE_PORT1_LEARNING;
         break;
      case SWITCH_PORT_STATE_FORWARDING:
         temp |= LAN937x_SWE_PORT_STATE_PORT1_FORWARDING;
         break;
      default:
         temp |= LAN937x_SWE_PORT_STATE_PORT1_DISABLED;
         break;
      }
   }
   else if(port == LAN937x_PORT2)
   {
      //Clear current configuration
      temp &= ~LAN937x_SWE_PORT_STATE_PORT2;

      //Update port 2 state
      switch(state)
      {
      case SWITCH_PORT_STATE_BLOCKING:
         temp |= LAN937x_SWE_PORT_STATE_PORT2_LISTENING;
         break;
      case SWITCH_PORT_STATE_LEARNING:
         temp |= LAN937x_SWE_PORT_STATE_PORT2_LEARNING;
         break;
      case SWITCH_PORT_STATE_FORWARDING:
         temp |= LAN937x_SWE_PORT_STATE_PORT2_FORWARDING;
         break;
      default:
         temp |= LAN937x_SWE_PORT_STATE_PORT2_DISABLED;
         break;
      }
   }
   else
   {
      //The specified port number is not valid
   }

   //Write the value back to port state register
   lan937xWriteSwitchReg(interface, LAN937x_SWE_PORT_STATE, temp);
}


/**
 * @brief Get port state
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @return Port state
 **/

SwitchPortState lan937xGetPortState(NetInterface *interface, uint8_t port)
{
   uint16_t temp;
   SwitchPortState state;

   //Read port state register
   temp = lan937xReadSwitchReg(interface, LAN937x_SWE_PORT_STATE);

   //Check port number
   if(port == LAN937x_PORT1)
   {
      //Check port 1 state
      switch(temp & LAN937x_SWE_PORT_STATE_PORT1)
      {
      case LAN937x_SWE_PORT_STATE_PORT1_LISTENING:
         state = SWITCH_PORT_STATE_BLOCKING;
         break;
      case LAN937x_SWE_PORT_STATE_PORT1_LEARNING:
         state = SWITCH_PORT_STATE_LEARNING;
         break;
      case LAN937x_SWE_PORT_STATE_PORT1_FORWARDING:
         state = SWITCH_PORT_STATE_FORWARDING;
         break;
      default:
         state = SWITCH_PORT_STATE_DISABLED;
         break;
      }
   }
   else if(port == LAN937x_PORT2)
   {
      //Check port 2 state
      switch(temp & LAN937x_SWE_PORT_STATE_PORT2)
      {
      case LAN937x_SWE_PORT_STATE_PORT2_LISTENING:
         state = SWITCH_PORT_STATE_BLOCKING;
         break;
      case LAN937x_SWE_PORT_STATE_PORT2_LEARNING:
         state = SWITCH_PORT_STATE_LEARNING;
         break;
      case LAN937x_SWE_PORT_STATE_PORT2_FORWARDING:
         state = SWITCH_PORT_STATE_FORWARDING;
         break;
      default:
         state = SWITCH_PORT_STATE_DISABLED;
         break;
      }
   }
   else
   {
      //The specified port number is not valid
      state = SWITCH_PORT_STATE_DISABLED;
   }

   //Return port state
   return state;
}


/**
 * @brief Set aging time for dynamic filtering entries
 * @param[in] interface Underlying network interface
 * @param[in] agingTime Aging time, in seconds
 **/

void lan937xSetAgingTime(NetInterface *interface, uint32_t agingTime)
{
   //The aging period is fixed to 5 minutes
}


/**
 * @brief Enable reserved multicast table
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable reserved group addresses
 **/

void lan937xEnableRsvdMcastTable(NetInterface *interface, bool_t enable)
{
   uint_t i;
   SwitchFdbEntry entry;

   //The reserved group addresses are in the range of 01-80-C2-00-00-00 to
   //01-80-C2-00-00-0F
   for(i = 0; i <= 15; i++)
   {
      //Specify the reserved group address to be added or removed
      entry.macAddr.b[0] = 0x01;
      entry.macAddr.b[1] = 0x80;
      entry.macAddr.b[2] = 0xC2;
      entry.macAddr.b[3] = 0x00;
      entry.macAddr.b[4] = 0x00;
      entry.macAddr.b[5] = i;

      //Format forwarding database entry
      entry.srcPort = 0;
      entry.destPorts = SWITCH_CPU_PORT_MASK;
      entry.override = TRUE;

      //Update the static MAC table
      if(enable)
      {
         lan937xAddStaticFdbEntry(interface, &entry);
      }
      else
      {
         lan937xDeleteStaticFdbEntry(interface, &entry);
      }
   }
}


/**
 * @brief Add a new entry to the static MAC table
 * @param[in] interface Underlying network interface
 * @param[in] entry Pointer to the forwarding database entry
 * @return Error code
 **/

error_t lan937xAddStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry)
{
   uint32_t value;
   uint32_t ports;

   //Check whether the forward port is the CPU port
   if(entry->destPorts == SWITCH_CPU_PORT_MASK)
   {
      ports = LAN937x_PORT0_MASK;
   }
   else
   {
      ports = entry->destPorts & LAN937x_PORT_MASK;
   }

   //Valid forward ports?
   if(ports != 0)
   {
      //Write SWE_ALR_WR_DAT_0 and SWE_ALR_WR_DAT_1 with the desired MAC
      //address and control bits
      value = LAN937x_SWE_ALR_WR_DAT1_VALID | LAN937x_SWE_ALR_WR_DAT1_STATIC;

      //When the Override bit set, packets received with a destination
      //address that matches the MAC address in the SWE_ALR_WR_DAT_1 and
      //SWE_ALR_WR_DAT_0 registers will be forwarded regardless of the port
      //state
      if(entry->override)
      {
         value |= LAN937x_SWE_ALR_WR_DAT1_AGE_OVERRIDE;
      }

      //Set the ports associated with this MAC address
      switch(ports)
      {
      case LAN937x_PORT0_MASK:
         value |= LAN937x_SWE_ALR_WR_DAT1_PORT_0;
         break;
      case LAN937x_PORT1_MASK:
         value |= LAN937x_SWE_ALR_WR_DAT1_PORT_1;
         break;
      case LAN937x_PORT2_MASK:
         value |= LAN937x_SWE_ALR_WR_DAT1_PORT_2;
         break;
      case LAN937x_PORT0_1_MASK:
         value |= LAN937x_SWE_ALR_WR_DAT1_PORT_0_1;
         break;
      case LAN937x_PORT0_2_MASK:
         value |= LAN937x_SWE_ALR_WR_DAT1_PORT_0_2;
         break;
      case LAN937x_PORT1_2_MASK:
         value |= LAN937x_SWE_ALR_WR_DAT1_PORT_1_2;
         break;
      default:
         value |= LAN937x_SWE_ALR_WR_DAT1_PORT_0_1_2;
         break;
      }
   }
   else
   {
      //An entry can be deleted by setting the Valid bit to 0
      value = LAN937x_SWE_ALR_WR_DAT1_STATIC;
   }

   //Copy MAC address (last 16 bits)
   value |= entry->macAddr.b[4] | (entry->macAddr.b[5] << 8);
   //Write SWE_ALR_WR_DAT_1 register
   lan937xWriteSwitchReg(interface, LAN937x_SWE_ALR_WR_DAT1, value);

   //Copy MAC address (first 32 bits)
   value = entry->macAddr.b[0] | (entry->macAddr.b[1] << 8) |
      (entry->macAddr.b[2] << 16) | (entry->macAddr.b[3] << 24);

   //Write SWE_ALR_WR_DAT_0 register
   lan937xWriteSwitchReg(interface, LAN937x_SWE_ALR_WR_DAT0, value);

   //Write the SWE_ALR_CMD register
   lan937xWriteSwitchReg(interface, LAN937x_SWE_ALR_CMD,
      LAN937x_SWE_ALR_CMD_MAKE_ENTRY);

   //Poll the Make Pending bit in the SWE_ALR_CMD_STS register until it is
   //cleared
   do
   {
      //Read the SWE_ALR_CMD_STS register
      value = lan937xReadSwitchReg(interface, LAN937x_SWE_ALR_CMD_STS);

      //Check the Make Pending bit
   } while((value & LAN937x_SWE_ALR_CMD_STS_MAKE_PENDING) != 0);

   //Clear the SWE_ALR_CMD register
   lan937xWriteSwitchReg(interface, LAN937x_SWE_ALR_CMD, 0);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Remove an entry from the static MAC table
 * @param[in] interface Underlying network interface
 * @param[in] entry Forwarding database entry to remove from the table
 * @return Error code
 **/

error_t lan937xDeleteStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry)
{
   uint32_t value;

   //An entry can be deleted by setting the Valid bit to 0
   value = LAN937x_SWE_ALR_WR_DAT1_STATIC;

   //Specify the MAC address to remove (last 16 bits)
   value |= entry->macAddr.b[4] | (entry->macAddr.b[5] << 8);
   //Write SWE_ALR_WR_DAT_1 register
   lan937xWriteSwitchReg(interface, LAN937x_SWE_ALR_WR_DAT1, value);

   //Specify the MAC address to remove (first 32 bits)
   value = entry->macAddr.b[0] | (entry->macAddr.b[1] << 8) |
      (entry->macAddr.b[2] << 16) | (entry->macAddr.b[3] << 24);

   //Write SWE_ALR_WR_DAT_0 register
   lan937xWriteSwitchReg(interface, LAN937x_SWE_ALR_WR_DAT0, value);

   //Write the SWE_ALR_CMD register
   lan937xWriteSwitchReg(interface, LAN937x_SWE_ALR_CMD,
      LAN937x_SWE_ALR_CMD_MAKE_ENTRY);

   //Poll the Make Pending bit in the SWE_ALR_CMD_STS register until it is
   //cleared
   do
   {
      //Read the SWE_ALR_CMD_STS register
      value = lan937xReadSwitchReg(interface, LAN937x_SWE_ALR_CMD_STS);

      //Check the Make Pending bit
   } while((value & LAN937x_SWE_ALR_CMD_STS_MAKE_PENDING) != 0);

   //Clear the SWE_ALR_CMD register
   lan937xWriteSwitchReg(interface, LAN937x_SWE_ALR_CMD, 0);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Read an entry from the static MAC table
 * @param[in] interface Underlying network interface
 * @param[in] index Zero-based index of the entry to read
 * @param[out] entry Pointer to the forwarding database entry
 * @return Error code
 **/

error_t lan937xGetStaticFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry)
{
   uint32_t value;

   //Loop through the ALR table
   while(index < LAN937x_ALR_TABLE_SIZE)
   {
      //First entry?
      if(index == 0)
      {
         value = LAN937x_SWE_ALR_CMD_GET_FIRST_ENTRY;
      }
      else
      {
         value = LAN937x_SWE_ALR_CMD_GET_NEXT_ENTRY;
      }

      //Write the SWE_ALR_CMD register
      lan937xWriteSwitchReg(interface, LAN937x_SWE_ALR_CMD, value);
      //Clear the SWE_ALR_CMD register
      lan937xWriteSwitchReg(interface, LAN937x_SWE_ALR_CMD, 0);

      //Poll the Valid and End of Table bits in the SWE_ALR_RD_DAT_1 register
      //until either is set
      while(1)
      {
         //Read SWE_ALR_RD_DAT_1 register
         value = lan937xReadSwitchReg(interface, LAN937x_SWE_ALR_RD_DAT1);

         //If the End of Table bit is set, then exit
         if((value & LAN937x_SWE_ALR_RD_DAT1_END_OF_TABLE) != 0)
         {
            return ERROR_END_OF_TABLE;
         }

         //If the Valid bit is set, then the entry is valid
         if((value & LAN937x_SWE_ALR_RD_DAT1_VALID) != 0)
         {
            break;
         }
      }

      //Static entry?
      if((value & LAN937x_SWE_ALR_RD_DAT1_STATIC) != 0)
      {
         break;
      }

      //Skip dynamic entries
      index++;
   }

   //Store the data from SWE_ALR_RD_DAT_0 and SWE_ALR_RD_DAT_1 registers
   entry->srcPort = 0;
   entry->override = FALSE;

   //Retrieve the ports associated with this MAC address
   switch(value & LAN937x_SWE_ALR_RD_DAT1_PORT)
   {
   case LAN937x_SWE_ALR_RD_DAT1_PORT_0:
      entry->destPorts = LAN937x_PORT0_MASK;
      break;
   case LAN937x_SWE_ALR_RD_DAT1_PORT_1:
      entry->destPorts = LAN937x_PORT1_MASK;
      break;
   case LAN937x_SWE_ALR_RD_DAT1_PORT_2:
      entry->destPorts = LAN937x_PORT2_MASK;
      break;
   case LAN937x_SWE_ALR_RD_DAT1_PORT_0_1:
      entry->destPorts = LAN937x_PORT0_1_MASK;
      break;
   case LAN937x_SWE_ALR_RD_DAT1_PORT_0_2:
      entry->destPorts = LAN937x_PORT0_2_MASK;
      break;
   case LAN937x_SWE_ALR_RD_DAT1_PORT_1_2:
      entry->destPorts = LAN937x_PORT1_2_MASK;
      break;
   case LAN937x_SWE_ALR_RD_DAT1_PORT_0_1_2:
      entry->destPorts = LAN937x_PORT0_1_2_MASK;
      break;
   default:
      entry->destPorts = 0;
      break;
   }

   //Copy MAC address (last 16 bits)
   entry->macAddr.b[4] = value & 0xFF;
   entry->macAddr.b[5] = (value >> 8) & 0xFF;

   //Read SWE_ALR_RD_DAT_0 register
   value = lan937xReadSwitchReg(interface, LAN937x_SWE_ALR_RD_DAT0);

   //Copy MAC address (first 32 bits)
   entry->macAddr.b[0] = value & 0xFF;
   entry->macAddr.b[1] = (value >> 8) & 0xFF;
   entry->macAddr.b[2] = (value >> 16) & 0xFF;
   entry->macAddr.b[3] = (value >> 24) & 0xFF;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Flush static MAC table
 * @param[in] interface Underlying network interface
 **/

void lan937xFlushStaticFdbTable(NetInterface *interface)
{
   error_t error;
   uint_t i;
   SwitchFdbEntry entry;

   //Loop through the ALR table
   for(i = 0; i < LAN937x_ALR_TABLE_SIZE; i++)
   {
      //Read current entry
      error = lan937xGetStaticFdbEntry(interface, i, &entry);

      //Valid entry?
      if(!error)
      {
         //An entry can be deleted by setting the Valid bit to 0
         lan937xDeleteStaticFdbEntry(interface, &entry);
      }
      else
      {
         //The end of the table has been reached
         break;
      }
   }
}

#if( VCI_III_ASING_MODE )
void lan9371Reset(void) //mod.kks to set the Ethernet.
{
#include "drivers/switch/lan937x_driver.h"
#include "drivers/mac/stm32h7xx_eth_driver.h"
#ifdef VCI3_DIAG
#include "git_eth.h"
#endif
    NetInterface	*interface;

    interface = &netInterface[0];

	lan937xReInit(interface);
}
#endif
/**
 * @brief Read an entry from the dynamic MAC table
 * @param[in] interface Underlying network interface
 * @param[in] index Zero-based index of the entry to read
 * @param[out] entry Pointer to the forwarding database entry
 * @return Error code
 **/

error_t lan937xGetDynamicFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry)
{
   uint32_t value;

   //Loop through the ALR table
   while(index < LAN937x_ALR_TABLE_SIZE)
   {
      //First entry?
      if(index == 0)
      {
         value = LAN937x_SWE_ALR_CMD_GET_FIRST_ENTRY;
      }
      else
      {
         value = LAN937x_SWE_ALR_CMD_GET_NEXT_ENTRY;
      }

      //Write the SWE_ALR_CMD register
      lan937xWriteSwitchReg(interface, LAN937x_SWE_ALR_CMD, value);
      //Clear the SWE_ALR_CMD register
      lan937xWriteSwitchReg(interface, LAN937x_SWE_ALR_CMD, 0);

      //Poll the Valid and End of Table bits in the SWE_ALR_RD_DAT_1 register
      //until either is set
      while(1)
      {
         //Read SWE_ALR_RD_DAT_1 register
         value = lan937xReadSwitchReg(interface, LAN937x_SWE_ALR_RD_DAT1);

         //If the End of Table bit is set, then exit
         if((value & LAN937x_SWE_ALR_RD_DAT1_END_OF_TABLE) != 0)
         {
            return ERROR_END_OF_TABLE;
         }

         //If the Valid bit is set, then the entry is valid
         if((value & LAN937x_SWE_ALR_RD_DAT1_VALID) != 0)
         {
            break;
         }
      }

      //Dynamic entry?
      if((value & LAN937x_SWE_ALR_RD_DAT1_STATIC) == 0)
      {
         break;
      }

      //Skip static entries
      index++;
   }

   //Store the data from SWE_ALR_RD_DAT_0 and SWE_ALR_RD_DAT_1 registers
   entry->destPorts = 0;
   entry->override = FALSE;

   //Retrieve the port associated with this MAC address
   switch(value & LAN937x_SWE_ALR_RD_DAT1_PORT)
   {
   case LAN937x_SWE_ALR_RD_DAT1_PORT_0:
      entry->srcPort = LAN937x_PORT0;
      break;
   case LAN937x_SWE_ALR_RD_DAT1_PORT_1:
      entry->srcPort = LAN937x_PORT1;
      break;
   case LAN937x_SWE_ALR_RD_DAT1_PORT_2:
      entry->srcPort = LAN937x_PORT2;
      break;
   default:
      entry->srcPort = 0;
      break;
   }

   //Copy MAC address (last 16 bits)
   entry->macAddr.b[4] = value & 0xFF;
   entry->macAddr.b[5] = (value >> 8) & 0xFF;

   //Read SWE_ALR_RD_DAT_0 register
   value = lan937xReadSwitchReg(interface, LAN937x_SWE_ALR_RD_DAT0);

   //Copy MAC address (first 32 bits)
   entry->macAddr.b[0] = value & 0xFF;
   entry->macAddr.b[1] = (value >> 8) & 0xFF;
   entry->macAddr.b[2] = (value >> 16) & 0xFF;
   entry->macAddr.b[3] = (value >> 24) & 0xFF;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Flush dynamic MAC table
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 **/

void lan937xFlushDynamicFdbTable(NetInterface *interface, uint8_t port)
{
   error_t error;
   uint_t i;
   uint32_t value;
   SwitchFdbEntry entry;

   //Loop through the ALR table
   for(i = 0; i < LAN937x_ALR_TABLE_SIZE; i++)
   {
      //Read current entry
      error = lan937xGetDynamicFdbEntry(interface, i, &entry);

      //Valid entry?
      if(!error)
      {
         //Matching port number?
         if(entry.srcPort == port || port == 0)
         {
            //Specify the MAC address to remove (last 16 bits)
            value = entry.macAddr.b[4] | (entry.macAddr.b[5] << 8);
            //Write SWE_ALR_WR_DAT_1 register
            lan937xWriteSwitchReg(interface, LAN937x_SWE_ALR_WR_DAT1, value);

            //Specify the MAC address to remove (first 32 bits)
            value = entry.macAddr.b[0] | (entry.macAddr.b[1] << 8) |
               (entry.macAddr.b[2] << 16) | (entry.macAddr.b[3] << 24);

            //Write SWE_ALR_WR_DAT_0 register
            lan937xWriteSwitchReg(interface, LAN937x_SWE_ALR_WR_DAT0, value);

            //Write the SWE_ALR_CMD register
            lan937xWriteSwitchReg(interface, LAN937x_SWE_ALR_CMD,
               LAN937x_SWE_ALR_CMD_MAKE_ENTRY);

            //Poll the Make Pending bit in the SWE_ALR_CMD_STS register until
            //it is cleared
            do
            {
               //Read the SWE_ALR_CMD_STS register
               value = lan937xReadSwitchReg(interface, LAN937x_SWE_ALR_CMD_STS);

               //Check the Make Pending bit
            } while((value & LAN937x_SWE_ALR_CMD_STS_MAKE_PENDING) != 0);

            //Clear the SWE_ALR_CMD register
            lan937xWriteSwitchReg(interface, LAN937x_SWE_ALR_CMD, 0);
         }
      }
      else
      {
         //The end of the table has been reached
         break;
      }
   }
}


/**
 * @brief Write PHY register
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @param[in] address PHY register address
 * @param[in] data Register value
 **/

void lan937xWritePhyReg(NetInterface *interface, uint8_t port,
   uint8_t address, uint16_t data)
{
   //Write the specified PHY register
   if(interface->smiDriver != NULL)
   {
      interface->smiDriver->writePhyReg(SMI_OPCODE_WRITE, port, address, data);
   }
   else
   {
      interface->nicDriver->writePhyReg(SMI_OPCODE_WRITE, port, address, data);
   }
}


/**
 * @brief Read PHY register
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @param[in] address PHY register address
 * @return Register value
 **/

uint16_t lan937xReadPhyReg(NetInterface *interface, uint8_t port,
   uint8_t address)
{
   uint16_t data;

   //Read the specified PHY register
   if(interface->smiDriver != NULL)
   {
      data = interface->smiDriver->readPhyReg(SMI_OPCODE_READ, port, address);
   }
   else
   {
      data = interface->nicDriver->readPhyReg(SMI_OPCODE_READ, port, address);
   }

   //Return the value of the PHY register
   return data;
}


/**
 * @brief Dump PHY registers for debugging purpose
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 **/

void lan937xDumpPhyReg(NetInterface *interface, uint8_t port)
{
   uint8_t i;

   //Loop through PHY registers
   for(i = 0; i < 32; i++)
   {
      //Display current PHY register
      TRACE_DEBUG("%02" PRIu8 ": 0x%04" PRIX16 "\r\n", i,
         lan937xReadPhyReg(interface, port, i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}


/**
 * @brief Write system CSR register
 * @param[in] interface Underlying network interface
 * @param[in] address System register address
 * @param[in] data Register value
 **/

void lan937xWriteSysReg(NetInterface *interface, uint16_t address,
   uint32_t data)
{
   uint8_t phyAddr;
   uint8_t regAddr;

   //PHY address bit 4 is 1 for SMI commands. PHY address 3:0 form system
   //register address bits 9:6
   phyAddr = 0x10 | ((address >> 6) & 0x0F);

   //Register address field forms register address bits 5:1
   regAddr = (address >> 1) & 0x1F;

   //Write system CSR register
   if(interface->smiDriver != NULL)
   {
      //Write the low word of the SMI register
      interface->smiDriver->writePhyReg(SMI_OPCODE_WRITE, phyAddr, regAddr,
         data & 0xFFFF);

      //Write the high word of the SMI register
      interface->smiDriver->writePhyReg(SMI_OPCODE_WRITE, phyAddr, regAddr + 1,
         (data >> 16) & 0xFFFF);
   }
   else
   {
      //Write the low word of the SMI register
      interface->nicDriver->writePhyReg(SMI_OPCODE_WRITE, phyAddr, regAddr,
         data & 0xFFFF);

      //Write the high word of the SMI register
      interface->nicDriver->writePhyReg(SMI_OPCODE_WRITE, phyAddr, regAddr + 1,
         (data >> 16) & 0xFFFF);
   }
}


/**
 * @brief Read system CSR register
 * @param[in] interface Underlying network interface
 * @param[in] address System register address
 * @return Register value
 **/

uint32_t lan937xReadSysReg(NetInterface *interface, uint16_t address)
{
   uint8_t phyAddr;
   uint8_t regAddr;
   uint32_t data;

   //PHY address bit 4 is 1 for SMI commands. PHY address 3:0 form system
   //register address bits 9:6
   phyAddr = 0x10 | ((address >> 6) & 0x0F);

   //Register address field forms register address bits 5:1
   regAddr = (address >> 1) & 0x1F;

   //Read system CSR register
   if(interface->smiDriver != NULL)
   {
      //Read the low word of the SMI register
      data = interface->smiDriver->readPhyReg(SMI_OPCODE_READ, phyAddr,
         regAddr);

      //Read the high word of the SMI register
      data |= interface->smiDriver->readPhyReg(SMI_OPCODE_READ, phyAddr,
         regAddr + 1) << 16;
   }
   else
   {
      //Read the low word of the SMI register
      data = interface->nicDriver->readPhyReg(SMI_OPCODE_READ, phyAddr,
         regAddr);


      //Read the high word of the SMI register
      data |= interface->nicDriver->readPhyReg(SMI_OPCODE_READ, phyAddr,
         regAddr + 1) << 16;
   }

   //Return register value
   return data;
}


/**
 * @brief Dump system CSR registers for debugging purpose
 * @param[in] interface Underlying network interface
 **/

void lan937xDumpSysReg(NetInterface *interface)
{
   uint16_t i;

   //Loop through system registers
   for(i = 80; i < 512; i += 4)
   {
      //Display current system register
      TRACE_DEBUG("0x%03" PRIX16 ": 0x%08" PRIX32 "\r\n", i,
         lan937xReadSysReg(interface, i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}


/**
 * @brief Write switch fabric CSR register
 * @param[in] interface Underlying network interface
 * @param[in] address Switch fabric register address
 * @param[in] data Register value
 **/

void lan937xWriteSwitchReg(NetInterface *interface, uint16_t address,
   uint32_t data)
{
   uint32_t value;

   //To perform a write to an individual switch fabric register, the desired
   //data must first be written into the SWITCH_CSR_DATA register
   lan937xWriteSysReg(interface, LAN937x_SWITCH_CSR_DATA, data);

   //Set up a write operation
   value = LAN937x_SWITCH_CSR_CMD_BUSY | LAN937x_SWITCH_CSR_CMD_BE;
   //Set register address
   value |= address & LAN937x_SWITCH_CSR_CMD_ADDR;

   //The write cycle is initiated by performing a single write to the
   //SWITCH_CSR_CMD register
   lan937xWriteSysReg(interface, LAN937x_SWITCH_CSR_CMD, value);

   //The completion of the write cycle is indicated by the clearing of the
   //CSR_BUSY bit
}


/**
 * @brief Read switch fabric CSR register
 * @param[in] interface Underlying network interface
 * @param[in] address Switch fabric register address
 * @return Register value
 **/

uint32_t lan937xReadSwitchReg(NetInterface *interface, uint16_t address)
{
   uint32_t value;

   //Set up a read operation
   value = LAN937x_SWITCH_CSR_CMD_BUSY | LAN937x_SWITCH_CSR_CMD_READ |
      LAN937x_SWITCH_CSR_CMD_BE;

   //Set register address
   value |= address & LAN937x_SWITCH_CSR_CMD_ADDR;

   //To perform a read of an individual switch fabric register, the read cycle
   //must be initiated by performing a single write to the SWITCH_CSR_CMD
   //register
   lan937xWriteSysReg(interface, LAN937x_SWITCH_CSR_CMD, value);

   //Valid data is available for reading when the CSR_BUSY bit is cleared
   do
   {
      //Read SWITCH_CSR_CMD register
      value = lan937xReadSysReg(interface, LAN937x_SWITCH_CSR_CMD);

      //Poll CSR_BUSY bit
   } while((value & LAN937x_SWITCH_CSR_CMD_BUSY) != 0);

   //Read data from the SWITCH_CSR_DATA register
   return lan937xReadSysReg(interface, LAN937x_SWITCH_CSR_DATA);
}
