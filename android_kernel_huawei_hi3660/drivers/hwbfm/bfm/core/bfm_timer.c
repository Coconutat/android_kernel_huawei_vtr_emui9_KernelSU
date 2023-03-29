/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfm_timer.c

    @brief: define the basic external enum/macros/interface for BFM (Boot Fail Monitor)

    @version: 2.0

    @author: QiDechun ID: 216641

    @date: 2016-08-17

    @history:
*/

/*----includes-----------------------------------------------------------------------*/

#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/stat.h>
#include <linux/io.h>
#include <asm/uaccess.h>
#include <linux/of_fdt.h>
#include <linux/list.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <chipset_common/bfmr/public/bfmr_public.h>
#include <chipset_common/bfmr/common/bfmr_common.h>
#include <chipset_common/bfmr/bfm/core/bfm_timer.h>
#include <chipset_common/bfmr/bfm/core/bfm_core.h>


/*----local macroes-----------------------------------------------------------------*/

#define BOOT_TIMER_COUNT_STEP ((unsigned int)5000) /* unit: msec */
#define BFMR_TIMEOUT_VALUE_FOR_SHORT_TIMER ((u32)(1000 * 60 * 10)) /* unit: msec */
#define BFMR_TIMEOUT_VALUE_FOR_LONG_TIMER ((u32)(1000 * 60 * 30)) /* unit: msec */
#define BFMR_MAX_TIMEOUT_VALUE_FOR_ACTION_TIMER ((u32)(1000 * 60 * 30)) /* unit: msec */
#define BFMR_VENDOR_HUAWEI_FLAG "/eng"


/*----local prototypes---------------------------------------------------------------*/

struct bfm_boot_timer_info
{
    unsigned long short_timer_last_start_time;
    unsigned int count_step_value;
    struct task_struct *boot_time_task;
    struct timer_list boot_timer;
    struct completion boot_complete;
    bool boot_timer_count_enable;
    bool boot_timer_count_task_should_run;
    u32 long_timer_timeout_value;
    u32 short_timer_timeout_value;
};

struct action_timer {
    struct list_head  act_list;
    char action_name[BFMR_ACTION_NAME_LEN];
    u32 action_timer_timeout_value;
    u32 action_timer_timing_count;
    unsigned long last_start_time;
    pid_t action_pid;
    char action_comm[TASK_COMM_LEN];
};

/*----local variables------------------------------------------------------------------*/

static struct bfm_boot_timer_info *s_boot_timer_info = NULL;
static struct mutex s_boot_timer_mutex;
static LIST_HEAD(action_timer_active_list);
static LIST_HEAD(action_timer_pause_list);
static u32 short_timer_timing_count = 0U;


/*----global variables-----------------------------------------------------------------*/


/*----global function prototypes---------------------------------------------------------*/


/*----local function prototypes---------------------------------------------------------*/

static void bfm_check_boot_time(struct bfm_boot_timer_info *boot_timer_info);
static void bfm_wakeup_boot_timer_count_task(unsigned long data);
static __ref int bfm_boot_timer_count_kthread(void *arg);
static void __bfm_free_action_timer_list(struct list_head *atlist);
static inline struct action_timer *__bfm_get_action_timer(char *act_name, struct list_head *atlist);

/*----function definitions--------------------------------------------------------------*/

