/*
 * blackbox cleartext. (kernel run data recorder clear text recording.)
 *
 * Copyright (c) 2013 Huawei Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/hisi/util.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/hisi/rdr_hisi_ap_ringbuffer.h>
#include "rdr_hisi_ap_adapter.h"
#include "../rdr_inner.h"
#include "../rdr_print.h"
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG HISI_BLACKBOX_TAG

/*
 * func name: is_addr_len_invalid
 *
 * [addr, addr + len] must be in the scope of reserved memory for core AP
 *
 * func args:
 * @addr: the physical start address of reserved trace memory to be printed in cleartext
 * @len: the length of reserved trace memory to be printed in cleartext
 * @log_addr: the head of reserved memory for core AP
 * @log_len: the total length of reserved memory for core AP
 *
 * return value
 * true invalid
 * false valid
 *
 */
static bool is_addr_len_invalid(u8 *addr, u32 len, u64 log_addr, u32 log_len)
{
	if (((u64)addr >= log_addr) && (len <= log_len) && ((u64)addr <= log_addr + log_len - len)) {
		return false;
	}

	BB_PRINT_ERR("%s() fail: addr 0x%pK len %u log_addr 0x%pK log_len %u.\n",
		__func__, addr, len, (u8 *)log_addr, log_len);
	return true;
}

/*
 * func name: get_real_addr
 *
 * [addr, addr + len] must be in the scope of reserved memory for core AP
 *
 * func args:
 * @addr: the physical start address of reserved trace memory to be printed in cleartext
 * @ap_root: the head of reserved memory for core AP
 * @log_addr: the start logical address of reserved memory for core AP
 *
 * return the logical start address of of reserved trace memory to be printed in cleartext
 *
 */
static inline u8 * get_real_addr(u8 *addr, AP_EH_ROOT *ap_root, u64 log_addr)
{
	u32 offset = addr - (u8 *)ap_root->rdr_ap_area_map_addr;

	return ((u8 *)log_addr + offset + PMU_RESET_RECORD_DDR_AREA_SIZE);
}

/*
 * func name: rdr_hisiap_cleartext_print_hk_cpuidle
 *
 * The clear text printing for the cpu on&off hook trace of core AP
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @log_addr: the start address of reserved memory for specified core
 * @log_len: the length of reserved memory for specified core
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
/*lint -e679*/
static int _rdr_hisiap_cleartext_print_hk_cpu_onoff(struct file   *fp,
                                                    u64           log_addr,
                                                    u32           log_len,
                                                    u8            *addr,
                                                    u32           len,
                                                    AP_EH_ROOT    *ap_root)
{
	struct hisiap_ringbuffer_s *q;
	cpu_onoff_info             *cpu_onoff;
	bool                       error;
	u32                        start, end, i;
	u8                         *real_addr;

	/* for cpu not online, will set addr NULL, so neglect it */
	if (unlikely(NULL == addr)) {
		return 0;
	}

	real_addr = get_real_addr(addr, ap_root, log_addr);
	if (unlikely(is_addr_len_invalid(real_addr, len, log_addr, log_len))) {
		BB_PRINT_ERR("%s() fail.\n", __func__);
		return -1;
	}

	q = (struct hisiap_ringbuffer_s *)real_addr;
	if (unlikely(is_ringbuffer_invalid(sizeof(cpu_onoff_info), len, q))) {
		BB_PRINT_ERR("%s() fail:check_ringbuffer_invalid.\n", __func__);
		return -1;
	}

	error = false;

	rdr_cleartext_print(fp, &error, "cpu_onoff\n");
	rdr_cleartext_print(fp, &error, "ktime             cpu on\n");

	/* ring buffer is empty, return directly */
	if (is_ringbuffer_empty(q)) {
		goto exit;
	}

	get_ringbuffer_start_end(q, &start, &end);
	for (i = start; i <= end; i++) {
		cpu_onoff = (cpu_onoff_info *)&q->data[(i % q->max_num) * q->field_count];
		rdr_cleartext_print(fp, &error, "%-18llu%-4u%-2u\n", 
			cpu_onoff->clock, cpu_onoff->cpu, cpu_onoff->on?1:0);
	}

exit:
	if (unlikely(true == error)) {
		return -1;
	}
	return 0;
}
/*lint +e679*/

