
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#define  HI11XX_LOG_MODULE_NAME "[PCIEL]"
#define  HI11XX_LOG_MODULE_NAME_VAR pciel_loglevel
#define  HISI_LOG_TAG "[PCIEL]"
#include "oal_util.h"

#include "oal_net.h"
#include "oal_ext_if.h"
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
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
#include <linux/pci.h>
#include "board.h"
#include "plat_pm.h"
#endif
#include "hisi_ini.h"
#include "oal_thread.h"
#include "oam_ext_if.h"

#include "oal_pcie_host.h"
#include "oal_pcie_linux.h"
#include "oal_pcie_pm.h"
#include "oal_hcc_host_if.h"

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_PCIE_LINUX_C
extern oal_int32 pcie_ringbuf_bugfix_enable;
extern oal_atomic g_bus_powerup_dev_wait_ack;
OAL_STATIC oal_pcie_linux_res *g_pcie_linux_res = NULL;

oal_completion          g_probe_complete;/*初始化信号量*/
OAL_VOLATILE oal_int32               g_probe_ret;     /*probe 返回值*/

#ifdef CONFIG_ARCH_SD56XX
oal_void* g_pcie_sys_ctrl = NULL;
#endif

oal_int32 g_pcie_enum_fail_reg_dump_flag = 0;


/*1103 MPW2 先使用INTX 中断*/
oal_int32 hipci_msi_enable = 0;  /* 0 -intx 1-pci*/
oal_int32 hipci_gen_select = PCIE_GEN2;
oal_int32 ft_pcie_aspm_check_bypass = 0;
oal_int32 ft_pcie_gen_check_bypass = 0;
oal_int32 hipcie_probe_fail_powerdown_bypass = 0;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
module_param(hipci_msi_enable, int, S_IRUGO | S_IWUSR);
module_param(hipci_gen_select, int, S_IRUGO | S_IWUSR);
module_param(ft_pcie_aspm_check_bypass, int, S_IRUGO | S_IWUSR);
module_param(ft_pcie_gen_check_bypass, int, S_IRUGO | S_IWUSR);
module_param(hipcie_probe_fail_powerdown_bypass, int, S_IRUGO | S_IWUSR);
#endif

#ifdef CONFIG_ARCH_KIRIN_PCIE
oal_int32 kirin_rc_idx = 0;
module_param(kirin_rc_idx, int, S_IRUGO | S_IWUSR);
#endif

oal_int32 pcie_aspm_enable = 1;
module_param(pcie_aspm_enable, int, S_IRUGO | S_IWUSR);

oal_int32 pcie_performance_mode = 0;
module_param(pcie_performance_mode, int, S_IRUGO | S_IWUSR);

oal_int32 pcie_rc_bar_bypass = 1;/*清掉rc bar寄存器*/
module_param(pcie_rc_bar_bypass, int, S_IRUGO | S_IWUSR);

/*Function Declare*/
OAL_STATIC oal_int32  oal_pcie_probe(oal_pci_dev_stru *pst_pci_dev, OAL_CONST oal_pci_device_id_stru *pst_id);
OAL_STATIC hcc_bus*   oal_pcie_bus_init(oal_pcie_linux_res * pst_pci_lres);
OAL_STATIC oal_void oal_pcie_bus_exit(hcc_bus* pst_bus);
OAL_STATIC oal_int32 oal_enable_pcie_irq(oal_pcie_linux_res* pst_pci_lres);
OAL_STATIC oal_int32 oal_pcie_host_ip_init(oal_pcie_linux_res* pst_pci_lres);
oal_int32 oal_pcie_enable_device_func(oal_pcie_linux_res* pst_pci_lres);
oal_int32 oal_pcie_enable_device_func_polling(oal_pcie_linux_res* pst_pci_lres);

oal_int32 oal_pci_wlan_power_on(oal_int32 power_on);
oal_int32 oal_pcie_shutdown_pre_respone(oal_void* data);

/*
 * 2 Global Variable Definition
 */
OAL_STATIC OAL_CONST oal_pci_device_id_stru g_st_pcie_id_table[] =
{
      {0x19e5, 0x1103, PCI_ANY_ID, PCI_ANY_ID }, /* 1103 PCIE */
//    {0x19e5, 0x1181, PCI_ANY_ID, PCI_ANY_ID }, /* 1181 PCIE */
    {0, }
};

oal_int32 oal_pcie_set_loglevel(oal_int32 loglevel)
{
    oal_int32 ret = hipcie_loglevel;
    hipcie_loglevel = loglevel;
    return ret;
}

oal_int32 oal_pcie_set_hi11xx_loglevel(oal_int32 loglevel)
{
    oal_int32 ret = HI11XX_LOG_MODULE_NAME_VAR;
    HI11XX_LOG_MODULE_NAME_VAR = loglevel;
    return ret;
}

oal_int32 oal_pcie_enumerate(oal_void)
{
    oal_int32 ret = -OAL_ENODEV;
#if defined(CONFIG_ARCH_KIRIN_PCIE)
    OAL_IO_PRINT("notify kirin to scan rc:%d\n", kirin_rc_idx);
	ret = kirin_pcie_enumerate(kirin_rc_idx);
#endif
    return ret;
}

oal_int32 oal_pcie_enable_device(oal_pcie_linux_res * pst_pci_lres)
{
    u16 old_cmd = 0;
    oal_int32 ret = -OAL_EFAIL;
    oal_pci_dev_stru * pst_pcie_dev = pst_pci_lres->pst_pcie_dev;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        return -OAL_ENOMEM;
    }

    if(OAL_WARN_ON(NULL == pst_pcie_dev))
    {
        return -OAL_ENODEV;
    }


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))

    if(OAL_WARN_ON(NULL == pst_pci_lres->default_state))
    {
        OAL_IO_PRINT("pcie had't any saved state!\n");
        return -OAL_ENODEV;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0))
	/* Updated with pci_load_and_free_saved_state to compatible
	 * with kernel 3.14 or higher
	 */
	pci_load_and_free_saved_state(pst_pcie_dev, &pst_pci_lres->default_state);

	/*Update default state*//*TBD:应该在初始化完成后保存，这里有可能PCI还不能正常访问*/
	pst_pci_lres->default_state = pci_store_saved_state(pst_pcie_dev);
#else
	pci_load_saved_state(pst_pcie_dev, pst_pci_lres->default_state);
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)) */
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)) */

	pci_restore_state(pst_pcie_dev);

	ret = oal_pci_enable_device(pst_pcie_dev);
	if (ret) {
	    OAL_IO_PRINT("enable device failed!ret=%d\n", ret);
		pci_disable_device(pst_pcie_dev);
	} else {
		pci_set_master(pst_pcie_dev);
	}

	pci_read_config_word(pst_pcie_dev, PCI_COMMAND, &old_cmd);
	OAL_IO_PRINT("PCI_COMMAND:0x%x\n", old_cmd);

	return ret;
}

oal_void oal_pcie_disable_device(oal_pcie_linux_res * pst_pci_lres)
{
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        return;
    }

    if(OAL_WARN_ON(NULL == pst_pci_lres->pst_pcie_dev))
    {
        return;
    }

    oal_pci_disable_device(pst_pci_lres->pst_pcie_dev);
}

oal_int32 oal_pcie_save_default_resource(oal_pcie_linux_res * pst_pci_lres)
{
    oal_int32 ret = OAL_SUCC;

    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        return -OAL_EINVAL;
    }

    if(OAL_WARN_ON(NULL == pst_pci_lres->pst_pcie_dev))
    {
        return -OAL_ENODEV;
    }

    pci_save_state(pst_pci_lres->pst_pcie_dev);
    OAL_IO_PRINT("saven default resource\n");
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
    if(NULL != pst_pci_lres->default_state)
    {
        /*There is already exist pcie state*/
        struct pci_saved_state* pst_state = pst_pci_lres->default_state;
        pst_pci_lres->default_state = NULL;
        kfree(pst_state);
        OAL_IO_PRINT("default state already exist, free first\n");
    }

    pst_pci_lres->default_state = pci_store_saved_state(pst_pci_lres->pst_pcie_dev);
    if(OAL_UNLIKELY(NULL == pst_pci_lres->default_state))
    {
        oal_pci_disable_device(pst_pci_lres->pst_pcie_dev);
        return -OAL_ENOMEM;
    }
#endif
    return ret;
}

irqreturn_t oal_pcie_intx_isr(int irq, void*dev_id)
{
    //oal_task_sched(&((oal_pcie_linux_res*)dev_id)->st_rx_task);
    /*中断处理内容太多，目前无法下移，因为中断需要每次读空 ，而不在中断读的话，
      要先锁住中断，否则中断会狂报*/
    oal_pcie_linux_res * pst_pci_lres = (oal_pcie_linux_res*)dev_id;
    if(OAL_UNLIKELY(oal_pcie_transfer_done(pst_pci_lres->pst_pci_res) < 0))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "froce to disable pcie intx");
        DECLARE_DFT_TRACE_KEY_INFO("transfer_done_fail", OAL_DFT_TRACE_EXCEP);
        oal_disable_pcie_irq(pst_pci_lres);

        /*DFR trigger*/
        oal_pcie_change_link_state(pst_pci_lres->pst_pci_res, PCI_WLAN_LINK_DOWN);
        hcc_bus_exception_submit(pst_pci_lres->pst_bus, WIFI_TRANS_FAIL);

        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_INTX_ISR_PCIE_LINK_DOWN);
        /*关闭低功耗*/

    }
    return IRQ_HANDLED;
}

oal_void oal_pcie_intx_task(oal_ulong data)
{
    oal_pcie_linux_res * pst_pci_lres = (oal_pcie_linux_res*)data;
    oal_pcie_mips_start(PCIE_MIPS_HCC_RX_TOTAL);
    if(oal_pcie_transfer_done(pst_pci_lres->pst_pci_res))
    {
        /*非0说明还需要调度*/
        oal_pcie_mips_end(PCIE_MIPS_HCC_RX_TOTAL);
        oal_task_sched(&pst_pci_lres->st_rx_task);
    }
    oal_pcie_mips_end(PCIE_MIPS_HCC_RX_TOTAL);
}

#ifdef CONFIG_ARCH_SD56XX
irqreturn_t oal_pcie_linkdown(int irq, void*dev_id)
{
    OAL_STATIC oal_uint32  g_linkdown_count = 0;

    if(g_pcie_sys_ctrl)
    {
        /*Clear linkdown Int.*/
        OAL_IO_PRINT("int[0xe8]=0x%8x, [0x38=0x%8x]\n", oal_readl(g_pcie_sys_ctrl + 0xe8), oal_readl(g_pcie_sys_ctrl + 0x38));
        oal_writel(0xffffffff, (g_pcie_sys_ctrl + 0xe8));
        oal_writel(0x0, (g_pcie_sys_ctrl + 0xe8));
        //oal_writel((1<<14), (g_pcie_sys_ctrl + 0xe8));
    }
    else
    {
        OAL_IO_PRINT("clean linkdown failed!\n");
    }
    g_linkdown_count++;
    DECLARE_DFT_TRACE_KEY_INFO("pcie_link_down", OAL_DFT_TRACE_EXCEP);
    return IRQ_HANDLED;
}
#endif

#ifdef CONFIG_ARCH_KIRIN_PCIE
OAL_STATIC oal_void oal_pcie_linkdown_callback(struct kirin_pcie_notify *noti)
{
    oal_pcie_linux_res * pst_pci_lres;
    oal_pci_dev_stru *pst_pci_dev = (oal_pci_dev_stru*)noti->user;
    if(NULL == pst_pci_dev)
    {
        OAL_IO_PRINT(KERN_ERR"pcie linkdown, pci dev is null!!\n");
        return;
    }

    pst_pci_lres = (oal_pcie_linux_res*)oal_pci_get_drvdata(pst_pci_dev);
    if(NULL == pst_pci_lres)
    {
        OAL_IO_PRINT(KERN_ERR"pcie linkdown, lres is null\n");
        return;
    }

    if(NULL == pst_pci_lres->pst_bus)
    {
        OAL_IO_PRINT(KERN_ERR"pcie linkdown, pst_bus is null\n");
        return;
    }

    OAL_IO_PRINT(KERN_ERR"pcie dev[%s] [%d:%d] linkdown\n", dev_name(&pst_pci_dev->dev), pst_pci_dev->vendor, pst_pci_dev->device);
    DECLARE_DFT_TRACE_KEY_INFO("pcie_link_down", OAL_DFT_TRACE_EXCEP);

    oal_pcie_change_link_state(pst_pci_lres->pst_pci_res, PCI_WLAN_LINK_DOWN);

    if(0 == board_get_wlan_wkup_gpio_val_etc())
    {
        hcc_bus_exception_submit(pst_pci_lres->pst_bus, WIFI_TRANS_FAIL);
#ifdef CONFIG_HUAWEI_DSM
        hw_1103_dsm_client_notify(DSM_WIFI_PCIE_LINKDOWN, "%s: pcie linkdown\n", __FUNCTION__);
#endif
        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_WKUP_GPIO_PCIE_LINK_DOWN);
    }
    else
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "dev wakeup gpio is high, dev maybe panic");
    }
}
#endif

oal_void oal_pcie_unregister_msi_by_index(oal_pcie_linux_res* pst_pci_lres, oal_int32 range)
{
    oal_int32 i,j;
    oal_pci_dev_stru *pst_pci_dev;
    pst_pci_dev = pst_pci_lres->pst_pcie_dev;

    j = (range > pst_pci_lres->st_msi.msi_num) ? pst_pci_lres->st_msi.msi_num : range;

    if(NULL == pst_pci_lres->st_msi.func)
    {
        return;
    }

    for(i = 0; i < j; i++)
    {
        oal_irq_handler_t msi_handler = pst_pci_lres->st_msi.func[i];
        if(NULL != msi_handler)
        {
            free_irq(pst_pci_dev->irq + i, (oal_void*)pst_pci_lres->pst_pci_res);
        }
    }
}

oal_void oal_pcie_unregister_msi(oal_pcie_linux_res* pst_pci_lres)
{
    oal_pcie_unregister_msi_by_index(pst_pci_lres, pst_pci_lres->st_msi.msi_num);
}

oal_int32 oal_pcie_register_msi(oal_pcie_linux_res* pst_pci_lres)
{
    oal_int32 i;
    oal_int32 ret;
    oal_pci_dev_stru *pst_pci_dev;

    if(pst_pci_lres->st_msi.msi_num <= 0)
    {
        return -OAL_ENODEV;
    }

    if(NULL == pst_pci_lres->st_msi.func)
    {
        return -OAL_ENODEV;
    }

    pst_pci_dev = pst_pci_lres->pst_pcie_dev;

    for(i = 0; i < pst_pci_lres->st_msi.msi_num; i++)
    {
        oal_irq_handler_t msi_handler = pst_pci_lres->st_msi.func[i];
        if(NULL != msi_handler)
        {
            PCI_PRINT_LOG(PCI_LOG_DBG, "try to request msi irq:%d", pst_pci_dev->irq + i);
            ret = request_irq(pst_pci_dev->irq + i, msi_handler, IRQF_SHARED,
	                "hisi_pci_msi", (oal_void*)pst_pci_lres);
            if(ret)
            {
                OAL_IO_PRINT("msi request irq failed, base irq:%u, msi index:%d, ret=%d\n", pst_pci_dev->irq, i, ret);
                goto failed_request_msi;
            }
        }
    }

    return OAL_SUCC;
failed_request_msi:
    oal_pcie_unregister_msi_by_index(pst_pci_lres, i);
    return ret;
}

