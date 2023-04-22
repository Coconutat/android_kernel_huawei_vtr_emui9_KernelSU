

/*****************************************************************************
  1 Header File Including
*****************************************************************************/
#include <linux/module.h>   /* kernel module definitions */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ktime.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/kobject.h>
#include <linux/irq.h>
#include <linux/mutex.h>
#include <linux/kernel.h>

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
#include <linux/tty.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/version.h>
#include <linux/pm_wakeup.h>

#include "board.h"
#include "hw_bfg_ps.h"
#include "plat_type.h"
#include "plat_debug.h"
#include "plat_sdio.h"
#include "plat_uart.h"
#include "plat_firmware.h"
#include "plat_pm.h"
#include "bfgx_low_power.h"
#include "plat_exception_rst.h"
#include "plat_pm.h"

#ifdef BFGX_UART_DOWNLOAD_SUPPORT
#include "wireless_patch.h"
#endif

#include "oal_sdio.h"
#include "oal_sdio_comm.h"
#include "oal_sdio_host_if.h"
#include "oal_hcc_host_if.h"
#include "oal_schedule.h"
#include "plat_firmware.h"
#include "bfgx_exception_rst.h"
#ifdef BFGX_UART_DOWNLOAD_SUPPORT
extern int32 bfg_patch_recv(const uint8 *data, int32 count);
#endif
extern int32 hw_ir_only_open_other_subsys(void);

/*****************************************************************************
  2 Global Variable Definition
*****************************************************************************/
/*
  * Function: suspend_notify
  * Description: suspend notify call back
  * Ruturn: 0 -- success
 **/
static int pf_suspend_notify(struct notifier_block *notify_block,
                unsigned long mode, void *unused)
{

    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return IRQ_NONE;
    }

    switch (mode) {
        case PM_POST_SUSPEND:
            PS_PRINT_INFO("host resume OK!\n");
            break;
        case PM_SUSPEND_PREPARE:
            PS_PRINT_INFO("host suspend now!\n");
            break;
        default:
            break;
    }
    return 0;
}

static struct notifier_block pf_suspend_notifier = {
        .notifier_call = pf_suspend_notify,
        .priority = INT_MIN,
    };

struct pm_drv_data * pm_drv_data_t_etc = NULL;

struct pm_drv_data * pm_get_drvdata_etc(void)
{
    return pm_drv_data_t_etc;
}

static void pm_set_drvdata(struct pm_drv_data * data)
{
    pm_drv_data_t_etc = data;
}


/*****************************************************************************
  3 Function Definition
*****************************************************************************/


int32 check_bfg_state_etc(void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    return pm_data->bfgx_dev_state;
}

STATIC void host_allow_devslp_in_node(struct ps_core_s *ps_core_d)
{
    /* make "host_allow_bfg_sleep_etc()" happy */
    struct pm_drv_data  *pm_data = pm_get_drvdata_etc();
    if (unlikely(NULL == pm_data))
    {
        PS_PRINT_ERR("pm_data is null\n");
        return;
    }

    atomic_dec(&ps_core_d->node_visit_flag);
    //host_allow_bfg_sleep_etc(ps_core_d);
    if(true!=queue_work(pm_data->wkup_dev_workqueue, &pm_data->send_allow_sleep_work))
    {
        PS_PRINT_INFO("queue_work send_allow_sleep_work not return true\n");
    }
    /* recovery the original value */
    atomic_inc(&ps_core_d->node_visit_flag);
}


void bfgx_state_set_etc(uint8 on)
{
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return;
    }
    PS_PRINT_WARNING("bfgx_state_set_etc:%d --> %d\n",pm_data->bfgx_dev_state,on);
    pm_data->bfgx_dev_state = on;
}

int32 bfgx_state_get_etc(void)
{
    return check_bfg_state_etc();
}

STATIC void bfgx_uart_state_set(uint8 uart_state)
{
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return;
    }

    PS_PRINT_WARNING("bfgx_uart_state_set:%d-->%d",pm_data->uart_state,uart_state);
    pm_data->uart_state = uart_state;
}

int8 bfgx_uart_state_get(void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -1;
    }

    return pm_data->uart_state;
}

int32 bfgx_uart_get_baud_rate(void)
{
    struct ps_plat_s *ps_plat_d = NULL;

    ps_get_plat_reference_etc(&ps_plat_d);
    if (unlikely(NULL == ps_plat_d))
    {
        PS_PRINT_ERR("ps_plat_d is NULL\n");
        return -EINVAL;
    }

    return ps_plat_d->baud_rate;

}

void bfgx_uart_baud_change_work(struct work_struct *work)
{
    int    ret = 0;
    uint32 wait_cnt = 0;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    struct ps_core_s   *ps_core_d = NULL;
    uint64 flags;

    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (!ps_core_d)
    {
        PS_PRINT_ERR("ps_core has not inited\n");
        return ;
    }

    spin_lock_irqsave(&pm_data->uart_state_spinlock,flags);
    bfgx_uart_state_set(UART_BPS_CHG_SEND_ACK);
    spin_unlock_irqrestore(&pm_data->uart_state_spinlock,flags);

    /* 切波特率时，此消息是host发送的最后一个UART数据 */
    ps_tx_urgent_cmd(ps_core_d, SYS_MSG, PL_BAUT_CHG_REQ_ACK);

    while(UART_BPS_CHG_IN_PROGRESS!=pm_data->uart_state)
    {
        oal_udelay(200);
        wait_cnt++;
        if(wait_cnt>=10000)
        {
            PS_PRINT_ERR("wait device start baud change timeout\n");
            spin_lock_irqsave(&pm_data->uart_state_spinlock,flags);
            bfgx_uart_state_set(UART_READY);
            spin_unlock_irqrestore(&pm_data->uart_state_spinlock,flags);

            return ;
        }

    }

    ret = ps_change_uart_baud_rate_etc(pm_data->uart_baud_switch_to, FLOW_CTRL_ENABLE);
    if (0 != ret)
    {
        PS_PRINT_ERR("It is bad!!!, change uart rate fail\n");
    }

    /* 等待device完成切换，pull down GPIO*/
    while(1==board_get_bwkup_gpio_val_etc())
    {
        oal_udelay(200);
        wait_cnt++;
        if(wait_cnt>=10000)
        {
            PS_PRINT_ERR("wait device bps change complete && pull down gpio fail\n");
            return ;
        }
    }

    spin_lock_irqsave(&pm_data->uart_state_spinlock,flags);
    bfgx_uart_state_set(UART_BPS_CHG_SEND_COMPLETE);
    spin_unlock_irqrestore(&pm_data->uart_state_spinlock,flags);

    /* 切波特率完成后的第一个握手消息 */
    ps_tx_urgent_cmd(ps_core_d, SYS_MSG, PL_BAUT_CHG_COMPLETE);

    mod_timer(&pm_data->baud_change_timer, jiffies + msecs_to_jiffies(100));

    return;

}


int32 bfgx_uart_rcv_baud_change_req(uint8 uc_msg_type)
{
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    int32  cur_rate;

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -EINVAL;
    }

    PS_PRINT_INFO("%s\n", __func__);

    cur_rate = bfgx_uart_get_baud_rate();
    if(((SYS_INF_BAUD_CHG_2M_REQ==uc_msg_type)&&(HIGH_FREQ_BAUD_RATE==cur_rate))
       ||((SYS_INF_BAUD_CHG_6M_REQ==uc_msg_type)&&(LOW_FREQ_BAUD_RATE==cur_rate)))
    {
        if(uc_msg_type == SYS_INF_BAUD_CHG_6M_REQ)
        {
            pm_data->uart_baud_switch_to = HIGH_FREQ_BAUD_RATE;
        }
        else
        {
            pm_data->uart_baud_switch_to = LOW_FREQ_BAUD_RATE;
        }
        queue_work(pm_data->wkup_dev_workqueue, &pm_data->baud_change_work);
    }
    else
    {
        PS_PRINT_ERR("It is bad!!!, req = 0x%x,HIGH_FREQ_BAUD_RATE=%d,current = 0x%x\n",HIGH_FREQ_BAUD_RATE,uc_msg_type,cur_rate);
    }

    return 0;

}

int32 bfgx_uart_rcv_baud_change_complete_ack(void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    struct ps_core_s   *ps_core_d = NULL;

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -EINVAL;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (!ps_core_d)
    {
        PS_PRINT_ERR("ps_core has not inited\n");
        return  -EINVAL;
    }

    PS_PRINT_INFO("%s\n", __func__);
    del_timer_sync(&pm_data->baud_change_timer);

    /*restart the tx work*/
    queue_work(ps_core_d->ps_tx_workqueue, &ps_core_d->tx_skb_work);

    return 0;

}

void bfgx_uart_baud_change_expire(uint64 data)
{
    uint64 flags;
    struct ps_core_s   *ps_core_d = NULL;
    struct pm_drv_data  *pm_data = (struct pm_drv_data*)data;

    if (unlikely(NULL == pm_data))
    {
        PS_PRINT_ERR("devack timer para is null\n");
        return;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (!ps_core_d)
    {
        PS_PRINT_ERR("ps_core has not inited\n");
        return ;
    }

    PS_PRINT_INFO("%s\n", __func__);
    spin_lock_irqsave(&pm_data->uart_state_spinlock,flags);
    if(UART_BPS_CHG_SEND_COMPLETE==pm_data->uart_state)
    {
        ps_tx_urgent_cmd(ps_core_d, SYS_MSG, PL_BAUT_CHG_COMPLETE);
        mod_timer(&pm_data->baud_change_timer, jiffies + msecs_to_jiffies(100));
    }
    spin_unlock_irqrestore(&pm_data->uart_state_spinlock,flags);

}

int32 bfgx_pm_feature_set_etc(void)
{
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (NULL == ps_core_d)
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -FAILURE;
    }

    if (BFGX_PM_DISABLE == pm_data->bfgx_pm_ctrl_enable)
    {
        PS_PRINT_INFO("bfgx platform pm ctrl disable\n");
        msleep(50);
        return SUCCESS;
    }

    if (BFGX_PM_ENABLE == pm_data->bfgx_lowpower_enable)
    {
        PS_PRINT_INFO("bfgx platform pm enable\n");
        ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_PL_ENABLE_PM);
    }
    else
    {
        PS_PRINT_INFO("bfgx platform pm disable\n");
        ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_PL_DISABLE_PM);
    }

    if (BFGX_PM_ENABLE == pm_data->bfgx_bt_lowpower_enable)
    {
        PS_PRINT_INFO("bfgx bt pm enable\n");
        ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_BT_ENABLE_PM);
    }
    else
    {
        PS_PRINT_INFO("bfgx bt pm disable\n");
        ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_BT_DISABLE_PM);
    }

    if (BFGX_PM_ENABLE == pm_data->bfgx_gnss_lowpower_enable)
    {
        PS_PRINT_INFO("bfgx gnss pm enable\n");
        ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_GNSS_ENABLE_PM);
    }
    else
    {
        PS_PRINT_INFO("bfgx gnss pm disable\n");
        ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_GNSS_DISABLE_PM);
    }

    if (BFGX_PM_ENABLE == pm_data->bfgx_nfc_lowpower_enable)
    {
        PS_PRINT_INFO("bfgx nfc pm enable\n");
        ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_NFC_ENABLE_PM);
    }
    else
    {
        PS_PRINT_INFO("bfgx nfc pm disable\n");
        ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_NFC_DISABLE_PM);
    }

    msleep(20);

    return SUCCESS;
}


