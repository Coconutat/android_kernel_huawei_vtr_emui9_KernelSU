/*
 * hisilicon ISP driver, isp.c
 *
 * Copyright (c) 2018 Hisilicon Technologies CO., Ltd.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/iommu.h>
#include "isp_ddr_map.h"
#include "hisp_internel.h"
#include <linux/platform_data/remoteproc-hisi.h>

/* Regs Base */
#define ISP_BASE_ADDR_SSMMU         0x6000
#define ISP_BASE_ADDR_CVDR_SRT      0x2E000
#define ISP_BASE_ADDR_CVDR_RT       0x22000
#define ISP_SUB_CTRL_BASE_ADDR      0x183000
#define ISP_020010_MODULE_CGR_TOP   0x020010

/* regs offset*/
#define CVDR_SRT_AXI_CFG_VP_WR_25           0xCE4
#define CVDR_SRT_AXI_CFG_NR_RD_1            0xC44
#define MID_FOR_JPGEN                       0xF

/*cvdr*/
#define CVDR_RT_CVDR_CFG_REG            0x0
#define CVDR_RT_CVDR_WR_QOS_CFG_REG     0xC
#define CVDR_RT_CVDR_RD_QOS_CFG_REG     0x10
#define CVDR_SRT_CVDR_CFG_REG           0x0
#define CVDR_SRT_CVDR_WR_QOS_CFG_REG    0xC
#define CVDR_SRT_CVDR_RD_QOS_CFG_REG    0x10
#define CVDR_SRT_VP_WR_IF_CFG_10_REG    0xC8
#define CVDR_SRT_VP_WR_IF_CFG_11_REG    0xD8
#define CVDR_SRT_VP_WR_IF_CFG_25_REG    0x1B8
#define CVDR_SRT_NR_RD_CFG_1_REG        0xA10
#define CVDR_SRT_LIMITER_NR_RD_1_REG    0xA18
#define CVDR_SRT_NR_RD_CFG_3_REG        0xA30

#define SUB_CTRL_ISP_FLUX_CTRL2_0_REG   0x190
#define SUB_CTRL_ISP_FLUX_CTRL2_1_REG   0x194
#define SUB_CTRL_ISP_FLUX_CTRL3_0_REG   0x198
#define SUB_CTRL_ISP_FLUX_CTRL3_1_REG   0x19C

#define VP_WR_REG_OFFSET (0x10)
#define VP_RD_REG_OFFSET (0x20)
#define CVDR_VP_WR_NBR    (38)
#define CVDR_VP_RD_NBR    (22)
#define CVDR_CLK_ENABLE   (0x40020)

/*smmu*/
#define PGD_BASE                  0x0
#define SMMU_CLK_ENABLE         0x400000

