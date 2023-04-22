/*
 * watchdog for preventing the charging work dead
 *
 * Copyright (c) 2013 Huawei Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/mod_devicetable.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/wakelock.h>
#include <linux/kern_levels.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/hisi/rdr_hisi_ap_ringbuffer.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#include <linux/hisi/kirin_partition.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <hisi_partition.h>
#include "securec.h"
#include "polar_table.h"
#define COUL_POLAR_INFO
#ifndef COUL_POLAR_INFO
#define polar_debug(fmt, args...)do {} while (0)
#define polar_info(fmt, args...) do {} while (0)
#define polar_warn(fmt, args...) do {} while (0)
#define polar_err(fmt, args...)  do {} while (0)
#else
#define polar_debug(fmt, args...)do {} while (0)
#define polar_info(fmt, args...) do { printk(KERN_INFO    "[coul_polar]" fmt, ## args); } while (0)
#define polar_warn(fmt, args...) do { printk(KERN_WARNING"[coul_polar]" fmt, ## args); } while (0)
#define polar_err(fmt, args...)  do { printk(KERN_ERR   "[coul_polar]" fmt, ## args); } while (0)
#endif
#define POLAR_FILE_LIMIT       0660
#define POLAR_BUF_SHOW_LEN    (128)
#define POLAR_LUT_SIZE    (0x1000)
#define POLAR_LUT_OFFSET    (0)
static unsigned long polar_sample_interval[POLAR_TIME_ARRAY_NUM] = {
    500,   1500,   3000,   5000,
    10000, 15000,  25000,  35000,
    45000, 65000,  85000,  135000,
    225000,375000, 625000, 1200000
};
static int polar_curr_interval[POLAR_CURR_ARRAY_NUM] = {
    200, 100, 150, 250, 400, 600, 1000, 2000};
static int polar_curr_vector_interval[POLAR_CURR_ARRAY_VECTOR_NUM] = {
    200, 150, 400, 1000};

/*----全极化OCV温度表 ----*/
/*soc(0~100):2.5%一档
55度40度25度10度5度0度-5度-10度-20度-----*/

static int polar_temp_points[POLAR_OCV_PC_TEMP_COLS] = {
    55, 40, 25, 10, 5, 0, -5, -10, -20};
static int polar_pc_points[POLAR_OCV_PC_TEMP_ROWS] = {
    1000,975,950,925,900,
    875,850,825,800,775,
    750,725,700,675,650,
    625,600,575,550,525,
    500,475,450,425,400,
    375,350,325,300,275,
    250,225,200,175,150,
    125,100,75,50,25,0
};

static int polar_resistence_pc_points[POLAR_RES_PC_CURR_ROWS] = {
    950,900,850,800,750,
    700,650,600,550,500,
    450,400,350,300,250,
    200,150,100,50,0
};

static int polar_learn_temp_points[POLAR_LEARN_TEMP_COLS] = {
    55, 40, 25,
    24, 23, 22, 21, 20,
    19, 18, 17, 16, 15,
    14, 13, 12, 11, 10,
     9,  8,  7,  6,  5,
     4,  3,  2,  1,  0,
    -1, -2, -3, -4, -5,
    -6, -7, -8, -9, -10,
    -11, -12, -13, -14, -15,
    -16, -17, -18, -19, -20
};

static int polar_full_res_temp_points[POLAR_OCV_PC_TEMP_COLS] = {
    55, 40, 25, 10, 5, 0, -5, -10, -20};
static struct hisi_polar_device *g_polar_di = NULL;
static polar_res_tbl polar_res_lut = {
    .rows = POLAR_OCV_PC_TEMP_COLS,
    .cols = POLAR_RES_PC_CURR_ROWS,
    .z_lens = POLAR_CURR_ARRAY_NUM,
    .x_array = polar_full_res_temp_points,
    .y_array = polar_resistence_pc_points,
    .z_array = polar_curr_interval,
};
static polar_x_y_z_tbl polar_vector_lut = {
    .rows = POLAR_RES_PC_CURR_ROWS,
    .cols = POLAR_CURR_ARRAY_NUM,
    .z_lens = POLAR_OCV_PC_TEMP_COLS,
    .x_array = polar_resistence_pc_points,
    .y_array = polar_curr_interval,
    .z_array = polar_temp_points,
};
static polar_ocv_tbl polar_ocv_lut = {
    .rows = POLAR_OCV_PC_TEMP_ROWS,
    .cols = POLAR_OCV_PC_TEMP_COLS,
    .percent = polar_pc_points,
    .temp = polar_temp_points,
};

static polar_learn_tbl polar_learn_lut = {
    .rows = POLAR_RES_PC_CURR_ROWS,
    .cols = POLAR_LEARN_TEMP_COLS,
    .x_array = polar_resistence_pc_points,
    .y_array = polar_learn_temp_points,
};
static struct polar_curr_info polar_avg_curr_info
    [POLAR_TIME_ARRAY_NUM][POLAR_CURR_ARRAY_NUM + 1];
static long polar_err_a = POLAR_ERR_A_DEFAULT;
static long polar_err_b1 = 0;
static long polar_err_b2 = 0;

static long polar_err_a_array[POLAR_VALID_A_NUM] = {0};
static int polar_err_a_coe[POLAR_VALID_A_NUM + 1] = {25,25,25,25};

 /*******************************************************
  Function:       polar_linear_interpolate
  Description:   get y according to : y = ax +b
  Input:           (x0,y0) (x1,y1) x
  Output:         NULL
  Return:         y conresponding x
  Remark:       a = (y1 - y0) / (x1 - x0)
********************************************************/
static int polar_linear_interpolate(int y0, int x0, int y1, int x1, int x)
{
    if ((y0 == y1) || (x == x0))
        return y0;
    if ((x1 == x0) || (x == x1))
        return y1;

    return y0 + ((y1 - y0) * (x - x0) / (x1 - x0));
}

/*******************************************************
  Function:        interpolate_find_pos
  Description:     找到数组中最接近x的值
  Input:           数组int *x_array(从大到小排)
                    数组大小int rows
                    插值int x
  Output:           插值后数组中最接近x的row1和row2
  Return:          NA
********************************************************/
static void interpolate_find_pos(int *x_array, int rows, int x,
                                    int *row1, int *row2)
{
    int i;

    if (!x_array || !row1 || !row2)
        return;

    if (rows < 1)
        return;
    *row1 = 0;
    *row2 = 0;
    if (x > x_array[0]) {
        x = x_array[0];
    }
    if (x < x_array[rows - 1]) {
        x = x_array[rows - 1];
    }
    for (i = 0; i < rows; i++) {
        if (x == x_array[i]) {
            *row1 = i;
            *row2 = i;
            return;
        }
        if (x > x_array[i]) {
            if (0 == i)
                return;
            else
                *row1 = i - 1;
            *row2 = i;
            break;
        }
    }
}

/*******************************************************
  Function:        interpolate_find_pos_reverse
  Description:     找到数组中最接近x的值
  Input:           数组int *x_array(从小到大排)
                    数组大小int rows
                    插值int x
  Output:           插值后数组中最接近x的row1和row2
  Return:          NA
********************************************************/
static void interpolate_find_pos_reverse(int *x_array, int rows, int x,
                                    int *row1, int *row2)
{
    int i;

    if (!x_array || !row1 || !row2)
        return;

    if (rows < 1)
        return;
    *row1 = 0;
    *row2 = 0;
    if (x < x_array[0]) {
        x = x_array[0];
    }
    if (x > x_array[rows - 1]) {
        x = x_array[rows - 1];
    }
    for (i = 0; i < rows; i++) {
        if (x == x_array[i]) {
            *row1 = i;
            *row2 = i;
            return;
        }
        if (x < x_array[i]) {
            if (0 == i)
                return;
            else
                *row1 = i - 1;
            *row2 = i;
            return;
        }
    }
}

/*******************************************************
  Function:        interpolate_linear_x
  Description:     找到数组中最接近x的值
  Input:           数组int *x_array(从小到大排)
                    数组大小int rows
                    插值int x
  Output:           插值后数组中最接近x的index
  Return:          NA
********************************************************/
static int interpolate_linear_x(int *x_array, int *y_array, int rows, int x)
{
    int row1 = 0;
    int row2 = 0;
    int result = 0;

    if (!x_array || !y_array || 0 >= x)
        return 0;
    if (rows < 1)
        return 0;

    interpolate_find_pos_reverse(x_array, rows, x, &row1, &row2);
    result = polar_linear_interpolate(y_array[row1], x_array[row1],
        y_array[row2], x_array[row2], x);
    return result;
}

/*******************************************************
  Function:       interpolate_two_dimension
  Description:    get y according to : y = ax +b in three dimension
  Input:          struct polar_x_y_tbl *polar_lut ---- lookup table
                  x                    ---- row entry
                  y                    ---- col entry
  Output:         NULL
  Return:         value of two dimension interpolated
********************************************************/
static int interpolate_two_dimension(polar_res_tbl* lut,
                int x, int y, int z)
{
    int tfactor1 = 0, tfactor2 = 0, socfactor1 = 0, socfactor2 = 0;
    int row1 = 0, row2 = 0, col1 = 0, col2 = 0, scalefactor = 0;
    int rows, cols, zcols, z_index;
    int z_res[POLAR_CURR_ARRAY_NUM] = {0};

    if (!lut || (lut->rows < 1) || (lut->cols < 1))
        return 0;

    rows = lut->rows;
    cols = lut->cols;
    zcols = lut->z_lens;
    interpolate_find_pos(lut->x_array, rows, x, &row1, &row2);
    interpolate_find_pos(lut->y_array, cols, y, &col1, &col2);

    if (lut->x_array[row1] != lut->x_array[row2]) {
        tfactor1 = POLAR_INT_COE * (lut->x_array[row1] - x)
            /(lut->x_array[row1] - lut->x_array[row2]);
        tfactor2 = POLAR_INT_COE * (x - lut->x_array[row2])
            /(lut->x_array[row1] - lut->x_array[row2]);
        if (lut->y_array[col1] != lut->y_array[col2]) {
            socfactor1 = POLAR_INT_COE * (lut->y_array[col1] - y)
                /(lut->y_array[col1] - lut->y_array[col2]);
            socfactor2 = POLAR_INT_COE * (y - lut->y_array[col2])
                /(lut->y_array[col1] - lut->y_array[col2]);
        }
    }else if (lut->y_array[col1] != lut->y_array[col2]) {
            socfactor1 = POLAR_INT_COE * (lut->y_array[col1] - y)
                /(lut->y_array[col1] - lut->y_array[col2]);
            socfactor2 = POLAR_INT_COE * (y - lut->y_array[col2])
                /(lut->y_array[col1] - lut->y_array[col2]);
    }

    for (z_index = 0; z_index < zcols; z_index++) {
            if (row1 != row2 && col1 != col2) {
                scalefactor = lut->value[row2][col2][z_index]
                        * tfactor1 * socfactor1
                    + lut->value[row2][col1][z_index]
                        * tfactor1 * socfactor2
                    + lut->value[row1][col2][z_index]
                        * tfactor2 * socfactor1
                    + lut->value[row1][col1][z_index]
                        * tfactor2 * socfactor2;
                z_res[z_index] = scalefactor / (POLAR_INT_COE * POLAR_INT_COE);
                continue;
            }else if (row1 == row2 && col1 != col2) {
                scalefactor = lut->value[row1][col2][z_index]
                        *socfactor1 + lut->value[row1][col1][z_index]
                        * socfactor2;
                z_res[z_index] = scalefactor / POLAR_INT_COE;
                continue;
            }else if (row1 != row2 && col1 == col2) {
                scalefactor = lut->value[row2][col2][z_index] * tfactor1
                    + lut->value[row1][col2][z_index] * tfactor2;
                z_res[z_index] = scalefactor / POLAR_INT_COE;
                continue;
            }else {
                scalefactor = lut->value[row2][col2][z_index];
                z_res[z_index] = scalefactor;
                continue;
            }
        }

    scalefactor = interpolate_linear_x(lut->z_array, z_res, zcols, z);
    return scalefactor;
}

/*******************************************************
  Function:        interpolate_nearest_x
  Description:     找到数组中最接近x的值
  Input:           数组int *x_array(从大到小排)
                    数组大小int rows
                    插值int x
  Output:           插值后数组中最接近x的index
  Return:          NA
********************************************************/
static int interpolate_nearest_x(int *x_array, int rows, int x)
{
    int row1 = 0;
    int row2 = 0;
    int index = 0;

    if (!x_array)
        return 0;

    if (rows < 1)
        return 0;
    interpolate_find_pos(x_array, rows, x, &row1, &row2);
    if (x > (x_array[row1] + x_array[row2])/2)
        index = row1;
    else
        index = row2;
    return index;
}

