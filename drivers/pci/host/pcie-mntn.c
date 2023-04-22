#include "pcie-kirin.h"
#include "pcie-kirin-common.h"

#if defined (CONFIG_HUAWEI_DSM)
#define DSM_LOG_BUFFER_SIZE 256
#define RC_AER_OFFSET 0x100
#include <dsm/dsm_pub.h>

static u32 dsm_record_info[DSM_LOG_BUFFER_SIZE];
u32 info_size = 0;

void dsm_pcie_dump_info(struct kirin_pcie *pcie, enum dsm_err_id id)
{
	u32 i = 0;

	if (!pcie || !atomic_read(&(pcie->is_power_on)))
		return;

	dsm_record_info[i++] = id;
	dsm_record_info[i++] = gpio_get_value(pcie->gpio_id_reset);
	if (id != DSM_ERR_POWER_ON) {
		dsm_record_info[i++] = kirin_apb_phy_readl(pcie, SOC_PCIEPHY_STATE0_ADDR);
		dsm_record_info[i++] = kirin_apb_phy_readl(pcie, 0x488); //mplla
		dsm_record_info[i++] = kirin_elb_readl(pcie, SOC_PCIECTRL_STATE4_ADDR);
		dsm_record_info[i++] = kirin_elb_readl(pcie, SOC_PCIECTRL_STATE5_ADDR);
		dsm_record_info[i++] = kirin_elb_readl(pcie, PCIE_APP_LTSSM_ENABLE);

	}
	info_size = i;

	PCIE_PR_INFO("--");
}

void dsm_pcie_clear_info(void)
{
	u32 i;
	info_size = 0;
	for (i = 0; i < DSM_LOG_BUFFER_SIZE; i++)
		dsm_record_info[i] = 0;
}

/*
* return pcie dsm log_buffer addr
* log string is stroaged in this buffer
* param:
*	buf - storage register value for wifi dmd_log
*	buflen - buf size reserved for PCIe, max 384
*/
/*lint -e679 -esym(679,*) */
void dsm_pcie_dump_reginfo(char* buf, u32 buflen) {
    u32 i = 0;
    u32 seglen = 9;
    if (buf != NULL && buflen > (seglen * info_size)) {
        for (i = 0; i < info_size; i++) {
            snprintf(&buf[i * seglen], seglen + 1, "%08x ", dsm_record_info[i]);
        }
    }
}
EXPORT_SYMBOL_GPL(dsm_pcie_dump_reginfo);

#else

void dsm_pcie_dump_info(struct kirin_pcie *pcie, enum dsm_err_id id)
{
	return;
}

void dsm_pcie_clear_info(void)
{
	return;
}

/*
* return pcie dsm log_buffer addr
* log string is stroaged in this buffer
*/
void dsm_pcie_dump_reginfo(void *addr)
{
	return;
}
EXPORT_SYMBOL_GPL(dsm_pcie_dump_reginfo);
#endif


