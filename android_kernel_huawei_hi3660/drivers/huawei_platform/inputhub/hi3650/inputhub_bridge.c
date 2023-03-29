/*
 *  drivers/misc/inputhub/inputhub_bridge.c
 *  Sensor Hub Channel Bridge
 *
 *  Copyright (C) 2013 Huawei, Inc.
 *  Author: huangjisong
 *
 */

#include "inputhub_bridge.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/notifier.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/hisi/hisi_mailbox.h>
#include "inputhub_route.h"
#include "rdr_sensorhub.h"
#include <linux/hisi/hisi_rproc.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include "inputhub_route.h"
#include <sensor_detect.h>
#include "sensor_info.h"
#include <linux/version.h>
#include <linux/pm_qos.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <soc_pmctrl_interface.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#include <linux/hisi/usb/hisi_usb.h>
#include <linux/hisi/hisi_syscounter.h>

#ifdef CONFIG_HUAWEI_DSM
#define SENSOR_DSM_CONFIG
#endif
#ifdef CONFIG_CONTEXTHUB_IDLE_32K
#include <linux/hisi/hisi_idle_sleep.h>
#endif

int (*api_inputhub_mcu_recv) (const char* buf,
                              unsigned int length) = 0;
/*called by inputhub_mcu module or test module.*/
extern int inputhub_route_init(void);
extern void inputhub_route_exit(void);
extern int sensor_set_cfg_data(void);
extern int send_fileid_to_mcu(void);
extern void emg_flush_logbuff(void);
extern void reset_logbuff(void);

extern int g_iom3_state;
extern struct completion sensorhub_rdr_completion;

static int isSensorMcuMode;	/*mcu power mode: 0 power off;  1 power on */
static struct notifier_block nb;
static struct completion send_complete;
struct completion iom3_reboot;
struct completion iom3_resume_mini;
struct completion iom3_resume_all;
struct CONFIG_ON_DDR* pConfigOnDDr = NULL;
/*lint -e550 -e715 -e773 */
#ifdef CONFIG_CONTEXTHUB_IDLE_32K
#define PERI_USED_TIMEOUT (jiffies + HZ/100)
#define LOWER_LIMIT 0
#define UPPER_LIMIT 255
#define BORDERLINE_UPPER_PROTECT(a,b)		(a ==b) ? a:(a+1)
#define BORDERLINE_LOWER_PROTECT(a,b)	( a == b) ? a:(a-1)
struct timer_list peri_timer;
static unsigned int peri_used_t;
static unsigned int peri_used;
spinlock_t peri_lock;

static void peri_used_timeout(unsigned long data)
{
	unsigned long flags;
	spin_lock_irqsave(&peri_lock, flags);
	pr_debug("[%s]used[%d],t[%d]\n", __func__ ,peri_used,peri_used_t);
	if(0 == peri_used) {
		int ret = hisi_idle_sleep_vote(ID_IOMCU, 0);
		if (ret)pr_err("[%s]hisi_idle_sleep_vote err\n", __func__);
		peri_used_t = 0;
	}
	spin_unlock_irqrestore(&peri_lock, flags);
}
#endif
void peri_used_init(void)
{
#ifdef CONFIG_CONTEXTHUB_IDLE_32K
	spin_lock_init(&peri_lock);
	setup_timer(&peri_timer, peri_used_timeout, 0);
#endif
}

void peri_used_request(void)
{
#ifdef CONFIG_CONTEXTHUB_IDLE_32K
	unsigned long flags;
	del_timer_sync(&peri_timer);
	spin_lock_irqsave(&peri_lock, flags);
	pr_debug("[%s]used[%d],t[%d]\n", __func__ ,peri_used,peri_used_t);
	if (0 != peri_used) {
		peri_used = BORDERLINE_UPPER_PROTECT(peri_used, UPPER_LIMIT);
		spin_unlock_irqrestore(&peri_lock, flags);
		return;
	}

	if (0 == peri_used_t) {
		int ret = hisi_idle_sleep_vote(ID_IOMCU, 1);
		if (ret)pr_err("[%s]hisi_idle_sleep_vote err\n", __func__);
	}else{/*just for pclist*/}

	peri_used = 1;
	peri_used_t = 1;

	spin_unlock_irqrestore(&peri_lock, flags);
#endif
}

