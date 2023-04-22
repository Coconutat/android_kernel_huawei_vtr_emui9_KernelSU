/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/suspend.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <bsp_slice.h>
#include <bsp_pm.h>
#include <bsp_dump.h>
#include <bsp_pm_om.h>
#include <bsp_m3pm_log.h>
#include "pm_om_platform.h"
#include "pm_om_debug.h"
#include <mdrv_pm_common.h>

struct pm_om_debug g_pmom_debug;
char *wakelock_name[]={
    "TLPHY" ,   /*MSP--fuxin*/
    "PS_G0",        /*GU--ganlan*/
    "PS_W0",       /*GU--ganlan*/
    "PS_G1",        /*GU--ganlan*/
    "PS_W1",       /*GU--ganlan*/
    "FTM",           /*GU--zhangyizhan*/
    "FTM_1",       /*GU--zhangyizhan*/
    "NAS",           /*GU--zhangyizhan*/
    "NAS_1",       /*GU--zhangyizhan*/
    "OAM",          /* GU--zhangyizhan */
    "SCI0",            /* LTE --yangzhi */
    "SCI1",            /* LTE --yangzhi */
    "TLPS",             /* LTE --maobibo*/
    "TLPS1",              /*LTE --maobibo*/
    "DSFLOW",          /* NAS --zhangyizhan */
    "PM",            /* PM  ---shangmianyou */
    "UART0",        /*UART0 -zhangliangdong */
    "TDS",         /*TRRC&TL2----leixiantiao*/
    "CDMAUART",         /*drv cdma uart 数传*/
    "USIM",             /*oam*/
    "DSPPOWERON",       /*v8r1 ccore 提供给GUTL DSP作为c核上电初始化投票用*/
    "RESET",            /*RESET -nieluhua */
    "PS_G2",        /*GU--ganlan*/
    "FTM_2",       /*GU--zhangyizhan*/
    "NAS_2",       /*GU--zhangyizhan*/
    "1X",       /*CDMA--ganlan*/
    "HRPD",       /*CDMA--ganlan*/
    "MSP",       /*cuijunqiang*/
    "VOWIFI",
    "RNIC"
};
/*lint --e{64,528}*//*64:list_for_each_entry, 528 for not referenced referenced*/
void pm_om_wakeup_log(void)
{
	if (g_pmom_debug.stat.wrs_flag == 1)
	{
		bsp_pm_log(PM_OM_PMLG, 0, NULL);
		g_pmom_debug.stat.wrs_flag = 0;
	}
}

void pm_om_wakeup_stat(void)
{
	u32 cur_slice   = bsp_get_slice_value();
	u32 delta_slice = 0;

	g_pmom_debug.stat.wakeup_cnt++;

	delta_slice = delta_slice - g_pmom_debug.stat.waket_prev;
	g_pmom_debug.stat.waket_prev = cur_slice;
	if (delta_slice > g_pmom_debug.stat.waket_max)
	{
		g_pmom_debug.stat.waket_max = delta_slice;
	}
	if (delta_slice < g_pmom_debug.stat.waket_min)
	{
		g_pmom_debug.stat.waket_min = delta_slice;
	}

	/* 32768hz，转换成秒
	delta_slice /= TIME_SLICE_FREQ;*/
	g_pmom_debug.stat.wrs_flag = 1;
	pmom_print("[C SR]pm om wakeup\n");
}

u32 pm_om_log_time_rec(u32 time_log_start)
{
	u32 ret = 0;
	u32 delta_slice = bsp_get_slice_value();
	delta_slice = delta_slice - time_log_start;

	if (g_pmom_debug.stat.logt_print_sw && delta_slice > g_pmom_debug.stat.logt_max)
	{
		g_pmom_debug.stat.logt_max = delta_slice;
		ret = delta_slice;
	}
	return ret;
}

void pm_om_dbg_on(void)
{
	bsp_mod_level_set(BSP_MODU_PM_OM, BSP_LOG_LEVEL_DEBUG);
}

void pm_om_dbg_off(void)
{
	bsp_mod_level_set(BSP_MODU_PM_OM, BSP_LOG_LEVEL_ERROR);
}

void pm_om_log_show(void)
{
	struct pm_om_ctrl *ctrl = pm_om_ctrl_get();
	pmom_print("**********************************************\n");
	pmom_print("init_flag   : 0x%x\n", ctrl->log.init_flag);
	pmom_print("buf_is_full : 0x%x\n", ctrl->log.buf_is_full);
	pmom_print("threshold   : 0x%x\n", ctrl->log.threshold);
	pmom_print("debug       : 0x%p\n", ctrl->debug);
	pmom_print("platform    : 0x%p\n", ctrl->platform);
	pmom_print("**********************************************\n");
}

void pm_om_debug_show(void)
{
	pmom_print("**********************************************\n");
	pmom_print("logt_print_sw: 0x%x\n", g_pmom_debug.stat.logt_print_sw);
	pmom_print("logt_max\t: 0x%x\n", g_pmom_debug.stat.logt_max);
	pmom_print("**********************************************\n");
}

u32 pm_om_feature_on(void)
{
	struct pm_om_ctrl *ctrl = pm_om_ctrl_get();
	struct pm_om_platform *linux_plat;
	if (NULL == ctrl || NULL == ctrl->platform)
	{
		return 1;
	}

	linux_plat = (struct pm_om_platform *)ctrl->platform;
	ctrl->log.init_flag = PM_OM_INIT_MAGIC;
	ctrl->log.smem->nv_cfg.mod_sw_bitmap = 0xFFFFFFFFFFFFFFFF;
	linux_plat->log_info.mem_is_ok = 1;

	return 0;
}

