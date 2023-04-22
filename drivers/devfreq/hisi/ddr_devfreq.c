/*
 * ddr devfreq driver
 *
 * Copyright (c) 2013-2014 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/version.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/devfreq.h>
#include <linux/pm_qos.h>
#include <linux/pm_opp.h>
#include <linux/hisi/hisi_devfreq.h>
#include "governor_pm_qos.h"
#include <linux/clk.h>

/*lint -save -e715 -e785 -e747*/

/*===tele_mntn===*/
#if defined (CONFIG_HISILICON_PLATFORM_TELE_MNTN)
/*===tele_mntn===*/
#include <linux/hisi/hisi_tele_mntn.h>
extern unsigned int get_slice_time(void);

#endif

#define MODULE_NAME "DDR_DEVFREQ"

typedef unsigned long (*calc_vote_value_func)(struct devfreq *devfreq, unsigned long freq);

struct ddr_devfreq_device {
	struct devfreq *devfreq;
	struct clk *set;
	struct clk *get;
	calc_vote_value_func calc_vote_value;
	unsigned long freq;
};

#ifdef CONFIG_INPUT_PULSE_SUPPORT
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/pm_qos.h>
#include <linux/hardirq.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/hisi/pm/pwrctrl_multi_dfs.h>
struct ddrfreq_inputopen {
	struct input_handle *handle;
	struct work_struct inputopen_work;
};

static struct ddrfreq_inputopen inputopen;
static struct workqueue_struct *down_wq;
static struct pm_qos_request ddrfreq_min_req;
atomic_t flag;
struct delayed_work ddrfreq_begin, ddrfreq_end;
static int boost_ddrfreq_switch = 0x0;
static unsigned int boost_ddr_dfs_band = 0;
static unsigned int boost_ddr_dfs_last = 0;
static int saved_jiffies = 0;
#endif
#ifdef CONFIG_INPUT_PULSE_SUPPORT

static void ddrfreq_begin_work(struct work_struct *work)
{
    //printk(KERN_ERR"in %s, %d, %d, %d, %d, %d\n", __func__, in_irq(), in_interrupt(), in_atomic(), jiffies, saved_jiffies);
    pm_qos_add_request(&ddrfreq_min_req, DFS_QOS_ID_DDR_MINFREQ, boost_ddr_dfs_band);
    schedule_delayed_work(&ddrfreq_end, boost_ddr_dfs_last);
}

static void ddrfreq_end_work(struct work_struct *work)
{
    //printk(KERN_ERR"in %s, %d, %d, %d, %d\n", __func__, in_irq(), in_interrupt(), in_atomic(), jiffies);
    pm_qos_remove_request(&ddrfreq_min_req);
    atomic_dec(&flag);
}

/*
 * Pulsed boost on input event raises CPUs to hispeed_freq.
 */
static void ddrfreq_input_event(struct input_handle *handle,
					    unsigned int type,
					    unsigned int code, int value)
{
    saved_jiffies = jiffies;
    if (atomic_read(&flag) == 0x0)
    {
        atomic_inc(&flag);
        schedule_work(&ddrfreq_begin);
    }
}

static void ddrfreq_input_open(struct work_struct *w)
{
	struct ddrfreq_inputopen *io =
		container_of(w, struct ddrfreq_inputopen,
			     inputopen_work);
	int error;

	error = input_open_device(io->handle);
	if (error)
		input_unregister_handle(io->handle);
}

static int ddrfreq_input_connect(struct input_handler *handler,
					     struct input_dev *dev,
					     const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	pr_info("%s: connect to %s\n", __func__, dev->name);
	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "ddrfreq";

	error = input_register_handle(handle);
	if (error)
		goto err;

	inputopen.handle = handle;
	queue_work(down_wq, &inputopen.inputopen_work);
	return 0;
err:
	kfree(handle);
	return error;
}

static void ddrfreq_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id ddrfreq_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT |
			 INPUT_DEVICE_ID_MATCH_ABSBIT,
		.evbit = { BIT_MASK(EV_ABS) },
		.absbit = { [BIT_WORD(ABS_MT_POSITION_X)] =
			    BIT_MASK(ABS_MT_POSITION_X) |
			    BIT_MASK(ABS_MT_POSITION_Y) },
	}, /* multi-touch touchscreen */
	{
		.flags = INPUT_DEVICE_ID_MATCH_KEYBIT |
			 INPUT_DEVICE_ID_MATCH_ABSBIT,
		.keybit = { [BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH) },
		.absbit = { [BIT_WORD(ABS_X)] =
			    BIT_MASK(ABS_X) | BIT_MASK(ABS_Y) },
	}, /* touchpad */
	{ },
};

