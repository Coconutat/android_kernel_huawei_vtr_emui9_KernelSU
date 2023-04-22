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

#include "hw_pmic.h"
#include "../cam_log.h"
#include <media/huawei/hw_extern_pmic.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif

//lint -save -e31
//lint -save -e568 -e456 -e454
//lint -esym(568,*)

/* NCP6925 Registers define */
#define CHIP_REV        0x00
#define EXTID           0x01
#define REARM_ID        0x2c
#define BUCK1_VOUT      0x02 /* BUCK VOUT=0.6V+BUCKx_VOUT[7:0]*0.0125      (0.6V to 2.1875V) */
#define BUCK2_VOUT      0x03
#define BUCK_VSEL       0x04
#define LDO1_VOUT       0x05 /* LDO VOUT = 0.5V + LDOx_VOUT[6:0] * 0.025V  ( Except  LDOx_VOUT[6:0] = 0x7F ) */
#define LDO2_VOUT       0x06
#define LDO3_VOUT       0x07
#define LDO4_VOUT       0x08
#define LDO5_VOUT       0x09
#define CHX_ERR         0x0a
#define CHX_EN          0x0b
//#define BUCK_SEQ_1_2  0x0c
//#define LDO_SEQ_1_2   0x0d
//#define LDO_SEQ_3_4   0x0e
//#define LDO_SEQ_5     0x0f
//#define SEQ_SPEED     0x10

#define INT_ACK1        0x23
#define INT_ACK2        0x24
#define INT_ACK3        0x25

#define BUCK_MIN        600000
#define BUCK_MAX        3012500
#define BUCK_TI_MAX     2187500
#define LDO_MIN         600000
#define LDO_MAX         3500000

#define BUCK1_ENABLE    (1<<0)
#define BUCK2_ENABLE    (1<<1)
#define LDO1_ENABLE     (1<<2)
#define LDO2_ENABLE     (1<<3)
#define LDO3_ENABLE     (1<<4)
#define LDO4_ENABLE     (1<<5)
#define LDO5_ENABLE     (1<<6)

/* #define MAX_ATTRIBUTE_BUFFER_SIZE       128 */


#define INVALID_GPIO                    999

#define TI_REARM    (0xff)
#define ON_REARM    (0x01)

#define PMIC_1P8V               1800000
#define PMIC_3P3V               3300000
#define PMIC_LDO2_INDEX     1
#define PMIC_LDO3_INDEX     2

#define PMIC_POWER_ON  1
#define PMIC_POWER_OFF 0

/* Internal data struct define */
typedef enum {
    PMIC_POWER_CTRL = 0,
    GPIO_CTRL_0,
    GPIO_CTRL_1,
    MAX_PIN,
}ncp6925_pin_type;

struct ncp6925_private_data_t {

    /* pmic control pin */
    unsigned int pin[MAX_PIN];
    unsigned int voltage[VOUT_MAX];
    unsigned int chipid;
    u8 rearm_id;
};

typedef struct {
    int chx_enable;
    int vout_reg;
}voltage_map_t;


static struct ncp6925_private_data_t ncp6925_pdata;
static struct i2c_driver ncp6925_i2c_driver;
struct hisi_pmic_ctrl_t ncp6925_ctrl;
static int ncp6925_poweron = 0;
static int pmic_enable_sensor_1v8_flag = 0;
static int pmic_enable_sensor_3v3_flag = 0;
static int pmic_extern_config_flag = 0;
static int pmic_lon_flag = 0;
extern struct dsm_client *client_pmic;
static voltage_map_t voltage_map[VOUT_MAX] =
{
    {LDO1_ENABLE, LDO1_VOUT},
    {LDO2_ENABLE, LDO2_VOUT},
    {LDO3_ENABLE, LDO3_VOUT},
    {LDO4_ENABLE, LDO4_VOUT},
    {LDO5_ENABLE, LDO5_VOUT},
    {BUCK1_ENABLE, BUCK1_VOUT},
    {BUCK2_ENABLE, BUCK2_VOUT},
};

