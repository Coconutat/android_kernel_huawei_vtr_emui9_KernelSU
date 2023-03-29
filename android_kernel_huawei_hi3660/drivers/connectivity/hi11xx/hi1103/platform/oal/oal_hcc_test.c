
#define HISI_LOG_TAG "[HCC_TEST]"
#include "oal_hcc_host_if.h"
#include "oal_ext_if.h"
#include "plat_pm_wlan.h"
#include "plat_pm.h"

#include "hisi_ini.h"

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#include "oal_pcie_host.h"
#include "oal_pcie_linux.h"
#endif
#if defined(_PRE_CONFIG_GPIO_TO_SSI_DEBUG)
#include "board.h"
#endif
#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
#include "oal_kernel_file.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/host.h>
typedef struct _hcc_test_data_{
    oal_int32                        mode_idx;
    oal_int32                      pkt_rcvd;/* packet received */
    oal_int32                      pkt_sent;/* packet sent */
    oal_int32                      pkt_gen; /* packet generate */
    oal_int32                      pkt_len; /* packet  length */
    oal_ulong                      trans_timeout;/*msec*/
    oal_uint64                      total_rcvd_bytes;
    oal_uint64                      total_sent_bytes;
    oal_uint64                      trans_time;
    oal_uint64                      trans_time_us;
    oal_uint64                      throughput;
    hsdio_trans_test_info           trans_info;
}hcc_test_data;

struct  hcc_test_event {
    oal_mutex_stru                mutex_lock;/* sdio test task lock */
    oal_int32                     errorno;

    struct workqueue_struct    *test_workqueue;
    struct work_struct          test_work;
    hcc_test_data     test_data;

    /* hcc perf started,for data transfer */
    oal_int32                      started;

    oal_int32                      rx_stop;

    /* hcc perf statistic */
    ktime_t                     start_time;
    /*The last update time*/
    ktime_t                     last_time;

    /*To hcc test sync*/
    oal_completion test_done;
    oal_completion test_trans_done;

    hcc_queue_type hcc_test_queue;

    oal_uint8      pad_payload;

    oal_uint8       test_value;
    oal_uint8       verified;

    oal_ulong       start_tick;
    oal_ulong       tick_timeout;

    /*sdio test thread and seam*/
};

struct hcc_test_stru {
    const char *mode;
    oal_uint16      start_cmd;
    const char *mode_desc;
    //oal_int32   send_mode;/*1:*/
};

/*
 * 2 Global Variable Definition
 */
OAL_STATIC oal_int32 g_test_force_stop = 0;
struct hcc_test_event  *g_hcc_test_event_etc = NULL;

struct hcc_test_stru g_hcc_test_stru_etc[HCC_TEST_CASE_COUNT] = {
    [HCC_TEST_CASE_TX] = {"tx", HCC_TEST_CMD_START_TX, "HCC_TX_MODE"},
    [HCC_TEST_CASE_RX] = {"rx", HCC_TEST_CMD_START_RX, "HCC_RX_MODE"},
    [HCC_TEST_CASE_LOOP] = {"loop", HCC_TEST_CMD_START_LOOP, "HCC_LOOP_MODE"},
};

#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
oal_uint32 wifi_panic_debug_etc = 0;
module_param(wifi_panic_debug_etc, int, S_IRUGO|S_IWUSR);
OAL_STATIC LIST_HEAD(wifi_panic_log_head);
#endif

oal_int32 ft_pcie_test_wifi_runtime = 5000;/*5 seconds*/
oal_int32 ft_sdio_test_wifi_runtime = 5000; /*5 seconds*/
oal_int32 ft_pcie_test_min_throught = 1613;/*1600Mbps*/
oal_int32 ft_pcie_test_retry_cnt    = 3;/*retry 3 times*/
oal_int32 ft_ip_test_cpu_max_freq    = 1;/*retry 3 times*/
oal_int32 hcc_test_performance_mode = 0;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
module_param(ft_pcie_test_wifi_runtime, int, S_IRUGO | S_IWUSR);
module_param(ft_sdio_test_wifi_runtime, int, S_IRUGO | S_IWUSR);
module_param(ft_pcie_test_min_throught, int, S_IRUGO | S_IWUSR);
module_param(ft_pcie_test_retry_cnt, int, S_IRUGO | S_IWUSR);
module_param(ft_ip_test_cpu_max_freq, int, S_IRUGO | S_IWUSR);
module_param(hcc_test_performance_mode, int, S_IRUGO | S_IWUSR);
#endif

oal_int32 conn_test_hcc_chann_switch(char* new_dev);
#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
extern oal_int32 hcc_assem_info_print_etc(struct hcc_handler* hcc,char* buf, oal_int32 buf_len);
extern oal_int32 hcc_flowctrl_info_print_etc(struct hcc_handler* hcc,char* buf, oal_int32 buf_len);
extern oal_int32 hcc_queues_len_info_print_etc(struct hcc_handler* hcc,char* buf, oal_int32 buf_len);
extern oal_int32 hcc_queues_pkts_info_print_etc(struct hcc_handler* hcc,char* buf, oal_int32 buf_len);
extern oal_int32 hsdio_sysfs_info_print_etc(struct oal_sdio *hi_sdio,char* buf, oal_int32 buf_len);
OAL_STATIC DECLARE_WIFI_PANIC_STRU(hcc_panic_assem_info,hcc_assem_info_print_etc);
OAL_STATIC DECLARE_WIFI_PANIC_STRU(hcc_panic_flowctrl,hcc_flowctrl_info_print_etc);
OAL_STATIC DECLARE_WIFI_PANIC_STRU(hcc_panic_queues_len,hcc_queues_len_info_print_etc);
OAL_STATIC DECLARE_WIFI_PANIC_STRU(hcc_panic_queues_pkts,hcc_queues_pkts_info_print_etc);
OAL_STATIC DECLARE_WIFI_PANIC_STRU(sdio_panic,hsdio_sysfs_info_print_etc);
#endif

OAL_STATIC oal_kobject* g_conn_syfs_hcc_object = NULL;
OAL_STATIC oal_int32 hcc_test_normal_start(oal_uint16 test_type);
OAL_STATIC oal_int32 hcc_send_test_cmd(oal_uint8* cmd,oal_int32 cmd_len);
OAL_STATIC oal_int32 hcc_test_start(oal_uint16 start_cmd);
int hcc_test_set_case_etc(hcc_test_data *data);

/*
 * 3 Function Declare
 */
#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
oal_void hwifi_panic_log_register_etc(hwifi_panic_log* pst_log, oal_void* data)
{
    if(OAL_UNLIKELY(!pst_log))
    {
         OAL_IO_PRINT("%s error: pst_log is null",__FUNCTION__);
         return;
    };

    pst_log->data = data;
    list_add_tail(&pst_log->list, &wifi_panic_log_head);
    OAL_IO_PRINT("Log module %s added![%pF]\n", pst_log->name,(void*)_RET_IP_);
}
oal_module_symbol(hwifi_panic_log_register_etc);

/*should't use dynamic mem when panic*/
OAL_STATIC char g_panic_mem[PAGE_SIZE];
void hwifi_panic_log_dump_etc(char* print_level)
{
    oal_uint32 buf_size = PAGE_SIZE;
    char* pst_buf = NULL;
    hwifi_panic_log* pst_log = NULL;
    struct list_head* head = &wifi_panic_log_head;

    print_level = print_level?:"";

    pst_buf = g_panic_mem;

    printk("%sdump wifi info when panic\n",print_level);
    list_for_each_entry(pst_log,head,list)
    {
        if(NULL == pst_log)
        {
            printk(KERN_ERR"hwifi_panic_log_dump_etc:pst_log is null\n");
            return;
        }
        pst_buf[0] = '\0';
        printk("%s[%s]:\n",print_level, pst_log->name);
        pst_log->cb(pst_log->data,pst_buf,buf_size);
        printk("%s%s\n",print_level,pst_buf);
        printk("%s\n",print_level);
    }
    printk("%s\n",print_level);
}
oal_module_symbol(hwifi_panic_log_dump_etc);
#endif

oal_int32 hcc_flowctrl_info_print_etc(struct hcc_handler* hcc,char* buf, oal_int32 buf_len)
{
    oal_int32 ret = 0;
    struct oal_sdio* hi_sdio;
    if(NULL == hcc)
    {
        return ret;
    }

#ifdef CONFIG_MMC
    hi_sdio = oal_get_sdio_default_handler();
    if(hi_sdio && hi_sdio->func->card->host->claimer)
        ret +=  snprintf(buf + ret , buf_len - ret,"claim host name:%s\n", hi_sdio->func->card->host->claimer->comm);
#else
    OAL_REFERENCE(hi_sdio);
#endif
    ret +=  snprintf(buf + ret , buf_len - ret,"hcc state:%s[%d]\n",
                (HCC_OFF == oal_atomic_read(&hcc->state))
                ?"off":"on", oal_atomic_read(&hcc->state));
    ret +=  snprintf(buf + ret , buf_len - ret,"flowctrl flag:%s\n",
                (D2H_MSG_FLOWCTRL_OFF==hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_flag)
                ?"off":"on");
    ret +=  snprintf(buf + ret , buf_len - ret,"flowctrl on:%d\n", hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_on_count);
    ret +=  snprintf(buf + ret , buf_len - ret,"flowctrl off:%d\n", hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_off_count);
    ret +=  snprintf(buf + ret , buf_len - ret,"flowctrl reset count:%d\n", hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_reset_count);
    ret +=  snprintf(buf + ret , buf_len - ret,"flowctrl hi update count:%d,hi credit value:%u\n",
                        hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_hipri_update_count,
                        hcc->hcc_transer_info.tx_flow_ctrl.uc_hipriority_cnt);
    return ret;
}

oal_int32 hcc_assem_info_print_etc(struct hcc_handler* hcc,char* buf, oal_int32 buf_len)
{
    oal_int32 ret = 0;
    oal_int32 i;
    oal_int32 total = 0;
    oal_int32 count = 0;
    if(NULL == hcc)
    {
        return ret;
    }
    for(i = 0; i < HCC_TX_ASSEM_INFO_MAX_NUM; i++)
    {
        if(hcc->hcc_transer_info.tx_assem_info.info[i])
        {
            if(hcc->hcc_transer_info.tx_assem_info.info[i])
            {
                ret = snprintf(buf + count, buf_len - count, "[tx][%2d]:%-20u pkts\n",i,hcc->hcc_transer_info.tx_assem_info.info[i]);
                if (0 >= ret)
                {
                    return count;
                }
                count += ret;
            }
            total += (oal_int32)hcc->hcc_transer_info.tx_assem_info.info[i]*(i==0 ? 1:i);
        }
    }
    if(total)
    {
        ret = snprintf(buf + count, buf_len - count, "hcc tx total:%d!\n", total);
        if (0 >= ret)
        {
            return count;
        }
        count += ret;
    }

    total = 0;

    for(i = 0; i < HCC_RX_ASSEM_INFO_MAX_NUM; i++)
    {
        if(hcc->hcc_transer_info.rx_assem_info.info[i])
        {
            total += (oal_int32)hcc->hcc_transer_info.rx_assem_info.info[i]*(i==0 ? 1:i);
            if(hcc->hcc_transer_info.rx_assem_info.info[i])
            {
                ret = snprintf(buf + count, buf_len - count, "[rx][%2d]:%-20u pkts\n",i,hcc->hcc_transer_info.rx_assem_info.info[i]);
                if (0 >= ret)
                {
                    return count;
                }
                count += ret;
            }
        }
    }

    if(total)
    {
        ret = snprintf(buf + count, buf_len - count, "hcc rx total:%d!\n", total);
        if (0 >= ret)
        {
            return count;
        }
        count += ret;
    }
    return count;
}

