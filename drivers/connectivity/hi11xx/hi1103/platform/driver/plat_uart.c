

/*****************************************************************************
  1 Header File Including
*****************************************************************************/
/*lint -e322*//*lint -e7*/
#include <linux/tty.h>
#include <linux/delay.h>
#include "plat_debug.h"
#include "plat_uart.h"
#include "plat_pm.h"
#include "hw_bfg_ps.h"
#include "bfgx_user_ctrl.h"
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/jiffies.h>

#include "oal_ext_if.h"
/*lint +e322*//*lint +e7*/

#ifdef BFGX_UART_DOWNLOAD_SUPPORT
#include "wireless_patch.h"
#endif
/*****************************************************************************
  2 Global Variable Definition
*****************************************************************************/
STATIC struct ps_uart_state_s g_uart_state = {0};
STATIC struct ps_uart_state_s g_uart_state_pre = {0};
uint32 g_default_baud_rate = DEFAULT_BAUD_RATE;
struct mutex  g_tty_mutex_etc;

/*****************************************************************************
  3 Function Definition
*****************************************************************************/
/* no lock while getting the state, just statistic*/
/* call only in one place!!! */
void ps_uart_tty_tx_add_etc(uint32 cnt)
{
    g_uart_state.tty_tx_cnt += cnt;
}
/* call only in one place!!! */
STATIC void ps_uart_tty_rx_add(uint32 cnt)
{
    g_uart_state.tty_rx_cnt += cnt;
}

uint32 ps_uart_state_cur_etc(uint32 index)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
    struct ps_core_s *ps_core_d = NULL;
    struct uart_state *state = NULL;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d) || (NULL == ps_core_d->tty) || (NULL == ps_core_d->tty->driver_data)))
    {
        PS_PRINT_ERR("ps_core_d ot tty is NULL\n");
        return 0;
    }

    state = ps_core_d->tty->driver_data;
    if (unlikely(NULL == state->uart_port))
    {
        PS_PRINT_ERR("uart_port is NULL\n");
        return 0;
    }
    switch (index)
    {
        case STATE_TTY_TX:
            return g_uart_state.tty_tx_cnt;
        case STATE_TTY_RX:
            return g_uart_state.tty_rx_cnt;
        case STATE_UART_TX:
            return state->uart_port->icount.tx;
        case STATE_UART_RX:
            return state->uart_port->icount.rx;
        default :
            PS_PRINT_ERR("not support index\n");
            break;
    }
#else
#warning ps_uart_state_cur_etc need adapt
#endif
    return 0;
}

STATIC void ps_uart_state_get(struct tty_struct *tty)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35))
    struct uart_state *state = NULL;

    if (unlikely((NULL == tty) || (NULL == tty->driver_data)))
    {
        PS_PRINT_DBG("tty_struct or driver_data is NULL\n");
        return;
    }
    state = tty->driver_data;
    if (unlikely(NULL == state->uart_port))
    {
        PS_PRINT_ERR("uart_port is NULL\n");
        return;
    }

    memcpy(&g_uart_state.uart_cnt, &state->uart_port->icount, sizeof(struct uart_icount));
    g_uart_state.tty_stopped = tty->stopped;
    g_uart_state.tty_hw_stopped = tty->hw_stopped;
#else
#warning ps_uart_state_get need adapt
#endif
    return;
}


STATIC void ps_uart_state_print(struct ps_uart_state_s *state)
{
    if (unlikely(NULL == state))
    {
        PS_PRINT_ERR("state is NULL\n");
        return;
    }

    PS_PRINT_INFO(" tty tx:%x    rx:%x\n", state->tty_tx_cnt, state->tty_rx_cnt);
    PS_PRINT_INFO("uart tx:%x    rx:%x\n", state->uart_cnt.tx, state->uart_cnt.rx);
    PS_PRINT_INFO("stopped:%x  hw_stopped:%x\n", state->tty_stopped, state->tty_hw_stopped);
    PS_PRINT_INFO("uart cts:%x,dsr:%x,rng:%x,dcd:%x,frame:%x,overrun:%x,parity:%x,brk:%x,buf_overrun:%x\n", \
                  state->uart_cnt.cts, state->uart_cnt.dsr, state->uart_cnt.rng, state->uart_cnt.dcd, \
                  state->uart_cnt.frame, state->uart_cnt.overrun, state->uart_cnt.parity, state->uart_cnt.brk, state->uart_cnt.buf_overrun);
    return;
}

void ps_uart_state_pre_etc(struct tty_struct *tty)
{
    ps_uart_state_get(tty);
    memcpy(&g_uart_state_pre, &g_uart_state, sizeof(struct ps_uart_state_s));
    return;
}

void ps_uart_state_dump_etc(struct tty_struct *tty)
{
    struct ps_core_s *ps_core_d = NULL;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
    struct uart_state *state = NULL;
#endif

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d) || (NULL == tty)))
    {
        PS_PRINT_ERR("ps_core_d ot tty is NULL\n");
        return;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
    state = tty->driver_data;
    if (unlikely(NULL == state->uart_port))
    {
        PS_PRINT_ERR("uart_port is NULL\n");
        return;
    }
#endif

    PS_PRINT_INFO("===pre uart&tty state===\n");
    ps_uart_state_print(&g_uart_state_pre);
    PS_PRINT_INFO("===cur uart&tty state===\n");
    ps_uart_state_get(tty);
    ps_uart_state_print(&g_uart_state);
    PS_PRINT_INFO("chars in tty tx buf len=%x\n", tty_chars_in_buffer(ps_core_d->tty));

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
    PS_PRINT_INFO("uart port mctrl:0x%x\n", state->uart_port->mctrl);

    PS_PRINT_INFO("DR   :0x%x\n", readw(state->uart_port->membase + 0x00));
    PS_PRINT_INFO("FR   :0x%x\n", readw(state->uart_port->membase + 0x18));
    PS_PRINT_INFO("IBRD :0x%x\n", readw(state->uart_port->membase + 0x24));
    PS_PRINT_INFO("FBRD :0x%x\n", readw(state->uart_port->membase + 0x28));
    PS_PRINT_INFO("LCR_H:0x%x\n", readw(state->uart_port->membase + 0x2C));
    PS_PRINT_INFO("CR   :0x%x\n", readw(state->uart_port->membase + 0x30));
    PS_PRINT_INFO("IFLS :0x%x\n", readw(state->uart_port->membase + 0x34));
    PS_PRINT_INFO("IMSC :0x%x\n", readw(state->uart_port->membase + 0x38));
    PS_PRINT_INFO("RIS  :0x%x\n", readw(state->uart_port->membase + 0x3C));
    PS_PRINT_INFO("MIS  :0x%x\n", readw(state->uart_port->membase + 0x40));
#endif

    return;
}

/**
 * Prototype    : ps_tty_complete_etc
 * Description  : to signal completion of line discipline installation
 *                  called from PS Core, upon tty_open.
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
int32 ps_tty_complete_etc(void *pm_data, uint8 install)
{
    struct ps_plat_s *ps_plat_d = (struct ps_plat_s *)pm_data;

    switch(install)
    {
        case TTY_LDISC_UNINSTALL:
            complete(&ps_plat_d->ldisc_uninstalled);
            break;

        case TTY_LDISC_INSTALL:
            complete(&ps_plat_d->ldisc_installed);
            break;

        case TTY_LDISC_RECONFIG:
            complete(&ps_plat_d->ldisc_reconfiged);
            break;

        default:
            PS_PRINT_ERR("unknown install type [%d]\n", install);
            break;
    }

    return 0;
}

/********************************************************************/
/**
 * Prototype    : ps_tty_open
 * Description  : called by tty uart itself when open tty uart from octty
 * input        : tty -> have opened tty
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
STATIC int32 ps_tty_open(struct tty_struct *tty)
{
    uint8  install;
    struct ps_core_s *ps_core_d = NULL;
    struct ps_plat_s *ps_plat_d = NULL;

    PS_PRINT_INFO("%s enter\n", __func__);

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    ps_core_d->tty = tty;
    tty->disc_data = ps_core_d;

    /* don't do an wakeup for now */
    clear_bit(TTY_DO_WRITE_WAKEUP, &tty->flags);

    /* set mem already allocated */
    tty->receive_room = PUBLIC_BUF_MAX;
    /* Flush any pending characters in the driver and discipline. */
    tty_ldisc_flush(tty);
    tty_driver_flush_buffer(tty);

    ps_plat_d = (struct ps_plat_s *)ps_core_d->pm_data;
    install   = ps_plat_d->ldisc_install;
    if (TTY_LDISC_INSTALL == install)
    {
        ps_tty_complete_etc(ps_core_d->pm_data, TTY_LDISC_INSTALL);
        PS_PRINT_INFO("install complete done!\n");
    }
    else if (TTY_LDISC_RECONFIG == install)
    {
        ps_tty_complete_etc(ps_core_d->pm_data, TTY_LDISC_RECONFIG);
        PS_PRINT_INFO("reconfig complete done!\n");
    }
    else
    {
        PS_PRINT_ERR("ldisc_install [%d] is error!\n", install);
    }

    return 0;
}

/**
 * Prototype    : ps_tty_close
 * Description  : called by tty uart when close tty uart from octty
 * input        : tty -> have opened tty
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
STATIC void ps_tty_close(struct tty_struct *tty)
{
    struct  ps_core_s *ps_core_d = NULL;

    PS_PRINT_INFO("%s: entered!!!\n", __func__);

    if ((NULL == tty)||(NULL == tty->disc_data))
    {
        PS_PRINT_ERR("tty or tty->disc_data is NULL\n");
        return;
    }

    mutex_lock(&g_tty_mutex_etc);

    ps_core_d = tty->disc_data;

    /* Flush any pending characters in the driver and discipline. */
    tty_ldisc_flush(tty);
    tty_driver_flush_buffer(tty);
    ps_core_d->tty = NULL;

    /* signal to complate that N_HW_BFG ldisc is un-installed */
    ps_tty_complete_etc(ps_core_d->pm_data, TTY_LDISC_UNINSTALL);

    mutex_unlock(&g_tty_mutex_etc);

    PS_PRINT_INFO("uninstall complete done!\n");

}

