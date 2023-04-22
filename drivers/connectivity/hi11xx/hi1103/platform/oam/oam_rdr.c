#include <linux/workqueue.h>
#include <linux/errno.h>
#include "oam_rdr.h"
#if (defined CONFIG_HISI_BB) && (defined CONFIG_HI110X_PLAT_BB)
#include "mntn_subtype_exception.h"
struct rdr_exception_info_s hisi_conn_excetption_info[] = {
    {
    .e_modid            = (u32)MODID_CONN_WIFI_EXEC,
    .e_modid_end        = (u32)MODID_CONN_WIFI_EXEC,
    .e_process_priority = RDR_ERR,
    .e_reboot_priority  = RDR_REBOOT_NO,
    .e_notify_core_mask = RDR_CONN,
    .e_reset_core_mask  = RDR_CONN,
    .e_from_core        = RDR_CONN,
    .e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
    .e_exce_type        = WIFI_S_EXCEPTION,
    .e_exce_subtype     = CONN_WIFI_EXEC,
    .e_upload_flag      = (u32)RDR_UPLOAD_YES,
    .e_from_module      = "CONN",
    .e_desc             = "CONN_WIFI_EXEC",
    },
    {
    .e_modid            = (u32)MODID_CONN_WIFI_CHAN_EXEC,
    .e_modid_end        = (u32)MODID_CONN_WIFI_CHAN_EXEC,
    .e_process_priority = RDR_ERR,
    .e_reboot_priority  = RDR_REBOOT_NO,
    .e_notify_core_mask = RDR_CONN,
    .e_reset_core_mask  = RDR_CONN,
    .e_from_core        = RDR_CONN,
    .e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
    .e_exce_type        = WIFI_S_EXCEPTION,
    .e_exce_subtype     = CONN_WIFI_CHAN_EXEC,
    .e_upload_flag      = (u32)RDR_UPLOAD_YES,
    .e_from_module      = "CONN",
    .e_desc             = "CONN_WIFI_CHAN_EXEC",
    },
    {
    .e_modid            = (u32)MODID_CONN_WIFI_WAKEUP_FAIL,
    .e_modid_end        = (u32)MODID_CONN_WIFI_WAKEUP_FAIL,
    .e_process_priority = RDR_ERR,
    .e_reboot_priority  = RDR_REBOOT_NO,
    .e_notify_core_mask = RDR_CONN,
    .e_reset_core_mask  = RDR_CONN,
    .e_from_core        = RDR_CONN,
    .e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
    .e_exce_type        = WIFI_S_EXCEPTION,
    .e_exce_subtype     = CONN_WIFI_WAKEUP_FAIL,
    .e_upload_flag      = (u32)RDR_UPLOAD_YES,
    .e_from_module      = "CONN",
    .e_desc             = "CONN_WIFI_WAKEUP_FAIL",
    },
    {
    .e_modid            = (u32)MODID_CONN_BFGX_EXEC,
    .e_modid_end        = (u32)MODID_CONN_BFGX_EXEC,
    .e_process_priority = RDR_ERR,
    .e_reboot_priority  = RDR_REBOOT_NO,
    .e_notify_core_mask = RDR_CONN,
    .e_reset_core_mask  = RDR_CONN,
    .e_from_core        = RDR_CONN,
    .e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
    .e_exce_type        = BFGX_S_EXCEPTION,
    .e_exce_subtype     = CONN_BFGX_EXEC,
    .e_upload_flag      = (u32)RDR_UPLOAD_YES,
    .e_from_module      = "CONN",
    .e_desc             = "CONN_BFGX_EXEC",
    },
    {
    .e_modid            = (u32)MODID_CONN_BFGX_BEAT_TIMEOUT,
    .e_modid_end        = (u32)MODID_CONN_BFGX_BEAT_TIMEOUT,
    .e_process_priority = RDR_ERR,
    .e_reboot_priority  = RDR_REBOOT_NO,
    .e_notify_core_mask = RDR_CONN,
    .e_reset_core_mask  = RDR_CONN,
    .e_from_core        = RDR_CONN,
    .e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
    .e_exce_type        = BFGX_S_EXCEPTION,
    .e_exce_subtype     = CONN_BFGX_BEAT_TIMEOUT,
    .e_upload_flag      = (u32)RDR_UPLOAD_YES,
    .e_from_module      = "CONN",
    .e_desc             = "CONN_BFGX_BEAT_TIMEOUT",
    },
    {
    .e_modid            = (u32)MODID_CONN_BFGX_WAKEUP_FAIL,
    .e_modid_end        = (u32)MODID_CONN_BFGX_WAKEUP_FAIL,
    .e_process_priority = RDR_ERR,
    .e_reboot_priority  = RDR_REBOOT_NO,
    .e_notify_core_mask = RDR_CONN,
    .e_reset_core_mask  = RDR_CONN,
    .e_from_core        = RDR_CONN,
    .e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
    .e_exce_type        = BFGX_S_EXCEPTION,
    .e_exce_subtype     = CONN_BFGX_WAKEUP_FAIL,
    .e_upload_flag      = (u32)RDR_UPLOAD_YES,
    .e_from_module      = "CONN",
    .e_desc             = "CONN_BFGX_WAKEUP_FAIL",
    }
};
hisi_conn_ctl_t hisi_conn_modid_cfg[MODID_CONN_ARRAY_LEN] = {
        /*MODID_CONN_WIFI_EXEC*/
        {
         .upload_en = UPLOAD_ALLOW,
         .interva_stime=60,
         },
        /*MODID_CONN_WIFI_CHAN_EXEC*/
        {
          .upload_en = UPLOAD_ALLOW,
         .interva_stime=60,
         },
        /*MODID_CONN_WIFI_WAKEUP_FAIL*/
        {
          .upload_en = UPLOAD_ALLOW,
         .interva_stime=60,
         },
        /*MODID_CONN_BFGX_EXEC*/
        {
          .upload_en = UPLOAD_ALLOW,
         .interva_stime=60,
         },
        /*MODID_CONN_BFGX_BEAT_TIMEOUT*/
        {
         .upload_en = UPLOAD_ALLOW,
         .interva_stime=60,
         },
        /*MODID_CONN_BFGX_WAKEUP_FAIL*/
        {
         .upload_en = UPLOAD_ALLOW,
         .interva_stime=60,
         },
};
static int32 initdone = 0;
extern int32 hisi_conn_initdone(void);
struct work_struct hisi_conn_dump_work;
struct workqueue_struct *hisi_conn_rdr_wq;