#define SMMU_APB_REG_SMMU_SCR_REG               0x0   /* SMMU Global Control Register */
#define SMMU_APB_REG_SMMU_MEMCTRL_REG           0x4   /* SMMU Memory Control Register */
#define SMMU_APB_REG_SMMU_LP_CTRL_REG           0x8   /* SMMU Low Power Control Register */
#define SMMU_APB_REG_SMMU_PRESS_REMAP_REG       0xC   /* SMMU Pressure Remap register */
#define SMMU_APB_REG_SMMU_INTMASK_NS_REG        0x10  /* SMMU Interrupt Mask Register for Non Secure contest bank */
#define SMMU_APB_REG_SMMU_INTRAW_NS_REG         0x14  /* SMMU Interrupt Raw Status for Non Secure contest bank */
#define SMMU_APB_REG_SMMU_INTSTAT_NS_REG        0x18  /* SMMU Interrupt Status after Mask for Non Secure contest bank */
#define SMMU_APB_REG_SMMU_INTCLR_NS_REG         0x1C  /* SMMU Interrupt Clear Register for Secure contest bank */
#define SMMU_APB_REG_SMMU_SMRX_NS_0_REG         0x20  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_1_REG         0x24  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_2_REG         0x28  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_3_REG         0x2C  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_4_REG         0x30  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_5_REG         0x34  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_6_REG         0x38  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_7_REG         0x3C  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_8_REG         0x40  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_9_REG         0x44  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_10_REG        0x48  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_11_REG        0x4C  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_12_REG        0x50  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_13_REG        0x54  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_14_REG        0x58  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_15_REG        0x5C  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_16_REG        0x60  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_17_REG        0x64  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_18_REG        0x68  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_19_REG        0x6C  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_20_REG        0x70  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_21_REG        0x74  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_22_REG        0x78  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_23_REG        0x7C  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_24_REG        0x80  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_25_REG        0x84  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_26_REG        0x88  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_27_REG        0x8C  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_28_REG        0x90  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_29_REG        0x94  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_30_REG        0x98  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_31_REG        0x9C  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_32_REG        0xA0  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_33_REG        0xA4  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_34_REG        0xA8  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_35_REG        0xAC  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_36_REG        0xB0  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_37_REG        0xB4  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_38_REG        0xB8  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_39_REG        0xBC  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_40_REG        0xC0  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_41_REG        0xC4  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_42_REG        0xC8  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_43_REG        0xCC  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_44_REG        0xD0  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_45_REG        0xD4  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_46_REG        0xD8  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_47_REG        0xDC  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_48_REG        0xE0  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_49_REG        0xE4  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_50_REG        0xE8  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_51_REG        0xEC  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_52_REG        0xF0  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_53_REG        0xF4  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_54_REG        0xF8  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_55_REG        0xFC  /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_56_REG        0x100 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_57_REG        0x104 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_58_REG        0x108 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_59_REG        0x10C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_60_REG        0x110 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_61_REG        0x114 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_62_REG        0x118 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_63_REG        0x11C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_64_REG        0x120 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_65_REG        0x124 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_66_REG        0x128 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_67_REG        0x12C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_68_REG        0x130 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_69_REG        0x134 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_70_REG        0x138 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_71_REG        0x13C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_72_REG        0x140 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_73_REG        0x144 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_74_REG        0x148 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_75_REG        0x14C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_76_REG        0x150 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_77_REG        0x154 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_78_REG        0x158 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_NS_79_REG        0x15C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_RLD_EN0_NS_REG        0x1F0 /* SMMU SMR Reload Enable Register0 */
#define SMMU_APB_REG_SMMU_RLD_EN1_NS_REG        0x1F4 /* SMMU SMR Reload Enable Register1 */
#define SMMU_APB_REG_SMMU_RLD_EN2_NS_REG        0x1F8 /* SMMU SMR Reload Enable Register2 */
#define SMMU_APB_REG_SMMU_CB_SCTRL_REG          0x200 /* SMMU System Control Register for Non-Secure Context BankCan be accessed in Non-Secure mode */
#define SMMU_APB_REG_SMMU_CB_TTBR0_REG          0x204 /* SMMU Translation Table Base Register for Non-Secure Context Bank0Can be accessed in Non-Secure mode */
#define SMMU_APB_REG_SMMU_CB_TTBR1_REG          0x208 /* SMMU Translation Table Base Register for Non-Secure Context Bank1Can be accessed in Non-Secure mode */
#define SMMU_APB_REG_SMMU_CB_TTBCR_REG          0x20C /* SMMU Translation Table Base Control Register for Non-Secure Context BankCan be accessed in Non-Secure mode */
#define SMMU_APB_REG_SMMU_OFFSET_ADDR_NS_REG    0x210 /* SMMU Offset Address Register for Non-Secure Context BankCan be accessed in Non-Secure mode */
#define SMMU_APB_REG_SMMU_SCACHEI_ALL_REG       0x214 /* Cache Invalid Register of all invalidation */
#define SMMU_APB_REG_SMMU_SCACHEI_L1_REG        0x218 /* Cache Invalid Register of Level1 invalidation */
#define SMMU_APB_REG_SMMU_SCACHEI_L2L3_REG      0x21C /* Cache Invalid Register of Level2 and Level3 invalidation */
#define SMMU_APB_REG_SMMU_FAMA_CTRL0_NS_REG     0x220 /* SMMU Control Register for FAMA for TBU of Non-Secure Context Bank */
#define SMMU_APB_REG_SMMU_FAMA_CTRL1_NS_REG     0x224 /* SMMU Control Register for FAMA for TCU of Non-Secure Context Bank */
#define SMMU_APB_REG_SMMU_ADDR_MSB_REG          0x300 /* Register for MSB of all 33-bits address configuration */
#define SMMU_APB_REG_SMMU_ERR_RDADDR_REG        0x304 /* SMMU Error Address of TLB miss for Read transaction */
#define SMMU_APB_REG_SMMU_ERR_WRADDR_REG        0x308 /* SMMU Error Address of TLB miss for Write transaction */
#define SMMU_APB_REG_SMMU_FAULT_ADDR_TCU_REG    0x310 /* Register of record of address information for the error scenario of TCU */
#define SMMU_APB_REG_SMMU_FAULT_ID_TCU_REG      0x314 /* Register of record of stream id and index id information for the error scenario of TCU */
#define SMMU_APB_REG_SMMU_FAULT_ADDR_TBUX_0_REG 0x320 /* Register of record of address information for the error scenariof is the number of TBU port */
#define SMMU_APB_REG_SMMU_FAULT_ADDR_TBUX_1_REG 0x330 /* Register of record of address information for the error scenariof is the number of TBU port */
#define SMMU_APB_REG_SMMU_FAULT_ADDR_TBUX_2_REG 0x340 /* Register of record of address information for the error scenariof is the number of TBU port */
#define SMMU_APB_REG_SMMU_FAULT_ADDR_TBUX_3_REG 0x350 /* Register of record of address information for the error scenariof is the number of TBU port */
#define SMMU_APB_REG_SMMU_FAULT_ID_TBUX_0_REG   0x324 /* Register of record of stream id and index id information for the error scenariof is the number of TBU port */
#define SMMU_APB_REG_SMMU_FAULT_ID_TBUX_1_REG   0x334 /* Register of record of stream id and index id information for the error scenariof is the number of TBU port */
#define SMMU_APB_REG_SMMU_FAULT_ID_TBUX_2_REG   0x344 /* Register of record of stream id and index id information for the error scenariof is the number of TBU port */
#define SMMU_APB_REG_SMMU_FAULT_ID_TBUX_3_REG   0x354 /* Register of record of stream id and index id information for the error scenariof is the number of TBU port */
#define SMMU_APB_REG_SMMU_FAULT_INFOX_0_REG     0x328 /* Register of record of  faults of TBUf is the number of TBU port */
#define SMMU_APB_REG_SMMU_FAULT_INFOX_1_REG     0x338 /* Register of record of  faults of TBUf is the number of TBU port */
#define SMMU_APB_REG_SMMU_FAULT_INFOX_2_REG     0x348 /* Register of record of  faults of TBUf is the number of TBU port */
#define SMMU_APB_REG_SMMU_FAULT_INFOX_3_REG     0x358 /* Register of record of  faults of TBUf is the number of TBU port */
#define SMMU_APB_REG_SMMU_DBGRPTR_TLB_REG       0x380 /* SMMU Debug Pointer of TLB */
#define SMMU_APB_REG_SMMU_DBGRDATA_TLB_REG      0x384 /* SMMU Debug Data of TLB */
#define SMMU_APB_REG_SMMU_DBGRPTR_CACHE_REG     0x388 /* SMMU Debug Pointer of Cache */
#define SMMU_APB_REG_SMMU_DBGRDATA0_CACHE_REG   0x38C /* SMMU Debug Data of Cache */
#define SMMU_APB_REG_SMMU_DBGRDATA1_CACHE_REG   0x390 /* SMMU Debug Data of Cache */
#define SMMU_APB_REG_SMMU_DBGAXI_CTRL_REG       0x394 /* SMMU Debug of AXI interface control */
#define SMMU_APB_REG_SMMU_OVA_ADDR_REG          0x398 /* SMMU Override of VA address */
#define SMMU_APB_REG_SMMU_OPA_ADDR_REG          0x39C /* SMMU Override of PA address */
#define SMMU_APB_REG_SMMU_OVA_CTRL_REG          0x3A0 /* SMMU Override of VA configuration */
#define SMMU_APB_REG_SMMU_OPREF_ADDR_REG        0x3A4 /* SMMU Override of Prefetch address */
#define SMMU_APB_REG_SMMU_OPREF_CTRL_REG        0x3A8 /* SMMU Override of Prefetch configuration */
#define SMMU_APB_REG_SMMU_OPREF_CNT_REG         0x3AC /* Counter for Override of prefetch */
#define SMMU_APB_REG_SMMU_SMRX_S_0_REG          0x500 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_1_REG          0x504 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_2_REG          0x508 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_3_REG          0x50C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_4_REG          0x510 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_5_REG          0x514 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_6_REG          0x518 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_7_REG          0x51C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_8_REG          0x520 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_9_REG          0x524 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_10_REG         0x528 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_11_REG         0x52C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_12_REG         0x530 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_13_REG         0x534 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_14_REG         0x538 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_15_REG         0x53C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_16_REG         0x540 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_17_REG         0x544 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_18_REG         0x548 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_19_REG         0x54C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_20_REG         0x550 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_21_REG         0x554 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_22_REG         0x558 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_23_REG         0x55C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_24_REG         0x560 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_25_REG         0x564 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_26_REG         0x568 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_27_REG         0x56C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_28_REG         0x570 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_29_REG         0x574 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_30_REG         0x578 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_31_REG         0x57C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_32_REG         0x580 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_33_REG         0x584 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_34_REG         0x588 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_35_REG         0x58C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_36_REG         0x590 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_37_REG         0x594 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_38_REG         0x598 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_39_REG         0x59C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_40_REG         0x5A0 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_41_REG         0x5A4 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_42_REG         0x5A8 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_43_REG         0x5AC /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_44_REG         0x5B0 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_45_REG         0x5B4 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_46_REG         0x5B8 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_47_REG         0x5BC /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_48_REG         0x5C0 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_49_REG         0x5C4 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_50_REG         0x5C8 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_51_REG         0x5CC /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_52_REG         0x5D0 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_53_REG         0x5D4 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_54_REG         0x5D8 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_55_REG         0x5DC /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_56_REG         0x5E0 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_57_REG         0x5E4 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_58_REG         0x5E8 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_59_REG         0x5EC /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_60_REG         0x5F0 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_61_REG         0x5F4 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_62_REG         0x5F8 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_63_REG         0x5FC /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_64_REG         0x600 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_65_REG         0x604 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_66_REG         0x608 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_67_REG         0x60C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_68_REG         0x610 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_69_REG         0x614 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_70_REG         0x618 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_71_REG         0x61C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_72_REG         0x620 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_73_REG         0x624 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_74_REG         0x628 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_75_REG         0x62C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_76_REG         0x630 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_77_REG         0x634 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_78_REG         0x638 /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_SMRX_S_79_REG         0x63C /* SMMU Control Register per Stream */
#define SMMU_APB_REG_SMMU_RLD_EN0_S_REG         0x6F0 /* SMMU SMR Reload Enable Register0 */
#define SMMU_APB_REG_SMMU_RLD_EN1_S_REG         0x6F4 /* SMMU SMR Reload Enable Register1 */
#define SMMU_APB_REG_SMMU_RLD_EN2_S_REG         0x6F8 /* SMMU SMR Reload Enable Register2 */
#define SMMU_APB_REG_SMMU_INTMAS_S_REG          0x700 /* SMMU Interrupt Mask Register for Secure contest bank */
#define SMMU_APB_REG_SMMU_INTRAW_S_REG          0x704 /* SMMU Interrupt Raw Status for Secure contest bank */
#define SMMU_APB_REG_SMMU_INTSTAT_S_REG         0x708 /* SMMU Interrupt Status after Mask for Secure contest bank */
#define SMMU_APB_REG_SMMU_INTCLR_S_REG          0x70C /* SMMU Interrupt Clear Register for Secure contest bank */
#define SMMU_APB_REG_SMMU_SCR_S_REG             0x710 /* SMMU Global Control Register */
#define SMMU_APB_REG_SMMU_SCB_SCTRL_REG         0x714 /* SMMU System Control Register for Secure Context Bank */
#define SMMU_APB_REG_SMMU_SCB_TTBR_REG          0x718 /* SMMU Translation Table Base Register for Secure Context Bank */
#define SMMU_APB_REG_SMMU_SCB_TTBCR_REG         0x71C /* SMMU Translation Table Base Control Register for Secure Context Bank */
#define SMMU_APB_REG_SMMU_OFFSET_ADDR_S_REG     0x720 /* SMMU Offset Address Register for Secure Context Bank */
#define SMMU_APB_REG_SMMU_FAMA_CTRL0_S_REG      0x724 /* SMMU Control Register for FAMA for TBU of Secure Context Bank */
#define SMMU_APB_REG_SMMU_FAMA_CTRL1_S_REG      0x728 /* SMMU Control Register for FAMA for TCU of Secure Context Bank */
#define SMMU_APB_REG_SMMU_DBGRPTR_TLB_S_REG     0x72C /* SMMU Debug Pointer of TLB */
#define SMMU_APB_REG_SMMU_DBGRPTR_CACHE_S_REG   0x730 /* SMMU Debug Pointer of Cache */
#define SMMU_APB_REG_SMMU_OVERRIDE_CTRL_S_REG   0x734 /* SMMU Override security control */

#define SMMU_APB_REG_PTW_MID_LEN          8
#define SMMU_APB_REG_PTW_MID_OFFSET       20
#define SMMU_APB_REG_PTW_PF_LEN           4
#define SMMU_APB_REG_PTW_PF_OFFSET        16
#define SMMU_APB_REG_SMR_RD_SHADOW_LEN    1
#define SMMU_APB_REG_SMR_RD_SHADOW_OFFSET 15
#define SMMU_APB_REG_WQOS_LEN             4
#define SMMU_APB_REG_WQOS_OFFSET          10
#define SMMU_APB_REG_RQOS_LEN             4
#define SMMU_APB_REG_RQOS_OFFSET          6
#define SMMU_APB_REG_CACHE_L2_EN_LEN      1
#define SMMU_APB_REG_CACHE_L2_EN_OFFSET   5
#define SMMU_APB_REG_CACHE_L1_EN_LEN      1
#define SMMU_APB_REG_CACHE_L1_EN_OFFSET   4
#define SMMU_APB_REG_INT_EN_LEN           1
#define SMMU_APB_REG_INT_EN_OFFSET        3
#define SMMU_APB_REG_WQOS_EN_LEN          1
#define SMMU_APB_REG_WQOS_EN_OFFSET       2
#define SMMU_APB_REG_RQOS_EN_LEN          1
#define SMMU_APB_REG_RQOS_EN_OFFSET       1
#define SMMU_APB_REG_GLB_BYPASS_LEN       1
#define SMMU_APB_REG_GLB_BYPASS_OFFSET    0

#define SMMU_APB_REG_MEM_CTRL_RD_LEN    16
#define SMMU_APB_REG_MEM_CTRL_RD_OFFSET 16
#define SMMU_APB_REG_MEM_CTRL_WR_LEN    16
#define SMMU_APB_REG_MEM_CTRL_WR_OFFSET 0

#define SMMU_APB_REG_SMMU_IDLE_LEN         1
#define SMMU_APB_REG_SMMU_IDLE_OFFSET      2
#define SMMU_APB_REG_AUTO_CLK_GT_EN_LEN    1
#define SMMU_APB_REG_AUTO_CLK_GT_EN_OFFSET 0

