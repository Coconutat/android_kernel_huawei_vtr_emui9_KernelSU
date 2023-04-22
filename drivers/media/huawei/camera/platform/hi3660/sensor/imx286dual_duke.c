


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

//lint -save -e846 -e826 -e785 -e838 -e715 -e747 -e774
//lint -save -e778 -e514 -e866 -e30 -e84 -e64 -esym(528,*) -esym(753,*)
#define I2S(i) container_of((i), sensor_t, intf)
#define CTL_RESET_HOLD    (0)
#define CTL_RESET_RELEASE (1)

static bool power_on_status = false;//false: power off, true:power on

static struct sensor_power_setting imx286dual_duke_power_setting[] = {

    //M0 AVDD0  2.80V  [PMIC-LDO4]
    {
        .seq_type = SENSOR_PMIC,
        .seq_val = VOUT_LDO_4,
        .config_val = PMIC_2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },

    //M1 AVDD1  2.80V  [PMIC-LDO5]
    {
        .seq_type = SENSOR_PMIC,
        .seq_val = VOUT_LDO_5,
        .config_val = PMIC_2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },

    // DVDD 1.125v[PMIC-BUCK1]
    {
        .seq_type = SENSOR_PMIC,
        .seq_val = VOUT_BUCK_1,
        .config_val = PMIC_1P125V,  // 1.125v
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },

    //M0 + M1  VCM  3V  [PMIC-BUCK2]
    {
        .seq_type = SENSOR_PMIC,
        .seq_val  = VOUT_BUCK_2,
        .config_val = PMIC_2P85V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
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
        .sensor_index = 0,
        .delay = 1,
    },

    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = 2,
        .delay = 1,
    },

    //M0 RESET  [GPIO_052]
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

    //M1 RESET  [GPIO_21]
    {
        .seq_type = SENSOR_RST2,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

};

static char const*
imx286dual_get_name(
        hwsensor_intf_t* si)
{
    sensor_t* sensor = NULL;
    if (NULL == si) {
        cam_err("%s si is NULL", __func__);
        return NULL;
    }
    sensor = I2S(si);
    return sensor->board_info->name;
}

static int
imx286dual_power_up(
        hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;

    if (NULL == si) {
        cam_err("%s si is NULL", __func__);
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

static int
imx286dual_power_down(
        hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;

    if (NULL == si) {
        cam_err("%s si is NULL", __func__);
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

static int imx286dual_csi_enable(hwsensor_intf_t* si)
{
    return 0;
}

static int imx286dual_csi_disable(hwsensor_intf_t* si)
{
    return 0;
}

static int
imx286dual_match_id(
        hwsensor_intf_t* si, void * data)
{
    sensor_t* sensor = NULL;
    struct sensor_cfg_data *cdata = NULL;
    if (NULL == si) {
        cam_err("%s si is NULL", __func__);
        return -EINVAL;
    }
    cdata = (struct sensor_cfg_data *)data;
    sensor = I2S(si);

    cam_info("%s name:%s", __func__, sensor->board_info->name);
    memset(cdata->cfg.name, 0, DEVICE_NAME_SIZE);
    strncpy(cdata->cfg.name, sensor->board_info->name, DEVICE_NAME_SIZE-1);
    cdata->data = sensor->board_info->sensor_index;

    return 0;
}

static int imx286dual_do_hw_reset(hwsensor_intf_t* si, int ctl, int id)
{
    char *state = NULL;
    sensor_t* sensor = NULL;
    hwsensor_board_info_t *b_info = NULL;
    int ret = 0;
    if (NULL == si) {
        cam_err("%s si is NULL", __func__);
        return -EINVAL;
    }
    sensor = I2S(si);

    b_info = sensor->board_info;
    if (NULL == b_info) {
        cam_warn("%s invalid sensor board info", __func__);
        return 0;
    }

    ret  = gpio_request(b_info->gpios[RESETB].gpio, "imx286reset-0");
    if (ret) {
        cam_err("%s requeset reset pin1 failed", __func__);
        return ret;
    }
    ret = gpio_request(b_info->gpios[RESETB2].gpio, "imx286reset-1");
    if (ret) {
        cam_err("%s requeset reset pin2 failed", __func__);
        gpio_free(b_info->gpios[RESETB].gpio);
        return ret;
    }

    if (ctl == CTL_RESET_HOLD) {
        state = "hold";
        ret  = gpio_direction_output(b_info->gpios[RESETB].gpio, 0);
        ret |= gpio_direction_output(b_info->gpios[RESETB2].gpio, 0);
    } else {
        state = "release";
        ret  = gpio_direction_output(b_info->gpios[RESETB].gpio, 1);
        ret |= gpio_direction_output(b_info->gpios[RESETB2].gpio, 1);
        udelay(2000);
    }
    gpio_free(b_info->gpios[RESETB].gpio);
    gpio_free(b_info->gpios[RESETB2].gpio);

    if (ret) {
        cam_err("%s set reset pin failed", __func__);
    } else {
        cam_info("%s: set reset state=%s", __func__, state);
    }
    return ret;
}

static int
imx286dual_config(
        hwsensor_intf_t* si,
        void  *argp)
{
    struct sensor_cfg_data *data = NULL;
    int ret = 0;

    if ((NULL == si) || (NULL == argp)){
        cam_err("%s : si or argp is null", __func__);
        return -EINVAL;
    }

    data = (struct sensor_cfg_data *)argp;
    cam_debug("imx286dual cfgtype = %d",data->cfgtype);
    switch(data->cfgtype) {
        case SEN_CONFIG_POWER_ON:
            if (!power_on_status) {
                ret = si->vtbl->power_up(si);
                if (0 == ret) {
                    power_on_status = true;
                } else {
                    cam_err("%s. power up fail.", __func__);
                }
            }
            break;
        case SEN_CONFIG_POWER_OFF:
            if (power_on_status) {
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
        case SEN_CONFIG_RESET_HOLD:
            ret = imx286dual_do_hw_reset(si, CTL_RESET_HOLD, data->mode);
            break;
        case SEN_CONFIG_RESET_RELEASE:
            ret = imx286dual_do_hw_reset(si, CTL_RESET_RELEASE, data->mode);
            break;
        default:
            cam_err("%s cfgtype(%d) is error", __func__, data->cfgtype);
            break;
    }
    return ret;
}

static hwsensor_vtbl_t s_imx286dual_vtbl =
{
    .get_name = imx286dual_get_name,
    .config = imx286dual_config,
    .power_up = imx286dual_power_up,
    .power_down = imx286dual_power_down,
    .match_id = imx286dual_match_id,
    .csi_enable = imx286dual_csi_enable,
    .csi_disable = imx286dual_csi_disable,
};
/* individual driver data for each device */
static sensor_t s_imx286dual_duke =
{
    .intf = { .vtbl = &s_imx286dual_vtbl, },//common intf
    .power_setting_array = {
        .size = ARRAY_SIZE(imx286dual_duke_power_setting),
        .power_setting = imx286dual_duke_power_setting,
     },
};

static const struct of_device_id
s_imx286dual_dt_match[] =
{
    {
        .compatible = "huawei,imx286dual_duke",
        .data = &s_imx286dual_duke.intf,
    },
    { } /* terminate list */
};

MODULE_DEVICE_TABLE(of, s_imx286dual_dt_match);
/* platform driver struct */
static int32_t imx286dual_platform_probe(struct platform_device* pdev);
static int32_t imx286dual_platform_remove(struct platform_device* pdev);

static struct platform_driver
s_imx286dual_driver =
{
    .probe = imx286dual_platform_probe,
    .remove = imx286dual_platform_remove,
    .driver =
    {
        .name = "huawei,imx286dual_duke",
        .owner = THIS_MODULE,
        .of_match_table = s_imx286dual_dt_match,
    },
};


static int32_t
imx286dual_platform_probe(
        struct platform_device* pdev)
{
    int rc = 0;
    struct device_node *np = NULL;
    const struct of_device_id *id = NULL;
    hwsensor_intf_t *intf = NULL;
    sensor_t *sensor = NULL;

    cam_info("enter %s ",__func__);

    if (NULL == pdev) {
        cam_err("%s pdev is NULL", __func__);
        return -ENODEV;
    }
    np = pdev->dev.of_node;
    if (NULL == np) {
        cam_err("%s of_node is NULL", __func__);
        return -ENODEV;
    }

    id = of_match_node(s_imx286dual_dt_match, np);
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
    rc = rpmsg_sensor_register(pdev, (void*)sensor);
    if (rc < 0) {
        hwsensor_unregister(intf);
        cam_err("%s rpmsg_sensor_register failed rc %d\n", __func__, rc);
        return -ENODEV;
    }

    return rc;
}

static int32_t
imx286dual_platform_remove(
    struct platform_device * pdev)
{
    struct device_node *np = NULL;
    const struct of_device_id *id = NULL;
    hwsensor_intf_t *intf = NULL;
    sensor_t *sensor = NULL;

    cam_info("enter %s",__func__);
    if (NULL == pdev) {
        cam_err("%s pdev is NULL", __func__);
        return 0;
    }
    np = pdev->dev.of_node;
    if (NULL == np) {
        cam_info("%s of_node is NULL", __func__);
        return 0;
    }
    /* don't use dev->p->driver_data
     * we need to search again */
    id = of_match_node(s_imx286dual_dt_match, np);
    if (!id) {
        cam_info("%s none id matched", __func__);
        return 0;
    }

    intf = (hwsensor_intf_t*)id->data;
    sensor = I2S(intf);

    rpmsg_sensor_unregister((void*)&sensor);
    hwsensor_unregister(intf);
    return 0;
}

static int __init
imx286dual_init_module(void)
{
    cam_info("enter %s",__func__);
    return platform_driver_probe(&s_imx286dual_driver,
            imx286dual_platform_probe);
}

static void __exit
imx286dual_exit_module(void)
{
    platform_driver_unregister(&s_imx286dual_driver);
}

module_init(imx286dual_init_module);
module_exit(imx286dual_exit_module);
MODULE_DESCRIPTION("imx286dual");
MODULE_LICENSE("GPL v2");
//lint -restore
