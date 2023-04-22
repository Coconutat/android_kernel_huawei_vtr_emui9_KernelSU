 


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
#define POWER_SETTING_DELAY_6 6

extern struct hw_csi_pad hw_csi_pad;
static hwsensor_vtbl_t s_ov13855_vtbl;
static hwsensor_intf_t *s_intf = NULL;

struct sensor_power_setting hw_ov13855_power_setting[] = {

/*
	{
		.seq_type = SENSOR_SUSPEND,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 0,
	},
*/
	//enable gpio88 output iovdd 1.8v
	{
		.seq_type = SENSOR_LDO_EN,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},
	//MCAM1 AFVDD LDO25 2.85V
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
		.delay = POWER_SETTING_DELAY_1,
	},

	//MCAM0 DVDD VLDO1.2V
	{
		.seq_type = SENSOR_DVDD2,
		.data = (void*)"main-sensor-dvdd",
		.config_val = LDO_VOLTAGE_1P2V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_1,
	},

	//MCAM Reset GPIO_052
	{
		.seq_type = SENSOR_RST,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_6,
	},

	//MCAM ISP_CLK0 16M
	{
		.seq_type = SENSOR_MCLK,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_1,
	},
};

struct sensor_power_setting hw_ov13855_power_setting_v4[] = {

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

	//enable gpio88 output iovdd 1.8v
	{
		.seq_type = SENSOR_LDO_EN,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_0,
	},
	//MCAM1 AFVDD LDO25 2.85V
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
		.delay = POWER_SETTING_DELAY_1,
	},

	//MCAM0 DVDD VLDO1.2V
	{
		.seq_type = SENSOR_DVDD2,
		.data = (void*)"main-sensor-dvdd",
		.config_val = LDO_VOLTAGE_1P2V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_1,
	},

	//MCAM Reset GPIO_052
	{
		.seq_type = SENSOR_RST,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_6,
	},

	//MCAM ISP_CLK0 16M
	{
		.seq_type = SENSOR_MCLK,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = POWER_SETTING_DELAY_1,
	},
};


static sensor_t s_ov13855 =
{
    .intf = { .vtbl = &s_ov13855_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(hw_ov13855_power_setting),
            .power_setting = hw_ov13855_power_setting,
     },
};

static sensor_t s_ov13855_v4 =
{
    .intf = { .vtbl = &s_ov13855_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(hw_ov13855_power_setting_v4),
            .power_setting = hw_ov13855_power_setting_v4,
     },
};

static const struct of_device_id
s_ov13855_dt_match[] =
{
    {
        .compatible = "huawei,ov13855",
        .data = &s_ov13855.intf,
    },
    {
        .compatible = "huawei,ov13855_v4",
        .data = &s_ov13855_v4.intf,
    },
    {
    },
};

MODULE_DEVICE_TABLE(of, s_ov13855_dt_match);

static struct platform_driver
s_ov13855_driver =
{
	.driver =
    {
		.name = "huawei,ov13855",
		.owner = THIS_MODULE,
		.of_match_table = s_ov13855_dt_match,
	},
};

char const*
ov13855_get_name(
        hwsensor_intf_t* si)
{
    sensor_t* sensor = I2S(si);
    return sensor->board_info->name;
}
static int
ov13855_power_up(
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
ov13855_power_down(
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

static int ov13855_csi_enable(hwsensor_intf_t* si)
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

int ov13855_csi_disable(hwsensor_intf_t* si)
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
ov13855_match_id(
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
ov13855_config(
        hwsensor_intf_t* si,
        void  *argp)
{
	struct sensor_cfg_data *data;

	int ret =0;
	static bool ov13855_power_on = false;

    if (NULL == si || NULL == argp){
        cam_err("%s si or argp is null.\n", __func__);
        return -1;
    }

	data = (struct sensor_cfg_data *)argp;
	cam_debug("ov13855 cfgtype = %d",data->cfgtype);
	switch(data->cfgtype){
		case SEN_CONFIG_POWER_ON:
			if (!ov13855_power_on) {
				ret = si->vtbl->power_up(si);
				ov13855_power_on = true;
			}
			break;
		case SEN_CONFIG_POWER_OFF:
			if (ov13855_power_on) {
				ret = si->vtbl->power_down(si);
				ov13855_power_on = false;
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
s_ov13855_vtbl =
{
	.get_name = ov13855_get_name,
	.config = ov13855_config,
	.power_up = ov13855_power_up,
	.power_down = ov13855_power_down,
	.match_id = ov13855_match_id,
	.csi_enable = ov13855_csi_enable,
	.csi_disable = ov13855_csi_disable,
};
static int32_t
ov13855_platform_probe(
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

    id = of_match_node(s_ov13855_dt_match, np);
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
ov13855_init_module(void)
{
    cam_notice("enter %s",__func__);
    return platform_driver_probe(&s_ov13855_driver,
            ov13855_platform_probe);
}

static void __exit
ov13855_exit_module(void)
{
    if (NULL != s_intf) {
    rpmsg_sensor_unregister((void *)&s_ov13855);
    hwsensor_unregister(&s_ov13855.intf);
        s_intf = NULL;
    }
    platform_driver_unregister(&s_ov13855_driver);
}

module_init(ov13855_init_module);
module_exit(ov13855_exit_module);
MODULE_DESCRIPTION("ov13855");
MODULE_LICENSE("GPL v2");
//lint -restore

