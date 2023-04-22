/*
 * fts.c
 *
 * FTS Capacitive touch screen controller (FingerTipS)
 *
 * Copyright (C) 2012, 2013 STMicroelectronics Limited.
 * Authors: AMS(Analog Mems Sensor)
 *        : Victor Phay <victor.phay@st.com>
 *        : Li Wu <li.wu@st.com>
 *        : Giuseppe Di Giore <giuseppe.di-giore@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG
#include <linux/device.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/completion.h>
#include <linux/wakelock.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_HAS_EARLYSUSPEND //hank modified
#include <linux/earlysuspend.h>
#endif
#include "fts.h"
#include <linux/notifier.h>
#include <linux/fb.h>
#ifdef KERNEL_ABOVE_2_6_38
#include <linux/input/mt.h>
#endif
#ifdef CONFIG_JANUARY_BOOSTER
#include <linux/input/janeps_booster.h>
#endif

static struct fts_ts_info *st_info;
static struct class *i2c_cmd_class;
static struct attribute_group i2c_cmd_attr_group;
static unsigned int data[CMD_RESULT_STR_LEN] = {0};
static unsigned char pAddress_i2c[CMD_RESULT_STR_LEN] = {0};
static int byte_count_read = 0 ;
static char Out_buff[TSP_BUF_SIZE];

/*I2C CMd functions: functions to interface with GUI without script */

static ssize_t st_i2c_wr_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct fts_ts_info *info = st_info;
	int i ;
	char buff[16];
	memset(Out_buff, 0x00, ARRAY_SIZE(Out_buff));
	if(byte_count_read == 0){
		snprintf(Out_buff, sizeof(Out_buff), "{FAILED}");
		return snprintf(buf, TSP_BUF_SIZE, "{%s}\n", Out_buff);
	}
#ifdef DEBUG
	printk("%s:DATA READ {", __func__);
	for(i=0;i<byte_count_read;i++){
		printk(" %02X",(unsigned int )info->cmd_wr_result[i]);
		if(i < (byte_count_read-1))
		{
			printk(" ");
		}
	}
	printk("}\n");
#endif
	snprintf(buff, sizeof(buff), "{");
	strncat(Out_buff, buff, ARRAY_SIZE(Out_buff));
	for (i = 0; i < (byte_count_read+2); i++){
		if((i == 0)){
			char temp_byte_count_read = (byte_count_read >> 8) & 0xFF;
			snprintf(buff, sizeof(buff), "%02X",temp_byte_count_read);
		}else if(i == 1){
			char temp_byte_count_read = (byte_count_read) & 0xFF;
			snprintf(buff, sizeof(buff), "%02X", temp_byte_count_read);

		}else {
			snprintf(buff, sizeof(buff), "%02X", info->cmd_wr_result[i-2]);
		}
		//snprintf(buff, sizeof(buff), "%02X", info->cmd_wr_result[i]);
		strncat(Out_buff, buff, ARRAY_SIZE(Out_buff));
		if(i < (byte_count_read+1)){
			snprintf(buff, sizeof(buff), " ");
			strncat(Out_buff, buff, ARRAY_SIZE(Out_buff));
		}
	}
	snprintf(buff, sizeof(buff), "}");
	strncat(Out_buff, buff, ARRAY_SIZE(Out_buff));
	return snprintf(buf, TSP_BUF_SIZE, "%s\n", Out_buff);
}

static ssize_t st_i2c_wr_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	struct fts_ts_info *info = st_info;
	unsigned char pAddress[8] = {0};
	unsigned int byte_count =0 ;
	int i ;

	unsigned int data[8] = {0};
	memset(data, 0x00, ARRAY_SIZE(data));
	memset(info->cmd_wr_result, 0x00, ARRAY_SIZE(info->cmd_wr_result));
	sscanf(buf, "%x %x %x %x %x %x %x %x ", (data+7), (data),(data+1),(data+2),(data+3),(data+4),(data+5),(data+6));

	byte_count = data[7];

	/*if(sizeof(buf) != byte_count )
	{
		printk("%s : Byte count is wrong\n",__func__);
		return count;
	}*/
#ifdef DEBUG
	printk(" \n");
	printk("%s: Input Data 1:",__func__);

	for(i =0 ; i <7; i++){
		printk(" %02X",data[i]);
		pAddress[i] = (unsigned char)data[i];
	}
	printk("\n");
#else
	for(i =0 ; i <7; i++){
		pAddress[i] = (unsigned char)data[i];
	}
#endif
	byte_count_read = data[byte_count-1];

	ret = st_i2c_write(info,pAddress,3);
	msleep(20);
	ret = st_i2c_read(info,&pAddress[3], (byte_count-4), info->cmd_wr_result ,byte_count_read );

