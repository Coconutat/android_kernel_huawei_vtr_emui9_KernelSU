
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
/*lint -e801*/
#define HISI_LOG_TAG "[HCC_BUS]"
#define HI11XX_LOG_MODULE_NAME "[HCC_BUS]"
#define HI11XX_LOG_MODULE_NAME_VAR hcc_bus_loglevel
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "board.h"
#include "plat_exception_rst.h"
#endif
#include "oal_hcc_bus.h"
#include "oal_sdio_host_if.h"
#include "oal_pcie_linux.h"
#include "oal_hcc_host_if.h"
#include "oam_ext_if.h"
#include "plat_pm_wlan.h"
#include "plat_firmware.h"

#ifndef WIN32
#include "plat_pm.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_HCC_BUS_C


/*****************************************************************************
  2 全局变量定义
*/
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
static oal_int32 hcc_bus_pm_state_switch_event(struct notifier_block *unused,
                                                        unsigned long wifi_state, void *ptr);
OAL_STATIC OAL_DLIST_CREATE_HEAD(g_hcc_bus_res_hdr);
OAL_STATIC OAL_DEFINE_SPINLOCK(g_hcc_bus_res_lock);
OAL_STATIC OAL_DLIST_CREATE_HEAD(g_hcc_bus_switch_hdr);

static struct notifier_block hcc_bus_pm_notifier = {
	.notifier_call = hcc_bus_pm_state_switch_event,
};

/*Set Format, split with ",", like below
  "sdio,pcie,switchon" -- board support sdio & pcie, enable dynamic switch
  "sdio,pcie,switchoff"
  "pcie,sdio"          -- board support pcie & sdio, default we use pcie
  "sdio"               -- only support sdio
  "pcie"               -- only support pcie
  */
char* hisi_wifi_bus_select = "";
module_param(hisi_wifi_bus_select, charp, S_IRUGO | S_IWUSR);

oal_uint32 hcc_exception_enable = 1;
module_param(hcc_exception_enable, uint, S_IRUGO|S_IWUSR);

oal_uint32 hcc_bus_switch_bypass = 0;
module_param(hcc_bus_switch_bypass, uint, S_IRUGO|S_IWUSR);

oal_uint32 hcc_bus_switch_test_delay_time = 0;
module_param(hcc_bus_switch_test_delay_time, uint, S_IRUGO|S_IWUSR);

oal_uint32 hcc_bus_switch_test_break = 0;
module_param(hcc_bus_switch_test_break, uint, S_IRUGO|S_IWUSR);

oal_uint32 hcc_bus_auto_switch = 0;
module_param(hcc_bus_auto_switch, uint, S_IRUGO|S_IWUSR);

oal_uint32 hcc_bus_auto_bindcpu = 0;
module_param(hcc_bus_auto_bindcpu, uint, S_IRUGO|S_IWUSR);

oal_uint32 hcc_bus_manual_bindcpu = 1;
module_param(hcc_bus_manual_bindcpu, uint, S_IRUGO|S_IWUSR);

/*pps 按照大包计算*/
oal_uint32 hcc_bus_auto_bindcpu_limit = (200*1024*128)/1500;
module_param(hcc_bus_auto_bindcpu_limit, uint, S_IRUGO|S_IWUSR);

oal_uint32 hcc_bus_current_pps = 0x0;
module_param(hcc_bus_current_pps, uint, S_IRUGO);

oal_uint32 hcc_bus_wakelock_debug = 0x0;
module_param(hcc_bus_wakelock_debug, uint, S_IRUGO|S_IWUSR);

oal_uint32 hcc_bus_auto_switch_limit       = (350*1024*128)/1500;
module_param(hcc_bus_auto_switch_limit, uint, S_IRUGO|S_IWUSR);

char str_ini_hcc_bus_switch[100] = {0};
module_param_string(ini_hcc_bus_switch, str_ini_hcc_bus_switch,
                    sizeof(str_ini_hcc_bus_switch), S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(ini_hcc_bus_switch, "Ini string for hcc bus switch");

#else
oal_dlist_head_stru g_hcc_bus_res_hdr;
oal_spin_lock_stru  g_hcc_bus_res_lock;
oal_dlist_head_stru g_hcc_bus_switch_hdr;
char* hisi_wifi_bus_select = "";
oal_uint32 hcc_exception_enable = 1;
oal_uint32 hcc_bus_switch_bypass = 0;
oal_uint32 hcc_bus_switch_test_delay_time = 0;
oal_uint32 hcc_bus_switch_test_break = 0;
char str_ini_hcc_bus_switch[100];
oal_uint32 hcc_bus_auto_switch = 0;
oal_uint32 hcc_bus_auto_bindcpu = 0;
oal_uint32 hcc_bus_manual_bindcpu = 1;
oal_uint32 hcc_bus_current_pps = 0x0;
oal_uint32 hcc_bus_wakelock_debug = 0x0;
/*pps 按照大包计算*/
oal_uint32 hcc_bus_auto_sdio_bindcpu_limit = (300*1024*128)/1500;
oal_uint32 hcc_bus_auto_pcie_bindcpu_limit = (450*1024*128)/1500;
oal_uint32 hcc_bus_auto_switch_limit       = (350*1024*128)/1500;
#endif

oal_uint32 g_switch_ip_pwrdown_bypass = 1;

#ifdef WIN32
oal_uint32 jiffies;
#else
module_param(g_switch_ip_pwrdown_bypass, uint, S_IRUGO|S_IWUSR);
#endif

oal_int32  g_switch_pwr_ret;/*TBD*/
oal_int32  g_switch_total_count = 0;
oal_uint32 g_hcc_tx_err_cnt = 0;
extern oal_bool_enum g_wlan_pm_switch_etc;
/*****************************************************************************
  3 函数声明
*****************************************************************************/
oal_int32 oal_register_gpio_intr_etc(hcc_bus_dev *pst_bus_dev);
oal_void  oal_unregister_gpio_intr_etc(hcc_bus_dev *pst_bus_dev);
OAL_STATIC oal_void hcc_bus_sched_gpio_task(hcc_bus*     pst_bus, oal_int32 irq);
OAL_STATIC oal_void hcc_dev_pps_count_timeout(oal_uint arg);
oal_int32 hcc_bus_auto_switch_is_support(oal_uint32 dev_id);
oal_int32 hcc_bus_auto_bindcpu_is_support(oal_uint32 dev_id);
oal_int32 hcc_bus_auto_sched_is_support(oal_uint32 dev_id);
oal_int32 hcc_switch_bus_request(oal_uint32 dev_id, oal_uint32 target, oal_int32 is_sync);

/*****************************************************************************
  4 函数实现
*****************************************************************************/

hcc_bus*  g_hisi_wifi_bus = OAL_PTR_NULL;
//struct semaphore bus_wake_sema;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
/*根据以下结构体来枚举 IP设备*/
OAL_STATIC hcc_bus_dev g_bus_dev_res[] =
{
    {
        .dev_id = HCC_CHIP_110X_DEV,
#if defined(CONFIG_ARCH_SD56XX)
        .is_wakeup_gpio_support = 0,
        .init_bus_type = HCC_BUS_PCIE,
#else
        .init_bus_type = HCC_BUS_SDIO,
        .is_wakeup_gpio_support = 1,
#endif
#if defined(CONFIG_ARCH_SD56XX)
        .bus_cap = (HCC_BUS_PCIE_CAP),
#elif defined(_PRE_PLAT_FEATURE_HI110X_PCIE)
        .bus_cap = (HCC_BUS_SDIO_CAP ) |  (HCC_BUS_PCIE_CAP),
#else
        .bus_cap = (HCC_BUS_SDIO_CAP),
#endif
        .name = "hi110x_dev",
    },
};
#else
OAL_STATIC hcc_bus_dev g_bus_dev_res[10];
#endif


oal_uint32       bus_dump_mem_flag = 0;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
module_param(bus_dump_mem_flag, uint, S_IRUGO | S_IWUSR);

oal_atomic g_wakeup_dev_wait_ack_etc;
oal_atomic g_bus_powerup_dev_wait_ack;/*ip 上电握手标记*/
#endif

/*-1 means did't bind cpus*/
OAL_STATIC oal_int32 g_lCpuId = -1;

extern oal_void  oal_sdio_func_remove_etc(struct oal_sdio* hi_sdio);
extern oal_void  oal_sdio_exit_module_etc(struct oal_sdio* hi_sdio);
//extern  oal_int32 oal_sdio_send_msg_etc(struct oal_sdio* hi_sdio, oal_uint val);

#ifdef WIN32
char* g_hcc_bus_type_str[HCC_BUS_BUTT] =
{
    "sdio",
    "pcie",
    "usb"
};
#else
char* g_hcc_bus_type_str[HCC_BUS_BUTT] =
{
    [HCC_BUS_SDIO] = "sdio",
    [HCC_BUS_PCIE] = "pcie",
    [HCC_BUS_USB] = "usb"
};
#endif

char* hcc_bus_get_bus_type_str(oal_uint32 bus_type)
{
    if(OAL_UNLIKELY(bus_type >= HCC_BUS_BUTT))
    {
        return "unkown";
    }

    return g_hcc_bus_type_str[bus_type] ? g_hcc_bus_type_str[bus_type] : "uninit";
}

oal_void hcc_bus_stop_auto_pps_task(hcc_bus_dev* pst_bus_dev)
{
    /*cancel the timer*/
    oal_timer_delete_sync(&pst_bus_dev->bus_pps_timer);
    pst_bus_dev->hcc->hcc_transer_info.tx_assem_info.pkt_cnt = 0;
    pst_bus_dev->hcc->hcc_transer_info.rx_assem_info.pkt_cnt = 0;
    hcc_dev_bindcpu(pst_bus_dev->dev_id, 0);
}

oal_void hcc_bus_start_auto_pps_task(hcc_bus_dev* pst_bus_dev)
{
    /*start the timer*/
    pst_bus_dev->bus_pps_start_time = jiffies;
    oal_timer_start(&pst_bus_dev->bus_pps_timer, HCC_BUS_PPS_COUNT_TIMEOUT);
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
static oal_int32 hcc_bus_pm_state_switch_event(struct notifier_block *unused,
				  unsigned long wifi_state, void *ptr)
{
    hcc_bus_dev* pst_bus_dev;
    pst_bus_dev = hcc_get_bus_dev(HCC_CHIP_110X_DEV);
    if(NULL == pst_bus_dev)
    {
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "110x dev is null");
        return -OAL_ENODEV;
    }

    if(WIFI_PM_POWERUP_EVENT == wifi_state)
    {
        g_switch_total_count = 0;
    }

    if(!hcc_bus_auto_sched_is_support(pst_bus_dev->dev_id))
    {
        return OAL_SUCC;
    }

    if(WIFI_PM_WAKEUP_EVENT == wifi_state)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "110x dev wakeup");
        hcc_bus_start_auto_pps_task(pst_bus_dev);
    }
    else if(WIFI_PM_SLEEP_EVENT == wifi_state)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "110x dev sleep");
        hcc_bus_stop_auto_pps_task(pst_bus_dev);
    }
    else if(WIFI_PM_POWERDOWN_EVENT == wifi_state)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "110x dev powerdown");
        hcc_bus_stop_auto_pps_task(pst_bus_dev);
    }
    else if(WIFI_PM_POWERUP_EVENT == wifi_state)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "110x dev powerup");
        hcc_bus_start_auto_pps_task(pst_bus_dev);
    }

    return 0;
}
#endif

oal_int32  hcc_switch_action_register(hcc_switch_action* action, void* data)
{
    action->data = data;
    oal_dlist_add_tail(&action->list, &g_hcc_bus_switch_hdr);

    return OAL_SUCC;
}

oal_void hcc_switch_action_unregister(hcc_switch_action* action)
{
    oal_dlist_delete_entry(&action->list);
}

oal_void hcc_bus_chan_init_from_string(hcc_bus_dev*  pst_bus_dev, char* bus_select)
{
    int len;
    char* start, *end;
    char* default_select = "";

    oal_uint32 init_bus_type = HCC_BUS_BUTT;

    if(0 != OAL_STRLEN(str_ini_hcc_bus_switch))
    {
       start = str_ini_hcc_bus_switch;
       oal_print_hi11xx_log(HI11XX_LOG_INFO, "get ini string from module para [%s]", str_ini_hcc_bus_switch);
    }
    else
    {
       start = bus_select;
    }

    end   = start;

    pst_bus_dev->bus_cap = 0;

    for(;;)
    {
        if(','==*end || '\0'==*end)
        {
            len = end - start;
            if(len)
            {
                if(!oal_memcmp(start, "sdio", OAL_STRLEN("sdio")))
                {
                    pst_bus_dev->bus_cap |= HCC_BUS_SDIO_CAP;
                    if(HCC_BUS_BUTT == init_bus_type)
                    {
                        init_bus_type = HCC_BUS_SDIO;
                        default_select = "sdio";
                    }
                    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hisi wifi support sdio %s", default_select);
                }

                if(!oal_memcmp(start, "pcie", OAL_STRLEN("pcie")))
                {
                    pst_bus_dev->bus_cap |= HCC_BUS_PCIE_CAP;
                    if(HCC_BUS_BUTT == init_bus_type)
                    {
                        init_bus_type = HCC_BUS_PCIE;
                        default_select = "pcie";
                    }
                    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hisi wifi support pcie %s", default_select);
                }

                /*动态切换开关，由驱动来仲裁哪2种接口切换*/
                if(!oal_memcmp(start, "switchon", OAL_STRLEN("switchon")))
                {
                    pst_bus_dev->bus_switch_enable = 1;
                    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hisi wifi enable dynamic bus switch");
                }

                if(!oal_memcmp(start, "switchoff", OAL_STRLEN("switchoff")))
                {
                    pst_bus_dev->bus_switch_enable = 0;
                    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hisi wifi disable dynamic bus switch");
                }

                if(!oal_memcmp(start, "auto_switch", OAL_STRLEN("auto_switch")))
                {
                    pst_bus_dev->bus_auto_switch = 1;
                    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hisi hcc bus auto switch enable");
                }

                if(!oal_memcmp(start, "auto_bindcpu", OAL_STRLEN("auto_bindcpu")))
                {
                    pst_bus_dev->bus_auto_bindcpu = 1;
                    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hisi hcc bus auto bindcpu enable");
                }
            }

            start = end + 1;
        }

        if('\0'==*end)
        {
            break;
        }

        ++end;
    }

    if(HCC_BUS_BUTT != init_bus_type)
    {
        pst_bus_dev->init_bus_type = init_bus_type;
    }
}

oal_int32 hcc_bus_cap_init(oal_uint32 dev_id, char* bus_select)
{
    hcc_bus_dev* pst_bus_dev;

    pst_bus_dev = hcc_get_bus_dev(dev_id);
    if(NULL == pst_bus_dev)
    {
        return -OAL_ENODEV;
    }

    if(NULL != hisi_wifi_bus_select &&
       0 != OAL_STRLEN(hisi_wifi_bus_select))
    {
        hcc_bus_chan_init_from_string(pst_bus_dev, hisi_wifi_bus_select);
        return OAL_SUCC;
    }

    if(NULL != bus_select &&
       0 != OAL_STRLEN(bus_select))
    {
        hcc_bus_chan_init_from_string(pst_bus_dev, bus_select);
        return OAL_SUCC;
    }

    /*default use sdio*/
    hcc_bus_chan_init_from_string(pst_bus_dev, "sdio");

    return OAL_SUCC;
}

/*This interface is not good, remove later*/
hcc_bus* hcc_get_current_110x_bus(oal_void)
{
    hcc_bus_dev* pst_bus_dev;
    pst_bus_dev = hcc_get_bus_dev(HCC_CHIP_110X_DEV);
    if(NULL == pst_bus_dev)
    {
        return NULL;
    }

    return pst_bus_dev->cur_bus;
}

oal_int32 hcc_bus_check_dev_id(oal_uint32 dev_id)
{
    if(OAL_WARN_ON(0xFFFFFFFF == dev_id))
    {
        return -OAL_ENODEV;
    }

    if(OAL_WARN_ON(dev_id >= (oal_uint32)OAL_ARRAY_SIZE(g_bus_dev_res)))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "invalid dev id :%d", dev_id);
        return -OAL_EINVAL;
    }

    return OAL_SUCC;
}

