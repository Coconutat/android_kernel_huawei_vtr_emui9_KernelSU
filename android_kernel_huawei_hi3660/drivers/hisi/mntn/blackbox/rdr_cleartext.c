/*
 * blackbox cleartext. (kernel run data recorder clear text recording.)
 *
 * Copyright (c) 2013 Huawei Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/rtc.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/of.h>
#include <linux/ctype.h>
#include <linux/hisi/hisi_bootup_keypoint.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/hisi/util.h>
#include <hisi_partition.h>
#include <linux/hisi/kirin_partition.h>
#include <linux/mfd/hisi_pmic.h>
#include <linux/hisi/rdr_hisi_platform.h>
#include <libhwsecurec/securec.h>
#include <linux/hisi/rdr_hisi_ap_ringbuffer.h>
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG HISI_BLACKBOX_TAG

#include "../bl31/hisi_bl31_exception.h"
#include "rdr_inner.h"
#include "rdr_print.h"
#include "rdr_field.h"

struct semaphore rdr_cleartext_sem;
static LIST_HEAD(rdr_cleartext_ops_list);
static DEFINE_SPINLOCK(rdr_cleartext_ops_list_lock);

static u64 g_coreid_from_area_id[RDR_AREA_MAXIMUM] =
{
    RDR_AP,
    RDR_CP,
    RDR_TEEOS,
    RDR_HIFI,
    RDR_LPM3,
    RDR_IOM3,
    RDR_ISP,
    RDR_IVP,
    RDR_EMMC,
    RDR_MODEMAP,
    RDR_CLK,
    RDR_REGULATOR,
    RDR_BFM,
    RDR_HISEE,
    RDR_NPU,
    RDR_CONN,
	RDR_EXCEPTION_TRACE
};

static int rdr_exception_trace_ap_cleartext_print(char *dir_path, u64 log_addr, u32 log_len);
static pfn_cleartext_ops g_exception_cleartext_fn[EXCEPTION_TRACE_ENUM] =
{
	rdr_exception_trace_ap_cleartext_print,
	rdr_exception_trace_bl31_cleartext_print,
};

/*
 * func name: rdr_cleartext_print
 *
 * append(save) data to path.
 * func args:
 *  struct file *fp: the pointer of file which to save the clear text.
 *  bool *error: to fast the cpu process when there is error happened 
 *              before nowadays print, please refer the function 
 *              bbox_head_cleartext_print to get the use of this parameter.
 *
 * return
 */
void rdr_cleartext_print(struct file *fp, bool *error, const char *format, ...)
{
	va_list arglist;
	size_t  len;
	char    buffer[PATH_MAXLEN];
	int     ret;

	if ( unlikely(IS_ERR_OR_NULL(fp) || IS_ERR_OR_NULL(error)) ) {
		return;
	}

	/* stoping the next printing for the previous error printing */
	if (unlikely(true == *error)) {
		return;
	}

	/* get the string buffer for the next printing */
	va_start(arglist, format);
	ret = vsnprintf_s(buffer, PATH_MAXLEN, PATH_MAXLEN - 1, format, arglist);
	va_end(arglist);

	if (unlikely(ret <= 0)) {
		*error = true;
		BB_PRINT_ERR("%s():vsnprintf_s error ret %d.\n", __func__, ret);
		return;
	}

	/* print the string buffer to the specified file descriptor */
	len = (size_t)ret;/*lint !e571*/
	ret = vfs_write(fp, buffer, len, &(fp->f_pos));/*lint !e613 */
	if (unlikely(ret != len)) {
		BB_PRINT_ERR("%s():write file exception with ret %d.\n",
			     __func__, ret);
		*error = true;
		return;
	}

	return;
}

/*
 * func name: bbox_cleartext_get_filep
 *
 * Get the file descriptor pointer whose abosolute path composed 
 * by the dir_path&file_name and initialize it.
 *
 * func args:
 *  char *dir_path: the directory path about the specified file.
 *  char *file_name:the name of the specified file.
 *
 * return
 * file descriptor pointer when success, otherwise NULL.
 *
 * attention
 * the function bbox_cleartext_get_filep shall be used
 * in paired with function bbox_cleartext_end_filep.
 */
