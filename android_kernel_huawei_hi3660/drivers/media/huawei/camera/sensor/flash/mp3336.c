/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
/*
    2016/09/19  created by huxiaoming
    2016/10/27  modified by huanglibo
*/
#include "hw_flash.h"
#include <linux/wakelock.h>

/* MP3336 Registers define */
#define REG_CHIPID          0x00
#define REG_MODE            0x01
#define REG_PEAK_CURRENT    0x02
#define REG_FLASH_TIMER     0x03
#define REG_VBL             0x04  //Low battery voltage

#define REG_L1_FL           0x06
#define REG_L2_FL           0x07
#define REG_L1_TX           0x08
#define REG_L2_TX           0x09
#define REG_IFL_ACT         0x0A
#define REG_IFL_MIN         0x0B
#define REG_L1_TOR          0x0C
#define REG_L2_TOR          0x0D
#define REG_FAULT_INDICATION_A               0x0E
#define REG_FAULT_INDICATION_B               0x0F


#define CHIP_ID             0x38
#define CHIP_ID_MASK        0xF8
#define ASIST_MODE          (0x2 << 1)
#define STR_FLASH_MODE      (0x3 << 1)
#define LED2_EN             (0x1 << 3)
#define LED1_EN             (0x1 << 4)
#define STR_MOD             (0x1 << 6)
#define LED_SD              (0x1 << 3)

#define OVER_VOLTAGE_PROTECT    (0x1 << 6)
#define OVER_TEMP_PROTECT       (0x1 << 3)
#define LED_SHORT               (0x1 << 4)
#define LED_OPEN                (0x1)

#define INVALID_GPIO         999
#define FLASH_TIMEOUT_TIME  (0x04 << 4)    //400ms

/* Internal data struct defie */
#define MAX_BRIGHTNESS_FORMMI   (0x09)     //MMI
#define MAX_FLASH_TORCH_CURRENT (0xBE)     //0x7f*1.51
#define MIN_FLASH_TORCH_CURRENT (0x00)
#define MAX_TX_CURRENT          (149)      //186mA

#define MAX_INDEX_MATRIX        (5)

#define WAKE_LOCK_ENABLE        (1)
#define WAKE_LOCK_DISABLE       (0)


#define TORCH_BRIGHT_LEVEL      (3)

#define COLD_WARM_MIDDLE_LEVEL  (271)
#define COLD_LEVEL              (335)
#define WARM_LEVEL              (208)
#define INVALID_LEVEL           (-1)

#define COLD_WARM_BOTH (3)
#define COLD (1)
#define WARM (2)
//lint -save -e24 -e40 -e63 -e64 -e84 -e120 -e156 -e514 -e528 -e778 -e651 -e570
//lint -save -e866 -e846 -e835 -e838 -e785 -e753 -e715 -e708 -e456 -e454 -esym(753,*)


enum {
    RESET = 0,
    STROBE,
    TORCH,
    MAX_PIN,
};

typedef enum {
    CURRENT_TORCH_MIN_MMI,
    CURRENT_TORCH_MAX_LEVEL_MMI,
    CURRENT_TORCH_MIN_LEVEL_MMI,
    CURRENT_MAX,
}lm3646_current_conf;

/* Internal data struct define */
struct hw_mp3336_private_data_t {
    unsigned int flash_led_num;
    unsigned int torch_led_num;
    struct wake_lock  mp3336_wakelock;
    unsigned int need_wakelock;
    /* flash control pin */
    unsigned int pin[MAX_PIN];
    /* flash current config */
    unsigned int ecurrent[CURRENT_MAX];

    unsigned int chipid;
    unsigned int ctrltype;
};

/* Internal varible define */
static struct hw_mp3336_private_data_t hw_mp3336_pdata;

extern struct hw_flash_ctrl_t hw_mp3336_ctrl;



struct hw_mp3336_flash_level_matrix{
    unsigned int flash_level_min;
    unsigned int flash_level_max;

    unsigned int max_current_flash;
    unsigned int max_current_torch;
};

//flash step 1:1.5,  torch step 1:1.17
static struct hw_mp3336_flash_level_matrix hw_mp3336_flash_level[MAX_INDEX_MATRIX] = {
    {0,    15,  22, 17}, //flash 172.48ma  torch 21ma
    {16,   47,  46, 36},   //flash 360.64ma  torch 45ma
    {48,   111, 95, 74},  //flash 744.8ma  torch 92.5ma
    {112,  207, 143, 112},//flash 1121.12ma  torch 140ma
    {208,  335, 191, 149},//flash 1497.44ma  torch 186.25ma
    /* mp3336 flash step 7.84ma torch step 1.25ma
     * lm3646 current
     * {187ma, 23ma}
     * {374ma, 46ma}
     * {749ma, 93ma}
     * {1124ma, 140ma}
     * {1499ma, 187ma}
     */
};

#define DUAL_LED_WHITE (0)
#define DUAL_LED_TEMPERATURE (1)
static int support_dual_led_type = DUAL_LED_TEMPERATURE;

extern struct dsm_client *client_flash;

DEFINE_HISI_FLASH_MUTEX(mp3336);

#ifdef CONFIG_LLT_TEST

