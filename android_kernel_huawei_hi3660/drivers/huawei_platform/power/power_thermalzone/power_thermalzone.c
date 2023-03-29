#include <linux/module.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/thermal.h>
#include <linux/hisi/hisi_adc.h>

#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/power_thermalzone.h>

#define HWLOG_TAG power_thermalzone
HWLOG_REGIST();

static struct power_thermalzone_info *g_info = NULL;

/* ntc code is Thermitor 07050125 (resistor to temperature) */
static int NTC_07050125_R_T[][2] =
{
	{2561, 125},   {2634, 124},   {2709, 123},   {2787, 122},   {2868, 121},   {2952, 120},
	{3038, 119},   {3128, 118},   {3220, 117},   {3316, 116},   {3415, 115},   {3518, 114},
	{3624, 113},   {3734, 112},   {3848, 111},   {3966, 110},   {3966, 109},   {4215, 108},
	{4346, 107},   {4482, 106},   {4623, 105},   {4769, 104},   {4921, 103},   {5078, 102},
	{5241, 101},   {5410, 100},   {5586, 99},    {5768, 98},    {5957, 97},    {6153, 96},
	{6357, 95},    {6568, 94},    {6788, 93},    {7016, 92},    {7254, 91},    {7500, 90},
	{7756, 89},    {8022, 88},    {8299, 87},    {8587, 86},    {8887, 85},    {9198, 84},
	{9522, 83},    {9859, 82},    {10210, 81},   {10580, 80},   {10960, 79},   {11350, 78},
	{11760, 77},   {12190, 76},   {12640, 75},   {13110, 74},   {13600, 73},   {14100, 72},
	{14630, 71},   {15180, 70},   {15760, 69},   {16360, 68},   {16990, 67},   {17640, 66},
	{18320, 65},   {19040, 64},   {19780, 63},   {20560, 62},   {21370, 61},   {22220, 60},
	{23110, 59},   {24040, 58},   {25010, 57},   {26030, 56},   {27090, 55},   {28200, 54},
	{29360, 53},   {30580, 52},   {31860, 51},   {33190, 50},   {34600, 49},   {36060, 48},
	{37600, 47},   {39210, 46},   {40900, 45},   {42670, 44},   {44530, 43},   {46490, 42},
	{48530, 41},   {50680, 40},   {52940, 39},   {55310, 38},   {57810, 37},   {60420, 36},
	{63180, 35},   {66070, 34},   {69120, 33},   {72320, 32},   {75690, 31},   {79230, 30},
	{82970, 29},   {86900, 28},   {91040, 27},   {95400, 26},   {100000, 25},  {104800, 24},
	{110000, 23},  {115400, 22},  {121000, 21},  {127000, 20},  {133400, 19},  {140100, 18},
	{147200, 17},  {154600, 16},  {162500, 15},  {170900, 14},  {179700, 13},  {189000, 12},
	{198900, 11},  {209400, 10},  {220500, 9},   {232200, 8},   {244700, 7},   {257800, 6},
	{271800, 5},   {286700, 4},   {302400, 3},   {319100, 2},   {336800, 1},   {355600, 0},
	{375600, -1},  {396900, -2},  {419500, -3},  {443500, -4},  {469100, -5},  {496300, -6},
	{525300, -7},  {556200, -8},  {589000, -9},  {624100, -10}, {661500, -11}, {701300, -12},
	{743900, -13}, {789300, -14}, {837800, -15}, {889600, -16}, {945000, -17}, {1004000, -18},
	{1068000, -19},{1135000, -20},{1208000, -21},{1286000, -22},{1369000, -23},{1458000, -24},
	{1554000, -25},{1656000, -26},{1767000, -27},{1885000, -28},{2012000, -29},{2149000, -30},
	{2296000, -31},{2454000, -32},{2624000, -33},{2807000, -34},{3005000, -35},{3218000, -36},
	{3447000, -37},{3695000, -38},{3962000, -39},{4251000, -40},
};

