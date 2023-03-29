

/*****************************************************************************
  1 Header File Including
*****************************************************************************/
#define  HI11XX_LOG_MODULE_NAME "[WLAN_PM]"
#define  HI11XX_LOG_MODULE_NAME_VAR wlan_pm_loglevel
#include <linux/module.h>   /* kernel module definitions */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/kobject.h>
#include <linux/irq.h>

#ifdef CONFIG_HISI_IDLE_SLEEP
#include <linux/hisi/hisi_idle_sleep.h>
#endif
#include <linux/mmc/sdio.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/host.h>
#include <linux/gpio.h>
#include <linux/pm_wakeup.h>

#include "oal_sdio.h"
#include "oal_sdio_comm.h"
#include "oal_sdio_host_if.h"
#include "oal_hcc_bus.h"

#include "plat_type.h"
#include "plat_debug.h"
#include "board.h"
#include "plat_pm_wlan.h"
#include "plat_pm.h"

#include "oal_hcc_host_if.h"
#include "oam_ext_if.h"
#include "bfgx_exception_rst.h"

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif //#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_PLAT_PM_WLAN_C


/*****************************************************************************
  2 Global Variable Definition
*****************************************************************************/

struct wlan_pm_s* gpst_wlan_pm_info_etc = OAL_PTR_NULL;

pm_callback_stru     gst_wlan_pm_callback_etc =
{
    .pm_wakeup_dev  = wlan_pm_wakeup_dev_etc,
    .pm_state_get   = wlan_pm_state_get_etc,
    .pm_wakeup_host = wlan_pm_wakeup_host_etc,
    .pm_feed_wdg    = wlan_pm_feed_wdg_etc,
    .pm_wakeup_dev_ack    = wlan_pm_wakeup_dev_ack_etc,
    .pm_disable     = wlan_pm_disable_check_wakeup_etc,

};

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
static RAW_NOTIFIER_HEAD(wifi_pm_chain);
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_uint8 g_uc_custom_cali_done_etc = OAL_FALSE;
#endif
oal_bool_enum g_wlan_pm_switch_etc = OAL_TRUE;
oal_uint8 g_wlan_device_pm_switch = OAL_TRUE; //device 低功耗开关
oal_uint8 g_wlan_ps_mode = 1;
oal_uint8 g_wlan_fast_ps_mode_dyn_ctl = 0;    //app layer dynamic ctrl enable
oal_uint8 g_wlan_fast_check_cnt = 1;

#ifdef _PRE_WLAN_RF_AUTOCALI
oal_uint8 g_uc_autocali_switch = OAL_FALSE;
EXPORT_SYMBOL_GPL(g_uc_autocali_switch);
#endif

#ifdef _PRE_WLAN_DOWNLOAD_PM
oal_uint16 g_us_download_rate_limit_pps_etc = 0;
EXPORT_SYMBOL_GPL(g_us_download_rate_limit_pps_etc);
#endif

EXPORT_SYMBOL_GPL(g_wlan_pm_switch_etc);
EXPORT_SYMBOL_GPL(g_wlan_device_pm_switch);
EXPORT_SYMBOL_GPL(g_wlan_ps_mode);
EXPORT_SYMBOL_GPL(g_wlan_fast_check_cnt);
EXPORT_SYMBOL_GPL(g_wlan_fast_ps_mode_dyn_ctl);

/*30000ms/100ms = 300 cnt*/
oal_uint32 wlan_sleep_request_forbid_limit = (30000)/(WLAN_SLEEP_TIMER_PERIOD*WLAN_SLEEP_DEFAULT_CHECK_CNT);


#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
EXPORT_SYMBOL_GPL(g_uc_custom_cali_done_etc);
#endif


extern  int32 wlan_power_on_etc(void);
extern  int32 wlan_power_off_etc(void);
void wlan_pm_wakeup_work_etc(oal_work_stru *pst_worker);
void wlan_pm_sleep_work_etc(oal_work_stru *pst_worker);
void wlan_pm_freq_adjust_work_etc(oal_work_stru *pst_worker);
void wlan_pm_wdg_timeout_etc(struct wlan_pm_s *pm_data);
int32 wlan_pm_wakeup_done_callback_etc(void *data);
int32 wlan_pm_close_done_callback_etc(void *data);
int32 wlan_pm_open_bcpu_done_callback_etc(void *data);
int32 wlan_pm_close_bcpu_done_callback_etc(void *data);
int32 wlan_pm_halt_bcpu_done_callback_etc(void *data);
int32 wlan_pm_stop_wdg_etc(struct wlan_pm_s *pst_wlan_pm_info);
oal_int wlan_pm_work_submit_etc(struct wlan_pm_s    *pst_wlan_pm, oal_work_stru* pst_worker);
void wlan_pm_info_clean_etc(void);
void wlan_pm_deepsleep_delay_timeout(struct wlan_pm_s *pm_data);
int32 wlan_pm_stop_deepsleep_delay_timer(struct wlan_pm_s *pm_data);

extern oal_atomic g_wakeup_dev_wait_ack_etc;

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_int32 wlan_pm_register_notifier(struct notifier_block *nb)
{
    return raw_notifier_chain_register(&wifi_pm_chain, nb);
}

oal_void wlan_pm_unregister_notifier(struct notifier_block *nb)
{
    raw_notifier_chain_unregister(&wifi_pm_chain, nb);
}
#endif
#ifdef CONFIG_HUAWEI_DSM

static struct dsm_dev dsm_wifi = {
    .name = "dsm_wifi",
    .device_name = NULL,
    .ic_name = NULL,
    .module_name = NULL,
    .fops = NULL,
    .buff_size = 1024,
};

struct dsm_client *hw_1103_dsm_client = NULL;

void hw_1103_register_wifi_dsm_client(void)
{
    if(NULL == hw_1103_dsm_client) {
        hw_1103_dsm_client = dsm_register_client(&dsm_wifi);
    }
}
void hw_1103_unregister_wifi_dsm_client(void)
{
   if(NULL != hw_1103_dsm_client) {
        dsm_unregister_client(hw_1103_dsm_client, &dsm_wifi);
        hw_1103_dsm_client = NULL;
   }
}
#define LOG_BUF_SIZE 512
void hw_1103_dsm_client_notify(int dsm_id, const char *fmt, ...)
{
    char buf[LOG_BUF_SIZE] = {0};
    va_list ap;

    va_start(ap, fmt);
    if(hw_1103_dsm_client && !dsm_client_ocuppy(hw_1103_dsm_client)) {
        if(fmt) {
            vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
            dsm_client_record(hw_1103_dsm_client, buf);
        }
        dsm_client_notify(hw_1103_dsm_client, dsm_id);
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "wifi dsm_client_notify success");
    } else {
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "wifi dsm_client_notify failed");
    }
    va_end(ap);
}
EXPORT_SYMBOL(hw_1103_dsm_client_notify);
#endif

struct wlan_pm_s*  wlan_pm_get_drv_etc(oal_void)
{
    return gpst_wlan_pm_info_etc;
}

EXPORT_SYMBOL_GPL(wlan_pm_get_drv_etc);


oal_int32 wlan_pm_sleep_request_etc(struct wlan_pm_s    *pst_wlan_pm)
{
    return hcc_bus_send_message(pst_wlan_pm->pst_bus, H2D_MSG_SLEEP_REQ);
}


oal_int32 wlan_pm_allow_sleep_callback_etc(void *data)
{
    struct wlan_pm_s *pst_wlan_pm = (struct wlan_pm_s *)data;

    oal_print_hi11xx_log(HI11XX_LOG_DBG, "wlan_pm_allow_sleep_callback_etc");

    pst_wlan_pm->ul_sleep_stage = SLEEP_ALLOW_RCV;
    OAL_COMPLETE(&pst_wlan_pm->st_sleep_request_ack);

    return SUCCESS;

}


oal_int32 wlan_pm_disallow_sleep_callback_etc(void *data)
{
    struct wlan_pm_s *pst_wlan_pm = (struct wlan_pm_s *)data;

    if(oal_print_rate_limit(PRINT_RATE_SECOND))
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "sleep request dev disalow, device busy");
    }
    //OAM_WARNING_LOG0(0, OAM_SF_PWR, "sleep request dev disalow, device busy");

    pst_wlan_pm->ul_sleep_stage = SLEEP_DISALLOW_RCV;
    OAL_COMPLETE(&pst_wlan_pm->st_sleep_request_ack);


    return SUCCESS;

}

#ifndef  BFGX_UART_DOWNLOAD_SUPPORT
OAL_STATIC hcc_switch_action g_plat_pm_switch_action;
OAL_STATIC oal_int32 wlan_switch_action_callback(oal_uint32 dev_id, hcc_bus *old_bus, hcc_bus *new_bus, oal_void* data)
{
    struct wlan_pm_s*   pst_wlan_pm;

    if(NULL == data)
    {
        return -OAL_EINVAL;
    }

    if(HCC_CHIP_110X_DEV != dev_id)
    {
        /*ignore other wlan dev*/
        return OAL_SUCC;
    }

    pst_wlan_pm = (struct wlan_pm_s*)data;

    /*Update new bus*/
    pst_wlan_pm->pst_bus = new_bus;
    pst_wlan_pm->pst_bus->pst_pm_callback = &gst_wlan_pm_callback_etc;

    return OAL_SUCC;
}
#endif


struct wlan_pm_s*  wlan_pm_init_etc(oal_void)
{
    struct wlan_pm_s*   pst_wlan_pm;
    BOARD_INFO *        pst_board = get_hi110x_board_info_etc();

    if(OAL_PTR_NULL == pst_board)
    {
        return OAL_PTR_NULL;
    }

    pst_wlan_pm = kzalloc(sizeof(struct wlan_pm_s), GFP_KERNEL);
    if (NULL == pst_wlan_pm)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "no mem to allocate wlan_pm_data");
        return OAL_PTR_NULL;
    }

    oal_memset(pst_wlan_pm, 0, sizeof(struct wlan_pm_s));
#ifndef  BFGX_UART_DOWNLOAD_SUPPORT
    OAL_MEMZERO((oal_void*)&g_plat_pm_switch_action, OAL_SIZEOF(g_plat_pm_switch_action));
    g_plat_pm_switch_action.name = "plat_pm_wlan";
    g_plat_pm_switch_action.switch_notify = wlan_switch_action_callback;
    hcc_switch_action_register(&g_plat_pm_switch_action, (oal_void*)pst_wlan_pm);

    pst_wlan_pm->pst_bus                  = hcc_get_current_110x_bus();
    if (NULL == pst_wlan_pm->pst_bus)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc bus is NULL, failed to create wlan_pm_wq!");
        kfree(pst_wlan_pm);
        return OAL_PTR_NULL;
    }
    pst_wlan_pm->pst_bus->pst_pm_callback = &gst_wlan_pm_callback_etc;
#endif
    pst_wlan_pm->ul_wlan_pm_enable          = OAL_FALSE;
    pst_wlan_pm->ul_apmode_allow_pm_flag    = OAL_TRUE;  /* 默认允许下电 */

    /*work queue初始化*/
    pst_wlan_pm->pst_pm_wq = oal_create_singlethread_workqueue("wlan_pm_wq");
    if (!pst_wlan_pm->pst_pm_wq)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "Failed to create wlan_pm_wq!");
        kfree(pst_wlan_pm);
        return OAL_PTR_NULL;
    }
    /*register wakeup and sleep work*/
    OAL_INIT_WORK(&pst_wlan_pm->st_wakeup_work, wlan_pm_wakeup_work_etc);
    OAL_INIT_WORK(&pst_wlan_pm->st_sleep_work,  wlan_pm_sleep_work_etc);

    /*初始化芯片自检work*/
    OAL_INIT_WORK(&pst_wlan_pm->st_ram_reg_test_work,  wlan_device_mem_check_work_etc);

    /*sleep timer初始化*/
    init_timer(&pst_wlan_pm->st_watchdog_timer);
    pst_wlan_pm->st_watchdog_timer.data        = (unsigned long)pst_wlan_pm;
    pst_wlan_pm->st_watchdog_timer.function    = (void *)wlan_pm_wdg_timeout_etc;
    pst_wlan_pm->ul_wdg_timeout_cnt            = WLAN_SLEEP_DEFAULT_CHECK_CNT;
    pst_wlan_pm->ul_wdg_timeout_curr_cnt       = 0;
    pst_wlan_pm->ul_packet_cnt                 = 0;
    pst_wlan_pm->ul_packet_total_cnt           = 0;
    pst_wlan_pm->ul_packet_check_time          = 0;
    pst_wlan_pm->ul_sleep_forbid_check_time    = 0;

     /*sleep timer初始化*/
    init_timer(&pst_wlan_pm->st_deepsleep_delay_timer);
    pst_wlan_pm->st_deepsleep_delay_timer.data        = (unsigned long)pst_wlan_pm;
    pst_wlan_pm->st_deepsleep_delay_timer.function    = (void *)wlan_pm_deepsleep_delay_timeout;

    oal_wake_lock_init(&pst_wlan_pm->st_deepsleep_wakelock, "wifi_deeepsleep_delay_wakelock");

    pst_wlan_pm->ul_wlan_power_state           = POWER_STATE_SHUTDOWN;
    pst_wlan_pm->ul_wlan_dev_state             = HOST_ALLOW_TO_SLEEP;
    pst_wlan_pm->ul_sleep_stage                = SLEEP_STAGE_INIT;

    pst_wlan_pm->st_wifi_srv_handler.p_wifi_srv_get_pm_pause_func = OAL_PTR_NULL;
    pst_wlan_pm->st_wifi_srv_handler.p_wifi_srv_open_notify = OAL_PTR_NULL;
    pst_wlan_pm->st_wifi_srv_handler.p_wifi_srv_pm_state_notify = OAL_PTR_NULL;