void dump_apb_register(struct kirin_pcie *pcie)
{
	u32 j;
#ifdef CONFIG_KIRIN_PCIE_MAR
	u32 val1, val2, val3, val4;
#endif

	if (!atomic_read(&pcie->is_power_on)) {
		PCIE_PR_ERR("PCIe is Poweroff");
		return;
	}

	PCIE_PR_INFO("####DUMP APB CORE Register : ");
	for (j = 0; j < 0x4; j++) {
		printk(KERN_INFO "0x%-8x: %8x %8x %8x %8x \n",
			0x10 * j,
			kirin_elb_readl(pcie, 0x10 * j + 0x0),
			kirin_elb_readl(pcie, 0x10 * j + 0x4),
			kirin_elb_readl(pcie, 0x10 * j + 0x8),
			kirin_elb_readl(pcie, 0x10 * j + 0xC));
	}

	for (j = 0; j < 0x2; j++) {
		printk(KERN_INFO "0x%-8x: %8x %8x %8x %8x \n",
			0x10 * j + 0x400,
			kirin_elb_readl(pcie, 0x10 * j + 0x0 + 0x400),
			kirin_elb_readl(pcie, 0x10 * j + 0x4 + 0x400),
			kirin_elb_readl(pcie, 0x10 * j + 0x8 + 0x400),
			kirin_elb_readl(pcie, 0x10 * j + 0xC + 0x400));
	}

        /* credit info*/
	for (j = 0; j < 0x3; j++) {
		printk(KERN_INFO "0x%-8x: %8x %8x %8x \n",
			0x4 * j + 0x430,
			kirin_elb_readl(pcie, 0x4 * j + 0x430),
			kirin_elb_readl(pcie, 0x4 * j + 0x430),
			kirin_elb_readl(pcie, 0x4 * j + 0x430));
	}

	PCIE_PR_INFO("####DUMP APB PHY Register : ");
	printk(KERN_INFO "0x%-8x: %8x %8x %8x %8x %8x ",
		0x0,
		kirin_apb_phy_readl(pcie, 0x0),
		kirin_apb_phy_readl(pcie, 0x4),
		kirin_apb_phy_readl(pcie, 0x8),
		kirin_apb_phy_readl(pcie, 0xc),
		kirin_apb_phy_readl(pcie, 0x400));
	PCIE_PR_INFO("MPLLA status: 0x%x", kirin_apb_phy_readl(pcie, 0x488));

#ifdef CONFIG_KIRIN_PCIE_MAR
	PCIE_PR_INFO("####DUMP SYNP PHY Register : ");

	if ((LTSSM_L1_2 == show_link_state(0)) || (LTSSM_L1_1 == show_link_state(0)))
		return;

	for (j = 0; j < 0xF0; j += 4) {
		val1 = kirin_natural_phy_readl(pcie, j + 0x0);
		val2 = kirin_natural_phy_readl(pcie, j + 0x1);
		val3 = kirin_natural_phy_readl(pcie, j + 0x2);
		val4 = kirin_natural_phy_readl(pcie, j + 0x3);
		printk(KERN_INFO "0x%-8x: %8x %8x %8x %8x \n",
			j, val1, val2, val3, val4);
	}

	for (j = 0x1000; j < 0x10A0; j += 4) {
		val1 = kirin_natural_phy_readl(pcie, j + 0x0);
		val2 = kirin_natural_phy_readl(pcie, j + 0x1);
		val3 = kirin_natural_phy_readl(pcie, j + 0x2);
		val4 = kirin_natural_phy_readl(pcie, j + 0x3);
		printk(KERN_INFO "0x%-8x: %8x %8x %8x %8x \n",
			j, val1, val2, val3, val4);
	}

	for (j = 0; j < 5; j++) {
		val1 = kirin_natural_phy_readl(pcie, 0x1059);
		printk(KERN_INFO "times[%d], 0x%-8x: %8x \n",
			j, 0x1059, val1);
	}

	for (j = 0; j < 5; j++) {
		val1 = kirin_natural_phy_readl(pcie, 0x404a);
		printk(KERN_INFO "times[%d], 0x%-8x: %8x \n",
			j, 0x404a, val1);
	}

	for (j = 0x2000; j < 0x2040; j += 4) {
		val1 = kirin_natural_phy_readl(pcie, j + 0x0);
		val2 = kirin_natural_phy_readl(pcie, j + 0x1);
		val3 = kirin_natural_phy_readl(pcie, j + 0x2);
		val4 = kirin_natural_phy_readl(pcie, j + 0x3);
		printk(KERN_INFO "0x%-8x: %8x %8x %8x %8x \n",
			j, val1, val2, val3, val4);
	}
	/* Invalid val */
	for (j = 0x3000; j < 0x30CF; j += 4) {
		val1 = kirin_natural_phy_readl(pcie, j + 0x0);
		val2 = kirin_natural_phy_readl(pcie, j + 0x1);
		val3 = kirin_natural_phy_readl(pcie, j + 0x2);
		val4 = kirin_natural_phy_readl(pcie, j + 0x3);
		printk(KERN_INFO "0x%-8x: %8x %8x %8x %8x \n",
			j, val1, val2, val3, val4);
	}
	/* all 0x0 */
	for (j = 0x4000; j < 0x4050; j += 4) {
		val1 = kirin_natural_phy_readl(pcie, j + 0x0);
		val2 = kirin_natural_phy_readl(pcie, j + 0x1);
		val3 = kirin_natural_phy_readl(pcie, j + 0x2);
		val4 = kirin_natural_phy_readl(pcie, j + 0x3);
		printk(KERN_INFO "0x%-8x: %8x %8x %8x %8x \n",
			j, val1, val2, val3, val4);
	}
#endif
	printk("\n");
}

