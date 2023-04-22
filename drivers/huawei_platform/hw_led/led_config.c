#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/hisi/hisi_leds.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/touthscreen/huawei_tp_color.h>
#include "led_config.h"

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG  led_config
HWLOG_REGIST();

struct led_config_data {
	uint8_t retry_times;
	uint8_t tp_color_flag;
	uint8_t tp_module_flag;
	uint8_t tp_ic_flag;
	uint8_t got_tp_info_flag;

	uint8_t work_flag;
	struct delayed_work work;

	struct led_current_config curr;
	struct device_node* node;
};

static struct led_config_data* g_data = NULL;

extern int tp_color_provider(void);
extern int tp_project_id_provider(char* name, uint8_t len);
static int led_config_parse_current_setting(struct led_config_data* data);

void led_config_get_current_setting(struct hisi_led_platform_data* hisi_leds)
{
	if (NULL == hisi_leds || NULL == g_data) {
		pr_err("%s null pointer error!\n", __func__);
		return;
	}

	if (!(g_data->tp_color_flag || g_data->tp_module_flag || g_data->tp_ic_flag)) {
		pr_debug("%s do not use led_current adjust feature!\n", __func__);
		return;
	}

	if (!g_data->curr.got_flag) {
		if (g_data->retry_times > 0) {
			g_data->retry_times--;
			if (g_data->work_flag) {
				cancel_delayed_work(&g_data->work);
				g_data->work_flag = 0;
			}
			led_config_parse_current_setting(g_data);
			hwlog_info("%s retry num %d!\n", __func__, g_data->retry_times);
		} else {
			hwlog_err("%s can not obtain tp_info, do not using the feature!\n", __func__);
			return;
		}
	}

	if (!g_data->got_tp_info_flag && g_data->curr.got_flag) {
		pr_info("%s set maxdr_iset!\n", __func__);
		hisi_leds->leds[0].each_maxdr_iset = g_data->curr.red_curr;
		hisi_leds->leds[1].each_maxdr_iset = g_data->curr.green_curr;
		hisi_leds->leds[2].each_maxdr_iset = g_data->curr.blue_curr;
		g_data->got_tp_info_flag = 1;
	}
	return;
}
EXPORT_SYMBOL(led_config_get_current_setting);

static uint8_t led_config_parse_single_current_conf(struct led_config_data* data, char* tp_module,
		char* tp_ic, char* tp_color, const char* led_name, uint8_t* ret_val)
{
	uint8_t flag = 0;
	uint8_t tmp_ret_val = 0;
	char config_name[128] = {0}; // total len
	uint32_t tmp = 0;

	if (NULL == data || NULL == tp_module || NULL == tp_ic ||
			NULL == tp_color || NULL == led_name || NULL == ret_val) {
		hwlog_err("%s null pointer error!\n", __func__);
		return flag;
	}

	if (data->tp_module_flag && data->tp_ic_flag && data->tp_color_flag) { // tp_module+tp_ic+tp_color

		if (sizeof(config_name) < (strlen(tp_module) + strlen("_module_") +
					strlen(tp_ic) + strlen("_ic_") +
					strlen(tp_color) + strlen("_color_tp_") +
					strlen(led_name) + 1)) { // + '\0'
			hwlog_err("%s config_name buffer overflow error!\n", __func__);
			return flag;
		}
		sprintf(config_name, "%s_module_%s_ic_%s_color_tp_%s", tp_module, tp_ic, tp_color, led_name);

	} else if (data->tp_module_flag && data->tp_ic_flag) { // tp_module+tp_ic

		if (sizeof(config_name) < (strlen(tp_module) + strlen("_module_") +
					strlen(tp_ic) + strlen("_ic_tp") +
					strlen(led_name) + 1)) { // + '\0'
			hwlog_err("%s config_name buffer overflow error!\n", __func__);
			return flag;
		}
		sprintf(config_name, "%s_module_%s_ic_tp_%s", tp_module, tp_ic, led_name);

	} else if (data->tp_module_flag && data->tp_color_flag) { // tp_module+tp_color

		if (sizeof(config_name) < (strlen(tp_module) + strlen("_module_") +
					strlen(tp_color) + strlen("_color_tp") +
					strlen(led_name) + 1)) { // + '\0'
			hwlog_err("%s config_name buffer overflow error!\n", __func__);
			return flag;
		}
		sprintf(config_name, "%s_module_%s_color_tp_%s", tp_module, tp_color, led_name);

	} else if (data->tp_ic_flag && data->tp_color_flag) { // tp_ic+tp_color

		if (sizeof(config_name) < (strlen(tp_ic) + strlen("_ic_") +
					strlen(tp_color) + strlen("_color_tp") +
					strlen(led_name) + 1)) { // + '\0'
			hwlog_err("%s config_name buffer overflow error!\n", __func__);
			return flag;
		}
		sprintf(config_name, "%s_ic_%s_color_tp_%s", tp_ic, tp_color, led_name);

	} else if (data->tp_module_flag) { // tp_module

		if (sizeof(config_name) < (strlen(tp_module) + strlen("_module_tp_") + strlen(led_name) + 1)) { // + '\0'
			hwlog_err("%s config_name buffer overflow error!\n", __func__);
			return flag;
		}
		sprintf(config_name, "%s_module_tp_%s", tp_module, led_name);

	} else if (data->tp_ic_flag) { // tp_ic

		if (sizeof(config_name) < (strlen(tp_ic) + strlen("_module_ic_") + strlen(led_name) + 1)) { // + '\0'
			hwlog_err("%s config_name buffer overflow error!\n", __func__);
			return flag;
		}
		sprintf(config_name, "%s_ic_tp_%s", tp_ic, led_name);

	} else if (data->tp_color_flag) { // tp_color

		if (sizeof(config_name) < (strlen(tp_color) + strlen("_color_tp_") + strlen(led_name) + 1)) { // + '\0'
			hwlog_err("%s config_name buffer overflow error!\n", __func__);
			return flag;
		}
		sprintf(config_name, "%s_color_tp_%s", tp_color, led_name);

	} else { // error
		hwlog_err("%s error return!\n", __func__);
		return flag;
	}

	hwlog_info("%s %s!\n", __func__, config_name);

	GET_U8_FROM_NODE(data->node, config_name, tmp, tmp_ret_val, flag);
	*ret_val = tmp_ret_val;

	return flag;
}

