
#include "laser_common.h"

//lint -save -e838 -e421

//use for LDO OUTPUT
int laser_ldo_config(hw_laser_t *s,
                    laser_ldo_t ldo_t,
                    power_ctrl_t power_state,
                    const laser_power_settings_t *power_setting,
                    int power_setting_num)
{
    char *supply_names = NULL;
    int i = 0;
    int rc = 0;
    int config_val = 0;//set default val 0

    if(NULL == s || NULL == power_setting) {
        cam_err("%s s or power_setting is NULL", __func__);
        return -EINVAL;
    }

    if(NULL == s->laser_info) {
        cam_err("%s laser_info is NULL", __func__);
        return -EINVAL;
    }

    if(s->laser_info->ldo_num <= LASER_SUPPLY_LDO_NO_USE || s->laser_info->ldo_num > LDO_MAX) {
        cam_info("%s not use ldo to supply laser ldo num %d", __func__, s->laser_info->ldo_num);
        return -EINVAL;
    }

    for(i = 0; i < power_setting_num; i++) {
        if(ldo_t == power_setting[i].ldo_t) {
            supply_names = power_setting[i].supply_names;
            config_val = power_setting[i].config_val;
            break;
        }
    }

    if(i >= power_setting_num) {
        cam_err("%s invalid ldo_t %d.", __func__, ldo_t);
        return -EINVAL;
    }

    for(i = 0; i < s->laser_info->ldo_num; i++) {
        if((NULL != s->laser_info->ldo[i].supply) && (NULL != supply_names)) {
            if(!strcmp(s->laser_info->ldo[i].supply, supply_names)) {
                break;
            }
        }
    }

    if(i >= s->laser_info->ldo_num) {
        cam_info("can not find specifical ldo, may not be configured in dts");
        return -EINVAL;
    }

    if(LDO_POWER_ON == power_state) {
        if(NULL == s->laser_info->ldo[i].consumer) {
            cam_info("consumer [%d] is NULL", i);
            return -EINVAL;
        }
        rc = regulator_set_voltage(s->laser_info->ldo[i].consumer, config_val, config_val);
        if(rc < 0) {
            cam_err("failed to set ldo[%s] to %d V, rc %d",s->laser_info->ldo[i].supply, config_val, rc);
            return rc;
        }

        rc = regulator_bulk_enable(LDO_ENABLE_NUMS, &s->laser_info->ldo[i]);
        if(rc < 0) {
            cam_err("failed to enable regulators %d", rc);
            return rc;
        }

        cam_info("success set laser ldo %s to %d", s->laser_info->ldo[i].supply, config_val);
    }
    else if(LDO_POWER_OFF == power_state) {
        rc = regulator_bulk_disable(LDO_ENABLE_NUMS, &s->laser_info->ldo[i]);
        if(rc < 0) {
            cam_err("failed to diable regulator %d, rc %d", i, rc);
            return rc;
        }
        cam_info("success disable laser ldo %s", s->laser_info->ldo[i].supply);
    }
    else {
        cam_err("%s power_ctrl_t error",__func__);
        return -EINVAL;
    }

    return rc;
}

//lint -restore