/*探测到一个PCIE设备, probe 函数可能会触发多次*/
OAL_STATIC oal_int32  oal_pcie_probe(oal_pci_dev_stru *pst_pci_dev, OAL_CONST oal_pci_device_id_stru *pst_id)
{
    oal_uint8 reg8;
    oal_int32 ret = OAL_SUCC;
    unsigned short device_id;
    hcc_bus* pst_bus;
    oal_pcie_linux_res * pst_pci_lres;

    if((OAL_PTR_NULL == pst_pci_dev) || (OAL_PTR_NULL == pst_id))
    {
        /*never touch here*/
        OAL_IO_PRINT("oal_pcie_probe failed, pst_pci_dev:%p, pst_id:%p\n", pst_pci_dev, pst_id);
        g_probe_ret = -OAL_EIO;
        OAL_COMPLETE(&g_probe_complete);
        return -OAL_EIO;
    }

    device_id = OAL_PCI_GET_DEV_ID(pst_pci_dev);

#if 0
    /*设备ID 和 产品ID*/
    OAL_IO_PRINT("[PCIe][%s]devfn:0x%x , vendor:0x%x , device:0x%x , subsystem_vendor:0x%x , subsystem_device:0x%x , class:0x%x \n",
                dev_name(&pst_pci_dev->dev),
                pst_pci_dev->devfn,
                pst_pci_dev->vendor,
                pst_pci_dev->device,
                pst_pci_dev->subsystem_vendor,
                pst_pci_dev->subsystem_device,
                pst_pci_dev->class);
#endif

    /*TBD:TBC 暂时只考虑一个PCIE设备*/
    if(g_pcie_linux_res)
    {
        OAL_IO_PRINT("pcie device init failed, already init!\n");
        g_probe_ret = -OAL_EBUSY;
        OAL_COMPLETE(&g_probe_complete);
        return -OAL_EBUSY;
    }

    ret = oal_pci_enable_device(pst_pci_dev);
    if(ret)
    {
        OAL_IO_PRINT("pci: pci_enable_device error ret=%d\n", ret);
        g_probe_ret = ret;
        OAL_COMPLETE(&g_probe_complete);
        return ret;
    }

    /*alloc pcie resources*/

    pst_pci_lres = (oal_pcie_linux_res*)oal_memalloc(sizeof(oal_pcie_linux_res));
    if(NULL == pst_pci_lres)
    {
        ret = -OAL_ENOMEM;
        OAL_IO_PRINT("%s alloc res failed,size:%lu\n", __FUNCTION__, (oal_ulong)sizeof(oal_pcie_linux_res));
        goto failed_res_alloc;
    }

    OAL_MEMZERO((oal_void*)pst_pci_lres, sizeof(oal_pcie_linux_res));

    pst_pci_lres->pst_pcie_dev = pst_pci_dev;

    pci_set_drvdata(pst_pci_dev, pst_pci_lres);

    OAL_INIT_COMPLETION(&pst_pci_lres->st_pcie_ready);
    oal_atomic_set(&pst_pci_lres->st_pcie_wakeup_flag, 0);

    oal_task_init(&pst_pci_lres->st_rx_task, oal_pcie_intx_task, (oal_uint)pst_pci_lres);

    oal_spin_lock_init(&pst_pci_lres->st_lock);

    OAL_INIT_COMPLETION(&pst_pci_lres->st_pcie_shutdown_response);

    oal_wake_lock_init(&pst_pci_lres->st_sr_bugfix, "pcie_sr_bugfix");

    pst_pci_lres->version = (pst_pci_dev->vendor) | (pst_pci_dev->device << 16);

    ret = oal_pci_read_config_byte(pst_pci_dev, PCI_REVISION_ID, &reg8);
    if(ret)
    {
        OAL_IO_PRINT("pci: read revision id  failed, ret=%d\n", ret);
        ret = -OAL_ENODEV;
        goto failed_pci_host_init;
    }

    pst_pci_lres->revision = (oal_uint32)reg8;

    PCI_PRINT_LOG(PCI_LOG_INFO, "wifi pcie version:0x%8x, revision:%d", pst_pci_lres->version, pst_pci_lres->revision);

    /*soft resource init*/
    pst_pci_lres->pst_pci_res= oal_pcie_host_init((void*)pst_pci_dev, &pst_pci_lres->st_msi, pst_pci_lres->revision);
    if(NULL == pst_pci_lres->pst_pci_res)
    {
        ret = -OAL_ENODEV;
        OAL_IO_PRINT("pci: oal_pcie_host_init failed\n");
        goto failed_pci_host_init;
    }

    pst_bus = oal_pcie_bus_init(pst_pci_lres);
    if(NULL == pst_bus)
    {
        goto failed_pci_bus_init;
    }

    hcc_bus_message_register(pst_bus, D2H_MSG_SHUTDOWN_IP_PRE_RESPONSE, oal_pcie_shutdown_pre_respone, (oal_void*)pst_pci_lres);

    /*硬件设备资源初始化, 5610+1103 FPGA 没有上下电接口*/
    ret = oal_pcie_dev_init(pst_pci_lres->pst_pci_res);
    if(OAL_SUCC != ret)
    {
        goto failed_pci_dev_init;
    }

    /*interrupt register*/
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    OAL_IO_PRINT("raw irq: %d\n", pst_pci_dev->irq);

    /*Read from DTS later*/
    pst_pci_lres->st_msi.is_msi_support = hipci_msi_enable;

    if(pst_pci_lres->st_msi.is_msi_support)
    {
        /*Try to enable msi*/
        ret = pci_enable_msi(pst_pci_dev);
        if(ret > 0)
        {
            /*ep support msi*/
            OAL_IO_PRINT("msi irq:%d, msi int nums:%d\n", pst_pci_dev->irq, ret);
            ret = oal_pcie_register_msi(pst_pci_lres);
            if(OAL_SUCC != ret)
            {
                goto failed_register_msi;
            }
            else
            {
                pst_pci_lres->irq_stat = 0;
            }
        }
        else
        {
            /*non msi drivers*/
            OAL_IO_PRINT("msi request failed, disable msi... ret=%d\n", ret);
            pst_pci_lres->st_msi.is_msi_support = 0;
        }
    }

    /*Try to register intx*/
    if(!pst_pci_lres->st_msi.is_msi_support)
    {
        ret = request_irq(pst_pci_dev->irq, oal_pcie_intx_isr, IRQF_SHARED,
    	                "hisi_pci_intx", (oal_void*)pst_pci_lres);
        if(ret < 0)
        {
            OAL_IO_PRINT("\n");
            goto failed_pci_intx_request;
        }

        pst_pci_lres->irq_stat = 0;
    }

#endif

    OAL_IO_PRINT("request pcie intx irq %d succ\n", pst_pci_dev->irq);

#ifdef CONFIG_ARCH_SD56XX
    g_pcie_sys_ctrl = oal_ioremap_nocache(0x10100000, 0x1000);
    ret = request_irq((69 + 32), oal_pcie_linkdown, IRQF_SHARED,
	                "5610_pci0_linkdown", (oal_void*)pst_pci_lres);
    if(ret < 0)
    {
        goto failed_pci_intx_request;
    }
#endif

#ifdef CONFIG_ARCH_KIRIN_PCIE
	pst_pci_lres->pcie_event.events = KIRIN_PCIE_EVENT_LINKDOWN;
	pst_pci_lres->pcie_event.user = pst_pci_dev;
	pst_pci_lres->pcie_event.mode = KIRIN_PCIE_TRIGGER_CALLBACK;
	pst_pci_lres->pcie_event.callback = oal_pcie_linkdown_callback;
	kirin_pcie_register_event(&pst_pci_lres->pcie_event);
#endif /* KIRIN_PCIE_LINKDOWN_RECOVERY */

    g_pcie_linux_res = pst_pci_lres;
    g_probe_ret = ret;
    OAL_COMPLETE(&g_probe_complete);

    return ret;
    /*fail process*/
failed_pci_intx_request:
    if(pst_pci_lres->st_msi.is_msi_support)
    {
        oal_pcie_unregister_msi(pst_pci_lres);
    }
failed_register_msi:
    oal_pcie_dev_deinit(pst_pci_lres->pst_pci_res);

failed_pci_dev_init:
    oal_pcie_bus_exit(pst_bus);
failed_pci_bus_init:
    oal_pcie_host_exit(pst_pci_lres->pst_pci_res);
failed_pci_host_init:
    pci_set_drvdata(pst_pci_dev, NULL);
    oal_wake_lock_exit(&pst_pci_lres->st_sr_bugfix);
    oal_free(pst_pci_lres);
failed_res_alloc:
    oal_pci_disable_device(pst_pci_dev);
    g_probe_ret = ret;
    OAL_COMPLETE(&g_probe_complete);
    return ret;

}

OAL_STATIC oal_void  oal_pcie_remove(oal_pci_dev_stru *pst_pci_dev)
{
    unsigned short device_id;
    oal_pcie_linux_res * pst_pci_lres;

    device_id = OAL_PCI_GET_DEV_ID(pst_pci_dev);

    OAL_IO_PRINT("pcie driver remove 0x%x, name:%s\n", device_id, dev_name(&pst_pci_dev->dev));

    pst_pci_lres = (oal_pcie_linux_res*)oal_pci_get_drvdata(pst_pci_dev);
    if(NULL == pst_pci_lres)
    {
        OAL_IO_PRINT("oal_pcie_remove lres is null\n");
        return;
    }

    g_pcie_linux_res = NULL;

#ifdef CONFIG_ARCH_KIRIN_PCIE
    kirin_pcie_deregister_event(&pst_pci_lres->pcie_event);
#endif

#ifdef CONFIG_ARCH_SD56XX
    if(g_pcie_sys_ctrl)
    {
        oal_iounmap(g_pcie_sys_ctrl);
    }
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if(pst_pci_lres->st_msi.is_msi_support)
    {
        oal_pcie_unregister_msi(pst_pci_lres);
    }
    else
    {
        free_irq(pst_pci_dev->irq, pst_pci_lres);
    }

#endif

    oal_wake_lock_exit(&pst_pci_lres->st_sr_bugfix);

    oal_task_kill(&pst_pci_lres->st_rx_task);

    oal_pcie_dev_deinit(pst_pci_lres->pst_pci_res);

    oal_pcie_host_exit(pst_pci_lres->pst_pci_res);

    pst_pci_lres->pst_pcie_dev = NULL;
    oal_free(pst_pci_lres);

    oal_pci_disable_device(pst_pci_dev);

}

#ifdef CONFIG_ARCH_KIRIN_PCIE
OAL_STATIC oal_int32 oal_pcie_device_wakeup_handler(void* data)
{
    OAL_REFERENCE(data);
    /*这里保证解复位EP控制器时efuse已经稳定*/
    board_host_wakeup_dev_set(1);
    /*TBD:TBC ,
      dev wakeup host 被复用成了panic消息，这里需要处理*/
    //oal_msleep(25);/*这里要用GPIO 做ACK 延迟不可靠, MPW2 硬件唤醒15ms,软件6ms*/
    PCI_PRINT_LOG(PCI_LOG_INFO, "pcie wakeup device control, pull up gpio");
    return 0;
}

OAL_STATIC oal_int32 oal_pcie_device_wake_powerup_handler(void* data)
{
    oal_uint32 ul_ret;
    oal_uint32 ul_retry_cnt = 0;
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();
    OAL_REFERENCE(data);
    DECLARE_DFT_TRACE_KEY_INFO("pcie_resume_powerup", OAL_DFT_TRACE_SUCC);
retry:
    PCI_PRINT_LOG(PCI_LOG_INFO, "pcie_resume_powerup pull up gpio,ul_retry_cnt=%u", ul_retry_cnt);
    if(NULL != pst_wlan_pm)
    {
        OAL_INIT_COMPLETION(&pst_wlan_pm->st_wifi_powerup_done);
    }

    oal_atomic_set(&g_bus_powerup_dev_wait_ack,1);
    /*这里保证解复位EP控制器时efuse已经稳定*/
    board_host_wakeup_dev_set(1);
    if(NULL != pst_wlan_pm)
    {
        ul_ret = oal_wait_for_completion_timeout(&pst_wlan_pm->st_wifi_powerup_done, (oal_uint32)OAL_MSECS_TO_JIFFIES(2000));
        if(OAL_UNLIKELY(0 == ul_ret))
        {
            /*超时不做处理继续尝试建链*/
            DECLARE_DFT_TRACE_KEY_INFO("pcie_resume_powerup ack timeout", OAL_DFT_TRACE_FAIL);
            if(HI1XX_ANDROID_BUILD_VARIANT_USER != hi11xx_get_android_build_variant())
            {
                /*eng mode*/
                ssi_dump_device_regs(SSI_MODULE_MASK_COMM|SSI_MODULE_MASK_PCIE_CFG);
            }

            if(ul_retry_cnt++ < 3)
            {
                PCI_PRINT_LOG(PCI_LOG_INFO, "pull down wakeup gpio and retry");
                board_host_wakeup_dev_set(0);
                oal_msleep(5);
                goto retry;
            }
            else
            {
                DECLARE_DFT_TRACE_KEY_INFO("pcie_resume_powerup_ack_timeout_retry_failed", OAL_DFT_TRACE_FAIL);
                if(HI1XX_ANDROID_BUILD_VARIANT_USER == hi11xx_get_android_build_variant())
                {
                    /*user mode*/
                    if(oal_print_rate_limit(PRINT_RATE_HOUR))
                    {
                        if(true == bfgx_is_shutdown_etc())
                        {
                            PCI_PRINT_LOG(PCI_LOG_INFO, "bfgx is shutdown");
                            ssi_dump_device_regs(SSI_MODULE_MASK_ARM_REG|SSI_MODULE_MASK_AON_CUT|SSI_MODULE_MASK_PCIE_CUT);
                        }
                        else
                        {
                            PCI_PRINT_LOG(PCI_LOG_INFO, "bfgx is working");
                            ssi_dump_device_regs(SSI_MODULE_MASK_ARM_REG);
                        }
                    }
                }
            }

        }
        else
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "powerup done");
        }
    }
    else
    {
        oal_msleep(100);/*这里要用GPIO 做ACK 延迟不可靠, S/R 唤醒 时间较长*/
    }

    oal_atomic_set(&g_bus_powerup_dev_wait_ack,0);

    return 0;
}

OAL_STATIC oal_int32 oal_pcie_device_suspend_handler(void* data)
{
    oal_pcie_linux_res * pst_pci_lres = (oal_pcie_linux_res*)data;
#if 0
    /*走到这里说明wakelock已经释放，WIFI已经深睡,通知RC/EP下电，
      发送TurnOff Message*/
    /*下电之前关闭 PCIE HOST 控制器*/
    kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL, NULL, NULL);
    kirin_pcie_pm_control(0, kirin_rc_idx);
#else
    /*下电在麒麟代码中处理
      kirin_pcie_suspend_noirq,
      无法判断turnoff 是否成功发送*/