void peri_used_release(void)
{
#ifdef CONFIG_CONTEXTHUB_IDLE_32K
	unsigned long flags;
	spin_lock_irqsave(&peri_lock, flags);
	pr_debug("[%s]used[%d]\n", __func__ ,peri_used);
	peri_used = BORDERLINE_LOWER_PROTECT(peri_used, LOWER_LIMIT);
	mod_timer(&peri_timer, PERI_USED_TIMEOUT);
	spin_unlock_irqrestore(&peri_lock, flags);
#endif
}
/*lint +e550 +e715 +e773 */
#ifdef CONFIG_IOM3_RECOVERY
BLOCKING_NOTIFIER_HEAD(iom3_recovery_notifier_list);
atomic_t iom3_rec_state = ATOMIC_INIT(IOM3_RECOVERY_UNINIT);
int iom3_power_state = ST_POWERON;
static struct delayed_work iom3_rec_work;
static struct workqueue_struct* iom3_rec_wq;
static struct wake_lock iom3_rec_wl;
static struct completion iom3_rec_done;
#endif

static void __iomem* iomcu_cfg_base;
extern unsigned long g_sensorhub_extend_dump_buff;
extern uint8_t* g_sensorhub_extend_dump_buff_remap;
extern uint32_t g_dump_extend_size;
extern uint32_t g_enable_dump;
#ifdef CONFIG_HUAWEI_CHARGER_SENSORHUB
extern struct coul_core_info_sh *g_di_coul_info_sh;
extern struct charge_core_info_sh *g_core_info_sh;
extern struct charge_device_info_sh *g_di_sh;
extern char sensor_chip_info[SENSOR_MAX][MAX_CHIP_INFO_LEN];
extern struct sensorhub_scene g_scens;
#endif
#ifdef CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT_SENSORHUB
extern struct uscp_device_info_sh *g_device_uscp;
#endif
#ifdef SENSOR_DSM_CONFIG
struct dsm_client* shb_dclient;
#endif

/*Note:this function is exported for 1102 check uart3 clk( contains of sensorhub subsystem) during resume initailly,
if other thread use this function,the state maybe not right because of no sync lock protect*/
int getSensorMcuMode(void)
{
    return isSensorMcuMode;
}
EXPORT_SYMBOL(getSensorMcuMode);

static void setSensorMcuMode(int mode)
{
    isSensorMcuMode = mode;
}
/*Note:this function is exported for 1102 check uart3 clk( contains of sensorhub subsystem) during resume initailly,
if other thread use this function,the state maybe not right because of no sync lock protect*/
int get_iomcu_power_state(void)
{
    return iom3_power_state;
}
EXPORT_SYMBOL(get_iomcu_power_state);

void restart_iom3(void)
{
    unsigned int reset_msg = RELOAD_IOM3_CMD;
    unsigned int boot_iom3 = STARTUP_IOM3_CMD;
    int ret = 0;

    ret =
        RPROC_ASYNC_SEND(HISI_RPROC_LPM3_MBX17, (mbox_msg_t*)&reset_msg,
                         1);

    if (ret)
    { hwlog_err("RPROC_ASYNC_SEND reset_msg error in %s\n", __func__); }

    msleep(10);

    ret =
        RPROC_ASYNC_SEND(HISI_RPROC_IOM3_MBX10, (mbox_msg_t*)&boot_iom3,
                         1);

    if (ret)
    { hwlog_err("RPROC_ASYNC_SEND boot_iom3 error in %s\n", __func__); }

    return;
}

