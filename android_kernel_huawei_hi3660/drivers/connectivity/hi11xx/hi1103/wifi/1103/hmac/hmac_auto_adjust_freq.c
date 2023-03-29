


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
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
#include "hmac_tx_data.h"

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#include <linux/pm_qos.h>
#endif


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_AUTO_ADJUST_FREQ_C

#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET
#define INVALID_PKMODE_TH  (0xFFFFFFFF)
#endif
/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#ifdef WIN32
oal_uint32 jiffies;
#endif

freq_lock_control_stru g_freq_lock_control_etc = {0};
OAL_STATIC wifi_txrx_pkt_stat g_st_wifi_rxtx_total = {0};
/* Wi-Fi驱动收发负载识别数据区 */
freq_wifi_load_stru g_st_wifi_load = {0};

#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET
oal_bool_enum_uint8 g_en_pk_mode_swtich = OAL_TRUE;

/* pk mode的判定的门限，以字节为单位 */
/*
PK模式门限基数:
{(单位Mbps)  20M     40M    80M   160M   80+80M
lagency:    {valid, valid, valid, valid, valid},   (基础协议模式没有pk mode )
HT:         {62, 123, valid, valid, valid},
VHT:        {77, 167, 329, 660, 660},
HE:         {valid, valid, valid, valid, valid},   (暂不支持11ax的pk mode)
};


PK模式二级门限:
高档位门限: g_st_pk_mode_high_th_table = PK模式门限基数 * 70% *1024 *1024 /8  (单位字节)
低档位门限: g_st_pk_mode_low_th_table  = PK模式门限基数 * 30% *1024 *1024 /8  (单位字节)

*/
#if 1
oal_uint32 g_st_pk_mode_high_th_table[WLAN_PROTOCOL_CAP_BUTT][WLAN_BW_CAP_BUTT] = {
    {INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH},
    {45,                        110,        INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH},
    {60,                        130,               280,               562,               562},
#ifdef _PRE_WLAN_FEATURE_11AX
    {INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH},
#endif /* #ifdef _PRE_WLAN_FEATURE_11AX */

};

oal_uint32 g_st_pk_mode_low_th_table[WLAN_PROTOCOL_CAP_BUTT][WLAN_BW_CAP_BUTT] = {
    {INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH},
    {30,                        80,        INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH},
    {40,                        100,                 240,              400,               400},
#ifdef _PRE_WLAN_FEATURE_11AX
    {INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH, INVALID_PKMODE_TH},
#endif /* #ifdef _PRE_WLAN_FEATURE_11AX */

};
#endif

#endif

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
oal_uint32 g_ul_orig_cpu_min_freq       = 0;
oal_uint32 g_ul_orig_cpu_max_freq       = 0;
oal_uint32 g_ul_orig_ddr_min_freq       = 0;
oal_uint32 g_ul_orig_ddr_max_freq       = 0;

#ifndef WIN32
/*由定制化进行初始化*/
host_speed_freq_level_stru g_host_speed_freq_level_etc[4] = {
    /*pps门限                   CPU主频下限                     DDR主频下限*/
    {PPS_VALUE_0,          CPU_MIN_FREQ_VALUE_0,            DDR_MIN_FREQ_VALUE_0},
    {PPS_VALUE_1,          CPU_MIN_FREQ_VALUE_1,            DDR_MIN_FREQ_VALUE_1},
    {PPS_VALUE_2,          CPU_MIN_FREQ_VALUE_2,            DDR_MIN_FREQ_VALUE_2},
    {PPS_VALUE_3,          CPU_MIN_FREQ_VALUE_3,            DDR_MIN_FREQ_VALUE_3},
};
host_speed_freq_level_stru g_host_no_ba_freq_level_etc[4] = {
    /*pps门限                        CPU主频下限                      DDR主频下限*/
    {NO_BA_PPS_VALUE_0,          CPU_MIN_FREQ_VALUE_0,            DDR_MIN_FREQ_VALUE_0},
    {NO_BA_PPS_VALUE_1,          CPU_MIN_FREQ_VALUE_1,            DDR_MIN_FREQ_VALUE_1},
    {NO_BA_PPS_VALUE_2,          CPU_MIN_FREQ_VALUE_2,            DDR_MIN_FREQ_VALUE_2},
    {NO_BA_PPS_VALUE_3,          CPU_MIN_FREQ_VALUE_2,            DDR_MIN_FREQ_VALUE_2},
};
device_speed_freq_level_stru g_device_speed_freq_level_etc[] = {
    /*device主频类型*/
    {FREQ_IDLE},
    {FREQ_MIDIUM},
    {FREQ_HIGHER},
    {FREQ_HIGHEST},
};

struct pm_qos_request *g_pst_wifi_auto_ddr_etc = NULL;

#else
host_speed_freq_level_stru g_host_speed_freq_level_etc[] = {
    /*pps门限                   CPU主频下限                     DDR主频下限*/
    {PPS_VALUE_0,          CPU_MIN_FREQ_VALUE_0,            DDR_MIN_FREQ_VALUE_0},
    {PPS_VALUE_1,          CPU_MIN_FREQ_VALUE_1,            DDR_MIN_FREQ_VALUE_1},
    {PPS_VALUE_2,          CPU_MIN_FREQ_VALUE_2,            DDR_MIN_FREQ_VALUE_2},
    {PPS_VALUE_3,          CPU_MIN_FREQ_VALUE_3,            DDR_MIN_FREQ_VALUE_3},
};
device_speed_freq_level_stru g_device_speed_freq_level_etc[] = {
    /*device主频类型*/
    {FREQ_IDLE},
    {FREQ_MIDIUM},
    {FREQ_HIGHEST},
    {FREQ_HIGHEST},
};

