#ifndef __HISI_NOC_INFO_H
#define __HISI_NOC_INFO_H

#include <linux/module.h>
#include <linux/bitops.h>
#include <linux/io.h>
/*
* NoC. (NoC Mntn Module.)
*
* Copyright (c) 2016 Huawei Technologies CO., Ltd.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/syscore_ops.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/string.h>
#include <linux/hisi/util.h>

#include "hisi_noc.h"

#define ARRAY_END_FLAG		0xffffffff
#define ARRAY_SIZE_NOC(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef struct datapath_routid_addr {
	int init_flow;
	int targ_flow;
	int targ_subrange;
	u64 init_localaddr;
} ROUTE_ID_ADDR_STRU;

struct noc_mid_info {
	unsigned int idx;	/*Bus Id: 0 */
	int init_flow;
	int mask;
	int mid_val;
	char *mid_name;
};

struct noc_sec_info {
	unsigned int mask;
	unsigned int sec_val;
	char *sec_array;
	char *sec_mode;
};

/* keep target route id, initiator flow id etc*/
struct noc_bus_info {
	char *name;
	unsigned int initflow_mask;
	unsigned int initflow_shift;

	unsigned int targetflow_mask;
	unsigned int targetflow_shift;

	unsigned int targ_subrange_mask;
	unsigned int seq_id_mask;

	char **initflow_array;
	unsigned int initflow_array_size;

	char **targetflow_array;
	unsigned int targetflow_array_size;

	const ROUTE_ID_ADDR_STRU *routeid_tbl;
	unsigned int routeid_tbl_size;

	unsigned int opc_mask;
	unsigned int opc_shift;
	char **opc_array;
	unsigned int opc_array_size;

	unsigned int err_code_mask;
	unsigned int err_code_shift;
	char **err_code_array;
	unsigned int err_code_array_size;

	struct noc_mid_info *p_noc_mid_info;
	unsigned int noc_mid_info_size;
	struct noc_sec_info *p_noc_sec_info;
	unsigned int noc_sec_info_size;
};

struct noc_arr_info {
	const struct noc_bus_info *ptr;
	unsigned int len;
};

/*for backup noc_errorprobe info*/
struct noc_errorprobe_backup_info {
	unsigned int bus_id;
	int init_flow;
	char *nodename;
};

/*for modemnoc_initflow array*/
struct noc_busid_initflow {
	unsigned int bus_id;
	int init_flow;
	int coreid;
};

struct noc_dump_reg {
	char *name;
	void __iomem *addr;
	unsigned int offset;
};

extern struct noc_dump_reg *noc_dump_reg_list;

extern const struct noc_bus_info noc_buses_info_hi3650[];
extern struct noc_mid_info noc_mid_hi3650[];
extern struct noc_dump_reg noc_dump_reg_list_hi3650[];
extern const struct noc_busid_initflow hisi_filter_initflow_hi3650[];

extern const struct noc_bus_info noc_buses_info_hi6250[];
extern struct noc_mid_info noc_mid_hi6250[];
extern struct noc_dump_reg noc_dump_reg_list_hi6250[];
extern const struct noc_busid_initflow hisi_filter_initflow_hi6250[];

extern const struct noc_bus_info noc_buses_info_hi3660[];
extern struct noc_mid_info noc_mid_hi3660[];
extern struct noc_dump_reg noc_dump_reg_list_hi3660[];
extern const struct noc_busid_initflow hisi_filter_initflow_hi3660[];

extern const struct noc_bus_info noc_buses_info_kirin970_es[];
extern struct noc_mid_info noc_mid_kirin970_es[];
extern struct noc_dump_reg noc_dump_reg_list_kirin970_es[];
extern const struct noc_busid_initflow hisi_filter_initflow_kirin970_es[];

extern const struct noc_bus_info noc_buses_info_ATLA_es[];
extern struct noc_mid_info noc_mid_ATLA_es[];
extern struct noc_dump_reg noc_dump_reg_list_ATLA_es[];
extern const struct noc_busid_initflow hisi_filter_initflow_ATLA_es[];

extern const struct noc_bus_info noc_buses_info_ATLA[];
extern struct noc_mid_info noc_mid_ATLA[];
extern struct noc_dump_reg noc_dump_reg_list_ATLA[];
extern const struct noc_busid_initflow hisi_filter_initflow_ATLA[];

extern const struct noc_bus_info noc_buses_info_ORLA[];
extern struct noc_mid_info noc_mid_ORLA[];
extern struct noc_dump_reg noc_dump_reg_list_ORLA[];
extern const struct noc_busid_initflow hisi_filter_initflow_ORLA[];

extern const struct noc_bus_info noc_buses_info_kirin970[];
extern struct noc_mid_info noc_mid_kirin970[];
extern struct noc_dump_reg noc_dump_reg_list_kirin970[];
extern const struct noc_busid_initflow hisi_filter_initflow_kirin970[];