/*******************************************************
  Function:        interpolate_curr_vector
  Description:     找到数组中最接近x的值
  Input:           数组int *curr_array
                    数组大小int rows
                    插值int x
  Output:           插值后对应矢量表中的电流向量index
  Return:          NA
********************************************************/
static int interpolate_curr_vector(int *x_array, int rows, int x)
{
    int row1 = 0;
    int row2 = 0;
    int index = 0;

    if (!x_array)
        return 0;

    if (rows < 1)
        return 0;

    interpolate_find_pos_reverse(x_array, rows, x, &row1, &row2);
    if (x > TWO_AVG(x_array[row1], x_array[row2]))
        index = row2;
    else
        index = row1;

    return index;
}

/*******************************************************
  Function:       interpolate_polar_ocv
  Description:    look for ocv according to temp, lookup table and pc
  Input:
                  struct pc_temp_ocv_lut *lut      ---- lookup table
                  int batt_temp_degc               ---- battery temperature
                  int pc                           ---- percent of uah
  Output:         NULL
  Return:         percent of uah
********************************************************/
static int interpolate_polar_ocv(polar_ocv_tbl *lut,
                int batt_temp_degc, int pc)
{
    int i, ocvrow1, ocvrow2, ocv;
    int rows, cols;
    int row1 = 0;
    int row2 = 0;
    if( NULL == lut ) {
        polar_err("NULL point in [%s]\n", __func__);
        return -1;
    }

    if (0 >= lut->rows || 0 >= lut->cols) {
        polar_err("lut mismatch [%s]\n", __func__);
        return -1;
    }
    rows = lut->rows;
    cols = lut->cols;
    interpolate_find_pos(lut->percent, rows, pc, &row1, &row2);

    if (batt_temp_degc > lut->temp[0])
        batt_temp_degc = lut->temp[0];
    if (batt_temp_degc < lut->temp[cols - 1])
        batt_temp_degc = lut->temp[cols - 1];

    for (i = 0; i < cols; i++)
        if (batt_temp_degc >= lut->temp[i])
            break;
    if ((batt_temp_degc == lut->temp[i]) || (0 == i)) {
        ocv = polar_linear_interpolate(
        lut->ocv[row1][i],
        lut->percent[row1],
        lut->ocv[row2][i],
        lut->percent[row2],
        pc);
        return ocv;
    }

    ocvrow1 = polar_linear_interpolate(
        lut->ocv[row1][i - 1],
        lut->temp[i - 1],
        lut->ocv[row1][i],
        lut->temp[i],
        batt_temp_degc);

    ocvrow2 = polar_linear_interpolate(
        lut->ocv[row2][i - 1],
        lut->temp[i - 1],
        lut->ocv[row2][i],
        lut->temp[i],
        batt_temp_degc);

    ocv = polar_linear_interpolate(
        ocvrow1,
        lut->percent[row1],
        ocvrow2,
        lut->percent[row2],
        pc);

    return ocv;
}

/*******************************************************
  Function:        get_polar_vector
  Description:     获取极化矢量数据
  Input:           极化矢量表polar_x_y_z_tbl* polar_vector_lut
                   电量中心值int soc
                   当前电池温度temp
                   极化电流int curr
  Output:          极化矢量值
  Return:          NA
********************************************************/
static int get_polar_vector_value(polar_x_y_z_tbl* lut,
    int batt_temp_degc, int soc, int curr, int t_index)
{
    int x_soc, y_curr, z_temp;
    if (NULL == lut || t_index > POLAR_TIME_ARRAY_NUM
        ||t_index < 0 || curr <= 0)
        return 0;
    z_temp =  interpolate_nearest_x(lut->z_array, lut->z_lens, batt_temp_degc);
    x_soc = interpolate_nearest_x(lut->x_array, lut->rows, soc);
    y_curr = interpolate_curr_vector(polar_curr_vector_interval,
        POLAR_CURR_ARRAY_VECTOR_NUM, curr);
    return lut->value[z_temp][x_soc][y_curr][t_index];
}

/*******************************************************
  Function:        get_polar_vector_res
  Description:     获取极化矢量数据
  Input:           极化矢量表polar_x_y_z_tbl* polar_vector_lut
                   电量中心值int soc
                   当前电池温度temp
                   极化电流int curr
  Output:          极化矢量内阻
  Return:          NA
********************************************************/
static int get_polar_vector_res(polar_res_tbl* lut,
    int batt_temp_degc, int soc, int curr)
{
    int soc_index, curr_index, batt_temp_index, res;
    if (NULL == lut || soc <= 0 || curr <= 0)
        return 0;
    batt_temp_index = interpolate_nearest_x(lut->x_array, lut->rows,
                        batt_temp_degc);
    soc_index = interpolate_nearest_x(lut->y_array, lut->cols, soc);
    curr_index = interpolate_curr_vector(polar_curr_vector_interval,
        POLAR_CURR_ARRAY_VECTOR_NUM, curr);
    res = interpolate_two_dimension(lut, lut->x_array[batt_temp_index],
            lut->y_array[soc_index], polar_curr_vector_interval[curr_index]);
    return res;
}

/*******************************************************
  Function:        interpolate_linear_a
  Description:     从当前温度正负3度内分别找到两个有效的a值进行插值
  Input:           polar_learn_tbl a值自学习表
                  batt_temp_index 当前温度对应查找表的索引
                  soc_index当前电量对应查找表的索引
  Output:           插值后当前温度对应的a值
  Return:          -1:no trained data found; others:trainded a value
********************************************************/
static int interpolate_linear_a(polar_learn_tbl* lut, int batt_temp_index, int soc_index)
{
    int i, temp_index, pos_index, neg_index, result;
    long pos_trained_a = 0, neg_trained_a = 0;

    temp_index = batt_temp_index;
    pos_index = batt_temp_index;
    neg_index = batt_temp_index;
    for (i = 0; i < POLAR_LEARN_TEMP_RANGE; i++) {
        temp_index++;
    polar_debug("%s:vol:%ld, temp_index:%d\n", __func__, lut->value[soc_index][temp_index].polar_vol_mv, temp_index);
        if (POLAR_LEARN_TEMP_COLS <= temp_index || 0 > temp_index)
            break;
        if (0 != lut->value[soc_index][temp_index].polar_vol_mv) {
            pos_trained_a = lut->value[soc_index][temp_index].a_trained;
            pos_index = temp_index;
            break;
        }
    }
    temp_index = batt_temp_index;
    for (i = 0; i < POLAR_LEARN_TEMP_RANGE; i++) {
        temp_index--;
        if (POLAR_LEARN_TEMP_COLS <= temp_index || 0 > temp_index)
            break;
        /*因为25度后温度不连续，超过25度不再继续查找有效值*/
        if (lut->y_array[temp_index] >= TEMP_25_DEGREE)
            break;
    polar_debug("%s:vol:%ld, temp_index:%d\n", __func__, lut->value[soc_index][temp_index].polar_vol_mv, temp_index);
        if (0 != lut->value[soc_index][temp_index].polar_vol_mv) {
            neg_trained_a = lut->value[soc_index][temp_index].a_trained;
            neg_index = temp_index;
            break;
        }
    }
    polar_debug("%s:neg_trained_a:%ld, pos_trained_a:%ld\n", __func__, neg_trained_a, pos_trained_a);
    if (neg_trained_a && pos_trained_a){
        result = polar_linear_interpolate(neg_trained_a, lut->y_array[neg_index],
            pos_trained_a,lut->y_array[pos_index], lut->y_array[batt_temp_index]);
        return result;
    } else if (neg_trained_a) {
        return neg_trained_a;
    } else if (pos_trained_a) {
        return pos_trained_a;
    }
    return -1;
}

/*******************************************************
  Function:        record_polar_vol
  Description:    记录2次极化电压值(5s更新)
  Input:          long polar_vol_uv:极化电压值
  Output:         2次极化电压值列表
  Return:          NA
********************************************************/
static void record_polar_vol(long polar_vol_uv)
{
    struct hisi_polar_device *di = g_polar_di;
    int i;
    if (NULL == di)
        return;
    mutex_lock(&di->polar_vol_lock);
    for (i = 0; i < POLAR_ARRAY_NUM; i++) {
        polar_debug("%s:vol:%ld,vol_now:%ld\n", __func__, di->polar_vol_array[i], polar_vol_uv);
    }
    di->polar_vol_array[di->polar_vol_index] = polar_vol_uv;
    di->polar_vol_index++;
    di->polar_vol_index = di->polar_vol_index % POLAR_ARRAY_NUM;
    mutex_unlock(&di->polar_vol_lock);
}


/****************************************************************************//**
 * @brief      : polar_flash_open
 * @param[in]  : flags
 * @return     : ::int
 * @note       :
********************************************************************************/
static int polar_flash_open(int flags)
{
	char p_name[POLAR_BUF_SHOW_LEN + 1] = {0};
	int ret, fd_dfx;

	ret = flash_find_ptn(PART_BATT_TP_PARA, p_name);
	if (0 != ret) {
		polar_err("%s()-line=%d\n", __func__, __LINE__);
	}
	p_name[POLAR_BUF_SHOW_LEN] = '\0';
	fd_dfx = sys_open(p_name, flags, POLAR_FILE_LIMIT);
	if (fd_dfx < 0) {
		polar_err("%s()-line=%d\n", __func__, __LINE__);
	}
	return fd_dfx;
}

/****************************************************************************//**
 * @brief      : polar_get_flash_data
 * @param[in]  : buf
 * @param[in]  : buf_size
 * @param[in]  : flash_offset
 * @return     : ::u32
 * @note       :
********************************************************************************/
u32 polar_get_flash_data(void *buf, u32 buf_size, u32 flash_offset)
{
	int ret, fd_flash;
	u32 cnt=0;
	mm_segment_t old_fs;

	if (!buf || 0 == buf_size) {
		polar_err("%s()-line=%d\n", __func__, __LINE__);
		return 0;
	}

	old_fs = get_fs(); /*lint !e501*/
	set_fs(KERNEL_DS); /*lint !e501*/

	fd_flash = polar_flash_open(O_RDONLY);
	if (fd_flash >= 0) {
		ret = sys_lseek(fd_flash, flash_offset, SEEK_SET);
		if (ret < 0) {
			polar_err("%s()-line=%d, ret=%d, flash_offset=%x\n", __func__, __LINE__, ret, flash_offset);
			goto close;
		}
		cnt = (u32)sys_read(fd_flash, buf, buf_size);
		if (cnt != buf_size) {
			polar_err("%s()-line=%d, cnt=%d\n", __func__, __LINE__, cnt);
			goto close;
		}
	} else {
		polar_err("%s()-line=%d, fd_flash=%d\n", __func__, __LINE__, fd_flash);
		set_fs(old_fs);
		return 0;
	}
close:
	sys_close(fd_flash);
	set_fs(old_fs); /*lint !e501*/
	return cnt;
}

/****************************************************************************//**
 * @brief      : polar_add_flash_data
 * @param[in]  : p_buf
 * @param[in]  : buf_size
 * @param[in]  : flash_offset
 * @return     : void
 * @note       :
********************************************************************************/
void polar_add_flash_data(void *p_buf, u32 buf_size, u32 flash_offset)
{
	int ret, fd_flash, cnt=0;
	mm_segment_t old_fs;

	if (NULL == p_buf) {
		polar_err("%s()-line=%d\n", __func__, __LINE__);
		return;
	}

	old_fs = get_fs(); /*lint !e501*/
	set_fs(KERNEL_DS); /*lint !e501*/

	fd_flash = polar_flash_open(O_WRONLY);
	if (fd_flash >= 0) {
		ret = sys_lseek(fd_flash, flash_offset, SEEK_SET);
		if (ret < 0) {
			polar_err("%s()-line=%d, ret=%d\n", __func__, __LINE__, ret);
			goto close;
		}
		cnt = sys_write(fd_flash, p_buf, buf_size);
		if (cnt != buf_size) {
			polar_err("%s()-line=%d, cnt=%d\n", __func__, __LINE__, cnt);
			goto close;
		}
	} else {
		polar_err("%s()-line=%d\n", __func__, __LINE__);
		set_fs(old_fs); /*lint !e501*/
		return;
	}
close:
	sys_close(fd_flash);
	set_fs(old_fs); /*lint !e501*/
}

