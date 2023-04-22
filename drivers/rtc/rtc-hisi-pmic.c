#include <asm/compiler.h>
#include <linux/compiler.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/reboot.h>
#include <linux/syscalls.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/mfd/hisi_pmic.h>
#include <linux/of.h>
#include <linux/rtc.h>
#include <linux/sizes.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/hisi/hw_cmdline_parse.h>
#include <pmic_interface.h>
#include <linux/version.h>
#include <linux/of_irq.h>
#ifdef CONFIG_HISI_RTC_SECURE_FEATURE
#include <linux/mtd/hisi_nve_interface.h>
#define  NVE_RTC_NUM             (272)
#define  NVE_NV_RTC_OFFSET_SIZE   4
#endif
#define  IRQ_ENABLE     (0x1)
#define  IRQ_DISABLE    (0x0)
/*lint -e750 -esym(750,*) */
#define  SHOW_TIME(time)   \
	do {\
		printk(KERN_INFO "[%s]: %d-%d-%d %d:%d:%d\n", __func__,\
			(time)->tm_year + 1900, (time)->tm_mon + 1,\
			(time)->tm_mday, (time)->tm_hour,\
			(time)->tm_min, (time)->tm_sec);\
		} while (0)
/*
 * Register definitions
 */
#define	SOC_RTC_DR		0x00	/* Data read register */
#define	SOC_RTC_MR		0x04	/* Match register */
#define	SOC_RTC_LR		0x08	/* Data load register */
#define	SOC_RTC_CR		0x0c	/* Control register */
#define	SOC_RTC_IMSC	0x10	/* Interrupt mask and set register */
#define	SOC_RTC_RIS		0x14	/* Raw interrupt status register */
#define	SOC_RTC_MIS		0x18	/* Masked interrupt status register */
#define	SOC_RTC_ICR		0x1c	/* Interrupt clear register */

#define	PMU_RTC_DR		0x00	/* Data read register */
#define	PMU_RTC_MR		0x04	/* Match register */
#define	PMU_RTC_LR		0x08	/* Data load register */
#define	PMU_RTC_CR		0x0c	/* Control register */

#define PMU_RTC_XO_THRESHOLD_LOW   PMIC_XO_THRESOLD0_ADDR(0)
#define PMU_RTC_XO_THRESHOLD_HIGH  PMIC_XO_THRESOLD1_ADDR(0)
/*lint -e750 +esym(750,*) */
#define SOC_RTCALARM_INT   0x1

struct hisi_rtc_dev {
	struct rtc_device *rtc_dev;

	struct resource *soc_rtc_res;
	void __iomem *soc_rtc_baseaddr;
	unsigned int soc_rtc_irq;

	int pmu_rtc_irq;
	unsigned int pmu_rtc_baseaddr;
	unsigned int pmu_rtc_imr;	/* IRQ Mask register */
	unsigned int pmu_rtc_icr;	/* IRQ & IRQ Clear register */
	unsigned int pmu_rtc_ib;	/* IRQ Bit in register */

	unsigned int pmu_rtc_xo_thrshd_pwroff_val;

	struct work_struct rtc_alarm_work;	/* work to notify here's a rtc alram */
#ifdef CONFIG_HISI_RTC_SECURE_FEATURE
	struct work_struct rtc_nv_work;	/* work to write offset to nv */
	struct workqueue_struct *nv_work_queue;
	int offset;		/* offset of PMU RTC and SOC RTC */
#endif
};

#ifdef CONFIG_HISI_RTC_SECURE_FEATURE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
enum pmu_rtc_ops {
	PMU_RTC_ENABLE = 0,
	PMU_RTC_XO_THRESHOLD_SET,
#ifdef CONFIG_HISI_RTC_TEST
	PMU_RTC_TEST,
#endif
};

#define RTC_REGISTER_FN_ID             (0xc500ddd0)
noinline int atfd_hisi_service_rtc_smc(u64 function_id, u64 arg0, u64 arg1,
				       u64 arg2)
{
	asm volatile (__asmeq("%0", "x0")
		      __asmeq("%1", "x1")
		      __asmeq("%2", "x2")
		      __asmeq("%3", "x3") "smc    #0\n":"+r"(function_id)
		      : "r"(arg0), "r"(arg1), "r"(arg2));

	return (int)function_id;
}
#endif
#endif

extern struct rtc_wkalrm poweroff_rtc_alarm;
static struct hisi_rtc_dev *ldata;

