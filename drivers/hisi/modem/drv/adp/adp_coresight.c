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

#include <product_config.h>

#include <osl_types.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <drv_nv_def.h>
#include <drv_nv_id.h>
#include <of.h>
#include <bsp_om_enum.h>
#include <bsp_coresight.h>
#include <bsp_dump.h>
#include <bsp_slice.h>
#include <bsp_nvim.h>
#include <bsp_rfile.h>
#include <bsp_version.h>


#define CPU_NUMS                         (4)



#define BIT(nr)                         (1UL << (nr))
#define BVAL(val, n)                    (((val) & BIT(n)) >> n)

#define CS_ETM_LOCK(addr)                       \
do {                                    \
    writel(0x0, addr+MDM_CORESIGHT_LAR);            \
} while (0)
#define CS_ETM_UNLOCK(addr)                     \
do {                                    \
    writel(MDM_CORESIGHT_UNLOCK,addr+MDM_CORESIGHT_LAR);   \
} while (0)

struct mdmcp_coresight_device_info
{
    void* etm_base;
    unsigned long etm_phy;
    void* etm4x_base;
    unsigned long etm4x_phy;
    void* tmc_base;
    unsigned long tmc_phy;
    struct coresight_etb_data_head_info * etb_buf;
    const char* etm_name;
    const char* etm4x_name;
    const char* tmc_name;
};
struct mdmcp_coresight_device_info g_mdmcp_coresight[CPU_NUMS];


#include <bsp_sysctrl.h>
typedef struct modem_sys_cfg
{
    void*           mdm_ctrl_sys_virt_addr;
    unsigned long   mdm_ctrl_sys_phy_addr;
    unsigned long   mdm_ctrl_sys_mem_size;
    struct
    {
        u32 offset;
        u32 mdm_dbg_clk_status;
        u32 mdm_pd_clk_status;
    }clk;
    struct
    {
        u32 offset;
        u32 mdm_pd_srst_status;
        u32 mdm_cpu_srst_status;
    }rst;
    struct
    {
        u32 offset;
        u32 mdm_mtcmos_strl_status;
    }mtcmos;
}modem_sysctrl_cfg_t;

typedef struct perctrl2
{
    void* virt_addr;
    u32 offset;
    u32 pclkdbg_clkoff_sys;           /*bit 1*/
    u32 atclkoff_sys;                 /*bit 7*/
    u32 pclkdbg_to_modem_clk_off_sys; /*bit 17*/
    u32 atclk_to_modem_clkoff_sys;    /*bit 16*/
    u32 modem_cssys_rst_req;          /*bit 18*/
}PERCTRL2_REG;

modem_sysctrl_cfg_t  g_modem_sysctrl_cfg;
PERCTRL2_REG g_perctrl2_reg = {0,};


int  mdmcp_debug_perctrl2_init(void)
{
    char* name = "coresight,extern_dep" ;
    struct device_node* dev_node;

    dev_node = of_find_compatible_node(NULL, NULL, name);
    if(NULL == dev_node)
    {
        return 0;
    }
    g_perctrl2_reg.virt_addr= of_iomap(dev_node,0);

    (void)of_property_read_u32_index(dev_node, "offset", 0,&g_perctrl2_reg.offset);
    (void)of_property_read_u32_index(dev_node, "pclkdbg_clkoff_sys", 0,&g_perctrl2_reg.pclkdbg_clkoff_sys);
    (void)of_property_read_u32_index(dev_node, "atclkoff_sys", 0,&g_perctrl2_reg.atclkoff_sys);
    (void)of_property_read_u32_index(dev_node, "pclkdbg_to_modem_clk_off_sys", 0,&g_perctrl2_reg.pclkdbg_to_modem_clk_off_sys);
    (void)of_property_read_u32_index(dev_node, "atclk_to_modem_clkoff_sys", 0,&g_perctrl2_reg.atclk_to_modem_clkoff_sys);
    (void)of_property_read_u32_index(dev_node, "modem_cssys_rst_req", 0,&g_perctrl2_reg.modem_cssys_rst_req);

    return 0;

}