oal_int32 hcc_queues_pkts_info_print_etc(struct hcc_handler* hcc,char* buf, oal_int32 buf_len)
{
    oal_int32 ret = 0;
    oal_int32 count = 0;
    int i,j;
    oal_uint64 total;
    hcc_trans_queue *pst_hcc_queue;
    if(NULL == hcc)
    {
        return ret;
    }
    ret = snprintf(buf + count, buf_len - count, "queues_pkts_info_show [tx_seq:%d]\n", atomic_read(&hcc->tx_seq));
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    for(i = 0; i < HCC_DIR_COUNT; i++)
    {
        total = 0;
        ret = snprintf(buf + count, buf_len - count, "transfer dir:%s\n",HCC_GET_CHAN_STRING(i));
        if (0 >= ret)
        {
            return count;
        }
        count += ret;

        for(j = 0; j < HCC_QUEUE_COUNT; j++ )
        {
            pst_hcc_queue = &hcc->hcc_transer_info.hcc_queues[i].queues[j];
            if(pst_hcc_queue->total_pkts || pst_hcc_queue->loss_pkts)
            {
                ret =  snprintf(buf + count, buf_len - count, "queue:%4d,pkts num:%10u,loss num:%10u\n",j,
                                pst_hcc_queue->total_pkts,
                                pst_hcc_queue->loss_pkts);
                if (0 >= ret)
                {
                    return count;
                }
                count += ret;
            }
            total += pst_hcc_queue->total_pkts;
        }
        if(total)
        {
            ret =  snprintf(buf + count, buf_len - count, "total:%llu\n", total);
            if (0 >= ret)
            {
                return count;
            }
            count += ret;
        }
    }

    ret = snprintf(buf + count, buf_len - count, "flow ctrl info show\n");
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    for(j = 0; j < HCC_QUEUE_COUNT;j++)
    {
        pst_hcc_queue = &hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[j];
        ret = snprintf(buf + count, buf_len - count, "tx queue:%4d,%s,low wl:%u, high wl:%u\n",
                        j,
                        (pst_hcc_queue->flow_ctrl.is_stopped == OAL_FALSE) ? "run ":"stop",
                        pst_hcc_queue->flow_ctrl.low_waterline,
                        pst_hcc_queue->flow_ctrl.high_waterline);
        if (0 >= ret)
        {
            return count;
        }
        count += ret;
    }

#if 1
    //for(j = 0; j < HCC_QUEUE_COUNT;j++)
    {
        pst_hcc_queue = &hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[DATA_LO_QUEUE];
        ret = snprintf(buf + count, buf_len - count, "rx queue:%4d,low wl:%u,high wl:%u, enabled:%d\n",
                        DATA_LO_QUEUE,
                        pst_hcc_queue->flow_ctrl.low_waterline,
                        pst_hcc_queue->flow_ctrl.high_waterline, pst_hcc_queue->flow_ctrl.enable);
        if (0 >= ret)
        {
            return count;
        }
        count += ret;
    }
#endif


#ifdef _PRE_CONFIG_WLAN_THRANS_THREAD_DEBUG
    ret = snprintf(buf + count, buf_len - count, "hcc thread stat info:\n");
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    ret = snprintf(buf + count, buf_len - count, "condition true:%llu,false:%llu\n",
                                               hcc->hcc_transer_info.thread_stat.wait_event_run_count,
                                               hcc->hcc_transer_info.thread_stat.wait_event_block_count);
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    ret = snprintf(buf + count, buf_len - count, "thread loop do:%llu,empty:%llu\n",
                                               hcc->hcc_transer_info.thread_stat.loop_have_data_count,
                                               hcc->hcc_transer_info.thread_stat.loop_no_data_count);
    if (0 >= ret)
    {
        return count;
    }
    count += ret;
#endif
    return count;
}

oal_int32 hcc_queues_len_info_print_etc(struct hcc_handler* hcc,char* buf, oal_int32 buf_len)
{
    oal_int32 ret = 0;
    oal_int32 count = 0;
    oal_int32 i ,j;

    if(NULL == hcc)
    {
        return ret;
    }
    for(i = 0; i < HCC_DIR_COUNT; i++)
    {
        ret = snprintf(buf + count, buf_len - count, "dir:%s\n", HCC_GET_CHAN_STRING(i));
        if (0 >= ret)
        {
            return count;
        }
        count += ret;

        for(j = 0; j < HCC_QUEUE_COUNT;j++)
        {
            if(oal_netbuf_list_len(&hcc->hcc_transer_info.hcc_queues[i].queues[j].data_queue))
            {
                ret = snprintf(buf + count, buf_len - count, "queue:%d, len:%d\n", j,
                            oal_netbuf_list_len(&hcc->hcc_transer_info.hcc_queues[i].queues[j].data_queue));
                if (0 >= ret)
                {
                    return count;
                }
                count += ret;
            }
        }
    }
    return count;
}

oal_int32 hcc_print_current_trans_info(oal_uint32 print_device_info)
{
    int ret = 0;
    char* buf = NULL;
    oal_uint32 buf_size = PAGE_SIZE;
    hcc_bus*            pst_bus = NULL;
    struct hcc_handler* hcc;
    hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        OAL_IO_PRINT("get hcc handler failed!%s\n",__FUNCTION__);
        return -OAL_EFAIL;
    }

    pst_bus = hcc_get_current_110x_bus();

    buf = oal_memalloc(buf_size);
    if(NULL != buf)
    {
        ret = 0;
        OAL_MEMZERO(buf, buf_size);

        //pst_bus = hcc_get_current_110x_bus();
        if((NULL != pst_bus) && (HCC_BUS_SDIO == pst_bus->bus_type))
        {
            ret += hcc_flowctrl_info_print_etc(hcc, buf , buf_size - ret);
        }

        ret += hcc_queues_len_info_print_etc(hcc, buf, buf_size - ret);

        ret += hcc_queues_pkts_info_print_etc(hcc, buf + ret, buf_size - ret);

        OAL_IO_PRINT("%s\n", buf);

        oal_free(buf);
    }
    else
    {
        OAL_IO_PRINT("alloc buf size %u failed\n", buf_size);
    }

    if(NULL != pst_bus)
    {
        /*打印device信息要保证打印过程中 不会进入深睡*/
        hcc_bus_print_trans_info(pst_bus, print_device_info ? (HCC_PRINT_TRANS_FLAG_DEVICE_STAT|HCC_PRINT_TRANS_FLAG_DEVICE_REGS) : 0x0);
    }

    return OAL_SUCC;
}

OAL_STATIC ssize_t  hcc_get_assem_info(struct device *dev, struct device_attribute *attr, char*buf)
{
    int ret = 0;
    struct hcc_handler* hcc;

    if (NULL == buf)
     {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return ret;
    }

     if (NULL == attr)
     {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return ret;
    }

     if (NULL == dev)
     {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return ret;
    }

    hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        OAL_IO_PRINT("get hcc handler failed!%s\n",__FUNCTION__);
        return ret;
    }

    ret += hcc_assem_info_print_etc(hcc,buf,PAGE_SIZE - ret);

    return ret;
}

OAL_STATIC ssize_t  hcc_set_assem_info(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct hcc_handler* hcc;

    if (NULL == buf)
     {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

     if (NULL == attr)
     {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

     if (NULL == dev)
     {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        OAL_IO_PRINT("get hcc handler failed!%s\n",__FUNCTION__);
        return count;
    }
    hcc_transfer_lock(hcc);
    hcc_tx_assem_info_reset_etc(hcc);
    oal_sdio_rx_assem_info_reset(hcc);
    hcc_transfer_unlock(hcc);

    OAL_IO_PRINT("reset done!\n");

    return count;
}

OAL_STATIC ssize_t  hcc_get_queues_pkts_info(struct device *dev, struct device_attribute *attr, char*buf)
{
    int ret = 0;
    struct hcc_handler* hcc;

    if (NULL == buf)
     {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return ret;
    }

     if (NULL == attr)
     {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return ret;
    }

     if (NULL == dev)
     {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return ret;
    }

    hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        OAL_IO_PRINT("get hcc handler failed!%s\n",__FUNCTION__);
        return ret;
    }

    ret += hcc_queues_pkts_info_print_etc(hcc,buf,PAGE_SIZE - ret);

    return ret;
}

OAL_STATIC ssize_t  hcc_get_queues_len_info(struct device *dev, struct device_attribute *attr, char*buf)
{
    int ret = 0;
    struct hcc_handler* hcc;

    if (NULL == buf)
     {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return ret;
    }

     if (NULL == attr)
     {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return ret;
    }

     if (NULL == dev)
     {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return ret;
    }
    hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        OAL_IO_PRINT("get hcc handler failed!%s\n",__FUNCTION__);
        return ret;
    }

    ret += hcc_queues_len_info_print_etc(hcc,buf,PAGE_SIZE - ret);

    return ret;
}

OAL_STATIC ssize_t  hcc_get_flowctrl_info(struct device *dev, struct device_attribute *attr, char*buf)
{
    int ret = 0;
    struct hcc_handler* hcc;

    if (NULL == buf)
     {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return ret;
    }

     if (NULL == attr)
     {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return ret;
    }

     if (NULL == dev)
     {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return ret;
    }

    hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        OAL_IO_PRINT("get hcc handler failed!%s\n",__FUNCTION__);
        return ret;
    }

    ret += hcc_flowctrl_info_print_etc(hcc,buf,PAGE_SIZE - ret);

    return ret;
}

oal_int32 hcc_wakelock_info_print_etc(struct hcc_handler* hcc, char* buf, oal_int32 buf_len)
{
    oal_int32 ret = 0;
    oal_int32 count = 0;

    if(NULL == hcc)
    {
        return ret;
    }
#ifdef CONFIG_PRINTK
    if(hcc->tx_wake_lock.locked_addr)
    {
        ret = snprintf(buf + count, buf_len - count, "wakelocked by:%pf\n",
                    (oal_void*)hcc->tx_wake_lock.locked_addr);
        if (0 >= ret)
        {
            return count;
        }
        count += ret;
    }
#endif

    ret = snprintf(buf + count, buf_len - count, "hold %lu locks\n", hcc->tx_wake_lock.lock_count);
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    return count;
}

OAL_STATIC ssize_t  hcc_get_wakelock_info(struct device *dev, struct device_attribute *attr, char*buf)
{
    int ret = 0;
    struct hcc_handler* hcc;

    if (NULL == buf)
     {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return ret;
    }

     if (NULL == attr)
     {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return ret;
    }

     if (NULL == dev)
     {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return ret;
    }

    hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        OAL_IO_PRINT("get hcc handler failed!%s\n",__FUNCTION__);
        return ret;
    }

    ret += hcc_wakelock_info_print_etc(hcc,buf,PAGE_SIZE - ret);

    return ret;
}