static unsigned int get_pd_charge_flag(void);
static unsigned int pd_charge_flag;

#ifdef CONFIG_HISI_RTC_TEST
static bool rtc_poweroff_alarm_test;
#endif

/**
 * parse powerdown charge cmdline which is passed from fastoot. *
 * Format : pd_charge=0 or 1             *
 */
static int __init early_parse_pdcharge_cmdline(char *p)
{
	if (p) {
		if (!strcmp(p, "charger")) {/*lint !e421*/
			pd_charge_flag = 1;
		} else {
			pd_charge_flag = 0;
		}

		printk("power down charge p:%s, pd_charge_flag :%u\n", p,
		       pd_charge_flag);
	}

	return 0;
}

early_param("androidboot.mode", early_parse_pdcharge_cmdline);

static unsigned int get_pd_charge_flag(void)
{
	return pd_charge_flag;
}

static inline struct hisi_rtc_dev *hisi_rtc_ldata_get(void)
{
	return (struct hisi_rtc_dev *)ldata;
}

#ifdef CONFIG_HISI_RTC_SECURE_FEATURE
static int hisi_rtc_nve_write(unsigned int index, int *data,
			      unsigned int len)
{
	int ret;
	struct hisi_nve_info_user pinfo;

	pinfo.nv_operation = NV_READ;
	pinfo.nv_number = NVE_RTC_NUM;
	pinfo.valid_size = NVE_NV_RTC_OFFSET_SIZE;

	memset(pinfo.nv_data, 0x0, (unsigned long)pinfo.valid_size);
	ret = hisi_nve_direct_access(&pinfo);
	if (0 != ret) {
		printk(KERN_ERR "rtc Read nve failed !\n");
		return -1;
	}

	pinfo.nv_operation = NV_WRITE;
	memcpy(pinfo.nv_data + index, data, (unsigned long)len);
	ret = hisi_nve_direct_access(&pinfo);
	if (0 != ret) {
		printk(KERN_ERR "hisi_rtc_nve_write failed !\n");
		return -1;
	}

	return 0;
}

static int hisi_rtc_nve_read(unsigned int index, int *buf, unsigned int len)
{
	int ret;
	struct hisi_nve_info_user pinfo;

	pinfo.nv_operation = NV_READ;
	pinfo.nv_number = NVE_RTC_NUM;
	pinfo.valid_size = NVE_NV_RTC_OFFSET_SIZE;

	memset(pinfo.nv_data, 0x0, (unsigned long)pinfo.valid_size);
	ret = hisi_nve_direct_access(&pinfo);
	if (0 != ret) {
		printk(KERN_ERR "hisi_rtc_nve_read failed !\n");
		return -1;
	}

	memcpy(buf, pinfo.nv_data + index, (unsigned long)len);

	return 0;
}

static void hisi_rtc_nv_write_work(struct work_struct *work)
{
	int ret;
	struct hisi_rtc_dev *s_ldata = hisi_rtc_ldata_get();

	ret = hisi_rtc_nve_write(0, &s_ldata->offset, NVE_NV_RTC_OFFSET_SIZE);
	if (ret) {
		printk(KERN_ERR "[%s]write offset to nv failed !\n", __func__);
	}
} /*lint -e715 */
#endif

static void hisi_pmu_rtc_write_bulk(unsigned int base, unsigned long data)
{
	unsigned int value, i;

	for (i = 0; i < 4; i++) {
		value = (data >> (i * 8)) & 0xff;
		hisi_pmic_reg_write((base + i), value);
	}
}

static unsigned long hisi_pmu_rtc_read_bulk(unsigned int base)
{
	unsigned long data = 0;
	unsigned int value, i;

	for (i = 0; i < 4; i++) {
		value = hisi_pmic_reg_read(base + i);
		data |= (value & 0xff) << (i * 8);/*lint !e647*/
	}

	return data;
}

static void hisi_pmu_rtc_irq_enable(unsigned int enable)
{
	struct irq_desc *desc;
	struct hisi_rtc_dev *ldata = hisi_rtc_ldata_get();/*lint !e578*/

	desc = irq_to_desc(ldata->pmu_rtc_irq);

	if (NULL == desc) {
		printk(KERN_ERR "%s:desc is NULL!\n", __func__);
		return;
	}

	if (enable == IRQ_ENABLE) {
		if (desc->irq_data.chip->irq_unmask)
			desc->irq_data.chip->irq_unmask(&desc->irq_data);
	} else {
		if (desc->irq_data.chip->irq_mask)
			desc->irq_data.chip->irq_mask(&desc->irq_data);
	}
}