int inputhub_mcu_recv(const char* buf, unsigned int length)
{
#ifdef CONFIG_IOM3_RECOVERY

    if (IOM3_RECOVERY_START == atomic_read(&iom3_rec_state))
    {
        hwlog_err("iom3 under recovery mode, ignore all recv data\n");
        return 0;
    }

#endif

    if (api_inputhub_mcu_recv != NULL)
    {
        return api_inputhub_mcu_recv(buf, length);
    }
    else
    {
        hwlog_err("---->error: api_inputhub_mcu_recv == NULL\n");
        __dmd_log_report(DSM_SHB_ERR_IOM7_READ, __func__,
                         "---->error: api_inputhub_mcu_recv == NULL\n");
        return -1;
    }
}

/*received data from mcu.*/
static int mbox_recv_notifier(struct notifier_block* nb, unsigned long len,
                              void* msg)
{
    /*
    	int i;

    	for (i = 0; i < len; ++i) {
    		hwlog_info("-------->msg[%d] = %#.8x\n", i, ((int *)msg)[i]);
    	}
    */
    inputhub_mcu_recv(msg, len * sizeof(int));	/*convert to bytes*/

    return 0;
}

int inputhub_mcu_connect(void)
{
    int ret = 0;
    /*connect to inputhub_route*/
    api_inputhub_mcu_recv = inputhub_route_recv_mcu_data;

    hwlog_info("----%s--->\n", __func__);

    nb.next = NULL;
    nb.notifier_call = mbox_recv_notifier;

    /* register the rx notify callback */
    ret = RPROC_MONITOR_REGISTER(HISI_RPROC_IOM3_MBX4, &nb);

    if (ret)
    { hwlog_info("%s:RPROC_MONITOR_REGISTER failed", __func__); }

    return 0;
}

int inputhub_mcu_disconnect(void)
{
    RPROC_PUT(HISI_RPROC_IOM3_MBX4);
    return 0;
}

#if 0
static inline unsigned int read_reg32(unsigned int phy_addr)
{
    unsigned int reg32_val = 0;
    unsigned int volatile* vir_addr =
        (unsigned int volatile*)ioremap(phy_addr, 4);

    if (vir_addr != NULL)
    {
        reg32_val = *vir_addr;
        iounmap(vir_addr);
    }
    else
    {
        hwlog_err("ioremap(%u) failed in %s!\n", phy_addr, __func__);
    }

    return reg32_val;
}
#endif

static int get_iomcu_cfg_base(void)
{
    struct device_node* np = NULL;

    if (iomcu_cfg_base == NULL)
    {
        np = of_find_compatible_node(NULL, NULL, "hisilicon,iomcuctrl");

        if (!np)
        {
            hwlog_err("can not find  iomcuctrl node !\n");
            return -1;
        }

        iomcu_cfg_base = of_iomap(np, 0);

        if (iomcu_cfg_base == NULL)
        {
            hwlog_err("get iomcu_cfg_base  error !\n");
            return -1;
        }
    }

    return 0;
}

/*extern void hisi_rdr_nmi_notify_iom3(void);*/
int sensorhub_img_dump(int type, void* buff, int size)
{
    /*hisi_rdr_nmi_notify_iom3(); */
    return 0;
}

#ifdef SENSOR_DSM_CONFIG
struct dsm_client_ops sensorhub_ops =
{
    .poll_state = NULL,
    .dump_func = sensorhub_img_dump,
};

static struct dsm_dev dsm_sensorhub =
{
    .name = "dsm_sensorhub",
    .device_name = NULL,
    .ic_name = NULL,
    .module_name = NULL,
    .fops = &sensorhub_ops,
    .buff_size = 1024,
};
#endif

