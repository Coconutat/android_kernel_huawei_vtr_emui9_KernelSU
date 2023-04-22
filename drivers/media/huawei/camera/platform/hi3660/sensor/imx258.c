 


#include <linux/module.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/rpmsg.h>

#include "hwsensor.h"
#include "sensor_commom.h"
#include "hw_csi.h"
#include "../pmic/hw_pmic.h"
//lint -save -e650 -e31

#define I2S(i) container_of(i, sensor_t, intf)
#define POWER_SETTING_DELAY_0 0
#define POWER_SETTING_DELAY_1 1

extern struct hw_csi_pad hw_csi_pad;
static hwsensor_vtbl_t s_imx258_vtbl;
static hwsensor_intf_t *s_intf = NULL;

struct sensor_power_setting hw_imx258_power_setting[] = {

	//set camera reset gpio 146 to low
/*	{
		.seq_type = SENSOR_SUSPEND,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},*/

	//enable gpio51 output iovdd 1.8v
	{
		.seq_type = SENSOR_LDO_EN,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

   {
        .seq_type = SENSOR_VCM_AVDD,
        .data = (void*)"cameravcm-vcc",
        .config_val = LDO_VOLTAGE_V2P85V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_SETTING_DELAY_1,
    },

	//enable gpio34 output ois af vdd 2.95v
	{
		.seq_type = SENSOR_VCM_PWDN,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

	//MCAM AVDD VOUT19 2.8V
	{
		.seq_type = SENSOR_AVDD2,
		.data = (void*)"main-sensor-avdd",
		.config_val = LDO_VOLTAGE_V2P8V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

	//MCAM0 DVDD VLDO1.2V
	{
		.seq_type = SENSOR_DVDD2,
		.data = (void*)"main-sensor-dvdd",
		.config_val = LDO_VOLTAGE_1P2V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

	//MCAM ISP_CLK0 16M
	{
		.seq_type = SENSOR_MCLK,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_1,
	},

	//MCAM Reset GPIO_052
	{
		.seq_type = SENSOR_RST,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_1,
	},
};

struct sensor_power_setting hw_imx258_power_setting_v4[] = {

	//SCAM1 Reset
	{
		.seq_type = SENSOR_SUSPEND,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

	//SCAM DVDD 1.2V
	{
		.seq_type = SENSOR_DVDD,
		.config_val = LDO_VOLTAGE_1P2V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_1,
	},

	//enable gpio51 output iovdd 1.8v
	{
		.seq_type = SENSOR_LDO_EN,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

	{
		.seq_type = SENSOR_VCM_AVDD,
		.data = (void*)"cameravcm-vcc",
		.config_val = LDO_VOLTAGE_V2P85V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_1,
	},

	//enable gpio34 output ois af vdd 2.95v
	{
		.seq_type = SENSOR_VCM_PWDN,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

	//MCAM AVDD VOUT19 2.8V
	{
		.seq_type = SENSOR_AVDD2,
		.data = (void*)"main-sensor-avdd",
		.config_val = LDO_VOLTAGE_V2P8V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

	//MCAM0 DVDD VLDO1.2V
	{
		.seq_type = SENSOR_DVDD2,
		.data = (void*)"main-sensor-dvdd",
		.config_val = LDO_VOLTAGE_1P2V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},

	//MCAM ISP_CLK0 16M
	{
		.seq_type = SENSOR_MCLK,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_1,
	},

	//MCAM Reset GPIO_052
	{
		.seq_type = SENSOR_RST,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_1,
	},
};

static sensor_t s_imx258 =
{
    .intf = { .vtbl = &s_imx258_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(hw_imx258_power_setting),
            .power_setting = hw_imx258_power_setting,
     },
};

static sensor_t s_imx258_v4 =
{
    .intf = { .vtbl = &s_imx258_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(hw_imx258_power_setting_v4),
            .power_setting = hw_imx258_power_setting_v4,
     },
};


static const struct of_device_id
s_imx258_dt_match[] =
{
    {
        .compatible = "huawei,imx258",
        .data = &s_imx258.intf,
    },
    {
        .compatible = "huawei,imx258_v4",
        .data = &s_imx258_v4.intf,
    },
    {
    },
};

MODULE_DEVICE_TABLE(of, s_imx258_dt_match);

static struct platform_driver
s_imx258_driver =
{
	.driver =
    {
		.name = "huawei,imx258",
		.owner = THIS_MODULE,
		.of_match_table = s_imx258_dt_match,
	},
};

char const*
imx258_get_name(
        hwsensor_intf_t* si)
{
    sensor_t* sensor = I2S(si);
    return sensor->board_info->name;
}
static int
imx258_power_up(
        hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;
    sensor = I2S(si);
   cam_info("enter %s. index = %d name = %s", __func__, sensor->board_info->sensor_index, sensor->board_info->name);
   if (hw_is_fpga_board()) {
		ret = do_sensor_power_on(sensor->board_info->sensor_index, sensor->board_info->name);
	} else {
		ret = hw_sensor_power_up(sensor);
	}
    if (0 == ret )
    {
        cam_info("%s. power up sensor success.", __func__);
    }
    else
    {
        cam_err("%s. power up sensor fail.", __func__);
    }
    return ret;
}

static int
imx258_power_down(
        hwsensor_intf_t* si)
{
	int ret = 0;
	sensor_t* sensor = NULL;
	sensor = I2S(si);
	cam_info("enter %s. index = %d name = %s", __func__, sensor->board_info->sensor_index, sensor->board_info->name);
	if (hw_is_fpga_board()) {
		ret = do_sensor_power_off(sensor->board_info->sensor_index, sensor->board_info->name);
	} else {
		ret = hw_sensor_power_down(sensor);
	}
    if (0 == ret )
    {
        cam_info("%s. power down sensor success.", __func__);
    }
    else
    {
        cam_err("%s. power down sensor fail.", __func__);
    }

	return ret;
}

static int imx258_csi_enable(hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;

    sensor = I2S(si);

    ret = hw_csi_pad.hw_csi_enable((csi_index_t)0, sensor->board_info->csi_lane, sensor->board_info->csi_mipi_clk);
    if(ret)
    {
        cam_err("failed to csi enable index 0 ");
        return ret;
    }

    return 0;
}

int imx258_csi_disable(hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;

    sensor = I2S(si);

    ret = hw_csi_pad.hw_csi_disable((csi_index_t)0);
    if(ret)
    {
        cam_err("failed to csi disable index 0 ");
        return ret;
    }

    return 0;
}

static int
imx258_match_id(
        hwsensor_intf_t* si, void * data)
{
    sensor_t* sensor = NULL;
    struct sensor_cfg_data *cdata = NULL;

    cam_info("%s enter.", __func__);

    if ((NULL == si)||(NULL == data)) {
        cam_err("%s. si is NULL.", __func__);
        return -EINVAL;
    }

    sensor = I2S(si);
    if ((NULL == sensor->board_info) || (NULL == sensor->board_info->name)){
        cam_err("%s. sensor->board_info or sensor->board_info->name is NULL .", __func__);
        return -EINVAL;
    }
    cdata  = (struct sensor_cfg_data *)data;
    cdata->data = sensor->board_info->sensor_index;
    cam_info("%s name:%s", __func__, sensor->board_info->name);

    return 0;
}

static int
imx258_config(
        hwsensor_intf_t* si,
        void  *argp)
{
	struct sensor_cfg_data *data;

	int ret =0;
	static bool imx258_power_on = false;

    if (NULL == si || NULL == argp){
        cam_err("%s si or argp is null.\n", __func__);
        return -1;
    }

	data = (struct sensor_cfg_data *)argp;
	cam_debug("imx258 cfgtype = %d",data->cfgtype);
	switch(data->cfgtype){
		case SEN_CONFIG_POWER_ON:
			if (!imx258_power_on) {
				ret = si->vtbl->power_up(si);
				imx258_power_on = true;
			}
			break;
		case SEN_CONFIG_POWER_OFF:
			if (imx258_power_on) {
				ret = si->vtbl->power_down(si);
				imx258_power_on = false;
			}
			break;
		case SEN_CONFIG_WRITE_REG:
			break;
		case SEN_CONFIG_READ_REG:
			break;
		case SEN_CONFIG_WRITE_REG_SETTINGS:
			break;
		case SEN_CONFIG_READ_REG_SETTINGS:
			break;
		case SEN_CONFIG_ENABLE_CSI:
			break;
		case SEN_CONFIG_DISABLE_CSI:
			break;
		/* case SEN_CONFIG_MATCH_ID_EXT: */
			/* break; */
		case SEN_CONFIG_MATCH_ID:
			ret = si->vtbl->match_id(si,argp);
			break;
		default:
            cam_err("%s cfgtype(%d) is error", __func__, data->cfgtype);
			break;
	}
	cam_debug("%s exit",__func__);
	return ret;
}

static hwsensor_vtbl_t
s_imx258_vtbl =
{
	.get_name = imx258_get_name,
	.config = imx258_config,
	.power_up = imx258_power_up,
	.power_down = imx258_power_down,
	.match_id = imx258_match_id,
	.csi_enable = imx258_csi_enable,
	.csi_disable = imx258_csi_disable,
};
static int32_t
imx258_platform_probe(
        struct platform_device* pdev)
{
	int rc = 0;
    const struct of_device_id *id = NULL;
    hwsensor_intf_t *intf = NULL;
    sensor_t *sensor = NULL;
    struct device_node *np = NULL;
    cam_notice("enter %s",__func__);

    if (NULL == pdev) {
        cam_err("%s pdev is NULL", __func__);
        return -EINVAL;
    }

    np = pdev->dev.of_node;
    if (NULL == np) {
        cam_err("%s of_node is NULL", __func__);
        return -ENODEV;
    }

    id = of_match_node(s_imx258_dt_match, np);
    if (!id) {
        cam_err("%s none id matched", __func__);
        return -ENODEV;
    }

    intf = (hwsensor_intf_t*)id->data;
    if (NULL == intf) {
        cam_err("%s intf is NULL", __func__);
        return -EINVAL;
    }
    sensor = I2S(intf);

    rc = hw_sensor_get_dt_data(pdev, sensor);
    if (rc < 0) {
        cam_err("%s no dt data rc %d", __func__, rc);
        return -ENODEV;
    }
    sensor->dev = &pdev->dev;

    rc = hwsensor_register(pdev, intf);
    if (rc < 0) {
        cam_err("%s hwsensor_register failed rc %d\n", __func__, rc);
        return -ENODEV;
    }
    s_intf = intf;
    rc = rpmsg_sensor_register(pdev, (void*)sensor);
    if (rc < 0) {
        hwsensor_unregister(intf);
        s_intf = NULL;
        cam_err("%s rpmsg_sensor_register failed rc %d\n", __func__, rc);
        return -ENODEV;
    }
    return rc;
}

static int __init
imx258_init_module(void)
{
    cam_notice("enter %s",__func__);
    return platform_driver_probe(&s_imx258_driver,
            imx258_platform_probe);
}

static void __exit
imx258_exit_module(void)
{
    if (NULL != s_intf) {
    rpmsg_sensor_unregister((void *)&s_imx258);
    hwsensor_unregister(&s_imx258.intf);
        s_intf = NULL;
    }
    platform_driver_unregister(&s_imx258_driver);
}

module_init(imx258_init_module);
module_exit(imx258_exit_module);
MODULE_DESCRIPTION("imx258");
MODULE_LICENSE("GPL v2");
//lint -restore

