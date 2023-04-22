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
/******************************************************************************
  File Name       : modem_avs.c
  Description     : calc modem avs voltage
  History         :
******************************************************************************/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <bsp_trace.h>
#include <bsp_nvim.h>
#include <bsp_regulator.h>
#include <bsp_sysctrl.h>
#include <bsp_shared_ddr.h>
#include <drv_nv_id.h>
#include <drv_nv_def.h>


/*lint --e{438, 754, 528, 529, 838, 732, 50, 58} */
#define HPM_SAMPLE_LOOP		16
/*lint -e750 -esym(750,*) */
#define DVS_HPM_CLK_EN_OFFSET            (0x0)
#define DVS_HPM_CLK_DIS_OFFSET           (0x4)
#define DVS_HPM_CLK_STAT_OFFSET          (0x8)
#define DVS_HPM_CLK_SRSTEN_OFFSET        (0x20)
#define DVS_HPM_CLK_SRSTDIS_OFFSET       (0x24)
#define DVS_HPM_SC_CTRL_OFFSET           (0x444)
#define DVS_HPM_SC_STAT_OFFSET           (0x60C)
/*lint -e750 +esym(750,*) */
#define DVS_HPM_VOLT_UV_TO_MV            (1000)

#define ABSOLUTE_SUB(a,b)	(((a) > (b)) ? ((a) - (b)) : ((b) - (a)))

struct avs_ctrl_s{
	u32 enable_flag;
	u32 avs_volt_num;
	struct regulator *avs_regulator;
	u32 *avs_volt_list;
};
struct avs_ctrl_s g_modem_avs_ctrl = {0,0,NULL,NULL};
/*
 * hpm_sel = 0 ·µ»Øhpm, =1·µ»Øhpmx
 */
s32 modem_avs_get_hpm(s32 hpm_sel)
{
	u32 tmp = 0;
	s32 temp = 0;
	u32 hpmx_value = 0;
	u32 hpm_value = 0;
	u32 timeout = 100;
	u32 peri_crg = 0;
	void * sysctrl_hpm_addr = bsp_sysctrl_addr_byindex(sysctrl_mdm);

	peri_crg = readl(sysctrl_hpm_addr + DVS_HPM_CLK_STAT_OFFSET);
	//ENABLE PERI_CLK FOR HPM_CLK
	if (!((peri_crg >> 29) & 0x1))
	{
		writel(0x20000000, sysctrl_hpm_addr+ DVS_HPM_CLK_EN_OFFSET);
	}
	else
	{
		;
	}
	/*ENABLE HPM CLK*/
	writel(0x80, sysctrl_hpm_addr+ DVS_HPM_CLK_EN_OFFSET);

	/*ENABLE/DISABLE HPM SRST*/
	writel(0x80000000, sysctrl_hpm_addr + DVS_HPM_CLK_SRSTEN_OFFSET);
	writel(0x80000000, sysctrl_hpm_addr + DVS_HPM_CLK_SRSTDIS_OFFSET);
	/* enable HPM */
	writel(0xf, sysctrl_hpm_addr + DVS_HPM_SC_CTRL_OFFSET);
	writel(0x18f, sysctrl_hpm_addr + DVS_HPM_SC_CTRL_OFFSET);

	/* wait HPM valid */
	while(((readl(sysctrl_hpm_addr + DVS_HPM_SC_STAT_OFFSET) & 0x400400) != 0x400400) && (timeout > 0))
	{
		/*lint --e{778, 774, 747} */
		udelay(1);
		timeout--;
	}

	/* 18 get hpm0/hpm1 opc */
	/* read opc twice, use the value of the second time */
	tmp = readl(sysctrl_hpm_addr + DVS_HPM_SC_STAT_OFFSET);
	tmp = readl(sysctrl_hpm_addr + DVS_HPM_SC_STAT_OFFSET);
	hpmx_value = (tmp >> 0) & (0x3FF);
	hpm_value = (tmp >> 12) & (0x3FF);

	/*DISABLE HPM CLK*/
	writel(0x80, sysctrl_hpm_addr + DVS_HPM_CLK_DIS_OFFSET);
	if (!((peri_crg >> 29) & 0x1))
	{
		writel(0x20000000, sysctrl_hpm_addr + DVS_HPM_CLK_DIS_OFFSET);
	}
	else
	{
		;
	}
	temp = hpm_sel ? (int)hpmx_value : (int)hpm_value;
	if (0 == timeout)
		temp = 0;
	avs_warn("hpmx_value: %d, hpm_value: %d\n", hpmx_value, hpm_value);
	return temp;
}

