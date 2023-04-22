#ifndef __HI6402_DEBUG_H__
#define __HI6402_DEBUG_H__

#include "hicodec_debug.h"

#define PAGE_6402_CODEC_BASE_ADDR  0x20000000
#define CODEC_BASE_ADDR_PAGE_CFG 0x7000
#define CODEC_BASE_ADDR_PAGE_DIG 0x7200

#define HI6402_DBG_PAGE_CFG_CODEC_START (PAGE_6402_CODEC_BASE_ADDR +CODEC_BASE_ADDR_PAGE_CFG)
#define HI6402_DBG_PAGE_CFG_CODEC_END   (PAGE_6402_CODEC_BASE_ADDR + CODEC_BASE_ADDR_PAGE_CFG + 0xff)
#define HI6402_DBG_PAGE_DIG_CODEC_START (PAGE_6402_CODEC_BASE_ADDR + CODEC_BASE_ADDR_PAGE_DIG)
#define HI6402_DBG_PAGE_DIG_CODEC_END   (PAGE_6402_CODEC_BASE_ADDR + CODEC_BASE_ADDR_PAGE_DIG + 0x1ff)

static struct hicodec_dump_reg_entry hi6402_dump_table[] = {
	{"PAGE CFG", HI6402_DBG_PAGE_CFG_CODEC_START, HI6402_DBG_PAGE_CFG_CODEC_END, 1},
	{"PAGE DIG", HI6402_DBG_PAGE_DIG_CODEC_START, HI6402_DBG_PAGE_DIG_CODEC_END, 1},
};

static struct hicodec_dump_reg_info hi6402_dump_info = {
	.entry = hi6402_dump_table,
	.count = sizeof(hi6402_dump_table) / sizeof(struct hicodec_dump_reg_entry),
};

#endif