int inputhub_mcu_send(const char* buf, unsigned int length)
{
    mbox_msg_len_t len = 0;
    int ret = -1;
	peri_used_request();
    len = (length + sizeof(mbox_msg_t) - 1) / (sizeof(mbox_msg_t));
    ret =
        RPROC_SYNC_SEND(HISI_RPROC_IOM3_MBX10, (mbox_msg_t*) buf, len,
                        NULL, 0);

    if (ret)
    {
        hwlog_err("RPROC_SYNC_SEND return %d.\n", ret);
        return -1;
    }
	peri_used_release();
    return ret;
}

static int g_boot_iom3 = STARTUP_IOM3_CMD;

void boot_iom3(void)
{
	int ret = 0;
	peri_used_request();
	ret = RPROC_ASYNC_SEND(HISI_RPROC_IOM3_MBX10,
	                   (mbox_msg_t*)&g_boot_iom3, 1);
	peri_used_release();
	if (ret)
	{ hwlog_err("RPROC_ASYNC_SEND error in %s\n", __func__); }
}

#ifdef CONFIG_IOM3_RECOVERY
int register_iom3_recovery_notifier(struct notifier_block* nb)
{
    return blocking_notifier_chain_register(&iom3_recovery_notifier_list,
                                            nb);
}

int iom3_rec_sys_callback(const pkt_header_t* head)
{
    int ret = 0;

    if (IOM3_RECOVERY_MINISYS == atomic_read(&iom3_rec_state))
    {
        if (ST_MINSYSREADY ==
            ((pkt_sys_statuschange_req_t*) head)->status)
        {
            hwlog_info("REC sys ready mini!\n");
            ret = send_fileid_to_mcu();

            if (ret)
                hwlog_err
                ("REC get sensors cfg data from dts fail,ret=%d, use default config data!\n",
                 ret);
            else
                hwlog_info
                ("REC get sensors cfg data from dts success!\n");
        }
        else if (ST_MCUREADY ==
                 ((pkt_sys_statuschange_req_t*) head)->status)
        {
            hwlog_info("REC mcu all ready!\n");
            ret = sensor_set_cfg_data();

            if (ret < 0)
                hwlog_err("REC sensor_chip_detect ret=%d\n",
                          ret);
            else
            { complete(&iom3_rec_done); }
        }
    }

    return 0;
}

static inline void show_iom3_stat(void)
{
    hwlog_err("CLK_SEL:0x%x,DIV0:0x%x,DIV1:0x%x,CLKSTAT0:0x%x, RSTSTAT0:0x%x\n",
              readl(iomcu_cfg_base + IOMCU_CLK_SEL),
              readl(iomcu_cfg_base + IOMCU_CFG_DIV0),
              readl(iomcu_cfg_base + IOMCU_CFG_DIV1),
              readl(iomcu_cfg_base + CLKSTAT0_OFFSET),
              readl(iomcu_cfg_base + RSTSTAT0_OFFSET));
}

#if 0
#define IOM3STAT_SLEEP_MASK	(0x2)
#define WAIT_IOM3_WFI_PERIOD	(100)
static int wait_for_iom3_wfi(int timeout)
{
    int count = (timeout > 0) ? timeout : 1;
    unsigned int reg32_val = 0;
    unsigned int volatile* vir_addr =
        (unsigned int volatile*)ioremap(IOM3STAT_ADDR, 4);

    if (!vir_addr)
    {
        hwlog_err("ioremap IOM3STAT_ADDR failed\n");
        return -1;
    }

    do
    {
        reg32_val = *vir_addr;

        if (reg32_val & IOM3STAT_SLEEP_MASK)
        { break; }
        else
        { msleep(WAIT_IOM3_WFI_PERIOD); }
    }
    while (--count > 0);

    iounmap(vir_addr);
    return count;
}
#endif

#define I2C_0_RST_VAL	(BIT(3))
static void reset_i2c_0_controller(void)
{
    unsigned long flags;

    local_irq_save(flags);
    writel(I2C_0_RST_VAL, iomcu_cfg_base + RSTEN0_OFFSET);
    udelay(5);
    writel(I2C_0_RST_VAL, iomcu_cfg_base + RSTDIS0_OFFSET);
    local_irq_restore(flags);

    return;
}

