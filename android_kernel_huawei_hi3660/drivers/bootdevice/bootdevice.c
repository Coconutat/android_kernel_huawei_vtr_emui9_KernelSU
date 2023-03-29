#include <linux/bootdevice.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
/*
#define SCSOCID0 (0xE00)
*/
/*lint -e750 -esym(750,*)*/
#define SCINNERSTAT (0x3A0)
/*lint -e750 +esym(750,*)*/
#define EMMC_UFS_SEL BIT(15)
#define BOOT_EMMC 0
/*
#define BOOT_UFS 1
*/
static int __init bootdevice_init(void)
{
	int err;
	void __iomem *sysctrl;
	struct device_node *sys_np;
	struct device_node *boot_np;
	enum bootdevice_type type;

	sys_np = of_find_compatible_node(NULL, NULL, "hisilicon,sysctrl");

	if (!sys_np) {
		pr_err("can't find sysctrl, get bootdevice failed\n");
		err = -ENXIO;
		goto out;
	}

	sysctrl = of_iomap(sys_np, 0);
	if (!sysctrl) {
		pr_err("sysctrl of_iomap failed, can not get "
		       "bootdevice type\n");
		err = -ENOMEM;
		goto out;
	}

	boot_np = of_find_compatible_node(NULL, NULL, "hisilicon,bootdevice");
	if (!boot_np) {
		pr_err("can't find bootdevice dts node\n");
		err = -ENODEV;
		goto sys_unmap;
	}

	if (of_find_property(boot_np, "boot-from-emmc", NULL))
		type = BOOT_DEVICE_EMMC;
	else if (of_find_property(boot_np, "boot-from-ufs", NULL))
		type = BOOT_DEVICE_UFS;
	else {
		type = ((readl(sysctrl + SCINNERSTAT) & EMMC_UFS_SEL) ==
			BOOT_EMMC) ?
			       BOOT_DEVICE_EMMC :
			       BOOT_DEVICE_UFS;
	}

	set_bootdevice_type(type);
	pr_info("storage bootdevice: %s\n",
		type == BOOT_DEVICE_EMMC ? "eMMC" : "UFS");
	err = 0;

sys_unmap:
	iounmap(sysctrl);
out:
	return err;
}
arch_initcall(bootdevice_init);