struct pmic_cfg_data cdata = {0};
u8 reg = 0;
u8 val = 0;

DEFINE_HISI_PMIC_MUTEX(ncp6925);

static int calc_buck_vlotage_ext(struct hisi_pmic_ctrl_t *pmic_ctrl, u32 in, u8 *out);
static int calc_ldo_vlotage(u32 in, u8 *out);
static int ncp6925_seq_config(struct hisi_pmic_ctrl_t *pmic_ctrl, pmic_seq_index_t seq_index, u32 voltage, int state);
static int ncp6925_on(struct hisi_pmic_ctrl_t *pmic_ctrl, void *data);

int hw_extern_pmic_config(int index, int voltage, int enable)
{
    struct hisi_pmic_ctrl_t *pmic_ctrl = NULL;
    if ((pmic_extern_config_flag == 0 && ncp6925_poweron == PMIC_POWER_ON) || (1 == pmic_lon_flag)){
        cam_debug("using pmic extern config");
        pmic_ctrl = hisi_get_pmic_ctrl();
        if(pmic_ctrl != NULL) {
            cam_debug("pmic power on!");
            pmic_ctrl->func_tbl->pmic_on(pmic_ctrl, 0);
        } else {
            cam_err("pmic_ctrl is NULL,just return");
            return -1;
        }
        if (ncp6925_poweron != PMIC_POWER_ON) {
            cam_err("ncp6925 do not power on, return");
            return -1;
        }
        return ncp6925_seq_config(&ncp6925_ctrl, (pmic_seq_index_t)index, (u32)voltage, enable);
    } else {
        cam_info("pmic extern config disable");
        return 0;
    }
}

int hw_extern_pmic_query_state(int index, int *state)
{
    u8 chx_enable = 0;
    u8 chx_enable_tmp = 0;
    struct hisi_pmic_i2c_client *i2c_client = NULL;
    struct hisi_pmic_i2c_fn_t *i2c_func = NULL;
    pmic_seq_index_t seq_index = (pmic_seq_index_t)index;
    struct hisi_pmic_ctrl_t *pmic_ctrl = hisi_get_pmic_ctrl();

    if (pmic_ctrl == NULL) {
        cam_err("pmic_ctrl is NULL,just return");
        return -1;
    }

    i2c_client = pmic_ctrl->pmic_i2c_client;
    i2c_func = pmic_ctrl->pmic_i2c_client->i2c_func_tbl;

    chx_enable = voltage_map[seq_index].chx_enable;
    i2c_func->i2c_read(i2c_client, CHX_EN, &chx_enable_tmp);
    if (state == NULL) {
        cam_err("state is NULL,just return");
        return -1;
    }
    *state = chx_enable & chx_enable_tmp;
     cam_debug("hw_extern_pmic_query_state chx_enable:%d,chx_enable_tmp:%d,state:%d",
         chx_enable,chx_enable_tmp,*state);

    return 0;
}

static int ncp6925_remove(struct i2c_client *client)
{
    cam_debug("%s enter.", __func__);

    client->adapter = NULL;
    return 0;
}

