

#ifndef __OAL_PCIE_LINUX_H__
#define __OAL_PCIE_LINUX_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include "oal_util.h"
#include "oal_pcie_host.h" 
#ifdef CONFIG_ARCH_KIRIN_PCIE
#include <linux/hisi/pcie-kirin-api.h>
#endif

typedef enum _PCIE_EP_IP_STATUS_
{
    PCIE_EP_IP_POWER_DOWN = 0,
    PCIE_EP_IP_POWER_UP
}PCIE_EP_IP_STATUS;

typedef struct _oal_pcie_linux_res__
{
    oal_uint32              version;    /*pcie version*/
    oal_uint32              revision;   /*1:4.7a  , 2:5.00a*/
    oal_uint32              irq_stat;    /*0:enabled, 1:disabled*/
    oal_spin_lock_stru      st_lock;
    oal_pci_dev_stru*       pst_pcie_dev;/*Linux PCIe device hander*/
    hcc_bus*                pst_bus;
    oal_pcie_res*           pst_pci_res;
    oal_pcie_msi_stru       st_msi;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
	struct pci_saved_state* default_state;
	struct pci_saved_state* state;
#endif
#ifdef CONFIG_ARCH_KIRIN_PCIE
	struct kirin_pcie_register_event pcie_event;
#endif
    oal_tasklet_stru        st_rx_task;

    PCIE_EP_IP_STATUS       power_status;
    oal_atomic              st_pcie_wakeup_flag;

    oal_completion          st_pcie_ready;
    oal_completion          st_pcie_shutdown_response;

    oal_wakelock_stru           st_sr_bugfix;/*暂时规避PCIE S/R失败的问题*/
}oal_pcie_linux_res;
oal_int32 oal_wifi_platform_load_pcie(oal_void);
oal_void oal_wifi_platform_unload_pcie(oal_void);

oal_int32 oal_pcie_set_loglevel(oal_int32 loglevel);
oal_int32 oal_pcie_set_hi11xx_loglevel(oal_int32 loglevel);
oal_int32 oal_pcie_firmware_read(oal_pcie_linux_res *pst_pcie_lres, oal_uint8* buff, oal_int32 len, oal_uint32 timeout);
oal_int32 oal_pcie_firmware_write(oal_pcie_linux_res *pst_pcie_lres, oal_uint8* buff, oal_int32 len);
oal_int32 oal_disable_pcie_irq(oal_pcie_linux_res* pst_pci_lres);
oal_int32 oal_pcie_ip_factory_test(hcc_bus *pst_bus, oal_int32 test_count);
oal_int32 oal_pcie_ip_voltage_bias_init(hcc_bus *pst_bus);
oal_void oal_pcie_chip_info(hcc_bus *pst_bus);
oal_int32 oal_pcie_rc_slt_chip_transfer(hcc_bus *pst_bus, oal_void* ddr_address, 
                                              oal_uint32 data_size, oal_int32 direction);
oal_int32 oal_pcie_ip_init(hcc_bus *pst_bus);
oal_int32 oal_pcie_110x_working_check(oal_void);

#ifdef CONFIG_ARCH_KIRIN_PCIE
extern oal_int32 kirin_rc_idx;
/*hisi kirin PCIe functions*/
extern int kirin_pcie_enumerate(u32 rc_idx);
extern u32 show_link_state(u32 rc_id);
extern int kirin_pcie_pm_control(int resume_flag, u32 rc_idx);
extern int kirin_pcie_lp_ctrl(u32 rc_idx, u32 enable);
/*notify WiFi when RC PCIE power on/off*/
extern int kirin_pcie_power_notifiy_register(u32 rc_idx, int (*poweron)(void* data), int (*poweroff)(void* data), void* data);
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif

