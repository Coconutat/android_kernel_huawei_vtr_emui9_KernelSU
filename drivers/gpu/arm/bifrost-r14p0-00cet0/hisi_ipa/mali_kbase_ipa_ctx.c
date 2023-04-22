/*
 *
 * (C) COPYRIGHT 2015-2016 ARM Limited. All rights reserved.
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


#include <linux/of.h>
#include <linux/sysfs.h>

#include <mali_kbase.h>

#define NR_IPA_GROUPS					8
#define NR_IPA_UNIT						5
#define KBASE_PM_TIME_SHIFT				8
#define DEFAULT_BOUND_THRESHOLD			300
#define HWC_BOOTING_UTIL				45
#define HWC_IDLE_COUNT					10

struct kbase_ipa_context;

/**
 * struct ipa_group - represents a single IPA group
 * @name:               name of the IPA group
 * @capacitance:        capacitance constant for IPA group
 * @calc_power:         function to calculate power for IPA group
 */
struct ipa_group {
	const char *name;
	u32 capacitance;
	u32 (*calc_power)(struct kbase_ipa_context *,
			struct ipa_group *);
};

#include "hisi_ipa/mali_kbase_ipa_tables_ctx.h"

/**
 * struct kbase_ipa_context - IPA context per device
 * @kbdev:              pointer to kbase device
 * @groups:             array of IPA groups for this context
 * @vinstr_cli:         vinstr client handle
 * @vinstr_buffer:      buffer to dump hardware counters onto
 * @ipa_lock:           protects the entire IPA context
 */
struct kbase_ipa_context {
	struct kbase_device *kbdev;
	struct ipa_group groups[NR_IPA_GROUPS];
	struct kbase_vinstr_client *vinstr_cli;
	void *vinstr_buffer;
	struct mutex ipa_lock;
	u32 dyn_power[NR_IPA_UNIT];
	u32 time_diff[NR_IPA_UNIT];
	ktime_t timestamp;
	u32 bound_threshold;
	bool bound_detection_enable;
	u32 idle_beats;
};

static ssize_t show_ipa_group(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct kbase_device *kbdev = dev_get_drvdata(dev);
	struct kbase_ipa_context *ctx = kbdev->ipa_ctx;
	ssize_t count = -EINVAL;
	size_t i;

	mutex_lock(&ctx->ipa_lock);
	for (i = 0; i < ARRAY_SIZE(ctx->groups); i++) {
		if (!strcmp(ctx->groups[i].name, attr->attr.name)) {/*lint !e421 */
			count = snprintf(buf, PAGE_SIZE, "%lu\n",
				(unsigned long)ctx->groups[i].capacitance);
			break;
		}
	}
	mutex_unlock(&ctx->ipa_lock);
	return count;
}

static ssize_t set_ipa_group(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct kbase_device *kbdev = dev_get_drvdata(dev);
	struct kbase_ipa_context *ctx = kbdev->ipa_ctx;
	unsigned long capacitance;
	size_t i;
	int err;

	err = kstrtoul(buf, 0, &capacitance);
	if (err < 0)
		return err;
	if (capacitance > U32_MAX)
		return -ERANGE;

	mutex_lock(&ctx->ipa_lock);
	for (i = 0; i < ARRAY_SIZE(ctx->groups); i++) {
		if (!strcmp(ctx->groups[i].name, attr->attr.name)) {/*lint !e421 */
			ctx->groups[i].capacitance = capacitance;
			mutex_unlock(&ctx->ipa_lock);
			return count;
		}
	}
	mutex_unlock(&ctx->ipa_lock);
	return -EINVAL;
}

static ssize_t show_boundth(struct device *dev,struct device_attribute *attr,char *buf)
{
	struct kbase_device *kbdev = dev_get_drvdata(dev);
	struct kbase_ipa_context *ctx = kbdev->ipa_ctx;
	ssize_t count = -EINVAL;

	mutex_lock(&ctx->ipa_lock);
	count = snprintf(buf, PAGE_SIZE, "%lu\n",(unsigned long)ctx->bound_threshold);
	mutex_unlock(&ctx->ipa_lock);
	return count;
}

