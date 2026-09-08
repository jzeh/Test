/*
    Copyright (C) 2009 - 2019. Microchip Technology Inc. and its
    subsidiaries (Microchip).  All rights reserved.

    You are permitted to use the software and its derivatives with Microchip
    products. See the license agreement accompanying this software, if any,
    for additional info regarding your rights and obligations.

    SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY
    WARRANTY OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A
    PARTICULAR PURPOSE. IN NO EVENT SHALL MICROCHIP, OR ITS LICENSORS BE
    LIABLE OR OBLIGATED UNDER CONTRACT, NEGLIGENCE, STRICT LIABILITY,
    CONTRIBUTION, BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE THEORY FOR
    ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO
    ANY INCIDENTAL, SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES, OR OTHER
    SIMILAR COSTS. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP AND ITS
    LICENSORS LIABILITY WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, THAT YOU
    PAID DIRECTLY TO MICROCHIP TO USE THIS SOFTWARE. MICROCHIP PROVIDES THIS
    SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE TERMS.
*/


#ifndef _LAN937X_EXT_H_
#define _LAN937X_EXT_H_

#include "lan937x_reg.h"
#include "lan87xx.h"
#include "ethutil_common.h"

#define LAN937X_FAMILY          0x00937000

#define CHIPID_LAN9370          0x00937000
#define CHIPID_LAN9371          0x00937100
#define CHIPID_LAN9372          0x00937200
#define CHIPID_LAN9373          0x00937300
#define CHIPID_LAN9374          0x00937400
#define CHIPID_LAN9377          0x00937700

#define SGMII_9373_PORT	4
#define LAN937X_DYNAMIC_MAC_TABLE_ENTRIES	1024
#define LAN937X_STATIC_MAC_TABLE_ENTRIES	256
#define LAN937X_MCAST_TABLE_ENTRIES		8
#define MAX_HSR_TBL_ENTRY			1024
#define MAX_VLAN_TBL_ENTRY			4096
#define LAN937X_MULTI_MAC_TABLE_ENTRIES		8
#define LAN937X_MAX_EGRESS_QUEUE		8
#define LAN937X_MAX_GCL				256
#define LAN937X_MAX_GCL_FPGA			32
#define LAN937X_MAX_RFR				10
#define LAN937X_MAX_STREAM			8
#define LAN937X_MAX_STREAM_FPGA			4
#define LAN937X_MAX_METER			8
#define LAN937X_MAX_METER_FPGA			4
#define LAN937X_MAX_QCI_GATE			1
#define MAX_SWITCH_PORTS			8

#define ADR2RFR         0x01
#define RFR2ADR         0x02
#define ADR2KIVR        0x03
#define KIVR2ADR        0x04
#define ADR2ADR         0x05

#define ACLTCAMHW	0x01
#define ACLTCAMRAM	0x02
#define ACLRFRHW	0x03
#define ACLRFRRAM	0x04
#define ACLTCAMDATA	0x02
#define ACLKIVRHW	0x05
#define ACLKIVRRAM	0x06
#define ACLRCMHW	0x07
#define ACLRCMRAM	0x08

#define ACLTCAMMASK	0x01
#define ACLTCAMDATA	0x02
#define ACLTCAMAAR	0x03
#define ACLTCAMPARX	0x04
#define ACLTCAMPARX_1	0x05

#define ACLTCAMPARSER0	  0x00
#define ACLTCAMPARSER1	  0x01
#define ACLTCAMPARSER2	  0x02
#define ACLTCAMPARSER3	  0x03
#define ACLTCAMPARSER_0_1 0x04
#define ACLTCAMPARSER_2_3 0x05
#define ACLTCAM_UMR	0x01
#define ACLTCAM_LMR	0x02
#define ACLTCAM_RCS	0x00
#define ACLTCAM_RCMRRAM	0x00
#define ACLTCAM_RCMWRAM	0x01
#define ACLTCAM_RCMRHW	0x02
#define ACLTCAM_RCMWHW	0x03

#define ACLPARSER_X	0x00
#define ACLPARSER_Y	0x01

extern int fpga;
/* lan78xx cmds */
extern ethutil_cmds lan937x_cmds[];

bool is_lan937x(void);
extern void string_to_mac(char *s, unsigned char *mac);

/* lan937x Commands and usage */
#define CHIP_ID			"chipid"
#define CHIP_ID_HELP		"to read chipid"
#define CHIP_ID_USAGE		"chipid"
int ethutil_chipid(ethutil_cmds *cmd, char *params);

