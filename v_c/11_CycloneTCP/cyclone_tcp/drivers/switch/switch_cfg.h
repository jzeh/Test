#ifndef __SWITCH_CFG_H__
#define __SWITCH_CFG_H__

#include<stdint.h>
#include<stdbool.h>

#ifdef __SWITCH_DLL__
    #ifdef __DLL_EXPORT__
    #define SWITCH_CFG_API extern "C" __declspec(dllexport)
    #elif defined(__DLL_IMPORT__)
    #define SWITCH_CFG_API extern "C" __declspec(dllimport)
    #else
    #error "DLL Type Unspecified!"
    #endif /* __DLL_EXPORT__ */
#elif defined(LINUX)
    #define SWITCH_CFG_API extern "C"
#else
    #define SWITCH_CFG_API
#endif /* __SWITCH_DLL__ */

#define MAX_SWITCH_PORTS	8
#define MAX_MIB_COUNTERS	36
#define MAX_MIB_CONT_30BIT	32
#define MAX_MIB_36BIT		2
#define MAX_ACL_ENTRIES		64
#define MAX_ACL_DATA_MASK_SIZE	48	/* Bytes */
#define MAX_ACL_ACTION_SIZE	8	/* Bytes */
#define MAX_KIVR_SIZE		48	/* Bytes */
#define MAX_RFR_SIZE		4	/* Bytes */
#define MAX_RNG_BOUND_SIZE	2	/* Bytes */
#define MAX_RNG_COMP_SIZE	2	/* Bytes */
#define MAX_RNG_MASK_SIZE	4	/* Bytes */
#define MAX_BYTE_ENABLE_SIZE	14	/* Bytes */
#define MAX_MASK_DATA_BEN_SIZE	6	/* Bytes */
#define MAX_ACL_COUNTER_SIZE	4	/* Bytes */
#define MAX_ACL_NMATCH_SIZE	8	/* Bytes */
#define MAX_DSCP_X_COLOR	16
#define MAX_RX_QUEUE		8
#define MAX_METER_STREAM	8
#define MAX_MAC_LENGTH		6
#define MAX_ACL_FRAME_COUNT	4
#define MAX_ACL_PARSERS		4
#define MAX_PARSER_PER_ENTRY	2
#define MAX_RFR_PER_PARSER	10
#define MAX_ACL_PARSER		4
#define MAX_RNG_CMP_PER_PARSER	10
#define MAX_RNG_LB		16
#define MAX_RNG_UB		16
#define BYTE_MASK	0xFF
#define WORD_MASK	0xFFFF
#define DWORD_MASK	0xFFFFFFFF
#define BYTE_SHIFT	8
#define WORD_SHIFT	16
#define DWORD_SHIFT	32
#define MIB_36BIT_OFST		0x80
#define LAN9373_SGMII_PORT	4
#define SMI2PHY(x)      (x * 4)
#define SMIIDX_MAX	32
#define SMIADDRIDX      20
#define SMIDATA0IDX     21
#define SMIDATA1IDX     21
#define VPHYINDADDR     23
#define VPHYINDDATA     24
#define VPHYINDCTRL     26
#define VPHYSPECCTRL 	31
#define VPHYSPECCTRL_SPI_IND_EN 0x1000

/* PHY Info */
#define PHY_BANK_ACCESS_ENABLE  0x8000
#define PHY_BANK_ADDR           0x0F00
#define PHY_BANK_ADDR_POS       (8)
#define PHY_LINK_UP		(1)
#define PHY_LINK_DOWN		(0)
#define PHY_SMI 0x0
#define PHY_MISC 0x1
#define PHY_PCS 0x2
#define PHY_AFE 0x3
#define PHY_DSP 0x4
#define PHY_INST 0x5

#define PHY_SQI_MEAN_MAX	200
#define PHY_SQI_MEAN_AVG_CNT	120
#define PHY_SQI_OUTLIER_MAX	160
#define PHY_SQI_OUTLIER_MIN	40

#define PHY_SQI1_MEAN_MAX	299
#define PHY_SQI1_MEAN_MIN	237
#define PHY_SQI2_MEAN_MAX	237
#define PHY_SQI2_MEAN_MIN	189
#define PHY_SQI3_MEAN_MAX	189
#define PHY_SQI3_MEAN_MIN	150
#define PHY_SQI4_MEAN_MAX	150
#define PHY_SQI4_MEAN_MIN	119
#define PHY_SQI5_MEAN_MAX	119
#define PHY_SQI5_MEAN_MIN	94
#define PHY_SQI6_MEAN_MAX	94
#define PHY_SQI6_MEAN_MIN	75
#define PHY_SQI7_MEAN_MAX	75
#define PHY_SQI7_MEAN_MIN	0

