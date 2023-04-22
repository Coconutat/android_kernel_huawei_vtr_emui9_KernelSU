/*
 *
 * (C) COPYRIGHT ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */





#include <linux/ioport.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#ifdef CONFIG_DEVFREQ_THERMAL
#include <linux/devfreq_cooling.h>
#endif

#include <trace/events/power.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
#include <linux/pm_opp.h>
#else
#include <linux/opp.h>
#endif

#include "mali_kbase_hisi_callback.h"

#ifdef CONFIG_PM_DEVFREQ
#include <linux/hisi/hisi_devfreq.h>
#endif

#ifdef CONFIG_HISI_HW_VOTE_GPU_FREQ
#include <linux/hisi/hisi_hw_vote.h>
#endif

#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_REPORT_VSYNC
#include <linux/export.h>
#endif
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include <backend/gpu/mali_kbase_device_internal.h>
#include "mali_kbase_config_platform.h"
#include "mali_kbase_config_hifeatures.h"
#ifdef CONFIG_HISI_IPA_THERMAL
#include <linux/thermal.h>
#endif

#include <linux/hisi/hisi_gpufreq.h>

#define MALI_TRUE ((uint32_t)1)
#define MALI_FALSE ((uint32_t)0)
typedef uint32_t     mali_bool;

typedef enum {
        MALI_ERROR_NONE = 0,
        MALI_ERROR_OUT_OF_GPU_MEMORY,
        MALI_ERROR_OUT_OF_MEMORY,
        MALI_ERROR_FUNCTION_FAILED,
}mali_error;

#define HARD_RESET_AT_POWER_OFF 0

#ifndef CONFIG_OF
static struct kbase_io_resources io_resources = {
	.job_irq_number = 68,
	.mmu_irq_number = 69,
	.gpu_irq_number = 70,
	.io_memory_region = {
	.start = 0xFC010000,
	.end = 0xFC010000 + (4096 * 4) - 1
	}
};
#endif /* CONFIG_OF */


#define DEFAULT_POLLING_MS        20


#define RUNTIME_PM_DELAY_1MS      1
#define RUNTIME_PM_DELAY_30MS    30

#ifdef CONFIG_REPORT_VSYNC
static struct kbase_device *kbase_dev = NULL;
#endif


#define KHz	(1000)
#ifdef CONFIG_HISI_HW_VOTE_GPU_FREQ
static struct hvdev *gpu_hvdev = NULL;
#endif



static int kbase_set_hi_features_mask(struct kbase_device *kbdev)
{
	const enum kbase_hi_feature *hi_features;
	u32 gpu_vid;
	u32 product_id;

	gpu_vid = kbdev->gpu_vid;
	product_id = gpu_vid & GPU_ID_VERSION_PRODUCT_ID;
	product_id >>= GPU_ID_VERSION_PRODUCT_ID_SHIFT;

	if (GPU_ID_IS_NEW_FORMAT(product_id)) {
		switch (gpu_vid) {
		case GPU_ID2_MAKE(6, 0, 10, 0, 0, 0, 2):
			hi_features = kbase_hi_feature_tMIx_r0p0;
			break;
		case GPU_ID2_MAKE(6, 2, 2, 1, 0, 0, 0):
			hi_features = kbase_hi_feature_tHEx_r0p0;
			break;
		case GPU_ID2_MAKE(6, 2, 2, 1, 0, 0, 1):
			hi_features = kbase_hi_feature_tHEx_r0p0;
			break;
		case GPU_ID2_MAKE(7, 2, 1, 1, 0, 0, 0):
			hi_features = kbase_hi_feature_tNOx_r0p0;
			break;
		case GPU_ID2_MAKE(7, 4, 0, 2, 1, 0, 0):
			hi_features = kbase_hi_feature_tGOx_r1p0;
			break;
		case GPU_ID2_MAKE(7, 0, 9, 0, 1, 1, 0):
			hi_features = kbase_hi_feature_tSIx_r1p1;
			break;
		default:
			dev_err(kbdev->dev,
				"[hi-feature]Unknown GPU ID %x", gpu_vid);
			return -EINVAL;
		}
	} else {
		switch (gpu_vid) {
		case GPU_ID_MAKE(GPU_ID_PI_TFRX, 0, 2, 0):
			hi_features = kbase_hi_feature_t880_r0p2;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T83X, 1, 0, 0):
			hi_features = kbase_hi_feature_t830_r2p0;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_TFRX, 2, 0, 0):
			hi_features = kbase_hi_feature_t880_r2p0;
			break;
		default:
			dev_err(kbdev->dev,
				"[hi-feature]Unknown GPU ID %x", gpu_vid);
			return -EINVAL;
		}
	}

	dev_info(kbdev->dev, "[hi-feature]GPU identified as 0x%04x r%dp%d status %d",
		(gpu_vid & GPU_ID_VERSION_PRODUCT_ID) >> GPU_ID_VERSION_PRODUCT_ID_SHIFT,
		(gpu_vid & GPU_ID_VERSION_MAJOR) >> GPU_ID_VERSION_MAJOR_SHIFT,
		(gpu_vid & GPU_ID_VERSION_MINOR) >> GPU_ID_VERSION_MINOR_SHIFT,
		(gpu_vid & GPU_ID_VERSION_STATUS) >> GPU_ID_VERSION_STATUS_SHIFT);

	for (; *hi_features != KBASE_HI_FEATURE_END; hi_features++)
		set_bit(*hi_features, &kbdev->hi_features_mask[0]);

	return 0;
}

