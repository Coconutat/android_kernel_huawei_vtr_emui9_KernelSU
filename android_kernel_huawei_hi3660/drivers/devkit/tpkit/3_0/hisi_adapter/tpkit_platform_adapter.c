#include <linux/notifier.h>
#include <linux/hisi/usb/hisi_usb.h>
#include <huawei_platform/devdetect/hw_dev_dec.h>
#include <linux/mtd/hisi_nve_interface.h>
#include <huawei_platform/log/hw_log.h>
#include "tpkit_platform_adapter.h"

#define HWLOG_TAG tp_color
HWLOG_REGIST();

extern unsigned int get_boot_into_recovery_flag(void);
extern unsigned int get_pd_charge_flag(void);
extern int hisifb_esd_recover_disable(int value);
extern int set_hw_dev_flag( int dev_id );
extern int hisi_charger_type_notifier_register(struct notifier_block *nb);
extern int hisi_charger_type_notifier_unregister(struct notifier_block *nb);
extern enum hisi_charger_type hisi_get_charger_type(void);

int charger_type_notifier_register(struct notifier_block *nb)
{
    return hisi_charger_type_notifier_register(nb);
}
int charger_type_notifier_unregister(struct notifier_block *nb)
{
    return hisi_charger_type_notifier_unregister(nb);
}
enum ts_charger_type get_charger_type(void)
{
	return hisi_get_charger_type();
}
void set_tp_dev_flag(void)
{
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
    set_hw_dev_flag(DEV_I2C_TOUCH_PANEL);
#endif
}
int read_tp_color_adapter(char * buf,int buf_size)
{
	int ret = 0;
	struct hisi_nve_info_user user_info;

	memset(&user_info, 0, sizeof(user_info));
	user_info.nv_operation = 1;
	user_info.nv_number = 16;
	user_info.valid_size = 15;
	strncpy(user_info.nv_name, "TPCOLOR", sizeof(user_info.nv_name));
	user_info.nv_name[sizeof(user_info.nv_name) - 1] = '\0';
	ret = hisi_nve_direct_access(&user_info);
	if (ret) {
		hwlog_err("hisi_nve_direct_access read error(%d)\n", ret);
		return -1;
	}
	/* buf_size value is NV_DATA_SIZE 104 */
	strncpy(buf, user_info.nv_data, buf_size - 1);
	return 0;
}
EXPORT_SYMBOL(read_tp_color_adapter);
int write_tp_color_adapter(char * buf)
{
	int ret = 0;
	struct hisi_nve_info_user user_info;

	memset(&user_info, 0, sizeof(user_info));
	user_info.nv_operation = 0;
	user_info.nv_number = 16;
	user_info.valid_size = 15;
	strncpy(user_info.nv_name, "TPCOLOR", sizeof(user_info.nv_name)-1);
	user_info.nv_name[sizeof(user_info.nv_name) - 1] = '\0';
	strncpy(user_info.nv_data, buf, sizeof(user_info.nv_data)-1);
	ret = hisi_nve_direct_access(&user_info);
	if (ret) {
		hwlog_err("hisi_nve_direct_access write error(%d)\n", ret);
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL(write_tp_color_adapter);
unsigned int get_into_recovery_flag_adapter(void)
{	
    return get_boot_into_recovery_flag();	
}
unsigned int get_pd_charge_flag_adapter(void)
{
    return get_pd_charge_flag();
}
int fb_esd_recover_disable(int value)
{
    return hisifb_esd_recover_disable(value);
}
EXPORT_SYMBOL(fb_esd_recover_disable);