/* Register read-write */
#define REG_READ_BYTE			"r"
#define REG_READ_BYTE_HELP		"to read register byte"
#define REG_READ_BYTE_USAGE		"r addr"
int reg_read_byte(ethutil_cmds *cmd, char *params);

#define REG_READ_WORD			"rw"
#define REG_READ_WORD_HELP		"to read register word"
#define REG_READ_WORD_USAGE		"r addr"
int reg_read_word(ethutil_cmds *cmd, char *params);

#define REG_READ_DWORD			"rl"
#define REG_READ_DWORD_HELP		"to read register dword"
#define REG_READ_DWORD_USAGE		"r addr"
int reg_read_dword(ethutil_cmds *cmd, char *params);

#define REG_WRITE_BYTE			"w"
#define REG_WRITE_BYTE_HELP		"to write register byte"
#define REG_WRITE_BYTE_USAGE		"w addr data"
int reg_write_byte(ethutil_cmds *cmd, char *params);

#define REG_WRITE_WORD			"ww"
#define REG_WRITE_WORD_HELP		"to write register word"
#define REG_WRITE_WORD_USAGE		"ww addr data"
int reg_write_word(ethutil_cmds *cmd, char *params);

#define REG_WRITE_DWORD			"wl"
#define REG_WRITE_DWORD_HELP		"to write register byte dword"
#define REG_WRITE_DWORD_USAGE		"wl addr data"
int reg_write_dword(ethutil_cmds *cmd, char *params);

#define SGMII_REG_READ			"sgr"
#define SGMII_REG_READ_HELP		"read sgmii register"
#define SGMII_REG_READ_USAGE		"sgr reg"
int sgmii_reg_read(ethutil_cmds *cmd, char *params);

#define SGMII_REG_WRITE			"sgw"
#define SGMII_REG_WRITE_HELP		"write sgmii register"
#define SGMII_REG_WRITE_USAGE		"sgw reg data"
int sgmii_reg_write(ethutil_cmds *cmd, char *params);

#define SGMII_REG_IREAD			"sgir"
#define SGMII_REG_IREAD_HELP		"read sgmii indirect register"
#define SGMII_REG_IREAD_USAGE		"sgir reg"
int sgmii_reg_iread(ethutil_cmds *cmd, char *params);

#define SGMII_REG_IWRITE		"sgiw"
#define SGMII_REG_IWRITE_HELP		"write sgmii indirect register"
#define SGMII_REG_IWRITE_USAGE		"sgiw reg data"
int sgmii_reg_iwrite(ethutil_cmds *cmd, char *params);

/* VLAN Table */
#define VLAN_CONFIG			"vlan"
#define VLAN_CONFIG_HELP		"to get or set the vlan configurations"
#define VLAN_CONFIG_USAGE		"vlan vid valid fid ports untags pri mstp fo"
int vlan_config(ethutil_cmds *cmd, char *params);

/* DYNAMIC MAC Table */
#define SHOW_DYNMAC			"showmac"
#define SHOW_DYNMAC_HELP		"dispaly mac table"
#define SHOW_DYNMAC_USAGE		"showmac fid_idx mac"
int show_dyn_mac_tbl(ethutil_cmds *cmd, char *params);

#define DYNAMIC_MAC_TBL_CONFIG		"dynmac"
#define DYNAMIC_MAC_TBL_CONFIG_HELP	"to set static entry of dynamic mac table"
#define DYNAMIC_MAC_TBL_CONFIG_USAGE	"dynmac valid fid override srcfilter desfilter pri mstp ports mac"
int dyn_mac_tbl_config(ethutil_cmds *cmd, char *params);

/* STATIC MAC Table */
#define STATIC_MAC_TBL_CONFIG		"mac"
#define STATIC_MAC_TBL_CONFIG_HELP	"to set or show static mac table"
#define STATIC_MAC_TBL_CONFIG_USAGE	"mac entry valid fid usefid override srcfilter desfilter pri mstp ports mac"
int static_mac_tbl_config(ethutil_cmds *cmd, char *params);

/* MCAST Table */
#define MCAST_TBL_CONFIG		"macm"
#define MCAST_TBL_CONFIG_HELP		"to set or show reserved multicast table"
#define MCAST_TBL_CONFIG_USAGE		"macm mac_6 ports"
int mcast_tbl_config(ethutil_cmds *cmd, char *params);

/* MIB Table */
#define SHOWMIB_COUNTERS		"showmib"
#define SHOWMIB_COUNTERS_HELP		"show MIB counters"
#define SHOWMIB_COUNTERS_USAGE		"showmib port"
int show_mib_counters(ethutil_cmds *cmd, char *params);