static int ncp6925_init(struct hisi_pmic_ctrl_t *pmic_ctrl)
{
    struct hisi_pmic_i2c_client *i2c_client;
    struct hisi_pmic_i2c_fn_t *i2c_func;
    struct ncp6925_private_data_t *pdata;
    u8 device_id = 0;
    u8 external_id = 0;
    int ret = 0;

    cam_debug("%s enter.", __func__);

    if (NULL == pmic_ctrl) {
        cam_err("%s pmic_ctrl is NULL.", __func__);
        return -1;
    }

    pmic_ctrl->pctrl = devm_pinctrl_get_select(pmic_ctrl->dev,
        PINCTRL_STATE_DEFAULT);

    if (NULL == pmic_ctrl->pctrl) {
        cam_err("%s failed to set pin.", __func__);
        return -EIO;
    }

    pdata = (struct ncp6925_private_data_t *)pmic_ctrl->pdata;

    cam_debug("%s PMIC_POWER_CTRL = %d", __func__,
            pdata->pin[PMIC_POWER_CTRL]);

    ret = gpio_request(pdata->pin[PMIC_POWER_CTRL], "pmic-power-ctrl");
    if (ret < 0) {
        cam_err("%s failed to request pmic-power-ctrl pin.", __func__);
        goto err1;
    }

    gpio_direction_output(pdata->pin[PMIC_POWER_CTRL], 1);

    msleep(10);

    if (pdata->pin[GPIO_CTRL_0] != INVALID_GPIO) {
        ret = gpio_request(pdata->pin[GPIO_CTRL_0], "pmic-gpio-ctrl-0");
        if (ret < 0) {
            cam_err("%s failed to request gpio ctrl 0.", __func__);
            goto err1;
        }

        gpio_direction_output(pdata->pin[GPIO_CTRL_0], 1);
    }

    if (pdata->pin[GPIO_CTRL_1] != INVALID_GPIO) {
        ret = gpio_request(pdata->pin[GPIO_CTRL_1], "pmic-gpio-ctrl-1");
        if (ret < 0) {
            cam_err("%s failed to request gpio ctrl 1.", __func__);
            goto err2;
        }

        gpio_direction_output(pdata->pin[GPIO_CTRL_1], 1);
    }

    i2c_client = pmic_ctrl->pmic_i2c_client;
    i2c_func = pmic_ctrl->pmic_i2c_client->i2c_func_tbl;

    ret = i2c_func->i2c_read(i2c_client, CHIP_REV, &device_id);
    if (ret < 0) {
        cam_err("%s: read CHIP_REV failed, ret = %d ", __func__, ret);
        goto err3;
    }
    cam_debug("%s device id=%d", __func__,
            device_id);

    ret = i2c_func->i2c_read(i2c_client, EXTID, &external_id);
    if (ret < 0) {
        cam_err("%s: read EXTID failed, ret = %d ", __func__, ret);
        goto err3;
    }
    cam_debug("%s external id=%d", __func__,
            external_id);

    ret = i2c_func->i2c_read(i2c_client, REARM_ID, &pdata->rearm_id);
    if (ret < 0) {
        cam_err("%s: read EXTID failed, ret = %d ", __func__, ret);
        goto err3;
    }
    cam_debug("%s rearm id=%d", __func__, pdata->rearm_id);

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
    if(device_id == 0 && external_id == 0xff) {
        set_hw_dev_flag(DEV_I2C_CAMERA_PMIC);
    }
#endif

    /***************************
    *ldo3 3.3v
    * Chicago		  Sensor/fp 3.3v
    ****************************/
    if (pmic_enable_sensor_3v3_flag) {
        ncp6925_on(&ncp6925_ctrl, pdata);
        ncp6925_seq_config(&ncp6925_ctrl, (pmic_seq_index_t)PMIC_LDO3_INDEX, PMIC_3P3V, 1);
        cam_debug("%s set LDO3=3V3", __func__);
    }
    /***************************
    *ldo2 1.8v
    * Chicago		  Sensor 1.8v
    ****************************/
    if(pmic_enable_sensor_1v8_flag == 1){
        ncp6925_on(&ncp6925_ctrl, pdata);
        ncp6925_seq_config(&ncp6925_ctrl, (pmic_seq_index_t)PMIC_LDO2_INDEX, PMIC_1P8V, 1);
        cam_debug("%s set LDO2=1V8", __func__);
    }

    return ret;
err3:
    gpio_free(pdata->pin[GPIO_CTRL_1]);
err2:
    gpio_free(pdata->pin[GPIO_CTRL_0]);
err1:
    gpio_free(pdata->pin[PMIC_POWER_CTRL]);
    return -EIO;
}