int  mdmcp_debug_get_perctrl2_status(void)
{
    u32 status;
    if(g_perctrl2_reg.virt_addr == NULL)
    {
        printk(KERN_ERR"base addr is  null.\r\n");
        return 0;
    }
    status = (u32)readl(g_perctrl2_reg.virt_addr + g_perctrl2_reg.offset);
    printk("status = 0x%x \n",status);
    if(!BVAL(status, g_perctrl2_reg.pclkdbg_clkoff_sys))
    {
        printk(KERN_ERR"g_perctrl2_reg.pclkdbg_clkoff_sys is 0\n");
        return -1;
    }
    if(!BVAL(status, g_perctrl2_reg.atclkoff_sys))
    {
        printk(KERN_ERR"g_perctrl2_reg.atclkoff_sys is 0\n");
        return -1;
    }
    if(!BVAL(status, g_perctrl2_reg.pclkdbg_to_modem_clk_off_sys))
    {
        printk(KERN_ERR"g_perctrl2_reg.pclkdbg_to_modem_clk_off_sys is 0\n");
        return -1;
    }
    if(!BVAL(status, g_perctrl2_reg.atclk_to_modem_clkoff_sys))
    {
        printk(KERN_ERR"g_perctrl2_reg.atclk_to_modem_clkoff_sys is 0\n");
        return -1;
    }
    if(BVAL(status, g_perctrl2_reg.modem_cssys_rst_req))
    {
        printk(KERN_ERR"g_perctrl2_reg.modem_cssys_rst_req is 1\n");
        return -1;
    }
    return 0;

}

int  mdmcp_debug_sysctrl_init(void)
{
    struct device_node* sysctrl_node;
    unsigned int reg_data[2] = {0,};
    unsigned int rst_data[3] = {0,};
    char* name = "ap_modem,sysctrl_cfg";
    unsigned long sz;
    memset(&g_modem_sysctrl_cfg,'\0',sizeof(g_modem_sysctrl_cfg));
    sysctrl_node = of_find_compatible_node(NULL, NULL, name);
    if(sysctrl_node)
    {
        sz = 2;
        if(of_property_read_u32_array(sysctrl_node, "reg", reg_data, sz))
        {
            printk(KERN_ERR" cs get dts reg error\n");
            return -1;
        }
        g_modem_sysctrl_cfg.mdm_ctrl_sys_phy_addr  = (reg_data[0]);
        g_modem_sysctrl_cfg.mdm_ctrl_sys_mem_size  = reg_data[1];
        g_modem_sysctrl_cfg.mdm_ctrl_sys_virt_addr = bsp_sysctrl_addr_get((void*)g_modem_sysctrl_cfg.mdm_ctrl_sys_phy_addr);
        sz = 3;
        if(of_property_read_u32_array(sysctrl_node, "clk", rst_data, sz))
        {
            memset(&g_modem_sysctrl_cfg,0,sizeof(g_modem_sysctrl_cfg));
            printk(KERN_ERR" cs get dts clk error\n");
            return -1;
        }
        g_modem_sysctrl_cfg.clk.offset = (rst_data[0]);
        g_modem_sysctrl_cfg.clk.mdm_dbg_clk_status = (rst_data[1]);
        g_modem_sysctrl_cfg.clk.mdm_pd_clk_status = (rst_data[2]);

        memset(rst_data,0,sizeof(rst_data));
        sz = 3;
        if(of_property_read_u32_array(sysctrl_node, "reset", rst_data, sz))
        {
            memset(&g_modem_sysctrl_cfg,0,sizeof(g_modem_sysctrl_cfg));
            printk(KERN_ERR" cs get dts reset error\n");
            return -1;
        }

        g_modem_sysctrl_cfg.rst.offset = (rst_data[0]);
        g_modem_sysctrl_cfg.rst.mdm_pd_srst_status = (rst_data[1]);
        g_modem_sysctrl_cfg.rst.mdm_cpu_srst_status = (rst_data[2]);

        sz = 2;
        if(of_property_read_u32_array(sysctrl_node, "mtcmos", reg_data, sz))
        {
            memset(&g_modem_sysctrl_cfg,0,sizeof(g_modem_sysctrl_cfg));
            printk(KERN_ERR" cs get dts tcmos error\n");
            return -1;
        }
        g_modem_sysctrl_cfg.mtcmos.offset = (reg_data[0]);
        g_modem_sysctrl_cfg.mtcmos.mdm_mtcmos_strl_status = (reg_data[1]);
    }

    return 0;
}