extern const struct noc_bus_info noc_buses_info_MIA[];
extern struct noc_mid_info noc_mid_MIA[];
extern struct noc_dump_reg noc_dump_reg_list_MIA[];
extern const struct noc_busid_initflow hisi_filter_initflow_MIA[];

extern const struct noc_bus_info noc_buses_info_PHOE_es[];
extern struct noc_mid_info noc_mid_PHOE_es[];
extern struct noc_dump_reg noc_dump_reg_list_PHOE_es[];
extern const struct noc_busid_initflow hisi_filter_initflow_PHOE_es[];

extern const struct noc_bus_info noc_buses_info_PHOE[];
extern struct noc_mid_info noc_mid_PHOE[];
extern struct noc_dump_reg noc_dump_reg_list_PHOE[];
extern const struct noc_busid_initflow hisi_filter_initflow_PHOE[];

int noc_set_buses_info(unsigned int info_index);
void noc_get_mid_info(unsigned int bus_id, struct noc_mid_info **pt_info,
		      unsigned int *pt_size);
void noc_get_sec_info(unsigned int bus_id, struct noc_sec_info **pt_info,
		      unsigned int *pt_size);
struct noc_arr_info *noc_get_buses_info(void);

extern const struct noc_bus_info *noc_get_bus_info(unsigned int bus_id);
extern unsigned int hisi_noc_get_bus_info_num(void);
extern unsigned int hisi_noc_get_dump_reg_list_num(void);
extern void hisi_get_noc_initflow(const struct noc_busid_initflow
				  **filter_initflow);
extern void hisi_get_modemnoc_nodename(char ***modemnoc_nodename);
extern void hisi_noc_get_array_size_hi3650(unsigned int *bus_info_size,
					     unsigned int *dump_list_size);
extern void hisi_noc_get_array_size_hi6250(unsigned int *bus_info_size,
					     unsigned int *dump_list_size);
extern void hisi_noc_get_array_size_hi3660(unsigned int *bus_info_size,
					     unsigned int *dump_list_size);
extern void hisi_noc_get_array_size_kirin970_es(unsigned int *bus_info_size,
						 unsigned int *dump_list_size);
extern void hisi_noc_get_array_size_kirin970(unsigned int *bus_info_size,
					     unsigned int *dump_list_size);
extern void hisi_noc_get_array_size_ATLA_es(unsigned int *bus_info_size,
		                 unsigned int *dump_list_size);
extern void hisi_noc_get_array_size_ATLA(unsigned int *bus_info_size,
		                 unsigned int *dump_list_size);
extern void hisi_noc_get_array_size_MIA(unsigned int *bus_info_size,
					     unsigned int *dump_list_size);
extern void hisi_noc_get_array_size_ORLA(unsigned int *bus_info_size,
		                 unsigned int *dump_list_size);
extern void hisi_noc_get_array_size_PHOE_es(unsigned int *bus_info_size,
					     unsigned int *dump_list_size);
extern void hisi_noc_get_array_size_PHOE(unsigned int *bus_info_size,
					     unsigned int *dump_list_size);

extern unsigned int hisi_noc_clock_enable(struct hisi_noc_device *noc_dev,
					  struct noc_node *node);
extern unsigned int hisi_noc_clock_enable_hi3650(struct hisi_noc_device
						 *noc_dev,
						 struct noc_node *node);
extern unsigned int hisi_noc_clock_enable_hi6250(struct hisi_noc_device
						 *noc_dev,
						 struct noc_node *node);
extern unsigned int hisi_noc_clock_enable_hi3660(struct hisi_noc_device
						 *noc_dev,
						 struct noc_node *node);
extern unsigned int hisi_noc_clock_enable_kirin970_es(struct hisi_noc_device
						      *noc_dev,
						      struct noc_node *node);
extern unsigned int hisi_noc_clock_enable_ATLA_es(struct hisi_noc_device
		                      *noc_dev,
		                      struct noc_node *node);
extern unsigned int hisi_noc_clock_enable_ATLA(struct hisi_noc_device
		                      *noc_dev,
		                      struct noc_node *node);
extern unsigned int hisi_noc_clock_enable_kirin970(struct hisi_noc_device
						   *noc_dev,
						   struct noc_node *node);
extern unsigned int hisi_noc_clock_enable_MIA(struct hisi_noc_device
						   *noc_dev,
						   struct noc_node *node);
extern unsigned int hisi_noc_clock_enable_ORLA(struct hisi_noc_device
		                      *noc_dev,
		                      struct noc_node *node);
extern unsigned int hisi_noc_clock_enable_PHOE_es(struct hisi_noc_device
						   *noc_dev,
						   struct noc_node *node);
extern unsigned int hisi_noc_clock_enable_PHOE(struct hisi_noc_device
						   *noc_dev,
						   struct noc_node *node);

#endif