OAL_STATIC ssize_t  hcc_get_allwakelock_info(struct device *dev, struct device_attribute *attr, char*buf)
{
    int ret = 0;

    if (NULL == buf)
     {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return ret;
    }

     if (NULL == attr)
     {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return ret;
    }

     if (NULL == dev)
     {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return ret;
    }


    ret += oal_print_all_wakelock_buff(buf, PAGE_SIZE);

    return ret;
}

OAL_STATIC ssize_t  hcc_set_allwakelock_info(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    oal_uint32 level;
    char lockname[200];

    if (NULL == buf)
     {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

     if (NULL == attr)
     {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

     if (NULL == dev)
     {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if ((0 == count) || (count >= OAL_SIZEOF(lockname)) || ('\0' != buf[count]))
    {
        OAL_IO_PRINT("input illegal!%s\n",__FUNCTION__);
        return -OAL_EINVAL;
    }

    OAL_MEMZERO(lockname, OAL_SIZEOF(lockname));

    if ((sscanf(buf, "%s %u", lockname, &level) < 2))
    {
        OAL_IO_PRINT("error input,must input more than 2 arguments!\n");
        return count;
    }

    oal_set_wakelock_debuglevel(lockname, !!level);

    return count;
}

OAL_STATIC DEVICE_ATTR(flowctrl, S_IRUGO, hcc_get_flowctrl_info, NULL);
OAL_STATIC DEVICE_ATTR(assem_info, S_IRUGO|S_IWUSR, hcc_get_assem_info, hcc_set_assem_info);
OAL_STATIC DEVICE_ATTR(queues_pkts, S_IRUGO, hcc_get_queues_pkts_info, NULL);
OAL_STATIC DEVICE_ATTR(queues_len, S_IRUGO, hcc_get_queues_len_info, NULL);
OAL_STATIC DEVICE_ATTR(wakelock, S_IRUGO, hcc_get_wakelock_info, NULL);
OAL_STATIC DEVICE_ATTR(wakelockall, S_IRUGO|S_IWUSR, hcc_get_allwakelock_info, hcc_set_allwakelock_info);


oal_uint64 hcc_test_get_trans_time_etc(ktime_t start_time, ktime_t stop_time)
{
    ktime_t trans_time;
    oal_uint64  trans_us;

    trans_time = ktime_sub(stop_time, start_time);

    //OAL_IO_PRINT("start time:%llu, stop time:%llu, trans_time:%llu\n",
    //                ktime_to_us(start_time),ktime_to_us(stop_time),ktime_to_us(trans_time));

    trans_us = (oal_uint64)ktime_to_us(trans_time);

    if (trans_us == 0)
    {
        trans_us = 1;
    }

    return trans_us;
}

oal_void hcc_test_throughput_cac_etc(oal_uint64 trans_bytes, ktime_t start_time, ktime_t stop_time)
 {
     oal_uint64 trans_us;
     oal_uint64 temp;
     oal_uint64 us_to_s; /* converted  usecond to second */

     trans_us = hcc_test_get_trans_time_etc(start_time, stop_time);

     temp = (trans_bytes);

     temp = temp * 1000u;

     temp = temp * 1000u;

     temp = (temp >> 17);

     //temp = temp * 8u;
     temp = div_u64(temp,trans_us);
     //do_div(temp, trans_us); /* temp = temp / trans_us */

     //do_div(temp, 1024u); /* Converted to KBit */

     //do_div(temp, 1024u); /* Converted to MBit */

     us_to_s = trans_us;
     g_hcc_test_event_etc->test_data.trans_time_us = trans_us;
     do_div(us_to_s, 1000000u); /* us_to_s = us_to_s / 1000000 */
     g_hcc_test_event_etc->test_data.throughput = temp;
     g_hcc_test_event_etc->test_data.trans_time = us_to_s;
 }

OAL_STATIC  oal_void hcc_test_throughput_gen(oal_void)
{
    if(HCC_TEST_CASE_TX == g_hcc_test_event_etc->test_data.mode_idx)
    {
        hcc_test_throughput_cac_etc(g_hcc_test_event_etc->test_data.total_sent_bytes,
                                g_hcc_test_event_etc->start_time,
                                g_hcc_test_event_etc->last_time);
    }
    else if(HCC_TEST_CASE_RX == g_hcc_test_event_etc->test_data.mode_idx)
    {
        hcc_test_throughput_cac_etc(g_hcc_test_event_etc->test_data.total_rcvd_bytes,
                                g_hcc_test_event_etc->start_time,
                                g_hcc_test_event_etc->last_time);
    }
    else if(HCC_TEST_CASE_LOOP == g_hcc_test_event_etc->test_data.mode_idx )
    {
        hcc_test_throughput_cac_etc(g_hcc_test_event_etc->test_data.total_rcvd_bytes + g_hcc_test_event_etc->test_data.total_sent_bytes,
                                g_hcc_test_event_etc->start_time,
                                g_hcc_test_event_etc->last_time);
    }

}

OAL_STATIC  oal_uint64 hcc_test_utilization_ratio_gen(oal_uint64 payload_size,oal_uint64 transfer_size)
{
    oal_uint64 ret;
    payload_size = payload_size*1000;
    if(transfer_size)
        ret = div_u64(payload_size,transfer_size);
    else
        ret = 0;
    return ret;
}

/*统计发送方向的丢包率，接收方向默认不丢包*/
OAL_STATIC  oal_uint32 hcc_test_tx_pkt_loss_gen(oal_uint32 tx_pkts,oal_uint32 actual_tx_pkts)
{
    oal_uint32 ul_loss;
    //g_hcc_test_event_etc->test_data.pkt_sent
    if(tx_pkts == actual_tx_pkts || !tx_pkts )
        return 0;
    if(tx_pkts < actual_tx_pkts )
        return 0;

    ul_loss = tx_pkts - actual_tx_pkts;
    return ul_loss * 1000 / tx_pkts;
}


OAL_STATIC oal_int32 hcc_test_rcvd(struct hcc_handler * hcc, oal_uint8 stype, hcc_netbuf_stru* pst_hcc_netbuf, oal_uint8 *pst_context)
{
    oal_int32 ret;
    oal_netbuf_stru* pst_netbuf = pst_hcc_netbuf->pst_netbuf;
    OAL_REFERENCE(pst_context);
    OAL_REFERENCE(hcc);

    //OAL_IO_PRINT("hcc_test_rcvd:%d\n",stype);

    if(OAL_LIKELY(HCC_TEST_SUBTYPE_DATA == stype))
    {
        oal_int32 filter_flag = 0;

        /*计算总共数据包长度*/
        if(OAL_UNLIKELY(OAL_NETBUF_LEN(pst_netbuf)!= g_hcc_test_event_etc->test_data.pkt_len))
        {
            if(printk_ratelimit())
            {
                OAL_IO_PRINT("[E]recvd netbuf pkt len:%d,but request len:%d\n",
                                OAL_NETBUF_LEN(pst_netbuf),
                                g_hcc_test_event_etc->test_data.pkt_len);
            }
            filter_flag = 1;
        }

        if(g_hcc_test_event_etc->verified)
        {
            oal_int32 i;
            oal_int32 flag = 0;
            oal_uint8 *data =  OAL_NETBUF_DATA(pst_netbuf);
            for(i = 0; i < OAL_NETBUF_LEN(pst_netbuf);i++)
            {
                if(*(data + i) != g_hcc_test_event_etc->test_value)
                {
                    flag = 1;
                    OAL_IO_PRINT("[E]data wrong, [i:%d] value:%x should be %x\n",i,*(data + i),g_hcc_test_event_etc->test_value);
                    break;
                }
            }

            if(flag)
            {
                oal_print_hex_dump(data, OAL_NETBUF_LEN(pst_netbuf), 32, "hcc rx verified ");
                filter_flag = 1;
            }
        }

        if(!filter_flag)
        {
            /*filter_flag=1 时接收的数据包不符合要求，则过滤掉*/
            g_hcc_test_event_etc->test_data.pkt_rcvd++;
            g_hcc_test_event_etc->test_data.total_rcvd_bytes += OAL_NETBUF_LEN(pst_netbuf);
            g_hcc_test_event_etc->last_time= ktime_get();
        }

        if(HCC_TEST_CASE_RX == g_hcc_test_event_etc->test_data.mode_idx)
        {
            if(time_after(jiffies, (g_hcc_test_event_etc->start_tick + g_hcc_test_event_etc->tick_timeout)))
            {
                if(!g_hcc_test_event_etc->rx_stop)
                {
                    OAL_IO_PRINT("RxTestTimeIsUp\n");
                    ret = hcc_send_message(hcc_get_110x_handler(), H2D_MSG_STOP_SDIO_TEST);
                    if(ret)
                    {
                        OAL_IO_PRINT("send message failed, ret=%d", ret);
                    }
                    g_hcc_test_event_etc->rx_stop = 1;
                }
            }
        }
    }
    else if(HCC_TEST_SUBTYPE_CMD == stype)
    {
        hcc_test_cmd_stru cmd;
        oal_memcopy(&cmd,OAL_NETBUF_DATA(pst_netbuf),OAL_SIZEOF(hcc_test_cmd_stru));

        if(HCC_TEST_CMD_STOP_TEST == cmd.cmd_type)
        {
            oal_memcopy(&g_hcc_test_event_etc->test_data.trans_info,
                        hcc_get_test_cmd_data(OAL_NETBUF_DATA(pst_netbuf)),
                        OAL_SIZEOF(hsdio_trans_test_info));
        }
        g_hcc_test_event_etc->last_time= ktime_get();
        OAL_IO_PRINT("hcc_test_rcvd:cmd %d recvd!\n",cmd.cmd_type);
        OAL_COMPLETE(&g_hcc_test_event_etc->test_trans_done);
    }
    else
    {
        /*unkown subtype*/
        OAL_IO_PRINT("receive unkown stype:%d\n",stype);
    }

    oal_netbuf_free(pst_netbuf);

    return OAL_SUCC;
}


OAL_STATIC oal_int32 hcc_test_sent(struct hcc_handler* hcc,struct hcc_transfer_param* param,oal_uint16 start_cmd)
{
    oal_uint8 pad_payload = g_hcc_test_event_etc->pad_payload;
    oal_netbuf_stru*       pst_netbuf;
    /*
    * 1) alloc memory for skb,
    * 2) skb free when send after dequeue from tx queue
    * */
    pst_netbuf  = hcc_netbuf_alloc(g_hcc_test_event_etc->test_data.pkt_len + pad_payload);
    if (NULL == pst_netbuf)
    {
     OAL_IO_PRINT("hwifi alloc skb fail.\n");
     return -OAL_EFAIL;
    }

    if(pad_payload)
    {
        oal_netbuf_reserve(pst_netbuf,pad_payload);
    }

    if(g_hcc_test_event_etc->test_value)
    {
        oal_memset(oal_netbuf_put(pst_netbuf,g_hcc_test_event_etc->test_data.pkt_len),g_hcc_test_event_etc->test_value,g_hcc_test_event_etc->test_data.pkt_len);
    }
    else
    {
        oal_netbuf_put(pst_netbuf,g_hcc_test_event_etc->test_data.pkt_len);
    }

    if(HCC_TEST_SUBTYPE_DATA == start_cmd)
    {
        g_hcc_test_event_etc->test_data.total_sent_bytes += OAL_NETBUF_LEN(pst_netbuf);
    }

    return hcc_tx_etc(hcc, pst_netbuf, param);
}

OAL_STATIC oal_int32 hcc_send_test_cmd(oal_uint8* cmd,oal_int32 cmd_len)
{
    oal_netbuf_stru*       pst_netbuf;
    struct hcc_transfer_param st_hcc_transfer_param = {0};
    struct hcc_handler* hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        return -OAL_EFAIL;
    }

    pst_netbuf  = hcc_netbuf_alloc(cmd_len);
    if (NULL == pst_netbuf)
    {
        OAL_IO_PRINT("hwifi alloc skb fail.\n");
        return -OAL_EFAIL;
    }

    oal_memcopy(oal_netbuf_put(pst_netbuf,cmd_len),cmd,cmd_len);

    hcc_hdr_param_init(&st_hcc_transfer_param,
                    HCC_ACTION_TYPE_TEST,
                    HCC_TEST_SUBTYPE_CMD,
                    0,
                    HCC_FC_WAIT,
                    g_hcc_test_event_etc->hcc_test_queue);
    return hcc_tx_etc(hcc, pst_netbuf, &st_hcc_transfer_param);
}

OAL_STATIC oal_int32 hcc_test_rx_start(oal_uint16 start_cmd)
{
    oal_uint32      cmd_len;
    oal_int32       ret = OAL_SUCC;
    //oal_int32       i;
    hcc_test_cmd_stru  *pst_cmd=NULL;
    hcc_trans_test_rx_info * pst_rx_info = NULL;

    struct hcc_handler* hcc;
    hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        return -OAL_EFAIL;
    }
    cmd_len = OAL_SIZEOF(hcc_test_cmd_stru) + OAL_SIZEOF(hcc_trans_test_rx_info);
    pst_cmd = (hcc_test_cmd_stru*)oal_memalloc(cmd_len);
    if(NULL == pst_cmd)
    {
        return -OAL_EFAIL;
    }

    OAL_INIT_COMPLETION(&g_hcc_test_event_etc->test_trans_done);
    g_hcc_test_event_etc->test_data.pkt_rcvd = 0;
    g_hcc_test_event_etc->test_data.pkt_sent = 0;
    g_hcc_test_event_etc->test_data.total_rcvd_bytes = 0;
    g_hcc_test_event_etc->test_data.total_sent_bytes = 0;
    g_hcc_test_event_etc->test_data.throughput = 0;
    g_hcc_test_event_etc->test_data.trans_time = 0;
    g_hcc_test_event_etc->start_tick = jiffies;
    g_hcc_test_event_etc->last_time= g_hcc_test_event_etc->start_time = ktime_get();

    oal_memset((oal_void*)pst_cmd, 0,cmd_len);
    pst_cmd->cmd_type = start_cmd;
    pst_cmd->cmd_len  = cmd_len;

    pst_rx_info = (hcc_trans_test_rx_info*)hcc_get_test_cmd_data(pst_cmd);

    pst_rx_info->total_trans_pkts = g_hcc_test_event_etc->test_data.pkt_gen;
    pst_rx_info->pkt_len = g_hcc_test_event_etc->test_data.pkt_len;
    pst_rx_info->pkt_value = g_hcc_test_event_etc->test_value;

    if(OAL_SUCC != hcc_send_test_cmd((oal_uint8*)pst_cmd,pst_cmd->cmd_len))
    {
        oal_free(pst_cmd);
        return -OAL_EFAIL;
    }

    oal_free(pst_cmd);

    g_hcc_test_event_etc->last_time= ktime_get();

    /*等待回来的CMD命令*/
    ret = wait_for_completion_interruptible(&g_hcc_test_event_etc->test_trans_done);
    if(ret < 0)
    {
        OAL_IO_PRINT("Test Event  terminated ret=%d\n", ret);
        ret = -OAL_EFAIL;
        OAL_IO_PRINT("H2D_MSG_STOP_SDIO_TEST send\n");
        hcc_send_message(hcc, H2D_MSG_STOP_SDIO_TEST);
    }

    if(g_test_force_stop)
    {
        hcc_send_message(hcc, H2D_MSG_STOP_SDIO_TEST);
        g_test_force_stop = 0;
        oal_msleep(100);
    }

    OAL_COMPLETE(&g_hcc_test_event_etc->test_done);
    return ret;
}

oal_int32 hcc_test_pcie_loopback(oal_int32 is_phy_loopback)
{
    oal_netbuf_stru*       pst_netbuf;
    hcc_test_cmd_stru  cmd={0};

    struct hcc_handler* hcc;

    struct hcc_transfer_param st_hcc_transfer_param = {0};

    hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        return -OAL_EFAIL;
    }

    cmd.cmd_type = is_phy_loopback ? HCC_TEST_CMD_PCIE_PHY_LOOPBACK_TST:HCC_TEST_CMD_PCIE_MAC_LOOPBACK_TST;
    cmd.cmd_len = OAL_SIZEOF(hcc_test_cmd_stru);

    pst_netbuf  = hcc_netbuf_alloc(cmd.cmd_len);
    if(NULL == pst_netbuf)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc %u mem failed!", cmd.cmd_len);
        return -OAL_ENOMEM;
    }

    oal_memcopy(oal_netbuf_put(pst_netbuf, cmd.cmd_len), &cmd, cmd.cmd_len);

    hcc_hdr_param_init(&st_hcc_transfer_param,
                    HCC_ACTION_TYPE_TEST,
                    HCC_TEST_SUBTYPE_CMD,
                    0,
                    HCC_FC_WAIT,
                    DATA_HI_QUEUE);

    return hcc_tx_etc(hcc, pst_netbuf, &st_hcc_transfer_param);
}

