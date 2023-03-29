

#include <linux/module.h>
#ifdef CONFIG_HUAWEI_WPA_DMD
#include <dsm/dsm_pub.h>
#endif

#ifdef CONFIG_HUAWEI_WPA_DMD
static struct dsm_dev dsm_wpa = {
    .name = "dsm_wpa",
    .device_name = NULL,
    .ic_name = NULL,
    .module_name = NULL,
    .fops = NULL,
    .buff_size = 1024,
};

struct dsm_client *wpa_dsm_client = NULL;

void hw_register_wpa_dsm_client(void)
{
    if(NULL == wpa_dsm_client) {
        wpa_dsm_client = dsm_register_client(&dsm_wpa);
    }
}

#if 0
#define LOG_BUF_SIZE 512
void hw_wpa_dsm_client_notify(int dsm_id, const char *fmt, ...)
{
    char buf[LOG_BUF_SIZE] = {0};
    va_list ap;

    va_start(ap, fmt);
    if(wpa_dsm_client && !dsm_client_ocuppy(wpa_dsm_client)) {
        if(fmt) {
            vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
            dsm_client_record(wpa_dsm_client, buf);
        }
        dsm_client_notify(wpa_dsm_client, dsm_id);
        printk("wifi dsm_client_notify success\n");
    } else {
        printk("wifi dsm_client_notify failed\n");
    }
    va_end(ap);
}
#endif
#else
void hw_register_wpa_dsm_client(void)
{
  return;
}

void hw_wpa_dsm_client_notify(int dsm_id, const char *fmt, ...)
{
  return;
}
#endif
static int __init wpa_dsm_init(void)
{
	hw_register_wpa_dsm_client();
	return 0;
}
static void __exit wpa_dsm_deinit(void)
{
	return;
}
module_init(wpa_dsm_init);
module_exit(wpa_dsm_deinit);
