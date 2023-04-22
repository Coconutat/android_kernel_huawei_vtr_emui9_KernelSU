#include <linux/version.h>
#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/syscore_ops.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/err.h>
#include <asm/suspend.h>
#include <linux/platform_device.h>
#include <linux/cpu_pm.h>
#include <linux/clk.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,15)
#include <linux/slab.h>
#include <asm/cpu_ops.h>
#include <linux/psci.h>
#endif
#include <asm/cputype.h>
#include <soc_sctrl_interface.h>
#include <soc_crgperiph_interface.h>
#include <soc_acpu_baseaddr_interface.h>
#include <pm_def.h>
#include <linux/mfd/hisi_pmic.h>
#include <linux/wakeup_reason.h>
#include <linux/mfd/hisi_pmic.h>
#include "hisi_lpregs.h"

#include <chipset_common/dubai/dubai.h>

#include <huawei_platform/power/hw_power_monitor.h>

#include <soc_gpio_interface.h>

#define POWER_STATE_TYPE_SYS_SUSPEND	3
#define BUFFER_LEN  40

/*lint -e750 -esym(750,*) */
#define REG_SCBAKDATA8_OFFSET	SOC_SCTRL_SCBAKDATA8_ADDR(0)
#define REG_SCBAKDATA9_OFFSET	SOC_SCTRL_SCBAKDATA9_ADDR(0)

#define CPUIDLE_FLAG_REG(cluster)	\
	((cluster == 0) ? REG_SCBAKDATA8_OFFSET :  REG_SCBAKDATA9_OFFSET)


#define AP_SUSPEND_FLAG		BIT(16)

#define IRQ_GROUP_MAX		(13)
#define IRQ_NUM_PER_WORD	(32)

#define NO_SEQFILE 0

#define FPGA_FLAG		(0x1)
#define LITTLE_CLUSTER		(0x0)

#define PMU_WRITE_SR_TICK(offset, pos)


static void __iomem *g_bbpdrx1_base;
static void __iomem *sysctrl_base;
static void __iomem *g_enable_base;
static void __iomem *g_pending_base;
static void __iomem *crgctrl_base_addr;
static unsigned int g_enable_value[IRQ_GROUP_MAX];
static const char *g_plat_name;
unsigned int g_ap_irq_num;
const char **g_ap_irq_name;
unsigned int g_pm_fpga_flag;

#include <log/log_usertype/log-usertype.h>
unsigned long g_latt_wakeuptime = 0;
const char *g_latt_sourcename = NULL;
int  g_latt_gpio = 0;
unsigned long get_wakeuptime(void)
{
     return g_latt_wakeuptime;
}
const char *get_sourcename(void)
{
     return g_latt_sourcename;
}
int get_gpio(void)
{
     return g_latt_gpio;
}
EXPORT_SYMBOL(get_wakeuptime);
EXPORT_SYMBOL(get_sourcename);
EXPORT_SYMBOL(get_gpio);

typedef struct {
    void __iomem *ao_gpio_base;
    u32	ao_gpio_irq;
    u32	group_idx;
}ao_gpio_info;

ao_gpio_info *ao_gpio_info_p = NULL;
int g_ao_gpio_grp_num = 0;

static int pm_ao_gpio_irq_dump(unsigned int irq_num)
{
	int i = 0;
	int index = 0;
	int data = 0;

	if(g_ao_gpio_grp_num == 0 || (ao_gpio_info_p == NULL)){
		pr_err("%s: grp num is %d or NULL pointer.\n",__func__,g_ao_gpio_grp_num);
		return -EINVAL;
	}

	for(index = 0; index < g_ao_gpio_grp_num; index++){
		if(irq_num == ao_gpio_info_p[index].ao_gpio_irq){
			break;
		}
		if(index == (g_ao_gpio_grp_num-1)){
			pr_err("%s: irq num %d does not match with dtsi.\n",__func__, irq_num);
			return -EINVAL;
		}
	}

	data = readl(ao_gpio_info_p[index].ao_gpio_base + SOC_GPIO_GPIOIE_ADDR(0))
			& readl(ao_gpio_info_p[index].ao_gpio_base + SOC_GPIO_GPIOMIS_ADDR(0));

	for (i = 0; i < 8; i++)
		if (data & BIT(i))
			return (int)((ao_gpio_info_p[index].group_idx) * 8) + i;

	return  -EINVAL;
}


