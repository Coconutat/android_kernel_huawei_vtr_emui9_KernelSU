#include <linux/module.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/rtc.h>
#include "hisi_peripheral_tm.h"
#include <linux/power/hisi/coul/hisi_coul_drv.h>

#define	DEFAULT_AMBIENT_TEMP	(0)
#define AMBIENT_MAX_TIME_SEC	(15 * 60)
#define HOURTOSEC		(3600)
#define LOW_CURRENT		(50)
#define KEEP_CURRENT		(100)

struct hisi_ambient_sensor_t {
	u32 id;
	int temp;
};

struct hisi_ambient_t {
	int sensor_count;
	u32 interval;
	int bias;
	int temp;
	int start_cc;
	u32 id;
	struct timeval last_time;
	struct timeval now;
	struct thermal_zone_device	*tz_dev;
	struct hisi_ambient_sensor_t hisi_ambient_sensor[0];
};

int ipa_get_periph_id(const char *name);
int ipa_get_periph_value(u32 sensor, int *val);
extern int coul_get_battery_cc (void);


static int hisi_get_ambient_temp(struct thermal_zone_device *thermal,
				      int *temp)
{
	struct hisi_ambient_t *hisi_ambient = thermal->devdata;

	if (!hisi_ambient || !temp)
		return -EINVAL;

	*temp = hisi_ambient->temp;

	return 0;
}

/*lint -e785*/
static struct thermal_zone_device_ops ambient_thermal_zone_ops = {
	.get_temp = hisi_get_ambient_temp,
};
/*lint +e785*/

static int hisi_ambient_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *dev_node = dev->of_node;
	int ret;
	int sensor_count;
	int i = 0;
	struct device_node *np;
	struct device_node *child;
	const char *ptr_type;
	struct hisi_ambient_sensor_t *ambient_sensor;
	struct hisi_ambient_t *hisi_ambient;

	if (!of_device_is_available(dev_node)) {
		dev_err(dev, "HISI ambient dev not found\n");
		return -ENODEV;
	}

	np = of_find_node_by_name(dev_node, "sensors");
	if (!np) {
		pr_err("sensors node not found\n");
		ret = -ENODEV;
		goto exit;
	}

	sensor_count = of_get_child_count(np);
	if (sensor_count <= 0) {
		ret = -EINVAL;
		pr_err("%s sensor count read err\n", __func__);
		goto node_put;
	}

	hisi_ambient = kzalloc(sizeof(struct hisi_ambient_t) + (u64)((long)sensor_count) * (sizeof(struct hisi_ambient_sensor_t)), GFP_KERNEL);
	if (!hisi_ambient) {
		ret = -ENOMEM;
		pr_err("no enough memory\n");
		goto node_put;
	}

	hisi_ambient->sensor_count = sensor_count;

	ret = of_property_read_u32(dev_node, "interval", &hisi_ambient->interval);
	if (ret) {
		pr_err("%s interval read err\n", __func__);
		goto free_mem;
	}

	ret = of_property_read_s32(dev_node, "bias", &hisi_ambient->bias);
	if (ret) {
		pr_err("%s bias read err\n", __func__);
		goto free_mem;
	}

	for_each_child_of_node(np, child) {
		ret = of_property_read_string(child, "type", &ptr_type);
		if (ret) {
			pr_err("%s type read err\n", __func__);
			goto free_mem;
		}

		ret = ipa_get_periph_id(ptr_type);
		if (ret < 0) {
			pr_err("%s sensor id get err\n", __func__);
			goto free_mem;
		}
		hisi_ambient->id = (u32)ret;
	}

	for_each_child_of_node(np, child) {
			ambient_sensor = (struct hisi_ambient_sensor_t *)(uintptr_t)((u64)hisi_ambient + sizeof(struct hisi_ambient_t) + (u64)((long)i) * (sizeof(struct hisi_ambient_sensor_t)));
			ret = of_property_read_string(child, "type", &ptr_type);
			if (ret) {
				pr_err("%s type read err\n", __func__);
				goto free_mem;
			}

			ret = ipa_get_periph_id(ptr_type);
			if (ret < 0) {
				pr_err("%s sensor id get err\n", __func__);
				goto free_mem;
			}
			ambient_sensor->id = (u32)ret;

			i++;
		}

	/*init data*/
	do_gettimeofday(&hisi_ambient->last_time);
	do_gettimeofday(&hisi_ambient->now);
	hisi_ambient->start_cc = -1;
	hisi_ambient->temp = DEFAULT_AMBIENT_TEMP;

	hisi_ambient->tz_dev = thermal_zone_device_register(dev_node->name,
			0, 0, hisi_ambient, &ambient_thermal_zone_ops, NULL, 0, 0);
	if (IS_ERR(hisi_ambient->tz_dev)) {
		dev_err(dev, "register thermal zone for ambient failed.\n");
		ret = -ENODEV;
		goto unregister;
	}

	of_node_put(np);

	platform_set_drvdata(pdev, hisi_ambient);

	pr_info("%s ok\n", __func__);

	return 0; /*lint !e429*/

