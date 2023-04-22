

/*****************************************************************************
  1 Include Head file
*****************************************************************************/
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/tty.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/reboot.h>
#include <linux/fs.h>


#include "board.h"
#include "hw_bfg_ps.h"
#include "plat_type.h"
#include "plat_debug.h"
#include "plat_uart.h"
#include "plat_pm.h"
#include "bfgx_user_ctrl.h"
#include "bfgx_exception_rst.h"
#include "plat_firmware.h"
#include "plat_cali.h"

#include "oal_ext_if.h"
#ifdef BFGX_UART_DOWNLOAD_SUPPORT
#include "wireless_patch.h"
#endif
/*****************************************************************************
  Define global variable
*****************************************************************************/
/**
 * This references is the per-PS platform device in the arch/arm/
 * board-xx.c file.
 */
struct platform_device *hw_ps_device_etc = NULL;
STATIC int g_debug_cnt = 0;
DUMP_CMD_QUEUE dump_cmd_queue_etc;

uint32 g_bfgx_open_cmd_etc[BFGX_BUTT] =
{
    SYS_CFG_OPEN_BT,
    SYS_CFG_OPEN_FM,
    SYS_CFG_OPEN_GNSS,
    SYS_CFG_OPEN_IR,
    SYS_CFG_OPEN_NFC,
};

uint32 g_bfgx_close_cmd_etc[BFGX_BUTT] =
{
    SYS_CFG_CLOSE_BT,
    SYS_CFG_CLOSE_FM,
    SYS_CFG_CLOSE_GNSS,
    SYS_CFG_CLOSE_IR,
    SYS_CFG_CLOSE_NFC,
};

uint32 g_bfgx_open_cmd_timeout_etc[BFGX_BUTT] =
{
    WAIT_BT_OPEN_TIME,
    WAIT_FM_OPEN_TIME,
    WAIT_GNSS_OPEN_TIME,
    WAIT_IR_OPEN_TIME,
    WAIT_NFC_OPEN_TIME,
};

uint32 g_bfgx_close_cmd_timeout_etc[BFGX_BUTT] =
{
    WAIT_BT_CLOSE_TIME,
    WAIT_FM_CLOSE_TIME,
    WAIT_GNSS_CLOSE_TIME,
    WAIT_IR_CLOSE_TIME,
    WAIT_NFC_CLOSE_TIME,
};

const uint8 *g_bfgx_subsys_name_etc[BFGX_BUTT] =
{
    "BT",
    "FM",
    "GNSS",
    "IR",
    "NFC",
};

struct bt_data_combination g_st_bt_data_combination_etc = {0};

oal_int32 bfgx_open_ssi_dump = 0;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
module_param(bfgx_open_ssi_dump, int, S_IRUGO | S_IWUSR);
#endif

uint32 g_ul_gnss_me_thread_status   = DEV_THREAD_EXIT;
uint32 g_ul_gnss_lppe_thread_status = DEV_THREAD_EXIT;
volatile  bool   g_b_ir_only_mode   = false;
/*****************************************************************************
  Function implement
*****************************************************************************/

/**********************************************************************/
/* internal functions */
/**
 * Prototype    : ps_get_plat_reference_etc
 * Description  : reference the plat's dat,This references the per-PS
 *                  platform device in the arch/arm/board-xx.c file.
 * input        : use *hw_ps_device_etc
 * output       : the have registered platform device struct
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
int32 ps_get_plat_reference_etc(struct ps_plat_s **plat_data)
{
    struct platform_device   *pdev = NULL;
    struct ps_plat_s    *ps_plat_d = NULL;

    pdev = hw_ps_device_etc;
    if (!pdev)
    {
        *plat_data = NULL;
        PS_PRINT_ERR("%s pdev is NULL\n", __func__);
        return FAILURE;
    }

    ps_plat_d  = dev_get_drvdata(&pdev->dev);
    *plat_data = ps_plat_d;

    return SUCCESS;
}

/**
 * Prototype    : ps_get_core_reference_etc
 * Description  : reference the core's data,This references the per-PS
 *                  platform device in the arch/xx/board-xx.c file..
 * input        : use *hw_ps_device_etc
 * output       : the have registered ps_core_d struct
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
int32 ps_get_core_reference_etc(struct ps_core_s **core_data)
{
    struct platform_device *pdev = NULL;
    struct ps_plat_s  *ps_plat_d = NULL;

    pdev = hw_ps_device_etc;
    if (!pdev)
    {
        *core_data = NULL;
        PS_PRINT_ERR("%s pdev is NULL\n", __func__);
        return FAILURE;
    }

    ps_plat_d  = dev_get_drvdata(&pdev->dev);
    if (NULL == ps_plat_d)
    {
        PS_PRINT_ERR("ps_plat_d is NULL\n");
        return FAILURE;
    }

    *core_data = ps_plat_d->core_data;

    return SUCCESS;
}

/**
 * Prototype    : ps_chk_bfg_active_etc
 * Description  : to chk wether or not bfg active
 *
 * input        : ps_plat_d
 * output       : no
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
bool ps_chk_bfg_active_etc(struct ps_core_s *ps_core_d)
{
    int32 i = 0;
    for (i = 0; i < BFGX_BUTT; i++)
    {
        if (POWER_STATE_SHUTDOWN != atomic_read(&ps_core_d->bfgx_info[i].subsys_state))
        {
            return true;
        }
    }

    return false;
}

/* only gnss is open and it agree to sleep */
bool ps_chk_only_gnss_and_cldslp_etc(struct ps_core_s *ps_core_d)
{
    struct pm_drv_data  *pm_data = pm_get_drvdata_etc();
    if ((POWER_STATE_SHUTDOWN == atomic_read(&ps_core_d->bfgx_info[BFGX_BT].subsys_state)) &&
        (POWER_STATE_SHUTDOWN == atomic_read(&ps_core_d->bfgx_info[BFGX_FM].subsys_state)) &&
        (POWER_STATE_SHUTDOWN == atomic_read(&ps_core_d->bfgx_info[BFGX_IR].subsys_state)) &&
        (POWER_STATE_SHUTDOWN == atomic_read(&ps_core_d->bfgx_info[BFGX_NFC].subsys_state)) &&
        (POWER_STATE_OPEN == atomic_read(&ps_core_d->bfgx_info[BFGX_GNSS].subsys_state))   &&
        (GNSS_AGREE_SLEEP == atomic_read(&pm_data->gnss_sleep_flag)) &&
        (BFGX_ACTIVE == pm_data->bfgx_dev_state))
    {
        return true;
    }
    return false;
}

bool ps_chk_tx_queue_empty(struct ps_core_s *ps_core_d)
{
    PS_PRINT_FUNCTION_NAME;
    if (NULL == ps_core_d)
    {
        PS_PRINT_ERR("ps_core_d is NULL");
        return true;
    }

    if (skb_queue_empty(&ps_core_d->tx_urgent_seq)&&
        skb_queue_empty(&ps_core_d->tx_high_seq)&&
        skb_queue_empty(&ps_core_d->tx_low_seq))
    {
        return true;
    }
    return false;
}

/**
 * Prototype    : ps_alloc_skb_etc
 * Description  : allocate mem for new skb
 *
 * input        : ps_plat_d
 * output       : no
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
struct sk_buff *ps_alloc_skb_etc(uint16 len)
{
    struct sk_buff *skb = NULL;

    PS_PRINT_FUNCTION_NAME;

    skb = alloc_skb(len, GFP_KERNEL);
    if (NULL == skb)
	{
        PS_PRINT_WARNING("can't allocate mem for new skb, len=%d\n", len);
        return NULL;
    }

    skb_put(skb, len);

    return skb;
}

/**
 * Prototype    : ps_kfree_skb_etc
 * Description  : when close a function, kfree skb
 * input        : ps_core_d, skb type
 * output       : no
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/12/26
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
void ps_kfree_skb_etc(struct ps_core_s *ps_core_d, uint8 type)
{
    struct sk_buff *skb = NULL;

    PS_PRINT_FUNCTION_NAME;
    if (NULL == ps_core_d)
    {
        PS_PRINT_ERR("ps_core_d is NULL");
        return;
    }

    while ((skb = ps_skb_dequeue_etc(ps_core_d, type)))
    {
        kfree_skb(skb);
    }

    switch (type)
    {
    case TX_URGENT_QUEUE:
        skb_queue_purge(&ps_core_d->tx_urgent_seq);
        break;
    case TX_HIGH_QUEUE:
        skb_queue_purge(&ps_core_d->tx_high_seq);
        break;
    case TX_LOW_QUEUE:
        skb_queue_purge(&ps_core_d->tx_low_seq);
        break;
    case RX_GNSS_QUEUE:
        skb_queue_purge(&ps_core_d->bfgx_info[BFGX_GNSS].rx_queue);
        break;
    case RX_FM_QUEUE:
        skb_queue_purge(&ps_core_d->bfgx_info[BFGX_FM].rx_queue);
        break;
    case RX_BT_QUEUE:
        skb_queue_purge(&ps_core_d->bfgx_info[BFGX_BT].rx_queue);
        break;
    case RX_DBG_QUEUE:
        skb_queue_purge(&ps_core_d->rx_dbg_seq);
        break;
    case RX_NFC_QUEUE:
        skb_queue_purge(&ps_core_d->bfgx_info[BFGX_NFC].rx_queue);
        break;
    case RX_IR_QUEUE:
        skb_queue_purge(&ps_core_d->bfgx_info[BFGX_IR].rx_queue);
        break;
    default:
        PS_PRINT_ERR("queue type is error, type=%d\n", type);
        break;
    }
    return;
}

/**
 * Prototype    : ps_restore_skbqueue_etc
 * Description  : when err and restore skb to seq function.
 * input        : ps_core_d, skb
 * output       : not
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
int32 ps_restore_skbqueue_etc(struct ps_core_s *ps_core_d, struct sk_buff *skb, uint8 type)
{
    PS_PRINT_FUNCTION_NAME;

    if (unlikely((NULL == skb) || (NULL == ps_core_d)))
    {
        PS_PRINT_ERR(" skb or ps_core_d is NULL\n");
        return -EINVAL;
    }

    switch (type)
    {
    case RX_GNSS_QUEUE:
        skb_queue_head(&ps_core_d->bfgx_info[BFGX_GNSS].rx_queue, skb);
        break;
    case RX_FM_QUEUE:
        skb_queue_head(&ps_core_d->bfgx_info[BFGX_FM].rx_queue, skb);
        break;
    case RX_BT_QUEUE:
        skb_queue_head(&ps_core_d->bfgx_info[BFGX_BT].rx_queue, skb);
        break;
    case RX_IR_QUEUE:
        skb_queue_head(&ps_core_d->bfgx_info[BFGX_IR].rx_queue, skb);
        break;
    case RX_NFC_QUEUE:
        skb_queue_head(&ps_core_d->bfgx_info[BFGX_NFC].rx_queue, skb);
        break;
    case RX_DBG_QUEUE:
        skb_queue_head(&ps_core_d->rx_dbg_seq, skb);
        break;

    default:
        PS_PRINT_ERR("queue type is error, type=%d\n", type);
        break;
    }

    return 0;
}

/* prepare to visit dev_node
*/
int32 prepare_to_visit_node_etc(struct ps_core_s *ps_core_d)
{
    struct pm_drv_data *pm_data = NULL;
    uint8 uart_ready = UART_NOT_READY;
    uint64 flags;

    pm_data = pm_get_drvdata_etc();
    if (unlikely(NULL == pm_data))
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -EFAULT;
    }

    /* lock wake_lock */
    bfg_wake_lock_etc();

    /* try to wake up device */
    spin_lock_irqsave(&pm_data->uart_state_spinlock,flags);
    atomic_inc(&ps_core_d->node_visit_flag);    /* mark someone is visiting dev node */
    uart_ready = ps_core_d->ps_pm->bfgx_uart_state_get();
    spin_unlock_irqrestore(&pm_data->uart_state_spinlock,flags);

    if (UART_NOT_READY == uart_ready)
    {
        if (0 != host_wkup_dev_etc())
        {
            PS_PRINT_ERR("wkup device FAILED!\n");
            atomic_dec(&ps_core_d->node_visit_flag);
            return -EIO;
        }
    }
    return 0;
}

/* we should do something before exit from visiting dev_node */
int32 post_to_visit_node_etc(struct ps_core_s *ps_core_d)
{
    atomic_dec(&ps_core_d->node_visit_flag);

    return 0;
}

int32 alloc_seperted_rx_buf_etc(uint8 subsys, uint32 len,uint8 alloctype)
{
    struct ps_core_s *ps_core_d = NULL;
    struct bfgx_sepreted_rx_st *pst_sepreted_data = NULL;
    uint8 *p_rx_buf = NULL;

    if (subsys >= BFGX_BUTT)
    {
        PS_PRINT_ERR("subsys out of range! subsys=%d\n", subsys);
        return -EINVAL;
    }

    if (BFGX_BT == subsys)
    {
        PS_PRINT_DBG("%s no sepreted buf\n", g_bfgx_subsys_name_etc[subsys]);
        return 0;
    }

    if (alloctype >= ALLOC_BUFF)
    {
        PS_PRINT_ERR("alloc type out of range! subsys=%d,alloctype=%d\n", subsys,alloctype);
        return -EINVAL;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -EINVAL;
    }
    pst_sepreted_data = &ps_core_d->bfgx_info[subsys].sepreted_rx;

    if (KZALLOC == alloctype)
    {
        p_rx_buf = kzalloc(len, GFP_KERNEL);
    }
    else if (VMALLOC == alloctype)
    {
        p_rx_buf = vmalloc(len);
    }

    if (NULL == p_rx_buf)
    {
        PS_PRINT_ERR("alloc failed! subsys=%d, len=%d\n", subsys, len);
        return -ENOMEM;
    }

    spin_lock(&pst_sepreted_data->sepreted_rx_lock);
    pst_sepreted_data->rx_prev_seq = RX_SEQ_NULL;
    pst_sepreted_data->rx_buf_all_len = 0;
    pst_sepreted_data->rx_buf_ptr = p_rx_buf;
    pst_sepreted_data->rx_buf_org_ptr = p_rx_buf;
    spin_unlock(&pst_sepreted_data->sepreted_rx_lock);

    return 0;
}

int32 free_seperted_rx_buf_etc(uint8 subsys,uint8 alloctype)
{
    struct ps_core_s *ps_core_d = NULL;
    struct bfgx_sepreted_rx_st *pst_sepreted_data = NULL;

    if (subsys >= BFGX_BUTT)
    {
        PS_PRINT_ERR("subsys out of range! subsys=%d\n", subsys);
        return -EINVAL;
    }

    if (BFGX_BT == subsys)
    {
        PS_PRINT_DBG("%s no sepreted buf\n", g_bfgx_subsys_name_etc[subsys]);
        return 0;
    }

    if (alloctype >= ALLOC_BUFF)
    {
        PS_PRINT_ERR("alloc type out of range! subsys=%d,alloctype=%d\n", subsys,alloctype);
        return -EINVAL;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }
    pst_sepreted_data = &ps_core_d->bfgx_info[subsys].sepreted_rx;

    spin_lock(&pst_sepreted_data->sepreted_rx_lock);
    if (NULL != pst_sepreted_data->rx_buf_org_ptr)
    {
        if (KZALLOC == alloctype)
        {
            kfree(pst_sepreted_data->rx_buf_org_ptr);
        }
        else if (VMALLOC == alloctype)
        {
            vfree(pst_sepreted_data->rx_buf_org_ptr);
        }
    }
    pst_sepreted_data->rx_prev_seq = RX_SEQ_NULL;
    pst_sepreted_data->rx_buf_all_len = 0;
    pst_sepreted_data->rx_buf_ptr = NULL;
    pst_sepreted_data->rx_buf_org_ptr = NULL;
    spin_unlock(&pst_sepreted_data->sepreted_rx_lock);

    return 0;
}