void bfg_wake_lock_etc(void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    oal_wakelock_stru  *pst_bfg_wake_lock;
    oal_ulong ul_flags;

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return;
    }

#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) )
    pst_bfg_wake_lock = &pm_data->bfg_wake_lock_etc;

    oal_spin_lock_irq_save(&pst_bfg_wake_lock->lock, &ul_flags);
    if (0 == oal_wakelock_active(pst_bfg_wake_lock))
    {
        __pm_stay_awake(&pst_bfg_wake_lock->st_wakelock);
        pst_bfg_wake_lock->locked_addr = (oal_ulong)_RET_IP_;
        pst_bfg_wake_lock->lock_count++;
        if(OAL_UNLIKELY(pst_bfg_wake_lock->debug))
        {
            printk(KERN_DEBUG"wakelock[%s] lockcnt:%lu <==%pf\n", pst_bfg_wake_lock->st_wakelock.name, pst_bfg_wake_lock->lock_count, (oal_void*)_RET_IP_);
        }
        #ifdef CONFIG_HISI_IDLE_SLEEP
        hisi_idle_sleep_vote(ID_GPS,1);
        PS_PRINT_INFO("hisi_idle_sleep_vote 1!\n");
        #endif
        PS_PRINT_INFO("bfg_wakelock active[%d],cnt %lu\n",oal_wakelock_active(pst_bfg_wake_lock),pst_bfg_wake_lock->lock_count);
    }
    oal_spin_unlock_irq_restore(&pst_bfg_wake_lock->lock, &ul_flags);
#endif

    return;
}

void bfg_wake_unlock_etc(void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    oal_wakelock_stru  *pst_bfg_wake_lock;
    oal_ulong ul_flags;

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return;
    }

#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) )
    pst_bfg_wake_lock = &pm_data->bfg_wake_lock_etc;

    oal_spin_lock_irq_save(&pst_bfg_wake_lock->lock, &ul_flags);

    if (oal_wakelock_active(pst_bfg_wake_lock))
    {
        pst_bfg_wake_lock->lock_count--;
         __pm_relax(&pst_bfg_wake_lock->st_wakelock);
        pst_bfg_wake_lock->locked_addr = (oal_ulong)0x0;

        if(OAL_UNLIKELY(pst_bfg_wake_lock->debug))
        {
            printk(KERN_DEBUG"wakeunlock[%s] lockcnt:%lu <==%pf\n", pst_bfg_wake_lock->st_wakelock.name, pst_bfg_wake_lock->lock_count, (oal_void*)_RET_IP_);
        }
        #ifdef CONFIG_HISI_IDLE_SLEEP
        hisi_idle_sleep_vote(ID_GPS,0);
        PS_PRINT_INFO("hisi_idle_sleep_vote 0!\n");
        #endif
        PS_PRINT_INFO("bfg_wakelock active[%d], cnt %lu\n",oal_wakelock_active(pst_bfg_wake_lock),pst_bfg_wake_lock->lock_count);
    }
    else
    {
        PS_PRINT_INFO("bfg_wakelock not active,cnt %lu\n",pst_bfg_wake_lock->lock_count);
    }
    oal_spin_unlock_irq_restore(&pst_bfg_wake_lock->lock, &ul_flags);
#endif

    return;
}


void host_wkup_dev_work_etc(struct work_struct *work)
{
#define RETRY_TIMES (3)
    int i = 0;
    int j;
    uint8 zero_num = 0;
    int bwkup_gpio_val = 0;
    uint64 timeleft;
    int ret = 0;
    struct ps_core_s *ps_core_d = NULL;
    uint64 flags;

    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return ;
    }

    PS_PRINT_INFO("%s,dev:%d,uart:%d\n", __func__,bfgx_state_get_etc(),bfgx_uart_get_baud_rate());
    ps_get_core_reference_etc(&ps_core_d);
    if(is_tty_open(ps_core_d->pm_data))
    {
        PS_PRINT_ERR("%s,tty is closed skip!\n",__func__);
        return ;
    }
    /* if B send work item of wkup_dev before A's work item finished, then
     * B should not do actual wkup operation.
     */
    if (BFGX_ACTIVE == bfgx_state_get_etc())
    {
        if (waitqueue_active(&pm_data->host_wkup_dev_comp.wait))
        {
            PS_PRINT_INFO("it seems like dev ack with NoSleep\n");
            complete_all(&pm_data->host_wkup_dev_comp);
        }
        else /* 目前用了一把host_mutex大锁，这种case不应存在，但低功耗模块不应依赖外部 */
        {
            PS_PRINT_DBG("B do wkup_dev work item after A do it but not finished\n");
        }
        return;
    }

    /* prepare to wake up device */
    ps_uart_state_pre_etc(ps_core_d->tty);
    timeleft = wait_for_completion_timeout(&pm_data->dev_ack_comp, msecs_to_jiffies(WAIT_DEVACK_TIMEOUT_MSEC));
    if (!timeleft)
    {
        ps_uart_state_dump_etc(ps_core_d->tty);
        PS_PRINT_ERR("wait dev allow slp ack timeout\n");
        DECLARE_DFT_TRACE_KEY_INFO("wait dev allow slp ack timeout",OAL_DFT_TRACE_FAIL);
        return;
    }

    INIT_COMPLETION(pm_data->dev_ack_comp);

    /* device doesn't agree to slp */
    if (BFGX_ACTIVE == bfgx_state_get_etc())
    {
        complete_all(&pm_data->host_wkup_dev_comp);
        PS_PRINT_DBG("we know dev ack with NoSleep\n");
        return;
    }

    /* begin to wake up device via uart rxd */
    for (i = 0; i < RETRY_TIMES; i++)
    {
        ret = ps_change_uart_baud_rate_etc(WKUP_DEV_BAUD_RATE, FLOW_CTRL_DISABLE);
        if (0 != ret)
        {
            PS_PRINT_ERR("It is bad!!!, change uart rate fail, try again\n");
            DECLARE_DFT_TRACE_KEY_INFO("change uart rate fail",OAL_DFT_TRACE_FAIL);
            continue;
        }

        do
        {
            bwkup_gpio_val = board_get_bwkup_gpio_val_etc();
            PS_PRINT_INFO("bfg wakeup dev,try %d,cur gpio level:%d\n", i,bwkup_gpio_val);
            /*uart write long zero to wake up device*/
            ps_write_tty_etc(ps_core_d, &zero_num, sizeof(uint8));

            timeleft = wait_for_completion_timeout(&pm_data->dev_ack_comp, msecs_to_jiffies(WAIT_WKUP_DEVACK_TIMEOUT_MSEC));
            if (timeleft || (BFGX_ACTIVE == bfgx_state_get_etc()))
            {
                bwkup_gpio_val = board_get_bwkup_gpio_val_etc();
                PS_PRINT_INFO("bfg wkup OK, gpio level:%d\n", bwkup_gpio_val);
                j = RETRY_TIMES;
                while(0 != ps_change_uart_baud_rate_etc(pm_data->uart_baud_switch_to, FLOW_CTRL_ENABLE))
                {
                    PS_PRINT_WARNING("change default uart rate fail,left retry cnt:%d,do retry\n", j);
                    DECLARE_DFT_TRACE_KEY_INFO("change uart rate fail",OAL_DFT_TRACE_FAIL);
                   if (--j){
                       msleep(100);
                    }else{
                       PS_PRINT_ERR("change default uart rate fail,retried but not succ\n");
                       break;
                    }
                }
                break;
            }
            else
            {
                if (!OAL_IS_ERR_OR_NULL(ps_core_d->tty) && tty_chars_in_buffer(ps_core_d->tty))
                {
                    PS_PRINT_INFO("tty tx buf is not empty\n");
                }

                bwkup_gpio_val = board_get_bwkup_gpio_val_etc();
                PS_PRINT_INFO("bfg still NOT wkup, gpio level:%d\n", bwkup_gpio_val);

                if(0==bwkup_gpio_val)
                {
                    DECLARE_DFT_TRACE_KEY_INFO("bfg wakeup fail",OAL_DFT_TRACE_EXCEP);
                    if(HI1XX_ANDROID_BUILD_VARIANT_USER == hi11xx_get_android_build_variant())
                    {
                        PS_PRINT_INFO("user mode,ssi dump bypass\n");
                    }
                    else
                    {

                        if(false == wlan_is_shutdown_etc())
                        {
                            ssi_dump_device_regs(SSI_MODULE_MASK_ARM_REG);
                        }
                        else
                        {
                            ssi_dump_device_regs(SSI_MODULE_MASK_AON | SSI_MODULE_MASK_ARM_REG | SSI_MODULE_MASK_BCTRL);
                        }
                    }

                    i++;
                }
                else
                {
                    PS_PRINT_INFO("bfg wakeup ack lost, complete it\n" );
                    spin_lock_irqsave(&pm_data->wakelock_protect_spinlock, flags);
                    bfg_wake_lock_etc();
                    bfgx_state_set_etc(BFGX_ACTIVE);
                    complete(&pm_data->dev_ack_comp);
                    spin_unlock_irqrestore(&pm_data->wakelock_protect_spinlock, flags);

                    j = RETRY_TIMES;
                    while(0 != ps_change_uart_baud_rate_etc(pm_data->uart_baud_switch_to, FLOW_CTRL_ENABLE))
                    {
                        PS_PRINT_WARNING("change default uart rate fail,left retry cnt:%d,do retry\n", j);
                        DECLARE_DFT_TRACE_KEY_INFO("change uart rate fail",OAL_DFT_TRACE_FAIL);
                       if (--j){
                           msleep(100);
                        }else{
                           PS_PRINT_ERR("change default uart rate fail,retried but not succ\n");
                           break;
                        }
                    }
                    queue_work(pm_data->wkup_dev_workqueue, &pm_data->send_disallow_msg_work);
                    break;
                }
            }
        }while(i < RETRY_TIMES);
        break;
    }

    if (RETRY_TIMES == i)
    {
        ps_change_uart_baud_rate_etc(pm_data->uart_baud_switch_to, FLOW_CTRL_ENABLE);
        PS_PRINT_INFO("host wkup bfg fail\n");
    }
}

