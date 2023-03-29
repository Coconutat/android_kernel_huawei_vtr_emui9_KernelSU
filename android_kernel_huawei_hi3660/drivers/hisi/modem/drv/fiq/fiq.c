#include <linux/init.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <bsp_fiq.h>
#include <bsp_sysctrl.h>

struct fiq_ctrl{
	unsigned int      fiq_init;
	void *               sysctrl_base_addr;
	void *              sysctrl_fiq_enable_reg;
	unsigned int      sysctrl_fiq_enable_bit;
	void *               sysctrl_fiq_status_reg;
	unsigned int      sysctrl_fiq_status_bit;
	unsigned int      chip_type;
	void *               smem_base_addr;
	void *               smem_fiq_status_addr;
	void *               smem_fiq_clear_addr;
	void *               smem_send_cnt_addr;
	void *               smem_recive_cnt_addr;
};
struct fiq_ctrl g_fiq_ctrl;

static void send_fiq(fiq_num fiq_num_t)
{
	u32 regval = 0;
	
	/*更新FIQ的状态*/
	regval = readl((volatile const void *)g_fiq_ctrl.smem_fiq_status_addr);
	regval |= ((u32)0x1 << fiq_num_t);
	writel(regval,(volatile void *)g_fiq_ctrl.smem_fiq_status_addr);

	if(g_fiq_ctrl.chip_type) /*mbb*/
	{
		writel((u32)0x1 << g_fiq_ctrl.sysctrl_fiq_enable_bit, (volatile void *)g_fiq_ctrl.sysctrl_fiq_enable_reg);
	}
	else /*PHONE*/
	{
		regval = readl((volatile const void *)(g_fiq_ctrl.sysctrl_fiq_enable_reg));
		regval &= (~((u32)1 << 12));
		writel(regval,(volatile void *)g_fiq_ctrl.sysctrl_fiq_enable_reg);
	}
}
static void check_fiq_send(void)
{
	pr_err("[fiq]fiq status regval[%pK]=0x%x[bit %d]\n", (g_fiq_ctrl.sysctrl_fiq_status_reg), readl((volatile const void *)(g_fiq_ctrl.sysctrl_fiq_status_reg)), g_fiq_ctrl.sysctrl_fiq_status_bit);
	if(readl((volatile const void *)g_fiq_ctrl.smem_send_cnt_addr) != readl((volatile const void *)g_fiq_ctrl.smem_recive_cnt_addr))
	{
		pr_err("[fiq]fiq send cnt != fiq receive cnt, lost modem fiq respond error\n");
	}
	pr_err("[fiq]fiq send cnt = %d,fiq receive cnt = %d\n",readl((volatile const void *)g_fiq_ctrl.smem_send_cnt_addr),readl((volatile const void *)g_fiq_ctrl.smem_recive_cnt_addr));

	if(readl((volatile const void *)g_fiq_ctrl.smem_fiq_clear_addr) != readl((volatile const void *)g_fiq_ctrl.smem_fiq_status_addr))
	{
		pr_err("[fiq]smem clear value != smem status value, modem fiq handler error\n");
	}
	pr_err("[fiq]smem clear value = 0x%x,smem status value = 0x%x\n",readl((volatile const void *)g_fiq_ctrl.smem_fiq_clear_addr),readl((volatile const void *)g_fiq_ctrl.smem_fiq_status_addr));
	
	writel(0x0,(volatile  void *)g_fiq_ctrl.smem_fiq_clear_addr);
	writel(0x0,(volatile  void *)g_fiq_ctrl.smem_fiq_status_addr);
}
int bsp_send_cp_fiq(fiq_num fiq_num_t)
{
	u32 tmp = 0;
	if (!g_fiq_ctrl.fiq_init)
	{
		pr_err("[fiq]fiq no init,send_cp_fiq too early\n");
		return BSP_ERROR;
	}
	if(fiq_num_t >= FIQ_MAX)
	{
		pr_err("[fiq]fiq_num = %d error\n",fiq_num_t);
		return BSP_ERROR;
	}
	check_fiq_send();

	/*更新中断处理次数*/
	tmp = readl((volatile const void *)g_fiq_ctrl.smem_send_cnt_addr);
	tmp += 1;
	writel(tmp, (volatile  void *)g_fiq_ctrl.smem_send_cnt_addr);

	/*发送FIQ*/
	send_fiq(fiq_num_t);
	
	return BSP_OK;
}

static int fiq_init(void)
{
	struct device_node *node = NULL;
	u32 tmp = 0;
       /* coverity[HUAWEI DEFECT] */
	(void)memset((void *)&g_fiq_ctrl,  0, sizeof(struct fiq_ctrl));
	
	g_fiq_ctrl.smem_base_addr = (void *)SHM_FIQ_BASE_ADDR;
       /* coverity[HUAWEI DEFECT] */
	(void)memset(g_fiq_ctrl.smem_base_addr,  0, SHM_SIZE_CCORE_FIQ);

	node = of_find_compatible_node(NULL, NULL, "hisilicon,fiq");
	if (!node)
	{
		pr_err("[fiq]get hisilicon,fiq fail!\n");
		return -1;
	}
	g_fiq_ctrl.sysctrl_base_addr = of_iomap(node, 0);
	if (!g_fiq_ctrl.sysctrl_base_addr)
	{
		pr_err("[fiq]of iomap fail\n");
		return -1;
	}

	if(of_property_read_u32_array(node, "fiq_enable_offset", &tmp, 1))
	{
		pr_err("[fiq]hisilicon,fiq fiq_enable_offset dts node not found!\n");
		return -1;
	}
	g_fiq_ctrl.sysctrl_fiq_enable_reg = g_fiq_ctrl.sysctrl_base_addr + tmp;

	if(of_property_read_u32_array(node, "fiq_enable_bit", &g_fiq_ctrl.sysctrl_fiq_enable_bit, 1))
	{
		pr_err("[fiq]hisilicon,fiq fiq_enable_bit dts node not found!\n");
		return -1;
	}

	if(of_property_read_u32_array(node, "fiq_status_offset", &tmp, 1))
	{
		pr_err("[fiq]hisilicon,fiq fiq_clear_reg dts node not found!\n");
		return -1;
	}
	g_fiq_ctrl.sysctrl_fiq_status_reg = g_fiq_ctrl.sysctrl_base_addr + tmp;
	
	if(of_property_read_u32_array(node, "fiq_status_bit", &g_fiq_ctrl.sysctrl_fiq_status_bit, 1))
	{
		pr_err("[fiq]hisilicon,fiq fiq_clear_reg dts node not found!\n");
		return -1;
	}
	if(of_property_read_u32_array(node, "chip_type", &g_fiq_ctrl.chip_type, 1))
	{
		pr_err("[fiq]hisilicon,fiq chip_type dts node not found!\n");
		return -1;
	}
	writel(0xFFFFFFFF, (volatile  void *)SHM_FIQ_BARRIER);
	g_fiq_ctrl.smem_fiq_clear_addr          = (void *)SHM_FIQ_CLEAR_ADDR;
	g_fiq_ctrl.smem_fiq_status_addr         = (void *)SHM_FIQ_STATUS_ADDR;
	g_fiq_ctrl.smem_send_cnt_addr         = (void *)SHM_FIQ_TOTAL_SEND_CNT;
	g_fiq_ctrl.smem_recive_cnt_addr        = (void *)SHM_FIQ_TOTAL_RECIVE_CNT;

	g_fiq_ctrl.fiq_init = 0x1;
	pr_err("[fiq]init OK\n");
	return 0;
}

arch_initcall(fiq_init);

