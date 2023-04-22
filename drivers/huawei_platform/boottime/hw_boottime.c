#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <huawei_platform/boottime/hw_boottime.h>
#define BOOT_LOG_NUM 64
#define BOOT_50_MS 50000
unsigned int bl1_time = 0;
unsigned int logo_time = 0;
unsigned int bl2_time = 0;
struct boot_log_struct
{
    u32 Second;
    u32 mSecond;
    char event[BOOT_STR_SIZE];
}hw_boottime[BOOT_LOG_NUM];

int boot_log_count = 0;
static DEFINE_MUTEX(hw_boottime_lock);
static int hw_boottime_enabled = 1;
#define BOOTUP_DONE "[INFOR]_wm_boot_animation_done"

int __init_or_module do_boottime_initcall(initcall_t fn)
{
    int ret;
    unsigned long long duration;
    ktime_t calltime, delta, rettime;
    char log_info[BOOT_STR_SIZE] = {0};

    calltime = ktime_get();
    ret = fn();
    rettime = ktime_get();
    delta = ktime_sub(rettime, calltime);
    duration = (unsigned long long) ktime_to_ns(delta) >> 10;
    if(duration > BOOT_50_MS)
    {
        printk(KERN_DEBUG "initcall %pF returned %d after %lld usecs\n",
        fn, ret, duration);
        snprintf(log_info,sizeof(log_info),"[WARNING] %pF %lld usecs", fn, duration);
        boot_record(log_info);
    }
    return ret;
}

static long long Second(unsigned long long nsec, unsigned long divs)
{
    if( (long long)nsec < 0 )
    {
        nsec = -nsec;/*lint !e501 */
        do_div(nsec, divs);
        return -nsec;/*lint !e501 */
    }
    do_div(nsec, divs);
    return nsec;
}

static long long mSecond(unsigned long long nsec, unsigned long divs)
{
    if( (long long)nsec < 0 )
        nsec = -nsec;/*lint !e501 */
    return do_div(nsec, divs);
}

static int __init get_bl1_time(char *str)
{
    int tmp;

    if (get_option(&str, &tmp))
    {
        bl1_time=tmp;
        return 0;
    }
    return -EINVAL;
}
early_param("bl1_time", get_bl1_time);

static int __init get_logo_time(char *str)
{
    int tmp;

    if (get_option(&str, &tmp))
    {
        logo_time=tmp;
        return 0;
    }
    return -EINVAL;
}
early_param("logo_time", get_logo_time);

static int __init get_bl2_time(char *str)
{
    int tmp;

    if (get_option(&str, &tmp))
    {
        bl2_time=tmp;
        return 0;
    }
    return -EINVAL;
}
early_param("bl2_time", get_bl2_time);

void boot_record(char *str)
{
    unsigned long long ts, tmp;
    if(0 == hw_boottime_enabled)
        return;
#ifdef CONFIG_HISI_TIME
    ts =  hisi_getcurtime();
#else
    ts = sched_clock();
#endif
    if((ts == 0) || (str == NULL))
    {
        printk("[boottime] invalid boottime point\n");
        return;
    }
    if(boot_log_count >= BOOT_LOG_NUM)
    {
        printk("[boottime] no enough boottime buffer\n");
        return;
    }
    if (strncmp(BOOTUP_DONE, str, strlen(BOOTUP_DONE)) == 0)
    {
        hw_boottime_enabled = 0;//zygote start
    }
    mutex_lock(&hw_boottime_lock);
    tmp = ts;
    hw_boottime[boot_log_count].Second = Second(tmp, 1000000000);
    tmp = ts;
    hw_boottime[boot_log_count].mSecond = mSecond(tmp, 1000000000);
	memset(&hw_boottime[boot_log_count].event, 0,
	sizeof(hw_boottime[boot_log_count].event));
    strncpy( (char*)&hw_boottime[boot_log_count].event, str, BOOT_STR_SIZE );
    boot_log_count++;
    mutex_unlock(&hw_boottime_lock);
}
EXPORT_SYMBOL(boot_record);

static int hw_boottime_show(struct seq_file *m, void *v)
{
    int i;
    seq_printf(m, "----------- BOOT TIME (sec1) -----------\n");
    seq_printf(m, "%2d.%d       s : [INFOR] %s\n", bl1_time/1000, bl1_time%1000, "bl1 boot time");
    seq_printf(m, "%2d.%d       s : [INFOR] %s\n", logo_time/1000, logo_time%1000, "display logo time");
    seq_printf(m, "%2d.%d       s : [INFOR] %s\n", bl2_time/1000, bl2_time%1000, "bl2 boot time");

    for(i=0; i<boot_log_count; i++)
    {
        seq_printf(m, "%2d.%09u s : %s\n", hw_boottime[i].Second, hw_boottime[i].mSecond, hw_boottime[i].event);
    }
    seq_printf(m, "\n   %s", hw_boottime_enabled?"starting...":"start done");
    return 0;
}

static int hw_boottime_open(struct inode *inode, struct file *file)
{
    return single_open(file, hw_boottime_show, inode->i_private);
}

static ssize_t hw_boottime_write(struct file *filp, const char *ubuf,size_t cnt, loff_t *data)
{
    char buf[BOOT_STR_SIZE] = { 0 };
    size_t copy_size = cnt;
    if (cnt >= sizeof(buf))
        copy_size = BOOT_STR_SIZE - 1;
    if (copy_from_user(&buf, ubuf, copy_size))
        return -EFAULT;

    buf[copy_size] = 0;
    boot_record(buf);
    return cnt;

}

static const struct file_operations hw_boottime_fops = {
    .open = hw_boottime_open,
    .write = hw_boottime_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int __init init_boot_time(void){
    struct proc_dir_entry *pe;
    pe = proc_create("boottime", 0664, NULL, &hw_boottime_fops);
    if (!pe)
        return -ENOMEM;
    return 0;
}
__initcall(init_boot_time);
