

/*******************************************************************************
  1 头文件包含
 *******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/thread_info.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/kmsg_dump.h>
#include <linux/hisi/mntn_record_sp.h>
#include <linux/io.h>
#include <linux/kallsyms.h>
#include <linux/blkdev.h>
#include <linux/hisi/util.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/hisi/hisi_bootup_keypoint.h>
#include <linux/hisi/rdr_hisi_ap_ringbuffer.h>
#include <linux/hisi/mntn_dump.h>
#include <libhwsecurec/securec.h>
#include <mntn_subtype_exception.h>
#include <linux/hisi/rdr_hisi_ap_hook.h>
#include <linux/hisi/rdr_hisi_platform.h>

#include "../bl31/hisi_bl31_exception.h"
#include "platform_ap/rdr_hisi_ap_adapter.h"
#include "rdr_inner.h"
#include "rdr_print.h"
#include "rdr_field.h"


#define TICK_PER_SECOND        (32768)
#define MIN_EXCEPTION_TIME_GAP (5*60*TICK_PER_SECOND) /* 5minute, 32768 32k time tick per second */

typedef struct {
	u32 offset;
	u32 size;
} exception_core_t;

static struct rdr_register_module_result current_info;
static exception_core_t                  g_exception_core[EXCEPTION_TRACE_ENUM];
static u8                                *g_exception_trace_addr = NULL;
static                                   DEFINE_SPINLOCK(rdr_exception_trace_lock);

/*
 * func name: rdr_exception_analysis_ap
 *
 * in the case of reboot reason error reported, we must correct it to the real
 * reboot reason.
 * The way is to traverse each recorded exception trace, select the most early
 * exception.
 * Now only support the ap watchdog reset case, afterword extend to the other
 * popular reset exception case.
 *
 * func args:
 * @etime: the exception break out time
 * @addr: the start address of reserved memory to record the exception trace
 * @len: the length of reserved memory to record the exception trace
 * @exception: to store the corrected real exception
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
/*lint -e679*/
int rdr_exception_analysis_ap(u64                         etime,
                              u8                          *addr,
                              u32                         len,
                              struct rdr_exception_info_s *exception)
{
	struct hisiap_ringbuffer_s *q;
	rdr_exception_trace_t      *trace, *min_trace;
	u64                        min_etime = etime;
	u32                        start, end, i;

	q = (struct hisiap_ringbuffer_s *)addr;
	if (unlikely(
		(NULL == q)
		|| is_ringbuffer_invalid(sizeof(rdr_exception_trace_t), len, q)
		|| (NULL == exception)
	)) {
		BB_PRINT_ERR("%s() fail:check_ringbuffer_invalid, exception 0x%pK.\n",
			__func__, exception);
		return -1;
	}

	/* ring buffer is empty, return directly */
	if (unlikely(is_ringbuffer_empty(q))) {
		BB_PRINT_ERR("%s():ring buffer is empty.\n", __func__);
		return -1;
	}

	get_ringbuffer_start_end(q, &start, &end);

	min_trace = NULL;
	for (i = start; i <= end; i++) {
		trace = (rdr_exception_trace_t *)&q->data[(i % q->max_num) * q->field_count];

		if (
			(AP_S_AWDT == trace->e_exce_type)
			|| ((rdr_get_reboot_type() == trace->e_exce_type)
				&& (rdr_get_exec_subtype_value() == trace->e_exce_subtype))
			) {
			continue;
		}

		if ((trace->e_32k_time < min_etime)
			&& (trace->e_32k_time + MIN_EXCEPTION_TIME_GAP >= etime)) {
			/* shall be exception which trigger the whose system reset */
			if (trace->e_reset_core_mask & RDR_AP) {
				min_trace = trace;
				min_etime = trace->e_32k_time;
			}
		}
	}

	if (unlikely(NULL == min_trace)) {
		BB_PRINT_PN("%s(): seach minimum exception trace fail.\n", __func__);
		return -1;
	}

	exception->e_reset_core_mask = min_trace->e_reset_core_mask;
	exception->e_from_core       = min_trace->e_from_core;
	exception->e_exce_type       = min_trace->e_exce_type;
	exception->e_exce_subtype    = min_trace->e_exce_subtype;

	return 0;
}
/*lint +e679*/