static int ncp6925_exit(struct hisi_pmic_ctrl_t *pmic_ctrl)
{
    struct ncp6925_private_data_t *pdata;

    cam_debug("%s enter.", __func__);

    if (NULL == pmic_ctrl) {
        cam_err("%s pmic_ctrl is NULL.", __func__);
        return -1;
    }

    pdata = (struct ncp6925_private_data_t *)pmic_ctrl->pdata;
    gpio_free(pdata->pin[PMIC_POWER_CTRL]);
    pmic_ctrl->pctrl = devm_pinctrl_get_select(pmic_ctrl->dev,
        PINCTRL_STATE_IDLE);
    return 0;
}

static int ncp6925_on(struct hisi_pmic_ctrl_t *pmic_ctrl, void *data)
{
    struct hisi_pmic_i2c_client *i2c_client;
    struct hisi_pmic_i2c_fn_t *i2c_func;
    struct ncp6925_private_data_t *pdata;
    int gpio_value = 0;

    cam_debug("%s enter.", __func__);

    if (NULL == pmic_ctrl) {
        cam_err("%s pmic_ctrl is NULL.", __func__);
        return -1;
    }
    if (ncp6925_poweron == 1)
        return 0;

    pdata = (struct ncp6925_private_data_t *)pmic_ctrl->pdata;
    i2c_client = pmic_ctrl->pmic_i2c_client;
    i2c_func = pmic_ctrl->pmic_i2c_client->i2c_func_tbl;

    gpio_value = gpio_get_value_cansleep(pdata->pin[PMIC_POWER_CTRL]);
    cam_debug("%s: pmic enable gpio value = %d.", __func__, gpio_value);
    if(0 == gpio_value) {
        if(!dsm_client_ocuppy(client_pmic)) {
            dsm_client_record(client_pmic, "pmic enable failed! gpio value = %d\n", gpio_value);
            dsm_client_notify(client_pmic, DSM_CAMPMIC_ENABLE_ERROR_NO);
            cam_warn("[I/DSM] %s dsm_client_notify", client_pmic->client_name);
        }
    }

    i2c_func->i2c_write(i2c_client, CHX_ERR, 0x00);
    ncp6925_poweron = 1;
    return 0;
}

static int ncp6925_off(struct hisi_pmic_ctrl_t *pmic_ctrl)
{
    struct ncp6925_private_data_t *pdata;
    cam_debug("%s enter.", __func__);

    if (NULL == pmic_ctrl) {
        cam_err("%s pmic_ctrl is NULL.", __func__);
        return -1;
    }

    pdata = (struct ncp6925_private_data_t *)pmic_ctrl->pdata;
    if (NULL == pdata){
        cam_err("%s pdata is NULL.", __func__);
        return -1;
    }
    ncp6925_poweron = 0;

    return 0;
}

