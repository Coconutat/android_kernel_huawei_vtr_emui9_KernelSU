 /*
 * Hisilicon ISP Sensor Driver
 * Name : sensor.c
 * Copyright (c) 2018- Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/compiler.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <uapi/linux/histarisp.h>
#include <linux/platform_data/remoteproc-hisi.h>
#include "sensor_commom.h"

enum hisp_gpio_type_e {
    RESET = 0,
    POWERDOWN,
    DPHY_TXRXZ,
    DPHY_RSTZCAL,
    CAM_1V05_EN,
    CAM_1V2_EN,
    CAM_1V8_EN,
    CAM_2V85_EN,
    CAM_VCM_2V85_EN,
    CAM_VCM_POWER,
    MAX_HISP_GPIO
};

enum hisp_level_type_e {
    LOW = 0,
    HIGH,
    MAX_HISP_LEVEL
};

struct hisp_gpio_s {
    unsigned int type;
    unsigned int level;
};

struct hisp_sensor_info {
    struct device *dev;
    sensor_t *sensor;
    struct list_head link;
};

static LIST_HEAD(g_sinfo);

int __weak hisi_is_clt_flag(void)
{
    pr_err("[%s] Not Supported Now!\n", __func__);
    return 0;
}

int __weak hw_is_fpga_board(void)
{
    pr_err("[%s] Not Supported Now!\n", __func__);
    return 1;
}

int __weak hw_sensor_power_up_config(struct device *dev, hwsensor_board_info_t *sensor_info)
{
    pr_err("[%s] Not Supported Now!\n", __func__);
    return 0;
}

void __weak hw_sensor_power_down_config(hwsensor_board_info_t *sensor_info)
{
    pr_err("[%s] Not Supported Now!\n", __func__);
    return;
}

static int hw_sensor_gpio_config(struct hisp_sensor_info *hsi, struct hisp_gpio_s *gpio)
{
    hwsensor_board_info_t *bi = NULL;
    sensor_t *sensor = NULL;
    int ret = 0;

    if (!(sensor = hsi->sensor)) {
        pr_err("[%s] Failed : sensor.%pK\n", __func__, sensor);
        return -EINVAL;
    }

    if (!(bi = sensor->board_info)) {
        pr_err("[%s] Failed : board_info.%pK\n", __func__, sensor->board_info);
        return -EINVAL;
    }

    if (!gpio) {
        pr_err("[%s] Failed : gpio.%pK\n", __func__, gpio);
        return -EINVAL;
    }

    if (gpio->type >= MAX_HISP_GPIO || gpio->level >= MAX_HISP_LEVEL) {
        pr_err("[%s] Failed : Invalid gpio.(%d, %d)\n", __func__, gpio->type, gpio->level);
        return -EINVAL;
    }

    if (!(ret = hisi_is_clt_flag())) {
        pr_err("[%s] Failed : hisi_is_clt_flag.%d\n", __func__, ret);
        return 0;
    }

    if (!bi->gpios[gpio->type].gpio) {
        pr_err("[%s] Failed : GPIO.%d Not Actived\n", __func__, gpio->type);
        return -EINVAL;
    }

    if (!(ret = gpio_request(bi->gpios[gpio->type].gpio, NULL))) {
        pr_err("[%s] Failed : gpio_request.%d, type.%d\n", __func__, ret, gpio->type);
        return ret;
    }

    if (!(ret = gpio_direction_output(bi->gpios[gpio->type].gpio, gpio->level))) {
        pr_err("[%s] Failed : gpio_direction_output.%d, type.%d\n", __func__, ret, gpio->type);
        return ret;
    }

    gpio_free(bi->gpios[gpio->type].gpio);

    return 0;
}

static int check_sensor_name(int sensorid, const char *name, struct hisp_sensor_info **ret_hsi)
{
    struct hisp_sensor_info *hsi = NULL;
    hwsensor_board_info_t *bi = NULL;
    sensor_t *sensor = NULL;
    int i = 0;

    pr_info("[%s] sensorid.%d, name.%s +\n", __func__, sensorid, name);
    if (list_empty_careful(&g_sinfo)) {
        pr_err("[%s] Failed : Sensor Info List Empty\n", __func__);
        return -ENOMEM;
    }

    list_for_each_entry(hsi, &g_sinfo, link) {
        if (!hsi) {
            pr_err("[%s] Failed : hsi.%pK\n", __func__, hsi);
            return -EINVAL;
        }

        if (!(sensor = hsi->sensor)) {
            pr_err("[%s] Failed : sensor.%pK\n", __func__, sensor);
            return -EINVAL;
        }

        if (!(bi = sensor->board_info)) {
            pr_err("[%s] Failed : board_info.%pK\n", __func__, sensor->board_info);
            return -EINVAL;
        }

        if (!bi->name) {
            pr_err("[%s] Failed : name.%pK\n", __func__, bi->name);
            return -EINVAL;
        }

        pr_info("[%s][%d][(%s, %d) = (%s, %d)]\n", __func__, i ++, name, sensorid, bi->name, bi->sensor_index);
        if (!strncmp(bi->name, name, strlen(name)) && (bi->sensor_index == sensorid)) {
            pr_info("[%s] %s @ %d Found\n", __func__, bi->name, bi->sensor_index);
            *ret_hsi = hsi;
            return 0;
        }
    }
    pr_err("[%s] Failed : %s Not Found %d in All\n", __func__, name, i);

    return -EINVAL;
}

/*lint -save -e429*/
int rpmsg_sensor_register(struct platform_device *pdev, void *psensor)
{
    struct hisp_sensor_info *hsi = NULL;

    if (!pdev || !psensor) {
        pr_err("[%s] Failed : pdev.%pK, psensor.%pK\n", __func__, pdev, psensor);
        return -ENOMEM;
    }

    if ((hsi = (struct hisp_sensor_info *)kzalloc( sizeof(struct hisp_sensor_info), GFP_KERNEL)) == NULL) {
        pr_err("[%s] Failed : kzalloc.%pK\n", __func__, hsi);
        return -ENOMEM;
    }

    hsi->dev = &pdev->dev;
    hsi->sensor = (sensor_t *)psensor;
    list_add_tail(&hsi->link, &g_sinfo);

    return 0;
}

