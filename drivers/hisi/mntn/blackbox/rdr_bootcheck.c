
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/hardirq.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/hisi/util.h>
#include <linux/mmc/core.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/syscalls.h>
#include <linux/hisi/hisi_bootup_keypoint.h>
#include <linux/hisi/hisi_bbox_diaginfo.h>
#include <libhwsecurec/securec.h>
#include <linux/hisi/hisi_pstore.h>
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG HISI_BLACKBOX_TAG

#include "platform_ap/rdr_hisi_ap_adapter.h"
#include "rdr_inner.h"
#include "rdr_field.h"
#include "rdr_print.h"
#include "rdr_debug.h"
#include <chipset_common/hwbfm/hw_boot_fail_core.h>

struct bootcheck {
	u64 mask;
	u64 core;
	u32 modid;
	u32 type;
	u32 subtype;
};

#define RDR_NEED_SAVE_MEM  1
#define RDR_DONTNEED_SAVE_MEM  0

static void rdr_check_log_save(u32 boot_type)
{

	if (need_save_mntndump_log(boot_type)) {
		if (save_mntndump_log("need save done.")) {
			BB_PRINT_ERR("save_mntndump_log fail\n");
		}
	}
}

/*
check status of last reboot.
return
0 dont need save.
1 need save log.
*/
static int rdr_check_exceptionboot(struct bootcheck *info)
{
	u32 reboot_type;/*lint !e578 */
	struct rdr_base_info_s *base;
	struct rdr_struct_s *tmpbb;

	if (NULL == info) {
		BB_PRINT_PN();
		return RDR_DONTNEED_SAVE_MEM;
	}

	reboot_type = rdr_get_reboot_type();
	/*如果异常类型为正常的启动，则不需要保存log */
	if (REBOOT_REASON_LABEL1 > reboot_type
	    || (REBOOT_REASON_LABEL4 <= reboot_type
	    && REBOOT_REASON_LABEL5 > reboot_type)) {
		BB_PRINT_PN();
		return RDR_DONTNEED_SAVE_MEM;
	}

	/*复位后保存log的默认值初始化 */
	info->modid = RDR_MODID_AP_ABNORMAL_REBOOT;
	info->mask = RDR_AP;
	info->core = RDR_AP;
	info->type = reboot_type;
	info->subtype = rdr_get_exec_subtype_value();

	/*如果异常类型为简易流程复位的，则都需要重启后保存log */
	if (REBOOT_REASON_LABEL1 <= reboot_type
	    && REBOOT_REASON_LABEL3 > reboot_type) {
		return RDR_NEED_SAVE_MEM;
	}

	/*获取bbox的内存地址 */
	tmpbb = rdr_get_tmppbb();
	if (NULL == tmpbb) {
		BB_PRINT_PN();
		return RDR_DONTNEED_SAVE_MEM;
	}

	/*获取bbox头结构体 */
	base = &(tmpbb->base_info);

	/*如果复位前log未保存完成，则复位启动后，需要再次保存一下 */
	if (RDR_PROC_EXEC_DONE != base->start_flag
	    || RDR_DUMP_LOG_DONE != base->savefile_flag) {
		BB_PRINT_ERR("ap error:start[%x],save done[%x]\n",
			     base->start_flag, base->savefile_flag);
		info->modid = BBOX_MODID_LAST_SAVE_NOT_DONE;
		return RDR_NEED_SAVE_MEM;
	}
	rdr_check_log_save(reboot_type);

	/* dump log is over but clear text is not, so need to save the clear text */
	if (RDR_CLEARTEXT_LOG_DONE != tmpbb->cleartext_info.savefile_flag) {
		rdr_exceptionboot_save_cleartext();
	}

	return RDR_DONTNEED_SAVE_MEM;
}

static inline void rdr_bootcheck_notify_dump(char *path, struct bootcheck *info)
{
	u64 result;

	BB_PRINT_PN("create dump file path:[%s].\n", path);
	while (info->mask) {
		if ((rdr_get_cur_regcore() & info->mask) == 0) {
			BB_PRINT_PN
			    ("wait module register. cur:[0x%llx],need[0x%llx]\n",
			     rdr_get_cur_regcore(), info->mask);
			msleep(1000);
			continue;
		}
		result = rdr_notify_onemodule_dump(info->modid, info->mask,
						info->type, info->core, path);
		BB_PRINT_PN("info.mask is [%llx], result = [0x%llx]\n", info->mask,
			     result);
		if (result) {
			info->mask &= (~result);
		} else {
			break;
		}
		BB_PRINT_PN("rdr: notify [%s] core dump data done.\n",
			     rdr_get_exception_core(result));
	}
}