#define PHY_CABLE_GOOD		0
#define PHY_OPEN_CIRCUITED	1
#define PHY_SHORT_CIRCUITED	2
#define PHY_DIAGNOSTIC_ERROR	3

typedef enum {
	PHY_SQI0 = 0,
	PHY_SQI1,
	PHY_SQI2,
	PHY_SQI3,
	PHY_SQI4,
	PHY_SQI5,
	PHY_SQI6,
	PHY_SQI7,
	PHY_SQI8,
}PHYSQIRESULT_t;

typedef struct SgmiiAddrControl {
	bool auto_inc;
	uint8_t device_addr;
	bool reg_acc_type;
	uint8_t reg_ofst;
} SgmiiAddrCtrl_t;

typedef struct VlanEntryConfig {
	bool entry_valid;
	bool fwd_opt;
	uint8_t pri;
	uint8_t mstp_idx;
	uint8_t fid;
	uint8_t untag_ports;
	uint8_t port_fwd;
	uint16_t vlan_idx;
} VlanEntryCfg_t;

typedef struct AluEntryAccessConfig {
	uint8_t fid;
	uint16_t tbl_idx;
	uint8_t mac[MAX_MAC_LENGTH];
	uint16_t  valid_cnt;
	bool use_fid;
	bool direct_acc;
	bool search;
	bool search_in_progress;
	bool search_complete;
} AluEntryAccCfg_t;

typedef struct AluEntryConfig {
	bool entry_valid;
	bool static_entry;
	bool src_fltr;
	bool dst_fltr;
	uint8_t age_pri;
	uint8_t mstp_idx;
	bool over_ride;
	bool use_fid;
	uint8_t port_fwd;
	uint8_t fid;
	uint8_t mac[MAX_MAC_LENGTH];
} AluEntryCfg_t;

typedef struct AluMcastEntryConfig_t {
	uint8_t mac[MAX_MAC_LENGTH];
	uint8_t port_fwd;
}AluMcastEntryCfg_t;

typedef struct PortMibControlConfig {
	uint8_t mib_idx;
	uint8_t port;
	bool flush;
	bool freez;
	bool freez_flush_port[MAX_SWITCH_PORTS];
} PortMibCtrlCfg_t;

typedef struct PortMibCounter {
	bool overflow;
	uint64_t counter;
} PortMibCnt_t;

typedef struct PortMibCounters {

        /* 00 */uint64_t rx_hi_pri_byte;
        /* 01 */uint64_t rx_under_sz_pkt;
        /* 02 */uint64_t rx_fragments;
        /* 03 */uint64_t rx_over_sz;
	    /* 04 */uint64_t rx_jabbers;
        /* 05 */uint64_t rx_symbol_err;
        /* 06 */uint64_t rx_crc_err;
        /* 07 */uint64_t rx_aligment_error;
        /* 08 */uint64_t rx_ctrl_8808_pkt;
        /* 09 */uint64_t rx_pause_pkt;
        /* 0A */uint64_t rx_broadcast;
        /* 0B */uint64_t rx_multicast;
        /* 0C */uint64_t rx_unicast;
        /* 0D */uint64_t rx_64_octets;
        /* 0E */uint64_t rx_65_to_127_octets;
        /* 0F */uint64_t rx_128_to_255_octets;
        /* 10 */uint64_t rx_256_to_511_octets;
        /* 11 */uint64_t rx_512_to_1023_octets;
        /* 12 */uint64_t rx_1024_to_15220_octets;
        /* 13 */uint64_t rx_1523_to_2000_octets;
        /* 14 */uint64_t rx_2001_higher;
        /* 15 */uint64_t tx_hi_pri_byte;
        /* 16 */uint64_t tx_late_colision;
        /* 17 */uint64_t tx_pause_pkts;
        /* 18 */uint64_t tx_broadcast_pkts;
        /* 19 */uint64_t tx_multicast_pkts;
        /* 1A */uint64_t tx_unicast_pkts;
        /* 1B */uint64_t tx_deferred;
        /* 1C */uint64_t tx_total_collision;
        /* 1D */uint64_t tx_exessive_collision;
        /* 1E */uint64_t tx_single_collision;
        /* 1F */uint64_t tx_multiple_collision;
        /* 80 */uint64_t rx_byte_cnt;		/* 36 bit */
        /* 81 */uint64_t tx_byte_cnt;		/* 36 bit */
        /* 81 */uint64_t rx_drop_pkts;
        /* 83 */uint64_t tx_drop_pkts;
        /* 84 */uint64_t rx_bad_byte_cnt;	/* 36 bit */
} PortMibCnts_t;

typedef struct EgressQueueConfig {
	uint8_t queue;
	uint8_t scheduler;
	uint8_t shaper;
	uint8_t queue_weight;
	uint16_t credit_high;
	uint16_t credit_low;
	uint32_t credit_inc;
	bool sdu_check;
	uint16_t max_sdu;
} EgQCfg_t;

