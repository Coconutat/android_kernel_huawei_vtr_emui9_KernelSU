

/*******************************************************************************
  1 头文件包含
 *******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/thread_info.h>
#include <linux/hardirq.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/notifier.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/preempt.h>
#include <asm/cacheflush.h>
#include <linux/kmsg_dump.h>
#include <linux/slab.h>
#include <linux/kdebug.h>
#include <linux/hisi/mntn_record_sp.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/blkdev.h>
#include <linux/hisi/util.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/hisi/hisi_bootup_keypoint.h>
#include "rdr_hisi_ap_adapter.h"
#include <linux/hisi/rdr_hisi_ap_ringbuffer.h>
#include "../rdr_inner.h"
#include "../../mntn_filesys.h"
#include <linux/hisi/mntn_dump.h>
#include <linux/hisi/eeye_ftrace_pub.h>
#include <linux/mfd/hisi_pmic.h>
#include <linux/hisi/hisi_powerkey_event.h>
#include <linux/hisi/hisi_sp805_wdt.h>
#include <linux/hisi/mntn_l3cache_ecc.h>
#include <libhwsecurec/securec.h>
#include <linux/version.h>
#include <linux/hisi/hisi_pstore.h>
#include <linux/hisi/hisi_bbox_diaginfo.h>
#include <linux/watchdog.h>
#include <mntn_subtype_exception.h>
#include "../rdr_print.h"
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG HISI_BLACKBOX_TAG


#define BUFFER_SIZE			128
#define BUILD_DISPLAY_ID	"ro.confg.hw_systemversion"
#define SRC_KERNELDUMP		"/proc/balong/memory/kernel_dump"
#define SRC_DUMPEND		"/proc/balong/memory/dump_end"
#define SRC_LPMCU_DDR_MEMORY		"/proc/balong/memory/lpmcu_ddr_mem"
#define SRC_BL31_MNTN_MEMORY		"/proc/balong/memory/bl31_mntn_mem"
#define SRC_ETR_DUMP	"/proc/balong/memory/etr_dump"

static AP_EH_ROOT *g_rdr_ap_root;
static AP_RECORD_PC *g_bbox_ap_record_pc;
u64 g_hisiap_addr;
static char g_log_path[LOG_PATH_LEN];
static int rdr_ap_init;
static void __iomem *sctrl_map_addr;
static void __iomem *g_32k_timer_l32bit_addr;
static void __iomem *g_32k_timer_h32bit_addr;
static struct rdr_register_module_result current_info;
static struct mutex dump_mem_mutex;
static u64 g_mi_notify_lpm3_addr;
static unsigned long long g_pmu_reset_reg;
static unsigned long long g_pmu_subtype_reg;
int g_bbox_fpga_flag = -1;
/* -1 未初始化；0 非fpga板；1 fpga板 */

static unsigned int g_dump_buffer_size_tbl[HK_MAX] = {0};
static unsigned int last_task_struct_size = 0;
static unsigned int last_task_stack_size = 0;

/*struct rdr_exception_info_s {
struct list_head e_list;
u32 e_modid;
u32 e_modid_end;
u32 e_process_priority;
u32 e_reboot_priority;
u64 e_notify_core_mask;
u64 e_reset_core_mask;
u64 e_from_core;
u32 e_reentrant;
u32 e_exce_type;
u32 e_upload_flag;
u8  e_from_module[MODULE_NAME_LEN];
u8  e_desc[STR_EXCEPTIONDESC_MAXLEN];
u32 e_reserve_u32;
void*   e_reserve_p;
rdr_e_callback e_callback;
};*/
struct rdr_exception_info_s einfo[] = {
	{{0, 0}, MODID_AP_S_PANIC, MODID_AP_S_PANIC, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_PANIC, HI_APPANIC_RESERVED, (u32)RDR_UPLOAD_YES, "ap", "ap",
	 0, 0, 0},
	{{0, 0}, MODID_AP_S_L3CACHE_ECC, MODID_AP_S_L3CACHE_ECC, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_PANIC, HI_APPANIC_L3CACHE_ECC, (u32)RDR_UPLOAD_YES, "ap", "ap",
	 0, 0, 0},
	{{0, 0}, MODID_AP_S_PANIC_SOFTLOCKUP, MODID_AP_S_PANIC_SOFTLOCKUP, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_PANIC, HI_APPANIC_SOFTLOCKUP, (u32)RDR_UPLOAD_YES, "ap", "ap",
	 0, 0, 0},
	{{0, 0}, MODID_AP_S_PANIC_OTHERCPU_HARDLOCKUP, MODID_AP_S_PANIC_OTHERCPU_HARDLOCKUP, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_PANIC, HI_APPANIC_OHARDLOCKUP, (u32)RDR_UPLOAD_YES, "ap", "ap",
	 0, 0, 0},
	{{0, 0}, MODID_AP_S_PANIC_SP805_HARDLOCKUP, MODID_AP_S_PANIC_SP805_HARDLOCKUP, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_PANIC, HI_APPANIC_HARDLOCKUP, (u32)RDR_UPLOAD_YES, "ap", "ap",
	 0, 0, 0},
	{{0, 0}, MODID_AP_S_NOC, MODID_AP_S_NOC, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_NOC, 0, (u32)RDR_UPLOAD_YES, "ap", "ap",
	 0, 0, 0},
	{{0, 0}, MODID_AP_S_DDRC_SEC, MODID_AP_S_DDRC_SEC, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, 0,(u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0},
	{{0, 0}, MODID_AP_S_COMBINATIONKEY, MODID_AP_S_COMBINATIONKEY,
	 RDR_ERR, RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_COMBINATIONKEY, 0, (u32)RDR_UPLOAD_YES,
	 "ap", "ap", 0, 0, 0},
	{{0, 0}, MODID_AP_S_MAILBOX, MODID_AP_S_MAILBOX, RDR_WARN,
	 RDR_REBOOT_NO, RDR_AP, 0, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_MAILBOX, 0, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0},
	{{0, 0}, MODID_AP_S_PMU, MODID_AP_S_PMU, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_PMU, 0, (u32)RDR_UPLOAD_YES, "ap pmu", "ap pmu",
	 0, 0, 0},
	 {{0, 0}, MODID_AP_S_SMPL, MODID_AP_S_SMPL, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_SMPL, 0, (u32)RDR_UPLOAD_YES, "ap smpl", "ap smpl",
	 0, 0, 0},
	 {{0, 0}, MODID_AP_S_SCHARGER, MODID_AP_S_SCHARGER, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_SCHARGER, 0, (u32)RDR_UPLOAD_YES, "ap scharger", "ap scharger",
	 0, 0, 0},
	{{0, 0}, MODID_AP_S_RESUME_SLOWY, MODID_AP_S_RESUME_SLOWY, RDR_WARN,
	RDR_REBOOT_NO, RDR_AP, 0, RDR_AP,
	(u32)RDR_REENTRANT_DISALLOW, AP_S_RESUME_SLOWY, 0, (u32)RDR_UPLOAD_YES,
	"ap resumeslowy", "ap resumeslowy", 0, 0, 0},
};

/* 以下是给AP的其他维测模块、IP使用的dump内存 */
static unsigned int g_dump_modu_mem_size_tbl[MODU_MAX] = {0};

static int acpu_panic_loop_notify(struct notifier_block *nb,
				  unsigned long event, void *buf);
static int rdr_hisiap_panic_notify(struct notifier_block *nb,
				   unsigned long event, void *buf);
static int rdr_hisiap_die_notify(struct notifier_block *nb,
				 unsigned long event, void *pReg);
static int save_exception_info(void *arg);
static void get_product_version_work_fn(struct work_struct *work);
static int rdr_copy_big_file_apend(char *dst, char *src);
static int rdr_copy_file_apend(char *dst, char *src);

static struct notifier_block acpu_panic_loop_block = {
	.notifier_call = acpu_panic_loop_notify,
	.priority = INT_MAX,
};

static struct notifier_block rdr_hisiap_panic_block = {
	.notifier_call = rdr_hisiap_panic_notify,
	.priority = INT_MIN,
};

static struct notifier_block rdr_hisiap_die_block = {
	.notifier_call = rdr_hisiap_die_notify,
	.priority = INT_MIN,
};

static struct notifier_block rdr_hisiap_powerkey_block = {
	.notifier_call = rdr_press_key_to_fastboot,
	.priority = INT_MIN,
};

static DECLARE_DELAYED_WORK(get_product_version_work,
			    get_product_version_work_fn);

extern struct cmdword reboot_reason_map[];
extern struct task_struct *g_last_task_ptr[NR_CPUS];

static char exception_buf[KSYM_SYMBOL_LEN] __attribute__((__section__(".data")));
static unsigned long exception_buf_len __attribute__((__section__(".data")));

/**
 * memcpy_rdr- Replacement for kernel memcpy function to solve KASAN problem in rdr stack dump flow.
 * @dest: Where to copy to
 * @src: Where to copy from
 * @count: The size of the area.
 *
 * You should not use this function to access IO space, use memcpy_toio()
 * or memcpy_fromio() instead.
 */

/*******************************************************************************
Function:         get_ap_trace_mem_size_from_dts
Description:      从dts中读取ap侧各轨迹的内存大小
Input:            NA
Output:           NA
Return:           0:读取成功, 非0:失败
********************************************************************************/
static int get_ap_trace_mem_size_from_dts(void)
{
	int ret;
	struct device_node *np;
	u32 i = 0;
	np = of_find_compatible_node(NULL, NULL,
				     "hisilicon,rdr_ap_adapter");
	if (!np) {
		BB_PRINT_ERR(
		       "[%s], cannot find rdr_ap_adapter node in dts!\n",
		       __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "ap_trace_irq_size",
				   &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], cannot find ap_trace_irq_size in dts!\n",
		       __func__);
		return ret;
	}

	ret = of_property_read_u32(np, "ap_trace_task_size",
				   &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], cannot find ap_trace_task_size in dts!\n",
		       __func__);
		return ret;
	}

	ret = of_property_read_u32(np, "ap_trace_cpu_idle_size",
				   &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], cannot find ap_trace_cpu_idle_size in dts!\n",
		       __func__);
		return ret;
	}

	ret = of_property_read_u32(np, "ap_trace_worker_size",
				   &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], cannot find ap_trace_worker_size in dts!\n",
		       __func__);
		return ret;
	}
	g_dump_buffer_size_tbl[i++] = 0;
	g_dump_buffer_size_tbl[i++] = 0;

	ret = of_property_read_u32(np, "ap_trace_time_size",
				   &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], cannot find ap_trace_time_size in dts!\n",
		       __func__);
		return ret;
	}

	ret = of_property_read_u32(np, "ap_trace_cpu_on_off_size",
				   &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], cannot find ap_trace_cpu_on_off_size in dts!\n",
		       __func__);
		return ret;
	}

	ret = of_property_read_u32(np, "ap_trace_syscall_size",
				   &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], cannot find ap_trace_syscall_size in dts!\n",
		       __func__);
		return ret;
	}

	ret = of_property_read_u32(np, "ap_trace_hung_task_size",
				   &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], cannot find ap_trace_hung_task_size in dts!\n",
		       __func__);
		return ret;
	}

	ret = of_property_read_u32(np, "ap_trace_tasklet_size",
				   &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], cannot find ap_trace_tasklet_size in dts!\n",
		       __func__);
		return ret;
	}

	ret = of_property_read_u32(np, "ap_trace_diaginfo_size",
				   &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], cannot find ap_trace_diaginfo_size in dts!\n",
		       __func__);
		return ret;
	}

	return ret;
}

