/*
 * Goodix GT9xx touchscreen driver
 *
 * Copyright  (C)  2010 - 2016 Goodix. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be a reference
 * to you, when you are integrating the GOODiX's CTP IC into your system,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Version: 2.4.7.1
 * Release Date: 2016/12/18
 */

#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include "goodix_ts.h"

#define DATA_LENGTH_UINT    512
#define CMD_HEAD_LENGTH     (sizeof(st_cmd_head) - sizeof(u8*))
static char proc_node_name[20] = {0};

#define UPDATE_FUNCTIONS

#pragma pack(1)
typedef struct{
    u8  wr;         //write read flag, 0:R  1:W  2:PID 3:
    u8  flag;       //0:no need flag/int 1: need flag  2:need int
    u8 flag_addr[2];  //flag address
    u8  flag_val;   //flag val
    u8  flag_relation;  //flag_val:flag 0:not equal 1:equal 2:> 3:<
    u16 circle;     //polling cycle
    u8  times;      //plling times
    u8  retry;      //I2C retry times
    u16 delay;      //delay befor read or after write
    u16 data_len;   //data length
    u8  addr_len;   //address length
    u8  addr[2];    //address
    u8  res[3];     //reserved
    u8* data;       //data pointer
}st_cmd_head;
#pragma pack()
st_cmd_head cmd_head;

static struct proc_dir_entry *goodix_proc_entry;

static ssize_t goodix_tool_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t goodix_tool_write(struct file *, const char __user *, size_t, loff_t *);
static const struct file_operations tool_ops = {
    .owner = THIS_MODULE,
    .read = goodix_tool_read,
    .write = goodix_tool_write,
};

static s32 (*tool_i2c_read)(u8 *, u16);
static s32 (*tool_i2c_write)(u8 *, u16);


s32 DATA_LENGTH = 0;
static s8 IC_TYPE[16] = "GT9XX";

static void tool_set_proc_name(char * proc_name)
{
    sprintf(proc_name, "gmnode");
}

static s32 tool_i2c_read_with_extra(u8* buf, u16 len)
{
	u16 addr = (buf[0] << 8) + buf[1];
	if (!goodix_i2c_read(addr, &buf[2], len)) {
		return 1;
	}
	return -1;
}

static s32 tool_i2c_write_with_extra(u8* buf, u16 len)
{
	u16 addr = (buf[0] << 8) + buf[1];
	if (!goodix_i2c_write(addr, &buf[2], len - 2)) {
		return 1;
	}
	return -1;
}

static void register_i2c_func(void)
{
        tool_i2c_read = tool_i2c_read_with_extra;
        tool_i2c_write = tool_i2c_write_with_extra;
        GTP_DEBUG("I2C function: with pre and end cmd!");
}

static void unregister_i2c_func(void)
{
    tool_i2c_read = NULL;
    tool_i2c_write = NULL;
    GTP_INFO("I2C function: unregister i2c transfer function!");
}

s32 init_wr_node(void)
{
    s32 i;

    memset(&cmd_head, 0, sizeof(cmd_head));
    cmd_head.data = NULL;

    i = 5;
    while ((!cmd_head.data) && i)
    {
        cmd_head.data = (u8*)kzalloc(5 * DATA_LENGTH_UINT, GFP_KERNEL);
        if (NULL != cmd_head.data)
        {
            break;
        }
        i--;
    }
    if (i)
    {
        DATA_LENGTH = 5 * DATA_LENGTH_UINT + GTP_ADDR_LENGTH;
        GTP_INFO("Applied memory size:%d.", DATA_LENGTH);
    }
    else
    {
        GTP_ERROR("Apply for memory failed.");
        return FAIL;
    }

    cmd_head.addr_len = 2;
    cmd_head.retry = 5;

    register_i2c_func();

    tool_set_proc_name(proc_node_name);
    goodix_proc_entry = proc_create(proc_node_name, 0666, NULL, &tool_ops);
    if (goodix_proc_entry == NULL)
    {
        GTP_ERROR("Couldn't create proc entry!");
        return FAIL;
    }
    else
    {
        GTP_INFO("Create proc entry success!");
    }

    return SUCCESS;
}

