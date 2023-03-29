
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#define  HI11XX_LOG_MODULE_NAME "[PCIE_H]"
#define  HISI_LOG_TAG "[PCIE]"
#include "oal_util.h"

#include "oal_net.h"
#include "oal_ext_if.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/kthread.h>

#include "oal_thread.h"
#include "oam_ext_if.h"

#include "oal_pcie_host.h"
#include "oal_pcie_linux.h"
#include "oal_pcie_reg.h"
#include "oal_pci_if.h"

#include "oal_pcie_comm.h"
#include "oal_hcc_host_if.h"
#include "oal_kernel_file.h"

#include "oal_pcie_pm.h"
#include "plat_firmware.h"
#include "plat_pm_wlan.h"
#include "board.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_PCIE_HOST_C

#define PCIE_MEM_SCAN_VERFIY_VAL_1  (0x5a)
#define PCIE_MEM_SCAN_VERFIY_VAL_2  (0xa5)

OAL_STATIC oal_pcie_res* g_pci_res = NULL;

OAL_STATIC oal_kobject* g_conn_syfs_pci_object = NULL;

char* g_pcie_link_state_str[PCI_WLAN_LINK_BUTT+1] =
{
    [PCI_WLAN_LINK_DOWN] = "linkdown",
    [PCI_WLAN_LINK_DEEPSLEEP] = "deepsleep",
    [PCI_WLAN_LINK_UP] = "linkup",
    [PCI_WLAN_LINK_MEM_UP] = "memup",
    [PCI_WLAN_LINK_RES_UP] = "resup",
    [PCI_WLAN_LINK_WORK_UP] = "workup",
    [PCI_WLAN_LINK_BUTT] = "butt"
};

/*
 * 2 Global Variable Definition
 */
OAL_STATIC oal_pcie_bar_info g_en_bar_table[] =
{
    /*1103 4.7a 一个BAR [8MB]， 5.0a 为两个BAR[Bar0 8M  BAR1 16KB]
      (因为1103 是64bit bar,所以对应bar index寄存器, 是对应bar index=2,
       参考 __pci_read_base 最后一行),
      第二个BAR 直接用MEM 方式 访问IATU表*/
    {
        .bar_idx = OAL_PCI_BAR_0,
    },
};

oal_int32 hipcie_loglevel = PCI_LOG_INFO;  /**/
module_param(hipcie_loglevel, int, S_IRUGO | S_IWUSR);

oal_int32 pcie_soft_fifo_enable = 0;
module_param(pcie_soft_fifo_enable, int, S_IRUGO | S_IWUSR);

oal_int32 pcie_ringbuf_bugfix_enable = 1;
module_param(pcie_ringbuf_bugfix_enable, int, S_IRUGO | S_IWUSR);

oal_int32 pcie_dma_data_check_enable = 0;/*Wi-Fi关闭时可以修改此标记*/
module_param(pcie_dma_data_check_enable, int, S_IRUGO | S_IWUSR);

oal_int32 ft_pcie_wcpu_max_freq_bypass = 0;
module_param(ft_pcie_wcpu_max_freq_bypass, int, S_IRUGO | S_IWUSR);

oal_int32 ft_pcie_link_err_bypass = 0;
module_param(ft_pcie_link_err_bypass, int, S_IRUGO | S_IWUSR);

oal_int32 ft_pcie_wcpu_mem_check_times = 1;
module_param(ft_pcie_wcpu_mem_check_times, int, S_IRUGO | S_IWUSR);

oal_int32 ft_pcie_perf_runtime = 200;
module_param(ft_pcie_perf_runtime, int, S_IRUGO | S_IWUSR);

oal_int32 ft_pcie_perf_wr_bypass = 0;
module_param(ft_pcie_perf_wr_bypass, int, S_IRUGO | S_IWUSR);

oal_int32 ft_pcie_perf_rd_bypass = 1;
module_param(ft_pcie_perf_rd_bypass, int, S_IRUGO | S_IWUSR);

oal_int32 ft_pcie_wcpu_mem_check_burst_check = 0;
module_param(ft_pcie_wcpu_mem_check_burst_check, int, S_IRUGO | S_IWUSR);

oal_int32 ft_pcie_wcpu_mem_check_byword_bypass = 1;
module_param(ft_pcie_wcpu_mem_check_byword_bypass, int, S_IRUGO | S_IWUSR);

oal_int32 ft_pcie_write_address_bypass = 0;
module_param(ft_pcie_write_address_bypass, int, S_IRUGO | S_IWUSR);

oal_int32 pcie_ldo_phy_0v9_param = 0;
module_param(pcie_ldo_phy_0v9_param, int, S_IRUGO | S_IWUSR);

oal_int32 pcie_ldo_phy_1v8_param = 0;
module_param(pcie_ldo_phy_1v8_param, int, S_IRUGO | S_IWUSR);

/*0 memcopy from kernel ,1 memcopy from self*/
oal_int32 pcie_memcopy_type = 0;
module_param(pcie_memcopy_type, int, S_IRUGO | S_IWUSR);
EXPORT_SYMBOL_GPL(pcie_memcopy_type);

char* g_pci_loglevel_format[] = {
    KERN_ERR"[PCIE][ERR] ",
    KERN_WARNING"[PCIE][WARN]",
    KERN_DEBUG"[PCIE][INFO]",
    KERN_DEBUG"[PCIE][DBG] ",
};

/*factory test*/
/*mpw2*/
oal_ulong g_mpw2_wmem_scan_array[][2] =
{
    {0x00004000 , 0x0008BFFF},/*itcm ram*/
    {0x20000000 , 0x20067FFF},/*dtcm*/
    {0x60000000 , 0x6008FFFF},/*share mem*/
};

/*pilot*/
oal_ulong g_pilot_wmem_scan_array[][2] =
{
    {0x00040000 , 0x000A7FFF},/*itcm ram*/
    {0x20018000 , 0x2007FFFF},/*dtcm*/
    {0x60000000 , 0x6007FFFF},/*share mem*/
};

oal_ulong g_mpw2_bmem_scan_array[][2] =
{
    {0x80000000 , 0x800FFFFF},/*itcm*/
    {0x80100000 , 0x801DFFFF},/*dtcm*/
};

/*pilot*/
oal_ulong g_pilot_bmem_scan_array[][2] =
{
    {0x80040000 , 0x8010FFFF},/*itcm*/
    {0x80200000 , 0x8030FFFF},/*dtcm*/
};

typedef enum _HI1103_REGIONS_
{
    HI1103_REGION_WCPU_ITCM ,
    HI1103_REGION_WCPU_DTCM ,
    HI1103_REGION_W_PKTRAM ,
    HI1103_REGION_W_PERP_AHB ,
    HI1103_REGION_BCPU_ITCM ,
    HI1103_REGION_BCPU_DTCM ,
    HI1103_REGION_B_PERP_AHB ,
    HI1103_REGION_AON_APB ,
    HI1103_REGION_BUTT
}HI1103_REGIONS;

 /*Region大小必须为4KB的倍数，iATU要求*/
 /*这里的分段都是对应iATU inbound*/
OAL_STATIC oal_pcie_region g_hi1103_pcie_mpw2_regions[] =
{
    {
        .pci_start = 0x00000000,
        .pci_end   = 0x0008BFFF,
        .cpu_start = 0x00000000,
        .cpu_end   = 0x0008BFFF,/*560KB*/
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        .flag      = OAL_IORESOURCE_MEM,
#else
        .flag      = OAL_IORESOURCE_REG,
#endif
        .name      = OAL_PCIE_TO_NAME(HI1103_REGION_WCPU_ITCM)
    },
    {
        .pci_start = 0x20000000,
        .pci_end   = 0x20067FFF,
        .cpu_start = 0x20000000,
        .cpu_end   = 0x20067FFF,/*416KB*/
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        .flag      = OAL_IORESOURCE_MEM,
#else
        .flag      = OAL_IORESOURCE_REG,
#endif
        .name      = OAL_PCIE_TO_NAME(HI1103_REGION_WCPU_DTCM)
    },
    {
        .pci_start = 0x60000000,
        .pci_end   = 0x6008FFFF,
        .cpu_start = 0x60000000,
        .cpu_end   = 0x6008FFFF,/*576KB*/
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        .flag      = OAL_IORESOURCE_MEM,
#else
        .flag      = OAL_IORESOURCE_REG,
#endif
        .name      = OAL_PCIE_TO_NAME(HI1103_REGION_W_PKTRAM)
    },
    {
        .pci_start = 0x40000000,
        .pci_end   = 0x40102FFF,
        .cpu_start = 0x40000000,
        .cpu_end   = 0x40102FFF,/*1036KB*/
        .flag      = OAL_IORESOURCE_REG,
        .name      = OAL_PCIE_TO_NAME(HI1103_REGION_W_PERP_AHB)
    },
    {
        .pci_start = 0x80000000,
        .pci_end   = 0x800FFFFF,
        .cpu_start = 0x80000000,
        .cpu_end   = 0x800FFFFF,/*1024KB*/
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        .flag      = OAL_IORESOURCE_MEM,
#else
        .flag      = OAL_IORESOURCE_REG,
#endif
        .name      = OAL_PCIE_TO_NAME(HI1103_REGION_BCPU_ITCM)
    },
    {
        .pci_start = 0x80100000,
        .pci_end   = 0x801DFFFF,
        .cpu_start = 0x80100000,
        .cpu_end   = 0x801DFFFF,/*896KB*/
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        .flag      = OAL_IORESOURCE_MEM,
#else
        .flag      = OAL_IORESOURCE_REG,
#endif
        .name      = OAL_PCIE_TO_NAME(HI1103_REGION_BCPU_DTCM)
    },
    {
        .pci_start = 0x48000000,
        .pci_end   = 0x48122FFF,
        .cpu_start = 0x48000000,
        .cpu_end   = 0x48122FFF,/*1164KB*/
        .flag      = OAL_IORESOURCE_REG,
        .name      = OAL_PCIE_TO_NAME(HI1103_REGION_B_PERP_AHB)
    },
    {
        .pci_start = 0x50000000,
        .pci_end   = 0x5000EDFF,
        .cpu_start = 0x50000000,
        .cpu_end   = 0x5000EDFF,/*59KB*/
        .flag      = OAL_IORESOURCE_REG,
        .name      = OAL_PCIE_TO_NAME(HI1103_REGION_AON_APB)
    },
};

OAL_STATIC oal_pcie_region g_hi1103_pcie_pilot_regions[] =
{
    {
        .pci_start = 0x00000000,
        .pci_end   = 0x000A7FFF,
        .cpu_start = 0x00000000,
        .cpu_end   = 0x000A7FFF,/*592KB*/
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        .flag      = OAL_IORESOURCE_MEM,
#else
        .flag      = OAL_IORESOURCE_REG,
#endif
        .name      = OAL_PCIE_TO_NAME(HI1103_REGION_WCPU_ITCM)
    },
    {
        .pci_start = 0x20018000,
        .pci_end   = 0x2007FFFF,
        .cpu_start = 0x20018000,
        .cpu_end   = 0x2007FFFF,/*416KB*/
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        .flag      = OAL_IORESOURCE_MEM,
#else
        .flag      = OAL_IORESOURCE_REG,
#endif
        .name      = OAL_PCIE_TO_NAME(HI1103_REGION_WCPU_DTCM)
    },
    {
        .pci_start = 0x60000000,
        .pci_end   = 0x6007FFFF,
        .cpu_start = 0x60000000,
        .cpu_end   = 0x6008FFFF,/*512KB*/
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        .flag      = OAL_IORESOURCE_MEM,
#else
        .flag      = OAL_IORESOURCE_REG,
#endif
        .name      = OAL_PCIE_TO_NAME(HI1103_REGION_W_PKTRAM)
    },
    {
        .pci_start = 0x40000000,
        .pci_end   = 0x40107FFF,
        .cpu_start = 0x40000000,
        .cpu_end   = 0x40107FFF,
        .flag      = OAL_IORESOURCE_REG,
        .name      = OAL_PCIE_TO_NAME(HI1103_REGION_W_PERP_AHB)
    },
    {
        .pci_start = 0x80040000,
        .pci_end   = 0x8010FFFF,
        .cpu_start = 0x80040000,
        .cpu_end   = 0x8010FFFF,/*832KB*/
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        .flag      = OAL_IORESOURCE_MEM,
#else
        .flag      = OAL_IORESOURCE_REG,
#endif
        .name      = OAL_PCIE_TO_NAME(HI1103_REGION_BCPU_ITCM)
    },
    {
        .pci_start = 0x80200000,
        .pci_end   = 0x8030FFFF,
        .cpu_start = 0x80200000,
        .cpu_end   = 0x8030FFFF,/*1088KB*/
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        .flag      = OAL_IORESOURCE_MEM,
#else
        .flag      = OAL_IORESOURCE_REG,
#endif
        .name      = OAL_PCIE_TO_NAME(HI1103_REGION_BCPU_DTCM)
    },
    {
        .pci_start = 0x48000000,
        .pci_end   = 0x48122FFF,
        .cpu_start = 0x48000000,
        .cpu_end   = 0x48122FFF,/*1164KB*/
        .flag      = OAL_IORESOURCE_REG,
        .name      = OAL_PCIE_TO_NAME(HI1103_REGION_B_PERP_AHB)
    },
    {
        .pci_start = 0x50000000,
        .pci_end   = 0x5000EDFF,
        .cpu_start = 0x50000000,
        .cpu_end   = 0x5000EDFF,/*59KB*/
        .flag      = OAL_IORESOURCE_REG,
        .name      = OAL_PCIE_TO_NAME(HI1103_REGION_AON_APB)
    },
};

oal_reg_bits_stru g_pcie_phy_0v9_bits[] =
{
    {700, 0x0, "0.7v"},
    {750, 0x1, "0.75v"},
    {775, 0x2, "0.775v"},
    {800, 0x3, "0.8v"},
    {825, 0x4, "0.825v"},
    {850, 0x5, "0.85v",},
    {875, 0x6, "0.875v",},
    {900, 0x7, "0.9v"},
    {925, 0x8, "0.925v"},
    {950, 0x9, "0.950v"},
    {975, 0xa, "0.975v"},
    {1000, 0xb, "1v"},
    {1025, 0xc, "1.025v"},
    {1050, 0xd, "1.050v"},
    {1075, 0xe, "1.075v"},
    {1100, 0xf, "1.1v"},
};

oal_reg_bits_stru g_pcie_phy_1v8_bits[] =
{
    {1600, 0x0, "1.6v"},
    {1625, 0x1, "1.625v"},
    {1650, 0x2, "1.650v"},
    {1675, 0x3, "1.675v"},
    {1700, 0x4, "1.700v"},
    {1725, 0x5, "1.725v",},
    {1750, 0x6, "1.750v",},
    {1775, 0x7, "1.775v"},
    {1800, 0x8, "1.800v"},
};

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_H2D_BYPASS
oal_uint32 g_h2d_bypass_pkt_num = 0;
oal_netbuf_stru* g_h2d_pst_netbuf = NULL;
dma_addr_t g_h2d_pci_dma_addr = 0x0;
#endif

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_D2H_BYPASS
oal_uint32 g_d2h_bypass_pkt_num = 0;
oal_uint32 g_d2h_bypass_total_pkt_num =  0;
oal_netbuf_stru* g_d2h_pst_netbuf = NULL;
dma_addr_t g_d2h_pci_dma_addr = 0x0;
oal_completion g_d2h_test_done;
#endif

/*Function Declare*/
oal_int32 oal_pcie_h2d_doorbell(oal_pcie_res* pst_pci_res);
oal_int32 oal_pcie_d2h_doorbell(oal_pcie_res* pst_pci_res);
oal_int32 oal_pcie_d2h_ringbuf_rd_update(oal_pcie_res* pst_pci_res);
oal_int32 oal_pcie_d2h_ringbuf_wr_update(oal_pcie_res* pst_pci_res);
oal_uint32 oal_pcie_d2h_ringbuf_freecount(oal_pcie_res* pst_pci_res, oal_int32 is_sync);
oal_int32 oal_pcie_d2h_ringbuf_write(oal_pcie_res* pst_pci_res,
                                                    pci_addr_map*     pst_map,
                                                    pcie_write_ringbuf_item* pst_item);
oal_int32 oal_pcie_share_mem_res_map(oal_pcie_res* pst_pci_res);
oal_void  oal_pcie_share_mem_res_unmap(oal_pcie_res* pst_pci_res);
oal_void oal_pcie_ringbuf_res_unmap(oal_pcie_res* pst_pci_res);
oal_int32 oal_pcie_ringbuf_res_map(oal_pcie_res* pst_pci_res);
oal_uint32 oal_pcie_h2d_ringbuf_freecount(oal_pcie_res* pst_pci_res, PCIE_H2D_RINGBUF_QTYPE qtype, oal_int32 is_sync);
oal_int32 oal_pcie_h2d_ringbuf_rd_update(oal_pcie_res* pst_pci_res, PCIE_H2D_RINGBUF_QTYPE qtype);
oal_void oal_pcie_tx_res_clean(oal_pcie_res* pst_pci_res);
oal_void oal_pcie_rx_res_clean(oal_pcie_res* pst_pci_res);
oal_void oal_pcie_tx_netbuf_free(oal_pcie_res* pst_pci_res, oal_netbuf_stru* pst_netbuf);
oal_int32 oal_pcie_unmap_tx_netbuf(oal_pcie_res* pst_pci_res, oal_netbuf_stru* pst_netbuf);
#ifdef CONFIG_ARCH_SD56XX
extern oal_int32 oal_pcie_check_link_up(oal_void);
extern oal_int32 oal_pci_wlan_power_on(oal_int32 power_on);
extern int  pcie_sys_reinit(unsigned int mode_sel);
#endif

OAL_STATIC oal_void oal_pcie_print_debug_usages(oal_void);
OAL_STATIC oal_void oal_pcie_release_rx_netbuf(oal_pcie_res* pst_pci_res, oal_netbuf_stru* pst_netbuf);
oal_int32 oal_pcie_ringbuf_read_rd(oal_pcie_res* pst_pci_res, PCIE_COMM_RINGBUF_TYPE type);
oal_int32 oal_pcie_ringbuf_read_wr(oal_pcie_res* pst_pci_res, PCIE_COMM_RINGBUF_TYPE type);
oal_int32 oal_pcie_ringbuf_read(oal_pcie_res* pst_pci_res, PCIE_COMM_RINGBUF_TYPE type, oal_uint8* buf, oal_uint32 len);
oal_int32 oal_pcie_ringbuf_write(oal_pcie_res* pst_pci_res,
                                                    PCIE_COMM_RINGBUF_TYPE type, oal_uint8* buf, oal_uint32 len);
oal_uint32 oal_pcie_comm_ringbuf_freecount(oal_pcie_res* pst_pci_res,
                                                    PCIE_COMM_RINGBUF_TYPE type);
oal_int32 oal_pcie_device_check_alive(oal_pcie_res* pst_pci_res);

/*
 * 3 Function Definition
 */
oal_void oal_pcie_mem64_copy(oal_uint64* dst, oal_uint64* src, oal_int32 size)
{
    oal_int32 remain = size;

    for(;;)
    {
        if(remain < 8)
        {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
            rmb();
            wmb();/*dsb*/
#endif
            break;
        }
        if(remain >= 128)
        {
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            remain -= 128;
        }
        else if(remain >= 64)
        {
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            remain -= 64;
        }
        else if(remain >= 32)
        {
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            remain -= 32;
        }
        else if(remain >= 16)
        {
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            remain -= 16;
        }
        else
        {
            *((volatile oal_uint64*)dst++) = *((volatile oal_uint64*)src++);
            remain -= 8;
        }
    }
}

oal_void oal_pcie_mem32_copy(oal_uint32* dst, oal_uint32* src, oal_int32 size)
{
    oal_int32 remain = size;

    for(;;)
    {
        if(remain < 4)
        {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
            rmb();
            wmb();/*dsb*/
#endif
            break;
        }
        if(remain >= 64)
        {
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            remain -= 64;
        }
        else if(remain >= 32)
        {
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            remain -= 32;
        }
        else if(remain >= 16)
        {
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            remain -= 16;
        }
        else if(remain >= 8)
        {
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            remain -= 8;
        }
        else
        {
            *((volatile oal_uint32*)dst++) = *((volatile oal_uint32*)src++);
            remain -= 4;
        }
    }

}

oal_void oal_pcie_memport_copy(oal_void* dst, oal_void* src, oal_int32 size)
{
    oal_uint32 copy_size;
    oal_int32 remain = size;
    for(;;)
    {
        if(remain < 4)
        {
            break;
        }

        if((!((oal_ulong)src & 0x7)) && (remain >= 8))/*8bytes*/
        {
            copy_size = OAL_ROUND_DOWN((oal_uint32)remain, 8);
            remain -= copy_size;
            oal_pcie_mem64_copy(dst, src, (oal_int32)copy_size);
            src += copy_size;
            dst += copy_size;
        }
        else if((!((oal_ulong)src & 0x3)) && (remain >= 4))/*4bytes*/
        {
            remain -= 4;
            *((volatile oal_uint32*)dst) = *((volatile oal_uint32*)src);
            dst += 4;
            src += 4;
        }
        else
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "invalid argument, dst:%p , src:%p, remain:%d", dst, src, remain);
        }
    }
}

oal_int32 oal_pcie_memport_copy_test(oal_void)
{
    oal_ulong burst_size = 4096;
    oal_ulong timeout;
    oal_ulong total_size;
    declare_time_cost_stru(cost);
    oal_void* buff_src, *buff_dst;
    oal_uint64  trans_size, us_to_s;

    buff_src = oal_memalloc(burst_size);
    if(NULL == buff_src)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc %lu buff failed", burst_size);
        return - 1;
    }

    buff_dst = oal_memalloc(burst_size);
    if(NULL == buff_dst)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc %lu buff failed", burst_size);
        oal_free(buff_src);
        return - 1;
    }

    oal_get_time_cost_start(cost);
    timeout = jiffies + OAL_MSECS_TO_JIFFIES(2000);
    total_size = 0;
    for(;;)
    {
        if(oal_time_after(jiffies, timeout))
        {
            break;
        }

        oal_pcie_memcopy((oal_ulong)buff_dst, (oal_ulong)buff_src, burst_size);
        total_size += burst_size;
    }

    oal_get_time_cost_end(cost);
    oal_calc_time_cost_sub(cost);

    us_to_s = time_cost_var_sub(cost);
    trans_size = ((total_size * 1000u * 1000u) >> 17);
    trans_size = div_u64(trans_size, us_to_s);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "memcopy: %llu Mbps, trans_time:%llu us, tran_size:%lu",
                                                trans_size, us_to_s, total_size);

    oal_free(buff_src);
    oal_free(buff_dst);
    return 0;
}

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_PERFORMANCE
OAL_STATIC ktime_t g_mips_arry_total[PCIE_MIPS_BUTT];
OAL_STATIC ktime_t g_mips_arry[PCIE_MIPS_BUTT];
oal_void  oal_pcie_mips_start(PCIE_MIPS_TYPE type)
{
    if(OAL_WARN_ON(type >= PCIE_MIPS_BUTT))
    {
        OAL_IO_PRINT("%s error: type:%d >= limit :%d",__FUNCTION__, type, PCIE_MIPS_BUTT);
        return;
    }
    g_mips_arry[type] = ktime_get();
}

oal_void  oal_pcie_mips_end(PCIE_MIPS_TYPE type)
{
    ktime_t end = ktime_get();

    if(OAL_WARN_ON(type >= PCIE_MIPS_BUTT))
    {
        OAL_IO_PRINT("%s error: type:%d >= limit :%d",__FUNCTION__, type, PCIE_MIPS_BUTT);
        return;
    }

    g_mips_arry_total[type] = ktime_add(g_mips_arry_total[type], ktime_sub(end, g_mips_arry[type]));
}

oal_void oal_pcie_mips_clear(oal_void)
{
    OAL_MEMZERO(g_mips_arry, sizeof(g_mips_arry));
    OAL_MEMZERO(g_mips_arry_total, sizeof(g_mips_arry_total));
}

oal_void oal_pcie_mips_show(oal_void)
{
    oal_int32 i;
    oal_int64 trans_us, total_us;
    total_us = 0;
    for(i = 0; i < PCIE_MIPS_BUTT; i++)
    {
        trans_us = (oal_uint64)ktime_to_us(g_mips_arry_total[i]);
        total_us += trans_us;
        OAL_IO_PRINT("mips type:%d , trans_us :%llu us\n", i, trans_us);
    }
    OAL_IO_PRINT("total_us :%llu us \n", total_us);
}

#endif

oal_ulong oal_pcie_get_itcm_vaddr(oal_pcie_res* pst_pci_res)
{
    /*TBD:TBC*/
    return 0;
}

oal_ulong oal_pcie_get_dtcm_vaddr(oal_pcie_res* pst_pci_res)
{
    /*TBD:TBC*/
    return 0;
}

oal_pcie_res* oal_get_default_pcie_handler(oal_void)
{
    return g_pci_res;
}

/*edma functions*/
oal_int32 oal_pcie_edma_get_read_done_fifo(oal_pcie_res* pst_pci_res, edma_paddr_t* addr, oal_uint32* count)
{
    /*TBD:TBC*/
    /*后续考虑优化， PCIE读内存比较耗时*/
    /*先读FIFO0，分三次读走64bit 数据，读空*/
    oal_uint32 addr_low,addr_high;
    oal_uint32 trans_count;

    /*处理fifo0*/
    trans_count    = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_READ_FIFO0_DATA_OFF);
//#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_FIFO_ADDRESS
    addr_low       = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_READ_FIFO0_DATA_OFF+4);
    addr_high      = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_READ_FIFO0_DATA_OFF+8);
//#endif
    if(PCI_DBG_CONDTION())
    {
//#ifndef _PRE_PLAT_FEATURE_HI110X_PCIE_FIFO_ADDRESS
        addr_low  = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_READ_FIFO0_DATA_OFF+4);
        addr_high = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_READ_FIFO0_DATA_OFF+8);
//#endif
        PCI_PRINT_LOG(PCI_LOG_DBG, "read done fifo0 addr_low:0x%8x, addr_high:0x%8x, trans_count:0x%8x\n",
                                    addr_low, addr_high, trans_count);
    }


    trans_count = trans_count >> 1;/*一个数据包对应2个描述符*/
//#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_FIFO_ADDRESS
    addr->bits.low_addr = addr_low;
    addr->bits.high_addr = addr_high;
//#endif
    *count = trans_count;

    /*处理fifo1*/
    addr++;
    count++;

    trans_count = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_READ_FIFO1_DATA_OFF);
//#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_FIFO_ADDRESS
    addr_low    = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_READ_FIFO1_DATA_OFF+4);
    addr_high   = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_READ_FIFO1_DATA_OFF+8);
//#endif


    if(PCI_DBG_CONDTION())
    {
//#ifndef _PRE_PLAT_FEATURE_HI110X_PCIE_FIFO_ADDRESS
        addr_low    = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_READ_FIFO1_DATA_OFF+4);
        addr_high   = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_READ_FIFO1_DATA_OFF+8);
//#endif
        PCI_PRINT_LOG(PCI_LOG_DBG, "read done fifo1 addr_low:0x%8x, addr_high:0x%8x, trans_count:0x%8x\n",
                                    addr_low, addr_high, trans_count);
    }

    trans_count = trans_count >> 1;/*一个数据包对应2个描述符*/
//#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_FIFO_ADDRESS
    addr->bits.low_addr = addr_low;
    addr->bits.high_addr = addr_high;
//#endif
    *count = trans_count;

    return OAL_TRUE;
}

oal_int32 oal_pcie_edma_get_write_done_fifo(oal_pcie_res* pst_pci_res, edma_paddr_t* addr, oal_uint32* count)
{
    /*TBD:TBC*/
    /*后续考虑优化， PCIE读内存比较耗时*/
    /*先读FIFO0，分三次读走64bit 数据，读空*/
    oal_uint32 addr_low, addr_high;
    oal_uint32 trans_count;

    /*处理fifo0*/
    trans_count = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_WRITE_FIFO0_DATA_OFF);
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_FIFO_ADDRESS
    addr_low    = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_WRITE_FIFO0_DATA_OFF+4);
    addr_high   = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_WRITE_FIFO0_DATA_OFF+8);
#endif

    if(PCI_DBG_CONDTION())
    {
#ifndef _PRE_PLAT_FEATURE_HI110X_PCIE_FIFO_ADDRESS
    addr_low    = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_WRITE_FIFO0_DATA_OFF+4);
    addr_high   = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_WRITE_FIFO0_DATA_OFF+8);
#endif
        PCI_PRINT_LOG(PCI_LOG_DBG, "write done fifo0 addr_low:0x%8x, addr_high:0x%8x, trans_count:0x%8x\n",
                                addr_low, addr_high, trans_count);
    }

    trans_count = trans_count >> 1;/*一个数据包对应2个描述符*/
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_FIFO_ADDRESS
    addr->bits.low_addr = addr_low;
    addr->bits.high_addr = addr_high;
#endif
    *count = trans_count;


    /*处理fifo1*/
    addr++;
    count++;

    trans_count = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_WRITE_FIFO1_DATA_OFF);
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_FIFO_ADDRESS
    addr_low    = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_WRITE_FIFO1_DATA_OFF+4);
    addr_high   = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_WRITE_FIFO1_DATA_OFF+8);
#endif

    if(PCI_DBG_CONDTION())
    {
#ifndef _PRE_PLAT_FEATURE_HI110X_PCIE_FIFO_ADDRESS
        addr_low    = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_WRITE_FIFO1_DATA_OFF+4);
        addr_high   = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_FIFO_REMOTE_WRITE_FIFO1_DATA_OFF+8);
#endif
        PCI_PRINT_LOG(PCI_LOG_DBG, "write done fifo1 addr_low:0x%8x, addr_high:0x%8x, trans_count:0x%8x\n",
                                addr_low, addr_high, trans_count);
    }

    trans_count = trans_count >> 1;/*一个数据包对应2个描述符*/
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_FIFO_ADDRESS
    addr->bits.low_addr = addr_low;
    addr->bits.high_addr = addr_high;
#endif
    *count = trans_count;

    return OAL_TRUE;
}

/*
 * 3 Function Definition
 */

oal_int32 oal_pcie_disable_regions(oal_pcie_res* pst_pci_res)
{
    if(OAL_WARN_ON(NULL == pst_pci_res))
    {
        return -OAL_ENODEV;
    }

    pst_pci_res->regions.inited = 0;
    PCI_PRINT_LOG(PCI_LOG_DBG, "disable_regions");
    return OAL_SUCC;
}

oal_int32 oal_pcie_enable_regions(oal_pcie_res* pst_pci_res)
{
    if(OAL_WARN_ON(NULL == pst_pci_res))
    {
        return -OAL_ENODEV;
    }

    pst_pci_res->regions.inited = 1;

    PCI_PRINT_LOG(PCI_LOG_DBG, "enable_regions");
    return OAL_SUCC;
}

oal_void oal_pcie_iatu_reg_dump_by_viewport(oal_pcie_res * pst_pci_res)
{
    oal_int32 index;
    oal_int32 ret;
    oal_uint32 reg = 0;
    oal_uint32 region_num;
    IATU_VIEWPORT_OFF vp;
    oal_pcie_region* region_base;
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);

    region_num  = pst_pci_res->regions.region_nums;
    region_base = pst_pci_res->regions.pst_regions;
    for(index = 0; index < region_num; index++, region_base++)
    {
        vp.bits.region_dir = HI_PCI_IATU_INBOUND;
        vp.bits.region_index = index;/*iatu index*/
        ret = oal_pci_write_config_dword(pst_pci_dev, HI_PCI_IATU_VIEWPORT_OFF, vp.AsDword);
        if(ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "dump write [0x%8x:0x%8x] pcie failed, ret=%d\n", HI_PCI_IATU_VIEWPORT_OFF, vp.AsDword, ret);
            break;
        }

        ret = oal_pci_read_config_dword(pst_pci_dev, HI_PCI_IATU_VIEWPORT_OFF, &reg);
        if(ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "dump read [0x%8x] pcie failed, ret=%d\n",HI_PCI_IATU_VIEWPORT_OFF, ret);
            break;
        }

        PCI_PRINT_LOG(PCI_LOG_INFO, "INBOUND iatu index:%d 's register:\n", index);

        if(reg != vp.AsDword)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "dump write [0x%8x:0x%8x] pcie viewport failed value still 0x%8x\n",
                            HI_PCI_IATU_VIEWPORT_OFF, vp.AsDword, reg);
            break;
        }

        PRINT_PCIE_CONFIG_REG(pst_pci_dev, HI_PCI_IATU_VIEWPORT_OFF);

        PRINT_PCIE_CONFIG_REG(pst_pci_dev, HI_PCI_IATU_REGION_CTRL_1_OFF_INBOUND_I(HI_PCI_IATU_BOUND_BASE_OFF));

        PRINT_PCIE_CONFIG_REG(pst_pci_dev, HI_PCI_IATU_REGION_CTRL_2_OFF_INBOUND_I(HI_PCI_IATU_BOUND_BASE_OFF));

        PRINT_PCIE_CONFIG_REG(pst_pci_dev, HI_PCI_IATU_LWR_BASE_ADDR_OFF_INBOUND_I(HI_PCI_IATU_BOUND_BASE_OFF));

        PRINT_PCIE_CONFIG_REG(pst_pci_dev, HI_PCI_IATU_UPPER_BASE_ADDR_OFF_INBOUND_I(HI_PCI_IATU_BOUND_BASE_OFF));

        PRINT_PCIE_CONFIG_REG(pst_pci_dev, HI_PCI_IATU_LIMIT_ADDR_OFF_INBOUND_I(HI_PCI_IATU_BOUND_BASE_OFF));

        PRINT_PCIE_CONFIG_REG(pst_pci_dev, HI_PCI_IATU_LWR_TARGET_ADDR_OFF_INBOUND_I(HI_PCI_IATU_BOUND_BASE_OFF));

        PRINT_PCIE_CONFIG_REG(pst_pci_dev, HI_PCI_IATU_UPPER_TARGET_ADDR_OFF_INBOUND_I(HI_PCI_IATU_BOUND_BASE_OFF));
    }
}

oal_void oal_pcie_iatu_reg_dump_by_membar(oal_pcie_res * pst_pci_res)
{
    oal_void* inbound_addr = NULL;

    oal_int32 index;
    oal_uint32 region_num;
    oal_pcie_region* region_base;


    if(NULL == pst_pci_res->st_iatu_bar.st_region.vaddr)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "iatu bar1 vaddr is null");
        return;
    }

    inbound_addr = pst_pci_res->st_iatu_bar.st_region.vaddr;

    region_num  = pst_pci_res->regions.region_nums;
    region_base = pst_pci_res->regions.pst_regions;
    for(index = 0; index < region_num; index++, region_base++)
    {
        if(index >= 16)
        {
            break;
        }

        PCI_PRINT_LOG(PCI_LOG_INFO, "INBOUND iatu index:%d 's register:\n", index);

        oal_pcie_print_config_reg_bar(pst_pci_res, HI_PCI_IATU_REGION_CTRL_1_OFF_INBOUND_I(HI_PCI_IATU_INBOUND_BASE_OFF(index)),
                                      "HI_PCI_IATU_REGION_CTRL_1_OFF_INBOUND_I");

        oal_pcie_print_config_reg_bar(pst_pci_res, HI_PCI_IATU_REGION_CTRL_2_OFF_INBOUND_I(HI_PCI_IATU_INBOUND_BASE_OFF(index)),
                                      "HI_PCI_IATU_REGION_CTRL_2_OFF_INBOUND_I");

        oal_pcie_print_config_reg_bar(pst_pci_res, HI_PCI_IATU_LWR_BASE_ADDR_OFF_INBOUND_I(HI_PCI_IATU_INBOUND_BASE_OFF(index)),
                                      "HI_PCI_IATU_LWR_BASE_ADDR_OFF_INBOUND_I");

        oal_pcie_print_config_reg_bar(pst_pci_res, HI_PCI_IATU_UPPER_BASE_ADDR_OFF_INBOUND_I(HI_PCI_IATU_INBOUND_BASE_OFF(index)),
                                      "HI_PCI_IATU_UPPER_BASE_ADDR_OFF_INBOUND_I");

        oal_pcie_print_config_reg_bar(pst_pci_res, HI_PCI_IATU_LIMIT_ADDR_OFF_INBOUND_I(HI_PCI_IATU_INBOUND_BASE_OFF(index)),
                                      "HI_PCI_IATU_LIMIT_ADDR_OFF_INBOUND_I");

        oal_pcie_print_config_reg_bar(pst_pci_res, HI_PCI_IATU_LWR_TARGET_ADDR_OFF_INBOUND_I(HI_PCI_IATU_INBOUND_BASE_OFF(index)),
                                      "HI_PCI_IATU_LWR_TARGET_ADDR_OFF_INBOUND_I");

        oal_pcie_print_config_reg_bar(pst_pci_res, HI_PCI_IATU_UPPER_TARGET_ADDR_OFF_INBOUND_I(HI_PCI_IATU_INBOUND_BASE_OFF(index)),
                                      "HI_PCI_IATU_UPPER_TARGET_ADDR_OFF_INBOUND_I");
    }
}

/*bar and iATU table config*/
/*set ep inbound, host->device*/
oal_void  oal_pcie_iatu_reg_dump(oal_pcie_res * pst_pci_res)
{
    if(PCIE_REVISION_4_70A == pst_pci_res->revision)
    {
        oal_pcie_iatu_reg_dump_by_viewport(pst_pci_res);
    }
    else if(PCIE_REVISION_5_00A == pst_pci_res->revision)
    {
        oal_pcie_iatu_reg_dump_by_membar(pst_pci_res);
    }
    else
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "unkown pcie ip revision :0x%x", pst_pci_res->revision);
    }
}

oal_void oal_pcie_regions_info_dump(oal_pcie_res * pst_pci_res)
{
    oal_int32 index;
    oal_uint32 region_num;
    oal_pcie_region* region_base;

    region_num  = pst_pci_res->regions.region_nums;
    region_base = pst_pci_res->regions.pst_regions;

    if(OAL_WARN_ON(!pst_pci_res->regions.inited))
    {
        return;
    }

    if(region_num)
        OAL_IO_PRINT("regions[%u] info dump\n", region_num);

    for(index = 0; index < region_num; index++, region_base++)
    {
        OAL_IO_PRINT("[%15s]va:0x%p, pa:0x%llx, [pci start:0x%8llx end:0x%8llx],[cpu start:0x%8llx end:0x%8llx], size:%u, flag:0x%x\n",
                      region_base->name,
                      region_base->vaddr,
                      region_base->paddr,
                      region_base->pci_start,
                      region_base->pci_end,
                      region_base->cpu_start,
                      region_base->cpu_end,
                      region_base->size,
                      region_base->flag);

    }
}

oal_int32 oal_pcie_set_inbound_by_viewport(oal_pcie_res * pst_pci_res)
{
    oal_uint32 reg = 0;
    oal_int32 ret = OAL_SUCC;
    edma_paddr_t start, target, end;
    IATU_VIEWPORT_OFF vp;
    IATU_REGION_CTRL_2_OFF ctr2;
    oal_int32 index;
    oal_uint32 region_num;
    oal_pcie_region* region_base;
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
    region_num  = pst_pci_res->regions.region_nums;
    region_base = pst_pci_res->regions.pst_regions;

    for(index = 0; index < region_num; index++, region_base++)
    {
        vp.bits.region_dir = HI_PCI_IATU_INBOUND;
        vp.bits.region_index = index;/*iatu index*/
        ret = oal_pci_write_config_dword(pst_pci_dev, HI_PCI_IATU_VIEWPORT_OFF, vp.AsDword);
        if(ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "write [0x%8x:0x%8x] pcie failed, ret=%d\n", HI_PCI_IATU_VIEWPORT_OFF, vp.AsDword, ret);
            return -OAL_EIO;
        }

        /*TBD:TBC*/
        /*是否需要回读等待*/
        ret = oal_pci_read_config_dword(pst_pci_dev, HI_PCI_IATU_VIEWPORT_OFF, &reg);
        if(ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "read [0x%8x] pcie failed, index:%d, ret=%d\n",HI_PCI_IATU_VIEWPORT_OFF, index,ret);
            return -OAL_EIO;
        }

        if(reg != vp.AsDword)
        {
            /*1.viewport 没有切换完成 2. iatu配置个数超过了Soc的最大个数*/
            PCI_PRINT_LOG(PCI_LOG_ERR, "write [0x%8x:0x%8x] pcie viewport failed value still 0x%8x, region's index:%d\n",
                            HI_PCI_IATU_VIEWPORT_OFF, vp.AsDword, reg, index);
            return -OAL_EIO;
        }

        ret |= oal_pci_write_config_dword(pst_pci_dev, HI_PCI_IATU_REGION_CTRL_1_OFF_INBOUND_I(HI_PCI_IATU_BOUND_BASE_OFF), 0x0);

        ctr2.AsDword = 0;
        ctr2.bits.region_en = 1;
        ctr2.bits.bar_num = region_base->bar_info->bar_idx;
        ret |= oal_pci_write_config_dword(pst_pci_dev, HI_PCI_IATU_REGION_CTRL_2_OFF_INBOUND_I(HI_PCI_IATU_BOUND_BASE_OFF), ctr2.AsDword);

        /*Host侧64位地址的低32位地址*/
        start.addr = region_base->bus_addr;
        PCI_PRINT_LOG(PCI_LOG_INFO, "PCIe inbound bus addr:0x%llx",start.addr);
        ret |= oal_pci_write_config_dword(pst_pci_dev, HI_PCI_IATU_LWR_BASE_ADDR_OFF_INBOUND_I(HI_PCI_IATU_BOUND_BASE_OFF),  start.bits.low_addr);
        ret |= oal_pci_write_config_dword(pst_pci_dev, HI_PCI_IATU_UPPER_BASE_ADDR_OFF_INBOUND_I(HI_PCI_IATU_BOUND_BASE_OFF), start.bits.high_addr);

        //ret |= oal_pci_write_config_dword(pst_pci_dev, HI_PCI_IATU_LIMIT_ADDR_OFF_INBOUND_I,  region_base->size);
        end.addr = start.addr + region_base->size -1;
        if(start.bits.high_addr != end.bits.high_addr)
        {
            /*TBD:TBC*/
            /*如果跨了4G地址应该多配置一个iatu表项，待增加*/
            PCI_PRINT_LOG(PCI_LOG_ERR,"iatu high 32 bits must same![start:0x%llx, end:0x%llx]", start.addr, end.addr);
            return -OAL_EIO;
        }
        ret |= oal_pci_write_config_dword(pst_pci_dev, HI_PCI_IATU_LIMIT_ADDR_OFF_INBOUND_I(HI_PCI_IATU_BOUND_BASE_OFF),  end.bits.low_addr);

        /*Device侧对应的地址(PCI看到的地址)*/
        target.addr = region_base->pci_start;
        ret |= oal_pci_write_config_dword(pst_pci_dev, HI_PCI_IATU_LWR_TARGET_ADDR_OFF_INBOUND_I(HI_PCI_IATU_BOUND_BASE_OFF),    target.bits.low_addr);
        ret |= oal_pci_write_config_dword(pst_pci_dev, HI_PCI_IATU_UPPER_TARGET_ADDR_OFF_INBOUND_I(HI_PCI_IATU_BOUND_BASE_OFF),  target.bits.high_addr);

    }

    /*TBD:TBC*/
    /* 配置命令寄存器                                                                         */
    /* BIT0 = 1(I/O Space Enable), BIT1 = 1(Memory Space Enable), BIT2 = 1(Bus Master Enable) */
    ret |= oal_pci_write_config_word(pst_pci_dev, 0x04, 0x7);
    if(ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pci write iatu config failed ret=%d\n", ret);
        return -OAL_EIO;
    }

    if(PCI_DBG_CONDTION())
        oal_pcie_iatu_reg_dump(pst_pci_res);
    return OAL_SUCC;
}

oal_int32 oal_pcie_set_inbound_by_membar(oal_pcie_res * pst_pci_res)
{
    oal_void* inbound_addr = NULL;

    oal_int32 ret = OAL_SUCC;
    edma_paddr_t start, target, end;
    IATU_REGION_CTRL_2_OFF ctr2;
    oal_int32 index;
    oal_uint32 region_num;
    oal_pcie_region* region_base;
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
    region_num  = pst_pci_res->regions.region_nums;
    region_base = pst_pci_res->regions.pst_regions;

    if(NULL == pst_pci_res->st_iatu_bar.st_region.vaddr)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "iatu bar1 vaddr is null");
        return -OAL_ENOMEM;
    }

    inbound_addr = pst_pci_res->st_iatu_bar.st_region.vaddr;

    for(index = 0; index < region_num; index++, region_base++)
    {

        if(index >= 16)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "iatu regions too many, start:0x%llx", region_base->bar_info->start);
            break;
        }

        oal_writel(0x0, inbound_addr + HI_PCI_IATU_REGION_CTRL_1_OFF_INBOUND_I(HI_PCI_IATU_INBOUND_BASE_OFF(index)));

        ctr2.AsDword = 0;
        ctr2.bits.region_en = 1;
        ctr2.bits.bar_num = region_base->bar_info->bar_idx;
        oal_writel(ctr2.AsDword, inbound_addr + HI_PCI_IATU_REGION_CTRL_2_OFF_INBOUND_I(HI_PCI_IATU_INBOUND_BASE_OFF(index)));

        /*Host侧64位地址的低32位地址*/
        start.addr = region_base->bus_addr;
        PCI_PRINT_LOG(PCI_LOG_INFO, "PCIe inbound bus addr:0x%llx",start.addr);
        oal_writel(start.bits.low_addr, inbound_addr + HI_PCI_IATU_LWR_BASE_ADDR_OFF_INBOUND_I(HI_PCI_IATU_INBOUND_BASE_OFF(index)));
        oal_writel(start.bits.high_addr, inbound_addr + HI_PCI_IATU_UPPER_BASE_ADDR_OFF_INBOUND_I(HI_PCI_IATU_INBOUND_BASE_OFF(index)));

        //ret |= oal_pci_write_config_dword(pst_pci_dev, HI_PCI_IATU_LIMIT_ADDR_OFF_INBOUND_I,  region_base->size);
        end.addr = start.addr + region_base->size -1;
        if(start.bits.high_addr != end.bits.high_addr)
        {
            /*TBD:TBC*/
            /*如果跨了4G地址应该多配置一个iatu表项，待增加*/
            PCI_PRINT_LOG(PCI_LOG_ERR,"iatu high 32 bits must same![start:0x%llx, end:0x%llx]", start.addr, end.addr);
            return -OAL_EIO;
        }
        oal_writel(end.bits.low_addr, inbound_addr + HI_PCI_IATU_LIMIT_ADDR_OFF_INBOUND_I(HI_PCI_IATU_INBOUND_BASE_OFF(index)));

        /*Device侧对应的地址(PCI看到的地址)*/
        target.addr = region_base->pci_start;
        oal_writel(target.bits.low_addr,  inbound_addr + HI_PCI_IATU_LWR_TARGET_ADDR_OFF_INBOUND_I(HI_PCI_IATU_INBOUND_BASE_OFF(index)));
        oal_writel(target.bits.high_addr, inbound_addr + HI_PCI_IATU_UPPER_TARGET_ADDR_OFF_INBOUND_I(HI_PCI_IATU_INBOUND_BASE_OFF(index)));

    }

    if(index)
    {
        /*回读可以保证之前的IATU立刻生效*/
        oal_uint32 callback_read;
        callback_read = oal_readl(inbound_addr + HI_PCI_IATU_REGION_CTRL_1_OFF_INBOUND_I(HI_PCI_IATU_INBOUND_BASE_OFF(0)));
        OAL_REFERENCE(callback_read);
    }

    /*TBD:TBC*/
    /* 配置命令寄存器                                                                         */
    /* BIT0 = 1(I/O Space Enable), BIT1 = 1(Memory Space Enable), BIT2 = 1(Bus Master Enable) */
    ret |= oal_pci_write_config_word(pst_pci_dev, 0x04, 0x7);
    if(ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pci write iatu config failed ret=%d\n", ret);
        return -OAL_EIO;
    }

    if(PCI_DBG_CONDTION())
        oal_pcie_iatu_reg_dump(pst_pci_res);

    return OAL_SUCC;
}

oal_int32 oal_pcie_set_inbound(oal_pcie_res * pst_pci_res)
{
    if(PCIE_REVISION_4_70A == pst_pci_res->revision)
    {
        return oal_pcie_set_inbound_by_viewport(pst_pci_res);
    }
    else if(PCIE_REVISION_5_00A == pst_pci_res->revision)
    {
        return oal_pcie_set_inbound_by_membar(pst_pci_res);
    }
    else
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "unkown pcie ip revision :0x%x", pst_pci_res->revision);
        return -OAL_ENODEV;
    }
}

/*set ep outbound, device->host*/
oal_int32 oal_pcie_set_outbound(oal_pcie_res * pst_pci_res)
{
    /*1103 暂时 没有这部分需求，数采方案还没有明确*/
//#ifdef CONFIG_PCIE_1181_TRY
#if 0
    /* iATU1:512M */
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
    oal_pci_write_config_dword(pst_pci_dev, 0x700 + 0x200, 0x1);            /* view index */
    oal_pci_write_config_dword(pst_pci_dev, 0x700 + 0x204, 0x0);            /* ctrl 1 */
    oal_pci_write_config_dword(pst_pci_dev, 0x700 + 0x208, 0x80000000);     /* ctrl 2 */
    oal_pci_write_config_dword(pst_pci_dev, 0x700 + 0x20c, 0x500000);       /* base lower */
    oal_pci_write_config_dword(pst_pci_dev, 0x700 + 0x210, 0);              /* base upper */
    oal_pci_write_config_dword(pst_pci_dev, 0x700 + 0x214, 0xfffff);        /* limit */
    oal_pci_write_config_dword(pst_pci_dev, 0x700 + 0x218, 0xbf400000);     /* target lower */
    oal_pci_write_config_dword(pst_pci_dev, 0x700 + 0x21c, 0);              /* target upper */
    /* 配置命令寄存器                                                                         */
    /* BIT0 = 1(I/O Space Enable), BIT1 = 1(Memory Space Enable), BIT2 = 1(Bus Master Enable) */
    oal_pci_write_config_word(pst_pci_dev, 0x04, 0x7);
#endif
    return OAL_SUCC;
}

oal_void oal_pcie_iatu_exit(oal_pcie_res* pst_pci_res)
{
    /*TBD:TBC*/
}

oal_int32 oal_pcie_iatu_init(oal_pcie_res* pst_pci_res)
{
    oal_int32 ret;

    if(NULL == pst_pci_res)
    {
        return -OAL_ENODEV;
    }

    if(!pst_pci_res->regions.inited)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pcie regions is disabled, iatu config failed");
        return -OAL_EIO;
    }

    ret = oal_pcie_set_inbound(pst_pci_res);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pcie inbound set failed ret=%d\n", ret);
        return ret;
    }

    ret = oal_pcie_set_outbound(pst_pci_res);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pcie outbound set failed ret=%d\n", ret);
        return ret;
    }

    /*mem方式访问使能*/
    oal_pcie_change_link_state(pst_pci_res, PCI_WLAN_LINK_MEM_UP);
    return OAL_SUCC;
}

oal_int32 oal_pcie_ctrl_base_address_init(oal_pcie_res* pst_pci_res)
{
    oal_int32 ret;
    pci_addr_map st_map;
    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, PCIE_CTRL_BASE_ADDR,&st_map);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "get dev address 0x%x failed", PCIE_CTRL_BASE_ADDR);
        oal_pcie_regions_info_dump(pst_pci_res);
        return -OAL_EFAIL;
    }
    pst_pci_res->pst_pci_ctrl_base = (oal_void*)st_map.va;

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, PCIE_DMA_CTRL_BASE_ADDR,&st_map);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "get dev address 0x%x failed", PCIE_DMA_CTRL_BASE_ADDR);
        oal_pcie_regions_info_dump(pst_pci_res);
        pst_pci_res->pst_pci_ctrl_base = NULL;
        return -OAL_EFAIL;
    }
    pst_pci_res->pst_pci_dma_ctrl_base = (oal_void*)st_map.va;

    /*dbi base*/
    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, PCIE_CONFIG_BASE_ADDRESS, &st_map);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "get dev address 0x%x failed", PCIE_CONFIG_BASE_ADDRESS);
        oal_pcie_regions_info_dump(pst_pci_res);
        pst_pci_res->pst_pci_ctrl_base = NULL;
        pst_pci_res->pst_pci_dma_ctrl_base = NULL;
        return -OAL_EFAIL;
    }
    pst_pci_res->pst_pci_dbi_base = (oal_void*)st_map.va;

    PCI_PRINT_LOG(PCI_LOG_INFO, "ctrl base addr init succ, pci va:0x%p, fifo va:0x%p, dbi va:0x%p",
                                pst_pci_res->pst_pci_ctrl_base, pst_pci_res->pst_pci_dma_ctrl_base, pst_pci_res->pst_pci_dbi_base);
    return OAL_SUCC;
}

oal_void oal_pcie_ctrl_base_address_exit(oal_pcie_res* pst_pci_res)
{
    pst_pci_res->pst_pci_dma_ctrl_base = NULL;
    pst_pci_res->pst_pci_ctrl_base = NULL;
    pst_pci_res->pst_pci_dbi_base = NULL;
}

oal_void oal_pcie_regions_exit(oal_pcie_res* pst_pci_res)
{
    oal_int32  index;
    oal_uint32 region_num;
    oal_pcie_region* region_base;
    PCI_PRINT_LOG(PCI_LOG_INFO, "oal_pcie_regions_exit\n");
    pst_pci_res->regions.inited = 0;

    region_num  = pst_pci_res->regions.region_nums;
    region_base = pst_pci_res->regions.pst_regions;

    /*释放申请的地址空间*/
    for(index = 0; index < region_num; index++, region_base++)
    {
        if(NULL != region_base->vaddr)
        {
            oal_iounmap(region_base->vaddr);
            oal_release_mem_region(region_base->paddr, region_base->size);
            region_base->vaddr = NULL;
        }
    }
}

oal_void oal_pcie_iatu_bar_exit(oal_pcie_res* pst_pci_res)
{
    if(NULL == pst_pci_res->st_iatu_bar.st_region.vaddr)
    {
        return;
    }

    oal_iounmap(pst_pci_res->st_iatu_bar.st_region.vaddr);
    oal_release_mem_region(pst_pci_res->st_iatu_bar.st_region.paddr, pst_pci_res->st_iatu_bar.st_region.size);
    pst_pci_res->st_iatu_bar.st_region.vaddr = NULL;
}

oal_int32 oal_pcie_iatu_bar_init(oal_pcie_res* pst_pci_res)
{
    oal_resource * pst_res;
    oal_pcie_bar_info* bar_base;
    oal_pcie_region* region_base;

    if(0 == pst_pci_res->st_iatu_bar.st_bar_info.size)
    {
        return OAL_SUCC;
    }

    bar_base = &pst_pci_res->st_iatu_bar.st_bar_info;
    region_base= &pst_pci_res->st_iatu_bar.st_region;

    /*Bar1 专门用于配置 iatu表*/
    region_base->vaddr = NULL;/*remap 后的虚拟地址*/
    region_base->paddr = bar_base->start;/*Host CPU看到的物理地址*/
    region_base->bus_addr = 0x0;
    region_base->res   = NULL;
    region_base->bar_info = bar_base;
    region_base->size  = bar_base->size;
    region_base->name = "iatu_bar1";
    region_base->flag = OAL_IORESOURCE_REG;

    pst_res = oal_request_mem_region(region_base->paddr, region_base->size, region_base->name);
    if(NULL == pst_res)
    {
        goto failed_request_region;
    }

    /*remap*/
    if(region_base->flag & OAL_IORESOURCE_REG)
    {
        /*寄存器映射成非cache段, 不需要刷cache*/
        region_base->vaddr = oal_ioremap_nocache(region_base->paddr, region_base->size);

    }else{
        /*cache 段，注意要刷cache*/
        region_base->vaddr = oal_ioremap(region_base->paddr, region_base->size);
    }

    if(NULL == region_base->vaddr)
    {

        oal_release_mem_region(region_base->paddr, region_base->size);
        goto failed_remap;
    }

    /*remap and request succ.*/
    region_base->res   = pst_res;

    PCI_PRINT_LOG(PCI_LOG_INFO, "iatu bar1 virtual address:%p", region_base->vaddr);

    return OAL_SUCC;
failed_remap:
    oal_iounmap(region_base->vaddr);
    oal_release_mem_region(region_base->paddr, region_base->size);
    region_base->vaddr = NULL;
failed_request_region:
    return -OAL_ENOMEM;
}

oal_int32 oal_pcie_regions_init(oal_pcie_res* pst_pci_res)
{
    /*初始化DEVICE 每个段分配的HOST物理地址，然后做remap*/
    oal_void* vaddr;
    oal_int32 index, region_idx, bar_used_size;
    oal_uint32 bar_num, region_num;
    oal_pcie_bar_info* bar_base;
    oal_pcie_region* region_base;
    oal_resource * pst_res;

    if(OAL_WARN_ON(pst_pci_res->regions.inited))
    {
        /*不能重复初始化*/
        return -OAL_EBUSY;
    }

    bar_num =  pst_pci_res->regions.bar_nums;
    region_num  = pst_pci_res->regions.region_nums;

    bar_base = pst_pci_res->regions.pst_bars;
    region_base = pst_pci_res->regions.pst_regions;

    /*清空regions的特定字段*/
    for(index = 0; index < region_num; index++, region_base++)
    {
        region_base->vaddr = NULL;/*remap 后的虚拟地址*/
        region_base->paddr = 0x0;/*Host CPU看到的物理地址*/
        region_base->bus_addr = 0x0;
        region_base->res   = NULL;
        region_base->bar_info = NULL;
        region_base->size  = region_base->pci_end - region_base->pci_start + 1;
    }

    region_idx = 0;
    bar_used_size = 0;
    bar_base = pst_pci_res->regions.pst_bars;
    region_base = pst_pci_res->regions.pst_regions;

    for(index = 0; index < bar_num; index++, bar_base++)
    {
        for(; region_idx < region_num; region_idx++, region_base++)
        {
            /*BAR可用的起始地址*/
            if(bar_base->start + bar_used_size + region_base->size - 1  > bar_base->end)
            {
                /*这个BAR地址空间不足*/
                PCI_PRINT_LOG(PCI_LOG_ERR, "index:%d,region_idx:%d, start:0x%llx ,end:0x%llx, used_size:0x%x, region_size:%u\n",
                                index, region_idx, bar_base->start, bar_base->end, bar_used_size, region_base->size);
                break;
            }

            region_base->paddr = bar_base->start + bar_used_size;
            region_base->bus_addr = bar_base->bus_start + bar_used_size;
            bar_used_size += region_base->size;
            region_base->bar_info = bar_base;
            PCI_PRINT_LOG(PCI_LOG_INFO, "bar idx:%d, region idx:%d, region paddr:0x%llx, region_size:%u\n",
                            index, region_idx,region_base->paddr, region_base->size);
        }
    }

    if(region_idx < region_num)
    {
        /*地址不够用*/
        PCI_PRINT_LOG(PCI_LOG_ERR, "bar address range is too small, region_idx %d < region_num %d\n", region_idx, region_num);
        return -OAL_ENOMEM;
    }

    PCI_PRINT_LOG(PCI_LOG_INFO, "Total region num:%d, size:%d\n", region_num,bar_used_size);

    region_base = pst_pci_res->regions.pst_regions;
    for(index = 0; index < region_num; index++, region_base++)
    {
        if(!region_base->flag)
            continue;

        pst_res = oal_request_mem_region(region_base->paddr, region_base->size, region_base->name);
        if(NULL == pst_res)
        {
            goto failed_remap;
        }

        /*remap*/
        if(region_base->flag & OAL_IORESOURCE_REG)
        {
            /*寄存器映射成非cache段, 不需要刷cache*/
            vaddr = oal_ioremap_nocache(region_base->paddr, region_base->size);

        }else{
            /*cache 段，注意要刷cache*/
            vaddr = oal_ioremap(region_base->paddr, region_base->size);
        }

        if(NULL == vaddr)
        {

            oal_release_mem_region(region_base->paddr, region_base->size);
            goto failed_remap;
        }

        /*remap and request succ.*/
        region_base->res   = pst_res;
        region_base->vaddr = vaddr;/*Host Cpu 可以访问的虚拟地址*/

    }

    oal_pcie_enable_regions(pst_pci_res);

    PCI_PRINT_LOG(PCI_LOG_INFO, "oal_pcie_regions_init succ\n");
    return OAL_SUCC;
failed_remap:
    PCI_PRINT_LOG(PCI_LOG_ERR, "request mem region failed, addr:0x%llx, size:%u, name:%s\n",
                    region_base->paddr, region_base->size, region_base->name);
    oal_pcie_regions_exit(pst_pci_res);
    return -OAL_EIO;
}

oal_void oal_pcie_bar_exit(oal_pcie_res * pst_pci_res)
{
    oal_pcie_iatu_exit(pst_pci_res);
    oal_pcie_regions_exit(pst_pci_res);
    oal_pcie_iatu_bar_exit(pst_pci_res);
}

oal_int32 oal_pcie_get_ca_by_pa(oal_pcie_res * pst_pci_res, oal_ulong paddr, oal_uint64* cpuaddr)
{
    oal_int32 index;
    oal_uint32 region_num;
    oal_uint64 offset;
    oal_pcie_region* region_base;
    oal_ulong end;

    region_num  = pst_pci_res->regions.region_nums;
    region_base = pst_pci_res->regions.pst_regions;

    if(NULL == cpuaddr)
    {
        return -OAL_EINVAL;
    }

    *cpuaddr = 0;

    if(OAL_WARN_ON(!pst_pci_res->regions.inited))
    {
        return -OAL_ENODEV;
    }

    for(index = 0; index < region_num; index++, region_base++)
    {
        if(NULL == region_base->vaddr)
        {
            continue;
        }

        end = (oal_ulong)region_base->paddr + region_base->size - 1;

        if((paddr >= (oal_ulong)region_base->paddr) && (paddr <= end))
        {
            /*地址在范围内*/
            offset = paddr - (oal_ulong)region_base->paddr;
            *cpuaddr = region_base->cpu_start + offset;
            return OAL_SUCC;
        } else {
            continue;
        }
    }

    return -OAL_ENOMEM;
}

/*将Device Cpu看到的地址转换为 Host侧的虚拟地址,
  虚拟地址返回NULL为无效地址，Device Cpu地址有可能为0,
  local ip inbound cpu address to host virtual address,
  函数返回非0为失败*/
oal_int32 oal_pcie_inbound_ca_to_va(oal_pcie_res * pst_pci_res, oal_uint64 dev_cpuaddr,
                                                         pci_addr_map* addr_map)
{
    oal_int32 index;
    oal_uint32 region_num;
    oal_uint64 offset;
    oal_pcie_region* region_base;

    region_num  = pst_pci_res->regions.region_nums;
    region_base = pst_pci_res->regions.pst_regions;

    if(NULL != addr_map)
    {
        addr_map->pa = 0;
        addr_map->va = 0;
    }

    if(OAL_UNLIKELY(pst_pci_res->link_state < PCI_WLAN_LINK_MEM_UP))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "addr request 0x%llx failed, link_state:%s", dev_cpuaddr, oal_pcie_get_link_state_str(pst_pci_res->link_state));
        return -OAL_EBUSY;
    }

    if(OAL_WARN_ON(!pst_pci_res->regions.inited))
    {
        return -OAL_ENODEV;
    }

    for(index = 0; index < region_num; index++, region_base++)
    {
        if(NULL == region_base->vaddr)
        {
            continue;
        }

        if((dev_cpuaddr >= region_base->cpu_start) && (dev_cpuaddr <= region_base->cpu_end))
        {
            /*地址在范围内*/
            offset = dev_cpuaddr - region_base->cpu_start;
            if(NULL != addr_map)
            {
                /*返回HOST虚拟地址*/
                addr_map->va = (oal_ulong)(region_base->vaddr + offset);
                /*返回HOST物理地址*/
                addr_map->pa = (oal_ulong)(region_base->paddr + offset);
            }
            return OAL_SUCC;
        } else {
            continue;
        }
    }

    return -OAL_ENOMEM;
}

/*检查通过PCIE操作的HOST侧虚拟地址是否合法 ，是否映射过*/
oal_int32 oal_pcie_vaddr_isvalid(oal_pcie_res * pst_pci_res, oal_void* vaddr)
{
    oal_int32 index;
    oal_uint32 region_num;
    oal_pcie_region* region_base;
    if(OAL_WARN_ON(!pst_pci_res->regions.inited))
    {
        return OAL_FALSE;
    }

    region_num  = pst_pci_res->regions.region_nums;
    region_base = pst_pci_res->regions.pst_regions;

    for(index = 0; index < region_num; index++, region_base++)
    {
        if(NULL == region_base->vaddr)
        {
            continue;
        }

        if(((oal_ulong)vaddr >= (oal_ulong)region_base->vaddr)
          && ((oal_ulong)vaddr < (oal_ulong)region_base->vaddr + region_base->size))
        {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}

oal_int32 oal_pcie_bar_init(oal_pcie_res * pst_pci_res)
{
    oal_int32 index;
    oal_int32 ret = OAL_SUCC;
    oal_uint32 bar_num, region_num;
    oal_pcie_bar_info* bar_base;
    oal_pcie_region* region_base;

    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);

    /*暂时只考虑1103*/
    bar_num  = OAL_ARRAY_SIZE(g_en_bar_table);
    bar_base = &g_en_bar_table[0];

    if(PCIE_REVISION_4_70A == pst_pci_res->revision)
    {
        region_num = OAL_ARRAY_SIZE(g_hi1103_pcie_mpw2_regions);
        region_base = &g_hi1103_pcie_mpw2_regions[0];
    }
    else if(PCIE_REVISION_5_00A == pst_pci_res->revision)
    {
        region_num = OAL_ARRAY_SIZE(g_hi1103_pcie_pilot_regions);
        region_base = &g_hi1103_pcie_pilot_regions[0];
    }
    else
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "unkown pcie ip revision :0x%x\n", pst_pci_res->revision);
        return -OAL_ENODEV;
    }

    PCI_PRINT_LOG(PCI_LOG_INFO, "bar_num:%u, region_num:%u\n", bar_num, region_num);

    pst_pci_res->regions.pst_bars = bar_base;
    pst_pci_res->regions.bar_nums = bar_num;

    pst_pci_res->regions.pst_regions = region_base;
    pst_pci_res->regions.region_nums = region_num;

    /*这里不映射，iatu配置要和映射分段对应*/
    for(index = 0; index < bar_num; index++)
    {
        /*获取Host分配的硬件地址资源,1103为8M大小,
          1103 4.7a 对应一个BAR, 5.0a 对应2个bar,
          其中第二个bar用于配置iatu表*/

        oal_pcie_bar_info* bar_curr = bar_base + index;
        oal_uint8 bar_idx = bar_curr->bar_idx;

        /*pci resource built in pci_read_bases kernel.*/
        bar_curr->start = oal_pci_resource_start(pst_pci_dev, bar_idx);
        bar_curr->end = oal_pci_resource_end(pst_pci_dev, bar_idx);
        bar_curr->bus_start = oal_pci_bus_address(pst_pci_dev, bar_idx);
        bar_curr->size = oal_pci_resource_len(pst_pci_dev, bar_idx);

        PCI_PRINT_LOG(PCI_LOG_INFO, "preapre for bar idx:%u, phy start:0x%llx, end:0x%llx, bus address 0x%lx size:0x%x, flags:0x%lx\n",
                            bar_idx,
                            bar_curr->start,
                            bar_curr->end,
                            (oal_ulong)bar_curr->bus_start,
                            bar_curr->size,
                            oal_pci_resource_flags(pst_pci_dev, bar_idx));
    }

    /*是否支持BAR1*/
    if(PCIE_REVISION_5_00A == pst_pci_res->revision)
    {
        /*Get Bar Address*/
        oal_pcie_bar_info* bar_curr = &pst_pci_res->st_iatu_bar.st_bar_info;
        bar_curr->size = oal_pci_resource_len(pst_pci_dev, PCIE_IATU_BAR_INDEX);
        if(0 == bar_curr->size)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "bar 1 size is zero, start:0x%lx, end:0x%lx",
                                        oal_pci_resource_start(pst_pci_dev, PCIE_IATU_BAR_INDEX),
                                        oal_pci_resource_end(pst_pci_dev, PCIE_IATU_BAR_INDEX));
            return -OAL_EIO;
        }

        bar_curr->end       = oal_pci_resource_end(pst_pci_dev, PCIE_IATU_BAR_INDEX);
        bar_curr->start     = oal_pci_resource_start(pst_pci_dev, PCIE_IATU_BAR_INDEX);
        bar_curr->bus_start = oal_pci_bus_address(pst_pci_dev, PCIE_IATU_BAR_INDEX);
        bar_curr->bar_idx  = PCIE_IATU_BAR_INDEX;

        PCI_PRINT_LOG(PCI_LOG_INFO, "preapre for bar idx:%u, phy start:0x%llx, end:0x%llx, bus address 0x%lx size:0x%x, flags:0x%lx\n",
                            PCIE_IATU_BAR_INDEX,
                            bar_curr->start,
                            bar_curr->end,
                            (oal_ulong)bar_curr->bus_start,
                            bar_curr->size,
                            oal_pci_resource_flags(pst_pci_dev, PCIE_IATU_BAR_INDEX));
    }

    ret = oal_pcie_iatu_bar_init(pst_pci_res);
    if(OAL_SUCC != ret)
    {
        return ret;
    }

    ret = oal_pcie_regions_init(pst_pci_res);
    if(OAL_SUCC != ret)
    {
        oal_pcie_iatu_bar_exit(pst_pci_res);
        return ret;
    }

    ret = oal_pcie_iatu_init(pst_pci_res);
    if(OAL_SUCC != ret)
    {
        oal_pcie_regions_exit(pst_pci_res);
        oal_pcie_iatu_bar_exit(pst_pci_res);
        return ret;
    }
    PCI_PRINT_LOG(PCI_LOG_INFO,"bar init succ");
    return OAL_SUCC;
}

/*补充rx netbuf*/
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_D2H_BYPASS
oal_int32 oal_pcie_rx_ringbuf_bypass_supply( oal_pcie_res* pst_pci_res,
                                            oal_int32 is_sync,
                                            oal_int32 is_doorbell,
                                            oal_uint32 request_cnt)
{
    oal_int32 i;
    oal_int32 cnt = 0;
    oal_ulong flags;
    pcie_cb_dma_res* pst_cb_res;
    dma_addr_t pci_dma_addr;
    pcie_write_ringbuf_item st_write_item;
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
    oal_pcie_mips_start(PCIE_MIPS_RX_NETBUF_SUPPLY);
    if(OAL_TRUE == is_sync)
    {
        /*同步Dev2Host的读指针*/
        oal_pcie_d2h_ringbuf_rd_update(pst_pci_res);
    }

    for(i = 0; i < request_cnt; i++)
    {
        if(0 == oal_pcie_d2h_ringbuf_freecount(pst_pci_res, OAL_FALSE))
        {
            break;
        }

        if(0 == g_d2h_pci_dma_addr)
        {
            break;
        }
        st_write_item.buff_paddr.addr = g_d2h_pci_dma_addr;

        PCI_PRINT_LOG(PCI_LOG_DBG, "d2h ringbuf write [netbuf:0x%p, data:[va:0x%lx,pa:0x%llx]",
                                    g_d2h_pst_netbuf, (oal_ulong)OAL_NETBUF_DATA(g_d2h_pst_netbuf) ,st_write_item.buff_paddr.addr);
        if(OAL_UNLIKELY(0 == oal_pcie_d2h_ringbuf_write(pst_pci_res, &pst_pci_res->st_rx_res.ringbuf_data_dma_addr, &st_write_item)))
        {
            break;
        }

        cnt++;
    }

    /*这里需要考虑HOST/DEVICE的初始化顺序*/
    if(cnt && (OAL_TRUE == is_doorbell))
    {
        oal_pcie_d2h_ringbuf_wr_update(pst_pci_res);
    }

    oal_pcie_mips_end(PCIE_MIPS_RX_NETBUF_SUPPLY);

    return cnt;
}
#endif
oal_int32 oal_pcie_rx_ringbuf_supply( oal_pcie_res* pst_pci_res,
                                            oal_int32 is_sync,
                                            oal_int32 is_doorbell,
                                            oal_uint32 request_cnt,
                                            oal_int32 gflag,
                                            oal_int32 * ret)
{
    oal_uint32 i;
    oal_uint32 cnt = 0;
    oal_ulong flags;
    oal_netbuf_stru* pst_netbuf;
    pcie_cb_dma_res* pst_cb_res;
    dma_addr_t pci_dma_addr;
    pcie_write_ringbuf_item st_write_item;
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
    *ret = OAL_SUCC;
    oal_pcie_mips_start(PCIE_MIPS_RX_NETBUF_SUPPLY);
    if(OAL_TRUE == is_sync)
    {
        /*同步Dev2Host的读指针*/
        oal_pcie_d2h_ringbuf_rd_update(pst_pci_res);
    }

    for(i = 0; i < request_cnt; i++)
    {
        if(0 == oal_pcie_d2h_ringbuf_freecount(pst_pci_res, OAL_FALSE))
        {
            break;
        }
        /*ringbuf 有空间*/
        /*预申请netbuf都按照大包来申请*/
        oal_pcie_mips_start(PCIE_MIPS_RX_MEM_ALLOC);
        pst_netbuf = oal_pcie_rx_netbuf_alloc(HCC_HDR_TOTAL_LEN + PCIE_EDMA_TRANS_MAX_FRAME_LEN, gflag);
        if(NULL == pst_netbuf)
        {
            pst_pci_res->st_rx_res.stat.alloc_netbuf_failed++;
            *ret = -OAL_ENOMEM;
            oal_pcie_mips_end(PCIE_MIPS_RX_MEM_ALLOC);
            break;
        }
        oal_netbuf_put(pst_netbuf,(HCC_HDR_TOTAL_LEN + PCIE_EDMA_TRANS_MAX_FRAME_LEN));
        oal_pcie_mips_end(PCIE_MIPS_RX_MEM_ALLOC);

        //oal_pcie_mips_start(PCIE_MIPS_RX_NETBUF_MAP);
        if(pcie_dma_data_check_enable)
        {
            oal_uint32 value;
            /*增加标记，判断DMA是否真的启动*/
            oal_writel(0xffffffff, (oal_void*)OAL_NETBUF_DATA(pst_netbuf));
            value = (oal_uint32)(oal_ulong)OAL_NETBUF_DATA(pst_netbuf) + HCC_HDR_TOTAL_LEN;
            oal_writel(value, ((oal_void*)OAL_NETBUF_DATA(pst_netbuf) + HCC_HDR_TOTAL_LEN));
            pci_dma_addr = dma_map_single(&pst_pci_dev->dev, OAL_NETBUF_DATA(pst_netbuf), OAL_NETBUF_LEN(pst_netbuf), PCI_DMA_TODEVICE);
        }
        else
        {
            pci_dma_addr = dma_map_single(&pst_pci_dev->dev, OAL_NETBUF_DATA(pst_netbuf), OAL_NETBUF_LEN(pst_netbuf), PCI_DMA_FROMDEVICE);
        }

        if (dma_mapping_error(&pst_pci_dev->dev, pci_dma_addr))
        {
            pst_pci_res->st_rx_res.stat.map_netbuf_failed++;
            PCI_PRINT_LOG(PCI_LOG_INFO, "rx dma map netbuf failed, len=%u,cnt:%u",
                        OAL_NETBUF_LEN(pst_netbuf),
                        pst_pci_res->st_rx_res.stat.map_netbuf_failed);
            oal_netbuf_free(pst_netbuf);
            break;
        }

        /*DMA地址填到CB中, CB首地址8字节对齐可以直接强转*/
        pst_cb_res = (pcie_cb_dma_res*)OAL_NETBUF_CB(pst_netbuf);
        pst_cb_res->paddr.addr = pci_dma_addr;
        pst_cb_res->len = OAL_NETBUF_LEN(pst_netbuf);

        st_write_item.buff_paddr.addr = pci_dma_addr;
        //st_write_item.reserved0  = 0x1234;
        //st_write_item.buf_len    = OAL_NETBUF_LEN(pst_netbuf);

        /*入队*/

        oal_spin_lock_irq_save(&pst_pci_res->st_rx_res.lock, &flags);
        oal_netbuf_list_tail_nolock(&pst_pci_res->st_rx_res.rxq, pst_netbuf);
        oal_spin_unlock_irq_restore(&pst_pci_res->st_rx_res.lock, &flags);


        PCI_PRINT_LOG(PCI_LOG_DBG, "d2h ringbuf write [netbuf:0x%p, data:[va:0x%lx,pa:0x%llx]",
                                    pst_netbuf, (oal_ulong)OAL_NETBUF_DATA(pst_netbuf) ,st_write_item.buff_paddr.addr);
        oal_pcie_mips_start(PCIE_MIPS_RX_RINGBUF_WRITE);
        if(OAL_UNLIKELY(0 == oal_pcie_d2h_ringbuf_write(pst_pci_res, &pst_pci_res->st_rx_res.ringbuf_data_dma_addr, &st_write_item)))
        {
            //dma_unmap_single(&pst_pci_dev->dev, pci_dma_addr, OAL_NETBUF_LEN(pst_netbuf), PCI_DMA_FROMDEVICE);
            //oal_netbuf_free(pst_netbuf);//
            oal_pcie_mips_end(PCIE_MIPS_RX_RINGBUF_WRITE);
            break;
        }
        oal_pcie_mips_end(PCIE_MIPS_RX_RINGBUF_WRITE);

        cnt++;
    }

    /*这里需要考虑HOST/DEVICE的初始化顺序*/
    if(cnt && (OAL_TRUE == is_doorbell))
    {
        oal_pcie_d2h_ringbuf_wr_update(pst_pci_res);
        /*暂时不需要敲铃，D2H Device是大循环*/
        //oal_pcie_d2h_doorbell(pst_pci_res);/*TBD:TBC*/
    }

    oal_pcie_mips_end(PCIE_MIPS_RX_NETBUF_SUPPLY);

    return cnt;
}

/*预先分配rx的接收buf*/
oal_int32 oal_pcie_rx_ringbuf_build(oal_pcie_res* pst_pci_res)
{
    /*走到这里要确保DEVICE ZI区已经初始化完成，
      中断已经注册和使能*/
    /*TBD:TBC*/
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_D2H_BYPASS
#else
    oal_int32 ret;
    oal_int32 supply_num;
    supply_num = oal_pcie_rx_ringbuf_supply(pst_pci_res, OAL_TRUE, OAL_TRUE, PCIE_RX_RINGBUF_SUPPLY_ALL, GFP_KERNEL, &ret);
    if(0 == supply_num)
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "oal_pcie_rx_ringbuf_build can't get any netbufs!, rxq len:%u", oal_netbuf_list_len(&pst_pci_res->st_rx_res.rxq));
        oal_pcie_print_ringbuf_info(&pst_pci_res->st_ringbuf.st_d2h_buf, PCI_LOG_WARN);
        return -OAL_ENOMEM;
    }else{
        PCI_PRINT_LOG(PCI_LOG_INFO, "oal_pcie_rx_ringbuf_build got %u netbufs!", supply_num);
    }
#endif
    return OAL_SUCC;
}

/*释放RX通路的资源*/
oal_void oal_pcie_rx_res_clean(oal_pcie_res* pst_pci_res)
{
    oal_ulong flags;
    oal_netbuf_stru* pst_netbuf;

    /*释放RX补充队列*/
    PCI_PRINT_LOG(PCI_LOG_INFO, "prepare free rxq len=%d", oal_netbuf_list_len(&pst_pci_res->st_rx_res.rxq));
    for(;;)
    {
        oal_spin_lock_irq_save(&pst_pci_res->st_rx_res.lock, &flags);
        pst_netbuf = oal_netbuf_delist_nolock(&pst_pci_res->st_rx_res.rxq);
        oal_spin_unlock_irq_restore(&pst_pci_res->st_rx_res.lock, &flags);

        if(NULL == pst_netbuf)
            break;

        oal_pcie_release_rx_netbuf(pst_pci_res, pst_netbuf);
    }
}

/*归还tx ringbuf 中的报文 回hcc 队列*/
oal_void oal_pcie_tx_res_restore(oal_pcie_res* pst_pci_res)
{
    oal_int32 i;
    oal_ulong flags;
    oal_netbuf_stru* pst_netbuf;
    oal_pcie_linux_res * pst_pci_lres = (oal_pcie_linux_res*)oal_pci_get_drvdata(PCIE_RES_TO_DEV(pst_pci_res));
    struct hcc_handler* hcc = HBUS_TO_HCC(pst_pci_lres->pst_bus);
    /*归还TX发送队列, Ringbuf 在DEV侧 由wcpu重新初始化*/
    for(i = 0; i < PCIE_H2D_QTYPE_BUTT; i++)
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "prepare restore txq[%d] len=%d", i ,oal_netbuf_list_len(&pst_pci_res->st_tx_res[i].txq));
        for(;;)
        {
            oal_spin_lock_irq_save(&pst_pci_res->st_tx_res[i].lock, &flags);
            pst_netbuf = oal_netbuf_delist_nolock(&pst_pci_res->st_tx_res[i].txq);
            oal_spin_unlock_irq_restore(&pst_pci_res->st_tx_res[i].lock, &flags);

            if(NULL == pst_netbuf)
                break;

			/*unmap pcie tx netbuf's phy address*/
            oal_pcie_unmap_tx_netbuf(pst_pci_res, pst_netbuf);

            hcc_restore_tx_netbuf(hcc, pst_netbuf);
        }
    }


}

/*释放TX通路的资源*/
oal_void oal_pcie_tx_res_clean(oal_pcie_res* pst_pci_res)
{
    oal_int32 i;
    oal_ulong flags;
    oal_netbuf_stru* pst_netbuf;


    /*释放待TX发送队列, Ringbuf 在DEV侧 直接下电*/
    for(i = 0; i < PCIE_H2D_QTYPE_BUTT; i++)
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "prepare free txq[%d] len=%d", i ,oal_netbuf_list_len(&pst_pci_res->st_tx_res[i].txq));
        for(;;)
        {
            oal_spin_lock_irq_save(&pst_pci_res->st_tx_res[i].lock, &flags);
            pst_netbuf = oal_netbuf_delist_nolock(&pst_pci_res->st_tx_res[i].txq);
            oal_spin_unlock_irq_restore(&pst_pci_res->st_tx_res[i].lock, &flags);

            if(NULL == pst_netbuf)
                break;

            oal_pcie_tx_netbuf_free(pst_pci_res, pst_netbuf);
        }
        oal_atomic_set(&pst_pci_res->st_tx_res[i].tx_ringbuf_sync_cond, 0);
    }
}

oal_int32 oal_pcie_transfer_res_init(oal_pcie_res * pst_pci_res)
{
    oal_int32 ret = OAL_SUCC;

    /*下载完PATCH才需要执行下面的操作,
      芯片验证阶段通过SSI下载代码*/
    /*TBD:TBC*/
    ret = oal_pcie_share_mem_res_map(pst_pci_res);
    if(OAL_SUCC != ret)
    {
        //oal_pcie_bar_exit(pst_pci_res);
        return ret;
    }

    ret = oal_pcie_ringbuf_res_map(pst_pci_res);
    if(OAL_SUCC != ret)
    {
        //oal_pcie_bar_exit(pst_pci_res);
        return ret;
    }

    oal_pcie_rx_res_clean(pst_pci_res);
    oal_pcie_tx_res_clean(pst_pci_res);

    ret = oal_pcie_rx_ringbuf_build(pst_pci_res);
    if(OAL_SUCC != ret)
    {
        //oal_pcie_bar_exit(pst_pci_res);
        return ret;
    }

    mutex_lock(&pst_pci_res->st_rx_mem_lock);
    oal_pcie_change_link_state(pst_pci_res, PCI_WLAN_LINK_RES_UP);
    mutex_unlock(&pst_pci_res->st_rx_mem_lock);

    return ret;
}

oal_void oal_pcie_transfer_res_exit(oal_pcie_res * pst_pci_res)
{
    oal_pcie_change_link_state(pst_pci_res, PCI_WLAN_LINK_DOWN);

    oal_pcie_rx_res_clean(pst_pci_res);
    oal_pcie_tx_res_clean(pst_pci_res);

    oal_pcie_ringbuf_res_unmap(pst_pci_res);

    oal_pcie_share_mem_res_unmap(pst_pci_res);
}

/*配置BAR,IATU等设备资源*/
oal_int32 oal_pcie_dev_init(oal_pcie_res * pst_pci_res)
{
    oal_int32 ret = OAL_SUCC;
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);

    if(OAL_WARN_ON(NULL == pst_pci_dev))
        return -OAL_ENODEV;

    ret = oal_pcie_bar_init(pst_pci_res);
    if(OAL_SUCC != ret)
    {
        return ret;
    }

    ret = oal_pcie_ctrl_base_address_init(pst_pci_res);
    if(OAL_SUCC != ret)
    {
        oal_pcie_bar_exit(pst_pci_res);
        return ret;
    }

    /*TBD:TBC*/
    /*移植到麒麟平台这里要打开,在Host主控上下电后合入*/
#if 0
    ret = oal_pcie_transfer_res_init(pst_pci_res);
    if(OAL_SUCC != ret)
    {
        oal_pcie_ctrl_base_address_exit(pst_pci_res);
        oal_pcie_bar_exit(pst_pci_res);
        return ret;
    }
#endif

    return OAL_SUCC;
}
oal_void oal_pcie_dev_deinit(oal_pcie_res * pst_pci_res)
{
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
    if(OAL_WARN_ON(NULL == pst_pci_dev))
        return;
    oal_pcie_ctrl_base_address_exit(pst_pci_res);
    oal_pcie_bar_exit(pst_pci_res);
}

/*isr functions*/
oal_int32 oal_pcie_tx_dma_addr_match(oal_netbuf_stru* pst_netbuf, edma_paddr_t dma_addr)
{
    /*dma_addr 存放在CB字段里*/
    pcie_cb_dma_res st_cb_dma;

    /*不是从CB的首地址开始，必须拷贝，对齐问题。*/
    oal_memcopy(&st_cb_dma, (oal_uint8*)OAL_NETBUF_CB(pst_netbuf) + sizeof(struct hcc_tx_cb_stru), sizeof(st_cb_dma));

    PCI_PRINT_LOG(PCI_LOG_DBG, "tx dma addr match, cb's addr 0x%llx , dma_addr 0x%llx", st_cb_dma.paddr.addr, dma_addr.addr);

    if(st_cb_dma.paddr.addr == dma_addr.addr)
        return OAL_TRUE;
    return OAL_FALSE;
}

oal_int32 oal_pcie_tx_dma_addr_match_low(oal_netbuf_stru* pst_netbuf, oal_uint16 dma_addr)
{
    /*dma_addr 存放在CB字段里*/
    pcie_cb_dma_res st_cb_dma;

    /*不是从CB的首地址开始，必须拷贝，对齐问题。*/
    oal_memcopy(&st_cb_dma, (oal_uint8*)OAL_NETBUF_CB(pst_netbuf) + sizeof(struct hcc_tx_cb_stru), sizeof(st_cb_dma));

    PCI_PRINT_LOG(PCI_LOG_DBG, "tx dma addr match, cb's addr 0x%llx , dma_addr 0x%x", st_cb_dma.paddr.addr, dma_addr);

    if((oal_uint16)st_cb_dma.paddr.bits.low_addr == dma_addr)
        return OAL_TRUE;

    PCI_PRINT_LOG(PCI_LOG_WARN, "tx dma addr match, cb's addr 0x%llx , dma_addr 0x%x", st_cb_dma.paddr.addr, dma_addr);
    return OAL_FALSE;
}

oal_int32 oal_pcie_rx_dma_addr_match(oal_netbuf_stru* pst_netbuf, edma_paddr_t dma_addr)
{
    pcie_cb_dma_res* pst_cb_res;
    /*DMA地址填到CB中, CB首地址8字节对齐可以直接强转*/
    pst_cb_res = (pcie_cb_dma_res*)OAL_NETBUF_CB(pst_netbuf);
    if(pst_cb_res->paddr.addr == dma_addr.addr)
        return OAL_TRUE;
    return OAL_FALSE;
}

oal_int32 oal_pcie_rx_dma_addr_matchlow(oal_netbuf_stru* pst_netbuf, oal_uint32 dma_addr)
{
    pcie_cb_dma_res* pst_cb_res;
    /*DMA地址填到CB中, CB首地址8字节对齐可以直接强转*/
    pst_cb_res = (pcie_cb_dma_res*)OAL_NETBUF_CB(pst_netbuf);
    if(pst_cb_res->paddr.bits.low_addr == dma_addr)
        return OAL_TRUE;

    PCI_PRINT_LOG(PCI_LOG_WARN, "rx dma addr match, cb's addr 0x%x , dma_addr 0x%x", pst_cb_res->paddr.bits.low_addr, dma_addr);
    return OAL_FALSE;
}

OAL_STATIC oal_void oal_pcie_release_rx_netbuf(oal_pcie_res* pst_pci_res, oal_netbuf_stru* pst_netbuf)
{
    pcie_cb_dma_res* pst_cb_res;
    oal_pci_dev_stru *pst_pci_dev;
    if(OAL_WARN_ON(NULL == pst_netbuf))
    {
        return;
    }

    if(OAL_WARN_ON(NULL == pst_pci_res))
    {
        DECLARE_DFT_TRACE_KEY_INFO("pcie release rx netbuf", OAL_DFT_TRACE_FAIL);
        oal_netbuf_free(pst_netbuf);
        return;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
    if(OAL_WARN_ON(NULL == pst_pci_dev))
    {
        DECLARE_DFT_TRACE_KEY_INFO("pcie release rx netbuf", OAL_DFT_TRACE_FAIL);
        oal_netbuf_free(pst_netbuf);
        return;
    }

    pst_cb_res = (pcie_cb_dma_res*)OAL_NETBUF_CB(pst_netbuf);
    if(OAL_LIKELY((0 != pst_cb_res->paddr.addr) && (0 != pst_cb_res->len)))
    {
        dma_unmap_single(&pst_pci_dev->dev, (dma_addr_t)pst_cb_res->paddr.addr, pst_cb_res->len, PCI_DMA_FROMDEVICE);
    }
    else
    {
        DECLARE_DFT_TRACE_KEY_INFO("pcie release rx netbuf", OAL_DFT_TRACE_FAIL);
    }

    oal_netbuf_free(pst_netbuf);

}

/*向Hcc层提交收到的netbuf*/
oal_void oal_pcie_rx_netbuf_submit(oal_pcie_res* pst_pci_res, oal_netbuf_stru* pst_netbuf)
{
    struct hcc_handler* hcc;
    oal_pci_dev_stru *pst_pci_dev;
    pcie_cb_dma_res* pst_cb_res;
    oal_pcie_linux_res * pst_pci_lres;

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
    if(OAL_UNLIKELY(NULL == pst_pci_dev))
    {
        goto release_netbuf;
    }

    pst_pci_lres = (oal_pcie_linux_res*)oal_pci_get_drvdata(pst_pci_dev);
    if(OAL_UNLIKELY(NULL == pst_pci_lres))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "lres is null");
        goto release_netbuf;
    }

    if(OAL_UNLIKELY(NULL == pst_pci_lres->pst_bus))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "lres's bus is null");
        goto release_netbuf;
    }

    if(OAL_UNLIKELY(NULL == HBUS_TO_DEV(pst_pci_lres->pst_bus)))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "lres's dev is null");
        goto release_netbuf;
    }

    hcc = HBUS_TO_HCC(pst_pci_lres->pst_bus);
    if(OAL_UNLIKELY(NULL == hcc))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "lres's hcc is null");
        goto release_netbuf;
    }

    pst_cb_res = (pcie_cb_dma_res*)OAL_NETBUF_CB(pst_netbuf);

    if(OAL_UNLIKELY((0 == pst_cb_res->paddr.addr) || (0 == pst_cb_res->len)))
    {

        goto release_netbuf;
    }

    /*unmap pcie dma addr*/
    dma_unmap_single(&pst_pci_dev->dev, (dma_addr_t)pst_cb_res->paddr.addr, pst_cb_res->len, PCI_DMA_FROMDEVICE);
    if(pcie_dma_data_check_enable)
    {
        if(OAL_UNLIKELY(0xffffffff == oal_readl(OAL_NETBUF_DATA(pst_netbuf))))
        {
            DECLARE_DFT_TRACE_KEY_INFO("invalid_pcie_rx_netbuf", OAL_DFT_TRACE_FAIL);
            oal_print_hex_dump((oal_uint8 *)(OAL_NETBUF_DATA(pst_netbuf)), HCC_HDR_TOTAL_LEN, 32, "hdr ");
            oal_print_hex_dump((oal_uint8 *)(OAL_NETBUF_DATA(pst_netbuf) + HCC_HDR_TOTAL_LEN), HCC_HDR_TOTAL_LEN/2, 32, "payload ");
            oal_netbuf_free(pst_netbuf);
            oal_disable_pcie_irq(pst_pci_lres);
            /*DFR trigger*/
            oal_pcie_change_link_state(pst_pci_lres->pst_pci_res, PCI_WLAN_LINK_DOWN);
            //pst_pci_lres->pst_pci_res->link_state = PCI_WLAN_LINK_MEM_UP;
            hcc_bus_exception_submit(pst_pci_lres->pst_bus, WIFI_TRANS_FAIL);
        }
        else
        {
            hcc_rx_submit(hcc, pst_netbuf);
        }
    }
    else
    {
        hcc_rx_submit(hcc, pst_netbuf);
    }
    return;

release_netbuf:
    oal_pcie_release_rx_netbuf(pst_pci_res, pst_netbuf);
    DECLARE_DFT_TRACE_KEY_INFO("pcie release rx netbuf", OAL_DFT_TRACE_OTHER);
    return;
}

oal_int32 oal_pcie_unmap_tx_netbuf(oal_pcie_res* pst_pci_res, oal_netbuf_stru* pst_netbuf)
{
    /*dma_addr 存放在CB字段里*/
    pcie_cb_dma_res st_cb_dma;
    oal_pci_dev_stru *pst_pci_dev;

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);

    /*不是从CB的首地址开始，必须拷贝，对齐问题。*/
    oal_memcopy(&st_cb_dma, (oal_uint8*)OAL_NETBUF_CB(pst_netbuf) + sizeof(struct hcc_tx_cb_stru), sizeof(st_cb_dma));

#ifdef _PRE_PLAT_FEATURE_PCIE_DEBUG
    /*Debug*/
    OAL_MEMZERO((oal_uint8*)OAL_NETBUF_CB(pst_netbuf) + sizeof(struct hcc_tx_cb_stru), sizeof(st_cb_dma));
#endif

    /*unmap pcie dma addr*/
    if(OAL_LIKELY((0 != st_cb_dma.paddr.addr) && (0 != st_cb_dma.len)))
    {
        dma_unmap_single(&pst_pci_dev->dev, (dma_addr_t)st_cb_dma.paddr.addr, st_cb_dma.len, PCI_DMA_TODEVICE);
    } else {
        DECLARE_DFT_TRACE_KEY_INFO("pcie tx netbuf free fail", OAL_DFT_TRACE_FAIL);
        oal_print_hex_dump((oal_uint8*)OAL_NETBUF_CB(pst_netbuf), OAL_NETBUF_CB_SIZE(), 32, "invalid cb: ");
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}

oal_void oal_pcie_tx_netbuf_free(oal_pcie_res* pst_pci_res, oal_netbuf_stru* pst_netbuf)
{
    oal_pcie_unmap_tx_netbuf(pst_pci_res, pst_netbuf);

    hcc_tx_netbuf_free(pst_netbuf);
}

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_H2D_BYPASS
/*发送完成中断*/
oal_void oal_pcie_h2d_transfer_done(oal_pcie_res* pst_pci_res)
{
    oal_ulong flags;
    oal_int32  j, flag, cnt, total_cnt;
    /*tx fifo中获取 发送完成的首地址,双通道，双地址，双count*/
    edma_paddr_t addr[PCIE_EDMA_MAX_CHANNELS];
    oal_uint32 count[PCIE_EDMA_MAX_CHANNELS];

    pst_pci_res->stat.intx_tx_count++;

    if(OAL_TRUE != oal_pcie_edma_get_read_done_fifo(pst_pci_res, addr, count))
    {
        /*TBD:TBC*/
        /*待增加维测计数*/
        return;
    }

    PCI_PRINT_LOG(PCI_LOG_DBG, "oal_pcie_h2d_transfer_done, cnt:%u", pst_pci_res->stat.intx_tx_count);

    /*获取到发送完成的DMA地址，遍历发送队列,
      先遍历第一个元素，正常应该队头就是发送完成的元素，
      如果不在队头说明丢中断了(有FIFO正常不会丢),需要释放元素之前的netbuf*/
    flag = 0;
    total_cnt = 0;
    //for(i = 0; i < PCIE_H2D_QTYPE_BUTT; i++)
    //{
        /*DMA双通道*/
        for(j = 0; j < PCIE_EDMA_MAX_CHANNELS; j++)
        {

            PCI_PRINT_LOG(PCI_LOG_DBG, "tx chan:%d pa 0x%llx, cnt:%d", j, addr[j].addr, count[j]);

            cnt = count[j];/*无效描述符时,count为0*/
            if(!cnt)
            {
#if 0
                if(addr[j].addr)
                {
                    /*cnt 和 addr 应该同时为 0*/
                    PCI_PRINT_LOG(PCI_LOG_DBG, "tx chan:%d get invalid dma pa 0x%llx", j, addr[j].addr);
                }
#endif
                continue;
            }

           if(NULL == g_h2d_pst_netbuf)
           {
                PCI_PRINT_LOG(PCI_LOG_DBG, "g_h2d_pst_netbuf is null!");
                break;
           }
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_FIFO_ADDRESS
            if((oal_uint64)g_h2d_pci_dma_addr != (oal_uint64)addr[j].addr)
            {
                /*地址不匹配，遍历下一个队列*/
                PCI_PRINT_LOG(PCI_LOG_ERR, "unkown bypass netbuf addr:0x%llu, should be :0x%llu",(oal_uint64)g_h2d_pci_dma_addr, addr[j]);
                break;
            }
#endif

            g_h2d_bypass_pkt_num += cnt;
            pst_pci_res->st_tx_res[PCIE_H2D_QTYPE_NORMAL].stat.tx_done_count++;
            total_cnt += cnt;
        }

    if(OAL_UNLIKELY(total_cnt > PCIE_EDMA_READ_BUSRT_COUNT))
    {
        pst_pci_res->st_tx_res[PCIE_H2D_QTYPE_NORMAL].stat.tx_burst_cnt[0]++;
    }
    else
    {
        pst_pci_res->st_tx_res[PCIE_H2D_QTYPE_NORMAL].stat.tx_burst_cnt[total_cnt]++;
    }
        /*未匹配，遍历下一条队列*/
    //}
}
#else
/*发送完成中断*/
oal_void oal_pcie_h2d_transfer_done(oal_pcie_res* pst_pci_res)
{
    oal_ulong flags;
    oal_int32 i, j, flag, cnt, total_cnt, netbuf_cnt, curr_cnt;
    oal_netbuf_stru* pst_netbuf;
    oal_netbuf_head_stru* pst_txq;
    /*tx fifo中获取 发送完成的首地址,双通道，双地址，双count*/
    edma_paddr_t addr[PCIE_EDMA_MAX_CHANNELS];
    oal_uint32 count[PCIE_EDMA_MAX_CHANNELS];

    pst_pci_res->stat.intx_tx_count++;

    if(OAL_TRUE != oal_pcie_edma_get_read_done_fifo(pst_pci_res, addr, count))
    {
        /*TBD:TBC*/
        /*待增加维测计数*/
        return;
    }

    PCI_PRINT_LOG(PCI_LOG_DBG, "oal_pcie_h2d_transfer_done, cnt:%u", pst_pci_res->stat.intx_tx_count);

    /*获取到发送完成的DMA地址，遍历发送队列,
      先遍历第一个元素，正常应该队头就是发送完成的元素，
      如果不在队头说明丢中断了(有FIFO正常不会丢),需要释放元素之前的netbuf*/
    flag = 0;
    netbuf_cnt = 0;

    for(i = 0; i < PCIE_H2D_QTYPE_BUTT; i++)
    {
        total_cnt = 0;

        pst_txq = &pst_pci_res->st_tx_res[i].txq;

        if(oal_netbuf_list_empty(pst_txq))
        {
            /*next txq*/
            continue;
        }

        /*DMA双通道*/
        for(j = 0; j < PCIE_EDMA_MAX_CHANNELS; j++)
        {
            if(oal_netbuf_list_empty(pst_txq))
            {
                /*队列为空*/
                break;
            }

            PCI_PRINT_LOG(PCI_LOG_DBG, "[q:%d]tx chan:%d pa 0x%llx, cnt:%d",i , j, addr[j].addr, count[j]);

            cnt = count[j];/*无效描述符时,count为0*/
            if(!cnt)
            {
#if 0
                if(addr[j].addr)
                {
                    /*cnt 为0 时，addr 是上一次的值*/
                    PCI_PRINT_LOG(PCI_LOG_DBG, "tx chan:%d get invalid dma pa 0x%llx", j, addr[j].addr);
                }
#endif
                continue;
            }

            /*保证一个地方入队，这里出队*/
            pst_netbuf = (oal_netbuf_stru*)OAL_NETBUF_NEXT(pst_txq);
//#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_FIFO_ADDRESS
            if(OAL_TRUE != oal_pcie_tx_dma_addr_match(pst_netbuf, addr[j]))
            {
                /*地址不匹配，遍历下一个队列*/
                PCI_PRINT_LOG(PCI_LOG_DBG, "[q:%d]tx chan:%d match failed, search next txq",i , j);
                break;
            }
//#endif

            /*match succ.*/

            /*找到地址，出队,先入先出，所以先检查通道0，再检查通道1,
              2个通道的地址 应该在同一个队列中*/
            curr_cnt = oal_netbuf_list_len(pst_txq);
            if(OAL_UNLIKELY(cnt > curr_cnt))
            {
                /*count 出错?*/
                PCI_PRINT_LOG(PCI_LOG_ERR, "[q:%d]tx chan:%d tx done invalid count cnt %d ,list len %u",i , j,
                                        cnt, curr_cnt);
                DECLARE_DFT_TRACE_KEY_INFO("pcie tx done count error", OAL_DFT_TRACE_EXCEP);
                flag = 0;
                goto done;
            }

            total_cnt += cnt;

            do
            {
                /*这里的锁可以优化*/
                /*TBD:TBC*/
                oal_spin_lock_irq_save(&pst_pci_res->st_tx_res[i].lock, &flags);
                /*头部出队*/
                pst_netbuf = oal_netbuf_delist_nolock(pst_txq);
                oal_spin_unlock_irq_restore(&pst_pci_res->st_tx_res[i].lock, &flags);
                if(NULL == pst_netbuf)
                {
                    /*不应该为空，count有可能有问题*/
                    PCI_PRINT_LOG(PCI_LOG_ERR, "[q:%d]tx chan:%d tx netbuf queue underflow[cnt:%d:%d, qlen:%d]",i , j,
                        cnt, count[j], curr_cnt);
                    DECLARE_DFT_TRACE_KEY_INFO("pcie tx done count error2", OAL_DFT_TRACE_EXCEP);
                    flag = 0;
                    goto done;
                }

                /*unmap dma addr & free netbuf*/
                pst_pci_res->st_tx_res[i].stat.tx_done_count++;
                PCI_PRINT_LOG(PCI_LOG_DBG, "[q:%d]tx chan:%d, send netbuf ok, va:0x%p, cnt:%u",
                                                i , j, pst_netbuf, pst_pci_res->st_tx_res[i].stat.tx_done_count);
                oal_pcie_tx_netbuf_free(pst_pci_res, pst_netbuf);
            }while(--cnt);

            if(!cnt)
            {
                /*一个通道的地址处理完成*/
                PCI_PRINT_LOG(PCI_LOG_DBG, "[q:%d]tx chan:%d all bus process done!",i , j);
                flag = 1;
            }

        }

        netbuf_cnt += total_cnt;

        if(OAL_UNLIKELY(total_cnt > PCIE_EDMA_READ_BUSRT_COUNT))
        {
            pst_pci_res->st_tx_res[i].stat.tx_burst_cnt[0]++;
        }
        else
        {
            if(total_cnt)
                pst_pci_res->st_tx_res[i].stat.tx_burst_cnt[total_cnt]++;
        }

        pst_pci_res->st_tx_res[i].stat.tx_count += total_cnt;

        if(flag)
            break;

        /*未匹配，遍历下一条队列*/
    }

    if(OAL_UNLIKELY(netbuf_cnt != (count[0] + count[1])))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "count error total cnt:%u != dev reg count[0]%u  count[1] %u", netbuf_cnt, count[0], count[1]);
        oal_disable_pcie_irq((oal_pcie_linux_res*)oal_pci_get_drvdata(PCIE_RES_TO_DEV(pst_pci_res)));
        flag = 0;
        goto done;
    }

done:
    if(!flag)
    {
        /*维测,未找到FIFO中的地址，地址有错，或者count有错,或者丢中断
          这里应该触发DFR*/
        /*TBD:TBC*/
        //DECLARE_DFT_TRACE_KEY_INFO("pcie tx done addr error", OAL_DFT_TRACE_EXCEP);
        PCI_PRINT_LOG(PCI_LOG_ERR, "pcie tx done addr error");
    }
    else
    {
        oal_pcie_linux_res * pst_pci_lres = (oal_pcie_linux_res*)oal_pci_get_drvdata(PCIE_RES_TO_DEV(pst_pci_res));
        /*发送完成,唤醒发送线程*/
        if(OAL_LIKELY(NULL != pst_pci_lres))
        {
            PCI_PRINT_LOG(PCI_LOG_DBG, "pcie sched hcc thread, qid:%d", i);
            oal_atomic_set(&pst_pci_res->st_tx_res[i].tx_ringbuf_sync_cond, 1);/*下半部刷新，保持一致性*/
            hcc_sched_transfer(HBUS_TO_HCC(pst_pci_lres->pst_bus));
        }
    }

}
#endif

extern oal_int32 hcc_send_rx_queue_etc(struct hcc_handler *hcc, hcc_queue_type type);
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_D2H_BYPASS
/*接收完成中断*/
oal_void oal_pcie_d2h_transfer_done(oal_pcie_res* pst_pci_res)
{
    oal_ulong flags;
    oal_int32 i, flag, cnt, total_cnt;
    /*rx fifo中获取 发送完成的首地址,双通道，双地址，双count*/
    edma_paddr_t addr[PCIE_EDMA_MAX_CHANNELS];
    oal_uint32   count[PCIE_EDMA_MAX_CHANNELS];

    pst_pci_res->stat.intx_rx_count++;

    oal_pcie_mips_start(PCIE_MIPS_RX_MSG_FIFO);
    if(OAL_TRUE != oal_pcie_edma_get_write_done_fifo(pst_pci_res, addr, count))
    {
        /*TBD:TBC*/
        /*待增加维测计数*/
        return;
    }
    oal_pcie_mips_end(PCIE_MIPS_RX_MSG_FIFO);

    PCI_PRINT_LOG(PCI_LOG_DBG, "oal_pcie_d2h_transfer_done, cnt:%u", pst_pci_res->stat.intx_rx_count);

    /*获取到发送完成的DMA地址，遍历发送队列,
      先遍历第一个元素，正常应该队头就是发送完成的元素，
      如果不在队头说明丢中断了(有FIFO正常不会丢),需要释放元素之前的netbuf*/
    flag = 0;
    total_cnt = 0;

    /*DMA双通道*/
    for(i = 0; i < PCIE_EDMA_MAX_CHANNELS; i++)
    {
        PCI_PRINT_LOG(PCI_LOG_DBG, "rx chan:%d pa 0x%llx, cnt:%d",i , addr[i].addr, count[i]);
        cnt = count[i];/*无效描述符时,count为0*/
        if(!cnt)
        {
#if 0
            if(addr[i].addr)
            {
                /*cnt 和 addr 应该同时为 0*/
                PCI_PRINT_LOG(PCI_LOG_DBG, "rx chan:%d get invalid dma pa 0x%llx", i, addr[i].addr);
            }
#endif
            continue;
        }

        /*找到地址，出队,先入先出，所以先检查通道0，再检查通道1,
          2个通道的地址 应该在同一个队列中*/
        cnt = count[i];
        pst_pci_res->st_rx_res.stat.rx_done_count++;
        g_d2h_bypass_pkt_num += cnt;
        total_cnt += cnt;

    }

    if(OAL_UNLIKELY(total_cnt > PCIE_EDMA_WRITE_BUSRT_COUNT))
    {
        pst_pci_res->st_rx_res.stat.rx_burst_cnt[0]++;
    }
    else
    {
        pst_pci_res->st_rx_res.stat.rx_burst_cnt[total_cnt]++;
    }

    if(g_d2h_bypass_pkt_num >= g_d2h_bypass_total_pkt_num)
    {
        OAL_COMPLETE(&g_d2h_test_done);
    }

    oal_pcie_rx_ringbuf_bypass_supply(pst_pci_res, OAL_TRUE, OAL_TRUE, PCIE_RX_RINGBUF_SUPPLY_ALL);
}
#else
oal_uint32 g_rx_addr_count_err_cnt = 0;
/*接收完成中断*/
oal_void oal_pcie_d2h_transfer_done(oal_pcie_res* pst_pci_res)
{
    oal_ulong flags;
    oal_int32 i, flag, cnt, total_cnt;
    oal_netbuf_stru* pst_netbuf;
    oal_netbuf_head_stru* pst_rxq;
    /*rx fifo中获取 发送完成的首地址,双通道，双地址，双count*/
    edma_paddr_t addr[PCIE_EDMA_MAX_CHANNELS];
    oal_uint32   count[PCIE_EDMA_MAX_CHANNELS];

    pst_pci_res->stat.intx_rx_count++;

    oal_pcie_mips_start(PCIE_MIPS_RX_MSG_FIFO);
    if(OAL_TRUE != oal_pcie_edma_get_write_done_fifo(pst_pci_res, addr, count))
    {
        /*TBD:TBC*/
        /*待增加维测计数*/
        return;
    }
    oal_pcie_mips_end(PCIE_MIPS_RX_MSG_FIFO);

    PCI_PRINT_LOG(PCI_LOG_DBG, "oal_pcie_d2h_transfer_done, cnt:%u", pst_pci_res->stat.intx_rx_count);

    /*获取到发送完成的DMA地址，遍历发送队列,
      先遍历第一个元素，正常应该队头就是发送完成的元素，
      如果不在队头说明丢中断了(有FIFO正常不会丢),需要释放元素之前的netbuf*/
    flag = 0;
    total_cnt = 0;

    pst_rxq = &pst_pci_res->st_rx_res.rxq;

    if(oal_netbuf_list_empty(pst_rxq))
    {
        /*next txq*/
        DECLARE_DFT_TRACE_KEY_INFO("pcie rx done fifo error", OAL_DFT_TRACE_EXCEP);
        return;
    }

    /*DMA双通道*/
    for(i = 0; i < PCIE_EDMA_MAX_CHANNELS; i++)
    {
        if(oal_netbuf_list_empty(pst_rxq))
        {
            /*队列为空*/
            break;
        }

        PCI_PRINT_LOG(PCI_LOG_DBG, "rx chan:%d pa 0x%llx, cnt:%d",i , addr[i].addr, count[i]);
        cnt = count[i];/*无效描述符时,count为0*/
        if(!cnt)
        {
#if 0
            if(addr[i].addr)
            {
                /*cnt 和 addr 应该同时为 0*/
                PCI_PRINT_LOG(PCI_LOG_DBG, "rx chan:%d get invalid dma pa 0x%llx", i, addr[i].addr);
            }
#endif
            continue;
        }

        /*保证一个地方入队，这里出队*/
        pst_netbuf = (oal_netbuf_stru*)OAL_NETBUF_NEXT(pst_rxq);

        if(OAL_UNLIKELY(NULL == pst_netbuf))
        {
             oal_print_hi11xx_log(HI11XX_LOG_ERR,"%s error: pst_netbuf is null",__FUNCTION__);
             continue;
        };

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_FIFO_ADDRESS
        if(OAL_TRUE != oal_pcie_rx_dma_addr_match(pst_netbuf, addr[i]))
        {
            g_rx_addr_count_err_cnt++;
            if(g_rx_addr_count_err_cnt >= 2)
            {
                /*地址不匹配 重试一次*/
                PCI_PRINT_LOG(PCI_LOG_ERR, "rx chan:%d match failed, rx error, count[i]:%u, errcnt:%d",i, count[i], g_rx_addr_count_err_cnt);
                PCI_PRINT_LOG(PCI_LOG_ERR, "count0:%u, count1:%u",count[0], count[1]);
                DECLARE_DFT_TRACE_KEY_INFO("pcie rx addr fatal error", OAL_DFT_TRACE_EXCEP);
                flag = 0;
                goto done;
            }
            else
            {
                /*地址不匹配，出错*/
                PCI_PRINT_LOG(PCI_LOG_ERR, "rx chan:%d match failed, rx error, count[i]:%u, errcnt:%d",i, count[i], g_rx_addr_count_err_cnt);
                PCI_PRINT_LOG(PCI_LOG_ERR, "count0:%u, count1:%u",count[0], count[1]);
                DECLARE_DFT_TRACE_KEY_INFO("pcie rx addr error,retry", OAL_DFT_TRACE_FAIL);
                flag = 1;
                goto done;
            }
        }
        else
        {
            g_rx_addr_count_err_cnt = 0;
        }
#endif

        /*找到地址，出队,先入先出，所以先检查通道0，再检查通道1,
          2个通道的地址 应该在同一个队列中*/
        cnt = count[i];
        if(OAL_UNLIKELY(cnt > oal_netbuf_list_len(pst_rxq)))
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "rx chan:%d rx done invalid count cnt %d ,list len %u",i,
                                    cnt, oal_netbuf_list_len(pst_rxq));
            DECLARE_DFT_TRACE_KEY_INFO("pcie rx done count error", OAL_DFT_TRACE_EXCEP);
            flag = 0;
            goto done;
        }
        total_cnt += cnt;

        oal_pcie_mips_start(PCIE_MIPS_RX_QUEUE_POP);
        do
        {
            /*这里的锁可以优化*/
            /*TBD:TBC*/

            oal_spin_lock_irq_save(&pst_pci_res->st_rx_res.lock, &flags);
            /*头部出队*/
            pst_netbuf = oal_netbuf_delist_nolock(pst_rxq);
            oal_spin_unlock_irq_restore(&pst_pci_res->st_rx_res.lock, &flags);
            if(NULL == pst_netbuf)
            {
                oal_pcie_mips_end(PCIE_MIPS_RX_QUEUE_POP);
                /*不应该为空，count有可能有问题*/
                PCI_PRINT_LOG(PCI_LOG_ERR, "rx chan:%d tx netbuf queue underflow[cnt:%d, qlen:%d]",i ,
                    cnt, oal_netbuf_list_len(pst_rxq));
                DECLARE_DFT_TRACE_KEY_INFO("pcie rx done count error2", OAL_DFT_TRACE_EXCEP);
                flag = 0;
                goto done;
            }
            pst_pci_res->st_rx_res.stat.rx_done_count++;
            PCI_PRINT_LOG(PCI_LOG_DBG, "rx chan:%d, send netbuf ok, va:0x%p, cnt:%u",
                                            i ,  pst_netbuf, pst_pci_res->st_rx_res.stat.rx_done_count);
            if(PCI_DBG_CONDTION())
            {
                oal_print_hex_dump(OAL_NETBUF_DATA(pst_netbuf),
                                    OAL_NETBUF_LEN(pst_netbuf) < 128 ? OAL_NETBUF_LEN(pst_netbuf) : 128 ,
                                    16, "d2h payload: ");
            }
            /*unmap dma addr & free netbuf*/
            oal_pcie_rx_netbuf_submit(pst_pci_res, pst_netbuf);
        }while(--cnt);
        oal_pcie_mips_end(PCIE_MIPS_RX_QUEUE_POP);

        if(!cnt)
        {
            /*一个通道的地址处理完成*/
            PCI_PRINT_LOG(PCI_LOG_DBG, "rx chan:%d all bus process done!",i);
            flag = 1;
        }

    }

    if(OAL_UNLIKELY(total_cnt > PCIE_EDMA_WRITE_BUSRT_COUNT))
    {
        pst_pci_res->st_rx_res.stat.rx_burst_cnt[0]++;
    }
    else
    {
        pst_pci_res->st_rx_res.stat.rx_burst_cnt[total_cnt]++;
    }

    pst_pci_res->st_rx_res.stat.rx_count += total_cnt;

done:
    if(!flag)
    {
        /*维测,未找到FIFO中的地址，地址有错，或者count有错,或者丢中断
          这里应该触发DFR*/
        /*TBD:TBC*/
        //DECLARE_DFT_TRACE_KEY_INFO("pcie rx done addr error", OAL_DFT_TRACE_EXCEP);
        oal_pcie_linux_res * pst_pci_lres = (oal_pcie_linux_res*)oal_pci_get_drvdata(PCIE_RES_TO_DEV(pst_pci_res));
        PCI_PRINT_LOG(PCI_LOG_ERR, "pcie rx done addr error");
        oal_disable_pcie_irq(pst_pci_lres);

        /*DFR trigger*/
        oal_pcie_change_link_state(pst_pci_lres->pst_pci_res, PCI_WLAN_LINK_DOWN);
        //pst_pci_lres->pst_pci_res->link_state = PCI_WLAN_LINK_MEM_UP;
        hcc_bus_exception_submit(pst_pci_lres->pst_bus, WIFI_TRANS_FAIL);

        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_D2H_TRANSFER_PCIE_LINK_DOWN);
    }
    else
    {
        oal_pcie_linux_res * pst_pci_lres;
        PCI_PRINT_LOG(PCI_LOG_DBG, "d2h trigger hcc_sched_transfer, dev:%p, lres:%p",
                       PCIE_RES_TO_DEV(pst_pci_res),  oal_pci_get_drvdata(PCIE_RES_TO_DEV(pst_pci_res)));
#if 0
        oal_pcie_mips_start(PCIE_MIPS_RX_MEM_FREE);
        hcc_send_rx_queue_etc(hcc_get_110x_handler(),DATA_LO_QUEUE);
        oal_pcie_mips_end(PCIE_MIPS_RX_MEM_FREE);
#endif

        pst_pci_lres = (oal_pcie_linux_res*)oal_pci_get_drvdata(PCIE_RES_TO_DEV(pst_pci_res));
        /*发送完成,唤醒发送线程*/
        if(OAL_LIKELY(NULL != pst_pci_lres))
        {
            if(OAL_LIKELY(pst_pci_lres->pst_bus))
                hcc_sched_transfer(HBUS_TO_HCC(pst_pci_lres->pst_bus));
            else
                PCI_PRINT_LOG(PCI_LOG_ERR, "lres's bus is null! %p", pst_pci_lres);

            /*通知线程，补充RX内存*/
            oal_pcie_shced_rx_hi_thread(pst_pci_res);
        }
    }
}
#endif

/*edma read isr*/
oal_void oal_pcie_h2d_edma_isr(oal_pcie_res* pst_pci_res)
{
    oal_int32 ret;
    oal_ulong flags;
    oal_int32 i, cnt, total_cnt, netbuf_cnt, curr_cnt;
    oal_netbuf_stru* pst_netbuf;
    oal_netbuf_head_stru* pst_txq;
    pcie_dma_read_fifo_item  soft_rd_item;
    oal_pcie_linux_res * pst_pci_lres = (oal_pcie_linux_res*)oal_pci_get_drvdata(PCIE_RES_TO_DEV(pst_pci_res));

    PCI_PRINT_LOG(PCI_LOG_DBG, "oal_pcie_h2d_edma_isr enter");

    /*读空soft fifo*/
    for(;;)
    {
        if(OAL_SUCC != oal_pcie_ringbuf_read_wr(pst_pci_res, PCIE_COMM_RINGBUF_DMA_READ_FIFO))
        {
            PCI_PRINT_LOG(PCI_LOG_WARN, "read dma read fifo ringbuf failed");
            break;
        }

        cnt = 0;

        for(;;)
        {
            ret = oal_pcie_ringbuf_read(pst_pci_res, PCIE_COMM_RINGBUF_DMA_READ_FIFO, (oal_uint8*)&soft_rd_item, sizeof(soft_rd_item));
            if(ret <= 0)
            {
                break;
            }

            pst_pci_res->stat.intx_tx_count++;

            PCI_PRINT_LOG(PCI_LOG_DBG, "oal_pcie_h2d_edma_isr, cnt:%u", pst_pci_res->stat.intx_tx_count);
            if((oal_uint16)pst_pci_res->stat.intx_tx_count != soft_rd_item.isr_count)
            {
                PCI_PRINT_LOG(PCI_LOG_WARN, "tx count:%u not equal to isr count:%u", (oal_uint16)pst_pci_res->stat.intx_tx_count, soft_rd_item.isr_count);
            }

            if(OAL_UNLIKELY(soft_rd_item.qid >= (oal_uint16)PCIE_H2D_QTYPE_BUTT))
            {
                PCI_PRINT_LOG(PCI_LOG_ERR, "invalid h2d qid:%u", soft_rd_item.qid);
                oal_print_hex_dump((oal_uint8 *)&soft_rd_item, OAL_SIZEOF(soft_rd_item), 32, "read item: ");
                break;
            }

            pst_txq = &pst_pci_res->st_tx_res[soft_rd_item.qid].txq;
            if(OAL_UNLIKELY(oal_netbuf_list_empty(pst_txq)))
            {
                /*队列为空*/
                PCI_PRINT_LOG(PCI_LOG_ERR,"invalid read item,qid:%u is empty", soft_rd_item.qid);
                oal_print_hex_dump((oal_uint8 *)&soft_rd_item, OAL_SIZEOF(soft_rd_item), 32, "read item: ");
                break;
            }

            total_cnt = 0;

            for(i = 0; i < PCIE_EDMA_MAX_CHANNELS; i++)
            {
                total_cnt += soft_rd_item.count[i];
            }

            netbuf_cnt = oal_netbuf_list_len(pst_txq);

            if(OAL_UNLIKELY( netbuf_cnt < total_cnt))
            {
                /*队列为空*/
                PCI_PRINT_LOG(PCI_LOG_ERR, "invalid read item,qid:%u had %u pkts less than %u", soft_rd_item.qid, netbuf_cnt, total_cnt);
                oal_print_hex_dump((oal_uint8 *)&soft_rd_item, OAL_SIZEOF(soft_rd_item), 32, "read item: ");
                break;
            }

            /*保证一个地方入队，这里出队*/
            pst_netbuf = (oal_netbuf_stru*)OAL_NETBUF_NEXT(pst_txq);
            if(OAL_TRUE != oal_pcie_tx_dma_addr_match_low(pst_netbuf, soft_rd_item.address))
            {
                PCI_PRINT_LOG(PCI_LOG_ERR, "[q:%d]tx queue match failed", soft_rd_item.qid);
                oal_print_hex_dump((oal_uint8 *)&soft_rd_item, OAL_SIZEOF(soft_rd_item), 32, "read item: ");
                break;
            }

            curr_cnt = total_cnt;

            do
            {
                /*头部出队*/
                oal_spin_lock_irq_save(&pst_pci_res->st_tx_res[soft_rd_item.qid].lock, &flags);
                pst_netbuf = oal_netbuf_delist_nolock(pst_txq);
                oal_spin_unlock_irq_restore(&pst_pci_res->st_tx_res[soft_rd_item.qid].lock, &flags);
                if(OAL_UNLIKELY(NULL == pst_netbuf))
                {
                    /*不应该为空，count有可能有问题*/
                    PCI_PRINT_LOG(PCI_LOG_ERR, "[q:%d]tx netbuf queue underflow[curr_cnt:%d:%d], qlen:%d",soft_rd_item.qid, curr_cnt, total_cnt, netbuf_cnt);
                    DECLARE_DFT_TRACE_KEY_INFO("pcie tx done count error", OAL_DFT_TRACE_EXCEP);
                    return;
                }

                /*unmap dma addr & free netbuf*/
                pst_pci_res->st_tx_res[soft_rd_item.qid].stat.tx_done_count++;
                PCI_PRINT_LOG(PCI_LOG_DBG, "[q:%d]send netbuf ok, va:0x%p, cnt:%u",
                                                soft_rd_item.qid , pst_netbuf, pst_pci_res->st_tx_res[soft_rd_item.qid].stat.tx_done_count);
                oal_pcie_tx_netbuf_free(pst_pci_res, pst_netbuf);
            }while(--total_cnt);


            /*发送完成,唤醒发送线程*/
            if(OAL_LIKELY(NULL != pst_pci_lres))
            {
                PCI_PRINT_LOG(PCI_LOG_DBG, "pcie sched hcc thread, qid:%d", soft_rd_item.qid);

                oal_atomic_set(&pst_pci_res->st_tx_res[soft_rd_item.qid].tx_ringbuf_sync_cond, 1);/*下半部刷新，保持一致性*/
                hcc_sched_transfer(HBUS_TO_HCC(pst_pci_lres->pst_bus));
            }
            /*get read item*/
            cnt++;
        }

        if(0 == cnt)
        {
            PCI_PRINT_LOG(PCI_LOG_DBG, "h2d soft fifo is empty, break");
            break;
        }

    }

}

/*edam write isr*/
oal_void oal_pcie_d2h_edma_isr(oal_pcie_res* pst_pci_res)
{
    oal_int32 ret;
    oal_ulong flags;
    oal_int32 i, flag, cnt, total_cnt, netbuf_cnt, curr_cnt;
    oal_netbuf_stru* pst_netbuf;
    oal_netbuf_head_stru* pst_rxq;
    pcie_dma_write_fifo_item  soft_wr_item;

    oal_pcie_linux_res * pst_pci_lres = (oal_pcie_linux_res*)oal_pci_get_drvdata(PCIE_RES_TO_DEV(pst_pci_res));

    flag = 0;
    total_cnt = 0;

    pst_rxq = &pst_pci_res->st_rx_res.rxq;

    PCI_PRINT_LOG(PCI_LOG_DBG, "oal_pcie_d2h_edma_isr enter");

    /*读空soft write fifo*/
    for(;;)
    {
        if(OAL_SUCC != oal_pcie_ringbuf_read_wr(pst_pci_res, PCIE_COMM_RINGBUF_DMA_WRITE_FIFO))
        {
            PCI_PRINT_LOG(PCI_LOG_WARN, "read dma write fifo ringbuf failed");
            break;
        }

        cnt = 0;
        for(;;)
        {
            ret = oal_pcie_ringbuf_read(pst_pci_res, PCIE_COMM_RINGBUF_DMA_WRITE_FIFO, (oal_uint8*)&soft_wr_item, sizeof(soft_wr_item));
            if(ret <= 0)
            {
                break;
            }

            pst_pci_res->stat.intx_rx_count++;
            PCI_PRINT_LOG(PCI_LOG_DBG, "oal_pcie_d2h_edma_isr, cnt:%u", pst_pci_res->stat.intx_rx_count);

            if((oal_uint16)pst_pci_res->stat.intx_rx_count != soft_wr_item.isr_count)
            {
                PCI_PRINT_LOG(PCI_LOG_WARN, "rx count:%u not equal to isr count:%u", (oal_uint16)pst_pci_res->stat.intx_rx_count, soft_wr_item.isr_count);
            }

            if(oal_netbuf_list_empty(pst_rxq))
            {
                break;
            }

            total_cnt = 0;
            for(i = 0; i < PCIE_EDMA_MAX_CHANNELS; i++)
            {
                total_cnt += soft_wr_item.count[i];
            }

            netbuf_cnt = oal_netbuf_list_len(pst_rxq);

            if(OAL_UNLIKELY( netbuf_cnt < total_cnt))
            {
                /*队列为空*/
                PCI_PRINT_LOG(PCI_LOG_ERR,"invalid write item, had %u pkts less than %u", netbuf_cnt, total_cnt);
                oal_print_hex_dump((oal_uint8 *)&soft_wr_item, OAL_SIZEOF(soft_wr_item), 32, "write item: ");
                flag = 0;
                goto done;
            }

            /*保证一个地方入队，这里出队*/
            pst_netbuf = (oal_netbuf_stru*)OAL_NETBUF_NEXT(pst_rxq);
            if(OAL_UNLIKELY(NULL == pst_netbuf))
            {
                 PCI_PRINT_LOG(PCI_LOG_ERR,"%s error: pst_netbuf is null",__FUNCTION__);
                 goto done;
            };

            if(OAL_TRUE != oal_pcie_rx_dma_addr_matchlow(pst_netbuf, soft_wr_item.address))
            {
                PCI_PRINT_LOG(PCI_LOG_ERR, "rx queue match failed");
                oal_print_hex_dump((oal_uint8 *)&soft_wr_item, OAL_SIZEOF(soft_wr_item), 32, "write item: ");
                flag = 0;
                goto done;
            }

            curr_cnt = total_cnt;

            /*get the rx netbuf list*/
            do
            {
                /*这里的锁可以优化*/
                /*TBD:TBC*/

                oal_spin_lock_irq_save(&pst_pci_res->st_rx_res.lock, &flags);
                /*头部出队*/
                pst_netbuf = oal_netbuf_delist_nolock(pst_rxq);
                oal_spin_unlock_irq_restore(&pst_pci_res->st_rx_res.lock, &flags);
                if(NULL == pst_netbuf)
                {
                    /*不应该为空，count有可能有问题*/
                    PCI_PRINT_LOG(PCI_LOG_ERR, "rx  netbuf queue underflow[netbuf_cnt:%d, total_count:%d]",
                        netbuf_cnt, total_cnt);
                    DECLARE_DFT_TRACE_KEY_INFO("pcie rx edma done count error", OAL_DFT_TRACE_EXCEP);
                    flag = 0;
                    goto done;
                }
                pst_pci_res->st_rx_res.stat.rx_done_count++;
                PCI_PRINT_LOG(PCI_LOG_DBG, "recv netbuf ok, va:0x%p, cnt:%u",
                                                 pst_netbuf, pst_pci_res->st_rx_res.stat.rx_done_count);
                if(PCI_DBG_CONDTION())
                {
                    oal_print_hex_dump(OAL_NETBUF_DATA(pst_netbuf),
                                        OAL_NETBUF_LEN(pst_netbuf) < 128 ? OAL_NETBUF_LEN(pst_netbuf) : 128 ,
                                        16, "d2h payload: ");
                }
                /*unmap dma addr & free netbuf*/
                oal_pcie_rx_netbuf_submit(pst_pci_res, pst_netbuf);
            }while(--curr_cnt);

            if(OAL_UNLIKELY(total_cnt > PCIE_EDMA_WRITE_BUSRT_COUNT))
            {
                pst_pci_res->st_rx_res.stat.rx_burst_cnt[0]++;
            }
            else
            {
                pst_pci_res->st_rx_res.stat.rx_burst_cnt[total_cnt]++;
            }

            cnt++;
        }

        if(0 == cnt)
        {
            PCI_PRINT_LOG(PCI_LOG_DBG, "d2h soft fifo is empty, break");
            break;
        }
    }

    flag = 1;

done:
    if(!flag)
    {
        /*维测,未找到FIFO中的地址，地址有错，或者count有错,或者丢中断
          这里应该触发DFR*/
        /*TBD:TBC*/
        //DECLARE_DFT_TRACE_KEY_INFO("pcie rx done addr error", OAL_DFT_TRACE_EXCEP);
        PCI_PRINT_LOG(PCI_LOG_ERR, "pcie rx done addr error");
        oal_disable_pcie_irq(pst_pci_lres);

        /*DFR trigger*/
        oal_pcie_change_link_state(pst_pci_lres->pst_pci_res, PCI_WLAN_LINK_DOWN);
        //pst_pci_lres->pst_pci_res->link_state = PCI_WLAN_LINK_MEM_UP;
        hcc_bus_exception_submit(pst_pci_lres->pst_bus, WIFI_TRANS_FAIL);

        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_D2H_EDMA_PCIE_LINK_DOWN);
    }
    else
    {
        PCI_PRINT_LOG(PCI_LOG_DBG, "d2h trigger hcc_sched_transfer, dev:%p, lres:%p",
                       PCIE_RES_TO_DEV(pst_pci_res),  oal_pci_get_drvdata(PCIE_RES_TO_DEV(pst_pci_res)));
        /*发送完成,唤醒发送线程*/
        if(OAL_LIKELY(NULL != pst_pci_lres))
        {
            if(OAL_LIKELY(pst_pci_lres->pst_bus))
                hcc_sched_transfer(HBUS_TO_HCC(pst_pci_lres->pst_bus));
            else
                PCI_PRINT_LOG(PCI_LOG_ERR, "lres's bus is null! %p", pst_pci_lres);

            /*通知线程，补充RX内存*/
            oal_pcie_shced_rx_hi_thread(pst_pci_res);
        }
    }
}

#ifdef _PRE_PLAT_FEATURE_PCIE_EDMA_ORI
/*原生EDMA, Not Finish*/
oal_int32 oal_pcie_transfer_done(oal_pcie_res* pst_pci_res)
{
    oal_int32 flag, trans_cnt;
    MSG_FIFO_STAT msg_fifo_stat;

    HOST_INTR_STATUS stat, mask;

    if(OAL_UNLIKELY(NULL == pst_pci_res))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_res is null!");
        return;
    }

    pst_pci_res->stat.intx_total_count++;
    PCI_PRINT_LOG(PCI_LOG_DBG, "intx int count:%u", pst_pci_res->stat.intx_total_count);

    /*Host收到intx中断,遍历TX/RX FIFO寄存器*/
    if(OAL_UNLIKELY(NULL == pst_pci_res->pst_pci_dma_ctrl_base))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "fifo base addr is null!");
        pst_pci_res->stat.done_err_cnt++;
        return;
    }

    mask.bits.pcie_edma_tx_intr_status = 1;
    mask.bits.pcie_edma_rx_intr_status = 1;

    trans_cnt = 0;

    do
    {
        stat.AsDword = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_HOST_INTR_STATUS_OFF);

        stat.AsDword &= mask.AsDword;/*Get mask int*/

        if(0 == stat.AsDword)
        {
            break;
        }


        if(stat.bits.pcie_edma_rx_intr_status)
        {
            /*获取当前接收的描述符个数,释放当前队头的netbuf*/
            #error
            /*device to host edma 传输完成, 触发h2d doorbell通知DEVICE 查中断*/
            oal_pcie_h2d_doorbell(pst_pci_res);
            trans_cnt++;
        }

        if(stat.bits.pcie_edma_tx_intr_status)
        {
            //oal_pcie_h2d_transfer_done(pst_pci_res);
            trans_cnt++;
        }

        //PCI_PRINT_LOG(PCI_LOG_DBG, "clear host:0x%8x", stat.AsDword);
        //oal_writel(stat.AsDword >> 2, (pst_pci_res->pst_pci_ctrl_base + PCIE_HOST_INTR_CLR));

    }while(1);
}
#else
oal_int32 oal_pcie_transfer_done(oal_pcie_res* pst_pci_res)
{
//    oal_int32 total_cnt = 0;
    oal_int32 trans_cnt, old_cnt;
    MSG_FIFO_STAT msg_fifo_stat;

    /*这里的mask 只是mask 状态位，并不是mask中断，
      这里的mask只用来标记是否处理这个中断*/
    HOST_INTR_STATUS stat, mask;

    if(OAL_UNLIKELY(NULL == pst_pci_res))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_res is null!");
        return 0;
    }

    pst_pci_res->stat.intx_total_count++;
    PCI_PRINT_LOG(PCI_LOG_DBG, "intx int count:%u", pst_pci_res->stat.intx_total_count);

    /*Host收到intx中断,遍历TX/RX FIFO寄存器*/
    if(OAL_UNLIKELY(NULL == pst_pci_res->pst_pci_dma_ctrl_base))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "fifo base addr is null!");
        pst_pci_res->stat.done_err_cnt++;
        return 0;
    }

    if(OAL_UNLIKELY(pst_pci_res->link_state < PCI_WLAN_LINK_MEM_UP))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "d2h int link invaild, link_state:%s", oal_pcie_get_link_state_str((pst_pci_res->link_state)));
        pst_pci_res->stat.done_err_cnt++;
        return 0;
    }

    /*
    mask.AsDword = 0x0;
    mask.bits.pcie_hw_edma_rx_intr_status = 1;
    mask.bits.pcie_hw_edma_tx_intr_status = 1;
    */
    mask.AsDword = 0xc;

    trans_cnt = 0;
    old_cnt = trans_cnt;

    do
    {
        if(OAL_UNLIKELY(pst_pci_res->link_state < PCI_WLAN_LINK_MEM_UP))
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "link state is disabled:%s, intx can't process!", oal_pcie_get_link_state_str(pst_pci_res->link_state));
            DECLARE_DFT_TRACE_KEY_INFO("pcie mem is not init", OAL_DFT_TRACE_OTHER);
        }

        oal_pcie_mips_start(PCIE_MIPS_RX_INTR_PROCESS);
        stat.AsDword = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_HOST_INTR_STATUS_OFF);
        PCI_PRINT_LOG(PCI_LOG_DBG, "intr status host:0x%8x", stat.AsDword);

        /*Link down check*/
        if(OAL_UNLIKELY(0xFFFFFFFF == stat.AsDword))
        {
            if(OAL_FALSE == oal_pcie_check_link_state(pst_pci_res))
            {
                PCI_PRINT_LOG(PCI_LOG_ERR, "read transfer done int failed, linkdown..");
                return -OAL_EBUSY;
            }
        }

        stat.AsDword &= mask.AsDword;/*Get mask int*/

        if(0 == stat.AsDword)
        {
            oal_pcie_mips_end(PCIE_MIPS_RX_INTR_PROCESS);
            break;
        }

        PCI_PRINT_LOG(PCI_LOG_DBG, "clear host:0x%8x", stat.AsDword);
        oal_writel(stat.AsDword, (pst_pci_res->pst_pci_ctrl_base + PCIE_HOST_INTR_CLR));
        if(pcie_ringbuf_bugfix_enable)
        {
            oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_HOST_DEVICE_REG0);/*force read, util the clear intr effect*/
        }
        oal_pcie_mips_end(PCIE_MIPS_RX_INTR_PROCESS);

        if((pst_pci_res->revision >= PCIE_REVISION_5_00A) && (pcie_soft_fifo_enable))
        {
            /*读空Soft FIFO*/
            if(stat.bits.pcie_hw_edma_tx_intr_status)
            {
                oal_pcie_h2d_edma_isr(pst_pci_res);
            }

            if(stat.bits.pcie_hw_edma_rx_intr_status)
            {
                oal_pcie_d2h_edma_isr(pst_pci_res);
            }
        }
        else
        {
            /*读空Hardware FIFO*/
            for(;;)
            {
                old_cnt = trans_cnt;
                oal_pcie_mips_start(PCIE_MIPS_RX_FIFO_STATUS);
                msg_fifo_stat.AsDword = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_MSG_FIFO_STATUS);
                oal_pcie_mips_end(PCIE_MIPS_RX_FIFO_STATUS);
                PCI_PRINT_LOG(PCI_LOG_DBG, "msg_fifo_stat host:0x%8x", msg_fifo_stat.AsDword);

                /*Link down check*/
                if(OAL_UNLIKELY(0xFFFFFFFF == msg_fifo_stat.AsDword))
                {
                    if(OAL_FALSE == oal_pcie_check_link_state(pst_pci_res))
                    {
                        PCI_PRINT_LOG(PCI_LOG_ERR, "read message fifo stat failed, linkdown..");
                        return -OAL_EBUSY;
                    }
                }

                if(0 == msg_fifo_stat.bits.rx_msg_fifo0_empty && 0 == msg_fifo_stat.bits.rx_msg_fifo1_empty)
                {
                    oal_pcie_d2h_transfer_done(pst_pci_res);
                    trans_cnt++;
                }

                if(0 == msg_fifo_stat.bits.tx_msg_fifo0_empty && 0 == msg_fifo_stat.bits.tx_msg_fifo1_empty)
                {
                    oal_pcie_h2d_transfer_done(pst_pci_res);
                    trans_cnt++;
                }

                if(old_cnt == trans_cnt)
                    break;
                else
                {
                    //if(++total_cnt > 20)
                    //    break;/*防止中断里面循环次数太多 影响系统中断处理*/
                }
            }
        }

    }while(1);

    PCI_PRINT_LOG(PCI_LOG_DBG, "trans done process %u cnt data", trans_cnt);

    /*相等说明已经读空*/
    return !(old_cnt == trans_cnt);
}
#endif

oal_void oal_pcie_print_ringbuf_info(pcie_ringbuf* pst_ringbuf, PCI_LOG_TYPE level)
{
    if(OAL_WARN_ON(NULL == pst_ringbuf))
    {
        return;
    }

    /*dump the ringbuf info*/
    PCI_PRINT_LOG(level, "ringbuf[0x%p] idx:%u, rd:%u, wr:%u, size:%u, item_len:%u, item_mask:0x%x, base_addr:0x%llx",
                    pst_ringbuf,
                    pst_ringbuf->idx,
                    pst_ringbuf->rd,
                    pst_ringbuf->wr,
                    pst_ringbuf->size,
                    pst_ringbuf->item_len,
                    pst_ringbuf->item_mask,
                    pst_ringbuf->base_addr);
}

/*ringbuf functions*/
oal_uint32 oal_pcie_ringbuf_freecount(pcie_ringbuf* pst_ringbuf)
{
    /*无符号，已经考虑了翻转*/
    oal_uint32 len = pst_ringbuf->size - (pst_ringbuf->wr - pst_ringbuf->rd);
    if(0 == len)
    {
        return 0;
    }

    if(len % pst_ringbuf->item_len)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "oal_pcie_ringbuf_freecount, size:%u, wr:%u, rd:%u",
                        pst_ringbuf->size,
                        pst_ringbuf->wr,
                        pst_ringbuf->rd);
        return 0;
    }

    if(pst_ringbuf->item_mask)
    {
        /*item len 如果是2的N次幂，则移位*/
        len = len >> pst_ringbuf->item_mask;
    }
    else
    {
        len /= pst_ringbuf->item_len;
    }
    return len;
}

oal_uint32 oal_pcie_ringbuf_is_empty(pcie_ringbuf* pst_ringbuf)
{
    if(pst_ringbuf->wr == pst_ringbuf->rd)
    {
        return OAL_TRUE;
    }
    return OAL_FALSE;
}

oal_int32 oal_pcie_check_link_state(oal_pcie_res* pst_pci_res)
{
    oal_int32 ret = -OAL_EFAIL;
    pci_addr_map addr_map;
    pcie_dev_ptr share_mem_address;/*Device cpu地址*/
    oal_pci_dev_stru *pst_pci_dev;

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, PCIE_DEV_SHARE_MEM_CPU_ADDRESS, &addr_map);
    if(OAL_UNLIKELY(OAL_SUCC != ret))
    {
        /*share mem 地址未映射!*/
        PCI_PRINT_LOG(PCI_LOG_ERR, "can not found mem map for dev cpu address 0x%x\n", PCIE_DEV_SHARE_MEM_CPU_ADDRESS);
        return OAL_FALSE;
    }

    /*Get sharemem's dev_cpu address*/
    oal_pcie_memcopy((oal_ulong)&share_mem_address, (oal_ulong)addr_map.va, sizeof(share_mem_address));

    if(share_mem_address == 0xFFFFFFFF)
    {
        oal_uint32 version = 0;

        DECLARE_DFT_TRACE_KEY_INFO("pcie_detect_linkdown", OAL_DFT_TRACE_EXCEP);

        ret = oal_pci_read_config_dword(PCIE_RES_TO_DEV(pst_pci_res), 0x0, &version);
        PCI_PRINT_LOG(PCI_LOG_INFO, "read pci version 0x%8x ret=%d, host wakeup dev gpio:%d", version, ret,
                        hcc_bus_get_sleep_state(((oal_pcie_linux_res*)oal_pci_get_drvdata(PCIE_RES_TO_DEV(pst_pci_res)))->pst_bus));
        do
        {
           oal_pcie_linux_res * pst_pci_lres = (oal_pcie_linux_res*)oal_pci_get_drvdata(PCIE_RES_TO_DEV(pst_pci_res));

		   oal_pcie_change_link_state(pst_pci_lres->pst_pci_res, PCI_WLAN_LINK_DOWN);

#if defined(_PRE_CONFIG_GPIO_TO_SSI_DEBUG)
           if(0 == board_get_wlan_wkup_gpio_val_etc())
           {
                if(HI1XX_ANDROID_BUILD_VARIANT_USER == hi11xx_get_android_build_variant())
                {
                    PCI_PRINT_LOG(PCI_LOG_INFO, "user mode, bypass the dump info");
                }
                else
                {
		            ssi_dump_device_regs(SSI_MODULE_MASK_COMM | SSI_MODULE_MASK_PCIE_CFG);
                }
           }
           else
           {
                PCI_PRINT_LOG(PCI_LOG_INFO, "dev wakeup gpio is high, dev maybe panic");
           }
#endif
           oal_pci_disable_device(PCIE_RES_TO_DEV(pst_pci_res));
           oal_disable_pcie_irq(pst_pci_lres);
        }while(0);
        return OAL_FALSE;
    }
    else
    {
        return OAL_TRUE;
    }
}

oal_void oal_pcie_share_mem_res_unmap(oal_pcie_res* pst_pci_res)
{
    OAL_MEMZERO((oal_void*)&pst_pci_res->dev_share_mem, OAL_SIZEOF(pst_pci_res->dev_share_mem));
}

/*调用必须在iATU配置, pcie device 使能之后，*/
oal_int32 oal_pcie_share_mem_res_map(oal_pcie_res* pst_pci_res)
{
    oal_int32 ret = -OAL_EFAIL;
    oal_void* pst_share_mem_vaddr;
    pcie_dev_ptr share_mem_address = 0xFFFFFFFF;/*Device cpu地址*/
    pci_addr_map addr_map, share_mem_map;
    unsigned long timeout, timeout1;
    oal_pci_dev_stru *pst_pci_dev;

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);

    /*忙等50ms 若超时 再等10S 超时*/
    timeout  = jiffies + msecs_to_jiffies(50);/*50ms*/
    timeout1 = jiffies + msecs_to_jiffies(10000);/*10s*/

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, PCIE_DEV_SHARE_MEM_CPU_ADDRESS, &addr_map);
    if(OAL_SUCC != ret)
    {
        /*share mem 地址未映射!*/
        PCI_PRINT_LOG(PCI_LOG_ERR, "can not found mem map for dev cpu address 0x%x\n", PCIE_DEV_SHARE_MEM_CPU_ADDRESS);
        return ret;
    }

    pst_share_mem_vaddr = (oal_void*)addr_map.va;

    PCI_PRINT_LOG(PCI_LOG_DBG, "device address:0x%x = va:0x%lx", PCIE_DEV_SHARE_MEM_CPU_ADDRESS, addr_map.va);

    for(;;)
    {
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        /*cache 无效化*/
        oal_pci_cache_inv(pst_pci_dev, (oal_void*)addr_map.pa, sizeof(pcie_dev_ptr));
#endif

        /*Get sharemem's dev_cpu address*/
        oal_pcie_memcopy((oal_ulong)&share_mem_address, (oal_ulong)pst_share_mem_vaddr, sizeof(share_mem_address));

        /*通过检查地址转换可以判断读出的sharemem地址是否是有效值*/
        ret = oal_pcie_inbound_ca_to_va(pst_pci_res, share_mem_address, &share_mem_map);
        if(OAL_SUCC == ret)
        {
            /*Device 初始化完成  & PCIE 通信正常*/
            if(share_mem_address != 0)/*TBD:TBC*/
            {
                /*TBD:TBC*/
                if(0xFFFFFFFF != share_mem_address)
                {
                    PCI_PRINT_LOG(PCI_LOG_INFO, "share_mem_address 0x%x", share_mem_address);
                    ret = OAL_SUCC;
                    break;
                }
            }
        }

        if(!time_after(jiffies, timeout))
        {
            cpu_relax();
            continue;/*未超时，继续*/
        }

        /*50ms 超时, 开始10S超时探测*/
        if(!time_after(jiffies, timeout1))
        {
            oal_msleep(1);
            continue;/*未超时，继续*/
        } else {
            /*10s+50ms 超时，退出*/
            PCI_PRINT_LOG(PCI_LOG_ERR, "share_mem_address 0x%x, jiffies:0x%lx, timeout:0x%lx, timeout1:0x%lx", share_mem_address, jiffies, timeout, timeout1);
            ret = -OAL_ETIMEDOUT;
            break;
        }

    }

    if(0 != share_mem_map.va && (OAL_SUCC == ret))
    {
        pst_pci_res->dev_share_mem.va = share_mem_map.va;
        pst_pci_res->dev_share_mem.pa = share_mem_map.pa;
        PCI_PRINT_LOG(PCI_LOG_DBG, "share mem va:0x%lx, pa:0x%lx",pst_pci_res->dev_share_mem.va, pst_pci_res->dev_share_mem.pa);
        return OAL_SUCC;
    }

    PCI_PRINT_LOG(PCI_LOG_INFO, "wait device & PCIe boot timeout, 0x%x", share_mem_address);

    if(HI1XX_ANDROID_BUILD_VARIANT_USER == hi11xx_get_android_build_variant())
    {
        if(oal_print_rate_limit(PRINT_RATE_HOUR))
        {
            ssi_dump_device_regs(SSI_MODULE_MASK_ARM_REG);
        }
        else
        {
            PCI_PRINT_LOG(PCI_LOG_INFO,"bypass ssi dump");
        }
    }
    else
    {
        ssi_dump_device_regs(SSI_MODULE_MASK_COMM);
    }

    return -OAL_EFAIL;

}

/*device shared mem write*/
oal_int32 oal_pcie_write_dsm32(oal_pcie_res* pst_pci_res, PCIE_SHARED_DEVICE_ADDR_TYPE type, oal_uint32 val)
{
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev;
    pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
#endif

    if(OAL_WARN_ON((oal_uint32)type >= (oal_uint32)PCIE_SHARED_ADDR_BUTT))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "invalid device addr type:%d", type);
        return -OAL_EINVAL;
    }

    if(OAL_WARN_ON(NULL == pst_pci_res))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pci res is null");
        return -OAL_ENODEV;
    }

    if(OAL_WARN_ON(0 == pst_pci_res->st_device_shared_addr_map[type].va))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "dsm type:%d va is null", type);
        return -OAL_ENODEV;
    }

    oal_writel(val, (void*)pst_pci_res->st_device_shared_addr_map[type].va);
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    if(NULL != pst_pci_dev)
    {
        oal_pci_cache_flush(pst_pci_dev, (oal_void*)pst_pci_res->st_device_shared_addr_map[type].pa, sizeof(val));
    }
#endif

    return OAL_SUCC;
}

/*device shared mem read*/
oal_int32 oal_pcie_read_dsm32(oal_pcie_res* pst_pci_res, PCIE_SHARED_DEVICE_ADDR_TYPE type, oal_uint32 *val)
{
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev;
    pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
#endif

    if(OAL_WARN_ON((oal_uint32)type >= (oal_uint32)PCIE_SHARED_ADDR_BUTT))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "invalid device addr type:%d", type);
        return -OAL_EINVAL;
    }

    if(OAL_WARN_ON(NULL == pst_pci_res))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pci res is null");
        return -OAL_ENODEV;
    }

    if(OAL_WARN_ON(0 == pst_pci_res->st_device_shared_addr_map[type].va))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "dsm type:%d va is null", type);
        return -OAL_ENODEV;
    }

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    if(NULL != pst_pci_dev)
    {
        /*cache 无效化*/
        oal_pci_cache_inv(pst_pci_dev, (oal_void*)pst_pci_res->st_device_shared_addr_map[type].pa, sizeof(*val));
    }
#endif

    *val = oal_readl((void*)pst_pci_res->st_device_shared_addr_map[type].va);

    return OAL_SUCC;
}

oal_int32 oal_pcie_set_device_soft_fifo_enable(oal_pcie_res* pst_pci_res)
{
    if(pst_pci_res->revision >= PCIE_REVISION_5_00A)
    {
        return oal_pcie_write_dsm32(pst_pci_res, PCIE_SHARED_SOFT_FIFO_ENABLE, !!pcie_soft_fifo_enable);
    }
    else
    {
        return OAL_SUCC;
    }
}

oal_int32 oal_pcie_set_device_ringbuf_bugfix_enable(oal_pcie_res* pst_pci_res)
{
    return oal_pcie_write_dsm32(pst_pci_res, PCIE_SHARED_RINGBUF_BUGFIX, !!pcie_ringbuf_bugfix_enable);
}

oal_int32 oal_pcie_set_device_dma_check_enable(oal_pcie_res* pst_pci_res)
{
    return oal_pcie_write_dsm32(pst_pci_res, PCIE_SHARED_SOFT_DMA_CHECK, !!pcie_dma_data_check_enable);
}

oal_int32 oal_pcie_device_aspm_init(oal_pcie_res* pst_pci_res)
{
    oal_uint32 value;

    value = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_CFG_OFF);
    value |= ( 1 << 1);
    value |= ( 1 << 3);
    oal_writel(value, pst_pci_res->pst_pci_ctrl_base + PCIE_CFG_OFF);

    value = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_LOW_POWER_CFG_OFF);
    value &= ~( 1 << 7);
    oal_writel(value, pst_pci_res->pst_pci_ctrl_base + PCIE_LOW_POWER_CFG_OFF);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "cfg reg:0x%x, low_power_cfg reg:0x%x"
                                            ,oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_CFG_OFF)
                                            ,oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_LOW_POWER_CFG_OFF));

    return OAL_SUCC;
}

oal_int32 oal_pcie_device_read32_timeout(oal_void* pst_va, oal_uint32 target, oal_uint32 mask, oal_ulong timeout)
{
    oal_uint32 value = 0xdeadbeaf;
    oal_ulong timeout_t;
    timeout_t = jiffies + OAL_MSECS_TO_JIFFIES(timeout);
    for(;;)
    {

        value = oal_readl(pst_va);
        if((value & mask) == target)
        {
            return OAL_SUCC;
        }

        if( 0 == timeout )
        {
            return -OAL_EFAIL;
        }

        if(oal_time_after(jiffies, timeout_t))
        {
            break;
        }

        cpu_relax();
    }

    oal_print_hi11xx_log(HI11XX_LOG_WARN, "%p expect 0x%x , read 0x%x, mask 0x%x, timeout=%lu ms", pst_va, target, value, mask, timeout);
    return -OAL_ETIMEDOUT;
}

oal_int32 oal_pcie_get_vol_reg_bits_value(oal_uint32 target_value, oal_reg_bits_stru* pst_reg_bits,
                                                oal_uint32 nums, oal_uint32* pst_value)
{
    oal_int32 i;
    oal_reg_bits_stru* pst_tmp;
    for(i = 0; i < nums; i++)
    {
        pst_tmp = pst_reg_bits + i;
        if(target_value == pst_tmp->flag)
        {
            *pst_value = pst_tmp->value;
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "flag:%u matched value:0x%x , [%s]", target_value, *pst_value, pst_tmp->name);
            return OAL_SUCC;
        }
    }

    return -OAL_ENODEV;
}

oal_void oal_pcie_set_voltage_bias_param(oal_uint32 phy_0v9_bias, oal_uint32 phy_1v8_bias)
{
    pcie_ldo_phy_0v9_param = phy_0v9_bias;
    pcie_ldo_phy_1v8_param = phy_1v8_bias;
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "param 0v9=%u 1v8=%u", phy_0v9_bias, phy_1v8_bias);
}

/*电压拉偏初始化*/
oal_int32 oal_pcie_voltage_bias_init(oal_pcie_res* pst_pci_res)
{
    /*vp,vptx,vph 降压 5%*/
    oal_int32 ret;
    oal_uint32 value, phy_0v9_bias, phy_1v8_bias;
    pci_addr_map addr_map;
    oal_void* pst_pmu_cmu_ctrl;/*0x50002000*/
    oal_void* pst_pmu2_cmu_ir_ctrl;/*0x50003000*/

    if(OAL_WARN_ON(NULL == pst_pci_res))
    {
        return -OAL_ENODEV;
    }

    if(0 == pcie_ldo_phy_0v9_param ||
       0 == pcie_ldo_phy_1v8_param)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "phy 0v9=%u mV, 1v8=%u mV invaild, pcie vol bias bypass", pcie_ldo_phy_0v9_param, pcie_ldo_phy_1v8_param);
        return OAL_SUCC;
    }

    ret = oal_pcie_get_vol_reg_bits_value(pcie_ldo_phy_0v9_param, g_pcie_phy_0v9_bits,
                                          OAL_ARRAY_SIZE(g_pcie_phy_0v9_bits), &phy_0v9_bias);
    if(OAL_SUCC != ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "invalid pcie ldo bias phy 0v9 param =%d mV", pcie_ldo_phy_0v9_param);
        return -OAL_EINVAL;
    }

    ret = oal_pcie_get_vol_reg_bits_value(pcie_ldo_phy_1v8_param, g_pcie_phy_1v8_bits,
                                          OAL_ARRAY_SIZE(g_pcie_phy_1v8_bits), &phy_1v8_bias);
    if(OAL_SUCC != ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "invalid pcie ldo bias phy 1v8 param =%d mV", pcie_ldo_phy_1v8_param);
        return -OAL_EINVAL;
    }


    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, HI1103_PA_PMU_CMU_CTL_BASE, &addr_map);
    if(OAL_UNLIKELY(OAL_SUCC != ret))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "can not found mem map for dev cpu address 0x%x\n", HI1103_PA_PMU_CMU_CTL_BASE);
        return -OAL_EFAIL;
    }

    pst_pmu_cmu_ctrl = (oal_void*)addr_map.va;

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, HI1103_PA_PMU2_CMU_IR_BASE, &addr_map);
    if(OAL_UNLIKELY(OAL_SUCC != ret))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "can not found mem map for dev cpu address 0x%x\n", HI1103_PA_PMU2_CMU_IR_BASE);
        return -OAL_EFAIL;
    }

    pst_pmu2_cmu_ir_ctrl = (oal_void*)addr_map.va;

    /*PCIELDO0V9_CFG0*/
    value = oal_readl(pst_pmu_cmu_ctrl + 0x0128);
    value &= ~(0xf << 8);
    value &= ~(0xf << 12);
    value |= (phy_0v9_bias << 8);
    value |= (phy_0v9_bias << 12);/**/
    oal_writel(value, (pst_pmu_cmu_ctrl + 0x0128));

    /*PCIELDO1V8_CFG0*/
    value = oal_readl(pst_pmu2_cmu_ir_ctrl + 0x0268);
    value &= ~(0xf << 8);
    value |= (phy_1v8_bias << 8);
    oal_writel(value , (pst_pmu2_cmu_ir_ctrl + 0x0268));

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcieldo0v9 reg=0x%x, pcieldo1v8 reg=0x%x", oal_readl(pst_pmu_cmu_ctrl + 0x0128), oal_readl(pst_pmu2_cmu_ir_ctrl + 0x0268));

    return OAL_SUCC;
}

oal_int32 oal_pcie_device_changeto_high_cpufreq(oal_pcie_res* pst_pci_res)
{
    oal_int32 ret;
    pci_addr_map addr_map;
    oal_void* pst_glb_ctrl;/*0x50000000*/
    oal_void* pst_pmu_cmu_ctrl;/*0x50002000*/
    oal_void* pst_pmu2_cmu_ir_ctrl;/*0x50003000*/
    oal_void* pst_wctrl;

    if(ft_pcie_wcpu_max_freq_bypass)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie_wcpu_max_freq_bypass");
        return OAL_SUCC;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, HI1103_PA_GLB_CTL_BASE_ADDR, &addr_map);
    if(OAL_UNLIKELY(OAL_SUCC != ret))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "can not found mem map for dev cpu address 0x%x\n", HI1103_PA_GLB_CTL_BASE_ADDR);
        return -OAL_EFAIL;
    }

    pst_glb_ctrl = (oal_void*)addr_map.va;

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, HI1103_PA_PMU_CMU_CTL_BASE, &addr_map);
    if(OAL_UNLIKELY(OAL_SUCC != ret))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "can not found mem map for dev cpu address 0x%x\n", HI1103_PA_PMU_CMU_CTL_BASE);
        return -OAL_EFAIL;
    }

    pst_pmu_cmu_ctrl = (oal_void*)addr_map.va;

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, HI1103_PA_PMU2_CMU_IR_BASE, &addr_map);
    if(OAL_UNLIKELY(OAL_SUCC != ret))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "can not found mem map for dev cpu address 0x%x\n", HI1103_PA_PMU2_CMU_IR_BASE);
        return -OAL_EFAIL;
    }

    pst_pmu2_cmu_ir_ctrl = (oal_void*)addr_map.va;

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, HI1103_PA_W_CTL_BASE, &addr_map);
    if(OAL_UNLIKELY(OAL_SUCC != ret))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "can not found mem map for dev cpu address 0x%x\n", HI1103_PA_W_CTL_BASE);
        return -OAL_EFAIL;
    }

    pst_wctrl = (oal_void*)addr_map.va;

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "pst_pci_res->revision %u", pst_pci_res->revision);

    if(PCIE_REVISION_4_70A == pst_pci_res->revision)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie 4.70a mpw2 freq");
        oal_setl_bit(HI1103_PMU2_CMU_IR_PMU2_CMU_ABB_MAN_SEL_W_6_REG, HI1103_PMU2_CMU_IR_PMU_XLDO_EN_MAN_W_SEL_OFFSET);
        /*open xldo*/
        oal_pcie_device_read32_timeout(HI1103_PMU2_CMU_IR_PMU2_CMU_ABB_STS_6_REG, 0x3, 0x3, 100);
#if 0
        /*open rfldo3*/
        oal_setl_bit(HI1103_PMU2_CMU_IR_PMU2_CMU_ABB_MAN_SEL_W_2_REG, HI1103_PMU2_CMU_IR_PMU_RFLDO3_EN_MAN_W_SEL_OFFSET);
        oal_pcie_device_read32_timeout(HI1103_PMU2_CMU_IR_PMU2_CMU_ABB_STS_2_REG, 0xc0, 0xc0, 100);
        /*open rfldo6*/
        oal_setl_bit(HI1103_PMU2_CMU_IR_PMU2_CMU_ABB_MAN_SEL_W_3_REG, HI1103_PMU2_CMU_IR_PMU_RFLDO6_EN_MAN_W_SEL_OFFSET);
        oal_pcie_device_read32_timeout(HI1103_PMU2_CMU_IR_PMU2_CMU_ABB_STS_3_REG, 0x30, 0x30, 100);
#endif
        /*frac_slp_sts*/
        oal_setl_bit(HI1103_PMU2_CMU_IR_FBDIV_FRAC_SLP_REG, HI1103_PMU2_CMU_IR_FBDIV_FRAC_WKUP_OFFSET);
        oal_pcie_device_read32_timeout(HI1103_PMU2_CMU_IR_FBDIV_FRAC_SLP_REG, 0x0, 0x3, 100);

        /*default is 1*/
        oal_setl_bits(HI1103_PMU2_CMU_IR_REFDIV_REG, 0, 6, 0x1);

        oal_writel(0x32, HI1103_PMU2_CMU_IR_FBDIV_REG);
        oal_writel(0x0,  HI1103_PMU2_CMU_IR_FRAC_H_REG);
        oal_writel(0x0,  HI1103_PMU2_CMU_IR_FRAC_L_REG);
        oal_writel(0x11, HI1103_PMU2_CMU_IR_POSTDIV_REG);

        /*pull down FOUTVCOPD*/
        oal_setl_bit(HI1103_PMU2_CMU_IR_PMU2_CMU_ABB_MAN_SEL_W_8_REG, 4);
        oal_clearl_bit(HI1103_PMU2_CMU_IR_PMU2_CMU_ABB_DBG_SEL_8_REG, 4);
        oal_msleep(1);

        oal_clearl_mask(HI1103_PMU2_CMU_IR_CMU_PD_REG, (BIT4 | BIT3 | BIT1));

        oal_msleep(1);

        /*wait lock*/
        oal_pcie_device_read32_timeout(HI1103_PMU2_CMU_IR_CMU_STATUS_GRM_REG, 0x0, 0x1, 100);

        /**/
        oal_clearl_mask(HI1103_PMU2_CMU_IR_CMU_GT_W_REG, (BIT1 | BIT2 | BIT4 | BIT5));
        oal_clearl_mask(HI1103_PMU2_CMU_IR_CMU_GT_W_REG, (BIT0 | BIT3));

        oal_clearl_bit(HI1103_PMU2_CMU_IR_TCXO_GT_W_REG, 4);

        /*switch to 640M*/
        oal_setl_mask(HI1103_W_CTL_WTOPCRG_SOFT_CLKEN_REG, (BIT9 | BIT8));

        /*select 640M*/
        oal_setl_bit(HI1103_W_CTL_W_TCXO_SEL_REG, 0);

#if 0
        oal_clearl_mask(HI1103_PMU2_CMU_IR_AON_CRG_CKEN_REG, (BIT1 | BIT0));
        oal_setl_bit(HI1103_PMU2_CMU_IR_AON_DIV_1_REG, 4);
        oal_setl_bit(HI1103_PMU2_CMU_IR_CLK_SEL_REG, 1);
        oal_setl_mask(HI1103_PMU2_CMU_IR_AON_CRG_CKEN_REG,  (BIT1 | BIT0));
#endif

    }
    else if(PCIE_REVISION_5_00A == pst_pci_res->revision)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie 5.00a pilot freq from 76.8M");
        /*changet to highfreq from 76.8M oal_writel(0x100, pst_pmu2_cmu_ir_ctrl + 0x338);*//*tcxo to 38.4M*/
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "tcxo div=0x%x", oal_readl(pst_pmu2_cmu_ir_ctrl + 0x338));

        /*sysldo power chagne from vddio to buck*/
        /*pmu_cmu_ctl_pmu1_man_frc_w_2_set(PMU_CMU_CTL_PMU_REF1_IBG_EN_MAN_W_FRC)*/
        oal_setl_bit(pst_pmu_cmu_ctrl + 0xc8 , 8);

        /*pmu_cmu_ctl_pmu1_man_frc_w_0_set(PMU_CMU_CTL_BUCK_EN_MAN_W_FRC)*/
        oal_setl_bit(pst_pmu_cmu_ctrl + 0x88 , 6);

        /*pmu_cmu_ctl_pmu1_man_frc_w_0_set(PMU_CMU_CTL_SYSLDO_ECO_REF_SEL_MAN_W_FRC*/
        oal_setl_bit(pst_pmu_cmu_ctrl + 0x88 , 12);
        oal_udelay(30);

        /*pmu_cmu_ctl_pmu1_man_frc_w_0_set(PMU_CMU_CTL_SYSLDO_ECO_EN_MAN_W_FRC*/
        oal_setl_bit(pst_pmu_cmu_ctrl + 0x88 , 2);
        oal_udelay(60);

        /*pmu_cmu_ctl_pmu1_man_frc_w_0_set(PMU_CMU_CTL_SYSLDO_EN_MAN_W_FRC_OFF*/
        oal_setl_bit(pst_pmu_cmu_ctrl + 0x88 , 1);
        oal_udelay(30);

        /*pmu_cmu_ctl_pmu1_man_frc_w_0_clear(PMU_CMU_CTL_SYSLDO_ECO_REF_SEL_MAN_W_FRC)*/
        oal_clearl_bit(pst_pmu_cmu_ctrl + 0x88, 12);

        /*pmu_cmu_ctl_pmu1_man_frc_w_0_set(PMU_CMU_CTL_SYSLDO_ECO_REF_SEL_MAN_W_FRC_OFF)*/
        oal_setl_bit(pst_pmu_cmu_ctrl + 0x88 , 13);

        //clk_960m_sel :2->3, frq=640M
        /*pmu2_cmu_ir_ts_ef_sel(PMU2_CMU_IR_TS_EF_CLK_960M_SEL_3, PMU2_CMU_IR_TS_EF_CLK_960M_SEL_MASK)*/
        oal_writel(0x31b, pst_pmu2_cmu_ir_ctrl + 0x328);

        //buck_clamp_sel: 1>3>2
        /*pmu_cmu_ctl_buck_clamp_sel_w(PMU_CMU_CTL_BUCK_FAST_MODE_W_1);*/
        oal_setl_bit(pst_pmu_cmu_ctrl + 0x170 , 0);

        /*open intldo1*/
        /*pmu2_cmu_ir_ts_ef_pmu2_cmu_abb_man_frc_w_0_set(PMU_CMU_IR_TS_EF_PMU_INTLDO1_EN_MAN_W_FRC)*/
        oal_setl_bit(pst_pmu2_cmu_ir_ctrl + 0x108, 2);

        /*poll_on_set(PMU2_CMU_IR_TS_EF_RB_PMU2_CMU_ABB_STS_0_ADDR,2) 0x11c*/
        /*poll_on_set(PMU2_CMU_IR_TS_EF_RB_PMU2_CMU_ABB_STS_0_ADDR,3)*/
        oal_pcie_device_read32_timeout((pst_pmu2_cmu_ir_ctrl + 0x11c), 0xc, 0xc, 100);

        /*open xldo*/
        /*pmu2_cmu_ir_ts_ef_pmu2_cmu_abb_man_frc_w_2_set(PMU2_CMU_IR_TS_EF_PMU_XLDO_EN_MAN_W_FRC) 0x148*/
        /*read_32(PMU2_CMU_IR_TS_EF_RB_PMU2_CMU_ABB_STS_2_ADDR)   0xC000 == 0xc000*/
        oal_setl_bit(pst_pmu2_cmu_ir_ctrl + 0x148, 14);

        oal_pcie_device_read32_timeout((pst_pmu2_cmu_ir_ctrl + 0x15c), 0xc000, 0xc000, 100);

        /*pmu2_cmu_ir_ts_ef_refdiv(PMU2_CMU_IR_TS_EF_REFDIV_1)*/
        /*pmu2_cmu_ir_ts_ef_fbdiv(0x32)  for 38.4MHZ tcxo*/
        /*pmu2_cmu_ir_ts_ef_frac_h(0x0)*/
        /*pmu2_cmu_ir_ts_ef_frac_l(0x0)*/
        /*pmu2_cmu_ir_ts_ef_postdiv(PMU2_CMU_IR_TS_EF_POSTDIV2_1|PMU2_CMU_IR_TS_EF_POSTDIV1_1, PMU2_CMU_IR_TS_EF_POSTDIV2_MASK|PMU2_CMU_IR_TS_EF_POSTDIV1_MASK)*/
        oal_setl_bits(HI1103_PMU2_CMU_IR_REFDIV_REG, 0, 6, 0x1);
        oal_writel(0x19, HI1103_PMU2_CMU_IR_FBDIV_REG);
        oal_writel(0x0,  HI1103_PMU2_CMU_IR_FRAC_H_REG);
        oal_writel(0x0,  HI1103_PMU2_CMU_IR_FRAC_L_REG);
        oal_writel(0x11, HI1103_PMU2_CMU_IR_POSTDIV_REG);

        /*pmu2_cmu_ir_ts_ef_pmu2_cmu_abb_man_frc_w_3(PMU2_CMU_IR_TS_EF_PD_MAN_W_FRC_OFF_0|PMU2_CMU_IR_TS_EF_PD_MAN_W_FRC_1, PMU2_CMU_IR_TS_EF_PD_MAN_W_FRC_OFF_MASK|PMU2_CMU_IR_TS_EF_PD_MAN_W_FRC_MASK)*/
        oal_value_mask(pst_pmu2_cmu_ir_ctrl + 0x168, BIT8, BIT9 | BIT8);
        /*PMU2_CMU_IR_TS_EF_RB_PMU2_CMU_ABB_STS_3_ADDR, value&0x200 != 0x0*/
        oal_pcie_device_read32_timeout((pst_pmu2_cmu_ir_ctrl + 0x17c), 0x200, 0x200, 100);

        /*pmu2_cmu_ir_ts_ef_cmu_pd_w_clear(PMU2_CMU_IR_TS_EF_FOUTVCOPD_W | PMU2_CMU_IR_TS_EF_FOUTPOSTDIVPD_W|PMU2_CMU_IR_TS_EF_DSMPD_W)*/
        oal_clearl_mask(HI1103_PMU2_CMU_IR_CMU_PD_REG, (BIT4 | BIT3 | BIT1));
        /*PMU2_CMU_IR_TS_EF_RB_CMU_PD_CMU_PD_W_ADDR value&0x1a != 0x0*/
        oal_msleep(1);

        /*pll_on_reset(PMU2_CMU_IR_TS_EF_RB_CMU_STATUS_RAW_ADDR, 0)*/
        oal_pcie_device_read32_timeout(pst_pmu2_cmu_ir_ctrl + 0x880, 0x0, 0x1, 100);

        /*pmu2_cmu_ir_ts_ef_cmu_gt_w_clear(PMU2_CMU_IR_TS_EF_CLK_640M_WCBB_CPU_GT_W|PMU2_CMU_IR_TS_EF_CLK_960M_WCBB_GT_W)*/
        oal_clearl_mask(HI1103_PMU2_CMU_IR_CMU_GT_W_REG, (BIT2|BIT5));

        /*pmu2_cmu_ir_ts_ef_cmu_gt_w_clear(PMU2_CMU_IR_TS_EF_CLK_640M_GT_W|PMU2_CMU_IR_TS_EF_CLK_960M_GT_W)*/
        oal_clearl_mask(HI1103_PMU2_CMU_IR_CMU_GT_W_REG, (BIT3|BIT0));

        /*pmu2_cmu_ir_ts_ef_tcxo_gt_w_clear(PMU2_CMU_IR_TS_EF_SSCG2DBB_GT_W)*/
        oal_clearl_bit(HI1103_PMU2_CMU_IR_TCXO_GT_W_REG, 4);

        /*w_ctr_wtopcrg_soft_clken(W_CTL_PLL2DBB_640M_CLKEN_1, W_CTL_PLL2DBB_640M_CLKEN_MASK)*/
        oal_setl_mask(HI1103_W_CTL_WTOPCRG_SOFT_CLKEN_REG, (BIT8));
        /*w_ctl_w_tcxo_sel(W_CTL_W_TCXO_SEL_1)*/

        oal_print_hi11xx_log(HI11XX_LOG_INFO, "change 640M set");
        oal_setl_bit(HI1103_W_CTL_W_TCXO_SEL_REG, 0);

        oal_print_hi11xx_log(HI11XX_LOG_INFO, "change 640M wait start");
        oal_msleep(10);/*防止这里高频切出问题，下面只回读一次*/
        {
            oal_uint32 value = oal_readl(HI1103_W_CTL_CLKMUX_STS_REG);
            if((value & 0x2) != 0x2)
            {
                oal_print_hi11xx_log(HI11XX_LOG_ERR, "change 640M wait failed, clkmux=0x%x", value);
                ssi_dump_device_regs(SSI_MODULE_MASK_COMM);
                //return ret;
            }
            else
            {
                oal_print_hi11xx_log(HI11XX_LOG_INFO, "change 640M wait ok, clkmux=0x%x", value);
            }
        }
    }
    else
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR,"unsupport revision: %u", pst_pci_res->revision);
        return -OAL_ENODEV;
    }

    ret = oal_pcie_device_check_alive(pst_pci_res);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "change 640M failed");
        ssi_dump_device_regs(SSI_MODULE_MASK_ARM_REG);
        return ret;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "change 640M done");
    return OAL_SUCC;
}

oal_int32 oal_pcie_device_check_alive(oal_pcie_res* pst_pci_res)
{
    oal_int32 ret;
    oal_uint32 value;
    pci_addr_map addr_map;

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, 0x50000000, &addr_map);
    if(OAL_UNLIKELY(OAL_SUCC != ret))
    {
        /*share mem 地址未映射!*/
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "can not found mem map for dev cpu address 0x%x\n", 0x50000000);
        return -OAL_EFAIL;
    }

    value = oal_readl((oal_void*)addr_map.va);
    if(0x101 == value)
    {
        return OAL_SUCC;
    }
    else
    {
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "pcie maybe linkdown, glbctrl=0x%x", value);
        return -OAL_EFAIL;
    }
}

oal_int32 oal_pcie_device_scan_reg(oal_pcie_res* pst_pci_res)
{
    return OAL_SUCC;
}

oal_int32 oal_pcie_get_gen_mode(oal_pcie_res* pst_pci_res)
{
    oal_int32 value = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_STATUS2_OFF);
    value = ((value >> 9) & 0x1);
    if(0 == value)
    {
        return PCIE_GEN1;
    }
    else
    {
        return PCIE_GEN2;
    }
}

oal_int32 oal_pcie_print_device_aer_cap_reg(oal_pcie_res* pst_pci_res)
{
    oal_int32 pos_cap_aer;
    oal_uint32 uncor = 0, cor = 0;
    oal_pci_dev_stru *pst_pci_dev;
    pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);

    pos_cap_aer = oal_pci_find_ext_capability(pst_pci_dev, PCI_EXT_CAP_ID_ERR);
    if( 0 == pos_cap_aer)
    {
        return -OAL_EFAIL;
    }

    /*状态寄存器读清*/
    if (oal_pci_read_config_dword(pst_pci_dev, pos_cap_aer + PCI_ERR_UNCOR_STATUS, &uncor))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "PCI_ERR_UNCOR_STATUS: read fail");
        return -OAL_EFAIL;
    }
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "PCI_ERR_UNCOR_STATUS: 0x%x", uncor);

    if (oal_pci_read_config_dword(pst_pci_dev, pos_cap_aer + PCI_ERR_COR_STATUS, &cor))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "PCI_ERR_COR_STATUS: read fail");
        return -OAL_EFAIL;
    }
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "PCI_ERR_COR_STATUS: 0x%x", cor);

    //oal_pci_write_config_dword(pst_pci_dev, pos_cap_aer + PCI_ERR_UNCOR_STATUS, uncor);
    //oal_pci_write_config_dword(pst_pci_dev, pos_cap_aer + PCI_ERR_COR_STATUS, cor);

    return OAL_SUCC;
}

oal_int32 oal_pcie_unmask_device_link_erros(oal_pcie_res* pst_pci_res)
{
    oal_uint32 ucor_mask = 0;
    oal_int32 pos_cap_aer;
    oal_pci_dev_stru *pst_pci_dev;
    pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);

    pos_cap_aer = oal_pci_find_ext_capability(pst_pci_dev, PCI_EXT_CAP_ID_ERR);
    if(0 == pos_cap_aer)
    {
        return -OAL_EFAIL;
    }

    if (oal_pci_read_config_dword(pst_pci_dev, pos_cap_aer + PCI_ERR_UNCOR_MASK, &ucor_mask))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "PCI_ERR_UNCOR_MASK: read fail");
        return -OAL_EFAIL;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "PCI_ERR_UNCOR_MASK: 0x%x", ucor_mask);

    /*bit 22, 26 unmask , vendor suggest*/
    ucor_mask = 0x0;

    oal_pci_write_config_dword(pst_pci_dev, pos_cap_aer + PCI_ERR_UNCOR_MASK, ucor_mask);

    return OAL_SUCC;
}

oal_int32 oal_pcie_check_device_link_errors(oal_pcie_res* pst_pci_res)
{
    PCIE_MSG_INTR_STATUS msg_intr_status, msg_mask;

    msg_mask.AsDword = 0x0;
    msg_intr_status.bits.soc_pcie_send_nf_err_status  = 1;
    msg_intr_status.bits.soc_pcie_send_f_err_status   = 1;
    msg_intr_status.bits.soc_pcie_send_cor_err_status = 1;

    msg_intr_status.AsDword = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_MSG_INTR_STATUS_OFF);
    msg_intr_status.AsDword &= msg_intr_status.AsDword;

    if(msg_intr_status.bits.soc_pcie_send_f_err_status)
    {
        /*链路信号极差*/
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "f_err found, intr_status=0x%8x", msg_intr_status.AsDword);
        oal_pcie_print_device_aer_cap_reg(pst_pci_res);
        /*Clear the int*/
        oal_writel(msg_intr_status.AsDword, (pst_pci_res->pst_pci_ctrl_base + PCIE_MSG_INTR_CLR_OFF));
        DECLARE_DFT_TRACE_KEY_INFO("soc_pcie_send_f_err",OAL_DFT_TRACE_SUCC);
        if(ft_pcie_link_err_bypass)
        {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie_link_err_bypass");
            return OAL_SUCC;
        }
        else
        {
            return -OAL_EFAIL;
        }
    }

    if(msg_intr_status.bits.soc_pcie_send_nf_err_status)
    {
        /*链路信号差*/
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "nf_err found, intr_status=0x%8x", msg_intr_status.AsDword);
        oal_pcie_print_device_aer_cap_reg(pst_pci_res);
        /*Clear the int*/
        oal_writel(msg_intr_status.AsDword, (pst_pci_res->pst_pci_ctrl_base + PCIE_MSG_INTR_CLR_OFF));
        DECLARE_DFT_TRACE_KEY_INFO("soc_pcie_send_nf_err",OAL_DFT_TRACE_SUCC);
        if(ft_pcie_link_err_bypass)
        {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie_link_err_bypass");
            return OAL_SUCC;
        }
        else
        {
            return -OAL_EFAIL;
        }
    }

    if(msg_intr_status.bits.soc_pcie_send_cor_err_status)
    {
        /*可忽略的错误*/
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "cor_err found, intr_status=0x%8x", msg_intr_status.AsDword);
        oal_pcie_print_device_aer_cap_reg(pst_pci_res);
        DECLARE_DFT_TRACE_KEY_INFO("soc_pcie_send_cor_err",OAL_DFT_TRACE_SUCC);
    }

    /*Clear the int*/
    oal_writel(msg_intr_status.AsDword, (pst_pci_res->pst_pci_ctrl_base + PCIE_MSG_INTR_CLR_OFF));

    /*pass*/
    return OAL_SUCC;
}

oal_int32 oal_pcie_device_mem_check_burst(oal_pcie_res* pst_pci_res, oal_uint32 burst_size,
                                                    oal_ulong start, oal_ulong length, oal_uint8 test_value)
{
    oal_uint32 i;
    oal_int32 ret;
    oal_void*  pst_ddr_buf = NULL;
    oal_uint32 verify, value;
    oal_ulong remain_size, offset, copy_size, total_size;

    pci_addr_map addr_map;

    if((length & 0x7) || (start & 0x7))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "length %lu, cpu address 0x%lx is invalid", length, start);
        return -OAL_EFAUL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, start + length - 1, &addr_map);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx + length:0x%lx -1 invalid", start, length);
        return ret;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, start, &addr_map);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx", start);
        return ret;
    }

    pst_ddr_buf = oal_memalloc(burst_size);
    if(NULL == pst_ddr_buf)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc mem %u bytes failed",  burst_size);
        return -OAL_ENOMEM;
    }

    oal_memset(pst_ddr_buf, test_value, burst_size);

    /*先连续写再连续读，连续写性能最优*/
    remain_size = length;
    offset = 0;
    total_size = 0;
    verify = (test_value | test_value << 8 | test_value << 16 | test_value << 24);

    for(;;)
    {
        if( 0 == remain_size)
        {
            break;
        }

        copy_size = OAL_MIN(burst_size, remain_size);

        /*h2d write*/
        oal_pcie_memcopy(addr_map.va + offset, (oal_ulong)pst_ddr_buf, copy_size);

        /*d2h read*/
        oal_pcie_memcopy((oal_ulong)pst_ddr_buf, addr_map.va + offset, copy_size);

        /*memcheck*/
        for(i = 0; i < copy_size; i += sizeof(oal_uint32))
        {
            value = *(oal_uint32*)(pst_ddr_buf + i);
            if( value != verify )
            {
                oal_print_hi11xx_log(HI11XX_LOG_ERR,"mem check failed, cpu address : 0x%lx , write 0x%x , read 0x%x, offset =%lu, i = %u",
                                                       start + offset +i, verify, value, offset, i);
                oal_print_hex_dump((oal_uint8 *)pst_ddr_buf, copy_size, 64, "burst check ");
                oal_print_hex_dump((oal_uint8 *)(addr_map.va + offset), copy_size, 64, "bcpu check ");
                oal_free(pst_ddr_buf);
                return -OAL_EFAIL;
            }
        }

        offset += copy_size;
        total_size += copy_size;
        remain_size -= copy_size;
    }

    if(total_size != length)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wrong total_size=%lu ,length=%lu", total_size, length);
        oal_free(pst_ddr_buf);
        return -OAL_EFAIL;
    }

    oal_free(pst_ddr_buf);
    return OAL_SUCC;
}

oal_int32 oal_pcie_device_mem_check_word(oal_pcie_res* pst_pci_res,
                                                    oal_ulong start, oal_ulong length, oal_uint8 test_value)
{
    oal_uint32 i;
    oal_int32 ret;
    oal_uint32 verify, value;
    declare_time_cost_stru(cost);

    pci_addr_map addr_map;

    oal_get_time_cost_start(cost);

    if((length & 0x7) || (start & 0x7))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "length %lu, cpu address 0x%lx is invalid", length, start);
        return -OAL_EFAUL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, start + length - 1, &addr_map);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx + length:0x%lx -1 invalid", start, length);
        return ret;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, start, &addr_map);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx", start);
        return ret;
    }

    verify = (test_value | test_value << 8 | test_value << 16 | test_value << 24);

    for(i = 0; i < (oal_int32)(oal_uint32)length; i += 4)
    {
        oal_writel(verify, (oal_void*)(addr_map.va + i));
        value = oal_readl((oal_void*)(addr_map.va + i));
        if(OAL_UNLIKELY(verify != value))
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu addr:0x%lx check failed, read=0x%x write=0x%x", start + i, value, verify);
            return -OAL_EFAIL;
        }
    }

    oal_get_time_cost_end(cost);
    oal_calc_time_cost_sub(cost);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "memcheck_byword 0x%lx length %lu, cost %llu us", start, length, time_cost_var_sub(cost));

    return OAL_SUCC;
}

oal_int32 oal_pcie_device_mem_check(oal_pcie_res* pst_pci_res, oal_ulong start, oal_ulong length)
{
    oal_int32 ret;
    oal_int32 i;

    if(ft_pcie_wcpu_mem_check_burst_check)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie_wcpu_mem_check_burst_check_bypass");
    }
    else
    {
        for(i = 0; i < ft_pcie_wcpu_mem_check_times; i++)
        {
            ret = oal_pcie_device_mem_check_burst(pst_pci_res, 4096, start, length, PCIE_MEM_SCAN_VERFIY_VAL_1);
            if(ret)
                return ret;

            ret = oal_pcie_device_mem_check_burst(pst_pci_res, 4096, start, length, PCIE_MEM_SCAN_VERFIY_VAL_2);
            if(ret)
                return ret;

            oal_schedule();
        }
    }

    if(ft_pcie_wcpu_mem_check_byword_bypass)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie_wcpu_mem_check_byword_bypass");
    }
    else
    {
        for(i = 0; i < ft_pcie_wcpu_mem_check_times; i++)
        {
            ret = oal_pcie_device_mem_check_word(pst_pci_res, start, length, PCIE_MEM_SCAN_VERFIY_VAL_1);
            if(ret)
                return ret;

            ret = oal_pcie_device_mem_check_word(pst_pci_res, start, length, PCIE_MEM_SCAN_VERFIY_VAL_2);
            if(ret)
                return ret;

            oal_schedule();
        }
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "mem check address: 0x%lx , length: 0x%lx  pass!", start, length);

    return OAL_SUCC;
}

/*一次性写入全部的地址值*/
oal_int32 oal_pcie_device_mem_write_address_onetime(oal_pcie_res* pst_pci_res, oal_ulong start, oal_ulong length)
{
    oal_int32 ret;
    oal_int32 i;
    oal_ulong remain_size, offset, copy_size;
    oal_void* pst_ddr_buf;

    pci_addr_map addr_map;

    if((length & 0x7) || (start & 0x7))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "length %lu, cpu address 0x%lx is invalid", length, start);
        return -OAL_EFAUL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, start + length - 1, &addr_map);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx + length:0x%lx -1 invalid", start, length);
        return ret;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, start, &addr_map);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx", start);
        return ret;
    }

    pst_ddr_buf = oal_memalloc(PAGE_SIZE);
    if(NULL == pst_ddr_buf)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc mem %lu failed", PAGE_SIZE);
        return -OAL_ENOMEM;
    }

    remain_size = length;
    offset = 0;

    /*4字节写*/
    for(;;)
    {
        if(0 == remain_size)
        {
            break;
        }

        copy_size = OAL_MIN(remain_size, PAGE_SIZE);

        for(i = 0; i < copy_size; i += 4)
        {
            *(oal_uint32*)(pst_ddr_buf + i) = start + offset + i;/*CPU地址*/
        }

        oal_pcie_memcopy((oal_ulong)addr_map.va + offset, (oal_ulong)pst_ddr_buf, copy_size);

        offset += copy_size;

        remain_size -= copy_size;
    }

    oal_free(pst_ddr_buf);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "device_mem_write_address_onetime start:0x%lx length:%lu", start, length);

    oal_schedule();

    return OAL_SUCC;
}

/*一次性读出全部的地址值并且校验*/
oal_int32 oal_pcie_device_mem_read_address_onetime(oal_pcie_res* pst_pci_res, oal_ulong start, oal_ulong length)
{
    oal_int32 ret;
    oal_int32 i;
    oal_ulong remain_size, offset, copy_size;
    oal_void* pst_ddr_buf;

    pci_addr_map addr_map;

    if((length & 0x7) || (start & 0x7))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "length %lu, cpu address 0x%lx is invalid", length, start);
        return -OAL_EFAUL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, start + length - 1, &addr_map);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx + length:0x%lx -1 invalid", start, length);
        return ret;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, start, &addr_map);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx", start);
        return ret;
    }

    pst_ddr_buf = oal_memalloc(PAGE_SIZE);
    if(NULL == pst_ddr_buf)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc mem %lu failed", PAGE_SIZE);
        return -OAL_ENOMEM;
    }

    remain_size = length;
    offset = 0;

    /*4字节写*/
    for(;;)
    {
        if(0 == remain_size)
        {
            break;
        }

        copy_size = OAL_MIN(remain_size, PAGE_SIZE);

        oal_pcie_memcopy((oal_ulong)pst_ddr_buf, (oal_ulong)addr_map.va + offset, copy_size);

        for(i = 0; i < copy_size; i += 4)
        {
            oal_uint32 value = *((oal_uint32*)(pst_ddr_buf + i));
            oal_uint32 cpu_address = start + offset + i;/*CPU地址*/
            if(OAL_UNLIKELY(value != cpu_address))
            {
                oal_print_hi11xx_log(HI11XX_LOG_ERR, "mem check address verify failed, [0x%lx--0x%lx] at 0x%x, write 0x%x read 0x%x",
                                                start, start + length - 1, cpu_address, cpu_address, value);
                oal_print_hex_dump((oal_uint8 *)pst_ddr_buf, copy_size, 64, "address check ");
                oal_print_hex_dump((oal_uint8 *)(addr_map.va + offset), copy_size, 64, "pcie mem ");
                oal_free(pst_ddr_buf);
                return -OAL_EFAIL;
            }
        }

        offset += copy_size;

        remain_size -= copy_size;
    }

    oal_free(pst_ddr_buf);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "device_mem_read_address_onetime start:0x%lx length:%lu", start, length);

    oal_schedule();

    return OAL_SUCC;
}

oal_int32 oal_pcie_device_mem_performance(oal_pcie_res* pst_pci_res, oal_ulong start, oal_ulong length, oal_ulong runtime)
{
    oal_int32 ret;
    oal_ulong timeout;
    oal_ulong remain_size, offset = 0, copy_size, total_size;
    oal_ulong burst_size = 4096;
    oal_uint64  trans_size, us_to_s;
    declare_time_cost_stru(cost);

    pci_addr_map addr_map;

    oal_void* pst_buf = NULL;

    if((length & 0x7) || (start & 0x7))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "start: %lu , length %lu is invalid", start, length);
        return -OAL_EFAUL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, start + length - 1, &addr_map);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx + length:0x%lx -1 invalid", start, length);
        return ret;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, start, &addr_map);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx", start);
        return ret;
    }

    pst_buf = oal_memalloc(burst_size);
    if(NULL == pst_buf)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc perf mem %lu bytes failed",  burst_size);
        return -OAL_ENOMEM;
    }

    if(ft_pcie_perf_wr_bypass)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie_perf_wr_bypass");
    }
    else
    {

        oal_get_time_cost_start(cost);

        /*写性能, 写可以覆盖读*/
        remain_size = 0;
        total_size = 0;

        timeout = jiffies + OAL_MSECS_TO_JIFFIES(runtime);
        for(;;)
        {
            if(oal_time_after(jiffies, timeout))
            {
                break;
            }

            if(0 == remain_size)
            {
                /*repeat*/
                remain_size = length;
                offset = 0;
            }

            copy_size = OAL_MIN(burst_size, remain_size);

            oal_pcie_memcopy(addr_map.va + offset, (oal_ulong)pst_buf, copy_size);

            offset += copy_size;
            total_size += copy_size;

            remain_size -= copy_size;

            cpu_relax();
        }
        oal_get_time_cost_end(cost);
        oal_calc_time_cost_sub(cost);

        us_to_s = time_cost_var_sub(cost);
        trans_size = ((total_size * 1000u * 1000u) >> 17);
        trans_size = div_u64(trans_size, us_to_s);

        oal_print_hi11xx_log(HI11XX_LOG_INFO, "Write(H2D) Thoughtput: %llu Mbps, trans_time:%llu us, tran_size:%lu, address:0x%lx",
                                                trans_size, us_to_s, total_size, start);
    }

    if(ft_pcie_perf_rd_bypass)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie_perf_wr_bypass");
    }
    else
    {

        oal_get_time_cost_start(cost);

        /*读性能, 写可以覆盖读*/
        remain_size = 0;
        total_size = 0;

        timeout = jiffies + OAL_MSECS_TO_JIFFIES(runtime);
        for(;;)
        {
            if(oal_time_after(jiffies, timeout))
            {
                break;
            }

            if(0 == remain_size)
            {
                /*repeat*/
                remain_size = length;
                offset = 0;
            }

            copy_size = OAL_MIN(burst_size, remain_size);

            oal_pcie_memcopy((oal_ulong)pst_buf, addr_map.va + offset,  copy_size);

            offset += copy_size;
            total_size += copy_size;

            remain_size -= copy_size;

            cpu_relax();
        }
        oal_get_time_cost_end(cost);
        oal_calc_time_cost_sub(cost);

        us_to_s = time_cost_var_sub(cost);
        trans_size = ((total_size * 1000u * 1000u) >> 17);
        trans_size = div_u64(trans_size, us_to_s);

        oal_print_hi11xx_log(HI11XX_LOG_INFO, "Read(D2H) Thoughtput: %llu Mbps, trans_time:%llu us, tran_size:%lu, address:0x%lx",
                                                trans_size, us_to_s, total_size, start);
    }

    ret = oal_pcie_device_check_alive(pst_pci_res);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "performance write failed!");
        oal_free(pst_buf);
        return ret;
    }


    oal_print_hi11xx_log(HI11XX_LOG_INFO, "mem performance done");

    oal_free(pst_buf);

    return OAL_SUCC;
}

oal_int32 oal_pcie_device_scan_wmem(oal_pcie_res* pst_pci_res)
{
    oal_uint32 i;
    oal_int32 ret;
    oal_ulong cpu_start, cpu_end, mem_len;
    oal_uint32 scan_nums;
    oal_ulong (*pst_scan_base)[2];

    declare_time_cost_stru(cost);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "start scan wmem..");

    if(PCIE_REVISION_4_70A == pst_pci_res->revision)
    {
        scan_nums = sizeof(g_mpw2_wmem_scan_array)/(2*sizeof(g_mpw2_wmem_scan_array[0][0]));
        pst_scan_base = g_mpw2_wmem_scan_array;
    }
    else if(PCIE_REVISION_5_00A == pst_pci_res->revision)
    {
        scan_nums = sizeof(g_pilot_wmem_scan_array)/(2*sizeof(g_pilot_wmem_scan_array[0][0]));
        pst_scan_base = g_pilot_wmem_scan_array;
    }
    else
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "unkown pcie ip revision :0x%x", pst_pci_res->revision);
        return -OAL_ENODEV;
    }

    oal_get_time_cost_start(cost);

    for(i = 0; i < scan_nums; i++)
    {
        //oal_print_hi11xx_log(HI11XX_LOG_INFO, "i:%u, %p", i, pst_scan_base + i);
        cpu_start = pst_scan_base[i][0];
        cpu_end   = pst_scan_base[i][1];
        if(cpu_end <= cpu_start)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "invalid mem region, start=0x%lx, end=0x%lx, i:%d", cpu_start, cpu_end, i);
            return -OAL_EFAUL;
        }

        mem_len = cpu_end - cpu_start + 1;

        ret = oal_pcie_device_mem_check(pst_pci_res, cpu_start, mem_len);
        if(ret)
            return ret;
    }

    oal_get_time_cost_end(cost);
    oal_calc_time_cost_sub(cost);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "device wmem done cost %llu us, regions :%u", time_cost_var_sub(cost), scan_nums);

    if(ft_pcie_write_address_bypass)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "wmem write_address_bypass");
    }
    else
    {
        oal_get_time_cost_start(cost);
        /*连续写，连续读，Soc要求写入当前地址的值*/
        for(i = 0; i < scan_nums; i++)
        {
            cpu_start = pst_scan_base[i][0];
            cpu_end   = pst_scan_base[i][1];

            mem_len = cpu_end - cpu_start + 1;

            ret = oal_pcie_device_mem_write_address_onetime(pst_pci_res, cpu_start, mem_len);
            if(ret)
                return ret;
        }

        for(i = 0; i < scan_nums; i++)
        {
            cpu_start = pst_scan_base[i][0];
            cpu_end   = pst_scan_base[i][1];

            mem_len = cpu_end - cpu_start + 1;

            ret = oal_pcie_device_mem_read_address_onetime(pst_pci_res, cpu_start, mem_len);
            if(ret)
                return ret;
        }

        oal_get_time_cost_end(cost);
        oal_calc_time_cost_sub(cost);
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "wmem address check done cost %llu us", time_cost_var_sub(cost));
    }

    if(ft_pcie_perf_wr_bypass)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie_perf_wr_bypass");
    }
    else
    {
        oal_get_time_cost_start(cost);
        for(i = 0; i < scan_nums; i++)
        {
            cpu_start = pst_scan_base[i][0];
            cpu_end   = pst_scan_base[i][1];
            if(cpu_end <= cpu_start)
            {
                oal_print_hi11xx_log(HI11XX_LOG_ERR, "invalid mem region, start=0x%lx, end=0x%lx, i:%d", cpu_start, cpu_end, i);
                return -OAL_EFAUL;
            }

            mem_len = cpu_end - cpu_start + 1;

            ret = oal_pcie_device_mem_performance(pst_pci_res, cpu_start, mem_len, ft_pcie_perf_runtime);
            if(ret)
                return ret;
        }
        oal_get_time_cost_end(cost);
        oal_calc_time_cost_sub(cost);
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "mem performance done cost %llu us", time_cost_var_sub(cost));
    }

    return OAL_SUCC;
}

oal_int32 oal_pcie_device_dereset_bcpu(oal_pcie_res* pst_pci_res)
{
    oal_int32 ret;
    pci_addr_map addr_map;
    oal_void* pst_glb_ctrl;/*0x50000000*/

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, HI1103_PA_GLB_CTL_BASE_ADDR, &addr_map);
    if(OAL_UNLIKELY(OAL_SUCC != ret))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "can not found mem map for dev cpu address 0x%x\n", HI1103_PA_GLB_CTL_BASE_ADDR);
        return -OAL_EFAIL;
    }

    pst_glb_ctrl = (oal_void*)addr_map.va;

    oal_writel(0x1, pst_glb_ctrl + 0x94);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "bcpu dereset, reg=0x%x", oal_readl(pst_glb_ctrl + 0x94));

    /*bcpu mem解复位需要时间, 1103 32K计数, 230us*/
    oal_msleep(1);

    return OAL_SUCC;
}

oal_int32 oal_pcie_device_scan_bmem(oal_pcie_res* pst_pci_res)
{
    oal_uint32 i;
    oal_int32 ret;
    oal_ulong cpu_start, cpu_end, mem_len;
    oal_uint32 scan_nums;
    oal_ulong (*pst_scan_base)[2];

    declare_time_cost_stru(cost);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "start scan bmem..");

    if(PCIE_REVISION_4_70A == pst_pci_res->revision)
    {
        scan_nums = sizeof(g_mpw2_bmem_scan_array)/(2*sizeof(g_mpw2_bmem_scan_array[0][0]));
        pst_scan_base = g_mpw2_bmem_scan_array;
    }
    else if(PCIE_REVISION_5_00A == pst_pci_res->revision)
    {
        scan_nums = sizeof(g_pilot_bmem_scan_array)/(2*sizeof(g_pilot_bmem_scan_array[0][0]));
        pst_scan_base = g_pilot_bmem_scan_array;
    }
    else
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "unkown pcie ip revision :0x%x", pst_pci_res->revision);
        return -OAL_ENODEV;
    }

    oal_get_time_cost_start(cost);

    ret = oal_pcie_device_dereset_bcpu(pst_pci_res);
    if(OAL_SUCC != ret)
    {
        return ret;
    }

    for(i = 0; i < scan_nums; i++)
    {
        //oal_print_hi11xx_log(HI11XX_LOG_INFO, "i:%u, %p", i, pst_scan_base + i);
        cpu_start = pst_scan_base[i][0];
        cpu_end   = pst_scan_base[i][1];
        if(cpu_end <= cpu_start)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "invalid mem region, start=0x%lx, end=0x%lx, i:%d", cpu_start, cpu_end, i);
            return -OAL_EFAUL;
        }

        mem_len = cpu_end - cpu_start + 1;

        ret = oal_pcie_device_mem_check(pst_pci_res, cpu_start, mem_len);
        if(ret)
            return ret;
    }

    if(ft_pcie_write_address_bypass)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "bmem write_address_bypass");
    }
    else
    {
        /*连续写，连续读，Soc要求写入当前地址的值*/
        for(i = 0; i < scan_nums; i++)
        {
            cpu_start = pst_scan_base[i][0];
            cpu_end   = pst_scan_base[i][1];

            mem_len = cpu_end - cpu_start + 1;

            ret = oal_pcie_device_mem_write_address_onetime(pst_pci_res, cpu_start, mem_len);
            if(ret)
                return ret;
        }

        for(i = 0; i < scan_nums; i++)
        {
            cpu_start = pst_scan_base[i][0];
            cpu_end   = pst_scan_base[i][1];

            mem_len = cpu_end - cpu_start + 1;

            ret = oal_pcie_device_mem_read_address_onetime(pst_pci_res, cpu_start, mem_len);
            if(ret)
                return ret;
        }
    }

    oal_get_time_cost_end(cost);
    oal_calc_time_cost_sub(cost);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "device bmem done cost %llu us, regions :%u", time_cost_var_sub(cost), scan_nums);

    return OAL_SUCC;
}

/*scan the mem*/
oal_int32 oal_pcie_device_mem_scanall(oal_pcie_res* pst_pci_res)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    oal_int32 ret;

    ret = oal_pcie_device_scan_reg(pst_pci_res);
    if(ret)
        return ret;

    ret = oal_pcie_device_scan_wmem(pst_pci_res);
    if(ret)
        return ret;

    ret = oal_pcie_device_scan_bmem(pst_pci_res);
    if(ret)
        return ret;

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "device_mem_scanall pass");
    return OAL_SUCC;
#else
    return -OAL_EIO;
#endif
}

oal_int32 oal_pcie_copy_from_device_by_dword(oal_pcie_res* pst_pci_res,
                                                        oal_void* ddr_address,
                                                        oal_ulong start,
                                                        oal_uint32 data_size)
{
    oal_uint32 i;
    oal_int32 ret;
    oal_uint32 value;
    oal_ulong length;
    pci_addr_map addr_map;

    length = (oal_ulong)data_size;

    if(OAL_UNLIKELY(((oal_ulong)ddr_address & 0x3) || (data_size & 0x3)))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "ddr address %lu, length 0x%lx is invalid", (oal_ulong)ddr_address, length);
        return -OAL_EINVAL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, start + length - 1, &addr_map);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx + length:0x%lx -1 invalid", start, length);
        return ret;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, start, &addr_map);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx", start);
        return ret;
    }

    for(i = 0; i < (oal_uint32)length; i += 4)
    {
        value = oal_readl((oal_void*)(addr_map.va + i));
        oal_writel(value, ddr_address + i);
    }

    return (oal_int32)data_size;
}

oal_ulong oal_pcie_get_deivce_dtcm_cpuaddr(oal_pcie_res* pst_pci_res)
{
    if(PCIE_REVISION_4_70A == pst_pci_res->revision)
    {
        return 0x20000000;
    }
    else if(PCIE_REVISION_5_00A == pst_pci_res->revision)
    {
        return 0x20018000;
    }
    else
    {
        return 0xffffffff;
    }
}

oal_int32 oal_pcie_copy_to_device_by_dword(oal_pcie_res* pst_pci_res,
                                                        oal_void* ddr_address,
                                                        oal_ulong start,
                                                        oal_uint32 data_size)
{
    oal_uint32 i;
    oal_int32 ret;
    oal_uint32 value;
    oal_ulong length;
    pci_addr_map addr_map;

    length = (oal_ulong)data_size;

    if(OAL_UNLIKELY(((oal_ulong)ddr_address & 0x3) || (data_size & 0x3)))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "ddr address %lu, length 0x%lx is invalid", (oal_ulong)ddr_address, length);
        return -OAL_EINVAL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, start + length - 1, &addr_map);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx + length:0x%lx  invalid", start, length);
        return ret;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, start, &addr_map);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "cpu address start:0x%lx", start);
        return ret;
    }

    for(i = 0; i < (oal_uint32)length; i += 4)
    {
        value = oal_readl(ddr_address + i);
        oal_writel(value, (oal_void*)(addr_map.va + i));
    }

    return (oal_int32)data_size;
}

/*时钟分频要在低功耗关闭下配置*/
oal_int32 oal_pcie_device_auxclk_init(oal_pcie_res* pst_pci_res)
{
    oal_int32  ret;
    oal_uint32 value;
    pci_addr_map      st_map;

    value = oal_readl(pst_pci_res->pst_pci_dbi_base + PCIE_AUX_CLK_FREQ_OFF);
    /*aux_clk 1M, synophys set*/
    value &= (~((1 << 10) - 1));
    value |= 0x1;
    oal_writel(value , pst_pci_res->pst_pci_dbi_base + PCIE_AUX_CLK_FREQ_OFF);

    /*tcxo 38.4M 39分频 = 0.98M 接近1M*/
    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, (0x50000000 + 0x2c), &st_map);
    if(OAL_SUCC != ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wcpu address  convert failed, ret=%d", ret);
        return ret;
    }

    value = oal_readl((oal_void*)st_map.va);
    value &=  (~(((1 << 6) - 1) << 8));
    value |= (0x27 << 8);
    oal_writel(value , (oal_void*)st_map.va);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "clk_freq reg:0x%x, freq_div reg:0x%x"
                                            ,oal_readl(pst_pci_res->pst_pci_dbi_base + PCIE_AUX_CLK_FREQ_OFF)
                                            ,oal_readl((oal_void*)st_map.va));
    return OAL_SUCC;
}

oal_int32 oal_pcie_device_shared_addr_res_map(oal_pcie_res* pst_pci_res, pcie_share_mem_stru *pst_share_mem)
{
    oal_int32 ret;
    oal_int32 i;
    //oal_pci_dev_stru *pst_pci_dev;
    pci_addr_map      st_map;

    //pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
    //st_device_shared_addr_map

    for(i = 0; i < PCIE_SHARED_ADDR_BUTT; i++)
    {
        if(0 == pst_share_mem->device_addr[i])
        {
            PCI_PRINT_LOG(PCI_LOG_DBG, "type:%d , device addr is zero", i);
            continue;
        }

        ret = oal_pcie_inbound_ca_to_va(pst_pci_res, pst_share_mem->device_addr[i], &st_map);
        if(OAL_SUCC != ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "convert device addr type:%d, addr:0x%x failed, ret=%d", i, pst_share_mem->device_addr[i], ret);
            return -OAL_ENOMEM;
        }

        oal_memcopy(&pst_pci_res->st_device_shared_addr_map[i], &st_map, sizeof(pci_addr_map));

    }

    return OAL_SUCC;
}

oal_int32 oal_pcie_device_shared_addr_res_unmap(oal_pcie_res* pst_pci_res)
{
    OAL_MEMZERO(&pst_pci_res->st_device_shared_addr_map, OAL_SIZEOF(pst_pci_res->st_device_shared_addr_map));
    return OAL_SUCC;
}

oal_int32 oal_pcie_comm_ringbuf_res_unmap(oal_pcie_res* pst_pci_res)
{
    oal_int32 i;
    for(i = 0; i < PCIE_COMM_RINGBUF_BUTT; i++)
    {
        OAL_MEMZERO(&pst_pci_res->st_ringbuf_res.comm_rb_res[i].ctrl_daddr, OAL_SIZEOF(pst_pci_res->st_ringbuf_res.comm_rb_res[i].ctrl_daddr));
        OAL_MEMZERO(&pst_pci_res->st_ringbuf_res.comm_rb_res[i].data_daddr, OAL_SIZEOF(pst_pci_res->st_ringbuf_res.comm_rb_res[i].data_daddr));
    }

    return OAL_SUCC;
}

oal_int32 oal_pcie_comm_ringbuf_res_map(oal_pcie_res* pst_pci_res)
{
    oal_int32 i;
    oal_int32 ret;
    pci_addr_map      st_map;/*DEVICE CPU地址*/

    for(i = 0; i < PCIE_COMM_RINGBUF_BUTT; i++)
    {

        if(0 == pst_pci_res->st_ringbuf.st_ringbuf[i].base_addr)
        {
            /*ringbuf invalid*/
            continue;
        }

        /*get ringbuf base_addr*/
        ret = oal_pcie_inbound_ca_to_va(pst_pci_res, pst_pci_res->st_ringbuf.st_ringbuf[i].base_addr, &st_map);
        if(OAL_SUCC != ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "invalid comm ringbuf base address 0x%llx, rb id:%d map failed, ret=%d\n",
                                        pst_pci_res->st_ringbuf.st_ringbuf[i].base_addr, i, ret);
            return -OAL_ENOMEM;
        }

        PCI_PRINT_LOG(PCI_LOG_DBG, "comm ringbuf %d base address is 0x%llx", i, pst_pci_res->st_ringbuf.st_ringbuf[i].base_addr);

        /*comm ringbuf data 所在DMA地址*/
        oal_memcopy((oal_void*)&pst_pci_res->st_ringbuf_res.comm_rb_res[i].data_daddr,
                        (oal_void*)&st_map, sizeof(st_map));

        /*comm ringbuf ctrl address*/
        pst_pci_res->st_ringbuf_res.comm_rb_res[i].ctrl_daddr.va = pst_pci_res->st_ringbuf_map.va + OAL_OFFSET_OF(pcie_ringbuf_res, st_ringbuf[i]);
        pst_pci_res->st_ringbuf_res.comm_rb_res[i].ctrl_daddr.pa = pst_pci_res->st_ringbuf_map.pa + OAL_OFFSET_OF(pcie_ringbuf_res, st_ringbuf[i]);
    }

    return OAL_SUCC;
}

oal_void oal_pcie_ringbuf_res_unmap(oal_pcie_res* pst_pci_res)
{
    oal_int32 i;
    OAL_MEMZERO(&pst_pci_res->st_ringbuf_map, OAL_SIZEOF(pst_pci_res->st_ringbuf_map));
    OAL_MEMZERO(&pst_pci_res->st_ringbuf, OAL_SIZEOF(pst_pci_res->st_ringbuf));
    OAL_MEMZERO(&pst_pci_res->st_rx_res.ringbuf_data_dma_addr, OAL_SIZEOF(pst_pci_res->st_rx_res.ringbuf_data_dma_addr));
    OAL_MEMZERO(&pst_pci_res->st_rx_res.ringbuf_ctrl_dma_addr, OAL_SIZEOF(pst_pci_res->st_rx_res.ringbuf_ctrl_dma_addr));
    for(i = 0; i < PCIE_H2D_QTYPE_BUTT; i++)
    {
        OAL_MEMZERO(&pst_pci_res->st_tx_res[i].ringbuf_data_dma_addr, OAL_SIZEOF(pst_pci_res->st_tx_res[i].ringbuf_data_dma_addr));
        OAL_MEMZERO(&pst_pci_res->st_tx_res[i].ringbuf_ctrl_dma_addr, OAL_SIZEOF(pst_pci_res->st_tx_res[i].ringbuf_ctrl_dma_addr));
    }

    OAL_MEMZERO(&pst_pci_res->st_message_res.d2h_res.ringbuf_ctrl_dma_addr, OAL_SIZEOF(pst_pci_res->st_message_res.d2h_res.ringbuf_ctrl_dma_addr));
    OAL_MEMZERO(&pst_pci_res->st_message_res.d2h_res.ringbuf_data_dma_addr, OAL_SIZEOF(pst_pci_res->st_message_res.d2h_res.ringbuf_data_dma_addr));

    OAL_MEMZERO(&pst_pci_res->st_message_res.h2d_res.ringbuf_ctrl_dma_addr, OAL_SIZEOF(pst_pci_res->st_message_res.h2d_res.ringbuf_ctrl_dma_addr));
    OAL_MEMZERO(&pst_pci_res->st_message_res.h2d_res.ringbuf_data_dma_addr, OAL_SIZEOF(pst_pci_res->st_message_res.h2d_res.ringbuf_data_dma_addr));

    OAL_MEMZERO(&pst_pci_res->st_device_stat, OAL_SIZEOF(pst_pci_res->st_device_stat));

    oal_pcie_device_shared_addr_res_unmap(pst_pci_res);

    oal_pcie_comm_ringbuf_res_unmap(pst_pci_res);
}

oal_int32 oal_pcie_ringbuf_h2d_refresh(oal_pcie_res* pst_pci_res)
{
    oal_int32 ret;
    oal_int32 i;
    pcie_share_mem_stru st_share_mem;
    pci_addr_map      st_map;/*DEVICE CPU地址*/

    oal_pcie_memcopy((oal_ulong)&st_share_mem,  (oal_ulong)pst_pci_res->dev_share_mem.va, sizeof(pcie_share_mem_stru));

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, st_share_mem.ringbuf_res_paddr, &st_map);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "invalid ringbuf device address 0x%x, map failed\n", st_share_mem.ringbuf_res_paddr);
        oal_print_hex_dump((oal_uint8*)&st_share_mem, OAL_SIZEOF(st_share_mem), 32, "st_share_mem: ");
        return -OAL_ENOMEM;
    }

    /*h->h*/
    oal_memcopy(&pst_pci_res->st_ringbuf_map, &st_map, sizeof(pst_pci_res->st_ringbuf_map));

    /*device的ringbuf管理结构同步到Host*/
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    //oal_pci_cache_inv(pst_pci_dev, (oal_void*)pst_pci_res->st_ringbuf_map.pa, sizeof(pst_pci_res->st_ringbuf));
#endif

    /*这里重新刷新h2d ringbuf 指针*/
    oal_pcie_memcopy((oal_ulong)&pst_pci_res->st_ringbuf, (oal_ulong)pst_pci_res->st_ringbuf_map.va, sizeof(pst_pci_res->st_ringbuf));

    /*初始化RX BUFF*/
    for(i = 0 ; i < PCIE_H2D_QTYPE_BUTT; i++)
    {
        oal_ulong offset;
        ret = oal_pcie_inbound_ca_to_va(pst_pci_res, pst_pci_res->st_ringbuf.st_h2d_buf[i].base_addr, &st_map);
        if(OAL_SUCC != ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "invalid d2h ringbuf[%d] base address 0x%llx, map failed, ret=%d\n",
                            i, pst_pci_res->st_ringbuf.st_h2d_buf[i].base_addr, ret);
            return -OAL_ENOMEM;
        }
        oal_memcopy(&pst_pci_res->st_tx_res[i].ringbuf_data_dma_addr, &st_map, sizeof(pst_pci_res->st_tx_res[i].ringbuf_data_dma_addr));
        offset = (oal_ulong)&pst_pci_res->st_ringbuf.st_h2d_buf[i] - (oal_ulong)&pst_pci_res->st_ringbuf;
        pst_pci_res->st_tx_res[i].ringbuf_ctrl_dma_addr.pa = pst_pci_res->st_ringbuf_map.pa + offset;
        pst_pci_res->st_tx_res[i].ringbuf_ctrl_dma_addr.va = pst_pci_res->st_ringbuf_map.va + offset;
    }

    return OAL_SUCC;
}

/*初始化Host ringbuf 和 Device ringbuf 的映射*/
oal_int32 oal_pcie_ringbuf_res_map(oal_pcie_res* pst_pci_res)
{
    oal_int32 ret;
    oal_int32 i;
    oal_uint8 reg = 0;
    oal_pci_dev_stru *pst_pci_dev;
    pci_addr_map      st_map;/*DEVICE CPU地址*/
    pcie_share_mem_stru st_share_mem;

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)pst_pci_res->dev_share_mem.pa, sizeof(pcie_share_mem_stru));
#endif

    oal_pci_read_config_byte(pst_pci_dev, PCI_CACHE_LINE_SIZE, &reg);
    PCI_PRINT_LOG(PCI_LOG_INFO, "L1_CACHE_BYTES: %d\n", reg);

    PCI_PRINT_LOG(PCI_LOG_INFO, "pst_pci_res->dev_share_mem.va:%lx", pst_pci_res->dev_share_mem.va);

    oal_pcie_memcopy((oal_ulong)&st_share_mem,  (oal_ulong)pst_pci_res->dev_share_mem.va, sizeof(pcie_share_mem_stru));
    oal_print_hex_dump((oal_uint8 * )pst_pci_res->dev_share_mem.va, sizeof(pcie_share_mem_stru), 32, "st_share_mem: ");
    PCI_PRINT_LOG(PCI_LOG_INFO, "st_share_mem.ringbuf_res_paddr :0x%x\n", st_share_mem.ringbuf_res_paddr);

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, st_share_mem.ringbuf_res_paddr, &st_map);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "invalid ringbuf device address 0x%x, map failed\n", st_share_mem.ringbuf_res_paddr);
        oal_print_hex_dump((oal_uint8*)&st_share_mem, OAL_SIZEOF(st_share_mem), 32, "st_share_mem: ");
        return -OAL_ENOMEM;
    }

    /*h->h*/
    oal_memcopy(&pst_pci_res->st_ringbuf_map, &st_map, sizeof(pst_pci_res->st_ringbuf_map));

    /*device的ringbuf管理结构同步到Host*/
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)pst_pci_res->st_ringbuf_map.pa, sizeof(pst_pci_res->st_ringbuf));
#endif
    oal_pcie_memcopy((oal_ulong)&pst_pci_res->st_ringbuf, (oal_ulong)pst_pci_res->st_ringbuf_map.va, sizeof(pst_pci_res->st_ringbuf));

    /*初始化ringbuf 管理结构体的映射*/
    pst_pci_res->st_rx_res.ringbuf_ctrl_dma_addr.pa = pst_pci_res->st_ringbuf_map.pa + OAL_OFFSET_OF(pcie_ringbuf_res,st_d2h_buf);
    pst_pci_res->st_rx_res.ringbuf_ctrl_dma_addr.va = pst_pci_res->st_ringbuf_map.va + OAL_OFFSET_OF(pcie_ringbuf_res,st_d2h_buf);


    /*TBD:TBC*/
    /*初始化TX BUFF, 不考虑大小端，host/dev 都是小端，否者这里的base_addr需要转换*/
    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, pst_pci_res->st_ringbuf.st_d2h_buf.base_addr, &st_map);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "invalid h2d ringbuf base address 0x%llx, map failed\n", pst_pci_res->st_ringbuf.st_d2h_buf.base_addr);
        return -OAL_ENOMEM;
    }

    oal_memcopy((oal_void*)&pst_pci_res->st_rx_res.ringbuf_data_dma_addr, (oal_void*)&st_map, sizeof(pst_pci_res->st_rx_res.ringbuf_data_dma_addr));


    /*初始化RX BUFF*/
    for(i = 0 ; i < PCIE_H2D_QTYPE_BUTT; i++)
    {
        oal_ulong offset;
        ret = oal_pcie_inbound_ca_to_va(pst_pci_res, pst_pci_res->st_ringbuf.st_h2d_buf[i].base_addr, &st_map);
        if(OAL_SUCC != ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "invalid d2h ringbuf[%d] base address 0x%llx, map failed, ret=%d\n",
                            i, pst_pci_res->st_ringbuf.st_h2d_buf[i].base_addr, ret);
            return -OAL_ENOMEM;
        }
        oal_memcopy(&pst_pci_res->st_tx_res[i].ringbuf_data_dma_addr, &st_map, sizeof(pst_pci_res->st_tx_res[i].ringbuf_data_dma_addr));
        offset = (oal_ulong)&pst_pci_res->st_ringbuf.st_h2d_buf[i] - (oal_ulong)&pst_pci_res->st_ringbuf;
        pst_pci_res->st_tx_res[i].ringbuf_ctrl_dma_addr.pa = pst_pci_res->st_ringbuf_map.pa + offset;
        pst_pci_res->st_tx_res[i].ringbuf_ctrl_dma_addr.va = pst_pci_res->st_ringbuf_map.va + offset;
    }

    /*初始化消息TX RINGBUFF*/
    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, pst_pci_res->st_ringbuf.st_h2d_msg.base_addr, &st_map);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "invalid h2d message ringbuf base address 0x%llx, map failed, ret=%d\n", pst_pci_res->st_ringbuf.st_h2d_msg.base_addr, ret);
        return -OAL_ENOMEM;
    }

    /*h2d message data 所在DMA地址*/
    oal_memcopy((oal_void*)&pst_pci_res->st_message_res.h2d_res.ringbuf_data_dma_addr,
                    (oal_void*)&st_map, sizeof(st_map));

    /*h2d message ctrl 结构体 所在DMA地址*/
    pst_pci_res->st_message_res.h2d_res.ringbuf_ctrl_dma_addr.va = pst_pci_res->st_ringbuf_map.va + OAL_OFFSET_OF(pcie_ringbuf_res,st_h2d_msg);
    pst_pci_res->st_message_res.h2d_res.ringbuf_ctrl_dma_addr.pa = pst_pci_res->st_ringbuf_map.pa + OAL_OFFSET_OF(pcie_ringbuf_res,st_h2d_msg);

    /*初始化消息RX RINGBUFF*/
    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, pst_pci_res->st_ringbuf.st_d2h_msg.base_addr, &st_map);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "invalid d2h message ringbuf base address 0x%llx, map failed, ret=%d\n", pst_pci_res->st_ringbuf.st_d2h_msg.base_addr, ret);
        return -OAL_ENOMEM;
    }

    /*d2h message data 所在DMA地址*/
    oal_memcopy((oal_void*)&pst_pci_res->st_message_res.d2h_res.ringbuf_data_dma_addr,
                    (oal_void*)&st_map, sizeof(st_map));

    /*d2h message ctrl 结构体 所在DMA地址*/
    pst_pci_res->st_message_res.d2h_res.ringbuf_ctrl_dma_addr.va = pst_pci_res->st_ringbuf_map.va + OAL_OFFSET_OF(pcie_ringbuf_res,st_d2h_msg);
    pst_pci_res->st_message_res.d2h_res.ringbuf_ctrl_dma_addr.pa = pst_pci_res->st_ringbuf_map.pa + OAL_OFFSET_OF(pcie_ringbuf_res,st_d2h_msg);

#ifdef _PRE_PLAT_FEATURE_PCIE_DEVICE_STAT
    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, st_share_mem.device_stat_paddr, &st_map);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "invalid device_stat_paddr  0x%x, map failed\n", st_share_mem.device_stat_paddr);
        return -OAL_ENOMEM;
    }

    /*h->h*/
    oal_memcopy(&pst_pci_res->st_device_stat_map, &st_map, sizeof(pst_pci_res->st_device_stat_map));
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)pst_pci_res->st_device_stat_map.pa, sizeof(pst_pci_res->st_device_stat));
#endif

    oal_pcie_memcopy((oal_ulong)&pst_pci_res->st_device_stat, (oal_ulong)pst_pci_res->st_device_stat_map.va, sizeof(pst_pci_res->st_device_stat));
#endif

    ret = oal_pcie_comm_ringbuf_res_map(pst_pci_res);
    if(OAL_SUCC != ret)
    {
        return ret;
    }

    ret = oal_pcie_device_shared_addr_res_map(pst_pci_res, &st_share_mem);
    if(OAL_SUCC != ret)
    {
        return ret;
    }

    PCI_PRINT_LOG(PCI_LOG_INFO, "oal_pcie_ringbuf_res_map succ");
    return OAL_SUCC;

}

/*edma read 对应device->host, ringbuf_write 指更新写指针*/
oal_int32 oal_pcie_d2h_ringbuf_write(oal_pcie_res* pst_pci_res,
                                                    pci_addr_map*     pst_map,
                                                    pcie_write_ringbuf_item* pst_item)
{
    /*不判断写指针，此函数只执行写操作*/
    oal_pci_dev_stru *pst_pci_dev;
    oal_uint32 real_wr;

    pcie_ringbuf* pst_ringbuf = &pst_pci_res->st_ringbuf.st_d2h_buf;
    pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);

    /*Debug*/
    if(OAL_UNLIKELY(pst_ringbuf->item_len != sizeof(pcie_write_ringbuf_item)))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "[%s]invalid item_len [%u!=%lu]\n", __FUNCTION__, pst_ringbuf->item_len, (oal_ulong)sizeof(pcie_write_ringbuf_item));
        return 0;
    }

    if(OAL_WARN_ON(pst_ringbuf->wr - pst_ringbuf->rd >= pst_ringbuf->size))
    {
        /*never touch here*/
        PCI_PRINT_LOG(PCI_LOG_ERR, "ringbuf full [wr:%u] [rd:%u] [size:%u]\n", pst_ringbuf->wr, pst_ringbuf->rd, pst_ringbuf->size);
        return 0;
    }

    //oal_pcie_mips_start(PCIE_MIPS_RX_RINGBUF_WRITE);
    real_wr = pst_ringbuf->wr & (pst_ringbuf->size - 1);
    oal_pcie_memcopy((oal_ulong)((oal_ulong)pst_map->va + real_wr), (oal_ulong)pst_item, pst_ringbuf->item_len);
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_flush(pst_pci_dev, (oal_void*)pst_map->pa, pst_ringbuf->item_len);
#endif
    pst_ringbuf->wr += pst_ringbuf->item_len;
    //oal_pcie_mips_end(PCIE_MIPS_RX_RINGBUF_WRITE);

    return 1;

}

oal_uint32 oal_pcie_d2h_ringbuf_freecount(oal_pcie_res* pst_pci_res, oal_int32 is_sync)
{
    pcie_ringbuf* pst_ringbuf = &pst_pci_res->st_ringbuf.st_d2h_buf;

    if(OAL_TRUE == is_sync)
    {
        /*同步Dev2Host的读指针*/
        oal_pcie_d2h_ringbuf_rd_update(pst_pci_res);
    }

    return oal_pcie_ringbuf_freecount(pst_ringbuf);
}

oal_int32 oal_pcie_d2h_ringbuf_wr_update(oal_pcie_res* pst_pci_res)
{
    /*d2h方向，同步host的ringbuf管理结构体的写指针到DEVICE侧,
      需要刷cache*/
    pci_addr_map st_map;

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
#endif

    oal_pcie_mips_start(PCIE_MIPS_RX_RINGBUF_WR_UPDATE);
    st_map.va = pst_pci_res->st_rx_res.ringbuf_ctrl_dma_addr.va + OAL_OFFSET_OF(pcie_ringbuf, wr);
    st_map.pa = pst_pci_res->st_rx_res.ringbuf_ctrl_dma_addr.pa + OAL_OFFSET_OF(pcie_ringbuf, wr);
    PCI_PRINT_LOG(PCI_LOG_DBG, "d2h ringbuf wr upate wr:%u", pst_pci_res->st_ringbuf.st_d2h_buf.wr);
    oal_pcie_write_mem32(st_map.va, pst_pci_res->st_ringbuf.st_d2h_buf.wr);
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_flush(pst_pci_dev, (oal_void*)st_map.pa, sizeof(pst_pci_res->st_ringbuf.st_d2h_buf.wr));
#endif
    oal_pcie_mips_end(PCIE_MIPS_RX_RINGBUF_WR_UPDATE);

    return OAL_SUCC;
}

oal_int32 oal_pcie_d2h_ringbuf_rd_update(oal_pcie_res* pst_pci_res)
{
    /*d2h方向，同步device的读指针到HOST ringbuf管理结构体*/
    /*需要刷cache*/
    oal_uint32 rd;
    pci_addr_map st_map;
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
#endif

    oal_pcie_mips_start(PCIE_MIPS_RX_RINGBUF_RD_UPDATE);
    st_map.va = pst_pci_res->st_rx_res.ringbuf_ctrl_dma_addr.va + OAL_OFFSET_OF(pcie_ringbuf, rd);
    st_map.pa = pst_pci_res->st_rx_res.ringbuf_ctrl_dma_addr.pa + OAL_OFFSET_OF(pcie_ringbuf, rd);

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    /*无效化cache*/
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)st_map.pa, sizeof(pst_pci_res->st_ringbuf.st_d2h_buf.rd));
#endif

    rd = oal_pcie_read_mem32(st_map.va);
    if(OAL_UNLIKELY(0xFFFFFFFF == rd))
    {
        if(OAL_FALSE == oal_pcie_check_link_state(pst_pci_res))
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "d2h ringbuf rd update: link down[va:0x%lx, pa:0x%lx]", st_map.va, st_map.pa);
            return -OAL_ENODEV;
        }
    }
    PCI_PRINT_LOG(PCI_LOG_DBG, "d2h ringbuf rd update:[0x%lx:rd:%u]", st_map.va, rd);
    if(OAL_UNLIKELY(rd < pst_pci_res->st_ringbuf.st_d2h_buf.rd))
    {
        /*判断rd 翻转*/
        PCI_PRINT_LOG(PCI_LOG_INFO, "d2h new rd %u over old rd %u, wr:%u",
                                     rd, pst_pci_res->st_ringbuf.st_d2h_buf.rd,
                                    pst_pci_res->st_ringbuf.st_d2h_buf.wr);
        DECLARE_DFT_TRACE_KEY_INFO("d2h_ringbuf_overrun", OAL_DFT_TRACE_SUCC);
    }
    pst_pci_res->st_ringbuf.st_d2h_buf.rd = rd;
    oal_pcie_mips_end(PCIE_MIPS_RX_RINGBUF_RD_UPDATE);

    return OAL_SUCC;;
}

oal_int32 oal_pcie_h2d_ringbuf_write(oal_pcie_res* pst_pci_res,
                                                    pci_addr_map*     pst_map,
                                                    PCIE_H2D_RINGBUF_QTYPE qtype,
                                                    pcie_read_ringbuf_item* pst_item)
{
    /*不判断写指针，此函数只执行写操作*/
    oal_pci_dev_stru *pst_pci_dev;
    oal_uint32 real_wr;

    pcie_ringbuf* pst_ringbuf = &pst_pci_res->st_ringbuf.st_h2d_buf[qtype];

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);

    /*Debug*/
    if(OAL_UNLIKELY(pst_pci_res->link_state < PCI_WLAN_LINK_WORK_UP ))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "oal_pcie_h2d_ringbuf_write invaild, link_state:%s", oal_pcie_get_link_state_str(pst_pci_res->link_state));
        return -OAL_ENODEV;
    }

    /*Debug*/
    if(OAL_UNLIKELY(pst_ringbuf->item_len != sizeof(pcie_read_ringbuf_item)))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "[%s]invalid item_len [%u!=%lu]\n", __FUNCTION__, pst_ringbuf->item_len, (oal_ulong)sizeof(pcie_read_ringbuf_item));
        return 0;
    }

    if(OAL_WARN_ON(pst_ringbuf->wr - pst_ringbuf->rd >= pst_ringbuf->size))
    {
        /*never touch here*/
        PCI_PRINT_LOG(PCI_LOG_ERR, "ringbuf full [wr:%u] [rd:%u] [size:%u]\n", pst_ringbuf->wr, pst_ringbuf->rd, pst_ringbuf->size);
        return 0;
    }

    real_wr = pst_ringbuf->wr & (pst_ringbuf->size - 1);
    oal_pcie_memcopy((oal_ulong)((oal_ulong)pst_map->va + real_wr), (oal_ulong)pst_item, pst_ringbuf->item_len);
    if(PCI_DBG_CONDTION())
    {
        oal_int32 ret;
        oal_uint64 cpuaddr;
        ret = oal_pcie_get_ca_by_pa(pst_pci_res, pst_map->pa, &cpuaddr);
        if(OAL_SUCC == ret)
        {
            PCI_PRINT_LOG(PCI_LOG_DBG, "h2d ringbuf write ringbuf data cpu address:0x%llx", cpuaddr);
        } else {
            PCI_PRINT_LOG(PCI_LOG_DBG, "h2d ringbuf rd pa:0x%lx invaild", pst_map->pa);
        }
        oal_print_hex_dump((oal_uint8 *)pst_item, pst_ringbuf->item_len, pst_ringbuf->item_len, "ringbuf write: ");
    }
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_flush(pst_pci_dev, (oal_void*)pst_map->pa, pst_ringbuf->item_len);
#endif
    pst_ringbuf->wr += pst_ringbuf->item_len;

    return 1;

}

oal_int32 oal_pcie_h2d_ringbuf_wr_update(oal_pcie_res* pst_pci_res, PCIE_H2D_RINGBUF_QTYPE qtype)
{
    /*h2d方向，同步host的ringbuf管理结构体的写指针到DEVICE侧,
      需要刷cache*/
    pci_addr_map st_map;
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
#endif

    st_map.va = pst_pci_res->st_tx_res[qtype].ringbuf_ctrl_dma_addr.va + OAL_OFFSET_OF(pcie_ringbuf, wr);
    st_map.pa = pst_pci_res->st_tx_res[qtype].ringbuf_ctrl_dma_addr.pa + OAL_OFFSET_OF(pcie_ringbuf, wr);
    PCI_PRINT_LOG(PCI_LOG_DBG, "h2d ringbuf wr upate wr:%u", pst_pci_res->st_ringbuf.st_h2d_buf[qtype].wr);

    if(OAL_UNLIKELY(pst_pci_res->link_state <= PCI_WLAN_LINK_DOWN))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "pcie is linkdown");
        return -OAL_ENODEV;
    }

    oal_pcie_write_mem32(st_map.va, pst_pci_res->st_ringbuf.st_h2d_buf[qtype].wr);
    if(PCI_DBG_CONDTION())
    {
        oal_int32 ret;
        oal_uint64 cpuaddr;
        ret = oal_pcie_get_ca_by_pa(pst_pci_res, st_map.pa, &cpuaddr);
        if(OAL_SUCC == ret)
        {
            PCI_PRINT_LOG(PCI_LOG_DBG, "h2d ringbuf wr cpu address:0x%llx", cpuaddr);
        } else {
            PCI_PRINT_LOG(PCI_LOG_DBG, "h2d ringbuf wr pa:0x%lx invaild", st_map.pa);
        }
    }

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_flush(pst_pci_dev, (oal_void*)st_map.pa, sizeof(pst_pci_res->st_ringbuf.st_h2d_buf[qtype].wr));
#endif

    return OAL_SUCC;
}

oal_int32 oal_pcie_h2d_ringbuf_rd_update(oal_pcie_res* pst_pci_res, PCIE_H2D_RINGBUF_QTYPE qtype)
{
    /*h2d方向，同步device的读指针到HOST ringbuf管理结构体*/
    /*需要刷cache*/
    oal_uint32 value;
    pci_addr_map st_map;
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
#endif

    st_map.va = pst_pci_res->st_tx_res[qtype].ringbuf_ctrl_dma_addr.va + OAL_OFFSET_OF(pcie_ringbuf, rd);
    st_map.pa = pst_pci_res->st_tx_res[qtype].ringbuf_ctrl_dma_addr.pa + OAL_OFFSET_OF(pcie_ringbuf, rd);

    if(PCI_DBG_CONDTION())
    {
        oal_int32 ret;
        oal_uint64 cpuaddr;
        ret = oal_pcie_get_ca_by_pa(pst_pci_res, st_map.pa, &cpuaddr);
        if(OAL_SUCC == ret)
        {
            PCI_PRINT_LOG(PCI_LOG_DBG, "h2d ringbuf rd cpu address:0x%llx", cpuaddr);
        } else {
            PCI_PRINT_LOG(PCI_LOG_DBG, "h2d ringbuf rd pa:0x%lx invaild", st_map.pa);
        }
    }

    if(OAL_UNLIKELY(pst_pci_res->link_state <= PCI_WLAN_LINK_DOWN))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "pcie is linkdown");
        return -OAL_ENODEV;
    }

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)st_map.pa, sizeof(pst_pci_res->st_ringbuf.st_h2d_buf[qtype].rd));
#endif
    value = oal_pcie_read_mem32(st_map.va);
    if(OAL_UNLIKELY(0xFFFFFFFF == value))
    {
        if(OAL_FALSE == oal_pcie_check_link_state(pst_pci_res))
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "h2d ringbuf rd update: link down[va:0x%lx, pa:0x%lx]", st_map.va, st_map.pa);
            return -OAL_ENODEV;
        }
    }
    PCI_PRINT_LOG(PCI_LOG_DBG, "h2d ringbuf rd upate rd:%u, curr wr:%u", value, pst_pci_res->st_ringbuf.st_h2d_buf[qtype].wr);
    if(OAL_UNLIKELY(value < pst_pci_res->st_ringbuf.st_h2d_buf[qtype].rd))
    {
        /*判断rd 翻转*/
        PCI_PRINT_LOG(PCI_LOG_INFO, "h2d qtype %d new rd %u over old rd %u, wr:%u", qtype,
                                    value, pst_pci_res->st_ringbuf.st_h2d_buf[qtype].rd,
                                    pst_pci_res->st_ringbuf.st_h2d_buf[qtype].wr);
        DECLARE_DFT_TRACE_KEY_INFO("h2d_ringbuf_overrun", OAL_DFT_TRACE_SUCC);
    }
    pst_pci_res->st_ringbuf.st_h2d_buf[qtype].rd = value;

    return OAL_SUCC;
}

/*获取ringbuf剩余空间大小，is_sync为TRUE时 先从DEVICE同步读指针再判断*/
oal_uint32 oal_pcie_h2d_ringbuf_freecount(oal_pcie_res* pst_pci_res, PCIE_H2D_RINGBUF_QTYPE qtype, oal_int32 is_sync)
{
    pcie_ringbuf* pst_ringbuf = &pst_pci_res->st_ringbuf.st_h2d_buf[qtype];

    if(OAL_TRUE == is_sync)
    {
        /*同步Host2Dev的读指针*/
        oal_pcie_h2d_ringbuf_rd_update(pst_pci_res, qtype);
    }

    oal_pcie_print_ringbuf_info(pst_ringbuf, PCI_LOG_DBG);
    return oal_pcie_ringbuf_freecount(pst_ringbuf);
}

oal_uint32 oal_pcie_h2d_ringbuf_is_empty(oal_pcie_res* pst_pci_res, PCIE_H2D_RINGBUF_QTYPE qtype, oal_int32 is_sync)
{
    pcie_ringbuf* pst_ringbuf = &pst_pci_res->st_ringbuf.st_h2d_buf[qtype];

    if(OAL_TRUE == is_sync)
    {
        /*同步Host2Dev的读指针*/
        oal_pcie_h2d_ringbuf_rd_update(pst_pci_res, qtype);
    }

    oal_pcie_print_ringbuf_info(pst_ringbuf, PCI_LOG_DBG);
    return oal_pcie_ringbuf_is_empty(pst_ringbuf);
}

oal_int32 oal_pcie_h2d_doorbell(oal_pcie_res* pst_pci_res)
{
    /*敲铃,host->device ringbuf 有数据更新,2个队列共享一个中断*/
    /*TBD:TBC*/
    pst_pci_res->stat.h2d_doorbell_cnt++;
    PCI_PRINT_LOG(PCI_LOG_DBG, "oal_pcie_h2d_doorbell,cnt:%u", pst_pci_res->stat.h2d_doorbell_cnt);
    if(OAL_UNLIKELY(pst_pci_res->link_state <= PCI_WLAN_LINK_DOWN))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "pcie is linkdown");
        return -OAL_ENODEV;
    }
    oal_writel(PCIE_H2D_DOORBELL_TRIGGER_VALUE, pst_pci_res->pst_pci_ctrl_base + PCIE_H2D_DOORBELL_OFF);
    return OAL_SUCC;
}

oal_int32 oal_pcie_d2h_doorbell(oal_pcie_res* pst_pci_res)
{
    /*敲铃,host->device ringbuf 有数据更新,2个队列共享一个中断*/
    /*TBD:TBC*/
    pst_pci_res->stat.d2h_doorbell_cnt++;
    PCI_PRINT_LOG(PCI_LOG_DBG, "oal_pcie_d2h_doorbell,cnt:%u", pst_pci_res->stat.d2h_doorbell_cnt);
    if(OAL_UNLIKELY(pst_pci_res->link_state <= PCI_WLAN_LINK_DOWN))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "pcie is linkdown");
        return -OAL_ENODEV;
    }
    oal_writel(PCIE_D2H_DOORBELL_TRIGGER_VALUE, pst_pci_res->pst_pci_ctrl_base + PCIE_D2H_DOORBELL_OFF);
    return OAL_SUCC;
}

/*队列从队头出队*/
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_H2D_BYPASS
oal_int32 oal_pcie_send_netbuf(oal_pcie_res* pst_pci_res, oal_netbuf_stru *pst_netbuf, PCIE_H2D_RINGBUF_QTYPE qtype)
{
    oal_ulong flags;
    oal_pci_dev_stru *pst_pci_dev;
    pcie_read_ringbuf_item st_item;
    oal_int32 send_cnt, queue_cnt, total_cnt;

    if(OAL_UNLIKELY(NULL == pst_pci_res))
        return 0;

    queue_cnt = 1;
    send_cnt = oal_pcie_h2d_ringbuf_freecount(pst_pci_res, qtype, OAL_FALSE);

    if(queue_cnt > send_cnt)
    {
        /*ringbuf 空间不够, 刷新rd指针，重新判断*/
        send_cnt = oal_pcie_h2d_ringbuf_freecount(pst_pci_res, qtype, OAL_TRUE);
    }

    if(0 == send_cnt)
    {
        /*ringbuf 为满*/
        return 0;
    }

    PCI_PRINT_LOG(PCI_LOG_DBG, "h2d_ringbuf freecount:%u, qlen:%u", send_cnt, queue_cnt);

    total_cnt = 0;
    pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);

    //for(;;)
    //{

        /*64bits 传输, 不考虑大小端*/
        st_item.buff_paddr.addr = (oal_uint64)g_h2d_pci_dma_addr;

        /*这里的长度包含64B的头*/
        if(OAL_LIKELY(OAL_NETBUF_LEN(pst_netbuf) > HCC_HDR_TOTAL_LEN))
        {
            /*tx ringbuf中的长度不包含头,就算包含也只是多传输一个头的长度*/
            st_item.buf_len = PADDING((OAL_NETBUF_LEN(pst_netbuf) - HCC_HDR_TOTAL_LEN), 4) ;
        }
        else
        {
            PCI_PRINT_LOG(PCI_LOG_WARN, "tx netbuf too short %u < %u\n", OAL_NETBUF_LEN(pst_netbuf), HCC_HDR_TOTAL_LEN);
            DECLARE_DFT_TRACE_KEY_INFO("tx netbuf too short", OAL_DFT_TRACE_FAIL);
        }

        st_item.reserved0 = 0x4321;

        PCI_PRINT_LOG(PCI_LOG_DBG, "h2d ringbuf write 0x%llx, len:%u", st_item.buff_paddr.addr, st_item.buf_len);

        /*这里直接写，上面已经判断过ringbuf有空间*/
        total_cnt += oal_pcie_h2d_ringbuf_write(pst_pci_res, &pst_pci_res->st_tx_res[qtype].ringbuf_data_dma_addr, qtype, &st_item);
    return total_cnt;
}
#endif

oal_int32 oal_pcie_tx_is_idle(oal_pcie_res* pst_pci_res, PCIE_H2D_RINGBUF_QTYPE qtype)
{
    if(OAL_WARN_ON(NULL == pst_pci_res))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "pci res is null");
        return OAL_FALSE;
    }

    /*pcie is link*/
    if(OAL_UNLIKELY(pst_pci_res->link_state <= PCI_WLAN_LINK_DOWN))
    {
        return OAL_FALSE;
    }

    return (oal_pcie_h2d_ringbuf_freecount(pst_pci_res, qtype, OAL_FALSE) ? OAL_TRUE : OAL_FALSE);
}

/*write message to ringbuf*/
/*Returns, bytes we wrote to ringbuf*/
oal_int32 oal_pcie_h2d_message_buf_write(oal_pcie_res* pst_pci_res, pcie_ringbuf* pst_ringbuf,
                                                    pci_addr_map* pst_ringbuf_base, oal_uint32 message)
{
    oal_uint32 real_wr;
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev;
#endif

    if(OAL_UNLIKELY(pst_ringbuf->item_len != (oal_uint16)OAL_SIZEOF(message)))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "[%s]invalid item_len [%u!=%lu]\n", __FUNCTION__, pst_ringbuf->item_len, (oal_ulong)OAL_SIZEOF(pcie_read_ringbuf_item));
        return 0;
    }

    if(OAL_WARN_ON(pst_ringbuf->wr - pst_ringbuf->rd >= pst_ringbuf->size))
    {
        /*never touch here*/
        PCI_PRINT_LOG(PCI_LOG_ERR, "message ringbuf full [wr:%u] [rd:%u] [size:%u]\n", pst_ringbuf->wr, pst_ringbuf->rd, pst_ringbuf->size);
        return 0;
    }

    real_wr = pst_ringbuf->wr & (pst_ringbuf->size - 1);
    oal_pcie_memcopy((oal_ulong)((oal_ulong)pst_ringbuf_base->va + real_wr), (oal_ulong)&message, OAL_SIZEOF(message));

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_flush(pst_pci_dev, (oal_void*)pst_ringbuf_base->pa, OAL_SIZEOF(message));
#endif

    pst_ringbuf->wr += OAL_SIZEOF(message);

    return 1;
}

oal_int32 oal_pcie_h2d_message_buf_rd_update(oal_pcie_res* pst_pci_res)
{
    /*需要刷cache*/
    /*h2d方向，同步device的读指针到HOST message ringbuf管理结构体*/
    oal_uint32 rd;
    pci_addr_map st_map;

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
#endif

    st_map.va = pst_pci_res->st_message_res.h2d_res.ringbuf_ctrl_dma_addr.va + OAL_OFFSET_OF(pcie_ringbuf, rd);
    st_map.pa = pst_pci_res->st_message_res.h2d_res.ringbuf_ctrl_dma_addr.pa + OAL_OFFSET_OF(pcie_ringbuf, rd);

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    /*无效化cache*/
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)st_map.pa, sizeof(pst_pci_res->st_ringbuf.st_h2d_msg.rd));
#endif

    rd = oal_pcie_read_mem32(st_map.va);
    if(OAL_UNLIKELY(0xFFFFFFFF == rd))
    {
        if(OAL_FALSE == oal_pcie_check_link_state(pst_pci_res))
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "h2d message ringbuf rd update: link down[va:0x%lx, pa:0x%lx]", st_map.va, st_map.pa);
            return -OAL_ENODEV;
        }
    }
    PCI_PRINT_LOG(PCI_LOG_DBG, "h2d message ringbuf rd update:[0x%lx:rd:0x%x]", st_map.va, rd);
    pst_pci_res->st_ringbuf.st_h2d_msg.rd = rd;

    return OAL_SUCC;;
}

oal_int32 oal_pcie_h2d_message_buf_wr_update(oal_pcie_res* pst_pci_res)
{
    /*需要刷cache*/
    /*h2d方向，同步device的读指针到HOST message ringbuf管理结构体*/
    oal_uint32 wr_back;
    pci_addr_map st_map;

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
#endif

    st_map.va = pst_pci_res->st_message_res.h2d_res.ringbuf_ctrl_dma_addr.va + OAL_OFFSET_OF(pcie_ringbuf, wr);
    st_map.pa = pst_pci_res->st_message_res.h2d_res.ringbuf_ctrl_dma_addr.pa + OAL_OFFSET_OF(pcie_ringbuf, wr);

    oal_pcie_write_mem32(st_map.va, pst_pci_res->st_ringbuf.st_h2d_msg.wr);

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    /*Flush cache*/
    oal_pci_cache_flush(pst_pci_dev, (oal_void*)st_map.pa, sizeof(pst_pci_res->st_ringbuf.st_h2d_msg.wr));
#endif

    wr_back = oal_pcie_read_mem32(st_map.va);
    if(wr_back != pst_pci_res->st_ringbuf.st_h2d_msg.wr)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pcie h2d message wr write failed, wr_back=%u, host_wr=%u", wr_back, pst_pci_res->st_ringbuf.st_h2d_msg.wr);
        DECLARE_DFT_TRACE_KEY_INFO("h2d_message_wr_update_failed", OAL_DFT_TRACE_FAIL);
    }

    return OAL_SUCC;;
}

/*update wr pointer to host ,check the read space*/
oal_int32 oal_pcie_d2h_message_buf_wr_update(oal_pcie_res* pst_pci_res)
{
    oal_uint32 wr;
    pci_addr_map st_map;
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
#endif

    st_map.va = pst_pci_res->st_message_res.d2h_res.ringbuf_ctrl_dma_addr.va + OAL_OFFSET_OF(pcie_ringbuf, wr);
    st_map.pa = pst_pci_res->st_message_res.d2h_res.ringbuf_ctrl_dma_addr.pa + OAL_OFFSET_OF(pcie_ringbuf, wr);

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    /*无效化cache*/
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)st_map.pa, sizeof(pst_pci_res->st_ringbuf.st_d2h_msg.wr));
#endif
    wr = oal_pcie_read_mem32(st_map.va);
    if(OAL_UNLIKELY(0xFFFFFFFF == wr))
    {
        if(OAL_FALSE == oal_pcie_check_link_state(pst_pci_res))
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "d2h message ringbuf wr update: link down[va:0x%lx, pa:0x%lx]", st_map.va, st_map.pa);
            return -OAL_ENODEV;
        }
    }

    PCI_PRINT_LOG(PCI_LOG_DBG, "d2h message ringbuf wr update:[0x%lx:wr:0x%x]", st_map.va, wr);
    pst_pci_res->st_ringbuf.st_d2h_msg.wr = wr;
    return OAL_SUCC;
}

/*update rd pointer to device*/
oal_int32 oal_pcie_d2h_message_buf_rd_update(oal_pcie_res* pst_pci_res)
{
    pci_addr_map st_map;
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
#endif
    st_map.va = pst_pci_res->st_message_res.d2h_res.ringbuf_ctrl_dma_addr.va + OAL_OFFSET_OF(pcie_ringbuf, rd);
    st_map.pa = pst_pci_res->st_message_res.d2h_res.ringbuf_ctrl_dma_addr.pa + OAL_OFFSET_OF(pcie_ringbuf, rd);

    oal_pcie_write_mem32(st_map.va, pst_pci_res->st_ringbuf.st_d2h_msg.rd);
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    /*Flush cache*/
    oal_pci_cache_flush(pst_pci_dev, (oal_void*)st_map.pa, sizeof(pst_pci_res->st_ringbuf.st_d2h_msg.rd));
#endif
    return OAL_SUCC;
}

/*Update rd pointer,
  Return the bytes we read*/
oal_int32 oal_pcie_d2h_message_buf_read(oal_pcie_res* pst_pci_res, pcie_ringbuf* pst_ringbuf,
                                                    pci_addr_map* pst_ringbuf_base, oal_uint32* message)
{
    oal_uint32 real_rd, wr, rd, data_size;
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
#endif

    rd   = pst_ringbuf->rd;
    wr   = pst_ringbuf->wr;

    data_size = wr - rd;

    if(OAL_UNLIKELY((data_size < pst_ringbuf->item_len) || (pst_ringbuf->item_len != OAL_SIZEOF(oal_uint32))))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "d2h message buf read failed, date_size[%d] < item_len:%d, wr:%u, rd:%u",
                                    data_size, pst_ringbuf->item_len, wr, rd);
        return 0;
    }

    real_rd = rd & (pst_ringbuf->size -1);

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    /*无效化cache*/
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)pst_ringbuf_base->pa, pst_ringbuf->item_len);
#endif

    if(OAL_FALSE == oal_pcie_check_link_state(pst_pci_res))
    {
        /*LinkDown*/
        PCI_PRINT_LOG(PCI_LOG_ERR, "d2h message read detect linkdown.");
        return 0;
    }

    oal_pcie_memcopy((oal_ulong)(message), (oal_ulong)(pst_ringbuf_base->va + (oal_ulong)real_rd), pst_ringbuf->item_len);

    pst_ringbuf->rd += pst_ringbuf->item_len;

    /*Update device's read pointer*/
    oal_pcie_d2h_message_buf_rd_update(pst_pci_res);

    return pst_ringbuf->item_len;
}

/*update rd pointer to device*/
oal_int32 oal_pcie_ringbuf_write_rd(oal_pcie_res* pst_pci_res, PCIE_COMM_RINGBUF_TYPE type)
{
    pci_addr_map st_map;
    pcie_ringbuf* pst_ringbuf;
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
#endif

    if(OAL_UNLIKELY(pst_pci_res->link_state < PCI_WLAN_LINK_RES_UP ))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "comm ringbuf %d write rd failed, link_state:%s", type, oal_pcie_get_link_state_str(pst_pci_res->link_state));
        return -OAL_ENODEV;
    }

    pst_ringbuf = &pst_pci_res->st_ringbuf.st_ringbuf[type];

    st_map.va = pst_pci_res->st_ringbuf_res.comm_rb_res[type].ctrl_daddr.va + OAL_OFFSET_OF(pcie_ringbuf, rd);
    st_map.pa = pst_pci_res->st_ringbuf_res.comm_rb_res[type].ctrl_daddr.pa + OAL_OFFSET_OF(pcie_ringbuf, rd);

    oal_pcie_write_mem32(st_map.va, pst_ringbuf->rd);
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    /*Flush cache*/
    oal_pci_cache_flush(pst_pci_dev, (oal_void*)st_map.pa, sizeof(pst_ringbuf->rd));
#endif
    return OAL_SUCC;
}

oal_int32 oal_pcie_ringbuf_write_wr(oal_pcie_res* pst_pci_res, PCIE_COMM_RINGBUF_TYPE type)
{
    pci_addr_map st_map;
    pcie_ringbuf* pst_ringbuf;

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
#endif

    if(OAL_UNLIKELY(pst_pci_res->link_state < PCI_WLAN_LINK_RES_UP ))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "comm ringbuf %d write wr failed, link_state:%s", type, oal_pcie_get_link_state_str(pst_pci_res->link_state));
        return -OAL_ENODEV;
    }

    pst_ringbuf = &pst_pci_res->st_ringbuf.st_ringbuf[type];

    st_map.va = pst_pci_res->st_ringbuf_res.comm_rb_res[type].ctrl_daddr.va + OAL_OFFSET_OF(pcie_ringbuf, wr);
    st_map.pa = pst_pci_res->st_ringbuf_res.comm_rb_res[type].ctrl_daddr.pa + OAL_OFFSET_OF(pcie_ringbuf, wr);

    oal_pcie_write_mem32(st_map.va, pst_ringbuf->wr);
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    /*Flush cache*/
    oal_pci_cache_flush(pst_pci_dev, (oal_void*)st_map.pa, sizeof(pst_ringbuf->wr));
#endif
    return OAL_SUCC;
}

oal_int32 oal_pcie_ringbuf_read_rd(oal_pcie_res* pst_pci_res, PCIE_COMM_RINGBUF_TYPE type)
{
    oal_uint32 rd;
    pci_addr_map st_map;
    pcie_ringbuf* pst_ringbuf;
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
#endif

    if(OAL_UNLIKELY(pst_pci_res->link_state < PCI_WLAN_LINK_RES_UP ))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "comm ringbuf %d read rd, link_state:%s", type, oal_pcie_get_link_state_str(pst_pci_res->link_state));
        return -OAL_ENODEV;
    }

    pst_ringbuf = &pst_pci_res->st_ringbuf.st_ringbuf[type];

    st_map.va = pst_pci_res->st_ringbuf_res.comm_rb_res[type].ctrl_daddr.va + OAL_OFFSET_OF(pcie_ringbuf, rd);
    st_map.pa = pst_pci_res->st_ringbuf_res.comm_rb_res[type].ctrl_daddr.pa + OAL_OFFSET_OF(pcie_ringbuf, rd);

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    /*无效化cache*/
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)st_map.pa, sizeof(pst_ringbuf->rd));
#endif

    rd = oal_pcie_read_mem32(st_map.va);
    if(OAL_UNLIKELY(0xFFFFFFFF == rd))
    {
        if(OAL_FALSE == oal_pcie_check_link_state(pst_pci_res))
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "ringbuf %d read rd: link down[va:0x%lx, pa:0x%lx]", type, st_map.va, st_map.pa);
            return -OAL_ENODEV;
        }
    }

    PCI_PRINT_LOG(PCI_LOG_DBG, "ringbuf %d read rd:[0x%lx:rd:0x%x]", type, st_map.va, rd);

    pst_ringbuf->rd = rd;

    return OAL_SUCC;
}

/*update wr pointer to device*/
oal_int32 oal_pcie_ringbuf_read_wr(oal_pcie_res* pst_pci_res, PCIE_COMM_RINGBUF_TYPE type)
{
    oal_uint32 wr;
    pci_addr_map st_map;
    pcie_ringbuf* pst_ringbuf;
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
#endif

    if(OAL_UNLIKELY(pst_pci_res->link_state < PCI_WLAN_LINK_RES_UP ))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "comm ringbuf %d read wr failed, link_state:%s", type, oal_pcie_get_link_state_str(pst_pci_res->link_state));
        return -OAL_ENODEV;
    }

    pst_ringbuf = &pst_pci_res->st_ringbuf.st_ringbuf[type];

    st_map.va = pst_pci_res->st_ringbuf_res.comm_rb_res[type].ctrl_daddr.va + OAL_OFFSET_OF(pcie_ringbuf, wr);
    st_map.pa = pst_pci_res->st_ringbuf_res.comm_rb_res[type].ctrl_daddr.pa + OAL_OFFSET_OF(pcie_ringbuf, wr);

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    /*无效化cache*/
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)st_map.pa, sizeof(pst_ringbuf->wr));
#endif

    wr = oal_pcie_read_mem32(st_map.va);
    if(OAL_UNLIKELY(0xFFFFFFFF == wr))
    {
        if(OAL_FALSE == oal_pcie_check_link_state(pst_pci_res))
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "ringbuf %d wr update: link down[va:0x%lx, pa:0x%lx]", type, st_map.va, st_map.pa);
            return -OAL_ENODEV;
        }
    }

    PCI_PRINT_LOG(PCI_LOG_DBG, "ringbuf %d read wr:[0x%lx:wr:0x%x]", type , st_map.va, wr);
    pst_ringbuf->wr = wr;

    return OAL_SUCC;
}


/*Update rd pointer,
  Return the bytes we read*/
oal_int32 oal_pcie_ringbuf_read(oal_pcie_res* pst_pci_res, PCIE_COMM_RINGBUF_TYPE type, oal_uint8* buf, oal_uint32 len)
{
    oal_uint32 real_rd, wr, rd, data_size;
    pcie_ringbuf* pst_ringbuf;
    pci_addr_map* pst_ringbuf_base = &pst_pci_res->st_ringbuf_res.comm_rb_res[type].data_daddr;
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
#endif

    if(OAL_WARN_ON(0 == pst_ringbuf_base->va))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "comm ringbuf %d read failed, ringbuf base va is null", type);
        return -OAL_EINVAL;
    }

    if(OAL_UNLIKELY(pst_pci_res->link_state < PCI_WLAN_LINK_RES_UP))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "comm ringbuf %d read failed, link_state:%s", type, oal_pcie_get_link_state_str(pst_pci_res->link_state));
        return -OAL_ENODEV;
    }

    pst_ringbuf = &pst_pci_res->st_ringbuf.st_ringbuf[type];

    if(OAL_WARN_ON(len != pst_ringbuf->item_len))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN,"ringbuf %d read request len %u not equal to %u", type, len, pst_ringbuf->item_len);
        return -OAL_EIO;
    }

    rd   = pst_ringbuf->rd;
    wr   = pst_ringbuf->wr;

    data_size = wr - rd;

    if((data_size < pst_ringbuf->item_len))
    {
        /*ringbuf is empty*/
        return 0;
    }

    real_rd = rd & (pst_ringbuf->size -1);

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    /*无效化cache*/
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)pst_ringbuf_base->pa, pst_ringbuf->item_len);
#endif

    oal_pcie_memcopy((oal_ulong)(buf), (oal_ulong)(pst_ringbuf_base->va + (oal_ulong)real_rd), pst_ringbuf->item_len);

    if(0xff == *buf)
    {
        if(OAL_FALSE == oal_pcie_check_link_state(pst_pci_res))
        {
            /*LinkDown*/
            PCI_PRINT_LOG(PCI_LOG_ERR, "linkdown when comm ringbuf %d read", type);
            return -OAL_ENODEV;
        }
    }

    pst_ringbuf->rd += pst_ringbuf->item_len;

    /*Update device's read pointer*/
    oal_pcie_ringbuf_write_rd(pst_pci_res, type);

    return pst_ringbuf->item_len;
}

oal_int32 oal_pcie_ringbuf_write(oal_pcie_res* pst_pci_res,
                                                    PCIE_COMM_RINGBUF_TYPE type, oal_uint8* buf, oal_uint32 len)
{
    /*不判断写指针，此函数只执行写操作*/
    oal_pci_dev_stru *pst_pci_dev;
    oal_uint32 real_wr;

    pcie_ringbuf* pst_ringbuf = &pst_pci_res->st_ringbuf.st_ringbuf[type];
    pci_addr_map* pst_ringbuf_base = &pst_pci_res->st_ringbuf_res.comm_rb_res[type].data_daddr;

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);

    /*Debug*/
    if(OAL_UNLIKELY(pst_pci_res->link_state < PCI_WLAN_LINK_RES_UP ))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "comm ringbuf %d write failed, link_state:%s", type, oal_pcie_get_link_state_str(pst_pci_res->link_state));
        return 0;
    }

    if(OAL_WARN_ON(len != pst_ringbuf->item_len))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN,"ringbuf %d write request len %u not equal to %u", type, len, pst_ringbuf->item_len);
        return 0;
    }

    if(OAL_WARN_ON(pst_ringbuf->wr - pst_ringbuf->rd >= pst_ringbuf->size))
    {
        /*never touch here*/
        PCI_PRINT_LOG(PCI_LOG_ERR, "ringbuf %d full [wr:%u] [rd:%u] [size:%u]\n", type, pst_ringbuf->wr, pst_ringbuf->rd, pst_ringbuf->size);
        return 0;
    }

    real_wr = pst_ringbuf->wr & (pst_ringbuf->size - 1);
    oal_pcie_memcopy((oal_ulong)((oal_ulong)pst_ringbuf_base->va + real_wr), (oal_ulong)buf, pst_ringbuf->item_len);
    if(PCI_DBG_CONDTION())
    {
        oal_int32 ret;
        oal_uint64 cpuaddr;
        ret = oal_pcie_get_ca_by_pa(pst_pci_res, pst_ringbuf_base->pa, &cpuaddr);
        if(OAL_SUCC == ret)
        {
            PCI_PRINT_LOG(PCI_LOG_DBG, "ringbuf %d write ringbuf data cpu address:0x%llx", type, cpuaddr);
        } else {
            PCI_PRINT_LOG(PCI_LOG_DBG, "ringbuf %d rd pa:0x%lx invaild", type, pst_ringbuf_base->pa);
        }
        oal_print_hex_dump((oal_uint8 *)buf, pst_ringbuf->item_len, pst_ringbuf->item_len, "ringbuf write: ");
    }

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_flush(pst_pci_dev, (oal_void*)pst_ringbuf_base->pa, pst_ringbuf->item_len);
#endif

    pst_ringbuf->wr += pst_ringbuf->item_len;

    return 1;
}

oal_uint32 oal_pcie_comm_ringbuf_freecount(oal_pcie_res* pst_pci_res,
                                                    PCIE_COMM_RINGBUF_TYPE type)
{
    pcie_ringbuf* pst_ringbuf = &pst_pci_res->st_ringbuf.st_ringbuf[type];
    return oal_pcie_ringbuf_freecount(pst_ringbuf);
}

oal_int32 oal_pcie_read_d2h_message(oal_pcie_res* pst_pci_res, oal_uint32 *message)
{
    oal_int32 ret;
    oal_uint32 len = 0;
    pcie_ringbuf* pst_ringbuf;
    if(OAL_UNLIKELY(NULL == pst_pci_res))
    {
        return -OAL_EINVAL;
    }

    if(OAL_UNLIKELY(pst_pci_res->link_state < PCI_WLAN_LINK_RES_UP))
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "link state is disabled:%s!", oal_pcie_get_link_state_str(pst_pci_res->link_state));
        return -OAL_ENODEV;
    }

    pst_ringbuf = &pst_pci_res->st_ringbuf.st_d2h_msg;
    PCI_PRINT_LOG(PCI_LOG_DBG, "oal_pcie_read_d2h_message ++");

    len = pcie_ringbuf_len(pst_ringbuf);
    if(0 == len)
    {
        /*No Message, update wr pointer and retry*/
        ret = oal_pcie_d2h_message_buf_wr_update(pst_pci_res);
        if(OAL_SUCC != ret)
        {
            return ret;
        }
        len = pcie_ringbuf_len(&pst_pci_res->st_ringbuf.st_d2h_msg);
    }

    if(0 == len)
    {
        return -OAL_ENODEV;
    }

    if(oal_pcie_d2h_message_buf_read(pst_pci_res, pst_ringbuf, &pst_pci_res->st_message_res.d2h_res.ringbuf_data_dma_addr, message))
    {
        PCI_PRINT_LOG(PCI_LOG_DBG, "oal_pcie_read_d2h_message --");
        return OAL_SUCC;
    }
    else
    {
        PCI_PRINT_LOG(PCI_LOG_DBG, "oal_pcie_read_d2h_message ^^");
        return -OAL_EINVAL;
    }
}

oal_int32 oal_pcie_send_message_to_dev(oal_pcie_res* pst_pci_res, oal_uint32 message)
{
    oal_int32  ret;
    oal_uint32 freecount;
    if(OAL_WARN_ON(NULL == pst_pci_res))
    {
        return -OAL_ENODEV;
    }

    if(OAL_UNLIKELY(!pst_pci_res->regions.inited))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "region is disabled!");
        return -OAL_EFAUL;
    }

    if(OAL_UNLIKELY(pst_pci_res->link_state < PCI_WLAN_LINK_RES_UP ))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "send message link invaild, link_state:%s", oal_pcie_get_link_state_str(pst_pci_res->link_state));
        return -OAL_EBUSY;
    }

    /*message ringbuf freecount*/
    oal_spin_lock(&pst_pci_res->st_message_res.h2d_res.lock);
    freecount = oal_pcie_ringbuf_freecount(&pst_pci_res->st_ringbuf.st_h2d_msg);
    if(0 == freecount)
    {
        /*no space, sync rd pointer*/
        oal_pcie_h2d_message_buf_rd_update(pst_pci_res);
        freecount = oal_pcie_ringbuf_freecount(&pst_pci_res->st_ringbuf.st_h2d_msg);
    }

    if(0 == freecount)
    {
        oal_spin_unlock(&pst_pci_res->st_message_res.h2d_res.lock);
        return -OAL_EBUSY;
    }

    /*write message to ringbuf*/
    ret = oal_pcie_h2d_message_buf_write(pst_pci_res, &pst_pci_res->st_ringbuf.st_h2d_msg,
                                &pst_pci_res->st_message_res.h2d_res.ringbuf_data_dma_addr,
                                message);
    if(ret <=0)
    {
        oal_spin_unlock(&pst_pci_res->st_message_res.h2d_res.lock);
        if(OAL_FALSE == oal_pcie_check_link_state(pst_pci_res))
        {
            /*Should trigger DFR here*/
            PCI_PRINT_LOG(PCI_LOG_ERR, "h2d message send failed: link down, ret=%d", ret);
        }
        return -OAL_EIO;
    }

    /*更新写指针*/
    oal_pcie_h2d_message_buf_wr_update(pst_pci_res);

    /*触发h2d int*/
    oal_writel(PCIE_H2D_TRIGGER_VALUE, pst_pci_res->pst_pci_ctrl_base + PCIE_D2H_DOORBELL_OFF);

    oal_spin_unlock(&pst_pci_res->st_message_res.h2d_res.lock);

    return OAL_SUCC;
}

oal_int32 oal_pcie_get_host_trans_count(oal_pcie_res* pst_pci_res, oal_uint64 *tx, oal_uint64 *rx)
{
    if(tx)
    {
        oal_int32 i;
        *tx = 0;
        for(i = 0; i < PCIE_H2D_QTYPE_BUTT; i++)
        {
            *tx += (oal_uint64)pst_pci_res->st_tx_res[i].stat.tx_count;
        }
    }
    if(rx)
    {
        *rx = (oal_uint64)pst_pci_res->st_rx_res.stat.rx_count;
    }
    return OAL_SUCC;
}

oal_int32 oal_pcie_sleep_request_host_check(oal_pcie_res* pst_pci_res)
{
    oal_int32 i;
    oal_uint32 len = 0;
    oal_uint32 total_len = 0;


    /*此时allow sleep 应该tx也被释放*/
    for(i = 0; i < PCIE_H2D_QTYPE_BUTT; i++)
    {
        len = oal_netbuf_list_len(&pst_pci_res->st_tx_res[i].txq);
        if(len)
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "txq:%d still had skb len:%u", i, len);
        }
        total_len += len;
    }

    return (0 != total_len) ? -OAL_EBUSY : OAL_SUCC;
}

oal_int32 oal_pcie_send_netbuf_list(oal_pcie_res* pst_pci_res, oal_netbuf_head_stru *pst_head, PCIE_H2D_RINGBUF_QTYPE qtype)
{
    oal_ulong flags;
    oal_pcie_linux_res * pst_pci_lres;
    oal_netbuf_stru* pst_netbuf;
    oal_pci_dev_stru *pst_pci_dev;
    dma_addr_t pci_dma_addr;
    pcie_read_ringbuf_item st_item;
    pcie_cb_dma_res st_cb_dma;
    oal_int32 send_cnt, queue_cnt, total_cnt;

    if(OAL_UNLIKELY(NULL == pst_pci_res || NULL == pst_head))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "invalid input pst_pci_res is %p, pst_head %p", pst_pci_res, pst_head);
        return -OAL_EINVAL;
    }

    /*pcie is link*/
    if(OAL_UNLIKELY(pst_pci_res->link_state <= PCI_WLAN_LINK_DOWN))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "linkdown hold the pkt, link_state:%s", oal_pcie_get_link_state_str(pst_pci_res->link_state));
        return 0;
    }


    if(OAL_UNLIKELY(oal_netbuf_list_empty(pst_head)))
    {
        return 0;
    }

    /*待优化,TBD:TBC*/
    pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
    pst_pci_lres = (oal_pcie_linux_res*)oal_pci_get_drvdata(pst_pci_dev);

#ifdef CONFIG_ARCH_SD56XX
    /*5610 non pm*/
#else
    if(OAL_UNLIKELY(OAL_SUCC != hcc_bus_pm_wakeup_device(pst_pci_lres->pst_bus)))
    {
        oal_msleep(100);/*wait for a while retry*/
        return -OAL_EIO;
    }
#endif

    if(OAL_UNLIKELY(pst_pci_res->link_state < PCI_WLAN_LINK_WORK_UP ))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "send netbuf link invaild, link_state:%s", oal_pcie_get_link_state_str(pst_pci_res->link_state));
        return -OAL_ENODEV;
    }

    queue_cnt = oal_netbuf_list_len(pst_head);
    send_cnt = oal_pcie_h2d_ringbuf_freecount(pst_pci_res, qtype, OAL_FALSE);

    if(queue_cnt > send_cnt)
    {
        /*ringbuf 空间不够, 刷新rd指针，重新判断*/
        send_cnt = oal_pcie_h2d_ringbuf_freecount(pst_pci_res, qtype, OAL_TRUE);
    }

    if(0 == send_cnt)
    {
        /*ringbuf 为满*/
        return 0;
    }

    PCI_PRINT_LOG(PCI_LOG_DBG, "[q:%d]h2d_ringbuf freecount:%u, qlen:%u", (oal_int32)qtype,send_cnt, queue_cnt);

    total_cnt = 0;

    for(;;)
    {
        /*填充ringbuf*/
        if(0 == oal_pcie_h2d_ringbuf_freecount(pst_pci_res, qtype, OAL_FALSE))
        {
            break;
        }

        /*取netbuf*/
        pst_netbuf = oal_netbuf_delist(pst_head);
        if(NULL == pst_netbuf)
        {
            break;
        }

        /*Debug*/
        if(OAL_UNLIKELY(pst_pci_res->link_state < PCI_WLAN_LINK_WORK_UP ))
        {
            PCI_PRINT_LOG(PCI_LOG_WARN, "oal_pcie_send_netbuf_list loop invaild, link_state:%s", oal_pcie_get_link_state_str(pst_pci_res->link_state));
            hcc_tx_netbuf_free(pst_netbuf);
            return -OAL_ENODEV;
        }

        /*TBD:TBC*/
        pci_dma_addr = dma_map_single(&pst_pci_dev->dev, OAL_NETBUF_DATA(pst_netbuf), OAL_NETBUF_LEN(pst_netbuf), PCI_DMA_TODEVICE);
        if (dma_mapping_error(&pst_pci_dev->dev, pci_dma_addr))
        {
            /*映射失败先简单处理丢掉netbuf, dma mask 范围内 这里只是刷Cache*/
            DECLARE_DFT_TRACE_KEY_INFO("pcie tx map failed", OAL_DFT_TRACE_OTHER);
            hcc_tx_netbuf_free(pst_netbuf);
            continue;
        }

        PCI_PRINT_LOG(PCI_LOG_DBG, "send netbuf 0x%p, dma pa:0x%llx", pst_netbuf, (oal_uint64)pci_dma_addr);
        if(PCI_DBG_CONDTION())
        {
            oal_print_hex_dump(OAL_NETBUF_DATA(pst_netbuf),
                                OAL_NETBUF_LEN(pst_netbuf) < 128 ? OAL_NETBUF_LEN(pst_netbuf) :128,
                                32, "netbuf: ");
        }

        /*64bits 传输, 不考虑大小端*/
        st_item.buff_paddr.addr = (oal_uint64)pci_dma_addr;

        /*这里的长度包含64B的头*/
        if(OAL_LIKELY(OAL_NETBUF_LEN(pst_netbuf) >= HCC_HDR_TOTAL_LEN))
        {
            /*tx ringbuf中的长度不包含头,就算包含也只是多传输一个头的长度*/
            st_item.buf_len = PADDING((OAL_NETBUF_LEN(pst_netbuf) - HCC_HDR_TOTAL_LEN), 4) ;
        }
        else
        {
            PCI_PRINT_LOG(PCI_LOG_WARN, "tx netbuf too short %u < %u\n", OAL_NETBUF_LEN(pst_netbuf), HCC_HDR_TOTAL_LEN);
            DECLARE_DFT_TRACE_KEY_INFO("tx netbuf too short", OAL_DFT_TRACE_FAIL);
        }

        st_item.reserved0 = 0x4321;

        st_cb_dma.paddr.addr = (oal_uint64)pci_dma_addr;
        st_cb_dma.len   = OAL_NETBUF_LEN(pst_netbuf);

        /*dma地址和长度存在CB字段中，发送完成后释放DMA地址*/
        oal_memcopy((oal_uint8*)OAL_NETBUF_CB(pst_netbuf) + sizeof(struct hcc_tx_cb_stru), &st_cb_dma, sizeof(st_cb_dma));

        /*netbuf入队*/
        oal_spin_lock_irq_save(&pst_pci_res->st_tx_res[qtype].lock, &flags);
        oal_netbuf_list_tail_nolock(&pst_pci_res->st_tx_res[qtype].txq, pst_netbuf);
        oal_spin_unlock_irq_restore(&pst_pci_res->st_tx_res[qtype].lock, &flags);

        PCI_PRINT_LOG(PCI_LOG_DBG, "h2d ringbuf write 0x%llx, len:%u", st_item.buff_paddr.addr, st_item.buf_len);

        /*这里直接写，上面已经判断过ringbuf有空间*/
        total_cnt += oal_pcie_h2d_ringbuf_write(pst_pci_res, &pst_pci_res->st_tx_res[qtype].ringbuf_data_dma_addr, qtype, &st_item);
    }

    if(total_cnt)
    {
        /*更新device侧wr指针,刷ringbuf cache*/
        oal_pcie_h2d_ringbuf_wr_update(pst_pci_res, qtype);

        /*tx doorbell*/
        oal_pcie_h2d_doorbell(pst_pci_res);
    }
    else
    {
        DECLARE_DFT_TRACE_KEY_INFO("pcie send toal cnt error", OAL_DFT_TRACE_FAIL);
    }

    return total_cnt;
}

oal_int32 oal_pcie_print_pcie_regs(oal_pcie_res* pst_pci_res, oal_uint32 base, oal_uint32 size)
{
    oal_int32 i;
    oal_int32 ret;
    oal_uint32 value;
    oal_void* pst_mem = NULL;
    pci_addr_map addr_map;
    size = OAL_ROUND_UP(size, 4);

    ret = oal_pcie_inbound_ca_to_va(pst_pci_res, base, &addr_map);
    if(OAL_UNLIKELY(OAL_SUCC != ret))
    {
        /*share mem 地址未映射!*/
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "can not found mem map for dev cpu address 0x%x\n", base);
        return -OAL_EFAIL;
    }

    pst_mem = vmalloc(size);
    if(NULL == pst_mem)
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "vmalloc mem size %u failed", size);
    }
    else
    {
        OAL_MEMZERO(pst_mem, size);
    }

    for(i = 0; i < size ; i += 4)
    {
        value = oal_readl((oal_void*)addr_map.va + i);
        if(0xffffffff == value)
        {
            ret = oal_pcie_device_check_alive(pst_pci_res);
            if(OAL_SUCC != ret)
            {
                if(NULL != pst_mem)
                {
                    vfree(pst_mem);
                }
                return -OAL_ENODEV;
            }
        }

        if(pst_mem)
        {
            oal_writel(value, pst_mem + i);
        }
        else
        {
            OAL_IO_PRINT("%8x:%8x\n", base + i , value);
        }
    }

    if(pst_mem)
    {
        if(i)
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "dump regs base 0x%x", base);
#ifdef CONFIG_PRINTK
            /*print to kenrel msg*/
            print_hex_dump(KERN_DEBUG, "pcie regs: ", DUMP_PREFIX_OFFSET, 32, 4,
               pst_mem, i, false);
#endif
        }
    }

    if(NULL != pst_mem)
    {
        vfree(pst_mem);
    }

    return OAL_SUCC;
}


oal_int32 oal_pcie_print_device_transfer_info(oal_pcie_res* pst_pcie_res)
{
    oal_int32 ret;
    if(0 != pst_pcie_res->st_device_stat_map.va)
    {
        ret = oal_pcie_device_check_alive(pst_pcie_res);
        if(OAL_SUCC != ret)
        {
            return ret;
        }

        PCI_PRINT_LOG(PCI_LOG_INFO, "show device info:");

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        oal_pci_cache_inv(pst_pci_dev, (oal_void*)pst_pcie_res->st_device_stat_map.pa, sizeof(pst_pcie_res->st_device_stat));
#endif
        oal_pcie_memcopy((oal_ulong)&pst_pcie_res->st_device_stat, (oal_ulong)pst_pcie_res->st_device_stat_map.va, sizeof(pst_pcie_res->st_device_stat));

        PCI_PRINT_LOG(PCI_LOG_INFO, "d2h fifo_full:%u, fifo_notfull:%u ringbuf_hit:%u, ringbuf_miss:%u\n",
                                                                      pst_pcie_res->st_device_stat.d2h_stats.stat.fifo_full, pst_pcie_res->st_device_stat.d2h_stats.stat.fifo_notfull,
                                                                      pst_pcie_res->st_device_stat.d2h_stats.stat.ringbuf_hit, pst_pcie_res->st_device_stat.d2h_stats.stat.ringbuf_miss);
        PCI_PRINT_LOG(PCI_LOG_INFO, "d2h dma_busy:%u, dma_idle:%u fifo_ele_empty:%u doorbell count:%u\n",
                                                    pst_pcie_res->st_device_stat.d2h_stats.stat.fifo_dma_busy, pst_pcie_res->st_device_stat.d2h_stats.stat.fifo_dma_idle,
                                                    pst_pcie_res->st_device_stat.d2h_stats.stat.fifo_ele_empty, pst_pcie_res->st_device_stat.d2h_stats.stat.doorbell_isr_count);

        PCI_PRINT_LOG(PCI_LOG_INFO, "d2h push_fifo_count:%u done_isr_count:%u dma_work_list_stat:%u dma_free_list_stat:%u dma_pending_list_stat:%u" ,
                                                    pst_pcie_res->st_device_stat.d2h_stats.stat.push_fifo_count,
                                                    pst_pcie_res->st_device_stat.d2h_stats.stat.done_isr_count,
                                                    pst_pcie_res->st_device_stat.d2h_stats.stat.dma_work_list_stat,
                                                    pst_pcie_res->st_device_stat.d2h_stats.stat.dma_free_list_stat,
                                                    pst_pcie_res->st_device_stat.d2h_stats.stat.dma_pending_list_stat);


        PCI_PRINT_LOG(PCI_LOG_INFO, "h2d fifo_full:%u, fifo_notfull:%u ringbuf_hit:%u, ringbuf_miss:%u fifo_ele_empty:%u\n",
                                        pst_pcie_res->st_device_stat.h2d_stats.stat.fifo_full, pst_pcie_res->st_device_stat.h2d_stats.stat.fifo_notfull,
                                        pst_pcie_res->st_device_stat.h2d_stats.stat.ringbuf_hit, pst_pcie_res->st_device_stat.h2d_stats.stat.ringbuf_miss,
                                        pst_pcie_res->st_device_stat.h2d_stats.stat.fifo_ele_empty);

        PCI_PRINT_LOG(PCI_LOG_INFO, "h2d push_fifo_count:%u done_isr_count:%u dma_work_list_stat:%u dma_free_list_stat:%u dma_pending_list_stat:%u" ,
                                                    pst_pcie_res->st_device_stat.h2d_stats.stat.push_fifo_count,
                                                    pst_pcie_res->st_device_stat.h2d_stats.stat.done_isr_count,
                                                    pst_pcie_res->st_device_stat.h2d_stats.stat.dma_work_list_stat,
                                                    pst_pcie_res->st_device_stat.h2d_stats.stat.dma_free_list_stat,
                                                    pst_pcie_res->st_device_stat.h2d_stats.stat.dma_pending_list_stat);

        PCI_PRINT_LOG(PCI_LOG_INFO, "comm_stat l1_wake_force_push_cnt:%u l1_wake_l1_hit:%u l1_wake_l1_miss:%u l1_wake_state_err_cnt:%u l1_wake_timeout_cnt:%u l1_wake_timeout_max_cnt:%u" ,
                                                    pst_pcie_res->st_device_stat.comm_stat.l1_wake_force_push_cnt,
                                                    pst_pcie_res->st_device_stat.comm_stat.l1_wake_l1_hit,
                                                    pst_pcie_res->st_device_stat.comm_stat.l1_wake_l1_miss,
                                                    pst_pcie_res->st_device_stat.comm_stat.l1_wake_state_err_cnt,
                                                    pst_pcie_res->st_device_stat.comm_stat.l1_wake_timeout_cnt,
                                                    pst_pcie_res->st_device_stat.comm_stat.l1_wake_timeout_max_cnt);
        if(pst_pcie_res->st_device_stat.comm_stat.l1_wake_force_push_cnt)
        {
            DECLARE_DFT_TRACE_KEY_INFO("l1_wake_force_push_error", OAL_DFT_TRACE_FAIL);
        }
        if(pst_pcie_res->st_device_stat.comm_stat.l1_wake_state_err_cnt)
        {
            DECLARE_DFT_TRACE_KEY_INFO("l1_wake_state_err", OAL_DFT_TRACE_FAIL);
        }
    }
    else
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "st_device_stat_map null:va:%lu, pa:%lu\n", pst_pcie_res->st_device_stat_map.va, pst_pcie_res->st_device_stat_map.pa);
    }

    return OAL_SUCC;
}

oal_int32 oal_pcie_print_all_ringbuf_info(oal_pcie_res* pst_pci_res)
{
    oal_int32 i;
    oal_pcie_print_ringbuf_info(&pst_pci_res->st_ringbuf.st_d2h_buf, PCI_LOG_INFO);
    for(i = 0; i < PCIE_H2D_QTYPE_BUTT; i++)
    {
        oal_pcie_print_ringbuf_info(&pst_pci_res->st_ringbuf.st_h2d_buf[i], PCI_LOG_INFO);
    }
    oal_pcie_print_ringbuf_info(&pst_pci_res->st_ringbuf.st_d2h_msg, PCI_LOG_INFO);
    oal_pcie_print_ringbuf_info(&pst_pci_res->st_ringbuf.st_h2d_msg, PCI_LOG_INFO);

    /*soft ringbuf*/
    return OAL_SUCC;
}

oal_void oal_pcie_print_transfer_info(oal_pcie_res* pst_pci_res, oal_uint64 print_flag)
{
    oal_int32 i = 0;
    oal_int32 j = 0;
    oal_uint32 len = 0;
    oal_uint32 total_len = 0;

    if(NULL == pst_pci_res)
        return;

    PCI_PRINT_LOG(PCI_LOG_INFO, "pcie transfer info:");
    if(pst_pci_res->stat.intx_total_count)
        PCI_PRINT_LOG(PCI_LOG_INFO, "intx_total_count:%u",       pst_pci_res->stat.intx_total_count);
    if(pst_pci_res->stat.intx_tx_count)
        PCI_PRINT_LOG(PCI_LOG_INFO, "intx_tx_count:%u",          pst_pci_res->stat.intx_tx_count);
    if(pst_pci_res->stat.intx_rx_count)
        PCI_PRINT_LOG(PCI_LOG_INFO, "intx_rx_count:%u",          pst_pci_res->stat.intx_rx_count);
    if(pst_pci_res->stat.done_err_cnt)
        PCI_PRINT_LOG(PCI_LOG_INFO, "done_err_cnt:%u",           pst_pci_res->stat.done_err_cnt);
    if(pst_pci_res->stat.h2d_doorbell_cnt)
        PCI_PRINT_LOG(PCI_LOG_INFO, "h2d_doorbell_cnt:%u",       pst_pci_res->stat.h2d_doorbell_cnt);
    if(pst_pci_res->stat.d2h_doorbell_cnt)
        PCI_PRINT_LOG(PCI_LOG_INFO, "d2h_doorbell_cnt:%u",       pst_pci_res->stat.d2h_doorbell_cnt);
    if(pst_pci_res->st_rx_res.stat.rx_count)
        PCI_PRINT_LOG(PCI_LOG_INFO, "rx_count:%u",               pst_pci_res->st_rx_res.stat.rx_count);
    if(pst_pci_res->st_rx_res.stat.rx_done_count)
        PCI_PRINT_LOG(PCI_LOG_INFO, "rx_done_count:%u",          pst_pci_res->st_rx_res.stat.rx_done_count);
    if(pst_pci_res->st_rx_res.stat.alloc_netbuf_failed)
        PCI_PRINT_LOG(PCI_LOG_INFO, "alloc_netbuf_failed:%u",    pst_pci_res->st_rx_res.stat.alloc_netbuf_failed);
    if(pst_pci_res->st_rx_res.stat.map_netbuf_failed)
        PCI_PRINT_LOG(PCI_LOG_INFO, "map_netbuf_failed:%u",      pst_pci_res->st_rx_res.stat.map_netbuf_failed);

    /*tx info*/
    for(i = 0 ; i < PCIE_H2D_QTYPE_BUTT; i++)
    {
        if(pst_pci_res->st_tx_res[i].stat.tx_count)
            PCI_PRINT_LOG(PCI_LOG_INFO, "[qid:%d]tx_count:%u", i, pst_pci_res->st_tx_res[i].stat.tx_count);
        if(pst_pci_res->st_tx_res[i].stat.tx_done_count)
            PCI_PRINT_LOG(PCI_LOG_INFO, "[qid:%d]tx_done_count:%u", i, pst_pci_res->st_tx_res[i].stat.tx_done_count);
        len = oal_netbuf_list_len(&pst_pci_res->st_tx_res[i].txq);
        if(len)
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "[qid:%d]len=%d", i, len);
            total_len += len;
        }

        PCI_PRINT_LOG(PCI_LOG_INFO,"[qid:%d]tx ringbuf cond is %d", i, oal_atomic_read(&pst_pci_res->st_tx_res[i].tx_ringbuf_sync_cond));
    }

    /*burst info*/
    for(i = 0; i < PCIE_EDMA_WRITE_BUSRT_COUNT + 1; i++)
    {
        if(pst_pci_res->st_rx_res.stat.rx_burst_cnt[i])
            PCI_PRINT_LOG(PCI_LOG_INFO, "rx burst %d count:%u", i, pst_pci_res->st_rx_res.stat.rx_burst_cnt[i]);
    }

    for(i = 0 ; i < PCIE_H2D_QTYPE_BUTT; i++)
    {
        for(j = 0; j < PCIE_EDMA_READ_BUSRT_COUNT + 1; j++)
        {
            if(pst_pci_res->st_tx_res[i].stat.tx_burst_cnt[j])
                PCI_PRINT_LOG(PCI_LOG_INFO, "tx qid %d burst %d count:%u", i, j, pst_pci_res->st_tx_res[i].stat.tx_burst_cnt[j]);
        }
    }

    oal_pcie_print_all_ringbuf_info(pst_pci_res);

    /*dump pcie hardware info*/
    if(OAL_UNLIKELY(pst_pci_res->link_state >= PCI_WLAN_LINK_WORK_UP))
    {
        if(board_get_host_wakeup_dev_stat() == 1)
        {
            /*gpio is high axi is alive*/
            if(print_flag & HCC_PRINT_TRANS_FLAG_DEVICE_REGS)
            {
                oal_pcie_print_pcie_regs(pst_pci_res, PCIE_CTRL_BASE_ADDR, 0x4c8 + 0x4);
                oal_pcie_print_pcie_regs(pst_pci_res, PCIE_DMA_CTRL_BASE_ADDR, 0x30 + 0x4);
            }

            if(print_flag & HCC_PRINT_TRANS_FLAG_DEVICE_STAT)
            {
                /*dump pcie status*/
                oal_pcie_print_device_transfer_info(pst_pci_res);
            }
        }
    }
    else
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "pcie is %s", g_pcie_link_state_str[pst_pci_res->link_state]);
    }

}

oal_void oal_pcie_reset_transfer_info(oal_pcie_res* pst_pci_res)
{
    oal_int32 i = 0;

    if(NULL == pst_pci_res)
        return;

    PCI_PRINT_LOG(PCI_LOG_INFO, "reset transfer info");
    pst_pci_res->stat.intx_total_count = 0;
    pst_pci_res->stat.intx_tx_count = 0;
    pst_pci_res->stat.intx_rx_count = 0;
    pst_pci_res->stat.h2d_doorbell_cnt = 0;
    pst_pci_res->stat.d2h_doorbell_cnt = 0;
    pst_pci_res->st_rx_res.stat.rx_count = 0;
    pst_pci_res->st_rx_res.stat.rx_done_count = 0;

    for(i = 0 ; i < PCIE_H2D_QTYPE_BUTT; i++)
    {
        pst_pci_res->st_tx_res[i].stat.tx_count = 0;
        pst_pci_res->st_tx_res[i].stat.tx_done_count = 0;
        oal_memset((oal_void*)pst_pci_res->st_tx_res[i].stat.tx_burst_cnt, 0, sizeof(pst_pci_res->st_tx_res[i].stat.tx_burst_cnt));
    }

    oal_memset((oal_void*)pst_pci_res->st_rx_res.stat.rx_burst_cnt, 0, sizeof(pst_pci_res->st_rx_res.stat.rx_burst_cnt));
}

oal_int32 oal_pcie_host_pending_signal_check(oal_pcie_res* pst_pci_res)
{
    oal_int32 i = 0;
    for(i = 0 ; i < PCIE_H2D_QTYPE_BUTT; i++)
    {
        if(oal_atomic_read(&pst_pci_res->st_tx_res[i].tx_ringbuf_sync_cond))
        {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}

oal_int32 oal_pcie_host_pending_signal_process(oal_pcie_res* pst_pci_res)
{
    oal_int32 i = 0;
    oal_int32 total_cnt = 0;
#ifdef CONFIG_ARCH_SD56XX
    /*5610 non pm*/
#else
    oal_pcie_linux_res * pst_pci_lres;
    oal_pci_dev_stru *pst_pci_dev;
    pst_pci_dev = PCIE_RES_TO_DEV(pst_pci_res);
    pst_pci_lres = (oal_pcie_linux_res*)oal_pci_get_drvdata(pst_pci_dev);
    if(OAL_UNLIKELY(OAL_SUCC != hcc_bus_pm_wakeup_device(pst_pci_lres->pst_bus)))
    {
        oal_msleep(100);/*wait for a while retry*/
        for(i = 0 ; i < PCIE_H2D_QTYPE_BUTT; i++)
        {
            oal_atomic_set(&pst_pci_res->st_tx_res[i].tx_ringbuf_sync_cond, 0);
        }
        return total_cnt;
    }
#endif
    for(i = 0 ; i < PCIE_H2D_QTYPE_BUTT; i++)
    {
        if(oal_atomic_read(&pst_pci_res->st_tx_res[i].tx_ringbuf_sync_cond))
        {
            oal_atomic_set(&pst_pci_res->st_tx_res[i].tx_ringbuf_sync_cond, 0);
            if(OAL_SUCC == oal_pcie_h2d_ringbuf_rd_update(pst_pci_res, (PCIE_H2D_RINGBUF_QTYPE)i))
            {
                total_cnt++;
            }
        }
    }

    return total_cnt;

}

/*Debug functions*/
oal_int32 oal_pcie_dump_all_regions_mem(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    oal_int32 index;
    oal_uint32 region_num;
    oal_pcie_region* region_base;
    oal_pci_dev_stru *pst_pci_dev ;

    /*1181 test , get pci cfg memory*/
    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "pci not init!\n");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        return -OAL_ENODEV;
    }

    if(OAL_WARN_ON(!pst_pcie_res->regions.inited))
    {
        return -OAL_EBUSY;
    }

    region_num  = pst_pcie_res->regions.region_nums;
    region_base = pst_pcie_res->regions.pst_regions;


    for(index = 0; index < region_num; index++, region_base++)
    {
        if(NULL == region_base->vaddr)
            continue;
        if(NULL != region_base->vaddr)
        {
            oal_uint32 size = region_base->size > 256 ? 256:region_base->size;
            OAL_IO_PRINT("dump region[%d],name:%s, cpu addr:0x%llx\n", index, region_base->name, region_base->cpu_start);
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
            oal_pci_cache_inv(pst_pci_dev, (oal_void*)(oal_ulong)region_base->paddr, size);
#endif
            oal_print_hex_dump((oal_uint8 *) region_base->vaddr,  size, 64, "pci mem  ");
        }

    }

    return OAL_SUCC;
}

oal_int32 oal_pcie_dump_all_regions(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pci not init!\n");
        return -OAL_EBUSY;
    }

    oal_pcie_regions_info_dump(pst_pcie_res);
    return OAL_SUCC;
}

oal_int32 oal_pcie_debug_read32(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    /*echo read32 address > debug*/
    oal_int32 ret;
    pci_addr_map addr_map;
    oal_uint32 cpu_address = 0xFFFFFFFF;
    oal_uint32 value;
    oal_pci_dev_stru *pst_pci_dev;

    if ((sscanf(buf, "0x%x", &cpu_address) != 1))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "read32 argument invalid,[%s]", buf);
        return -OAL_EINVAL;
    }

    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpu_address, &addr_map);
    if(OAL_SUCC != ret)
    {
        OAL_IO_PRINT("pcie_read32 0x%8x unmap, read failed!\n", cpu_address);
        return -OAL_EBUSY;
    }
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)addr_map.pa, sizeof(value));
#endif
    value = oal_readl((void*)addr_map.va);
    oal_pcie_print_bits(&value, sizeof(oal_uint32));
    return OAL_SUCC;
}

oal_int32 oal_pcie_debug_write32(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    /*echo write32 address value > debug*/
    oal_int32 ret;
    pci_addr_map addr_map;
    oal_uint32 cpu_address = 0xFFFFFFFF;
    oal_uint32 value, old;
    oal_pci_dev_stru *pst_pci_dev;
    if ((sscanf(buf, "0x%x 0x%x", &cpu_address, &value) != 2))
    {
        return -OAL_EINVAL;
    }

    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpu_address, &addr_map);
    if(OAL_SUCC != ret)
    {
        OAL_IO_PRINT("pcie_write32 0x%8x unmap, read failed!\n", cpu_address);
        return -OAL_ENODEV;
    }
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)addr_map.pa, sizeof(value));
#endif
    old = oal_readl((void*)addr_map.va);
    oal_writel(value, (void*)addr_map.va);
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_flush(pst_pci_dev, (oal_void*)addr_map.pa, sizeof(value));
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)addr_map.pa, sizeof(value));
#endif
    OAL_IO_PRINT("pcie_write32  change 0x%8x from 0x%8x to 0x%8x callback-read= 0x%8x\n", cpu_address, old, value, oal_readl((void*)addr_map.va));
    return OAL_SUCC;
}

oal_int32 oal_pcie_debug_read_dsm32(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    oal_int32 ret;
    oal_uint32 val, type;
    if ((sscanf(buf, "%u", &type) != 1))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "host read_dsm32 argument invalid,[%s]", buf);
        return -OAL_EINVAL;
    }

    ret = oal_pcie_read_dsm32(pst_pcie_res, type, &val);
    if(OAL_SUCC == ret)
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "read_dsm32 type=%u, val=%u", type, val);
    }

    return ret;
}

oal_int32 oal_pcie_debug_write_dsm32(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    oal_int32 ret;
    oal_uint32 val, type;
    if ((sscanf(buf, "%u %u", &type, &val) != 2))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "host write_dsm32 argument invalid,[%s]", buf);
        return -OAL_EINVAL;
    }

    ret = oal_pcie_write_dsm32(pst_pcie_res, type, val);
    if(OAL_SUCC == ret)
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "write_dsm32 type=%u, val=%u", type, val);
    }

    return ret;
}

oal_int32 oal_pcie_debug_host_read32(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    /*echo host_read32 address > debug*/
    oal_ulong cpu_address, align_addr;
    oal_void * pst_vaddr;
    oal_int32 value;

    if ((sscanf(buf, "0x%lx", &cpu_address) != 1))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "host read32 argument invalid,[%s]", buf);
        return -OAL_EINVAL;
    }

    align_addr = OAL_ROUND_DOWN(cpu_address, PAGE_SIZE);

    pst_vaddr = oal_ioremap_nocache(align_addr, PAGE_SIZE);
    if(OAL_SUCC == pst_vaddr)
    {
        OAL_IO_PRINT("pcie_host_read32 0x%lx map failed!\n", align_addr);
        return -OAL_EBUSY;
    }

    value = oal_readl(pst_vaddr + (cpu_address - align_addr));
    oal_pcie_print_bits(&value, sizeof(oal_uint32));

    oal_iounmap(pst_vaddr);
    return OAL_SUCC;
}

oal_int32 oal_pcie_debug_host_write32(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    /*echo host_write32 address value > debug*/
    oal_ulong cpu_address, align_addr;
    oal_void * pst_vaddr, *pst_align_addr;
    oal_uint32 old, value;

    if ((sscanf(buf, "0x%lx 0x%x", &cpu_address, &value) != 2))
    {
        return -OAL_EINVAL;
    }

    align_addr = OAL_ROUND_DOWN(cpu_address, PAGE_SIZE);

    pst_vaddr = oal_ioremap_nocache(align_addr, PAGE_SIZE);
    if(OAL_SUCC == pst_vaddr)
    {
        OAL_IO_PRINT("pcie_host_write32 0x%lx map failed!\n", align_addr);
        return -OAL_EBUSY;
    }

    pst_align_addr = pst_vaddr + (cpu_address - align_addr);

    old = oal_readl((void*)pst_align_addr);
    oal_writel(value, (void*)pst_align_addr);
    OAL_IO_PRINT("host_write32  change 0x%lx from 0x%8x to 0x%8x callback-read= 0x%8x\n", cpu_address, old, value, oal_readl((void*)pst_align_addr));
    oal_iounmap(pst_vaddr);
    return OAL_SUCC;
}

oal_int32 oal_pcie_debug_read16(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    /*echo read32 address > debug*/
    oal_int32 ret;
    pci_addr_map addr_map;
    oal_uint32 cpu_address = 0xFFFFFFFF;
    oal_uint32 value;
    oal_pci_dev_stru *pst_pci_dev;

    if ((sscanf(buf, "0x%x", &cpu_address) != 1))
    {
        return -OAL_EINVAL;
    }

    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpu_address, &addr_map);
    if(OAL_SUCC != ret)
    {
        OAL_IO_PRINT("pcie_read16 0x%8x unmap, read failed!\n", cpu_address);
        return -OAL_ENODEV;
    }
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)addr_map.pa, sizeof(value));
#endif
    value = readw((void*)addr_map.va);
    oal_pcie_print_bits(&value, sizeof(oal_uint16));
    return OAL_SUCC;
}

oal_int32 oal_pcie_debug_write16(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    /*echo write32 address value > debug*/
    oal_int32 ret;
    pci_addr_map addr_map;
    oal_uint32 cpu_address = 0xFFFFFFFF;
    oal_uint32 value, old;
    oal_pci_dev_stru *pst_pci_dev;
    if ((sscanf(buf, "0x%x 0x%x", &cpu_address, &value) != 2))
    {
        return -OAL_EINVAL;
    }

    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpu_address, &addr_map);
    if(OAL_SUCC != ret)
    {
        OAL_IO_PRINT("pcie_write16 0x%8x unmap, read failed!\n", cpu_address);
        return -OAL_ENODEV;
    }
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)addr_map.pa, sizeof(value));
#endif
    old = readw((void*)addr_map.va);
    writew((oal_uint16)value, (void*)addr_map.va);
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_flush(pst_pci_dev, (oal_void*)addr_map.pa, sizeof(value));
    oal_pci_cache_inv(pst_pci_dev, (oal_void*)addr_map.pa, sizeof(value));
#endif
    OAL_IO_PRINT("pcie_write16  change 0x%8x from 0x%4x to 0x%4x callback-read= 0x%4x\n", cpu_address, old, (oal_uint16)value, readw((void*)addr_map.va));
    return OAL_SUCC;
}

oal_int32 oal_pcie_saveconfigmem(oal_pcie_res* pst_pcie_res, char* file_name, oal_uint32 cpu_address, oal_uint32 length)
{
    struct file *fp;
    oal_uint32 index;
    oal_int32 ret = OAL_SUCC;
    mm_segment_t fs;
    oal_void* pst_buf;

    oal_pci_dev_stru *pst_pci_dev;

    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }

    length = PADDING(length, 4);
    //file_name[sizeof(file_name) - 1] = '\0';

    pst_buf = vmalloc(length);
    if(NULL == pst_buf)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "savemem pst_buf is null, vmalloc size %u failed!", length);
        return -OAL_ENOMEM;
    }

    fp = filp_open(file_name, O_RDWR | O_CREAT, 0664);
    if (OAL_IS_ERR_OR_NULL(fp))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "create file error,fp = 0x%p, filename is [%s]\n", fp, file_name);
        vfree(pst_buf);
        return -OAL_EINVAL;
    }

    //PCI_PRINT_LOG(PCI_LOG_INFO, "savemem cpu:0x%8x len:%u save_path:%s", cpu_address, length, file_name);

    fs = get_fs();
    set_fs(KERNEL_DS);

    OAL_REFERENCE(ret);

    for(index = 0; index < length; index += 4)
    {
        oal_uint32 reg = 0;
        ret = oal_pci_read_config_dword(pst_pci_dev, cpu_address + index, &reg);
        if(ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "read 0x%x failed, ret=%d", cpu_address + index, ret);
            break;
        }

        oal_writel(reg, pst_buf + index);
    }

    if(index)
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "savemem cpu:0x%8x len:%u save_path:%s done", cpu_address, index, file_name);
        ret = vfs_write(fp, pst_buf, length, &fp->f_pos);
        if(ret < 0)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "vfs write failed!");
        }
    }else{
        PCI_PRINT_LOG(PCI_LOG_WARN, "savemem cpu:0x%8x len:%u save_path:%s failed!", cpu_address, length, file_name);
        ret = -OAL_EINVAL;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
    vfs_fsync(fp, 0);
#else
    vfs_fsync(fp, fp->f_path.dentry, 0);
#endif

    set_fs(fs);
    filp_close(fp, NULL);
    vfree(pst_buf);
    return ret;
}

oal_int32 oal_pcie_savemem(oal_pcie_res* pst_pcie_res, char* file_name, oal_uint32 cpu_address, oal_uint32 length)
{
    struct file *fp;
    oal_uint32 index, value;
    oal_int32 ret = OAL_SUCC;
    pci_addr_map addr_map;
    mm_segment_t fs;
    oal_void* pst_buf;

    oal_pci_dev_stru *pst_pci_dev;

    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }

    length = PADDING(length, 4);
    //file_name[sizeof(file_name) - 1] = '\0';

    pst_buf = vmalloc(length);
    if(NULL == pst_buf)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "savemem pst_buf is null, vmalloc size %u failed!", length);
        return -OAL_ENOMEM;
    }

    fp = filp_open(file_name, O_RDWR | O_CREAT, 0664);
    if (OAL_IS_ERR_OR_NULL(fp))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "create file error,fp = 0x%p, filename is [%s]\n", fp, file_name);
        vfree(pst_buf);
        return -OAL_EINVAL;
    }

    //PCI_PRINT_LOG(PCI_LOG_INFO, "savemem cpu:0x%8x len:%u save_path:%s", cpu_address, length, file_name);

    fs = get_fs();
    set_fs(KERNEL_DS);

    for(index = 0; index < length; index += 4)
    {
        ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpu_address + index, &addr_map);
        if(OAL_SUCC != ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "savemem address 0x%8x invalid", cpu_address + index);
            break;
        }
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        oal_pci_cache_inv(pst_pci_dev, (oal_void*)addr_map.pa, sizeof(value));
#endif
        /*这里有可能保存的是寄存器区域，按4字节对齐访问*/
        value = oal_readl((void*)addr_map.va);
        oal_writel(value, pst_buf + index);
    }

    if(index)
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "savemem cpu:0x%8x len:%u save_path:%s done", cpu_address, index, file_name);
        ret = vfs_write(fp, pst_buf, length, &fp->f_pos);
        if(ret < 0)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "vfs write failed!");
        }
    }else{
        PCI_PRINT_LOG(PCI_LOG_WARN, "savemem cpu:0x%8x len:%u save_path:%s failed!", cpu_address, length, file_name);
        ret = -OAL_EINVAL;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
    vfs_fsync(fp, 0);
#else
    vfs_fsync(fp, fp->f_path.dentry, 0);
#endif

    set_fs(fs);
    filp_close(fp, NULL);
    vfree(pst_buf);
    return ret;
}

oal_int32 oal_pcie_save_hostmem(oal_pcie_res* pst_pcie_res, char* file_name, oal_ulong host_address, oal_uint32 length)
{
    struct file *fp;
    oal_uint32 index, value;
    oal_int32 ret = OAL_SUCC;
    mm_segment_t fs;
    oal_void* pst_buf;
    oal_void* vaddr;

    oal_pci_dev_stru *pst_pci_dev;

    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }

    length = PADDING(length, 4);

    vaddr = oal_ioremap_nocache(host_address, length);
    if(NULL == vaddr)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "ioremap %lx , len:%u ,fail", host_address, length);
        return -OAL_ENOMEM;
    }

    //file_name[sizeof(file_name) - 1] = '\0';

    pst_buf = vmalloc(length);
    if(NULL == pst_buf)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "savemem pst_buf is null, vmalloc size %u failed!", length);
        return -OAL_ENOMEM;
    }

    fp = filp_open(file_name, O_RDWR | O_CREAT, 0664);
    if (OAL_IS_ERR_OR_NULL(fp))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "create file error,fp = 0x%p, filename is [%s]\n", fp, file_name);
        vfree(pst_buf);
        oal_iounmap(vaddr);
        return -OAL_EINVAL;
    }

    //PCI_PRINT_LOG(PCI_LOG_INFO, "savemem cpu:0x%8x len:%u save_path:%s", cpu_address, length, file_name);

    fs = get_fs();
    set_fs(KERNEL_DS);

    for(index = 0; index < length; index += 4)
    {
        /*这里有可能保存的是寄存器区域，按4字节对齐访问*/
        value = oal_readl(vaddr + index);
        oal_writel(value, pst_buf + index);
    }

    if(index)
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "save_hostmem cpu:0x%lx len:%u save_path:%s done", host_address, index, file_name);
        ret = vfs_write(fp, pst_buf, length, &fp->f_pos);
        if(ret < 0)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "vfs write failed!");
        }
    }else{
        PCI_PRINT_LOG(PCI_LOG_WARN, "save_hostmem cpu:0x%lx len:%u save_path:%s failed!", host_address, length, file_name);
        ret = -OAL_EINVAL;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
    vfs_fsync(fp, 0);
#else
    vfs_fsync(fp, fp->f_path.dentry, 0);
#endif

    set_fs(fs);
    filp_close(fp, NULL);
    vfree(pst_buf);
    oal_iounmap(vaddr);
    return ret;
}

oal_int32 oal_pcie_debug_savemem(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    char file_name[100];
    oal_uint32 cpu_address, length;
    oal_int32 ret = OAL_SUCC;

    if (strlen(buf) >= OAL_SIZEOF(file_name))
    {
        OAL_IO_PRINT("input illegal!%s\n",__FUNCTION__);
        return -OAL_EINVAL;
    }

    if ((sscanf(buf, "0x%x %u %s", &cpu_address, &length, file_name) != 3))
    {
        return -OAL_EINVAL;
    }

    ret = oal_pcie_savemem(pst_pcie_res, file_name, cpu_address, length);
    return ret;
}

oal_int32 oal_pcie_debug_saveconfigmem(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    char file_name[100];
    oal_uint32 cpu_address, length;
    oal_int32 ret = OAL_SUCC;

    if (strlen(buf) >= OAL_SIZEOF(file_name))
    {
        OAL_IO_PRINT("input illegal!%s\n",__FUNCTION__);
        return -OAL_EINVAL;
    }

    if ((sscanf(buf, "0x%x %u %s", &cpu_address, &length, file_name) != 3))
    {
        return -OAL_EINVAL;
    }

    ret = oal_pcie_saveconfigmem(pst_pcie_res, file_name, cpu_address, length);
    return ret;
}

oal_int32 oal_pcie_debug_save_hostmem(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    char file_name[100];
    oal_ulong host_address;
    oal_uint32 length;
    oal_int32 ret = OAL_SUCC;

    if (strlen(buf) >= OAL_SIZEOF(file_name))
    {
        OAL_IO_PRINT("input illegal!%s\n",__FUNCTION__);
        return -OAL_EINVAL;
    }

    if ((sscanf(buf, "0x%lx %u %s", &host_address, &length, file_name) != 3))
    {
        return -OAL_EINVAL;
    }

    ret = oal_pcie_save_hostmem(pst_pcie_res, file_name, host_address, length);
    return ret;
}

oal_int32 oal_pcie_debug_readmem(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    /*echo readmem address length(hex) > debug*/
    oal_uint32 cpu_address, length, index;
    oal_int32 ret;
    pci_addr_map addr_map;
    oal_void *print_buf;
    oal_pci_dev_stru *pst_pci_dev;

    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }

    if ((sscanf(buf, "0x%x %u", &cpu_address, &length) != 2))
    {
        return -OAL_EINVAL;
    }

    length = PADDING(length, 4);
    print_buf = vmalloc(length);
    if(NULL == print_buf)
    {
        return -OAL_EINVAL;
    }

    for(index = 0; index < length; index += 4)
    {
        ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpu_address + index, &addr_map);
        if(OAL_SUCC != ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "readmem address 0x%8x invalid", cpu_address + index);
            break;
        }
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        oal_pci_cache_inv(pst_pci_dev, (oal_void*)addr_map.pa, sizeof(oal_uint32));
#endif
        *(oal_uint32*)(print_buf + index) = oal_readl((void*)addr_map.va);
    }

    if(index)
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "readmem cpu:0x%8x len:%u, va:0x%lx, pa:0x%lx  done", cpu_address, index, addr_map.va, addr_map.pa);
        oal_print_hex_dump(print_buf, length, 64, "readmem: ");
    }else{
        PCI_PRINT_LOG(PCI_LOG_WARN, "readmem cpu:0x%8x len:%u  failed!", cpu_address, length);
        vfree(print_buf);
        return -OAL_EINVAL;
    }

    vfree(print_buf);
    return OAL_SUCC;
}

oal_int32 oal_pcie_debug_readmem_config(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    /*echo readmem address length(hex) > debug*/
    oal_uint32 cpu_address, length, index;
    oal_int32 ret;
    oal_uint32 reg = 0;
    oal_void *print_buf;
    oal_pci_dev_stru *pst_pci_dev;

    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }

    if ((sscanf(buf, "0x%x %u", &cpu_address, &length) != 2))
    {
        return -OAL_EINVAL;
    }

    length = PADDING(length, 4);
    print_buf = vmalloc(length);
    if(NULL == print_buf)
    {
        return -OAL_EINVAL;
    }

    for(index = 0; index < length; index += 4)
    {
        ret = oal_pci_read_config_dword(pst_pci_dev, cpu_address + index, &reg);
        if(ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "read 0x%x failed, ret=%d", cpu_address + index, ret);
            break;
        }
        *(oal_uint32*)(print_buf + index) = reg;
    }

    if(index)
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "readmem cpu:0x%8x len:%u  done", cpu_address, index);
        oal_print_hex_dump(print_buf, length, 64, "readmem: ");
    }else{
        PCI_PRINT_LOG(PCI_LOG_WARN, "readmem cpu:0x%8x len:%u  failed!", cpu_address, length);
        vfree(print_buf);
        return -OAL_EINVAL;
    }

    vfree(print_buf);
    return OAL_SUCC;
}

oal_int32 oal_pcie_send_test_pkt(oal_int32 num)
{
    oal_uint32 cmd_len = 1500;
    oal_netbuf_stru*       pst_netbuf;
    struct hcc_transfer_param st_hcc_transfer_param = {0};
    struct hcc_handler* hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        return -OAL_EFAIL;
    }

    if(0 == num)
    {
        return -OAL_EINVAL;
    }

    do
    {
        pst_netbuf  = hcc_netbuf_alloc(cmd_len);
        if (NULL == pst_netbuf)
        {
            OAL_IO_PRINT("hwifi alloc skb fail.\n");
            return -OAL_EFAIL;
        }

        oal_memset(oal_netbuf_put(pst_netbuf,cmd_len),0x5a,cmd_len);

        hcc_hdr_param_init(&st_hcc_transfer_param,
                        HCC_ACTION_TYPE_TEST,
                        HCC_TEST_SUBTYPE_CMD,
                        0,
                        HCC_FC_WAIT,
                        0);
        if(OAL_SUCC != hcc_tx_etc(hcc, pst_netbuf, &st_hcc_transfer_param))
        {
            OAL_IO_PRINT("hcc tx failed\n");
            return -OAL_EFAIL;
        }
    }while(--num);


    return OAL_SUCC;
}

/*测试outbound是否生效，返回DDR地址，
  通过SSI或者WCPU 读写Device 侧PCIe Slave地址 查看DDR是否有改变,
  1103 Slave 空间为256M*/
oal_int32 oal_pcie_outbound_test(oal_pcie_res* pst_pcie_res, char*buf)
{
    OAL_STATIC dma_addr_t g_outbound_dma_addr = 0;
    oal_uint32 g_outbound_size = 4096;
    OAL_STATIC oal_uint*  g_outbound_vaddr = NULL;
    char casename[100] = {0};
    oal_pci_dev_stru *pst_pci_dev;

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }

    if (strlen(buf) >= OAL_SIZEOF(casename))
    {
        OAL_IO_PRINT("input illegal!%s\n",__FUNCTION__);
        return -OAL_EINVAL;
    }

    /*Just for debug*/
    if ((sscanf(buf, "%s", casename) != 1))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "invalid agrument");
        return -OAL_EINVAL;
    }
    else
    {
        /*dump*/
        if(g_outbound_vaddr)
        {
            oal_print_hex_dump((oal_uint8*)g_outbound_vaddr, g_outbound_size, 32, "outbound: ");
        }
    }
    PCI_PRINT_LOG(PCI_LOG_INFO, "outbound:[%s]", casename);
    if(!oal_strcmp(casename, "rebound"))
    {
        if(g_outbound_vaddr)
        {
            dma_free_coherent(&pst_pci_dev->dev, g_outbound_size, g_outbound_vaddr, g_outbound_dma_addr);
        }
        g_outbound_vaddr = dma_alloc_coherent(&pst_pci_dev->dev, g_outbound_size,
                    &g_outbound_dma_addr,
                    GFP_KERNEL);
        if(g_outbound_vaddr)
        {
            oal_void* inbound_addr = pst_pcie_res->st_iatu_bar.st_region.vaddr;
            /*set outbound*/
            OAL_IO_PRINT("host dma addr:0x%lx , vaddr: 0x%p\n", (oal_ulong)g_outbound_dma_addr, g_outbound_vaddr);
            if(PCIE_REVISION_4_70A == pst_pcie_res->revision)
            {
                oal_pci_write_config_dword(pst_pci_dev, 0x700 + 0x200, 0x1);            /* view index */
                oal_pci_write_config_dword(pst_pci_dev, 0x700 + 0x204, 0x0);            /* ctrl 1 */
                oal_pci_write_config_dword(pst_pci_dev, 0x700 + 0x208, 0x80000000);     /* ctrl 2 */
                oal_pci_write_config_dword(pst_pci_dev, 0x700 + 0x20c, 0x70000000);       /* base lower */
                oal_pci_write_config_dword(pst_pci_dev, 0x700 + 0x210, 0);              /* base upper */
                oal_pci_write_config_dword(pst_pci_dev, 0x700 + 0x214, 0x70000000+g_outbound_size -1);        /* limit */
                oal_pci_write_config_dword(pst_pci_dev, 0x700 + 0x218, (oal_uint32)g_outbound_dma_addr);     /* target lower */
                oal_pci_write_config_dword(pst_pci_dev, 0x700 + 0x21c, 0);                               /* target upper */
            }
            else if(PCIE_REVISION_5_00A == pst_pcie_res->revision)
            {
                IATU_REGION_CTRL_2_OFF ctr2;
                oal_uint32 index = 0;
                if(NULL == inbound_addr)
                {
                    PCI_PRINT_LOG(PCI_LOG_ERR, "inbound_addr is null");
                    return -OAL_ENODEV;
                }
                oal_writel(0x0, inbound_addr + HI_PCI_IATU_REGION_CTRL_1_OFF_OUTBOUND_I(HI_PCI_IATU_OUTBOUND_BASE_OFF(index)));
                ctr2.AsDword = 0;
                ctr2.bits.region_en = 1;
                ctr2.bits.bar_num = 0x0;
                oal_writel(ctr2.AsDword, inbound_addr + HI_PCI_IATU_REGION_CTRL_2_OFF_OUTBOUND_I(HI_PCI_IATU_OUTBOUND_BASE_OFF(index)));

                oal_writel(0x70000000, inbound_addr + HI_PCI_IATU_LWR_BASE_ADDR_OFF_OUTBOUND_I(HI_PCI_IATU_OUTBOUND_BASE_OFF(index)));
                oal_writel(0x0, inbound_addr + HI_PCI_IATU_UPPER_BASE_ADDR_OFF_OUTBOUND_I(HI_PCI_IATU_OUTBOUND_BASE_OFF(index)));
                oal_writel(0x70000000+g_outbound_size -1, inbound_addr + HI_PCI_IATU_LIMIT_ADDR_OFF_OUTBOUND_I(HI_PCI_IATU_OUTBOUND_BASE_OFF(index)));

                oal_writel((oal_uint32)g_outbound_dma_addr,  inbound_addr + HI_PCI_IATU_LWR_TARGET_ADDR_OFF_OUTBOUND_I(HI_PCI_IATU_OUTBOUND_BASE_OFF(index)));
                oal_writel((oal_uint32)(g_outbound_dma_addr >> 32) , inbound_addr + HI_PCI_IATU_UPPER_TARGET_ADDR_OFF_OUTBOUND_I(HI_PCI_IATU_OUTBOUND_BASE_OFF(index)));
            }
            else
            {
                PCI_PRINT_LOG(PCI_LOG_ERR, "unkown pcie ip revision :0x%x", pst_pcie_res->revision);
                return -OAL_ENODEV;
            }
        }
    }

    return OAL_SUCC;

}

oal_int32 oal_pcie_device_memcheck(oal_pcie_res* pst_pcie_res, oal_uint32 cpuaddr, oal_uint32 size, oal_uint32 data)
{
    volatile unsigned int data_rd;
    volatile unsigned int data_wt;
    unsigned int i = 0;
    unsigned int mode = 0;
    oal_int32 ret;
    pci_addr_map addr_map;
    oal_pci_dev_stru *pst_pci_dev;

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }


    if(cpuaddr & (0x3))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "invalid cpu address, must align to 4 bytes:%u", cpuaddr);
        return -OAL_EINVAL;
    }

    if(size & 0x3)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "invalid size, must align to 4 bytes:%u", size);
        return -OAL_EINVAL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpuaddr + size - 1, &addr_map);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "unmap device address 0x%x, size:%u", cpuaddr, size);
        return -OAL_EINVAL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpuaddr, &addr_map);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "unmap device address 0x%x", cpuaddr);
        return -OAL_EINVAL;
    }


    for(i = 0, mode = 0; i < (size); i += 4, mode++)
    {
        if (mode % 4 < 2)
        {
            data_wt = data;
        }
        else
        {
            data_wt = ~data;
        }

        oal_writel(data_wt, (void*)(addr_map.va + i));
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        oal_pci_cache_flush(pst_pci_dev, (oal_void*)addr_map.pa +i , sizeof(oal_uint32));
        oal_pci_cache_inv(pst_pci_dev, (oal_void*)addr_map.pa +i , sizeof(oal_uint32));
#endif
        data_rd = oal_readl((void*)(addr_map.va + i));
        if(data_rd != data_wt)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "bad address :0x%8x ,write: 0x%8x ,read: 0x%8x", cpuaddr + i, data_wt, data_rd);
            return -OAL_EFAIL;
        }
    }
    return OAL_SUCC;
}

typedef struct _memcheck_item_
{
    oal_uint32 address;/*device cpu address*/
    oal_uint32 size;/*device cpu address*/
}memcheck_item;

/*需要WCPU代码在bootloader 阶段*/
oal_int32 oal_pcie_device_memcheck_auto(oal_pcie_res* pst_pcie_res)
{
    oal_int32 ret, i;
    memcheck_item test_address[] =
    {
        {0x00004000, 544*1024},
        {0x20001c00, 409*1024},
        {0x60000000, 576*1024}
    };

    PCI_PRINT_LOG(PCI_LOG_INFO, "memcheck start...");
    for(i = 0; i < sizeof(test_address)/sizeof(test_address[0]); i++)
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "Test address: 0x%8x ,size: 0x%8x start.", test_address[i].address, test_address[i].size);
        ret = oal_pcie_device_memcheck(pst_pcie_res, test_address[i].address, test_address[i].size, 0xFFFFFFFF);
        if(OAL_SUCC != ret)
        {
           break;
        }

        ret = oal_pcie_device_memcheck(pst_pcie_res, test_address[i].address, test_address[i].size, 0x00000000);
        if(OAL_SUCC != ret)
        {
           break;
        }

        ret = oal_pcie_device_memcheck(pst_pcie_res, test_address[i].address, test_address[i].size, 0x5a5a5a5a);
        if(OAL_SUCC != ret)
        {
           break;
        }

        ret = oal_pcie_device_memcheck(pst_pcie_res, test_address[i].address, test_address[i].size, 0xa5a5a5a5);
        if(OAL_SUCC != ret)
        {
           break;
        }
        PCI_PRINT_LOG(PCI_LOG_INFO, "Test address: 0x%8x ,size: 0x%8x OK.", test_address[i].address, test_address[i].size);
    }

    return OAL_SUCC;
}

#ifdef CONFIG_ARCH_SD56XX
oal_void pcie_change_2G_init(oal_void)
{
    oal_void*    pst_5115_pci0;
    oal_void*    pst_5115_pcie_phy;
    oal_void*    pst_5115_sys_ctl;
    oal_uint32          ul_val = 0;
    pst_5115_pci0 = oal_ioremap_nocache(0x10A00000, 0x1000);
    pst_5115_pcie_phy = oal_ioremap_nocache(0x14880030,0x1000);
    pst_5115_sys_ctl = oal_ioremap_nocache(0x10100000, 0x1000);
    if (NULL == pst_5115_sys_ctl)
    {
        OAL_IO_PRINT("remap addr fail\n");
        return;
    }

    ul_val = oal_readl(pst_5115_sys_ctl + 0xBC);
    ul_val |= BIT21;
    oal_writel(ul_val, pst_5115_sys_ctl + 0xBC);

    ul_val = oal_readl(pst_5115_sys_ctl + 0xC0);
    ul_val |= BIT21;
    oal_writel(ul_val, pst_5115_sys_ctl + 0xC0);


    ul_val  = oal_readl(pst_5115_pci0 + 0xA0);
    ul_val &= (~0x7);
    ul_val |= BIT0;
    oal_writel(ul_val, pst_5115_pci0 + 0xA0);
    OAL_IO_PRINT("ul_val:0x%x\n",ul_val);

    ul_val = oal_readl(pst_5115_sys_ctl + 0xC0);
    ul_val &= (~BIT21);
    oal_writel(ul_val, pst_5115_sys_ctl + 0xC0);

    ul_val = oal_readl(pst_5115_sys_ctl + 0xBC);
    ul_val &= (~BIT21);
    oal_writel(ul_val, pst_5115_sys_ctl + 0xBC);

    oal_iounmap(pst_5115_sys_ctl);
    oal_iounmap(pst_5115_pcie_phy);
    oal_iounmap(pst_5115_pci0);
}

oal_void pcie_change_5G(oal_void)
{
    oal_void*    pst_5115_pci0;
    oal_void*    pst_5115_pcie_phy;
    oal_void*    pst_5115_sys_ctl;
    oal_uint32          ul_val = 0;
    oal_uint32          ul_loop;

    pst_5115_pci0 = oal_ioremap_nocache(0x10A00000, 0x1000);
    pst_5115_pcie_phy = oal_ioremap_nocache(0x14880030,0x1000);
    pst_5115_sys_ctl = oal_ioremap_nocache(0x10100000, 0x1000);
    if (NULL == pst_5115_pci0)
    {
        OAL_IO_PRINT("remap addr fail\n");
        return;
    }

    ul_val = oal_readl(pst_5115_sys_ctl + 0xBC);
    ul_val |= BIT21;
    oal_writel(ul_val, pst_5115_sys_ctl + 0xBC);

    ul_val = oal_readl(pst_5115_sys_ctl + 0xC0);
    ul_val |= BIT21;
    oal_writel(ul_val, pst_5115_sys_ctl + 0xC0);


    ul_val  = oal_readl(pst_5115_pci0 + 0xA0);
    ul_val &= (~0x7);
    ul_val |= BIT1;
    oal_writel(ul_val, pst_5115_pci0 + 0xA0);
    OAL_IO_PRINT("ul_val:0x%x\n",ul_val);

    ul_val  = oal_readl(pst_5115_pci0 + 0x80);
    ul_val |= BIT5;
    oal_writel(ul_val, pst_5115_pci0 + 0x80);
    OAL_IO_PRINT("ul_val:0x%x\n",ul_val);

    ul_val = oal_readl(pst_5115_sys_ctl + 0xC0);
    ul_val &= (~BIT21);
    oal_writel(ul_val, pst_5115_sys_ctl + 0xC0);

    ul_val = oal_readl(pst_5115_sys_ctl + 0xBC);
    ul_val &= (~BIT21);
    oal_writel(ul_val, pst_5115_sys_ctl + 0xBC);

#if 1
#define PCIE_CHANGE_LOOP    100
    for (ul_loop = 0;ul_loop < PCIE_CHANGE_LOOP; ul_loop++)
    {
        ul_val = oal_readl(pst_5115_sys_ctl + 0x38);
        if ((ul_val & 0x440000) == 0x440000)
        {
            break;
        }
        else
        {
            OAL_IO_PRINT("[%d], ul_val = 0x%x\r\n",ul_loop , ul_val);
        }
    }
#endif


    if(PCIE_CHANGE_LOOP == ul_loop)
    {
        OAL_IO_PRINT("0x38 change timeout\n");
        oal_pcie_print_bits(&ul_val, 4);
    }

    OAL_IO_PRINT("ul_val:0x%x\n", ul_val);
    ul_val = oal_readl(pst_5115_sys_ctl + 0x38);
    OAL_IO_PRINT("ul_val:0x%x\n", ul_val);

    if ((ul_val & BIT17) != 0)
    {
        OAL_IO_PRINT("change to 5G succ:0x%x\r\n",ul_val);
    }
    else
    {
        OAL_IO_PRINT("change to 5G fail:0x%x\r\n",ul_val);
        oal_pcie_print_bits(&ul_val, 4);
    }

    oal_iounmap(pst_5115_sys_ctl);
    oal_iounmap(pst_5115_pcie_phy);
    oal_iounmap(pst_5115_pci0);
}
extern oal_int32 oal_pci_card_detect(oal_void);
#endif

oal_int32 oal_pci_sm_state_monitor(oal_void)
{
#ifdef CONFIG_ARCH_SD56XX
#define TRY_MAX 2000000
    unsigned int val, reg;
    unsigned int retry_count = 0;
    unsigned int loop = 0;
    unsigned int link_up_stable_counter = 0;
    void* __iomem pcie_sys_base_virt   = NULL;
    OAL_STATIC oal_uint32 old = 0;
    /*等待建链*/
    pcie_sys_base_virt = ioremap_nocache(0x10100000, 0x1000);
    if(NULL == pcie_sys_base_virt)
    {
        return -OAL_ENOMEM;
    }

    OAL_IO_PRINT("oal_pci_sm_state_monitor start\n");

    for(;;)
    {

        val = readl(pcie_sys_base_virt + 0x38);
        reg = (val >> 18 ) & 0x3F;
        if(reg != old)
        {
            //if(reg == 0x11 || reg == 0x12)
            {
                OAL_IO_PRINT("state change from 0x%x to 0x%x \n", old, reg);
            }
            old = reg;
        }
        loop++;
        oal_schedule();
        if(loop%10000)
        {
            //oal_msleep(10);
        }
    }


    iounmap(pcie_sys_base_virt);
    return OAL_SUCC;
#else
    return -OAL_ENODEV;
#endif
}

extern int32 wlan_power_on_etc(void);
oal_int32 oal_pcie_debug_wlan_power_on(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    oal_pci_dev_stru *pst_pci_dev;

    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }
    wlan_power_on_etc();
    return OAL_SUCC;
}

#ifdef CONFIG_ARCH_KIRIN_PCIE
OAL_STATIC oal_int32 oal_pcie_resume_handler(void* data)
{
    OAL_REFERENCE(data);
    /*这里保证解复位EP控制器时efuse已经稳定*/
    board_host_wakeup_dev_set(1);
    /*TBD:TBC ,
      dev wakeup host 被复用成了panic消息，这里需要处理*/
    oal_msleep(25);/*这里要用GPIO 做ACK 延迟不可靠, MPW2 硬件唤醒15ms,软件6ms*/
    PCI_PRINT_LOG(PCI_LOG_DBG, "oal_pcie_resume_handler, pull up gpio");
    return 0;
}
#endif

oal_int32 oal_pcie_debug_testcase(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    /*echo readmem address length(hex) > debug*/
    oal_int32 ret;
    char casename[100] = {0};
    oal_pci_dev_stru *pst_pci_dev;

    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }

    if (strlen(buf) >= OAL_SIZEOF(casename))
    {
        OAL_IO_PRINT("input illegal!%s\n",__FUNCTION__);
        return -OAL_EINVAL;
    }

    /*Just for debug*/
    if ((sscanf(buf, "%s", casename) != 1))
    {
        return -OAL_EINVAL;
    }

    PCI_PRINT_LOG(PCI_LOG_INFO, "testcase:[%s]", casename);

    if(!oal_strcmp(casename, "transfer_res_init"))
    {
        ret = oal_pcie_transfer_res_init(pst_pcie_res);
        if(OAL_SUCC == ret)
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "oal_pcie_transfer_res_init:SUCC");
        } else {
            PCI_PRINT_LOG(PCI_LOG_ERR,  "oal_pcie_transfer_res_init:FAIL");
        }
    }

    if(!oal_strcmp(casename, "send_test_pkt"))
    {
        oal_pcie_send_test_pkt(1);
    }

    if(!oal_strcmp(casename, "host_sm_monitor"))
    {
        oal_pci_sm_state_monitor();
    }

    if(!oal_strcmp(casename, "send_test_pkt2"))
    {
        oal_pcie_send_test_pkt(2);
    }

    if(!oal_strcmp(casename, "send_test_pkt3"))
    {
        oal_pcie_send_test_pkt(3);
    }

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_PERFORMANCE
    if(!oal_strcmp(casename, "pcie_mips_show"))
    {
        oal_pcie_mips_show();
    }

    if(!oal_strcmp(casename, "pcie_mips_clear"))
    {
        oal_pcie_mips_clear();
    }
#endif

#ifdef CONFIG_ARCH_SD56XX
    if(!oal_strcmp(casename, "gen2_test"))
    {
        pcie_change_5G();
    }
    if(!oal_strcmp(casename, "wlan_poweron_2g"))
    {
        /*FPGA 默认切换到Gen 1*/
        OAL_STATIC oal_uint32 change_2g = 0;
        if(!change_2g)
        {
            pcie_change_2G_init();
            change_2g = 1;
        }
        oal_pci_card_detect();
    }

    if(!oal_strcmp(casename, "wlan_poweron"))
    {
        oal_pci_card_detect();
    }

    if(!oal_strcmp(casename, "pm_aspm_l0"))
    {
        oal_pcie_rc_mem_map();
        oal_pcie_link_l0s_entry_latency_set(pst_pci_dev,0x7);


//        oal_pcie_link_clkpm_set(pst_pci_dev,OAL_TRUE);

        oal_pcie_link_common_clk_set(pst_pci_dev);
        oal_pcie_link_aspm_enable(pst_pci_dev, PCIE_ASPM_CAP_L0S);
        oal_pcie_link_retrain();
        oal_pcie_rc_mem_unmap();
    }

    if(!oal_strcmp(casename, "pm_aspm_l1"))
    {
        oal_pcie_rc_mem_map();

        oal_pcie_link_l1_entry_latency_set(pst_pci_dev,PCIE_L1_ENT_64_US);

        oal_pcie_link_complete_timeout_set(pst_pci_dev,PCIE_COMP_TIMEOUT_50U_TO_100U);

       // oal_pcie_link_clkpm_set(pst_pci_dev,OAL_TRUE);

        oal_pcie_link_common_clk_set(pst_pci_dev);
        oal_pcie_link_aspm_enable(pst_pci_dev, PCIE_ASPM_CAP_L1);

        oal_pcie_link_retrain();

        oal_pcie_rc_mem_unmap();
    }

    if(!oal_strcmp(casename, "pm_aspm_l0_l1"))
    {
        oal_pcie_rc_mem_map();

        oal_pcie_link_clkpm_set(pst_pci_dev,OAL_TRUE);

        oal_pcie_link_common_clk_set(pst_pci_dev);

        oal_pcie_link_aspm_enable(pst_pci_dev, PCIE_ASPM_CAP_L0S_AND_L1);

        oal_pcie_link_retrain();
        oal_pcie_rc_mem_unmap();
    }

    if(!oal_strcmp(casename, "pm_aspm_l1_1"))
    {

        oal_pcie_rc_mem_map();

        oal_pcie_link_clkpm_set(pst_pci_dev,OAL_TRUE);

        oal_pcie_link_common_clk_set(pst_pci_dev);

        oal_pcie_l11_set(pst_pci_dev, OAL_TRUE);

        oal_pcie_link_aspm_enable(pst_pci_dev, PCIE_ASPM_CAP_L0S_AND_L1);

        oal_pcie_link_retrain();
        oal_pcie_rc_mem_unmap();
    }

    if(!oal_strcmp(casename, "pm_aspm_l1_2"))
    {

        oal_pcie_rc_mem_map();

        oal_pcie_link_clkpm_set(pst_pci_dev,OAL_TRUE);

        oal_pcie_link_common_clk_set(pst_pci_dev);

        oal_pcie_l12_set(pst_pci_dev, OAL_TRUE);

        oal_pcie_link_aspm_enable(pst_pci_dev, PCIE_ASPM_CAP_L0S_AND_L1);

        oal_pcie_link_retrain();
        oal_pcie_rc_mem_unmap();
    }

    if(!oal_strcmp(casename, "pm_aspm_l1ss"))
    {

        oal_pcie_rc_mem_map();

        oal_pcie_link_clkpm_set(pst_pci_dev,OAL_TRUE);

        oal_pcie_link_common_clk_set(pst_pci_dev);

        oal_pcie_link_l1ss_set(pst_pci_dev, OAL_TRUE);

        oal_pcie_link_aspm_enable(pst_pci_dev, PCIE_ASPM_CAP_L0S_AND_L1);

        oal_pcie_link_retrain();
        oal_pcie_rc_mem_unmap();
    }

    if(!oal_strcmp(casename, "read_rc_reg32"))
    {
        oal_uint32 reg = 0;
        for(;*buf == ' ';buf++);
        if ((sscanf(buf, "0x%x", &reg) != 1) || reg > 4096)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR,  "invaild input:[%s]", buf);
        }
        else
        {
            oal_pcie_rc_reg_print(reg);
        }
    }
#endif

    if(!oal_strcmp(casename, "d2h_doorbell"))
    {
        oal_pcie_d2h_doorbell(pst_pcie_res);
    }

    if(!oal_strcmp(casename, "h2d_doorbell"))
    {
        oal_pcie_h2d_doorbell(pst_pcie_res);
    }

    if(!oal_strcmp(casename, "outbound_test"))
    {
        buf = buf + OAL_STRLEN("outbound_test");
        for(;*buf == ' ';buf++);
        oal_pcie_outbound_test(pst_pcie_res, buf);
    }

    if(!oal_strcmp(casename, "ram_memcheck"))
    {
        /*遍历itcm,dtcm,pktmem,扫内存*/
        oal_pcie_device_memcheck_auto(pst_pcie_res);
    }

    if(!oal_strcmp(casename, "PME_ENABLE"))
    {
        oal_uint32 pm, reg = 0;
        /*Enable PME*/
        pm = pci_find_capability(pst_pci_dev, PCI_CAP_ID_PM);
        if (!pm) {
            PCI_PRINT_LOG(PCI_LOG_ERR,  "can't get PCI_CAP_ID_PM");
            return OAL_SUCC;
        }
        PCI_PRINT_LOG(PCI_LOG_INFO, "PME OFF : %u", pm);
        ret = oal_pci_read_config_dword(pst_pci_dev, pm + PCI_PM_CTRL, &reg);
        if(ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR,  "read %u failed", pm + PCI_PM_CTRL);
            return OAL_SUCC;
        }

        OAL_IO_PRINT("read %u value:\n", pm + PCI_PM_CTRL);
        oal_pcie_print_bits(&reg, sizeof(reg));

        reg |= 0x100;

        ret = oal_pci_write_config_dword(pst_pci_dev, pm + PCI_PM_CTRL, reg);
        if(ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR,  "write %u failed", pm + PCI_PM_CTRL);
        } else {
            PCI_PRINT_LOG(PCI_LOG_INFO,  "write %u ok", pm + PCI_PM_CTRL);
        }
    }

    if(!oal_strcmp(casename, "ringbuf_stat"))
    {
        oal_int32 i;
        oal_uint32 freecount, usedcount;
        OAL_IO_PRINT("h2d ringbuf info:\n");
        for(i = 0; i < PCIE_H2D_QTYPE_BUTT ; i++)
        {
            freecount = oal_pcie_h2d_ringbuf_freecount(pst_pcie_res, (PCIE_H2D_RINGBUF_QTYPE)i, OAL_TRUE);
            usedcount = pcie_ringbuf_len(&pst_pcie_res->st_ringbuf.st_h2d_buf[i]);
            OAL_IO_PRINT("qtype:%d , freecount:%u, used_count:%u \n",
                i, freecount, usedcount);
        }
        OAL_IO_PRINT("\nd2h ringbuf info:\n");
        freecount =  oal_pcie_d2h_ringbuf_freecount(pst_pcie_res, OAL_TRUE);
        usedcount = pcie_ringbuf_len(&pst_pcie_res->st_ringbuf.st_d2h_buf);
        OAL_IO_PRINT("freecount:%u, used_count:%u \n",
               freecount, usedcount);

    }

    if(!oal_strcmp(casename, "clear_trans_info"))
    {
        OAL_MEMZERO((oal_void*)pst_pcie_res->st_tx_res[PCIE_H2D_QTYPE_NORMAL].stat.tx_burst_cnt, sizeof(pst_pcie_res->st_tx_res[PCIE_H2D_QTYPE_NORMAL].stat.tx_burst_cnt) );
        OAL_MEMZERO((oal_void*)pst_pcie_res->st_rx_res.stat.rx_burst_cnt, sizeof(pst_pcie_res->st_rx_res.stat.rx_burst_cnt) );
        OAL_IO_PRINT("clear_trans_info done\n");
    }

#ifdef CONFIG_ARCH_SD56XX
    if(!oal_strcmp(casename, "gpio_87_test"))
    {
        oal_uint32 ul_val = 0x0;
        oal_void* pst_gpio_mode = oal_ioremap_nocache(0x14900000, 0x200);
        oal_void* pst_gpio_base =  oal_ioremap_nocache(0x10108000, 0x100);
        if (OAL_PTR_NULL == pst_gpio_base || NULL == pst_gpio_mode)
        {
            OAL_IO_PRINT("pst_gpio_base is %p, pst_gpio_mode is %p!\n", pst_gpio_base, pst_gpio_mode);
        }
        else
        {
#if 1
            /* 操作芯片1的上下电复位 */
            /*1.设置成软件模式,配置寄存器0x149001a0第7bit为1 */
            ul_val = oal_readl(pst_gpio_mode + 0x1a0);
            ul_val |= BIT11;
            oal_writel(ul_val, pst_gpio_mode + 0x1a0);
#endif

            /*2.设置数据方向，配置寄存器0x10108004第21bit为1 */
            ul_val =  oal_readl(pst_gpio_base + 0x4);
            ul_val |= BIT23;
            oal_writel(ul_val, pst_gpio_base + 0x4);

            /*3.设置GPIO87拉低，GPIO芯片1下电 */
            ul_val =  oal_readl(pst_gpio_base + 0x0);
            ul_val &= ~BIT23;
            oal_writel(ul_val, pst_gpio_base + 0x0);

            oal_udelay(10);

            /*4.设置GPIO87拉高，GPIO芯片1上电 */
            ul_val =  oal_readl(pst_gpio_base + 0x0);
            ul_val |= BIT23;
            oal_writel(ul_val, pst_gpio_base + 0x0);
        }

    }
#endif

    if(!oal_strcmp(casename, "show_trans_info"))
    {
        oal_int32 i;
        OAL_IO_PRINT("tx h2d trans info:\n");
        for(i = 0 ; i <
          sizeof(pst_pcie_res->st_tx_res[PCIE_H2D_QTYPE_NORMAL].stat.tx_burst_cnt)/sizeof(pst_pcie_res->st_tx_res[PCIE_H2D_QTYPE_NORMAL].stat.tx_burst_cnt[0]); i++)
        {
            OAL_IO_PRINT("[%3d] burst cnt:%u\n",i,
            pst_pcie_res->st_tx_res[PCIE_H2D_QTYPE_NORMAL].stat.tx_burst_cnt[i]);
        }

        OAL_IO_PRINT("\nrx d2h trans info:\n");
        for(i = 0 ; i <
          sizeof(pst_pcie_res->st_rx_res.stat.rx_burst_cnt)/sizeof(pst_pcie_res->st_rx_res.stat.rx_burst_cnt[0]); i++)
        {
            OAL_IO_PRINT("[%3d] burst cnt:%u\n",i,
            pst_pcie_res->st_rx_res.stat.rx_burst_cnt[i]);
        }

        oal_pcie_print_device_transfer_info(pst_pcie_res);
    }

    if(!oal_strcmp(casename, "send_message"))
    {
        oal_uint32 message;
        buf = buf + OAL_STRLEN("send_message");
        for(;*buf == ' ';buf++);
        if ((sscanf(buf, "%u", &message) != 1))
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "invalid agrument");
        }
        else
        {
            ret = oal_pcie_send_message_to_dev(pst_pcie_res, message);
            PCI_PRINT_LOG(PCI_LOG_INFO, "send pcie message %u to dev %s", message, (OAL_SUCC == ret) ? "succ":"failed");
        }
    }

    if(!oal_strcmp(casename, "sched_rx_task"))
    {
        oal_pcie_linux_res *pst_pci_lres = (oal_pcie_linux_res*)oal_pci_get_drvdata(pst_pci_dev);
        if(NULL != pst_pci_lres && NULL != pst_pci_lres->pst_bus)
        {
            up(&pst_pci_lres->pst_bus->rx_sema);
        }
        else
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "sched failed!");
        }
    }

    /*重新上下电*/
    if(!oal_strcmp(casename, "pcie_powerdown"))
    {
#ifdef CONFIG_ARCH_SD56XX
        hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_DOWN);
        oal_pci_wlan_power_on(0);
#else
#endif
    }

    if(!oal_strcmp(casename, "pcie_powerup"))
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "pcie_powerup");
#ifdef CONFIG_ARCH_SD56XX
        hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LOAD_PREPARE);
        oal_pci_wlan_power_on(1);
        /*检查建链是否完成*/
        if(OAL_SUCC != oal_pcie_check_link_up())
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "power test relink failed!\n");
        }
        else
        {
            pcie_sys_reinit(1);
            PCI_PRINT_LOG(PCI_LOG_INFO, "pcie link up succ");
            ret = firmware_cfg_init_etc();
            if (0 == ret)
            {
                OAL_IO_PRINT("firmware_cfg_init_etc succ\n");
            }
            else
            {
                OAL_IO_PRINT("firmware_cfg_init_etc fail\n");
            }

            ret = hcc_bus_reinit(hcc_get_current_110x_bus());
            if(OAL_SUCC != ret)
            {
                PCI_PRINT_LOG(PCI_LOG_ERR, "hcc_bus_reinit failed!\n");
            }
            else
            {
                PCI_PRINT_LOG(PCI_LOG_INFO, "hcc_bus_reinit succ");
            }

            ret = firmware_download_etc(WIFI_CFG);
            if(ret)
            {
                PCI_PRINT_LOG(PCI_LOG_ERR, "firmware_download_etc %d failed!ret=%d\n", BFGX_AND_WIFI_CFG, ret);
            }
            else
            {
                PCI_PRINT_LOG(PCI_LOG_INFO, "firmware_download_etc ok");
            }

        }
#else
#endif
    }

    if(!oal_strcmp(casename, "pcie_poweron_test"))
    {
        /*Before this, must down firmware*/
        ret = hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LAUCH);
        if(ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "power on failed, ret=%d", ret);
        }
        else
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "power on succ, wcpu is up");
        }
    }

    if(!oal_strcmp(casename, "pcie_enum_download"))
    {
        /*第一次枚举下载PATCH*/
        hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LOAD_PREPARE);
#ifdef CONFIG_ARCH_SD56XX
        if(OAL_SUCC != oal_pcie_check_link_up())
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "power test relink failed!\n");
        }
        else
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "pcie link up succ");
#endif
            ret = firmware_cfg_init_etc();
            if (0 == ret)
            {
                OAL_IO_PRINT("firmware_cfg_init_etc succ\n");
                ret = hcc_bus_reinit(hcc_get_current_110x_bus());
                if(OAL_SUCC != ret)
                {
                    PCI_PRINT_LOG(PCI_LOG_ERR, "hcc_bus_reinit failed!\n");
                }
                else
                {
                    PCI_PRINT_LOG(PCI_LOG_INFO, "hcc_bus_reinit succ");
                    ret = firmware_download_etc(WIFI_CFG);
                    if(ret)
                    {
                        PCI_PRINT_LOG(PCI_LOG_ERR, "firmware_download_etc %d failed!ret=%d\n", BFGX_AND_WIFI_CFG, ret);
                    }
                    else
                    {
                        PCI_PRINT_LOG(PCI_LOG_INFO, "firmware_download_etc ok");
                        ret = hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LAUCH);
                        if(ret)
                        {
                            PCI_PRINT_LOG(PCI_LOG_ERR, "power on failed, ret=%d", ret);
                        }
                        else
                        {
                            PCI_PRINT_LOG(PCI_LOG_INFO, "power on succ, wcpu is up");
                        }
                    }
                }
            }
            else
            {
                OAL_IO_PRINT("firmware_cfg_init_etc fail\n");
            }

#ifdef CONFIG_ARCH_SD56XX
        }
#endif

    }


    if(!oal_strcmp(casename, "firmware_cfg_init_etc"))
    {
        ret = firmware_cfg_init_etc();
        if (0 == ret)
        {
            OAL_IO_PRINT("firmware_cfg_init_etc succ\n");
        }
        else
        {
            OAL_IO_PRINT("firmware_cfg_init_etc fail\n");
        }
    }

    if(!oal_strcmp(casename, "firmware_download_etc"))
    {
        oal_uint32 cfg_index = 0;
        buf = buf + OAL_STRLEN("firmware_download_etc");
        for(;*buf == ' ';buf++);
        if ((sscanf(buf, "%u", &cfg_index) != 1) || cfg_index > 4)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR,  "invaild input:[%s]", buf);
        }
        else
        {
            firmware_download_etc(cfg_index);
        }
	}

	if(!oal_strcmp(casename, "turnoff_message"))
	{
#ifdef CONFIG_ARCH_KIRIN_PCIE
        /*走到这里说明wakelock已经释放，WIFI已经深睡,通知RC/EP下电，
      发送TurnOff Message*/
        /*下电之前关闭 PCIE HOST 控制器*/
        PCI_PRINT_LOG(PCI_LOG_INFO, "turnoff_message kirin");
        kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL, NULL, NULL);
        kirin_pcie_pm_control(0, kirin_rc_idx);
#endif
	}

	if(!oal_strcmp(casename, "suspend_test"))
	{
#ifdef CONFIG_ARCH_KIRIN_PCIE
        oal_int32 ret;
        /*走到这里说明wakelock已经释放，WIFI已经深睡,通知RC/EP下电，
      发送TurnOff Message*/
        /*下电之前关闭 PCIE HOST 控制器*/
        PCI_PRINT_LOG(PCI_LOG_INFO, "suspend_test kirin");

        oal_pcie_change_link_state(pst_pcie_res, PCI_WLAN_LINK_DOWN);

        kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL, NULL, NULL);
        ret = kirin_pcie_pm_control(0, kirin_rc_idx);
        board_host_wakeup_dev_set(0);
        if(ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "suspend test power on failed, ret=%d", ret);
            DECLARE_DFT_TRACE_KEY_INFO("suspend test power on failed", OAL_DFT_TRACE_OTHER);
        }
#endif
	}

	if(!oal_strcmp(casename, "resume_test"))
	{
#ifdef CONFIG_ARCH_KIRIN_PCIE
        oal_int32 ret;
        /*走到这里说明wakelock已经释放，WIFI已经深睡,通知RC/EP下电，
      发送TurnOff Message*/
        /*下电之前关闭 PCIE HOST 控制器*/
        PCI_PRINT_LOG(PCI_LOG_INFO, "resume_test kirin");
        kirin_pcie_power_notifiy_register(kirin_rc_idx, oal_pcie_resume_handler, NULL, NULL);
        ret = kirin_pcie_pm_control(1, kirin_rc_idx);
        oal_pcie_change_link_state(pst_pcie_res, PCI_WLAN_LINK_WORK_UP);
        if(ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "resume test power on failed, ret=%d", ret);
            DECLARE_DFT_TRACE_KEY_INFO("resume test power on failed", OAL_DFT_TRACE_OTHER);
        }
#endif
	}

	if(!oal_strcmp(casename, "rc_send_polling"))
	{
#ifdef CONFIG_ARCH_KIRIN_PCIE
	    oal_pci_dev_stru *pst_rc_dev;
	    pst_rc_dev = pci_upstream_bridge(pst_pci_dev);
	    if(NULL == pst_rc_dev)
	    {
	        PCI_PRINT_LOG(PCI_LOG_ERR, "no upstream dev");
	    }
	    else
	    {
	        oal_uint16 val = 0;
	        oal_pci_read_config_word(pst_rc_dev, oal_pci_pcie_cap(pst_rc_dev) + PCI_EXP_LNKCTL2, &val);
	        PCI_PRINT_LOG(PCI_LOG_INFO, "rc polling read 0x%x , value:0x%x", oal_pci_pcie_cap(pst_rc_dev) + PCI_EXP_LNKCTL2,
	                        val);
            val |= (1 << 4);
	        oal_pci_write_config_word(pst_rc_dev, oal_pci_pcie_cap(pst_rc_dev) + PCI_EXP_LNKCTL2, val);
	    }
#endif
	}

	if(!oal_strcmp(casename, "ep_send_polling"))
	{
        oal_uint16 val = 0;
        ret = oal_pci_read_config_word(pst_pci_dev, oal_pci_pcie_cap(pst_pci_dev) + PCI_EXP_LNKCTL2, &val);
        if( 0 == ret )
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "ep polling read 0x%x , value:0x%x", oal_pci_pcie_cap(pst_pci_dev) + PCI_EXP_LNKCTL2,
                            val);
            val |= (1 << 4);
            ret = oal_pci_write_config_word(pst_pci_dev, oal_pci_pcie_cap(pst_pci_dev) + PCI_EXP_LNKCTL2, val);
            if( ret )
            {
                PCI_PRINT_LOG(PCI_LOG_WARN, "ep_send_polling write fail");
            }
        }
	}

    return OAL_SUCC;
}

oal_int32 oal_pcie_performance_read(oal_pcie_res* pst_pcie_res, oal_uint32 cpu_address,
                                           oal_uint32 length, oal_uint32 times, oal_uint32 burst_size)
{
    oal_int32 i;
    oal_int32 ret = OAL_SUCC;
    ktime_t  start_time, last_time;
    ktime_t trans_time;
    oal_uint64  trans_us;
    oal_uint64  trans_size, us_to_s, total_size;
    oal_uint32 size, copy_size, remainder;
    pci_addr_map addr_map_start, addr_map_end;
    oal_void* pst_burst_buf;
    oal_pci_dev_stru *pst_pci_dev;
    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }

    if(burst_size > length)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "burst_size  large %u than length %u", burst_size, length);
        return -OAL_EINVAL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpu_address, &addr_map_start);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "invalid device cpu address 0x%x", cpu_address);
        return -OAL_EINVAL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpu_address + length - 1, &addr_map_end);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "invalid device cpu address 0x%x and length %u", cpu_address, length);
        return -OAL_EINVAL;
    }

    pst_burst_buf = oal_memalloc(length);
    if(NULL == pst_burst_buf)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "alloc burst buf failed, buf size:%u", length);
        return -OAL_ENOMEM;
    }

    trans_size = 0;
    start_time= ktime_get();
    for(i = 0; i < times ;i++)
    {
        size = 0;
        remainder = length;
        for(;;)
        {
            if(0 == remainder)
            {
                break;
            }

            copy_size = (remainder <= burst_size) ? remainder : burst_size;

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
            oal_pci_cache_inv(pst_pci_dev, (oal_void*)(addr_map_start.pa + size), copy_size);
#endif
            oal_pcie_memcopy((oal_ulong)pst_burst_buf, (oal_ulong)addr_map_start.va + size, copy_size);
            remainder -= copy_size;
            size += copy_size;
            trans_size += copy_size;
        }
        //oal_schedule();
    }
    total_size = trans_size;
    last_time = ktime_get();
    trans_time = ktime_sub(last_time, start_time);
    trans_us = (oal_uint64)ktime_to_us(trans_time);

    if (trans_us == 0)
    {
        trans_us = 1;
    }

    trans_size = trans_size * 1000u;
    trans_size = trans_size * 1000u;
    trans_size = (trans_size >> 17);
    trans_size = div_u64(trans_size,trans_us);
    us_to_s = trans_us;
    do_div(us_to_s, 1000000u);

    OAL_IO_PRINT("red length:%u, total_size:%llu, burst_size:%u, thoughtput:%llu Mbps, trans_time:%llu us,  %llu s\n",
                    length, total_size, burst_size, trans_size, trans_us, us_to_s);

    oal_free(pst_burst_buf);

    return ret;
}

oal_int32 oal_pcie_performance_netbuf_alloc(oal_pcie_res* pst_pcie_res,
                                           oal_uint32 test_time)
{
    oal_uint64 count;
    oal_int32 ret = OAL_SUCC;
    oal_netbuf_stru* pst_netbuf;
    dma_addr_t pci_dma_addr;
    ktime_t  start_time, last_time;
    ktime_t trans_time;
    oal_ulong timeout;
    oal_uint64  trans_us;
    oal_uint64  trans_size, us_to_s, total_size;
    oal_pci_dev_stru *pst_pci_dev;
    pcie_cb_dma_res* pst_cb_res;
    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }


    start_time= ktime_get();
    timeout = jiffies + OAL_MSECS_TO_JIFFIES(test_time);
    count = 0;
    for(;;)
    {
        if(oal_time_after(jiffies, timeout))
        {
            break;
        }

        pst_netbuf = oal_pcie_rx_netbuf_alloc(HCC_HDR_TOTAL_LEN + PCIE_EDMA_TRANS_MAX_FRAME_LEN, GFP_ATOMIC);
        if(NULL == pst_netbuf)
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "alloc netbuf failed!");
            break;
        }

        oal_netbuf_put(pst_netbuf,(HCC_HDR_TOTAL_LEN + PCIE_EDMA_TRANS_MAX_FRAME_LEN));

        pci_dma_addr = dma_map_single(&pst_pci_dev->dev, OAL_NETBUF_DATA(pst_netbuf), OAL_NETBUF_LEN(pst_netbuf), PCI_DMA_FROMDEVICE);
        if (dma_mapping_error(&pst_pci_dev->dev, pci_dma_addr))
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "rx dma map netbuf failed, len=%u",
                        OAL_NETBUF_LEN(pst_netbuf));
            oal_netbuf_free(pst_netbuf);
            break;
        }

        pst_cb_res = (pcie_cb_dma_res*)OAL_NETBUF_CB(pst_netbuf);
        pst_cb_res->paddr.addr = pci_dma_addr;
        pst_cb_res->len = OAL_NETBUF_LEN(pst_netbuf);


        /*释放内存*/
        dma_unmap_single(&pst_pci_dev->dev, (dma_addr_t)pst_cb_res->paddr.addr, pst_cb_res->len, PCI_DMA_FROMDEVICE);
        oal_netbuf_free(pst_netbuf);

        count++;

    }

    total_size = (HCC_HDR_TOTAL_LEN + PCIE_EDMA_TRANS_MAX_FRAME_LEN)*count;
    trans_size = total_size;
    last_time = ktime_get();
    trans_time = ktime_sub(last_time, start_time);
    trans_us = (oal_uint64)ktime_to_us(trans_time);

    if (trans_us == 0)
    {
        trans_us = 1;
    }

    trans_size = trans_size * 1000u;
    trans_size = trans_size * 1000u;
    trans_size = (trans_size >> 17);
    trans_size = div_u64(trans_size,trans_us);
    us_to_s = trans_us;
    do_div(us_to_s, 1000000u);

    OAL_IO_PRINT("total_size:%llu, thoughtput:%llu Mbps, trans_time:%llu us,  %llu s\n",
                    total_size, trans_size, trans_us, us_to_s);


    return ret;
}

oal_int32 oal_pcie_performance_netbuf_queue(oal_pcie_res* pst_pcie_res,
                                           oal_uint32 test_time, oal_uint32 alloc_count)
{
    oal_uint64 count;
    oal_int32 i;
    oal_int32 ret = OAL_SUCC;
    oal_netbuf_stru* pst_netbuf;
    oal_netbuf_head_stru st_netbuf_queue;
    dma_addr_t pci_dma_addr;
    ktime_t  start_time, last_time;
    ktime_t trans_time;
    oal_ulong timeout;
    oal_uint64  trans_us;
    oal_uint64  trans_size, us_to_s, total_size;
    oal_pci_dev_stru *pst_pci_dev;
    pcie_cb_dma_res* pst_cb_res;
    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }

    if(0 == alloc_count)
    {
        alloc_count = 1;
    }


    start_time= ktime_get();
    timeout = jiffies + OAL_MSECS_TO_JIFFIES(test_time);
    count = 0;
    oal_netbuf_head_init(&st_netbuf_queue);
    for(;;)
    {
        if(oal_time_after(jiffies, timeout))
        {
            break;
        }

        for(i = 0 ; i < alloc_count; i++)
        {
            pst_netbuf = oal_pcie_rx_netbuf_alloc(HCC_HDR_TOTAL_LEN + PCIE_EDMA_TRANS_MAX_FRAME_LEN, GFP_ATOMIC);
            if(NULL == pst_netbuf)
            {
                PCI_PRINT_LOG(PCI_LOG_INFO, "alloc netbuf failed!");
                break;
            }

            oal_netbuf_put(pst_netbuf,(HCC_HDR_TOTAL_LEN + PCIE_EDMA_TRANS_MAX_FRAME_LEN));

            pci_dma_addr = dma_map_single(&pst_pci_dev->dev, OAL_NETBUF_DATA(pst_netbuf), OAL_NETBUF_LEN(pst_netbuf), PCI_DMA_FROMDEVICE);
            if (dma_mapping_error(&pst_pci_dev->dev, pci_dma_addr))
            {
                PCI_PRINT_LOG(PCI_LOG_INFO, "rx dma map netbuf failed, len=%u",
                            OAL_NETBUF_LEN(pst_netbuf));
                oal_netbuf_free(pst_netbuf);
                break;
            }

            pst_cb_res = (pcie_cb_dma_res*)OAL_NETBUF_CB(pst_netbuf);
            pst_cb_res->paddr.addr = pci_dma_addr;
            pst_cb_res->len = OAL_NETBUF_LEN(pst_netbuf);

            oal_netbuf_list_tail_nolock(&st_netbuf_queue, pst_netbuf);
        }


        for(;;)
        {
            pst_netbuf = oal_netbuf_delist_nolock(&st_netbuf_queue);
            if(NULL == pst_netbuf)
            {
                break;
            }

            pst_cb_res = (pcie_cb_dma_res*)OAL_NETBUF_CB(pst_netbuf);
            /*释放内存*/
            dma_unmap_single(&pst_pci_dev->dev, (dma_addr_t)pst_cb_res->paddr.addr, pst_cb_res->len, PCI_DMA_FROMDEVICE);
            oal_netbuf_free(pst_netbuf);
            count++;
        }

    }

    total_size = (HCC_HDR_TOTAL_LEN + PCIE_EDMA_TRANS_MAX_FRAME_LEN)*count;
    last_time = ktime_get();
    trans_time = ktime_sub(last_time, start_time);
    trans_us = (oal_uint64)ktime_to_us(trans_time);

    if (trans_us == 0)
    {
        trans_us = 1;
    }
    trans_size = total_size;
    trans_size = trans_size * 1000u;
    trans_size = trans_size * 1000u;
    trans_size = (trans_size >> 17);
    trans_size = div_u64(trans_size,trans_us);
    us_to_s = trans_us;
    do_div(us_to_s, 1000000u);

    OAL_IO_PRINT("alloc_count:%u, thoughtput:%llu Mbps, trans_time:%llu us,  %llu s\n",
                    alloc_count, trans_size, trans_us, us_to_s);

    return ret;
}



oal_int32 oal_pcie_performance_cpu(oal_pcie_res* pst_pcie_res,
                                           oal_uint32 length, oal_uint32 times, oal_uint32 burst_size)
{
    oal_int32 i;
    oal_int32 ret = OAL_SUCC;
    ktime_t  start_time, last_time;
    ktime_t trans_time;
    oal_uint64  trans_us;
    oal_uint64  trans_size, us_to_s, total_size;
    oal_uint32 size, copy_size, remainder;
    oal_void* pst_burst_buf_src;
    oal_void* pst_burst_buf_dst;
    oal_pci_dev_stru *pst_pci_dev;
    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }

    if(burst_size > length)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "burst_size  large %u than length %u", burst_size, length);
        return -OAL_EINVAL;
    }


    pst_burst_buf_dst = oal_memalloc(length);
    if(NULL == pst_burst_buf_dst)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "alloc src burst buf failed, buf size:%u", length);
        return -OAL_ENOMEM;
    }

    pst_burst_buf_src = oal_memalloc(length);
    if(NULL == pst_burst_buf_src)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "alloc dst burst buf failed, buf size:%u", length);
        oal_free(pst_burst_buf_dst);
        return -OAL_ENOMEM;
    }

    trans_size = 0;
    start_time= ktime_get();
    for(i = 0; i < times ;i++)
    {
        size = 0;
        remainder = length;
        for(;;)
        {
            if(0 == remainder)
            {
                break;
            }

            copy_size = (remainder <= burst_size) ? remainder : burst_size;

            oal_memcopy(pst_burst_buf_dst, pst_burst_buf_src, copy_size);
            remainder -= copy_size;
            size += copy_size;
            trans_size += copy_size;
        }
        //oal_schedule();
    }
    total_size = trans_size;
    last_time = ktime_get();
    trans_time = ktime_sub(last_time, start_time);
    trans_us = (oal_uint64)ktime_to_us(trans_time);

    if (trans_us == 0)
    {
        trans_us = 1;
    }

    trans_size = trans_size * 1000u;
    trans_size = trans_size * 1000u;
    trans_size = (trans_size >> 17);
    trans_size = div_u64(trans_size,trans_us);
    us_to_s = trans_us;
    do_div(us_to_s, 1000000u);

    OAL_IO_PRINT("red length:%u, total_size:%llu, burst_size:%u, thoughtput:%llu Mbps, trans_time:%llu us,  %llu s\n",
                    length, total_size, burst_size, trans_size, trans_us, us_to_s);

    oal_free(pst_burst_buf_dst);
    oal_free(pst_burst_buf_src);

    return ret;
}

oal_int32 oal_pcie_performance_write(oal_pcie_res* pst_pcie_res, oal_uint32 cpu_address,
                                           oal_uint32 length, oal_uint32 times, oal_uint32 burst_size, oal_uint32 value)
{
    oal_int32 i;
    oal_int32 ret = OAL_SUCC;
    ktime_t  start_time, last_time;
    ktime_t trans_time;
    oal_uint64  trans_us;
    oal_uint64  trans_size, us_to_s, total_size;
    oal_uint32 size, copy_size, remainder;
    pci_addr_map addr_map_start, addr_map_end;
    oal_void* pst_burst_buf;
    oal_pci_dev_stru *pst_pci_dev;
    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }

    if(burst_size > length)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "burst_size  large %u than length %u", burst_size, length);
        return -OAL_EINVAL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpu_address, &addr_map_start);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "invalid device cpu address 0x%x", cpu_address);
        return -OAL_EINVAL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpu_address + length - 1, &addr_map_end);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "invalid device cpu address 0x%x and length %u", cpu_address, length);
        return -OAL_EINVAL;
    }

    pst_burst_buf = oal_memalloc(length);
    if(NULL == pst_burst_buf)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "alloc burst buf failed, buf size:%u", length);
        return -OAL_ENOMEM;
    }

    oal_memset(pst_burst_buf, (oal_int32)value, length);

    trans_size = 0;
    start_time= ktime_get();
    for(i = 0; i < times ;i++)
    {
        size = 0;
        remainder = length;
        for(;;)
        {
            if(0 == remainder)
            {
                break;
            }

            copy_size = (remainder <= burst_size) ? remainder : burst_size;
            oal_pcie_memcopy((oal_ulong)addr_map_start.va + size, (oal_ulong)pst_burst_buf, copy_size);
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
            oal_pci_cache_flush(pst_pci_dev, (oal_void*)(addr_map_start.pa + size), copy_size);
#endif
            remainder -= copy_size;
            size += copy_size;
            trans_size += copy_size;
        }
        //oal_schedule();
    }

    total_size = trans_size;
    last_time = ktime_get();
    trans_time = ktime_sub(last_time, start_time);
    trans_us = (oal_uint64)ktime_to_us(trans_time);

    if (trans_us == 0)
    {
        trans_us = 1;
    }

    trans_size = trans_size * 1000u;
    trans_size = trans_size * 1000u;
    trans_size = (trans_size >> 17);
    trans_size = div_u64(trans_size,trans_us);
    us_to_s = trans_us;
    do_div(us_to_s, 1000000u);

    OAL_IO_PRINT(" write length:%u, total_size:%llu,burst_size:%u,value:0x%x thoughtput:%llu Mbps, trans_time:%llu us,  %llu s\n",
                    length, total_size,burst_size, value,trans_size, trans_us, us_to_s);

    oal_free(pst_burst_buf);

    return ret;
}


oal_int32 oal_pcie_debug_performance_read(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    /*echo readmem address length(hex) > debug*/
    oal_int32 ret;
    oal_uint32 cpu_address;
    oal_uint32 length, times, burst_size;

    /*Just for debug*/
    if ((sscanf(buf, "0x%x %u %u %u", &cpu_address, &length, &times, &burst_size) != 4))
    {
        return -OAL_EINVAL;
    }

    ret = oal_pcie_performance_read(pst_pcie_res, cpu_address, length, times, burst_size);
    return ret;
}

oal_int32 oal_pcie_debug_performance_cpu(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    oal_int32 ret;
    oal_uint32 length, times, burst_size;

    /*Just for debug*/
    if ((sscanf(buf, "%u %u %u", &length, &times, &burst_size) != 3))
    {
        return -OAL_EINVAL;
    }

    ret = oal_pcie_performance_cpu(pst_pcie_res, length, times, burst_size);
    return ret;
}

oal_int32 oal_pcie_debug_performance_netbuf_alloc(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    oal_int32 ret;
    oal_uint32 test_time;

    /*Just for debug*/
    if ((sscanf(buf, "%u", &test_time) != 1))
    {
        return -OAL_EINVAL;
    }

    ret = oal_pcie_performance_netbuf_alloc(pst_pcie_res, test_time);
    return ret;
}

oal_int32 oal_pcie_debug_performance_netbuf_queue(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    oal_int32 ret;
    oal_uint32 test_time, alloc_count;

    /*Just for debug*/
    if ((sscanf(buf, "%u %u", &test_time, &alloc_count) != 2))
    {
        return -OAL_EINVAL;
    }

    ret = oal_pcie_performance_netbuf_queue(pst_pcie_res, test_time, alloc_count);
    return ret;
}

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_H2D_BYPASS
oal_int32 oal_pcie_performance_h2d_bypass(oal_pcie_res* pst_pcie_res,
                                           oal_uint32 pkt_num, oal_uint32 pkt_len)
{
    /*bypass hcc*/
    oal_int32 i;
    oal_pci_dev_stru *pst_pci_dev;
    pcie_cb_dma_res* pst_cb_res;
    oal_ulong timeout;
    ktime_t  start_time, last_time;
    ktime_t trans_time;
    oal_int32 flag = 0;
    oal_uint64  trans_us;
    oal_uint64  trans_size, us_to_s, total_size;

    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }


    if(pkt_num < 1 || pkt_len >  PCIE_EDMA_TRANS_MAX_FRAME_LEN)
    {
        OAL_IO_PRINT("invalid argument, pkt_num:%u, pkt_len:%u\n", pkt_num, pkt_len);
        return -OAL_EINVAL;
    }

    g_h2d_bypass_pkt_num = 0;

    g_h2d_pst_netbuf = oal_pcie_rx_netbuf_alloc(HCC_HDR_TOTAL_LEN + pkt_len, GFP_ATOMIC);
    if(NULL == g_h2d_pst_netbuf)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "alloc netbuf %u failed!", HCC_HDR_TOTAL_LEN + pkt_len);
        return -OAL_ENOMEM;
    }

    oal_netbuf_put(g_h2d_pst_netbuf,HCC_HDR_TOTAL_LEN + pkt_len);

    g_h2d_pci_dma_addr = dma_map_single(&pst_pci_dev->dev, OAL_NETBUF_DATA(g_h2d_pst_netbuf), OAL_NETBUF_LEN(g_h2d_pst_netbuf), PCI_DMA_FROMDEVICE);
    if (dma_mapping_error(&pst_pci_dev->dev, g_h2d_pci_dma_addr))
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "h2d bypass dma map netbuf failed, len=%u",
                    OAL_NETBUF_LEN(g_h2d_pst_netbuf));
        oal_netbuf_free(g_h2d_pst_netbuf);
        g_h2d_pst_netbuf = NULL;
        return -OAL_ENOMEM;
    }

    pst_cb_res = (pcie_cb_dma_res*)OAL_NETBUF_CB(g_h2d_pst_netbuf);
    pst_cb_res->paddr.addr = g_h2d_pci_dma_addr;
    pst_cb_res->len = OAL_NETBUF_LEN(g_h2d_pst_netbuf);

    OAL_IO_PRINT("performance_h2d_bypass cpu=%d\n", get_cpu());
    put_cpu();

    start_time= ktime_get();

    for(i = 0; i < pkt_num; i++)
    {
        for(;;)
        {
            if(0 == oal_pcie_send_netbuf(pst_pcie_res, g_h2d_pst_netbuf, PCIE_H2D_QTYPE_NORMAL))
            {
                if(flag)
                {
                    /*更新device侧wr指针,刷ringbuf cache*/
                    oal_pcie_h2d_ringbuf_wr_update(pst_pcie_res, PCIE_H2D_QTYPE_NORMAL);

                    /*tx doorbell*/
                    oal_pcie_h2d_doorbell(pst_pcie_res);
                }
                flag = 0;
                cpu_relax();
            }
            else
            {
                flag = 1;
                break;
            }
        }

        /*更新device侧wr指针,刷ringbuf cache*/
        oal_pcie_h2d_ringbuf_wr_update(pst_pcie_res, PCIE_H2D_QTYPE_NORMAL);

        /*tx doorbell*/
        oal_pcie_h2d_doorbell(pst_pcie_res);
    }

    /*更新device侧wr指针,刷ringbuf cache*/
    oal_pcie_h2d_ringbuf_wr_update(pst_pcie_res, PCIE_H2D_QTYPE_NORMAL);

    /*tx doorbell*/
    oal_pcie_h2d_doorbell(pst_pcie_res);


    /*等待 回来的 包个数 */

    timeout = jiffies + OAL_MSECS_TO_JIFFIES(10000);
    for(;;)
    {
        if(oal_time_after(jiffies, timeout))
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "wait h2d bypass transfer done timeout, pkt num:%u, just came :%u", pkt_num, g_h2d_bypass_pkt_num);
            break;
        }

        if(g_h2d_bypass_pkt_num >= pkt_num)
        {
            break;
        }
        cpu_relax();
    }

    last_time = ktime_get();
    trans_time = ktime_sub(last_time, start_time);
    trans_us = (oal_uint64)ktime_to_us(trans_time);

    if (trans_us == 0)
    {
        trans_us = 1;
    }

    trans_size = (oal_uint64)pkt_len*pkt_num;
    trans_size = trans_size * 1000u;
    trans_size = trans_size * 1000u;
    trans_size = (trans_size >> 17);
    trans_size = div_u64(trans_size,trans_us);
    us_to_s = trans_us;
    do_div(us_to_s, 1000000u);

    OAL_IO_PRINT("pkt_num:%u, thoughtput:%llu Mbps, trans_time:%llu us,  %llu s\n",
                    pkt_num, trans_size, trans_us, us_to_s);

    dma_unmap_single(&pst_pci_dev->dev, (dma_addr_t)g_h2d_pci_dma_addr, OAL_NETBUF_LEN(g_h2d_pst_netbuf), PCI_DMA_FROMDEVICE);
    g_h2d_pci_dma_addr = NULL;
    oal_netbuf_free(g_h2d_pst_netbuf);
    g_h2d_pst_netbuf = NULL;
    return OAL_SUCC;
}

oal_int32 oal_pcie_debug_performance_h2d_bypass(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    oal_int32 ret;
    oal_uint32 pkt_num, pkt_len;

    /*Just for debug*/
    if ((sscanf(buf, "%u %u", &pkt_num, &pkt_len) != 2))
    {
        return -OAL_EINVAL;
    }

    ret = oal_pcie_performance_h2d_bypass(pst_pcie_res, pkt_num, pkt_len);
    return ret;
}
#endif

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_D2H_BYPASS
oal_int32 oal_pcie_performance_d2h_bypass(oal_pcie_res* pst_pcie_res,
                                           oal_uint32 pkt_num, oal_uint32 pkt_len)
{
    /*bypass hcc*/
    oal_int32 i;
    oal_int32 ret;
    oal_pci_dev_stru *pst_pci_dev;
    pcie_cb_dma_res* pst_cb_res;
    oal_ulong timeout;
    ktime_t  start_time, last_time;
    ktime_t trans_time;
    oal_int32 flag = 0;
    oal_uint64  trans_us;
    oal_uint64  trans_size, us_to_s, total_size;

    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }


    if(pkt_num < 1 || pkt_len >  PCIE_EDMA_TRANS_MAX_FRAME_LEN || pkt_num > 65536)
    {
        OAL_IO_PRINT("invalid argument, pkt_num:%u, pkt_len:%u\n", pkt_num, pkt_len);
        return -OAL_EINVAL;
    }

    g_d2h_bypass_pkt_num = 0;

    g_d2h_pst_netbuf = oal_pcie_rx_netbuf_alloc(HCC_HDR_TOTAL_LEN + PCIE_EDMA_TRANS_MAX_FRAME_LEN, GFP_ATOMIC);
    if(NULL == g_d2h_pst_netbuf)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "alloc netbuf %u failed!", HCC_HDR_TOTAL_LEN + PCIE_EDMA_TRANS_MAX_FRAME_LEN);
        return -OAL_ENOMEM;
    }

    oal_netbuf_put(g_d2h_pst_netbuf,HCC_HDR_TOTAL_LEN + PCIE_EDMA_TRANS_MAX_FRAME_LEN);

    g_d2h_pci_dma_addr = dma_map_single(&pst_pci_dev->dev, OAL_NETBUF_DATA(g_d2h_pst_netbuf), OAL_NETBUF_LEN(g_d2h_pst_netbuf), PCI_DMA_FROMDEVICE);
    if (dma_mapping_error(&pst_pci_dev->dev, g_d2h_pci_dma_addr))
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "h2d bypass dma map netbuf failed, len=%u",
                    OAL_NETBUF_LEN(g_d2h_pst_netbuf));
        oal_netbuf_free(g_d2h_pst_netbuf);
        g_d2h_pst_netbuf = NULL;
        return -OAL_ENOMEM;
    }

    pst_cb_res = (pcie_cb_dma_res*)OAL_NETBUF_CB(g_d2h_pst_netbuf);
    pst_cb_res->paddr.addr = g_d2h_pci_dma_addr;
    pst_cb_res->len = OAL_NETBUF_LEN(g_d2h_pst_netbuf);
    INIT_COMPLETION(g_d2h_test_done);

    oal_pcie_rx_ringbuf_bypass_supply(pst_pcie_res, OAL_TRUE, OAL_TRUE, PCIE_RX_RINGBUF_SUPPLY_ALL);
    //start_time= ktime_get();

    /*启动RX*/
    g_d2h_bypass_total_pkt_num = pkt_num;
    oal_writel((oal_uint16)pkt_len << 16 | ((oal_uint16) pkt_num ), pst_pcie_res->pst_pci_ctrl_base + PCIE_HOST_DEVICE_REG0);
    oal_writel(PCIE_H2D_TRIGGER_VALUE, pst_pcie_res->pst_pci_ctrl_base + PCIE_D2H_DOORBELL_OFF);

    oal_pcie_mips_start(PCIE_MIPS_HCC_RX_TOTAL);
    start_time= ktime_get();
    /*补充内存*/

    ret = wait_for_completion_interruptible(&g_d2h_test_done);
    if(ret < 0)
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "g_d2h_test_done wait interrupt!, rx cnt:%u", g_d2h_bypass_pkt_num);
    }

    last_time = ktime_get();
    oal_pcie_mips_end(PCIE_MIPS_HCC_RX_TOTAL);
    trans_time = ktime_sub(last_time, start_time);
    trans_us = (oal_uint64)ktime_to_us(trans_time);

    if (trans_us == 0)
    {
        trans_us = 1;
    }

    trans_size = pkt_len*g_d2h_bypass_pkt_num;
    trans_size = trans_size * 1000u;
    trans_size = trans_size * 1000u;
    trans_size = (trans_size >> 17);
    trans_size = div_u64(trans_size,trans_us);
    us_to_s = trans_us;
    do_div(us_to_s, 1000000u);

    OAL_IO_PRINT("pkt_num:%u, thoughtput:%llu Mbps, trans_time:%llu us,  %llu s\n",
                    g_d2h_bypass_pkt_num, trans_size, trans_us, us_to_s);
    oal_pcie_mips_show();

    dma_unmap_single(&pst_pci_dev->dev, (dma_addr_t)g_d2h_pci_dma_addr, OAL_NETBUF_LEN(g_d2h_pst_netbuf), PCI_DMA_FROMDEVICE);
    g_d2h_pci_dma_addr = NULL;
    oal_netbuf_free(g_d2h_pst_netbuf);
    g_d2h_pst_netbuf = NULL;
    return OAL_SUCC;
}

oal_int32 oal_pcie_debug_performance_d2h_bypass(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    oal_int32 ret;
    oal_uint32 pkt_num, pkt_len;

    /*Just for debug*/
    if ((sscanf(buf, "%u %u", &pkt_num, &pkt_len) != 2))
    {
        return -OAL_EINVAL;
    }

    ret = oal_pcie_performance_d2h_bypass(pst_pcie_res, pkt_num, pkt_len);
    return ret;
}
#endif


oal_int32 oal_pcie_debug_performance_write(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    /*echo readmem address length(hex) > debug*/
    oal_int32 ret;
    oal_uint32 cpu_address;
    oal_uint32 length, times, burst_size, value;

    /*Just for debug*/
    if ((sscanf(buf, "0x%x %u %u %u 0x%x", &cpu_address, &length, &times, &burst_size, &value) != 5))
    {
        return -OAL_EINVAL;
    }

    ret = oal_pcie_performance_write(pst_pcie_res, cpu_address, length, times, burst_size, value);
    return ret;
}

oal_int32 oal_pcie_loadfile(oal_pcie_res* pst_pcie_res, char* file_name, oal_uint32 cpu_address, oal_int32 performance)
{
    /*echo loadfile address filename > debug*/
    /*echo loadfile 0x600000 /tmp/readmem.bin */
    struct file *fp;

    oal_int32 ret, rlen, total_len;
    pci_addr_map addr_map;
    //mm_segment_t fs;
    oal_void* pst_buf;
    oal_pci_dev_stru *pst_pci_dev;
    ktime_t  start_time, last_time;
    ktime_t  trans_time;
    oal_uint64 us_to_s, trans_us, file_us;

    if(NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_EBUSY;
    }

    pst_pci_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pci_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pci_dev is null");
        return -OAL_ENODEV;
    }

    pst_buf = oal_memalloc(PAGE_SIZE);
    if(NULL == pst_buf)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_buf is null");
        return -OAL_ENOMEM;
    }

    fp = filp_open(file_name, O_RDWR, 0664);
    if (OAL_IS_ERR_OR_NULL(fp))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "open file error,fp = 0x%p, filename is [%s]\n", fp, file_name);
        oal_free(pst_buf);
        return -OAL_EINVAL;
    }

    //PCI_PRINT_LOG(PCI_LOG_INFO, "loadfile cpu:0x%8x loadpath:%s", cpu_address, file_name);
    total_len = 0;
    file_us = 0;
    us_to_s = 0;
    trans_us = 0;

    if(performance)
        start_time= ktime_get();

    for(;;)
    {
        rlen = kernel_read(fp, fp->f_pos, (void*)pst_buf, PAGE_SIZE);
        if(rlen <= 0)
        {
            break;
        }
        fp->f_pos += rlen;
        ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpu_address + total_len, &addr_map);
        if(OAL_SUCC != ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "loadfile address 0x%8x invalid", cpu_address + total_len);
            break;
        }
        total_len += rlen;
        oal_pcie_memcopy((oal_ulong)addr_map.va, (oal_ulong)pst_buf, rlen);
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        oal_pci_cache_flush(pst_pci_dev, (oal_void*)addr_map.pa, rlen);
#endif
    }

    if(performance)
    {
        last_time = ktime_get();
        trans_time = ktime_sub(last_time, start_time);
        trans_us = (oal_uint64)ktime_to_us(trans_time);

        if (trans_us == 0)
        {
            trans_us = 1;
        }

        us_to_s = trans_us;
        do_div(us_to_s, 1000000u);
    }

    if(total_len)
    {
        if(performance)
            PCI_PRINT_LOG(PCI_LOG_INFO, "loadfile cpu:0x%8x len:%u loadpath:%s done, cost %llu us %llu s, file_us:%llu",
                    cpu_address, total_len, file_name, trans_us, us_to_s, file_us);
        else
            PCI_PRINT_LOG(PCI_LOG_INFO, "loadfile cpu:0x%8x len:%u loadpath:%s done",
                    cpu_address, total_len, file_name);
    }else{
        PCI_PRINT_LOG(PCI_LOG_INFO, "loadfile cpu:0x%8x len:%u loadpath:%s failed", cpu_address, total_len, file_name);
    }
    oal_free(pst_buf);
    //fs = get_fs();
    //set_fs(KERNEL_DS);
    filp_close(fp, NULL);
    return OAL_SUCC;
}

oal_int32 oal_pcie_debug_loadfile(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    oal_int32 ret;
    char file_name[100];
    oal_uint32 cpu_address;

    if (strlen(buf) >= OAL_SIZEOF(file_name))
    {
        OAL_IO_PRINT("input illegal!%s\n",__FUNCTION__);
        return -OAL_EINVAL;
    }

    if ((sscanf(buf, "0x%x %s", &cpu_address, file_name) != 2))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "loadfile argument invalid,[%s], should be [echo writemem address  filename > debug]", buf);
        return -OAL_EINVAL;
    }

    ret = oal_pcie_loadfile(pst_pcie_res, file_name, cpu_address, 1);

    return ret;
}

/*sysfs debug*/
typedef struct _pcie_sysfs_debug_info_
{
    char* name;
    char* usage;
    oal_int32  (*debug)(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res);
}pcie_sysfs_debug_info;

oal_int32 oal_pcie_print_debug_info(struct device *dev, struct device_attribute *attr, char*buf, oal_pcie_res* pst_pcie_res)
{
    oal_pcie_print_debug_usages();
    return OAL_SUCC;
}

OAL_STATIC pcie_sysfs_debug_info g_pci_debug[] =
{
    {"help",                 "" , oal_pcie_print_debug_info},
    {"dump_all_regions_mem", "" , oal_pcie_dump_all_regions_mem},
    {"dump_all_regions_info","address(hex) value(hex)" , oal_pcie_dump_all_regions},
    {"read32",               "address(hex)" , oal_pcie_debug_read32},
    {"write32",              "address(hex) value(hex)" , oal_pcie_debug_write32},
    {"host_read32",          "address(hex)" , oal_pcie_debug_host_read32},
    {"host_write32",         "address(hex) value(hex)" , oal_pcie_debug_host_write32},
    {"read16",               "address(hex)" , oal_pcie_debug_read16},
    {"write16",              "address(hex) value(hex)" , oal_pcie_debug_write16},
    {"saveconfigmem",        "address(hex) length(decimal) filename" , oal_pcie_debug_saveconfigmem},
    {"savemem",              "address(hex) length(decimal) filename" , oal_pcie_debug_savemem},
    {"save_hostmem",         "address(hex) length(decimal) filename" , oal_pcie_debug_save_hostmem},
    {"loadfile",             "address(hex) filename" , oal_pcie_debug_loadfile},
    {"readmem",              "address(hex) length(decimal)" , oal_pcie_debug_readmem},
    {"readconfigmem",        "address(hex) length(decimal)" , oal_pcie_debug_readmem_config},
    {"performance_read",     "address(hex) length(decimal) times(decimal) burst_size(decimal)" , oal_pcie_debug_performance_read},
    {"performance_cpu",      "length(decimal) times(decimal) burst_size(decimal)" , oal_pcie_debug_performance_cpu},
    {"performance_write",    "address(hex) length(decimal) times(decimal) burst_size(decimal) value(hex) " , oal_pcie_debug_performance_write},
    {"performance_netbuf_alloc",    "test_time(decimal msec)" , oal_pcie_debug_performance_netbuf_alloc},
    {"performance_netbuf_queue",    "test_time(decimal msec) alloc_count(decimal)" , oal_pcie_debug_performance_netbuf_queue},
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_H2D_BYPASS
    {"performance_h2d_bypass","pkt_num(decimal) pkt_len(decimal)" , oal_pcie_debug_performance_h2d_bypass},
#endif
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_D2H_BYPASS
    {"performance_d2h_bypass","pkt_num(decimal) pkt_len(decimal)" , oal_pcie_debug_performance_d2h_bypass},
#endif
    {"testcase",             "casename(string)" , oal_pcie_debug_testcase},
    {"wlan_power_on_etc", "", oal_pcie_debug_wlan_power_on},
    {"read_dsm32", "dsm_type(decimal)", oal_pcie_debug_read_dsm32},
    {"write_dsm32", "dsm_type(decimal) value(decimal)", oal_pcie_debug_write_dsm32}
};

OAL_STATIC oal_void oal_pcie_print_debug_usages(oal_void)
{
    oal_int32 i;
    oal_void* buf = oal_memalloc(PAGE_SIZE);
    if(NULL == buf)
    {
        return;
    }

    for(i = 0; i < OAL_ARRAY_SIZE(g_pci_debug); i++)
    {
        snprintf(buf, PAGE_SIZE, "echo %s %s > /sys/hisys/pci/pcie/debug\n", g_pci_debug[i].name ? :"", g_pci_debug[i].usage ? :"");
        printk("%s", (char*)buf);
    }

    oal_free(buf);
}

OAL_STATIC oal_void oal_pcie_print_debug_usage(oal_int32 i)
{
    oal_void* buf = oal_memalloc(PAGE_SIZE);
    if(NULL == buf)
    {
        return;
    }

    snprintf(buf, PAGE_SIZE, "echo %s %s > /sys/hisys/pci/pcie/debug\n", g_pci_debug[i].name ? :"", g_pci_debug[i].usage ? :"");
    printk("%s", (char*)buf);

    oal_free(buf);
}


OAL_STATIC ssize_t  oal_pcie_get_debug_info(struct device *dev, struct device_attribute *attr, char*buf)
{
    int ret;
    oal_int32 i;
    oal_int32 count = 0;

    ret = snprintf(buf + count, PAGE_SIZE - count, "pci debug cmds:\n");
    if (0 >= ret)
    {
        return count;
    }
    count += ret;
    for(i = 0; i < OAL_ARRAY_SIZE(g_pci_debug); i++)
    {
        ret = snprintf(buf + count, PAGE_SIZE - count, "%s\n", g_pci_debug[i].name);
        if (0 >= ret)
        {
            return count;
        }
        count += ret;
    }
    return count;
}

OAL_STATIC ssize_t  oal_pcie_set_debug_info(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    oal_int32 i;
    oal_pcie_res* pst_pcie_res = oal_get_default_pcie_handler();

    if(buf[count] != '\0') /*确保传进来的buf是一个字符串, count不包含结束符*/
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "invalid pci cmd\n");
        return 0;
    }

    //oal_print_hex_dump(buf, 64, 32, "dump: ");
    for(i = 0; i < OAL_ARRAY_SIZE(g_pci_debug); i++)
    {
        if(g_pci_debug[i].name)
        {
            if((count >= OAL_STRLEN(g_pci_debug[i].name)) &&
                !oal_memcmp(g_pci_debug[i].name, buf, OAL_STRLEN(g_pci_debug[i].name)))
            {
                /*判断最后一个字符是回车还是空格*/
                char last_c = *(buf + OAL_STRLEN(g_pci_debug[i].name));
                if(last_c == '\n' || last_c == ' ' || last_c == '\0')
                {
                    break;
                }
            }
        }
    }

    if(i == OAL_ARRAY_SIZE(g_pci_debug))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "invalid pci cmd:%s\n", buf);
        oal_pcie_print_debug_usages();
        return count;
    }

    //OAL_IO_PRINT("pcie cmd:%s process\n", g_pci_debug[i].name);

    buf += OAL_STRLEN(g_pci_debug[i].name);
    if(*buf != '\0')    //count > OAL_STRLEN(g_pci_debug[i].name)
    {
        buf += 1; /*EOF*/
    }

    for(;*buf == ' ';buf++);

    if(-OAL_EINVAL == g_pci_debug[i].debug(dev, attr, (char*)buf, pst_pcie_res))
    {
        oal_pcie_print_debug_usage(i);
    }
    return count;
}
OAL_STATIC DEVICE_ATTR(debug, S_IRUGO|S_IWUSR, oal_pcie_get_debug_info, oal_pcie_set_debug_info);
OAL_STATIC struct attribute *hpci_sysfs_entries[] = {
        &dev_attr_debug.attr,
        NULL
};

OAL_STATIC struct attribute_group hpci_attribute_group = {
        .name = "pcie",
        .attrs = hpci_sysfs_entries,
};

oal_int32 oal_pcie_sysfs_init(oal_pcie_res* pst_pcie_res)
{
    oal_int32       ret = -OAL_EFAIL;
    oal_kobject*     pst_root_object = NULL;

    pst_root_object = oal_get_sysfs_root_object_etc();
    if(NULL == pst_root_object)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "[E]get pci root sysfs object failed!\n");
        return -OAL_EFAIL;
    }

    g_conn_syfs_pci_object = kobject_create_and_add("pci", pst_root_object);
    if(NULL == g_conn_syfs_pci_object)
    {
        ret = -OAL_ENODEV;
        PCI_PRINT_LOG(PCI_LOG_ERR, "sysfs create kobject_create_and_add pci fail\n");
        goto fail_g_conn_syfs_pci_object;
    }

    ret = oal_debug_sysfs_create_group(g_conn_syfs_pci_object,&hpci_attribute_group);
    if (ret)
    {
        ret = -OAL_ENOMEM;
        PCI_PRINT_LOG(PCI_LOG_ERR, "sysfs create hpci_attribute_group group fail.ret=%d\n",ret);
        goto fail_create_pci_group;
    }

    return OAL_SUCC;
fail_create_pci_group:
    kobject_put(g_conn_syfs_pci_object);
    g_conn_syfs_pci_object = NULL;
fail_g_conn_syfs_pci_object:
    return ret;
}

oal_int32 oal_pcie_rx_thread_condtion(oal_atomic* pst_ato)
{
    oal_int32 ret = oal_atomic_read(pst_ato);
    if(OAL_LIKELY(1 == ret))
    {
        oal_atomic_set(pst_ato, 0);
    }

    return ret;
}

oal_int32 oal_pcie_rx_hi_thread(oal_void *data)
{
    oal_int32 ret = OAL_SUCC;
    oal_int32 supply_num;
    oal_pcie_res* pst_pcie_res = (oal_pcie_res*)data;

    if(OAL_WARN_ON(NULL == pst_pcie_res))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR,"%s error: pst_pcie_res is null",__FUNCTION__);
        return -EFAIL;
    };

    allow_signal(SIGTERM);

    for (;;)
    {
        if (OAL_UNLIKELY(kthread_should_stop()))
        {
            break;
        }

        ret = OAL_WAIT_EVENT_INTERRUPTIBLE(pst_pcie_res->st_rx_hi_wq,
                                        oal_pcie_rx_thread_condtion(&pst_pcie_res->rx_hi_cond));
        if(OAL_UNLIKELY(-ERESTARTSYS == ret))
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "task %s was interrupted by a signal\n", oal_get_current_task_name());
            break;
        }

        mutex_lock(&pst_pcie_res->st_rx_mem_lock);
        if(OAL_UNLIKELY(pst_pcie_res->link_state < PCI_WLAN_LINK_WORK_UP || !pst_pcie_res->regions.inited))
        {
            PCI_PRINT_LOG(PCI_LOG_WARN, "hi thread link invaild, stop supply mem, link_state:%s, region:%d",
                                        oal_pcie_get_link_state_str(pst_pcie_res->link_state),
                                        pst_pcie_res->regions.inited);
        }
        else
        {
            supply_num = oal_pcie_rx_ringbuf_supply(pst_pcie_res, OAL_TRUE, OAL_TRUE, PCIE_RX_RINGBUF_SUPPLY_ALL, GFP_ATOMIC|__GFP_NOWARN, &ret);
            if(OAL_SUCC != ret)
            {
                /*补充内存失败，成功则忽略，有可能当前不需要补充内存也视为成功*/
                oal_pcie_shced_rx_normal_thread(pst_pcie_res);
            }
        }
        mutex_unlock(&pst_pcie_res->st_rx_mem_lock);

    }

    return 0;
}

oal_int32 oal_pcie_rx_normal_thread(oal_void *data)
{
    oal_int32 resched = 0;
    oal_int32 ret = OAL_SUCC;
    oal_int32 supply_num;
    oal_pcie_res* pst_pcie_res = (oal_pcie_res*)data;

    if(OAL_WARN_ON(NULL == pst_pcie_res))
    {
         PCI_PRINT_LOG(PCI_LOG_ERR,"%s error: pst_pcie_res is null",__FUNCTION__);
         return -EFAIL;
    };

    allow_signal(SIGTERM);

    for (;;)
    {
        resched = 0;
        if (OAL_UNLIKELY(kthread_should_stop()))
        {
            break;
        }

        ret = OAL_WAIT_EVENT_INTERRUPTIBLE(pst_pcie_res->st_rx_normal_wq,
                                        oal_pcie_rx_thread_condtion(&pst_pcie_res->rx_normal_cond));
        if(OAL_UNLIKELY(-ERESTARTSYS == ret))
        {
            PCI_PRINT_LOG(PCI_LOG_INFO,"task %s was interrupted by a signal\n", oal_get_current_task_name());
            break;
        }

        mutex_lock(&pst_pcie_res->st_rx_mem_lock);
        if(OAL_UNLIKELY(pst_pcie_res->link_state < PCI_WLAN_LINK_WORK_UP || !pst_pcie_res->regions.inited))
        {
            PCI_PRINT_LOG(PCI_LOG_WARN, "hi thread link invaild, stop supply mem, link_state:%s, region:%d",
                            oal_pcie_get_link_state_str(pst_pcie_res->link_state),
                            pst_pcie_res->regions.inited);
        }
        else
        {
            supply_num = oal_pcie_rx_ringbuf_supply(pst_pcie_res, OAL_TRUE, OAL_TRUE, PCIE_RX_RINGBUF_SUPPLY_ALL, GFP_KERNEL, &ret);
            if(OAL_SUCC != ret)
            {
                resched = 1;
            }
        }

        mutex_unlock(&pst_pcie_res->st_rx_mem_lock);

        if(resched)
        {
            /*补充内存失败，成功则忽略，有可能当前不需要补充内存也视为成功,
              如果GFP_KERNEL 方式补充失败，则启动轮询,循环申请*/
            oal_schedule();
            oal_pcie_shced_rx_normal_thread(pst_pcie_res);
        }
    }

    return 0;
}

oal_void oal_pcie_task_exit(oal_pcie_res* pst_pcie_res)
{
    if(NULL != pst_pcie_res->pst_rx_normal_task)
    {
        oal_thread_stop_etc(pst_pcie_res->pst_rx_normal_task, NULL);
    }

    if(NULL != pst_pcie_res->pst_rx_hi_task)
    {
        oal_thread_stop_etc(pst_pcie_res->pst_rx_hi_task, NULL);
    }
}

oal_int32 oal_pcie_task_init(oal_pcie_res* pst_pcie_res)
{

    OAL_WAIT_QUEUE_INIT_HEAD(&pst_pcie_res->st_rx_hi_wq);
    OAL_WAIT_QUEUE_INIT_HEAD(&pst_pcie_res->st_rx_normal_wq);

    oal_atomic_set(&pst_pcie_res->rx_hi_cond,     0);
    oal_atomic_set(&pst_pcie_res->rx_normal_cond, 0);

    /*高优先级内存用于补充内存 低耗时*/
    pst_pcie_res->pst_rx_hi_task = oal_thread_create_etc(oal_pcie_rx_hi_thread,
                                                    (oal_void*)pst_pcie_res,
                                                    NULL,
                                                    "pci_rx_hi_task",
                                                    SCHED_RR,
                                                    98,
                                                    -1);
    if(NULL == pst_pcie_res->pst_rx_hi_task)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pcie rx hi task create failed!");
        return -OAL_EFAIL;
    }

    /*低优先级线程用于补充内存  高耗时，申请不到就轮询*/
    pst_pcie_res->pst_rx_normal_task = oal_thread_create_etc(oal_pcie_rx_normal_thread,
                                                    (oal_void*)pst_pcie_res,
                                                    NULL,
                                                    "pci_rx_normal_task",
                                                    SCHED_NORMAL,
                                                    -20,
                                                    -1);
    if(NULL == pst_pcie_res->pst_rx_normal_task)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pcie rx normal task create failed!");
        oal_pcie_task_exit(pst_pcie_res);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}

oal_void oal_pcie_sysfs_exit(oal_pcie_res* pst_pcie_res)
{
    if(NULL == g_conn_syfs_pci_object)
        return;
    oal_debug_sysfs_remove_group(g_conn_syfs_pci_object, &hpci_attribute_group);
    kobject_put(g_conn_syfs_pci_object);
    g_conn_syfs_pci_object = NULL;
}

/*原生dma rx完成MSI中断*/
irqreturn_t oal_pcie_edma_rx_intr_status_handler(int irq, void*dev_id)
{
    /*log for msi bbit, del later*/
    OAL_IO_PRINT("pcie_edma_rx_intr_status come, irq:%d\n", irq);
    return IRQ_HANDLED;
}

irqreturn_t oal_pcie_edma_tx_intr_status_handler(int irq, void*dev_id)
{
    OAL_IO_PRINT("pcie_edma_tx_intr_status come, irq:%d\n", irq);
    return IRQ_HANDLED;
}

OAL_STATIC oal_void oal_pcie_hw_edma_intr_status_handler(oal_pcie_res* pst_pci_res)
{
//    oal_int32 total_cnt = 0;
    oal_int32 trans_cnt = 0;
    oal_int32 old_cnt;
    MSG_FIFO_STAT msg_fifo_stat;

    for(;;)
    {

        if(OAL_UNLIKELY(pst_pci_res->link_state < PCI_WLAN_LINK_MEM_UP))
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "link state is disabled:%d, msi can't process!", pst_pci_res->link_state);
            DECLARE_DFT_TRACE_KEY_INFO("pcie mem is not init", OAL_DFT_TRACE_OTHER);
//            return -OAL_ENODEV;
        }

        old_cnt = trans_cnt;
        oal_pcie_mips_start(PCIE_MIPS_RX_FIFO_STATUS);
        msg_fifo_stat.AsDword = oal_readl(pst_pci_res->pst_pci_ctrl_base + PCIE_MSG_FIFO_STATUS);
        oal_pcie_mips_end(PCIE_MIPS_RX_FIFO_STATUS);
        PCI_PRINT_LOG(PCI_LOG_DBG, "msg_fifo_stat host:0x%8x", msg_fifo_stat.AsDword);

        if(0 == msg_fifo_stat.bits.rx_msg_fifo0_empty && 0 == msg_fifo_stat.bits.rx_msg_fifo1_empty)
        {
            oal_pcie_d2h_transfer_done(pst_pci_res);
            trans_cnt++;
        }

        if(0 == msg_fifo_stat.bits.tx_msg_fifo0_empty && 0 == msg_fifo_stat.bits.tx_msg_fifo1_empty)
        {
            oal_pcie_h2d_transfer_done(pst_pci_res);
            trans_cnt++;
        }

        if(old_cnt == trans_cnt)
            break;
        else
        {
            /*这里不能break 否者会导致丢中断*/
            //if(++total_cnt > 5)
            //    break;/*防止中断处理时间过长*/
        }
    }
}

irqreturn_t oal_pcie_hw_edma_rx_intr_status_handler(int irq, void*dev_id)
{
    oal_pcie_res* pst_pci_res = (oal_pcie_res*)dev_id;
    PCI_PRINT_LOG(PCI_LOG_DBG, "oal_pcie_hw_edma_rx_intr_status_handler come, irq:%d\n", irq);
    oal_pcie_hw_edma_intr_status_handler(pst_pci_res);
    return IRQ_HANDLED;
}

irqreturn_t oal_pcie_hw_edma_tx_intr_status_handler(int irq, void*dev_id)
{
    oal_pcie_res* pst_pci_res = (oal_pcie_res*)dev_id;
    PCI_PRINT_LOG(PCI_LOG_DBG, "oal_pcie_hw_edma_tx_intr_status_handler come, irq:%d\n", irq);
    oal_pcie_hw_edma_intr_status_handler(pst_pci_res);
    return IRQ_HANDLED;
}

irqreturn_t oal_device2host_init_handler(int irq, void*dev_id)
{
    OAL_IO_PRINT("oal_device2host_init_handler come, irq:%d\n", irq);
    return IRQ_HANDLED;
}

irqreturn_t oal_pcie_msg_irq_handler(int irq, void*dev_id)
{
    OAL_IO_PRINT("oal_pcie_msg_irq_handler come, irq:%d\n", irq);
    return IRQ_HANDLED;
}

/*msi int callback, should move to oal_pcie_host.c*/
OAL_STATIC oal_irq_handler_t g_msi_110x_callback[] =
{
    oal_pcie_edma_rx_intr_status_handler,
    oal_pcie_edma_tx_intr_status_handler,
    oal_pcie_hw_edma_rx_intr_status_handler,
    oal_pcie_hw_edma_tx_intr_status_handler,
    oal_device2host_init_handler,
    oal_pcie_msg_irq_handler,
};

oal_pcie_res* oal_pcie_host_init(oal_void* data, oal_pcie_msi_stru* pst_msi, oal_uint32 revision)
{
    oal_int32 i;
    oal_int32 ret;
    oal_pcie_res* pst_pci_res;
    if(g_pci_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "PCIe host had already init!\n");
        return NULL;
    }

    if(OAL_NETBUF_CB_SIZE() < sizeof(pcie_cb_dma_res) + sizeof(struct hcc_tx_cb_stru))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pcie cb is too large,[cb %lu < pcie cb %lu + hcc cb %lu]\n",
                        (oal_ulong)OAL_NETBUF_CB_SIZE(),
                        (oal_ulong)sizeof(pcie_cb_dma_res),
                        (oal_ulong)sizeof(struct hcc_tx_cb_stru));
        return NULL;
    }

    pst_pci_res = (oal_pcie_res*)oal_memalloc(sizeof(oal_pcie_res));
    if(NULL == pst_pci_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "alloc pcie res failed, size:%u\n", (oal_uint32)sizeof(oal_pcie_res));
        return NULL;
    }

    OAL_MEMZERO((oal_void*)pst_pci_res, sizeof(oal_pcie_res));
    pst_pci_res->data = data;
    pst_pci_res->revision = revision;

    pst_msi->func = g_msi_110x_callback;
    pst_msi->msi_num = (oal_int32)(OAL_SIZEOF(g_msi_110x_callback)/OAL_SIZEOF(oal_irq_handler_t));

    /*初始化tx/rx队列*/
    oal_spin_lock_init(&pst_pci_res->st_rx_res.lock);
    oal_netbuf_list_head_init(&pst_pci_res->st_rx_res.rxq);

    for(i = 0; i < PCIE_H2D_QTYPE_BUTT; i++)
    {
        oal_atomic_set(&pst_pci_res->st_tx_res[i].tx_ringbuf_sync_cond, 0);
        oal_spin_lock_init(&pst_pci_res->st_tx_res[i].lock);
        oal_netbuf_list_head_init(&pst_pci_res->st_tx_res[i].txq);
    }

    oal_spin_lock_init(&pst_pci_res->st_message_res.d2h_res.lock);
    oal_spin_lock_init(&pst_pci_res->st_message_res.h2d_res.lock);

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE_D2H_BYPASS
    OAL_INIT_COMPLETION(&g_d2h_test_done);
#endif

    ret = oal_pcie_sysfs_init(pst_pci_res);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "oal_pcie_sysfs_init failed, ret=%d\n", ret);
        oal_free(pst_pci_res);
        return NULL;
    }

    mutex_init(&pst_pci_res->st_rx_mem_lock);

    ret = oal_pcie_task_init(pst_pci_res);
    if(OAL_SUCC != ret)
    {
        oal_pcie_sysfs_exit(pst_pci_res);
        oal_free(pst_pci_res);
        return NULL;
    }

    oal_pcie_change_link_state(pst_pci_res, PCI_WLAN_LINK_UP);

    g_pci_res = pst_pci_res;

    return pst_pci_res;

}

oal_void  oal_pcie_host_exit(oal_pcie_res* pst_pci_res)
{
    g_pci_res = NULL;
    oal_pcie_task_exit(pst_pci_res);
    mutex_destroy(&pst_pci_res->st_rx_mem_lock);
    oal_pcie_sysfs_exit(pst_pci_res);
    oal_free(pst_pci_res);
}
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