#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
    pst_wlan_pm->ul_wkup_src_print_en = OAL_FALSE;
#endif

    gpst_wlan_pm_info_etc = pst_wlan_pm;

    OAL_INIT_COMPLETION(&pst_wlan_pm->st_open_bcpu_done);
    OAL_INIT_COMPLETION(&pst_wlan_pm->st_close_bcpu_done);
    OAL_INIT_COMPLETION(&pst_wlan_pm->st_close_done);
    OAL_INIT_COMPLETION(&pst_wlan_pm->st_wakeup_done);
    OAL_INIT_COMPLETION(&pst_wlan_pm->st_wifi_powerup_done);
    OAL_INIT_COMPLETION(&pst_wlan_pm->st_sleep_request_ack);
    OAL_INIT_COMPLETION(&pst_wlan_pm->st_halt_bcpu_done);
#ifndef  BFGX_UART_DOWNLOAD_SUPPORT
    hcc_message_register_etc(hcc_get_110x_handler(),D2H_MSG_WAKEUP_SUCC,wlan_pm_wakeup_done_callback_etc,pst_wlan_pm);
    hcc_message_register_etc(hcc_get_110x_handler(),D2H_MSG_ALLOW_SLEEP,wlan_pm_allow_sleep_callback_etc,pst_wlan_pm);
    hcc_message_register_etc(hcc_get_110x_handler(),D2H_MSG_DISALLOW_SLEEP,wlan_pm_disallow_sleep_callback_etc,pst_wlan_pm);
    hcc_message_register_etc(hcc_get_110x_handler(),D2H_MSG_POWEROFF_ACK,wlan_pm_close_done_callback_etc,pst_wlan_pm);
    hcc_message_register_etc(hcc_get_110x_handler(),D2H_MSG_OPEN_BCPU_ACK,wlan_pm_open_bcpu_done_callback_etc,pst_wlan_pm);
    hcc_message_register_etc(hcc_get_110x_handler(),D2H_MSG_CLOSE_BCPU_ACK,wlan_pm_close_bcpu_done_callback_etc,pst_wlan_pm);
    hcc_message_register_etc(hcc_get_110x_handler(),D2H_MSG_HALT_BCPU,wlan_pm_halt_bcpu_done_callback_etc,pst_wlan_pm);

   // pst_wlan_pm->pst_sdio->gpio_int_count = 0;
    pst_wlan_pm->pst_bus->data_int_count = 0;
    pst_wlan_pm->pst_bus->wakeup_int_count = 0;
#endif
#ifdef CONFIG_HUAWEI_DSM
    hw_1103_register_wifi_dsm_client();
#endif
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "wlan_pm_init_etc ok!");
    return  pst_wlan_pm;
}


oal_uint  wlan_pm_exit_etc(oal_void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    if(OAL_PTR_NULL == pst_wlan_pm)
    {
        return OAL_SUCC;
    }

    wlan_pm_stop_wdg_etc(pst_wlan_pm);

    wlan_pm_stop_deepsleep_delay_timer(pst_wlan_pm);

    hcc_bus_message_unregister(pst_wlan_pm->pst_bus,D2H_MSG_WAKEUP_SUCC);
    hcc_bus_message_unregister(pst_wlan_pm->pst_bus,D2H_MSG_WLAN_READY);
    hcc_bus_message_unregister(pst_wlan_pm->pst_bus,D2H_MSG_ALLOW_SLEEP);
    hcc_bus_message_unregister(pst_wlan_pm->pst_bus,D2H_MSG_DISALLOW_SLEEP);
    hcc_bus_message_unregister(pst_wlan_pm->pst_bus,D2H_MSG_POWEROFF_ACK);
    hcc_bus_message_unregister(pst_wlan_pm->pst_bus,D2H_MSG_OPEN_BCPU_ACK);
    hcc_bus_message_unregister(pst_wlan_pm->pst_bus,D2H_MSG_CLOSE_BCPU_ACK);
    hcc_bus_message_unregister(pst_wlan_pm->pst_bus,D2H_MSG_HALT_BCPU);
#ifndef  BFGX_UART_DOWNLOAD_SUPPORT
    hcc_switch_action_unregister(&g_plat_pm_switch_action);
#endif

    kfree(pst_wlan_pm);

    gpst_wlan_pm_info_etc = OAL_PTR_NULL;
#ifdef CONFIG_HUAWEI_DSM
    hw_1103_unregister_wifi_dsm_client();
#endif
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "wlan_pm_exit_etc ok!");

    return OAL_SUCC;

}


oal_uint32 wlan_pm_is_poweron_etc(oal_void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    //struct pm_drv_data  *pm_data = pm_get_drvdata_etc();

    if(OAL_PTR_NULL == pst_wlan_pm)
    {
        return OAL_FALSE;
    }
    //mutex_lock(&pm_data->host_mutex);

    if(POWER_STATE_OPEN == pst_wlan_pm->ul_wlan_power_state)
    {
        //mutex_unlock(&pm_data->host_mutex);
        return OAL_TRUE;
    }
    else
    {
        //mutex_unlock(&pm_data->host_mutex);
        return OAL_FALSE;
    }
}
EXPORT_SYMBOL_GPL(wlan_pm_is_poweron_etc);


struct wifi_srv_callback_handler* wlan_pm_get_wifi_srv_handler_etc(oal_void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    if(OAL_PTR_NULL == pst_wlan_pm)
    {
        return OAL_FALSE;
    }

    return &pst_wlan_pm->st_wifi_srv_handler;
}
EXPORT_SYMBOL_GPL(wlan_pm_get_wifi_srv_handler_etc);

oal_void wlan_pm_idle_sleep_vote(oal_uint8 uc_allow)
{
#ifdef CONFIG_HISI_IDLE_SLEEP
    if(ALLOW_IDLESLEEP == uc_allow)
    {
        hisi_idle_sleep_vote(ID_WIFI,0);
    }
    else
    {
        hisi_idle_sleep_vote(ID_WIFI,1);
    }
#endif
}


oal_int32 wlan_pm_open_etc(oal_void)
{
    oal_int32 ret;
    hcc_bus* pst_bus;
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    oal_print_hi11xx_log(HI11XX_LOG_DBG, "wlan_pm_open_etc enter");

    if ((NULL == pm_data)||(OAL_PTR_NULL == pst_wlan_pm))
    {
        OAM_ERROR_LOG2(0, OAM_SF_PWR, "wlan_pm_open_etc::pm_data[%p] or pst_wlan_pm[%p] is NULL!", pm_data, pst_wlan_pm);
        return OAL_FAIL;
    }

    mutex_lock(&pm_data->host_mutex);

    pst_bus = hcc_get_current_110x_bus();
    if(NULL == pst_bus)
    {
        OAM_ERROR_LOG0(0,OAM_SF_PWR, "wlan_pm_open_etc::get 110x bus failed!");
        mutex_unlock(&pm_data->host_mutex);

        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_WIFI_DRV_ERROR_POWER_ON_NON_BUS);
        return OAL_FAIL;
    }

    if(POWER_STATE_OPEN == pst_wlan_pm->ul_wlan_power_state)
    {
        mutex_unlock(&pm_data->host_mutex);
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_open_etc::aleady opened");
        return OAL_ERR_CODE_ALREADY_OPEN;
    }

    if(!pst_wlan_pm->pst_bus->st_bus_wakelock.lock_count)
    {
        /*make sure open only lock once*/
        hcc_bus_wake_lock(pst_wlan_pm->pst_bus);
        wlan_pm_idle_sleep_vote(DISALLOW_IDLESLEEP);
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_idle_sleep_vote DISALLOW::hisi_idle_sleep_vote ID_WIFI 1!");
    }

    OAM_WARNING_LOG1(0, OAM_SF_PWR, "wlan_pm_open_etc::get wakelock %lu!",pst_wlan_pm->pst_bus->st_bus_wakelock.lock_count);

    pst_wlan_pm->ul_open_cnt++;

    ret = wlan_power_on_etc();
    if(OAL_SUCC != ret)
    {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "wlan_pm_open_etc::wlan_power_on_etc fail!");
        pst_wlan_pm->ul_wlan_power_state = POWER_STATE_SHUTDOWN;
        hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
        wlan_pm_idle_sleep_vote(ALLOW_IDLESLEEP);
        mutex_unlock(&pm_data->host_mutex);
        if(OAL_EINTR != ret)
        {
            DECLARE_DFT_TRACE_KEY_INFO("wlan_power_on_fail",OAL_DFT_TRACE_FAIL);
        }
        return OAL_FAIL;
    }

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#if (defined(_PRE_PRODUCT_ID_HI110X_DEV) || defined(_PRE_PRODUCT_ID_HI110X_HOST))
    //初始化配置定制化参数
    if(OAL_SUCC != hwifi_hcc_customize_h2d_data_cfg())
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "wlan_pm_open_etc::hwifi_hcc_customize_h2d_data_cfg fail");
    }
#endif

    OAL_INIT_COMPLETION(&pst_bus->st_device_ready);

    if(OAL_PTR_NULL == g_pst_custom_process_func_etc.p_custom_cali_func)
    {
        OAM_ERROR_LOG0(0,OAM_SF_PWR, "wlan_pm_open_etc::NO g_pst_custom_process_func_etc registered");
        hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
        wlan_pm_idle_sleep_vote(ALLOW_IDLESLEEP);

        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_WIFI_DRV_ERROR_POWER_ON_NO_CUSTOM_CALL);
        mutex_unlock(&pm_data->host_mutex);
        return OAL_FAIL;
    }

    /* 如果校准下发成功则等待device ready；否则继续打开wifi */
    if (OAL_SUCC == g_pst_custom_process_func_etc.p_custom_cali_func())
    {
#ifdef _PRE_WLAN_RF_AUTOCALI
        /* 开机不执行自动化校准 */
        if ((g_uc_autocali_switch == OAL_FALSE) || (g_uc_custom_cali_done_etc == OAL_FALSE))
#endif
        {
            if(0 == oal_wait_for_completion_timeout(&pst_bus->st_device_ready, (oal_uint32)OAL_MSECS_TO_JIFFIES(HOST_WAIT_BOTTOM_INIT_TIMEOUT)))
            {
                oal_int32 loglevel;
                DECLARE_DFT_TRACE_KEY_INFO("wlan_wait_custom_cali_fail_retry", OAL_DFT_TRACE_FAIL);
                OAM_ERROR_LOG1(0,OAM_SF_PWR, "wlan_pm_open_etc::wlan_pm_wait_custom_cali timeout retry %d !!!!!!", HOST_WAIT_BOTTOM_INIT_TIMEOUT);

                hcc_print_current_trans_info(1);

                loglevel = hcc_set_all_loglevel(HI11XX_LOG_VERBOSE);

                hcc_bus_send_message(pst_bus, H2D_MSG_DEVICE_MEM_DUMP);

                hcc_sched_transfer(hcc_get_110x_handler());

                if(0 == oal_wait_for_completion_timeout(&pst_bus->st_device_ready, (oal_uint32)OAL_MSECS_TO_JIFFIES(HOST_WAIT_BOTTOM_INIT_TIMEOUT/4)))
                {
                    hcc_print_current_trans_info(1);

#ifndef  CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT
                    {
                        struct hcc_handler* hcc = hcc_get_110x_handler();
                        if(NULL != hcc && NULL != hcc->hcc_transer_info.hcc_transfer_thread_etc)
                        {
                            sched_show_task(hcc->hcc_transer_info.hcc_transfer_thread_etc);
                        }
                    }
#endif
                    hcc_set_all_loglevel(loglevel);
                    DECLARE_DFT_TRACE_KEY_INFO("wlan_wait_custom_cali_retry_fail", OAL_DFT_TRACE_FAIL);
                    OAM_ERROR_LOG1(0,OAM_SF_PWR, "wlan_pm_open_etc::wlan_pm_wait_custom_cali timeout retry failed %d !!!!!!", HOST_WAIT_BOTTOM_INIT_TIMEOUT/4);
                    CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_WIFI_DRV_ERROR_POWER_ON_CALL_TIMEOUT);
                    if(OAL_TRUE == oal_trigger_bus_exception(pm_data->pst_wlan_pm_info->pst_bus, OAL_TRUE))
                    {
                        oal_print_hi11xx_log(HI11XX_LOG_WARN, "dump device mem when cali custom failed!");
                    }
                    mutex_unlock(&pm_data->host_mutex);

                    return OAL_FAIL;
                }
                else
                {
                    /*chr统计*/
                    CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_WIFI_DRV_ERROR_POWER_ON_CALL_TIMEOUT);
                }
                hcc_set_all_loglevel(loglevel);
            }
        }
    }