static inline void kbase_platform_on(struct kbase_device *kbdev)
{
	if (kbdev->regulator) {
		if (unlikely(regulator_enable(kbdev->regulator))) {
			dev_err(kbdev->dev, "Failed to enable regulator\n");
			BUG_ON(1);
		}

		kbdev->regulator_refcount++;
		if(1 != kbdev->regulator_refcount)
		{
			dev_err(kbdev->dev, "kbase_platform_on called not match, regulator_refcount is %d\n", kbdev->regulator_refcount);
		}

		if (kbdev->gpu_vid == 0) {
			kbdev->gpu_vid = kbase_reg_read(kbdev, GPU_CONTROL_REG(GPU_ID));
			if (unlikely(kbase_set_hi_features_mask(kbdev))) {
				dev_err(kbdev->dev, "Failed to set hi features\n");
			}
		}

		if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_HI0004)) {
			kbase_reg_write(kbdev, GPU_CONTROL_REG(PWR_KEY), KBASE_PWR_KEY_VALUE);
			kbase_reg_write(kbdev, GPU_CONTROL_REG(PWR_OVERRIDE1), KBASE_PWR_OVERRIDE_VALUE);
		}

		if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_HI0003)) {
			int value = 0;
			value = readl(kbdev->pctrlreg + PERI_CTRL19) & GPU_X2P_GATOR_BYPASS;
			writel(value, kbdev->pctrlreg + PERI_CTRL19);
		}
	}
}

static inline void kbase_platform_off(struct kbase_device *kbdev)
{

	if (kbdev->regulator) {
		if (unlikely(regulator_disable(kbdev->regulator))) {
			dev_err(kbdev->dev, "MALI-MIDGARD: Failed to disable regulator\n");
		}
		kbdev->regulator_refcount--;
		if(0 != kbdev->regulator_refcount)
		{
			dev_err(kbdev->dev, "kbase_platform_off called not match, regulator_tick is %d\n", kbdev->regulator_refcount);
		}
	}

}

#ifdef CONFIG_PM_DEVFREQ
static struct hisi_devfreq_data hisi_devfreq_priv_data = {
	.vsync_hit = 0,
	.cl_boost = 0,
};