void pm_gic_dump(void)
{
	unsigned int i = 2;

	for (i = 2; i < IRQ_GROUP_MAX; i++){
	    g_enable_value[i] = readl(g_enable_base + i*4);
	}
}

void pm_gic_pending_dump(void)
{
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int value = 0;
	unsigned int irq = 0;
	int gpio = 0;

	for (i = 2; i < IRQ_GROUP_MAX; i++) {
		value = readl(g_pending_base + i * 4);

		for (j = 0; j < IRQ_NUM_PER_WORD; j++) {
			if ((value & BIT_MASK(j)) && ((value & BIT_MASK(j))
					== (g_enable_value[i] & BIT_MASK(j)))) {
				irq = i * IRQ_NUM_PER_WORD + j;
				if (irq < g_ap_irq_num) {
					printk("wake up irq num: %d, irq name: %s", irq, g_ap_irq_name[irq]);
					log_wakeup_reason((int)irq);
				} else {
					printk("wake up irq num: %d, irq name: no name!", irq);
				}
				power_monitor_report(WAKEUP_IRQ, "%s",
						g_ap_irq_name[irq]);
				gpio = pm_ao_gpio_irq_dump(irq);
				if (gpio >= 0) {
					printk("(gpio-%d)", gpio);
					power_monitor_report(WAKEUP_GPIO, "%d",
							gpio);
                }
				/* notify dubai module to update wakeup information */
				dubai_update_wakeup_info(g_ap_irq_name[irq], gpio);
                                if (BETA_USER == get_logusertype_flag()) {
                                          g_latt_wakeuptime = hisi_getcurtime() / 1000000;
                                          g_latt_sourcename = g_ap_irq_name[irq];
                                          g_latt_gpio = gpio;
                                }
				printk("\n");
			}
		}
	}
}

void hisi_set_ap_suspend_flag(unsigned int cluster)
{
	unsigned int val = 0;

	/* do not need lock, as the core is only one now. */
	val = readl(sysctrl_base + CPUIDLE_FLAG_REG(cluster));
	val |= AP_SUSPEND_FLAG;
	writel(val, sysctrl_base + CPUIDLE_FLAG_REG(cluster));
}

void hisi_clear_ap_suspend_flag(unsigned int cluster)
{
	unsigned int val = 0;

	/* do not need lock, as the core is only one now. */
	val = readl(sysctrl_base + CPUIDLE_FLAG_REG(cluster));
	val &= ~AP_SUSPEND_FLAG;
	writel(val, sysctrl_base + CPUIDLE_FLAG_REG(cluster));
}