/*
 * func name: rdr_hisiap_cleartext_print_hk_cpuidle
 *
 * The clear text printing for the cpu on&off hook trace of core AP
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @log_addr: the start address of reserved memory for specified core
 * @log_len: the length of reserved memory for specified core
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
static int rdr_hisiap_cleartext_print_hk_cpu_onoff(char *dir_path,
                                                   u64  log_addr,
                                                   u32  log_len)
{
	struct device_node *np;
	AP_EH_ROOT         *ap_root;
	struct file        *fp;
	int                ret = 0;
	u32                size = 0;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,rdr_ap_adapter");
	if (unlikely(!np)) {
		BB_PRINT_ERR(
		       "[%s], cannot find rdr_ap_adapter node in dts!\n",
		       __func__);
		return -1;
	}

	ret = of_property_read_u32(np, "ap_trace_cpu_on_off_size", &size);
	if (unlikely(ret)) {
		BB_PRINT_ERR(
		       "[%s], cannot find ap_trace_cpu_on_off_size in dts!\n",
		       __func__);
		return -1;
	}

	/* get the file descriptor from the specified directory path */
	fp = bbox_cleartext_get_filep(dir_path, "cpu_onoff.txt");
	if (unlikely(IS_ERR_OR_NULL(fp))) {
		BB_PRINT_ERR("%s() error:fp 0x%pK.\n", __func__, fp);
		return -1;
	}

	ap_root = (AP_EH_ROOT *)(log_addr + PMU_RESET_RECORD_DDR_AREA_SIZE);

	if (unlikely(_rdr_hisiap_cleartext_print_hk_cpu_onoff(fp, log_addr, 
					log_len, ap_root->hook_buffer_addr[HK_CPU_ONOFF], 
					size, ap_root))) {
		ret = -1;
		goto exit;
	}

exit:
	/* the cleaning of specified file descriptor */
	bbox_cleartext_end_filep(fp, dir_path, "cpu_onoff.txt");
	return ret;
}

/*
 * func name: rdr_hisiap_cleartext_print_hk_cpuidle
 *
 * The clear text printing for the cpu idle hook trace of core AP
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @log_addr: the start address of reserved memory for specified core
 * @log_len: the length of reserved memory for specified core
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
/*lint -e679*/
static int rdr_hisiap_cleartext_print_hk_cpuidle_on_cpu(struct file *fp,
                                                        u64         log_addr,
                                                        u32         log_len,
                                                        u8          *addr,
                                                        u32         len,
                                                        AP_EH_ROOT  *ap_root,
                                                        u32         cpu)
{
	struct hisiap_ringbuffer_s *q;
	cpuidle_info               *cpuidle;
	bool                       error;
	u32                        start, end, i;
	u8                         *real_addr;

	/* for cpu not online, will set addr NULL, so neglect it */
	if (unlikely(NULL == addr)) {
		return 0;
	}

	real_addr = get_real_addr(addr, ap_root, log_addr);
	if (unlikely(is_addr_len_invalid(real_addr, len, log_addr, log_len))) {
		BB_PRINT_ERR("%s() fail.\n", __func__);
		return -1;
	}

	q = (struct hisiap_ringbuffer_s *)real_addr;
	if (unlikely(is_ringbuffer_invalid(sizeof(cpuidle_info), len, q))) {
		BB_PRINT_ERR("%s() fail:check_ringbuffer_invalid.\n", __func__);
		return -1;
	}

	error = false;

	rdr_cleartext_print(fp, &error, "cpuidle cpu[%u]\n", cpu);
	rdr_cleartext_print(fp, &error, "ktime             dir\n");

	/* ring buffer is empty, return directly */
	if (is_ringbuffer_empty(q)) {
		goto exit;
	}

	get_ringbuffer_start_end(q, &start, &end);
	for (i = start; i <= end; i++) {
		cpuidle = (cpuidle_info *)&q->data[(i % q->max_num) * q->field_count];
		rdr_cleartext_print(fp, &error, "%-18llu%-3u\n", 
			cpuidle->clock, cpuidle->dir?1:0);
	}

exit:
	if (unlikely(true == error)) {
		return -1;
	}
	return 0;
}
/*lint +e679*/

/*
 * func name: rdr_hisiap_cleartext_print_hk_cpuidle
 *
 * The clear text printing for the idle hook trace of core AP
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @log_addr: the start address of reserved memory for specified core
 * @log_len: the length of reserved memory for specified core
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
static int rdr_hisiap_cleartext_print_hk_cpuidle(char *dir_path, u64 log_addr, u32 log_len)
{
	struct file *fp;
	AP_EH_ROOT  *ap_root;
	int         ret = 0;
	u32         i;

	/* get the file descriptor from the specified directory path */
	fp = bbox_cleartext_get_filep(dir_path, "cpuidle.txt");
	if (unlikely(IS_ERR_OR_NULL(fp))) {
		BB_PRINT_ERR("%s() error:fp 0x%pK.\n", __func__, fp);
		return -1;
	}

	ap_root = (AP_EH_ROOT *)(log_addr + PMU_RESET_RECORD_DDR_AREA_SIZE);

	for (i = 0; i < NR_CPUS; i++) {
		if (unlikely(rdr_hisiap_cleartext_print_hk_cpuidle_on_cpu(fp, log_addr, 
						log_len, ap_root->hook_percpu_buffer[HK_CPUIDLE].percpu_addr[i], 
						ap_root->hook_percpu_buffer[HK_CPUIDLE].percpu_length[i], ap_root, i))) {
			ret = -1;
			break;
		}
	}

	/* the cleaning of specified file descriptor */
	bbox_cleartext_end_filep(fp, dir_path, "cpuidle.txt");
	return ret;
}

