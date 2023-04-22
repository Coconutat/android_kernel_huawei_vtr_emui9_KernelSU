#ifndef __LINUX_HW_BOOTTIME_H__
#define __LINUX_HW_BOOTTIME_H__

#ifndef CONFIG_HUAWEI_BOOT_TIME
void boot_record(char *str)
{
}
int __init_or_module do_boottime_initcall(initcall_t fn)
{
}
#else
void boot_record(char *str);
int __init_or_module do_boottime_initcall(initcall_t fn);
#endif
#define BOOT_STR_SIZE 128
#endif