void hisi_pmu_rtc_readtime(struct rtc_time *tm)
{
	unsigned long time;
	struct hisi_rtc_dev *ldata = hisi_rtc_ldata_get();/*lint !e578*/
#ifdef CONFIG_HISI_RTC_SECURE_FEATURE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	int ret, offset;
#endif
#endif

	time = hisi_pmu_rtc_read_bulk(ldata->pmu_rtc_baseaddr + PMU_RTC_DR);

#ifdef CONFIG_HISI_RTC_SECURE_FEATURE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	ret = hisi_rtc_nve_read(0, &offset, NVE_NV_RTC_OFFSET_SIZE);
	if (ret) {
		printk(KERN_ERR "[%s]read offset from nv failed\n", __func__);
	}
	time += offset; /*lint -e737 */
#endif
#endif

	rtc_time_to_tm(time, tm);
}

unsigned long hisi_pmu_rtc_readcount(void)
{
	struct hisi_rtc_dev *data = hisi_rtc_ldata_get();
	if (NULL == data) {
		printk(KERN_ERR "[%s]data is NULL !\n", __func__);
		return 0;
	}
	return hisi_pmu_rtc_read_bulk(data->pmu_rtc_baseaddr + PMU_RTC_DR);
}

#ifdef CONFIG_HISI_PMU_RTC_READCOUNT
EXPORT_SYMBOL_GPL(hisi_pmu_rtc_readcount);
#endif

void hisi_pmu_rtc_setalarmtime(unsigned long time)
{
	struct hisi_rtc_dev *ldata = hisi_rtc_ldata_get();/*lint !e578*/
	struct rtc_time tm;
#ifdef CONFIG_HISI_RTC_SECURE_FEATURE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	int ret, offset = 0;

	ret = hisi_rtc_nve_read(0, &offset, NVE_NV_RTC_OFFSET_SIZE);
	if (ret) {
		printk(KERN_ERR "[%s]read offset from nv failed\n", __func__);
	}

	hisi_pmu_rtc_write_bulk(ldata->pmu_rtc_baseaddr + PMU_RTC_MR,
				time - offset); /*lint -e737 */
	atfd_hisi_service_rtc_smc(RTC_REGISTER_FN_ID, PMU_RTC_XO_THRESHOLD_SET,
				  ldata->pmu_rtc_xo_thrshd_pwroff_val, 0);
#endif
#else
	hisi_pmu_rtc_write_bulk(ldata->pmu_rtc_baseaddr + PMU_RTC_MR, time);
	hisi_pmic_reg_write(PMU_RTC_XO_THRESHOLD_HIGH,
			    ((ldata->
			      pmu_rtc_xo_thrshd_pwroff_val >> 8) & 0xFF));
#endif
	hisi_pmu_rtc_irq_enable(IRQ_ENABLE);

	rtc_time_to_tm(time, &tm);
	SHOW_TIME(&tm);
}

int hisi_pmu_rtc_settime(struct rtc_time *tm)
{
	int err;
	unsigned long time;
	struct hisi_rtc_dev *ldata = hisi_rtc_ldata_get();/*lint !e578*/

	SHOW_TIME(tm);
	err = rtc_valid_tm(tm);
	if (err != 0) {
		printk(KERN_ERR "[%s]Time is invalid\n", __func__);
		return err;
	}
	rtc_tm_to_time(tm, &time);

	hisi_pmu_rtc_write_bulk(ldata->pmu_rtc_baseaddr + PMU_RTC_LR, time);

	return 0;
}

unsigned long hisi_pmu_rtc_readalarmtime(void)
{
	unsigned long time;
	struct hisi_rtc_dev *ldata = hisi_rtc_ldata_get();/*lint !e578*/
#ifdef CONFIG_HISI_RTC_SECURE_FEATURE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	int ret, offset = 0;
#endif
#endif
	time = hisi_pmu_rtc_read_bulk(ldata->pmu_rtc_baseaddr + PMU_RTC_MR);

#ifdef CONFIG_HISI_RTC_SECURE_FEATURE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	ret = hisi_rtc_nve_read(0, &offset, NVE_NV_RTC_OFFSET_SIZE);
	if (ret) {
		printk(KERN_ERR "[%s]read offset from nv failed\n", __func__);
	}
	time += offset; /*lint -e737 */
#endif
#endif
	return time;
}

