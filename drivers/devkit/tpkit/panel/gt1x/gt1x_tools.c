/*
 * gt1x_ts_tools.c - gt1x debug tools
 *
 * 2010 - 2016 gt1x Technology.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be a reference
 * to you, when you are integrating the gt1x's CTP IC into your system,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/atomic.h>
#include "gt1x.h"

static ssize_t gt1x_tool_read(struct file *filp, char __user * buffer, size_t count, loff_t * ppos);
static ssize_t gt1x_tool_write(struct file *filp, const char *buffer, size_t count, loff_t * ppos);

static int gt1x_tool_release(struct inode *inode, struct file *filp);
static int gt1x_tool_open(struct inode *inode,struct file *file);

extern int gt1x_update_firmware(void);
extern void gt1x_leave_update_mode(void);

#pragma pack(1)
typedef struct {
	u8 wr;			//write read flag£¬0:R  1:W  2:PID 3:
	u8 flag;		//0:no need flag/int 1: need flag  2:need int
	u8 flag_addr[2];	//flag address 
	u8 flag_val;		//flag val
	u8 flag_relation;	//flag_val:flag 0:not equal 1:equal 2:> 3:<
	u16 circle;		//polling cycle
	u8 times;		//plling times
	u8 retry;		//I2C retry times
	u16 delay;		//delay befor read or after write
	u16 data_len;		//data length
	u8 addr_len;		//address length
	u8 addr[2];		//address
	u8 res[3];		//reserved
	u8 *data;		//data pointer
} st_cmd_head;
#pragma pack()

static struct file_operations gt1x_tool_fops = {
	.read = gt1x_tool_read,
	.write = gt1x_tool_write,
	.open = gt1x_tool_open,
	.release = gt1x_tool_release,
	.owner = THIS_MODULE,
};

#define WRITE_REG_MODE     1
#define SET_IC_TYPE        3
#define GET_BUF_LENGTH     5
#define DISABLE_IRQ_MODE   7
#define ENABLE_IRQ_MODE    9
#define LEAVE_UPDATA_MODE  13
#define ENTER_UPDATA_MODE  15
#define ENTER_DATA_MODE    17
#define CHECK_IC_STATUS    100
#define GET_UPDATE_STAUTS  101
#define FORCE_UPDATA_MODE  102	
#define  CHECK_DATA        1

#define READ_IC_TYPE		2
#define READ_FW_UPDATE_PROGRESS	4
#define READ_ERROR		6
#define READ_DRIVER_VERSION	8
#define DATA_LENGTH_UINT	512
#define CMD_HEAD_LENGTH		(sizeof(st_cmd_head) - sizeof(u8*))
#define OFFSET_8BITS  		8

static st_cmd_head cmd_head;
static s32 DATA_LENGTH = 0;
static s8 IC_TYPE[16] = "GT1X";
static char procname[] = "gmnode";
static struct proc_dir_entry *gt1x_tool_proc_entry=NULL;


int gt1x_init_tool_node(void)
{
	memset(&cmd_head, 0, sizeof(cmd_head));
	cmd_head.wr = 1;	//if the first operation is read, will return fail.
	cmd_head.data = kzalloc(DATA_LENGTH_UINT, GFP_KERNEL);
	if (NULL == cmd_head.data) {
		TS_LOG_ERR("Apply for memory failed.\n");
		return -ENOMEM;
	}
	TS_LOG_DEBUG("Alloc memory size:%d.\n", DATA_LENGTH_UINT);
	DATA_LENGTH = DATA_LENGTH_UINT - GTP_ADDR_LENGTH;

	gt1x_tool_proc_entry = proc_create(procname, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,  NULL, &gt1x_tool_fops);
	if (gt1x_tool_proc_entry == NULL) {
		TS_LOG_ERR("CAN't create proc entry /proc/%s.\n", procname);
		goto exit;
	} else {
		TS_LOG_INFO("Created proc entry /proc/%s.\n", procname);
	}
	return NO_ERR;
exit:
	if(cmd_head.data){
		kfree(cmd_head.data);
		cmd_head.data = NULL;
	}

	return -EIO;
}

void gt1x_deinit_tool_node(void)
{
	if(gt1x_tool_proc_entry)
		remove_proc_entry(procname, NULL);
	if(cmd_head.data){
		kfree(cmd_head.data);
		cmd_head.data = NULL;
	}
}

static s32 tool_i2c_read(u8 * buf, u16 len)
{
	u16 addr = (buf[0] << OFFSET_8BITS) + buf[1];
	if (!gt1x_i2c_read(addr, &buf[2], len)) {
		return NO_ERR;
	}
	return RESULT_ERR;
}

static s32 tool_i2c_write(u8 * buf, u16 len)
{
	u16 addr = (buf[0] << OFFSET_8BITS) + buf[1];
	if (!gt1x_i2c_write(addr, &buf[2], len - 2)) {
		return NO_ERR;
	}
	return RESULT_ERR;
}
enum {NOT_EQU = 0, EQUAL, GREATER_THAN, LESS_HTAN, AND_OPERA, OR_OPERA};
static u8 relation(u8 src, u8 dst, u8 rlt)
{
	u8 ret = 0;

	switch (rlt) {
	case NOT_EQU:
		ret = (src != dst) ? true : false;
		break;

	case EQUAL:
		ret = (src == dst) ? true : false;
		TS_LOG_DEBUG("equal:src:0x%02x   dst:0x%02x   ret:%d.\n", src, dst, (s32) ret);
		break;

	case GREATER_THAN:
		ret = (src > dst) ? true : false;
		break;

	case LESS_HTAN:
		ret = (src < dst) ? true : false;
		break;

	case AND_OPERA:
		ret = (src & dst) ? true : false;
		break;

	case OR_OPERA:
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
static char comfirm(void)
{
	s32 i = 0;
	u8 buf[32] = {0};

	if (cmd_head.addr_len > sizeof(buf))
		cmd_head.addr_len = sizeof(buf);
	memcpy(buf, cmd_head.flag_addr, (cmd_head.addr_len > sizeof(buf)) ? sizeof(buf): cmd_head.addr_len);

	for (i = 0; i < cmd_head.times; i++) {
		if (tool_i2c_read(buf, 1)) {
			TS_LOG_ERR("Read flag data failed!\n");
			return RESULT_ERR;
		}

		if (true == relation(buf[GTP_ADDR_LENGTH], cmd_head.flag_val, cmd_head.flag_relation)) {
			TS_LOG_DEBUG("value at flag addr:0x%02x.\n", buf[GTP_ADDR_LENGTH]);
			TS_LOG_DEBUG("flag value:0x%02x.\n", cmd_head.flag_val);
			break;
		}

		msleep(cmd_head.circle);
	}

	if (i >= cmd_head.times) {
		TS_LOG_ERR("Didn't get the flag to continue!\n");
		return RESULT_ERR;
	}

	return NO_ERR;
}

/*******************************************************
Function:
    gt1x tool write function.
Input:
  standard proc write function param.
Output:
    Return write length.
********************************************************/
static ssize_t gt1x_tool_write(struct file *filp, const char __user * buff, size_t len, loff_t * data)
{
	u64 ret = 0;
	int data_len = 0;
	if(!buff){
		TS_LOG_ERR("%s: invalid buff\n", __func__);
		return -EINVAL;
	}
	ret = copy_from_user(&cmd_head, buff, CMD_HEAD_LENGTH);
	if (ret) {
		TS_LOG_ERR("copy_from_user failed.\n");
		return -EIO;
	}
	TS_LOG_DEBUG ("wr  :0x%02x.\n", cmd_head.wr);

	if (WRITE_REG_MODE == cmd_head.wr) {
		u16 addr, pos;

		if (CHECK_DATA == cmd_head.flag) {
			if (comfirm()) {
				TS_LOG_ERR("[WRITE]Comfirm fail!\n");
				return RESULT_ERR;
			}
		}

		addr = (cmd_head.addr[0] << OFFSET_8BITS) + cmd_head.addr[1];
		data_len = cmd_head.data_len;
		pos = 0;
		while (data_len > 0) {
			len = data_len > DATA_LENGTH ? DATA_LENGTH : data_len;
			ret = copy_from_user(&cmd_head.data[GTP_ADDR_LENGTH], &buff[CMD_HEAD_LENGTH + pos], len);
			if (ret) {
				TS_LOG_ERR("[WRITE]copy_from_user failed.\n");
				return RESULT_ERR;
			}
			cmd_head.data[0] = ((addr >> OFFSET_8BITS) & 0xFF);
			cmd_head.data[1] = (addr & 0xFF);

			if (tool_i2c_write(cmd_head.data, len + GTP_ADDR_LENGTH) ) {
				TS_LOG_ERR("[WRITE]Write data failed!\n");
				return RESULT_ERR;
			}
			addr += len;
			pos += len;
			data_len -= len;
		}

		if (cmd_head.delay) {
			msleep(cmd_head.delay);
		}

		return cmd_head.data_len + CMD_HEAD_LENGTH;
	} else if (SET_IC_TYPE == cmd_head.wr) {	//gt1x unused
		data_len = cmd_head.data_len > (sizeof(IC_TYPE)-1)?(sizeof(IC_TYPE)-1): cmd_head.data_len;
		memcpy(IC_TYPE, cmd_head.data, data_len);
		return cmd_head.data_len + CMD_HEAD_LENGTH;
	} else if (GET_BUF_LENGTH== cmd_head.wr) {	//?
		return cmd_head.data_len + CMD_HEAD_LENGTH;
	} else if (ENTER_DATA_MODE == cmd_head.wr) {
		data_len = cmd_head.data_len > DATA_LENGTH ? DATA_LENGTH : cmd_head.data_len;
		ret = copy_from_user(&cmd_head.data[GTP_ADDR_LENGTH], &buff[CMD_HEAD_LENGTH], data_len);
		if (ret) {
			TS_LOG_ERR("copy_from_user failed.\n");
			return RESULT_ERR;
		}

		if (cmd_head.data[GTP_ADDR_LENGTH]) {
			TS_LOG_DEBUG("gtp enter rawdiff.\n");
			gt1x_ts->rawdiff_mode = true;
		} else {
			gt1x_ts->rawdiff_mode = false;
			TS_LOG_DEBUG("gtp leave rawdiff.\n");
		}

		return CMD_HEAD_LENGTH;
	} else if (LEAVE_UPDATA_MODE == cmd_head.wr) {
		gt1x_leave_update_mode();
	} else if (ENTER_UPDATA_MODE == cmd_head.wr) {
		data_len = cmd_head.data_len > DATA_LENGTH_UINT ? DATA_LENGTH_UINT : cmd_head.data_len;
		memset(cmd_head.data, 0, DATA_LENGTH_UINT);
		copy_from_user(cmd_head.data, &buff[CMD_HEAD_LENGTH], data_len);
		memset(gt1x_ts->firmware_name, 0, GT1X_FW_NAME_LEN);
		snprintf(gt1x_ts->firmware_name, GT1X_FW_NAME_LEN, "%s",cmd_head.data);
		TS_LOG_INFO("%s: fw_name=%s\n",__func__, gt1x_ts->firmware_name);
		return gt1x_update_firmware();
	} else if (cmd_head.wr == CHECK_IC_STATUS) {
		if (!gt1x_ts->sensor_id_valid)
			return -EINVAL;
	} else if (cmd_head.wr == GET_UPDATE_STAUTS) {
		if (gt1x_ts->fw_update_ok == false)
			return -EINVAL;
	} else if (cmd_head.wr == FORCE_UPDATA_MODE) {
		if (cmd_head.data[0])
			update_info.force_update = true;
		else
			update_info.force_update = false;
	}else{
		TS_LOG_INFO("write active end\n");
	}

	return CMD_HEAD_LENGTH;
}
#define GT1X_TOOLS_OPEN 1
#define GT1X_TOOLS_CLOSE 0
static atomic_t open_flag = ATOMIC_INIT(GT1X_TOOLS_CLOSE);
static int gt1x_tool_open(struct inode *inode,struct file *file)
{
	if(GT1X_TOOLS_CLOSE == atomic_read(&open_flag)){
		atomic_set(&open_flag, GT1X_TOOLS_OPEN);
		TS_LOG_INFO("%s: open sucess\n", __func__);
		return NO_ERR;
	}else{
		TS_LOG_ERR("tools already open!\n");
		return -ERESTARTSYS;
	}
}

