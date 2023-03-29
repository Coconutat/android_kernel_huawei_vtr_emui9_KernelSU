
#ifndef __RDR_HISI_AP_ADAPTER_H__
#define __RDR_HISI_AP_ADAPTER_H__

#include <linux/thread_info.h>
#include <linux/hisi/rdr_hisi_platform.h>
#include <linux/hisi/rdr_hisi_ap_hook.h>
#include <soc_acpu_baseaddr_interface.h>
#include <soc_sctrl_interface.h>

#define REG_NAME_LEN 12
#define PRODUCT_VERSION_LEN 32
#define PRODUCT_DEVICE_LEN 32
#define REGS_DUMP_MAX_NUM   10
#define AP_DUMP_MAGIC   0x19283746
#define BBOX_VERSION    0x1000B	/*v1.0.11 */
#define AP_DUMP_END_MAGIC   0x1F2E3D4C
#define SIZE_1K         0x400
#define SYSTEM_BUILD_POP    "/system/build.prop"
#define AMNTN_MODULE_NAME_LEN 12
#define NMI_NOTIFY_LPM3_ADDR SOC_SCTRL_SCLPMCUCTRL_ADDR(SOC_ACPU_SCTRL_BASE_ADDR)
#define WDT_KICK_SLICE_TIMES    (3)
#define FPGA 1

#define PSTORE_PATH            "/proc/balong/pstore/"
#define FASTBOOT_LOG_FILE      "/proc/balong/log/fastboot_log"
#define LAST_FASTBOOT_LOG_FILE "/proc/balong/log/last_fastboot_log"


typedef struct {
	rdr_hisiap_dump_func_ptr dump_funcptr;
	unsigned char *dump_addr;
	unsigned int dump_size;
	char module_name[AMNTN_MODULE_NAME_LEN];
} module_dump_mem_info;

typedef struct {
	char reg_name[12];
	u32 reg_size;
	u64 reg_base;
	void __iomem *reg_map_addr;
	unsigned char *reg_dump_addr;
} regs_info;

typedef struct {
	unsigned int dump_magic;
	unsigned char version[PRODUCT_VERSION_LEN];
	u32 modid;
	u32 e_exce_type;
	u32 e_exce_subtype;
	u64 coreid;
	u64 slice;
	struct rdr_register_module_result ap_rdr_info;
	unsigned int enter_times;	/* 重入计数，初始化为0，每次进入++； */

	unsigned int num_reg_regions;
	regs_info dump_regs_info[REGS_DUMP_MAX_NUM];

	unsigned char *hook_buffer_addr[HK_MAX];

	percpu_buffer_info hook_percpu_buffer[HK_PERCPU_TAG];

	unsigned char *last_task_struct_dump_addr[NR_CPUS];
	unsigned char *last_task_stack_dump_addr[NR_CPUS];

	char log_path[LOG_PATH_LEN];
	unsigned char *rdr_ap_area_map_addr;	/* rdr非配给ap内存的映射地址 */
	module_dump_mem_info module_dump_info[MODU_MAX];
	u64 wdt_kick_slice[WDT_KICK_SLICE_TIMES];
	unsigned char device_id[PRODUCT_DEVICE_LEN];
	u64 bbox_version;	/* 表示BBox版本信息 */
	unsigned int end_magic;	/* 标记结构体末尾，用于判断结构体范围 */
	char reserved[1];	/*sizeof(AP_EH_ROOT)=1024byte*/
} AP_EH_ROOT;			/*AP_EH_ROOT占用2K空间，通过get_rdr_hisiap_dump_addr函数预留 */

/*外部可以使用的变量声明*/
void get_product_version(char *version, size_t count);
void print_debug_info(void);
int rdr_hisiap_dump_init(struct rdr_register_module_result *retinfo);
void rdr_hisiap_dump(u32 modid, u32 etype, u64 coreid,
		     char *log_path, pfn_cb_dump_done pfn_cb);

/* 测试函数声明 */
void ap_exch_task_stack_dump(int taskPid);
void ap_exch_buf_show(unsigned int offset, unsigned int size);
void ap_exch_hex_dump(unsigned char *buf, unsigned int size,
		      unsigned char per_row);
int ap_exchUNDEF(void *arg);
int ap_exchSWI(void *arg);
int ap_exchPABT(void *arg);
int ap_exchDABT(void *arg);
int save_mntndump_log(void *arg);

u32 bbox_test_get_exce_size(void);
int bbox_test_registe_exception(void);
void bbox_test_enable(int enable);
void bbox_test_print_exc(void);
int bbox_test_unregiste_exception(void);
int rdr_hisiap_cleartext_print(char *dir_path, u64 log_addr, u32 log_len);
#endif