oal_int32 hcc_test_fix_wcpu_freq(oal_void)
{
    oal_netbuf_stru*       pst_netbuf;
    hcc_test_cmd_stru  cmd={0};

    struct hcc_handler* hcc;

    struct hcc_transfer_param st_hcc_transfer_param = {0};

    if(0 == ft_ip_test_cpu_max_freq)
    {
        return OAL_SUCC;
    }

    hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        return -OAL_EFAIL;
    }

    cmd.cmd_type = HCC_TEST_CMD_CFG_FIX_FREQ;
    cmd.cmd_len = OAL_SIZEOF(hcc_test_cmd_stru);

    pst_netbuf  = hcc_netbuf_alloc(cmd.cmd_len);
    if(NULL == pst_netbuf)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc %u mem failed!", cmd.cmd_len);
        return -OAL_ENOMEM;
    }

    oal_memcopy(oal_netbuf_put(pst_netbuf, cmd.cmd_len), &cmd, cmd.cmd_len);

    hcc_hdr_param_init(&st_hcc_transfer_param,
                    HCC_ACTION_TYPE_TEST,
                    HCC_TEST_SUBTYPE_CMD,
                    0,
                    HCC_FC_WAIT,
                    DATA_HI_QUEUE);

    return hcc_tx_etc(hcc, pst_netbuf, &st_hcc_transfer_param);
}

OAL_STATIC oal_int32 hcc_test_normal_start(oal_uint16 start_cmd)
{
    oal_int32            ret = OAL_SUCC,retry_count = 0;
    oal_int32            i;
    hcc_test_cmd_stru  cmd={0};

    struct hcc_transfer_param st_hcc_transfer_param = {0};
    struct hcc_handler* hcc;
    hcc_hdr_param_init(&st_hcc_transfer_param,HCC_ACTION_TYPE_TEST,HCC_TEST_SUBTYPE_DATA,0,HCC_FC_WAIT,g_hcc_test_event_etc->hcc_test_queue);
    hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR,"wifi is closed!");
        return -OAL_EFAIL;
    }

    OAL_INIT_COMPLETION(&g_hcc_test_event_etc->test_trans_done);
    g_hcc_test_event_etc->test_data.pkt_rcvd = 0;
    g_hcc_test_event_etc->test_data.pkt_sent = 0;
    g_hcc_test_event_etc->test_data.total_rcvd_bytes = 0;
    g_hcc_test_event_etc->test_data.total_sent_bytes = 0;
    g_hcc_test_event_etc->test_data.throughput = 0;
    g_hcc_test_event_etc->test_data.trans_time = 0;

    cmd.cmd_type = start_cmd;
    cmd.cmd_len = OAL_SIZEOF(hcc_test_cmd_stru) + OAL_SIZEOF(hsdio_trans_test_info);
    if(OAL_SUCC != hcc_send_test_cmd((oal_uint8*)&cmd,cmd.cmd_len))
    {
        return -OAL_EFAIL;
    }
    /*wait the device recv the cmd,change the test mode!*/
    oal_msleep(50);

    g_hcc_test_event_etc->last_time= g_hcc_test_event_etc->start_time = ktime_get();
    g_hcc_test_event_etc->start_tick = jiffies;

    for (i = 0; i < g_hcc_test_event_etc->test_data.pkt_gen; i++)
    {
        ret = hcc_test_sent(hcc, &st_hcc_transfer_param,HCC_TEST_SUBTYPE_DATA);
        if(ret < 0)
        {
            OAL_IO_PRINT("hcc test gen pkt send fail.\n");
            break;
        }

        g_hcc_test_event_etc->test_data.pkt_sent++ ;
        g_hcc_test_event_etc->last_time= ktime_get();

        if(time_after(jiffies, (g_hcc_test_event_etc->start_tick + g_hcc_test_event_etc->tick_timeout)))
        {
            OAL_IO_PRINT("TestTimeIsUp\n");
            break;
        }

        if(OAL_UNLIKELY(OAL_FALSE == g_hcc_test_event_etc->started))
        {
            ret = -OAL_EFAIL;
            break;
        }
    }

    cmd.cmd_type = HCC_TEST_CMD_STOP_TEST;
    hcc_send_test_cmd((oal_uint8*)&cmd,cmd.cmd_len);
    g_hcc_test_event_etc->last_time= ktime_get();

retry:
    /*等待回来的CMD命令*/
    ret = wait_for_completion_interruptible_timeout(&g_hcc_test_event_etc->test_trans_done,OAL_MSECS_TO_JIFFIES(5000));
    if(ret < 0)
    {
        OAL_IO_PRINT("Test Event  terminated ret=%d\n", ret);
        ret = -OAL_EFAIL;
        hcc_print_current_trans_info(0);
        hcc_send_test_cmd((oal_uint8*)&cmd,cmd.cmd_len);
    }
    else if(ret == 0)
    {
        /*cmd response timeout*/
        if(retry_count++ < 1)
        {
            oal_msleep(100);
            hcc_send_test_cmd((oal_uint8*)&cmd,cmd.cmd_len);
            g_hcc_test_event_etc->last_time= ktime_get();
            OAL_IO_PRINT("resend the stop cmd!retry count:%d\n",retry_count);
            goto retry;
        }
        else
        {
            OAL_IO_PRINT("resend the stop cmd timeout!retry count:%d\n",retry_count);
            ret = -OAL_EFAIL;
        }
    }
    else
    {
        if(g_test_force_stop)
        {
            hcc_send_test_cmd((oal_uint8*)&cmd,cmd.cmd_len);
            g_hcc_test_event_etc->last_time= ktime_get();
            g_test_force_stop = 0;
            OAL_IO_PRINT("normal start force stop\n");
            oal_msleep(100);
        }
        ret = OAL_SUCC;
    }

    OAL_COMPLETE(&g_hcc_test_event_etc->test_done);
    return ret;
}

