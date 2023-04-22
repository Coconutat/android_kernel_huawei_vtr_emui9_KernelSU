/*
 * blackbox header file (blackbox: kernel run data recorder.)
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __BB_PUB_H__
#define __BB_PUB_H__

#include <linux/module.h>
#include <linux/hisi/rdr_types.h>
#include <mntn_public_interface.h>

#define STR_MODULENAME_MAXLEN		16
#define STR_EXCEPTIONDESC_MAXLEN	48
#define STR_TASKNAME_MAXLEN		16
#define STR_USERDATA_MAXLEN		64

#define PATH_ROOT           "/data/hisi_logs/"
#define RDR_REBOOT_TIMES_FILE    "/data/hisi_logs/reboot_times.log"
#define RDR_ERECOVERY_REASON_FILE    "/cache/recovery/last_erecovery_entry"
#define RDR_UNEXPECTED_REBOOT_MARK_ADDR    0x2846579

#define INT_IN_FLAG             0xAAAAUL
#define INT_EXIT_FLAG           0xBBBBUL

#define BBOX_SAVE_DONE_FILENAME "/DONE"    /*异常文件目录log保存完毕的标志文件名字*/

/* 版本检测宏定义开始 */
#define FILE_EDITION	"/proc/log-usertype"
#define OVERSEA_USER	5
#define BETA_USER	3
#define COMMERCIAL_USER	1

#define START_CHAR_0	'0'
#define END_CHAR_9	'9'

enum EDITION_KIND {
	EDITION_USER		= 1,
	EDITION_INTERNAL_BETA	= 2,
	EDITION_OVERSEA_BETA	= 3,
	EDITION_MAX
};
/* 版本检测宏定义结束 */

/*异常时，log保存完毕的标志*/
enum SAVE_STEP {
    BBOX_SAVE_STEP1     = 0x1,
    BBOX_SAVE_STEP2     = 0x2,
    BBOX_SAVE_STEP3     = 0x3,
    BBOX_SAVE_STEP_DONE = 0x100
};

/*this is for test*/
enum rdr_except_reason_e {
        RDR_EXCE_WD         = 0x01,/*watchdog timeout*/
        RDR_EXCE_INITIATIVE,       /*initictive call sys_error*/
        RDR_EXCE_PANIC,            /*ARM except(eg:data abort)*/
        RDR_EXCE_STACKOVERFLOW,
        RDR_EXCE_DIE,
        RDR_EXCE_UNDEF,
        RDR_EXCE_MAX
};

enum PROCESS_PRI {
    RDR_ERR      = 0x01,
    RDR_WARN,
    RDR_OTHER,
    RDR_PPRI_MAX
};

enum REBOOT_PRI {
    RDR_REBOOT_NOW      = 0x01,
    RDR_REBOOT_WAIT,
    RDR_REBOOT_NO,
    RDR_REBOOT_MAX
};

enum REENTRANT {
    RDR_REENTRANT_ALLOW = 0xff00da00,
    RDR_REENTRANT_DISALLOW
};

enum UPLOAD_FLAG {
    RDR_UPLOAD_YES = 0xff00fa00,
    RDR_UPLOAD_NO
};


enum RDR_RETURN {
    RDR_SUCCESSED                   = 0x9f000000,
    RDR_FAILD                       = 0x9f000001,
    RDR_NULLPOINTER                 = 0x9f0000ff
};

typedef void (*rdr_e_callback)( u32, void* );

/*
 *   struct list_head   e_list;
 *   u32 modid,			    exception id;
 *		if modid equal 0, will auto generation modid, and return it.
 *   u32 modid_end,		    can register modid region. [modid~modid_end];
		need modid_end >= modid,
 *		if modid_end equal 0, will be register modid only,
		but modid & modid_end cant equal 0 at the same time.
 *   u32 process_priority,	exception process priority
 *   u32 reboot_priority,	exception reboot priority
 *   u64 save_log_mask,		need save log mask
 *   u64 notify_core_mask,	need notify other core mask
 *   u64 reset_core_mask,	need reset other core mask
 *   u64 from_core,		    the core of happen exception
 *   u32 reentrant,		    whether to allow exception reentrant
 *   u32 exce_type,		    the type of exception
 *   char* from_module,		    the module of happen excption
 *   char* desc,		        the desc of happen excption
 *   rdr_e_callback callback,	will be called when excption has processed.
 *   u32 reserve_u32;		reserve u32
 *   void* reserve_p		    reserve void *
 */
