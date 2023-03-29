


#include "vl53l0.h"
#include "../pmic/hw_pmic.h"
#include "laser_common.h"
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif

//lint -save -e838 -e732 -e747 -e713 -e826 -e715 -e785  -e774 -esym(753,*)
//lint -save -e578 -e438 -e30 -e142 -e64 -esym(528,*)
static hw_vl53l0_t *s_laser = NULL;

#define SD2Laser(sd) container_of((sd), hw_vl53l0_t, subdev)
#define VAL_IOVDD             1800000
#define POWER_SETTINGS_LENGTH (3)
#define DT_LDO_NAME           "huawei,ldo-names"
static const laser_power_settings_t laser_power_supply_checklist[POWER_SETTINGS_LENGTH] =
{
	{LDO_AVDD,	 "avdd",	LDO_AVDD_2P8V },
	{LDO_IOVDD,	"iovdd",	LDO_IOVDD_1P8V},
	{LDO_XSHUT,	"xshut",	LDO_XSHUT_1P8V},
};

extern struct hisi_pmic_ctrl_t ncp6925_ctrl;
extern void hw_camdrv_msleep(unsigned int ms);


int vl53l0_duke_get_dt_data(struct platform_device *pdev, hw_vl53l0_t *laser)
{
	struct device_node *of_node = NULL;
	struct hw_laser_info *laser_info = NULL;
	int rc = 0;
	char *gpio_tag = NULL;
	const char *gpio_ctrl_types[2] = {"xshut","iovdd"};
	int index = 0;
	int i = 0;

	if ((NULL == pdev) || (NULL == laser)) {
		cam_err("%s pdev or laser is NULL", __func__);
		return -EINVAL;
	}
	of_node = pdev->dev.of_node;
	laser_info = kzalloc(sizeof(struct hw_laser_info), GFP_KERNEL);
	if (!laser_info) {
		cam_err("%s failed %d", __func__, __LINE__);
		return -ENOMEM;
	}
	laser->laser_info = laser_info;

	rc = of_property_read_string(of_node, "product_name", &laser_info->product_name);
	cam_info("%s product-name %s, rc %d", __func__, laser_info->product_name, rc);
	if (rc < 0) {
		cam_err("%s failed %d", __func__, __LINE__);
		goto fail;
	}

	rc = of_property_read_string(of_node, "huawei,laser_name", &laser_info->laser_name);
	cam_info("%s laser-name %s, rc %d", __func__, laser_info->laser_name, rc);
	if (rc < 0) {
		cam_err("%s failed %d", __func__, __LINE__);
		goto fail;
	}

	rc = of_property_read_u32(of_node, "huawei,i2c_idx", &laser_info->i2c_index);
	cam_info("%s huawei,i2c_idx 0x%x, rc %d", __func__, laser_info->i2c_index, rc);
	if (rc < 0) {
		cam_err("%s failed %d", __func__, __LINE__);
		goto fail;
	}

	//check ldo supply
	laser_info->ldo_num = of_property_count_strings(of_node, DT_LDO_NAME);
	if(LASER_SUPPLY_LDO_NO_USE < laser_info->ldo_num) {
		cam_info("%s use ldo to supply laser ldo num %d", __func__, laser_info->ldo_num);
		if(LDO_MAX < laser_info->ldo_num) {
			cam_err("%s failed %d, LDO_MAX(%d) < ldo_num(%d)", __func__, __LINE__,
				LDO_MAX, laser_info->ldo_num);
			laser_info->ldo_num = LASER_SUPPLY_LDO_NO_USE; //set default ldo num 0, before error handling
			rc = -EINVAL;
			goto fail;
		}

		for (i = 0; i < laser_info->ldo_num; i++) {
			rc = of_property_read_string_index(of_node, DT_LDO_NAME,
						i, &laser_info->ldo[i].supply);
			cam_info("%s huawei,ldo-names index:%d, ldo_name:%s, rc:%d", __func__,
						i, laser_info->ldo[i].supply, rc);
			if(rc < 0) {
				cam_err("%s failed %d index %d", __func__, __LINE__, i);
				goto fail;
			}
		}

		rc = devm_regulator_bulk_get(&(pdev->dev), laser_info->ldo_num, laser_info->ldo);
		if (rc < 0) {
			cam_err("%s failed %d\n", __func__, __LINE__);
			goto fail;
		}
	} else {
		laser_info->ldo_num = LASER_SUPPLY_LDO_NO_USE;
		cam_info("%s not use ldo to supply laser",__func__);
	}

	laser_info->gpio_num = of_gpio_count(of_node);
	if(laser_info->gpio_num < 0 ) {
		cam_err("%s failed %d, ret is %d", __func__, __LINE__, laser_info->gpio_num);
		goto fail;
	}
	cam_info("laser gpio num = %d", laser_info->gpio_num);

	for (index = 0;index < laser_info->gpio_num;index++)
	{
		rc = of_property_read_string_index(of_node, "huawei,gpio-ctrl-types", index, (const char **)&gpio_tag);
		if(rc < 0) {
			cam_err("%s failed %d", __func__, __LINE__);
			goto fail;
		}
		for (i = 0; i < IO_MAX; i++)
		{
			if (!strcmp(gpio_ctrl_types[i], gpio_tag)) {
				laser_info->laser_gpio[i].gpio = of_get_gpio(of_node, index);
			}
		}
		cam_info("gpio ctrl types: %s gpio = %d", gpio_tag, laser_info->laser_gpio[index].gpio);
	}

	laser_info->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(laser_info->pinctrl)) {
		cam_err("could not get pinctrl");
		goto end;
	}

	laser_info->pins_default = pinctrl_lookup_state(laser_info->pinctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR(laser_info->pins_default)) {
		cam_err("could not get default pinstate");
	}


	laser_info->pins_idle = pinctrl_lookup_state(laser_info->pinctrl, PINCTRL_STATE_IDLE);
	if (IS_ERR(laser_info->pins_idle)) {
		cam_err("could not get idle pinstate");
	}

