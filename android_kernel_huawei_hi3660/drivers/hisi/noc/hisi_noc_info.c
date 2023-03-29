/*
* NoC. (NoC Mntn Module.)
*
* Copyright (c) 2016 Huawei Technologies CO., Ltd.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/bitops.h>
#include <linux/io.h>
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
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG HISI_NOC_TAG

#include "hisi_noc_info.h"
#include "hisi_noc.h"

/* !!NOTE: Used to caculate platform id defined in DTS!!*/
#define PLATFORM_ID_BIT_HI3650       (0U)
#define PLATFORM_ID_BIT_HI6250       (1U)
#define PLATFORM_ID_BIT_HI3660       (2U)
#define PLATFORM_ID_BIT_KIRIN970_ES  (3U)
#define PLATFORM_ID_BIT_KIRIN970     (4U)
#define PLATFORM_ID_BIT_MIA          (5U)
#define PLATFORM_ID_BIT_ATLA_ES      (6U)
#define PLATFORM_ID_BIT_ATLA         (7U)
#define PLATFORM_ID_BIT_ORLA         (8U)
#define PLATFORM_ID_BIT_PHOE_ES      (9U)
#define PLATFORM_ID_BIT_PHOE         (10U)

/*hisi platform noc bus info struct.*/
struct noc_platform_info {
	const char *name;
	unsigned int platform_id;
	const struct noc_bus_info *p_noc_info_bus;
	unsigned int noc_info_bus_len;
	struct noc_dump_reg *p_noc_info_dump;
	unsigned int noc_info_dump_len;
	const struct noc_busid_initflow *p_noc_info_filter_initflow;
	void (*pfun_get_size) (unsigned int *, unsigned int *);
	unsigned int (*pfun_clock_enable) (struct hisi_noc_device *,
					   struct noc_node *);
};

/*hisi platform noc bus info gloabl variable.*/
static struct noc_platform_info g_noc_platform_info[] = {
	/*hisi platform: hi3650 */
	[0] = {
	       .name = "hi3650",
	       .platform_id = 1 << PLATFORM_ID_BIT_HI3650,	/* must be same as the value defined in DTS. */
	       .p_noc_info_bus = noc_buses_info_hi3650,
	       .p_noc_info_dump = noc_dump_reg_list_hi3650,
	       .p_noc_info_filter_initflow = hisi_filter_initflow_hi3650,
	       .pfun_get_size = hisi_noc_get_array_size_hi3650,
	       .pfun_clock_enable = hisi_noc_clock_enable_hi3650,
	       },

	/*hisi platform: hi6250 */
	[1] = {
	       .name = "hi6250",
	       .platform_id = 1 << PLATFORM_ID_BIT_HI6250,	/* must be same as the value defined in DTS. */
	       .p_noc_info_bus = noc_buses_info_hi6250,
	       .p_noc_info_dump = noc_dump_reg_list_hi6250,
	       .p_noc_info_filter_initflow = hisi_filter_initflow_hi6250,
	       .pfun_get_size = hisi_noc_get_array_size_hi6250,
	       .pfun_clock_enable = hisi_noc_clock_enable_hi6250,
	       },

	/*hisi platform: hi3660 */
	[2] = {
	       .name = "hi3660",
	       .platform_id = 1 << PLATFORM_ID_BIT_HI3660,	/* must be same as the value defined in DTS. */
	       .p_noc_info_bus = noc_buses_info_hi3660,
	       .p_noc_info_dump = noc_dump_reg_list_hi3660,
	       .p_noc_info_filter_initflow = hisi_filter_initflow_hi3660,
	       .pfun_get_size = hisi_noc_get_array_size_hi3660,
	       .pfun_clock_enable = hisi_noc_clock_enable_hi3660,
	       },

