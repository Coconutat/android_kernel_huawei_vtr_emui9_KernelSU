 

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
#define POWER_SETTING_DELAY_1 1 //wait 1 ms

static hwsensor_vtbl_t s_ov8856_vtbl;
static bool power_on_status = false;//false: power off, true:power on
static int ov8856_config(hwsensor_intf_t* si, void  *argp);
static hwsensor_intf_t *s_intf = NULL;

static struct sensor_power_setting ov8856_duke_power_setting[] = {
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

    //M0 + M1  IOVDD  1.8V  [CAM_PMIC_LDO1]
    {
        .seq_type = SENSOR_PMIC,
        .seq_val = VOUT_LDO_1,
        .config_val = LDO_VOLTAGE_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_SETTING_DELAY_0,
    },

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

static sensor_t s_ov8856_duke =
{
    .intf = { .vtbl = &s_ov8856_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(ov8856_duke_power_setting),
            .power_setting = ov8856_duke_power_setting,
     },
};

static const struct of_device_id s_ov8856_dt_match[] =
{
    {
        .compatible = "huawei,ov8856_duke",
        .data = &s_ov8856_duke.intf,
    },
    {

    },/* terminate list */
};

MODULE_DEVICE_TABLE(of, s_ov8856_dt_match);

static struct platform_driver s_ov8856_driver =
{
    .driver =
    {
        .name = "huawei,ov8856_duke",
        .owner = THIS_MODULE,
        .of_match_table = s_ov8856_dt_match,
    },
};

static char const* ov8856_get_name(hwsensor_intf_t* si)
{
    sensor_t* sensor = NULL;
    if (NULL == si) {
        cam_err("%s. si is NULL.", __func__);
        return NULL;
    }
    sensor = I2S(si);
    return sensor->board_info->name;
}

static int ov8856_power_up(hwsensor_intf_t* si)
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
ov8856_power_down(
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

static int ov8856_csi_enable(hwsensor_intf_t* si)
{
    return 0;
}

static int ov8856_csi_disable(hwsensor_intf_t* si)
{
    return 0;
}

static int ov8856_match_id(
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

    strncpy(cdata->cfg.name, sensor->board_info->name, DEVICE_NAME_SIZE-1);
    cdata->data = sensor->board_info->sensor_index;
    return 0;
}

static hwsensor_vtbl_t s_ov8856_vtbl =
{
    .get_name = ov8856_get_name,
    .config = ov8856_config,
    .power_up = ov8856_power_up,
    .power_down = ov8856_power_down,
    .match_id = ov8856_match_id,
    .csi_enable = ov8856_csi_enable,
    .csi_disable = ov8856_csi_disable,
};

static int ov8856_config(hwsensor_intf_t* si, void  *argp)
{
    struct sensor_cfg_data *data = NULL;
    int ret = 0;

    if ((NULL == si) || (NULL == argp) || (NULL == si->vtbl)) {
        cam_err("%s : si, argp or si->vtbl is null", __func__);
        return -EINVAL;
    }
    data = (struct sensor_cfg_data *)argp;
    cam_debug("ov8856_duke cfgtype = %d",data->cfgtype);
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

static int32_t ov8856_platform_probe(struct platform_device* pdev)
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

    id = of_match_node(s_ov8856_dt_match, np);
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

static int __init ov8856_init_module(void)
{
    cam_info("Enter: %s", __func__);
    return platform_driver_probe(&s_ov8856_driver,
            ov8856_platform_probe);
}

static void __exit ov8856_exit_module(void)
{
    if (NULL != s_intf) {
        rpmsg_sensor_unregister((void*)&s_ov8856_duke);
        hwsensor_unregister(s_intf);
        s_intf = NULL;
    }

    platform_driver_unregister(&s_ov8856_driver);
}

module_init(ov8856_init_module);
module_exit(ov8856_exit_module);
MODULE_DESCRIPTION("ov8856");
MODULE_LICENSE("GPL v2");
//lint -restore
