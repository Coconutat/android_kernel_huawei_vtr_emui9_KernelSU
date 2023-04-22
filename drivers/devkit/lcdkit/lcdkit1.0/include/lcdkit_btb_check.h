#define BTB_STATE_LOW_LEVER	(0)
#define BTB_STATE_HIGH_LEVER	(1)
#define PULLUP	(1)
#define PULLDOWN	(2)
#define CHECKED_OK	(1)
#define CHECK_SKIPPED	(0)
#define CHECKED_ERROR	(-1)
#define MAX_REPPORT_TIMES	(5)
#define BUF_LENGTH	(50)
#define GPIO_LCD_BTB_NAME           "gpio_lcdkit_btb"

static struct lcdkit_btb_info {
	unsigned char btb_support;	/*0:not support btb check  1:support btb check*/
	unsigned int btb_con_addr;
	unsigned int gpio_lcdkit_btb;
};

enum btb_gpio_optype {
	BTB_GPIO_REQUEST = 0,
	BTB_GPIO_READ,
	BTB_GPIO_FREE,
};

extern int mipi_lcdkit_btb_check(void);

extern struct lcdkit_btb_info lcdkit_btb_inf;
