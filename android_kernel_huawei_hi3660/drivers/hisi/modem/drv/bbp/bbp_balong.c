#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>

#include <bsp_dump.h>
#include <bsp_pm_om.h>

#include <hi_bbp.h>

void* g_bbp_acore_om_info_addr = NULL;
dump_handle g_bbp_acore_dump_handle;

void balong_bbp_get_om_info(int print_enable)
{
    u32 i;
    u32 value;
    void *pctrl_addr;

    printk(KERN_ERR"balong_bbp_dump_hook in...\n");

    pctrl_addr = ioremap(BBP_PCTRL_OM_BASE_ADDR, BBP_PCTRL_OM_BASE_SIZE);
    if (!pctrl_addr) {
        printk(KERN_ERR"fail to ioremap\n");
        return;
    }

    for (i = BBP_PCTRL_OM_CTRL_CMD_MIN; i <= BBP_PCTRL_OM_CTRL_CMD_MAX; i++) {
        writel(i, pctrl_addr+BBP_PCTRL_OM_CTRL_OFFSET);
        value = (u32)readl(pctrl_addr+BBP_PCTRL_OM_DATA_OFFSET);
        if (g_bbp_acore_om_info_addr) {
            writel(value, g_bbp_acore_om_info_addr + (i - BBP_PCTRL_OM_CTRL_CMD_MIN + 1)*4);
        }

        if ((!g_bbp_acore_om_info_addr) || (print_enable)) {
            printk(KERN_ERR"BBP om info, cmd: %08X, data:%08X\n", i, value);
        }
    }
    iounmap(pctrl_addr);

    printk(KERN_ERR"balong_bbp_dump_hook out...\n");
}

void balong_bbp_dump_hook(void)
{
    balong_bbp_get_om_info(0);
}

static int __init balong_bbp_acore_init(void)
{
    g_bbp_acore_dump_handle = bsp_dump_register_hook("BBP_ACore", balong_bbp_dump_hook);
    g_bbp_acore_om_info_addr = bsp_pm_dump_get(PM_OM_BBP, ((BBP_PCTRL_OM_CTRL_CMD_MAX - BBP_PCTRL_OM_CTRL_CMD_MIN) + 2) * 4);
    if (g_bbp_acore_om_info_addr) {
        writel(0x50424241U, g_bbp_acore_om_info_addr);  /* Magic Number: ABBP */
    }

    return 0;
}

module_init(balong_bbp_acore_init);/*lint -e528*/