struct file *bbox_cleartext_get_filep(char *dir_path, char *file_name)
{
	struct file *fp;
	char        path[PATH_MAXLEN];
	int         flags, ret;

	if ( unlikely(IS_ERR_OR_NULL(dir_path) || IS_ERR_OR_NULL(file_name)) ) {
		BB_PRINT_ERR("[%s], invalid file path dir_path %pK file_name %pK!\n",
			__func__, dir_path, file_name);
		return NULL;
	}

	/* get the absolute file path string */
	ret = snprintf_s(path, PATH_MAXLEN, PATH_MAXLEN - 1, "%s/%s", dir_path, file_name);
	if (unlikely(ret < 0)) {
		BB_PRINT_ERR("[%s], snprintf_s ret %d!\n", __func__, ret);
		return NULL;
	}

	/* the file need to be trucated */
	flags = O_CREAT | O_RDWR | O_TRUNC;
	fp = filp_open(path, flags, FILE_LIMIT);
	if (unlikely(IS_ERR_OR_NULL(fp))) {
		BB_PRINT_ERR("%s():create file %s err. fp=0x%pK\n", __func__, path, fp);
		return NULL;
	}

	vfs_llseek(fp, 0L, SEEK_END);
	return fp;
}

/*
 * func name: bbox_cleartext_end_filep
 *
 * cleaning of the specified file
 *
 * func args:
 *  struct file *fp: the file descriptor pointer .
 *  char *dir_path: the directory path about the specified file.
 *  char *file_name:the name of the specified file.
 *
 * return
 *
 * attention
 * the function bbox_cleartext_end_filep shall be used
 * in paired with function bbox_cleartext_get_filep.
 */
void bbox_cleartext_end_filep(struct file *fp, char *dir_path, char *file_name)
{
	char path[PATH_MAXLEN];
	int  ret;

	if ( unlikely(IS_ERR_OR_NULL(fp) 
		|| IS_ERR_OR_NULL(dir_path) 
		|| IS_ERR_OR_NULL(file_name)) ) {
		BB_PRINT_ERR("[%s], invalid file path dir_path %pK file_name %pK fp %pK!\n",
			__func__, dir_path, file_name, fp);
		return;
	}

	/* get the absolute file path string */
	ret = snprintf_s(path, PATH_MAXLEN, PATH_MAXLEN - 1, "%s/%s", dir_path, file_name);
	if (unlikely(ret < 0)) {
		BB_PRINT_ERR("[%s], snprintf_s ret %d!\n", __func__, ret);
		return;
	}

	/* flush synchronize the specified file */
	vfs_fsync(fp, 0);

	/* close the specified file descriptor */
	filp_close(fp, NULL);/*lint !e668 */

	/* 根据权限要求，hisi_logs目录及子目录群组调整为root-system */
	ret = (int)bbox_chown((const char __user *)path, ROOT_UID, SYSTEM_GID, false);
	if (unlikely(ret)) {
		BB_PRINT_ERR("[%s], chown %s uid [%d] gid [%d] failed err [%d]!\n",
		     __func__, path, ROOT_UID, SYSTEM_GID, ret);
	}
}

/*
 * func name: bbox_head_cleartext_print
 *
 * The clear text printing for the common header of reserved debug memory
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @log_addr: the start address of reserved memory for specified core
 * @log_len: the length of reserved memory for specified core
 *
 * return value
 *
 */