/****************************************************************************//**
 * @brief      : polar_clear_flash_data
 * @param[in]  : NA
 * @return     : void
 * @note       :
********************************************************************************/
void polar_clear_flash_data(void)
{
	void *p_buf = NULL;

	p_buf = kzalloc(POLAR_LUT_SIZE, GFP_KERNEL);
	if (NULL == p_buf) {
		pr_err("%s()-line=%d\n", __func__, __LINE__);
		return;
	}
	polar_add_flash_data(p_buf, POLAR_LUT_SIZE, POLAR_LUT_OFFSET);
	kfree(p_buf);
	p_buf = NULL;
}
EXPORT_SYMBOL(polar_clear_flash_data);
/****************************************************************************//**
 * @brief      : polar_partition_ready
 * @param[in]  : NA
 * @return     : int -1:not ready,0:ready
 * @note       :
********************************************************************************/
static int polar_partition_ready(void)
{
	char p_name[POLAR_BUF_SHOW_LEN + 1] = {0};
	int ret;

	ret = flash_find_ptn(PART_BATT_TP_PARA, p_name);
	if (0 != ret) {
		polar_err("%s()-line=%d\n", __func__, __LINE__);
	}
	p_name[POLAR_BUF_SHOW_LEN] = '\0';
	if(0 != sys_access(p_name,0)) {
        polar_err("%s()-line=%d, TP partition name:%s\n", __func__, __LINE__, p_name);
	    return -1;
    }
    return 0;
}

/****************************************************************************//**
 * @brief      : polar_partition_data_check
 * @param[in]  : NA
 * @return     : int -1:check fail,0:ok
 * @note       :
********************************************************************************/
static int polar_partition_data_check(polar_learn_tbl* lut)
{
    int i,j;
    if (NULL == lut)
        return -1;
    for (i = 0; i < lut->rows; i++){
        for (j = 0; j < lut->cols; j++){
            if (lut->value[i][j].polar_vol_mv) {
                if (lut->value[i][j].a_trained < POLAR_ERR_A_MIN
                    || lut->value[i][j].a_trained > POLAR_ERR_A_MAX) {
                    polar_err("fail data lut[%d][%d].vol:%d,.a:%d",
                        i, j, lut->value[i][j].polar_vol_mv, lut->value[i][j].a_trained);
                    return -1;
                }
            }
            polar_debug("self_learn_lut[%d][%d].vol:%d,.a:%d",
                        i, j, lut->value[i][j].polar_vol_mv, lut->value[i][j].a_trained);
        }
    }
    return 0;
}
#ifdef CONFIG_HISI_DEBUG_FS
/****************************************************************************//**
 * @brief      : hisee_pdswipe_record_show
 * @param[in]  : dev
 * @param[in]  : attr
 * @param[in]  : buf
 * @return     : ::ssize_t
 * @note       :
********************************************************************************/
ssize_t polar_self_learn_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	void *p_buf = NULL;
	u32	cnt=0;

	p_buf = kzalloc(POLAR_LUT_SIZE, GFP_KERNEL);
	if (NULL == p_buf) {
		polar_err("%s()-line=%d\n", __func__, __LINE__);
		return 0;
	}
	/*todo:copy data from emmc, to save in user buffer*/
	cnt = polar_get_flash_data(p_buf, POLAR_LUT_SIZE, POLAR_LUT_OFFSET);
	if (cnt > 0) {
		if (memcpy_s((void *)buf, PAGE_SIZE, (const void *)p_buf, min_t(size_t, POLAR_LUT_SIZE, PAGE_SIZE))) {
			polar_err("%s()-line=%d\n", __func__, __LINE__);
		}
	} else {
		polar_err("%s()-line=%d\n", __func__, __LINE__);
	}

	kfree(p_buf);
	p_buf = NULL;

	return (ssize_t)cnt;
}/*lint !e715*/
#endif

/*******************************************************
  Function:        store_trained_a
  Description:    记录学习到的a值
  Input:          polar_learn_tbl* lut:自学习表
                  int batt_temp_degc:电池温度
                  int soc:当前电量
                  long trained_a:自学习系数a
                  long polar_vol_uv:本次自学习a值对应的极化电压
  Output:         NA
  Return:          NA
********************************************************/
static void store_trained_a(polar_learn_tbl* lut, int batt_temp_degc, int soc,
                                long trained_a, long polar_vol_uv)
{
    int soc_index, batt_temp_index;
    static unsigned long last_self_learn_time = 0;
    unsigned long time_now =  (unsigned long)(hisi_getcurtime()/NSEC_PER_SEC);

    if (NULL == lut)
        return;
    soc_index = interpolate_nearest_x(lut->x_array, lut->rows, soc);
    batt_temp_index = interpolate_nearest_x(lut->y_array, lut->cols,
                    batt_temp_degc);
    lut->value[soc_index][batt_temp_index].polar_vol_mv = polar_vol_uv / UVOLT_PER_MVOLT;
    lut->value[soc_index][batt_temp_index].a_trained = trained_a;
    if (time_after(time_now, last_self_learn_time + SELF_LEARN_GAP)) {
        polar_debug("%s:last_learn_time:%d,time_now:%d\n", __func__, last_self_learn_time, time_now);
        last_self_learn_time = time_now;
        polar_add_flash_data(lut->value, sizeof(lut->value), POLAR_LUT_OFFSET);
    }
    polar_info("%s:polar trained a:%ld, polar vol:%ld\n", __func__, trained_a, polar_vol_uv);
}

/*******************************************************
  Function:        get_trained_polar_vol
  Description:    查询本次自学习a值对应的极化电压
  Input:          polar_learn_tbl* lut:自学习表
                  int batt_temp_degc:电池温度
                  int soc:当前电量
  Output:         NA
  Return:         long polar_vol_uv:本次自学习a值对应的极化电压
********************************************************/
static short get_trained_polar_vol(polar_learn_tbl* lut, int batt_temp_degc, int soc)
{
    int soc_index, batt_temp_index;
    if (NULL == lut)
        return 0;
    soc_index = interpolate_nearest_x(lut->x_array, lut->rows, soc);
    batt_temp_index = interpolate_nearest_x(lut->y_array, lut->cols,
                    batt_temp_degc);
    return lut->value[soc_index][batt_temp_index].polar_vol_mv;
}

/*******************************************************
  Function:        get_trained_a
  Description:    查询本次自学习a值
  Input:          polar_learn_tbl* lut:自学习表
                  int batt_temp_degc:电池温度
                  int soc:当前电量
  Output:         NA
  Return:         long polar_vol_uv:本次自学习a值
********************************************************/
static long get_trained_a(polar_learn_tbl* lut, int batt_temp_degc, int soc)
{
    int soc_index, batt_temp_index;
    long trained_a = -1;

    if (NULL == lut)
        return -1;
    soc_index = interpolate_nearest_x(lut->x_array, lut->rows, soc);
    batt_temp_index = interpolate_nearest_x(lut->y_array, lut->cols,
                    batt_temp_degc);
    polar_debug("%s:temp:%d,temp_idx:%d,soc:%d,soc_idx:%d\n", __func__,
        batt_temp_degc, batt_temp_index, soc, soc_index);
    /*超出温度范围内，返回-1*/
    if (batt_temp_index >= POLAR_LEARN_TEMP_COLS || batt_temp_index  < 0)
        return -1;
    /*超出电量范围内，返回-1*/
    if (soc_index >= POLAR_RES_PC_CURR_ROWS || soc_index < 0)
        return -1;
    /*当前电量温度对应的自学习表中极化电压不为0，则认为a值有效*/
    if (0 != lut->value[soc_index][batt_temp_index].polar_vol_mv) {
        trained_a = lut->value[soc_index][batt_temp_index].a_trained;
    } else if (TEMP_25_DEGREE > batt_temp_degc) {
    /*如果当前温度小于25，且该温度、电量节点没有自学习值，可通过插值方式获取a值*/
        trained_a = interpolate_linear_a(lut, batt_temp_index, soc_index);
    }
    return trained_a;
}

/*******************************************************
  Function:        could_vbat_learn_a
  Description:    查询电池条件是否可以自学习
  Input:          struct hisi_polar_device *di:设备信息
                  int ocv_soc_mv:当前电量对应ocv
                  int vol_now_mv:当前电压
                  int cur:当前电流
                  long polar_vol_uv:当前极化电压
                  int temp:电池温度
                  int soc:当前电量
  Output:         NA
  Return:         TRUE:可以自学习/FALSE:不可以进行自学习
********************************************************/
static bool could_vbat_learn_a (struct hisi_polar_device *di, int ocv_soc_mv,
                                int vol_now_mv, int cur, long polar_vol_uv,
                                int temp, int soc)
{
    int vol_coe = 0;
    long polar_vol_trained = 0;
    long polar_vol_mv = polar_vol_uv / UVOLT_PER_MVOLT;

    if (NULL == di)
        return FALSE;
    polar_debug("%s:v_cutoff:%d\n", __func__, di->v_cutoff);
    /*VBAT(tn)+I(tn)*RPCB> Vcutoff-100mV@放电电流为负*/
    if (VBAT_LEARN_GAP_MV <= (int)di->v_cutoff - vol_now_mv)
        return FALSE;
    /*即[OCV(tn)-VBAT(tn)+I(tn)*RPCB]/ [OCV(tn)-Vcutoff+I(tn)*RPCB]>0.8@放电电流为负*/
    vol_coe = (TENTH * (ocv_soc_mv - vol_now_mv)) /(ocv_soc_mv - (int)di->v_cutoff + 
            (cur * ((int)di->r_pcb / UOHM_PER_MOHM))/UVOLT_PER_MVOLT);
    polar_debug("%s:vol_coe:%d, last_avgcurr_5s:%d\n", __func__, vol_coe, di->last_avgcurr_5s);
    if (VBAT_LEARN_COE_HIGH < vol_coe)
        return TRUE;
    /*平均电流I<-1.5A@放电电流为负*/
    if (di->last_avgcurr_5s < VBAT_LEARN_AVGCURR_HIGH)
        return TRUE;
    polar_vol_trained = get_trained_polar_vol(&polar_learn_lut, temp, soc);
    polar_debug("%s:trained_vol:%ld\n", __func__, polar_vol_trained);
    /*即0.6<[OCV(tn)-VBAT(tn)+I(tn)*RPCB]/ [OCV(tn)-Vcutoff+I(tn)*RPCB]≤0.8
    且Vpert(tn)< Vpert_ training(SOCn ,Tempm) @放电电流为负*/
    if (VBAT_LEARN_COE_LOW < vol_coe && polar_vol_mv < polar_vol_trained)
        return TRUE;
    /*即-1.5A<最近5s放电平均电流I≤-0.5A
    且Vpert(tn)< Vpert_ training(SOCn ,Tempm) @放电电流为正*/
    if (di->last_avgcurr_5s < VBAT_LEARN_AVGCURR_LOW
            && polar_vol_mv < polar_vol_trained)
        return TRUE;
    /*不满足上述条件，则不进行自学习*/
    return FALSE;
}