OAL_STATIC oal_int32 hcc_test_start(oal_uint16 start_cmd)
{
    OAL_IO_PRINT("%s Test start.\n",g_hcc_test_stru_etc[g_hcc_test_event_etc->test_data.mode_idx].mode);
    if(HCC_TEST_CASE_RX == g_hcc_test_event_etc->test_data.mode_idx)
    {
        return hcc_test_rx_start(start_cmd);
    }
    else
    {
        return hcc_test_normal_start(start_cmd);
    }
}


oal_void hcc_test_work_etc(struct work_struct *work)
{
    oal_uint16       start_cmd;
    oal_int32       ret;

#if defined(CONFIG_ARCH_HISI)
    struct cpumask fast_cpus;
#endif
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    struct sched_param       param = {0};
#endif

    start_cmd = g_hcc_test_stru_etc[g_hcc_test_event_etc->test_data.mode_idx].start_cmd;

    if(hcc_test_performance_mode)
    {
#if defined(CONFIG_ARCH_HISI)
        hisi_get_fast_cpus(&fast_cpus);
#endif

#if defined(CONFIG_ARCH_HISI)
        OAL_IO_PRINT("hcc test bind to fast cpus\n");
        set_cpus_allowed_ptr(current, &fast_cpus);
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        param.sched_priority = 97;
        oal_set_thread_property_etc(current,
                                SCHED_RR,
                                &param,
                                -10);
        oal_msleep(OAL_JIFFIES_TO_MSECS(1));
#endif
    }

    /* hcc test start */
    ret = hcc_test_start(start_cmd);
    if ( -OAL_EFAIL == ret)
    {
        g_hcc_test_event_etc->errorno = ret;
        OAL_COMPLETE(&g_hcc_test_event_etc->test_done);
        OAL_IO_PRINT("hcc test work start test pkt send fail. ret = %d\n", ret);
        return;
    }
}

ssize_t hcc_test_print_thoughput(char*buf, oal_uint32 buf_len)
{
    int ret;
    oal_int32 count = 0;
    const char *mode_str;
    oal_int32       tmp_mode_idx;

    if(NULL == buf)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "buf is null");
        return 0;
    }

    tmp_mode_idx = g_hcc_test_event_etc->test_data.mode_idx;

    mode_str = "unknown";

    if ((tmp_mode_idx >= 0) && (tmp_mode_idx < OAL_ARRAY_SIZE(g_hcc_test_stru_etc)))
    {
        mode_str = g_hcc_test_stru_etc[tmp_mode_idx].mode;
    }

    hcc_test_throughput_gen();

    ret = snprintf(buf + count, buf_len - count, "Test_Mode: %s\n",mode_str);
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    ret = snprintf(buf + count, buf_len - count, "Actual sent %d pkts, request %llu bytes\n",
                                              g_hcc_test_event_etc->test_data.pkt_sent,
                                              ((oal_uint64)g_hcc_test_event_etc->test_data.pkt_sent)*(oal_uint64)g_hcc_test_event_etc->test_data.pkt_len);
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    ret = snprintf(buf + count, buf_len - count, "Actual rcvd %d pkts, request %llu bytes\n", g_hcc_test_event_etc->test_data.pkt_rcvd,
                                              ((oal_uint64)g_hcc_test_event_etc->test_data.pkt_rcvd)*(oal_uint64)g_hcc_test_event_etc->test_data.pkt_len);
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    ret = snprintf(buf + count, buf_len - count, "PayloadSend %llu bytes, ActualSend  %llu bytes\n",
                                                            g_hcc_test_event_etc->test_data.total_sent_bytes,
                                                            g_hcc_test_event_etc->test_data.trans_info.total_h2d_trans_bytes);
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    ret = snprintf(buf + count, buf_len - count, "PayloadRcvd %llu bytes, ActualRecv  %llu bytes\n",
                                                            g_hcc_test_event_etc->test_data.total_rcvd_bytes,
                                                            g_hcc_test_event_etc->test_data.trans_info.total_d2h_trans_bytes);
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    /*SDIO通道利用率*/
    ret = snprintf(buf + count, buf_len - count, "Hcc Utilization Ratio %llu ‰\n",
                            hcc_test_utilization_ratio_gen(g_hcc_test_event_etc->test_data.total_sent_bytes + g_hcc_test_event_etc->test_data.total_rcvd_bytes,
                                                            g_hcc_test_event_etc->test_data.trans_info.total_h2d_trans_bytes +
                                                             g_hcc_test_event_etc->test_data.trans_info.total_d2h_trans_bytes));
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    /*发送方向的丢包率*/
    ret = snprintf(buf + count, buf_len - count, "TxPackageLoss %u ‰, pkt_sent: %d actual_tx_pkts: %u\n",
                                    hcc_test_tx_pkt_loss_gen(g_hcc_test_event_etc->test_data.pkt_sent,g_hcc_test_event_etc->test_data.trans_info.actual_tx_pkts),
                                    g_hcc_test_event_etc->test_data.pkt_sent,
                                    g_hcc_test_event_etc->test_data.trans_info.actual_tx_pkts);
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    ret = snprintf(buf + count, buf_len - count, "Requet Generate %d pkts\n", g_hcc_test_event_etc->test_data.pkt_gen);
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    ret = snprintf(buf + count, buf_len - count, "Per-package Length %d\n", g_hcc_test_event_etc->test_data.pkt_len);
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    ret = snprintf(buf + count, buf_len - count, "TranserTimeCost %llu Seconds, %llu microsecond\n",
                                        g_hcc_test_event_etc->test_data.trans_time,g_hcc_test_event_etc->test_data.trans_time_us);
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    ret = snprintf(buf + count, buf_len - count, "Throughput %u Mbps\n",(oal_int32)g_hcc_test_event_etc->test_data.throughput);
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    return count;
}


OAL_STATIC ssize_t  hcc_test_get_para(struct device *dev, struct device_attribute *attr, char*buf)
{
    if(NULL == dev)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "dev is null");
        return 0;
    }

    if(NULL == attr)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "attr is null");
        return 0;
    }

    return hcc_test_print_thoughput(buf, PAGE_SIZE - 1);
}


