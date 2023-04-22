

#include "hw_voltage_dev.h"
#include <asm/uaccess.h>

#define HW_VOLTAGE_CHANNELS_PERCHIP		8


#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG hw_voltage
HWLOG_REGIST();

/*
struct hw_voltage_channels {
	int result[16];
	int voltage[16];
	struct hw_voltage_info vol_info[16];
};
*/

static struct hw_voltage_device *voltage_dev;
extern int fsa7830_no_exist_flag;

/*
* kernel enable/disable switch
*/
int hw_voltage_swith_enable(struct hw_voltage_device *vol_dev,
					int enable)
{
	int ret = -1;
	struct hw_voltage_data *temp_data = NULL;

	if (!vol_dev) {
		hwlog_err("[%s] input null\n", __func__);
		return ret;
	}

	list_for_each_entry(temp_data, &vol_dev->head, list) {
		ret = temp_data->hw_voltage_enable(temp_data->data, enable);
		if (ret < 0) {
			hwlog_err("enable err; id:%d\n", temp_data->id);
			return ret;
		}
		if (enable)
			vol_dev->voltage_states |= (1 << temp_data->id);
		else
			vol_dev->voltage_states &= ~(1 << temp_data->id);
	}

	hwlog_info("status:%d\n", vol_dev->voltage_states);
	return 0;
}

/*
* the kernel interface to get the input channel voltages by inputcfg
*/
/*
static int hw_voltage_get_value(struct hw_voltage_device *vol_dev,
				struct hw_voltage_channels *vol_info,
				int inputcfg)
*/

/*
* kernel get channelvoltage whitch the channel is configed
*/
int hw_voltage_get_value(struct hw_voltage_device *vol_dev)
{
	int ret = -1;
	int channel = 0;
	int result = -1;
	int exist_flag = 0;
	struct hw_voltage_info *info;
	struct hw_voltage_data *temp_data = NULL;

	if (!vol_dev) {
		hwlog_err("[%s] input null\n", __func__);
		goto out;
	}
	info = &vol_dev->info;
	hwlog_info("id:%d channel:%d\n", info->id, info->channel);
	list_for_each_entry(temp_data, &vol_dev->head, list) {
		if (temp_data->id == info->id) {
			exist_flag = 1;
			channel = temp_data->hw_voltage_chennel
					(temp_data->data, info);
			if (0 == channel) {
				hwlog_info("hardware not support.\n");
				result = HW_VOLTAGE_HARDWARE_NULL;
				break;
			}
			if (channel < 0) {
				hwlog_info("err channel.channel:%d\n",
					info->channel);
				result = HW_VOLTAGE_HARDWARE_NULL;
				break;
			}
			ret = temp_data->hw_voltage_getvoltage
					(temp_data->data, info);
			temp_data->hw_voltage_switch_enable
					(temp_data->data, 0);

			break;
		}
	}

	if (!exist_flag) {
		hwlog_info("reserve channel set 0\n");
		memset(&vol_dev->info, 0, sizeof(vol_dev->info));
	}
out:
	return ret;
}

static ssize_t hw_voltage_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct hw_voltage_device *vol_dev = dev_get_drvdata(dev);

	return snprintf(buf, 50, "%d\n", vol_dev->voltage_states);
}

static ssize_t hw_voltage_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	int enable = 0;
	long state = 0;
	struct hw_voltage_device *vol_dev = dev_get_drvdata(dev);
	struct hw_voltage_data *temp_data = NULL;

	if(fsa7830_no_exist_flag == 1){
		hwlog_info("%s:fsa7830 no exist!\n", __func__);
		return size;
	}

	ret = kstrtol(buf, 10, &state);
	if (ret < 0) {
		hwlog_err("voltage enable input err. ret:%d\n", ret);
		return ret;
	}

	enable = !!state;
	list_for_each_entry(temp_data, &vol_dev->head, list) {
		ret = temp_data->hw_voltage_enable(temp_data->data, enable);
		if (ret < 0) {
			hwlog_err("enable err; id:%d\n", temp_data->id);
			return ret;
		}
		if (enable)
			vol_dev->voltage_states |= (1 << temp_data->id);
		else
			vol_dev->voltage_states &= ~(1 << temp_data->id);
	}
	hwlog_info("status:%d\n", vol_dev->voltage_states);
	return size;
}

