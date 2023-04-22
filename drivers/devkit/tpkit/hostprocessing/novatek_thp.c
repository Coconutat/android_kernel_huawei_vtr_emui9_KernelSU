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
#include "huawei_thp.h"


#define NOVATECH_IC_NAME "novatech"
#define THP_NOVA_DEV_NODE_NAME "novatech"

#define SPI_WRITE_MASK(a)	(a | 0x80)
#define SPI_READ_MASK(a)	(a & 0x7F)

#define NVT_LOW_WORD_BIT_MASK(x) (uint8_t)((x >> 7) & 0x000000FF)
#define NVT_HI_WORD_BIT_MASK(x) (uint8_t)((x >> 15) & 0x000000FF)
#define NVT_ID_BYTE_MAX 6
#define NVT_SW_RESET_RW_LEN 4
#define NVT_SW_RESET_REG_ADDR 0x3F0FE
#define NVT_SW_RESET_DELAY_MS 10
#define NVT_CHIP_VER_TRIM_RW_LEN 16
#define NVT_CHIP_VER_TRIM_REG_ADDR 0x1F64E
#define NVT_CHIP_VER_TRIM_RETRY_TIME 5

struct nvt_ts_trim_id_table_entry {
	uint8_t id[NVT_ID_BYTE_MAX];
	uint8_t mask[NVT_ID_BYTE_MAX];
};

static const struct nvt_ts_trim_id_table_entry trim_id_table[] = {
	{.id = {0x0A, 0xFF, 0xFF, 0x72, 0x67, 0x03}, .mask = {1, 0, 0, 1, 1, 1} },
	{.id = {0x0A, 0xFF, 0xFF, 0x82, 0x66, 0x03}, .mask = {1, 0, 0, 1, 1, 1} },
	{.id = {0x0B, 0xFF, 0xFF, 0x82, 0x66, 0x03}, .mask = {1, 0, 0, 1, 1, 1} },
	{.id = {0x0A, 0xFF, 0xFF, 0x72, 0x66, 0x03}, .mask = {1, 0, 0, 1, 1, 1} },
	{.id = {0x55, 0x00, 0xFF, 0x00, 0x00, 0x00}, .mask = {1, 1, 0, 1, 1, 1} },
	{.id = {0x55, 0x72, 0xFF, 0x00, 0x00, 0x00}, .mask = {1, 1, 0, 1, 1, 1} },
	{.id = {0xAA, 0x00, 0xFF, 0x00, 0x00, 0x00}, .mask = {1, 1, 0, 1, 1, 1} },
	{.id = {0xAA, 0x72, 0xFF, 0x00, 0x00, 0x00}, .mask = {1, 1, 0, 1, 1, 1} },
	{.id = {0xFF, 0xFF, 0xFF, 0x72, 0x67, 0x03}, .mask = {0, 0, 0, 1, 1, 1} },
	{.id = {0xFF, 0xFF, 0xFF, 0x70, 0x66, 0x03}, .mask = {0, 0, 0, 1, 1, 1} },
	{.id = {0xFF, 0xFF, 0xFF, 0x70, 0x67, 0x03}, .mask = {0, 0, 0, 1, 1, 1} },
	{.id = {0xFF, 0xFF, 0xFF, 0x72, 0x66, 0x03}, .mask = {0, 0, 0, 1, 1, 1} },
	{.id = {0xFF, 0xFF, 0xFF, 0x25, 0x65, 0x03}, .mask = {0, 0, 0, 1, 1, 1} },
	{.id = {0xFF, 0xFF, 0xFF, 0x70, 0x68, 0x03}, .mask = {0, 0, 0, 1, 1, 1} }
};

static inline int thp_nvt_spi_read_write(struct spi_device *client,
			void *tx_buf, void *rx_buf, size_t len)
{
	struct spi_transfer t = {
		.tx_buf = tx_buf,
		.rx_buf = rx_buf,
		.len    = len,
	};
	struct spi_message m;
	struct thp_core_data *cd = spi_get_drvdata(client);
	struct thp_device *tdev = cd->thp_dev;
	int rc;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	thp_bus_lock();
	thp_spi_cs_set(GPIO_HIGH);
	rc = thp_spi_sync(client, &m);
	thp_bus_unlock();

	return rc;
}

static int thp_nvt_check_spi_master_capacity(struct spi_device *sdev)
{
	if (sdev->master->flags & SPI_MASTER_HALF_DUPLEX) {
		THP_LOG_ERR("%s: Full duplex not supported by master\n", __func__);
		return -EIO;
	} else {
		return 0;
	}
}