static void reset_charge(void)
{
    if (pConfigOnDDr == NULL) {
        hwlog_err ("pConfigOnDDr is NULL, maybe ioremap (%x) failed!\n", IOMCU_CONFIG_START);
        return;
    }
    pConfigOnDDr->g_di.fcp_stage = FCP_STAGE_DEFAUTL;
}

#define IOM3_REC_NEST_MAX		(5)
extern void operations_when_recovery_iom3(void);

extern void reset_sensor_power(void);

static void iom3_recovery_work(struct work_struct* work)
{
    int rec_nest_count = 0;
    int rc;
    u32 ack_buffer;
    u32 tx_buffer;

    hwlog_err("%s enter\n", __func__);
    wake_lock(&iom3_rec_wl);
    peri_used_request();
    wait_for_completion(&sensorhub_rdr_completion);
recovery_iom3:

    if (rec_nest_count++ > IOM3_REC_NEST_MAX)
    {
        hwlog_err("unlucky recovery iom3 times exceed limit\n");
        atomic_set(&iom3_rec_state, IOM3_RECOVERY_FAILED);
        blocking_notifier_call_chain(&iom3_recovery_notifier_list,
                                     IOM3_RECOVERY_FAILED, NULL);
        atomic_set(&iom3_rec_state, IOM3_RECOVERY_IDLE);
	peri_used_release();
        wake_unlock(&iom3_rec_wl);
        hwlog_err("%s exit\n", __func__);
        return;
    }

    show_iom3_stat();	/*only for IOM3 debug*/

    reset_sensor_power();
    //reload iom3 system
    tx_buffer = RELOAD_IOM3_CMD;
    rc = RPROC_ASYNC_SEND(HISI_RPROC_LPM3_MBX17, &tx_buffer, 1);

    if (rc)
    {
        hwlog_err("RPROC reload iom3 failed %d, nest_count %d\n", rc,
                  rec_nest_count);
        goto recovery_iom3;
    }

    show_iom3_stat();	/*only for IOM3 debug*/
    reset_i2c_0_controller();
    reset_logbuff();
    write_ramdump_info_to_sharemem();
    write_timestamp_base_to_sharemem();
    reset_charge();

    msleep(5);
    atomic_set(&iom3_rec_state, IOM3_RECOVERY_MINISYS);

    /*startup iom3 system*/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
    INIT_COMPLETION(iom3_rec_done);
#else
    reinit_completion(&iom3_rec_done);
#endif
    tx_buffer = STARTUP_IOM3_CMD;
    rc = RPROC_SYNC_SEND(HISI_RPROC_IOM3_MBX10, &tx_buffer, 1, &ack_buffer,
                         1);

    if (rc)
    {
        hwlog_err("RPROC start iom3 failed %d, nest_count %d\n", rc,
                  rec_nest_count);
        goto recovery_iom3;
    }

    hwlog_err("RPROC restart iom3 success\n");
    show_iom3_stat();	/*only for IOM3 debug*/

    /*dynamic loading*/
    if (!wait_for_completion_timeout(&iom3_rec_done, 5 * HZ))
    {
        hwlog_err("wait for iom3 system ready timeout\n");
        msleep(1000);
        goto recovery_iom3;
    }

    /*repeat send cmd*/
    msleep(100);		/*wait iom3 finish handle config-data*/
    atomic_set(&iom3_rec_state, IOM3_RECOVERY_DOING);
    hwlog_err("%s doing\n", __func__);
    blocking_notifier_call_chain(&iom3_recovery_notifier_list,
                                 IOM3_RECOVERY_DOING, NULL);
    operations_when_recovery_iom3();
    blocking_notifier_call_chain(&iom3_recovery_notifier_list, IOM3_RECOVERY_3RD_DOING, NULL);	/*recovery pdr*/
    hwlog_err("%s pdr recovery\n", __func__);
    atomic_set(&iom3_rec_state, IOM3_RECOVERY_IDLE);
    wake_unlock(&iom3_rec_wl);
    hwlog_err("%s finish recovery\n", __func__);
    blocking_notifier_call_chain(&iom3_recovery_notifier_list,
                                 IOM3_RECOVERY_IDLE, NULL);
    hwlog_err("%s exit\n", __func__);
    peri_used_release();
    return;
}

