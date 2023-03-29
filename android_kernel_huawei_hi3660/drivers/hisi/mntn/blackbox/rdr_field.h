/*
 * blackbox header file (blackbox: kernel run data recorder.)
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __BB_FIELD_H__
#define __BB_FIELD_H__

#include <linux/types.h>
#include <linux/hisi/rdr_pub.h>
#include "rdr_inner.h"

#define RDR_TIME_LEN            16

#define FILE_MAGIC              0xdead8d8d
#ifdef CONFIG_SMP
#define RDR_SMP_FLAG            1
#else
#define RDR_SMP_FLAG            0
#endif

#define RDR_PRODUCT "PRODUCT_NAME"	/* "hi3630_udp" */
#define RDR_VERSION             ((RDR_SMP_FLAG << 16) | (0x203 << 0))
						    /* v2.02 2018.4.11 */
#define RDR_BASEINFO_SIZE  0x200

enum RDR_AREA_LIST {
	RDR_AREA_AP = 0x0,
	RDR_AREA_CP = 0x1,
	RDR_AREA_TEEOS = 0x2,
	RDR_AREA_HIFI = 0x3,
	RDR_AREA_LPM3 = 0x4,
	RDR_AREA_IOM3 = 0x5,
	RDR_AREA_ISP = 0x6,
	RDR_AREA_IVP = 0x7,
	RDR_AREA_EMMC = 0x8,
	RDR_AREA_MODEMAP = 0x9,
	RDR_AREA_CLK = 0xA,
	RDR_AREA_REGULATOR = 0xB,
	RDR_AREA_BFM = 0xC,
	RDR_AREA_HISEE = 0xD,
	RDR_AREA_NPU   = 0xE,
	RDR_AREA_CONN  = 0xF,
	RDR_AREA_EXCEPTION_TRACE = 0x10,
	RDR_AREA_MAXIMUM = 0x11
};

struct rdr_base_info_s {
	u32 modid;
	u32 arg1;
	u32 arg2;
	u32 e_core;
	u32 e_type;
	u32 e_subtype;
	u32 start_flag;
	u32 savefile_flag;
	u32 reboot_flag;
	u8 e_module[MODULE_NAME_LEN];
	u8 e_desc[STR_EXCEPTIONDESC_MAXLEN];

	u8 datetime[DATATIME_MAXLEN];
};

#define RDR_BUILD_DATE_TIME_LEN 16
struct rdr_top_head_s {
	u32 magic;
	u32 version;
	u32 area_number;
	u32 reserve;
	u8 build_time[RDR_BUILD_DATE_TIME_LEN];
	u8 product_name[16];
	u8 product_version[16];
};

struct rdr_area_s {
	u64 offset;	/* offset from area, unit is bytes(1 bytes) */
	u32 length;	/* unit is bytes */
};

struct rdr_cleartext_s {
	u8 savefile_flag;
	u8 aucResv[3];
};

#pragma pack(4)
struct rdr_struct_s {
	struct rdr_top_head_s top_head;
	struct rdr_base_info_s base_info;
	struct rdr_area_s area_info[RDR_AREA_MAXIMUM];
	struct rdr_cleartext_s cleartext_info;
	u8 padding2[RDR_BASEINFO_SIZE - sizeof(struct rdr_top_head_s)
		    - sizeof(struct rdr_area_s) * RDR_AREA_MAXIMUM
		    - sizeof(struct rdr_base_info_s) - sizeof(struct rdr_cleartext_s)];
};
#pragma pack()

/* (RDR_AREA_RES - sizeof(struct rdr_area_head_s))/sizeof(u32) */
/* TYPE,reference for parser: */
enum rdr_fid_type_e {
	RDR_LONG,		/* long long    */
	RDR_ARR,		/* array  */
	RDR_QUE,		/* queue  */
	RDR_STR			/* string */
};

#define RDR_FIELD_TYPE(type) (type << 0)	/* use 2 bits */
#define RDR_FIELD_USED       (1 << 2)	/* use 1 bits */
#define RDR_FID_MASK 0xffff0000
#define rdr_offset(type, member) ((u64)&(((type *)0)->member))

struct rdr_field_s {
	u32 id;
	u32 offset;		/* offset from rdr_a0_struct_s, unit is byte */
	u32 size;		/* unit is bytes. */
	u32 reserve;		/* type/registed etc. */
};

struct field_rec_s {
	u32 id;
	u32 mask;
	u32 type;
	u32 len;
	u32 count;
	void *addr;
	parse_record_t f;
};

/* cppcheck-suppress * */
#define RDR_ASSERT(ret) \
	do { \
		if ((ret) != 0) \
			return -1; \
	} while (0)

struct rdr_struct_s *rdr_get_pbb(void);
struct rdr_struct_s *rdr_get_tmppbb(void);
void rdr_clear_tmppbb(void);
u64 rdr_get_pbb_size(void);
int rdr_field_init(void);
void rdr_set_area_info(int index, u32 size);
void rdr_save_args(u32 modid, u32 arg1, u32 arg2);
void rdr_show_base_info(int flag);
void rdr_fill_edata(struct rdr_exception_info_s *e, char *date);
int rdr_get_areainfo(enum RDR_AREA_LIST area,
		     struct rdr_register_module_result *retinfo);
#endif /* End #define __BB_FIELD_H__ */
