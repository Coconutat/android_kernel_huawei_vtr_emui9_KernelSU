#ifndef _GOVERNOR_PM_QOS_H
#define _GOVERNOR_PM_QOS_H

/**
 * struct devfreq_pm_qos_data - void *data fed to struct devfreq
 *	and devfreq_add_device
 * @ bytes_per_sec_per_hz	Ratio to convert throughput request to devfreq
 *				frequency.
 * @ pm_qos_class		pm_qos class to query for requested throughput
 */
struct devfreq_pm_qos_data {
	unsigned int bytes_per_sec_per_hz;
	unsigned int bd_utilization;
	int pm_qos_class;
	unsigned long freq;
};

#endif /* _GOVERNOR_PM_QOS_H */

