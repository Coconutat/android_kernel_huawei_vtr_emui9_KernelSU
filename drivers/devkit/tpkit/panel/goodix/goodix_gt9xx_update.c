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

#include <linux/kthread.h>
#include <linux/firmware.h>

#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/delay.h>

#include "goodix_ts.h"
#include "goodix_dts.h"

#define GT9XX_FW_NAME "gt9xx.BIN"

#define GUP_REG_HW_INFO             0x4220
#define GUP_REG_FW_MSG              0x41E4
#define GUP_REG_PID_VID             0x8140

#define GUP_SEARCH_FILE_TIMES       5
#define GUP_ENTER_UPDATE_TIMES       5

#define GTP_FW_MANUAL_UPDATE_FILE_NAME          "/vendor/firmware/ts/touch_screen_firmware.img"
#define GTP_FW_NAME						      "ts/gtp_firmware.img"

#define CONFIG_FILE_PATH_1          "/data/_goodix_config_.cfg"
#define CONFIG_FILE_PATH_2          "/sdcard/_goodix_config_.cfg"

#define FW_HEAD_LENGTH               14
#define FW_SECTION_LENGTH            0x2000         // 8K
#define FW_DSP_ISP_LENGTH            0x1000         // 4K
#define FW_DSP_LENGTH                0x1000         // 4K
#define FW_BOOT_LENGTH               0x800          // 2K
#define FW_SS51_LENGTH               (4 * FW_SECTION_LENGTH)    // 32K
#define FW_BOOT_ISP_LENGTH           0x800                     // 2k
#define FW_GLINK_LENGTH              0x3000                    // 12k
#define FW_GWAKE_LENGTH              (4 * FW_SECTION_LENGTH)   // 32k

#define PACK_SIZE                    256
#define MAX_FRAME_CHECK_TIME         5



#define _bRW_MISCTL__SRAM_BANK       0x4048
#define _bRW_MISCTL__MEM_CD_EN       0x4049
#define _bRW_MISCTL__CACHE_EN        0x404B
#define _bRW_MISCTL__TMR0_EN         0x40B0
#define _rRW_MISCTL__SWRST_B0_       0x4180
#define _bWO_MISCTL__CPU_SWRST_PULSE 0x4184
#define _rRW_MISCTL__BOOTCTL_B0_     0x4190
#define _rRW_MISCTL__BOOT_OPT_B0_    0x4218
#define _rRW_MISCTL__BOOT_CTL_       0x5094

#define AUTO_SEARCH_BIN           0x01
#define AUTO_SEARCH_CFG           0x02
#define BIN_FILE_READY            0x80
#define CFG_FILE_READY            0x08
#define HEADER_FW_READY           0x00
#if defined(CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif

#pragma pack(1)
typedef struct
{
    u8  hw_info[4];          //hardware info//
    u8  pid[8];              //product id   //
    u16 vid;                 //version id   //
}st_fw_head;
#pragma pack()

typedef struct
{
    int update_type;
    u8 force_update;
    u8 fw_flag;
    const struct firmware *fw;
    struct file *file;
    struct file *cfg_file;
    st_fw_head  ic_fw_msg;
    mm_segment_t old_fs;
    u32 fw_total_len;
    u32 fw_burned_len;
}st_update_msg;

st_update_msg update_msg;
u16 show_len;
u16 total_len;
u8 got_file_flag = 0;
u8 searching_file = 0;

extern u8 config[GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH];
extern void gtp_reset_guitar(struct i2c_client *client, s32 ms);

extern s32 gtp_read_version(struct i2c_client *, u16* );

static u8 gup_burn_fw_gwake_section(u8 *fw_section, u16 start_addr, u32 len, u8 bank_cmd );

#define _CLOSE_FILE(p_file) if (p_file && !IS_ERR(p_file)) \
                            { \
                                filp_close(p_file, NULL); \
                            }

/*******************************************************
Function:
    Read data from the i2c slave device.
Input:
    client:     i2c device.
    buf[0~1]:   read start address.
    buf[2~len-1]:   read data buffer.
    len:    GTP_ADDR_LENGTH + read bytes count
Output:
    numbers of i2c_msgs to transfer:
      2: succeed, otherwise: failed
*********************************************************/
s32 gup_i2c_read(u8 *buf, s32 len)
{
	struct ts_bus_info *bops = g_goodix_dev_data->ts_platform_data->bops;
	int ret;
	u16 addr;

	if (unlikely(!bops || !bops->bus_read))
		return -ENODEV;

	addr = ((buf[0] & 0xff)<< 8)+(buf[1] & 0xff);
	//addr = cpu_to_be16(addr);
	GTP_INFO("i2c read,addr:0x%04x bytes:%d", addr, len);
	//ret = bops->bus_read((u8 *)&addr, 2, &buf[GTP_ADDR_LENGTH], len-GTP_ADDR_LENGTH);
	ret = goodix_i2c_read(addr, &buf[2], len-2);
	if (ret != 0){
		GTP_ERROR("i2c read error,addr:%04x bytes:%d", addr, len-GTP_ADDR_LENGTH);
	}else{
		ret = 2;
	}

	return ret;
}

/*******************************************************
Function:
    Write data to the i2c slave device.
Input:
    client:     i2c device.
    buf[0~1]:   write start address.
    buf[2~len-1]:   data buffer
    len:    GTP_ADDR_LENGTH + write bytes count
Output:
    numbers of i2c_msgs to transfer:
        1: succeed, otherwise: failed
*********************************************************/
s32 gup_i2c_write(u8 *buffer,u32 len)
{
	struct ts_bus_info *bops = g_goodix_dev_data->ts_platform_data->bops;
	u8 stack_mem[32], *data;
	int ret;
	u16 addr;

u8 buf[1024];

	if (unlikely(!bops || !bops->bus_write))
		return -ENODEV;

	if (len> sizeof(stack_mem)) {
		data = kmalloc(len, GFP_KERNEL);
		if (!data) {
			GTP_ERROR("No memory");
			return -ENOMEM;
		}
	} else {
		data = &stack_mem[0];
	}

	data[0] = buffer[0] & 0xff;
	data[1] = buffer[1] & 0xff;
	addr = (data[0]<<8)|data[1];
	GTP_DEBUG_ARRAY(&buffer[0], len);
	if(len < 2)
		return -ENODEV;	
	memcpy(&data[2], &buffer[2], len-2);
	GTP_INFO("i2c write,addr:0x%04x bytes:%d", addr, len);

ret = goodix_i2c_write(addr, &buffer[2], len-2);

#if 1

 goodix_i2c_read(addr, &buf[0], len-2);
 GTP_DEBUG("write-ro-read-array-----:");
 GTP_DEBUG_ARRAY(&buf[0], len-2);
#endif

	if (ret != 0){
		GTP_ERROR("i2c write error,addr:0x%04x bytes:%d", addr, len-2);
	}else{
		ret = 1;
	}

	if (data != &stack_mem[0])
		kfree(data);

	return ret;
}

static s32 gup_init_panel(struct goodix_ts_data *ts)
{
	int ret;
	ret = goodix_init_configs(ts);
	if (ret < 0) {
		GTP_ERROR("Init panel failed");
	}
	return ret;
}


static u8 gup_get_ic_msg(u16 addr, u8* msg, s32 len)
{
    s32 i = 0;
    for (i = 0; i < 5; i++)
    {
        if (goodix_i2c_read(addr, msg, (u16)len) == 0)
            break;
    }

    if (i >= 5)
    {
        GTP_ERROR("Read data from 0x%x failed!", addr);
        return FAIL;
    }

    return SUCCESS;
}

static u8 gup_set_ic_msg(u16 addr, u8 val)
{
    s32 i = 0;
    u8 msg[1];
    msg[0] = val;

    for (i = 0; i < 5; i++)
    {
        if (goodix_i2c_write(addr, msg, 1) == 0)
        {
            break;
        }
    }

    if (i >= 5)
    {
        GTP_ERROR("Set data to 0x%x failed!", addr);
        return FAIL;
    }

    return SUCCESS;
}

static u8 gup_get_ic_fw_msg(void)
{
    s32 ret = -1;
    u8  retry = 0;
    u8  buf[16];
    u8  i;

    // step1:get hardware info
    ret = goodix_i2c_read_dbl_check(GUP_REG_HW_INFO, &buf[0], 4);
    if (0 != ret)
    {
        GTP_ERROR("[get_ic_fw_msg]get hw_info failed,exit");
        return FAIL;
    }

    // buf[2~5]: 00 06 90 00
    // hw_info: 00 90 06 00
    for(i=0; i<4; i++)
    {
        update_msg.ic_fw_msg.hw_info[i] = buf[0 + 3 - i];
    }
    GTP_DEBUG("IC Hardware info:%02x%02x%02x%02x", update_msg.ic_fw_msg.hw_info[0], update_msg.ic_fw_msg.hw_info[1],
                                                   update_msg.ic_fw_msg.hw_info[2], update_msg.ic_fw_msg.hw_info[3]);
    // step2:get firmware message
    for(retry=0; retry<2; retry++)
    {
        ret = gup_get_ic_msg(GUP_REG_FW_MSG, buf, 1);
        if(FAIL == ret)
        {
            GTP_ERROR("Read firmware message fail.");
            return ret;
        }

        update_msg.force_update = buf[0];
        if((0xBE != update_msg.force_update)&&(!retry))
        {
            GTP_INFO("The check sum in ic is error.");
            GTP_INFO("The IC will be updated by force.");
            continue;
        }
        break;
    }
    GTP_DEBUG("IC force update flag:0x%x", update_msg.force_update);

    // step3:get pid & vid
    ret = goodix_i2c_read_dbl_check(GUP_REG_PID_VID, &buf[0], 6);
    if (0 != ret)
    {
        GTP_ERROR("[get_ic_fw_msg]get pid & vid failed,exit");
        return FAIL;
    }

    memset(update_msg.ic_fw_msg.pid, 0, sizeof(update_msg.ic_fw_msg.pid));
    memcpy(update_msg.ic_fw_msg.pid, &buf[0], 4);
    GTP_DEBUG("IC Product id:%s", update_msg.ic_fw_msg.pid);

    //GT9XX PID MAPPING
    /*|-----FLASH-----RAM-----|
      |------918------918-----|
      |------968------968-----|
      |------913------913-----|
      |------913P-----913P----|
      |------927------927-----|
      |------927P-----927P----|
      |------9110-----9110----|
      |------9110P----9111----|*/
    if(update_msg.ic_fw_msg.pid[0] != 0)
    {
        if(!memcmp(update_msg.ic_fw_msg.pid, "9111", 4))
        {
            GTP_DEBUG("IC Mapping Product id:%s", update_msg.ic_fw_msg.pid);
            memcpy(update_msg.ic_fw_msg.pid, "9110P", 5);
        }
    }

    update_msg.ic_fw_msg.vid = buf[0+4] + (buf[0+5]<<8);
    GTP_DEBUG("IC version id:%04x", update_msg.ic_fw_msg.vid);
    return SUCCESS;
}

