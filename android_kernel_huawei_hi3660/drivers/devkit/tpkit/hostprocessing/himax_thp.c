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

#include <linux/firmware.h>
#define THP_HX83112_DEV_NODE_NAME "himax-hx83112"
#define hx83112_IC_NAME "himax"

#define SPI_BITS_PER_WORD (8)
#define SPI_BITS_PER_WORD_READ (8)
#define SPI_BITS_PER_WORD_WRITE (8)

#define SPI_FORMAT_ARRAY_SIZE 3

#define COMM_TEST_RW_LENGTH (8)

#define COMM_TEST_RW_RETRY_TIME	3
#define COMM_TEST_RW_RETRY_DELAY_MS	50
#define NO_ERR 0

#define HIMAX_BUS_RETRY_TIMES		10
#define HX_83112A_SERIES_PWON		14
#define HIMAX_NORMAL_DATA_LEN		4
#define HIMAX_NORMAL_ADDR_LEN	4
#define HIMAX_LONG_DATA_LEN		8
#define HIMAX_MAX_DATA_LEN		256
#define HIMAX_HEADER_LEN			4
#define HIMAX_BEGING_ADDR			0x00
#define HIMAX_CONTI_BURST_ADDR	0x13
#define HIMAX_CONTI_BURST_EN		0x31
#define HIMAX_CONTI_BURST_DIS
#define HIMAX_SSOFF_ADDR_FIRST	0x31
#define HIMAX_SSOFF_CMD_FIRST		0x27
#define HIMAX_SSOFF_ADDR_SECOND	0x32
#define HIMAX_SSOFF_CMD_SECOND	0x95
#define HIMAX_AUTO_PLUS_4_ADDR	0x0D
#define HIMAX_AUTO_PLUS_4_EN		0x11
#define HIMAX_AUTO_PLUS_4_DIS		0x10
#define HIMAX_WAKEUP_ADDR			0x08
#define HIMAX_FW_SSOFF				0x0C
#define HIMAX_HEADER_OK_HBYTE		0xA5
#define HIMAX_HEADER_OK_LBYTE		0x5A
#define HIMAX_REG_READ_EN_ADDR	0x0C
#define HIMAX_EVENT_STACK_CMD_ADDR	0x30
#define HIMAX_REG_READ_EN			0x00
#define HIMAX_SYS_RST_ADDR		0x90000018
#define HIMAX_SYS_RST_CMD			0x00000055
#define HIMAX_WTDOG_OFF_ADDR		0x9000800C
#define HIMAX_WTDOG_OFF_CMD		0x0000AC53
#define HIMAX_CHK_FW_STATUS		0x900000A8
#define HIMAX_ICID_ADDR			0x900000D0
#define HIMAX_ICID_83112A			0x83112A00
#define HIMAX_DDREG_ADDR			0x90000020
#define HIMAX_DDREG_CMD			0x00000000
#define HIMAX_TCON_RST_ADDR		0x800201E0
#define HIMAX_TCON_RST_LOW		0x00000000
#define HIMAX_TCON_RST_HIGH		0x00000001
#define HIMAX_RAWDATA_ADDR		0x10000000
#define HIMAX_RAWDATA_HEADER		0x00005AA5
#define HIMAX_SET_RAWDATA_ADDR	0x800204B4
#define HIMAX_SET_IIR_CMD			0x00000009
#define HIMAX_CHK_KEY_ADDR		0x800070E8

static u32 himax_id_match_table[] = {
	0x83112A00, /* chip HX83112A id */
	0x83112B00, /* chip HX83112B id */
	0x83112C00, /* chip HX83112C id */
	0x83112E00, /* chip HX83112E id */
};

struct himax_thp_private_data{
	int hx_get_frame_optimized_flag;
};
struct himax_thp_private_data thp_private_data;
/******* SPI-start *******/
static struct spi_device *hx_spi_dev;
/******* SPI-end *******/

void himax_assign_data(uint32_t cmd,uint8_t *tmp_value)
{
	tmp_value[3] = cmd / 0x1000000;
	tmp_value[2] = (cmd >> 16) % 0x100;
	tmp_value[1] = (cmd >> 8) % 0x100;
	tmp_value[0] = cmd % 0x100;
}

static void himax_spi_complete(void *arg)
{
	complete(arg);
}

static ssize_t himax_spi_sync(struct thp_device *tdev, struct spi_message *message)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int status;

	message->complete = himax_spi_complete;
	message->context = &done;

	if (hx_spi_dev == NULL)
		status = -ESHUTDOWN;
	else
	{
		thp_spi_cs_set(GPIO_HIGH);
		status = thp_spi_sync(hx_spi_dev, message);
	}


	return status;
}

static int himax_spi_read(struct thp_device *tdev,uint8_t *command,uint8_t command_len, uint8_t *data, unsigned int length, uint8_t toRetry)
{
	struct spi_message message;
	struct spi_transfer xfer[2];
	int retry;
	int error;

	spi_message_init(&message);
	memset(xfer, 0, sizeof(xfer));

	xfer[0].tx_buf = command;
	xfer[0].len = command_len;
	spi_message_add_tail(&xfer[0], &message);

	xfer[1].rx_buf = data;
	xfer[1].len = length;
	spi_message_add_tail(&xfer[1], &message);

	for (retry = 0; retry < toRetry; retry++) {
		thp_spi_cs_set(GPIO_HIGH);
		error = thp_spi_sync(hx_spi_dev, &message);
		if (unlikely(error))
			THP_LOG_ERR("SPI read error: %d\n", error);
		else
			break;
	}

	if (retry == toRetry) {
		THP_LOG_ERR("%s: SPI read error retry over %d\n",
			__func__, toRetry);
		return -EIO;
	}

	return 0;
}

static int himax_spi_write(struct thp_device *tdev, uint8_t *buf, unsigned int length)
{
	struct spi_transfer	t = {
		.tx_buf		= buf,
		.len		= length,
		.bits_per_word = SPI_BITS_PER_WORD_READ,
	};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	return himax_spi_sync(tdev,&m);
}

static int himax_bus_read(struct thp_device *tdev,uint8_t command, uint8_t *data, unsigned int length, uint8_t toRetry)
{
	uint8_t spi_format_buf[SPI_FORMAT_ARRAY_SIZE];

	/*0xF3 is head of command*/
	spi_format_buf[0] = 0xF3;
	spi_format_buf[1] = command;
	/*0x00 is tail of command*/
	spi_format_buf[2] = 0x00;

	return himax_spi_read(tdev,&spi_format_buf[0],
						SPI_FORMAT_ARRAY_SIZE,
						data, length, toRetry);
}

static int himax_bus_write(struct thp_device *tdev,uint8_t command, uint8_t *data, unsigned int length, uint8_t toRetry)
{
	uint8_t spi_format_buf[length + 2];
	int i;

	/*0xF2 is head of command*/
	spi_format_buf[0] = 0xF2;
	spi_format_buf[1] = command;

	for(i = 0;i < length;i++) {
		spi_format_buf[i + 2] = data[i];
	}

	return himax_spi_write(tdev,spi_format_buf,length + 2);
}