/*
 * func name: rdr_hisiap_cleartext_print_hk_irq_on_cpu
 *
 * The clear text printing for the cpu irq hook trace of core AP
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @log_addr: the start address of reserved memory for specified core
 * @log_len: the length of reserved memory for specified core
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
/*lint -e679*/
static int rdr_hisiap_cleartext_print_hk_irq_on_cpu(struct file *fp,
                                                    u64         log_addr,
                                                    u32         log_len,
                                                    u8          *addr,
                                                    u32         len,
                                                    AP_EH_ROOT  *ap_root,
                                                    u32         cpu)
{
	struct hisiap_ringbuffer_s *q;
	irq_info                   *irq;
	bool                       error;
	u32                        start, end, i;
	u8                         *real_addr;

	/* for cpu not online, will set addr NULL, so neglect it */
	if (unlikely(NULL == addr)) {
		return 0;
	}

	real_addr = get_real_addr(addr, ap_root, log_addr);
	if (unlikely(is_addr_len_invalid(real_addr, len, log_addr, log_len))) {
		BB_PRINT_ERR("%s() fail.\n", __func__);
		return -1;
	}

	q = (struct hisiap_ringbuffer_s *)real_addr;
	if (unlikely(is_ringbuffer_invalid(sizeof(irq_info), len, q))) {
		BB_PRINT_ERR("%s() fail:check_ringbuffer_invalid.\n", __func__);
		return -1;
	}

	error = false;

	rdr_cleartext_print(fp, &error, "irq_switch cpu[%u]\n", cpu);
	rdr_cleartext_print(fp, &error, "ktime             slice          vec  dir\n");

	/* ring buffer is empty, return directly */
	if (is_ringbuffer_empty(q)) {
		goto exit;
	}

	get_ringbuffer_start_end(q, &start, &end);
	for (i = start; i <= end; i++) {
		irq = (irq_info *)&q->data[(i % q->max_num) * q->field_count];
		rdr_cleartext_print(fp, &error, "%-18llu%-15llu%-5u%-3u\n", 
			irq->clock, irq->jiff, irq->irq, irq->dir?1:0);
	}

exit:
	if (unlikely(true == error)) {
		return -1;
	}
	return 0;
}
/*lint +e679*/

/*
 * func name: rdr_hisiap_cleartext_print_hk_irq
 *
 * The clear text printing for the irq hook trace of core AP
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @log_addr: the start address of reserved memory for specified core
 * @log_len: the length of reserved memory for specified core
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
static int rdr_hisiap_cleartext_print_hk_irq(char *dir_path, u64 log_addr, u32 log_len)
{
	struct file *fp;
	AP_EH_ROOT  *ap_root;
	int         ret = 0;
	u32         i;

	/* get the file descriptor from the specified directory path */
	fp = bbox_cleartext_get_filep(dir_path, "last_kirq.txt");
	if (unlikely(IS_ERR_OR_NULL(fp))) {
		BB_PRINT_ERR("%s() error:fp 0x%pK.\n", __func__, fp);
		return -1;
	}

	ap_root = (AP_EH_ROOT *)(log_addr + PMU_RESET_RECORD_DDR_AREA_SIZE);

	for (i = 0; i < NR_CPUS; i++) {
		if (unlikely(rdr_hisiap_cleartext_print_hk_irq_on_cpu(fp, log_addr, 
						log_len, ap_root->hook_percpu_buffer[HK_IRQ].percpu_addr[i], 
						ap_root->hook_percpu_buffer[HK_IRQ].percpu_length[i], ap_root, i))) {
			ret = -1;
			break;
		}
	}

	/* the cleaning of specified file descriptor */
	bbox_cleartext_end_filep(fp, dir_path, "last_kirq.txt");
	return ret;
}