#ifdef CONFIG_INPUTHUB
/*麒麟内核函数，先用内核版本宏隔开*/
/* sensorbub模块的函数，睡眠唤醒时用来查询sensorhub的状态 */
extern int getSensorMcuMode(void);
extern int get_iomcu_power_state(void);
#endif

void host_send_disallow_msg_etc(struct work_struct *work)
{
#define MAX_TTYRESUME_LOOPCNT (300)
#define MAX_SENSORHUB_LOOPCNT (30)
#ifdef ASYNCB_SUSPENDED
    uint32 loop_tty_resume_cnt = 0;
#endif
#ifdef CONFIG_INPUTHUB
    uint32 loop_sensorhub_resume_cnt = 0;
#endif
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    uint64 flags;

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return;
    }

    PS_PRINT_INFO("%s\n", __func__);

    ps_get_core_reference_etc(&ps_core_d);
    if (!ps_core_d)
    {
        PS_PRINT_ERR("ps_core has not inited\n");
        return;
    }

    /* 防止host睡眠情况下被dev唤醒进入gpio中断后直接在这里下发消息，
     * 此时uart可能还没有ready,所以这里等待tty resume之后才下发消息 */
    if ((ps_core_d->tty) && (ps_core_d->tty->port))
    {
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) )
        while(tty_port_suspended(ps_core_d->tty->port))
        {
            if(loop_tty_resume_cnt++ >= MAX_TTYRESUME_LOOPCNT)
            {
                PS_PRINT_ERR("tty is not ready, state:%d!\n", tty_port_suspended(ps_core_d->tty->port));
                break;
            }
            msleep(10);
        }
        PS_PRINT_INFO("tty state: 0x%x ,loop_tty_resume_cnt:%d\n", tty_port_suspended(ps_core_d->tty->port),  loop_tty_resume_cnt);
#else
        PS_PRINT_INFO("tty port flag 0x%x\n", (unsigned int)ps_core_d->tty->port->flags);
#ifdef ASYNCB_SUSPENDED
        while(test_bit(ASYNCB_SUSPENDED, (volatile unsigned long*)&(ps_core_d->tty->port->flags)))
        {
            if(loop_tty_resume_cnt++ >= MAX_TTYRESUME_LOOPCNT)
            {
                PS_PRINT_ERR("tty is not ready, flag is 0x%x!\n", (unsigned int)ps_core_d->tty->port->flags);
                break;
            }
            msleep(10);
        }
#endif
#endif

#ifdef CONFIG_INPUTHUB
        if (UART_PCLK_FROM_SENSORHUB == get_uart_pclk_source_etc())
        {
            /*查询sensorhub状态，如果不是wkup状态，uart的时钟可能会不对*/
            if (1 == getSensorMcuMode())
            {
                PS_PRINT_INFO("sensorbub state is %d\n", get_iomcu_power_state());
                /* 0,1->ST_POWERON,8->ST_SLEEP,9->ST_WAKEUP */
                while((9 != get_iomcu_power_state()) && (1 != get_iomcu_power_state()) && (0 != get_iomcu_power_state()))
                {
                    if (loop_sensorhub_resume_cnt++ >= MAX_SENSORHUB_LOOPCNT)
                    {
                        PS_PRINT_ERR("sensorhub not wakeup yet, state is %d\n", get_iomcu_power_state());
                        break;
                    }
                    msleep(10);
                }
            }
        }
#endif
    }
    else
    {
        PS_PRINT_ERR("tty has not inited\n");
        return;
    }

    /* clear pf msg parsing buffer to avoid problem caused by wrong packet */
    reset_uart_rx_buf_etc();

    /* 设置uart可用,下发disallow sleep消息,唤醒完成 */
    spin_lock_irqsave(&pm_data->uart_state_spinlock,flags);
    bfgx_uart_state_set(UART_READY);
    spin_unlock_irqrestore(&pm_data->uart_state_spinlock,flags);

    if (!g_b_ir_only_mode)
    {
        if(0!=ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_DISALLOW_SLP))
        {
           PS_PRINT_INFO("SYS_CFG_DISALLOW_SLP MSG send fail, retry\n");
           msleep(10);
           queue_work(pm_data->wkup_dev_workqueue, &pm_data->send_disallow_msg_work);
           return;
        }
    }

    /* 这里设置完成量对于dev wkup host没有意义, 只是保证和host wkup dev的操作一致
     * 注意这就要求host wkup dev前需要INIT完成量计数 */
    complete_all(&pm_data->host_wkup_dev_comp);

    if (!g_b_ir_only_mode)
    {
        /* if any of BFNI is open, we should mod timer. */
        if((!bfgx_other_subsys_all_shutdown_etc(BFGX_GNSS))||(GNSS_AGREE_SLEEP == atomic_read(&pm_data->gnss_sleep_flag)))
        {
            mod_timer(&pm_data->bfg_timer, jiffies + (PLATFORM_SLEEP_TIME * HZ/1000));
            pm_data->bfg_timer_mod_cnt++;
            PS_PRINT_INFO("mod_timer:host_send_disallow_msg_etc\n");
        }
        ps_core_d->ps_pm->operate_beat_timer(BEAT_TIMER_RESET);
    }

    if (NEED_SET_FLAG == atomic_read(&pm_data->bfg_needwait_devboot_flag))
    {
        complete(&pm_data->dev_bootok_ack_comp);
    }
}


void host_allow_bfg_sleep_etc(struct work_struct *work)
{
    uint64 flags;
    struct ps_core_s *ps_core_d = NULL;
    struct st_exception_info *pst_exception_data = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return;
    }

    PS_PRINT_INFO("%s\n", __func__);

    ps_get_core_reference_etc(&ps_core_d);
    if (!ps_core_d)
    {
        PS_PRINT_ERR("ps_core has not inited\n");
        return;
    }

    if (false == ps_core_d->tty_have_open)
    {
        PS_PRINT_INFO("tty has closed, not send msg to dev\n");
        return;
    }

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return ;
    }
    if (PLAT_EXCEPTION_RESET_IDLE != atomic_read(&pst_exception_data->is_reseting_device))
    {
        PS_PRINT_ERR("plat is doing dfr not allow sleep\n");
        mod_timer(&pm_data->bfg_timer, jiffies + (PLATFORM_SLEEP_TIME * HZ/1000));
        pm_data->bfg_timer_mod_cnt++;
        return;
    }

    spin_lock_irqsave(&pm_data->uart_state_spinlock,flags);

    /* if someone is visiting the dev_node */
    if (atomic_read(&ps_core_d->node_visit_flag) > 0)
    {
        PS_PRINT_INFO("someone visit node, not send allow sleep msg\n");
        spin_unlock_irqrestore(&pm_data->uart_state_spinlock,flags);
        /* gnss write do NOT mod timer */
        mod_timer(&pm_data->bfg_timer, jiffies + (PLATFORM_SLEEP_TIME * HZ/1000));
        pm_data->bfg_timer_mod_cnt++;
        return;
    }

    if((GNSS_AGREE_SLEEP != atomic_read(&pm_data->gnss_sleep_flag))||(false==ps_chk_tx_queue_empty(ps_core_d)))
    {
        PS_PRINT_INFO("tx queue not empty, not send allow sleep msg\n");
        spin_unlock_irqrestore(&pm_data->uart_state_spinlock,flags);
        mod_timer(&pm_data->bfg_timer, jiffies + (PLATFORM_SLEEP_TIME * HZ/1000));
        pm_data->bfg_timer_mod_cnt++;
        return;
    }
    /* 设置device状态为睡眠态，在host唤醒dev完成之前(或dev唤醒host前)uart不可用 */
    PS_PRINT_INFO("%s,set UART_NOT_READY,BFGX_SLEEP\n", __func__);
    ps_core_d->ps_pm->bfgx_uart_state_set(UART_NOT_READY);
    ps_core_d->ps_pm->bfgx_dev_state_set(BFGX_SLEEP);
    /*clear mod cnt*/
    pm_data->bfg_timer_mod_cnt = 0;
    pm_data->bfg_timer_mod_cnt_pre = 0;

    spin_unlock_irqrestore(&pm_data->uart_state_spinlock,flags);

    mod_timer(&pm_data->dev_ack_timer, jiffies + (WAIT_DEVACK_MSEC * HZ / 1000));

    /* we need reinit completion cnt as 0, to prevent such case:
     * 1)host allow dev sleep, dev ack with OK, cnt=1,
     * 2)device wkup host,
     * 3)host allow dev sleep,
     * 4)host wkup dev, it will wait dev_ack succ immediately since cnt==1,
     * 5)dev ack with ok, cnt=2,
     * this case will cause host wait dev_ack invalid.
     */
    INIT_COMPLETION(pm_data->dev_ack_comp);

    if(0 !=  ps_tx_urgent_cmd(ps_core_d, SYS_MSG, SYS_CFG_ALLOWDEV_SLP))
    {
        PS_PRINT_INFO("SYS_CFG_ALLOWDEV_SLP MSG send fail\n");
    }

}

/**
 * Prototype    : bfg_check_timer_work
 * Description  : check bfg timer is work fine
 * input        : ps_core_d
 * output       : no
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2013/05/09
 *     Author       : wx145522
 *     Modification : Created function
 *
 */