static int hisi_test_pwrdn_othercores(unsigned int cluster, unsigned int core)
{
	unsigned int pwrack_stat = 0;
	unsigned int mask = 0;

	if (FPGA_FLAG == g_pm_fpga_flag) { /* FOR FPGA */
		pr_err("[%s-cluster%d] Only For fpga: don't check cores' power state in sr!!!!!!!!", __func__, cluster);
		pr_err("A53_COREPOWERSTAT:%x, MAIA_COREPOWERSTAT:%x, PERPWRACK:%x\n",
				readl(SOC_CRGPERIPH_A53_COREPOWERSTAT_ADDR(crgctrl_base_addr)),
				readl(SOC_CRGPERIPH_MAIA_COREPOWERSTAT_ADDR(crgctrl_base_addr)),
				readl(SOC_CRGPERIPH_PERPWRACK_ADDR(crgctrl_base_addr)));

		if (LITTLE_CLUSTER == cluster) {
			return ((readl(SOC_CRGPERIPH_A53_COREPOWERSTAT_ADDR(crgctrl_base_addr)) != 0x6)
					|| (readl(SOC_CRGPERIPH_MAIA_COREPOWERSTAT_ADDR(crgctrl_base_addr)) != 0x0));
		} else {
			return ((readl(SOC_CRGPERIPH_A53_COREPOWERSTAT_ADDR(crgctrl_base_addr)) != 0x0)
					|| (readl(SOC_CRGPERIPH_MAIA_COREPOWERSTAT_ADDR(crgctrl_base_addr)) != 0x6));
		}

	} else {
		/* boot core mask */
		mask = (0x1 << (11 + cluster * 4 + core));
		pwrack_stat = readl(SOC_CRGPERIPH_PERPWRACK_ADDR(crgctrl_base_addr));
		/* non boot core mask */
		mask = (unsigned int)COREPWRACK_MASK & (~mask);
		pwrack_stat &= mask;

		return pwrack_stat;
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,15)

#define SR_POWER_STATE_SUSPEND (0x01010000)

static int sr_psci_suspend(unsigned long index)
{
	return psci_ops.cpu_suspend(SR_POWER_STATE_SUSPEND,
			virt_to_phys(cpu_resume));
}

void hisi_cpu_suspend(void)
{
	cpu_suspend(0, sr_psci_suspend);
}
EXPORT_SYMBOL(hisi_cpu_suspend);

#endif

static int hisi_pm_enter(suspend_state_t state)
{
	unsigned int cluster = 0;
	unsigned int core = 0;
	unsigned long mpidr = read_cpuid_mpidr();
	unsigned int tickmark = 0;

	PMU_WRITE_SR_TICK(PMUOFFSET_SR_TICK, KERNEL_SUSPEND_IN);
	pr_err("%s ++\n", __func__);

	cluster = (mpidr >> 8) & 0xff;
	core = mpidr & 0xff;

	pr_debug("%s: mpidr is 0x%lx, cluster = %d, core = %d.\n", __func__, mpidr, cluster, core);

	pm_gic_dump();
	get_ip_regulator_state();
	dbg_io_status_show();
	dbg_pmu_status_show();
	dbg_clk_status_show();
	pmclk_monitor_enable();
	while (hisi_test_pwrdn_othercores(cluster, core))
		;
	PMU_WRITE_SR_TICK(PMUOFFSET_SR_TICK, KERNEL_SUSPEND_SETFLAG);
	hisi_set_ap_suspend_flag(cluster);
	cpu_cluster_pm_enter();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,15)
	hisi_cpu_suspend();
#else
	cpu_suspend(POWER_STATE_TYPE_SYS_SUSPEND);
#endif
	tickmark = readl(g_bbpdrx1_base);
	debuguart_reinit();
	cpu_cluster_pm_exit();
	hisi_clear_ap_suspend_flag(cluster);
	pr_info("%s tick: 0x%x, 0x%x, 0x%x\n",
			__func__, tickmark, readl(g_bbpdrx1_base),
			readl(g_bbpdrx1_base) - tickmark);
	pm_gic_pending_dump();
	pm_status_show(NO_SEQFILE);
	pr_err("%s --\n", __func__);
	PMU_WRITE_SR_TICK(PMUOFFSET_SR_TICK, KERNEL_RESUME);

	return 0;
}

static const struct platform_suspend_ops hisi_pm_ops = {
	.enter		= hisi_pm_enter,
	.valid		= suspend_valid_only_mem,
};

