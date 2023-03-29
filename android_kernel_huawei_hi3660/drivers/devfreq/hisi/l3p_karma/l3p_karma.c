/*
 *
 * Copyright (C) 2015 Hisilicon
 * License terms: GNU General Public License (GPL) version 2
 *
 */


#include <linux/devfreq.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include <asm/compiler.h>
#include <trace/events/power.h>
#include <linux/cpu_pm.h>
#include <linux/cpu.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/perf_event.h>
#include <l3p_karma.h>

#include "governor.h"

#define CREATE_TRACE_POINTS
#include <trace/events/l3p_karma.h>

#define set_bits(mask, addr)  do {writel((((u32)readl(addr))|(mask)), (addr));} while (0)
#define clr_bits(mask, addr)   do {writel((((u32)readl(addr))&(~((u32)(mask)))), (addr));} while (0)


#define L3P_KARMA_PLATFORM_DEVICE_NAME			"l3p_karma"
#define L3P_KARMA_GOVERNOR_NAME					"l3p_karma_gov"

#define L3P_KARMA_DEFAULT_POLLING_MS			20


/* CPU PMU EVENT ID */
#define L3D_REFILL_EV		0x02A
#define L3D_EV			0x02B
#define INST_EV		0x08
#define CYC_EV		0x11

/* CPU PMU EVENT IDX */
enum cpu_ev_idx {
	L3D_REFILL_IDX,
	L3D_IDX,
	INST_IDX,
	CYC_IDX,
	CPU_NUM_EVENTS
};

/* KARMA PMU EVENT IDX */
enum karma_ev_index {
	REP_GOOD_IDX,
	DCP_GOOD_IDX,
	REP_GEN_IDX,
	DCP_GEN_IDX,
	ACP_DRP_REP_IDX,
	ACP_DRP_DCP_IDX,
	KARMA_NUM_EVENTS
};

struct event_data {
	struct perf_event *pevent;
	unsigned long prev_count;
};

struct cpu_evt_count {
	unsigned long l3d_refill_cnt;
	unsigned long l3d_cnt;
	unsigned long inst_cnt;
	unsigned long cyc_cnt;
};

struct karma_evt_count {
	unsigned long rep_gd_cnt;
	unsigned long dcp_gd_cnt;
	unsigned long rep_gen_cnt;
	unsigned long dcp_gen_cnt;
	unsigned long acp_drp_rep_cnt;
	unsigned long acp_drp_dcp_cnt;
};

struct cpu_evt_data {
	struct event_data events[CPU_NUM_EVENTS];
	ktime_t prev_ts;
};

struct karma_evt_data {
	struct event_data events[KARMA_NUM_EVENTS];
};

struct cpu_grp_info {
	cpumask_t cpus;
	cpumask_t inited_cpus;
	unsigned int event_ids[CPU_NUM_EVENTS];
	struct cpu_evt_data *evtdata;
	struct notifier_block perf_cpu_notif;
	struct list_head mon_list;
};

struct cpu_hwmon{
	int (*start_hwmon)(struct cpu_hwmon *hw);
	void (*stop_hwmon)(struct cpu_hwmon *hw);
	unsigned long (*get_cnt)(struct cpu_hwmon *hw);
	struct cpu_grp_info *cpu_grp;
	cpumask_t cpus;
	unsigned int num_cores;
	struct cpu_evt_count *core_stats;
	unsigned long total_refill;
	unsigned long total_access;
	unsigned long total_inst;
	unsigned long total_cyc;
};

struct karma_hwmon {
	int (*start_hwmon)(struct karma_hwmon *hw);
	void (*stop_hwmon)(struct karma_hwmon *hw);
	unsigned long (*get_cnt)(struct karma_hwmon *hw);
	unsigned int event_ids[KARMA_NUM_EVENTS];
	struct karma_evt_count count;
	struct karma_evt_data hw_data;
};

#define to_evtdata(cpu_grp, cpu) \
	(&cpu_grp->evtdata[cpu - cpumask_first(&cpu_grp->cpus)])
#define to_evtcnts(hw, cpu) \
	(&hw->core_stats[cpu - cpumask_first(&hw->cpus)])

static LIST_HEAD(perf_mon_list);
static DEFINE_MUTEX(list_lock);



struct l3p_karma {
	struct devfreq *devfreq;
	struct platform_device *pdev;
	struct devfreq_dev_profile *devfreq_profile;
	void __iomem *base;
	u32 polling_ms;
	unsigned int monitor_enable;
	bool mon_started;
	struct mutex mon_mutex_lock;
	u32 control_data[KARMA_SYS_CTRL_NUM-1];
	bool fcm_idle;
	spinlock_t data_spinlock;

	unsigned long *freq_table;
	int freq_table_len;

	struct {
		unsigned long accesses;
		unsigned long misses;
		unsigned int last_update;
	} alg;

	struct cpu_hwmon *cpu_hw;
	struct karma_hwmon *karma_hw;

	struct notifier_block idle_nb;
	struct attribute_group *attr_grp;
};


static const struct of_device_id l3p_karma_devfreq_id[] = {
	{.compatible = "hisi,l3p_karma"},
	{}
};



static int l3p_start_monitor(struct devfreq *df)
{
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);
	struct cpu_hwmon *cpu_hw;
	struct karma_hwmon *karma_hw;
	struct device *dev;
	int ret;

	if (!l3p->monitor_enable || l3p->mon_started) {
		return 0;
	}

	cpu_hw = l3p->cpu_hw;
	karma_hw = l3p->karma_hw;
	dev = df->dev.parent;

	ret = cpu_hw->start_hwmon(cpu_hw);
	if (ret) {
		dev_err(dev, "Unable to start CPU HW monitor! (%d)\n", ret);
		cpu_hw->stop_hwmon(cpu_hw);
		return ret;
	}

	ret = karma_hw->start_hwmon(karma_hw);
	if (ret) {
		dev_err(dev, "Unable to start KARMA HW monitor! (%d)\n", ret);
		cpu_hw->stop_hwmon(cpu_hw);
		return ret;
	}

	//mutex_lock(&df->lock);
	devfreq_monitor_start(df);
	//mutex_unlock(&df->lock);

	l3p->mon_started = true;

	return 0;
}