struct UT_TEST_MP3336
{
    int (*hw_mp3336_set_pin_strobe)(struct hw_flash_ctrl_t *flash_ctrl, unsigned int state);
    int (*hw_mp3336_set_pin_torch)(struct hw_flash_ctrl_t *flash_ctrl, unsigned int state);
    int (*hw_mp3336_set_pin_reset)(struct hw_flash_ctrl_t *flash_ctrl, unsigned int state);
    int (*hw_mp3336_init)(struct hw_flash_ctrl_t *flash_ctrl);
    int (*hw_mp3336_exit)(struct hw_flash_ctrl_t *flash_ctrl);
    int (*hw_mp3336_flash_mode)(struct hw_flash_ctrl_t *flash_ctrl, int data);
    int (*hw_mp3336_torch_mode_mmi)(struct hw_flash_ctrl_t *flash_ctrl, int data);
    int (*hw_mp3336_set_mode)(struct hw_flash_ctrl_t *flash_ctrl, void *data);
    int (*hw_mp3336_on)(struct hw_flash_ctrl_t *flash_ctrl, void *data);
    int (*hw_mp3336_brightness)(struct hw_flash_ctrl_t *flash_ctrl, void *data);
    int (*hw_mp3336_off)(struct hw_flash_ctrl_t *flash_ctrl);
    int (*hw_mp3336_get_dt_data)(struct hw_flash_ctrl_t *flash_ctrl);
    ssize_t (*hw_mp3336_dual_leds_show)(struct device *dev, struct device_attribute *attr, char *buf);
    ssize_t (*hw_mp3336_dual_leds_store)(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
    ssize_t (*hw_mp3336_lightness_store)(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
    ssize_t (*hw_mp3336_lightness_show)(struct device *dev, struct device_attribute *attr, char *buf);
    ssize_t (*hw_mp3336_flash_lightness_store)(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
    ssize_t (*hw_mp3336_flash_mask_show)(struct device *dev, struct device_attribute *attr, char *buf);
    int (*hw_mp3336_register_attribute)(struct hw_flash_ctrl_t *flash_ctrl, struct device *dev);
    ssize_t (*hw_mp3336_flash_mask_store)(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
    int (*hw_mp3336_match)(struct hw_flash_ctrl_t *flash_ctrl);
    void (*hw_mp3336_torch_brightness_set)(struct led_classdev *cdev,enum led_brightness brightness);
    int (*hw_mp3336_param_check)(char *buf, unsigned long *param, int num_of_par);
};

#endif /* CONFIG_LLT_TEST */


/* Function define */
static int hw_mp3336_param_check(char *buf, unsigned long *param,
    int num_of_par);
static int hw_mp3336_set_pin_strobe(struct hw_flash_ctrl_t *flash_ctrl, unsigned int state)
{
    struct hw_mp3336_private_data_t *pdata = NULL;
    int rc = 0;

    if (NULL == flash_ctrl) {
        cam_err("%s flash_ctrl is NULL.", __func__);
        return -1;
    }

    pdata = (struct hw_mp3336_private_data_t *)flash_ctrl->pdata;
    if (NULL == pdata) {
        cam_err("%s pdata is NULL.", __func__);
        return -1;
    }

    cam_debug("%s strobe0=%d, state=%d.", __func__,
        pdata->pin[STROBE], state);
    if (pdata->pin[STROBE] != INVALID_GPIO) {
        rc = gpio_direction_output(pdata->pin[STROBE], (int)state);
        if (rc < 0) {
            cam_err("%s gpio output is err rc=%d.", __func__, rc);
        }
    }
    return rc;
}

static int hw_mp3336_set_pin_torch(struct hw_flash_ctrl_t *flash_ctrl, unsigned int state)
{
    struct hw_mp3336_private_data_t *pdata = NULL;
    int rc = 0;

    if (NULL == flash_ctrl) {
        cam_err("%s flash_ctrl is NULL.", __func__);
        return -1;
    }

    pdata = (struct hw_mp3336_private_data_t *)flash_ctrl->pdata;
    if (NULL == pdata) {
        cam_err("%s pdata is NULL.", __func__);
        return -1;
    }

    cam_debug("%s strobe1=%d, state=%d.", __func__,
    pdata->pin[TORCH], state);
    if (pdata->pin[TORCH] != INVALID_GPIO) {
        rc = gpio_direction_output(pdata->pin[TORCH], (int)state);
        if (rc < 0) {
            cam_err("%s gpio output is err rc=%d.", __func__, rc);
        }
    }
    return rc;
}

static int hw_mp3336_set_pin_reset(struct hw_flash_ctrl_t *flash_ctrl, unsigned int state)
{
    struct hw_mp3336_private_data_t *pdata = NULL;
    int rc = 0;

    if (NULL == flash_ctrl) {
        cam_err("%s flash_ctrl is NULL.", __func__);
        return -1;
    }

    pdata = (struct hw_mp3336_private_data_t *)flash_ctrl->pdata;
    if (NULL == pdata) {
        cam_err("%s pdata is NULL.", __func__);
        return -1;
    }

    cam_debug("%s reset=%d, state=%d.", __func__,
        pdata->pin[RESET], state);
    rc = gpio_direction_output(pdata->pin[RESET], (int)state);
    if (rc < 0) {
        cam_err("%s gpio output is err rc=%d.", __func__, rc);
    }
    return rc;
}

static int hw_mp3336_init(struct hw_flash_ctrl_t *flash_ctrl)
{
    struct hw_mp3336_private_data_t *pdata = NULL;
    int rc = 0;

    cam_debug("%s enter.", __func__);

    if (NULL == flash_ctrl) {
        cam_err("%s flash_ctrl is NULL.", __func__);
        return -1;
    }

    pdata = (struct hw_mp3336_private_data_t *)flash_ctrl->pdata;
    if (NULL == pdata) {
        cam_err("%s pdata is NULL.", __func__);
        return -1;
    }

    flash_ctrl->pctrl = devm_pinctrl_get_select(flash_ctrl->dev,
        PINCTRL_STATE_DEFAULT);
    if (NULL == flash_ctrl->pctrl) {
        cam_err("%s failed to set pin.", __func__);
        return -EIO;
    }

    rc = gpio_request(pdata->pin[RESET], "flash-reset");
    if (rc < 0) {
        cam_err("%s failed to request reset pin.", __func__);
        return -EIO;
    }

    if(pdata->pin[STROBE] != INVALID_GPIO) {
        rc = gpio_request(pdata->pin[STROBE], "flash-strobe");
        if (rc < 0) {
            cam_err("%s failed to request strobe pin.", __func__);
            goto err1;
        }
    }

    if(pdata->pin[TORCH] != INVALID_GPIO) {
        rc = gpio_request(pdata->pin[TORCH], "flash-torch");
        if (rc < 0) {
            cam_err("%s failed to request torch pin.", __func__);
            goto err2;
        }
    }

    rc = of_property_read_u32(flash_ctrl->dev->of_node, "huawei,need-wakelock", &pdata->need_wakelock);
    cam_info("%s huawei,need-wakelock %d, rc %d", __func__, pdata->need_wakelock, rc);
    if (rc < 0) {
        pdata->need_wakelock = WAKE_LOCK_DISABLE;
        cam_err("%s failed %d", __func__, __LINE__);
        goto err3;
    }

    hw_mp3336_set_pin_reset(flash_ctrl,LOW);
    //msleep(MP3336_RESET_HOLD_TIME);
    if(pdata->need_wakelock == WAKE_LOCK_ENABLE) {
        wake_lock_init(&pdata->mp3336_wakelock,WAKE_LOCK_SUSPEND,"mp3336");
    }
    return rc;

err3:
    if(pdata->pin[TORCH] != INVALID_GPIO) {
        gpio_free(pdata->pin[TORCH]);
    }
err2:
    if(pdata->pin[STROBE] != INVALID_GPIO) {
        gpio_free(pdata->pin[STROBE]);
    }
err1:
    gpio_free(pdata->pin[RESET]);
    return rc;
}

static int hw_mp3336_exit(struct hw_flash_ctrl_t *flash_ctrl)
{
    struct hw_mp3336_private_data_t *pdata = NULL;

    cam_debug("%s enter.", __func__);

    if (NULL == flash_ctrl) {
        cam_err("%s flash_ctrl is NULL.", __func__);
        return -1;
    }
    if ((NULL == flash_ctrl->func_tbl) || (NULL == flash_ctrl->func_tbl->flash_off)) {
        cam_err("%s flash_ctrl->func_tbl or flash_ctrl->func_tbl->flash_off is NULL.", __func__);
        return -1;
    }
    flash_ctrl->func_tbl->flash_off(flash_ctrl);

    pdata = (struct hw_mp3336_private_data_t *)flash_ctrl->pdata;
    if (NULL == pdata) {
        cam_err("%s pdata is NULL.", __func__);
        return -1;
    }

    if (pdata->pin[TORCH] != INVALID_GPIO) {
        gpio_free(pdata->pin[TORCH]);
    }
    if (pdata->pin[STROBE] != INVALID_GPIO) {
        gpio_free(pdata->pin[STROBE]);
    }
    gpio_free(pdata->pin[RESET]);
    flash_ctrl->pctrl = devm_pinctrl_get_select(flash_ctrl->dev,
        PINCTRL_STATE_IDLE);

    return 0;
}

static int hw_mp3336_flash_mode(struct hw_flash_ctrl_t *flash_ctrl,
int data)
{
    struct hw_flash_i2c_client *i2c_client = NULL;
    struct hw_flash_i2c_fn_t *i2c_func = NULL;
    struct hw_mp3336_private_data_t *pdata = NULL;
    unsigned char reg_val = 0;

    cam_info("%s data=%d.", __func__, data);
    if (NULL == flash_ctrl) {
        cam_err("%s flash_ctrl is NULL.", __func__);
        return -1;
    }

    i2c_client = flash_ctrl->flash_i2c_client;
    pdata = flash_ctrl->pdata;
    if ((NULL == pdata) || (NULL == i2c_client)) {
        cam_err("%s pdata or i2c_client is NULL.", __func__);
        return -1;
    }
    i2c_func = i2c_client->i2c_func_tbl;
    if (NULL == i2c_func) {
        cam_err("%s i2c_func is NULL.", __func__);
        return -1;
    }

    /* clear error flag,resume chip */
    i2c_func->i2c_read(i2c_client, REG_FAULT_INDICATION_A, &reg_val);
    i2c_func->i2c_read(i2c_client, REG_FAULT_INDICATION_B, &reg_val);
    i2c_func->i2c_read(i2c_client, REG_MODE, &reg_val);
    i2c_func->i2c_write(i2c_client, REG_MODE, (reg_val & (~STR_MOD)) | LED2_EN  | LED1_EN | STR_FLASH_MODE);

    i2c_func->i2c_write(i2c_client, REG_L1_TX, MAX_TX_CURRENT);
    i2c_func->i2c_write(i2c_client, REG_L2_TX, MAX_TX_CURRENT);

    i2c_func->i2c_write(i2c_client, REG_L1_FL , MAX_FLASH_TORCH_CURRENT);
    i2c_func->i2c_write(i2c_client, REG_L2_FL , MAX_FLASH_TORCH_CURRENT);

    return 0;
}

static int hw_mp3336_torch_mode_mmi(struct hw_flash_ctrl_t *flash_ctrl,
int data)
{
    struct hw_flash_i2c_client *i2c_client = NULL;
    struct hw_flash_i2c_fn_t *i2c_func = NULL;
    struct hw_mp3336_private_data_t *pdata = NULL;
    unsigned char reg_val = 0;
    unsigned char level = 0;

    cam_info("%s data=%d.", __func__, data);
    if (NULL == flash_ctrl) {
        cam_err("%s flash_ctrl is NULL.", __func__);
        return -1;
    }

    i2c_client = flash_ctrl->flash_i2c_client;
    pdata = flash_ctrl->pdata;
    if ((NULL == pdata) || (NULL == i2c_client)) {
        cam_err("%s pdata or i2c_client is NULL.", __func__);
        return -1;
    }
    i2c_func = i2c_client->i2c_func_tbl;
    if (NULL == i2c_func) {
        cam_err("%s i2c_func is NULL.", __func__);
        return -1;
    }

    /* clear error flag,resume chip */
    i2c_func->i2c_read(i2c_client, REG_FAULT_INDICATION_A, &reg_val);
    i2c_func->i2c_read(i2c_client, REG_FAULT_INDICATION_B, &reg_val);
    i2c_func->i2c_read(i2c_client, REG_MODE, &reg_val);
    i2c_func->i2c_write(i2c_client, REG_MODE, reg_val | ASIST_MODE);
    /* set LED Flash current value */
    if (data == TORCH_LEFT_MODE) {
        cam_info("%s flash_led=TORCH_LEFT_MODE", __func__);
        level = pdata->ecurrent[CURRENT_TORCH_MAX_LEVEL_MMI];
    } else if (data == TORCH_RIGHT_MODE){
        cam_info("%s flash_led=TORCH_RIGHT_MODE", __func__);
        level = pdata->ecurrent[CURRENT_TORCH_MIN_LEVEL_MMI];
    }else {
        cam_info("%s default flash_led=TORCH_LEFT_MODE  mode = %d", __func__,data);
        level = pdata->ecurrent[CURRENT_TORCH_MAX_LEVEL_MMI];
    }

    i2c_func->i2c_write(i2c_client, REG_L1_TOR, level);
    i2c_func->i2c_write(i2c_client, REG_L2_TOR, pdata->ecurrent[CURRENT_TORCH_MAX_LEVEL_MMI] - level);
    i2c_func->i2c_read(i2c_client, REG_MODE, &reg_val);
    i2c_func->i2c_write(i2c_client, REG_MODE,  (reg_val | LED1_EN | LED2_EN) & (~STR_MOD) );

    return 0;
}

static int hw_mp3336_set_mode(struct hw_flash_ctrl_t *flash_ctrl, void *data)
{
    struct hw_flash_cfg_data *cdata = (struct hw_flash_cfg_data *)data;
    struct hw_flash_i2c_client *i2c_client = NULL;
    struct hw_flash_i2c_fn_t *i2c_func = NULL;
    struct hw_mp3336_private_data_t *pdata = NULL;
    struct hw_mp3336_flash_level_matrix* matrix = NULL;
    unsigned char reg1_val = 0;
    unsigned char reg2_val = 0;
    unsigned char reg3_val = 0;
    unsigned char reg4_val = 0;
    unsigned char mode = 0;

    unsigned int regcurrentflash1 = 0;
    unsigned int regcurrentflash2 = 0;
    unsigned int regcurrenttorch1 = 0;
    unsigned int regcurrenttorch2 = 0;
    unsigned int tmp_led1 = 0;
    unsigned int tmp_led2 = 0;
    unsigned int current_index = 0;
    int rc = 0;
    int i = 0;

    if ((NULL == flash_ctrl) || (NULL == cdata)) {
        cam_err("%s flash_ctrl or cdata is NULL.", __func__);
        return -1;
    }

    i2c_client = flash_ctrl->flash_i2c_client;
    if (NULL == i2c_client) {
        cam_err("%s i2c_client NULL.", __func__);
        return -1;
    }
    i2c_func = i2c_client->i2c_func_tbl;
    pdata = flash_ctrl->pdata;
    if ((NULL == i2c_func) || (NULL == pdata)) {
        cam_err("%s pdata or i2c_func is NULL.", __func__);
        return -1;
    }

    current_index = (unsigned int)cdata->data;
    if (current_index >= pdata->flash_led_num) {
        cam_err("%s limit flash to current = %u[%u] mode = %d ", __func__, pdata->flash_led_num - 1,
                    current_index, cdata->mode);
        current_index = pdata->flash_led_num > 0 ? pdata->flash_led_num - 1:0; //limit max index
    }

    for (i = 0; i < MAX_INDEX_MATRIX; i++) {
        if (hw_mp3336_flash_level[i].flash_level_min <= current_index
        && hw_mp3336_flash_level[i].flash_level_max >= current_index) {
            matrix = &hw_mp3336_flash_level[i];
            break;
        }
    }



    if (matrix == NULL) {
        cam_err("%s curret out of range mode = %d curret = %u  ",__func__,cdata->mode, current_index);
        return -1;
    }
    i2c_func->i2c_read(i2c_client, REG_FAULT_INDICATION_A, &reg1_val);//read back error status
    i2c_func->i2c_read(i2c_client, REG_FAULT_INDICATION_B, &reg1_val);
    i2c_func->i2c_read(i2c_client, REG_MODE, &reg1_val);
    i2c_func->i2c_read(i2c_client, REG_PEAK_CURRENT, &reg2_val);
    i2c_func->i2c_read(i2c_client, REG_VBL, &reg4_val);

    //current limit 2.5A
    reg2_val &= ~(0x3 << 3); //clear bit3,bit4
    reg2_val |= (0x1 << 3);  //set 2.5A(0x1)
    i2c_func->i2c_write(i2c_client, REG_PEAK_CURRENT, reg2_val);

    //input DC Current limit 3A
    reg4_val &= 0xF0;   //clear bit 0~3
    reg4_val |= 0x08;   //set 3A(0x8)
    i2c_func->i2c_write(i2c_client, REG_VBL, reg4_val);

    if (FLASH_MODE == cdata->mode) {
            /* 1 means I2C control, 0 means GPIO control. */
            mode = STR_FLASH_MODE;
            tmp_led1 = (current_index - matrix->flash_level_min) * 151 / 100; //reg value = 1.51*index
            if ((matrix->max_current_flash) <= tmp_led1) {
                tmp_led1 = matrix->max_current_flash;
                tmp_led2 = 0;
            } else {
                tmp_led2 = (matrix->max_current_flash) - tmp_led1;
            }
            regcurrentflash1 = tmp_led1;
            regcurrentflash2 = tmp_led2;
            cam_info("FLASH_MODE regcurrenttorch1 = %d, regcurrenttorch2 = %d", regcurrentflash1, regcurrentflash2);
    } else {
            mode = ASIST_MODE; //ASIST_MODE for software control torch
            tmp_led1 = (current_index - matrix->flash_level_min) * 118 / 100; //reg value = 1.18*index
            if ((matrix->max_current_torch) <= tmp_led1) {
                tmp_led1 = matrix->max_current_torch;
                tmp_led2 = 0;
            } else {
                tmp_led2 = (matrix->max_current_torch) - tmp_led1;
            }
            regcurrenttorch1 = tmp_led1;
            regcurrenttorch2 = tmp_led2;
            cam_info("ASIST_MODE regcurrenttorch1 = %d, regcurrenttorch2 = %d", regcurrenttorch1, regcurrenttorch2);
    }
    i2c_func->i2c_write(i2c_client, REG_MODE, reg1_val | mode );

    if (FLASH_MODE == cdata->mode) {
        //set flash timeout
        i2c_func->i2c_read(i2c_client, REG_FLASH_TIMER, &reg3_val);
        i2c_func->i2c_write(i2c_client, REG_FLASH_TIMER, (reg3_val&(~LED_SD))| FLASH_TIMEOUT_TIME);

        //set TX current
        i2c_func->i2c_write(i2c_client, REG_L1_TX, MAX_TX_CURRENT);
        i2c_func->i2c_write(i2c_client, REG_L2_TX, MAX_TX_CURRENT);

        i2c_func->i2c_write(i2c_client, REG_L1_FL, (unsigned char)regcurrentflash1);
        i2c_func->i2c_write(i2c_client, REG_L2_FL, (unsigned char)regcurrentflash2);
        cam_info("flash1 = 0x%x, flash2 = 0x%x", regcurrentflash1, regcurrentflash2);
    } else {
        i2c_func->i2c_write(i2c_client, REG_L1_TOR, (unsigned char)regcurrenttorch1);
        i2c_func->i2c_write(i2c_client, REG_L2_TOR, (unsigned char)regcurrenttorch2);
        cam_info("torch1 = 0x%x, torch2 = 0x%x", regcurrenttorch1, regcurrenttorch2);
    }

    i2c_func->i2c_read(i2c_client, REG_MODE, &reg1_val);
    if (cdata->mode == FLASH_MODE) {
        rc = i2c_func->i2c_write(i2c_client, REG_MODE,  (reg1_val | LED1_EN | LED2_EN) & (~STR_MOD) );
    } else {
        rc = i2c_func->i2c_write(i2c_client, REG_MODE,  (reg1_val | LED1_EN | LED2_EN));
    }
    if (rc < 0) {
        if (!dsm_client_ocuppy(client_flash)) {
            dsm_client_record(client_flash, "flash i2c transfer fail\n");
            dsm_client_notify(client_flash, DSM_FLASH_I2C_ERROR_NO);
            cam_warn("[I/DSM] %s dsm_client_notify", client_flash->client_name);
        }
    }


    return 0;
}


static int hw_mp3336_on(struct hw_flash_ctrl_t *flash_ctrl, void *data)
{
    struct hw_mp3336_private_data_t *pdata = NULL;
    struct hw_flash_cfg_data *cdata = (struct hw_flash_cfg_data *)data;
    int rc = 0;
    if ((NULL == flash_ctrl) || (NULL == cdata)) {
        cam_err("%s flash_ctrl or cdata is NULL.", __func__);
        return -1;
    }
    pdata = (struct hw_mp3336_private_data_t *)flash_ctrl->pdata;
    if (NULL == pdata) {
        cam_err("%s pdata is NULL.", __func__);
        return -1;
    }

    cam_info("%s mode=%d, level=%d.", __func__, cdata->mode, cdata->data);

    mutex_lock(flash_ctrl->hw_flash_mutex);
    if (pdata->need_wakelock == WAKE_LOCK_ENABLE) {
        wake_lock(&pdata->mp3336_wakelock);
    }

    hw_mp3336_set_pin_strobe(flash_ctrl,LOW);
    hw_mp3336_set_pin_torch(flash_ctrl,LOW);


    hw_mp3336_set_mode(flash_ctrl,data);
    flash_ctrl->state.mode = cdata->mode;
    flash_ctrl->state.data = cdata->data;
    hw_mp3336_set_pin_strobe(flash_ctrl,HIGH);
    hw_mp3336_set_pin_torch(flash_ctrl,HIGH);

    mutex_unlock(flash_ctrl->hw_flash_mutex);

    return rc;
}

static int hw_mp3336_brightness(struct hw_flash_ctrl_t *flash_ctrl, void *data)
{
    struct hw_flash_cfg_data *cdata = (struct hw_flash_cfg_data *)data;
    int rc = -1;

    if ((NULL == flash_ctrl) || (NULL == cdata)) {
        cam_err("%s flash_ctrl or cdata is NULL.", __func__);
        return -1;
    }

    cam_debug("%s mode=%d, level=%d.", __func__, cdata->mode, cdata->data);
    cam_info("%s enter.", __func__);
    mutex_lock(flash_ctrl->hw_flash_mutex);

    if (FLASH_MODE == cdata->mode) {
        rc = hw_mp3336_flash_mode(flash_ctrl, cdata->data);
        if (0 > rc) {
            cam_err("%s hw_mp3336_flash_mode faild rc %d.", __func__, rc);
        }
    } else {
        rc = hw_mp3336_torch_mode_mmi(flash_ctrl, cdata->mode);
        if (0 > rc) {
            cam_err("%s hw_mp3336_flash_mode faild rc %d.", __func__, rc);
        }
    }
    flash_ctrl->state.mode = cdata->mode;
    flash_ctrl->state.data = cdata->data;
    mutex_unlock(flash_ctrl->hw_flash_mutex);

return rc;
}


static int hw_mp3336_off(struct hw_flash_ctrl_t *flash_ctrl)
{
    struct hw_flash_i2c_client *i2c_client = NULL;
    struct hw_flash_i2c_fn_t *i2c_func = NULL;
    unsigned char mode_val = 0;
    unsigned char timeout_val = 0;
    unsigned char fault_val = 0;
    unsigned char fault_val2 = 0;
    unsigned char flash_act_cur = 0;
    unsigned char flash_min_cur = 0;

    struct hw_mp3336_private_data_t *pdata = NULL;
    cam_debug("%s enter.", __func__);
    if (NULL == flash_ctrl) {
        cam_err("%s flash_ctrl is NULL.", __func__);
        return -1;
    }
    pdata = (struct hw_mp3336_private_data_t *)flash_ctrl->pdata;
    if (NULL == pdata) {
        cam_err("%s pdata is NULL.", __func__);
        return -1;
    }
    i2c_client = flash_ctrl->flash_i2c_client;
    if (NULL == i2c_client) {
        cam_err("%s i2c_client is NULL.", __func__);
        return -1;
    }
    i2c_func = i2c_client->i2c_func_tbl;
    if (NULL == i2c_func) {
        cam_err("%s i2c_func is NULL.", __func__);
        return -1;
    }
    mutex_lock(flash_ctrl->hw_flash_mutex);
    if (flash_ctrl->state.mode == STANDBY_MODE) {
        mutex_unlock(flash_ctrl->hw_flash_mutex);
        return 0;
    }
    flash_ctrl->state.mode = STANDBY_MODE;
    flash_ctrl->state.data = 0;


    i2c_func->i2c_read(i2c_client, REG_MODE, &mode_val);
    i2c_func->i2c_read(i2c_client, REG_FLASH_TIMER, &timeout_val);
    i2c_func->i2c_write(i2c_client, REG_MODE, (mode_val & (~LED1_EN) & (~LED2_EN) & (~ASIST_MODE) & (~STR_FLASH_MODE)));
    i2c_func->i2c_write(i2c_client, REG_FLASH_TIMER, (timeout_val & (~LED_SD)));

    i2c_func->i2c_read(i2c_client, REG_FAULT_INDICATION_A, &fault_val);//read back error status
    if(fault_val & OVER_VOLTAGE_PROTECT) {
        if(!dsm_client_ocuppy(client_flash)) {
            dsm_client_record(client_flash, "flash over voltage protect! FlagReg1[0x%x]\n", fault_val);
            dsm_client_notify(client_flash, DSM_FLASH_OPEN_SHOTR_ERROR_NO);
            cam_warn("[I/DSM] %s dsm_client_notify", client_flash->client_name);
        }
    }
    if (fault_val & OVER_TEMP_PROTECT) {
        if(!dsm_client_ocuppy(client_flash)) {
            dsm_client_record(client_flash, "flash temperature is too hot! FlagReg1[0x%x]\n", fault_val);
            dsm_client_notify(client_flash, DSM_FLASH_HOT_DIE_ERROR_NO);
            cam_warn("[I/DSM] %s dsm_client_notify", client_flash->client_name);
        }
    }
    if (fault_val & LED_SHORT) {
        if(!dsm_client_ocuppy(client_flash)) {
            dsm_client_record(client_flash, "flash led short fault! FlagReg2[0x%x]\n", fault_val);
            dsm_client_notify(client_flash, DSM_FLASH_OPEN_SHOTR_ERROR_NO);
            cam_warn("[I/DSM] %s dsm_client_notify", client_flash->client_name);
        }
    }

    i2c_func->i2c_read(i2c_client, REG_FAULT_INDICATION_B, &fault_val2);
    if (fault_val2 & LED_OPEN) {
        if(!dsm_client_ocuppy(client_flash)) {
            dsm_client_record(client_flash, "flash led open fault! FlagReg2[0x%x]\n", fault_val2);
            dsm_client_notify(client_flash, DSM_FLASH_OPEN_SHOTR_ERROR_NO);
            cam_warn("[I/DSM] %s dsm_client_notify", client_flash->client_name);
        }
    }
    cam_info("%s Fault and Flag Indication Reg E 0x%x Reg F 0x%x", __func__, fault_val, fault_val2);

    i2c_func->i2c_read(i2c_client, REG_IFL_ACT, &flash_act_cur);
    i2c_func->i2c_read(i2c_client, REG_IFL_MIN, &flash_min_cur);
    cam_info("%s IFL ACT current 0x%x MIN Current 0x%x", __func__, flash_act_cur, flash_min_cur);

    hw_mp3336_set_pin_strobe(flash_ctrl, LOW);
    hw_mp3336_set_pin_torch(flash_ctrl, LOW);

    if(pdata->need_wakelock == WAKE_LOCK_ENABLE) {
        wake_unlock(&pdata->mp3336_wakelock);
    }
    cam_info("%s end", __func__);
    mutex_unlock(flash_ctrl->hw_flash_mutex);

    return 0;
}

static int hw_mp3336_get_dt_data(struct hw_flash_ctrl_t *flash_ctrl)
{
    struct hw_mp3336_private_data_t *pdata = NULL;
    struct device_node *node = NULL;
    int i = 0;
    int rc = -1;

    cam_debug("%s enter.", __func__);

    if (NULL == flash_ctrl) {
        cam_err("%s flash_ctrl is NULL.", __func__);
        return rc;
    }
    pdata = (struct hw_mp3336_private_data_t *)flash_ctrl->pdata;
    if (NULL == pdata) {
        cam_err("%s pdata is NULL.", __func__);
        return rc;
    }
    if (flash_ctrl->dev) {
        node = flash_ctrl->dev->of_node;
    }
    if (NULL == node) {
        cam_err("%s of_node is NULL.", __func__);
        return rc;
    }

    rc = of_property_read_u32_array(node, "huawei,flash-pin",
    pdata->pin, (unsigned long)MAX_PIN);
    if (rc < 0) {
        cam_err("%s get dt flash-pin failed line %d", __func__, __LINE__);
        return rc;
    } else {
        for (i = 0; i < MAX_PIN; i++) {
            cam_info("%s pin[%d]=%d.", __func__, i,
            pdata->pin[i]);
        }
    }

    rc = of_property_read_u32(node, "huawei,flash-chipid",
            &pdata->chipid);
    cam_info("%s hisi,chipid 0x%x, rc %d", __func__,
            pdata->chipid, rc);
    if (rc < 0) {
        cam_err("%s failed %d", __func__, __LINE__);
        return rc;
    }

    rc = of_property_read_u32(node, "huawei,flash-ctrltype",
            &pdata->ctrltype);
    cam_info("%s hisi,ctrltype 0x%x, rc %d", __func__,
            pdata->ctrltype, rc);
    if (rc < 0) {
        cam_err("%s failed %d", __func__, __LINE__);
        return rc;
    }

    rc = of_property_read_u32(node, "huawei,flash_led_num",
            &pdata->flash_led_num);
    cam_info("%s hisi,flash_led_num %d, rc %d", __func__,
            pdata->flash_led_num, rc);
    if (rc < 0) {
        cam_err("%s failed %d", __func__, __LINE__);
        return rc;
    }

    rc = of_property_read_u32(node, "huawei,torch_led_num",
            &pdata->torch_led_num);
    cam_info("%s hisi,torch_led_num %d, rc %d", __func__,
            pdata->torch_led_num, rc);
    if (rc < 0) {
        cam_err("%s failed %d", __func__, __LINE__);
        return rc;
    }

    rc = of_property_read_u32(node, "huawei,led-type",
                              &support_dual_led_type);
    cam_info("%s huawei,led-type %d, rc %d\n", __func__,
             support_dual_led_type, rc);
    if (rc < 0) {
        cam_err("%s read led-type failed %d\n", __func__, __LINE__);
        return rc;
    }

    rc = of_property_read_u32_array(node, "huawei,flash-current",
                                    pdata->ecurrent, CURRENT_MAX);
    if (rc < 0) {
        cam_err("%s read flash-current failed line %d\n", __func__, __LINE__);
        return rc;
    } else {
        for (i=0; i<CURRENT_MAX; i++) {
            cam_info("%s ecurrent[%d]=%d.\n", __func__, i,
                     pdata->ecurrent[i]);
        }
    }

    return rc;
}

static ssize_t hw_mp3336_dual_leds_show(struct device *dev,
struct device_attribute *attr,char *buf)
{
    int rc = 0;
    if (NULL == buf) {
        cam_err("%s buf is NULL", __func__);
        return -1;
    }
    rc = scnprintf(buf, PAGE_SIZE, "%d\n",
        support_dual_led_type);

    return rc;
}

static ssize_t hw_mp3336_dual_leds_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t count)
{
    return (ssize_t)count;
}


static ssize_t hw_mp3336_lightness_show(struct device *dev,
struct device_attribute *attr,char *buf)
{
    int rc = 0;
    if (NULL == buf) {
        cam_err("%s buf is NULL", __func__);
        return -1;
    }
    rc = scnprintf(buf, PAGE_SIZE, "mode=%d, data=%d.\n",
        hw_mp3336_ctrl.state.mode, hw_mp3336_ctrl.state.data);

    return rc;
}

static int hw_mp3336_param_check(char *buf, unsigned long *param,
int num_of_par)
{
    char *token = NULL;
    unsigned int base = 0;
    int cnt = 0;
    if ((NULL == buf) || (NULL == param)) {
        cam_err("%s buf or param is NULL", __func__);
        return -1;
    }

    token = strsep(&buf, " ");
    for (cnt = 0; cnt < num_of_par; cnt++) {
        if (token != NULL) {
            if ((token[1] == 'x') || (token[1] == 'X')) {//parse 0x*
                base = 16;  //hex
            } else {
                base = 10;  //decimal
            }
            if (strict_strtoul(token, base, &param[cnt]) != 0) {
                return -EINVAL;
            }
            token = strsep(&buf, " ");
        } else{
            return -EINVAL;
        }
    }
    return 0;
}

#define MAX_LIGHTNESS_PARAM_NUM 2
#define LIGHTNESS_PARAM_MODE_INDEX 0
#define LIGHTNESS_PARAM_DATA_INDEX 1

static ssize_t hw_mp3336_lightness_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct hw_flash_cfg_data cdata = {0};
    unsigned long param[MAX_LIGHTNESS_PARAM_NUM] = {0};
    int rc = 0;

    rc = hw_mp3336_param_check((char *)buf, param, MAX_LIGHTNESS_PARAM_NUM);
    if (rc < 0) {
        cam_err("%s failed to check param.", __func__);
        return rc;
    }

    cdata.mode = (int)param[LIGHTNESS_PARAM_MODE_INDEX];
    cdata.data = (int)param[LIGHTNESS_PARAM_DATA_INDEX];
    cam_info("%s mode = %d data = %d", __func__, cdata.mode, cdata.data);
    if (cdata.mode == STANDBY_MODE) {
        rc = hw_mp3336_off(&hw_mp3336_ctrl);
        if (rc < 0) {
            cam_err("%s mp3336 flash off error.", __func__);
            return rc;
        }
    } else {
        rc = hw_mp3336_on(&hw_mp3336_ctrl, &cdata);
        if (rc < 0) {
            cam_err("%s mp3336 flash on error.", __func__);
            return rc;
        }
    }

    return (ssize_t)count;
}

static  int calc_id_to_reg(int flash_id)
{
    int data;
    switch(flash_id) {
        case COLD_WARM_BOTH:
            data = COLD_WARM_MIDDLE_LEVEL;
            break;
        case WARM:
            data = WARM_LEVEL;
            break;
        case COLD:
            data = COLD_LEVEL;
            break;
        default:
            data = INVALID_LEVEL;//invalid data
            break;
    }
    return data;
}

#define MAX_LIGHTNESS_PARAM_NUM_RT 2
#define LIGHTNESS_PARAM_FD_INDEX_RT 0
#define LIGHTNESS_PARAM_MODE_INDEX_RT 1

static ssize_t hw_mp3336_flash_lightness_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct hw_flash_cfg_data cdata = {0};
    unsigned long param[MAX_LIGHTNESS_PARAM_NUM_RT] = {0};
    int rc = 0;

    if (NULL == buf) {
        cam_err("%s buf is NULL", __func__);
        return -1; //return err
    }

    rc = hw_mp3336_param_check((char *)buf, param, MAX_LIGHTNESS_PARAM_NUM_RT);
    if (rc < 0) {
        cam_err("%s failed to check param.", __func__);
        return rc;
    }
    cdata.data = calc_id_to_reg((int)param[LIGHTNESS_PARAM_FD_INDEX_RT]);
    if(INVALID_LEVEL == cdata.data) {//invalid data, turn off
        cdata.mode = STANDBY_MODE;
    } else {
        cdata.mode = (int)param[LIGHTNESS_PARAM_MODE_INDEX_RT];
    }
    cam_info("%s mode = %d data = %d", __func__, cdata.mode, cdata.data);
    if (cdata.mode == STANDBY_MODE) {
        rc = hw_mp3336_off(&hw_mp3336_ctrl);
        if (rc < 0) {
            cam_err("%s mp3336 flash off error.", __func__);
            return rc;
        }
    } else {
        rc = hw_mp3336_on(&hw_mp3336_ctrl, &cdata);
        if (rc < 0) {
            cam_err("%s mp3336 flash on error.", __func__);
            return rc;
        }
    }

    return (ssize_t)count;
}

static ssize_t hw_mp3336_flash_mask_show(struct device *dev,
    struct device_attribute *attr,char *buf)
{
    int rc = 0;
    if (NULL == buf) {
        cam_err("%s buf is NULL", __func__);
        return -1;
    }
    rc = scnprintf(buf, PAGE_SIZE, "flash_mask_disabled=%d.\n",
        hw_mp3336_ctrl.flash_mask_enable);

