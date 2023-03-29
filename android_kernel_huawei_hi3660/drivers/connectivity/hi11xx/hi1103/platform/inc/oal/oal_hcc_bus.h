

#ifndef __OAL_HCC_BUS_H
#define __OAL_HCC_BUS_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_sdio_comm.h"
#include "oal_mem.h"
#include "oal_schedule.h"
#include "oal_util.h"
//#include "oal_hcc_host_if.h"
#include "oal_hcc_comm.h"
#include "oal_workqueue.h"

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define OAL_BUS_RX_THREAD_NICE    (-20)
#define OAL_BUS_RX_THREAD_POLICY             SCHED_NORMAL
#define OAL_BUS_RXDATA_THREAD_PRIORITY       (0)
#define OAL_BUS_DISPOSE_THREAD_PRIORITY      (10)

/*高性能CPU，默认值*/
#define OAL_BUS_HPCPU_NUM   (4)
#define OAL_BUS_HPCPU_NUM_1 (5)

#define OAL_BUS_STATE_TX        (1<<0)
#define OAL_BUS_STATE_RX        (1<<1)
#define OAL_BUS_STATE_ALL       (OAL_BUS_STATE_TX|OAL_BUS_STATE_RX)

/*****************************************************************************
  3 枚举定义
*****************************************************************************/

typedef struct pm_callback
{
    oal_uint (*pm_wakeup_dev)(oal_void);                    //??????PM????,???????,????
    oal_uint (*pm_state_get)(oal_void);                     //????PM??
    oal_uint (*pm_wakeup_host)(oal_void);                   //device??host????
    oal_void (*pm_feed_wdg)(oal_void);                      //PM Sleep watch dog????
    oal_void (*pm_wakeup_dev_ack)(oal_void);   				//??device?ACK ????
    oal_uint32 (*pm_disable)(oal_int32);
}pm_callback_stru;

typedef oal_int32 (*hcc_bus_msg_rx)(oal_void* data);

typedef oal_int32 (*hcc_bus_data_rx)(oal_void* data);
typedef struct _hcc_bus_ops_
{
    hcc_bus_data_rx rx;
}hcc_bus_ops;


struct bus_msg_stru
{
    hcc_bus_msg_rx msg_rx;
    void* data;
    oal_uint32 count;
    oal_uint64 cpu_time;/*the time of the last come!*/
};

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/



/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/

/*一种类型对应一种IP驱动，
  PCIE驱动不同SOC架构驱动差异较大*/
#define HCC_BUS_SDIO    (0)
#define HCC_BUS_PCIE    (1) /*110x PCIE*/
#define HCC_BUS_PCIE2   (2) /*reserved for 118x's PCIE*/
#define HCC_BUS_USB     (3)
#define HCC_BUS_BUTT    (HCC_BUS_USB+1)

#define HCC_BUS_SDIO_CAP    (1 << HCC_BUS_SDIO)
#define HCC_BUS_PCIE_CAP    (1 << HCC_BUS_PCIE)
#define HCC_BUS_PCIE2_CAP   (1 << HCC_BUS_PCIE2)
#define HCC_BUS_USB_CAP     (1 << HCC_BUS_USB)


/*dev define*/
#define HCC_CHIP_110X_DEV       (0)
#define HCC_CHIP_118X_DEV       (1) /*Reserved*/

#define HOST_WAIT_BOTTOM_INIT_TIMEOUT   (20000)

#define HCC_BUS_PPS_COUNT_TIMEOUT       (100) /*100ms*/

/*队列优先级*/
typedef enum _HCC_BUS_Q_PRIO_
{
    HCC_BUS_Q_HIGH,
    HCC_BUS_Q_NORMAL,
    HCC_BUS_Q_LOW,
    HCC_BUS_Q_BUTT
}HCC_BUS_Q_PRIO;