oal_int32 hcc_bus_is_match(hcc_bus* pst_new, hcc_bus* pst_old)
{
    if(pst_new->bus_type != pst_old->bus_type)
    {
        return OAL_FALSE;
    }

    if(pst_new->bus_id != pst_old->bus_id)
    {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}

hcc_bus_dev* hcc_get_bus_dev(oal_uint32 dev_id)
{
    oal_int32 i;
    hcc_bus_dev *pst_bus_dev;

    if(OAL_SUCC != hcc_bus_check_dev_id(dev_id))
    {
        return NULL;
    }

    for(i = 0; i < (oal_int32)OAL_ARRAY_SIZE(g_bus_dev_res); i++)
    {
        pst_bus_dev = &g_bus_dev_res[i];
        if(dev_id == pst_bus_dev->dev_id)
        {
            return pst_bus_dev;
        }
    }

    return NULL;
}

oal_int32 hcc_foreach_switch_actions(oal_uint32 dev_id,hcc_bus* old_bus ,hcc_bus* new_bus)
{
    oal_int32 ret = OAL_SUCC;
    oal_dlist_head_stru  *pst_entry;
    hcc_switch_action* pst_action;

    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &g_hcc_bus_switch_hdr)
    {
        pst_action = OAL_DLIST_GET_ENTRY(pst_entry, hcc_switch_action, list);
        if(NULL != pst_action->switch_notify)
        {
            ret = pst_action->switch_notify(dev_id, old_bus, new_bus, pst_action->data);
            if(OAL_SUCC != ret)
            {
                oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc switch action probe failed,name:%s, ret=%d,func:%p",
                                pst_action->name?pst_action->name:"unkown", ret, pst_action->switch_notify);
                return ret;
            }
        }
    }

    return ret;
}

hcc_bus* hcc_find_bus_by_devid(oal_uint32 dev_id, oal_uint32 bus_type)
{
    oal_uint             irqsave;
    oal_dlist_head_stru  *pst_entry;
    hcc_bus*             pst_bus_tmp;

    hcc_bus_dev* pst_bus_dev  = hcc_get_bus_dev(dev_id);
    if(NULL == pst_bus_dev)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "dev_id %u invalid", dev_id);
        return NULL;
    }

    oal_spin_lock_irq_save(&g_hcc_bus_res_lock, &irqsave);
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &g_hcc_bus_res_hdr)
    {
        pst_bus_tmp = OAL_DLIST_GET_ENTRY(pst_entry, hcc_bus, list);

        /*found the bus*/
        if(pst_bus_tmp->bus_type == bus_type)
        {
            oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);
            return pst_bus_tmp;
        }
    }
    oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);

    return NULL;
}

oal_int32 hcc_select_bus(oal_uint32 dev_id, oal_uint32 bus_type)
{
    oal_uint             irqsave;
    oal_dlist_head_stru  *pst_entry;
    hcc_bus*             pst_bus_tmp;

    hcc_bus_dev* pst_bus_dev  = hcc_get_bus_dev(dev_id);
    if(NULL == pst_bus_dev)
    {
        return -OAL_EINVAL;
    }

    oal_spin_lock_irq_save(&g_hcc_bus_res_lock, &irqsave);
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &g_hcc_bus_res_hdr)
    {
        pst_bus_tmp = OAL_DLIST_GET_ENTRY(pst_entry, hcc_bus, list);
        if(pst_bus_tmp->bus_dev == pst_bus_dev)
        {
            if(pst_bus_tmp->bus_type == bus_type)
            {
                /*match*/
                pst_bus_dev->cur_bus = pst_bus_tmp;
                oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);
                return OAL_SUCC;
            }
        }
    }
    oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);

    return -OAL_ENODEV;
}

/*switch bus from sdio to pcie or pcie to sdio*/
oal_int32 hcc_switch_bus(oal_uint32 dev_id, oal_uint32 bus_type)
{
    oal_int32 ret = OAL_SUCC;
    hcc_bus* old_bus;
    hcc_bus_dev* pst_bus_dev  = hcc_get_bus_dev(dev_id);
    /*暂时只允许在关机状态切换*/
    if(OAL_WARN_ON(HCC_CHIP_110X_DEV != dev_id))
    {
        return -OAL_EINVAL;
    }

    if(NULL == pst_bus_dev)
    {
        return -OAL_EINVAL;
    }

    if(OAL_WARN_ON(NULL == pst_bus_dev->cur_bus))
    {
        return -OAL_ENODEV;
    }

#if 0
    if(OAL_SUCC == hcc_switch_bus_request(dev_id, bus_type, 1))
    {
        return OAL_SUCC;
    }

    /*WiFi power down*/
#endif

    oal_wake_lock(&pst_bus_dev->st_switch_wakelock);

    old_bus = pst_bus_dev->cur_bus;

    ret = hcc_select_bus(dev_id, bus_type);
    if(OAL_SUCC == ret)
    {
        /*call switch bus actions after bus changed*/
        ret = hcc_foreach_switch_actions(dev_id, old_bus, pst_bus_dev->cur_bus);
    }

    oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
    hcc_sched_transfer(pst_bus_dev->hcc);
    return ret;
}

oal_int32 hcc_bus_isvalid(hcc_bus* pst_bus)
{
    if(NULL == pst_bus)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "bus is null");
        return -OAL_EINVAL;
    }

    if(NULL == pst_bus->bus_dev)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "bus %d 's bus dev is null", pst_bus->bus_id);
        return -OAL_ENODEV;
    }

#if 0
    if(NULL == pst_bus->hcc)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "bus %d 's hcc handler is null", pst_bus->bus_id);
        return -OAL_ENODEV;
    }
#endif

    return OAL_SUCC;
}

OAL_STATIC oal_int32 hcc_bus_rx_response(struct hcc_handler * hcc, oal_uint8 stype, hcc_netbuf_stru* pst_hcc_netbuf, oal_uint8 *pst_context)
{
    hcc_switch_response st_switch_info_resp;
    oal_uint16 transfer_type;

    OAL_REFERENCE(pst_context);

    oal_memcopy((oal_void*)&transfer_type, (oal_void*)oal_netbuf_data(pst_hcc_netbuf->pst_netbuf), OAL_SIZEOF(transfer_type));

    if(HCC_BUS_TRANSFER_SWITCH_ACTION == transfer_type)
    {
        if(OAL_NETBUF_LEN(pst_hcc_netbuf->pst_netbuf) < OAL_SIZEOF(transfer_type) + OAL_SIZEOF(st_switch_info_resp))
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_rx_response netbuf too short, len %d < %d", (oal_int32)OAL_NETBUF_LEN(pst_hcc_netbuf->pst_netbuf), (oal_int32)(OAL_SIZEOF(transfer_type) + OAL_SIZEOF(st_switch_info_resp)));
            oal_print_hex_dump((oal_uint8 *)oal_netbuf_data(pst_hcc_netbuf->pst_netbuf), OAL_NETBUF_LEN(pst_hcc_netbuf->pst_netbuf), 32, "switch resp: ");
        }
        else
        {
#ifdef WIN32
            oal_memcopy((char*)&st_switch_info_resp, (char*)oal_netbuf_data(pst_hcc_netbuf->pst_netbuf)+OAL_SIZEOF(transfer_type), OAL_SIZEOF(st_switch_info_resp));
#else
            oal_memcopy((oal_void*)&st_switch_info_resp, (oal_void*)oal_netbuf_data(pst_hcc_netbuf->pst_netbuf)+OAL_SIZEOF(transfer_type), OAL_SIZEOF(st_switch_info_resp));
#endif
            if(HCC_BUS_SWITCH_POWERUP == st_switch_info_resp.action_type)
            {
                oal_print_hi11xx_log(HI11XX_LOG_INFO, "bus %d power up response, ret=%d", st_switch_info_resp.bus_id, st_switch_info_resp.is_succ);
                OAL_COMPLETE(&hcc->bus_dev->st_switch_powerup_ready);
            }
            else if(HCC_BUS_SWITCH_POWERDOWN == st_switch_info_resp.action_type)
            {
                oal_print_hi11xx_log(HI11XX_LOG_INFO, "bus %d power down response, ret=%d", st_switch_info_resp.bus_id, st_switch_info_resp.is_succ);
                OAL_COMPLETE(&hcc->bus_dev->st_switch_powerdown_ready);
            }
            else
            {
                oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_rx_response unkown action type:%d", st_switch_info_resp.action_type);
            }
        }
    }
    else
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "unkown bus rx resposne type %d", transfer_type);
        oal_print_hex_dump((oal_uint8 *)oal_netbuf_data(pst_hcc_netbuf->pst_netbuf), OAL_NETBUF_LEN(pst_hcc_netbuf->pst_netbuf), 32, "switch resp: ");
    }

    return OAL_SUCC;
}

OAL_STATIC oal_int32 hcc_bus_notify_devip_powerctrl(hcc_bus* pst_bus, oal_uint32 is_powerup, oal_uint32 is_need_resp)
{
    oal_int32 cmd_len;
    hcc_switch_info st_switch_info;
    oal_uint16 transfer_type;
    oal_netbuf_stru*       pst_netbuf;
    struct hcc_transfer_param st_hcc_transfer_param = {0};
    struct hcc_handler* hcc;

    if(NULL == pst_bus->bus_dev)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_notify_devip_powerctrl bus_dev is null");
        return -OAL_ENODEV;
    }

    hcc = pst_bus->bus_dev->hcc;
    if(NULL == hcc)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_notify_devip_powerctrl hcc is null");
        return -OAL_ENODEV;
    }

    cmd_len = OAL_SIZEOF(oal_uint16) + OAL_SIZEOF(hcc_switch_info);

    pst_netbuf  = hcc_netbuf_alloc(cmd_len);
    if (NULL == pst_netbuf)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_notify_devip_powerctrl alloc skb fail, cmd_len=%d", cmd_len);
        return -OAL_ENOMEM;
    }

    transfer_type = (oal_uint16)HCC_BUS_TRANSFER_SWITCH_ACTION;
    st_switch_info.action_type = is_powerup ? HCC_BUS_SWITCH_POWERUP : HCC_BUS_SWITCH_POWERDOWN;
    st_switch_info.bus_id = pst_bus->bus_type;
    st_switch_info.is_need_resp = !!is_need_resp;

    oal_netbuf_put(pst_netbuf,cmd_len);

    oal_memcopy((oal_void*)oal_netbuf_data(pst_netbuf), (oal_void*)&transfer_type, OAL_SIZEOF(oal_uint16));

#ifdef WIN32
    oal_memcopy((char*)oal_netbuf_data(pst_netbuf) + OAL_SIZEOF(oal_uint16), (char*)&st_switch_info, OAL_SIZEOF(st_switch_info));
#else
    oal_memcopy((oal_void*)oal_netbuf_data(pst_netbuf) + OAL_SIZEOF(oal_uint16), (oal_void*)&st_switch_info, OAL_SIZEOF(st_switch_info));
#endif

    /*高优先级发送*/
    hcc_hdr_param_init(&st_hcc_transfer_param,
                    HCC_ACTION_TYPE_BUS,
                    0,
                    0,
                    HCC_FC_NONE,
                    DATA_HI_QUEUE);//DATA_HI_QUEUE

    return hcc_tx_etc(hcc, pst_netbuf, &st_hcc_transfer_param);

}

/*异常下电，不需要等response*/
oal_int32 hcc_bus_switch_ip_power_down_excep(void* data)
{
    /*target ip power down process*/
    //oal_int32 ret;
    hcc_bus_dev* pst_bus_dev = NULL;
    /*time cost*/
    hcc_bus* target_bus = (hcc_bus*)(data);

    if(OAL_WARN_ON(NULL == target_bus))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "target bus is null");
        return -OAL_EFAIL;
    }

    pst_bus_dev = target_bus->bus_dev;
    if(OAL_WARN_ON(NULL == pst_bus_dev))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "target bus %d 's bus dev is null", target_bus->bus_id);
        return -OAL_ENODEV;
    }

    if(g_switch_ip_pwrdown_bypass)
    {
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "bus :%s exception power down bypass", hcc_bus_get_bus_type_str(target_bus->bus_type));
        return OAL_SUCC;
    }

    /*Power Down old bus*/
    OAL_INIT_COMPLETION(&pst_bus_dev->st_switch_powerdown_ready);

    if(OAL_SUCC != hcc_bus_notify_devip_powerctrl(target_bus,  0, 0))
    {
        /*通知老的IP下电*/
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc_bus_switch_thread send power down bus %s message failed", hcc_bus_get_bus_type_str(target_bus->bus_type));
    }

    return OAL_SUCC;
}

/*正常下电，等待response*/
oal_int32 hcc_bus_switch_ip_power_down(void* data)
{
    /*target ip power down process*/
    oal_int32 ret;
    hcc_bus_dev* pst_bus_dev = NULL;
    hcc_bus* target_bus;
    /*time cost*/
    declare_time_cost_stru(cost);
    target_bus = (hcc_bus*)(data);

    if(OAL_WARN_ON(NULL == target_bus))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, ":%s error:target bus  is null", __FUNCTION__);
        return -OAL_EINVAL;
    };

    pst_bus_dev = target_bus->bus_dev;
    if(OAL_WARN_ON(NULL == pst_bus_dev))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "target bus %d 's bus dev is null", target_bus->bus_id);
        return -OAL_ENODEV;
    }

    oal_get_time_cost_start(cost);

    /*Power Down old bus*/
    OAL_INIT_COMPLETION(&pst_bus_dev->st_switch_powerdown_ready);

    if(OAL_SUCC != hcc_bus_notify_devip_powerctrl(target_bus,  0, 1))
    {
        /*通知老的IP下电*/
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc_bus_switch_thread send power down bus %d message failed", target_bus->bus_id);
    }

    ret = oal_wait_for_completion_interruptible_timeout(&pst_bus_dev->st_switch_powerdown_ready, (oal_uint32)OAL_MSECS_TO_JIFFIES(10000));
    if(0 == ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wait power down bus %s response timeout", hcc_bus_get_bus_type_str(target_bus->bus_type));
        DECLARE_DFT_TRACE_KEY_INFO("power down bus response timeout failed", OAL_DFT_TRACE_FAIL);
    }
    else if( ret < 0 )
    {
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "wait power down bus %s response interrupt", hcc_bus_get_bus_type_str(target_bus->bus_type));
    }

    oal_get_time_cost_end(cost);
    oal_calc_time_cost_sub(cost);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hardware powerdown %s cost time %llu us", hcc_bus_get_bus_type_str(target_bus->bus_type), time_cost_var_sub(cost));

    return OAL_SUCC;
}

