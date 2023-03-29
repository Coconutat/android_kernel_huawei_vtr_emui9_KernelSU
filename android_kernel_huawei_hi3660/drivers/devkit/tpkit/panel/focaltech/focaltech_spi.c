/*
 *                  Copyright 2010 - 2017, Huawei Tech. Co., Ltd.
 *                            ALL RIGHTS RESERVED
 *
 * Filename      : focaltech_spi.c
 * Author        : lihai
 * Creation time : 2017/12/2
 * Description   :
 *
 * Version       : 1.0
 */

/*****************************************************************************
* Included header files
*****************************************************************************/
#include "focaltech_core.h"

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define SPI_BUF_LENGTH                      256
#define SPI_HEADER_LENGTH                   3
#define STATUS_PACKAGE				0x05
#define COMMAND_PACKAGE				0xC0
#define DATA_PACKAGE				0x3F
#define BUSY_QUERY_TIMEOUT				100
#define BUSY_QUERY_DELAY					150 /* unit: us */
#define CS_HIGH_DELAY				30 /* unit: us */
#define DELAY_AFTER_FIRST_BYTE		20
#define POLYNOMIAL_PARAMETER		0x8408

#define CTRL_READ_WRITE_FLAG				7
#define CTRL_CMD_CRC_BIT					6
#define CTRL_DATA_CRC_BIT				5

#define STATUS_BUSY_BIT					7

#define WRITE_CMD					0x00//0x60
#define READ_CMD					0x80//0xE0

#define SPI_STATUS_MASK				0x81
extern struct ts_kit_platform_data g_ts_kit_platform_data;
/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/
static struct special_cmd {
	u8 cmd;
	u16 cmd_len;
};

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
extern  struct ts_kit_device_data *g_focal_dev_data;

/*****************************************************************************
* Static function prototypes
*****************************************************************************/
static u8 spibuf[SPI_BUF_LENGTH] = {0};
static struct special_cmd special_cmd_list[] = {
	{0x55, 1},
	{0x90, 1},
	{0xAE, 6},
	{0x85, 6},
	{0xCC, 7},
	{0xCE, 1},
	{0xCD, 1},
	{0x07, 1},
	{0x08, 1},
	{0xA8, 1},
	{0xD0, 1},
	{0xDB, 1},
	{0xF2, 6},
};

/*****************************************************************************
* functions body
*****************************************************************************/
void crckermit(u8 *data, u16 len, u16 *crc_out)
{
	u16 i = 0;
	u16 j = 0;
	u16 crc = 0xFFFF;

	if ((NULL == data) || (NULL == crc_out)) {
		TS_LOG_ERR("%s: data/crc_out is NULL\n", __func__);
		return;
	}

	for( i = 0; i < len; i++)
	{
		crc ^= data[i];
		for(j = 0; j < 8; j++)
		{
			if(crc & 0x01) 
			crc = (crc >> 1) ^ POLYNOMIAL_PARAMETER;
		else
			crc=(crc >> 1);
		}
	}

	*crc_out = crc;
}

/*
 * param - cmd : cmd need check
 * cmdlen - specail command length returned
 *
 * return : if is special cmd, return 1, otherwize return 0
 */
static bool fts_check_specail_cmd(u8 cmd, u32 *cmdlen)
{
	int i = 0;
	int list_len = sizeof(special_cmd_list)/sizeof(special_cmd_list[0]);

	if (NULL == cmdlen) {
		TS_LOG_ERR("%s:cmdlen is NULL\n", __func__);
		return false;
	}

	if (true == g_focal_pdata->fw_is_running) {
		TS_LOG_DEBUG("%s:fw is not running\n", __func__);
		return false;
	}

	for (i = 0; i < list_len; i++) {
		if (cmd == special_cmd_list[i].cmd) {
			*cmdlen = special_cmd_list[i].cmd_len;
			return true;
		}
	}

	return false;
}