/*lint -restore */
void rpmsg_sensor_unregister(void *psensor)
{
    struct hisp_sensor_info *hsi = NULL;
    struct list_head *pos = NULL;

    if (!psensor) {
        pr_err("[%s] Failed : si.%pK\n", __func__, psensor);
        return;
    }

    if ((hsi = container_of(psensor, struct hisp_sensor_info, sensor)) == NULL) {
        pr_err("[%s] Failed : container_of.%pK\n", __func__, hsi);
        return;
    }

    list_for_each(pos, &g_sinfo) {
        if (pos == &hsi->link) {
            list_del(&hsi->link);
            kfree(hsi);
            return;
        }
    }
    pr_err("[%s] Failed : Can't find sensor!\n", __func__);

    return;
}

static int do_gpio_config_on(struct hisp_sensor_info *hsi)
{
    unsigned int i = 0;
    int ret = 0;
    struct hisp_gpio_s *gpio = NULL;
    struct hisp_gpio_s on_sequence[] = {
        {RESET,             HIGH},
        {POWERDOWN,         LOW},
        {CAM_VCM_POWER,     HIGH},
        {DPHY_TXRXZ,        LOW},
        {DPHY_RSTZCAL,      HIGH},
        {CAM_1V05_EN,       HIGH},
        {CAM_1V2_EN,        HIGH},
        {CAM_1V8_EN,        HIGH},
        {CAM_2V85_EN,       HIGH},
        {CAM_VCM_2V85_EN,   HIGH},
    };

    for (gpio = &on_sequence[0], i = 0; i < sizeof(on_sequence)/sizeof(on_sequence[0]); gpio ++, i ++) {
        if ((ret = hw_sensor_gpio_config(hsi, gpio)) != 0) {
            pr_err("[%s] Failed : hw_sensor_gpio_config.%d.(%pK, %pK)\n", __func__, ret, hsi, gpio);
            return ret;
        }
    }

    return 0;
}

