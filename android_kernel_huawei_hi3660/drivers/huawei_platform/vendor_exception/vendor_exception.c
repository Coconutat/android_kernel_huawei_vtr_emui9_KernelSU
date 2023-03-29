/*
 * vendor_exception.c
 *
 * Copyright (C) 2018 Huawei Device Co., Ltd.
 * This file is licensed under GPL.
 *
 * Register vendor exceptions.
 */

#include <linux/module.h>
#include <mntn_subtype_exception.h>
#include <huawei_platform/vendor_exception/vendor_exception.h>

static int __init vendor_exception_init(void)
{
	struct rdr_exception_info_s einfo[] = {
		{
			.e_modid = VENDOR_MODID_AP_S_CFI,
			.e_modid_end = VENDOR_MODID_AP_S_CFI,
			.e_process_priority = RDR_ERR,
			.e_reboot_priority = RDR_REBOOT_NOW,
			.e_notify_core_mask = RDR_AP,
			.e_reset_core_mask = RDR_AP,
			.e_from_core = RDR_AP,
			.e_reentrant = RDR_REENTRANT_DISALLOW,
			.e_exce_type = AP_S_VENDOR_PANIC,
			.e_exce_subtype = HI_APVNDPANIC_CFI,
			.e_upload_flag = RDR_UPLOAD_YES,
			.e_from_module = "VENDOR AP CFI",
			.e_desc = "VENDOR AP CFI"
		},
	};

	size_t i;
	for (i = 0; i < sizeof(einfo)/sizeof(einfo[0]); ++i) {
		u32 expect = einfo[i].e_modid_end;
		u32 result = rdr_register_exception(&einfo[i]);
		if (result != expect) {
			printk(KERN_ERR "%s: rdr_register_exception expect %u but %u\n",
					__func__, expect, result);
			return -1;
		}
	}
	return 0;
}

module_init(vendor_exception_init);
MODULE_LICENSE("GPL");