int fts_spi_write(u8 *buf, u32 len)
{
	int ret = 0;
	struct spi_device *spi = g_focal_dev_data->ts_platform_data->spi;
	static enum ssp_mode temp_com_mode_w = POLLING_MODE;
	struct spi_transfer xfer[] = {
		{
			.tx_buf = &buf[0],
			.len = 1,
			.delay_usecs = DELAY_AFTER_FIRST_BYTE,
			.cs_change = 0,
			.bits_per_word = 8,
		},
		{
			.tx_buf = &buf[1],
			.len = len - 1,
		},
	};
	if(g_focal_pdata->use_dma_download_firmware) {
		if(g_ts_kit_platform_data.spidev0_chip_info.com_mode != temp_com_mode_w) {
			TS_LOG_INFO("%s: spi com_mode change to %d (0:INTERRUPT_MODE ,1:POLLING_MODE ,2:DMA_MODE).\n", __func__, g_ts_kit_platform_data.spidev0_chip_info.com_mode);
			spi->controller_data = &g_focal_dev_data->ts_platform_data->spidev0_chip_info;
			temp_com_mode_w = g_ts_kit_platform_data.spidev0_chip_info.com_mode;
			ret = spi_setup(spi);
			if (ret) {
				TS_LOG_ERR("%s: spi_setup error, ret=%d\n", __func__, ret);
				return ret;
			}
		}
	}
	ret = spi_sync_transfer(spi, xfer, ARRAY_SIZE(xfer));
	if (ret) {
		TS_LOG_ERR("%s: spi_transfer(write) error, ret=%d\n", __func__, ret);
		return ret;
	}

	return 0;
}