static int do_gpio_config_off(struct hisp_sensor_info *hsi)
{
    unsigned int i = 0;
    int ret = 0;
    struct hisp_gpio_s *gpio = NULL;
    struct hisp_gpio_s off_sequence[] = {
        {CAM_VCM_POWER,     LOW},
        {POWERDOWN,         HIGH},
        {RESET,             LOW},
        {DPHY_TXRXZ,        HIGH},
        {DPHY_RSTZCAL,      LOW},
        {CAM_1V05_EN,       LOW},
        {CAM_1V2_EN,        LOW},
        {CAM_1V8_EN,        LOW},
        {CAM_2V85_EN,       LOW},
        {CAM_VCM_2V85_EN,   LOW},
    };

    for (gpio = &off_sequence[0], i = 0; i < sizeof(off_sequence)/sizeof(off_sequence[0]); gpio ++, i ++) {
        if ((ret = hw_sensor_gpio_config(hsi, gpio)) != 0) {
            pr_err("[%s] Failed : hw_sensor_gpio_config.%d, (%pK, %pK)\n", __func__, ret, hsi, gpio);
            return ret;
        }
    }

    return 0;
}

static int all_sensor_power_on(int index)
{
    struct hisp_sensor_info *hsi = NULL;
    hwsensor_board_info_t *bi = NULL;
    sensor_t *sensor = NULL;
    int ret = 0;

    if (list_empty_careful(&g_sinfo)) {
        pr_err("[%s] Failed : Sensor Info List Empty\n", __func__);
        return -ENOMEM;
    }

    list_for_each_entry(hsi, &g_sinfo, link) {
        if (!(sensor = hsi->sensor)) {
            pr_err("[%s] Failed : sensor.%pK\n", __func__, sensor);
            return -EINVAL;
        }

        if (!(bi = sensor->board_info)) {
            pr_err("[%s] Failed : board_info.%pK\n", __func__, sensor->board_info);
            return -EINVAL;
        }

        if (index != bi->sensor_index)
            continue;

        pr_info("[%s] %s@%d\n", __func__, bi->name, index);
        if ((ret = do_gpio_config_on(hsi)) != 0) {
            pr_err("[%s] Failed : do_gpio_config_on.%d, %s@%d\n", __func__, ret, bi->name, index);
            return ret;
        }
    }

    return 0;
}

static int all_sensor_power_off(int index)
{
    struct hisp_sensor_info *hsi = NULL;
    hwsensor_board_info_t *bi = NULL;
    sensor_t *sensor = NULL;
    int ret = 0;

    if (list_empty_careful(&g_sinfo)) {
        pr_err("[%s] Failed : Sensor Info List Empty\n", __func__);
        return -ENOMEM;
    }

    list_for_each_entry(hsi, &g_sinfo, link) {
        if (!(sensor = hsi->sensor)) {
            pr_err("[%s] Failed : sensor.%pK\n", __func__, sensor);
            return -EINVAL;
        }

        if (!(bi = sensor->board_info)) {
            pr_err("[%s] Failed : board_info.%pK\n", __func__, sensor->board_info);
            return -EINVAL;
        }

        if (index != bi->sensor_index)
            continue;

        pr_info("[%s] %s@%d\n", __func__, bi->name, index);
        if((ret = do_gpio_config_off(hsi)) != 0) {
            pr_err("[%s] Failed : do_gpio_config_off.%d, %s@%d\n", __func__, ret, bi->name, index);
            return ret;
        }
    }

    return 0;
}