	/*hisi platform: kirin970_es */
	[3] = {
	       .name = "kirin970_es",
	       .platform_id = 1 << PLATFORM_ID_BIT_KIRIN970_ES,	/* must be same as the value defined in DTS. */
	       .p_noc_info_bus = noc_buses_info_kirin970_es,
	       .p_noc_info_dump = noc_dump_reg_list_kirin970_es,
	       .p_noc_info_filter_initflow = hisi_filter_initflow_kirin970_es,
	       .pfun_get_size = hisi_noc_get_array_size_kirin970_es,
	       .pfun_clock_enable = hisi_noc_clock_enable_kirin970_es,
	       },

	/*hisi platform: kirin970 */
	[4] = {
	       .name = "kirin970",
	       .platform_id = 1 << PLATFORM_ID_BIT_KIRIN970,	/* must be same as the value defined in DTS. */
	       .p_noc_info_bus = noc_buses_info_kirin970,
	       .p_noc_info_dump = noc_dump_reg_list_kirin970,
	       .p_noc_info_filter_initflow = hisi_filter_initflow_kirin970,
	       .pfun_get_size = hisi_noc_get_array_size_kirin970,
	       .pfun_clock_enable = hisi_noc_clock_enable_kirin970,
	       },

	/*hisi platform: MIA */
	[5] = {
	       .name = "MIA",
	       .platform_id = 1 << PLATFORM_ID_BIT_MIA,	/* must be same as the value defined in DTS. */
	       .p_noc_info_bus = noc_buses_info_MIA,
	       .p_noc_info_dump = noc_dump_reg_list_MIA,
	       .p_noc_info_filter_initflow = hisi_filter_initflow_MIA,
	       .pfun_get_size = hisi_noc_get_array_size_MIA,
	       .pfun_clock_enable = hisi_noc_clock_enable_MIA,
	       },

	/*hisi platform: ATLA_es */
	[6] = {
	       .name = "ATLA_es",
	       .platform_id = 1 << PLATFORM_ID_BIT_ATLA_ES,	/* must be same as the value defined in DTS. */
	       .p_noc_info_bus = noc_buses_info_ATLA_es,
	       .p_noc_info_dump = noc_dump_reg_list_ATLA_es,
	       .p_noc_info_filter_initflow = hisi_filter_initflow_ATLA_es,
	       .pfun_get_size = hisi_noc_get_array_size_ATLA_es,
	       .pfun_clock_enable = hisi_noc_clock_enable_ATLA_es,
	       },

	/*hisi platform: ATLA */
	[7] = {
	       .name = "ATLA",
	       .platform_id = 1 << PLATFORM_ID_BIT_ATLA,	/* must be same as the value defined in DTS. */
	       .p_noc_info_bus = noc_buses_info_ATLA,
	       .p_noc_info_dump = noc_dump_reg_list_ATLA,
	       .p_noc_info_filter_initflow = hisi_filter_initflow_ATLA,
	       .pfun_get_size = hisi_noc_get_array_size_ATLA,
	       .pfun_clock_enable = hisi_noc_clock_enable_ATLA,
	       },

	/*hisi platform: ORLA */
	[8] = {
	       .name = "ORLA",
	       .platform_id = 1 << PLATFORM_ID_BIT_ORLA,	/* must be same as the value defined in DTS. */
	       .p_noc_info_bus = noc_buses_info_ORLA,
	       .p_noc_info_dump = noc_dump_reg_list_ORLA,
	       .p_noc_info_filter_initflow = hisi_filter_initflow_ORLA,
	       .pfun_get_size = hisi_noc_get_array_size_ORLA,
	       .pfun_clock_enable = hisi_noc_clock_enable_ORLA,
	      },