oal_int32 hcc_bus_switch_ip_power_on(void* data)
{
    oal_int32 ret;
    hcc_bus_dev* pst_bus_dev = NULL;
    hcc_bus* target_bus;
    /*time cost*/
    declare_time_cost_stru(cost);

    target_bus = (hcc_bus*)(data);

    if(OAL_WARN_ON(NULL == target_bus))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, ":%s error:target bus  is null", __FUNCTION__);
        return -OAL_EINVAL;
    };

    oal_get_time_cost_start(cost);

    pst_bus_dev = target_bus->bus_dev;
    if(OAL_WARN_ON(NULL == pst_bus_dev))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "target bus %s 's bus dev is null", hcc_bus_get_bus_type_str(target_bus->bus_type));
        DECLARE_DFT_TRACE_KEY_INFO("hcc_bus_switch_thread response timeout failed", OAL_DFT_TRACE_FAIL);
        g_switch_pwr_ret = -OAL_ENODEV;
        return -OAL_ENODEV;
    }

    /*ip上电*/
    /*通知Device 给目标IP上电*/
    if(OAL_SUCC != hcc_bus_notify_devip_powerctrl(target_bus, 1, 1))
    {
        /*通知上电失败，忽略这次切换操作*/
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_switch_thread power up bus %s failed", hcc_bus_get_bus_type_str(target_bus->bus_type));
        DECLARE_DFT_TRACE_KEY_INFO("hcc_bus_switch_thread response timeout failed", OAL_DFT_TRACE_FAIL);
        g_switch_pwr_ret = -OAL_EBUSY;
        return -OAL_EBUSY;
    }

    /*等待上电完成*/
    ret = oal_wait_for_completion_interruptible_timeout(&pst_bus_dev->st_switch_powerup_ready, (oal_uint32)OAL_MSECS_TO_JIFFIES(10000));
    if(0 == ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wait power up bus %s response timeout", hcc_bus_get_bus_type_str(target_bus->bus_type));
        DECLARE_DFT_TRACE_KEY_INFO("hcc_bus_switch_thread response timeout failed", OAL_DFT_TRACE_FAIL);
        /*timeout, we send power down command*/
        hcc_bus_notify_devip_powerctrl(target_bus, 0, 0);
        g_switch_pwr_ret = -OAL_ETIMEDOUT;
        return -OAL_ETIMEDOUT;
    }
    else if( ret < 0)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wait power up bus %s response interrupt, ret=%d ", hcc_bus_get_bus_type_str(target_bus->bus_type), ret);
        g_switch_pwr_ret = -OAL_ETIMEDOUT;
        return -OAL_EIO;
    }

    oal_get_time_cost_end(cost);
    oal_calc_time_cost_sub(cost);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hardware powerup %s cost time %llu us", hcc_bus_get_bus_type_str(target_bus->bus_type), time_cost_var_sub(cost));

    g_switch_pwr_ret = OAL_SUCC;
    return OAL_SUCC;
}

OAL_STATIC oal_int32 hcc_bus_switch_thread_condtion(hcc_bus_dev* pst_bus_dev)
{
    //oal_print_hi11xx_log(HI11XX_LOG_INFO, "[%s]:%d, %p", __FUNCTION__, __LINE__, pst_bus_dev->switch_bus_req);
    return (NULL != pst_bus_dev->switch_bus_req);
}

OAL_STATIC oal_void hcc_bus_switch_process_start(hcc_bus_dev* pst_bus_dev)
{
    pst_bus_dev->switch_state = HCC_BUS_SWITCH_STATE_START;
}

OAL_STATIC oal_void hcc_bus_switch_process_end(hcc_bus_dev* pst_bus_dev)
{
    pst_bus_dev->switch_state = HCC_BUS_SWITCH_STATE_END;
    /*all done, switch success*/
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    wake_up_interruptible(&pst_bus_dev->st_swtich_ack_wq);
#endif
}

OAL_STATIC oal_void hcc_bus_switch_process_abort(hcc_bus_dev* pst_bus_dev)
{
    pst_bus_dev->switch_state = HCC_BUS_SWITCH_STATE_ABORT;
    /*all done, switch success*/
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    wake_up_interruptible(&pst_bus_dev->st_swtich_ack_wq);
#endif
}
#if 0
/*Test Code, Power Up PCie*/
oal_int32 hcc_bus_test_powercrcle(oal_int32 test_cnt)
{
    oal_int32 ret;
    oal_int32 i;
    oal_int32 target = HCC_BUS_SDIO;
    hcc_bus* target_bus;
    hcc_bus_dev* pst_bus_dev  = hcc_get_bus_dev(HCC_CHIP_110X_DEV);

    if(OAL_WARN_ON(NULL == pst_bus_dev))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "dev id:%u 's bus dev is null", HCC_CHIP_110X_DEV);
        return -OAL_EINVAL;
    }

    if(OAL_WARN_ON(target >= HCC_BUS_BUTT))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "invalid bus type:%d", target);
        return -OAL_EINVAL;
    }

    target_bus = hcc_find_bus_by_devid(HCC_CHIP_110X_DEV, target);
    if(OAL_WARN_ON(NULL == target_bus))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "find bus failed by devid:%u , bus type:%u", HCC_CHIP_110X_DEV, target);
        return -OAL_ENODEV;
    }

    OAL_INIT_COMPLETION(&pst_bus_dev->st_switch_powerup_ready);

    oal_wake_lock(&pst_bus_dev->st_switch_wakelock);

    hcc_bus_power_action(target_bus, HCC_BUS_SW_POWER_PATCH_LOAD_PREPARE);

    /*Host发起目标IP硬件初始化*/
    hcc_bus_power_ctrl_register(target_bus, HCC_BUS_CTRL_POWER_UP, hcc_bus_switch_ip_power_on, (void*)target_bus);
    ret = hcc_bus_power_action(target_bus, HCC_BUS_SW_POWER_UP);
    if(OAL_SUCC != ret || OAL_SUCC != g_switch_pwr_ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_switch_thread init bus %s falied=%d, g_ret=%d", hcc_bus_get_bus_type_str(target_bus->bus_type), ret, g_switch_pwr_ret);
        oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
        hcc_bus_switch_process_abort(pst_bus_dev);
        return -OAL_EFAIL;
    }
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "[%s]:%d", __FUNCTION__, __LINE__);
    /*资源初始化*/
    ret = hcc_bus_reinit(target_bus);
    if(OAL_SUCC != ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_switch_thread reinit bus %s falied=%d", hcc_bus_get_bus_type_str(target_bus->bus_type), ret);
        DECLARE_DFT_TRACE_KEY_INFO("hcc_bus_switch_thread reinit bus failed", OAL_DFT_TRACE_FAIL);
        hcc_bus_power_ctrl_register(target_bus, HCC_BUS_CTRL_POWER_DOWN, hcc_bus_switch_ip_power_down_excep, (void*)target_bus);
        hcc_bus_power_action(target_bus, HCC_BUS_SW_POWER_DOWN);
        oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
        hcc_bus_switch_process_abort(pst_bus_dev);
        return -OAL_EFAIL;
    }
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "[%s]:%d", __FUNCTION__, __LINE__);
    /*Host发起目标IP软件初始化*/
    /*ip初始化 不需要下载PATCH 此时WCPU在正常运行*/
    ret = hcc_bus_power_action(target_bus, HCC_BUS_SW_POWER_PATCH_LAUCH);
    if(OAL_SUCC != ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_switch_thread lauch bus %s falied=%d", hcc_bus_get_bus_type_str(target_bus->bus_type), ret);
        DECLARE_DFT_TRACE_KEY_INFO("hcc_bus_switch_thread lauch bus failed", OAL_DFT_TRACE_FAIL);
        hcc_bus_power_ctrl_register(target_bus, HCC_BUS_CTRL_POWER_DOWN, hcc_bus_switch_ip_power_down_excep, (void*)target_bus);
        hcc_bus_power_action(target_bus, HCC_BUS_SW_POWER_DOWN);
        oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
        hcc_bus_switch_process_abort(pst_bus_dev);
        return -OAL_EFAIL;
    }
#if 0
    /*sdio try*/
    for(i = 0; i < test_cnt; i++)
    {
        oal_sdio_claim_host(oal_get_sdio_default_handler());
        ret = oal_sdio_extend_buf_get(oal_get_sdio_default_handler());
        oal_sdio_release_host(oal_get_sdio_default_handler());
        if(ret)
        {
            DECLARE_DFT_TRACE_KEY_INFO("sdio_extend_buf_get failed", OAL_DFT_TRACE_FAIL);
            oal_msleep(5000);
            oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
            return -OAL_EFAIL;
        }
    }
#endif

    ret = hcc_bus_power_ctrl_register(target_bus,
                                        HCC_BUS_CTRL_POWER_DOWN, hcc_bus_switch_ip_power_down, (oal_void*)target_bus);
    hcc_bus_power_action(target_bus, HCC_BUS_SW_POWER_DOWN);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "[%s]:%d", __FUNCTION__, __LINE__);

    oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);

    return OAL_SUCC;
}

/*Test Code, Power Up PCie*/
oal_int32 hcc_bus_test_powerup(oal_int32 target)
{
    oal_int32 ret;
    hcc_bus* target_bus;
    hcc_bus_dev* pst_bus_dev  = hcc_get_bus_dev(HCC_CHIP_110X_DEV);

    if(OAL_WARN_ON(NULL == pst_bus_dev))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "dev id:%u 's bus dev is null", HCC_CHIP_110X_DEV);
        return -OAL_EINVAL;
    }

    if(OAL_WARN_ON(target >= HCC_BUS_BUTT))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "invalid bus type:%d", target);
        return -OAL_EINVAL;
    }

    target_bus = hcc_find_bus_by_devid(HCC_CHIP_110X_DEV, target);
    if(OAL_WARN_ON(NULL == target_bus))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "find bus failed by devid:%u , bus type:%u", HCC_CHIP_110X_DEV, target);
        return -OAL_ENODEV;
    }

    OAL_INIT_COMPLETION(&pst_bus_dev->st_switch_powerup_ready);

    oal_wake_lock(&pst_bus_dev->st_switch_wakelock);

    hcc_bus_power_action(target_bus, HCC_BUS_SW_POWER_PATCH_LOAD_PREPARE);

    /*Host发起目标IP硬件初始化*/
    hcc_bus_power_ctrl_register(target_bus, HCC_BUS_CTRL_POWER_UP, hcc_bus_switch_ip_power_on, (void*)target_bus);
    ret = hcc_bus_power_action(target_bus, HCC_BUS_SW_POWER_UP);
    if(OAL_SUCC != ret || OAL_SUCC != g_switch_pwr_ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_switch_thread init bus %s falied=%d, g_ret=%d", hcc_bus_get_bus_type_str(target_bus->bus_type), ret, g_switch_pwr_ret);
        oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
        hcc_bus_switch_process_abort(pst_bus_dev);
        return -OAL_EFAIL;
    }
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "[%s]:%d", __FUNCTION__, __LINE__);
    /*资源初始化*/
    ret = hcc_bus_reinit(target_bus);
    if(OAL_SUCC != ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_switch_thread reinit bus %s falied=%d", hcc_bus_get_bus_type_str(target_bus->bus_type), ret);
        DECLARE_DFT_TRACE_KEY_INFO("hcc_bus_switch_thread reinit bus failed", OAL_DFT_TRACE_FAIL);
        hcc_bus_power_ctrl_register(target_bus, HCC_BUS_CTRL_POWER_DOWN, hcc_bus_switch_ip_power_down_excep, (void*)target_bus);
        hcc_bus_power_action(target_bus, HCC_BUS_SW_POWER_DOWN);
        oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
        hcc_bus_switch_process_abort(pst_bus_dev);
        return -OAL_EFAIL;
    }
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "[%s]:%d", __FUNCTION__, __LINE__);
    /*Host发起目标IP软件初始化*/
    /*ip初始化 不需要下载PATCH 此时WCPU在正常运行*/
    ret = hcc_bus_power_action(target_bus, HCC_BUS_SW_POWER_PATCH_LAUCH);
    if(OAL_SUCC != ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_switch_thread lauch bus %s falied=%d", hcc_bus_get_bus_type_str(target_bus->bus_type), ret);
        DECLARE_DFT_TRACE_KEY_INFO("hcc_bus_switch_thread lauch bus failed", OAL_DFT_TRACE_FAIL);
        hcc_bus_power_ctrl_register(target_bus, HCC_BUS_CTRL_POWER_DOWN, hcc_bus_switch_ip_power_down_excep, (void*)target_bus);
        hcc_bus_power_action(target_bus, HCC_BUS_SW_POWER_DOWN);
        oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
        hcc_bus_switch_process_abort(pst_bus_dev);
        return -OAL_EFAIL;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "[%s]:%d", __FUNCTION__, __LINE__);

    oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);

    return OAL_SUCC;
}
#endif