    return rc;
}

static ssize_t hw_mp3336_flash_mask_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    if (NULL == buf) {
        cam_err("%s buf is NULL", __func__);
        return -1;
    }
    if ('0' == buf[0]) { //char '0' for mask disable
        hw_mp3336_ctrl.flash_mask_enable = false;
    } else {
        hw_mp3336_ctrl.flash_mask_enable = true;
    }
    cam_info("%s flash_mask_enable=%d.", __func__,
            hw_mp3336_ctrl.flash_mask_enable);
    return (ssize_t)count;
}

static void hw_mp3336_torch_brightness_set(struct led_classdev *cdev,
    enum led_brightness brightness)
{
    struct hw_flash_cfg_data cdata;
    int rc = 0;
    unsigned int led_bright = brightness;

    cam_info("%s brightness= %d",__func__,brightness);
    if (STANDBY_MODE == led_bright) {

        rc = hw_mp3336_off(&hw_mp3336_ctrl);
        if (rc < 0) {
            cam_err("%s pmu_led off error.", __func__);
            return;
        }
    } else {
        cdata.mode = ((brightness-1) / TORCH_BRIGHT_LEVEL) + TORCH_MODE; //1~3[TORCH_MODE] 4~6[TORCH_LEFT_MODE] 7~9[TORCH_RIGHT_MODE]
        cdata.data =((brightness-1) % TORCH_BRIGHT_LEVEL);//level 0,1,2

        cam_info("%s brightness=0x%x, mode=%d, data=%d.", __func__, brightness, cdata.mode, cdata.data);

        rc = hw_mp3336_brightness(&hw_mp3336_ctrl, &cdata);
        if (rc < 0) {
            cam_err("%s pmu_led on error.",__func__);
            return;
        }
    }
}

