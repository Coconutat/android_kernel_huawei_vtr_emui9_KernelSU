 

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

//lint -save -e846 -e866 -e826 -e785 -e838 -e715 -e747 -e774 -e778 -e732 -e731
//lint -save -e514 -e30 -e84 -e31 -e64 -esym(528,*) -esym(753,*)
#define I2S(i) container_of((i), sensor_t, intf)
#define POWER_SETTING_DELAY_0 0
#define POWER_SETTING_DELAY_1 1

static hwsensor_vtbl_t s_hi846_vtbl;
static bool power_on_status = false;//false: power off, true:power on
static int hi846_config(hwsensor_intf_t* si, void  *argp);
static hwsensor_intf_t *s_intf = NULL;
extern int strncpy_s(char *strDest, size_t destMax, const char *strSrc, size_t count);

static struct sensor_power_setting hi846_power_setting[] = {
    //SCAM IOVDD 2.80V
    {
        .seq_type = SENSOR_LDO_EN,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_SETTING_DELAY_0,
    },

    //SCAM AVDD 2.80V
    {
        .seq_type = SENSOR_AVDD,
        .data = (void*)"slave-sensor-avdd",
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_SETTING_DELAY_0,
    },

    {
        .seq_type = SENSOR_AVDD2_EN,
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
    //SCAM MCLK
    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_SETTING_DELAY_1,
    },

    //SCAM RST
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_SETTING_DELAY_1,
    },
};

static struct sensor_power_setting hi846_power_setting_v4[] = {
    //MCAM Reset GPIO_052
    {
        .seq_type = SENSOR_SUSPEND,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_SETTING_DELAY_1,
    },

    //MCAM0 DVDD VLDO1.2V
    {
        .seq_type = SENSOR_DVDD2,
        .data = (void*)"main-sensor-dvdd",
        .config_val = LDO_VOLTAGE_1P2V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_SETTING_DELAY_0,
    },

    //SCAM IOVDD 2.80V
    {
        .seq_type = SENSOR_LDO_EN,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_SETTING_DELAY_0,
    },

    //SCAM AVDD 2.80V
    {
        .seq_type = SENSOR_AVDD,
        .data = (void*)"slave-sensor-avdd",
        .config_val = LDO_VOLTAGE_V2P8V,
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
    //SCAM MCLK
    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_SETTING_DELAY_1,
    },

    //SCAM RST
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_SETTING_DELAY_1,
    },
};

static sensor_t s_hi846 =
{
    .intf = { .vtbl = &s_hi846_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(hi846_power_setting),
            .power_setting = hi846_power_setting,
     },
};

static sensor_t s_hi846_v4 =
{
    .intf = { .vtbl = &s_hi846_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(hi846_power_setting_v4),
            .power_setting = hi846_power_setting_v4,
     },
};

static const struct of_device_id s_hi846_dt_match[] =
{
    {
        .compatible = "huawei,hi846",
        .data = &s_hi846.intf,
    },
    {
        .compatible = "huawei,hi846_v4",
        .data = &s_hi846_v4.intf,
    },
    {

    },/* terminate list */
};

MODULE_DEVICE_TABLE(of, s_hi846_dt_match);

static struct platform_driver s_hi846_driver =
{
    .driver =
    {
        .name = "huawei,hi846",
        .owner = THIS_MODULE,
        .of_match_table = s_hi846_dt_match,
    },
};

static char const* hi846_get_name(hwsensor_intf_t* si)
{
    sensor_t* sensor = NULL;
    if (NULL == si) {
        cam_err("%s. si is NULL.", __func__);
        return NULL;
    }
    sensor = I2S(si);
    return sensor->board_info->name;
}

static int hi846_power_up(hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;

    if (NULL == si) {
        cam_err("%s. si is NULL.", __func__);
        return -EINVAL;
    }

    sensor = I2S(si);

    if (NULL == sensor->board_info) {
        cam_err("%s. sensor->board_info is NULL.", __func__);
        return -EINVAL;
    }
    cam_info("enter %s. index = %d name = %s", __func__, sensor->board_info->sensor_index, sensor->board_info->name);

    if (hw_is_fpga_board()) {
        ret = do_sensor_power_on(sensor->board_info->sensor_index, sensor->board_info->name);
    } else {
        ret = hw_sensor_power_up(sensor);
    }

    if (0 == ret) {
        cam_info("%s. power up sensor success.", __func__);
    } else {
        cam_err("%s. power up sensor fail.", __func__);
    }
    return ret;
}

