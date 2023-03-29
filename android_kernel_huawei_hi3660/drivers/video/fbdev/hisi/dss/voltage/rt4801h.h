/*
 * Copyright 2016 HUAWEI Tech. Co., Ltd.
 */

#ifndef __RT4801H_H
#define __RT4801H_H

#define RT4801H_VOL_40     (0x00)     //4.0V
#define RT4801H_VOL_41     (0x01)     //4.1V
#define RT4801H_VOL_42     (0x02)     //4.2V
#define RT4801H_VOL_43     (0x03)     //4.3V
#define RT4801H_VOL_44     (0x04)     //4.4V
#define RT4801H_VOL_45     (0x05)     //4.5V
#define RT4801H_VOL_46     (0x06)     //4.6V
#define RT4801H_VOL_47     (0x07)     //4.7V
#define RT4801H_VOL_48     (0x08)     //4.8V
#define RT4801H_VOL_49     (0x09)     //4.9V
#define RT4801H_VOL_50     (0x0A)     //5.0V
#define RT4801H_VOL_51     (0x0B)     //5.1V
#define RT4801H_VOL_52     (0x0C)     //5.2V
#define RT4801H_VOL_53     (0x0D)     //5.3V
#define RT4801H_VOL_54     (0x0E)     //5.4V
#define RT4801H_VOL_55     (0x0F)     //5.5V
#define RT4801H_VOL_56     (0x10)     //5.6V
#define RT4801H_VOL_57     (0x11)     //5.7V
#define RT4801H_VOL_58     (0x12)     //5.8V
#define RT4801H_VOL_59     (0x13)     //5.9V
#define RT4801H_VOL_60     (0x14)     //6.0V
#define RT4801H_VOL_MAX    (0x15)     //6.1V

#define RT4801H_REG_VPOS   (0x00)
#define RT4801H_REG_VNEG   (0x01)
#define RT4801H_REG_APP_DIS   (0x03)

#define RT4801H_REG_VOL_MASK  (0x1F)
#define RT4801H_APPS_BIT   (1<<6)
#define RT4801H_DISP_BIT   (1<<1)
#define RT4801H_DISN_BIT   (1<<0)

struct rt4801h_voltage {
	u32 voltage;
	int value;
};

static struct rt4801h_voltage vol_table[] = {
	{4000000,RT4801H_VOL_40},
	{4100000,RT4801H_VOL_41},
	{4200000,RT4801H_VOL_42},
	{4300000,RT4801H_VOL_43},
	{4400000,RT4801H_VOL_44},
	{4500000,RT4801H_VOL_45},
	{4600000,RT4801H_VOL_46},
	{4700000,RT4801H_VOL_47},
	{4800000,RT4801H_VOL_48},
	{4900000,RT4801H_VOL_49},
	{5000000,RT4801H_VOL_50},
	{5100000,RT4801H_VOL_51},
	{5200000,RT4801H_VOL_52},
	{5300000,RT4801H_VOL_53},
	{5400000,RT4801H_VOL_54},
	{5500000,RT4801H_VOL_55},
	{5600000,RT4801H_VOL_56},
	{5700000,RT4801H_VOL_57},
	{5800000,RT4801H_VOL_58},
	{5900000,RT4801H_VOL_59},
	{6000000,RT4801H_VOL_60},
};


struct rt4801h_device_info {
    struct device  *dev;
    struct i2c_client      *client;
};

struct work_data {
	struct i2c_client *client;
	struct delayed_work setvol_work;
	int vpos;
	int vneg;
};

struct rt4801h_configure_info {
	char *lcd_name;
	int vpos_cmd;
	int vneg_cmd;
};

bool check_rt4801h_device(void);
int rt4801h_set_voltage(void);

#ifdef CONFIG_HISI_FB_6250
extern int is_normal_lcd(void);
extern int get_vsp_voltage(void);
extern int get_vsn_voltage(void);
#endif

#endif