static bool led_config_get_tp_color_name(struct led_config_data *data, char* color_name, uint8_t len)
{
	int tp_color = 0;
	char* tmp_str = NULL;

	if (NULL == data || NULL == color_name) {
		hwlog_err("%s null pointer error!\n", __func__);
		return false;
	}

	tp_color = tp_color_provider();
	if (-1 == tp_color) { // fail return -1
		data->tp_color_flag = 0;
		hwlog_err("%s got tp_color fail!\n", __func__);
		return false;
	}
	hwlog_info("%s tp_color:0x%x\n", __func__, tp_color);

	switch (tp_color) {
		case WHITE:
			tmp_str = "white";
			break;
		case BLACK:
			tmp_str = "black";
			break;
		case BLACK2:
			tmp_str = "black2";
			break;
		case PINK:
			tmp_str = "pink";
			break;
		case RED:
			tmp_str = "red";
			break;
		case YELLOW:
			tmp_str = "yellow";
			break;
		case BLUE:
			tmp_str = "blue";
			break;
		case GOLD:
			tmp_str = "gold";
			break;
		case PINKGOLD:
			tmp_str = "pinkgold";
			break;
		case SILVER:
			tmp_str = "silver";
			break;
		case GRAY:
			tmp_str = "gray";
			break;
		case CAFE:
			tmp_str = "cafe";
			break;
		case CAFE2:
			tmp_str = "cafe2";
			break;
		case GREEN:
			tmp_str = "green";
			break;
		default:
			hwlog_err("%s tp_color_id error!\n", __func__);
			return false;
	}

	if (tmp_str == NULL) {
		return false;
	}
	if (len <= (uint8_t)strlen(tmp_str)) {
		hwlog_err("%s color_name len is too small, tmp_str len %d!\n", __func__, (int)strlen(tmp_str));
		return false;
	}
	memcpy(color_name, tmp_str, ((int)strlen(tmp_str) + 1));

	hwlog_info("%s out, color_name %s!\n", __func__, color_name);
	return true;
}

