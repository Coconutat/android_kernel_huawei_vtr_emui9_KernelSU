

#include <linux/module.h>
#include "dlock_balong.h"
#include "drv_nv_def.h"
#include "drv_nv_id.h"
#include "drv_comm.h"
#include "bsp_sysctrl.h"
#include <mdrv_sysboot_commmon.h>
#include "bsp_nvim.h"
#include <osl_common.h>
#include <linux/delay.h>
#include <linux/module.h>
#include "bsp_dump.h"
#include <osl_bio.h>
#include <of.h>

struct bus_info g_bus_info = {0, 0};
struct bus_reset_info g_reset_info = {{0}};
u32 g_dlock_reset_enable = 0;
u32 g_dlock_int_no = 0;
bool g_dlock_flag = false;


static inline void hi_dlock_get_reg(unsigned int *value,void *base_address, u32 *pstreg)
{
	 unsigned int mask = 0;
	 unsigned int temp = 0;
	 void *reg  = base_address + pstreg[0];

	 temp	= readl(reg);
	 mask	= ((1U << (pstreg[2] - pstreg[1] + 1)) -1) << pstreg[1];
	 *value = temp & mask;
	 *value = (*value) >> pstreg[1];
}


static inline void hi_dlock_set_reg(unsigned int value, void *base_address, u32 *pstreg)
{
	void *reg	= base_address + pstreg[0];
	unsigned int temp	= 0;
	unsigned int mask	= 0;

	temp   = readl(reg);
	mask   = ((1U << (pstreg[2] - pstreg[1] + 1)) -1) << pstreg[1];
	value  = (temp & (~mask)) | ((value <<pstreg[1]) & mask);
	writel(value  ,reg);
}


void dlock_bus_state_free(void)
{
    if(NULL != g_bus_info.bus_state_info)
    {
        dlock_free(g_bus_info.bus_state_info);
    }
}


void dlock_bus_dlock_free(u32 i)
{
	u32 j;

	for(j = 0; j <= i; j++)
	{
        if(NULL != g_bus_info.bus_state_info[j].bus_dlock_info.mst_port_info)
            dlock_free(g_bus_info.bus_state_info[j].bus_dlock_info.mst_id_info);
        if(NULL != g_bus_info.bus_state_info[j].bus_dlock_info.mst_id_info)
            dlock_free(g_bus_info.bus_state_info[j].bus_dlock_info.mst_port_info);
	}
    dlock_bus_state_free();
}


void dlock_get_axi_gm_master(u32 mst_port,u32 i,char *mst_name)
{
    u32 j = 0,k = 0, id = 0;
    for (j = 0; j < g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_num; j++)
    {
        if (mst_port == g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_info[j].mst_port)
        {
            strncpy(mst_name, g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_info[j].mst_name, 32);
            return;
        }
    }

    if (mst_port == g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_id)
    {
        hi_dlock_get_reg(&id, g_bus_info.bus_state_info[i].bus_state.bus_vir_addr, g_bus_info.bus_state_info[i].bus_dlock_info.mst_id_reg);
        for (k = 0; k < g_bus_info.bus_state_info[i].bus_dlock_info.mst_id_num; k++)
        {
            if (id == g_bus_info.bus_state_info[i].bus_dlock_info.mst_id_info[k].id)
            {
                strncpy(mst_name, g_bus_info.bus_state_info[i].bus_dlock_info.mst_id_info[k].id_mst_name, 32);
                return;
            }
        }
    }
}


void dlock_get_axi_fbp_master(u32 mst_port,u32 i,char *mst_name)
{
    u32 j = 0;
    for (j = 0; j < g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_num; j++)
    {
        if (mst_port == g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_info[j].mst_port)
        {
            strncpy(mst_name, g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_info[j].mst_name, 32);
            return;
        }
    }
}


void dlock_master_match(char *bus_name, u32 mst_port, u32 i, char *mst_name)
{
    if ((0 == strncmp(bus_name, "AXI_GLB", strlen("AXI_GLB")))||(0 == strncmp(bus_name, "AXI_MDM", strlen("AXI_MDM"))))
    {
        dlock_get_axi_gm_master(mst_port, i, mst_name);
    }
    else if ((0 == strncmp(bus_name, "AXI_FAST", strlen("AXI_FAST")))||(0 == strncmp(bus_name, "AXI_BBPHY", strlen("AXI_BBPHY")))||(0 == strncmp(bus_name, "AXI_PCIE", strlen("AXI_PCIE"))))
    {
        dlock_get_axi_fbp_master(mst_port, i, mst_name);
    }
    else
    {
        printk(KERN_ERR"get master fail!\n");
    }
}