#define SMMU_APB_REG_REMAP_SEL15_LEN    2
#define SMMU_APB_REG_REMAP_SEL15_OFFSET 30
#define SMMU_APB_REG_REMAP_SEL14_LEN    2
#define SMMU_APB_REG_REMAP_SEL14_OFFSET 28
#define SMMU_APB_REG_REMAP_SEL13_LEN    2
#define SMMU_APB_REG_REMAP_SEL13_OFFSET 26
#define SMMU_APB_REG_REMAP_SEL12_LEN    2
#define SMMU_APB_REG_REMAP_SEL12_OFFSET 24
#define SMMU_APB_REG_REMAP_SEL11_LEN    2
#define SMMU_APB_REG_REMAP_SEL11_OFFSET 22
#define SMMU_APB_REG_REMAP_SEL10_LEN    2
#define SMMU_APB_REG_REMAP_SEL10_OFFSET 20
#define SMMU_APB_REG_REMAP_SEL9_LEN     2
#define SMMU_APB_REG_REMAP_SEL9_OFFSET  18
#define SMMU_APB_REG_REMAP_SEL8_LEN     2
#define SMMU_APB_REG_REMAP_SEL8_OFFSET  16
#define SMMU_APB_REG_REMAP_SEL7_LEN     2
#define SMMU_APB_REG_REMAP_SEL7_OFFSET  14
#define SMMU_APB_REG_REMAP_SEL6_LEN     2
#define SMMU_APB_REG_REMAP_SEL6_OFFSET  12
#define SMMU_APB_REG_REMAP_SEL5_LEN     2
#define SMMU_APB_REG_REMAP_SEL5_OFFSET  10
#define SMMU_APB_REG_REMAP_SEL4_LEN     2
#define SMMU_APB_REG_REMAP_SEL4_OFFSET  8
#define SMMU_APB_REG_REMAP_SEL3_LEN     2
#define SMMU_APB_REG_REMAP_SEL3_OFFSET  6
#define SMMU_APB_REG_REMAP_SEL2_LEN     2
#define SMMU_APB_REG_REMAP_SEL2_OFFSET  4
#define SMMU_APB_REG_REMAP_SEL1_LEN     2
#define SMMU_APB_REG_REMAP_SEL1_OFFSET  2
#define SMMU_APB_REG_REMAP_SEL0_LEN     2
#define SMMU_APB_REG_REMAP_SEL0_OFFSET  0

#define SMMU_APB_REG_INTNS_PTW_NS_MSK_LEN         1
#define SMMU_APB_REG_INTNS_PTW_NS_MSK_OFFSET      5
#define SMMU_APB_REG_INTNS_PTW_INVALID_MSK_LEN    1
#define SMMU_APB_REG_INTNS_PTW_INVALID_MSK_OFFSET 4
#define SMMU_APB_REG_INTNS_PTW_TRANS_MSK_LEN      1
#define SMMU_APB_REG_INTNS_PTW_TRANS_MSK_OFFSET   3
#define SMMU_APB_REG_INTNS_TLBMISS_MSK_LEN        1
#define SMMU_APB_REG_INTNS_TLBMISS_MSK_OFFSET     2
#define SMMU_APB_REG_INTNS_EXT_MSK_LEN            1
#define SMMU_APB_REG_INTNS_EXT_MSK_OFFSET         1
#define SMMU_APB_REG_INTNS_PERMIS_MSK_LEN         1
#define SMMU_APB_REG_INTNS_PERMIS_MSK_OFFSET      0

#define SMMU_APB_REG_INTNS_PTW_NS_RAW_LEN         1
#define SMMU_APB_REG_INTNS_PTW_NS_RAW_OFFSET      5
#define SMMU_APB_REG_INTNS_PTW_INVALID_RAW_LEN    1
#define SMMU_APB_REG_INTNS_PTW_INVALID_RAW_OFFSET 4
#define SMMU_APB_REG_INTNS_PTW_TRANS_RAW_LEN      1
#define SMMU_APB_REG_INTNS_PTW_TRANS_RAW_OFFSET   3
#define SMMU_APB_REG_INTNS_TLBMISS_RAW_LEN        1
#define SMMU_APB_REG_INTNS_TLBMISS_RAW_OFFSET     2
#define SMMU_APB_REG_INTNS_EXT_RAW_LEN            1
#define SMMU_APB_REG_INTNS_EXT_RAW_OFFSET         1
#define SMMU_APB_REG_INTNS_PERMIS_RAW_LEN         1
#define SMMU_APB_REG_INTNS_PERMIS_RAW_OFFSET      0

#define SMMU_APB_REG_INTNS_PTW_NS_STAT_LEN         1
#define SMMU_APB_REG_INTNS_PTW_NS_STAT_OFFSET      5
#define SMMU_APB_REG_INTNS_PTW_INVALID_STAT_LEN    1
#define SMMU_APB_REG_INTNS_PTW_INVALID_STAT_OFFSET 4
#define SMMU_APB_REG_INTNS_PTW_TRANS_STAT_LEN      1
#define SMMU_APB_REG_INTNS_PTW_TRANS_STAT_OFFSET   3
#define SMMU_APB_REG_INTNS_TLBMISS_STAT_LEN        1
#define SMMU_APB_REG_INTNS_TLBMISS_STAT_OFFSET     2
#define SMMU_APB_REG_INTNS_EXT_STAT_LEN            1
#define SMMU_APB_REG_INTNS_EXT_STAT_OFFSET         1
#define SMMU_APB_REG_INTNS_PERMIS_STAT_LEN         1
#define SMMU_APB_REG_INTNS_PERMIS_STAT_OFFSET      0

#define SMMU_APB_REG_INTNS_PTW_NS_CLR_LEN         1
#define SMMU_APB_REG_INTNS_PTW_NS_CLR_OFFSET      5
#define SMMU_APB_REG_INTNS_PTW_INVALID_CLR_LEN    1
#define SMMU_APB_REG_INTNS_PTW_INVALID_CLR_OFFSET 4
#define SMMU_APB_REG_INTNS_PTW_TRANS_CLR_LEN      1
#define SMMU_APB_REG_INTNS_PTW_TRANS_CLR_OFFSET   3
#define SMMU_APB_REG_INTNS_TLBMISS_CLR_LEN        1
#define SMMU_APB_REG_INTNS_TLBMISS_CLR_OFFSET     2
#define SMMU_APB_REG_INTNS_EXT_CLR_LEN            1
#define SMMU_APB_REG_INTNS_EXT_CLR_OFFSET         1
#define SMMU_APB_REG_INTNS_PERMIS_CLR_LEN         1
#define SMMU_APB_REG_INTNS_PERMIS_CLR_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_0_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_0_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_0_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_0_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_0_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_0_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_0_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_0_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_1_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_1_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_1_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_1_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_1_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_1_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_1_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_1_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_2_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_2_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_2_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_2_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_2_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_2_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_2_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_2_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_3_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_3_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_3_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_3_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_3_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_3_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_3_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_3_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_4_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_4_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_4_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_4_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_4_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_4_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_4_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_4_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_5_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_5_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_5_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_5_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_5_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_5_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_5_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_5_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_6_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_6_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_6_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_6_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_6_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_6_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_6_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_6_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_7_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_7_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_7_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_7_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_7_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_7_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_7_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_7_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_8_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_8_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_8_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_8_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_8_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_8_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_8_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_8_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_9_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_9_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_9_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_9_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_9_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_9_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_9_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_9_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_10_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_10_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_10_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_10_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_10_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_10_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_10_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_10_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_11_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_11_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_11_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_11_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_11_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_11_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_11_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_11_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_12_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_12_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_12_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_12_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_12_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_12_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_12_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_12_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_13_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_13_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_13_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_13_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_13_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_13_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_13_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_13_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_14_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_14_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_14_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_14_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_14_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_14_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_14_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_14_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_15_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_15_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_15_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_15_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_15_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_15_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_15_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_15_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_16_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_16_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_16_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_16_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_16_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_16_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_16_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_16_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_17_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_17_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_17_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_17_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_17_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_17_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_17_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_17_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_18_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_18_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_18_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_18_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_18_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_18_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_18_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_18_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_19_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_19_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_19_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_19_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_19_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_19_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_19_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_19_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_20_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_20_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_20_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_20_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_20_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_20_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_20_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_20_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_21_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_21_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_21_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_21_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_21_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_21_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_21_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_21_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_22_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_22_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_22_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_22_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_22_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_22_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_22_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_22_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_23_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_23_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_23_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_23_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_23_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_23_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_23_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_23_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_24_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_24_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_24_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_24_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_24_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_24_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_24_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_24_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_25_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_25_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_25_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_25_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_25_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_25_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_25_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_25_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_26_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_26_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_26_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_26_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_26_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_26_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_26_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_26_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_27_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_27_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_27_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_27_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_27_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_27_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_27_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_27_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_28_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_28_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_28_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_28_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_28_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_28_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_28_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_28_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_29_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_29_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_29_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_29_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_29_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_29_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_29_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_29_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_30_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_30_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_30_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_30_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_30_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_30_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_30_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_30_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_31_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_31_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_31_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_31_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_31_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_31_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_31_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_31_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_32_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_32_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_32_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_32_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_32_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_32_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_32_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_32_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_33_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_33_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_33_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_33_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_33_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_33_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_33_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_33_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_34_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_34_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_34_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_34_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_34_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_34_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_34_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_34_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_35_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_35_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_35_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_35_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_35_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_35_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_35_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_35_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_36_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_36_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_36_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_36_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_36_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_36_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_36_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_36_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_37_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_37_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_37_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_37_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_37_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_37_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_37_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_37_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_38_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_38_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_38_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_38_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_38_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_38_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_38_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_38_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_39_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_39_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_39_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_39_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_39_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_39_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_39_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_39_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_40_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_40_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_40_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_40_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_40_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_40_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_40_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_40_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_41_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_41_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_41_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_41_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_41_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_41_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_41_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_41_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_42_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_42_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_42_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_42_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_42_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_42_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_42_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_42_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_43_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_43_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_43_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_43_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_43_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_43_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_43_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_43_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_44_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_44_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_44_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_44_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_44_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_44_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_44_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_44_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_45_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_45_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_45_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_45_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_45_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_45_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_45_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_45_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_46_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_46_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_46_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_46_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_46_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_46_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_46_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_46_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_47_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_47_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_47_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_47_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_47_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_47_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_47_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_47_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_48_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_48_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_48_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_48_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_48_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_48_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_48_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_48_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_49_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_49_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_49_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_49_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_49_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_49_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_49_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_49_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_50_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_50_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_50_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_50_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_50_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_50_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_50_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_50_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_51_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_51_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_51_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_51_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_51_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_51_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_51_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_51_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_52_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_52_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_52_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_52_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_52_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_52_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_52_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_52_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_53_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_53_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_53_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_53_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_53_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_53_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_53_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_53_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_54_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_54_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_54_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_54_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_54_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_54_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_54_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_54_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_55_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_55_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_55_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_55_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_55_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_55_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_55_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_55_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_56_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_56_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_56_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_56_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_56_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_56_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_56_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_56_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_57_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_57_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_57_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_57_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_57_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_57_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_57_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_57_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_58_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_58_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_58_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_58_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_58_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_58_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_58_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_58_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_59_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_59_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_59_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_59_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_59_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_59_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_59_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_59_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_60_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_60_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_60_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_60_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_60_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_60_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_60_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_60_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_61_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_61_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_61_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_61_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_61_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_61_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_61_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_61_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_62_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_62_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_62_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_62_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_62_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_62_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_62_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_62_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_63_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_63_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_63_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_63_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_63_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_63_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_63_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_63_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_64_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_64_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_64_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_64_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_64_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_64_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_64_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_64_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_65_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_65_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_65_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_65_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_65_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_65_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_65_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_65_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_66_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_66_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_66_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_66_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_66_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_66_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_66_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_66_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_67_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_67_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_67_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_67_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_67_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_67_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_67_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_67_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_68_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_68_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_68_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_68_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_68_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_68_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_68_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_68_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_69_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_69_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_69_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_69_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_69_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_69_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_69_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_69_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_70_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_70_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_70_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_70_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_70_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_70_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_70_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_70_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_71_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_71_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_71_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_71_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_71_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_71_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_71_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_71_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_72_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_72_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_72_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_72_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_72_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_72_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_72_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_72_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_73_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_73_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_73_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_73_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_73_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_73_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_73_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_73_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_74_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_74_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_74_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_74_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_74_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_74_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_74_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_74_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_75_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_75_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_75_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_75_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_75_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_75_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_75_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_75_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_76_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_76_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_76_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_76_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_76_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_76_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_76_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_76_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_77_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_77_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_77_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_77_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_77_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_77_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_77_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_77_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_78_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_78_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_78_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_78_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_78_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_78_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_78_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_78_OFFSET      0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_79_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_79_OFFSET 12
#define SMMU_APB_REG_SMR_PTW_QOS_79_LEN        7
#define SMMU_APB_REG_SMR_PTW_QOS_79_OFFSET     5
#define SMMU_APB_REG_SMR_INVLD_EN_79_LEN       1
#define SMMU_APB_REG_SMR_INVLD_EN_79_OFFSET    4
#define SMMU_APB_REG_SMR_BYPASS_79_LEN         1
#define SMMU_APB_REG_SMR_BYPASS_79_OFFSET      0