OAL_STATIC oal_int32 hcc_bus_switch_thread(oal_void *data)
{
    oal_int32 ret;
    oal_uint  flags;
    hcc_bus* target_bus;
    hcc_bus* old_bus;
    struct sched_param       param;
    hcc_bus_dev* pst_bus_dev;

    /*time cost*/
    declare_time_cost_stru(cost);

    pst_bus_dev = (hcc_bus_dev *)data;
    if(OAL_UNLIKELY(NULL == pst_bus_dev))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_switch_thread pst_bus_dev is null, exit");
        return OAL_SUCC;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc_bus_switch_thread[%d] start", pst_bus_dev->dev_id);

    /*Must response immediately*/
    param.sched_priority = 97;
    oal_set_thread_property_etc(current,
                            SCHED_RR,
                            &param,
                            -20);

    allow_signal(SIGTERM);

    for(;;)
    {

        if (OAL_UNLIKELY(oal_kthread_should_stop()))
        {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "exit hcc_bus_switch_thread[%d]", pst_bus_dev->dev_id);
            break;
        }

        ret = OAL_WAIT_EVENT_INTERRUPTIBLE(pst_bus_dev->st_switch_request_wq, hcc_bus_switch_thread_condtion(pst_bus_dev));
        if(OAL_UNLIKELY(-ERESTARTSYS == ret))
        {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc switch task %s was interrupted by a signal", oal_get_current_task_name());
            break;
        }

        oal_print_hi11xx_log(HI11XX_LOG_INFO, "[%s]:%d", __FUNCTION__, __LINE__);

        oal_wake_lock(&pst_bus_dev->st_switch_wakelock);

        oal_spin_lock_irq_save(&pst_bus_dev->st_switch_lock, &flags);
        target_bus = pst_bus_dev->switch_bus_req;
        pst_bus_dev->switch_bus_req = NULL;
        if(NULL == target_bus)
        {
            oal_print_hi11xx_log(HI11XX_LOG_DBG, "target_bus is null");
            oal_spin_unlock_irq_restore(&pst_bus_dev->st_switch_lock, &flags);
            oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
            continue;
        }
        oal_spin_unlock_irq_restore(&pst_bus_dev->st_switch_lock, &flags);

        if(target_bus == pst_bus_dev->cur_bus)
        {
            oal_print_hi11xx_log(HI11XX_LOG_DBG, "req bus %s is working now!", hcc_bus_get_bus_type_str(pst_bus_dev->cur_bus->bus_type));
            hcc_bus_switch_process_end(pst_bus_dev);
            oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
            continue;
        }

        old_bus = pst_bus_dev->cur_bus;

        /*判断WLAN上下电状态*/

        /*process target_bus*/

        /*close dfr*/

        /*cancel wlan pm*/

        /*block the wlan open/close process*/

        hcc_bus_switch_process_start(pst_bus_dev);

        if(HCC_BUS_SDIO == target_bus->bus_type)
        {
            DECLARE_DFT_TRACE_KEY_INFO("dync_switch_request_sdio", OAL_DFT_TRACE_OTHER);
        }
        else if(HCC_BUS_PCIE == target_bus->bus_type)
        {
            DECLARE_DFT_TRACE_KEY_INFO("dync_switch_request_pcie", OAL_DFT_TRACE_OTHER);
        }
        else
        {
            DECLARE_DFT_TRACE_KEY_INFO("dync_switch_request_xxxx", OAL_DFT_TRACE_OTHER);
        }

        OAL_INIT_COMPLETION(&pst_bus_dev->st_switch_powerup_ready);

        wlan_pm_statesave();

        hcc_bus_power_action(target_bus, HCC_BUS_SW_POWER_PATCH_LOAD_PREPARE);
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "switch to %s: g_switch_total_count:%d",
                             hcc_bus_get_bus_type_str(target_bus->bus_type), ++g_switch_total_count);
        oal_get_time_cost_start(cost);

        g_switch_pwr_ret = OAL_SUCC;

        /*Host发起目标IP硬件初始化*/
        hcc_bus_power_ctrl_register(target_bus, HCC_BUS_CTRL_POWER_UP, hcc_bus_switch_ip_power_on, (void*)target_bus);
        ret = hcc_bus_power_action(target_bus, HCC_BUS_SW_POWER_UP);
        if(OAL_SUCC != ret || OAL_SUCC != g_switch_pwr_ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_switch_thread init bus %s falied=%d, g_ret=%d", hcc_bus_get_bus_type_str(target_bus->bus_type), ret, g_switch_pwr_ret);
            DECLARE_DFT_TRACE_KEY_INFO("hcc_bus_switch_thread init bus failed", OAL_DFT_TRACE_FAIL);
            oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
            wlan_pm_staterestore();
            hcc_bus_switch_process_abort(pst_bus_dev);
            continue;
        }
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "[%s]:%d", __FUNCTION__, __LINE__);
        /*资源初始化*/
        ret = hcc_bus_reinit(target_bus);
        if(OAL_SUCC != ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_switch_thread reinit bus %s falied=%d", hcc_bus_get_bus_type_str(target_bus->bus_type), ret);
            DECLARE_DFT_TRACE_KEY_INFO("hcc_bus_switch_thread reinit bus failed", OAL_DFT_TRACE_FAIL);
            hcc_bus_power_ctrl_register(target_bus, HCC_BUS_CTRL_POWER_DOWN, hcc_bus_switch_ip_power_down_excep, (void*)target_bus);
            hcc_bus_power_action(target_bus, HCC_BUS_SW_POWER_DOWN);
            oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
            wlan_pm_staterestore();
            hcc_bus_switch_process_abort(pst_bus_dev);
            continue;
        }
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "[%s]:%d", __FUNCTION__, __LINE__);
        /*Host发起目标IP软件初始化*/
        /*ip初始化 不需要下载PATCH 此时WCPU在正常运行*/
        ret = hcc_bus_power_action(target_bus, HCC_BUS_SW_POWER_PATCH_LAUCH);
        if(OAL_SUCC != ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_switch_thread lauch bus %s falied=%d", hcc_bus_get_bus_type_str(target_bus->bus_type), ret);
            DECLARE_DFT_TRACE_KEY_INFO("hcc_bus_switch_thread lauch bus failed", OAL_DFT_TRACE_FAIL);
            hcc_bus_power_ctrl_register(target_bus, HCC_BUS_CTRL_POWER_DOWN, hcc_bus_switch_ip_power_down_excep, (void*)target_bus);
            hcc_bus_power_action(target_bus, HCC_BUS_SW_POWER_DOWN);
            oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
            wlan_pm_staterestore();
            hcc_bus_switch_process_abort(pst_bus_dev);
            continue;
        }

        oal_get_time_cost_end(cost);
        oal_calc_time_cost_sub(cost);
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "bus res init %s cost time %llu us", hcc_bus_get_bus_type_str(target_bus->bus_type), time_cost_var_sub(cost));
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "[%s]:%d", __FUNCTION__, __LINE__);

        if(hcc_bus_auto_sched_is_support(pst_bus_dev->dev_id))
        {
            /*stop auto pps timer*/
            hcc_bus_stop_auto_pps_task(pst_bus_dev);
        }

        /*硬件初始化完成，切换软件*/
        /*暂停hcc 线程,或者hcc_tx锁*/
        oal_get_time_cost_start(cost);
        hcc_tx_transfer_lock(pst_bus_dev->hcc);

        /*挂起发送通道,等待通道清空, 清空后WCPU会切换bus*/
        ret = hcc_bus_switch_suspend_tx(old_bus);

        ret = hcc_bus_switch_clean_res(old_bus);
        if(ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_switch_thread clean_res bus %s failed", hcc_bus_get_bus_type_str(old_bus->bus_type));
            DECLARE_DFT_TRACE_KEY_INFO("hcc_bus_switch_thread clean_res bus failed", OAL_DFT_TRACE_FAIL);

            hcc_tx_transfer_unlock(pst_bus_dev->hcc);
            if(hcc_bus_auto_sched_is_support(pst_bus_dev->dev_id))
            {
                /*stop auto pps timer*/
                hcc_bus_start_auto_pps_task(pst_bus_dev);
            }
            oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
            wlan_pm_staterestore();
            hcc_bus_switch_process_abort(pst_bus_dev);
            continue;
        }

        /*重入问题*/
        hcc_switch_bus(pst_bus_dev->dev_id, target_bus->bus_type);

        if(old_bus == pst_bus_dev->cur_bus)
        {
            /*switch failed?*/
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_switch_thread switch bus %s failed", hcc_bus_get_bus_type_str(old_bus->bus_type));
            DECLARE_DFT_TRACE_KEY_INFO("hcc_bus_switch_thread switch bus failed", OAL_DFT_TRACE_FAIL);
        }
        else
        {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc_bus_switch_thread switch bus from %s to %s",
                                hcc_bus_get_bus_type_str(old_bus->bus_type), hcc_bus_get_bus_type_str(target_bus->bus_type));
        }

        /*发消息通知DEV锁住DMA调度关闭消息接口，等待IP空闲*/

        /*切换完成恢复发送通道, 上面Host bus 切换有时间差，
          所有完成后需要重新调度受影响的任务*/
        hcc_bus_sched_gpio_task(pst_bus_dev->cur_bus, 0);

        ret = hcc_bus_switch_resume_tx(pst_bus_dev->cur_bus);

        hcc_tx_transfer_unlock(pst_bus_dev->hcc);
        oal_get_time_cost_end(cost);
        oal_calc_time_cost_sub(cost);
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "lock bus %s tx chan cost time %llu us", hcc_bus_get_bus_type_str(target_bus->bus_type), time_cost_var_sub(cost));

        if(old_bus != pst_bus_dev->cur_bus)
        {

            /*Power Down old bus*/
            OAL_INIT_COMPLETION(&pst_bus_dev->st_switch_powerdown_ready);

            hcc_bus_rx_transfer_lock(old_bus);

            ret = hcc_bus_power_ctrl_register(old_bus,
                                                HCC_BUS_CTRL_POWER_DOWN, hcc_bus_switch_ip_power_down, (oal_void*)old_bus);
            hcc_bus_power_action(old_bus, HCC_BUS_SW_POWER_DOWN);
            /*此时不管低功耗是否打开，old_bus已经持有一把锁，需要释放,并且锁住new_bus*/
            hcc_bus_wake_lock(pst_bus_dev->cur_bus);
            hcc_bus_wake_unlock(old_bus);
            hcc_bus_rx_transfer_unlock(old_bus);
            /*clean old bus's res*/
        }

        if(HCC_BUS_SDIO == pst_bus_dev->cur_bus->bus_type)
        {
            DECLARE_DFT_TRACE_KEY_INFO("dync_switch_sdio_done", OAL_DFT_TRACE_OTHER);
        }
        else if(HCC_BUS_PCIE == pst_bus_dev->cur_bus->bus_type)
        {
            DECLARE_DFT_TRACE_KEY_INFO("dync_switch_pcie_done", OAL_DFT_TRACE_OTHER);
        }
        else
        {
            DECLARE_DFT_TRACE_KEY_INFO("dync_switch_xxxx_done", OAL_DFT_TRACE_OTHER);
        }

        /*pm had disabled*/
        if(hcc_bus_auto_bindcpu_is_support(pst_bus_dev->dev_id))
        {
            hcc_bus_bindcpu(old_bus, HCC_DIR_COUNT, 0);
        }

        if(hcc_bus_auto_sched_is_support(pst_bus_dev->dev_id))
        {
            /*stop auto pps timer*/
            hcc_bus_start_auto_pps_task(pst_bus_dev);
        }

        wlan_pm_staterestore();

        hcc_bus_switch_process_end(pst_bus_dev);
        oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);

    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc_bus_switch_thread[%d] end", pst_bus_dev->dev_id);
    return OAL_SUCC;
}

oal_int32 hcc_bus_auto_switch_is_support(oal_uint32 dev_id)
{
    hcc_bus_dev* pst_bus_dev  = hcc_get_bus_dev(dev_id);
    if(OAL_UNLIKELY(NULL == pst_bus_dev))
    {
        return 0;
    }

    if(hcc_bus_auto_switch)
        return 1;

    return pst_bus_dev->bus_auto_switch;
}

oal_int32 hcc_bus_auto_bindcpu_is_support(oal_uint32 dev_id)
{
    hcc_bus_dev* pst_bus_dev  = hcc_get_bus_dev(dev_id);
    if(OAL_UNLIKELY(NULL == pst_bus_dev))
    {
        return 0;
    }

    if(hcc_bus_auto_bindcpu)
        return 1;

    return pst_bus_dev->bus_auto_bindcpu;
}

oal_int32 hcc_bus_auto_sched_is_support(oal_uint32 dev_id)
{
    if(!hcc_bus_auto_switch_is_support(dev_id) && !hcc_bus_auto_bindcpu_is_support(dev_id))
    {
        return 0;
    }

    return 1;
}

oal_int32 hi110x_hcc_dev_bindcpu(oal_int32 is_bind)
{
    hcc_bus* pst_bus;
    oal_uint32 dev_id = HCC_CHIP_110X_DEV;
    hcc_bus_dev* pst_bus_dev  = hcc_get_bus_dev(dev_id);

    if(OAL_UNLIKELY(NULL == pst_bus_dev))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "non-support switch dev id:%u", dev_id);
        return -OAL_EINVAL;
    }

    if(hcc_bus_auto_bindcpu_is_support(dev_id))
    {
        /*自动绑核打开直接返回*/
        return OAL_SUCC;
    }

    if(0 == hcc_bus_manual_bindcpu)
    {
        /*forbid manual control*/
        return OAL_SUCC;
    }

    pst_bus = pst_bus_dev->cur_bus;

    return hcc_bus_bindcpu(pst_bus, HCC_DIR_COUNT, is_bind);
}
EXPORT_SYMBOL_GPL(hi110x_hcc_dev_bindcpu);

oal_int32 hcc_dev_bindcpu(oal_uint32 dev_id, oal_int32 is_bind)
{
    hcc_bus* pst_bus;
    hcc_bus_dev* pst_bus_dev  = hcc_get_bus_dev(dev_id);

    if(OAL_UNLIKELY(NULL == pst_bus_dev))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "non-support switch dev id:%u", dev_id);
        return -OAL_EINVAL;
    }

    pst_bus = pst_bus_dev->cur_bus;
    if(hcc_bus_auto_bindcpu_is_support(dev_id))
    {
        return hcc_bus_bindcpu(pst_bus, HCC_DIR_COUNT, is_bind);
    }
    else
    {
        return -OAL_EIO;
    }

}
EXPORT_SYMBOL_GPL(hcc_dev_bindcpu);

/*bus性能调度*/
oal_int32 hcc_bus_performance_core_schedule(oal_uint32 dev_id)
{
    oal_uint32 run_time;
    oal_ulong current_time, pkt_pps_rx, pkt_pps_tx, pkt_pps_total;
    oal_ulong tx_cnt, rx_cnt, total_cnt;
    hcc_bus_dev* pst_bus_dev  = hcc_get_bus_dev(dev_id);

    if(OAL_UNLIKELY(NULL == pst_bus_dev))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "non-support switch dev id:%u", dev_id);
        return -OAL_EINVAL;
    }

    if(!hcc_bus_auto_sched_is_support(dev_id))
    {
        oal_print_hi11xx_log(HI11XX_LOG_VERBOSE, "hcc bus auto sched disabled");
        return OAL_SUCC;
    }

    current_time = jiffies;

    if(oal_time_after(current_time, (pst_bus_dev->bus_pps_start_time + 1*HZ)))
    {
        /*time is up, calc pps*/
        tx_cnt = pst_bus_dev->hcc->hcc_transer_info.tx_assem_info.pkt_cnt;
        rx_cnt = pst_bus_dev->hcc->hcc_transer_info.rx_assem_info.pkt_cnt;
        total_cnt = tx_cnt + rx_cnt;

        run_time = OAL_JIFFIES_TO_MSECS(current_time)
                   - OAL_JIFFIES_TO_MSECS(pst_bus_dev->bus_pps_start_time);

        if(0 == run_time)
        {
            run_time = 1;/*1 msec at least*/
        }

        pkt_pps_tx = (tx_cnt*1000)/(oal_ulong)run_time;
        pkt_pps_rx = (rx_cnt*1000)/(oal_ulong)run_time;

        pkt_pps_total = pkt_pps_tx + pkt_pps_rx;

        hcc_bus_current_pps = pkt_pps_total;

        if(hcc_bus_auto_switch_is_support(dev_id))
        {
            if(pkt_pps_total >= hcc_bus_auto_switch_limit)
            {
                /*switch to PCIe*/
                hcc_switch_bus_request(dev_id, HCC_BUS_PCIE, 0);
            }
            else if(pkt_pps_total <= hcc_bus_auto_switch_limit/2)
            {
                if(HDEV_TO_HBUS(pst_bus_dev)->bus_type != HCC_BUS_SDIO)
                {
                    /*低吞吐不需要重复切换*/
                    hcc_switch_bus_request(dev_id, HCC_BUS_SDIO, 0);
                }
            }
        }

        if(hcc_bus_auto_bindcpu_is_support(dev_id))
        {
            hcc_bus* pst_bus = pst_bus_dev->cur_bus;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
            if(pkt_pps_total >= hcc_bus_auto_bindcpu_limit)
            {
                hcc_bus_bindcpu(pst_bus, HCC_DIR_COUNT, 1);
                //hcc_bus_bindcpu(pst_bus, HCC_RX, 1);
            }
            else if(pkt_pps_total <= hcc_bus_auto_bindcpu_limit/2)
            {
                hcc_bus_bindcpu(pst_bus, HCC_DIR_COUNT, 0);
                //hcc_bus_bindcpu(pst_bus, HCC_RX, 0);
            }
#endif
        }

        oal_print_hi11xx_log(HI11XX_LOG_DBG, "pkt_pps_tx: %lu , pkt_pps_rx: %lu, runtime:%u", pkt_pps_tx, pkt_pps_rx, run_time);

        pst_bus_dev->hcc->hcc_transer_info.tx_assem_info.pkt_cnt = 0;
        pst_bus_dev->hcc->hcc_transer_info.rx_assem_info.pkt_cnt = 0;
        pst_bus_dev->bus_pps_start_time = current_time;
    }

    return OAL_SUCC;
}

oal_int32 hcc_bus_performance_core_init(oal_uint32 dev_id)
{
    hcc_bus_dev* pst_bus_dev  = hcc_get_bus_dev(dev_id);

    if(OAL_UNLIKELY(NULL == pst_bus_dev))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "non-support switch dev id:%u", dev_id);
        return -OAL_EINVAL;
    }

    pst_bus_dev->bus_pps_start_time = jiffies;
    pst_bus_dev->hcc->hcc_transer_info.tx_assem_info.pkt_cnt = 0;
    pst_bus_dev->hcc->hcc_transer_info.rx_assem_info.pkt_cnt = 0;
    oal_timer_init(&pst_bus_dev->bus_pps_timer, HCC_BUS_PPS_COUNT_TIMEOUT, hcc_dev_pps_count_timeout, (oal_uint)pst_bus_dev);

    return OAL_SUCC;
}

oal_int32 hcc_bus_performance_core_exit(oal_uint32 dev_id)
{
    hcc_bus_dev* pst_bus_dev  = hcc_get_bus_dev(dev_id);

    if(OAL_UNLIKELY(NULL == pst_bus_dev))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "non-support switch dev id:%u", dev_id);
        return -OAL_EINVAL;
    }

    oal_timer_delete_sync(&pst_bus_dev->bus_pps_timer);
    return OAL_SUCC;
}

/*Export Symbol*/