/*******************************************************************************
Function:         get_ap_dump_mem_modu_size_from_dts
Description:      从dts中读取ap侧各模块的dump内存大小
Input:            NA
Output:           NA
Return:           0:读取成功, 非0:失败
********************************************************************************/
static int get_ap_dump_mem_modu_size_from_dts(void)
{
	int ret;
	struct device_node *np;
	u32 i = 0;
	np = of_find_compatible_node(NULL, NULL,
				     "hisilicon,rdr_ap_adapter");
	if (!np) {
		BB_PRINT_ERR(
		       "[%s], cannot find rdr_ap_adapter node in dts!\n",
		       __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "ap_dump_mem_modu_noc_size",
				   &g_dump_modu_mem_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], cannot find ap_dump_mem_modu_noc_size in dts!\n",
		       __func__);
		return ret;
	}

	ret = of_property_read_u32(np, "ap_dump_mem_modu_ddr_size",
				   &g_dump_modu_mem_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], cannot find ap_dump_mem_modu_ddr_size in dts!\n",
		       __func__);
		return ret;
	}
	g_dump_modu_mem_size_tbl[i++] = 0;
	ret = of_property_read_u32(np, "ap_dump_mem_modu_gap_size",
				   &g_dump_modu_mem_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], cannot find ap_dump_mem_modu_gap_size in dts!\n",
		       __func__);
		return ret;
	}

	return ret;
}

/*******************************************************************************
Function:         ap_last_task_switch
Description:      从dts中读取ap_last_task内存分配的开关
Input:            NA
Output:           NA
Return:           0:读取失败或开关关闭, 非0:开关打开
********************************************************************************/
static unsigned int get_ap_last_task_switch_from_dts(void)
{
	int ret;
	struct device_node *np;
	unsigned int ap_last_task_switch;
	np = of_find_compatible_node(NULL, NULL,
				     "hisilicon,rdr_ap_adapter");
	if (!np) {
		BB_PRINT_ERR(
		       "[%s], cannot find rdr_ap_adapter node in dts!\n",
		       __func__);
		return 0;
	}

	ret = of_property_read_u32(np, "ap_last_task_switch",
				   &ap_last_task_switch);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], cannot find ap_last_task_switch in dts!\n",
		       __func__);
		return 0;
	}

	return ap_last_task_switch;
}

/*******************************************************************************
Function:       set_exception_info
Description:    设置调用__show_regs的异常的pc
Input:          buf:调用__show_regs的异常的pc的地址
Output:         NA
Return:         NA
********************************************************************************/
void set_exception_info(unsigned long address)
{
    memset(exception_buf, 0, sizeof(exception_buf));
    exception_buf_len = sprint_symbol(exception_buf, address);
}

/*******************************************************************************
Function:       get_exception_info
Description:    获取调用__show_regs的异常的pc
Input:          NA
Output:         buf:调用__show_regs的异常的pc的地址；buf_len:获取的buf的长度
Return:         NA
********************************************************************************/
void get_exception_info(unsigned long *buf, unsigned long *buf_len)
{
	if (unlikely(NULL == buf || NULL == buf_len)) {
		return;
	}

	*buf = (unsigned long)exception_buf;
	*buf_len = exception_buf_len;
}

/*version < 32byte*/
void get_product_version(char *version, size_t count)
{
	struct file *fp;
	char buf[BUFFER_SIZE];
	char *p;
	int i;
	ssize_t length;

	if (IS_ERR_OR_NULL(version)) {
		BB_PRINT_ERR("[%s], invalid para version [0x%pK]!\n",
		       __func__, version);
		return;
	}
	memset(version, 0, count);

	/* 等待文件系统ok，读取/system/build.prop的ro.confg.hw_systemversion=BalongV100R001C50B310属性 */
	/* 不能在module_init调用 */
	while (rdr_wait_partition("/data/lost+found", 1000) != 0)
		;


	fp = filp_open(SYSTEM_BUILD_POP, O_RDONLY, FILE_LIMIT);
	if (IS_ERR_OR_NULL(fp)) {
		BB_PRINT_ERR("[%s], open [%s] failed! err [%pK]\n",
		       __func__, SYSTEM_BUILD_POP, fp);
		return;
	}

	while ((length =
		vfs_read(fp, buf, BUFFER_SIZE, &fp->f_pos)) > 0) { /*lint !e613 */
		for (i = 0; i < BUFFER_SIZE; i++) {
			if ('\n' == buf[i])
				break;	/* 找到完整1行 */
		}
		memset(buf, '\0', BUFFER_SIZE);
		vfs_llseek(fp, -length, SEEK_CUR);
		if (BUFFER_SIZE == i) {	/* 1行超过128byte */
			i--;
		}
		if (0 != i) {
			vfs_read(fp, buf, i, &fp->f_pos); /*lint !e613 */
			p = strstr(buf, BUILD_DISPLAY_ID);
			if (NULL != p) {
				p = p + strlen(BUILD_DISPLAY_ID);
				if ('=' == *p) {
					p++;
					strncpy(version, p,
						strlen(p) >= count ? count - 1 : strlen(p));
					break;
				}
			}
			memset(buf, '\0', BUFFER_SIZE);
		} else {
			vfs_llseek(fp, 1, SEEK_CUR);	/* 去掉空行 */
		}
	}

	filp_close(fp, NULL); /*lint !e668 */
	BB_PRINT_PN("[%s], version [%s]!\n", __func__, version);
}

void print_debug_info(void)
{
	int i;
	regs_info *regs_info = g_rdr_ap_root->dump_regs_info; /*lint !e578*/

	pr_info("=================AP_EH_ROOT================");
	pr_info("[%s], dump_magic [0x%x]\n", __func__,
	       g_rdr_ap_root->dump_magic);
	pr_info("[%s], version [%s]\n", __func__,
	       g_rdr_ap_root->version);
	pr_info("[%s], modid [0x%x]\n", __func__,
	       g_rdr_ap_root->modid);
	pr_info("[%s], e_exce_type [0x%x],\n", __func__,
	       g_rdr_ap_root->e_exce_type);
	pr_info("[%s], e_exce_subtype [0x%x],\n", __func__,
	       g_rdr_ap_root->e_exce_subtype);
	pr_info("[%s], coreid [0x%llx]\n", __func__,
	       g_rdr_ap_root->coreid);
	pr_info("[%s], slice [%llu]\n", __func__,
	       g_rdr_ap_root->slice);
	pr_info("[%s], enter_times [0x%x]\n", __func__,
	       g_rdr_ap_root->enter_times);
	pr_info("[%s], num_reg_regions [0x%x]\n", __func__,
	       g_rdr_ap_root->num_reg_regions);

	for (i = 0; (unsigned int)i < g_rdr_ap_root->num_reg_regions; i++) {
		pr_info(
		       "[%s], reg_name [%s], reg_base [0x%pK], reg_size [0x%x], reg_dump_addr [0x%pK]\n",
		       __func__, regs_info[i].reg_name,
		       (void *)regs_info[i].reg_base,
		       regs_info[i].reg_size,
		       regs_info[i].reg_dump_addr);
	}
}

static int check_addr_overflow(unsigned char *addr)
{
	unsigned char *max_addr;

	max_addr = g_rdr_ap_root->rdr_ap_area_map_addr +
	    g_rdr_ap_root->ap_rdr_info.log_len - PMU_RESET_RECORD_DDR_AREA_SIZE;
	if ((addr < g_rdr_ap_root->rdr_ap_area_map_addr)
	    || (addr >= max_addr)) {
		return 1;
	}
	return 0;
}

/* 不包括AP_EH_ROOT所占1K空间 */
static unsigned char *get_rdr_hisiap_dump_start_addr(void)
{
	unsigned char *addr;
	unsigned int timers = sizeof(AP_EH_ROOT) / SIZE_1K + 1;

	addr = g_rdr_ap_root->rdr_ap_area_map_addr +
	    ALIGN(sizeof(AP_EH_ROOT), timers * SIZE_1K);
	pr_info(
	       "[%s], aligned by %dK, dump_start_addr [0x%pK]\n",
	       __func__, timers, addr);
	if (check_addr_overflow(addr)) {
		BB_PRINT_ERR(
		       "[%s], there is no space left for ap to dump!\n",
		       __func__);
		return NULL;
	}
	return addr;
}

int io_resources_init(void)
{
	int i, ret;
	struct device_node *np = NULL;
	struct resource res;
	regs_info *regs = NULL;
	unsigned char *tmp = NULL;
	u32 data[4];

	regs = g_rdr_ap_root->dump_regs_info;
	memset((void *)regs, 0, REGS_DUMP_MAX_NUM * sizeof(regs_info));

	np = of_find_compatible_node(NULL, NULL,
				     "hisilicon,rdr_ap_adapter");
	if (!np) {
		BB_PRINT_ERR(
		       "[%s], cannot find rdr_ap_adapter node in dts!\n",
		       __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "reg-dump-regions",
				   &g_rdr_ap_root->num_reg_regions);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], cannot find reg-dump-regions in dts!\n",
		       __func__);
		goto timer_ioinit;
	}

	if (0 == g_rdr_ap_root->num_reg_regions) {
		BB_PRINT_ERR(
		       "[%s], reg-dump-regions in zero, so no reg resource to init.\n",
		       __func__);
		goto timer_ioinit;
	}

	for (i = 0; (unsigned int)i < g_rdr_ap_root->num_reg_regions; i++) {
		if (of_address_to_resource(np, i, &res)) {
			BB_PRINT_ERR(
			       "[%s], of_address_to_resource [%d] fail!\n",
			       __func__, i);
			goto timer_ioinit;
		}

		strncpy(regs[i].reg_name, res.name, REG_NAME_LEN - 1);
		regs[i].reg_name[REG_NAME_LEN - 1] = '\0';
		regs[i].reg_base = res.start;
		regs[i].reg_size = resource_size(&res);

		if ( 0 == regs[i].reg_size ) {
			BB_PRINT_ERR(
			       "[%s], [%s] registers size is 0, skip map!\n",
			       __func__, (regs[i].reg_name));
			goto reg_dump_addr_init;
		}
		regs[i].reg_map_addr = of_iomap(np, i);
		pr_info(
		       "[%s], regs_info[%d].reg_name[%s], reg_base[0x%pK], reg_size[0x%x], map_addr[0x%pK]\n",
		       __func__, i, regs[i].reg_name,
		       (void *)regs[i].reg_base, regs[i].reg_size,
		       regs[i].reg_map_addr);
		if (!regs[i].reg_map_addr) {
			BB_PRINT_ERR(
			       "[%s], unable to map [%s] registers\n",
			       __func__, (regs[i].reg_name));
			goto timer_ioinit;
		}
		pr_info("[%s], map [%s] registers ok\n",
		       __func__, (regs[i].reg_name));
reg_dump_addr_init:

		if (0 == i) {
			regs[i].reg_dump_addr =
			    get_rdr_hisiap_dump_start_addr();
			if (IS_ERR_OR_NULL(regs[i].reg_dump_addr)) {
				BB_PRINT_ERR(
				       "[%s], reg_dump_addr is invalid!\n",
				       __func__);
				goto timer_ioinit;
			}
		} else {
			regs[i].reg_dump_addr =
			    regs[i - 1].reg_dump_addr + regs[i - 1].reg_size;
		}
	}

	tmp = regs[i - 1].reg_dump_addr + regs[i - 1].reg_size;
	if (check_addr_overflow(tmp)) {
		BB_PRINT_ERR(
		       "[%s], there is no space left for ap to dump regs!\n",
		       __func__);
	}