#define mutex_init(mux)
#define mutex_lock(mux)
#define mutex_unlock(mux)
#define spin_lock_init(mux)
#define mutex_destroy(mux)
#define spin_unlock_bh(mux)
#endif

extern hmac_rxdata_thread_stru     g_st_rxdata_thread_etc;

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

OAL_STATIC oal_bool_enum_uint8 hmac_get_cpu_freq_raw(oal_uint8 uc_freq_type, oal_uint32 * pst_ul_freq_value)
{
#if 0
    struct file* filp = NULL;
    mm_segment_t old_fs;
    oal_int8 buf[12] = {0};

    if (uc_freq_type == SCALING_MAX_FREQ)
        filp = filp_open(CPU_MAX_FREQ, O_RDONLY, 0);
    else if (uc_freq_type == SCALING_MIN_FREQ)
        filp = filp_open(CPU_MIN_FREQ, O_RDONLY, 0);
    else
        return -1;

    if (OAL_IS_ERR_OR_NULL(filp))
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
#endif
    return 0;
}


oal_bool_enum_uint8 hmac_set_cpu_freq_raw_etc(oal_uint8 uc_freq_type, oal_uint32 ul_freq_value)
{
#if 0
    struct file* filp = NULL;
    mm_segment_t old_fs;
    oal_int8 buf[12] = {0};

    OAL_SPRINTF(buf, 12, "%d", ul_freq_value);

    if (uc_freq_type == SCALING_MIN_FREQ)
        filp = filp_open(CPU_MIN_FREQ, O_RDWR, 0);
    else if (uc_freq_type == SCALING_MAX_FREQ)
        filp = filp_open(CPU_MAX_FREQ, O_RDWR, 0);
    else
        return -1;

    if (OAL_IS_ERR_OR_NULL(filp))
    {
        OAM_WARNING_LOG2(0,OAM_SF_ANY,"{hmac_set_cpu_freq_raw_etc:freq type[%d]req　= %d error !}",uc_freq_type, ul_freq_value);
        return -1;
    }
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    filp->f_pos = 0;
    filp->f_op->write(filp, buf, 12, &filp->f_pos);
    filp_close(filp, NULL);
    set_fs(old_fs);
#endif
    return 0;
}

#if 0

OAL_STATIC oal_bool_enum_uint8 hmac_get_ddr_freq_raw(oal_uint8 uc_freq_type, oal_uint32 * pst_ul_freq_value)
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

    if (OAL_IS_ERR_OR_NULL(filp))
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
#endif



oal_bool_enum_uint8 hmac_set_ddr_freq_raw_etc(oal_uint8 uc_freq_type, oal_uint32 ul_freq_value)
{
#if 0
    pm_qos_update_request(g_pst_wifi_auto_ddr_etc, ul_freq_value);
#endif
    return 0;
}

oal_void hmac_wifi_auto_ddr_init_etc(oal_void)
{
#if 0
    g_pst_wifi_auto_ddr_etc = kmalloc(sizeof(struct pm_qos_request), GFP_KERNEL);
    if (g_pst_wifi_auto_ddr_etc == NULL)
    {
        OAL_IO_PRINT("[AUTODDR]pm_qos_request alloc memory failed.\n");
        return;
    }
    g_pst_wifi_auto_ddr_etc->pm_qos_class = 0;
    pm_qos_add_request(g_pst_wifi_auto_ddr_etc, PM_QOS_MEMORY_THROUGHPUT,
                       PM_QOS_MEMORY_THROUGHPUT_DEFAULT_VALUE);
#endif
    return;
}
oal_void hmac_wifi_auto_ddr_exit_etc(oal_void)
{
#if 0
    pm_qos_remove_request(g_pst_wifi_auto_ddr_etc);
    kfree(g_pst_wifi_auto_ddr_etc);
    g_pst_wifi_auto_ddr_etc = NULL;
#endif
}
#endif


oal_uint8 hmac_set_auto_freq_mod_etc(oal_uint8 uc_freq_enable)
{
    g_freq_lock_control_etc.uc_lock_mod = uc_freq_enable;

    /* 设置device是否使能 */
    if(FREQ_LOCK_ENABLE == uc_freq_enable)
    {
        /* 初始频率为idle */
        g_freq_lock_control_etc.uc_curr_lock_level = FREQ_IDLE;
        /* 期望频率为idle */
        g_freq_lock_control_etc.uc_req_lock_level = FREQ_IDLE;
        /* WIFI打开时,定时器参数更新 */
        g_freq_lock_control_etc.ul_pre_jiffies = jiffies;
    }
    else
    {
        /* 关闭WIFI时,将CPU频率调整为默认值 */
        mutex_lock(&g_freq_lock_control_etc.st_lock_freq_mtx);
        hmac_set_cpu_freq_raw_etc(SCALING_MIN_FREQ, g_ul_orig_cpu_min_freq);
        hmac_set_cpu_freq_raw_etc(SCALING_MAX_FREQ, g_ul_orig_cpu_max_freq);
        mutex_unlock(&g_freq_lock_control_etc.st_lock_freq_mtx);

        g_freq_lock_control_etc.uc_curr_lock_level = FREQ_BUTT;
        OAM_WARNING_LOG2(0,OAM_SF_ANY,"{hmac_set_auto_freq_mod_etc: freq,min[%d]max[%d]!}",
                       g_ul_orig_cpu_min_freq, g_ul_orig_cpu_max_freq);
    }

    return OAL_SUCC;
}