/*
    switch hcc to target bus SDIO/PCIE/USB
    target: the bus type you want to use
    is_sync: 1 means function util switch done
*/
oal_int32 hcc_switch_bus_request(oal_uint32 dev_id, oal_uint32 target, oal_int32 is_sync)
{
    oal_int32 ret;
    oal_uint  flags;
    //oal_int32 ret = OAL_SUCC;
    hcc_bus* pst_bus = NULL;
    hcc_bus_dev* pst_bus_dev  = hcc_get_bus_dev(dev_id);

    if(OAL_WARN_ON(HCC_CHIP_110X_DEV != dev_id))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "non-support switch dev id:%u", dev_id);
        return -OAL_EINVAL;
    }

    if(OAL_WARN_ON(NULL == pst_bus_dev))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR,"dev id:%u 's bus dev is null", dev_id);
        return -OAL_EINVAL;
    }

    if(OAL_WARN_ON(target >= HCC_BUS_BUTT))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR,"invalid bus type:%d", target);
        return -OAL_EINVAL;
    }

    if(0 == pst_bus_dev->bus_switch_enable)
    {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "hcc switch disable");
        return OAL_SUCC;
    }

    if(NULL == pst_bus_dev->pst_switch_task)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "switch task is not exist");
        return -OAL_ENODEV;
    }

    pst_bus = hcc_find_bus_by_devid(dev_id, target);
    if(OAL_WARN_ON(NULL == pst_bus))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "find bus failed by devid:%u , bus type:%u", dev_id, target);
        return -OAL_ENODEV;
    }

    if(OAL_SUCC != hcc_bus_isvalid(pst_bus))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "bus %d is not ready", target);
        return -OAL_ENODEV;
    }

    if(NULL == pst_bus_dev->pst_switch_task)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "switch task is not ready");
        return -OAL_ENODEV;
    }

    oal_wake_lock(&pst_bus_dev->st_switch_wakelock);

    oal_spin_lock_irq_save(&pst_bus_dev->st_switch_lock, &flags);

    if(NULL != pst_bus_dev->switch_bus_req)
    {
        /*Last req is processing, overwrite*/
        if(OAL_TRUE == hcc_bus_is_match(pst_bus, pst_bus_dev->cur_bus))
        {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "bus %s is already working", hcc_bus_get_bus_type_str(pst_bus->bus_type));
            return OAL_SUCC;
        }
    }

    pst_bus_dev->switch_bus_req = pst_bus;

    oal_spin_unlock_irq_restore(&pst_bus_dev->st_switch_lock, &flags);

    hcc_bus_switch_process_start(pst_bus_dev);

    /*wakeup the switch thread*/
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&pst_bus_dev->st_switch_request_wq);

    if(is_sync)
    {
        /*等待切换完成*/
        ret = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pst_bus_dev->st_swtich_ack_wq,
                                           ((HCC_BUS_SWITCH_STATE_ABORT == pst_bus_dev->switch_state)
                                           || (HCC_BUS_SWITCH_STATE_END == pst_bus_dev->switch_state)),
                                           30*OAL_TIME_HZ);
        if(OAL_UNLIKELY(-ERESTARTSYS == ret))
        {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc switch task %s was interrupted by a signal, ret=%d, switch_state=%d",
                                                oal_get_current_task_name(), ret, pst_bus_dev->switch_state);
            oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
            return -OAL_EFAIL;
        }
        else if(0 == ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc switch task %s was timeout, ret=%d, switch_state=%d",
                                                oal_get_current_task_name(), ret, pst_bus_dev->switch_state);
            oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
            return -OAL_ETIMEDOUT;
        }

        if(HCC_BUS_SWITCH_STATE_END == pst_bus_dev->switch_state)
        {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "switch bus type %s sync done", hcc_bus_get_bus_type_str(pst_bus->bus_type));
            oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
            return OAL_SUCC;
        }
        else if(HCC_BUS_SWITCH_STATE_ABORT == pst_bus_dev->switch_state)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "switch bus type %s abort", hcc_bus_get_bus_type_str(pst_bus->bus_type));
            oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
            return -OAL_EFAIL;
        }

        oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
        return -OAL_EFAIL;
    }
    else
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "switch bus type %s async done", hcc_bus_get_bus_type_str(pst_bus->bus_type));
        oal_wake_unlock(&pst_bus_dev->st_switch_wakelock);
        return OAL_SUCC;
    }
}

/*high speed chan*/
oal_int32 hi110x_switch_to_hcc_highspeed_chan(oal_uint32 is_high)
{
    oal_uint32 target = is_high ? HCC_BUS_PCIE : HCC_BUS_SDIO;
    return hi110x_switch_hcc_bus_request(target);
}
/*lint -e19 */
oal_module_symbol(hi110x_switch_to_hcc_highspeed_chan);
/*lint +e19 */

oal_int32 hi110x_switch_hcc_bus_request(oal_uint32 target)
{
    if(hcc_bus_switch_bypass)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "hi110x_switch_bus_request %d bypass", target);
        return OAL_SUCC;
    }

    if(hcc_bus_auto_switch_is_support(HCC_CHIP_110X_DEV))
    {
        return OAL_SUCC;
    }

    return hcc_switch_bus_request(HCC_CHIP_110X_DEV, target, 0);
}
/*lint -e19 */
oal_module_symbol(hi110x_switch_hcc_bus_request);
/*lint +e19 */

oal_int32 hcc_switch_bus_loop_test(oal_uint32 dev_id, oal_uint32 loop_count)
{
    oal_uint32 i;
    oal_int32 ret;
    hcc_bus* pst_bus;
    hcc_bus_dev* pst_bus_dev;

    for(i = 0; i < loop_count*2; i++)
    {
        if(OAL_UNLIKELY(hcc_bus_switch_test_break))
        {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc_switch_bus_loop_test abort");
            return -OAL_EBUSY;
        }

        pst_bus_dev = hcc_get_bus_dev(dev_id);
        if(NULL == pst_bus_dev)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "dev id %d invalid", dev_id);
            return -OAL_ENODEV;
        }

        pst_bus = pst_bus_dev->cur_bus;
        if(NULL == pst_bus)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "dev id %d invalid, bus is null", dev_id);
            return -OAL_ENODEV;
        }

        ret = hcc_switch_bus_request(dev_id, ((HCC_BUS_SDIO == pst_bus->bus_type) ? HCC_BUS_PCIE:HCC_BUS_SDIO), 1);
        if(OAL_SUCC != ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "switch to %s failed ret=%d", hcc_bus_get_bus_type_str(pst_bus->bus_type), ret);
            return ret;
        }
        else
        {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "switch to %s ok, test times %u", hcc_bus_get_bus_type_str(pst_bus->bus_type), (i >> 1)+1);
        }

        if(hcc_bus_switch_test_delay_time)
            oal_msleep(hcc_bus_switch_test_delay_time);
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "test bus loop %u times done!", loop_count);

    return OAL_SUCC;
}

/*after all bus inited*/
oal_int32 hcc_probe_bus(hcc_bus* pst_bus)
{
    hcc_bus_dev *pst_bus_dev;

    if(OAL_WARN_ON(NULL != pst_bus->bus_dev))
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "%s had probed", pst_bus->name);
        return OAL_SUCC;
    }

    pst_bus_dev = hcc_get_bus_dev(pst_bus->dev_id);
    if(NULL == pst_bus_dev)
    {
        return -OAL_EINVAL;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "bus_type:%s, cap:%u, dev_name:%s",
                hcc_bus_get_bus_type_str(pst_bus->bus_type), pst_bus_dev->bus_cap, pst_bus_dev->name);

    if((1 << pst_bus->bus_type) & pst_bus_dev->bus_cap)
    {
        pst_bus->bus_dev = pst_bus_dev;
        pst_bus_dev->bus_num++;
        return OAL_SUCC;
    }

    return -OAL_ENODEV;
}


oal_int32 hcc_message_register_etc(struct hcc_handler* hcc, oal_uint8 msg,
                                hcc_msg_rx cb, oal_void* data)
{
    oal_int32 flag = 0;
    oal_int32 ret = OAL_SUCC;
    oal_uint             irqsave;
    oal_dlist_head_stru  *pst_entry;
    hcc_bus*             pst_bus_tmp;
    oal_spin_lock_irq_save(&g_hcc_bus_res_lock, &irqsave);
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &g_hcc_bus_res_hdr)
    {
        pst_bus_tmp = OAL_DLIST_GET_ENTRY(pst_entry, hcc_bus, list);
        if(pst_bus_tmp->bus_dev == hcc->bus_dev)
        {
            /*注册在当前设备上的所有BUS 都需要注册*/
            ret = hcc_bus_message_register(pst_bus_tmp, msg, cb, data);
            if(OAL_SUCC != ret)
            {
                oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);
                oal_print_hi11xx_log(HI11XX_LOG_ERR, "register bus %d failed, ret=%d", pst_bus_tmp->bus_id, ret);
                return ret;
            }
            flag++;
        }
    }
    oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);


    if(flag)
        return OAL_SUCC;
    else
        return -OAL_ENODEV;
}

oal_void hcc_message_unregister_etc(struct hcc_handler* hcc, oal_uint8 msg)
{
    oal_uint             irqsave;
    oal_dlist_head_stru  *pst_entry;
    hcc_bus*             pst_bus_tmp;
    oal_spin_lock_irq_save(&g_hcc_bus_res_lock, &irqsave);
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &g_hcc_bus_res_hdr)
    {
        pst_bus_tmp = OAL_DLIST_GET_ENTRY(pst_entry, hcc_bus, list);
        if(pst_bus_tmp->bus_dev == hcc->bus_dev)
        {
            /*注册在当前设备上的所有BUS 都需要注册*/
            hcc_bus_message_unregister(pst_bus_tmp, msg);
        }
    }
    oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);
}


hcc_bus* hcc_alloc_bus(oal_void)
{
    hcc_bus       *pst_bus = OAL_PTR_NULL;

    pst_bus = oal_memalloc(OAL_SIZEOF(hcc_bus));
    if(OAL_PTR_NULL == pst_bus)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc hcc bus failed, size:%u", (oal_uint32)OAL_SIZEOF(hcc_bus));
        return NULL;
    }

    OAL_MEMZERO((oal_void*)pst_bus, OAL_SIZEOF(hcc_bus));

    pst_bus->dev_id   = 0xFFFFFFFF;

    pst_bus->cap.align_size[HCC_TX] = 4;
    pst_bus->cap.align_size[HCC_RX] = 4;

    pst_bus->cap.is_full_duplex = 0;

    pst_bus->cap.max_trans_size = 0x7fffffff;
    return pst_bus;
}

oal_void hcc_free_bus(hcc_bus* pst_bus)
{
    if(OAL_LIKELY(NULL != pst_bus))
        oal_free(pst_bus);
}

oal_int32 hcc_send_message(struct hcc_handler* hcc, oal_uint32 val)
{
    oal_uint32 ret = OAL_SUCC;
    hcc_bus *hi_bus;

    if(OAL_UNLIKELY(NULL == hcc))
    {
        return -OAL_ENODEV;
    }

    hi_bus = HCC_TO_BUS(hcc);

    hcc_tx_transfer_lock(hcc);
    if(OAL_SUCC == hcc_bus_pm_wakeup_device(hi_bus))
    {
        ret = hcc_bus_send_message(hi_bus, val);
    }
    else
    {
        hcc_tx_transfer_unlock(hcc);
        return -OAL_EFAIL;
    }
    hcc_tx_transfer_unlock(hcc);

    return ret;
}

OAL_STATIC oal_void hcc_dev_res_exit_by_id(oal_int32 dev_max)
{
    oal_int32 i;
    hcc_bus_dev * pst_bus_dev;
    dev_max = OAL_MIN(dev_max, (oal_int32)OAL_ARRAY_SIZE(g_bus_dev_res));
    for(i = 0; i < dev_max; i++)
    {
        pst_bus_dev = &g_bus_dev_res[i];
        oal_wake_lock_exit(&pst_bus_dev->st_switch_wakelock);
        if(pst_bus_dev->is_wakeup_gpio_support)
        {
            oal_unregister_gpio_intr_etc(pst_bus_dev);
        }
    }
}

oal_void hcc_dev_res_exit(oal_void)
{
    hcc_dev_res_exit_by_id((oal_int32)OAL_ARRAY_SIZE(g_bus_dev_res));
}

oal_int32 hcc_dev_res_init(oal_void)
{
    oal_int32 i;
    oal_int32 ret;
    hcc_bus_dev * pst_bus_dev;
    for(i = 0; i < (oal_int32)OAL_ARRAY_SIZE(g_bus_dev_res); i++)
    {
        pst_bus_dev = &g_bus_dev_res[i];
        oal_spin_lock_init(&pst_bus_dev->st_irq_lock);
        oal_spin_lock_init(&pst_bus_dev->st_switch_lock);
        OAL_WAIT_QUEUE_INIT_HEAD(&pst_bus_dev->st_switch_request_wq);
        oal_wake_lock_init(&pst_bus_dev->st_switch_wakelock, "bus_dev_wakelock");
        if(pst_bus_dev->is_wakeup_gpio_support)
        {
            ret = oal_register_gpio_intr_etc(pst_bus_dev);
            if(ret)
                goto failed_reg_gpio_int;
        }
    }

    return OAL_SUCC;
failed_reg_gpio_int:
    hcc_dev_res_exit_by_id(i);
    return ret;
}

oal_int32 hcc_add_bus(hcc_bus* pst_bus, char* bus_name)
{
    oal_int32            ret;
    oal_int32            init_flag = 0;
    oal_uint             irqsave;
    oal_dlist_head_stru  *pst_entry;
    hcc_bus*             pst_bus_tmp;
    if(OAL_WARN_ON(NULL == pst_bus))
    {
        return -OAL_EINVAL;
    }

    /*Check duplicate bus*/
    oal_spin_lock_irq_save(&g_hcc_bus_res_lock, &irqsave);
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &g_hcc_bus_res_hdr)
    {
        pst_bus_tmp = OAL_DLIST_GET_ENTRY(pst_entry, hcc_bus, list);
        if(OAL_WARN_ON(OAL_TRUE == hcc_bus_is_match(pst_bus_tmp, pst_bus)))
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "bus[%p] is duplicate, type:%s, id:%d",
                            pst_bus, hcc_bus_get_bus_type_str(pst_bus->bus_type), pst_bus->bus_id);
            oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);
            return -OAL_EBUSY;
        }
    }

    oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);

    ret = hcc_bus_resource_alloc(pst_bus);
    if(OAL_SUCC != ret)
    {
        return ret;
    }

    OAL_SPRINTF(pst_bus->name, OAL_SIZEOF(pst_bus->name), "%s:%u", bus_name, pst_bus->bus_id);
    oal_spin_lock_irq_save(&g_hcc_bus_res_lock, &irqsave);
    /*If empty init dev res*/
    if(OAL_TRUE == oal_dlist_is_empty(&g_hcc_bus_res_hdr))
    {
        oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);
        ret = hcc_dev_res_init();
        oal_spin_lock_irq_save(&g_hcc_bus_res_lock, &irqsave);
        if(ret)
        {
            goto failed_dev_res_init;
        }
        init_flag = 1;
    }

    oal_dlist_add_tail(&pst_bus->list, &g_hcc_bus_res_hdr);
    ret = hcc_probe_bus(pst_bus);
    if(OAL_SUCC != ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc bus %s probed failed!", pst_bus->name);
        goto failed_probe_bus;
    }
    oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);

    return OAL_SUCC;
failed_probe_bus:
    oal_dlist_delete_entry(&pst_bus->list);
    if(init_flag)
    {
        oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);
        hcc_dev_res_exit();
        oal_spin_lock_irq_save(&g_hcc_bus_res_lock, &irqsave);
    }
failed_dev_res_init:
    oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);
    hcc_bus_resource_free(pst_bus);
    return ret;
}

oal_int32 hcc_remove_bus(hcc_bus* pst_bus)
{
    oal_uint             irqsave;

    if(OAL_WARN_ON(NULL == pst_bus))
    {
        return OAL_SUCC;
    }

    /*delete bus and remove from dev resource*/
    oal_spin_lock_irq_save(&g_hcc_bus_res_lock, &irqsave);

    oal_dlist_delete_entry(&pst_bus->list);

    oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);
    hcc_bus_resource_free(pst_bus);
    oal_spin_lock_irq_save(&g_hcc_bus_res_lock, &irqsave);

    if(OAL_TRUE == oal_dlist_is_empty(&g_hcc_bus_res_hdr))
    {
        oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);
        hcc_dev_res_exit();
        oal_spin_lock_irq_save(&g_hcc_bus_res_lock, &irqsave);
    }

    oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);
    return OAL_SUCC;
}

extern oal_workqueue_stru* wifi_get_exception_workqueue_etc(oal_void);