timer_ioinit:

	/*io resource of 32k timer */
	ret = of_property_read_u32_array(np, "ldrx2dbg-abs-timer",
				       &data[0], 4);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], cannot find ldrx2dbg-abs-timer in dts!\n",
		       __func__);
		return ret;
	}

	sctrl_map_addr = ioremap(data[0], data[1]);
	if (NULL == sctrl_map_addr) {
		BB_PRINT_ERR(
		       "[%s], cannot find ldrx2dbg-abs-timer in dts!\n",
		       __func__);
		return -EFAULT;
	}
	g_32k_timer_l32bit_addr = sctrl_map_addr + data[2];
	g_32k_timer_h32bit_addr = sctrl_map_addr + data[3];

	return 0;
}

static unsigned int get_total_regdump_region_size(regs_info *regs_info)
{ /*lint !e578 */
	int i;
	u32 total = 0;

	if (!regs_info) {
		BB_PRINT_ERR("[%s],\n", __func__);
		return 0;
	}

	for (i = 0; (unsigned int)i < g_rdr_ap_root->num_reg_regions; i++) {
		total += regs_info[i].reg_size;
	}

	pr_info(
	       "[%s], num_reg_regions [%u], regdump size [0x%x]\n",
	       __func__, g_rdr_ap_root->num_reg_regions, total);
	return total;
}

/*中断、任务、cpuidle根据cpu频繁度划分区域，以及其他不区分cpu的轨迹区域初始化*/
int __init hook_buffer_alloc(void)
{
	int ret;

	pr_info("[%s], irq_buffer_init start!\n", __func__);
	ret = irq_buffer_init(&g_rdr_ap_root->hook_percpu_buffer[HK_IRQ],
			    g_rdr_ap_root->hook_buffer_addr[HK_IRQ],
			    g_dump_buffer_size_tbl[HK_IRQ]);
	if (ret) {
		BB_PRINT_ERR("[%s], irq_buffer_init failed!\n",
		       __func__);
		return ret;
	}

	pr_info("[%s], task_buffer_init start!\n", __func__);
	ret = task_buffer_init(&g_rdr_ap_root->hook_percpu_buffer[HK_TASK],
			     g_rdr_ap_root->hook_buffer_addr[HK_TASK],
			     g_dump_buffer_size_tbl[HK_TASK]);
	if (ret) {
		BB_PRINT_ERR("[%s], task_buffer_init failed!\n",
		       __func__);
		return ret;
	}

	pr_info("[%s], cpuidle_buffer_init start!\n",
	       __func__);
	ret = cpuidle_buffer_init(&g_rdr_ap_root->hook_percpu_buffer[HK_CPUIDLE],
				g_rdr_ap_root->hook_buffer_addr[HK_CPUIDLE],
				g_dump_buffer_size_tbl[HK_CPUIDLE]);
	if (ret) {
		BB_PRINT_ERR("[%s], cpuidle_buffer_init failed!\n",
		       __func__);
		return ret;
	}

	pr_info("[%s], worker_buffer_init start!\n", __func__);
	ret = worker_buffer_init(&g_rdr_ap_root->hook_percpu_buffer[HK_WORKER],
			       g_rdr_ap_root->hook_buffer_addr[HK_WORKER],
			       g_dump_buffer_size_tbl[HK_WORKER]);
	if (ret) {
		BB_PRINT_ERR("[%s], worker_buffer_init failed!\n",
		       __func__);
		return ret;
	}

	pr_info("[%s], mem_alloc_buffer_init start!\n",
	       __func__);
	ret = mem_alloc_buffer_init(&g_rdr_ap_root->hook_percpu_buffer[HK_MEM_ALLOCATOR],
			       g_rdr_ap_root->hook_buffer_addr[HK_MEM_ALLOCATOR],
			       g_dump_buffer_size_tbl[HK_MEM_ALLOCATOR]);
	if (ret) {
		BB_PRINT_ERR("[%s], mem_alloc_buffer_init failed!\n",
		       __func__);
		return ret;
	}

	pr_info("[%s], ion_alloc_buffer_init start!\n",
	       __func__);
	ret = ion_alloc_buffer_init(&g_rdr_ap_root->hook_percpu_buffer[HK_ION_ALLOCATOR],
			       g_rdr_ap_root->hook_buffer_addr[HK_ION_ALLOCATOR],
			       g_dump_buffer_size_tbl[HK_ION_ALLOCATOR]);
	if (ret) {
		BB_PRINT_ERR("[%s], ion_alloc_buffer_init failed!\n",
		       __func__);
		return ret;
	}

	pr_info("[%s], worker_buffer_init start!\n", __func__);
	ret = time_buffer_init(&g_rdr_ap_root->hook_percpu_buffer[HK_TIME],
			       g_rdr_ap_root->hook_buffer_addr[HK_TIME],
			       g_dump_buffer_size_tbl[HK_TIME]);
	if (ret) {
		BB_PRINT_ERR("[%s], time_buffer_init failed!\n",
		       __func__);
		return ret;
	}

	pr_info("[%s], cpu_onoff_buffer_init start!\n",
	       __func__);
	ret = cpu_onoff_buffer_init(&g_rdr_ap_root->hook_buffer_addr[HK_CPU_ONOFF],
				  g_dump_buffer_size_tbl[HK_CPU_ONOFF]);
	if (ret) {
		BB_PRINT_ERR("[%s], cpu_onoff_buffer_init failed!\n",
		       __func__);
		return ret;
	}

	pr_info("[%s], syscall_buffer_init start!\n",
	       __func__);
	ret = syscall_buffer_init(&g_rdr_ap_root->hook_buffer_addr[HK_SYSCALL],
				g_dump_buffer_size_tbl[HK_SYSCALL]);
	if (ret) {
		BB_PRINT_ERR("[%s], syscall_buffer_init failed!\n",
		       __func__);
		return ret;
	}

	pr_info("[%s], hung_task_buffer_init start!\n",
	       __func__);
	ret = hung_task_buffer_init(&g_rdr_ap_root->hook_buffer_addr[HK_HUNGTASK],
				  g_dump_buffer_size_tbl[HK_HUNGTASK]);
	if (ret) {
		BB_PRINT_ERR("[%s], hung_task_buffer_init failed!\n",
		       __func__);
		return ret;
	}

	pr_info("[%s], tasklet_buffer_init start!\n",
	       __func__);
	ret = tasklet_buffer_init(&g_rdr_ap_root->hook_buffer_addr[HK_TASKLET],
				g_dump_buffer_size_tbl[HK_TASKLET]);
	if (ret) {
		BB_PRINT_ERR("[%s], tasklet_buffer_init failed!\n",
		       __func__);
		return ret;
	}

	pr_info("[%s], diaginfo_buffer_init start!\n",
	       __func__);
	ret = diaginfo_buffer_init(&g_rdr_ap_root->hook_buffer_addr[HK_DIAGINFO],
				g_dump_buffer_size_tbl[HK_DIAGINFO]);
	if (ret) {
		BB_PRINT_ERR("[%s], diaginfo_buffer_init failed!\n",
		       __func__);
		return ret;
	}

	return 0;
}

void rdr_hisiap_print_all_dump_addrs(void)
{
	int i;
	if (IS_ERR_OR_NULL(g_rdr_ap_root)) {
		BB_PRINT_ERR(
		       "[%s], g_rdr_ap_root [0x%pK] is invalid \n",
		       __func__, g_rdr_ap_root);
		return;
	}

	for (i = 0; (unsigned int)i < g_rdr_ap_root->num_reg_regions; i++) {
		pr_info(
		       "[%s], reg_name [%s], reg_dump_addr [0x%pK] \n",
		       __func__,
		       g_rdr_ap_root->dump_regs_info[i].reg_name,
		       g_rdr_ap_root->dump_regs_info[i].reg_dump_addr);
	}

	pr_info("[%s], rdr_ap_area_map_addr [0x%pK].\n",
	       __func__, g_rdr_ap_root->rdr_ap_area_map_addr);
	pr_info("[%s], kirq_switch_addr [0x%pK].\n", __func__,
	       g_rdr_ap_root->hook_buffer_addr[HK_IRQ]);
	pr_info("[%s], ktask_switch_addr [0x%pK].\n", __func__,
	       g_rdr_ap_root->hook_buffer_addr[HK_TASK]);
	pr_info("[%s], cpu_on_off_addr [0x%pK].\n", __func__,
	       g_rdr_ap_root->hook_buffer_addr[HK_CPU_ONOFF]);
	pr_info("[%s], cpu_idle_stat_addr [0x%pK].\n", __func__,
	       g_rdr_ap_root->hook_buffer_addr[HK_CPUIDLE]);
	pr_info("[%s], worker_trace_addr [0x%pK].\n", __func__,
	       g_rdr_ap_root->hook_buffer_addr[HK_WORKER]);
	pr_info("[%s], mem_allocator_trace_addr [0x%pK].\n", __func__,
	       g_rdr_ap_root->hook_buffer_addr[HK_MEM_ALLOCATOR]);
	pr_info("[%s], ion_allocator_trace_addr [0x%pK].\n", __func__,
	       g_rdr_ap_root->hook_buffer_addr[HK_ION_ALLOCATOR]);
	pr_info("[%s], time_trace_addr [0x%pK].\n", __func__,
	       g_rdr_ap_root->hook_buffer_addr[HK_TIME]);
	pr_info("[%s], syscall_trace_addr [0x%pK].\n", __func__,
	       g_rdr_ap_root->hook_buffer_addr[HK_SYSCALL]);
	pr_info("[%s], hung_task_addr [0x%pK].\n", __func__,
	       g_rdr_ap_root->hook_buffer_addr[HK_HUNGTASK]);
	pr_info("[%s], tasklet_trace_addr [0x%pK].\n", __func__,
	       g_rdr_ap_root->hook_buffer_addr[HK_TASKLET]);
	pr_info("[%s], diaginfo_trace_addr [0x%pK].\n", __func__,
	       g_rdr_ap_root->hook_buffer_addr[HK_DIAGINFO]);

	for (i = 0; i < NR_CPUS; i++) {
		pr_info(
		       "[%s], last_task_stack_dump_addr[%d] [0x%pK].\n",
		       __func__, i,
		       g_rdr_ap_root->last_task_stack_dump_addr[i]);
	}
	for (i = 0; i < NR_CPUS; i++) {
		pr_info(
		       "[%s], last_task_struct_dump_addr[%d] [0x%pK].\n",
		       __func__, i,
		       g_rdr_ap_root->last_task_struct_dump_addr[i]);
	}

	for (i = 0; i < MODU_MAX; i++) {
		if (0 != g_rdr_ap_root->module_dump_info[i].dump_size) {
			pr_info(
			       "[%s], module_dump_info[%d].dump_addr [0x%pK].\n",
			       __func__, i,
			       g_rdr_ap_root->module_dump_info[i].
			       dump_addr);
		}
	}
}