int iom3_need_recovery(int modid, exp_source_t f)
{
    int ret = 0;
    int old_state;
    old_state = atomic_cmpxchg(&iom3_rec_state, IOM3_RECOVERY_IDLE,
                               IOM3_RECOVERY_START);
    hwlog_err("recovery prev state %d, modid 0x%x\n", old_state, modid);

    if (old_state == IOM3_RECOVERY_IDLE)  	/*prev state is IDLE start recovery progress*/
    {
        if (modid == SENSORHUB_MODID)
        {
            __dmd_log_report(DSM_SHB_ERR_IOM7_WDG, __func__,
                             "sensorhub crash, trigger rdr dump\n");
        }

        wake_lock_timeout(&iom3_rec_wl, 10 * HZ);
        blocking_notifier_call_chain(&iom3_recovery_notifier_list,
                                     IOM3_RECOVERY_START, NULL);

        if (f > SH_FAULT_INTERNELFAULT)
        {
            pConfigOnDDr->dump_config.reason = (uint8_t)f;
        }

        //flush old logs
        emg_flush_logbuff();

        //write extend dump config
        if (g_enable_dump && g_dump_extend_size && !g_sensorhub_extend_dump_buff)
        {
            g_sensorhub_extend_dump_buff = kmalloc(g_dump_extend_size, GFP_KERNEL);
            hwlog_warn("%s alloc pages logic %x phy addr %x\n", __func__, g_sensorhub_extend_dump_buff, virt_to_phys(g_sensorhub_extend_dump_buff));

            if (g_sensorhub_extend_dump_buff)
            {
                pConfigOnDDr->dump_config.ext_dump_addr = virt_to_phys(g_sensorhub_extend_dump_buff);
                pConfigOnDDr->dump_config.ext_dump_size = g_dump_extend_size;
                barrier();
            }
        }

#ifdef CONFIG_HISI_BB
        rdr_system_error(modid, 0, 0);
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
        INIT_COMPLETION(sensorhub_rdr_completion);
#else
        reinit_completion(&sensorhub_rdr_completion);
#endif
        __send_nmi();
        notify_rdr_thread();
        queue_delayed_work(iom3_rec_wq, &iom3_rec_work, 0);
    }

    return ret;
}
#else
int iom3_need_recovery(int modid, exp_source_t f)
{
    hwlog_err("[WARNING:] !!! recovery not support\n");
    return 0;
}
#endif

int is_sensorhub_disabled(void)
{
    int len = 0;
    struct device_node* sh_node = NULL;
    const char* sh_status = NULL;

    sh_node =
        of_find_compatible_node(NULL, NULL, "huawei,sensorhub_status");

    if (!sh_node)
    {
        hwlog_err("%s, can not find node  sensorhub_status n",
                  __func__);
        return -1;
    }

    sh_status = of_get_property(sh_node, "status", &len);

    if (!sh_status)
    {
        hwlog_err("%s, can't find property status\n", __func__);
        return -1;
    }

    if (strstr(sh_status, "ok"))
    {
        hwlog_info("%s, sensorhub enabled!\n", __func__);
        return 0;
    }
    else
    {
        hwlog_info("%s, sensorhub disabled!\n", __func__);
        return -1;
    }
}

