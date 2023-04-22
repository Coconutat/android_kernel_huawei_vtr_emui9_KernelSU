


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_net.h"
#include "oal_types.h"
#include "oam_ext_if.h"
#include  "mac_vap.h"
#include  "mac_resource.h"
#include  "hmac_vap.h"
#include  "hmac_auto_adjust_freq.h"
#include  "hmac_ext_if.h"
#include  "hmac_blockack.h"

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/pm_qos.h>
#endif


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_AUTO_ADJUST_FREQ_C


#define HMAC_AUTO_FREQ_NORMAL_CPU   0
#define HMAC_AUTO_FREQ_BUSY_CPU     1

#define HMAC_AUTO_FREQ_CPU_NUM      4

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
extern hmac_rxdata_thread_stru     g_st_rxdata_thread;

#ifndef WIN32
OAL_STATIC oal_uint32 pre_jiffies            = 0;
OAL_STATIC oal_uint32 g_adjust_count            = 0;

/*由定制化进行初始化*/
host_speed_freq_level_stru g_host_speed_freq_level[4] = {
    /*pps门限                   CPU主频下限                     DDR主频下限*/
    {PPS_VALUE_0,          CPU_MIN_FREQ_VALUE_0,            DDR_MIN_FREQ_VALUE_0},
    {PPS_VALUE_1,          CPU_MIN_FREQ_VALUE_1,            DDR_MIN_FREQ_VALUE_1},
    {PPS_VALUE_2,          CPU_MIN_FREQ_VALUE_2,            DDR_MIN_FREQ_VALUE_2},
    {PPS_VALUE_3,          CPU_MIN_FREQ_VALUE_3,            DDR_MIN_FREQ_VALUE_3},
};
host_speed_freq_level_stru g_host_no_ba_freq_level[4] = {
    /*pps门限                        CPU主频下限                      DDR主频下限*/
    {NO_BA_PPS_VALUE_0,          CPU_MIN_FREQ_VALUE_0,            DDR_MIN_FREQ_VALUE_0},
    {NO_BA_PPS_VALUE_1,          CPU_MIN_FREQ_VALUE_1,            DDR_MIN_FREQ_VALUE_1},
    {NO_BA_PPS_VALUE_2,          CPU_MIN_FREQ_VALUE_2,            DDR_MIN_FREQ_VALUE_2},
    {NO_BA_PPS_VALUE_3,          CPU_MIN_FREQ_VALUE_2,            DDR_MIN_FREQ_VALUE_2},
};
device_speed_freq_level_stru g_device_speed_freq_level[] = {
    /*device主频类型*/
    {FREQ_IDLE},
    {FREQ_MIDIUM},
    {FREQ_HIGHER},
    {FREQ_HIGHEST},
};

struct pm_qos_request *g_pst_wifi_auto_ddr = NULL;


freq_lock_control_stru g_freq_lock_control = {0};
OAL_STATIC oal_uint32 g_ul_wifi_rxtx_total          = 0;

OAL_STATIC oal_uint32 g_ul_orig_cpu_min_freq       = 0;
OAL_STATIC oal_uint32 g_ul_orig_cpu_max_freq       = 0;
OAL_STATIC oal_uint32 g_ul_orig_ddr_min_freq       = 0;
OAL_STATIC oal_uint32 g_ul_orig_ddr_max_freq       = 0;
#else
oal_uint32 pre_jiffies;

host_speed_freq_level_stru g_host_speed_freq_level[] = {
    /*pps门限                   CPU主频下限                     DDR主频下限*/
    {PPS_VALUE_0,          CPU_MIN_FREQ_VALUE_0,            DDR_MIN_FREQ_VALUE_0},
    {PPS_VALUE_1,          CPU_MIN_FREQ_VALUE_1,            DDR_MIN_FREQ_VALUE_1},
    {PPS_VALUE_2,          CPU_MIN_FREQ_VALUE_2,            DDR_MIN_FREQ_VALUE_2},
    {PPS_VALUE_3,          CPU_MIN_FREQ_VALUE_3,            DDR_MIN_FREQ_VALUE_3},
};
device_speed_freq_level_stru g_device_speed_freq_level[] = {
    /*device主频类型*/
    {FREQ_IDLE},
    {FREQ_MIDIUM},
    {FREQ_HIGHEST},
    {FREQ_HIGHEST},
};

freq_lock_control_stru g_freq_lock_control;
oal_uint32 g_ul_wifi_rxtx_total;