#ifdef DEBUG
	 printk("%s:DATA READ \n{", __func__);
	for(i=0;i<(2+byte_count_read);i++){
		if((i == 0)){
			char temp_byte_count_read = (byte_count_read >> 8) & 0xFF;
			printk("%02X",(unsigned int )temp_byte_count_read);
		}else if(i == 1){
			char temp_byte_count_read = (byte_count_read) & 0xFF;
			printk("%02X",(unsigned int )temp_byte_count_read);

		}else {
			printk("%02X",(unsigned int )info->cmd_read_result[i-2]);
		}
		if(i < (byte_count_read+1)){
			printk( " ");
		}
	}
	printk("}\n");
#endif
	if (ret)
		printk("Unable to read register \n");
	return count;
}

static ssize_t st_i2c_read_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct fts_ts_info *info = st_info;
	int i ;
	char buff[16];

	memset(Out_buff, 0x00, ARRAY_SIZE(Out_buff));
	if(byte_count_read == 0){
		snprintf(Out_buff, sizeof(Out_buff), "{FAILED}");
		return snprintf(buf, TSP_BUF_SIZE, "{%s}\n", Out_buff);
	}
#ifdef DEBUG
	printk("%s:DATA READ {", __func__);
	for(i=0;i<byte_count_read;i++){
		printk("%02X",(unsigned int )info->cmd_read_result[i]);
		if(i < (byte_count_read-1)){
			printk(" ");
		}
	}
	printk("}\n");
#endif
	snprintf(buff, sizeof(buff), "{");
	strncat(Out_buff, buff, ARRAY_SIZE(Out_buff));
	for (i = 0; i < (byte_count_read+2); i++){
		if((i == 0)){
			char temp_byte_count_read = (byte_count_read >> 8) & 0xFF;
			snprintf(buff, sizeof(buff), "%02X",temp_byte_count_read);
		}else if(i == 1){
			char temp_byte_count_read = (byte_count_read) & 0xFF;
			snprintf(buff, sizeof(buff), "%02X", temp_byte_count_read);

		}else{
			snprintf(buff, sizeof(buff), "%02X", info->cmd_read_result[i-2]);
		}
		strncat(Out_buff, buff, ARRAY_SIZE(Out_buff));
		if(i < (byte_count_read+1)){
			snprintf(buff, sizeof(buff), " ");
			strncat(Out_buff, buff, ARRAY_SIZE(Out_buff));
		}
	}
	snprintf(buff, sizeof(buff), "}");
	strncat(Out_buff, buff, ARRAY_SIZE(Out_buff));

	return snprintf(buf, TSP_BUF_SIZE, "%s\n", Out_buff);
}

static ssize_t st_i2c_read_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	struct fts_ts_info *info = st_info;
	unsigned char pAddress[8] = {0};
	unsigned int byte_count =0 ;
	int i ;
	unsigned int data[8] = {0};

	byte_count_read = 0;
	memset(data, 0x00, ARRAY_SIZE(data));
	memset(info->cmd_read_result, 0x00, ARRAY_SIZE(info->cmd_read_result));
	sscanf(buf, "%x %x %x %x %x %x %x %x ", (data+7), (data),(data+1),(data+2),(data+3),(data+4),(data+5),(data+6));
	byte_count = data[7];


	if(byte_count >7 ){
#ifdef DEBUG
		printk("%s : Byte count is more than 7\n",__func__);
#endif
		return count;
	}
	/*if(sizeof(buf) != byte_count )
	{
		printk("%s : Byte count is wrong\n",__func__);
		return count;
	}*/
#ifdef DEBUG
	printk(" \n");
	printk("%s: Input Data 1:",__func__);
	for(i =0 ; i < byte_count; i++){
		 printk(" %02X",data[i]);
		pAddress[i] = (unsigned char)data[i];
	}
	printk(" \n");
#else
	for(i =0 ; i < byte_count; i++){
		pAddress[i] = (unsigned char)data[i];
	}
#endif
	byte_count_read = data[byte_count-1];

	ret = st_i2c_read(info,pAddress, (byte_count-1), info->cmd_read_result ,byte_count_read );
#ifdef DEBUG
	printk("%s:DATA READ \n{", __func__);
	for(i=0;i<(byte_count_read+2);i++){
		if((i == 0)){
			char temp_byte_count_read = (byte_count_read >> 8) & 0xFF;
			printk("%02X",(unsigned int )temp_byte_count_read);
		}else if(i == 1){
			char temp_byte_count_read = (byte_count_read) & 0xFF;
			printk("%02X",(unsigned int )temp_byte_count_read);
		}else{
			printk("%02X",(unsigned int )info->cmd_read_result[i-2]);
		}
		if(i < (byte_count_read+1)){
			printk(" ");
		}
	}
	printk("}\n");
#endif
	if (ret)
		printk("Unable to read register \n");
	return count;
}


static ssize_t st_i2c_write_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	return snprintf(buf, TSP_BUF_SIZE, "%s", st_info->cmd_write_result);

}