static int hisi_soc_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct hisi_rtc_dev *ldata = dev_get_drvdata(dev);/*lint !e578*/

	rtc_time_to_tm(readl(ldata->soc_rtc_baseaddr + SOC_RTC_DR), tm);

	return 0;
}

static int hisi_soc_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	int err;
	unsigned long time;
#ifdef CONFIG_HISI_RTC_SECURE_FEATURE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	long pmu_time = 0;
	struct hisi_rtc_dev *s_ldata = hisi_rtc_ldata_get();
#endif
#endif
	struct hisi_rtc_dev *ldata = dev_get_drvdata(dev);/*lint !e578*/

	SHOW_TIME(tm);
	err = rtc_valid_tm(tm);
	if (err != 0) {
		printk(KERN_ERR "[%s]Time is invalid\n", __func__);
		return err;
	}
	rtc_tm_to_time(tm, &time);

	writel(time, ldata->soc_rtc_baseaddr + SOC_RTC_LR);
	/* set pmu rtc */
#ifdef CONFIG_HISI_RTC_SECURE_FEATURE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	pmu_time = hisi_pmu_rtc_read_bulk(ldata->pmu_rtc_baseaddr);
	s_ldata->offset = (int)time - (int)pmu_time;
	queue_work(s_ldata->nv_work_queue, &s_ldata->rtc_nv_work);
#endif
#else
	hisi_pmu_rtc_write_bulk(ldata->pmu_rtc_baseaddr + PMU_RTC_LR, time);
#endif
	return 0;
}

static int hisi_soc_rtc_read_alarmtime(struct device *dev,
				       struct rtc_wkalrm *alarm)
{
	struct hisi_rtc_dev *ldata = dev_get_drvdata(dev);/*lint !e578*/
	unsigned long time;

	time = readl(ldata->soc_rtc_baseaddr + SOC_RTC_MR);

	rtc_time_to_tm(time, &alarm->time);

	alarm->pending =
	    readl(ldata->soc_rtc_baseaddr + SOC_RTC_RIS) & SOC_RTCALARM_INT;
	alarm->enabled =
	    readl(ldata->soc_rtc_baseaddr + SOC_RTC_IMSC) & SOC_RTCALARM_INT;

	return 0;
}

static int hisi_soc_rtc_alarm_irq_enable(struct device *dev,
					 unsigned int enabled)
{
	unsigned int imsc;
	struct hisi_rtc_dev *ldata = dev_get_drvdata(dev);/*lint !e578*/

	/* Clear any pending alarm interrupts. */
	writel(SOC_RTCALARM_INT, ldata->soc_rtc_baseaddr + SOC_RTC_ICR);

	imsc = readl(ldata->soc_rtc_baseaddr + SOC_RTC_IMSC);
	if (enabled == 1)
		writel(imsc | SOC_RTCALARM_INT,
		       ldata->soc_rtc_baseaddr + SOC_RTC_IMSC);
	else
		writel(imsc & ~SOC_RTCALARM_INT,
		       ldata->soc_rtc_baseaddr + SOC_RTC_IMSC);

	return 0;
}

static int hisi_soc_rtc_set_alarmtime(struct device *dev,
				      struct rtc_wkalrm *alarm)
{
	struct hisi_rtc_dev *ldata = dev_get_drvdata(dev);/*lint !e578*/
	unsigned long time;
	int ret;

	SHOW_TIME(&alarm->time);
	ret = rtc_valid_tm(&alarm->time);
	if (ret == 0) {
		rtc_tm_to_time(&alarm->time, &time);
		writel(time, ldata->soc_rtc_baseaddr + SOC_RTC_MR);
		hisi_soc_rtc_alarm_irq_enable(dev, alarm->enabled);
	} else {
		printk(KERN_ERR "[%s]Alarm time is invalid\n", __func__);
	}

	return ret;
}

static struct rtc_class_ops hisi_rtc_ops = {
	.read_time = hisi_soc_rtc_read_time,
	.set_time = hisi_soc_rtc_set_time,
	.read_alarm = hisi_soc_rtc_read_alarmtime,
	.set_alarm = hisi_soc_rtc_set_alarmtime,
	.alarm_irq_enable = hisi_soc_rtc_alarm_irq_enable,
};