static void l3p_stop_monitor(struct devfreq *df)
{
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);
	struct cpu_hwmon *cpu_hw;
	struct karma_hwmon *karma_hw;

	if (!l3p->monitor_enable || !l3p->mon_started) {
		return ;
	}

	cpu_hw = l3p->cpu_hw;
	karma_hw = l3p->karma_hw;

	l3p->mon_started = false;

	devfreq_monitor_stop(df);
	cpu_hw->stop_hwmon(cpu_hw);
	karma_hw->stop_hwmon(karma_hw);
}

static void l3p_karma_init(struct l3p_karma *l3p)
{
#ifdef L3P_KARMA_ES
	u32 value = 0;
	u32 mask = 0;

	spin_lock(&l3p->data_spinlock);
	/*
	* DCP basic control
	* [29:24] = 6'd8 Default DCP DEG
	*/
	value = readl(l3p->base + KARMA_DCPBASE_CONTROL_OFFSET);
	mask = 0x3F;
	value &= (~(mask << 24));
	value |= (0x8 << 24);
	writel(value, l3p->base + KARMA_DCPBASE_CONTROL_OFFSET);

	/*
	* REP control
	* [5:0] = 6'd8 Default REP degree
	* [9:6] = 4'd7 Default AC shift
	* [28:28] = 1'd1 DVM sync clear disable
	*/
	value = readl(l3p->base + KARMA_REP_CONTROL_OFFSET);
	mask = 0x3F;
	value &= (~mask);
	value |= 0x8;

	mask = 0xF;
	value &= (~(mask << 6));
	value |= (0x7 << 6);

	mask = 0x1;
	value &= (~(mask << 28));
	value |= (0x1 << 28);
	writel(value, l3p->base + KARMA_REP_CONTROL_OFFSET);

	/*
	* STG control
	* [21:20] = 2'd2 ADP interval threshold (256 evictions)
	* [22:22] =1'd1 Force DCP TAG hit for update
	*/
	value = readl(l3p->base + KARMA_STG_CONTROL_OFFSET);
	mask = 0x3;
	value &= (~(mask << 20));
	value |= (0x2 << 20);

	mask = 0x1;
	value &= (~(mask << 22));
	value |= (0x1 << 22);
	writel(value, l3p->base + KARMA_STG_CONTROL_OFFSET);

	/*
	* ADB adaptive parameter
	* [2:0] = 3'd4 ADBS REP BIAS0
	* [5:3] = 3'd4 ADBS REP BIAS1
	* [8:6] = 3'd4 ADBS REP BIAS2
	* [11:9] = 3'd 4ADBS REP BIAS3
	* [16:12] = 5'd2 ADBS effictive0
	* [21:17] = 5'd2 ADBS effictive1
	* [26:22] = 5'd2 ADBS effictive2
	* [31:27] = 5'd2 ADBS effictive3
	*/
	value = readl(l3p->base + KARMA_ADB_APA_PARA_OFFSET);
	mask = 0x7;
	value &= (~(mask));
	value |= (0x4);

	value &= (~(mask << 3));
	value |= (0x4 << 3);

	value &= (~(mask << 6));
	value |= (0x4 << 6);

	value &= (~(mask << 9));
	value |= (0x4 << 9);

	mask = 0x1F;
	value &= (~(mask << 12));
	value |= (0x2 << 12);

	value &= (~(mask << 17));
	value |= (0x2 << 17);

	value &= (~(mask << 22));
	value |= (0x2 << 22);

	value &= (~(mask << 27));
	value |= (0x2 << 27);
	writel(value, l3p->base + KARMA_ADB_APA_PARA_OFFSET);

	/*
	* ADB basic control
	* [27:27] = 1'd1 DCP across 4K enable
	*/
	value = readl(l3p->base + KARMA_ADB_BASIC_OFFSET);
	mask = 0x1;
	value &= (~(mask << 27));
	value |= (0x1 << 27);
	writel(value, l3p->base + KARMA_ADB_BASIC_OFFSET);
	spin_unlock(&l3p->data_spinlock);
#endif
}

static void l3p_karma_enable(struct l3p_karma *l3p)
{
	/* enable  karma*/
	spin_lock(&l3p->data_spinlock);
	set_bits(BIT(0), (l3p->base + KARMA_MAIN_ENABLE_OFFSET));
	spin_unlock(&l3p->data_spinlock);
}

static void l3p_karma_disable(struct l3p_karma *l3p)
{
	u32 value = 0;

	/* disable  karma*/
	spin_lock(&l3p->data_spinlock);
	clr_bits(BIT(0), (l3p->base + KARMA_MAIN_ENABLE_OFFSET));

	value = readl(l3p->base + KARMA_IDLE_STATUS_OFFSET);
	while(0 == (value & BIT(0))){
		value = readl(l3p->base + KARMA_IDLE_STATUS_OFFSET);
	}
	spin_unlock(&l3p->data_spinlock);
}

static ssize_t show_monitor_enable(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	ssize_t ret = 0;
	struct devfreq *df = to_devfreq(dev);
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);

	mutex_lock(&l3p->mon_mutex_lock);
	ret = scnprintf(buf, PAGE_SIZE, "%u\n", l3p->monitor_enable);
	mutex_unlock(&l3p->mon_mutex_lock);

	return ret;
}