#endif

    OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_open_etc::wlan_pm_open_etc SUCC!!");
    DECLARE_DFT_TRACE_KEY_INFO("wlan_open_succ",OAL_DFT_TRACE_SUCC);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    raw_notifier_call_chain(&wifi_pm_chain, WIFI_PM_POWERUP_EVENT, (oal_void*)pst_wlan_pm);/*powerup chain*/
#endif

    wlan_pm_enable_etc();

    /* WIFI开机成功后,通知业务侧 */
    if(OAL_PTR_NULL != pst_wlan_pm->st_wifi_srv_handler.p_wifi_srv_open_notify)
    {
       pst_wlan_pm->st_wifi_srv_handler.p_wifi_srv_open_notify(OAL_TRUE);
    }

    /*将timeout值恢复为默认值，并启动定时器*/
    wlan_pm_set_timeout_etc(WLAN_SLEEP_DEFAULT_CHECK_CNT);

    ret = hcc_dev_switch_enable(HCC_CHIP_110X_DEV);
    if(OAL_SUCC != ret)
    {
        DECLARE_DFT_TRACE_KEY_INFO("hcc_dev_switch_enable failed" , OAL_DFT_TRACE_FAIL);
    }

    mutex_unlock(&pm_data->host_mutex);

    return OAL_SUCC;

}
#if (defined(_PRE_PRODUCT_ID_HI110X_DEV) || defined(_PRE_PRODUCT_ID_HI110X_HOST))
oal_int32           ram_reg_test_result_etc = OAL_SUCC;
unsigned long long  ram_reg_test_time_etc = 0;
oal_int32           wlan_mem_check_mdelay=4000;
oal_int32           bfgx_mem_check_mdelay=4000;

wlan_memdump_t st_wlan_memdump_cfg = {0x60000000, 0x1000};

oal_uint32 set_wlan_mem_check_mdelay(int32 mdelay)
{
    wlan_mem_check_mdelay = mdelay;
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "set_wlan_mem_check_mdelay::set delay:%dms!!",wlan_mem_check_mdelay);
    return 0;
}
oal_uint32 set_bfgx_mem_check_mdelay(int32 mdelay)
{
    bfgx_mem_check_mdelay = mdelay;
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "bfgx_mem_check_mdelay::set delay:%dms!!",bfgx_mem_check_mdelay);
    return 0;
}

EXPORT_SYMBOL_GPL(set_wlan_mem_check_mdelay);

wlan_memdump_t* get_wlan_memdump_cfg(void)
{
    return &st_wlan_memdump_cfg;
}

oal_uint32 set_wlan_mem_check_memdump(int32 addr,int32 len)
{
    st_wlan_memdump_cfg.addr = addr;
    st_wlan_memdump_cfg.len = len;
    st_wlan_memdump_cfg.en = 1;
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "set_wlan_mem_check_memdump set ok: addr:0x%x,len:%d",addr,len);
    return 0;
}

EXPORT_SYMBOL_GPL(set_wlan_mem_check_memdump);

oal_int32 wlan_device_mem_check_etc(void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    if(OAL_WARN_ON(!pst_wlan_pm))
    {
        OAM_ERROR_LOG1(0, OAM_SF_PWR,"%s fail!:pst_wlan_pm is null \n", __FUNCTION__);
        return -OAL_FAIL;
    }

    ram_reg_test_result_etc = OAL_SUCC;
    ram_reg_test_time_etc = 0;
    hcc_bus_wake_lock(pst_wlan_pm->pst_bus);

    if(0!=wlan_pm_work_submit_etc(pst_wlan_pm,&pst_wlan_pm->st_ram_reg_test_work))
    {
        hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
        OAM_WARNING_LOG0(0, OAM_SF_PWR,"st_ram_reg_test_work submit work fail !\n");
    }

    return OAL_SUCC;

}
oal_int32 wlan_device_mem_check_result_etc(unsigned long long *time)
{
    *time = ram_reg_test_time_etc;
    return ram_reg_test_result_etc;
}
oal_void wlan_device_mem_check_work_etc(oal_work_stru *pst_worker)
{
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    mutex_lock(&pm_data->host_mutex);

    hcc_bus_disable_state(pm_data->pst_wlan_pm_info->pst_bus, OAL_BUS_STATE_ALL);
    ram_reg_test_result_etc = device_mem_check_etc(&ram_reg_test_time_etc);
    hcc_bus_enable_state(pm_data->pst_wlan_pm_info->pst_bus, OAL_BUS_STATE_ALL);

    hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);

    mutex_unlock(&pm_data->host_mutex);
}
EXPORT_SYMBOL_GPL(wlan_device_mem_check_etc);
EXPORT_SYMBOL_GPL(wlan_device_mem_check_result_etc);


#endif

EXPORT_SYMBOL_GPL(wlan_pm_open_etc);


oal_uint32 wlan_pm_close_etc(oal_void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    oal_print_hi11xx_log(HI11XX_LOG_DBG, "wlan_pm_close_etc enter");

    /*For Debug, print key_info_log*/
    if(HI1XX_ANDROID_BUILD_VARIANT_USER != hi11xx_get_android_build_variant())
    {
        if(oal_print_rate_limit(PRINT_RATE_HOUR))
        {
            /*1小时打印一次*/
            oal_dft_print_all_key_info_etc();
        }
    }

    if (NULL == pm_data)
    {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "wlan_pm_close_etc,pm_data is NULL!");
        return OAL_FAIL;
    }

    if (OAL_PTR_NULL == pst_wlan_pm)
    {
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "pst_wlan_pm is null");
        return OAL_FAIL;
    }

    if (!pst_wlan_pm->ul_apmode_allow_pm_flag)
    {
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "wlan_pm_close_etc,AP mode,do not shutdown power.");
        return OAL_ERR_CODE_FOBID_CLOSE_DEVICE;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "wlan_pm_close_etc start!!");

    mutex_lock(&pm_data->host_mutex);

    hcc_dev_switch_disable(HCC_CHIP_110X_DEV);

    pst_wlan_pm->ul_close_cnt++;

    if(POWER_STATE_SHUTDOWN == pst_wlan_pm->ul_wlan_power_state)
    {
        mutex_unlock(&pm_data->host_mutex);
        return OAL_ERR_CODE_ALREADY_CLOSE;
    }

    /* WIFI关闭前,通知业务侧 */
    if(OAL_PTR_NULL != pst_wlan_pm->st_wifi_srv_handler.p_wifi_srv_open_notify)
    {
       pst_wlan_pm->st_wifi_srv_handler.p_wifi_srv_open_notify(OAL_FALSE);
    }

    wlan_pm_disable_etc();

    wlan_pm_stop_wdg_etc(pst_wlan_pm);

    wlan_pm_stop_deepsleep_delay_timer(pst_wlan_pm);

    wlan_pm_info_clean_etc();

    /*mask rx ip data interrupt*/
    hcc_bus_rx_int_mask(hcc_get_current_110x_bus());

    if(OAL_SUCC!=wlan_power_off_etc())
    {
        OAM_ERROR_LOG0(0, OAM_SF_PWR,"wlan_power_off_etc FAIL!\n");
        mutex_unlock(&pm_data->host_mutex);
        DECLARE_DFT_TRACE_KEY_INFO("wlan_power_off_fail",OAL_DFT_TRACE_FAIL);
        return OAL_FAIL;
    }

    pst_wlan_pm->ul_wlan_power_state = POWER_STATE_SHUTDOWN;

    /*unmask rx ip data interrupt*/
    hcc_bus_rx_int_unmask(hcc_get_current_110x_bus());

    hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    raw_notifier_call_chain(&wifi_pm_chain, WIFI_PM_POWERDOWN_EVENT, (oal_void*)pst_wlan_pm);/*powerdown chain*/
#endif

    wlan_pm_idle_sleep_vote(ALLOW_IDLESLEEP);

    OAM_WARNING_LOG1(0,OAM_SF_PWR,"wlan_pm_close_etc release wakelock %lu!\n",pst_wlan_pm->pst_bus->st_bus_wakelock.lock_count);

    hcc_bus_wakelocks_release_detect(pst_wlan_pm->pst_bus);

    hcc_bus_reset_trans_info(pst_wlan_pm->pst_bus);

    mutex_unlock(&pm_data->host_mutex);

    hcc_dev_flowctrl_on_etc(hcc_get_110x_handler(), 0);

    OAM_WARNING_LOG0(0,OAM_SF_PWR,"wlan_pm_close_etc succ!\n");
    DECLARE_DFT_TRACE_KEY_INFO("wlan_close_succ",OAL_DFT_TRACE_SUCC);
    return OAL_SUCC;

}
EXPORT_SYMBOL_GPL(wlan_pm_close_etc);


oal_uint32 wlan_pm_enable_etc(oal_void)
{

    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    if(!g_wlan_pm_switch_etc)
    {
        return OAL_SUCC;
    }

    hcc_tx_transfer_lock(hcc_get_110x_handler());

    if(OAL_TRUE == pst_wlan_pm->ul_wlan_pm_enable)
    {
        OAM_WARNING_LOG0(0, OAM_SF_PWR,"wlan_pm_enable_etc already enabled!");
        hcc_tx_transfer_unlock(hcc_get_110x_handler());
        return OAL_SUCC;
    }

    pst_wlan_pm->ul_wlan_pm_enable = OAL_TRUE;

    wlan_pm_feed_wdg_etc();

    hcc_tx_transfer_unlock(hcc_get_110x_handler());

    OAM_WARNING_LOG0(0, OAM_SF_PWR,"wlan_pm_enable_etc SUCC!");

    return OAL_SUCC;
}
EXPORT_SYMBOL_GPL(wlan_pm_enable_etc);


oal_uint32 wlan_pm_disable_check_wakeup_etc(oal_int32 flag)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    hcc_tx_transfer_lock(hcc_get_110x_handler());

    if(OAL_FALSE == pst_wlan_pm->ul_wlan_pm_enable)
    {
        OAM_WARNING_LOG0(0, OAM_SF_PWR,"wlan_pm_disable_etc already disabled!");
        hcc_tx_transfer_unlock(hcc_get_110x_handler());
        return OAL_SUCC;
    }

    if(OAL_TRUE == flag)
    {
         if (OAL_SUCC != wlan_pm_wakeup_dev_etc())
         {
             OAM_WARNING_LOG0(0, OAM_SF_PWR,"pm wake up dev fail!");
         }
    }

    pst_wlan_pm->ul_wlan_pm_enable = OAL_FALSE;

    hcc_tx_transfer_unlock(hcc_get_110x_handler());

    wlan_pm_stop_wdg_etc(pst_wlan_pm);

    oal_cancel_work_sync(&pst_wlan_pm->st_wakeup_work);
    oal_cancel_work_sync(&pst_wlan_pm->st_sleep_work);

    OAM_WARNING_LOG0(0, OAM_SF_PWR,"wlan_pm_disable_etc SUCC!");

    return OAL_SUCC;

}
EXPORT_SYMBOL_GPL(wlan_pm_disable_check_wakeup_etc);

oal_uint32 wlan_pm_disable_etc(oal_void)
{
    return wlan_pm_disable_check_wakeup_etc(OAL_TRUE);
}
EXPORT_SYMBOL_GPL(wlan_pm_disable_etc);

oal_uint32 wlan_pm_statesave(oal_void)
{
    if(g_wlan_pm_switch_etc)
    {
        return wlan_pm_disable_etc();
    }
    else
    {
        return OAL_SUCC;
    }
}

EXPORT_SYMBOL_GPL(wlan_pm_statesave);

oal_uint32 wlan_pm_staterestore(oal_void)
{
    if(g_wlan_pm_switch_etc)
    {
        return wlan_pm_enable_etc();
    }
    else
    {
        return OAL_SUCC;
    }
}
EXPORT_SYMBOL_GPL(wlan_pm_staterestore);


oal_uint wlan_pm_init_dev_etc(void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();
    oal_int              ret;
    hcc_bus*            pst_bus;


    if (OAL_PTR_NULL == pst_wlan_pm)
    {
        return OAL_FAIL;
    }

    pst_bus = hcc_get_current_110x_bus();
    if(OAL_WARN_ON(NULL == pst_bus))
    {
        OAM_ERROR_LOG0(0, OAM_SF_PWR,"wlan_pm_init_etc get non bus!");

        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_WIFI_DRV_ERROR_POWER_ON_NON_BUS);
        return OAL_FAIL;
    }

    OAM_WARNING_LOG0(0, OAM_SF_PWR,"wlan_pm_init_dev_etc!\n");

    pst_wlan_pm->ul_wlan_dev_state = HOST_DISALLOW_TO_SLEEP;


    /* wait for bus wakeup */
    ret = down_timeout(&pst_bus->sr_wake_sema, 6*HZ);
    if (-ETIME == ret)
    {
        OAM_ERROR_LOG0(0, OAM_SF_PWR,"host bus controller is not ready!");
        hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
        DECLARE_DFT_TRACE_KEY_INFO("wifi_controller_wait_init_fail", OAL_DFT_TRACE_FAIL);
        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_WIFI_DRV_ERROR_POWER_ON_SDIO_NO_READY);
        return OAL_FAIL;
    }
    up(&pst_bus->sr_wake_sema);

    return (oal_uint)hcc_bus_wakeup_request(pst_wlan_pm->pst_bus);

}




