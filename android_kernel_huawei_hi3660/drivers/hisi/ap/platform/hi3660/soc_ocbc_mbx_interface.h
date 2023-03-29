#ifndef __SOC_OCBC_MBX_INTERFACE_H__
#define __SOC_OCBC_MBX_INTERFACE_H__ 
#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif
#define SOC_OCBC_MBX_PE_OCB_MBOX_ADDR(base) ((base) + (0x000))
#define SOC_OCBC_MBX_PE_OCB_MBOX_OVF_ADDR(base) ((base) + (0x004))
#define SOC_OCBC_MBX_PE_OCB_MBOX_RDY_ADDR(base) ((base) + (0x008))
#define SOC_OCBC_MBX_OCB_PE_MBOX_ADDR(base) ((base) + (0x020))
#define SOC_OCBC_MBX_OCB_PE_MBOX_OVF_ADDR(base) ((base) + (0x024))
#define SOC_OCBC_MBX_OCB_PE_MBOX_RDY_ADDR(base) ((base) + (0x028))
#define SOC_OCBC_MBX_OCB_GPR_REG_00_ADDR(base) ((base) + (0x040))
#define SOC_OCBC_MBX_TIMER_CTRL_ADDR(base) ((base) + (0x060))
#define SOC_OCBC_MBX_TIMEOUT_VAL_ADDR(base) ((base) + (0x064))
#define SOC_OCBC_MBX_TIMER_CNT_ADDR(base) ((base) + (0x068))
#define SOC_OCBC_MBX_TIMER_ISR_ADDR(base) ((base) + (0x06C))
#define SOC_OCBC_MBX_PE_OCB_CTRL_SET_ADDR(base,N) ((base) + (0x100+0x10*(N)))
#define SOC_OCBC_MBX_PE_OCB_CTRL_CLR_ADDR(base,N) ((base) + (0x104+0x10*(N)))
#define SOC_OCBC_MBX_OCB_PE_CTRL_SET_ADDR(base,N) ((base) + (0x108+0x10*(N)))
#define SOC_OCBC_MBX_OCB_PE_CTRL_CLR_ADDR(base,N) ((base) + (0x10C+0x10*(N)))
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int pe_ocb_mlbox : 32;
    } reg;
} SOC_OCBC_MBX_PE_OCB_MBOX_UNION;
#endif
#define SOC_OCBC_MBX_PE_OCB_MBOX_pe_ocb_mlbox_START (0)
#define SOC_OCBC_MBX_PE_OCB_MBOX_pe_ocb_mlbox_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int pe_ocb_mbx_ovf : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_OCBC_MBX_PE_OCB_MBOX_OVF_UNION;
#endif
#define SOC_OCBC_MBX_PE_OCB_MBOX_OVF_pe_ocb_mbx_ovf_START (0)
#define SOC_OCBC_MBX_PE_OCB_MBOX_OVF_pe_ocb_mbx_ovf_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int pe_ocb_mbx_rdy : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_OCBC_MBX_PE_OCB_MBOX_RDY_UNION;
#endif
#define SOC_OCBC_MBX_PE_OCB_MBOX_RDY_pe_ocb_mbx_rdy_START (0)
#define SOC_OCBC_MBX_PE_OCB_MBOX_RDY_pe_ocb_mbx_rdy_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ocb_pe_mlbox : 32;
    } reg;
} SOC_OCBC_MBX_OCB_PE_MBOX_UNION;
#endif
#define SOC_OCBC_MBX_OCB_PE_MBOX_ocb_pe_mlbox_START (0)
#define SOC_OCBC_MBX_OCB_PE_MBOX_ocb_pe_mlbox_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ocb_pe_mbx_ovf : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_OCBC_MBX_OCB_PE_MBOX_OVF_UNION;
#endif
#define SOC_OCBC_MBX_OCB_PE_MBOX_OVF_ocb_pe_mbx_ovf_START (0)
#define SOC_OCBC_MBX_OCB_PE_MBOX_OVF_ocb_pe_mbx_ovf_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ocb_pe_mbx_rdy : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_OCBC_MBX_OCB_PE_MBOX_RDY_UNION;
#endif
#define SOC_OCBC_MBX_OCB_PE_MBOX_RDY_ocb_pe_mbx_rdy_START (0)
#define SOC_OCBC_MBX_OCB_PE_MBOX_RDY_ocb_pe_mbx_rdy_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ocb_gpr_reg_00 : 32;
    } reg;
} SOC_OCBC_MBX_OCB_GPR_REG_00_UNION;
#endif
#define SOC_OCBC_MBX_OCB_GPR_REG_00_ocb_gpr_reg_00_START (0)
#define SOC_OCBC_MBX_OCB_GPR_REG_00_ocb_gpr_reg_00_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int timer_en : 1;
        unsigned int timer_rst : 1;
        unsigned int reserved_0 : 2;
        unsigned int timeout_sel : 4;
        unsigned int reserved_1 : 24;
    } reg;
} SOC_OCBC_MBX_TIMER_CTRL_UNION;
#endif
#define SOC_OCBC_MBX_TIMER_CTRL_timer_en_START (0)
#define SOC_OCBC_MBX_TIMER_CTRL_timer_en_END (0)
#define SOC_OCBC_MBX_TIMER_CTRL_timer_rst_START (1)
#define SOC_OCBC_MBX_TIMER_CTRL_timer_rst_END (1)
#define SOC_OCBC_MBX_TIMER_CTRL_timeout_sel_START (4)
#define SOC_OCBC_MBX_TIMER_CTRL_timeout_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int timeout_val : 12;
        unsigned int reserved : 20;
    } reg;
} SOC_OCBC_MBX_TIMEOUT_VAL_UNION;
#endif
#define SOC_OCBC_MBX_TIMEOUT_VAL_timeout_val_START (0)
#define SOC_OCBC_MBX_TIMEOUT_VAL_timeout_val_END (11)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int timer_cnt : 12;
        unsigned int reserved : 20;
    } reg;
} SOC_OCBC_MBX_TIMER_CNT_UNION;
#endif
#define SOC_OCBC_MBX_TIMER_CNT_timer_cnt_START (0)
#define SOC_OCBC_MBX_TIMER_CNT_timer_cnt_END (11)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int timeout_intr : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_OCBC_MBX_TIMER_ISR_UNION;
#endif
#define SOC_OCBC_MBX_TIMER_ISR_timeout_intr_START (0)
#define SOC_OCBC_MBX_TIMER_ISR_timeout_intr_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int request : 1;
        unsigned int release : 1;
        unsigned int grant : 1;
        unsigned int preempted : 1;
        unsigned int reserved_0: 2;
        unsigned int init : 1;
        unsigned int deinit : 1;
        unsigned int reserved_1: 24;
    } reg;
} SOC_OCBC_MBX_PE_OCB_CTRL_SET_UNION;
#endif
#define SOC_OCBC_MBX_PE_OCB_CTRL_SET_request_START (0)
#define SOC_OCBC_MBX_PE_OCB_CTRL_SET_request_END (0)
#define SOC_OCBC_MBX_PE_OCB_CTRL_SET_release_START (1)
#define SOC_OCBC_MBX_PE_OCB_CTRL_SET_release_END (1)
#define SOC_OCBC_MBX_PE_OCB_CTRL_SET_grant_START (2)
#define SOC_OCBC_MBX_PE_OCB_CTRL_SET_grant_END (2)
#define SOC_OCBC_MBX_PE_OCB_CTRL_SET_preempted_START (3)
#define SOC_OCBC_MBX_PE_OCB_CTRL_SET_preempted_END (3)
#define SOC_OCBC_MBX_PE_OCB_CTRL_SET_init_START (6)
#define SOC_OCBC_MBX_PE_OCB_CTRL_SET_init_END (6)
#define SOC_OCBC_MBX_PE_OCB_CTRL_SET_deinit_START (7)
#define SOC_OCBC_MBX_PE_OCB_CTRL_SET_deinit_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int request : 1;
        unsigned int release : 1;
        unsigned int grant : 1;
        unsigned int preempted : 1;
        unsigned int reserved_0: 2;
        unsigned int init : 1;
        unsigned int deinit : 1;
        unsigned int reserved_1: 24;
    } reg;
} SOC_OCBC_MBX_PE_OCB_CTRL_CLR_UNION;
#endif
#define SOC_OCBC_MBX_PE_OCB_CTRL_CLR_request_START (0)
#define SOC_OCBC_MBX_PE_OCB_CTRL_CLR_request_END (0)
#define SOC_OCBC_MBX_PE_OCB_CTRL_CLR_release_START (1)
#define SOC_OCBC_MBX_PE_OCB_CTRL_CLR_release_END (1)
#define SOC_OCBC_MBX_PE_OCB_CTRL_CLR_grant_START (2)
#define SOC_OCBC_MBX_PE_OCB_CTRL_CLR_grant_END (2)
#define SOC_OCBC_MBX_PE_OCB_CTRL_CLR_preempted_START (3)
#define SOC_OCBC_MBX_PE_OCB_CTRL_CLR_preempted_END (3)
#define SOC_OCBC_MBX_PE_OCB_CTRL_CLR_init_START (6)
#define SOC_OCBC_MBX_PE_OCB_CTRL_CLR_init_END (6)
#define SOC_OCBC_MBX_PE_OCB_CTRL_CLR_deinit_START (7)
#define SOC_OCBC_MBX_PE_OCB_CTRL_CLR_deinit_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int request : 1;
        unsigned int release : 1;
        unsigned int grant : 1;
        unsigned int preempted : 1;
        unsigned int reserved_0: 2;
        unsigned int init : 1;
        unsigned int deinit : 1;
        unsigned int reserved_1: 24;
    } reg;
} SOC_OCBC_MBX_OCB_PE_CTRL_SET_UNION;
#endif
#define SOC_OCBC_MBX_OCB_PE_CTRL_SET_request_START (0)
#define SOC_OCBC_MBX_OCB_PE_CTRL_SET_request_END (0)
#define SOC_OCBC_MBX_OCB_PE_CTRL_SET_release_START (1)
#define SOC_OCBC_MBX_OCB_PE_CTRL_SET_release_END (1)
#define SOC_OCBC_MBX_OCB_PE_CTRL_SET_grant_START (2)
#define SOC_OCBC_MBX_OCB_PE_CTRL_SET_grant_END (2)
#define SOC_OCBC_MBX_OCB_PE_CTRL_SET_preempted_START (3)
#define SOC_OCBC_MBX_OCB_PE_CTRL_SET_preempted_END (3)
#define SOC_OCBC_MBX_OCB_PE_CTRL_SET_init_START (6)
#define SOC_OCBC_MBX_OCB_PE_CTRL_SET_init_END (6)
#define SOC_OCBC_MBX_OCB_PE_CTRL_SET_deinit_START (7)
#define SOC_OCBC_MBX_OCB_PE_CTRL_SET_deinit_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int request : 1;
        unsigned int release : 1;
        unsigned int grant : 1;
        unsigned int preempted : 1;
        unsigned int reserved_0: 2;
        unsigned int init : 1;
        unsigned int deinit : 1;
        unsigned int reserved_1: 24;
    } reg;
} SOC_OCBC_MBX_OCB_PE_CTRL_CLR_UNION;
#endif
#define SOC_OCBC_MBX_OCB_PE_CTRL_CLR_request_START (0)
#define SOC_OCBC_MBX_OCB_PE_CTRL_CLR_request_END (0)
#define SOC_OCBC_MBX_OCB_PE_CTRL_CLR_release_START (1)
#define SOC_OCBC_MBX_OCB_PE_CTRL_CLR_release_END (1)
#define SOC_OCBC_MBX_OCB_PE_CTRL_CLR_grant_START (2)
#define SOC_OCBC_MBX_OCB_PE_CTRL_CLR_grant_END (2)
#define SOC_OCBC_MBX_OCB_PE_CTRL_CLR_preempted_START (3)
#define SOC_OCBC_MBX_OCB_PE_CTRL_CLR_preempted_END (3)
#define SOC_OCBC_MBX_OCB_PE_CTRL_CLR_init_START (6)
#define SOC_OCBC_MBX_OCB_PE_CTRL_CLR_init_END (6)
#define SOC_OCBC_MBX_OCB_PE_CTRL_CLR_deinit_START (7)
#define SOC_OCBC_MBX_OCB_PE_CTRL_CLR_deinit_END (7)
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
#endif