static ssize_t set_boundth(struct device *dev,struct device_attribute *attr,const char *buf,size_t count)
{
	struct kbase_device *kbdev = dev_get_drvdata(dev);
	struct kbase_ipa_context *ctx = kbdev->ipa_ctx;
	unsigned long threshold;
	int err;

	err = kstrtoul(buf, 0, &threshold);
	if (err < 0)
		return err;
	if (threshold > U32_MAX)
		return -ERANGE;

	mutex_lock(&ctx->ipa_lock);
	ctx->bound_threshold = threshold;
	mutex_unlock(&ctx->ipa_lock);
	return count;
}

static ssize_t show_bound_dect_enable(struct device *dev,struct device_attribute *attr,char *buf)
{
	struct kbase_device *kbdev = dev_get_drvdata(dev);
	struct kbase_ipa_context *ctx = kbdev->ipa_ctx;
	ssize_t count = -EINVAL;

	mutex_lock(&ctx->ipa_lock);

	if (ctx->bound_detection_enable)
		count = snprintf(buf, PAGE_SIZE, "enable\n");
	else
		count = snprintf(buf, PAGE_SIZE, "disable\n");

	mutex_unlock(&ctx->ipa_lock);

	return count;
}

static ssize_t set_bound_dect_enable(struct device *dev,struct device_attribute *attr,const char *buf,size_t count)
{
	struct kbase_device *kbdev = dev_get_drvdata(dev);
	struct kbase_ipa_context *ctx = kbdev->ipa_ctx;
	bool enable_mode;

	if (!strncmp("disable", buf,sizeof("disable") - 1))
		enable_mode = false;
	else if (!strncmp("enable", buf,sizeof("enable") - 1))
		enable_mode = true;
	else
		return -EINVAL;

	mutex_lock(&ctx->ipa_lock);
	ctx->bound_detection_enable = enable_mode;
	mutex_unlock(&ctx->ipa_lock);
	return count;
}

static DEVICE_ATTR(group0, S_IRUGO | S_IWUSR, show_ipa_group, set_ipa_group);
static DEVICE_ATTR(group1, S_IRUGO | S_IWUSR, show_ipa_group, set_ipa_group);
static DEVICE_ATTR(group2, S_IRUGO | S_IWUSR, show_ipa_group, set_ipa_group);
static DEVICE_ATTR(group3, S_IRUGO | S_IWUSR, show_ipa_group, set_ipa_group);
static DEVICE_ATTR(group4, S_IRUGO | S_IWUSR, show_ipa_group, set_ipa_group);
static DEVICE_ATTR(group5, S_IRUGO | S_IWUSR, show_ipa_group, set_ipa_group);
static DEVICE_ATTR(group6, S_IRUGO | S_IWUSR, show_ipa_group, set_ipa_group);
static DEVICE_ATTR(group7, S_IRUGO | S_IWUSR, show_ipa_group, set_ipa_group);
static DEVICE_ATTR(boundth, S_IRUGO | S_IWUSR, show_boundth, set_boundth);
static DEVICE_ATTR(bound_enable, S_IRUGO | S_IWUSR, show_bound_dect_enable, set_bound_dect_enable);

static struct attribute *kbase_ipa_attrs[] = {
	&dev_attr_group0.attr,
	&dev_attr_group1.attr,
	&dev_attr_group2.attr,
	&dev_attr_group3.attr,
	&dev_attr_group4.attr,
	&dev_attr_group5.attr,
	&dev_attr_group6.attr,
	&dev_attr_group7.attr,
	&dev_attr_boundth.attr,
	&dev_attr_bound_enable.attr,
	NULL,
};

static struct attribute_group kbase_ipa_attr_group = {
	.name = "ipa",
	.attrs = kbase_ipa_attrs,
};

static void init_ipa_groups(struct kbase_ipa_context *ctx)
{
	memcpy(ctx->groups, ipa_groups_def, sizeof(ctx->groups));
}