static ssize_t store_monitor_enable(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	struct devfreq *df = to_devfreq(dev);
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);
	unsigned int val;
	int ret = 0;

	ret = kstrtouint(buf, 10, &val);
	if (ret)
		return ret;

	if (val != 0)
		val = 1;

	mutex_lock(&l3p->mon_mutex_lock);
	if (l3p->monitor_enable == 0 && val == 1) {
		l3p->monitor_enable = 1;
		if (unlikely(l3p_start_monitor(df))) {
			dev_err(dev, "l3p_start_monitor failed\n");
			/* handle error */
			l3p->monitor_enable = 0;
			//l3p_karma_disable(l3p);
			ret = -EINVAL;
		} else {
			/* enable karma */
			l3p_karma_enable(l3p);
		}
	} else if (l3p->monitor_enable == 1 && val == 0) {
		l3p_stop_monitor(df);
		l3p->monitor_enable = 0;
		l3p_karma_disable(l3p);
	}
	mutex_unlock(&l3p->mon_mutex_lock);

	return  ret ? ret : count;
}
static DEVICE_ATTR(monitor_enable, 0660, show_monitor_enable, store_monitor_enable);

#ifdef CONFIG_HISI_L3P_KARMA_DEBUG
static ssize_t show_reg_cfg(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	ssize_t ret = 0;
	struct devfreq *df = to_devfreq(dev);
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);
	ssize_t count = 0;
	int i = 0;

	spin_lock(&l3p->data_spinlock);
	for(i = 0; i< KARMA_SYS_CTRL_NUM; i++){
		ret = snprintf(buf + count, (PAGE_SIZE - count),"offset:0x%x \t value:0x%x\n", (i * 4), readl(l3p->base + i * 4));/* unsafe_function_ignore: snprintf */
		if (ret >= (PAGE_SIZE - count) || ret < 0) { /*lint !e574 */
			goto err;
		}
		count += ret;
	}
err:
	spin_unlock(&l3p->data_spinlock);
	return count;
}

static int cmd_parse(char *para_cmd, char *argv[], int max_args)
{
    int para_id = 0;

	while (*para_cmd != '\0')
	{
		if (para_id >= max_args)
			break;
		while (*para_cmd == ' ')
			para_cmd++;

		if ('\0' == *para_cmd)
			break;

		argv[para_id] = para_cmd;
		para_id++;

		while ((*para_cmd != ' ') && (*para_cmd != '\0'))
			para_cmd++;

		if ('\0' == *para_cmd)
			break;

		*para_cmd = '\0';
		para_cmd++;

	}

	return para_id;
}

#define LOCAL_BUF_MAX       (128)
static ssize_t store_reg_cfg(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	struct devfreq *df = to_devfreq(dev);
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);
	char local_buf[LOCAL_BUF_MAX] = {0};
	char *argv[2] = {0};
	unsigned int offset = 0, value = 0;
	int argc = 0, ret = 0;

	if (count >= sizeof(local_buf)) {
		return -ENOMEM;
	}

	strncpy(local_buf, buf, min_t(size_t, sizeof(local_buf) - 1, count)); /* unsafe_function_ignore: strncpy */

	argc = cmd_parse(local_buf, argv, sizeof(argv) / sizeof(argv[0]));
	if (2 != argc) {
		dev_err(dev, "error, only surport two para\n");
		return -EINVAL;
	}

	ret = sscanf(argv[0], "%x", &offset); /* unsafe_function_ignore: sscanf */
	if (ret != 1) {
		dev_err(dev, "parse offset error %s\n", argv[0]);
		return -EINVAL;
	}

	ret = sscanf(argv[1], "%x", &value);/* unsafe_function_ignore: sscanf */
	if (ret != 1) {
		dev_err(dev, "parse value error %s\n", argv[1]);
		return -EINVAL;
	}

	if (0 == offset){
		dev_err(dev, "Do'nt en/dis karma by this way, through monitor_enable instead\n");
		return -EINVAL;
#ifdef L3P_KARMA_ES
	}else if(0x10 == offset) {
		value |= BIT(28);
#endif
	}else if(offset > KARMA_RW_OFFSET_MAX || 0 != (offset % 4)) {
		dev_err(dev, "not right value, offset = 0x%x\n", offset);
		return -EINVAL;
	}

	spin_lock(&l3p->data_spinlock);
	writel(value, l3p->base + offset);
	spin_unlock(&l3p->data_spinlock);

	return  count;
}
static DEVICE_ATTR(reg_cfg, 0660, show_reg_cfg, store_reg_cfg);
#endif


static struct attribute *dev_attr[] = {
	&dev_attr_monitor_enable.attr,
#ifdef CONFIG_HISI_L3P_KARMA_DEBUG
	&dev_attr_reg_cfg.attr,
#endif
	NULL,
};

static struct attribute_group dev_attr_group = {
	.name = "karma",
	.attrs = dev_attr,
};

static inline unsigned long read_event(struct event_data *event)
{
	unsigned long ev_count;
	u64 total, enabled, running;

	if (!event->pevent){
		pr_err("%s pevent is NULL\n", __func__);
		return 0;
	}

	total = perf_event_read_value(event->pevent, &enabled, &running);
	ev_count = total - event->prev_count;
	event->prev_count = total;
	return ev_count;

}

static void cpu_read_perf_counters(int cpu, struct cpu_hwmon *hw)
{
	struct cpu_grp_info *cpu_grp = hw->cpu_grp;
	struct cpu_evt_data *evtdata = to_evtdata(cpu_grp, cpu);
	struct cpu_evt_count *evtcnts = to_evtcnts(hw, cpu);

	evtcnts->l3d_refill_cnt = read_event(&evtdata->events[L3D_REFILL_IDX]);
	evtcnts->l3d_cnt = read_event(&evtdata->events[L3D_IDX]);
	evtcnts->inst_cnt = read_event(&evtdata->events[INST_IDX]);
	evtcnts->cyc_cnt = read_event(&evtdata->events[CYC_IDX]);
	pr_debug("cpu = %d, l3d_refill_cnt=%lu, l3d_cnt=%lu\n", cpu, evtcnts->l3d_refill_cnt, evtcnts->l3d_cnt);
	pr_debug("inst_cnt=%lu, cyc_cnt=%lu\n", evtcnts->inst_cnt, evtcnts->cyc_cnt);
	//trace_l3p_karma_read_perf_counters(cpu, evtcnts->l3d_refill_cnt, evtcnts->l3d_cnt);
}