#if defined(CONFIG_ARCH_HISI)
/*Debug*/
oal_int32  hcc_bus_printbindcpu(oal_void)
{
    hcc_bus *hi_bus;
    struct hcc_handler* hcc;
    struct cpumask slow_cpus, fast_cpus;

    hisi_get_slow_cpus(&slow_cpus);
    hisi_get_fast_cpus(&fast_cpus);

    hi_bus = hcc_get_current_110x_bus();
    if(NULL == hi_bus)
    {
        return -OAL_EFAIL;
    }

    hcc = HBUS_TO_HCC(hi_bus);
    if(NULL == hcc)
    {
        return -OAL_EFAIL;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc cpu mask:0x%lx", cpumask_bits(&(hcc->hcc_transer_info.hcc_transfer_thread_etc->cpus_allowed))[0]);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc cpu fast_cpus:0x%lx", cpumask_bits(&fast_cpus)[0]);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc cpu slow_cpus:0x%lx", cpumask_bits(&slow_cpus)[0]);

    return OAL_SUCC;
}
#endif

oal_int32  hcc_bus_bindcpu(hcc_bus *hi_bus, oal_uint32 chan, oal_int32 is_bind)
{
    struct hcc_handler* hcc;
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_ENODEV;;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->opt_ops->bindcpu))
    {
        return -OAL_EIO;
    }

    hcc = HBUS_TO_HCC(hi_bus);

    if(hi_bus->is_bind == is_bind)
    {
        return OAL_SUCC;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc %s cpu", is_bind ? "bind":"unbind");

    hi_bus->is_bind = is_bind;

    if(OAL_UNLIKELY(NULL != hcc))
    {
        if(hcc->hcc_transer_info.hcc_transfer_thread_etc)
        {
#if defined(CONFIG_ARCH_HISI)
            struct cpumask slow_cpus, fast_cpus;
            hisi_get_slow_cpus(&slow_cpus);
            hisi_get_fast_cpus(&fast_cpus);
            cpumask_clear_cpu(OAL_BUS_HPCPU_NUM, &fast_cpus);
            //cpumask_clear_cpu(OAL_BUS_HPCPU_NUM_1, &fast_cpus);
            oal_print_hi11xx_log(HI11XX_LOG_DBG, "hcc thread hisi rt opt cpu");
            if(is_bind)
                set_cpus_allowed_ptr(hcc->hcc_transer_info.hcc_transfer_thread_etc, &fast_cpus);
            else
                set_cpus_allowed_ptr(hcc->hcc_transer_info.hcc_transfer_thread_etc, &slow_cpus);
#endif
        }
    }

    if(NULL != hi_bus->pst_rx_tsk)
    {
#if defined(CONFIG_ARCH_HISI)
            struct cpumask slow_cpus, fast_cpus;
            hisi_get_slow_cpus(&slow_cpus);
            hisi_get_fast_cpus(&fast_cpus);
            oal_print_hi11xx_log(HI11XX_LOG_DBG, "bus rx task hisi rt opt cpu");
            if(is_bind)
                set_cpus_allowed_ptr(hi_bus->pst_rx_tsk, &fast_cpus);
            else
                set_cpus_allowed_ptr(hi_bus->pst_rx_tsk, &slow_cpus);
#endif
    }

    return hi_bus->opt_ops->bindcpu(hi_bus, chan, is_bind);
}

EXPORT_SYMBOL_GPL(hcc_bus_bindcpu);

/*PM*/
oal_int32 hcc_bus_pm_wakeup_device(hcc_bus *hi_bus)
{
    oal_uint32 uret;

    if(OAL_WARN_ON(NULL == hi_bus))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hi_bus is null");
        return -OAL_EIO;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->pst_pm_callback))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "pst_pm_callback is null");
        return -OAL_EIO;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->pst_pm_callback->pm_wakeup_dev))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "pm_wakeup_dev is null");
        return -OAL_EIO;
    }

    uret = hi_bus->pst_pm_callback->pm_wakeup_dev();
    if(OAL_UNLIKELY(OAL_SUCC != uret))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "pm_wakeup_dev failed ret=%u, bus_type=%u", uret, hi_bus->bus_type);
        wlan_pm_dump_host_info_etc();
        return -OAL_EBUSY;
    }

    return OAL_SUCC;
}
EXPORT_SYMBOL_GPL(hcc_bus_pm_wakeup_device);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_void hcc_bus_exception_submit(hcc_bus *hi_bus, oal_int32 excep_type)
{
    oal_ulong flags;
    oal_workqueue_stru*  pst_excp_workqueue;

    DECLARE_DFT_TRACE_KEY_INFO("wifi_trans_exception", OAL_DFT_TRACE_EXCEP);
    if(OAL_UNLIKELY(NULL == hi_bus))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "[E]%s, hi_bus is null", __FUNCTION__);
        return;
    }

    pst_excp_workqueue = wifi_get_exception_workqueue_etc();
    if(OAL_UNLIKELY(NULL == pst_excp_workqueue))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "[E]%s, pst_excp_workqueue is null", __FUNCTION__);
        return;
    }

    /*disable bus*/
    hcc_bus_disable_state(hi_bus, OAL_BUS_STATE_RX);

    oal_spin_lock_irq_save(&hi_bus->bus_excp_lock, &flags);

    if(oal_work_is_busy(&hi_bus->bus_excp_worker))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "excep %d block, exception %d is working",excep_type, hi_bus->bus_excp_type);
        oal_spin_unlock_irq_restore(&hi_bus->bus_excp_lock, &flags);
        return;
    }

    hcc_change_state_exception_etc();
    hi_bus->bus_excp_type = excep_type;

    if(WIFI_DEVICE_PANIC != excep_type)
    {
#if defined(_PRE_CONFIG_GPIO_TO_SSI_DEBUG)
        unsigned long long set = 0x0;
        oal_spin_unlock_irq_restore(&hi_bus->bus_excp_lock, &flags);

        if(HI1XX_ANDROID_BUILD_VARIANT_USER == hi11xx_get_android_build_variant())
        {
            if(true == bfgx_is_shutdown_etc())
            {
                set = (SSI_MODULE_MASK_ARM_REG|SSI_MODULE_MASK_AON_CUT);
                oal_print_hi11xx_log(HI11XX_LOG_INFO, "bfgx is shutdown");
            }
            else
            {
                //set = SSI_MODULE_MASK_ARM_REG;
                set = 0x0;
            }

            if(!oal_print_rate_limit(24*PRINT_RATE_HOUR))
            {
                /*24*60分钟打印一次*/
                set = 0x0;
            }
        }
        else
        {
            if(HCC_BUS_SDIO == hi_bus->bus_type)
            {
                set = SSI_MODULE_MASK_ARM_REG;
            }

            if(HCC_BUS_PCIE == hi_bus->bus_type)
            {
                set = SSI_MODULE_MASK_COMM | SSI_MODULE_MASK_PCIE_CFG;
#ifdef PLATFORM_SSI_FULL_LOG
                set |= SSI_MODULE_MASK_PCIE_DBI;
#endif
            }
        }

        ssi_dump_device_regs(set);

        oal_spin_lock_irq_save(&hi_bus->bus_excp_lock, &flags);
#endif
    }

    oal_dft_print_error_key_info_etc();

    if(hcc_exception_enable)
    {
        /*stop switch thread before dfr*/
        hcc_dev_switch_disable(hi_bus->dev_id);
        oal_wake_lock_timeout(&hi_bus->bus_excp_wlock, 10*1000);
        queue_work(pst_excp_workqueue, &hi_bus->bus_excp_worker);
    }
    else
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "wifi exception memdump function disabled");
    }
    oal_spin_unlock_irq_restore(&hi_bus->bus_excp_lock, &flags);

}
/*lint -e19*/
oal_module_symbol(hcc_bus_exception_submit);
/*lint +e19*/

oal_int32 hcc_bus_exception_is_busy(hcc_bus *hi_bus )
{
    if(OAL_UNLIKELY(NULL == hi_bus))
    {
        return OAL_FALSE;
    }

    if(oal_work_is_busy(&hi_bus->bus_excp_worker))
    {
        /*mem dump is processing,
          can't power off or submit repeat*/
        return OAL_TRUE;
    }

    return OAL_FALSE;
}
/*lint -e19*/
oal_module_symbol(hcc_bus_exception_is_busy);
/*lint +e19*/

oal_int32 oal_trigger_bus_exception(hcc_bus *hi_bus, oal_int32 is_sync)
{
    oal_ulong timeout_jiffies;
    if(OAL_UNLIKELY(NULL == hi_bus))
    {
        return OAL_FALSE;
    }

    if(OAL_TRUE == hcc_bus_exception_is_busy(hi_bus))
    {
        return OAL_TRUE;
    }

    /*trigger device panic*/
    if(hcc_bus_send_message(hi_bus, H2D_MSG_TEST))
    {
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "send device panic message failed!");
        return OAL_FALSE;
    }

    if(OAL_TRUE != is_sync)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "wifi exception is doing...");
        return OAL_TRUE;
    }

    /*wait device panic*/
    timeout_jiffies = jiffies + OAL_MSECS_TO_JIFFIES(2000);
    for(;;)
    {
        if(OAL_TRUE == hcc_bus_exception_is_busy(hi_bus))
        {
            break;
        }

        if(time_after(jiffies, timeout_jiffies))
        {
            oal_print_hi11xx_log(HI11XX_LOG_WARN, "wait panic message timeout!");
            return OAL_FALSE;
        }

        oal_msleep(OAL_JIFFIES_TO_MSECS(1));
        up(&hi_bus->rx_sema);
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "trigger wifi exception manually sucuess");
    return OAL_TRUE;
}
/*lint -e19*/
oal_module_symbol(oal_trigger_bus_exception);
/*lint +e19*/

oal_void hcc_bus_exception_submit_test(oal_void)
{
    hcc_bus_exception_submit(hcc_get_current_110x_bus(), WIFI_TRANS_FAIL);
}

oal_int32 hcc_bus_device_panic_callback(void *data)
{
    hcc_bus* hi_bus = (hcc_bus*)data;
    hcc_bus_disable_state(hi_bus, OAL_BUS_STATE_ALL);
    hcc_bus_exception_submit(hi_bus, WIFI_DEVICE_PANIC);

         CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_DEVICE_PANIC);

    return OAL_SUCC;
}

oal_int32 hcc_bus_device_init_ready(void *data)
{
    hcc_bus* hi_bus = (hcc_bus*)data;
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "device ready, bus:%s", hcc_bus_get_bus_type_str(hi_bus->bus_type));
    OAL_COMPLETE(&hi_bus->st_device_ready);
	return OAL_SUCC;
}

/*检查DEVICE WAKEUP HOST gpio 是否拉高。*/
oal_int32 oal_dev2host_gpio_hold_time_check_etc(oal_uint32 switch_timeout, oal_uint32 hold_time)
{
    oal_ulong timeout;
    oal_uint32 gpio_value;
    oal_int32 state = 0;/*0 init, 1 gpio to high*/
    declare_time_cost_stru(cost);

    if(!switch_timeout)
    {
        switch_timeout = 200;
    }

    timeout = jiffies + OAL_MSECS_TO_JIFFIES(switch_timeout);
    oal_get_time_cost_start(cost);
    for(;;)
    {
        gpio_value = board_get_wlan_wkup_gpio_val_etc();
        if(0 == state)
        {
            if(0 == gpio_value)
            {
                if(time_after(jiffies, timeout))
                {
                    oal_print_hi11xx_log(HI11XX_LOG_ERR, "[E]wait wakeup gpio to high timeout [%u] ms", switch_timeout);
                    return OAL_FALSE;
                }
                else
                {
                    /*wifi power*/
                    if(wlan_is_shutdown_etc())
                    {
                        OAL_IO_PRINT("[E]hold_time_check:wifi is shutdown!\n");
                        return OAL_FALSE;
                    }
                    oal_usleep_range(10, 20);
                    cpu_relax();
                    continue;
                }
            }
            else
            {
                oal_get_time_cost_end(cost);
                oal_calc_time_cost_sub(cost);
                oal_print_hi11xx_log(HI11XX_LOG_INFO, "device reset  ip cost %llu us", time_cost_var_sub(cost));
                timeout = jiffies + OAL_MSECS_TO_JIFFIES(hold_time);
                state = 1;
                continue;
            }
        }
        else if(1 == state)
        {
            if(0 == gpio_value)
            {
                oal_print_hi11xx_log(HI11XX_LOG_INFO, "[E]gpio pull down again!");
                return OAL_FALSE;
            }
            else
            {
                if(time_after(jiffies, timeout))
                {
                    /*gpio high and hold enough time.*/
                    return OAL_TRUE;
                }
                else
                {
                    /*wifi power*/
                    if(wlan_is_shutdown_etc())
                    {
                        OAL_IO_PRINT("[E]hold_time_check:wifi is shutdown!\n");
                        return OAL_FALSE;
                    }
                    oal_usleep_range(10, 20);
                    cpu_relax();
                    continue;
                }
            }
        }
        else
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "[E]error state=%d", state);
            return OAL_FALSE;
        }
    }
}

/*check whether device had panic*/
oal_int32 hcc_bus_panic_status_check(hcc_bus* hi_bus)
{
    oal_int32 ret = OAL_FALSE;

    if(WIFI_DEVICE_PANIC == hi_bus->bus_excp_type)
    {
        ret = OAL_TRUE;
    }

 /*gpio mode*/
    if(OAL_TRUE == oal_dev2host_gpio_hold_time_check_etc(5000, 100))
    {
        ret = OAL_TRUE;
    }
    else
    {
        ret = OAL_FALSE;
    }
    return ret;

}

/*when recv error data from device, dump device mem*/
#define WCPU_PANIC_MEMDUMP_MAX_SIZE		(0x3c00) /*15K, 0x1004_0000--0x1005_FFFF , 128K pktmem*/
#define WCPU_PANIC_MEMDUMP_MAX_ADDR		(0x10060000)
/*0x1005C400*/
#define WCPU_PANIC_MEMDUMP_STORE_ADDR		(WCPU_PANIC_MEMDUMP_MAX_ADDR-WCPU_PANIC_MEMDUMP_MAX_SIZE)


struct st_wifi_dump_mem_info g_hi1102_meminfo[] =
{
    {
        .file_name = "wifi_device_panic_mem",
        .mem_addr =  WCPU_PANIC_MEMDUMP_STORE_ADDR,
        .size =     WCPU_PANIC_MEMDUMP_MAX_SIZE
    },
    {
        .file_name = "wifi_device_tcm_l2ram",
        .mem_addr =  0x70000,
        .size =     512*1024,/*512KB*/
    },
    {
        .file_name = "wifi_device_pkt_mem",
        .mem_addr =  0x10020000,
        .size =     358*1024,
    },

};

/*for 1103*/
/** mpw2 config **/
struct st_wifi_dump_mem_info g_hi1103_mpw2_meminfo[] =
{
    {
        .file_name = "wifi_device_panic_mem",
        .mem_addr =  0x6007c400,
        .size =     WCPU_PANIC_MEMDUMP_MAX_SIZE,/*15KB*/
    },
    {
        .file_name = "wifi_device_itcm_ram",
        .mem_addr =  0x4000,
        .size =     0x88000,/*544KB*/
    },
    {
        .file_name = "wifi_device_dtcm_ram",
        .mem_addr =  0x20000000,
        .size =     0x68000,/*416KB*/
    },
    {
        .file_name = "wifi_device_pkt_mem",
        .mem_addr =  0x60000000,
        .size =     0x90000,/*576KB*/
    },

};

/** pilot config **/
struct st_wifi_dump_mem_info g_hi1103_pilot_meminfo[] =
{
    {
        .file_name = "wifi_device_panic_mem",
        .mem_addr =  0x6007c400,
        .size =     WCPU_PANIC_MEMDUMP_MAX_SIZE,/*15KB*/
    },
    {
        .file_name = "wifi_device_itcm_ram",
        .mem_addr =  0x10000,
        .size =     0x98000,/*608KB*/
    },
    {
        .file_name = "wifi_device_dtcm_ram",
        .mem_addr =  0x20018000,
        .size =     0x68000,/*416KB*/
    },
    {
        .file_name = "wifi_device_pkt_mem",
        .mem_addr =  0x60000000,
        .size =     0x80000,/*512KB*/
    },

};