static s32 modem_avs_set_volt(u32 vol)
{
	return regulator_set_voltage(g_modem_avs_ctrl.avs_regulator, (int)vol, (int)vol);
}

static unsigned int hpm_code_sample(s32 hpm_sel)
{
	unsigned int sum = 0;
	unsigned int mean = HPM_SAMPLE_LOOP;
	unsigned int hpm_value;

	/* read hpm value */
	while (mean--) {
		sum += (unsigned int)modem_avs_get_hpm(hpm_sel);
	}

	hpm_value = sum / HPM_SAMPLE_LOOP;

	return hpm_value;
}
static unsigned int new_hpm_trim(unsigned int target_hpm, s32 hpm_sel)
{
	unsigned int last_hpm, post_hpm_delta;
	unsigned int hpm = 0;
	unsigned int hpm_delta = target_hpm;
	unsigned int uplimit = g_modem_avs_ctrl.avs_volt_num - 1;
	unsigned int dnlimit = 0;
	unsigned int vol = uplimit;
	unsigned int last_vol = vol;

	if (!target_hpm)
	{
		return ~0;
	}
	do {
		post_hpm_delta = hpm_delta;
		if (modem_avs_set_volt(g_modem_avs_ctrl.avs_volt_list[vol] * DVS_HPM_VOLT_UV_TO_MV))
			return ~0;
		last_hpm = hpm;
 		hpm = hpm_code_sample(hpm_sel);
 		if (!hpm)
 		{
 			avs_err("get hpm failed\n");
			return ~0;
 		}
		avs_warn("tarhpm:%d, vol:%d, hpm:%d, sel:%d\n", target_hpm, vol, hpm, hpm_sel);

		hpm_delta = ABSOLUTE_SUB(hpm, target_hpm);
		if (hpm_delta >= post_hpm_delta)
			break;

		last_vol = vol;
		if (hpm < target_hpm) {
			if (vol < uplimit)
				vol++;
		} else if (hpm > target_hpm) {
			if (vol > dnlimit)
				vol--;
		}
	} while (hpm_delta != 0);

	if (hpm_delta > post_hpm_delta) {
		vol = last_vol;
		hpm = last_hpm;
		hpm_delta = post_hpm_delta;
	}

	if (hpm_delta > 32) {
		avs_err("hpm divergent:%d\n", hpm_delta);
		return ~0;
	}

	last_vol = vol;
	avs_warn("hpm:%d\t 0.%d mV\n", hpm, g_modem_avs_ctrl.avs_volt_list[last_vol]);
	return g_modem_avs_ctrl.avs_volt_list[vol] ;
}