static int himax_register_read(struct thp_device *tdev,uint8_t *read_addr, unsigned int read_length, uint8_t *read_data)
{
	uint8_t tmp_data[HIMAX_NORMAL_DATA_LEN];
	/* Restore the address */
	int address = (read_addr[3] << 24) + (read_addr[2] << 16) + (read_addr[1] << 8) + read_addr[0];
	tmp_data[0] = (uint8_t)address;
	tmp_data[1] = (uint8_t)(address >> 8);
	tmp_data[2] = (uint8_t)(address >> 16);
	tmp_data[3] = (uint8_t)(address >> 24);

	if ( himax_bus_write(tdev,HIMAX_BEGING_ADDR,tmp_data,
						HIMAX_NORMAL_DATA_LEN, HIMAX_BUS_RETRY_TIMES) < 0){
			THP_LOG_ERR("%s: i2c access fail!\n", __func__);
			return -ENOMEM;
	}
	tmp_data[0] = HIMAX_REG_READ_EN;
	if ( himax_bus_write(tdev,HIMAX_REG_READ_EN_ADDR ,tmp_data, 1, HIMAX_BUS_RETRY_TIMES) < 0) {
			THP_LOG_ERR("%s: i2c access fail!\n", __func__);
			return -ENOMEM;
	}

	if ( himax_bus_read(tdev,HIMAX_WAKEUP_ADDR,read_data, read_length, HIMAX_BUS_RETRY_TIMES) < 0) {
		THP_LOG_ERR("%s: i2c access fail!\n", __func__);
		return -ENOMEM;
	}

	return 0;
}

static void himax_interface_on(struct thp_device *tdev)
{
	uint8_t tmp_data[HIMAX_NORMAL_DATA_LEN];
	/*Read a dummy register to wake up I2C.*/
	if ( himax_bus_read(tdev,HIMAX_WAKEUP_ADDR, tmp_data,HIMAX_NORMAL_DATA_LEN,
					HIMAX_BUS_RETRY_TIMES) < 0){/* to knock I2C*/
		THP_LOG_ERR("%s: i2c access fail!\n", __func__);
		return;
	}

}

int thp_hx83112_init(struct thp_device *tdev)
{
	int rc;
	unsigned int value = 0;
	struct thp_core_data *cd = tdev->thp_core;
	struct device_node *hx83112_node = of_get_child_by_name(cd->thp_node,
						THP_HX83112_DEV_NODE_NAME);
	struct himax_thp_private_data *himax_p = tdev->private_data;

	THP_LOG_INFO("Enter %s \n",__func__);

	hx_spi_dev = tdev->sdev;

	THP_LOG_INFO("%s: called\n", __func__);

	if (!hx83112_node) {
		THP_LOG_ERR("%s: hx83112 dev not config in dts\n", __func__);
		return -ENODEV;
	}

	rc = of_property_read_u32(hx83112_node, "get_frame_optimized_method", &value);
	if (rc) {
		himax_p->hx_get_frame_optimized_flag= 0;
		THP_LOG_ERR("%s:hx_get_frame_optimized_method_flag not found,use default value \n",__func__);
	}else{
		himax_p->hx_get_frame_optimized_flag= value;
		THP_LOG_INFO("%s:hx_get_frame_optimized_method_flag %d \n",__func__, value);
	}

	rc = thp_parse_spi_config(hx83112_node, cd);
	if (rc){
		THP_LOG_ERR("%s: spi config parse fail\n", __func__);
		return -ENODEV;
	}

	rc = thp_parse_timing_config(hx83112_node, &tdev->timing_config);
	if (rc){
		THP_LOG_ERR("%s: timing config parse fail\n", __func__);
		return -ENODEV;
	}

	return 0;
}

static int thp_hx83112_communication_check(
		struct thp_device *tdev)
{
	uint8_t tmp_addr[HIMAX_NORMAL_ADDR_LEN] = {0};
	uint8_t tmp_data[HIMAX_NORMAL_DATA_LEN] = {0};
	uint8_t ic_name[HIMAX_NORMAL_DATA_LEN] = {0};
	int ret = 0;
	int i = 0;
	int j = 0;