end:
	return rc;

fail:
	cam_err("%s can not read laser info exit.", __func__);
	kfree(laser_info);
	laser_info = NULL;
	laser->laser_info = NULL;
	return rc;
}

static long
hwlaser_subdev_config(
	hw_vl53l0_t* s,
	hwlaser_config_data_t* data)
{
	long rc = -EINVAL;

	if ((NULL == s) || (NULL == data)) {
		cam_err("%s s or data is NULL", __func__);
		return -EINVAL;
	}
	if (NULL == s->laser_info) {
		cam_err("%s s->laser_info is NULL", __func__);
		return -EINVAL;
	}
	switch (data->cfgtype)
	{
		case HWCAM_LASER_CMD:
			HWCAM_CFG_INFO("not support");
			break;

		case HWCAM_LASER_POWERON:
			if (!IS_ERR(s->laser_info->pins_default)) {
				rc = pinctrl_select_state(s->laser_info->pinctrl, s->laser_info->pins_default);
				if (rc < 0) {
					cam_err("could not set default pins");
				}
			}
			mutex_lock(&s->lock);
			if (LASER_SUPPLY_LDO_NO_USE != s->laser_info->ldo_num) {
				rc = laser_ldo_config(s, LDO_AVDD, LDO_POWER_ON,
					laser_power_supply_checklist,
					POWER_SETTINGS_LENGTH);
				if(rc < 0) {
					cam_err("laser ldo config fail");
					mutex_unlock(&s->lock);
					return rc;
				}
			}

			HWCAM_CFG_INFO("laser xshut power up");
			rc = gpio_direction_output(s->laser_info->laser_gpio[XSHUT].gpio, HIGH);
			mutex_unlock(&s->lock);

			if (rc < 0) {
				cam_err("%s failed output pin. gpio = %x", __func__,
						s->laser_info->laser_gpio[XSHUT].gpio);
				return rc;
			}

			hw_camdrv_msleep(1);
			break;

		case HWCAM_LASER_POWEROFF:
			mutex_lock(&s->lock);
			HWCAM_CFG_INFO("laser xshut power down");
			rc = gpio_direction_output(s->laser_info->laser_gpio[XSHUT].gpio, LOW);

			if (rc < 0) {
				cam_err("%s failed output pin. gpio = %x", __func__,
						s->laser_info->laser_gpio[XSHUT].gpio);
				mutex_unlock(&s->lock);
				return rc;
			}

			if (LASER_SUPPLY_LDO_NO_USE != s->laser_info->ldo_num) {
				rc = laser_ldo_config(s, LDO_AVDD, LDO_POWER_OFF,
					laser_power_supply_checklist,
					POWER_SETTINGS_LENGTH);
				if(rc < 0) {
					cam_err("laser ldo config fail");
					mutex_unlock(&s->lock);
					return rc;
				}
			}
			mutex_unlock(&s->lock);
			if (!IS_ERR(s->laser_info->pins_idle)) {
				rc = pinctrl_select_state(s->laser_info->pinctrl, s->laser_info->pins_idle);
				if (rc < 0) {
					cam_err("could not set idle pins");
				}
			}

			break;

		case HWCAM_LASER_POWERON_EXT:
			HWCAM_CFG_INFO("laser iovdd power up");
			if (0 == s->laser_info->laser_gpio[IOVDD].gpio) {
				//use pmic
				if (ncp6925_ctrl.func_tbl->pmic_seq_config) {
					HWCAM_CFG_INFO("laser power on ext");
					rc = ncp6925_ctrl.func_tbl->pmic_seq_config(&ncp6925_ctrl,
								VOUT_LDO_1, VAL_IOVDD, 1);
					if (rc < 0) {
						cam_err("%s pmic_seq_config failed ", __func__);
					}
				}
			} else { //use gpio
				rc = gpio_request(s->laser_info->laser_gpio[IOVDD].gpio, "vl53l0");
				if (rc < 0) {
					cam_err("%s failed to request iovdd pin. gpio = %x", __func__,
						s->laser_info->laser_gpio[IOVDD].gpio);
					return -EIO;
				}

				rc = gpio_direction_output(s->laser_info->laser_gpio[IOVDD].gpio, HIGH);
				hw_camdrv_msleep(1);
				if (rc < 0) {
					cam_err("%s failed output pin. gpio = %x", __func__,
							s->laser_info->laser_gpio[IOVDD].gpio);
				}
				gpio_free(s->laser_info->laser_gpio[IOVDD].gpio);
			}
			break;

		case HWCAM_LASER_POWEROFF_EXT:
			HWCAM_CFG_INFO("laser iovdd power down");
			if (0 == s->laser_info->laser_gpio[IOVDD].gpio) {
				//use pmic
				if (ncp6925_ctrl.func_tbl->pmic_seq_config) {
					HWCAM_CFG_INFO("laser power off ext");
					rc = ncp6925_ctrl.func_tbl->pmic_seq_config(&ncp6925_ctrl,
								VOUT_LDO_1, VAL_IOVDD, 0);
					if (rc < 0) {
						cam_err("%s pmic_seq_config failed ", __func__);
					}
				}
			} else {
				rc = gpio_request(s->laser_info->laser_gpio[IOVDD].gpio, "vl53l0");
				if (rc < 0) {
					cam_err("%s failed to request iovdd pin. gpio = %x", __func__,
						s->laser_info->laser_gpio[IOVDD].gpio);
					return -EIO;
				}
				rc = gpio_direction_output(s->laser_info->laser_gpio[IOVDD].gpio, LOW);
				if (rc < 0) {
					cam_err("%s failed output pin. gpio = %x", __func__,
							s->laser_info->laser_gpio[IOVDD].gpio);
				}
				gpio_free(s->laser_info->laser_gpio[IOVDD].gpio);
			}

			break;

		case HWCAM_LASER_SET_FLAG:
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
			if(data->data == 1) {
				set_hw_dev_flag(DEV_I2C_LASER);
			}
#endif
			break;

		default:
			HWCAM_CFG_INFO("invalid cfgtype(%d)! ", data->cfgtype);
			break;
	}
	return rc;
}