#define SMMU_APB_REG_SMR_RLD_EN0_LEN    32
#define SMMU_APB_REG_SMR_RLD_EN0_OFFSET 0

#define SMMU_APB_REG_SMR_RLD_EN1_LEN    32
#define SMMU_APB_REG_SMR_RLD_EN1_OFFSET 0

#define SMMU_APB_REG_SMR_RLD_EN2_LEN    16
#define SMMU_APB_REG_SMR_RLD_EN2_OFFSET 0

#define SMMU_APB_REG_CB_BYPASS_LEN    1
#define SMMU_APB_REG_CB_BYPASS_OFFSET 0

#define SMMU_APB_REG_CB_TTBR0_LEN    32
#define SMMU_APB_REG_CB_TTBR0_OFFSET 0

#define SMMU_APB_REG_CB_TTBR1_LEN    32
#define SMMU_APB_REG_CB_TTBR1_OFFSET 0

#define SMMU_APB_REG_CB_TTBCR_T1SZ_LEN    3
#define SMMU_APB_REG_CB_TTBCR_T1SZ_OFFSET 16
#define SMMU_APB_REG_CB_TTBCR_N_LEN       3
#define SMMU_APB_REG_CB_TTBCR_N_OFFSET    4
#define SMMU_APB_REG_CB_TTBCR_T0SZ_LEN    3
#define SMMU_APB_REG_CB_TTBCR_T0SZ_OFFSET 1
#define SMMU_APB_REG_CB_TTBCR_DES_LEN     1
#define SMMU_APB_REG_CB_TTBCR_DES_OFFSET  0

#define SMMU_APB_REG_OFFSET_ADDR_NS_LEN    14
#define SMMU_APB_REG_OFFSET_ADDR_NS_OFFSET 0

#define SMMU_APB_REG_CACHE_ALL_LEVEL_LEN      2
#define SMMU_APB_REG_CACHE_ALL_LEVEL_OFFSET   1
#define SMMU_APB_REG_CACHE_ALL_INVALID_LEN    1
#define SMMU_APB_REG_CACHE_ALL_INVALID_OFFSET 0

#define SMMU_APB_REG_CACHE_L1_SECURITY_LEN    1
#define SMMU_APB_REG_CACHE_L1_SECURITY_OFFSET 1
#define SMMU_APB_REG_CACHE_L1_INVALID_LEN     1
#define SMMU_APB_REG_CACHE_L1_INVALID_OFFSET  0

#define SMMU_APB_REG_CACHE_L2L3_STRMID_LEN     15
#define SMMU_APB_REG_CACHE_L2L3_STRMID_OFFSET  1
#define SMMU_APB_REG_CACHE_L2L3_INVALID_LEN    1
#define SMMU_APB_REG_CACHE_L2L3_INVALID_OFFSET 0

#define SMMU_APB_REG_FAMA_BPS_MSB_NS_LEN     6
#define SMMU_APB_REG_FAMA_BPS_MSB_NS_OFFSET  8
#define SMMU_APB_REG_FAMA_CHN_SEL_NS_LEN     1
#define SMMU_APB_REG_FAMA_CHN_SEL_NS_OFFSET  7
#define SMMU_APB_REG_FAMA_SDES_MSB_NS_LEN    7
#define SMMU_APB_REG_FAMA_SDES_MSB_NS_OFFSET 0

#define SMMU_APB_REG_FAMA_PTW_MSB_NS_LEN    7
#define SMMU_APB_REG_FAMA_PTW_MSB_NS_OFFSET 0

#define SMMU_APB_REG_MSB_OVA_LEN      7
#define SMMU_APB_REG_MSB_OVA_OFFSET   14
#define SMMU_APB_REG_MSB_ERRWR_LEN    7
#define SMMU_APB_REG_MSB_ERRWR_OFFSET 7
#define SMMU_APB_REG_MSB_ERRRD_LEN    7
#define SMMU_APB_REG_MSB_ERRRD_OFFSET 0

#define SMMU_APB_REG_ERR_RD_ADDR_LEN    32
#define SMMU_APB_REG_ERR_RD_ADDR_OFFSET 0

#define SMMU_APB_REG_ERR_WR_ADDR_LEN    32
#define SMMU_APB_REG_ERR_WR_ADDR_OFFSET 0

#define SMMU_APB_REG_FAULT_ADDR_TCU_LEN    32
#define SMMU_APB_REG_FAULT_ADDR_TCU_OFFSET 0

#define SMMU_APB_REG_FAULT_STRM_ID_TCU_LEN     16
#define SMMU_APB_REG_FAULT_STRM_ID_TCU_OFFSET  16
#define SMMU_APB_REG_FAULT_INDEX_ID_TCU_LEN    16
#define SMMU_APB_REG_FAULT_INDEX_ID_TCU_OFFSET 0

#define SMMU_APB_REG_FAULT_ADDR_TBU_0_LEN    32
#define SMMU_APB_REG_FAULT_ADDR_TBU_0_OFFSET 0

#define SMMU_APB_REG_FAULT_ADDR_TBU_1_LEN    32
#define SMMU_APB_REG_FAULT_ADDR_TBU_1_OFFSET 0

#define SMMU_APB_REG_FAULT_ADDR_TBU_2_LEN    32
#define SMMU_APB_REG_FAULT_ADDR_TBU_2_OFFSET 0

#define SMMU_APB_REG_FAULT_ADDR_TBU_3_LEN    32
#define SMMU_APB_REG_FAULT_ADDR_TBU_3_OFFSET 0

#define SMMU_APB_REG_FAULT_STRM_ID_TBU_0_LEN     16
#define SMMU_APB_REG_FAULT_STRM_ID_TBU_0_OFFSET  16
#define SMMU_APB_REG_FAULT_INDEX_ID_TBU_0_LEN    16
#define SMMU_APB_REG_FAULT_INDEX_ID_TBU_0_OFFSET 0