#endif

    /*此处不一定是真的下电了*/
    pst_pci_lres->power_status = PCIE_EP_IP_POWER_DOWN;

    oal_pcie_change_link_state(pst_pci_lres->pst_pci_res, PCI_WLAN_LINK_DEEPSLEEP);

    PCI_PRINT_LOG(PCI_LOG_INFO, "pcie_suspend_powerdown");
    DECLARE_DFT_TRACE_KEY_INFO("pcie_suspend_powerdown", OAL_DFT_TRACE_SUCC);
#ifdef CONFIG_ARCH_KIRIN_PCIE
    kirin_pcie_power_notifiy_register(kirin_rc_idx, oal_pcie_device_wake_powerup_handler,
                                            NULL, NULL);
#endif

    return 0;
}
#endif

OAL_STATIC oal_int32 oal_pcie_suspend(oal_pci_dev_stru *pst_pci_dev,oal_pm_message_t state)
{
    hcc_bus* pst_bus;
    struct hcc_handler* pst_hcc;
    oal_pcie_linux_res * pst_pci_lres = (oal_pcie_linux_res*)oal_pci_get_drvdata(pst_pci_dev);

    if(NULL == pst_pci_lres)
    {
        OAL_IO_PRINT("pcie_suspend lres is null\n");
        return 0;
    }

    pst_bus = pst_pci_lres->pst_bus;
    if(NULL == pst_bus)
    {
        OAL_IO_PRINT(KERN_ERR"enter oal_pcie_suspend\n");
        return -OAL_ENODEV;
    }

    if(NULL == HBUS_TO_DEV(pst_bus))
    {
        OAL_IO_PRINT("suspend,pcie is not work...\n");
        return OAL_SUCC;
    }

    pst_hcc = HBUS_TO_HCC(pst_bus);
    if(NULL == pst_hcc)
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "pcie dev's hcc handler is null!");
        return OAL_SUCC;
    }

    if(pst_bus != HDEV_TO_HBUS(HBUS_TO_DEV(pst_bus)))
    {
        /*pcie非当前接口*/
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie is not current bus, return");
        return OAL_SUCC;
    }

    if(HCC_ON != oal_atomic_read(&pst_hcc->state))
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "wifi is closed");
        return OAL_SUCC;
    }

    PCI_PRINT_LOG(PCI_LOG_INFO, "oal_pcie_suspend");

    if (down_interruptible(&pst_bus->sr_wake_sema))
    {
        OAL_IO_PRINT(KERN_ERR"pcie_wake_sema down failed.");
        return -OAL_EFAIL;
    }

    if (hcc_bus_wakelock_active(pst_bus))
    {
        /* has wake lock so stop controller's suspend,
         * otherwise controller maybe error while sdio reinit*/
        OAL_IO_PRINT(KERN_ERR"Already wake up");
        up(&pst_bus->sr_wake_sema);
        return -OAL_EFAIL;
    }

    wlan_pm_wkup_src_debug_set(OAL_TRUE);

    DECLARE_DFT_TRACE_KEY_INFO("pcie_android_suspend", OAL_DFT_TRACE_SUCC);

    oal_pcie_save_default_resource(pst_pci_lres);
#ifdef CONFIG_ARCH_KIRIN_PCIE
    kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL,
                                            oal_pcie_device_suspend_handler, (void*)pst_pci_lres);
#endif
    return 0;
}

OAL_STATIC oal_int32 oal_pcie_resume(oal_pci_dev_stru *pst_pci_dev)
{
    oal_int32 ret;
    oal_uint32      version = 0x0;
    hcc_bus*        pst_bus;
    struct hcc_handler* pst_hcc;
    oal_pcie_linux_res * pst_pci_lres = (oal_pcie_linux_res*)oal_pci_get_drvdata(pst_pci_dev);
    if(NULL == pst_pci_lres)
    {
        OAL_IO_PRINT("pcie_resume lres is null\n");
        return 0;
    }

    pst_bus = pst_pci_lres->pst_bus;
    if(NULL == pst_bus)
    {
        OAL_IO_PRINT(KERN_ERR"enter oal_pcie_suspend\n");
        return -OAL_ENODEV;
    }

    if(NULL == HBUS_TO_DEV(pst_bus))
    {
        OAL_IO_PRINT("resume,pcie is not work...\n");
        return OAL_SUCC;
    }

    pst_hcc = HBUS_TO_HCC(pst_bus);
    if(NULL == pst_hcc)
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "pcie dev's hcc handler is null!");
        return OAL_SUCC;
    }

    if(pst_bus != HDEV_TO_HBUS(HBUS_TO_DEV(pst_bus)))
    {
        /*pcie非当前接口*/
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie is not current bus, return");
        return OAL_SUCC;
    }

    if(HCC_ON != oal_atomic_read(&pst_hcc->state))
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "wifi is closed");
        return OAL_SUCC;
    }

    DECLARE_DFT_TRACE_KEY_INFO("pcie_android_resume", OAL_DFT_TRACE_SUCC);

    if(PCIE_EP_IP_POWER_DOWN != pst_pci_lres->power_status)
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "oal_pcie_resume");
        up(&pst_bus->sr_wake_sema);
        return OAL_SUCC;
    }

    ret = oal_pci_read_config_dword(pst_pci_lres->pst_pcie_dev, 0x0, &version);
    if(ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "read pci version failed ret=%d", ret);
        oal_msleep(1000);
        ret = oal_pci_read_config_dword(pst_pci_lres->pst_pcie_dev, 0x0, &version);
        if(ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "read pci version failed ret=%d", ret);
        }
        else
        {
            PCI_PRINT_LOG(PCI_LOG_WARN, "version:0x%x is not match with:0x%x", version , pst_pci_lres->version);
        }
        up(&pst_bus->sr_wake_sema);
        hcc_bus_exception_submit(pst_pci_lres->pst_bus, WIFI_TRANS_FAIL);

        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_RESUME_FIRMWARE_DOWN);
        return OAL_SUCC;
    }

    if(version != pst_pci_lres->version)
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "version:0x%x is not match with:0x%x", version , pst_pci_lres->version);
        //return 0;
    }

    board_host_wakeup_dev_set(0);

    up(&pst_bus->sr_wake_sema);
    return 0;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
OAL_STATIC oal_void oal_pcie_shutdown(oal_pci_dev_stru *pst_pcie_dev)
{
    oal_int32 ret;
    oal_pcie_linux_res* pst_pci_lres = (oal_pcie_linux_res*)oal_pci_get_drvdata(pst_pcie_dev);
    if(NULL == pst_pci_lres)
    {
        return;
    }

#ifdef CONFIG_ARCH_SD56XX
    return;
#else

    /*system shutdown, should't write sdt file*/
    oam_set_output_type_etc(OAM_OUTPUT_TYPE_CONSOLE);

    if(NULL == pst_pci_lres->pst_bus)
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "pcie bus is null");
        return;
    }

    oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_pci_lres->pst_bus), OAL_FALSE);
    oal_disable_pcie_irq(pst_pci_lres);

    /*power off wifi*/
    ret = wlan_power_off_etc();
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "wifi shutdown failed, ret=%d", ret);
    }
#endif
}

OAL_STATIC oal_pci_driver_stru g_st_pcie_drv =
{
    .name       = "hi110x_pci",
    .id_table   = g_st_pcie_id_table,
    .probe      = oal_pcie_probe,
    .remove     = oal_pcie_remove,
    .suspend    = oal_pcie_suspend,
    .resume     = oal_pcie_resume,
    .shutdown   = oal_pcie_shutdown,
};
#elif (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
OAL_STATIC oal_pci_driver_stru g_st_pcie_drv =
{
    "hi110x_pci",
    g_st_pcie_id_table,
    oal_pcie_probe,
    oal_pcie_remove
};
#endif

OAL_STATIC oal_int32 oal_pcie_get_state(hcc_bus *pst_bus, oal_uint32 mask)
{
    oal_pcie_linux_res * pst_pci_lres;

    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    OAL_REFERENCE(pst_pci_lres);

    return OAL_TRUE;
}

OAL_STATIC oal_void oal_enable_pcie_state(hcc_bus* pst_bus, oal_uint32 mask)
{
    oal_pcie_linux_res * pst_pci_lres;

    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    OAL_REFERENCE(pst_pci_lres);
}

OAL_STATIC oal_void oal_disable_pcie_state(hcc_bus* pst_bus, oal_uint32 mask)
{
    oal_pcie_linux_res * pst_pci_lres;

    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    OAL_REFERENCE(pst_pci_lres);
}

OAL_STATIC oal_int32 oal_pcie_rx_netbuf(hcc_bus *pst_bus, oal_netbuf_head_stru* pst_head)
{
    oal_pcie_linux_res * pst_pci_lres;

    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    OAL_REFERENCE(pst_pci_lres);
    return OAL_SUCC;
}

OAL_STATIC PCIE_H2D_RINGBUF_QTYPE oal_pcie_hcc_qtype_to_pci_qtype(hcc_netbuf_queue_type qtype)
{
    return (HCC_NETBUF_HIGH_QUEUE == qtype) ? PCIE_H2D_QTYPE_HIGH : PCIE_H2D_QTYPE_NORMAL;
}

OAL_STATIC oal_int32 oal_pcie_tx_netbuf(hcc_bus *pst_bus, oal_netbuf_head_stru* pst_head, hcc_netbuf_queue_type qtype)
{
    oal_int32 ret;
    oal_pcie_linux_res * pst_pci_lres;
    struct hcc_handler* pst_hcc;

    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        OAL_IO_PRINT("pcie tx netbuf failed, lres is null\n");
        hcc_tx_netbuf_list_free(pst_head);
        return 0;
    }
    pst_hcc = HBUS_TO_HCC(pst_bus);
    if(NULL == pst_hcc)
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "pcie dev's hcc handler is null!");
        hcc_tx_netbuf_list_free(pst_head);
        return 0;
    }

    if(OAL_UNLIKELY(HCC_ON != oal_atomic_read(&pst_hcc->state)))
    {
        /*drop netbuf list, wlan close or exception*/
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "drop pcie netbuflist %u", oal_netbuf_list_len(pst_head));
        hcc_tx_netbuf_list_free(pst_head);
        return 0;
    }

    ret = oal_pcie_send_netbuf_list(pst_pci_lres->pst_pci_res, pst_head, oal_pcie_hcc_qtype_to_pci_qtype(qtype));
    return ret;
}

oal_int32 oal_pcie_send_message2dev(oal_pcie_linux_res * pst_pci_lres, oal_uint32 message)
{
    hcc_bus *       pst_bus;
    oal_int32       ret      = OAL_SUCC;
    pst_bus = pst_pci_lres->pst_bus;

    if(OAL_WARN_ON(NULL == pst_bus))
    {
        return -OAL_ENODEV;
    }

    PCI_PRINT_LOG(PCI_LOG_DBG, "send msg to device [0x%8x]\n", (oal_uint32)message);

    hcc_bus_wake_lock(pst_bus);
    ret = oal_pcie_send_message_to_dev(pst_pci_lres->pst_pci_res, message);
    hcc_bus_wake_unlock(pst_bus);

    return ret;
}

OAL_STATIC oal_int32 oal_pcie_send_msg(hcc_bus *pst_bus, oal_uint32 val)
{
    oal_pcie_linux_res * pst_pci_lres;

    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;

    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        return -OAL_ENODEV;
    }

    if(val >= H2D_MSG_COUNT)
    {
        OAL_IO_PRINT("[Error]invaild param[%u]!\n", val);
        return -OAL_EINVAL;
    }

    return oal_pcie_send_message2dev(pst_pci_lres, (oal_uint32)val);
}

OAL_STATIC oal_int32 oal_pcie_bindcpu(hcc_bus *pst_bus, oal_uint32 chan, oal_int32 is_bind)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_ulong cpu;
#if defined(CONFIG_ARCH_HISI)
    struct cpumask slow_cpus, fast_cpus;
#endif
    oal_pcie_linux_res *pst_pci_lres;
    oal_pci_dev_stru   *pst_pci_dev;

    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    if(OAL_PTR_NULL == pst_pci_lres)
    {
        return OAL_SUCC;
    }

    pst_pci_dev = pst_pci_lres->pst_pcie_dev;
    if(OAL_PTR_NULL == pst_pci_dev)
    {
        return OAL_SUCC;
    }


#if defined(CONFIG_ARCH_HISI)
    hisi_get_slow_cpus(&slow_cpus);
    hisi_get_fast_cpus(&fast_cpus);
#endif

    if(is_bind)
    {
#ifdef CONFIG_NR_CPUS
#if CONFIG_NR_CPUS > OAL_BUS_HPCPU_NUM
        cpu = OAL_BUS_HPCPU_NUM;
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "pcie bindcpu irq =%u, bind to cpu %lu", pst_pci_dev->irq, cpu);
        irq_set_affinity_hint(pst_pci_dev->irq, cpumask_of(cpu));
#else
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "pcie bindcpu fail, cpu nums:%d irq =%u",  CONFIG_NR_CPUS, pst_pci_dev->irq);
#endif
#endif

#if defined(CONFIG_ARCH_HISI)
        if(pst_pci_lres->pst_pci_res->pst_rx_hi_task)
            set_cpus_allowed_ptr(pst_pci_lres->pst_pci_res->pst_rx_hi_task, &fast_cpus);
#endif
    }
    else
    {
#ifdef CONFIG_NR_CPUS
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "pcie unbind cpu irq =%u", pst_pci_dev->irq);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
        /*2.6.35-rc1*/
        irq_set_affinity_hint(pst_pci_dev->irq, cpumask_of(0));
#endif
#if defined(CONFIG_ARCH_HISI)
        if(pst_pci_lres->pst_pci_res->pst_rx_hi_task)
            set_cpus_allowed_ptr(pst_pci_lres->pst_pci_res->pst_rx_hi_task, &slow_cpus);
#endif
#endif
    }
#endif
    return OAL_SUCC;
}

oal_int32 oal_pcie_get_trans_count(hcc_bus *hi_bus, oal_uint64 *tx, oal_uint64 *rx)
{
    oal_pcie_linux_res* pst_pci_lres = (oal_pcie_linux_res*)hi_bus->data;
    return oal_pcie_get_host_trans_count(pst_pci_lres->pst_pci_res, tx, rx);
}

oal_int32 oal_pcie_shutdown_pre_respone(oal_void* data)
{
    oal_pcie_linux_res* pst_pci_lres = (oal_pcie_linux_res*)data;
    OAL_IO_PRINT("oal_pcie_shutdown_pre_respone\n");
    OAL_COMPLETE(&pst_pci_lres->st_pcie_shutdown_response);
    return OAL_SUCC;
}

oal_int32 oal_pcie_switch_clean_res(hcc_bus* pst_bus)
{
    oal_int32 ret;
    /*清空PCIE 通道，通知Device关闭发送通道，
      等待DMA完成所有传输后返回*/

    oal_pcie_linux_res* pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;

    OAL_INIT_COMPLETION(&pst_pci_lres->st_pcie_shutdown_response);

    ret = oal_pcie_send_msg(pst_bus, H2D_MSG_SHUTDOWN_IP_PRE);
    if(ret)
    {
        OAL_IO_PRINT("shutdown pre message send failed=%d\n", ret);
        return ret;
    }

    /*wait shutdown response*/
    ret = oal_wait_for_completion_timeout(&pst_pci_lres->st_pcie_shutdown_response, (oal_uint32)OAL_MSECS_TO_JIFFIES(10000));
    if(0 == ret)
    {
        OAL_IO_PRINT("wait pcie shutdown response timeout\n");
        return -OAL_ETIMEDOUT;
    }

    return OAL_SUCC;;
}