s32 gup_enter_update_mode(void)
{
    s32 ret = -1;
    s32 retry = 0;
    u8 rd_buf[3];

    int reset_gpio = goodix_ts->dev_data->ts_platform_data->reset_gpio;
    int irq_gpio = goodix_ts->dev_data->ts_platform_data->irq_gpio;

    //step1:RST output low last at least 2ms
    gpio_direction_output(reset_gpio, 0);
    msleep(20);

    //step2:select I2C slave addr,INT:0--0xBA;1--0x28.
    goodix_pinctr_int_ouput_low();
    msleep(20);

    //step3:RST output high reset guitar
    gpio_direction_output(reset_gpio, 1);

    //20121211 modify start
    msleep(20);
    while(retry++ < GUP_ENTER_UPDATE_TIMES)
    {
        //step4:Hold ss51 & dsp
        ret = gup_set_ic_msg(_rRW_MISCTL__SWRST_B0_, 0x0C);
        if(ret <= 0)
        {
            GTP_DEBUG("Hold ss51 & dsp I2C error,retry:%d", retry);
            continue;
        }

        //step5:Confirm hold
        ret = gup_get_ic_msg(_rRW_MISCTL__SWRST_B0_, rd_buf, 1);
        if(ret <= 0)
        {
            GTP_DEBUG("Hold ss51 & dsp I2C error,retry:%d", retry);
            continue;
        }
        if(0x0C == rd_buf[0])
        {
            GTP_DEBUG("Hold ss51 & dsp confirm SUCCESS");
            break;
        }
        GTP_DEBUG("Hold ss51 & dsp confirm 0x4180 failed,value:%d", rd_buf[0]);
    }
    if(retry >= GUP_ENTER_UPDATE_TIMES)
    {
        GTP_ERROR("Enter update Hold ss51 failed.");
        return FAIL;
    }

    //step6:DSP_CK and DSP_ALU_CK PowerOn
    ret = gup_set_ic_msg(0x4010, 0x00);

    //20121211 modify end
    return ret;
}

void gup_leave_update_mode(void)
{
	int irq_gpio = goodix_ts->dev_data->ts_platform_data->irq_gpio;
    gpio_direction_input(irq_gpio);

    GTP_DEBUG("[leave_update_mode]reset chip.");
    goodix_chip_reset();
}

// Get the correct nvram data
// The correct conditions:
//  1. the hardware info is the same
//  2. the product id is the same
//  3. the firmware version in update file is greater than the firmware version in ic
//      or the check sum in ic is wrong
/* Update Conditions:
    1. Same hardware info
    2. Same PID
    3. File VID > IC VID
   Force Update Conditions:
    1. Wrong ic firmware checksum
    2. INVALID IC PID or VID
    3. (IC PID == 91XX || File PID == 91XX) && (File VID > IC VID)
*/

static u8 gup_enter_update_judge(st_fw_head *fw_head)
{
    u16 u16_tmp;
    s32 i = 0;
    u32 fw_len = 0;
    u32 pid_cmp_len = 0;
    u16_tmp = fw_head->vid;
    fw_head->vid = (u16)(u16_tmp>>8) + (u16)(u16_tmp<<8);

    GTP_INFO("FILE HARDWARE INFO:%02x%02x%02x%02x", fw_head->hw_info[0], fw_head->hw_info[1], fw_head->hw_info[2], fw_head->hw_info[3]);
    GTP_INFO("FILE PID:%s", fw_head->pid);
    GTP_INFO("FILE VID:%04x", fw_head->vid);
    GTP_INFO("IC HARDWARE INFO:%02x%02x%02x%02x", update_msg.ic_fw_msg.hw_info[0], update_msg.ic_fw_msg.hw_info[1],
             update_msg.ic_fw_msg.hw_info[2], update_msg.ic_fw_msg.hw_info[3]);
    GTP_INFO("IC PID:%s", update_msg.ic_fw_msg.pid);
    GTP_INFO("IC VID:%04x", update_msg.ic_fw_msg.vid);

    if (!memcmp(fw_head->pid, "9158", 4) && !memcmp(update_msg.ic_fw_msg.pid, "915S", 4))
    {
        GTP_INFO("Update GT915S to GT9158 directly!");
        return SUCCESS;
    }
    //First two conditions
    if (!memcmp(fw_head->hw_info, update_msg.ic_fw_msg.hw_info, sizeof(update_msg.ic_fw_msg.hw_info)))
    {
        fw_len = 42 * 1024;
    }
    else
    {
        fw_len = fw_head->hw_info[3];
        fw_len += (((u32)fw_head->hw_info[2]) << 8);
        fw_len += (((u32)fw_head->hw_info[1]) << 16);
        fw_len += (((u32)fw_head->hw_info[0]) << 24);
    }
    if (update_msg.fw_total_len != fw_len)
    {
        GTP_ERROR("Inconsistent firmware size, Update aborted! Default size: %d(%dK), actual size: %d(%dK)", fw_len, fw_len/1024, update_msg.fw_total_len, update_msg.fw_total_len/1024);
        return FAIL;
    }
    GTP_INFO("Firmware length:%d(%dK)", update_msg.fw_total_len, update_msg.fw_total_len/1024);

    if (update_msg.force_update != 0xBE)
    {
        GTP_INFO("FW chksum error,need enter update.");
        return SUCCESS;
    }

    // 20130523 start
    if (strlen(update_msg.ic_fw_msg.pid) < 3)
    {
        GTP_INFO("Illegal IC pid, need enter update");
        return SUCCESS;
    }
    else
    {
        for (i = 0; i < 3; i++)
        {
            if ((update_msg.ic_fw_msg.pid[i] < 0x30) || (update_msg.ic_fw_msg.pid[i] > 0x39))
            {
                GTP_INFO("Illegal IC pid, out of bound, need enter update");
                return SUCCESS;
            }
        }
    }
    // 20130523 end

    pid_cmp_len = strlen(fw_head->pid);
    if (pid_cmp_len < strlen(update_msg.ic_fw_msg.pid))
    {
        pid_cmp_len = strlen(update_msg.ic_fw_msg.pid);
    }

    if ((!memcmp(fw_head->pid, update_msg.ic_fw_msg.pid, pid_cmp_len)) ||
            (!memcmp(update_msg.ic_fw_msg.pid, "91XX", 4))||
            (!memcmp(fw_head->pid, "91XX", 4)))
    {
        if(!memcmp(fw_head->pid, "91XX", 4))
        {
            GTP_DEBUG("Force none same pid update mode.");
        }
        else
        {
            GTP_DEBUG("Get the same pid.");
        }

        //The third condition
        if (fw_head->vid != update_msg.ic_fw_msg.vid)
        {
            GTP_INFO("Need enter update.");
            return SUCCESS;
        }
        GTP_ERROR("Don't meet the third condition.");
        GTP_ERROR("File VID <= Ic VID, update aborted!");

		return FW_NO_NEED_UPDATE;
    }
    else
    {
        GTP_ERROR("File PID != Ic PID, update aborted!");
    }

    return FAIL;
}



#if GTP_AUTO_UPDATE_CFG
static u8 ascii2hex(u8 a)
{
    s8 value = 0;

    if(a >= '0' && a <= '9')
    {
        value = a - '0';
    }
    else if(a >= 'A' && a <= 'F')
    {
        value = a - 'A' + 0x0A;
    }
    else if(a >= 'a' && a <= 'f')
    {
        value = a - 'a' + 0x0A;
    }
    else
    {
        value = 0xff;
    }

    return value;
}

static s8 gup_update_config(void)
{
    s32 file_len = 0;
    s32 ret = 0;
    s32 i = 0;
    s32 file_cfg_len = 0;
    s32 chip_cfg_len = 0;
    s32 count = 0;
    u8 *buf;
    u8 *pre_buf;
    u8 *file_config;
    //u8 checksum = 0;
    struct goodix_ts_config *normal_config;
    struct goodix_ts_data *ts = goodix_ts;
    normal_config = &goodix_ts->normal_config;

    if(NULL == update_msg.cfg_file)
    {
        GTP_ERROR("[update_cfg]No need to upgrade config!");
        return FAIL;
    }
    file_len = update_msg.cfg_file->f_op->llseek(update_msg.cfg_file, 0, SEEK_END);

    chip_cfg_len = normal_config->size;////////need check

    GTP_DEBUG("[update_cfg]config file len:%d", file_len);
    GTP_DEBUG("[update_cfg]need config len:%d", chip_cfg_len);
    if((file_len+5) < chip_cfg_len*5)
    {
        GTP_ERROR("Config length error");
        return -1;
    }

    buf = (u8*)kzalloc(file_len, GFP_KERNEL);
    pre_buf = (u8*)kzalloc(file_len, GFP_KERNEL);
    file_config = (u8*)kzalloc(chip_cfg_len, GFP_KERNEL);
	if (!buf || !pre_buf || !file_config) {
		TS_LOG_ERR("%s:[update_cfg]Alloc memory failed.\n",__func__);
		ret = -ENOMEM;
	}
    update_msg.cfg_file->f_op->llseek(update_msg.cfg_file, 0, SEEK_SET);

    GTP_DEBUG("[update_cfg]Read config from file.");
    ret = vfs_read(update_msg.cfg_file, pre_buf, file_len, &update_msg.cfg_file->f_pos);
    if(ret<0)
    {
        GTP_ERROR("[update_cfg]Read config file failed.");
        goto update_cfg_file_failed;
    }

    GTP_DEBUG("[update_cfg]Delete illgal charactor.");
    for(i=0,count=0; i<file_len; i++)
    {
        if (pre_buf[i] == ' ' || pre_buf[i] == '\r' || pre_buf[i] == '\n')
        {
            continue;
        }
        buf[count++] = pre_buf[i];
    }

    GTP_DEBUG("[update_cfg]Ascii to hex.");

    for(i=0,file_cfg_len=0; i<count; i+=5)
    {
        if((buf[i]=='0') && ((buf[i+1]=='x') || (buf[i+1]=='X')))
        {
            u8 high,low;
            high = ascii2hex(buf[i+2]);
            low = ascii2hex(buf[i+3]);

            if((high == 0xFF) || (low == 0xFF))
            {
                ret = 0;
                GTP_ERROR("[update_cfg]Illegal config file.");
                goto update_cfg_file_failed;
            }
            file_config[file_cfg_len++] = (high<<4) + low;
        }
        else
        {
            ret = 0;
            GTP_ERROR("[update_cfg]Illegal config file.");
            goto update_cfg_file_failed;
        }
    }

    GTP_DEBUG("config:");

    i = 0;
    while(i++ < 5)
    {
        ret = goodix_i2c_write(GTP_REG_CONFIG_DATA, file_config, file_cfg_len);
        if(ret > 0)
        {
            GTP_INFO("[update_cfg]Send config SUCCESS.");
            break;
        }
        GTP_ERROR("[update_cfg]Send config i2c error.");
    }

update_cfg_file_failed:
    kfree(pre_buf);
    kfree(buf);
    kfree(file_config);
    return ret;
}