/* pmom在mbb/phone共代码, 都需要实现. pm_wakeup_ccore在phone需要在pmom模块实现,mbb上在pm模块实现 */
s32 pm_wakeup_ccore(enum debug_wake_type type)
{
	u32 channel_id = (ICC_CHN_IFC << 16) | IFC_RECV_FUNC_WAKEUP;
	s32 ret;
	struct debug_pm_s pm_msg;
	u32 msglen = sizeof(pm_msg);
	pm_msg.wake_type=type;

	ret = bsp_icc_send(ICC_CPU_MODEM, channel_id, (u8*)(&pm_msg), msglen);
	if((u32)ret != msglen)
	{
		return -1;
	}
    return 0;
}

void debug_pm_wake_lock(void)
{
    wake_lock(&g_pmom_debug.wakelock_debug);
} /*lint !e454*/

void debug_pm_wake_unlock(void)
{
    wake_unlock(&g_pmom_debug.wakelock_debug); /*lint !e455*/
}

static s32 pm_wakeup_icc_msg(u32 id , u32 len, void* context)
{/*lint --e{715} suppress context not referenced*/
	u32 ret;
	struct debug_pm_s read_data;

	/* 不可以通过icc发送非法长度的消息过来(包括长度为0) */
	if(len != (u32)sizeof(read_data))
	{
		pmom_pr_err("icc packet len(%d) != sizeof(struct debug_pm_s)(%d)\n", len, (u32)sizeof(read_data));
		return -1;
	}

	ret = (u32)bsp_icc_read(id, (u8*)&read_data, len);
	if (ret != (u32)sizeof(read_data))
	{
		pmom_pr_err("readed len(%d) != icc packet len(%d)", ret, len);
		return -1;
	}
	if(PM_WAKEUP == read_data.wake_type)
	{
		debug_pm_wake_lock();
	}
	else if(PM_WAKEUP_THEN_SLEEP== read_data.wake_type)
	{
		debug_pm_wake_unlock();
	}
	return 0;
}
#define print_buf_size  (800)
char print_buf[print_buf_size];

static inline void print_dpm_device_info(void)
{
    ssize_t count = 0;
    struct dpm_device_info *device_info;
    device_info = (struct dpm_device_info *)(g_pmom_debug.cdrx_dump_addr + CDRX_DUMP_DPM_INFOS_OFFSET);/*lint !e826 suppress pointer-to-pointer conversion */

    count += snprintf(print_buf,(size_t)print_buf_size,"[C SR]dpm (fail_cnt, max suspend, resume):");

    while(*device_info->device_name != 0){
        if( ((unsigned long)device_info + sizeof(struct dpm_device_info)) >= ((unsigned long)g_pmom_debug.cdrx_dump_addr+CDRX_DUMP_DPM_INFOS_END)){
            return;
        }
        if(strlen(device_info->device_name)>dpm_debug_char_num){
            printk(KERN_ERR"print cdpm get an err name \n");
            break;
        }
        if((count+50) > print_buf_size){
            printk(KERN_ERR"print cdpm, count=%d, buf no mem\n",(int)count);
            break;
        }
        count += snprintf(print_buf+count,(size_t)(print_buf_size-count),"%s(%d,%d,%d),"\
            ,device_info->device_name, device_info->fail_cnt, device_info->max_s, device_info->max_r);

        device_info++;
    }
    count += snprintf(print_buf+count,(size_t)(print_buf_size-count),"\n");/* [false alarm]:屏蔽Fortify */

    printk(KERN_ERR"%s",print_buf);
}
static inline void print_ccpu_wakeup_irq_info(void)
{/*lint --e{737} suppress snprintf return value turn to unsigned int, TODO*/
	u32 ret,i,cnt = 0;
	struct pm_wakeup_irq_info *wakeirq_debug_addr;
	wakeirq_debug_addr = (struct pm_wakeup_irq_info *)(g_pmom_debug.cdrx_dump_corepm_addr +WAKEUP_IRQ_DEBUG );/*lint !e826 suppress pointer-to-pointer conversion */
	ret = (u32)readl(g_pmom_debug.cdrx_dump_corepm_addr + WAKEUP_INT_NUM);
	cnt += snprintf((char*)print_buf,(size_t)print_buf_size,"[C SR]pm wake cnt:");/*lint !e737 suppress promotion from int to unsigned int */
	for(i=0;i<ret;i++)
	{
		if(strlen((wakeirq_debug_addr+i)->name)>dpm_debug_char_num){
			printk(KERN_ERR"print cwakelock get an err name \n");
			break;
		}
		if((cnt+30) > print_buf_size){
			printk(KERN_ERR"print cwakelock, count=%d, buf no mem\n",cnt);
			break;
		}
		cnt += snprintf((char*)print_buf+cnt,(size_t)(print_buf_size-cnt),"%s(%d),",\
			(wakeirq_debug_addr+i)->name,\
			(wakeirq_debug_addr+i)->wake_cnt);
	}
	cnt += snprintf((char*)print_buf+cnt,(size_t)(print_buf_size-cnt),"\n");
	printk(KERN_ERR"%s",print_buf);
}
static inline void print_ccpu_wakelock_info(void)
{/*lint --e{737} suppress snprintf return value turn to unsigned int, TODO*/
	u32 i,ret,cnt = 0,temp;
	cnt += snprintf((char*)print_buf,(size_t)print_buf_size,"[C SR]wakelock(locked,total time):");
	ret = (u32)readl(g_pmom_debug.cdrx_dump_addr+CDRX_DUMP_WAKE_OFFSET);
	temp = sizeof(wakelock_name)/sizeof(wakelock_name[0]);
	for(i=0;i<temp;i++)
	{
		cnt += snprintf((char*)print_buf+cnt,(size_t)(print_buf_size-cnt),"%s(%d,0x%x),", wakelock_name[i],(ret>>i)&0x1,readl(g_pmom_debug.cdrx_dump_addr+CDRX_DUMP_WAKELOCK_TIME_OFFSET+0x4*i));
	}
	cnt += snprintf((char*)print_buf+cnt,(size_t)(print_buf_size-cnt),"\n");
	printk(KERN_ERR"%s",print_buf);
}