void module_dump_mem_init(void)
{
	int i;
	for (i = 0; i < MODU_MAX; i++) {
		if (0 == i) {
			g_rdr_ap_root->module_dump_info[0].dump_addr =
			    g_rdr_ap_root->last_task_stack_dump_addr[NR_CPUS - 1] +
			    last_task_stack_size;
		} else {
			g_rdr_ap_root->module_dump_info[i].dump_addr =
			    g_rdr_ap_root->module_dump_info[i - 1].dump_addr +
			    g_dump_modu_mem_size_tbl[i - 1];
		}

		if (check_addr_overflow
		    (g_rdr_ap_root->module_dump_info[i].dump_addr +
		     g_dump_modu_mem_size_tbl[i])) {
			BB_PRINT_ERR(
			       "[%s], there is no enough space for modu [%d] to dump mem!\n",
			       __func__, i);
			break;
		}
		g_rdr_ap_root->module_dump_info[i].dump_size =
		    g_dump_modu_mem_size_tbl[i];

		pr_info(
		       "[%s], dump_addr [0x%pK] dump_size [0x%x]!\n",
		       __func__,
		       g_rdr_ap_root->module_dump_info[i].dump_addr,
		       g_rdr_ap_root->module_dump_info[i].dump_size);
	}
	/*ap_last_task_switch关闭时，将last_task struct和stack的dump内存起始地址赋0*/
	if (!get_ap_last_task_switch_from_dts()) {
		for (i = 0; i < NR_CPUS; i++) {
			g_rdr_ap_root->last_task_struct_dump_addr[i] = 0;
			g_rdr_ap_root->last_task_stack_dump_addr[i] = 0;
		}
	}
}

static int __init ap_dump_buffer_init(void)
{
	int i;

	/* 轨迹记录区 */
	g_rdr_ap_root->hook_buffer_addr[0] =
	    get_rdr_hisiap_dump_start_addr()
	    + get_total_regdump_region_size(g_rdr_ap_root->dump_regs_info);
	for (i = 1; i < HK_MAX; i++) {
		g_rdr_ap_root->hook_buffer_addr[i] =
		    g_rdr_ap_root->hook_buffer_addr[i - 1] +
		    g_dump_buffer_size_tbl[i - 1];
	}

	/* 任务记录区 */
	if (get_ap_last_task_switch_from_dts()) {
		last_task_struct_size = sizeof(struct task_struct);
		last_task_stack_size = THREAD_SIZE;
	}

	g_rdr_ap_root->last_task_struct_dump_addr[0] =
	    g_rdr_ap_root->hook_buffer_addr[HK_MAX - 1]
	    + g_dump_buffer_size_tbl[HK_MAX - 1];
	for (i = 1; i < NR_CPUS; i++) {
		g_rdr_ap_root->last_task_struct_dump_addr[i] =
		    g_rdr_ap_root->last_task_struct_dump_addr[i - 1]
		    + last_task_struct_size;
	}

	g_rdr_ap_root->last_task_stack_dump_addr[0] = (unsigned char *)
	    ALIGN(((unsigned long)g_rdr_ap_root->last_task_struct_dump_addr[NR_CPUS - 1] + last_task_struct_size), SIZE_1K);	/* 按1K对齐 */
	for (i = 1; i < NR_CPUS; i++) {
		g_rdr_ap_root->last_task_stack_dump_addr[i] =
		    g_rdr_ap_root->last_task_stack_dump_addr[i - 1] +
		    last_task_stack_size;
	}

	if (check_addr_overflow(g_rdr_ap_root->last_task_stack_dump_addr[NR_CPUS - 1] +
	     last_task_stack_size)) {
		BB_PRINT_ERR(
		       "[%s], there is no enough space for ap to dump!\n",
		       __func__);
		return -ENOSPC;
	}
	pr_info("[%s], module_dump_mem_init start.\n",
	       __func__);
	module_dump_mem_init();

	rdr_hisiap_print_all_dump_addrs();
	return hook_buffer_alloc();
}

u64 get_32k_abs_timer_value(void)
{
	u64 timer_count = 0;
	if (!g_32k_timer_l32bit_addr || !g_32k_timer_h32bit_addr) {
		return 0;
	}
	timer_count = *(volatile unsigned int *)g_32k_timer_l32bit_addr;
	timer_count |=
	    ((u64) (*(volatile unsigned int *)g_32k_timer_h32bit_addr)) << 32;
	return timer_count;
}

void rdr_set_wdt_kick_slice(u64 kickslice)
{
	static u32 kicktimes;
	rdr_arctimer_t *rdr_arctimer = rdr_get_arctimer_record();

	rdr_arctimer_register_read(rdr_arctimer);

	if (g_rdr_ap_root != NULL) {
		g_rdr_ap_root->wdt_kick_slice[kicktimes % WDT_KICK_SLICE_TIMES] = kickslice;
		kicktimes++;
	}
}

u64 rdr_get_last_wdt_kick_slice(void)
{
	int i;
	u64 last_kick_slice = 0;

	if (WDT_KICK_SLICE_TIMES <= 0)
		return 0;

	if (NULL == g_rdr_ap_root)
		return 0;

	if (1 == WDT_KICK_SLICE_TIMES)
		return g_rdr_ap_root->wdt_kick_slice[0];

	last_kick_slice = g_rdr_ap_root->wdt_kick_slice[0];
	for (i = 1; i < WDT_KICK_SLICE_TIMES; i++) {
		last_kick_slice =
		    max(last_kick_slice, g_rdr_ap_root->wdt_kick_slice[i]); /*lint !e1058*/
	}

	return last_kick_slice;
}

void rdr_regs_memcpy(void *dest, const void *src, size_t len)
{
	size_t remain, mult, i;
	u64    *u64_dst, *u64_src;

	remain  = len % sizeof(u64);
	mult    = len / sizeof(u64);
	u64_dst = (u64 *)dest;
	u64_src = (u64 *)src;

	for (i = 0; i < mult; i++) {
		*(u64_dst++) = *(u64_src++);
	}

	for (i = 0; i < remain; i++) {
		*((u8 *)u64_dst + i) = *((u8 *)u64_src + i);
	}
}

void regs_dump(void)
{
	int i;
	regs_info *regs_info = NULL; /*lint !e578 */

	regs_info = g_rdr_ap_root->dump_regs_info;

	/* NOTE:sctrl在上电区, pctrl, pericrg在外设区,A核访问不要做domain域判断 */
	for (i = 0; (unsigned int)i < g_rdr_ap_root->num_reg_regions; i++) {
		if (IS_ERR_OR_NULL(regs_info[i].reg_map_addr)
		    || IS_ERR_OR_NULL(regs_info[i].reg_dump_addr)) {
			regs_info[i].reg_dump_addr = 0;
			BB_PRINT_ERR(
			       "[%s], regs_info[%d].reg_map_addr [%pK] reg_dump_addr [%pK] invalid!\n",
			       __func__, i, regs_info[i].reg_map_addr,
			       regs_info[i].reg_dump_addr);
			continue;
		}
		pr_info(
		       "[%s], memcpy [0x%x] size from regs_info[%d].reg_map_addr [%pK] to reg_dump_addr [%pK].\n",
		       __func__, regs_info[i].reg_size, i,
		       regs_info[i].reg_map_addr,
		       regs_info[i].reg_dump_addr);
		rdr_regs_memcpy(regs_info[i].reg_dump_addr,
		       regs_info[i].reg_map_addr,
		       regs_info[i].reg_size);
	}
}


static int hisi_trace_hook_install(void)
{
	int ret = 0;
	hook_type hk;

	for (hk = HK_IRQ; hk < HK_MAX; hk++) {
		ret = hisi_ap_hook_install(hk);
		if (ret) {
			BB_PRINT_ERR(
			       "[%s], hook_type [%d] install failed!\n",
			       __func__, hk);
			return ret;
		}
	}
	return ret;
}

static void hisi_trace_hook_uninstall(void)
{
	hook_type hk;
	for (hk = HK_IRQ; hk < HK_MAX; hk++) {
		hisi_ap_hook_uninstall(hk);
	}
}

static void get_product_version_work_fn(struct work_struct *work)
{
	BB_PRINT_PN("[%s], enter!\n", __func__);
	if (!g_rdr_ap_root) {
		BB_PRINT_ERR("[%s], g_rdr_ap_root is NULL!\n",
		       __func__);
		return;
	}

	get_product_version((char *)g_rdr_ap_root->version,
			    PRODUCT_VERSION_LEN);
	BB_PRINT_PN("[%s], exit!\n", __func__);
}

/*******************************************************************************
Function:       register_module_dump_mem_func
Description:    向AP维测模块及IP提供的内存dump注册接口；
Input:          func:注册的dump函数, module_name:模块名, modu:模块ID，此为统一分配;
Output:         NA
Return:         0:注册成功，<0:失败
********************************************************************************/
int register_module_dump_mem_func(rdr_hisiap_dump_func_ptr func,
				  char *module_name,
				  dump_mem_module modu)
{
	int ret = -1;

	if (modu >= MODU_MAX) {
		BB_PRINT_ERR("[%s], modu [%d] is invalid!\n",
		       __func__, modu);
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(func)) {
		BB_PRINT_ERR("[%s], func [0x%pK] is invalid!\n",
		       __func__, func);
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(module_name)) {
		BB_PRINT_ERR("[%s], module_name is invalid!\n",
		       __func__);
		return -EINVAL;
	}

	if (NULL == g_rdr_ap_root) {
		BB_PRINT_ERR("[%s], g_rdr_ap_root is null!\n",
		       __func__);
		return -1;
	}

	BB_PRINT_PN("[%s], module_name [%s]\n", __func__,
	       module_name);
	mutex_lock(&dump_mem_mutex);
	if (0 != g_rdr_ap_root->module_dump_info[modu].dump_size) {
		g_rdr_ap_root->module_dump_info[modu].dump_funcptr = func;
		strncpy(g_rdr_ap_root->module_dump_info[modu].module_name,
			module_name, AMNTN_MODULE_NAME_LEN - 1);
		g_rdr_ap_root->module_dump_info[modu].module_name[AMNTN_MODULE_NAME_LEN - 1] = '\0';
		ret = 0;
	}
	mutex_unlock(&dump_mem_mutex);

	if (ret) {
		BB_PRINT_ERR(
		       "[%s], func[0x%pK], size[%d], [%s] register failed!\n",
		       __func__, func,
		       g_rdr_ap_root->module_dump_info[modu].dump_size,
		       module_name);
	}
	return ret;
}

/*******************************************************************************
Function:       save_module_dump_mem
Description:    异常复位前，调用AP维测模块及IP提供的内存dump注册函数；
Input:          NA
Output:         NA
Return:         NA
********************************************************************************/
void save_module_dump_mem(void)
{
	int i;
	void *addr = NULL;
	unsigned int size = 0;

	BB_PRINT_PN("[%s], enter.\n", __func__);
	for (i = 0; i < MODU_MAX; i++) {
		if (NULL != g_rdr_ap_root->module_dump_info[i].dump_funcptr) {
			addr = (void *)g_rdr_ap_root->module_dump_info[i].dump_addr;
			size = g_rdr_ap_root->module_dump_info[i].dump_size;
			if (!g_rdr_ap_root->module_dump_info[i].dump_funcptr(addr, size)) {
				BB_PRINT_ERR(
				       "[%s], [%s] dump failed!\n",
				       __func__,
				       g_rdr_ap_root->module_dump_info[i].module_name);
				continue;
			}
		}
	}
	BB_PRINT_PN("[%s], exit.\n", __func__);
}