static int rdr_save_history_log_back(void)
{
	struct rdr_exception_info_s temp;
	char date[DATATIME_MAXLEN];
	int ret;

	ret = snprintf_s(date, DATATIME_MAXLEN, DATATIME_MAXLEN-1, "%s-%08lld",
		  rdr_get_timestamp(), rdr_get_tick());
	if(unlikely(ret < 0)){
		BB_PRINT_ERR("[%s], snprintf_s date ret %d!\n", __func__, ret);
		return -1;
	}

	temp.e_from_core = RDR_AP;
	temp.e_reset_core_mask = RDR_AP;
	temp.e_exce_type = rdr_get_reboot_type();
	temp.e_exce_subtype = rdr_get_exec_subtype_value();

	/* reboot reason ap wdt which is error reported, means other failure result into ap wdt */
	if (unlikely(0 == ap_awdt_analysis(&temp))) {
		BB_PRINT_ERR("[%s], ap_awdt_analysis correct reboot reason [%s]!\n",
			__func__, rdr_get_exception_type(temp.e_exce_type));
	}

	if (temp.e_exce_type == LPM3_S_EXCEPTION) {
		temp.e_from_core = RDR_LPM3;
	}

	if (temp.e_exce_type == MMC_S_EXCEPTION) {
		temp.e_from_core = RDR_EMMC;
	}

	rdr_save_history_log(&temp, &date[0], false, get_last_boot_keypoint());

	return 0;
}

int rdr_bootcheck_thread_body(void *arg)
{
	int cur_reboot_times;
	int ret;
	char path[PATH_MAXLEN];
	struct bootcheck info;
	struct rdr_syserr_param_s p;
	struct rdr_struct_s *temp_pbb;
	int max_reboot_times = rdr_get_reboot_times();
	BB_PRINT_START();

	memset(path, 0, PATH_MAXLEN);

	save_hwbootfailInfo_to_file();

	BB_PRINT_PN("============wait for fs ready start =============\n");
	while (rdr_wait_partition("/data/lost+found", 1000) != 0)
		;
	BB_PRINT_PN("============wait for fs ready e n d =============\n");

	if (is_need_save_dfx2file()) {
		save_dfxpartition_to_file();
	}

	if (RDR_NEED_SAVE_MEM != rdr_check_exceptionboot(&info)) {
		BB_PRINT_PN("need not save dump file when boot.\n");
		goto end;
	}

	temp_pbb = rdr_get_tmppbb();
	if (RDR_UNEXPECTED_REBOOT_MARK_ADDR == temp_pbb->top_head.reserve) {
		cur_reboot_times = rdr_record_reboot_times2file();
		BB_PRINT_PN("ap has reboot %d times\n", cur_reboot_times);
		if (max_reboot_times < cur_reboot_times) {

			/*reset the file of reboot_times*/
			rdr_reset_reboot_times();

		}
	} else {
		rdr_reset_reboot_times();
	}
	p.modid = info.modid, p.arg1 = info.core, p.arg2 = info.type;

	if (!rdr_check_log_rights()) {
		ret = rdr_save_history_log_back();
		if (ret < 0) {
			goto end;
		}
		goto check_log;
	}

	ret = rdr_create_epath_bc(path);
	if (-1 == ret) {
		BB_PRINT_ERR("failed to create epath!\n");
		goto end;
	}
	rdr_set_saving_state(1);

	rdr_bootcheck_notify_dump(path, &info);

	if (check_himntn(HIMNTN_GOBAL_RESETLOG)) {
		rdr_save_last_baseinfo(path);
	}

	/* 在异常目录下面新建DONE文件，标志此次异常log保存完毕 */
	bbox_save_done(path, BBOX_SAVE_STEP_DONE);

	/* 文件系统sync，保证读写任务完成 */
	if (!in_atomic() && !irqs_disabled() && !in_irq()) {
		sys_sync();
	}

	rdr_set_saving_state(0);

	BB_PRINT_PN("saving data done.\n");
check_log:
	rdr_count_size();
	BB_PRINT_PN("rdr_count_size: done.\n");
end:

	hisi_free_persist_store();
	bbox_diaginfo_exception_save2fs();
	rdr_clear_tmppbb();

	BB_PRINT_END();
	return 0;
}