static inline void print_ccpu_pm_info(void)
{/*lint --e{826} suppress pointer-to-pointer conversion*/
	printk(KERN_ERR"[C SR]pm:in cnt:(%d),out cnt:(%d),dpm fail cnt:(%d),gic fail cnt:(%d),sleep total(0x%x),wake total(0x%x),last waked by %s\n"\
		,*((u32 *)(g_pmom_debug.cdrx_dump_corepm_addr + PM_ENTER_COUNT)) \
		,*((u32 *)(g_pmom_debug.cdrx_dump_corepm_addr + PM_OUT_COUNT)) \
		,*((u32 *)(g_pmom_debug.cdrx_dump_corepm_addr + PM_DPM_FAIL_COUNT))\
		,*((u32 *)(g_pmom_debug.cdrx_dump_corepm_addr + PM_GIC_SUS_FAIL_TIME))\
		,*((u32 *)(g_pmom_debug.cdrx_dump_corepm_addr + PM_SYS_SLEEP_TOTAL_TIME)) \
		,*((u32 *)(g_pmom_debug.cdrx_dump_corepm_addr + PM_SYS_WORK_TOTAL_TIME))\
		,(char*)((g_pmom_debug.cdrx_dump_corepm_addr + WAKEUP_LATEST_INTER_NAME)));
	printk(KERN_ERR"[C SR]pm slice(start,end):sleep (0x%x, 0x%x), wake (0x%x, 0x%x)\n"\
		,*((u32 *)(g_pmom_debug.cdrx_dump_corepm_addr + STAMP_PM_SUSPEND_START)) \
		,*((u32 *)(g_pmom_debug.cdrx_dump_corepm_addr + STAMP_AFTER_SEND_IPC)) \
		,*((u32 *)(g_pmom_debug.cdrx_dump_corepm_addr + STAMP_PWRUP_CODE_BEGIN)) \
		, *((u32 *)(g_pmom_debug.cdrx_dump_corepm_addr + STAMP_AFTER_DPM_RESUME)));
	print_ccpu_wakeup_irq_info();
}

static inline void print_ccpu_ipc_info(void)
{/*lint --e{737} suppress snprintf return value turn to unsigned int, TODO*/
	u32 i, cnt = 0;

	/* coverity[secure_coding] */
	cnt += snprintf((char*)print_buf,(size_t)print_buf_size,"[C SR]ipcm(int count):");
	for(i=0; i<IPC_INT_BUTTOM; i++)
	{
		/* coverity[secure_coding] */
		cnt += snprintf((char*)print_buf+cnt,(size_t)(print_buf_size-cnt),"%d,", readl(g_pmom_debug.cdrx_dump_addr+CDRX_DUMP_IPC_OFFSET+4*i)); /* [false alarm]:fortify */
	}
	/* coverity[secure_coding] */
	cnt += snprintf((char*)print_buf+cnt,(size_t)(print_buf_size-cnt),"\n");
	printk(KERN_ERR"%s",print_buf);
}

static inline void print_ccpu_icc_info(void)
{/*lint --e{737} suppress snprintf return value turn to unsigned int, TODO*/
	u32 i, cnt = 0;

	/* coverity[secure_coding] */
	cnt += snprintf((char*)print_buf,(size_t)print_buf_size,"[C SR]icc(channel count):");
	for(i=0; i<ICC_CHN_ID_MAX; i++)
	{
		/* coverity[secure_coding] */
		cnt += snprintf((char*)print_buf+cnt,(size_t)(print_buf_size-cnt),"%d,", readl(g_pmom_debug.cdrx_dump_addr+CDRX_DUMP_ICC_OFFSET+4*i)); /* [false alarm]:fortify */
	}
	/* coverity[secure_coding] */
	cnt += snprintf((char*)print_buf+cnt,(size_t)(print_buf_size-cnt),"\n");
	printk(KERN_ERR"%s",print_buf);
}



void print_ccpu_lowpower_info(void)
{
	printk(KERN_ERR"[C SR]current slice:0x%x, mem phy:0x%pK, virt:0x%pK\n",
        bsp_get_slice_value(), bsp_dump_get_field_phy_addr(DUMP_CP_DRX), g_pmom_debug.cdrx_dump_addr);
	print_ccpu_wakelock_info();
	print_ccpu_pm_info();
	print_dpm_device_info();
	print_ccpu_ipc_info();
	print_ccpu_icc_info();
}