#define CLEARMIB_COUNTERS		"clearmib"
#define CLEARMIB_COUNTERS_HELP		"clear MIB counters by flush MIB count"
#define CLEARMIB_COUNTERS_USAGE		"clearmib port"
int clear_mib_counters(ethutil_cmds *cmd, char *params);

#define READCLEARMIB_COUNTERS		"readclearmib"
#define READCLEARMIB_COUNTERS_HELP	"clear MIB counters by read MIB count"
#define READCLEARMIB_COUNTERS_USAGE	"readclearmib port"
int read_clear_mib_counters(ethutil_cmds *cmd, char *params);

/* VPHY */
#define VPHY_REG_READ                   "vphyr"
#define VPHY_REG_READ_HELP              "to read virtual PHY register"
#define VPHY_REG_READ_USAGE             "vphyr smiidx"
int vphy_reg_read(ethutil_cmds *cmd, char *params);

#define VPHY_REG_WRITE                  "vphyw"
#define VPHY_REG_WRITE_HELP             "to write virtual PHY register"
#define VPHY_REG_WRITE_USAGE            "vphyw smiidx"
int vphy_reg_write(ethutil_cmds *cmd, char *params);

#define VPHY_SMI_REG_READ               "vphysmir"
#define VPHY_SMI_REG_READ_HELP          "to virtual PHY SMI read access"
#define VPHY_SMI_REG_READ_USAGE         "vphysmir addr"
int vphy_smi_reg_read(ethutil_cmds *cmd, char *params);

#define VPHY_SMI_REG_WRITE              "vphysmiw"
#define VPHY_SMI_REG_WRITE_HELP         "to virtual PHY SMI write"
#define VPHY_SMI_REG_WRITE_USAGE        "vphysmiw addr data"
int vphy_smi_reg_write(ethutil_cmds *cmd, char *params);

#define VPHY_IND_REG_READ               "vphyindr"
#define VPHY_IND_REG_READ_HELP          "to virtual PHY indirect read access"
#define VPHY_IND_REG_READ_USAGE         "vphyindr port addr"
int vphy_ind_reg_read(ethutil_cmds *cmd, char *params);

#define VPHY_IND_REG_WRITE              "vphyindw"
#define VPHY_IND_REG_WRITE_HELP         "to virtual PHY indirect write"
#define VPHY_IND_REG_WRITE_USAGE        "vphyindw port addr data"
int vphy_ind_reg_write(ethutil_cmds *cmd, char *params);

/* Egress Queue Config commands */
/* #CMDS */
#define EGRESS_QUEUE_CONFIG		"egqcfg"
#define EGRESS_QUEUE_CONFIG_HELP	"to set or get egress queue configurations"
#define EGRESS_QUEUE_CONFIG_USAGE	"egqcfg port queue scheduler qweight shaper chigh clow cinc sducheck maxsdu"
int egress_queue_config(ethutil_cmds *cmd, char *params);

#define CUTTHROUGH_CONFIG		"cut-through"
#define CUTTHROUGH_CONFIG_HELP		"to set or get egress port queue cut-through configurations"
#define CUTTHROUGH_CONFIG_USAGE		"cut-through port queue enable"
int cuttrough_config(ethutil_cmds *cmd, char *params);

#define PTP_CONFIG			"ptp"
#define PTP_CONFIG_HELP			"to set or get egress port PTP configurations"
#define PTP_CONFIG_USAGE		"ptp port tstsec tstnsec oct"
int ptp_config(ethutil_cmds *cmd, char *params);

#define PTPTRG_CONFIG			"ptptrg"
#define PTPTRG_CONFIG_HELP		"to set or get egress port PTP trigger config with offset"
#define PTPTRG_CONFIG_USAGE		"ptptrg port ofstsec ofstnsec obsoct"
int ptptrg_config(ethutil_cmds *cmd, char *params);

#define TAS_EVT_CONFIG			"tasevtcfg"
#define TAS_EVT_CONFIG_HELP		"to set or get egress port TAS event configurations"
#define TAS_EVT_CONFIG_USAGE		"tasevtcfg port evtidx gstate cctime"
int tas_evt_config(ethutil_cmds *cmd, char *params);

#define TAS_GATE_CTRL_LAST_CONFIG	"tasgctrllastidx"
#define TAS_GATE_CTRL_LAST_CONFIG_HELP	"to set or get egress port TAS last gate control configurations"
#define TAS_GATE_CTRL_LAST_CONFIG_USAGE	"tasgctrllastidx port idx"
int tas_gate_ctrl_last_config(ethutil_cmds *cmd, char *params);

#define TAS_GATE_CTRL_CONFIG		"tasgctrlcfg"
#define TAS_GATE_CTRL_CONFIG_HELP	"to set or get egress port TAS gate control configurations"
#define TAS_GATE_CTRL_CONFIG_USAGE	"tasgctrlcfg port cfgchange genable gaccess"
int tas_gate_ctrl_config(ethutil_cmds *cmd, char *params);

#define TAS_INTERRUPT_CONFIG		"tasint"
#define TAS_INTERRUPT_CONFIG_HELP	"to set or get egress port TAS interrupt mask configurations"
#define TAS_INTERRUPT_CONFIG_USAGE	"tasint port ccdone ccerr goverrun setclear"
int tas_interrupt_config(ethutil_cmds *cmd, char *params);

#define SHOW_TAS_STATS			"showtasstat"
#define SHOW_TAS_STATS_HELP		"to set or get egress port TAS status and statistics"
#define SHOW_TAS_STATS_USAGE		"showtasstat port"
int show_tas_stats(ethutil_cmds *cmd, char *params);


#define EGRESS_DBG_TAG_CONFIG		"egdbgtag"
#define EGRESS_DBG_TAG_CONFIG_HELP	"to set or get egress port debug tag configurations"
#define EGRESS_DBG_TAG_CONFIG_USAGE	"egdbgtag port enable"
int egress_dbg_config_tag(ethutil_cmds *cmd, char *params);

#define EGRESS_VID_REPLACE_CONFIG	"egvidrep"
#define EGRESS_VID_REPLACE_CONFIG_HELP	"to set or get egress port VID replacement configurations"
#define EGRESS_VID_REPLACE_CONFIG_USAGE	"egvidrep port enable"
int egress_vid_replace_config(ethutil_cmds *cmd, char *params);

/* Ingress Config commands */
/* #CMDS */
#define MIRROR_SNOOPING_CONFIG		"mirrsnpcfg"
#define MIRROR_SNOOPING_CONFIG_HELP	"to set or get mirror and snooping configurations"
#define MIRROR_SNOOPING_CONFIG_USAGE	"mirrsnpcfg igmp mldopt mld sniffmode"
int mirror_snooping_config(ethutil_cmds *cmd, char *params);

#define RX_MIRROR_CONFIG		"rxmirrcfg"
#define RX_MIRROR_CONFIG_HELP		"to set or get receive mirror configurations"
#define RX_MIRROR_CONFIG_USAGE		"rxmirrcfg port rxsniff txsniff sniffport"
int rx_mirror_config(ethutil_cmds *cmd, char *params);

#define RX_PRIORITY_CTRL_CONFIG		"rxprictrlcfg"
#define RX_PRIORITY_CTRL_CONFIG_HELP	"to set or get receive port priority control configurations"
#define RX_PRIORITY_CTRL_CONFIG_USAGE	"rxprictrlcfg port high or mac vlan 8021p diffserv acl"
int rx_priority_ctrl_config(ethutil_cmds *cmd, char *params);

#define RX_MAC_CTRL_CONFIG		"rxmacctrlcfg"
#define RX_MAC_CTRL_CONFIG_HELP		"to set or get receive port mac control configurations"
#define RX_MAC_CTRL_CONFIG_USAGE	"rxmacctrlcfg port pricei vlanhop duntag dtag defpri"
int rx_mac_ctrl_config(ethutil_cmds *cmd, char *params);

#define RX_AUTH_CTRL_CONFIG		"rxauthctrlcfg"
#define RX_AUTH_CTRL_CONFIG_HELP	"to set or get receive port authentication control configurations"
#define RX_AUTH_CTRL_CONFIG_USAGE	"rxauthctrlcfg port acl authmode"
int rx_auth_ctrl_config(ethutil_cmds *cmd, char *params);

#define RX_TCMAP_8021p_CONFIG		"rxtcmap8021p"
#define RX_TCMAP_8021p_CONFIG_HELP	"to set or get receive port 802.1p traffic class map configurations"
#define RX_TCMAP_8021p_CONFIG_USAGE	"rxtcmap8021p port rpq0 rpq1 rpq2 rpq3 rpq4 prq5 prq6 rpq7"
int rx_tcmap_8021p_config(ethutil_cmds *cmd, char *params);

