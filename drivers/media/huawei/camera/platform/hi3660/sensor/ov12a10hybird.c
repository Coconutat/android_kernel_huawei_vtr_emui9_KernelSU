


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
//lint -save -e514 -e30 -e84 -e64 -e650 -e737 -e31 -e64 -esym(528,*) -esym(753,*)
#define MASTER_SENSOR_INDEX  (0)
#define SLAVE_SENSOR_INDEX   (2)
#define DELAY_1MS            (1)
#define DELAY_0MS            (0)

#define I2S(i) container_of((i), sensor_t, intf)
static bool power_on_status = false; //false for power down, ture for power up


static struct sensor_power_setting ov12a10hybird_power_setting [] = {
    //M0 AVDD0  2.80V  [CAM_PMIC_LDO4]
    {
        .seq_type = SENSOR_PMIC,
        .seq_val = VOUT_LDO_4,
        .config_val = PMIC_2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M1 AVDD1  2.80V  [CAM_PMIC_LDO5]
    //OV20381 sensor only gets one AVDD for 2.8V, which is different from imx350 AVDD(2.8V) + AVDD(1.8V).
    {
        .seq_type = SENSOR_PMIC,
        .seq_val = VOUT_LDO_5,
        .config_val = PMIC_2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    // M0 (ov12a) DVDD 1.2V [CAM_PMIC_BUCK1]
    {
        .seq_type = SENSOR_PMIC,
        .seq_val = VOUT_BUCK_1,
        .config_val = PMIC_1P2V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    // M1 DVDD(ov20381) 1.1V [CAM_PMIC_BUCK2]
    {
        .seq_type = SENSOR_PMIC,
        .seq_val = VOUT_BUCK_2,
        .config_val = PMIC_1P1V,//modify to 1.1V according to ov FAE
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M0 + M1 VCM are supplied by enabling outside buck through ldo19 with 2.85V
    {
        .seq_type = SENSOR_VCM_AVDD,
        .config_val = LDO_VOLTAGE_V2P85V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M0 + M1  IOVDD  1.8V  [CAM_PMIC_LDO1]
    {
        .seq_type = SENSOR_PMIC,
        .seq_val = VOUT_LDO_1,
        .config_val = PMIC_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = MASTER_SENSOR_INDEX,
        .delay = DELAY_1MS,
    },

    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = SLAVE_SENSOR_INDEX,
        .delay = DELAY_1MS,
    },

    //M0 RESET  [GPIO_52]
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_1MS,
    },

    //M1 RESET  [GPIO_21]
    {
        .seq_type = SENSOR_RST2,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_1MS,
    },
};

static char const*
ov12a10hybird_get_name(
        hwsensor_intf_t* si)
{
    sensor_t* sensor = NULL;
    if(NULL == si)
    {
        cam_err("%s. si is NULL.", __func__);
        return NULL;
    }

    sensor = I2S(si);

    if (NULL == sensor->board_info) {
        cam_err("%s. sensor->board_info is NULL.", __func__);
        return NULL;
    }

    return sensor->board_info->name;
}

static int
ov12a10hybird_power_up(
        hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;
    if(NULL == si)
    {
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
ov12a10hybird_power_down(
        hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;
    if(NULL == si)
    {
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

static int ov12a10hybird_csi_enable(hwsensor_intf_t* si)
{
    return 0;
}

static int ov12a10hybird_csi_disable(hwsensor_intf_t* si)
{
    return 0;
}

static int
ov12a10hybird_match_id(
        hwsensor_intf_t* si, void * data)
{
    sensor_t* sensor = NULL;
    struct sensor_cfg_data *cdata = NULL;
    if(NULL == si || NULL == data)
    {
        cam_err("%s. si or data is NULL.", __func__);
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

    memset(cdata->cfg.name, 0, DEVICE_NAME_SIZE);
    strncpy(cdata->cfg.name, sensor->board_info->name, DEVICE_NAME_SIZE-1);
    cdata->data = sensor->board_info->sensor_index;

    return 0;
}

static int
ov12a10hybird_config(
        hwsensor_intf_t* si,
        void  *argp)
{
    struct sensor_cfg_data *data = NULL;
    int ret =0;

    if ((NULL == si) || (NULL == argp) || (NULL == si->vtbl)) {
        cam_err("%s : si, argp or si->vtbl is null", __func__);
        return -EINVAL;
    }

    data = (struct sensor_cfg_data *)argp;
    cam_debug("ov12a10hybird cfgtype = %d",data->cfgtype);
    switch(data->cfgtype){
        case SEN_CONFIG_POWER_ON:
            if (NULL == si->vtbl->power_up)
            {
                cam_err("%s. si power_up is null", __func__);
                return -EINVAL;
            }

            if (!power_on_status)
            {
                ret = si->vtbl->power_up(si);
                if (0 == ret)
                {
                    power_on_status = true;
                }
                else
                {
                    cam_err("%s. power up fail.", __func__);
                }
            }
            break;
        case SEN_CONFIG_POWER_OFF:
            if (NULL == si->vtbl->power_down)
            {
                cam_err("%s. si power_up is null", __func__);
                return -EINVAL;
            }

            if (power_on_status)
            {
                ret = si->vtbl->power_down(si);
                if (0 == ret)
                {
                    power_on_status = false;
                }
                else
                {
                    cam_err("%s. power down fail.", __func__);
                }
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
            if (NULL == si->vtbl->match_id)
            {
                cam_err("%s. si power_up is null", __func__);
                return -EINVAL;
            }

            ret = si->vtbl->match_id(si,argp);
            break;
        default:
            cam_err("%s cfgtype(%d) is error", __func__, data->cfgtype);
            break;
    }
    return ret;
}

static hwsensor_vtbl_t s_ov12a10hybird_vtbl =
{
    .get_name = ov12a10hybird_get_name,
    .config = ov12a10hybird_config,
    .power_up = ov12a10hybird_power_up,
    .power_down = ov12a10hybird_power_down,
    .match_id = ov12a10hybird_match_id,
    .csi_enable = ov12a10hybird_csi_enable,
    .csi_disable = ov12a10hybird_csi_disable,
};

static sensor_t s_ov12a10hybird =
{
    .intf = { .vtbl = &s_ov12a10hybird_vtbl, },
    .power_setting_array = {
        .size = ARRAY_SIZE(ov12a10hybird_power_setting),
        .power_setting = ov12a10hybird_power_setting,
    },
};

static const struct of_device_id
s_ov12a10hybird_dt_match[] =
{
    {
        .compatible = "huawei,ov12a10hybird_stf",
        .data = &s_ov12a10hybird.intf,
    },
    { } /* terminate list */
};
MODULE_DEVICE_TABLE(of, s_ov12a10hybird_dt_match);
/* platform driver struct */
static int32_t ov12a10hybird_platform_probe(struct platform_device* pdev);
static int32_t ov12a10hybird_platform_remove(struct platform_device* pdev);
static struct platform_driver
s_ov12a10hybird_driver =
{
    .probe = ov12a10hybird_platform_probe,
    .remove = ov12a10hybird_platform_remove,
    .driver =
    {
        .name = "huawei,ov12a10hybird",
        .owner = THIS_MODULE,
        .of_match_table = s_ov12a10hybird_dt_match,
    },
};

static int32_t
ov12a10hybird_platform_probe(
        struct platform_device* pdev)
{
    int rc = 0;
    cam_info("enter %s stf",__func__);

    if(NULL == pdev)
    {
        rc = -ENODEV;
        goto ov12a10hybird_sensor_probe_fail;
    }

    if (pdev->dev.of_node) {
        rc = hw_sensor_get_dt_data(pdev, &s_ov12a10hybird);
        if (rc < 0) {
            cam_err("%s get dt data fail.\n", __func__);
            goto ov12a10hybird_sensor_probe_fail;
        }
    } else {
        cam_err("%s ov12a10hybird of_node is NULL.\n", __func__);
        rc = -ENODEV;
        goto ov12a10hybird_sensor_probe_fail;
    }

    s_ov12a10hybird.dev = &pdev->dev;

    rc = hwsensor_register(pdev, &s_ov12a10hybird.intf);
    if (rc < 0)
    {
        cam_err("%s hwsensor_register fail.\n", __func__);
        goto ov12a10hybird_sensor_probe_fail;
    }

    rc = rpmsg_sensor_register(pdev, (void*)&s_ov12a10hybird);
    if (rc < 0)
    {
        cam_err("%s rpmsg_sensor_register fail.\n", __func__);
        hwsensor_unregister(&s_ov12a10hybird.intf);
        goto ov12a10hybird_sensor_probe_fail;
    }

ov12a10hybird_sensor_probe_fail:
    return rc;
}

static int32_t
ov12a10hybird_platform_remove(
    struct platform_device * pdev)
{
    rpmsg_sensor_unregister((void*)&s_ov12a10hybird);
    hwsensor_unregister(&s_ov12a10hybird.intf);
    return 0;
}
static int __init
ov12a10hybird_init_module(void)
{
    cam_info("enter %s",__func__);
    return platform_driver_probe(&s_ov12a10hybird_driver,
            ov12a10hybird_platform_probe);
}

static void __exit
ov12a10hybird_exit_module(void)
{
    platform_driver_unregister(&s_ov12a10hybird_driver);
}

module_init(ov12a10hybird_init_module);
module_exit(ov12a10hybird_exit_module);
MODULE_DESCRIPTION("ov12a10hybird");
MODULE_LICENSE("GPL v2");
//lint -restore