static struct input_handler ddrfreq_input_handler = {
	.event          = ddrfreq_input_event,
	.connect        = ddrfreq_input_connect,
	.disconnect     = ddrfreq_input_disconnect,
	.name           = "ddrfreq",
	.id_table       = ddrfreq_ids,
};

#endif /*CONFIG_INPUT_PULSE_SUPPORT*/
/*===tele_mntn===*/



/*===tele_mntn===*/
#if defined (CONFIG_HISILICON_PLATFORM_TELE_MNTN)
static void tele_mntn_ddrfreq_setrate(struct devfreq *devfreq, unsigned int new_freq)
{
    ACORE_TELE_MNTN_DFS_DDR_QOS_STRU *qos = NULL;
    ACORE_TELE_MNTN_DFS_DDR_QOSINFO_STRU * info = NULL;
    struct devfreq_pm_qos_data *data = devfreq->data;

    if(!p_acore_tele_mntn)
        return;

    qos = &(p_acore_tele_mntn->dfsDdr.qos);
    info = &(qos->info);
    info->qos_id = (short)data->pm_qos_class;
    if(current) {
        info->pid = current->pid;
        if(current->parent)
            info->ppid = current->parent->pid;
    }

    info->new_freq = new_freq;
    info->min_freq = (unsigned int)devfreq->min_freq;
    info->max_freq = (unsigned int)devfreq->max_freq;
    qos->qosSliceTime = get_slice_time();
    (void)tele_mntn_write_log(TELE_MNTN_QOS_DDR_ACPU, sizeof(ACORE_TELE_MNTN_DFS_DDR_QOS_STRU), (void *)qos);
}

#endif

static struct devfreq_pm_qos_data ddr_devfreq_latency_pm_qos_data = {
	.pm_qos_class = PM_QOS_MEMORY_LATENCY,
	.freq = 0,
};

static struct devfreq_pm_qos_data ddr_devfreq_pm_qos_data = {
	.pm_qos_class = PM_QOS_MEMORY_THROUGHPUT,
	.freq = 0,
};

static struct devfreq_pm_qos_data ddr_devfreq_up_th_pm_qos_data = {
	.pm_qos_class = PM_QOS_MEMORY_THROUGHPUT_UP_THRESHOLD,
	.freq = 0,
};

#ifdef CONFIG_DDR_HW_VOTE_15BITS
#define VOTE_MAX_VALUE	(0x7FFF)
#define VOTE_QUOTIENT(vote)	(vote)
#define VOTE_REMAINDER(vote)	(0)
#else
#define VOTE_MAX_VALUE	(0xFF)
#define VOTE_QUOTIENT(vote)	((vote) >> 4)
#define VOTE_REMAINDER(vote)	((vote) & 0xF)
#endif
#define FREQ_HZ	(1000000)

static unsigned long calc_vote_value_hw(struct devfreq *devfreq, unsigned long freq)
{
	unsigned long freq_hz = freq / FREQ_HZ;
	unsigned long quotient = VOTE_QUOTIENT(freq_hz);
	unsigned long remainder = VOTE_REMAINDER(freq_hz);
	unsigned long x_quotient, x_remainder;
	unsigned int lev;

	for (lev = 0; lev < devfreq->profile->max_state; lev++)
	{
		x_quotient = VOTE_QUOTIENT(devfreq->profile->freq_table[lev] / FREQ_HZ);
		if (quotient == x_quotient)
		{
			x_remainder = VOTE_REMAINDER(devfreq->profile->freq_table[lev] / FREQ_HZ);
			if (remainder > x_remainder)
			{
				quotient ++;
			}
			break;
		}else if (quotient < x_quotient)
			break;
	}

	if (quotient > VOTE_MAX_VALUE) {
		quotient = VOTE_MAX_VALUE;
	}

	return (quotient * FREQ_HZ);
}

static unsigned long calc_vote_value_ipc(struct devfreq *devfreq, unsigned long freq)
{
	return freq;
}

