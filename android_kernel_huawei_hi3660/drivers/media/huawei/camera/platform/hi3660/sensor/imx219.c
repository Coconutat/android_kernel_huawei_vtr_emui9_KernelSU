 

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
#include "../pmic/hw_pmic.h"

//lint -save -e846 -e866 -e826 -e785 -e838 -e715 -e747 -e774 -e778 -e732 -e731 -e650
//lint -save -e31
#define I2S(i) container_of((i), sensor_t, intf)
static hwsensor_vtbl_t s_imx219_vtbl;
static bool power_on_status = false;//false: power off, true:power on
int imx219_config(hwsensor_intf_t* si, void  *argp);
static hwsensor_intf_t *s_intf = NULL;

struct sensor_power_setting imx219_power_setting[] = {

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

struct sensor_power_setting imx219_power_setting_v3[] = {

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

    //M0 + M1  IOVDD  1.8V  [CAM_PMIC_LDO1]
    {
        .seq_type = SENSOR_PMIC,
        .seq_val = VOUT_LDO_1,
        .config_val = LDO_VOLTAGE_1P8V,
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

static sensor_t s_imx219 =
{
    .intf = { .vtbl = &s_imx219_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(imx219_power_setting),
            .power_setting = imx219_power_setting,
     },
};

static sensor_t s_imx219_v3 =
{
    .intf = { .vtbl = &s_imx219_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(imx219_power_setting_v3),
            .power_setting = imx219_power_setting_v3,
     },
};

static const struct of_device_id s_imx219_dt_match[] =
{
    {
        .compatible = "huawei,imx219_cmb",
        .data = &s_imx219.intf,
    },
    {
        .compatible = "huawei,imx219_cmb_v3",
        .data = &s_imx219_v3.intf,
    },
    {

    },/* terminate list */
};

MODULE_DEVICE_TABLE(of, s_imx219_dt_match);

static struct platform_driver s_imx219_driver =
{
    .driver =
    {
        .name = "huawei,imx219",
        .owner = THIS_MODULE,
        .of_match_table = s_imx219_dt_match,
    },
};

char const* imx219_get_name(hwsensor_intf_t* si)
{
    sensor_t* sensor = NULL;
    if (NULL == si) {
        cam_err("%s. si is NULL.", __func__);
        return NULL;
    }
    sensor = I2S(si);
    return sensor->board_info->name;
}

int imx219_power_up(hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;

    if (NULL == si) {
        cam_err("%s. si is NULL.", __func__);
        return -EINVAL;
    }

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
imx219_power_down(
        hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;

    if (NULL == si) {
        cam_err("%s. si is NULL.", __func__);
        return -EINVAL;
    }

    sensor = I2S(si);
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

int imx219_csi_enable(hwsensor_intf_t* si)
{
    return 0;
}

int imx219_csi_disable(hwsensor_intf_t* si)
{
    return 0;
}

static int imx219_match_id(
        hwsensor_intf_t* si, void * data)
{


    sensor_t* sensor = NULL;
    struct sensor_cfg_data *cdata = (struct sensor_cfg_data *)data;
    struct pinctrl_state *pinctrl_def = NULL;
    struct pinctrl_state *pinctrl_idle = NULL;
    uint32_t module_id_0 = 0;
    uint32_t module_id_1 = 0;
    struct pinctrl *p = NULL;
    int rc = 0;
    const char *sensor_name [] = { "IMX219_SUNNY", "IMX219_LITEON"};
    enum {
        IMX219_SUNNY_INDEX  = 0,
        IMX219_LITEON_INDEX = 1,
    };

    if (NULL == si) {
        cam_err("%s. si is NULL.", __func__);
        return -EINVAL;
    }
    sensor = I2S(si);
    memset(cdata->cfg.name, 0, DEVICE_NAME_SIZE);

    p = devm_pinctrl_get(sensor->dev);
    if (IS_ERR_OR_NULL(p)) {
        cam_err("could not get pinctrl.\n");
        rc = -1;
        goto matchID_exit;
    }

    rc = gpio_request(sensor->board_info->gpios[FSIN].gpio, NULL);
    if(rc < 0) {
        cam_err("%s failed to request gpio[%d]", __func__, sensor->board_info->gpios[FSIN].gpio);
        rc = -1;
        goto matchID_exit;
    }
    cam_info("%s gpio[%d].", __func__, sensor->board_info->gpios[FSIN].gpio);

    pinctrl_def = pinctrl_lookup_state(p, "default");
    if (IS_ERR_OR_NULL(pinctrl_def)) {
        cam_err("could not get defstate.\n");
        rc = -1;
        goto pinctrl_error;
    }

    pinctrl_idle = pinctrl_lookup_state(p, "idle");
    if (IS_ERR_OR_NULL(pinctrl_idle)) {
        pr_err("could not get idle defstate.\n");
        rc = -1;
        goto pinctrl_error;
    }
    /*PULL UP*/
    rc = pinctrl_select_state(p, pinctrl_def);
    if (rc) {
        cam_err("could not set pins to default state.\n");
        rc = -1;
        goto pinctrl_error;
    }
    udelay(10);
    cam_info("%s gpio[%d].", __func__, sensor->board_info->gpios[FSIN].gpio);
    rc = gpio_direction_input(sensor->board_info->gpios[FSIN].gpio);
    if (rc < 0) {
        cam_err("%s failed to config gpio(%d) input.\n",__func__, sensor->board_info->gpios[FSIN].gpio);
        rc = -1;
        goto pinctrl_error;
    }
    module_id_0 = gpio_get_value(sensor->board_info->gpios[FSIN].gpio);

    /*PULL DOWN*/
    rc = pinctrl_select_state(p, pinctrl_idle);
    if (rc) {
        cam_err("could not set pins to idle state.\n");
        rc = -1;
        goto pinctrl_error;
    }
    udelay(10);
    cam_info("%s gpio[%d].", __func__, sensor->board_info->gpios[FSIN].gpio);
    rc = gpio_direction_input(sensor->board_info->gpios[FSIN].gpio);
    if (rc < 0) {
        cam_err("%s failed to config gpio(%d) input.\n",__func__, sensor->board_info->gpios[FSIN].gpio);
        rc = -1;
        goto pinctrl_error;
    }
    module_id_1 = gpio_get_value(sensor->board_info->gpios[FSIN].gpio);

    cam_info("%s module_id_0 %d module_id_1 %d .\n",__func__, module_id_0, module_id_1);
    if ((module_id_0 == 1) && (module_id_1 == 1)){
        //sunny module
        strncpy(cdata->cfg.name, sensor_name[IMX219_SUNNY_INDEX], DEVICE_NAME_SIZE - 1);
        cdata->data = sensor->board_info->sensor_index;
        rc = 0;
    } else if ((module_id_0 == 1) && (module_id_1 == 0)){
        //liteon module
        strncpy(cdata->cfg.name, sensor_name[IMX219_LITEON_INDEX], DEVICE_NAME_SIZE - 1);
        cdata->data = sensor->board_info->sensor_index;
        rc = 0;
    } else {
        strncpy(cdata->cfg.name, sensor_name[IMX219_SUNNY_INDEX], DEVICE_NAME_SIZE - 1); //set default name
        cam_warn("%s failed to get the module id value set default name.\n",__func__);
        cdata->data = sensor->board_info->sensor_index;
        //strncpy(cdata->cfg.name, sensor->board_info->name, strlen(sensor->board_info->name)+1);
        //cam_err("%s failed to get the module id value.\n",__func__);
        rc = 0;
    }

pinctrl_error:
        gpio_free(sensor->board_info->gpios[FSIN].gpio);

matchID_exit:
        if (cdata->data != SENSOR_INDEX_INVALID) {
            cam_info("%s, cdata->cfg.name = %s", __func__,cdata->cfg.name );
        }
    return rc;
}

static hwsensor_vtbl_t s_imx219_vtbl =
{
    .get_name = imx219_get_name,
    .config = imx219_config,
    .power_up = imx219_power_up,
    .power_down = imx219_power_down,
    .match_id = imx219_match_id,
    .csi_enable = imx219_csi_enable,
    .csi_disable = imx219_csi_disable,
};

int imx219_config(hwsensor_intf_t* si, void  *argp)
{
    struct sensor_cfg_data *data = NULL;
    int ret = 0;

    if ((NULL == si) || (NULL == argp)) {
        cam_err("%s : si or argp is null", __func__);
        return -EINVAL;
    }

    data = (struct sensor_cfg_data *)argp;
    cam_debug("imx219 cfgtype = %d",data->cfgtype);
    switch(data->cfgtype) {
        case SEN_CONFIG_POWER_ON:
            if (false == power_on_status) {
                ret = si->vtbl->power_up(si);
                if (0 == ret) {
                    power_on_status = true;
                } else {
                    cam_err("%s. power up fail.", __func__);
                }
            }
            break;
        case SEN_CONFIG_POWER_OFF:
            if (true == power_on_status) {
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
            ret = si->vtbl->match_id(si,argp);
            break;
        default:
            cam_warn("%s cfgtype(%d) is unknow", __func__, data->cfgtype);
            break;
    }

    return ret;
}

static int32_t imx219_platform_probe(struct platform_device* pdev)
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

    id = of_match_node(s_imx219_dt_match, np);
    if (!id) {
        cam_err("%s none id matched", __func__);
        return -ENODEV;
    }

    intf = (hwsensor_intf_t*)id->data;
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

static int __init imx219_init_module(void)
{
    cam_info("Enter: %s", __func__);
    return platform_driver_probe(&s_imx219_driver,
            imx219_platform_probe);
}

static void __exit imx219_exit_module(void)
{
    if (NULL != s_intf) {
        hwsensor_unregister(s_intf);
        s_intf = NULL;
    }

    platform_driver_unregister(&s_imx219_driver);
}

module_init(imx219_init_module);
module_exit(imx219_exit_module);
MODULE_DESCRIPTION("imx219");
MODULE_LICENSE("GPL v2");
//lint -restore