int  mdmcp_debug_get_sysctrl_status(void)
{
    u32 clk;
    u32 rst;
    u32 tcmos;

    if(g_modem_sysctrl_cfg.mdm_ctrl_sys_virt_addr == NULL)
    {
        printk(KERN_ERR"sys ctrl base addr is null\n");
        return 0;
    }

    clk = (u32)readl(g_modem_sysctrl_cfg.mdm_ctrl_sys_virt_addr + g_modem_sysctrl_cfg.clk.offset);
    rst = (u32)readl(g_modem_sysctrl_cfg.mdm_ctrl_sys_virt_addr + g_modem_sysctrl_cfg.rst.offset);
    tcmos = (u32)readl(g_modem_sysctrl_cfg.mdm_ctrl_sys_virt_addr + g_modem_sysctrl_cfg.mtcmos.offset);

    printk(KERN_ERR"clk = %x,rst = %x,tcmos = %x\n",clk,rst,tcmos);

    if(!BVAL(clk, g_modem_sysctrl_cfg.clk.mdm_dbg_clk_status))
    {
        printk(KERN_ERR"mdm_dbg_clk_status is disable,mdm_dbg_clk_status=0x%x status=0x%x\n",g_modem_sysctrl_cfg.clk.mdm_dbg_clk_status,clk);
        return -1;
    }
    if(!BVAL(clk, g_modem_sysctrl_cfg.clk.mdm_pd_clk_status))
    {
        printk(KERN_ERR"mdm_pd_clk_status is disable,bit=0x%x status=0x%x\n",g_modem_sysctrl_cfg.clk.mdm_pd_clk_status,clk);
        return -1;
    }
    if(BVAL(rst, g_modem_sysctrl_cfg.rst.mdm_pd_srst_status))
    {
        printk(KERN_ERR"mdm_pd_srst_status is disable,bit=0x%x status =0x%x\n",g_modem_sysctrl_cfg.rst.mdm_pd_srst_status,rst);
        return -1;
    }
    if(BVAL(rst, g_modem_sysctrl_cfg.rst.mdm_cpu_srst_status))
    {
        printk(KERN_ERR"mdm_cpu_srst_status is disable,bit=0x%x status =0x%x\n",g_modem_sysctrl_cfg.rst.mdm_cpu_srst_status,rst);
        return -1;
    }
    if(!BVAL(tcmos, g_modem_sysctrl_cfg.mtcmos.mdm_mtcmos_strl_status))
    {
        printk(KERN_ERR"mdm_cpu_srst_status is disable,bit=0x%x status = 0x%x",g_modem_sysctrl_cfg.mtcmos.mdm_mtcmos_strl_status,tcmos);
        return -1;
    }
    return 0;
}


static void* get_mdmcp_etb_buf(u32 cpu)
{
    void* addr;
    switch(cpu){
        case 0:
            addr   = (void*)bsp_dump_get_field_addr(CP_TRACE_ID(0));
            break;
        case 1:
            addr   = (void*)bsp_dump_get_field_addr(CP_TRACE_ID(1));
            break;
        case 2:
            addr   = (void*)bsp_dump_get_field_addr(CP_TRACE_ID(2));
            break;
        case 3:
            addr   = (void*)bsp_dump_get_field_addr(CP_TRACE_ID(3));
            break;
        default:
            return NULL;
    }
    return addr;
}

