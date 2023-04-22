

#ifndef __OAL_PCIE_PM_H__
#define __OAL_PCIE_PM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include "oal_util.h"
#include "oal_pci_if.h"

typedef enum {
    PCIE_ASPM_CAP_L0S,
    PCIE_ASPM_CAP_L1,
    PCIE_ASPM_CAP_L0S_AND_L1,

    PCIE_ASPM_CAP_BUTT
}oal_pcie_aspm;
typedef oal_uint8 oal_pcie_aspm_enum;

#define  PCI_EXT_CAP_ID_L1SS    0x1E	/* PCIE L1SS CAP */
#define  PCI_L1SS_CAP		0x4	        /* LSSS CAP Regist */
#define  PCI_L1SS_CTRL1		0x8	        /* LSSS CONTROL 1 Regist */
#define   PCI_L1SS_L12_PCIPM_EN  (1<<0)
#define   PCI_L1SS_L11_PCIPM_EN  (1<<1)
#define   PCI_L1SS_L12_ASPM_EN   (1<<2)
#define   PCI_L1SS_L11_ASPM_EN   (1<<3)

#define  PCI_L1SS_CTRL2		0xC	        /* LSSS CONTROL 2 Regist */

#define OAL_PCIE_RC_SYS_BASE        (0x10100000)          /* SYSCTRL */
#define OAL_DBI_RC_BASE_ADDR_0      (0x10A00000)        /* IO空间 64k */
#define OAL_DBI_RC_BASE_ADDR_1      (0x10A02000)        /* IO空间 64k */
#define OAL_PERI_W_PCIE0	     (0xBC)
#define OAL_PERI_R_PCIE0         (0xC0)
#define OAL_PERI_W_PCIE1         (0xEC)
#define OAL_PERI_R_PCIE1         (0xF0)
#define OAL_PCIE_LINK_STATUS_OFFSET   (0x18)
#define OAL_PCIE_CAP_POS               (0x70)

typedef enum {
    PCIE_L0S_ENT_1_US=0x0,
    PCIE_L0S_ENT_2_US,
    PCIE_L0S_ENT_3_US,
    PCIE_L0S_ENT_4_US,
    PCIE_L0S_ENT_5_US,
    PCIE_L0S_ENT_6_US,
    PCIE_L0S_ENT_7_US,

    PCIE_L0S_ENT_BUT
}oal_pcie_L0s_ent_latency;
typedef oal_uint8 oal_pcie_L0s_ent_enum;

typedef enum {
    PCIE_L1_ENT_1_US=0x0,
    PCIE_L1_ENT_2_US,
    PCIE_L1_ENT_4_US,
    PCIE_L1_ENT_8_US,
    PCIE_L1_ENT_16_US,
    PCIE_L1_ENT_32_US,
    PCIE_L1_ENT_64_US,

    PCIE_L1_ENT_BUT
}oal_pcie_L1_ent_latency;
typedef oal_uint8 oal_pcie_L1_ent_enum;

typedef enum {
    PCIE_COMP_TIMEOUT_50U_TO_50M  =0x0,
    PCIE_COMP_TIMEOUT_50U_TO_100U =0x1,
    PCIE_COMP_TIMEOUT_1M_TO_10M   =0x2,

    PCIE_COMPLETE_TIMEOUT_BUT
}oal_pcie_comp_timeout;
typedef oal_uint8 oal_pcie_comp_timeout_enum;


#define COMMON_CLK_NFTS_BIT_OFFSET           (8)
#define ACK_NFTS_BIT_OFFSET                  (16)

#define PCIE_PORT_LOGIC_BASE         (0x700)
#define PCIE_PL_ASPM_CTRL_OFFSET     (PCIE_PORT_LOGIC_BASE+0xc)


/* 将var中[pos, pos + bits-1]比特设置为val,  pos从0开始编号
   e.g BDOM_SET_BITS(var, 4, 2, 2) 表示将Bit5~4设置为b'10 */
#define REG32_SETBITS(reg, pos, bits, val)do {\
    (reg) = ((oal_uint32)(reg) & (~((((oal_uint32)1 << (bits)) - 1) << (pos)))) \
                            | ((oal_uint32)((val) & (((oal_uint32)1 << (bits)) - 1)) << (pos));\
 }while(0)
#define REG16_SETBITS(reg, pos, bits, val)do {\
    (reg) = ((oal_uint16)(reg) & (~((((oal_uint16)1 << (bits)) - 1) << (pos)))) \
                          | ((oal_uint16)((val) & (((oal_uint16)1 << (bits)) - 1)) << (pos));\
}while(0)
#define REG8_SETBITS(reg, pos, bits, val)do {\
    (reg) = ((oal_uint8)(reg) & (~((((oal_uint8)1 << (bits)) - 1) << (pos)))) \
                         | ((oal_uint8)((val) & (((oal_uint8)1 << (bits)) - 1)) << (pos));\
 }while(0)

extern oal_uint32  oal_pcie_rc_mem_map(oal_void);
extern oal_void  oal_pcie_rc_mem_unmap(oal_void);
extern oal_uint32 oal_pcie_link_retrain(oal_void);
extern oal_void oal_pcie_link_aspm_enable(oal_pci_dev_stru *pst_dev, oal_pcie_aspm_enum mode);
extern oal_void oal_pcie_link_aspm_disable(oal_pci_dev_stru *pst_dev, oal_pcie_aspm_enum mode);
extern oal_void oal_pcie_link_clkpm_set(oal_pci_dev_stru *pst_dev, oal_uint32 enable);
extern oal_void oal_pcie_link_common_clk_set(oal_pci_dev_stru *pst_dev);
extern oal_void oal_pcie_link_l1ss_set(oal_pci_dev_stru *pst_dev, oal_uint32 enable);
extern oal_void oal_pcie_link_l0s_entry_latency_set(oal_pci_dev_stru *pst_dev,oal_pcie_L0s_ent_enum value);
extern oal_void oal_pcie_link_l1_entry_latency_set(oal_pci_dev_stru *pst_dev,oal_pcie_L1_ent_enum value);
extern oal_void oal_pcie_link_complete_timeout_set(oal_pci_dev_stru *pst_dev,oal_pcie_comp_timeout_enum value);
extern oal_void oal_pcie_rc_reg_print(oal_uint32 reg);
extern oal_uint32 oal_pcie_l12_set(oal_pci_dev_stru *pst_dev,oal_uint32 enable);
extern oal_uint32 oal_pcie_l11_set(oal_pci_dev_stru *pst_dev,oal_uint32 enable);
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif

