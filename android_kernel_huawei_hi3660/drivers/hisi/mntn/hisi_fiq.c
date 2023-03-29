#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/smp.h>
#include <linux/kmsg_dump.h>
#include <linux/blkdev.h>
#include <linux/io.h>
#include <linux/hisi/rdr_hisi_platform.h>


#include <asm/stacktrace.h>
#include <asm/exception.h>
#include <asm/system_misc.h>
#include <asm/cacheflush.h>

#include <linux/hisi/hisi_ddr.h>
#include <linux/hisi/mntn_record_sp.h>
#include <linux/hisi/eeye_ftrace_pub.h>
#include <linux/hisi/hisi_fiq.h>
#include <mntn_subtype_exception.h>
#include <linux/hisi/mntn_l3cache_ecc.h>
#include "bl31/hisi_bl31_exception.h"
#include "blackbox/rdr_inner.h"
#include "blackbox/rdr_field.h"
#include <linux/hisi/hisi_sp805_wdt.h>
#include <libhwsecurec/securec.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/version.h>
#include <linux/hisi/hisi_bbox_diaginfo.h>
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG HISI_FIQ_TAG
static u32 fiq_dump_flag;

static void sp805_wdt_reset(void)
{
	if (!wdt_base)
		return ;

	writel_relaxed(UNLOCK, wdt_base + WDTLOCK);
	writel_relaxed(WDT_DISABLE, wdt_base + WDTCONTROL);
	writel_relaxed(WDT_TIMEOUT_KICK, wdt_base + WDTLOAD);
	writel_relaxed(WDT_ENABlE, wdt_base + WDTCONTROL);
	writel_relaxed(LOCK, wdt_base+ WDTLOCK);

	readl_relaxed(wdt_base + WDTLOCK);
}


void hisi_mntn_inform(void)
{
	if (get_bl31_exception_flag() == BL31_PANIC_MAGIC)
		bl31_panic_ipi_handle();
	else {
		dmss_ipi_handler();
	}
}

#define ABNORMAL_RST_FLAG (0xFF)

asmlinkage void fiq_dump(struct pt_regs *regs, unsigned int esr)
{
	struct rdr_exception_info_s *p_exce_info;
	char date[DATATIME_MAXLEN];
	int ret = 0;
	u64 err1_status, err1_misc0;
	unsigned int reset_reason;

	fiq_dump_flag = 0xdeaddead;

	bust_spinlocks(1);
	flush_ftrace_buffer_cache();

	/* 系统异常触发复位动作，存在复位不成功的可能，这时只能靠狗复位，这样就会覆盖之前的异常记录 */
	reset_reason = get_reboot_reason();
	if ((ABNORMAL_RST_FLAG == reset_reason) || (AP_S_AWDT == reset_reason))
		set_subtype_exception(HI_APWDT_AP, true);


	pr_crit("fiq_dump begin!\n");
	pr_emerg("%s", linux_banner);

	dmss_fiq_handler();
	console_verbose();
	show_regs(regs);

	blk_power_off_flush(0);/*Flush the storage device cache*/

	dump_stack();
	smp_send_stop();

	if (!rdr_init_done()) {
		pr_crit("rdr init faild!\n");
		return;
	}

	last_task_stack_dump();
	regs_dump(); /*"sctrl", "pctrl", "peri_crg", "gic"*/

	save_module_dump_mem();

	sp805_wdt_dump();

	rdr_field_baseinfo_reinit();
	rdr_save_args(MODID_AP_S_WDT, 0, 0);
	p_exce_info = rdr_get_exception_info(MODID_AP_S_WDT);
	if (p_exce_info) {
		memset_s(date, DATATIME_MAXLEN, 0, DATATIME_MAXLEN);
		ret = snprintf_s(date, DATATIME_MAXLEN, DATATIME_MAXLEN - 1, "%s-%08lld",
			 rdr_get_timestamp(), rdr_get_tick());
		if(unlikely(ret < 0)){
			pr_crit("snprintf_s ret %d!\n", ret);
		}
		rdr_fill_edata(p_exce_info, date);

		(void)rdr_exception_trace_record(p_exce_info->e_reset_core_mask,
			p_exce_info->e_from_core, p_exce_info->e_exce_type, p_exce_info->e_exce_subtype);
	}

	rdr_hisiap_dump_root_head(MODID_AP_S_WDT, AP_S_AWDT, RDR_AP);
	bbox_diaginfo_dump_lastmsg();
	pr_crit("fiq_dump end\n");
	l3cache_ecc_get_status(&err1_status, &err1_misc0, 1);
	mntn_show_stack_cpustall();

	kmsg_dump(KMSG_DUMP_PANIC);
	flush_cache_all();
	sp805_wdt_reset();

	asm("b .");
}