static int mali_kbase_devfreq_target(struct device *dev, unsigned long *_freq,
			      u32 flags)
{
	struct kbase_device *kbdev = (struct kbase_device *)dev->platform_data;
	unsigned long old_freq = kbdev->devfreq->previous_freq;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	struct dev_pm_opp *opp = NULL;
#else
	struct opp *opp = NULL;
#endif
	unsigned long freq;

	rcu_read_lock();
	opp = devfreq_recommended_opp(dev, _freq, flags);
	if (IS_ERR(opp)) {
		pr_err("[mali]  Failed to get Operating Performance Point\n");
		rcu_read_unlock();
		return PTR_ERR(opp);
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	freq = dev_pm_opp_get_freq(opp);
#else
	freq = opp_get_freq(opp);
#endif
	rcu_read_unlock();

#ifdef CONFIG_HISI_IPA_THERMAL
	freq = ipa_freq_limit(IPA_GPU,freq);
#endif

	if (old_freq == freq)
		goto update_target;

	trace_clock_set_rate("clk-g3d",freq,raw_smp_processor_id());

#ifdef CONFIG_HISI_HW_VOTE_GPU_FREQ
	if (hisi_hv_set_freq(gpu_hvdev, freq / KHz)) {
#else
	if (clk_set_rate((kbdev->clk), freq)) {
#endif
		pr_err("[mali]  Failed to set gpu freqency, [%lu->%lu]\n", old_freq, freq);
		return -ENODEV;
	}

update_target:
	*_freq = freq;

	return 0;
}

#ifdef CONFIG_MALI_BOUND_REPORT
static bool mali_kbase_bound_report(struct kbase_device *kbdev,bool bound_event)
{
	bool out_data, bound_it = false;
	if(NULL == kbdev)
		return false;

	if(bound_event)
		kbdev->bound_report_info.bound_times_in_fifo ++;
	if(kfifo_is_full(&kbdev->bound_report_info.bound_fifo)){
		kfifo_out(&kbdev->bound_report_info.bound_fifo,&out_data,1);
		if(out_data)
			kbdev->bound_report_info.bound_times_in_fifo --;
	}
	kfifo_in(&kbdev->bound_report_info.bound_fifo,&bound_event,1);

	if(kbdev->bound_report_info.bound_times_in_fifo > 3)
		bound_it = true;	//bound detected!!!

	-- kbdev->bound_report_info.duration_times;
	if(bound_it != kbdev->bound_report_info.report_bound_flag){		//new bound state is changed
		if(kbdev->bound_report_info.duration_times <= 0){			//check the period, make sure the previous flag state last at least for a while(BOUND_DURATION_TIMES*20 ms)
			kbdev->bound_report_info.report_bound_flag = bound_it;
			kbdev->bound_report_info.duration_times = BOUND_DURATION_TIMES;
		}
	}

	return kbdev->bound_report_info.report_bound_flag;
}
#endif

#ifdef CONFIG_DEVFREQ_THERMAL
void mali_kbase_devfreq_detect_bound_worker(struct work_struct *work)
{
	int err;
	struct kbase_device *kbdev = container_of(work,
					struct kbase_device, bound_detect_work);

	bool bound_event = false;
	struct thermal_cooling_device *cdev = kbdev->devfreq_cooling;

#if defined(CONFIG_MALI_MIDGARD_DVFS)
	bound_event = kbase_ipa_dynamic_bound_detect(kbdev->ipa_ctx, &err, kbdev->bound_detect_freq, kbdev->bound_detect_btime, cdev->ipa_enabled);
#endif

	cdev->ipa_enabled = false;

	cdev->bound_event = bound_event;

#ifdef CONFIG_MALI_BOUND_REPORT
	kbdev->bound_report_info.report_bound_flag = mali_kbase_bound_report(kbdev,bound_event);
#endif

}

void mali_kbase_devfreq_detect_bound(struct kbase_device *kbdev,
		unsigned long cur_freq,
		unsigned long btime)
{
	kbdev->bound_detect_freq = cur_freq;
	kbdev->bound_detect_btime = btime;
	queue_work(system_unbound_wq, &kbdev->bound_detect_work);
}
#endif

static int mali_kbase_get_dev_status(struct device *dev,
				      struct devfreq_dev_status *stat)
{
	struct hisi_devfreq_data *priv_data = &hisi_devfreq_priv_data;
	struct kbase_device *kbdev = (struct kbase_device *)dev->platform_data;
#ifdef CONFIG_HISI_HW_VOTE_GPU_FREQ
	u32 freq = 0;
#endif

	if (kbdev->pm.backend.metrics.kbdev != kbdev) {
		pr_err("%s pm backend metrics not initialized\n", __func__);
		return -EINVAL;
	}

	(void)kbase_pm_get_dvfs_action(kbdev);
	stat->busy_time = kbdev->pm.backend.metrics.utilisation;
	stat->total_time = 100;
#ifdef CONFIG_HISI_HW_VOTE_GPU_FREQ
	hisi_hv_get_last(gpu_hvdev, &freq);
	stat->current_frequency = (unsigned long)freq * KHz;
#else
	stat->current_frequency = clk_get_rate(kbdev->clk);
#endif
	priv_data->vsync_hit = kbdev->pm.backend.metrics.vsync_hit;
	priv_data->cl_boost = kbdev->pm.backend.metrics.cl_boost;
	stat->private_data = (void *)priv_data;

#ifdef CONFIG_DEVFREQ_THERMAL
	/*Avoid sending HWC dump cmd to GPU when GPU is power-off*/
	if (kbdev->pm.backend.gpu_powered)
		(void)mali_kbase_devfreq_detect_bound(kbdev, stat->current_frequency, stat->busy_time);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,15)
	memcpy(&kbdev->devfreq->last_status, stat, sizeof(*stat));
#else
	memcpy(&kbdev->devfreq_cooling->last_status, stat, sizeof(*stat));
#endif
#endif

	return 0;
}

static struct devfreq_dev_profile mali_kbase_devfreq_profile = {
	/* it would be abnormal to enable devfreq monitor during initialization. */
	.polling_ms	= DEFAULT_POLLING_MS, //STOP_POLLING,
	.target		= mali_kbase_devfreq_target,
	.get_dev_status	= mali_kbase_get_dev_status,
};
#endif

#ifdef CONFIG_REPORT_VSYNC
void mali_kbase_pm_report_vsync(int buffer_updated)
{
	unsigned long flags;

	if (kbase_dev){
		spin_lock_irqsave(&kbase_dev->pm.backend.metrics.lock, flags);
		kbase_dev->pm.backend.metrics.vsync_hit = buffer_updated;
		spin_unlock_irqrestore(&kbase_dev->pm.backend.metrics.lock, flags);
	}
}
EXPORT_SYMBOL(mali_kbase_pm_report_vsync);
#endif

#ifdef CONFIG_MALI_MIDGARD_DVFS
int kbase_platform_dvfs_event(struct kbase_device *kbdev, u32 utilisation, u32 util_gl_share, u32 util_cl_share[2])
{
	return 1;
}

int kbase_platform_dvfs_enable(struct kbase_device *kbdev, bool enable, int freq)
{
	unsigned long flags;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	if (enable != kbdev->pm.backend.metrics.timer_active) {
		if (enable) {
			spin_lock_irqsave(&kbdev->pm.backend.metrics.lock, flags);
			kbdev->pm.backend.metrics.timer_active = MALI_TRUE;
			spin_unlock_irqrestore(&kbdev->pm.backend.metrics.lock, flags);
			hrtimer_start(&kbdev->pm.backend.metrics.timer,
					HR_TIMER_DELAY_MSEC(kbdev->pm.dvfs_period),
					HRTIMER_MODE_REL);
		} else {
			spin_lock_irqsave(&kbdev->pm.backend.metrics.lock, flags);
			kbdev->pm.backend.metrics.timer_active = MALI_FALSE;
			spin_unlock_irqrestore(&kbdev->pm.backend.metrics.lock, flags);
			hrtimer_cancel(&kbdev->pm.backend.metrics.timer);
		}
	}

	return 1;
}
#endif

#ifdef CONFIG_DEVFREQ_THERMAL
static unsigned long hisi_model_static_power(unsigned long voltage)
{
	int temperature;
	const unsigned long voltage_cubed = (voltage * voltage * voltage) >> 10;
	unsigned long temp, temp_squared, temp_cubed;
	unsigned long temp_scaling_factor = 0;

	struct device_node *dev_node = NULL;
	int ret = -EINVAL, i;
	const char *temperature_scale_capacitance[5];
	int capacitance[5] = {0};

	dev_node = of_find_node_by_name(NULL, "capacitances");
	if (dev_node) {
		for (i = 0; i < 5; i++) {
			ret = of_property_read_string_index(dev_node, "hisilicon,gpu_temp_scale_capacitance", i, &temperature_scale_capacitance[i]);
			if (ret) {
				pr_err("%s temperature_scale_capacitance [%d] read err\n",__func__,i);
				continue;
			}

			ret = kstrtoint(temperature_scale_capacitance[i], 10, &capacitance[i]);
			if (ret)
				continue;
		}
	}

	temperature = get_soc_temp();
	temp =(unsigned long)((long)temperature) / 1000;
	temp_squared = temp * temp;
	temp_cubed = temp_squared * temp;
	temp_scaling_factor = capacitance[3] * temp_cubed +
				capacitance[2] * temp_squared +
				capacitance[1] * temp +
				capacitance[0];

	return (((capacitance[4] * voltage_cubed) >> 20) * temp_scaling_factor) / 1000000;/* [false alarm]: no problem - fortify check */
}

#ifdef CONFIG_HISI_THERMAL_SPM
unsigned long hisi_calc_gpu_static_power(unsigned long voltage, int temperature)
{
	const long voltage_cubed = (voltage * voltage * voltage) >> 10;
	long temp, temp_squared, temp_cubed;
	long temp_scaling_factor;

	struct device_node *dev_node = NULL;
	int ret = -EINVAL, i;
	const char *temperature_scale_capacitance[5];
	int capacitance[5] = {0};

	dev_node = of_find_node_by_name(NULL, "capacitances");
	if (dev_node) {
		for (i = 0; i < 5; i++) {
			ret = of_property_read_string_index(dev_node, "hisilicon,gpu_temp_scale_capacitance", i, &temperature_scale_capacitance[i]);
			if (ret) {
				pr_err("%s temperature_scale_capacitance [%d] read err\n",__func__,i);
				continue;
			}

			ret = kstrtoint(temperature_scale_capacitance[i], 10, &capacitance[i]);
			if (ret)
				continue;
		}
	}

	temp = (long)temperature / 1000;
	temp_squared = temp * temp;
	temp_cubed = temp_squared * temp;
	temp_scaling_factor = capacitance[3] * temp_cubed +
				capacitance[2] * temp_squared +
				capacitance[1] * temp +
				capacitance[0];

	return (unsigned long)(((((long)capacitance[4] * voltage_cubed) / (1024 * 1024)) * temp_scaling_factor) / 1000000);/* [false alarm]: no problem - fortify check */
}
EXPORT_SYMBOL(hisi_calc_gpu_static_power);
#endif

static unsigned long hisi_model_dynamic_power(unsigned long freq,
		unsigned long voltage)
{
	/* The inputs: freq (f) is in Hz, and voltage (v) in mV.
	 * The coefficient (c) is in mW/(MHz mV mV).
	 *
	 * This function calculates the dynamic power after this formula:
	 * Pdyn (mW) = c (mW/(MHz*mV*mV)) * v (mV) * v (mV) * f (MHz)
	 */
	const unsigned long v2 = (voltage * voltage) / 1000; /* m*(V*V) */
	const unsigned long f_mhz = freq / 1000000; /* MHz */
	unsigned long coefficient = 3600; /* mW/(MHz*mV*mV) */
    struct device_node * dev_node = NULL;
    u32 prop = 0;

	dev_node = of_find_node_by_name(NULL, "capacitances");
    if(dev_node)
    {
        int ret = of_property_read_u32(dev_node,"hisilicon,gpu_dyn_capacitance",&prop);
        if(ret == 0)
        {
            coefficient = prop;
        }
    }

	return (coefficient * v2 * f_mhz) / 1000000; /* mW */
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
static struct devfreq_cooling_power hisi_model_ops = {
#else
static struct devfreq_cooling_ops hisi_model_ops = {
#endif
	.get_static_power = hisi_model_static_power,
	.get_dynamic_power = hisi_model_dynamic_power,
};
#endif

/* This function is used when memory_group_manager is not supported. */
static struct page *native_alloc_pages(struct memory_group_manager_device *mgm_dev __maybe_unused,
                                       u8 __maybe_unused id, gfp_t gfp_mask, unsigned int order)
{
	KBASE_DEBUG_ASSERT(mgm_dev != NULL);
	/* we ignore mgm_dev and policy id. */
	return alloc_pages(gfp_mask, order);
}

/* This function is used when memory_group_manager is not supported. */
static void native_free_pages(struct memory_group_manager_device *mgm_dev __maybe_unused,
                              u8 id __maybe_unused, struct page *page, unsigned int order)
{
	KBASE_DEBUG_ASSERT(page != NULL);
	__free_pages(page, order);
}

struct memory_group_manager_ops kbase_native_mgm_ops = {
	.mgm_alloc_pages = &native_alloc_pages,
	.mgm_free_pages = &native_free_pages,
};

static int kbasep_hisi_mgm_init(struct kbase_device *kbdev)
{
#ifdef CONFIG_OF
	struct device_node *mgm_node;
	struct platform_device *pdev;
	struct memory_group_manager_device *mgm_dev;

	mgm_node = of_parse_phandle(kbdev->dev->of_node,
                                "physical_memory_group_manager", 0);

	if (!mgm_node) {
		/* If memory_group_manager cannot be looked up then we assume
		 * memory_group_manager is not supported on this platform. */
		kbdev->hisi_dev_data.mgm_dev = kzalloc(sizeof(*kbdev->hisi_dev_data.mgm_dev),
                                               GFP_KERNEL);
		if (!kbdev->hisi_dev_data.mgm_dev)
			return -ENOMEM;

		kbdev->hisi_dev_data.mgm_ops = &kbase_native_mgm_ops;

		dev_info(kbdev->dev, "physical memory group manager is not available, use native instead.\n");
		return 0;
	}

	pdev = of_find_device_by_node(mgm_node);
	if (!pdev)
		return -EINVAL;

	mgm_dev = platform_get_drvdata(pdev);
	if (!mgm_dev)
		return -EPROBE_DEFER;

	kbdev->hisi_dev_data.mgm_dev = mgm_dev;
	kbdev->hisi_dev_data.mgm_ops = &mgm_dev->ops;

	dev_info(kbdev->dev, "physical memory group manager init succ.\n");
#endif
	return 0;
}

static void kbasep_hisi_mgm_term(struct kbase_device *kbdev)
{
	if (kbdev->hisi_dev_data.mgm_dev)
		kfree(kbdev->hisi_dev_data.mgm_dev);
}

static int kbase_platform_backend_init(struct kbase_device *kbdev)
{
	int err = 0;
	kbdev->hisi_dev_data.callbacks = (struct kbase_hisi_callbacks *)gpu_get_callbacks();

	/* Init the hisilicon memory group manager. */
	err = kbasep_hisi_mgm_init(kbdev);
	if (err) {
		dev_err(kbdev->dev, "[mali] Init memory group manager failed.\n");
		return err;
	}

#ifdef CONFIG_HISI_LAST_BUFFER
	cache_policy_callbacks *lb_cache_cbs = kbase_hisi_get_lb_cache_cbs(kbdev);
	KBASE_DEBUG_ASSERT(lb_cache_cbs != NULL);
	/* This operation will parse the device tree and create policy info. */
	err = lb_cache_cbs->lb_create_policy_info(kbdev);
	if (err) {
		dev_err(kbdev->dev,"[mali] Create last buffer policy info failed.\n");
		return err;
	}

	lb_quota_manager *lb_quota_cbs = kbase_hisi_get_lb_quota_cbs(kbdev);
	KBASE_DEBUG_ASSERT(lb_quota_cbs != NULL);
	lb_quota_cbs->quota_timer_init(&lb_quota_cbs->quota_timer);
#endif

	return 0;
}

static void kbase_platform_backend_term(struct kbase_device *kbdev)
{
	/* term the hisilicon memory group manager. */
	kbasep_hisi_mgm_term(kbdev);

#ifdef CONFIG_HISI_LAST_BUFFER
	cache_policy_callbacks *lb_cache_cbs = kbase_hisi_get_lb_cache_cbs(kbdev);
	KBASE_DEBUG_ASSERT(lb_cache_cbs != NULL);
	lb_cache_cbs->lb_destroy_policy_info(kbdev);

	lb_quota_manager *lb_quota_cbs = kbase_hisi_get_lb_quota_cbs(kbdev);
	KBASE_DEBUG_ASSERT(lb_quota_cbs != NULL);
	lb_quota_cbs->quota_timer_cancel(&lb_quota_cbs->quota_timer);
#endif

	kbdev->hisi_dev_data.callbacks = NULL;
}

#ifdef CONFIG_HISI_LAST_BUFFER
static void kbase_platform_request_quota(struct kbase_device *kbdev)
{
	lb_quota_manager *lb_quota_cbs = kbase_hisi_get_lb_quota_cbs(kbdev);
	KBASE_DEBUG_ASSERT(lb_quota_cbs);

	lb_quota_cbs->quota_timer_cancel(&lb_quota_cbs->quota_timer);

	/* Request last buffer quota again if quota released. */
	if (lb_quota_cbs->quota_released) {
		lb_quota_cbs->requestQuota(kbdev);
		lb_quota_cbs->quota_released = false;
	}
}

static void kbase_platform_release_quota(struct kbase_device *kbdev)
{
	lb_quota_manager *lb_quota_cbs = kbase_hisi_get_lb_quota_cbs(kbdev);
	KBASE_DEBUG_ASSERT(NULL != lb_quota_cbs);

	lb_quota_cbs->quota_timer_start(&lb_quota_cbs->quota_timer);
}
#endif

static int kbase_platform_init(struct kbase_device *kbdev)
{
#ifdef CONFIG_DEVFREQ_THERMAL
	int err;
#endif
#ifdef CONFIG_HISI_HW_VOTE_GPU_FREQ
	u32 freq = 0;
	unsigned long freq_hz;
	struct dev_pm_opp *opp;
#endif
	struct device *dev = kbdev->dev;
	dev->platform_data = kbdev;

#ifdef CONFIG_REPORT_VSYNC
	kbase_dev = kbdev;
#endif

	/* Init the hisilicon platform related data first. */
	if (kbase_platform_backend_init(kbdev)) {
		pr_err("[mali] platform backend init failed.\n");
		return 0;
	}

	kbdev->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(kbdev->clk)) {
		pr_err("[mali]  Failed to get clk\n");
		return 0;
	}


	kbdev->regulator = devm_regulator_get(dev, "gpu");
	if (IS_ERR(kbdev->regulator)) {
		pr_err("[mali]  Failed to get regulator\n");
		return 0;
	}

	kbdev->regulator_refcount = 0;

	if (fhss_init(dev))
		pr_err("[mali] Failed to init fhss\n");

#ifdef CONFIG_PM_DEVFREQ
#ifdef CONFIG_HISI_HW_VOTE_GPU_FREQ
	gpu_hvdev = hisi_hvdev_register(dev, "gpu-freq", "vote-src-1");
	if (!gpu_hvdev) {
		pr_err("[%s] register hvdev fail!\n", __func__);
	}
#endif

	if (/*dev_pm_opp_of_add_table(dev) ||*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
		hisi_devfreq_init_freq_table(dev,
			&mali_kbase_devfreq_profile.freq_table)){
#else
		opp_init_devfreq_table(dev,
			&mali_kbase_devfreq_profile.freq_table)) {
#endif
		pr_err("[mali]  Failed to init devfreq_table\n");
		kbdev->devfreq = NULL;
	} else {
#ifdef CONFIG_HISI_HW_VOTE_GPU_FREQ
		hisi_hv_get_last(gpu_hvdev, &freq);
		freq_hz = (unsigned long)freq * KHz;
		rcu_read_lock();
		opp = dev_pm_opp_find_freq_ceil(dev, &freq_hz);
		if (IS_ERR(opp)) {
			freq_hz = mali_kbase_devfreq_profile.freq_table[0];
		}
		rcu_read_unlock();
		hisi_hv_set_freq(gpu_hvdev, freq_hz/KHz);/*update last freq in hv driver*/
		mali_kbase_devfreq_profile.initial_freq = freq_hz;
#else
		mali_kbase_devfreq_profile.initial_freq = clk_get_rate(kbdev->clk);
#endif
		rcu_read_lock();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
		mali_kbase_devfreq_profile.max_state = dev_pm_opp_get_opp_count(dev);
#else
		mali_kbase_devfreq_profile.max_state = opp_get_opp_count(dev);
#endif
		rcu_read_unlock();
		dev_set_name(dev, "gpufreq");
		kbdev->devfreq = devfreq_add_device(dev,
						&mali_kbase_devfreq_profile,
						GPU_DEFAULT_GOVERNOR,
						NULL);
	}

	if (NULL == kbdev->devfreq) {
		pr_err("[mali]  NULL pointer [kbdev->devFreq]\n");
		goto JUMP_DEVFREQ_THERMAL;
	}
/*lint -e565*/
#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wunused-variable"
#ifdef CONFIG_DEVFREQ_THERMAL
	{
		struct devfreq_cooling_ops *callbacks;

		callbacks = (struct devfreq_cooling_ops *)POWER_MODEL_CALLBACKS;

		kbdev->devfreq_cooling = of_devfreq_cooling_register_power(
				kbdev->dev->of_node,
				kbdev->devfreq,
				callbacks);
		if (IS_ERR_OR_NULL(kbdev->devfreq_cooling)) {
			err = PTR_ERR(kbdev->devfreq_cooling);
			dev_err(kbdev->dev,
				"Failed to register cooling device (%d)\n",
				err);
			goto JUMP_DEVFREQ_THERMAL;
		}
	}
#endif
#pragma GCC diagnostic pop
/*lint +e565*/
	/* make devfreq function */
	//mali_kbase_devfreq_profile.polling_ms = DEFAULT_POLLING_MS;
#endif/*CONFIG_PM_DEVFREQ*/
JUMP_DEVFREQ_THERMAL:
	return 1;
}

static void kbase_platform_term(struct kbase_device *kbdev)
{
#ifdef CONFIG_PM_DEVFREQ
	devfreq_remove_device(kbdev->devfreq);
#endif

	/* term the hisilicon platform related data at last. */
	kbase_platform_backend_term(kbdev);
}

kbase_platform_funcs_conf platform_funcs = {
	.platform_init_func = &kbase_platform_init,
	.platform_term_func = &kbase_platform_term,
};

static int pm_callback_power_on(struct kbase_device *kbdev)
{
#ifdef CONFIG_HISI_LAST_BUFFER
	kbase_platform_request_quota(kbdev);
#endif

#ifdef CONFIG_MALI_MIDGARD_RT_PM
	int result;
	int ret_val;
	struct device *dev = kbdev->dev;

	/* es */
	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_HI0014)) {
		unsigned int value = 0;
		/* read PERI_CTRL21 and set it's [0~16]bit to 0, disable mem shutdown by software */
		value = readl(kbdev->pctrlreg + PERI_CTRL21) & MASK_DISABLESDBYSF;
		writel(value, kbdev->pctrlreg + PERI_CTRL21);
		/* read PERI_CTRL93 and set it's [1~17]bit to 0, disable deep sleep by software */
		value = readl(kbdev->pctrlreg + PERI_CTRL93) & MASK_DISABLEDSBYSF;
		writel(value, kbdev->pctrlreg + PERI_CTRL93);
	}
	/* cs */
	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_HI0015)) {
		unsigned int value = 0;
		/* read PERI_CTRL93 and set it's [1~17]bit to 0, disable deep sleep by software */
		value = readl(kbdev->pctrlreg + PERI_CTRL93) & MASK_DISABLEDSBYSF;
		writel(value, kbdev->pctrlreg + PERI_CTRL93);
	}