/**
 * Prototype    : ps_tty_receive
 * Description  : called by tty uart when recive data from tty uart
 * input        : tty   -> have opened tty
 *                data -> recive data ptr
 *                count-> recive data count
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
STATIC void ps_tty_receive(struct tty_struct *tty, const uint8 *data,
               int8 *tty_flags, int32 count)
{
#ifdef PLATFORM_DEBUG_ENABLE
    struct timeval tv;
    struct rtc_time tm;
    uint64  tmp;
    char filename[60] = {0};
#endif
    struct  ps_core_s *ps_core_d = NULL;

    PS_PRINT_FUNCTION_NAME;

    if (unlikely((NULL == tty)||(NULL == tty->disc_data)||(NULL == tty_recv_etc)))
    {
        PS_PRINT_ERR("tty or tty->disc_data or tty_recv_etc is NULL\n");
        return;
    }
    ps_core_d = tty->disc_data;
    spin_lock(&ps_core_d->rx_lock);
#ifdef PLATFORM_DEBUG_ENABLE
    if(g_uart_rx_dump_etc)
    {
        ps_core_d->curr_time = jiffies;
        tmp = ps_core_d->curr_time - ps_core_d->pre_time;
        if ((tmp > DBG_FILE_TIME * HZ)||(0 == ps_core_d->pre_time))
        {
            if (!OAL_IS_ERR_OR_NULL(ps_core_d->rx_data_fp))
            {
                filp_close(ps_core_d->rx_data_fp, NULL);
            }
            do_gettimeofday(&tv);
            rtc_time_to_tm(tv.tv_sec, &tm);
            snprintf(filename, sizeof(filename) - 1, "/data/hwlogdir/uart_rx/uart_rx-%04d-%02d-%02d:%02d-%02d-%02d",
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            PS_PRINT_INFO("filename = %s",filename);

            ps_core_d->rx_data_fp = filp_open(filename, O_RDWR | O_CREAT, 0777);
            ps_core_d->pre_time = ps_core_d->curr_time;
        }
    }
#endif

    PS_PRINT_DBG("RX:data[0] = %x, data[1] = %x, data[2] = %x, data[3] = %x, data[4] = %x, data[count-1] = %x\n",
                 data[0],data[1],data[2],data[3],data[4],data[count-1]);
    ps_uart_tty_rx_add(count);
    tty_recv_etc(tty->disc_data, data, count);

    spin_unlock(&ps_core_d->rx_lock);
}

/**
 * Prototype    : ps_tty_wakeup
 * Description  : called by tty uart when wakeup from suspend
 * input        : tty   -> have opened tty
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
STATIC void ps_tty_wakeup(struct tty_struct *tty)
{
    struct  ps_core_s *ps_core_d = NULL;

    PS_PRINT_FUNCTION_NAME;
    if ((NULL == tty)||(NULL == tty->disc_data))
    {
        PS_PRINT_ERR("tty or tty->disc_data is NULL\n");
        return;
    }
    ps_core_d = tty->disc_data;
    /* don't do an wakeup for now */
    clear_bit(TTY_DO_WRITE_WAKEUP, &tty->flags);

    /* call our internal wakeup */
    queue_work(ps_core_d->ps_tx_workqueue, &ps_core_d->tx_skb_work);
}

/**
 * Prototype    : ps_tty_flush_buffer
 * Description  : called by tty uart when flush buffer
 * input        : tty   -> have opened tty
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
STATIC void ps_tty_flush_buffer(struct tty_struct *tty)
{
    PS_PRINT_INFO("time to %s\n", __func__);

    reset_uart_rx_buf_etc();

    return;
}

/**
 * Prototype    : ps_change_uart_baud_rate_etc
 * Description  : change arm platform uart baud rate to secend
 *                baud rate for high baud rate when download patch
 * input        : ps_core_d
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
 extern     void dump_uart_rx_buf(void);
int32 ps_change_uart_baud_rate_etc(int64 baud_rate, uint8 enable_flowctl)
{
    struct ps_plat_s *ps_plat_d = NULL;
    struct ps_core_s *ps_core_d;
    uint64 timeleft = 0;
    struct st_exception_info *pst_exception_data = NULL;

    PS_PRINT_INFO("%s %lu\n", __func__,baud_rate);

    ps_get_plat_reference_etc(&ps_plat_d);
    if (unlikely(NULL == ps_plat_d))
    {
        PS_PRINT_ERR("ps_plat_d is NULL\n");
        return -EINVAL;
    }

    ps_core_d = ps_plat_d->core_data;
    if (NULL == ps_core_d)
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EINVAL;
    }

    /* for debug only */
    dump_uart_rx_buf();

    get_exception_info_reference_etc(&pst_exception_data);

    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EINVAL;
    }

    if (1 == atomic_read(&pst_exception_data->is_memdump_runing))
    {
        INIT_COMPLETION(pst_exception_data->wait_read_bfgx_stack);
        /*wait for read stack completion*/
        timeleft = wait_for_completion_timeout(&pst_exception_data->wait_read_bfgx_stack, msecs_to_jiffies(WAIT_BFGX_READ_STACK_TIME));
        if (!timeleft)
        {
            ps_uart_state_dump_etc(ps_core_d->tty);
            atomic_set(&pst_exception_data->is_memdump_runing, 0);
            PS_PRINT_ERR("read bfgx stack failed!\n");
        }
        else
        {
            PS_PRINT_INFO("read bfgx stack success!\n");
        }
    }

    if (!OAL_IS_ERR_OR_NULL(ps_core_d->tty))
    {
        if (tty_chars_in_buffer(ps_core_d->tty))
        {
            PS_PRINT_INFO("uart tx buf is not empty\n");
            tty_driver_flush_buffer(ps_core_d->tty);
        }
    }

    INIT_COMPLETION(ps_plat_d->ldisc_reconfiged);
    ps_plat_d->flow_cntrl    = enable_flowctl;
    ps_plat_d->baud_rate     = baud_rate;
    ps_plat_d->ldisc_install = TTY_LDISC_RECONFIG;

    PS_PRINT_INFO("ldisc_install = %d\n", TTY_LDISC_RECONFIG);
    sysfs_notify(g_sysfs_hi110x_bfgx_etc, NULL, "install");
    timeleft = wait_for_completion_timeout(&ps_plat_d->ldisc_reconfiged, msecs_to_jiffies(HISI_LDISC_TIME));
    if (!timeleft)
    {
        PS_PRINT_ERR("hisi bfgx ldisc reconfig timeout\n");
        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_CFG_UART);

        return -EINVAL;
    }

    PS_PRINT_SUC("hisi bfgx ldisc reconfig succ\n");

    return 0;
}