oal_uint32 g_ul_orig_cpu_min_freq;
oal_uint32 g_ul_orig_cpu_max_freq;
oal_uint32 g_ul_orig_ddr_min_freq;
oal_uint32 g_ul_orig_ddr_max_freq;
#endif
#ifdef WIN32
#define mutex_init(mux)
#define mutex_lock(mux)
#define mutex_unlock(mux)
#define spin_lock_init(mux)
#define mutex_destroy(mux)
#define spin_unlock_bh(mux)
#endif


oal_uint8 hmac_set_auto_freq_mod(oal_uint8 uc_freq_enable)
{
    g_freq_lock_control.uc_lock_mod = uc_freq_enable;
    /* 设置device是否使能 */
    if(FREQ_LOCK_ENABLE == uc_freq_enable)
    {
        hmac_set_device_freq_mode(FREQ_LOCK_ENABLE);
    }
    else
    {
        hmac_set_device_freq_mode(FREQ_LOCK_DISABLE);
    }
    return 0;
}

oal_bool_enum_uint8 hmac_set_auto_freq_debug_print(oal_bool_enum_uint8 en_debug_print)
{
    OAM_WARNING_LOG1(0,OAM_SF_ANY,"{hmac_set_auto_freq_debug_print en_debug_print = %d!}",g_freq_lock_control.en_debug_print);
    g_freq_lock_control.en_debug_print = en_debug_print;
    return 0;
}