oal_uint wlan_pm_wakeup_dev_etc(oal_void)
{
    oal_uint32           ul_ret;
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();
    //oal_uint             flags;
    oal_int32              ret;
    oal_uint8              uc_retry;
	oal_uint8              uc_wakeup_retry = 0;
    static oal_uint8       uc_wakeup_err_count = 0;
    hcc_bus*            pst_bus;
    ktime_t time_start, time_stop;
    oal_uint64  trans_us;

    if (OAL_PTR_NULL == pst_wlan_pm)
    {
        return OAL_FAIL;
    }

    if(OAL_FALSE == pst_wlan_pm->ul_wlan_pm_enable)
    {
        if(HOST_ALLOW_TO_SLEEP == pst_wlan_pm->ul_wlan_dev_state)
        {
            /*唤醒流程没走完不允许发送数据*/
            return OAL_EFAIL;
        }
        else
        {
            return OAL_SUCC;
        }
    }

    if(HOST_DISALLOW_TO_SLEEP == pst_wlan_pm->ul_wlan_dev_state)
    {
      return OAL_SUCC;
    }

    pst_bus = hcc_get_current_110x_bus();
    if(OAL_WARN_ON(NULL == pst_bus))
    {
        OAM_ERROR_LOG0(0, OAM_SF_PWR,"wlan_pm_wakeup_dev_etc get non bus!\n");

        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_WAKEUP_DEV);
        return OAL_FAIL;
    }

    wlan_pm_stop_deepsleep_delay_timer(pst_wlan_pm);

wakeup_again:
    time_start = ktime_get();
    hcc_bus_wake_lock(pst_wlan_pm->pst_bus);
    wlan_pm_idle_sleep_vote(DISALLOW_IDLESLEEP);

    OAL_INIT_COMPLETION(&pst_wlan_pm->st_wakeup_done);
    oal_print_hi11xx_log(HI11XX_LOG_DBG, "wait bus wakeup");

    /* wait for bus wakeup */
    ret = down_timeout(&pst_bus->sr_wake_sema, 6*HZ);
    if (-ETIME == ret)
    {
        pst_wlan_pm->ul_wakeup_fail_wait_sdio++;
        OAM_ERROR_LOG0(0, OAM_SF_PWR,"wifi controller is not ready!");
        DECLARE_DFT_TRACE_KEY_INFO("wifi_controller_wait_fail",OAL_DFT_TRACE_FAIL);
        hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
        return OAL_FAIL;
    }
    up(&pst_bus->sr_wake_sema);

    if(HCC_BUS_PCIE == pst_wlan_pm->pst_bus->bus_type)
    {
         /*依赖回来的GPIO 做唤醒，此时回来的消息PCIE 还不确定是否已经唤醒，
           PCIE通道不可用*/
        oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_wlan_pm->pst_bus),OAL_FALSE);
        oal_atomic_set(&g_wakeup_dev_wait_ack_etc,1);
        oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_wlan_pm->pst_bus),OAL_TRUE);
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "wifi wakeup cmd send,wakelock cnt %lu",pst_wlan_pm->pst_bus->st_bus_wakelock.lock_count);
        ret = hcc_bus_wakeup_request(pst_wlan_pm->pst_bus);
        if(OAL_SUCC != ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "wakeup request failed ret=%d", ret);
            DECLARE_DFT_TRACE_KEY_INFO("wifi wakeup cmd send fail",OAL_DFT_TRACE_FAIL);
            if(HI1XX_ANDROID_BUILD_VARIANT_USER == hi11xx_get_android_build_variant())
            {
                oal_print_hi11xx_log(HI11XX_LOG_INFO, "dump when dfr");
            }
            else
            {
                if(oal_print_rate_limit(30*PRINT_RATE_SECOND))
                {
                    ssi_dump_device_regs(SSI_MODULE_MASK_COMM | SSI_MODULE_MASK_BCTRL |SSI_MODULE_MASK_WCTRL);
                }
            }
            hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
            goto wakeup_fail;
        }

        pst_wlan_pm->ul_wlan_dev_state = HOST_DISALLOW_TO_SLEEP;
    }
    else if(HCC_BUS_SDIO == pst_wlan_pm->pst_bus->bus_type)
    {
#ifdef _PRE_PLAT_FEATURE_HI110X_SDIO_GPIO_WAKE
        /*use gpio to wakeup sdio device
          1.触发上升沿
          2.sdio wakeup 寄存器写0,写0会取消sdio mem pg功能*/
        for(uc_retry = 0; uc_retry < WLAN_SDIO_MSG_RETRY_NUM; uc_retry++)
        {
            OAL_INIT_COMPLETION(&pst_wlan_pm->st_wakeup_done);
            oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_wlan_pm->pst_bus),OAL_FALSE);
            oal_atomic_set(&g_wakeup_dev_wait_ack_etc,1);
            oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_wlan_pm->pst_bus),OAL_TRUE);
            board_host_wakeup_dev_set(0);/*wakeup dev*/
            oal_udelay(100);
            board_host_wakeup_dev_set(1);/*wakeup dev*/
            ul_ret =  oal_wait_for_completion_timeout(&pst_wlan_pm->st_wakeup_done, (oal_uint32)OAL_MSECS_TO_JIFFIES(WLAN_WAKUP_MSG_WAIT_TIMEOUT));
            if(0 != ul_ret)
            {
                /*sdio gpio wakeup dev sucess*/
                DECLARE_DFT_TRACE_KEY_INFO("gpio_wakeup_sdio_succ", OAL_DFT_TRACE_SUCC);
                break;
            }
        }

        if(uc_retry == WLAN_SDIO_MSG_RETRY_NUM)
        {
            OAM_ERROR_LOG1(0, OAM_SF_PWR,"oal_sdio_gpio_wakeup_dev retry %d failed",uc_retry);
            DECLARE_DFT_TRACE_KEY_INFO("oal_sdio_gpio_wakeup_dev final fail",OAL_DFT_TRACE_EXCEP);
            if(HI1XX_ANDROID_BUILD_VARIANT_USER == hi11xx_get_android_build_variant())
            {
                oal_print_hi11xx_log(HI11XX_LOG_INFO, "dump when dfr");
            }
            else
            {
                if(oal_print_rate_limit(30*PRINT_RATE_SECOND))
                {
                    ssi_dump_device_regs(SSI_MODULE_MASK_COMM | SSI_MODULE_MASK_BCTRL |SSI_MODULE_MASK_WCTRL);
                }
            }
            hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
            goto wakeup_fail;
        }

        hcc_bus_enable_state(pst_wlan_pm->pst_bus, OAL_BUS_STATE_ALL);

        OAL_INIT_COMPLETION(&pst_wlan_pm->st_wakeup_done);
#endif
         /* set sdio register */
        oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_wlan_pm->pst_bus),OAL_FALSE);
       // oal_spin_lock_irq_save(&pst_wlan_pm->pst_sdio->st_irq_lock, &flags);

        oal_print_hi11xx_log(HI11XX_LOG_INFO, "wifi wakeup cmd send,wakelock cnt %lu",pst_wlan_pm->pst_bus->st_bus_wakelock.lock_count);
        ret = hcc_bus_wakeup_request(pst_wlan_pm->pst_bus);
        if(OAL_SUCC != ret)
        {
            DECLARE_DFT_TRACE_KEY_INFO("wifi wakeup cmd send fail",OAL_DFT_TRACE_FAIL);
            for(uc_retry = 0;uc_retry<WLAN_SDIO_MSG_RETRY_NUM;uc_retry++)
            {
                msleep(10);
                ret = hcc_bus_wakeup_request(pst_wlan_pm->pst_bus);
                if(OAL_SUCC == ret)
                {
                    break;
                }
                if(HI1XX_ANDROID_BUILD_VARIANT_USER == hi11xx_get_android_build_variant())
                {
                    oal_print_hi11xx_log(HI11XX_LOG_INFO, "dump when dfr");
                }
                else
                {
                    if(oal_print_rate_limit(30*PRINT_RATE_SECOND))
                    {
                        ssi_dump_device_regs(SSI_MODULE_MASK_COMM | SSI_MODULE_MASK_BCTRL |SSI_MODULE_MASK_WCTRL);
                    }
                }
                OAM_ERROR_LOG2(0, OAM_SF_PWR,"oal_wifi_wakeup_dev retry %d ret = %d",uc_retry,ret);
                DECLARE_DFT_TRACE_KEY_INFO("wifi wakeup cmd send retry fail",OAL_DFT_TRACE_FAIL);
            }

            /*after max retry still fail,log error*/
            if(OAL_SUCC != ret)
            {
                pst_wlan_pm->ul_wakeup_fail_set_reg++;
                OAM_ERROR_LOG1(0, OAM_SF_PWR,"oal_wifi_wakeup_dev Fail ret = %d",ret);
                DECLARE_DFT_TRACE_KEY_INFO("oal_wifi_wakeup_dev final fail",OAL_DFT_TRACE_EXCEP);
                oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_wlan_pm->pst_bus),OAL_TRUE);
                hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
                //ssi_save_device_regs();
                goto wakeup_fail;
            }

        }

        oal_atomic_set(&g_wakeup_dev_wait_ack_etc,1);

        pst_wlan_pm->ul_wlan_dev_state = HOST_DISALLOW_TO_SLEEP;
        oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_wlan_pm->pst_bus),OAL_TRUE);
        up(&pst_wlan_pm->pst_bus->rx_sema);
    }
    else
    {
        DECLARE_DFT_TRACE_KEY_INFO("oal_wifi_wakeup_dev final fail",OAL_DFT_TRACE_EXCEP);
        hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
        goto wakeup_fail;
    }

#ifdef PLATFORM_DEBUG_ENABLE
    if (!is_dfr_test_en(WIFI_WKUP_FAULT))
    {
        OAM_ERROR_LOG0(0, OAM_SF_PWR,"[dfr test] trigger wkup fail!\n");
        uc_wakeup_err_count = WLAN_WAKEUP_FAIL_MAX_TIMES;
        hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
        goto wakeup_fail;
    }
#endif

    ul_ret =  oal_wait_for_completion_timeout(&pst_wlan_pm->st_wakeup_done, (oal_uint32)OAL_MSECS_TO_JIFFIES(WLAN_WAKUP_MSG_WAIT_TIMEOUT));
    if(0 == ul_ret)
    {
        oal_int32 sleep_state = hcc_bus_get_sleep_state(pst_wlan_pm->pst_bus);
        if((DISALLOW_TO_SLEEP_VALUE == sleep_state) || (sleep_state < 0))
        {
            if(OAL_UNLIKELY(sleep_state < 0))
            {
                OAM_ERROR_LOG1(0, OAM_SF_PWR,"get state failed, sleep_state=%d", sleep_state);
            }

            pst_wlan_pm->ul_wakeup_fail_timeout++;
            OAM_WARNING_LOG0(0, OAM_SF_PWR,"oal_wifi_wakeup_dev SUCC to set 0xf0 = 0");
            hcc_bus_sleep_request(pst_wlan_pm->pst_bus);
            pst_wlan_pm->ul_wlan_dev_state = HOST_ALLOW_TO_SLEEP;
            hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
            if(0==uc_wakeup_retry)
            {
                OAM_WARNING_LOG1(0, OAM_SF_PWR,"wlan_pm_wakeup_dev_etc wait device complete fail,wait time %d ms!,try again",WLAN_WAKUP_MSG_WAIT_TIMEOUT);
                uc_wakeup_retry++;
                goto wakeup_again;
            }
            else
            {
                OAM_ERROR_LOG2(0, OAM_SF_PWR,"wlan_pm_wakeup_dev_etc [%d]wait device complete fail,wait time %d ms!",uc_wakeup_err_count,WLAN_WAKUP_MSG_WAIT_TIMEOUT);
                oal_print_hi11xx_log(HI11XX_LOG_INFO, KERN_ERR"wlan_pm_wakeup_dev_etc [%d]wait device complete fail,wait time %d ms!",uc_wakeup_err_count,WLAN_WAKUP_MSG_WAIT_TIMEOUT);
                goto wakeup_fail;
            }
        }
        else
        {
            pst_wlan_pm->ul_wakeup_fail_set_reg++;
            OAM_ERROR_LOG0(0, OAM_SF_PWR,"wakeup_dev Fail to set 0xf0 = 0");
            oal_print_hi11xx_log(HI11XX_LOG_INFO, KERN_ERR"wakeup_dev Fail to set 0xf0 = 0");
            pst_wlan_pm->ul_wlan_dev_state = HOST_ALLOW_TO_SLEEP;
            hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
            goto wakeup_fail;
        }
    }

    pst_wlan_pm->ul_wakeup_succ++;
    DECLARE_DFT_TRACE_KEY_INFO("wlan_wakeup_succ",OAL_DFT_TRACE_SUCC);
    //oal_print_hi11xx_log(HI11XX_LOG_DBG, "wlan_pm_wakeup_dev_etc succ, wakelock cnt %d", pst_wlan_pm->pst_sdio->ul_wklock_cnt);

    pst_wlan_pm->ul_wdg_timeout_curr_cnt = 0;
    pst_wlan_pm->ul_packet_cnt           = 0;
    pst_wlan_pm->ul_packet_check_time = jiffies + msecs_to_jiffies(WLAN_PACKET_CHECK_TIME);
    pst_wlan_pm->ul_packet_total_cnt = 0;

    /* HOST WIFI退出低功耗,通知业务侧开启定时器 */
    if(OAL_PTR_NULL != pst_wlan_pm->st_wifi_srv_handler.p_wifi_srv_pm_state_notify)
    {
       pst_wlan_pm->st_wifi_srv_handler.p_wifi_srv_pm_state_notify(OAL_TRUE);
    }

    wlan_pm_feed_wdg_etc();

    uc_wakeup_err_count = 0;

    oal_usleep_range(500,510);

    hcc_bus_wakeup_complete(pst_wlan_pm->pst_bus);

    pst_wlan_pm->ul_sleep_fail_forbid_cnt = 0;

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    raw_notifier_call_chain(&wifi_pm_chain, WIFI_PM_WAKEUP_EVENT, (oal_void*)pst_wlan_pm);/*wakeup chain*/
#endif

    time_stop = ktime_get();
    trans_us = (oal_uint64)ktime_to_us(ktime_sub(time_stop, time_start));
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "wakeup dev succ, cost %llu us", trans_us);
    return OAL_SUCC;
