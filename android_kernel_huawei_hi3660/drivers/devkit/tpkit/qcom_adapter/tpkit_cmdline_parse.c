#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/module.h>

#include <huawei_platform/log/hw_log.h>
#define HWLOG_TAG TS_KIT
HWLOG_REGIST();


static unsigned int enter_recovery_flag;
static unsigned int pd_charge_flag;

#define CMDLINE_RECOVERY_MODE "recovery"

/**
* parse huawei_bootmode cmdline which is passed from lk *
* Format : //androidboot.huawei_bootmode=recovery          *
*/
static int __init early_parse_enterrecovery_cmdline(char *bootmode)
{
    if (bootmode && !strncmp(bootmode, CMDLINE_RECOVERY_MODE, strlen(CMDLINE_RECOVERY_MODE))) 
    {
        enter_recovery_flag = true;
    }

    hwlog_info("boot mode cmdline: [%s], enter_recovery_flag [%d]\n", bootmode, enter_recovery_flag);
    return 0;
}

early_param("androidboot.huawei_bootmode", early_parse_enterrecovery_cmdline);

unsigned int get_boot_into_recovery_flag(void)
{
	return enter_recovery_flag;
}
//EXPORT_SYMBOL(get_boot_into_recovery_flag);

/**
* parse huawei_chrager cmdline which is passed from lk *
* Format : //androidboot.mode=charger *
*/

static int __init early_parse_pdcharge_cmdline(char *p)
{
	if (p) 
	{
		if (!strncmp(p, "charger", strlen("charger")))
			pd_charge_flag = true;
		else
			pd_charge_flag = false;
		hwlog_info("power down charge p:%s, pd_charge_flag :%u\n", p,pd_charge_flag);
	}
	return 0;
}

early_param("androidboot.mode", early_parse_pdcharge_cmdline);

unsigned int get_pd_charge_flag(void)
{
	return pd_charge_flag;
}
//EXPORT_SYMBOL(get_pd_charge_flag);

