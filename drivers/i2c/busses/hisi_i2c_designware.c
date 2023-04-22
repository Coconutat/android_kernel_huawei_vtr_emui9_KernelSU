/*
 * Hisilicon Synopsys DesignWare I2C adapter driver (master only).
 *
 * Copyright (c) 2012-2013 Linaro Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include "i2c-designware-core.h"
#include <linux/version.h>
#include <linux/clk.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
#include <linux/of_i2c.h>
#endif

/*lint -e750 -esym(750,*) */
#define HISI_DW_IC_CON			0x0
#define HISI_DW_IC_TAR			0x4
#define HISI_DW_IC_ENABLE		0x6c
#define HISI_DW_IC_STATUS		0x70
#define HISI_DW_IC_TXFLR		0x74
#define HISI_DW_IC_RXFLR		0x78
#define DW_IC_COMP_PARAM_1		0xf4
/*lint -e750 +esym(750,*) */
#define DW_IC_DMA_CR			0x88
#define DW_IC_DMA_TDLR	    		0x8c
#define DW_IC_DMA_RDLR      		0x90

/*lint -e750 -esym(750,*) */
#define HISI_DW_IC_TX_ABRT_SOURCE	0x80
/*lint -e750 +esym(750,*) */
#define HISI_DW_IC_DATA_CMD		0x10
/*lint -e750 -esym(750,*) */
#define HISI_DW_IC_ERR_TX_ABRT		0x1
/*lint -e750 +esym(750,*) */

#define DW_IC_TXDMAE			(1 << 1)	/* enable transmit dma */
#define DW_IC_RXDMAE			(1 << 0)	/* enable receive dma */
/*lint -e750 -esym(750,*) */
#define HISI_DW_IC_RD_CFG		(1 << 8)
#define HISI_DW_IC_CON_RECOVERY_CFG	(0x43)
#define HISI_DW_IC_TAR_RECOVERY_CFG	(0x7f)
/*lint -e750 +esym(750,*) */

#define HISI_STATUS_WRITE_IN_PROGRESS	0x1

#define I2C_DW_MAX_DMA_BUF_LEN	(60*1024)

#define I2C_DELAY_70NS                  70
/*lint -e750 -esym(750,*) */
#define I2C_DELAY_300NS                 300
/*lint -e750 +esym(750,*) */

#define GET_DEV_LOCK_TIMEOUT  500

struct i2c_adapter *device_adap_addr = NULL;

static u32 hs_i2c_dw_get_clk_rate_khz(struct dw_i2c_dev *dev)
{
	u32 rate;

	rate = clk_get_rate(dev->clk) /1000;
	dev_dbg(dev->dev, "input_clock_khz value is %d\n", rate);

	return rate;
}