oal_void hmac_adjust_set_freq(oal_void)
{
    oal_uint8 uc_req_lock_level = g_freq_lock_control_etc.uc_req_lock_level;

    /* 相同则不需要配置系统文件 */
    if (uc_req_lock_level == g_freq_lock_control_etc.uc_curr_lock_level)
    {
        return;
    }

    OAM_WARNING_LOG2(0,OAM_SF_PWR,"{hmac_adjust_set_freq: freq [%d]to[%d]}",g_freq_lock_control_etc.uc_curr_lock_level,uc_req_lock_level);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if(hmac_is_device_ba_setup_etc())
    {
        hmac_set_cpu_freq_raw_etc(SCALING_MIN_FREQ,g_host_speed_freq_level_etc[uc_req_lock_level].ul_min_cpu_freq);
        hmac_set_ddr_freq_raw_etc(SCALING_MIN_FREQ,g_host_speed_freq_level_etc[uc_req_lock_level].ul_min_ddr_freq);
    }
    else
    {
        hmac_set_cpu_freq_raw_etc(SCALING_MIN_FREQ,g_host_no_ba_freq_level_etc[uc_req_lock_level].ul_min_cpu_freq);
        hmac_set_ddr_freq_raw_etc(SCALING_MIN_FREQ,g_host_no_ba_freq_level_etc[uc_req_lock_level].ul_min_ddr_freq);
    }
#endif

    g_freq_lock_control_etc.uc_curr_lock_level = uc_req_lock_level;
}