u32 modem_avs_get_realvolt(u32 para_A, u32 para_B, u32 readvolt)
{
	u32 tmp = 0;
	u32 uplimit = g_modem_avs_ctrl.avs_volt_num;
	u32 para_A_flag = para_A & 0x40000000;
	u32 para_a = para_A & 0x3FFFFFFF;
	u32 tmp_avs_volt = para_A_flag ? ((para_B * readvolt - 1000 * para_a) / 10000) : ((para_B * readvolt + 1000 * para_a) / 10000);
	for (tmp = 0; tmp < uplimit; tmp++)
	{
		if (tmp_avs_volt <= g_modem_avs_ctrl.avs_volt_list[tmp])
		{
			tmp_avs_volt = g_modem_avs_ctrl.avs_volt_list[tmp];
			break;
		}
	}
	if (tmp == uplimit)
	{
		tmp_avs_volt = g_modem_avs_ctrl.avs_volt_list[uplimit - 1];
	}
	return tmp_avs_volt;
}
void modem_avs_trim(struct avs_config_str *avs_config, u32 avs_num)
{
	u32 avs_hpm_volt = 0;
	u32 avs_hpmx_volt = 0;
	u32 tmp = 0;
	if (!g_modem_avs_ctrl.enable_flag)
		return;

	for (tmp = 0; tmp < avs_num; tmp++)
	{
		avs_config[tmp].hpm.core_read_volt = (int)new_hpm_trim(avs_config[tmp].hpm.core_target_hpm, 0);
		if ((~0) == avs_config[tmp].hpm.core_read_volt)
		{
			avs_config[tmp].core_avs_volt = 0;
			continue;
		}
		avs_hpm_volt = modem_avs_get_realvolt((u32)avs_config[tmp].hpm.core_para_A, (u32)avs_config[tmp].hpm.core_para_B, (u32)avs_config[tmp].hpm.core_read_volt);

		avs_config[tmp].hpmx.core_read_volt = (int)new_hpm_trim(avs_config[tmp].hpmx.core_target_hpm, 1);
		if ((~0) == avs_config[tmp].hpmx.core_read_volt)
		{
			avs_config[tmp].core_avs_volt = 0;
			continue;
		}
		avs_hpmx_volt = modem_avs_get_realvolt((unsigned int)avs_config[tmp].hpmx.core_para_A, (unsigned int)avs_config[tmp].hpmx.core_para_B, (u32)avs_config[tmp].hpmx.core_read_volt);
		avs_err("tmp:%d, avs hpm volt:%d, voltx:%d\n", tmp, avs_hpm_volt, avs_hpmx_volt);
		avs_config[tmp].core_avs_volt = (avs_hpm_volt > avs_hpmx_volt) ? (avs_hpm_volt * DVS_HPM_VOLT_UV_TO_MV) : (avs_hpmx_volt * DVS_HPM_VOLT_UV_TO_MV);
		if (avs_config[tmp].core_avs_volt > avs_config[tmp].core_max_volt)
		{
			avs_config[tmp].core_avs_volt = avs_config[tmp].core_max_volt;
		}
		else if (avs_config[tmp].core_avs_volt < avs_config[tmp].core_min_volt)
		{
			avs_config[tmp].core_avs_volt = avs_config[tmp].core_min_volt;
		}
		else
		{
		}
	}
}

void modem_avs_get_hpm_config(struct device_node *node, u32 *avs_tmp, struct avs_config_str *avs_config, u32 avs_num)
{
	u32 tmp = 0;
	if(!of_property_read_u32_array(node, "hisi_avs_target_hpm", avs_tmp, (unsigned long)avs_num))
	{
		for (tmp = 0; tmp < avs_num; tmp++)
		{
			avs_config[tmp].hpm.core_target_hpm = avs_tmp[tmp];
		}
	}
	if(!of_property_read_u32_array(node, "hisi_avs_para_A", avs_tmp, (unsigned long)avs_num))
	{
		for (tmp = 0; tmp < avs_num; tmp++)
		{
			avs_config[tmp].hpm.core_para_A = (int)avs_tmp[tmp];
		}
	}
	if(!of_property_read_u32_array(node, "hisi_avs_para_B", avs_tmp, (unsigned long)avs_num))
	{
		for (tmp = 0; tmp < avs_num; tmp++)
		{
			avs_config[tmp].hpm.core_para_B = (int)avs_tmp[tmp];
		}
	}
	if(!of_property_read_u32_array(node, "hisi_avs_target_hpmx", avs_tmp, (unsigned long)avs_num))
	{
		for (tmp = 0; tmp < avs_num; tmp++)
		{
			avs_config[tmp].hpmx.core_target_hpm = avs_tmp[tmp];
		}
	}
	if(!of_property_read_u32_array(node, "hisi_avs_para_Ax", avs_tmp, (unsigned long)avs_num))
	{
		for (tmp = 0; tmp < avs_num; tmp++)
		{
			avs_config[tmp].hpmx.core_para_A = (int)avs_tmp[tmp];
		}
	}
	if(!of_property_read_u32_array(node, "hisi_avs_para_Bx", avs_tmp, (unsigned long)avs_num))
	{
		for (tmp = 0; tmp < avs_num; tmp++)
		{
			avs_config[tmp].hpmx.core_para_B = (int)avs_tmp[tmp];
		}
	}
}

