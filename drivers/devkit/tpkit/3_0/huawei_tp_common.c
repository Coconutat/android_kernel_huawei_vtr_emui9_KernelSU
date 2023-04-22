#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_ts_kit.h>
#include <linux/notifier.h>

#define TP_COLOR_BUF_SIZE		20
#define WHITE	0xE1
#define BLACK	0xD2
#define CMDLINE_PANEL_NAME "boe_nt51017te_9p6_800p_video"

extern char tp_color_buf[TP_COLOR_BUF_SIZE];
unsigned int panel_name_flag;

int __init early_parse_tp_color_cmdline(char *arg)
{
	unsigned int len = 0;
	memset(tp_color_buf, 0, sizeof(tp_color_buf));
	if (arg) {
		len = strlen(arg);

		if (len >= sizeof(tp_color_buf)) {
			len = sizeof(tp_color_buf) - 1;
		}
		memcpy(tp_color_buf, arg, len);
	} else {
		TS_LOG_INFO("%s : arg is NULL\n", __func__);
	}

	return 0;
}

/*lint -save -e* */
early_param("TP_COLOR", early_parse_tp_color_cmdline);

/*lint -restore*/

/**
* parse panel_name cmdline which is passed from lk *
* Format : //new agassi lcd:panel_name_flag = true; *
* Format : //old agassi lcd:panel_name_flag = false; *
*/

static int __init early_parse_panel_name_cmdline(char *p)
{
	if (p) {
		if (NULL != strstr(p, CMDLINE_PANEL_NAME))
			panel_name_flag = true;
		else
			panel_name_flag = false;
		TS_LOG_INFO("panel_name_flag :%u\n", panel_name_flag);
	}
	return 0;
}

early_param("mdss_mdp.panel", early_parse_panel_name_cmdline);