int32 bfgx_open_fail_process_etc(uint8 subsys, int32 error)
{
    struct ps_core_s *ps_core_d = NULL;

    if (subsys >= BFGX_BUTT)
    {
         PS_PRINT_ERR("subsys is error, subsys=[%d]\n", subsys);
         return BFGX_POWER_FAILED;
    }

    if (error >= BFGX_POWER_ENUM_BUTT)
    {
         PS_PRINT_ERR("error is error, error=[%d]\n", error);
         return BFGX_POWER_FAILED;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return BFGX_POWER_FAILED;
    }

    PS_PRINT_INFO("bfgx open fail, type=[%d]\n", error);

    if(bfgx_open_ssi_dump)
    {
        ssi_dump_device_regs(SSI_MODULE_MASK_COMM | SSI_MODULE_MASK_BCTRL);
    }

    switch (error)
    {
    case BFGX_POWER_PULL_POWER_GPIO_FAIL:
    case BFGX_POWER_TTY_OPEN_FAIL:
    case BFGX_POWER_TTY_FLOW_ENABLE_FAIL:
        break;

    case BFGX_POWER_WIFI_DERESET_BCPU_FAIL:
    case BFGX_POWER_WIFI_ON_BOOT_UP_FAIL:
        if (BFGX_POWER_SUCCESS == plat_power_fail_exception_info_set_etc(SUBSYS_BFGX, subsys, BFGX_POWERON_FAIL))
        {
            bfgx_system_reset_etc();
            plat_power_fail_process_done_etc();
        }
        else
        {
            PS_PRINT_ERR("bfgx power fail, set exception info fail\n");
        }

        ps_core_d->ps_pm->bfg_power_set(subsys, BFG_POWER_GPIO_DOWN);
        break;

    case BFGX_POWER_WIFI_OFF_BOOT_UP_FAIL:
    case BFGX_POWER_DOWNLOAD_FIRMWARE_FAIL:
        ps_core_d->ps_pm->bfg_power_set(subsys, BFG_POWER_GPIO_DOWN);
        break;

    case BFGX_POWER_WAKEUP_FAIL:
    case BFGX_POWER_OPEN_CMD_FAIL:
        if (BFGX_POWER_SUCCESS == plat_power_fail_exception_info_set_etc(SUBSYS_BFGX, subsys, BFGX_POWERON_FAIL))
        {
            if (EXCEPTION_SUCCESS != bfgx_subsystem_reset_etc())
            {
                PS_PRINT_ERR("bfgx_subsystem_reset_etc failed \n");
            }
            plat_power_fail_process_done_etc();
        }
        else
        {
            PS_PRINT_ERR("bfgx power fail, set exception info fail\n");
        }

        ps_core_d->ps_pm->bfg_power_set(subsys, BFG_POWER_GPIO_DOWN);
        break;

    default:
        PS_PRINT_ERR("error is undefined, error=[%d]\n", error);
        break;
    }

    return BFGX_POWER_SUCCESS;
}

/**********************************************************************/

int32 uart_wifi_open_etc(void)
{
    struct ps_core_s *ps_core_d = NULL;
    uint64 timeleft;
    int32 ret;

    PS_PRINT_INFO("%s\n", __func__);

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -EINVAL;
    }

    /*如果BFGIN睡眠，则唤醒之*/
    ret = prepare_to_visit_node_etc(ps_core_d);
    if (ret < 0)
    {
        PS_PRINT_ERR("prepare work FAIL\n");
        return ret;
    }

    PS_PRINT_INFO("uart open WCPU\n");
    INIT_COMPLETION(ps_core_d->wait_wifi_opened);
    /* tx sys bt open info */
    ps_uart_state_pre_etc(ps_core_d->tty);
    ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_OPEN_WIFI);

    timeleft = wait_for_completion_timeout(&ps_core_d->wait_wifi_opened, msecs_to_jiffies(WAIT_WIFI_OPEN_TIME));
    if (!timeleft)
    {
        ps_uart_state_dump_etc(ps_core_d->tty);
        PS_PRINT_ERR("wait wifi open ack timeout\n");
        post_to_visit_node_etc(ps_core_d);
        return -ETIMEDOUT;
    }

    post_to_visit_node_etc(ps_core_d);

    msleep(20);

    return SUCCESS;
}

/**********************************************************************/

int32 uart_wifi_close_etc(void)
{
    struct ps_core_s *ps_core_d = NULL;
    uint64 timeleft;
    int32  ret;

    PS_PRINT_INFO("%s\n", __func__);

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -EINVAL;
    }

    /*如果BFGIN睡眠，则唤醒之*/
    ret = prepare_to_visit_node_etc(ps_core_d);
    if (ret < 0)
    {
        PS_PRINT_ERR("prepare work FAIL\n");
        return ret;
    }

    PS_PRINT_INFO("uart close WCPU\n");

    INIT_COMPLETION(ps_core_d->wait_wifi_closed);
    ps_uart_state_pre_etc(ps_core_d->tty);
    ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_CLOSE_WIFI);

    timeleft = wait_for_completion_timeout(&ps_core_d->wait_wifi_closed, msecs_to_jiffies(WAIT_WIFI_CLOSE_TIME));
    if (!timeleft)
    {
        ps_uart_state_dump_etc(ps_core_d->tty);
        PS_PRINT_ERR("wait wifi close ack timeout\n");
        post_to_visit_node_etc(ps_core_d);
        return -ETIMEDOUT;
    }

    PS_PRINT_WARNING("uart close WCPU done,gpio level[%d]\n",board_get_wlan_wkup_gpio_val_etc());

    post_to_visit_node_etc(ps_core_d);

    return SUCCESS;
}

/**********************************************************************/

int32 uart_bfgx_close_cmd_etc(void)
{
#define wait_close_times  (100)
    struct ps_core_s *ps_core_d = NULL;
    int bwkup_gpio_val = 1;
    int32 ret;
    int i;

    PS_PRINT_INFO("%s\n", __func__);

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -EINVAL;
    }

    /* 单红外dev不处理系统消息 */
    if (g_b_ir_only_mode)
    {
        return SUCCESS;
    }

    /*如果BFGIN睡眠，则唤醒之*/
    ret = prepare_to_visit_node_etc(ps_core_d);
    if (ret < 0)
    {
        PS_PRINT_ERR("prepare work FAIL\n");
        return ret;
    }

    /*下发BFGIN shutdown命令*/
    PS_PRINT_INFO("uart shutdown BCPU\n");

    ps_uart_state_pre_etc(ps_core_d->tty);
    ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_SHUTDOWN_SLP);

    ret = FAILURE;
    for(i=0;i<wait_close_times;i++)
    {
        bwkup_gpio_val = board_get_bwkup_gpio_val_etc();
        if(0 == bwkup_gpio_val)
        {
            ret = SUCCESS;
            break;
        }
        msleep(10);
    }
    PS_PRINT_INFO("bfg gpio level:%d, i=%d\n", bwkup_gpio_val, i);

    if (FAILURE == ret)
    {
        ps_uart_state_dump_etc(ps_core_d->tty);
    }

    post_to_visit_node_etc(ps_core_d);

    return ret;
}

int32 bfgx_open_cmd_send_etc(uint32 subsys)
{
    uint64 timeleft;
    struct ps_core_s *ps_core_d = NULL;
    struct st_bfgx_data *pst_bfgx_data = NULL;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is null\n");
        return -EINVAL;
    }

    /* 单红外dev不处理系统消息 */
    if (g_b_ir_only_mode)
    {
        return SUCCESS;
    }

    if (subsys >= BFGX_BUTT)
    {
        PS_PRINT_ERR("subsys is err, subsys is [%d]\n", subsys);
        return -EINVAL;
    }

    if (BFGX_IR == subsys)
    {
        ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, g_bfgx_open_cmd_etc[subsys]);
        msleep(20);
        return 0;
    }

#ifdef PLATFORM_DEBUG_ENABLE
    if (!is_dfr_test_en(BFGX_POWEON_FAULT))
    {
        PS_PRINT_WARNING("[dfr test]:trigger powon fail\n");
        return -EINVAL;
    }
#endif

    if (BFGX_GNSS == subsys)
    {
        g_ul_gnss_me_thread_status   = DEV_THREAD_EXIT;
        g_ul_gnss_lppe_thread_status = DEV_THREAD_EXIT;
    }

    pst_bfgx_data = &ps_core_d->bfgx_info[subsys];

    INIT_COMPLETION(pst_bfgx_data->wait_opened);
    ps_uart_state_pre_etc(ps_core_d->tty);
    ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, g_bfgx_open_cmd_etc[subsys]);
    timeleft = wait_for_completion_timeout(&pst_bfgx_data->wait_opened, msecs_to_jiffies(g_bfgx_open_cmd_timeout_etc[subsys]));
    if (!timeleft)
    {
        ps_uart_state_dump_etc(ps_core_d->tty);
        PS_PRINT_ERR("wait %s open ack timeout\n", g_bfgx_subsys_name_etc[subsys]);
        if (BFGX_GNSS == subsys)
        {
            CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_OPEN_THREAD);
        }
        else if (BFGX_BT == subsys)
        {
            CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_BT, CHR_LAYER_DRV, 0, CHR_PLAT_DRV_ERROR_OPEN_THREAD);
        }
        return -ETIMEDOUT;
    }

    return 0;
}

int32 bfgx_close_cmd_send_etc(uint32 subsys)
{
    uint64 timeleft;
    struct ps_core_s *ps_core_d = NULL;
    struct st_bfgx_data *pst_bfgx_data = NULL;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is null\n");
        return -EINVAL;
    }

    if (subsys >= BFGX_BUTT)
    {
        PS_PRINT_ERR("subsys is err, subsys is [%d]\n", subsys);
        return -EINVAL;
    }

    /* 单红外dev不处理系统消息 */
    if (g_b_ir_only_mode)
    {
        return SUCCESS;
    }

#ifdef PLATFORM_DEBUG_ENABLE
    if (!is_dfr_test_en(BFGX_POWEOFF_FAULT))
    {
        PS_PRINT_WARNING("[dfr test]:trigger power off fail\n");
        return -EINVAL;
    }
#endif

    if (BFGX_IR == subsys)
    {
        ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, g_bfgx_close_cmd_etc[subsys]);
        msleep(20);
        return 0;
    }

    pst_bfgx_data = &ps_core_d->bfgx_info[subsys];

    INIT_COMPLETION(pst_bfgx_data->wait_closed);
    ps_uart_state_pre_etc(ps_core_d->tty);
    ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, g_bfgx_close_cmd_etc[subsys]);
    timeleft = wait_for_completion_timeout(&pst_bfgx_data->wait_closed, msecs_to_jiffies(g_bfgx_close_cmd_timeout_etc[subsys]));
    if (!timeleft)
    {
        ps_uart_state_dump_etc(ps_core_d->tty);
        PS_PRINT_ERR("wait %s close ack timeout\n", g_bfgx_subsys_name_etc[subsys]);
        if (BFGX_GNSS == subsys)
        {
            CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_CLOSE_THREAD);
        }
        else if (BFGX_BT == subsys)
        {
            CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_BT, CHR_LAYER_DRV, 0, CHR_PLAT_DRV_ERROR_CLOSE_THREAD);
        }
        return -ETIMEDOUT;
    }

    return 0;
}

/*单红外模式打开其他子系统时调用,关闭红外*/
int32 hw_ir_only_open_other_subsys(void)
{
    int32 ret = 0;

    ret = hw_bfgx_close(BFGX_IR);
    g_b_ir_only_mode = false;
    return ret;
}

int32 hw_bfgx_open(uint32 subsys)
{
    struct ps_core_s *ps_core_d = NULL;
    struct st_bfgx_data *pst_bfgx_data = NULL;
    int32  error = BFGX_POWER_SUCCESS;

    if (subsys >= BFGX_BUTT)
    {
        PS_PRINT_ERR("subsys is err, subsys is [%d]\n", subsys);
        return -EINVAL;
    }

    PS_PRINT_INFO("open %s\n", g_bfgx_subsys_name_etc[subsys]);

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == ps_core_d->ps_pm)
                 ||(NULL == ps_core_d->ps_pm->bfg_power_set)
                 ||(NULL == ps_core_d->ps_pm->pm_priv_data)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    pst_bfgx_data = &ps_core_d->bfgx_info[subsys];

    if (POWER_STATE_OPEN == atomic_read(&pst_bfgx_data->subsys_state))
    {
        PS_PRINT_WARNING("%s has opened! It's Not necessary to send msg to device\n", g_bfgx_subsys_name_etc[subsys]);
        return BFGX_POWER_SUCCESS;
    }

    if (BFGX_POWER_SUCCESS != alloc_seperted_rx_buf_etc(subsys, g_bfgx_rx_max_frame_etc[subsys], VMALLOC))
    {
        PS_PRINT_ERR("no mem allocate to read!\n");
        return -ENOMEM;
    }

    /*当单红外模式打开其他子系统时，需要关闭单红外才能其他子系统正常上电*/
    if (g_b_ir_only_mode && BFGX_IR != subsys)
    {
        if (hw_ir_only_open_other_subsys() != BFGX_POWER_SUCCESS)
        {
            PS_PRINT_ERR("ir only mode,but close ir only mode fail!\n");
            free_seperted_rx_buf_etc(subsys, VMALLOC);
            return -ENOMEM;
        }
    }

    error = ps_core_d->ps_pm->bfg_power_set(subsys, BFG_POWER_GPIO_UP);
    if (BFGX_POWER_SUCCESS != error)
    {
        PS_PRINT_ERR("set %s power on err! error = %d\n", g_bfgx_subsys_name_etc[subsys], error);
        goto bfgx_power_on_fail;
    }

    if (BFGX_POWER_SUCCESS != prepare_to_visit_node_etc(ps_core_d))
    {
        PS_PRINT_ERR("prepare work FAIL\n");
        error = BFGX_POWER_WAKEUP_FAIL;
        goto bfgx_wakeup_fail;
    }

    if (BFGX_POWER_SUCCESS != bfgx_open_cmd_send_etc(subsys))
    {
        PS_PRINT_ERR("bfgx open cmd fail\n");
        error = BFGX_POWER_OPEN_CMD_FAIL;
        goto bfgx_open_cmd_fail;
    }

    /* 单红外没有低功耗 */
    if (!g_b_ir_only_mode)
    {
        mod_timer(&ps_core_d->ps_pm->pm_priv_data->bfg_timer, jiffies + (BT_SLEEP_TIME * HZ/1000));
        ps_core_d->ps_pm->pm_priv_data->bfg_timer_mod_cnt++;
    }

    atomic_set(&pst_bfgx_data->subsys_state, POWER_STATE_OPEN);
    post_to_visit_node_etc(ps_core_d);

    return BFGX_POWER_SUCCESS;

bfgx_open_cmd_fail:
    post_to_visit_node_etc(ps_core_d);
bfgx_wakeup_fail:
bfgx_power_on_fail:
    free_seperted_rx_buf_etc(subsys, VMALLOC);
    bfgx_open_fail_process_etc(subsys, error);
    return BFGX_POWER_FAILED;
}