oal_uint8 hmac_get_freq_level_etc(oal_uint32 ul_speed)//根据吞吐率计算目标主频等级
{
    oal_uint8 level_idx;

    if(hmac_is_device_ba_setup_etc())
    {
        if (ul_speed <= g_host_speed_freq_level_etc[1].ul_speed_level)
        {
            level_idx = 0;
        }
        else if ((ul_speed > g_host_speed_freq_level_etc[1].ul_speed_level)
            && (ul_speed <= g_host_speed_freq_level_etc[2].ul_speed_level))
        {
            level_idx = 1;
        }
        else if ((ul_speed > g_host_speed_freq_level_etc[2].ul_speed_level)
            && (ul_speed <= g_host_speed_freq_level_etc[3].ul_speed_level))
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
        if (ul_speed <= g_host_no_ba_freq_level_etc[1].ul_speed_level)
        {
            level_idx = 0;
        }
        else if ((ul_speed > g_host_no_ba_freq_level_etc[1].ul_speed_level)
            && (ul_speed <= g_host_no_ba_freq_level_etc[2].ul_speed_level))
        {
            level_idx = 1;
        }
        else if ((ul_speed > g_host_no_ba_freq_level_etc[2].ul_speed_level)
            && (ul_speed <= g_host_no_ba_freq_level_etc[3].ul_speed_level))
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


oal_void hmac_adjust_freq_etc(oal_void)
{
    oal_uint8          uc_req_lock_level = 0; //目标主频等级
    oal_uint32         ul_cur_jiffies;
    oal_uint32         ul_sdio_dur_ms; //时间
    oal_uint32         ul_trx_total = 0;
    oal_uint32         ul_tx_total;
    oal_uint32         ul_rx_total;

    if(FREQ_LOCK_DISABLE == g_freq_lock_control_etc.uc_lock_mod)
    {
        return;
    }

    ul_trx_total = g_st_wifi_rxtx_total.ul_rx_pkts + g_st_wifi_rxtx_total.ul_tx_pkts;
    ul_tx_total  = g_st_wifi_rxtx_total.ul_tx_pkts;
    ul_rx_total  = g_st_wifi_rxtx_total.ul_rx_pkts;
    g_st_wifi_rxtx_total.ul_rx_pkts = 0;
    g_st_wifi_rxtx_total.ul_tx_pkts = 0;

    ul_cur_jiffies = jiffies; //jiffies是Linux内核中的一个全局变量，用来记录自系统启动以来产生的节拍的总数
    ul_sdio_dur_ms = OAL_JIFFIES_TO_MSECS(ul_cur_jiffies - g_freq_lock_control_etc.ul_pre_jiffies);
    g_freq_lock_control_etc.ul_pre_jiffies = ul_cur_jiffies;

    /* 如果当前统计时间不足定时器周期的一半,会导致统计PPS值偏大返回 */
    if ((0 == ul_sdio_dur_ms) || (ul_sdio_dur_ms < (WLAN_FREQ_TIMER_PERIOD >> 1)))
    {
        return;
    }

    //计算PPS
    g_freq_lock_control_etc.ul_total_sdio_pps = (ul_trx_total*1000)/ul_sdio_dur_ms;
    g_freq_lock_control_etc.ul_tx_pps         = (ul_tx_total*1000)/ul_sdio_dur_ms;
    g_freq_lock_control_etc.ul_rx_pps         = (ul_rx_total*1000)/ul_sdio_dur_ms;

    //OAM_WARNING_LOG4(0,OAM_SF_ANY,"{SDIO perform tx statistic: packet_rate = %lu pps, sumlen = %lu B, [use time] = %lu ms,g_adjust_count = %d!}",
        //g_freq_lock_control_etc.ul_total_sdio_pps , ul_trx_total, ul_sdio_dur_ms,g_freq_lock_control_etc.ul_adjust_count);
    //OAM_WARNING_LOG2(0,OAM_SF_ANY,"{SDIO perform tx statistic: tx pps  = %lu pps, rx pps = %lu pps}",
        //g_freq_lock_control_etc.ul_tx_pps, g_freq_lock_control_etc.ul_rx_pps);

    g_freq_lock_control_etc.uc_req_lock_level = hmac_get_freq_level_etc(g_freq_lock_control_etc.ul_total_sdio_pps);

    uc_req_lock_level = g_freq_lock_control_etc.uc_req_lock_level;
    if (uc_req_lock_level == g_freq_lock_control_etc.uc_curr_lock_level) //目标主频和当前主频相同，不需要调频
    {
        g_freq_lock_control_etc.ul_adjust_count = 0;
        return;
    }

    mutex_lock(&g_freq_lock_control_etc.st_lock_freq_mtx);

    if(uc_req_lock_level < g_freq_lock_control_etc.uc_curr_lock_level)
    {
        /*连续MAX_DEGRADE_FREQ_TIME_THRESHOLD后才降频，保证性能*/
        g_freq_lock_control_etc.ul_adjust_count++;

        if(0 != ul_trx_total) //当前数据流量不为零
        {
            if(g_freq_lock_control_etc.ul_adjust_count >= MAX_DEGRADE_FREQ_COUNT_THRESHOLD_SUCCESSIVE_10)//有包时连续100个周期都需要降频才降频
            {
                g_freq_lock_control_etc.ul_adjust_count = 0;
                /*在这里进行主频值的调整*/
                hmac_adjust_set_freq();
            }
        }
        else //没有包时连续3个周期都需要降频再降频
        {
            if(g_freq_lock_control_etc.ul_adjust_count >= MAX_DEGRADE_FREQ_COUNT_THRESHOLD_SUCCESSIVE_3)
            {
                g_freq_lock_control_etc.ul_adjust_count = 0;
                hmac_adjust_set_freq();
            }
        }
    }
    else
    {
           /*升频不等待，立即执行保证性能*/
           g_freq_lock_control_etc.ul_adjust_count = 0;
          hmac_adjust_set_freq();
    }

   mutex_unlock(&g_freq_lock_control_etc.st_lock_freq_mtx);
}


oal_void hmac_wifi_auto_freq_ctrl_init_etc(void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if(OAL_TRUE != g_freq_lock_control_etc.en_is_inited)
    {
        mutex_init(&g_freq_lock_control_etc.st_lock_freq_mtx);
        mutex_lock(&g_freq_lock_control_etc.st_lock_freq_mtx);

        hmac_get_cpu_freq_raw(SCALING_MIN_FREQ, &g_ul_orig_cpu_min_freq);
        hmac_get_cpu_freq_raw(SCALING_MAX_FREQ, &g_ul_orig_cpu_max_freq);

        hmac_wifi_auto_ddr_init_etc();

        OAM_WARNING_LOG4(0,OAM_SF_ANY,"{hmac_wifi_auto_freq_ctrl_init_etc g_ul_orig_cpu_min_freq = %d,g_ul_orig_cpu_max_freq = %d,g_ul_orig_ddr_max_freq = %d,g_ul_orig_ddr_max_freq = %d}",
            g_ul_orig_cpu_min_freq,g_ul_orig_cpu_max_freq,g_ul_orig_ddr_min_freq,g_ul_orig_ddr_max_freq);

        g_freq_lock_control_etc.en_is_inited = OAL_TRUE;

        mutex_unlock(&g_freq_lock_control_etc.st_lock_freq_mtx);
    }

    //OAL_INIT_WORK(&(g_freq_lock_control_etc.st_work), hmac_del_virtual_inf_worker_etc);

    //hmac_freq_timer_init(); //定时器初始化
#endif
}

oal_void hmac_wifi_auto_freq_ctrl_deinit_etc(void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    //hmac_freq_timer_deinit();

    mutex_lock(&g_freq_lock_control_etc.st_lock_freq_mtx);

    if (OAL_TRUE == g_freq_lock_control_etc.en_is_inited)
    {
        hmac_wifi_auto_ddr_exit_etc();
    }
    else
    {
        OAM_WARNING_LOG0(0,OAM_SF_ANY,"{hw_wifi_freq_ctrl_destroy freq lock has already been released!}");
    }
    //oal_cancel_work_sync(&(pst_hmac_vap->st_del_virtual_inf_worker));

    mutex_unlock(&g_freq_lock_control_etc.st_lock_freq_mtx);
    mutex_destroy(&g_freq_lock_control_etc.st_lock_freq_mtx);

    g_freq_lock_control_etc.en_is_inited = OAL_FALSE;
#endif
}

oal_void hmac_auto_freq_wifi_rx_stat(oal_uint32 ul_pkt_count)
{
    g_st_wifi_rxtx_total.ul_rx_pkts += ul_pkt_count;
}

oal_void hmac_auto_freq_wifi_tx_stat(oal_uint32 ul_pkt_count)
{
    g_st_wifi_rxtx_total.ul_tx_pkts += ul_pkt_count;
}
#endif

oal_void hmac_auto_freq_wifi_rx_bytes_stat(oal_uint32 ul_pkt_bytes)
{
    g_st_wifi_rxtx_total.ul_rx_bytes += ul_pkt_bytes;
}

oal_void hmac_auto_freq_wifi_tx_bytes_stat(oal_uint32 ul_pkt_bytes)
{
    g_st_wifi_rxtx_total.ul_tx_bytes += ul_pkt_bytes;
}

oal_bool_enum_uint8 hmac_wifi_rx_is_busy(oal_void)
{
    return g_st_wifi_load.en_wifi_rx_busy;
}

oal_void hmac_adjust_set_irq(oal_uint8 uc_cpu_id)
{
    if (uc_cpu_id == g_freq_lock_control_etc.uc_cur_irq_cpu)
    {
        return;
    }

    g_freq_lock_control_etc.uc_cur_irq_cpu = uc_cpu_id;

    //OAM_WARNING_LOG1(0,OAM_SF_PWR,"{hmac_adjust_set_irq: irq to cpu[%d]}",uc_cpu_id);
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    hi110x_hcc_dev_bindcpu(uc_cpu_id >= WLAN_IRQ_AFFINITY_BUSY_CPU ? 1 : 0);

#if defined(CONFIG_ARCH_HISI)
#ifdef CONFIG_NR_CPUS
#if CONFIG_NR_CPUS > OAL_BUS_HPCPU_NUM
    if( g_st_rxdata_thread_etc.pst_rxdata_thread)
    {
        struct cpumask fast_cpus;
        if(uc_cpu_id >= WLAN_IRQ_AFFINITY_BUSY_CPU)
        {
            hisi_get_fast_cpus(&fast_cpus);
            cpumask_clear_cpu(OAL_BUS_HPCPU_NUM, &fast_cpus);
            //cpumask_clear_cpu(OAL_BUS_HPCPU_NUM_1, &fast_cpus);
            set_cpus_allowed_ptr( g_st_rxdata_thread_etc.pst_rxdata_thread , &fast_cpus);
        }
        else
        {
            set_cpus_allowed_ptr( g_st_rxdata_thread_etc.pst_rxdata_thread , cpumask_of(1));
        }
    }
#endif
#endif
#endif

#endif
}

#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET

oal_void hmac_update_pk_mode_info(oal_uint8 *puc_vap_id)
{
    oal_uint8            uc_vap_id;
    mac_vap_stru        *pst_mac_vap;
    mac_user_stru       *pst_mac_user;
    oal_dlist_head_stru *pst_entry;
    oal_dlist_head_stru *pst_dlist_tmp;

    g_st_wifi_pk_mode_status.en_curr_bw_cap = WLAN_BW_CAP_BUTT;
    g_st_wifi_pk_mode_status.en_curr_protocol_cap       = WLAN_PROTOCOL_CAP_BUTT;
    g_st_wifi_pk_mode_status.en_curr_num_spatial_stream = WLAN_SINGLE_NSS;

    for (uc_vap_id = 0; uc_vap_id < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_vap_id++)
    {
        pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(uc_vap_id);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_update_pk_mode_info::pst_mac_vap(%d) is null.}", uc_vap_id);
            continue;
        }

        if (!IS_LEGACY_STA(pst_mac_vap))
        {
            continue;
        }

        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_dlist_tmp, &(pst_mac_vap->st_mac_user_list_head))
        {
            pst_mac_user      = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
            if (OAL_PTR_NULL == pst_mac_user)
            {
                OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_update_pk_mode_info::pst_user_tmp null.}");
                continue;
            }

            /* 从用户的信息中更新目前带宽和协议模式 */
            /* TBD 此设计目前仅适用支持一个LEGACY_STA且LEGACY_STA仅一个业务用户，若规格有修改此函数需要重构 */
            g_st_wifi_pk_mode_status.en_curr_bw_cap             = pst_mac_user->en_bandwidth_cap;
            g_st_wifi_pk_mode_status.en_curr_num_spatial_stream = pst_mac_user->en_avail_num_spatial_stream;

            switch(pst_mac_user->en_protocol_mode)
            {
                case WLAN_LEGACY_11A_MODE:
                case WLAN_LEGACY_11B_MODE:
                case WLAN_LEGACY_11G_MODE:
                case WLAN_MIXED_ONE_11G_MODE:
                case WLAN_MIXED_TWO_11G_MODE:
                    g_st_wifi_pk_mode_status.en_curr_protocol_cap  = WLAN_PROTOCOL_CAP_LEGACY;
                    break;
                case WLAN_HT_MODE:
                case WLAN_HT_ONLY_MODE:
                case WLAN_HT_11G_MODE:
                    g_st_wifi_pk_mode_status.en_curr_protocol_cap  = WLAN_PROTOCOL_CAP_HT;
                    break;
                case WLAN_VHT_MODE:
                case WLAN_VHT_ONLY_MODE:
                    g_st_wifi_pk_mode_status.en_curr_protocol_cap  = WLAN_PROTOCOL_CAP_VHT;
                    break;
#ifdef _PRE_WLAN_FEATURE_11AX
                case WLAN_HE_MODE:
                    g_st_wifi_pk_mode_status.en_curr_protocol_cap  = WLAN_PROTOCOL_CAP_HE;
                    break;
#endif
                case WLAN_PROTOCOL_BUTT:
                default:
                     g_st_wifi_pk_mode_status.en_curr_protocol_cap  = WLAN_PROTOCOL_CAP_BUTT;
                     break;
            }

            *puc_vap_id = uc_vap_id;
        }

    }

    return;
}