int fts_spi_read(u8 *buf, u32 len)
{
	int ret = 0;
	struct spi_device *spi = g_focal_dev_data->ts_platform_data->spi;
	static enum ssp_mode temp_com_mode_r = POLLING_MODE;
	struct spi_transfer xfer[] = {
		{
			.tx_buf = &buf[0],
			.rx_buf = &buf[0],
	    		.len = 1,
	  		.delay_usecs = DELAY_AFTER_FIRST_BYTE,
	        	.cs_change = 0,
	    		.bits_per_word = 8,
		},
		{
			.tx_buf = &buf[1],
			.rx_buf = &buf[1],
			.len = len - 1,
		},
	};
	if(g_focal_pdata->use_dma_download_firmware) {
		if(g_ts_kit_platform_data.spidev0_chip_info.com_mode != temp_com_mode_r) {
			TS_LOG_INFO("%s: spi com_mode change to %d (0:INTERRUPT_MODE ,1:POLLING_MODE ,2:DMA_MODE).\n", __func__, g_ts_kit_platform_data.spidev0_chip_info.com_mode);
			spi->controller_data = &g_focal_dev_data->ts_platform_data->spidev0_chip_info;
			temp_com_mode_r = g_ts_kit_platform_data.spidev0_chip_info.com_mode;
			ret = spi_setup(spi);
			if (ret) {
				TS_LOG_ERR("%s: spi_setup error, ret=%d\n", __func__, ret);
				return ret;
			}
		}
	}
	ret = spi_sync_transfer(spi, xfer, ARRAY_SIZE(xfer));
	if (ret) {
		TS_LOG_ERR("%s: spi_transfer(read) error, ret=%d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int fts_read_status(u8 *status)
{
	int ret = 0;
	u8 status_cmd[2] = {STATUS_PACKAGE, 0xFF};
	struct spi_device *spi = g_focal_dev_data->ts_platform_data->spi;
	struct spi_message msg;
	struct spi_transfer xfer = {
	.tx_buf = status_cmd,
		.rx_buf = status_cmd,
		.len = 2,
	};

	if (NULL == status) {
		TS_LOG_ERR("%s:status is NULL\n", __func__);
		return -EINVAL;
	}

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	ret = spi_sync(spi, &msg);
	if (ret) {
		TS_LOG_ERR("%s: spi_transfer(read) error, ret=%d\n", __func__, ret);
		return ret;
	}

	*status = status_cmd[1];
	return ret;
}

static int fts_wait_idle(void)
{
	int ret = 0;
	int i = 0;
	u8 status = 0xFF;

	for (i = 0; i < BUSY_QUERY_TIMEOUT; i++) {
		udelay(BUSY_QUERY_DELAY);

		ret = fts_read_status(&status);
		if (ret >= 0) {
			status &= SPI_STATUS_MASK;
			if ((g_focal_pdata->fw_is_running && (0x01 == status))
					|| (!g_focal_pdata->fw_is_running && (0x00 == status))) {
				break;
			} else {
				TS_LOG_DEBUG("fw:%d,spi_st:%x", g_focal_pdata->fw_is_running, (int)status);
			}
		}
	}

	if (i >= BUSY_QUERY_TIMEOUT) {
		TS_LOG_ERR("%s:spi is busy\n", __func__);
		return -EIO;
	}

	udelay(CS_HIGH_DELAY);
	return (int)status;
}

static int fts_cmd_wirte(u8 ctrl, u8 *cmd, u8 len)
{
	int ret = 0;
	int i = 0;
	int pos = 0;
	u16 crc = 0;
	u8 buf[MAX_COMMAND_LENGTH] = { 0 };

	if ((len <= 0) || (len >= MAX_COMMAND_LENGTH - 4)) {
		TS_LOG_ERR("%s:command length(%d) fail\n", __func__, len);
		return -EINVAL;
	}

	if (NULL == cmd) {
		TS_LOG_ERR("%s:command is NULL\n", __func__);
		return -EINVAL;
	}

	buf[pos++] = COMMAND_PACKAGE;
	buf[pos++] = ctrl | (len & 0x0F);
	for (i = 0; i < len; i++) {
		buf[pos++] = cmd[i];
	}

	if ((ctrl & BIT(CTRL_CMD_CRC_BIT))) {
		crckermit(buf, pos, &crc);
		buf[pos++] = crc & 0xFF;
		buf[pos++] = (crc >> 8) & 0xFF;
	}

	ret = fts_spi_write(buf, pos);

	return ret;
}

static int fts_boot_write(u8 *cmd, u8 cmdlen, u8 *data, u32 datalen)
{
	int ret = 0;
	u16 crc = 0;
	u8 *txbuf = NULL;
	u32 txlen = 0;
	u8 ctrl = WRITE_CMD;

	if ((!cmd) || (0 == cmdlen) || (datalen > FTS_PACKAGE_SIZE_SPI)) {
		TS_LOG_ERR("%s parameter is invalid\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&g_focal_pdata->spilock);

	/* wait spi idle */
	ret = fts_wait_idle();
	if (ret < 0) {
		TS_LOG_ERR("%s:wait spi idle fail\n", __func__);
		goto err_boot_write;
	}

	/* write cmd */
	if ((NULL == data) || (0 == datalen)) {
		ctrl &= ~BIT(CTRL_DATA_CRC_BIT);
	}

	ret = fts_cmd_wirte(ctrl, cmd, cmdlen);
	if (ret < 0) {
		TS_LOG_ERR("%s:command package wirte fail\n", __func__);
		goto err_boot_write;
	}

	if (data && datalen) {
		/* wait spi idle */
		ret = fts_wait_idle();
		if (ret < 0) {
			TS_LOG_ERR("%s:wait spi idle from cmd packet fail\n", __func__);
			goto err_boot_write;
		}

		/* write data */
		/* spi DMA_MODE transfer don't support static stack point in KASAN test, so we neet alloc memory to transfer data. */
		if (datalen > SPI_BUF_LENGTH - SPI_HEADER_LENGTH || g_ts_kit_platform_data.spidev0_chip_info.com_mode == DMA_MODE) {
			txbuf = kzalloc(datalen + SPI_HEADER_LENGTH, GFP_KERNEL);
			if (NULL == txbuf) {
				TS_LOG_ERR("%s:txbuf kzalloc fail\n", __func__);
				ret = -ENOMEM;
				goto err_boot_write;
			}
		} else {
			txbuf = spibuf;
		}
		memset(txbuf, 0xFF, datalen + SPI_HEADER_LENGTH);
		txbuf[0] = DATA_PACKAGE;
		memcpy(txbuf + 1, data, datalen);
		txlen = datalen + 1;
		if (ctrl & BIT(CTRL_DATA_CRC_BIT)) {
			crckermit(txbuf, txlen, &crc);
			txbuf[txlen++] = crc & 0xFF;
			txbuf[txlen++] = (crc >> 8) & 0xFF;
		}
		ret = fts_spi_write(txbuf, txlen);
		if (ret < 0) {
			TS_LOG_ERR("%s:data wirte fail\n", __func__);
		}

		if (((txbuf) && (datalen > SPI_BUF_LENGTH - SPI_HEADER_LENGTH)) ||
			((txbuf) && (g_ts_kit_platform_data.spidev0_chip_info.com_mode == DMA_MODE))) {
			kfree(txbuf);
			txbuf = NULL;
		}
	}
err_boot_write:
	mutex_unlock(&g_focal_pdata->spilock);
	return ret;
}

static int fts_fw_write(u8 addr, u8 *data, u32 datalen)
{
	u8 cmd[3] = { 0 };

	cmd[0] = addr;
	cmd[1] = (datalen >> 8) & 0xFF;
	cmd[2] = datalen & 0xFF;

	return fts_boot_write(cmd, 3, data, datalen);
}

int fts_write(u8 *writebuf, u32 writelen)
{
	int ret = 0;
	u32 cmdlen = 0;

	if ((NULL == writebuf) || (0 == writelen)) {
		TS_LOG_ERR("%s writebuf is null/writelen is 0\n", __func__);
		return -EINVAL;
	}

	ret = fts_check_specail_cmd(writebuf[0], &cmdlen);
	if (0 == ret) {
		TS_LOG_DEBUG("fw cmd");
		ret = fts_fw_write(writebuf[0], writebuf + 1, writelen - 1);
	} else {
		TS_LOG_DEBUG("boot cmd");
		if (cmdlen == writelen)
			ret = fts_boot_write(writebuf, cmdlen, NULL, 0);
		else
			ret = fts_boot_write(writebuf, cmdlen, writebuf + cmdlen, writelen - cmdlen);
	}

	return ret;
}

static int fts_boot_read(u8 *cmd, u8 cmdlen, u8 *data, u32 datalen)
{
	int ret = 0;
	u16 crc = 0;
	u16 crc_read = 0;
	u8 ctrl = READ_CMD;
	u8 *txbuf = NULL;
	u32 txlen = 0;

	if ((!data) || (0 == datalen) || (datalen > FTS_PACKAGE_SIZE_SPI)) {
		TS_LOG_ERR("%s parameter is invalid\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&g_focal_pdata->spilock);

	if (cmd && cmdlen) {
		/* wait spi idle */
		ret = fts_wait_idle();
		if (ret < 0) {
			TS_LOG_ERR("%s:wait spi idle fail\n", __func__);
			goto boot_read_err;
		}

		/* write cmd */
		ret = fts_cmd_wirte(ctrl, cmd, cmdlen);
		if (ret < 0) {
			TS_LOG_ERR("%s:command package wirte fail\n", __func__);
			goto boot_read_err;
		}

		/* wait spi idle */
		ret = fts_wait_idle();
		if (ret < 0) {
			TS_LOG_ERR("%s:wait spi idle from cmd packet fail\n", __func__);
			goto boot_read_err;
		}
	}
	/* write data */
	/* spi DMA_MODE transfer don't support static stack point in KASAN test, so we neet alloc memory to transfer data. */
	if (datalen > SPI_BUF_LENGTH - SPI_HEADER_LENGTH || g_ts_kit_platform_data.spidev0_chip_info.com_mode == DMA_MODE) {
		txbuf = kzalloc(datalen + SPI_HEADER_LENGTH, GFP_KERNEL);
		if (NULL == txbuf) {
			TS_LOG_ERR("%s:txbuf kzalloc fail\n", __func__);
			ret =  -ENOMEM;
			goto boot_read_err;
		}
	} else {
		txbuf = spibuf;
	}
	memset(txbuf, 0xFF, datalen + SPI_HEADER_LENGTH);
	txbuf[0] = DATA_PACKAGE;
	txlen = datalen + 1;
	if (ctrl & BIT(CTRL_DATA_CRC_BIT)) {
		txlen = txlen + 2;
	}
	ret = fts_spi_read(txbuf, txlen);
	if (ret < 0) {
		TS_LOG_ERR("%s:data wirte fail\n", __func__);
		goto boot_read_err;
	}

	if (ctrl & BIT(CTRL_DATA_CRC_BIT)) {
		crckermit(txbuf, txlen - 2, &crc);
		crc_read = (txbuf[txlen - 1] << 8) + txbuf[txlen - 2];
		if (crc != crc_read) {
			TS_LOG_ERR("%s:crc(r) check fail,crc calc:%04x read:%04x\n", __func__, crc, crc_read);
			ret = -EIO;
			goto boot_read_err;
		}
	}

	memcpy(data, txbuf + 1, datalen);
boot_read_err:
	if (((txbuf) && (datalen > SPI_BUF_LENGTH - SPI_HEADER_LENGTH)) ||
		((txbuf) && (g_ts_kit_platform_data.spidev0_chip_info.com_mode == DMA_MODE))) {
		kfree(txbuf);
		txbuf = NULL;
	}
	mutex_unlock(&g_focal_pdata->spilock);
	return ret;
}

static int fts_fw_read(u8 addr, u8 *data, u32 datalen)
{
	u8 cmd[3] = { 0 };

	cmd[0] = addr;
	cmd[1] = (datalen >> 8) & 0xFF;
	cmd[2] = datalen & 0xFF;

	return fts_boot_read(cmd, 3, data, datalen);
}

int fts_read(u8 *writebuf, u32 writelen, u8 *readbuf, u32 readlen)
{
	int ret = 0;
	u32 cmdlen = 0;

	if ((NULL == readbuf) || (0 == readlen)) {
		TS_LOG_ERR("readbuf/readlen is invalid in fts_read\n");
		return -EINVAL;
	}

	if ((NULL == writebuf) || (0 == writelen)) {
		ret = fts_boot_read(NULL, 0, readbuf, readlen);
	} else {
		ret = fts_check_specail_cmd(writebuf[0], &cmdlen);
		if (0 == ret) {
			TS_LOG_DEBUG("fw cmd");
			ret = fts_fw_read(writebuf[0], readbuf, readlen);
		} else {
			TS_LOG_DEBUG("boot cmd");
			ret = fts_boot_read(writebuf, cmdlen, readbuf, readlen);
		}
	}

	return ret;
}