/* ntc code is Thermitor 07050124 10K (voltage to temperature) */
static int NTC_07050124_PULLUP_10K_V_T[][2] =
{
	{208, 125},  {218, 124},  {217, 123},  {222, 122},  {227, 121},  {232, 120},
	{237, 119},  {242, 118},  {247, 117},  {253, 116},  {259, 115},  {264, 114},
	{270, 113},  {277, 112},  {283, 111},  {289, 110},  {296, 109},  {303, 108},
	{310, 107},  {317, 106},  {324, 105},  {332, 104},  {340, 103},  {347, 102},
	{356, 101},  {364, 100},  {373, 99},   {381, 98},   {391, 97},   {400, 96},
	{409, 95},   {419, 94},   {429, 93},   {439, 92},   {450, 91},   {461, 90},
	{472, 89},   {483, 88},   {495, 87},   {507, 86},   {519, 85},   {532, 84},
	{545, 83},   {558, 82},   {572, 81},   {586, 80},   {600, 79},   {614, 78},
	{630, 77},   {645, 76},   {661, 75},   {677, 74},   {694, 73},   {711, 72},
	{728, 71},   {746, 70},   {765, 69},   {783, 68},   {802, 67},   {822, 66},
	{842, 65},   {863, 64},   {884, 63},   {905, 62},   {927, 61},   {950, 60},
	{973, 59},   {996, 58},   {1020, 57},  {1045, 56},  {1070, 55},  {1095, 54},
	{1122, 53},  {1148, 52},  {1175, 51},  {1203, 50},  {1231, 49},  {1260, 48},
	{1289, 47},  {1319, 46},  {1349, 45},  {1380, 44},  {1411, 43},  {1443, 42},
	{1475, 41},  {1508, 40},  {1541, 39},  {1575, 38},  {1609, 37},  {1643, 36},
	{1678, 35},  {1714, 34},  {1750, 33},  {1786, 32},  {1822, 31},  {1859, 30},
	{1896, 29},  {1934, 28},  {1972, 27},  {2010, 26},  {2048, 25},  {2086, 24},
	{2125, 23},  {2164, 22},  {2203, 21},  {2242, 20},  {2281, 19},  {2321, 18},
	{2360, 17},  {2399, 16},  {2438, 15},  {2476, 14},  {2515, 13},  {2554, 12},
	{2593, 11},  {2631, 10},  {2669, 9},   {2707, 8},   {2745, 7},   {2782, 6},
	{2819, 5},   {2856, 4},   {2892, 3},   {2927, 2},   {2963, 1},   {2997, 0},
	{3032, -1},  {3066, -2},  {3099, -3},  {3131, -4},  {3164, -5},  {3195, -6},
	{3226, -7},  {3256, -8},  {3286, -9},  {3315, -10}, {3343, -11}, {3371, -12},
	{3398, -13}, {3425, -14}, {3451, -15}, {3476, -16}, {3500, -17}, {3524, -18},
	{3547, -19}, {3569, -20}, {3591, -21}, {3612, -22}, {3633, -23}, {3653, -24},
	{3672, -25}, {3690, -26}, {3708, -27}, {3726, -28}, {3742, -29}, {3758, -30},
	{3774, -31}, {3789, -32}, {3803, -33}, {3817, -34}, {3831, -35}, {3843, -36},
	{3856, -37}, {3867, -38}, {3879, -39}, {3890, -40},
};

