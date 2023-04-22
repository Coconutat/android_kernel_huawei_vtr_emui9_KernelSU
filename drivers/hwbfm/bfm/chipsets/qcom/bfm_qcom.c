#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/stat.h>
#include <linux/spinlock.h>
#include <linux/notifier.h>
#include <linux/kprobes.h>
#include <linux/reboot.h>
#include <linux/io.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <asm/barrier.h>
#include <linux/platform_device.h>
#include <linux/of_fdt.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/of_address.h>
#include <linux/kallsyms.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/syscalls.h>
#include <soc/qcom/smem.h>
#include <chipset_common/bfmr/public/bfmr_public.h>
#include <chipset_common/bfmr/bfm/core/bfm_core.h>
#include <chipset_common/bfmr/bfm/chipsets/qcom/bfm_qcom.h>

struct boot_log_struct *boot_log = NULL;

#define OFFSET_TAG_SIZE 8
#define OFFSET_TAG "PAD0"

extern void msm_trigger_wdog_bark(void);

static u32 hwboot_calculate_checksum(unsigned char *addr, u32 len)
{
    int i;
    uint8_t *w = addr;
    u32 sum = 0;

    for(i=0;i<len;i++)
    {
        sum += *w++;
    }

    return sum;
}


void qcom_set_boot_stage(bfmr_detail_boot_stage_e stage)
{
    if(NULL != boot_log)
    {
        boot_log->boot_stage = stage;
        pr_info("%s: boot_stage = 0x%08x\n", __func__, boot_log->boot_stage);
    }
       return ;
}
EXPORT_SYMBOL(qcom_set_boot_stage);

u32 qcom_get_boot_stage(void)
{
    if(NULL != boot_log)
    {
        pr_info(" boot_stage = 0x%08x\n", boot_log->boot_stage);
        return boot_log->boot_stage;
    }
    pr_err("get boot stage fail\n");
    return 0;
}
EXPORT_SYMBOL(qcom_get_boot_stage);

static unsigned long long qcom_get_system_time(void)
{
    struct timeval tv = {0};

    do_gettimeofday(&tv);

    return (unsigned long long)tv.tv_sec;
}

int qcom_set_boot_fail_flag(bfmr_bootfail_errno_e bootfail_errno)
{
    int ret = -1;
    if(NULL != boot_log)
    {
        if(!boot_log->boot_error_no)
        {
            boot_log->boot_error_no = bootfail_errno;
            boot_log->rcv_method = BFM_CAN_NOT_CALL_TRY_TO_RECOVERY;
            boot_log->rtc_time = 0;
            boot_log->isUserPerceptiable = 1;
            ret = 0;
        }
    }
    return ret;
}
EXPORT_SYMBOL(qcom_set_boot_fail_flag);

static struct bootlog_inject_struct *bootlog_inject =NULL;
void  hwboot_fail_init_struct(void)
{
    u64 *fseq,*nseq;
    u32 *fidx;
    void * boot_log_virt = ioremap_nocache(HWBOOT_LOG_INFO_START_BASE,HWBOOT_LOG_INFO_SIZE);
    boot_log = (struct boot_log_struct *)(boot_log_virt);
    bootlog_inject = (struct bootlog_inject_struct *)(boot_log_virt + sizeof(struct boot_log_struct));
    if(NULL != boot_log){
        boot_log->boot_stage = KERNEL_STAGE_START;
        pr_notice("hwboot:boot_log=%p\n", boot_log);
        pr_notice("hwboot:boot_log->boot_magic=%x\n", boot_log->boot_magic);
        pr_notice("hwboot:boot_log->boot_stage=%x\n", boot_log->boot_stage);
        pr_notice("hwboot:boot_log->boot_error_no=%x\n", boot_log->boot_error_no);
    }else {
        pr_notice("hwboot: bootlog is null\n");
    }
    return;
}
EXPORT_SYMBOL(hwboot_fail_init_struct);

bool check_bootfail_inject(u32 err_code)
{
    if (bootlog_inject->flag == HWBOOT_FAIL_INJECT_MAGIC) {
        if(err_code == bootlog_inject->inject_boot_fail_no) {
            bootlog_inject->flag = 0;
            bootlog_inject->inject_boot_fail_no = 0;
            return true;
        }
    }
    return false;
}
EXPORT_SYMBOL(check_bootfail_inject);

void hwboot_clear_magic(void)
{
    boot_log->boot_magic = 0;
    return;
}
EXPORT_SYMBOL(hwboot_clear_magic);

static unsigned long long bfm_get_rtcoffset_from_smem(void)
{
    smem_exten_huawei_paramater *smem = NULL;
    unsigned long long rtc_time_offset = 0;
    char off_tag[OFFSET_TAG_SIZE];
    smem = smem_alloc(SMEM_ID_VENDOR1, sizeof(smem_exten_huawei_paramater),
                        0,
                        SMEM_ANY_HOST_FLAG);
    if (NULL == smem)
    {
        pr_err("%s: SMEM Error, READING RTC OFFSET INFO \n", __func__);
        return 0;
    }
    memset(off_tag, 0, OFFSET_TAG_SIZE);
    memcpy(off_tag, (smem->rtc_offset_info), OFFSET_TAG_SIZE);
    if(!strcmp(off_tag,"PAD0"))
    {
        memcpy(&rtc_time_offset, (smem->rtc_offset_info+OFFSET_TAG_SIZE), OFFSET_TAG_SIZE);
        pr_err("%s:rtc_time_offset = %llu\n",__func__,rtc_time_offset);
    }
    else
    {
        pr_err("%s: SMEM Error, Rtc_Offset_Info is not set \n", __func__);
    }

    return rtc_time_offset;

}


unsigned long long bfm_hctosys(unsigned long long current_secs)
{
    unsigned long long rtc_offset = 0;
    unsigned long long time_1980 = 0;
    unsigned long long system_time = 0;

    pr_err("%s:current_secs = %llu\n",__func__,current_secs);
    time_1980 = mktime(1980, 1, 1, 0, 0, 0);

    /* if the current_secs is smaller than time_1980, we need to modify the time. */
    if(current_secs < time_1980)
    {
        system_time = qcom_get_system_time();
        if(system_time > time_1980) 
        {
            return system_time;
        }

        rtc_offset = bfm_get_rtcoffset_from_smem();
        if(rtc_offset > time_1980)
        {
            return (current_secs + rtc_offset);
        }
    }

    return current_secs;

}

int qcom_hwboot_fail_init(void)
{
    pr_err("%s start\n",__func__);

    if(check_bootfail_inject(KERNEL_AP_PANIC))
    {
        panic("hwboot: inject KERNEL_AP_PANIC");
    }
    if(check_bootfail_inject(KERNEL_AP_WDT))
    {
        msm_trigger_wdog_bark();
    }
    if(check_bootfail_inject(KERNEL_BOOT_TIMEOUT))
    {
        boot_fail_err(KERNEL_BOOT_TIMEOUT, NO_SUGGESTION, NULL);
    }

    pr_err("%s end\n",__func__);
    return 0;
}

