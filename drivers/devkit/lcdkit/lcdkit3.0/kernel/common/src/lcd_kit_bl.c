#include "lcd_kit_common.h"
#include "lcd_kit_bl.h"

static struct lcd_kit_bl_ops *g_bl_ops = NULL;
int lcd_kit_bl_register(struct lcd_kit_bl_ops* ops)
{
	if (g_bl_ops) {
		LCD_KIT_ERR("g_bl_ops has already been registered!\n");
		return LCD_KIT_FAIL;
	}
	g_bl_ops = ops;
	LCD_KIT_INFO("g_bl_ops register success!\n");
	return LCD_KIT_OK;
}

int lcd_kit_bl_unregister(struct lcd_kit_bl_ops* ops)
{
	if (g_bl_ops == ops) {
		g_bl_ops = NULL;
		LCD_KIT_INFO("g_bl_ops unregister success!\n");
		return LCD_KIT_OK;
	}
	LCD_KIT_ERR("g_bl_ops unregister fail!\n");
	return LCD_KIT_FAIL;
}

struct lcd_kit_bl_ops* lcd_kit_get_bl_ops(void)
{
	return g_bl_ops;
}