#define RX_TCMAP_DSCP_CONFIG		"rxtcmapdscp"
#define RX_TCMAP_DSCP_CONFIG_HELP	"to view the receive port dscp traffic class map configurations"
#define RX_TCMAP_DSCP_CONFIG_USAGE	"rxtcmapdscp port"
int rx_tcmap_dscp_config(ethutil_cmds *cmd, char *params);

#define RX_TCMAP_DSCP0_CONFIG		"rxtcmapdscp_0"
#define RX_TCMAP_DSCP0_CONFIG_HELP	"to set the receive port dscp traffic class map from 0-15"
#define RX_TCMAP_DSCP0_CONFIG_USAGE	"rxtcmapdscp_0 port rpq0 rpq1 prq2 rpq3 rpq4 rpq4 .... rpq15"
int rx_tcmap_dscp0_config(ethutil_cmds *cmd, char *params);

#define RX_TCMAP_DSCP1_CONFIG		"rxtcmapdscp_1"
#define RX_TCMAP_DSCP1_CONFIG_HELP	"to set the receive port dscp traffic class map from 16-31"
#define RX_TCMAP_DSCP1_CONFIG_USAGE	"rxtcmapdscp_1 port rpq0 rpq1 prq2 rpq3 rpq4 rpq4 .... rpq15"
int rx_tcmap_dscp1_config(ethutil_cmds *cmd, char *params);

#define RX_TCMAP_DSCP2_CONFIG		"rxtcmapdscp_2"
#define RX_TCMAP_DSCP2_CONFIG_HELP	"to set the receive port dscp traffic class map from 31-47"
#define RX_TCMAP_DSCP2_CONFIG_USAGE	"rxtcmapdscp_2 port rpq0 rpq1 prq2 rpq3 rpq4 rpq4 .... rpq15"
int rx_tcmap_dscp2_config(ethutil_cmds *cmd, char *params);

#define RX_TCMAP_DSCP3_CONFIG		"rxtcmapdscp_3"
#define RX_TCMAP_DSCP3_CONFIG_HELP	"to set the receive port dscp traffic class map from 48-63"
#define RX_TCMAP_DSCP3_CONFIG_USAGE	"rxtcmapdscp_3 port rpq0 rpq1 prq2 rpq3 rpq4 rpq4 .... rpq15"
int rx_tcmap_dscp3_config(ethutil_cmds *cmd, char *params);

#define RX_PSFP_CONFIG			"rxpsfpcfg"
#define RX_PSFP_CONFIG_HELP		"to set or get receive port psfp configurations"
#define RX_PSFP_CONFIG_USAGE		"rxpsfpcfg port color remapcolor psfp cntrst"
int rx_psfp_config(ethutil_cmds *cmd, char *params);

#define RX_QCI_METER_CTRL_CONFIG	"rxqcimeterctrlcfg"
#define RX_QCI_METER_CTRL_CONFIG_HELP	"to set or get receive port meter control configurations"
#define RX_QCI_METER_CTRL_CONFIG_USAGE	"rxqcimeterctrlcfg port qciidx coup color reprien repri remprien rempri drpy mre mr"
int rx_qci_meter_ctrl_config(ethutil_cmds *cmd, char *params);

#define RX_QCI_METER_STREAM_CTRL_CONFIG		"rxqcimeterstrcfg"
#define RX_QCI_METER_STREAM_CTRL_CONFIG_HELP	"to set or get receive port qci metering stream control configurations"
#define RX_QCI_METER_STREAM_CTRL_CONFIG_USAGE	"rxqcimeterstrcfg port qciidx cir pir cbs pbs"
int rx_qci_meter_stream_ctrl_config(ethutil_cmds *cmd, char *params);

#define RX_QCI_FILTER_STREAM_CONFIG		"rxqcifltrstrcfg"
#define RX_QCI_FILTER_STREAM_CONFIG_HELP	"to set or get receive port qci filtering stream control configurations"
#define RX_QCI_FILTER_STREAM_CONFIG_USAGE	"rxqcifltrstrcfg port qciidx msdue msdusz rpri mtren mtrid gen gid osfb osf"
int rx_qci_filter_stream_config(ethutil_cmds *cmd, char *params);

#define SHOW_RX_QCI_FILTER_STREAM_STATS		"showrxqcifltrstrstats"
#define SHOW_RX_QCI_FILTER_STREAM_STATS_HELP	"to view receive port qci filtering stream statistics"
#define SHOW_RX_QCI_FILTER_STREAM_STATS_USAGE	"showrxqcifltrstrstats port qciidx"
int show_rx_qci_filter_stream_stats(ethutil_cmds *cmd, char *params);