static int bbox_head_cleartext_print(char *dir_path, u64 log_addr, u32 log_len)
{
	struct rdr_struct_s *p = (struct rdr_struct_s *)log_addr;
	struct file         *fp;
	bool                error;

	if (unlikely(IS_ERR_OR_NULL(dir_path) || IS_ERR_OR_NULL(p))) {
		BB_PRINT_ERR("%s() error:dir_path 0x%pK log_addr 0x%pK.\n", __func__, dir_path, p);
		return -1;
	}

	if (unlikely(log_len < sizeof(struct rdr_struct_s))) {
		BB_PRINT_ERR("%s() error:log_len %u sizeof(struct rdr_struct_s) %lu.\n",
			__func__, log_len, sizeof(struct rdr_struct_s));
		return -1;
	}

	/* get the file descriptor from the specified directory path */
	fp = bbox_cleartext_get_filep(dir_path, "BBOX_HEAD_INFO.txt");
	if (unlikely(IS_ERR_OR_NULL(fp))) {
		BB_PRINT_ERR("%s() error:fp 0x%pK.\n", __func__, fp);
		return -1;
	}

	error = false;

	p->base_info.datetime[DATATIME_MAXLEN - 1] = '\0';
	p->base_info.e_module[MODULE_NAME_LEN - 1] = '\0';
	p->base_info.e_desc[STR_EXCEPTIONDESC_MAXLEN - 1] = '\0';
	p->top_head.build_time[RDR_BUILD_DATE_TIME_LEN - 1] = '\0';

	rdr_cleartext_print(fp, &error, "========= print top head start =========\n");
	rdr_cleartext_print(fp, &error, "magic        :[0x%x]\n", p->top_head.magic);
	rdr_cleartext_print(fp, &error, "version      :[0x%x]\n", p->top_head.version);
	rdr_cleartext_print(fp, &error, "area num     :[0x%x]\n", p->top_head.area_number);
	rdr_cleartext_print(fp, &error, "reserve      :[0x%x]\n", p->top_head.reserve);
	rdr_cleartext_print(fp, &error, "buildtime    :[%s]\n",   p->top_head.build_time);
	rdr_cleartext_print(fp, &error, "========= print top head e n d =========\n");

	rdr_cleartext_print(fp, &error, "========= print baseinfo start =========\n");
	rdr_cleartext_print(fp, &error, "modid        :[0x%x]\n", p->base_info.modid);
	rdr_cleartext_print(fp, &error, "arg1         :[0x%x]\n", p->base_info.arg1);
	rdr_cleartext_print(fp, &error, "arg2         :[0x%x]\n", p->base_info.arg2);
	rdr_cleartext_print(fp, &error, "coreid       :[0x%x]\n", p->base_info.e_core);
	rdr_cleartext_print(fp, &error, "reason       :[0x%x]\n", p->base_info.e_type);
	rdr_cleartext_print(fp, &error, "subtype      :[0x%x]\n", p->base_info.e_subtype);
	rdr_cleartext_print(fp, &error, "e data       :[%s]\n",   p->base_info.datetime);
	rdr_cleartext_print(fp, &error, "e module     :[%s]\n",   p->base_info.e_module);
	rdr_cleartext_print(fp, &error, "e desc       :[%s]\n",   p->base_info.e_desc);
	rdr_cleartext_print(fp, &error, "e start_flag :[0x%x]\n", p->base_info.start_flag);
	rdr_cleartext_print(fp, &error, "e save_flag  :[0x%x]\n", p->base_info.savefile_flag);
	rdr_cleartext_print(fp, &error, "e reboot_flag:[0x%x]\n", p->base_info.reboot_flag);
	rdr_cleartext_print(fp, &error, "savefile_flag:[0x%x]\n", p->cleartext_info.savefile_flag);
	rdr_cleartext_print(fp, &error, "========= print baseinfo e n d =========\n");

	/* the cleaning of specified file descriptor */
	bbox_cleartext_end_filep(fp, dir_path, "BBOX_HEAD_INFO.txt");

	if (unlikely(true == error)) {
		return -1;
	}

	return 0;
}

/*
 * func name: _incorrect_reboot_reason_analysis
 *
 * in the case of reboot reason error reported, we must correct it to the real
 * real reboot reason.
 *
 * func args:
 * @fp: the file descriptor of saved clear text
 * @error: to fast the cpu process when there is error happened
 *         before nowadays print, please refer the function
 *         bbox_head_cleartext_print to get the use of this parameter.
 * @e_from_core: the reboot exception comes from which core
 * @e_exce_type: the reboot exception type
 * @e_exce_subtype: the reboot exception subtype
 *
 * return value
 *
 */