static int ncp6925_check_state_exception(struct hisi_pmic_ctrl_t *pmic_ctrl)
{
    struct hisi_pmic_i2c_client *i2c_client;
    struct hisi_pmic_i2c_fn_t *i2c_func;
    u8 reg_value = 0;

    if (NULL == pmic_ctrl) {
        cam_err("%s pmic_ctrl is NULL.", __func__);
        return -1;
    }

    i2c_client = pmic_ctrl->pmic_i2c_client;
    i2c_func = pmic_ctrl->pmic_i2c_client->i2c_func_tbl;

    // TSD warning: INT_ACK3:00000100
    i2c_func->i2c_read(i2c_client, INT_ACK3, &reg_value);
    cam_debug("%s: pmic INT_ACK3 = %d.", __func__, reg_value);
    if ((reg_value & 0x04) == 0x04) {
        if (!dsm_client_ocuppy(client_pmic)) {
            dsm_client_record(client_pmic, "NCP6925: thermal shutdown error.\n");
            dsm_client_notify(client_pmic, DSM_CAMPMIC_TSD_ERROR_NO);
            cam_warn("[I/DSM] %s : NCP6925 thermal shutdown error.", client_pmic->client_name);
        }
    }

    // under voltage lock
    reg_value = 0;
    i2c_func->i2c_read(i2c_client, INT_ACK1, &reg_value);
    cam_debug("%s: pmic INT_ACK1 = %d.", __func__, reg_value);
    if (reg_value != 0) {
        if (!dsm_client_ocuppy(client_pmic)) {
            dsm_client_record(client_pmic, "NCP6925: under voltage threshold error.\n");
            dsm_client_notify(client_pmic, DSM_CAMPMIC_UNDER_VOLTAGE_ERROR_NO);
            cam_warn("[I/DSM] %s : NCP6925 under voltage lock.", client_pmic->client_name);
        }
    }

    // over current
    reg_value = 0;
    i2c_func->i2c_read(i2c_client, INT_ACK2, &reg_value);
    cam_debug("%s: pmic INT_ACK2 = %d.", __func__, reg_value);
    if (reg_value != 0) {
        if (!dsm_client_ocuppy(client_pmic)) {
            dsm_client_record(client_pmic, "NCP6925: over current error.\n");
            dsm_client_notify(client_pmic, DSM_CAMPMIC_OVER_CURRENT_ERROR_NO);
            cam_warn("[I/DSM] %s : NCP6925 over current error.", client_pmic->client_name);
        }
    }

    return 0;
}

static int twl80125_check_state_exception(struct hisi_pmic_ctrl_t *pmic_ctrl)
{
    struct hisi_pmic_i2c_client *i2c_client;
    struct hisi_pmic_i2c_fn_t *i2c_func;
    u8 reg_value = 0;

    if (NULL == pmic_ctrl) {
        cam_err("%s pmic_ctrl is NULL.", __func__);
        return -1;
    }

    i2c_client = pmic_ctrl->pmic_i2c_client;
    i2c_func = pmic_ctrl->pmic_i2c_client->i2c_func_tbl;

    i2c_func->i2c_read(i2c_client, CHX_ERR, &reg_value);
    cam_debug("%s: pmic CHX_ERR = %d.", __func__, reg_value);
    if ((reg_value & 0x0E) == 0x0E) {
        if(dsm_client_ocuppy(client_pmic)) {
            dsm_client_record(client_pmic, "TWL80125: thermal shutdown error.\n");
            dsm_client_notify(client_pmic, DSM_CAMPMIC_TSD_ERROR_NO);
            cam_warn("[I/DSM] %s : TWL80125 thermal shutdown error.", client_pmic->client_name);
        }
    }

    i2c_func->i2c_write(i2c_client, CHX_ERR, 0x00);
    return 0;
}

static int pmic_check_state_exception(struct hisi_pmic_ctrl_t *pmic_ctrl)
{
    struct ncp6925_private_data_t *pdata;
    if (NULL == pmic_ctrl) {
        cam_err("%s pmic_ctrl is NULL.", __func__);
        return -1;
    }
    pdata = (struct ncp6925_private_data_t *)pmic_ctrl->pdata;
    if(NULL == pdata) {
        cam_err("%s pdata is NULL", __func__);
        return -1;
    }

    if (pdata->rearm_id == ON_REARM) {
        cam_debug("%s: pmic rearm_id(ON_REARM): %u", __func__, pdata->rearm_id);
        ncp6925_check_state_exception(pmic_ctrl);
    } else if (pdata->rearm_id == TI_REARM) {
        cam_debug("%s: pmic rearm_id(TI_REARM): %u", __func__, pdata->rearm_id);
        twl80125_check_state_exception(pmic_ctrl);
    } else {
        cam_err("InvalidArgument chipid %u", pdata->rearm_id);
    }
    return 0;
}


static int ncp6925_match(struct hisi_pmic_ctrl_t *pmic_ctrl)
{
    cam_debug("%s enter.", __func__);
    return 0;
}