int32 hw_bfgx_close(uint32 subsys)
{
    struct ps_core_s *ps_core_d = NULL;
    struct st_bfgx_data *pst_bfgx_data = NULL;
    int32  ret = 0;
    struct pm_drv_data  *pm_data = pm_get_drvdata_etc();

    if (subsys >= BFGX_BUTT)
    {
        PS_PRINT_ERR("subsys is err, subsys is [%d]\n", subsys);
        return -EINVAL;
    }

    PS_PRINT_INFO("close %s\n", g_bfgx_subsys_name_etc[subsys]);

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == ps_core_d->ps_pm)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    pst_bfgx_data = &ps_core_d->bfgx_info[subsys];

    if (POWER_STATE_SHUTDOWN== atomic_read(&pst_bfgx_data->subsys_state))
    {
        PS_PRINT_WARNING("%s has closed! It's Not necessary to send msg to device\n", g_bfgx_subsys_name_etc[subsys]);
        return BFGX_POWER_SUCCESS;
    }
    wake_up_interruptible(&pst_bfgx_data->rx_wait);

    ret = prepare_to_visit_node_etc(ps_core_d);
    if (ret < 0)
    {
        /*唤醒失败，bfgx close时的唤醒失败不进行DFR恢复*/
        PS_PRINT_ERR("prepare work FAIL\n");
    }

    ret = bfgx_close_cmd_send_etc(subsys);
    if (ret < 0)
    {
        /*发送close命令失败，不进行DFR，继续进行下电流程，DFR恢复延迟到下次open时或者其他业务运行时进行*/
        PS_PRINT_ERR("bfgx close cmd fail\n");
    }

    atomic_set(&pst_bfgx_data->subsys_state, POWER_STATE_SHUTDOWN);
    free_seperted_rx_buf_etc(subsys, VMALLOC);
    ps_kfree_skb_etc(ps_core_d, g_bfgx_rx_queue_etc[subsys]);

    ps_core_d->rx_pkt_num[subsys] = 0;
    ps_core_d->tx_pkt_num[subsys] = 0;

    if(bfgx_other_subsys_all_shutdown_etc(BFGX_GNSS))
    {
        del_timer_sync(&pm_data->bfg_timer);
        pm_data->bfg_timer_mod_cnt = 0;
        pm_data->bfg_timer_mod_cnt_pre = 0;
    }

    ret = ps_core_d->ps_pm->bfg_power_set(subsys, BFG_POWER_GPIO_DOWN);
    if (ret)
    {
        /*下电失败，不进行DFR，DFR恢复延迟到下次open时或者其他业务运行时进行*/
        PS_PRINT_ERR("set %s power off err!ret = %d", g_bfgx_subsys_name_etc[subsys], ret);
    }

    post_to_visit_node_etc(ps_core_d);

    return 0;
}

/**********************************************************************/
/**
 * Prototype    : hw_bt_open
 * Description  : functions called from above bt hal,when open bt file
 *                  input : "/dev/hwbt"
 * input        : return 0 --> open is ok
 * output       : return !0--> open is false
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/12/26
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC int32 hw_bt_open(struct inode *inode, struct file *filp)
{
    int32  ret = BFGX_POWER_SUCCESS;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    if (NULL == pm_data)
    {
       PS_PRINT_ERR("pm_data is NULL!\n");
        return -EINVAL;
    }

    if (unlikely((NULL == inode)||(NULL == filp)))
    {
        PS_PRINT_ERR("%s param is error", __func__);
        return -EINVAL;
    }

    mutex_lock(&pm_data->host_mutex);

    ret = hw_bfgx_open(BFGX_BT);

    mutex_unlock(&pm_data->host_mutex);

    return ret;
}

/**
 * Prototype    : hw_bt_read
 * Description  : functions called from above bt hal,read count data to buf
 * input        : file handle, buf, count
 * output       : return size --> actual read byte size
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/12/26
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC ssize_t hw_bt_read(struct file *filp, int8 __user *buf,
                                size_t count,loff_t *f_pos)
{
    struct ps_core_s *ps_core_d = NULL;
    struct sk_buff *skb = NULL;
    uint16 count1;

    PS_PRINT_FUNCTION_NAME;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == buf)||(NULL == filp)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    if (NULL == (skb = ps_skb_dequeue_etc(ps_core_d, RX_BT_QUEUE)))
    {
        PS_PRINT_WARNING("bt read skb queue is null!\n");
        return 0;
    }

    /* read min value from skb->len or count */
    count1 = min_t(size_t, skb->len, count);
    if (copy_to_user(buf, skb->data, count1))
    {
        PS_PRINT_ERR("copy_to_user is err!\n");
        ps_restore_skbqueue_etc(ps_core_d, skb, RX_BT_QUEUE);
        return -EFAULT;
    }

    /* have read count1 byte */
    skb_pull(skb, count1);

    /* if skb->len = 0: read is over */
    if (0 == skb->len)
    {   /* curr skb data have read to user */
        kfree_skb(skb);
    }
    else
    {   /* if don,t read over; restore to skb queue */
        ps_restore_skbqueue_etc(ps_core_d, skb, RX_BT_QUEUE);
    }

    return count1;
}

/**
 * Prototype    : hw_bt_write
 * Description  : functions called from above bt hal,write count data to buf
 * input        : file handle, buf, count
 * output       : return size --> actual write byte size
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/12/26
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC ssize_t hw_bt_write(struct file *filp, const int8 __user *buf, size_t count,loff_t *f_pos)
{
    struct ps_core_s *ps_core_d = NULL;
    struct sk_buff *skb;
    uint16 total_len;
    int32  ret = 0;
    uint8 __user *puser = (uint8 __user *)buf;
    uint8 type = 0;

    PS_PRINT_FUNCTION_NAME;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == buf)||(NULL == filp)||(NULL == ps_core_d->ps_pm)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        g_st_bt_data_combination_etc.len = 0;
        return -EINVAL;
    }

    if (count > BT_TX_MAX_FRAME)
    {
        PS_PRINT_ERR("bt skb len is too large!\n");
        g_st_bt_data_combination_etc.len = 0;
        return -EINVAL;
    }

    /*适配Android O，BT数据分两次下发，先发数据类型，长度固定为1Byte，然后发数据，需要在驱动中组合起来发给device*/
    if (BT_TYPE_DATA_LEN == count)
    {
        get_user(type, puser);
        g_st_bt_data_combination_etc.type = type;
        g_st_bt_data_combination_etc.len  = count;

        return count;
    }

    /* if high queue num > MAX_NUM and don't write */
    if (ps_core_d->tx_high_seq.qlen > TX_HIGH_QUE_MAX_NUM)
    {
        PS_PRINT_ERR("bt tx high seqlen large than MAXNUM\n");
        g_st_bt_data_combination_etc.len = 0;
        return 0;
    }

    ret = prepare_to_visit_node_etc(ps_core_d);
    if (ret < 0)
    {
        PS_PRINT_ERR("prepare work fail, bring to reset work\n");
        g_st_bt_data_combination_etc.len = 0;
        plat_exception_handler_etc(SUBSYS_BFGX, THREAD_BT, BFGX_WAKEUP_FAIL);
        return ret;
    }

    /* modify expire time of uart idle timer */
    mod_timer(&ps_core_d->ps_pm->pm_priv_data->bfg_timer, jiffies + (BT_SLEEP_TIME * HZ/1000));
    ps_core_d->ps_pm->pm_priv_data->bfg_timer_mod_cnt++;

    total_len = count + g_st_bt_data_combination_etc.len + sizeof(struct ps_packet_head) + sizeof(struct ps_packet_end);

    skb  = ps_alloc_skb_etc(total_len);
    if (NULL == skb)
    {
        PS_PRINT_ERR("ps alloc skb mem fail\n");
        post_to_visit_node_etc(ps_core_d);
        g_st_bt_data_combination_etc.len = 0;
        return -EFAULT;
    }

    if (copy_from_user(&skb->data[sizeof(struct ps_packet_head) + g_st_bt_data_combination_etc.len], buf, count))
    {
        PS_PRINT_ERR("copy_from_user from bt is err\n");
        kfree_skb(skb);
        post_to_visit_node_etc(ps_core_d);
        g_st_bt_data_combination_etc.len = 0;
        return -EFAULT;
    }

    if (BT_TYPE_DATA_LEN == g_st_bt_data_combination_etc.len)
    {
        skb->data[sizeof(struct ps_packet_head)] = g_st_bt_data_combination_etc.type;
    }

    g_st_bt_data_combination_etc.len = 0;

    ps_add_packet_head_etc(skb->data, BT_MSG, total_len);
    ps_skb_enqueue_etc(ps_core_d, skb, TX_HIGH_QUEUE);
    queue_work(ps_core_d->ps_tx_workqueue, &ps_core_d->tx_skb_work);

    ps_core_d->tx_pkt_num[BFGX_BT]++;

    post_to_visit_node_etc(ps_core_d);

    return count;
}

/**
 * Prototype    : hw_bt_poll
 * Description  : called by bt func from hal;
 *                  check whether or not allow read and write
 * input        : file handle
 * output       : no
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/12/26
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC uint32 hw_bt_poll(struct file *filp, poll_table *wait)
{
    struct ps_core_s *ps_core_d = NULL;
    uint32 mask = 0;

    PS_PRINT_FUNCTION_NAME;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == filp)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    /* push curr wait event to wait queue */
    poll_wait(filp, &ps_core_d->bfgx_info[BFGX_BT].rx_wait, wait);

    if (ps_core_d->bfgx_info[BFGX_BT].rx_queue.qlen)
    {   /* have data to read */
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}

/**
 * Prototype    : hw_bt_ioctl
 * Description  : called by bt func from hal; default not use
 * input        : file handle, cmd, arg
 * output       : no
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/12/26
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC int64 hw_bt_ioctl(struct file *file, uint32 cmd, uint64 arg)
{
    PS_PRINT_FUNCTION_NAME;

    return 0;
}

/**
 * Prototype    : hw_bt_release
 * Description  : called by bt func from hal when close bt inode
 * input        : have opened file handle
 * output       : return 0 --> close is ok
 *                return !0--> close is false
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/12/26
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC int32 hw_bt_release(struct inode *inode, struct file *filp)
{
    int32  ret = 0;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    if (NULL == pm_data)
    {
       PS_PRINT_ERR("pm_data is NULL!\n");
        return -EINVAL;
    }

    if (unlikely((NULL == inode)||(NULL == filp)))
    {
        PS_PRINT_ERR("%s param is error", __func__);
        return -EINVAL;
    }

    mutex_lock(&pm_data->host_mutex);

    ret = hw_bfgx_close(BFGX_BT);

    mutex_unlock(&pm_data->host_mutex);

    return ret;
}

STATIC int32 hw_nfc_open(struct inode *inode, struct file *filp)
{
    int32  ret = BFGX_POWER_SUCCESS;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    if (NULL == pm_data)
    {
       PS_PRINT_ERR("pm_data is NULL!\n");
        return -EINVAL;
    }

    if (unlikely((NULL == inode)||(NULL == filp)))
    {
        PS_PRINT_ERR("%s param is error", __func__);
        return -EINVAL;
    }

    mutex_lock(&pm_data->host_mutex);

    ret = hw_bfgx_open(BFGX_NFC);

    mutex_unlock(&pm_data->host_mutex);

    return ret;
}

STATIC ssize_t hw_nfc_read(struct file *filp, int8 __user *buf,
                                size_t count,loff_t *f_pos)
{
    struct ps_core_s *ps_core_d = NULL;
    struct sk_buff *skb = NULL;
    uint16 count1;

    PS_PRINT_FUNCTION_NAME;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == buf)||(NULL == filp)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    if (NULL == (skb = ps_skb_dequeue_etc(ps_core_d, RX_NFC_QUEUE)))
    {
        PS_PRINT_WARNING("nfc read skb queue is null!");
        return 0;
    }

    count1 = min_t(size_t, skb->len, count);
    if (copy_to_user(buf, skb->data, count1))
    {
        PS_PRINT_ERR("copy_to_user is err!\n");
        ps_restore_skbqueue_etc(ps_core_d, skb, RX_NFC_QUEUE);
        return -EFAULT;
    }

    skb_pull(skb, count1);
    if (0 == skb->len)
    {
        kfree_skb(skb);
    }
    else
    {
        ps_restore_skbqueue_etc(ps_core_d, skb, RX_NFC_QUEUE);
    }

    return count1;
}

STATIC ssize_t hw_nfc_write(struct file *filp, const int8 __user *buf,
                                size_t count, loff_t *f_pos)
{
    struct ps_core_s *ps_core_d = NULL;
    int32 ret = 0;

    PS_PRINT_FUNCTION_NAME;

    ps_get_core_reference_etc(&ps_core_d);

    if (unlikely((NULL == ps_core_d)||(NULL == buf)||(NULL == filp)||(NULL == ps_core_d->ps_pm)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    if (count > NFC_TX_MAX_FRAME)
    {
        PS_PRINT_ERR("bt skb len is too large!\n");
        return -EINVAL;
    }

    if (ps_core_d->tx_low_seq.qlen > TX_LOW_QUE_MAX_NUM)
    {
        return 0;
    }

    ret = prepare_to_visit_node_etc(ps_core_d);
    if (ret < 0)
    {
        PS_PRINT_ERR("prepare work fail, bring to reset work\n");
        plat_exception_handler_etc(SUBSYS_BFGX, THREAD_NFC, BFGX_WAKEUP_FAIL);
        return ret;
    }
    /* modify expire time of uart idle timer */
    mod_timer(&ps_core_d->ps_pm->pm_priv_data->bfg_timer, jiffies + (BT_SLEEP_TIME * HZ/1000));
    ps_core_d->ps_pm->pm_priv_data->bfg_timer_mod_cnt++;

    /* to divide up packet function and tx to tty work */
    if (ps_tx_nfcbuf_etc(ps_core_d, buf, count) < 0)
    {
        PS_PRINT_ERR("hw_nfc_write is err\n");
        post_to_visit_node_etc(ps_core_d);
        return -EFAULT;
    }

    ps_core_d->tx_pkt_num[BFGX_NFC]++;

    post_to_visit_node_etc(ps_core_d);

    PS_PRINT_DBG("NFC data write end\n");

    return count;
}


STATIC uint32 hw_nfc_poll(struct file *filp, poll_table *wait)
{
    struct ps_core_s *ps_core_d = NULL;
    uint32 mask = 0;

    PS_PRINT_FUNCTION_NAME;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == filp)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    /* push curr wait event to wait queue */
    poll_wait(filp, &ps_core_d->bfgx_info[BFGX_NFC].rx_wait, wait);

    if (ps_core_d->bfgx_info[BFGX_NFC].rx_queue.qlen)
    {   /* have data to read */
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}

STATIC int32 hw_nfc_release(struct inode *inode, struct file *filp)
{
    int32  ret = 0;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    if (NULL == pm_data)
    {
       PS_PRINT_ERR("pm_data is NULL!\n");
        return -EINVAL;
    }

    if (unlikely((NULL == inode)||(NULL == filp)))
    {
        PS_PRINT_ERR("%s param is error", __func__);
        return -EINVAL;
    }

    mutex_lock(&pm_data->host_mutex);

    ret = hw_bfgx_close(BFGX_NFC);

    mutex_unlock(&pm_data->host_mutex);

    return ret;
}

/**
 * Prototype    : hw_ir_open
 * Description  : open ir device
 * input        : struct inode *inode, struct file *filp
 * output       : return 0
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2015/2/11
 *     Author       : wx222439
 *     Modification : Created function
 *
 */
STATIC int32 hw_ir_open(struct inode *inode, struct file *filp)
{
    int32  ret = BFGX_POWER_SUCCESS;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    if (NULL == pm_data)
    {
       PS_PRINT_ERR("pm_data is NULL!\n");
        return -EINVAL;
    }

    if (unlikely((NULL == inode)||(NULL == filp)))
    {
        PS_PRINT_ERR("%s param is error", __func__);
        return -EINVAL;
    }

    mutex_lock(&pm_data->host_mutex);

    /* judge ir only mode*/
    if ((true == wlan_is_shutdown_etc()) && (true == bfgx_is_shutdown_etc())
        && (HI1103_ASIC_PILOT == get_hi1103_asic_type()))
    {
        g_b_ir_only_mode = true;
    }

    ret = hw_bfgx_open(BFGX_IR);

    mutex_unlock(&pm_data->host_mutex);

    return ret;
}

/**
 * Prototype    : hw_ir_read
 * Description  : read ir node data
 * input        : struct file *filp, int8 __user *buf, size_t count,loff_t *f_pos
 * output       : return read len
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2015/2/11
 *     Author       : wx222439
 *     Modification : Created function
 *
 */
STATIC ssize_t hw_ir_read(struct file *filp, int8 __user *buf,
                                size_t count, loff_t *f_pos)
{
    uint16 ret_count;
    struct sk_buff *skb = NULL;
    struct ps_core_s *ps_core_d = NULL;

    PS_PRINT_FUNCTION_NAME;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == buf)||(NULL == filp)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    if (NULL == (skb = ps_skb_dequeue_etc(ps_core_d, RX_IR_QUEUE)))
    {
        PS_PRINT_DBG("ir read skb queue is null!\n");
        return 0;
    }

    ret_count = min_t(size_t, skb->len, count);
    if (copy_to_user(buf, skb->data, ret_count))
    {
        PS_PRINT_ERR("copy_to_user is err!\n");
        ps_restore_skbqueue_etc(ps_core_d, skb, RX_IR_QUEUE);
        return -EFAULT;
    }

    skb_pull(skb, ret_count);

    if (0 == skb->len)
    {
        kfree_skb(skb);
    }
    else
    {
        ps_restore_skbqueue_etc(ps_core_d, skb, RX_IR_QUEUE);
    }

    return ret_count;
}