/*Power Action*/
typedef enum _HCC_BUS_POWER_ACTION_TYPE_
{
    HCC_BUS_POWER_DOWN,/*The action when wlan power down*/
    HCC_BUS_POWER_UP,/*The action when wlan power up*/
    HCC_BUS_POWER_PATCH_LOAD_PREPARE,/*The action before patch downlaod*/
    HCC_BUS_POWER_PATCH_LAUCH,/*The action after patch download before channel enabled*/
    HCC_BUS_SW_POWER_DOWN,/*switch power down*/
    HCC_BUS_SW_POWER_UP,/*switch power down*/
    HCC_BUS_SW_POWER_PATCH_LAUCH,/*swtich power patch lauch*/
    HCC_BUS_SW_POWER_PATCH_LOAD_PREPARE,
    HCC_BUS_POWER_BUTT
}HCC_BUS_POWER_ACTION_TYPE;

typedef enum _HCC_BUS_POWER_CTRL_TYPE_
{
    HCC_BUS_CTRL_POWER_DOWN, /*the func callbed by bus*/
    HCC_BUS_CTRL_POWER_UP, /*the func callbed by bus*/
    HCC_BUS_CTRL_POWER_BUTT
}HCC_BUS_POWER_CTRL_TYPE;

#define HCC_TO_BUS(hcc)     (((struct hcc_handler*)hcc)->bus_dev->cur_bus)
#define HCC_TO_DEV(hcc)     (((struct hcc_handler*)hcc)->bus_dev)
#define HBUS_TO_DEV(bus) (((hcc_bus*)bus)->bus_dev)
#define HBUS_TO_HCC(bus) (((hcc_bus*)bus)->bus_dev->hcc)
#define HDEV_TO_HBUS(dev)   (dev->cur_bus)
typedef struct _hcc_bus_ hcc_bus;
typedef struct _hcc_bus_dev_ hcc_bus_dev;

typedef struct _hcc_switch_action_{
    oal_dlist_head_stru         list;
    oal_int32 (*switch_notify)(oal_uint32 dev_id, hcc_bus *old_bus, hcc_bus *new_bus, oal_void* data);
    oal_void* data;
    char* name;
}hcc_switch_action;

typedef struct _hcc_bus_opt_ops{
    oal_int32 (*get_bus_state)(hcc_bus *pst_bus, oal_uint32 mask);
    oal_void  (*enable_bus_state)(hcc_bus *pst_bus, oal_uint32 mask);
    oal_void  (*disable_bus_state)(hcc_bus *pst_bus, oal_uint32 mask);

    oal_int32 (*rx_netbuf_list)(hcc_bus *pst_bus, oal_netbuf_head_stru* pst_head);
    oal_int32 (*tx_netbuf_list)(hcc_bus *pst_bus, oal_netbuf_head_stru* pst_head, hcc_netbuf_queue_type qtype);

    oal_int32 (*tx_condition)(hcc_bus *pst_bus, hcc_netbuf_queue_type qtype);/*ip idle ?, 1 is idle, 0 is busy*/

    oal_int32 (*send_msg_etc)(hcc_bus *pst_bus, oal_uint32 val);

    oal_int32 (*lock)(hcc_bus *pst_bus);
    oal_int32 (*unlock)(hcc_bus *pst_bus);

    oal_int32 (*sleep_request)(hcc_bus *pst_bus);/*硬件行为 通知DEV睡眠*/
    oal_int32 (*sleep_request_host)(hcc_bus *pst_bus);/*检查Host是否满足睡眠条件*/
    oal_int32 (*wakeup_request)(hcc_bus *pst_bus);/*硬件行为 唤醒DEV*/
    oal_int32 (*get_sleep_state)(hcc_bus *pst_bus);
    oal_int32 (*wakeup_complete)(hcc_bus *pst_bus);

    oal_int32 (*switch_suspend_tx)(hcc_bus *pst_bus);
    oal_int32 (*switch_resume_tx)(hcc_bus *pst_bus);
    oal_int32 (*get_trans_count)(hcc_bus *pst_bus, oal_uint64 *tx, oal_uint64 *rx);/*the pkt nums had transfer done*/

    oal_int32 (*switch_clean_res)(hcc_bus* pst_bus);

    oal_int32 (*rx_int_mask)(hcc_bus *pst_bus, oal_int32 is_mask);

    oal_int32 (*reinit)(hcc_bus *pst_bus);/*ip reinit*/
    oal_int32 (*deinit)(hcc_bus *pst_bus);/*ip deinit*/

    oal_int32 (*power_action)(hcc_bus *pst_bus, HCC_BUS_POWER_ACTION_TYPE action);/*do something before power on/off*/

    oal_int32 (*power_ctrl)(hcc_bus *hi_bus, HCC_BUS_POWER_CTRL_TYPE ctrl, int (*func)(void* data), oal_void* data);

    oal_int32 (*wlan_gpio_handler)(hcc_bus *pst_bus, oal_int32 irq);/*wlan gpio handler func*/
    oal_int32 (*wlan_gpio_rxdata_proc)(hcc_bus *pst_bus);/*wlan rx gpio bottom half*/

    oal_int32 (*patch_read)(hcc_bus *pst_bus, oal_uint8* buff, oal_int32 len, oal_uint32 timeout);
    oal_int32 (*patch_write)(hcc_bus *pst_bus, oal_uint8* buff, oal_int32 len);

    oal_int32 (*bindcpu)(hcc_bus *pst_bus, oal_uint32 chan, oal_int32 is_bind);/*绑定相关任务，提高处理能力*/

    oal_int32 (*voltage_bias_init)(hcc_bus *pst_bus);

    oal_void  (*chip_info)(hcc_bus *pst_bus);

    oal_void  (*print_trans_info)(hcc_bus *pst_bus, oal_uint64 print_flag);
    oal_void  (*reset_trans_info)(hcc_bus *pst_bus);
    oal_int32 (*pending_signal_check)(hcc_bus *pst_bus);/*调度中检查process条件是否符合, 0:不符合   非0:符合*/
    oal_int32 (*pending_signal_process)(hcc_bus *pst_bus);/*process中要清掉调度标记，否者会死循环*/
}hcc_bus_opt_ops;

