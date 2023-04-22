#include <linux/module.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/sched/rt.h>
#include <linux/workqueue.h>
#include <linux/vmalloc.h>
#include <linux/notifier.h>
#include <huawei_ts_kit.h>
#include <lcdkit_panel.h>

extern struct lcdkit_panel_data lcdkit_info;
volatile int g_ts_kit_lcd_brightness_info = 0;
EXPORT_SYMBOL(g_ts_kit_lcd_brightness_info);
volatile int g_tskit_pt_station_flag = 0;
EXPORT_SYMBOL(g_tskit_pt_station_flag);
#define TP_COLOR_BUF_SIZE		20
char tp_color_buf[TP_COLOR_BUF_SIZE] ={0};
EXPORT_SYMBOL(tp_color_buf);
#define TP_COLOR_SIZE   15
u8 cypress_ts_kit_color[TP_COLOR_SIZE];
EXPORT_SYMBOL(cypress_ts_kit_color);
unsigned int panel_name_flag = 0;
EXPORT_SYMBOL(panel_name_flag);
struct tp_kit_device_ops *tp_kit_ops;
EXPORT_SYMBOL(tp_kit_ops);
struct tp_thp_device_ops *tp_thp_ops;
EXPORT_SYMBOL(tp_thp_ops);
struct tp_synaptics_dev_ops *tp_synaptics_ops;
EXPORT_SYMBOL(tp_synaptics_ops);
volatile bool ts_kit_gesture_func = false;
EXPORT_SYMBOL(ts_kit_gesture_func);
volatile bool g_tskit_fw_upgrade_flag = false;
EXPORT_SYMBOL(g_tskit_fw_upgrade_flag);

void lcd_huawei_ts_kit_register(struct tp_kit_device_ops *tp_kit_device_ops){
	tp_kit_ops = tp_kit_device_ops;
}
EXPORT_SYMBOL(lcd_huawei_ts_kit_register);
void lcd_huawei_thp_register(struct tp_thp_device_ops *tp_thp_device_ops){
	tp_thp_ops = tp_thp_device_ops;
}
EXPORT_SYMBOL(lcd_huawei_thp_register);
void lcd_huawei_ts_synaptics_register(struct tp_synaptics_dev_ops *tp_synaptics_dev_ops){
	tp_synaptics_ops = tp_synaptics_dev_ops;
}
EXPORT_SYMBOL(lcd_huawei_ts_synaptics_register);

char *trans_lcd_panel_name_to_tskit(void){
	return lcdkit_info.panel_infos.panel_name;
}
EXPORT_SYMBOL(trans_lcd_panel_name_to_tskit);

volatile int g_tskit_ic_type= HYBRID;  //this type means oncell incell tddi ... in order to decide the power policy between lcd & tp
EXPORT_SYMBOL(g_tskit_ic_type);
bool isbulcked;
EXPORT_SYMBOL(isbulcked);