struct hisi_conn_info_s hisi_conn_info;
int32 hisi_conn_stat_init(void)
{
    int32 i;
    struct timex  txc;;
    do_gettimeofday(&(txc.time));
    txc.time.tv_sec -= sys_tz.tz_minuteswest * 60;
    for( i = 0; i < MODID_CONN_ARRAY_LEN; i++)  {
        hisi_conn_modid_cfg[i].stat_info.lastuploadtime =txc.time.tv_sec;
        hisi_conn_modid_cfg[i].stat_info.happen_cnt = 0;
        hisi_conn_modid_cfg[i].stat_info.upload_cnt = 0;
    }
    return 0;
}
void _hisi_conn_rdr_system_error(uint32 modid,uint32 arg1,uint32 arg2)
{
   unsigned long long subtime;
   int32 modid_array;
   struct timex  txc;;
   modid_array=modid-MODID_CONN_WIFI_EXEC;

   if (UPLOAD_DISALLOW == hisi_conn_modid_cfg[modid_array].upload_en) {
        PS_PRINT_INFO("[%s]:upload modid[0x%x] not allow ship,value:%d!\n", __func__,modid,hisi_conn_modid_cfg[modid_array].upload_en);
        return;
   }

   do_gettimeofday(&(txc.time));
   txc.time.tv_sec -= sys_tz.tz_minuteswest * 60;
   subtime =  txc.time.tv_sec-hisi_conn_modid_cfg[modid_array].stat_info.lastuploadtime;
    if(hisi_conn_modid_cfg[modid_array].interva_stime < subtime)  {
        PS_PRINT_INFO("[%s]:upload bbox[0x%x]!\n", __func__,modid);
        rdr_system_error(modid, arg1, arg2);
        hisi_conn_modid_cfg[modid_array].stat_info.lastuploadtime = txc.time.tv_sec;
        hisi_conn_modid_cfg[modid_array].stat_info.upload_cnt++;
    }else {
        PS_PRINT_WARNING("[%s]:upload bbox error[0x%x],cur_interva_stime:%lld,set interva_stime:%lld!\n", __func__,modid,subtime,hisi_conn_modid_cfg[modid_array].interva_stime);
    }
    hisi_conn_modid_cfg[modid_array].stat_info.happen_cnt++;
}
int32 hisi_conn_rdr_system_error(uint32 modid,uint32 arg1,uint32 arg2)
{
    if ((modid < MODID_CONN_WIFI_EXEC) ||(modid >= MODID_CONN_BOTT)) {
        PS_PRINT_ERR("[%s]:Input parameter is error[0x%x]!\n", __func__,modid);
        return -EINVAL;
    } else {
        if (!hisi_conn_initdone()) {
            _hisi_conn_rdr_system_error(modid, arg1, arg2);
        }
    }
    return 0;
}
 void plat_bbox_msg_hander(int32 subsys_type, int32 exception_type)
 {
        if(subsys_type == SUBSYS_BFGX) {
            switch (exception_type)
            {
                 case BFGX_LASTWORD_PANIC:
                     RDR_EXCEPTION(MODID_CONN_BFGX_EXEC);
                     break;
                 case BFGX_BEATHEART_TIMEOUT:
                     RDR_EXCEPTION(MODID_CONN_BFGX_BEAT_TIMEOUT);
                     break;
     //            case BFGX_TIMER_TIMEOUT:
        //         case BFGX_POWERON_FAIL:
                 case BFGX_WAKEUP_FAIL:
                    RDR_EXCEPTION(MODID_CONN_BFGX_WAKEUP_FAIL);
                    break;
                default:
                    break;
                }
        }else if (subsys_type == SUBSYS_WIFI)  {
            switch (exception_type)
            {
                case WIFI_WATCHDOG_TIMEOUT:
                case BFGX_TIMER_TIMEOUT:
                case WIFI_DEVICE_PANIC:
                     RDR_EXCEPTION(MODID_CONN_WIFI_EXEC);
                     break;
             //   case WIFI_POWERON_FAIL:
                case WIFI_WAKEUP_FAIL:
                     RDR_EXCEPTION(MODID_CONN_WIFI_WAKEUP_FAIL);
                     break;
                case WIFI_TRANS_FAIL:
                     RDR_EXCEPTION(MODID_CONN_WIFI_CHAN_EXEC);
                     break;
                default:
                break;
              }
        }
 }