void bfg_check_timer_work(void)
{

    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return;
    }

    /*10s后没有人启动bfg timer
      补救:直接提交allow to sleep work*/
    if((pm_data->bfg_timer_mod_cnt_pre==pm_data->bfg_timer_mod_cnt)&&(0!=pm_data->bfg_timer_mod_cnt)
       &&(GNSS_AGREE_SLEEP == atomic_read(&pm_data->gnss_sleep_flag)))
    {
        if(time_after(jiffies, pm_data->bfg_timer_check_time))
        {
            DECLARE_DFT_TRACE_KEY_INFO("bfg_timer not work in 10s",OAL_DFT_TRACE_FAIL);
            if(true!=queue_work(pm_data->wkup_dev_workqueue, &pm_data->send_allow_sleep_work))
            {
                PS_PRINT_INFO("queue_work send_allow_sleep_work not return true\n");
            }
            else
            {
                PS_PRINT_INFO("timer_state(%d),queue_work send_allow_sleep_work succ\n",timer_pending(&pm_data->bfg_timer));
            }
        }
    }
    else
    {
        pm_data->bfg_timer_mod_cnt_pre = pm_data->bfg_timer_mod_cnt;
        pm_data->bfg_timer_check_time = jiffies + msecs_to_jiffies(PL_CHECK_TIMER_WORK);
    }
}


/**
 * Prototype    : bfg_timer_expire_etc
 * Description  : bfg timer expired function
 * input        : uint64
 * output       : no
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2013/05/09
 *     Author       : wx145522
 *     Modification : Created function
 *
 */
void bfg_timer_expire_etc(uint64 data)
{
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data  *pm_data = (struct pm_drv_data*)data;
    if (unlikely(NULL == pm_data))
    {
        PS_PRINT_ERR("pm_data is null\n");
        return;
    }

    ps_core_d = pm_data->ps_pm_interface->ps_core_data;
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return ;
    }

    //PS_PRINT_INFO("%s\n", __func__);

    if (BFGX_PM_DISABLE == pm_data->bfgx_lowpower_enable)
    {
        PS_PRINT_DBG("lowpower function disabled\n");
        return;
    }
    if (BFGX_SLEEP == pm_data->ps_pm_interface->bfgx_dev_state_get())
    {
        PS_PRINT_DBG("dev has been sleep\n");
        return;
    }

    if (GNSS_AGREE_SLEEP == atomic_read(&pm_data->gnss_sleep_flag)&&
        (UART_BPS_CHG_SEND_ACK > pm_data->uart_state))
    {
        if(true!=queue_work(pm_data->wkup_dev_workqueue, &pm_data->send_allow_sleep_work))
        {
            PS_PRINT_INFO("queue_work send_allow_sleep_work not return true\n");
        }
        pm_data->gnss_votesleep_check_cnt = 0;
        pm_data->rx_pkt_gnss_pre = 0;
    }
    else if(UART_BPS_CHG_SEND_ACK > pm_data->uart_state)
    {
        /*GNSS NOT AGREE SLEEP ,Check it*/
        if(pm_data->rx_pkt_gnss_pre != ps_core_d->rx_pkt_num[BFGX_GNSS])
        {
            pm_data->rx_pkt_gnss_pre = ps_core_d->rx_pkt_num[BFGX_GNSS];
            pm_data->gnss_votesleep_check_cnt = 0;

            mod_timer(&pm_data->bfg_timer, jiffies + (PLATFORM_SLEEP_TIME * HZ/1000));
            pm_data->bfg_timer_mod_cnt++;
        }
        else
        {
            pm_data->gnss_votesleep_check_cnt++;
            if(pm_data->gnss_votesleep_check_cnt >= PL_CHECK_GNSS_VOTE_CNT)
            {
                PS_PRINT_ERR("gnss_votesleep_check_cnt %d,set GNSS_AGREE_SLEEP\n",pm_data->gnss_votesleep_check_cnt);
                atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);
                queue_work(pm_data->wkup_dev_workqueue, &pm_data->send_allow_sleep_work);

                pm_data->gnss_votesleep_check_cnt = 0;
                pm_data->rx_pkt_gnss_pre = 0;
            }
            else
            {
                mod_timer(&pm_data->bfg_timer, jiffies + (PLATFORM_SLEEP_TIME * HZ/1000));
                pm_data->bfg_timer_mod_cnt++;
            }
        }
    }
    else
    {
        PS_PRINT_INFO("uart_state %d\n",pm_data->uart_state);
        mod_timer(&pm_data->bfg_timer, jiffies + (PLATFORM_SLEEP_TIME * HZ/1000));
        pm_data->bfg_timer_mod_cnt++;
    }

}


int32 host_wkup_dev_etc(void)
{
    uint64 timeleft;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    struct ps_core_s *ps_core_d = NULL;
    if (unlikely(NULL == pm_data))
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }
    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d ot tty is NULL\n");
        return -EINVAL;
    }
    if (BFGX_PM_DISABLE == pm_data->bfgx_lowpower_enable)
    {
        return 0;
    }
    PS_PRINT_DBG("wkup start\n");

    INIT_COMPLETION(pm_data->host_wkup_dev_comp);
    queue_work(pm_data->wkup_dev_workqueue, &pm_data->wkup_dev_work);
    ps_uart_state_pre_etc(ps_core_d->tty);
    timeleft = wait_for_completion_timeout(&pm_data->host_wkup_dev_comp, msecs_to_jiffies(WAIT_WKUPDEV_MSEC));
    if (!timeleft)
    {
        ps_uart_state_dump_etc(ps_core_d->tty);
        PS_PRINT_ERR("wait wake up dev timeout\n");
        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_WAKEUP_DEV);

        return -ETIMEDOUT;
    }
    PS_PRINT_DBG("wkup over\n");

    return 0;
}


int32 bfgx_other_subsys_all_shutdown_etc(uint8 subsys)
{
    int32  i = 0;
    struct ps_core_s *ps_core_d = NULL;

    ps_get_core_reference_etc(&ps_core_d);
    if (NULL == ps_core_d)
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    for (i = 0; i < BFGX_BUTT; i++)
    {
        if (i == subsys)
        {
            continue;
        }

        if (POWER_STATE_OPEN == atomic_read(&ps_core_d->bfgx_info[i].subsys_state))
        {
            return false;
        }
    }

    return true;
}


void bfgx_print_subsys_state(void)
{
    int32  i = 0;
    int32  len, total;
    char print_str[200] = {0};
    struct ps_core_s *ps_core_d = NULL;

    ps_get_core_reference_etc(&ps_core_d);
    if (NULL == ps_core_d)
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return ;
    }

    len = sizeof(print_str);
    total = 0;

    for (i = 0; i < BFGX_BUTT; i++)
    {
        if (POWER_STATE_OPEN == atomic_read(&ps_core_d->bfgx_info[i].subsys_state))
        {
            total += snprintf(print_str + total, len - total, "%s:%s ", ps_core_d->bfgx_info[i].name, "on ");
        }
        else
        {
            total += snprintf(print_str + total, len - total, "%s:%s ", ps_core_d->bfgx_info[i].name, "off");
        }
    }

    PS_PRINT_ERR("%s\n", print_str);
}

void bfgx_gpio_intr_enable(uint32  ul_en)
{
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    uint64 flags;
    spin_lock_irqsave(&pm_data->bfg_irq_spinlock, flags);
    if (ul_en)
    {
        /*不再支持中断开关嵌套*/
        if(pm_data->ul_irq_stat)
        {
            enable_irq(pm_data->bfg_irq);
            pm_data->ul_irq_stat = 0;
        }
    }
    else
    {
        if(!pm_data->ul_irq_stat)
        {
            disable_irq_nosync(pm_data->bfg_irq);
            pm_data->ul_irq_stat = 1;
        }
    }
    spin_unlock_irqrestore(&pm_data->bfg_irq_spinlock, flags);
}

int32 bfgx_dev_power_on_etc(void)
{
    uint64 timeleft;
    int32 error = BFGX_POWER_FAILED;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    BOARD_INFO * bd_info = NULL;

    bd_info = get_hi110x_board_info_etc();
    if (unlikely(NULL == bd_info))
    {
        PS_PRINT_ERR("board info is err\n");
        return BFGX_POWER_FAILED;
    }
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return BFGX_POWER_FAILED;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return BFGX_POWER_FAILED;
    }
    /*防止Host睡眠*/
    oal_wake_lock(&pm_data->bfg_wake_lock_etc);

    bfgx_gpio_intr_enable(OAL_TRUE);
    INIT_COMPLETION(pm_data->dev_bootok_ack_comp);
    atomic_set(&pm_data->bfg_needwait_devboot_flag, NEED_SET_FLAG);
    error = bd_info->bd_ops.bfgx_dev_power_on_etc();
    if (BFGX_POWER_SUCCESS != error)
    {
        goto bfgx_power_on_fail;
    }

    ps_uart_state_pre_etc(ps_core_d->tty);
    /*WAIT_BFGX_BOOTOK_TIME:这个时间目前为1s，有1s不够的情况，需要关注*/
    timeleft = wait_for_completion_timeout(&pm_data->dev_bootok_ack_comp, msecs_to_jiffies(WAIT_BFGX_BOOTOK_TIME));
    if (!timeleft)
    {
        ps_uart_state_dump_etc(ps_core_d->tty);
        if (wlan_is_shutdown_etc())
        {
            PS_PRINT_ERR("wifi off, wait bfgx boot up ok timeout\n");
            error = BFGX_POWER_WIFI_OFF_BOOT_UP_FAIL;
            CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_BCPU_BOOTUP);

            goto bfgx_power_on_fail;
        }
        else
        {
            PS_PRINT_ERR("wifi on, wait bfgx boot up ok timeout\n");
            error = BFGX_POWER_WIFI_ON_BOOT_UP_FAIL;
            CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_BCPU_BOOTUP);

            goto bfgx_power_on_fail;
        }
    }

    atomic_set(&pm_data->bfg_needwait_devboot_flag, NONEED_SET_FLAG);

    if (!g_b_ir_only_mode)
    {
        if (BOARD_VERSION_HI1103 == get_hi110x_subchip_type())
        {
            if (wlan_is_shutdown_etc())
            {
                ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_NOTIFY_WIFI_CLOSE);
            }
            else
            {
                ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_NOTIFY_WIFI_OPEN);
            }
        }

        bfgx_pm_feature_set_etc();
    }

    return BFGX_POWER_SUCCESS;

bfgx_power_on_fail:
    oal_wake_unlock(&pm_data->bfg_wake_lock_etc);
    return error;
}


