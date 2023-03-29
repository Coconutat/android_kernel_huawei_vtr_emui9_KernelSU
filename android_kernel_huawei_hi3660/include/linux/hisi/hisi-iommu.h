#ifndef _HI36XX_SMMU_H
#define _HI36XX_SMMU_H

#include <linux/types.h>
#include <linux/iommu.h>
#include <linux/platform_device.h>

extern struct iommu_domain *hisi_ion_enable_iommu(struct platform_device *pdev);
extern int of_get_iova_info(struct device_node *np, unsigned long *iova_start,
			    unsigned long *iova_size, unsigned long *iova_align);

struct iommu_domain_data {
	unsigned long     iova_start;
	unsigned long     iova_size;
	phys_addr_t      phy_pgd_base;
	unsigned long    iova_align;
	struct list_head list;
};

#endif
