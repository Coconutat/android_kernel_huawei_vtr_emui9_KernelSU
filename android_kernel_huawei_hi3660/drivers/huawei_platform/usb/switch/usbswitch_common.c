#include "usbswitch_common.h"
#include <huawei_platform/log/hw_log.h>
#include <protocol.h>
#include <linux/errno.h>

#define HWLOG_TAG usbswitch_common
HWLOG_REGIST();

struct usbswitch_common_ops *g_switch_ops;

#ifdef CONFIG_INPUTHUB
extern sys_status_t iom3_sr_status;
#endif

int usbswitch_common_ops_register(struct usbswitch_common_ops *ops)
{
	int ret = 0;

	if (ops != NULL) {
		g_switch_ops = ops;
		hwlog_info("usbswitch common ops register ok\n");
	}
	else {
		hwlog_info("usbswitch common ops register fail!\n");
		ret = -1;
	}

	return ret;
}

int usbswitch_common_manual_sw(int input_select)
{
	if (NULL == g_switch_ops || NULL == g_switch_ops->manual_switch
#ifdef CONFIG_INPUTHUB
	|| iom3_sr_status == ST_SLEEP
#endif
	) {
		hwlog_err("error: g_switch_ops is null or sensorhub is sleep!\n");
		return -1;
	}

	return g_switch_ops->manual_switch(input_select);
}

int usbswitch_common_manual_detach(void)
{
	if (NULL == g_switch_ops || NULL == g_switch_ops->manual_detach
#ifdef CONFIG_INPUTHUB
	|| iom3_sr_status == ST_SLEEP
#endif
	) {
		hwlog_err("error: g_switch_ops is null or sensorhub is sleep!\n");
		return -1;
	}

	return g_switch_ops->manual_detach();
}

int usbswitch_common_dcd_timeout_enable(bool enable_flag)
{
	if (NULL == g_switch_ops || NULL == g_switch_ops->dcd_timeout_enable
#ifdef CONFIG_INPUTHUB
	|| iom3_sr_status == ST_SLEEP
#endif
	) {
		hwlog_err("error: g_switch_ops is null or sensorhub is sleep!\n");
		return -1;
	}

	return g_switch_ops->dcd_timeout_enable(enable_flag);
}

int usbswitch_common_dcd_timeout_status(void)
{
	if (NULL == g_switch_ops || NULL == g_switch_ops->dcd_timeout_status
#ifdef CONFIG_INPUTHUB
	|| iom3_sr_status == ST_SLEEP
#endif
	) {
		hwlog_err("error: g_switch_ops is null or sensorhub is sleep!\n");
		return -1;
	}

	return g_switch_ops->dcd_timeout_status();
}
