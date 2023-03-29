/*
 * drivers/char/watchdog/sp805-wdt.c
 *
 * Watchdog driver for ARM SP805 watchdog module
 *
 * Copyright (C) 2010 ST Microelectronics
 * Viresh Kumar <viresh.linux@gmail.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2 or later. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/device.h>
#include <linux/resource.h>
#include <linux/amba/bus.h>
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#include <linux/syscore_ops.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <asm/arch_timer.h>

#include <linux/hisi/hisi_sp805_wdt.h>
#include <linux/hisi/hisi_mailbox.h>
#include <linux/hisi/hisi_mailbox_dev.h>
#include <linux/hisi/hisi_rproc.h>
#include <linux/hisi/rdr_pub.h>
#include <soc_rtctimerwdt_interface.h>
#include <linux/../../kernel/sched/sched.h>
#include <linux/../../kernel/workqueue_internal.h>
#include <linux/hisi/rdr_hisi_platform.h>
#include <linux/version.h>
#include <linux/hisi/util.h>
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG HISI_SP805_WDT_TAG

/* default timeout in seconds */
#define DEFAULT_TIMEOUT     60
/*it is used to adjust the time of feeding wdg,
	defined 3,  So TIMEOUT/2/3 to feed */
#define WDG_FEED_MOMENT_ADJUST	3
#define MODULE_NAME		"sp805-wdt"


/* watchdog register offsets and masks */
#define WDTLOAD         SOC_RTCTIMERWDT_WDLOAD_ADDR(0)
#define LOAD_MIN    0x00000001
#define LOAD_MAX    0xFFFFFFFF
#define WDTVALUE        SOC_RTCTIMERWDT_WDVALUE_ADDR(0)
#define WDTCONTROL      SOC_RTCTIMERWDT_WDCONTROL_ADDR(0)
    /* control register masks */
#define INT_ENABLE  (1 << SOC_RTCTIMERWDT_WDCONTROL_inten_START)
#define RESET_ENABLE    (1 << SOC_RTCTIMERWDT_WDCONTROL_resen_START)
#define WDTINTCLR       SOC_RTCTIMERWDT_WDINTCLR_ADDR(0)
#define WDTRIS          SOC_RTCTIMERWDT_WDRIS_ADDR(0)
#define INT_MASK    (1 << SOC_RTCTIMERWDT_WDRIS_wdogris_START)
#define WDTLOCK         SOC_RTCTIMERWDT_WDLOCK_ADDR(0)
#define UNLOCK      0x1ACCE551
#define LOCK        0x00000001

/* wdt aging test, default 120s to be reset */
#define AGING_WDT_TIMEOUT 240

#undef CONFIG_HISI_AGING_WDT
#ifdef CONFIG_HISI_AGING_WDT
static bool disable_wdt_flag;
#endif
static struct sp805_wdt *g_wdt;
static struct syscore_ops sp805_wdt_syscore_ops;
void __iomem *wdt_base;

/**
 * struct sp805_wdt: sp805 wdt device structure
 * @wdd: instance of struct watchdog_device
 * @lock: spin lock protecting dev structure and io access
 * @base: base address of wdt
 * @clk: clock structure of wdt
 * @adev: amba device structure of wdt
 * @kick_time: kick dog time
 * @hisi_wdt_delayed_work: kick dog work
 * @status: current status of wdt
 * @load_val: load value to be set for current timeout
 * @timeout: current programmed timeout
 */
struct sp805_wdt {
	struct watchdog_device		wdd;
	spinlock_t			lock;
	void __iomem			*base;
	struct clk			*clk;
	struct amba_device		*adev;
	unsigned int			load_val;
	unsigned int kick_time;
	struct delayed_work hisi_wdt_delayed_work;
	struct workqueue_struct *hisi_wdt_wq;
	bool active;
	unsigned int timeout;
};

rdr_arctimer_t g_rdr_arctimer_record;

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout,
		"Set to 1 to keep watchdog running after device release");