int dw_hisi_pins_ctrl(struct dw_i2c_dev *dev, const char *name)
{
	struct dw_hisi_controller *controller;
	struct pinctrl_state *s;
	int ret;

	controller = dev->priv_data;
	if (!controller) {
		dev_err(dev->dev, "%s: i2c contrller do not be init.\n", __func__);
		return -ENODEV;
	}

	if (0 == controller->pinctrl_flag) {
		controller->pinctrl = devm_pinctrl_get(dev->dev);
		if (IS_ERR(controller->pinctrl))
			return -1;
		controller->pinctrl_flag = 1;
	}

	s = pinctrl_lookup_state(controller->pinctrl, name);
	if (IS_ERR(s)) {
		devm_pinctrl_put(controller->pinctrl);
		controller->pinctrl_flag = 0;
		return -1;
	}

	ret = pinctrl_select_state(controller->pinctrl, s);
	if (ret < 0) {
		devm_pinctrl_put(controller->pinctrl);
		controller->pinctrl_flag = 0;
		return -1;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(dw_hisi_pins_ctrl);

void hs_i2c_dw_reset_controller(struct dw_i2c_dev *dev)
{
	struct dw_hisi_controller *controller = dev->priv_data;
	struct hs_i2c_priv_data *priv;
	u32 val = 0, timeout = 10;

	if (!controller) {
		dev_err(dev->dev, "%s: i2c contrller do not be init.\n", __func__);
		return;
	}

	priv = &controller->priv;

	writel(BIT(priv->reset_bit), controller->reset_reg_base + priv->reset_enable_off);
	do {
		val = readl(controller->reset_reg_base + priv->reset_status_off);
		val &= BIT(priv->reset_bit);
		udelay(1);
	} while (!val && timeout--);

	timeout = 10;

	writel(BIT(priv->reset_bit), controller->reset_reg_base + priv->reset_disable_off);
	do {
		val = readl(controller->reset_reg_base + priv->reset_status_off);
		val &= BIT(priv->reset_bit);
		udelay(1);
	} while (val && timeout--);

	return;
}


/* reset i2c controller */
void reset_i2c_controller(struct dw_i2c_dev *dev)
{
	struct dw_hisi_controller *controller = dev->priv_data;
	int r;

	if (!controller) {
		dev_err(dev->dev, "%s: i2c contrller do not be init.\n", __func__);
		return;
	}

	r = dw_hisi_pins_ctrl(dev, PINCTRL_STATE_IDLE);
	if (r < 0)
		dev_warn(dev->dev,
				 "pins are not configured from the driver\n");

	hs_i2c_dw_reset_controller(dev);

	i2c_dw_init(dev);

	i2c_dw_disable_int(dev);

	r = dw_hisi_pins_ctrl(dev, PINCTRL_STATE_DEFAULT);
	if (r < 0)
		dev_warn(dev->dev,
				 "pins are not configured from the driver\n");
}

#ifdef CONFIG_DMA_ENGINE
static void i2c_dw_dma_probe_initcall(struct dw_i2c_dev *dev)
{
	struct dma_chan *chan;
	dma_cap_mask_t mask;
	struct dma_slave_config tx_conf = {};
	struct dma_slave_config rx_conf = {};

	struct dw_hisi_controller *controller = dev->priv_data;

	if (!controller) {
		dev_err(dev->dev, "%s: i2c contrller do not be init.\n", __func__);
		return;
	}

	/* DMA is the sole user of the platform data right now */
	tx_conf.dst_addr = controller->mapbase + HISI_DW_IC_DATA_CMD;
	tx_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
	tx_conf.direction = DMA_TO_DEVICE;/*lint !e64*/
	tx_conf.dst_maxburst = 16;

	rx_conf.src_addr = controller->mapbase + HISI_DW_IC_DATA_CMD;
	rx_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	rx_conf.direction = DMA_FROM_DEVICE;/*lint !e64*/
	rx_conf.src_maxburst = 16;

	/* Try to acquire a generic DMA engine slave TX channel */
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	chan = dma_request_slave_channel(dev->dev, "tx");
	if (!chan) {
		dev_err(dev->dev, "no TX DMA channel!\n");
		return;
	}

	dmaengine_slave_config(chan, &tx_conf);
	controller->dmatx.chan = chan;

	dev_info(dev->dev, "DMA channel TX %s-%d\n",
			 dma_chan_name(controller->dmatx.chan),
			 controller->dmatx.chan->chan_id);

	chan = dma_request_slave_channel(dev->dev, "rx");
	if (!chan) {
		dev_err(dev->dev, "no RX DMA channel!\n");
		return;
	}

	dmaengine_slave_config(chan, &rx_conf);
	controller->dmarx.chan = chan;

	dev_info(dev->dev, "DMA channel RX %s-%d\n",
			 dma_chan_name(controller->dmarx.chan),
			 controller->dmarx.chan->chan_id);
}

static void i2c_dw_dma_probe(struct dw_i2c_dev *dev)
{
	i2c_dw_dma_probe_initcall(dev);
}

static void i2c_dw_dma_remove(struct dw_i2c_dev *dev)
{
	struct dw_hisi_controller *controller = dev->priv_data;

	if (!controller) {
		dev_err(dev->dev, "%s: i2c contrller does not be init.\n", __func__);
		return;
	}

	/* TODO: remove the initcall if it has not yet executed */
	if (controller->dmatx.chan)
		dma_release_channel(controller->dmatx.chan);
	if (controller->dmarx.chan)
		dma_release_channel(controller->dmarx.chan);
}

/*
 * The current DMA TX buffer has been sent.
 * Try to queue up another DMA buffer.
 */
static void i2c_dw_dma_tx_callback(void *data)
{
	struct dw_i2c_dev *dev = data;
	struct dw_hisi_controller *controller = dev->priv_data;

	if (!controller) {
		dev_err(dev->dev, "%s: i2c contrller does not be init.\n", __func__);
		return;
	}

	dev_dbg(dev->dev, "%s: entry.\n", __func__);

	controller->dmacr &= ~DW_IC_TXDMAE;
	dw_writel(dev, controller->dmacr & 0xffff, DW_IC_DMA_CR);/*lint !e144*/
	controller->using_tx_dma = false;

	if (!(controller->using_tx_dma) && !(controller->using_rx_dma))
		complete(&controller->dma_complete);
}

static int i2c_dw_dma_tx_refill(struct dw_i2c_dev *dev)
{
	struct dw_i2c_dma_data *dmatx;
	struct dma_chan *chan;
	struct dma_device *dma_dev;
	struct dma_async_tx_descriptor *desc;
	struct dw_hisi_controller *controller = dev->priv_data;

	if (!controller) {
		dev_err(dev->dev, "%s: i2c contrller does not be init.\n", __func__);
		return -ENODEV;
	}

	dmatx = &controller->dmatx;
	if (!dmatx) {
		dev_err(dev->dev, "dmatx is NULL!\n");
		return -EIO;
	}

	chan = dmatx->chan;
	if (!chan) {
		dev_err(dev->dev, "chan is NULL!\n");
		return -EIO;
	}

	dma_dev = chan->device;

	if (dma_map_sg(dma_dev->dev, &dmatx->sg, 1, DMA_TO_DEVICE) != 1) {
		dev_warn(dev->dev, "unable to map TX DMA\n");
		return -EBUSY;
	}

	desc = dmaengine_prep_slave_sg(chan, &dmatx->sg, 1, DMA_TO_DEVICE,/*lint !e64*/
					DMA_PREP_INTERRUPT | DMA_CTRL_ACK);/*lint !e655*/
	if (!desc) {
		dma_unmap_sg(dma_dev->dev, &dmatx->sg, 1, DMA_TO_DEVICE);
		dev_warn(dev->dev, "TX DMA busy\n");
		return -EBUSY;
	}

	/* Some data to go along to the callback */
	desc->callback = i2c_dw_dma_tx_callback;
	desc->callback_param = dev;

	desc->tx_submit(desc);

	/* Fire the DMA transaction */
	dma_dev->device_issue_pending(chan);

	return 1;
}

static void i2c_dw_dma_rx_callback(void *data)
{
	struct dw_i2c_dev *dev = data;
	struct dw_hisi_controller *controller = dev->priv_data;
	struct i2c_msg *msgs;
	struct dw_i2c_dma_data *dmarx;
	int rx_valid;
	int rd_idx = 0;
	u32 len;
	u8 *buf;

	dev_dbg(dev->dev, "%s: entry.\n", __func__);

	if (!controller) {
		dev_err(dev->dev, "%s: i2c contrller does not be init.\n", __func__);
		return;
	}


	msgs = dev->msgs;
	dmarx = &controller->dmarx;
	rx_valid = dmarx->sg.length;

	/* Sync in buffer */
	dma_sync_sg_for_cpu(dev->dev, &dmarx->sg, 1, DMA_FROM_DEVICE);

	controller->dmacr &= ~DW_IC_RXDMAE;
	dw_writel(dev, controller->dmacr & 0xffff, DW_IC_DMA_CR);/*lint !e144*/

	for (; dev->msg_read_idx < dev->msgs_num; dev->msg_read_idx++) {
		if (!(msgs[dev->msg_read_idx].flags & I2C_M_RD))
			continue;

		len = msgs[dev->msg_read_idx].len;
		buf = msgs[dev->msg_read_idx].buf;

		for (; len > 0 && rx_valid > 0; len--, rx_valid--) {
			*buf++ = dmarx->buf[rd_idx++];
		}

	}

	controller->using_rx_dma = false;

	if (!(controller->using_tx_dma) && !(controller->using_rx_dma))
		complete(&controller->dma_complete);
}


/*
 * Returns:
 *	1 if we queued up a RX DMA buffer.
 *	0 if we didn't want to handle this by DMA
 */
static int i2c_dw_dma_rx_trigger_dma(struct dw_i2c_dev *dev)
{
	struct dw_i2c_dma_data *dmarx;
	struct dma_chan *rxchan;
	struct dma_device *dma_dev = NULL;
	struct dma_async_tx_descriptor *desc;
	struct dw_hisi_controller *controller = dev->priv_data;

	if (!controller) {
		dev_err(dev->dev, "%s: i2c contrller does not be init.\n", __func__);
		return -ENODEV;
	}

	dmarx = &controller->dmarx;
	if(!dmarx) {
		dev_err(dev->dev, "dmarx is NULL!\n");
                return -EIO;
	}
	rxchan = dmarx->chan;

	if (!rxchan) {
		dev_err(dev->dev, "rxchan is NULL!\n");
		return -EIO;
	}

	dma_dev =  rxchan->device;

	dev_dbg(dev->dev, "i2c_dw_dma_rx_trigger_dma, %d bytes to read\n",
			controller->dmarx.sg.length);

	if (dma_map_sg(dma_dev->dev, &dmarx->sg, 1, DMA_FROM_DEVICE) != 1) {
		dev_warn(dev->dev, "unable to map TX DMA\n");
		return -EBUSY;
	}

	desc = dmaengine_prep_slave_sg(rxchan, &dmarx->sg, 1, DMA_FROM_DEVICE,/*lint !e64*/
					DMA_PREP_INTERRUPT | DMA_CTRL_ACK);/*lint !e655*/
	if (!desc) {
		dma_unmap_sg(dma_dev->dev, &dmarx->sg, 1, DMA_FROM_DEVICE);
		dev_warn(dev->dev, "RX DMA busy\n");
		return -EBUSY;
	}

	/* Some data to go along to the callback */
	desc->callback = i2c_dw_dma_rx_callback;
	desc->callback_param = dev;

	/* All errors should happen at prepare time */
	dmaengine_submit(desc);

	/* Fire the DMA transaction */
	dma_async_issue_pending(rxchan);
	return 1;
}

static int i2c_dw_dma_sg_init(struct dw_i2c_dev *dev,
				struct dw_i2c_dma_data *dma_data,
				unsigned long length)
{
	dma_data->buf = devm_kzalloc(dev->dev, length, GFP_KERNEL);
	if (!dma_data->buf) {
		dev_err(dev->dev, "%s: no memory for DMA buffer, length: %lu\n",
				__func__, length);
		return -ENOMEM;
	}

	sg_init_one(&dma_data->sg, dma_data->buf, length);

	return 0;
}

void i2c_dw_dma_clear(struct dw_i2c_dev *dev)
{
	struct dw_hisi_controller *controller = dev->priv_data;

	if (!controller) {
		dev_err(dev->dev, "%s: i2c contrller does not be init.\n", __func__);
		return;
	}

	if (controller->dmatx.buf) {
		dmaengine_terminate_all(controller->dmatx.chan);
		dma_unmap_sg(controller->dmatx.chan->device->dev,
					 &controller->dmatx.sg, 1, DMA_TO_DEVICE);
		devm_kfree(dev->dev, controller->dmatx.buf);
		controller->dmatx.buf = NULL;
		controller->using_tx_dma = false;
	}

	if (controller->dmarx.buf) {
		dmaengine_terminate_all(controller->dmarx.chan);
		dma_unmap_sg(controller->dmarx.chan->device->dev,
					 &controller->dmarx.sg, 1, DMA_FROM_DEVICE);
		devm_kfree(dev->dev, controller->dmarx.buf);
		controller->dmarx.buf = NULL;
		controller->using_rx_dma = false;
	}

	controller->using_dma = false;
	controller->dmacr = 0;
	dw_writel(dev, controller->dmacr & 0xffff, DW_IC_DMA_CR);/*lint !e144*/
}

void i2c_dw_dma_fifo_cfg(struct dw_i2c_dev *dev)
{
	dw_writel(dev, dev->tx_fifo_depth - 16, DW_IC_DMA_TDLR);
	dw_writel(dev, 15, DW_IC_DMA_RDLR);
}

#else
/* Blank functions if the DMA engine is not available */
void i2c_dw_dma_probe(struct dw_i2c_dev *dev)
{
}

void i2c_dw_dma_remove(struct dw_i2c_dev *dev)
{
}

static int i2c_dw_dma_tx_refill(struct dw_i2c_dev *dev)
{
	return -EIO;
}

static int i2c_dw_dma_rx_trigger_dma(struct dw_i2c_dev *dev)
{
	return -EIO;
}

static int i2c_dw_dma_sg_init(struct dw_i2c_dev *dev,
			      struct dw_i2c_dma_data *dma_data,
			      unsigned long length)
{
	return -EIO;
}

void i2c_dw_dma_clear(struct dw_i2c_dev *dev)
{
}

void i2c_dw_dma_fifo_cfg(struct dw_i2c_dev *dev)
{
}
#endif

EXPORT_SYMBOL_GPL(i2c_dw_dma_clear);
EXPORT_SYMBOL_GPL(i2c_dw_dma_fifo_cfg);

int i2c_dw_xfer_msg_dma(struct dw_i2c_dev *dev, int *alllen)
{
	struct dw_hisi_controller *controller = dev->priv_data;
	struct i2c_msg *msgs = dev->msgs;
	u8 *buf = dev->tx_buf;
	int rx_len = 0;
	int tx_len;
	int i;
	int total_len = 0;
	u32 buf_len;
	u16 *dma_txbuf;
	int ret = -EPERM;

	if (!controller) {
		dev_err(dev->dev, "%s: i2c contrller does not be init.\n", __func__);
		return -ENODEV;
	}

	if (!controller->dmatx.chan || !controller->dmarx.chan) {
		return -EPERM;

	}

	/* If total date length less than a fifodepth, not use DMA */
	for (i = dev->msg_write_idx; i < dev->msgs_num; i++)
		total_len += dev->msgs[i].len;

	dev_dbg(dev->dev, "%s: msg num: %d, total length: %d\n",
			__func__, dev->msgs_num, total_len);

	*alllen = total_len;

	if (total_len < dev->tx_fifo_depth)/*lint !e574*/
		return -EPERM;

	tx_len = total_len * sizeof(unsigned short);

	if (tx_len > I2C_DW_MAX_DMA_BUF_LEN) {
		dev_err(dev->dev, "Too long to send with DMA: %d\n", tx_len);
		dev->msg_err = -EINVAL;
		return -EPERM;
	}

	dev_dbg(dev->dev, "use DMA transfer, len=%d\n", tx_len);

	reinit_completion(&controller->dma_complete);
	controller->using_dma = true;

	ret = i2c_dw_dma_sg_init(dev, &controller->dmatx, tx_len);
	if (ret < 0)
		return ret;

	controller->using_tx_dma = true;

	dma_txbuf = (u16 *)controller->dmatx.buf;

	for (; dev->msg_write_idx < dev->msgs_num; dev->msg_write_idx++) {
		u32 cmd = 0;
		buf = msgs[dev->msg_write_idx].buf;
		buf_len = msgs[dev->msg_write_idx].len;

		if (msgs[dev->msg_write_idx].flags & I2C_M_RD) {
			for (i = 0; i < buf_len; i++) {/*lint !e574*/
				if (dev->msg_write_idx == dev->msgs_num - 1 &&
					i == buf_len - 1)
					cmd |= BIT(9);

				*dma_txbuf++ = cmd | 0x100;
				rx_len++;
			}
		} else {
			for (i = 0; i < buf_len; i++) {/*lint !e574*/
				if (dev->msg_write_idx == dev->msgs_num - 1 &&
					i == buf_len - 1)
					cmd |= BIT(9);

				*dma_txbuf++ = cmd | *buf++;
			}
		}
	}

	dev_dbg(dev->dev, "%s: dev->dmatx.sg.length: %d, tx_len: %d\n",
			__func__, controller->dmatx.sg.length, tx_len);

	if (rx_len > 0) {
		ret = i2c_dw_dma_sg_init(dev, &controller->dmarx, rx_len);
		if (ret < 0)
			goto error;

		controller->using_rx_dma = true;

		if (i2c_dw_dma_rx_trigger_dma(dev) >= 0)
			controller->dmacr |= DW_IC_RXDMAE;
		else {
			dev_warn(dev->dev, "Dma rx failed.\n");
			goto error;
		}
	}

	if (i2c_dw_dma_tx_refill(dev) >= 0)
		controller->dmacr |= DW_IC_TXDMAE;
	else {
		dev_warn(dev->dev, "Dma tx failed.\n");
		goto error;
	}

	dw_writel(dev, controller->dmacr & 0xffff, DW_IC_DMA_CR);/*lint !e144*/

	ret = 0;
error:
	if (ret < 0) {
		controller->using_dma = false;
		/* Restore for CPU transfer */
		dev->msg_write_idx = 0;
		dev->msg_read_idx = 0;
		dev->status &= ~HISI_STATUS_WRITE_IN_PROGRESS;
		dev_err(dev->dev, "i2c_dw_xfer_msg_dma return erron %d.\n",
				ret);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(i2c_dw_xfer_msg_dma);

/*
 * This routine does i2c bus recovery by using i2c_generic_gpio_recovery
 * which is provided by I2C Bus recovery infrastructure.
 */
static void hisi_dw_i2c_prepare_recovery(struct i2c_adapter *adap)
{
	struct dw_i2c_dev *dev = i2c_get_adapdata(adap);
	int ret;

	ret = dw_hisi_pins_ctrl(dev, PINCTRL_STATE_IDLE);
	if (ret < 0)
		dev_warn(dev->dev,"pins are not configured to idle\n");
}

static void hisi_dw_i2c_unprepare_recovery(struct i2c_adapter *adap)
{
	struct dw_i2c_dev *dev = i2c_get_adapdata(adap);
	int ret;

	ret = dw_hisi_pins_ctrl(dev, PINCTRL_STATE_DEFAULT);
	if (ret < 0)
		dev_warn(dev->dev,"pins are not configured to default\n");
}

void hisi_dw_i2c_scl_recover_bus(struct i2c_adapter *adap)
{
	struct dw_i2c_dev *dev = i2c_get_adapdata(adap);
	struct i2c_bus_recovery_info *bri = adap->bus_recovery_info;
	int ret;

	dev_info(dev->dev, "bus prepares recovery ...\n");

	ret = gpio_request_one(bri->sda_gpio, GPIOF_IN, "i2c-sda");
	if (ret)
		dev_warn(dev->dev, "Can't get SDA gpio: %d. Not using SDA polling\n",
				bri->sda_gpio);

	/* disable IC */
	dw_writel(dev, 0, HISI_DW_IC_ENABLE);

	/* speed is 100KHz*/
	dw_writel(dev, HISI_DW_IC_CON_RECOVERY_CFG, HISI_DW_IC_CON);

	/* config slave address to  0x7f */
	dw_writel(dev, HISI_DW_IC_TAR_RECOVERY_CFG, HISI_DW_IC_TAR);
	/* enable IC */
	dw_writel(dev, 1, HISI_DW_IC_ENABLE);
	/* recived data from bus*/
	dw_writel(dev, HISI_DW_IC_RD_CFG, HISI_DW_IC_DATA_CMD);

	msleep(100);

	gpio_free(bri->sda_gpio);

	ret = dw_hisi_pins_ctrl(dev, PINCTRL_STATE_DEFAULT);
	if (ret < 0)
		dev_warn(dev->dev,"pins are not configured to default\n");

	i2c_dw_init(dev);

	dev_info(dev->dev, "bus recovered completely!\n");
}


int i2c_init_secos(struct i2c_adapter *adap)
{
	int ret;
	struct dw_i2c_dev *dev = NULL;

	if (!adap) {
		pr_err("i2c_init_secos: i2c adapter is NULL!\n");
		return -ENODEV;
	}

	dev = i2c_get_adapdata(adap);
	if (!dev) {
		pr_err( "i2c_init_secos: can not get i2c dev!\n");
		return -ENODEV;
	}
	if (IS_ERR(dev->clk)) {
		dev_err(dev->dev, "i2c_init_secos: i2c clk is error!\n");
		return -EINVAL;
	}

	mutex_lock(&dev->lock);

	ret = clk_enable(dev->clk);
	if (ret) {
		dev_err(dev->dev, "i2c_init_secos: can not enable i2c clock!\n");
		goto err;
	}

	ret = dw_hisi_pins_ctrl(dev, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		dev_err(dev->dev, "i2c_init_secos: pins are not configured to DEFAULT!\n");
		clk_disable(dev->clk);
		goto err;
	}

	return 0;/*lint !e454*/

err:
	mutex_unlock(&dev->lock);
	return -EINVAL;
}
EXPORT_SYMBOL_GPL(i2c_init_secos);

int i2c_exit_secos(struct i2c_adapter *adap)
{
	struct dw_i2c_dev *dev = NULL;
	int ret;

	if (!adap) {
		pr_err("i2c_exit_secos: i2c adapter is NULL!\n");
		return -ENODEV;
	}

	dev = i2c_get_adapdata(adap);
	if (!dev) {
		pr_err("i2c_exit_secos: can not get i2c dev!\n");
		return -ENODEV;
	}

	if (IS_ERR(dev->clk)) {
		dev_err(dev->dev, "i2c_exit_secos: i2c clk is error!\n");
		return -EINVAL;
	}

	ret = dw_hisi_pins_ctrl(dev, PINCTRL_STATE_IDLE);
	if (ret < 0)
		dev_err(dev->dev, "i2c_exit_secos: pins are not configured to IDLE!\n");

	clk_disable(dev->clk);
	mutex_unlock(&dev->lock);/*lint !e455*/

	return 0;
}
EXPORT_SYMBOL_GPL(i2c_exit_secos);

static struct i2c_algorithm hs_i2c_dw_algo = {
	.master_xfer	= i2c_dw_xfer,
	.functionality	= i2c_dw_func,
};

void i2c_frequency_division(struct dw_i2c_dev *dev, u32 clk)
{
	u32 sda_falling_time, scl_falling_time;

	sda_falling_time = dev->sda_falling_time ?: 300; /* ns */
	scl_falling_time = dev->scl_falling_time ?: 300; /* ns */

	dev->hs_hcnt = i2c_dw_scl_hcnt(clk,
					100,    /* tHD;STA = tHIGH = 0.1 us */
					sda_falling_time,   /* tf = 0.3 us */
					0,      /* 0: DW default, 1: Ideal */
					0);     /* No offset */
	dev->hs_lcnt = i2c_dw_scl_lcnt(clk,
					200,    /* tLOW = 0.2 us */
					scl_falling_time,   /* tf = 0.3 us */
					0);     /* No offset */
}

static int hs_dw_i2c_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dw_i2c_dev *d;
	struct i2c_adapter *adap;
	struct resource *iores;
	struct dw_hisi_controller *controller;
	struct i2c_bus_recovery_info *gpio_recovery_info;
	u32 data[4] = {0};
	u64 clk_rate = 0;
	u32 speed_mode = 0;
	u32 retries = 0;
	u32 timeout = 0;
	u32 input_clock_khz;
	int gpio_scl, gpio_sda, r;
	u32 secure_mode = 0;

	d = devm_kzalloc(dev, sizeof(struct dw_i2c_dev), GFP_KERNEL);
	if (!d) {
		dev_err(dev, "mem alloc failed for dw_i2c_dev data\n");
		return -ENOMEM;
	}

	controller = devm_kzalloc(dev, sizeof(struct dw_hisi_controller), GFP_KERNEL);
	if (!controller) {
		dev_err(dev, "mem alloc failed for controller\n");
		return -ENOMEM;/*lint !e429*/
	}

	gpio_recovery_info = devm_kzalloc(dev, sizeof(struct i2c_bus_recovery_info), GFP_KERNEL);
	if (!gpio_recovery_info) {
		dev_err(dev, "mem alloc failed for gpio_recovery_info\n");
		return -ENOMEM;/*lint !e429*/
	}

	d->dev = get_device(dev);
	d->priv_data = controller;

	/* NOTE: driver uses the static register mapping */
	iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!iores)
		return -EINVAL;/*lint !e429*/

	r = of_property_read_u32(dev->of_node, "secure-mode", &secure_mode);
	if (r) {
		secure_mode = 0;
	}
	d->secure_mode = secure_mode;

	controller->mapbase  = iores->start;
	if (secure_mode) {
		d->base = 0;
	} else {
		d->base = ioremap(iores->start, resource_size(iores));
		if (!d->base)
			return -EADDRNOTAVAIL;/*lint !e429*/
	}

	controller->pinctrl_flag = 0;
	controller->pinctrl = NULL;

	if (of_find_property(dev->of_node, "i2c3-usf", NULL)) {
		d->setpin = 0;
	} else {
		d->setpin = 1;
	}

	r = of_property_read_u32_array(dev->of_node, "reset-reg-base", &data[0], 4);
	if (r) {
		dev_err(dev,  "doesn't have reset-reg-base property!\n");
	} else {
		controller->reset_reg_base = devm_ioremap(dev, data[1], data[3]);
		dev_info(dev, "i2c reset register phy_addr is: %x\n", data[1]);
	}

	dev_info(dev, "i2c reset register vir_addr is: %pK\n", controller->reset_reg_base);

	r = of_property_read_u32_array(dev->of_node, "delay-off", &controller->delay_off, 1);
	if (r) {
		dev_err(dev, "doesn't have delay-off property!\n");
	}

	r = of_property_read_u32_array(dev->of_node, "reset-controller-reg",
						&data[0], 4);
	if (r) {
		dev_err(dev, "doesn't have reset-controller-reg property!\n");
	}

	gpio_scl = of_get_named_gpio(dev->of_node, "cs-gpios", 0);
	gpio_sda = of_get_named_gpio(dev->of_node, "cs-gpios", 1);
	dev_info(dev, "i2c cs-gpios = %d, %d!\n", gpio_scl, gpio_sda);
	if (gpio_scl == -ENOENT || gpio_sda == -ENOENT) {
		dev_err(dev, "doesn't have gpio scl/sda property!\n");
		goto cs_gpio_err;
	}

cs_gpio_err:
	controller->priv.reset_enable_off = data[0];
	controller->priv.reset_disable_off = data[1];
	controller->priv.reset_status_off = data[2];
	controller->priv.reset_bit = data[3];

	d->get_clk_rate_khz = hs_i2c_dw_get_clk_rate_khz;
	controller->reset_controller = reset_i2c_controller;
	controller->recover_bus = hisi_dw_i2c_scl_recover_bus;

	d->clk = devm_clk_get(dev, "clk_i2c");
	if (IS_ERR(d->clk))
		return -ENODEV;/*lint !e429*/

	r = of_property_read_u64(dev->of_node, "clock-rate", &clk_rate);
	if (r) {
		dev_info(dev, "doesn't have clock-rate property!\n");
	} else {
		dev_info(dev, "clock rate is %llu\n", clk_rate);
		r = clk_set_rate(d->clk, clk_rate);
		if (r) {
			dev_err(dev, "clock rate set failed r[0x%x]\n", r);
			return -EINVAL;/*lint !e429*/
		}
	}

	r = of_property_read_u32(dev->of_node, "speed-mode", &speed_mode);
	if (r) {
		dev_info(dev, "no speed-mode property, use default!\n");
		speed_mode = DW_IC_CON_SPEED_FAST;
	} else {
		if ((speed_mode != DW_IC_CON_SPEED_STD) && (speed_mode != DW_IC_CON_SPEED_FAST)
				&& (speed_mode != DW_IC_CON_SPEED_HIGH)) {
			speed_mode = DW_IC_CON_SPEED_FAST;
		}
		dev_info(dev, "speed mode is %d!\n", speed_mode);
	}

	r = of_property_read_u32(dev->of_node, "retries", &retries);
	if (r) {
		dev_info(dev,"no retries ,use default!\n");
		retries = 3;
	} else {
		dev_info(dev, "retries is: %d!\n", retries);
	}

	r = of_property_read_u32(dev->of_node, "timeout", &timeout);
	if (r) {
		dev_info(dev,"no timeout ,use default!\n");
		timeout = 100;
	} else {
		dev_info(dev, "timeout is: %d!\n", timeout);
	}

	r = clk_prepare_enable(d->clk);
	if (r) {
		dev_warn(dev, "Unable to enable clock!\n");
		return  -EINVAL;/*lint !e429*/
	}

	input_clock_khz = hs_i2c_dw_get_clk_rate_khz(d);
	i2c_frequency_division(d, input_clock_khz);

	init_completion(&d->cmd_complete);
	init_completion(&controller->dma_complete);
	mutex_init(&d->lock);

	d->functionality =
		I2C_FUNC_I2C |
		I2C_FUNC_10BIT_ADDR |
		I2C_FUNC_SMBUS_BYTE |
		I2C_FUNC_SMBUS_BYTE_DATA |
		I2C_FUNC_SMBUS_WORD_DATA |
		I2C_FUNC_SMBUS_I2C_BLOCK;

	d->master_cfg =  DW_IC_CON_MASTER | DW_IC_CON_SLAVE_DISABLE |
					 DW_IC_CON_RESTART_EN | speed_mode;

	d->accessor_flags = ACCESS_32BIT;

	hs_i2c_dw_reset_controller(d);

	{
		u32 param1 = dw_readl(d, DW_IC_COMP_PARAM_1);

		d->tx_fifo_depth = ((param1 >> 16) & 0xff) + 1;
		d->rx_fifo_depth = ((param1 >> 8)  & 0xff) + 1;
		dev_info(dev, "tx_fifo_depth: %d, rx_fifo_depth: %d\n",
				 d->tx_fifo_depth, d->rx_fifo_depth);
	}

	if(I2C_DELAY_70NS == controller->delay_off)
		d->sda_hold_time = (input_clock_khz * 70)/1000000;
	else
		d->sda_hold_time= (input_clock_khz * 300)/1000000;

	r = i2c_dw_init(d);
	if (r)
		goto err;

	i2c_dw_disable_int(d);

	d->irq = platform_get_irq(pdev, 0);
	if (d->irq < 0) {
		dev_err(dev, "no irq resource?\n");
		return d->irq; /* -ENXIO *//*lint !e429*/
	}

	controller->irq_is_run = 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	r = devm_request_irq(dev, d->irq,
			i2c_dw_isr, 0, pdev->name, d);
#else
	r = devm_request_irq(dev, d->irq,
			i2c_dw_isr, IRQF_DISABLED, pdev->name, d);
#endif
	if (r) {
		dev_err(dev, "failure requesting irq %i\n", d->irq);
		return -EINVAL;/*lint !e429*/
	}

	gpio_recovery_info->recover_bus = i2c_generic_gpio_recovery;
	gpio_recovery_info->prepare_recovery = hisi_dw_i2c_prepare_recovery;
	gpio_recovery_info->unprepare_recovery = hisi_dw_i2c_unprepare_recovery;
	gpio_recovery_info->scl_gpio = gpio_scl;
	gpio_recovery_info->sda_gpio =gpio_sda;

	adap = &d->adapter;
	i2c_set_adapdata(adap, d);
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON;
	strlcpy(adap->name, "Synopsys DesignWare I2C adapter",
			sizeof(adap->name));
	adap->algo = &hs_i2c_dw_algo;
	adap->dev.parent = dev;
	adap->dev.of_node = dev->of_node;
	adap->bus_recovery_info = gpio_recovery_info;
	adap->nr = pdev->id;
	adap->retries = (int)retries;
	adap->timeout = (int)timeout;
	dev_info(dev,"adap->retries = %d adap->timeout = %d\n ",adap->retries,adap->timeout);

	r = i2c_add_numbered_adapter(adap);
	if (r) {
		dev_err(dev, "failure adding adapter\n");
		goto err;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
	of_i2c_register_devices(adap);
#endif

	platform_set_drvdata(pdev, d);

	/*DMA probe*/
	i2c_dw_dma_probe(d);

	clk_disable(d->clk);

	dev_info(dev, "i2c probe succeed!\n");
	return 0;

err:
	clk_disable_unprepare(d->clk);
	d->clk = NULL;
	put_device(dev);
	return r;/*lint !e429 !e593*/
}

static int hs_dw_i2c_remove(struct platform_device *pdev)
{
	struct dw_i2c_dev *d = platform_get_drvdata(pdev);

	if (!d) {
		pr_err("%s: get drvdata failed\n", __func__);
		return -EINVAL;
	}

	platform_set_drvdata(pdev, NULL);
	i2c_del_adapter(&d->adapter);
	put_device(&pdev->dev);
	clk_disable_unprepare(d->clk);
	devm_clk_put(&pdev->dev, d->clk);
	d->clk = NULL;

	i2c_dw_dma_remove(d);

	i2c_dw_disable(d);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id hs_dw_i2c_of_match[] = {
	{ .compatible = "hisilicon,designware-i2c", },
	{},
};
MODULE_DEVICE_TABLE(of, hs_dw_i2c_of_match);
#endif

#ifdef CONFIG_PM
static int hs_dw_i2c_suspend(struct device *dev)
{
	unsigned long time, timeout;
	struct platform_device *pdev = to_platform_device(dev);
	struct dw_i2c_dev *i_dev = platform_get_drvdata(pdev);

	if (!i_dev) {
		pr_err("%s: get drvdata failed\n", __func__);
		return -EINVAL;
	}

	dev_info(&pdev->dev, "%s: suspend +\n", __func__);

	timeout = jiffies + msecs_to_jiffies(GET_DEV_LOCK_TIMEOUT);
	while (!mutex_trylock(&i_dev->lock)) {
		time = jiffies;
		if (time_after(time, timeout)) {
			dev_info(&pdev->dev, "%s: mutex_trylock timeout fail.\n", __func__);

			return -EAGAIN;
		}

		usleep_range(1000, 2000);
	}

	dev_info(&pdev->dev, "%s: suspend -\n", __func__);
	return 0;
}

static int hs_dw_i2c_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct dw_i2c_dev *i_dev = platform_get_drvdata(pdev);
	int ret = 0;

	if (!i_dev) {
		pr_err("%s: get drvdata failed\n", __func__);
		return -EINVAL;
	}

	dev_info(&pdev->dev, "%s: resume +\n", __func__);

	ret = clk_enable(i_dev->clk);
	if (ret) {
		dev_err(&pdev->dev, "clk_prepare_enable failed!\n");
		return -EAGAIN;
	}
	hs_i2c_dw_reset_controller(i_dev);
	i2c_dw_init(i_dev);
	i2c_dw_disable_int(i_dev);
	clk_disable(i_dev->clk);

	mutex_unlock(&i_dev->lock);/*lint !e455*/

	dev_info(&pdev->dev, "%s: resume -\n", __func__);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(hs_dw_i2c_dev_pm_ops, hs_dw_i2c_suspend, hs_dw_i2c_resume);

static struct platform_driver hs_dw_i2c_driver = {
	.probe		= hs_dw_i2c_probe,
	.remove		= hs_dw_i2c_remove,
	.driver		= {
		.name	= "i2c_designware-hs",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(hs_dw_i2c_of_match),
		.pm	= &hs_dw_i2c_dev_pm_ops,
	},
};
module_platform_driver(hs_dw_i2c_driver);

MODULE_DESCRIPTION("HS Synopsys DesignWare I2C bus adapter");
MODULE_ALIAS("platform:i2c_designware-hs");
MODULE_LICENSE("GPL");