static void bfm_check_boot_time(struct bfm_boot_timer_info *boot_timer_info)
{
    static u32 long_timer_timing_count = 0U;
    struct action_timer  *atimer;

    long_timer_timing_count += boot_timer_info->count_step_value;
    if (boot_timer_info->boot_timer_count_enable)
    {
        BFMR_PRINT_SIMPLE_INFO("hwboot_timer:update timer [long and short]!\n");
        short_timer_timing_count  += jiffies_to_msecs(jiffies - boot_timer_info->short_timer_last_start_time);
        boot_timer_info->short_timer_last_start_time = jiffies;
    }
    else
    {
        BFMR_PRINT_SIMPLE_INFO("hwboot_timer:update timer [only long]!\n");
    }

    if ((short_timer_timing_count >= boot_timer_info->short_timer_timeout_value)
        || (long_timer_timing_count >= boot_timer_info->long_timer_timeout_value))
    {
        BFMR_PRINT_SIMPLE_INFO("hwboot_timer: %u minutes timer expired! boot fail!\n",
            (long_timer_timing_count >= boot_timer_info->long_timer_timeout_value)
            ? (boot_timer_info->long_timer_timeout_value / (unsigned int)(1000 * 60))
            : (boot_timer_info->short_timer_timeout_value / (unsigned int)(1000 * 60)));
        boot_timer_info->boot_timer_count_task_should_run = false;
        mutex_unlock(&s_boot_timer_mutex);

        if (bfmr_is_dir_existed(BFMR_VENDOR_HUAWEI_FLAG))
        {
            bfm_send_signal_to_init();//for dump trace of init Process.

            boot_fail_err(KERNEL_BOOT_TIMEOUT, NO_SUGGESTION, NULL);
        }
    }

    list_for_each_entry(atimer, &action_timer_active_list, act_list) {
        BFMR_PRINT_SIMPLE_INFO("hwboot_timer:update timer [%s]!\n",atimer->action_name);
        /*updata action timer*/
        atimer->action_timer_timing_count += jiffies_to_msecs(jiffies - atimer->last_start_time);
        atimer->last_start_time = jiffies;

        if (atimer->action_timer_timing_count >= atimer->action_timer_timeout_value) {
            BFMR_PRINT_SIMPLE_INFO("hwboot_timer: %s timer expired! boot fail!\n",atimer->action_name);
            boot_timer_info->boot_timer_count_task_should_run = false;
            mutex_unlock(&s_boot_timer_mutex);

            if (bfmr_is_dir_existed(BFMR_VENDOR_HUAWEI_FLAG))
            {
                bfm_send_signal_to_init();

                boot_fail_err(KERNEL_BOOT_TIMEOUT, NO_SUGGESTION, NULL);
            }
        }
    }
}


static void bfm_wakeup_boot_timer_count_task(unsigned long data)
{
    struct bfm_boot_timer_info *boot_timer_info = (struct bfm_boot_timer_info *)data;
    complete(&boot_timer_info->boot_complete);
}


static __ref int bfm_boot_timer_count_kthread(void *arg)
{
    struct bfm_boot_timer_info *boot_timer_info = (struct bfm_boot_timer_info *)arg;
    unsigned long delay_time = 0;
    struct sched_param param = {.sched_priority = MAX_RT_PRIO - 1};

    sched_setscheduler(current, SCHED_FIFO, &param);
    while (!kthread_should_stop())
    {
        while (wait_for_completion_interruptible(&boot_timer_info->boot_complete) != 0)
        {
            ;
        }
        reinit_completion(&boot_timer_info->boot_complete);

        mutex_lock(&s_boot_timer_mutex);
        delay_time = msecs_to_jiffies(boot_timer_info->count_step_value);
        if (boot_timer_info->boot_timer_count_task_should_run)
        {
            bfm_check_boot_time(boot_timer_info);
            mod_timer(&boot_timer_info->boot_timer, jiffies + delay_time);
        }
        else
        {
            BFMR_PRINT_SIMPLE_INFO("hwboot_timer: time counting thread will stop now:[boot_short_timer:%d]!\n",
                                    short_timer_timing_count);
            BFMR_PRINT_KEY_INFO("start free action_timer_active_list!\n");
            __bfm_free_action_timer_list(&action_timer_active_list);
            BFMR_PRINT_KEY_INFO("start free action_timer_pause_list!\n");
            __bfm_free_action_timer_list(&action_timer_pause_list);
            s_boot_timer_info = NULL;
            del_timer_sync(&boot_timer_info->boot_timer);
            bfmr_free(boot_timer_info);
            mutex_unlock(&s_boot_timer_mutex);
            break;
        }
        mutex_unlock(&s_boot_timer_mutex);
    }

    return 0;
}


/**
    @function: int bfm_get_boot_timer_state(int *state)
    @brief: get state of the boot timer.

    @param: state [out], the state of the boot timer.

    @return: 0 - success, -1 - failed.

    @note:
        1. this fuction only need be initialized in kernel.
        2. if *state == 0, the boot timer is disabled, if *state == 1, the boot timer is enbaled.
*/
int bfm_get_boot_timer_state(int *state)
{
    int ret = -1;

    if (unlikely(NULL == state))
    {
        return -1;
    }

    mutex_lock(&s_boot_timer_mutex);
    if (likely(NULL != s_boot_timer_info))
    {
        *state = (s_boot_timer_info->boot_timer_count_enable) ? 1 : 0;
        ret = 0;
    }
    else
    {
        *state = 0;
    }
    mutex_unlock(&s_boot_timer_mutex);

    return ret;
}
EXPORT_SYMBOL(bfm_get_boot_timer_state);