static unsigned long cpu_get_cnt(struct cpu_hwmon *hw)
{
	int cpu;
	struct cpu_grp_info *cpu_grp = hw->cpu_grp;
	struct cpu_evt_count *evtcnts =  NULL;

	hw->total_refill= 0;
	hw->total_access = 0;
	hw->total_inst= 0;
	hw->total_cyc = 0;
	for_each_cpu(cpu, &cpu_grp->inited_cpus){
		cpu_read_perf_counters(cpu, hw);
		evtcnts =  to_evtcnts(hw, cpu);
		hw->total_refill += evtcnts->l3d_refill_cnt;
		hw->total_access += evtcnts->l3d_cnt;
		hw->total_inst += evtcnts->inst_cnt;
		hw->total_cyc += evtcnts->cyc_cnt;
	}

	//trace_cpu_get_cnt(hw->total_refill, hw->total_access);

	return 0;
}


static unsigned long karma_get_cnt(struct karma_hwmon *hw)
{
	struct karma_evt_data *hw_data = &(hw->hw_data);

	hw->count.rep_gd_cnt =
			read_event(&hw_data->events[REP_GOOD_IDX]);

	hw->count.dcp_gd_cnt =
			read_event(&hw_data->events[DCP_GOOD_IDX]);

	hw->count.rep_gen_cnt =
			read_event(&hw_data->events[REP_GEN_IDX]);

	hw->count.dcp_gen_cnt =
			read_event(&hw_data->events[DCP_GEN_IDX]);

	hw->count.acp_drp_rep_cnt =
			read_event(&hw_data->events[ACP_DRP_REP_IDX]);

	hw->count.acp_drp_dcp_cnt =
			read_event(&hw_data->events[ACP_DRP_DCP_IDX]);

	return 0;
}

static void cpu_delete_events(struct cpu_evt_data *evtdata)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(evtdata->events); i++) {
		evtdata->events[i].prev_count = 0;
		if(evtdata->events[i].pevent){
			perf_event_disable(evtdata->events[i].pevent);
			perf_event_release_kernel(evtdata->events[i].pevent);
			evtdata->events[i].pevent = NULL;
		}
	}
}

static void karma_delete_events(struct karma_evt_data *hw_data)
{
	int i;

	for (i = 0; i < KARMA_NUM_EVENTS; i++) {
		hw_data->events[i].prev_count = 0;
		if(hw_data->events[i].pevent){
			perf_event_release_kernel(hw_data->events[i].pevent);
			hw_data->events[i].pevent = NULL;
		}
	}
}

static void cpu_stop_hwmon(struct cpu_hwmon *hw)
{
	int cpu;
	struct cpu_grp_info *cpu_grp = hw->cpu_grp;
	struct cpu_evt_count *evtcnts;

	get_online_cpus();
	for_each_cpu(cpu, &cpu_grp->inited_cpus) {
		cpu_delete_events(to_evtdata(cpu_grp, cpu));
		evtcnts = to_evtcnts(hw, cpu);
		evtcnts->l3d_refill_cnt = 0;
		evtcnts->l3d_cnt = 0;
		evtcnts->inst_cnt = 0;
		evtcnts->cyc_cnt = 0;
		hw->total_refill = 0;
		hw->total_access = 0;
		hw->total_inst = 0;
		hw->total_cyc = 0;
	}
	mutex_lock(&list_lock);
	if (!cpumask_equal(&cpu_grp->cpus, &cpu_grp->inited_cpus))
		list_del(&cpu_grp->mon_list);
	mutex_unlock(&list_lock);
	cpumask_clear(&cpu_grp->inited_cpus);

	put_online_cpus();

	unregister_cpu_notifier(&cpu_grp->perf_cpu_notif);
}


static void karma_stop_hwmon(struct karma_hwmon *hw)
{
	struct karma_evt_data *hw_data = &(hw->hw_data);

	karma_delete_events(hw_data);

	hw->count.rep_gd_cnt = 0;
	hw->count.dcp_gd_cnt = 0;
	hw->count.rep_gen_cnt = 0;
	hw->count.dcp_gen_cnt = 0;
	hw->count.acp_drp_rep_cnt = 0;
	hw->count.acp_drp_dcp_cnt = 0;
}

static struct perf_event_attr *cpu_alloc_attr(void)
{
	struct perf_event_attr *attr;

	attr = kzalloc(sizeof(struct perf_event_attr), GFP_KERNEL);
	if (!attr)
		return attr;

	attr->type = PERF_TYPE_RAW;
	attr->size = sizeof(struct perf_event_attr);
	attr->pinned = 1;
	attr->exclude_idle = 1;

	return attr;
}

static struct perf_event_attr *karma_alloc_attr(void)
{
	struct perf_event_attr *attr;

	attr = kzalloc(sizeof(struct perf_event_attr), GFP_KERNEL);
	if (!attr)
		return attr;

	attr->type = PERF_TYPE_KARMA;
	attr->size = sizeof(struct perf_event_attr);
	attr->pinned = 1;

	return attr;
}

static int cpu_set_events(struct cpu_grp_info *cpu_grp, int cpu)
{
	struct perf_event *pevent;
	struct perf_event_attr *attr;
	int err;
	unsigned int i = 0, j = 0;
	struct cpu_evt_data *evtdata = to_evtdata(cpu_grp, cpu);

	/* Allocate an attribute for event initialization */
	attr = cpu_alloc_attr();
	if (!attr)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(evtdata->events); i++) {
		attr->config = cpu_grp->event_ids[i];
		pevent = perf_event_create_kernel_counter(attr, cpu, NULL,
							  NULL, NULL);
		if (IS_ERR(pevent))
			goto err_out;
		evtdata->events[i].pevent = pevent;
		perf_event_enable(pevent);
	}

	kfree(attr);
	return 0;

err_out:
	pr_err("l3p_karma: fail to create %d events\n", i);
	for (j = 0; j < i; j++) {
		perf_event_disable(evtdata->events[j].pevent);
		perf_event_release_kernel(evtdata->events[j].pevent);
		evtdata->events[j].pevent = NULL;
	}
	err = PTR_ERR(pevent);
	kfree(attr);
	return err;
}