static int gt1x_tool_release(struct inode *inode, struct file *filp)
{
	atomic_set(&open_flag, GT1X_TOOLS_CLOSE);
	return NO_ERR;
}
/*******************************************************
Function:
	gt1x tool read function.
Input:
	standard proc read function param.
Output:
	Return read length.
********************************************************/
static ssize_t gt1x_tool_read(struct file *filp, char __user * buffer, size_t count, loff_t * ppos)
{
	u16 addr, len, loc = 0;
	int data_len = 0;
	u8 tmp =0;

	if(!buffer){
		TS_LOG_ERR("%s: invalid buff\n", __func__);
		return -EINVAL;
	}
	if(*ppos) {
		TS_LOG_DEBUG("[PARAM]size: %zd, *ppos: %d\n", count, (int)*ppos);
		*ppos = 0;
		return NO_ERR;
	}

	if (cmd_head.wr % 2) {
		TS_LOG_ERR("[READ] invaild operator fail!\n");
		return RESULT_ERR;
	} else if (!cmd_head.wr) {
		/* general  i2c read  */
		if (CHECK_DATA == cmd_head.flag) {
			if (comfirm()) {
				TS_LOG_ERR("[READ]Comfirm fail!\n");
				return RESULT_ERR;
			}
		} 

		addr = (cmd_head.addr[0] << OFFSET_8BITS) + cmd_head.addr[1];
		data_len = cmd_head.data_len;
		

		TS_LOG_DEBUG("[READ] ADDR:0x%04X.\n", addr);
		TS_LOG_DEBUG("[READ] Length: %d\n", data_len);

		if (cmd_head.delay) {
			msleep(cmd_head.delay);
		}

		while (data_len > 0) {
			len = data_len > DATA_LENGTH ? DATA_LENGTH : data_len;
			cmd_head.data[0] = (addr >> OFFSET_8BITS) & 0xFF;
			cmd_head.data[1] = (addr & 0xFF);
			if (tool_i2c_read(cmd_head.data, len) ) {
				TS_LOG_ERR("[READ]Read data failed!\n");
				return RESULT_ERR;
			}
			copy_to_user(&buffer[loc], &cmd_head.data[GTP_ADDR_LENGTH], len);
			data_len -= len;
			addr += len;
			loc += len;
		}
		 *ppos += cmd_head.data_len;
		return cmd_head.data_len;
	} else if (READ_IC_TYPE == cmd_head.wr) {
		TS_LOG_DEBUG("Return ic type:%s len:%d.\n", buffer, (s32) cmd_head.data_len);
		return RESULT_ERR;
	} else if (READ_FW_UPDATE_PROGRESS == cmd_head.wr) {
	    /* read fw update progress */
		tmp = update_info.progress >> OFFSET_8BITS;
		copy_to_user(&buffer[0], &tmp,sizeof(tmp));
		tmp = update_info.progress & 0xff;
		copy_to_user(&buffer[1], &tmp,sizeof(tmp));
		tmp = update_info.max_progress >> OFFSET_8BITS;
		copy_to_user(&buffer[2], &tmp,sizeof(tmp));
		tmp = update_info.max_progress & 0xff;
		copy_to_user(&buffer[3], &tmp,sizeof(tmp));
		*ppos += 4;
		return READ_FW_UPDATE_PROGRESS;
	} else if (READ_ERROR == cmd_head.wr) {
		//Read error code!
		return RESULT_ERR;
	} else if (READ_DRIVER_VERSION == cmd_head.wr) {	
		/* Read driver version */
		s32 tmp_len;
		tmp_len = strlen(GTP_DRIVER_VERSION);
		copy_to_user(buffer, GTP_DRIVER_VERSION, sizeof(GTP_DRIVER_VERSION));
		*ppos += tmp_len + 1;
		return (tmp_len + 1);
	}

	*ppos += cmd_head.data_len;
	return cmd_head.data_len;
}