int cp_pm_notify(struct notifier_block *nb, unsigned long event, void *dummy)
{/*lint --e{715} suppress 'nb' & 'dummy' not referenced*/
	char* dump_base = NULL;
	if(event == PM_SUSPEND_PREPARE)
	{
		if(!g_pmom_debug.cdrx_dump_addr)
		{
			/*获取DUMP 低功耗共享内存地址*/
			dump_base = (char *)bsp_dump_get_field_addr(DUMP_CP_DRX);
			if(NULL == dump_base)
			{
				pmom_pr_err("*******get cdrx dump buffer failed!*******\n");
				return 0;
			}
			g_pmom_debug.cdrx_dump_addr        = (char*)dump_base;
			g_pmom_debug.cdrx_dump_corepm_addr = (char*)(dump_base + CDRX_DUMP_PM_OFFSET);
		}
	}
	if ((event == PM_POST_SUSPEND)&&(g_pmom_debug.cdrx_dump_addr))
	{
		/*此处用于增加CCPU的打印信息函数*/
		print_ccpu_lowpower_info();
        print_m3_lopower_info();
	}
	return 0;
}


void pm_wakeup_init(void)
{
	char* dump_base = NULL;

	wake_lock_init(&g_pmom_debug.wakelock_debug, WAKE_LOCK_SUSPEND, "cp_pm_wakeup");

	/* 即使注册icc失败(有错误打印),只影响调测,不影响功能 */
	(void)bsp_icc_event_register(ICC_CHN_IFC << 16 | IFC_RECV_FUNC_WAKEUP, \
		(read_cb_func)pm_wakeup_icc_msg, NULL, NULL, NULL);
	(void)bsp_icc_event_register(ICC_CHN_MCORE_ACORE << 16 | MCORE_ACORE_FUNC_WAKEUP, \
		(read_cb_func)pm_wakeup_icc_msg, NULL, NULL, NULL);

	g_pmom_debug.pm.notifier_call = cp_pm_notify;
	register_pm_notifier(&g_pmom_debug.pm);

	if(!g_pmom_debug.cdrx_dump_addr)
	{
    	/*获取DUMP 低功耗共享内存地址*/
		dump_base = (char *)bsp_dump_get_field_addr(DUMP_CP_DRX);
		if(NULL == dump_base)
		{
			printk(KERN_ERR"notice: cp drx dump is not ready, try again in S/R\n");
			return;
		}
        g_pmom_debug.cdrx_dump_addr        = (void*)dump_base;
		g_pmom_debug.cdrx_dump_corepm_addr = (char*)(dump_base + CDRX_DUMP_PM_OFFSET);
    }
}

static inline void* wakelock_pm_info_base_get(void)
{
	return (void *)(g_pmom_debug.cdrx_dump_addr+CDRX_DUMP_WAKELOCK_TIME_OFFSET);
}

static inline void* pm_stat_info_base_get(void)
{
	return  (void *)(g_pmom_debug.cdrx_dump_addr+PM_MODEM_LOG_ADDR_OFFSET);
}

void pm_om_help(void)
{
	pmom_print("***********************\n");
	pmom_print("pm_om_log_show\n");
	pmom_print("pm_om_debug_show\n");
	pmom_print("pm_om_dbg_on\n");
	pmom_print("pm_om_dbg_off\n");
	pmom_print("pm_om_wake_interval_set slice\n");
	pmom_print("pm_om_feature_on\n");
	pmom_print("pm_wakeup_ccore 0|1\n");
	pmom_print("debug_pm_wake_lock\n");
	pmom_print("debug_pm_wake_unlock\n");
	pmom_print("***********************\n");
}

static ssize_t pm_om_mod_sw_show(struct file *file,  char  __user *buffer, size_t count, loff_t *ppos)
{/*lint --e{715} suppress 'file' not referenced*/
    int  len;
    char buf[32]  = {0};
    u64  sw_value;
    u32  tmp_low;
    u32  tmp_high;
    ssize_t ret;
	struct pm_om_ctrl *ctrl = g_pmom_debug.ctrl;

    sw_value = ctrl->log.smem->nv_cfg.mod_sw_bitmap;
    tmp_low  = (u32)(sw_value & (u32)0xFFFFFFFF);
    tmp_high = (u32)((sw_value >> 32) & (u32)0xFFFFFFFF);

    len = snprintf(buf, (size_t)32, "0x%08x%08x\n", tmp_high, tmp_low);
	ret = simple_read_from_buffer(buffer, count, ppos, (void *)buf, (size_t)len);/*lint !e571  (Warning -- Suspicious cast)*/
	if (ret == 0)
		{
		return -EFAULT;
		}
	return ret;
}

static ssize_t get_u32_data_from_user(const char  __user *usr_buf, size_t count)
{
	char buf[32]= {0};
	u32  value  = 0;
	int  ret;

	if(count >= sizeof(buf) || usr_buf == NULL)
	{
		pmom_pr_err("input size(%zu) >= buf size(%d), usr_buf(%d)\n", count, sizeof(buf), usr_buf);
		return -EINVAL;
	}
	/* coverity[extend_simple_error] */
	// cppcheck-suppress *
	if (copy_from_user(buf, usr_buf, count))
	{
		pmom_pr_err("copy_from_user err\n");
		return -EFAULT;
	}
       // cppcheck-suppress *
	ret = sscanf(buf, "%u", &value);
	if (ret < 0)
	{
		return ret;
	}
	return value;
}

