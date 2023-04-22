#include <linux/io.h>
#include <libhwsecurec/securec.h>

#include <linux/mfd/hisi_pmic.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/hisi/util.h>
#include <linux/hisi/rdr_hisi_platform.h>
#include <mntn_subtype_exception.h>
#include "../rdr_inner.h"
#include "../rdr_print.h"
#include "rdr_hisi_ap_adapter.h"
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG HISI_BLACKBOX_TAG

extern struct cmdword reboot_reason_map[];

static char g_subtype_name[RDR_REBOOT_REASON_LEN] = "undef";
static u32 g_subtype;


/* subtype exception map */
#undef __MMC_EXCEPTION_SUBTYPE_MAP
#define __MMC_EXCEPTION_SUBTYPE_MAP(x, y, z) {x, #y, #z, z},

#undef __AP_PANIC_SUBTYPE_MAP
#define __AP_PANIC_SUBTYPE_MAP(x, y, z) {x, #y, #z, z},

#undef __AP_BL31PANIC_SUBTYPE_MAP
#define __AP_BL31PANIC_SUBTYPE_MAP(x, y, z) {x, #y, #z, z},

#undef __AP_VENDOR_PANIC_SUBTYPE_MAP
#define __AP_VENDOR_PANIC_SUBTYPE_MAP(x, y, z) {x, #y, #z, z},

#undef __APWDT_HWEXC_SUBTYPE_MAP
#define __APWDT_HWEXC_SUBTYPE_MAP(x, y, z) {x, #y":hw", #z, z},

#undef __APWDT_EXC_SUBTYPE_MAP
#define __APWDT_EXC_SUBTYPE_MAP(x, y, z) {x, #y, #z, z},

#undef __LPM3_EXC_SUBTYPE_MAP
#define __LPM3_EXC_SUBTYPE_MAP(x, y, z) {x, #y, #z, z},

#undef __SCHARGER_EXC_SUBTYPE_MAP
#define __SCHARGER_EXC_SUBTYPE_MAP(x, y, z) {x, #y, #z, z},

#undef __PMU_EXC_SUBTYPE_MAP
#define __PMU_EXC_SUBTYPE_MAP(x, y, z) {x, #y, #z, z},

#undef __NPU_EXC_SUBTYPE_MAP
#define __NPU_EXC_SUBTYPE_MAP(x, y, z) {x, #y, #z, z},

#undef __CONN_EXC_SUBTYPE_MAP
#define __CONN_EXC_SUBTYPE_MAP(x, y, z) {x, #y, #z, z},

#undef __HISEE_EXC_SUBTYPE_MAP
#define __HISEE_EXC_SUBTYPE_MAP(x, y, z) {x, #y, #z, z},

struct exp_subtype exp_subtype_map[] = {
	#include <mntn_subtype_exception_map.h>
};

#undef __MMC_EXCEPTION_SUBTYPE_MAP
#undef __AP_PANIC_SUBTYPE_MAP
#undef __AP_BL31PANIC_SUBTYPE_MAP
#undef __AP_VENDOR_PANIC_SUBTYPE_MAP
#undef __APWDT_HWEXC_SUBTYPE_MAP
#undef __APWDT_EXC_SUBTYPE_MAP
#undef __LPM3_EXC_SUBTYPE_MAP
#undef __SCHARGER_EXC_SUBTYPE_MAP
#undef __PMU_EXC_SUBTYPE_MAP
#undef __NPU_EXC_SUBTYPE_MAP
#undef __CONN_EXC_SUBTYPE_MAP
#undef __HISEE_EXC_SUBTYPE_MAP

u32 get_exception_subtype_map_size(void)
{
	return (unsigned int)(sizeof(exp_subtype_map) /
		sizeof(exp_subtype_map[0]));
}

u32 get_category_value(u32 e_exce_type)
{
	u32 i, category = 0;

	for (i = 0; i < get_reboot_reason_map_size(); i++) {
		if (reboot_reason_map[i].num == e_exce_type) {
			category = reboot_reason_map[i].category_num;
			break;
		}
	}
	return category;
}


