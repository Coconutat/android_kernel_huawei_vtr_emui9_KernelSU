#ifndef __HI_PSAM_H__
#define __HI_PSAM_H__ 
#ifndef HI_SET_GET
#define HI_SET_GET(a0,a1,a2,a3,a4) 
#endif
#define HI_PSAM_SRST_OFFSET (0x0)
#define HI_PSAM_CONFIG_OFFSET (0x4)
#define HI_PSAM_VERSION_OFFSET (0x8)
#define HI_PSAM_EN_OFFSET (0xC)
#define HI_PSAM_INT0_STAT_OFFSET (0x40)
#define HI_PSAM_INT1_STAT_OFFSET (0x44)
#define HI_PSAM_INT0_MSTAT_OFFSET (0x48)
#define HI_PSAM_INT1_MSTAT_OFFSET (0x4C)
#define HI_PSAM_INT0_MASK_OFFSET (0x50)
#define HI_PSAM_INT1_MASK_OFFSET (0x54)
#define HI_PSAM_CIPHER_CH_SOFTRESET_OFFSET (0x80)
#define HI_PSAM_CIPHER_CH_EN_OFFSET (0x84)
#define HI_PSAM_CBDQ_CONFIG_OFFSET (0x88)
#define HI_PSAM_CBDQ_BADDR_OFFSET (0x8C)
#define HI_PSAM_CBDQ_SIZE_OFFSET (0x90)
#define HI_PSAM_CBDQ_WPTR_OFFSET (0x94)
#define HI_PSAM_CBDQ_STAT_OFFSET (0x98)
#define HI_PSAM_CBDQ_WPTR_ADDR_OFFSET (0x9C)
#define HI_PSAM_CRDQ_CTRL_OFFSET (0x100)
#define HI_PSAM_CRDQ_STAT_OFFSET (0x104)
#define HI_PSAM_CRDQ_PTR_OFFSET (0x108)
#define HI_PSAM_CRDQ_RPTR_ADDR_OFFSET (0x10C)
#define HI_PSAM_IBDQ_STAT_OFFSET (0x154)
#define HI_PSAM_IBDQ_BADDR_OFFSET (0x158)
#define HI_PSAM_IBDQ_SIZE_OFFSET (0x15C)
#define HI_PSAM_IBDQ_WPTR_OFFSET (0x160)
#define HI_PSAM_IBDQ_RPTR_OFFSET (0x164)
#define HI_PSAM_IBDQ_WPTR_ADDR_OFFSET (0x168)
#define HI_PSAM_IBDQ_RPTR_ADDR_OFFSET (0x16C)
#define HI_PSAM_IBDQ_RPTR_TIMER_OFFSET (0x170)
#define HI_PSAM_IBDQ_PKT_CNT_OFFSET (0x174)
#define HI_PSAM_LBDQ_STAT_OFFSET (0x254)
#define HI_PSAM_LBDQ_BADDR_OFFSET (0x258)
#define HI_PSAM_LBDQ_SIZE_OFFSET (0x25C)
#define HI_PSAM_LBDQ_WPTR_OFFSET (0x260)
#define HI_PSAM_LBDQ_RPTR_OFFSET (0x264)
#define HI_PSAM_LBDQ_DEPTH_OFFSET (0x268)
#define HI_PSAM_ADQ_CTRL_OFFSET (0x284)
#define HI_PSAM_ADQ0_BASE_OFFSET (0x290)
#define HI_PSAM_ADQ0_STAT_OFFSET (0x294)
#define HI_PSAM_ADQ0_WPTR_OFFSET (0x298)
#define HI_PSAM_ADQ0_RPTR_OFFSET (0x29C)
#define HI_PSAM_ADQ1_BASE_OFFSET (0x2A0)
#define HI_PSAM_ADQ1_STAT_OFFSET (0x2A4)
#define HI_PSAM_ADQ1_WPTR_OFFSET (0x2A8)
#define HI_PSAM_ADQ1_RPTR_OFFSET (0x2AC)
#define HI_PSAM_ADQ_PADDR_ERR_OFFSET (0x2B0)
#define HI_PSAM_ADQ_FAMA_ATTR_OFFSET (0x2B4)
#define HI_PSAM_ADQ_PADDR_STR0_OFFSET (0x300)
#define HI_PSAM_ADQ_PADDR_END0_OFFSET (0x304)
#define HI_PSAM_ADQ_PADDR_STR1_OFFSET (0x308)
#define HI_PSAM_ADQ_PADDR_END1_OFFSET (0x30c)
#define HI_PSAM_ADQ_PADDR_STR2_OFFSET (0x310)
#define HI_PSAM_ADQ_PADDR_END2_OFFSET (0x314)
#define HI_PSAM_ADQ_PADDR_STR3_OFFSET (0x318)
#define HI_PSAM_ADQ_PADDR_END3_OFFSET (0x31c)
#define HI_PSAM_ADQ_PADDR_CTRL_OFFSET (0x320)
#define HI_PSAM_CBDQ_FAMA_ATTR_OFFSET (0x330)
#define HI_PSAM_IBDQ_FAMA_ATTR_OFFSET (0x334)
#define HI_PSAM_LBDQ_FAMA_ATTR_OFFSET (0x338)
#define HI_PSAM_CRDQ_BADDR_OFFSET (0x400)
#define HI_PSAM_REG_END_OFFSET (HI_PSAM_LBDQ_FAMA_ATTR_OFFSET + 4)
#ifndef __ASSEMBLY__
typedef union
{
    struct
    {
        unsigned int psam_srst : 1;
        unsigned int psam_idle : 1;
        unsigned int reserved : 30;
    } bits;
    unsigned int u32;
}HI_PSAM_SRST_T;
typedef union
{
    struct
    {
        unsigned int psam_rd_pri : 3;
        unsigned int psam_wr_pri : 3;
        unsigned int cbdq_wptr_update_mode : 1;
        unsigned int cbdq_wptr_update_num : 7;
        unsigned int reserved : 2;
        unsigned int psam_cfg_rsvd : 16;
    } bits;
    unsigned int u32;
}HI_PSAM_CONFIG_T;
typedef union
{
    struct
    {
        unsigned int psam_version : 32;
    } bits;
    unsigned int u32;
}HI_PSAM_VERSION_T;
typedef union
{
    struct
    {
        unsigned int psam_en : 1;
        unsigned int reserved : 31;
    } bits;
    unsigned int u32;
}HI_PSAM_EN_T;
typedef union
{
    struct
    {
        unsigned int adq0_epty_int0 : 1;
        unsigned int adq1_epty_int0 : 1;
        unsigned int lbdq_epty_int0 : 1;
        unsigned int reserved_2 : 13;
        unsigned int rd_wbuf_overflow_int0 : 1;
        unsigned int adq0_upoverflow_int0 : 1;
        unsigned int adq1_upoverflow_int0 : 1;
        unsigned int lbdq_upoverflow_int0 : 1;
        unsigned int reserved_1 : 4;
        unsigned int rd_wbuf_full_int0 : 1;
        unsigned int adq0_full_int0 : 1;
        unsigned int adq1_full_int0 : 1;
        unsigned int lbdq_full_int0 : 1;
        unsigned int reserved_0 : 4;
    } bits;
    unsigned int u32;
}HI_PSAM_INT0_STAT_T;
typedef union
{
    struct
    {
        unsigned int adq0_epty_int1 : 1;
        unsigned int adq1_epty_int1 : 1;
        unsigned int lbdq_epty_int1 : 1;
        unsigned int reserved_2 : 13;
        unsigned int rd_wbuf_overflow_int1 : 1;
        unsigned int adq0_upoverflow_int1 : 1;
        unsigned int adq1_upoverflow_int1 : 1;
        unsigned int lbdq_upoverflow_int1 : 1;
        unsigned int reserved_1 : 4;
        unsigned int rd_wbuf_full_int1 : 1;
        unsigned int adq0_full_int1 : 1;
        unsigned int adq1_full_int1 : 1;
        unsigned int lbdq_full_int1 : 1;
        unsigned int reserved_0 : 4;
    } bits;
    unsigned int u32;
}HI_PSAM_INT1_STAT_T;
typedef union
{
    struct
    {
        unsigned int adq0_epty_mstat0 : 1;
        unsigned int adq1_epty_mstat0 : 1;
        unsigned int lbdq_epty_mstat0 : 1;
        unsigned int reserved_2 : 13;
        unsigned int rd_wbuf_overflow_mstat0 : 1;
        unsigned int adq0_upoverflow_mstat0 : 1;
        unsigned int adq1_upoverflow_mstat0 : 1;
        unsigned int lbdq_upoverflow_mstat0 : 1;
        unsigned int reserved_1 : 4;
        unsigned int rd_wbuf_full_mstat0 : 1;
        unsigned int adq0_full_mstat0 : 1;
        unsigned int adq1_full_mstat0 : 1;
        unsigned int lbdq_full_mstat0 : 1;
        unsigned int reserved_0 : 4;
    } bits;
    unsigned int u32;
}HI_PSAM_INT0_MSTAT_T;
typedef union
{
    struct
    {
        unsigned int adq0_epty_mstat1 : 1;
        unsigned int adq1_epty_mstat1 : 1;
        unsigned int lbdq_epty_mstat1 : 1;
        unsigned int reserved_2 : 13;
        unsigned int rd_wbuf_overflow_mstat1 : 1;
        unsigned int adq0_upoverflow_mstat1 : 1;
        unsigned int adq1_upoverflow_mstat1 : 1;
        unsigned int lbdq_upoverflow_mstat1 : 1;
        unsigned int reserved_1 : 4;
        unsigned int rd_wbuf_full_mstat1 : 1;
        unsigned int adq0_full_mstat1 : 1;
        unsigned int adq1_full_mstat1 : 1;
        unsigned int lbdq_full_mstat1 : 1;
        unsigned int reserved_0 : 4;
    } bits;
    unsigned int u32;
}HI_PSAM_INT1_MSTAT_T;
typedef union
{
    struct
    {
        unsigned int adq0_epty_mask0 : 1;
        unsigned int adq1_epty_mask0 : 1;
        unsigned int lbdq_epty_mask0 : 1;
        unsigned int reserved_2 : 13;
        unsigned int rd_wbuf_overflow_mask0 : 1;
        unsigned int adq0_upoverflow_mask0 : 1;
        unsigned int adq1_upoverflow_mask0 : 1;
        unsigned int lbdq_upoverflow_mask0 : 1;
        unsigned int reserved_1 : 4;
        unsigned int rd_wbuf_full_mask0 : 1;
        unsigned int adq0_full_mask0 : 1;
        unsigned int adq1_full_mask0 : 1;
        unsigned int lbdq_full_mask0 : 1;
        unsigned int reserved_0 : 4;
    } bits;
    unsigned int u32;
}HI_PSAM_INT0_MASK_T;
typedef union
{
    struct
    {
        unsigned int adq0_epty_mask1 : 1;
        unsigned int adq1_epty_mask1 : 1;
        unsigned int lbdq_epty_mask1 : 1;
        unsigned int reserved_2 : 13;
        unsigned int rd_wbuf_overflow_mask1 : 1;
        unsigned int adq0_upoverflow_mask1 : 1;
        unsigned int adq1_upoverflow_mask1 : 1;
        unsigned int lbdq_upoverflow_mask1 : 1;
        unsigned int reserved_1 : 4;
        unsigned int rd_wbuf_full_mask1 : 1;
        unsigned int adq0_full_mask1 : 1;
        unsigned int adq1_full_mask1 : 1;
        unsigned int lbdq_full_mask1 : 1;
        unsigned int reserved_0 : 4;
    } bits;
    unsigned int u32;
}HI_PSAM_INT1_MASK_T;
typedef union
{
    struct
    {
        unsigned int cipher_ch_srst : 1;
        unsigned int reserved : 31;
    } bits;
    unsigned int u32;
}HI_PSAM_CIPHER_CH_SOFTRESET_T;
typedef union
{
    struct
    {
        unsigned int cipher_ch_en : 1;
        unsigned int reserved : 30;
        unsigned int cipher_ch_busy : 1;
    } bits;
    unsigned int u32;
}HI_PSAM_CIPHER_CH_EN_T;
typedef union
{
    struct
    {
        unsigned int cbd_iv_sel : 1;
        unsigned int cbd_iv_num : 1;
        unsigned int reserved_3 : 2;
        unsigned int reserved_2 : 1;
        unsigned int reserved_1 : 2;
        unsigned int reserved_0 : 25;
    } bits;
    unsigned int u32;
}HI_PSAM_CBDQ_CONFIG_T;
typedef union
{
    struct
    {
        unsigned int reserved : 3;
        unsigned int cbdq_base_addr : 29;
    } bits;
    unsigned int u32;
}HI_PSAM_CBDQ_BADDR_T;
typedef union
{
    struct
    {
        unsigned int cbdq_size : 10;
        unsigned int reserved : 22;
    } bits;
    unsigned int u32;
}HI_PSAM_CBDQ_SIZE_T;
typedef union
{
    struct
    {
        unsigned int cbdq_wptr : 10;
        unsigned int reserved : 22;
    } bits;
    unsigned int u32;
}HI_PSAM_CBDQ_WPTR_T;
typedef union
{
    struct
    {
        unsigned int local_cbdq_wptr : 10;
        unsigned int cbdq_wptr_invalid : 1;
        unsigned int reserved_1 : 5;
        unsigned int local_cbdq_rptr : 10;
        unsigned int reserved_0 : 6;
    } bits;
    unsigned int u32;
}HI_PSAM_CBDQ_STAT_T;
typedef union
{
    struct
    {
        unsigned int cbdq_wptr_addr : 32;
    } bits;
    unsigned int u32;
}HI_PSAM_CBDQ_WPTR_ADDR_T;
typedef union
{
    struct
    {
        unsigned int fc_head : 3;
        unsigned int reserved : 29;
    } bits;
    unsigned int u32;
}HI_PSAM_CRDQ_CTRL_T;
typedef union
{
    struct
    {
        unsigned int crd_wbuf1_full : 1;
        unsigned int crd_wbuf1_epty : 1;
        unsigned int crd_wbuf2_full : 1;
        unsigned int crd_wbuf2_epty : 1;
        unsigned int crd_wbuf3_full : 1;
        unsigned int crd_wbuf3_epty : 1;
        unsigned int crd_wbuf4_full : 1;
        unsigned int crd_wbuf4_epty : 1;
        unsigned int crd_wbuf5_full : 1;
        unsigned int crd_wbuf5_epty : 1;
        unsigned int reserved : 22;
    } bits;
    unsigned int u32;
}HI_PSAM_CRDQ_STAT_T;
typedef union
{
    struct
    {
        unsigned int local_crdq_wptr : 2;
        unsigned int reserved_1 : 14;
        unsigned int local_crdq_rptr : 2;
        unsigned int reserved_0 : 14;
    } bits;
    unsigned int u32;
}HI_PSAM_CRDQ_PTR_T;
typedef union
{
    struct
    {
        unsigned int crdq_rptr_addr : 32;
    } bits;
    unsigned int u32;
}HI_PSAM_CRDQ_RPTR_ADDR_T;
typedef union
{
    struct
    {
        unsigned int last_lbd_ibdq_wptr : 8;
        unsigned int last_lbd_lbdq_rptr : 8;
        unsigned int need_to_update : 1;
        unsigned int reserved : 15;
    } bits;
    unsigned int u32;
}HI_PSAM_IBDQ_STAT_T;
typedef union
{
    struct
    {
        unsigned int reserved : 3;
        unsigned int ibdq_addr : 29;
    } bits;
    unsigned int u32;
}HI_PSAM_IBDQ_BADDR_T;
typedef union
{
    struct
    {
        unsigned int ibdq_size : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_PSAM_IBDQ_SIZE_T;
typedef union
{
    struct
    {
        unsigned int ibdq_wptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_PSAM_IBDQ_WPTR_T;
typedef union
{
    struct
    {
        unsigned int ibdq_rptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_PSAM_IBDQ_RPTR_T;
typedef union
{
    struct
    {
        unsigned int ibdq_wptr_addr : 32;
    } bits;
    unsigned int u32;
}HI_PSAM_IBDQ_WPTR_ADDR_T;
typedef union
{
    struct
    {
        unsigned int ibdq_rptr_addr : 32;
    } bits;
    unsigned int u32;
}HI_PSAM_IBDQ_RPTR_ADDR_T;
typedef union
{
    struct
    {
        unsigned int ibdq_rptr_timer : 16;
        unsigned int reserved : 16;
    } bits;
    unsigned int u32;
}HI_PSAM_IBDQ_RPTR_TIMER_T;
typedef union
{
    struct
    {
        unsigned int ibdq_pkt_cnt : 32;
    } bits;
    unsigned int u32;
}HI_PSAM_IBDQ_PKT_CNT_T;
typedef union
{
    struct
    {
        unsigned int lbdq_epty : 1;
        unsigned int lbdq_full : 1;
        unsigned int lbd_rbuf_epty : 1;
        unsigned int lbd_rbuf_full : 1;
        unsigned int reserved_1 : 12;
        unsigned int lbdq_wptr_invalid : 1;
        unsigned int reserved_0 : 7;
        unsigned int local_lbdq_rptr : 8;
    } bits;
    unsigned int u32;
}HI_PSAM_LBDQ_STAT_T;
typedef union
{
    struct
    {
        unsigned int reserved : 3;
        unsigned int lbdq_addr : 29;
    } bits;
    unsigned int u32;
}HI_PSAM_LBDQ_BADDR_T;
typedef union
{
    struct
    {
        unsigned int lbdq_size : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_PSAM_LBDQ_SIZE_T;
typedef union
{
    struct
    {
        unsigned int lbdq_wptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_PSAM_LBDQ_WPTR_T;
typedef union
{
    struct
    {
        unsigned int real_lbdq_rptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_PSAM_LBDQ_RPTR_T;
typedef union
{
    struct
    {
        unsigned int lbdq_depth : 9;
        unsigned int reserved : 23;
    } bits;
    unsigned int u32;
}HI_PSAM_LBDQ_DEPTH_T;
typedef union
{
    struct
    {
        unsigned int reserved_1 : 1;
        unsigned int adq_en : 1;
        unsigned int adq0_size_sel : 2;
        unsigned int adq1_size_sel : 2;
        unsigned int reserved_0 : 10;
        unsigned int adq_plen_th : 16;
    } bits;
    unsigned int u32;
}HI_PSAM_ADQ_CTRL_T;
typedef union
{
    struct
    {
        unsigned int reserved : 3;
        unsigned int adq0_base : 29;
    } bits;
    unsigned int u32;
}HI_PSAM_ADQ0_BASE_T;
typedef union
{
    struct
    {
        unsigned int adq0_epty : 1;
        unsigned int adq0_full : 1;
        unsigned int ad0_buf_epty : 1;
        unsigned int ad0_buf_full : 1;
        unsigned int adq0_rptr_invalid : 1;
        unsigned int adq0_wptr_invalid : 1;
        unsigned int reserved : 26;
    } bits;
    unsigned int u32;
}HI_PSAM_ADQ0_STAT_T;
typedef union
{
    struct
    {
        unsigned int adq0_wptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_PSAM_ADQ0_WPTR_T;
typedef union
{
    struct
    {
        unsigned int adq0_rptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_PSAM_ADQ0_RPTR_T;
typedef union
{
    struct
    {
        unsigned int reserved : 3;
        unsigned int adq1_base : 29;
    } bits;
    unsigned int u32;
}HI_PSAM_ADQ1_BASE_T;
typedef union
{
    struct
    {
        unsigned int adq1_epty : 1;
        unsigned int adq1_full : 1;
        unsigned int ad1_buf_epty : 1;
        unsigned int ad1_buf_full : 1;
        unsigned int adq1_rptr_invalid : 1;
        unsigned int adq1_wptr_invalid : 1;
        unsigned int reserved : 26;
    } bits;
    unsigned int u32;
}HI_PSAM_ADQ1_STAT_T;
typedef union
{
    struct
    {
        unsigned int adq1_wptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_PSAM_ADQ1_WPTR_T;
typedef union
{
    struct
    {
        unsigned int adq1_rptr : 8;
        unsigned int reserved : 24;
    } bits;
    unsigned int u32;
}HI_PSAM_ADQ1_RPTR_T;
typedef union
{
    struct
    {
        unsigned int psam_crdq_baddr : 32;
    } bits;
    unsigned int u32;
}HI_PSAM_CRDQ_BADDR_T;
#if 0
HI_SET_GET(hi_psam_srst_psam_srst,psam_srst,HI_PSAM_SRST_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_SRST_OFFSET)
HI_SET_GET(hi_psam_srst_psam_idle,psam_idle,HI_PSAM_SRST_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_SRST_OFFSET)
HI_SET_GET(hi_psam_srst_reserved,reserved,HI_PSAM_SRST_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_SRST_OFFSET)
HI_SET_GET(hi_psam_config_psam_rd_pri,psam_rd_pri,HI_PSAM_CONFIG_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CONFIG_OFFSET)
HI_SET_GET(hi_psam_config_psam_wr_pri,psam_wr_pri,HI_PSAM_CONFIG_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CONFIG_OFFSET)
HI_SET_GET(hi_psam_config_cbdq_wptr_update_mode,cbdq_wptr_update_mode,HI_PSAM_CONFIG_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CONFIG_OFFSET)
HI_SET_GET(hi_psam_config_cbdq_wptr_update_num,cbdq_wptr_update_num,HI_PSAM_CONFIG_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CONFIG_OFFSET)
HI_SET_GET(hi_psam_config_reserved,reserved,HI_PSAM_CONFIG_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CONFIG_OFFSET)
HI_SET_GET(hi_psam_config_psam_cfg_rsvd,psam_cfg_rsvd,HI_PSAM_CONFIG_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CONFIG_OFFSET)
HI_SET_GET(hi_psam_version_psam_version,psam_version,HI_PSAM_VERSION_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_VERSION_OFFSET)
HI_SET_GET(hi_psam_en_psam_en,psam_en,HI_PSAM_EN_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_EN_OFFSET)
HI_SET_GET(hi_psam_en_reserved,reserved,HI_PSAM_EN_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_EN_OFFSET)
HI_SET_GET(hi_psam_int0_stat_adq0_epty_int0,adq0_epty_int0,HI_PSAM_INT0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_STAT_OFFSET)
HI_SET_GET(hi_psam_int0_stat_adq1_epty_int0,adq1_epty_int0,HI_PSAM_INT0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_STAT_OFFSET)
HI_SET_GET(hi_psam_int0_stat_lbdq_epty_int0,lbdq_epty_int0,HI_PSAM_INT0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_STAT_OFFSET)
HI_SET_GET(hi_psam_int0_stat_reserved_2,reserved_2,HI_PSAM_INT0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_STAT_OFFSET)
HI_SET_GET(hi_psam_int0_stat_rd_wbuf_overflow_int0,rd_wbuf_overflow_int0,HI_PSAM_INT0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_STAT_OFFSET)
HI_SET_GET(hi_psam_int0_stat_adq0_upoverflow_int0,adq0_upoverflow_int0,HI_PSAM_INT0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_STAT_OFFSET)
HI_SET_GET(hi_psam_int0_stat_adq1_upoverflow_int0,adq1_upoverflow_int0,HI_PSAM_INT0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_STAT_OFFSET)
HI_SET_GET(hi_psam_int0_stat_lbdq_upoverflow_int0,lbdq_upoverflow_int0,HI_PSAM_INT0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_STAT_OFFSET)
HI_SET_GET(hi_psam_int0_stat_reserved_1,reserved_1,HI_PSAM_INT0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_STAT_OFFSET)
HI_SET_GET(hi_psam_int0_stat_rd_wbuf_full_int0,rd_wbuf_full_int0,HI_PSAM_INT0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_STAT_OFFSET)
HI_SET_GET(hi_psam_int0_stat_adq0_full_int0,adq0_full_int0,HI_PSAM_INT0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_STAT_OFFSET)
HI_SET_GET(hi_psam_int0_stat_adq1_full_int0,adq1_full_int0,HI_PSAM_INT0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_STAT_OFFSET)
HI_SET_GET(hi_psam_int0_stat_lbdq_full_int0,lbdq_full_int0,HI_PSAM_INT0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_STAT_OFFSET)
HI_SET_GET(hi_psam_int0_stat_reserved_0,reserved_0,HI_PSAM_INT0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_STAT_OFFSET)
HI_SET_GET(hi_psam_int1_stat_adq0_epty_int1,adq0_epty_int1,HI_PSAM_INT1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_STAT_OFFSET)
HI_SET_GET(hi_psam_int1_stat_adq1_epty_int1,adq1_epty_int1,HI_PSAM_INT1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_STAT_OFFSET)
HI_SET_GET(hi_psam_int1_stat_lbdq_epty_int1,lbdq_epty_int1,HI_PSAM_INT1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_STAT_OFFSET)
HI_SET_GET(hi_psam_int1_stat_reserved_2,reserved_2,HI_PSAM_INT1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_STAT_OFFSET)
HI_SET_GET(hi_psam_int1_stat_rd_wbuf_overflow_int1,rd_wbuf_overflow_int1,HI_PSAM_INT1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_STAT_OFFSET)
HI_SET_GET(hi_psam_int1_stat_adq0_upoverflow_int1,adq0_upoverflow_int1,HI_PSAM_INT1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_STAT_OFFSET)
HI_SET_GET(hi_psam_int1_stat_adq1_upoverflow_int1,adq1_upoverflow_int1,HI_PSAM_INT1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_STAT_OFFSET)
HI_SET_GET(hi_psam_int1_stat_lbdq_upoverflow_int1,lbdq_upoverflow_int1,HI_PSAM_INT1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_STAT_OFFSET)
HI_SET_GET(hi_psam_int1_stat_reserved_1,reserved_1,HI_PSAM_INT1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_STAT_OFFSET)
HI_SET_GET(hi_psam_int1_stat_rd_wbuf_full_int1,rd_wbuf_full_int1,HI_PSAM_INT1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_STAT_OFFSET)
HI_SET_GET(hi_psam_int1_stat_adq0_full_int1,adq0_full_int1,HI_PSAM_INT1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_STAT_OFFSET)
HI_SET_GET(hi_psam_int1_stat_adq1_full_int1,adq1_full_int1,HI_PSAM_INT1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_STAT_OFFSET)
HI_SET_GET(hi_psam_int1_stat_lbdq_full_int1,lbdq_full_int1,HI_PSAM_INT1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_STAT_OFFSET)
HI_SET_GET(hi_psam_int1_stat_reserved_0,reserved_0,HI_PSAM_INT1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_STAT_OFFSET)
HI_SET_GET(hi_psam_int0_mstat_adq0_epty_mstat0,adq0_epty_mstat0,HI_PSAM_INT0_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int0_mstat_adq1_epty_mstat0,adq1_epty_mstat0,HI_PSAM_INT0_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int0_mstat_lbdq_epty_mstat0,lbdq_epty_mstat0,HI_PSAM_INT0_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int0_mstat_reserved_2,reserved_2,HI_PSAM_INT0_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int0_mstat_rd_wbuf_overflow_mstat0,rd_wbuf_overflow_mstat0,HI_PSAM_INT0_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int0_mstat_adq0_upoverflow_mstat0,adq0_upoverflow_mstat0,HI_PSAM_INT0_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int0_mstat_adq1_upoverflow_mstat0,adq1_upoverflow_mstat0,HI_PSAM_INT0_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int0_mstat_lbdq_upoverflow_mstat0,lbdq_upoverflow_mstat0,HI_PSAM_INT0_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int0_mstat_reserved_1,reserved_1,HI_PSAM_INT0_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int0_mstat_rd_wbuf_full_mstat0,rd_wbuf_full_mstat0,HI_PSAM_INT0_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int0_mstat_adq0_full_mstat0,adq0_full_mstat0,HI_PSAM_INT0_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int0_mstat_adq1_full_mstat0,adq1_full_mstat0,HI_PSAM_INT0_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int0_mstat_lbdq_full_mstat0,lbdq_full_mstat0,HI_PSAM_INT0_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int0_mstat_reserved_0,reserved_0,HI_PSAM_INT0_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int1_mstat_adq0_epty_mstat1,adq0_epty_mstat1,HI_PSAM_INT1_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int1_mstat_adq1_epty_mstat1,adq1_epty_mstat1,HI_PSAM_INT1_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int1_mstat_lbdq_epty_mstat1,lbdq_epty_mstat1,HI_PSAM_INT1_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int1_mstat_reserved_2,reserved_2,HI_PSAM_INT1_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int1_mstat_rd_wbuf_overflow_mstat1,rd_wbuf_overflow_mstat1,HI_PSAM_INT1_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int1_mstat_adq0_upoverflow_mstat1,adq0_upoverflow_mstat1,HI_PSAM_INT1_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int1_mstat_adq1_upoverflow_mstat1,adq1_upoverflow_mstat1,HI_PSAM_INT1_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int1_mstat_lbdq_upoverflow_mstat1,lbdq_upoverflow_mstat1,HI_PSAM_INT1_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int1_mstat_reserved_1,reserved_1,HI_PSAM_INT1_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int1_mstat_rd_wbuf_full_mstat1,rd_wbuf_full_mstat1,HI_PSAM_INT1_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int1_mstat_adq0_full_mstat1,adq0_full_mstat1,HI_PSAM_INT1_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int1_mstat_adq1_full_mstat1,adq1_full_mstat1,HI_PSAM_INT1_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int1_mstat_lbdq_full_mstat1,lbdq_full_mstat1,HI_PSAM_INT1_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int1_mstat_reserved_0,reserved_0,HI_PSAM_INT1_MSTAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MSTAT_OFFSET)
HI_SET_GET(hi_psam_int0_mask_adq0_epty_mask0,adq0_epty_mask0,HI_PSAM_INT0_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MASK_OFFSET)
HI_SET_GET(hi_psam_int0_mask_adq1_epty_mask0,adq1_epty_mask0,HI_PSAM_INT0_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MASK_OFFSET)
HI_SET_GET(hi_psam_int0_mask_lbdq_epty_mask0,lbdq_epty_mask0,HI_PSAM_INT0_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MASK_OFFSET)
HI_SET_GET(hi_psam_int0_mask_reserved_2,reserved_2,HI_PSAM_INT0_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MASK_OFFSET)
HI_SET_GET(hi_psam_int0_mask_rd_wbuf_overflow_mask0,rd_wbuf_overflow_mask0,HI_PSAM_INT0_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MASK_OFFSET)
HI_SET_GET(hi_psam_int0_mask_adq0_upoverflow_mask0,adq0_upoverflow_mask0,HI_PSAM_INT0_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MASK_OFFSET)
HI_SET_GET(hi_psam_int0_mask_adq1_upoverflow_mask0,adq1_upoverflow_mask0,HI_PSAM_INT0_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MASK_OFFSET)
HI_SET_GET(hi_psam_int0_mask_lbdq_upoverflow_mask0,lbdq_upoverflow_mask0,HI_PSAM_INT0_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MASK_OFFSET)
HI_SET_GET(hi_psam_int0_mask_reserved_1,reserved_1,HI_PSAM_INT0_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MASK_OFFSET)
HI_SET_GET(hi_psam_int0_mask_rd_wbuf_full_mask0,rd_wbuf_full_mask0,HI_PSAM_INT0_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MASK_OFFSET)
HI_SET_GET(hi_psam_int0_mask_adq0_full_mask0,adq0_full_mask0,HI_PSAM_INT0_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MASK_OFFSET)
HI_SET_GET(hi_psam_int0_mask_adq1_full_mask0,adq1_full_mask0,HI_PSAM_INT0_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MASK_OFFSET)
HI_SET_GET(hi_psam_int0_mask_lbdq_full_mask0,lbdq_full_mask0,HI_PSAM_INT0_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MASK_OFFSET)
HI_SET_GET(hi_psam_int0_mask_reserved_0,reserved_0,HI_PSAM_INT0_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT0_MASK_OFFSET)
HI_SET_GET(hi_psam_int1_mask_adq0_epty_mask1,adq0_epty_mask1,HI_PSAM_INT1_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MASK_OFFSET)
HI_SET_GET(hi_psam_int1_mask_adq1_epty_mask1,adq1_epty_mask1,HI_PSAM_INT1_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MASK_OFFSET)
HI_SET_GET(hi_psam_int1_mask_lbdq_epty_mask1,lbdq_epty_mask1,HI_PSAM_INT1_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MASK_OFFSET)
HI_SET_GET(hi_psam_int1_mask_reserved_2,reserved_2,HI_PSAM_INT1_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MASK_OFFSET)
HI_SET_GET(hi_psam_int1_mask_rd_wbuf_overflow_mask1,rd_wbuf_overflow_mask1,HI_PSAM_INT1_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MASK_OFFSET)
HI_SET_GET(hi_psam_int1_mask_adq0_upoverflow_mask1,adq0_upoverflow_mask1,HI_PSAM_INT1_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MASK_OFFSET)
HI_SET_GET(hi_psam_int1_mask_adq1_upoverflow_mask1,adq1_upoverflow_mask1,HI_PSAM_INT1_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MASK_OFFSET)
HI_SET_GET(hi_psam_int1_mask_lbdq_upoverflow_mask1,lbdq_upoverflow_mask1,HI_PSAM_INT1_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MASK_OFFSET)
HI_SET_GET(hi_psam_int1_mask_reserved_1,reserved_1,HI_PSAM_INT1_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MASK_OFFSET)
HI_SET_GET(hi_psam_int1_mask_rd_wbuf_full_mask1,rd_wbuf_full_mask1,HI_PSAM_INT1_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MASK_OFFSET)
HI_SET_GET(hi_psam_int1_mask_adq0_full_mask1,adq0_full_mask1,HI_PSAM_INT1_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MASK_OFFSET)
HI_SET_GET(hi_psam_int1_mask_adq1_full_mask1,adq1_full_mask1,HI_PSAM_INT1_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MASK_OFFSET)
HI_SET_GET(hi_psam_int1_mask_lbdq_full_mask1,lbdq_full_mask1,HI_PSAM_INT1_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MASK_OFFSET)
HI_SET_GET(hi_psam_int1_mask_reserved_0,reserved_0,HI_PSAM_INT1_MASK_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_INT1_MASK_OFFSET)
HI_SET_GET(hi_psam_cipher_ch_softreset_cipher_ch_srst,cipher_ch_srst,HI_PSAM_CIPHER_CH_SOFTRESET_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CIPHER_CH_SOFTRESET_OFFSET)
HI_SET_GET(hi_psam_cipher_ch_softreset_reserved,reserved,HI_PSAM_CIPHER_CH_SOFTRESET_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CIPHER_CH_SOFTRESET_OFFSET)
HI_SET_GET(hi_psam_cipher_ch_en_cipher_ch_en,cipher_ch_en,HI_PSAM_CIPHER_CH_EN_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CIPHER_CH_EN_OFFSET)
HI_SET_GET(hi_psam_cipher_ch_en_reserved,reserved,HI_PSAM_CIPHER_CH_EN_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CIPHER_CH_EN_OFFSET)
HI_SET_GET(hi_psam_cipher_ch_en_cipher_ch_busy,cipher_ch_busy,HI_PSAM_CIPHER_CH_EN_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CIPHER_CH_EN_OFFSET)
HI_SET_GET(hi_psam_cbdq_config_cbd_iv_sel,cbd_iv_sel,HI_PSAM_CBDQ_CONFIG_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_CONFIG_OFFSET)
HI_SET_GET(hi_psam_cbdq_config_cbd_iv_num,cbd_iv_num,HI_PSAM_CBDQ_CONFIG_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_CONFIG_OFFSET)
HI_SET_GET(hi_psam_cbdq_config_reserved_3,reserved_3,HI_PSAM_CBDQ_CONFIG_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_CONFIG_OFFSET)
HI_SET_GET(hi_psam_cbdq_config_reserved_2,reserved_2,HI_PSAM_CBDQ_CONFIG_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_CONFIG_OFFSET)
HI_SET_GET(hi_psam_cbdq_config_reserved_1,reserved_1,HI_PSAM_CBDQ_CONFIG_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_CONFIG_OFFSET)
HI_SET_GET(hi_psam_cbdq_config_reserved_0,reserved_0,HI_PSAM_CBDQ_CONFIG_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_CONFIG_OFFSET)
HI_SET_GET(hi_psam_cbdq_baddr_reserved,reserved,HI_PSAM_CBDQ_BADDR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_BADDR_OFFSET)
HI_SET_GET(hi_psam_cbdq_baddr_cbdq_base_addr,cbdq_base_addr,HI_PSAM_CBDQ_BADDR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_BADDR_OFFSET)
HI_SET_GET(hi_psam_cbdq_size_cbdq_size,cbdq_size,HI_PSAM_CBDQ_SIZE_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_SIZE_OFFSET)
HI_SET_GET(hi_psam_cbdq_size_reserved,reserved,HI_PSAM_CBDQ_SIZE_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_SIZE_OFFSET)
HI_SET_GET(hi_psam_cbdq_wptr_cbdq_wptr,cbdq_wptr,HI_PSAM_CBDQ_WPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_WPTR_OFFSET)
HI_SET_GET(hi_psam_cbdq_wptr_reserved,reserved,HI_PSAM_CBDQ_WPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_WPTR_OFFSET)
HI_SET_GET(hi_psam_cbdq_stat_local_cbdq_wptr,local_cbdq_wptr,HI_PSAM_CBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_cbdq_stat_cbdq_wptr_invalid,cbdq_wptr_invalid,HI_PSAM_CBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_cbdq_stat_reserved_1,reserved_1,HI_PSAM_CBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_cbdq_stat_local_cbdq_rptr,local_cbdq_rptr,HI_PSAM_CBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_cbdq_stat_reserved_0,reserved_0,HI_PSAM_CBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_cbdq_wptr_addr_cbdq_wptr_addr,cbdq_wptr_addr,HI_PSAM_CBDQ_WPTR_ADDR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CBDQ_WPTR_ADDR_OFFSET)
HI_SET_GET(hi_psam_crdq_ctrl_fc_head,fc_head,HI_PSAM_CRDQ_CTRL_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_CTRL_OFFSET)
HI_SET_GET(hi_psam_crdq_ctrl_reserved,reserved,HI_PSAM_CRDQ_CTRL_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_CTRL_OFFSET)
HI_SET_GET(hi_psam_crdq_stat_crd_wbuf1_full,crd_wbuf1_full,HI_PSAM_CRDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_crdq_stat_crd_wbuf1_epty,crd_wbuf1_epty,HI_PSAM_CRDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_crdq_stat_crd_wbuf2_full,crd_wbuf2_full,HI_PSAM_CRDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_crdq_stat_crd_wbuf2_epty,crd_wbuf2_epty,HI_PSAM_CRDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_crdq_stat_crd_wbuf3_full,crd_wbuf3_full,HI_PSAM_CRDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_crdq_stat_crd_wbuf3_epty,crd_wbuf3_epty,HI_PSAM_CRDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_crdq_stat_crd_wbuf4_full,crd_wbuf4_full,HI_PSAM_CRDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_crdq_stat_crd_wbuf4_epty,crd_wbuf4_epty,HI_PSAM_CRDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_crdq_stat_crd_wbuf5_full,crd_wbuf5_full,HI_PSAM_CRDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_crdq_stat_crd_wbuf5_epty,crd_wbuf5_epty,HI_PSAM_CRDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_crdq_stat_reserved,reserved,HI_PSAM_CRDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_crdq_ptr_local_crdq_wptr,local_crdq_wptr,HI_PSAM_CRDQ_PTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_PTR_OFFSET)
HI_SET_GET(hi_psam_crdq_ptr_reserved_1,reserved_1,HI_PSAM_CRDQ_PTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_PTR_OFFSET)
HI_SET_GET(hi_psam_crdq_ptr_local_crdq_rptr,local_crdq_rptr,HI_PSAM_CRDQ_PTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_PTR_OFFSET)
HI_SET_GET(hi_psam_crdq_ptr_reserved_0,reserved_0,HI_PSAM_CRDQ_PTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_PTR_OFFSET)
HI_SET_GET(hi_psam_crdq_rptr_addr_crdq_rptr_addr,crdq_rptr_addr,HI_PSAM_CRDQ_RPTR_ADDR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_RPTR_ADDR_OFFSET)
HI_SET_GET(hi_psam_ibdq_stat_last_lbd_ibdq_wptr,last_lbd_ibdq_wptr,HI_PSAM_IBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_ibdq_stat_last_lbd_lbdq_rptr,last_lbd_lbdq_rptr,HI_PSAM_IBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_ibdq_stat_need_to_update,need_to_update,HI_PSAM_IBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_ibdq_stat_reserved,reserved,HI_PSAM_IBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_ibdq_baddr_reserved,reserved,HI_PSAM_IBDQ_BADDR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_BADDR_OFFSET)
HI_SET_GET(hi_psam_ibdq_baddr_ibdq_addr,ibdq_addr,HI_PSAM_IBDQ_BADDR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_BADDR_OFFSET)
HI_SET_GET(hi_psam_ibdq_size_ibdq_size,ibdq_size,HI_PSAM_IBDQ_SIZE_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_SIZE_OFFSET)
HI_SET_GET(hi_psam_ibdq_size_reserved,reserved,HI_PSAM_IBDQ_SIZE_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_SIZE_OFFSET)
HI_SET_GET(hi_psam_ibdq_wptr_ibdq_wptr,ibdq_wptr,HI_PSAM_IBDQ_WPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_WPTR_OFFSET)
HI_SET_GET(hi_psam_ibdq_wptr_reserved,reserved,HI_PSAM_IBDQ_WPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_WPTR_OFFSET)
HI_SET_GET(hi_psam_ibdq_rptr_ibdq_rptr,ibdq_rptr,HI_PSAM_IBDQ_RPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_RPTR_OFFSET)
HI_SET_GET(hi_psam_ibdq_rptr_reserved,reserved,HI_PSAM_IBDQ_RPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_RPTR_OFFSET)
HI_SET_GET(hi_psam_ibdq_wptr_addr_ibdq_wptr_addr,ibdq_wptr_addr,HI_PSAM_IBDQ_WPTR_ADDR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_WPTR_ADDR_OFFSET)
HI_SET_GET(hi_psam_ibdq_rptr_addr_ibdq_rptr_addr,ibdq_rptr_addr,HI_PSAM_IBDQ_RPTR_ADDR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_RPTR_ADDR_OFFSET)
HI_SET_GET(hi_psam_ibdq_rptr_timer_ibdq_rptr_timer,ibdq_rptr_timer,HI_PSAM_IBDQ_RPTR_TIMER_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_RPTR_TIMER_OFFSET)
HI_SET_GET(hi_psam_ibdq_rptr_timer_reserved,reserved,HI_PSAM_IBDQ_RPTR_TIMER_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_RPTR_TIMER_OFFSET)
HI_SET_GET(hi_psam_ibdq_pkt_cnt_ibdq_pkt_cnt,ibdq_pkt_cnt,HI_PSAM_IBDQ_PKT_CNT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_IBDQ_PKT_CNT_OFFSET)
HI_SET_GET(hi_psam_lbdq_stat_lbdq_epty,lbdq_epty,HI_PSAM_LBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_lbdq_stat_lbdq_full,lbdq_full,HI_PSAM_LBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_lbdq_stat_lbd_rbuf_epty,lbd_rbuf_epty,HI_PSAM_LBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_lbdq_stat_lbd_rbuf_full,lbd_rbuf_full,HI_PSAM_LBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_lbdq_stat_reserved_1,reserved_1,HI_PSAM_LBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_lbdq_stat_lbdq_wptr_invalid,lbdq_wptr_invalid,HI_PSAM_LBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_lbdq_stat_reserved_0,reserved_0,HI_PSAM_LBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_lbdq_stat_local_lbdq_rptr,local_lbdq_rptr,HI_PSAM_LBDQ_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_STAT_OFFSET)
HI_SET_GET(hi_psam_lbdq_baddr_reserved,reserved,HI_PSAM_LBDQ_BADDR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_BADDR_OFFSET)
HI_SET_GET(hi_psam_lbdq_baddr_lbdq_addr,lbdq_addr,HI_PSAM_LBDQ_BADDR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_BADDR_OFFSET)
HI_SET_GET(hi_psam_lbdq_size_lbdq_size,lbdq_size,HI_PSAM_LBDQ_SIZE_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_SIZE_OFFSET)
HI_SET_GET(hi_psam_lbdq_size_reserved,reserved,HI_PSAM_LBDQ_SIZE_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_SIZE_OFFSET)
HI_SET_GET(hi_psam_lbdq_wptr_lbdq_wptr,lbdq_wptr,HI_PSAM_LBDQ_WPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_WPTR_OFFSET)
HI_SET_GET(hi_psam_lbdq_wptr_reserved,reserved,HI_PSAM_LBDQ_WPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_WPTR_OFFSET)
HI_SET_GET(hi_psam_lbdq_rptr_real_lbdq_rptr,real_lbdq_rptr,HI_PSAM_LBDQ_RPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_RPTR_OFFSET)
HI_SET_GET(hi_psam_lbdq_rptr_reserved,reserved,HI_PSAM_LBDQ_RPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_RPTR_OFFSET)
HI_SET_GET(hi_psam_lbdq_depth_lbdq_depth,lbdq_depth,HI_PSAM_LBDQ_DEPTH_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_DEPTH_OFFSET)
HI_SET_GET(hi_psam_lbdq_depth_reserved,reserved,HI_PSAM_LBDQ_DEPTH_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_LBDQ_DEPTH_OFFSET)
HI_SET_GET(hi_psam_adq_ctrl_reserved_1,reserved_1,HI_PSAM_ADQ_CTRL_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ_CTRL_OFFSET)
HI_SET_GET(hi_psam_adq_ctrl_adq_en,adq_en,HI_PSAM_ADQ_CTRL_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ_CTRL_OFFSET)
HI_SET_GET(hi_psam_adq_ctrl_adq0_size_sel,adq0_size_sel,HI_PSAM_ADQ_CTRL_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ_CTRL_OFFSET)
HI_SET_GET(hi_psam_adq_ctrl_adq1_size_sel,adq1_size_sel,HI_PSAM_ADQ_CTRL_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ_CTRL_OFFSET)
HI_SET_GET(hi_psam_adq_ctrl_reserved_0,reserved_0,HI_PSAM_ADQ_CTRL_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ_CTRL_OFFSET)
HI_SET_GET(hi_psam_adq_ctrl_adq_plen_th,adq_plen_th,HI_PSAM_ADQ_CTRL_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ_CTRL_OFFSET)
HI_SET_GET(hi_psam_adq0_base_reserved,reserved,HI_PSAM_ADQ0_BASE_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ0_BASE_OFFSET)
HI_SET_GET(hi_psam_adq0_base_adq0_base,adq0_base,HI_PSAM_ADQ0_BASE_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ0_BASE_OFFSET)
HI_SET_GET(hi_psam_adq0_stat_adq0_epty,adq0_epty,HI_PSAM_ADQ0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ0_STAT_OFFSET)
HI_SET_GET(hi_psam_adq0_stat_adq0_full,adq0_full,HI_PSAM_ADQ0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ0_STAT_OFFSET)
HI_SET_GET(hi_psam_adq0_stat_ad0_buf_epty,ad0_buf_epty,HI_PSAM_ADQ0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ0_STAT_OFFSET)
HI_SET_GET(hi_psam_adq0_stat_ad0_buf_full,ad0_buf_full,HI_PSAM_ADQ0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ0_STAT_OFFSET)
HI_SET_GET(hi_psam_adq0_stat_adq0_rptr_invalid,adq0_rptr_invalid,HI_PSAM_ADQ0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ0_STAT_OFFSET)
HI_SET_GET(hi_psam_adq0_stat_adq0_wptr_invalid,adq0_wptr_invalid,HI_PSAM_ADQ0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ0_STAT_OFFSET)
HI_SET_GET(hi_psam_adq0_stat_reserved,reserved,HI_PSAM_ADQ0_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ0_STAT_OFFSET)
HI_SET_GET(hi_psam_adq0_wptr_adq0_wptr,adq0_wptr,HI_PSAM_ADQ0_WPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ0_WPTR_OFFSET)
HI_SET_GET(hi_psam_adq0_wptr_reserved,reserved,HI_PSAM_ADQ0_WPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ0_WPTR_OFFSET)
HI_SET_GET(hi_psam_adq0_rptr_adq0_rptr,adq0_rptr,HI_PSAM_ADQ0_RPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ0_RPTR_OFFSET)
HI_SET_GET(hi_psam_adq0_rptr_reserved,reserved,HI_PSAM_ADQ0_RPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ0_RPTR_OFFSET)
HI_SET_GET(hi_psam_adq1_base_reserved,reserved,HI_PSAM_ADQ1_BASE_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ1_BASE_OFFSET)
HI_SET_GET(hi_psam_adq1_base_adq1_base,adq1_base,HI_PSAM_ADQ1_BASE_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ1_BASE_OFFSET)
HI_SET_GET(hi_psam_adq1_stat_adq1_epty,adq1_epty,HI_PSAM_ADQ1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ1_STAT_OFFSET)
HI_SET_GET(hi_psam_adq1_stat_adq1_full,adq1_full,HI_PSAM_ADQ1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ1_STAT_OFFSET)
HI_SET_GET(hi_psam_adq1_stat_ad1_buf_epty,ad1_buf_epty,HI_PSAM_ADQ1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ1_STAT_OFFSET)
HI_SET_GET(hi_psam_adq1_stat_ad1_buf_full,ad1_buf_full,HI_PSAM_ADQ1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ1_STAT_OFFSET)
HI_SET_GET(hi_psam_adq1_stat_adq1_rptr_invalid,adq1_rptr_invalid,HI_PSAM_ADQ1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ1_STAT_OFFSET)
HI_SET_GET(hi_psam_adq1_stat_adq1_wptr_invalid,adq1_wptr_invalid,HI_PSAM_ADQ1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ1_STAT_OFFSET)
HI_SET_GET(hi_psam_adq1_stat_reserved,reserved,HI_PSAM_ADQ1_STAT_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ1_STAT_OFFSET)
HI_SET_GET(hi_psam_adq1_wptr_adq1_wptr,adq1_wptr,HI_PSAM_ADQ1_WPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ1_WPTR_OFFSET)
HI_SET_GET(hi_psam_adq1_wptr_reserved,reserved,HI_PSAM_ADQ1_WPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ1_WPTR_OFFSET)
HI_SET_GET(hi_psam_adq1_rptr_adq1_rptr,adq1_rptr,HI_PSAM_ADQ1_RPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ1_RPTR_OFFSET)
HI_SET_GET(hi_psam_adq1_rptr_reserved,reserved,HI_PSAM_ADQ1_RPTR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_ADQ1_RPTR_OFFSET)
HI_SET_GET(hi_psam_crdq_baddr_psam_crdq_baddr,psam_crdq_baddr,HI_PSAM_CRDQ_BADDR_T,HI_REG_DESIGN_BASE_ADDR, HI_PSAM_CRDQ_BADDR_OFFSET)
#endif
#endif
#endif