int32 bfgx_dev_power_off_etc(void)
{
    int32  error = SUCCESS;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    BOARD_INFO * bd_info = NULL;
    bd_info = get_hi110x_board_info_etc();
    if (unlikely(NULL == bd_info))
    {
        PS_PRINT_ERR("board info is err\n");
        return -FAILURE;
    }

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -FAILURE;
    }

    /*单红外没有心跳*/
    if (!g_b_ir_only_mode)
    {
        pm_data->ps_pm_interface->operate_beat_timer(BEAT_TIMER_DELETE);
        del_timer_sync(&pm_data->bfg_timer);
        pm_data->bfg_timer_mod_cnt = 0;
        pm_data->bfg_timer_mod_cnt_pre = 0;
    }

    /* 下电即将完成，需要在此时设置下次上电要等待device上电成功的flag */
    atomic_set(&pm_data->bfg_needwait_devboot_flag, NEED_SET_FLAG);
    bd_info->bd_ops.bfgx_dev_power_off_etc();

    ps_core_d->rx_pkt_sys = 0;
    ps_core_d->rx_pkt_oml = 0;

    bfg_wake_unlock_etc();

    bfgx_gpio_intr_enable(OAL_FALSE);

    return error;
}


int32 bfgx_dev_power_control_etc(uint8 subsys, uint8 flag)
{
    int32 ret = 0;

 #ifdef BFGX_UART_DOWNLOAD_SUPPORT
    if((BFGX_GNSS != subsys))
    {
        PS_PRINT_ERR("gnss only only not support subs:%d\n",subsys);
        return -FAILURE;
    }
 #endif

    if (BFG_POWER_GPIO_UP == flag)
    {
        ret = bfgx_power_on_etc(subsys);
        if (ret)
        {
            PS_PRINT_ERR("bfgx power on is error!\n");
        }
    }
    else if (BFG_POWER_GPIO_DOWN == flag)
    {
        ret = bfgx_power_off_etc(subsys);
        if (ret)
        {
            PS_PRINT_ERR("bfgx power off is error!\n");
        }
    }
    else
    {
        PS_PRINT_ERR("invalid input data!\n");
        ret = -FAILURE;
    }

    return ret;
}


int firmware_download_function_etc(uint32 which_cfg)
{
    int32 ret = 0;
    unsigned long long total_time = 0;
    ktime_t start_time, end_time, trans_time;
    static unsigned long long max_time = 0;
    static unsigned long long count = 0;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    PS_PRINT_INFO("enter firmware_download_function_etc\n");

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    if (which_cfg >= CFG_FILE_TOTAL)
    {
        PS_PRINT_ERR("cfg file index [%d] outof range\n", which_cfg);
        return -FAILURE;
    }

    if(NULL  == pm_data->pst_wlan_pm_info->pst_bus)
    {
        PS_PRINT_ERR("pst_bus is null\n");
        return -FAILURE;
    }

    start_time = ktime_get();

    hcc_bus_wake_lock(pm_data->pst_wlan_pm_info->pst_bus);
    hcc_bus_lock(pm_data->pst_wlan_pm_info->pst_bus);

    ret = hcc_bus_reinit(pm_data->pst_wlan_pm_info->pst_bus);
    if (OAL_SUCC != ret)
    {
        hcc_bus_unlock(pm_data->pst_wlan_pm_info->pst_bus);
        hcc_bus_wake_unlock(pm_data->pst_wlan_pm_info->pst_bus);
        PS_PRINT_ERR("sdio reinit failed, ret:%d!\n", ret);
        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_SDIO_FAIL_FIRMWARE_DOWN);
        return -FAILURE;
    }

    wlan_pm_init_dev_etc();

    /*firmware_cfg_init_etc(sdio) function should just be called once*/
    if (!test_bit(FIRMWARE_CFG_INIT_OK, &pm_data->firmware_cfg_init_flag))
    {
        PS_PRINT_INFO("firmware_cfg_init_etc begin\n");
        ret = firmware_cfg_init_etc();
        if (ret)
        {
            PS_PRINT_ERR("firmware_cfg_init_etc failed, ret:%d!\n", ret);
            hcc_bus_unlock(pm_data->pst_wlan_pm_info->pst_bus);
            hcc_bus_wake_unlock(pm_data->pst_wlan_pm_info->pst_bus);

            CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_CFG_FAIL_FIRMWARE_DOWN);
            return ret;
        }

        PS_PRINT_INFO("firmware_cfg_init_etc OK\n");
        set_bit(FIRMWARE_CFG_INIT_OK, &pm_data->firmware_cfg_init_flag);
    }

    PS_PRINT_INFO("firmware_download_etc begin\n");

    ret = firmware_download_etc(which_cfg);

    if (ret < 0)
    {
        hcc_bus_unlock(pm_data->pst_wlan_pm_info->pst_bus);
        hcc_bus_wake_unlock(pm_data->pst_wlan_pm_info->pst_bus);
        if(-OAL_EINTR != ret)
        {
            PS_PRINT_ERR("firmware download fail!\n");
            DECLARE_DFT_TRACE_KEY_INFO("patch_download_fail",OAL_DFT_TRACE_FAIL);
            if (BFGX_CFG == which_cfg)
            {
                CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_FIRMWARE_DOWN);
             }
            else
            {
                CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_FIRMWARE_DOWN);
            }

            if(HI1XX_ANDROID_BUILD_VARIANT_USER != hi11xx_get_android_build_variant())
            {
                unsigned long long reg = SSI_MODULE_MASK_COMM;
                if(HCC_BUS_SDIO == pm_data->pst_wlan_pm_info->pst_bus->bus_type)
                {
                    reg |= SSI_MODULE_MASK_SDIO;
                }

                ssi_dump_device_regs(reg);

                /*dump bootloader rw dtcm*/
                ssi_read_reg_info_test(0x20019c00, 2592, 1, 2);
            }
        }
        else
        {
            /*download firmware interrupt*/
            PS_PRINT_INFO("firmware download interrupt!\n");
            DECLARE_DFT_TRACE_KEY_INFO("patch_download_interrupt",OAL_DFT_TRACE_FAIL);
        }
        return ret;
    }
    DECLARE_DFT_TRACE_KEY_INFO("patch_download_ok",OAL_DFT_TRACE_SUCC);

    hcc_bus_unlock(pm_data->pst_wlan_pm_info->pst_bus);
    hcc_bus_wake_unlock(pm_data->pst_wlan_pm_info->pst_bus);

    PS_PRINT_INFO("firmware_download_etc success\n");

    end_time = ktime_get();

    trans_time = ktime_sub(end_time, start_time);
    total_time = (unsigned long long)ktime_to_us(trans_time);

    if (total_time > max_time)
    {
        max_time = total_time;
    }

    count++;
    PS_PRINT_WARNING("download firmware, count [%llu], current time [%llu]us, max time [%llu]us\n", count, total_time, max_time);

    return SUCCESS;
}

bool  wlan_is_shutdown_etc(void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return true;
    }

    return ((POWER_STATE_SHUTDOWN == pm_data->pst_wlan_pm_info->ul_wlan_power_state) ? true : false);
}

bool bfgx_is_shutdown_etc(void)
{
    struct ps_core_s *ps_core_d = NULL;

    ps_get_core_reference_etc(&ps_core_d);
    if (NULL == ps_core_d)
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return true;
    }

    return ps_chk_bfg_active_etc(ps_core_d) ? false : true;
}
EXPORT_SYMBOL(bfgx_is_shutdown_etc);

int32 wifi_power_fail_process_etc(int32 error)
{
    int32 ret = WIFI_POWER_FAIL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return ret;
    }

    if (error >= WIFI_POWER_ENUM_BUTT)
    {
         PS_PRINT_ERR("error is undefined, error=[%d]\n", error);
         return ret;
    }

    PS_PRINT_INFO("wifi power fail, error=[%d]\n", error);

    switch (error)
    {
    case WIFI_POWER_SUCCESS:
    case WIFI_POWER_PULL_POWER_GPIO_FAIL:
        break;

    /*BFGX off，wifi firmware download fail和wait boot up fail，直接返回失败，上层重试，不走DFR*/
    case WIFI_POWER_BFGX_OFF_BOOT_UP_FAIL:
        if(OAL_TRUE == oal_trigger_bus_exception(pm_data->pst_wlan_pm_info->pst_bus, OAL_TRUE))
        {
            /*exception is processing, can't power off*/
            PS_PRINT_INFO("bfgx off,sdio exception is working\n");
            break;
        }
        PS_PRINT_INFO("bfgx off,set wlan_power_state to shutdown\n");
        oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pm_data->pst_wlan_pm_info->pst_bus), OAL_FALSE);
        pm_data->pst_wlan_pm_info->ul_wlan_power_state = POWER_STATE_SHUTDOWN;
    case WIFI_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL:
        PS_PRINT_INFO("wifi power fail: pull down power on gpio\n");
        board_power_off_etc(WLAN_POWER);
        break;

    /*BFGX on，wifi上电失败，进行全系统复位，wifi本次返回失败，上层重试*/
    case WIFI_POWER_BFGX_ON_BOOT_UP_FAIL:
        if(OAL_TRUE == oal_trigger_bus_exception(pm_data->pst_wlan_pm_info->pst_bus, OAL_TRUE))
        {
            /*exception is processing, can't power off*/
            PS_PRINT_INFO("bfgx on,sdio exception is working\n");
            break;
        }
        PS_PRINT_INFO("bfgx on,set wlan_power_state to shutdown\n");
        oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(pm_data->pst_wlan_pm_info->pst_bus), OAL_FALSE);
        pm_data->pst_wlan_pm_info->ul_wlan_power_state = POWER_STATE_SHUTDOWN;
    case WIFI_POWER_BFGX_DERESET_WCPU_FAIL:
    case WIFI_POWER_BFGX_ON_FIRMWARE_DOWNLOAD_FAIL:
        if (WIFI_POWER_SUCCESS == plat_power_fail_exception_info_set_etc(SUBSYS_WIFI, THREAD_WIFI, WIFI_POWERON_FAIL))
        {
            bfgx_system_reset_etc();
            plat_power_fail_process_done_etc();
        }
        else
        {
            PS_PRINT_ERR("wifi power fail, set exception info fail\n");
        }
        break;

    case WIFI_POWER_ON_FIRMWARE_DOWNLOAD_INTERRUPT:
        ret = -OAL_EINTR;
        break;

    default:
        PS_PRINT_ERR("error is undefined, error=[%d]\n", error);
        break;
    }

    return ret;
}

int32 wifi_notify_bfgx_status(uint8 ucStatus)
{
    struct ps_core_s *ps_core_d = NULL;
    int32  ret;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -EFAULT;;
    }

    if (!bfgx_is_shutdown_etc())
    {
        ret = prepare_to_visit_node_etc(ps_core_d);
        if (ret < 0)
        {
            PS_PRINT_ERR("prepare work fail, bring to reset work\n");
            return ret;
        }

        ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, ucStatus);

        post_to_visit_node_etc(ps_core_d);
    }

    return 0;
}

