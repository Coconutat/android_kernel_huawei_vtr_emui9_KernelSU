/*
 * record the data to rdr. (RDR: kernel run data recorder.)
 * This file wraps the ring buffer.
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/hisi/rdr_hisi_ap_ringbuffer.h>
#include <libhwsecurec/securec.h>
#include "../rdr_print.h"
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG HISI_BLACKBOX_TAG

int hisiap_ringbuffer_init(struct hisiap_ringbuffer_s *q, u32 bytes,
			   u32 fieldcnt, const char *keys)
{
	if (NULL == q) {
		BB_PRINT_ERR("[%s], buffer head is null!\n", __func__);
		return -EINVAL;
	}

	if (bytes <
	    (sizeof(struct hisiap_ringbuffer_s) + sizeof(u8) * fieldcnt)) {
		BB_PRINT_ERR("[%s], ringbuffer size [0x%x] is too short!\n",
		       __func__, bytes);
		return -EINVAL;
	}

	/* max_num: records count. */
	q->max_num = (bytes - sizeof(struct hisiap_ringbuffer_s)) /
	    (sizeof(u8) * fieldcnt);
	atomic_set((atomic_t *)&(q->rear), 0);/*lint !e1058*//* point to the last NULL record. UNIT is record. */
	q->r_idx = 0;		/* point to the last read record. */
	q->count = 0;
	q->is_full = 0;
	q->field_count = fieldcnt;	/* How many u8 in ONE record. */

	memset(q->keys, 0, HISIAP_KEYS_MAX + 1);
	if (keys)
		strncpy(q->keys, keys, HISIAP_KEYS_MAX);/*lint !e747 */
	return 0;
}

/*lint -e818 */
void hisiap_ringbuffer_write(struct hisiap_ringbuffer_s *q, u8 *element)
{
	atomic_add(1, (atomic_t *)&(q->rear));
	if (q->rear >= q->max_num) {
		q->rear = 1;
		q->is_full = 1;
	}

/*lint -e737 -e679*/
	memcpy((void *)&q->data[(q->rear - 1) * q->field_count],
	       (void *)element, q->field_count * sizeof(u8));
/*lint +e737 +e679*/

	q->count ++;
	if (q->count >= q->max_num){
		q->count = q->max_num;
	}

	return;
}

/* Return:  success: = 0 ;  fail: other  */
int hisiap_ringbuffer_read(struct hisiap_ringbuffer_s *q, u8 *element, u32 len)
{
	u32 ridx;

	if (IS_ERR_OR_NULL(q)) {
		BB_PRINT_ERR("[%s], parameter q ringbuffer is null!\n", __func__);
		return -1;
	}

	if (IS_ERR_OR_NULL(element)) {
		BB_PRINT_ERR("[%s], parameter element element is null!\n", __func__);
		return -1;
	}

	if (0 == q->count){
		return -1;
	}
	q->count --;

	if (q->count >= q->max_num){
		q->r_idx = q->rear;
	}

	if (q->r_idx >= q->max_num){
		 q->r_idx = 0;
	}

	ridx = q->r_idx++;

	memcpy_s((void *)element, len, (void *)&q->data[(long)ridx * q->field_count],
		q->field_count * sizeof(u8));

	return 0;
}

/*************************************************************************
Function:		hisiap_is_ringbuffer_full
Description:	判断ringbuffer是否已经写满；
Input:		buffer_addr: buffer首地址；
Return:		0：buffer未满；1：buffer已满；-1: 查询无效；
*************************************************************************/
int hisiap_is_ringbuffer_full(const void *buffer_addr)
{
	if (NULL == buffer_addr)
		return -1;

	return (int)(((struct hisiap_ringbuffer_s *)buffer_addr)->is_full);
}/*lint !e818*/

/*lint -e818*/
void get_ringbuffer_start_end(struct hisiap_ringbuffer_s *q, u32 *start, u32 *end)
{
	if (IS_ERR_OR_NULL(q) || IS_ERR_OR_NULL(start) || IS_ERR_OR_NULL(end)) {
		BB_PRINT_ERR("[%s], parameter q 0x%pK start 0x%pK end 0x%pK is null!\n",
			__func__, q, start, end);
		return;
	}

	if (q->is_full) {
		if ((q->rear >= q->max_num) || (q->rear <= 0)) {
			*start = 0;
			*end = q->max_num - 1;
		} else if (q->rear) {
			*start = q->rear;
			*end = q->rear - 1 + q->max_num;
		}
	} else {
		*start = 0;
		*end = q->rear - 1;
	}
}
/*lint +e818*/

/*lint -e818*/
bool is_ringbuffer_empty(struct hisiap_ringbuffer_s *q)
{
	if (IS_ERR_OR_NULL(q)) {
		BB_PRINT_ERR("[%s], parameter q ringbuffer is null!\n", __func__);
		return true;
	}

	if ((0 == q->is_full) && (0 == q->rear)) {
		return true;
	}

	return false;
}
/*lint +e818*/

/*lint -e818*/
bool is_ringbuffer_invalid(u32 field_count, u32 len, struct hisiap_ringbuffer_s *q)
{
	if (IS_ERR_OR_NULL(q)) {
		BB_PRINT_ERR("[%s], parameter q ringbuffer is null!\n", __func__);
		return true;
	}

	if (unlikely(q->field_count != field_count)) {
		BB_PRINT_ERR("%s() fail:hisiap_ringbuffer_s field_count %u != %u.\n", 
			__func__, q->field_count, field_count);
		return true;
	}

	if (unlikely(q->rear > q->max_num)) {
		BB_PRINT_ERR("%s() fail:q->rear %u > q->max_num %u.\n", 
			__func__, q->rear, q->max_num);
		return true;
	}

	if ( unlikely(
		(q->max_num <= 0)
		|| (field_count <= 0)
		|| (len <= sizeof(struct hisiap_ringbuffer_s)
		|| ( q->max_num  > ((len - sizeof(struct hisiap_ringbuffer_s))/field_count)) )
		)
	) {
		BB_PRINT_ERR("%s() fail:hisiap_ringbuffer_s max_num %u field_count %u len %u.\n", 
			__func__, q->max_num, field_count, len);
		return true;
	}

	return false;
}
/*lint +e818*/
