

#ifndef __OAL_PCI_IF_H__
#define __OAL_PCI_IF_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

#include "oal_types.h"
#include "oal_util.h"
#include "oal_hardware.h"
#include "arch/oal_pci_if.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_PCI_IF_H

/*****************************************************************************
  2 枚举定义
*****************************************************************************/
/*****************************************************************************
  枚举名  : oal_pci_bar_idx_enum_uint8
  协议表格:
  枚举说明:
*****************************************************************************/
typedef enum
{
    OAL_PCI_BAR_0 = 0,
    OAL_PCI_BAR_1,
    OAL_PCI_BAR_2,
    OAL_PCI_BAR_3,
    OAL_PCI_BAR_4,
    OAL_PCI_BAR_5,

    OAL_PCI_BAR_BUTT
}oal_pci_bar_idx_enum;
typedef oal_uint8 oal_pci_bar_idx_enum_uint8;

typedef enum
{
    OAL_PCI_GEN_1_0 = 0,
    OAL_PCI_GEN_2_0,
    OAL_PCI_GEN_3_0,

    OAL_PCI_GEN_BUTT
}oal_pci_gen_enum;
typedef oal_uint8 oal_pci_gen_enum_uint8;


/*****************************************************************************
  3 全局变量声明
*****************************************************************************/
extern oal_pci_bar_idx_enum_uint8 g_en_bar_table[1];


/*****************************************************************************
  4 宏定义
*****************************************************************************/
#define OAL_WITP_MAX_BARS   OAL_ARRAY_SIZE(g_en_bar_table)

/*
1151 SOC+PHY+MAC: 0x20000000~0x2003bfff
PCIE internal:    0x0~0x3ff*/
#define OAL_HI1151_SOC_ADDR_LIMIT    (0x3C000)

#if(_PRE_TARGET_PRODUCT_TYPE_E5 == _PRE_CONFIG_TARGET_PRODUCT)
#define OAL_PCIE_SYS_BASE_PHYS     (0x90100000)       /* SYSCTRL */
#ifdef _PRE_E5_750_PLATFORM
#define OAL_DBI_BASE_ADDR_0        (0x98000000)
#else
#define OAL_DBI_BASE_ADDR_0        (0x40000000)
#endif
//pcie1暂时不使用，定义为750平台形式，不考虑区分平台
#define OAL_DBI_BASE_ADDR_1        (0xA8010000)

#define OAL_PERI_W_PCIE0         (0xC28)
#define OAL_PERI_R_PCIE0         (0xC2C)
#define OAL_PERI_W_PCIE1         (0xC88)
#define OAL_PERI_R_PCIE1         (0xC8C)

#define OAL_PCIE_LINK_STATUS_OFFSET   (0xE38)
#ifdef _PRE_E5_750_PLATFORM
#define OAL_PCIE_TARGET_ADDR         (0x40000000)
#else
#define OAL_PCIE_TARGET_ADDR         (0xA0000000)
#endif

#else   /* 主要用作5610平台使用 */

#define OAL_PCIE_CRG_BASE        (0x14880000)        /* cfg_base */
#define OAL_PCIE_SYS_BASE_PHYS   (0x10100000)        /* SYSCTRL */
#define OAL_DBI_BASE_ADDR_0      (0x10A00000)        /* IO空间 64k */
#define OAL_DBI_BASE_ADDR_1      (0x10A02000)        /* IO空间 64k */
#define OAL_PERI_W_PCIE0	     (0xBC)
#define OAL_PERI_R_PCIE0         (0xC0)
#define OAL_PERI_W_PCIE1         (0xEC)
#define OAL_PERI_R_PCIE1         (0xF0)

#define OAL_PCIE_LINK_STATUS_OFFSET   (0x18)
#define OAL_PCIE_TARGET_ADDR         (0x80000000)
#endif

#define OAL_PCIE_NONFATAL_ERR_CNT           200     /* pcie err_nonfatal周期错误次数阈值 */
#define OAL_PCIE_NONFATAL_ERR_TIMEOUT_MS    1000    /* pcie err_nonfatal每1000ms到期 */
/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
/*****************************************************************************
  结构名  : oal_pcie_iatu_stru
  结构说明: Internal Address Translation Unit(iATU)结构体
*****************************************************************************/
typedef struct
{
    oal_uint32    ul_viewport;          /* iATU Viewport Register                    */
	oal_uint32    ul_region_ctrl_1;     /* iATU Region Control 1 Register            */
	oal_uint32    ul_region_ctrl_2;     /* iATU Region Control 2 Register            */
	oal_uint32    ul_lbar;              /* iATU Region Lower Base Address Register   */
	oal_uint32    ul_ubar;              /* iATU Region Upper Base Address Register   */
	oal_uint32    ul_lar;               /* iATU Region Limit Address Register        */
	oal_uint32    ul_ltar;		        /* iATU Region Lower Target Address Register */
	oal_uint32    ul_utar;              /* iATU Region Upper Target Address Register */
} oal_pcie_iatu_stru;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_void oal_pcie_set_inbound_iatu_slip_window(oal_pci_dev_stru *pst_pci_dev);
extern oal_void oal_pcie_set_inbound_iatu(oal_pci_dev_stru *pst_pci_dev);
extern oal_void  oal_pcie_set_outbound_iatu(oal_pci_dev_stru *pst_pci_dev);
extern oal_uint32  oal_pci_ep_reconfig(oal_pci_dev_stru *pst_pci_dev);
extern oal_uint32  oal_pci_rc_reconfig(oal_uint8 uc_chip_id);
extern oal_void  oal_pci_exit(oal_void);
extern oal_uint32  oal_pci_init(oal_void);
extern oal_uint32  oal_5115_pci_init(oal_void);
extern oal_void oal_5115_pci_exit(oal_void);
extern oal_uint32  oal_pci_check_clear_error_nonfatal(oal_uint8 uc_chip_id);
extern oal_uint32  oal_pci_hand_reset(oal_uint8 uc_chip_id);
extern oal_void    oal_machine_restart(oal_void);
extern oal_void oal_pcie_set_gen(oal_uint32 id,oal_pci_gen_enum_uint8 uc_gen);
extern oal_void oal_pcie_dbi_enable(oal_uint32 id);
extern oal_void oal_pcie_dbi_disable(oal_uint32 id);
extern oal_void oal_pcie_show_link_status(oal_uint32 id);
extern oal_uint32 oal_pcie_internal_get(oal_uint32 ul_inter_addr, oal_uint32 bit_offset, oal_uint32 bit_width);
extern oal_void oal_pcie_internal_set(oal_uint32 ul_inter_addr, oal_uint32 bit_offset, oal_uint32 bit_width, oal_uint32 bit_value);
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of oal_pci_if.h */