wakeup_fail:
    DECLARE_DFT_TRACE_KEY_INFO("wlan_wakeup_fail",OAL_DFT_TRACE_FAIL);
    uc_wakeup_err_count++;

    /* pm唤醒失败超出门限，启动dfr流程 */
    if (WLAN_WAKEUP_FAIL_MAX_TIMES < uc_wakeup_err_count)
    {
        OAM_ERROR_LOG1(0, OAM_SF_PWR,"Now ready to enter DFR process after [%d]times wlan_wakeup_fail!", uc_wakeup_err_count);
        uc_wakeup_err_count = 0;
        hcc_bus_exception_submit(pst_wlan_pm->pst_bus, WIFI_WAKEUP_FAIL);
        //wifi_exception_work_submit_etc(BFGX_TIMER_TIMEOUT);
    }
    CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_WAKEUP_DEV);

    return OAL_FAIL;

}

oal_void wlan_pm_wakeup_dev_ack_etc(oal_void)
{
    struct wlan_pm_s    *pst_wlan_pm;

    if(oal_atomic_read(&g_wakeup_dev_wait_ack_etc))
    {
        pst_wlan_pm = wlan_pm_get_drv_etc();

        if (OAL_PTR_NULL == pst_wlan_pm)
        {
            return ;
        }

        pst_wlan_pm->ul_wakeup_dev_ack++;

        OAL_COMPLETE(&pst_wlan_pm->st_wakeup_done);


        oal_atomic_set(&g_wakeup_dev_wait_ack_etc,0);
        //OAL_WARN_ON(1);
    }

    return;

}


oal_uint  wlan_pm_open_bcpu_etc(oal_void)
{
#define RETRY_TIMES (3)
    oal_uint32           i;
    oal_int32            ret = OAL_FAIL;
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();
    oal_int32           ul_ret;

    if(OAL_PTR_NULL == pst_wlan_pm)
    {
        return OAL_FAIL;
    }

     /*通过sdio配置命令，解复位BCPU*/
    OAM_WARNING_LOG0(0, OAM_SF_PWR,"open BCPU");

    hcc_tx_transfer_lock(hcc_get_110x_handler());

    for (i = 0; i < RETRY_TIMES; i++)
    {
        ret = wlan_pm_wakeup_dev_etc();
        if (OAL_SUCC == ret)
        {
            break;
        }
    }

    if(OAL_SUCC != ret)
    {
        OAM_ERROR_LOG0(0, OAM_SF_PWR,"wlan_pm_wakeup_dev_etc fail!");
        hcc_tx_transfer_unlock(hcc_get_110x_handler());
        return OAL_FAIL;
    }

    OAM_WARNING_LOG1(0, OAM_SF_PWR,"wlan_pm_wakeup_dev_etc succ, retry times [%d]", i);

    OAL_INIT_COMPLETION(&pst_wlan_pm->st_open_bcpu_done);

    ret = hcc_bus_send_message(pst_wlan_pm->pst_bus, H2D_MSG_RESET_BCPU);
    if(OAL_SUCC == ret)
    {
        /*等待device执行命令*/
        up(&pst_wlan_pm->pst_bus->rx_sema);
        ul_ret =  oal_wait_for_completion_timeout(&pst_wlan_pm->st_open_bcpu_done, (oal_uint32)OAL_MSECS_TO_JIFFIES(WLAN_OPEN_BCPU_WAIT_TIMEOUT));
        if(0 == ul_ret)
        {
            OAM_ERROR_LOG0(0, OAM_SF_PWR,"wlan_pm_open_bcpu_etc wait device ACK timeout !");
            hcc_tx_transfer_unlock(hcc_get_110x_handler());
            return OAL_FAIL;
        }

        hcc_tx_transfer_unlock(hcc_get_110x_handler());
        return  OAL_SUCC;
    }
    else
    {
        OAM_ERROR_LOG1(0, OAM_SF_PWR,"fail to send H2D_MSG_RESET_BCPU, ret=%d", ret);
        hcc_tx_transfer_unlock(hcc_get_110x_handler());
        return  OAL_FAIL;
    }
}


oal_uint wlan_pm_wakeup_host_etc(void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    if(OAL_WARN_ON(!pst_wlan_pm))
    {
        OAM_ERROR_LOG1(0, OAM_SF_PWR,"%s fail!:pst_wlan_pm is null \n", __FUNCTION__);
        return -OAL_FAIL;
    }

    hcc_bus_wake_lock(pst_wlan_pm->pst_bus);
    wlan_pm_idle_sleep_vote(DISALLOW_IDLESLEEP);
    OAM_INFO_LOG1(0, OAM_SF_PWR,"wlan_pm_wakeup_host_etc get wakelock %lu!\n",pst_wlan_pm->pst_bus->st_bus_wakelock.lock_count);

    if(0!=wlan_pm_work_submit_etc(pst_wlan_pm,&pst_wlan_pm->st_wakeup_work))
    {
        pst_wlan_pm->ul_wakeup_fail_submit_work++;
        hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
        //OAM_WARNING_LOG1(0, OAM_SF_PWR,"wlan_pm_wakeup_host_etc submit work fail, release wakelock %lu!\n",pst_wlan_pm->pst_bus->st_bus_wakelock.lock_count);
    }
    else
    {
        pst_wlan_pm->ul_wakeup_succ_work_submit++;
    }

    return OAL_SUCC;

}



oal_int wlan_pm_work_submit_etc(struct wlan_pm_s    *pst_wlan_pm, oal_work_stru* pst_worker)
{
    oal_int  i_ret    = 0;

    if(oal_work_is_busy(pst_worker))
    {
        /*If comm worker is processing,
          we need't submit again*/
        i_ret = -OAL_EBUSY;
        goto done;
    }
    else
    {
        OAM_INFO_LOG1(0, OAM_SF_PWR,"WiFi %pF Worker Submit\n",(void*)pst_worker->func);
        if(false==queue_work(pst_wlan_pm->pst_pm_wq, pst_worker))
        {
           i_ret = -OAL_EFAIL;
        }
    }
done:
    return i_ret;
}


void wlan_pm_wakeup_work_etc(oal_work_stru *pst_worker)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();
    oal_uint l_ret;

    OAM_INFO_LOG0(0, OAM_SF_PWR,"wlan_pm_wakeup_work_etc start!\n");

    hcc_tx_transfer_lock(hcc_get_110x_handler());

    l_ret = wlan_pm_wakeup_dev_etc();
    if(OAL_UNLIKELY(OAL_SUCC!=l_ret))
    {
        DECLARE_DFT_TRACE_KEY_INFO("wlan_wakeup_fail",OAL_DFT_TRACE_FAIL);
    }

    hcc_tx_transfer_unlock(hcc_get_110x_handler());

    /*match for the work submit*/
    hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
    DECLARE_DFT_TRACE_KEY_INFO("wlan_d2h_wakeup_succ",OAL_DFT_TRACE_SUCC);
    OAM_INFO_LOG1(0, OAM_SF_PWR,"wlan_pm_wakeup_work_etc release wakelock %lu!\n",pst_wlan_pm->pst_bus->st_bus_wakelock.lock_count);

    return;

}



int32 wlan_pm_wakeup_done_callback_etc(void *data)
{
    struct wlan_pm_s *pst_wlan_pm = (struct wlan_pm_s *)data;

    OAM_INFO_LOG0(0, OAM_SF_PWR,"wlan_pm_wakeup_done_callback_etc !");

	pst_wlan_pm->ul_wakeup_done_callback++;

	wlan_pm_wakeup_dev_ack_etc();

    return SUCCESS;

}



int32 wlan_pm_close_done_callback_etc(void *data)
{
    struct wlan_pm_s *pst_wlan_pm = (struct wlan_pm_s *)data;

    OAM_WARNING_LOG0(0, OAM_SF_PWR,"wlan_pm_close_done_callback_etc !");

    /*关闭RX通道，防止SDIO RX thread继续访问SDIO*/
    hcc_bus_disable_state(hcc_get_current_110x_bus(), OAL_BUS_STATE_RX);

    pst_wlan_pm->ul_close_done_callback++;
    OAL_COMPLETE(&pst_wlan_pm->st_close_done);

    OAM_WARNING_LOG0(0, OAM_SF_PWR,"complete H2D_MSG_PM_WLAN_OFF done!");

    return SUCCESS;

}



int32 wlan_pm_open_bcpu_done_callback_etc(void *data)
{
    struct wlan_pm_s *pst_wlan_pm = (struct wlan_pm_s *)data;

    OAM_WARNING_LOG0(0, OAM_SF_PWR,"wlan_pm_open_bcpu_done_callback_etc !");

    pst_wlan_pm->ul_open_bcpu_done_callback++;
    OAL_COMPLETE(&pst_wlan_pm->st_open_bcpu_done);

    OAM_WARNING_LOG0(0, OAM_SF_PWR,"complete H2D_MSG_RESET_BCPU done!");

    return SUCCESS;
}



int32 wlan_pm_close_bcpu_done_callback_etc(void *data)
{
    struct wlan_pm_s *pst_wlan_pm = (struct wlan_pm_s *)data;

    OAM_WARNING_LOG0(0, OAM_SF_PWR,"wlan_pm_close_bcpu_done_callback_etc !");

    pst_wlan_pm->ul_close_bcpu_done_callback++;
    OAL_COMPLETE(&pst_wlan_pm->st_close_bcpu_done);

    OAM_WARNING_LOG0(0, OAM_SF_PWR,"complete H2D_MSG_PM_BCPU_OFF done!");

    return SUCCESS;
}



int32 wlan_pm_halt_bcpu_done_callback_etc(void *data)
{
    struct wlan_pm_s *pst_wlan_pm = (struct wlan_pm_s *)data;

    OAM_WARNING_LOG0(0, OAM_SF_PWR,"wlan_pm_halt_bcpu_done_callback_etc !");

    OAL_COMPLETE(&pst_wlan_pm->st_halt_bcpu_done);

    OAM_WARNING_LOG0(0, OAM_SF_PWR,"complete wlan_pm_halt_bcpu_done_callback_etc done!");

    return SUCCESS;
}

oal_uint64 old_tx, old_rx;
oal_uint64 new_tx, new_rx;