/*****************************************************
Description:   get device id. ex: hi3650, hi6250...
*****************************************************/
static void get_device_platform(unsigned char *device_platform, size_t count)
{
	const char *tmp_platform;

	memset(device_platform, '\0', count);

	tmp_platform = of_flat_dt_get_machine_name();
	if (tmp_platform) {
		strncpy((char *)device_platform, tmp_platform, count);
	}
	else {
		strncpy((char *)device_platform, "unknown", count);
		BB_PRINT_ERR("unrecognizable device_id? new or old product.\n");
	}
	device_platform[count - 1] = '\0';
}

int __init rdr_hisiap_dump_init(struct rdr_register_module_result *retinfo)
{
	int ret = 0;

	BB_PRINT_PN("[%s], begin.\n", __func__);

	g_rdr_ap_root = (AP_EH_ROOT *) g_hisiap_addr;
	strncpy(g_log_path, g_rdr_ap_root->log_path, LOG_PATH_LEN - 1);
	g_log_path[LOG_PATH_LEN - 1] = '\0';

	/* 由于pmu扣板还没有，暂时用ap异常区的后8个字节替代 */
	memset((void *)g_hisiap_addr, 0,
	       retinfo->log_len - PMU_RESET_RECORD_DDR_AREA_SIZE);
	g_rdr_ap_root = (AP_EH_ROOT *) g_hisiap_addr;
	g_rdr_ap_root->ap_rdr_info.log_addr = retinfo->log_addr;
	g_rdr_ap_root->ap_rdr_info.log_len = retinfo->log_len;
	g_rdr_ap_root->ap_rdr_info.nve = retinfo->nve;
	g_rdr_ap_root->rdr_ap_area_map_addr = (void *)g_hisiap_addr;
	get_device_platform(g_rdr_ap_root->device_id, PRODUCT_DEVICE_LEN);
	g_rdr_ap_root->bbox_version = BBOX_VERSION;
	g_rdr_ap_root->dump_magic = AP_DUMP_MAGIC;
	g_rdr_ap_root->end_magic = AP_DUMP_END_MAGIC;

	BB_PRINT_PN("[%s], io_resources_init start.\n", __func__);
	ret = io_resources_init();
	if (ret) {
		BB_PRINT_ERR("[%s], io_resources_init failed!\n",
		       __func__);
		return ret;
	}

	BB_PRINT_PN("[%s], get_ap_trace_mem_size_from_dts start.\n", __func__);
	ret = get_ap_trace_mem_size_from_dts();
	if (ret) {
		BB_PRINT_ERR("[%s], get_ap_trace_mem_size_from_dts failed!\n",
		       __func__);
		return ret;
	}

	BB_PRINT_PN("[%s], get_ap_dump_mem_modu_size_from_dts start.\n", __func__);
	ret = get_ap_dump_mem_modu_size_from_dts();
	if (ret) {
		BB_PRINT_ERR("[%s], get_ap_dump_mem_modu_size_from_dts failed!\n",
		       __func__);
		return ret;
	}

	BB_PRINT_PN("[%s], ap_dump_buffer_init start.\n", __func__);
	ret = ap_dump_buffer_init();
	if (ret) {
		BB_PRINT_ERR("[%s], ap_dump_buffer_init failed!\n",
		       __func__);
		return ret;
	}

	BB_PRINT_PN("[%s], register_arch_timer_func_ptr start.\n",
	       __func__);
	ret = register_arch_timer_func_ptr(get_32k_abs_timer_value);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], register_arch_timer_func_ptr failed!\n",
		       __func__);
		return ret;
	}

	if (check_himntn(HIMNTN_KERNEL_DUMP_ENABLE)) {	/* 轨迹是否生成保持和kernel dump一致 */
		BB_PRINT_PN(
			"[%s], hisi_trace_hook_install start.\n",
			__func__);
		ret = hisi_trace_hook_install();
		if (ret) {
			BB_PRINT_ERR(
				"[%s], hisi_trace_hook_install failed!\n",
				__func__);
			return ret;
		}
	} else {
		BB_PRINT_PN(
			"[%s], hisi_ap_defopen_hook_install start.\n",
			__func__);
		hisi_ap_defopen_hook_install();
	}

	schedule_delayed_work(&get_product_version_work, 0);

	BB_PRINT_PN("[%s], end.\n", __func__);
	return ret;
}


static void save_bl31_exc_memory(void)
{
	int iret = 0;
	int len = 0;
	char fullpath_arr[LOG_PATH_LEN] = "";
	char dst_str[LOG_PATH_LEN] = "";

	memset(fullpath_arr, 0, LOG_PATH_LEN);
	strncat(fullpath_arr, g_log_path,
		(LOG_PATH_LEN - 1 - strlen(fullpath_arr)));
	strncat(fullpath_arr, "/ap_log/",
		(LOG_PATH_LEN - 1 - strlen(fullpath_arr)));
	BB_PRINT_PN("%s: %s\n", __func__, fullpath_arr);

	iret = mntn_filesys_create_dir(fullpath_arr, DIR_LIMIT);
	if (0 != iret) {
		BB_PRINT_ERR("%s: iret is [%d]\n", __func__, iret);
		return;
	}
	len = strlen(fullpath_arr);
	memcpy(dst_str, fullpath_arr, len + 1);
	strncat(&dst_str[len], "/bl31_mntn_memory.bin",
		(LOG_PATH_LEN - 1 - len));
	iret = rdr_copy_file_apend(dst_str, SRC_BL31_MNTN_MEMORY);
	if (iret) {
		BB_PRINT_ERR(
		       "[%s], save bl31_mntn_memory.bin error, ret = %d\n",
		       __func__, iret);
	}

	return;
}


static void save_lpmcu_exc_memory(void)
{
	int iret = 0;
	int len = 0;
	char fullpath_arr[LOG_PATH_LEN] = "";
	char dst_str[LOG_PATH_LEN] = "";

	memset(fullpath_arr, 0, LOG_PATH_LEN);
	strncat(fullpath_arr, g_log_path, ((LOG_PATH_LEN - 1) - strlen(fullpath_arr)));
	strncat(fullpath_arr, "/lpmcu_log/", ((LOG_PATH_LEN - 1) - strlen(fullpath_arr)));
	BB_PRINT_PN("%s: %s\n", __func__, fullpath_arr);

	iret = mntn_filesys_create_dir(fullpath_arr, DIR_LIMIT);
	if (0 != iret) {
		BB_PRINT_ERR("%s: iret is [%d]\n", __func__, iret);
		return;
	}
	len = strlen(fullpath_arr);
	memcpy(dst_str, fullpath_arr, len + 1);
	memcpy(&dst_str[len], "/lpmcu_ddr_memory.bin",
	       strlen("/lpmcu_ddr_memory.bin") + 1);
	iret = rdr_copy_file_apend(dst_str, SRC_LPMCU_DDR_MEMORY);
	if (iret) {
		BB_PRINT_ERR(
		       "[%s], save lpmcu_ddr_memory.bin error, ret = %d\n",
		       __func__, iret);
	}
	return;
}

static void save_kernel_dump(void *arg)
{
	int fd;
	int ret;
	int len;
	char date[DATATIME_MAXLEN];
	char dst_str[LOG_PATH_LEN] = "";
	char exce_dir[LOG_PATH_LEN] = "";

	snprintf(exce_dir, LOG_PATH_LEN, "%s%s/", PATH_ROOT,
		 PATH_MEMDUMP);

	mntn_rm_old_log(exce_dir, 1);
	memset(dst_str, 0, LOG_PATH_LEN);
	memset(date, 0, DATATIME_MAXLEN);

	BB_PRINT_PN("exce_dir is [%s]\n", exce_dir);

	/* 如果arg为真，表示是起线程保存kerneldump，此时要从内存中获取异常时间戳 */
	if (arg && (LOG_PATH_LEN - 1 >= strlen(g_log_path))) {
		len = strlen(g_log_path);
		memcpy(date, &g_log_path[len - DATATIME_MAXLEN],
		       DATATIME_MAXLEN - 1);
	} else {
		/* 如果arg为空，则认为是ap侧的异常，是复位之后保存的log，则获取当前时间即可 */
		snprintf(date, DATATIME_MAXLEN, "%s-%08lld",
			 rdr_get_timestamp(), rdr_get_tick());
	}

	fd = sys_open(exce_dir, O_DIRECTORY, 0);

	/* if dir is not exist,then create new dir */
	if (fd < 0) {
		fd = sys_mkdir(exce_dir, DIR_LIMIT);
		if (fd < 0) {
			BB_PRINT_ERR("%s %d\n", __func__, fd);
			goto out;
		}
	} else {
		sys_close(fd);
	}
	strncat(exce_dir, "/", ((LOG_PATH_LEN - 1) - strlen(exce_dir)));
	strncat(exce_dir, date, ((LOG_PATH_LEN - 1) - strlen(exce_dir)));
	fd = sys_mkdir(exce_dir, DIR_LIMIT);
	if (fd < 0) {
		BB_PRINT_ERR("%s %d\n", __func__, fd);
		goto out;
	}

	len = strlen(exce_dir);
	memcpy(dst_str, exce_dir, len + 1);
	memcpy(&dst_str[len], "/kerneldump_",
	       strlen("/kerneldump_") + 1);

	strncat(dst_str, date, ((LOG_PATH_LEN - 1) - strlen(dst_str)));
	BB_PRINT_PN("%s: %s\n", __func__, dst_str);

	if (check_himntn(HIMNTN_KERNEL_DUMP_ENABLE)) {
		/* On FPGA it will take half one hour to transfer kerneldump,
		it's too slowly and useless to transfer kerneldump.
		We just create the kerneldump file name. */ 
		if (FPGA == g_bbox_fpga_flag) {
			ret = rdr_copy_big_file_apend(dst_str, SRC_KERNELDUMP);
		} else {
			ret = rdr_copy_file_apend(dst_str, SRC_KERNELDUMP);
		}

		if (ret) {
			BB_PRINT_ERR(
			       "[%s], save kerneldump error, ret = %d\n",
			       __func__, ret);
			goto out;
		}
	}

out:
	return;
}

/*********************************************************************
Function:       read_dump_end
Description:    After cpoying kerneldump to filesystem, need to free memory.
				So we just  need to read SRC_DUMPEND.
Input:          NA
Output:         NA
Return:         NA
*********************************************************************/
void read_dump_end(void)
{
	int fd;
	long long cnt;
	char buf[SZ_4K / 4];

	fd = sys_open(SRC_DUMPEND, O_RDONLY, FILE_LIMIT);
	if (fd < 0) {
		BB_PRINT_ERR("[%s]: open %s failed, return [%d]\n", __func__,
				SRC_DUMPEND, fd);
		return;
	}

	cnt = sys_read(fd, buf, SZ_4K / 4);
	if (cnt < 0) {
		BB_PRINT_ERR("[%s]: read %s failed, return [%lld]\n",
				__func__, SRC_DUMPEND, cnt);
		goto out;
	}
out:
	sys_close(fd);
	return;
}