char *rdr_get_subtype_name(u32 e_exce_type,u32 subtype)
{
	u32 i;

	if (SUBTYPE == get_category_value(e_exce_type))
		for (i = 0; (unsigned int)i < get_exception_subtype_map_size(); i++) {
			if (exp_subtype_map[i].exception == e_exce_type &&
				exp_subtype_map[i].subtype_num == subtype)
				return (char *)exp_subtype_map[i].subtype_name;
		}

	return NULL;
}



char *rdr_get_category_name(u32 e_exce_type, u32 subtype)
{
	int i, category = 0;
	char *type = "UNDEF";

	category = get_category_value(e_exce_type);

	if (SUBTYPE == category) {
		for (i = 0; (unsigned int)i < get_exception_subtype_map_size(); i++) {
			if (exp_subtype_map[i].exception == e_exce_type &&
				exp_subtype_map[i].subtype_num == subtype) {
				return (char *)exp_subtype_map[i].category_name;
			}
		}
	} else {
		for (i = 0; (unsigned int)i < get_reboot_reason_map_size(); i++) {
			if (reboot_reason_map[i].num == e_exce_type) {
			return (char *)reboot_reason_map[i].category_name;
			}
		}
	}
	return type;
}



void set_subtype_exception(unsigned int subtype, bool save_value)
{
	unsigned int value = 0;
	unsigned long long pmu_reset_reg;

	if (FPGA == g_bbox_fpga_flag) {
		pmu_reset_reg = get_pmu_subtype_reg();
		if (pmu_reset_reg) {
			value = readl((char *)pmu_reset_reg);
		}
	} else {
		value = hisi_pmic_reg_read(PMU_EXCSUBTYPE_REG_OFFSET);/*lint !e747*/
	}
	if (save_value == false)
		value &= (PMU_RESET_REG_MASK);

	subtype &= (RST_FLAG_MASK);
	value |= subtype;
	if (FPGA == g_bbox_fpga_flag) {
		pmu_reset_reg = get_pmu_subtype_reg();
		if (pmu_reset_reg) {
			writel(value, (char *)pmu_reset_reg);
		}
	} else {
		hisi_pmic_reg_write(PMU_EXCSUBTYPE_REG_OFFSET, value);/*lint !e747*/
	}
	pr_info("[%s]:set subtype is 0x%x\n", __func__, value);

}


unsigned int get_subtype_exception(void)
{
	unsigned int value = 0;
	unsigned long long pmu_reset_reg;

	if (FPGA == g_bbox_fpga_flag) {
		pmu_reset_reg = get_pmu_reset_reg();
		if (pmu_reset_reg) {
			value = readl((char *)pmu_reset_reg);
		}
	} else {
		value = hisi_pmic_reg_read(PMU_EXCSUBTYPE_REG_OFFSET);/*lint !e747*/
	}
	value &= RST_FLAG_MASK;
	return value;

}

char *rdr_get_exec_subtype(void)
{
	return g_subtype_name;
}

u32 rdr_get_exec_subtype_value(void)
{
	return g_subtype;
}
static int __init early_parse_exec_subtype_cmdline(char *exec_subtype_cmdline)
{
	int i;

	memset_s(g_subtype_name, RDR_REBOOT_REASON_LEN, 0x0, RDR_REBOOT_REASON_LEN);
	if (memcpy_s(g_subtype_name, RDR_REBOOT_REASON_LEN, exec_subtype_cmdline,
                RDR_REBOOT_REASON_LEN - 1)) {
		BB_PRINT_ERR("failed to memcpy_s exception subtype.\n");
		return -1;
	}

	for (i = 0; (u32)i < get_exception_subtype_map_size(); i++) {
		if (!strncmp((char *)exp_subtype_map[i].subtype_name, g_subtype_name, RDR_REBOOT_REASON_LEN)) {
			g_subtype = exp_subtype_map[i].subtype_num;
			break;
		}
	}
	pr_info("[%s][%s][%d]\n", __func__, g_subtype_name, g_subtype);
	return 0;
}

early_param("exception_subtype", early_parse_exec_subtype_cmdline);

