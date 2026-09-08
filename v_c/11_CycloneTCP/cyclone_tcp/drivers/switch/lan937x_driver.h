/**
 * @file lan937x_driver.h
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

#ifndef _LAN937x_DRIVER_H
#define _LAN937x_DRIVER_H

//Dependencies
#include "core/nic.h"

//Port identifiers
#define LAN937x_PORT0 3
#define LAN937x_PORT1 1
#define LAN937x_PORT2 2

//Port masks
#define LAN937x_PORT_MASK      0x07
#define LAN937x_PORT0_MASK     0x04
#define LAN937x_PORT1_MASK     0x01
#define LAN937x_PORT2_MASK     0x02
#define LAN937x_PORT0_1_MASK   0x05
#define LAN937x_PORT0_2_MASK   0x06
#define LAN937x_PORT1_2_MASK   0x03
#define LAN937x_PORT0_1_2_MASK 0x07

//Size of of the MAC address lookup table
#define LAN937x_ALR_TABLE_SIZE 512

//Special VLAN tag (host to LAN937x)
#define LAN937x_VID_VLAN_RULES    0x0040
#define LAN937x_VID_CALC_PRIORITY 0x0020
#define LAN937x_VID_STP_OVERRIDE  0x0010
#define LAN937x_VID_ALR_LOOKUP    0x0008
#define LAN937x_VID_BROADCAST     0x0003
#define LAN937x_VID_DEST_PORT2    0x0002
#define LAN937x_VID_DEST_PORT1    0x0001
#define LAN937x_VID_DEST_PORT0    0x0000

//Special VLAN tag (LAN937x to host)
#define LAN937x_VID_PRIORITY      0x0380
#define LAN937x_VID_PRIORITY_EN   0x0040
#define LAN937x_VID_STATIC        0x0020
#define LAN937x_VID_STP_OVERRIDE  0x0010
#define LAN937x_VID_IGMP_PACKET   0x0008
#define LAN937x_VID_SRC_PORT      0x0003

//LAN937x PHY registers
#define LAN937x_BMCR                                 0x00
#define LAN937x_BMSR                                 0x01
#define LAN937x_PHYID1                               0x02
#define LAN937x_PHYID2                               0x03
#define LAN937x_ANAR                                 0x04
#define LAN937x_ANLPAR                               0x05
#define LAN937x_ANER                                 0x06
#define LAN937x_PMCSR                                0x11
#define LAN937x_PSMR                                 0x12
#define LAN937x_PSCSIR                               0x1B
#define LAN937x_PISR                                 0x1D
#define LAN937x_PIMR                                 0x1E
#define LAN937x_PSCSR                                0x1F

//LAN937x System registers
#define LAN937x_ID                                   0x0050
#define LAN937x_BYTE_TEST                            0x0064
#define LAN937x_HW_CFG                               0x0074
#define LAN937x_SWITCH_CSR_DATA                      0x01AC
#define LAN937x_SWITCH_CSR_CMD                       0x01B0

//LAN937x Switch Fabric registers
#define LAN937x_SW_DEV_ID                            0x0000
#define LAN937x_SW_RESET                             0x0001
#define LAN937x_SW_IMR                               0x0004
#define LAN937x_SW_IPR                               0x0005
#define LAN937x_MAC_VER_ID0                          0x0400
#define LAN937x_MAC_RX_CFG0                          0x0401
#define LAN937x_MAC_TX_CFG0                          0x0440
#define LAN937x_MAC_VER_ID1                          0x0800
#define LAN937x_MAC_RX_CFG1                          0x0801
#define LAN937x_MAC_TX_CFG1                          0x0840
#define LAN937x_MAC_VER_ID2                          0x0C00
#define LAN937x_MAC_RX_CFG2                          0x0C01
#define LAN937x_MAC_TX_CFG2                          0x0C40
#define LAN937x_SWE_ALR_CMD                          0x1800
#define LAN937x_SWE_ALR_WR_DAT0                      0x1801
#define LAN937x_SWE_ALR_WR_DAT1                      0x1802
#define LAN937x_SWE_ALR_RD_DAT0                      0x1805
#define LAN937x_SWE_ALR_RD_DAT1                      0x1806
#define LAN937x_SWE_ALR_CMD_STS                      0x1808
#define LAN937x_SWE_ALR_CFG                          0x1809
#define LAN937x_SWE_PORT_STATE                       0x1843
#define LAN937x_SWE_PORT_MIRROR                      0x1846
#define LAN937x_SWE_INGRSS_PORT_TYP                  0x1847
#define LAN937x_BM_EGRSS_PORT_TYPE                   0x1C0C

//LAN937x Switch Fabric register access macros
#define LAN937x_MAC_VER_ID(port)                     (0x0400 + ((port) * 0x0400))
#define LAN937x_MAC_RX_CFG(port)                     (0x0401 + ((port) * 0x0400))
#define LAN937x_MAC_TX_CFG(port)                     (0x0440 + ((port) * 0x0400))

//PHY Basic Control register
#define LAN937x_BMCR_RESET                           0x8000
#define LAN937x_BMCR_LOOPBACK                        0x4000
#define LAN937x_BMCR_SPEED_SEL                       0x2000
#define LAN937x_BMCR_AN_EN                           0x1000
#define LAN937x_BMCR_POWER_DOWN                      0x0800
#define LAN937x_BMCR_RESTART_AN                      0x0200
#define LAN937x_BMCR_DUPLEX_MODE                     0x0100
#define LAN937x_BMCR_COL_TEST                        0x0080

//PHY Basic Status register
#define LAN937x_BMSR_100BT4                          0x8000
#define LAN937x_BMSR_100BTX_FD                       0x4000
#define LAN937x_BMSR_100BTX_HD                       0x2000
#define LAN937x_BMSR_10BT_FD                         0x1000
#define LAN937x_BMSR_10BT_HD                         0x0800
#define LAN937x_BMSR_100BT2_FD                       0x0400
#define LAN937x_BMSR_100BT2_HD                       0x0200
#define LAN937x_BMSR_AN_COMPLETE                     0x0020
#define LAN937x_BMSR_REMOTE_FAULT                    0x0010
#define LAN937x_BMSR_AN_CAPABLE                      0x0008
#define LAN937x_BMSR_LINK_STATUS                     0x0004
#define LAN937x_BMSR_JABBER_DETECT                   0x0002
#define LAN937x_BMSR_EXTENDED_CAPABLE                0x0001

//PHY Identification MSB register
#define LAN937x_PHYID1_PHY_ID_MSB                    0xFFFF
#define LAN937x_PHYID1_PHY_ID_MSB_DEFAULT            0x0007

//PHY Identification LSB register
#define LAN937x_PHYID2_PHY_ID_LSB                    0xFFFF
#define LAN937x_PHYID2_PHY_ID_LSB_DEFAULT            0x0030
#define LAN937x_PHYID2_MODEL_NUM                     0x03F0
#define LAN937x_PHYID2_MODEL_NUM_DEFAULT             0x00D0
#define LAN937x_PHYID2_REVISION_NUM                  0x000F

//PHY Auto-Negotiation Advertisement register
#define LAN937x_ANAR_REMOTE_FAULT                    0x2000
#define LAN937x_ANAR_ASYM_PAUSE                      0x0800
#define LAN937x_ANAR_SYM_PAUSE                       0x0400
#define LAN937x_ANAR_100BTX_FD                       0x0100
#define LAN937x_ANAR_100BTX_HD                       0x0080
#define LAN937x_ANAR_10BT_FD                         0x0040
#define LAN937x_ANAR_10BT_HD                         0x0020
#define LAN937x_ANAR_SELECTOR                        0x001F
#define LAN937x_ANAR_SELECTOR_DEFAULT                0x0001

//PHY Auto-Negotiation Link Partner Base Page Ability register
#define LAN937x_ANLPAR_NEXT_PAGE                     0x8000
#define LAN937x_ANLPAR_ACK                           0x4000
#define LAN937x_ANLPAR_REMOTE_FAULT                  0x2000
#define LAN937x_ANLPAR_ASYM_PAUSE                    0x0800
#define LAN937x_ANLPAR_SYM_PAUSE                     0x0400
#define LAN937x_ANLPAR_100BT4                        0x0200
#define LAN937x_ANLPAR_100BTX_FD                     0x0100
#define LAN937x_ANLPAR_100BTX_HD                     0x0080
#define LAN937x_ANLPAR_10BT_FD                       0x0040
#define LAN937x_ANLPAR_10BT_HD                       0x0020
#define LAN937x_ANLPAR_SELECTOR                      0x001F
#define LAN937x_ANLPAR_SELECTOR_DEFAULT              0x0001

//PHY Auto-Negotiation Expansion register
#define LAN937x_ANER_PAR_DETECT_FAULT                0x0010
#define LAN937x_ANER_LP_NEXT_PAGE_ABLE               0x0008
#define LAN937x_ANER_NEXT_PAGE_ABLE                  0x0004
#define LAN937x_ANER_PAGE_RECEIVED                   0x0002
#define LAN937x_ANER_LP_AN_ABLE                      0x0001

//PHY Mode Control/Status register
#define LAN937x_PMCSR_EDPWRDOWN                      0x2000
#define LAN937x_PMCSR_ENERGYON                       0x0002

//PHY Special Modes register
#define LAN937x_PSMR_MODE                            0x00E0
#define LAN937x_PSMR_MODE_10BT_HD                    0x0000
#define LAN937x_PSMR_MODE_10BT_FD                    0x0020
#define LAN937x_PSMR_MODE_100BTX_HD                  0x0040
#define LAN937x_PSMR_MODE_100BTX_FD                  0x0060
#define LAN937x_PSMR_MODE_POWER_DOWN                 0x00C0
#define LAN937x_PSMR_MODE_AN                         0x00E0
#define LAN937x_PSMR_PHYAD                           0x001F

//PHY Special Control/Status Indication register
#define LAN937x_PSCSIR_AMDIXCTRL                     0x8000
#define LAN937x_PSCSIR_AMDIXEN                       0x4000
#define LAN937x_PSCSIR_AMDIXSTATE                    0x2000
#define LAN937x_PSCSIR_SQEOFF                        0x0800
#define LAN937x_PSCSIR_VCOOFF_LP                     0x0400
#define LAN937x_PSCSIR_XPOL                          0x0010

//PHY Interrupt Source Flags register
#define LAN937x_PISR_ENERGYON                        0x0080
#define LAN937x_PISR_AN_COMPLETE                     0x0040
#define LAN937x_PISR_REMOTE_FAULT                    0x0020
#define LAN937x_PISR_LINK_DOWN                       0x0010
#define LAN937x_PISR_AN_LP_ACK                       0x0008
#define LAN937x_PISR_PAR_DETECT_FAULT                0x0004
#define LAN937x_PISR_AN_PAGE_RECEIVED                0x0002

//PHY Interrupt Mask register
#define LAN937x_PIMR_ENERGYON                        0x0080
#define LAN937x_PIMR_AN_COMPLETE                     0x0040
#define LAN937x_PIMR_REMOTE_FAULT                    0x0020
#define LAN937x_PIMR_LINK_DOWN                       0x0010
#define LAN937x_PIMR_AN_LP_ACK                       0x0008
#define LAN937x_PIMR_PAR_DETECT_FAULT                0x0004
#define LAN937x_PIMR_AN_PAGE_RECEIVED                0x0002

//PHY Special Control/Status register
#define LAN937x_PSCSR_AUTODONE                       0x1000
#define LAN937x_PSCSR_SPEED                          0x001C
#define LAN937x_PSCSR_SPEED_10BT_HD                  0x0004
#define LAN937x_PSCSR_SPEED_100BTX_HD                0x0008
#define LAN937x_PSCSR_SPEED_10BT_FD                  0x0014
#define LAN937x_PSCSR_SPEED_100BTX_FD                0x0018

//Byte Order Test register
#define LAN937x_BYTE_TEST_DEFAULT                    0x87654321

//Hardware Configuration register
#define LAN937x_HW_CFG_DEVICE_READY                  0x08000000
#define LAN937x_HW_CFG_AMDIX_EN_STRAP_STATE_PORT2    0x04000000
#define LAN937x_HW_CFG_AMDIX_EN_STRAP_STATE_PORT1    0x02000000

//Switch Fabric CSR Interface Command register
#define LAN937x_SWITCH_CSR_CMD_BUSY                  0x80000000
#define LAN937x_SWITCH_CSR_CMD_READ                  0x40000000
#define LAN937x_SWITCH_CSR_CMD_AUTO_INC              0x20000000
#define LAN937x_SWITCH_CSR_CMD_AUTO_DEC              0x10000000
#define LAN937x_SWITCH_CSR_CMD_BE                    0x000F0000
#define LAN937x_SWITCH_CSR_CMD_BE_0                  0x00010000
#define LAN937x_SWITCH_CSR_CMD_BE_1                  0x00020000
#define LAN937x_SWITCH_CSR_CMD_BE_2                  0x00040000
#define LAN937x_SWITCH_CSR_CMD_BE_3                  0x00080000
#define LAN937x_SWITCH_CSR_CMD_ADDR                  0x0000FFFF

//Switch Device ID register
#define LAN937x_SW_DEV_ID_DEVICE_TYPE                0x00FF0000
#define LAN937x_SW_DEV_ID_DEVICE_TYPE_DEFAULT        0x00030000
#define LAN937x_SW_DEV_ID_CHIP_VERSION               0x0000FF00
#define LAN937x_SW_DEV_ID_CHIP_VERSION_DEFAULT       0x00000400
#define LAN937x_SW_DEV_ID_REVISION                   0x000000FF
#define LAN937x_SW_DEV_ID_REVISION_DEFAULT           0x00000007

//Switch Reset register
#define LAN937x_SW_RESET_SW_RESET                    0x00000001

//Switch Global Interrupt Mask register
#define LAN937x_SW_IMR_BM                            0x00000040
#define LAN937x_SW_IMR_SWE                           0x00000020
#define LAN937x_SW_IMR_MAC2                          0x00000004
#define LAN937x_SW_IMR_MAC1                          0x00000002
#define LAN937x_SW_IMR_MAC0                          0x00000001

//Switch Global Interrupt Pending register
#define LAN937x_SW_IPR_BM                            0x00000040
#define LAN937x_SW_IPR_SWE                           0x00000020
#define LAN937x_SW_IPR_MAC2                          0x00000004
#define LAN937x_SW_IPR_MAC1                          0x00000002
#define LAN937x_SW_IPR_MAC0                          0x00000001

//Port x MAC Version ID register
#define LAN937x_MAC_VER_ID_DEVICE_TYPE               0x00000F00
#define LAN937x_MAC_VER_ID_DEVICE_TYPE_DEFAULT       0x00000500
#define LAN937x_MAC_VER_ID_CHIP_VERSION              0x000000F0
#define LAN937x_MAC_VER_ID_CHIP_VERSION_DEFAULT      0x00000080
#define LAN937x_MAC_VER_ID_REVISION                  0x0000000F
#define LAN937x_MAC_VER_ID_REVISION_DEFAULT          0x00000003

//Port x MAC Receive Configuration register
#define LAN937x_MAC_RX_CFG_RECEIVE_OWN_TRANSMIT_EN   0x00000020
#define LAN937x_MAC_RX_CFG_JUMBO_2K                  0x00000008
#define LAN937x_MAC_RX_CFG_REJECT_MAC_TYPES          0x00000002
#define LAN937x_MAC_RX_CFG_RX_EN                     0x00000001

//Port x MAC Transmit Configuration register
#define LAN937x_MAC_TX_CFG_MAC_COUNTER_TEST          0x00000080
#define LAN937x_MAC_TX_CFG_IFG_CONFIG                0x0000007C
#define LAN937x_MAC_TX_CFG_IFG_CONFIG_DEFAULT        0x00000054
#define LAN937x_MAC_TX_CFG_TX_PAD_EN                 0x00000002
#define LAN937x_MAC_TX_CFG_TX_EN                     0x00000001

//Switch Engine ALR Command register
#define LAN937x_SWE_ALR_CMD_MAKE_ENTRY               0x00000004
#define LAN937x_SWE_ALR_CMD_GET_FIRST_ENTRY          0x00000002
#define LAN937x_SWE_ALR_CMD_GET_NEXT_ENTRY           0x00000001

//Switch Engine ALR Write Data 0 register
#define LAN937x_SWE_ALR_WR_DAT0_MAC_ADDR             0xFFFFFFFF

//Switch Engine ALR Write Data 1 register
#define LAN937x_SWE_ALR_WR_DAT1_VALID                0x04000000
#define LAN937x_SWE_ALR_WR_DAT1_AGE_OVERRIDE         0x02000000
#define LAN937x_SWE_ALR_WR_DAT1_STATIC               0x01000000
#define LAN937x_SWE_ALR_WR_DAT1_FILTER               0x00800000
#define LAN937x_SWE_ALR_WR_DAT1_PRIORITY_EN          0x00400000
#define LAN937x_SWE_ALR_WR_DAT1_PRIORITY             0x00380000
#define LAN937x_SWE_ALR_WR_DAT1_PORT                 0x00070000
#define LAN937x_SWE_ALR_WR_DAT1_PORT_0               0x00000000
#define LAN937x_SWE_ALR_WR_DAT1_PORT_1               0x00010000
#define LAN937x_SWE_ALR_WR_DAT1_PORT_2               0x00020000
#define LAN937x_SWE_ALR_WR_DAT1_PORT_RESERVED        0x00030000
#define LAN937x_SWE_ALR_WR_DAT1_PORT_0_1             0x00040000
#define LAN937x_SWE_ALR_WR_DAT1_PORT_0_2             0x00050000
#define LAN937x_SWE_ALR_WR_DAT1_PORT_1_2             0x00060000
#define LAN937x_SWE_ALR_WR_DAT1_PORT_0_1_2           0x00070000
#define LAN937x_SWE_ALR_WR_DAT1_MAC_ADDR             0x0000FFFF

//Switch Engine ALR Read Data 0 register
#define LAN937x_SWE_ALR_RD_DAT0_MAC_ADDR             0xFFFFFFFF

//Switch Engine ALR Read Data 1 register
#define LAN937x_SWE_ALR_RD_DAT1_VALID                0x04000000
#define LAN937x_SWE_ALR_RD_DAT1_END_OF_TABLE         0x02000000
#define LAN937x_SWE_ALR_RD_DAT1_STATIC               0x01000000
#define LAN937x_SWE_ALR_RD_DAT1_FILTER               0x00800000
#define LAN937x_SWE_ALR_RD_DAT1_PRIORITY_EN          0x00400000
#define LAN937x_SWE_ALR_RD_DAT1_PRIORITY             0x00380000
#define LAN937x_SWE_ALR_RD_DAT1_PORT                 0x00070000
#define LAN937x_SWE_ALR_RD_DAT1_PORT_0               0x00000000
#define LAN937x_SWE_ALR_RD_DAT1_PORT_1               0x00010000
#define LAN937x_SWE_ALR_RD_DAT1_PORT_2               0x00020000
#define LAN937x_SWE_ALR_RD_DAT1_PORT_RESERVED        0x00030000
#define LAN937x_SWE_ALR_RD_DAT1_PORT_0_1             0x00040000
#define LAN937x_SWE_ALR_RD_DAT1_PORT_0_2             0x00050000
#define LAN937x_SWE_ALR_RD_DAT1_PORT_1_2             0x00060000
#define LAN937x_SWE_ALR_RD_DAT1_PORT_0_1_2           0x00070000
#define LAN937x_SWE_ALR_RD_DAT1_MAC_ADDR             0x0000FFFF

//Switch Engine ALR Command Status register
#define LAN937x_SWE_ALR_CMD_STS_ALR_INIT_DONE        0x00000002
#define LAN937x_SWE_ALR_CMD_STS_MAKE_PENDING         0x00000001

//Switch Engine ALR Configuration register
#define LAN937x_SWE_ALR_CFG_ALR_AGE_TEST             0x00000001

//Switch Engine Port State register
#define LAN937x_SWE_PORT_STATE_PORT2                 0x00000030
#define LAN937x_SWE_PORT_STATE_PORT2_FORWARDING      0x00000000
#define LAN937x_SWE_PORT_STATE_PORT2_LISTENING       0x00000010
#define LAN937x_SWE_PORT_STATE_PORT2_LEARNING        0x00000020
#define LAN937x_SWE_PORT_STATE_PORT2_DISABLED        0x00000030
#define LAN937x_SWE_PORT_STATE_PORT1                 0x0000000C
#define LAN937x_SWE_PORT_STATE_PORT1_FORWARDING      0x00000000
#define LAN937x_SWE_PORT_STATE_PORT1_LISTENING       0x00000004
#define LAN937x_SWE_PORT_STATE_PORT1_LEARNING        0x00000008
#define LAN937x_SWE_PORT_STATE_PORT1_DISABLED        0x0000000C
#define LAN937x_SWE_PORT_STATE_PORT0                 0x00000003
#define LAN937x_SWE_PORT_STATE_PORT0_FORWARDING      0x00000000
#define LAN937x_SWE_PORT_STATE_PORT0_LISTENING       0x00000001
#define LAN937x_SWE_PORT_STATE_PORT0_LEARNING        0x00000002
#define LAN937x_SWE_PORT_STATE_PORT0_DISABLED        0x00000003

//Switch Engine Port Mirroring register
#define LAN937x_SWE_PORT_MIRROR_RX_MIRRORING_FILT_EN 0x00000100
#define LAN937x_SWE_PORT_MIRROR_SNIFFER              0x000000E0
#define LAN937x_SWE_PORT_MIRROR_SNIFFER_PORT0        0x00000020
#define LAN937x_SWE_PORT_MIRROR_SNIFFER_PORT1        0x00000040
#define LAN937x_SWE_PORT_MIRROR_SNIFFER_PORT2        0x00000080
#define LAN937x_SWE_PORT_MIRROR_MIRRORED             0x0000001C
#define LAN937x_SWE_PORT_MIRROR_MIRRORED_PORT0       0x00000004
#define LAN937x_SWE_PORT_MIRROR_MIRRORED_PORT1       0x00000008
#define LAN937x_SWE_PORT_MIRROR_MIRRORED_PORT2       0x00000010
#define LAN937x_SWE_PORT_MIRROR_RX_MIRRORING_EN      0x00000002
#define LAN937x_SWE_PORT_MIRROR_TX_MIRRORING_EN      0x00000001

//Switch Engine Ingress Port Type register
#define LAN937x_SWE_INGRSS_PORT_TYP_PORT2            0x00000030
#define LAN937x_SWE_INGRSS_PORT_TYP_PORT2_DIS        0x00000000
#define LAN937x_SWE_INGRSS_PORT_TYP_PORT2_EN         0x00000030
#define LAN937x_SWE_INGRSS_PORT_TYP_PORT1            0x0000000C
#define LAN937x_SWE_INGRSS_PORT_TYP_PORT1_DIS        0x00000000
#define LAN937x_SWE_INGRSS_PORT_TYP_PORT1_EN         0x0000000C
#define LAN937x_SWE_INGRSS_PORT_TYP_PORT0            0x00000003
#define LAN937x_SWE_INGRSS_PORT_TYP_PORT0_DIS        0x00000000
#define LAN937x_SWE_INGRSS_PORT_TYP_PORT0_EN         0x00000003

//Buffer Manager Egress Port Type register
#define LAN937x_BM_EGRSS_PORT_TYPE_VID_SEL_PORT2     0x00400000
#define LAN937x_BM_EGRSS_PORT_TYPE_INSERT_TAG_PORT2  0x00200000
#define LAN937x_BM_EGRSS_PORT_TYPE_CHANGE_VID_PORT2  0x00100000
#define LAN937x_BM_EGRSS_PORT_TYPE_CHANGE_PRIO_PORT2 0x00080000
#define LAN937x_BM_EGRSS_PORT_TYPE_CHANGE_TAG_PORT2  0x00040000
#define LAN937x_BM_EGRSS_PORT_TYPE_PORT2             0x00030000
#define LAN937x_BM_EGRSS_PORT_TYPE_PORT2_DUMB        0x00000000
#define LAN937x_BM_EGRSS_PORT_TYPE_PORT2_ACCESS      0x00010000
#define LAN937x_BM_EGRSS_PORT_TYPE_PORT2_HYBRID      0x00020000
#define LAN937x_BM_EGRSS_PORT_TYPE_PORT2_CPU         0x00030000
#define LAN937x_BM_EGRSS_PORT_TYPE_VID_SEL_PORT1     0x00004000
#define LAN937x_BM_EGRSS_PORT_TYPE_INSERT_TAG_PORT1  0x00002000
#define LAN937x_BM_EGRSS_PORT_TYPE_CHANGE_VID_PORT1  0x00001000
#define LAN937x_BM_EGRSS_PORT_TYPE_CHANGE_PRIO_PORT1 0x00000800
#define LAN937x_BM_EGRSS_PORT_TYPE_CHANGE_TAG_PORT1  0x00000400
#define LAN937x_BM_EGRSS_PORT_TYPE_PORT1             0x00000300
#define LAN937x_BM_EGRSS_PORT_TYPE_PORT1_DUMB        0x00000000
#define LAN937x_BM_EGRSS_PORT_TYPE_PORT1_ACCESS      0x00000100
#define LAN937x_BM_EGRSS_PORT_TYPE_PORT1_HYBRID      0x00000200
#define LAN937x_BM_EGRSS_PORT_TYPE_PORT1_CPU         0x00000300
#define LAN937x_BM_EGRSS_PORT_TYPE_VID_SEL_PORT0     0x00000040
#define LAN937x_BM_EGRSS_PORT_TYPE_INSERT_TAG_PORT0  0x00000020
#define LAN937x_BM_EGRSS_PORT_TYPE_CHANGE_VID_PORT0  0x00000010
#define LAN937x_BM_EGRSS_PORT_TYPE_CHANGE_PRIO_PORT0 0x00000008
#define LAN937x_BM_EGRSS_PORT_TYPE_CHANGE_TAG_PORT0  0x00000004
#define LAN937x_BM_EGRSS_PORT_TYPE_PORT0             0x00000003
#define LAN937x_BM_EGRSS_PORT_TYPE_PORT0_DUMB        0x00000000
#define LAN937x_BM_EGRSS_PORT_TYPE_PORT0_ACCESS      0x00000001
#define LAN937x_BM_EGRSS_PORT_TYPE_PORT0_HYBRID      0x00000002
#define LAN937x_BM_EGRSS_PORT_TYPE_PORT0_CPU         0x00000003

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//LAN937x Ethernet switch driver
extern const SwitchDriver lan937xSwitchDriver;

//LAN937x related functions
error_t lan937xInit(NetInterface *interface);

void lan937xTick(NetInterface *interface);

void lan937xEnableIrq(NetInterface *interface);
void lan937xDisableIrq(NetInterface *interface);

void lan937xEventHandler(NetInterface *interface);

error_t lan937xTagFrame(NetInterface *interface, NetBuffer *buffer,
   size_t *offset, NetTxAncillary *ancillary);

error_t lan937xUntagFrame(NetInterface *interface, uint8_t **frame,
   size_t *length, NetRxAncillary *ancillary);

bool_t lan937xGetLinkState(NetInterface *interface, uint8_t port);
uint32_t lan937xGetLinkSpeed(NetInterface *interface, uint8_t port);
NicDuplexMode lan937xGetDuplexMode(NetInterface *interface, uint8_t port);

void lan937xSetPortState(NetInterface *interface, uint8_t port,
   SwitchPortState state);

SwitchPortState lan937xGetPortState(NetInterface *interface, uint8_t port);

void lan937xSetAgingTime(NetInterface *interface, uint32_t agingTime);
void lan937xEnableRsvdMcastTable(NetInterface *interface, bool_t enable);

error_t lan937xAddStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry);

error_t lan937xDeleteStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry);

error_t lan937xGetStaticFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry);

void lan937xFlushStaticFdbTable(NetInterface *interface);

error_t lan937xGetDynamicFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry);

void lan937xFlushDynamicFdbTable(NetInterface *interface, uint8_t port);

void lan937xWritePhyReg(NetInterface *interface, uint8_t port,
   uint8_t address, uint16_t data);

uint16_t lan937xReadPhyReg(NetInterface *interface, uint8_t port,
   uint8_t address);

void lan937xDumpPhyReg(NetInterface *interface, uint8_t port);

void lan937xWriteSysReg(NetInterface *interface, uint16_t address,
   uint32_t data);

uint32_t lan937xReadSysReg(NetInterface *interface, uint16_t address);

void lan937xDumpSysReg(NetInterface *interface);

void lan937xWriteSwitchReg(NetInterface *interface, uint16_t address,
   uint32_t data);

uint32_t lan937xReadSwitchReg(NetInterface *interface, uint16_t address);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