/* This routine finds load value that will reset system in required timout */
static int wdt_setload(struct watchdog_device *wdd, unsigned int timeout)
{
	struct sp805_wdt *wdt = watchdog_get_drvdata(wdd);
	u64 load, rate;

	rate = clk_get_rate(wdt->clk);
	if (!rate) {
		return 0;
	}

	/*
	 * sp805 runs counter with given value twice, after the end of first
	 * counter it gives an interrupt and then starts counter again. If
	 * interrupt already occurred then it resets the system. This is why
	 * load is half of what should be required.
	 */

	wdt->wdd.timeout = timeout;

	load = div_u64(rate, 2) * timeout - 1;

	load = (load > LOAD_MAX) ? LOAD_MAX : load;
	load = (load < LOAD_MIN) ? LOAD_MIN : load;

	spin_lock(&wdt->lock);
	wdt->load_val = load;
	/* roundup timeout to closest positive integer value */

	wdt->timeout = div_u64((load + 1) * 2 + (rate / 2), rate);
	spin_unlock(&wdt->lock);
	dev_info(&wdt->adev->dev, "default-timeout=%d, load_val=0x%x\n",
		 timeout, wdt->load_val);

	return 0;
}

/* returns number of seconds left for reset to occur */
static unsigned int wdt_timeleft(struct watchdog_device *wdd)
{
	struct sp805_wdt *wdt = watchdog_get_drvdata(wdd);
	u64 load, rate;

	rate = clk_get_rate(wdt->clk);
	if (!rate) {
		return DEFAULT_TIMEOUT;
	}

	spin_lock(&wdt->lock);
	load = readl_relaxed(wdt->base + WDTVALUE);

	/*If the interrupt is inactive then time left is WDTValue + WDTLoad. */
	if (!(readl_relaxed(wdt->base + WDTRIS) & INT_MASK))
		load += wdt->load_val + 1;
	spin_unlock(&wdt->lock);

	return div_u64(load, rate);
}

static int wdt_config(struct watchdog_device *wdd, bool ping)
{
	struct sp805_wdt *wdt = watchdog_get_drvdata(wdd);
	int ret;

	if (!ping) {

		ret = clk_prepare_enable(wdt->clk);
		if (ret) {
			dev_err(&wdt->adev->dev, "clock enable fail");
			return ret;
		}
	}

	spin_lock(&wdt->lock);

	writel_relaxed(UNLOCK, wdt->base + WDTLOCK);
	writel_relaxed(wdt->load_val, wdt->base + WDTLOAD);

	if (!ping) {
		writel_relaxed(INT_MASK, wdt->base + WDTINTCLR);
		writel_relaxed(INT_ENABLE | RESET_ENABLE, wdt->base + WDTCONTROL);
		wdt->active = true;
	}

	writel_relaxed(LOCK, wdt->base + WDTLOCK);

	/* Flush posted writes. */
	readl_relaxed(wdt->base + WDTLOCK);
	spin_unlock(&wdt->lock);

	return 0;
}

static int wdt_ping(struct watchdog_device *wdd)
{
	return wdt_config(wdd, true);
}

/* enables watchdog timers reset */
static int wdt_enable(struct watchdog_device *wdd)
{
	return wdt_config(wdd, false);
}

/* disables watchdog timers reset */
static int wdt_disable(struct watchdog_device *wdd)
{
	struct sp805_wdt *wdt = watchdog_get_drvdata(wdd);

	spin_lock(&wdt->lock);

	writel_relaxed(UNLOCK, wdt->base + WDTLOCK);
	writel_relaxed(0, wdt->base + WDTCONTROL);
	writel_relaxed(LOCK, wdt->base + WDTLOCK);

	/* Flush posted writes. */
	readl_relaxed(wdt->base + WDTLOCK);
	wdt->active = false;
	spin_unlock(&wdt->lock);

	clk_disable_unprepare(wdt->clk);

	return 0;
}

static const struct watchdog_info wdt_info = {
	.options = WDIOF_MAGICCLOSE | WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING,
	.identity = MODULE_NAME,
};

static const struct watchdog_ops wdt_ops = {
	.owner		= THIS_MODULE,
	.start		= wdt_enable,
	.stop		= wdt_disable,
	.ping		= wdt_ping,
	.set_timeout	= wdt_setload,
	.get_timeleft	= wdt_timeleft,
};

unsigned long get_wdt_expires_time(void)
{
	if (NULL == g_wdt)
		return 0;

	return g_wdt->hisi_wdt_delayed_work.timer.expires;
}