/*******************************************************
  Function:        could_learn_a
  Description:    查询电池条件是否可以自学习
  Input:          struct hisi_polar_device *di:设备信息
                  int ocv_soc_mv:当前电量对应ocv
                  int vol_now_mv:当前电压
                  int cur:当前电流
                  long polar_vol_uv:当前极化电压
                  int temp:电池温度
                  int soc:当前电量
  Output:         NA
  Return:         TRUE:可以自学习/FALSE:不可以进行自学习
********************************************************/
static bool could_learn_a (struct hisi_polar_device *di, int ocv_soc_mv,
                                int vol_now_mv, int cur, long polar_vol_uv,
                                int temp, int soc)
{
    long last_polar_vol = 0;

    if (NULL == di)
        return FALSE;
    /*Vpert(tn-1)、Vpert(tn)<0@放电电流为负，且Vpert(tn)<Vpert(tn-1)*/
    if (0 <= polar_vol_uv)
        return FALSE;
    mutex_lock(&di->polar_vol_lock);
    if (di->polar_vol_index >= 0 && di->polar_vol_index < POLAR_ARRAY_NUM) {
        last_polar_vol = ( di->polar_vol_index - 1 < 0) ?
            di->polar_vol_array[POLAR_ARRAY_NUM - 1] :
                di->polar_vol_array[di->polar_vol_index - 1];
    }
    polar_debug("%s:last_polar_vol:%ld,polar_vol_now:%ld\n", __func__, last_polar_vol, polar_vol_uv);
    if (last_polar_vol <= polar_vol_uv ||
        0 <= last_polar_vol) {
        mutex_unlock(&di->polar_vol_lock);
        return FALSE;
    }
    mutex_unlock(&di->polar_vol_lock);
    /*判断电池相关其他条件*/
    polar_debug("%s:ocv:%d,vol_now:%d,cur:%d,temp:%d,soc:%d\n", __func__,
                ocv_soc_mv, vol_now_mv, cur, temp, soc);
    if (FALSE == could_vbat_learn_a(di, ocv_soc_mv, vol_now_mv, cur,
                        polar_vol_uv, temp, soc))
        return FALSE;
    return TRUE;
}
static bool could_update_b (struct hisi_polar_device *di, long polar_vol_uv)
{
    int i;
    if (NULL == di)
        return FALSE;
    /*judge if can update polar_b with current polar voltage */
    if (VPERT_NOW_LOW_B > polar_vol_uv || VPERT_NOW_HIGH_B < polar_vol_uv)
        return FALSE;
    mutex_lock(&di->polar_vol_lock);
    /*judge if can update polar_b with former two polar voltages */
    for (i = 0; i < POLAR_ARRAY_NUM; i++) {
        if (POLAR_VOL_INVALID == di->polar_vol_array[i])
            break;
        if (VPERT_PAST_LOW_B > di->polar_vol_array[i]
            ||VPERT_PAST_HIGH_B < di->polar_vol_array[i])
            break;
    }
    if (POLAR_ARRAY_NUM == i) {
        mutex_unlock(&di->polar_vol_lock);
        return FALSE;
    }
    mutex_unlock(&di->polar_vol_lock);
    return TRUE;
}
/*******************************************************
  Function:        update_polar_error_b
  Description:     更新极化电压b值
  Input:           电量查表OCV ocv_soc_mv
                   当前电压 vol_now_mv
                   当前极化电压 polar_vol_uv
  Output:          更新后的极化电压b值
  Return:          NA
********************************************************/
static void update_polar_error_b(int ocv_soc_mv, int vol_now_mv,
                                    long polar_vol_uv)
{
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di)
        return;
    if (FALSE == could_update_b(di, polar_vol_uv))
        return;
    /*calculate polar b*/
    polar_err_b1 = (long)(ocv_soc_mv - vol_now_mv) * UVOLT_PER_MVOLT;
    polar_err_b2 = polar_vol_uv;
    polar_debug("update polar b1,b2:%ld:%ld\n", polar_err_b1, polar_err_b2);
    return;
}

/*******************************************************
  Function:        calc_weighted_a
  Description:     加权平均计算a值
  Input:          a_trained，需要加权的a值
                  是否需要加入a值滤波队列 enqueue
  Output:          平均后的a值
  Return:          NA
********************************************************/
static long calc_weighted_a(long a_trained, int enqueue)
{
    int i;
    long temp_err_a_wavg = 0, a_weighted = 0;
    static int polar_valid_a_index = 0;
    /*calculate weighted average of A with current A and former three A values*/
    for (i = 0; i < POLAR_VALID_A_NUM; i++) {
        polar_debug("%s:a_array[%d]:%ld,coe:%d\n", __func__,
            i, polar_err_a_array[i], polar_err_a_coe[i]);
        if (0 == polar_err_a_array[i])
            temp_err_a_wavg += (a_trained * polar_err_a_coe[i]);
        else
            temp_err_a_wavg += (polar_err_a_array[i] * polar_err_a_coe[i]);
    }
    a_weighted = (a_trained * polar_err_a_coe[POLAR_VALID_A_NUM]
        +temp_err_a_wavg) / POLAR_A_COE_MUL;
    if (enqueue) {
        polar_err_a_array [polar_valid_a_index % POLAR_VALID_A_NUM] =
            a_trained;
        polar_valid_a_index++;
        polar_valid_a_index = polar_valid_a_index % POLAR_VALID_A_NUM;
    }
    polar_debug("%s:update real polar a:%ld, weighted average a:%ld\n",
        __func__, a_trained, a_weighted);
    return a_weighted;
}

/*******************************************************
  Function:        polar_learn_lut_init
  Description:     自学习表初始化
  Input:         自学习表lut
  Output:         NA
  Return:          NA
********************************************************/
static void polar_learn_lut_init(polar_learn_tbl* lut)
{
    int i,j;
    if (NULL == lut)
        return;
    for (i = 0; i < lut->rows; i++){
        for (j = 0; j < lut->cols; j++){
            lut->value[i][j].a_trained = POLAR_ERR_A_DEFAULT;
            lut->value[i][j].polar_vol_mv = 0;
        }
    }
}
/*******************************************************
  Function:        update_polar_error_a
  Description:     更新极化电压a值
  Input:           电量查表OCV ocv_soc_mv
                   当前电压 vol_now_mv
                   当前极化电压 polar_vol_uv
  Output:          更新后的极化电压a值
  Return:          NA
********************************************************/
static void update_polar_error_a(int ocv_soc_mv, int vol_now_mv, int cur,
                                    long polar_vol_uv, int temp, int soc)
{
    long temp_err_a = 0, a_trained = 0;
    static int polar_partition_read_flag = 0;
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di)
        return;
    if (0 != polar_partition_ready()) {
        polar_err("%s:partition not ready\n", __func__);
        polar_err_a = -1;
        return;
    }
    /*read polar partition once partition is ready*/
    if (0 == polar_partition_read_flag) {
        polar_get_flash_data(&polar_learn_lut.value, sizeof(polar_learn_lut.value), POLAR_LUT_OFFSET);
        if (0 != polar_partition_data_check(&polar_learn_lut)) {
            polar_err("%s:partition check fail\n", __func__);
            polar_clear_flash_data();
            polar_learn_lut_init(&polar_learn_lut);
        }
        polar_partition_read_flag = 1;
    }
    if ((VPERT_NOW_LOW_A <= polar_vol_uv)
        &&(VPERT_NOW_HIGH_A >= polar_vol_uv)) {
        polar_debug("polar_vol:%ld too small,use default a\n", polar_vol_uv);
        temp_err_a = POLAR_ERR_A_DEFAULT;
        goto get_a;
    }
    temp_err_a = ((long)(ocv_soc_mv  - vol_now_mv) * UVOLT_PER_MVOLT
                - polar_err_b1) * POLAR_ERR_COE_MUL / (-polar_vol_uv);
    /*if polar_a was negative,we use last max average current instead*/
    if (temp_err_a <= 0) {
        polar_debug("temp_err_a:%ld too small,use default a\n", temp_err_a);
        temp_err_a = POLAR_ERR_A_DEFAULT;
        goto get_a;
    }
    /*clamp a in range[0.9~3]*/
    temp_err_a = clamp_val(temp_err_a, POLAR_ERR_A_MIN, POLAR_ERR_A_MAX);
    /*check whether update in self learning*/
    if (TRUE == could_learn_a(di, ocv_soc_mv, vol_now_mv, cur,
                              polar_vol_uv, temp, soc))
        store_trained_a(&polar_learn_lut, temp, soc, temp_err_a, polar_vol_uv);
get_a:
    a_trained = get_trained_a(&polar_learn_lut, temp, soc);
    polar_debug("%s:polar a before weighted average:%ld, trained_a:%ld\n",
                __func__, temp_err_a, a_trained);
    if (a_trained <= 0) {
        polar_debug("no self learn polar a ,use current value\n");
        polar_err_a = calc_weighted_a(temp_err_a, 0);
    } else
        polar_err_a = calc_weighted_a(a_trained, 1);
    return;
}
/*******************************************************
  Function:        get_estimate_max_avg_curr
  Description:     计算最大负载电流
  Input:           全局设备信息指针di
                   soc当前电量
                   polar_vol当前极化电压
                   polar_past过去5s-20min内的极化电压
                   电池温度batt_temp_degc
  Output:
  Return:         最大负载电流(mA)
********************************************************/
static int get_estimate_max_avg_curr(int ocv_soc_mv, int vol_now,
                int soc, long polar_vol, long polar_past,
                int batt_temp_degc, int v_cutoff, int r_pcb)
{
    int curr_index = 0, t_index = 0;
    long polar_vol_future = 0;
    int polar_res_future = 0;
    int curr_future = 0;
    int curr_tmp = 0;
    int curr_thresh = 0;
    int res, res_zero, res_vector, polar_vector;
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di)
        return 0;

    if (polar_err_a < 0) {
        return 0;
    }
    /*calculate polar_vol_future in uV*/
    polar_vol_future = (long)(ocv_soc_mv - v_cutoff) * UVOLT_PER_MVOLT
            -(polar_err_b1 + polar_err_a * polar_err_b2 / POLAR_ERR_COE_MUL)
            +(polar_past * polar_err_a/POLAR_ERR_COE_MUL);
        polar_debug("%s:polar_vol_future:%ld,ocv_soc_mv:%d,\
v_cutoff:%d,polar_err_b1:%ld,polar_err_b2:%ld,polar_err_a:%ld,\
polar_past:%ld\n", __func__, polar_vol_future, ocv_soc_mv, v_cutoff,
            polar_err_b1, polar_err_b2, polar_err_a, polar_past);
    for (curr_index = POLAR_CURR_ARRAY_VECTOR_NUM -1; curr_index >= 0;
        curr_index--) {
        curr_future = polar_curr_vector_interval[curr_index];
        polar_vector = 0;
        res = interpolate_two_dimension(&polar_res_lut,
            batt_temp_degc, soc, curr_future);
        res_vector = get_polar_vector_res(&polar_res_lut,
            batt_temp_degc, soc, curr_future);
        for (t_index = 0; t_index < POLAR_VECTOR_5S; t_index++){
            polar_vector += get_polar_vector_value(&polar_vector_lut,
                batt_temp_degc, soc, curr_future, t_index + 1);
        }
        res_zero = get_polar_vector_value(&polar_vector_lut,
            batt_temp_degc, soc, curr_future, 0);
        if (0 != res_vector)
            polar_res_future = ((res_zero + polar_vector) * res) / res_vector;
        polar_debug("%s:res_zero:%d, polar_vector:%d, polar_res_future:%d,\
res_vector:%d, res:%d\n", __func__, res_zero, polar_vector,
            polar_res_future, res_vector, res);
        /*calculate polar_res_future in mΩ*/
        polar_res_future = (polar_res_future * polar_err_a)
            /(POLAR_ERR_COE_MUL * POLAR_RES_MHOM_MUL) + r_pcb / UOHM_PER_MOHM;
        if (0 != polar_res_future)
            curr_tmp = (int)(polar_vol_future / polar_res_future);
        polar_debug("%s:polar_vol_future:%ld,polar_res_future:%d,\n",
            __func__, polar_vol_future, polar_res_future);
        if (curr_index > 0)
            curr_thresh = TWO_AVG(polar_curr_vector_interval[curr_index],
                                polar_curr_vector_interval[curr_index - 1]);
        else
            curr_thresh = polar_curr_vector_interval[0];
        if (curr_thresh <= curr_tmp) {
                curr_future = curr_tmp;
                polar_vector = 0;
                res = interpolate_two_dimension(&polar_res_lut,
                    batt_temp_degc, soc, curr_future);
                res_vector = get_polar_vector_res(&polar_res_lut,
                    batt_temp_degc, soc, curr_future);
                for (t_index = 0; t_index < POLAR_VECTOR_5S; t_index++){
                    polar_vector += get_polar_vector_value(&polar_vector_lut,
                        batt_temp_degc, soc, curr_future, t_index + 1);
                }
                res_zero = get_polar_vector_value(&polar_vector_lut,
                    batt_temp_degc, soc, curr_future, 0);
                if (0 != res_vector)
                    polar_res_future = ((res_zero + polar_vector) * res)
                        /res_vector;
                polar_debug("%s:predict:curr_future:%d,res_zero:%d,\
polar_res_future:%d\n", __func__, curr_future, res_zero, polar_res_future);
                /*calculate polar_res_future in mΩ*/
                polar_res_future = (polar_res_future * polar_err_a)
                    /(POLAR_ERR_COE_MUL * POLAR_RES_MHOM_MUL)
                        + r_pcb / UOHM_PER_MOHM;
                if (0 != polar_res_future)
                    curr_tmp = (int)(polar_vol_future / polar_res_future);
                di->polar_res_future = polar_res_future;
                di->last_max_avg_curr = curr_tmp;
                polar_debug("%s:predict:curr_future:%d,polar_res_future:%d\n",
                    __func__, curr_tmp,polar_res_future);
                return curr_tmp;
            }
    }
    di->polar_res_future = polar_res_future;
    di->last_max_avg_curr = curr_tmp;
    return curr_tmp;
}