static pfn_exception_analysis_ops g_exception_analysis_fn[EXCEPTION_TRACE_ENUM] =
{
	rdr_exception_analysis_ap,
	rdr_exception_analysis_bl31,
};

/*
 * func name: ap_awdt_analysis
 *
 * in the case of reboot reason error reported, we must correct it to the real
 * reboot reason.
 * Now only support the ap watchdog reset case, afterword extend to the other
 * popular reset exception case.
 * The ap watchdog breakout recording in follow ways:
 * AP_S_AWDT,
 * AP_S_PANIC:HI_APPANIC_SOFTLOCKUP,AP_S_PANIC:HI_APPANIC_OHARDLOCKUP,
 * AP_S_PANIC:HI_APPANIC_HARDLOCKUP
 *
 *
 * return value
 * 0 return the correct ap watchdog
 * otherwise incorrect
 *
 */
static int ap_awdt_check(void)
{
	u32 reboot_reason = rdr_get_reboot_type(), subtype;

	if (AP_S_AWDT == reboot_reason) {
		return 0;
	}

	if (AP_S_PANIC == reboot_reason) {
		subtype = rdr_get_exec_subtype_value();

		if ((HI_APPANIC_SOFTLOCKUP == subtype)
			|| (HI_APPANIC_OHARDLOCKUP == subtype)
			|| (HI_APPANIC_HARDLOCKUP == subtype)) {
			return 0;
		}
	}

	return -1;
}

/*
 * func name: ap_awdt_analysis_get_etime
 *
 * get the exception breaks out real time
 *
 * func args:
 * @p_etime: to record the exception breaks out real time
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
static int ap_awdt_analysis_get_etime(u64 *p_etime)
{
	struct rdr_register_module_result info;
	AP_EH_ROOT                        *ap_root;
	u64                               offset, etime;
	u32                               i;

	if (unlikely(rdr_get_areainfo(RDR_AREA_AP, &info))) {
		BB_PRINT_ERR("[%s], rdr_get_areainfo fail!\n", __func__);
		return -1;
	}

	offset = info.log_addr - rdr_reserved_phymem_addr();
	if ( unlikely(
		(offset + info.log_len > rdr_reserved_phymem_size())
		|| (offset + sizeof(AP_EH_ROOT) + PMU_RESET_RECORD_DDR_AREA_SIZE
				> rdr_reserved_phymem_size())
		) ) {
		BB_PRINT_ERR("[%s], offset %llu log_len %u sizeof(AP_EH_ROOT) %u "
			"PMU_RESET_RECORD_DDR_AREA_SIZE %u rdr_reserved_phymem_size %llu!\n",
			__func__, offset, info.log_len, (u32)sizeof(AP_EH_ROOT), 
			(u32)PMU_RESET_RECORD_DDR_AREA_SIZE, rdr_reserved_phymem_size());
		return -1;
	}

	ap_root = (AP_EH_ROOT *)((u8 *)rdr_get_tmppbb() + offset + PMU_RESET_RECORD_DDR_AREA_SIZE);

	etime = 0;
	if (AP_S_AWDT == rdr_get_reboot_type()) {
		for (i = 0; i < WDT_KICK_SLICE_TIMES; i++) {
			if (ap_root->wdt_kick_slice[i] > etime) {
				etime = ap_root->wdt_kick_slice[i];
			}
		}
	} else {
		etime = ap_root->slice;
	}

	if (unlikely(0 == etime)) {
		BB_PRINT_ERR("[%s], etime invalid must not be zero!\n", __func__);
		return -1;
	}

	*p_etime = etime;
	return 0;
}

/*
 * func name: ap_awdt_analysis
 *
 * in the case of reboot reason error reported, we must correct it to the real
 * reboot reason.
 * The way is to traverse each recorded exception trace, select the most early
 * exception.
 * Now only support the ap watchdog reset case, afterword extend to the other
 * popular reset exception case.
 *
 * func args:
 * @exception: to store the corrected real exception
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
int ap_awdt_analysis(struct rdr_exception_info_s *exception)
{
	pfn_exception_analysis_ops        ops_fn;
	u64                               offset, etime;
	u32                               i, num, size[EXCEPTION_TRACE_ENUM];
	u8                                *exception_addr;

	if (unlikely(IS_ERR_OR_NULL(exception))) {
		return -1;
	}

	if (ap_awdt_check()) {
		return -1;
	}

	if (ap_awdt_analysis_get_etime(&etime)) {
		return -1;
	}

	if (get_every_core_exception_info(&num, size)) {
		BB_PRINT_ERR("[%s], bbox_get_every_core_area_info fail!\n", __func__);
		return -1;
	}

	offset = current_info.log_addr - rdr_reserved_phymem_addr();
	if ( unlikely(offset + current_info.log_len > rdr_reserved_phymem_size()) ) {
		BB_PRINT_ERR("[%s], offset %llu log_len %u rdr_reserved_phymem_size %llu!\n",
			__func__, offset, current_info.log_len, rdr_reserved_phymem_size());
		return -1;
	}

	exception_addr = (u8 *)rdr_get_tmppbb() + offset;
	offset = 0;
	for (i = 0; i < EXCEPTION_TRACE_ENUM; i++) {
		if (unlikely(offset + size[i] > current_info.log_len)) {
			BB_PRINT_ERR("[%s], offset %llu overflow! core %u size %u log_len %u\n",
					 __func__, offset, i, size[i], current_info.log_len);
			return -1;
		}

		ops_fn = g_exception_analysis_fn[i];
		if (!ops_fn) {
			offset += size[i];
			continue;
		}

		/* the ap wdt timeout 12s */
		if (ops_fn(etime + 12*TICK_PER_SECOND, exception_addr + offset, (u32)size[i], exception)) {
			BB_PRINT_PN("[%s], pfn_exception_analysis_ops 0x%pK fail! core %u size %u\n",
					 __func__, ops_fn, i, size[i]);
		} else {
			return 0;
		}

		offset += size[i];
	}

	return -1;
}

