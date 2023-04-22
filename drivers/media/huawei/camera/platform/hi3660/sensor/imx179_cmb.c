 

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/rpmsg.h>

#include "hwsensor.h"
#include "sensor_commom.h"
#include "hw_csi.h"
#include <linux/pinctrl/consumer.h>
//lint -save -e650 -e31

#define I2S(i) container_of(i, sensor_t, intf)
static hwsensor_vtbl_t s_imx179_vtbl;
static bool power_on_status = false;//false: power off, true:power on
int imx179_config(hwsensor_intf_t* si, void  *argp);

struct sensor_power_setting imx179_power_setting[] = {

    //SCAM AVDD 2.80V
    {
        .seq_type = SENSOR_AVDD,
        .data = (void*)"slave-sensor-avdd",
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },

    //SCAM DVDD 1.2V
    {
        .seq_type = SENSOR_DVDD,
        .config_val = LDO_VOLTAGE_1P2V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

    //M0 + M1  IOVDD  1.8V  [GPIO 088 Enable]
    {
        .seq_type = SENSOR_IOVDD_EN,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },

    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

    //SCAM RST
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
};

static sensor_t s_imx179 =
{
    .intf = { .vtbl = &s_imx179_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(imx179_power_setting),
            .power_setting = imx179_power_setting,
     },
};

static const struct of_device_id s_imx179_dt_match[] =
{
    {
        .compatible = "huawei,imx179_cmb",
        .data = &s_imx179.intf,
    },
    {

    },/* terminate list */
};

MODULE_DEVICE_TABLE(of, s_imx179_dt_match);

static struct platform_driver s_imx179_driver =
{
    .driver =
    {
        .name = "huawei,imx179",
        .owner = THIS_MODULE,
        .of_match_table = s_imx179_dt_match,
    },
};

char const* imx179_get_name(hwsensor_intf_t* si)
{
    sensor_t* sensor = I2S(si);
    return sensor->board_info->name;
}

int imx179_power_up(hwsensor_intf_t* si)
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

    if (0 == ret) {
        cam_info("%s. power up sensor success.", __func__);
    } else {
        cam_err("%s. power up sensor fail.", __func__);
    }
    return ret;
}

int
imx179_power_down(
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

    if (0 == ret ) {
        cam_info("%s. power down sensor success.", __func__);
    } else {
        cam_err("%s. power down sensor fail.", __func__);
    }

    return ret;
}

int imx179_csi_enable(hwsensor_intf_t* si)
{
    return 0;
}

int imx179_csi_disable(hwsensor_intf_t* si)
{
    return 0;
}

static int imx179_match_id(
        hwsensor_intf_t* si, void * data)
{
    sensor_t* sensor = I2S(si);
    struct sensor_cfg_data *cdata = (struct sensor_cfg_data *)data;
    char *sensor_name [] = {"IMX179_LITEON"}; //just support liteon for cmb

    memset(cdata->cfg.name, 0, DEVICE_NAME_SIZE);
    strncpy(cdata->cfg.name, sensor_name[0], DEVICE_NAME_SIZE-1);
    cdata->data = sensor->board_info->sensor_index;

    if (cdata->data != SENSOR_INDEX_INVALID) {
        cam_info("%s, cdata->cfg.name = %s", __func__,cdata->cfg.name );
    }
    return 0;
}

static hwsensor_vtbl_t s_imx179_vtbl =
{
    .get_name = imx179_get_name,
    .config = imx179_config,
    .power_up = imx179_power_up,
    .power_down = imx179_power_down,
    .match_id = imx179_match_id,
    .csi_enable = imx179_csi_enable,
    .csi_disable = imx179_csi_disable,
};

int imx179_config(hwsensor_intf_t* si, void  *argp)
{
    struct sensor_cfg_data *data;

    int ret =0;

    if (NULL == si || NULL == argp) {
        cam_err("%s : si or argp is null", __func__);
        return -1;
    }

    data = (struct sensor_cfg_data *)argp;
    cam_debug("imx179 cfgtype = %d",data->cfgtype);
    switch(data->cfgtype) {
        case SEN_CONFIG_POWER_ON:
            if (false == power_on_status) {
                ret = si->vtbl->power_up(si);
                power_on_status = true;
            }
            break;
        case SEN_CONFIG_POWER_OFF:
            if (true == power_on_status) {
                ret = si->vtbl->power_down(si);
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
            ret = si->vtbl->match_id(si,argp);
            break;
        default:
            cam_warn("%s cfgtype(%d) is unknow", __func__, data->cfgtype);
            break;
    }

    return ret;
}

static int32_t imx179_platform_probe(struct platform_device* pdev)
{
    int rc = 0;
    cam_notice("enter %s",__func__);

    if (pdev->dev.of_node) {
        rc = hw_sensor_get_dt_data(pdev, &s_imx179);
        if (rc < 0) {
            cam_err("%s failed line %d\n", __func__, __LINE__);
            goto imx179_sensor_probe_fail;
        }

    } else {
        cam_err("%s imx179 of_node is NULL.\n", __func__);
        goto imx179_sensor_probe_fail;
    }
    s_imx179.dev = &pdev->dev;
    rc = hwsensor_register(pdev, &s_imx179.intf);
    rc = rpmsg_sensor_register(pdev, (void*)&s_imx179);

imx179_sensor_probe_fail:
    return rc;
}

static int __init imx179_init_module(void)
{
    cam_info("Enter: %s", __func__);
    return platform_driver_probe(&s_imx179_driver,
            imx179_platform_probe);
}

static void __exit imx179_exit_module(void)
{
    hwsensor_unregister(&s_imx179.intf);
    platform_driver_unregister(&s_imx179_driver);
}

module_init(imx179_init_module);
module_exit(imx179_exit_module);
MODULE_DESCRIPTION("imx179");
MODULE_LICENSE("GPL v2");
//lint -restore

