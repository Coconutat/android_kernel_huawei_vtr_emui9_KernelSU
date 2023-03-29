#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <asm/byteorder.h>
#include "huawei_thp.h"
#include <lcdkit_tp.h>

#define THP_BU21150_DEV_NODE_NAME "rohm-bu21150"
#define BU21150_IC_NAME "rohm"

#define SPI_HEADER_SIZE (3)
#define SPI_BITS_PER_WORD_READ (8)
#define SPI_BITS_PER_WORD_WRITE (8)
#define COMM_TEST_REG (0x0010)
#define COMM_TEST_RW_LENGTH (8)
#define COMM_TEST_STR "HOST_SPI"
#define REG_SIZE             (2)
#define COMM_TEST_RW_RETRY_TIME	3
#define COMM_TEST_RW_RETRY_DELAY_MS	50
#define FRAME_DATA_REG	(0x0400)
#define IRQ_SETTINGS_REG (0x007E)
#define INT_RUN_ENB_REG (0x00CE)
#define SENS_START_REG (0x0086)
#define OTP_ADDR_REG (0x0088)
#define OTP_ADDR_RECALL_REG (0x0098)
#define OTP_SIZE	8

int bu21150_otp_read(char *otp_data);
extern void lcd_huawei_thp_register(struct tp_thp_device_ops *tp_thp_device_ops);
struct tp_thp_device_ops ts_thp_ops = {
    .thp_otp_read = bu21150_otp_read,
};

static void swap_2byte(unsigned char *buf, unsigned int size)
{
	unsigned int i;

	if (size % REG_SIZE == 1) {
		THP_LOG_ERR("%s:error size is odd.size=[%u]\n", __func__, size);
		return;
	}

	for (i = 0; i < size; i += REG_SIZE)
		be16_to_cpus((__u16 *)(buf + i));
}

static int thp_bu21150_spi_transfer(struct thp_device *tdev,
			struct spi_transfer *xfer, struct spi_message *msg)
{
	int rc;
	struct spi_device *sdev = tdev->sdev;

	THP_LOG_DEBUG("%s called\n", __func__);

	spi_message_add_tail(xfer, msg);
	thp_bus_lock();
	rc = thp_spi_sync(sdev, msg);
	thp_bus_unlock();

	return rc;
}

static int thp_bu21150_read_register_(struct thp_device *tdev,
			u32 addr, u16 size, u8 *data)
{
	struct spi_transfer t;
	struct spi_message msg;
	int rc;
	char *tx_buff = tdev->tx_buff;
	char *rx_buff = tdev->rx_buff;

	THP_LOG_DEBUG("%s: called\n", __func__);

#if defined (CONFIG_TEE_TUI)
	if(thp_tui_info.enable)
		return 0;
#endif

	/* set header */
	tx_buff[0] = 0x03;	/* read command */
	tx_buff[1] = (addr & 0xFF00) >> 8;	/* address hi */
	tx_buff[2] = (addr & 0x00FF) >> 0;	/* address lo */

	/* read data */
	spi_message_init(&msg);
	memset(&t, 0, sizeof(struct spi_transfer));
	t.tx_buf = tx_buff;
	t.rx_buf = rx_buff;
	t.len = size + SPI_HEADER_SIZE;
	t.cs_change = 1;
	t.bits_per_word = SPI_BITS_PER_WORD_READ;

	rc = thp_bu21150_spi_transfer(tdev, &t, &msg);
	if (rc)
		THP_LOG_ERR("%s: spi sync err,rc=%d\n", __func__, rc);
	else
		memcpy(data, rx_buff + SPI_HEADER_SIZE, size);

	return rc;

}
static int thp_bu21150_read_register(struct thp_device *tdev,
			u32 addr, u16 size, u8 *data)
{

	int rc = thp_bu21150_read_register_(tdev, addr, size, data);

	if (!rc)
		swap_2byte(data, size);

	return rc;
}