typedef void (* WIFI_DUMP_FUNC) (void);
typedef void (* DEVICE_DUMP_FUNC) (void);
#ifdef CONFIG_KIRIN_PCIE_NOC_DBG
bool g_pcie_dump_flag = false;
WIFI_DUMP_FUNC g_device_dump = NULL;

bool is_pcie_target(int target_id)
{
	u32 i;

	for (i = 0; i < g_rc_num; i++)
		if (g_kirin_pcie[i].dtsinfo.noc_target_id == target_id)
			return true;

	return false;
}

void set_pcie_dump_flag(int target_id)
{
	u32 i;

	for (i = 0; i < g_rc_num; i++)
		if (g_kirin_pcie[i].dtsinfo.noc_target_id == target_id) {
			g_kirin_pcie[i].dtsinfo.noc_mntn = 1;
			return;
		}
}

void clear_pcie_dump_flag(void)
{
	u32 i;

	for (i = 0; i < g_rc_num; i++)
		if (g_kirin_pcie[i].dtsinfo.noc_mntn)
			g_kirin_pcie[i].dtsinfo.noc_mntn = 0;
}

bool get_pcie_dump_flag(void)
{
	u32 i;

	for (i = 0; i < g_rc_num; i++)
		if (g_kirin_pcie[i].dtsinfo.noc_mntn)
			return true;

	return false;
}

void dump_pcie_apb_info(void)
{
	struct kirin_pcie *pcie;
	u32 i;

	for (i = 0; i < g_rc_num; i++)
		if (g_kirin_pcie[i].dtsinfo.noc_mntn)
			pcie = &g_kirin_pcie[i];

	if (!atomic_read(&pcie->is_power_on)) {
		PCIE_PR_ERR("PCIe is Poweroff");
		return;
	}

	dump_apb_register(pcie);

	if (g_device_dump) {
		PCIE_PR_ERR("Dump wifi info");
		g_device_dump();
	}

	clear_pcie_dump_flag();
}

void register_device_dump_func(WIFI_DUMP_FUNC func)
{
	g_device_dump = func;
}
#else
bool is_pcie_target(int target_id)
{
	return false;
}

void set_pcie_dump_flag(int target_id)
{
	return;
}

void clear_pcie_dump_flag(void)
{
	return;
}

bool get_pcie_dump_flag(void)
{
	return false;
}

void dump_pcie_apb_info(void)
{
	return;
}

void register_device_dump_func(WIFI_DUMP_FUNC func)
{
	return;
}
#endif

void register_wifi_dump_func(WIFI_DUMP_FUNC func)
{
	register_device_dump_func(func);
}

EXPORT_SYMBOL_GPL(set_pcie_dump_flag);
EXPORT_SYMBOL_GPL(get_pcie_dump_flag);
EXPORT_SYMBOL_GPL(dump_pcie_apb_info);
EXPORT_SYMBOL_GPL(register_wifi_dump_func);
EXPORT_SYMBOL_GPL(register_device_dump_func);
EXPORT_SYMBOL_GPL(is_pcie_target);

