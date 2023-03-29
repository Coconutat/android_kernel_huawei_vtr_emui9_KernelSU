#ifndef __HI_IPF_H__
#define __HI_IPF_H__ 
#define HI_IPF_SRST_OFFSET (0x0)
#define HI_IPF_SRST_STATE_OFFSET (0x4)
#define HI_IPF_CH_EN_OFFSET (0x8)
#define HI_IPF_EN_STATE_OFFSET (0xC)
#define HI_IPF_GATE_OFFSET (0x10)
#define HI_IPF_CTRL_OFFSET (0x14)
#define HI_IPF_DMA_CTRL0_OFFSET (0x18)
#define HI_IPF_DMA_CTRL1_OFFSET (0x1C)
#define HI_IPF_VERSION_OFFSET (0x20)
#define HI_IPF_INT0_OFFSET (0x30)
#define HI_IPF_INT1_OFFSET (0x34)
#define HI_IPF_INT_MASK0_OFFSET (0x38)
#define HI_IPF_INT_MASK1_OFFSET (0x3C)
#define HI_IPF_INT_STATE_OFFSET (0x40)
#define HI_IPF_TIME_OUT_OFFSET (0x44)
#define HI_IPF_PKT_LEN_OFFSET (0x48)
#define HI_IPF_FILTER_ZERO_INDEX_OFFSET (0x70)
#define HI_IPF_EF_BADDR_OFFSET (0x74)
#define HI_BFLT_INDEX_OFFSET (0x78)
#define HI_FLT_CHAIN_LOOP_OFFSET (0x7C)
#define HI_FLT_LOCAL_ADDR0_OFFSET (0x90)
#define HI_FLT_LOCAL_ADDR1_OFFSET (0x94)
#define HI_FLT_LOCAL_ADDR2_OFFSET (0x98)
#define HI_FLT_LOCAL_ADDR3_OFFSET (0x9C)
#define HI_FLT_REMOTE_ADDR0_OFFSET (0xA0)
#define HI_FLT_REMOTE_ADDR1_OFFSET (0xA4)
#define HI_FLT_REMOTE_ADDR2_OFFSET (0xA8)
#define HI_FLT_REMOTE_ADDR3_OFFSET (0xAC)
#define HI_FLT_REMOTE_ADDR0_MASK_OFFSET (0xB0)
#define HI_FLT_REMOTE_ADDR1_MASK_OFFSET (0xB4)
#define HI_FLT_REMOTE_ADDR2_MASK_OFFSET (0xB8)
#define HI_FLT_REMOTE_ADDR3_MASK_OFFSET (0xBC)
#define HI_FLT_LOCAL_PORT_OFFSET (0xC0)
#define HI_FLT_REMOTE_PORT_OFFSET (0xC4)
#define HI_FLT_TRAFFIC_CLASS_OFFSET (0xC8)
#define HI_FLT_LADD_MASK_OFFSET (0xCC)
#define HI_FLT_NEXT_HEADER_OFFSET (0xD0)
#define HI_FLT_FLOW_LABEL_OFFSET (0xD4)
#define HI_FLT_ICMP_OFFSET (0xD8)
#define HI_FLT_CHAIN_OFFSET (0xDC)
#define HI_FLT_SPI_OFFSET (0xE0)
#define HI_FLT_RULE_CTRL_OFFSET (0xE4)
#define HI_IPF_CH0_CTRL_OFFSET (0x100)
#define HI_IPF_CH0_STATE_OFFSET (0x104)
#define HI_IPF_CH0_BDQ_BADDR_OFFSET (0x108)
#define HI_IPF_CH0_BDQ_SIZE_OFFSET (0x10C)
#define HI_IPF_CH0_BDQ_WPTR_OFFSET (0x110)
#define HI_IPF_CH0_BDQ_RPTR_OFFSET (0x114)
#define HI_IPF_CH0_BDQ_WADDR_OFFSET (0x118)
#define HI_IPF_CH0_BDQ_RADDR_OFFSET (0x11C)
#define HI_IPF_CH0_RDQ_BADDR_OFFSET (0x120)
#define HI_IPF_CH0_RDQ_SIZE_OFFSET (0x124)
#define HI_IPF_CH0_RDQ_WPTR_OFFSET (0x128)
#define HI_IPF_CH0_RDQ_RPTR_OFFSET (0x12C)
#define HI_IPF_CH0_RDQ_WADDR_OFFSET (0x130)
#define HI_IPF_CH0_RDQ_RADDR_OFFSET (0x134)
#define HI_IPF_CH0_DQ_DEPTH_OFFSET (0x138)
#define HI_IPF_CH1_CTRL_OFFSET (0x150)
#define HI_IPF_CH1_STATE_OFFSET (0x154)
#define HI_IPF_CH1_BDQ_BADDR_OFFSET (0x158)
#define HI_IPF_CH1_BDQ_SIZE_OFFSET (0x15C)
#define HI_IPF_CH1_BDQ_WPTR_OFFSET (0x160)
#define HI_IPF_CH1_BDQ_RPTR_OFFSET (0x164)
#define HI_IPF_CH1_BDQ_WADDR_OFFSET (0x168)
#define HI_IPF_CH1_BDQ_RADDR_OFFSET (0x16C)
#define HI_IPF_CH1_RDQ_BADDR_OFFSET (0x170)
#define HI_IPF_CH1_RDQ_SIZE_OFFSET (0x174)
#define HI_IPF_CH1_RDQ_WPTR_OFFSET (0x178)
#define HI_IPF_CH1_RDQ_RPTR_OFFSET (0x17C)
#define HI_IPF_CH1_RDQ_WADDR_OFFSET (0x180)
#define HI_IPF_CH1_RDQ_RADDR_OFFSET (0x184)
#define HI_IPF_CH1_DQ_DEPTH_OFFSET (0x188)
#define HI_IPF_CH0_ADQ_CTRL_OFFSET (0x204)
#define HI_IPF_CH0_ADQ0_BASE_OFFSET (0x210)
#define HI_IPF_CH0_ADQ0_STAT_OFFSET (0x214)
#define HI_IPF_CH0_ADQ0_WPTR_OFFSET (0x218)
#define HI_IPF_CH0_ADQ0_RPTR_OFFSET (0x21C)
#define HI_IPF_CH0_ADQ1_BASE_OFFSET (0x220)
#define HI_IPF_CH0_ADQ1_STAT_OFFSET (0x224)
#define HI_IPF_CH0_ADQ1_WPTR_OFFSET (0x228)
#define HI_IPF_CH0_ADQ1_RPTR_OFFSET (0x22C)
#define HI_IPF_CH1_ADQ_CTRL_OFFSET (0x284)
#define HI_IPF_CH1_ADQ0_BASE_OFFSET (0x290)
#define HI_IPF_CH1_ADQ0_STAT_OFFSET (0x294)
#define HI_IPF_CH1_ADQ0_WPTR_OFFSET (0x298)
#define HI_IPF_CH1_ADQ0_RPTR_OFFSET (0x29C)
#define HI_IPF_CH1_ADQ1_BASE_OFFSET (0x2A0)
#define HI_IPF_CH1_ADQ1_STAT_OFFSET (0x2A4)
#define HI_IPF_CH1_ADQ1_WPTR_OFFSET (0x2A8)
#define HI_IPF_CH1_ADQ1_RPTR_OFFSET (0x2AC)
#define HI_IPF_CH0_WDATA_FAMA_ATTR_OFFSET (0x2B0)
#define HI_IPF_CH0_RDATA_FAMA_ATTR_OFFSET (0x2B4)
#define HI_IPF_CH0_RD_FAMA_ATTR_OFFSET (0x2B8)
#define HI_IPF_CH0_BD_FAMA_ATTR_OFFSET (0x2BC)
#define HI_IPF_CH0_AD_FAMA_ATTR_OFFSET (0x2C0)
#define HI_IPF_CH0_CD_FAMA_ATTR_OFFSET (0x2C4)
#define HI_IPF_CH1_WDATA_FAMA_ATTR_OFFSET (0x2C8)
#define HI_IPF_CH1_RDATA_FAMA_ATTR_OFFSET (0x2CC)
#define HI_IPF_CH1_RD_FAMA_ATTR_OFFSET (0x2D0)
#define HI_IPF_CH1_BD_FAMA_ATTR_OFFSET (0x2D4)
#define HI_IPF_CH1_AD_FAMA_ATTR_OFFSET (0x2D8)
#define HI_IPF_CH1_CD_FAMA_ATTR_OFFSET (0x2DC)
#define HI_IPF_EF_FAMA_ATTR_OFFSET (0x2E0)
#define HI_IPF_CH1_DIRECT_RDATA_FAMA_ATTR_OFFSET (0x2E4)
#define HI_IPF_MST_REGION0_START_OFFSET (0x300)
#define HI_IPF_MST_REGION0_END_OFFSET (0x304)
#define HI_IPF_MST_REGION1_START_OFFSET (0x308)
#define HI_IPF_MST_REGION1_END_OFFSET (0x30c)
#define HI_IPF_MST_REGION2_START_OFFSET (0x310)
#define HI_IPF_MST_REGION2_END_OFFSET (0x314)
#define HI_IPF_MST_REGION3_START_OFFSET (0x318)
#define HI_IPF_MST_REGION3_END_OFFSET (0x31c)
#define HI_IPF_MST_DEFAULT_OFFSET (0x320)
#define HI_IPF_MST_SECCTRL_OFFSET (0x324)
#define HI_IPF_REG_END_OFFSET (HI_IPF_MST_SECCTRL_OFFSET + 4)
#ifndef __ASSEMBLY__
typedef union
{
    struct
    {
        unsigned int ipf_srst : 1;
        unsigned int reserved : 31;
    } bits;
    unsigned int u32;
}HI_IPF_SRST_T;
typedef union
{
    struct
    {
        unsigned int ipf_srst_state : 1;
        unsigned int reserved : 31;
    } bits;
    unsigned int u32;
}HI_IPF_SRST_STATE_T;
typedef union
{
    struct
    {
        unsigned int ul_en : 1;
        unsigned int dl_en : 1;
        unsigned int reserved : 30;
    } bits;
    unsigned int u32;
}HI_IPF_CH_EN_T;
typedef union
{
    struct
    {
        unsigned int ul_en_state : 1;
        unsigned int dl_en_state : 1;
        unsigned int reserved : 30;
    } bits;
    unsigned int u32;
}HI_IPF_EN_STATE_T;
typedef union
{
    struct
    {
        unsigned int ipf_sysclk_sel0 : 1;
        unsigned int ipf_sysclk_sel1 : 1;
        unsigned int ipf_busclk_sel : 1;
        unsigned int reserved_1 : 5;
        unsigned int ipf_ram0clk_sel : 1;
        unsigned int ipf_ram1clk_sel : 1;
        unsigned int reserved_0 : 22;
    } bits;
    unsigned int u32;
}HI_IPF_GATE_T;
typedef union
{
    struct
    {
        unsigned int filter_seq : 1;
        unsigned int flt_addr_reverse : 1;
        unsigned int sp_cfg : 1;
        unsigned int sp_wrr_sel : 1;
        unsigned int max_burst_len : 2;
        unsigned int ipv6_nh_sel : 1;
        unsigned int ah_esp_sel : 1;
        unsigned int ah_disable : 1;
        unsigned int esp_disable : 1;
        unsigned int mfc_en : 1;
        unsigned int mm_en : 1;
        unsigned int add_rep : 1;
  unsigned int schedule_strategy : 1;
        unsigned int cd_from_bd_en : 1;
        unsigned int reserved : 17;
    } bits;
    unsigned int u32;
}HI_IPF_CTRL_T;
typedef union
{
    struct
    {
        unsigned int ipf_wr_ch0_id : 4;
        unsigned int ipf_wr_ch1_id : 4;
        unsigned int ipf_rd_ch0_id : 4;
        unsigned int ipf_rd_ch1_id : 4;
        unsigned int ipf_wr_ch0_pri : 3;
        unsigned int reserved_3 : 1;
        unsigned int ipf_wr_ch1_pri : 3;
        unsigned int reserved_2 : 1;
        unsigned int ipf_rd_ch0_pri : 3;
        unsigned int reserved_1 : 1;
        unsigned int ipf_rd_ch1_pri : 3;
        unsigned int reserved_0 : 1;
    } bits;
    unsigned int u32;
}HI_IPF_DMA_CTRL0_T;
typedef union
{
    struct
    {
        unsigned int outstanding_w : 1;
        unsigned int outstanding_r : 1;
        unsigned int reserved : 30;
    } bits;
    unsigned int u32;
}HI_IPF_DMA_CTRL1_T;
typedef union
{
    struct
    {
        unsigned int ul_rpt_int0 : 1;
        unsigned int ul_timeout_int0 : 1;
        unsigned int ul_disable_end_int0 : 1;
        unsigned int ul_idle_cfg_bd_int0 : 1;
        unsigned int ul_trail_cfg_bd_int0 : 1;
        unsigned int ul_noidle_clrptr_int0 : 1;
        unsigned int ul_rdq_downoverflow_int0 : 1;
        unsigned int ul_bdq_upoverflow_int0 : 1;
        unsigned int ul_rdq_full_int0 : 1;
        unsigned int ul_bdq_epty_int0 : 1;
        unsigned int ul_adq0_epty_int0 : 1;
        unsigned int ul_adq1_epty_int0 : 1;
        unsigned int reserved_1 : 4;
        unsigned int dl_rpt_int0 : 1;
        unsigned int dl_timeout_int0 : 1;
        unsigned int dl_disable_end_int0 : 1;
        unsigned int dl_idle_cfg_bd_int0 : 1;
        unsigned int dl_trail_cfg_bd_int0 : 1;
        unsigned int dl_noidle_clrptr_int0 : 1;
        unsigned int dl_rdq_downoverflow_int0 : 1;
        unsigned int dl_bdq_upoverflow_int0 : 1;
        unsigned int dl_rdq_full_int0 : 1;
        unsigned int dl_bdq_epty_int0 : 1;
        unsigned int dl_adq0_epty_int0 : 1;
        unsigned int dl_adq1_epty_int0 : 1;
        unsigned int reserved_0 : 4;
    } bits;
    unsigned int u32;
}HI_IPF_INT0_T;
typedef union
{
    struct
    {
        unsigned int ul_rpt_int1 : 1;
        unsigned int ul_timeout_int1 : 1;
        unsigned int ul_disable_end_int1 : 1;
        unsigned int ul_idle_cfg_bd_int1 : 1;
        unsigned int ul_trail_cfg_bd_int1 : 1;
        unsigned int ul_noidle_clrptr_int1 : 1;
        unsigned int ul_rdq_downoverflow_int1 : 1;
        unsigned int ul_bdq_upoverflow_int1 : 1;
        unsigned int ul_rdq_full_int1 : 1;
        unsigned int ul_bdq_epty_int1 : 1;
        unsigned int ul_adq0_epty_int1 : 1;
        unsigned int ul_adq1_epty_int1 : 1;
        unsigned int reserved_1 : 4;
        unsigned int dl_rpt_int1 : 1;
        unsigned int dl_timeout_int1 : 1;
        unsigned int dl_disable_end_int1 : 1;
        unsigned int dl_idle_cfg_bd_int1 : 1;
        unsigned int dl_trail_cfg_bd_int1 : 1;
        unsigned int dl_noidle_clrptr_int1 : 1;
        unsigned int dl_rdq_downoverflow_int1 : 1;
        unsigned int dl_bdq_upoverflow_int1 : 1;
        unsigned int dl_rdq_full_int1 : 1;
        unsigned int dl_bdq_epty_int1 : 1;
        unsigned int dl_adq0_epty_int1 : 1;
        unsigned int dl_adq1_epty_int1 : 1;
        unsigned int reserved_0 : 4;
    } bits;
    unsigned int u32;
}HI_IPF_INT1_T;
typedef union
{
    struct
    {
        unsigned int ul_rpt_mask0 : 1;
        unsigned int ul_timeout_mask0 : 1;
        unsigned int ul_disable_end_mask0 : 1;
        unsigned int ul_idle_cfg_bd_mask0 : 1;
        unsigned int ul_trail_cfg_bd_mask0 : 1;
        unsigned int ul_noidle_clrptr_mask0 : 1;
        unsigned int ul_rdq_downoverflow_mask0 : 1;
        unsigned int ul_bdq_upoverflow_mask0 : 1;
        unsigned int ul_rdq_full_mask0 : 1;
        unsigned int ul_bdq_epty_mask0 : 1;
        unsigned int ul_adq0_epty_mask0 : 1;
        unsigned int ul_adq1_epty_mask0 : 1;
        unsigned int reserved_1 : 4;
        unsigned int dl_rpt_mask0 : 1;
        unsigned int dl_timeout_mask0 : 1;
        unsigned int dl_disable_end_mask0 : 1;
        unsigned int dl_idle_cfg_bd_mask0 : 1;
        unsigned int dl_trail_cfg_bd_mask0 : 1;
        unsigned int dl_noidle_clrptr_mask0 : 1;
        unsigned int dl_rdq_downoverflow_mask0 : 1;
        unsigned int dl_bdq_upoverflow_mask0 : 1;
        unsigned int dl_rdq_full_mask0 : 1;
        unsigned int dl_bdq_epty_mask0 : 1;
        unsigned int dl_adq0_epty_mask0 : 1;
        unsigned int dl_adq1_epty_mask0 : 1;
        unsigned int reserved_0 : 4;
    } bits;
    unsigned int u32;
}HI_IPF_INT_MASK0_T;
typedef union
{
    struct
    {
        unsigned int ul_rpt_mask1 : 1;
        unsigned int ul_timeout_mask1 : 1;
        unsigned int ul_disable_end_mask1 : 1;
        unsigned int ul_idle_cfg_bd_mask1 : 1;
        unsigned int ul_trail_cfg_bd_mask1 : 1;
        unsigned int ul_noidle_clrptr_mask1 : 1;
        unsigned int ul_rdq_downoverflow_mask1 : 1;
        unsigned int ul_bdq_upoverflow_mask1 : 1;
        unsigned int ul_rdq_full_mask1 : 1;
        unsigned int ul_bdq_epty_mask1 : 1;
        unsigned int ul_adq0_epty_mask1 : 1;
        unsigned int ul_adq1_epty_mask1 : 1;
        unsigned int reserved_1 : 4;
        unsigned int dl_rpt_mask1 : 1;
        unsigned int dl_timeout_mask1 : 1;
        unsigned int dl_disable_end_mask1 : 1;
        unsigned int dl_idle_cfg_bd_mask1 : 1;
        unsigned int dl_trail_cfg_bd_mask1 : 1;
        unsigned int dl_noidle_clrptr_mask1 : 1;
        unsigned int dl_rdq_downoverflow_mask1 : 1;
        unsigned int dl_bdq_upoverflow_mask1 : 1;
        unsigned int dl_rdq_full_mask1 : 1;
        unsigned int dl_bdq_epty_mask1 : 1;
        unsigned int dl_adq0_epty_mask1 : 1;
        unsigned int dl_adq1_epty_mask1 : 1;
        unsigned int reserved_0 : 4;
    } bits;
    unsigned int u32;
}HI_IPF_INT_MASK1_T;
typedef union
{
    struct
    {
        unsigned int ul_rpt : 1;
        unsigned int ul_timeout : 1;
        unsigned int ul_disable_end : 1;
        unsigned int ul_idle_cfg_bd : 1;
        unsigned int ul_trail_cfg_bd : 1;
        unsigned int ul_noidle_clrptr : 1;
        unsigned int ul_rdq_downoverflow : 1;
        unsigned int ul_bdq_upoverflow : 1;
        unsigned int ul_rdq_full : 1;
        unsigned int ul_bdq_epty : 1;
        unsigned int ul_adq0_epty : 1;
        unsigned int ul_adq1_epty : 1;
        unsigned int reserved_1 : 4;
        unsigned int dl_rpt : 1;
        unsigned int dl_timeout : 1;
        unsigned int dl_disable_end : 1;
        unsigned int dl_idle_cfg_bd : 1;
        unsigned int dl_trail_cfg_bd : 1;
        unsigned int dl_noidle_clrptr : 1;
        unsigned int dl_rdq_downoverflow : 1;
        unsigned int dl_bdq_upoverflow : 1;
        unsigned int dl_rdq_full : 1;
        unsigned int dl_bdq_epty : 1;
        unsigned int dl_adq0_epty : 1;
        unsigned int dl_adq1_epty : 1;
        unsigned int reserved_0 : 4;
    } bits;
    unsigned int u32;
}HI_IPF_INT_STATE_T;
typedef union
{
    struct
    {
        unsigned int time_out_cfg : 16;
        unsigned int time_out_valid : 1;
        unsigned int reserved : 15;
    } bits;
    unsigned int u32;
}HI_IPF_TIME_OUT_T;
typedef union
{
    struct
    {
        unsigned int min_pkt_len : 14;
        unsigned int reserved_1 : 2;
        unsigned int max_pkt_len : 14;
        unsigned int reserved_0 : 2;
    } bits;
    unsigned int u32;
}HI_IPF_PKT_LEN_T;
typedef union
{
    struct
    {
        unsigned int ul_filter_zero_index : 9;
        unsigned int reserved_1 : 7;
        unsigned int dl_filter_zero_index : 9;
        unsigned int reserved_0 : 7;
    } bits;
    unsigned int u32;
}HI_IPF_FILTER_ZERO_INDEX_T;
typedef union
{
    struct
    {
        unsigned int ef_baddr : 32;
    } bits;
    unsigned int u32;
}HI_IPF_EF_BADDR_T;
typedef union
{
    struct
    {
        unsigned int bf_index : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_BFLT_INDEX_T;
typedef union
{
    struct
    {
        unsigned int flt_chain_loop : 1;
        unsigned int reserved : 31;
    } bits;
    unsigned int u32;
}HI_FLT_CHAIN_LOOP_T;
typedef union
{
    struct
    {
        unsigned int flt_laddr0 : 32;
    } bits;
    unsigned int u32;
}HI_FLT_LOCAL_ADDR0_T;
typedef union
{
    struct
    {
        unsigned int flt_laddr1 : 32;
    } bits;
    unsigned int u32;
}HI_FLT_LOCAL_ADDR1_T;
typedef union
{
    struct
    {
        unsigned int flt_laddr2 : 32;
    } bits;
    unsigned int u32;
}HI_FLT_LOCAL_ADDR2_T;
typedef union
{
    struct
    {
        unsigned int flt_laddr3 : 32;
    } bits;
    unsigned int u32;
}HI_FLT_LOCAL_ADDR3_T;
typedef union
{
    struct
    {
        unsigned int flt_raddr0 : 32;
    } bits;
    unsigned int u32;
}HI_FLT_REMOTE_ADDR0_T;
typedef union
{
    struct
    {
        unsigned int flt_raddr1 : 32;
    } bits;
    unsigned int u32;
}HI_FLT_REMOTE_ADDR1_T;
typedef union
{
    struct
    {
        unsigned int flt_raddr2 : 32;
    } bits;
    unsigned int u32;
}HI_FLT_REMOTE_ADDR2_T;
typedef union
{
    struct
    {
        unsigned int flt_raddr3 : 32;
    } bits;
    unsigned int u32;
}HI_FLT_REMOTE_ADDR3_T;
typedef union
{
    struct
    {
        unsigned int flt_raddr0_mask : 32;
    } bits;
    unsigned int u32;
}HI_FLT_REMOTE_ADDR0_MASK_T;
typedef union
{
    struct
    {
        unsigned int flt_raddr1_mask : 32;
    } bits;
    unsigned int u32;
}HI_FLT_REMOTE_ADDR1_MASK_T;
typedef union
{
    struct
    {
        unsigned int flt_raddr2_mask : 32;
    } bits;
    unsigned int u32;
}HI_FLT_REMOTE_ADDR2_MASK_T;
typedef union
{
    struct
    {
        unsigned int flt_raddr3_mask : 32;
    } bits;
    unsigned int u32;
}HI_FLT_REMOTE_ADDR3_MASK_T;
typedef union
{
    struct
    {
        unsigned int flt_local_port_low : 16;
        unsigned int flt_local_port_high : 16;
    } bits;
    unsigned int u32;
}HI_FLT_LOCAL_PORT_T;
typedef union
{
    struct
    {
        unsigned int flt_remote_port_low : 16;
        unsigned int flt_remote_port_high : 16;
    } bits;
    unsigned int u32;
}HI_FLT_REMOTE_PORT_T;
typedef union
{
    struct
    {
        unsigned int flt_traffic_class : 8;
        unsigned int flt_traffic_class_mask : 8;
        unsigned int reserved : 16;
    } bits;
    unsigned int u32;
}HI_FLT_TRAFFIC_CLASS_T;
typedef union
{
    struct
    {
        unsigned int flt_local_addr_mask : 7;
        unsigned int reserved : 25;
    } bits;
    unsigned int u32;
}HI_FLT_LADD_MASK_T;
typedef union
{
    struct
    {
        unsigned int flt_next_header : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_FLT_NEXT_HEADER_T;
typedef union
{
    struct
    {
        unsigned int flt_fow_lable : 20;
        unsigned int reserved : 12;
    } bits;
    unsigned int u32;
}HI_FLT_FLOW_LABEL_T;
typedef union
{
    struct
    {
        unsigned int flt_type : 8;
        unsigned int reserved_1 : 8;
        unsigned int flt_code : 8;
        unsigned int reserved_0 : 8;
    } bits;
    unsigned int u32;
}HI_FLT_ICMP_T;
typedef union
{
    struct
    {
        unsigned int flt_next_index : 9;
        unsigned int reserved_1 : 7;
        unsigned int flt_pri : 9;
        unsigned int reserved_0 : 7;
    } bits;
    unsigned int u32;
}HI_FLT_CHAIN_T;
typedef union
{
    struct
    {
        unsigned int flt_spi : 32;
    } bits;
    unsigned int u32;
}HI_FLT_SPI_T;
typedef union
{
    struct
    {
        unsigned int flt_en : 1;
        unsigned int flt_type : 1;
        unsigned int reserved_2 : 2;
        unsigned int flt_spi_en : 1;
        unsigned int flt_code_en : 1;
        unsigned int flt_type_en : 1;
        unsigned int flt_fl_en : 1;
        unsigned int flt_nh_en : 1;
        unsigned int flt_tos_en : 1;
        unsigned int flt_rport_en : 1;
        unsigned int flt_lport_en : 1;
        unsigned int flt_raddr_en : 1;
        unsigned int flt_laddr_en : 1;
        unsigned int reserved_1 : 2;
        unsigned int flt_bear_id : 6;
        unsigned int reserved_0 : 10;
    } bits;
    unsigned int u32;
}HI_FLT_RULE_CTRL_T;
typedef union
{
    struct
    {
        unsigned int ul_mod : 2;
        unsigned int ul_endian : 1;
        unsigned int ul_data_chain : 1;
        unsigned int ul_bdq_clr : 1;
        unsigned int ul_rdq_clr : 1;
        unsigned int Reserved : 10;
        unsigned int ul_wrr_value : 16;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_CTRL_T;
typedef union
{
    struct
    {
        unsigned int ul_busy : 1;
        unsigned int ul_rdq_full : 1;
        unsigned int ul_rdq_empty : 1;
        unsigned int ul_bdq_full : 1;
        unsigned int ul_bdq_empty : 1;
        unsigned int reserved_1 : 3;
        unsigned int ul_rdq_rptr_invalid : 1;
        unsigned int ul_bdq_wptr_invalid : 1;
        unsigned int reserved_0 : 22;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_STATE_T;
typedef union
{
    struct
    {
        unsigned int ul_bdq_baddr : 32;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_BDQ_BADDR_T;
typedef union
{
    struct
    {
        unsigned int ul_bdq_size : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_BDQ_SIZE_T;
typedef union
{
    struct
    {
        unsigned int ul_bdq_wptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_BDQ_WPTR_T;
typedef union
{
    struct
    {
        unsigned int ul_bdq_rptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_BDQ_RPTR_T;
typedef union
{
    struct
    {
        unsigned int ul_bdq_waddr : 32;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_BDQ_WADDR_T;
typedef union
{
    struct
    {
        unsigned int ul_bdq_raddr : 32;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_BDQ_RADDR_T;
typedef union
{
    struct
    {
        unsigned int ul_rdq_baddr : 32;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_RDQ_BADDR_T;
typedef union
{
    struct
    {
        unsigned int ul_rdq_size : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_RDQ_SIZE_T;
typedef union
{
    struct
    {
        unsigned int ul_rdq_wptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_RDQ_WPTR_T;
typedef union
{
    struct
    {
        unsigned int ul_rdq_rptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_RDQ_RPTR_T;
typedef union
{
    struct
    {
        unsigned int ul_rdq_waddr : 32;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_RDQ_WADDR_T;
typedef union
{
    struct
    {
        unsigned int ul_rdq_raddr : 32;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_RDQ_RADDR_T;
typedef union
{
    struct
    {
        unsigned int ul_bdq_depth : 9;
        unsigned int reserved_1 : 7;
        unsigned int ul_rdq_depth : 9;
        unsigned int reserved_0 : 7;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_DQ_DEPTH_T;
typedef union
{
    struct
    {
        unsigned int dl_mod : 2;
        unsigned int dl_endian : 1;
        unsigned int dl_data_chain : 1;
        unsigned int dl_bdq_clr : 1;
        unsigned int dl_rdq_clr : 1;
        unsigned int Reserved : 10;
        unsigned int dl_wrr_value : 16;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_CTRL_T;
typedef union
{
    struct
    {
        unsigned int dl_busy : 1;
        unsigned int dl_rdq_full : 1;
        unsigned int dl_rdq_empty : 1;
        unsigned int dl_bdq_full : 1;
        unsigned int dl_bdq_empty : 1;
        unsigned int reserved_1 : 3;
        unsigned int dl_rdq_rptr_invalid : 1;
        unsigned int dl_bdq_wptr_invalid : 1;
        unsigned int reserved_0 : 22;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_STATE_T;
typedef union
{
    struct
    {
        unsigned int dl_bdq_baddr : 32;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_BDQ_BADDR_T;
typedef union
{
    struct
    {
        unsigned int dl_bdq_size : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_BDQ_SIZE_T;
typedef union
{
    struct
    {
        unsigned int dl_bdq_wptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_BDQ_WPTR_T;
typedef union
{
    struct
    {
        unsigned int dl_bdq_rptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_BDQ_RPTR_T;
typedef union
{
    struct
    {
        unsigned int dl_bdq_waddr : 32;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_BDQ_WADDR_T;
typedef union
{
    struct
    {
        unsigned int dl_bdq_raddr : 32;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_BDQ_RADDR_T;
typedef union
{
    struct
    {
        unsigned int dl_rdq_baddr : 32;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_RDQ_BADDR_T;
typedef union
{
    struct
    {
        unsigned int dl_rdq_size : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_RDQ_SIZE_T;
typedef union
{
    struct
    {
        unsigned int dl_rdq_wptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_RDQ_WPTR_T;
typedef union
{
    struct
    {
        unsigned int dl_rdq_rptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_RDQ_RPTR_T;
typedef union
{
    struct
    {
        unsigned int dl_rdq_waddr : 32;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_RDQ_WADDR_T;
typedef union
{
    struct
    {
        unsigned int dl_rdq_raddr : 32;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_RDQ_RADDR_T;
typedef union
{
    struct
    {
        unsigned int dl_bdq_depth : 9;
        unsigned int reserved_1 : 7;
        unsigned int dl_rdq_depth : 9;
        unsigned int reserved_0 : 7;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_DQ_DEPTH_T;
typedef union
{
    struct
    {
        unsigned int ul_adq_en : 2;
        unsigned int ul_adq0_size_sel : 2;
        unsigned int ul_adq1_size_sel : 2;
        unsigned int reserved : 10;
        unsigned int ul_adq_plen_th : 16;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_ADQ_CTRL_T;
typedef union
{
    struct
    {
        unsigned int ul_adq0_base : 32;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_ADQ0_BASE_T;
typedef union
{
    struct
    {
        unsigned int ul_adq0_empty : 1;
        unsigned int reserved_1 : 1;
        unsigned int ul_adq0_buf_epty : 1;
        unsigned int ul_adq0_buf_full : 1;
        unsigned int ul_adq0_rptr_invalid : 1;
        unsigned int ul_adq0_wptr_invalid : 1;
        unsigned int reserved_0 : 26;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_ADQ0_STAT_T;
typedef union
{
    struct
    {
        unsigned int ul_adq0_wptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_ADQ0_WPTR_T;
typedef union
{
    struct
    {
        unsigned int ul_adq0_rptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_ADQ0_RPTR_T;
typedef union
{
    struct
    {
        unsigned int ul_adq1_base : 32;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_ADQ1_BASE_T;
typedef union
{
    struct
    {
        unsigned int ul_adq1_empty : 1;
        unsigned int reserved_1 : 1;
        unsigned int ul_adq1_buf_epty : 1;
        unsigned int ul_adq1_buf_full : 1;
        unsigned int ul_adq1_rptr_invalid : 1;
        unsigned int ul_adq1_wptr_invalid : 1;
        unsigned int reserved_0 : 26;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_ADQ1_STAT_T;
typedef union
{
    struct
    {
        unsigned int ul_adq1_wptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_ADQ1_WPTR_T;
typedef union
{
    struct
    {
        unsigned int ul_adq1_rptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH0_ADQ1_RPTR_T;
typedef union
{
    struct
    {
        unsigned int dl_adq_en : 2;
        unsigned int dl_adq0_size_sel : 2;
        unsigned int dl_adq1_size_sel : 2;
        unsigned int reserved : 10;
        unsigned int dl_adq_plen_th : 16;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_ADQ_CTRL_T;
typedef union
{
    struct
    {
        unsigned int dl_adq0_base : 32;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_ADQ0_BASE_T;
typedef union
{
    struct
    {
        unsigned int dl_adq0_empty : 1;
        unsigned int reserved_1 : 1;
        unsigned int dl_adq0_buf_epty : 1;
        unsigned int dl_adq0_buf_full : 1;
        unsigned int dl_adq0_rptr_invalid : 1;
        unsigned int dl_adq0_wptr_invalid : 1;
        unsigned int reserved_0 : 26;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_ADQ0_STAT_T;
typedef union
{
    struct
    {
        unsigned int dl_adq0_wptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_ADQ0_WPTR_T;
typedef union
{
    struct
    {
        unsigned int dl_adq0_rptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_ADQ0_RPTR_T;
typedef union
{
    struct
    {
        unsigned int dl_adq1_base : 32;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_ADQ1_BASE_T;
typedef union
{
    struct
    {
        unsigned int dl_adq1_empty : 1;
        unsigned int reserved_1 : 1;
        unsigned int dl_adq1_buf_epty : 1;
        unsigned int dl_adq1_buf_full : 1;
        unsigned int dl_adq1_rptr_invalid : 1;
        unsigned int dl_adq1_wptr_invalid : 1;
        unsigned int reserved_0 : 26;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_ADQ1_STAT_T;
typedef union
{
    struct
    {
        unsigned int dl_adq1_wptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_ADQ1_WPTR_T;
typedef union
{
    struct
    {
        unsigned int dl_adq1_rptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_IPF_CH1_ADQ1_RPTR_T;
#endif
#endif