/*********************************************************************
Function:       save_mntndump_log
Description:    save the log from file_node
Input:          arg:if arg is null, the func is called hisiap_dump
Output:         NA
Return:         0:success;
*********************************************************************/
int save_mntndump_log(void *arg)
{
	int ret;
	struct kstat mem_stat;

	if (!check_himntn(HIMNTN_GOBAL_RESETLOG))
		return 0;

	while (rdr_wait_partition("/data/lost+found", 1000) != 0)
		;

	if (0 == vfs_stat(SRC_LPMCU_DDR_MEMORY, &mem_stat))
		save_lpmcu_exc_memory();



	if (0 == vfs_stat(SRC_BL31_MNTN_MEMORY, &mem_stat))
		save_bl31_exc_memory();

	if (0 == vfs_stat(SRC_KERNELDUMP, &mem_stat)) {
		save_kernel_dump(arg);
	}


	if (0 == vfs_stat(SRC_DUMPEND, &mem_stat)) {
		read_dump_end();
	}

	ret = save_exception_info(arg);
	if (ret) {
		BB_PRINT_ERR(
			"save_exception_info fail, ret=%d\n", ret);
	}

	return ret;
}

/*********************************************************************
Function:       save_hisiap_log
Description:    After phone reboots, save hisiap_log
Input:          log_path:path;modid:excep_id
Output:         NA
Return:         NA
*********************************************************************/
void save_hisiap_log(char *log_path, u32 modid)
{
	struct rdr_exception_info_s temp;
	int ret, path_root_len;
	bool is_save_done;

	temp.e_notify_core_mask = RDR_AP;
	temp.e_reset_core_mask = RDR_AP;
	temp.e_from_core = RDR_AP;
	temp.e_exce_type = rdr_get_reboot_type();
	temp.e_exce_subtype = rdr_get_exec_subtype_value();

	if (temp.e_exce_type == LPM3_S_EXCEPTION) {
		temp.e_from_core = RDR_LPM3;
	}

	if (temp.e_exce_type == MMC_S_EXCEPTION) {
		temp.e_from_core = RDR_EMMC;
	}

	path_root_len = strlen(PATH_ROOT);

	/* reboot reason ap wdt which is error reported, means other failure result into ap wdt */
	if (unlikely(0 == ap_awdt_analysis(&temp))) {
		BB_PRINT_ERR("[%s], ap_awdt_analysis correct reboot reason [%s]!\n",
			__func__, rdr_get_exception_type(temp.e_exce_type));
		incorrect_reboot_reason_analysis(log_path, &temp);
	}

	/*if last save not done, need to add "last_save_not_done" in history.log*/
	if (modid == BBOX_MODID_LAST_SAVE_NOT_DONE) {
		is_save_done = false;
	} else {
		is_save_done = true;
	}
	rdr_save_history_log(&temp, &log_path[path_root_len],
		is_save_done, get_last_boot_keypoint());

	ret = save_mntndump_log(NULL);
	if (ret) {
		BB_PRINT_ERR("save_mntndump_log fail, ret=%d", ret);
	}

	return;
}

void rdr_hisiap_dump_root_head(u32 modid, u32 etype, u64 coreid)
{
	if (!g_rdr_ap_root) {
		BB_PRINT_ERR("[%s], exit!\n", __func__);
		return;
	}

	g_rdr_ap_root->modid = modid;
	g_rdr_ap_root->e_exce_type = etype;
	g_rdr_ap_root->coreid = coreid;
	g_rdr_ap_root->slice = get_32k_abs_timer_value();
	g_rdr_ap_root->enter_times++;
	return;
}

void rdr_hisiap_dump(u32 modid, u32 etype,
	u64 coreid, char *log_path, pfn_cb_dump_done pfn_cb)
{
	unsigned long exception_info = 0;
	unsigned long exception_info_len = 0;

	BB_PRINT_PN("[%s], begin.\n", __func__);
	BB_PRINT_PN("modid is 0x%x, etype is 0x%x\n", modid, etype);

	if (!rdr_ap_init) {
		BB_PRINT_ERR("rdr_hisi_ap_adapter is not ready\n");
		if (pfn_cb) {
			pfn_cb(modid, coreid);
		}
		return;
	}

	if (modid == RDR_MODID_AP_ABNORMAL_REBOOT
	    || modid == BBOX_MODID_LAST_SAVE_NOT_DONE) {
		BB_PRINT_PN("RDR_MODID_AP_ABNORMAL_REBOOT\n");
		if (log_path && check_himntn(HIMNTN_GOBAL_RESETLOG)) {
			strncpy(g_log_path, log_path, LOG_PATH_LEN - 1);
			g_log_path[LOG_PATH_LEN - 1] = '\0';

			save_hisiap_log(log_path, modid);
		}

		if (pfn_cb) {
			pfn_cb(modid, coreid);
		}
		return;
	}

	console_loglevel = 7;

	/* 如果是panic，则需要将pc指针记录下来，并传递到fastboot  */
	if ((etype == AP_S_PANIC || etype == AP_S_VENDOR_PANIC) && g_bbox_ap_record_pc) {
		get_exception_info(&exception_info,
				   &exception_info_len);
		memset(g_bbox_ap_record_pc->exception_info, 0,
		       RECORD_PC_STR_MAX_LENGTH);
		memcpy(g_bbox_ap_record_pc->exception_info,
		       (char *)exception_info, exception_info_len);
		BB_PRINT_PN("exception_info is [%s],len is [%ld]\n",
		       (char *)exception_info, exception_info_len);
		g_bbox_ap_record_pc->exception_info_len = exception_info_len;
	}

	BB_PRINT_PN("rdr_hisiap_dump modid[%x],etype[%x],coreid[%llx], log_path[%s]\n",
	     modid, etype, coreid, log_path);
	BB_PRINT_PN("[%s], hisi_trace_hook_uninstall start!\n",
	       __func__);
	hisi_trace_hook_uninstall();

	if (!g_rdr_ap_root)
		goto out;

	g_rdr_ap_root->modid = modid;
	g_rdr_ap_root->e_exce_type = etype;
	g_rdr_ap_root->coreid = coreid;
	BB_PRINT_PN("rdr_hisiap_dump log_path_ptr [%pK]\n",
	       g_rdr_ap_root->log_path);
	if (log_path) {
		strncpy(g_rdr_ap_root->log_path, log_path,
			LOG_PATH_LEN - 1);
		g_rdr_ap_root->log_path[LOG_PATH_LEN - 1] = '\0';
	}

	g_rdr_ap_root->slice = get_32k_abs_timer_value();

	BB_PRINT_PN("[%s], regs_dump start!\n", __func__);
	regs_dump();

	BB_PRINT_PN("[%s], last_task_stack_dump start!\n",
	       __func__);
	last_task_stack_dump();

	g_rdr_ap_root->enter_times++;

	BB_PRINT_PN("[%s], save_module_dump_mem start!\n",
	       __func__);
	save_module_dump_mem();

	print_debug_info();

	show_irq_register();
out:
	BB_PRINT_PN("[%s], exit!\n", __func__);
	if (pfn_cb) {
		pfn_cb(modid, coreid);
	}
}


void hisiap_nmi_notify_lpm3(void)
{
	unsigned int value;
	u64 addr = 0;

	addr = g_mi_notify_lpm3_addr;
	if (!addr) {
		BB_PRINT_ERR("[%s]", __func__);
		return;
	}
	value = readl((char *)addr);
	value |= (0x1 << 2);
	writel(value, (char *)addr);
	value &= ~(0x1 << 2);
	writel(value, (char *)addr);

	return;
}

void get_bbox_curtime_slice(void)
{
	u64 curtime, curslice;

	curtime = hisi_getcurtime();
	curslice = get_32k_abs_timer_value();

	BB_PRINT_PN(
	       "printk_time is %llu, 32k_abs_timer_value is %llu.\n",
	       curtime, curslice);
}

void rdr_hisiap_reset(u32 modid, u32 etype, u64 coreid)
{
	u64 err1_status, err1_misc0;
	BB_PRINT_PN("%s start\n", __func__);
	get_bbox_curtime_slice();
	if (!in_atomic() && !irqs_disabled() && !in_irq()) {
		sys_sync();
	}

	blk_power_off_flush(0); /*Flush the storage device cache*/

	if (AP_S_PANIC != etype) {
		BB_PRINT_PN("etype is not panic\n");
		dump_stack();
		preempt_disable();
		smp_send_stop();
	}

	bbox_diaginfo_dump_lastmsg();
	//HIMNTN_PANIC_INTO_LOOP will disbale ap reset.
	if (check_himntn(HIMNTN_PANIC_INTO_LOOP) == 1) {
		do {
		} while (1);
	}
	BB_PRINT_PN("%s blk flush ok\n", __func__);
	flush_ftrace_buffer_cache();
	mntn_show_stack_cpustall();
	kmsg_dump(KMSG_DUMP_PANIC);
	flush_cache_all();

	l3cache_ecc_get_status(&err1_status, &err1_misc0, 1);
	hisiap_nmi_notify_lpm3();
	BB_PRINT_PN("%s end\n", __func__);

	while (1)
		;
}

int get_pmu_reset_base_addr(void)
{
	u64 pmu_reset_reg_addr = (u64)0;
	unsigned int fpga_flag = 0;
	struct device_node *np = NULL;
	int ret;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,hisifb");
	if (!np) {
		BB_PRINT_ERR("NOT FOUND device node 'hisilicon,hisifb'!\n");
		return -ENXIO;
	}
	ret = of_property_read_u32(np, "fpga_flag", &fpga_flag);
	if (ret) {
		BB_PRINT_ERR("failed to get fpga_flag resource.\n");
		return -ENXIO;
	}
	if (fpga_flag == FPGA) {
		g_bbox_fpga_flag = FPGA;
		pmu_reset_reg_addr = FPGA_RESET_REG_ADDR;
		g_pmu_reset_reg = (unsigned long long)
			hisi_bbox_map(pmu_reset_reg_addr, 0x4);
		if (!g_pmu_reset_reg) {
			BB_PRINT_ERR("get pmu reset reg error\n");
			return -1;
		}
		g_pmu_subtype_reg = (unsigned long long)
			hisi_bbox_map(FPGA_EXCSUBTYPE_REG_ADDR, 0x4);
		if (!g_pmu_subtype_reg) {
			BB_PRINT_ERR("get pmu subtype reg error\n");
			return -1;
		}
		BB_PRINT_DBG("pmu reset reg phy is 0x%llx\n", pmu_reset_reg_addr);
	}
	return 0;
}


unsigned long long get_pmu_reset_reg(void)
{
	return g_pmu_reset_reg;
}


unsigned long long get_pmu_subtype_reg(void)
{
	return g_pmu_subtype_reg;
}


