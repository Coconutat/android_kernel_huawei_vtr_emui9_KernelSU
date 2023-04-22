/*
 * blackbox diaginfo file
 *
 * Copyright (c) 2018 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __HISI_BBOX_DIAGINFO_H__
#define __HISI_BBOX_DIAGINFO_H__
#include <bbox_diaginfo_id_def.h>

#define BBOX_DIAGINFO_OK 			(0)
#define BBOX_DIAGINFO_INVALIDPAR 	(-1)
#define BBOX_DIAGINFO_REPEATMSG 		(-2)
#define BBOX_DIAGINFO_STR_MSG_ERR 	(-3)
#define BBOX_DIAGINFO_NO_MEM		(-4)
#define BBOX_DIAGINFO_AP_SR 			(-5)
#define BBOX_DIAGINFO_INV_ID			(-6)
#define BBOX_DIAGINFO_OVER_COUNT	(-7)

#define DIAGINFO_STRING_MAX_LEN         (256)
#define DIAGINFO_COUNT_MAX			(10)

#ifdef CONFIG_HISI_BB_DIAGINFO
int bbox_diaginfo_exception_save2fs(void);
void mntn_ipc_msg_nb(unsigned int * msg);
void bbox_ap_ipc_init(void);
int bbox_diaginfo_init(void);
int bbox_lpmcu_diaginfo_init(void);
int bbox_diaginfo_register(unsigned int err_id,char* pdata,unsigned int data_len, u64 ts);
int bbox_diaginfo_record(unsigned int err_id,const char *fmt, ...);
void cpu_up_diaginfo_record(unsigned int cpu, int status);
void bbox_diaginfo_dump_lastmsg(void);
#else
static inline int bbox_diaginfo_exception_save2fs(void) {return 0;};
static inline int bbox_diaginfo_init(void) {return 0;};
static inline int bbox_diaginfo_record(unsigned int err_id,const char *fmt, ...) {return 0;};
static inline void mntn_ipc_msg_nb(unsigned int * msg) {return;};
static inline void cpu_up_diaginfo_record(unsigned int cpu, int status) {return;};
static inline void bbox_diaginfo_dump_lastmsg(void) {return;};
#endif

#endif /* __HISI_BBOX_DIAGINFO_H__ */
