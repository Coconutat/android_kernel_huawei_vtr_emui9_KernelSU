/*
* NoC. (NoC Mntn Module.)
*
* Copyright (c) 2016 Huawei Technologies CO., Ltd.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#ifndef __HISI_ERR_PROBE
#define __HISI_ERR_PROBE

#include "hisi_noc.h"

/* register bit offset*/
#define ERR_PROBE_CLEAR_BIT     BIT(0)
#define ERR_PORBE_ENABLE_BIT    BIT(0)
#define ERR_PORBE_ERRVLD_BIT    BIT(0)

/*global variables define here, starts*/

struct noc_err_probe_val {
	int val;
	char *pt_str;
};
struct err_probe_msg {
	struct noc_err_probe_val err_code;
	struct noc_err_probe_val opc;
	struct noc_err_probe_val init_flow;
	struct noc_err_probe_val target_flow;
	int targetsubrange;
	u64 base_addr;
	uint addr_low;
	uint addr_high;
	uint user_signal;
	char *mid_name;
	uint SECURITY;
};

/* declare functions */
void noc_err_probe_init(void);
int noc_err_save_noc_dev(struct hisi_noc_device *noc_dev);
void enable_err_probe(void __iomem *base);
void disable_err_probe(void __iomem *base);
void enable_err_probe_by_name(const char *name);
void disable_err_probe_by_name(char *name);
void noc_err_get_msg(uint *reg_val_buf, uint idx,
		     struct err_probe_msg *pt_err_msg);
void noc_timeout_handler_wq(void);
void noc_err_probe_exit(void);
#endif