int32 wlan_power_on_etc(void)
{
    int32  error = WIFI_POWER_SUCCESS;
    unsigned long long total_time = 0;
    ktime_t start_time, end_time, trans_time;
    static unsigned long long max_download_time = 0;
    static unsigned long long num = 0;
    BOARD_INFO* bd_info = NULL;

    PS_PRINT_INFO("wlan power on!\n");

    bd_info = get_hi110x_board_info_etc();
    if (unlikely(NULL == bd_info))
    {
        PS_PRINT_ERR("board info is err\n");
        return -FAILURE;
    }

    /*wifi上电时如果单红外打开，则需要关闭单红外，下载全patch*/
    if (g_b_ir_only_mode)
    {
        if (hw_ir_only_open_other_subsys() != BFGX_POWER_SUCCESS)
        {
            PS_PRINT_ERR("ir only mode,but close ir only mode fail!\n");
            return -FAILURE;
        }
    }

    start_time = ktime_get();

    if(OAL_TRUE == hcc_bus_exception_is_busy(hcc_get_current_110x_bus()))
    {
        DECLARE_DFT_TRACE_KEY_INFO("open_fail_exception_is_busy", OAL_DFT_TRACE_FAIL);

        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_OPEN, CHR_WIFI_DRV_ERROR_POWER_ON_NON_BUS);
        return -FAILURE;
    }

#ifdef PLATFORM_DEBUG_ENABLE
    if (!is_dfr_test_en(WIFI_POWER_ON_FAULT))
    {
        error = WIFI_POWER_BFGX_DERESET_WCPU_FAIL;
        PS_PRINT_WARNING("dfr test WIFI_POWER_ON_FAULT enable\n");
        goto wifi_power_fail;
    }
#endif

    error = bd_info->bd_ops.wlan_power_on_etc();
    if (WIFI_POWER_SUCCESS != error)
    {
        goto wifi_power_fail;
    }

    if (BOARD_VERSION_HI1103 == get_hi110x_subchip_type())
    {
        wifi_notify_bfgx_status(SYS_CFG_NOTIFY_WIFI_OPEN);
    }

    //hcc_enable_etc(hcc_get_110x_handler(), OAL_TRUE);

    end_time = ktime_get();

    trans_time = ktime_sub(end_time, start_time);
    total_time = (unsigned long long)ktime_to_us(trans_time);

    if (total_time > max_download_time)
    {
        max_download_time = total_time;
    }

    num++;
    PS_PRINT_WARNING("power on, count [%llu], current time [%llu]us, max time [%llu]us\n", num, total_time, max_download_time);

    return WIFI_POWER_SUCCESS;

wifi_power_fail:

    CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_OPEN, CHR_WIFI_DRV_ERROR_POWER_ON);
    return wifi_power_fail_process_etc(error);
}

int32 wlan_power_off_etc(void)
{
    int32 error;
    BOARD_INFO* bd_info = NULL;

    PS_PRINT_INFO("wlan power off!\n");

    bd_info = get_hi110x_board_info_etc();
    if (unlikely(NULL == bd_info))
    {
        PS_PRINT_ERR("board info is err\n");
        return -FAILURE;
    }

    if (BOARD_VERSION_HI1103 == get_hi110x_subchip_type())
    {
        wifi_notify_bfgx_status(SYS_CFG_NOTIFY_WIFI_CLOSE);
    }

    error = bd_info->bd_ops.wlan_power_off_etc();
    if (SUCCESS != error)
    {
        return error;
    }

    //oal_wlan_gpio_intr_enable_etc(HBUS_TO_DEV(hcc_get_current_110x_bus()), OAL_FALSE);

    return SUCCESS;
}

int32 bfgx_power_on_etc(uint8 subsys)
{
    int32  ret = BFGX_POWER_SUCCESS;
    unsigned long long total_time = 0;
    ktime_t start_time, end_time, trans_time;
    static unsigned long long max_download_time = 0;
    static unsigned long long num = 0;

    start_time = ktime_get();

    if (bfgx_other_subsys_all_shutdown_etc(subsys))
    {
        ret = bfgx_dev_power_on_etc();
        if(BFGX_POWER_SUCCESS != ret)
        {
            return ret;
        }
    }

    end_time = ktime_get();

    trans_time = ktime_sub(end_time, start_time);
    total_time = (unsigned long long)ktime_to_us(trans_time);

    if (total_time > max_download_time)
    {
        max_download_time = total_time;
    }

    num++;
    PS_PRINT_WARNING("power on, count [%llu], current time [%llu]us, max time [%llu]us\n", num, total_time, max_download_time);

    return BFGX_POWER_SUCCESS;
}

int32 bfgx_power_off_etc(uint8 subsys)
{
    struct ps_core_s *ps_core_d = NULL;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is null\n");
        return -FAILURE;
    }

    if (ps_chk_only_gnss_and_cldslp_etc(ps_core_d))
    {
        PS_PRINT_SUC("%s power off request sleep!\n", g_bfgx_subsys_name_etc[subsys]);
        host_allow_devslp_in_node(ps_core_d);

        return SUCCESS;
    }

    PS_PRINT_INFO("%s power off!\n", g_bfgx_subsys_name_etc[subsys]);

    if (bfgx_other_subsys_all_shutdown_etc(subsys))
    {
        return bfgx_dev_power_off_etc();
    }

    return SUCCESS;
}
int32 pro_memcheck_en = 0;
struct completion pro_memcheck_finish;
int32 memcheck_is_working(void)
{
    if (pro_memcheck_en)
    {
        complete(&pro_memcheck_finish);
        PS_PRINT_ERR("is in product mem check test !\n");
        return 0;
    }
    return -1;
}
void memcheck_bfgx_init(void)
{
    bfgx_gpio_intr_enable(OAL_TRUE);
    pro_memcheck_en = 1;
    init_completion(&pro_memcheck_finish);
}
void memcheck_bfgx_exit(void)
{
    pro_memcheck_en = 0;
    bfgx_gpio_intr_enable(OAL_FALSE);
}
int32 memcheck_bfgx_is_succ(void)
{
    uint64 timeleft;
    PS_PRINT_FUNCTION_NAME;
    timeleft = wait_for_completion_timeout(&pro_memcheck_finish, msecs_to_jiffies(bfgx_mem_check_mdelay));
    if (!timeleft)
    {
        PS_PRINT_ERR("wait bfgx memcheck_bfgx_is_succ timeout\n");
        return  -1;
    }
    return 0;
}
int32 device_mem_check_etc(unsigned long long *time)
{
    int32 ret = -FAILURE;
    unsigned long long total_time = 0;
    ktime_t start_time, end_time, trans_time;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    if (NULL == time)
    {
        PS_PRINT_ERR("param time is  NULL!\n");
        return -FAILURE;
    }
    start_time = ktime_get();

    PS_PRINT_INFO("device ram reg test!\n");

    if (!bfgx_is_shutdown_etc())
    {
        PS_PRINT_SUC("factory ram reg test need bfgx shut down!\n");
        bfgx_print_subsys_state();
        return -FAILURE;
    }
    if (!wlan_is_shutdown_etc())
    {
        PS_PRINT_SUC("factory ram reg test need wlan shut down!\n");
        return -FAILURE;
    }

    ret = board_power_on_etc(WLAN_POWER);
    if(ret)
    {
        PS_PRINT_ERR("WLAN_POWER on failed ret=%d\n", ret);
        return -FAILURE;
    }

    PS_PRINT_INFO("===================start wcpu ram reg test!\n");
    ret = firmware_download_function_etc(RAM_REG_TEST_CFG);
    if(SUCCESS == ret)
    {
         /*等待device信息处理*/
         mdelay(wlan_mem_check_mdelay);
         ret = is_device_mem_test_succ();
         get_device_test_mem();
         if (!ret)
         {
             PS_PRINT_INFO("==device wcpu ram reg test success!\n");
         }
         else
         {
             PS_PRINT_INFO("==device wcpu ram reg test failed!\n");
             goto exit_error;
         }
    }
    PS_PRINT_INFO("===================start bcpu ram reg test!\n");
    board_power_off_etc(WLAN_POWER);

    board_power_on_etc(WLAN_POWER);
    g_auc_pilot_cfg_patch_in_vendor[RAM_REG_TEST_CFG] = RAM_BCPU_CHECK_CFG_HI1103_PILOT_PATH;
    ret = firmware_get_cfg_etc(g_auc_cfg_path_etc[RAM_REG_TEST_CFG], RAM_REG_TEST_CFG);
    if(ret)
    {
        PS_PRINT_INFO("ini analysis fail!\n");
        goto exit_error;
    }

    memcheck_bfgx_init();

    ret = firmware_download_function_etc(RAM_REG_TEST_CFG);

    g_auc_pilot_cfg_patch_in_vendor[RAM_REG_TEST_CFG]=RAM_CHECK_CFG_HI1103_PILOT_PATH;
    if(SUCCESS == ret)
    {
        /*等待device信息处理*/
        ret = memcheck_bfgx_is_succ();
        if (!ret)
        {
            PS_PRINT_INFO("==device bcpu ram reg test success!\n");
        }
        else
        {
            PS_PRINT_INFO("==device bcpu ram reg test failed!\n");
            firmware_get_cfg_etc(g_auc_cfg_path_etc[RAM_REG_TEST_CFG], RAM_REG_TEST_CFG);
            goto exit_error;
        }
    }
    firmware_get_cfg_etc(g_auc_cfg_path_etc[RAM_REG_TEST_CFG], RAM_REG_TEST_CFG);

    end_time = ktime_get();

    trans_time = ktime_sub(end_time, start_time);
    total_time = (unsigned long long)ktime_to_us(trans_time);

    *time = total_time;

    PS_PRINT_SUC("device mem reg test time [%llu]us\n", total_time);
exit_error:
    memcheck_bfgx_exit();
    board_power_off_etc(WLAN_POWER);
    return ret;
}

EXPORT_SYMBOL(device_mem_check_etc);
#ifdef BFGX_UART_DOWNLOAD_SUPPORT