typedef struct _hcc_bus_cap_
{
    /*接口是否为全双工,全双工软件收发不能相互阻塞*/
    oal_int32                   is_full_duplex;/*预留*/

    /*TX/RX 长度对齐要求，业务使用*/
    oal_uint32                  align_size[HCC_DIR_COUNT];/*bus align request*/

    oal_uint32                  max_trans_size;           /*IP层一次可以传输的最大长度*/
}hcc_bus_cap;

struct _hcc_bus_{
    oal_dlist_head_stru         list;
    oal_uint32                  bus_type;                /*current bus type, sdio/usb/pcie */
    oal_uint32                  bus_id;
    oal_uint32                  dev_id;

    hcc_bus_cap                 cap;

    hcc_bus_dev                 *bus_dev;

    char                        name[50];

    /*IP 私有结构体索引*/
    oal_void	                *data;                      /*dev driver strut reference, sdio~usb*/

    oal_void                    *hcc;                       /*reference to hcc*/

    /*pm part*/
    struct semaphore            sr_wake_sema;               /*android suspend and resume sem*/
    pm_callback_stru            *pst_pm_callback;           /*???????*/

    oal_wakelock_stru           st_bus_wakelock;

    struct task_struct          *pst_rx_tsk;                /*????*/
    struct semaphore            rx_sema;                    /*?????*/
    oal_mutex_stru              rx_transfer_lock;           /*?????*/

    oal_void*                   bus_ops_data;
    hcc_bus_ops                 bus_ops;

    hcc_bus_opt_ops             *opt_ops;

    oal_completion              st_device_ready;


    struct bus_msg_stru         msg[D2H_MSG_COUNT];
    oal_uint32                  last_msg;

    oal_work_stru               bus_excp_worker;
    oal_int32                   bus_excp_type;
    oal_spin_lock_stru          bus_excp_lock;
    oal_wakelock_stru           bus_excp_wlock;/*kernel wakelock*/

    oal_uint64                  gpio_int_count;
    oal_uint64                  data_int_count;
    oal_uint64                  wakeup_int_count;

    oal_uint32                  is_bind;
};

#define HCC_BUS_SWITCH_STATE_START  0
#define HCC_BUS_SWITCH_STATE_END    1
#define HCC_BUS_SWITCH_STATE_ABORT  2

struct _hcc_bus_dev_
{
    oal_uint32                  dev_id;                     /*bus id, reference to the chip*/
    oal_uint32                  init_bus_type;              /*init select bus type*/
    oal_uint32                  bus_cap;                    /*support bus type*/

