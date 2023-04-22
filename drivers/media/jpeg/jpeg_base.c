/*
 * jpeg_base.c
 *
 * implement for jpeg subsystem.
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * luolidong <luolidong@huawei.com>
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
 */
#include <linux/version.h>
#include <media/camera/jpeg/jpeg_base.h>

#include <linux/mutex.h>
#include "cam_log.h"

#define MAX(a,b) ((a)>(b)?(a):(b))

enum {
    JPEG_ENC = 0,
    JPEG_DEC,
    JPEG_IPP,
    JPEG_MAX
};

static unsigned long jpeg_rate[JPEG_MAX] = {0,0};
static unsigned int jpeg_enable_ref = 0;

static DEFINE_MUTEX(jpeg_base_lock);

static int jpeg_set_rate(struct clk *clk, unsigned long rate, int index);

int jpeg_enc_set_rate(struct clk *clk, unsigned long rate)
{
    if (clk == NULL) {
        cam_err("%s: enc clk is null! (%d)",__func__, __LINE__);
        return -EINVAL;
    }

    if (rate == 0) {
        cam_err("%s: enc rate is zero! (%d)",__func__, __LINE__);
        return -EINVAL;
    }

    return jpeg_set_rate(clk, rate, JPEG_ENC);
}

int jpeg_enc_clk_prepare_enable(struct clk *clk)
{
    int ret;

    if (clk == NULL) {
        cam_err("%s: enc clk is null! (%d)",__func__, __LINE__);
        return -EINVAL;
    }

    mutex_lock(&jpeg_base_lock);

    jpeg_enable_ref++;

    ret = clk_prepare_enable(clk);

    mutex_unlock(&jpeg_base_lock);

    return ret;
}

void jpeg_enc_clk_disable_unprepare(struct clk *clk)
{
    if (clk == NULL) {
        cam_err("%s: enc clk is null! (%d)",__func__, __LINE__);
        return;
    }

    mutex_lock(&jpeg_base_lock);

    clk_disable_unprepare(clk);

    if (jpeg_enable_ref > 0) {
        jpeg_enable_ref--;
    }

    if (jpeg_enable_ref == 0) {
        jpeg_rate[JPEG_ENC] = 0;
        jpeg_rate[JPEG_DEC] = 0;
	jpeg_rate[JPEG_IPP] = 0;
    }

    mutex_unlock(&jpeg_base_lock);
}

int jpeg_dec_set_rate(struct clk *clk, unsigned long rate)
{
    if (clk == NULL) {
        cam_err("%s: dec clk is null! (%d)",__func__, __LINE__);
        return -EINVAL;
    }

    if (rate == 0) {
        cam_err("%s: dec rate is zero! (%d)",__func__, __LINE__);
        return -EINVAL;
    }

    return jpeg_set_rate(clk, rate, JPEG_DEC);
}

int jpeg_dec_clk_prepare_enable(struct clk *clk)
{
    int ret;

    if (clk == NULL) {
        cam_err("%s: dec clk is null! (%d)",__func__, __LINE__);
        return -EINVAL;
    }

    mutex_lock(&jpeg_base_lock);

    jpeg_enable_ref++;

    ret = clk_prepare_enable(clk);

    mutex_unlock(&jpeg_base_lock);

    return ret;
}

void jpeg_dec_clk_disable_unprepare(struct clk *clk)
{
    if (clk == NULL) {
        cam_err("%s: dec clk is null! (%d)",__func__, __LINE__);
        return;
    }

    mutex_lock(&jpeg_base_lock);

    clk_disable_unprepare(clk);

    if (jpeg_enable_ref > 0) {
        jpeg_enable_ref--;
    }

    if (jpeg_enable_ref == 0) {
        jpeg_rate[JPEG_ENC] = 0;
        jpeg_rate[JPEG_DEC] = 0;
	jpeg_rate[JPEG_IPP] = 0;
    }

    mutex_unlock(&jpeg_base_lock);
}

int jpeg_ipp_set_rate(struct clk* clk, unsigned long rate)
{
    if (clk == NULL)
    {
        cam_err("%s: ipp clk is null! (%d)", __func__, __LINE__);
        return -EINVAL;
    }

    if (rate == 0)
    {
        cam_err("%s: ipp rate is zero! (%d)", __func__, __LINE__);
        return -EINVAL;
    }

    return jpeg_set_rate(clk, rate, JPEG_IPP);
}

int jpeg_ipp_clk_prepare_enable(struct clk* clk)
{
    int ret;

    if (clk == NULL)
    {
        cam_err("%s: ipp clk is null! (%d)", __func__, __LINE__);
        return -EINVAL;
    }

    mutex_lock(&jpeg_base_lock);
    jpeg_enable_ref++;
    ret = clk_prepare_enable(clk);
    mutex_unlock(&jpeg_base_lock);
	
    return ret;
}

void jpeg_ipp_clk_disable_unprepare(struct clk* clk)
{
    if (clk == NULL)
    {
        cam_err("%s: ipp clk is null! (%d)", __func__, __LINE__);
        return;
    }

    mutex_lock(&jpeg_base_lock);
    clk_disable_unprepare(clk);

    if (jpeg_enable_ref > 0)
    {
        jpeg_enable_ref--;
    }

    if (jpeg_enable_ref == 0)
    {
        jpeg_rate[JPEG_ENC] = 0;
        jpeg_rate[JPEG_DEC] = 0;
        jpeg_rate[JPEG_IPP] = 0;
    }

    mutex_unlock(&jpeg_base_lock);
}

static int jpeg_set_rate(struct clk *clk, unsigned long rate, int index)
{
    int ret;
    unsigned long max_rate;

    mutex_lock(&jpeg_base_lock);

    jpeg_rate[index] = rate;

    max_rate =MAX(MAX(jpeg_rate[JPEG_ENC], jpeg_rate[JPEG_DEC]), jpeg_rate[JPEG_IPP]);

    if (max_rate > rate) {
        cam_info("%s: index(%d) just return", __func__, index);
        mutex_unlock(&jpeg_base_lock);
        return 0;
    }

    ret = clk_set_rate(clk, rate);

    mutex_unlock(&jpeg_base_lock);

    return ret;
}

