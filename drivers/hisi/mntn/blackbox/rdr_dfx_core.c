/*
 * blackbox. (kernel run data recorder.)
 *
 * Copyright (c) 2013 Huawei Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/of.h>

#include <linux/hisi/hisi_bootup_keypoint.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/hisi/util.h>
#include <linux/hisi/rdr_hisi_platform.h>
#include <linux/hisi/rdr_dfx_core.h>
#include <linux/hisi/kirin_partition.h>
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG HISI_BLACKBOX_TAG

#include <hisi_partition.h>
#include "rdr_print.h"
#include "rdr_inner.h"

int dfx_open(void)
{
	void *buf;
	char p_name[BDEVNAME_SIZE + 12];
	int ret, fd_dfx;

	BB_PRINT_START();

	buf = kzalloc(SZ_4K, GFP_KERNEL);
	if (!buf) {
		BB_PRINT_ERR("%s():%d:kzalloc buf1 fail\n", __func__, __LINE__);
		return -ENOMEM;
	}
	ret = flash_find_ptn(PART_DFX, buf);
	if (0 != ret) {
		BB_PRINT_ERR("%s():%d:flash_find_ptn fail\n", __func__, __LINE__);
		kfree(buf);
		return ret;
	}

	memset(p_name, 0, sizeof(p_name));
	strncpy(p_name, buf, sizeof(p_name));
	p_name[BDEVNAME_SIZE + 11] = '\0';
	kfree(buf);

	fd_dfx = sys_open(p_name, O_RDWR, FILE_LIMIT);

	return fd_dfx;
}

int dfx_read(u32 module, void *buffer, u32 size)
{
	int ret, fd_dfx, cnt=0;
	mm_segment_t old_fs = get_fs(); //lint !e501

	if (dfx_size_tbl[module] < size || !dfx_size_tbl[module])
		return cnt;
	if (!buffer)
		return cnt;
	/*lint -e501 -esym(501,*)*/
	set_fs(KERNEL_DS);
	/*lint -e501 +esym(501,*)*/

	fd_dfx = dfx_open();
	ret = sys_lseek(fd_dfx, dfx_addr_tbl[module], SEEK_SET);
	if (ret < 0) {
		BB_PRINT_ERR("%s():%d:lseek fail[%d]\n", __func__, __LINE__, ret);
		goto close;
	}
	cnt = sys_read(fd_dfx, buffer, size);
	if (cnt < 0) {
		BB_PRINT_ERR("%s():%d:read fail[%d]\n", __func__, __LINE__, cnt);
		goto close;
	}
close:
	sys_close(fd_dfx);
	set_fs(old_fs);
	return cnt;
}

int dfx_write(u32 module,void *buffer, u32 size)
{
	int ret, fd_dfx, cnt=0;
	mm_segment_t old_fs = get_fs();

	if (dfx_size_tbl[module] < size || !dfx_size_tbl[module])
		return cnt;
	if (!buffer)
		return cnt;
	/*lint -e501 -esym(501,*)*/
	set_fs(KERNEL_DS);
	/*lint -e501 +esym(501,*)*/

	fd_dfx = dfx_open();
	ret = sys_lseek(fd_dfx, dfx_addr_tbl[module], SEEK_SET);
	if (ret < 0) {
		BB_PRINT_ERR("%s():%d:lseek fail[%d]\n", __func__, __LINE__, ret);
		goto close;
	}
	cnt = sys_write(fd_dfx, buffer, size);
	if (cnt < 0) {
		BB_PRINT_ERR("%s():%d:read fail[%d]\n", __func__, __LINE__, cnt);
		goto close;
	}
close:
	sys_close(fd_dfx);
	set_fs(old_fs);
	return cnt;
}