#if (HARD_RESET_AT_POWER_OFF != 1)
	if (!pm_runtime_status_suspended(dev))
		ret_val = 0;
	else
#endif
		ret_val = 1;

	if (unlikely(dev->power.disable_depth > 0)) {
		kbase_platform_on(kbdev);
	} else {
		result = pm_runtime_resume(dev);
		if (result < 0 && result == -EAGAIN)
			kbase_platform_on(kbdev);
		else if (result < 0)
			pr_err("[mali]  pm_runtime_resume failed (%d)\n", result);
	}

	return ret_val;
#else
	kbase_platform_on(kbdev);

	return 1;
#endif
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
#ifdef CONFIG_HISI_LAST_BUFFER
	kbase_platform_release_quota(kbdev);
#endif

#ifdef CONFIG_MALI_MIDGARD_RT_PM
	struct device *dev = kbdev->dev;
	int ret = 0, retry = 0;

	/* es */
	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_HI0014)) {
		unsigned int value = 0;
		/* read PERI_CTRL21 and set it's [0~16]bit to 1, enable mem shutdown by software */
		value = readl(kbdev->pctrlreg + PERI_CTRL21) | MASK_ENABLESDBYSF;
		writel(value, kbdev->pctrlreg + PERI_CTRL21);
		/* read PERI_CTRL93 and set it's [1~17]bit to 1, enable deep sleep by software */
		value = readl(kbdev->pctrlreg + PERI_CTRL93) | MASK_ENABLEDSBYSF;
		writel(value, kbdev->pctrlreg + PERI_CTRL93);
	}
	/* cs */
	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_HI0015)) {
		unsigned int value = 0;
		/* read PERI_CTRL93 and set it's [1~17]bit to 1, enable deep sleep by software */
		value = readl(kbdev->pctrlreg + PERI_CTRL93) | MASK_ENABLEDSBYSF;
		writel(value, kbdev->pctrlreg + PERI_CTRL93);
	}
	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_HI0008)) {
		/* when GPU in idle state, auto decrease the clock rate.
		 */
		unsigned int tiler_lo = kbdev->tiler_available_bitmap & 0xFFFFFFFF;
		unsigned int tiler_hi = (kbdev->tiler_available_bitmap >> 32) & 0xFFFFFFFF;
		unsigned int l2_lo = kbdev->l2_available_bitmap & 0xFFFFFFFF;
		unsigned int l2_hi = (kbdev->l2_available_bitmap >> 32) & 0xFFFFFFFF;

		kbase_reg_write(kbdev, GPU_CONTROL_REG(TILER_PWROFF_LO), tiler_lo);
		kbase_reg_write(kbdev, GPU_CONTROL_REG(TILER_PWROFF_HI), tiler_hi);
		kbase_reg_write(kbdev, GPU_CONTROL_REG(L2_PWROFF_LO), l2_lo);
		kbase_reg_write(kbdev, GPU_CONTROL_REG(L2_PWROFF_HI), l2_hi);
	}