typedef struct CutThroughConfig {
	uint8_t queue;
	bool enable;
} CutThroughCfg_t;

typedef struct PtpClockConfig {
	uint32_t time_sec; 
	uint32_t time_nsec;
	uint32_t cycle_time;
} PtpClkCfg_t;

typedef struct TasEventConfig {
	uint8_t evt_idx;
	uint8_t gate_status;
	uint32_t cycle_count;
} TasEvtCfg_t;

typedef struct TasGateCtrlConfig {
	bool config_change;
	bool gate_enable;
	bool access;
} TasGateCtrlCfg_t;

typedef struct TasInterruptConfig {
	bool cfg_done;
	bool cfg_err;
	uint8_t q_overrun_int;
} TasIntCfg_t;

typedef struct TasCurrentGateStatus {
	uint8_t gate_idx;
	uint8_t gate_state;
} TasCurGateStatus_t;

typedef struct VlanTagConfig {
	uint8_t pcp;
	uint8_t dei;
	uint16_t vid;
} VlanTagCfg_t;

typedef struct MirrorSnoopingConfig {
	bool igmp_snp_en;
	bool mld_opt;
	bool mld_snp_en;
	bool sniff_mode;
} MirrSnpCfg_t;

typedef struct MirroringControlConfig {
	bool rx_sniff;
	bool tx_sniff;
	bool sniffer_port;
} MirrCtrlCfg_t;;

typedef struct RxPriorityControlConfig {
	uint8_t high_pri;
	uint8_t or_pri;
	uint8_t mac_pri;
	uint8_t vlan_pri;
	uint8_t pcp_pri;
	uint8_t diffserv_pri;
	uint8_t acl_pri;
} RxPriCtrlCfg_t;

typedef struct RxMacControlConfig {
	bool usr_pri_ceil;
	bool vlan_hop_det_en;
	bool dis_untag_pkt;
	bool dis_tag_pkt;
	uint8_t port_dflt_pri;
} RxMacCtrlCfg_t;

typedef struct RxAuthControlConfig {
	bool acl_en;
	uint8_t auth_mode;
} RxAuthCtrlCfg_t;

typedef struct RxTrafficClassMapConfig {
	uint8_t rx_queue_pri[MAX_RX_QUEUE];
} RxTCMapCfg_t;

typedef struct DscpColorRemapConfig {
	uint8_t remap_idx;
	uint8_t dscp_color[MAX_DSCP_X_COLOR];
} DscpColRemapCfg_t;

typedef struct RxPsfpConfig {
	uint8_t non_dscp_color;
	bool color_remap_en;
	bool cnt_rst;
	bool psfp_en;
} RxPsfpCfg_t;

typedef struct RxQciMeterControlConfig {
	uint8_t qci_idx;
	bool coupling;
	bool color_mode;
	bool remap_pri_en;
	uint8_t remap_pri;
	bool remark_pri_en;
	uint8_t remark_pri;
	bool drop_yellow_en;
	bool mark_red_en;
} RxQciMetrCtrlCfg_t;

typedef struct RxMeterStreamConfig {
	uint8_t qci_idx;
	uint16_t cir;
	uint16_t pir;
	uint16_t cbs;
	uint16_t pbs;
} RxMetrStrCfg_t;

typedef struct RxQciFilterStramConfig {
	uint8_t qci_idx;
	uint16_t max_sdu_sz;
	bool remap_pri;
	bool metr_en;
	uint8_t metr_id;
	bool gate_en;
	uint8_t gate_id;
	bool max_sdu_en;
	bool ovr_sz_frm_block_en;
} RxQciFltrStrCfg_t;

typedef struct RxQciFilterStramFramesStats {
	uint8_t qci_idx;
	uint32_t frm_match_cnt;
	uint32_t frm_pass_gate_cnt;
	uint32_t frm_not_pass_gate_cnt;
	uint32_t frm_pass_max_sdu_cnt;
	uint32_t frm_not_pass_max_sdu_cnt;
	uint32_t frm_drop_cnt;
} RxQciFltrStrFrmStats_t;

typedef struct RxQciGateControlConfig {
	uint8_t qci_idx;
	bool cfg_change;
	bool acc_ctrl;
	bool inv_rx_gate_close_en;
	bool oct_exce_gate_close_en;
} RxQciGateCtrlCfg_t;

typedef struct RxGateControlListLastIndexConfig {
	uint8_t qci_idx;
	uint8_t last_idx;
} RxGateLidxCfg_t;

typedef struct RxGateControlListEventConfig {
	uint8_t qci_idx;
	uint8_t event_id;
	bool ipv_en;
	uint8_t ipv;
	bool event;
	uint32_t cyc_cnt;
} RxGateCtrlListEvtCfg_t;