static int karma_set_events(struct karma_hwmon *hw, int cpu)
{
	struct perf_event *pevent= NULL;
	struct perf_event_attr *attr = NULL;
	struct karma_evt_data *hw_data = &hw->hw_data;
	unsigned int i = 0, j = 0;
	int err;

	/* Allocate an attribute for event initialization */
	attr = karma_alloc_attr();
	if (IS_ERR(attr)){
		pr_debug("karma alloc attr failed\n");
		return PTR_ERR(attr);
	}

	for (i = 0; i < ARRAY_SIZE(hw_data->events); i++) {
		attr->config = hw->event_ids[i];
		pr_debug("l3p_karma: i= %d, config = 0x%llx\n", i, attr->config);
		pevent = perf_event_create_kernel_counter(attr, cpu, NULL,
							  NULL, NULL);
		if (IS_ERR(pevent))
			goto err_out;
		hw_data->events[i].pevent = pevent;
		perf_event_enable(pevent);
	}

	kfree(attr);
	return 0;

err_out:
	pr_err("l3p_karma: fail to create %d events\n", i);
	for (j = 0; j < i; j++) {
		perf_event_disable(hw_data->events[j].pevent);
		perf_event_release_kernel(hw_data->events[j].pevent);
		hw_data->events[j].pevent = NULL;
	}
	err = PTR_ERR(pevent);
	kfree(attr);
	return err;
}

static int l3p_karma_cpu_callback(struct notifier_block *nb,
		unsigned long action, void *hcpu)
{
	unsigned long cpu = (unsigned long)hcpu;
	struct cpu_grp_info *cpu_grp, *tmp;

	if (action != CPU_ONLINE)
		return NOTIFY_OK;

	mutex_lock(&list_lock);
	list_for_each_entry_safe(cpu_grp, tmp, &perf_mon_list, mon_list) {
		if (!cpumask_test_cpu(cpu, &cpu_grp->cpus) ||
			cpumask_test_cpu(cpu, &cpu_grp->inited_cpus))
			continue;

		if (cpu_set_events(cpu_grp, cpu))
			pr_warn("Failed to create perf ev for CPU%lu\n", cpu);
		else
			cpumask_set_cpu(cpu, &cpu_grp->inited_cpus);

		if (cpumask_equal(&cpu_grp->cpus, &cpu_grp->inited_cpus)){
			list_del(&cpu_grp->mon_list);
			}
	}
	mutex_unlock(&list_lock);

	return NOTIFY_OK;
}

static int cpu_start_hwmon(struct cpu_hwmon *hw)
{
	int cpu, ret = 0;
	struct cpu_grp_info *cpu_grp = hw->cpu_grp;

	register_cpu_notifier(&cpu_grp->perf_cpu_notif);

	get_online_cpus();
	for_each_cpu(cpu, &cpu_grp->cpus) {
		ret = cpu_set_events(cpu_grp, cpu);
		if (ret) {
			if (!cpu_online(cpu)) {
				ret = 0;
			} else {
				pr_warn("Perf event init failed on CPU%d\n", cpu);
				break;
			}
		} else {
			cpumask_set_cpu(cpu, &cpu_grp->inited_cpus);
		}
	}
	mutex_lock(&list_lock);
	if (!cpumask_equal(&cpu_grp->cpus, &cpu_grp->inited_cpus)){
		list_add_tail(&cpu_grp->mon_list, &perf_mon_list);
	}
	mutex_unlock(&list_lock);

	put_online_cpus();
	return ret;
}

static int karma_start_hwmon(struct karma_hwmon *hw)
{
	int ret = 0;
	/* cpu must be 0*/
	int cpu = 0;

	ret = karma_set_events(hw, cpu);
	if (ret) {
		pr_err("Perf event init failed on CPU%d\n", cpu);
		WARN_ON_ONCE(1);
		return ret;
	}

	return ret;
}

static int cpu_hwmon_setup(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct l3p_karma *l3p = platform_get_drvdata(pdev);
	struct cpu_hwmon *hw = NULL;
	struct cpu_grp_info *cpu_grp = NULL;
	int ret = 0;

	hw = devm_kzalloc(dev, sizeof(*hw), GFP_KERNEL);
	if (IS_ERR_OR_NULL(hw))
		return -ENOMEM;
	l3p->cpu_hw = hw;

	cpu_grp = devm_kzalloc(dev, sizeof(*cpu_grp), GFP_KERNEL);
	if (IS_ERR_OR_NULL(cpu_grp))
		return -ENOMEM;
	hw->cpu_grp = cpu_grp;

	cpu_grp->perf_cpu_notif.notifier_call = l3p_karma_cpu_callback;

	cpumask_copy(&cpu_grp->cpus, cpu_online_mask);
	cpumask_copy(&hw->cpus, &cpu_grp->cpus);

	hw->num_cores = cpumask_weight(&cpu_grp->cpus);
	hw->core_stats = devm_kzalloc(dev, hw->num_cores *
				sizeof(*(hw->core_stats)), GFP_KERNEL);
	if (IS_ERR_OR_NULL(hw->core_stats))
		return -ENOMEM;

	cpu_grp->evtdata = devm_kzalloc(dev, hw->num_cores *
		sizeof(*(cpu_grp->evtdata)), GFP_KERNEL);
	if (IS_ERR_OR_NULL(cpu_grp->evtdata))
		return -ENOMEM;

	cpu_grp->event_ids[L3D_REFILL_IDX] = L3D_REFILL_EV;
	cpu_grp->event_ids[L3D_IDX] = L3D_EV;
	cpu_grp->event_ids[INST_IDX] = INST_EV;
	cpu_grp->event_ids[CYC_IDX] = CYC_EV;

	hw->start_hwmon = &cpu_start_hwmon;
	hw->stop_hwmon = &cpu_stop_hwmon;
	hw->get_cnt = &cpu_get_cnt;

	return ret;

}

