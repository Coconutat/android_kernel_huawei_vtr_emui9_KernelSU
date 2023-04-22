/**********************************************************
 * Filename:    zrhung_lastword.c
 *
 * Discription: kernel process of the hung lastword
 *
 * Copyright: (C) 2017 huawei.
 *
 * Author: fanghua(00396017) zhangliang(00175161)
 *
**********************************************************/
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/poll.h>
#ifdef CONFIG_HISI_DFX_CORE
#include "linux/hisi/rdr_dfx_core.h"
#endif

#include "zrhung_lastword.h"
#include "zrhung_transtation.h"
#include <chipset_common/hwzrhung/zrhung.h>
#include "zrhung_common.h"

int hlastword_write(void* buf, uint32_t len)
{
	if (!buf || len > HTRANS_TOTAL_BUF_SIZE) {
		HTRANS_ERROR("param not right\n");
		return -1;
	}

#ifdef CONFIG_HISI_DFX_CORE
	dfx_write(DFX_ZEROHUNG, buf, len);
#endif

	return 0;
}

int hlastword_read(void* buf, uint32_t len)
{
	if (!buf || len > HTRANS_TOTAL_BUF_SIZE) {
		HTRANS_ERROR("param not right\n");
		return -1;
	}

#ifdef CONFIG_HISI_DFX_CORE
	dfx_read(DFX_ZEROHUNG, buf, len);
#endif

	return 0;
}

int hlastword_init(void)
{
	return 0;
}