void uninit_wr_node(void)
{
    kfree(cmd_head.data);
    cmd_head.data = NULL;
    unregister_i2c_func();
    remove_proc_entry(proc_node_name, NULL);
}

static u8 relation(u8 src, u8 dst, u8 rlt)
{
    u8 ret = 0;

    switch (rlt)
    {
    case 0:
        ret = (src != dst) ? true : false;
        break;

    case 1:
        ret = (src == dst) ? true : false;
        GTP_DEBUG("equal:src:0x%02x   dst:0x%02x   ret:%d.", src, dst, (s32)ret);
        break;

    case 2:
        ret = (src > dst) ? true : false;
        break;

    case 3:
        ret = (src < dst) ? true : false;
        break;

    case 4:
        ret = (src & dst) ? true : false;
        break;

    case 5:
        ret = (!(src | dst)) ? true : false;
        break;

    default:
        ret = false;
        break;
    }

    return ret;
}

/*******************************************************
Function:
    Comfirm function.
Input:
  None.
Output:
    Return write length.
********************************************************/
static u8 comfirm(void)
{
    s32 i = 0;
    u8 buf[32];

    memcpy(buf, cmd_head.flag_addr, cmd_head.addr_len);

    for (i = 0; i < cmd_head.times; i++)
    {
        if (tool_i2c_read(buf, 1) <= 0)
        {
            GTP_ERROR("Read flag data failed!");
            return FAIL;
        }
        if (true == relation(buf[GTP_ADDR_LENGTH], cmd_head.flag_val, cmd_head.flag_relation))
        {
            GTP_DEBUG("value at flag addr:0x%02x.", buf[GTP_ADDR_LENGTH]);
            GTP_DEBUG("flag value:0x%02x.", cmd_head.flag_val);
            break;
        }

        msleep(cmd_head.circle);
    }

    if (i >= cmd_head.times)
    {
        GTP_ERROR("Didn't get the flag to continue!");
        return FAIL;
    }

    return SUCCESS;
}