#endif

static void gup_search_file(s32 search_type)
{
    s32 i = 0;
    struct file *pfile = NULL;

    got_file_flag = 0x00;
    searching_file = 1;
    for (i = 0; i < GUP_SEARCH_FILE_TIMES; ++i)
    {
        if (0 == searching_file)
        {
            GTP_INFO("Force exiting file searching");
            got_file_flag = 0x00;
            return;
        }

        if (search_type & AUTO_SEARCH_BIN)
        {
            GTP_DEBUG("Search for %s for fw update.(%d/%d)", GTP_FW_MANUAL_UPDATE_FILE_NAME, i+1, GUP_SEARCH_FILE_TIMES);
		pfile = filp_open(GTP_FW_MANUAL_UPDATE_FILE_NAME, O_RDONLY, 0);
		if (!IS_ERR(pfile))
		{
		    GTP_INFO("Bin file: %s for fw update.", GTP_FW_MANUAL_UPDATE_FILE_NAME);
		    got_file_flag |= BIN_FILE_READY;
		    update_msg.file = pfile;
		}
            if (got_file_flag & BIN_FILE_READY)
            {
            #if GTP_AUTO_UPDATE_CFG
                if (search_type & AUTO_SEARCH_CFG)
                {
                    i = GUP_SEARCH_FILE_TIMES;    // Bin & Cfg File required to be in the same directory
                }
                else
            #endif
                {
                    searching_file = 0;
                    return;
                }
            }
        }

    #if GTP_AUTO_UPDATE_CFG
        if ( (search_type & AUTO_SEARCH_CFG) && !(got_file_flag & CFG_FILE_READY) )
        {
            GTP_DEBUG("Search for %s, %s for config update.(%d/%d)", CONFIG_FILE_PATH_1, CONFIG_FILE_PATH_2, i+1, GUP_SEARCH_FILE_TIMES);
            pfile = filp_open(CONFIG_FILE_PATH_1, O_RDONLY, 0);
            if (IS_ERR(pfile))
            {
                pfile = filp_open(CONFIG_FILE_PATH_2, O_RDONLY, 0);
                if (!IS_ERR(pfile))
                {
                    GTP_INFO("Cfg file: %s for config update.", CONFIG_FILE_PATH_2);
                    got_file_flag |= CFG_FILE_READY;
                    update_msg.cfg_file = pfile;
                }
            }
            else
            {
                GTP_INFO("Cfg file: %s for config update.", CONFIG_FILE_PATH_1);
                got_file_flag |= CFG_FILE_READY;
                update_msg.cfg_file = pfile;
            }
            if (got_file_flag & CFG_FILE_READY)
            {
                searching_file = 0;
                return;
            }
        }
    #endif
        msleep(3000);
    }
    searching_file = 0;
}


static u8 gup_check_update_file(st_fw_head* fw_head)
{
    s32 ret = 0;
    u32 i = 0;
    u32 fw_checksum = 0;
    u8 buf[FW_HEAD_LENGTH];

    got_file_flag = 0x00;

    {

	if (update_msg.update_type == UPDATE_TYPE_HEADER) {
        GTP_INFO("Request firmware...");
		ret = request_firmware(&update_msg.fw, goodix_ts->firmware_name, &goodix_ts->pdev->dev);
		if (ret < 0) {
			GTP_ERROR("Request firmware failed - %s (%d)\n",
						goodix_ts->firmware_name, ret);
			ret = request_firmware(&update_msg.fw, GTP_FW_NAME, &goodix_ts->pdev->dev);
			if(ret < 0){
				TS_LOG_ERR("Request firmware failed - %s (%d)\n",
						GTP_FW_NAME, ret);
				return FW_NOT_EXIST;
			}
		}
		GTP_INFO("Firmware size: %d", update_msg.fw->size);
		if (update_msg.fw->size <
		     (FW_HEAD_LENGTH + FW_SECTION_LENGTH * 4 +
		     FW_DSP_ISP_LENGTH + FW_DSP_LENGTH + FW_BOOT_LENGTH)) {
			GTP_ERROR("INVALID firmware!");
			return FAIL;
		}
		update_msg.fw_total_len = update_msg.fw->size - FW_HEAD_LENGTH;

		GTP_DEBUG("Firmware actual size: %d(%dK)",
			  update_msg.fw_total_len,
			  update_msg.fw_total_len / 1024);
		memcpy(fw_head, &update_msg.fw->data[0], FW_HEAD_LENGTH);

		/*check firmware legality */
		fw_checksum = 0;
		for (i = 0; i < update_msg.fw_total_len; i += 2) {
			u32 index1 = FW_HEAD_LENGTH+i;
			u32 index2 = FW_HEAD_LENGTH+i+1;
			fw_checksum += 
			    (update_msg.fw->data[index1] << 8) + update_msg.fw->data[index2];
		}

		GTP_DEBUG("firmware checksum:%x", fw_checksum&0xFFFF);
		if (fw_checksum&0xFFFF)
		{
			GTP_ERROR("Illegal firmware file.");
			return FAIL;
		}
		got_file_flag = HEADER_FW_READY;
		return SUCCESS;
    }else{

	    #if GTP_AUTO_UPDATE_CFG
	        gup_search_file(AUTO_SEARCH_BIN | AUTO_SEARCH_CFG);
	        if (got_file_flag & CFG_FILE_READY)
	        {
	            ret = gup_update_config();
	            if(ret <= 0)
	            {
	                GTP_ERROR("Update config failed.");
	            }
	            _CLOSE_FILE(update_msg.cfg_file);
	            msleep(500);                //waiting config to be stored in FLASH.
	        }
	    #else
	        gup_search_file(AUTO_SEARCH_BIN);
	    #endif

	        if ( !(got_file_flag & BIN_FILE_READY) )
	        {
	            GTP_ERROR("No bin file for fw update");
	            return FAIL;
              }

        }

    }

    update_msg.old_fs = get_fs();
    set_fs(KERNEL_DS);

    update_msg.file->f_op->llseek(update_msg.file, 0, SEEK_SET);
    update_msg.fw_total_len = update_msg.file->f_op->llseek(update_msg.file, 0, SEEK_END);
    if (update_msg.fw_total_len < (FW_HEAD_LENGTH + FW_SECTION_LENGTH*4+FW_DSP_ISP_LENGTH+FW_DSP_LENGTH+FW_BOOT_LENGTH))
    {
		set_fs(update_msg.old_fs);
		GTP_ERROR("INVALID bin file(size: %d), update aborted.", update_msg.fw_total_len);
		return FAIL;
    }

    update_msg.fw_total_len -= FW_HEAD_LENGTH;
    GTP_DEBUG("Bin firmware actual size: %d(%dK)", update_msg.fw_total_len, update_msg.fw_total_len/1024);

    update_msg.file->f_op->llseek(update_msg.file, 0, SEEK_SET);
    ret = vfs_read(update_msg.file, buf, FW_HEAD_LENGTH, &update_msg.file->f_pos);
    if (ret < 0)
    {
		set_fs(update_msg.old_fs);
		GTP_ERROR("Read firmware head in update file error.");
		return FAIL;
    }

    memcpy(fw_head, buf, FW_HEAD_LENGTH);

    //check firmware legality
    fw_checksum = 0;
    for(i=0; i<update_msg.fw_total_len; i+=2)
    {
        u16 temp;
        ret = vfs_read(update_msg.file, buf, 2, &update_msg.file->f_pos);
        if (ret < 0)
        {
			set_fs(update_msg.old_fs);
			GTP_ERROR("Read firmware file error.");
			return FAIL;
        }
        //GTP_DEBUG("BUF[0]:%x", buf[0]);
        temp = (buf[0]<<8) + buf[1];
        fw_checksum += temp;
    }

    GTP_DEBUG("firmware checksum:%x", fw_checksum&0xFFFF);
    if(fw_checksum&0xFFFF)
    {
		set_fs(update_msg.old_fs);
		GTP_ERROR("Illegal firmware file.");
		return FAIL;
    }
	set_fs(update_msg.old_fs);
	return SUCCESS;
}

static u8 gup_burn_proc(u8 *burn_buf, u16 start_addr, u16 total_length)
{
    s32 ret = 0;
    u16 burn_addr = start_addr;
    u16 frame_length = 0;
    u16 burn_length = 0;
    u8  wr_buf[PACK_SIZE + GTP_ADDR_LENGTH];
    u8  rd_buf[PACK_SIZE + GTP_ADDR_LENGTH];
    u8  retry = 0;

    GTP_DEBUG("Begin burn %dk data to addr 0x%x", (total_length/1024), start_addr);
    while(burn_length < total_length)
    {
        GTP_DEBUG("B/T:%04d/%04d", burn_length, total_length);
        frame_length = ((total_length - burn_length) > PACK_SIZE) ? PACK_SIZE : (total_length - burn_length);
        wr_buf[0] = (u8)(burn_addr>>8);
        rd_buf[0] = wr_buf[0];
        wr_buf[1] = (u8)burn_addr;
        rd_buf[1] = wr_buf[1];
        memcpy(&wr_buf[GTP_ADDR_LENGTH], &burn_buf[burn_length], frame_length);

        for(retry = 0; retry < MAX_FRAME_CHECK_TIME; retry++)
        {
            ret = gup_i2c_write(wr_buf, GTP_ADDR_LENGTH + frame_length);
            if(ret <= 0)
            {
                GTP_ERROR("Write frame data i2c error.");
                continue;
            }
            ret = gup_i2c_read(rd_buf, GTP_ADDR_LENGTH + frame_length);
            if(ret <= 0)
            {
                GTP_ERROR("Read back frame data i2c error.");
                continue;
            }

            if(memcmp(&wr_buf[GTP_ADDR_LENGTH], &rd_buf[GTP_ADDR_LENGTH], frame_length))
            {
                GTP_ERROR("Check frame data fail,not equal.");
                GTP_DEBUG("write array:");
                GTP_DEBUG_ARRAY(&wr_buf[GTP_ADDR_LENGTH], frame_length);
                GTP_DEBUG("read array:");
                GTP_DEBUG_ARRAY(&rd_buf[GTP_ADDR_LENGTH], frame_length);
                continue;
            }
            else
            {
                //GTP_DEBUG("Check frame data success.");
                break;
            }
        }
        if(retry >= MAX_FRAME_CHECK_TIME)
        {
            GTP_ERROR("Burn frame data time out,exit.");
            return FAIL;
        }
        burn_length += frame_length;
        burn_addr += frame_length;
    }
    return SUCCESS;
}