oal_void hmac_update_pk_mode(oal_uint32 ul_tx_throughput,
                                        oal_uint32 ul_rx_throughput,
                                        oal_uint32 ul_dur_time)
{
    oal_bool_enum_uint8          en_curr_pk_mode = OAL_FALSE;
    oal_uint8                    uc_vap_id;
    mac_cfg_pk_mode_stru         st_pk_mode_info;
    mac_vap_stru                *pst_mac_vap = OAL_PTR_NULL;
    oal_uint32                   ul_ret;
    oal_uint32                   ul_txrx_throughput;
    oal_uint32                   ul_pk_mode_th;
    mac_device_stru             *pst_mac_device;

    if (OAL_FALSE == g_en_pk_mode_swtich)
    {
        return;
    }
    /* 避免低功耗时ul_dur_time统计时间变短，导致此处直接return，而无法退出PK MODE */
#if 0
    /* ul_dur_time入参小于检测间隔时间不做pk mode的更新 */
    if (ul_dur_time < (WLAN_FREQ_TIMER_PERIOD * WLAN_THROUGHPUT_STA_PERIOD))
    {
        return;
    }
#endif
    pst_mac_device = mac_res_get_dev_etc(0);
    /* 如果非单VAP,则不开启硬件聚合 */
    if (1 != mac_device_calc_up_vap_num_etc(pst_mac_device))
    {
        return;
    }

    /* 更新相关的信息 */
    hmac_update_pk_mode_info(&uc_vap_id);

    /* 非有效模式，不进行吞吐计算 */
    if ((WLAN_BW_CAP_BUTT  == g_st_wifi_pk_mode_status.en_curr_bw_cap)||
    (WLAN_PROTOCOL_CAP_BUTT == g_st_wifi_pk_mode_status.en_curr_protocol_cap))
    {
        return;
    }

    /* 计算吞吐率: rxtx吞吐之和 / 统计的时间间隔(单位s) */
    ul_txrx_throughput = ul_tx_throughput + ul_rx_throughput;

    /* 依据门限值判定pk mode状态 */
    if (OAL_TRUE == g_st_wifi_pk_mode_status.en_is_pk_mode)
    {
        ul_pk_mode_th = (g_st_pk_mode_low_th_table[g_st_wifi_pk_mode_status.en_curr_protocol_cap][g_st_wifi_pk_mode_status.en_curr_bw_cap]) * (1 + g_st_wifi_pk_mode_status.en_curr_num_spatial_stream);
        en_curr_pk_mode = (ul_txrx_throughput < ul_pk_mode_th) ? OAL_FALSE : OAL_TRUE;
    }
    else
    {
        ul_pk_mode_th = (g_st_pk_mode_high_th_table[g_st_wifi_pk_mode_status.en_curr_protocol_cap][g_st_wifi_pk_mode_status.en_curr_bw_cap]) * (1 + g_st_wifi_pk_mode_status.en_curr_num_spatial_stream);
        en_curr_pk_mode = (ul_txrx_throughput > ul_pk_mode_th) ? OAL_TRUE : OAL_FALSE;
    }
    /* dmac逻辑挪上来，只有下行时才进入pk mode */
    if((OAL_TRUE == en_curr_pk_mode) && (ul_rx_throughput < ul_tx_throughput))
    {
        en_curr_pk_mode = OAL_FALSE;
    }
    /* PK mode状态未变化，不更新数值 */
    if (en_curr_pk_mode == g_st_wifi_pk_mode_status.en_is_pk_mode)
    {
        return;
    }

    pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(uc_vap_id);
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_update_pk_mode::pst_mac_vap(%d) is null.}", uc_vap_id);
        return;
    }
    if ((WLAN_VHT_MODE != pst_mac_vap->en_protocol) &&
        (WLAN_VHT_ONLY_MODE != pst_mac_vap->en_protocol)
#ifdef _PRE_WLAN_FEATURE_11AX
         &&(WLAN_HE_MODE != pst_mac_vap->en_protocol)
#endif
        )
    {
        return;
    }
    OAM_WARNING_LOG4(0,OAM_SF_ANY,"{hmac_update_pk_mode: pk_mode changed (%d) -> (%d),tx_throughput[%d],rx_throughput[%d]!}",
            g_st_wifi_pk_mode_status.en_is_pk_mode,en_curr_pk_mode,ul_tx_throughput,ul_rx_throughput);
    g_st_wifi_pk_mode_status.en_is_pk_mode = en_curr_pk_mode;
    g_st_wifi_pk_mode_status.ul_rx_bytes   = ul_rx_throughput;
    g_st_wifi_pk_mode_status.ul_tx_bytes   = ul_tx_throughput;
    g_st_wifi_pk_mode_status.ul_dur_time   = ul_dur_time;


    /***************************************************************************
        抛事件到DMAC层, 同步VAP最新状态到DMAC
    ***************************************************************************/
    st_pk_mode_info.ul_rx_bytes = g_st_wifi_pk_mode_status.ul_rx_bytes;
    st_pk_mode_info.ul_tx_bytes = g_st_wifi_pk_mode_status.ul_tx_bytes;
    st_pk_mode_info.ul_dur_time = g_st_wifi_pk_mode_status.ul_dur_time;
    st_pk_mode_info.en_is_pk_mode  = g_st_wifi_pk_mode_status.en_is_pk_mode;
    st_pk_mode_info.en_curr_bw_cap = g_st_wifi_pk_mode_status.en_curr_bw_cap;
    st_pk_mode_info.en_curr_protocol_cap       = g_st_wifi_pk_mode_status.en_curr_protocol_cap;
    st_pk_mode_info.en_curr_num_spatial_stream = g_st_wifi_pk_mode_status.en_curr_num_spatial_stream;

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SYNC_PK_MODE, OAL_SIZEOF(mac_cfg_pk_mode_stru), (oal_uint8 *)(&st_pk_mode_info));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_update_pk_mode::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }


    return;
}
#endif