unsigned int get_wdt_kick_time(void)
{
	if (NULL == g_wdt)
		return 0;

	return g_wdt->kick_time;
}

#ifdef CONFIG_HISI_AGING_WDT
void set_wdt_disable_flag(bool iflag)
{
	disable_wdt_flag = iflag;
	return;
}
#endif

static int sp805_wdt_rdr_init(void)
{
	struct rdr_exception_info_s einfo;
	unsigned int ret;

	memset(&einfo, 0, sizeof(struct rdr_exception_info_s)); /* unsafe_function_ignore: memset */
	einfo.e_modid = MODID_AP_S_WDT;
	einfo.e_modid_end = MODID_AP_S_WDT;
	einfo.e_process_priority = RDR_ERR;
	einfo.e_reboot_priority = RDR_REBOOT_NOW;
	einfo.e_notify_core_mask = RDR_AP;
	einfo.e_reset_core_mask = RDR_AP;
	einfo.e_from_core = RDR_AP;
	einfo.e_reentrant = (u32)RDR_REENTRANT_DISALLOW;
	einfo.e_exce_type = AP_S_AWDT;
	einfo.e_upload_flag = (u32)RDR_UPLOAD_YES;
	memcpy(einfo.e_from_module, "ap wdt", sizeof("ap wdt")); /* unsafe_function_ignore: memcpy */
	memcpy(einfo.e_desc, "ap wdt", sizeof("ap wdt")); /* unsafe_function_ignore: memcpy */

	ret = rdr_register_exception(&einfo);
	if (ret != MODID_AP_S_WDT) {
		return -1;
	}

	return 0;
}

void rdr_arctimer_register_read(rdr_arctimer_t *arctimer)
{
	if (NULL == arctimer)
		return;

	/* virtual time control register */
	asm volatile("mrs %0,  cntv_ctl_el0" : "=r" (arctimer->cntv_ctl_el0));
	/* virtual time value register */
	asm volatile("mrs %0, cntv_tval_el0" : "=r" (arctimer->cntv_tval_el0));
	/* virtual time compare register */
	asm volatile("mrs %0, cntv_cval_el0" : "=r" (arctimer->cntv_cval_el0));
	/* virtual time count register */
	asm volatile("mrs %0, cntvct_el0" : "=r" (arctimer->cntvct_el0));
	/* physical time control register */
	asm volatile("mrs %0, cntp_ctl_el0" : "=r" (arctimer->cntp_ctl_el0));
	/* physical time value register */
	asm volatile("mrs %0, cntp_tval_el0" : "=r" (arctimer->cntp_tval_el0));
	/* physical time compare register */
	asm volatile("mrs %0, cntp_cval_el0" : "=r" (arctimer->cntp_cval_el0));
	/* physical time count register */
	asm volatile("mrs %0, cntpct_el0" : "=r" (arctimer->cntpct_el0));

	/* counter time frequency register */
	asm volatile("mrs %0, cntfrq_el0" : "=r" (arctimer->cntfrq_el0));
}

void rdr_archtime_register_print(rdr_arctimer_t *arctimer, bool after)
{
	if (unlikely(NULL == arctimer))
		return;

	pr_err("<%s>cntv_ctl_el0 0x%x cntv_tval_el0 0x%x cntv_cval_el0 0x%llx cntvct_el0 0x%llx, "
		"cntp_ctl_el0 0x%x cntp_tval_el0 0x%x cntp_cval_el0 0x%llx cntpct_el0 0x%llx "
		"cntfrq_el0 0x%x\n",
		(true == after) ? "after" : "before", arctimer->cntv_ctl_el0, arctimer->cntv_tval_el0,
		arctimer->cntv_cval_el0, arctimer->cntvct_el0, arctimer->cntp_ctl_el0,
		arctimer->cntp_tval_el0, arctimer->cntp_cval_el0, arctimer->cntpct_el0, arctimer->cntfrq_el0);
}

