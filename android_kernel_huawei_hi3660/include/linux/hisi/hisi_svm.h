/*
 *
 * Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __HISI_SVM_H
#define __HISI_SVM_H
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/mmu_notifier.h>

struct hisi_svm {
	char                *name;
	struct device       *dev;
	struct mm_struct    *mm;
	struct mmu_notifier  mn;
	struct iommu_domain *dom;
	unsigned long        l2addr;
	struct dentry       *debug_root;
};

int hisi_smmu_poweron(int smmuid);
int hisi_smmu_poweroff(int smmuid);
void hisi_svm_unbind_task(struct hisi_svm *svm);
struct hisi_svm *hisi_svm_bind_task(struct device *dev, struct task_struct *task);
void *hisi_svm_get_l2buf_pte(struct hisi_svm *svm, unsigned long addr);
void hisi_svm_show_pte(struct hisi_svm *svm, unsigned long addr, size_t size);
int hisi_svm_get_ssid(struct hisi_svm *svm, u16 *ssid,  u64 *ttbr, u64 *tcr);

#endif