static int get_u32_seq_data_from_user(const char  __user *usr_buf, size_t usr_count, u8 *seq_data, u32 len)
{
	char buf[PM_OM_LOG_THRE_BUF_SIZE]= {0};
	int ret;

	if(usr_count >= sizeof(buf) || seq_data == NULL || len < 4 || usr_buf == NULL)
	{
		pmom_pr_err("input size(%zu) >= buf size(%d), seq_data(%p), len(%d), usr_buf(%p)\n", usr_count, sizeof(buf), seq_data, len, usr_buf);
		return -EINVAL;
	}
	/* coverity[extend_simple_error] */
	// cppcheck-suppress *
	if (copy_from_user(buf, usr_buf, usr_count))
	{
		pmom_pr_err("copy_from_user err\n");
		return -EFAULT;
	}
      // cppcheck-suppress *
	ret = sscanf(buf, "%c %c %c %c", &seq_data[0], &seq_data[1], &seq_data[2], &seq_data[3]);/* [false alarm]:fortify */
	if (ret != 4)
	{
		return  -EFAULT;
	}
	return 0;
}

/* supress pclint of list_for_each_entry */
/*lint -save -e144 -e413 -e613 -e826 -e838*/
/* 通知其他核开启log功能 */
void pm_om_notify_other_core(pm_om_icc_data_type data)
{
	s32 ret;
	u32 channel_id;
	struct pm_info_list *pm_info = NULL;
	struct pm_om_ctrl *ctrl = g_pmom_debug.ctrl;

	list_for_each_entry(pm_info, &(ctrl->pm_info.list), entry) /* [false alarm]:屏蔽Fortify错误 */
	{
		if (pm_info && pm_info->cb_func)
		{
			(void)pm_info->cb_func(&pm_info->usr_data);
			pmom_pr_err("mod[%d](%p) is invoked\n", pm_info->usr_data.mod_id, pm_info->cb_func);
		}
	}

	channel_id = PM_OM_ICC_ACORE_CHN_ID;
	ret = bsp_icc_send(ICC_CPU_MODEM, channel_id, (u8 *)&data, (u32)sizeof(data));
	if((s32)sizeof(data) != ret)
	{
		goto icc_send_fail;
	}

	/* TODO:  等M3开启后打开 */
	return;

icc_send_fail:
	pmom_pr_err("icc[0x%x] send fail: 0x%x\n", PM_OM_ICC_ACORE_CHN_ID, ret);
	return;
}
/*lint -restore +e144 +e413 +e613*/

static ssize_t pm_om_mod_on_store(struct file *file,  const char  __user *usr_buf, size_t count, loff_t *ppos)
{/*lint --e{715} suppress 'file' & 'ppos' not referenced*/
	struct pm_om_ctrl *ctrl = g_pmom_debug.ctrl;
	struct pm_om_platform *linux_plat = (struct pm_om_platform *)ctrl->platform;
	ssize_t mod_id = get_u32_data_from_user(usr_buf, count);

	if(mod_id < 0)
	{
		return mod_id;
	}
	else if(mod_id < 64)
	{
		ctrl->log.init_flag = PM_OM_INIT_MAGIC;
		linux_plat->log_info.mem_is_ok = 1;
		pm_om_log_mod_on(&(ctrl->log.smem->nv_cfg.mod_sw_bitmap), (u32)mod_id);
		pm_om_notify_other_core(1);
	}
	else if(mod_id == 64)/* enable all */
	{
		ctrl->log.init_flag = PM_OM_INIT_MAGIC;
		linux_plat->log_info.mem_is_ok = 1;
		pm_om_log_sw_set(&(ctrl->log.smem->nv_cfg.mod_sw_bitmap), 0xffffffffffffffff);
		pm_om_notify_other_core(1);
	}

	return (ssize_t)count;
}

static ssize_t pm_om_mod_off_store(struct file *fil,  const char  __user *usr_buf, size_t count, loff_t *ppos)
{/*lint --e{715} suppress 'fil' & 'ppos' not referenced*/
	struct pm_om_ctrl *ctrl = g_pmom_debug.ctrl;
	ssize_t mod_id = get_u32_data_from_user(usr_buf, count);

	if(mod_id < 0)
	{
		return mod_id;
	}
	else if(mod_id < 64)
	{
		ctrl->log.init_flag = 0;
		pm_om_log_mod_off(&(ctrl->log.smem->nv_cfg.mod_sw_bitmap), (u32)mod_id);
		pm_om_notify_other_core(0);
	}
	else if(mod_id == 64)/* disable all */
	{
		ctrl->log.init_flag = 0;
		pm_om_log_sw_set(&(ctrl->log.smem->nv_cfg.mod_sw_bitmap), (u64)0);
		pm_om_notify_other_core(0);
	}

	return (ssize_t)count;
}

static ssize_t pm_om_fwrite_trigger_on_store(struct file *file,  const char  __user *usr_buf, size_t count, loff_t *ppos)
{/*lint --e{715} suppress 'file' & 'ppos' not referenced*/
	struct pm_om_ctrl *ctrl = g_pmom_debug.ctrl;
	ssize_t sw = get_u32_data_from_user(usr_buf, count);

	if(sw < 0)
	{
		return sw;
	}
	else if(sw > 0)
	{
		ctrl->log.smem->mem_info.app_is_active = 1;
	}
	else if(sw == 0)
	{
		ctrl->log.smem->mem_info.app_is_active = 0;
	}

	return (ssize_t)count;
}