int32 pm_uart_send(uint8 *data, int32 len)
{
    uint16  count   = 0;

    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -1;
    }

    /*call interface supplied by 3 in 1*/
    if (NULL == pm_data->ps_pm_interface->write_patch)
    {
        PS_PRINT_ERR("bfg_write_patch is NULL!\n");
        return -1;
    }

    while(1)
    {
        /*this function return the length has been sent*/
        count = pm_data->ps_pm_interface->write_patch(data, len);
        /*data has been sent over and return*/
        if(count == len)
        {
            return len;
        }

        /*data has not been sent over, we will send again*/
        data = data + count;
        len  = len  - count;
        msleep(1);
    }
}


int bfg_patch_download_function(void)
{
    int32 ret       = 0;
    int32 counter   = 0;
    ktime_t start_time, end_time, trans_time;
    static unsigned long long max_time = 0;
    static unsigned long long count = 0;
    unsigned long long total_time = 0;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data =  pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -1;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -1;
    }

    PS_PRINT_DBG("enter\n");
    start_time = ktime_get();

    /*patch_init(uart) function should just be called once*/
//    if (!test_bit(UART_INIT_OK, &pm_data->patch_init_flag))
//    {
        ret = patch_init(ENUM_INFO_UART);
        if (ret)
        {
            PS_PRINT_ERR("patch modem init failed, ret:%d!\n", ret);
            return ret;
        }

//        set_bit(UART_INIT_OK, &pm_data->patch_init_flag);
//    }
    /*bfg patch download, three times for fail*/
    for (counter = 0; counter < 3; counter++)
    {
        ps_uart_state_pre_etc(ps_core_d->tty);
        ret = patch_download_patch(ENUM_INFO_UART);
        if (ret)
        {
            PS_PRINT_ERR("bfg patch download fail, and reset power!\n");
            ps_uart_state_dump_etc(ps_core_d->tty);
#if 0

            err = pm_uart_set_baudrate(DEFAULT_BAUD_RATE);
            if (err)
            {
                PS_PRINT_ERR(" modify baudrate fail!!\n");
                return err;
            }


            gpio_direction_output(g_wire_dev->bfg_power_on, LOWLEVEL);

            /*if wlan is off, we must pull up power on gpio*/
            if (check_dev_is_shutdown(WLAN_STATE_FLAG))
            {
                PS_PRINT_SUC("bfg reset poweron!\n");
                power_reset();
            }
            mdelay(10);
            gpio_direction_output(g_wire_dev->bfg_power_on, HIGHLEVEL);
            mdelay(1);
            /* pull up gpio to disagree bfg public section sleep*/
            gpio_direction_output(g_wire_dev->host_wakeup_bfg, HIGHLEVEL);
#endif
        }
        else
        {
            end_time = ktime_get();
            trans_time = ktime_sub(end_time, start_time);
            total_time = (unsigned long long)ktime_to_us(trans_time);
            if (total_time > max_time)
            {
                max_time = total_time;
            }

            PS_PRINT_WARNING("download bfg patch succ,count [%llu], current time [%llu]us, max time [%llu]us\n", count, total_time, max_time);
            /*download patch successfully*/
            return ret;
        }
    }

    /*going to exception*/
    PS_PRINT_ERR("bfg patch download has failed finally!\n");
    return ret;

}
#endif

int32 ps_pm_register_etc(struct ps_pm_s *new_pm)
{
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL\n");
        return -FAILURE;
    }

    if (NULL == new_pm)
    {
        PS_PRINT_ERR("new_pm is null!\n");
        return -FAILURE;
    }

    pm_data->ps_pm_interface                        = new_pm;
    pm_data->ps_pm_interface->pm_priv_data          = pm_data;
    pm_data->ps_pm_interface->bfg_wake_lock_etc         = bfg_wake_lock_etc;
    pm_data->ps_pm_interface->bfg_wake_unlock_etc       = bfg_wake_unlock_etc;
    pm_data->ps_pm_interface->bfgx_dev_state_get    = bfgx_state_get_etc;
    pm_data->ps_pm_interface->bfgx_dev_state_set    = bfgx_state_set_etc;
    pm_data->ps_pm_interface->bfg_power_set         = bfgx_dev_power_control_etc;
    pm_data->ps_pm_interface->bfgx_uart_state_get   = bfgx_uart_state_get;
    pm_data->ps_pm_interface->bfgx_uart_state_set   = bfgx_uart_state_set;
#ifdef BFGX_UART_DOWNLOAD_SUPPORT
    pm_data->ps_pm_interface->download_patch        = bfg_patch_download_function;
    pm_data->ps_pm_interface->recv_patch            = bfg_patch_recv;
    pm_data->ps_pm_interface->write_patch           = ps_patch_write;
#endif
    PS_PRINT_SUC("pm registered over!");

    return SUCCESS;
}
EXPORT_SYMBOL_GPL(ps_pm_register_etc);


int32 ps_pm_unregister_etc(struct ps_pm_s *new_pm)
{
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL\n");
        return -FAILURE;
    }

    PS_PRINT_DBG("enter\n");

    if (NULL == new_pm)
    {
        PS_PRINT_ERR("new_pm is null!\n");
        return -FAILURE;
    }

    new_pm->bfg_wake_lock_etc           = NULL;
    new_pm->bfg_wake_unlock_etc         = NULL;
    new_pm->bfgx_dev_state_get      = NULL;
    new_pm->bfgx_dev_state_set      = NULL;
    new_pm->bfg_power_set           = NULL;
    new_pm->bfgx_uart_state_set     = NULL;
    new_pm->bfgx_uart_state_get     = NULL;
    pm_data->ps_pm_interface        = NULL;

    PS_PRINT_SUC("pm unregistered over!");

    return SUCCESS;
}
EXPORT_SYMBOL_GPL(ps_pm_unregister_etc);


extern   uint8  uc_wakeup_src_debug;
irqreturn_t bfg_wake_host_isr_etc(int irq, void *dev_id)
{
    struct ps_core_s *ps_core_d = NULL;
    uint64 flags;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (!memcheck_is_working())
    {
        return IRQ_NONE;
    }
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return IRQ_NONE;
    }

    PS_PRINT_INFO("%s\n", __func__);
    spin_lock_irqsave(&pm_data->uart_state_spinlock,flags);
    if(UART_BPS_CHG_SEND_ACK <= pm_data->uart_state)
    {
        if(UART_BPS_CHG_SEND_ACK == pm_data->uart_state)
        {
            bfgx_uart_state_set(UART_BPS_CHG_IN_PROGRESS);
        }
        else if (UART_BPS_CHG_SEND_COMPLETE == pm_data->uart_state)
        {
            bfgx_uart_state_set(UART_READY);
        }

        spin_unlock_irqrestore(&pm_data->uart_state_spinlock,flags);
        return IRQ_HANDLED;
    }
    spin_unlock_irqrestore(&pm_data->uart_state_spinlock,flags);

    pm_data->bfg_wakeup_host++;

    uc_wakeup_src_debug = 1;

    ps_get_core_reference_etc(&ps_core_d);
    if (ps_core_d == NULL)
    {
        PS_PRINT_ERR("ps_core_d is null\n");
        return IRQ_NONE;
    }

    spin_lock_irqsave(&pm_data->wakelock_protect_spinlock, flags);
    bfg_wake_lock_etc();
    bfgx_state_set_etc(BFGX_ACTIVE);
    complete(&pm_data->dev_ack_comp);
    spin_unlock_irqrestore(&pm_data->wakelock_protect_spinlock, flags);

    queue_work(pm_data->wkup_dev_workqueue, &pm_data->send_disallow_msg_work);

    return IRQ_HANDLED;
}

/*return 1 for wifi power on,0 for off.*/
oal_int32 hi110x_get_wifi_power_stat_etc(oal_void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return 0;
    }
    return (pm_data->pst_wlan_pm_info->ul_wlan_power_state != POWER_STATE_SHUTDOWN);
}
EXPORT_SYMBOL(hi110x_get_wifi_power_stat_etc);

STATIC int low_power_remove(void)
{
    int ret = 0;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    wlan_pm_exit_etc();

    /*delete timer*/
    del_timer_sync(&pm_data->bfg_timer);
    pm_data->bfg_timer_mod_cnt = 0;
    pm_data->bfg_timer_mod_cnt_pre = 0;

    del_timer_sync(&pm_data->dev_ack_timer);
    /*destory wake lock*/
    oal_wake_lock_exit(&pm_data->bfg_wake_lock_etc);
    /*free platform driver data struct*/
    kfree(pm_data);

    pm_data = NULL;

    pm_set_drvdata(NULL);

    return ret;
}

STATIC void devack_timer_expire(uint64 data)
{
    uint64 flags;
    struct pm_drv_data  *pm_data = (struct pm_drv_data*)data;
    if (unlikely(NULL == pm_data))
    {
        PS_PRINT_ERR("devack timer para is null\n");
        return;
    }

    PS_PRINT_INFO("%s\n", __func__);

    if (1 == board_get_bwkup_gpio_val_etc())
    {
        pm_data->uc_dev_ack_wait_cnt++;
        if(WAIT_DEVACK_CNT > pm_data->uc_dev_ack_wait_cnt)
        {
            mod_timer(&pm_data->dev_ack_timer, jiffies + (WAIT_DEVACK_MSEC * HZ / 1000));
            return;
        }
        /* device doesn't agree to sleep */
        PS_PRINT_INFO("device does not agree to sleep\n");
        if (unlikely(1 == pm_data->rcvdata_bef_devack_flag))
        {
            PS_PRINT_INFO("device send data to host before dev rcv allow slp msg\n");
            pm_data->rcvdata_bef_devack_flag = 0;
        }

        bfgx_state_set_etc(BFGX_ACTIVE);
        bfgx_uart_state_set(UART_READY);
        /* we mod timer at any time, since we could get another chance to sleep
           in exception case like:dev agree to slp after this ack timer expired
         */
        if(!bfgx_other_subsys_all_shutdown_etc(BFGX_GNSS))
        {
            mod_timer(&pm_data->bfg_timer, jiffies + (BT_SLEEP_TIME * HZ/1000));
            pm_data->bfg_timer_mod_cnt++;
        }

        complete(&pm_data->dev_ack_comp);
    }
    else
    {
        spin_lock_irqsave(&pm_data->wakelock_protect_spinlock, flags);
        if (BFGX_ACTIVE == pm_data->bfgx_dev_state)
        {
            PS_PRINT_INFO("wkup isr occur during wait for dev allow ack\n");
        }
        else
        {
            pm_data->ps_pm_interface->operate_beat_timer(BEAT_TIMER_DELETE);
            bfg_wake_unlock_etc();
        }
        spin_unlock_irqrestore(&pm_data->wakelock_protect_spinlock, flags);

        complete(&pm_data->dev_ack_comp);

        pm_data->uc_dev_ack_wait_cnt = 0;
    }

}