typedef struct RxStreamCounterOverflowStatus {
	uint8_t qci_idx;
	bool frm_match_cnt;
	bool frm_pass_gate_cnt;
	bool frm_not_pass_gate_cnt;
	bool frm_pass_max_sdu_cnt;
	bool frm_not_pass_max_sdu_cnt;
	bool frm_drop_cnt;
} RxStrCntOvrSts_t;

typedef struct RxQciMeterStreamInterrupts {
	bool interrupt[MAX_METER_STREAM];
} RxQciMetrStrIntr_t;

typedef struct RxQciGateInterrupts {
	bool gate_inv_rx;
	bool gate_oct_excd;
	bool gate_cfg_err;
} RxQciGateIntr_t;

typedef struct AclActionConfig {
	bool frm_ts;
	bool frm_cnt_en;
	uint8_t cnt_sel;
	bool str_en;
	uint8_t str_idx;
	bool rep_vlan_en;
	uint16_t vlan_id;
	uint8_t pri_mode;
	uint8_t que_sel;
	bool remark_pri_en;
	uint8_t pri;
	uint8_t map_mode;
	uint8_t dst_port;
} AclActionCfg_t;

typedef struct AclEntryConfig {
	uint8_t acl_mask[MAX_ACL_DATA_MASK_SIZE];
	uint8_t acl_data[MAX_ACL_DATA_MASK_SIZE];
	AclActionCfg_t acl_action;
} AclEntryCfg_t;

typedef struct AclRfrConfig {
	bool rng_match_en;
	bool l4;
	bool l3;
	bool l2;
	uint16_t ofst;
	uint16_t len;
	uint8_t rng_ofst;
} AclRfrCfg_t;

typedef struct AclRuleConfig {
	AclRfrCfg_t rfr[MAX_PARSER_PER_ENTRY][MAX_RFR_PER_PARSER];
} AclRuleCfg_t;

typedef struct AclRangeConfig {
	uint16_t rng_upper_bound[MAX_RNG_UB];
	uint16_t rng_lower_bound[MAX_RNG_LB];
	uint32_t rng_bound_msk;
} AclRngCfg_t;

typedef struct AclRangeComparatorConfig {
	uint16_t rng_cmp[MAX_ACL_PARSER][MAX_RNG_CMP_PER_PARSER];
} AclRngCmpCfg_t;

typedef struct AclKivrConfig {
	uint8_t kivr[MAX_PARSER_PER_ENTRY][MAX_KIVR_SIZE];
} AclKivrCfg_t;

typedef struct AclAccessControlConfig {
	bool pri_low;
	bool tcam_flush;
	bool tcam_vben;
	bool tcam_vbi;
	uint8_t tcam_row_vld;
	uint8_t row_shift;
	uint8_t tcam_req;
	uint8_t tcam_acc;
	uint8_t num_shift;
	uint8_t tcam_addr;
} AclAccCtrlCfg_t;

typedef struct AclMaskDataActionByteEnableControlConfig {
	bool acl_mask[MAX_ACL_DATA_MASK_SIZE];
	bool acl_data[MAX_ACL_DATA_MASK_SIZE];
	bool acl_action[MAX_ACL_ACTION_SIZE];
} AclByteEnCtrlCfg_t;

typedef struct AclParserControlConfig {
	uint8_t key_fmt;
	bool key_type[MAX_ACL_PARSER];
	bool ip_opts[MAX_ACL_PARSER];
	bool vlan_tag[MAX_ACL_PARSER];
	bool abs_off[MAX_ACL_PARSER];
	bool hsr_tag[MAX_ACL_PARSER];
	bool snap_tag[MAX_ACL_PARSER];
} AclParserCtrlCfg_t;

typedef struct AclFrameCount {
	bool clear_cnt[MAX_ACL_FRAME_COUNT];
	uint32_t frm_cnt[MAX_ACL_FRAME_COUNT];
}AclFrmCnt_t;

typedef struct AclNegativeRuleMatchConfig {
	bool nmatch[MAX_ACL_ENTRIES];
} AclNRuleMatchCfg_t;

typedef struct AclIntrruptConfig {
	bool frm_cnt_int[MAX_ACL_FRAME_COUNT];
	bool tcm_op_done;
} AclIntCfg_t;

typedef struct PhySQIInformation {
	uint8_t sqi;
	uint8_t link;
} PhySQIInfo_t;


typedef struct PhyCDRefValues {
	//Defaults {20,89,35,30,96}
	uint16_t noise_margin;
	uint16_t time_margin;
	uint16_t max_distance;
	uint16_t jitter_var;
	uint16_t mintd;
} PhyCDRefVal_t;