void sp805_wdt_dump(void)
{
	rdr_arctimer_t rdr_arctimer;
	struct sp805_wdt *wdt = g_wdt;
	struct timer_list *timer;

	rdr_arctimer_register_read(&rdr_arctimer);

#ifdef CONFIG_SCHED_INFO
	if (current)
		pr_crit("current process last_arrival clock %llu last_queued clock %llu, "
			"printk_time is %llu, 32k_abs_timer_value is %llu.\n",
			current->sched_info.last_arrival,
			current->sched_info.last_queued,
			hisi_getcurtime(),
			get_32k_abs_timer_value()
		);
#endif

	rdr_archtime_register_print(&g_rdr_arctimer_record, false);
	rdr_archtime_register_print(&rdr_arctimer, true);

	if (NULL == wdt)
		return;

	pr_crit("work_busy 0x%x, latest kick slice %llu.\n",
		work_busy(&(wdt->hisi_wdt_delayed_work.work)),
		rdr_get_last_wdt_kick_slice());

	/* check if the watchdog work timer in running state */
	timer = &(wdt->hisi_wdt_delayed_work.timer);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 76))
	pr_crit("timer 0x%p: next 0x%p pprev 0x%p expires %lu jiffies %lu "
		"sec_to_jiffies %lu flags 0x%x slacks %d.\n",
		timer, timer->entry.next, timer->entry.pprev, timer->expires,
		jiffies, msecs_to_jiffies(1000), timer->flags, timer->slack
		);
#else
	pr_crit("timer 0x%p: next 0x%p pprev 0x%p expires %lu jiffies %lu "
		"sec_to_jiffies %lu flags 0x%x.\n",
		timer, timer->entry.next, timer->entry.pprev, timer->expires,
		jiffies, msecs_to_jiffies(1000), timer->flags);
#endif
	return;
}

static void hisi_wdt_mond(struct work_struct *work)
{
	struct sp805_wdt *wdt = container_of(work, struct sp805_wdt,
					     hisi_wdt_delayed_work.work);
	bool ret;
	u64 kickslice = get_32k_abs_timer_value();
	struct rq *rq;/*lint !e578 */

#ifdef CONFIG_HISI_AGING_WDT
	if (true == disable_wdt_flag) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 76))
		dev_err(wdt->wdd.dev, "disable wdt ok!!!!!\n");
#else
		dev_err(&wdt->adev->dev, "disable wdt ok!!!!!\n");
#endif
		return;
	}
#endif
	wdt_ping(&wdt->wdd);

	rdr_set_wdt_kick_slice(kickslice);

	rq = (struct rq *)cpu_rq(get_cpu());/*lint !e64 !e507*/
	put_cpu();

	if (cpu_online(0)) {
		ret = queue_delayed_work_on(0, wdt->hisi_wdt_wq,
				      &wdt->hisi_wdt_delayed_work,
				      msecs_to_jiffies(wdt->kick_time * 1000));
	} else {
		ret = queue_delayed_work(wdt->hisi_wdt_wq,
				   &wdt->hisi_wdt_delayed_work,
				   msecs_to_jiffies(wdt->kick_time * 1000));
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 76))
	dev_info(wdt->wdd.dev, "watchdog kick now 32K %llu rqclock %llu ret %d\n",
		kickslice, (rq ? rq->clock : 0), ret);
#else
	dev_info(&wdt->adev->dev, "watchdog kick now 32K %llu rqclock %llu ret %d\n",
		kickslice, (rq ? rq->clock : 0), ret);
#endif

	watchdog_check_hardlockup_sp805();

	return;
}

#ifdef CONFIG_HISI_RPROC
static void m3_wdt_timeout_notify(unsigned int default_timeout)
{
	int msg[2], ret;

/* ask mcu reset its wdt timeout value  start */
/*
如下PSCI_MSG_TYPE_M3_WDTTIMEOUT 等宏定义，如下三个文件要保持一致:
kernel\drivers\watchdog\sp805_wdt.c
kernel\drivers\hisi\mntn\blackbox\platform_lpm3\rdr_hisi_lpm3.c
vendor\hisi\confidential\lpmcu\include\psci.h
*/
#include <linux/hisi/ipc_msg.h>
#define PSCI_MSG_TYPE_M3_WDTTIMEOUT IPC_CMD(OBJ_AP, OBJ_LPM3, CMD_INQUIRY, 1)
	msg[0] = PSCI_MSG_TYPE_M3_WDTTIMEOUT;
	msg[1] = default_timeout;
	ret = RPROC_ASYNC_SEND(HISI_RPROC_LPM3_MBX17, (mbox_msg_t *) msg, 2);
	if (ret != 0) {
		pr_err
		    ("RPROC_ASYNC_SEND failed! return 0x%x, msg:(0x%x 0x%x)\n",
		     ret, msg[0], msg[1]);
	}
/* end */
}
#else
static inline void m3_wdt_timeout_notify(unsigned int default_timeout){return;}
#endif

