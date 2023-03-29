
#define BTB_STATE_LOW_LEVER	(0)
#define BTB_STATE_HIGH_LEVER	(1)
#define PULLUP	(1)
#define PULLDOWN	(2)
#define GET_STATUS_DONE	(1)
#define GET_STATUS_FAILED	(0)
#define CHECKED_OK	(1)
#define CHECK_SKIPPED	(0)
#define CHECKED_ERROR	(-1)
#define MAX_REPPORT_TIMES	(5)
#define GPIO_LCD_BTB_NAME           "gpio_lcd_btb"

extern int mipi_lcd_btb_check(void);
extern void lcd_btb_init(char *dts_comp_lcd_name);
extern int lcd_check_mipi_fifo_empty(char __iomem *dsi_base);
extern void hw_lcd_get_on_time(void);
extern void hw_lcd_delay_on(int lcd_panel_on_delay_time, uint32_t bl_level );
extern void hw_lcd_delay_off(int lcd_panel_on_delay_time);


extern uint32_t gpio_lcd_btb;
extern struct semaphore lcd_cmd_sem;


static struct lcd_btb_info {
	uint32_t get_status;	/*0:get information failed  1:get information succeed*/
	uint32_t btb_support;	/*0:not support btb check  1:support btb check*/
	uint32_t btb_con_addr;
};

static struct gpio_desc lcd_gpio_request_btb =
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_BTB_NAME, &gpio_lcd_btb, 0};

static struct gpio_desc lcd_gpio_free_btb =
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
		GPIO_LCD_BTB_NAME, &gpio_lcd_btb, 0};

static struct gpio_desc lcd_gpio_read_btb =
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_BTB_NAME, &gpio_lcd_btb, 0};
