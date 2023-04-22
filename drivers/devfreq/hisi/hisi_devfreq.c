#include <linux/device.h>
#include <linux/module.h>
#include <linux/hisi/hisi_devfreq.h>

#include <linux/pm_opp.h>
#include <linux/slab.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
int hisi_devfreq_init_freq_table(struct device *dev,
				unsigned int **table)
#else
int hisi_devfreq_init_freq_table(struct device *dev,
				unsigned long **table)
#endif
{
	struct dev_pm_opp *opp;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	unsigned int *freq_table;
#else
	unsigned long *freq_table;
#endif
	unsigned long freq = 0;
	unsigned int i = 0;
	int opp_count = 0;

	if (!table)
		return -ENOMEM;

	opp_count = dev_pm_opp_get_opp_count(dev);
	if (opp_count <= 0)
		return -ENOMEM;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	freq_table = kzalloc(sizeof(unsigned int) * opp_count, GFP_KERNEL);
#else
	freq_table = kzalloc(sizeof(unsigned long) * opp_count, GFP_KERNEL);
#endif
	if (!freq_table) {
		dev_warn(dev, "%s: Unable to allocate frequency table\n", __func__);
		return -ENOMEM;
	}

	rcu_read_lock();
	do{
		opp = dev_pm_opp_find_freq_ceil(dev, &freq);
		if (IS_ERR(opp))
			break;

		freq_table[i++] = freq;
		freq++;
	}while(1);
	rcu_read_unlock();

	*table = freq_table;

	return 0;
}
EXPORT_SYMBOL(hisi_devfreq_init_freq_table);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
int hisi_devfreq_free_freq_table(struct device *dev,
				unsigned int **table)
#else
int hisi_devfreq_free_freq_table(struct device *dev,
				unsigned long **table)
#endif
{
	if (table == NULL || (*table) == NULL)
		return -ENOMEM;

	kfree(*table);
	*table = NULL;

	return 0;
}
EXPORT_SYMBOL(hisi_devfreq_free_freq_table);