static void mdmcp_coresight_stop_etm(u32 cpu)
{
    void* etm_base;
    uint32_t etmcr;
    int count;

    if(!g_mdmcp_coresight[cpu].etm_base)
        return;

    etm_base = g_mdmcp_coresight[cpu].etm_base;

    CS_ETM_UNLOCK(etm_base);

    etmcr = (uint32_t)readl(etm_base+MDM_ETMCR);
    etmcr |= BIT(10);
    writel(etmcr,etm_base+MDM_ETMCR);
    for (count = 10000; BVAL(readl(etm_base+MDM_ETMSR), 1) != 1 && count > 0; count--)/*lint !e737*/
    {
        udelay(10);/*lint !e747 !e774 !e778*/
    }

    writel(0x6F | BIT(14), etm_base+MDM_ETMTEEVR);

    etmcr = (uint32_t)readl(etm_base+MDM_ETMCR);/*lint !e838*/
    etmcr |= BIT(0);
    writel(etmcr,etm_base+MDM_ETMCR);

    CS_ETM_LOCK(etm_base);
    printk(KERN_ERR"%s stop success 0x%x 0x%pK!\n",g_mdmcp_coresight[cpu].etm_name,etmcr,etm_base);
}


static void mdmcp_coresight_stop_etm4x(u32 cpu)
{
    void* etm4x_base;
    u32 control;

    if(!g_mdmcp_coresight[cpu].etm4x_base)
        return;

    etm4x_base = g_mdmcp_coresight[cpu].etm4x_base;

    CS_ETM_UNLOCK(etm4x_base);

	control = (u32)readl(etm4x_base+MDM_TRCPRGCTLR);

	/* EN, bit[0] Trace unit enable bit */
	control &= ~0x1;

	/* make sure everything completes before disabling */
	mb();
	isb();
	writel(control,etm4x_base+MDM_TRCPRGCTLR);

    CS_ETM_LOCK(etm4x_base);
    printk(KERN_ERR"%s stop success 0x%x,0x%pK!\n",g_mdmcp_coresight[cpu].etm4x_name,control,etm4x_base);
}