void  hcc_bus_exception_handler(oal_work_stru *work)
{
    oal_int32 ret = OAL_SUCC;
    hcc_bus* hi_bus = hcc_get_current_110x_bus();
    struct st_wifi_dump_mem_info *mem_info;
    oal_uint32 size;
    oal_int32 excp_type;

    if(NULL == hi_bus)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hi_bus is null");
        return;
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
        mem_info = g_hi1102_meminfo;
        size = sizeof(g_hi1102_meminfo)/sizeof(struct st_wifi_dump_mem_info);
#elif (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    if (HI1103_ASIC_PILOT == get_hi1103_asic_type())
    {
        mem_info = g_hi1103_pilot_meminfo;
        size = sizeof(g_hi1103_pilot_meminfo)/sizeof(struct st_wifi_dump_mem_info);
    }
    else
    {
        mem_info = g_hi1103_mpw2_meminfo;
        size = sizeof(g_hi1103_mpw2_meminfo)/sizeof(struct st_wifi_dump_mem_info);
    }
#else
    #error unadapt choose
#endif

    hcc_bus_wake_lock(hi_bus);
    if(WIFI_DEVICE_PANIC == hi_bus->bus_excp_type)
    {
        DECLARE_DFT_TRACE_KEY_INFO("wifi_device_panic",OAL_DFT_TRACE_EXCEP);
        OAM_ERROR_LOG0(0,OAM_SF_ANY,"[E]WiFi Device Panic");
    }

    if(WIFI_TRANS_FAIL == hi_bus->bus_excp_type)
    {
        DECLARE_DFT_TRACE_KEY_INFO("wifi_trans_fail",OAL_DFT_TRACE_EXCEP);
        OAM_ERROR_LOG0(0,OAM_SF_ANY,"[E]wifi_trans_fail");
    }

    hcc_bus_disable_state(hi_bus, OAL_BUS_STATE_ALL);

    oal_dft_print_all_key_info_etc();

#ifdef CONFIG_PRINTK
    hwifi_panic_log_dump_etc(KERN_DEBUG);
    hcc_bus_print_trans_info(hi_bus, 0);
#endif

    if(hi_bus->pst_pm_callback->pm_disable)
    {
        hi_bus->pst_pm_callback->pm_disable(OAL_FALSE);
    }

    excp_type = hi_bus->bus_excp_type;
    if(OAL_TRUE == hcc_bus_panic_status_check(hi_bus))
    {
        /*device panic*/
        OAM_ERROR_LOG0(0,OAM_SF_ANY,"[E]check wakeup gpio ok, wcpu had panic");
        if(hi_bus->bus_excp_type != WIFI_DEVICE_PANIC)
        {
            OAM_ERROR_LOG1(0,OAM_SF_ANY,"[E]change sdio excp type from %d to device panic ", hi_bus->bus_excp_type);
        }
        hi_bus->bus_excp_type = WIFI_DEVICE_PANIC;
        oal_msleep(10);
        //ssi_dump_device_regs(SSI_MODULE_MASK_COMM);
    }
    else
    {
        /*device is't panic, reset sdio ip from host*/
        OAM_ERROR_LOG0(0,OAM_SF_ANY,"[E]check wakeup gpio fail, wcpu crash");
        hi_bus->bus_excp_type = WIFI_TRANS_FAIL;
    }

    if(wlan_is_shutdown_etc())
    {
        OAL_IO_PRINT("[E]dfr ignored, wifi shutdown\n");
        return;
    }
    /*close sdio data transfer when device panic.*/

    ret = wifi_exception_mem_dump_etc(mem_info,size, hi_bus->bus_excp_type);
    if( ret < 0)
    {
        OAM_ERROR_LOG0(0,OAM_SF_ANY,"Panic File Save Failed!");
    }
    else
    {
        OAM_ERROR_LOG0(0,OAM_SF_ANY,"Panic File Save OK!");
    }

    if(wlan_is_shutdown_etc())
    {
        OAM_ERROR_LOG0(0,OAM_SF_ANY,"wifi is shutdown, dfr submit ignored");
    }
    else
    {
        wifi_exception_work_submit_etc(excp_type);
    }

    hcc_bus_wake_unlock(hi_bus);
}

#undef COFNIG_TEST_SDIO_INT_LOSS


OAL_STATIC oal_int32 hcc_bus_rxdata_thread(oal_void *data)
{
#ifdef COFNIG_TEST_SDIO_INT_LOSS
    int i = 0;
#endif
    hcc_bus         *hi_bus;

    struct sched_param       param;

    hi_bus = (hcc_bus *)data;

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hisi wifi sched ctrl rx thread high speed");
    param.sched_priority = OAL_BUS_RXDATA_THREAD_PRIORITY;
    oal_set_thread_property_etc(current,
                            OAL_BUS_RX_THREAD_POLICY,
                            &param,
                            OAL_BUS_RX_THREAD_NICE);


    allow_signal(SIGTERM);
    while (OAL_LIKELY(!down_interruptible(&hi_bus->rx_sema)))
    {
        /* smp_read_barrier_depends(); */
        if (OAL_UNLIKELY(kthread_should_stop()))
        {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "exit gpio rx thread");
            break;
        }
#ifdef COFNIG_TEST_SDIO_INT_LOSS
        for(i = 0; i < 5; i++)
        {
#endif
	        /* start to read GPIO interrupt */
	        hcc_bus_rx_transfer_lock(hi_bus);
            if(hi_bus->opt_ops->wlan_gpio_rxdata_proc)
            {
                hi_bus->opt_ops->wlan_gpio_rxdata_proc(hi_bus);
            }
	        hcc_bus_rx_transfer_unlock(hi_bus);
#ifdef COFNIG_TEST_SDIO_INT_LOSS
        }
#endif
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "gpio rx data is terminated");
    return OAL_SUCC;
}


oal_uint64 oal_get_gpio_int_count_para_etc(oal_void)
{
    hcc_bus *hi_bus;

    hi_bus = hcc_get_current_110x_bus();

    if (NULL == hi_bus)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hi_bus is null");
        return 0;
    }

    return hi_bus->gpio_int_count;
}

OAL_STATIC oal_void hcc_bus_sched_gpio_task(hcc_bus*     pst_bus, oal_int32 irq)
{
    if(OAL_LIKELY(pst_bus->opt_ops->wlan_gpio_handler))
    {
        pst_bus->opt_ops->wlan_gpio_handler(pst_bus, irq);
    }
    else
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wlan_gpio_handler is NULL, irq:%d, bus type:%s, disable gpio interrupt!",
                                irq, hcc_bus_get_bus_type_str(pst_bus->bus_type));
    }
}


OAL_STATIC irqreturn_t hcc_bus_wlan_gpio_irq(oal_int32 irq, oal_void *dev_id)
{
    hcc_bus*                pst_bus;
    hcc_bus_dev             *pst_bus_dev;

    pst_bus_dev = (hcc_bus_dev *)dev_id;
    if(OAL_UNLIKELY(NULL == pst_bus_dev))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "dev id  is NULL, irq:%d", irq);
        return IRQ_HANDLED;
    }

    pst_bus = HDEV_TO_HBUS(pst_bus_dev);

    if(OAL_UNLIKELY(NULL == pst_bus))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "curr bus is NULL, irq:%d, bus_dev id:%u", irq, pst_bus_dev->dev_id);
        return IRQ_HANDLED;
    }

    if(OAL_LIKELY(pst_bus->opt_ops->wlan_gpio_handler))
    {
        hcc_bus_sched_gpio_task(pst_bus, irq);
    }
    else
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wlan_gpio_handler is NULL, irq:%d, bus_dev id:%u, disable gpio interrupt!",
                                irq, pst_bus_dev->dev_id);
        oal_wlan_gpio_intr_enable_etc(pst_bus_dev, OAL_FALSE);
    }

    return IRQ_HANDLED;
}


oal_int32 oal_register_gpio_intr_etc(hcc_bus_dev *pst_bus_dev)
{
    oal_int32 ret = OAL_SUCC;
    BOARD_INFO * pst_board = get_hi110x_board_info_etc();

    unsigned int wlan_irq = pst_board->wlan_irq;

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "wlan gpio irq = %u", wlan_irq);
    if(!wlan_irq)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "[BUS][ERROR]wlan irq should't be 0");
        return -OAL_EFAIL;
    }

    //oal_spin_lock_init(&pst_bus_dev->st_irq_lock);
    pst_bus_dev->ul_wlan_irq = pst_board->wlan_irq;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44))
   ret = request_irq(wlan_irq, hcc_bus_wlan_gpio_irq, IRQF_NO_SUSPEND | IRQF_TRIGGER_RISING | IRQF_DISABLED, "wifi_gpio_intr", pst_bus_dev);
#else
    /*低版本内核没有IRQF_NO_SUSPEND标记*/
   ret = request_irq(wlan_irq, hcc_bus_wlan_gpio_irq, IRQF_TRIGGER_RISING | IRQF_DISABLED, "wifi_gpio_intr", pst_bus_dev);
#endif
   if (ret < 0)
   {
       oal_print_hi11xx_log(HI11XX_LOG_ERR, "failed to request wlan irq");

       return ret;
   }

   pst_bus_dev->ul_irq_stat = 0;/*irq enabled default.*/

   oal_wlan_gpio_intr_enable_etc(pst_bus_dev,OAL_FALSE);

   oal_print_hi11xx_log(HI11XX_LOG_INFO, "success to register sdio gpio intr");
   return OAL_SUCC;

}


oal_void oal_unregister_gpio_intr_etc(hcc_bus_dev *pst_bus_dev)
{
    /* disable wlan irq */
    oal_wlan_gpio_intr_enable_etc(pst_bus_dev,OAL_FALSE);

    /* free irq when sdio driver deinit */
    free_irq(pst_bus_dev->ul_wlan_irq, pst_bus_dev);
    //oal_thread_stop_etc(hi_bus->pst_rx_tsk, &hi_bus->rx_sema);
    //hi_bus->pst_rx_tsk = NULL;
}


oal_void oal_wlan_gpio_intr_enable_etc(hcc_bus_dev *pst_bus_dev,oal_uint32  ul_en)
{
    oal_uint            flags;

    oal_spin_lock_irq_save(&pst_bus_dev->st_irq_lock, &flags);
    if (ul_en)
    {
        /*不再支持中断开关嵌套*/
        if(pst_bus_dev->ul_irq_stat)
        {
            enable_irq(pst_bus_dev->ul_wlan_irq);
            oal_print_hi11xx_log(HI11XX_LOG_DBG, "gpio irq: %d enable", pst_bus_dev->ul_wlan_irq);
            pst_bus_dev->ul_irq_stat = 0;
        }

    }
    else
    {
        if(!pst_bus_dev->ul_irq_stat)
        {
            disable_irq_nosync(pst_bus_dev->ul_wlan_irq);
            oal_print_hi11xx_log(HI11XX_LOG_DBG, "gpio irq: %d disable nosync", pst_bus_dev->ul_wlan_irq);
            pst_bus_dev->ul_irq_stat = 1;
        }
    }
    oal_spin_unlock_irq_restore(&pst_bus_dev->st_irq_lock, &flags);
}

OAL_STATIC OAL_INLINE oal_void hcc_bus_print_state(oal_uint32 old_state, oal_uint32 new_state)
{
    if(old_state != new_state)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "bus state changed, tx[%s=>%s],rx[%s=>%s]",
                    (old_state & OAL_BUS_STATE_TX) ? "on " :"off",
                    (new_state & OAL_BUS_STATE_TX) ? "on " :"off",
                    (old_state & OAL_BUS_STATE_RX) ? "on " :"off",
                    (new_state & OAL_BUS_STATE_RX) ? "on " :"off");
    }
}




oal_void hcc_bus_wakelocks_release_detect(hcc_bus *pst_bus)
{
    /*before call this function , please make sure the rx/tx queue is empty and no data transfer!!*/
   if(OAL_WARN_ON(!pst_bus))
   {
         oal_print_hi11xx_log(HI11XX_LOG_ERR, "[E]pst_bus is error can not release wake locks");
         return;
   };

    if (hcc_bus_wakelock_active(pst_bus))
    {
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) )
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "[E]We still hold %s   %lu wake locks, Now release all",
                    pst_bus->st_bus_wakelock.st_wakelock.name,
                    pst_bus->st_bus_wakelock.lock_count);
#endif
        DECLARE_DFT_TRACE_KEY_INFO("wlan_wakelock_error_hold", OAL_DFT_TRACE_EXCEP);
        pst_bus->st_bus_wakelock.lock_count = 1;
        hcc_bus_wake_unlock(pst_bus);
    }
}

oal_int32 hcc_bus_enable(hcc_bus* pst_bus)
{
    return OAL_SUCC;
}

oal_int32 hcc_bus_disable(hcc_bus* pst_bus)
{
    return OAL_SUCC;
}

oal_int32 hcc_bus_resource_alloc(hcc_bus* pst_bus)
{
//    oal_int32 ret;
    oal_uint32 len;
    char* wakelock_name;
    oal_uint8 name[200];

    if (OAL_WARN_ON(!pst_bus))
    {
           oal_print_hi11xx_log(HI11XX_LOG_ERR, "pst_bus is null,alloc fail ");
           return -OAL_EFAIL;
    };

    //name[0] = 0;
    OAL_SPRINTF(name, OAL_SIZEOF(name), "wlan_bus_lock/%s:%u", hcc_bus_get_bus_type_str(pst_bus->bus_type), pst_bus->dev_id);

    len = (oal_uint32)OAL_STRLEN(name) + 1;
    wakelock_name = oal_memalloc(len);
    if(NULL == wakelock_name)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc wakelock name space failed, len=%u, [%s]", len , name);
        return -OAL_ENOMEM;
    }

    oal_memcopy((oal_void*)wakelock_name, (const oal_void *)name, len);

    oal_wake_lock_init(&pst_bus->st_bus_wakelock, wakelock_name);

    pst_bus->st_bus_wakelock.debug = hcc_bus_wakelock_debug;

    OAL_INIT_COMPLETION(&pst_bus->st_device_ready);

    //name[0] = 0;
    OAL_SPRINTF(name, OAL_SIZEOF(name), "wlan_bus_rx/%s:%u", hcc_bus_get_bus_type_str(pst_bus->bus_type), pst_bus->dev_id);

    /* create thread for gpio rx data in interrupt handler*/
    pst_bus->pst_rx_tsk = oal_thread_create_etc(hcc_bus_rxdata_thread,
                            (hcc_bus *)pst_bus,
                            &pst_bus->rx_sema,
                            name,
                            OAL_BUS_RX_THREAD_POLICY,
                            OAL_BUS_RXDATA_THREAD_PRIORITY,g_lCpuId);
    if (NULL == pst_bus->pst_rx_tsk)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "[SDIO][ERROR]failed to create rx data thread");
        oal_free(wakelock_name);
        oal_wake_lock_exit(&pst_bus->st_bus_wakelock);
        return -OAL_EFAIL;
    }
    mutex_init(&pst_bus->rx_transfer_lock);

    oal_atomic_set(&g_wakeup_dev_wait_ack_etc,0);
    oal_atomic_set(&g_bus_powerup_dev_wait_ack,0);

    sema_init(&pst_bus->sr_wake_sema, 1);

    /*init exception work*/
    oal_spin_lock_init(&pst_bus->bus_excp_lock);
    OAL_INIT_WORK(&pst_bus->bus_excp_worker, hcc_bus_exception_handler);
    oal_wake_lock_init(&pst_bus->bus_excp_wlock, hcc_bus_get_bus_type_str(pst_bus->bus_type));


    hcc_bus_message_register(pst_bus,
                             D2H_MSG_DEVICE_PANIC,
                             hcc_bus_device_panic_callback,
                             pst_bus);

    hcc_bus_message_register(pst_bus,
                             D2H_MSG_WLAN_READY,
                             hcc_bus_device_init_ready,
                             pst_bus);

    g_hisi_wifi_bus = pst_bus;

    return OAL_SUCC;