static ssize_t hw_voltage_value_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int channel = 0;
	int result = 0;
	int ret = 0;
	int voltage_mv = 0;
	int min_threshold = 0;
	int max_threshold = 0;
	struct hw_voltage_device *vol_dev = dev_get_drvdata(dev);
	struct hw_voltage_info *info = NULL;
	struct hw_voltage_data *temp_data = NULL;

	if(fsa7830_no_exist_flag == 1){
		hwlog_info("%s:fsa7830 no exist!\n", __func__);
		return snprintf(buf, 50, "%d %d %d %d\n", result, voltage_mv,
				min_threshold, max_threshold);
	}

	if (NULL == vol_dev) {
		hwlog_err("dev data null.\n");
		result = HW_VOLTAGE_ERR_COMMUNICATIN;
		goto out;
	}

	info = &vol_dev->info;
	info->min_voltage = 0;
	info->max_voltage = 0;
	list_for_each_entry(temp_data, &vol_dev->head, list) {
		if (temp_data->id == info->id) {
			channel = temp_data->hw_voltage_chennel
					(temp_data->data, info);
			if (0 == channel) {
				hwlog_info("hardware not support.\n");
				result = HW_VOLTAGE_HARDWARE_NULL;
				break;
			}
			if (channel < 0) {
				hwlog_info("err channel.channel:%d\n",
					info->channel);
				result = HW_VOLTAGE_HARDWARE_NULL;
				break;
			}
			ret = temp_data->hw_voltage_getvoltage
					(temp_data->data, info);
			if (ret < 0) {
				hwlog_err("voltage:%d\n", voltage_mv);
				result = HW_VOLTAGE_ERR_COMMUNICATIN;
				break;
			}
			voltage_mv = info->voltage;
			if (voltage_mv < 0) {
				ret = HW_VOLTAGE_ERR_COMMUNICATIN;
			} else {
				min_threshold = info->min_voltage;
				max_threshold = info->max_voltage;
				if (!min_threshold && !max_threshold) {
					result = HW_VOLTAGE_OK_NO_THRESHOLD;
				} else {
					if (voltage_mv < min_threshold)
						result =
						  HW_VOLTAFE_ERR_OUTOF_LOW_RANG;
					else if (voltage_mv > max_threshold)
						result =
						HW_VOLTAGE_ERR_OUTOF_HIGHT_RANGE;
					else
						result =
						  HW_VOLTAGE_OK_WITH_THRESHOLD;
				}
			}
			temp_data->hw_voltage_switch_enable
					(temp_data->data, 0);
			break;
		}
	}

out:
	if (result < 1) {
		voltage_mv = 0;
		min_threshold = 0;
		max_threshold = 0;
	}
	hwlog_info("%d %d %d %d\n", result, voltage_mv,
				min_threshold, max_threshold);
	return snprintf(buf, 50, "%d %d %d %d\n", result, voltage_mv,
				min_threshold, max_threshold);
}

static ssize_t hw_voltage_value_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	long state = 0;
	struct hw_voltage_device *vol_dev = dev_get_drvdata(dev);
	struct hw_voltage_info *info = NULL;

	if(fsa7830_no_exist_flag == 1){
		hwlog_info("%s:fsa7830 no exist!\n", __func__);
		return size;
	}

	if (NULL == vol_dev) {
		hwlog_err("dev data null.\n");
		return -1;
	}

	ret = kstrtol(buf, 10, &state);
	if (ret < 0) {
		hwlog_err("voltage value input err. ret:%d\n", ret);
		return ret;
	}

	info = &vol_dev->info;
	/* a chip 8 channel */
	info->id = state/HW_VOLTAGE_CHANNELS_PERCHIP;
	info->channel = state%HW_VOLTAGE_CHANNELS_PERCHIP;
	hwlog_info("id:%d; channel:%d\n", info->id, info->channel);

	return size;
}

static ssize_t hw_voltage_values_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned long length = 0;
	unsigned long ret = 0;
	struct hw_voltage_device *vol_dev = dev_get_drvdata(dev);
	struct hw_voltage_info info;

	if (NULL == vol_dev) {
		hwlog_err("dev data null.\n");
		return -1;
	}

	if(fsa7830_no_exist_flag == 1){
		hwlog_info("%s:fsa7830 no exist!\n", __func__);
		vol_dev->info.id = 0;
		vol_dev->info.channel = 0;
		vol_dev->info.voltage = 0;
		vol_dev->info.min_voltage = 0;
		vol_dev->info.max_voltage = 0;

		length = sizeof(struct hw_voltage_info);
		if (length > 1024)
			length = 1024;
		memcpy(buf, &vol_dev->info, length);
		return (size_t)length;
	}

	hw_voltage_get_value(vol_dev);

	length = sizeof(struct hw_voltage_info);
	if (length > 1024)
		length = 1024;
	hwlog_info("length:%lu\n", length);
	hwlog_info("id:%d; channel:%d;voltage:%d\n",
			vol_dev->info.id,
			vol_dev->info.channel,
			vol_dev->info.voltage);
	memcpy(buf, &vol_dev->info, length);
	hwlog_info("ret:%lu\n", length);
	return (size_t)length;
}

