#ifndef __LINUX_LCD_ON_OFF_CHECK_H__
#define __LINUX_LCD_ON_OFF_CHECK_H__

#ifndef CONFIG_LCD_ON_OFF_CHECK
void lcd_on_off_check_init(void)
{
}
void lcd_on_off_check_timer_start(void)
{
}
void lcd_on_off_state_set(int level)
{
}
#else
void lcd_on_off_check_init(void);
void lcd_on_off_check_timer_start(void);
void lcd_on_off_state_set(int level);
#endif
#endif