static u8 gup_load_section_file(u8 *buf, u32 offset, u16 length, u8 set_or_end)
{
	u32 index = 0;
	if(update_msg.update_type == UPDATE_TYPE_HEADER){
		if (got_file_flag == HEADER_FW_READY)
		{
			if(SEEK_SET == set_or_end)
			{
				index = FW_HEAD_LENGTH + offset;
				memcpy(buf, &update_msg.fw->data[index], length);
			}
			else    /*seek end*/
			{
				index = update_msg.fw_total_len + FW_HEAD_LENGTH - offset;
				memcpy(buf, &update_msg.fw->data[index], length);
			}
			return SUCCESS;
		}
	}
	{
        s32 ret = 0;

        if ( (update_msg.file == NULL) || IS_ERR(update_msg.file))
        {
            GTP_ERROR("cannot find update file,load section file fail.");
            return FAIL;
        }

        if(SEEK_SET == set_or_end)
        {
            update_msg.file->f_pos = FW_HEAD_LENGTH + offset;
        }
        else    //seek end
        {
            update_msg.file->f_pos = update_msg.fw_total_len + FW_HEAD_LENGTH - offset;
        }

        ret = vfs_read(update_msg.file, buf, length, &update_msg.file->f_pos);

        if (ret < 0)
        {
            GTP_ERROR("Read update file fail.");
            return FAIL;
        }

        return SUCCESS;
	}
}

static u8 gup_recall_check(u8* chk_src, u16 start_rd_addr, u16 chk_length)
{
    u8  rd_buf[PACK_SIZE + GTP_ADDR_LENGTH];
    s32 ret = 0;
    u16 recall_addr = start_rd_addr;
    u16 recall_length = 0;
    u16 frame_length = 0;
	u8 hold_buf[4];

    while(recall_length < chk_length)
    {
        frame_length = ((chk_length - recall_length) > PACK_SIZE) ? PACK_SIZE : (chk_length - recall_length);
        ret = gup_get_ic_msg(recall_addr, rd_buf, frame_length);
        if(ret <= 0)
        {
            GTP_ERROR("recall i2c error,exit");
            return FAIL;
        }
#if 1
ret = gup_get_ic_msg(0x4244, hold_buf, 4);
GTP_INFO("Hold ss51 & dsp confirm SUCCESS 0x%x 0x%x 0x%x 0x%x,,",hold_buf[0],hold_buf[1],hold_buf[2],hold_buf[3]);
#endif

        if(memcmp(&rd_buf[0], &chk_src[recall_length], frame_length))
        {
            GTP_ERROR("Recall frame data fail,not equal.");
            GTP_DEBUG("chk_src array:");
            GTP_DEBUG_ARRAY(&chk_src[recall_length], frame_length);
            GTP_DEBUG("recall array:");
            GTP_DEBUG_ARRAY(&rd_buf[0], frame_length);
            return FAIL;
        }

        recall_length += frame_length;
        recall_addr += frame_length;
    }
    GTP_DEBUG("Recall check %dk firmware success.", (chk_length/1024));

    return SUCCESS;
}

static u8 gup_burn_fw_section(u8 *fw_section, u16 start_addr, u8 bank_cmd )
{
    s32 ret = 0;
    u8  rd_buf[5];
    u8  hold_buf[1];
    int hold_time=0;

    //step1:hold ss51 & dsp
    /*
    ret = gup_set_ic_msg(_rRW_MISCTL__SWRST_B0_, 0x0C);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_section]hold ss51 & dsp fail.");
        return FAIL;
    }
    */
#if 1
       while(hold_time++ < 200)
    {
        //step4:Hold ss51 & dsp
        ret = gup_set_ic_msg(_rRW_MISCTL__SWRST_B0_, 0x0C);
        if(ret <= 0)
        {
            GTP_DEBUG("Hold ss51 & dsp I2C error,retry:%d", hold_time);
            continue;
        }

        //step5:Confirm hold
        ret = gup_get_ic_msg(_rRW_MISCTL__SWRST_B0_, hold_buf, 1);

	 GTP_INFO("0000000000000Hold ss51 & dsp confirm SUCCESS 0x%x",hold_buf[0]);
        if(ret <= 0)
        {
            GTP_DEBUG("Hold ss51 & dsp I2C error,retry:%d", hold_time);
            continue;
        }
        if(0x0C == hold_buf[0])
        {
            GTP_DEBUG("Hold ss51 & dsp confirm SUCCESS");
            break;
        }
        GTP_DEBUG("Hold ss51 & dsp confirm 0x4180 failed,value:%d", hold_buf[0]);
    }
    if(hold_time >= 200)
    {
        GTP_ERROR("Enter update Hold ss51 failed.");
        return FAIL;
    }
#endif

    //step2:set scramble
    ret = gup_set_ic_msg(_rRW_MISCTL__BOOT_OPT_B0_, 0x00);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_section]set scramble fail.");
        return FAIL;
    }

    //step3:select bank
    ret = gup_set_ic_msg(_bRW_MISCTL__SRAM_BANK, (bank_cmd >> 4)&0x0F);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_section]select bank %d fail.", (bank_cmd >> 4)&0x0F);
        return FAIL;
    }

    //step4:enable accessing code
    ret = gup_set_ic_msg(_bRW_MISCTL__MEM_CD_EN, 0x01);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_section]enable accessing code fail.");
        return FAIL;
    }

    //step5:burn 8k fw section
    ret = gup_burn_proc(fw_section, start_addr, FW_SECTION_LENGTH);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_section]burn fw_section fail.");
        return FAIL;
    }

    //step6:hold ss51 & release dsp
    ret = gup_set_ic_msg(_rRW_MISCTL__SWRST_B0_, 0x04);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_section]hold ss51 & release dsp fail.");
        return FAIL;
    }
    //must delay
    msleep(1);

    //step7:send burn cmd to move data to flash from sram
    ret = gup_set_ic_msg(_rRW_MISCTL__BOOT_CTL_, bank_cmd&0x0f);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_section]send burn cmd fail.");
        return FAIL;
    }
    GTP_DEBUG("[burn_fw_section]Wait for the burn is complete......");
    do{
        ret = gup_get_ic_msg(_rRW_MISCTL__BOOT_CTL_, rd_buf, 1);
        if(ret <= 0)
        {
            GTP_ERROR("[burn_fw_section]Get burn state fail");
            return FAIL;
        }
        msleep(10);
        //GTP_DEBUG("[burn_fw_section]Get burn state:%d.", rd_buf[GTP_ADDR_LENGTH]);
    }while(rd_buf[0]);

    //step8:select bank
    ret = gup_set_ic_msg(_bRW_MISCTL__SRAM_BANK, (bank_cmd >> 4)&0x0F);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_section]select bank %d fail.", (bank_cmd >> 4)&0x0F);
        return FAIL;
    }

    //step9:enable accessing code
    ret = gup_set_ic_msg(_bRW_MISCTL__MEM_CD_EN, 0x01);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_section]enable accessing code fail.");
        return FAIL;
    }

    //step10:recall 8k fw section
    ret = gup_recall_check(fw_section, start_addr, FW_SECTION_LENGTH);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_section]recall check %dk firmware fail.", FW_SECTION_LENGTH/1024);
        return FAIL;
    }

    //step11:disable accessing code
    ret = gup_set_ic_msg(_bRW_MISCTL__MEM_CD_EN, 0x00);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_section]disable accessing code fail.");
        return FAIL;
    }

    return SUCCESS;
}

static u8 gup_burn_dsp_isp(void)
{
    s32 ret = 0;
    u8* fw_dsp_isp = NULL;
    u8  retry = 0;

    GTP_INFO("[burn_dsp_isp]Begin burn dsp isp---->>");

    //step1:alloc memory
    GTP_DEBUG("[burn_dsp_isp]step1:alloc memory");
    while(retry++ < 5)
    {
        fw_dsp_isp = (u8*)kzalloc(FW_DSP_ISP_LENGTH, GFP_KERNEL);
        if(fw_dsp_isp == NULL)
        {
            continue;
        }
        else
        {
            GTP_INFO("[burn_dsp_isp]Alloc %dk byte memory success.", (FW_DSP_ISP_LENGTH/1024));
            break;
        }
    }
    if(retry >= 5)
    {
        GTP_ERROR("[burn_dsp_isp]Alloc memory fail,exit.");
        return FAIL;
    }

    //step2:load dsp isp file data
    GTP_DEBUG("[burn_dsp_isp]step2:load dsp isp file data");
    ret = gup_load_section_file(fw_dsp_isp, FW_DSP_ISP_LENGTH, FW_DSP_ISP_LENGTH, SEEK_END);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_dsp_isp]load firmware dsp_isp fail.");
        goto exit_burn_dsp_isp;
    }

    //step3:disable wdt,clear cache enable
    GTP_DEBUG("[burn_dsp_isp]step3:disable wdt,clear cache enable");
    ret = gup_set_ic_msg(_bRW_MISCTL__TMR0_EN, 0x00);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_dsp_isp]disable wdt fail.");
        ret = FAIL;
        goto exit_burn_dsp_isp;
    }
    ret = gup_set_ic_msg(_bRW_MISCTL__CACHE_EN, 0x00);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_dsp_isp]clear cache enable fail.");
        ret = FAIL;
        goto exit_burn_dsp_isp;
    }

    //step4:hold ss51 & dsp
    GTP_DEBUG("[burn_dsp_isp]step4:hold ss51 & dsp");
    ret = gup_set_ic_msg(_rRW_MISCTL__SWRST_B0_, 0x0C);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_dsp_isp]hold ss51 & dsp fail.");
        ret = FAIL;
        goto exit_burn_dsp_isp;
    }

    //step5:set boot from sram
    GTP_DEBUG("[burn_dsp_isp]step5:set boot from sram");
    ret = gup_set_ic_msg(_rRW_MISCTL__BOOTCTL_B0_, 0x02);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_dsp_isp]set boot from sram fail.");
        ret = FAIL;
        goto exit_burn_dsp_isp;
    }

    //step6:software reboot
    GTP_DEBUG("[burn_dsp_isp]step6:software reboot");
    ret = gup_set_ic_msg(_bWO_MISCTL__CPU_SWRST_PULSE, 0x01);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_dsp_isp]software reboot fail.");
        ret = FAIL;
        goto exit_burn_dsp_isp;
    }

    //step7:select bank2
    GTP_DEBUG("[burn_dsp_isp]step7:select bank2");
    ret = gup_set_ic_msg(_bRW_MISCTL__SRAM_BANK, 0x02);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_dsp_isp]select bank2 fail.");
        ret = FAIL;
        goto exit_burn_dsp_isp;
    }

    //step8:enable accessing code
    GTP_DEBUG("[burn_dsp_isp]step8:enable accessing code");
    ret = gup_set_ic_msg(_bRW_MISCTL__MEM_CD_EN, 0x01);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_dsp_isp]enable accessing code fail.");
        ret = FAIL;
        goto exit_burn_dsp_isp;
    }

    //step9:burn 4k dsp_isp
    GTP_DEBUG("[burn_dsp_isp]step9:burn 4k dsp_isp");
    ret = gup_burn_proc(fw_dsp_isp, 0xC000, FW_DSP_ISP_LENGTH);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_dsp_isp]burn dsp_isp fail.");
        goto exit_burn_dsp_isp;
    }

    //step10:set scramble
    GTP_DEBUG("[burn_dsp_isp]step10:set scramble");
    ret = gup_set_ic_msg(_rRW_MISCTL__BOOT_OPT_B0_, 0x00);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_dsp_isp]set scramble fail.");
        ret = FAIL;
        goto exit_burn_dsp_isp;
    }
    update_msg.fw_burned_len += FW_DSP_ISP_LENGTH;
    GTP_DEBUG("[burn_dsp_isp]Burned length:%d", update_msg.fw_burned_len);
    ret = SUCCESS;