void dlock_sysctrl_reset(void)
{
    if (0 == strncmp(g_reset_info.version, "v7r22", strlen("v7r22")))
    {
        hi_dlock_set_reg(1, g_reset_info.pd_vir_addr, g_reset_info.pd_reset_reg);
        hi_dlock_set_reg(1, g_reset_info.pcie_vir_addr, g_reset_info.pcie_reset_reg);
        hi_dlock_set_reg(1, g_reset_info.mdm_vir_addr, g_reset_info.mdm_reset_reg);
    }
}


void dlock_sysctrl_unreset(void)
{
    if (0 == strncmp(g_reset_info.version, "v7r22", strlen("v7r22")))
    {
        hi_dlock_set_reg(0, g_reset_info.pd_vir_addr, g_reset_info.pd_reset_reg);
        hi_dlock_set_reg(0, g_reset_info.pcie_vir_addr, g_reset_info.pcie_reset_reg);
        hi_dlock_set_reg(0, g_reset_info.mdm_vir_addr, g_reset_info.mdm_reset_reg);
    }
}


s32 dlock_get_reset_reg(void)
{
    struct device_node *dev = NULL;
    const char *node_name = "hisilicon,bus_reset_reg";
    const char *cur_ver = NULL;
    u32 pd_phy_addr = 0, mdm_phy_addr = 0, pcie_phy_addr = 0;
    s32 ret = 0;

    dev = of_find_compatible_node(NULL, NULL, node_name);
    if(NULL == dev)
    {
    	dlock_print("get bus_reset_reg_dev_node not found!\n");
        return dlock_error;
    }
    ret = of_property_read_string(dev, "version", &cur_ver);
    if(ret)
    {
        dlock_print("Read version failed!\n");
        return dlock_error;
    }
    strncpy(g_reset_info.version, cur_ver, 32);

    if (0 == strncmp(cur_ver, "v7r22", strlen("v7r22")))
    {
        ret = of_property_read_u32_array(dev, "dlock_pd_reset_reg", g_reset_info.pd_reset_reg, 3);
        if(ret)
        {
            dlock_print("Read pd_reset_reg failed!\n");
            return dlock_error;
        }
        ret = of_property_read_u32_array(dev, "dlock_pd_base_addr", &pd_phy_addr, 1);
        if(ret)
        {
            dlock_print("Read pd_base_addr failed!\n");
            return dlock_error;
        }
        if(NULL == g_reset_info.pd_vir_addr)
        {
            g_reset_info.pd_vir_addr = ioremap(pd_phy_addr, SYSCTRL_ADDR_SIZE);
        }

        ret = of_property_read_u32_array(dev, "dlock_pcie_reset_reg", g_reset_info.pcie_reset_reg, 3);
        if(ret)
        {
            dlock_print("Read pcie_reset_reg failed!\n");
            return dlock_error;
        }

        ret = of_property_read_u32_array(dev, "dlock_pcie_base_addr", &pcie_phy_addr, 1);
        if(ret)
        {
            dlock_print("Read pcie_base_addr failed!\n");
            return dlock_error;
        }
        if(NULL == g_reset_info.pcie_vir_addr)
        {
            g_reset_info.pcie_vir_addr = ioremap(pcie_phy_addr, SYSCTRL_ADDR_SIZE);
        }
    }

    ret = of_property_read_u32_array(dev, "dlock_mdm_reset_reg", g_reset_info.mdm_reset_reg, 3);
    if(ret)
    {
        dlock_print("Read mdm_reset_reg failed!\n");
        return dlock_error;
    }
    ret = of_property_read_u32_array(dev, "dlock_mdm_base_addr", &mdm_phy_addr, 1);
    if(ret)
    {
        dlock_print("Read mdm_base_addr failed!\n");
        return dlock_error;
    }
    if(NULL == g_reset_info.mdm_vir_addr)
    {
        g_reset_info.mdm_vir_addr = ioremap(mdm_phy_addr, SYSCTRL_ADDR_SIZE);
    }

    /*获取死锁计数分频系数寄存器，并将其写为2*/

    ret = of_property_read_u32_array(dev, "dlock_cnt_clk_div_num_reg", g_reset_info.cnt_div_num_reg, 3);
    if(ret)
    {
        dlock_print("Read cnt_clk_div_num_reg failed!\n");
        return dlock_error;
    }
    if (0 == strncmp(cur_ver, "v7r22", strlen("v7r22")))
    {
        hi_dlock_set_reg(2, g_reset_info.mdm_vir_addr, g_reset_info.cnt_div_num_reg);
    }
    else if (0 == strncmp(cur_ver, "chicago", strlen("chicago")))
    {
        hi_dlock_set_reg(15, g_reset_info.mdm_vir_addr, g_reset_info.cnt_div_num_reg);
    }

    return dlock_ok;
}


