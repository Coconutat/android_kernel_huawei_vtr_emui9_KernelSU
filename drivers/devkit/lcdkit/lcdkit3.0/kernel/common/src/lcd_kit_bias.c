#include "lcd_kit_bias.h"
#include "lcd_kit_common.h"

static struct lcd_kit_bias_ops *g_bias_ops = NULL;
int lcd_kit_bias_register(struct lcd_kit_bias_ops* ops)
{
	if (g_bias_ops) {
		LCD_KIT_ERR("g_bias_ops has already been registered!\n");
		return LCD_KIT_FAIL;
	}
	g_bias_ops = ops;
	LCD_KIT_INFO("g_bias_ops register success!\n");
	return LCD_KIT_OK;
}

int lcd_kit_bias_unregister(struct lcd_kit_bias_ops* ops)
{
	if (g_bias_ops == ops) {
		g_bias_ops = NULL;
		LCD_KIT_INFO("g_bias_ops unregister success!\n");
		return LCD_KIT_OK;
	}
	LCD_KIT_ERR("g_bias_ops unregister fail!\n");
	return LCD_KIT_FAIL;
}

struct lcd_kit_bias_ops* lcd_kit_get_bias_ops(void)
{
	return g_bias_ops;
}

