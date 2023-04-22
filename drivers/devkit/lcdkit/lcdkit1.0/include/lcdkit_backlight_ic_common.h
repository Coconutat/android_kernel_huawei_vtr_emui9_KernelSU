#ifndef _LCDKIT_BACKLIGHT_IC_COMMON_H_
#define _LCDKIT_BACKLIGHT_IC_COMMON_H_
#include <linux/semaphore.h>

#define LCD_BACKLIGHT_IC_NAME_LEN 20
#define LCD_BACKLIGHT_INIT_CMD_NUM 30

#ifndef BIT
#define BIT(x)  (1<<(x))
#endif

#define TEST_OK 0
#define TEST_ERROR_DEV_NULL BIT(0)
#define TEST_ERROR_DATA_NULL BIT(1)
#define TEST_ERROR_CLIENT_NULL BIT(2)
#define TEST_ERROR_I2C BIT(3)
#define TEST_ERROR_LED1_OPEN BIT(4)
#define TEST_ERROR_LED2_OPEN BIT(5)
#define TEST_ERROR_LED3_OPEN BIT(6)
#define TEST_ERROR_LED1_SHORT BIT(7)
#define TEST_ERROR_LED2_SHORT BIT(8)
#define TEST_ERROR_LED3_SHORT BIT(9)
#define TEST_ERROR_CHIP_INIT BIT(10)

#define LED_ONE 1
#define LED_TWO 2
#define LED_THREE 3


enum backlight_ctrl_mode
{
	BL_REG_ONLY_MODE = 0,
	BL_PWM_ONLY_MODE = 1,
	BL_MUL_RAMP_MODE = 2,
	BL_RAMP_MUL_MODE = 3
};

enum baclight_ic_type
{
	BACKLIGHT_IC = 0,
	BACKLIGHT_BIAS_IC = 1
};

enum bl_resume_type
{
	BL_RESUME_IDLE = 0,
	BL_RESUME_SINK = 1,
	BL_RESUME_REMP_OVP_OCP = 2
};

struct backlight_ic_cmd
{
    unsigned char ops_type; //0:read  1:write  2:update
    unsigned char cmd_reg;
    unsigned char cmd_val;
    unsigned char cmd_mask;
};

struct backlight_reg_info
{
    unsigned char val_bits;
	unsigned char cmd_reg;
    unsigned char cmd_val;
    unsigned char cmd_mask;	
};


struct lcdkit_bl_ic_info
{
	unsigned int   bl_level;
	unsigned int   bl_ctrl_mod;
	unsigned int   ic_type;
    unsigned int   ic_before_init_delay;
    unsigned int   ic_init_delay;
    unsigned int   ovp_check_enable;
    unsigned int   fake_lcd_ovp_check;
	struct backlight_ic_cmd init_cmds[LCD_BACKLIGHT_INIT_CMD_NUM];
	unsigned int   num_of_init_cmds;
	struct backlight_reg_info bl_lsb_reg_cmd;
    struct backlight_reg_info bl_msb_reg_cmd;
	struct backlight_ic_cmd bl_enable_cmd;
	struct backlight_ic_cmd bl_disable_cmd;
	struct backlight_ic_cmd disable_dev_cmd;
	struct backlight_ic_cmd bl_fault_flag_cmd;
	struct backlight_ic_cmd bias_enable_cmd;
	struct backlight_ic_cmd bias_disable_cmd;
    unsigned int   led_open_short_test;
    unsigned int   led_num;
    struct backlight_ic_cmd bl_brt_ctrl_cmd;
    struct backlight_ic_cmd bl_fault_ctrl_cmd;
	unsigned int   ic_hidden_reg_support;
	struct backlight_ic_cmd security_reg_enable_cmd;
	struct backlight_ic_cmd security_reg_disable_cmd;
	struct backlight_ic_cmd hidden_reg_cmd;
	unsigned int   hidden_reg_val_mask;
	unsigned int   suspend_disbrightness_support;
	unsigned int   bl_wq_support;
	unsigned int   bl_enhance_support;
	unsigned int   bl_lowpower_delay;
	unsigned int   bl_normal_level;
	unsigned int   bl_enhance_level;
	unsigned int   bl_enhance_hrdtimer_time;
	struct backlight_ic_cmd bl_normal_ovp_cmd;
	struct backlight_ic_cmd bl_normal_cur_ramp_cmd;
	struct backlight_ic_cmd bl_normal_boost_cur_cmd;
	struct backlight_ic_cmd bl_normal_bl_sink_cmd;
	struct backlight_ic_cmd bl_enhance_ovp_cmd;
	struct backlight_ic_cmd bl_enhance_cur_ramp_cmd;
	struct backlight_ic_cmd bl_enhance_boost_cur_cmd;
	struct backlight_ic_cmd bl_enhance_bl_sink_cmd;
	unsigned int   bl_ocp_fault_bit;
	unsigned int   bl_ovp_fault_bit;
	unsigned int   bl_tsd_bit;
};

struct lcdkit_bl_ic_device {
    struct device *dev;
	struct i2c_client *client;
	struct lcdkit_bl_ic_info bl_config;
    struct semaphore test_sem;
	struct work_struct bl_resume_worker;
	struct workqueue_struct *bl_resume_wq;
	struct hrtimer bl_resume_hrtimer;
};

int lcdkit_backlight_ic_inital(void);
int lcdkit_backlight_ic_set_brightness(unsigned int level);
int lcdkit_backlight_ic_enable_brightness(void);
int lcdkit_backlight_ic_disable_brightness(void);
int lcdkit_backlight_ic_disable_device(void);
int lcdkit_backlight_ic_fault_check(unsigned char *pval);
int lcdkit_backlight_ic_bias(bool enable);
void lcdkit_backlight_ic_get_chip_name(char *pname);
void lcdkit_parse_backlight_ic_config(struct device_node *np);
int lcdkit_backlight_ic_get_ctrl_mode(void);
struct lcdkit_bl_ic_info * lcdkit_get_lcd_backlight_ic_info(void);
void lcdkit_before_init_delay(void);
#if defined (CONFIG_HUAWEI_DSM)
void lcdkit_backlight_ic_ovp_check(void);
#endif

#endif