#if HARD_RESET_AT_POWER_OFF
	/* Cause a GPU hard reset to test whether we have actually idled the GPU
	 * and that we properly reconfigure the GPU on power up.
	 * Usually this would be dangerous, but if the GPU is working correctly it should
	 * be completely safe as the GPU should not be active at this point.
	 * However this is disabled normally because it will most likely interfere with
	 * bus logging etc.
	 */
	KBASE_TRACE_ADD(kbdev, CORE_GPU_HARD_RESET, NULL, NULL, 0u, 0);
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND), GPU_COMMAND_HARD_RESET);
#endif

	if (unlikely(dev->power.disable_depth > 0)) {
		kbase_platform_off(kbdev);
	} else {
		do {
			if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_HI0007))
				ret = pm_schedule_suspend(dev, RUNTIME_PM_DELAY_1MS);
			else
				ret = pm_schedule_suspend(dev, RUNTIME_PM_DELAY_30MS);
			if (ret != -EAGAIN) {
				if (unlikely(ret < 0)) {
					pr_err("[mali]  pm_schedule_suspend failed (%d)\n\n", ret);
					WARN_ON(1);
				}

				/* correct status */
				break;
			}

			/* -EAGAIN, repeated attempts for 1s totally */
			msleep(50);
		} while (++retry < 20);
	}