OAL_STATIC ssize_t  hcc_test_set_para(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    hcc_test_data  data = {0};
    oal_int32              tmp_pkt_len;
    oal_int32              tmp_pkt_gen;
    char                mode[128] = {0};
    oal_int32              i;

    if (NULL == buf)
     {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

     if (NULL == attr)
     {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

     if (NULL == dev)
     {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if ((0 == count) || (count >= OAL_SIZEOF(mode)) || ('\0' != buf[count]))
    {
        OAL_IO_PRINT("input illegal!%s\n",__FUNCTION__);
        return -OAL_EINVAL;
    }

    if ((sscanf(buf, "%15s %d %d", mode, &tmp_pkt_len, &tmp_pkt_gen) < 1))
    {
        OAL_IO_PRINT("error input,must input more than 1 arguments!\n");
        return -OAL_EINVAL;
    }

    for (i = 0; i < OAL_ARRAY_SIZE(g_hcc_test_stru_etc); i++)
    {
        /* find mode if match */
        if (sysfs_streq(g_hcc_test_stru_etc[i].mode, mode))
        {
            break;
        }
    }

    if (OAL_ARRAY_SIZE(g_hcc_test_stru_etc) == i)
    {
        OAL_IO_PRINT("unknown test mode.%s\n",mode);

        return -OAL_EINVAL;
    }


    data.pkt_len = tmp_pkt_len;
    data.pkt_gen = tmp_pkt_gen;
    data.mode_idx = i;
    data.trans_timeout = ~0UL;

    if(hcc_test_set_case_etc(&data))
    {
        return count;
    }

    //hcc_test_result_report(g_hcc_test_event_etc->test_data.throughput, g_hcc_test_event_etc->test_data.trans_time);
    return count;
}


OAL_STATIC ssize_t  hcc_test_set_rt_para(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    hcc_test_data  data = {0};
    oal_int32              tmp_pkt_len;
    oal_ulong              tmp_runtime;
    char                mode[128] = {0};
    oal_int32              i;

    if (NULL == buf)
     {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

     if (NULL == attr)
     {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

     if (NULL == dev)
     {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if ((0 == count) || (count >= OAL_SIZEOF(mode)) || ('\0' != buf[count]))
    {
        OAL_IO_PRINT("input illegal!%s\n",__FUNCTION__);
        return -OAL_EINVAL;
    }

    if ((sscanf(buf, "%15s %d %lu", mode, &tmp_pkt_len, &tmp_runtime) < 1))
    {
        OAL_IO_PRINT("error input,must input more than 1 arguments!\n");
        return -OAL_EINVAL;
    }

    for (i = 0; i < OAL_ARRAY_SIZE(g_hcc_test_stru_etc); i++)
    {
        /* find mode if match */
        if (sysfs_streq(g_hcc_test_stru_etc[i].mode, mode))
        {
            break;
        }
    }

    if (OAL_ARRAY_SIZE(g_hcc_test_stru_etc) == i)
    {
        OAL_IO_PRINT("unknown test mode.%s\n",mode);

        return -OAL_EINVAL;
    }


    data.pkt_len = tmp_pkt_len;
    data.pkt_gen = 0x7fffffff;
    data.mode_idx = i;
    data.trans_timeout = tmp_runtime;

    if(hcc_test_set_case_etc(&data))
    {
        return count;
    }

    return count;
}

OAL_STATIC DEVICE_ATTR(test, S_IRUGO | S_IWUSR, hcc_test_get_para, hcc_test_set_para);
OAL_STATIC DEVICE_ATTR(test_rt, S_IRUGO | S_IWUSR, hcc_test_get_para, hcc_test_set_rt_para);

OAL_STATIC ssize_t  hcc_dev_panic_set_para(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    char                mode[128] = {0};

    if (NULL == buf)
     {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

     if (NULL == attr)
     {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

     if (NULL == dev)
     {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if ((0 == count) || (count >= OAL_SIZEOF(mode)) || ('\0' != buf[count]))
    {
        OAL_IO_PRINT("input illegal!%s\n",__FUNCTION__);
        return -OAL_EINVAL;
    }

    if ((sscanf(buf, "%s", mode) < 1))
    {
        OAL_IO_PRINT("error input,must input more than 1 arguments!\n");
        return count;
    }

    if(oal_strncmp(mode, "panic", OAL_STRLEN("panic")))
    {
        OAL_IO_PRINT("invalid input:%s\n", mode);
        return count;
    }

    wlan_pm_disable_etc();
    hcc_bus_send_message(hcc_get_current_110x_bus(), H2D_MSG_TEST);
    wlan_pm_enable_etc();
    return count;
}
OAL_STATIC DEVICE_ATTR(dev_panic, S_IWUSR, NULL, hcc_dev_panic_set_para);

OAL_STATIC ssize_t  hcc_test_set_abort_test(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    oal_int32 ret;
    struct hcc_handler* hcc;
    hcc = hcc_get_110x_handler();
    ret = hcc_send_message(hcc, H2D_MSG_STOP_SDIO_TEST);
    OAL_IO_PRINT("hcc_test_set_abort_test msg send ret=%d\n", ret);
    oal_msleep(500);
    return count;
}

OAL_STATIC DEVICE_ATTR(abort_test, S_IWUSR, NULL, hcc_test_set_abort_test);

OAL_STATIC ssize_t  hcc_test_set_value(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    oal_uint32      value;

    if (NULL == buf)
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == attr)
    {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == dev)
    {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if ((sscanf(buf, "0x%2x", &value) != 1))
    {
        OAL_IO_PRINT("set value one char!\n");
        return -OAL_EINVAL;
    }

    g_hcc_test_event_etc->test_value = value;

    return count;
}

OAL_STATIC ssize_t  hcc_test_get_value(struct device *dev, struct device_attribute *attr, char*buf)
{
    int ret = 0;

    if (NULL == buf)
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == attr)
    {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == dev)
    {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    ret +=  snprintf(buf + ret , PAGE_SIZE-1, "0x%2x\n",g_hcc_test_event_etc->test_value);
    return ret;
}

OAL_STATIC ssize_t  hcc_test_set_queue_id(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    oal_uint32      queue_id;

    if (NULL == buf)
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == attr)
    {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == dev)
    {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if ((sscanf(buf, "%u", &queue_id) != 1))
    {
        OAL_IO_PRINT("set value one char!\n");
        return -OAL_EINVAL;
    }
    if(queue_id >= HCC_QUEUE_COUNT)
    {
        OAL_IO_PRINT("wrong queue id:%u\n",queue_id);
        return count;
    }
    g_hcc_test_event_etc->hcc_test_queue = (hcc_queue_type)queue_id;

    return count;
}

OAL_STATIC ssize_t  hcc_test_get_queue_id(struct device *dev, struct device_attribute *attr, char*buf)
{
    int ret = 0;
    if (NULL == buf)
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == attr)
    {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == dev)
    {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    ret +=  snprintf(buf + ret , PAGE_SIZE-1, "%d\n",g_hcc_test_event_etc->hcc_test_queue);
    return ret;
}

OAL_STATIC ssize_t  hcc_test_set_pad_payload(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    oal_uint32      pad_payload;

    if (NULL == buf)
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == attr)
    {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == dev)
    {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if ((sscanf(buf, "%u", &pad_payload) != 1))
    {
        OAL_IO_PRINT("set value one char!\n");
        return -OAL_EINVAL;
    }

    g_hcc_test_event_etc->pad_payload = (oal_uint8)pad_payload;

    return count;
}

OAL_STATIC ssize_t  hcc_test_get_pad_payload(struct device *dev, struct device_attribute *attr, char*buf)
{
    int ret = 0;
    if (NULL == buf)
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == attr)
    {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == dev)
    {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }
    ret +=  snprintf(buf + ret , PAGE_SIZE-1, "%d\n",g_hcc_test_event_etc->pad_payload);
    return ret;
}

OAL_STATIC ssize_t  hcc_test_set_verified(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    oal_uint32      verified;

    if (NULL == buf)
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == attr)
    {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == dev)
    {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if ((sscanf(buf, "%u", &verified) != 1))
    {
        OAL_IO_PRINT("set value one char!\n");
        return -OAL_EINVAL;
    }
    verified = !!verified;
    g_hcc_test_event_etc->verified = (oal_uint8)verified;

    return count;
}

OAL_STATIC ssize_t  hcc_test_get_verified(struct device *dev, struct device_attribute *attr, char*buf)
{
    int ret = 0;
    if (NULL == buf)
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == attr)
    {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == dev)
    {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    ret +=  snprintf(buf + ret , PAGE_SIZE-1, "%d\n",g_hcc_test_event_etc->verified);
    return ret;
}

OAL_STATIC ssize_t  hcc_test_set_switch(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    oal_int32 ret;
    oal_uint8 input[200];

    if (NULL == buf)
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == attr)
    {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == dev)
    {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if ((0 == count) || (count >= OAL_SIZEOF(input)) || ('\0' != buf[count]))
    {
        OAL_IO_PRINT("input illegal!%s\n",__FUNCTION__);
        return -OAL_EINVAL;
    }

    OAL_MEMZERO((oal_void*)input, OAL_SIZEOF(input));

    if ((sscanf(buf, "%s", input) != 1))
    {
        OAL_IO_PRINT("set value one char!\n");
        return -OAL_EINVAL;
    }

    ret = conn_test_hcc_chann_switch(input);
    if(OAL_SUCC == ret)
    {
        OAL_IO_PRINT("swtich to %s succ\n", input);
    }
    else
    {
        OAL_IO_PRINT("swtich to %s failed\n", input);
    }

    return count;
}

OAL_STATIC ssize_t  hcc_test_get_switch(struct device *dev, struct device_attribute *attr, char*buf)
{
    int ret = 0;
    char* current_bus;
    hcc_bus* pst_bus;

    if (NULL == buf)
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == attr)
    {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == dev)
    {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    pst_bus = hcc_get_current_110x_bus();

    if(NULL == pst_bus)
    {
        current_bus = "none";
    }
    else
    {
        if(HCC_BUS_SDIO == pst_bus->bus_type)
        {
            current_bus = "sdio";
        }
        else if(HCC_BUS_PCIE == pst_bus->bus_type)
        {
            current_bus = "pcie";
        }
        else if(HCC_BUS_USB == pst_bus->bus_type)
        {
            current_bus = "usb";
        }
        else
        {
            current_bus = "unkown type";
        }
    }

    ret +=  snprintf(buf + ret , PAGE_SIZE-1, "%s\n",current_bus);
    return ret;
}

OAL_STATIC DEVICE_ATTR(value, S_IRUGO | S_IWUSR, hcc_test_get_value, hcc_test_set_value);
OAL_STATIC DEVICE_ATTR(queue_id, S_IRUGO | S_IWUSR, hcc_test_get_queue_id, hcc_test_set_queue_id);
OAL_STATIC DEVICE_ATTR(pad_payload, S_IRUGO | S_IWUSR, hcc_test_get_pad_payload, hcc_test_set_pad_payload);
OAL_STATIC DEVICE_ATTR(verified, S_IRUGO | S_IWUSR, hcc_test_get_verified, hcc_test_set_verified);
OAL_STATIC DEVICE_ATTR(switch_bus, S_IRUGO | S_IWUSR, hcc_test_get_switch, hcc_test_set_switch);

OAL_STATIC struct attribute *hcc_test_sysfs_entries[] = {
        &dev_attr_test.attr,
        &dev_attr_test_rt.attr,
        &dev_attr_abort_test.attr,
        &dev_attr_value.attr,
        &dev_attr_verified.attr,
        &dev_attr_queue_id.attr,
        &dev_attr_pad_payload.attr,
        &dev_attr_switch_bus.attr,
        &dev_attr_dev_panic.attr,
        //&dev_attr_Readme.attr,
        NULL
};

OAL_STATIC struct attribute_group hcc_test_attribute_group = {
        .name = "test",
        .attrs = hcc_test_sysfs_entries,
};

OAL_STATIC struct attribute *hcc_sysfs_entries[] = {
        &dev_attr_flowctrl.attr,
        &dev_attr_assem_info.attr,
        &dev_attr_queues_pkts.attr,
        &dev_attr_queues_len.attr,
        &dev_attr_wakelock.attr,
        &dev_attr_wakelockall.attr,
        NULL
};
OAL_STATIC struct attribute_group hcc_attribute_group = {
        //.name = "hcc",
        .attrs = hcc_sysfs_entries,
};

oal_int32 hsdio_sysfs_info_print_etc(struct oal_sdio *hi_sdio,char* buf, oal_int32 buf_len)
{
    oal_int32 ret = 0;
    oal_int32 bit;
    ret +=  snprintf(buf + ret , buf_len - ret, "sdio info, state:0x%4x\n",hi_sdio->state);
    ret +=  snprintf(buf + ret , buf_len - ret,"gpio_int_count:%llu \n",  hi_sdio->pst_bus->gpio_int_count);
    ret +=  snprintf(buf + ret , buf_len - ret,"wakeup_int_count:%llu \n", hi_sdio->pst_bus->wakeup_int_count);
    ret +=  snprintf(buf + ret , buf_len - ret,"func1_err_reg_info:%u \n", hi_sdio->func1_stat.func1_err_reg_info);
    ret +=  snprintf(buf + ret , buf_len - ret,"func1_err_int_count:%u \n", hi_sdio->func1_stat.func1_err_int_count);
    ret +=  snprintf(buf + ret , buf_len - ret,"func1_ack_int_acount:%u \n", hi_sdio->func1_stat.func1_ack_int_acount);
    ret +=  snprintf(buf + ret , buf_len - ret,"func1_msg_int_count:%u \n", hi_sdio->func1_stat.func1_msg_int_count);
    ret +=  snprintf(buf + ret , buf_len - ret,"func1_data_int_count:%u \n", hi_sdio->func1_stat.func1_data_int_count);
    ret +=  snprintf(buf + ret , buf_len - ret,"func1_no_int_count:%u \n", hi_sdio->func1_stat.func1_no_int_count);

    ret +=  snprintf(buf + ret , buf_len - ret,"\nsdio error stat:\n");
    ret +=  snprintf(buf + ret , buf_len - ret,"rx_scatt_info_not_match:%d\n", hi_sdio->error_stat.rx_scatt_info_not_match);
    ret +=  snprintf(buf + ret , buf_len - ret,"msg count info:\n");
    ret +=  snprintf(buf + ret , buf_len - ret,"tx scatt buf len:%u\n", hi_sdio->tx_scatt_buff.len);
	ret +=  snprintf(buf + ret , buf_len - ret,"rx scatt buf len:%u\n", hi_sdio->rx_scatt_buff.len);
    for(bit = 0; bit < D2H_MSG_COUNT; bit++)
    {
        if(hi_sdio->pst_bus->msg[bit].count)
            ret +=  snprintf(buf + ret , buf_len - ret,"msg [%d] count:%u:,last update time:%llu\n",
                        bit,hi_sdio->pst_bus->msg[bit].count,
                        hi_sdio->pst_bus->msg[bit].cpu_time);
    }
    ret +=  snprintf(buf + ret , buf_len - ret,"last msg:%u\n",hi_sdio->pst_bus->last_msg);
    return ret;
}

OAL_STATIC ssize_t  hsdio_get_sdio_info(struct device *dev, struct device_attribute *attr, char*buf)
{
    int ret = 0;
    struct hcc_handler* hcc;

    if (NULL == buf)
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == attr)
    {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == dev)
    {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        OAL_IO_PRINT("get hcc handler failed!%s\n",__FUNCTION__);
        return ret;
    }

    if(oal_get_sdio_default_handler())
        ret += hsdio_sysfs_info_print_etc(oal_get_sdio_default_handler(),buf,PAGE_SIZE - ret);

    return ret;
}

oal_int32 hsdio_wakelock_info_print_etc(struct oal_sdio *hi_sdio,char* buf, oal_int32 buf_len)
{
    oal_int32 ret;
    oal_int32 count = 0;

#ifdef CONFIG_PRINTK
    if(hi_sdio->pst_bus->st_bus_wakelock.locked_addr)
    {
        ret = snprintf(buf + count, buf_len - count, "wakelocked by:%pf\n",
                    (oal_void*)hi_sdio->pst_bus->st_bus_wakelock.locked_addr);
        if (0 >= ret)
        {
            return count;
        }
        count += ret;
    }
#endif

    ret = snprintf(buf + count, buf_len - count, "hold %lu locks\n", hi_sdio->pst_bus->st_bus_wakelock.lock_count);
    if (0 >= ret)
    {
        return count;
    }
    count += ret;

    return count;
}

OAL_STATIC ssize_t  hsdio_get_wakelock_info(struct device *dev, struct device_attribute *attr, char*buf)
{
    int ret = 0;
    struct hcc_handler* hcc;

    if (NULL == buf)
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == attr)
    {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == dev)
    {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        OAL_IO_PRINT("get hcc handler failed!%s\n",__FUNCTION__);
        return ret;
    }

    if(oal_get_sdio_default_handler())
        ret += hsdio_wakelock_info_print_etc(oal_get_sdio_default_handler(),buf,PAGE_SIZE - ret);

    return ret;
}

OAL_STATIC DEVICE_ATTR(sdio_info, S_IRUGO, hsdio_get_sdio_info, NULL);
OAL_STATIC struct device_attribute dev_attr_sdio_wakelock = __ATTR(wakelock, S_IRUGO, hsdio_get_wakelock_info, NULL);
OAL_STATIC struct attribute *hsdio_sysfs_entries[] = {
        &dev_attr_sdio_info.attr,
        &dev_attr_sdio_wakelock.attr,
        NULL
};

OAL_STATIC struct attribute_group hsdio_attribute_group = {
        .name = "sdio",
        .attrs = hsdio_sysfs_entries,
};

int hcc_test_set_case_etc(hcc_test_data *data)
{
    int ret;
    int errorno = OAL_SUCC;
    if(OAL_WARN_ON(NULL == data))
        return -OAL_EINVAL;

    if(OAL_UNLIKELY(!g_hcc_test_event_etc->test_workqueue))
    {
        OAL_IO_PRINT("wifi probe failed, please retry.\n");
        return -OAL_EBUSY;
    }

    mutex_lock(&g_hcc_test_event_etc->mutex_lock);
    if (OAL_TRUE == g_hcc_test_event_etc->started)
    {
        OAL_IO_PRINT("sdio test task is processing, wait for end and reinput.\n");
        mutex_unlock(&g_hcc_test_event_etc->mutex_lock);
        return -OAL_EINVAL;
    }

    OAL_IO_PRINT("%s Test Start,test pkts:%d,pkt len:%d\n",
                    g_hcc_test_stru_etc[data->mode_idx].mode,data->pkt_gen,data->pkt_len);

    g_hcc_test_event_etc->started = OAL_TRUE;
    g_hcc_test_event_etc->rx_stop = 0;
    g_hcc_test_event_etc->errorno = OAL_SUCC;
    g_hcc_test_event_etc->tick_timeout =  OAL_MSECS_TO_JIFFIES(data->trans_timeout);

    oal_memcopy(&g_hcc_test_event_etc->test_data, data, sizeof(hcc_test_data));

    g_test_force_stop = 0;
    INIT_COMPLETION(g_hcc_test_event_etc->test_done);

    queue_work(g_hcc_test_event_etc->test_workqueue, &g_hcc_test_event_etc->test_work);
    ret = wait_for_completion_interruptible(&g_hcc_test_event_etc->test_done);
    if(ret < 0)
    {
        OAL_IO_PRINT("Test Event  terminated ret=%d\n", ret);
        g_hcc_test_event_etc->started = OAL_FALSE;
        OAL_COMPLETE(&g_hcc_test_event_etc->test_trans_done);
        g_test_force_stop = 1;
        cancel_work_sync(&g_hcc_test_event_etc->test_work);
    }

    OAL_IO_PRINT("Test Done.ret=%d\n", g_hcc_test_event_etc->errorno);

    hcc_test_throughput_gen();

    oal_memcopy(data, &g_hcc_test_event_etc->test_data, sizeof(hcc_test_data));
    g_hcc_test_event_etc->started = OAL_FALSE;
    errorno = g_hcc_test_event_etc->errorno;
    mutex_unlock(&g_hcc_test_event_etc->mutex_lock);
    return errorno;
}

extern oal_uint8 g_uc_netdev_is_open_etc;
oal_int32 hcc_test_current_bus_chan(oal_int32 pkt_len,
                                          oal_int32 mode_idx,
                                          oal_ulong trans_timeout,
                                          oal_uint32 min_throught,
                                          oal_int32 retry_cnt)
{
    oal_int32 ret;
    oal_int32 retry_times  = 0;

    oal_void* pst_buff = NULL;

    hcc_test_data data = {0};

    data.pkt_len = pkt_len;
    data.pkt_gen = 0x7fffffff;
    data.trans_timeout = trans_timeout;
    data.mode_idx = mode_idx;

    g_uc_netdev_is_open_etc = OAL_TRUE;
    ret = wlan_pm_open_etc();
    if(OAL_SUCC != ret)
    {
        return ret;
    }

    /*下发固定频率的命令*/
    ret = hcc_test_fix_wcpu_freq();
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "set  wcpu fix freq failed! ret=%d", ret);
        wlan_pm_close_etc();
        return ret;
    }

    /*电压拉偏*/
    hcc_bus_voltage_bias_init(hcc_get_current_110x_bus());

retry:

    ret = hcc_test_set_case_etc(&data);
    if(ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "conn_test_wifi_chan_loop test failed!");
        wlan_pm_close_etc();
        return ret;
    }

    pst_buff = oal_memalloc(PAGE_SIZE);
    if(NULL != pst_buff)
    {
        OAL_MEMZERO(pst_buff, 1);
        hcc_test_print_thoughput(pst_buff, PAGE_SIZE - 1);
        OAL_IO_PRINT("%s", (char*)pst_buff);
        oal_free(pst_buff);
    }
    else
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "alloc mem failed!");
    }

    if(min_throught > 0)
    {
        /*吞吐率门限，当小于最小值时，认为性能失败*/
        if(g_hcc_test_event_etc->test_data.throughput  < (oal_uint64)min_throught)
        {
            retry_times++;
            if(retry_times <= retry_cnt)
            {
                /*retry*/
                oal_print_hi11xx_log(HI11XX_LOG_WARN, "loop test warning, throught too low, just %llu Mbps, min_limit is %u Mbps, retry_cnt:%d",
                                g_hcc_test_event_etc->test_data.throughput,
                                min_throught,
                                retry_cnt);
                goto retry;
            }
            else
            {
                /*touch retry limit, failed*/
                oal_print_hi11xx_log(HI11XX_LOG_ERR, "loop test failed, throught too low, just %llu Mbps, min_limit is %u Mbps, retry_cnt:%d",
                                g_hcc_test_event_etc->test_data.throughput,
                                min_throught,
                                retry_cnt);
                wlan_pm_close_etc();
                return -OAL_EFAIL;
            }
        }
        else
        {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "throught %llu Mbps over limit %u Mbps, retry times %d, test pass!",
                                                  g_hcc_test_event_etc->test_data.throughput,
                                                  min_throught,
                                                  retry_times);
        }
    }

    hcc_bus_chip_info(hcc_get_current_110x_bus());

    ret = wlan_pm_close_etc();
    if(OAL_SUCC != ret)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "conn_test_wifi_chan_loop test failed! ret=%d", ret);
    }

    return OAL_SUCC;
}

