#ifndef _OAM_RDR_H_
#define _OAM_RDR_H_

#include "plat_debug.h"
#define RDR_EXCEPTION(modid)           hisi_conn_rdr_system_error(modid,0,0)

#if (defined CONFIG_HISI_BB) && (defined CONFIG_HI110X_PLAT_BB)
#include <linux/hisi/rdr_pub.h>

#define HISI_CONN_BUF_LEN_MAX      (1024)
struct hisi_conn_rdr_dump_info_s {
    u32 modid;
    u64 coreid;
    pfn_cb_dump_done cb;
    char *pathname;
};
struct hisi_conn_info_s {
    unsigned int hisi_conn_run_status;
    unsigned int bbox_addr_offset;
    struct rdr_register_module_result hisi_conn_ret_info;
    struct hisi_conn_rdr_dump_info_s dump_info;
    void   *rdr_addr;
};
struct hisi_conn_stat_info {
    unsigned long long lastuploadtime;
    unsigned long long happen_cnt;
    unsigned long long upload_cnt;
};
typedef struct hisi_conn_ctl_s{
    struct hisi_conn_stat_info stat_info;
    unsigned long long interva_stime;
    unsigned int upload_en;
}hisi_conn_ctl_t;

extern int hisi_conn_rdr_init(void);
extern int32 hisi_conn_rdr_system_error(uint32 modid,uint32 arg1,uint32 arg2);
extern int hisi_conn_rdr_exit(void);
extern int32 hisi_conn_save_stat_info(char* buf,int32 index ,int32 limit);
extern  void plat_bbox_msg_hander(int32 subsys_type, int32 exception_type);
#else
#define HISI_BB_MOD_CONN_START 0
#define HISI_BB_MOD_CONN_END 100
static inline int hisi_conn_rdr_init(void){return 0;};
static inline int32 hisi_conn_rdr_system_error(uint32 modid,uint32 arg1,uint32 arg2){return 0;};
static inline int hisi_conn_rdr_exit(void){return 0;};
static inline int32 hisi_conn_save_stat_info(char* buf,int32 index ,int32 limit){return index;};
static inline  void plat_bbox_msg_hander(int32 subsys_type, int32 exception_type){};
#endif
enum rdr_hisi_conn_system_error_type {
    MODID_CONN_WIFI_EXEC = HISI_BB_MOD_CONN_START,
    MODID_CONN_WIFI_CHAN_EXEC,
    MODID_CONN_WIFI_WAKEUP_FAIL,
    MODID_CONN_BFGX_EXEC,
    MODID_CONN_BFGX_BEAT_TIMEOUT,
    MODID_CONN_BFGX_WAKEUP_FAIL,
    MODID_CONN_BOTT,
    MODID_CONN_EXC_END = HISI_BB_MOD_CONN_END,
};
enum bbox_upload_e {
    UPLOAD_ALLOW      = 0,
    UPLOAD_DISALLOW = 1,
};
#define MODID_CONN_ARRAY_LEN      (MODID_CONN_BOTT-MODID_CONN_WIFI_EXEC)
#endif