void thp_nvt_sw_reset_idle(struct spi_device *sdev)
{
	uint8_t w_buf[NVT_SW_RESET_RW_LEN] = {0};
	/*This register addr is for NT36870 */
	uint32_t addr = NVT_SW_RESET_REG_ADDR;
	unsigned int rw_len;

	/* write 0xAA @0x3F0FE */
	w_buf[0] = SPI_WRITE_MASK(0x7F);
	w_buf[1] = NVT_HI_WORD_BIT_MASK(addr);
	w_buf[2] = NVT_LOW_WORD_BIT_MASK(addr);
	rw_len = 3;

	thp_nvt_spi_read_write(sdev, w_buf, NULL, rw_len);

	w_buf[0] = (uint8_t)(SPI_WRITE_MASK((addr & 0x0000007F)));
	w_buf[1] = 0xAA;
	rw_len = 2;
	thp_nvt_spi_read_write(sdev, w_buf, NULL, rw_len);

	msleep(NVT_SW_RESET_DELAY_MS);
}

void thp_nvt_bootloader_reset(struct spi_device *sdev)
{
	uint8_t w_buf[NVT_SW_RESET_RW_LEN] = {0};
	/*This register addr is for NT36870 */
	uint32_t addr = NVT_SW_RESET_REG_ADDR;
	unsigned int rw_len;

	THP_LOG_INFO("%s: called\n", __func__);
	/* write 0xAA @0x3F0FE */
	w_buf[0] = SPI_WRITE_MASK(0x7F);
	w_buf[1] = NVT_HI_WORD_BIT_MASK(addr);
	w_buf[2] = NVT_LOW_WORD_BIT_MASK(addr);
	rw_len = 3;

	thp_nvt_spi_read_write(sdev, w_buf, NULL, rw_len);

	w_buf[0] = (uint8_t)(SPI_WRITE_MASK((addr & 0x0000007F)));
	w_buf[1] = 0x69;/* bootloader reset cmd */
	rw_len = 2;
	thp_nvt_spi_read_write(sdev, w_buf, NULL, rw_len);

	msleep(NVT_SW_RESET_DELAY_MS);
}

static int thp_nvt_check_ver_trim_table(uint8_t *r_buf)
{
	int i;
	int list;

	THP_LOG_INFO("%s:rbuf:0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n",
			__func__, r_buf[0], r_buf[1], r_buf[2], r_buf[3],
			r_buf[4], r_buf[5], r_buf[6], r_buf[7]);

	for (list = 0; list < ARRAY_SIZE(trim_id_table); list++) {
		for (i = 0; i < NVT_ID_BYTE_MAX; i++) {
			if (trim_id_table[list].mask[i] &&
				r_buf[i + 2] != trim_id_table[list].id[i])
				break;
		}

		if (i == NVT_ID_BYTE_MAX) {
			THP_LOG_INFO("This is NVT touch IC\n");
			return 0;
		}
	}

	return -ENODEV;
}

static void thp_nvt_timing_work(struct thp_device *tdev)
{

	THP_LOG_ERR("%s:called,do sw reset idle in hard reset\n", __func__);

	gpio_direction_output(tdev->gpios->rst_gpio, GPIO_LOW);
	thp_do_time_delay(tdev->timing_config.boot_reset_low_delay_ms);

	thp_nvt_sw_reset_idle(tdev->sdev);

	gpio_set_value(tdev->gpios->rst_gpio, THP_RESET_HIGH);
	thp_do_time_delay(tdev->timing_config.boot_reset_hi_delay_ms);
}

static int8_t thp_nvt_ts_check_chip_ver_trim(struct spi_device *sdev)
{
	uint8_t w_buf[NVT_CHIP_VER_TRIM_RW_LEN] = {0};
	uint8_t r_buf[NVT_CHIP_VER_TRIM_RW_LEN] = {0};
	uint32_t addr = NVT_CHIP_VER_TRIM_REG_ADDR;
	int rw_len;
	int32_t retry;

	/* Check for 5 times */
	for (retry = NVT_CHIP_VER_TRIM_RETRY_TIME; retry > 0; retry--) {
		thp_nvt_bootloader_reset(sdev);
		thp_nvt_sw_reset_idle(sdev);
		/* read chip id @0x1F64E */
		w_buf[0] = SPI_WRITE_MASK(0x7F);
		w_buf[1] = NVT_HI_WORD_BIT_MASK(addr);
		w_buf[2] = NVT_LOW_WORD_BIT_MASK(addr);
		rw_len = 3;
		thp_nvt_spi_read_write(sdev, w_buf, NULL, rw_len);

		memset(w_buf, 0, NVT_CHIP_VER_TRIM_RW_LEN);
		w_buf[0] = (uint8_t)(SPI_READ_MASK(addr));
		rw_len = 8;
		thp_nvt_spi_read_write(sdev, w_buf, r_buf, rw_len);

		if (!thp_nvt_check_ver_trim_table(r_buf))
			return 0;

		msleep(10);
	}

	return -ENODEV;
}