/*******************************************************
  Function:        get_estimate_peak_curr
  Description:     计算最大峰值电流
  Input:           全局设备信息指针di
                   soc当前电量
                   电池温度batt_temp_degc
  Output:
  Return:         最大峰值电流(mA)
********************************************************/
static int get_estimate_peak_curr(int ocv_soc_mv, long polar_vol, int soc,
                        int batt_temp_degc, int v_cutoff, int r_pcb)
{
    int curr_index, res_zero, curr_future, curr_temp;
    int curr_thresh, res, res_vector;
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di)
        return 0;

    if (polar_err_a < 0) {
        return 0;
    }

    for (curr_index = POLAR_CURR_ARRAY_VECTOR_NUM -1; curr_index >= 0;
            curr_index--) {
        curr_future = polar_curr_vector_interval[curr_index];
        res = interpolate_two_dimension(&polar_res_lut,
            batt_temp_degc, soc, curr_future);
        res_vector = get_polar_vector_res(&polar_res_lut,
            batt_temp_degc, soc, curr_future);
        res_zero = get_polar_vector_value(&polar_vector_lut,
            batt_temp_degc, soc, curr_future, 0);
        if (0 != res_vector)
            res_zero = (res_zero * res) / res_vector;
        /*voltage in uv,resistence in mΩ,curr in mA*/
        curr_temp = ((long)(ocv_soc_mv - v_cutoff) * UVOLT_PER_MVOLT
              -(polar_err_b1 + polar_err_a * polar_err_b2 / POLAR_ERR_COE_MUL))
                    /(r_pcb / UOHM_PER_MOHM + polar_err_a * res_zero
                        / (POLAR_RES_MHOM_MUL * POLAR_ERR_COE_MUL));
        polar_debug("%s:curr_future:%d, res_zero:%d, curr_temp:%d,\
ocv_soc_mv:%d, v_cutoff:%d\n", __func__, curr_future, res_zero,
                curr_temp, ocv_soc_mv, v_cutoff);
        if (curr_index > 0)
            curr_thresh = TWO_AVG(polar_curr_vector_interval[curr_index],
                                polar_curr_vector_interval[curr_index - 1]);
        else
            curr_thresh = polar_curr_vector_interval[0];
        if (curr_thresh <= curr_temp) {
            curr_future = curr_temp;
            res = interpolate_two_dimension(&polar_res_lut,
                batt_temp_degc, soc, curr_future);
            res_vector = get_polar_vector_res(&polar_res_lut,
                batt_temp_degc, soc, curr_future);
            res_zero = get_polar_vector_value(&polar_vector_lut,
                batt_temp_degc, soc, curr_future, 0);
            if (0 != res_vector)
                res_zero = (res_zero * res) / res_vector;
            /*voltage in uv,resistence in mΩ,curr in mA*/
            curr_temp = ((long)(ocv_soc_mv - v_cutoff) * UVOLT_PER_MVOLT
               -(polar_err_b1 + polar_err_a * polar_err_b2 / POLAR_ERR_COE_MUL))
                        /(r_pcb / UOHM_PER_MOHM + polar_err_a * res_zero
                            /(POLAR_RES_MHOM_MUL * POLAR_ERR_COE_MUL));
            polar_debug("%s:predict:curr_future:%d, res_zero:%d, curr_temp:%d,\
ocv_soc_mv:%d, v_cutoff:%d\n", __func__, curr_future, res_zero,
                curr_temp, ocv_soc_mv, v_cutoff);
            di->last_max_peak_curr = curr_temp;
            return curr_temp;
        }
    }
    di->last_max_peak_curr = curr_temp;
    return curr_temp;
}

/*******************************************************
  Function:        calculate_polar_volatge
  Description:     计算极化电压
  Input:           全局设备信息指针di
                   极化矢量表polar_x_y_z_tbl* polar_vector_lut
                   极化阻抗表polar_res_tbl* polar_res_lut
                   当前电量int soc
                   当前电池温度temp
  Output:
  Return:          极化电压(uV)
********************************************************/
static long calculate_polar_volatge(int soc, int temp)
{
    int t_index;
    int curr_index;
    int soc_avg, curr_avg, ratio;
    int res, res_vector, polar_vector;
    long vol_sum = 0;
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di)
        return 0;
    for (t_index = 0; t_index < POLAR_TIME_ARRAY_NUM; t_index++) {
        for (curr_index = 0; curr_index < (POLAR_CURR_ARRAY_NUM + 1);
                curr_index++) {
            soc_avg = polar_avg_curr_info[t_index][0].soc_avg;
            curr_avg = polar_avg_curr_info[t_index][curr_index].current_avg;
            ratio = polar_avg_curr_info[t_index][curr_index].duration;
            if (0 == ratio || 0 == soc_avg || 0 == curr_avg)
                continue;
            res = interpolate_two_dimension(&polar_res_lut,
                    temp, soc_avg, abs(curr_avg));
            res_vector = get_polar_vector_res(&polar_res_lut,
                    temp, soc_avg, abs(curr_avg));
            polar_vector = get_polar_vector_value(&polar_vector_lut,
                    temp, soc_avg, abs(curr_avg), t_index + 1);
            /*curr_avg is in ma, ratio is in percentage, res is in 0.1mΩ*/
            if (0 != res_vector)
                vol_sum += (long)curr_avg * ((((long)ratio
                    *(res / POLAR_RES_MHOM_MUL))/POLAR_RATIO_PERCENTAGE)
                        *polar_vector) / res_vector;
            polar_debug("vol_sum:%ld\n", vol_sum);
        }
    }
    polar_debug("vol_sum:%ld\n", vol_sum);
    return vol_sum;
}

/*******************************************************
  Function:        calculate_polar_vol_r0
  Description:     计算极化电压
  Input:           全局设备信息指针di
                   极化矢量表polar_x_y_z_tbl* polar_vector_lut
                   极化阻抗表polar_res_tbl* polar_res_lut
                   当前电量int soc
                   当前电池温度temp
                   当前电流curr
  Output:
  Return:          R0对应的极化电压(uV)
********************************************************/
static long calculate_polar_vol_r0(int soc, int temp, int curr)
{
    int res, res_zero, res_vector, polar_vector;
    long polar_vol = 0;

    res = interpolate_two_dimension(&polar_res_lut, temp, soc, abs(curr));
    res_vector = get_polar_vector_res(&polar_res_lut, temp, soc, abs(curr));
    polar_vector = get_polar_vector_value(&polar_vector_lut, temp, soc,
                    abs(curr), 0);
    if (0 != res_vector)
        res_zero = (polar_vector * res) / res_vector;
    else
        res_zero = polar_vector;
    polar_vol =  (long)curr * res_zero / POLAR_RES_MHOM_MUL;
    polar_debug("res_zero:%ld,vol0:%ld\n", res_zero, polar_vol);
    return polar_vol;
}

/*******************************************************
  Function:        add_polar_info
  Description:     极化数据求和
  Input:          struct ploarized_info *ppolar, int t_index
  Output:         t_index某个时间区间内的极化数据求和
  Return:          NA
********************************************************/
static void add_polar_info(int current_ma, int duration, int t_index)
{
    int curr_index;

    if (0 > t_index || POLAR_TIME_ARRAY_NUM <= t_index || 0 >= duration)
        return;
    /*找到对应的电流区间*/
    for (curr_index = 0; curr_index < POLAR_CURR_ARRAY_NUM; curr_index++){
        if (abs(current_ma) <= polar_curr_interval[curr_index])
            break;
    }
    /*反极化是否需要考虑，方案中没有标明，电流用绝对值代替*/
    /*电流大于2c的情况也要计算到POLAR_CURR_ARRAY_NUM*/
    polar_avg_curr_info[t_index][curr_index].current_avg +=
                                    ((long)current_ma * duration);
    polar_avg_curr_info[t_index][curr_index].cnt++;
    polar_avg_curr_info[t_index][curr_index].duration += duration;
}

static void update_polar_avg_curr(struct hisi_polar_device *di, int predict_msec)
{
    int time_interval = 0;
    int t_index = 0;
    int curr_index = 0;
    long past_avg_cc = 0;
    if ( 0 == predict_msec) {
        for (t_index = 0; t_index < POLAR_TIME_ARRAY_NUM; t_index++) {
            for (curr_index = 0; curr_index < (POLAR_CURR_ARRAY_NUM + 1);
                        curr_index++){
                past_avg_cc += polar_avg_curr_info[t_index][curr_index].current_avg;
                }
            if (POLAR_TIME_ARRAY_0S == t_index) {
                di->last_avgcurr_0s = (int)(past_avg_cc / POLAR_TIME_0S);
            }
            if (POLAR_TIME_ARRAY_5S == t_index)
                di->last_avgcurr_5s = (int)(past_avg_cc / POLAR_TIME_5S);
            if (POLAR_TIME_ARRAY_25S == t_index) {
                di->last_avgcurr_25s = (int)(past_avg_cc / POLAR_TIME_25S);
                break;
            }
        }
        polar_debug("last_avgcurr_0s:%d,5s:%d,25s:%d\n",di->last_avgcurr_0s,
                    di->last_avgcurr_5s, di->last_avgcurr_25s);
    }

    /*求Tn区间内的各电流区间平均电流*/
    for (t_index = 0; t_index < POLAR_TIME_ARRAY_NUM; t_index++) {
        for (curr_index = 0; curr_index < (POLAR_CURR_ARRAY_NUM + 1);
                    curr_index++){
            /*平均电流计算cc/t*/
            if (0 != polar_avg_curr_info[t_index][curr_index].duration) {
                polar_avg_curr_info[t_index][curr_index].current_avg /=
                polar_avg_curr_info[t_index][curr_index].duration;
            /*电流对应时间比例计算*/
                if (0 != t_index)
                    time_interval = (int)(polar_sample_interval[t_index]
                                    - polar_sample_interval[t_index - 1]);
                else
                    time_interval = (int)polar_sample_interval[t_index];
                polar_avg_curr_info[t_index][curr_index].duration =
                    (polar_avg_curr_info[t_index][curr_index].duration
                        * POLAR_RATIO_PERCENTAGE) / time_interval;
                if (polar_avg_curr_info[t_index][curr_index].duration >
                        POLAR_RATIO_PERCENTAGE)
                    polar_avg_curr_info[t_index][curr_index].duration =
                        POLAR_RATIO_PERCENTAGE;
            }
            polar_debug("polar_avg_curr_info[%d][%d]|duration_ratio|soc_avg:\
%lld ma,%ld%%,%d\n", t_index, curr_index,
                polar_avg_curr_info[t_index][curr_index].current_avg,
                polar_avg_curr_info[t_index][curr_index].duration,
                polar_avg_curr_info[t_index][0].soc_avg);
        }
    }
}