static void _incorrect_reboot_reason_analysis(struct file *fp,
	                                          bool        *error, 
	                                          u64         e_from_core,
	                                          u32         e_exce_type,
	                                          u32         e_exce_subtype)
{
	char *subtype_name;

	subtype_name = rdr_get_subtype_name(e_exce_type, e_exce_subtype);
	if (subtype_name) {
		rdr_cleartext_print(fp, error,
			 "system exception core [%s], reason [%s:%s], category [%s]\n",
			rdr_get_exception_core(e_from_core),
			rdr_get_exception_type(e_exce_type),
			subtype_name,
			rdr_get_category_name(e_exce_type, e_exce_subtype));
	}
	else {
		rdr_cleartext_print(fp, error,
			 "system exception core [%s], reason [%s], category [%s]\n",
			rdr_get_exception_core(e_from_core),
			rdr_get_exception_type(e_exce_type),
			rdr_get_category_name(e_exce_type, e_exce_subtype));
	}
}

/*
 * func name: incorrect_reboot_reason_analysis
 *
 * in the case of reboot reason error reported, we must correct it to the real
 * real reboot reason.
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @exception: the corrected real reboot exception
 *
 * return value
 *
 */
void incorrect_reboot_reason_analysis(char *dir_path, struct rdr_exception_info_s *exception)
{
	struct file *fp;
	bool        error;

	if ( unlikely(IS_ERR_OR_NULL(dir_path) || IS_ERR_OR_NULL(exception)) ) {
		BB_PRINT_ERR("%s() error:dir_path 0x%pK exception 0x%pK.\n",
			__func__, dir_path, exception);
		return;
	}

	/* get the file descriptor from the specified directory path */
	fp = bbox_cleartext_get_filep(dir_path, "incorrect_reboot_reason.txt");
	if (unlikely(IS_ERR_OR_NULL(fp))) {
		BB_PRINT_ERR(KERN_ERR "%s() error:fp 0x%pK.\n", __func__, fp);
		return;
	}

	error = false;

	rdr_cleartext_print(fp, &error, "<incorrect reboot reason>\n");
	_incorrect_reboot_reason_analysis(fp, &error, RDR_AP, rdr_get_reboot_type(),
		rdr_get_exec_subtype_value());

	rdr_cleartext_print(fp, &error, "<correct reboot reason>\n");
	_incorrect_reboot_reason_analysis(fp, &error, exception->e_from_core, 
		exception->e_exce_type, exception->e_exce_subtype);

	/* the cleaning of specified file descriptor */
	bbox_cleartext_end_filep(fp, dir_path, "incorrect_reboot_reason.txt");
	return;
}

/*
 * func name: rdr_exception_trace_ap_cleartext_print
 *
 * The clear text printing for the reserved debug memory of ap exceptiontrace
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @log_addr: the start address of reserved memory for specified core
 * @log_len: the length of reserved memory for specified core
 *
 * return value
 * 0 success
 * -1 failed
 *
 */
/*lint -e679*/
static int rdr_exception_trace_ap_cleartext_print(char *dir_path, u64 log_addr, u32 log_len)
{
	struct hisiap_ringbuffer_s *q;
	rdr_exception_trace_t      *trace;
	struct file                *fp;
	bool                       error;
	u32                        start, end, i;

	q = (struct hisiap_ringbuffer_s *)log_addr;
	if (unlikely(is_ringbuffer_invalid(sizeof(rdr_exception_trace_t), log_len, q))) {
		BB_PRINT_ERR("%s() fail:check_ringbuffer_invalid.\n", __func__);
		return -1;
	}

	/* ring buffer is empty, return directly */
	if (is_ringbuffer_empty(q)) {
		BB_PRINT_DBG("%s():ring buffer is empty.\n", __func__);
		return 0;
	}

	/* get the file descriptor from the specified directory path */
	fp = bbox_cleartext_get_filep(dir_path, "exception_trace_ap.txt");
	if (unlikely(IS_ERR_OR_NULL(fp))) {
		BB_PRINT_ERR("%s() error:fp 0x%pK.\n", __func__, fp);
		return -1;
	}

	error = false;

	rdr_cleartext_print(fp, &error, "slice          reset_core_mask   from_core      "
		"exception_type           exception_subtype        \n");

	get_ringbuffer_start_end(q, &start, &end);
	for (i = start; i <= end; i++) {
		trace = (rdr_exception_trace_t *)&q->data[(i % q->max_num) * q->field_count];
		rdr_cleartext_print(fp, &error, "%-15llu0x%-16llx%-15s%-25s%-25s\n",
			trace->e_32k_time, trace->e_reset_core_mask, rdr_get_exception_core(trace->e_from_core), 
			rdr_get_exception_type(trace->e_exce_type),
			rdr_get_subtype_name(trace->e_exce_type, trace->e_exce_subtype)
		);
	}

	/* the cleaning of specified file descriptor */
	bbox_cleartext_end_filep(fp, dir_path, "exception_trace_ap.txt");

	if (unlikely(true == error)) {
		return -1;
	}
	return 0;
}
/*lint +e679*/