/*static unsigned long min_freq_table[2];*/
static int ddr_devfreq_target(struct device *dev, unsigned long *freq, u32 flags)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);  //lint !e826
	struct ddr_devfreq_device *ddev = platform_get_drvdata(pdev);
	struct devfreq_pm_qos_data *data;
	struct devfreq *devfreq;
	unsigned long _freq = *freq;
	int lev;

	if(NULL == ddev)
	{
		return -1;
	}

	devfreq = ddev->devfreq;
	data = devfreq->data;

	if (!data) {
		pr_err("%s %d, invalid pointer\n", __func__, __LINE__);
		return -EINVAL;
	}

	if ((data->pm_qos_class == PM_QOS_MEMORY_LATENCY)
		|| (data->pm_qos_class == PM_QOS_MEMORY_THROUGHPUT))
	{
		unsigned long cur_freq;
		/* get current min freq */
		cur_freq = (ddr_devfreq_latency_pm_qos_data.freq >= ddr_devfreq_pm_qos_data.freq) ?
			ddr_devfreq_latency_pm_qos_data.freq : ddr_devfreq_pm_qos_data.freq;

		/* update freq */
		data->freq = *freq;
		*freq = (ddr_devfreq_latency_pm_qos_data.freq > ddr_devfreq_pm_qos_data.freq) ?
			ddr_devfreq_latency_pm_qos_data.freq : ddr_devfreq_pm_qos_data.freq;

		if (cur_freq != *freq)
		{
			(void)clk_set_rate(ddev->set, ddev->calc_vote_value(devfreq, *freq));
			ddev->freq = *freq;
		}
	}
	else
	{
		data->freq = *freq;
		if (ddev->freq != *freq) {
			/* undate ddr freqency down threshold */
			(void)clk_set_rate(ddev->set, ddev->calc_vote_value(devfreq, *freq));
			ddev->freq = *freq;
		}
	}


	/* fix: fail to update devfreq freq_talbe state. */
	*freq = clk_get_rate(ddev->get);

	/* check */
	for (lev = 0; lev < devfreq->profile->max_state; lev++)	//lint !e574 !e737
		if (*freq == devfreq->profile->freq_table[lev])
			goto out;

	/* exception */
	pr_err("\n");
	dev_err(dev,
		"odd freq status.\n<Target: %09lu hz>\n<Status: %09lu hz>\n%s",
		_freq, *freq, "--- freq table ---\n");
	for (lev = 0; lev < devfreq->profile->max_state; lev++) {	//lint !e574 !e737
		pr_err("<%d> %09lu hz\n",
			lev, (unsigned long)(devfreq->profile->freq_table[lev]));
	}
	pr_err("------- end ------\n");

out:
/*===tele_mntn===*/
/*===tele_mntn===*/

#if defined (CONFIG_HISILICON_PLATFORM_TELE_MNTN)
    tele_mntn_ddrfreq_setrate(devfreq, (unsigned int)ddev->freq);
#endif

	return 0;
}

/*
 * we can ignore setting current devfreq state,
 * because governor, "pm_qos", could get status through pm qos.
 */
static int
ddr_devfreq_get_dev_status(struct device *dev, struct devfreq_dev_status *stat)
{
	return 0;
}

static int ddr_devfreq_get_cur_status(struct device *dev, unsigned long *freq)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);	/*lint !e826 */
	struct ddr_devfreq_device *ddev = platform_get_drvdata(pdev);

	if(NULL == ddev)
	{
		return -1;
	}
	*freq = clk_get_rate(ddev->get);
	return 0;
}

static struct devfreq_dev_profile ddr_devfreq_dev_profile = {
	.polling_ms		= 0,
	.target			= ddr_devfreq_target,
	.get_dev_status		= ddr_devfreq_get_dev_status,
	.get_cur_freq		= ddr_devfreq_get_cur_status,
	.freq_table		= NULL,
	.max_state		= 0,
};