STATIC int low_power_probe(void)
{
    int ret = 0;
    struct pm_drv_data  *pm_data = NULL;
    struct workqueue_struct * host_wkup_dev_workq = NULL;

    pm_data = kzalloc(sizeof(struct pm_drv_data), GFP_KERNEL);
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("no mem to allocate pm_data\n");
        goto PMDATA_MALLOC_FAIL;
    }

    pm_data->pst_wlan_pm_info = wlan_pm_init_etc();
    if(0 == pm_data->pst_wlan_pm_info)
    {
        PS_PRINT_ERR("no mem to allocate wlan_pm_info\n");
        goto WLAN_INIT_FAIL;
    }

    /*FPGA版本支持2M，动态修改*/
    if(!isAsic_etc())
    {
        g_default_baud_rate = LOW_FREQ_BAUD_RATE;
    }
    PS_PRINT_INFO("init baudrate=%d\n",g_default_baud_rate);

    pm_data->firmware_cfg_init_flag  = 0;
    pm_data->rcvdata_bef_devack_flag = 0;
    pm_data->bfgx_dev_state         = BFGX_SLEEP;
    pm_data->bfgx_pm_ctrl_enable    = BFGX_PM_DISABLE;
    pm_data->bfgx_lowpower_enable   = BFGX_PM_ENABLE;            /* enable host low_power function defaultly */

    pm_data->bfgx_bt_lowpower_enable   = BFGX_PM_ENABLE;
    pm_data->bfgx_gnss_lowpower_enable = BFGX_PM_DISABLE;
    pm_data->bfgx_nfc_lowpower_enable  = BFGX_PM_DISABLE;

    atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);
    atomic_set(&pm_data->bfg_needwait_devboot_flag, NEED_SET_FLAG);

    spin_lock_init(&pm_data->bfg_irq_spinlock);
    pm_data->board  = get_hi110x_board_info_etc();
    pm_data->bfg_irq = pm_data->board->bfgx_irq;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44))
    ret = request_irq(pm_data->bfg_irq, bfg_wake_host_isr_etc, IRQF_DISABLED | IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND, "bfgx_wake_host", NULL);
#else
    ret = request_irq(pm_data->bfg_irq, bfg_wake_host_isr_etc, IRQF_DISABLED | IRQF_TRIGGER_RISING , "bfgx_wake_host", NULL);
#endif
    if (ret < 0)
    {
        PS_PRINT_ERR("couldn't acquire %s IRQ\n", PROC_NAME_GPIO_BFGX_WAKEUP_HOST);
       goto REQ_IRQ_FAIL;
    }

    disable_irq_nosync(pm_data->bfg_irq);
    pm_data->ul_irq_stat = 1;/*irq diabled default.*/

    /* create an ordered workqueue with @max_active = 1 & WQ_UNBOUND flag to wake up device */
    host_wkup_dev_workq = create_singlethread_workqueue("wkup_dev_workqueue");
    if (!host_wkup_dev_workq)
    {
        PS_PRINT_ERR("create wkup workqueue failed\n");
        goto CREATE_WORKQ_FAIL;
    }
    pm_data->wkup_dev_workqueue = host_wkup_dev_workq;
    INIT_WORK(&pm_data->wkup_dev_work, host_wkup_dev_work_etc);
    INIT_WORK(&pm_data->send_disallow_msg_work, host_send_disallow_msg_etc);
    INIT_WORK(&pm_data->send_allow_sleep_work, host_allow_bfg_sleep_etc);
    INIT_WORK(&pm_data->baud_change_work, bfgx_uart_baud_change_work);

    /*init bfg wake lock */
    oal_wake_lock_init(&pm_data->bfg_wake_lock_etc, BFG_LOCK_NAME);
    /*init mutex*/
    mutex_init(&pm_data->host_mutex);

    /*init spinlock*/
    spin_lock_init(&pm_data->uart_state_spinlock);
    spin_lock_init(&pm_data->wakelock_protect_spinlock);

    pm_data->uart_state          = UART_NOT_READY;
    pm_data->uart_baud_switch_to = g_default_baud_rate;
    /* init timer */
    init_timer(&pm_data->dev_ack_timer);
    pm_data->dev_ack_timer.function = devack_timer_expire;
    pm_data->dev_ack_timer.data     = (unsigned long)pm_data;
    pm_data->uc_dev_ack_wait_cnt    = 0;

    /* init bfg data timer */
    init_timer(&pm_data->bfg_timer);
    pm_data->bfg_timer.function = bfg_timer_expire_etc;
    pm_data->bfg_timer.data     = (unsigned long)pm_data;
    pm_data->bfg_timer_mod_cnt  = 0;
    pm_data->bfg_timer_mod_cnt_pre = 0;
    pm_data->bfg_timer_check_time  = 0;
    pm_data->rx_pkt_gnss_pre = 0;
    pm_data->gnss_votesleep_check_cnt = 0;

    PS_PRINT_INFO("uart baud change support version\n");
    init_timer(&pm_data->baud_change_timer);
    pm_data->baud_change_timer.function = bfgx_uart_baud_change_expire;
    pm_data->baud_change_timer.data     = (unsigned long)pm_data;

    /* init completion */
    init_completion(&pm_data->host_wkup_dev_comp);
    init_completion(&pm_data->dev_ack_comp);
    init_completion(&pm_data->dev_bootok_ack_comp);

    /*set driver data*/
    pm_set_drvdata(pm_data);

    /* register host pm */
    ret = register_pm_notifier(&pf_suspend_notifier);
    if (ret < 0)
    {
        PS_PRINT_ERR("%s : register_pm_notifier failed!\n", __func__);
    }

    return OAL_SUCC;



CREATE_WORKQ_FAIL:

REQ_IRQ_FAIL:

WLAN_INIT_FAIL:
    kfree(pm_data);
PMDATA_MALLOC_FAIL:
    return -ENOMEM;
}

int low_power_init_etc(void)
{
    int ret = -FAILURE;

    ret = low_power_probe();
    if (SUCCESS != ret)
    {
        PS_PRINT_ERR("low_power_init_etc: low_power_probe fail\n");
    }

     PS_PRINT_INFO("low_power_init_etc: success\n");
    return ret;
}

void  low_power_exit_etc(void)
{
    low_power_remove();
    firmware_cfg_clear_etc();
}

#ifdef CONFIG_HI110X_GPS_SYNC
#define GNSS_SYNC_IOREMAP_SIZE 0x1000
struct gnss_sync_data * gnss_sync_data_t = NULL;

struct gnss_sync_data * gnss_get_sync_data(void)
{
    return gnss_sync_data_t;
}

static void gnss_set_sync_data(struct gnss_sync_data * data)
{
    gnss_sync_data_t = data;
}

static int gnss_sync_probe(struct platform_device *pdev)
{
    struct gnss_sync_data *sync_info = NULL;
    struct device_node *np = pdev->dev.of_node;
    uint32 addr_base;
    int32 ret = SUCCESS;
    uint32 version = 0;

    PS_PRINT_INFO("[GPS] gnss sync probe start\n");

    ret = of_property_read_u32(np, "version", &version);
    if (SUCCESS != ret)
    {
        PS_PRINT_ERR("[GPS] get gnss sync version failed!\n");
        return -FAILURE;
    }

    if (0 == version)
    {
        PS_PRINT_INFO("[GPS] gnss sync version 0\n");
        return SUCCESS;
    }

    sync_info = kzalloc(sizeof(struct gnss_sync_data), GFP_KERNEL);
    if (!sync_info) {
        PS_PRINT_ERR("[GPS] alloc memory failed\n");
        return -ENOMEM;
    }

    sync_info->version = version;

    ret = of_property_read_u32(np, "addr_base", &addr_base);
    if (SUCCESS != ret)
    {
        PS_PRINT_ERR("[GPS] get gnss sync reg base failed!\n");
        ret = -FAILURE;
        goto sync_get_resource_fail;
    }

    sync_info->addr_base_virt = ioremap(addr_base, GNSS_SYNC_IOREMAP_SIZE);
    if (NULL == sync_info->addr_base_virt)
    {
        PS_PRINT_ERR("[GPS] gnss sync reg ioremap failed!\n");
        ret = -ENOMEM;
        goto sync_get_resource_fail;
    }

    ret = of_property_read_u32(np, "addr_offset", &sync_info->addr_offset);
    if (SUCCESS != ret)
    {
        PS_PRINT_ERR("[GPS] get gnss sync reg offset failed!\n");
        ret = -FAILURE;
        goto sync_get_resource_fail;
    }

    PS_PRINT_INFO("[GPS] gnss sync probe is finished!\n");

    gnss_set_sync_data(sync_info);

    return SUCCESS;

sync_get_resource_fail:
    gnss_set_sync_data(NULL);
    kfree(sync_info);
    return ret;
}

static void gnss_sync_shutdown(struct platform_device *pdev)
{
    struct gnss_sync_data *sync_info = gnss_get_sync_data();
    PS_PRINT_INFO("[GPS] gnss sync shutdown!\n");

    if (!sync_info)
    {
        PS_PRINT_ERR("[GPS] gnss sync info is NULL.\n");
        return;
    }

    gnss_set_sync_data(NULL);
    kfree(sync_info);
    return;
}

#define DTS_COMP_GNSS_SYNC_NAME "hisilicon,hisi_gps_sync"

static const struct of_device_id gnss_sync_match_table[] = {
    {
        .compatible = DTS_COMP_GNSS_SYNC_NAME,   // compatible must match with which defined in dts
        .data = NULL,
    },
    {
    },
};

static struct platform_driver gnss_sync_driver = {
    .probe          = gnss_sync_probe,
    .suspend        = NULL,
    .remove         = NULL,
    .shutdown       = gnss_sync_shutdown,
    .driver = {
        .name = "hisi_gps_sync",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(gnss_sync_match_table), // dts required code
    },
};

int gnss_sync_init(void)
{
    int ret;
    ret = platform_driver_register(&gnss_sync_driver);
    if (ret)
    {
        PS_PRINT_ERR("[GPS] unable to register gnss sync driver!\n");
    }
    else
    {
        PS_PRINT_INFO("[GPS] gnss sync init ok!\n");
    }
    return ret;
}

void gnss_sync_exit(void)
{
    platform_driver_unregister(&gnss_sync_driver);
}
#endif