/**
    @function: int bfm_suspend_boot_timer(void)
    @brief: suspend the boot timer.

    @param: none.

    @return: 0 - success, -1 - failed.

    @note:
*/
int bfm_suspend_boot_timer(void)
{
    int ret = -1;

    BFMR_PRINT_KEY_INFO("disable short timer!\n");
    mutex_lock(&s_boot_timer_mutex);
    if (likely(NULL != s_boot_timer_info) && s_boot_timer_info->boot_timer_count_enable)
    {
        s_boot_timer_info->boot_timer_count_enable = false;
        short_timer_timing_count += jiffies_to_msecs(jiffies - s_boot_timer_info->short_timer_last_start_time);
        ret = 0;
    }
    mutex_unlock(&s_boot_timer_mutex);

    return ret;
}
EXPORT_SYMBOL(bfm_suspend_boot_timer);


/**
    @function: int bfmr_resume_boot_timer(void)
    @brief: resume the boot timer.

    @param: none.

    @return: 0 - success, -1 - failed.

    @note:
*/
int bfmr_resume_boot_timer(void)
{
    int ret = -1;

    BFMR_PRINT_KEY_INFO("enable short timer!\n");
    mutex_lock(&s_boot_timer_mutex);
    if (likely(NULL != s_boot_timer_info) && !s_boot_timer_info->boot_timer_count_enable)
    {
        s_boot_timer_info->boot_timer_count_enable = true;
        s_boot_timer_info->short_timer_last_start_time = jiffies;
        ret = 0;
    }
    mutex_unlock(&s_boot_timer_mutex);

    return ret;
}
EXPORT_SYMBOL(bfmr_resume_boot_timer);


/**
 ***************************** !!!CAUTION!!!!***************************************************
 * __bfm_XXX, such as this fuction is called as lib, pls check your input parameter in up level.
*/
static void __bfm_free_action_timer_list(struct list_head *atlist)
{
    struct action_timer  *atimer;
    struct list_head *pos, *next;

    list_for_each_safe(pos, next, atlist) {
        atimer = list_entry(pos, struct action_timer, act_list);
        BFMR_PRINT_KEY_INFO("WARNING:[%s:%d]%s act_timer left in list,so clear it!(activetime:%d ms, timeout:%d ms)\n",
                    atimer->action_comm, atimer->action_pid, atimer->action_name,
                    atimer->action_timer_timing_count, atimer->action_timer_timeout_value);
        list_del(&atimer->act_list);
        bfmr_free(atimer);
    }
    return;
}

static inline struct action_timer *__bfm_get_action_timer(char *act_name, struct list_head *atlist)
{
    struct action_timer  *atimer;

    list_for_each_entry(atimer, atlist, act_list) {
        if (!strncmp(act_name, atimer->action_name, sizeof(atimer->action_name))) {
            return atimer;
        }
    }
    return NULL;
}