    oal_uint32                  bus_switch_enable;          /*1 for enable*/
    oal_uint32                  bus_auto_switch;             /*自动切换*/
    oal_wakelock_stru           st_switch_wakelock;         /*wake lock for switch*/

    oal_uint32                  bus_auto_bindcpu;             /*动态绑核*/

    oal_ulong                   bus_pps_start_time;
    oal_timer_list_stru         bus_pps_timer;

    oal_wait_queue_head_stru    st_swtich_ack_wq;

    oal_int32                   ul_wlan_irq;                  /*gpio for wlan transfer*/
    oal_int32                   ul_irq_stat;                  /*0:enable, 1:disable*/
    oal_int32                   is_wakeup_gpio_support;
    oal_spin_lock_stru          st_irq_lock;

    struct hcc_handler*         hcc;

    hcc_bus*                    cur_bus;

    oal_wait_queue_head_stru    st_switch_request_wq;
    hcc_bus*                    switch_bus_req;             /*bus switch req ptr*/
    oal_int32                   switch_state;
    oal_spin_lock_stru          st_switch_lock;
    struct task_struct *        pst_switch_task;

    oal_uint32                  bus_num;                    /*for each cap bit*/
    char*                       name;

    /*swtich info*/
    oal_completion              st_switch_powerup_ready;
    oal_completion              st_switch_powerdown_ready;
};

typedef struct _hcc_bus_res_{
    oal_uint32  cur_type;

    oal_spin_lock_stru  lock;
    oal_dlist_head_stru head;
}hcc_bus_res;

#define HCC_BUS_SWITCH_POWERUP      (1)
#define HCC_BUS_SWITCH_POWERDOWN    (2)

#define HCC_BUS_TRANSFER_SWITCH_ACTION (0)

typedef struct _hcc_switch_info_
{
    oal_uint8   action_type;/*action type*/
    oal_uint8   bus_id;   /*ip index, the next work ip*/
    oal_uint8   is_need_resp;
}hcc_switch_info;

typedef struct _hcc_switch_response_
{
    oal_uint8   action_type;/*action type*/
    oal_uint8   bus_id;   /*ip index*/
    oal_uint8   is_succ;  /**/
}hcc_switch_response;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern hcc_bus* hcc_get_current_110x_bus(oal_void);
extern oal_int32 hcc_bus_resource_alloc(hcc_bus* pst_bus);
extern oal_void hcc_bus_resource_free(hcc_bus* pst_bus);
extern hcc_bus_dev* hcc_get_bus_dev(oal_uint32 dev_id);
extern oal_int32  hcc_bus_bindcpu(hcc_bus *hi_bus, oal_uint32 chan, oal_int32 is_bind);
extern oal_int32 hcc_dev_bindcpu(oal_uint32 dev_id, oal_int32 is_bind);
extern oal_int32 hi110x_hcc_dev_bindcpu(oal_int32 is_bind);
#if defined(CONFIG_ARCH_HISI)
extern void hisi_get_slow_cpus(struct cpumask *cpumask);
extern void hisi_get_fast_cpus(struct cpumask *cpumask);
#endif


#define hcc_bus_wake_lock(pst_hi_bus) oal_wake_lock(&pst_hi_bus->st_bus_wakelock)


#define hcc_bus_wake_unlock(pst_hi_bus) oal_wake_unlock(&pst_hi_bus->st_bus_wakelock)


#define hcc_bus_wakelock_active(pst_hi_bus)   oal_wakelock_active(&pst_hi_bus->st_bus_wakelock)
extern struct task_struct *oal_thread_create_etc(int (*threadfn)(void *data),
                                        void                    *data,
                                        struct semaphore        *sema_sync,
                                        const char*               namefmt,
                                        oal_uint32                   policy,
                                        oal_int32                    prio,
                                        oal_int32                    cpuid);
extern void oal_thread_stop_etc( struct task_struct      *tsk,
                          struct semaphore        *sema_sync);
extern void oal_set_thread_property_etc(struct task_struct *p,int policy,
                                        const struct sched_param *param,
                                        long nice);