#define RX_GATE_CTRL_CONFIG		"rxgatectrlcfg"
#define RX_GATE_CTRL_CONFIG_HELP	"to set or get receive port gate control configurations"
#define RX_GATE_CTRL_CONFIG_USAGE	"rxgatectrlcfg port qciidx cfgchage access invrxgcloseen invrx oexcegcloseen oexce"
int rx_gate_ctrl_config(ethutil_cmds *cmd, char *params);

#define RX_GATE_PTP_CTRL_CONFIG		"rxgptpctrlcfg"
#define RX_GATE_PTP_CTRL_CONFIG_HELP	"to set or get receive port gate PTP control configurations"
#define RX_GATE_PTP_CTRL_CONFIG_USAGE	"rxgptpctrlcfg port qciidx cycstsec cycstnsec gcyccnt"
int rx_gate_ptp_ctrl_config(ethutil_cmds *cmd, char *params);

#define RX_GATE_PTPTRG_CTRL_CONFIG		"rxgptptrgctrlcfg"
#define RX_GATE_PTPTRG_CTRL_CONFIG_HELP		"to set or get receive port gate PTP trigger control configurations"
#define RX_GATE_PTPTRG_CTRL_CONFIG_USAGE	"rxgptptrgctrlcfg port qciidx ofstsec ofstnsec obscyccnt"
int rx_gate_ptptrg_ctrl_config(ethutil_cmds *cmd, char *params);

#define RX_GATE_CTRL_LAST_IDX_CONFIG		"rxgctrllastidx"
#define RX_GATE_CTRL_LAST_IDX_CONFIG_HELP	"to set or get gate control last index configurations"
#define RX_GATE_CTRL_LAST_IDX_CONFIG_USAGE	"rxgctrllastidx port qciidx gctrllastidx"
int rx_age_ctrl_last_idx_config(ethutil_cmds *cmd, char *params);

#define RX_GATE_CTRL_EVENT_CONFIG	"rxgctrlevtcfg"
#define RX_GATE_CTRL_EVENT_CONFIG_HELP	"to set or get receive port gate control list event configurations"
#define RX_GATE_CTRL_EVENT_CONFIG_USAGE	"rxgctrlevtcfg port qciidx evtidx ipven ipv event cctime"
int rx_gate_ctrl_event_config(ethutil_cmds *cmd, char *params);

#define RX_QCI_STREAM_STATUS		"rxqcistrstatus"
#define RX_QCI_STREAM_STATUS_HELP	"to clear or get receive port stream status"
#define RX_QCI_STREAM_STATUS_USAGE	"rxqcistrstatus port strid clear"
int rx_qci_stream_status(ethutil_cmds *cmd, char *params);

#define RX_QCI_STREAM_CNT_INTERRUPT_CONFIG		"rxqcistrcntint"
#define RX_QCI_STREAM_CNT_INTERRUPT_CONFIG_HELP		"to set or get receive port stream interrupt configurations"
#define RX_QCI_STREAM_CNT_INTERRUPT_CONFIG_USAGE	"rxqcistrcntint port intmask"
int rx_qci_stream_cnt_interrupt_config(ethutil_cmds *cmd, char *params);

#define RX_QCI_METER_RED_INTERRUPT_CONFIG		"rxqcimeterredint"
#define RX_QCI_METER_RED_INTERRUPT_CONFIG_HELP		"to set or get receive port meter red interrupt configurations"
#define RX_QCI_METER_RED_INTERRUPT_CONFIG_USAGE		"rxqcimeterredint port intmask"
int rx_qci_meter_red_interrupt_config(ethutil_cmds *cmd, char *params);

#define RX_QCI_STREAM_OVR_SIZE_INTERRUPT_CONFIG		"rxqcistrovrszint"
#define RX_QCI_STREAM_OVR_SIZE_INTERRUPT_CONFIG_HELP	"to set or get receive port stream oversize frame interrupt configurations"
#define RX_QCI_STREAM_OVR_SIZE_INTERRUPT_CONFIG_USAGE	"rxqcistrovrszint port intmask"
int rx_qci_stream_ovr_size_interrupt_config(ethutil_cmds *cmd, char *params);

#define RX_QCI_GATE_INTERRUPT_CONFIG		"rxqcigint"
#define RX_QCI_GATE_INTERRUPT_CONFIG_HELP	"to set or get receive port stream interrupt configurations"
#define RX_QCI_GATE_INTERRUPT_CONFIG_USAGE	"rxqcigint port rxinv octexcd cfgerr"
int rx_qci_gate_interrupt_config(ethutil_cmds *cmd, char *params);