static int thp_novatech_init(struct thp_device *tdev)
{
	int rc;
	struct thp_core_data *cd = tdev->thp_core;
	struct device_node *nova_node = of_get_child_by_name(cd->thp_node,
						THP_NOVA_DEV_NODE_NAME);

	THP_LOG_INFO("%s: called\n", __func__);

	if (!nova_node) {
		THP_LOG_INFO("%s: nova dev not config in dts\n", __func__);
		return -ENODEV;
	}

	rc = thp_nvt_check_spi_master_capacity(tdev->sdev);
	if (rc) {
		THP_LOG_ERR("%s: spi capacity check fail\n", __func__);
		return rc;
	}

	rc = thp_parse_spi_config(nova_node, cd);
	if (rc)
		THP_LOG_ERR("%s: spi config parse fail\n", __func__);

	rc = thp_parse_timing_config(nova_node, &tdev->timing_config);
	if (rc)
		THP_LOG_ERR("%s: timing config parse fail\n", __func__);

	return 0;
}

static int thp_novatech_chip_detect(struct thp_device *tdev)
{

	int rc;

	thp_nvt_timing_work(tdev);

	rc =  thp_nvt_ts_check_chip_ver_trim(tdev->thp_core->sdev);
	if (rc) {
		THP_LOG_INFO("%s:chip is not identified\n", __func__);
		return -ENODEV;
	}

	return 0;
}
static int thp_novatech_get_frame(struct thp_device *tdev,
			char *buf, unsigned int len)
{
	unsigned char get_frame_cmd = 0x20; /* read frame data command */
	uint8_t *w_buf;

	if (!tdev) {
		THP_LOG_INFO("%s: input dev null\n", __func__);
		return -ENOMEM;
	}

	if (!len) {
		THP_LOG_INFO("%s: read len illegal\n", __func__);
		return -ENOMEM;
	}

	w_buf = tdev->tx_buff;

	memset(tdev->tx_buff, 0, THP_MAX_FRAME_SIZE);
	/* write cmd 0x20 and get frame */
	w_buf[0] = (uint8_t)(SPI_READ_MASK(get_frame_cmd));
	thp_nvt_spi_read_write(tdev->thp_core->sdev, w_buf, buf, len);

	return 0;
}

static int thp_novatech_resume(struct thp_device *tdev)
{
	THP_LOG_INFO("%s: called\n", __func__);
	gpio_set_value(tdev->gpios->cs_gpio, 1);
	thp_nvt_sw_reset_idle(tdev->sdev);
	gpio_set_value(tdev->gpios->rst_gpio, 1);
	thp_do_time_delay(tdev->timing_config.resume_reset_after_delay_ms);

	return 0;
}

static int thp_novatech_suspend(struct thp_device *tdev)
{
	THP_LOG_INFO("%s: called\n", __func__);
	int pt_test_mode = 0;

	pt_test_mode = is_pt_test_mode(tdev);

	if (pt_test_mode) {
		THP_LOG_INFO("%s: sleep mode\n", __func__);
		gpio_set_value(tdev->gpios->cs_gpio, 0);
	} else {
		THP_LOG_INFO("%s: power off mode\n", __func__);
		gpio_set_value(tdev->gpios->rst_gpio, 0);
		gpio_set_value(tdev->gpios->cs_gpio, 0);
	}

	thp_do_time_delay(tdev->timing_config.suspend_reset_after_delay_ms);

	return 0;
}

static void thp_novatech_exit(struct thp_device *tdev)
{
	THP_LOG_INFO("%s: called\n", __func__);
	kfree(tdev->tx_buff);
	kfree(tdev->rx_buff);
	kfree(tdev);
}

struct thp_device_ops nova_dev_ops = {
	.init = thp_novatech_init,
	.detect = thp_novatech_chip_detect,
	.get_frame = thp_novatech_get_frame,
	.resume = thp_novatech_resume,
	.suspend = thp_novatech_suspend,
	.exit = thp_novatech_exit,
};

static int __init thp_novatech_module_init(void)
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

	dev->ic_name = NOVATECH_IC_NAME;
	dev->ops = &nova_dev_ops;

	rc = thp_register_dev(dev);
	if (rc) {
		THP_LOG_ERR("%s: register fail\n", __func__);
		goto err;
	}

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
static void __exit thp_novatech_module_exit(void)
{
	THP_LOG_INFO("%s: called \n", __func__);
};

module_init(thp_novatech_module_init);
module_exit(thp_novatech_module_exit);
MODULE_AUTHOR("novatech");
MODULE_DESCRIPTION("novatech driver");
MODULE_LICENSE("GPL");