int do_sensor_power_on(int index, const char *name)
{
    struct hisp_sensor_info *hsi = NULL;
    int ret = 0;

    pr_info("[%s] %s@%d\n", __func__, name, index);
    if ((ret = check_sensor_name(index, name, &hsi)) != 0) {
        pr_err("[%s] Failed : check_sensor_name.%d.(%d, %s, %pK)\n", __func__, ret, index, name, hsi);
        return ret;
    }

    if ((ret = hw_sensor_power_up_config(hsi->dev, hsi->sensor->board_info))) {
        pr_err("[%s] Failed : hw_sensor_power_up_config.%d\n", __func__, ret);
        return ret;
    }

    if (hw_is_fpga_board()) {
        if ((ret = do_gpio_config_on(hsi)) != 0)
            pr_err("[%s] Failed : do_gpio_config_on.%d, %s@%d\n", __func__, ret, name, index);
        return ret;
    }

    if ((ret = hw_sensor_power_up(hsi->sensor)) != 0)
        pr_err("[%s] Failed : hw_sensor_power_up.%d, %s@%d\n", __func__, ret, name, index);

    return 0;
}

int do_sensor_power_off(int index, const char *name)
{
    struct hisp_sensor_info *hsi = NULL;
    int ret = 0;

    pr_info("[%s] %s@%d\n", __func__, name, index);
    if ((ret = check_sensor_name(index, name, &hsi)) != 0) {
        pr_err("[%s] Failed : check_sensor_name.%d.(%d, %s, %pK)\n", __func__, ret, index, name, hsi);
        return ret;
    }

    if (hw_is_fpga_board()) {
        if ((ret = do_gpio_config_off(hsi)) != 0)
            pr_err("[%s] Failed : do_gpio_config_off.%d, %s@%d\n", __func__, ret, name, index);
        return ret;
    }

    if ((ret = hw_sensor_power_down(hsi->sensor)) != 0)
        pr_err("[%s] Failed : hw_sensor_power_down.%d, %s@%d\n", __func__, ret, name, index);

    hw_sensor_power_down_config(hsi->sensor->board_info);

    return 0;
}

int rpmsg_sensor_ioctl(unsigned int cmd, int index, char *name)
{
    int ret = -EINVAL;

    switch (cmd) {
        case HWSENSOR_IOCTL_POWER_UP:
            if (0 == strlen(name)) {
                if ((ret = all_sensor_power_on(index)) != 0)
                    pr_err("[%s] Failed : all_sensor_power_on.%d, index.%d, name.%s\n", __func__, ret, index, name);
                break;
            }

            if ((ret = hisi_rproc_select_def()) != 0) {
                pr_err("[%s] Failed : hisi_rproc_select_def.%d, index.%d, name.%s\n", __func__, ret, index, name);
                break;
            }

            if ((ret = do_sensor_power_on(index, name)) != 0)
                pr_err("[%s] Failed : do_sensor_power_on.%d, index.%d, name.%s\n", __func__, ret, index, name);

            break;
        case HWSENSOR_IOCTL_POWER_DOWN:
            if (0 == strlen(name)) {
                if ((ret = all_sensor_power_off(index)) != 0)
                    pr_err("[%s] Failed : all_sensor_power_off.%d, index.%d, name.%s\n", __func__, ret, index, name);
                break;
            }

            if ((ret = do_sensor_power_off(index, name)) != 0) {
                pr_err("[%s] Failed : do_sensor_power_off.%d, index.%d, name.%s\n", __func__, ret, index, name);
                break;
            }

            if ((ret = hisi_rproc_select_idle()) != 0)
                pr_err("[%s] Failed : hisi_rproc_select_idle.%d, index.%d, name.%s\n", __func__, ret, index, name);

            break;
        default:
            pr_err("[%s] Failed : cmd.%d, index.%d, name.%s\n", __func__, cmd, index, name);
            break;
    }
    pr_info("[%s] cmd.%d, index.%d, name.%s\n", __func__, cmd, index, name);

    return ret;
}

MODULE_AUTHOR("Chen Tao <chentao20@hisilicon.com>");
MODULE_DESCRIPTION("Hisilicon ISP Sensor Driver");
MODULE_LICENSE("GPL v2");
