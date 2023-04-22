#include "../rasbase/rasbase.h"
#include "../rasbase/rasprobe.h"
#include "../rasbase/rasproc.h"
#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/statfs.h>
#include <linux/module.h>
#include <linux/path.h>
#include <linux/kthread.h>
#include <linux/thermal.h>

struct Fault {
	char *thermal_type;
	int temperature;
};

struct FaultList {
	struct Fault ft[16];
	rwlock_t rwk;
};
static struct FaultList flt;

#define VALID_TEMPREATURE -255
static int should_fail(const char *type)
{
	int i, len = ARRAY_SIZE(flt.ft), temp = VALID_TEMPREATURE;

	for (i = 0; i < len; i++) {
		if (NULL == flt.ft[i].thermal_type)
			continue;
		if (0 == strcmp(type, flt.ft[i].thermal_type)) {
			temp = flt.ft[i].temperature;
			break;
		}
	}
	return temp;
}

static int fail(const char *type, int temp)
{
	int i, j = -1, len = ARRAY_SIZE(flt.ft);

	for (i = 0; i < len; i++) {
		if (NULL == flt.ft[i].thermal_type) {
			if (-1 == j)
				j = i;
			continue;
		}
		if (0 == strcmp(type, flt.ft[i].thermal_type)) {
			j = i;
			break;
		}
	}
	if (j == -1)
		return -1;
	ras_retn_iferr(ras_malloc
		       ((void **)&flt.ft[j].thermal_type, strlen(type) + 1));
	strcpy(flt.ft[j].thermal_type, type);
	flt.ft[j].temperature = temp;
	return 0;
}

static void restore(struct Fault *ft)
{
	if (!ft->thermal_type)
		ras_free(ft->thermal_type);
	memset(ft, 0, sizeof(struct Fault));
}

static int clean(void)
{
	int i, len = ARRAY_SIZE(flt.ft);

	for (i = 0; i < len; i++)
		restore(&flt.ft[i]);

	return 0;
}

#ifdef CONFIG_HISI_THERMAL_PERIPHERAL
#include "drivers/thermal/hisi/hisi_peripheral_tm.h"
static int rasprobe_handler(hisi_peripheral_ntc_2_temp) (
	struct rasprobe_instance *ri, struct pt_regs *regs) {
	int temp;
	unsigned int *temperature = NULL;
	struct RasRegs *rd = (struct RasRegs *)ri->data;
	struct periph_tsens_tm_device_sensor *psensor = rd->args[0];
	struct thermal_zone_device *pzone = NULL;

	if (!psensor)
		return 0;

	pzone = psensor->tz_dev;
	if (!pzone)
		return 0;
	temperature = rd->args[1];
	temp = should_fail(pzone->type);
	if (VALID_TEMPREATURE != temp) {
		pr_warn("ThermalType:%s,Old:%d,New:%d\n", pzone->type,
		       *temperature, temp);
		*temperature = temp;
	}
	return 0;
}
#endif

#ifdef CONFIG_HISI_THERMAL_TSENSOR
static int rasprobe_handler(tsens_tz_get_temp) (struct rasprobe_instance *ri,
						struct pt_regs *regs) {
	int temp;
	unsigned int *temperature = NULL;
	struct RasRegs *rd = (struct RasRegs *)ri->data;
	struct thermal_zone_device *pzone = rd->args[0];

	if (!pzone)
		return 0;
	temperature = rd->args[1];
	temp = should_fail(pzone->type);
	if (VALID_TEMPREATURE != temp) {
		pr_warn("ThermalType:%s,Old:%d,New:%d\n", pzone->type,
		       *temperature, temp);
		*temperature = temp;
	}
	return 0;
}
#endif

#ifdef CONFIG_HISI_COUL
static int rasprobe_handler(hisi_battery_temperature) (struct rasprobe_instance
						       *ri,
						       struct pt_regs *regs) {
	int temp = should_fail("Battery");

	if (VALID_TEMPREATURE != temp) {
		pr_warn("ThermalType:Battery,Old:%d,New:%d\n",
		       (int)regs_return_value(regs), temp);
		rasprobe_seturn(regs, temp);
	}
	return 0;
}
#endif

#ifdef CONFIG_HISI_IPA_THERMAL
static int rasprobe_handler(get_temp_value) (struct rasprobe_instance *ri,
					     struct pt_regs *regs) {
	struct RasRegs *rd = (struct RasRegs *)ri->data;
	unsigned int *temperature = rd->args[1];
	int temp = should_fail("soc_thermal");

	if (VALID_TEMPREATURE != temp)
		*temperature = temp * 1000;
	return 0;
}
#endif