static int
hi846_power_down(
        hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;

    if (NULL == si) {
        cam_err("%s. si is NULL.", __func__);
        return -EINVAL;
    }

    sensor = I2S(si);

    if (NULL == sensor->board_info) {
        cam_err("%s. sensor->board_info is NULL.", __func__);
        return -EINVAL;
    }
    if (NULL == sensor->board_info->name) {
        cam_err("%s. sensor name is NULL.", __func__);
        return -EINVAL;
    }
    cam_info("enter %s. index = %d name = %s", __func__, sensor->board_info->sensor_index, sensor->board_info->name);
    if (hw_is_fpga_board()) {
        ret = do_sensor_power_off(sensor->board_info->sensor_index, sensor->board_info->name);
    } else {
        ret = hw_sensor_power_down(sensor);
    }

    if (0 == ret) {
        cam_info("%s. power down sensor success.", __func__);
    } else {
        cam_err("%s. power down sensor fail.", __func__);
    }
    return ret;
}

static int hi846_csi_enable(hwsensor_intf_t* si)
{
    return 0;
}

static int hi846_csi_disable(hwsensor_intf_t* si)
{
    return 0;
}

static int hi846_match_id(
        hwsensor_intf_t* si, void * data)
{
    sensor_t* sensor = NULL;
    struct sensor_cfg_data *cdata = NULL;
    if ((NULL == si)||(NULL == data)) {
        cam_err("%s. si is NULL.", __func__);
        return -EINVAL;
    }

    sensor = I2S(si);
    cdata = (struct sensor_cfg_data *)data;

    if(NULL == sensor->board_info) {
        cam_err("%s. sensor->board_info is NULL.", __func__);
        return -EINVAL;
    }
    if(NULL == sensor->board_info->name) {
        cam_err("%s. sensor name is NULL.", __func__);
        return -EINVAL;
    }
    cam_info("%s name:%s", __func__, sensor->board_info->name);

    strncpy_s(cdata->cfg.name, DEVICE_NAME_SIZE-1, sensor->board_info->name, DEVICE_NAME_SIZE-1);
    cdata->data = sensor->board_info->sensor_index;
    return 0;
}

static hwsensor_vtbl_t s_hi846_vtbl =
{
    .get_name = hi846_get_name,
    .config = hi846_config,
    .power_up = hi846_power_up,
    .power_down = hi846_power_down,
    .match_id = hi846_match_id,
    .csi_enable = hi846_csi_enable,
    .csi_disable = hi846_csi_disable,
};

static int hi846_config(hwsensor_intf_t* si, void  *argp)
{
    struct sensor_cfg_data *data = NULL;
    int ret = 0;

    if ((NULL == si) || (NULL == argp)) {
        cam_err("%s : si or argp is null", __func__);
        return -EINVAL;
    }
    data = (struct sensor_cfg_data *)argp;
    cam_debug("hi846 cfgtype = %d",data->cfgtype);
    switch(data->cfgtype) {
        case SEN_CONFIG_POWER_ON:
            if (NULL == si->vtbl->power_up) {
                cam_err("%s. si power_up is null", __func__);
                ret = -EINVAL;
            } else if (!power_on_status) {
                ret = si->vtbl->power_up(si);
                if (0 == ret) {
                    power_on_status = true;
                } else {
                    cam_err("%s. power up fail.", __func__);
                }
            }
            break;
        case SEN_CONFIG_POWER_OFF:
            if (NULL == si->vtbl->power_down) {
                cam_err("%s. si power_down is null.", __func__);
                ret = -EINVAL;
            } else if (power_on_status) {
                ret = si->vtbl->power_down(si);
                if (0 != ret) {
                    cam_err("%s. power_down fail.", __func__);
                }
                power_on_status = false;
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
        case SEN_CONFIG_MATCH_ID:
            if (NULL == si->vtbl->match_id) {
                cam_err("%s. match_id fail.", __func__);
                ret = -EINVAL;
            } else {
                ret = si->vtbl->match_id(si,argp);
            }
            break;
        default:
            cam_warn("%s cfgtype(%d) is unknow", __func__, data->cfgtype);
            break;
    }

    return ret;
}

static int32_t hi846_platform_probe(struct platform_device* pdev)
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

    id = of_match_node(s_hi846_dt_match, np);
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

static int __init hi846_init_module(void)
{
    cam_info("Enter: %s", __func__);
    return platform_driver_probe(&s_hi846_driver,
            hi846_platform_probe);
}

static void __exit hi846_exit_module(void)
{
    if (NULL != s_intf) {
        rpmsg_sensor_unregister((void*)&s_hi846);
        hwsensor_unregister(s_intf);
        s_intf = NULL;
    }

    platform_driver_unregister(&s_hi846_driver);
}

module_init(hi846_init_module);
module_exit(hi846_exit_module);
MODULE_DESCRIPTION("hi846");
MODULE_LICENSE("GPL v2");
//lint -restore