/* ntc code is Thermitor 07050124 12K (voltage to temperature) */
static int NTC_07050124_PULLUP_12K_V_T[][2] =
{
	{175, 125},  {178, 124},  {182, 123},  {186, 122},  {191, 121},  {195, 120},
	{199, 119},  {204, 118},  {208, 117},  {213, 116},  {218, 115},  {223, 114},
	{228, 113},  {233, 112},  {238, 111},  {244, 110},  {250, 109},  {255, 108},
	{261, 107},  {268, 106},  {274, 105},  {280, 104},  {287, 103},  {294, 102},
	{301, 101},  {308, 100},  {315, 99},   {323, 98},   {331, 97},   {338, 96},
	{347, 95},   {355, 94},   {364, 93},   {373, 92},   {382, 91},   {391, 90},
	{401, 89},   {411, 88},   {421, 87},   {431, 86},   {442, 85},   {453, 84},
	{464, 83},   {486, 82},   {488, 81},   {500, 80},   {512, 79},   {525, 78},
	{539, 77},   {552, 76},   {566, 75},   {580, 74},   {595, 73},   {610, 72},
	{626, 71},   {641, 70},   {658, 69},   {674, 68},   {691, 67},   {709, 66},
	{727, 65},   {745, 64},   {764, 63},   {783, 62},   {803, 61},   {823, 60},
	{844, 59},   {866, 58},   {887, 57},   {909, 56},   {932, 55},   {955, 54},
	{979, 53},   {1004, 52},  {1029, 51},  {1054, 50},  {1080, 49},  {1107, 48},
	{1134, 47},  {1161, 46},  {1189, 45},  {1218, 44},  {1247, 43},  {1277, 42},
	{1308, 41},  {1339, 40},  {1370, 39},  {1402, 38},  {1435, 37},  {1468, 36},
	{1501, 35},  {1535, 34},  {1570, 33},  {1605, 32},  {1640, 31},  {1676, 30},
	{1713, 29},  {1749, 28},  {1786, 27},  {1824, 26},  {1862, 25},  {1900, 24},
	{1938, 23},  {1977, 22},  {2017, 21},  {2056, 20},  {2096, 19},  {2135, 18},
	{2175, 17},  {2215, 16},  {2255, 15},  {2295, 14},  {2335, 13},  {2376, 12},
	{2416, 11},  {2455, 10},  {2495, 9},   {2535, 8},   {2575, 7},   {2614, 6},
	{2653, 5},   {2692, 4},   {2731, 3},   {2769, 2},   {2807, 1},   {2845, 0},
	{2882, -1},  {2919, -2},  {2955, -3},  {2990, -4},  {3026, -5},  {3060, -6},
	{3095, -7},  {3128, -8},  {3161, -9},  {3193, -10}, {3225, -11}, {3256, -12},
	{3286, -13}, {3316, -14}, {3345, -15}, {3373, -16}, {3401, -17}, {3428, -18},
	{3454, -19}, {3480, -20}, {3505, -21}, {3529, -22}, {3552, -23}, {3575, -24},
	{3597, -25}, {3619, -26}, {3639, -27}, {3659, -28}, {3679, -29}, {3697, -30},
	{3716, -31}, {3733, -32}, {3750, -33}, {3766, -34}, {3782, -35}, {3796, -36},
	{3811, -37}, {3825, -38}, {3838, -39}, {3851, -40},
};

static int data_to_temp_linear(long data, int index_min, int index_max, int *trans_list)
{
	int prorate = 0;
	int resistance_max = 0;
	int resistance_min = 0;
	int temper_top = 0;
	int temper_bottom = 0;
	int itemper = 0;

	resistance_min = *(trans_list + 2 * index_min + 0);
	resistance_max = *(trans_list + 2 * index_max + 0);
	temper_bottom = *(trans_list + 2 * index_min + 1);
	temper_top = *(trans_list + 2 * index_max + 1);

	if ((resistance_max - resistance_min) == 0)
		return POWER_THERMALZONE_DEFAULT_TEMP;

	prorate =  ((resistance_max - data) * PRORATE_OF_INIT) / (resistance_max - resistance_min);
	itemper = (temper_bottom - temper_top) * prorate + temper_top * PRORATE_OF_INIT;

	return itemper;
}


static int get_data_range_using_dichotomy(int *temp, long data, int trans_len, int *trans_list)
{

	int iLow = 0;
	int iMid = 0;

	if (data < *(trans_list + 2 * 0 + 0)) {
		*temp = (*(trans_list + 2 * 0 + 1)) * PRORATE_OF_INIT;
		return 0;
	} else if (data > *(trans_list + 2 * (trans_len - 1) + 0)) {
		*temp = (*(trans_list + 2 * (trans_len - 1) + 1)) * PRORATE_OF_INIT;
		return 0;
	}

	while (iLow <= trans_len) {
		iMid = (iLow + trans_len) / 2;
		if (*(trans_list + 2 * iMid + 0) > data) {
			if (*(trans_list + 2 * (iMid - 1) + 0) < data) {
				*temp = data_to_temp_linear(data,
						(iMid - 1), iMid,trans_list);
				return 0;
			}
			trans_len = iMid - 1;
		} else if (*(trans_list + 2 * iMid + 0) < data) {
			if (*(trans_list + 2 * (iMid + 1) + 0) > data) {
				*temp = data_to_temp_linear(data,
					iMid, (iMid + 1),trans_list);
			return 0;
			}
			iLow = iMid + 1;
		} else {
			*temp = (*(trans_list + 2 * iMid + 1)) * PRORATE_OF_INIT;
			break;
		}
	}

	return 0;
}