/*
 * func name: rdr_hisi_exception_trace_cleartext_print
 *
 * The clear text printing for the reserved debug memory of core exceptiontrace
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @log_addr: the start address of reserved memory for specified core
 * @log_len: the length of reserved memory for specified core
 *
 * return value
 * 0 success
 * -1 failed
 *
 */
int rdr_exception_trace_cleartext_print(char *dir_path, u64 log_addr, u32 log_len)
{
	pfn_cleartext_ops ops_fn;
	u32               i, num, size[EXCEPTION_TRACE_ENUM], offset;

	if ( unlikely(IS_ERR_OR_NULL(dir_path) || IS_ERR_OR_NULL((void *)log_addr)) ) {
		BB_PRINT_ERR("%s() error:dir_path 0x%pK log_addr 0x%pK.\n",
			__func__, dir_path, (void *)log_addr);
		return -1;
	}

	if (get_every_core_exception_info(&num, size)) {
		BB_PRINT_ERR("[%s], bbox_get_every_core_area_info fail!\n", __func__);
		return -1;
	}

	offset = 0;
	for (i = 0; i < EXCEPTION_TRACE_ENUM; i++) {
		ops_fn = g_exception_cleartext_fn[i];

		if (unlikely(offset + size[i] > log_len)) {
			BB_PRINT_ERR("[%s], offset %u overflow! core %u size %u log_len %u\n",
					 __func__, offset, i, size[i], log_len);
			return -1;
		}

		if ( unlikely(ops_fn && ops_fn(dir_path, log_addr + offset, size[i])) ) {
			BB_PRINT_ERR("[%s], pfn_cleartext_ops 0x%pK fail! core %u size %u\n",
					 __func__, ops_fn, i, size[i]);
			return -1;
		}

		offset += size[i];
	}

	return 0;
}

/*
 * func name: rdr_get_coreid_from_areaid
 *
 * The core id mapping from the memory area id
 *
 * func args:
 * @area_id: the specified core area id
 *
 * return value
 * -1 failure
 * otherwise success
 *
 */
static inline int64_t rdr_get_coreid_from_areaid(u32 area_id)
{
	if (unlikely(area_id >= RDR_AREA_MAXIMUM)) {
		return -1;
	} else {
		return (int64_t)g_coreid_from_area_id[area_id];
	}
}

/*
 * func name: rdr_get_cleartext_fn
 *
 * Get the registered clear text callback function 
 * from the memory area id.
 *
 * func args:
 * @area_id: the specified core area id
 *
 * return value
 * NULL failure
 * otherwise success
 *
 */
static pfn_cleartext_ops rdr_get_cleartext_fn(u32 area_id)
{
	struct rdr_cleartext_ops_s *p_cleartext_ops = NULL;
	pfn_cleartext_ops          ops;
	struct list_head           *cur = NULL;
	unsigned long              lock_flag;
	int64_t                    ret;
	u64                        coreid;

	ret = rdr_get_coreid_from_areaid(area_id);
	if (unlikely(ret < 0))
		return NULL;
	coreid = (u64)ret;/*lint !e571*/

	spin_lock_irqsave(&rdr_cleartext_ops_list_lock, lock_flag);
	list_for_each(cur, &rdr_cleartext_ops_list) {
		p_cleartext_ops = list_entry(cur, struct rdr_cleartext_ops_s, s_list);

		if (coreid == p_cleartext_ops->s_core_id) {
			ops = p_cleartext_ops->ops_cleartext;
			spin_unlock_irqrestore(&rdr_cleartext_ops_list_lock, lock_flag);
			return (ops);
		}
	}
	spin_unlock_irqrestore(&rdr_cleartext_ops_list_lock, lock_flag);
	return NULL;
}