#if defined(CONFIG_OF) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0))
static int update_ipa_groups_from_dt(struct kbase_ipa_context *ctx)
{
	struct kbase_device *kbdev = ctx->kbdev;
	struct device_node *np, *child;
	struct ipa_group *group;
	size_t nr_groups;
	size_t i;
	int err;

	np = of_get_child_by_name(kbdev->dev->of_node, "ipa-groups");
	if (!np)
		return 0;

	nr_groups = 0;
	for_each_available_child_of_node(np, child)
		nr_groups++;
	if (!nr_groups || nr_groups > ARRAY_SIZE(ctx->groups)) {
		dev_err(kbdev->dev, "invalid number of IPA groups: %zu", nr_groups);
		err = -EINVAL;
		goto err0;
	}

	for_each_available_child_of_node(np, child) {
		const char *name;
		u32 capacitance;

		name = of_get_property(child, "label", NULL);
		if (!name) {
			dev_err(kbdev->dev, "label missing for IPA group");
			err = -EINVAL;
			goto err0;
		}
		err = of_property_read_u32(child, "capacitance",
				&capacitance);
		if (err < 0) {
			dev_err(kbdev->dev, "capacitance missing for IPA group");
			goto err0;
		}

		for (i = 0; i < ARRAY_SIZE(ctx->groups); i++) {
			group = &ctx->groups[i];
			if (!strcmp(group->name, name)) {/*lint !e421 */
				group->capacitance = capacitance;
				break;
			}
		}
	}

	of_node_put(np);
	return 0;
err0:
	of_node_put(np);
	return err;
}
#else
static int update_ipa_groups_from_dt(struct kbase_ipa_context *ctx)
{
	return 0;
}
#endif

static int reset_ipa_groups(struct kbase_ipa_context *ctx)
{
	init_ipa_groups(ctx);
	return update_ipa_groups_from_dt(ctx);
}

static inline u32 read_hwcnt(struct kbase_ipa_context *ctx,
	u32 offset)
{
	u8 *p = ctx->vinstr_buffer;

	return *(u32 *)&p[offset];
}

static inline u32 add_saturate(u32 a, u32 b)
{
	if (U32_MAX - a < b)
		return U32_MAX;
	return a + b;
}

/*
 * Calculate power estimation based on hardware counter `c'
 * across all shader cores.
 */
static u32 calc_power_sc_single(struct kbase_ipa_context *ctx,
	struct ipa_group *group, u32 c)
{
	struct kbase_device *kbdev = ctx->kbdev;
	u64 core_mask;
	u32 base = 0, r = 0;

	core_mask = kbdev->gpu_props.props.coherency_info.group[0].core_mask;
	while (core_mask != 0ull) {
		if ((core_mask & 1ull) != 0ull) {
			u64 n = read_hwcnt(ctx, base + c);
			u32 d = read_hwcnt(ctx, GPU_ACTIVE);
			u32 s = group->capacitance;

			if (d != 0)
			{
				r = add_saturate(r, div_u64(n * s, d));
			}
		}
		base += NR_CNT_PER_BLOCK * NR_BYTES_PER_CNT;
		core_mask >>= 1;
	}
	return r;
}

/*
 * Calculate power estimation based on hardware counter `c1'
 * and `c2' across all shader cores.
 */
static u32 calc_power_sc_double(struct kbase_ipa_context *ctx,
	struct ipa_group *group, u32 c1, u32 c2)
{
	struct kbase_device *kbdev = ctx->kbdev;
	u64 core_mask;
	u32 base = 0, r = 0;

	core_mask = kbdev->gpu_props.props.coherency_info.group[0].core_mask;
	while (core_mask != 0ull) {
		if ((core_mask & 1ull) != 0ull) {
			u64 n = read_hwcnt(ctx, base + c1);
			u32 d = read_hwcnt(ctx, GPU_ACTIVE);
			u32 s = group->capacitance;

			if (d != 0)
			{
				r = add_saturate(r, div_u64(n * s, d));
				n = read_hwcnt(ctx, base + c2);
				r = add_saturate(r, div_u64(n * s, d));
			}
		}
		base += NR_CNT_PER_BLOCK * NR_BYTES_PER_CNT;
		core_mask >>= 1;
	}
	return r;
}

static u32 calc_power_single(struct kbase_ipa_context *ctx,
	struct ipa_group *group, u32 c)
{
	u64 n = read_hwcnt(ctx, c);
	u32 d = read_hwcnt(ctx, GPU_ACTIVE);
	u32 s = group->capacitance;

	if (d != 0)
	{
		return div_u64(n * s, d);
	}
	else
	{
		return 0;
	}
}

static u32 calc_power_group0(struct kbase_ipa_context *ctx,
		struct ipa_group *group)
{
	return calc_power_single(ctx, group, L2_ANY_LOOKUP);
}

static u32 calc_power_group1(struct kbase_ipa_context *ctx,
		struct ipa_group *group)
{
	return calc_power_single(ctx, group, TILER_ACTIVE);
}