static int hisi_conn_copy_reg_to_bbox(char *src_addr, unsigned int len)
{
    unsigned int temp_offset = 0;

    if ((NULL == src_addr) || (0 == len)) {
        PS_PRINT_ERR("[%s]:HISI_CONN_ERROR:Input parameter is error!\n", __func__);
        return -EINVAL;
    }

    temp_offset = hisi_conn_info.bbox_addr_offset + len;
    //hisi_conn_bbox alloc size 8k
    if (temp_offset > hisi_conn_info.hisi_conn_ret_info.log_len) {
        PS_PRINT_ERR("[%s]:HISI_CONN_ERROR:Copy log to bbox size is error! temp_offset=%d\n", __func__, temp_offset);
        temp_offset = 0;
        hisi_conn_info.bbox_addr_offset = 0;
        return -ENOMEM ;
    }

    memcpy(((char*)hisi_conn_info.rdr_addr + hisi_conn_info.bbox_addr_offset), src_addr, len);
    hisi_conn_info.bbox_addr_offset = temp_offset;

    return 0;

}
int32 hisi_conn_save_stat_info(char* buf,int32 index ,int32 limit)
{
    int32 i;
    struct rtc_time tm;
    index += snprintf( buf + index, limit - index,"==========curr bbox info:\n");
    for( i = 0; i < MODID_CONN_ARRAY_LEN; i++)  {
            rtc_time_to_tm(hisi_conn_modid_cfg[i].stat_info.lastuploadtime, &tm);
            index += snprintf( buf + index, limit - index,"   id:%-10d  upload_en:%d    upload_cnt:%llu    happen_cnt:%llu    interva_stime:%llus   lastuploadtime:%4d-%02d-%02d  %02d:%02d:%02d\n",
                                                                                                i,
                                                                                               hisi_conn_modid_cfg[i].upload_en,
                                                                                               hisi_conn_modid_cfg[i].stat_info.upload_cnt,
                                                                                               hisi_conn_modid_cfg[i].stat_info.happen_cnt,
                                                                                               hisi_conn_modid_cfg[i].interva_stime,
                                                                                               tm.tm_year + 1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
    return index;
}
void hisi_conn_save_bbox_info(void)
{
    char log_buf[HISI_CONN_BUF_LEN_MAX+1] = {0};

    memset(hisi_conn_info.rdr_addr, 0 ,hisi_conn_info.hisi_conn_ret_info.log_len);

    hisi_conn_save_stat_info(log_buf,   0,  HISI_CONN_BUF_LEN_MAX);

    hisi_conn_copy_reg_to_bbox(log_buf, strlen(log_buf));

    return;
}
static void hisi_conn_write_reg_log(void)
{
    switch (hisi_conn_info.dump_info.modid) {
        case MODID_CONN_WIFI_EXEC:
        case MODID_CONN_WIFI_CHAN_EXEC:
        case MODID_CONN_WIFI_WAKEUP_FAIL:
        case MODID_CONN_BFGX_EXEC:
        case MODID_CONN_BFGX_BEAT_TIMEOUT:
        case MODID_CONN_BFGX_WAKEUP_FAIL:
        break;
        default:
        break;
    }
    hisi_conn_save_bbox_info();
    return;
}
static void hisi_conn_rdr_dump(u32 modid, u32 etype, u64 coreid, char *pathname, pfn_cb_dump_done pfn_cb)
{
    if (NULL == pathname) {
        PS_PRINT_ERR("[%s]:HISI_CONN_ERROR:pathname is empty\n", __func__);
        return;
    }
    if (!initdone) {
        PS_PRINT_WARNING("[%s]:HISI_CONN_ERROR: rdr not init\n", __func__);
        return;
    }
    hisi_conn_info.dump_info.modid = modid;
    hisi_conn_info.dump_info.coreid = coreid;
    hisi_conn_info.dump_info.pathname = pathname;
    hisi_conn_info.dump_info.cb = pfn_cb;
    hisi_conn_info.bbox_addr_offset = 0;
    queue_work(hisi_conn_rdr_wq, &hisi_conn_dump_work);
    return;
}
static void hisi_conn_rdr_reset(u32 modid, u32 etype, u64 coreid)
{
    return;
}
static void hisi_conn_rdr_dump_work(struct work_struct *work)
{
    hisi_conn_write_reg_log();

    if (hisi_conn_info.dump_info.cb) {
        hisi_conn_info.dump_info.cb(hisi_conn_info.dump_info.modid, hisi_conn_info.dump_info.coreid);
    }

    return;
}
static int hisi_conn_register_exception(void)
{
    int ret;
    unsigned int  size;
    unsigned long index;

    size = sizeof(hisi_conn_excetption_info)/sizeof(struct rdr_exception_info_s);
    for (index = 0; index < size; index++) {
        /* error return 0, ok return modid */
        ret = rdr_register_exception(&hisi_conn_excetption_info[index]);
        if (!ret) {
            PS_PRINT_ERR("[%s]:HISI_CONN_ERROR:rdr_register_exception is failed! index=%ld ret=%d\n", __func__, index, ret);
        return -EINTR;
    }
    }

    return 0;
}
static int hisi_conn_register_core(void)
{
    int ret;
    struct rdr_module_ops_pub s_soc_ops;

    s_soc_ops.ops_dump = hisi_conn_rdr_dump;
    s_soc_ops.ops_reset = hisi_conn_rdr_reset;
    /* register hisi_conn core dump and reset function */
    ret = rdr_register_module_ops((u64)RDR_CONN, &s_soc_ops, &hisi_conn_info.hisi_conn_ret_info);
    if (ret != 0) {
        PS_PRINT_ERR("[%s]:HISI_CONN_ERROR:rdr_register_module_ops is failed! ret=0x%08x\n", __func__, ret);
    }

    return ret;
}
static int hisi_conn_addr_map(void)
{
    hisi_conn_info.rdr_addr = hisi_bbox_map((phys_addr_t)hisi_conn_info.hisi_conn_ret_info.log_addr, hisi_conn_info.hisi_conn_ret_info.log_len);
    if (!hisi_conn_info.rdr_addr) {
        PS_PRINT_ERR("[%s]:HISI_CONN_ERROR:hisi_bbox_map is failed!\n", __func__);
        return -EFAULT;
    }

    return 0;
}
static int hisi_conn_remove_map(void)
{
    hisi_bbox_unmap((char*)(hisi_conn_info.hisi_conn_ret_info.log_addr));
    return 0;
}
static int hisi_conn_rdr_resource_init(void)
{
    hisi_conn_rdr_wq = create_singlethread_workqueue("hisi_conn_rdr_wq");
    if (!hisi_conn_rdr_wq) {
        PS_PRINT_ERR("[%s]:HISI_CONN_ERROR:Create_singlethread_workqueue is failed!\n", __func__);
        return -EINTR;
    }

    INIT_WORK(&hisi_conn_dump_work, hisi_conn_rdr_dump_work);

    return 0;
}
int hisi_conn_rdr_init(void)
{
    int ret;

    ret = hisi_conn_rdr_resource_init();
    if (0 != ret) {
        PS_PRINT_ERR("[%s]:HISI_CONN_ERROR:hisi_conn_rdr_resource_init is faild!ret=%d\n", __func__, ret);
        return ret;
    }

    /* register ics exception */
    ret = hisi_conn_register_exception();
    if (0 != ret) {
        PS_PRINT_ERR("[%s]:HISI_CONN_ERROR:hisi_conn_register_exception is faild!ret=%d\n", __func__, ret);
        goto exit_resource;
    }

    /* register ics dump and reset function */
    ret = hisi_conn_register_core();
    if (0 != ret) {
        PS_PRINT_ERR("[%s]:HISI_CONN_ERROR:hisi_conn_register_core is failed!ret=%d\n", __func__, ret);
        goto exit_exception;
    }

    ret = hisi_conn_addr_map();
    if (0 != ret) {
        PS_PRINT_ERR("[%s]:HISI_CONN_ERROR:hisi_conn_addr_map is failed!ret=%d\n", __func__, ret);
        goto exit_core;
    }

     ret = hisi_conn_stat_init();
    if (0 != ret) {
        PS_PRINT_ERR("[%s]:HISI_CONN_ERROR:hisi_conn_stat_init is failed!ret=%d\n", __func__, ret);
        goto exit_core;
    }   

    PS_PRINT_INFO("hisi_conn_rdr_init all succ, buffer_len:0x%x,\n", hisi_conn_info.hisi_conn_ret_info.log_len);
    initdone = 1;
    return 0;

exit_core:
    rdr_unregister_module_ops((u64)RDR_CONN);
exit_exception:
    hisi_conn_remove_map();
exit_resource:
    destroy_workqueue(hisi_conn_rdr_wq);
    initdone = 0;
    return -EFAULT;
}
int32 hisi_conn_initdone(void)
{
    if (!initdone) {
        if(hisi_conn_rdr_init()) {
            initdone = 0;
            return -1;
        }
    }
    return 0;
}

int hisi_conn_rdr_exit(void)
{
    if (initdone) {
        hisi_conn_remove_map();
        rdr_unregister_module_ops((u64)RDR_CONN);
        rdr_unregister_exception(RDR_CONN);
        destroy_workqueue(hisi_conn_rdr_wq);
        initdone = 0;
        PS_PRINT_INFO("hisi_conn_rdr_init exit\n");
    }else {
        PS_PRINT_INFO("hisi_conn_rdr_init not init no need exit\n");
    }

    return 0;
}
#endif