/**
 * Prototype    : hw_ir_write
 * Description  : write data to ir node
 * input        : struct file *filp, const int8 __user *buf, size_t count, loff_t *f_pos
 * output       : return write len
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2015/2/11
 *     Author       : wx222439
 *     Modification : Created function
 *
 */
STATIC ssize_t hw_ir_write(struct file *filp, const int8 __user *buf,
                                size_t count, loff_t *f_pos)
{
    struct ps_core_s *ps_core_d = NULL;
    struct st_bfgx_data *pst_bfgx_data = NULL;
    int32 ret = 0;

    PS_PRINT_FUNCTION_NAME;

    ps_get_core_reference_etc(&ps_core_d);

    if (unlikely((NULL == ps_core_d)||(NULL == buf)||(NULL == filp)||(NULL == ps_core_d->ps_pm)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    pst_bfgx_data = &ps_core_d->bfgx_info[BFGX_IR];
    if (POWER_STATE_SHUTDOWN == atomic_read(&pst_bfgx_data->subsys_state))
    {
        PS_PRINT_WARNING("IR has closed! It's Not necessary to send msg to device\n");
        return 0;
    }

    if (count > IR_TX_MAX_FRAME)
    {
        PS_PRINT_ERR("IR skb len is too large!\n");
        return -EINVAL;
    }

    if (ps_core_d->tx_low_seq.qlen > TX_LOW_QUE_MAX_NUM)
    {
        return 0;
    }

    ret = prepare_to_visit_node_etc(ps_core_d);
    if (ret < 0)
    {
        PS_PRINT_ERR("prepare work fail, bring to reset work\n");
        plat_exception_handler_etc(SUBSYS_BFGX, THREAD_IR, BFGX_WAKEUP_FAIL);
        return ret;
    }

    if (!g_b_ir_only_mode)
    {
        /* modify expire time of uart idle timer */
        mod_timer(&ps_core_d->ps_pm->pm_priv_data->bfg_timer, jiffies + (BT_SLEEP_TIME * HZ/1000));
        ps_core_d->ps_pm->pm_priv_data->bfg_timer_mod_cnt++;
    }

    /* to divide up packet function and tx to tty work */
    if (ps_tx_irbuf_etc(ps_core_d, buf, count) < 0)
    {
        PS_PRINT_ERR("hw_ir_write is err\n");
        post_to_visit_node_etc(ps_core_d);
        return -EFAULT;
    }

    ps_core_d->tx_pkt_num[BFGX_IR]++;

    post_to_visit_node_etc(ps_core_d);

    PS_PRINT_DBG("IR data write end\n");

    return count;
}

/**
 * Prototype    : hw_ir_release
 * Description  : release
 * input        : struct inode *inode, struct file *filp
 * output       : return 0
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2015/2/11
 *     Author       : wx222439
 *     Modification : Created function
 *
 */
STATIC int32 hw_ir_release(struct inode *inode, struct file *filp)
{
    int32  ret = 0;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    if (NULL == pm_data)
    {
       PS_PRINT_ERR("pm_data is NULL!\n");
        return -EINVAL;
    }

    if (unlikely((NULL == inode)||(NULL == filp)))
    {
        PS_PRINT_ERR("%s param is error", __func__);
        return -EINVAL;
    }

    mutex_lock(&pm_data->host_mutex);

    ret = hw_bfgx_close(BFGX_IR);
    g_b_ir_only_mode = false;

    mutex_unlock(&pm_data->host_mutex);

    return ret;
}

/**********************************************************************/
/**
 * Prototype    : hw_fm_open
 * Description  : functions called from above fm hal,when open fm file
 *                  input : "/dev/hwfm"
 * input        : return 0 --> open is ok
 * output       : return !0--> open is false
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC int32 hw_fm_open(struct inode *inode, struct file *filp)
{
    int32  ret = BFGX_POWER_SUCCESS;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    if (NULL == pm_data)
    {
       PS_PRINT_ERR("pm_data is NULL!\n");
        return -EINVAL;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == inode)||(NULL == filp)))
    {
        PS_PRINT_ERR("%s param is error", __func__);
        return -EINVAL;
    }

    mutex_lock(&pm_data->host_mutex);

    ret = hw_bfgx_open(BFGX_FM);

    ps_core_d->fm_read_delay = FM_READ_DEFAULT_TIME;

    mutex_unlock(&pm_data->host_mutex);

    return ret;
}

/**
 * Prototype    : hw_fm_read
 * Description  : functions called from above fm hal,read count data to buf
 * input        : file handle, buf, count
 * output       : return size --> actual read byte size
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC ssize_t hw_fm_read(struct file *filp, int8 __user *buf,
                                size_t count,loff_t *f_pos)
{
    struct ps_core_s *ps_core_d = NULL;
    struct sk_buff *skb = NULL;
    uint16 count1;
    int64 timeout;

    PS_PRINT_FUNCTION_NAME;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == buf)||(NULL == filp)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    if (0 == ps_core_d->bfgx_info[BFGX_FM].rx_queue.qlen)
    {   /* if don,t data, and wait timeout function */
        if (filp->f_flags & O_NONBLOCK)
        {   /* if O_NONBLOCK read and return */
            return -EAGAIN;
        }
        /* timeout function;when have data,can interrupt */
        timeout = wait_event_interruptible_timeout(ps_core_d->bfgx_info[BFGX_FM].rx_wait,
            (ps_core_d->bfgx_info[BFGX_FM].rx_queue.qlen > 0), msecs_to_jiffies(ps_core_d->fm_read_delay));
        if (!timeout)
        {
            PS_PRINT_DBG("fm read time out!\n");
            return -ETIMEDOUT;
        }
    }

    if (NULL == (skb = ps_skb_dequeue_etc(ps_core_d, RX_FM_QUEUE)))
    {
        PS_PRINT_WARNING("fm read no data!\n");
        return -ETIMEDOUT;
    }

    count1 = min_t(size_t, skb->len, count);
    if (copy_to_user(buf, skb->data, count1))
    {
        PS_PRINT_ERR("copy_to_user is err!\n");
        ps_restore_skbqueue_etc(ps_core_d, skb, RX_FM_QUEUE);
        return -EFAULT;
    }

    skb_pull(skb, count1);

    /* if skb->len == 0: curr skb read is over */
    if (0 == skb->len)
    {   /* curr skb data have read to user */
        kfree_skb(skb);
    }
    else
    {   /* if don,t read over; restore to skb queue */
        ps_restore_skbqueue_etc(ps_core_d, skb, RX_FM_QUEUE);
    }

    return count1;
}

/**
 * Prototype    : hw_fm_write
 * Description  : functions called from above fm hal,write count data to buf
 * input        : file handle, buf, count
 * output       : return size --> actual write byte size
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC ssize_t hw_fm_write(struct file *filp, const int8 __user *buf,
                                size_t count,loff_t *f_pos)
{
    struct ps_core_s *ps_core_d = NULL;
    int32 ret = 0;

    PS_PRINT_FUNCTION_NAME;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == buf)||(NULL == filp)||(NULL == ps_core_d->ps_pm)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    /* if count is too large;and don,t tx */
    if (count > (FM_TX_MAX_FRAME - sizeof(struct ps_packet_head) - sizeof(struct ps_packet_end)))
    {
        PS_PRINT_ERR("err:fm packet is too large!\n");
        return -EINVAL;
    }

    /* if low queue num > MAX_NUM and don't write */
    if (ps_core_d->tx_low_seq.qlen > TX_LOW_QUE_MAX_NUM)
    {
        return 0;
    }

    ret = prepare_to_visit_node_etc(ps_core_d);
    if (ret < 0)
    {
        PS_PRINT_ERR("prepare work fail, bring to reset work\n");
        plat_exception_handler_etc(SUBSYS_BFGX, THREAD_FM, BFGX_WAKEUP_FAIL);
        return ret;
    }

    /* modify expire time of uart idle timer */
    mod_timer(&ps_core_d->ps_pm->pm_priv_data->bfg_timer, jiffies + (BT_SLEEP_TIME * HZ/1000));
    ps_core_d->ps_pm->pm_priv_data->bfg_timer_mod_cnt++;

    /* to divide up packet function and tx to tty work */
    if (ps_tx_fmbuf_etc(ps_core_d, buf, count) < 0)
    {
        PS_PRINT_ERR("hw_fm_write is err\n");
        post_to_visit_node_etc(ps_core_d);
        return -EFAULT;
    }
    ps_core_d->tx_pkt_num[BFGX_FM]++;

    post_to_visit_node_etc(ps_core_d);

    PS_PRINT_DBG("FM data write end\n");

    return count;
}

/**
 * Prototype    : hw_fm_ioctl
 * Description  : called by hw func from hal when open power gpio or close power gpio
 * input        : file handle, cmd, arg
 * output       : no
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC int64 hw_fm_ioctl(struct file *file, uint32 cmd, uint64 arg)
{
    struct ps_core_s *ps_core_d = NULL;

    PS_PRINT_FUNCTION_NAME;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == file)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    if (FM_SET_READ_TIME == cmd)
    {
        if (arg < FM_MAX_READ_TIME)
        {   /* set timeout for fm read function */
            ps_core_d->fm_read_delay = arg;
        }
        else
        {
            PS_PRINT_ERR("arg is too large!\n");
            return -EINVAL;
        }
    }

    return 0;
}

/**
 * Prototype    : hw_fm_release
 * Description  : called by fm func from hal when close fm inode
 * input        : have opened file handle
 * output       : return 0 --> close is ok
 *                return !0--> close is false
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC int32 hw_fm_release(struct inode *inode, struct file *filp)
{
    int32  ret = 0;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    if (NULL == pm_data)
    {
       PS_PRINT_ERR("pm_data is NULL!\n");
        return -EINVAL;
    }

    if (unlikely((NULL == inode)||(NULL == filp)))
    {
        PS_PRINT_ERR("%s param is error", __func__);
        return -EINVAL;
    }

    mutex_lock(&pm_data->host_mutex);

    ret = hw_bfgx_close(BFGX_FM);

    mutex_unlock(&pm_data->host_mutex);

    return ret;
}

/*device bfgx pm debug switch on/off*/
void plat_pm_debug_switch(void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    struct ps_core_s *ps_core_d = NULL;
    int32 ret;

    if (unlikely(NULL == pm_data))
    {
        PS_PRINT_ERR("pm_data is null\n");
        return;
    }

    PS_PRINT_INFO("%s", __func__);

    ps_get_core_reference_etc(&ps_core_d);

    ret = prepare_to_visit_node_etc(ps_core_d);
    if (ret < 0)
    {
        PS_PRINT_ERR("prepare work FAIL\n");
        return ;
    }

    ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, PL_PM_DEBUG);

    post_to_visit_node_etc(ps_core_d);

    return;

}

/**
 * Prototype    : hw_gnss_open
 * Description  : functions called from above gnss hal,when open gnss file
 * input        : "/dev/hwgnss"
 * output       : return 0 --> open is ok
 *                return !0--> open is false
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC int32 hw_gnss_open(struct inode *inode, struct file *filp)
{
    int32  ret = BFGX_POWER_SUCCESS;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    if (NULL == pm_data)
    {
       PS_PRINT_ERR("pm_data is NULL!\n");
        return -EINVAL;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == inode)||(NULL == filp)))
    {
        PS_PRINT_ERR("%s param is error", __func__);
        return -EINVAL;
    }

    mutex_lock(&pm_data->host_mutex);

    atomic_set(&pm_data->gnss_sleep_flag, GNSS_NOT_AGREE_SLEEP);

    ret = hw_bfgx_open(BFGX_GNSS);

    ps_core_d->gnss_read_delay = GNSS_READ_DEFAULT_TIME;

    if (BFGX_POWER_SUCCESS != ret)
    {
        atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);
    }

    mutex_unlock(&pm_data->host_mutex);

    return ret;
}


STATIC uint32 hw_gnss_poll(struct file *filp, poll_table *wait)
{
    struct ps_core_s *ps_core_d = NULL;
    uint32 mask = 0;

    PS_PRINT_DBG("%s\n", __func__);

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == filp)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    /* push curr wait event to wait queue */
    poll_wait(filp, &ps_core_d->bfgx_info[BFGX_GNSS].rx_wait, wait);

    PS_PRINT_DBG("%s, recive gnss data\n", __func__);

    if (ps_core_d->bfgx_info[BFGX_GNSS].rx_queue.qlen)
    {   /* have data to read */
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}

/**
 * Prototype    : hw_gnss_read
 * Description  : functions called from above gnss hal,read count data to buf
 * input        : file handle, buf, count
 * output       : return size --> actual read byte size
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC ssize_t hw_gnss_read(struct file *filp, int8 __user *buf, size_t count, loff_t *f_pos)
{
    struct ps_core_s *ps_core_d = NULL;
    struct sk_buff *skb = NULL;
    struct sk_buff_head read_queue;
    int32 count1;
    uint8 seperate_tag = GNSS_SEPER_TAG_INIT;
    int32 copy_cnt = 0;
    uint32 ret;

    PS_PRINT_FUNCTION_NAME;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == buf)||(NULL == filp)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    skb_queue_head_init(&read_queue);

    spin_lock(&ps_core_d->gnss_rx_lock);
    do
    {
        if (NULL == (skb = ps_skb_dequeue_etc(ps_core_d, RX_GNSS_QUEUE)))
        {
            spin_unlock(&ps_core_d->gnss_rx_lock);
            if (0 != read_queue.qlen)
            {
                //没有找到last包，skb queue就空了
                PS_PRINT_ERR("skb dequeue error, qlen=%x!\n", read_queue.qlen);
                goto skb_dequeue_error;
            }
            else
            {
                PS_PRINT_INFO("gnss read no data!\n");
                return 0;
            }
        }

        seperate_tag = skb->data[skb->len -1];
        switch (seperate_tag)
        {
            case GNSS_SEPER_TAG_INIT:
            case GNSS_SEPER_TAG_LAST:
                break;
            default:
                PS_PRINT_ERR("seperate_tag=%x not support\n", seperate_tag);
                seperate_tag = GNSS_SEPER_TAG_LAST;
                break;
        }

        skb_queue_tail(&read_queue, skb);
    }while(GNSS_SEPER_TAG_INIT == seperate_tag);
    spin_unlock(&ps_core_d->gnss_rx_lock);

    copy_cnt = 0;
    do
    {
        skb = skb_dequeue(&read_queue);
        if (NULL == skb)
        {
            PS_PRINT_ERR("copy dequeue error, copy_cnt=%x\n", copy_cnt);
            goto skb_dequeue_error;
        }

        if (1 >= skb->len)
        {
            PS_PRINT_ERR("skb len error,skb->len=%x,copy_cnt=%x,count=%x\n", skb->len, copy_cnt, (uint32)count);
            goto copy_error;
        }

        count1 = skb->len -1;
        if (count1 + copy_cnt > count)
        {
            PS_PRINT_ERR("copy total len error,skb->len=%x,tag=%x,copy_cnt=%x,read_cnt=%x\n", \
                                                 skb->len, skb->data[skb->len -1], copy_cnt, (uint32)count);
            goto copy_error;
        }

        ret = copy_to_user(buf+copy_cnt, skb->data, count1);
        if (0 != ret)
        {
            PS_PRINT_ERR("copy_to_user err,ret=%x,dest=%p,src=%p,tag:%x,count1=%x,copy_cnt=%x,read_cnt=%x\n", \
                                  ret,buf+copy_cnt,skb->data,skb->data[skb->len -1],count1,copy_cnt,(uint32)count);
            goto copy_error;
        }

        copy_cnt += count1;
        kfree_skb(skb);
    }while(0 != read_queue.qlen);

    return copy_cnt;

copy_error:
    kfree_skb(skb);
skb_dequeue_error:
    while (NULL != (skb = skb_dequeue(&read_queue)))
    {
        PS_PRINT_ERR("free skb: len=%x, tag=%x\n", skb->len, skb->data[skb->len -1]);
        kfree_skb(skb);
    }

    return -EFAULT;
}

/**
 * Prototype    : hw_gnss_write
 * Description  : functions called from above gnss hal,write count data to buf
 * input        : file handle, buf, count
 * output       : return size --> actual write byte size
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC ssize_t hw_gnss_write(struct file *filp, const int8 __user *buf,
                                    size_t count, loff_t *f_pos)
{
    struct ps_core_s *ps_core_d = NULL;
    int32 ret = 0;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    PS_PRINT_FUNCTION_NAME;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == buf)||(NULL == filp)||(NULL == ps_core_d->ps_pm) ||(NULL ==  pm_data)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    if (count > GNSS_TX_MAX_FRAME)
    {
        PS_PRINT_ERR("err:gnss packet is too large!\n");
        return -EINVAL;
    }

    /* if low queue num > MAX_NUM and don't write */
    if (ps_core_d->tx_low_seq.qlen > TX_LOW_QUE_MAX_NUM)
    {
        return 0;
    }

    atomic_set(&pm_data->gnss_sleep_flag, GNSS_NOT_AGREE_SLEEP);
    ret = prepare_to_visit_node_etc(ps_core_d);
    if (ret < 0)
    {
        atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);
        PS_PRINT_ERR("prepare work fail, bring to reset work\n");
        plat_exception_handler_etc(SUBSYS_BFGX, THREAD_GNSS, BFGX_WAKEUP_FAIL);
        return ret;
    }

    /* to divide up packet function and tx to tty work */
    if (ps_tx_gnssbuf_etc(ps_core_d, buf, count) < 0)
    {
        PS_PRINT_ERR("hw_gnss_write is err\n");
        atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);
        count = -EFAULT;
    }

    ps_core_d->tx_pkt_num[BFGX_GNSS]++;

    post_to_visit_node_etc(ps_core_d);

    return count;
}