static int rasprobe_handler(power_supply_read_temp) (struct rasprobe_instance
						     *ri,
						     struct pt_regs *regs) {
	int temp;
	struct RasRegs *rd = (struct RasRegs *)ri->data;
	struct thermal_zone_device *pzone = rd->args[0];
	unsigned int *temperature = rd->args[1];

	temp = should_fail(pzone->type);
	if (VALID_TEMPREATURE != temp) {
		pr_warn("ThermalType:%s,Old:%d,New:%d\n", pzone->type,
		       *temperature, temp);
		*temperature = temp * 1000;
	}
	return 0;
}

rasprobe_define(power_supply_read_temp);
#ifdef CONFIG_HISI_COUL
rasprobe_define(hisi_battery_temperature);
#endif

#ifdef CONFIG_HISI_IPA_THERMAL
rasprobe_define(get_temp_value);
#endif
#ifdef CONFIG_HISI_THERMAL_PERIPHERAL
rasprobe_define(hisi_peripheral_ntc_2_temp);
#endif

#ifdef CONFIG_HISI_THERMAL_TSENSOR
rasprobe_define(tsens_tz_get_temp);
#endif

static struct rasprobe *probes[] = {
#ifdef CONFIG_HISI_THERMAL_PERIPHERAL
	&rasprobe_name(hisi_peripheral_ntc_2_temp),
#endif
#ifdef CONFIG_HISI_THERMAL_TSENSOR
	&rasprobe_name(tsens_tz_get_temp),
#endif

#ifdef CONFIG_HISI_COUL
	&rasprobe_name(hisi_battery_temperature),
#endif
#ifdef CONFIG_HISI_IPA_THERMAL
	&rasprobe_name(get_temp_value),
#endif
	&rasprobe_name(power_supply_read_temp),
};

static int cmd_main(void *data, int argc, char *argv[])
{
	long long temp;

	ras_retn_if(2 > argc, -EINVAL);
	ras_retn_iferr(ras_atoll(&temp, argv[1], strlen(argv[1]), 0));
	ras_retn(fail(argv[0], temp));
}

static int proc_ops_show(rThermal)(struct seq_file *m, void *v)
{
	struct Fault *ft;
	int i, len = ARRAY_SIZE(flt.ft);
	/*read_lock(&flt.rwk); */
	for (i = 0; i < len; i++) {
		ft = &flt.ft[i];
		if (NULL == ft->thermal_type)
			continue;
		seq_printf(m, "%2d\t%20s\t%d\n", i, ft->thermal_type,
			   ft->temperature);
	}
	/*read_unlock(&flt.rwk); */
	return 0;
}

static int proc_ops_open(rThermal)(struct inode *inode, struct file *file)
{
	return single_open(file, proc_ops_show(rThermal), PDE_DATA(inode));

}

static ssize_t proc_ops_write(rThermal)(struct file *filp,
					 const char __user *bff, size_t count,
					 loff_t *data) {
	char buf_cmd[256] = { 0 };

	if (unlikely(count >= sizeof(buf_cmd)))
		return -ENOMEM;
	ras_retn_iferr(copy_from_user(buf_cmd, bff, count));
	ras_retn_iferr(ras_args(buf_cmd,
				count, cmd_main, PDE_DATA(FILE_NODE(filp))));
	return count;
}

#define MODULE_NAME "rThermal"
proc_ops_define(rThermal);
static int tool_init(void)
{
	ras_debugset(1);
	ras_retn_iferr(ras_check());
	memset(&flt, 0, sizeof(flt));
	rwlock_init(&flt.rwk);
	ras_retn_iferr(register_rasprobes(probes, ARRAY_SIZE(probes)));
	ras_retn_iferr(proc_init(MODULE_NAME, &proc_ops_name(rThermal), &flt));
	return 0;
}
static void tool_exit(void)
{
	clean();
	unregister_rasprobes(probes, ARRAY_SIZE(probes));
	proc_exit(MODULE_NAME);
}

module_init(tool_init);
module_exit(tool_exit);
MODULE_DESCRIPTION("Change the Temperature of each thermal manager.");
MODULE_LICENSE("GPL");
#ifndef RASFIRE_VERSION
#define RASFIRE_VERSION "V001R001C151-"
#endif
MODULE_VERSION(RASFIRE_VERSION "1.0");