exit_burn_dsp_isp:
    kfree(fw_dsp_isp);
    return ret;
}

static u8 gup_burn_fw_ss51(void)
{
    u8* fw_ss51 = NULL;
    u8  retry = 0;
    s32 ret = 0;

    GTP_INFO("[burn_fw_ss51]Begin burn ss51 firmware---->>");

    //step1:alloc memory
    GTP_DEBUG("[burn_fw_ss51]step1:alloc memory");
    while(retry++ < 5)
    {
        fw_ss51 = (u8*)kzalloc(FW_SECTION_LENGTH, GFP_KERNEL);
        if(fw_ss51 == NULL)
        {
            continue;
        }
        else
        {
            GTP_DEBUG("[burn_fw_ss51]Alloc %dk byte memory success.", (FW_SECTION_LENGTH / 1024));
            break;
        }
    }
    if(retry >= 5)
    {
        GTP_ERROR("[burn_fw_ss51]Alloc memory fail,exit.");
        return FAIL;
    }

    //step2:load ss51 firmware section 1 file data
//    GTP_DEBUG("[burn_fw_ss51]step2:load ss51 firmware section 1 file data");
//    ret = gup_load_section_file(fw_ss51, 0, FW_SECTION_LENGTH, SEEK_SET);
//    if(FAIL == ret)
//    {
//        GTP_ERROR("[burn_fw_ss51]load ss51 firmware section 1 fail.");
//        goto exit_burn_fw_ss51;
//    }
    if(fw_ss51 == NULL)
		return FAIL;
    GTP_INFO("[burn_fw_ss51]Reset first 8K of ss51 to 0xFF.");
    GTP_DEBUG("[burn_fw_ss51]step2: reset bank0 0xC000~0xD000");
    memset(fw_ss51, 0xFF, FW_SECTION_LENGTH);

    //step3:clear control flag
    GTP_DEBUG("[burn_fw_ss51]step3:clear control flag");
    ret = gup_set_ic_msg(_rRW_MISCTL__BOOT_CTL_, 0x00);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_ss51]clear control flag fail.");
        ret = FAIL;
        goto exit_burn_fw_ss51;
    }

    //step4:burn ss51 firmware section 1
    GTP_DEBUG("[burn_fw_ss51]step4:burn ss51 firmware section 1");
    ret = gup_burn_fw_section(fw_ss51, 0xC000, 0x01);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_ss51]burn ss51 firmware section 1 fail.");
        goto exit_burn_fw_ss51;
    }

    //step5:load ss51 firmware section 2 file data
    GTP_DEBUG("[burn_fw_ss51]step5:load ss51 firmware section 2 file data");
    ret = gup_load_section_file(fw_ss51, FW_SECTION_LENGTH, FW_SECTION_LENGTH, SEEK_SET);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_ss51]load ss51 firmware section 2 fail.");
        goto exit_burn_fw_ss51;
    }

    //step6:burn ss51 firmware section 2
    GTP_DEBUG("[burn_fw_ss51]step6:burn ss51 firmware section 2");
    ret = gup_burn_fw_section(fw_ss51, 0xE000, 0x02);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_ss51]burn ss51 firmware section 2 fail.");
        goto exit_burn_fw_ss51;
    }

    //step7:load ss51 firmware section 3 file data
    GTP_DEBUG("[burn_fw_ss51]step7:load ss51 firmware section 3 file data");
    ret = gup_load_section_file(fw_ss51, 2 * FW_SECTION_LENGTH, FW_SECTION_LENGTH, SEEK_SET);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_ss51]load ss51 firmware section 3 fail.");
        goto exit_burn_fw_ss51;
    }

    //step8:burn ss51 firmware section 3
    GTP_DEBUG("[burn_fw_ss51]step8:burn ss51 firmware section 3");
    ret = gup_burn_fw_section(fw_ss51, 0xC000, 0x13);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_ss51]burn ss51 firmware section 3 fail.");
        goto exit_burn_fw_ss51;
    }

    //step9:load ss51 firmware section 4 file data
    GTP_DEBUG("[burn_fw_ss51]step9:load ss51 firmware section 4 file data");
    ret = gup_load_section_file(fw_ss51, 3 * FW_SECTION_LENGTH, FW_SECTION_LENGTH, SEEK_SET);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_ss51]load ss51 firmware section 4 fail.");
        goto exit_burn_fw_ss51;
    }

    //step10:burn ss51 firmware section 4
    GTP_DEBUG("[burn_fw_ss51]step10:burn ss51 firmware section 4");
    ret = gup_burn_fw_section(fw_ss51, 0xE000, 0x14);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_ss51]burn ss51 firmware section 4 fail.");
        goto exit_burn_fw_ss51;
    }

    update_msg.fw_burned_len += (FW_SECTION_LENGTH*4);
    GTP_DEBUG("[burn_fw_ss51]Burned length:%d", update_msg.fw_burned_len);
    ret = SUCCESS;

exit_burn_fw_ss51:
    kfree(fw_ss51);
    return ret;
}

static u8 gup_burn_fw_dsp(void)
{
    s32 ret = 0;
    u8* fw_dsp = NULL;
    u8  retry = 0;
    u8  rd_buf[5];

    GTP_INFO("[burn_fw_dsp]Begin burn dsp firmware---->>");
    //step1:alloc memory
    GTP_DEBUG("[burn_fw_dsp]step1:alloc memory");
    while(retry++ < 5)
    {
        fw_dsp = (u8*)kzalloc(FW_DSP_LENGTH, GFP_KERNEL);
        if(fw_dsp == NULL)
        {
            continue;
        }
        else
        {
            GTP_DEBUG("[burn_fw_dsp]Alloc %dk byte memory success.", (FW_SECTION_LENGTH / 1024));
            break;
        }
    }
    if(retry >= 5)
    {
        GTP_ERROR("[burn_fw_dsp]Alloc memory fail,exit.");
        return FAIL;
    }

    //step2:load firmware dsp
    GTP_DEBUG("[burn_fw_dsp]step2:load firmware dsp");
    ret = gup_load_section_file(fw_dsp, 4 * FW_SECTION_LENGTH, FW_DSP_LENGTH, SEEK_SET);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_dsp]load firmware dsp fail.");
        goto exit_burn_fw_dsp;
    }

    //step3:select bank3
    GTP_DEBUG("[burn_fw_dsp]step3:select bank3");
    ret = gup_set_ic_msg(_bRW_MISCTL__SRAM_BANK, 0x03);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_dsp]select bank3 fail.");
        ret = FAIL;
        goto exit_burn_fw_dsp;
    }

    //step4:hold ss51 & dsp
    GTP_DEBUG("[burn_fw_dsp]step4:hold ss51 & dsp");
    ret = gup_set_ic_msg(_rRW_MISCTL__SWRST_B0_, 0x0C);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_dsp]hold ss51 & dsp fail.");
        ret = FAIL;
        goto exit_burn_fw_dsp;
    }

    //step5:set scramble
    GTP_DEBUG("[burn_fw_dsp]step5:set scramble");
    ret = gup_set_ic_msg(_rRW_MISCTL__BOOT_OPT_B0_, 0x00);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_dsp]set scramble fail.");
        ret = FAIL;
        goto exit_burn_fw_dsp;
    }

    //step6:release ss51 & dsp
    GTP_DEBUG("[burn_fw_dsp]step6:release ss51 & dsp");
    ret = gup_set_ic_msg(_rRW_MISCTL__SWRST_B0_, 0x04);                 //20121211
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_dsp]release ss51 & dsp fail.");
        ret = FAIL;
        goto exit_burn_fw_dsp;
    }
    //must delay
    msleep(1);

    //step7:burn 4k dsp firmware
    GTP_DEBUG("[burn_fw_dsp]step7:burn 4k dsp firmware");
    ret = gup_burn_proc(fw_dsp, 0x9000, FW_DSP_LENGTH);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_dsp]burn fw_section fail.");
        goto exit_burn_fw_dsp;
    }

    //step8:send burn cmd to move data to flash from sram
    GTP_DEBUG("[burn_fw_dsp]step8:send burn cmd to move data to flash from sram");
    ret = gup_set_ic_msg(_rRW_MISCTL__BOOT_CTL_, 0x05);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_dsp]send burn cmd fail.");
        goto exit_burn_fw_dsp;
    }
    GTP_DEBUG("[burn_fw_dsp]Wait for the burn is complete......");
    do{
        ret = gup_get_ic_msg(_rRW_MISCTL__BOOT_CTL_, rd_buf, 1);
        if(ret <= 0)
        {
            GTP_ERROR("[burn_fw_dsp]Get burn state fail");
            goto exit_burn_fw_dsp;
        }
        msleep(10);
        //GTP_DEBUG("[burn_fw_dsp]Get burn state:%d.", rd_buf[GTP_ADDR_LENGTH]);
    }while(rd_buf[0]);

    //step9:recall check 4k dsp firmware
    GTP_DEBUG("[burn_fw_dsp]step9:recall check 4k dsp firmware");
    ret = gup_recall_check(fw_dsp, 0x9000, FW_DSP_LENGTH);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_dsp]recall check 4k dsp firmware fail.");
        goto exit_burn_fw_dsp;
    }

    update_msg.fw_burned_len += FW_DSP_LENGTH;
    GTP_DEBUG("[burn_fw_dsp]Burned length:%d", update_msg.fw_burned_len);
    ret = SUCCESS;

exit_burn_fw_dsp:
    kfree(fw_dsp);
    return ret;
}