/*******************************************************
  Function:        polar_info_calc
  Description:     极化数据计算
  Input:          struct smartstar_coul_device *di
  Output:         分Tn区间极化电流和ratio、电量中心值
  Return:          NA
********************************************************/
static void polar_info_calc(struct hisi_polar_device *di, int predict_msec)
{
    int t_index = 0, last_soc_avg;
    struct ploarized_info *ppolar, *ppolar_head;
    struct list_head *pos;
    unsigned long node_sample_time = 0, node_duration_time = 0;
    unsigned long temp_duration_time = 0, head_sample_time = 0;

    if (NULL == di || NULL == di->polar_buffer) {
        polar_err("[polar] %s di is null.\n", __func__);
        return;
    }
    if (list_empty(&(di->polar_head.list)))
        return;
    memset_s(polar_avg_curr_info, sizeof(polar_avg_curr_info),
            0, sizeof(polar_avg_curr_info));
    ppolar_head = list_first_entry(&(di->polar_head.list),
                        struct ploarized_info, list);
    last_soc_avg = ppolar_head->soc_now;
    head_sample_time = ppolar_head->sample_time + predict_msec;
    /*遍历循环链表，求Tn时间内各电流区间的平均电流*/
    list_for_each(pos, &(di->polar_head.list)){
        ppolar = list_entry(pos, struct ploarized_info, list);
        node_sample_time = ppolar->sample_time;
        node_duration_time = ppolar->duration;
        /*--big data divide start--*/
        while (POLAR_TIME_ARRAY_NUM > t_index) {
             /*sample end time of each node is in Tn*/
            if (time_after_eq(node_sample_time + polar_sample_interval[t_index],
                            head_sample_time)) {
                /*sample start time of each node is in Tn*/
                if (time_after_eq(node_sample_time - node_duration_time
                        + polar_sample_interval[t_index], head_sample_time)) {
                    add_polar_info(ppolar->current_ma, (int)node_duration_time,
                            t_index);
                    break;
                } else{
                /*sample start time of each node is not in Tn, divide node*/
                    temp_duration_time = node_sample_time
                        -(head_sample_time - polar_sample_interval[t_index]);
                    add_polar_info(ppolar->current_ma, (int)temp_duration_time,
                            t_index);
                    node_duration_time = node_duration_time
                        -temp_duration_time;
                    node_sample_time = head_sample_time
                        -polar_sample_interval[t_index];
                    /*求Tn区间内的电量中心值*/
                    polar_avg_curr_info[t_index][0].soc_avg =
                        TWO_AVG(ppolar->soc_now, last_soc_avg);
                    last_soc_avg = ppolar->soc_now;
                    /*节点的时间相对头结点超过Tn，进入下一个Tn区间*/
                    t_index++;
                }
        } else {
        /*sample end time of each node is in Tn*/
            /*求Tn区间内的电量中心值*/
                polar_avg_curr_info[t_index][0].soc_avg =
                        TWO_AVG(ppolar->soc_now, last_soc_avg);
                last_soc_avg = ppolar->soc_now;
                /*节点的时间相对头结点超过Tn，进入下一个Tn区间*/
                t_index++;
            }
        }
    }
    update_polar_avg_curr(di, predict_msec);
}

/*******************************************************
  Function:        fill_up_polar_fifo
  Description:     极化数据放入fifo中
  Input:           极化数据struct ploarized_info *ppolar
                   head:极化数据索引的链表头
                   rbuffer:极化数据存放的循环buffer
                   total_sample_time:该buffer存放数据的总采样时间
  Output:       NA
  Return:       NA
********************************************************/
static void fill_up_polar_fifo(struct hisi_polar_device *di,
                                struct ploarized_info* ppolar,
                                struct list_head* head,
                                struct hisiap_ringbuffer_s *rbuffer,
                                unsigned long total_sample_time)
{
    struct ploarized_info *ppolar_head;
    struct ploarized_info *ppolar_tail;
    struct ploarized_info* ppolar_buff;
    u32 buff_pos = 0;

    if (NULL == ppolar || NULL == rbuffer || NULL == head || NULL == di)
        return;

    if (!list_empty(head)){
        ppolar_tail = list_last_entry(head, struct ploarized_info, list);
        ppolar_head = list_first_entry(head, struct ploarized_info, list);
        /*judge if the node is before the head sample time*/
        if (time_before(ppolar->sample_time,
                ppolar_head->sample_time + di->fifo_interval)) {
            return;
        }
        /*judge if we need to del node after total_sample_time*/
        if (time_after(ppolar->sample_time,
            (ppolar_tail->sample_time + total_sample_time))) {
            list_del(&ppolar_tail->list);
        }
    }
    hisiap_ringbuffer_write(rbuffer, (u8*)ppolar);
    /*this is to get the buff_pos after write*/
    if (0 == rbuffer->rear) {
        if (rbuffer->is_full)
            buff_pos = rbuffer->max_num;
        else {
            polar_err("[%s]:ringbuffer write failed\n",__func__);
            return;
        }
    }else{
        buff_pos = rbuffer->rear - 1;
    }
    ppolar_buff = (struct ploarized_info*)
        &rbuffer->data[(unsigned long)buff_pos * rbuffer->field_count];
    list_add(&(ppolar_buff->list), head);
}

static unsigned long polar_get_head_time(struct hisi_polar_device *di,
                                              struct list_head* head)
{
    struct ploarized_info *ppolar_head;
    unsigned long sample_time;

    if (NULL == di || NULL == head)
        return 0;
    if (list_empty(head)){
        return 0;
    }
    ppolar_head = list_first_entry(head, struct ploarized_info, list);
    sample_time = ppolar_head->sample_time;
    return sample_time;
}

static int polar_get_head_curr(struct hisi_polar_device *di,
                                              struct list_head* head)
{
    struct ploarized_info *ppolar_head;
    int sample_curr;

    if (NULL == di || NULL == head)
        return 0;
    if (list_empty(head)){
        return 0;
    }
    ppolar_head = list_first_entry(head, struct ploarized_info, list);
    sample_curr = ppolar_head->current_ma;
    return sample_curr;
}
/*******************************************************
  Function:        sample_timer_func
  Description:     coul fifo sample time callback
  Input:           struct hrtimer *timer
  Output:          NA
  Return:          NA
********************************************************/
static enum hrtimer_restart sample_timer_func(struct hrtimer *timer)
{
    struct hisi_polar_device *di = g_polar_di;
    struct ploarized_info node;
    unsigned long sample_time, flags, fifo_time_ms;
    int fifo_depth, current_ua, i;
    ktime_t kt;

    if (NULL == di)
        return HRTIMER_NORESTART;
    spin_lock_irqsave(&di->coul_fifo_lock, flags);
    /*get coul fifo according to the fifo depth*/
    fifo_depth = hisi_coul_battery_fifo_depth();
    sample_time = hisi_getcurtime();
    sample_time = sample_time / NSEC_PER_MSEC;
    for (i = fifo_depth - 1; i >= 0; i--)
    {
        node.sample_time = sample_time - ((unsigned long)i*di->fifo_interval);//lint !e571
        current_ua = hisi_coul_battery_fifo_curr(i);
        node.current_ma = - (current_ua / UA_PER_MA);
        node.duration = di->fifo_interval;
        node.temperature = hisi_battery_temperature();
        node.soc_now = hisi_coul_battery_ufcapacity_tenth();
        node.list.next = NULL;
        node.list.prev = NULL;
        polar_debug("%s:time:%lu,curr:%d,duration:%lu,temp:%d:soc:%d\n", __func__,
        node.sample_time, node.current_ma, node.duration,
        node.temperature, node.soc_now);
        /*here we put the fifo info to 30S ringbuffer*/
        fill_up_polar_fifo(di, &node, &di->coul_fifo_head.list, di->fifo_buffer,
                           COUL_FIFO_SAMPLE_TIME);
    }
    fifo_time_ms = (unsigned long)fifo_depth * di->fifo_interval;//lint !e571
    kt = ktime_set(fifo_time_ms / MSEC_PER_SEC,
                    (fifo_time_ms % MSEC_PER_SEC) * NSEC_PER_MSEC);
    hrtimer_forward_now(timer, kt);
    spin_unlock_irqrestore(&di->coul_fifo_lock, flags);
    return HRTIMER_RESTART;
}

/*******************************************************
  Function:        get_polar_vol
  Description:     获取极化电压
  Input:          soc_now:0.1% percent
                  batt_temp:℃
                  curr_now:ma
  Output:
  Return:          极化电压(uV)
********************************************************/
static long get_polar_vol(struct hisi_polar_device *di, int soc_now,
                            int batt_temp, int curr_now)
{
    long vol = 0;

    if (NULL == di)
        return 0;
    vol = calculate_polar_volatge(soc_now, batt_temp);
    vol += calculate_polar_vol_r0(soc_now, batt_temp, curr_now);
    return vol;
}

/*******************************************************
  Function:        copy_fifo_buffer
  Description:     将fifo中的35s数据拷贝到20min的buffer
  Input:           fifo_head:fifo的链表头
                   polar_head:极化buffer的链表头
                   fifo_rbuffer:fifo的循环buffer
                   polar_rbuffer:极化信息的循环buffer
  Output:          NA
  Return:          NA
********************************************************/
void copy_fifo_buffer(struct list_head* fifo_head,
                           struct list_head* polar_head,
                           struct hisiap_ringbuffer_s *fifo_rbuffer,
                           struct hisiap_ringbuffer_s *polar_rbuffer)
{
    struct hisi_polar_device *di = g_polar_di;
    struct ploarized_info* ppolar_temp;
    struct ploarized_info* n;