/* ACL Config commands */
/* #CMDS */
#define ACL_READ_ENTRY				"aclr"
#define ACL_READ_ENTRY_HELP			"to get acl tcam settings from hardware"
#define ACL_READ_ENTRY_USAGE			"aclr port atype entry"
int ethutil_acl_read_entry(ethutil_cmds *cmd, char *params);

#define ACL_WRITE_ENTRY				"aclw"
#define ACL_WRITE_ENTRY_HELP			"to get acl tcam settings from RAM and to set hardware"
#define ACL_WRITE_ENTRY_USAGE			"aclw port atype entry pri rvld rshift nshift etype flush"
int ethutil_acl_write_entry(ethutil_cmds *cmd, char *params);

#define ACL_DATA_WRITE				"acldataw"
#define ACL_DATA_WRITE_HELP			"to write acl data and mask setting to RAM"
#define ACL_DATA_WRITE_USAGE			"acldataw atype length offset data0 data1 data2 .... data11"
int ethutil_acl_data_write(ethutil_cmds *cmd, char *params);

#define ACL_AAR_WRITE				"aclaarw"
#define ACL_AAR_WRITE_HELP			"to write acl action setting to RAM"
#define ACL_AAR_WRITE_USAGE			"aclaarw ts cnt cntsel stren strid rvtg vid qen qsel rp pri mm dport"
int ethutil_acl_aar_write(ethutil_cmds *cmd, char *params);

#define ACL_BYTE_EN_CFG				"aclwbytecfg"
#define ACL_BYTE_EN_CFG_HELP			"to set or get acl entry byte enable settings "
#define ACL_BYTE_EN_CFG_USAGE			"aclwbytecfg port atype ben0 ben1"
int ethutil_acl_byte_en_cfg(ethutil_cmds *cmd, char *params);

#define ACL_RFR_READ				"aclrfrr"
#define ACL_RFR_READ_HELP			"to get acl tcam rfr setting from hardware"
#define ACL_RFR_READ_USAGE			"aclrfrr port parser rfridx cnt"
int ethutil_acl_rfr_read(ethutil_cmds *cmd, char *params);

#define ACL_RFR_WRITE				"aclrfrw"
#define ACL_RFR_WRITE_HELP			"to get acl tcam rfr setting from RAM and to set hardware"
#define ACL_RFR_WRITE_USAGE			"aclrfrw port parser"
int ethutil_acl_rfr_write(ethutil_cmds *cmd, char *params);

#define ACL_RFR_DATA_WRITE			"aclrfrdataw"
#define ACL_RFR_DATA_WRITE_HELP			"to write acl tcam rfr settings to RAM"
#define ACL_RFR_DATA_WRITE_USAGE		"aclrfrdataw parser rfridx l4 l3 l2 ofst len rne rnofst"
int ethutil_acl_rfr_data_write(ethutil_cmds *cmd, char *params);

#define ACL_RCM					"aclrcm"
#define ACL_RCM_HELP				"to get or set acl tcam range matching parameters"
#define ACL_RCM_USAGE				"aclrcm port atype rcmtype mp r0 r1 r2 ... r10 ... r15"
int ethutil_acl_rcm(ethutil_cmds *cmd, char *params);

#define ACL_KIVR_READ				"aclkivrr"
#define ACL_KIVR_READ_HELP			"to get acl tcam kivr settings from hardware"
#define ACL_KIVR_READ_USAGE			"aclkivrr port parser"
int ethutil_acl_kivr_read(ethutil_cmds *cmd, char *params);

#define ACL_KIVR_WRITE				"aclkivrw"
#define ACL_KIVR_WRITE_HELP			"to get acl tcam kivr settings from RAM and to set hardware"
#define ACL_KIVR_WRITE_USAGE			"aclkivrw port parser"
int ethutil_acl_kivr_write(ethutil_cmds *cmd, char *params);

#define ACL_KIVR_DATA_WRITE			"aclkivrdataw"
#define ACL_KIVR_DATA_WRITE_HELP		"to write acl tcam kivr settings to RAM"
#define ACL_KIVR_DATA_WRITE_USAGE		"aclkivrdataw parser length offset data0 data1 data2 ... data11"
int ethutil_acl_kivr_data_write(ethutil_cmds *cmd, char *params);