struct rdr_exception_info_s {
	struct list_head e_list;
	u32	e_modid;
	u32	e_modid_end;
	u32	e_process_priority;
	u32	e_reboot_priority;
	u64	e_notify_core_mask;
	u64	e_reset_core_mask;
	u64	e_from_core;
	u32	e_reentrant;
	u32	e_exce_type;
	u32	e_exce_subtype;
	u32	e_upload_flag;
	u8	e_from_module[MODULE_NAME_LEN];
	u8	e_desc[STR_EXCEPTIONDESC_MAXLEN];
	u32	e_reserve_u32;
	void*	e_reserve_p;
	rdr_e_callback e_callback;
};

/*
 * func name: pfn_cb_dump_done
 * func args:
 *    u32   modid
 *      exception id
 *    u64   coreid
 *      which core done
 * return value		null
 */
typedef void (*pfn_cb_dump_done)( u32 modid, u64 coreid);

/*
 * func name: pfn_dump
 * func args:
 *    u32   modid
 *      exception id
 *    u64   coreid
 *      exception core
 *    u32   etype
 *      exception type
 *    char*     logpath
 *      exception log path
 *    pfn_cb_dump_done fndone
 * return       mask bitmap.
 */
typedef void (*pfn_dump)( u32 modid, u32 etype, u64 coreid,
	char* logpath, pfn_cb_dump_done fndone);
/*
 * func name: pfn_reset
 * func args:
 *    u32   modid
 *      exception id
 *    u32   coreid
 *      exception core
 *    u32   e_type
 *      exception type
 * return value		null
 */
typedef void (*pfn_reset)( u32 modid, u32 etype, u64 coreid);

/*
 * func name: pfn_cleartext_ops
 * func args:
 *   log_dir_path: the direcotory path of the file to be written in clear text format
 *   u64 log_addr: the start address of the reserved memory for each core
 *   u32 log_len: the length of the reserved memory for each core
 *
 * Attention:
 * the user can't dump through it's saved dump address but must in use of the log_addr
 *
 * return value
 *     < 0 error
 *     >=0 success
 */
typedef int (*pfn_cleartext_ops)(char *log_dir_path, u64 log_addr, u32 log_len);


typedef int (*pfn_exception_init_ops)(u8 *phy_addr, u8 *virt_addr, u32 log_len);

typedef int (*pfn_exception_analysis_ops)(u64 etime, u8 *addr, u32 len,
	struct rdr_exception_info_s *exception);

struct rdr_module_ops_pub {
    pfn_dump    ops_dump;
    pfn_reset   ops_reset;
};

struct rdr_register_module_result {
    u64   log_addr;
    u32     log_len;
    RDR_NVE nve;
};

#ifdef CONFIG_HISI_BB
/*
 * func name: rdr_register_exception_type
 * func args:
 *    struct rdr_exception_info_pub* s_e_type
 * return value		e_modid
 *	< 0 error
 *	>=0 success
 */
u32 rdr_register_exception(struct rdr_exception_info_s* e);

/*
 * func name: rdr_unregister_exception
 * func args:
 *   u32 modid,			exception id;
 * return
 *	< 0 fail
 *	>=0 success
 */
int rdr_unregister_exception(u32 modid);

/*
  * func name: hisi_bbox_map
  * func args:
  *    @paddr: physical address in black box
  *    @size: size of memory
  * return:
  *    success: virtual address
  *    fail: NULL or -ENOMEM
  */
void *hisi_bbox_map(phys_addr_t paddr, size_t size);

/*
  * func name: hisi_bbox_unmap
  * func args:
  *    @addr: virtual address that alloced by hisi_bbox_map
  */
void hisi_bbox_unmap(const void *vaddr);

/*
 * func name: rdr_register_module_ops
 * func args:
 *   u32 coreid,       core id;
 *   struct rdr_module_ops_pub* ops;
 *   struct rdr_register_module_result* retinfo
 *
 * return value		e_modid
 *	< 0 error
 *	>=0 success
 */
int rdr_register_module_ops(
        u64 coreid,
        struct rdr_module_ops_pub* ops,
        struct rdr_register_module_result* retinfo);

/*
 * func name: rdr_unregister_module_ops
 * func args:
 *   u64 coreid,		core id;
 * return
 *	< 0 fail
 *	>=0 success
 */
int rdr_unregister_module_ops(u64 coreid);

/*
 * func name: rdr_system_error
 * func args:
 *   u32 modid,			modid( must be registered);
 *   u32 arg1,			arg1;
 *   u32 arg2,			arg2;
 *   char *  data,			short message.
 *   u32 length,		len(IMPORTANT: <=4k)
 * return void
 */
void rdr_system_error(u32 modid, u32 arg1, u32 arg2);

