 

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/rpmsg.h>
#include <linux/pinctrl/consumer.h>

#include "hwsensor.h"
#include "sensor_commom.h"
#include "hw_csi.h"

#define I2S(i) container_of(i, sensor_t, intf)
#define POWER_DELAY_0        0//delay 0 ms
#define POWER_DELAY_1        1//delay 1 ms
#define POWER_DELAY_2        2//delay 2 ms
#define POWER_DELAY_5        5//delay 5 ms

static hwsensor_vtbl_t s_hi259b_vtbl;
static bool s_power_on_status = false;//false: power off, true:power on
static sensor_t *s_sensor = NULL;
static hwsensor_intf_t *s_intf = NULL;
struct mutex hi259b_power_lock;

/*lint -e826 -e31 -e485 -e785 -e731 -e846 -e514 -e866 -e30 -e84 -e838 -e64 -e528 -e753 -e749 -e715 -esym(826, 31, 485, 785, 731, 846, 514, 866, 30, 84, 838, 64, 528, 753, 749, 715*)*/
/*lint -save -e826 -e31 -e485 -e785 -e731 -e846 -e514 -e866 -e30 -e84 -e838 -e64 -e528 -e753 -e749 -e715 -specific(-e826 -e31 -e485 -e785 -e731 -e846 -e514 -e866 -e30 -e84 -e838 -e64 -e528 -e753 -e749 -e715)*/

struct sensor_power_setting hi259b_power_up_setting[] = {

