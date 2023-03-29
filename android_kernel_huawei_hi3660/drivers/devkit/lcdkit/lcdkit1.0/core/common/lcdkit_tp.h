#ifndef __LCDKIT_TP_H_
#define __LCDKIT_TP_H_

struct tp_kit_device_ops
{
    void (*tp_thread_stop_notify)(void);
};

struct tp_thp_device_ops
{
    int (*thp_otp_read)(char *otp_data);
};

struct tp_synaptics_dev_ops
{
    bool (*synaptics_tddi_power_seq)(void);
};
void lcd_huawei_ts_kit_register(struct tp_kit_device_ops *tp_kit_device_ops);
void lcd_huawei_ts_synaptics_register(struct tp_synaptics_dev_ops *tp_synaptics_dev_ops);
void lcd_huawei_thp_register(struct tp_thp_device_ops *tp_thp_device_ops);
char *trans_lcd_panel_name_to_tskit(void);

extern struct tp_synaptics_dev_ops *tp_synaptics_ops;
extern struct tp_kit_device_ops *tp_kit_ops;
extern struct tp_thp_device_ops *tp_thp_ops;
extern struct attribute_group lcdkit_fb_attr_group;
#endif