void hmac_perform_calc_throughput(oal_uint32 ul_tx_throughput_mbps,
                                  oal_uint32 ul_rx_throughput_mbps,
                                  oal_uint32 ul_trx_pps)
{
    oal_uint32 ul_limit_throughput_high;
    oal_uint32 ul_limit_throughput_low;
    oal_uint32   ul_limit_pps_high = 0;
    oal_uint32   ul_limit_pps_low = 0;

    if (OAL_FALSE == g_freq_lock_control_etc.en_irq_affinity)
    {
        return;
    }
    /* 每秒吞吐量门限 */
    if ((0 != g_freq_lock_control_etc.us_throughput_irq_high) && (0 != g_freq_lock_control_etc.us_throughput_irq_low))
    {
        ul_limit_throughput_high = g_freq_lock_control_etc.us_throughput_irq_high;
        ul_limit_throughput_low  = g_freq_lock_control_etc.us_throughput_irq_low;
    }
    else
    {
        ul_limit_throughput_high = WLAN_IRQ_THROUGHPUT_THRESHOLD_HIGH;
        ul_limit_throughput_low  = WLAN_IRQ_THROUGHPUT_THRESHOLD_LOW;
    }

    /* 每秒PPS门限 */
    if ((0 != g_freq_lock_control_etc.ul_irq_pps_high) && (0 != g_freq_lock_control_etc.ul_irq_pps_low))
    {
        ul_limit_pps_high = g_freq_lock_control_etc.ul_irq_pps_high;
        ul_limit_pps_low  = g_freq_lock_control_etc.ul_irq_pps_low;
    }
    else
    {
        ul_limit_pps_high = WLAN_IRQ_PPS_THRESHOLD_HIGH;
        ul_limit_pps_low  = WLAN_IRQ_PPS_THRESHOLD_LOW;
    }

    /* 高于200M时切大核，低于150M切小核 */
    if ((ul_tx_throughput_mbps > ul_limit_throughput_high)
        || (ul_rx_throughput_mbps >  ul_limit_throughput_high)
        || (ul_trx_pps >= ul_limit_pps_high))
    {
        g_freq_lock_control_etc.uc_req_irq_cpu = WLAN_IRQ_AFFINITY_BUSY_CPU;
    }
    else if((ul_tx_throughput_mbps < ul_limit_throughput_low)
           && (ul_rx_throughput_mbps <  ul_limit_throughput_low)
           && (ul_trx_pps < ul_limit_pps_low))
    {
        g_freq_lock_control_etc.uc_req_irq_cpu = WLAN_IRQ_AFFINITY_IDLE_CPU;
    }
    else
    {
        return;
    }

    if (g_freq_lock_control_etc.uc_req_irq_cpu != g_freq_lock_control_etc.uc_cur_irq_cpu)
    {
        OAM_WARNING_LOG4(0,OAM_SF_ANY,"{hmac_perform_calc_throughput: rx = %d , tx = %d, pps_high = %d, pps low = %d!}",
            ul_rx_throughput_mbps , ul_tx_throughput_mbps,ul_limit_pps_high,ul_limit_pps_low);
        OAM_WARNING_LOG4(0,OAM_SF_ANY,"{hmac_perform_calc_throughput: req cpu id[%d],cur cpu id[%d],high_th = %d,low_th = %d!}",
              g_freq_lock_control_etc.uc_req_irq_cpu, g_freq_lock_control_etc.uc_cur_irq_cpu,ul_limit_throughput_high,ul_limit_throughput_low);
        OAM_WARNING_LOG3(0,OAM_SF_ANY,"{hmac_perform_calc_throughput: rx pps = %d , tx pps = %d, trx pps = %d,!}",
            g_freq_lock_control_etc.ul_rx_pps, g_freq_lock_control_etc.ul_tx_pps,ul_trx_pps);
    }

    hmac_adjust_set_irq(g_freq_lock_control_etc.uc_req_irq_cpu);

}