static struct device_attribute hw_mp3336_lightness =
__ATTR(lightness, 0664, hw_mp3336_lightness_show, hw_mp3336_lightness_store);//wr+wr+r


static struct device_attribute hw_mp3336_flash_lightness =
__ATTR(flash_lightness, 0664, hw_mp3336_lightness_show, hw_mp3336_flash_lightness_store);//664:wr+wr+r

static struct device_attribute hw_mp3336_dual_leds =
__ATTR(dual_leds, 0664, hw_mp3336_dual_leds_show, hw_mp3336_dual_leds_store);//wr+wr+r

static struct device_attribute hw_mp3336_flash_mask =
__ATTR(flash_mask, 0664, hw_mp3336_flash_mask_show, hw_mp3336_flash_mask_store);//wr+wr+r

static int hw_mp3336_register_attribute(struct hw_flash_ctrl_t *flash_ctrl, struct device *dev)
{
    int rc = 0;

    if ((NULL == flash_ctrl) || (NULL == dev)) {
        cam_err("%s flash_ctrl or dev is NULL.", __func__);
        return -1;
    }

    flash_ctrl->cdev_torch.name = "torch";
    flash_ctrl->cdev_torch.max_brightness
        = MAX_BRIGHTNESS_FORMMI;
    flash_ctrl->cdev_torch.brightness_set = hw_mp3336_torch_brightness_set;
    rc = led_classdev_register((struct device *)dev, &flash_ctrl->cdev_torch);
    if (rc < 0) {
        cam_err("%s failed to register torch classdev.", __func__);
        goto err_out;
    }

    rc = device_create_file(dev, &hw_mp3336_lightness);
    if (rc < 0) {
        cam_err("%s failed to creat lightness attribute.", __func__);
        goto err_create_lightness_file;
    }
    rc = device_create_file(dev, &hw_mp3336_flash_lightness);
    if (rc < 0) {
        cam_err("%s failed to creat flash_lightness attribute.", __func__);
        goto err_create_flash_lightness_file;
    }

    rc = device_create_file(dev, &hw_mp3336_flash_mask);
    if (rc < 0) {
        cam_err("%s failed to creat flash_mask attribute.", __func__);
        goto err_create_flash_mask_file;
    }
    return 0;
err_create_flash_mask_file:
    device_remove_file(dev, &hw_mp3336_flash_lightness);
err_create_flash_lightness_file:
    device_remove_file(dev, &hw_mp3336_lightness);
err_create_lightness_file:
    led_classdev_unregister(&flash_ctrl->cdev_torch);
err_out:
    return rc;
}