/*******************************************************
Function:
    Goodix tool write function.
Input:
  standard proc write function param.
Output:
    Return write length.
********************************************************/
//static s32 goodix_tool_write(struct file *filp, const char __user *buff, unsigned long len, void *data)
static ssize_t goodix_tool_write(struct file *filp, const char __user *buff, size_t len, loff_t *off)
{
    s32 ret = 0;

    GTP_DEBUG_FUNC();
    GTP_DEBUG_ARRAY((u8*)buff, len);

    ret = copy_from_user(&cmd_head, buff, CMD_HEAD_LENGTH);
    if(ret)
    {
        GTP_ERROR("copy_from_user failed.");
        return -EPERM;
    }

    GTP_DEBUG("[Operation]wr: %02X", cmd_head.wr);
    GTP_DEBUG("[Flag]flag: %02X, addr: %02X%02X, value: %02X, relation: %02X", cmd_head.flag, cmd_head.flag_addr[0],
                        cmd_head.flag_addr[1], cmd_head.flag_val, cmd_head.flag_relation);
    GTP_DEBUG("[Retry]circle: %d, times: %d, retry: %d, delay: %d", (s32)cmd_head.circle, (s32)cmd_head.times,
                        (s32)cmd_head.retry, (s32)cmd_head.delay);
    GTP_DEBUG("[Data]data len: %d, addr len: %d, addr: %02X%02X, buffer len: %d", (s32)cmd_head.data_len,
                        (s32)cmd_head.addr_len, cmd_head.addr[0], cmd_head.addr[1], (s32)len);

    if (1 == cmd_head.wr)
    {
	if (DATA_LENGTH < cmd_head.data_len) {
		TS_LOG_ERR("%s,%d:Data_len is to large, must no more than %d\n!",
					__func__,__LINE__,DATA_LENGTH);
		return -EPERM;
	}
        ret = copy_from_user(&cmd_head.data[GTP_ADDR_LENGTH], &buff[CMD_HEAD_LENGTH], cmd_head.data_len);
        if(ret)
        {
            GTP_ERROR("copy_from_user failed.");
            return -EPERM;
        }
        memcpy(&cmd_head.data[GTP_ADDR_LENGTH - cmd_head.addr_len], cmd_head.addr, cmd_head.addr_len);

        GTP_DEBUG_ARRAY(cmd_head.data, cmd_head.data_len + cmd_head.addr_len);
        GTP_DEBUG_ARRAY((u8*)(&buff[CMD_HEAD_LENGTH]), cmd_head.data_len);

        if (1 == cmd_head.flag)
        {
            if (FAIL == comfirm())
            {
                GTP_ERROR("[WRITE]Comfirm fail!");
                return -EPERM;
            }
        }
        else if (2 == cmd_head.flag)
        {
            //Need interrupt!
        }
        if (tool_i2c_write(&cmd_head.data[GTP_ADDR_LENGTH - cmd_head.addr_len],
            cmd_head.data_len + cmd_head.addr_len) <= 0)
        {
            GTP_ERROR("[WRITE]Write data failed!");
            return -EPERM;
        }

        GTP_DEBUG_ARRAY(&cmd_head.data[GTP_ADDR_LENGTH - cmd_head.addr_len],cmd_head.data_len + cmd_head.addr_len);
        if (cmd_head.delay)
        {
            msleep(cmd_head.delay);
        }
    }
    else if (3 == cmd_head.wr)  //Write ic type
    {
		if (DATA_LENGTH < cmd_head.data_len) {
			TS_LOG_ERR("%s,%d:Data_len is to large, must no more than %d\n!",
						__func__,__LINE__,DATA_LENGTH);
			return -EPERM;
		}
        ret = copy_from_user(&cmd_head.data[0], &buff[CMD_HEAD_LENGTH], cmd_head.data_len);
        if(ret)
        {
            GTP_ERROR("copy_from_user failed.");
            return -EPERM;
        }
		if (sizeof(IC_TYPE) < cmd_head.data_len) {
			TS_LOG_ERR("%s,%d:Data_len is to large, must no more than %d\n",
						__func__,__LINE__,sizeof(IC_TYPE));
			return -EPERM;
		}
        memcpy(IC_TYPE, cmd_head.data, cmd_head.data_len);

        register_i2c_func();
    }
    else if (5 == cmd_head.wr)
    {
        //memcpy(IC_TYPE, cmd_head.data, cmd_head.data_len);
    }
    else if (7 == cmd_head.wr)//disable irq!
    {
    /*
        gtp_irq_disable(i2c_get_clientdata(gt_client));
        
    #if GTP_ESD_PROTECT
        gtp_esd_switch(gt_client, SWITCH_OFF);
    #endif
    */
    }
    else if (9 == cmd_head.wr) //enable irq!
    {
    /*
        gtp_irq_enable(i2c_get_clientdata(gt_client));

    #if GTP_ESD_PROTECT
        gtp_esd_switch(gt_client, SWITCH_ON);
    #endif
    */
    }
    else if(17 == cmd_head.wr)
    {
	if (DATA_LENGTH < cmd_head.data_len) {
		TS_LOG_ERR("%s,%d:Data_len is to large, must no more than %d\n!",
					__func__,__LINE__,DATA_LENGTH);
		return -EPERM;
	}
        ret = copy_from_user(&cmd_head.data[GTP_ADDR_LENGTH], &buff[CMD_HEAD_LENGTH], cmd_head.data_len);
        if(ret)
        {
            GTP_DEBUG("copy_from_user failed.");
            return -EPERM;
        }
        if(cmd_head.data[GTP_ADDR_LENGTH])
        {
            GTP_INFO("gtp enter rawdiff.");
            goodix_ts->rawdiff_mode = true;
        }
        else
        {
            goodix_ts->rawdiff_mode = false;
            GTP_INFO("gtp leave rawdiff.");
        }
    }
#ifdef UPDATE_FUNCTIONS
    else if (11 == cmd_head.wr)//Enter update mode!
    {
        if (FAIL == gup_enter_update_mode())
        {
            return -EPERM;
        }
    }
    else if (13 == cmd_head.wr)//Leave update mode!
    {
        gup_leave_update_mode();
    }
    else if (15 == cmd_head.wr) //Update firmware!
    {
        if (FAIL == gup_update_proc(0))
        {
            return -EPERM;
        }
    }

#endif

    return len;
}