#define SMMU_APB_REG_FAULT_STRM_ID_TBU_1_LEN     16
#define SMMU_APB_REG_FAULT_STRM_ID_TBU_1_OFFSET  16
#define SMMU_APB_REG_FAULT_INDEX_ID_TBU_1_LEN    16
#define SMMU_APB_REG_FAULT_INDEX_ID_TBU_1_OFFSET 0

#define SMMU_APB_REG_FAULT_STRM_ID_TBU_2_LEN     16
#define SMMU_APB_REG_FAULT_STRM_ID_TBU_2_OFFSET  16
#define SMMU_APB_REG_FAULT_INDEX_ID_TBU_2_LEN    16
#define SMMU_APB_REG_FAULT_INDEX_ID_TBU_2_OFFSET 0

#define SMMU_APB_REG_FAULT_STRM_ID_TBU_3_LEN     16
#define SMMU_APB_REG_FAULT_STRM_ID_TBU_3_OFFSET  16
#define SMMU_APB_REG_FAULT_INDEX_ID_TBU_3_LEN    16
#define SMMU_APB_REG_FAULT_INDEX_ID_TBU_3_OFFSET 0

#define SMMU_APB_REG_FAULT_EXT_ERR_ID_TBU_0_LEN     10
#define SMMU_APB_REG_FAULT_EXT_ERR_ID_TBU_0_OFFSET  6
#define SMMU_APB_REG_FAUTL_EXT_ERR_TBU_0_LEN        2
#define SMMU_APB_REG_FAUTL_EXT_ERR_TBU_0_OFFSET     4
#define SMMU_APB_REG_FAULT_PERMIS_ERR_TBU_0_LEN     2
#define SMMU_APB_REG_FAULT_PERMIS_ERR_TBU_0_OFFSET  2
#define SMMU_APB_REG_FAULT_TLBMISS_ERR_TBU_0_LEN    2
#define SMMU_APB_REG_FAULT_TLBMISS_ERR_TBU_0_OFFSET 0

#define SMMU_APB_REG_FAULT_EXT_ERR_ID_TBU_1_LEN     10
#define SMMU_APB_REG_FAULT_EXT_ERR_ID_TBU_1_OFFSET  6
#define SMMU_APB_REG_FAUTL_EXT_ERR_TBU_1_LEN        2
#define SMMU_APB_REG_FAUTL_EXT_ERR_TBU_1_OFFSET     4
#define SMMU_APB_REG_FAULT_PERMIS_ERR_TBU_1_LEN     2
#define SMMU_APB_REG_FAULT_PERMIS_ERR_TBU_1_OFFSET  2
#define SMMU_APB_REG_FAULT_TLBMISS_ERR_TBU_1_LEN    2
#define SMMU_APB_REG_FAULT_TLBMISS_ERR_TBU_1_OFFSET 0

#define SMMU_APB_REG_FAULT_EXT_ERR_ID_TBU_2_LEN     10
#define SMMU_APB_REG_FAULT_EXT_ERR_ID_TBU_2_OFFSET  6
#define SMMU_APB_REG_FAUTL_EXT_ERR_TBU_2_LEN        2
#define SMMU_APB_REG_FAUTL_EXT_ERR_TBU_2_OFFSET     4
#define SMMU_APB_REG_FAULT_PERMIS_ERR_TBU_2_LEN     2
#define SMMU_APB_REG_FAULT_PERMIS_ERR_TBU_2_OFFSET  2
#define SMMU_APB_REG_FAULT_TLBMISS_ERR_TBU_2_LEN    2
#define SMMU_APB_REG_FAULT_TLBMISS_ERR_TBU_2_OFFSET 0

#define SMMU_APB_REG_FAULT_EXT_ERR_ID_TBU_3_LEN     10
#define SMMU_APB_REG_FAULT_EXT_ERR_ID_TBU_3_OFFSET  6
#define SMMU_APB_REG_FAUTL_EXT_ERR_TBU_3_LEN        2
#define SMMU_APB_REG_FAUTL_EXT_ERR_TBU_3_OFFSET     4
#define SMMU_APB_REG_FAULT_PERMIS_ERR_TBU_3_LEN     2
#define SMMU_APB_REG_FAULT_PERMIS_ERR_TBU_3_OFFSET  2
#define SMMU_APB_REG_FAULT_TLBMISS_ERR_TBU_3_LEN    2
#define SMMU_APB_REG_FAULT_TLBMISS_ERR_TBU_3_OFFSET 0

#define SMMU_APB_REG_DBG_TLB_TYPE_LEN            1
#define SMMU_APB_REG_DBG_TLB_TYPE_OFFSET         30
#define SMMU_APB_REG_DBG_TLBENTRY_POINTER_LEN    13
#define SMMU_APB_REG_DBG_TLBENTRY_POINTER_OFFSET 3
#define SMMU_APB_REG_DBG_TLBWORD_POINTER_LEN     3
#define SMMU_APB_REG_DBG_TLBWORD_POINTER_OFFSET  0

#define SMMU_APB_REG_DBG_TLB_RDATA_LEN    31
#define SMMU_APB_REG_DBG_TLB_RDATA_OFFSET 0

#define SMMU_APB_REG_DBG_CACHE_L2_STRMID_LEN     11
#define SMMU_APB_REG_DBG_CACHE_L2_STRMID_OFFSET  5
#define SMMU_APB_REG_DBG_CACHE_L1_NS_LEN         1
#define SMMU_APB_REG_DBG_CACHE_L1_NS_OFFSET      4
#define SMMU_APB_REG_DBG_CACHE_L1_POINTER_LEN    2
#define SMMU_APB_REG_DBG_CACHE_L1_POINTER_OFFSET 2
#define SMMU_APB_REG_DBG_CACHE_LEVEL_LEN         2
#define SMMU_APB_REG_DBG_CACHE_LEVEL_OFFSET      0

#define SMMU_APB_REG_DBG_CACHE_RDATA0_LEN    29
#define SMMU_APB_REG_DBG_CACHE_RDATA0_OFFSET 0

#define SMMU_APB_REG_DBG_CACHE_RDATA1_LEN    12
#define SMMU_APB_REG_DBG_CACHE_RDATA1_OFFSET 0

#define SMMU_APB_REG_DBG_AXILOCK_EN_LEN    1
#define SMMU_APB_REG_DBG_AXILOCK_EN_OFFSET 0

#define SMMU_APB_REG_OVERRIDE_VA_LEN    32
#define SMMU_APB_REG_OVERRIDE_VA_OFFSET 0

#define SMMU_APB_REG_OVERRIDE_PA_DONE_LEN       1
#define SMMU_APB_REG_OVERRIDE_PA_DONE_OFFSET    31
#define SMMU_APB_REG_OVERRIDE_ACQUIRE_PA_LEN    27
#define SMMU_APB_REG_OVERRIDE_ACQUIRE_PA_OFFSET 0

#define SMMU_APB_REG_OVERRIDE_TBU_NUM_LEN       4
#define SMMU_APB_REG_OVERRIDE_TBU_NUM_OFFSET    28
#define SMMU_APB_REG_OVERRIDE_VA_STRMID_LEN     12
#define SMMU_APB_REG_OVERRIDE_VA_STRMID_OFFSET  16
#define SMMU_APB_REG_OVERRIDE_VA_INDEXID_LEN    13
#define SMMU_APB_REG_OVERRIDE_VA_INDEXID_OFFSET 3
#define SMMU_APB_REG_OVERRIDE_VA_TYPE_LEN       1
#define SMMU_APB_REG_OVERRIDE_VA_TYPE_OFFSET    2
#define SMMU_APB_REG_OVERRIDE_VA_CFG_LEN        1
#define SMMU_APB_REG_OVERRIDE_VA_CFG_OFFSET     0

#define SMMU_APB_REG_OVERRIDE_PREF_ADDR_LEN    32
#define SMMU_APB_REG_OVERRIDE_PREF_ADDR_OFFSET 0

#define SMMU_APB_REG_OVERRIDE_PREF_STRMID_LEN     16
#define SMMU_APB_REG_OVERRIDE_PREF_STRMID_OFFSET  16
#define SMMU_APB_REG_OVERRIDE_PREF_INDEXID_LEN    12
#define SMMU_APB_REG_OVERRIDE_PREF_INDEXID_OFFSET 4
#define SMMU_APB_REG_OVERRIDE_PREF_INITIAL_LEN    1
#define SMMU_APB_REG_OVERRIDE_PREF_INITIAL_OFFSET 3
#define SMMU_APB_REG_OVERRIDE_PREF_TYPE_LEN       1
#define SMMU_APB_REG_OVERRIDE_PREF_TYPE_OFFSET    2
#define SMMU_APB_REG_OVERRIDE_PREF_CFG_LEN        1
#define SMMU_APB_REG_OVERRIDE_PREF_CFG_OFFSET     0