/**
 * Prototype    : open_tty_drv_etc
 * Description  : called from PS Core when BT protocol stack drivers
 *                  registration,or FM/GNSS hal stack open FM/GNSS inode
 * input        : ps_plat_d
 * output       : return 0--> open tty uart is ok
 *                return !0-> open tty uart is false
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
int32 open_tty_drv_etc(void *pm_data)
{
    struct ps_plat_s *ps_plat_d = NULL;
    struct ps_core_s *ps_core_d;
    uint8  retry = OPEN_TTY_RETRY_COUNT;
    uint64 timeleft = 0;

    PS_PRINT_DBG("%s\n", __func__);

    if (unlikely(NULL == pm_data))
    {
        PS_PRINT_ERR("pm_data is NULL\n");
        return -EINVAL;
    }

    ps_plat_d = (struct ps_plat_s *)pm_data;
    ps_core_d = ps_plat_d->core_data;
    if (true == ps_core_d->tty_have_open)
    {
        PS_PRINT_DBG("hisi bfgx line discipline have installed\n");
        return 0;
    }

    reset_uart_rx_buf_etc();

    do {
        INIT_COMPLETION(ps_plat_d->ldisc_installed);
        ps_plat_d->ldisc_install = TTY_LDISC_INSTALL;
        if (g_b_ir_only_mode)
        {
            /*ir only mode use baudrate 921600*/
            ps_plat_d->baud_rate = IR_ONLY_BAUD_RATE;
        }

        PS_PRINT_INFO("ldisc_install = %d,baud=%lu\n", TTY_LDISC_INSTALL,ps_plat_d->baud_rate);
        sysfs_notify(g_sysfs_hi110x_bfgx_etc, NULL, "install");
        timeleft = wait_for_completion_timeout(&ps_plat_d->ldisc_installed, msecs_to_jiffies(HISI_LDISC_TIME));
        if (!timeleft)
        {
            PS_PRINT_ERR("hisi bfgx ldisc installation timeout\n");
            PS_BUG_ON(1);
            continue;
        }
        else
        {
            PS_PRINT_SUC("hisi bfgx line discipline install succ\n");
            ps_core_d->tty_have_open = true;
            return 0;
        }
    } while (retry--);

    CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_OPEN_UART);

    return -EPERM;
}
#ifdef BFGX_UART_DOWNLOAD_SUPPORT

int32 bfg_patch_recv(const uint8 *data, int32 count)
{
    int32 ret;

    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -EINVAL;
    }

    /*this function should be called after patch_init(uart),
      otherwise maybe null pointer*/
#if 0
    if (test_bit(UART_INIT_OK, &pm_data->patch_init_flag))
    {
        ret = uart_recv_data(data, count);
        return ret;
    }
