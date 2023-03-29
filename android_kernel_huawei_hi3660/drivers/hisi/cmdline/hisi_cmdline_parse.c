#include <linux/of_platform.h>
#include <linux/memblock.h>
#include <linux/irqchip.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include "securec.h"

/* Format of hex string: 0x12345678 */
#define RUNMODE_FLAG_NORMAL  0
#define RUNMODE_FLAG_FACTORY 1
#define HEX_STRING_MAX  (10)
#define TRANSFER_BASE    (16)

static unsigned int runmode_factory = RUNMODE_FLAG_NORMAL;

static int __init early_parse_runmode_cmdline(char *p)
{
	if (p) {
		if (!strncmp(p, "factory", strlen("factory")))
			runmode_factory = RUNMODE_FLAG_FACTORY;

		pr_info("runmode is %s, runmode_factory = %d\n", p,
		       runmode_factory);
	}

	return 0;
}

early_param("androidboot.swtype", early_parse_runmode_cmdline);

/* the function interface to check factory/normal mode in kernel */
unsigned int runmode_is_factory(void)
{
	return runmode_factory;
}
EXPORT_SYMBOL(runmode_is_factory);

static int logctl_flag;	/* default 0, switch logger off */
int get_logctl_value(void)
{
	return logctl_flag;
}
EXPORT_SYMBOL(get_logctl_value);

unsigned int enter_recovery_flag;
/**
* parse boot_into_recovery cmdline which is passed from boot_recovery() of boot.c *
* Format : boot_into_recovery_flag=0 or 1             *
*/
static int __init early_parse_enterrecovery_cmdline(char *p)
{
	int ret = 0;
	char enter_recovery[HEX_STRING_MAX + 1];

	memset(enter_recovery, 0, HEX_STRING_MAX + 1);

	memcpy(enter_recovery, p, HEX_STRING_MAX);
	enter_recovery[HEX_STRING_MAX] = '\0';

	ret = kstrtouint(enter_recovery, TRANSFER_BASE, &enter_recovery_flag);
	if (ret)
		return ret;

	pr_info("enter recovery p:%s, enter_recovery_flag :%d\n", p,
	       enter_recovery_flag);

	return 0;
}

early_param("enter_recovery", early_parse_enterrecovery_cmdline);

unsigned int get_boot_into_recovery_flag(void)
{
	return enter_recovery_flag;
}
EXPORT_SYMBOL(get_boot_into_recovery_flag);

unsigned int recovery_update_flag;
/**
* Format : recovery_update=0 or 1 *
*/
static int __init early_parse_recovery_update_cmdline(char *p)
{
	int ret = 0;
	char recovery_update[HEX_STRING_MAX + 1];

	memset(recovery_update, 0, HEX_STRING_MAX + 1);

	memcpy(recovery_update, p, HEX_STRING_MAX);
	recovery_update[HEX_STRING_MAX] = '\0';

	ret = kstrtouint(recovery_update, TRANSFER_BASE, &recovery_update_flag);
	if (ret)
		return ret;

	pr_info("recovery_update p:%s, recovery_update_flag :%d\n", p,
	       recovery_update_flag);

	return 0;
}

early_param("recovery_update", early_parse_recovery_update_cmdline);

unsigned int get_recovery_update_flag(void)
{
	return recovery_update_flag;
}
EXPORT_SYMBOL(get_recovery_update_flag);

static int __init early_parse_logctl_cmdline(char *p)
{
	int ret = 0;
	char logctl[HEX_STRING_MAX + 1] = {'\0'};

	if (p == NULL) {
		logctl_flag = 0;	/* error parameter, switch logger off */
		pr_err("%s: setup_logctl parameter error!\n", __func__);
		return 0;
	}

	memset(logctl, 0, HEX_STRING_MAX + 1);

	memcpy(logctl, p, HEX_STRING_MAX);

	ret = kstrtoint(logctl, TRANSFER_BASE, &logctl_flag);
	if (ret)
		return ret;

	pr_info("%s: p: %s, logctl: %d\n", __func__, p, logctl_flag);

	return 0;
}