static int thp_bu21150_write_register(struct thp_device *tdev,
			u32 addr, u16 size, u8 *data)
{
	struct spi_transfer t;
	struct spi_message msg;
	int rc;
	char *tx_buff = tdev->tx_buff;
	char *rx_buff = tdev->rx_buff;

	THP_LOG_DEBUG("%s: called\n", __func__);

#if defined (CONFIG_TEE_TUI)
	if(thp_tui_info.enable)
		return 0;
#endif

	/* set header */
	tx_buff[0] = 0x02;		 /* write command */
	tx_buff[1] = (addr & 0xFF00) >> 8; /* address hi */
	tx_buff[2] = (addr & 0x00FF) >> 0; /* address lo */

	/* set data */
	memcpy(tx_buff + SPI_HEADER_SIZE, data, size);
	swap_2byte(tx_buff + SPI_HEADER_SIZE, size);

	/* write data */
	memset(&t, 0, sizeof(struct spi_transfer));
	spi_message_init(&msg);
	t.tx_buf = tx_buff;
	t.rx_buf = rx_buff;
	t.len = size + SPI_HEADER_SIZE;
	t.cs_change = 1;
	t.bits_per_word = SPI_BITS_PER_WORD_WRITE;

	rc = thp_bu21150_spi_transfer(tdev, &t, &msg);

	if (rc)
		 THP_LOG_ERR("%s:spi sync error:rc=%d\n", __func__, rc);

	return rc;
}
int bu21150_otp_read(char *otp_data)
{
	int ret;
	uint16_t i;
	u8 otp_recall[2] = {0x01, 0x00};
	u8 otp_read[2] = {0x00, 0x00};
	struct thp_core_data *cd = thp_get_core_data();
	struct thp_device *tdev = cd->thp_dev;

	if (!otp_data) {
		THP_LOG_ERR("%s:input null\n", __func__);
		return -EINVAL;
	}

	ret = thp_bu21150_write_register(tdev, OTP_ADDR_RECALL_REG,
			(u16)sizeof(otp_recall), otp_recall);
	if(ret) {
		THP_LOG_ERR("bu21150 OTP write failed!");
		return ret;
	}

	msleep(1);

	for(i = 0; i < OTP_SIZE; i++) {
		thp_bu21150_read_register(tdev, (OTP_ADDR_REG + i * 2),
			(u16)sizeof(otp_read), otp_read);
		sprintf(&otp_data[i * 2], "%c", otp_read[0]);
		sprintf(&otp_data[i * 2 + 1], "%c", otp_read[1]);
	}

	THP_LOG_INFO("%s:bu21150 OTP read data: %s\n", __func__, otp_data);

	return ret;
}
EXPORT_SYMBOL(bu21150_otp_read);

static int thp_bu21150_init(struct thp_device *tdev)
{
	int rc;
	struct thp_core_data *cd = tdev->thp_core;
	struct device_node *bu21150_node = of_get_child_by_name(cd->thp_node,
						THP_BU21150_DEV_NODE_NAME);

	THP_LOG_INFO("%s: called\n", __func__);

	if (!bu21150_node) {
		THP_LOG_INFO("%s: bu21150 dev not config in dts\n", __func__);
		return -ENODEV;
	}

	rc = thp_parse_spi_config(bu21150_node, cd);
	if (rc)
		THP_LOG_ERR("%s: spi config parse fail\n", __func__);

	rc = thp_parse_timing_config(bu21150_node, &tdev->timing_config);
	if (rc)
		THP_LOG_ERR("%s: timing config parse fail\n", __func__);

	return 0;
}

static void thp_bu21150_timing_work(struct thp_device *tdev)
{

	THP_LOG_ERR("%s:called\n", __func__);

	gpio_direction_output(tdev->gpios->rst_gpio, GPIO_LOW);
	thp_do_time_delay(tdev->timing_config.boot_reset_low_delay_ms);

	gpio_set_value(tdev->gpios->rst_gpio, THP_RESET_HIGH);
	thp_do_time_delay(tdev->timing_config.boot_reset_hi_delay_ms);
}

static int thp_bu21150_communication_check(
		struct thp_device *tdev)
{
	int i;
	u8 buf_zero[COMM_TEST_RW_LENGTH] = {0};
	u8 buf_read[COMM_TEST_RW_LENGTH + 1] = {0};

	for (i = 0; i < COMM_TEST_RW_RETRY_TIME; i++) {
		thp_bu21150_write_register(tdev, COMM_TEST_REG,
			COMM_TEST_RW_LENGTH, (u8 *)COMM_TEST_STR);
		thp_bu21150_read_register(tdev, COMM_TEST_REG,
			COMM_TEST_RW_LENGTH, buf_read);
		thp_bu21150_write_register(tdev, COMM_TEST_REG,
			COMM_TEST_RW_LENGTH, buf_zero);

		THP_LOG_INFO("%s:get str: %s\n", __func__, buf_read);

		if (!strncmp((char *)buf_read, COMM_TEST_STR,
			(unsigned long)COMM_TEST_RW_LENGTH)) {
			THP_LOG_INFO("%s:spi comm check success\n", __func__);
			return 0;
		} else {
			THP_LOG_ERR("%s:spi comm failed %d\n", __func__, i);
			msleep(COMM_TEST_RW_RETRY_DELAY_MS);
		}
	}

	return -ENODEV;
}