void modem_avs_get_common_config(struct device_node *node, u32 *avs_tmp, struct avs_config_str *avs_config, u32 avs_num)
{
	u32 tmp = 0;
	if(!of_property_read_u32_array(node, "hisi_avs_core_type", avs_tmp, (unsigned long)avs_num))
	{
		for (tmp = 0; tmp < avs_num; tmp++)
		{
			avs_config[tmp].core_type = (enum avs_core_type)avs_tmp[tmp];
		}
	}
	if(!of_property_read_u32_array(node, "hisi_avs_dstfreq", avs_tmp, (unsigned long)avs_num))
	{
		for (tmp = 0; tmp < avs_num; tmp++)
		{
			avs_config[tmp].core_freq = avs_tmp[tmp];
		}
	}
	if(!of_property_read_u32_array(node, "hisi_avs_dstvolt", avs_tmp, (unsigned long)avs_num))
	{
		for (tmp = 0; tmp < avs_num; tmp++)
		{
			avs_config[tmp].core_normal_volt = avs_tmp[tmp] * DVS_HPM_VOLT_UV_TO_MV;
		}
	}
	if(!of_property_read_u32_array(node, "hisi_avs_maxvolt", avs_tmp, (unsigned long)avs_num))
	{
		for (tmp = 0; tmp < avs_num; tmp++)
		{
			avs_config[tmp].core_max_volt = avs_tmp[tmp];
		}
	}
	if(!of_property_read_u32_array(node, "hisi_avs_minvolt", avs_tmp, (unsigned long)avs_num))
	{
		for (tmp = 0; tmp < avs_num; tmp++)
		{
			avs_config[tmp].core_min_volt = avs_tmp[tmp];
		}
	}
}
/*lint --e{785} */
static const struct of_device_id hisi_avs_match[] = {
	{ .compatible = "hisilicon,avs_balong",},
	{},
};