static ssize_t st_i2c_write_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	struct fts_ts_info *info = st_info;
	unsigned int byte_count =0 ;
	int i ;

	memset(data, 0x00, ARRAY_SIZE(data));
	memset(pAddress_i2c, 0x00, ARRAY_SIZE(pAddress_i2c));
	memset(info->cmd_write_result, 0x00, ARRAY_SIZE(info->cmd_write_result));
	sscanf(buf, "%u %u", data ,(data + 1));
	byte_count = data[0] << 8 | data[1];

	if(byte_count <= ARRAY_SIZE(pAddress_i2c)){
		/*if(sizeof(buf) != byte_count )
		{
			printk("%s : Byte count is wrong\n",__func__);
			snprintf(info->cmd_write_result, sizeof(info->cmd_write_result), "{Write NOT OK}\n");
		}*/
		for(i=0; i < (byte_count);i++){
			sscanf(&buf[3*(i+2)] , "%u ", (data+i));
		}
	}else{
#ifdef DEBUG
		printk("%s : message size is more than allowed limit of 512 bytes\n",__func__);
#endif
		snprintf(info->cmd_write_result, sizeof(info->cmd_write_result), "{Write NOT OK}\n");
	}
#ifdef DEBUG
	printk(" \n");
	printk("%s: Byte_count=  %02d| Count = %02d | size of buf:%02d\n",__func__,byte_count,(int)count,(int)sizeof(buf));
	printk("%s: Input Data 1:",__func__);
	for(i =0; i < byte_count; i++){
		printk(" %02X",data[i]);
		pAddress_i2c[i] = (unsigned char)data[i];
	}
	printk(" \n");
#else
	for(i =0; i < byte_count; i++){
		pAddress_i2c[i] = (unsigned char)data[i];
	}
#endif
	if((pAddress_i2c[0] == 0xb3)&&(pAddress_i2c[3] == 0xb1)){
		ret = st_i2c_write(info, pAddress_i2c, 3);
		msleep(20);
		ret = st_i2c_write(info,&pAddress_i2c[3], byte_count-3);
	}else{
		ret = st_i2c_write(info,pAddress_i2c, byte_count);
	}

#ifdef DEBUG
	printk("%s:DATA :", __func__);
	for(i=0;i<byte_count;i++){
		printk(" %02X",(unsigned int )pAddress_i2c[i]);
	}
	printk(" byte_count: %02X\n",byte_count);
#endif
	if (ret < 0){
		printk("{Write NOT OK}\n");
		snprintf(info->cmd_write_result, sizeof(info->cmd_write_result), "{Write NOT OK}\n");
	}else{
		snprintf(info->cmd_write_result, sizeof(info->cmd_write_result), "{Write OK}\n");
#ifdef DEBUG
		printk("%s : {Write OK}\n",__func__);
#endif
	}
	return count;
}

static DEVICE_ATTR(iread,(0644), NULL, st_i2c_read_store);
static DEVICE_ATTR(iread_result,(S_IRUGO), st_i2c_read_show, NULL);
static DEVICE_ATTR(iwr,(0644), NULL, st_i2c_wr_store);
static DEVICE_ATTR(iwr_result,(S_IRUGO), st_i2c_wr_show, NULL);
static DEVICE_ATTR(iwrite,(0644), NULL, st_i2c_write_store);
static DEVICE_ATTR(iwrite_result,(S_IRUGO), st_i2c_write_show, NULL);


static struct attribute *i2c_cmd_attributes[] = {
	&dev_attr_iread.attr,
	&dev_attr_iread_result.attr,
	&dev_attr_iwr.attr,
	&dev_attr_iwr_result.attr,
	&dev_attr_iwrite.attr,
	&dev_attr_iwrite_result.attr,
	NULL,
};

static struct attribute_group i2c_cmd_attr_group = {
	.attrs = i2c_cmd_attributes,
};

void st_i2c_cmd_sys_create(struct fts_ts_info *info)
{
	int retval = 0;
	TS_LOG_INFO("create sysfs group for i2c cmd\n");
#if 0
	i2c_cmd_class = class_create(THIS_MODULE,FTS_TS_DRV_NAME);
	info->i2c_cmd_dev = device_create(i2c_cmd_class,
				 NULL, FTS_ID0, info, "st_i2c");
	if (IS_ERR(info->i2c_cmd_dev)){
		TS_LOG_ERR("ST Failed to create device for the sysfs\n");
	}
	retval = sysfs_create_group(&info->i2c_cmd_dev->kobj,
					 &i2c_cmd_attr_group);
	if (retval){
		TS_LOG_ERR("FTS Failed to create sysfs group\n");
	}
#else
	st_info = info;

	retval = sysfs_create_group(&info->pdev->dev.kobj,
					 &i2c_cmd_attr_group);
	if (retval){
		TS_LOG_ERR("FTS Failed to create sysfs group\n");
	}
#endif
}

 /*I2C cmd funtions :END*/