#define SMMU_APB_REG_OVERRIDE_PREF_CNT_LEN    32
#define SMMU_APB_REG_OVERRIDE_PREF_CNT_OFFSET 0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_0_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_0_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_0_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_0_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_0_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_0_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_1_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_1_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_1_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_1_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_1_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_1_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_2_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_2_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_2_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_2_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_2_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_2_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_3_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_3_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_3_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_3_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_3_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_3_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_4_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_4_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_4_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_4_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_4_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_4_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_5_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_5_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_5_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_5_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_5_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_5_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_6_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_6_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_6_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_6_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_6_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_6_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_7_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_7_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_7_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_7_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_7_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_7_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_8_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_8_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_8_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_8_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_8_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_8_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_9_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_9_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_9_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_9_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_9_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_9_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_10_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_10_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_10_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_10_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_10_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_10_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_11_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_11_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_11_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_11_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_11_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_11_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_12_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_12_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_12_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_12_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_12_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_12_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_13_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_13_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_13_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_13_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_13_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_13_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_14_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_14_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_14_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_14_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_14_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_14_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_15_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_15_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_15_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_15_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_15_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_15_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_16_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_16_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_16_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_16_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_16_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_16_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_17_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_17_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_17_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_17_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_17_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_17_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_18_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_18_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_18_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_18_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_18_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_18_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_19_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_19_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_19_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_19_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_19_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_19_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_20_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_20_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_20_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_20_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_20_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_20_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_21_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_21_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_21_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_21_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_21_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_21_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_22_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_22_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_22_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_22_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_22_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_22_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_23_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_23_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_23_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_23_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_23_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_23_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_24_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_24_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_24_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_24_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_24_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_24_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_25_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_25_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_25_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_25_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_25_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_25_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_26_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_26_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_26_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_26_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_26_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_26_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_27_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_27_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_27_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_27_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_27_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_27_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_28_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_28_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_28_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_28_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_28_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_28_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_29_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_29_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_29_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_29_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_29_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_29_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_30_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_30_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_30_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_30_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_30_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_30_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_31_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_31_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_31_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_31_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_31_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_31_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_32_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_32_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_32_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_32_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_32_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_32_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_33_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_33_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_33_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_33_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_33_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_33_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_34_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_34_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_34_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_34_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_34_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_34_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_35_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_35_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_35_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_35_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_35_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_35_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_36_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_36_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_36_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_36_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_36_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_36_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_37_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_37_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_37_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_37_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_37_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_37_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_38_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_38_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_38_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_38_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_38_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_38_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_39_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_39_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_39_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_39_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_39_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_39_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_40_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_40_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_40_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_40_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_40_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_40_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_41_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_41_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_41_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_41_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_41_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_41_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_42_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_42_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_42_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_42_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_42_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_42_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_43_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_43_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_43_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_43_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_43_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_43_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_44_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_44_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_44_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_44_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_44_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_44_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_45_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_45_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_45_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_45_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_45_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_45_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_46_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_46_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_46_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_46_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_46_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_46_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_47_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_47_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_47_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_47_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_47_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_47_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_48_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_48_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_48_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_48_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_48_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_48_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_49_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_49_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_49_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_49_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_49_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_49_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_50_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_50_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_50_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_50_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_50_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_50_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_51_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_51_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_51_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_51_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_51_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_51_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_52_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_52_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_52_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_52_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_52_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_52_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_53_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_53_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_53_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_53_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_53_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_53_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_54_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_54_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_54_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_54_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_54_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_54_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_55_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_55_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_55_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_55_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_55_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_55_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_56_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_56_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_56_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_56_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_56_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_56_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_57_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_57_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_57_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_57_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_57_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_57_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_58_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_58_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_58_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_58_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_58_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_58_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_59_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_59_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_59_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_59_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_59_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_59_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_60_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_60_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_60_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_60_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_60_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_60_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_61_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_61_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_61_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_61_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_61_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_61_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_62_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_62_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_62_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_62_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_62_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_62_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_63_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_63_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_63_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_63_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_63_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_63_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_64_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_64_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_64_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_64_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_64_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_64_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_65_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_65_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_65_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_65_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_65_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_65_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_66_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_66_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_66_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_66_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_66_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_66_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_67_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_67_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_67_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_67_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_67_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_67_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_68_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_68_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_68_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_68_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_68_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_68_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_69_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_69_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_69_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_69_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_69_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_69_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_70_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_70_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_70_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_70_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_70_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_70_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_71_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_71_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_71_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_71_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_71_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_71_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_72_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_72_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_72_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_72_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_72_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_72_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_73_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_73_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_73_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_73_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_73_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_73_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_74_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_74_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_74_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_74_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_74_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_74_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_75_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_75_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_75_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_75_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_75_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_75_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_76_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_76_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_76_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_76_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_76_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_76_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_77_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_77_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_77_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_77_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_77_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_77_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_78_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_78_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_78_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_78_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_78_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_78_OFFSET         0

#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_79_LEN    20
#define SMMU_APB_REG_SMR_OFFSET_ADDR_S_79_OFFSET 12
#define SMMU_APB_REG_SMR_NSCFG_EN_79_LEN         1
#define SMMU_APB_REG_SMR_NSCFG_EN_79_OFFSET      1
#define SMMU_APB_REG_SMR_NSCFG_79_LEN            1
#define SMMU_APB_REG_SMR_NSCFG_79_OFFSET         0

#define SMMU_APB_REG_SMR_RLD_EN0_S_LEN    32
#define SMMU_APB_REG_SMR_RLD_EN0_S_OFFSET 0

#define SMMU_APB_REG_SMR_RLD_EN1_S_LEN    32
#define SMMU_APB_REG_SMR_RLD_EN1_S_OFFSET 0

#define SMMU_APB_REG_SMR_RLD_EN2_S_LEN    16
#define SMMU_APB_REG_SMR_RLD_EN2_S_OFFSET 0

#define SMMU_APB_REG_INTS_PTW_NS_MSK_LEN         1
#define SMMU_APB_REG_INTS_PTW_NS_MSK_OFFSET      5
#define SMMU_APB_REG_INTS_PTW_INVALID_MSK_LEN    1
#define SMMU_APB_REG_INTS_PTW_INVALID_MSK_OFFSET 4
#define SMMU_APB_REG_INTS_PTW_TRANS_MSK_LEN      1
#define SMMU_APB_REG_INTS_PTW_TRANS_MSK_OFFSET   3
#define SMMU_APB_REG_INTS_TLBMISS_MSK_LEN        1
#define SMMU_APB_REG_INTS_TLBMISS_MSK_OFFSET     2
#define SMMU_APB_REG_INTS_EXT_MSK_LEN            1
#define SMMU_APB_REG_INTS_EXT_MSK_OFFSET         1
#define SMMU_APB_REG_INTS_PERMIS_MSK_LEN         1
#define SMMU_APB_REG_INTS_PERMIS_MSK_OFFSET      0

#define SMMU_APB_REG_INTS_PTW_NS_RAW_LEN         1
#define SMMU_APB_REG_INTS_PTW_NS_RAW_OFFSET      5
#define SMMU_APB_REG_INTS_PTW_INVALID_RAW_LEN    1
#define SMMU_APB_REG_INTS_PTW_INVALID_RAW_OFFSET 4
#define SMMU_APB_REG_INTS_PTW_TRANS_RAW_LEN      1
#define SMMU_APB_REG_INTS_PTW_TRANS_RAW_OFFSET   3
#define SMMU_APB_REG_INTS_TLBMISS_RAW_LEN        1
#define SMMU_APB_REG_INTS_TLBMISS_RAW_OFFSET     2
#define SMMU_APB_REG_INTS_EXT_RAW_LEN            1
#define SMMU_APB_REG_INTS_EXT_RAW_OFFSET         1
#define SMMU_APB_REG_INTS_PERMIS_RAW_LEN         1
#define SMMU_APB_REG_INTS_PERMIS_RAW_OFFSET      0

#define SMMU_APB_REG_INTS_PTW_NS_STAT_LEN         1
#define SMMU_APB_REG_INTS_PTW_NS_STAT_OFFSET      5
#define SMMU_APB_REG_INTS_PTW_INVALID_STAT_LEN    1
#define SMMU_APB_REG_INTS_PTW_INVALID_STAT_OFFSET 4
#define SMMU_APB_REG_INTS_PTW_TRANS_STAT_LEN      1
#define SMMU_APB_REG_INTS_PTW_TRANS_STAT_OFFSET   3
#define SMMU_APB_REG_INTS_TLBMISS_STAT_LEN        1
#define SMMU_APB_REG_INTS_TLBMISS_STAT_OFFSET     2
#define SMMU_APB_REG_INTS_EXT_STAT_LEN            1
#define SMMU_APB_REG_INTS_EXT_STAT_OFFSET         1
#define SMMU_APB_REG_INTS_PERMIS_STAT_LEN         1
#define SMMU_APB_REG_INTS_PERMIS_STAT_OFFSET      0

#define SMMU_APB_REG_INTS_PTW_NS_CLR_LEN         1
#define SMMU_APB_REG_INTS_PTW_NS_CLR_OFFSET      5
#define SMMU_APB_REG_INTS_PTW_INVALID_CLR_LEN    1
#define SMMU_APB_REG_INTS_PTW_INVALID_CLR_OFFSET 4
#define SMMU_APB_REG_INTS_PTW_TRANS_CLR_LEN      1
#define SMMU_APB_REG_INTS_PTW_TRANS_CLR_OFFSET   3
#define SMMU_APB_REG_INTS_TLBMISS_CLR_LEN        1
#define SMMU_APB_REG_INTS_TLBMISS_CLR_OFFSET     2
#define SMMU_APB_REG_INTS_EXT_CLR_LEN            1
#define SMMU_APB_REG_INTS_EXT_CLR_OFFSET         1
#define SMMU_APB_REG_INTS_PERMIS_CLR_LEN         1
#define SMMU_APB_REG_INTS_PERMIS_CLR_OFFSET      0

#define SMMU_APB_REG_GLB_NSCFG_LEN    2
#define SMMU_APB_REG_GLB_NSCFG_OFFSET 0

#define SMMU_APB_REG_SCB_BYPASS_LEN    1
#define SMMU_APB_REG_SCB_BYPASS_OFFSET 0

#define SMMU_APB_REG_SCB_TTBR_LEN    32
#define SMMU_APB_REG_SCB_TTBR_OFFSET 0

#define SMMU_APB_REG_SCB_TTBCR_DES_LEN    1
#define SMMU_APB_REG_SCB_TTBCR_DES_OFFSET 0

#define SMMU_APB_REG_OFFSET_ADDR_S_LEN    14
#define SMMU_APB_REG_OFFSET_ADDR_S_OFFSET 0