early_param("setup_logctl", early_parse_logctl_cmdline);

/*******************
*0: battery is normal;
*1: battery is low;
*********************/
static int g_lowbattery;

int get_lowbatteryflag(void)
{
	return g_lowbattery;
}
EXPORT_SYMBOL(get_lowbatteryflag);

void set_lowBatteryflag(int flag)
{
	g_lowbattery = flag;
}
EXPORT_SYMBOL(set_lowBatteryflag);

static int __init early_parse_lowpower_cmdline(char *p)
{
	int ret = 0;
	char clow[HEX_STRING_MAX + 1] = {'\0'};

	if (p == NULL) {
		g_lowbattery = 0;	/* error parameter, switch logger off */
		pr_err("%s input is NULL\n", __func__);
		return 0;
	}

	memset(clow, 0, HEX_STRING_MAX + 1);
	memcpy(clow, p, HEX_STRING_MAX);

	ret = kstrtoint(clow, TRANSFER_BASE, &g_lowbattery);
	if (ret)
		return ret;

	pr_info("%s %d\n", __func__, g_lowbattery);
	return 0;
}

early_param("low_volt_flag", early_parse_lowpower_cmdline);

static int need_dsm_notify = 1;
int get_dsm_notify_flag(void)
{
	return need_dsm_notify;
}
EXPORT_SYMBOL(get_dsm_notify_flag);

static int __init early_parse_dsm_notify_cmdline(char *p)
{
	int ret = 0;
	char clow[HEX_STRING_MAX + 1] = {'\0'};

	if (p == NULL) {
		need_dsm_notify = 1;       /* error parameter, dsm notify */
		pr_err("%s input is NULL\n", __func__);
		return 0;
	}

	memset(clow, 0, HEX_STRING_MAX + 1);
	memcpy_s(clow, HEX_STRING_MAX, p, HEX_STRING_MAX);

	ret = kstrtoint(clow, TRANSFER_BASE, &need_dsm_notify);
	if (ret)
		return ret;

	pr_info("%s %d\n", __func__, need_dsm_notify);
	return 0;
}

early_param("dsm_notify", early_parse_dsm_notify_cmdline);

/**
 * parse powerdown charge cmdline which is passed from fastoot. *
 * Format : pd_charge=0 or 1             *
 */
static unsigned int pd_charge_flag;
static int __init early_parse_pdcharge_cmdline(char *p)
{
	if (p) {
		if (!strncmp(p, "charger", strlen("charger")))
			pd_charge_flag = 1;
		else
			pd_charge_flag = 0;

		pr_info("power down charge p:%s, pd_charge_flag :%u\n", p,
			   pd_charge_flag);
	}

	return 0;
}

early_param("androidboot.mode", early_parse_pdcharge_cmdline);

unsigned int get_pd_charge_flag(void)
{
	return pd_charge_flag;
}
EXPORT_SYMBOL(get_pd_charge_flag);

static unsigned int userlock = 0;
static int __init early_parse_userlock_cmdline(char *p)
{
	if (p) {
		if (!strncmp(p, "locked", strlen("locked")))
			userlock = 1;
		else
			userlock = 0;

		pr_info("userlock p:%s, userlock :%u\n", p,
			   userlock);
	}

	return 0;
}

early_param("userlock", early_parse_userlock_cmdline);

unsigned int get_userlock(void)
{
	return userlock;
}
EXPORT_SYMBOL(get_userlock);

unsigned int bsp_need_loadmodem(void)
{
	unsigned int rs = 0;

	rs |= get_recovery_update_flag();
	return !rs;
}
EXPORT_SYMBOL(bsp_need_loadmodem);

unsigned int is_load_modem(void)
{
	unsigned int rs = 0;

	rs |= get_recovery_update_flag();
	rs |= get_pd_charge_flag();
	return !rs;
}
EXPORT_SYMBOL(is_load_modem);