extern oal_int32 hcc_add_bus(hcc_bus* pst_bus, char* name);
extern oal_int32 hcc_remove_bus(hcc_bus* pst_bus);
extern hcc_bus* hcc_alloc_bus(oal_void);
extern oal_void hcc_free_bus(hcc_bus* pst_bus);
extern oal_void hcc_dev_exit(oal_void);
extern oal_int32 hcc_dev_init(oal_void);
extern oal_int32 oal_wifi_platform_load_dev(oal_void);
extern oal_void oal_wifi_platform_unload_dev(oal_void);
extern oal_int32 oal_trigger_bus_exception(hcc_bus *hi_bus, oal_int32 is_sync);
extern oal_int32 hcc_bus_exception_is_busy(hcc_bus *hi_bus);
extern oal_void hcc_bus_exception_submit(hcc_bus *hi_bus, oal_int32 excep_type);
extern oal_uint32 hcc_bus_dump_mem_check(hcc_bus *hi_bus);
extern oal_void hcc_bus_try_to_dump_device_mem(hcc_bus *hi_bus, oal_int32 is_sync);
extern oal_void hcc_bus_wakelocks_release_detect(hcc_bus *pst_bus);
extern oal_void oal_wlan_gpio_intr_enable_etc(hcc_bus_dev *pst_bus_dev,oal_uint32  ul_en);

extern oal_int32 hcc_bus_message_register(hcc_bus *hi_bus,oal_uint8 msg, hcc_bus_msg_rx cb, oal_void* data);
extern oal_void hcc_bus_message_unregister(hcc_bus *hi_bus,oal_uint8 msg);
extern oal_int32 hcc_bus_transfer_rx_register(hcc_bus *hi_bus, oal_void* data,hcc_bus_data_rx rx);
extern oal_void hcc_bus_transfer_rx_unregister(hcc_bus *hi_bus);
extern oal_int32 hcc_bus_cap_init(oal_uint32 dev_id, char* bus_select);
extern oal_uint64 oal_get_gpio_int_count_para_etc(oal_void);
extern oal_int32  hcc_switch_action_register(hcc_switch_action* action, void* data);
extern oal_void   hcc_switch_action_unregister(hcc_switch_action* action);
extern oal_int32 hcc_switch_bus(oal_uint32 dev_id, oal_uint32 bus_type);
extern oal_int32 hcc_send_message(struct hcc_handler* hcc, oal_uint32 val);
extern oal_int32 hcc_dev_switch_enable(oal_uint32 dev_id);
extern oal_int32 hcc_dev_switch_disable(oal_uint32 dev_id);
extern oal_int32 hi110x_switch_hcc_bus_request(oal_uint32 target);
extern oal_int32 hi110x_switch_to_hcc_highspeed_chan(oal_uint32 is_high);
extern oal_int32 hcc_bus_performance_core_schedule(oal_uint32 dev_id);
extern oal_int32 hcc_bus_performance_core_init(oal_uint32 dev_id);
extern oal_int32 hcc_bus_pm_wakeup_device(hcc_bus *hi_bus);

OAL_STATIC OAL_INLINE oal_void hcc_bus_rx_transfer_lock(hcc_bus *hi_bus)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return;
    }
    mutex_lock(&hi_bus->rx_transfer_lock);
#endif
}

OAL_STATIC OAL_INLINE oal_void hcc_bus_rx_transfer_unlock(hcc_bus *hi_bus)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return;
    }
    mutex_unlock(&hi_bus->rx_transfer_lock);
#endif
}

OAL_STATIC OAL_INLINE oal_uint32 hcc_bus_get_max_trans_size(hcc_bus* hi_bus)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return 0;
    }

    return hi_bus->cap.max_trans_size;
}


OAL_STATIC OAL_INLINE oal_int32 hcc_bus_get_state(hcc_bus *hi_bus, oal_uint32 mask)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_get_state: hibus is null");
        return 0;
    }

    if(OAL_WARN_ON(NULL == hi_bus->opt_ops))
    {
         oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_get_state: hi_bus->opt_ops is null");
        return 0;
    }

    if(NULL == hi_bus->opt_ops->get_bus_state)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_get_state: hi_bus->opt_ops->get_bus_state is null");
        return 0;
    }

    return hi_bus->opt_ops->get_bus_state(hi_bus, mask);
}