/*
 * func name: rdr_exception_trace_record
 *
 * when the exception break out, it's necessary to record it
 *
 * func args:
 * @e_reset_core_mask: notify which core need to be reset, when include
 *  the ap core to be reset that will reboot the whole system
 * @e_from_core: exception triggered from which core
 * @e_exce_type: exception type
 * @e_exce_subtype: exception subtype
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
int rdr_exception_trace_record(u64 e_reset_core_mask,
                               u64 e_from_core,
                               u32 e_exce_type,
                               u32 e_exce_subtype)
{
	rdr_exception_trace_t trace;
	unsigned long         lock_flag;

	BB_PRINT_START();

	if (!rdr_init_done()) {
		BB_PRINT_ERR("rdr init faild!\n");
		BB_PRINT_END();
		return -1;
	}

	if (g_arch_timer_func_ptr) {
		trace.e_32k_time = (*g_arch_timer_func_ptr) ();
	} else {
		trace.e_32k_time = jiffies_64;
	}

	trace.e_reset_core_mask = e_reset_core_mask;
	trace.e_from_core = e_from_core;
	trace.e_exce_type = e_exce_type;
	trace.e_exce_subtype = e_exce_subtype;

	spin_lock_irqsave(&rdr_exception_trace_lock, lock_flag);

	hisiap_ringbuffer_write((struct hisiap_ringbuffer_s *)
				(g_exception_trace_addr + g_exception_core[EXCEPTION_TRACE_AP].offset),
				(u8 *)&trace);

	spin_unlock_irqrestore(&rdr_exception_trace_lock, lock_flag);

	BB_PRINT_END();

	return 0;
}

/*
 * func name: get_every_core_exception_info
 *
 * Get the info about the reserved debug memroy area from
 * the dtsi file.
 *
 * func args:
 * @num: the number of reserved debug memory area
 * @size: the size of each reserved debug memory area
 *
 * return value
 * 0 success
 * -1 failed
 *
 */
int get_every_core_exception_info(u32 *num, u32 *size)
{
	struct device_node *np;
	int                ret;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,exceptiontrace");
	if (unlikely(!np)) {
		BB_PRINT_ERR("[%s], find rdr_memory node fail!\n", __func__);
		return -1;
	}

	ret = of_property_read_u32(np, "area_num", num);
	if (unlikely(ret)) {
		BB_PRINT_ERR("[%s], cannot find area_num in dts!\n", __func__);
		return -1;
	}

	BB_PRINT_DBG("[%s], get area_num %u in dts!\n", __func__, *num);

	if (unlikely(*num != EXCEPTION_TRACE_ENUM)) {
		BB_PRINT_ERR("[%s], invaild core num in dts!\n", __func__);
		return -1;
	}

	ret = of_property_read_u32_array(np, "area_sizes", 
		&size[0], (unsigned long)(*num));
	if (unlikely(ret)) {
		BB_PRINT_ERR("[%s], cannot find area_sizes in dts!\n", __func__);
		return -1;
	}

	return 0;
}

/*
 * func name: exception_trace_buffer_init
 *
 * to initialize the ring buffer head of reserved memory for core AP exception trace
 *
 * func args:
 * @addr: the virtual start address of the reserved memory for core AP exception trace
 * @size: the length of the reserved memory for core AP exception trace
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
int exception_trace_buffer_init(u8 *addr, unsigned int size)
{
	u32 min_size = sizeof(struct hisiap_ringbuffer_s) + sizeof(rdr_exception_trace_t);

	if (unlikely(IS_ERR_OR_NULL(addr))) {
		return -1;
	}

	if (unlikely(size < min_size)) {
		return -1;
	}

	return hisiap_ringbuffer_init((struct hisiap_ringbuffer_s *)(addr), size,
				      sizeof(rdr_exception_trace_t), NULL);
}

/*
 * func name: rdr_exception_trace_ap_init
 *
 * to initialize the reserved memory of core AP exception trace
 *
 * func args:
 * @phy_addr: the physical start address of the reserved memory for core AP exception trace
 * @virt_addr: the virtual start address of the reserved memory for core AP exception trace
 * @log_len: the length of the reserved memory for core AP exception trace
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
static int rdr_exception_trace_ap_init(u8 *phy_addr, u8 *virt_addr, u32 log_len)
{
	memset_s(virt_addr, log_len, 0, log_len);
	
	if ( unlikely(exception_trace_buffer_init(virt_addr, log_len)) ) {
		return -1;
	}

	return 0;
}

static pfn_exception_init_ops g_exception_init_fn[EXCEPTION_TRACE_ENUM] =
{
	rdr_exception_trace_ap_init,
	rdr_exception_trace_bl31_init,
};

/*
 * func name: rdr_exception_trace_init
 *
 * The initialize of exception trace module
 *
 *
 * return value
 * 0 success
 * -1 failed
 *
 */
int rdr_exception_trace_init(void)
{
	pfn_exception_init_ops ops_fn;
	static bool            init;
	u32                    i, num, size[EXCEPTION_TRACE_ENUM], offset;

	if (init) {
		return 0;
	}

	BB_PRINT_START();

	if (unlikely(rdr_get_areainfo(RDR_AREA_EXCEPTION_TRACE, &current_info))) {
		BB_PRINT_ERR("[%s], rdr_get_areainfo fail!\n", __func__);
		goto error;
	}

	if (NULL == g_exception_trace_addr) {
		g_exception_trace_addr = (u8 *)hisi_bbox_map(current_info.log_addr, current_info.log_len);
		if (unlikely(!g_exception_trace_addr)) {
			BB_PRINT_ERR("[%s], hisi_bbox_map fail!\n", __func__);
			goto error;
		}
	}

	if (unlikely(get_every_core_exception_info(&num, size))) {
		BB_PRINT_ERR("[%s], bbox_get_every_core_area_info fail!\n", __func__);
		goto error;
	}

	offset = 0;
	for (i = 0; i < EXCEPTION_TRACE_ENUM; i++) {
		g_exception_core[i].offset = offset;
		g_exception_core[i].size = size[i];

		BB_PRINT_PN("[%s]core %u offset %u size %u addr 0x%pK\n",
			__func__, i, offset, size[i], g_exception_trace_addr + offset);

		offset += size[i];

		if (unlikely(offset > current_info.log_len)) {
			BB_PRINT_ERR("[%s], offset %u overflow! core %u size %u log_len %u\n",
					 __func__, offset, i, size[i], current_info.log_len);
			goto error;
		}

		ops_fn = g_exception_init_fn[i];
		if ( unlikely(ops_fn 
			&& ops_fn((u8 *)current_info.log_addr + g_exception_core[i].offset,
				g_exception_trace_addr + g_exception_core[i].offset, size[i])) ) {
			BB_PRINT_ERR("[%s], exception init fail: core %u size %u ops_fn 0x%pK\n",
					 __func__, i, size[i], ops_fn);
			goto error;
		}
	}

	init = true;
	BB_PRINT_END();
	return 0;

error:
	BB_PRINT_END();
	return -1;
}

