/*
 * Hisilicon Platforms GET_CPU_VOLT support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/export.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/types.h>
#include <linux/string.h>
#include <global_ddr_map.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/compiler.h>
#include <asm/compiler.h>
#include <linux/proc_fs.h>


#define CPU_VOLT_FN_GET_VAL				(0xc800aa01)
#define AVS_VOLT_MAX_BYTE				(192)
#ifdef CONFIG_HISI_ENABLE_HPM_DATA_COLLECT
#define HPM_VOLT_MAX_BYTE				(102)
#define DIEID_MAX_BYTE					(20)
#endif

struct tag_cpu_volt_data {
	phys_addr_t phy_addr;
	unsigned char *virt_addr;
	struct mutex cpu_mutex;
};

static struct tag_cpu_volt_data g_cpu_volt_data;
extern char *g_lpmcu_rdr_ddr_addr;
extern u32 rdr_lpm3_buf_len;

/*lint -e715 -e838*/
noinline int atfd_hisi_service_get_val_smc(u64 function_id, u64 arg0, u64 arg1, u64 arg2)
{
	asm volatile (
		__asmeq("%0", "x0")
		__asmeq("%1", "x1")
		__asmeq("%2", "x2")
		__asmeq("%3", "x3")
		"smc	#0\n"
		: "+r" (function_id)
		: "r" (arg0), "r" (arg1), "r" (arg2));

	return (int)function_id;
}

static int get_volt_show(struct seq_file *m, void *v)
{
	int i = 0;
	int ret = 0;

	mutex_lock(&g_cpu_volt_data.cpu_mutex);

#ifdef CONFIG_HISI_ENABLE_HPM_DATA_COLLECT
	ret = atfd_hisi_service_get_val_smc((u64)CPU_VOLT_FN_GET_VAL,
					    g_cpu_volt_data.phy_addr,
					    (u64)(AVS_VOLT_MAX_BYTE + HPM_VOLT_MAX_BYTE + DIEID_MAX_BYTE +2), 0ULL);
#else
	ret = atfd_hisi_service_get_val_smc((u64)CPU_VOLT_FN_GET_VAL,
					    g_cpu_volt_data.phy_addr,
					    (u64)AVS_VOLT_MAX_BYTE, 0ULL);
#endif
	if (ret != 0) {
		(void)seq_printf(m, "get val failed.\n");
		mutex_unlock(&g_cpu_volt_data.cpu_mutex);
		return -EAGAIN;
	}
#ifdef CONFIG_HISI_ENABLE_HPM_DATA_COLLECT
	for (i = 0; i < (AVS_VOLT_MAX_BYTE + HPM_VOLT_MAX_BYTE + DIEID_MAX_BYTE + 2); i++) {
		if (i == AVS_VOLT_MAX_BYTE) {
			seq_printf(m, "\n hpm_volt: \n");
		} else if (i == (AVS_VOLT_MAX_BYTE + HPM_VOLT_MAX_BYTE)) {
			i += 2;
			seq_printf(m, "\n die_id: \n");
		}
		seq_printf(m, "0x%-2x ", g_cpu_volt_data.virt_addr[i]);
		if ((i != 0) && (i % 16 == 15))
			seq_printf(m, "\n");
	}
#else
	for (i = 0; i < AVS_VOLT_MAX_BYTE; i++) {
		seq_printf(m, "0x%-2x ", g_cpu_volt_data.virt_addr[i]);
		if ((i != 0) && (i % 16 == 15))
			seq_printf(m, "\n");
	}
#endif

	if (rdr_lpm3_buf_len < AVS_VOLT_MAX_BYTE) {
		mutex_unlock(&g_cpu_volt_data.cpu_mutex);
		return -ENOSPC;
	}

	memcpy((void *)(g_lpmcu_rdr_ddr_addr+rdr_lpm3_buf_len-AVS_VOLT_MAX_BYTE), g_cpu_volt_data.virt_addr, (size_t)AVS_VOLT_MAX_BYTE);

	mutex_unlock(&g_cpu_volt_data.cpu_mutex);

	return 0;
}

static int get_volt_open(struct inode *inode, struct file *file)
{
	return single_open(file, get_volt_show, NULL);
}

/*lint -e785*/
static const struct file_operations get_volt_operations = {
	.open		= get_volt_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
/*lint +e785*/
static int __init cpufreq_get_volt_init(void)
{
	int ret;
	struct device_node *np = NULL;
	uint32_t data[2] = { 0 };

	phys_addr_t bl31_smem_base =
	    HISI_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE;
	np = of_find_compatible_node(NULL, NULL, "hisilicon, get_val");
	if (!np) {
		pr_err("%s: no compatible node found.\n", __func__);
		return -EPERM;
	}

	ret = of_property_read_u32_array(np, "hisi,bl31-share-mem", &data[0], 2UL);
	if (ret) {
		pr_err("%s , get val mem compatible node err.\n",
		     __func__);
		return -EPERM;
	}

	g_cpu_volt_data.phy_addr = bl31_smem_base + data[0];
	g_cpu_volt_data.virt_addr =
	    (unsigned char *)ioremap(bl31_smem_base + data[0], (size_t)data[1]);
	if (NULL == g_cpu_volt_data.virt_addr) {
		pr_err
		    ("%s: %d: allocate memory for g_cpu_volt_data.virt_addr failed.\n",
		     __func__, __LINE__);
		return -EPERM;
	}
	mutex_init(&(g_cpu_volt_data.cpu_mutex));

	proc_create("param", 0660, NULL, &get_volt_operations);

	return 0;
}
/*lint +e715 +e838*/
/*lint -e528 -esym(528,*)*/
/*lint -e753 -esym(753,*)*/
module_init(cpufreq_get_volt_init);
/*lint -e528 +esym(528,*)*/
/*lint -e753 +esym(753,*)*/

