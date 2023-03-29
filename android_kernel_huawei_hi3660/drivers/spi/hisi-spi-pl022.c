#define GET_HARDWARE_TIMEOUT 10000
#define SPI_4G_PHYS_ADDR 0xFFFFFFFF

#define SSP_TXFIFOCR(r)	(r + 0x028)
#define SSP_RXFIFOCR(r)	(r + 0x02C)

#define SSP_TXFIFOCR_MASK_DMA		(0x07UL << 0)
#define SSP_TXFIFOCR_MASK_INT		(0x3UL << 3)
/*
 * SSP RX FIFO Register - SSP_TXFIFOCR
 */
#define SSP_RXFIFOCR_MASK_DMA		(0x07UL << 0)
#define SSP_RXFIFOCR_MASK_INT		(0x3UL << 3)

#define DEFAULT_SSP_REG_TXFIFOCR ( \
	GEN_MASK_BITS(SSP_TX_64_OR_MORE_EMPTY_LOC, SSP_TXFIFOCR_MASK_DMA, 0) | \
	GEN_MASK_BITS(SSP_TX_64_OR_MORE_EMPTY_LOC, SSP_TXFIFOCR_MASK_INT, 3) \
)

/*
 * Default SSP Register Values
 */
#define DEFAULT_SSP_REG_RXFIFOCR ( \
	GEN_MASK_BITS(SSP_RX_16_OR_MORE_ELEM, SSP_RXFIFOCR_MASK_DMA, 0) | \
	GEN_MASK_BITS(SSP_RX_16_OR_MORE_ELEM, SSP_RXFIFOCR_MASK_INT, 3) \
)

#define ENUM_SPI_HWSPIN_LOCK 27

#if defined(CONFIG_SPI_HISI_CS_USE_PCTRL)
static inline bool pctrl_cs_is_valid(int cs)
{
	return (cs != ~0);
}

static void pl022_pctrl_cs_set(struct pl022 *pl022, u32 command)
{
	/*cmd bit0~3 mask bit16~19 cs0~3*/
	if (SSP_CHIP_SELECT == command)
		writel(pl022->cur_cs, pl022->pctrl_base + 0x04);
	else
		writel(pl022->cur_cs & 0xffff0000, pl022->pctrl_base + 0x04);
}
#endif

static int pl022_prepare_transfer_hardware(struct spi_master *master)
{
	struct pl022 *pl022 = spi_master_get_devdata(master);
	struct hwspinlock *hwlock = pl022->spi_hwspin_lock;
	volatile int ret = 0;
	unsigned long time, timeout;

	/*
	 * Just make sure we have all we need to run the transfer by syncing
	 * with the runtime PM framework.
	 */
	timeout = jiffies + msecs_to_jiffies(GET_HARDWARE_TIMEOUT);
	if (pl022->hardware_mutex) {
		do {
			ret = hwlock->bank->ops->trylock(hwlock);
			if (0 == ret) {
				time = jiffies;
				if (time_after(time, timeout)) {
					dev_err(&pl022->adev->dev, "get hardware_mutex wait for completion timeout\n");
					return -ETIME;
				}
				usleep_range(1000, 2000);
			}
		} while (0 == ret);
	}

	return 0;
}

static struct {
	struct mutex lock;
	int   users;
} spi_secos_gates[5] = {

	[0] = {
		.lock = __MUTEX_INITIALIZER(spi_secos_gates[0].lock),
	},
	[1] = {
		.lock = __MUTEX_INITIALIZER(spi_secos_gates[1].lock),
	},
	[2] = {
		.lock = __MUTEX_INITIALIZER(spi_secos_gates[2].lock),
	},
	[3] = {
		.lock = __MUTEX_INITIALIZER(spi_secos_gates[3].lock),
	},
	[4] = {
		.lock = __MUTEX_INITIALIZER(spi_secos_gates[4].lock),
	},

};