oal_int32 hmac_set_auto_freq_process_func(oal_void)
{
    struct alg_process_func_handler* pst_alg_process_func_handler;

    g_freq_lock_control.uc_lock_mod = FREQ_LOCK_DISABLE;
    g_freq_lock_control.en_debug_print = OAL_FALSE;
    g_freq_lock_control.uc_curr_lock_level = 0;
    pst_alg_process_func_handler = oal_get_alg_process_func();
    if(OAL_PTR_NULL == pst_alg_process_func_handler)
    {
        OAM_ERROR_LOG0(0,OAM_SF_ANY,"{hmac_set_auto_freq_process_func get handler failed!}");
    }
    else
    {
        pst_alg_process_func_handler->p_auto_freq_count_func = hmac_hcc_auto_freq_count;
        pst_alg_process_func_handler->p_auto_freq_process_func = hmac_hcc_auto_freq_process;
        pst_alg_process_func_handler->p_auto_freq_set_lock_mod_func = hmac_set_auto_freq_mod;
        pst_alg_process_func_handler->p_auto_freq_adjust_to_level_func = hmac_adjust_freq_to_level;
    }

    return OAL_SUCC;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_get_cpu_freq_raw(oal_uint8 uc_freq_type, oal_uint32 * pst_ul_freq_value)
{
    struct file* filp = NULL;
    mm_segment_t old_fs;
    oal_int8 buf[12] = {0};

    if (uc_freq_type == SCALING_MAX_FREQ)
        filp = filp_open(CPU_MAX_FREQ, O_RDONLY, 0);
    else if (uc_freq_type == SCALING_MIN_FREQ)
        filp = filp_open(CPU_MIN_FREQ, O_RDONLY, 0);
    else
        return -1;

    if (IS_ERR_OR_NULL(filp))
    {
        OAM_ERROR_LOG1(0,OAM_SF_ANY,"{hmac_get_cpu_freq_raw:　freq　= %d error !}",uc_freq_type);
        return -1;
    }
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    filp->f_pos = 0;
    filp->f_op->read(filp, buf, 12, &filp->f_pos);
    filp_close(filp, NULL);
    set_fs(old_fs);

    if (kstrtouint(buf, 10, pst_ul_freq_value) != 0)
    {
        OAM_ERROR_LOG0(0,OAM_SF_ANY,"{error to get cpu freq !}");
        return -1;
    }

    return 0;
}


oal_bool_enum_uint8 hmac_set_cpu_freq_raw(oal_uint8 uc_freq_type, oal_uint32 ul_freq_value)
{
    struct file* filp = NULL;
    mm_segment_t old_fs;
    oal_int8 buf[12] = {0};

    snprintf(buf, 12, "%d", ul_freq_value);

    if (uc_freq_type == SCALING_MIN_FREQ)
        filp = filp_open(CPU_MIN_FREQ, O_RDWR, 0);
    else if (uc_freq_type == SCALING_MAX_FREQ)
        filp = filp_open(CPU_MAX_FREQ, O_RDWR, 0);
    else
        return -1;

    if (IS_ERR_OR_NULL(filp))
    {
        OAM_ERROR_LOG1(0,OAM_SF_ANY,"{hmac_set_cpu_freq_raw:　freq　= %d error !}",ul_freq_value);
        return -1;
    }
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    filp->f_pos = 0;
    filp->f_op->write(filp, buf, 12, &filp->f_pos);
    filp_close(filp, NULL);
    set_fs(old_fs);

    return 0;
}



OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_get_ddr_freq_raw(oal_uint8 uc_freq_type, oal_uint32 * pst_ul_freq_value)
{
    struct file* filp = NULL;
    mm_segment_t old_fs;
    oal_int8 buf[12] = {0};

    if (uc_freq_type == SCALING_MAX_FREQ)
        filp = filp_open(DDR_MAX_FREQ, O_RDONLY, 0);
    else if (uc_freq_type == SCALING_MIN_FREQ)
        filp = filp_open(DDR_MIN_FREQ, O_RDONLY, 0);
    else
        return -1;

    if (IS_ERR_OR_NULL(filp))
    {
        OAM_ERROR_LOG1(0,OAM_SF_ANY,"{hmac_get_ddr_freq_raw:　freq　= %d error !}",uc_freq_type);
        return -1;
    }
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    filp->f_pos = 0;
    filp->f_op->read(filp, buf, 12, &filp->f_pos);
    filp_close(filp, NULL);
    set_fs(old_fs);

    if (kstrtouint(buf, 10, pst_ul_freq_value) != 0)
    {
        printk("error to get cpu freq\n");
        return -1;
    }

    return 0;
}



oal_bool_enum_uint8 hmac_set_ddr_freq_raw(oal_uint8 uc_freq_type, oal_uint32 ul_freq_value)
{
#if 0
    struct file* filp = NULL;
    mm_segment_t old_fs;
    char buf[12] = {0};

    snprintf(buf, 12, "%d", ul_freq_value);

    if (uc_freq_type == SCALING_MIN_FREQ)
        filp = filp_open(DDR_MIN_FREQ, O_RDWR, 0);
    else if (uc_freq_type == SCALING_MAX_FREQ)
        filp = filp_open(DDR_MAX_FREQ, O_RDWR, 0);
    else
        return -1;

    if (IS_ERR_OR_NULL(filp))
    {
        OAM_ERROR_LOG1(0,OAM_SF_ANY,"{hmac_set_ddr_freq_raw:　freq　= %d error !}",ul_freq_value);
        return -1;
    }
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    filp->f_pos = 0;
    filp->f_op->write(filp, buf, 12, &filp->f_pos);
    filp_close(filp, NULL);
    set_fs(old_fs);
#else
    pm_qos_update_request(g_pst_wifi_auto_ddr, ul_freq_value);
#endif
    return 0;
}
#endif


oal_void  hmac_auto_freq_set_thread_affinity(oal_uint32 ul_total_sdio_rate)
{
#ifdef CONFIG_NR_CPUS
#if (CONFIG_NR_CPUS > HMAC_AUTO_FREQ_CPU_NUM)
    OAL_STATIC oal_uint32 ul_current_cpu = HMAC_AUTO_FREQ_NORMAL_CPU;
    struct cpumask cpu_mask;

    if ((ul_total_sdio_rate >= g_host_speed_freq_level[FREQ_HIGHEST].ul_speed_level)
        && (HMAC_AUTO_FREQ_NORMAL_CPU == ul_current_cpu))
    {
        cpumask_setall(&cpu_mask);
        ul_current_cpu = HMAC_AUTO_FREQ_BUSY_CPU;
        /* 设置wifi 线程到CPU4~7 */
        cpumask_clear_cpu(0, &cpu_mask);
        cpumask_clear_cpu(1, &cpu_mask);
        cpumask_clear_cpu(2, &cpu_mask);
        cpumask_clear_cpu(3, &cpu_mask);
    }
    else if ((ul_total_sdio_rate <= g_host_speed_freq_level[FREQ_HIGHER].ul_speed_level)
        && (HMAC_AUTO_FREQ_BUSY_CPU == ul_current_cpu))
    {
        cpumask_setall(&cpu_mask);
        ul_current_cpu = HMAC_AUTO_FREQ_NORMAL_CPU;
        /* 设置wifi 线程到CPU1~3 */
        cpumask_clear_cpu(0, &cpu_mask);
        cpumask_clear_cpu(4, &cpu_mask);
        cpumask_clear_cpu(5, &cpu_mask);
        cpumask_clear_cpu(6, &cpu_mask);
        cpumask_clear_cpu(7, &cpu_mask);
    }
    else
    {
        return;
    }

    if (OAL_PTR_NULL != g_st_rxdata_thread.pst_rxdata_thread)
    {
        set_cpus_allowed_ptr(g_st_rxdata_thread.pst_rxdata_thread, &cpu_mask);
    }

    if (OAL_PTR_NULL != hcc_get_default_handler()->hcc_transer_info.hcc_transfer_thread)
    {
        set_cpus_allowed_ptr(hcc_get_default_handler()->hcc_transer_info.hcc_transfer_thread, &cpu_mask);
    }
#endif  /* CONFIG_NR_CPUS > HMAC_AUTO_FREQ_CPU_NUM */
#endif  /* CONFIG_NR_CPUS */
}


oal_void hmac_adjust_freq_to_level(oal_void)
{
    /* 根据帧速率调整wifi 线程绑定大核/小核 */
    hmac_auto_freq_set_thread_affinity(g_freq_lock_control.ul_total_sdio_rate);

#if 0
    oal_uint8 uc_req_lock_level = g_freq_lock_control.uc_req_lock_level;

    OAM_WARNING_LOG2(0,OAM_SF_PWR,"{hmac_adjust_freq_to_level: freq to [%d][%d]}",g_freq_lock_control.uc_curr_lock_level,uc_req_lock_level);

    if(hmac_is_device_ba_setup())
    {
        hmac_set_cpu_freq_raw(SCALING_MIN_FREQ,g_host_speed_freq_level[uc_req_lock_level].ul_min_cpu_freq);
        hmac_set_ddr_freq_raw(SCALING_MIN_FREQ,g_host_speed_freq_level[uc_req_lock_level].ul_min_ddr_freq);
    }
    else
    {
        hmac_set_cpu_freq_raw(SCALING_MIN_FREQ,g_host_no_ba_freq_level[uc_req_lock_level].ul_min_cpu_freq);
        hmac_set_ddr_freq_raw(SCALING_MIN_FREQ,g_host_no_ba_freq_level[uc_req_lock_level].ul_min_ddr_freq);
    }

    g_freq_lock_control.uc_curr_lock_level = uc_req_lock_level;
#endif
}


void hmac_perform_calc_rwtotal_throughput(oal_uint32 ul_rxtx_total,oal_uint32 ul_sdio_dur_us)
{
    if(0 != ul_sdio_dur_us)
    {
        g_freq_lock_control.ul_total_sdio_rate = (ul_rxtx_total*1000)/ul_sdio_dur_us;
        //OAM_WARNING_LOG1(0,OAM_SF_ANY,"{SDIO perform tx statistic: packet_rate = [%d]!}",g_freq_lock_control.ul_total_sdio_rate);

        if(OAL_TRUE == g_freq_lock_control.en_debug_print)
        {
            OAM_WARNING_LOG4(0,OAM_SF_ANY,"{SDIO perform tx statistic: packet_rate = %lu pps, sumlen = %lu B, [use time] = %lu ms,g_adjust_count = %d!}",
                g_freq_lock_control.ul_total_sdio_rate , ul_rxtx_total, ul_sdio_dur_us,g_adjust_count);
        }
    }
}

oal_uint8 hmac_get_freq_level(oal_uint32 ul_speed)
{
    oal_uint8 level_idx;

    if(hmac_is_device_ba_setup())
    {
        if (ul_speed <= g_host_speed_freq_level[1].ul_speed_level)
        {
            level_idx = 0;
        }
        else if ((ul_speed > g_host_speed_freq_level[1].ul_speed_level)
            && (ul_speed <= g_host_speed_freq_level[2].ul_speed_level))
        {
            level_idx = 1;
        }
        else if ((ul_speed > g_host_speed_freq_level[2].ul_speed_level)
            && (ul_speed <= g_host_speed_freq_level[3].ul_speed_level))
        {
            level_idx = 2;
        }
        else
        {
            level_idx = 3;
        }
    }
    else
    {
        if (ul_speed <= g_host_no_ba_freq_level[1].ul_speed_level)
        {
            level_idx = 0;
        }
        else if ((ul_speed > g_host_no_ba_freq_level[1].ul_speed_level)
            && (ul_speed <= g_host_no_ba_freq_level[2].ul_speed_level))
        {
            level_idx = 1;
        }
        else if ((ul_speed > g_host_no_ba_freq_level[2].ul_speed_level)
            && (ul_speed <= g_host_no_ba_freq_level[3].ul_speed_level))
        {
            level_idx = 2;
        }
        else
        {
            level_idx = 3;
        }
    }
    return level_idx;
}

oal_void hmac_adjust_freq(oal_void)
{
    oal_uint8 uc_req_lock_level = 0;
    oal_uint32         ul_cur_jiffies;
    oal_uint32         ul_sdio_dur_us;

    ul_cur_jiffies = jiffies;
    ul_sdio_dur_us = OAL_JIFFIES_TO_MSECS(ul_cur_jiffies - pre_jiffies);
    pre_jiffies = ul_cur_jiffies;

    /*计算调频级别*/
    hmac_perform_calc_rwtotal_throughput(g_ul_wifi_rxtx_total,ul_sdio_dur_us);
    g_freq_lock_control.uc_req_lock_level = hmac_get_freq_level(g_freq_lock_control.ul_total_sdio_rate);

    uc_req_lock_level = g_freq_lock_control.uc_req_lock_level;
    if (uc_req_lock_level != g_freq_lock_control.uc_curr_lock_level)
    {
        mutex_lock(&g_freq_lock_control.st_lock_freq_mtx);

        if (uc_req_lock_level != g_freq_lock_control.uc_curr_lock_level)
        {
            if(uc_req_lock_level < g_freq_lock_control.uc_curr_lock_level)
            {
                /*连续MAX_DEGRADE_FREQ_TIME_THRESHOLD后才降频，保证性能*/
                g_adjust_count++;
                if(0 != g_ul_wifi_rxtx_total)
                {
                    if(g_adjust_count >= MAX_DEGRADE_FREQ_COUNT_THRESHOLD_SUCCESSIVE_10)
                    {
                        g_adjust_count = 0;
                        wlan_pm_adjust_feq();
                    }
                }
                else
                {
                    if(g_adjust_count >= MAX_DEGRADE_FREQ_COUNT_THRESHOLD_SUCCESSIVE_3)
                    {
                        g_adjust_count = 0;
                        wlan_pm_adjust_feq();
                    }
                }
            }
            else
            {
                /*升频不等待，立即执行保证性能*/
                g_adjust_count = 0;
                wlan_pm_adjust_feq();
            }
        }
        else
        {
            g_adjust_count = 0;
        }

        mutex_unlock(&g_freq_lock_control.st_lock_freq_mtx);
    }
    else
    {
        g_adjust_count = 0;
    }
}

oal_void hmac_wifi_init_freq_threshold(void)
{
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_void hmac_wifi_auto_ddr_init(oal_void)
{
    g_pst_wifi_auto_ddr = kmalloc(sizeof(struct pm_qos_request), GFP_KERNEL);
    if (g_pst_wifi_auto_ddr == NULL)
    {
        OAL_IO_PRINT("[AUTODDR]pm_qos_request alloc memory failed.\n");
        return;
    }
    g_pst_wifi_auto_ddr->pm_qos_class = 0;
    pm_qos_add_request(g_pst_wifi_auto_ddr, PM_QOS_MEMORY_THROUGHPUT,
                       PM_QOS_MEMORY_THROUGHPUT_DEFAULT_VALUE);
    return;
}
oal_void hmac_wifi_auto_ddr_exit(oal_void)
{
    pm_qos_remove_request(g_pst_wifi_auto_ddr);
    kfree(g_pst_wifi_auto_ddr);
    g_pst_wifi_auto_ddr = NULL;
}
#endif

oal_void hmac_wifi_auto_freq_ctrl_init(void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if(OAL_TRUE != g_freq_lock_control.en_is_inited)
    {
        OAM_WARNING_LOG0(0,OAM_SF_ANY,"{hmac_wifi_auto_freq_ctrl_init enter!}");

        mutex_init(&g_freq_lock_control.st_lock_freq_mtx);

        mutex_lock(&g_freq_lock_control.st_lock_freq_mtx);

        pre_jiffies = jiffies;
        g_freq_lock_control.uc_lock_mod = FREQ_LOCK_DISABLE;
        g_freq_lock_control.uc_curr_lock_level = FREQ_IDLE;
        hmac_get_cpu_freq_raw(SCALING_MIN_FREQ, &g_ul_orig_cpu_min_freq);
        hmac_get_cpu_freq_raw(SCALING_MAX_FREQ, &g_ul_orig_cpu_max_freq);
#if 0
        hmac_get_ddr_freq_raw(SCALING_MIN_FREQ, &g_ul_orig_ddr_min_freq);
        hmac_get_ddr_freq_raw(SCALING_MAX_FREQ, &g_ul_orig_ddr_max_freq);
#else
        hmac_wifi_auto_ddr_init();
#endif
        hmac_wifi_init_freq_threshold();
        OAM_WARNING_LOG4(0,OAM_SF_ANY,"{hmac_wifi_auto_freq_ctrl_init g_ul_orig_cpu_min_freq = %d,g_ul_orig_cpu_max_freq = %d,g_ul_orig_ddr_max_freq = %d,g_ul_orig_ddr_max_freq = %d}",
            g_ul_orig_cpu_min_freq,g_ul_orig_cpu_max_freq,g_ul_orig_ddr_min_freq,g_ul_orig_ddr_max_freq);
        g_freq_lock_control.en_is_inited = OAL_TRUE;
        mutex_unlock(&g_freq_lock_control.st_lock_freq_mtx);
    }
#endif
}

oal_void hmac_wifi_auto_freq_ctrl_deinit(void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    OAM_WARNING_LOG0(0,OAM_SF_ANY,"{hmac_wifi_auto_freq_ctrl_deinit enter!}");
    mutex_lock(&g_freq_lock_control.st_lock_freq_mtx);
    if (FREQ_LOCK_ENABLE == g_freq_lock_control.uc_lock_mod)
    {
        hmac_set_cpu_freq_raw(SCALING_MIN_FREQ, g_ul_orig_cpu_min_freq);
        hmac_set_cpu_freq_raw(SCALING_MAX_FREQ, g_ul_orig_cpu_max_freq);
        OAM_WARNING_LOG0(0,OAM_SF_ANY,"{hw_wifi_freq_ctrl_destroy freq lock release here!}");
    }
    else
    {
        OAM_WARNING_LOG0(0,OAM_SF_ANY,"{hw_wifi_freq_ctrl_destroy freq lock has already been released!}");
    }
    g_freq_lock_control.uc_lock_mod = FREQ_LOCK_DISABLE;
    g_freq_lock_control.uc_curr_lock_level = 0;
#if 1
    hmac_wifi_auto_ddr_exit();
#endif

    g_freq_lock_control.en_is_inited = OAL_FALSE;
    mutex_unlock(&g_freq_lock_control.st_lock_freq_mtx);
    mutex_destroy(&g_freq_lock_control.st_lock_freq_mtx);
#endif
}
oal_uint32 hmac_wifi_tx_rx_counter(oal_uint32 ul_pkt_count)
{
    g_ul_wifi_rxtx_total = g_ul_wifi_rxtx_total + ul_pkt_count;
#if 0
    OAM_WARNING_LOG2(0,OAM_SF_ANY,"{hmac_wifi_tx_rx_counter, ul_pkt_count = %d, g_ul_wifi_rxtx_total = %d!}",ul_pkt_count,g_ul_wifi_rxtx_total);
#endif

    return 0;
}
oal_void hmac_hcc_auto_freq_count(oal_uint32 ul_pkt_count)
{
    g_ul_wifi_rxtx_total = g_ul_wifi_rxtx_total + ul_pkt_count;
#if 0
    OAM_WARNING_LOG2(0,OAM_SF_ANY,"{hmac_hcc_auto_freq_count, ul_pkt_count = %d, g_ul_wifi_rxtx_total = %d!}",ul_pkt_count,g_ul_wifi_rxtx_total);
#endif

}
oal_uint32 hmac_hcc_auto_freq_process(oal_void)
{
    oal_uint32 ul_return_total_count = 0;

    /*保存之前的值，返回给平台*/
    ul_return_total_count = g_ul_wifi_rxtx_total;

    if(FREQ_LOCK_ENABLE == g_freq_lock_control.uc_lock_mod)
    {
        hmac_adjust_freq();
    }

#ifdef _PRE_WLAN_TCP_OPT
    /* 根据流量pps 来控制 启动/关闭 TCP_ACK 优化功能 */
    hmac_tcp_ack_opt_switch_ctrol(ul_return_total_count);
#endif  /* _PRE_WLAN_TCP_OPT */

    g_ul_wifi_rxtx_total = 0;
    return ul_return_total_count;
}


#endif


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

