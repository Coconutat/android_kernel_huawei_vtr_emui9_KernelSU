#ifndef __SOC_SYSCOUNTER_INTERFACE_H__
#define __SOC_SYSCOUNTER_INTERFACE_H__ 
#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif
#define SOC_SYSCOUNTER_CNTCR_ADDR(base) ((base) + (0x0000))
#define SOC_SYSCOUNTER_CNTSR_ADDR(base) ((base) + (0x0004))
#define SOC_SYSCOUNTER_CNTCV_L32_ADDR(base) ((base) + (0x0008))
#define SOC_SYSCOUNTER_CNTCV_H32_ADDR(base) ((base) + (0x000C))
#define SOC_SYSCOUNTER_CNTFID0_ADDR(base) ((base) + (0x0020))
#define SOC_SYSCOUNTER_CNTFID1_ADDR(base) ((base) + (0x0024))
#define SOC_SYSCOUNTER_CNTFID2_ADDR(base) ((base) + (0x0028))
#define SOC_SYSCOUNTER_CNTCV_L32_NS_ADDR(base) ((base) + (0x1008))
#define SOC_SYSCOUNTER_CNTCV_H32_NS_ADDR(base) ((base) + (0x100C))
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int en : 1;
        unsigned int hdbg : 1;
        unsigned int reserved_0: 6;
        unsigned int fcreq : 3;
        unsigned int reserved_1: 21;
    } reg;
} SOC_SYSCOUNTER_CNTCR_UNION;
#endif
#define SOC_SYSCOUNTER_CNTCR_en_START (0)
#define SOC_SYSCOUNTER_CNTCR_en_END (0)
#define SOC_SYSCOUNTER_CNTCR_hdbg_START (1)
#define SOC_SYSCOUNTER_CNTCR_hdbg_END (1)
#define SOC_SYSCOUNTER_CNTCR_fcreq_START (8)
#define SOC_SYSCOUNTER_CNTCR_fcreq_END (10)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved_0: 1;
        unsigned int dbgh : 1;
        unsigned int reserved_1: 6;
        unsigned int fcack : 3;
        unsigned int reserved_2: 21;
    } reg;
} SOC_SYSCOUNTER_CNTSR_UNION;
#endif
#define SOC_SYSCOUNTER_CNTSR_dbgh_START (1)
#define SOC_SYSCOUNTER_CNTSR_dbgh_END (1)
#define SOC_SYSCOUNTER_CNTSR_fcack_START (8)
#define SOC_SYSCOUNTER_CNTSR_fcack_END (10)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int cntcv_l32 : 32;
    } reg;
} SOC_SYSCOUNTER_CNTCV_L32_UNION;
#endif
#define SOC_SYSCOUNTER_CNTCV_L32_cntcv_l32_START (0)
#define SOC_SYSCOUNTER_CNTCV_L32_cntcv_l32_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int cntcv_h32 : 32;
    } reg;
} SOC_SYSCOUNTER_CNTCV_H32_UNION;
#endif
#define SOC_SYSCOUNTER_CNTCV_H32_cntcv_h32_START (0)
#define SOC_SYSCOUNTER_CNTCV_H32_cntcv_h32_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int cntfid0 : 32;
    } reg;
} SOC_SYSCOUNTER_CNTFID0_UNION;
#endif
#define SOC_SYSCOUNTER_CNTFID0_cntfid0_START (0)
#define SOC_SYSCOUNTER_CNTFID0_cntfid0_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int cntfid1 : 32;
    } reg;
} SOC_SYSCOUNTER_CNTFID1_UNION;
#endif
#define SOC_SYSCOUNTER_CNTFID1_cntfid1_START (0)
#define SOC_SYSCOUNTER_CNTFID1_cntfid1_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int cntfid2 : 32;
    } reg;
} SOC_SYSCOUNTER_CNTFID2_UNION;
#endif
#define SOC_SYSCOUNTER_CNTFID2_cntfid2_START (0)
#define SOC_SYSCOUNTER_CNTFID2_cntfid2_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int cntcv_l32_ns : 32;
    } reg;
} SOC_SYSCOUNTER_CNTCV_L32_NS_UNION;
#endif
#define SOC_SYSCOUNTER_CNTCV_L32_NS_cntcv_l32_ns_START (0)
#define SOC_SYSCOUNTER_CNTCV_L32_NS_cntcv_l32_ns_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int cntcv_h32_ns : 32;
    } reg;
} SOC_SYSCOUNTER_CNTCV_H32_NS_UNION;
#endif
#define SOC_SYSCOUNTER_CNTCV_H32_NS_cntcv_h32_ns_START (0)
#define SOC_SYSCOUNTER_CNTCV_H32_NS_cntcv_h32_ns_END (31)
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
#endif