/**
 * Prototype    : hw_gnss_ioctl
 * Description  : called by gnss func from hal when open power gpio or close power gpio
 * input        : file handle, cmd, arg
 * output       : no
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC int64 hw_gnss_ioctl(struct file *file, uint32 cmd, uint64 arg)
{
    struct ps_core_s *ps_core_d = NULL;

    PS_PRINT_FUNCTION_NAME;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == file)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    if (GNSS_SET_READ_TIME == cmd)
    {
        if (arg < GNSS_MAX_READ_TIME)
        {   /* set timeout for gnss read function */
            ps_core_d->gnss_read_delay = arg;
        }
        else
        {
            PS_PRINT_ERR("arg is too large!\n");
            return -EINVAL;
        }
    }

    return 0;
}

/**
 * Prototype    : hw_gnss_release
 * Description  : called by gnss func from hal when close gnss inode
 * input        : have opened file handle
 * output       : return 0 --> close is ok
 *                return !0--> close is false
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC int32 hw_gnss_release(struct inode *inode, struct file *filp)
{
    int32  ret = 0;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    if (NULL == pm_data)
    {
       PS_PRINT_ERR("pm_data is NULL!\n");
        return -EINVAL;
    }

    if (unlikely((NULL == inode)||(NULL == filp)))
    {
        PS_PRINT_ERR("%s param is error", __func__);
        return -EINVAL;
    }

    mutex_lock(&pm_data->host_mutex);

    ret = hw_bfgx_close(BFGX_GNSS);

    atomic_set(&pm_data->gnss_sleep_flag, GNSS_AGREE_SLEEP);

    mutex_unlock(&pm_data->host_mutex);

    return ret;
}
#ifndef HI110X_HAL_MEMDUMP_ENABLE

void plat_exception_dump_file_rotate_init_etc(void)
{
    init_waitqueue_head(&dump_cmd_queue_etc.dump_type_wait);
    skb_queue_head_init(&dump_cmd_queue_etc.dump_type_queue);
    atomic_set(&dump_cmd_queue_etc.rotate_finish_state, ROTATE_FINISH);
    PS_PRINT_INFO("plat exception dump file rotate init success\n");
}


void plat_rotate_finish_set_etc(void)
{
    atomic_set(&dump_cmd_queue_etc.rotate_finish_state, ROTATE_FINISH);
}


void plat_wait_last_rotate_finish_etc(void)
{
    uint8 retry = 0;

#define RETRY_TIME 3

    while (ROTATE_FINISH != atomic_read(&dump_cmd_queue_etc.rotate_finish_state))
    {
        /*maximum app rotate time is 0.1s*/
        oal_udelay(100);
        if ((++retry) >= RETRY_TIME)
        {
            PS_PRINT_WARNING("retry wait last rotate fail:retry =%d", retry);
            break;
        }
    }

    atomic_set(&dump_cmd_queue_etc.rotate_finish_state, ROTATE_NOT_FINISH);
}

int32 plat_send_rotate_cmd_2_app_etc(uint32 which_dump)
{
    struct sk_buff  *skb =NULL;

    if (CMD_DUMP_BUFF <= which_dump)
    {
        PS_PRINT_WARNING("which dump:%d error\n", which_dump);
        return -EINVAL;
    }
    if (skb_queue_len(&dump_cmd_queue_etc.dump_type_queue) > MEMDUMP_ROTATE_QUEUE_MAX_LEN)
    {
        PS_PRINT_WARNING("too many dump type in queue,dispose type:%d", which_dump);
        return -EINVAL;
    }

    skb = alloc_skb(sizeof(which_dump), GFP_KERNEL);
    if( NULL == skb)
    {
        PS_PRINT_ERR("alloc errno skbuff failed! len=%d, errno=%x\n", (int32)sizeof(which_dump), which_dump);
        return -EINVAL;
    }
    skb_put(skb, sizeof(which_dump));
    *(uint32*)skb->data = which_dump;
    skb_queue_tail(&dump_cmd_queue_etc.dump_type_queue, skb);
    PS_PRINT_INFO("save rotate cmd [%d] in queue\n", which_dump);

    wake_up_interruptible(&dump_cmd_queue_etc.dump_type_wait);

    return 0;
}


int32 plat_dump_rotate_cmd_read_etc(uint64 arg)
{
    uint32 __user  *puser = (uint32 __user *)arg;

    struct sk_buff  *skb =NULL;

    if (!access_ok(VERIFY_WRITE, puser, (int32)sizeof(uint32)))
    {
        PS_PRINT_ERR("address can not write\n");
        return -EINVAL;
    }

    if (wait_event_interruptible(dump_cmd_queue_etc.dump_type_wait,  (skb_queue_len(&dump_cmd_queue_etc.dump_type_queue)) > 0))
    {
        return -EINVAL;
    }

    skb = skb_dequeue(&dump_cmd_queue_etc.dump_type_queue);
    if (NULL == skb)
    {
        PS_PRINT_WARNING("skb is NULL\n");
        return -EINVAL;
    }

    if (copy_to_user(puser, skb->data, sizeof(uint32)))
    {
        PS_PRINT_WARNING("copy_to_user err!restore it, len=%d\n", (int32)sizeof(uint32));
        skb_queue_head(&dump_cmd_queue_etc.dump_type_queue, skb);
        return -EINVAL;
    }

    PS_PRINT_INFO("read rotate cmd [%d] from queue\n", *(uint32*)skb->data);

    skb_pull(skb, skb->len);
    kfree_skb(skb);

    return 0;
}



STATIC int64 hw_debug_ioctl(struct file *file, uint32 cmd, uint64 arg)
{

    if (NULL == file)
    {
        PS_PRINT_ERR("file is null\n");
        return -EINVAL;
    }

    switch (cmd)
    {
        case PLAT_DFR_CFG_CMD:
            plat_dfr_cfg_set_etc(arg);
            break;
        case PLAT_BEATTIMER_TIMEOUT_RESET_CFG_CMD:
            plat_beatTimer_timeOut_reset_cfg_set_etc(arg);
            break;
        case PLAT_BFGX_CALI_CMD:
            bfgx_cali_data_init();
            break;
        case PLAT_DUMP_FILE_READ_CMD:
            plat_dump_rotate_cmd_read_etc(arg);
            break;
        case PLAT_DUMP_ROTATE_FINISH_CMD:
            plat_rotate_finish_set_etc();
            break;
        default:
            PS_PRINT_WARNING("hw_debug_ioctl cmd = %d not find\n", cmd);
            return -EINVAL;
    }

    return 0;
}
#else
int32 plat_excp_dump_rotate_cmd_read_etc(uint64 arg, memdump_info_t* memdump_info)
{
    uint32 __user  *puser = (uint32 __user *)arg;

    struct sk_buff  *skb =NULL;

    if (!access_ok(VERIFY_WRITE, puser, (int32)sizeof(uint32)))
    {
        PS_PRINT_ERR("address can not write\n");
        return -EINVAL;
    }

    if (wait_event_interruptible(memdump_info->dump_type_wait,  (skb_queue_len(&memdump_info->dump_type_queue)) > 0))
    {
        return -EINVAL;
    }

    skb = skb_dequeue(&memdump_info->dump_type_queue);
    if (NULL == skb)
    {
        PS_PRINT_WARNING("skb is NULL\n");
        return -EINVAL;
    }

    if (copy_to_user(puser, skb->data, sizeof(uint32)))
    {
        PS_PRINT_WARNING("copy_to_user err!restore it, len=%d,arg=%ld\n", (int32)sizeof(uint32),arg);
        skb_queue_head(&memdump_info->dump_type_queue, skb);
        return -EINVAL;
    }

    PS_PRINT_INFO("read rotate cmd [%d] from queue\n", *(uint32*)skb->data);

    skb_pull(skb, skb->len);
    kfree_skb(skb);

    return 0;
}
int32 plat_bfgx_dump_rotate_cmd_read_etc(uint64 arg)
{
    return plat_excp_dump_rotate_cmd_read_etc(arg, &bcpu_memdump_cfg_etc);
}
int32 plat_wifi_dump_rotate_cmd_read_etc(uint64 arg)
{
    return plat_excp_dump_rotate_cmd_read_etc(arg, &wcpu_memdump_cfg_etc);
}



STATIC int64 hw_debug_ioctl(struct file *file, uint32 cmd, uint64 arg)
{

    if (NULL == file)
    {
        PS_PRINT_ERR("file is null\n");
        return -EINVAL;
    }

    switch (cmd)
    {
        case PLAT_DFR_CFG_CMD:
            plat_dfr_cfg_set_etc(arg);
            break;
        case PLAT_BEATTIMER_TIMEOUT_RESET_CFG_CMD:
            plat_beatTimer_timeOut_reset_cfg_set_etc(arg);
            break;
        case PLAT_BFGX_CALI_CMD:
            bfgx_cali_data_init();
            break;
        default:
            PS_PRINT_WARNING("hw_debug_ioctl cmd = %d not find\n", cmd);
            return -EINVAL;
    }

    return 0;
}
#endif
int32 arm_timeout_submit(enum BFGX_THREAD_ENUM subs)
{
#define DFR_SUBMIT_LIMIT_TIME      (300) /*second*/
    static unsigned long long dfr_submit_last_time = 0;
    unsigned long long dfr_submit_current_time = 0;
    unsigned long long dfr_submit_interval_time = 0;
    struct timespec dfr_submit_time;

    if (subs >= BFGX_THREAD_BOTTOM)
    {
        return -EINVAL;
    }

    PS_PRINT_INFO("[subs id:%d]arm timeout trigger", subs);

    dfr_submit_time          = current_kernel_time();
    dfr_submit_current_time  = dfr_submit_time.tv_sec;
    dfr_submit_interval_time = dfr_submit_current_time - dfr_submit_last_time;

    /*5分钟内最多触发一次*/
    if((dfr_submit_interval_time > DFR_SUBMIT_LIMIT_TIME) || (0 == dfr_submit_last_time))
    {
        dfr_submit_last_time = dfr_submit_current_time;
        plat_exception_handler_etc(SUBSYS_BFGX, subs, BFGX_ARP_TIMEOUT);
        return 0;
    }
    else
    {
        PS_PRINT_ERR("[subs id:%d]arm timeout cnt max than limit", subs);
        return -EAGAIN;
    }
}
#ifdef HI110X_HAL_MEMDUMP_ENABLE
STATIC int32 hw_excp_read(struct file *filp, int8 __user *buf,
                                size_t count,loff_t *f_pos, memdump_info_t *memdump_t)
{
    struct sk_buff *skb = NULL;
    uint16 count1;

   // PS_PRINT_WARNING("hw_excp_read\n");

    if (unlikely((NULL == buf)||(NULL == filp)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }
    if (NULL == (skb = skb_dequeue(&memdump_t->quenue)))
    {
        //PS_PRINT_WARNING("hw_excp_read read skb queue is null!\n");
        return 0;
    }

    /* read min value from skb->len or count */
    count1 = min_t(size_t, skb->len, count);
    if (copy_to_user(buf, skb->data, count1))
    {
        PS_PRINT_ERR("copy_to_user is err!\n");
        skb_queue_head(&memdump_t->quenue, skb);
        return -EFAULT;
    }

    /* have read count1 byte */
    skb_pull(skb, count1);

    /* if skb->len = 0: read is over */
    if (0 == skb->len)
    {   /* curr skb data have read to user */
        kfree_skb(skb);
    }
    else
    {   /* if don,t read over; restore to skb queue */
        skb_queue_head(&memdump_t->quenue, skb);
    }
    return count1;

}

STATIC ssize_t hw_bfgexcp_read(struct file *filp, int8 __user *buf,
                                size_t count,loff_t *f_pos)
{
    return hw_excp_read(filp, buf, count, f_pos, &bcpu_memdump_cfg_etc);
}
STATIC int64 hw_bfgexcp_ioctl(struct file *file, uint32 cmd, uint64 arg)
{
    int32 ret = 0;
    if (NULL == file)
    {
        PS_PRINT_ERR("file is null\n");
        return -EINVAL;
    }
    switch (cmd)
    {
        case PLAT_BFGX_DUMP_FILE_READ_CMD:
            ret =  plat_bfgx_dump_rotate_cmd_read_etc(arg);
            break;
        case DFR_HAL_GNSS_CFG_CMD:
            return arm_timeout_submit(THREAD_GNSS);
        case DFR_HAL_BT_CFG_CMD:
            return arm_timeout_submit(THREAD_BT);
        case DFR_HAL_FM_CFG_CMD:
            return arm_timeout_submit(THREAD_FM);
        default:
            PS_PRINT_WARNING("hw_debug_ioctl cmd = %d not find\n", cmd);
            return -EINVAL;
    }

    return ret;
}
STATIC int64 hw_wifiexcp_ioctl(struct file *file, uint32 cmd, uint64 arg)
{
    int32 ret = 0;

    if (NULL == file)
    {
        PS_PRINT_ERR("file is null\n");
        return -EINVAL;
    }
    switch (cmd)
    {
        case PLAT_WIFI_DUMP_FILE_READ_CMD:
            ret =  plat_wifi_dump_rotate_cmd_read_etc(arg);
            break;
        default:
            PS_PRINT_WARNING("hw_debug_ioctl cmd = %d not find\n", cmd);
            return -EINVAL;
    }

    return ret;
}

STATIC ssize_t hw_wifiexcp_read(struct file *filp, int8 __user *buf,
                                size_t count,loff_t *f_pos)
{
    return hw_excp_read(filp, buf, count, f_pos, &wcpu_memdump_cfg_etc);
}
#endif
/**********************************************************************/
/**
 * Prototype    : hw_debug_open
 * Description  : functions called from above oam hal,when open debug file
 *                  input : "/dev/hwbfgdbg"
 * input        : return 0 --> open is ok
 * output       : return !0--> open is false
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC int32 hw_debug_open(struct inode *inode, struct file *filp)
{
    struct ps_core_s *ps_core_d = NULL;

    PS_PRINT_INFO("%s", __func__);

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == inode)||(NULL == filp)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

	g_debug_cnt++;
    PS_PRINT_INFO("%s g_debug_cnt=%d\n", __func__, g_debug_cnt);
    atomic_set(&ps_core_d->dbg_func_has_open, 1);

    ps_core_d->dbg_read_delay = DBG_READ_DEFAULT_TIME;

    return 0;
}

/**
 * Prototype    : hw_debug_read
 * Description  : functions called from above oam hal,read count data to buf
 * input        : file handle, buf, count
 * output       : return size --> actual read byte size
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC ssize_t hw_debug_read(struct file *filp, int8 __user *buf,
                                size_t count,loff_t *f_pos)
{
    struct ps_core_s *ps_core_d = NULL;
    struct sk_buff *skb = NULL;
    uint16 count1 = 0;
    int64 timeout;

    PS_PRINT_FUNCTION_NAME;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == buf)||(NULL == filp)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    if (0 == ps_core_d->rx_dbg_seq.qlen)
    {   /* if no data, and wait timeout function */
        if (filp->f_flags & O_NONBLOCK)
        {   /* if O_NONBLOCK read and return */
            return -EAGAIN;
        }

        /* timeout function;when have data,can interrupt */
        timeout = wait_event_interruptible_timeout(ps_core_d->rx_dbg_wait,
            (ps_core_d->rx_dbg_seq.qlen > 0), msecs_to_jiffies(ps_core_d->dbg_read_delay));
        if (!timeout)
        {
            PS_PRINT_DBG("debug read time out!\n");
            return -ETIMEDOUT;
        }
    }

    /* pull skb data from skb queue */
    if (NULL == (skb = ps_skb_dequeue_etc(ps_core_d, RX_DBG_QUEUE)))
    {
        PS_PRINT_DBG("dbg read no data!\n");
        return -ETIMEDOUT;
    }
    /* read min value from skb->len or count */
    count1 = min_t(size_t, skb->len, count);
    if (copy_to_user(buf, skb->data, count1))
    {
        PS_PRINT_ERR("debug copy_to_user is err!\n");
        ps_restore_skbqueue_etc(ps_core_d, skb, RX_DBG_QUEUE);
        return -EFAULT;
    }

    skb_pull(skb, count1);

    /* if skb->len == 0: curr skb read is over */
    if (0 == skb->len)
    {   /* curr skb data have read to user */
        kfree_skb(skb);
    }
    else
    {   /* if don,t read over; restore to skb queue */
        ps_restore_skbqueue_etc(ps_core_d, skb, RX_DBG_QUEUE);
    }

    return count1;
}