static int power_thermalzone_get_temp(struct thermal_zone_device *thermal, int *temp)
{
	struct power_thermalzone_sensor *sensor = NULL;
	int ret = 0;
	long data = 0;
	int *trans_list = NULL;
	int trans_len = 0;

	if (thermal == NULL) {
		hwlog_err("error: thermal is null!\n");
		return -EINVAL;
	}
	sensor = thermal->devdata;

	if (sensor->get_raw_data == NULL) {
		hwlog_err("error: get_raw_data is null[sensor:%s ops_name:%s]!\n", sensor->sensor_name, sensor->ops_name);
		goto err_temp;
	}

	ret = sensor->get_raw_data(sensor->adc_channel, &data);
	if (ret) {
		hwlog_err("error: temp get fail[sensor:%s ops_name:%s]!\n", sensor->sensor_name, sensor->ops_name);
		goto err_temp;
	}

	switch(sensor->ntc_index){
		case NTC_07050125:
			trans_len= sizeof(NTC_07050125_R_T) / sizeof(NTC_07050125_R_T[0][0]) / 2;
			trans_list = (int *)NTC_07050125_R_T;
		break;

		case NTC_07050124_PULLUP_10K:
			trans_len= sizeof(NTC_07050124_PULLUP_10K_V_T) / sizeof(NTC_07050124_PULLUP_10K_V_T[0][0]) / 2;
			trans_list = (int *)NTC_07050124_PULLUP_10K_V_T;
		break;

		case NTC_07050124_PULLUP_12K:
			trans_len= sizeof(NTC_07050124_PULLUP_12K_V_T) / sizeof(NTC_07050124_PULLUP_12K_V_T[0][0]) / 2;
			trans_list = (int *)NTC_07050124_PULLUP_12K_V_T;
		break;

		default:
			hwlog_err("error: unknow ntc:%d!\n", sensor->ntc_index);
			goto err_temp;
		break;
	}

	ret = get_data_range_using_dichotomy(temp, data, trans_len, trans_list);
	if (ret) {
		hwlog_err("error: temp get fail!\n");
		goto err_temp;
	}

	if (*temp < POWER_THERMALZONE_MIN_TEMP || *temp > POWER_THERMALZONE_MAX_TEMP) {
		hwlog_err("error: invalid temp [%d]!\n", *temp);
		goto err_temp;
	}

	return 0;

err_temp:
	*temp = POWER_THERMALZONE_DEFAULT_TEMP;
	return 0;
}

struct thermal_zone_device_ops power_thermal_zone_ops = {
	.get_temp = power_thermalzone_get_temp,
};

static int power_thermalzone_get_adc_sample(int adc_channel, long *data)
{
	int i = 0;
	int adc_sample = -1;

	for (i = 0; i < POWER_THERMALZONE_ADC_RETRY_TIMES; i++) {
		adc_sample = hisi_adc_get_adc(adc_channel);
		if (adc_sample < 0) {
			hwlog_err("error: hisi adc read fail!\n");
		}
		else {
			break;
		}
	}
	*data = adc_sample;

	return 0;
}

int power_thermalzone_ops_register(struct power_thermalzone_ops *ops, char *ops_name)
{
	int i = 0;

	hwlog_info("ops_name=%s\n", ops_name);

	if (g_info == NULL || ops == NULL) {
		hwlog_err("error: g_info or ops is null!\n");
		return -EPERM;
	}

	for (i = 0; i < g_info->total_sensor; i++) {
		if (!strncmp(ops_name, g_info->sensor[i].ops_name, strlen(g_info->sensor[i].ops_name))) {
			g_info->sensor[i].get_raw_data = ops->get_raw_data;
			break;
		}
	}

	if (i >= g_info->total_sensor) {
		hwlog_err("error: %s ops register fail!\n", ops_name);
		return -EPERM;
	}

	return 0;
}