extern int register_camerafs_attr(struct device_attribute *attr);
static int hw_mp3336_match(struct hw_flash_ctrl_t *flash_ctrl)
{
    struct hw_flash_i2c_client *i2c_client;
    struct hw_flash_i2c_fn_t *i2c_func;
    struct hw_mp3336_private_data_t *pdata;
    unsigned char id = 0;

    cam_debug("%s enter.", __func__);

    if (NULL == flash_ctrl) {
        cam_err("%s flash_ctrl is NULL.", __func__);
        return -1;
    }

    i2c_client = flash_ctrl->flash_i2c_client;
    if (NULL == i2c_client) {
        cam_err("%s i2c_client is NULL.", __func__);
        return -1;
    }
    i2c_func = i2c_client->i2c_func_tbl;
    pdata = (struct hw_mp3336_private_data_t *)flash_ctrl->pdata;
    if ((NULL == i2c_func) || (NULL == pdata)) {
        cam_err("%s i2c_func or pdata is NULL.", __func__);
        return -1;
    }

    /* Enable mp3336 switch to standby current is 10ua,
     * if match id success, reset pin will always be enabled.
     */
    hw_mp3336_set_pin_reset(flash_ctrl, HIGH);
    i2c_func->i2c_read(i2c_client, REG_CHIPID, &id);
    cam_info("%s id=0x%x.", __func__, id);
    id = id & CHIP_ID_MASK;
    if (id != CHIP_ID) {
        cam_err("%s match error, id(0x%x) != 0x%x.",
        __func__, id, CHIP_ID);

        //Enable mp3336 switch to shutdown when matchid fail, current is 1.3ua
        hw_mp3336_set_pin_reset(flash_ctrl,LOW);
        return -1;
    }

    register_camerafs_attr(&hw_mp3336_dual_leds);
    //add for debug only
    register_camerafs_attr(&hw_mp3336_lightness);
    register_camerafs_attr(&hw_mp3336_flash_lightness);
    return 0;
}