static ssize_t pm_om_fwrite_trigger_on_show(struct file *file,  char  __user *usr_buf, size_t count, loff_t *ppos)
{/*lint --e{715} suppress 'file' & 'ppos' not referenced*/
	int  len;
	char buf[32]  = {0};
	u32  sw_value;
	ssize_t ret;
	struct pm_om_ctrl *ctrl = pm_om_ctrl_get();

	sw_value = ctrl->log.smem->mem_info.app_is_active;
	len = snprintf(buf, (size_t)32, "0x%08x\n", sw_value);
	ret = simple_read_from_buffer(usr_buf, count, ppos, (void *)buf, (size_t)len);/*lint !e571  (Warning -- Suspicious cast)*/
	if (ret == 0)
		{
		return -EFAULT;
		}
	return ret;
}

static ssize_t pm_om_rb_size_store(struct file *file,  const char  __user *usr_buf, size_t count, loff_t *ppos)
{/*lint --e{715} suppress 'file' & 'ppos' not referenced*/
	struct pm_om_ctrl *ctrl = g_pmom_debug.ctrl;
	ssize_t rb_size = get_u32_data_from_user(usr_buf, count);

	if(rb_size < 0)
	{
		return rb_size;
	}
	else if(rb_size)
	{
		ctrl->log.rb.size = (u32)rb_size;
	}

	return (ssize_t)count;
}

static ssize_t pm_om_rb_info_show(struct file *file,  char  __user *usr_buf, size_t count, loff_t *ppos)
{/*lint --e{715} suppress 'file' & 'ppos' not referenced*/
	int  len;
	ssize_t ret;
	char buf[PM_OM_LOG_THRE_BUF_SIZE]  = {0};

	struct pm_om_ctrl *ctrl = g_pmom_debug.ctrl;
	struct ring_buffer *rb = &(ctrl->log.rb);
	len = snprintf(buf, (size_t)PM_OM_LOG_THRE_BUF_SIZE,
		"rb_base\t\t\trb_size\t\trb_write\trb_read\n0x%p(0x%p)\t0x%08x\t0x%08x\t0x%08x\t\n",
		(void *)rb->buf, (void *)SHD_DDR_V2P((void *)rb->buf), rb->size, rb->write, rb->read);/*lint !e834 suppress SHD_DDR_V2P warning */
	ret = simple_read_from_buffer(usr_buf, count, ppos, (void *)buf, (size_t)len);/*lint !e571  (Warning -- Suspicious cast)*/
	if (ret == 0)
		{
		return -EFAULT;
		}
	return ret;
}

static ssize_t pm_om_log_threshold_store(struct file *file,  const char  __user *usr_buf, size_t count, loff_t *ppos)
{/*lint --e{715} suppress 'file' & 'ppos' not referenced*/
	struct pm_om_ctrl *ctrl = g_pmom_debug.ctrl;
	u8 *seq_data ;
	int ret;
	u32 len = sizeof(u8) * PM_OM_LOG_THRE_CORE_NUM;

	/* coverity[extend_simple_error] */
	seq_data = kmalloc((size_t)len, GFP_KERNEL);
	if (!seq_data)
	{
		return -ENOMEM;
	}

	ret = get_u32_seq_data_from_user(usr_buf, count, seq_data, len);
	if(ret < 0)
	{
		kfree(seq_data);
		//seq_data = NULL;
		return ret;
	}
	else
	{
		/* coverity[extend_simple_error] */
		memcpy((void *)ctrl->log.smem->nv_cfg.log_threshold, (void *)seq_data, len);/*lint !e747 type coversion */
		ctrl->log.threshold = ctrl->log.smem->nv_cfg.log_threshold[PM_OM_CPUID];
	}

	kfree(seq_data);
	//seq_data = NULL;
	return (ssize_t)count;
}

static ssize_t pm_om_log_threshold_show(struct file *file,  char  __user *usr_buf, size_t count, loff_t *ppos)
{/*lint --e{715} suppress 'file' & 'ppos' not referenced*/
	int len;
	ssize_t ret;
	char buf[PM_OM_LOG_THRE_BUF_SIZE] = {0};
	struct pm_om_ctrl *ctrl = g_pmom_debug.ctrl;

	len = snprintf(buf, (size_t)PM_OM_LOG_THRE_BUF_SIZE, "%2d %2d %2d %2d\n",
		ctrl->log.smem->nv_cfg.log_threshold[0],
		ctrl->log.smem->nv_cfg.log_threshold[1],
		ctrl->log.smem->nv_cfg.log_threshold[2],
		ctrl->log.smem->nv_cfg.log_threshold[3]);
	ret = simple_read_from_buffer(usr_buf, count, ppos, (void *)buf, (size_t)len);/*lint !e571  (Warning -- Suspicious cast)*/
	if (ret == 0)
		{
		return -EFAULT;
		}
	return ret;
}

static int pm_om_wakeup_stats_show(struct seq_file *m, void *unused)
{/*lint --e{715} suppress 'unused' not referenced*/
	struct pm_om_debug *debug = &g_pmom_debug;

	seq_puts(m, "wake_times\twaket_min\twaket_max\n");
 	seq_printf(m, "%u\t\t%u\t\t%u\n", debug->stat.wakeup_cnt,debug->stat.waket_min, debug->stat.waket_max);

	return 0;
}

static int wakeup_sources_stats_open(struct inode *inode, struct file *file)
{/*lint --e{715} suppress 'inode' not referenced*/
	return single_open(file, pm_om_wakeup_stats_show, NULL);
}

/*lint -save -e785*/ /*suppress too few initializers for struct*/
static const struct file_operations pm_om_mod_on_fops = {
	.read  = pm_om_mod_sw_show,
	.write = pm_om_mod_on_store,
};