static int karma_hwmon_setup(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct l3p_karma *l3p = platform_get_drvdata(pdev);
	struct karma_hwmon *hw = NULL;
	int ret = 0;

	hw = devm_kzalloc(dev, sizeof(*hw), GFP_KERNEL);
	if (IS_ERR_OR_NULL(hw)){
		return -ENOMEM;
	}

	l3p->karma_hw = hw;

	hw->start_hwmon = &karma_start_hwmon;
	hw->stop_hwmon = &karma_stop_hwmon;
	hw->get_cnt = &karma_get_cnt;

	hw->event_ids[REP_GOOD_IDX] = REP_GOOD_EV;
	hw->event_ids[DCP_GOOD_IDX] = DCP_GOOD_EV;
	hw->event_ids[REP_GEN_IDX] = REP_GEN_EV;
	hw->event_ids[DCP_GEN_IDX] = DCP_GEN_EV;
	hw->event_ids[ACP_DRP_REP_IDX] = ACP_DRP_REP_EV;
	hw->event_ids[ACP_DRP_DCP_IDX] = ACP_DRP_DCP_EV;


	pr_debug("karma_hwmon_setup OK\n");
	return ret;
}

static int l3p_karma_target(struct device *dev,
				    unsigned long *freq, u32 flags)
{
	*freq = 0;

	return 0;
}

static int devfreq_get_dev_status(struct device *dev,
					    struct devfreq_dev_status *stat)
{
	struct l3p_karma *l3p = dev_get_drvdata(dev);
	struct karma_hwmon *karma_hw = l3p->karma_hw;
	unsigned int const usec = ktime_to_us(ktime_get());
	unsigned int delta;
	unsigned long hits = 0;
	unsigned long misses = 0;
	unsigned long accesses = 0;
	unsigned long miss_ratio = 0;
	unsigned long l3p_accuracy = 0;
	unsigned long numerator = 0;
	unsigned long denominator = 1;

	delta = usec - l3p->alg.last_update;
	l3p->alg.last_update = usec;

	if (!mutex_trylock(&l3p->mon_mutex_lock)) {
		return 0;
	}

	if (!l3p->mon_started) {
		mutex_unlock(&l3p->mon_mutex_lock); /*lint !e455*/
		return 0;
	}

	cpu_get_cnt(l3p->cpu_hw);
	karma_get_cnt(l3p->karma_hw);
	mutex_unlock(&l3p->mon_mutex_lock); /*lint !e455*/

	misses = l3p->cpu_hw->total_refill;
	accesses = l3p->cpu_hw->total_access;
	hits = accesses - misses;

	l3p->alg.accesses   += accesses;
	l3p->alg.misses     += misses;

	if (!accesses)
		accesses = 1;

	miss_ratio = misses * 1000 / accesses;

	stat->total_time = delta;
	stat->busy_time  = stat->total_time * hits / accesses;

	dev_info(dev, "misses=%lu, accesses=%lu, miss_ratio = %lu permillage\n", misses, accesses, miss_ratio);
	dev_info(dev, "total_inst=%lu, total_cyc=%lu\n", l3p->cpu_hw->total_inst, l3p->cpu_hw->total_cyc);

	numerator = karma_hw->count.rep_gd_cnt + karma_hw->count.dcp_gd_cnt;
	denominator = karma_hw->count.rep_gen_cnt + karma_hw->count.dcp_gen_cnt
		- karma_hw->count.acp_drp_rep_cnt - karma_hw->count.acp_drp_dcp_cnt;

	if(denominator){
		l3p_accuracy = numerator * 100 /denominator;
	}

	dev_info(dev, "rep_gd_cnt = %lu, dcp_gd_cnt = %lu\n", karma_hw->count.rep_gd_cnt, karma_hw->count.dcp_gd_cnt);
	dev_info(dev, "rep_gen_cnt = %lu, dcp_gen_cnt = %lu\n", karma_hw->count.rep_gen_cnt, karma_hw->count.dcp_gen_cnt);
	dev_info(dev, "acp_drp_rep_cnt = %lu, acp_drp_dcp_cnt = %lu\n", karma_hw->count.acp_drp_rep_cnt,  karma_hw->count.acp_drp_dcp_cnt);
	dev_info(dev, "l3p_accuracy = %lu percent\n", l3p_accuracy);

	return 0;
}

static int l3p_gov_get_target(struct devfreq *df,
						  unsigned long *freq)
{
	//struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);
	int err;
	err = devfreq_update_stats(df);
	if (err)
		return err;

	return 0;
}


static int l3p_gov_start(struct devfreq *df)
{
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);
	int ret = 0;

	ret = sysfs_create_group(&df->dev.kobj, l3p->attr_grp);
	if (ret)
		return ret;

	mutex_lock(&l3p->mon_mutex_lock);
	l3p->monitor_enable = 1;
	ret = l3p_start_monitor(df);
	if (ret) {
		l3p->monitor_enable = 0;
		mutex_unlock(&l3p->mon_mutex_lock);
		goto err_start;
	}
	mutex_unlock(&l3p->mon_mutex_lock);

	return 0;

err_start:
	sysfs_remove_group(&df->dev.kobj, l3p->attr_grp);
	return ret;
}


static void l3p_gov_stop(struct devfreq *df)
{
	struct l3p_karma *l3p = df->data;

	sysfs_remove_group(&df->dev.kobj, l3p->attr_grp);
	mutex_lock(&l3p->mon_mutex_lock);
	l3p_stop_monitor(df);
	l3p->monitor_enable = 0;
	mutex_unlock(&l3p->mon_mutex_lock);
}