static void mdmcp_coresight_stop_etb(u32 cpu)
{
    void*     tmc_base;
    void*     etb_buf;
    u32       reg_value;
    u32       i;
    u32*      data;

    if(!g_mdmcp_coresight[cpu].tmc_base)
        return;
    if(g_mdmcp_coresight[cpu].etb_buf  != NULL)
        etb_buf = g_mdmcp_coresight[cpu].etb_buf;
    else{
        etb_buf = get_mdmcp_etb_buf(cpu);
        if(etb_buf == NULL){
            printk(KERN_ERR"get modem cp cpu%d dump buf failed!\n",cpu);
            return;
        }
        g_mdmcp_coresight[cpu].etb_buf = etb_buf;
    }
    tmc_base = g_mdmcp_coresight[cpu].tmc_base;

    if(*(u32*)etb_buf == CORESIGHT_MAGIC_NUM)
    {
        printk(KERN_ERR"modem cp cpu%d etb data has store finished,no need to store again!\n",(int)cpu);
        return;
    }

    /*该cpu是否已经热插拔*/
    if(*(u32*)etb_buf == CORESIGHT_HOTPLUG_MAGICNUM)
    {
        printk(KERN_ERR"modem cp cpu%d has powerdown or hotplug,no need to store data!\n",(int)cpu);
        return;
    }


    /* unlock etb, 配置ETF_LOCK_ACCESS */
    writel(0xC5ACCE55, tmc_base + 0xFB0);

    /* stop etb, 配置ETF_FORMAT_FLUSH_CTRL */
    reg_value = (u32)readl(tmc_base + 0x304);
    /* FFCR StopOnFl */
    reg_value |= 1 << 12;
    /* FFCR FlushMem */
    reg_value |= 1 << 6;
    writel(reg_value, tmc_base + 0x304);

    for(i=0; i<10000; i++)
    {
        /* read etb status, 读取ETF_STATUS */
        reg_value = (u32)readl(tmc_base + 0x304);
        /* bit2为TMCReady指示位 */
        if(0 != ((reg_value & (1 << 6)) >> 6))
        {
            break;
        }
        udelay(10);/*lint !e747 !e774 !e778*/
    }
    if(i >= 10000)
    {
        printk(KERN_ERR"ETF_STATUS register is not ready\n");
    }
    /* 等待TMCReady */
    for(i=0; i<10000; i++)
    {
        /* read etb status, 读取ETF_STATUS */
        reg_value = (u32)readl(tmc_base + 0xc);
        /* bit2为TMCReady指示位 */
        if(0 != (reg_value & 0x4))/*lint !e774*/
        {
            break;
        }
        udelay(10);/*lint !e747 !e774 !e778*/
    }

    /* 超时判断 */
    if(i >= 10000)
    {
        printk(KERN_ERR"save etb time out\n");
    }

    /* 导出etb数据 */
    memset((void *)etb_buf, 0x0, (unsigned long)DUMP_CP_UTRACE_SIZE);
    /* lint --e{124}*/
    data = (u32*)(etb_buf + 8);/*lint !e124*/
    for(i=0; i<(1024*8)/4; i++)
    {
        /* read etb, 读取ETF_RAM_RD_DATA */
        reg_value = (u32)readl(tmc_base + 0x10);
        *data = reg_value;
        data++;
        if(reg_value == 0xffffffff)
        {
            break;
        }
    }

    /* 0-3字节存放标识码 */
    *((u32 *)etb_buf) = (u32)CORESIGHT_MAGIC_NUM;
    /* 4-7个字节存放ETB数据长度 */
    *((u32 *)etb_buf + 1) = i*4;

    /* lock etb, 配置ETF_LOCK_ACCESS */
    writel(0x1, tmc_base + 0xFB0);

    printk(KERN_ERR"%s store success 0x%pK!\n",g_mdmcp_coresight[cpu].tmc_name,tmc_base);
}

static int mdmcp_coresight_tmc_probe(struct device_node* dev_node)
{
    u32 cpu_index=0;
    int ret;
    unsigned int phy_addr = 0;
    unsigned long sz;

    ret = of_property_read_u32(dev_node,"cpu_index",&cpu_index);
    if(ret){
        printk(KERN_ERR"read cpu_index failed,use default value! ret = %d\n",ret);
        cpu_index = 0;
    }

    if(!g_mdmcp_coresight[cpu_index].tmc_base){
        g_mdmcp_coresight[cpu_index].tmc_base = (void*)of_iomap(dev_node,0);
        sz = 1;
        (void)of_property_read_u32_array(dev_node, "reg",&phy_addr, sz);
        g_mdmcp_coresight[cpu_index].tmc_phy = phy_addr;
    }
    else
        printk(KERN_ERR"cpu %d have init before,cpu index may be error!\n",cpu_index);

    g_mdmcp_coresight[cpu_index].etb_buf = get_mdmcp_etb_buf(cpu_index);

    ret = of_property_read_string(dev_node,"coresight-name",&(g_mdmcp_coresight[cpu_index].tmc_name));
    if(ret)
        printk(KERN_ERR"read name failed! ret = %d\n",ret);

    return 0;
}

static int mdmcp_coresight_etm_probe(struct device_node* dev_node)
{
    u32 cpu_index=0;
    int ret;
    unsigned int phy_addr = 0;
    unsigned long sz;

    ret = of_property_read_u32(dev_node,"cpu_index",&cpu_index);
    if(ret){
        printk(KERN_ERR"get cpu_index failed,use default value! ret = %d\n",ret);
        cpu_index = 0;
    }

    cpu_index = cpu_index >= CPU_NUMS ? 0 : cpu_index;

    if(!g_mdmcp_coresight[cpu_index].etm_base){
        g_mdmcp_coresight[cpu_index].etm_base = (void*)of_iomap(dev_node,0);
        sz = 1;
        (void)of_property_read_u32_array(dev_node, "reg",&phy_addr, sz);
        g_mdmcp_coresight[cpu_index].etm_phy = phy_addr;
    }
    else
        printk(KERN_ERR"cpu %d have init before,cpu index may be error!\n",cpu_index);

    ret = of_property_read_string(dev_node,"coresight-name",&(g_mdmcp_coresight[cpu_index].etm_name));
    if(ret)
        printk(KERN_ERR"read name failed! ret = %d\n",ret);

    return 0;
}