static const struct file_operations pm_om_mod_off_fops = {
	.read  = pm_om_mod_sw_show,
	.write = pm_om_mod_off_store,
};

static const struct file_operations pm_om_fwrite_trigger_fops = {
	.read  = pm_om_fwrite_trigger_on_show,
	.write = pm_om_fwrite_trigger_on_store,
};

static const struct file_operations pm_om_rb_info_fops = {
	.read  = pm_om_rb_info_show,
	.write = pm_om_rb_size_store,
};

static const struct file_operations pm_om_log_threshold_fops = {
	.read  = pm_om_log_threshold_show,
	.write = pm_om_log_threshold_store,
};

static const struct file_operations pm_om_wakeup_stats_fops = {
	.open = wakeup_sources_stats_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
/*lint -restore +e785*/
static int pm_om_debugfs_create(void)
{
	struct dentry *debugfs_root;
	/* coverity[var_deref_model] */
	debugfs_root = debugfs_create_dir("pm_om", NULL);
	if (!debugfs_root)
		return (-ENOENT);

	debugfs_create_file("mod_on", (S_IRUGO|S_IWUSR), debugfs_root, NULL, &pm_om_mod_on_fops);
	debugfs_create_file("mod_off", (S_IRUGO|S_IWUSR), debugfs_root, NULL, &pm_om_mod_off_fops);
	debugfs_create_file("fwrite_trigger", (S_IRUGO|S_IWUSR), debugfs_root, NULL, &pm_om_fwrite_trigger_fops);
	debugfs_create_file("rb_info", (S_IRUGO|S_IWUSR), debugfs_root, NULL, &pm_om_rb_info_fops);
	debugfs_create_file("log_threshold", (S_IRUGO|S_IWUSR), debugfs_root, NULL, &pm_om_log_threshold_fops);
	debugfs_create_file("wakeup_stats",(S_IRUGO|S_IWUSR), debugfs_root, NULL, &pm_om_wakeup_stats_fops);

	return 0;
}

static ssize_t pm_stat_info_get(struct device *dev, struct device_attribute *attr, char *buf)
{/*lint --e{715} suppress 'dev' & 'attr' not referenced*/
    int  len;
    u64  sw_value;
    u32  tmp_low;
    u32  tmp_high;
	struct pm_om_ctrl *ctrl = g_pmom_debug.ctrl;

    sw_value = ctrl->log.smem->nv_cfg.mod_sw_bitmap;
    tmp_low  = (u32)(sw_value & (u32)0xFFFFFFFF);
    tmp_high = (u32)((sw_value >> 32) & (u32)0xFFFFFFFF);

    len = snprintf(buf, (size_t)32, "0x%08x%08x\n", tmp_high, tmp_low);
	return len;
}

static ssize_t pm_stat_info_on_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{/*lint --e{715} suppress 'dev' & 'attr' not referenced*/
	char* endp;
	struct pm_om_ctrl *ctrl = g_pmom_debug.ctrl;
	struct pm_om_platform *linux_plat = (struct pm_om_platform *)ctrl->platform;
	int i;
	char p[200];
	char *pointer,*tempp;
	ssize_t mod_id;

	if (count >= sizeof(p) || buf == NULL)
	{
		pmom_pr_err("input buf larger than maximum value(200) or buf is NULL");
		return -EINVAL;
	}

	memset(p, 0, sizeof(p));
	memcpy(p,buf, count);
	pointer = p;

	for(i = 0; i < 64 && ((tempp = strsep(&pointer, " ")) != NULL); i++ )
	{
		if (*tempp == '\0')
		{
			continue;
		}
		mod_id = simple_strtol(tempp, &endp, 10);
		if (tempp == endp)
		{
			pmom_pr_err("input includes Non-number parameters. endp=%s\n", endp);
			return -EINVAL;
		}
		if((mod_id > (ssize_t)PM_MOD_BEGIN && mod_id < (ssize_t)PM_MOD_END)||(mod_id == (ssize_t)PM_OM_MDRV))
		{
			ctrl->log.init_flag = PM_OM_INIT_MAGIC;
			linux_plat->log_info.mem_is_ok = 1;
			pm_om_log_mod_on(&(ctrl->log.smem->nv_cfg.mod_sw_bitmap), (u32)mod_id);
		}

		else
		{
			pmom_pr_err("input number is invalid\n");
			return -EINVAL;
		}
	}

	pm_om_notify_other_core(1);
	return (ssize_t)count;
}

static ssize_t pm_stat_info_off_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{/*lint --e{715} suppress 'dev' & 'attr' not referenced*/
	char* endp;
	struct pm_om_ctrl *ctrl = g_pmom_debug.ctrl;
	int i;
	char p[200];
	char *pointer,*tempp;
	ssize_t mod_id;

	if (count >= sizeof(p) || buf == NULL)
	{
		pmom_pr_err("input buf larger than maximum value(200) or buf is NULL");
		return -EINVAL;
	}

	memset(p, 0, sizeof(p));
	memcpy(p, buf, count);
	pointer = p;

	for(i = 0; i < 64 && ((tempp = strsep(&pointer, " ")) != NULL); i++ )
	{
		if (*tempp == '\0')
		{
			continue;
		}
		mod_id = simple_strtol(tempp, &endp, 10);
		if (tempp == endp)
		{
			pmom_pr_err("input includes Non-number parameters. endp=%s\n", endp);
			return -EINVAL;
		}
		if((mod_id > (ssize_t)PM_MOD_BEGIN && mod_id < (ssize_t)PM_MOD_END)||(mod_id == (ssize_t)PM_OM_MDRV))
		{
			ctrl->log.init_flag = 0;
			pm_om_log_mod_off(&(ctrl->log.smem->nv_cfg.mod_sw_bitmap), (u32)mod_id);
		}

		else
		{
			pmom_pr_err("input number is invalid\n");
			return -EINVAL;
		}
	}

	pm_om_notify_other_core(1);
	return (ssize_t)count;
}

/*lint -save -e846 -e514 -e778 -e866 -e84 suppress DEVICE_ATTR warning*/
static DEVICE_ATTR(mod_on,  0600, pm_stat_info_get, pm_stat_info_on_set);
static DEVICE_ATTR(mod_off, 0600, pm_stat_info_get, pm_stat_info_off_set);
/*lint -restore +e846 +e514 +e778 +e866 +e84*/
static struct attribute *pm_stat_info_on_attributes[] = {
        &dev_attr_mod_on.attr,
        NULL
};
/*lint -save -e785*/ /*suppress too few initializers for struct*/
static const struct attribute_group pm_stat_info_on_group = {
    .attrs = pm_stat_info_on_attributes,
};

static struct attribute *pm_stat_info_off_attributes[] = {
        &dev_attr_mod_off.attr,
        NULL
};

static const struct attribute_group pm_stat_info_off_group = {
    .attrs = pm_stat_info_off_attributes,
};

static int __init pm_info_probe(struct platform_device *pdev)
{
    int ret = 0;

    ret |= device_create_file(&(pdev->dev), &dev_attr_mod_on);
    ret |= device_create_file(&(pdev->dev), &dev_attr_mod_off);
    if (ret)
    {
        printk("fail to creat balong modem pm info sysfs\n");
        return ret;
    }

    return ret;
}

static struct platform_device balong_modem_pm_info_device = {
    .name = "balong_modem_pm_info",
    .id = 0,
    .dev = {
    .init_name = "balong_modem_pm_info",
    },
};

static struct platform_driver balong_modem_pm_info_drv = {
    .probe      = pm_info_probe,
    .driver     = {
        .name     = "balong_modem_pm_info",
        .owner    = THIS_MODULE,
    },
};
/*lint -restore +e785*/

static int __init balong_modem_pm_info_init(void)
{
    int ret;

    ret = platform_device_register(&balong_modem_pm_info_device);
    if(ret)
    {
        printk("register balong modem info device fail\n");
        return ret;
    }

    ret = platform_driver_register(&balong_modem_pm_info_drv);
    if(ret)
    {
        printk("register balong modem info driver fail\n");
        platform_device_unregister(&balong_modem_pm_info_device);
    }

    return ret;
}

static void __exit balong_modem_pm_info_exit(void)
{
    platform_driver_unregister(&balong_modem_pm_info_drv);
    platform_device_unregister(&balong_modem_pm_info_device);
}

module_init(balong_modem_pm_info_init);
module_exit(balong_modem_pm_info_exit);

int pm_om_debug_init(void)
{
	struct pm_om_ctrl *ctrl = pm_om_ctrl_get();

	memset((void *)&g_pmom_debug, 0, sizeof(g_pmom_debug));

	g_pmom_debug.stat.waket_prev    = bsp_get_slice_value();
	g_pmom_debug.stat.waket_min     = 0xffffffff;
	g_pmom_debug.stat.logt_print_sw = ctrl->log.smem->nv_cfg.reserved;

	g_pmom_debug.ctrl = ctrl;
	ctrl->debug = (void *)&g_pmom_debug;

	pm_wakeup_init();

	pm_om_debugfs_create();

	return 0;
}

#define TEST_BUF_LEN   16
#define CB_INIT_FLAG  0xaa000000

char    test_data1[TEST_BUF_LEN] = {0x0};
char    test_data2[TEST_BUF_LEN] = {0x0};

struct pm_info_usr_data usr_data1 = {PM_OM_PMLG, PM_OM_MAGIC_PMLG, test_data1, TEST_BUF_LEN, NULL};
struct pm_info_usr_data usr_data2 = {PM_OM_TCXO, PM_OM_MAGIC_TCXO, test_data2, TEST_BUF_LEN, NULL};

int pm_info_cb_common(struct pm_info_usr_data *usr_data, u32 test_value)
{/*lint --e{713,747} suppress memset warning*/
	(void)memset_s(usr_data->buf, usr_data->buf_len, test_value, usr_data->buf_len);
	return bsp_pm_log(usr_data->mod_id, usr_data->buf_len, usr_data->buf);
}

int pm_info_cb1(struct pm_info_usr_data *usr_data)
{
	static u32 cb1_init_flag = CB_INIT_FLAG;
	cb1_init_flag++;

	return pm_info_cb_common(usr_data, cb1_init_flag);
}

int pm_info_cb2(struct pm_info_usr_data *usr_data)
{
	static u32 cb2_init_flag = CB_INIT_FLAG;
	cb2_init_flag++;

	return pm_info_cb_common(usr_data, cb2_init_flag);
}
void pm_info_reg_test(void)
{
	(void)bsp_pm_info_stat_register((pm_info_cbfun)pm_info_cb1, &usr_data1);
	(void)bsp_pm_info_stat_register((pm_info_cbfun)pm_info_cb2, &usr_data2);
}

EXPORT_SYMBOL(pm_om_notify_other_core);
EXPORT_SYMBOL(pm_wakeup_ccore);