#define MIN_MS	10U
#define MAX_MS	500U
static int l3p_gov_ev_handler(struct devfreq *devfreq,
					    unsigned int event, void *data)
{
	int ret = 0;
	unsigned int sample_ms;

	switch (event) {
	case DEVFREQ_GOV_START:
		ret = l3p_gov_start(devfreq);
		if(ret)
			return ret;
		dev_dbg(devfreq->dev.parent,
			"Started L3 prefetch governor\n");
		break;

	case DEVFREQ_GOV_STOP:
		l3p_gov_stop(devfreq);
		dev_dbg(devfreq->dev.parent,
			"Stoped L3 prefetch governor\n");
		break;

	case DEVFREQ_GOV_SUSPEND:
		devfreq_monitor_suspend(devfreq);
		break;

	case DEVFREQ_GOV_RESUME:
		devfreq_monitor_resume(devfreq);
		break;

	case DEVFREQ_GOV_INTERVAL:
		sample_ms = *(unsigned int *)data;
		sample_ms = max(MIN_MS, sample_ms);
		sample_ms = min(MAX_MS, sample_ms);

		mutex_lock(&devfreq->lock);
		devfreq->profile->polling_ms = sample_ms;
		mutex_unlock(&devfreq->lock);
		break;
	}

	return ret;
}

static struct devfreq_governor l3p_karma_governor = {
	.name		 = L3P_KARMA_GOVERNOR_NAME,
	.immutable = 1,
	.get_target_freq = l3p_gov_get_target,
	.event_handler	 = l3p_gov_ev_handler,
};

static int l3p_reinit_device(struct device *dev)
{
	struct l3p_karma *l3p = dev_get_drvdata(dev);

	/* Clean the algorithm statistics and start from scrach */
	memset(&l3p->alg, 0, sizeof(l3p->alg));	/* unsafe_function_ignore: memset */
	l3p->alg.last_update = ktime_to_us(ktime_get());

	return 0;

}

static int l3p_setup_devfreq_profile(struct platform_device *pdev)
{
	struct l3p_karma *l3p = platform_get_drvdata(pdev);
	struct devfreq_dev_profile *df_profile;

	l3p->devfreq_profile = devm_kzalloc(&pdev->dev,
			sizeof(struct devfreq_dev_profile), GFP_KERNEL);
	if (IS_ERR_OR_NULL(l3p->devfreq_profile)) {
		dev_err(&pdev->dev, "no memory.\n");
		return PTR_ERR(l3p->devfreq_profile);
	}

	df_profile = l3p->devfreq_profile;

	df_profile->target = l3p_karma_target;
	df_profile->get_dev_status = devfreq_get_dev_status;
	//df_profile->freq_table = l3p->freq_table;
	//df_profile->max_state = l3p->freq_table_len;
	//df_profile->polling_ms = l3p->polling_ms;
	df_profile->polling_ms = 1000;
	//df_profile->polling_ms = L3P_KARMA_DEFAULT_REQUIRED_GOV_POLLING;
	//df_profile->initial_freq = l3p->initial_freq;

	return 0;
}

static int l3p_parse_dt(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct l3p_karma *l3p = platform_get_drvdata(pdev);
	int ret = 0;

	of_node_get(node);

	ret = of_property_read_u32(node, "polling", &l3p->polling_ms);
	if (ret)
		l3p->polling_ms = L3P_KARMA_DEFAULT_POLLING_MS;
	dev_info(&pdev->dev, "polling_ms = %d\n", l3p->polling_ms);

	of_node_put(node);

	return 0;
}

static int l3p_karma_setup(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct l3p_karma *l3p = platform_get_drvdata(pdev);
	int ret = 0;

	l3p->base = devm_ioremap(dev, KARMA_CTRL_BASEADDR, KARMA_CTRL_SIZE);
	if(IS_ERR_OR_NULL(l3p->base)){
		dev_err(dev, "l3p karma ctrl baseaddr ioremap failed.\n");
		return -ENOMEM;
	}

	l3p->attr_grp = &dev_attr_group;
	l3p->monitor_enable = 0;
	l3p->mon_started = false;
	mutex_init(&l3p->mon_mutex_lock);
	spin_lock_init(&l3p->data_spinlock);

	ret = l3p_setup_devfreq_profile(pdev);
	if (ret) {
		dev_err(dev, "device setup devfreq profile failed.\n");
		goto err;
	}

	memset(&l3p->alg, 0, sizeof(l3p->alg));	/* unsafe_function_ignore: memset */
	l3p->alg.last_update = ktime_to_us(ktime_get());

	l3p->devfreq = devm_devfreq_add_device(dev,
					       l3p->devfreq_profile,
					       L3P_KARMA_GOVERNOR_NAME, NULL);

	if (IS_ERR_OR_NULL(l3p->devfreq)) {
		dev_err(dev, "registering to devfreq failed.\n");
		ret = PTR_ERR(l3p->devfreq);
		goto err;
	}

	mutex_lock(&l3p->devfreq->lock);
	//l3p->devfreq->min_freq = l3p->l3p_data->portion_min;
	//l3p->devfreq->max_freq = l3p->l3p_data->portion_max;
	mutex_unlock(&l3p->devfreq->lock);

	l3p_karma_init(l3p);
	l3p_karma_enable(l3p);

	return 0;

err:
	devm_iounmap(dev, l3p->base);
	return ret;
}

#if 0
static void l3p_karma_unsetup(struct platform_device *pdev)
{
	struct l3p_karma *l3p = platform_get_drvdata(pdev);

	devm_devfreq_remove_device(&pdev->dev, l3p->devfreq);
}


static int l3p_karma_init_device(struct platform_device *pdev)
{
	l3p_reinit_device(&pdev->dev);
	return 0;
}
#endif

static void l3p_save_reg_data(struct l3p_karma *l3p)
{
	int i = 0;

	/* not include idle status register */
	for(i = 0; i < (KARMA_SYS_CTRL_NUM - 1); i++){
		pr_debug("i = %d, value = 0x%x\n", i, readl(l3p->base + 4 * i));
		l3p->control_data[i] = readl(l3p->base + 4 * i);
	}
}

static void l3p_restore_reg_data(struct l3p_karma *l3p)
{
	int i = 0;

	/* first restore exclude enable register, at last do it */
	for(i = 1; i < (KARMA_SYS_CTRL_NUM - 1); i++){
		writel(l3p->control_data[i], (l3p->base + 4 * i));
		pr_debug("i = %d, value = 0x%x\n", i, readl(l3p->base + 4 * i));
	}
	writel(l3p->control_data[0], (l3p->base + 0));
	pr_debug("main enable value = 0x%x\n", readl(l3p->base + 0));
}

