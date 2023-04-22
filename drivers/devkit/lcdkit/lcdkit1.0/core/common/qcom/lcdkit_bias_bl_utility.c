#include "lcdkit_bias_bl_utility.h"
static char bl_chip_name[LCD_BACKLIGHT_IC_NAME_LEN] = "default";
static char bias_chip_name[LCD_BIAS_IC_NAME_LEN] = "default";
static struct device_node* lcdkit_lcd_np = NULL;
extern bool lcdkit_is_default_panel(void);

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
void set_lcd_bias_dev_flag(void)
{
    set_hw_dev_flag(DEV_I2C_DC_DC);
    return;
}
#endif
int lcdkit_get_backlight_ic_name(char* buf, int len)
{
    int ret = 0;
    int name_len = 0;

    if(NULL == buf)
    {
        return -1;
    }

    name_len = strlen(bl_chip_name);
    if(0 == name_len)
    {
        return -1;
    }

    if(name_len < len)
    {
        memcpy(buf, bl_chip_name, name_len+1);
    }
    else
    {
        memcpy(buf, bl_chip_name, len-1);
    }
    buf[len-1] = 0;

    return 0;
}

int lcdkit_get_bias_ic_name(char* buf, int len)
{
    int ret = 0;
    int name_len = 0;

    if(NULL == buf)
    {
        return -1;
    }

    name_len = strlen(bias_chip_name);
    if(0 == name_len)
    {
        return -1;
    }

    if(name_len < len)
    {
        memcpy(buf, bias_chip_name, name_len+1);
    }
    else
    {
        memcpy(buf, bias_chip_name, len-1);
    }
    buf[len-1] = 0;

    return 0;
}

struct device_node* lcdkit_get_lcd_node(void)
{
    return lcdkit_lcd_np;
}

void lcdkit_set_lcd_node(struct device_node* pnode)
{
    lcdkit_lcd_np = pnode;
    return;
}

bool lcdkit_check_lcd_plugin(void)
{
    return lcdkit_is_default_panel();
}

void hisi_blpwm_bl_regisiter(int (*set_bl)(int bl_level))
{
	return;
}

static int __init early_parse_bias_ic_cmdline(char *arg)
{
    int len = 0;

    if (arg)
    {
		int buf_len = sizeof(bias_chip_name);
        memset(bias_chip_name, 0, sizeof(bias_chip_name));
        len = strlen(arg);
        if (len > buf_len)
        {
            len = buf_len;
        }
        memcpy(bias_chip_name, arg, len);
		bias_chip_name[buf_len-1] = 0;
	}
    else
    {
        printk("%s : arg is NULL\n", __func__);
    }

    return 0;
}

early_param("lcdbias_ic", early_parse_bias_ic_cmdline);