#if 0
 fail_gpio_intr:
    oal_free(wakelock_name);
    oal_wake_lock_exit(&pst_bus->st_bus_wakelock);
    oal_thread_stop_etc(pst_bus->pst_rx_tsk, &pst_bus->rx_sema);
    pst_bus->pst_rx_tsk = NULL;
    mutex_destroy(&pst_bus->rx_transfer_lock);

    return OAL_FAIL;
#endif
}

oal_void hcc_bus_resource_free(hcc_bus* pst_bus)
{
    char* name = NULL;
    /*include tread stop and irq free*/
    //oal_unregister_gpio_intr_etc(pst_bus);
    if(NULL != pst_bus->pst_rx_tsk)
    {
        oal_thread_stop_etc(pst_bus->pst_rx_tsk, &pst_bus->rx_sema);
        pst_bus->pst_rx_tsk = NULL;
    }
    mutex_destroy(&pst_bus->rx_transfer_lock);
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) )
    name = (char*)pst_bus->st_bus_wakelock.st_wakelock.name;
#endif
    oal_wake_lock_exit(&pst_bus->st_bus_wakelock);

    oal_wake_lock_exit(&pst_bus->bus_excp_wlock);
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) )
    oal_free((oal_void*)name);
#endif

}

oal_int32 hcc_dev_switch_thread_create(hcc_bus_dev * bus_dev)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_int32 ret;
    struct task_struct      *tsk;
    struct sched_param       param;

    tsk = kthread_create(hcc_bus_switch_thread, (oal_void*)bus_dev, "switch_bus/%d", bus_dev->dev_id);
    if (OAL_IS_ERR_OR_NULL(tsk))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "failed to run thread:switch_bus/%d, task=%p", bus_dev->dev_id, tsk);
        return -OAL_EFAIL;
    }

    bus_dev->switch_bus_req = NULL;

    bus_dev->pst_switch_task = tsk;

    /* set thread priority and schedule policy */
    param.sched_priority = 99;
    ret = sched_setscheduler(tsk, SCHED_FIFO, &param);
    if(OAL_UNLIKELY(ret))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "switch_bus/%d sched_setscheduler failed! ret =%d, prio=%d",bus_dev->dev_id, ret, 99);
        //return ret;
    }

    wake_up_process(tsk);
#endif
    return OAL_SUCC;
}

oal_void hcc_dev_switch_thread_destory(hcc_bus_dev * bus_dev)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if(bus_dev->pst_switch_task)
    {
        /*sema_sync can be NULL*/
        /* create thread for gpio rx data in interrupt handler*/
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "stop switch task");
        send_sig(SIGTERM, bus_dev->pst_switch_task, 1);
        kthread_stop(bus_dev->pst_switch_task);
        bus_dev->pst_switch_task = NULL;
    }
#endif
}

oal_int32 hcc_dev_switch_enable(oal_uint32 dev_id)
{
    oal_int32 ret;
    oal_ulong flags;

    hcc_bus_dev* pst_bus_dev = hcc_get_bus_dev(dev_id);
    if(OAL_WARN_ON(NULL == pst_bus_dev))
    {
        return -OAL_ENODEV;
    }

    if(0 == pst_bus_dev->bus_switch_enable)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc switch disable");
        return OAL_SUCC;
    }

    oal_spin_lock_irq_save(&pst_bus_dev->st_switch_lock, &flags);

    /*default no switch request*/
    pst_bus_dev->switch_bus_req = NULL;

    oal_spin_unlock_irq_restore(&pst_bus_dev->st_switch_lock, &flags);

    ret = hcc_dev_switch_thread_create(pst_bus_dev);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc dev switch enable ret=%d", ret);

    return ret;
}

oal_int32 hcc_dev_switch_disable(oal_uint32 dev_id)
{
    hcc_bus_dev* pst_bus_dev = hcc_get_bus_dev(dev_id);
    if(OAL_WARN_ON(NULL == pst_bus_dev))
    {
        return -OAL_ENODEV;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc dev switch disable+");
    hcc_dev_switch_thread_destory(pst_bus_dev);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc dev switch disable-");

    return OAL_SUCC;
}

OAL_STATIC oal_void hcc_dev_pps_count_timeout(oal_uint arg)
{
    hcc_bus_dev * pst_bus_dev = (hcc_bus_dev *)arg;
    if(OAL_WARN_ON(NULL == pst_bus_dev))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "bus dev is null");
        return;
    }

    if(!hcc_bus_auto_sched_is_support(pst_bus_dev->dev_id))
    {
        return;
    }

    hcc_bus_performance_core_schedule(pst_bus_dev->dev_id);

    oal_timer_start(&pst_bus_dev->bus_pps_timer, HCC_BUS_PPS_COUNT_TIMEOUT);
}

oal_void hcc_dev_exit(oal_void)
{
    oal_int32 i;
    hcc_bus_dev * pst_bus_dev;

    for(i = 0; i < (oal_int32)OAL_ARRAY_SIZE(g_bus_dev_res); i++)
    {
        pst_bus_dev = &g_bus_dev_res[i];

        hcc_bus_performance_core_exit(pst_bus_dev->dev_id);

        if(NULL != pst_bus_dev->hcc)
        {
            hcc_dev_switch_thread_destory(pst_bus_dev);
            hcc_module_exit_etc(pst_bus_dev->hcc);
            pst_bus_dev->hcc = NULL;
        }
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    wlan_pm_unregister_notifier(&hcc_bus_pm_notifier);
#endif

}

oal_int32 hcc_dev_init(oal_void)
{
    oal_int32 i;
    oal_int32 ret;
    hcc_bus_dev * pst_bus_dev;
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc_dev_init we had %d devs", (oal_int32)OAL_ARRAY_SIZE(g_bus_dev_res));

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    wlan_pm_register_notifier(&hcc_bus_pm_notifier);
#endif

    for(i = 0; i < (oal_int32)OAL_ARRAY_SIZE(g_bus_dev_res); i++)
    {
        pst_bus_dev = &g_bus_dev_res[i];
        ret = hcc_select_bus(pst_bus_dev->dev_id, pst_bus_dev->init_bus_type);
        if(OAL_SUCC != ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "bus dev %s select bus failed, ret=%d", pst_bus_dev->name,ret);
            goto hcc_init_fail;
        }

        pst_bus_dev->hcc = hcc_module_init_etc(pst_bus_dev);
        if(NULL == pst_bus_dev->hcc)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc hcc failed!");
            goto hcc_init_fail;
        }

        hcc_bus_performance_core_init(pst_bus_dev->dev_id);

        OAL_INIT_COMPLETION(&pst_bus_dev->st_switch_powerup_ready);
        OAL_INIT_COMPLETION(&pst_bus_dev->st_switch_powerdown_ready);
        OAL_WAIT_QUEUE_INIT_HEAD(&pst_bus_dev->st_swtich_ack_wq);
        hcc_rx_register_etc(pst_bus_dev->hcc, HCC_ACTION_TYPE_BUS, hcc_bus_rx_response, NULL);

        /*create switch bus thread*/
        //hcc_dev_switch_thread_create(pst_bus_dev);

        oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc init dev %d succ", pst_bus_dev->dev_id);
    }

    return OAL_SUCC;

hcc_init_fail:
    hcc_dev_exit();
    return -OAL_EFAIL;
}

/*枚举接口设备*/
oal_int32 oal_wifi_platform_load_dev(oal_void)
{
    oal_int32 ret = OAL_SUCC;

    ret = oal_wifi_platform_load_sdio();
    if(OAL_SUCC != ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "load sdio failed ret=%d", ret);
        return ret;
    }

    ret = oal_wifi_platform_load_pcie();
    if(OAL_SUCC != ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "load pcie failed ret=%d", ret);
        goto failed_load_pcie;
    }

    return OAL_SUCC;
failed_load_pcie:
    oal_wifi_platform_unload_sdio();
    return ret;
}

/*卸载接口设备*/
oal_void oal_wifi_platform_unload_dev(oal_void)
{
    oal_wifi_platform_unload_pcie();
    oal_wifi_platform_unload_sdio();
}


#else
oal_void hcc_dev_exit(oal_void)
{
}

oal_int32 hcc_dev_init(oal_void)
{
    return OAL_SUCC;
}

oal_int32 hcc_bus_resource_alloc(hcc_bus* pst_bus)
{
    OAL_REFERENCE(pst_bus);
    return OAL_SUCC;
}

oal_void hcc_bus_resource_free(hcc_bus* pst_bus)
{
    OAL_REFERENCE(pst_bus);
}

oal_int32 oal_register_gpio_intr_etc(hcc_bus_dev *pst_bus_dev)
{
    OAL_REFERENCE(pst_bus_dev);
    return OAL_SUCC;
}

oal_void oal_unregister_gpio_intr_etc(hcc_bus_dev *pst_bus_dev)
{
    OAL_REFERENCE(pst_bus_dev);
}

void  hcc_bus_exception_handler(oal_work_stru *work)
{
    OAL_REFERENCE(work);
}

oal_int32 oal_trigger_bus_exception(hcc_bus *hi_bus, oal_int32 is_sync)
{
    OAL_REFERENCE(hi_bus);
    OAL_REFERENCE(is_sync);
    return OAL_SUCC;
}

/*枚举接口设备*/
oal_int32 oal_wifi_platform_load_dev(oal_void)
{
    return OAL_SUCC;
}

/*卸载接口设备*/
oal_void oal_wifi_platform_unload_dev(oal_void)
{
}
#endif

oal_uint32 hcc_bus_dump_mem_check(hcc_bus *hi_bus)
{
    OAL_REFERENCE(hi_bus);
    return !!bus_dump_mem_flag;
}
/*lint -e19*/
oal_module_symbol(hcc_bus_dump_mem_check);
/*lint +e19*/


oal_int32 hcc_bus_message_register(hcc_bus *hi_bus,
                                            oal_uint8 msg, hcc_bus_msg_rx cb,
                                            oal_void* data)
{
    if(NULL == hi_bus || msg >= D2H_MSG_COUNT)
        return -OAL_EFAIL;
    hi_bus->msg[msg].msg_rx = cb;
    hi_bus->msg[msg].data = data;
    return OAL_SUCC;
}


oal_void hcc_bus_message_unregister(hcc_bus *hi_bus,
                                                            oal_uint8 msg)
{
    if(NULL == hi_bus || msg >= D2H_MSG_COUNT)
        return;
    hi_bus->msg[msg].msg_rx = NULL;
    hi_bus->msg[msg].msg_rx = NULL;
}



 oal_int32 hcc_bus_transfer_rx_register(hcc_bus *hi_bus, oal_void* data,hcc_bus_data_rx rx)
{
    if(OAL_WARN_ON(!hi_bus))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "bus is null, register rx bus fail\n");
        return OAL_FAIL;
    }
    /*FIXME? add lock later.*/
    hi_bus->bus_ops.rx = rx;
    hi_bus->bus_ops_data = data;
    return OAL_SUCC;
}


oal_void hcc_bus_transfer_rx_unregister(hcc_bus *hi_bus)
{
    /*FIXME? add lock later.*/
    hi_bus->bus_ops.rx = NULL;
    hi_bus->bus_ops_data = NULL;
}

oal_int32 hcc_transfer_rx_register(struct hcc_handler* hcc, oal_void* data,hcc_bus_data_rx rx)
{
    oal_int32 flag = 0;
    oal_int32 ret = OAL_SUCC;
    oal_uint             irqsave;
    oal_dlist_head_stru  *pst_entry;
    hcc_bus*             pst_bus_tmp;
    oal_spin_lock_irq_save(&g_hcc_bus_res_lock, &irqsave);
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &g_hcc_bus_res_hdr)
    {
        pst_bus_tmp = OAL_DLIST_GET_ENTRY(pst_entry, hcc_bus, list);
        if(pst_bus_tmp->bus_dev == hcc->bus_dev)
        {
            /*注册在当前设备上的所有BUS 都需要注册*/
            ret = hcc_bus_transfer_rx_register(pst_bus_tmp, data, rx);
            if(OAL_SUCC != ret)
            {
                oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);
                oal_print_hi11xx_log(HI11XX_LOG_ERR, "register rx bus %d failed, ret=%d", pst_bus_tmp->bus_id, ret);
                return ret;
            }
            flag++;
        }

    }
    oal_spin_unlock_irq_restore(&g_hcc_bus_res_lock, &irqsave);

    if(flag)
        return OAL_SUCC;
    else
        return -OAL_ENODEV;
}

oal_uint32 hcc_get_max_trans_size(struct hcc_handler* hcc)
{
    if(OAL_WARN_ON(NULL == hcc))
    {
        return 0;
    }

    if(OAL_WARN_ON(NULL == hcc->bus_dev))
    {
        return 0;
    }

    return hcc_bus_get_max_trans_size(hcc->bus_dev->cur_bus);
}


struct task_struct *oal_thread_create_etc(int (*threadfn)(void *data),
                                        void                    *data,
                                        struct semaphore        *sema_sync,
                                        const char*               namefmt,
                                        oal_uint32                   policy,
                                        oal_int32                    prio,
                                        oal_int32                    cpuid)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    int ret = 0;
    struct task_struct      *tsk;
    struct sched_param       param;

    /* create thread for gpio rx data in interrupt handler*/
    if(sema_sync)
        sema_init(sema_sync, 0);

    tsk = kthread_create(threadfn, data, "%s",namefmt);
    if (OAL_IS_ERR_OR_NULL(tsk))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "failed to run thread:%s", namefmt);
        return NULL;
    }

    /* set thread priority and schedule policy */
    param.sched_priority = prio;
    ret = sched_setscheduler(tsk, policy, &param);
    if(OAL_UNLIKELY(ret))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "%s sched_setscheduler failed! ret =%d, prio=%d",namefmt, ret, prio);
    }
    if(cpuid >= 0)
    {
#if 0
        /*set cpu bind, can't sure it effect*/
        ret_bind    = set_cpus_allowed_ptr(tsk, &cpumask_of_cpu(cpuid));
        if (0 == ret_bind)
        {
            tsk->flags |= PF_NO_SETAFFINITY;
            return tsk;
        }
        else
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "thread %s can't bind cpuid %d, error code is %d",namefmt, cpuid, ret_bind);
            return NULL;
        }
#endif

        oal_kthread_bind(tsk,cpuid);
    }
    else
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "did not bind cpu...");
    }
    wake_up_process(tsk);
    return tsk;
#else
    return NULL;
#endif
}

/*lint -e19*/
oal_module_symbol(oal_thread_create_etc);
/*lint +e19*/


void oal_thread_stop_etc( struct task_struct      *tsk,
                          struct semaphore        *sema_sync)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    /*sema_sync can be NULL*/
    /* create thread for gpio rx data in interrupt handler*/
    send_sig(SIGTERM, tsk, 1);
    kthread_stop(tsk);
#endif
}
/*lint -e19*/
oal_module_symbol(oal_thread_stop_etc);
/*lint +e19*/

void oal_set_thread_property_etc(struct task_struct *p,int policy,
                                        const struct sched_param *param,
                                        long nice)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (OAL_WARN_ON(!p) )
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "param set thread scheduler policy fail task_struct is null");
        return;
    };

     if (!param)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "param set thread scheduler policy fail, param in null");
        return;
    };

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "set thread scheduler policy %d", policy);

    if(sched_setscheduler(p, policy, (struct sched_param *)param))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "[Error]set scheduler failed! %d", policy);
    }

    if(policy != SCHED_FIFO && policy != SCHED_RR)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "set thread scheduler nice %ld", nice);
        set_user_nice(p, nice);
    }
#endif
}

/*lint -e19*/
oal_module_symbol(oal_set_thread_property_etc);
/*lint +e19*/

/*Try to dump device mem, controlled by flag sdio_dump_mem_flag*/
oal_void hcc_bus_try_to_dump_device_mem(hcc_bus *hi_bus, oal_int32 is_sync)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return;
    }

    if(!hcc_bus_dump_mem_check(hi_bus))
        return;

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "Try to dump device mem!");
    oal_trigger_bus_exception(hi_bus, is_sync);
}
/*lint -e19*/
oal_module_symbol(hcc_bus_try_to_dump_device_mem);
oal_module_symbol(hcc_bus_transfer_rx_register);
/*lint +e19*/
/*lint +e801*/
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif



