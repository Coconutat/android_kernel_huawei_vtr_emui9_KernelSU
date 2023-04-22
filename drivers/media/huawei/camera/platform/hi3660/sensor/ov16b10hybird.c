


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
extern int strncpy_s(char *strDest, size_t destMax, const char *strSrc, size_t count);
struct mutex ov16b10hybird_power_lock;
extern int memset_s(void *dest, size_t destMax, int c, size_t count);
static hwsensor_intf_t *s_intf = NULL;
static sensor_t *s_sensor = NULL;

static struct sensor_power_setting ov16b10hybird_power_setting [] = {
    //MIPI SWITCH POWER ON [PMU-LDO17]
    {
        .seq_type = SENSOR_MIPI_LDO_EN,
        .config_val = LDO_VOLTAGE_3PV,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_1MS,
    },

    //MIPI switch to M0 [GPIO-044]
    {
        .seq_type = SENSOR_MIPI_SW,
        .config_val = SENSOR_GPIO_LOW,//high for M0
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_1MS,
    },

    //M0 AVDD0  2.80V  [PMU-LDO13]
    {
        .seq_type = SENSOR_AVDD0,
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M1 AVDD1  2.80V  [PMU-LDO31]
    {
        .seq_type = SENSOR_AVDD2,
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    // M0+M1 DVDD 1.1V [GPIO-036]
    {
        .seq_type = SENSOR_DVDD0_EN,
        .config_val = SENSOR_GPIO_LOW,//ENABLE
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M0 + M1 AFVDD 2.8V [PMU-LDO19]
    {
        .seq_type = SENSOR_VCM_AVDD,
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M0 + M1  IOVDD  1.8V  [PMU-LDO25]
    {
        .seq_type = SENSOR_DVDD,
        .config_val = LDO_VOLTAGE_1P8V,
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

static struct sensor_power_setting ov16b10hybird_power_setting_v3 [] = {
    //MIPI SWITCH POWER ON [PMU-LDO17]
    {
        .seq_type = SENSOR_MIPI_LDO_EN,
        .config_val = LDO_VOLTAGE_3PV,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_1MS,
    },

    //MIPI switch to M0 [GPIO-044]
    {
        .seq_type = SENSOR_MIPI_SW,
        .config_val = SENSOR_GPIO_LOW,//high for M0
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_1MS,
    },

    //M0 AVDD0  2.80V  [GPIO-015]
    {
        .seq_type = SENSOR_AVDD1_EN,
        .config_val = SENSOR_GPIO_LOW,//pull high
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M1 AVDD1  2.80V  [GPIO-011]
    {
        .seq_type = SENSOR_AVDD2_EN,
        .config_val = SENSOR_GPIO_LOW,//pull high
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    // M0+M1 DVDD 1.1V [GPIO-036]
    {
        .seq_type = SENSOR_DVDD0_EN,
        .config_val = SENSOR_GPIO_LOW,//ENABLE
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M0 + M1 AFVDD 2.8V [PMU-LDO19]
    {
        .seq_type = SENSOR_VCM_AVDD,
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M0 + M1  IOVDD  1.8V  [PMU-LDO25]
    {
        .seq_type = SENSOR_DVDD,
        .config_val = LDO_VOLTAGE_1P8V,
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

static struct sensor_power_setting ov16b10hybird_power_setting_vn1 [] = {
    //MIPI SWITCH POWER ON [PMU-LDO17]
    {
        .seq_type = SENSOR_MIPI_LDO_EN,
        .config_val = LDO_VOLTAGE_3PV,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_1MS,
    },

    //MIPI switch to M0 [GPIO-044]
    {
        .seq_type = SENSOR_MIPI_SW,
        .config_val = SENSOR_GPIO_LOW,//high for M0
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_1MS,
    },

    //M0 AVDD0  2.80V  [GPIO-015]
    {
        .seq_type = SENSOR_AVDD1_EN,
        .config_val = SENSOR_GPIO_LOW,//pull high
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M1 AVDD1  2.80V  [LDO31]
    {
        .seq_type = SENSOR_AVDD2,
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    // M0+M1 DVDD 1.1V [GPIO-036]
    {
        .seq_type = SENSOR_DVDD0_EN,
        .config_val = SENSOR_GPIO_LOW,//ENABLE
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M0 + M1 AFVDD 2.8V [PMU-LDO19]
    {
        .seq_type = SENSOR_VCM_AVDD,
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = DELAY_0MS,
    },

    //M0 + M1  IOVDD  1.8V  [PMU-LDO25]
    {
        .seq_type = SENSOR_DVDD,
        .config_val = LDO_VOLTAGE_1P8V,
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
ov16b10hybird_get_name(
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
ov16b10hybird_power_up(
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
ov16b10hybird_power_down(
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

static int ov16b10hybird_csi_enable(hwsensor_intf_t* si)
{
    return 0;
}

static int ov16b10hybird_csi_disable(hwsensor_intf_t* si)
{
    return 0;
}

static int
ov16b10hybird_match_id(
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

    memset_s(cdata->cfg.name, DEVICE_NAME_SIZE, 0, DEVICE_NAME_SIZE);
    strncpy_s(cdata->cfg.name, DEVICE_NAME_SIZE - 1, sensor->board_info->name, DEVICE_NAME_SIZE - 1);
    cdata->data = sensor->board_info->sensor_index;

    return 0;
}

static int
ov16b10hybird_config(
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
    cam_debug("ov16b10hybird cfgtype = %d",data->cfgtype);
    switch(data->cfgtype){
        case SEN_CONFIG_POWER_ON:
            mutex_lock(&ov16b10hybird_power_lock);
            if (NULL == si->vtbl->power_up)
            {
                cam_err("%s. si power_up is null", __func__);
                /*lint -e455 -esym(455,*)*/
                mutex_unlock(&ov16b10hybird_power_lock);
                /*lint -e455 +esym(455,*)*/
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
            /*lint -e455 -esym(455,*)*/
            mutex_unlock(&ov16b10hybird_power_lock);
            /*lint -e455 +esym(455,*)*/
            break;
        case SEN_CONFIG_POWER_OFF:
            mutex_lock(&ov16b10hybird_power_lock);
            if (NULL == si->vtbl->power_down)
            {
                cam_err("%s. si power_up is null", __func__);
                /*lint -e455 -esym(455,*)*/
                mutex_unlock(&ov16b10hybird_power_lock);
                /*lint -e455 +esym(455,*)*/
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
            /*lint -e455 -esym(455,*)*/
            mutex_unlock(&ov16b10hybird_power_lock);
            /*lint -e455 +esym(455,*)*/
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

static hwsensor_vtbl_t s_ov16b10hybird_vtbl =
{
    .get_name = ov16b10hybird_get_name,
    .config = ov16b10hybird_config,
    .power_up = ov16b10hybird_power_up,
    .power_down = ov16b10hybird_power_down,
    .match_id = ov16b10hybird_match_id,
    .csi_enable = ov16b10hybird_csi_enable,
    .csi_disable = ov16b10hybird_csi_disable,
};

static sensor_t s_ov16b10hybird =
{
    .intf = { .vtbl = &s_ov16b10hybird_vtbl, },
    .power_setting_array = {
        .size = ARRAY_SIZE(ov16b10hybird_power_setting),
        .power_setting = ov16b10hybird_power_setting,
    },
};

static sensor_t s_ov16b10hybird_v3 =
{
    .intf = { .vtbl = &s_ov16b10hybird_vtbl, },
    .power_setting_array = {
        .size = ARRAY_SIZE(ov16b10hybird_power_setting_v3),
        .power_setting = ov16b10hybird_power_setting_v3,
    },
};

static sensor_t s_ov16b10hybird_vn1 =
{
    .intf = { .vtbl = &s_ov16b10hybird_vtbl, },
    .power_setting_array = {
        .size = ARRAY_SIZE(ov16b10hybird_power_setting_vn1),
        .power_setting = ov16b10hybird_power_setting_vn1,
    },
};

static const struct of_device_id
s_ov16b10hybird_dt_match[] =
{
    {
        .compatible = "huawei,hwi_back_cam02_o",
        .data = &s_ov16b10hybird.intf,
    },
    {
        .compatible = "huawei,hwi_back_cam02_o_v3",
        .data = &s_ov16b10hybird_v3.intf,
    },
    {
        .compatible = "huawei,hwi_back_cam02_o_vn1",
        .data = &s_ov16b10hybird_vn1.intf,
    },

    { } /* terminate list */
};
MODULE_DEVICE_TABLE(of, s_ov16b10hybird_dt_match);
/* platform driver struct */
static struct platform_driver
s_ov16b10hybird_driver =
{
    .driver =
    {
        .name = "huawei,hwi_back_cam02_o",
        .owner = THIS_MODULE,
        .of_match_table = s_ov16b10hybird_dt_match,
    },
};

static int32_t
ov16b10hybird_platform_probe(
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

    mutex_init(&ov16b10hybird_power_lock);
    np = pdev->dev.of_node;
    if (NULL == np) {
        cam_err("%s of_node is NULL", __func__);
        return -ENODEV;
    }

    id = of_match_node(s_ov16b10hybird_dt_match, np);
    if (NULL == id) {
        cam_err("%s none id matched", __func__);
        return -ENODEV;
    }

    intf = (hwsensor_intf_t*)id->data;
    if (NULL == intf) {
        cam_err("%s intf is NULL", __func__);
        return -ENODEV;
    }

    sensor = I2S(intf);
    if(NULL == sensor){
        cam_err("%s sensor is NULL rc %d", __func__, rc);
        return -ENODEV;
    }
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
    s_sensor = sensor;

    return rc;
}

static int __init
ov16b10hybird_init_module(void)
{
    cam_info("enter %s",__func__);
    return platform_driver_probe(&s_ov16b10hybird_driver,
            ov16b10hybird_platform_probe);
}

static void __exit
ov16b10hybird_exit_module(void)
{
    if( NULL != s_sensor) {
        rpmsg_sensor_unregister((void*)s_sensor);
        s_sensor = NULL;
    }
    if (NULL != s_intf) {
        hwsensor_unregister(s_intf);
        s_intf = NULL;
    }
    platform_driver_unregister(&s_ov16b10hybird_driver);
}

module_init(ov16b10hybird_init_module);
module_exit(ov16b10hybird_exit_module);
MODULE_DESCRIPTION("ov16b10hybird");
MODULE_LICENSE("GPL v2");
//lint -restore