void wlan_pm_sleep_work_etc(oal_work_stru *pst_worker)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();
    //oal_uint             flags;
    oal_int32            l_ret;
    oal_uint32           ul_ret;
    oal_uint8            uc_retry;
    oal_bool_enum_uint8  en_wifi_pause_pm = OAL_FALSE;
    static oal_uint8     uc_fail_sleep_count = 0;

    hcc_tx_transfer_lock(hcc_get_110x_handler());

    if(OAL_FALSE == pst_wlan_pm->ul_wlan_pm_enable)
    {
        wlan_pm_feed_wdg_etc();
        hcc_tx_transfer_unlock(hcc_get_110x_handler());
        return ;
    }

    /*协议栈回调获取是否pause低功耗*/
    if(pst_wlan_pm->st_wifi_srv_handler.p_wifi_srv_get_pm_pause_func)
    {
       en_wifi_pause_pm = pst_wlan_pm->st_wifi_srv_handler.p_wifi_srv_get_pm_pause_func();
    }

    if(OAL_TRUE == en_wifi_pause_pm)
    {
        wlan_pm_feed_wdg_etc();
        hcc_tx_transfer_unlock(hcc_get_110x_handler());
        return;
    }

   if(HOST_ALLOW_TO_SLEEP == pst_wlan_pm->ul_wlan_dev_state)
   {
     oal_print_hi11xx_log(HI11XX_LOG_DBG, "wakeuped,ne ed not do again");
     wlan_pm_feed_wdg_etc();
     hcc_tx_transfer_unlock(hcc_get_110x_handler());
     return ;
   }

   pst_wlan_pm->ul_sleep_stage = SLEEP_REQ_SND;

   OAL_INIT_COMPLETION(&pst_wlan_pm->st_sleep_request_ack);

   l_ret = wlan_pm_sleep_request_etc(pst_wlan_pm);
   if(OAL_SUCC != l_ret)
   {
      pst_wlan_pm->ul_sleep_fail_request++;
      OAM_ERROR_LOG0(0, OAM_SF_PWR,"wlan_pm_sleep_request_etc fail !\n");
      goto fail_sleep;
   }

   oal_print_hi11xx_log(HI11XX_LOG_INFO, "sleep request send!");
   up(&pst_wlan_pm->pst_bus->rx_sema);

   ul_ret =  oal_wait_for_completion_timeout(&pst_wlan_pm->st_sleep_request_ack, (oal_uint32)OAL_MSECS_TO_JIFFIES(WLAN_SLEEP_MSG_WAIT_TIMEOUT));
   if(0 == ul_ret)
   {
       pst_wlan_pm->ul_sleep_fail_wait_timeout++;
       OAM_ERROR_LOG0(0, OAM_SF_PWR,"wlan_pm_sleep_work_etc wait completion fail !\n");
       goto fail_sleep;
   }

   if(SLEEP_ALLOW_RCV == pst_wlan_pm->ul_sleep_stage)
   {

       /*check host*/
       l_ret = hcc_bus_sleep_request_host(pst_wlan_pm->pst_bus);
       if(OAL_SUCC != l_ret)
       {
           //pst_wlan_pm->ul_sleep_fail_forbid++;
           pst_wlan_pm->ul_sleep_request_host_forbid++;
           DECLARE_DFT_TRACE_KEY_INFO("wlan_forbid_sleep_host", OAL_DFT_TRACE_SUCC);
           if(pst_wlan_pm->ul_sleep_request_host_forbid >= 10)
           {
                /*防止频繁打印*/
                if(oal_print_rate_limit(10*PRINT_RATE_SECOND))
                {
                    oal_int32 allow_print;
                    OAM_WARNING_LOG2(0, OAM_SF_PWR,"wlan_pm_sleep_work_etc host forbid sleep %ld, forbid_cnt:%u",
                                                        pst_wlan_pm->ul_sleep_stage, pst_wlan_pm->ul_sleep_request_host_forbid);
                    allow_print = oal_print_rate_limit(10*PRINT_RATE_MINUTE);
                    hcc_bus_print_trans_info(pst_wlan_pm->pst_bus, allow_print ? (HCC_PRINT_TRANS_FLAG_DEVICE_STAT|HCC_PRINT_TRANS_FLAG_DEVICE_REGS):0x0);
                }
           }
           else
           {
                /*防止频繁打印*/
                if(oal_print_rate_limit(10*PRINT_RATE_SECOND))
                {
                    OAM_WARNING_LOG2(0, OAM_SF_PWR,"wlan_pm_sleep_work_etc host forbid sleep %ld, forbid_cnt:%u",
                                                    pst_wlan_pm->ul_sleep_stage, pst_wlan_pm->ul_sleep_request_host_forbid);
                }
           }

           goto sleep_forbid;
       }
       else
       {
            pst_wlan_pm->ul_sleep_request_host_forbid = 0;
       }

       oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_wlan_pm->pst_bus),OAL_FALSE);

       pst_wlan_pm->ul_wlan_dev_state = HOST_ALLOW_TO_SLEEP;

       l_ret = hcc_bus_sleep_request(pst_wlan_pm->pst_bus);
       if(oal_print_rate_limit(PRINT_RATE_SECOND))
       {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "wifi sleep cmd send ,wakelock cnt %lu",pst_wlan_pm->pst_bus->st_bus_wakelock.lock_count);
       }
       if(OAL_SUCC!=l_ret)
       {
            for(uc_retry = 0;uc_retry<WLAN_SDIO_MSG_RETRY_NUM;uc_retry++)
            {
                msleep(10);
                l_ret = hcc_bus_sleep_request(pst_wlan_pm->pst_bus);;
                if(OAL_SUCC == l_ret)
                {
                    break;
                }
                OAM_ERROR_LOG2(0, OAM_SF_PWR,"sleep_dev retry %d ret = %d",uc_retry,l_ret);
            }

            /*after max retry still fail,log error*/
           if(OAL_SUCC!=l_ret)
           {
                pst_wlan_pm->ul_sleep_fail_set_reg++;
                DECLARE_DFT_TRACE_KEY_INFO("wlan_sleep_cmd_fail",OAL_DFT_TRACE_FAIL);
                OAM_ERROR_LOG1(0, OAM_SF_PWR,"sleep_dev Fail ret = %d\r\n",l_ret);
                pst_wlan_pm->ul_wlan_dev_state = HOST_DISALLOW_TO_SLEEP;
                oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_wlan_pm->pst_bus),OAL_TRUE);
                goto fail_sleep;
           }

       }

       oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pst_wlan_pm->pst_bus),OAL_TRUE);

       pst_wlan_pm->ul_sleep_fail_forbid_cnt = 0;
       pst_wlan_pm->ul_sleep_fail_forbid = 0;
   }
   else
   {
       pst_wlan_pm->ul_sleep_fail_forbid++;
       if(1==pst_wlan_pm->ul_sleep_fail_forbid)
       {
            pst_wlan_pm->ul_sleep_forbid_check_time = jiffies + msecs_to_jiffies(WLAN_SLEEP_FORBID_CHECK_TIME);
       }
       else if((0!=pst_wlan_pm->ul_sleep_fail_forbid)&&(time_after(jiffies, pst_wlan_pm->ul_sleep_forbid_check_time)))
       {
           /*暂时连续2分钟被forbid sleep，上报一次CHR，看大数据再决定做不做DFR*/
           CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_SLEEP_FORBID);
           pst_wlan_pm->ul_sleep_fail_forbid = 0;
           oal_print_hi11xx_log(HI11XX_LOG_INFO, "device_forbid_sleep for %d second",WLAN_SLEEP_FORBID_CHECK_TIME/1000);
           if(HI1XX_ANDROID_BUILD_VARIANT_ROOT == hi11xx_get_android_build_variant())
           {
               if(OAL_TRUE == oal_trigger_bus_exception(pst_wlan_pm->pst_bus, OAL_TRUE))
               {
                   oal_print_hi11xx_log(HI11XX_LOG_WARN, "tigger dump device mem for device_forbid_sleep %d second",WLAN_SLEEP_FORBID_CHECK_TIME/1000);
               }

           }
       }

       pst_wlan_pm->ul_sleep_fail_forbid_cnt++;
       if(pst_wlan_pm->ul_sleep_fail_forbid_cnt <= 1)
       {
            /*get hcc trans count*/
            hcc_bus_get_trans_count(pst_wlan_pm->pst_bus, &old_tx, &old_rx);
       }
       else
       {

            /*ul_sleep_fail_forbid_cnt > 1*/
            hcc_bus_get_trans_count(pst_wlan_pm->pst_bus, &new_tx, &new_rx);
            /*trans pending*/
            if(pst_wlan_pm->ul_sleep_fail_forbid_cnt >= wlan_sleep_request_forbid_limit)
            {
                /*maybe device memleak*/
                DECLARE_DFT_TRACE_KEY_INFO("wlan_forbid_sleep_print_info",OAL_DFT_TRACE_SUCC);
                OAM_WARNING_LOG2(0, OAM_SF_PWR,"wlan_pm_sleep_work_etc device forbid sleep %ld, forbid_cnt:%u try dump device mem info",
                                                    pst_wlan_pm->ul_sleep_stage, pst_wlan_pm->ul_sleep_fail_forbid_cnt);
                OAM_WARNING_LOG4(0, OAM_SF_PWR,"old[tx:%u rx:%u] new[tx:%u rx:%u]", old_tx, old_rx, new_tx, new_rx);
                pst_wlan_pm->ul_sleep_fail_forbid_cnt = 0;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
                hcc_print_current_trans_info(1);
#endif
                hcc_bus_send_message(pst_wlan_pm->pst_bus, H2D_MSG_DEVICE_MEM_DUMP);
            }
            else if(0  == (pst_wlan_pm->ul_sleep_fail_forbid_cnt % (wlan_sleep_request_forbid_limit/10)))
            {
                oal_print_hi11xx_log(HI11XX_LOG_INFO, "sleep request too many forbid %ld, device busy, forbid_cnt:%u, old[tx:%u rx:%u] new[tx:%u rx:%u]",
                                                pst_wlan_pm->ul_sleep_stage, pst_wlan_pm->ul_sleep_fail_forbid_cnt,
                                                (oal_uint32)old_tx, (oal_uint32)old_rx, (oal_uint32)new_tx, (oal_uint32)new_rx);
            }
            else
            {
                oal_print_hi11xx_log(HI11XX_LOG_DBG, "sleep request forbid %ld, device busy, forbid_cnt:%u, old[tx:%u rx:%u] new[tx:%u rx:%u]",
                                                pst_wlan_pm->ul_sleep_stage, pst_wlan_pm->ul_sleep_fail_forbid_cnt,
                                                (oal_uint32)old_tx, (oal_uint32)old_rx, (oal_uint32)new_tx, (oal_uint32)new_rx);
            }
       }

       DECLARE_DFT_TRACE_KEY_INFO("wlan_forbid_sleep",OAL_DFT_TRACE_SUCC);
       goto sleep_forbid;
   }

   pst_wlan_pm->ul_sleep_stage = SLEEP_CMD_SND;

   /*继续持锁500ms, 防止系统频繁进入退出PM*/
   oal_wake_lock(&pst_wlan_pm->st_deepsleep_wakelock);

   oal_print_hi11xx_log(HI11XX_LOG_DBG, "wlan_pm_sleep_work hold deepsleep_wakelock....%lu", pst_wlan_pm->st_deepsleep_wakelock.lock_count);
   mod_timer(&pst_wlan_pm->st_deepsleep_delay_timer, jiffies + msecs_to_jiffies(WLAN_WAKELOCK_HOLD_TIME));

   hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
   hcc_tx_transfer_unlock(hcc_get_110x_handler());
   wlan_pm_idle_sleep_vote(ALLOW_IDLESLEEP);

   /* HOST WIFI进入低功耗,通知业务侧关闭定时器 */
   if(OAL_PTR_NULL != pst_wlan_pm->st_wifi_srv_handler.p_wifi_srv_pm_state_notify)
   {
      pst_wlan_pm->st_wifi_srv_handler.p_wifi_srv_pm_state_notify(OAL_FALSE);
   }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    raw_notifier_call_chain(&wifi_pm_chain, WIFI_PM_SLEEP_EVENT, (oal_void*)pst_wlan_pm);/*sleep chain*/
#endif

   DECLARE_DFT_TRACE_KEY_INFO("wlan_sleep_ok",OAL_DFT_TRACE_SUCC);
   pst_wlan_pm->ul_sleep_succ++;
   if(0!=pst_wlan_pm->pst_bus->st_bus_wakelock.lock_count)
   {
      OAM_WARNING_LOG1(0, OAM_SF_PWR,"wlan_pm_sleep_work_etc release wakelock %lu!\n",pst_wlan_pm->pst_bus->st_bus_wakelock.lock_count);
   }

   uc_fail_sleep_count = 0;

   return;

fail_sleep:

    uc_fail_sleep_count++;
    wlan_pm_feed_wdg_etc();
    hcc_tx_transfer_unlock(hcc_get_110x_handler());

    /* 失败超出门限，启动dfr流程 */
    if (WLAN_WAKEUP_FAIL_MAX_TIMES < uc_fail_sleep_count)
    {
        OAM_ERROR_LOG1(0, OAM_SF_PWR,"Now ready to enter DFR process after [%d]times wlan_sleep_fail!", uc_fail_sleep_count);
        uc_fail_sleep_count = 0;
        wlan_pm_stop_wdg_etc(pst_wlan_pm);
        hcc_bus_exception_submit(pst_wlan_pm->pst_bus, WIFI_WAKEUP_FAIL);
        //wifi_exception_work_submit_etc(BFGX_TIMER_TIMEOUT);
    }
    CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_WAKEUP_DEV);
    return;

sleep_forbid:

    uc_fail_sleep_count = 0;
    wlan_pm_feed_wdg_etc();
    hcc_tx_transfer_unlock(hcc_get_110x_handler());
    return ;

}


