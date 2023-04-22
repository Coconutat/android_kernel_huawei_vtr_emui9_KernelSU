#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/seq_file.h>
#include <linux/printk.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/of.h>
#include "soc_sctrl_interface.h"
#include "soc_acpu_baseaddr_interface.h"
#define DDR_INFO_ADDR (SOC_SCTRL_SCBAKDATA7_ADDR(SOC_ACPU_SCTRL_BASE_ADDR))
#define DDR_TYPE_ADDR (SOC_SCTRL_SCBAKDATA4_ADDR(SOC_ACPU_SCTRL_BASE_ADDR))

#define L29_VN1_2_BOARD 3533
static char ddr_rod[20] = {0};

/* DDR_INFO_ADDR is the physical address of the register for storing*/
/*DDR manufacturer id and memory size.the register vlaue is conveyed*/
/*from xloader to Linux kernel,defined in "xloader/ddr/ddr_asic.h",*/
/*and named  SYSCTRL_DDRINFO. */

static unsigned int ddr_get_boardid(void)
{
	unsigned int full_boardid;
	unsigned int boardid[4] = {0};
	struct device_node *root;
	int ret;

	root = of_find_node_by_path("/");
	ret = of_property_read_u32_array(root, "hisi,boardid", boardid, 4);
	if (ret < 0) {
		pr_err("Failed: board id read_u32.%d\n", ret);
		return 0xFFFFFFFF;
	}

	full_boardid = boardid[0] * 1000 + boardid[1] * 100 +
		boardid[2] * 10 + boardid[3];
	pr_info("Board ID.%x\n", full_boardid);

	return full_boardid;
}

/****************************************************************************
FUNCTION:
    get_ddr_nfo
DESCRIPTION:
    getting the DDR manufacturer id and memory size from xloader.
RETURN VALUE:
    0 -- error.
    nonzero value -- the register value,DDR manufacturer id and memory size.
****************************************************************************/

static unsigned int get_ddr_info(void)
{
	unsigned int tmp_reg_value;

	unsigned long *virtual_addr = (unsigned long *)ioremap_nocache
		(DDR_INFO_ADDR&0xFFFFF000, 0x800);

	if (NULL == virtual_addr) {
		pr_err("%s  ioremap ERROR !!\n", __func__);
		return 0;
	}
	pr_info("ddr_info va = 0x%lx\n", (unsigned long)virtual_addr);

	tmp_reg_value = *(unsigned long *)((unsigned long)virtual_addr +
		(DDR_INFO_ADDR&0x00000FFF));
	iounmap(virtual_addr);
	return tmp_reg_value;
}

static int ddr_info_show(struct seq_file *m, void *v)
{
	unsigned int tmp_reg_value;
	unsigned int board_id;

	tmp_reg_value = get_ddr_info();

	board_id = ddr_get_boardid();
	if (L29_VN1_2_BOARD == board_id)
		tmp_reg_value = (tmp_reg_value & 0xFF) | 0x300;

	tmp_reg_value = tmp_reg_value & 0xfff;
	seq_printf(m, "ddr_info:\n0x%x\n", tmp_reg_value);
	return 0;
}


static int ddrinfo_open(struct inode *inode, struct file *file)
{
	return single_open(file, ddr_info_show, NULL);
}

static const struct file_operations proc_ddrinfo_operations = {
	.open		= ddrinfo_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void get_rod_info(void)
{
#if defined(DDR_HI3650_PLATFORM)
	unsigned int tmp_info_value;
	unsigned int tmp_type_value;

	unsigned long *virtual_addr =  (unsigned long *)ioremap_nocache
		(DDR_INFO_ADDR&0xFFFFF000, 0x800);

	if (NULL == virtual_addr) {
		pr_err("%s  ioremap ERROR !!\n", __func__);
		return;
	}
	pr_info("ROD_INFO  va = 0x%lx\n", (unsigned long)virtual_addr);

	tmp_info_value = *(unsigned long *)((unsigned long)virtual_addr
		+ (DDR_INFO_ADDR&0x00000FFF));
	tmp_type_value = *(unsigned int *)((unsigned long)virtual_addr
		+ (DDR_TYPE_ADDR&0x00000FFF));
	iounmap(virtual_addr);

	if (tmp_type_value>>31 & 0x1) {
		if ((tmp_info_value&0xff) == 0x06) {
			pr_info("manufacture: Hynix; ddr_type: lpddr4!!\n");
			memcpy(ddr_rod, "hynix_lpddr4",
				strlen("hynix_lpddr4")+1);
		} else {
			pr_info("manufacture: UnHynix; ddr_type: lpddr4!!\n");
			memcpy(ddr_rod, "lpddr4", strlen("lpddr4")+1);
		}
	} else {
		pr_info("ddr_type: lpddr3!!\n");
		memcpy(ddr_rod, "lpddr3", strlen("lpddr3")+1);
	}
#elif defined(DDR_HI6250_PLATFORM)
	pr_info("ddr_type: lpddr3!!\n");
	memcpy(ddr_rod, "lpddr3", strlen("lpddr3")+1);
#elif defined(DDR_HI3660_PLATFORM)
	pr_info("ddr_type: lpddr4!!\n");
	memcpy(ddr_rod, "lpddr4", strlen("lpddr4")+1);
#else
	pr_info("ddr_type: lpddr!!\n");
	memcpy(ddr_rod, "lpddr", strlen("lpddr")+1);
#endif
}

static int ddr_rod_show(struct seq_file *m, void *v)
{
	get_rod_info();
	seq_printf(m, "%s\n", ddr_rod);
	return 0;
}

static int ddr_rod_open(struct inode *inode, struct file *file)
{
	return single_open(file, ddr_rod_show, NULL);
}

static const struct file_operations proc_ddrrod_operations = {
	.open		= ddr_rod_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_ddr_info_init(void)
{
	void *ret;

	ret = proc_create("ddr_info", S_IRUGO | S_IWUSR,
		NULL, &proc_ddrinfo_operations);
	if (NULL == ret)
		pr_err("%s /proc/ddr_info init ERROR !!\n", __func__);

	ret = proc_create("ddr_rod", S_IRUGO | S_IWUSR,
		NULL, &proc_ddrrod_operations);
	if (NULL == ret)
		pr_err("%s  /proc/ddr_rod init ERROR !!\n", __func__);
	return 0;
}

module_init(proc_ddr_info_init);