#else
    ret = uart_recv_data(data, count);
#endif

    return ret;
}
/**
 * Prototype    : ps_patch_to_nomal
 * Description  : from download patch state to normal state
 * input        : ps_core_d
 * output       : not
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2013/12/06
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
void ps_patch_to_nomal(void)
{
    struct ps_core_s *ps_core_d;
    PS_PRINT_INFO("%s", __func__);
    ps_core_d = NULL;
    ps_get_core_reference_etc(&ps_core_d);
    if ((NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is NULL");
        return;
    }
    //ps_core_d->in_download_patch_state = false;
    /* init complation */
    //INIT_COMPLETION(ps_core_d->dev_3in1_opened);
    /* change function pointer, pointer to ps_core_recv_etc */
    tty_recv_etc = ps_core_recv_etc;
    /* init variable for rx data */
    ps_core_d->rx_pkt_total_len = 0;
    ps_core_d->rx_have_recv_pkt_len = 0;
    ps_core_d->rx_have_del_public_len = 0;
    ps_core_d->rx_decode_public_buf_ptr = ps_core_d->rx_public_buf_org_ptr;
    return;
}

/********************************************************************/
/**
 * Prototype    : ps_patch_write
 * Description  : functions called from pm drivers,used download patch data
 * input        : type, content
 * output       : no
 *
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/12
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
int32 ps_patch_write(uint8 *data, int32 count)
{
    struct ps_core_s *ps_core_d;
    int32 len;

    PS_PRINT_FUNCTION_NAME;

    ps_core_d = NULL;
    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely((NULL == ps_core_d) || (NULL == ps_core_d->tty)))
    {
        PS_PRINT_ERR(" tty unavailable to perform write");
        return -EINVAL;
    }
    /* write to uart */
    len = ps_write_tty_etc(ps_core_d, data, count);
    return len;
}

/**
 * Prototype    : ps_recv_patch
 * Description  : PS's pm receive function.this function is called when download patch.
 * input        : data -> recive data ptr   from UART TTY
 *                count -> recive data count from UART TTY
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
int32 ps_recv_patch(void *disc_data, const uint8 *data, int32 count)
{
    struct ps_core_s *ps_core_d;

    PS_PRINT_FUNCTION_NAME;

    ps_core_d = (struct ps_core_s *)disc_data;
    if (unlikely((NULL == data) || (NULL == disc_data) || (NULL == ps_core_d->ps_pm)||
        (NULL == ps_core_d->ps_pm->recv_patch)))
    {
        PS_PRINT_ERR(" received null from TTY ");
        return -EINVAL;
    }

#ifdef DEBUG_USE
    vfs_write(ps_core_d->rx_data_fp_patch, data, count, &ps_core_d->rx_data_fp_patch->f_pos);
#endif

    /* tx data to hw-pm */
    if (ps_core_d->ps_pm->recv_patch(data, count) < 0)
    {
        PS_PRINT_ERR(" %s-err", __func__);
        return -EPERM;
    }
    return 0;
}
#endif
int32 is_tty_open(void *pm_data)
{
    struct ps_plat_s *ps_plat_d = NULL;
    struct ps_core_s *ps_core_d;

    if (unlikely(NULL == pm_data))
    {
        PS_PRINT_ERR("pm_data is NULL");
        return -EINVAL;
    }

    ps_plat_d = (struct ps_plat_s *)pm_data;
    ps_core_d = ps_plat_d->core_data;

    if (false == ps_core_d->tty_have_open)
    {
        PS_PRINT_INFO("hisi bfgx line discipline have uninstalled\n");
        return -EINVAL;
    }
    return 0;
}
/**
 * Prototype    : release_tty_drv_etc
 * Description  : called from PS Core when BT protocol stack drivers
 *                  unregistration,or FM/GNSS hal stack close FM/GNSS inode
 * input        : ps_plat_d
 * output       : return 0--> open tty uart is ok
 *                return !0-> open tty uart is false
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2012/11/05
 *     Author       : wx144390
 *     Modification : Created function
 *
 */