static u8 gup_burn_fw_boot(void)
{
    s32 ret = 0;
    u8* fw_boot = NULL;
    u8  retry = 0;
    u8  rd_buf[5];

    GTP_INFO("[burn_fw_boot]Begin burn bootloader firmware---->>");

    //step1:Alloc memory
    GTP_DEBUG("[burn_fw_boot]step1:Alloc memory");
    while(retry++ < 5)
    {
        fw_boot = (u8*)kzalloc(FW_BOOT_LENGTH, GFP_KERNEL);
        if(fw_boot == NULL)
        {
            continue;
        }
        else
        {
            GTP_DEBUG("[burn_fw_boot]Alloc %dk byte memory success.", (FW_BOOT_LENGTH/1024));
            break;
        }
    }
    if(retry >= 5)
    {
        GTP_ERROR("[burn_fw_boot]Alloc memory fail,exit.");
        return FAIL;
    }

    //step2:load firmware bootloader
    GTP_DEBUG("[burn_fw_boot]step2:load firmware bootloader");
    ret = gup_load_section_file(fw_boot, (4 * FW_SECTION_LENGTH + FW_DSP_LENGTH), FW_BOOT_LENGTH, SEEK_SET);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_boot]load firmware bootcode fail.");
        goto exit_burn_fw_boot;
    }

    //step3:hold ss51 & dsp
    GTP_DEBUG("[burn_fw_boot]step3:hold ss51 & dsp");
    ret = gup_set_ic_msg(_rRW_MISCTL__SWRST_B0_, 0x0C);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_boot]hold ss51 & dsp fail.");
        ret = FAIL;
        goto exit_burn_fw_boot;
    }

    //step4:set scramble
    GTP_DEBUG("[burn_fw_boot]step4:set scramble");
    ret = gup_set_ic_msg(_rRW_MISCTL__BOOT_OPT_B0_, 0x00);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_boot]set scramble fail.");
        ret = FAIL;
        goto exit_burn_fw_boot;
    }

    //step5:hold ss51 & release dsp
    GTP_DEBUG("[burn_fw_boot]step5:hold ss51 & release dsp");
    ret = gup_set_ic_msg(_rRW_MISCTL__SWRST_B0_, 0x04);                 //20121211
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_boot]release ss51 & dsp fail.");
        ret = FAIL;
        goto exit_burn_fw_boot;
    }
    //must delay
    msleep(1);

    //step6:select bank3
    GTP_DEBUG("[burn_fw_boot]step6:select bank3");
    ret = gup_set_ic_msg(_bRW_MISCTL__SRAM_BANK, 0x03);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_boot]select bank3 fail.");
        ret = FAIL;
        goto exit_burn_fw_boot;
    }

    //step6:burn 2k bootloader firmware
    GTP_DEBUG("[burn_fw_boot]step6:burn 2k bootloader firmware");
    ret = gup_burn_proc(fw_boot, 0x9000, FW_BOOT_LENGTH);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_boot]burn fw_boot fail.");
        goto exit_burn_fw_boot;
    }

    //step7:send burn cmd to move data to flash from sram
    GTP_DEBUG("[burn_fw_boot]step7:send burn cmd to move data to flash from sram");
    ret = gup_set_ic_msg(_rRW_MISCTL__BOOT_CTL_, 0x06);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_boot]send burn cmd fail.");
        goto exit_burn_fw_boot;
    }
    GTP_DEBUG("[burn_fw_boot]Wait for the burn is complete......");
    do{
        ret = gup_get_ic_msg(_rRW_MISCTL__BOOT_CTL_, rd_buf, 1);
        if(ret <= 0)
        {
            GTP_ERROR("[burn_fw_boot]Get burn state fail");
            goto exit_burn_fw_boot;
        }
        msleep(10);
        //GTP_DEBUG("[burn_fw_boot]Get burn state:%d.", rd_buf[GTP_ADDR_LENGTH]);
    }while(rd_buf[0]);
    
    //step8:recall check 2k bootloader firmware
    GTP_DEBUG("[burn_fw_boot]step8:recall check 2k bootloader firmware");
    ret = gup_recall_check(fw_boot, 0x9000, FW_BOOT_LENGTH);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_boot]recall check 2k bootcode firmware fail.");
        goto exit_burn_fw_boot;
    }

    update_msg.fw_burned_len += FW_BOOT_LENGTH;
    GTP_DEBUG("[burn_fw_boot]Burned length:%d", update_msg.fw_burned_len);
    ret = SUCCESS;

exit_burn_fw_boot:
    kfree(fw_boot);
    return ret;
}
static u8 gup_burn_fw_boot_isp(void)
{
    s32 ret = 0;
    u8* fw_boot_isp = NULL;
    u8  retry = 0;
    u8  rd_buf[5];

    if(update_msg.fw_burned_len >= update_msg.fw_total_len)
    {
        GTP_DEBUG("No need to upgrade the boot_isp code!");
        return SUCCESS;
    }
    GTP_INFO("[burn_fw_boot_isp]Begin burn boot_isp firmware---->>");

    //step1:Alloc memory
    GTP_DEBUG("[burn_fw_boot_isp]step1:Alloc memory");
    while(retry++ < 5)
    {
        fw_boot_isp = (u8*)kzalloc(FW_BOOT_ISP_LENGTH, GFP_KERNEL);
        if(fw_boot_isp == NULL)
        {
            continue;
        }
        else
        {
            GTP_DEBUG("[burn_fw_boot_isp]Alloc %dk byte memory success.", (FW_BOOT_ISP_LENGTH/1024));
            break;
        }
    }
    if(retry >= 5)
    {
        GTP_ERROR("[burn_fw_boot_isp]Alloc memory fail,exit.");
        return FAIL;
    }

    //step2:load firmware bootloader
    GTP_DEBUG("[burn_fw_boot_isp]step2:load firmware bootloader isp");
    //ret = gup_load_section_file(fw_boot_isp, (4*FW_SECTION_LENGTH+FW_DSP_LENGTH+FW_BOOT_LENGTH+FW_DSP_ISP_LENGTH), FW_BOOT_ISP_LENGTH, SEEK_SET);
    ret = gup_load_section_file(fw_boot_isp, (update_msg.fw_burned_len - FW_DSP_ISP_LENGTH), FW_BOOT_ISP_LENGTH, SEEK_SET);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_boot_isp]load firmware boot_isp fail.");
        goto exit_burn_fw_boot_isp;
    }

    //step3:hold ss51 & dsp
    GTP_DEBUG("[burn_fw_boot_isp]step3:hold ss51 & dsp");
    ret = gup_set_ic_msg(_rRW_MISCTL__SWRST_B0_, 0x0C);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_boot_isp]hold ss51 & dsp fail.");
        ret = FAIL;
        goto exit_burn_fw_boot_isp;
    }

    //step4:set scramble
    GTP_DEBUG("[burn_fw_boot_isp]step4:set scramble");
    ret = gup_set_ic_msg(_rRW_MISCTL__BOOT_OPT_B0_, 0x00);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_boot_isp]set scramble fail.");
        ret = FAIL;
        goto exit_burn_fw_boot_isp;
    }

    //step5:hold ss51 & release dsp
    GTP_DEBUG("[burn_fw_boot_isp]step5:hold ss51 & release dsp");
    ret = gup_set_ic_msg(_rRW_MISCTL__SWRST_B0_, 0x04);                 //20121211
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_boot_isp]release ss51 & dsp fail.");
        ret = FAIL;
        goto exit_burn_fw_boot_isp;
    }
    //must delay
    msleep(1);

    //step6:select bank3
    GTP_DEBUG("[burn_fw_boot_isp]step6:select bank3");
    ret = gup_set_ic_msg(_bRW_MISCTL__SRAM_BANK, 0x03);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_boot_isp]select bank3 fail.");
        ret = FAIL;
        goto exit_burn_fw_boot_isp;
    }

    //step7:burn 2k bootload_isp firmware
    GTP_DEBUG("[burn_fw_boot_isp]step7:burn 2k bootloader firmware");
    ret = gup_burn_proc(fw_boot_isp, 0x9000, FW_BOOT_ISP_LENGTH);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_boot_isp]burn fw_section fail.");
        goto exit_burn_fw_boot_isp;
    }

    //step7:send burn cmd to move data to flash from sram
    GTP_DEBUG("[burn_fw_boot_isp]step8:send burn cmd to move data to flash from sram");
    ret = gup_set_ic_msg(_rRW_MISCTL__BOOT_CTL_, 0x07);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_boot_isp]send burn cmd fail.");
        goto exit_burn_fw_boot_isp;
    }
    GTP_DEBUG("[burn_fw_boot_isp]Wait for the burn is complete......");
    do{
        ret = gup_get_ic_msg(_rRW_MISCTL__BOOT_CTL_, rd_buf, 1);
        if(ret <= 0)
        {
            GTP_ERROR("[burn_fw_boot_isp]Get burn state fail");
            goto exit_burn_fw_boot_isp;
        }
        msleep(10);
        //GTP_DEBUG("[burn_fw_boot_isp]Get burn state:%d.", rd_buf[GTP_ADDR_LENGTH]);
    }while(rd_buf[0]);

    //step8:recall check 2k bootload_isp firmware
    GTP_DEBUG("[burn_fw_boot_isp]step9:recall check 2k bootloader firmware");
    ret = gup_recall_check(fw_boot_isp, 0x9000, FW_BOOT_ISP_LENGTH);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_boot_isp]recall check 2k bootcode_isp firmware fail.");
        goto exit_burn_fw_boot_isp;
    }

    update_msg.fw_burned_len += FW_BOOT_ISP_LENGTH;
    GTP_DEBUG("[burn_fw_boot_isp]Burned length:%d", update_msg.fw_burned_len);
    ret = SUCCESS;

exit_burn_fw_boot_isp:
    kfree(fw_boot_isp);
    return ret;
}

static u8 gup_burn_fw_link(void)
{
    s32 ret = 0;
    u8* fw_link = NULL;
    u8  retry = 0;
    u32 offset;

    if(update_msg.fw_burned_len >= update_msg.fw_total_len)
    {
        GTP_DEBUG("No need to upgrade the link code!");
        return SUCCESS;
    }
    GTP_INFO("[burn_fw_link]Begin burn link firmware---->>");

    //step1:Alloc memory
    GTP_DEBUG("[burn_fw_link]step1:Alloc memory");
    while(retry++ < 5)
    {
        fw_link = (u8*)kzalloc(FW_SECTION_LENGTH, GFP_KERNEL);
        if(fw_link == NULL)
        {
            continue;
        }
        else
        {
            GTP_DEBUG("[burn_fw_link]Alloc %dk byte memory success.", (FW_SECTION_LENGTH/1024));
            break;
        }
    }
    if(retry >= 5)
    {
        GTP_ERROR("[burn_fw_link]Alloc memory fail,exit.");
        return FAIL;
    }

    //step2:load firmware link section 1
    GTP_DEBUG("[burn_fw_link]step2:load firmware link section 1");
    offset = update_msg.fw_burned_len - FW_DSP_ISP_LENGTH;
    ret = gup_load_section_file(fw_link, offset, FW_SECTION_LENGTH, SEEK_SET);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_link]load firmware link section 1 fail.");
        goto exit_burn_fw_link;
    }

    //step3:burn link firmware section 1
    GTP_DEBUG("[burn_fw_link]step3:burn link firmware section 1");
    ret = gup_burn_fw_gwake_section(fw_link, 0x9000, FW_SECTION_LENGTH, 0x38);

    if (FAIL == ret)
    {
        GTP_ERROR("[burn_fw_link]burn link firmware section 1 fail.");
        goto exit_burn_fw_link;
    }

    //step4:load link firmware section 2 file data
    GTP_DEBUG("[burn_fw_link]step4:load link firmware section 2 file data");
    offset += FW_SECTION_LENGTH;
    ret = gup_load_section_file(fw_link, offset, FW_GLINK_LENGTH - FW_SECTION_LENGTH, SEEK_SET);

    if (FAIL == ret)
    {
        GTP_ERROR("[burn_fw_link]load link firmware section 2 fail.");
        goto exit_burn_fw_link;
    }

    //step5:burn link firmware section 2
    GTP_DEBUG("[burn_fw_link]step4:burn link firmware section 2");
    ret = gup_burn_fw_gwake_section(fw_link, 0x9000, FW_GLINK_LENGTH - FW_SECTION_LENGTH, 0x39);

    if (FAIL == ret)
    {
        GTP_ERROR("[burn_fw_link]burn link firmware section 2 fail.");
        goto exit_burn_fw_link;
    }

    update_msg.fw_burned_len += FW_GLINK_LENGTH;
    GTP_DEBUG("[burn_fw_link]Burned length:%d", update_msg.fw_burned_len);
    ret = SUCCESS;