unregister:
	thermal_zone_device_unregister(hisi_ambient->tz_dev);
free_mem:
	kfree(hisi_ambient);
node_put:
	of_node_put(np);
exit:

	return ret;
}

static int hisi_ambient_remove(struct platform_device *pdev)
{
	struct hisi_ambient_t *hisi_ambient = platform_get_drvdata(pdev);

	if (hisi_ambient) {
		platform_set_drvdata(pdev, NULL);
		thermal_zone_device_unregister(hisi_ambient->tz_dev);
		kfree(hisi_ambient);
	}

	return 0;
}
/*lint -e785*/
static struct of_device_id hisi_ambient_of_match[] = {
	{ .compatible = "hisi,ambient-temp" },
	{},
};
/*lint +e785*/
MODULE_DEVICE_TABLE(of, hisi_ambient_of_match);

int hisi_ambient_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct hisi_ambient_t *hisi_ambient;
	int now_cc = 0;
	long delta_time = 0;
	int delta_cc = 0;
	int avg_current = 0;

	pr_info("%s+\n", __func__);

	hisi_ambient = platform_get_drvdata(pdev);

	if (hisi_ambient) {
		now_cc = coul_get_battery_cc();
		/* check avg_current, and update the new time. */
		if(hisi_ambient->start_cc == -1) {
			pr_info("ambient start_cc == -1, start to log.\n");
			hisi_ambient->start_cc = now_cc;
			do_gettimeofday(&hisi_ambient->last_time);
		} else {
			do_gettimeofday(&hisi_ambient->now);
			delta_time = (hisi_ambient->now.tv_sec - hisi_ambient->last_time.tv_sec);

			if(delta_time == 0){
				return 0;
			} else {
				delta_cc = abs(now_cc - hisi_ambient->start_cc) / 1000; //unit modify from uah to mah
				if (delta_cc > (INT_MAX / HOURTOSEC)) {
					pr_err("%s-,delta battery CC is too big. now_cc is %d, start_cc is %d .\n", __func__, now_cc, hisi_ambient->start_cc);
					hisi_ambient->start_cc = -1;
					return 0;
				}

				avg_current = (delta_cc * HOURTOSEC / (int)delta_time);//unit is ma
				if(avg_current > KEEP_CURRENT) {
					pr_info("ambient avg.current (%d) too high, reset ambient monitor data.\n", avg_current);
					do_gettimeofday(&hisi_ambient->last_time);
					hisi_ambient->start_cc = now_cc;
				}
			}
		}
	}

	pr_info("%s-\n", __func__);

	return 0;
}