static u32 calc_power_group2(struct kbase_ipa_context *ctx,
		struct ipa_group *group)
{
	return calc_power_sc_single(ctx, group, FRAG_ACTIVE);
}

static u32 calc_power_group3(struct kbase_ipa_context *ctx,
		struct ipa_group *group)
{
	return calc_power_sc_double(ctx, group, VARY_SLOT_32,
			VARY_SLOT_16);
}

static u32 calc_power_group4(struct kbase_ipa_context *ctx,
		struct ipa_group *group)
{
	return calc_power_sc_single(ctx, group, TEX_COORD_ISSUE);
}

static u32 calc_power_group5(struct kbase_ipa_context *ctx,
		struct ipa_group *group)
{
	return calc_power_sc_single(ctx, group, EXEC_INSTR_COUNT);
}

static u32 calc_power_group6(struct kbase_ipa_context *ctx,
		struct ipa_group *group)
{
	return calc_power_sc_double(ctx, group, BEATS_RD_LSC,
			BEATS_WR_LSC);
}

static u32 calc_power_group7(struct kbase_ipa_context *ctx,
		struct ipa_group *group)
{
	return calc_power_sc_single(ctx, group, EXEC_CORE_ACTIVE);
}

static int attach_vinstr(struct kbase_ipa_context *ctx)
{
	struct kbase_device *kbdev = ctx->kbdev;
	struct kbase_ioctl_hwcnt_reader_setup setup;
	size_t dump_size;

	dump_size = kbase_vinstr_dump_size(kbdev);
	ctx->vinstr_buffer = kzalloc(dump_size, GFP_KERNEL);
	if (!ctx->vinstr_buffer) {
		dev_err(kbdev->dev, "Failed to allocate IPA dump buffer");
		return -1;
	}

	setup.jm_bm = ~0u;
	setup.shader_bm = ~0u;
	setup.tiler_bm = ~0u;
	setup.mmu_l2_bm = ~0u;
	ctx->vinstr_cli = kbase_vinstr_hwcnt_kernel_setup(kbdev->vinstr_ctx,
			&setup, ctx->vinstr_buffer);
	if (!ctx->vinstr_cli) {
		dev_err(kbdev->dev, "Failed to register IPA with vinstr core");
		kfree(ctx->vinstr_buffer);
		ctx->vinstr_buffer = NULL;
		return -1;
	}
	return 0;
}

static void detach_vinstr(struct kbase_ipa_context *ctx)
{
	if (ctx->vinstr_cli)
		kbase_vinstr_detach_client(ctx->vinstr_cli);
	ctx->vinstr_cli = NULL;
	kfree(ctx->vinstr_buffer);
	ctx->vinstr_buffer = NULL;
}

struct kbase_ipa_context *kbase_dynipa_init(struct kbase_device *kbdev)
{
	struct kbase_ipa_context *ctx;
	int err;

	int i;
	const void * bound_threshold_dts;
	const char * bound_detection_enable_mode;
	u32 bound_threshold;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return NULL;

	mutex_init(&ctx->ipa_lock);
	ctx->kbdev = kbdev;

	err = reset_ipa_groups(ctx);
	if (err < 0)
		goto err0;

	err = sysfs_create_group(&kbdev->dev->kobj, &kbase_ipa_attr_group);
	if (err < 0)
		goto err0;

	ctx->timestamp = ktime_get();
	for (i = 0; i < NR_IPA_UNIT; i++) {
		ctx->dyn_power[i] = 0;
		ctx->time_diff[i] = 0;
	}

       ctx->bound_threshold = DEFAULT_BOUND_THRESHOLD;
       ctx->bound_detection_enable =  true;
       ctx->idle_beats = 0;

#ifdef CONFIG_OF
	bound_threshold_dts = of_get_property(kbdev->dev->of_node,"gpu-bound-check-threshold",NULL);
	if (bound_threshold_dts) {
		bound_threshold = be32_to_cpup(bound_threshold_dts);
		ctx->bound_threshold = bound_threshold;

		dev_info(kbdev->dev,
			"Using gpu bound-check-threshold %u set from dtb",bound_threshold);
	}

	bound_detection_enable_mode = of_get_property(kbdev->dev->of_node,"gpu-bound-detection-enable", NULL);
	if (bound_detection_enable_mode) {
		if (!strcmp("disable", bound_detection_enable_mode))/*lint !e421 */
			ctx->bound_detection_enable = false;
       }
#endif