#define ACL_PARSER_CTRL_CFG			"aclpsrcfg"
#define ACL_PARSER_CTRL_CFG_HELP		"to set or get acl parser control configurations"
#define ACL_PARSER_CTRL_CFG_USAGE			"aclpsrcfg port keyfmt keytype ipopt vtag abs hsr snap"
int ethutil_acl_parser_ctrl_cfg(ethutil_cmds *cmd, char *params);

#define ACL_FRAME_COUNTERS			"aclfrcnt"
#define ACL_FRAME_COUNTERS_HELP			"to clear or get acl frame counters"
#define ACL_FRAME_COUNTERS_USAGE		"aclfrcnt port clear"
int ethutil_acl_frame_counters(ethutil_cmds *cmd, char *params);

#define ACL_NMATCH_CFG				"aclnrmcfg"
#define ACL_NMATCH_CFG_HELP			"to set or get acl negative rule match settings"
#define ACL_NMATCH_CFG_USAGE			"aclnrmcfg port cfg0 cfg1"
int ethutil_acl_nmatch_cfg(ethutil_cmds *cmd, char *params);

#define ACL_INT_STATUS				"aclint"
#define ACL_INT_STATUS_HELP			"to get current acl tcam interrupt status"
#define ACL_INT_STATUS_USAGE			"aclint port"
int ethutil_acl_int_status(ethutil_cmds *cmd, char *params);

#define ACL_INT_CFG				"aclintcfg"
#define ACL_INT_CFG_HELP			"to get or set acl tcam interrupt settings"
#define ACL_INT_CFG_USAGE			"aclintcfg port etop efrc0 efrc1 efrc2 efrc3"
int ethutil_acl_int_cfg(ethutil_cmds *cmd, char *params);

#define ACL_BIST_CFG				"aclbistcfg"
#define ACL_BIST_CFG_HELP			"to get or set BIST Settings"
#define ACL_BIST_CFG_USAGE			"aclbistcfg port tcam sho shi resume rtn run reset"
int ethutil_acl_bist_cfg(ethutil_cmds *cmd, char *params);

#define ACL_BIST_PARAM_CFG			"aclbistcfgparam"
#define ACL_BIST_PARAM_CFG_HELP			"to set BIST Param settings"
#define ACL_BIST_PARAM_CFG_USAGE		"aclbistcfgparam port deftaddr failseq skperrcnt map0 map1 map2 map3"
int ethutil_acl_bist_param_cfg(ethutil_cmds *cmd, char *params);

/* PHY Config Commands */
#define PHY_SQI				"physqi"
#define PHY_SQI_HELP			"to get SQI results of PORT"
//#define PHY_SQI_USAGE			"physqi port mode init"
#define PHY_SQI_USAGE			"physqi port"
int ethutil_phy_sqi(ethutil_cmds *cmd, char *params);

#define PHY_CABLE_DIAG			"phycdiag"
#define PHY_CABLE_DIAG_HELP		"to get Cable Diag results of PORT"
//#define PHY_CABLE_DIAG_USAGE		"phycdiag port mode init nm tm md jv mint"
#define PHY_CABLE_DIAG_USAGE		"phycdiag port"
int ethutil_phy_cdiag(ethutil_cmds *cmd, char *params);

/* HSR Config commands */
/* #CMDS */
#define HSR_CTRL_CFG			"hsrctrlcfg"
#define HSR_CTRL_CFG_HELP		"to set or get HSR settings"
#define HSR_CTRL_CFG_USAGE		"hsrctrlcfg portmap dupdis ageen dflagecnt ageperiod mlrn ulrn psaddr hash dahash zhash flush"
int hsr_ctrl_cfg(ethutil_cmds *cmd, char *params);

#define HSR_INT_CFG			"hsrintcfg"
#define HSR_INT_CFG_HELP		"to set or get HSR interrupt config"
#define HSR_INT_CFG_USAGE		"hsrintlcfg lrnfail full wfail clear"
int hsr_int_cfg(ethutil_cmds *cmd, char *params);

#define HSR_INDX_IDX			"hsridxstatus"
#define HSR_INDX_IDX_HELP		"to get HSR access indexs"
#define HSR_INDX_IDX_USAGE		"hsrindxidx"
int hsr_indx_idx(ethutil_cmds *cmd, char *params);

#define HSR_TBL				"hsrtbl"
#define HSR_TBL_HELP			"to get or set HSR table"
#define HSR_TBL_USAGE			"hsrtbl index dst src path static age seq1 seq2 expseq1 expseq2 outseq1 outseq2"
int hsr_tlb(ethutil_cmds *cmd, char *params);

/******************************/
#endif
