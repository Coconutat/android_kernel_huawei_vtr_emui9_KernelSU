#ifndef _HIFREQ_HOTPLUG_H
#define _HIFREQ_HOTPLUG_H

void bL_hifreq_hotplug_init(void);
bool hifreq_hotplug_is_enabled(void);
int bL_hifreq_hotplug_set_target(struct cpufreq_policy *policy, struct device *dev,
		unsigned int freq);
void set_bL_hifreq_load(unsigned int max_load);
#endif