s32 dlock_axi_pub_info(struct device_node *dev,u32 i)
{
	s32 ret = 0;

    ret = of_property_read_u32_array(dev, "dlock_mst_port_reg", g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_reg, 3);
    if(ret)
    {
	    dlock_print("Read mst_port_reg failed!\n");
        return dlock_error;
    }

    ret = of_property_read_u32_array(dev, "dlock_slv_port_reg", g_bus_info.bus_state_info[i].bus_dlock_info.slv_port_reg, 3);
    if(ret)
    {
	    dlock_print("Read slv_port_reg failed!\n");
        return dlock_error;
    }

    ret = of_property_read_u32_array(dev, "dlock_wr_reg", g_bus_info.bus_state_info[i].bus_dlock_info.wr_reg, 3);
    if(ret)
    {
	    dlock_print("Read wr_reg failed!\n");
        return dlock_error;
    }

    ret = of_property_read_u32_array(dev, "dlock_addr_reg", g_bus_info.bus_state_info[i].bus_dlock_info.addr_reg, 3);
    if(ret)
    {
	    dlock_print("Read addr_reg failed!\n");
        return dlock_error;
    }
    return dlock_ok;
}


s32 dlock_axi_gm_info(const char *node_name,u32 i)
{
    struct device_node *dev = NULL;
    u32 j = 0, k = 0, id_num = 0, id = 0, mst_port_num = 0, mst_port = 0;
    const char *temp1 = NULL,*temp2 = NULL;
    s32 ret = 0;

    dev = of_find_compatible_node(NULL, NULL, node_name);
	if(NULL == dev)
	{
		dlock_print("get gm_dev_node not found!\n");
        goto error_free;
	}

    ret = dlock_axi_pub_info(dev,i);
    if(ret)
    {
        dlock_print("get_gm_pub_info failed!\n");
        goto error_free;
    }

    ret = of_property_read_u32_array(dev, "dlock_mst_port_id", &g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_id, 1);
    if(ret)
    {
        dlock_print("Read gm_mst_port_id failed!\n");
        goto error_free;
    }

    ret = of_property_read_u32_array(dev, "dlock_id_reg", g_bus_info.bus_state_info[i].bus_dlock_info.mst_id_reg, 3);
    if(ret)
    {
        dlock_print("Read gm_mst_id_reg failed!\n");
        goto error_free;
    }

    ret = of_property_read_u32_array(dev, "dlock_mst_port_num", &mst_port_num, 1);
    if(ret)
    {
    	dlock_print("Read gm_mst_port_num failed!\n");
    	goto error_free;
    }
    g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_num = mst_port_num;
    g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_info = (struct mst_port_info *)dlock_malloc(mst_port_num*(sizeof(struct mst_port_info)));
    if(!g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_info)
    {
        dlock_print("gm_mst_port_info malloc failed!\n");
        goto error_free;
    }
    memset(g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_info, 0, mst_port_num*(sizeof(struct mst_port_info)));

    /*确定mst_port和mst_name的一一对应关系*/
    for(j = 0; j < mst_port_num; j++)
    {
        ret = of_property_read_u32_index(dev, "dlock_mst_port", j, &mst_port);
        ret |= of_property_read_string_index(dev, "dlock_mst_name", (s32)j, &temp1);
        if(ret)
        {
            dlock_print("Read gm_mst_port or gm_mst_name failed!\n");
            goto error_free;
        }
        g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_info[j].mst_port = mst_port;
        strncpy(g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_info[j].mst_name, temp1, 32);
    }

    ret = of_property_read_u32_array(dev, "dlock_id_num", &id_num, 1);
    if(ret)
    {
    	dlock_print("Read gm_id_num failed!\n");
    	goto error_free;
    }
    g_bus_info.bus_state_info[i].bus_dlock_info.mst_id_num = id_num;
    g_bus_info.bus_state_info[i].bus_dlock_info.mst_id_info = (struct mst_id_info *)dlock_malloc(id_num*(sizeof(struct mst_id_info)));
    if(!g_bus_info.bus_state_info[i].bus_dlock_info.mst_id_info)
    {
        dlock_print("gm_mst_id_info malloc failed!\n");
        goto error_free;
    }
    memset(g_bus_info.bus_state_info[i].bus_dlock_info.mst_id_info, 0, id_num*(sizeof(struct mst_id_info)));

    /*确定mst_id和mst_name的一一对应关系*/
    for(k = 0; k < id_num; k++)
    {
        ret = of_property_read_u32_index(dev, "dlock_id", k, &id);
        ret |= of_property_read_string_index(dev, "dlock_id_mst_name", (s32)k, &temp2);
        if(ret)
        {
            dlock_print("Read gm_id or gm_id_mst_name failed!\n");
            goto error_free;
        }
        g_bus_info.bus_state_info[i].bus_dlock_info.mst_id_info[k].id = id;
        strncpy(g_bus_info.bus_state_info[i].bus_dlock_info.mst_id_info[k].id_mst_name, temp2, 32);
    }
    return dlock_ok;

error_free:
    dlock_bus_dlock_free(i);
	return dlock_error;
}


