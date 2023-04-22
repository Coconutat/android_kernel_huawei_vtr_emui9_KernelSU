#include <linux/notifier.h>
#include "tpkit_platform_adapter.h"
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif

unsigned int get_pd_charge_flag(void);
unsigned int get_boot_into_recovery_flag(void);

int charger_type_notifier_register(struct notifier_block *nb)
{
    return 0;
}
int charger_type_notifier_unregister(struct notifier_block *nb)
{
    return 0;
}
enum ts_charger_type get_charger_type(void)
{
	return TS_CHARGER_TYPE_NONE;
}

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
void set_tp_dev_flag(void)
{	
	set_hw_dev_flag(DEV_I2C_TOUCH_PANEL);
      return;
}
#endif

int read_tp_color_adapter(char * buf,int buf_size)
{
    return -1;
}
int write_tp_color_adapter(char * buf)
{
    return -1;
}
unsigned int get_into_recovery_flag_adapter(void)
{	
	unsigned int touch_recovery_mode =0;
	touch_recovery_mode = get_boot_into_recovery_flag();
      return touch_recovery_mode;
}
unsigned int get_pd_charge_flag_adapter(void)
{
	unsigned int charge_flag =0;
	charge_flag = get_pd_charge_flag();
      return charge_flag;
}
int fb_esd_recover_disable(int value)
{
	return 0;
}