OAL_STATIC oal_int32 oal_pcie_deinit(hcc_bus *pst_bus)
{
    return OAL_SUCC;
}

/*reinit pcie ep control*/
OAL_STATIC oal_int32 oal_pcie_reinit(hcc_bus *pst_bus)
{
    oal_int32 ret;
    oal_uint32 version = 0x0;
    oal_pcie_linux_res * pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    declare_time_cost_stru(reinit);

    oal_get_time_cost_start(reinit);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "wake_sema_count=%d", pst_bus->sr_wake_sema.count);
    sema_init(&pst_bus->sr_wake_sema, 1);/*S/R信号量*/

    hcc_bus_disable_state(pst_bus, OAL_BUS_STATE_ALL);
    ret = oal_pcie_enable_device(pst_pci_lres);
    if(OAL_SUCC == ret)
    {
        /*需要在初始化完成后打开*/
        //hcc_bus_enable_state(pst_bus, OAL_BUS_STATE_ALL);
    }

    /*check link status*/
    if(NULL != pst_pci_lres->pst_pcie_dev)
    {
        ret = oal_pci_read_config_dword(pst_pci_lres->pst_pcie_dev, 0x0, &version);
        if(ret)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "read pci version failed, enable device failed, ret=%d, version=0x%x", ret, version);
            DECLARE_DFT_TRACE_KEY_INFO("pcie_enable_device_reinit_fail", OAL_DFT_TRACE_FAIL);
            return -OAL_ENODEV;;
        }

        if(version != pst_pci_lres->version)
        {
            PCI_PRINT_LOG(PCI_LOG_WARN, "version:0x%x is not match with:0x%x", version , pst_pci_lres->version);
        }
        else
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "pcie enable device check succ");
        }
    }

    /*初始化PCIE资源*/
    ret = oal_pcie_enable_regions(pst_pci_lres->pst_pci_res);
    if(ret)
    {
        OAL_IO_PRINT(KERN_ERR"enable regions failed, ret=%d\n", ret);
        return ret;
    }

    ret = oal_pcie_iatu_init(pst_pci_lres->pst_pci_res);
    if(ret)
    {
        OAL_IO_PRINT(KERN_ERR"iatu init failed, ret=%d\n", ret);
        return ret;
    }

    oal_get_time_cost_end(reinit);
    oal_calc_time_cost_sub(reinit);
    PCI_PRINT_LOG(PCI_LOG_INFO, "pcie reinit cost %llu us", time_cost_var_sub(reinit));
    return ret;
}

oal_int32 oal_pci_wlan_power_on(oal_int32 power_on)
{
#ifdef CONFIG_ARCH_SD56XX
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

        /*2.设置数据方向，配置寄存器0x10108004第23bit为1 */
        ul_val =  oal_readl(pst_gpio_base + 0x4);
        ul_val |= BIT23;
        oal_writel(ul_val, pst_gpio_base + 0x4);


        /*3.设置GPIO87拉低，GPIO芯片1下电 */
        ul_val =  oal_readl(pst_gpio_base + 0x0);
        ul_val &= ~BIT23;
        oal_writel(ul_val, pst_gpio_base + 0x0);

        if(OAL_TRUE == power_on)
        {
            //oal_udelay(10);
            oal_msleep(100);

            /*4.设置GPIO87拉高，GPIO芯片1上电 */
            ul_val =  oal_readl(pst_gpio_base + 0x0);
            ul_val |= BIT23;
            oal_writel(ul_val, pst_gpio_base + 0x0);
            OAL_IO_PRINT("wlan power on\n");
        }
    }

    iounmap(pst_gpio_base);
    iounmap(pst_gpio_mode);
    return OAL_SUCC;
#else
    return -OAL_ENODEV;
#endif
}
#ifdef CONFIG_ARCH_SD56XX
extern int sd56xx_pcie_enumerate(int is_enum);
#endif
#ifdef CONFIG_ARCH_SD56XX
oal_int32 oal_pcie_check_link_up(oal_void)
{
#define TRY_MAX 2000000
	unsigned int val;
	unsigned int retry_count = 0;
	unsigned int loop = 0;
	unsigned int link_up_stable_counter = 0;
    void* __iomem pcie_sys_base_virt   = NULL;
    /*等待建链*/
    pcie_sys_base_virt = ioremap_nocache(0x10100000, 0x1000);
    if(NULL == pcie_sys_base_virt)
    {
        return -OAL_ENOMEM;
    }

    for(;;)
    {
        OAL_IO_PRINT("wait pcie link up...\n");

    	val = readl(pcie_sys_base_virt + 0x18);
    	link_up_stable_counter=0;
    	for(loop = 0; loop < TRY_MAX; loop++)
    	{
    		if ( (val & 0x20) && (val & 0x8000 ))
    		{
    			link_up_stable_counter++;
    		}
    		else
    		{
    			link_up_stable_counter = 0;
    			oal_msleep(1000);
    		}

    		if (3 <= link_up_stable_counter)
    		{
    			break;
    		}
    		val = readl(pcie_sys_base_virt + 0x18);
    	}

    	if(TRY_MAX == loop)
    	{
            retry_count++;
    	    OAL_IO_PRINT("wait pcie link up timeout, retry...=%u, val=0x%8x\n", retry_count, val);
    	    iounmap(pcie_sys_base_virt);
    	    return -OAL_ETIMEDOUT;
    	}
    	else
    	{
    	    OAL_IO_PRINT("wait pcie link up succ, val=0x%8x\n", val);
    	    break;
    	}
	}

	if(retry_count)
	{
	    OAL_IO_PRINT("wlan_link_succ retry %u\n", retry_count);
	}
	else
	{
	    OAL_IO_PRINT("wlan_link_succ retry %u\n", retry_count);
	}

    iounmap(pcie_sys_base_virt);
    return OAL_SUCC;
}
#endif

oal_int32 oal_pci_card_detect(oal_void)
{
#ifdef CONFIG_ARCH_SD56XX
#define TRY_MAX 2000000
	unsigned int val;
	unsigned int retry_count = 0;
	unsigned int loop = 0;
	unsigned int link_up_stable_counter = 0;
    void* __iomem pcie_sys_base_virt   = NULL;
    /*等待建链*/
    pcie_sys_base_virt = ioremap_nocache(0x10100000, 0x1000);
    if(NULL == pcie_sys_base_virt)
    {
        return -OAL_ENOMEM;
    }

    for(;;)
    {
        oal_pci_wlan_power_on(OAL_TRUE);
        oal_msleep(100);
        OAL_IO_PRINT("wait pcie link up...\n");

    	val = readl(pcie_sys_base_virt + 0x18);
    	link_up_stable_counter=0;
    	for(loop = 0; loop < TRY_MAX; loop++)
    	{
    		if ( (val & 0x20) && (val & 0x8000 ))
    		{
    			link_up_stable_counter++;
    		}
    		else
    		{
    			link_up_stable_counter = 0;
    			oal_msleep(1000);
    		}

    		if (3 <= link_up_stable_counter)
    		{
    			break;
    		}
    		val = readl(pcie_sys_base_virt + 0x18);
    	}

    	if(TRY_MAX == loop)
    	{
            retry_count++;
    	    OAL_IO_PRINT("wait pcie link up timeout, retry...=%u, val=0x%8x\n", retry_count, val);
    	}
    	else
    	{
    	    OAL_IO_PRINT("wait pcie link up succ, val=0x%8x\n", val);
    	    break;
    	}
	}

	if(retry_count)
	{
	    OAL_IO_PRINT("wlan_link_succ retry %u\n", retry_count);
	}
	else
	{
	    OAL_IO_PRINT("wlan_link_succ retry %u\n", retry_count);
	}

    iounmap(pcie_sys_base_virt);
    return OAL_SUCC;
#else
    return -OAL_ENODEV;
#endif
}

OAL_STATIC oal_int32 oal_pcie_tx_condition(hcc_bus* pst_bus, hcc_netbuf_queue_type qtype)
{
    oal_pcie_linux_res * pst_pci_lres;
    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        OAL_IO_PRINT("pci linux res is null\n");
        return OAL_FALSE;
    }
#if 0
    if(OAL_LIKELY(pst_pci_lres->pst_pci_res))
    {
        if(OAL_UNLIKELY(pst_pci_lres->pst_pci_res->link_state < PCI_WLAN_LINK_WORK_UP))
        {
            PCI_PRINT_LOG(PCI_LOG_DBG, "tx condition check ,link state:%d", pst_pci_lres->pst_pci_res->link_state);
            return OAL_FALSE;
        }
    }
#endif

    return oal_pcie_tx_is_idle(pst_pci_lres->pst_pci_res, oal_pcie_hcc_qtype_to_pci_qtype(qtype));
}

OAL_STATIC oal_int32 oal_pcie_host_lock(hcc_bus* pst_bus)
{
    oal_pcie_linux_res * pst_pci_lres;
    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    return OAL_SUCC;
}

OAL_STATIC oal_int32 oal_pcie_host_unlock(hcc_bus* pst_bus)
{
    oal_pcie_linux_res * pst_pci_lres;
    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    return OAL_SUCC;
}

/*1103 PCIE 通过 host_wakeup_dev gpio 来唤醒和通知WCPU睡眠*/
OAL_STATIC oal_int32 oal_pcie_sleep_request(hcc_bus* pst_bus)
{
    oal_pcie_linux_res * pst_pci_lres;
    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;

    oal_disable_pcie_irq(pst_pci_lres);

    /*拉低GPIO，PCIE只有在system suspend的时候才会下电
      GPIO 拉低之后 DEV 随时可能进深睡，不允许再通过PCIE 访问*/
    mutex_lock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
    oal_pcie_change_link_state(pst_pci_lres->pst_pci_res, PCI_WLAN_LINK_UP);
    mutex_unlock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);

    pci_clear_master(pst_pci_lres->pst_pcie_dev);

    return board_host_wakeup_dev_set(0);
}

OAL_STATIC oal_int32 oal_pcie_sleep_request_host(hcc_bus* pst_bus)
{
    /*check host sleep condtion*/
    oal_pcie_linux_res* pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    return oal_pcie_sleep_request_host_check(pst_pci_lres->pst_pci_res);
}

oal_void oal_pcie_log_print(oal_void)
{
    if(oal_print_rate_limit(PRINT_RATE_SECOND))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "oal_print_rate_limit");
    }
}

oal_void oal_pcie_log_print_hi1xx(oal_void)
{
    oal_pcie_log_print();
    oal_msleep(1200);
    oal_pcie_log_print();
    oal_pcie_log_print();
    oal_msleep(2000);
    oal_pcie_log_print();
}

OAL_STATIC oal_int32 oal_pcie_wakeup_request(hcc_bus* pst_bus)
{
#ifdef CONFIG_ARCH_KIRIN_PCIE
    oal_int32 ret;
    oal_pcie_linux_res * pst_pci_lres;
    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;

    //1.拉高 Host WakeUp Device gpio
    //2.调用kirin_pcie_pm_control 上电RC 检查建链
    //3.restore state, load iatu config

    if(OAL_UNLIKELY(pst_pci_lres->pst_pci_res->link_state <= PCI_WLAN_LINK_DOWN))
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "link invaild, wakeup failed, link_state:%s", oal_pcie_get_link_state_str(pst_pci_lres->pst_pci_res->link_state));
        return -OAL_ENODEV;
    }

    if(PCIE_EP_IP_POWER_DOWN == pst_pci_lres->power_status)
    {
        oal_uint32 ul_ret;
        declare_time_cost_stru(cost);
        struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();
        oal_get_time_cost_start(cost);

        if(NULL != pst_wlan_pm)
        {
            OAL_INIT_COMPLETION(&pst_wlan_pm->st_wifi_powerup_done);
        }

        oal_atomic_set(&g_bus_powerup_dev_wait_ack, 1);
#if 0
        /*suspend 会下电PCIE，这里如果已经下电需要重新初始化PCIE,恢复iatu表项*/
        ret = kirin_pcie_power_notifiy_register(kirin_rc_idx, oal_pcie_device_wakeup_handler,
                                            NULL, NULL);
        if(ret)
        {
            return ret;
        }

        ret = kirin_pcie_pm_control(1, kirin_rc_idx);
        if(ret)
        {
            /*这里可以增加DFR流程，麒麟LINKUP 只有20ms的超时时间*/
            OAL_IO_PRINT(KERN_ERR"kirin_pcie_pm_control 1 failed!ret=%d\n", ret);
            return ret;
        }
#else
        oal_pcie_device_wakeup_handler(NULL);
        if(NULL != pst_wlan_pm)
        {
            ul_ret = oal_wait_for_completion_timeout(&pst_wlan_pm->st_wifi_powerup_done, (oal_uint32)OAL_MSECS_TO_JIFFIES(2000));
            if(OAL_UNLIKELY(0 == ul_ret))
            {
                /*超时不做处理继续尝试建链*/
                DECLARE_DFT_TRACE_KEY_INFO("pcie_powerup_wakeup ack timeout", OAL_DFT_TRACE_FAIL);
            }
        }
        else
        {
            oal_msleep(100);
        }

        oal_atomic_set(&g_bus_powerup_dev_wait_ack, 0);
#endif
        /*iatu*/
        ret = oal_pcie_reinit(pst_bus);
        if(OAL_SUCC != ret)
        {
            //kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL, NULL, NULL);
            //kirin_pcie_pm_control(0, kirin_rc_idx);
            PCI_PRINT_LOG(PCI_LOG_ERR, "oal_pcie_reinit failed!ret=%d", ret);
            return ret;
        }

        oal_pcie_host_ip_init(pst_pci_lres);

        if(OAL_FALSE == oal_pcie_check_link_state(pst_pci_lres->pst_pci_res))
        {
            oal_uint32 version = 0x0;
            ret = oal_pci_read_config_dword(pst_pci_lres->pst_pcie_dev, 0x0, &version);
            PCI_PRINT_LOG(PCI_LOG_ERR, "mem access error, maybe linkdown! config read version :0x%x, ret=%d",
                                            version , ret);
            return -OAL_ENODEV;
        }

        /*唤醒流程，RES已经初始化*/
        oal_pcie_change_link_state(pst_pci_lres->pst_pci_res, PCI_WLAN_LINK_RES_UP);

        oal_atomic_set(&pst_pci_lres->st_pcie_wakeup_flag, 1);
        ret = oal_pcie_enable_device_func(pst_pci_lres);
        oal_atomic_set(&pst_pci_lres->st_pcie_wakeup_flag, 0);
        if(OAL_SUCC != ret)
        {
            oal_pcie_disable_regions(pst_pci_lres->pst_pci_res);
            //kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL, NULL, NULL);
            //kirin_pcie_pm_control(0, kirin_rc_idx);
            return ret;
        }

        mutex_lock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
        oal_pcie_change_link_state(pst_pci_lres->pst_pci_res, PCI_WLAN_LINK_WORK_UP);
        mutex_unlock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);

        hcc_bus_enable_state(pst_bus, OAL_BUS_STATE_ALL);
        pst_pci_lres->power_status = PCIE_EP_IP_POWER_UP;

        oal_get_time_cost_end(cost);
        oal_calc_time_cost_sub(cost);
        PCI_PRINT_LOG(PCI_LOG_INFO, "pcie power up init succ, cost %llu us\n", time_cost_var_sub(cost));

    }
    else
    {
        /*正常单芯片唤醒拉高GPIO即可*/
        oal_pcie_device_wakeup_handler(NULL);
        pci_set_master(pst_pci_lres->pst_pcie_dev);
    }