s32 dlock_axi_fbp_info(const char *node_name,u32 i)
{
    struct device_node *dev = NULL;
    u32 j = 0, mst_port_num = 0, mst_port = 0;
    const char *temp1;
    s32 ret = 0;

    dev = of_find_compatible_node(NULL, NULL, node_name);
	if(NULL == dev)
	{
		dlock_print("get fbp_dev_node not found!\n");
        goto error_free;
	}

    ret = dlock_axi_pub_info(dev,i);
    if(ret)
    {
        dlock_print("get_fbp_pub_info failed!\n");
        goto error_free;
    }


    ret = of_property_read_u32_array(dev, "dlock_mst_port_num", &mst_port_num, 1);
    if(ret)
    {
    	dlock_print("Read fbp_mst_port_num failed!\n");
    	goto error_free;
    }
    g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_num = mst_port_num;
    g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_info = (struct mst_port_info *)dlock_malloc(mst_port_num*(sizeof(struct mst_port_info)));
    if(!g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_info)
    {
        dlock_print("fbp_mst_port_info malloc failed!\n");
        goto error_free;
    }
    memset(g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_info, 0, mst_port_num*(sizeof(struct mst_port_info)));

    /*确定mst_port和mst_name的一一对应关系*/
    for(j = 0; j < mst_port_num; j++)
    {
        ret = of_property_read_u32_index(dev, "dlock_mst_port", j, &mst_port);
        ret |= of_property_read_string_index(dev, "dlock_mst_name", (s32)j, &temp1);
        if(ret)
        {
            dlock_print("Read fbp_mst_port or fbp_mst_name failed!\n");
            goto error_free;
        }
        g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_info[j].mst_port = mst_port;
        strncpy(g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_info[j].mst_name, temp1, 32);
    }
    return dlock_ok;

error_free:
    dlock_bus_dlock_free(i);
	return dlock_error;
}


void dlock_bus_match(const char *bus_name, u32 i)
{
    if (0 == strncmp(bus_name, "AXI_GLB", strlen("AXI_GLB")))
    {
        dlock_axi_gm_info("hisilicon,dlock_axi_glb_balong", i);
    }
    else if (0 == strncmp(bus_name, "AXI_FAST", strlen("AXI_FAST")))
    {
        dlock_axi_fbp_info("hisilicon,dlock_axi_fast_balong", i);
    }
    else if (0 == strncmp(bus_name, "AXI_MDM", strlen("AXI_MDM")))
    {
        dlock_axi_gm_info("hisilicon,dlock_axi_mdm_balong", i);
    }
    else if (0 == strncmp(bus_name, "AXI_BBPHY", strlen("AXI_BBPHY")))
    {
        dlock_axi_fbp_info("hisilicon,dlock_axi_bbphy_balong", i);
    }
    else if (0 == strncmp(bus_name, "AXI_PCIE", strlen("AXI_PCIE")))
    {
        dlock_axi_fbp_info("hisilicon,dlock_axi_pcie_balong", i);
    }
    else
    {
        dlock_print("error axi_bus not found!\n");
    }
}