static int sp805_wdt_get_timeout(struct amba_device *adev, unsigned int *default_timeout)
{
	struct device_node *np;
	int ret;

	np = of_find_compatible_node(NULL, NULL, "arm,sp805");
	if (IS_ERR(np)) {
		dev_err(&adev->dev, "Can not find sp805 node\n");
		return (-ENOENT);
	}

	ret =
	    of_property_read_u32_index(np, "default-timeout", 0,
				       default_timeout);
	if (ret) {
		dev_warn(&adev->dev,
			"find default-timeout property fail, Use the default value: %ds\n",
			DEFAULT_TIMEOUT);
		*default_timeout = DEFAULT_TIMEOUT;
	}
	return 0;
}

/*lint -save -e429*/
static int sp805_wdt_probe(struct amba_device *adev, const struct amba_id *id)
{
	struct sp805_wdt *wdt;
	int ret = 0;

	unsigned int default_timeout;

	dev_info(&adev->dev, "sp805_wdt_probe enter \n");
	wdt = devm_kzalloc(&adev->dev, sizeof(*wdt), GFP_KERNEL);
	if (!wdt) {
		dev_warn(&adev->dev, "Kzalloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	wdt->base = devm_ioremap_resource(&adev->dev, &adev->res);
	if (IS_ERR(wdt->base)) {
		dev_warn(&adev->dev, "base is error\n");
		return PTR_ERR(wdt->base);
	}

	wdt_base = wdt->base;
	wdt->clk = devm_clk_get(&adev->dev, NULL);
	if (IS_ERR(wdt->clk)) {
		dev_warn(&adev->dev, "Clock not found\n");
		ret = PTR_ERR(wdt->clk);
		goto err;
	}

	wdt->adev = adev;
	wdt->wdd.info = &wdt_info;
	wdt->wdd.ops = &wdt_ops;

	spin_lock_init(&wdt->lock);
	watchdog_set_nowayout(&wdt->wdd, nowayout);
	watchdog_set_drvdata(&wdt->wdd, wdt);

	ret = sp805_wdt_get_timeout(adev, &default_timeout);
	if (ret) {
		goto err;
	}

#ifdef CONFIG_HISI_AGING_WDT
	if (check_himntn(HIMNTN_AGING_WDT)) {
		wdt_setload(&wdt->wdd, AGING_WDT_TIMEOUT);
	} else {
		wdt_setload(&wdt->wdd, default_timeout);
	}
#else
	wdt_setload(&wdt->wdd, default_timeout);
#endif

	m3_wdt_timeout_notify(default_timeout);

	if ((default_timeout >> 1) < WDG_FEED_MOMENT_ADJUST) {
		wdt->kick_time = (default_timeout >> 1) - 1;
	} else {
		/* minus 1 from the total */
		wdt->kick_time =
		    ((default_timeout >> 1) - 1) / WDG_FEED_MOMENT_ADJUST;
	}

	wdt_ping(&wdt->wdd);

#ifdef CONFIG_HISI_AGING_WDT
	if (check_himntn(HIMNTN_AGING_WDT)) {
		set_wdt_disable_flag(true);
	}
#endif

	INIT_DELAYED_WORK(&wdt->hisi_wdt_delayed_work, hisi_wdt_mond);

	wdt->hisi_wdt_wq = alloc_workqueue("wdt_wq", WQ_MEM_RECLAIM | WQ_HIGHPRI, 1);
	if (NULL == wdt->hisi_wdt_wq) {
		dev_err(&adev->dev, "alloc workqueue failed\n");
		ret = -ENOMEM;
		goto err;
	}

	ret = watchdog_register_device(&wdt->wdd);
	if (ret) {
		dev_err(&adev->dev, "watchdog_register_device() failed: %d\n",
				ret);
		goto err2;
	}
/*    schedule_delayed_work(&wdt->hisi_wdt_delayed_work, 0);*/
	if (cpu_online(0)) {
		queue_delayed_work_on(0, wdt->hisi_wdt_wq,
				      &wdt->hisi_wdt_delayed_work, 0);
	} else {
		queue_delayed_work(wdt->hisi_wdt_wq,
				   &wdt->hisi_wdt_delayed_work, 0);
	}
	wdt_enable(&wdt->wdd);

	g_wdt = wdt;
	register_syscore_ops(&sp805_wdt_syscore_ops);

	amba_set_drvdata(adev, wdt);

	if (sp805_wdt_rdr_init()) {
		dev_err(&adev->dev, " register sp805_wdt_rdr_init failed.\n");
	}

	watchdog_lockup_panic_config();
	watchdog_set_thresh((int)(default_timeout >> 1));

	dev_info(&adev->dev, "registration successful\n");
	return 0;

err2:
	if (wdt->hisi_wdt_wq != NULL) {
		destroy_workqueue(wdt->hisi_wdt_wq);
		wdt->hisi_wdt_wq = NULL;
	}
err:
	dev_err(&adev->dev, "Probe Failed!!!\n");
	return ret;
}
/*lint -restore*/

static int sp805_wdt_remove(struct amba_device *adev)
{
	struct sp805_wdt *wdt = amba_get_drvdata(adev);

	cancel_delayed_work(&wdt->hisi_wdt_delayed_work);
	destroy_workqueue(wdt->hisi_wdt_wq);
	wdt->hisi_wdt_wq = NULL;
	watchdog_unregister_device(&wdt->wdd);
	watchdog_set_drvdata(&wdt->wdd, NULL);

	return 0;
}


static int sp805_wdt_suspend(void)
{
	struct sp805_wdt *wdt = g_wdt;
	int ret = 0;

	pr_info("%s+.\n", __func__);

	if (watchdog_active(&wdt->wdd) || wdt->active) {
#ifdef CONFIG_HISI_SR_AP_WATCHDOG
		/* delay the disable operation to the lpm3 for the case
			of system failure in the suspend&resume flow */
		spin_lock(&wdt->lock);
		wdt->active = false;
		spin_unlock(&wdt->lock);

		ret = wdt_ping(&wdt->wdd);
#else
		ret = wdt_disable(&wdt->wdd);
#endif
		cancel_delayed_work(&wdt->hisi_wdt_delayed_work);
	}

	pr_info("%s-. ret=%d\n", __func__, ret);

	return ret;
}

static void sp805_wdt_resume(void)
{
	struct sp805_wdt *wdt = g_wdt;
	int ret = 0;

	pr_info("%s+.\n", __func__);

	if (watchdog_active(&wdt->wdd) || !wdt->active) {
		if (cpu_online(0)) {
			queue_delayed_work_on(0, wdt->hisi_wdt_wq,
					      &wdt->hisi_wdt_delayed_work, 0);
		} else {
			queue_delayed_work(wdt->hisi_wdt_wq,
					   &wdt->hisi_wdt_delayed_work, 0);
		}
		ret = wdt_enable(&wdt->wdd);
	}

	pr_info("%s-. ret=%d\n", __func__, ret);

	return;
}

static struct syscore_ops sp805_wdt_syscore_ops = {
	.suspend = sp805_wdt_suspend,
	.resume = sp805_wdt_resume
};


static struct amba_id sp805_wdt_ids[] = {
	{
		.id	= 0x00141805,
		.mask	= 0x00ffffff,
	},
	{ 0, 0 },
};

MODULE_DEVICE_TABLE(amba, sp805_wdt_ids);

static struct amba_driver sp805_wdt_driver = {
	.drv = {
		.name	= MODULE_NAME,
		.pm = NULL,
	},
	.id_table	= sp805_wdt_ids,
	.probe		= sp805_wdt_probe,
	.remove = sp805_wdt_remove,
};

module_amba_driver(sp805_wdt_driver);

MODULE_AUTHOR("Viresh Kumar <viresh.linux@gmail.com>");
MODULE_DESCRIPTION("HISI SP805 Watchdog Driver");
MODULE_LICENSE("GPL");