#endif
    return OAL_SUCC;
}

OAL_STATIC oal_int32 oal_pcie_get_sleep_state(hcc_bus* pst_bus)
{
    oal_pcie_linux_res * pst_pci_lres;
    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        OAL_IO_PRINT("pst_pci_lres is null\n");
        return -OAL_EINVAL;
    }

    return (board_get_host_wakeup_dev_stat() == 1) ? DISALLOW_TO_SLEEP_VALUE : ALLOW_TO_SLEEP_VALUE;
}

OAL_STATIC oal_int32 oal_pcie_wakeup_complete(hcc_bus* pst_bus)
{
    oal_pcie_linux_res * pst_pci_lres;
    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        OAL_IO_PRINT("pst_pci_lres is null\n");
        return -OAL_EINVAL;
    }

    mutex_lock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
    oal_pcie_change_link_state(pst_pci_lres->pst_pci_res, PCI_WLAN_LINK_WORK_UP);
    mutex_unlock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);

    oal_enable_pcie_irq(pst_pci_lres);

    oal_pcie_shced_rx_hi_thread(pst_pci_lres->pst_pci_res);

    return OAL_SUCC;
}


OAL_STATIC oal_int32 oal_pcie_rx_int_mask(hcc_bus* pst_bus, oal_int32 is_mask)
{
    oal_pcie_linux_res * pst_pci_lres;
    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
//    TODO:to be implement
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        OAL_IO_PRINT("pst_pci_lres is null\n");
        return -OAL_EINVAL;
    }
    if(is_mask)
    {
        //TODO
    }
    else
    {
        //TODO
    }
    return OAL_SUCC;
}

OAL_STATIC oal_int32 oal_pcie_power_ctrl(hcc_bus *hi_bus, HCC_BUS_POWER_CTRL_TYPE ctrl, int (*func)(void* data), oal_void* data)
{
    oal_int32 ret = -OAL_EFAIL;

    if(HCC_BUS_CTRL_POWER_UP == ctrl)
    {
#ifdef CONFIG_ARCH_KIRIN_PCIE
        ret = kirin_pcie_power_notifiy_register(kirin_rc_idx, func, NULL, data);
#endif
    }

    if(HCC_BUS_CTRL_POWER_DOWN == ctrl)
    {
#ifdef CONFIG_ARCH_KIRIN_PCIE
        ret = kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL, func, data);
#endif
    }

    return ret;
}

oal_int32 oal_pcie_enable_device_func(oal_pcie_linux_res* pst_pci_lres)
{
    oal_int32 i;
    oal_int32 retry = 2;
    oal_int32 ret;
    hcc_bus* pst_bus = pst_pci_lres->pst_bus;

    if(NULL == pst_bus)
    {
        return -OAL_ENODEV;
    }

    for(i = 0; i < retry; i++)
    {
        OAL_INIT_COMPLETION(&pst_pci_lres->st_pcie_ready);

        ret = oal_pcie_send_message2dev(pst_pci_lres, PCIE_H2D_MESSAGE_HOST_READY);
        if(OAL_SUCC != ret)
        {
            OAL_IO_PRINT(KERN_ERR"oal_pcie_send_message2dev failed, ret=%d\n", ret);
            continue;
        }

        /*第一个中断有可能在中断使能之前上报，强制调度一次RX Thread*/
        up(&pst_bus->rx_sema);

        if(0 == oal_wait_for_completion_timeout(&pst_pci_lres->st_pcie_ready, (oal_uint32)OAL_MSECS_TO_JIFFIES(HOST_WAIT_BOTTOM_INIT_TIMEOUT)))
        {
            OAL_IO_PRINT(KERN_ERR"wait pcie ready timeout... %d ms \n", HOST_WAIT_BOTTOM_INIT_TIMEOUT);
            up(&pst_bus->rx_sema);
            if(0 == oal_wait_for_completion_timeout(&pst_pci_lres->st_pcie_ready, (oal_uint32)OAL_MSECS_TO_JIFFIES(5000)))
            {
                OAL_IO_PRINT(KERN_ERR"pcie retry 5 second hold, still timeout");
                if(0 == i)
                {
                    DECLARE_DFT_TRACE_KEY_INFO("wait pcie ready when power on, retry",OAL_DFT_TRACE_FAIL);
                }
                else
                {
                    DECLARE_DFT_TRACE_KEY_INFO("wait pcie ready when power on, retry still failed",OAL_DFT_TRACE_FAIL);
                }
                continue;
            }
            else
            {
                /*强制调度成功，说明有可能是GPIO中断未响应*/
                OAL_IO_PRINT(KERN_WARNING"[E]retry succ, maybe gpio interrupt issue");
                DECLARE_DFT_TRACE_KEY_INFO("pcie gpio int issue",OAL_DFT_TRACE_FAIL);
                break;
            }
        }
        else
        {
            break;
        }
    }

    if(i >= 2)
    {
        return ret;
    }

    return OAL_SUCC;
}

/*非中断触发，轮询消息*/
oal_int32 oal_pcie_enable_device_func_polling(oal_pcie_linux_res* pst_pci_lres)
{
    oal_int32 ret;
    oal_int32 retry = 0;
    oal_ulong timeout_jiffies;

    hcc_bus* pst_bus = pst_pci_lres->pst_bus;

    if(NULL == pst_bus)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst bus is null");
        return -OAL_ENODEV;
    }

    OAL_INIT_COMPLETION(&pst_pci_lres->st_pcie_ready);

   /*等待device初始化完成*/
    //oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_pci_lres->pst_bus), OAL_FALSE);

    /*通知Device已经初始化完成*/
    ret = oal_pcie_send_message2dev(pst_pci_lres, PCIE_H2D_MESSAGE_HOST_READY);
    if(OAL_SUCC != ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "oal_pcie_send_message2dev failed!ret=%d\n", ret);
        return ret;
    }

    up(&pst_bus->rx_sema);

    timeout_jiffies = jiffies + OAL_MSECS_TO_JIFFIES(10000);
    for(;;)
    {
        if(try_wait_for_completion(&pst_pci_lres->st_pcie_ready))
        {
            /*decrement succ*/
            break;
        }

        if(time_after(jiffies, timeout_jiffies))
        {
            if(retry)
            {
                //oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_bus), OAL_TRUE);
                PCI_PRINT_LOG(PCI_LOG_ERR, "retry wait for pcie dev ready, 0x%lx, 0x%lx", jiffies, timeout_jiffies);
                DECLARE_DFT_TRACE_KEY_INFO("retry wait pcie dev ready",OAL_DFT_TRACE_FAIL);
                return -OAL_ETIMEDOUT;
            }
            else
            {
                retry = 1;
                PCI_PRINT_LOG(PCI_LOG_ERR, "wait for pcie dev ready, 0x%lx, 0x%lx", jiffies, timeout_jiffies);
                DECLARE_DFT_TRACE_KEY_INFO("wait pcie dev ready",OAL_DFT_TRACE_FAIL);
                ret = oal_pcie_send_message2dev(pst_pci_lres, PCIE_H2D_MESSAGE_HOST_READY);
                if(OAL_SUCC != ret)
                {
                    PCI_PRINT_LOG(PCI_LOG_ERR, "oal_pcie_send_message2dev retry failed!ret=%d\n", ret);
                    return ret;
                }

                timeout_jiffies = jiffies + OAL_MSECS_TO_JIFFIES(5000);
                continue;
            }
        }

        up(&pst_bus->rx_sema);
        oal_msleep(1);
    }

    mutex_lock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
    oal_pcie_change_link_state(pst_pci_lres->pst_pci_res, PCI_WLAN_LINK_WORK_UP);
    mutex_unlock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);

    //oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_bus), OAL_TRUE);
    PCI_PRINT_LOG(PCI_LOG_DBG, "device all ready, pcie wakeup done.");

    return OAL_SUCC;
}

OAL_STATIC oal_int32 oal_enable_pcie_irq(oal_pcie_linux_res* pst_pci_lres)
{
    oal_uint flag;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        return -OAL_ENODEV;
    }


    oal_spin_lock_irq_save(&pst_pci_lres->st_lock, &flag);
    if(1 == pst_pci_lres->irq_stat)
    {
        if(!pst_pci_lres->st_msi.is_msi_support)
        {
            /*intx*/
            PCI_PRINT_LOG(PCI_LOG_DBG, "enable_pcie_irq");
            enable_irq(pst_pci_lres->pst_pcie_dev->irq);
        }
        else
        {
            /*msi*/
        }
        pst_pci_lres->irq_stat = 0;
    }
    oal_spin_unlock_irq_restore(&pst_pci_lres->st_lock, &flag);

    return OAL_SUCC;
}

oal_int32 oal_disable_pcie_irq(oal_pcie_linux_res* pst_pci_lres)
{
    oal_uint flag;

    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        return -OAL_ENODEV;
    }

    oal_spin_lock_irq_save(&pst_pci_lres->st_lock, &flag);
    if(0 == pst_pci_lres->irq_stat)
    {
        pst_pci_lres->irq_stat = 1;
        if(!pst_pci_lres->st_msi.is_msi_support)
        {
            /*intx*/
            PCI_PRINT_LOG(PCI_LOG_DBG, "disable_pcie_irq");
            if(in_irq())
            {
                disable_irq_nosync(pst_pci_lres->pst_pcie_dev->irq);
            }
            else
            {
                /*process context*/
                disable_irq_nosync(pst_pci_lres->pst_pcie_dev->irq);
                oal_spin_unlock_irq_restore(&pst_pci_lres->st_lock, &flag);
                synchronize_irq(pst_pci_lres->pst_pcie_dev->irq);
                oal_spin_lock_irq_save(&pst_pci_lres->st_lock, &flag);
            }
        }
        else
        {
            /*msi*/
        }
    }
    oal_spin_unlock_irq_restore(&pst_pci_lres->st_lock, &flag);

    return OAL_SUCC;
}

OAL_STATIC oal_int32 oal_pcie_host_aspm_init(oal_pcie_linux_res* pst_pci_lres)
{
#ifdef CONFIG_ARCH_KIRIN_PCIE
    oal_uint16 val = 0;
    oal_uint32 reg;
    oal_pci_dev_stru *pst_rc_dev;
    pst_rc_dev = pci_upstream_bridge(pst_pci_lres->pst_pcie_dev);

    /*使能/去使能ASPM，RC & EP*/
    kirin_pcie_lp_ctrl(kirin_rc_idx, 0);
    if(pcie_aspm_enable)
    {
        /*L1SS config*/
        if((NULL != pst_pci_lres->pst_pci_res) && (NULL != pst_pci_lres->pst_pci_res->pst_pci_dbi_base))
        {
            reg = oal_readl(pst_pci_lres->pst_pci_res->pst_pci_dbi_base + PCIE_ACK_F_ASPM_CTRL_OFF);
            //reg &= ~ ((0x7 << 24) | (0x7 << 27));
            reg &= ~ ((0x7 << 27));
            /*L0s 7us entry, L1 64us entery, tx & rx*/
            //reg |= (0x7 << 24) | (0x7 << 27);
            reg |= (0x3 << 27);
            oal_writel(reg, pst_pci_lres->pst_pci_res->pst_pci_dbi_base + PCIE_ACK_F_ASPM_CTRL_OFF);
            PCI_PRINT_LOG(PCI_LOG_INFO, "ack aspm ctrl val:0x%x", reg);
#if 0
            reg = oal_readl(pst_pci_lres->pst_pci_res->pst_pci_dbi_base + PCIE_AUX_CLK_FREQ_OFF);
            reg &= ~(0x3ff);
            reg |= (0x2 << 0);/*aux_clk 2m,actual 1.92M  38.4M/20*/
            oal_writel(reg, pst_pci_lres->pst_pci_res->pst_pci_dbi_base + PCIE_AUX_CLK_FREQ_OFF);
            PCI_PRINT_LOG(PCI_LOG_INFO, "aux_clk_freq val:0x%x", reg);
#endif
        }
        else
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "pci res null or ctrl_base is null");
        }

        kirin_pcie_lp_ctrl(kirin_rc_idx, 1);
    }
    PCI_PRINT_LOG(PCI_LOG_INFO, "pcie_aspm_enable:%d", pcie_aspm_enable);

    /*rc noc protect*/
    if(NULL != pst_rc_dev)
    {
        oal_pci_read_config_word(pst_rc_dev, oal_pci_pcie_cap(pst_rc_dev) + PCI_EXP_DEVCTL2, &val);
#if 0
        val |= (0xe << 0);
        oal_pci_write_config_word(pst_rc_dev, oal_pci_pcie_cap(pst_rc_dev) + PCI_EXP_DEVCTL2, val);
#endif
        PCI_PRINT_LOG(PCI_LOG_INFO,"devctrl:0x%x", val);
    }
#endif
    return OAL_SUCC;
}

oal_int32 oal_pcie_get_mps(oal_pci_dev_stru*  pst_pcie_dev)
{
    oal_int32 ret;
	oal_int32 pos;
	oal_uint16 reg16;

	pos = pci_find_capability(pst_pcie_dev, PCI_CAP_ID_EXP);
	if (!pos)
		return -1;

	ret = oal_pci_read_config_word(pst_pcie_dev, pos + PCI_EXP_DEVCAP, &reg16);
	if(ret)
	{
	    PCI_PRINT_LOG(PCI_LOG_ERR, "pci read devcap failed ret=%d", ret);
	    return -1;
	}

	return (oal_int32)(reg16 & PCI_EXP_DEVCAP_PAYLOAD);
}

/*Max Payload Size Supported,
  must config beofre pcie access*/
OAL_STATIC oal_int32 oal_pcie_mps_init(oal_pcie_linux_res* pst_pci_lres)
{
#ifdef CONFIG_ARCH_KIRIN_PCIE
    oal_int32 rc_mps, ep_mps, mps, mrq;
    oal_pci_dev_stru *pst_rc_dev;

    if(!pcie_performance_mode)
    {
        return OAL_SUCC;
    }

    pst_rc_dev = pci_upstream_bridge(pst_pci_lres->pst_pcie_dev);
    if(NULL == pst_rc_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "no upstream dev");
        return -OAL_ENODEV;
    }

    rc_mps = oal_pcie_get_mps(pst_rc_dev);
    ep_mps = oal_pcie_get_mps(pst_pci_lres->pst_pcie_dev);

    if(rc_mps < 0 || ep_mps < 0)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "mps get failed, rc_mps:%d  , ep_mps:%d", rc_mps, ep_mps);
        return -OAL_EFAIL;
    }

    mps = OAL_MIN(rc_mps, ep_mps);
    mrq = mps;
    PCI_PRINT_LOG(PCI_LOG_INFO, "rc/ep max payload size, rc_mps:%d  , ep_mps:%d, select mps:%d bytes", rc_mps, ep_mps, 128 << mps);
    mps <<= 5;/*PCI_EXP_DEVCTL_PAYLOAD*/
    mrq <<= 12;/*PCI_EXP_DEVCTL_READRQ*/

    pcie_capability_clear_and_set_word(pst_pci_lres->pst_pcie_dev, PCI_EXP_DEVCTL,
						  PCI_EXP_DEVCTL_READRQ, mrq);
    pcie_capability_clear_and_set_word(pst_pci_lres->pst_pcie_dev, PCI_EXP_DEVCTL,
						  PCI_EXP_DEVCTL_PAYLOAD, mps);
    pcie_capability_clear_and_set_word(pst_rc_dev, PCI_EXP_DEVCTL,
						  PCI_EXP_DEVCTL_PAYLOAD, mps);
    pcie_capability_clear_and_set_word(pst_rc_dev, PCI_EXP_DEVCTL,
						  PCI_EXP_DEVCTL_READRQ, mrq);
#endif
    return OAL_SUCC;
}

OAL_STATIC oal_int32 oal_pcie_host_ip_init(oal_pcie_linux_res* pst_pci_lres)
{

    /*rc bar init*/
#ifdef CONFIG_ARCH_KIRIN_PCIE
    if((PCIE_REVISION_5_00A == pst_pci_lres->pst_pci_res->revision) && (pcie_rc_bar_bypass))
    {
        oal_pci_dev_stru *pst_rc_dev;
        pst_rc_dev = pci_upstream_bridge(pst_pci_lres->pst_pcie_dev);
        if(NULL == pst_rc_dev)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "no upstream dev");
        }
        else
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "reset rc bar");
            oal_pci_write_config_dword(pst_rc_dev, 0x10, 0xc);
            oal_pci_write_config_dword(pst_rc_dev, 0x14, 0x0);
            oal_pci_write_config_dword(pst_rc_dev, 0x18, 0x0);
            oal_pci_write_config_dword(pst_rc_dev, 0x1c, 0x0);
            oal_pci_write_config_dword(pst_rc_dev, 0x20, 0x0);
            oal_pci_write_config_dword(pst_rc_dev, 0x24, 0x0);
            oal_pci_write_config_dword(pst_rc_dev, 0x28, 0x0);
            oal_pci_write_config_dword(pst_rc_dev, 0x2c, 0x0);
        }
    }
#endif
    oal_pcie_mps_init(pst_pci_lres);
    oal_pcie_host_aspm_init(pst_pci_lres);
    oal_pcie_set_device_soft_fifo_enable(pst_pci_lres->pst_pci_res);
    oal_pcie_set_device_ringbuf_bugfix_enable(pst_pci_lres->pst_pci_res);
    oal_pcie_set_device_dma_check_enable(pst_pci_lres->pst_pci_res);
    return OAL_SUCC;
}

oal_void oal_pcie_print_chip_info(oal_pcie_linux_res* pst_pci_lres)
{
#if 0
    /*L1 recovery count*/
    oal_uint32 l1_err_cnt = oal_readl(pst_pci_lres->pst_pci_res->pst_pci_ctrl_base + PCIE_STAT_CNT_LTSSM_ENTER_RCVRY_OFF);
    oal_uint32 l1_recvy_cnt = oal_readl(pst_pci_lres->pst_pci_res->pst_pci_ctrl_base + PCIE_STAT_CNT_L1_ENTER_RCVRY_OFF);

    if(l1_err_cnt > 2)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "not l1 recovry error count %u, link_state unstable", l1_err_cnt);
    }
    else
    {
        /*建链过程中会进入2次*/
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "link_state stable, l1 excp count:%u", l1_err_cnt);
    }

    if(l1_recvy_cnt)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "l1 enter count is %u", l1_recvy_cnt);
    }
    else
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "l1 maybe not enable");
    }
#endif
}

oal_void oal_pcie_chip_info(hcc_bus *pst_bus)
{
    oal_pcie_linux_res * pst_pci_lres;
    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        return;
    }
    return oal_pcie_print_chip_info(pst_pci_lres);
}

oal_int32 oal_pcie_ip_l1pm_check(oal_pcie_linux_res* pst_pci_lres)
{
    oal_pcie_print_chip_info(pst_pci_lres);

    oal_msleep(5);/*wait pcie enter L1.2*/
    if(ft_pcie_aspm_check_bypass)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "l1pm_check bypass");
        return OAL_SUCC;
    }
    else
    {
#ifdef CONFIG_ARCH_KIRIN_PCIE
#if defined(CONFIG_KIRIN_PCIE_HI3660)
        oal_uint32 value;
        void* __iomem pst_rc_ahb_reg =  NULL;
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "kirin960 pcie test");
        pst_rc_ahb_reg = ioremap_nocache(0xff3fe000, 0x1000);
        if(NULL == pst_rc_ahb_reg)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "pst_rc_ahb_reg request failed");
            return -OAL_ENOMEM;
        }

        value = oal_readl(pst_rc_ahb_reg + 0x414);
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "SOC_PCIECTRL_STATE4_ADDR reg=0x%x", value);

        if((value & (1 << 14 | 1 << 15)) == ((1 << 14 | 1 << 15)))
        {
            return OAL_SUCC;
        }
        else
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "L1.2 check failed, reg=0x%x, bit 14 15 must be 1", value);
            return -OAL_EFAIL;
        }

#elif defined(CONFIG_KIRIN_PCIE_KIRIN970)
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "kirin970 not implemented");
        return OAL_ENODEV;
#else
        oal_uint32 value = show_link_state(kirin_rc_idx);
        if(value == 0xC000)
        {
            return OAL_SUCC;
        }
        else
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "L1.2 check failed");
            return -OAL_EFAIL;
        }
#endif
#else
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "to be implemented");
        return -OAL_ENODEV;
#endif
    }
}

oal_int32 oal_pcie_gen_mode_check(oal_pcie_linux_res* pst_pci_lres)
{
    /*根据实际产品来判断当前链路状态是否正常*/
    oal_int32 gen_select = oal_pcie_get_gen_mode(pst_pci_lres->pst_pci_res);

    if(ft_pcie_gen_check_bypass)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie_gen_check_bypass");
    }
    else
    {
        if(hipci_gen_select != gen_select)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "expect link mode is %d, but current is %d",
                                                hipci_gen_select, gen_select);
            return -OAL_EFAIL;
        }
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "current link is %s",
                                (PCIE_GEN2 == gen_select) ? "GEN2": "GEN1");

    return OAL_SUCC;
}


oal_int32 oal_pcie_ip_init(hcc_bus *pst_bus)
{
    oal_int32 ret;
    oal_pcie_linux_res * pst_pci_lres;
    if(NULL == pst_bus)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "pst_bus is null");
        return -OAL_ENODEV;
    }

    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "pst_pci_lres is null");
        return -OAL_ENODEV;
    }

    ret = oal_pcie_device_auxclk_init(pst_pci_lres->pst_pci_res);
    if(ret)
        return ret;

    ret = oal_pcie_device_aspm_init(pst_pci_lres->pst_pci_res);
    if(ret)
        return ret;

    /*使能低功耗*/
    ret = oal_pcie_host_aspm_init(pst_pci_lres);
    if(ret)
        return ret;

    return OAL_SUCC;
}

oal_int32 oal_pcie_ip_voltage_bias_init(hcc_bus *pst_bus)
{
    oal_pcie_linux_res * pst_pci_lres;
    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        return -OAL_ENODEV;
    }
    return oal_pcie_voltage_bias_init(pst_pci_lres->pst_pci_res);
}

oal_int32 oal_pcie_ip_factory_test(hcc_bus *pst_bus, oal_int32 test_count)
{
    oal_int32 i;
    oal_int32 ret;
    oal_pcie_linux_res * pst_pci_lres;

    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        return -OAL_ENODEV;
    }

    ret = oal_pcie_ip_init(pst_bus);

    oal_msleep(5);/*wait pcie enter L1.2*/

    ret = oal_pcie_ip_l1pm_check(pst_pci_lres);
    if(ret)
        return ret;

    ret = oal_pcie_voltage_bias_init(pst_pci_lres->pst_pci_res);
    if(ret)
        return ret;

    ret = oal_pcie_device_changeto_high_cpufreq(pst_pci_lres->pst_pci_res);
    if(ret)
        return ret;

    ret = oal_pcie_gen_mode_check(pst_pci_lres);
    if(ret)
        return ret;

    ret = oal_pcie_unmask_device_link_erros(pst_pci_lres->pst_pci_res);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "unmask device link err failed = %d", ret);
        return ret;
    }

    for(i = 0; i < test_count; i++)
    {
        /*memcheck*/
        ret = oal_pcie_device_mem_scanall(pst_pci_lres->pst_pci_res);
        if(ret)
            return ret;

        oal_print_hi11xx_log(HI11XX_LOG_INFO, "scan all mem done , test %d times", i+1);

        ret = oal_pcie_check_device_link_errors(pst_pci_lres->pst_pci_res);
        if(ret)
            return ret;

        ret = oal_pcie_gen_mode_check(pst_pci_lres);
        if(ret)
            return ret;
    }

    oal_msleep(5);/*wait pcie enter L1.2*/

    ret = oal_pcie_ip_l1pm_check(pst_pci_lres);
    if(ret)
        return ret;

    return OAL_SUCC;
}

oal_int32 oal_pcie_rc_slt_chip_transfer(hcc_bus *pst_bus, oal_void* ddr_address,
                                              oal_uint32 data_size, oal_int32 direction)
{
    oal_pcie_linux_res * pst_pci_lres;
    if(NULL == pst_bus)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "pst_bus is null");
        return -OAL_ENODEV;
    }

    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "pst_pci_lres is null");
        return -OAL_ENODEV;
    }

    if(1 == direction)
    {
        /*H2D*/
        return oal_pcie_copy_to_device_by_dword(pst_pci_lres->pst_pci_res, ddr_address,
                                               oal_pcie_get_deivce_dtcm_cpuaddr(pst_pci_lres->pst_pci_res),
                                               data_size);
    }
    else if(2 == direction)
    {
        /*D2H*/
        return oal_pcie_copy_from_device_by_dword(pst_pci_lres->pst_pci_res, ddr_address,
                                               oal_pcie_get_deivce_dtcm_cpuaddr(pst_pci_lres->pst_pci_res),
                                               data_size);
    }
    else
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "invaild direction:%d", direction);
        return -OAL_EINVAL;
    }
}


oal_void oal_pcie_power_down(hcc_bus *pst_bus)
{
    /*关掉LINKDOWN注册回调函数 TBD:TBC*/
    oal_pcie_linux_res * pst_pci_lres;
    PCIE_EP_IP_STATUS       old_power_status;

    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;

    /*disable intx gpio... 等待中断处理完*/
    oal_disable_pcie_irq(pst_pci_lres);

    old_power_status = pst_pci_lres->power_status;

    pst_pci_lres->power_status = PCIE_EP_IP_POWER_DOWN;
    if(NULL == pst_pci_lres->pst_pci_res)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "pst_pci_res is null");
        return;;
    }

    mutex_lock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
    oal_pcie_change_link_state(pst_pci_lres->pst_pci_res, PCI_WLAN_LINK_DOWN);
    mutex_unlock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
    //hcc_disable_etc(HBUS_TO_HCC(pst_bus), OAL_TRUE);

    oal_task_kill(&pst_pci_lres->st_rx_task);

    //hcc_transfer_lock(HBUS_TO_HCC(pst_bus));
    mutex_lock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
    oal_pcie_disable_regions(pst_pci_lres->pst_pci_res);
    oal_pcie_transfer_res_exit(pst_pci_lres->pst_pci_res);
    mutex_unlock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
    //hcc_transfer_unlock(HBUS_TO_HCC(pst_bus));
#ifdef CONFIG_ARCH_KIRIN_PCIE
    kirin_pcie_deregister_event(&pst_pci_lres->pcie_event);
    /*下电之前关闭 PCIE HOST 控制器*/
    {
        oal_int32 ret;
        ret = kirin_pcie_pm_control(0, kirin_rc_idx);
        if(PCIE_EP_IP_POWER_DOWN != old_power_status)
        {
            if(ret)
            {
                PCI_PRINT_LOG(PCI_LOG_INFO, "kirin_pcie_pm_control failed=%d", ret);
                DECLARE_DFT_TRACE_KEY_INFO("pcie_power_down_fail", OAL_DFT_TRACE_EXCEP);
            }
        }
    }
    kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL, NULL, NULL);
#endif

    //board_host_wakeup_dev_set(0);
}

#ifdef CONFIG_ARCH_KIRIN_PCIE
/*chip test*/
oal_void pcie_power_down_test(oal_void)
{
    oal_pcie_linux_res * pst_pci_lres = g_pcie_linux_res;
    if(NULL == pst_pci_lres)
    {
        PCI_PRINT_LOG(PCI_LOG_INFO,"pst_pci_lres is null");
        return;
    }
    kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL, oal_pcie_device_suspend_handler, (void*)pst_pci_lres);
    kirin_pcie_deregister_event(&pst_pci_lres->pcie_event);
    oal_pcie_save_default_resource(pst_pci_lres);
    {
        oal_int32 ret;
        ret = kirin_pcie_pm_control(0, kirin_rc_idx);
        if(ret)
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "kirin_pcie_pm_controltest failed=%d", ret);
            DECLARE_DFT_TRACE_KEY_INFO("pcie_power_down_fail", OAL_DFT_TRACE_EXCEP);
            ssi_dump_device_regs(SSI_MODULE_MASK_COMM | SSI_MODULE_MASK_PCIE_CFG | SSI_MODULE_MASK_PCIE_DBI);
        }
    }

}

oal_void pcie_power_down_test_wake(oal_void)
{
    oal_int32 ret;
    ret = kirin_pcie_pm_control(1, kirin_rc_idx);
    if(ret)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "ret = %d", ret);
    }
}
#endif