	for (i = 0; i < COMM_TEST_RW_RETRY_TIME; i++) {
		himax_assign_data(HIMAX_ICID_ADDR,tmp_addr);
		ret = himax_register_read(tdev,tmp_addr, COMM_TEST_RW_LENGTH, tmp_data);

		THP_LOG_INFO("%s:Read driver IC ID = %X,%X,%X\n", __func__, tmp_data[3],tmp_data[2],tmp_data[1]);
		for (j = 0; j < ARRAY_SIZE(himax_id_match_table); j++) {
			himax_assign_data(himax_id_match_table[j], ic_name);
			if ((tmp_data[3] == ic_name[3]) && (tmp_data[2] == ic_name[2]) && (tmp_data[1] == ic_name[1])) {
				break;
			}
		}
		if(j < ARRAY_SIZE(himax_id_match_table)) {
			THP_LOG_INFO("Himax IC found\n");
			break;
		}
		THP_LOG_ERR("%s:Read driver ID register Fail:\n", __func__);
	}

	if(i == COMM_TEST_RW_RETRY_TIME)
		return -EINVAL;

	return 0;
}

static void thp_hx_timing_work(struct thp_device *tdev)
{
	uint8_t tmp_data[HIMAX_NORMAL_DATA_LEN];

	THP_LOG_INFO("%s:called,do hard reset\n", __func__);
	gpio_direction_output(tdev->gpios->rst_gpio, GPIO_LOW);
	thp_do_time_delay(tdev->timing_config.boot_reset_low_delay_ms);

	gpio_set_value(tdev->gpios->rst_gpio, THP_RESET_HIGH);
	mdelay(tdev->timing_config.boot_reset_hi_delay_ms);
	tmp_data[0] = HIMAX_SSOFF_CMD_FIRST;
	if ( himax_bus_write(tdev,HIMAX_SSOFF_ADDR_FIRST,tmp_data, 1, HIMAX_BUS_RETRY_TIMES) < 0) {
		THP_LOG_ERR("%s: i2c first access fail!\n", __func__);
		return;
	}

	tmp_data[0] = HIMAX_SSOFF_CMD_SECOND;
	if ( himax_bus_write(tdev,HIMAX_SSOFF_ADDR_SECOND,tmp_data, 1, HIMAX_BUS_RETRY_TIMES) < 0) {
		THP_LOG_ERR("%s: i2c second access fail!\n", __func__);
		return;
	}

}


int thp_hx83112_chip_detect(struct thp_device *tdev)
{
	int ret = 0;
	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}
	thp_hx_timing_work(tdev);

	thp_bus_lock();
	ret =  thp_hx83112_communication_check(tdev);
	thp_bus_unlock();

	return ret;

}

static int himax_get_DSRAM_data(struct thp_device *tdev,char *info_data, unsigned int len)
{
	unsigned char tmp_addr[HIMAX_NORMAL_ADDR_LEN];
	unsigned int read_size = len;
	uint8_t *temp_info_data;
	int ret = 0;
	struct himax_thp_private_data *himax_p = tdev->private_data;

	temp_info_data = kzalloc(read_size,GFP_KERNEL);
	if (!temp_info_data) {
		THP_LOG_ERR("%s: temp_info_data malloc fail\n", __func__);
		return -ENOMEM;
	}
	thp_bus_lock();

	himax_assign_data(HIMAX_RAWDATA_ADDR, tmp_addr);
	if(himax_p->hx_get_frame_optimized_flag){
		if ( himax_bus_read(tdev,HIMAX_EVENT_STACK_CMD_ADDR,temp_info_data, read_size, HIMAX_BUS_RETRY_TIMES) < 0) {
			THP_LOG_ERR("%s: spi access fail!\n", __func__);
			ret = -ENOMEM;
		}
	}
	else{
		himax_interface_on(tdev);
		ret = himax_register_read(tdev,tmp_addr,read_size,temp_info_data);
	}

	thp_bus_unlock();

	memcpy(info_data, temp_info_data,read_size);
	kfree(temp_info_data);
	temp_info_data = NULL;

	return ret;
}