static void ncp6925_get_extern_cfg(struct device_node* dev_node)
{
    int rc = -1;
    int pmic_ex_config = 0;

    rc = of_property_read_u32(dev_node, "hisi,pmic_ex_func_disable",
            (u32 *)&pmic_ex_config);
    if (rc < 0) {
        cam_debug("%s can not read pmic_ex_func_disable\n", __func__);
    } else {
        pmic_extern_config_flag = pmic_ex_config;
        cam_debug("%s pmic_ex_func_disable:%d\n", __func__, pmic_ex_config);
    }
}

static int ncp6925_get_dt_data(struct hisi_pmic_ctrl_t *pmic_ctrl)
{
    struct ncp6925_private_data_t *pdata;
    struct device_node *dev_node;
    int i;
    int rc = -1;
    int pmic_sensor_1v8 = 0;
    int pmic_sensor_3v3 = 0;
    struct hisi_pmic_info *pmic_info;

    cam_info("%s enter.\n", __func__);

    if (NULL == pmic_ctrl) {
        cam_err("%s pmic_ctrl is NULL.", __func__);
        return rc;
    }

    pmic_info = &pmic_ctrl->pmic_info;
    pdata = (struct ncp6925_private_data_t *)pmic_ctrl->pdata;
    dev_node = pmic_ctrl->dev->of_node;

    rc = of_property_read_u32_array(dev_node, "hisi,pmic-pin",
            pdata->pin, MAX_PIN);
    if (rc < 0) {
        cam_err("%s failed line %d\n", __func__, __LINE__);
        return rc;
    } else {
        for (i=0; i<MAX_PIN; i++) {
            cam_debug("%s pin[%d]=%d.\n", __func__, i,
                    pdata->pin[i]);
        }
    }

    rc = of_property_read_u32_array(dev_node, "hisi,pmic-voltage",
            pdata->voltage, VOUT_MAX);
    if (rc < 0) {
        cam_err("%s failed line %d\n", __func__, __LINE__);
        return rc;
    } else {
        for (i=0; i<VOUT_MAX; i++) {
            cam_debug("%s voltage[%d]=%d.\n", __func__, i,
                    pdata->voltage[i]);
        }
    }

    ncp6925_get_extern_cfg(dev_node);

    rc = of_property_read_u32(dev_node, "hisi,pmic_sensor_1v8",
        (u32 *)&pmic_sensor_1v8);
    if (rc < 0)
        cam_debug("%s not read pmic_sensor_1v8\n", __func__);
    else
        cam_debug("%s pmic_sensor_1v8:%d\n", __func__,pmic_sensor_1v8);

    rc = of_property_read_u32(dev_node, "hisi,pmic_sensor_3v3", (u32 *)&pmic_sensor_3v3);
    if (rc < 0)
        cam_debug("%s not read pmic_sensor_3v3\n", __func__);
    else
        cam_debug("%s pmic_sensor_3v3:%d\n", __func__,pmic_sensor_3v3);

    if(pmic_sensor_1v8 == 1)
        pmic_enable_sensor_1v8_flag = 1;
    if (pmic_sensor_3v3)
        pmic_enable_sensor_3v3_flag = 1;

    if(of_property_read_u32(dev_node, "hisi,pmic_mutex_flag",(u32 *)&pmic_info->mutex_flag)){
        cam_err("%s read pmic_mutex_flag failed %d\n", __func__, __LINE__);
    }else{
        pmic_lon_flag = 1;
        cam_info("%s hisi,pmic_mutex_flag %d\n", __func__,
        pmic_info->mutex_flag);
    }

    return 0;
}

static int calc_buck_vlotage_on(u32 in, u8 *out)
{
    *out = ((in - 600000) / 12500) & 0xff;
    return 0;
}