	/*hisi platform: PHOE_es */
	[9] = {
	       .name = "PHOE_es",
	       .platform_id = 1 << PLATFORM_ID_BIT_PHOE_ES,	/* must be same as the value defined in DTS. */
	       .p_noc_info_bus = noc_buses_info_PHOE_es,
	       .p_noc_info_dump = noc_dump_reg_list_PHOE_es,
	       .p_noc_info_filter_initflow = hisi_filter_initflow_PHOE_es,
	       .pfun_get_size = hisi_noc_get_array_size_PHOE_es,
	       .pfun_clock_enable = hisi_noc_clock_enable_PHOE_es,
	       },
	/*hisi platform: PHOE */
	[10] = {
	       .name = "PHOE",
	       .platform_id = 1 << PLATFORM_ID_BIT_PHOE,	/* must be same as the value defined in DTS. */
	       .p_noc_info_bus = noc_buses_info_PHOE,
	       .p_noc_info_dump = noc_dump_reg_list_PHOE,
	       .p_noc_info_filter_initflow = hisi_filter_initflow_PHOE,
	       .pfun_get_size = hisi_noc_get_array_size_PHOE,
	       .pfun_clock_enable = hisi_noc_clock_enable_PHOE,
	       }
};

static unsigned int g_info_index = 0;
static struct noc_arr_info noc_buses_i;
struct noc_dump_reg *noc_dump_reg_list = NULL;
/*if noc_happend's nodename is in the hisi_modemnoc_nodemame,
  firstly save log, then system reset.*/
static const char *hisi_modemnoc_nodemame[] = {
	"modem_bus",
	NULL,			/*end */
};

/*******************************************************************************
Function:       hisi_get_modemnoc_nodename
Description:    get the filter conditions of modemnoc.
Input:          NA
Output:         modemnoc_nodename:the filter condition of nodename
Return:         NA
********************************************************************************/
void hisi_get_modemnoc_nodename(char ***modemnoc_nodename)
{
	*modemnoc_nodename = (char **)hisi_modemnoc_nodemame;
}

/*
 * hisi_noc_get_bus_info_num -- return noc parse table pointer.
 */
unsigned int hisi_noc_get_bus_info_num(void)
{
	return noc_buses_i.len;
}

/*
 * hisi_noc_get_dump_reg_list_num -- return noc dump register list lenght.
 */
unsigned int hisi_noc_get_dump_reg_list_num(void)
{
	return g_noc_platform_info[g_info_index].noc_info_dump_len;
}

/*******************************************************************************
Function:       hisi_get_noc_initflow
Description:    get the filter conditions of modem,hifi etc noc.
Input:          NA
Output:         noc_initflow:the filter condition of initflow
Return:         NA
********************************************************************************/
void hisi_get_noc_initflow(const struct noc_busid_initflow **filter_initflow)
{
	*filter_initflow =
	    g_noc_platform_info[g_info_index].p_noc_info_filter_initflow;
}

/*
 * hisi_noc_clock_enable -- check noc device node clock state.
 * @noc_dev -- noc device node pointer;
 * return clock state of related node.
 */
unsigned int hisi_noc_clock_enable(struct hisi_noc_device *noc_dev,
				   struct noc_node *node)
{
	return g_noc_platform_info[g_info_index].pfun_clock_enable(noc_dev,
								   node);
}

/*
 * noc_set_buses_info -- Get NoC Error Address By Route-id Table.
 * @platform_id -- param from noc probe flow;
 * return 0 if success, otherwise, -1
 */
int noc_get_platform_info_index(unsigned int platform_id)
{
	unsigned int index;

	for (index = 0; index < ARRAY_SIZE_NOC(g_noc_platform_info); index++) {
		if (g_noc_platform_info[index].platform_id == platform_id)
			return index;
	}

	return -1;
}

/*
 * noc_set_buses_info -- get platform info define by platform id value.
 * @platform_id -- param from noc probe flow;
 * return 0 if success, otherwise, -1
 */