int hisi_ambient_resume(struct platform_device *pdev)
{
	int i = 0;
	int ambient_temp = 0;
	int min_temp = 0;
	int now_cc = 0;
	long delta_time = 0;
	int delta_cc = 0;
	int avg_current = 0;
	int ret = 0;
	struct hisi_ambient_sensor_t *ambient_sensor;
	struct hisi_ambient_t *hisi_ambient;
	struct rtc_time tm = {0};


	pr_info("%s+\n", __func__);

	hisi_ambient = platform_get_drvdata(pdev);

	if (hisi_ambient) {
		do_gettimeofday(&hisi_ambient->now);
		now_cc = coul_get_battery_cc();
		delta_time = hisi_ambient->now.tv_sec - hisi_ambient->last_time.tv_sec;

		if(0 == delta_time) {
			pr_info("%s-, delta_time is 0. \n", __func__);
			return 0;
		}

		delta_cc = abs(now_cc - hisi_ambient->start_cc) / 1000;//unit modify from uah to mah
		if (delta_cc > (INT_MAX / HOURTOSEC)) {
			pr_err("%s-,delta battery CC is too big. now_cc is %d, start_cc is %d .\n", __func__, now_cc, hisi_ambient->start_cc);
			hisi_ambient->start_cc = -1;
			return 0;
		}

		avg_current = (delta_cc * HOURTOSEC / (int)delta_time);//unit is ma
		if ( delta_time >= hisi_ambient->interval &&
			(avg_current <= LOW_CURRENT)) {
			/* time > interval (15 mins) and avg current is lower than LOW_CURRENT (50mA). */
			pr_info("%s time pass %ld > interval(%d s) , now_cc = %d, start_cc = %d, avg_current = %d < LOW_CURRENT.\n ",
					__func__, delta_time,  hisi_ambient->interval, now_cc, hisi_ambient->start_cc, avg_current);

			for (i = 0; i < hisi_ambient->sensor_count ; i++) {
				ambient_sensor = (struct hisi_ambient_sensor_t *)(uintptr_t)((u64)hisi_ambient + sizeof(struct hisi_ambient_t) + (u64)((long)i) * (sizeof(struct hisi_ambient_sensor_t)));

				ret = ipa_get_periph_value(ambient_sensor->id, &ambient_temp);
				if (ret) {
					pr_err("%s get_periph_value fail.", __func__);
					return 0;
				}

				if (i == 0 || min_temp > ambient_temp)
					min_temp = ambient_temp;
			}
			/* temp is highres, so div 1000) */
			min_temp = min_temp/1000;

			/*update ambient temp and print UTC time*/
			hisi_ambient->temp = (min_temp - hisi_ambient->bias);
			rtc_time_to_tm(hisi_ambient->now.tv_sec,&tm);

			pr_err("%s ambient temp: %d at UTC time: %d-%d-%d %d:%d:%d. \n", __func__, hisi_ambient->temp,tm.tm_year+1900, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

			/*reset calc cycle */
			hisi_ambient->start_cc = -1;
			do_gettimeofday(&hisi_ambient->last_time);
			do_gettimeofday(&hisi_ambient->now);
		} else if (avg_current > KEEP_CURRENT) {
			/*clean the data, becuase current is too high. */
			pr_info("%s avg_current = %d > KEEP_CURRENT(%d), reset calc cycle.\n ", __func__, avg_current, KEEP_CURRENT);
			hisi_ambient->start_cc = -1;
		} else {
			/* current is lower KEEP_CURRNET, but time is not enough. Just wait. */
			pr_info("%s time pass %ld, avg_current = %d.\n ", __func__, delta_time, avg_current);
		}
	}

	pr_info("%s-\n", __func__);

	return 0;
}

/*lint -e64 -e785 -esym(64,785,*)*/
static struct platform_driver hisi_ambient_platdrv = {
	.driver = {
		.name		= "hisi-ambient-temp",
		.owner		= THIS_MODULE,
		.of_match_table = hisi_ambient_of_match,
	},
	.probe	= hisi_ambient_probe,
	.remove	= hisi_ambient_remove,
	.suspend = hisi_ambient_suspend,
	.resume = hisi_ambient_resume,
};
/*lint -e64 -e785 +esym(64,785,*)*/

static int __init hisi_ambient_init(void)
{
	return platform_driver_register(&hisi_ambient_platdrv); /*lint !e64*/
}

static void __exit hisi_ambient_exit(void)
{
	platform_driver_unregister(&hisi_ambient_platdrv);
}
/*lint -e528 -esym(528,*)*/
module_init(hisi_ambient_init);
module_exit(hisi_ambient_exit);
/*lint -e528 +esym(528,*)*/

/*lint -e753 -esym(753,*)*/
MODULE_LICENSE("GPL v2");
/*lint -e753 +esym(753,*)*/