void rdr_syserr_process_for_ap(u32 modid, u64 arg1, u64 arg2);

/*
 * func name: rdr_cleartext_print
 *
 * append(save) data to path.
 * func args:
 *  struct file *fp: the pointer of file which to save the clear text.
 *  bool *error: to fast the cpu process when there is error happened before nowadays print, please
 *              refer the function bbox_head_cleartext_print to get the use of this parameter.
 *
 * return
 */
void rdr_cleartext_print(struct file *fp, bool *error, const char *format, ...);

/*
 * func name: rdr_register_cleartext_ops
 * func args:
 *   u64 core_id: the same with the parameter coreid of function rdr_register_module_ops
 *   pfn_cleartext_ops ops_fn: the function to write the content of reserved memory in clear text format
 *
 * return value
 *	< 0 error
 *	0 success
 */
int rdr_register_cleartext_ops(u64 coreid, pfn_cleartext_ops ops_fn);

/*
 * func name: bbox_cleartext_get_filep
 *
 * Get the file descriptor pointer whose abosolute path composed by the dir_path&file_name
 * and initialize it.
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
struct file *bbox_cleartext_get_filep(char *dir_path, char *file_name);

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
void bbox_cleartext_end_filep(struct file *fp, char *dir_path, char *file_name);

/*
 * 函数名: bbox_check_edition
 * 函数参数:
 *     void
 * 返回值:
 *     unsigned int:	返回版本信息
 *				0x01        USER
 *				0x02        INTERNAL BETA
 *                         0x03        OVERSEA BETA
 *
 * 该函数会访问用户的data分区，因此依赖于文件系统的正确挂载。
 * 由于没有超时机制，等待文件系统挂载的过程会导致进程进入
 * 不确定时长的睡眠。综上在不能睡眠的场景不能调用该接口。
 */
unsigned int bbox_check_edition(void);
int rdr_wait_partition(char *path, int timeouts);
void rdr_set_wdt_kick_slice(u64 kickslice);
u64 rdr_get_last_wdt_kick_slice(void);
u64 get_32k_abs_timer_value(void);
void save_log_to_dfx_tempbuffer(u32 reboot_type);
void clear_dfx_tempbuffer(void);
void systemerror_save_log2dfx(u32 reboot_type);
u64 rdr_get_logsize(void);
u32 rdr_get_diaginfo_size(void);
u32 rdr_get_lognum(void);
char *rdr_get_timestamp(void);
void *bbox_vmap(phys_addr_t paddr, size_t size);
int rdr_dir_size(char *path, bool recursion);
#else
static inline void *hisi_bbox_map(phys_addr_t paddr, size_t size){ return NULL; }
static inline u32 rdr_register_exception(struct rdr_exception_info_s* e){ return 0;}
static inline int rdr_unregister_exception(u32 modid){ return 0; }
static inline int rdr_register_module_ops(
        u64 coreid,
        struct rdr_module_ops_pub* ops,
        struct rdr_register_module_result* retinfo){ return -1; }
static inline int rdr_unregister_module_ops(u64 coreid){ return 0; }
static inline void rdr_system_error(u32 modid, u32 arg1, u32 arg2){}
static inline void rdr_syserr_process_for_ap(u32 modid, u64 arg1, u64 arg2){}

static inline unsigned int bbox_check_edition(void){return EDITION_USER;}
static inline int rdr_wait_partition(char *path, int timeouts) { return 0; }
static inline void rdr_set_wdt_kick_slice(u64 kickslice){ return; }
static inline u64 rdr_get_last_wdt_kick_slice(void){ return 0; }
static inline u64 get_32k_abs_timer_value(void) { return 0; }
static inline void save_log_to_dfx_tempbuffer(u32 reboot_type){return;};
static inline void clear_dfx_tempbuffer(void){return;};
static inline void systemerror_save_log2dfx(u32 reboot_type){return;}
static inline void hisi_bbox_unmap(const void *vaddr){return;}
static inline u64 rdr_get_logsize(void){return 0;}
static inline u32 rdr_get_diaginfo_size(void){return 0};
static inline u32 rdr_get_lognum(void){return 0;}
static inline char *rdr_get_timestamp(void){return NULL;}
static inline void *bbox_vmap(phys_addr_t paddr, size_t size){return NULL;}
static inline int rdr_dir_size(char *path, bool recursion){return 0;}
#endif

void get_exception_info(unsigned long *buf, unsigned long *buf_len);
#define RDR_REBOOTDUMPINFO_FLAG     0xdd140607

#endif/* End #define __BB_PUB_H__ */