#define SMMU_APB_REG_FAMA_BPS_MSB_S_LEN     6
#define SMMU_APB_REG_FAMA_BPS_MSB_S_OFFSET  8
#define SMMU_APB_REG_FAMA_CHN_SEL_S_LEN     1
#define SMMU_APB_REG_FAMA_CHN_SEL_S_OFFSET  7
#define SMMU_APB_REG_FAMA_SDES_MSB_S_LEN    7
#define SMMU_APB_REG_FAMA_SDES_MSB_S_OFFSET 0

#define SMMU_APB_REG_FAMA_PTW_MSB_S_LEN    7
#define SMMU_APB_REG_FAMA_PTW_MSB_S_OFFSET 0

#define SMMU_APB_REG_DBG_TLB_EN_LEN    1
#define SMMU_APB_REG_DBG_TLB_EN_OFFSET 0

#define SMMU_APB_REG_DBG_CACHE_EN_LEN    1
#define SMMU_APB_REG_DBG_CACHE_EN_OFFSET 0

#define SMMU_APB_REG_OVERRIDE_VA_SECURITY_LEN    1
#define SMMU_APB_REG_OVERRIDE_VA_SECURITY_OFFSET 1
/*lint -e773*/
#define SCR_DEFAULT     (0 << SMMU_APB_REG_PTW_MID_OFFSET) |      \
                        (1 << SMMU_APB_REG_PTW_PF_OFFSET) |       \
                        (0 << SMMU_APB_REG_SMR_RD_SHADOW_OFFSET) |\
                        (1 << SMMU_APB_REG_CACHE_L2_EN_OFFSET) |  \
                        (1 << SMMU_APB_REG_CACHE_L1_EN_OFFSET) |  \
                        (1 << SMMU_APB_REG_INT_EN_OFFSET) |       \
                        (0 << SMMU_APB_REG_GLB_BYPASS_OFFSET)
#define INTMASK_NS      (1 << SMMU_APB_REG_INTNS_PTW_NS_MSK_OFFSET) |      \
                        (1 << SMMU_APB_REG_INTNS_PTW_INVALID_MSK_OFFSET) | \
                        (1 << SMMU_APB_REG_INTNS_PTW_TRANS_MSK_OFFSET) |   \
                        (1 << SMMU_APB_REG_INTNS_TLBMISS_MSK_OFFSET) |     \
                        (1 << SMMU_APB_REG_INTNS_EXT_MSK_OFFSET) |         \
                        (1 << SMMU_APB_REG_INTNS_PERMIS_MSK_OFFSET)
/*lint +e773*/
#define INTMASK_S       (0x30)
#define SCB_TTBCR_DEFAULT  (0 << SMMU_APB_REG_SCB_TTBCR_DES_OFFSET)
#define RLD_EN0_NS_DEFAULT (0x0)
#define RLD_EN1_NS_DEFAULT (0x0)
#define RLD_EN2_NS_DEFAULT (0x0)
/*lint -e773*/
#define SMRX_NS_DEFAULT (0 << SMMU_APB_REG_SMR_OFFSET_ADDR_0_OFFSET) | \
                        (0 << SMMU_APB_REG_SMR_PTW_QOS_0_OFFSET) |     \
                        (0 << SMMU_APB_REG_SMR_INVLD_EN_0_OFFSET) |    \
                        (0 << SMMU_APB_REG_SMR_BYPASS_0_OFFSET)
/*lint +e773*/

#define SCR_S_DEFAULT   (0x2 << SMMU_APB_REG_GLB_NSCFG_OFFSET)
#define RLD_EN0_S_DEFAULT (0x0)
#define RLD_EN1_S_DEFAULT (0x0)
#define RLD_EN2_S_DEFAULT (0x0)
/*lint -e773*/
#define SMRX_S_DEFAULT  (0 << SMMU_APB_REG_SMR_OFFSET_ADDR_S_0_OFFSET) | \
                        (1 << SMMU_APB_REG_SMR_NSCFG_EN_0_OFFSET) | \
                        (0 << SMMU_APB_REG_SMR_NSCFG_0_OFFSET)

#define SCR_BYPASS      (0 << SMMU_APB_REG_PTW_MID_OFFSET) |      \
                        (0 << SMMU_APB_REG_PTW_PF_OFFSET) |       \
                        (0 << SMMU_APB_REG_SMR_RD_SHADOW_OFFSET) |\
                        (0 << SMMU_APB_REG_CACHE_L2_EN_OFFSET) |  \
                        (0 << SMMU_APB_REG_CACHE_L1_EN_OFFSET) |  \
                        (0 << SMMU_APB_REG_INT_EN_OFFSET) |       \
                        (1 << SMMU_APB_REG_GLB_BYPASS_OFFSET)
/*lint +e773*/
#define RLD_EN0_NS_BYPASS  (0x0)
#define RLD_EN1_NS_BYPASS  (0x0)
#define RLD_EN2_NS_BYPASS  (0x0)
/*lint -e773*/
#define SMRX_NS_BYPASS  (0 << SMMU_APB_REG_SMR_OFFSET_ADDR_0_OFFSET) | \
                        (0 << SMMU_APB_REG_SMR_PTW_QOS_0_OFFSET) |     \
                        (0 << SMMU_APB_REG_SMR_INVLD_EN_0_OFFSET) |    \
                        (1 << SMMU_APB_REG_SMR_BYPASS_0_OFFSET)
/*lint +e773*/

#define SCR_S_BYPASS    (0x2 << SMMU_APB_REG_GLB_NSCFG_OFFSET)
#define RLD_EN0_S_BYPASS  (0x0)
#define RLD_EN1_S_BYPASS  (0x0)
#define RLD_EN2_S_BYPASS  (0x0)
/*lint -e773*/
#define SMRX_S_BYPASS   (0 << SMMU_APB_REG_SMR_OFFSET_ADDR_S_0_OFFSET) | \
                        (0 << SMMU_APB_REG_SMR_NSCFG_EN_0_OFFSET) | \
                        (0 << SMMU_APB_REG_SMR_NSCFG_0_OFFSET)
/*lint +e773*/

#define SCR_S_BYPASS    (0x2 << SMMU_APB_REG_GLB_NSCFG_OFFSET)
#define RLD_EN0_S_BYPASS  (0x0)
#define RLD_EN1_S_BYPASS  (0x0)
#define RLD_EN2_S_BYPASS  (0x0)

struct hisp_cvdr_device {
    int ispsmmu_init_byap;
    u64 pgd_base;
} hisp_cvdr_dev;

struct hisi_ispmmu_regs_s {
    unsigned int offset;
    unsigned int data;
    int num;
};

struct hisi_ispmmu_regs_s ispmmu_up_regs[] = {
    {SMMU_APB_REG_SMMU_SCR_REG, SCR_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_INTMASK_NS_REG, INTMASK_NS, 1},
    {SMMU_APB_REG_SMMU_INTMAS_S_REG, INTMASK_S, 1},
    {SMMU_APB_REG_SMMU_CB_TTBR0_REG, PGD_BASE, 1},
    {SMMU_APB_REG_SMMU_RLD_EN0_NS_REG, RLD_EN0_NS_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_RLD_EN1_NS_REG, RLD_EN1_NS_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_RLD_EN2_NS_REG, RLD_EN2_NS_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_SMRX_NS_0_REG, SMRX_NS_DEFAULT, 80},
    {SMMU_APB_REG_SMMU_SCR_S_REG, SCR_S_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_RLD_EN0_S_REG, RLD_EN0_S_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_RLD_EN1_S_REG, RLD_EN1_S_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_RLD_EN2_S_REG, RLD_EN2_S_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_SCB_TTBCR_REG, SCB_TTBCR_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_SCB_TTBR_REG, PGD_BASE, 1},
    {SMMU_APB_REG_SMMU_SMRX_S_0_REG, SMRX_NS_DEFAULT, 80},
};

int ispcvdr_init(void)
{
    void __iomem *isp_regs_base;
    void __iomem *isprt_base;
    void __iomem *ispsrt_base;
    void __iomem *ispsctrl_base;

    pr_info("[%s] +\n", __func__);

    isp_regs_base = get_regaddr_by_pa(ISPCORE);
    if (unlikely(!isp_regs_base)) {
        pr_err("[%s] isp_regs_base failed!\n", __func__);
        return -ENOMEM;
    }

    isprt_base = isp_regs_base + ISP_BASE_ADDR_CVDR_RT;
    ispsrt_base = isp_regs_base + ISP_BASE_ADDR_CVDR_SRT;
    ispsctrl_base = isp_regs_base + ISP_SUB_CTRL_BASE_ADDR;

    writel((readl(ISP_020010_MODULE_CGR_TOP + isp_regs_base) | CVDR_CLK_ENABLE),
            ISP_020010_MODULE_CGR_TOP + isp_regs_base);

    /* CVDR RT*/
    writel(0x0B0B4001, isprt_base + CVDR_RT_CVDR_CFG_REG);
    /* CVDR SRT*/
    writel(0x0B132201, ispsrt_base + CVDR_SRT_CVDR_CFG_REG);
    //CVDR_RT QOS
    writel(0xF8765432, isprt_base + CVDR_RT_CVDR_WR_QOS_CFG_REG);
    writel(0xF8122334, isprt_base + CVDR_RT_CVDR_RD_QOS_CFG_REG);

    //CVDR_SRT QOS
    writel(0xD0765432, ispsrt_base + CVDR_SRT_CVDR_WR_QOS_CFG_REG);
    writel(0xD0122334, ispsrt_base + CVDR_SRT_CVDR_RD_QOS_CFG_REG);

    //CVDR SRT PORT STOP
    writel(0x00020000, ispsrt_base + CVDR_SRT_NR_RD_CFG_3_REG);
    writel(0x00020000, ispsrt_base + CVDR_SRT_VP_WR_IF_CFG_10_REG);
    writel(0x00020000, ispsrt_base + CVDR_SRT_VP_WR_IF_CFG_11_REG);

    //JPGENC limiter DU=8
    writel(0x00000000, ispsrt_base + CVDR_SRT_VP_WR_IF_CFG_25_REG);
    writel(0x80060100, ispsrt_base + CVDR_SRT_NR_RD_CFG_1_REG);
    writel(0x0F001111, ispsrt_base + CVDR_SRT_LIMITER_NR_RD_1_REG);

    //SRT READ
    writel(0x00026E10, ispsctrl_base + SUB_CTRL_ISP_FLUX_CTRL2_0_REG);
    writel(0x0000021F, ispsctrl_base + SUB_CTRL_ISP_FLUX_CTRL2_1_REG);
    //SRT WRITE
    writel(0x00027210, ispsctrl_base + SUB_CTRL_ISP_FLUX_CTRL3_0_REG);
    writel(0x0000024E, ispsctrl_base + SUB_CTRL_ISP_FLUX_CTRL3_1_REG);

    pr_info("[%s] -\n", __func__);

    return 0;
}