static int l3p_fcm_idle_notif(struct notifier_block *nb, unsigned long action,
                                                        void *data)
{
	bool fcm_pwrdn = 0;
	struct l3p_karma *l3p = container_of(nb, struct l3p_karma, idle_nb);

	/* device hasn't been initilzed yet */
	if (!l3p->monitor_enable || !l3p->mon_started)
		return NOTIFY_OK;

	switch (action) {
	case CPU_PM_ENTER:
			spin_lock(&l3p->data_spinlock);
			fcm_pwrdn = hisi_fcm_cluster_pwrdn();
			if (fcm_pwrdn) {
				l3p_save_reg_data(l3p);
				l3p->fcm_idle = true;
			}
			spin_unlock(&l3p->data_spinlock);
			break;

	case CPU_PM_ENTER_FAILED:
	case CPU_PM_EXIT:
			spin_lock(&l3p->data_spinlock);
			if(true == l3p->fcm_idle){
				l3p->fcm_idle = false;
				l3p_restore_reg_data(l3p);
			}
			spin_unlock(&l3p->data_spinlock);
			break;
	}

	return NOTIFY_OK;
}

static int l3p_karma_suspend(struct device *dev)
{
	struct l3p_karma *l3p = dev_get_drvdata(dev);
	struct devfreq *devfreq = l3p->devfreq;
	int ret = 0;

	mutex_lock(&l3p->mon_mutex_lock);
	l3p_stop_monitor(devfreq);
	l3p->monitor_enable = 0;
	mutex_unlock(&l3p->mon_mutex_lock);

	/*save all karma contrl register*/
	spin_lock(&l3p->data_spinlock);
	l3p_save_reg_data(l3p);
	spin_unlock(&l3p->data_spinlock);

	return ret;
}

static int l3p_karma_resume(struct device *dev)
{

	struct l3p_karma *l3p = dev_get_drvdata(dev);
	struct devfreq *devfreq = l3p->devfreq;
	int ret = 0;

	l3p_reinit_device(dev);

	/*restore all karma contrl register*/
	spin_lock(&l3p->data_spinlock);
	l3p_restore_reg_data(l3p);
	spin_unlock(&l3p->data_spinlock);

	mutex_lock(&l3p->mon_mutex_lock);
	l3p->monitor_enable = 1;
	ret = l3p_start_monitor(devfreq);
	if (ret) {
		l3p->monitor_enable = 0;
	}
	mutex_unlock(&l3p->mon_mutex_lock);

	return ret;
}

static SIMPLE_DEV_PM_OPS(l3p_karma_pm, l3p_karma_suspend, l3p_karma_resume);



/*lint -e429*/
static int l3p_karma_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct l3p_karma *l3p;
	int ret = 0;

	dev_err(dev, "register l3p karma enter\n");

	l3p = devm_kzalloc(dev, sizeof(*l3p), GFP_KERNEL);
	if (!l3p)
		return -ENOMEM;

	platform_set_drvdata(pdev, l3p);
	l3p->pdev = pdev;

	ret = l3p_parse_dt(pdev);
	if (ret){
		dev_err(dev, "parse dt failed\n");
		goto failed;
	}

	ret = cpu_hwmon_setup(pdev);
	if (ret){
		dev_err(dev, "cpu hwmon setup failed\n");
		goto failed;
	}

	ret = karma_hwmon_setup(pdev);
	if (ret){
		dev_err(dev, "karma hwmon setup failed\n");
		goto failed;
	}

	l3p->idle_nb.notifier_call = l3p_fcm_idle_notif;
	ret = cpu_pm_register_notifier(&l3p->idle_nb);
	if (ret){
		dev_err(dev, "cpu pm register failed\n");
		goto failed;
	}

	ret = l3p_karma_setup(pdev);
	if (ret){
		dev_err(dev, "setup failed\n");
		goto unreg_cpu_pm;
	}

#if 0
	ret = l3p_karma_init_device(pdev);
	if (ret){
		dev_err(dev, "init device failed\n");
		goto failed;
	}
#endif

	dev_err(dev, "register l3p karma exit\n");

	return 0;

unreg_cpu_pm:
	cpu_pm_unregister_notifier(&l3p->idle_nb);
failed:
	dev_err(dev, "failed to register driver, err %d.\n", ret);
	return ret;
}
/*lint +e429*/

static int l3p_karma_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct l3p_karma *l3p = platform_get_drvdata(pdev);
	int ret = 0;

	devm_devfreq_remove_device(dev, l3p->devfreq);
	cpu_pm_unregister_notifier(&l3p->idle_nb);

	return ret;
}

MODULE_DEVICE_TABLE(of, l3p_karma_devfreq_id);

static struct platform_driver l3p_karma_driver = {
	.probe	= l3p_karma_probe,
	.remove = l3p_karma_remove,
	.driver = {
		.name = L3P_KARMA_PLATFORM_DEVICE_NAME,
		.of_match_table = l3p_karma_devfreq_id,
		.pm = &l3p_karma_pm,
		.owner = THIS_MODULE,
	},
};

static int __init l3p_karma_devfreq_init(void)
{
	int ret = 0;

	ret = devfreq_add_governor(&l3p_karma_governor);
	if (ret) {
		pr_err("%s: failed to add governor: %d.\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&l3p_karma_driver);
	if (ret){
		ret = devfreq_remove_governor(&l3p_karma_governor);
		if(ret){
			pr_err("%s: failed to remove governor: %d.\n", __func__, ret);
		}
	}

	return ret;
}

static void __exit l3p_karma_devfreq_exit(void)
{
	int ret;

	ret = devfreq_remove_governor(&l3p_karma_governor);
	if (ret)
		pr_err("%s: failed to remove governor: %d.\n", __func__, ret);

	platform_driver_unregister(&l3p_karma_driver);
}

late_initcall(l3p_karma_devfreq_init)

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("L3CACHE PREFETCHER devfreq driver");
MODULE_VERSION("1.0");