#else
	kbase_platform_off(kbdev);
#endif
}

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wunused-function"
static int pm_callback_runtime_init(struct kbase_device *kbdev)
{
	pm_suspend_ignore_children(kbdev->dev, true);
	pm_runtime_enable(kbdev->dev);
	return 0;
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wunused-function"
static void pm_callback_runtime_term(struct kbase_device *kbdev)
{
	pm_runtime_disable(kbdev->dev);
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void pm_callback_runtime_off(struct kbase_device *kbdev)
{
#ifdef CONFIG_PM_DEVFREQ
	devfreq_suspend_device(kbdev->devfreq);
#elif defined(CONFIG_MALI_MIDGARD_DVFS)
	kbase_platform_dvfs_enable(kbdev, false, 0);
#endif

	kbase_platform_off(kbdev);
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static int pm_callback_runtime_on(struct kbase_device *kbdev)
{
	/*es */
	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_HI0014)) {
		unsigned int value = 0;
		/* read PERI_CTRL21 and set it's [0~16]bit to 0, disable mem shutdown by software */
		value = readl(kbdev->pctrlreg + PERI_CTRL21) & MASK_DISABLESDBYSF;
		writel(value, kbdev->pctrlreg + PERI_CTRL21);
		/* read PERI_CTRL93 and set it's [1~17]bit to 0, disable deep sleep by software */
		value = readl(kbdev->pctrlreg + PERI_CTRL93) & MASK_DISABLEDSBYSF;
		writel(value, kbdev->pctrlreg + PERI_CTRL93);
	}
	/* cs */
	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_HI0015)) {
		unsigned int value = 0;
		/* read PERI_CTRL93 and set it's [1~17]bit to 0, disable deep sleep by software */
		value = readl(kbdev->pctrlreg + PERI_CTRL93) & MASK_DISABLEDSBYSF;
		writel(value, kbdev->pctrlreg + PERI_CTRL93);
	}
	kbase_platform_on(kbdev);

