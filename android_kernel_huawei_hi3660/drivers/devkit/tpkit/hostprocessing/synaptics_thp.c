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
#include <linux/string.h>
#include "huawei_thp.h"

#include <linux/time.h>
#include <linux/syscalls.h>

#define SYNAPTICS_IC_NAME "synaptics"
#define THP_SYNA_DEV_NODE_NAME "synaptics_thp"
#define SYNA_FRAME_SIZE 1092

#define MESSAGE_HEADER_SIZE 4
#define MESSAGE_MARKER 0xa5
#define MESSAGE_PADDING 0x5a
#define FRAME_LENGTH   (2*18*30)

#define FIRST_FRAME_USEFULL_LEN   2


enum status_code {
	STATUS_IDLE = 0x00,
	STATUS_OK = 0x01,
	STATUS_BUSY = 0x02,
	STATUS_CONTINUED_READ = 0x03,
	STATUS_RECEIVE_BUFFER_OVERFLOW = 0x0c,
	STATUS_PREVIOUS_COMMAND_PENDING = 0x0d,
	STATUS_NOT_IMPLEMENTED = 0x0e,
	STATUS_ERROR = 0x0f,
	STATUS_INVALID = 0xff,
};

enum report_type {
	REPORT_IDENTIFY = 0x10,
	REPORT_TOUCH = 0x11,
	REPORT_DELTA = 0x12,
	REPORT_RAW = 0x13,
	REPORT_PRINTF = 0x82,
	REPORT_STATUS = 0x83,
	REPORT_FRAME = 0xC0,
	REPORT_HDL = 0xfe,
};
static unsigned char *buf;

static struct spi_transfer *xfer;

static int syna_tcm_spi_alloc_mem(struct spi_device *client,
		unsigned int count, unsigned int size)
{
	static unsigned int buf_size = 0;
	static unsigned int xfer_count = 0;

	if (count > xfer_count) {
		kfree(xfer);
		xfer = kcalloc(count, sizeof(*xfer), GFP_KERNEL);
		if (!xfer) {
			THP_LOG_ERR("Failed to allocate memory for xfer\n");
			xfer_count = 0;
			return -ENOMEM;
		}
		xfer_count = count;
	} else {
		memset(xfer, 0, count * sizeof(*xfer));
	}

	if (size > buf_size) {
		if (buf_size)
			kfree(buf);
		buf = kmalloc(size, GFP_KERNEL);
		if (!buf) {
			THP_LOG_ERR("Failed to allocate memory for buf\n");
			buf_size = 0;
			return -ENOMEM;
		}
		buf_size = size;
	}

	return 0;
}

static int syna_tcm_spi_read(struct spi_device *client, unsigned char *data,
		unsigned int length)
{
	int retval;
	struct spi_message msg;
	struct thp_core_data *cd = spi_get_drvdata(client);

	spi_message_init(&msg);

	retval = syna_tcm_spi_alloc_mem(client, 1, length);
	if (retval < 0) {
		THP_LOG_ERR("Failed to allocate memory\n");
		goto exit;
	}

	memset(buf, 0xff, length);
	xfer[0].len = length;
	xfer[0].tx_buf = buf;
	xfer[0].rx_buf = data;
	spi_message_add_tail(&xfer[0], &msg);
	//thp_spi_cs_set(GPIO_HIGH);
	retval = thp_spi_sync(client, &msg);
	if (retval == 0) {
		retval = length;
	} else {
		THP_LOG_ERR("Failed to complete SPI transfer, error = %d\n",
				retval);
	}

exit:


	return retval;
}

static int syna_tcm_spi_write(struct spi_device *client, unsigned char *data,
		unsigned int length)
{
	int retval;
	struct spi_message msg;
	struct thp_core_data *cd = spi_get_drvdata(client);

	spi_message_init(&msg);

	retval = syna_tcm_spi_alloc_mem(client, 1, 0);
	if (retval < 0) {
		THP_LOG_ERR("Failed to allocate memory\n");
		goto exit;
	}

	xfer[0].len = length;
	xfer[0].tx_buf = data;
	spi_message_add_tail(&xfer[0], &msg);
	//thp_spi_cs_set(GPIO_HIGH);
	retval = thp_spi_sync(client, &msg);
	if (retval == 0) {
		retval = length;
	} else {
		THP_LOG_ERR("Failed to complete SPI transfer, error = %d\n",
				retval);
	}

exit:


	return retval;
}

static int thp_synaptics_init(struct thp_device *tdev)
{
	int rc;
	struct thp_core_data *cd = tdev->thp_core;
	struct device_node *syna_node = of_get_child_by_name(cd->thp_node,
						THP_SYNA_DEV_NODE_NAME);

	THP_LOG_INFO("%s: called\n", __func__);

	if (!syna_node) {
		THP_LOG_INFO("%s: syna dev not config in dts\n", __func__);
		return -ENODEV;
	}

	rc = thp_parse_spi_config(syna_node, cd);
	if (rc)
		THP_LOG_ERR("%s: spi config parse fail\n", __func__);

	rc = thp_parse_timing_config(syna_node, &tdev->timing_config);
	if (rc)
		THP_LOG_ERR("%s: timing config parse fail\n", __func__);

	rc = thp_parse_feature_config(syna_node, cd);
	if (rc)
		THP_LOG_ERR("%s: feature_config fail\n", __func__);

	return 0;
}