OAL_STATIC oal_int32 oal_pcie_power_action(hcc_bus *pst_bus, HCC_BUS_POWER_ACTION_TYPE action)
{
    oal_int32 ret;
    oal_pcie_linux_res * pst_pci_lres;

    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        return -OAL_ENODEV;
    }

    oal_print_inject_check_stack();

    if(HCC_BUS_POWER_DOWN == action)
    {
        /*关掉LINKDOWN注册回调函数 TBD:TBC*/
        hcc_disable_etc(HBUS_TO_HCC(pst_bus), OAL_TRUE);
        oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_bus), OAL_FALSE);
        hcc_transfer_lock(HBUS_TO_HCC(pst_bus));
        oal_pcie_power_down(pst_bus);
        hcc_transfer_unlock(HBUS_TO_HCC(pst_bus));
        board_host_wakeup_dev_set(0);
    }

    if(HCC_BUS_SW_POWER_DOWN == action)
    {
        oal_pcie_power_down(pst_bus);
    }

    if(HCC_BUS_POWER_UP == action || HCC_BUS_SW_POWER_UP == action)
    {
        /*上电之前打开PCIE HOST 控制器*/
#ifdef CONFIG_ARCH_KIRIN_PCIE
        declare_time_cost_stru(cost);
        if(HCC_BUS_POWER_UP == action)
        {
            oal_get_time_cost_start(cost);
        }

        ret = kirin_pcie_pm_control(1, kirin_rc_idx);
        board_host_wakeup_dev_set(0);
        kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL, NULL, NULL);
        if(ret)
        {
            OAL_IO_PRINT(KERN_ERR"kirin pcie power on and link failed, ret=%d\n", ret);
            if(HI1XX_ANDROID_BUILD_VARIANT_USER == hi11xx_get_android_build_variant())
            {
                if(oal_print_rate_limit(24*PRINT_RATE_HOUR))
                {
                    if(true == bfgx_is_shutdown_etc())
                    {
                        PCI_PRINT_LOG(PCI_LOG_INFO, "bfgx is shutdown");
                        ssi_dump_device_regs(SSI_MODULE_MASK_ARM_REG|SSI_MODULE_MASK_AON_CUT|SSI_MODULE_MASK_PCIE_CUT);
                    }
                    else
                    {
                        PCI_PRINT_LOG(PCI_LOG_INFO, "bfgx is working");
                        ssi_dump_device_regs(SSI_MODULE_MASK_ARM_REG);
                    }
                }
            }
            else
            {
                ssi_dump_device_regs(SSI_MODULE_MASK_COMM | SSI_MODULE_MASK_PCIE_CFG | SSI_MODULE_MASK_PCIE_DBI);
            }
            return ret;
        }

        kirin_pcie_register_event(&pst_pci_lres->pcie_event);

        pst_pci_lres->power_status = PCIE_EP_IP_POWER_UP;
        if(pst_pci_lres->pst_pci_res)
        {
            mutex_lock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
            oal_pcie_change_link_state(pst_pci_lres->pst_pci_res, PCI_WLAN_LINK_UP);
            mutex_unlock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
        }

        if(HCC_BUS_POWER_UP == action)
        {
            oal_get_time_cost_end(cost);
            oal_calc_time_cost_sub(cost);
            PCI_PRINT_LOG(PCI_LOG_INFO, "pcie link cost %llu us", time_cost_var_sub(cost));
        }
#endif
    }

    if(HCC_BUS_POWER_PATCH_LOAD_PREPARE == action)
    {
        /*close hcc*/
        hcc_disable_etc(HBUS_TO_HCC(pst_bus), OAL_TRUE);
        OAL_INIT_COMPLETION(&pst_bus->st_device_ready);
        oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_bus), OAL_FALSE);
    }

    if(HCC_BUS_SW_POWER_PATCH_LOAD_PREPARE == action)
    {
        OAL_INIT_COMPLETION(&pst_bus->st_device_ready);
    }

    if(HCC_BUS_POWER_PATCH_LAUCH == action)
    {
        OAL_IO_PRINT("power patch lauch\n");
        /*Patch下载完后 初始化通道资源，然后等待业务初始化完成*/

        ret = oal_pcie_transfer_res_init(pst_pci_lres->pst_pci_res);
        if(ret)
        {
            OAL_IO_PRINT(KERN_ERR"pcie_transfer_res_init failed, ret=%d\n", ret);
            return ret;
        }

        oal_pcie_host_ip_init(pst_pci_lres);

        oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_bus), OAL_TRUE);

        oal_enable_pcie_irq(pst_pci_lres);

        ret = oal_pcie_enable_device_func(pst_pci_lres);
        if(OAL_SUCC != ret)
        {
            OAL_IO_PRINT("enable pcie device func failed, ret=%d\n", ret);
            return ret;
        }

        if(0 == oal_wait_for_completion_timeout(&pst_bus->st_device_ready, (oal_uint32)OAL_MSECS_TO_JIFFIES(HOST_WAIT_BOTTOM_INIT_TIMEOUT)))
        {
            OAL_IO_PRINT(KERN_ERR"wait device ready timeout... %d ms \n", HOST_WAIT_BOTTOM_INIT_TIMEOUT);
            up(&pst_bus->rx_sema);
            if(0 == oal_wait_for_completion_timeout(&pst_bus->st_device_ready, (oal_uint32)OAL_MSECS_TO_JIFFIES(5000)))
            {
                OAL_IO_PRINT(KERN_ERR"retry 5 second hold, still timeout");
                return -OAL_ETIMEDOUT;
            }
            else
            {
                /*强制调度成功，说明有可能是GPIO中断未响应*/
                OAL_IO_PRINT(KERN_WARNING"[E]retry succ, maybe gpio interrupt issue");
                DECLARE_DFT_TRACE_KEY_INFO("pcie gpio int issue",OAL_DFT_TRACE_FAIL);
            }
        }

        if(0 == pcie_ringbuf_bugfix_enable)
        {
            ret = oal_pcie_ringbuf_h2d_refresh(pst_pci_lres->pst_pci_res);
            if(OAL_SUCC != ret)
            {
                return ret;
            }
        }

        mutex_lock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
        oal_pcie_change_link_state(pst_pci_lres->pst_pci_res, PCI_WLAN_LINK_WORK_UP);
        mutex_unlock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);

        hcc_enable_etc(HBUS_TO_HCC(pst_bus), OAL_TRUE);

        /*调度一次数据RX通道, PCIE 数据走INTX，消息走GPIO*/
        //oal_task_sched(&pst_pci_lres->st_rx_task);

        //oal_wake_lock(&pst_pci_lres->st_sr_bugfix);/*hold the AP*/

    }

    if(HCC_BUS_SW_POWER_PATCH_LAUCH == action)
    {
        OAL_IO_PRINT("HCC_BUS_SW_POWER_PATCH_LAUCH\n");
        /*Patch下载完后 初始化通道资源，然后等待业务初始化完成*/

        ret = oal_pcie_transfer_res_init(pst_pci_lres->pst_pci_res);
        if(ret)
        {
            OAL_IO_PRINT(KERN_ERR"pcie_transfer_res_init failed, ret=%d\n", ret);
            return ret;
        }

        oal_pcie_host_ip_init(pst_pci_lres);

        oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_bus), OAL_TRUE);

        oal_enable_pcie_irq(pst_pci_lres);

        /*此时GPIO不能使用，BUS还未切换*/
        ret = oal_pcie_enable_device_func_polling(pst_pci_lres);
        if(OAL_SUCC != ret)
        {
            OAL_IO_PRINT("enable pcie device func failed, ret=%d\n", ret);
            return ret;
        }
    }

    return OAL_SUCC;
}

OAL_STATIC oal_int32 oal_pcie_patch_read(hcc_bus *pst_bus, oal_uint8* buff, oal_int32 len, oal_uint32 timeout)
{
    oal_int32 ret;
    oal_pcie_linux_res * pst_pci_lres;

    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        return -OAL_ENODEV;
    }

    if(OAL_LIKELY(pst_pci_lres->pst_pci_res))
    {
        mutex_lock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
    }

    ret = oal_pcie_firmware_read(pst_pci_lres, buff, len, timeout);

    if(OAL_LIKELY(pst_pci_lres->pst_pci_res))
    {
        mutex_unlock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
    }

    return ret;
}

OAL_STATIC oal_int32 oal_pcie_patch_write(hcc_bus *pst_bus, oal_uint8* buff, oal_int32 len)
{
    oal_int32 ret;
    oal_pcie_linux_res * pst_pci_lres;

    pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        OAL_IO_PRINT("oal_pcie_patch_write failed, lres is null\n");
        return -OAL_ENODEV;
    }

    if(OAL_LIKELY(pst_pci_lres->pst_pci_res))
    {
        mutex_lock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
    }
    ret = oal_pcie_firmware_write(pst_pci_lres, buff, len);
    if(OAL_LIKELY(pst_pci_lres->pst_pci_res))
    {
        mutex_unlock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
    }
    return ret;
}

extern oal_atomic g_wakeup_dev_wait_ack_etc;
OAL_STATIC oal_int32 oal_pcie_gpio_irq(hcc_bus *hi_bus, oal_int32 irq)
{
    oal_uint                ul_state;
    oal_pcie_linux_res * pst_pci_lres;
    struct wlan_pm_s       *pst_wlan_pm = wlan_pm_get_drv_etc();
    pst_pci_lres = (oal_pcie_linux_res*)hi_bus->data;

    if(OAL_UNLIKELY(NULL == hi_bus))
    {
        OAL_IO_PRINT(KERN_ERR"pcie bus is null, irq:%d\n", irq);
        return -OAL_EINVAL;
    }

    if(!hi_bus->pst_pm_callback ||!hi_bus->pst_pm_callback->pm_state_get)
    {
        OAL_IO_PRINT("GPIO interrupt function param is NULL\r\n");
        return -OAL_EINVAL;
    }


    hi_bus->gpio_int_count++;

    if(oal_atomic_read(&g_wakeup_dev_wait_ack_etc))
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "pcie_gpio_irq wakeup dev ack");
        hi_bus->pst_pm_callback->pm_wakeup_dev_ack();
    }

    if(oal_atomic_read(&g_bus_powerup_dev_wait_ack))
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "pcie_gpio_irq powerup dev ack");
        if(OAL_LIKELY(NULL != pst_wlan_pm))
        {
            OAL_COMPLETE(&pst_wlan_pm->st_wifi_powerup_done);
        }
        else
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "pst_wlan_pm is null");
        }
    }

    if(OAL_UNLIKELY(NULL != pst_pci_lres))
    {
        if(oal_atomic_read(&pst_pci_lres->st_pcie_wakeup_flag))
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "pcie_gpio_irq dev ready ack");
            if(OAL_LIKELY(NULL != pst_wlan_pm))
            {
                OAL_COMPLETE(&pst_wlan_pm->st_wifi_powerup_done);
                up(&hi_bus->rx_sema);
            }
            else
            {
                PCI_PRINT_LOG(PCI_LOG_INFO, "pst_wlan_pm is null");
            }
        }
    }

	ul_state = hi_bus->pst_pm_callback->pm_state_get();

    if(0 == ul_state)
    {
        /*0==HOST_DISALLOW_TO_SLEEP表示不允许休眠*/
        hi_bus->data_int_count++;
        /*PCIE message use gpio interrupt*/
        PCI_PRINT_LOG(PCI_LOG_DBG, "pcie message come..");
        up(&hi_bus->rx_sema);

    }
    else
    {
        /*1==HOST_ALLOW_TO_SLEEP表示当前是休眠，唤醒host*/
        if(OAL_WARN_ON(!hi_bus->pst_pm_callback->pm_wakeup_host))
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "%s error:hi_bus->pst_pm_callback->pm_wakeup_host is null",__FUNCTION__);
            return -OAL_FAIL;
        }

        hi_bus->wakeup_int_count++;
        PCI_PRINT_LOG(PCI_LOG_INFO, "trigger wakeup work...");
        //OAL_IO_PRINT("[SDIO][DBG]Sdio Wakeup Interrrupt %llu,data intr %llu \r\n",hi_sdio->wakeup_int_count,hi_sdio->gpio_int_count);
        hi_bus->pst_pm_callback->pm_wakeup_host();

    }
    return OAL_SUCC;
}

OAL_STATIC oal_int32 oal_pcie_gpio_rx_data(hcc_bus *hi_bus)
{
    oal_int32 ret;
    oal_uint32 message;
    oal_pcie_linux_res* pst_pci_lres = (oal_pcie_linux_res*)hi_bus->data;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        return -OAL_EINVAL;
    }

    //hcc_bus_rx_transfer_unlock(hi_bus);
    /*read message from device, read until ringbuf empty*/
    for(;;)
    {

        if(!pst_pci_lres->pst_pci_res->regions.inited)
        {
            OAL_IO_PRINT("pcie rx data region is disabled!\n");
            return -OAL_ENODEV;
        }


        /*read one message*/
        hcc_bus_wake_lock(hi_bus);
        mutex_lock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
        ret = oal_pcie_read_d2h_message(pst_pci_lres->pst_pci_res, &message);
        mutex_unlock(&pst_pci_lres->pst_pci_res->st_rx_mem_lock);
        hcc_bus_wake_unlock(hi_bus);
        if(OAL_SUCC != ret)
        {
            /*TBD:DFR*/
            break;
        }

        PCI_PRINT_LOG(PCI_LOG_DBG, "pci  message : %u", message);
        /*get message succ.*/
        if(message < D2H_MSG_COUNT)
        {
            /*standard message*/
            hi_bus->msg[message].count++;
            hi_bus->last_msg = message;
            hi_bus->msg[message].cpu_time = cpu_clock(UINT_MAX);
            if(hi_bus->msg[message].msg_rx)
                hi_bus->msg[message].msg_rx(hi_bus->msg[message].data);
            else
                OAL_IO_PRINT("pcie mssage :%u no callback functions\n", message);
        }
        else
        {
            if(PCIE_D2H_MESSAGE_HOST_READY_ACK == message)
            {
                PCI_PRINT_LOG(PCI_LOG_INFO, "d2h host read ack");
                OAL_COMPLETE(&pst_pci_lres->st_pcie_ready);
            }
        }
    }
    //hcc_bus_rx_transfer_lock(hi_bus);
    return OAL_SUCC;
}

OAL_STATIC oal_void oal_pcie_print_trans_info(hcc_bus *hi_bus, oal_uint64 print_flag)
{
    oal_pcie_linux_res* pst_pci_lres = (oal_pcie_linux_res*)hi_bus->data;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        return ;
    }

    oal_pcie_print_transfer_info(pst_pci_lres->pst_pci_res, print_flag);
}

OAL_STATIC oal_void oal_pcie_reset_trans_info(hcc_bus *hi_bus)
{
    oal_pcie_linux_res* pst_pci_lres = (oal_pcie_linux_res*)hi_bus->data;
    if(OAL_WARN_ON(NULL == pst_pci_lres))
    {
        return ;
    }

    oal_pcie_reset_transfer_info(pst_pci_lres->pst_pci_res);
}

OAL_STATIC oal_int32 oal_pcie_pending_signal_check(hcc_bus *hi_bus)
{
    oal_pcie_linux_res* pst_pci_lres = (oal_pcie_linux_res*)hi_bus->data;
    if(OAL_UNLIKELY(NULL == pst_pci_lres))
    {
        PCI_PRINT_LOG(PCI_LOG_INFO,"pst_pci_lres is null");
        return OAL_FALSE;
    }
    return oal_pcie_host_pending_signal_check(pst_pci_lres->pst_pci_res);
}

OAL_STATIC oal_int32 oal_pcie_pending_signal_process(hcc_bus *hi_bus)
{
    oal_int32 ret = 0;
    oal_pcie_linux_res* pst_pci_lres = (oal_pcie_linux_res*)hi_bus->data;
    if(OAL_UNLIKELY(NULL == pst_pci_lres))
    {
        PCI_PRINT_LOG(PCI_LOG_INFO,"pst_pci_lres is null");
        return 0;
    }

    if(OAL_TRUE == oal_pcie_host_pending_signal_check(pst_pci_lres->pst_pci_res))
    {
        hcc_tx_transfer_lock(hi_bus->bus_dev->hcc);
        if(OAL_TRUE == oal_pcie_host_pending_signal_check(pst_pci_lres->pst_pci_res))/*for wlan power off*/
        {
            ret = oal_pcie_host_pending_signal_process(pst_pci_lres->pst_pci_res);
        }
        else
        {
            PCI_PRINT_LOG(PCI_LOG_INFO, "pcie tx pending signal was cleared");
        }
        hcc_tx_transfer_unlock(hi_bus->bus_dev->hcc);
    }
    return ret;
}