static long
hwlaser_subdev_get_info(
	hw_vl53l0_t* s,
	hwlaser_info_t* info)
{
    if ((NULL == s) || (NULL == s->laser_info) || (NULL == s->laser_info->product_name) || (NULL == s->laser_info->laser_name) || (NULL == info) ) {
        cam_err("%s s or info is NULL", __func__);
        return -EINVAL;
    }
    strncpy(info->product_name, s->laser_info->product_name, HWLASER_NAME_SIZE - 1);
    info->product_name[HWLASER_NAME_SIZE - 1] = '\0';
    strncpy(info->name, s->laser_info->laser_name, HWLASER_NAME_SIZE - 1);
    info->name[HWLASER_NAME_SIZE - 1] = '\0';
    info->i2c_idx = s->laser_info->i2c_index;
    return 0;
}

static long
hw_laser_subdev_ioctl(
	struct v4l2_subdev *sd,
	unsigned int cmd,
	void *arg)
{
	long rc = -EINVAL;
	hw_vl53l0_t* s = NULL;

	if (NULL == sd) {
		cam_err("%s, sd for duke laser is NULL \n", __func__);
		return -EINVAL;
	}

	if (NULL == arg) {
		cam_err("%s, arg for duke laser is NULL \n", __func__);
		return -EINVAL;
	}

	s = SD2Laser(sd);
	cam_info("hw laser cmd = %x",cmd);

	switch (cmd)
	{
		case HWLASER_IOCTL_CONFIG:
			rc =hwlaser_subdev_config(s,arg);
			break;

		case HWLASER_IOCTL_GET_INFO:
			rc = hwlaser_subdev_get_info(s, arg);
			break;

		default:
			cam_err("%s, invalid IOCTL CMD(%d)! ", __func__, cmd);
			break;
	}

	return rc;
}