static int mdmcp_coresight_etm4x_probe(struct device_node* dev_node)
{
    u32 cpu_index=0;
    int ret;
    unsigned int phy_addr = 0;
    unsigned long sz;

    ret = of_property_read_u32(dev_node,"cpu_index",&cpu_index);
    if(ret){
        printk(KERN_ERR"get cpu_index failed,use default value! ret = %d\n",ret);
        cpu_index = 0;
    }

    cpu_index = cpu_index >= CPU_NUMS ? 0 : cpu_index;

    if(!g_mdmcp_coresight[cpu_index].etm4x_base){
        g_mdmcp_coresight[cpu_index].etm4x_base = (void*)of_iomap(dev_node,0);
        sz = 1;
        (void)of_property_read_u32_array(dev_node, "reg",&phy_addr, sz);
        g_mdmcp_coresight[cpu_index].etm4x_phy = phy_addr;
    }
    else
        printk(KERN_ERR"cpu %d have init before,cpu index may be error!\n",cpu_index);

    ret = of_property_read_string(dev_node,"coresight-name",&(g_mdmcp_coresight[cpu_index].etm4x_name));
    if(ret)
        printk(KERN_ERR"read name failed! ret = %d\n",ret);

    return 0;
}


static struct of_device_id coresight_match[] = {
    {
        .name       = "coresight-tmc",
        .compatible = "arm,coresight-tmc,cp"
    },/*lint !e785*/
    {
        .name       = "coresight-etm",
        .compatible = "arm,coresight-etm,cp"
    },/*lint !e785*/
    {
        .name       = "coresight-etm4x",
        .compatible = "arm,coresight-etm4x,cp"
    },/*lint !e785*/
    {}/*lint !e785*/
};

static struct of_device_id coresight_match_es[] = {
    {
        .name       = "coresight-tmc",
        .compatible = "arm,coresight-tmc,cp,es"
    },/*lint !e785*/
    {
        .name       = "coresight-etm",
        .compatible = "arm,coresight-etm,cp,es"
    },/*lint !e785*/
    {
        .name       = "coresight-etm4x",
        .compatible = "arm,coresight-etm4x,cp,es"
    },/*lint !e785*/
    {}/*lint !e785*/
};
int mdmcp_coresight_init(void)
{
    struct device_node * node;
    struct device_node * child;

    if(bsp_get_version_info()->cses_type == TYPE_ES)
    {
        /*parse tmc node*/
        node = of_find_compatible_node(NULL, NULL, coresight_match_es[0].compatible);
        if(!node)
        {
            printk(KERN_ERR"can not find %s node!\n",coresight_match_es[0].compatible);
            return -1;
        }
        for_each_child_of_node(node, child)
        {
            mdmcp_coresight_tmc_probe(child);
        }

        /*parse etm node*/
        node = of_find_compatible_node(NULL, NULL, coresight_match_es[1].compatible);
        if(node)
        {
            for_each_child_of_node(node, child)
            {
                mdmcp_coresight_etm_probe(child);
            }
        }

        /*parse etm4x node*/
        node = of_find_compatible_node(NULL, NULL, coresight_match_es[2].compatible);
        if(node)
        {
            for_each_child_of_node(node, child)
            {
                mdmcp_coresight_etm4x_probe(child);
            }
        }
    }
    else
    {
        /*parse tmc node*/
        node = of_find_compatible_node(NULL, NULL, coresight_match[0].compatible);
        if(!node)
        {
            printk(KERN_ERR"can not find %s node!\n",coresight_match[0].compatible);
            return -1;
        }
        for_each_child_of_node(node, child)
        {
            mdmcp_coresight_tmc_probe(child);
        }

        /*parse etm node*/
        node = of_find_compatible_node(NULL, NULL, coresight_match[1].compatible);
        if(node)
        {
            for_each_child_of_node(node, child)
            {
                mdmcp_coresight_etm_probe(child);
            }
        }

        /*parse etm4x node*/
        node = of_find_compatible_node(NULL, NULL, coresight_match[2].compatible);
        if(node)
        {
            for_each_child_of_node(node, child)
            {
                mdmcp_coresight_etm4x_probe(child);
            }
        }
    }

    (void)mdmcp_debug_perctrl2_init();
    (void)mdmcp_debug_sysctrl_init();
    printk(KERN_ERR"mdmcp_coresight_init ok\n");

    return 0;

}
/*lint --e{528}*/
late_initcall(mdmcp_coresight_init);/*lint !e528*/