OAL_STATIC hcc_bus_opt_ops g_pcie_opt_ops =
{
    .get_bus_state      = oal_pcie_get_state,
    .disable_bus_state  = oal_disable_pcie_state,
    .enable_bus_state   = oal_enable_pcie_state,
    .rx_netbuf_list     = oal_pcie_rx_netbuf,
    .tx_netbuf_list     = oal_pcie_tx_netbuf,
    .send_msg_etc           = oal_pcie_send_msg,
    .lock               = oal_pcie_host_lock,
    .unlock             = oal_pcie_host_unlock,
    .sleep_request      = oal_pcie_sleep_request,
    .sleep_request_host = oal_pcie_sleep_request_host,
    .wakeup_request     = oal_pcie_wakeup_request,
    .get_sleep_state    = oal_pcie_get_sleep_state,
    .wakeup_complete    = oal_pcie_wakeup_complete,
    .rx_int_mask        = oal_pcie_rx_int_mask,
    .power_action       = oal_pcie_power_action,
    .power_ctrl         = oal_pcie_power_ctrl,
    .wlan_gpio_handler  = oal_pcie_gpio_irq,
    .wlan_gpio_rxdata_proc = oal_pcie_gpio_rx_data,
    .reinit             = oal_pcie_reinit,
    .deinit             = oal_pcie_deinit,
    .tx_condition       = oal_pcie_tx_condition,
    .patch_read         = oal_pcie_patch_read,
    .patch_write        = oal_pcie_patch_write,
    .bindcpu            = oal_pcie_bindcpu,
    .switch_suspend_tx  = NULL,
    .get_trans_count    = oal_pcie_get_trans_count,
    .switch_clean_res   = oal_pcie_switch_clean_res,
    .voltage_bias_init  = oal_pcie_ip_voltage_bias_init,
    .chip_info          = oal_pcie_chip_info,

    .print_trans_info   = oal_pcie_print_trans_info,
    .reset_trans_info   = oal_pcie_reset_trans_info,
    .pending_signal_check = oal_pcie_pending_signal_check,
    .pending_signal_process = oal_pcie_pending_signal_process
};

OAL_STATIC hcc_bus* oal_pcie_bus_init(oal_pcie_linux_res * pst_pci_lres)
{
    oal_int32 ret;
    hcc_bus       *pst_bus = OAL_PTR_NULL;

    pst_bus = hcc_alloc_bus();
    if(OAL_PTR_NULL == pst_bus)
    {
        OAL_IO_PRINT("alloc pcie hcc bus failed, size:%u\n", (oal_uint32)OAL_SIZEOF(hcc_bus));
        return NULL;
    }

    pst_bus->bus_type = HCC_BUS_PCIE;
    pst_bus->bus_id   = 0x0;
    pst_bus->dev_id = HCC_CHIP_110X_DEV;/*这里可以根据 vendor id 区分110X 和118X*/

    /*PCIE 只需要4字节对齐, burst大小对性能的影响有限*/
    pst_bus->cap.align_size[HCC_TX] = 4;
    pst_bus->cap.align_size[HCC_RX] = 4;
    pst_bus->cap.max_trans_size = 0x7fffffff;

    pst_bus->opt_ops  = &g_pcie_opt_ops;

    pst_bus->data = (oal_void*)pst_pci_lres;

    ret = hcc_add_bus(pst_bus, "pcie");
    if(ret)
    {
        OAL_IO_PRINT("add pcie bus failed, ret=%d\n", ret);
        hcc_free_bus(pst_bus);
        return NULL;
    }

    pst_pci_lres->pst_bus = pst_bus;
    return pst_bus;
}

OAL_STATIC oal_void oal_pcie_bus_exit(hcc_bus* pst_bus)
{
    hcc_remove_bus(pst_bus);
    hcc_free_bus(pst_bus);
}

#ifdef CONFIG_ARCH_KIRIN_PCIE
oal_int32 wlan_first_power_on_callback(void* data)
{
    OAL_REFERENCE(data);
    /*before wlan chip power up, rc clock must on.*/
    /*bootloader had delay 20us before reset pcie,
      soc requet 10us delay, need't delay here*/
    hi_wlan_power_set_etc(1);
    return 0;
}

oal_int32 wlan_first_power_off_fail_callback(void* data)
{
    OAL_REFERENCE(data);
    /*阻止麒麟枚举失败后下电关时钟*/
    OAL_IO_PRINT("wlan_first_power_off_fail_callback\n");
    g_pcie_enum_fail_reg_dump_flag = 1;
    if(HI1XX_ANDROID_BUILD_VARIANT_USER == hi11xx_get_android_build_variant())
    {
        if(oal_print_rate_limit(PRINT_RATE_MINUTE))
            ssi_dump_device_regs(SSI_MODULE_MASK_ARM_REG|SSI_MODULE_MASK_AON_CUT|SSI_MODULE_MASK_PCIE_CUT);
    }
    else
    {
        ssi_dump_device_regs(SSI_MODULE_MASK_COMM|SSI_MODULE_MASK_PCIE_CFG|SSI_MODULE_MASK_PCIE_DBI);
    }
    return -1;
}

oal_int32 wlan_first_power_off_callback(void* data)
{
    OAL_REFERENCE(data);
    hi_wlan_power_set_etc(0);
    return 0;
}
#endif

oal_void oal_pcie_voltage_bias_param_init(oal_void)
{
    char *token;
    char bias_param[100];
    oal_uint32 param[10];
    oal_uint32 param_nums;
    char* bias_buf = bias_param;

    OAL_MEMZERO(bias_param, sizeof(bias_param));
    OAL_MEMZERO(param, sizeof(param));

    if(INI_FAILED == get_cust_conf_string_etc(INI_MODU_PLAT, "pcie_vol_bias_param", bias_param, sizeof(bias_param) - 1))
    {
        return;
    }

    param_nums = 0;
    for(;;)
    {
        token = strsep(&bias_buf, ",");
        if(NULL == token)
            break;
        if(param_nums >= sizeof(param)/sizeof(oal_uint32))
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "too many bias param:%u", param_nums);
            return;
        }

        param[param_nums++] = oal_strtol(token, NULL, 10);
        PCI_PRINT_LOG(PCI_LOG_INFO, "bias param %u , value is %u", param_nums, param[param_nums - 1]);
    }

    oal_pcie_set_voltage_bias_param(param[0], param[1]);
}

oal_void oal_pcie_ringbuf_bugfix_init(oal_void)
{
    char buff[100];

    OAL_MEMZERO(buff, sizeof(buff));

    if(INI_FAILED == get_cust_conf_string_etc(INI_MODU_PLAT, "pcie_ringbuf_bugfix", buff, sizeof(buff) - 1))
    {
        PCI_PRINT_LOG(PCI_LOG_INFO, "can't found ini:pcie_ringbuf_bugfix, bugfix stat:%d", pcie_ringbuf_bugfix_enable);
        return;
    }

    if(!oal_strncmp("enable", buff, OAL_STRLEN("enable")))
    {
        pcie_ringbuf_bugfix_enable = 1;
    }
    else if(!strncmp("disable", buff, OAL_STRLEN("disable")))
    {
        pcie_ringbuf_bugfix_enable = 0;
    }
    else
    {
        PCI_PRINT_LOG(PCI_LOG_WARN, "unkdown ringbuf bugfix ini:%s", buff);
    }

    PCI_PRINT_LOG(PCI_LOG_INFO, "ringbuf bugfix %s", pcie_ringbuf_bugfix_enable ? "enable":"disable");
}

/*trigger pcie probe, alloc pcie resource*/
oal_int32 oal_pcie_110x_init(oal_void)
{
    oal_int32 ret;

    /*init host pcie*/
    OAL_IO_PRINT("%s PCIe driver register start\n", g_st_pcie_drv.name);/*Debug*/
    OAL_INIT_COMPLETION(&g_probe_complete);


    g_probe_ret = -OAL_EFAUL;

    g_pcie_enum_fail_reg_dump_flag = 0;

    ret = oal_pci_register_driver(&g_st_pcie_drv);
    OAL_IO_PRINT("%s PCIe driver register end\n", g_st_pcie_drv.name);
    if(ret)
    {
        OAL_IO_PRINT("pcie driver register failed ret=%d, driname:%s\n", ret, g_st_pcie_drv.name);
        return ret;
    }

#ifdef CONFIG_ARCH_SD56XX
    oal_pci_wlan_power_on(OAL_TRUE);
#endif
#ifdef CONFIG_ARCH_KIRIN_PCIE
    /*打开参考时钟*/
    ret = kirin_pcie_power_notifiy_register(kirin_rc_idx, wlan_first_power_on_callback, wlan_first_power_off_fail_callback, NULL);
    if(OAL_SUCC != ret)
    {
        OAL_IO_PRINT("enumerate_prepare failed ret=%d\n", ret);
        oal_pci_unregister_driver(&g_st_pcie_drv);
        return ret;
    }

    ret = oal_pcie_enumerate();
    if(OAL_SUCC != ret)
    {
        if(!g_pcie_enum_fail_reg_dump_flag)
        {
            if(HI1XX_ANDROID_BUILD_VARIANT_USER == hi11xx_get_android_build_variant())
            {
                if(oal_print_rate_limit(PRINT_RATE_MINUTE))
                    ssi_dump_device_regs(SSI_MODULE_MASK_ARM_REG|SSI_MODULE_MASK_AON_CUT|SSI_MODULE_MASK_PCIE_CUT);
            }
            else
            {
                ssi_dump_device_regs(SSI_MODULE_MASK_COMM|SSI_MODULE_MASK_PCIE_CFG|SSI_MODULE_MASK_PCIE_DBI);
            }
        }
        OAL_IO_PRINT("enumerate failed ret=%d\n", ret);
        kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL, NULL, NULL);
        oal_pci_unregister_driver(&g_st_pcie_drv);
        return ret;
    }
#endif

    if(oal_wait_for_completion_timeout(&g_probe_complete, 10*HZ))
    {
        if(OAL_SUCC != g_probe_ret)
        {
            OAL_IO_PRINT("pcie driver probe failed ret=%d, driname:%s\n", g_probe_ret, g_st_pcie_drv.name);
#ifdef CONFIG_ARCH_KIRIN_PCIE
            kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL, NULL, NULL);
#endif
            return g_probe_ret;
        }
    }
    else
    {
        OAL_IO_PRINT("pcie driver probe timeout  driname:%s\n", g_st_pcie_drv.name);
		if(!g_pcie_enum_fail_reg_dump_flag)
		{
            if(HI1XX_ANDROID_BUILD_VARIANT_USER == hi11xx_get_android_build_variant())
            {
                if(oal_print_rate_limit(PRINT_RATE_MINUTE))
                    ssi_dump_device_regs(SSI_MODULE_MASK_ARM_REG|SSI_MODULE_MASK_AON_CUT|SSI_MODULE_MASK_PCIE_CUT);
            }
            else
            {
                ssi_dump_device_regs(SSI_MODULE_MASK_COMM|SSI_MODULE_MASK_PCIE_CFG|SSI_MODULE_MASK_PCIE_DBI);
            }
		}
#ifdef CONFIG_ARCH_KIRIN_PCIE
        kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL, NULL, NULL);
#endif
        return -OAL_ETIMEDOUT;
    }

    oal_pcie_voltage_bias_param_init();
    oal_pcie_ringbuf_bugfix_init();

    OAL_IO_PRINT("%s PCIe driver register succ\n", g_st_pcie_drv.name);

#ifdef CONFIG_ARCH_SD56XX
    oal_pcie_save_default_resource(g_pcie_linux_res);
#else
    /*Power off Wlan Chip*/
    if(g_pcie_linux_res)
    {
        hcc_bus_disable_state(g_pcie_linux_res->pst_bus, OAL_BUS_STATE_ALL);
        /*保存PCIE 配置寄存器*/
        ret = oal_pcie_save_default_resource(g_pcie_linux_res);
        if(OAL_SUCC != ret)
        {
            oal_pcie_disable_device(g_pcie_linux_res);
            oal_disable_pcie_irq(g_pcie_linux_res);
            oal_pci_unregister_driver(&g_st_pcie_drv);
            hi_wlan_power_set_etc(0);
#ifdef CONFIG_ARCH_KIRIN_PCIE
            kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL, NULL, NULL);
#endif
            return ret;
        }
#ifndef HAVE_HISI_NFC
#ifdef CONFIG_ARCH_KIRIN_PCIE
        kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL, wlan_first_power_off_callback, NULL);
#endif
        oal_disable_pcie_irq(g_pcie_linux_res);
        oal_pcie_power_action(g_pcie_linux_res->pst_bus, HCC_BUS_POWER_DOWN);
        /*等到读取完nfc低电的log数据再拉低GPIO*/
        hi_wlan_power_set_etc(0);
#endif
    }
    else
    {
        OAL_IO_PRINT("g_pcie_linux_res is null\n");
#ifdef CONFIG_ARCH_KIRIN_PCIE
        kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL, NULL, NULL);
#endif
        return -OAL_ENODEV;
    }
#endif
    return OAL_SUCC;
}

oal_void oal_pcie_110x_exit(oal_void)
{
#ifdef CONFIG_ARCH_KIRIN_PCIE
    kirin_pcie_power_notifiy_register(kirin_rc_idx, NULL, NULL, NULL);
#endif
#ifdef CONFIG_ARCH_SD56XX
    oal_pcie_rc_mem_unmap();
#endif
    oal_pci_unregister_driver(&g_st_pcie_drv);
}
#endif

oal_int32 oal_pcie_110x_working_check(oal_void)
{
    hcc_bus_dev* pst_bus_dev;
    pst_bus_dev = hcc_get_bus_dev(HCC_CHIP_110X_DEV);

    if (NULL == pst_bus_dev)
    {
        return OAL_FALSE;
    }

    if(pst_bus_dev->bus_cap & HCC_BUS_PCIE_CAP)
    {
        return OAL_TRUE;
    }
    else
    {
        return OAL_FALSE;
    }
}

#ifdef CONFIG_PCIE_KIRIN_SLT_HI110X
extern oal_int32 hi1103_pcie_chip_rc_slt_register(oal_void);
extern oal_int32 hi1103_pcie_chip_rc_slt_unregister(oal_void);
#endif
oal_int32 oal_wifi_platform_load_pcie(oal_void)
{
    oal_int32 ret = OAL_SUCC;

    if(OAL_TRUE != oal_pcie_110x_working_check())
    {
        /*pcie driver don't support*/
        OAL_IO_PRINT("pcie driver don't support\n");
        return OAL_SUCC;
    }

    /*WiFi 芯片上电 + PCIE 枚举*/
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
    ret = oal_pcie_110x_init();
#endif
#ifdef CONFIG_PCIE_KIRIN_SLT_HI110X
    if(OAL_SUCC == ret)
    {
        hi1103_pcie_chip_rc_slt_register();
    }
#endif
    return ret;
}

oal_void oal_wifi_platform_unload_pcie(oal_void)
{
    if(OAL_TRUE != oal_pcie_110x_working_check())
    {
        return;
    }

#ifdef CONFIG_PCIE_KIRIN_SLT_HI110X
    hi1103_pcie_chip_rc_slt_unregister();
#endif
}

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