/**
 * Prototype    : hw_debug_write
 * Description  : functions called from above oam hal,write count data to buf
 * input        : file handle, buf, count
 * output       : return size --> actual write byte size
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
#ifdef PLATFORM_DEBUG_ENABLE
STATIC ssize_t hw_debug_write(struct file *filp, const int8 __user *buf,
                                size_t count,loff_t *f_pos)
{
    struct ps_core_s *ps_core_d = NULL;
    struct sk_buff *skb;
    uint16 total_len;
    int32 ret = 0;

    PS_PRINT_FUNCTION_NAME;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == buf)||(NULL == filp)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    if (false == ps_core_d->tty_have_open)
    {
        PS_PRINT_ERR("err: uart not opened!\n");
        return -EFAULT;
    }

    if (count > (DBG_TX_MAX_FRAME - sizeof(struct ps_packet_head) - sizeof(struct ps_packet_end)))
    {
        PS_PRINT_ERR("err: dbg packet is too large!\n");
        return -EINVAL;
    }

    /* if low queue num > MAX_NUM and don't write */
    if (ps_core_d->tx_low_seq.qlen > TX_LOW_QUE_MAX_NUM)
    {
        return 0;
    }

    if (false == ps_chk_bfg_active_etc(ps_core_d))
    {
        PS_PRINT_ERR("bfg is closed, /dev/hwdebug cant't write!!!\n");
        return -EINVAL;
    }

    ret = prepare_to_visit_node_etc(ps_core_d);
    if (ret < 0)
    {
        PS_PRINT_ERR("prepare work FAIL\n");
        return ret;
    }
    /* modify expire time of uart idle timer */
    mod_timer(&ps_core_d->ps_pm->pm_priv_data->bfg_timer, jiffies + (BT_SLEEP_TIME * HZ/1000));
    ps_core_d->ps_pm->pm_priv_data->bfg_timer_mod_cnt++;

    total_len = count + sizeof(struct ps_packet_head) + sizeof(struct ps_packet_end);

    skb  = ps_alloc_skb_etc(total_len);
    if (NULL == skb)
    {
        PS_PRINT_ERR("ps alloc skb mem fail\n");
        post_to_visit_node_etc(ps_core_d);
        return -EFAULT;
    }

    if (copy_from_user(&skb->data[sizeof(struct ps_packet_head)], buf, count))
    {
        PS_PRINT_ERR("copy_from_user from dbg is err\n");
        kfree_skb(skb);
        post_to_visit_node_etc(ps_core_d);
        return -EFAULT;
    }

    ps_add_packet_head_etc(skb->data, OML_MSG, total_len);
    ps_skb_enqueue_etc(ps_core_d, skb, TX_LOW_QUEUE);
    queue_work(ps_core_d->ps_tx_workqueue, &ps_core_d->tx_skb_work);

    post_to_visit_node_etc(ps_core_d);

    return count;
}
#endif
/**
 * Prototype    : hw_debug_release
 * Description  : called by oam func from hal when close debug inode
 * input        : have opened file handle
 * output       : return 0 --> close is ok
 *                return !0--> close is false
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC int32 hw_debug_release(struct inode *inode, struct file *filp)
{
    struct ps_core_s *ps_core_d = NULL;

    PS_PRINT_INFO("%s", __func__);

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == inode)||(NULL == filp)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

	g_debug_cnt--;
    PS_PRINT_INFO("%s g_debug_cnt=%d", __func__, g_debug_cnt);
	if (0 == g_debug_cnt)
	{
		/* wake up bt dbg wait queue */
		wake_up_interruptible(&ps_core_d->rx_dbg_wait);
		atomic_set(&ps_core_d->dbg_func_has_open, 0);

		/* kfree have rx dbg skb */
		ps_kfree_skb_etc(ps_core_d, RX_DBG_QUEUE);
	}

    return 0;
}
#ifndef HI110X_HAL_MEMDUMP_ENABLE
STATIC int32 hw_excp_open(struct inode *inode, struct file *filp)
{
    PS_PRINT_INFO("%s", __func__);
    return 0;
}
STATIC int32 hw_excp_release(struct inode *inode, struct file *filp)
{
    PS_PRINT_INFO("%s", __func__);
    return 0;
}
STATIC int64 hw_excp_ioctl(struct file *file, uint32 cmd, uint64 arg)
{
    struct st_exception_info *pst_exception_data = NULL;
    if (NULL == file)
    {
        PS_PRINT_ERR("file is null\n");
        return -EINVAL;
    }
    PS_PRINT_INFO("%s", __func__);

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EINVAL;
    }

    switch (cmd)
    {
        case DFR_HAL_GNSS_CFG_CMD:
            return arm_timeout_submit(THREAD_GNSS);
            break;
        case DFR_HAL_BT_CFG_CMD:
            return arm_timeout_submit(THREAD_BT);
            break;
        case DFR_HAL_FM_CFG_CMD:
            return arm_timeout_submit(THREAD_FM);
            break;
        default:
            PS_PRINT_WARNING("hw_excp_ioctl cmd = %d not find\n", cmd);
            return -EINVAL;
    }

    return 0;
}
#endif
//uart_loop_cfg g_st_uart_loop_test_cfg_etc = {256, 100, 0, 0};
uart_loop_cfg g_st_uart_loop_test_cfg_etc = {256, 60000, 0, 0, 0};
uart_loop_test_struct *g_pst_uart_loop_test_info_etc = NULL;
#ifdef BFGX_UART_DOWNLOAD_SUPPORT
uart_download_test_st g_st_uart_download_test;
extern RINGBUF_STRU                    g_stringbuf;
extern uint8                          *g_pucDataBuf_t;
extern int32                           g_usemalloc ;
extern PATCH_GLOBALS_STUR              g_st_global[ENUM_INFO_TOTAL];
extern int32 ps_recv_patch(void *disc_data, const uint8 *data, int32 count);
extern uint8 powerpin_state;
#define DATA_SRC_LEN   256
int32 patch_download_buffer_pre(int32 type)
{
    uint32                  ul_alloc_len = READ_DATA_BUF_LEN;
    if (ENUM_INFO_UART == type)
    {
        g_stringbuf.pbufstart = kmalloc(ul_alloc_len, GFP_KERNEL);
        if (NULL == g_stringbuf.pbufstart)
        {
            ul_alloc_len = READ_DATA_REALLOC_BUF_LEN;
            g_stringbuf.pbufstart = kmalloc(ul_alloc_len, GFP_KERNEL);
            if (NULL == g_stringbuf.pbufstart)
            {
                g_usemalloc = 0;
                PS_PRINT_ERR("ringbuf KMALLOC SIZE(%d) failed.\n", ul_alloc_len);
                g_stringbuf.pbufstart = g_st_global[type].auc_Recvbuf1;
                g_stringbuf.pbufend = RECV_BUF_LEN + g_stringbuf.pbufstart;

                return -EFAIL;
            }

            powerpin_state &= 0xFF - BFG_PINDISABLE;
            PS_PRINT_INFO("ringbuf kmalloc size(%d) suc.\n", ul_alloc_len);
        }

        g_stringbuf.pbufend = ul_alloc_len + g_stringbuf.pbufstart;
        g_usemalloc = 1;

        g_stringbuf.phead = g_stringbuf.pbufstart;
        g_stringbuf.ptail = g_stringbuf.pbufstart;
    }
    return 0;
}
/*****************************************************************************
 Prototype    : patch_download_patch
 Description  : download patch
 Input        : int32 type
 Output       :

 Return Value : int32
 Calls        :
 Called By    :

 History         :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_download_patch_test(int32 type)
{
    int32                   l_ret = 0;
    mm_segment_t fs = {0};
    OS_KERNEL_FILE_STRU *fp = {0};
    int8 send_cmd_para[100]={0};
    int8 send_cmd_addr[100]="0x00020000";
    int8 filename[100] = "/system/vendor/firmware/test_uart_cfg";
    int8 write_data[DATA_SRC_LEN];
    int32 write_total_len=(g_st_uart_download_test.file_len)/DATA_SRC_LEN;
    int32 write_count = 0;
    struct timeval stime,etime;
    for (write_count=0;write_count < DATA_SRC_LEN;write_count++)
    {
        write_data[write_count] = write_count;
    }
    //file prepare
    fp = filp_open(filename, O_RDWR | O_CREAT, 0664);
    if (OAL_IS_ERR_OR_NULL(fp))
    {
        PS_PRINT_ERR("create file error,fp = 0x%p\n", fp);
        return -1;
    }
    fs = get_fs();
    set_fs(KERNEL_DS);
    for (write_count = 0; write_count < write_total_len; write_count++)
    {
        vfs_write(fp, write_data, DATA_SRC_LEN, &fp->f_pos);
    }
    set_fs(fs);
    filp_close(fp, NULL);
    PS_PRINT_INFO("#@file:%s prepare succ", filename);

    //send file and collect data
    snprintf(send_cmd_para, 100, "%s,%s,%s", "1",send_cmd_addr,filename);
    PS_PRINT_INFO("send_cmd_para:[%s]",send_cmd_para);

    PS_PRINT_INFO("type:%d\n", type);

    do_gettimeofday(&stime);
    l_ret =  patch_file_type("FILES", send_cmd_para, ENUM_INFO_UART);
    do_gettimeofday(&etime);
    //ms
    g_st_uart_download_test.used_time = (etime.tv_sec - stime.tv_sec)*1000 + (etime.tv_usec - stime.tv_usec)/1000;
    PS_PRINT_DBG("patch_file_type:%d", l_ret);
    if (0 != l_ret)
    {
        //fail
        g_st_uart_download_test.send_status = -1;
    }
    else
    {
         //succ
        g_st_uart_download_test.send_status = 0;
    }
    if (0 > l_ret)
    {
        if ((ENUM_INFO_UART == type) && (1 == g_usemalloc))
        {
            kfree(g_stringbuf.pbufstart);
            g_stringbuf.pbufstart   = NULL;
            g_stringbuf.pbufend     = NULL;
            g_stringbuf.phead       = NULL;
            g_stringbuf.ptail       = NULL;
            g_usemalloc = 0;
        }
        return l_ret;
    }

    if ((ENUM_INFO_UART == type) && (1 == g_usemalloc))
    {
        kfree(g_stringbuf.pbufstart);
        g_usemalloc = 0;
    }
    g_stringbuf.pbufstart   = NULL;
    g_stringbuf.pbufend     = NULL;
    g_stringbuf.phead       = NULL;
    g_stringbuf.ptail       = NULL;
    return SUCC;
}
int32 uart_download_test(uint8* baud, uint32 file_len)
{
    int32 l_ret = 0;
    struct ps_core_s *ps_core_d = NULL;
    if (NULL == baud)
    {
        return -1;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -1;
    }

    PS_PRINT_INFO("write data prepard succ");

    memset(&g_st_uart_download_test,0, sizeof(g_st_uart_download_test));
    memcpy(g_st_uart_download_test.baud, baud, sizeof(g_st_uart_download_test.baud)-1);
    g_st_uart_download_test.xmodern_len = XMODE_DATA_LEN;
    g_st_uart_download_test.file_len = file_len;
    g_st_uart_download_test.send_status = -1;

    //open power
    l_ret = hi1103_board_power_on(BFGX_POWER);
    if(l_ret)
    {
        PS_PRINT_ERR("hi1103_board_power_on bfgx failed=%d\n", l_ret);
        return -1;
    }

     //open tty
    if (BFGX_POWER_SUCCESS != open_tty_drv_etc(ps_core_d->pm_data))
    {
        PS_PRINT_ERR("open tty fail!\n");
        return -1;
    }
    tty_recv_etc = ps_recv_patch;
    PS_PRINT_INFO("#@open uart succ");

    /*初始化回调函数变量*/
    l_ret = patch_init(ENUM_INFO_UART);
    if (l_ret)
    {
        PS_PRINT_ERR("patch modem init failed, ret:%d!\n", l_ret);
        return -1;
    }
    PS_PRINT_INFO("#@patch_init succ");

    //recv buffer prepare
    l_ret = patch_download_buffer_pre(ENUM_INFO_UART);
    if (l_ret)
    {
        PS_PRINT_ERR("patch_download_buffer_pre");
        return -1;
    };
    PS_PRINT_INFO("#@buffer succ");

    //baud set
    l_ret = patch_number_type("BAUDRATE", g_st_uart_download_test.baud, ENUM_INFO_UART);
    if (l_ret < 0)
    {
        PS_PRINT_ERR("set baud fail!\n");
        return -1;
    }
    PS_PRINT_INFO("#@set baud:%s succ", g_st_uart_download_test.baud);

    l_ret = patch_download_patch_test(ENUM_INFO_UART);
    return l_ret;
}
#endif /* BFGX_UART_DOWNLOAD_SUPPORT */


void uart_loop_test_tx_buf_init(uint8* puc_data, uint16 us_data_len)
{
    get_random_bytes(puc_data, us_data_len);
}

int32 uart_loop_set_pkt_count_etc(uint32 count)
{
    PS_PRINT_INFO("uart loop test, set pkt count to [%d]\n", count);
    g_st_uart_loop_test_cfg_etc.loop_count = count;

    return 0;
}

int32 uart_loop_set_pkt_len_etc(uint32 pkt_len)
{
    PS_PRINT_INFO("uart loop test, set pkt len to [%d]\n", pkt_len);
    g_st_uart_loop_test_cfg_etc.pkt_len = pkt_len;

    return 0;
}