DEVICE_ATTR(vol_enable, 0660, hw_voltage_enable_show, hw_voltage_enable_store);
DEVICE_ATTR(vol_value, 0660, hw_voltage_value_show, hw_voltage_value_store);
DEVICE_ATTR(vol_values, 0440, hw_voltage_values_show, NULL);

static struct attribute *hw_voltage_attributes[] = {
	&dev_attr_vol_enable.attr,
	&dev_attr_vol_value.attr,
	&dev_attr_vol_values.attr,
	NULL,
};

static const struct attribute_group hw_voltage_attr_group = {
	.attrs = hw_voltage_attributes,
};

static int hw_voltage_init(struct hw_voltage_device *vol_dev)
{
	int ret = -1;

	hwlog_info("[%s] +++++\n", __func__);
	vol_dev->vol_class = class_create(THIS_MODULE, "voltage");
	if (IS_ERR(vol_dev->vol_class)) {
		hwlog_err("class create err.\n");
		goto out;
	}
	vol_dev->dev = device_create(vol_dev->vol_class,
				NULL, 0, vol_dev, "voltage");
	if (NULL == vol_dev->dev) {
		hwlog_err("device create err.\n");
		goto out;
	}
	INIT_LIST_HEAD(&vol_dev->head);
	ret = sysfs_create_group(&vol_dev->dev->kobj, &hw_voltage_attr_group);
	if (ret) {
		ret = -1;
	}
	vol_dev->voltage_states = 0;
	vol_dev->info.min_voltage = 0;
	vol_dev->info.max_voltage = 0;
	vol_dev->info.id = -1;
	vol_dev->info.channel = -1;
	ret = 0;

out:
	hwlog_info("[%s] -----\n", __func__);
	return ret;
}

static void hw_voltage_exit(struct hw_voltage_device *vol_dev)
{
	sysfs_remove_group(&vol_dev->dev->kobj, &hw_voltage_attr_group);
	device_destroy(vol_dev->vol_class, vol_dev->dev->devt);
	class_destroy(vol_dev->vol_class);
}

struct hw_voltage_data *hw_voltage_register(void *data, int id)
{
	int ret = 0;
	struct hw_voltage_data *vol_data = NULL;

	if (!voltage_dev) {
		voltage_dev = (struct hw_voltage_device *)
			kzalloc(sizeof(struct hw_voltage_device), GFP_KERNEL);
		if (!voltage_dev) {
			hwlog_err("zalloc voltage_dev err.\n");
			goto out;
		}
		ret = hw_voltage_init(voltage_dev);
		if (ret < 0) {
			hwlog_err("hw voltage init err. ret:%d\n", ret);
			goto out;
		}
	}

	vol_data = (struct hw_voltage_data *)
		kzalloc(sizeof(struct hw_voltage_data), GFP_KERNEL);
	if (NULL == vol_data) {
		hwlog_err("hw voltage data kzalloc err.\n");
		goto out;
	}
	vol_data->id = id;
	vol_data->data = data;
	list_add_tail(&vol_data->list, &voltage_dev->head);
	hwlog_info("voltage register success. id:%d\n", id);

out:
	return vol_data;
}
EXPORT_SYMBOL_GPL(hw_voltage_register);

void hw_voltage_unregister(int id)
{
	struct hw_voltage_data *vol_data = NULL;
	struct hw_voltage_data *temp_data = NULL;

	list_for_each_entry_safe(vol_data, temp_data,
					&voltage_dev->head, list) {
		if (temp_data->id == id) {
			list_del(&vol_data->list);
			kfree(vol_data);
		}
	}
	if (list_empty(&voltage_dev->head)) {
		hw_voltage_exit(voltage_dev);
		kfree(voltage_dev);
	}

	hwlog_info("voltage unregister success. id:%d\n", id);
}
EXPORT_SYMBOL_GPL(hw_voltage_unregister);
