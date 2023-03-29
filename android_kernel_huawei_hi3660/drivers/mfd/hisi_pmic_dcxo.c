/*
 * All other module's reference of nve.
 */

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/mtd/hisi_nve_number.h>
#include "../mtd/hisi_nve.h"
#include "securec.h"

#define NVE_CALC1C2_NAME "CALC1C2"
extern int pmu_dcxo_set(uint16_t dcxo_ctrim, uint16_t dcxo_c2_fix);
extern int pmu_dcxo_get(uint16_t *dcxo_ctrim, uint16_t *dcxo_c2_fix);
EXPORT_SYMBOL(pmu_dcxo_get);
EXPORT_SYMBOL(pmu_dcxo_set);
#ifndef CONFIG_HISI_PMIC_DCXO
int pmu_dcxo_set(uint16_t dcxo_ctrim, uint16_t dcxo_c2_fix){ return 0;}
int pmu_dcxo_get(uint16_t *dcxo_ctrim, uint16_t *dcxo_c2_fix){ return 0;}
#else

struct pmu_dcxo {
	uint16_t dcxo_ctrim;
	uint16_t dcxo_c2_fix;
	uint16_t calibra_times;
};

static int pmu_dcxo_get_set(
	uint16_t *dcxo_ctrim, uint16_t *dcxo_c2_fix, bool get)
{
	int ret;
	struct pmu_dcxo *dcxo;
	struct hisi_nve_info_user nve = {0};

	if (!dcxo_ctrim || !dcxo_c2_fix)
		return -EFAULT;

	strncpy_s(nve.nv_name, sizeof(nve.nv_name),
			NVE_CALC1C2_NAME, sizeof(nve.nv_name));
	nve.nv_number = NVE_CALC1C2_NUM;
	nve.valid_size = sizeof(struct pmu_dcxo);
	nve.nv_operation = NV_READ;

	ret = hisi_nve_direct_access(&nve);
	if (ret) {
		pr_err("[%s]nve get dcxo failed: 0x%x\n", __FUNCTION__, ret);
		return ret;
	}

	dcxo = (struct pmu_dcxo *)nve.nv_data;

	if (get) {
		*dcxo_ctrim = dcxo->dcxo_ctrim;
		*dcxo_c2_fix = dcxo->dcxo_c2_fix;
		pr_err("[%s]get dcxo ctrim = 0x%x, dcxo_c2_fix = 0x%x\n",
			__FUNCTION__, *dcxo_ctrim, *dcxo_c2_fix);
		return ret;
	}

	dcxo->calibra_times++;
	dcxo->dcxo_ctrim = *dcxo_ctrim;
	dcxo->dcxo_c2_fix = *dcxo_c2_fix;

	nve.nv_operation = NV_WRITE;

	ret = hisi_nve_direct_access(&nve);
	if (ret)
		pr_err("[%s]nve set dcxo failed: 0x%x\n", __FUNCTION__, ret);
	else
		pr_err("[%s]%d times setting dcxo\n", __FUNCTION__, dcxo->calibra_times);

	return ret;
}

int pmu_dcxo_set(uint16_t dcxo_ctrim, uint16_t dcxo_c2_fix)
{
	pr_err("[%s]set dcxo ctrim = 0x%x, dcxo_c2_fix = 0x%x\n",
		__FUNCTION__, dcxo_ctrim, dcxo_c2_fix);

	return pmu_dcxo_get_set(&dcxo_ctrim, &dcxo_c2_fix, false);
}

int pmu_dcxo_get(uint16_t *dcxo_ctrim, uint16_t *dcxo_c2_fix)
{
	return pmu_dcxo_get_set(dcxo_ctrim, dcxo_c2_fix, true);
}

int pmu_dcxo_get_test(int get, uint16_t v1, uint16_t v2)
{
	int ret;
	uint16_t dcxo_ctrim = v1;
	uint16_t dcxo_c2_fix = v2;

	ret = pmu_dcxo_get_set(&dcxo_ctrim, &dcxo_c2_fix, get);

	pr_err("[%s]test get trim value, ctrim = 0x%x, dcxo_c2_fix = "
	       "0x%x\n",
		__FUNCTION__, dcxo_ctrim, dcxo_c2_fix);

	return ret;
}

#endif