int32 alloc_uart_loop_test_etc(void)
{
    uint8 *uart_loop_tx_buf = NULL;
    uint8 *uart_loop_rx_buf = NULL;
    uint16 pkt_len = 0;

    if (NULL == g_pst_uart_loop_test_info_etc)
    {
        g_pst_uart_loop_test_info_etc = (uart_loop_test_struct *)kzalloc(sizeof(uart_loop_test_struct), GFP_KERNEL);
        if (NULL == g_pst_uart_loop_test_info_etc)
        {
            PS_PRINT_ERR("malloc g_pst_uart_loop_test_info_etc fail\n");
            goto malloc_test_info_fail;
        }

        pkt_len = g_st_uart_loop_test_cfg_etc.pkt_len;
        if (0 == pkt_len || pkt_len > UART_LOOP_MAX_PKT_LEN)
        {
            pkt_len = UART_LOOP_MAX_PKT_LEN;
            g_st_uart_loop_test_cfg_etc.pkt_len = UART_LOOP_MAX_PKT_LEN;
        }

        uart_loop_tx_buf = (uint8 *)kzalloc(pkt_len, GFP_KERNEL);
        if (NULL == uart_loop_tx_buf)
        {
            PS_PRINT_ERR("malloc uart_loop_tx_buf fail\n");
            goto malloc_tx_buf_fail;
        }

        memset(uart_loop_tx_buf, 0xa5, pkt_len);

        uart_loop_rx_buf = (uint8 *)kzalloc(pkt_len, GFP_KERNEL);
        if (NULL == uart_loop_rx_buf)
        {
            PS_PRINT_ERR("malloc uart_loop_rx_buf fail\n");
            goto malloc_rx_buf_fail;
        }

        g_st_uart_loop_test_cfg_etc.uart_loop_enable = 1;
        g_st_uart_loop_test_cfg_etc.uart_loop_tx_random_enable = 1;

        init_completion(&g_pst_uart_loop_test_info_etc->set_done);
        init_completion(&g_pst_uart_loop_test_info_etc->loop_test_done);

        g_pst_uart_loop_test_info_etc->test_cfg   = &g_st_uart_loop_test_cfg_etc;
        g_pst_uart_loop_test_info_etc->tx_buf     = uart_loop_tx_buf;
        g_pst_uart_loop_test_info_etc->rx_buf     = uart_loop_rx_buf;
        g_pst_uart_loop_test_info_etc->rx_pkt_len = 0;

        PS_PRINT_INFO("uart loop test, pkt len is [%d]\n", pkt_len);
        PS_PRINT_INFO("uart loop test, loop count is [%d]\n", g_st_uart_loop_test_cfg_etc.loop_count);
    }

    return 0;

malloc_rx_buf_fail:
    kfree(uart_loop_tx_buf);
malloc_tx_buf_fail:
    kfree(g_pst_uart_loop_test_info_etc);
    g_pst_uart_loop_test_info_etc = NULL;
malloc_test_info_fail:
    return -ENOMEM;
}

void free_uart_loop_test_etc(void)
{
    if (NULL == g_pst_uart_loop_test_info_etc)
    {
        return;
    }
    PS_PRINT_ERR("free uart loop test buf\n");
    g_st_uart_loop_test_cfg_etc.uart_loop_enable = 0;
    kfree(g_pst_uart_loop_test_info_etc->rx_buf);
    kfree(g_pst_uart_loop_test_info_etc->tx_buf);
    kfree(g_pst_uart_loop_test_info_etc);
    g_pst_uart_loop_test_info_etc = NULL;

    return;
}

int32 uart_loop_test_open_etc(void)
{
    struct ps_core_s *ps_core_d = NULL;
    int32  error = BFGX_POWER_SUCCESS;

    PS_PRINT_INFO("%s\n", __func__);

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == ps_core_d->ps_pm)||(NULL == ps_core_d->ps_pm->bfg_power_set)))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -EINVAL;
    }

    if (ps_chk_bfg_active_etc(ps_core_d))
    {
        PS_PRINT_ERR("bfgx subsys must all close\n");
        return -EINVAL;
    }

    if (BFGX_POWER_SUCCESS != alloc_uart_loop_test_etc())
    {
        PS_PRINT_ERR("alloc mem for uart loop test fail!\n");
        goto alloc_mem_fail;
    }

    error = ps_core_d->ps_pm->bfg_power_set(BFGX_GNSS, BFG_POWER_GPIO_UP);
    if (BFGX_POWER_SUCCESS != error)
    {
        PS_PRINT_ERR("uart loop test, power on err! error = %d\n", error);
        goto power_on_fail;
    }

    if (BFGX_POWER_SUCCESS != prepare_to_visit_node_etc(ps_core_d))
    {
        PS_PRINT_ERR("uart loop test, prepare work fail\n");
        error = BFGX_POWER_WAKEUP_FAIL;
        goto wakeup_fail;
    }

    post_to_visit_node_etc(ps_core_d);

    return BFGX_POWER_SUCCESS;

wakeup_fail:
    ps_core_d->ps_pm->bfg_power_set(BFGX_GNSS, BFG_POWER_GPIO_DOWN);
power_on_fail:
    free_uart_loop_test_etc();
alloc_mem_fail:
    return BFGX_POWER_FAILED;
}

int32 uart_loop_test_close_etc(void)
{
    struct ps_core_s *ps_core_d = NULL;

    PS_PRINT_INFO("%s", __func__);

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == ps_core_d->ps_pm)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    if (0 != prepare_to_visit_node_etc(ps_core_d))
    {
        PS_PRINT_ERR("uart loop test, prepare work fail\n");
    }

    if (0 != ps_core_d->ps_pm->bfg_power_set(BFGX_GNSS, BFG_POWER_GPIO_DOWN))
    {
        PS_PRINT_ERR("uart loop test, power off err!");
    }

    free_uart_loop_test_etc();

    post_to_visit_node_etc(ps_core_d);

    return 0;
}

int32 uart_loop_test_set_etc(uint8 flag)
{
    uint64 timeleft;
    struct ps_core_s *ps_core_d = NULL;
    uint8 cmd;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is null\n");
        return -EINVAL;
    }

    if (UART_LOOP_SET_DEVICE_DATA_HANDLER == flag)
    {
        cmd = SYS_CFG_SET_UART_LOOP_HANDLER;
    }
    else
    {
        cmd = SYS_CFG_SET_UART_LOOP_FINISH;
    }

    INIT_COMPLETION(g_pst_uart_loop_test_info_etc->set_done);
    ps_uart_state_pre_etc(ps_core_d->tty);
    ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, cmd);
    timeleft = wait_for_completion_timeout(&g_pst_uart_loop_test_info_etc->set_done, msecs_to_jiffies(5000));
    if (!timeleft)
    {
        ps_uart_state_dump_etc(ps_core_d->tty);
        PS_PRINT_ERR("wait set uart loop ack timeout\n");
        return -ETIMEDOUT;
    }

    return 0;
}

int32 uart_loop_test_send_data_etc(struct ps_core_s *ps_core_d, uint8 *buf, size_t count)
{
    struct sk_buff *skb;
    uint16 tx_skb_len;
    uint16 tx_gnss_len;
    uint8  start = 0;

    PS_PRINT_FUNCTION_NAME;

    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    while(count > 0)
    {
        if (count > GNSS_TX_PACKET_LIMIT)
        {
            tx_gnss_len = GNSS_TX_PACKET_LIMIT;
        }
        else
        {
            tx_gnss_len = count;
        }
        /* curr tx skb total lenth */
        tx_skb_len = tx_gnss_len + sizeof(struct ps_packet_head);
        tx_skb_len = tx_skb_len + sizeof(struct ps_packet_end);

        skb  = ps_alloc_skb_etc(tx_skb_len);
        if (NULL == skb)
        {
            PS_PRINT_ERR("ps alloc skb mem fail\n");
            return -EFAULT;
        }

        if (count > GNSS_TX_PACKET_LIMIT)
        {
            if (false == start)
            {   /* this is a start gnss packet */
                ps_add_packet_head_etc(skb->data, GNSS_First_MSG, tx_skb_len);
                start = true;
            }
            else
            {   /* this is a int gnss packet */
                ps_add_packet_head_etc(skb->data, GNSS_Common_MSG, tx_skb_len);
            }
        }
        else
        {   /* this is the last gnss packet */
            ps_add_packet_head_etc(skb->data, GNSS_Last_MSG, tx_skb_len);
        }

        memcpy(&skb->data[sizeof(struct ps_packet_head)], buf, tx_gnss_len);

        /* push the skb to skb queue */
        ps_skb_enqueue_etc(ps_core_d, skb, TX_LOW_QUEUE);
        queue_work(ps_core_d->ps_tx_workqueue, &ps_core_d->tx_skb_work);

        buf   = buf + tx_gnss_len;
        count = count - tx_gnss_len;
    }

    return 0;
}

int32 uart_loop_test_send_pkt_etc(void)
{
    uint64 timeleft;
    struct ps_core_s *ps_core_d = NULL;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d)||(NULL == g_pst_uart_loop_test_info_etc)||(NULL == g_pst_uart_loop_test_info_etc->tx_buf)))
    {
        PS_PRINT_ERR("para is invalided\n");
        return -EFAULT;
    }

    /* if low queue num > MAX_NUM and don't write */
    if (ps_core_d->tx_low_seq.qlen > TX_LOW_QUE_MAX_NUM)
    {
        PS_PRINT_ERR("uart loop test, tx low seq is too large [%d]\n", ps_core_d->tx_low_seq.qlen);
        return 0;
    }

    if (prepare_to_visit_node_etc(ps_core_d) < 0)
    {
        PS_PRINT_ERR("prepare work fail\n");
        return -EFAULT;
    }

    INIT_COMPLETION(g_pst_uart_loop_test_info_etc->loop_test_done);

    /* to divide up packet function and tx to tty work */
    if (uart_loop_test_send_data_etc(ps_core_d, g_pst_uart_loop_test_info_etc->tx_buf, g_st_uart_loop_test_cfg_etc.pkt_len) < 0)
    {
        PS_PRINT_ERR("uart loop test pkt send is err\n");
        post_to_visit_node_etc(ps_core_d);
        return -EFAULT;
    }

    timeleft = wait_for_completion_timeout(&g_pst_uart_loop_test_info_etc->loop_test_done, msecs_to_jiffies(5000));
    if (!timeleft)
    {
        ps_uart_state_dump_etc(ps_core_d->tty);
        PS_PRINT_ERR("wait uart loop done timeout\n");
        post_to_visit_node_etc(ps_core_d);
        return -ETIMEDOUT;
    }

    post_to_visit_node_etc(ps_core_d);

    return 0;
}

int32 uart_loop_test_data_check(uint8* puc_src, uint8* puc_dest, uint16 us_data_len)
{
    uint16 us_index;

    for (us_index = 0; us_index < us_data_len; us_index++)
    {
        if (puc_src[us_index] != puc_dest[us_index])
        {
            return false;
        }
    }

    return true;
}

int32 uart_loop_test_recv_pkt_etc(struct ps_core_s *ps_core_d, const uint8 *buf_ptr, uint16 pkt_len)
{
    uint16  expect_pkt_len;
    uint8 * rx_buf;
    uint16  recvd_len;

    if (unlikely((NULL == ps_core_d)||(NULL == g_pst_uart_loop_test_info_etc)))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    expect_pkt_len = g_pst_uart_loop_test_info_etc->test_cfg->pkt_len;
    rx_buf         = g_pst_uart_loop_test_info_etc->rx_buf;
    recvd_len      = g_pst_uart_loop_test_info_etc->rx_pkt_len;

    if (recvd_len + pkt_len <= expect_pkt_len)
    {
        memcpy(&rx_buf[recvd_len], buf_ptr, pkt_len);
        g_pst_uart_loop_test_info_etc->rx_pkt_len += pkt_len;
    }
    else
    {
        PS_PRINT_ERR("pkt len err! pkt_len=[%d], recvd_len=[%d], max len=[%d]\n", pkt_len, recvd_len, expect_pkt_len);
    }

    if (expect_pkt_len == g_pst_uart_loop_test_info_etc->rx_pkt_len)
    {
        if (uart_loop_test_data_check(rx_buf, g_pst_uart_loop_test_info_etc->tx_buf, expect_pkt_len))
        {
            PS_PRINT_INFO("uart loop recv pkt SUCC\n");
        }
        g_pst_uart_loop_test_info_etc->rx_pkt_len = 0;
        complete(&g_pst_uart_loop_test_info_etc->loop_test_done);
    }

    return 0;
}

int32 uart_loop_test_etc(void)
{
    uint32 i, count;
    uint16 pkt_len;
    unsigned long long tx_total_len = 0, total_time = 0, throughout, effect;
    ktime_t start_time, end_time, trans_time;
    uint8* puc_buf;

    if (uart_loop_test_open_etc() < 0)
    {
        goto open_fail;
    }

    if (uart_loop_test_set_etc(UART_LOOP_SET_DEVICE_DATA_HANDLER) < 0)
    {
        goto test_set_fail;
    }

    count   = g_pst_uart_loop_test_info_etc->test_cfg->loop_count;
    pkt_len = g_pst_uart_loop_test_info_etc->test_cfg->pkt_len;
    tx_total_len = ((unsigned long long)count) * ((unsigned long long)pkt_len);
    puc_buf = g_pst_uart_loop_test_info_etc->tx_buf;

    for (i = 0; i < count; i++)
    {
        if (g_pst_uart_loop_test_info_etc->test_cfg->uart_loop_tx_random_enable)
        {
            uart_loop_test_tx_buf_init(puc_buf, pkt_len); // 初始化tx_buf为随机数
        }

        start_time = ktime_get();

        if (SUCCESS != uart_loop_test_send_pkt_etc())
        {
            PS_PRINT_ERR("uart loop test fail, i=[%d]\n", i);
            goto send_pkt_fail;
        }

        end_time = ktime_get();
        trans_time = ktime_sub(end_time, start_time);
        total_time += (unsigned long long)ktime_to_us(trans_time);
    }

    if (uart_loop_test_set_etc(UART_LOOP_RESUME_DEVICE_DATA_HANDLER) < 0)
    {
        PS_PRINT_ERR("uart loop test, resume device data handler failer\n");
    }

    uart_loop_test_close_etc();

    throughout = tx_total_len*1000000*10*2;
    do_div(throughout, total_time);
    effect = throughout;
    do_div(throughout, 8192);
    do_div(effect, (g_default_baud_rate/100));

    PS_PRINT_INFO("[UART Test] pkt count      [%d] pkts sent\n", count);
    PS_PRINT_INFO("[UART Test] pkt len        [%d] is pkt len\n", pkt_len);
    PS_PRINT_INFO("[UART Test] data lenth     [%llu]\n", tx_total_len*2);
    PS_PRINT_INFO("[UART Test] used time      [%llu] us\n", total_time);
    PS_PRINT_INFO("[UART Test] throughout     [%llu] KBps\n", throughout);
    PS_PRINT_INFO("[UART Test] effect         [%llu]%%\n", effect);

    return SUCCESS;

send_pkt_fail:
test_set_fail:
    uart_loop_test_close_etc();
open_fail:
    return -FAILURE;
}

int conn_test_uart_loop_etc(char *param)
{
    return uart_loop_test_etc();
}
EXPORT_SYMBOL(conn_test_uart_loop_etc);

/**********************************************************************/
STATIC const struct file_operations hw_bt_fops = {
        .owner          = THIS_MODULE,
        .open           = hw_bt_open,
        .write          = hw_bt_write,
        .read           = hw_bt_read,
        .poll           = hw_bt_poll,
        .unlocked_ioctl = hw_bt_ioctl,
        .release        = hw_bt_release,
};

STATIC const struct file_operations hw_fm_fops = {
        .owner          = THIS_MODULE,
        .open           = hw_fm_open,
        .write          = hw_fm_write,
        .read           = hw_fm_read,
        .unlocked_ioctl = hw_fm_ioctl,
        .release        = hw_fm_release,
};

STATIC const struct file_operations hw_gnss_fops = {
        .owner          = THIS_MODULE,
        .open           = hw_gnss_open,
        .write          = hw_gnss_write,
        .read           = hw_gnss_read,
        .poll           = hw_gnss_poll,
        .unlocked_ioctl = hw_gnss_ioctl,
        .release        = hw_gnss_release,
};