s32 dlock_get_bus_state_reg(void)
{
    struct device_node *dev = NULL;
    const char *node_name = "hisilicon,bus_state_balong";
    struct property *prop = NULL;
    const __be32 *p = NULL;
    const char *bus_name = NULL;
    u32 i = 0, j = 0, k = 0, u = 0, phy_addr = 0, bus_num = 0;
    void *vir_addr = NULL;
    s32 ret = 0;

	dev = of_find_compatible_node(NULL, NULL, node_name);
	if(NULL == dev)
	{
		dlock_print("Read bus_state_dev_node failed!\n");
		return dlock_error;
	}

    ret = of_property_read_u32_array(dev, "bus_num", &bus_num, 1);
    if(ret)
    {
	    dlock_print("Read bus_num failed!\n");
	    return dlock_error;
    }
    g_bus_info.bus_num = bus_num;

    g_bus_info.bus_state_info = (struct bus_state_info *)dlock_malloc(bus_num*sizeof(struct bus_state_info));
    if(!g_bus_info.bus_state_info)
    {
        dlock_print("Struct bus_state_info malloc failed!\n");
        return dlock_error;
    }
    memset(g_bus_info.bus_state_info, 0, bus_num*sizeof(struct bus_state_info));

    /*获取总线锁死状态寄存器偏移、起始位、终止位*/
    of_property_for_each_u32(dev, "bus_state_reg", prop, p, u)
    {
    	if(j % 3 == 0)
    	{
    		g_bus_info.bus_state_info[k].bus_state.bus_state_reg[0] = u;
    	}
    	else if(j % 3 == 1)
    	{
    		g_bus_info.bus_state_info[k].bus_state.bus_state_reg[1] = u;
    	}
        else
    	{
    		g_bus_info.bus_state_info[k].bus_state.bus_state_reg[2] = u;
    		k++;
    	}
    	j++;
    }
    if(j != 3*bus_num || k != bus_num)
	{
		dlock_print("Read bus_state_reg failed!\n");
        goto error_free;
	}

    for(i = 0; i < bus_num; i++)
    {
        /*获取锁死总线基地址、名称、标志位*/
        ret = of_property_read_u32_index(dev, "bus_base_addr", i, &phy_addr);
        ret |= of_property_read_string_index(dev, "bus_name", (s32)i, &bus_name);
        if(ret)
        {
            dlock_print("Read bus_base_addr or bus_name failed!\n");
            goto error_free;
        }
        vir_addr= ioremap_wc(phy_addr, SYSCTRL_ADDR_SIZE);
        if(NULL == vir_addr)
        {
            dlock_print("sysctrl ioremap fail!\n");
            goto error_free;
        }
        g_bus_info.bus_state_info[i].bus_state.bus_vir_addr = vir_addr;
        strncpy(g_bus_info.bus_state_info[i].bus_state.bus_name, bus_name, 32);

        /*根据bus_name匹配对应的总线，获取对应总线锁死寄存器信息*/
        dlock_bus_match(bus_name,i);
    }
    return dlock_ok;

error_free:
	dlock_bus_state_free();
	return dlock_error;
}


s32 bsp_dlock_reset_cb(DRV_RESET_CB_MOMENT_E eparam, int usrdata)
{
    if ((MDRV_RESET_CB_AFTER == eparam)&&(true == g_dlock_flag))
    {
        enable_irq(g_dlock_int_no);
        g_dlock_flag = false;
        dlock_print("enable dlock irq after modem reset!\n");
        return dlock_ok;
    }
    return dlock_ok;
}