#ifdef CONFIG_PM_DEVFREQ
	devfreq_resume_device(kbdev->devfreq);
#elif defined(CONFIG_MALI_MIDGARD_DVFS)
	if (kbase_platform_dvfs_enable(kbdev, true, 0) != MALI_TRUE)
		return -EPERM;
#endif

	return 0;
}
#pragma GCC diagnostic pop

static inline void pm_callback_suspend(struct kbase_device *kbdev)
{
#ifdef CONFIG_MALI_MIDGARD_RT_PM
	if (!pm_runtime_status_suspended(kbdev->dev))
		pm_callback_runtime_off(kbdev);
	else
		pr_err("%s pm_runtime_status_suspended!\n", __func__);
#else
	pm_callback_power_off(kbdev);
#endif
}

static inline void pm_callback_resume(struct kbase_device *kbdev)
{
#ifdef CONFIG_MALI_MIDGARD_RT_PM
	if (!pm_runtime_status_suspended(kbdev->dev))
		pm_callback_runtime_on(kbdev);
	else
		pm_callback_power_on(kbdev);
#else
	pm_callback_power_on(kbdev);
#endif
}

static inline int pm_callback_runtime_idle(struct kbase_device *kbdev)
{
	return 1;
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback = pm_callback_suspend,
	.power_resume_callback = pm_callback_resume,
#ifdef CONFIG_MALI_MIDGARD_RT_PM
	.power_runtime_init_callback = pm_callback_runtime_init,
	.power_runtime_term_callback = pm_callback_runtime_term,
	.power_runtime_off_callback = pm_callback_runtime_off,
	.power_runtime_on_callback = pm_callback_runtime_on,
	.power_runtime_idle_callback = pm_callback_runtime_idle
#else
	.power_runtime_init_callback = NULL,
	.power_runtime_term_callback = NULL,
	.power_runtime_off_callback = NULL,
	.power_runtime_on_callback = NULL,
	.power_runtime_idle_callback = NULL
#endif
};



static struct kbase_platform_config hi_platform_config = {
#ifndef CONFIG_OF
	.io_resources = &io_resources
#endif
};

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &hi_platform_config;
}

int kbase_platform_early_init(void)
{
	/* Nothing needed at this stage */
	return 0;
}