OAL_STATIC OAL_INLINE oal_void hcc_bus_enable_state(hcc_bus *hi_bus, oal_uint32 mask)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return;
    }

    if(OAL_WARN_ON(NULL == hi_bus->opt_ops))
    {
        return;
    }

    if(NULL == hi_bus->opt_ops->enable_bus_state)
    {
        return;
    }

    hi_bus->opt_ops->enable_bus_state(hi_bus, mask);

}

OAL_STATIC OAL_INLINE oal_void hcc_bus_disable_state(hcc_bus *hi_bus, oal_uint32 mask)
{

    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return;
    }

    if(OAL_WARN_ON(NULL == hi_bus->opt_ops))
    {
        return;
    }

    if(NULL == hi_bus->opt_ops->disable_bus_state)
    {
        return;
    }

    hi_bus->opt_ops->disable_bus_state(hi_bus, mask);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_lock(hcc_bus *hi_bus)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_ENODEV;
    }

    if(OAL_WARN_ON(NULL == hi_bus->opt_ops))
    {
        return -OAL_ENODEV;
    }

    if(NULL == hi_bus->opt_ops->lock)
    {
        return -OAL_EIO;
    }

    return hi_bus->opt_ops->lock(hi_bus);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_unlock(hcc_bus *hi_bus)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_ENODEV;
    }

    if(OAL_WARN_ON(NULL == hi_bus->opt_ops))
    {
        return -OAL_ENODEV;
    }

    if(NULL == hi_bus->opt_ops->unlock)
    {
        return -OAL_EIO;
    }

    return hi_bus->opt_ops->unlock(hi_bus);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_sleep_request(hcc_bus *hi_bus)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_ENODEV;
    }

    if(OAL_WARN_ON(NULL == hi_bus->opt_ops))
    {
        return -OAL_ENODEV;
    }

    if(NULL == hi_bus->opt_ops->sleep_request)
    {
        return -OAL_EIO;
    }

    return hi_bus->opt_ops->sleep_request(hi_bus);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_sleep_request_host(hcc_bus *hi_bus)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_ENODEV;
    }

    if(OAL_WARN_ON(NULL == hi_bus->opt_ops))
    {
        return -OAL_ENODEV;
    }

    if(NULL == hi_bus->opt_ops->sleep_request_host)
    {
        return -OAL_EIO;
    }

    return hi_bus->opt_ops->sleep_request_host(hi_bus);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_get_sleep_state(hcc_bus *hi_bus)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_ENODEV;
    }

    if(OAL_WARN_ON(NULL == hi_bus->opt_ops))
    {
        return -OAL_ENODEV;
    }

    if(NULL == hi_bus->opt_ops->get_sleep_state)
    {
        return -OAL_EIO;
    }

    return hi_bus->opt_ops->get_sleep_state(hi_bus);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_rx_int_mask(hcc_bus *hi_bus)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_ENODEV;
    }

    if(OAL_WARN_ON(NULL == hi_bus->opt_ops))
    {
        return -OAL_ENODEV;
    }

    if(NULL == hi_bus->opt_ops->rx_int_mask)
    {
        return -OAL_EIO;
    }

    return hi_bus->opt_ops->rx_int_mask(hi_bus, 1);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_rx_int_unmask(hcc_bus *hi_bus)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_ENODEV;
    }

    if(OAL_WARN_ON(NULL == hi_bus->opt_ops))
    {
        return -OAL_ENODEV;
    }

    if(NULL == hi_bus->opt_ops->rx_int_mask)
    {
        return -OAL_EIO;
    }

    return hi_bus->opt_ops->rx_int_mask(hi_bus, 0);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_get_trans_count(hcc_bus *hi_bus, oal_uint64 *tx, oal_uint64 *rx)
{
    if(OAL_LIKELY(tx))
    {
        *tx = 0;
    }

    if(OAL_LIKELY(rx))
    {
        *rx = 0;
    }

    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_ENODEV;
    }

    if(NULL == hi_bus->opt_ops->get_trans_count)
    {
        return -OAL_EIO;
    }

    return hi_bus->opt_ops->get_trans_count(hi_bus, tx, rx);

}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_wakeup_request(hcc_bus *hi_bus)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_ENODEV;
    }

    if(OAL_WARN_ON(NULL == hi_bus->opt_ops))
    {
        return -OAL_ENODEV;
    }

    if(NULL == hi_bus->opt_ops->wakeup_request)
    {
        return -OAL_EIO;
    }

    return hi_bus->opt_ops->wakeup_request(hi_bus);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_power_action(hcc_bus *hi_bus, HCC_BUS_POWER_ACTION_TYPE action)
{
    if(OAL_UNLIKELY(NULL == hi_bus))
    {
        return -OAL_ENODEV;
    }

    if(OAL_WARN_ON(NULL == hi_bus->opt_ops))
    {
        return -OAL_ENODEV;
    }

    if(NULL == hi_bus->opt_ops->power_action)
    {
        return -OAL_EIO;
    }

    return hi_bus->opt_ops->power_action(hi_bus, action);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_power_ctrl_register(hcc_bus *hi_bus, HCC_BUS_POWER_CTRL_TYPE ctrl, int (*func)(void* data), void* data)
{
    if(OAL_UNLIKELY(NULL == hi_bus))
    {
        if(func)
        {
            /*ip driver no callback, call the function directly*/
            return func(data);
        }
        return -OAL_ENODEV;
    }

    if(OAL_WARN_ON(NULL == hi_bus->opt_ops))
    {
        return -OAL_ENODEV;
    }

    if(NULL == hi_bus->opt_ops->power_ctrl)
    {
        if(func)
        {
            /*ip driver no callback, call the function directly*/
            return func(data);
        }
        return -OAL_EIO;
    }

    return hi_bus->opt_ops->power_ctrl(hi_bus, ctrl, func, data);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_send_message(hcc_bus *hi_bus, oal_uint32 val)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_EINVAL;
    }

    if(OAL_LIKELY(hi_bus->opt_ops->send_msg_etc))
    {
        return hi_bus->opt_ops->send_msg_etc(hi_bus, val);
    }
    else
    {
        return -OAL_ENODEV;
    }
}