/**
    @function: int bfmr_action_timer_start(char *act_name, unsigned int timeout_value)
    @brief:
    1. creat a new action timer and start the timer
    2. stop the short boot timer

    @param: none.

    @return: 0 - success, -1 - failed.

    @note:
*/
int bfm_action_timer_start(char *act_name, unsigned int timeout_value)
{
    int ret = -1;
    struct action_timer  *atimer;
    char task_comm_tmp[TASK_COMM_LEN];
    pid_t task_pid_tmp;

    if (NULL == act_name) {
        BFMR_PRINT_INVALID_PARAMS("act_name.\n");
        return ret;
    }

    if (timeout_value > BFMR_MAX_TIMEOUT_VALUE_FOR_ACTION_TIMER) {
        timeout_value = BFMR_MAX_TIMEOUT_VALUE_FOR_ACTION_TIMER;
        BFMR_PRINT_INVALID_PARAMS("!!WARNING!!timeout value too large! timeout_value:%d\n",
                                    timeout_value);
    }

    /*get current task info*/
    get_task_comm(task_comm_tmp, current);
    task_pid_tmp = task_pid_nr(current);

    BFMR_PRINT_KEY_INFO("[%s:%d]%s timer start-->\n", task_comm_tmp, task_pid_tmp, act_name);
    mutex_lock(&s_boot_timer_mutex);

    /*0. check if the timer already start */
    if ((atimer = __bfm_get_action_timer(act_name, &action_timer_active_list)) ||
        (atimer = __bfm_get_action_timer(act_name, &action_timer_pause_list))) {
        BFMR_PRINT_KEY_INFO("[%s:%d]%s timer already start!\n", atimer->action_comm, atimer->action_pid, act_name);
        goto __out;
    }

    /*1.init & start action timer*/
    atimer  = (struct action_timer *)bfmr_malloc(sizeof(struct action_timer));
    if (!atimer) {
        goto __out;
    }
    strncpy(atimer->action_name, act_name, sizeof(atimer->action_name));
    atimer->action_name[BFMR_ACTION_NAME_LEN-1] = '\0';
    atimer->action_timer_timeout_value = timeout_value;
    atimer->last_start_time = jiffies;
    atimer->action_timer_timing_count = 0;
    atimer->action_pid = task_pid_tmp;
    strncpy(atimer->action_comm, task_comm_tmp, sizeof(atimer->action_comm));
    atimer->action_comm[TASK_COMM_LEN-1] = '\0';
    list_add_tail(&atimer->act_list, &action_timer_active_list);

    /*2.suspend short boot timer*/
    if (likely(NULL != s_boot_timer_info) && s_boot_timer_info->boot_timer_count_enable) {
        /*stop short boot timer*/
        s_boot_timer_info->boot_timer_count_enable = false;
        short_timer_timing_count += jiffies_to_msecs(jiffies - s_boot_timer_info->short_timer_last_start_time);
        ret = 0;
    }

__out:
    mutex_unlock(&s_boot_timer_mutex);
    return ret;
}
EXPORT_SYMBOL(bfm_action_timer_start);


/**
    @function: int bfmr_action_timer_stop(char *act_name)
    @brief:
    1. stop & destroy the action timer
    2. resume the short boot timer if the action timer list is empty

    @param: none.

    @return: 0 - success, -1 - failed.

    @note:
*/
int bfm_action_timer_stop(char *act_name)
{
    struct action_timer  *atimer;
    int ret = -1;
    char task_comm_tmp[TASK_COMM_LEN];
    pid_t task_pid_tmp;

    if (NULL == act_name)
    {
        BFMR_PRINT_INVALID_PARAMS("act_name.\n");
        return ret;
    }

    /*get current task info*/
    get_task_comm(task_comm_tmp, current);
    task_pid_tmp = task_pid_nr(current);

    BFMR_PRINT_KEY_INFO("[%s:%d]%s timer stop-->\n", task_comm_tmp, task_pid_tmp, act_name);
    mutex_lock(&s_boot_timer_mutex);
    if ((atimer = __bfm_get_action_timer(act_name, &action_timer_active_list)) ||
        (atimer = __bfm_get_action_timer(act_name, &action_timer_pause_list))) {

            BFMR_PRINT_KEY_INFO("have find out \"%s\" timer,so stop it!(activetime:%d ms, timeout:%d ms)\n",
                           act_name, atimer->action_timer_timing_count, atimer->action_timer_timeout_value);
            if (atimer->action_pid != task_pid_tmp)
            {
                BFMR_PRINT_ERR("WARNING: pid mismatch stop[%s:%d]: start[%s:%d]!!",
                            task_comm_tmp, task_pid_tmp, atimer->action_comm, atimer->action_pid);
            }
            list_del(&atimer->act_list);
            bfmr_free(atimer);
    }

    /*resume the short boot timer if needed*/
    if (likely(NULL != s_boot_timer_info))
    {
        if(list_empty(&action_timer_active_list) && !s_boot_timer_info->boot_timer_count_enable) {
            /*resume short boot timer*/
            s_boot_timer_info->boot_timer_count_enable = true;
            s_boot_timer_info->short_timer_last_start_time = jiffies;
        }
        ret = 0;
    }
    mutex_unlock(&s_boot_timer_mutex);

    return ret;
}
EXPORT_SYMBOL(bfm_action_timer_stop);