/*测试WIFI通道是否连通*/
oal_int32 conn_test_wifi_chan_loop(char *param)
{
    oal_int32 ret;
    oal_int32 run_flag = 0;
    hcc_bus   *pst_bus, *old_bus;
    declare_time_cost_stru(cost);
    oal_int32  pkt_len = 1500;

    OAL_REFERENCE(param);

    if(!wlan_is_shutdown_etc())
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wlan is open, test abort!");
        return -OAL_EBUSY;
    }

    old_bus = hcc_get_current_110x_bus();

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
    if(OAL_TRUE == oal_pcie_110x_working_check())
    {
        oal_int32 pcie_min_throught = 0;
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "PCIe Chan Test!");
        oal_get_time_cost_start(cost);

        pst_bus = hcc_get_current_110x_bus();
        if (NULL == pst_bus)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "pst_bus is null");
            return -OAL_EFAIL;
        }

        if(HCC_BUS_PCIE != pst_bus->bus_type)
        {
            /*尝试切换到PCIE*/
            ret = hcc_switch_bus(HCC_CHIP_110X_DEV, HCC_BUS_PCIE);
            if(ret)
            {
                oal_print_hi11xx_log(HI11XX_LOG_ERR, "switch to PCIe failed, ret=%d", ret);
                return -OAL_ENODEV;
            }
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "switch to PCIe ok.");
        }

        /*Test Start*/
        /*check pcie loop first*/
        ret = hcc_test_current_bus_chan(pkt_len, HCC_TEST_CMD_START_LOOP,
                                        ft_pcie_test_wifi_runtime, 0, 0);
        if(OAL_SUCC != ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "test PCIe failed, ret=%d", ret);
            return ret;
        }


        if(INI_SUCC == get_cust_conf_int32_etc(INI_MODU_PLAT, "pcie_min_throught", &pcie_min_throught))
        {
            ft_pcie_test_min_throught = pcie_min_throught;
            oal_print_hi11xx_log(HI11XX_LOG_INFO,"pcie min throught %d Mbps from ini", pcie_min_throught);
        }

        /*check the pcie throught*/
        if(ft_pcie_test_min_throught)
        {
            ret = hcc_test_current_bus_chan(pkt_len, HCC_TEST_CMD_START_RX,
                                            ft_pcie_test_wifi_runtime, (oal_uint32)ft_pcie_test_min_throught, ft_pcie_test_retry_cnt);
            if(OAL_SUCC != ret)
            {
                oal_print_hi11xx_log(HI11XX_LOG_ERR, "test PCIe failed, ret=%d", ret);
                return ret;
            }
        }

        oal_get_time_cost_end(cost);
        oal_calc_time_cost_sub(cost);
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "pcie loop cost %llu us", time_cost_var_sub(cost));

        run_flag++;
    }
#endif

#ifdef CONFIG_MMC
    if(OAL_TRUE == oal_sdio_110x_working_check())
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "SDIO Chan Test!");
        oal_get_time_cost_start(cost);
        pst_bus = hcc_get_current_110x_bus();
        if (NULL == pst_bus)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "pst_bus is null");
            return -OAL_EFAIL;
        }

        if(HCC_BUS_SDIO != pst_bus->bus_type)
        {
            /*尝试切换到SDIO*/
            ret = hcc_switch_bus(HCC_CHIP_110X_DEV, HCC_BUS_SDIO);
            if(ret)
            {
                oal_print_hi11xx_log(HI11XX_LOG_ERR, "switch to PCIe failed, ret=%d", ret);
                return -OAL_ENODEV;
            }
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "switch to SDIO ok.");
        }

        /*Test Start*/
        ret = hcc_test_current_bus_chan(pkt_len,
                                        HCC_TEST_CMD_START_RX,
                                        ft_sdio_test_wifi_runtime,
                                        0,0);
        if(OAL_SUCC != ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "test PCIe failed, ret=%d", ret);
            return ret;
        }

        oal_get_time_cost_end(cost);
        oal_calc_time_cost_sub(cost);
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "sdio loop cost %llu us", time_cost_var_sub(cost));

        run_flag++;
    }