/*
 * func name: bbox_save_cleartext
 *
 * Get the start address of reserved memory and length for specified core first,
 * then transfer them to the correponding clear text callback function.
 * The registered clear text callback function is respobsible of the clear text
 * file output.
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @log_addr: the start address of reserved memory for specified core
 * @log_len: the length of reserved memory for specified core
 * @area_id: the specified core area id
 *
 * return value
 *
 */
static void bbox_save_cleartext(char *dir_path,
                                u64  log_addr,
                                u32  log_len,
                                u32  area_id,
                                u32  is_bbox_head)
{
	pfn_cleartext_ops ops;
	int               ret;

	BB_PRINT_START();

	/* for the common head, it introduces special process */
	if (true == is_bbox_head) {
		ops = bbox_head_cleartext_print;
	} else {
		ops = rdr_get_cleartext_fn(area_id);
		if (IS_ERR_OR_NULL(ops)) {
			return;
		}
	}

	ret = ops(dir_path, log_addr, log_len);
	if (unlikely(ret < 0)) {
		BB_PRINT_ERR("[%s], call pfn_cleartext_ops 0x%pK failed err [%d]!\n",
		     __func__, ops, ret);
	}

	BB_PRINT_END();
}

/*
 * func name: bbox_get_every_core_area_info
 *
 * Get the info about the reserved debug memroy area from
 * the dtsi file.
 *
 * func args:
 * @value: the number of reserved debug memory area
 * @data: the size of each reserved debug memory area
 *
 * return value
 * 0 success
 * -1 failed
 *
 */
int bbox_get_every_core_area_info(u32 *value, u32 *data, struct device_node **npp)
{
	struct device_node *np;
	int                ret;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,rdr");
	if (unlikely(!np)) {
		BB_PRINT_ERR("[%s], find rdr_memory node fail!\n", __func__);
		return -1;
	}

	ret = of_property_read_u32(np, "rdr_area_num", value);
	if (unlikely(ret)) {
		BB_PRINT_ERR("[%s], cannot find rdr_area_num in dts!\n",
			     __func__);
		return -1;
	}

	BB_PRINT_DBG("[%s], get rdr_area_num [0x%x] in dts!\n", __func__,
		     *value);

	if (unlikely(*value != RDR_CORE_MAX)) {
		BB_PRINT_ERR("[%s], invaild core num in dts!\n", __func__);
		return -1;
	}

	ret = of_property_read_u32_array(np, "rdr_area_sizes", 
		&data[0], (unsigned long)(*value));
	if (unlikely(ret)) {
		BB_PRINT_ERR("[%s], cannot find rdr_area_sizes in dts!\n",
			     __func__);
		return -1;
	}

	*npp = np;
	return 0;
}

/*
 * func name: bbox_cleartext_proc
 *
 * Get the start address of reserved memory and length for each core first,
 * then transfer them to each registered clear text callback function who
 * come from surrounding core.
 * The registered clear text callback function is respobsible of the clear text
 * file output.
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @base_addr:the start address of reserved memory to save the debug info for each core
 *
 * return value
 *
 */
void bbox_cleartext_proc(const char *path, const char *base_addr)
{
	struct device_node *np;
	char               *addr;
	char               dir_path[PATH_MAXLEN];
	int                ret;
	u32                value, size, i;
	u32                data[RDR_CORE_MAX];

	if (unlikely(bbox_get_every_core_area_info(&value, data, &np))) {
		BB_PRINT_ERR("[%s], bbox_get_every_core_area_info fail!\n",
			     __func__);
		return;
	}

	ret = snprintf_s(dir_path, PATH_MAXLEN, PATH_MAXLEN - 1, "%s/%s", path, BBOX_SPLIT_BIN);
	if (unlikely(ret < 0)) {
		BB_PRINT_ERR("[%s], snprintf_s error!\n", __func__);
		return;
	}

	addr = (char *)base_addr + rdr_get_pbb_size();
	size = 0;
	for (i = value - 1; i > 0; i--) {
		addr -= data[i];
		size += data[i];

		if (data[i] > 0) {
			bbox_save_cleartext(dir_path, (u64)addr, data[i], i, false);
		}
	}

	bbox_save_cleartext(dir_path, (u64)base_addr, RDR_BASEINFO_SIZE, 0, true);

	/* save AP data info */
	addr = (char *)base_addr + RDR_BASEINFO_SIZE;
	size = (u32)rdr_get_pbb_size() - (size + RDR_BASEINFO_SIZE);
	bbox_save_cleartext(dir_path, (u64)addr, size, 0, false);

	if (!in_atomic() && !irqs_disabled()
		&& !in_irq()) {
		/* synchronize the file system */
		sys_sync();
	}

	return;
}