void set_reboot_reason(unsigned int reboot_reason)
{
	unsigned int value = 0;
	unsigned long long pmu_reset_reg;

	if (FPGA == g_bbox_fpga_flag) {
		pmu_reset_reg = get_pmu_reset_reg();
		if (pmu_reset_reg) {
			value = readl((char *)pmu_reset_reg);
		}
	} else {
		value = hisi_pmic_reg_read(PMU_RESET_REG_OFFSET);/*lint !e747*/
	}
	value &= (PMU_RESET_REG_MASK);
	reboot_reason &= (RST_FLAG_MASK);
	value |= reboot_reason;
	if (FPGA == g_bbox_fpga_flag) {
		pmu_reset_reg = get_pmu_reset_reg();
		if (pmu_reset_reg) {
			writel(value, (char *)pmu_reset_reg);
		}
	} else {
		hisi_pmic_reg_write(PMU_RESET_REG_OFFSET, value);/*lint !e747*/
	}
	BB_PRINT_PN("[%s]:set reboot_reason is 0x%x\n", __func__, value);

}


unsigned int get_reboot_reason(void)
{
	unsigned int value = 0;
	unsigned long long pmu_reset_reg;

	if (FPGA == g_bbox_fpga_flag) {
		pmu_reset_reg = get_pmu_reset_reg();
		if (pmu_reset_reg) {
			value = readl((char *)pmu_reset_reg);
		}
	} else {
		value = hisi_pmic_reg_read(PMU_RESET_REG_OFFSET);/*lint !e747*/
	}
	value &= RST_FLAG_MASK;
	return value;

}

void record_exce_type(struct rdr_exception_info_s *e_info)
{
	if (!e_info) {
		BB_PRINT_ERR("einfo is null\n");
		return;
	}
	set_reboot_reason(e_info->e_exce_type);
	set_subtype_exception(e_info->e_exce_subtype, false);
}

void hisiap_callback(u32 argc, void *argv)
{
	int ret;

	if (check_himntn(HIMNTN_GOBAL_RESETLOG)) {
		ret = hisi_trace_hook_install();

		if (ret) {
			BB_PRINT_ERR("[%s]\n", __func__);
		}
	}
	return;
}


static void rdr_hisiap_register_exception(void)
{
	int i;
	u32 ret;
	for (i = 0;
	     (unsigned int)i < sizeof(einfo) / sizeof(struct rdr_exception_info_s);
	     i++) {
		BB_PRINT_DBG("register exception:%d", einfo[i].e_exce_type);
		einfo[i].e_callback = hisiap_callback;
		if (0 == i) {
			/* 注册AP core公共callback函数，其他core有通知ap core dump，则调用此callback函数，
			   RDR_COMMON_CALLBACK为公共callback标记，没有标记的为ap core私有callback函数 */
			einfo[i].e_callback = (rdr_e_callback) (
				(u64)(einfo[i].e_callback) | BBOX_COMMON_CALLBACK);
		}
		ret = rdr_register_exception(&einfo[i]);
		if (ret == 0) {
			BB_PRINT_ERR(
			       "rdr_register_exception fail, ret = [%d]\n",
			       ret);
		}
	}
	BB_PRINT_PN("[%s], end\n", __func__);
}


static int rdr_hisiap_register_core(void)
{
	struct rdr_module_ops_pub s_soc_ops;
	struct rdr_register_module_result retinfo;
	int ret = 0;
	u64 coreid = RDR_AP;

	s_soc_ops.ops_dump = rdr_hisiap_dump;
	s_soc_ops.ops_reset = rdr_hisiap_reset;

	ret = rdr_register_module_ops(coreid, &s_soc_ops, &retinfo);
	if (ret < 0) {
		BB_PRINT_ERR(
		       "rdr_register_module_ops fail, ret = [%d]\n",
		       ret);
		return ret;
	}

	ret = rdr_register_cleartext_ops(coreid, rdr_hisiap_cleartext_print);
	if (ret < 0) {
		BB_PRINT_ERR(
		       "rdr_register_cleartext_ops fail, ret = [%d]\n",
		       ret);
		return ret;
	}

	current_info.log_addr = retinfo.log_addr;
	current_info.log_len = retinfo.log_len;
	current_info.nve = retinfo.nve;

	return ret;
}


bool rdr_get_ap_init_done(void)
{
	return rdr_ap_init == 1;
}


static int rdr_copy_big_file_apend(char *dst, char *src)
{
	long fddst, fdsrc;
	int ret = 0;

	if (NULL == dst || NULL == src) {
		BB_PRINT_ERR("rdr:%s():dst(0x%pK) or src(0x%pK) is NULL.\n",
		       __func__, dst, src);
		return -1;
	}

	fdsrc = sys_open(src, O_RDONLY, FILE_LIMIT);
	if (fdsrc < 0) {
		BB_PRINT_ERR("rdr:%s():open %s failed, return [%ld]\n",
		       __func__, src, fdsrc);
		ret = -1;
		goto out;
	}
	fddst =
	    sys_open(dst, O_CREAT | O_WRONLY | O_APPEND, FILE_LIMIT);
	if (fddst < 0) {
		BB_PRINT_ERR("rdr:%s():open %s failed, return [%ld]\n",
		       __func__, dst, fddst);
		sys_close(fdsrc);
		ret = -1;
		goto out;
	}

	sys_close(fdsrc);
	sys_close(fddst);
out:
	return ret;
}


static int rdr_copy_file_apend(char *dst, char *src)
{
	long fddst, fdsrc;
	char buf[SZ_4K / 4];
	long cnt;
	int ret = 0;

	if (NULL == dst || NULL == src) {
		BB_PRINT_ERR("rdr:%s():dst(0x%pK) or src(0x%pK) is NULL.\n",
		       __func__, dst, src);
		return -1;
	}

	fdsrc = sys_open(src, O_RDONLY, FILE_LIMIT);
	if (fdsrc < 0) {
		BB_PRINT_ERR("rdr:%s():open %s failed, return [%ld]\n",
		       __func__, src, fdsrc);
		ret = -1;
		goto out;
	}
	fddst =
	    sys_open(dst, O_CREAT | O_WRONLY | O_APPEND, FILE_LIMIT);
	if (fddst < 0) {
		BB_PRINT_ERR("rdr:%s():open %s failed, return [%ld]\n",
		       __func__, dst, fddst);
		sys_close(fdsrc);
		ret = -1;
		goto out;
	}

	while (1) {
		cnt = sys_read(fdsrc, buf, SZ_4K / 4);
		if (cnt == 0)
			break;
		if (cnt < 0) {
			BB_PRINT_ERR
			    ("rdr:%s():read %s failed, return [%ld]\n",
			     __func__, src, cnt);
			ret = -1;
			goto close;
		}

		cnt = sys_write(fddst, buf, SZ_4K / 4);
		if (cnt <= 0) {
			BB_PRINT_ERR
			    ("rdr:%s():write %s failed, return [%ld]\n",
			     __func__, dst, cnt);
			ret = -1;
			goto close;
		}
	}

close:
	sys_close(fdsrc);
	sys_close(fddst);
out:
	return ret;
}

/*******************************************************************************
Function:       save_pstore_info
Description:    copy file from /sys/fs/pstore to dst_dir_str
Input:          dst_dir_str
Output:         NA
Return:         NA
********************************************************************************/
static void save_pstore_info(char *dst_dir_str)
{
	int i, ret, len, tmp_cnt;
	char *pbuff;
	char dst_str[NEXT_LOG_PATH_LEN];
	char fullpath_arr[LOG_PATH_LEN];

	hisi_create_pstore_entry();
	if (rdr_wait_partition(PSTORE_PATH, 60)) {
		BB_PRINT_ERR("pstore is not ready\n");
		return;
	}

	tmp_cnt =
	    MNTN_FILESYS_MAX_CYCLE * MNTN_FILESYS_PURE_DIR_NAME_LEN;

	/* 申请一块内存用于存储遍历/sys/fs/pstore/目录下所有文件的名字 */
	pbuff = kmalloc(tmp_cnt, GFP_KERNEL);
	if (NULL == pbuff) {
		BB_PRINT_ERR(
		       "kmalloc tmp_cnt fail, tmp_cnt = [%d]\n",
		       tmp_cnt);
		return;
	}

	/* 清空申请的buff */
	memset((void *)pbuff, 0, tmp_cnt);

	/* 调用接口将/sys/fs/pstore/目录下的所有文件名字存入pbuff中 */
	tmp_cnt =
	    mntn_filesys_dir_list(PSTORE_PATH, pbuff, tmp_cnt, DT_REG);

	/* 生成last_kmsg文件及dmesg-ramoops-x文件 */
	for (i = 0; i < tmp_cnt; i++) {
		/* 生成源文件绝对路径 */
		memset((void *)fullpath_arr, 0, sizeof(fullpath_arr));
		strncat(fullpath_arr, PSTORE_PATH, ((LOG_PATH_LEN - 1) - strlen(fullpath_arr)));
		len =
		    strlen(pbuff + ((unsigned long)i * MNTN_FILESYS_PURE_DIR_NAME_LEN));/*lint !e571*/
		BB_PRINT_PN("file is [%s]\n",
		       (pbuff + ((unsigned long)i * MNTN_FILESYS_PURE_DIR_NAME_LEN)));/*lint !e571*/
		strncat(fullpath_arr,
			(const char *)(pbuff + ((unsigned long)i * MNTN_FILESYS_PURE_DIR_NAME_LEN)),/*lint !e571*/
			((LOG_PATH_LEN - 1) - strlen(fullpath_arr)));

		/* 如果不是console则目的文件非last_kmsg，为dmesg-ramoops-x */
		if (0 != strncmp((const char *)(pbuff + ((unsigned long)i * MNTN_FILESYS_PURE_DIR_NAME_LEN)),/*lint !e571*/
				"console-ramoops",
				strlen("console-ramoops"))) {
			memset((void *)dst_str, 0, NEXT_LOG_PATH_LEN);
			strncat(dst_str, dst_dir_str,
				((NEXT_LOG_PATH_LEN - 1) - strlen(dst_str)));
			strncat(dst_str, "/", ((NEXT_LOG_PATH_LEN - 1) - strlen(dst_str)));
			strncat(dst_str,
				(const char *)(pbuff + ((unsigned long)i * MNTN_FILESYS_PURE_DIR_NAME_LEN)),/*lint !e571*/
				((NEXT_LOG_PATH_LEN - 1) - strlen(dst_str)));
		} else {
			memset((void *)dst_str, 0, NEXT_LOG_PATH_LEN);
			strncat(dst_str, dst_dir_str,
				((NEXT_LOG_PATH_LEN - 1) - strlen(dst_str)));
			strncat(dst_str, "/", ((NEXT_LOG_PATH_LEN - 1) - strlen(dst_str)));
			strncat(dst_str,
				"last_kmsg",
				((NEXT_LOG_PATH_LEN - 1) - strlen(dst_str)));
		}

		/* 将源文件的内容拷贝到目的文件中 */
		ret = rdr_copy_file_apend(dst_str, fullpath_arr);
		if (ret) {
			BB_PRINT_ERR(
			       "rdr_copy_file_apend [%s] fail, ret = [%d]\n",
			       fullpath_arr, ret);
		}
	}

	hisi_remove_pstore_entry();
	kfree(pbuff);
	return;
}