static int thp_synaptics_chip_detect(struct thp_device *tdev)
{
	unsigned char rmiaddr[2] = {0x80, 0xEE};
	unsigned char fnnum = 0;

	THP_LOG_INFO("%s: called\n", __func__);
	thp_bus_lock();
	syna_tcm_spi_write(tdev->thp_core->sdev, rmiaddr, sizeof(rmiaddr));
	syna_tcm_spi_read(tdev->thp_core->sdev, &fnnum, sizeof(fnnum));
	thp_bus_unlock();
	if (fnnum != 0x35) {
		THP_LOG_ERR("%s: fnnum error: 0x%02x\n", __func__, fnnum);
		return -ENODEV;
	}
	THP_LOG_ERR("%s: fnnum error: 0x%02x\n", __func__, fnnum);
	return 0;
}

static int thp_synaptics_get_frame(struct thp_device *tdev,
			char *frame_buf, unsigned int len)
{
	unsigned char data[4] = {0};
	unsigned int length = 0;
	int retval;

	thp_bus_lock();
	retval = syna_tcm_spi_read(tdev->thp_core->sdev, data, sizeof(data));  // read length
	if (retval < 0) {
		THP_LOG_ERR("%s: Failed to read length\n", __func__);
		goto ERROR;
	}
	if(data[1]==0xFF){
		THP_LOG_ERR("%s: should ignore this irq.\n", __func__);
		 retval = -ENODATA;
		goto ERROR;
	}
	if (data[0] != MESSAGE_MARKER) {
		THP_LOG_ERR("%s: incorrect marker: 0x%02x\n", __func__, data[0]);
		if (data[1] == STATUS_CONTINUED_READ) {
			// just in case
			THP_LOG_ERR("%s: continued Read\n", __func__);
			syna_tcm_spi_read(tdev->thp_core->sdev, tdev->rx_buff, THP_MAX_FRAME_SIZE); //  drop one transaction
		}
		retval = -ENODATA;
		goto ERROR;
	}

	length = (data[3] << 8) | data[2];
	if(length > (THP_MAX_FRAME_SIZE -FIRST_FRAME_USEFULL_LEN)){
		THP_LOG_INFO("%s: out of length.\n", __func__);
		length = THP_MAX_FRAME_SIZE -FIRST_FRAME_USEFULL_LEN;
	}
	if (length) {
		retval = syna_tcm_spi_read(tdev->thp_core->sdev, frame_buf + FIRST_FRAME_USEFULL_LEN, length + FIRST_FRAME_USEFULL_LEN); //read packet
		if (retval < 0) {
			THP_LOG_ERR("%s: Failed to read length\n", __func__);
			goto ERROR;
		}
	}
	thp_bus_unlock();
	memcpy(frame_buf, data, sizeof(data));
	return 0;

ERROR:
	thp_bus_unlock();
	return retval;
}

static int thp_synaptics_resume(struct thp_device *tdev)
{
	THP_LOG_INFO("%s: called\n", __func__);
	gpio_set_value(tdev->gpios->cs_gpio, 1);
	gpio_set_value(tdev->gpios->rst_gpio, 1);//keep TP rst  high before LCD  reset hign

	return 0;
}

static int thp_synaptics_suspend(struct thp_device *tdev)
{
	THP_LOG_INFO("%s: called\n", __func__);
	gpio_set_value(tdev->gpios->rst_gpio,0);
	gpio_set_value(tdev->gpios->cs_gpio, 0);

	return 0;
}

static void thp_synaptics_exit(struct thp_device *tdev)
{
	THP_LOG_INFO("%s: called\n", __func__);
	if(tdev){
		if(tdev->tx_buff){
			kfree(tdev->tx_buff);
			tdev->tx_buff = NULL;
		}
		if(tdev->rx_buff){
			kfree(tdev->rx_buff);
			tdev->rx_buff = NULL;
		}
		kfree(tdev);
		tdev = NULL;
	}
	if(buf){
		kfree(buf);
		buf = NULL;
	}
	if(xfer){
		kfree(xfer);
		xfer = NULL;
	}
}

struct thp_device_ops syna_dev_ops = {
	.init = thp_synaptics_init,
	.detect = thp_synaptics_chip_detect,
	.get_frame = thp_synaptics_get_frame,
	.resume = thp_synaptics_resume,
	.suspend = thp_synaptics_suspend,
	.exit = thp_synaptics_exit,
};

static int __init thp_synaptics_module_init(void)
{
	int rc;
	struct thp_device *dev = kzalloc(sizeof(struct thp_device), GFP_KERNEL);

	THP_LOG_INFO("%s: called \n", __func__);
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
	buf = NULL;
	xfer = NULL;
	dev->ic_name = SYNAPTICS_IC_NAME;
	dev->dev_node_name = THP_SYNA_DEV_NODE_NAME;
	dev->ops = &syna_dev_ops;

	rc = thp_register_dev(dev);
	if (rc) {
		THP_LOG_ERR("%s: register fail\n", __func__);
		goto err;
	} else
	THP_LOG_INFO("%s: register success\n", __func__);

	return rc;
err:
	if(dev){
		if(dev->tx_buff){
			kfree(dev->tx_buff);
			dev->tx_buff = NULL;
		}
		if(dev->rx_buff){
			kfree(dev->rx_buff);
			dev->rx_buff = NULL;
		}
		kfree(dev);
		dev = NULL;
	}
	return rc;
}
static void __exit thp_synaptics_module_exit(void)
{
	THP_LOG_ERR("%s: called \n", __func__);
};

module_init(thp_synaptics_module_init);
module_exit(thp_synaptics_module_exit);