/*******************************************************
Function:
    Goodix tool read function.
Input:
  standard proc read function param.
Output:
    Return read length.
********************************************************/
static ssize_t goodix_tool_read(struct file *file, char __user *page, size_t size, loff_t *ppos)
{
    s32 ret = 0;

    GTP_DEBUG_FUNC();

    if (*ppos)      // ADB call again
    {
        *ppos = 0;
        return 0;
    }

    if (cmd_head.wr % 2)
    {
        return -EPERM;
    }
    else if (!cmd_head.wr)
    {
        u16 len = 0;
        s16 data_len = 0;
        u16 loc = 0;

        if (1 == cmd_head.flag)
        {
            if (FAIL == comfirm())
            {
                GTP_ERROR("[READ]Comfirm fail!");
                return -EPERM;
            }
        }
        else if (2 == cmd_head.flag)
        {
            //Need interrupt!
        }

        memcpy(cmd_head.data, cmd_head.addr, cmd_head.addr_len);

        GTP_DEBUG("[CMD HEAD DATA] ADDR:0x%02x%02x.", cmd_head.data[0], cmd_head.data[1]);
        GTP_DEBUG("[CMD HEAD ADDR] ADDR:0x%02x%02x.", cmd_head.addr[0], cmd_head.addr[1]);

        if (cmd_head.delay)
        {
            msleep(cmd_head.delay);
        }

        data_len = cmd_head.data_len;

        while(data_len > 0)
        {
            if (data_len > DATA_LENGTH)
            {
                len = DATA_LENGTH;
            }
            else
            {
                len = data_len;
            }
            data_len -= len;

            if (tool_i2c_read(cmd_head.data, len) <= 0)
            {
                GTP_ERROR("[READ]Read data failed!");
                return -EPERM;
            }

            //memcpy(&page[loc], &cmd_head.data[GTP_ADDR_LENGTH], len);
            ret = simple_read_from_buffer(&page[loc], size, ppos, &cmd_head.data[GTP_ADDR_LENGTH], len);
            if (ret < 0)
            {
                return ret;
            }
            loc += len;

            GTP_DEBUG_ARRAY(&cmd_head.data[GTP_ADDR_LENGTH], len);
            GTP_DEBUG_ARRAY(page, len);
        }
        return cmd_head.data_len;
    }
    else if (2 == cmd_head.wr)
    {
        ret = simple_read_from_buffer(page, size, ppos, IC_TYPE, sizeof(IC_TYPE));
        return ret;
    }
    else if (4 == cmd_head.wr)
    {
        u8 progress_buf[4];
        progress_buf[0] = show_len >> 8;
        progress_buf[1] = show_len & 0xff;
        progress_buf[2] = total_len >> 8;
        progress_buf[3] = total_len & 0xff;

        ret = simple_read_from_buffer(page, size, ppos, progress_buf, 4);
        return ret;
    }
    else if (6 == cmd_head.wr)
    {

    }
    else if (8 == cmd_head.wr)  //Read driver version
    {
        ret = simple_read_from_buffer(page, size, ppos, GTP_DRIVER_VERSION, strlen(GTP_DRIVER_VERSION));
        return ret;
    }
    return -EPERM;
}