	return ctx;
err0:
	kfree(ctx);
	return NULL;
}
KBASE_EXPORT_TEST_API(kbase_dynipa_init);

void kbase_dynipa_term(struct kbase_ipa_context *ctx)
{
	struct kbase_device *kbdev = ctx->kbdev;

	detach_vinstr(ctx);
	sysfs_remove_group(&kbdev->dev->kobj, &kbase_ipa_attr_group);
	kfree(ctx);
}
KBASE_EXPORT_TEST_API(kbase_dynipa_term);

u32 kbase_ipa_dynamic_core_power(struct kbase_ipa_context *ctx, int *err)
{
	struct ipa_group *group;
	u32 power = 0;
	size_t i;
	mutex_lock(&ctx->ipa_lock);
	if (!ctx->vinstr_cli) {
		*err = attach_vinstr(ctx);
		if (*err < 0)
			goto err0;
	}
	*err = kbase_vinstr_hwc_dump(ctx->vinstr_cli,
			BASE_HWCNT_READER_EVENT_MANUAL);
	if (*err)
		goto err0;
	for (i = 0; i < ARRAY_SIZE(ctx->groups); i++) {
		group = &ctx->groups[i];
		power = add_saturate(power, group->calc_power(ctx, group));
	}
err0:
	mutex_unlock(&ctx->ipa_lock);
	return power;
}
KBASE_EXPORT_TEST_API(kbase_ipa_dynamic_core_power);

void kbase_ipa_hwcounts(struct kbase_ipa_context *ctx, int *err)
{
	u32 loading = 0;
	static unsigned int statistics_idx = 0;
	ktime_t time_now;
	ktime_t timestamp_diff;

	/*Dynamic power is defined in mw @ 1Ghz, Dynamic power will be high
	if GPU is extremely busy. But we found it is not quite proportional to
	GPU frequency. So it will be normalied it by GPU frequency. In different
	freqency range, we use different coefficient.
	      We statistic latest 5 dynamic power samples. GPU bound event is signalled+	when average dynamic power value is high.
	*/
	loading = kbase_ipa_dynamic_core_power(ctx, err);
	if (*err)
		goto err0;

	time_now = ktime_get();
	timestamp_diff = ktime_sub(time_now, ctx->timestamp);
	ctx->timestamp = time_now;
	ctx->time_diff[statistics_idx % NR_IPA_UNIT] = (u32) (ktime_to_ns(timestamp_diff) >> KBASE_PM_TIME_SHIFT);
	ctx->dyn_power[statistics_idx % NR_IPA_UNIT] = loading;

	statistics_idx++;

err0:
	return ;
}

u32 kbase_ipa_dynamic_bound_measure(struct kbase_ipa_context *ctx, int *err)
{
	size_t i;
	u32 total_power = 0;
	u32 total_time = 0;

	(void)kbase_ipa_hwcounts(ctx, err);
	for (i = 0; i < NR_IPA_UNIT; i++) {
		total_power += ctx->dyn_power[i];
		total_time += ctx->time_diff[i];
	}
	total_power = total_power * 1000;
	total_time = total_time / 4000;

	if (total_time != 0)
		return total_power / total_time;

	return 0;
}

bool kbase_ipa_dynamic_bound_detect(struct kbase_ipa_context *ctx, int *err, unsigned long cur_freq,unsigned long load, bool ipa_enable)
{
	u32 bound_measure = 0;
	unsigned long mhz = cur_freq / 1000000;

	/*GPU won't bound, if GPU loading less than 45%.*/
	if (load < HWC_BOOTING_UTIL) {
	ctx->idle_beats++;
	/*If GPU keep be in low ultilization rate for a while, disable the
	hwcount dump, so that the GPU can be idle.
	*/
	if (ctx->idle_beats > HWC_IDLE_COUNT && ctx->vinstr_cli)
		detach_vinstr(ctx);

		return false;
	}
	ctx->idle_beats = 0;
	if (ipa_enable)
	{
		bound_measure = kbase_ipa_dynamic_bound_measure(ctx, err);
	}

	if (*err)
		goto err0;

	if (ctx->bound_detection_enable && (mhz != 0))
		return ((bound_measure / mhz) > ctx->bound_threshold);

err0:
	return false;
}