/*******************************************************************************
Function:       save_fastboot_log
Description:    copy fastboot_log to dst_dir_str
Input:          dst_dir_str
Output:         NA
Return:         NA
********************************************************************************/
static void save_fastboot_log(char *dst_dir_str)
{
	int ret, len;
	char fastbootlog_path[NEXT_LOG_PATH_LEN];
	char last_fastbootlog_path[NEXT_LOG_PATH_LEN];

	/* 组合fastbootlog的文件的绝对路径 */
	memset(last_fastbootlog_path, 0, NEXT_LOG_PATH_LEN);
	memset(fastbootlog_path, 0, NEXT_LOG_PATH_LEN);
	len = strlen(dst_dir_str);
	memcpy(last_fastbootlog_path, dst_dir_str, len + 1);
	memcpy(&last_fastbootlog_path[len], "/last_fastboot_log",
	       strlen("/last_fastboot_log") + 1);
	memcpy(fastbootlog_path, dst_dir_str, len + 1);
	memcpy(&fastbootlog_path[len], "/fastboot_log",
	       strlen("/fastboot_log") + 1);

	/* 生成last_fastbootlog文件 */
	ret = rdr_copy_file_apend(last_fastbootlog_path,
				  LAST_FASTBOOT_LOG_FILE);
	if (ret) {
		BB_PRINT_ERR("rdr_copy_file_apend [%s] fail, ret = [%d]\n",
		       LAST_FASTBOOT_LOG_FILE, ret);
	}

	/* 生成curr_fastbootlog文件 */
	ret = rdr_copy_file_apend(fastbootlog_path, FASTBOOT_LOG_FILE);
	if (ret) {
		BB_PRINT_ERR("rdr_copy_file_apend [%s] fail, ret = [%d]\n",
		       FASTBOOT_LOG_FILE, ret);
	}

	return;
}

/*******************************************************************************
Function:       save_exception_info
Description:    保存异常信息
Input:          void *arg暂不使用
Output:         NA
Return:         -1 : error, 0 : ok
********************************************************************************/
int save_exception_info(void *arg)
{
	int fd, ret, len;
	char date[DATATIME_MAXLEN];
	char exce_dir[LOG_PATH_LEN];
	char dst_dir_str[DEST_LOG_PATH_LEN];
	char default_dir[LOG_PATH_LEN];

	BB_PRINT_PN("[%s], start\n", __func__);
	ret = 0;

	/* 从全局变量中获取此次异常的log目录路径 */
	memset(exce_dir, 0, LOG_PATH_LEN);
	if (LOG_PATH_LEN - 1 < strlen(g_log_path)) {
		BB_PRINT_ERR("g_log_path's len too large\n");
		return -1;
	}
	memcpy(exce_dir, g_log_path, strlen(g_log_path) + 1);
	BB_PRINT_PN("exce_dir is [%s]\n", exce_dir);

	/* 打开异常目录，如果不存在则以当前时间为目录创建 */
	fd = sys_open(exce_dir, O_DIRECTORY, 0);
	if (fd < 0) {
		BB_PRINT_ERR(
		       "sys_open exce_dir[%s] fail,fd = [%d]\n",
		       exce_dir, fd);
		memset(date, 0, DATATIME_MAXLEN);
		memset(default_dir, 0, LOG_PATH_LEN);
		snprintf(date, DATATIME_MAXLEN, "%s-%08lld",
			 rdr_get_timestamp(), rdr_get_tick());
		snprintf(default_dir, LOG_PATH_LEN, "%s%s", PATH_ROOT,
			 date);
		BB_PRINT_PN("default_dir is [%s]\n", default_dir);
		fd = sys_mkdir(default_dir, DIR_LIMIT);
		if (fd < 0) {
			BB_PRINT_ERR(
			       "sys_mkdir default_dir[%s] fail, fd = [%d]\n",
			       default_dir, fd);
			ret = -1;
			goto out;
		}
		memset(dst_dir_str, 0, DEST_LOG_PATH_LEN);
		len = strlen(default_dir);
		memcpy(dst_dir_str, default_dir, len + 1);
		memcpy(&dst_dir_str[len], "/ap_log",
		       strlen("/ap_log") + 1);
	} else {
		sys_close(fd);
		memset(dst_dir_str, 0, DEST_LOG_PATH_LEN);
		len = strlen(exce_dir);
		memcpy(dst_dir_str, exce_dir, len + 1);
		memcpy(&dst_dir_str[len], "/ap_log",
		       strlen("/ap_log") + 1);
	}

	/* 打开异常目录下的ap_log目录，如果不存在则创建 */
	fd = sys_open(dst_dir_str, O_DIRECTORY, 0);
	if (fd < 0) {
		fd = sys_mkdir(dst_dir_str, DIR_LIMIT);
		if (fd < 0) {
			BB_PRINT_ERR(
			       "sys_mkdir dst_dir_str[%s] fail, fd = [%d]\n",
			       dst_dir_str, fd);
			ret = -1;
			goto out;
		}
	} else {
		sys_close(fd);
	}

	save_fastboot_log(dst_dir_str);
	save_pstore_info(dst_dir_str);

	/* 如果函数入参是null，则表示是在rdr_hisiap_dump中调用的，后面操作不需要 */
	if (!arg) {
		goto out;
	}

	/* 在异常目录下面新建DONE文件，标志此次异常log保存完毕 */
	bbox_save_done(g_log_path, BBOX_SAVE_STEP_DONE);

	/* 文件系统sync，保证读写任务完成 */
	if (!in_atomic() && !irqs_disabled() && !in_irq()) {
		sys_sync();
	}

	/* 根据权限要求，hisi_logs目录及子目录群组调整为root-system */
	ret = (int)bbox_chown((const char __user *)g_log_path, ROOT_UID,
			      SYSTEM_GID, true);
	if (ret) {
		BB_PRINT_ERR(
		       "[%s], chown %s uid [%d] gid [%d] failed err [%d]!\n",
		       __func__, PATH_ROOT, ROOT_UID, SYSTEM_GID, ret);
	}

out:
	return ret;
}


int record_reason_task(void *arg)
{
	char date[DATATIME_MAXLEN];
	struct rdr_exception_info_s temp = {
		{0, 0}, 0x80000001, 0x80000001, RDR_ERR, RDR_REBOOT_NOW,
		(u64)RDR_AP, (u64)RDR_AP, (u64)RDR_AP,(u32)RDR_REENTRANT_DISALLOW,
		(u32)COLDBOOT, 0, (u32)RDR_UPLOAD_YES, "ap", "ap", 0, (void *)0, 0
	};
	temp.e_from_core = RDR_AP;
	temp.e_exce_type = rdr_get_reboot_type();

	while (rdr_wait_partition("/data/lost+found", 1000) != 0)
		;
	memset(date, 0, DATATIME_MAXLEN);
	snprintf(date, DATATIME_MAXLEN, "%s-%08lld",
		 rdr_get_timestamp(), rdr_get_tick());
	rdr_save_history_log(&temp, &date[0], true, get_last_boot_keypoint());
	return 0;
}


static int acpu_panic_loop_notify(struct notifier_block *nb,
				  unsigned long event, void *buf)
{
	if (check_himntn(HIMNTN_PANIC_INTO_LOOP) == 1) {
		do {
		} while (1);
	}

	return 0;
}


static int rdr_hisiap_panic_notify(struct notifier_block *nb,
				   unsigned long event, void *buf)
{
	BB_PRINT_PN("[%s], ===> enter panic notify!\n", __func__);

	if (watchdog_softlockup_happen()) {
		rdr_syserr_process_for_ap(MODID_AP_S_PANIC_SOFTLOCKUP, 0, 0);
	} else if (watchdog_othercpu_hardlockup_happen()) {
		rdr_syserr_process_for_ap(MODID_AP_S_PANIC_OTHERCPU_HARDLOCKUP, 0, 0);
	} else if (watchdog_sp805_hardlockup_happen()) {
		rdr_syserr_process_for_ap(MODID_AP_S_PANIC_SP805_HARDLOCKUP, 0, 0);
	} else {
		rdr_syserr_process_for_ap(MODID_AP_S_PANIC, 0, 0);
	}

	return 0;
}


static int rdr_hisiap_die_notify(struct notifier_block *nb,
				 unsigned long event, void *pReg)
{
	return 0;
}


int __init rdr_hisiap_init(void)
{
	struct task_struct *recordTask = NULL;
	u32 reboot_type = 0;
	u32 bootup_keypoint = 0;
	int ret = 0;
	BB_PRINT_PN("%s init start\n", __func__);

	mutex_init(&dump_mem_mutex);

	g_mi_notify_lpm3_addr = (u64)ioremap(NMI_NOTIFY_LPM3_ADDR, 0x4);
	if (!g_mi_notify_lpm3_addr) {
		BB_PRINT_ERR("[%s]", __func__);
		return -1;
	}

	rdr_hisiap_register_exception();
	ret = rdr_hisiap_register_core();
	if (ret) {
		BB_PRINT_ERR("%s rdr_hisiap_register_core fail, ret = [%d]\n",
		       __func__, ret);
		return ret;
	}

	ret = register_mntn_dump(MNTN_DUMP_PANIC, sizeof(AP_RECORD_PC) , (void **)&g_bbox_ap_record_pc);
	if (ret) {
		BB_PRINT_ERR(
		       "%s register g_bbox_ap_record_pc fail\n", __func__);
	}
	if (!g_bbox_ap_record_pc) {
		BB_PRINT_ERR(
		       "%s g_bbox_ap_record_pc is NULLl\n", __func__);
	}
	g_hisiap_addr =
	    (u64) hisi_bbox_map(current_info.log_addr +
				PMU_RESET_RECORD_DDR_AREA_SIZE,
				current_info.log_len - PMU_RESET_RECORD_DDR_AREA_SIZE);
	if (!g_hisiap_addr) {
		BB_PRINT_ERR("hisi_bbox_map g_hisiap_addr fail\n");
		return -1;
	}
	BB_PRINT_PN("[%s], retinfo: log_addr [0x%llx][0x%llx]",
	       __func__, current_info.log_addr, g_hisiap_addr);

	ret = rdr_hisiap_dump_init(&current_info);
	if (ret) {
		BB_PRINT_ERR("%s rdr_hisiap_dump_init fail, ret = [%d]\n",
		       __func__, ret);
		return -1;
	}
	reboot_type = rdr_get_reboot_type();
	bootup_keypoint = get_last_boot_keypoint();

	if (REBOOT_REASON_LABEL1 > reboot_type
		&& (!(AP_S_PRESS6S == reboot_type && STAGE_BOOTUP_END != bootup_keypoint))
		&& check_himntn(HIMNTN_GOBAL_RESETLOG)) {
		recordTask = kthread_run(record_reason_task, NULL, "recordTask");
		pr_info(
		       "%s: create record_reason_task, return %pK\n",
		       __func__, recordTask);
	}

	atomic_notifier_chain_register(&panic_notifier_list,
				       &acpu_panic_loop_block);
	atomic_notifier_chain_register(&panic_notifier_list,
				       &rdr_hisiap_panic_block);
	panic_on_oops = 1;
	register_die_notifier(&rdr_hisiap_die_block);
	hisi_powerkey_register_notifier(&rdr_hisiap_powerkey_block);
	get_bbox_curtime_slice();
	bbox_diaginfo_init();

	rdr_ap_init = 1;
	BB_PRINT_PN("%s init end\n", __func__);

	return 0;
}

static void __exit rdr_hisiap_exit(void)
{
	return;
}

module_init(rdr_hisiap_init);
module_exit(rdr_hisiap_exit);

MODULE_LICENSE("GPL");