int spi_init_secos(unsigned int spi_bus_id)
{
	int ret = 0;
	struct spi_master *master = NULL;
	struct pl022 *pl022 = NULL;

	if (spi_smc_flag[spi_bus_id]) {
		master = spi_busnum_to_master(spi_bus_id);
		if (!master) {
			printk(KERN_ERR"switch to secos get spi master:%d is wrong\n", spi_bus_id);
			return -EINVAL;
		}

		pl022 = spi_master_get_devdata(master);
		if (!pl022) {
			printk(KERN_ERR"switch to secos pl022:%d is NULL!\n", spi_bus_id);
			return -EINVAL;
		}

		if (!pl022->clk) {
			dev_err(&pl022->adev->dev, "switch to secos pl022->clk is NULL for spi%d!\n", spi_bus_id);
			return -EINVAL;
		}

		mutex_lock(&spi_secos_gates[spi_bus_id].lock);
		if (spi_secos_gates[spi_bus_id].users) {
			++spi_secos_gates[spi_bus_id].users;
			mutex_unlock(&spi_secos_gates[spi_bus_id].lock);
			return 0;
		}

		ret = clk_prepare_enable(pl022->clk);
		if (ret) {
			dev_err(&pl022->adev->dev, "switch to secos could not enable SPI%d bus clock\n", spi_bus_id);
			ret = -EINVAL;
		} else {
			spi_secos_gates[spi_bus_id].users = 1;
		}
		mutex_unlock(&spi_secos_gates[spi_bus_id].lock);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(spi_init_secos);

int spi_exit_secos(unsigned int spi_bus_id)
{
	struct spi_master *master = NULL;
	struct pl022 *pl022 = NULL;

	if (spi_smc_flag[spi_bus_id]) {
		master = spi_busnum_to_master(spi_bus_id);
		if (!master) {
			printk(KERN_ERR"back to kernel get spi master:%d is wrong\n", spi_bus_id);
			return -EINVAL;
		}

		pl022 = spi_master_get_devdata(master);
		if (!pl022) {
			printk(KERN_ERR"back to kernel pl022:%d is NULL!\n", spi_bus_id);
			return -EINVAL;
		}

		if (!pl022->clk) {
			dev_err(&pl022->adev->dev, "back to kernel pl022->clk is NULL for spi:%d\n", spi_bus_id);
			return -EINVAL;
		}

		mutex_lock(&spi_secos_gates[spi_bus_id].lock);
		if (--spi_secos_gates[spi_bus_id].users) {
			mutex_unlock(&spi_secos_gates[spi_bus_id].lock);
			return 0;
		}
		clk_disable_unprepare(pl022->clk);
		mutex_unlock(&spi_secos_gates[spi_bus_id].lock);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(spi_exit_secos);

static int hisi_spi_get_pins_data(struct pl022 *pl022, struct device *dev)
{
	int status;
	pl022->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(pl022->pinctrl)) {
		dev_err(dev, "%d %s\n", __LINE__, __func__);
		return -EFAULT;
	}

	pl022->pins_default = pinctrl_lookup_state(pl022->pinctrl,
						 PINCTRL_STATE_DEFAULT);
	/* enable pins to be muxed in and configured */
	if (!IS_ERR(pl022->pins_default)) {
		status = pinctrl_select_state(pl022->pinctrl,
				pl022->pins_default);
		if (status)
			dev_err(dev, "could not set default pins\n");
	} else
		dev_err(dev, "could not get default pinstate\n");

	pl022->pins_idle = pinctrl_lookup_state(pl022->pinctrl,
						  PINCTRL_STATE_IDLE);
	if (IS_ERR(pl022->pins_idle))
		dev_dbg(dev, "could not get idle pinstate\n");

	pl022->pins_sleep = pinctrl_lookup_state(pl022->pinctrl,
						   PINCTRL_STATE_SLEEP);
	if (IS_ERR(pl022->pins_sleep))
		dev_dbg(dev, "could not get sleep pinstate\n");

	pl022->pins.p = pl022->pinctrl;
	pl022->pins.default_state = pl022->pins_default;
#ifdef CONFIG_PM
	pl022->pins.idle_state = pl022->pins_idle;
	pl022->pins.sleep_state = pl022->pins_sleep;
#endif
	dev->pins = &pl022->pins;

	return 0;
}

static void hisi_spi_txrx_buffer_check(struct pl022 *pl022,struct spi_transfer *transfer)
{
	if ((virt_to_phys(pl022->cur_transfer->tx_buf) > SPI_4G_PHYS_ADDR)) {
		/* wrining! must be use dma buffer, and needs the flag "GFP_DMA" when alloc memery */
		WARN_ON(1);
		pl022->tx_buffer = kzalloc(pl022->cur_transfer->len, GFP_KERNEL | GFP_DMA);
		if (NULL != pl022->tx_buffer) {
			memcpy(pl022->tx_buffer, transfer->tx_buf, pl022->cur_transfer->len);
			pl022->tx = (void*)pl022->tx_buffer;
			dev_err(&pl022->adev->dev,"tx is not dma-buffer\n");
		} else {
			pl022->tx = (void *)transfer->tx_buf;
			dev_err(&pl022->adev->dev,"can not alloc dma-buffer for tx\n");
		}

	} else {
		pl022->tx = (void *)transfer->tx_buf;
	}
	pl022->tx_end = pl022->tx + pl022->cur_transfer->len;

	if ((virt_to_phys(pl022->cur_transfer->rx_buf) > SPI_4G_PHYS_ADDR)) {
		/* wrining! must be use dma buffer, and needs the flag "GFP_DMA" when alloc memery */
		WARN_ON(1);
		pl022->rx_buffer = kzalloc(pl022->cur_transfer->len, GFP_KERNEL | GFP_DMA);
		if (NULL != pl022->rx_buffer) {
			pl022->rx = (void *)pl022->rx_buffer;
			dev_err(&pl022->adev->dev,"rx is not dma-buffer\n");
		} else {
			pl022->rx = (void *)transfer->rx_buf;
			dev_err(&pl022->adev->dev,"can not alloc dma-buffer for rx\n");
		}
	} else {
		pl022->rx = (void *)transfer->rx_buf;
	}
	pl022->rx_end = pl022->rx + pl022->cur_transfer->len;
}

static void hisi_spi_dma_buffer_free(struct pl022 *pl022)
{
	if (NULL != pl022->rx_buffer) {
		memcpy(pl022->cur_transfer->rx_buf, pl022->rx, pl022->cur_transfer->len);
		kfree(pl022->rx_buffer);
		pl022->rx_buffer = NULL;
	}

	if (NULL != pl022->tx_buffer) {
		kfree(pl022->tx_buffer);
		pl022->tx_buffer = NULL;
	}
}