static bool led_config_get_tp_module_name(char* info, char* module_name, uint8_t len)
{
	uint8_t module_id = 0;
	char* tmp_str = NULL;

	hwlog_info("%s enter!\n", __func__);
	if (NULL == info || NULL == module_name) {
		hwlog_err("%s null pointer error!\n", __func__);
		return false;
	}

	if (info[0] == 0) {
		hwlog_err("%s info is not ready!\n", __func__);
		return false;
	}

	//tp_project_info [6]~[8] is the module info
	module_id = LED_CONFIG_COEFF_10 * (info[6] - '0') + (info[7] - '0');

	switch (module_id) {
		case TP_MODULE_ID_NUM_00:
			tmp_str = "O-Film";
			break;
		case TP_MODULE_ID_NUM_01:
			tmp_str = "ECW";
			break;
		case TP_MODULE_ID_NUM_02:
			tmp_str = "Truly";
			break;
		case TP_MODULE_ID_NUM_03:
			tmp_str = "Mutto";
			break;
		case TP_MODULE_ID_NUM_04:
			tmp_str = "GIS";
			break;
		case TP_MODULE_ID_NUM_05:
			tmp_str = "JUNDA";
			break;
		case TP_MODULE_ID_NUM_06:
			tmp_str = "LENSONE";
			break;
		case TP_MODULE_ID_NUM_07:
			tmp_str = "YASSY";
			break;
		case TP_MODULE_ID_NUM_08:
			tmp_str = "JDI";
			break;
		case TP_MODULE_ID_NUM_09:
			tmp_str = "Samsung";
			break;
		case TP_MODULE_ID_NUM_10:
			tmp_str = "LG";
			break;
		case TP_MODULE_ID_NUM_11:
			tmp_str = "TIANMA";
			break;
		case TP_MODULE_ID_NUM_12:
			tmp_str = "CMI";
			break;
		case TP_MODULE_ID_NUM_13:
			tmp_str = "BOE";
			break;
		case TP_MODULE_ID_NUM_14:
			tmp_str = "CTC";
			break;
		case TP_MODULE_ID_NUM_15:
			tmp_str = "EDO";
			break;
		case TP_MODULE_ID_NUM_16:
			tmp_str = "Sharp";
			break;
		case TP_MODULE_ID_NUM_17:
			tmp_str = "Auo";
			break;
		case TP_MODULE_ID_NUM_18:
			tmp_str = "TopTouch";
			break;
		case TP_MODULE_ID_NUM_19:
			tmp_str = "BOE";
			break;
		case TP_MODULE_ID_NUM_20:
			tmp_str = "CTC";
			break;
		case TP_MODULE_ID_NUM_21:
			tmp_str = "BIEL";
			break;
		case TP_MODULE_ID_NUM_22:
			tmp_str = "KD";
			break;
		case TP_MODULE_ID_NUM_23:
			tmp_str = "EACH";
			break;
		case TP_MODULE_ID_NUM_24:
			tmp_str = "DJN";
			break;
		case TP_MODULE_ID_NUM_25:
			tmp_str = "TXD";
			break;
		case TP_MODULE_ID_NUM_26:
			tmp_str = "HLT";
			break;
		default:
			hwlog_err("%s module_id error!\n", __func__);
			return false;
	}

	if (NULL == tmp_str) {
		hwlog_err("%s tmp_str null error!\n", __func__);
		return false;
	}
	if (len <= (uint8_t)strlen(tmp_str)) {
		hwlog_err("%s module_name len is too small, tmp_str len %d!\n", __func__, (int)strlen(tmp_str));
		return false;
	}
	memcpy(module_name, tmp_str, ((int)strlen(tmp_str) + 1));

	hwlog_info("%s module_name %s!\n", __func__, module_name);
	return true;
}