extern oal_uint32 g_hcc_tx_err_cnt;
OAL_STATIC OAL_INLINE oal_int32 hcc_bus_tx_netbuf_list(hcc_bus *hi_bus, oal_netbuf_head_stru* pst_head, hcc_netbuf_queue_type qtype)
{
    oal_int32 ret;

    if(OAL_UNLIKELY(!hi_bus))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_tx_netbuf_list: bus is null");
        return-OAL_EINVAL ;
    }

    if(OAL_LIKELY(hi_bus->opt_ops->tx_netbuf_list))
    {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "tx netbuf list:%s, skb head:%p, len:%d, qtype:%d",
                                                    hi_bus->bus_type == HCC_BUS_SDIO ? "sdio":"pcie",
                                                    OAL_NETBUF_HEAD_NEXT(pst_head),
                                                    oal_netbuf_list_len(pst_head),
                                                    qtype);

        ret = hi_bus->opt_ops->tx_netbuf_list(hi_bus, pst_head, qtype);
        if(OAL_UNLIKELY(ret < 0))
        {
            g_hcc_tx_err_cnt++;
            if(g_hcc_tx_err_cnt >= 2)
            {
                oal_msleep(1000);
            }
            else
            {
                oal_msleep(100);
            }
            return 0;
        }
        else
        {
            if(OAL_UNLIKELY(g_hcc_tx_err_cnt))
                g_hcc_tx_err_cnt = 0;
            return ret;
        }
    }
    else
    {
        return 0;
    }
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_rx_netbuf_list(hcc_bus *hi_bus, oal_netbuf_head_stru* pst_head)
{
    if(OAL_UNLIKELY(!hi_bus))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc_bus_rx_netbuf_list: bus is null");
        return-OAL_EINVAL ;
    }

    if(OAL_LIKELY(hi_bus->opt_ops->rx_netbuf_list))
    {
        return hi_bus->opt_ops->rx_netbuf_list(hi_bus, pst_head);
    }
    else
    {
        return -OAL_ENODEV;
    }
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_reinit(hcc_bus *hi_bus)
{
    if(OAL_UNLIKELY(NULL == hi_bus))
    {
        return -OAL_EINVAL;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->opt_ops->reinit))
    {
        return -OAL_ENODEV;
    }

    return hi_bus->opt_ops->reinit(hi_bus);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_voltage_bias_init(hcc_bus *hi_bus)
{
    if(OAL_UNLIKELY(NULL == hi_bus))
    {
        return -OAL_EINVAL;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->opt_ops->voltage_bias_init))
    {
        return -OAL_ENODEV;
    }

    return hi_bus->opt_ops->voltage_bias_init(hi_bus);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_check_tx_condition(hcc_bus* hi_bus, hcc_netbuf_queue_type qtype)
{
    if(OAL_UNLIKELY(NULL == hi_bus))
    {
        return OAL_TRUE;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->opt_ops->tx_condition))
    {
        /*没有回调说明 发送不受限制 直接返回1*/
        return OAL_TRUE;
    }

    return hi_bus->opt_ops->tx_condition(hi_bus, qtype);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_patch_read(hcc_bus *hi_bus, oal_uint8* buff, oal_int32 len, oal_uint32 timeout)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_EINVAL;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->opt_ops->patch_read))
    {

        return -OAL_EIO;
    }

    return hi_bus->opt_ops->patch_read(hi_bus, buff, len, timeout);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_patch_write(hcc_bus *hi_bus, oal_uint8* buff, oal_int32 len)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_EINVAL;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->opt_ops->patch_write))
    {

        return -OAL_EIO;
    }

    return hi_bus->opt_ops->patch_write(hi_bus, buff, len);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_wakeup_complete(hcc_bus *hi_bus)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_EINVAL;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->opt_ops->wakeup_complete))
    {
        return -OAL_EIO;
    }

    return hi_bus->opt_ops->wakeup_complete(hi_bus);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_switch_suspend_tx(hcc_bus *hi_bus)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_EINVAL;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->opt_ops->switch_suspend_tx))
    {
        return -OAL_EIO;
    }

    return hi_bus->opt_ops->switch_suspend_tx(hi_bus);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_switch_resume_tx(hcc_bus *hi_bus)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_EINVAL;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->opt_ops->switch_resume_tx))
    {
        return OAL_SUCC;
    }

    return hi_bus->opt_ops->switch_resume_tx(hi_bus);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_switch_clean_res(hcc_bus *hi_bus)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return -OAL_EINVAL;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->opt_ops->switch_clean_res))
    {
        return OAL_SUCC;
    }

    return hi_bus->opt_ops->switch_clean_res(hi_bus);
}