    if (NULL == fifo_head || NULL == polar_head || NULL == di ||
        NULL == fifo_rbuffer || NULL == polar_rbuffer) {
        return;
    }
    if (list_empty(fifo_head))
        return;
    list_for_each_entry_safe_reverse(ppolar_temp, n, fifo_head, list) {
            fill_up_polar_fifo(di, ppolar_temp, polar_head, polar_rbuffer,
                               COUL_POLAR_SAMPLE_TIME);
    }
}
/*******************************************************
  Function:        sync_sample_info
  Description:     极化数据同步
  Input:           NA
  Output:          NA
  Return:          NA
********************************************************/
void sync_sample_info(void)
{
    struct hisi_polar_device *di = g_polar_di;
    unsigned long sample_time, flags, fifo_time_ms, sync_time, delta_time;
    int fifo_depth, current_ua, i, head_curr, last_fifo_curr;
    int sync_num = 0;
    struct ploarized_info node;
    ktime_t kt;

    if (NULL == di)
        return;
    polar_debug("[polar] %s ++ \n", __func__);
    spin_lock_irqsave(&di->coul_fifo_lock, flags);
    /*read polar info from the fifo*/
    fifo_depth = hisi_coul_battery_fifo_depth();
    sync_time = hisi_getcurtime();
    sync_time = sync_time / NSEC_PER_MSEC;
    sample_time = polar_get_head_time(di, &di->coul_fifo_head.list);
    head_curr = polar_get_head_curr(di, &di->coul_fifo_head.list);
    delta_time = (sync_time - sample_time) % di->fifo_interval;
    if (time_after_eq(sync_time, sample_time + di->fifo_interval)) {
        sync_num = (int)(sync_time - sample_time) / (int)di->fifo_interval;
        /*check whether add one more fifo when we have left time after divide*/
        if (delta_time && sync_num < fifo_depth) {
            last_fifo_curr = hisi_coul_battery_fifo_curr(sync_num);
            last_fifo_curr = -last_fifo_curr / UA_PER_MA;
            if (last_fifo_curr != head_curr)
                sync_num++;
        }
        sync_num = clamp_val(sync_num, 0, fifo_depth);
        for (i = sync_num - 1; i >= 0; i--)
        {
            /*avoid data lost when left time is not divisible by fifo interval*/
            node.sample_time = sample_time + ((unsigned long)(sync_num - i)*di->fifo_interval);//lint !e571
           // node.sample_time = sync_time - ((unsigned long)i*di->fifo_interval);//lint !e571
            current_ua = hisi_coul_battery_fifo_curr(i);
            node.current_ma = - (current_ua / UA_PER_MA);
            node.duration = di->fifo_interval;
            node.temperature =  hisi_battery_temperature();
            node.soc_now = hisi_coul_battery_ufcapacity_tenth();
            node.list.next = NULL;
            node.list.prev = NULL;
            polar_debug("%s:time:%lu,curr:%d,duration:%lu,temp:%d:soc:%d\n",
                __func__, node.sample_time, node.current_ma, node.duration,
                node.temperature, node.soc_now);
            /*here we put the fifo info to 35S ringbuffer*/
            fill_up_polar_fifo(di, &node, &di->coul_fifo_head.list,
                                di->fifo_buffer, COUL_FIFO_SAMPLE_TIME);
        }
        /*modify the sample timer when we have synchronized the polar info*/
        fifo_time_ms = (unsigned long)fifo_depth * di->fifo_interval;//lint !e571
        kt = ktime_set(fifo_time_ms / MSEC_PER_SEC,
                        (fifo_time_ms % MSEC_PER_SEC) * NSEC_PER_MSEC);
        if (!hrtimer_active(&di->coul_sample_timer))
            hrtimer_forward_now(&di->coul_sample_timer, kt);
        else
            polar_debug("%s:not forward\n", __func__);
    }
    /*copy 35s fifo buffer to 20min polar buffer*/
    copy_fifo_buffer(&di->coul_fifo_head.list, &di->polar_head.list,
                     di->fifo_buffer, di->polar_buffer);
    spin_unlock_irqrestore(&di->coul_fifo_lock, flags);
    polar_debug("[polar] %s sync_num:%d--\n", __func__, sync_num);
}
EXPORT_SYMBOL(sync_sample_info);
/*******************************************************
  Function:        is_polar_list_ready
  Description:     判断极化数据链表是否满20min
         注意:只在resume时调用，不做spinlock互斥
  Input:           NA
  Output:          NA
  Return:          TRUE/FALSE
********************************************************/
bool is_polar_list_ready(void)
{
    struct hisi_polar_device *di = g_polar_di;
    struct list_head* head = NULL;
    struct ploarized_info *ppolar_head;

    if (NULL == di)
        return FALSE;
    head = &di->polar_head.list;
    if (!list_empty(head)){
        ppolar_head = list_first_entry(head, struct ploarized_info, list);
        if (time_after(ppolar_head->sample_time, COUL_POLAR_SAMPLE_TIME)) {
            return TRUE;
        }
    }
    return FALSE;
}
EXPORT_SYMBOL(is_polar_list_ready);
/*******************************************************
  Function:        get_resume_polar_info
  Description:     获取唤醒极化数据，并复制到fifo中
              注意:只在resume中调用，没有做互斥处理
  Input:           int eco_ibat:eco下t0时刻电流采样值
                   int curr:t0时刻之前的平均电流
                   int duration:t0时刻之前的平均电流持续时间
                   int sample_time:t0时刻的采样时间(rtc时间)
                   int temp:当前温度
                   int soc:当前电量
  Output:          NA
  Return:          NA
********************************************************/
void get_resume_polar_info(int eco_ibat, int curr, int duration, int sample_time, int temp, int soc)
{
    struct hisi_polar_device *di = g_polar_di;
    struct ploarized_info node, node_eco;
    if (NULL == di || 0 >= duration)
        return;
    /*sample for Tn-1*/
    if (0 != eco_ibat) 
        node.sample_time = sample_time - di->fifo_interval;
    else
        node.sample_time = sample_time;
    node.current_ma = curr;
    node.duration = duration;
    node.temperature =  temp;
    node.soc_now = soc;
    node.list.next = NULL;
    node.list.prev = NULL;
    polar_debug("%s:time:%lu,curr:%d,duration:%lu,temp:%d:soc:%d\n",
        __func__, node.sample_time, node.current_ma, node.duration, 
        node.temperature, node.soc_now);
    fill_up_polar_fifo(di, &node, &di->coul_fifo_head.list,
                       di->fifo_buffer, COUL_FIFO_SAMPLE_TIME);
    /*sample for Tn*/
    if (0 != eco_ibat) {
        node_eco.sample_time = sample_time;
        node_eco.current_ma = eco_ibat / UA_PER_MA;
        node_eco.duration = di->fifo_interval;
        node_eco.temperature =  temp;
        node_eco.soc_now = soc;
        node_eco.list.next = NULL;
        node_eco.list.prev = NULL;
        polar_debug("%s:time:%lu,curr:%d,duration:%lu,temp:%d:soc:%d\n",
            __func__, node_eco.sample_time, node_eco.current_ma, node_eco.duration, 
            node_eco.temperature, node_eco.soc_now);
        fill_up_polar_fifo(di, &node_eco, &di->coul_fifo_head.list,
                           di->fifo_buffer, COUL_FIFO_SAMPLE_TIME);
    }
    copy_fifo_buffer(&di->coul_fifo_head.list, &di->polar_head.list,
                     di->fifo_buffer, di->polar_buffer);
}
EXPORT_SYMBOL(get_resume_polar_info);
/*******************************************************
  Function:        start_polar_sample
  Description:     开始采样定时器
  Input:           NA
  Output:          NA
  Return:          NA
********************************************************/
void start_polar_sample(void)
{
    struct hisi_polar_device *di = g_polar_di;
    ktime_t kt;
    unsigned long fifo_time_ms;
    int fifo_depth;

    if (NULL == di)
        return;
    fifo_depth = hisi_coul_battery_fifo_depth();
    fifo_time_ms = (unsigned long)fifo_depth * di->fifo_interval;//lint !e571
    kt = ktime_set(fifo_time_ms / MSEC_PER_SEC,
                    (fifo_time_ms % MSEC_PER_SEC) * NSEC_PER_MSEC);
    hrtimer_start(&di->coul_sample_timer, kt, HRTIMER_MODE_REL);
}
EXPORT_SYMBOL(start_polar_sample);
/*******************************************************
  Function:        start_polar_sample
  Description:     停止采样定时器
  Input:           NA
  Output:          NA
  Return:          NA
********************************************************/
void stop_polar_sample(void)
{
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di)
        return;
    hrtimer_cancel(&di->coul_sample_timer);
}
EXPORT_SYMBOL(stop_polar_sample);
/*******************************************************
  Function:        polar_params_calculate
  Description:     更新极化数据和供电能力
  Input:           struct polar_calc_info* polar:极化数据结构体
                   int ocv_soc_mv:当前电量对应ocv
                   int vol_now:当前电压
                   int cur:当前电流
                   bool update_a:是否更新修正系数
  Output:          更新极化数据和供电能力
  Return:          NA
********************************************************/
int polar_params_calculate(struct polar_calc_info* polar,int ocv_soc_mv,
                                    int vol_now, int cur, bool update_a)
{
    int curr_future_5s = 0;
    int curr_future_peak = 0;
    int batt_soc_real, temp;
    long polar_future_5s = 0;
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di || NULL == polar)
        return -1;
    batt_soc_real =  hisi_coul_battery_ufcapacity_tenth();
    temp = hisi_battery_temperature();
    polar_info_calc(di, 0);
     /*get current polar vol*/
    polar->vol = get_polar_vol(di, batt_soc_real, temp, cur);
    polar->vol = polar->vol - polar_err_b2;
    /*pull 5s ahead to calculate future polar vol*/
    polar_info_calc(di, POLAR_CURR_PREDICT_MSECS);
    polar_future_5s = calculate_polar_volatge(batt_soc_real, temp);
    if (TRUE == update_a)
        update_polar_error_a(ocv_soc_mv, vol_now, cur, polar->vol, temp, batt_soc_real);
    /*calculate future max avg current*/
    curr_future_5s = get_estimate_max_avg_curr(ocv_soc_mv, vol_now,
                            batt_soc_real, polar->vol, polar_future_5s, temp,
                            di->v_cutoff, di->r_pcb);
    /*calculate future max peak current*/
    curr_future_peak = get_estimate_peak_curr(ocv_soc_mv, polar->vol,
                            batt_soc_real, temp, di->v_cutoff, di->r_pcb);
    if (curr_future_peak < curr_future_5s)
        curr_future_peak = curr_future_5s;
    polar->curr_5s = curr_future_5s;
    polar->curr_peak = curr_future_peak;
    //polar->ocv = get_polar_ocv(batt_soc_real, cur, temp, vol_now);
    polar->ocv_old = ocv_soc_mv;
    polar->ori_vol = vol_now;
    polar->ori_cur = cur;
    if (TRUE == update_a) {
        polar->err_a = polar_err_a;
        update_polar_error_b(ocv_soc_mv, vol_now, polar->vol);
        record_polar_vol(polar->vol);
    }
    return 0;
}
EXPORT_SYMBOL(polar_params_calculate);

/*******************************************************
  Function:        polar_ocv_params_calc
  Description:     更新极化OCV数据
  Input:           struct polar_calc_info* polar:极化数据结构体
                   int batt_soc_real:当前电量
                   int temp:当前温度
                   int cur:当前电流
  Output:          更新极化ocv数据
  Return:          NA
********************************************************/
int polar_ocv_params_calc(struct polar_calc_info* polar,
                                int batt_soc_real, int temp, int cur)
{
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di || NULL == polar)
        return -1;

    polar_info_calc(di, 0);
     /*get current polar vol*/
    polar->sr_polar_vol0 = get_polar_vol(di, batt_soc_real, temp, cur);
    polar_info_calc(di, -POLAR_CURR_PREDICT_MSECS);
    polar->sr_polar_vol1 = get_polar_vol(di, batt_soc_real, temp, di->last_avgcurr_5s);
    polar->sr_polar_err_a = get_trained_a(&polar_learn_lut, temp, batt_soc_real);
    polar_info("[%s]sr_polar_vol0:%d,sr_polar_vol1:%d,sr_polar_err_a:%d\n",
        __FUNCTION__, polar->sr_polar_vol0, polar->sr_polar_vol1, polar->sr_polar_err_a);
    return 0;
}
EXPORT_SYMBOL(polar_ocv_params_calc);

/*******************************************************
  Function:        clear_polar_err_b
  Description:     清空修正系数b值，只在ocv更新时起作用，流程需要保证互斥
  Input:          NA
  Output:          NA
  Return:          NA
********************************************************/
void clear_polar_err_b(void)
{
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di)
        return;
    polar_err_b1 = 0;
    polar_err_b2 = 0;
    return;
}
EXPORT_SYMBOL(clear_polar_err_b);
#ifdef CONFIG_HISI_DEBUG_FS
int test_polar_ocv_tbl_lookup(int soc, int batt_temp_degc)
{
    int ocv = 0;
    ocv = interpolate_polar_ocv(&polar_ocv_lut, batt_temp_degc, soc);
    return ocv;
}

int test_res_tbl_lookup(int soc, int batt_temp_degc, int curr)
{
    int res = 0;
    res = interpolate_two_dimension(&polar_res_lut,
            batt_temp_degc, soc, curr);
    return res;
}

int test_vector_res_tbl_lookup(int soc, int batt_temp_degc, int curr)
{
    int res_vector = 0;
    res_vector = get_polar_vector_res(&polar_res_lut,
            batt_temp_degc, soc, curr);
    return res_vector;
}

int test_vector_value_tbl_lookup(int soc, int batt_temp_degc, int curr)
{
    int polar_vector = 0;
    polar_vector = get_polar_vector_value(&polar_vector_lut,
            batt_temp_degc, soc, curr, 0);
    return polar_vector;
}

int test_vector_curr_lookup(int curr)
{
    return interpolate_curr_vector(polar_curr_vector_interval,
            POLAR_CURR_ARRAY_VECTOR_NUM, curr);
}
int test_nearest_lookup(int soc)
{
    return interpolate_nearest_x(polar_resistence_pc_points,
            POLAR_RES_PC_CURR_ROWS, soc);
}

int test_get_trained_a(int temp, int soc)
{
    return get_trained_a(&polar_learn_lut,temp,soc);
}

int test_store_trained_a(int temp, int soc, long a, long polar_vol_uv)
{
    store_trained_a(&polar_learn_lut, temp, soc, a, polar_vol_uv);
    return 0;
}

int get_last_5s_curr(void)
{
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di)
        return -1;
    return  di->last_avgcurr_5s;
}

int get_last_25s_curr(void)
{
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di)
        return -1;
    return  di->last_avgcurr_25s;
}

int test_could_vbat_learn_a (int ocv_soc_mv,int vol_now_mv, int cur,
                                long polar_vol_uv,int temp, int soc)

{
    struct hisi_polar_device *di = g_polar_di;

    if (NULL == di)
        return -1;
    polar_info("ocv:%d,vbat:%d,curr:%d,polar_vol:%ld,temp:%d,soc:%d",
        ocv_soc_mv,vol_now_mv, cur, polar_vol_uv,temp, soc);
    return could_vbat_learn_a(di, ocv_soc_mv,vol_now_mv, cur, polar_vol_uv,temp, soc);
}
#endif
/*******************************************************
  Function:       get_batt_phandle
  Description:    look for batt_phandle with id_voltage
  Input:
                  struct device *dev      ---- device pointer
                  int id_voltage          ---- battery id
                  const char * prop       ---- battery prop
  Output:         struct device_node*
  Return:         phandle of the battery
********************************************************/
static struct device_node *get_batt_phandle(struct device_node *np,
                    const char * prop, int p_num, u32 id_voltage)
{
    int i, ret;
    u32 id_identify_min = 0,id_identify_max = 0;
    const char *batt_brand;
    struct device_node *temp_node = NULL;

    if (NULL == np)
        return NULL;

    if (p_num < 0) {
        polar_info("[%s]get phandle list count failed", __func__);
        return NULL;
    }
    /*iter the phandle list*/
    for (i = 0; i < p_num; i++) {
        temp_node = of_parse_phandle(np, prop, i);
        ret = of_property_read_u32(temp_node, "id_voltage_min",
                &id_identify_min);
        if (ret) {
            polar_err("get id_identify_min failed\n");
            return NULL;
        }
        ret = of_property_read_u32(temp_node, "id_voltage_max",
                &id_identify_max);
        if (ret) {
            polar_err("get id_identify_max failed\n");
            return NULL;
        }
        polar_info("id_identify_min:%d,id_identify_max:%d\n",
                id_identify_min, id_identify_max);
        if ((id_voltage >= id_identify_min) && (id_voltage <= id_identify_max))
            break;
    }
    if (p_num == i) {
        polar_err("no battery modle matched\n");
        return NULL;
    }
    /*print the battery brand*/
    ret = of_property_read_string(temp_node, "batt_brand", &batt_brand);
    if (ret) {
        polar_err("get batt_brand failed\n");
        return NULL;
    }
    polar_info("batt_name:%s matched\n", batt_brand);
    return temp_node;
}