void hmac_adjust_throughput(oal_void)
{
    oal_uint32 ul_tx_throughput      = 0;
    oal_uint32 ul_rx_throughput      = 0;
    oal_uint32 ul_dur_time;
    oal_uint32 ul_tx_throughput_mbps = 0;
    oal_uint32 ul_rx_throughput_mbps = 0;

    /* 循环次数统计 */
    g_freq_lock_control_etc.uc_timer_cycles++;

    ul_dur_time = OAL_JIFFIES_TO_MSECS(jiffies - g_freq_lock_control_etc.ul_pre_time);

    /* 小于一个周期,不统计 */
    if (ul_dur_time < WLAN_FREQ_TIMER_PERIOD - 10)
    {
        return;
    }

    /* 超过一定时常则不统计,排除定时器异常 */
    if (ul_dur_time > ((WLAN_FREQ_TIMER_PERIOD * WLAN_THROUGHPUT_STA_PERIOD) << 2))
    {
        g_freq_lock_control_etc.ul_pre_time = jiffies;
        g_freq_lock_control_etc.uc_timer_cycles = 0;

        g_st_wifi_rxtx_total.ul_rx_bytes = 0;
        g_st_wifi_rxtx_total.ul_tx_bytes = 0;

        return;
    }


    /* 总字节数 */
    ul_rx_throughput = g_st_wifi_rxtx_total.ul_rx_bytes;
    ul_tx_throughput = g_st_wifi_rxtx_total.ul_tx_bytes;
    if(0 != ul_dur_time)
    {
        ul_tx_throughput_mbps = (ul_tx_throughput >> 7) / ul_dur_time;
        ul_rx_throughput_mbps = (ul_rx_throughput >> 7) / ul_dur_time;
    }

    /* Wi-Fi业务负载标记 */
    if (ul_rx_throughput_mbps <= WLAN_THROUGHPUT_LOAD_LOW)
    {
        g_st_wifi_load.en_wifi_rx_busy = OAL_FALSE;
    }
    else
    {
        g_st_wifi_load.en_wifi_rx_busy = OAL_TRUE;
    }

#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
    /* AMSDU+AMPDU聚合切换入口 */
    hmac_tx_amsdu_ampdu_switch(ul_tx_throughput_mbps);
#endif
    hmac_tx_small_amsdu_switch(ul_rx_throughput_mbps, g_freq_lock_control_etc.ul_tx_pps);

#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
    hmac_tx_tcp_ack_buf_switch(ul_rx_throughput_mbps);
#endif

#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
    /* 聚合软硬件切换入口,每个周期探测一次 */
    hmac_tx_ampdu_switch(ul_tx_throughput_mbps);
#endif
#ifdef _PRE_WLAN_TCP_OPT
    hmac_tcp_ack_filter_switch(ul_rx_throughput_mbps);
#endif
    /* 根据吞吐量，判断是否需要bypass 外置LNA */
    hmac_rx_dyn_bypass_extlna_switch(ul_tx_throughput_mbps, ul_rx_throughput_mbps);


    /* irq 2s 探测一次 */
    if (g_freq_lock_control_etc.uc_timer_cycles < WLAN_THROUGHPUT_STA_PERIOD)
    {
        return;
    }
    g_freq_lock_control_etc.uc_timer_cycles = 0;

    /* 2s周期清零一次 */
    g_st_wifi_rxtx_total.ul_rx_bytes = 0;
    g_st_wifi_rxtx_total.ul_tx_bytes = 0;

    g_freq_lock_control_etc.ul_pre_time = jiffies;

#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET
    /* 更新pk mode */
    hmac_update_pk_mode(ul_tx_throughput_mbps, ul_rx_throughput_mbps, ul_dur_time);
#endif
    /* 中断动态调整 */
    hmac_perform_calc_throughput(ul_tx_throughput_mbps, ul_rx_throughput_mbps,g_freq_lock_control_etc.ul_total_sdio_pps);

}