static bool led_config_get_tp_ic_name(char* info, char* ic_name, uint8_t len)
{
	uint8_t ic_id = 0;
	char* tmp_str = NULL;

	hwlog_info("%s enter!\n", __func__);
	if (NULL == info || NULL == ic_name) {
		hwlog_err("%s null pointer error!\n", __func__);
		return false;
	}

	if (info[0] == 0) {
		hwlog_err("%s info is not ready!\n", __func__);
		return false;
	}

	//tp_project_info [4]~[5] is the ic info
	ic_id = LED_CONFIG_COEFF_10 * (info[4] - '0') + (info[5] - '0');

	switch (ic_id) {
		case TP_IC_ID_NUM_10:
		case TP_IC_ID_NUM_11:
		case TP_IC_ID_NUM_12:
		case TP_IC_ID_NUM_13:
		case TP_IC_ID_NUM_14:
		case TP_IC_ID_NUM_15:
		case TP_IC_ID_NUM_16:
		case TP_IC_ID_NUM_17:
		case TP_IC_ID_NUM_18:
		case TP_IC_ID_NUM_21:
		case TP_IC_ID_NUM_24:
		case TP_IC_ID_NUM_25:
		case TP_IC_ID_NUM_26:
		case TP_IC_ID_NUM_27:
		case TP_IC_ID_NUM_28:
		case TP_IC_ID_NUM_34:
		case TP_IC_ID_NUM_35:
			tmp_str = "synaptics";
			break;
		case TP_IC_ID_NUM_19:
			tmp_str = "atmel";
			break;
		case TP_IC_ID_NUM_20:
			tmp_str = "hideep";
			break;
		case TP_IC_ID_NUM_22:
		case TP_IC_ID_NUM_30:
		case TP_IC_ID_NUM_36:
		case TP_IC_ID_NUM_37:
		case TP_IC_ID_NUM_44:
			tmp_str = "focalTech";
			break;
		case TP_IC_ID_NUM_23:
		case TP_IC_ID_NUM_43:
			tmp_str = "ST";
			break;
		case TP_IC_ID_NUM_29:
			tmp_str = "ILITEK";
			break;
		case TP_IC_ID_NUM_31:
		case TP_IC_ID_NUM_39:
		case TP_IC_ID_NUM_51:
			tmp_str = "parada";
			break;
		case TP_IC_ID_NUM_32:
			tmp_str = "ROHM";
			break;
		case TP_IC_ID_NUM_33:
		case TP_IC_ID_NUM_38:
		case TP_IC_ID_NUM_40:
		case TP_IC_ID_NUM_42:
			tmp_str = "NOVATECH";
			break;
		case TP_IC_ID_NUM_41:
			tmp_str = "HIMAX";
			break;
		case TP_IC_ID_NUM_45:
			tmp_str = "goodix";
			break;
		default:
			hwlog_err("%s tp_ic_id error!\n", __func__);
			return false;
	}

	if (NULL == tmp_str) {
		hwlog_err("%s tmp_str null error!\n", __func__);
		return false;
	}
	if (len <= (uint8_t)strlen(tmp_str)) {
		hwlog_err("%s ic_name len is too small, tmp_str len %d!\n", __func__, (int)strlen(tmp_str));
		return false;
	}
	memcpy(ic_name, tmp_str, ((int)strlen(tmp_str) + 1));

	hwlog_info("%s ic_name %s!\n", __func__, ic_name);
	return true;
}

static bool led_config_get_tp_project_info(struct led_config_data* data,
		char* module_name, uint8_t module_len, char* ic_name, uint8_t ic_len)
{
	char tp_project_info[16] = {0}; // the max lenth is 11

	hwlog_info("%s enter!\n", __func__);
	if (NULL == data || NULL == module_name || NULL == ic_name) {
		hwlog_err("%s null pointer error!\n", __func__);
		return false;
	}

	if (tp_project_id_provider(tp_project_info, (uint8_t)sizeof(tp_project_info))) {
		hwlog_err("%s get tp_project_info fail!\n", __func__);
		return false;
	}
	hwlog_err("%s tp_module_info %s!\n", __func__, tp_project_info);

	if (data->tp_module_flag && !led_config_get_tp_module_name(tp_project_info, module_name, module_len)) {
		hwlog_err("%s get tp_module_info fail!\n", __func__);
		return false;
	}

	if (data->tp_ic_flag && !led_config_get_tp_ic_name(tp_project_info, ic_name, ic_len)) {
		hwlog_err("%s get tp_ic_info fail!\n", __func__);
		return false;
	}

	return true;
}

static int led_config_parse_current_setting(struct led_config_data *data)
{
	uint8_t flag = 0;
	uint8_t tmp = 0;
	char tp_color_name[LED_CONFIG_BUFF_LEN] = {0};
	char tp_module_name[LED_CONFIG_BUFF_LEN] = {0};
	char tp_ic_name[LED_CONFIG_BUFF_LEN] = {0};

	if (NULL == data) {
		hwlog_err("%s null pointer error!\n", __func__);
		return -EPERM;
	}
	hwlog_info("%s enter!\n", __func__);

	if (data->tp_color_flag && (data->tp_module_flag || data->tp_ic_flag)) {
		if (!(led_config_get_tp_color_name(data, tp_color_name, sizeof(tp_color_name)) &&
			led_config_get_tp_project_info(data, tp_module_name, sizeof(tp_module_name), tp_ic_name, sizeof(tp_ic_name)))) {
			hwlog_err("%s get tp_name fail!\n", __func__);
			return -EPERM;
		}
	} else if (data->tp_module_flag || data->tp_ic_flag) {
		if (!led_config_get_tp_project_info(data, tp_module_name, sizeof(tp_module_name), tp_ic_name, sizeof(tp_ic_name))) {
			hwlog_err("%s get tp_module_name fail!\n", __func__);
			return -EPERM;
		}
	} else if (data->tp_color_flag) {
		if (!led_config_get_tp_color_name(data, tp_color_name, sizeof(tp_color_name))) {
			hwlog_err("%s get tp_color_name fail!\n", __func__);
			return -EPERM;
		}
	} else {
		hwlog_err("%s error return!\n", __func__);
		return -EPERM;
	}

	hwlog_info("%s tp_modele_name %s, tp_ic_name %s, tp_color_name %s!\n",
			__func__, tp_module_name, tp_ic_name, tp_color_name);

	flag = led_config_parse_single_current_conf(data, tp_module_name, tp_ic_name,
			tp_color_name, "led_red_maxdr_iset", &data->curr.red_curr);
	tmp = led_config_parse_single_current_conf(data, tp_module_name, tp_ic_name,
			tp_color_name, "led_green_maxdr_iset", &data->curr.green_curr);
	flag = flag && tmp;
	tmp = led_config_parse_single_current_conf(data, tp_module_name, tp_ic_name,
			tp_color_name, "led_blue_maxdr_iset", &data->curr.blue_curr);
	flag = flag && tmp;

	data->curr.got_flag = flag;

	hwlog_info("%s out!\n", __func__);
	return flag ? 0 : -EPERM;
}