static int calc_buck_vlotage_ti(u32 in, u8 *out)
{
    if (in <= BUCK_TI_MAX)
    {
        *out = ((in - 600000) / 12500) & 0xff;
    } else {
        *out = (((in * 5 / 8  - 600000) / 12500) & 0xff) | 0x80;
    }

    return 0;
}

static int calc_buck_vlotage_ext(struct hisi_pmic_ctrl_t *pmic_ctrl,
                                 u32 in,
                                 u8 *out)
{
    struct ncp6925_private_data_t *pdata = NULL;
    int ret = -1;
    cam_debug("%s enter.", __func__);
    if (pmic_ctrl == NULL || out == NULL)
    {
        cam_err("invalid arguments");
        return ret;
    }

    if (in <= BUCK_MIN || in >= BUCK_MAX) {
        cam_err("do not support the buck voltage, %u", in);
        return ret;
    }

    pdata = (struct ncp6925_private_data_t *)pmic_ctrl->pdata;
    if (pdata->rearm_id == ON_REARM) {
        ret = calc_buck_vlotage_on(in, out);
    } else if (pdata->rearm_id == TI_REARM) {
        ret = calc_buck_vlotage_ti(in, out);
    } else {
        cam_err("InvalidArgument chipid %u", pdata->rearm_id);
    }

    return ret;
}

static int calc_ldo_vlotage(u32 in, u8 *out)
{
    int ret = -1;
    cam_debug("%s enter, ldo in %d", __func__, in);
    if (in <= LDO_MIN || in >= LDO_MAX) {
        cam_err("do not support the ldo voltage");
        return ret;
    }
    *out = ((in - 500000) / 25000) & 0xff;
    return ret;
}

static int ncp6925_seq_config(struct hisi_pmic_ctrl_t *pmic_ctrl, pmic_seq_index_t seq_index, u32 voltage, int state)
{
    u8 chx_enable_tmp = 0;
    u8 chx_enable = 0;
    u8 voltage_reg = 0;
    u8 voltage_val = 0;
    u8 buck12_prog = 0;
    u8 buck12_prog_old = 0;
    struct hisi_pmic_i2c_client *i2c_client;
    struct hisi_pmic_i2c_fn_t *i2c_func;
    int ret = 0;

    cam_debug("%s enter.", __func__);

    if (NULL == pmic_ctrl) {
        cam_err("%s pmic_ctrl is NULL.", __func__);
        return -1;
    }

    if(VOUT_MAX <= seq_index)
    {
        cam_err("%s seq_index out of range.", __func__);
        return -1;
    }

    i2c_client = pmic_ctrl->pmic_i2c_client;
    i2c_func = pmic_ctrl->pmic_i2c_client->i2c_func_tbl;
    if(pmic_ctrl->pmic_info.mutex_flag == 1){
        mutex_lock(pmic_ctrl->hisi_pmic_mutex);
    }

    chx_enable = voltage_map[seq_index].chx_enable;
    voltage_reg = voltage_map[seq_index].vout_reg;

    i2c_func->i2c_read(i2c_client, CHX_EN, &chx_enable_tmp);

    if (state == 1) {
        if (seq_index >= VOUT_BUCK_1) {
            calc_buck_vlotage_ext(pmic_ctrl, voltage, &voltage_val);
            buck12_prog = (voltage_val & 0x80)>>7;
            buck12_prog = (seq_index == VOUT_BUCK_1 ? buck12_prog : buck12_prog << 1);
            voltage_val &= 0x7F;
            cam_debug("%s, buck[6:0] voltage 0x%x", __func__, voltage_val);
        } else {
            calc_ldo_vlotage(voltage, &voltage_val);
        }

        i2c_func->i2c_write(i2c_client, voltage_reg, voltage_val);

        /* write Buck12_prog with Buckx_VOUT[7]
         *      D1           D0
         * BUCK2_VOUT[7] BUCK1_VOUT[7]
         * */
        if (seq_index >= VOUT_BUCK_1) {
            i2c_func->i2c_read(i2c_client, BUCK_VSEL, &buck12_prog_old);
            buck12_prog |= buck12_prog_old;
            i2c_func->i2c_write(i2c_client, BUCK_VSEL, buck12_prog);
            cam_debug("%s buck_vsel 0x%x", __func__, buck12_prog);
        }
        mdelay(1);
        i2c_func->i2c_write(i2c_client, CHX_EN, chx_enable_tmp | chx_enable);
        cam_debug("%s chx_enable 0x%x, voltage_reg 0x%x, voltage_val 0x%x", __func__, chx_enable, voltage_reg, voltage_val);
    } else {
        i2c_func->i2c_write(i2c_client, CHX_EN, chx_enable_tmp & (~chx_enable));
        //i2c_func->i2c_write(i2c_client, voltage_reg, state);
        if (seq_index >= VOUT_BUCK_1) {
            i2c_func->i2c_read(i2c_client, BUCK_VSEL, &buck12_prog_old);
            buck12_prog = buck12_prog_old & (~(seq_index == VOUT_BUCK_1 ? 0x1 : 0x2));
            i2c_func->i2c_write(i2c_client, BUCK_VSEL, buck12_prog);
        }
    }

    i2c_func->i2c_read(i2c_client, CHX_ERR, &chx_enable_tmp);
    if (chx_enable_tmp != 0) {
        cam_err("%s PMIC CHX_ERR code 0x%x", __func__, chx_enable_tmp);
    }
    if(pmic_ctrl->pmic_info.mutex_flag == 1){
        mutex_unlock(pmic_ctrl->hisi_pmic_mutex);
    }

    return ret;
}