static int hw_mp3336_remove(struct i2c_client *client)
{
    cam_debug("%s enter.", __func__);
    if (NULL == client) {
        cam_err("%s client is NULL.", __func__);
        return -1;
    }
    hw_mp3336_ctrl.func_tbl->flash_exit(&hw_mp3336_ctrl);

    client->adapter = NULL;
    return 0;
}

static const struct i2c_device_id hw_mp3336_id[] = {
    {"mp3336", (unsigned long)&hw_mp3336_ctrl},
    {}
};

static const struct of_device_id hw_mp3336_dt_match[] = {
    {.compatible = "huawei,mp3336"},
    {}
};
MODULE_DEVICE_TABLE(of, mp3336_dt_match);

static struct i2c_driver hw_mp3336_i2c_driver = {
    .probe  = hw_flash_i2c_probe,
    .remove = hw_mp3336_remove,
    .id_table   = hw_mp3336_id,
    .driver = {
    .name = "hw_mp3336",
    .of_match_table = hw_mp3336_dt_match,
    },
};

static int __init hw_mp3336_module_init(void)
{
    cam_info("%s erter.", __func__);
    return i2c_add_driver(&hw_mp3336_i2c_driver);
}

static void __exit hw_mp3336_module_exit(void)
{
    cam_info("%s enter.", __func__);
    i2c_del_driver(&hw_mp3336_i2c_driver);
    return;
}