oal_void hmac_wlan_freq_wdg_timeout(oal_void)
{
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    /* CPU调频 */
    hmac_adjust_freq_etc();
#endif
    /* 吞吐统计 */
    hmac_adjust_throughput();

}



oal_void  hmac_freq_timer_init(oal_void)
{
    /*如果这个定时器已经注册成功，则不能再次被注册，不然后发生错误！*/
    if (OAL_TRUE == g_freq_lock_control_etc.hmac_freq_timer.en_is_registerd)
    {
        return;
    }

    g_freq_lock_control_etc.ul_pre_time = jiffies;
    /* uc_timer_cycles 无需清零。避免停止跑流进入低功耗后，大小核切换、PK mode判断无法恢复问题 */
    //g_freq_lock_control_etc.uc_timer_cycles = 0;
    /* 清空统计 */
    OAL_MEMZERO(&g_st_wifi_rxtx_total, OAL_SIZEOF(g_st_wifi_rxtx_total));

    FRW_TIMER_CREATE_TIMER(&g_freq_lock_control_etc.hmac_freq_timer, //pst_timeout
                        (oal_void*)hmac_wlan_freq_wdg_timeout,   //p_timeout_func
                        WLAN_FREQ_TIMER_PERIOD,                 //ul_timeout
                        OAL_PTR_NULL,                          // p_timeout_arg
                        OAL_TRUE,                             //en_is_periodic
                        OAM_MODULE_ID_HMAC,0 );             //en_module_id && ul_core_id

    //OAM_WARNING_LOG1(0,OAM_SF_ANY,"{hmac_freq_timer_init: throughput timer period[%d]ms enabled!}",WLAN_FREQ_TIMER_PERIOD);
}


oal_void  hmac_freq_timer_deinit(oal_void)
{
    /*定时器没有注册，就不用删除了，不然会发生错误！*/
    if (OAL_FALSE == g_freq_lock_control_etc.hmac_freq_timer.en_is_registerd)
    {
        return;
    }

    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&g_freq_lock_control_etc.hmac_freq_timer);
}

oal_void  hmac_wifi_pm_state_notify(oal_bool_enum_uint8 en_wake_up)
{
    if (OAL_TRUE == en_wake_up)
    {
        /* WIFI唤醒,启动吞吐量统计定时器 */
        hmac_freq_timer_init();
    }
    else
    {
        /* WIFI睡眠,关闭吞吐量统计定时器 */
        hmac_freq_timer_deinit();
    }
}


oal_void  hmac_wifi_state_notify(oal_bool_enum_uint8 en_wifi_on)
{
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    if (OAL_TRUE == en_wifi_on)
    {
        /* WIFI上电,启动吞吐量统计定时器,打开调频功能 */
        hmac_freq_timer_init();
        hmac_set_device_freq_mode_etc(FREQ_LOCK_ENABLE);
    }
    else
    {
        /* WIFI下电,关闭吞吐量统计定时器,关闭调频功能 */
        hmac_freq_timer_deinit();
        hmac_set_device_freq_mode_etc(FREQ_LOCK_DISABLE);
    }
#endif
}
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

