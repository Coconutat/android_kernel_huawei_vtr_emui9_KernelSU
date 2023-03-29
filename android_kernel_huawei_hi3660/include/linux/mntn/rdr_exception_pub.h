#ifndef __RDR_EXCEPTION_PUB_H__
#define __RDR_EXCEPTION_PUB_H__

#define MODULE_MAGIC                0xbaba0514
#define MODULE_VALID                 1
#define MODULE_EXCEPTION_REGISTER_MAXNUM   512
#define RDR_MODULE_NAME_LEN  16
#define RDR_EXCEPTIONDESC_MAXLEN 48

typedef struct excep_time_t {
    u64 tv_sec;
    u64 tv_usec;
}excep_time, rdr_time_st;

struct exc_description_s{
    u32 e_modid;                                                 /* 异常id */
    u8 e_process_level;                                        /* 异常处理级别:PROCESS_PRI */
    u8 e_reboot_priority;                                      /* 异常重启级别:REBOOT_PRI */
    u8 e_exce_type;                                             /* 异常类型 */ 
    u8 e_reentrant;                                              /* 异常是否可重入 */
    u64 e_notify_core_mask;                                /* 异常联动掩码 */
    u8 e_desc[RDR_EXCEPTIONDESC_MAXLEN];        /* 异常描述 */ 
};

struct exc_info_s{
    excep_time e_clock;                                         /* 模块触发异常时间 */
    u32 e_excepid;                                               /* 模块触发的异常id */
    u16 e_dump_status;                                        /* 模块将异常信息存预留内存的控制状态 */
    u16 e_save_status;                                         /* 代理将异常信息从预留内存导出的控制状态 */ 
};

struct exc_module_info_s{
    u32 magic;                                                      /* 使用宏MODULE_MAGIC */
    u16 e_mod_valid;                                            /* 模块写完注册的异常，则设置1 */
    u16 e_mod_num;                                            /* 模块注册异常个数 */
    u8 e_from_module[RDR_MODULE_NAME_LEN];   /* 模块名 */
    struct exc_info_s cur_info;                               /* 模块dump信息控制状态 */
    u32 e_mini_offset;                                          /* 模块最小集异常信息偏移值，基于模块预留内存首地址，从magic开始 */
    u32 e_mini_len;                                              /* 模块最小集异常信息长度 */
    u32 e_info_offset;                                          /* 模块全部异常信息偏移值，基于模块预留内存首地址，从magic开始 */
    u32 e_info_len;                                              /* 模块全部异常信息长度 */
    struct exc_description_s e_description[1];         /* 模块异常注册信息 */
};

enum MODULE_DUMP_STATUS {
    STATUS_INIT = 0,
    STATUS_DOING = 1,
    STATUS_DONE = 2,
};

#endif //__RDR_EXCEPTION_PUB_H__