int write_defualt_config_info_to_sharemem(void)
{
    if (!pConfigOnDDr)
    {
        pConfigOnDDr =
            (struct CONFIG_ON_DDR*)ioremap_wc(IOMCU_CONFIG_START,
                                              IOMCU_CONFIG_SIZE);
    }

    if (pConfigOnDDr == NULL)
    {
        hwlog_err("ioremap (%x) failed in %s!\n", IOMCU_CONFIG_START,
                  __func__);
        return -1;
    }

    memset(pConfigOnDDr, 0, sizeof(struct CONFIG_ON_DDR));
    pConfigOnDDr->LogBuffCBBackup.mutex = 0;
    pConfigOnDDr->log_level = INFO_LEVEL;
#ifdef CONFIG_HUAWEI_CHARGER_SENSORHUB
    if (g_core_info_sh && g_di_sh && g_di_coul_info_sh) {
        memcpy(&pConfigOnDDr->g_core_info, g_core_info_sh, sizeof(struct charge_core_info_sh));
        memcpy(&pConfigOnDDr->g_di, g_di_sh, sizeof(struct charge_device_info_sh));
	memcpy(&pConfigOnDDr->g_di_coul_info_sh, g_di_coul_info_sh, sizeof(struct coul_core_info_sh));
	memcpy(&pConfigOnDDr->scenes, &g_scens, sizeof(struct sensorhub_scene));
    }
#endif
#ifdef CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT_SENSORHUB
    if (g_device_uscp) {
	memcpy(&pConfigOnDDr->g_di_uscp, g_device_uscp, sizeof(struct uscp_device_info_sh));
    }
#endif
    return 0;
}

void write_timestamp_base_to_sharemem(void)
{
	u64 syscnt;
	u64 kernel_ns;
	struct timespec64 ts;

	get_monotonic_boottime64(&ts);
	syscnt = hisi_get_syscount();
	kernel_ns = (u64)(ts.tv_sec * NSEC_PER_SEC) + (u64)ts.tv_nsec;

	pConfigOnDDr->timestamp_base.syscnt = syscnt;
	pConfigOnDDr->timestamp_base.kernel_ns = kernel_ns;

	return;
}

static int inputhub_mcu_init(void)
{
    int ret;


    if (is_sensorhub_disabled())
    { return -1; }

	peri_used_init();

    if (write_defualt_config_info_to_sharemem())
    { return -1; }

    write_timestamp_base_to_sharemem();

    ret = rdr_sensorhub_init();

    if (ret < 0)
    {
        hwlog_err("%s rdr_sensorhub_init ret=%d\n", __func__, ret);
    }

    if (get_iomcu_cfg_base())
    { return -1; }

    init_completion(&send_complete);
#ifdef CONFIG_IOM3_RECOVERY
    iom3_rec_wq = create_singlethread_workqueue("iom3_rec_wq");

    if (!iom3_rec_wq)
    {
        hwlog_err
        ("--------------------> faild in create iom3 wq in %s!\n",
         __func__);
        return -1;
    }

    INIT_DELAYED_WORK(&iom3_rec_work, iom3_recovery_work);
    init_completion(&iom3_rec_done);
    wake_lock_init(&iom3_rec_wl, WAKE_LOCK_SUSPEND, "iom3_rec_wl");
#endif
#ifdef SENSOR_DSM_CONFIG
    shb_dclient = dsm_register_client(&dsm_sensorhub);
#endif
    init_completion(&iom3_reboot);
    init_completion(&iom3_resume_mini);
    init_completion(&iom3_resume_all);

    sensor_redetect_init();
    ret = inputhub_route_init();  
    inputhub_mcu_connect();
    boot_iom3();
    setSensorMcuMode(1);
    hwlog_info("----%s--->\n", __func__);
    return ret;
}

static void __exit inputhub_mcu_exit(void)
{
    inputhub_route_exit();
    RPROC_PUT(HISI_RPROC_IOM3_MBX10);
}

late_initcall(inputhub_mcu_init);
module_exit(inputhub_mcu_exit);

MODULE_AUTHOR("Input Hub <smartphone@huawei.com>");
MODULE_DESCRIPTION("input hub bridge");
MODULE_LICENSE("GPL");