static int power_thermalzone_parse_dts(struct device_node *dev_node, struct power_thermalzone_info *info)
{
	int i = 0;
	int ret = 0;
	struct device_node *child_node = NULL;
	const char *sensor_name = NULL;
	const char *ops_name = NULL;

	info->total_sensor = of_get_child_count(dev_node);
	if (info->total_sensor <= 0) {
		hwlog_err("error: total_sensor dts read failed!\n");
		return -EINVAL;
	}

	for_each_child_of_node(dev_node, child_node) {
		ret = of_property_read_string(child_node, "sensor_name", &sensor_name);
		if (ret) {
			hwlog_err("error: sensor_name dts read failed!\n");
			return -EINVAL;
		}

		ret = of_property_read_string(child_node, "ops_name", &ops_name);
		if (ret) {
			hwlog_err("error: ops_name dts read failed!\n");
			return -EINVAL;
		}

		ret = of_property_read_u32(child_node, "adc_channel", &info->sensor[i].adc_channel);
		if (ret) {
			hwlog_err("error: adc_channel dts read failed!\n");
			return -EINVAL;
		}

		ret = of_property_read_u32(child_node,"ntc_index",&info->sensor[i].ntc_index);
		if (ret) {
			hwlog_err("error: ntc_index dts read failed!\n");
			return -EINVAL;
		}

		if (!strncmp(ops_name, POWER_THERMALZONE_DEFAULT_OPS, strlen(POWER_THERMALZONE_DEFAULT_OPS))) {
			info->sensor[i].get_raw_data = power_thermalzone_get_adc_sample;
		}

		strncpy(info->sensor[i].sensor_name, sensor_name, (POWER_THERMALZONE_STR_MAX_LEN - 1));
		strncpy(info->sensor[i].ops_name, ops_name, (POWER_THERMALZONE_STR_MAX_LEN - 1));

		i++;
	}

	for (i = 0; i < info->total_sensor; i++) {
		hwlog_info("para[%d]: sensor_name:%s, ops_name:%s, adc_channel:%d, ntc_index:%d\n", \
			i, info->sensor[i].sensor_name, info->sensor[i].ops_name, \
			info->sensor[i].adc_channel, info->sensor[i].ntc_index);
	}

	return 0;
}

static int power_thermalzone_probe(struct platform_device *pdev)
{
	struct power_thermalzone_info *info = NULL;
	struct device_node *np = NULL;
	struct device_node *dev_node = NULL;
	int ret = -1;
	int i = 0;

	hwlog_info("probe begin\n");

	info = devm_kzalloc(&pdev->dev, sizeof(struct power_thermalzone_info), GFP_KERNEL);
	if (!info) {
		hwlog_err("error: kzalloc failed!\n");
		return -ENOMEM;
	}
	g_info = info;

	info->pdev = pdev;
	np = pdev->dev.of_node;
	if (NULL == info->pdev || NULL == np) {
		hwlog_err("error: device_node is null!\n");
		goto free_mem;
	}

	dev_node = of_find_node_by_name(np, "sensors");
	if (!dev_node) {
		hwlog_err("error: sensors node dts read failed!\n");
		goto free_mem;
	}

	ret = power_thermalzone_parse_dts(dev_node, info);
	if (ret < 0) {
		goto err_parse_dts;
	}

	for (i = 0; i < info->total_sensor; i++) {
		info->sensor[i].tz_dev = thermal_zone_device_register(info->sensor[i].sensor_name,
				0, 0, &info->sensor[i], &power_thermal_zone_ops, NULL, 0, 0);

		if (IS_ERR(info->sensor[i].tz_dev)) {
			hwlog_err("error: thermalzone register fail!\n");
			ret = -ENODEV;
			goto register_fail;
		}
	}

	platform_set_drvdata(pdev, info);

	hwlog_info("probe end\n");
	return 0;

register_fail:
	for (i = 0; i < info->total_sensor; i++) {
		thermal_zone_device_unregister(info->sensor[i].tz_dev);
	}

err_parse_dts:
	of_node_put(dev_node);

free_mem:
	devm_kfree(&pdev->dev, info);
	g_info = NULL;

	return ret;
}

static int power_thermalzone_remove(struct platform_device *pdev)
{
	int i = 0;
	struct power_thermalzone_info *info = platform_get_drvdata(pdev);

	hwlog_info("remove begin\n");

	for (i = 0; i < info->total_sensor; i++) {
		thermal_zone_device_unregister(info->sensor[i].tz_dev);
	}

	platform_set_drvdata(pdev, NULL);
	devm_kfree(&pdev->dev, info);
	g_info = NULL;

	hwlog_info("remove end\n");

	return 0;
}

static struct of_device_id power_thermalzone_match_table[] = {
	{
		.compatible = "huawei,power_thermalzone",
		.data = NULL,
	},
	{ },
};

static struct platform_driver power_thermalzone_driver = {
	.probe = power_thermalzone_probe,
	.remove = power_thermalzone_remove,
	.driver = {
		.name = "huawei,power_thermalzone",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(power_thermalzone_match_table),
	},
};

static int __init power_thermalzone_init(void)
{
	return platform_driver_register(&power_thermalzone_driver);
}

static void __exit power_thermalzone_exit(void)
{
	platform_driver_unregister(&power_thermalzone_driver);
}

fs_initcall_sync(power_thermalzone_init);
module_exit(power_thermalzone_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("power thermal_zone module driver");
MODULE_AUTHOR("HUAWEI Inc");