/*******************************************************
  Function:       get_polar_table_info
  Description:    look for batt_phandle with id_voltage
  Input:
  @bat_node:device node from which the property value is to be read
  @propname:name of the property to be searched
  @outvalues:pointer to return value, modified only if return value is 0
  @tbl_size:number of array elements to read
  Output:  outvalues
  Return:  Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough
********************************************************/
static int get_polar_table_info(struct device_node *bat_node,
                const char* propname, u32* outvalues, int tbl_size)
{
    int ele_count;
    int ret;
    /*get polar_ocv_table from dts*/
    ele_count = of_property_count_u32_elems(bat_node, propname);
    polar_info("%s:ele_cnt:%d\n", propname, ele_count);
    /*check if ele_count match with polar_ocv_table*/
    if (ele_count != tbl_size) {
        polar_err("ele_count:%d mismatch with %s\n", ele_count, propname);
        return -EINVAL;
    }
    ret = of_property_read_u32_array(bat_node, propname,
                    outvalues, ele_count);
    if (ret) {
            polar_err("get polar_ocv_table failed\n");
            return ret;
    }
    return 0;
}

/*******************************************************
  Function:       get_polar_dts_info
  Description:    look for dts info
  Input:
  @dev:device pointer
  Output:  NA
  Return:  Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough
********************************************************/
static int get_polar_dts_info(struct hisi_polar_device *di)
{
    int id_voltage = 0;
    int ret = 0;
    int batt_count = 0;
    struct device_node *bat_node;
    struct device_node *coul_node;
    if (NULL == di)
        return -EINVAL;
    coul_node = of_find_compatible_node(NULL, NULL, "hisi,coul_core");
    if (coul_node) {
        ret = of_property_read_u32(coul_node, "normal_cutoff_vol_mv",
                                   &di->v_cutoff);
        ret |= of_property_read_u32(coul_node, "r_pcb",&di->r_pcb);
    }
    if (!coul_node || ret) {
        di->v_cutoff = BATTERY_NORMAL_CUTOFF_VOL;
        di->r_pcb = DEFAULT_RPCB;
        polar_err("get coul info failed\n");
        return -1;
    }

    ret = of_property_read_u32(di->np, "fifo_interval", &di->fifo_interval);
    if (ret) {
        di->fifo_interval = COUL_DEFAULT_SAMPLE_INTERVAL;
        polar_err("get fifo_interval failed\n");
        return ret;
    }
    polar_info("fifo_interval:%d\n", di->fifo_interval);
    ret = of_property_read_u32(di->np, "fifo_depth", 
            &di->fifo_depth);
    if (ret) {
        polar_err("get fifo_depth failed\n");
        return ret;
    }
    polar_info("fifo_depth:%d\n", di->fifo_depth);
    ret = of_property_read_u32(di->np, "polar_batt_cnt", &batt_count);
    if (ret) {
        polar_err("get fifo_depth failed\n");
        return ret;
    }
    polar_info("polar_batt_cnt:%d\n", batt_count);
    id_voltage = hisi_battery_id_voltage();
    bat_node = get_batt_phandle(di->np, "polar_batt_name",
                    batt_count, (u32)id_voltage);
    if (NULL == bat_node) {
        polar_err("get polar_phandle failed\n");
        ret = -EINVAL;
        goto out;
    }
    polar_err("polar_bat_node:%s\n", bat_node->name);
    ret = get_polar_table_info(bat_node,
                    "polar_ocv_table", (u32 *)polar_ocv_lut.ocv,
                    (POLAR_OCV_PC_TEMP_ROWS * POLAR_OCV_PC_TEMP_COLS));
    if (ret)
        goto out;
    ret = get_polar_table_info(bat_node,
                    "polar_res_table", (u32 *)polar_res_lut.value,
                    (POLAR_OCV_PC_TEMP_COLS * POLAR_RES_PC_CURR_ROWS
                        * POLAR_CURR_ARRAY_NUM));
    if (ret)
        goto clr;
    ret = get_polar_table_info(bat_node,
                    "polar_vector_table", (u32 *)polar_vector_lut.value,
                    (POLAR_OCV_PC_TEMP_COLS * POLAR_RES_PC_CURR_ROWS
                        * POLAR_CURR_ARRAY_VECTOR_NUM * POLAR_VECTOR_SIZE));
    if (ret)
        goto clr;
    polar_info("%s:get polar dts info success\n", __func__);
    return 0;
clr:
    memset_s(polar_ocv_lut.ocv, sizeof(polar_ocv_lut.ocv),
            0, sizeof(polar_ocv_lut.ocv));
    memset_s(polar_res_lut.value, sizeof(polar_res_lut.value),
            0, sizeof(polar_res_lut.value));
    memset_s(polar_vector_lut.value, sizeof(polar_vector_lut.value),
            0, sizeof(polar_vector_lut.value));
out:
    return ret;
}
#ifdef CONFIG_HISI_DEBUG_FS
static DEVICE_ATTR(self_learn_value, (S_IRUSR | S_IRGRP), polar_self_learn_show, NULL);
#endif
/*******************************************************
  Function:        polar_info_init
  Description:     极化相关数据初始化(根据电池容量初始化电流档位)
  Input:          struct smartstar_coul_device *di
  Output:           初始化后的极化档位数据
  Return:          NA
********************************************************/
static int polar_info_init(struct hisi_polar_device *di)
{
    int i, batt_fcc;
    int batt_present;
    int ret = 0;

    batt_fcc = hisi_battery_fcc();
    batt_present = is_hisi_battery_exist();
    for (i = 1; i < POLAR_CURR_ARRAY_NUM; i++){
        polar_curr_interval[i] = polar_curr_interval[i]
                        *  batt_fcc / UA_PER_MA;
    }
    for (i = 1; i < POLAR_CURR_ARRAY_VECTOR_NUM; i++){
        polar_curr_vector_interval[i] = polar_curr_vector_interval[i]
                        * batt_fcc / UA_PER_MA;
    }
    polar_learn_lut_init(&polar_learn_lut);
    di->polar_vol_index = 0;
    for (i = 0; i < POLAR_ARRAY_NUM; i++)
        di->polar_vol_array[i] =  POLAR_VOL_INVALID;
    memset_s(polar_ocv_lut.ocv, sizeof(polar_ocv_lut.ocv),
            0, sizeof(polar_ocv_lut.ocv));
    memset_s(polar_res_lut.value, sizeof(polar_res_lut.value),
            0, sizeof(polar_res_lut.value));
    memset_s(polar_vector_lut.value, sizeof(polar_vector_lut.value),
            0, sizeof(polar_vector_lut.value));
    memset_s(polar_avg_curr_info, sizeof(polar_avg_curr_info),
            0, sizeof(polar_avg_curr_info));
#ifdef CONFIG_HISI_DEBUG_FS
    ret = device_create_file(di->dev, &dev_attr_self_learn_value);
    if (ret)
        polar_err("failed to create file");
#endif
    ret |= get_polar_dts_info(di);
    if (ret)
        polar_err("get dts info failed\n");
    return ret;
}

//lint -esym(429, di)
static int hisi_coul_polar_probe(struct platform_device *pdev)
{
    struct device_node *node = pdev->dev.of_node;
    int retval = 0, fifo_depth;
    struct hisi_polar_device *di = NULL;
    ktime_t kt;
    unsigned long fifo_time_ms;

    di = devm_kzalloc(&pdev->dev, sizeof(struct hisi_polar_device),
                        GFP_KERNEL);
    if (!di)
        return -ENOMEM;
    di->dev =&pdev->dev;
    di->np = node;
    di->polar_buffer =
        (struct hisiap_ringbuffer_s*) devm_kzalloc(&pdev->dev,
                                        POLAR_BUFFER_SIZE,
                                        GFP_KERNEL);
    di->fifo_buffer =
        (struct hisiap_ringbuffer_s*) devm_kzalloc(&pdev->dev,
                                        FIFO_BUFFER_SIZE,
                                        GFP_KERNEL);
    if (!di->polar_buffer || !di->fifo_buffer) {
        polar_err("failed to alloc polar_buffer struct\n");
        return -ENOMEM;
    } else
        polar_info("polar_buffer alloc ok:%pK", di->polar_buffer);
    retval = hisiap_ringbuffer_init(di->polar_buffer,
                POLAR_BUFFER_SIZE, sizeof(struct ploarized_info),
                "coul_polar");
    retval |= hisiap_ringbuffer_init(di->fifo_buffer,
                FIFO_BUFFER_SIZE, sizeof(struct ploarized_info),
                "coul_fifo");
    retval |= polar_info_init(di);
    if (retval) {
        polar_err("%s failed to init polar info!!!\n", __FUNCTION__);
        goto out;
    }
    INIT_LIST_HEAD(&di->polar_head.list);
    INIT_LIST_HEAD(&di->coul_fifo_head.list);
    spin_lock_init(&di->coul_fifo_lock);
    mutex_init(&di->polar_vol_lock);
    if ((di->polar_buffer->max_num * di->fifo_interval) <= COUL_POLAR_SAMPLE_TIME){//lint !e647
        polar_err("buffer is not enough for sample:max_node:%u,fifo_time:%d",
            di->polar_buffer->max_num, di->fifo_interval);
        retval = -ENOMEM;
        goto out;
    }
    if ((di->fifo_buffer->max_num * di->fifo_interval) <= COUL_FIFO_SAMPLE_TIME){//lint !e647
        polar_err("buffer is not enough for sample:max_node:%u,fifo_time:%d",
            di->fifo_buffer->max_num, di->fifo_interval);
        retval = -ENOMEM;
        goto out;
    }
    fifo_depth = hisi_coul_battery_fifo_depth();
    fifo_time_ms = (unsigned long)fifo_depth * di->fifo_interval;//lint !e571
    kt = ktime_set(fifo_time_ms / MSEC_PER_SEC,
                    (fifo_time_ms % MSEC_PER_SEC) * NSEC_PER_MSEC);
    hrtimer_init(&di->coul_sample_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    di->coul_sample_timer.function = sample_timer_func;
    hrtimer_start(&di->coul_sample_timer, kt, HRTIMER_MODE_REL);
    platform_set_drvdata(pdev, di);
    g_polar_di = di;
    polar_info("Hisi coul polar ready\n");
    return 0;
out:
    g_polar_di = NULL;
    return retval;//lint !e593
}
//lint +esym(429, di)
static int hisi_coul_polar_remove(struct platform_device *pdev)
{
    struct hisi_polar_device *di = platform_get_drvdata(pdev);

    hrtimer_cancel(&di->coul_sample_timer);
    di = NULL;
    g_polar_di = NULL;
#ifdef CONFIG_HISI_DEBUG_FS
    device_remove_file(&pdev->dev, &dev_attr_self_learn_value);
#endif
    platform_set_drvdata(pdev, NULL);
    return 0;
}

static const struct of_device_id hisi_coul_polar_of_match[] = {
    {
    .compatible = "hisi,coul_polar",
    .data = NULL
    },
    {},
};

MODULE_DEVICE_TABLE(of, hisi_coul_polar_of_match);

static struct platform_driver hisi_coul_polar_driver = {
    .probe = hisi_coul_polar_probe,
    .driver = {
               .name = "coul_polar",
               .owner = THIS_MODULE,
               .of_match_table = of_match_ptr(hisi_coul_polar_of_match),
        },
     .remove = hisi_coul_polar_remove,
};

static int __init hisi_coul_polar_init(void)
{
    platform_driver_register(&hisi_coul_polar_driver);
    return 0;
}

fs_initcall(hisi_coul_polar_init);

static void __exit hisi_coul_polar_exit(void)
{
    platform_driver_unregister(&hisi_coul_polar_driver);
}

module_exit(hisi_coul_polar_exit);

MODULE_DESCRIPTION("COUL POLARIZATION driver");
MODULE_LICENSE("GPL V2");