/*
 * func name: rdr_hisiap_cleartext_print_ap_root
 *
 * The clear text printing for the ap head of core AP
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @log_addr: the start address of reserved memory for specified core
 * @log_len: the length of reserved memory for specified core
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
static int rdr_hisiap_cleartext_print_ap_root(char *dir_path, u64 log_addr, u32 log_len)
{
	struct file *fp;
	AP_EH_ROOT  *ap_root;
	bool        error;
	u32         i;

	if ( unlikely(log_len < (sizeof(AP_EH_ROOT) + PMU_RESET_RECORD_DDR_AREA_SIZE)) ) {
		BB_PRINT_ERR("%s() error:log_len %u not enough.\n",
			__func__, log_len);
		return -1;
	}

	/* get the file descriptor from the specified directory path */
	fp = bbox_cleartext_get_filep(dir_path, "AP.txt");
	if (unlikely(IS_ERR_OR_NULL(fp))) {
		BB_PRINT_ERR("%s() error:fp 0x%pK.\n", __func__, fp);
		return -1;
	}

	ap_root = (AP_EH_ROOT *)(log_addr + PMU_RESET_RECORD_DDR_AREA_SIZE);
	error = false;

	ap_root->version[PRODUCT_VERSION_LEN - 1] = '\0';
	ap_root->device_id[PRODUCT_DEVICE_LEN - 1] = '\0';

	rdr_cleartext_print(fp, &error, "=================AP_EH_ROOT START================\n");
	rdr_cleartext_print(fp, &error, "dump_magic [0x%x]\n", ap_root->dump_magic);
	rdr_cleartext_print(fp, &error, "version [%s]\n", ap_root->version);
	rdr_cleartext_print(fp, &error, "modid [0x%x]\n", ap_root->modid);
	rdr_cleartext_print(fp, &error, "e_exce_type [0x%x],\n", ap_root->e_exce_type);
	rdr_cleartext_print(fp, &error, "e_exce_subtype [0x%x],\n", ap_root->e_exce_subtype);
	rdr_cleartext_print(fp, &error, "coreid [0x%llx]\n", ap_root->coreid);
	rdr_cleartext_print(fp, &error, "slice [%llu]\n", ap_root->slice);
	rdr_cleartext_print(fp, &error, "enter_times [0x%x]\n", ap_root->enter_times);
	rdr_cleartext_print(fp, &error, "bbox_version [0x%llx]\n", ap_root->bbox_version);
	rdr_cleartext_print(fp, &error, "num_reg_regions [0x%x]\n", ap_root->num_reg_regions);

	rdr_cleartext_print(fp, &error, "wdt_kick_slice:");
	for (i = 0; i < WDT_KICK_SLICE_TIMES; i++) {
		rdr_cleartext_print(fp, &error, "%llu ", ap_root->wdt_kick_slice[i]);
	}
	rdr_cleartext_print(fp, &error, "\n");
	rdr_cleartext_print(fp, &error, "device_id %s\n", (char *)(ap_root->device_id));
	rdr_cleartext_print(fp, &error, "=================AP_EH_ROOT END--================\n");

	/* For the formal commercial version, hook trace&last task trace& is closed */

	/* the cleaning of specified file descriptor */
	bbox_cleartext_end_filep(fp, dir_path, "AP.txt");

	if (unlikely(true == error)) {
		return -1;
	}

	return 0;
}

/*
 * func name: rdr_hisiap_cleartext_print
 *
 * The clear text printing for the reserved debug memory of core AP
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @log_addr: the start address of reserved memory for specified core
 * @log_len: the length of reserved memory for specified core
 *
 * return value
 *
 */
int rdr_hisiap_cleartext_print(char *dir_path, u64 log_addr, u32 log_len)
{
	if ( unlikely(IS_ERR_OR_NULL(dir_path) || IS_ERR_OR_NULL((void *)log_addr)) ) {
		BB_PRINT_ERR("%s() error:dir_path 0x%pK log_addr 0x%pK.\n", 
			__func__, dir_path, (void *)log_addr);
		return -1;
	}

	if (unlikely(rdr_hisiap_cleartext_print_ap_root(dir_path, log_addr, log_len))) {
		BB_PRINT_ERR("rdr_hisiap_cleartext_print_ap_root() ret error.\n");
		return -1;
	}

	if (unlikely(rdr_hisiap_cleartext_print_hk_irq(dir_path, log_addr, log_len))) {
		BB_PRINT_ERR("rdr_hisiap_cleartext_print_hk_irq() ret error.\n");
		return -1;
	}

	if (unlikely(rdr_hisiap_cleartext_print_hk_cpuidle(dir_path, log_addr, log_len))) {
		BB_PRINT_ERR("rdr_hisiap_cleartext_print_hk_cpuidle() ret error.\n");
		return -1;
	}

	if (unlikely(rdr_hisiap_cleartext_print_hk_cpu_onoff(dir_path, log_addr, log_len))) {
		BB_PRINT_ERR("rdr_hisiap_cleartext_print_hk_cpu_onoff() ret error.\n");
		return -1;
	}

	return 0;
}