exit_burn_fw_link:
    kfree(fw_link);
    return ret;
}

static u8 gup_burn_fw_gwake_section(u8 *fw_section, u16 start_addr, u32 len, u8 bank_cmd )
{
    s32 ret = 0;
    u8  rd_buf[5];

    //step1:hold ss51 & dsp
    ret = gup_set_ic_msg( _rRW_MISCTL__SWRST_B0_, 0x0C);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_app_section]hold ss51 & dsp fail.");
        return FAIL;
    }

    //step2:set scramble
    ret = gup_set_ic_msg(_rRW_MISCTL__BOOT_OPT_B0_, 0x00);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_app_section]set scramble fail.");
        return FAIL;
    }

    //step3:hold ss51 & release dsp
    ret = gup_set_ic_msg(_rRW_MISCTL__SWRST_B0_, 0x04);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_app_section]hold ss51 & release dsp fail.");
        return FAIL;
    }
    //must delay
    msleep(1);

    //step4:select bank
    ret = gup_set_ic_msg(_bRW_MISCTL__SRAM_BANK, (bank_cmd >> 4)&0x0F);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_section]select bank %d fail.", (bank_cmd >> 4)&0x0F);
        return FAIL;
    }

    //step5:burn fw section
    ret = gup_burn_proc(fw_section, start_addr, len);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_app_section]burn fw_section fail.");
        return FAIL;
    }

    //step6:send burn cmd to move data to flash from sram
    ret = gup_set_ic_msg(_rRW_MISCTL__BOOT_CTL_, bank_cmd&0x0F);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_app_section]send burn cmd fail.");
        return FAIL;
    }
    GTP_DEBUG("[burn_fw_section]Wait for the burn is complete......");
    do{
        ret = gup_get_ic_msg(_rRW_MISCTL__BOOT_CTL_, rd_buf, 1);
        if(ret <= 0)
        {
            GTP_ERROR("[burn_fw_app_section]Get burn state fail");
            return FAIL;
        }
        msleep(10);
        GTP_DEBUG("[burn_fw_app_section]Get burn state:%d.", rd_buf[GTP_ADDR_LENGTH]);
    }while(rd_buf[0]);

    //step7:recall fw section
    ret = gup_recall_check(fw_section, start_addr, len);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_app_section]recall check %dk firmware fail.", len/1024);
        return FAIL;
    }

    return SUCCESS;
}

static u8 gup_burn_fw_gwake(void)
{
    u8* fw_gwake = NULL;
    u8  retry = 0;
    s32 ret = 0;
    u16 start_index = 4*FW_SECTION_LENGTH+FW_DSP_LENGTH+FW_BOOT_LENGTH + FW_BOOT_ISP_LENGTH + FW_GLINK_LENGTH; // 32 + 4 + 2 + 4 = 42K
    //u16 start_index;

    if(update_msg.fw_burned_len >= update_msg.fw_total_len)
    {
        GTP_DEBUG("No need to upgrade the gwake code!");
        return SUCCESS;
    }
    //start_index = update_msg.fw_burned_len - FW_DSP_ISP_LENGTH;
    GTP_INFO("[burn_fw_gwake]Begin burn gwake firmware---->>");

    //step1:alloc memory
    GTP_DEBUG("[burn_fw_gwake]step1:alloc memory");
    while(retry++ < 5)
    {
        fw_gwake = (u8*)kzalloc(FW_SECTION_LENGTH, GFP_KERNEL);
        if(fw_gwake == NULL)
        {
            continue;
        }
        else
        {
            GTP_DEBUG("[burn_fw_gwake]Alloc %dk byte memory success.", (FW_SECTION_LENGTH/1024));
            break;
        }
    }
    if(retry >= 5)
    {
        GTP_ERROR("[burn_fw_gwake]Alloc memory fail,exit.");
        return FAIL;
    }

	//clear control flag
	ret = gup_set_ic_msg(_rRW_MISCTL__BOOT_CTL_, 0x00);
	if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_finish]clear control flag fail.");
        goto exit_burn_fw_gwake;
    }

    //step2:load app_code firmware section 1 file data
    GTP_DEBUG("[burn_fw_gwake]step2:load app_code firmware section 1 file data");
    ret = gup_load_section_file(fw_gwake, start_index, FW_SECTION_LENGTH, SEEK_SET);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_gwake]load app_code firmware section 1 fail.");
        goto exit_burn_fw_gwake;
    }

    //step3:burn app_code firmware section 1
    GTP_DEBUG("[burn_fw_gwake]step3:burn app_code firmware section 1");
    ret = gup_burn_fw_gwake_section(fw_gwake, 0x9000, FW_SECTION_LENGTH, 0x3A);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_gwake]burn app_code firmware section 1 fail.");
        goto exit_burn_fw_gwake;
    }

    //step5:load app_code firmware section 2 file data
    GTP_DEBUG("[burn_fw_gwake]step5:load app_code firmware section 2 file data");
    ret = gup_load_section_file(fw_gwake, start_index+FW_SECTION_LENGTH, FW_SECTION_LENGTH, SEEK_SET);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_gwake]load app_code firmware section 2 fail.");
        goto exit_burn_fw_gwake;
    }

    //step6:burn app_code firmware section 2
    GTP_DEBUG("[burn_fw_gwake]step6:burn app_code firmware section 2");
    ret = gup_burn_fw_gwake_section(fw_gwake, 0x9000, FW_SECTION_LENGTH, 0x3B);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_gwake]burn app_code firmware section 2 fail.");
        goto exit_burn_fw_gwake;
    }

    //step7:load app_code firmware section 3 file data
    GTP_DEBUG("[burn_fw_gwake]step7:load app_code firmware section 3 file data");
    ret = gup_load_section_file(fw_gwake, start_index+2*FW_SECTION_LENGTH, FW_SECTION_LENGTH, SEEK_SET);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_gwake]load app_code firmware section 3 fail.");
        goto exit_burn_fw_gwake;
    }

    //step8:burn app_code firmware section 3
    GTP_DEBUG("[burn_fw_gwake]step8:burn app_code firmware section 3");
    ret = gup_burn_fw_gwake_section(fw_gwake, 0x9000, FW_SECTION_LENGTH, 0x3C);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_gwake]burn app_code firmware section 3 fail.");
        goto exit_burn_fw_gwake;
    }

    //step9:load app_code firmware section 4 file data
    GTP_DEBUG("[burn_fw_gwake]step9:load app_code firmware section 4 file data");
    ret = gup_load_section_file(fw_gwake, start_index + 3*FW_SECTION_LENGTH, FW_SECTION_LENGTH, SEEK_SET);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_gwake]load app_code firmware section 4 fail.");
        goto exit_burn_fw_gwake;
    }

    //step10:burn app_code firmware section 4
    GTP_DEBUG("[burn_fw_gwake]step10:burn app_code firmware section 4");
    ret = gup_burn_fw_gwake_section(fw_gwake, 0x9000, FW_SECTION_LENGTH, 0x3D);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_gwake]burn app_code firmware section 4 fail.");
        goto exit_burn_fw_gwake;
    }

    //update_msg.fw_burned_len += FW_GWAKE_LENGTH;
    GTP_DEBUG("[burn_fw_gwake]Burned length:%d", update_msg.fw_burned_len);
    ret = SUCCESS;

exit_burn_fw_gwake:
    kfree(fw_gwake);
    return ret;
}

static u8 gup_burn_fw_finish(void)
{
    u8* fw_ss51 = NULL;
    u8  retry = 0;
    s32 ret = 0;

    GTP_INFO("[burn_fw_finish]burn first 8K of ss51 and finish update.");
    //step1:alloc memory
    GTP_DEBUG("[burn_fw_finish]step1:alloc memory");
    while(retry++ < 5)
    {
        fw_ss51 = (u8*)kzalloc(FW_SECTION_LENGTH, GFP_KERNEL);
        if(fw_ss51 == NULL)
        {
            continue;
        }
        else
        {
            GTP_DEBUG("[burn_fw_finish]Alloc %dk byte memory success.", (FW_SECTION_LENGTH/1024));
            break;
        }
    }
    if(retry >= 5)
    {
        GTP_ERROR("[burn_fw_finish]Alloc memory fail,exit.");
        return FAIL;
    }

    GTP_DEBUG("[burn_fw_finish]step2: burn ss51 first 8K.");
    ret = gup_load_section_file(fw_ss51, 0, FW_SECTION_LENGTH, SEEK_SET);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_finish]load ss51 firmware section 1 fail.");
        goto exit_burn_fw_finish;
    }

    GTP_DEBUG("[burn_fw_finish]step3:clear control flag");
    ret = gup_set_ic_msg(_rRW_MISCTL__BOOT_CTL_, 0x00);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_finish]clear control flag fail.");
        goto exit_burn_fw_finish;
    }

    GTP_DEBUG("[burn_fw_finish]step4:burn ss51 firmware section 1");
    ret = gup_burn_fw_section(fw_ss51, 0xC000, 0x01);
    if(FAIL == ret)
    {
        GTP_ERROR("[burn_fw_finish]burn ss51 firmware section 1 fail.");
        goto exit_burn_fw_finish;
    }

    //step11:enable download DSP code
    GTP_DEBUG("[burn_fw_finish]step5:enable download DSP code ");
    ret = gup_set_ic_msg(_rRW_MISCTL__BOOT_CTL_, 0x99);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_finish]enable download DSP code fail.");
        goto exit_burn_fw_finish;
    }

    //step12:release ss51 & hold dsp
    GTP_DEBUG("[burn_fw_finish]step6:release ss51 & hold dsp");
    ret = gup_set_ic_msg(_rRW_MISCTL__SWRST_B0_, 0x08);
    if(ret <= 0)
    {
        GTP_ERROR("[burn_fw_finish]release ss51 & hold dsp fail.");
        goto exit_burn_fw_finish;
    }

    if (fw_ss51)
    {
        kfree(fw_ss51);
    }
    return SUCCESS;

exit_burn_fw_finish:
    if (fw_ss51)
    {
        kfree(fw_ss51);
    }
    return FAIL;
}