//lint -save -e529 -e438
int ispmmu_init(void)
{
    struct hisp_cvdr_device *dev = &hisp_cvdr_dev;
    int i = 0, num = 0;
    void __iomem *smmu_base;
    u64 pgt_addr = dev->pgd_base;
    void __iomem *isp_regs_base;

    pr_info("[%s] +\n", __func__);

    isp_regs_base = get_regaddr_by_pa(ISPCORE);
    if (unlikely(!isp_regs_base)) {
        pr_err("[%s] isp_regs_base failed\n", __func__);
        return -ENOMEM;
    }

    writel((readl(ISP_020010_MODULE_CGR_TOP + isp_regs_base) | SMMU_CLK_ENABLE),
            ISP_020010_MODULE_CGR_TOP + isp_regs_base);
    if(dev->ispsmmu_init_byap){
    smmu_base = isp_regs_base + ISP_BASE_ADDR_SSMMU;
    for (i = 0; i < sizeof(ispmmu_up_regs) / sizeof(struct hisi_ispmmu_regs_s); i++) {/*lint !e574 */
        for (num = 0; num < ispmmu_up_regs[i].num; num++) {
            writel(ispmmu_up_regs[i].data, smmu_base + ispmmu_up_regs[i].offset + num * 4);
        }
    }
    writel(pgt_addr, SMMU_APB_REG_SMMU_CB_TTBR0_REG + smmu_base);
    writel(pgt_addr, SMMU_APB_REG_SMMU_SCB_TTBR_REG + smmu_base);
    writel(0x1, SMMU_APB_REG_SMMU_SCB_TTBCR_REG + smmu_base);
    writel(0x1, SMMU_APB_REG_SMMU_CB_TTBCR_REG + smmu_base);
    }
    /*set jpeg for nosec*/
    //writel(0x6,ISP_BASE_ADDR_SUB_CTRL + ISP_CORE_CTRL_S);
    //writel(0x8,ISP_SUB_CTRL_S + ISP_BASE_ADDR_SUB_CTRL);

    /*set mid*/
    writel(MID_FOR_JPGEN, isp_regs_base + ISP_BASE_ADDR_CVDR_SRT + CVDR_SRT_AXI_CFG_VP_WR_25);
    writel(MID_FOR_JPGEN, isp_regs_base + ISP_BASE_ADDR_CVDR_SRT + CVDR_SRT_AXI_CFG_NR_RD_1);

    pr_info("[%s] -\n", __func__);

    return 0;
}
//lint -restore
int ispmmu_exit(void)
{
    pr_info("[%s] +\n", __func__);

    return 0;
}

int hisi_isp_cvdr_getdts(struct device_node *np)
{
    int ret;
    struct hisp_cvdr_device *dev = &hisp_cvdr_dev;

    if (!np) {
        pr_err("[%s] Failed : np.%pK\n", __func__, np);
        return -ENODEV;
    }

    pr_info("[%s] +\n", __func__);
    if ((ret = of_property_read_u32(np, "ispsmmu-init-byap", (unsigned int *)(&dev->ispsmmu_init_byap))) < 0 ) {
        pr_err("[%s] Failed: ispsmmu-init-byap of_property_read_u32.%d\n", __func__, ret);
        return -EINVAL;
    }
    pr_info("[%s] isp-smmu-flag.0x%x\n", __func__, dev->ispsmmu_init_byap);
    pr_info("[%s] -\n", __func__);

    return 0;
}

int hisi_isp_set_pgd(void)
{
    int ret;

    if ((ret = hisp_get_pgd_base(&hisp_cvdr_dev.pgd_base)) < 0) {
        pr_err("[%s] Failed : hisp_get_pgd_base.%d\n", __func__, ret);
        return ret;
    }

    return 0;
}

struct hisp_mdc_device {
    int isp_mdc_flag;
    struct hisi_isp_fstcma_mdc_s *mdc_fstcma;
} hisp_mdc_dev;

void free_mdc_ion(unsigned int size)
{
    struct hisp_mdc_device *dev = &hisp_mdc_dev;

    struct hisi_isp_fstcma_mdc_s *hisi_mdc_fstcma = dev->mdc_fstcma;
    if(hisi_mdc_fstcma == NULL){
        return;
    }
    hisi_fstcma_free(hisi_mdc_fstcma->mdc_virt_addr,hisi_mdc_fstcma->mdc_dma_addr,MEM_MDC_SIZE);
    kfree(hisi_mdc_fstcma);
    dev->mdc_fstcma = NULL;
}

void *get_mdc_addr_va(void)
{
    struct hisp_mdc_device *dev = &hisp_mdc_dev;

    struct hisi_isp_fstcma_mdc_s *hisi_mdc_fstcma = NULL;
    if(dev->mdc_fstcma == NULL)
    {
        pr_err("[%s], NONE mdc_mem_info!\n", __func__);
        return 0;
    }
    pr_debug("get_mdc_addr_va FASTCMA_MDC_DEBUG!\n");
    hisi_mdc_fstcma = dev->mdc_fstcma;/*lint !e838 */
    if(hisi_mdc_fstcma->mdc_virt_addr == NULL)
    {
        pr_err("[%s], NONE hisi_mdc_fstcma mdc_virt_addr!\n", __func__);
        return 0;
    }
    return hisi_mdc_fstcma->mdc_virt_addr;
}

u64 get_mdc_addr_pa(void)
{
    struct hisp_mdc_device *dev = &hisp_mdc_dev;

    struct hisi_isp_fstcma_mdc_s *hisi_mdc_fstcma = NULL;
    if(dev->mdc_fstcma == NULL)
    {
        pr_err("[%s], NONE mdc_mem_info!\n", __func__);
        return 0;
    }
    pr_debug("get_mdc_addr_pa FASTCMA_MDC_DEBUG!\n");
    hisi_mdc_fstcma = dev->mdc_fstcma;/*lint !e838 */
    if(hisi_mdc_fstcma->mdc_dma_addr == 0x0)
    {
        pr_err("[%s], NONE hisi_mdc_fstcma mdc_dma_addr!\n", __func__);
        return 0;
    }
    return hisi_mdc_fstcma->mdc_dma_addr;
}

struct hisi_isp_fstcma_mdc_s *get_fstcma_mdc(unsigned int size)
{
    struct hisi_isp_fstcma_mdc_s *fstcma_mdc = NULL;

    fstcma_mdc = kzalloc(sizeof(struct hisi_isp_fstcma_mdc_s), GFP_KERNEL);/*lint !e838 */
    if (!fstcma_mdc) {
        pr_err("in %s fstcma_mdc kzalloc is failed\n", __func__);
        return NULL;
    }

    if ((fstcma_mdc->mdc_virt_addr = hisi_fstcma_alloc(&fstcma_mdc->mdc_dma_addr, size, GFP_KERNEL)) == NULL) {
        pr_err("[%s] mdc_virt_addr.%pK\n", __func__, fstcma_mdc->mdc_virt_addr);
        kfree(fstcma_mdc);
        return NULL;
    }

    return fstcma_mdc;
}

int mdc_addr_pa_init(void)
{
    struct hisp_mdc_device *dev = &hisp_mdc_dev;

    pr_debug("mdc_addr_pa_init FASTCMA_MDC_DEBUG!\n");
    dev->mdc_fstcma = get_fstcma_mdc(MEM_MDC_SIZE);
    if(dev->mdc_fstcma == NULL){
        pr_err("get_fstcma_mdc failed!\n");
        return -EINVAL;
    }
    return 0;
}

void hisp_mdc_dev_init(void)
{
        hisp_mdc_dev.mdc_fstcma = NULL;
}

int hisi_isp_mdc_getdts(struct device_node *np)
{
    int ret;
    struct hisp_mdc_device *dev = &hisp_mdc_dev;

    if (!np) {
        pr_err("[%s] Failed : np.%pK\n", __func__, np);
        return -ENODEV;
    }

    pr_info("[%s] +\n", __func__);
    if ((ret = of_property_read_u32(np, "isp-mdc-flag", (unsigned int *)(&dev->isp_mdc_flag))) < 0 ) {
        pr_err("[%s] Failed: isp-mdc-flag of_property_read_u32.%d\n", __func__, ret);
        return -EINVAL;
    }
    pr_info("[%s] isp_mdc_flag.0x%x\n", __func__, dev->isp_mdc_flag);
    pr_info("[%s] -\n", __func__);

    return 0;
}

//lint -save -e529 -e438
int get_isp_mdc_flag(void)
{
    struct hisp_mdc_device *dev = &hisp_mdc_dev;

    if(dev != NULL)
        return dev->isp_mdc_flag;

    return 0;
}
//lint -restore