/*
 * func name: rdr_cleartext_body
 *
 * When exception occur, save the dump log first then trigger the clear text saving
 * thread who is responsible of the saving operation.
 * func args:
 * @arg: not used in this function
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
int rdr_cleartext_body(void *arg)
{
	char path[PATH_MAXLEN];
	char *date;
	int  ret = 0;

	/*
	 * in the case of multiple continuos rdr_syserr_process 
	 * when the previous rdr_cleartext_body is still in running, 
	 * we don't care this case.
	 */
	while (1) {
		down(&rdr_cleartext_sem);

		BB_PRINT_PN("rdr_cleartext_body enter\n");


		date = rdr_field_get_datetime();
		memset_s(path, PATH_MAXLEN, 0, PATH_MAXLEN);
		ret = snprintf_s(path, PATH_MAXLEN, PATH_MAXLEN - 1, "%s%s/", PATH_ROOT, date);
		if(unlikely(ret < 0)){
			BB_PRINT_ERR("[%s], snprintf_s ret %d!\n", __func__, ret);
		}
		bbox_cleartext_proc(path, (char *)rdr_get_pbb());

		/* save the cleartext dumplog over flag to avoid 
		the same saving during the next reboot procedure */
		rdr_cleartext_dumplog_done();
		BB_PRINT_PN("rdr_cleartext_dumplog_done\n");
	}

	return 0; /*lint !e527*/
}

/*
 * func name: rdr_check_date
 *
 * Check the date string if it's valid. 
 *
 * return value
 * return 0 for successful checking, otherwise failed.
 *
 */
static int rdr_check_date(char *date, u32 len)
{
	u32 i;

	for (i = 0; i < len; i++) {
		if ('\0' == date[i]) {
			if (i > 0) {
				return 0;
			} else {
				return -1;
			}
		}
	}

	return -1;
}

/*
 * func name: rdr_exceptionboot_save_cleartext
 *
 * When exception occur, save the dump log first then trigger the cleartext saving.
 * If the cleartext saving isn't finised,but the reset opertation has been triggered,
 * so it's necessary to delay the cleartext saving to the next reboot procedure,
 * this function is in responsible of the delayed cleartext saving.
 *
 * return value
 *
 */
void rdr_exceptionboot_save_cleartext(void)
{
	struct rdr_struct_s *tmpbb;
	char path[PATH_MAXLEN];
	char *date;
	int ret = 0;

	tmpbb = rdr_get_tmppbb();
	date = (char *)(tmpbb->base_info.datetime);

	if (unlikely(rdr_check_date(date, DATATIME_MAXLEN))) {
		BB_PRINT_ERR("%s():rdr_check_date invalid\n", __func__);
		return;
	}

	memset_s(path, PATH_MAXLEN, 0, PATH_MAXLEN);
	ret = snprintf_s(path, PATH_MAXLEN, PATH_MAXLEN - 1,
		"%s%s/", PATH_ROOT, date);
	if(unlikely(ret < 0)){
		BB_PRINT_ERR("[%s], snprintf_s ret %d!\n", __func__, ret);
	}

	bbox_cleartext_proc(path, (char *)tmpbb);
}