static int hw_laser_subdev_open(
	struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	cam_notice("hw_laser_sbudev open! ");
	return 0;
}

static int
hw_laser_subdev_close(
	struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	cam_notice("hw_laser_sbudev close! ");
	return 0;
}

static struct v4l2_subdev_internal_ops
s_subdev_internal_ops_hw_laser =
{
	.open = hw_laser_subdev_open,
	.close = hw_laser_subdev_close,
};

static struct v4l2_subdev_core_ops
s_subdev_core_ops_hw_laser =
{
	.ioctl = hw_laser_subdev_ioctl,
};

static struct v4l2_subdev_ops
s_subdev_ops_hw_laser =
{
	.core = &s_subdev_core_ops_hw_laser,
};

static int32_t vl53l0_platform_probe(struct platform_device *pdev)
{
	int rc = 0;
	hw_vl53l0_t *laser = NULL;
	struct v4l2_subdev *subdev = NULL;

	if (NULL == pdev) {
		cam_err("%s pdev is NULL", __func__);
		return -EINVAL;
	}
	laser = kzalloc(sizeof(*laser), GFP_KERNEL);
	if (!laser) {
		cam_err("probe - can not alloc driver data");
		return -ENOMEM;
	}

	//get dt data
	if (pdev->dev.of_node) {
		rc = vl53l0_duke_get_dt_data(pdev, laser);
		if (rc < 0) {
			cam_err("%s failed line %d", __func__, __LINE__);
			goto vl53l0_probe_fail;
		}
	} else {
		cam_err("%s vl53l0 of_node is NULL.", __func__);
		rc = -ENOMEM;
		goto vl53l0_probe_fail;
	}

	//init gpio
	rc = gpio_request(laser->laser_info->laser_gpio[XSHUT].gpio, "vl53l0");
	if (rc < 0) {
		cam_err("%s failed to request reset pin. gpio = %x", __func__,laser->laser_info->laser_gpio[XSHUT].gpio);
		rc = -EIO;
		goto vl53l0_probe_fail;
	}
	gpio_direction_output(laser->laser_info->laser_gpio[XSHUT].gpio, LOW);

	//register subdev
	subdev = &laser->subdev;
	mutex_init(&laser->lock);

	v4l2_subdev_init(subdev, &s_subdev_ops_hw_laser);
	subdev->internal_ops = &s_subdev_internal_ops_hw_laser;
	snprintf(subdev->name, sizeof(subdev->name), "%s", laser->laser_info->laser_name);
	subdev->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	v4l2_set_subdevdata(subdev, pdev);

	init_subdev_media_entity(subdev,HWCAM_SUBDEV_LASER);
	hwcam_cfgdev_register_subdev(subdev,HWCAM_SUBDEV_LASER);
	cam_info("%s register end.", __func__);

	laser->pdev = pdev;
	s_laser = laser;
	return rc;

vl53l0_probe_fail:
	if (laser) {
		if (laser->laser_info) {
			kfree(laser->laser_info);
			laser->laser_info = NULL;
		}
		kfree(laser);
		laser = NULL;
	}

	return rc;
}

static const struct of_device_id hw_vl53l0_dt_match[] = {
	{.compatible = "huawei,vl53l0_duke"},
	{}
};

MODULE_DEVICE_TABLE(of, hw_vl53l0_dt_match);
static struct platform_driver  vl53l0_duke_platform_driver = {
	.driver = {
		.name = "vl53l0_duke",
		.owner = THIS_MODULE,
		.of_match_table = hw_vl53l0_dt_match,
	},
};

static int __init vl53l0_duke_mod_init(void)
{
	int rc = 0;
	cam_info(" Enter %s ", __func__);

	rc = platform_driver_probe(&vl53l0_duke_platform_driver, vl53l0_platform_probe);
	if (rc < 0) {
		cam_notice("%s platform_driver_probe error.", __func__);
	}
	return rc;
}

static void __exit vl53l0_duke_mod_exit(void)
{
	platform_driver_unregister(&vl53l0_duke_platform_driver);
	if (s_laser) {
		if (s_laser->laser_info) {
			gpio_free(s_laser->laser_info->laser_gpio[XSHUT].gpio);
			kfree(s_laser->laser_info);
			s_laser->laser_info = NULL;
		}
		kfree(s_laser);
		s_laser = NULL;
	}
}

module_init(vl53l0_duke_mod_init);
module_exit(vl53l0_duke_mod_exit);
MODULE_DESCRIPTION("VL53L0 LASER");
MODULE_LICENSE("GPL v2");
//lint -restore