int noc_set_buses_info(unsigned int platform_id)
{
	int ret;

	/*get platform info index by platform id value defined in DTS. */
	ret = noc_get_platform_info_index(platform_id);
	if (ret < 0) {
		pr_err
		    ("[%s]: Error!! platform_id[%d], No platform id matched!!\n",
		     __func__, platform_id);
		return -1;
	}

	/*save platform info index. */
	g_info_index = ret;
	pr_crit("[%s]: info index is [%d], platform is:[%s].\n", __func__,
		g_info_index, g_noc_platform_info[g_info_index].name);

	/*get platform info array size. */
	g_noc_platform_info[g_info_index].
	    pfun_get_size(&g_noc_platform_info[g_info_index].noc_info_bus_len,
			  &g_noc_platform_info[g_info_index].noc_info_dump_len);
	if ((0 == g_noc_platform_info[g_info_index].noc_info_dump_len)
	    || (0 == g_noc_platform_info[g_info_index].noc_info_bus_len)) {
		pr_err("[%s]: Get noc info length Error!!\n", __func__);
		return -1;
	}

	/*save platform info. */
	noc_buses_i.ptr = g_noc_platform_info[g_info_index].p_noc_info_bus;
	noc_buses_i.len = g_noc_platform_info[g_info_index].noc_info_bus_len;
	noc_dump_reg_list = g_noc_platform_info[g_info_index].p_noc_info_dump;

	return 0;
}

/*
Function: noc_get_mid_info
Description: noc get mid_info and mid_info size
input: bus_id
output: noc_mid_info pointer and mid_info size
return: none
*/
void noc_get_mid_info(unsigned int bus_id, struct noc_mid_info **pt_info,
		      unsigned int *pt_size)
{
	const struct noc_bus_info *pt_noc_bus = noc_get_bus_info(bus_id);

	if (!pt_noc_bus) {
		pr_err("[%s]: pt_noc_bus is NULL\n", __func__);
		return;
	}
	*pt_info = (struct noc_mid_info *)(pt_noc_bus->p_noc_mid_info);
	*pt_size = pt_noc_bus->noc_mid_info_size;
	return;
}

/*
Function: noc_get_sec_info
Description: noc get sec_info and sec_info size
input: bus_id
output: noc_sec_info pointer and sec_info size
return: none
*/
void noc_get_sec_info(unsigned int bus_id, struct noc_sec_info **pt_info,
		      unsigned int *pt_size)
{
	const struct noc_bus_info *pt_noc_bus = noc_get_bus_info(bus_id);

	if (!pt_noc_bus) {
		pr_err("[%s]: pt_noc_bus is NULL\n", __func__);
		return;
	}
	*pt_info = (struct noc_sec_info *)(pt_noc_bus->p_noc_sec_info);
	*pt_size = pt_noc_bus->noc_sec_info_size;
	return;
}

/*
Function: noc_get_buses_info
Description: get noc_bus_info
input: none
output: none
return: noc_arr_info pointer
*/
struct noc_arr_info *noc_get_buses_info(void)
{
	return &noc_buses_i;
}

/*
Function: noc_get_bus_info
Description: get noc_bus_info from bus_id
input: int bus_id -> bus id input
output: none
return: noc_bus_info
*/
const struct noc_bus_info *noc_get_bus_info(unsigned int bus_id)
{
	const struct noc_bus_info *noc_bus = NULL;

	if (bus_id < noc_buses_i.len) {
		noc_bus = noc_buses_i.ptr;
		noc_bus += bus_id;
	}

	return noc_bus;
}

/*
 * Function: noc_find_addr_from_routeid
 * Description:  Get NoC Error Address By Route-id Table.
 * @unsigned int idx -- Bus Id;
 * @int initflow -- Access Init Flow
 * @int targetflow -- Access Target Flow
 * @int targetsubrange -- Target Subrange
 * Output: NA
 * Return: u64 -- NoC Error Local Address
 */
u64 noc_find_addr_from_routeid(unsigned int idx, int initflow, int targetflow,
			       int targetsubrange)
{
	unsigned int i;
	unsigned int count;
	const ROUTE_ID_ADDR_STRU *pastTbl;

	if (idx >= hisi_noc_get_bus_info_num())
		return 0;

	pastTbl = noc_buses_i.ptr[idx].routeid_tbl;
	count = noc_buses_i.ptr[idx].routeid_tbl_size;

	for (i = 0; i < count; i++) {
		if ((pastTbl[i].targ_flow == targetflow) &&
		    (pastTbl[i].targ_subrange == targetsubrange))

			return pastTbl[i].init_localaddr;
	}

	return 0;
}