int bfm_action_timer_pause(char *act_name)
{
    struct action_timer  *atimer;
    int ret = -1;
    char task_comm_tmp[TASK_COMM_LEN];
    pid_t task_pid_tmp;

    if (NULL == act_name)
    {
        BFMR_PRINT_INVALID_PARAMS("act_name.\n");
        return ret;
    }

    /*get current task info*/
    get_task_comm(task_comm_tmp, current);
    task_pid_tmp = task_pid_nr(current);

    BFMR_PRINT_KEY_INFO("[%s:%d]%s timer pause-->\n", task_comm_tmp, task_pid_tmp, act_name);
    mutex_lock(&s_boot_timer_mutex);
    atimer = __bfm_get_action_timer(act_name, &action_timer_active_list);
    if (!atimer) {
        BFMR_PRINT_KEY_INFO("\"%s\" timer is not in running quene!\n",act_name);
        goto __out;
    }
    BFMR_PRINT_KEY_INFO("find out \"%s\" timer is running, so pause it!\n",act_name);
    if (atimer->action_pid != task_pid_tmp) {
                BFMR_PRINT_ERR("WARNING: pid mismatch pause[%s:%d]: start[%s:%d]!!",
                            task_comm_tmp, task_pid_tmp, atimer->action_comm, atimer->action_pid);
    }

    /*pasue the action timer at2*/
    atimer->action_timer_timing_count += jiffies_to_msecs(jiffies - atimer->last_start_time);
    list_move_tail(&atimer->act_list, &action_timer_pause_list);

    if (likely(NULL != s_boot_timer_info)) {
        if(list_empty(&action_timer_active_list) && !s_boot_timer_info->boot_timer_count_enable) {
            /*resume short boot timer*/
            s_boot_timer_info->boot_timer_count_enable = true;
            s_boot_timer_info->short_timer_last_start_time = jiffies;
        }
        ret = 0;
    }

__out:
    mutex_unlock(&s_boot_timer_mutex);
    return ret;
}
EXPORT_SYMBOL(bfm_action_timer_pause);

int bfm_action_timer_resume(char *act_name)
{
    struct action_timer  *atimer;
    int ret = -1;
    char task_comm_tmp[TASK_COMM_LEN];
    pid_t task_pid_tmp;

    if (NULL == act_name)
    {
        BFMR_PRINT_INVALID_PARAMS("act_name.\n");
        return ret;
    }

    /*get current task info*/
    get_task_comm(task_comm_tmp, current);
    task_pid_tmp = task_pid_nr(current);

    BFMR_PRINT_KEY_INFO("[%s:%d]%s timer resume-->\n", task_comm_tmp, task_pid_tmp, act_name);
    mutex_lock(&s_boot_timer_mutex);
    atimer = __bfm_get_action_timer(act_name, &action_timer_pause_list);
    if (!atimer) {
        BFMR_PRINT_KEY_INFO("\"%s\" timer is not in pause quene!\n",act_name);
        goto __out;
    }
    BFMR_PRINT_KEY_INFO("find out \"%s\" timer is paused, so resume it!\n",act_name);
    if (atimer->action_pid != task_pid_tmp) {
                BFMR_PRINT_ERR("WARNING: pid mismatch resume[%s:%d]: start[%s:%d]!!",
                            task_comm_tmp, task_pid_tmp, atimer->action_comm, atimer->action_pid);
    }

    /*resume the action timer*/
    atimer->last_start_time = jiffies;
    list_move_tail(&atimer->act_list, &action_timer_active_list);
    if (likely(NULL != s_boot_timer_info)) {
        if(!list_empty(&action_timer_active_list) && s_boot_timer_info->boot_timer_count_enable) {
            /*stop short boot timer*/
            s_boot_timer_info->boot_timer_count_enable = false;
            short_timer_timing_count += jiffies_to_msecs(jiffies - s_boot_timer_info->short_timer_last_start_time);
        }
        ret = 0;
    }

__out:
    mutex_unlock(&s_boot_timer_mutex);
    return ret;
}
EXPORT_SYMBOL(bfm_action_timer_resume);

/**
    @function: int bfm_set_boot_timer_timeout_value(unsigned int timeout_value)
    @brief: set timeout value of the boot timer.

    @param: timeout_value [in], timeout value to be set, unit:msec.

    @return: 0 - success, -1 - failed.

    @note:
        1. this fuction only need be initialized in kernel.
*/
int bfm_set_boot_timer_timeout_value(unsigned int timeout_value)
{
    int ret = -1;

    BFMR_PRINT_KEY_INFO("update timeout value to: %d!\n", timeout_value);
    mutex_lock(&s_boot_timer_mutex);
    if (likely(NULL != s_boot_timer_info))
    {
        s_boot_timer_info->short_timer_timeout_value = timeout_value;
        ret = 0;
    }
    mutex_unlock(&s_boot_timer_mutex);

    return ret;
}
EXPORT_SYMBOL(bfm_set_boot_timer_timeout_value);