static int thp_bu21150_chip_detect(struct thp_device *tdev)
{
	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}

	thp_bu21150_timing_work(tdev);

	return thp_bu21150_communication_check(tdev);
}

static int thp_bu21150_get_frame(struct thp_device *tdev,
			char *buf, unsigned int len)
{
	if (!tdev) {
		THP_LOG_INFO("%s: input dev null\n", __func__);
		return -EINVAL;
	}

	if (!len) {
		THP_LOG_INFO("%s: read len illegal\n", __func__);
		return -EINVAL;
	}

	return thp_bu21150_read_register_(tdev, FRAME_DATA_REG, len, buf);
}

static int thp_bu21150_resume(struct thp_device *tdev)
{
	THP_LOG_INFO("%s: called_\n", __func__);

	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}
	gpio_set_value(tdev->gpios->rst_gpio, GPIO_HIGH);
	gpio_set_value(tdev->gpios->cs_gpio, GPIO_HIGH);
	thp_do_time_delay(tdev->timing_config.resume_reset_after_delay_ms);

	return 0;
}

static int thp_bu21150_suspend(struct thp_device *tdev)
{

	int rc;
	u8 sens_stop_cmd[] = {0x00, 0x00};	/* set sens stop command */
	u8 int_stop_cmd[] = {0x04, 0x00};	/* set INT stop command */

	THP_LOG_INFO("%s: called\n", __func__);

	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}

	rc = thp_bu21150_write_register(tdev, SENS_START_REG,
			(u16)sizeof(sens_stop_cmd), sens_stop_cmd);
	if (rc)
		THP_LOG_ERR("%s:err to write SENS_START_REG\n", __func__);

	rc = thp_bu21150_write_register(tdev, INT_RUN_ENB_REG,
			(u16)sizeof(int_stop_cmd), int_stop_cmd);
	if (rc)
		THP_LOG_ERR("%s:err to write INT_RUN_ENB_REG\n", __func__);

	gpio_set_value(tdev->gpios->rst_gpio, GPIO_LOW);
	gpio_set_value(tdev->gpios->cs_gpio, GPIO_LOW);
	thp_do_time_delay(tdev->timing_config.suspend_reset_after_delay_ms);

	return 0;
}

static void thp_bu21150_exit(struct thp_device *tdev)
{
	THP_LOG_INFO("%s: called\n", __func__);

	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return;
	}

	kfree(tdev->tx_buff);
	kfree(tdev->rx_buff);
	kfree(tdev);
}

static struct thp_device_ops bu21150_dev_ops = {
	.init = thp_bu21150_init,
	.detect = thp_bu21150_chip_detect,
	.get_frame = thp_bu21150_get_frame,
	.resume = thp_bu21150_resume,
	.suspend = thp_bu21150_suspend,
	.exit = thp_bu21150_exit,
};

static int __init thp_bu21150_module_init(void)
{
	int rc;

	struct thp_device *dev = kzalloc(sizeof(struct thp_device), GFP_KERNEL);
	if (!dev) {
		THP_LOG_ERR("%s: thp device malloc fail\n", __func__);
		return -ENOMEM;
	}

	dev->tx_buff = kzalloc(THP_MAX_FRAME_SIZE, GFP_KERNEL);
	dev->rx_buff = kzalloc(THP_MAX_FRAME_SIZE, GFP_KERNEL);
	if (!dev->tx_buff || !dev->rx_buff) {
		THP_LOG_ERR("%s: out of memory\n", __func__);
		rc = -ENOMEM;
		goto err;
	}

	dev->ic_name = BU21150_IC_NAME;
	dev->dev_node_name = THP_BU21150_DEV_NODE_NAME;
	dev->ops = &bu21150_dev_ops;

	rc = thp_register_dev(dev);
	if (rc) {
		THP_LOG_ERR("%s: register fail\n", __func__);
		goto err;
	}
#ifndef CONFIG_LCD_KIT_DRIVER
	lcd_huawei_thp_register(&ts_thp_ops);
#endif
	return rc;
err:
	if(dev->tx_buff){
		kfree(dev->tx_buff);
		dev->tx_buff = NULL;
	}
	if(dev->rx_buff){
		kfree(dev->rx_buff);
		dev->rx_buff = NULL;
	}
	if(dev){
		kfree(dev);
		dev = NULL;
	}
	return rc;
}
static void __exit thp_bu21150_module_exit(void)
{
	THP_LOG_INFO("%s: called \n", __func__);
};

module_init(thp_bu21150_module_init);
module_exit(thp_bu21150_module_exit);
MODULE_AUTHOR("bu21150");
MODULE_DESCRIPTION("bu21150 driver");
MODULE_LICENSE("GPL");
