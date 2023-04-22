/*
 * Header file for device driver Hi6423v100 PMIC
 */

#ifndef	__HISI_6423_PMIC_H
#define	__HISI_6423_PMIC_H

#include <linux/irqdomain.h>

struct hisi_6423_pmic {
	struct resource		*res;
	struct device		*dev;
	void __iomem		*regs;
	struct delayed_work   check_6423_vbatt_work;
	struct pinctrl *pctrl;
	struct pinctrl_state *pctrl_default;
	struct pinctrl_state *pctrl_idle;
	int		  	       irq;
	int                gpio;
	unsigned int                sid;
	unsigned int                irq_mask_addr;
	unsigned int                irq_addr;
	unsigned int                irq_np_record;
};

#endif		/* __HISI_6423_PMIC_H */