int bsp_coresight_stop_cp(void)
{
    int cpu;
    DRV_CORESIGHT_CFG_STRU cs_cfg;

    memset(&cs_cfg,0,sizeof(cs_cfg));

    if(bsp_nvm_read(NV_ID_DRV_CORESIGHT, (u8 *)&cs_cfg, (u32)sizeof(DRV_CORESIGHT_CFG_STRU)))
    {
        printk(KERN_ERR"read nv %d fail\n", NV_ID_DRV_CORESIGHT);
        return -1;
    }
    if(!cs_cfg.cp_enable)
    {
        printk(KERN_ERR"modem cp trace not enable\n");
        return 0;
    }

    if(mdmcp_debug_get_perctrl2_status()|| mdmcp_debug_get_sysctrl_status()){
        printk(KERN_ERR"cp coresight is powerdown or cp is pownerdown!\n");
        return -1;
    }

    for(cpu=0 ; cpu<CPU_NUMS ; cpu++)
    {
        mdmcp_coresight_stop_etb((u32)cpu);
        mdmcp_coresight_stop_etm((u32)cpu);
        mdmcp_coresight_stop_etm4x((u32)cpu);
    }

    return 0;
}

void bsp_coresight_save_cp_etb(char* dir_name)
{
    char file_name[256] = {0,};
    void* data = NULL;
    DRV_CORESIGHT_CFG_STRU cs_cfg;
    int cpu;
    int fd;
    int ret;


    if(bsp_nvm_read(NV_ID_DRV_CORESIGHT, (u8 *)&cs_cfg, (u32)sizeof(DRV_CORESIGHT_CFG_STRU)))
    {
        printk(KERN_ERR"read nv %d fail\n", NV_ID_DRV_CORESIGHT);
        return;
    }
    if(!cs_cfg.cp_store)
    {
        printk(KERN_ERR"modem cp trace not enable store\n");
        return;
    }

    for(cpu = 0; cpu < CPU_NUMS; cpu++)
    {
        memset(file_name, 0, sizeof(file_name));
        /*modem etb数据手机版本中需要上传apr网站，cpu0 的命名不能加索引号*/
        if(cpu == 0)
            snprintf(file_name, sizeof(file_name), "%smodem_etb.bin", dir_name);
        else
            snprintf(file_name, sizeof(file_name), "%smodem_etb%d.bin", dir_name,cpu);

        data = get_mdmcp_etb_buf((u32)cpu);
        if(!data)
            continue;

        fd = bsp_open(file_name, O_CREAT|O_RDWR|O_SYNC, 0660);
        if(fd<0)
            continue;

        ret = bsp_write((u32)fd,data,(u32)DUMP_CP_UTRACE_SIZE);
        if(ret != DUMP_CP_UTRACE_SIZE){
            printk(KERN_ERR"write modem cp cpu%d etb data error,ret = 0x%x\n",cpu,ret);
            bsp_close((u32)fd);
            continue;
        }
        bsp_close((u32)fd);
        printk(KERN_ERR" %s save success!\n",file_name);
    }
    return;
}