OAL_STATIC OAL_INLINE oal_void hcc_bus_chip_info(hcc_bus *hi_bus)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->opt_ops->chip_info))
    {
        return;
    }

    hi_bus->opt_ops->chip_info(hi_bus);
}

#define HCC_PRINT_TRANS_FLAG_DEVICE_STAT   (1 << 0)
#define HCC_PRINT_TRANS_FLAG_DEVICE_REGS   (1 << 1)
OAL_STATIC OAL_INLINE oal_void hcc_bus_print_trans_info(hcc_bus *hi_bus, oal_uint64 print_flag)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->opt_ops->print_trans_info))
    {
        return;
    }

    /*打印device信息要保证打印过程中 不会进入深睡*/
    hi_bus->opt_ops->print_trans_info(hi_bus, print_flag);
}

OAL_STATIC OAL_INLINE oal_void hcc_bus_reset_trans_info(hcc_bus *hi_bus)
{
    if(OAL_WARN_ON(NULL == hi_bus))
    {
        return;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->opt_ops->reset_trans_info))
    {
        return;
    }

    hi_bus->opt_ops->reset_trans_info(hi_bus);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_pending_signal_check(hcc_bus *hi_bus)
{
    if(OAL_UNLIKELY(NULL == hi_bus))
    {
        return OAL_FALSE;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->opt_ops->pending_signal_check))
    {
        return OAL_FALSE;
    }

    return hi_bus->opt_ops->pending_signal_check(hi_bus);
}

OAL_STATIC OAL_INLINE oal_int32 hcc_bus_pending_signal_process(hcc_bus *hi_bus)
{
    if(OAL_UNLIKELY(NULL == hi_bus))
    {
        return 0;
    }

    if(OAL_UNLIKELY(NULL == hi_bus->opt_ops->pending_signal_process))
    {
        return 0;
    }

    return hi_bus->opt_ops->pending_signal_process(hi_bus);
}

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of Hcc.h */

