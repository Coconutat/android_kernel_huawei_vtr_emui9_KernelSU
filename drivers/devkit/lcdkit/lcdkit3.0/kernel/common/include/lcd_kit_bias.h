#ifndef _LCD_KIT_BIAS_H_
#define _LCD_KIT_BIAS_H_

struct lcd_kit_bias_ops {
	int (*set_bias_voltage)(int vpos, int vneg);
	int (*dbg_set_bias_voltage)(int vpos, int vneg);
};

/*function declare*/
struct lcd_kit_bias_ops* lcd_kit_get_bias_ops(void);
int lcd_kit_bias_register(struct lcd_kit_bias_ops* ops);
int lcd_kit_bias_unregister(struct lcd_kit_bias_ops* ops);
#endif