void wlan_pm_freq_adjust_work_etc(oal_work_stru *pst_worker)
{

    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();


    hcc_tx_transfer_lock(hcc_get_110x_handler());

    if(OAL_FALSE == pst_wlan_pm->ul_wlan_pm_enable)
    {
        hcc_tx_transfer_unlock(hcc_get_110x_handler());
        return ;
    }


    hcc_tx_transfer_unlock(hcc_get_110x_handler());

}





oal_uint wlan_pm_state_get_etc(void)
{
    struct wlan_pm_s   *pst_wlan_pm     = wlan_pm_get_drv_etc();

    return pst_wlan_pm->ul_wlan_dev_state;
}



oal_void wlan_pm_state_set_etc(struct wlan_pm_s  *pst_wlan_pm,oal_uint ul_state)
{

    pst_wlan_pm->ul_wlan_dev_state = ul_state;

}

oal_void  wlan_pm_set_timeout_etc(oal_uint32 ul_timeout)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    if(OAL_PTR_NULL == pst_wlan_pm)
    {
        return ;
    }

    OAM_WARNING_LOG1(0, OAM_SF_PWR,"wlan_pm_set_timeout_etc[%d]",ul_timeout);

    pst_wlan_pm ->ul_wdg_timeout_cnt = ul_timeout;

    pst_wlan_pm->ul_wdg_timeout_curr_cnt = 0;

    pst_wlan_pm->ul_packet_cnt = 0;

    wlan_pm_feed_wdg_etc();

}
EXPORT_SYMBOL_GPL(wlan_pm_set_timeout_etc);

oal_void  wlan_pm_feed_wdg_etc(oal_void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    pst_wlan_pm->ul_sleep_feed_wdg_cnt++;

#ifdef _PRE_WLAN_DOWNLOAD_PM
    if (g_us_download_rate_limit_pps_etc != 0)
    {
        mod_timer(&pst_wlan_pm->st_watchdog_timer, jiffies + msecs_to_jiffies(10));
    }
    else
    {
        mod_timer(&pst_wlan_pm->st_watchdog_timer, jiffies + msecs_to_jiffies(WLAN_SLEEP_TIMER_PERIOD));
    }
#else
    mod_timer(&pst_wlan_pm->st_watchdog_timer, jiffies + msecs_to_jiffies(WLAN_SLEEP_TIMER_PERIOD));
#endif
}




int32 wlan_pm_stop_wdg_etc(struct wlan_pm_s *pst_wlan_pm_info)
{
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "wlan_pm_stop_wdg_etc");

    pst_wlan_pm_info->ul_wdg_timeout_curr_cnt = 0;
    pst_wlan_pm_info->ul_packet_cnt = 0;

    if(in_interrupt())
    {
        return del_timer(&pst_wlan_pm_info->st_watchdog_timer);
    }
    else
    {
        return del_timer_sync(&pst_wlan_pm_info->st_watchdog_timer);
    }

}




void wlan_pm_wdg_timeout_etc(struct wlan_pm_s *pm_data)
{
    if(NULL == pm_data)
    {
        return;
    }
    oal_print_hi11xx_log(HI11XX_LOG_DBG, "wlan_pm_wdg_timeout_etc....%d", pm_data->ul_wdg_timeout_curr_cnt);

    /*hcc bus switch process*/
    hcc_bus_performance_core_schedule(HCC_CHIP_110X_DEV);

    pm_data->ul_packet_cnt += g_pm_wifi_rxtx_count; //和hmac中统计收发包分离

    pm_data->ul_packet_total_cnt+= g_pm_wifi_rxtx_count;
    if(time_after(jiffies, pm_data->ul_packet_check_time))
    {
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "pkt_num:WIFI[%d]", pm_data->ul_packet_total_cnt);
        pm_data->ul_packet_check_time = jiffies + msecs_to_jiffies(WLAN_PACKET_CHECK_TIME);
    }

    g_pm_wifi_rxtx_count = 0;

    /*低功耗关闭时timer不会停*/
    if(pm_data->ul_wlan_pm_enable)
    {
        if(0 == pm_data->ul_packet_cnt)
        {
            pm_data->ul_wdg_timeout_curr_cnt++;
            if((pm_data->ul_wdg_timeout_curr_cnt >= pm_data->ul_wdg_timeout_cnt))
            {
                if(0 != wlan_pm_work_submit_etc(pm_data, &pm_data->st_sleep_work))
                {
                     OAM_WARNING_LOG0(0, OAM_SF_PWR,"wlan_pm_sleep_work_etc submit fail,work is running !\n");
                }
                else
                {
                    /*提交了sleep work后，定时器不重启，避免重复提交sleep work*/
                    pm_data->ul_sleep_work_submit++;
                    pm_data->ul_wdg_timeout_curr_cnt = 0;
                    return;
                }
            }
        }
        else
        {
            oal_print_hi11xx_log(HI11XX_LOG_DBG, "plat:wlan_pm_wdg_timeout_etc %d have packet %d....", pm_data->ul_wdg_timeout_curr_cnt, pm_data->ul_packet_cnt);
            pm_data->ul_wdg_timeout_curr_cnt = 0;
            pm_data->ul_packet_cnt           = 0;

            /* 有报文收发,连续forbid sleep次数清零 */
            pm_data->ul_sleep_fail_forbid     = 0;
        }

    }
    else
    {
        pm_data->ul_packet_cnt = 0;
    }


    oal_print_hi11xx_log(HI11XX_LOG_DBG, "wlan_pm_feed_wdg_etc");
    wlan_pm_feed_wdg_etc();

    return;

}

void wlan_pm_deepsleep_delay_timeout(struct wlan_pm_s *pm_data)
{
    oal_wake_unlock(&pm_data->st_deepsleep_wakelock);

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "wlan_pm_deepsleep_delay_timeout release wakelock....%lu", pm_data->st_deepsleep_wakelock.lock_count);

    if(0!=pm_data->st_deepsleep_wakelock.lock_count)
    {
       OAM_WARNING_LOG1(0, OAM_SF_PWR,"wlan_pm_deepsleep_delay_timeout release wakelock %lu!\n",pm_data->st_deepsleep_wakelock.lock_count);
    }
}

int32 wlan_pm_stop_deepsleep_delay_timer(struct wlan_pm_s *pm_data)
{
    oal_wake_unlock(&pm_data->st_deepsleep_wakelock);

    oal_print_hi11xx_log(HI11XX_LOG_DBG, "wlan_pm_stop_deepsleep_delay_timer release wakelock....%lu", pm_data->st_deepsleep_wakelock.lock_count);

    if(0!=pm_data->st_deepsleep_wakelock.lock_count)
    {
       OAM_WARNING_LOG1(0, OAM_SF_PWR,"wlan_pm_stop_deepsleep_delay_timer release wakelock %lu!\n",pm_data->st_deepsleep_wakelock.lock_count);
    }

    if(in_interrupt())
    {
        return del_timer(&pm_data->st_deepsleep_delay_timer);
    }
    else
    {
        return del_timer_sync(&pm_data->st_deepsleep_delay_timer);
    }

}




oal_int32 wlan_pm_poweroff_cmd_etc(oal_void)
{
    oal_int32            ret;
    //oal_uint32           ul_ret;
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    OAM_WARNING_LOG0(0, OAM_SF_PWR,"Send H2D_MSG_PM_WLAN_OFF cmd");

    hcc_tx_transfer_lock(hcc_get_110x_handler());

    if(OAL_SUCC != wlan_pm_wakeup_dev_etc())
    {
        ssi_dump_device_regs((HI1XX_ANDROID_BUILD_VARIANT_USER == hi11xx_get_android_build_variant()) ? (0x0):(SSI_MODULE_MASK_COMM));
        hcc_tx_transfer_unlock(hcc_get_110x_handler());
        return OAL_FAIL;
    }

    ret = hcc_bus_send_message(pst_wlan_pm->pst_bus, H2D_MSG_PM_WLAN_OFF);
    if(OAL_SUCC == ret)
    {
        /*等待device执行命令*/
        msleep(20);
        if(0==board_get_wlan_wkup_gpio_val_etc())
        {
            OAM_ERROR_LOG1(0, OAM_SF_PWR,"wlan_pm_poweroff_cmd_etc  wait device ACK timeout && GPIO_LEVEL[%d] !",board_get_wlan_wkup_gpio_val_etc());

#ifdef PLATFORM_DEBUG_ENABLE
            debug_uart_read_wifi_mem_etc(OAL_FALSE);
#endif
            ssi_dump_device_regs((HI1XX_ANDROID_BUILD_VARIANT_USER == hi11xx_get_android_build_variant()) ? (0x0):(SSI_MODULE_MASK_COMM));
            hcc_tx_transfer_unlock(hcc_get_110x_handler());
            return OAL_FAIL;
        }
    }
    else
    {
        OAM_ERROR_LOG0(0, OAM_SF_PWR,"fail to send H2D_MSG_PM_WLAN_OFF");
        hcc_tx_transfer_unlock(hcc_get_110x_handler());
        return  OAL_FAIL;
    }

    hcc_tx_transfer_unlock(hcc_get_110x_handler());

    return OAL_SUCC;


}

void wlan_pm_wkup_src_debug_set(oal_uint32 ul_en)
{
#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    if(OAL_PTR_NULL == pst_wlan_pm)
    {
        return;
    }
    pst_wlan_pm ->ul_wkup_src_print_en = ul_en;

#endif

}

EXPORT_SYMBOL_GPL(wlan_pm_wkup_src_debug_set);

oal_uint32 wlan_pm_wkup_src_debug_get(void)
{
#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    if(OAL_PTR_NULL == pst_wlan_pm)
    {
        return OAL_FALSE;
    }
    return  pst_wlan_pm ->ul_wkup_src_print_en;
#else
    return  OAL_FALSE;
#endif

}
EXPORT_SYMBOL_GPL(wlan_pm_wkup_src_debug_get);


oal_int32 wlan_pm_shutdown_bcpu_cmd_etc(oal_void)
{
#define RETRY_TIMES (3)
    oal_uint32           i;
    oal_int32            ret = OAL_FAIL;
    oal_uint32           ul_ret;
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    OAM_WARNING_LOG0(0, OAM_SF_PWR,"Send H2D_MSG_PM_BCPU_OFF cmd");

    hcc_tx_transfer_lock(hcc_get_110x_handler());

    for (i = 0; i < RETRY_TIMES; i++)
    {
        ret = wlan_pm_wakeup_dev_etc();
        if (OAL_SUCC == ret)
        {
            break;
        }
    }

    if(OAL_SUCC != ret)
    {
        OAM_ERROR_LOG0(0, OAM_SF_PWR,"wlan_pm_wakeup_dev_etc fail!");
        hcc_tx_transfer_unlock(hcc_get_110x_handler());
        return OAL_FAIL;
    }

    OAM_WARNING_LOG1(0, OAM_SF_PWR,"wlan_pm_wakeup_dev_etc succ, retry times [%d]", i);

    OAL_INIT_COMPLETION(&pst_wlan_pm->st_close_bcpu_done);

    ret = hcc_bus_send_message(pst_wlan_pm->pst_bus, H2D_MSG_PM_BCPU_OFF);
    if(OAL_SUCC == ret)
    {
        /*等待device执行命令*/
        ul_ret = oal_wait_for_completion_timeout(&pst_wlan_pm->st_close_bcpu_done, (oal_uint32)OAL_MSECS_TO_JIFFIES(WLAN_POWEROFF_ACK_WAIT_TIMEOUT));
        if(0 == ul_ret)
        {
            OAM_ERROR_LOG0(0, OAM_SF_PWR,"wlan_pm_shutdown_bcpu_cmd_etc wait device ACK timeout !");
            hcc_tx_transfer_unlock(hcc_get_110x_handler());
            return OAL_FAIL;
        }
    }
    else
    {
        OAM_ERROR_LOG0(0, OAM_SF_PWR,"fail to send H2D_MSG_PM_BCPU_OFF");
        hcc_tx_transfer_unlock(hcc_get_110x_handler());
        return  OAL_FAIL;
    }

    hcc_tx_transfer_unlock(hcc_get_110x_handler());

    return OAL_SUCC;
}



