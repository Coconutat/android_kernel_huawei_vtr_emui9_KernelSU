/* Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
#ifndef SOC_JPU_INTERFACE_H
#define SOC_JPU_INTERFACE_H


/*******************************************************************************
**
*/
#define JPU_LB_SIZE	(128 * SZ_1K)


/*******************************************************************************
** SMMU
*/
#define JPU_SMMU_OFFSET	(0x1F000)

#define SMMU_SCR	(0x0000)
#define SMMU_MEMCTRL	(0x0004)
#define SMMU_LP_CTRL	(0x0008)
#define SMMU_PRESS_REMAP	(0x000C)
#define SMMU_INTMASK_NS	(0x0010)
#define SMMU_INTRAW_NS	(0x0014)
#define SMMU_INTSTAT_NS	(0x0018)
#define SMMU_INTCLR_NS	(0x001C)
/*(0x0020+n*0x4)*/
#define SMMU_SMRx_NS	(0x0020)
#define SMMU_RLD_EN0_NS	(0x01F0)
#define SMMU_RLD_EN1_NS	(0x01F4)
#define SMMU_RLD_EN2_NS	(0x01F8)
#define SMMU_CB_SCTRL	(0x0200)
#define SMMU_CB_TTBR0	(0x0204)
#define SMMU_CB_TTBR1	(0x0208)
#define SMMU_CB_TTBCR	(0x020C)
#define SMMU_OFFSET_ADDR_NS	(0x0210)
#define SMMU_SCACHEI_ALL	(0x0214)
#define SMMU_SCACHEI_L1	(0x0218)
#define SMMU_SCACHEI_L2L3	(0x021C)
#define SMMU_FAMA_CTRL0	(0x0220)
#define SMMU_FAMA_CTRL1	(0x0224)
#define SMMU_ADDR_MSB	(0x0300)
#define SMMU_ERR_RDADDR	(0x0304)
#define SMMU_ERR_WRADDR	(0x0308)
#define SMMU_FAULT_ADDR_TCU (0x0310)
#define SMMU_FAULT_ID_TCU	(0x0314)
/*(0x0320+n*0x10)*/
#define SMMU_FAULT_ADDR_TBUx	(0x0320)
#define SMMU_FAULT_ID_TBUx	(0x0324)
#define SMMU_FAULT_INFOx	(0x0328)
#define SMMU_DBGRPTR_TLB	(0x0380)
#define SMMU_DBGRDATA_TLB	(0x0380)
#define SMMU_DBGRDATA0_CACHE	(0x038C)
#define SMMU_DBGRDATA1_CACHE	(0x0390)
#define SMMU_DBGAXI_CTRL	(0x0394)
#define SMMU_OVA_ADDR	(0x0398)
#define SMMU_OPA_ADDR	(0x039C)
#define SMMU_OVA_CTRL	(0x03A0)
#define SMMU_OPREF_ADDR	(0x03A4)
#define SMMU_OPREF_CTRL	(0x03A8)
#define SMMU_OPREF_CNT	(0x03AC)
/*(0x0500+n*0x4)*/
#define SMMU_SMRx_S	(0x0500)
#define SMMU_RLD_EN0_S	(0x06F0)
#define SMMU_RLD_EN1_S	(0x06F4)
#define SMMU_RLD_EN2_S	(0x06F8)
#define SMMU_INTMAS_S	(0x0700)
#define SMMU_INTRAW_S	(0x0704)
#define SMMU_INTSTAT_S	(0x0708)
#define SMMU_INTCLR_S	(0x070C)
#define SMMU_SCR_S	(0x0710)
#define SMMU_SCB_SCTRL	(0x0714)
#define SMMU_SCB_TTBR	(0x0718)
#define SMMU_SCB_TTBCR	(0x071C)
#define SMMU_OFFSET_ADDR_S	(0x0720)
#define SMMU_SMRX_P	(0x10000)
#define SMMU_SCR_P	(0x10210)


#endif  /* SOC_JPU_INTERFACE_H */