static void rtc_alarm_notify_work(struct work_struct *work)
{
	struct hisi_rtc_dev *ldata =  /*lint !e578*/
	    container_of(work, struct hisi_rtc_dev, rtc_alarm_work);
	printk(KERN_INFO "Entering rtc_alarm_notify_work!");
	if ((ldata) && (ldata->rtc_dev) && (&(ldata->rtc_dev->dev))) {
		kobject_uevent(&(ldata->rtc_dev->dev.kobj), KOBJ_CHANGE);
	}
}

static irqreturn_t hisi_pmu_rtc_interrupt(int irq, void *dev_id)
{
	printk(KERN_ERR "[%s]RTC Interrupt Comes\n", __func__);

	if (unlikely(get_pd_charge_flag())) {
		schedule_work(&ldata->rtc_alarm_work);
		return IRQ_HANDLED;
	}

	return IRQ_HANDLED;
}

static irqreturn_t hisi_soc_rtc_interrupt(int irq, void *dev_id)
{
	struct hisi_rtc_dev *ldata = dev_id;/*lint !e578*/
	unsigned int rtcmis;
	unsigned long events = 0;

	printk(KERN_ERR "[%s]RTC Interrupt Comes\n", __func__);

	rtcmis = readl(ldata->soc_rtc_baseaddr + SOC_RTC_MIS);	/*read interrupt */
	if (rtcmis) {
		writel(rtcmis, ldata->soc_rtc_baseaddr + SOC_RTC_ICR);	/*clear interrupt */
		if (rtcmis & SOC_RTCALARM_INT)
			events |= (RTC_AF | RTC_IRQF);
		rtc_update_irq(ldata->rtc_dev, 1, events);

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static void hisi_rtc_shutdown(struct platform_device *pdev)
{
	unsigned long time;
	struct hisi_rtc_dev *ldata = dev_get_drvdata(&pdev->dev);/*lint !e578*/

	if (unlikely(get_pd_charge_flag()))
		return;

#ifdef CONFIG_HISI_RTC_TEST
	if (rtc_poweroff_alarm_test)
		return;
#endif

	if (poweroff_rtc_alarm.enabled) {
		rtc_tm_to_time(&poweroff_rtc_alarm.time, &time);
		hisi_pmu_rtc_setalarmtime(time);
	} else {
		hisi_pmu_rtc_write_bulk(ldata->pmu_rtc_baseaddr + PMU_RTC_MR,
					0);
	}

	return;
}

static int hisi_rtc_remove(struct platform_device *pdev)
{
	struct hisi_rtc_dev *ldata = dev_get_drvdata(&pdev->dev);/*lint !e578*/

#ifdef CONFIG_HISI_RTC_SECURE_FEATURE
	struct hisi_rtc_dev *s_ldata = hisi_rtc_ldata_get();
	cancel_work_sync(&s_ldata->rtc_nv_work);
	flush_workqueue(s_ldata->nv_work_queue);
	destroy_workqueue(s_ldata->nv_work_queue);
#endif

	free_irq(ldata->pmu_rtc_irq, ldata->rtc_dev);
	free_irq(ldata->soc_rtc_irq, ldata->rtc_dev);

	platform_set_drvdata(pdev, NULL);

	rtc_device_unregister(ldata->rtc_dev);
	iounmap(ldata->soc_rtc_baseaddr);

	return 0;
}

static int hisi_rtc_get_property(struct device_node *np,
				 struct hisi_rtc_dev *prtc)
{
	int ret = 0;

	ret =
	    of_property_read_u32(np, "hisilicon,pmic-rtc-base",
				 &prtc->pmu_rtc_baseaddr);
	if (ret) {
		printk(KERN_ERR "[%s]Get hisilicon,pmic-rtc-base failed.\n",
		       __func__);
		return -ENODEV;
	}

	ret =
	    of_property_read_u32(np, "hisilicon,pmic-rtc-imr",
				 &prtc->pmu_rtc_imr);
	if (ret) {
		printk(KERN_ERR "[%s]Get hisilicon,pmic-rtc-imr failed.\n",
		       __func__);
		return -ENODEV;
	}

	ret =
	    of_property_read_u32(np, "hisilicon,pmic-rtc-icr",
				 &prtc->pmu_rtc_icr);
	if (ret) {
		printk(KERN_ERR "[%s]Get hisilicon,pmic-rtc-icr failed.\n",
		       __func__);
		return -ENODEV;
	}

	ret =
	    of_property_read_u32(np, "hisilicon,pmic-rtc-ib",
				 &prtc->pmu_rtc_ib);
	if (ret) {
		printk(KERN_ERR "[%s]Get hisilicon,pmic-rtc-ib failed.\n",
		       __func__);
		return -ENODEV;
	}

	ret =
	    of_property_read_u32(np, "hisilicon,pmic-rtc-xo-thrshd-pwroff-val",
				 &prtc->pmu_rtc_xo_thrshd_pwroff_val);
	if (ret) {
		printk(KERN_ERR
		       "[%s]Get hisilicon,pmic-rtc-xo-thrshd-pwroff-val failed.\n",
		       __func__);
		return -ENODEV;
	}

	return 0;
}

static int hisi_rtc_probe(struct platform_device *pdev)
{
	int ret;
	unsigned int soc_baseaddr;
#ifndef CONFIG_HISI_RTC_SECURE_FEATURE
	unsigned long time;
	unsigned int reg_value;
#endif
	struct hisi_rtc_dev *prtc = NULL;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *root = NULL;

	printk(KERN_INFO "HISI PMU RTC Init Start----------------\n");

	prtc = devm_kzalloc(dev, sizeof(struct hisi_rtc_dev), GFP_KERNEL);
	if (!prtc) {
		printk(KERN_ERR "[%s]devm_kzalloc error\n", __func__);
		ret = -ENOMEM;
		goto out;
	}
	ldata = prtc;
	platform_set_drvdata(pdev, prtc);

	root =
	    of_find_compatible_node(pdev->dev.of_node, NULL,
				    "hisilicon,soc-rtc");
	ret = of_property_read_u32(root, "soc-rtc-baseaddr", &soc_baseaddr);
	if (ret) {
		printk(KERN_ERR "[%s]get soc_rtc_baseaddr err\n", __func__);
		goto out_no_rtc;
	}

	prtc->soc_rtc_baseaddr = ioremap(soc_baseaddr, PAGE_SIZE);
	if (!prtc->soc_rtc_baseaddr) {
		printk(KERN_ERR "[%s]base address ioremap fail\n", __func__);
		ret = -ENOMEM;
		goto out;
	}

	prtc->soc_rtc_irq = irq_of_parse_and_map(root, 0);
	if (prtc->soc_rtc_irq == 0) {
		printk(KERN_ERR "[%s]get soc_rtc_irq err\n", __func__);
		ret = -ENOENT;
		goto out_no_rtc;
	}

	prtc->pmu_rtc_irq = platform_get_irq_byname(pdev, "hisi-pmic-rtc");
	if (prtc->pmu_rtc_irq <= 0) {
		printk(KERN_ERR "[%s]get pmu_rtc_irq err\n", __func__);
		ret = -ENOENT;
		goto out_no_rtc;
	}

	ret = hisi_rtc_get_property(np, prtc);
	if (ret) {
		printk(KERN_ERR "[%s]rtc_get_property error\n", __func__);
		goto out_no_rtc;
	}

	device_init_wakeup(&pdev->dev, 1);

	prtc->rtc_dev =
	    rtc_device_register("hisi-rtc", &pdev->dev, &hisi_rtc_ops,
				THIS_MODULE);
	if (IS_ERR(prtc->rtc_dev)) {
		printk(KERN_ERR "[%s]rtc_device_register error\n", __func__);
		ret = PTR_ERR(prtc->rtc_dev);
		goto out_no_rtc;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	if (devm_request_irq(dev, prtc->pmu_rtc_irq, hisi_pmu_rtc_interrupt,
			     IRQF_NO_SUSPEND, "hisi-pmu-rtc", prtc)) {
		printk(KERN_ERR "[%s]request_pmu_rtc_irq error\n", __func__);
		ret = -EIO;
		goto out_no_irq;
	}

	if (devm_request_irq(dev, prtc->soc_rtc_irq, hisi_soc_rtc_interrupt,
			     IRQF_NO_SUSPEND, "hisi-soc-rtc", prtc)) {
		printk(KERN_ERR "[%s]request_soc_rtc_irq error\n", __func__);
		ret = -EIO;
		goto out_no_irq;
	}
#else
	if (devm_request_irq(dev, prtc->pmu_rtc_irq, hisi_pmu_rtc_interrupt,
			     IRQF_DISABLED | IRQF_NO_SUSPEND, "hisi-pmu-rtc",
			     prtc)) {
		printk(KERN_ERR "[%s]request_pmu_rtc_irq error\n", __func__);
		ret = -EIO;
		goto out_no_irq;
	}

	if (devm_request_irq(dev, prtc->soc_rtc_irq, hisi_soc_rtc_interrupt,
			     IRQF_DISABLED | IRQF_NO_SUSPEND, "hisi-soc-rtc",
			     prtc)) {
		printk(KERN_ERR "[%s]request_soc_rtc_irq error\n", __func__);
		ret = -EIO;
		goto out_no_irq;
	}
#endif
	/*enable pmu rtc */
#ifdef CONFIG_HISI_RTC_SECURE_FEATURE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	ldata->nv_work_queue = create_workqueue("rtc_nv_work");
	INIT_WORK(&ldata->rtc_nv_work, hisi_rtc_nv_write_work);
	atfd_hisi_service_rtc_smc(RTC_REGISTER_FN_ID, PMU_RTC_ENABLE,
				  0, 0);
#endif
#else
	reg_value = hisi_pmic_reg_read(prtc->pmu_rtc_baseaddr + PMU_RTC_CR);
	if (0 == (reg_value & 0x1)) {
		reg_value |= 0x1;
		hisi_pmic_reg_write(prtc->pmu_rtc_baseaddr + PMU_RTC_CR,
				    reg_value);
		mdelay(200);
	}

	/*enable soc rtc */
	writel(0x1, prtc->soc_rtc_baseaddr + SOC_RTC_CR);

	/*sync pmu rtc to soc rtc */
	time = hisi_pmu_rtc_read_bulk(prtc->pmu_rtc_baseaddr + PMU_RTC_DR);
	writel(time, prtc->soc_rtc_baseaddr + SOC_RTC_LR);
#endif

	INIT_WORK(&ldata->rtc_alarm_work, rtc_alarm_notify_work);

	printk(KERN_INFO "HISI PMU RTC Init End----------------\n");

	return 0;

out_no_irq:
	rtc_device_unregister(prtc->rtc_dev);

out_no_rtc:
	iounmap(prtc->soc_rtc_baseaddr);
	platform_set_drvdata(pdev, NULL);

out:
	return ret;
}

#ifdef CONFIG_PM
static int hisi_rtc_suspend(struct device *dev)
{
	struct hisi_rtc_dev *ldata = dev_get_drvdata(dev);/*lint !e578*/

	dev_info(&ldata->rtc_dev->dev, "%s: suspend +\n", __func__);

	if (device_may_wakeup(dev) && enable_irq_wake(ldata->soc_rtc_irq)) {
		dev_err(&ldata->rtc_dev->dev, "%s: failed to enable irq wake\n",
			__func__);
	}

	dev_info(&ldata->rtc_dev->dev, "%s: suspend -\n", __func__);
	return 0;
}

static int hisi_rtc_resume(struct device *dev)
{
	struct hisi_rtc_dev *ldata = dev_get_drvdata(dev);/*lint !e578*/

	dev_info(&ldata->rtc_dev->dev, "%s: resume +\n", __func__);

	if (device_may_wakeup(dev) && disable_irq_wake(ldata->soc_rtc_irq)) {
		dev_err(&ldata->rtc_dev->dev,
			"%s: failed to disable irq wake\n", __func__);
	}

	dev_info(&ldata->rtc_dev->dev, "%s: resume -\n", __func__);
	return 0;
}

static SIMPLE_DEV_PM_OPS(hisi_rtc_pm_ops, hisi_rtc_suspend, hisi_rtc_resume);
#define HISI_RTC_PM (&hisi_rtc_pm_ops)
#else
#define HISI_RTC_PM NULL
#endif

static struct of_device_id hisi_rtc_of_match[] = {
	{.compatible = "hisilicon,hisi-rtc"},
	{}
};

MODULE_DEVICE_TABLE(of, hisi_rtc_of_match);

static struct platform_driver hisi_rtc_driver = {
	.probe = hisi_rtc_probe,
	.remove = hisi_rtc_remove,
	.shutdown = hisi_rtc_shutdown,
	.driver = {
		   .name = "hisi-rtc",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(hisi_rtc_of_match),
		   .pm = HISI_RTC_PM,
		   },
};

static int __init hisi_rtc_init(void)
{
	return platform_driver_register(&hisi_rtc_driver);
}

static void __exit hisi_rtc_exit(void)
{
	platform_driver_unregister(&hisi_rtc_driver);
}

module_init(hisi_rtc_init);
module_exit(hisi_rtc_exit);

#ifdef CONFIG_HISI_RTC_TEST
/*For Test*/
#ifndef CONFIG_HISI_RTC_SECURE_FEATURE
#define  RTC_TEST_WRITE_PMURTC_TIME     0
#endif
#define  RTC_TEST_READ_PMURTC_TIME      1
#define  RTC_TEST_ENABLE_PMURTC_ALARM   2
#define  RTC_TEST_CANCEL_PMURTC_ALARM   3
#define  RTC_TEST_WRITE_SOCRTC_TIME     4
#define  RTC_TEST_READ_SOCRTC_TIME      5
#define  RTC_TEST_ENABLE_SOCRTC_ALARM   6
#define  RTC_TEST_CANCEL_SOCRTC_ALARM   7
#define  RTC_TEST_POWEROFF_ALARM        8
#ifdef CONFIG_HISI_RTC_SECURE_FEATURE
#define  RTC_TEST_ATF_READ_WRITE        9
#endif
/*lint -save -e713*/
unsigned long hisi_rtc_test(int cmd, unsigned long wtime)
{
	struct hisi_rtc_dev *data = hisi_rtc_ldata_get();
	unsigned long rtime;
	unsigned int reg_val;

	switch (cmd) {
#ifndef CONFIG_HISI_RTC_SECURE_FEATURE
	case RTC_TEST_WRITE_PMURTC_TIME:
		hisi_pmu_rtc_write_bulk(data->pmu_rtc_baseaddr + PMU_RTC_LR,
					wtime);
		return 0;
#endif
	case RTC_TEST_READ_PMURTC_TIME:
		rtime =
		    hisi_pmu_rtc_read_bulk(data->pmu_rtc_baseaddr +
					   PMU_RTC_DR);
		return rtime;
	case RTC_TEST_ENABLE_PMURTC_ALARM:
		rtime =
		    hisi_pmu_rtc_read_bulk(data->pmu_rtc_baseaddr +
					   PMU_RTC_DR);
		wtime = rtime + wtime;
		hisi_pmu_rtc_write_bulk(data->pmu_rtc_baseaddr + PMU_RTC_MR,
					wtime);
		reg_val = hisi_pmic_reg_read(data->pmu_rtc_imr);
		reg_val &= ~((unsigned int)0x1 << (data->pmu_rtc_ib));
		hisi_pmic_reg_write(data->pmu_rtc_imr, reg_val);
		return 0;
	case RTC_TEST_CANCEL_PMURTC_ALARM:
		reg_val = hisi_pmic_reg_read(data->pmu_rtc_imr);
		reg_val |= ((unsigned int)0x1 << (data->pmu_rtc_ib));
		hisi_pmic_reg_write(data->pmu_rtc_imr, reg_val);
		return 0;
	case RTC_TEST_WRITE_SOCRTC_TIME:
		writel(wtime, data->soc_rtc_baseaddr + SOC_RTC_LR);
		return 0;
	case RTC_TEST_READ_SOCRTC_TIME:
		rtime = readl(data->soc_rtc_baseaddr + SOC_RTC_DR);/*lint !e732*/
		return rtime;
	case RTC_TEST_ENABLE_SOCRTC_ALARM:
		rtime = readl(data->soc_rtc_baseaddr + SOC_RTC_DR);/*lint !e732*/
		wtime = rtime + wtime;
		writel(wtime, data->soc_rtc_baseaddr + SOC_RTC_MR);
		writel(0x1, data->soc_rtc_baseaddr + SOC_RTC_IMSC);
		return 0;/*lint !e438*/
	case RTC_TEST_CANCEL_SOCRTC_ALARM:
		writel(0x0, data->soc_rtc_baseaddr + SOC_RTC_IMSC);
		return 0;
	case RTC_TEST_POWEROFF_ALARM:
		rtc_poweroff_alarm_test = true;	/* set rtc_poweroff_alarm_test true to change shutdown flow */
		return 0;
#ifdef CONFIG_HISI_RTC_SECURE_FEATURE
	case RTC_TEST_ATF_READ_WRITE:
		atfd_hisi_service_rtc_smc(RTC_REGISTER_FN_ID, PMU_RTC_TEST, 0, 0);/*lint !e747*/
		return 0;
#endif
	default:
		return 0;
	}
}
/*lint -restore*/
#endif

MODULE_AUTHOR("Hisilicon Co. Ltd");
MODULE_DESCRIPTION("HISI RTC Driver");
MODULE_LICENSE("GPL");