    //MIPI SWITCH POWER ON
    {
        .seq_type = SENSOR_MIPI_LDO_EN,
        .data = (void*)"mipi_switch_en",
        .config_val = LDO_VOLTAGE_3PV,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //MIPI SWITCH PIPE SELECT, 1 FOR BACK CAMERA0, 0 FOR FRONT CAMERA3
    {
        .seq_type = SENSOR_MIPI_SW,
        .data = (void*)"mipi_switch_pipe_select",
        .config_val = SENSOR_GPIO_HIGH,//pull down
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //FRONT SLAVE CAM PWDN(1) STANDBY
    {
        .seq_type = SENSOR_PWDN2,
        .data = (void*)"front-slave-sensor-pwdn",
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //FRONT SlAVE CAM IOVDD&DVDD 1.80V
    {
        .seq_type = SENSOR_DVDD,
        .data = (void*)"front-slave-sensor-iovdd-dvdd",
        .config_val = LDO_VOLTAGE_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },
        
    //FRONT SlAVE CAM AVDD 2.80V
    {
        .seq_type = SENSOR_AVDD,
        .data = (void*)"front-slave-sensor-avdd",
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //MCLK
    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = 0,//set index 0 to use clk0
        .delay = POWER_DELAY_2,
    },

    //FRONT SLAVE CAM PWDN(0) NORMAL WORK
    {
        .seq_type = SENSOR_PWDN2,
        .data = (void*)"front-slave-sensor-pwdn",
        .config_val = SENSOR_GPIO_HIGH,//pull down
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_5,
    },

    //SCAM RST
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_LOW,//pull up
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },
};

/*
 *  1. Using for hi259b power down.
 *  2. Power sequence is configured from rear to front of the array, as the implementation
 * in func 'hw_sensor_power_down' for hi3660.
 *  3. GPIO can be configured as the plain meaning of macro, different from power-on seq.
 */
struct sensor_power_setting hi259b_power_down_setting[] = {
    //MIPI SWITCH POWER ON
    {
        .seq_type = SENSOR_MIPI_LDO_EN,
        .data = (void*)"mipi_switch_en",
        .config_val = LDO_VOLTAGE_3PV,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //MIPI SWITCH PIPE SELECT, 1 FOR BACK CAMERA0, 0 FOR FRONT CAMERA3
    {
        .seq_type = SENSOR_MIPI_SW,
        .data = (void*)"mipi_switch_pipe_select",
        .config_val = SENSOR_GPIO_LOW,//pull up
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //FRONT SLAVE CAM PWDN(1) STANDBY
    {
        .seq_type = SENSOR_PWDN2,
        .data = (void*)"front-slave-sensor-pwdn",
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //FRONT SlAVE CAM IOVDD&DVDD 1.80V
    {
        .seq_type = SENSOR_DVDD,
        .data = (void*)"front-slave-sensor-iovdd-dvdd",
        .config_val = LDO_VOLTAGE_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //FRONT SlAVE CAM AVDD 2.80V
    {
        .seq_type = SENSOR_AVDD,
        .data = (void*)"front-slave-sensor-avdd",
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //FRONT SLAVE CAM PWDN(0) NORMAL WORK
    {
        .seq_type = SENSOR_PWDN2,
        .data = (void*)"front-slave-sensor-pwdn",
        .config_val = SENSOR_GPIO_HIGH,//pull down
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_5,
    },

    //MCLK
    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = 0,//set index 0 to use clk0
        .delay = POWER_DELAY_1,
    },

    //SCAM RST
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_LOW,//pull up
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },
};

struct sensor_power_setting hi259b_power_up_setting_v3[] = {

    //MIPI SWITCH POWER ON
    {
        .seq_type = SENSOR_MIPI_LDO_EN,
        .data = (void*)"mipi_switch_en",
        .config_val = LDO_VOLTAGE_3PV,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //MIPI SWITCH PIPE SELECT, 1 FOR BACK CAMERA0, 0 FOR FRONT CAMERA3
    {
        .seq_type = SENSOR_MIPI_SW,
        .data = (void*)"mipi_switch_pipe_select",
        .config_val = SENSOR_GPIO_HIGH,//pull down
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //FRONT SLAVE CAM PWDN(1) STANDBY
    {
        .seq_type = SENSOR_PWDN2,
        .data = (void*)"front-slave-sensor-pwdn",
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //FRONT SlAVE CAM IOVDD&DVDD 1.80V
    {
        .seq_type = SENSOR_DVDD,
        .data = (void*)"front-slave-sensor-iovdd-dvdd",
        .config_val = LDO_VOLTAGE_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },
        
    //FRONT SlAVE CAM AVDD 2.80V [GPIO-015]
    {
        .seq_type = SENSOR_AVDD2_EN,
        .data = (void*)"front-slave-sensor-avdd",
        .config_val = SENSOR_GPIO_LOW,//pull high
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //MCLK
    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = 0,//set index 0 to use clk0
        .delay = POWER_DELAY_2,
    },

    //FRONT SLAVE CAM PWDN(0) NORMAL WORK
    {
        .seq_type = SENSOR_PWDN2,
        .data = (void*)"front-slave-sensor-pwdn",
        .config_val = SENSOR_GPIO_HIGH,//pull down
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_5,
    },

    //SCAM RST
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_LOW,//pull up
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },
};

/* Using for hi259b v3 power down */
struct sensor_power_setting hi259b_power_down_setting_v3[] = {

    //MIPI SWITCH POWER ON
    {
        .seq_type = SENSOR_MIPI_LDO_EN,
        .data = (void*)"mipi_switch_en",
        .config_val = LDO_VOLTAGE_3PV,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //MIPI SWITCH PIPE SELECT, 1 FOR BACK CAMERA0, 0 FOR FRONT CAMERA3
    {
        .seq_type = SENSOR_MIPI_SW,
        .data = (void*)"mipi_switch_pipe_select",
        .config_val = SENSOR_GPIO_LOW,//pull down
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //FRONT SLAVE CAM PWDN(1) STANDBY
    {
        .seq_type = SENSOR_PWDN2,
        .data = (void*)"front-slave-sensor-pwdn",
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //FRONT SlAVE CAM IOVDD&DVDD 1.80V
    {
        .seq_type = SENSOR_DVDD,
        .data = (void*)"front-slave-sensor-iovdd-dvdd",
        .config_val = LDO_VOLTAGE_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //FRONT SlAVE CAM AVDD 2.80V [GPIO-015]
    {
        .seq_type = SENSOR_AVDD2_EN,
        .data = (void*)"front-slave-sensor-avdd",
        .config_val = SENSOR_GPIO_LOW,//pull high
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },

    //FRONT SLAVE CAM PWDN(0) NORMAL WORK
    {
        .seq_type = SENSOR_PWDN2,
        .data = (void*)"front-slave-sensor-pwdn",
        .config_val = SENSOR_GPIO_HIGH,//pull down
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_5,
    },

    //MCLK
    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = 0,//set index 0 to use clk0
        .delay = POWER_DELAY_2,
    },

    //SCAM RST
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_LOW,//pull up
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = POWER_DELAY_1,
    },
};


atomic_t volatile hi259b_powered = ATOMIC_INIT(0);
static sensor_t s_hi259b =
{
    .intf = { .vtbl = &s_hi259b_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(hi259b_power_up_setting),
            .power_setting = hi259b_power_up_setting,
     },
    .power_down_setting_array = {
            .size = ARRAY_SIZE(hi259b_power_down_setting),
            .power_setting = hi259b_power_down_setting,
     },

    .p_atpowercnt = &hi259b_powered,
};


static sensor_t s_hi259b_v3 =
{
    .intf = { .vtbl = &s_hi259b_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(hi259b_power_up_setting_v3),
            .power_setting = hi259b_power_up_setting_v3,
     },
    .power_down_setting_array = {
            .size = ARRAY_SIZE(hi259b_power_down_setting_v3),
            .power_setting = hi259b_power_down_setting_v3,
     },

    .p_atpowercnt = &hi259b_powered,
};

static const struct of_device_id s_hi259b_dt_match[] =
{
    {
        .compatible = "huawei,hwi_front_cam3_h",
        .data = &s_hi259b.intf,
    },
    {
        .compatible = "huawei,hwi_front_cam3_h_v3",
        .data = &s_hi259b_v3.intf,
    },
    {
    },
};

MODULE_DEVICE_TABLE(of, s_hi259b_dt_match);

static struct platform_driver s_hi259b_driver =
{
    .driver =
    {
        .name = "huawei,hwi_front_cam3_h",
        .owner = THIS_MODULE,
        .of_match_table = s_hi259b_dt_match,
    },
};

char const* hi259b_get_name(hwsensor_intf_t* si)
{
    sensor_t* sensor = NULL;
    if (NULL == si) {
        cam_err("%s. si is NULL.", __func__);
        return NULL;
    }

    sensor = I2S(si);
    if (NULL == sensor || NULL == sensor->board_info || NULL == sensor->board_info->name) {
        cam_err("%s. sensor or board_info->name is NULL.", __func__);
        return NULL;
    }
    return sensor->board_info->name;
}

int hi259b_power_up(hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;

    if (NULL == si) {
        cam_err("%s. si is NULL.", __func__);
        return -EINVAL;
    }
    sensor = I2S(si);
    if (NULL == sensor || NULL == sensor->board_info || NULL == sensor->board_info->name) {
        cam_err("%s. sensor or board_info->name is NULL.", __func__);
        return -EINVAL;
    }
    cam_info("enter %s. index = %d name = %s", __func__, sensor->board_info->sensor_index, sensor->board_info->name);

    if (hw_is_fpga_board()) {
        ret = do_sensor_power_on(sensor->board_info->sensor_index, sensor->board_info->name);
    } else {
        ret = hw_sensor_power_up(sensor);
    }
    if (0 == ret ) {
        cam_info("%s. power up sensor success.", __func__);
    } else {
        cam_err("%s. power up sensor fail.", __func__);
    }
    
    return ret;
}

int hi259b_power_down(hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;

    if (NULL == si) {
        cam_err("%s. si is NULL.", __func__);
        return -EINVAL;
    }

    sensor = I2S(si);
    if (NULL == sensor || NULL == sensor->board_info || NULL == sensor->board_info->name) {
        cam_err("%s. sensor or board_info->name is NULL.", __func__);
        return -EINVAL;
    }
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

int hi259b_csi_enable(hwsensor_intf_t* si)
{
    return 0;
}

int hi259b_csi_disable(hwsensor_intf_t* si)
{
    return 0;
}

static int hi259b_match_id(
        hwsensor_intf_t* si, void * data)
{
    sensor_t* sensor = NULL;
    struct sensor_cfg_data *cdata = NULL;
    
    if(NULL == si || NULL == data) {
        cam_err("%s. si or data is NULL.", __func__);
        return -EINVAL;
    }

    sensor = I2S(si);
    if (NULL == sensor || NULL == sensor->board_info || NULL == sensor->board_info->name) {
        cam_err("%s. sensor or board_info is NULL.", __func__);
        return -EINVAL;
    }
    cdata = (struct sensor_cfg_data *)data;
    cdata->data = sensor->board_info->sensor_index;
    cam_info("%s name:%s", __func__, sensor->board_info->name);

    return 0;
}

int hi259b_config(hwsensor_intf_t* si, void  *argp)
{
    struct sensor_cfg_data *data;
    int ret =0;

    if (NULL == si || NULL == argp){
        cam_err("%s : si or argp is null", __func__);
        return -EINVAL;
    }

    data = (struct sensor_cfg_data *)argp;
    cam_debug("hi259b cfgtype = %d",data->cfgtype);
    if (NULL == si->vtbl) {
        cam_err("%s :  si->vtbl is null", __func__);
        return -EINVAL;
    }
    switch(data->cfgtype){
        case SEN_CONFIG_POWER_ON:
            mutex_lock(&hi259b_power_lock);
            if(!s_power_on_status){
                if(NULL == si->vtbl->power_up){
                    cam_err("%s. si->vtbl->power_up is null.", __func__);
                    ret=-EINVAL;
                }else{
                    ret = si->vtbl->power_up(si);
                    if (0 == ret) {
                        s_power_on_status = true;
                    } else {
                        cam_err("%s. power up fail.", __func__);
                    }
                }
            }
            /*lint -e455 -esym(455,*)*/
            mutex_unlock(&hi259b_power_lock);
            /*lint -e455 +esym(455,*)*/
            break;
        case SEN_CONFIG_POWER_OFF:
            mutex_lock(&hi259b_power_lock);
            if(true == s_power_on_status){
                if(NULL == si->vtbl->power_down){
                    cam_err("%s. si->vtbl->power_down is null.", __func__);
                    ret=-EINVAL;
                }else{
                    ret = si->vtbl->power_down(si);
                    if (0 == ret) {
                        s_power_on_status = false;
                    } else
                        cam_err("%s. power_down fail.", __func__);
                }
            }
            /*lint -e455 -esym(455,*)*/
            mutex_unlock(&hi259b_power_lock);
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
            if(NULL == si->vtbl->match_id){
                cam_err("%s. si->vtbl->match_id is null.", __func__);
                ret=-EINVAL;
            }else{
                ret = si->vtbl->match_id(si,argp);
            }
            break;
        default:
            cam_err("%s cfgtype(%d) is error", __func__, data->cfgtype);
            break;
    }

    return ret;
}

static hwsensor_vtbl_t s_hi259b_vtbl =
{
    .get_name = hi259b_get_name,
    .config = hi259b_config,
    .power_up = hi259b_power_up,
    .power_down = hi259b_power_down,
    .match_id = hi259b_match_id,
    .csi_enable = hi259b_csi_enable,
    .csi_disable = hi259b_csi_disable,
};

static int32_t hi259b_platform_probe(struct platform_device* pdev)
{
    int rc = 0;
    const struct of_device_id *id = NULL;
    hwsensor_intf_t *intf = NULL;
    sensor_t *sensor = NULL;
    struct device_node *np = NULL;
    
    cam_notice("enter %s",__func__);

    if (NULL == pdev) {
        cam_err("%s pdev is null.\n", __func__);
        return -EINVAL;
    }

    mutex_init(&hi259b_power_lock);
    np = pdev->dev.of_node;
    if (NULL == np) {
        cam_err("%s of_node is NULL", __func__);
        return -ENODEV;
    }

    id = of_match_node(s_hi259b_dt_match, np);
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
    if(NULL == sensor) {
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

int __init hi259b_init_module(void)
{
    cam_info("Enter: %s", __func__);
    return platform_driver_probe(&s_hi259b_driver,
            hi259b_platform_probe);
}

void __exit hi259b_exit_module(void)
{
    if( NULL != s_sensor) {
        rpmsg_sensor_unregister((void*)s_sensor);
        s_sensor = NULL;
    }
    if (NULL != s_intf) {
        hwsensor_unregister(s_intf);
        s_intf = NULL;
    }
    platform_driver_unregister(&s_hi259b_driver);
}
/*lint -restore*/

/*lint -e826 -e31 -e485 -e785 -e731 -e846 -e514 -e866 -e30 -e84 -e838 -e64 -e528 -e753 -e749 +esym(826, 31, 485, 785, 731, 846, 514, 866, 30, 84, 838, 64, 528, 753, 749, 715*)*/
/*lint -e528 -esym(528,*)*/
module_init(hi259b_init_module);
module_exit(hi259b_exit_module);
/*lint -e528 +esym(528,*)*/
/*lint -e753 -esym(753,*)*/
MODULE_DESCRIPTION("hi259b");
MODULE_LICENSE("GPL v2");
/*lint -e753 +esym(753,*)*/