static irqreturn_t dlock_int_handler(void)
{
	u32 i, num = 0, state_value = 0;
	u32 mst_port = 0, slv_port = 0, slv_addr = 0, wr_flag = 0;
    char dlock_bus_name[32]= {0};
    char mst_name[32] = {0};
    char type[10]= {0};

    printk(KERN_ERR"dlock_int_handler enter!\n");
    disable_irq_nosync(g_dlock_int_no);
    g_dlock_flag = true;

    printk(KERN_ERR"*************************dlock_info*************************\n");
    printk(KERN_ERR"%-10s %-10s %-10s %-10s %-10s %-10s\n","Bus","Master","M_port","S_port","Slv_addr","Operation");

    /*从所有axi总线中找到所有锁死总线，并将其放入全局数组中*/
    for(i =0; i < g_bus_info.bus_num; i++)
    {
        strncpy(dlock_bus_name, g_bus_info.bus_state_info[i].bus_state.bus_name, 32);
        hi_dlock_get_reg(&state_value, g_bus_info.bus_state_info[i].bus_state.bus_vir_addr, g_bus_info.bus_state_info[i].bus_state.bus_state_reg);
        if (1 == state_value)
        {
            /*读取每条死锁总线的mst、slv端口号、访问地址、读写指示*/
            hi_dlock_get_reg(&mst_port, g_bus_info.bus_state_info[i].bus_state.bus_vir_addr, g_bus_info.bus_state_info[i].bus_dlock_info.mst_port_reg);
            hi_dlock_get_reg(&slv_port, g_bus_info.bus_state_info[i].bus_state.bus_vir_addr, g_bus_info.bus_state_info[i].bus_dlock_info.slv_port_reg);
            slv_addr = readl(g_bus_info.bus_state_info[i].bus_state.bus_vir_addr + g_bus_info.bus_state_info[i].bus_dlock_info.addr_reg[0]);
            hi_dlock_get_reg(&wr_flag, g_bus_info.bus_state_info[i].bus_state.bus_vir_addr, g_bus_info.bus_state_info[i].bus_dlock_info.wr_reg);
            if (1 == wr_flag)
                strncpy(type, "write", 10);
            else
                strncpy(type, "read", 10);

            /*根据bus_name匹配对应的总线，然后再根据mst_port、mst_id获取对应的mst_name*/
            dlock_master_match(dlock_bus_name, mst_port, i, mst_name);
            mst_port++;

            /*打印死锁总线dlock信息到串口，包括死锁总线、端口号、master、slv地址、读写类型*/
            printk(KERN_ERR"%-10s %-10s M%-9x S%-9x %#-10x %-10s\n",dlock_bus_name,mst_name,mst_port,slv_port,slv_addr,type);
            memset(mst_name, 0, sizeof(mst_name));
            num++;
        }
    }
    printk(KERN_ERR"**************************dlock_end**************************\n");

    if((g_dlock_reset_enable == DLOCK_ENABLE)&& (num >= 1))
    {
        system_error(DRV_ERRNO_DLOCK, DUMP_REASON_DLOCK, 0, NULL, 0);
    }
    else
    {
        enable_irq(g_dlock_int_no);
        printk(KERN_ERR"dlock nv disable or not real bus error!\n");
    }

    return IRQ_HANDLED;
}


s32 bsp_dlock_init(void)
{
    struct device_node *dev = NULL;
	const char *node_name = "hisilicon,dlock_irq_balong";
    u32 num = 0, irq = 0;
    s32 ret = 0;
    DRV_DLOCK_CFG_STRU cfg = {0,0};

    /*若为MBB产品，且TEEOS编译宏打开，则认为A核为非安全区，不能解析安全区的系统控制器，此时不做解析*/

    if(BSP_OK != bsp_nvm_read(NV_ID_DRV_DLOCK, (u8 *)&cfg, sizeof(DRV_DLOCK_CFG_STRU)))
	{
		dlock_print("dlock read nv 0x%x error\n", NV_ID_DRV_DLOCK);
		return dlock_error;
	}
	else
	{
        if(cfg.enable == DLOCK_ENABLE)
        {
	        g_dlock_reset_enable = cfg.reset_enable;

            /*获取总线状态寄存器、地址寄存器等信息*/
            ret = dlock_get_bus_state_reg();
            if(ret)
            {
                dlock_print("dlock_get_bus_state_reg failed!\n");
                return dlock_error;
            }
            num = (g_bus_info.bus_num -1);

            /*获取总线复位寄存器、死锁计数分频系数等信息*/
            ret = dlock_get_reset_reg();
            if(ret)
            {
                dlock_print("dlock_get_reset_reg failed!\n");
                dlock_bus_dlock_free(num);
                return dlock_error;
            }

            /*清除dlock残留状态*/
            dlock_sysctrl_reset();

            /*注册中断*/
            dev = of_find_compatible_node(NULL, NULL, node_name);
            if(NULL == dev)
            {
            	dlock_print("Read irq_dev_node not found!\n");
                dlock_bus_dlock_free(num);
            	return dlock_error;
            }
            irq = irq_of_parse_and_map(dev, 0);

            ret = request_irq(irq, (irq_handler_t)dlock_int_handler, 0, "dlock irq", NULL);
            if(ret)
            {
            	dlock_print("dlock_int_handler request irq failed.\n");
                dlock_bus_dlock_free(num);
            	return dlock_error;
            }
            g_dlock_int_no = irq;

            /*解复位dlock*/
            dlock_sysctrl_unreset();

            dlock_print("dlock init ok\n");
        }
        else
        {
            dlock_print("dlock nv disable\n");
        }
	}
    return dlock_ok;
}

static void __exit bsp_dlock_exit(void)
{
}

module_init(bsp_dlock_init);
module_exit(bsp_dlock_exit);