int gup_chip_reinit(struct goodix_ts_data *ts)
{
	int ret = 0;

	GTP_INFO("[update_proc]chip reinit start.");
    /* read version information. pid/vid/sensor id */
    ret = goodix_read_version(&ts->hw_info);
    if (ret < 0)
    {
		GTP_ERROR("[update_proc]read verison fail.");
		return ret;
    }

    /* obtain specific dt properties */
    ret = goodix_parse_specific_dts(ts);
    if (ret < 0)
    {
		GTP_ERROR("[update_proc]parse specific dts properties.");
		return ret;
    }

    if (ts->tools_support)
        init_wr_node();

    goodix_param_init();

    /* init config data, normal/glove/hoslter config data */
    ret = goodix_init_configs(ts);
    if (ret < 0) {
        GTP_ERROR("[update_proc]init configs failed");
		return ret;
    }

    ret = goodix_check_normal_config_version(&ts->normal_config);
	if (ret < 0) {
		GTP_INFO("[update_proc]no need check normal config version");
	} else {
		GTP_INFO("[update_proc]check normal config version success");
	}

#ifdef ROI
    goodix_ts_roi_init(&ts->roi);
#endif

    ret = goodix_get_fw_data();
    if (ret < 0) {
        GTP_ERROR("[update_proc]get fw data failed");
		return ret;
    }

	GTP_INFO("[update_proc]chip reinit ok");

	return 0;
}

s32 gup_update_proc(int type)
{
    s32 ret = 0;
    s32 update_ret = FAIL;
    u8  retry = 0;
    st_fw_head fw_head;
    struct goodix_ts_data *ts = NULL;

    GTP_DEBUG("[update_proc]Begin update ......");
    if (type != UPDATE_TYPE_FILE && type != UPDATE_TYPE_HEADER) {
	GTP_ERROR("Invalid update type");
	return -EINVAL;
    }

    ts = goodix_ts;

    if (searching_file)
    {
        u8 timeout = 0;
        searching_file = 0;     // exit .bin update file searching
        GTP_INFO("Exiting searching .bin update file...");
        while ((show_len != 200) && (show_len != 100) && (timeout++ < 100))     // wait for auto update quitted completely
        {
            msleep(100);
        }
    }

    show_len = 1;
    total_len = 100;

    update_msg.file = NULL;
    update_msg.fw = NULL;
    update_msg.update_type = type;
    ret = gup_check_update_file(&fw_head);     //20121211
    if(FAIL == ret)
    {
        GTP_ERROR("[update_proc]check update file fail.");
#if defined (CONFIG_HUAWEI_DSM)
        g_goodix_dev_data->ts_platform_data->dsm_info.constraints_UPDATE_status =  GTP_CHECK_UPDATE_FILE_FAIL;
#endif
        goto file_fail;
    } else if(FW_NOT_EXIST == ret) {
	    GTP_ERROR("fw image no exit, not need to update\n");
	    update_ret = SUCCESS;
	    goto file_fail;
    }

    ret = gup_get_ic_fw_msg();
    if(FAIL == ret)
    {
        GTP_ERROR("[update_proc]get ic message fail.");
#if defined (CONFIG_HUAWEI_DSM)
        g_goodix_dev_data->ts_platform_data->dsm_info.constraints_UPDATE_status =  GTP_GET_IC_FW_FAIL;
#endif
        goto file_fail;
    }

    ret = gup_enter_update_judge(&fw_head);
    if (FW_NO_NEED_UPDATE == ret)
    {
        GTP_INFO("[update_proc] no need to update firmware.");
        update_ret = SUCCESS;
        goto file_fail;
    }
	else if(FAIL == ret)
    {
        GTP_ERROR("[update_proc]Check *.bin file fail.");
#if defined (CONFIG_HUAWEI_DSM)
        g_goodix_dev_data->ts_platform_data->dsm_info.constraints_UPDATE_status =  GTP_ENTER_UPDATE_JUDGE_FAIL;
#endif
        goto file_fail;
    }

    ts->enter_update = 1;
    //gtp_irq_disable(ts);
    ret = gup_enter_update_mode();
    if(FAIL == ret)
    {
         GTP_ERROR("[update_proc]enter update mode fail.");
#if defined (CONFIG_HUAWEI_DSM)
         g_goodix_dev_data->ts_platform_data->dsm_info.constraints_UPDATE_status =  GTP_ENTER_UPDATE_MODE_FAIL;
#endif
         goto update_fail;
    }

    while(retry++ < 5)
    {
        show_len = 10;
        total_len = 100;
        update_msg.fw_burned_len = 0;
        ret = gup_burn_dsp_isp();
        if(FAIL == ret)
        {
            GTP_ERROR("[update_proc]burn dsp isp fail.");
#if defined (CONFIG_HUAWEI_DSM)
            g_goodix_dev_data->ts_platform_data->dsm_info.constraints_UPDATE_status =  GTP_BURN_DSP_ISP_FAIL;
#endif
            continue;
        }

	 show_len = 20;
	 ret = gup_burn_fw_gwake();
        if (FAIL == ret)
        {
            GTP_ERROR("[update_proc]burn app_code firmware fail.");
#if defined (CONFIG_HUAWEI_DSM)
            g_goodix_dev_data->ts_platform_data->dsm_info.constraints_UPDATE_status =  GTP_BURN_FW_GWAKE_FAIL;
#endif
            continue;
        }

        show_len = 30;
        ret = gup_burn_fw_ss51();
        if(FAIL == ret)
        {
            GTP_ERROR("[update_proc]burn ss51 firmware fail.");
#if defined (CONFIG_HUAWEI_DSM)
            g_goodix_dev_data->ts_platform_data->dsm_info.constraints_UPDATE_status =  GTP_BURN_FW_SS51_FAIL;
#endif
            continue;
        }

        show_len = 40;
        ret = gup_burn_fw_dsp();
        if(FAIL == ret)
        {
            GTP_ERROR("[update_proc]burn dsp firmware fail.");
#if defined (CONFIG_HUAWEI_DSM)
            g_goodix_dev_data->ts_platform_data->dsm_info.constraints_UPDATE_status =  GTP_BURN_FW_DSP_FAIL;
#endif
            continue;
        }

        show_len = 50;
        ret = gup_burn_fw_boot();
        if(FAIL == ret)
        {
            GTP_ERROR("[update_proc]burn bootloader firmware fail.");
#if defined (CONFIG_HUAWEI_DSM)
            g_goodix_dev_data->ts_platform_data->dsm_info.constraints_UPDATE_status =  GTP_BURN_FW_BOOT_FAIL;
#endif
            continue;
        }
        show_len = 60;

        ret = gup_burn_fw_boot_isp();
        if (FAIL == ret)
        {
            GTP_ERROR("[update_proc]burn boot_isp firmware fail.");
#if defined (CONFIG_HUAWEI_DSM)
            g_goodix_dev_data->ts_platform_data->dsm_info.constraints_UPDATE_status =  GTP_BURN_FW_BOOT_ISP_FAIL;
#endif
            continue;
        }

        show_len = 70;
        ret = gup_burn_fw_link();
        if (FAIL == ret)
        {
            GTP_ERROR("[update_proc]burn link firmware fail.");
#if defined (CONFIG_HUAWEI_DSM)
            g_goodix_dev_data->ts_platform_data->dsm_info.constraints_UPDATE_status =  GTP_BURN_FW_LINK_FAIL;
#endif
            continue;
        }

        show_len = 80;
        ret = gup_burn_fw_finish();
        if (FAIL == ret)
        {
            GTP_ERROR("[update_proc]burn finish fail.");
#if defined (CONFIG_HUAWEI_DSM)
            g_goodix_dev_data->ts_platform_data->dsm_info.constraints_UPDATE_status =  GTP_BURN_FW_FINISH_FAIL;
#endif
            continue;
        }
        show_len = 90;
        GTP_INFO("[update_proc]UPDATE SUCCESS.");
	goodix_ts->hw_info.patch_id = (u32)fw_head.vid<<8;
        retry = 0;
        break;
    }

    if (retry >= 5)
    {
        GTP_ERROR("[update_proc]retry timeout,UPDATE FAIL.");
        update_ret = FAIL;
#if defined (CONFIG_HUAWEI_DSM)
        if (!dsm_client_ocuppy(ts_dclient)) {
            TS_LOG_INFO("%s: try to client record DSM_TP_FWUPDATE_ERROR_NO(%d)\n", __func__, DSM_TP_FWUPDATE_ERROR_NO);
            dsm_client_record(ts_dclient, "fw update result: failed updata_status is %d.\n",
				g_goodix_dev_data->ts_platform_data->dsm_info.constraints_UPDATE_status);
            dsm_client_notify(ts_dclient,DSM_TP_FWUPDATE_ERROR_NO);
        }
        strncpy(g_goodix_dev_data->ts_platform_data->dsm_info.fw_update_result, "failed", strlen("failed"));
#endif
    }
    else
    {
        update_ret = SUCCESS;
    }

update_fail:
    GTP_DEBUG("[update_proc]leave update mode.");
    gup_leave_update_mode();

    msleep(100);

    if (SUCCESS == update_ret)
    {
        if (ts->fw_error)
        {
            GTP_INFO("firmware error auto update, reset config!");
            gup_init_panel(ts);
        }
        else
        {
            if (update_msg.force_update != 0xBE) {
				ret = gup_chip_reinit(ts);
				if (ret < 0) {
					GTP_ERROR("[update_proc]chip reinit failed.");
				}
            }

            GTP_DEBUG("[update_proc]send config.");
            ret = goodix_send_cfg(&goodix_ts->normal_config);
            if (ret != 0)
            {
                GTP_ERROR("[update_proc]send config fail.");
            }
            else
            {
                msleep(100);
            }
        }
    }
    ts->enter_update = 0;
    //gtp_irq_enable(ts);

file_fail:
	if (update_msg.update_type == UPDATE_TYPE_HEADER) {
		if (update_msg.fw != NULL) {
			release_firmware(update_msg.fw);
			update_msg.fw = NULL;
		}
	}

	if (update_msg.update_type == UPDATE_TYPE_FILE) {
		if (update_msg.file && !IS_ERR(update_msg.file))
		{
	        if (update_msg.old_fs)
	        {
	            set_fs(update_msg.old_fs);
	        }
			filp_close(update_msg.file, NULL);
		}
	}

#if GTP_AUTO_UPDATE_CFG

    if ((update_msg.update_type==UPDATE_TYPE_HEADER)&&NULL == dir)
    {
        gup_search_file(AUTO_SEARCH_CFG);
        if (got_file_flag & CFG_FILE_READY)
        {
            ret = gup_update_config();
            if(ret <= 0)
            {
                GTP_ERROR("Update config failed.");
            }
            _CLOSE_FILE(update_msg.cfg_file);
            msleep(500);                //waiting config to be stored in FLASH.
        }
    }
#endif

    total_len = 100;
    if (SUCCESS == update_ret)
    {
        show_len = 100;
        return 0;
    }
    else
    {
        show_len = 200;
        return -1;
    }
}