int32 release_tty_drv_etc(void *pm_data)
{
    int32  error;
    struct ps_plat_s *ps_plat_d = NULL;
    struct tty_struct *tty = NULL;
    struct ps_core_s *ps_core_d;
    uint64 timeleft = 0;
    uint8  delay_times = RELEASE_DELAT_TIMES;

    PS_PRINT_INFO("%s\n", __func__);

    if (unlikely(NULL == pm_data))
    {
        PS_PRINT_ERR("pm_data is NULL");
        return -EINVAL;
    }

    ps_plat_d = (struct ps_plat_s *)pm_data;
    ps_core_d = ps_plat_d->core_data;

    if (false == ps_core_d->tty_have_open)
    {
        PS_PRINT_INFO("hisi bfgx line discipline have uninstalled, ignored\n");
        return 0;
    }

    ps_kfree_skb_etc(ps_core_d, TX_URGENT_QUEUE);
    ps_kfree_skb_etc(ps_core_d, TX_HIGH_QUEUE);
    ps_kfree_skb_etc(ps_core_d, TX_LOW_QUEUE);

    PS_PRINT_INFO("free tx sbk buf done!\n");

    /* clean all tx sk_buff */
    while(((ps_core_d->tx_urgent_seq.qlen)||(ps_core_d->tx_high_seq.qlen)||(ps_core_d->tx_low_seq.qlen))&&(delay_times))
    {
        msleep(10);
        delay_times --;
    }

    mutex_lock(&g_tty_mutex_etc);
    tty = ps_core_d->tty;
    if (tty)
    {   /* can be called before ldisc is installed */
        /* Flush any pending characters in the driver and discipline. */
        PS_PRINT_INFO(" %s--> into flush_buffer\n", __func__);
        tty_ldisc_flush(tty);
        tty_driver_flush_buffer(tty);
    }
    mutex_unlock(&g_tty_mutex_etc);

    INIT_COMPLETION(ps_plat_d->ldisc_uninstalled);
    ps_plat_d->ldisc_install = TTY_LDISC_UNINSTALL;

    PS_PRINT_INFO("ldisc_install = %d\n", TTY_LDISC_UNINSTALL);
    sysfs_notify(g_sysfs_hi110x_bfgx_etc, NULL, "install");

    timeleft = wait_for_completion_timeout(&ps_plat_d->ldisc_uninstalled, msecs_to_jiffies(HISI_LDISC_TIME));
    if (!timeleft)
    {
        PS_PRINT_ERR("hisi bfgx ldisc uninstall timeout\n");
        error = -ETIMEDOUT;
    }
    else
    {
        PS_PRINT_SUC("hisi bfgx line discipline uninstall succ\n");
        error = 0;
    }

    ps_core_d->tty_have_open = false;
    ps_plat_d->flow_cntrl    = FLOW_CTRL_ENABLE;
    ps_plat_d->baud_rate     = g_default_baud_rate;

    return error;
}

STATIC struct tty_ldisc_ops ps_ldisc_ops = {
    .magic          = TTY_LDISC_MAGIC,
    .name           = "n_ps",
    .open           = ps_tty_open,
    .close          = ps_tty_close,
    .receive_buf    = ps_tty_receive,
    .write_wakeup   = ps_tty_wakeup,
    .flush_buffer   = ps_tty_flush_buffer,
    .owner          = THIS_MODULE
};

int32 plat_uart_init_etc(void)
{
    mutex_init(&g_tty_mutex_etc);

#ifdef N_HW_BFG
    return tty_register_ldisc(N_HW_BFG, &ps_ldisc_ops);
#else
    return OAL_SUCC;
#endif
}

int32 plat_uart_exit_etc(void)
{
#ifdef N_HW_BFG
    return tty_unregister_ldisc(N_HW_BFG);
#else
    return OAL_SUCC;
#endif
}