static const struct file_operations hw_ir_fops = {
        .owner          = THIS_MODULE,
        .open           = hw_ir_open,
        .write          = hw_ir_write,
        .read           = hw_ir_read,
        .release        = hw_ir_release,
};


static const struct file_operations hw_nfc_fops = {
        .owner          = THIS_MODULE,
        .open           = hw_nfc_open,
        .write          = hw_nfc_write,
        .read           = hw_nfc_read,
        .poll           = hw_nfc_poll,
        .release        = hw_nfc_release,
};

STATIC const struct file_operations hw_debug_fops = {
        .owner          = THIS_MODULE,
        .open           = hw_debug_open,
#ifdef PLATFORM_DEBUG_ENABLE
        .write          = hw_debug_write,
#endif
        .read           = hw_debug_read,
        .unlocked_ioctl = hw_debug_ioctl,
        .release        = hw_debug_release,
};
#ifdef HI110X_HAL_MEMDUMP_ENABLE
STATIC const struct file_operations hw_bfgexcp_fops = {
        .owner          = THIS_MODULE,
        .read           = hw_bfgexcp_read,
        .unlocked_ioctl = hw_bfgexcp_ioctl,
};

STATIC const struct file_operations hw_wifiexcp_fops = {
        .owner          = THIS_MODULE,
        .read           = hw_wifiexcp_read,
        .unlocked_ioctl = hw_wifiexcp_ioctl,
};
#else
STATIC const struct file_operations hw_excp_fops = {
        .owner          = THIS_MODULE,
        .open           = hw_excp_open,
        .unlocked_ioctl = hw_excp_ioctl,
        .release        = hw_excp_release,
};
#endif
#ifdef HAVE_HISI_BT
STATIC struct miscdevice hw_bt_device = {
        .minor  = MISC_DYNAMIC_MINOR,
        .name   = "hwbt",
        .fops   = &hw_bt_fops,
};
#endif

#ifdef HAVE_HISI_FM
STATIC struct miscdevice hw_fm_device = {
        .minor  = MISC_DYNAMIC_MINOR,
        .name   = "hwfm",
        .fops   = &hw_fm_fops,
};
#endif

#ifdef HAVE_HISI_GNSS
STATIC struct miscdevice hw_gnss_device = {
        .minor  = MISC_DYNAMIC_MINOR,
        .name   = "hwgnss",
        .fops   = &hw_gnss_fops,
};
#endif

#ifdef HAVE_HISI_IR
STATIC struct miscdevice hw_ir_device = {
        .minor  = MISC_DYNAMIC_MINOR,
        .name   = "hwir",
        .fops   = &hw_ir_fops,
};
#endif

#ifdef HAVE_HISI_NFC
STATIC struct miscdevice hw_nfc_device = {
        .minor  = MISC_DYNAMIC_MINOR,
        .name   = "hwnfc",
        .fops   = &hw_nfc_fops,
};
#endif

STATIC struct miscdevice hw_debug_device = {
        .minor  = MISC_DYNAMIC_MINOR,
        .name   = "hwbfgdbg",
        .fops   = &hw_debug_fops,
};
#ifdef HI110X_HAL_MEMDUMP_ENABLE
STATIC struct miscdevice hw_bfgexcp_device = {
        .minor  = MISC_DYNAMIC_MINOR,
        .name   = "hwbfgexcp",
        .fops   = &hw_bfgexcp_fops,
};
STATIC struct miscdevice hw_wifiexcp_device = {
        .minor  = MISC_DYNAMIC_MINOR,
        .name   = "hwwifiexcp",
        .fops   = &hw_wifiexcp_fops,
};
#else
STATIC struct miscdevice hw_excp_device = {
        .minor  = MISC_DYNAMIC_MINOR,
        .name   = "hwexcp",
        .fops   = &hw_excp_fops,
};
#endif
static struct  hw_ps_plat_data   hisi_platform_data = {
    .dev_name           = "/dev/ttyAMA4",
    .flow_cntrl         = FLOW_CTRL_ENABLE,
    .baud_rate          = DEFAULT_BAUD_RATE,
    .suspend            = NULL,
    .resume             = NULL,
    .set_bt_power       = NULL,
    .set_fm_power       = NULL,
    .set_gnss_power     = NULL,
    .clear_bt_power     = NULL,
    .clear_fm_power     = NULL,
    .clear_gnss_power   = NULL,
};
#ifdef HAVE_HISI_NFC
static int plat_poweroff_notify_sys(struct notifier_block *this, unsigned long code,void *unused)
{
    struct ps_core_s    *ps_core_d     = NULL;
    struct st_bfgx_data *pst_gnss_data = NULL;
    struct inode gnss_inode;
    struct file gnss_filp;
    int32  err;


    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("plat_poweroff_notify_sys get ps_core_d is NULL\n");
        return NOTIFY_BAD;
    }

    pst_gnss_data = &ps_core_d->bfgx_info[BFGX_GNSS];
    if (POWER_STATE_OPEN == atomic_read(&pst_gnss_data->subsys_state))
    {
        err = hw_gnss_release(&gnss_inode, &gnss_filp);
        if (0 != err)
        {
            PS_PRINT_WARNING("plat_poweroff_notify_sys call hw_gnss_release failed (err=%d)\n",err);
        }
    }
    return NOTIFY_OK;
}

static struct notifier_block  plat_poweroff_notifier = {
        .notifier_call = plat_poweroff_notify_sys,
};
#endif

#ifdef _PRE_CONFIG_USE_DTS
STATIC int32 ps_probe(struct platform_device *pdev)
{
    struct hw_ps_plat_data *pdata = NULL;
    struct ps_plat_s *ps_plat_d;
    int32  err;
    BOARD_INFO * bd_info = NULL;

    bd_info = get_hi110x_board_info_etc();
    if (unlikely(NULL == bd_info))
    {
        PS_PRINT_ERR("board info is err\n");
        return -FAILURE;
    }

    strncpy(hisi_platform_data.dev_name, bd_info->uart_port, sizeof(hisi_platform_data.dev_name) - 1);
    hisi_platform_data.dev_name[sizeof(hisi_platform_data.dev_name) - 1] = '\0';

    /*FPGA版本支持2M，动态修改*/
    if(!isAsic_etc())
    {
        hisi_platform_data.baud_rate = LOW_FREQ_BAUD_RATE;
        g_default_baud_rate = LOW_FREQ_BAUD_RATE;
    }

    pdev->dev.platform_data = &hisi_platform_data;
    pdata = &hisi_platform_data;

    hw_ps_device_etc = pdev;

    ps_plat_d = kzalloc(sizeof(struct ps_plat_s), GFP_KERNEL);
    if (NULL == ps_plat_d)
    {
        PS_PRINT_ERR("no mem to allocate\n");
        return -ENOMEM;
    }
    dev_set_drvdata(&pdev->dev, ps_plat_d);

    err = ps_core_init_etc(&ps_plat_d->core_data);
    if (0 != err)
    {
        PS_PRINT_ERR(" PS core init failed\n");
        goto err_core_init;
    }

    /* refer to itself */
    ps_plat_d->core_data->pm_data = ps_plat_d;
    /* get reference of pdev */
    ps_plat_d->pm_pdev = pdev;

    init_completion(&ps_plat_d->ldisc_uninstalled);
    init_completion(&ps_plat_d->ldisc_installed);
    init_completion(&ps_plat_d->ldisc_reconfiged);

    err = plat_bfgx_exception_rst_register_etc(ps_plat_d);
    if (0 > err)
    {
        PS_PRINT_ERR("bfgx_exception_rst_register failed\n");
        goto err_exception_rst_reg;
    }

    err = bfgx_user_ctrl_init_etc();
    if (0 > err)
    {
        PS_PRINT_ERR("bfgx_user_ctrl_init_etc failed\n");
        goto err_user_ctrl_init;
    }

    err = bfgx_customize_init();
    if (0 > err)
    {
        PS_PRINT_ERR("bfgx_customize_init failed\n");
        goto err_bfgx_custmoize_exit;
    }

    /* copying platform data */
    strncpy(ps_plat_d->dev_name, pdata->dev_name, HISI_UART_DEV_NAME_LEN - 1);
    ps_plat_d->flow_cntrl = pdata->flow_cntrl;
    ps_plat_d->baud_rate  = pdata->baud_rate;
    PS_PRINT_INFO("sysfs entries created\n");

    tty_recv_etc = ps_core_recv_etc;

#ifdef HAVE_HISI_BT
    err = misc_register(&hw_bt_device);
    if (0 != err)
    {
        PS_PRINT_ERR("Failed to register bt inode\n ");
        goto err_register_bt;
    }
#endif

#ifdef HAVE_HISI_FM
    err = misc_register(&hw_fm_device);
    if (0 != err)
    {
        PS_PRINT_ERR("Failed to register fm inode\n ");
        goto err_register_fm;
    }
#endif

#ifdef HAVE_HISI_GNSS
    err = misc_register(&hw_gnss_device);
    if (0 != err)
    {
        PS_PRINT_ERR("Failed to register gnss inode\n ");
        goto err_register_gnss;
    }
#endif

#ifdef HAVE_HISI_IR
    err = misc_register(&hw_ir_device);
    if (0 != err)
    {
        PS_PRINT_ERR("Failed to register ir inode\n");
        goto err_register_ir;
    }
#endif

#ifdef HAVE_HISI_NFC
    err = misc_register(&hw_nfc_device);
    if (0 != err)
    {
        PS_PRINT_ERR("Failed to register nfc inode\n ");
        goto err_register_nfc;
    }

    err = register_reboot_notifier(&plat_poweroff_notifier);
    if (err)
    {
        PS_PRINT_WARNING("Failed to registe plat_poweroff_notifier (err=%d)\n",err);
    }
#endif

    err = misc_register(&hw_debug_device);
    if (0 != err)
    {
        PS_PRINT_ERR("Failed to register debug inode\n");
        goto err_register_debug;
    }
#ifdef HI110X_HAL_MEMDUMP_ENABLE
    err = misc_register(&hw_bfgexcp_device);
    if (0 != err)
    {
        PS_PRINT_ERR("Failed to register hw_bfgexcp_device inode\n");
        goto err_register_bfgexcp;
    }

    err = misc_register(&hw_wifiexcp_device);
    if (0 != err)
    {
        PS_PRINT_ERR("Failed to register hw_wifiexcp_device inode\n");
        goto err_register_wifiexcp;
    }
#else
    err = misc_register(&hw_excp_device);
    if (0 != err)
    {
        PS_PRINT_ERR("Failed to register hw excp inode\n");
        goto err_register_excp;
    }
#endif

    PS_PRINT_SUC("%s is success!\n", __func__);

    return 0;
#ifdef HI110X_HAL_MEMDUMP_ENABLE
err_register_wifiexcp:
    misc_deregister(&hw_bfgexcp_device);
err_register_bfgexcp:
#else
err_register_excp:
#endif
    misc_deregister(&hw_debug_device);
err_register_debug:
#ifdef HAVE_HISI_NFC
    misc_deregister(&hw_nfc_device);
err_register_nfc:
#endif
#ifdef HAVE_HISI_IR
    misc_deregister(&hw_ir_device);
err_register_ir:
#endif
#ifdef HAVE_HISI_GNSS
    misc_deregister(&hw_gnss_device);
err_register_gnss:
#endif
#ifdef HAVE_HISI_FM
    misc_deregister(&hw_fm_device);
err_register_fm:
#endif
#ifdef HAVE_HISI_BT
    misc_deregister(&hw_bt_device);
err_register_bt:
#endif
err_bfgx_custmoize_exit:
    bfgx_user_ctrl_exit_etc();
err_user_ctrl_init:
err_exception_rst_reg:
    ps_core_exit_etc(ps_plat_d->core_data);
err_core_init:
	kfree(ps_plat_d);

    return -EFAULT;
}

/**
 * Prototype    : ps_suspend_etc
 * Description  : called by kernel when kernel goto suspend
 * input        : pdev, state
 * output       : return 0 --> ps_suspend_etc is ok
 *                return !0--> ps_suspend_etc is false
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
int32 ps_suspend_etc(struct platform_device *pdev, pm_message_t state)
{
#if 0
    struct hw_ps_plat_data  *pdata = pdev->dev.platform_data;
    if (pdata->suspend)
    {
        return pdata->suspend(pdev, state);
    }

    return -EOPNOTSUPP;
#else
    return 0;
#endif
}

/**
 * Prototype    : ps_resume_etc
 * Description  : called by kernel when kernel resume from suspend
 * input        : pdev
 * output       : return 0 --> ps_suspend_etc is ok
 *                return !0--> ps_suspend_etc is false
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
int32 ps_resume_etc(struct platform_device *pdev)
{
#if 0
    struct hw_ps_plat_data  *pdata = pdev->dev.platform_data;
    if (pdata->resume)
    {
        return pdata->resume(pdev);
    }

    return -EOPNOTSUPP;
#else
    return 0;
#endif
}
/**
 * Prototype    : ps_remove
 * Description  : called when user applation rmmod driver
 * input        : pdev
 * output       : return 0 --> ps_suspend_etc is ok
 *                return !0--> ps_suspend_etc is false
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
STATIC int32 ps_remove(struct platform_device *pdev)
{
    struct ps_plat_s *ps_plat_d;
    struct hw_ps_plat_data *pdata;

    PS_PRINT_FUNCTION_NAME;

    pdata = pdev->dev.platform_data;
    ps_plat_d = dev_get_drvdata(&pdev->dev);
    if (NULL == ps_plat_d)
    {
        PS_PRINT_ERR("ps_plat_d is null\n");
    }

    bfgx_user_ctrl_exit_etc();
    PS_PRINT_INFO("sysfs user ctrl removed\n");

    if (NULL != ps_plat_d)
    {
        ps_plat_d->pm_pdev = NULL;
        ps_core_exit_etc(ps_plat_d->core_data);
    }

#ifdef HAVE_HISI_BT
    misc_deregister(&hw_bt_device);
    PS_PRINT_INFO("misc bt device have removed\n");
#endif
#ifdef HAVE_HISI_FM
    misc_deregister(&hw_fm_device);
    PS_PRINT_INFO("misc fm device have removed\n");
#endif
#ifdef HAVE_HISI_GNSS
    misc_deregister(&hw_gnss_device);
    PS_PRINT_INFO("misc gnss device have removed\n");
#endif
#ifdef HAVE_HISI_IR
    misc_deregister(&hw_ir_device);
    PS_PRINT_INFO("misc ir have removed\n");
#endif
#ifdef HAVE_HISI_NFC
    misc_deregister(&hw_nfc_device);
    PS_PRINT_INFO("misc nfc have removed\n");
#endif
    misc_deregister(&hw_debug_device);
    PS_PRINT_INFO("misc debug device have removed\n");

    if (NULL != ps_plat_d)
    {
        kfree(ps_plat_d);
        ps_plat_d = NULL;
    }

    return 0;
}

static struct of_device_id hi110x_ps_match_table[] = {
	{
		.compatible = DTS_COMP_HI110X_PS_NAME,
		.data = NULL,
    },
	{ },
};
#endif

/**********************************************************/
/*  platform_driver struct for PS module */
STATIC struct platform_driver ps_platform_driver = {
#ifdef _PRE_CONFIG_USE_DTS
        .probe      = ps_probe,
        .remove     = ps_remove,
        .suspend    = ps_suspend_etc,
        .resume     = ps_resume_etc,
#endif
        .driver     = {
            .name   = "hisi_bfgx",
            .owner  = THIS_MODULE,
#ifdef _PRE_CONFIG_USE_DTS
			.of_match_table	= hi110x_ps_match_table,
#endif
        },
};

int32 hw_ps_init_etc(void)
{
    int32 ret;

    PS_PRINT_FUNCTION_NAME;

    ret = platform_driver_register(&ps_platform_driver);
    if (ret)
    {
        PS_PRINT_ERR("Unable to register platform bfgx driver.\n");
    }
    return ret;
}

void hw_ps_exit_etc(void)
{
    platform_driver_unregister(&ps_platform_driver);
}

MODULE_DESCRIPTION("Public serial Driver for huawei BT/FM/GNSS chips");
MODULE_LICENSE("GPL");