static int hisi_get_gic_base(void)
{
	void __iomem *hisi_gic_dist_base = NULL;
	struct device_node *node = NULL;
	u32 enable_offset = 0;
	u32 pending_offset = 0;
	int ret = 0;

	node = of_find_compatible_node(NULL, NULL, "arm,lp_gic");
	if (!node) {
		pr_err("%s: hisilicon,gic No compatible node found\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32(node, "enable-offset", &enable_offset);
	if (ret) {
		pr_err("%s: no enable-offset!\n", __func__);
		goto err_put_node;
	}

	ret = of_property_read_u32(node, "pending-offset", &pending_offset);
	if (ret) {
		pr_err("%s: no pending-offset!\n", __func__);
		goto err_put_node;
	}

	hisi_gic_dist_base = of_iomap(node, 0);
	if (!hisi_gic_dist_base) {
		of_node_put(node);
		pr_err("%s: hisilicon,gic_dist_base is NULL\n", __func__);
		return -ENODEV;
	}

	of_node_put(node);

	g_enable_base = hisi_gic_dist_base + enable_offset;
	g_pending_base = hisi_gic_dist_base + pending_offset;

	pr_info("g_enable_base = %pK, g_pending_base = %pK.\n", g_enable_base, g_pending_base);

	return 0;

err_put_node:
	of_node_put(node);

	pr_err("%s failed.\n", __func__);
	return ret;
}

static int init_lowpm_table(struct device_node *np)
{
	int ret = 0;
	u32 i = 0;

	/* init ap irq table */
	ret = of_property_count_strings(np, "ap-irq-table");
	if (ret < 0) {
		pr_err("%s, not find ap-irq-table property!\n", __func__);
		goto err;
	}
	g_ap_irq_num = ret;
	pr_info("%s, ap-irq-table num: %d!\n", __func__, g_ap_irq_num);

	g_ap_irq_name = kzalloc(g_ap_irq_num * sizeof(char *), GFP_KERNEL);
	if (!g_ap_irq_name) {
		pr_err("%s:%d kzalloc err!!\n", __func__, __LINE__);
		ret = -ENOMEM;
		goto err;
	}

	for (i = 0; i < g_ap_irq_num; i++) {
		ret = of_property_read_string_index(np, "ap-irq-table", i, &g_ap_irq_name[i]);
		if (ret) {
			pr_err("%s, no ap-irq-table %d!\n",
				__func__, i);
			goto err_free;
		}
	}

	pr_info("%s: init lowpm table success.\n", __func__);
	return ret;

err_free:
	kfree(g_ap_irq_name);
	g_ap_irq_name = NULL;
err:
	return ret;
}

static int init_ao_gpio(struct device_node *np)
{
	int i = 0;
	int ret = 0;
	struct device_node *dn = NULL;
	char *io_buffer = NULL;

	g_ao_gpio_grp_num = of_property_count_u32_elems(np, "ao-gpio-irq");
	if (g_ao_gpio_grp_num < 0){
	    pr_err("%s[%d], no ao gpio irq!\n", __func__,__LINE__);
	    ret = -ENODEV;
	    goto err;
	 }
	pr_info("%s: g_ao_gpio_grp_num is %d.\n", __func__, g_ao_gpio_grp_num);

	io_buffer = kzalloc(BUFFER_LEN*sizeof(char), GFP_KERNEL);
	if(!io_buffer){
	    pr_err("%s[%d]: kzalloc err!", __func__,__LINE__);
		ret = -ENOMEM;
	    goto err;
	}

	ao_gpio_info_p = kzalloc((unsigned long)(g_ao_gpio_grp_num * sizeof(ao_gpio_info)), GFP_KERNEL);
	if(!ao_gpio_info_p){
		pr_err("%s[%d]: kzalloc err!!\n",__func__, __LINE__);
		ret = -ENOMEM;
		goto err_free_buffer;
	}

	for(i = 0; i < g_ao_gpio_grp_num; i++){
	    ret = of_property_read_u32_index(np, "ao-gpio-irq", (unsigned int)i, &(ao_gpio_info_p[i].ao_gpio_irq));
	    if(ret){
		pr_err("%s, no ao-gpio-irq, %d.\n", __func__, i);
		goto err_free;
	    }
	}

	for(i = 0; i < g_ao_gpio_grp_num; i++){
	    ret = of_property_read_u32_index(np, "ao-gpio-group-idx", (unsigned int)i, &(ao_gpio_info_p[i].group_idx));
	    if(ret){
		pr_err("%s, no ao-gpio-group-idx, %d\n", __func__, i);
		goto err_free;
	    }
	}

	for(i = 0; i < g_ao_gpio_grp_num; i++){
	    memset(io_buffer, 0, BUFFER_LEN*sizeof(char));
	    snprintf(io_buffer, BUFFER_LEN*sizeof(char), "arm,primecell%d", ao_gpio_info_p[i].group_idx);
	    dn = of_find_compatible_node(NULL, NULL, io_buffer);
	    if (!dn) {
		    pr_err("%s: hisilicon,primecell%d No compatible node found\n",__func__, ao_gpio_info_p[i].group_idx);
		    ret = -ENODEV;
		    goto err_free;
	    }

	    ao_gpio_info_p[i].ao_gpio_base = of_iomap(dn, 0);
	    if(!ao_gpio_info_p[i].ao_gpio_base){
		    ret = -EINVAL;
		    of_node_put(dn);
		    goto err_free;
	    }

	    of_node_put(dn);
	}

	kfree(io_buffer);
	io_buffer = NULL;
	pr_info("%s: init ao gpio success.\n", __func__);
	return ret;

err_free:
	kfree(ao_gpio_info_p);
	ao_gpio_info_p = NULL;
err_free_buffer:
	kfree(io_buffer);
	io_buffer = NULL;
err:
	return ret;
}

static int sr_tick_pm_notify(struct notifier_block *nb,
		unsigned long mode, void *_unused)
{
	switch (mode) {
		case PM_SUSPEND_PREPARE:
			PMU_WRITE_SR_TICK(PMUOFFSET_SR_TICK, KERNEL_SUSPEND_PREPARE);
			break;
		case PM_POST_SUSPEND:
			PMU_WRITE_SR_TICK(PMUOFFSET_SR_TICK, KERNEL_RESUME_OUT);
			break;
		default:
			break;
	}

	return 0;
}

static struct notifier_block sr_tick_pm_nb = {
	.notifier_call = sr_tick_pm_notify,
};

static __init int hisi_pm_drvinit(void)
{
	struct device_node *np = NULL;
	int ret = 0;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,lowpm_func");
	if (!np) {
		pr_err("%s: hisilicon,lowpm_func No compatible node found\n",
				__func__);
		return -ENODEV;
	}

	g_pm_fpga_flag = 0;
	ret = of_property_read_u32_array(np, "fpga_flag", &g_pm_fpga_flag, 1);
	if (ret) {
		pr_err("%s , no fpga_flag.\n", __func__);
	}

	ret = of_property_read_string(np, "plat-name", &g_plat_name);
	if (ret)
		pr_err("%s, no plat-name!\n", __func__);
	pr_info("%s: current plat is %s.\n", __func__, g_plat_name);

	/* AO GPIO */
	ret = init_ao_gpio(np);
	if (ret){
		of_node_put(np);
		pr_err("%s, init ao GPIO failed!\n", __func__);
		return -ENODEV;
	}

	ret = init_lowpm_table(np);
	if (ret) {
		of_node_put(np);
		pr_err("%s, init lowpm_table err!\n", __func__);
		return -ENODEV;
	}

	of_node_put(np);

	np = of_find_compatible_node(NULL, NULL, "hisilicon,sysctrl");
	if (!np) {
		pr_err("%s: hisilicon,sysctrl No compatible node found\n",
				__func__);
		return -ENODEV;
	}

	sysctrl_base = of_iomap(np, 0);
	if (!sysctrl_base) {
		of_node_put(np);
		pr_err("%s: hisilicon,sysctrl_base is NULL\n", __func__);
		return -ENODEV;
	}

	of_node_put(np);

	np = of_find_compatible_node(NULL, NULL, "hisilicon,crgctrl");
	if (!np) {
		pr_info("%s: hisilicon,crgctrl No compatible node found\n",
				__func__);
		return -ENODEV;
	}

	crgctrl_base_addr = of_iomap(np, 0);
	if (!crgctrl_base_addr) {
		of_node_put(np);
		pr_err("%s: hisilicon,crgctrl_base_addr is NULL\n", __func__);
		return -ENODEV;
	}

	of_node_put(np);

	if (hisi_get_gic_base()) {
		pr_err("%s: hisilicon,get gic base failed!\n", __func__);
		return -ENODEV;
	}

	suspend_set_ops(&hisi_pm_ops);

	g_bbpdrx1_base = ioremap(SOC_SCTRL_SCBBPDRXSTAT1_ADDR(SOC_ACPU_SCTRL_BASE_ADDR), 0x4);
	if (!g_bbpdrx1_base){
		pr_err("%s: get SCBBPDRXSTAT1_ADDR failed!\n", __func__);
		return -ENODEV;
	}

	ret = register_pm_notifier(&sr_tick_pm_nb);
	if (ret) {
		pr_err("%s: register_pm_notifier failed!\n", __func__);
		return -ENODEV;
	}

	return 0;
}
arch_initcall(hisi_pm_drvinit);