static void led_config_work_func(struct work_struct *work)
{
	struct led_config_data *data = NULL;

	if (NULL == work) {
		hwlog_err("%s null pointer error!\n", __func__);
		return;
	}
	data = container_of(work, struct led_config_data, work.work);

	if (led_config_parse_current_setting(data)) {
		hwlog_info("%s get current fail!\n", __func__);
	}
	g_data->work_flag = 0;

	return;
}

static const struct of_device_id led_config_match_table[] = {
	{.compatible = "huawei,led_config",},
	{},
};
MODULE_DEVICE_TABLE(of, led_config_match_table);

static int led_config_probe(struct platform_device *pdev)
{
	uint8_t flag = 0;
	uint32_t tmp_u32 = 0;
	struct led_config_data *data = NULL;

	if (NULL == pdev) {
		hwlog_err("%s null pointer error!\n", __func__);
		return -ENODEV;
	}

	data = devm_kzalloc(&pdev->dev, sizeof(struct led_config_data), GFP_KERNEL);
	if (!data) {
		dev_err(&pdev->dev, "unable to allocate memory\n");
		return -ENOMEM;
	}

	data->node = pdev->dev.of_node;
	if (!data->node) {
		hwlog_err("%s failed to find dts node led_alwayson\n", __func__);
		kfree(data);
		data = NULL;
		return -ENODEV;;
	}

	GET_U8_FROM_NODE(data->node, "led_current_using_tp_color_setting", tmp_u32, data->tp_color_flag, flag);
	GET_U8_FROM_NODE(data->node, "led_current_using_tp_module_setting", tmp_u32, data->tp_module_flag, flag);
	GET_U8_FROM_NODE(data->node, "led_current_using_tp_ic_setting", tmp_u32, data->tp_ic_flag, flag);

	INIT_DELAYED_WORK(&data->work, led_config_work_func);
	if (data->tp_color_flag || data->tp_module_flag || data->tp_ic_flag) {
		schedule_delayed_work(&data->work, 3 * HZ); // delay 3s
		data->work_flag = 1;
	}

	data->retry_times = 10; //retry 10 times
	g_data = data;
	platform_set_drvdata(pdev, data);

	hwlog_info("%s succ.\n", __func__);
	return 0;
}

static int led_config_remove(struct platform_device *pdev)
{
	struct led_config_data *data = NULL;

	if (NULL == pdev) {
		hwlog_err("%s null pointer error!\n", __func__);
		return -ENODEV;
	}
	data = platform_get_drvdata(pdev);

	kfree(data);
	data = NULL;
	g_data = NULL;
	platform_set_drvdata(pdev, NULL);

	return 0;
}

struct platform_driver led_config_driver = {
	.probe = led_config_probe,
	.remove = led_config_remove,
	.driver = {
		.name = LED_CONFIG,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(led_config_match_table),
	},
};

static int __init led_config_init(void)
{
	hwlog_info("init!\n");
	return platform_driver_register(&led_config_driver);
}

static void __exit led_config_exit(void)
{
	platform_driver_unregister(&led_config_driver);
}

module_init(led_config_init);
module_exit(led_config_exit);

MODULE_AUTHOR("HUAWEI");
MODULE_DESCRIPTION("Led config driver");
MODULE_LICENSE("GPL");