static int __init his_modem_probe(struct platform_device *pdev)
{
    u32 ret_value = 0;
    u32 *avs_tmp = NULL;
    u32 avs_num = 0;
    u32 volt_num = 0;
    struct device *dev = &pdev->dev;
    struct device_node *node = NULL;
    ST_DVS_CONFIG_STRU dvs_nv_ctrl = {0, 0};

    u32 *avs_magic = (u32 *)((unsigned long)SHM_BASE_ADDR + SHM_OFFSET_MODEM_AVS);
    u32 *avs_core_num = (u32 *)((unsigned long)SHM_BASE_ADDR + SHM_OFFSET_MODEM_AVS + 0x4);
    struct avs_config_str *avs_config = (struct avs_config_str *)((unsigned long)SHM_BASE_ADDR + SHM_OFFSET_MODEM_AVS + 0x8);

    avs_warn( "avs ......init\n");
    *avs_magic = 0xfa0000a0;
    ret_value = bsp_nvm_read(NV_ID_DRV_DVS_CONFIG, (u8*)&dvs_nv_ctrl, (unsigned int)sizeof(ST_DVS_CONFIG_STRU));
    if (NV_OK == ret_value)
    {
    	g_modem_avs_ctrl.enable_flag = dvs_nv_ctrl.dvs_en;
    }
    else
    {
    	g_modem_avs_ctrl.enable_flag = 0;
    	*avs_magic = 0xfa0000a1;
    	return 0;
    }
    node = of_find_compatible_node(NULL, NULL, "hisilicon,avs_balong");
    if (!node)
    {
    	*avs_magic = 0xfa0000a2;
    	return 0;
    }
    (void)of_property_read_u32_array(node, "hisi_avs_core_num", &avs_num, (unsigned long)1);
    if (!avs_num)
    {
    	*avs_magic = 0xfa0000a3;
    	return 0;
    }
    *avs_core_num = avs_num;
    avs_tmp = (u32 *)kmalloc(sizeof(u32) * avs_num,GFP_KERNEL);
    if (NULL == avs_tmp)
    {
    	*avs_magic = 0xfa0000a4;
    	return 0;
    }
    modem_avs_get_common_config(node, avs_tmp, avs_config, avs_num);
    modem_avs_get_hpm_config(node, avs_tmp, avs_config, avs_num);
    (void)of_property_read_u32_array(node, "hisi_avs_volt_num", &volt_num, (unsigned long)1);
    if (!volt_num)
    {
    	kfree(avs_tmp);
    	*avs_magic = 0xfa0000a5;
    	return 0;
    }
    g_modem_avs_ctrl.avs_volt_list = (u32 *)kmalloc(sizeof(u32) * volt_num,GFP_KERNEL);
    if (NULL == g_modem_avs_ctrl.avs_volt_list)
    {
    	kfree(avs_tmp);
    	*avs_magic = 0xfa0000a6;
    	return 0;
    }
    g_modem_avs_ctrl.avs_volt_num = volt_num;
    (void)of_property_read_u32_array(node, "hisi_avs_volt_list", g_modem_avs_ctrl.avs_volt_list, (unsigned long)volt_num);
    //regu_name = of_get_property(node, "hisi_avs_regulator_name", NULL);
    g_modem_avs_ctrl.avs_regulator = regulator_get(dev, "hisi_avs-vdd");
    if (IS_ERR(g_modem_avs_ctrl.avs_regulator))
    {
    	kfree(avs_tmp);
    	kfree(g_modem_avs_ctrl.avs_volt_list);
    	*avs_magic = 0xfa0000a7;
    	return 0;
    }
    modem_avs_trim(avs_config, avs_num);
    *avs_magic = AVS_MAGIC_NUM;
    avs_err( "avs init ok..\n");
    kfree(avs_tmp);
    kfree(g_modem_avs_ctrl.avs_volt_list);

    return 0;
}

/*lint --e{785, 64} */
static struct platform_driver his_modem_avs_drv = {
    .probe      = his_modem_probe,
    .driver     = {
        .name     = "his_modem_avs",
        .owner    = THIS_MODULE,
        .of_match_table = of_match_ptr(hisi_avs_match),
    },
};

static int __init his_modem_init_driver(void)
{
    int ret = 0;

    ret = platform_driver_register(&his_modem_avs_drv);
    if(ret)
    {
        printk(KERN_ERR "register his_modem_avs driver failed\n");
    }

    return ret;
}

static void __exit his_modem_exit_driver(void)
{
    regulator_put(g_modem_avs_ctrl.avs_regulator);
    platform_driver_unregister(&his_modem_avs_drv);
}

/* arch_initcall(his_modem_init_driver);*/
module_init(his_modem_init_driver);
module_exit(his_modem_exit_driver);

void avs_print_debug(void)
{
	u32 tmp = 0;
	u32 *avs_core_num = (u32 *)((unsigned long)SHM_BASE_ADDR + SHM_OFFSET_MODEM_AVS + 0x4);
	struct avs_config_str *avs_config = (struct avs_config_str *)((unsigned long)SHM_BASE_ADDR + SHM_OFFSET_MODEM_AVS + 0x8);
	avs_err("avs volt,	avs freq,	tar hpm,	read volt,	para_a,	tar hpmx,	read voltx,	para_ax\n");
	for (tmp = 0; tmp < *avs_core_num; tmp++)
	{
		avs_err("%d,	%d,	%d,	%d,	%d,	%d,	%d,	%d\n",avs_config[tmp].core_avs_volt,avs_config[tmp].core_freq,avs_config[tmp].hpm.core_target_hpm, \
										avs_config[tmp].hpm.core_read_volt,avs_config[tmp].hpm.core_para_A,avs_config[tmp].hpmx.core_target_hpm, \
										avs_config[tmp].hpmx.core_read_volt,avs_config[tmp].hpmx.core_para_A);
	}
}