/*lint -e429*/
static int ddr_devfreq_probe(struct platform_device *pdev)
{
    struct ddr_devfreq_device *ddev = NULL;
    struct device_node *np = pdev->dev.of_node;
    struct devfreq_pm_qos_data *ddata = NULL;
    const char *type = NULL;
    const char *vote_method = NULL;
    int ret = 0;

#ifdef CONFIG_INPUT_PULSE_SUPPORT
    int rc = 0;
    static int inited = 0;
    struct device_node *root = NULL;
    if (inited == 0)
    {
        root = of_find_compatible_node(NULL, NULL, "hisilicon,ddrfreq_boost");
        if (!root)
        {
            pr_err("%s hisilicon,ddrfreq_boost no root node\n",__func__);
        }
        else
        {
            of_property_read_u32_array(root, "switch-value", &boost_ddrfreq_switch, 0x1);
            pr_err("switch-value: %d", boost_ddrfreq_switch);

            if (boost_ddrfreq_switch != 0x0)
            {
                of_property_read_u32_array(root, "ddrfreq_dfs_value", &boost_ddr_dfs_band, 0x1);
                of_property_read_u32_array(root, "ddrfreq_dfs_last", &boost_ddr_dfs_last, 0x1);
                pr_err("boost_ddr_dfs_band: %d, boost_ddr_dfs_last: %d\n", boost_ddr_dfs_band, boost_ddr_dfs_last);
                rc = input_register_handler(&ddrfreq_input_handler);
                if (rc)
                    pr_warn("%s: failed to register input handler\n",
                            __func__);

            down_wq = alloc_workqueue("ddrfreq_down", 0, 1);

            if (!down_wq)
                return -ENOMEM;

                INIT_WORK(&inputopen.inputopen_work, ddrfreq_input_open);
                ddrfreq_min_req.pm_qos_class = 0;
                atomic_set(&flag, 0x0);
                INIT_DELAYED_WORK(&ddrfreq_begin, (work_func_t)ddrfreq_begin_work);
                INIT_DELAYED_WORK(&ddrfreq_end, (work_func_t)ddrfreq_end_work);
        }
        }
        inited = 1;
    }

#endif /*#ifdef CONFIG_INPUT_PULSE_SUPPORT*/

	if (!np) {
		pr_err("%s: %s %d, no device node\n",
			MODULE_NAME, __func__, __LINE__);
		ret = -ENODEV;
		goto out;
	}

	ret = of_property_read_string(np, "pm_qos_class", &type);
	if (ret) {
		pr_err("%s: %s %d, no type\n",
			MODULE_NAME, __func__, __LINE__);
		ret = -EINVAL;
		goto no_type;
	}

	if (!strncmp("memory_latency", type, strlen(type) + 1)) {
		ret=of_property_read_u32_array(np, "pm_qos_data_reg", (u32 *)&ddr_devfreq_latency_pm_qos_data, 0x2);
		if (ret) {
			pr_err("%s: %s %d, no type\n",
			MODULE_NAME, __func__, __LINE__);
		}
		pr_err("%s: %s %d, per_hz %d  utilization %d\n",
			MODULE_NAME, __func__, __LINE__,ddr_devfreq_latency_pm_qos_data.bytes_per_sec_per_hz,ddr_devfreq_latency_pm_qos_data.bd_utilization);
		ddata = &ddr_devfreq_latency_pm_qos_data;
		dev_set_name(&pdev->dev, "ddrfreq_latency");
	}
	else if (!strncmp("memory_tput", type, strlen(type) + 1)) {
		ret=of_property_read_u32_array(np, "pm_qos_data_reg", (u32 *)&ddr_devfreq_pm_qos_data, 0x2);
		if (ret) {
			pr_err("%s: %s %d, no type\n",
			MODULE_NAME, __func__, __LINE__);
		}
		pr_err("%s: %s %d, per_hz %d  utilization %d\n",
			MODULE_NAME, __func__, __LINE__,ddr_devfreq_pm_qos_data.bytes_per_sec_per_hz,ddr_devfreq_pm_qos_data.bd_utilization);
		ddata = &ddr_devfreq_pm_qos_data;
		dev_set_name(&pdev->dev, "ddrfreq");
	} else if (!strncmp("memory_tput_up_threshold", type, strlen(type) + 1)) {
		ret = of_property_read_u32_array(np, "pm_qos_data_reg", (u32 *) &ddr_devfreq_up_th_pm_qos_data, 0x2);
		if (ret) {
			pr_err("%s: %s %d, no type\n",
			MODULE_NAME, __func__, __LINE__);
		}
		pr_err("%s: %s %d, per_hz %d  utilization %d\n",
			MODULE_NAME, __func__, __LINE__,ddr_devfreq_up_th_pm_qos_data.bytes_per_sec_per_hz,ddr_devfreq_up_th_pm_qos_data.bd_utilization);
		ddata = &ddr_devfreq_up_th_pm_qos_data;
		dev_set_name(&pdev->dev, "ddrfreq_up_threshold");
	} else {
		pr_err("%s: %s %d, err type\n",
			MODULE_NAME, __func__, __LINE__);
		ret = -EINVAL;
		goto err_type;
	}

	ddev = kzalloc(sizeof(struct ddr_devfreq_device), GFP_KERNEL);
	if (!ddev) {
		pr_err("%s: %s %d, no mem\n",
			MODULE_NAME, __func__, __LINE__);
		ret = -ENOMEM;
		goto no_men;
	}

	ddev->set = of_clk_get(np, 0);
	if (IS_ERR(ddev->set)) {
		pr_err("%s: %s %d, Failed to get set-clk\n",
			MODULE_NAME, __func__, __LINE__);
		ret = -ENODEV;
		goto no_clk1;
	}

	ddev->get = of_clk_get(np, 1);
	if (IS_ERR(ddev->get)) {
		pr_err("%s: %s %d, Failed to get get-clk\n",
			MODULE_NAME, __func__, __LINE__);
		ret = -ENODEV;
		goto no_clk2;
	}

	if (of_property_read_string(np, "vote_method", &vote_method)) {
		ddev->calc_vote_value = calc_vote_value_ipc;
	} else if (!strncmp("hardware", vote_method, strlen(vote_method) + 1)) {
		ddev->calc_vote_value = calc_vote_value_hw;
	} else {
		ddev->calc_vote_value = calc_vote_value_ipc;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	if (dev_pm_opp_of_add_table(&pdev->dev))
#else
	if (dev_pm_opp_of_add_table(&pdev->dev) ||
		hisi_devfreq_init_freq_table(&pdev->dev,
			&ddr_devfreq_dev_profile.freq_table))
#endif
	{
		ddev->devfreq = NULL;
	} else {
		ddr_devfreq_dev_profile.initial_freq = clk_get_rate(ddev->get);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
		rcu_read_lock();
		ddr_devfreq_dev_profile.max_state = dev_pm_opp_get_opp_count(&pdev->dev); /*lint !e732 */
		rcu_read_unlock();
#endif
		ddev->devfreq = devm_devfreq_add_device(&pdev->dev,
					&ddr_devfreq_dev_profile,
					"pm_qos",
					ddata);
	}
	if (IS_ERR_OR_NULL(ddev->devfreq)) {
		pr_err("%s: %s %d, <%s>, Failed to init ddr devfreq_table\n",
			MODULE_NAME, __func__, __LINE__, type);
		ret = -ENODEV;
		goto no_devfreq;
	}

	/*
	 *	cache value.
	 *	It does not mean actual ddr clk currently,
	 *	but a frequency cache of every clk setting in the module.
	 *	Because, it is not obligatory that setting value is equal to
	 *	the currently actual ddr clk frequency.
	 */
	ddev->freq = 0;
	if(ddev->devfreq) {
		ddev->devfreq->max_freq = ddr_devfreq_dev_profile.freq_table[ddr_devfreq_dev_profile.max_state - 1];
		ddev->devfreq->min_freq = ddr_devfreq_dev_profile.freq_table[ddr_devfreq_dev_profile.max_state - 1];
	}

	platform_set_drvdata(pdev, ddev);

	pr_info("%s: <%s> ready\n", MODULE_NAME, type);
	return ret;

no_devfreq:
	clk_put(ddev->get);
	hisi_devfreq_free_freq_table(&pdev->dev,
            &ddr_devfreq_dev_profile.freq_table);
no_clk2:
	clk_put(ddev->set);
no_clk1:
	kfree(ddev);
no_men:
err_type:
no_type:
out:
	return ret;
}
/*lint +e429*/

static int ddr_devfreq_remove(struct platform_device *pdev)
{
	struct ddr_devfreq_device *ddev;
	struct devfreq_dev_profile *profile;

	ddev = platform_get_drvdata(pdev);
	if(NULL == ddev)
	{
		return -1;
	}
	profile = ddev->devfreq->profile;

	hisi_devfreq_free_freq_table(&pdev->dev, &profile->freq_table);
	devfreq_remove_device(ddev->devfreq);

	platform_set_drvdata(pdev, NULL);

	clk_put(ddev->get);
	clk_put(ddev->set);

	kfree(ddev);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id ddr_devfreq_of_match[] = {
	{.compatible = "hisilicon,ddr_devfreq",},
	{.compatible = "hisilicon,ddr_devfreq_latency",},
	{.compatible = "hisilicon,ddr_devfreq_up_threshold",},
	{},
};
MODULE_DEVICE_TABLE(of, ddr_devfreq_of_match);
#endif
/*lint -restore */

/*lint -e64 -e785 */
static struct platform_driver ddr_devfreq_driver = {
	.probe = ddr_devfreq_probe,
	.remove = ddr_devfreq_remove,
	.driver = {
		.name = "ddr_devfreq",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ddr_devfreq_of_match),
	},
};
/*lint +e64 +e785 */

/*lint -e528 -esym(528,*) -e64 -esym(64,*) */
module_platform_driver(ddr_devfreq_driver);
/*lint -e528 +esym(528,*) +e64 +esym(64,*) */