static struct hw_flash_i2c_client hw_mp3336_i2c_client;

static struct hw_flash_fn_t hw_mp3336_func_tbl = {
    .flash_config = hw_flash_config,
    .flash_init = hw_mp3336_init,
    .flash_exit = hw_mp3336_exit,
    .flash_on = hw_mp3336_on,
    .flash_off = hw_mp3336_off,
    .flash_match = hw_mp3336_match,
    .flash_get_dt_data = hw_mp3336_get_dt_data,
    .flash_register_attribute = hw_mp3336_register_attribute,
};

struct hw_flash_ctrl_t hw_mp3336_ctrl = {
    .flash_i2c_client = &hw_mp3336_i2c_client,
    .func_tbl = &hw_mp3336_func_tbl,
    .hw_flash_mutex = &flash_mut_mp3336,
    .pdata = (void*)&hw_mp3336_pdata,
    .flash_mask_enable = true,
    .state = {
        .mode = STANDBY_MODE,
    },
};

#ifdef CONFIG_LLT_TEST

struct UT_TEST_MP3336 UT_mp3336 =
{
    .hw_mp3336_set_pin_strobe   = hw_mp3336_set_pin_strobe,
    .hw_mp3336_set_pin_torch   = hw_mp3336_set_pin_torch,
    .hw_mp3336_set_pin_reset   = hw_mp3336_set_pin_reset,
    .hw_mp3336_init = hw_mp3336_init,
    .hw_mp3336_exit = hw_mp3336_exit,
    .hw_mp3336_flash_mode = hw_mp3336_flash_mode,
    .hw_mp3336_torch_mode_mmi   = hw_mp3336_torch_mode_mmi,
    .hw_mp3336_set_mode =hw_mp3336_set_mode,
    .hw_mp3336_on =hw_mp3336_on,
    .hw_mp3336_brightness = hw_mp3336_brightness,
    .hw_mp3336_off = hw_mp3336_off,
    .hw_mp3336_get_dt_data = hw_mp3336_get_dt_data,
    .hw_mp3336_dual_leds_show =hw_mp3336_dual_leds_show,
    .hw_mp3336_dual_leds_store =hw_mp3336_dual_leds_store,
    .hw_mp3336_lightness_store = hw_mp3336_lightness_store,
    .hw_mp3336_lightness_show = hw_mp3336_lightness_show,
    .hw_mp3336_flash_lightness_store = hw_mp3336_lightness_store,
    .hw_mp3336_flash_mask_show = hw_mp3336_flash_mask_show,
    .hw_mp3336_register_attribute = hw_mp3336_register_attribute,
    .hw_mp3336_flash_mask_store = hw_mp3336_flash_mask_store,
    .hw_mp3336_match = hw_mp3336_match,
    .hw_mp3336_torch_brightness_set = hw_mp3336_torch_brightness_set,
    .hw_mp3336_param_check = hw_mp3336_param_check,
};

#endif /* CONFIG_LLT_TEST */

module_init(hw_mp3336_module_init);
module_exit(hw_mp3336_module_exit);
MODULE_DESCRIPTION("MP3336 FLASH");
MODULE_LICENSE("GPL v2");
//lint -restore
