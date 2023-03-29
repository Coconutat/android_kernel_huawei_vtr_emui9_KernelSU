#include <linux/kthread.h>
#include <linux/sched/rt.h>
#include <linux/random.h>
#include <huawei_platform/log/hw_log.h>
#include <dsm/dsm_pub.h>
#include <linux/module.h>
#include <linux/printk.h>
#ifdef FACTORY_TEST
#define HWLOG_TAG    DSM_KIRIN_CHIPSETS
HWLOG_REGIST();

#define DSM_LOG_INFO(x...)      _hwlog_info(HWLOG_TAG, ##x)
#define DSM_LOG_ERR(x...)       _hwlog_err(HWLOG_TAG, ##x)
#define DSM_LOG_DEBUG(x...)     _hwlog_debug(HWLOG_TAG, ##x)

#define POWER_KEY_CLIENT_NAME "dsm_power_key"

struct dsm_dev dsm_kirin_chipsets = {
    .name = "dsm_kirin_chipsets",
    .device_name = NULL,
    .ic_name = NULL,
    .module_name = NULL,
    .fops = NULL,
    .buff_size = 1024,
};

struct dsm_client *kirin_chipsets_client = NULL;
#endif

static void check_long_press_powe_key(void)
{
#ifdef FACTORY_TEST
    struct dsm_client *power_key_client = dsm_find_client(POWER_KEY_CLIENT_NAME);
    if(power_key_client) {
        if(NULL != strstr(saved_command_line, "islongpress=true")) {
            if (!dsm_client_ocuppy(power_key_client)) {
            dsm_client_record(power_key_client,
                "long press poweron key happend.\n");
            dsm_client_notify(power_key_client,
                 DSM_LONG_PRESS_POWER_KEY_ERROR_NO);
            }
            else
                printk(KERN_ERR "long press occupy fail\n");
        }
    }
#endif
    return;

}
static int __init dsm_kirin_chipsets_init(void)
{
#ifdef FACTORY_TEST
    if (!kirin_chipsets_client) {
        kirin_chipsets_client = dsm_register_client(&dsm_kirin_chipsets);
    }
    if(!kirin_chipsets_client){
        DSM_LOG_ERR("kirin_chipsets_client reg failed\n");
    }
    check_long_press_powe_key();
#endif
    return 0;
}
module_init(dsm_kirin_chipsets_init);