/**
    @function: int bfm_get_boot_timer_timeout_value(unsigned int *ptimeout_value)
    @brief: get timeout value of the boot timer.

    @param: ptimeout_value [out], timeout value of the timer, unit:msec.

    @return: 0 - success, -1 - failed.

    @note:
        1. this fuction only need be initialized in kernel.
*/
int bfm_get_boot_timer_timeout_value(unsigned int *ptimeout_value)
{
    int ret = -1;

    if (unlikely(NULL == ptimeout_value))
    {
        return -1;
    }

    mutex_lock(&s_boot_timer_mutex);
    if (likely(NULL != s_boot_timer_info))
    {
        *ptimeout_value = s_boot_timer_info->short_timer_timeout_value;
        ret = 0;
    }
    mutex_unlock(&s_boot_timer_mutex);

    return ret;
}
EXPORT_SYMBOL(bfm_get_boot_timer_timeout_value);


/**
    @function: int bfm_stop_boot_timer(void)
    @brief: stop the boot timer.

    @param: none.

    @return: 0 - success, -1 - failed.

    @note: this fuction only need be initialized in kernel.
*/
int bfm_stop_boot_timer(void)
{
    int ret = -1;

    BFMR_PRINT_KEY_INFO("stop timer!\n");
    mutex_lock(&s_boot_timer_mutex);
    if (likely(NULL != s_boot_timer_info))
    {
        s_boot_timer_info->boot_timer_count_task_should_run = false;
        ret = 0;
    }
    mutex_unlock(&s_boot_timer_mutex);

    return ret;
}
EXPORT_SYMBOL(bfm_stop_boot_timer);


/**
    @function: int bfm_init_boot_timer(void)
    @brief: init the boot timer.

    @param: none.

    @return: 0 - success, -1 - failed.

    @note: this fuction only need be initialized in kernel.
*/
int bfm_init_boot_timer(void)
{
    int ret = 0;
    unsigned long delay_time = 0;
    struct bfm_boot_timer_info *boot_timer_info;

    boot_timer_info = bfmr_malloc(sizeof(struct bfm_boot_timer_info));
    if (NULL == boot_timer_info)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        return -EIO;
    }

    s_boot_timer_info = boot_timer_info;
    boot_timer_info->boot_time_task = kthread_create(bfm_boot_timer_count_kthread,
        boot_timer_info, "hwboot_time");
    if (IS_ERR(boot_timer_info->boot_time_task))
    {
        ret = PTR_ERR(boot_timer_info->boot_time_task);
        goto err;
    }

    boot_timer_info->count_step_value = BOOT_TIMER_COUNT_STEP;
    boot_timer_info->long_timer_timeout_value = BFMR_TIMEOUT_VALUE_FOR_LONG_TIMER;
    boot_timer_info->short_timer_timeout_value = BFMR_TIMEOUT_VALUE_FOR_SHORT_TIMER;
    boot_timer_info->boot_timer_count_enable = true;
    boot_timer_info->boot_timer_count_task_should_run = true;
    mutex_init(&s_boot_timer_mutex);

    delay_time = msecs_to_jiffies(boot_timer_info->count_step_value);
    init_completion(&boot_timer_info->boot_complete);
    wake_up_process(boot_timer_info->boot_time_task);
    init_timer(&boot_timer_info->boot_timer);
    boot_timer_info->boot_timer.data = (unsigned long)boot_timer_info;
    boot_timer_info->boot_timer.function = bfm_wakeup_boot_timer_count_task;
    boot_timer_info->boot_timer.expires = jiffies + delay_time;
    boot_timer_info->short_timer_last_start_time = jiffies;
    add_timer(&boot_timer_info->boot_timer);
    BFMR_PRINT_KEY_INFO("boot timer start!\n");

    return 0;

err:
    bfmr_free(boot_timer_info);

    return ret;
}
EXPORT_SYMBOL(bfm_init_boot_timer);

