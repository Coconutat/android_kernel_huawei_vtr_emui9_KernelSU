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
#include <soc/mediatek/log_store_kernel.h>
#include <chipset_common/bfmr/public/bfmr_public.h>
#include <chipset_common/bfmr/bfm/core/bfm_core.h>
#include <chipset_common/bfmr/bfm/chipsets/mtk/bfm_mtk.h>

struct boot_log_struct *boot_log = NULL;

#define OFFSET_TAG_SIZE 8
#define OFFSET_TAG "PAD0"

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

void set_boot_stage(bfmr_detail_boot_stage_e stage)
{
    if(NULL != boot_log)
    {
        boot_log->boot_stage = stage;
        pr_info("%s: boot_stage = 0x%08x\n", __func__, boot_log->boot_stage);
        boot_log->hash_code = hwboot_calculate_checksum((u8 *)boot_log, BOOT_LOG_CHECK_SUM_SIZE);
    }
       return ;
}
EXPORT_SYMBOL(set_boot_stage);

u32 get_boot_stage(void)
{
    if(NULL != boot_log)
    {
        pr_info(" boot_stage = 0x%08x\n", boot_log->boot_stage);
        return boot_log->boot_stage;
    }
    pr_err("get boot stage fail\n");
    return 0;
}
EXPORT_SYMBOL(get_boot_stage);


int set_boot_fail_flag(bfmr_bootfail_errno_e bootfail_errno)
{
    int ret = -1;
    if(NULL != boot_log)
    {
        if(!boot_log->boot_error_no)
        {
            boot_log->boot_error_no = bootfail_errno;
            boot_log->hash_code = hwboot_calculate_checksum((u8 *)boot_log, BOOT_LOG_CHECK_SUM_SIZE);
            pr_info(" boot_error_no = 0x%08x\n", boot_log->boot_error_no);
            ret = 0;
        }
    }
    return ret;
}
EXPORT_SYMBOL(set_boot_fail_flag);

void  hwboot_fail_init_struct(void)
{
    /* init lk log structure */    
    struct sram_log_header *bf_sram_header = NULL;

    bf_sram_header = ioremap_wc(CONFIG_MTK_DRAM_LOG_STORE_ADDR,
        CONFIG_MTK_DRAM_LOG_STORE_SIZE);

    if (bf_sram_header->sig != SRAM_HEADER_SIG) {
        pr_err("bf_sram_header is not match\n");
        return;
    }

    boot_log = (struct boot_log_struct*)&bf_sram_header->reserve[0];

    if (boot_log->boot_magic != HWBOOT_MAGIC_NUMBER/* || 
        hwboot_match_checksum(boot_log)*/) {
        pr_err("boot_log structure is err\n");
        boot_log = NULL;
        return;
    }

    if(NULL != boot_log){
        set_boot_stage(KERNEL_STAGE_START);
    }else {
        pr_notice("hwboot: bootlog is null\n");
    }
    return;
}
EXPORT_SYMBOL(hwboot_fail_init_struct);


void hwboot_clear_magic(void)
{
    boot_log->boot_magic = 0;
    return;
}
EXPORT_SYMBOL(hwboot_clear_magic);