void wlan_pm_dump_host_info_etc(void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();
    struct oal_sdio     *pst_sdio = oal_get_sdio_default_handler();

    if(NULL == pst_wlan_pm)
    {
        return;
    }

    OAL_IO_PRINT("----------wlan_pm_dump_host_info_etc begin-----------\n");
    OAL_IO_PRINT("power on:%ld, enable:%ld,g_wlan_pm_switch_etc:%d\n",pst_wlan_pm->ul_wlan_power_state,pst_wlan_pm->ul_wlan_pm_enable,g_wlan_pm_switch_etc);
    OAL_IO_PRINT("dev state:%ld, sleep stage:%ld\n",pst_wlan_pm->ul_wlan_dev_state,pst_wlan_pm->ul_sleep_stage);
    OAL_IO_PRINT("open:%d,close:%d\n",pst_wlan_pm->ul_open_cnt,pst_wlan_pm->ul_close_cnt);
    if(NULL != pst_sdio)
        OAL_IO_PRINT("sdio suspend:%d,sdio resume:%d\n",pst_sdio->ul_sdio_suspend,pst_sdio->ul_sdio_resume);
    OAL_IO_PRINT("gpio_intr[no.%d]:%llu\n",pst_wlan_pm->pst_bus->bus_dev->ul_wlan_irq,pst_wlan_pm->pst_bus->gpio_int_count);
    OAL_IO_PRINT("data_intr:%llu\n",pst_wlan_pm->pst_bus->data_int_count);
    OAL_IO_PRINT("wakeup_intr:%llu\n",pst_wlan_pm->pst_bus->wakeup_int_count);
    OAL_IO_PRINT("D2H_MSG_WAKEUP_SUCC:%d\n",pst_wlan_pm->pst_bus->msg[D2H_MSG_WAKEUP_SUCC].count);
    OAL_IO_PRINT("D2H_MSG_ALLOW_SLEEP:%d\n",pst_wlan_pm->pst_bus->msg[D2H_MSG_ALLOW_SLEEP].count);
    OAL_IO_PRINT("D2H_MSG_DISALLOW_SLEEP:%d\n",pst_wlan_pm->pst_bus->msg[D2H_MSG_DISALLOW_SLEEP].count);


    OAL_IO_PRINT("wakeup_dev_wait_ack:%d\n",oal_atomic_read(&g_wakeup_dev_wait_ack_etc));
    OAL_IO_PRINT("wakeup_succ:%d\n",pst_wlan_pm->ul_wakeup_succ);
    OAL_IO_PRINT("wakeup_dev_ack:%d\n",pst_wlan_pm->ul_wakeup_dev_ack);
    OAL_IO_PRINT("wakeup_done_callback:%d\n",pst_wlan_pm->ul_wakeup_done_callback);
    OAL_IO_PRINT("wakeup_succ_work_submit:%d\n",pst_wlan_pm->ul_wakeup_succ_work_submit);
    OAL_IO_PRINT("wakeup_fail_wait_sdio:%d\n",pst_wlan_pm->ul_wakeup_fail_wait_sdio);
    OAL_IO_PRINT("wakeup_fail_timeout:%d\n",pst_wlan_pm->ul_wakeup_fail_timeout);
    OAL_IO_PRINT("wakeup_fail_set_reg:%d\n",pst_wlan_pm->ul_wakeup_fail_set_reg);
    OAL_IO_PRINT("wakeup_fail_submit_work:%d\n",pst_wlan_pm->ul_wakeup_fail_submit_work);
    OAL_IO_PRINT("sleep_succ:%d\n",pst_wlan_pm->ul_sleep_succ);
    OAL_IO_PRINT("sleep feed wdg:%d\n",pst_wlan_pm->ul_sleep_feed_wdg_cnt);
    OAL_IO_PRINT("sleep_fail_request:%d\n",pst_wlan_pm->ul_sleep_fail_request);
    OAL_IO_PRINT("sleep_fail_set_reg:%d\n",pst_wlan_pm->ul_sleep_fail_set_reg);
    OAL_IO_PRINT("sleep_fail_wait_timeout:%d\n",pst_wlan_pm->ul_sleep_fail_wait_timeout);
    OAL_IO_PRINT("sleep_fail_forbid:%d\n",pst_wlan_pm->ul_sleep_fail_forbid);
    OAL_IO_PRINT("sleep_work_submit:%d\n",pst_wlan_pm->ul_sleep_work_submit);
    OAL_IO_PRINT("wklock_cnt:%lu\n \n",pst_wlan_pm->pst_bus->st_bus_wakelock.lock_count);
    OAL_IO_PRINT("----------wlan_pm_dump_host_info_etc end-----------\n");

}



oal_int32 wlan_pm_host_info_print_etc(struct wlan_pm_s *pst_wlan_pm,char* buf,oal_int32 buf_len)
{
    oal_int32 ret = 0;
    struct oal_sdio     *pst_sdio = oal_get_sdio_default_handler();

    ret += snprintf(buf + ret , buf_len - ret,"----------wlan_pm_host_info_print_etc begin-----------\n");
    ret += snprintf(buf + ret , buf_len - ret,"power on:%ld, enable:%ld,g_wlan_pm_switch_etc:%d\n",pst_wlan_pm->ul_wlan_power_state,pst_wlan_pm->ul_wlan_pm_enable,g_wlan_pm_switch_etc);
    ret += snprintf(buf + ret , buf_len - ret,"dev state:%ld, sleep stage:%ld\n",pst_wlan_pm->ul_wlan_dev_state,pst_wlan_pm->ul_sleep_stage);
    ret += snprintf(buf + ret , buf_len - ret,"open:%d,close:%d\n",pst_wlan_pm->ul_open_cnt,pst_wlan_pm->ul_close_cnt);
    if(NULL != pst_sdio)
        ret += snprintf(buf + ret , buf_len - ret,"sdio suspend:%d,sdio resume:%d\n",pst_sdio->ul_sdio_suspend,pst_sdio->ul_sdio_resume);
    ret += snprintf(buf + ret , buf_len - ret,"gpio_intr[no.%d]:%llu\n",pst_wlan_pm->pst_bus->bus_dev->ul_wlan_irq,pst_wlan_pm->pst_bus->gpio_int_count);
    ret += snprintf(buf + ret , buf_len - ret,"data_intr:%llu\n",pst_wlan_pm->pst_bus->data_int_count);
    ret += snprintf(buf + ret , buf_len - ret,"wakeup_intr:%llu\n",pst_wlan_pm->pst_bus->wakeup_int_count);
    ret += snprintf(buf + ret , buf_len - ret,"D2H_MSG_WAKEUP_SUCC:%d\n",pst_wlan_pm->pst_bus->msg[D2H_MSG_WAKEUP_SUCC].count);
    ret += snprintf(buf + ret , buf_len - ret,"D2H_MSG_ALLOW_SLEEP:%d\n",pst_wlan_pm->pst_bus->msg[D2H_MSG_ALLOW_SLEEP].count);
    ret += snprintf(buf + ret , buf_len - ret,"D2H_MSG_DISALLOW_SLEEP:%d\n",pst_wlan_pm->pst_bus->msg[D2H_MSG_DISALLOW_SLEEP].count);


    ret += snprintf(buf + ret , buf_len - ret,"wakeup_dev_wait_ack:%d\n",oal_atomic_read(&g_wakeup_dev_wait_ack_etc));
    ret += snprintf(buf + ret , buf_len - ret,"wakeup_succ:%d\n",pst_wlan_pm->ul_wakeup_succ);
    ret += snprintf(buf + ret , buf_len - ret,"wakeup_dev_ack:%d\n",pst_wlan_pm->ul_wakeup_dev_ack);
    ret += snprintf(buf + ret , buf_len - ret,"wakeup_done_callback:%d\n",pst_wlan_pm->ul_wakeup_done_callback);
    ret += snprintf(buf + ret , buf_len - ret,"wakeup_succ_work_submit:%d\n",pst_wlan_pm->ul_wakeup_succ_work_submit);
    ret += snprintf(buf + ret , buf_len - ret,"wakeup_fail_wait_sdio:%d\n",pst_wlan_pm->ul_wakeup_fail_wait_sdio);
    ret += snprintf(buf + ret , buf_len - ret,"wakeup_fail_timeout:%d\n",pst_wlan_pm->ul_wakeup_fail_timeout);
    ret += snprintf(buf + ret , buf_len - ret,"wakeup_fail_set_reg:%d\n",pst_wlan_pm->ul_wakeup_fail_set_reg);
    ret += snprintf(buf + ret , buf_len - ret,"wakeup_fail_submit_work:%d\n",pst_wlan_pm->ul_wakeup_fail_submit_work);
    ret += snprintf(buf + ret , buf_len - ret,"sleep_succ:%d\n",pst_wlan_pm->ul_sleep_succ);
    ret += snprintf(buf + ret , buf_len - ret,"sleep feed wdg:%d\n",pst_wlan_pm->ul_sleep_feed_wdg_cnt);
    ret += snprintf(buf + ret , buf_len - ret,"sleep_fail_request:%d\n",pst_wlan_pm->ul_sleep_fail_request);
    ret += snprintf(buf + ret , buf_len - ret,"sleep_fail_set_reg:%d\n",pst_wlan_pm->ul_sleep_fail_set_reg);
    ret += snprintf(buf + ret , buf_len - ret,"sleep_fail_wait_timeout:%d\n",pst_wlan_pm->ul_sleep_fail_wait_timeout);
    ret += snprintf(buf + ret , buf_len - ret,"sleep_fail_forbid:%d\n",pst_wlan_pm->ul_sleep_fail_forbid);
    ret += snprintf(buf + ret , buf_len - ret,"sleep_work_submit:%d\n",pst_wlan_pm->ul_sleep_work_submit);
    ret += snprintf(buf + ret , buf_len - ret,"wklock_cnt:%lu\n \n",pst_wlan_pm->pst_bus->st_bus_wakelock.lock_count);
    ret += snprintf(buf + ret , buf_len - ret,"----------wlan_pm_host_info_print_etc end-----------\n");

    return ret;
}

void wlan_pm_dump_device_info_etc(void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    hcc_bus_send_message(pst_wlan_pm->pst_bus, H2D_MSG_PM_DEBUG);
}


void wlan_pm_info_clean_etc(void)
{
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();
    struct oal_sdio     *pst_sdio = oal_get_sdio_default_handler();

   // pst_wlan_pm->pst_sdio->gpio_int_count   = 0;
    pst_wlan_pm->pst_bus->data_int_count   = 0;
    pst_wlan_pm->pst_bus->wakeup_int_count = 0;

    pst_wlan_pm->pst_bus->msg[D2H_MSG_WAKEUP_SUCC].count = 0;
    pst_wlan_pm->pst_bus->msg[D2H_MSG_ALLOW_SLEEP].count = 0;
    pst_wlan_pm->pst_bus->msg[D2H_MSG_DISALLOW_SLEEP].count = 0;

    if(NULL != pst_sdio)
    {
        pst_sdio->ul_sdio_suspend               = 0;
        pst_sdio->ul_sdio_resume                = 0;
    }

    pst_wlan_pm->ul_wakeup_succ = 0;
	pst_wlan_pm->ul_wakeup_dev_ack = 0;
    pst_wlan_pm->ul_wakeup_done_callback = 0;
    pst_wlan_pm->ul_wakeup_succ_work_submit = 0;
    pst_wlan_pm->ul_wakeup_fail_wait_sdio = 0;
    pst_wlan_pm->ul_wakeup_fail_timeout = 0;
    pst_wlan_pm->ul_wakeup_fail_set_reg = 0;
    pst_wlan_pm->ul_wakeup_fail_submit_work = 0;

    pst_wlan_pm->ul_sleep_succ = 0;
    pst_wlan_pm->ul_sleep_feed_wdg_cnt = 0;
    pst_wlan_pm->ul_wakeup_done_callback = 0;
    pst_wlan_pm->ul_sleep_fail_set_reg = 0;
    pst_wlan_pm->ul_sleep_fail_wait_timeout = 0;
    pst_wlan_pm->ul_sleep_fail_forbid = 0;
    pst_wlan_pm->ul_sleep_work_submit = 0;


    return;

}


oal_void wlan_pm_debug_sleep_etc(void)
{
    struct wlan_pm_s   *pst_wlan_pm     = wlan_pm_get_drv_etc();

    if(pst_wlan_pm && pst_wlan_pm->pst_bus)
    {
        hcc_bus_sleep_request(pst_wlan_pm->pst_bus);

        pst_wlan_pm->ul_wlan_dev_state = HOST_ALLOW_TO_SLEEP;
    }

    return  ;
}

oal_void wlan_pm_debug_wakeup_etc(void)
{
    struct wlan_pm_s   *pst_wlan_pm     = wlan_pm_get_drv_etc();

    if(pst_wlan_pm && pst_wlan_pm->pst_bus)
    {
        hcc_bus_wakeup_request(pst_wlan_pm->pst_bus);

        pst_wlan_pm->ul_wlan_dev_state = HOST_DISALLOW_TO_SLEEP;
    }

    return  ;
}

oal_void wlan_pm_debug_wake_lock_etc(void)
{
    struct wlan_pm_s   *pst_wlan_pm     = wlan_pm_get_drv_etc();

    hcc_bus_wake_lock(pst_wlan_pm->pst_bus);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "wlan_pm_debug_wake_lock_etc:wklock_cnt = %lu",pst_wlan_pm->pst_bus->st_bus_wakelock.lock_count);

    return  ;
}

oal_void wlan_pm_debug_wake_unlock_etc(void)
{
    struct wlan_pm_s   *pst_wlan_pm     = wlan_pm_get_drv_etc();

    hcc_bus_wake_unlock(pst_wlan_pm->pst_bus);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "wlan_pm_debug_wake_unlock_etc:wklock_cnt = %lu",pst_wlan_pm->pst_bus->st_bus_wakelock.lock_count);

    return  ;
}