static void __rdr_register_cleartext_ops(struct rdr_cleartext_ops_s *ops)
{
	struct rdr_cleartext_ops_s *p_info;
	struct list_head          *cur, *next;

	BB_PRINT_START();

	list_for_each_safe(cur, next, &rdr_cleartext_ops_list) {
		p_info = list_entry(cur, struct rdr_cleartext_ops_s, s_list);
		if (ops->s_core_id < p_info->s_core_id) {
			list_add_tail(&(ops->s_list), cur);
			goto out;
		}
	}
	list_add_tail(&(ops->s_list), &rdr_cleartext_ops_list);

out:
	BB_PRINT_DBG("__rdr_register_cleartext_ops coreid is [0x%llx]\n", ops->s_core_id);
	BB_PRINT_END();
}

/*
 * func name: rdr_register_cleartext_ops
 * func args:
 *   u64 core_id: the same with the parameter coreid of function rdr_register_module_ops
 *   pfn_cleartext_ops ops_fn: the function to write the content 
 *       of reserved memory in clear text format
 *
 * return value
 *	< 0 error
 *	0 success
 */
int rdr_register_cleartext_ops(u64 coreid, pfn_cleartext_ops ops_fn)
{
	/*lint -e429*/
	struct rdr_cleartext_ops_s *p_cleartext_ops;
	struct list_head           *cur;
	unsigned long              lock_flag;
	int                        ret = -1;

	BB_PRINT_START();

	if ( unlikely(IS_ERR_OR_NULL(ops_fn)) ) {
		BB_PRINT_ERR("invalid para ops_fn!\n");
		BB_PRINT_END();
		return ret;
	}

	if (unlikely(!rdr_init_done())) {
		BB_PRINT_ERR("rdr init faild!\n");
		BB_PRINT_END();
		return ret;
	}

	spin_lock_irqsave(&rdr_cleartext_ops_list_lock, lock_flag);
	list_for_each(cur, &rdr_cleartext_ops_list) {
		p_cleartext_ops = list_entry(cur, struct rdr_cleartext_ops_s, s_list);

		if (coreid == p_cleartext_ops->s_core_id) {
			spin_unlock_irqrestore(&rdr_cleartext_ops_list_lock, lock_flag);
			BB_PRINT_ERR("core_id exist already\n");
			BB_PRINT_END();
			return ret;
		}
	}

	p_cleartext_ops = kzalloc(sizeof(struct rdr_cleartext_ops_s), GFP_ATOMIC);
	if ( unlikely(IS_ERR_OR_NULL(p_cleartext_ops)) ) {
		spin_unlock_irqrestore(&rdr_cleartext_ops_list_lock, lock_flag);
		BB_PRINT_ERR("kzalloc error\n");
		BB_PRINT_END();
		return ret;
	}

	INIT_LIST_HEAD(&(p_cleartext_ops->s_list));/*lint !e613*/
	p_cleartext_ops->s_core_id = coreid;/*lint !e613*/
	p_cleartext_ops->ops_cleartext = ops_fn;/*lint !e613*/

	__rdr_register_cleartext_ops(p_cleartext_ops);

	spin_unlock_irqrestore(&rdr_cleartext_ops_list_lock, lock_flag);

	BB_PRINT_DBG("rdr_register_cleartext_ops success.\n");
	BB_PRINT_END();
	return 0;
	/*lint +e429*/
}

/*
 * func name: rdr_cleartext_print_ops
 *
 * print registered clear text rollback function.
 * func args:
 *
 * return
 */
void rdr_cleartext_print_ops(void)
{
	struct rdr_cleartext_ops_s *p_cleartext_ops = NULL;
	struct list_head           *cur = NULL;
	struct list_head           *next = NULL;
	int                        index = 1;

	BB_PRINT_START();
	index = 1;
	spin_lock(&rdr_cleartext_ops_list_lock);
	list_for_each_safe(cur, next, &rdr_cleartext_ops_list) {
		p_cleartext_ops = list_entry(cur, struct rdr_cleartext_ops_s, s_list);

		BB_PRINT_DBG("==========[%.2d]-start==========\n", index);
		BB_PRINT_DBG(" cleartext-fn:   [0x%pK]\n",
			     p_cleartext_ops->ops_cleartext);
		BB_PRINT_DBG("==========[%.2d]-e n d==========\n", index);
		index++;
	}
	spin_unlock(&rdr_cleartext_ops_list_lock);

	BB_PRINT_END();
	return;
}
