#include "lcdkit_bias_bl_utility.h"
#include "lcdkit_disp.h"
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif

struct device_node* lcdkit_lcd_np = NULL;

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
    struct device_node* np = NULL;
    char *tmp_name = NULL;
    int name_len = 0;

    if(NULL == buf)
    {
        return -1;
    }
    np = of_find_compatible_node(NULL, NULL, DTS_COMP_LCDKIT_PANEL_TYPE);
    if(!np)
    {
        printk("NOT FOUND device node %s!\n", DTS_COMP_LCDKIT_PANEL_TYPE);
        return -1;
    }

    tmp_name = (char*)of_get_property(np, "lcd-bl-ic-name", NULL);
    if(NULL == tmp_name)
    {
        return -1;
    }

    name_len = strlen(tmp_name);
    if(0 == name_len)
    {
        return -1;
    }

    if(name_len < len)
    {
        memcpy(buf, tmp_name, name_len+1);
    }
    else
    {
        memcpy(buf, tmp_name, len-1);
    }
    buf[len-1] = 0;

    return 0;
}
int lcdkit_get_bias_ic_name(char* buf, int len)
{
    struct device_node* np = NULL;
    char *tmp_name = NULL;
    int name_len = 0;

	if(NULL == buf)
    {
        return -1;
    }
    np = of_find_compatible_node(NULL, NULL, DTS_COMP_LCDKIT_PANEL_TYPE);
    if(!np)
    {
        printk("NOT FOUND device node %s!\n", DTS_COMP_LCDKIT_PANEL_TYPE);
        return -1;
    }

    tmp_name = (char*)of_get_property(np, "lcd-bias-ic-name", NULL);
    if(NULL == tmp_name)
    {
        return -1;
    }

    name_len = strlen(tmp_name);
    if(0 == name_len)
    {
        return -1;
    }

    if(name_len < len)
    {
        memcpy(buf, tmp_name, name_len+1);
    }
    else
    {
        memcpy(buf, tmp_name, len-1);
    }
    buf[len-1] = 0;

    return 0;
}

struct device_node* lcdkit_get_lcd_node(void)
{
    struct device_node* np = NULL;
    struct lcdkit_panel_data* plcdkit_info = NULL;

    plcdkit_info = lcdkit_get_panel_info();
    if(NULL == plcdkit_info)
    {
        printk("lcdkit_info is NULL Point!\n");
        return NULL;
    }
    np = of_find_compatible_node(NULL, NULL, plcdkit_info->panel_infos.lcd_compatible);
    if(!np)
    {
        printk("NOT FOUND device node %s!\n", plcdkit_info->panel_infos.lcd_compatible);
        return NULL;
    }
    lcdkit_lcd_np = np;

    return lcdkit_lcd_np;
}

void lcdkit_set_lcd_node(struct device_node* pnode)
{
    return;
}

uint32_t lcdkit_check_lcd_plugin(void)
{
    return g_fake_lcd_flag;
}