static int ncp6925_register_attribute(struct hisi_pmic_ctrl_t *pmic_ctrl, struct device *dev)
{


    return 0;

}


static const struct i2c_device_id ncp6925_id[] = {
    {"ncp6925", (unsigned long)&ncp6925_ctrl},
    {}
};

static const struct of_device_id ncp6925_dt_match[] = {
    {.compatible = "hisi,ncp6925"},
    {}
};

MODULE_DEVICE_TABLE(of, ncp6925_dt_match);

static struct i2c_driver ncp6925_i2c_driver = {
    .probe    = hisi_pmic_i2c_probe,
    .remove = ncp6925_remove,
    .id_table    = ncp6925_id,
    .driver = {
        .name = "ncp6925",
        .of_match_table = ncp6925_dt_match,
    },
};

static int __init ncp6925_module_init(void)
{
    cam_info("%s enter.\n", __func__);
    return i2c_add_driver(&ncp6925_i2c_driver);
}

static void __exit ncp6925_module_exit(void)
{
    cam_info("%s enter.", __func__);
    i2c_del_driver(&ncp6925_i2c_driver);
    return;
}

static struct hisi_pmic_i2c_client ncp6925_i2c_client;

static struct hisi_pmic_fn_t ncp6925_func_tbl = {
    .pmic_config = hisi_pmic_config,
    .pmic_init = ncp6925_init,
    .pmic_exit = ncp6925_exit,
    .pmic_on = ncp6925_on,
    .pmic_off = ncp6925_off,
    .pmic_match = ncp6925_match,
    .pmic_get_dt_data = ncp6925_get_dt_data,
    .pmic_seq_config = ncp6925_seq_config,
    .pmic_register_attribute = ncp6925_register_attribute,
    .pmic_check_exception = pmic_check_state_exception,
};

struct hisi_pmic_ctrl_t ncp6925_ctrl = {
    .pmic_i2c_client = &ncp6925_i2c_client,
    .func_tbl = &ncp6925_func_tbl,
    .hisi_pmic_mutex = &pmic_mut_ncp6925,
    .pdata = (void*)&ncp6925_pdata,
};

module_init(ncp6925_module_init);
module_exit(ncp6925_module_exit);
MODULE_DESCRIPTION("NCP6925 PMIC");
MODULE_LICENSE("GPL v2");

//lint -restore