int thp_hx83112_get_frame(struct thp_device *tdev,
			char *buf, unsigned int len)
{
	if (!tdev) {
		THP_LOG_ERR("%s: input dev null\n", __func__);
		return -EINVAL;
	}

	if (!len) {
		THP_LOG_ERR("%s: read len illegal\n", __func__);
		return -EINVAL;
	}

	return himax_get_DSRAM_data(tdev,buf, len);

}

int thp_hx83112_resume(struct thp_device *tdev)
{
	uint8_t tmp_data[HIMAX_NORMAL_DATA_LEN];

	THP_LOG_DEBUG("%s: called_\n", __func__);

	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}

	gpio_set_value(tdev->gpios->cs_gpio, GPIO_HIGH);
	gpio_set_value(tdev->gpios->rst_gpio, THP_RESET_HIGH);
	mdelay(tdev->timing_config.resume_reset_after_delay_ms);

	tmp_data[0] = HIMAX_SSOFF_CMD_FIRST;

	thp_bus_lock();
	if ( himax_bus_write(tdev,HIMAX_SSOFF_ADDR_FIRST,tmp_data, 1, HIMAX_BUS_RETRY_TIMES) < 0) {
		THP_LOG_ERR("%s: i2c first access fail!\n", __func__);
			goto ERROR;
	}

	tmp_data[0] = HIMAX_SSOFF_CMD_SECOND;
	if ( himax_bus_write(tdev,HIMAX_SSOFF_ADDR_SECOND,tmp_data, 1, HIMAX_BUS_RETRY_TIMES) < 0) {
		THP_LOG_ERR("%s: i2c second access fail!\n", __func__);
			goto ERROR;
	}
	thp_bus_unlock();

	return 0;

ERROR:
	thp_bus_unlock();
	return -EIO;
}

int thp_hx83112_suspend(struct thp_device *tdev)
{
	THP_LOG_INFO("%s: called\n", __func__);

	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return -EINVAL;
	}

	gpio_set_value(tdev->gpios->cs_gpio, GPIO_LOW);
	gpio_direction_output(tdev->gpios->rst_gpio, GPIO_LOW);
	thp_do_time_delay(tdev->timing_config.suspend_reset_after_delay_ms);

	return 0;
}

void thp_hx83112_exit(struct thp_device *tdev)
{
	THP_LOG_INFO("%s: called\n", __func__);

	if (!tdev) {
		THP_LOG_ERR("%s: tdev null\n", __func__);
		return;
	}

	kfree(tdev->tx_buff);
	tdev->tx_buff = NULL;
	kfree(tdev->rx_buff);
	tdev->rx_buff = NULL;
	kfree(tdev);
	tdev = NULL;
}

static struct thp_device_ops hx83112_dev_ops = {
	.init = thp_hx83112_init,
	.detect = thp_hx83112_chip_detect,
	.get_frame = thp_hx83112_get_frame,
	.resume = thp_hx83112_resume,
	.suspend = thp_hx83112_suspend,
	.exit = thp_hx83112_exit,
};

static int __init thp_hx83112_module_init(void)
{
	int rc;
	struct thp_device *dev;

	THP_LOG_INFO("%s in\n", __func__ );

	dev = kzalloc(sizeof(struct thp_device), GFP_KERNEL);
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

	dev->ic_name = hx83112_IC_NAME;
	dev->dev_node_name = THP_HX83112_DEV_NODE_NAME;
	dev->ops = &hx83112_dev_ops;
	dev->private_data = (void *)&thp_private_data;

	rc = thp_register_dev(dev);
	if (rc) {
		THP_LOG_ERR("%s: register fail\n", __func__);
		goto err;
	}

	return rc;
err:
	kfree(dev->tx_buff);
	dev->tx_buff = NULL;
	kfree(dev->rx_buff);
	dev->rx_buff = NULL;
	kfree(dev);
	dev = NULL;
	return rc;
}
static void __exit thp_hx83112_module_exit(void)
{
	THP_LOG_INFO("%s: called \n", __func__);
};

module_init(thp_hx83112_module_init);
module_exit(thp_hx83112_module_exit);