#endif

    pst_bus = hcc_get_current_110x_bus();
    if (NULL == pst_bus)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "pst_bus is null");
        return -OAL_EFAIL;
    }

    if(old_bus->bus_type != pst_bus->bus_type)
    {
        ret = hcc_switch_bus(HCC_CHIP_110X_DEV, old_bus->bus_type);
        if(ret)
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "restore to %u failed, ret=%d", old_bus->bus_type, ret);
            return -OAL_ENODEV;
        }
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "restore to %d ok.", old_bus->bus_type);
    }

    if(run_flag)
    {
        return 0;
    }
    else
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "no wifi chan is runing...");
        return -1;
    }
}
EXPORT_SYMBOL(conn_test_wifi_chan_loop);

/*Input, "sdio" or "pcie"*/
oal_int32 conn_test_hcc_chann_switch(char* new_dev)
{
    oal_int32 ret = -OAL_EFAIL;
    if(OAL_WARN_ON(NULL == new_dev))
    {
        return -OAL_EINVAL;
    }
    if(!oal_strcmp("sdio", new_dev))
    {
        ret = hcc_switch_bus(HCC_CHIP_110X_DEV, HCC_BUS_SDIO);
    }
    else if(!oal_strcmp("pcie", new_dev))
    {
        ret = hcc_switch_bus(HCC_CHIP_110X_DEV, HCC_BUS_PCIE);
    }
    else
    {
        ret = -OAL_ENODEV;
    }

    return ret;
}
EXPORT_SYMBOL(conn_test_hcc_chann_switch);

oal_void hcc_test_get_case_etc(hcc_test_data *data)
{
    if(OAL_WARN_ON(NULL == data))
        return;
    hcc_test_throughput_gen();
    oal_memcopy((oal_void*)data, (oal_void*)&g_hcc_test_event_etc->test_data, sizeof(hcc_test_data));
}

#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
OAL_STATIC oal_int32 hwifi_panic_handler(struct notifier_block *this,
                   oal_ulong event, oal_void *unused)
{
    hcc_bus* pst_bus;

    if(wifi_panic_debug_etc)
        hwifi_panic_log_dump_etc(KERN_ERR);
    else
        printk(KERN_WARNING"wifi panic debug off\n");

    pst_bus = hcc_get_current_110x_bus();
    if(NULL != pst_bus)
    {
        if(HCC_BUS_PCIE == pst_bus->bus_type)
        {
#if defined(_PRE_CONFIG_GPIO_TO_SSI_DEBUG)
            //ssi_save_device_regs();
#endif
        }
    }
    return NOTIFY_OK;
}

OAL_STATIC struct notifier_block hwifi_panic_notifier = {
    .notifier_call  = hwifi_panic_handler,
};
#endif

oal_void hcc_test_kirin_noc_handler(oal_void)
{
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
    hcc_bus* pst_bus;

    pst_bus = hcc_get_current_110x_bus();
    if(NULL != pst_bus)
    {
        if(HCC_BUS_PCIE == pst_bus->bus_type)
        {
#if defined(_PRE_CONFIG_GPIO_TO_SSI_DEBUG)
            oal_pcie_linux_res * pst_pci_lres = (oal_pcie_linux_res*)pst_bus->data;
            printk(KERN_ERR "hisi wifi noc dump\n");
            if((NULL != pst_pci_lres) && (NULL != pst_pci_lres->pst_pci_res))
            {
                printk(KERN_ERR "pcie link state:%s\n", oal_pcie_get_link_state_str(pst_pci_lres->pst_pci_res->link_state));
            }

            printk(KERN_ERR "host wakeup dev gpio:%d\n", board_get_host_wakeup_dev_stat());
            if(HI1XX_ANDROID_BUILD_VARIANT_USER == hi11xx_get_android_build_variant())
            {
                ssi_dump_device_regs(SSI_MODULE_MASK_AON_CUT | SSI_MODULE_MASK_PCIE_CUT);
            }
            else
            {
                ssi_dump_device_regs(SSI_MODULE_MASK_COMM | SSI_MODULE_MASK_PCIE_CFG);
            }
#else
            printk(KERN_ERR"ssi not support\n");
#endif
        }
    }
#endif
}

#if (defined(CONFIG_KIRIN_PCIE_NOC_NOTIFY))
typedef void (*func_pcie_noc_notify)(void);
extern func_pcie_noc_notify g_pcie_noc_notify;
#elif (defined(CONFIG_KIRIN_PCIE_NOC_DBG))
typedef void (* WIFI_DUMP_FUNC) (void);
void register_wifi_dump_func(WIFI_DUMP_FUNC func);
#else
#endif

oal_int32  hcc_test_init_module_etc(struct hcc_handler* hcc)
{
    oal_int32       ret = OAL_SUCC;
    oal_kobject*     pst_root_object = NULL;

#if (defined(CONFIG_KIRIN_PCIE_NOC_NOTIFY))
    g_pcie_noc_notify = hcc_test_kirin_noc_handler;
#elif (defined(CONFIG_KIRIN_PCIE_NOC_DBG))
    register_wifi_dump_func(hcc_test_kirin_noc_handler);
#else
#endif

    pst_root_object = oal_get_sysfs_root_object_etc();
    if(NULL == pst_root_object)
    {
        OAL_IO_PRINT("[E]get root sysfs object failed!\n");
        return -OAL_EFAIL;
    }

    g_conn_syfs_hcc_object = kobject_create_and_add("hcc", pst_root_object);
    if(NULL == g_conn_syfs_hcc_object)
    {
        goto fail_g_conn_syfs_hcc_object;
    }

    ret = oal_debug_sysfs_create_group(g_conn_syfs_hcc_object,&hsdio_attribute_group);
    if (ret)
    {
        ret = -OAL_ENOMEM;
        OAL_IO_PRINT("sysfs create sdio_sysfs_entries group fail.ret=%d\n",ret);
        goto fail_create_sdio_group;
    }

    /* create the files associated with this kobject */
    ret = oal_debug_sysfs_create_group(g_conn_syfs_hcc_object, &hcc_test_attribute_group);
    if (ret)
    {
        ret = -OAL_ENOMEM;
        OAL_IO_PRINT("sysfs create test group fail.ret=%d\n",ret);
        goto fail_create_hcc_test_group;
    }

    ret = oal_debug_sysfs_create_group(g_conn_syfs_hcc_object,&hcc_attribute_group);
    if (ret)
    {
        OAL_IO_PRINT("sysfs create hcc group fail.ret=%d\n",ret);
        ret = -OAL_ENOMEM;
        goto fail_create_hcc_group;
    }

    /* alloc memory for perf_action pointer */
    g_hcc_test_event_etc = kzalloc(sizeof(*g_hcc_test_event_etc), GFP_KERNEL);
    if (!g_hcc_test_event_etc)
    {
        OAL_IO_PRINT("error kzalloc g_hcc_test_event_etc mem.\n");
        ret = -OAL_ENOMEM;
        goto fail_g_hcc_test_event;
    }

    g_hcc_test_event_etc->hcc_test_queue = DATA_LO_QUEUE;
    g_hcc_test_event_etc->test_value = 0x5a;

    /* register callback for rx */
    ret = hcc_rx_register_etc(hcc,HCC_ACTION_TYPE_TEST, hcc_test_rcvd, NULL);
    if ( OAL_SUCC != ret)
    {
        OAL_IO_PRINT("error %d register callback for rx.\n", ret);
        ret = -OAL_EFAIL;
        goto fail_rx_cb_register;
    }

    /* mutex lock init */
    mutex_init(&g_hcc_test_event_etc->mutex_lock);

    OAL_INIT_COMPLETION(&g_hcc_test_event_etc->test_done);
    OAL_INIT_COMPLETION(&g_hcc_test_event_etc->test_trans_done);

    /* init hcc_test param */
    g_hcc_test_event_etc->test_data.mode_idx = -1;
    g_hcc_test_event_etc->test_data.pkt_len  = 0;
    g_hcc_test_event_etc->test_data.pkt_sent = 0;
    g_hcc_test_event_etc->test_data.pkt_gen  = 0;
    g_hcc_test_event_etc->started  = OAL_FALSE;

    /* create workqueue */
    g_hcc_test_event_etc->test_workqueue = create_singlethread_workqueue("sdio_test");
    if (NULL == g_hcc_test_event_etc)
    {
        OAL_IO_PRINT("work queue create fail.\n");
        ret =  -OAL_ENOMEM;
        goto fail_sdio_test_workqueue;
    }
    INIT_WORK(&g_hcc_test_event_etc->test_work, hcc_test_work_etc);

#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
    atomic_notifier_chain_register(&panic_notifier_list,
                       &hwifi_panic_notifier);
#endif

#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
    if(oal_get_sdio_default_handler())
        hwifi_panic_log_register_etc(&sdio_panic,oal_get_sdio_default_handler());
    hwifi_panic_log_register_etc(&hcc_panic_assem_info,hcc);
    hwifi_panic_log_register_etc(&hcc_panic_flowctrl,hcc);

    hwifi_panic_log_register_etc(&hcc_panic_queues_len,hcc);
    hwifi_panic_log_register_etc(&hcc_panic_queues_pkts,hcc);
#endif
    return ret;
fail_sdio_test_workqueue:
    hcc_rx_unregister_etc(hcc,HCC_ACTION_TYPE_TEST);
fail_rx_cb_register:
    kfree(g_hcc_test_event_etc);
fail_g_hcc_test_event:
    oal_debug_sysfs_remove_group(g_conn_syfs_hcc_object, &hcc_attribute_group);
fail_create_hcc_group:
    oal_debug_sysfs_remove_group(g_conn_syfs_hcc_object, &hcc_test_attribute_group);
fail_create_hcc_test_group:
    oal_debug_sysfs_remove_group(g_conn_syfs_hcc_object, &hsdio_attribute_group);
fail_create_sdio_group:
    kobject_put(g_conn_syfs_hcc_object);
fail_g_conn_syfs_hcc_object:
    return ret;
}

oal_void  hcc_test_exit_module_etc(struct hcc_handler* hcc)
{
#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
    atomic_notifier_chain_unregister(&panic_notifier_list,
                                    &hwifi_panic_notifier);
#endif

    if(g_hcc_test_event_etc->test_workqueue)
    {
        destroy_workqueue(g_hcc_test_event_etc->test_workqueue);
        g_hcc_test_event_etc->test_workqueue = NULL;
    }
    oal_debug_sysfs_remove_group(g_conn_syfs_hcc_object, &hcc_attribute_group);
    oal_debug_sysfs_remove_group(g_conn_syfs_hcc_object, &hcc_test_attribute_group);
    oal_debug_sysfs_remove_group(g_conn_syfs_hcc_object, &hsdio_attribute_group);
    kobject_put(g_conn_syfs_hcc_object);
    hcc_rx_unregister_etc(hcc,HCC_ACTION_TYPE_TEST);

#if (defined(CONFIG_KIRIN_PCIE_NOC_NOTIFY))
    g_pcie_noc_notify = NULL;
#elif (defined(CONFIG_KIRIN_PCIE_NOC_DBG))
    register_wifi_dump_func(NULL);
#else
#endif

    kfree(g_hcc_test_event_etc);
    g_hcc_test_event_etc = NULL;
}
#endif