typedef struct PhyCDIAGInformation {
	uint8_t result;
	PhyCDRefVal_t ref_val;
} PhyCDIAGInfo_t;

#if !__EXPORT_APP__
SWITCH_CFG_API int32_t get_chipid(uint8_t switch_id, uint32_t *chipid);
SWITCH_CFG_API int32_t enable_phy_over_spi(uint8_t switch_id);
SWITCH_CFG_API int32_t disable_phy_over_spi(uint8_t switch_id);
SWITCH_CFG_API int32_t sgmii_read(uint8_t switch_id, SgmiiAddrCtrl_t *cfg, uint16_t *data);
SWITCH_CFG_API int32_t sgmii_write(uint8_t switch_id, SgmiiAddrCtrl_t *cfg, uint16_t data);
SWITCH_CFG_API int32_t sgmii_iread(uint8_t switch_id, SgmiiAddrCtrl_t *in, uint16_t *data);
SWITCH_CFG_API int32_t sgmii_iwrite(uint8_t switch_id, SgmiiAddrCtrl_t *in, uint16_t data);
SWITCH_CFG_API int32_t get_vlan_entry(uint8_t switch_id, VlanEntryCfg_t *cfg);
SWITCH_CFG_API int32_t set_vlan_entry(uint8_t switch_id, VlanEntryCfg_t *cfg);
SWITCH_CFG_API int32_t clear_vlan_entries(uint8_t switch_id);
SWITCH_CFG_API int32_t read_dyn_tbl(uint8_t switch_id, AluEntryAccCfg_t *cfg, AluEntryCfg_t *out);
SWITCH_CFG_API int32_t write_dyn_tbl(uint8_t switch_id, AluEntryAccCfg_t *cfg, AluEntryCfg_t *in);
SWITCH_CFG_API int32_t read_static_tbl(uint8_t switch_id, AluEntryAccCfg_t *cfg, AluEntryCfg_t *out);
SWITCH_CFG_API int32_t write_static_tbl(uint8_t switch_id, AluEntryAccCfg_t *cfg, AluEntryCfg_t *in);
SWITCH_CFG_API int32_t read_mcast_tbl(uint8_t switch_id, uint8_t mcast_mac_l6, AluMcastEntryCfg_t *out);
SWITCH_CFG_API int32_t update_mcast_tbl(uint8_t switch_id, uint8_t mcast_mac_l6, AluMcastEntryCfg_t *entry);
SWITCH_CFG_API int32_t get_port_mib_counter(uint8_t switch_id, PortMibCtrlCfg_t *cfg, PortMibCnt_t *counter);
SWITCH_CFG_API int32_t port_mib_ctrl_cfg(uint8_t switch_id, PortMibCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t get_mib_counters(uint8_t switch_id, uint8_t port, PortMibCnts_t *stat);
SWITCH_CFG_API int32_t vphy_read(uint8_t switch_id, uint16_t smiidx, uint16_t *data);
SWITCH_CFG_API int32_t vphy_write(uint8_t switch_id, uint16_t smiidx, uint16_t data);
SWITCH_CFG_API int32_t enable_smi_indirect(uint8_t switch_id);
SWITCH_CFG_API int32_t disable_smi_indirect(uint8_t switch_id);
SWITCH_CFG_API int32_t vphy_smi_read(uint8_t switch_id, uint16_t addr, uint32_t *data);
SWITCH_CFG_API int32_t vphy_smi_write(uint8_t switch_id, uint16_t addr, uint32_t data);
SWITCH_CFG_API int32_t enable_phy_indirect_over_spi(uint8_t switch_id);
SWITCH_CFG_API int32_t disable_phy_indirect_over_spi(uint8_t switch_id);
SWITCH_CFG_API int32_t vphy_ind_read(uint8_t switch_id, uint8_t phy_port, uint16_t addr, uint16_t *data);
SWITCH_CFG_API int32_t vphy_ind_write(uint8_t switch_id, uint8_t phy_port, uint16_t addr, uint16_t data);
SWITCH_CFG_API int32_t get_egress_queue_cfg(uint8_t switch_id, uint8_t port, EgQCfg_t *cfg);
SWITCH_CFG_API int32_t set_egress_queue_cfg(uint8_t switch_id, uint8_t port, EgQCfg_t *cfg);
SWITCH_CFG_API int32_t get_cut_through_cfg(uint8_t switch_id, uint8_t port, CutThroughCfg_t *cfg);
SWITCH_CFG_API int32_t set_cut_through_cfg(uint8_t switch_id, uint8_t port, CutThroughCfg_t *cfg);
SWITCH_CFG_API int32_t get_ptp_rtc_cfg(uint8_t switch_id, PtpClkCfg_t *cfg);
SWITCH_CFG_API int32_t set_ptp_rtc_cfg(uint8_t switch_id, PtpClkCfg_t *cfg);
SWITCH_CFG_API int32_t get_ptp_trg_cfg(uint8_t switch_id, uint8_t port, PtpClkCfg_t *cfg);
SWITCH_CFG_API int32_t set_ptp_trg_cfg(uint8_t switch_id, uint8_t port, PtpClkCfg_t *cfg);
SWITCH_CFG_API int32_t get_tas_evt_cfg(uint8_t switch_id, uint8_t port, TasEvtCfg_t *cfg);
SWITCH_CFG_API int32_t set_tas_evt_cfg(uint8_t switch_id, uint8_t port, TasEvtCfg_t *cfg);
SWITCH_CFG_API int32_t get_last_gctrl_index(uint8_t switch_id, uint8_t port, uint8_t *lidx);
SWITCH_CFG_API int32_t set_last_gctrl_index(uint8_t switch_id, uint8_t port, uint8_t lidx);
SWITCH_CFG_API int32_t get_tas_gate_ctrl_cfg(uint8_t switch_id, uint8_t port, TasGateCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t set_tas_gate_ctrl_cfg(uint8_t switch_id, uint8_t port, TasGateCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t get_tas_int_cfg(uint8_t switch_id, uint8_t port, TasIntCfg_t *cfg);
SWITCH_CFG_API int32_t set_tas_int_cfg(uint8_t switch_id, uint8_t port, TasIntCfg_t *cfg);
SWITCH_CFG_API int32_t clear_tas_int_status(uint8_t switch_id, uint8_t port, TasIntCfg_t *cfg);
SWITCH_CFG_API int32_t get_tas_int_status(uint8_t switch_id, uint8_t port, TasIntCfg_t *cfg);
SWITCH_CFG_API int32_t get_tas_overrun_count(uint8_t switch_id, uint8_t port, uint8_t queue, uint16_t *overrun_cnt);
SWITCH_CFG_API int32_t get_tas_gate_state(uint8_t switch_id, uint8_t port, TasCurGateStatus_t *cfg);
SWITCH_CFG_API int32_t get_egress_dbg_tag_cfg(uint8_t switch_id, uint8_t port, uint8_t *enable);
SWITCH_CFG_API int32_t set_egress_dbg_tag_cfg(uint8_t switch_id, uint8_t port, uint8_t enable);
SWITCH_CFG_API int32_t get_egress_vid_replace_cfg(uint8_t switch_id, uint8_t port, uint8_t *enable);
SWITCH_CFG_API int32_t set_egress_vid_replace_cfg(uint8_t switch_id, uint8_t port, uint8_t enable);
SWITCH_CFG_API int32_t get_dflt_pvid_cfg(uint8_t switch_id, uint8_t port, VlanTagCfg_t *cfg);
SWITCH_CFG_API int32_t set_dflt_pvid_cfg(uint8_t switch_id, uint8_t port, VlanTagCfg_t *cfg);
SWITCH_CFG_API int32_t get_mirr_snoop_cfg(uint8_t switch_id, MirrSnpCfg_t *cfg);
SWITCH_CFG_API int32_t set_mirr_snoop_cfg(uint8_t switch_id, MirrSnpCfg_t *cfg);
SWITCH_CFG_API int32_t get_mirr_ctrl_cfg(uint8_t switch_id, uint8_t port, MirrCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t set_mirr_ctrl_cfg(uint8_t switch_id, uint8_t port, MirrCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t get_rx_pri_ctrl_cfg(uint8_t switch_id, uint8_t port, RxPriCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t set_rx_pri_ctrl_cfg(uint8_t switch_id, uint8_t port, RxPriCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t get_rx_mac_ctrl_cfg(uint8_t switch_id, uint8_t port, RxMacCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t set_rx_mac_ctrl_cfg(uint8_t switch_id, uint8_t port, RxMacCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t get_rx_auth_ctrl_cfg(uint8_t switch_id, uint8_t port, RxAuthCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t set_rx_auth_ctrl_cfg(uint8_t switch_id, uint8_t port, RxAuthCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t get_rx_tcmap_cfg(uint8_t switch_id, uint8_t port, RxTCMapCfg_t *cfg);
SWITCH_CFG_API int32_t set_rx_tcmap_cfg(uint8_t switch_id, uint8_t port, RxTCMapCfg_t *cfg);
SWITCH_CFG_API int32_t get_rx_psfp_color_remap_cfg(uint8_t switch_id, uint8_t port, DscpColRemapCfg_t *cfg);
SWITCH_CFG_API int32_t set_rx_psfp_color_remap_cfg(uint8_t switch_id, uint8_t port, DscpColRemapCfg_t *cfg);
SWITCH_CFG_API int32_t get_rx_psfp_cfg(uint8_t switch_id, uint8_t port, RxPsfpCfg_t *cfg);
SWITCH_CFG_API int32_t set_rx_psfp_cfg(uint8_t switch_id, uint8_t port, RxPsfpCfg_t *cfg);
SWITCH_CFG_API int32_t get_rx_qci_metr_ctrl_cfg(uint8_t switch_id, uint8_t port, RxQciMetrCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t set_rx_qci_metr_ctrl_cfg(uint8_t switch_id, uint8_t port, RxQciMetrCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t get_rx_metr_str_cfg(uint8_t switch_id, uint8_t port, RxMetrStrCfg_t *cfg);
SWITCH_CFG_API int32_t set_rx_metr_str_cfg(uint8_t switch_id, uint8_t port, RxMetrStrCfg_t *cfg);
SWITCH_CFG_API int32_t get_rx_qci_fltr_str_cfg(uint8_t switch_id, uint8_t port, RxQciFltrStrCfg_t *cfg);
SWITCH_CFG_API int32_t set_rx_qci_fltr_str_cfg(uint8_t switch_id, uint8_t port, RxQciFltrStrCfg_t *cfg);
SWITCH_CFG_API int32_t get_rx_qci_fltr_frm_stats(uint8_t switch_id, uint8_t port, RxQciFltrStrFrmStats_t *cfg);
SWITCH_CFG_API int32_t get_rx_qci_gate_ctrl_cfg(uint8_t switch_id, uint8_t port, RxQciGateCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t set_rx_qci_gate_ctrl_cfg(uint8_t switch_id, uint8_t port, RxQciGateCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t get_rx_gate_ptp_ctrl_cfg(uint8_t switch_id, uint8_t port, uint8_t qci_idx, PtpClkCfg_t *cfg);
SWITCH_CFG_API int32_t set_rx_gate_ptp_ctrl_cfg(uint8_t switch_id, uint8_t port, uint8_t qci_idx, PtpClkCfg_t *cfg);
SWITCH_CFG_API int32_t get_rx_gate_ctrl_last_idx_cfg(uint8_t switch_id, uint8_t port, RxGateLidxCfg_t *cfg);
SWITCH_CFG_API int32_t set_rx_gate_ctrl_last_idx_cfg(uint8_t switch_id, uint8_t port, RxGateLidxCfg_t *cfg);
SWITCH_CFG_API int32_t get_rx_gate_ctrl_evnt_cfg(uint8_t switch_id, uint8_t port, RxGateCtrlListEvtCfg_t *cfg);
SWITCH_CFG_API int32_t set_rx_gate_ctrl_evnt_cfg(uint8_t switch_id, uint8_t port, RxGateCtrlListEvtCfg_t *cfg);
SWITCH_CFG_API int32_t get_rx_str_count_status(uint8_t switch_id, uint8_t port, RxStrCntOvrSts_t *cfg);
SWITCH_CFG_API int32_t clear_rx_str_count_status(uint8_t switch_id, uint8_t port, RxStrCntOvrSts_t *cfg);
SWITCH_CFG_API int32_t get_rx_str_cnt_ovr_int_staus(uint8_t switch_id, uint8_t port, RxQciMetrStrIntr_t *cfg);
SWITCH_CFG_API int32_t get_rx_str_cnt_ovr_int_cfg(uint8_t switch_id, uint8_t port, RxQciMetrStrIntr_t *cfg);
SWITCH_CFG_API int32_t set_rx_str_cnt_ovr_int_cfg(uint8_t switch_id, uint8_t port, RxQciMetrStrIntr_t *cfg);
SWITCH_CFG_API int32_t get_rx_meter_red_int_status(uint8_t switch_id, uint8_t port, RxQciMetrStrIntr_t *cfg);
SWITCH_CFG_API int32_t clear_rx_meter_red_int_status(uint8_t switch_id, uint8_t port, RxQciMetrStrIntr_t *cfg);
SWITCH_CFG_API int32_t get_rx_meter_red_int_cfg(uint8_t switch_id, uint8_t port, RxQciMetrStrIntr_t *cfg);
SWITCH_CFG_API int32_t set_rx_meter_red_int_cfg(uint8_t switch_id, uint8_t port, RxQciMetrStrIntr_t *cfg);
SWITCH_CFG_API int32_t get_rx_str_ovr_sz_frm_int_status(uint8_t switch_id, uint8_t port, RxQciMetrStrIntr_t *cfg);
SWITCH_CFG_API int32_t clear_rx_str_ovr_sz_frm_int_status(uint8_t switch_id, uint8_t port, RxQciMetrStrIntr_t *cfg);
SWITCH_CFG_API int32_t get_rx_str_ovr_sz_frm_int_cfg(uint8_t switch_id, uint8_t port, RxQciMetrStrIntr_t *cfg);
SWITCH_CFG_API int32_t set_rx_str_ovr_sz_frm_int_cfg(uint8_t switch_id, uint8_t port, RxQciMetrStrIntr_t *cfg);
SWITCH_CFG_API int32_t get_rx_gate_int_status(uint8_t switch_id, uint8_t port, RxQciGateIntr_t *cfg);
SWITCH_CFG_API int32_t clear_rx_gate_int_status(uint8_t switch_id, uint8_t port, RxQciGateIntr_t *cfg);
SWITCH_CFG_API int32_t get_rx_gate_int_cfg(uint8_t switch_id, uint8_t port, RxQciGateIntr_t *cfg);
SWITCH_CFG_API int32_t set_rx_gate_int_cfg(uint8_t switch_id, uint8_t port, RxQciGateIntr_t *cfg);
SWITCH_CFG_API int32_t get_acl_entry(uint8_t switch_id, uint8_t port, AclEntryCfg_t *entry);
SWITCH_CFG_API int32_t set_acl_entry(uint8_t switch_id, uint8_t port, AclEntryCfg_t *entry);
SWITCH_CFG_API int32_t get_acl_rfr_entry(uint8_t switch_id, uint8_t port, AclRuleCfg_t *entry);
SWITCH_CFG_API int32_t set_acl_rfr_entry(uint8_t switch_id, uint8_t port, AclRuleCfg_t *entry);
SWITCH_CFG_API int32_t get_acl_range_entry(uint8_t switch_id, uint8_t port, AclRngCfg_t *entry);
SWITCH_CFG_API int32_t set_acl_range_entry(uint8_t switch_id, uint8_t port, AclRngCfg_t *entry);
SWITCH_CFG_API int32_t get_acl_comparater_entry(uint8_t switch_id, uint8_t port, AclRngCmpCfg_t *entry);
SWITCH_CFG_API int32_t set_acl_comparater_entry(uint8_t switch_id, uint8_t port, AclRngCmpCfg_t *entry);
SWITCH_CFG_API int32_t get_acl_kivr_entry(uint8_t switch_id, uint8_t port, AclKivrCfg_t *entry);
SWITCH_CFG_API int32_t set_acl_kivr_entry(uint8_t switch_id, uint8_t port, AclKivrCfg_t *entry);
SWITCH_CFG_API int32_t acl_acc_cfg(uint8_t switch_id, uint8_t port, AclAccCtrlCfg_t *access);
SWITCH_CFG_API int32_t get_acl_byte_en_cfg(uint8_t switch_id, uint8_t port, AclByteEnCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t set_acl_byte_en_cfg(uint8_t switch_id, uint8_t port, AclByteEnCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t get_acl_parser_cfg(uint8_t switch_id, uint8_t port, AclParserCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t set_acl_parser_cfg(uint8_t switch_id, uint8_t port, AclParserCtrlCfg_t *cfg);
SWITCH_CFG_API int32_t get_acl_counters(uint8_t switch_id, uint8_t port, AclFrmCnt_t *count);
SWITCH_CFG_API int32_t clear_acl_counters(uint8_t switch_id, uint8_t port, AclFrmCnt_t *cnt);
SWITCH_CFG_API int32_t get_acl_nmatch_cfg(uint8_t switch_id, uint8_t port, AclNRuleMatchCfg_t *cfg);
SWITCH_CFG_API int32_t set_acl_nmatch_cfg(uint8_t switch_id, uint8_t port, AclNRuleMatchCfg_t *cfg);
SWITCH_CFG_API int32_t get_acl_int_status(uint8_t switch_id, uint8_t port, AclIntCfg_t *cfg);
SWITCH_CFG_API int32_t get_acl_int_cfg(uint8_t switch_id, uint8_t port, AclIntCfg_t *cfg);
SWITCH_CFG_API int32_t set_acl_int_cfg(uint8_t switch_id, uint8_t port, AclIntCfg_t *cfg);
SWITCH_CFG_API int32_t phy_reset(uint8_t switch_id, uint8_t port);
SWITCH_CFG_API int32_t phy_set_mode(uint8_t switch_id, uint8_t port, bool master);
SWITCH_CFG_API int32_t phy_init(uint8_t switch_id, uint8_t port, bool mode);
SWITCH_CFG_API int32_t phy_get_link_status(uint8_t switch_id, uint8_t port, uint8_t *status);
SWITCH_CFG_API int32_t get_phy_sqi_info(uint8_t switch_id, uint8_t port, PhySQIInfo_t *psqi);
SWITCH_CFG_API int32_t get_phy_cabel_diag_info(uint8_t switch_id, uint8_t port, PhyCDIAGInfo_t *pcdiag);
#endif /* __EXPORT_APP__ */
#endif /* __SWITCH_CFG_H_ */
