/**************************************************************************//**
 *
 *  @file		tuner_drv_hw_spi.c
 *
 *  @brief		Implementation of the hardware control layer in SPI.
 *
 *  @data		2014.08.19
 *
 *  @author	K.Okawa (KXDA3)
 *
 ****************************************************************************//*
 * Copyright (c) 2015 Socionext Inc.
 ******************************************************************************
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ******************************************************************************/
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/
/******************************************************************************
 * include
 ******************************************************************************/
#include "tuner_drv_hw.h"

#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/of_device.h>
#include <linux/device.h>

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif

/******************************************************************************
 * function
 ******************************************************************************/
static int tuner_drv_spi_probe(struct spi_device *spidev);
static int tuner_drv_spi_remove(struct spi_device *spidev);

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
static struct delayed_work tuner_dbc_work;

static void tuner_dbc_work_handler(struct work_struct *work);
#endif

// Private functions
int tuner_drv_spi_edge(int ctrl);
int tuner_drv_spi_calibration(void);

#define SPI_CMD_NUM 0xc
#define SPI_DATA_NUM 0xc
#define SPI_PRG_MAX_NUM 0x100
#define SPI_PSEQ_ADRS_INIT 0x00
#define SPI_BREAKCODE_PATTERN 0xff, 0xfe, 0x81, 0x00

/******************************************************************************
 * Macro
 ******************************************************************************/
#define CALC_LENGTH(len) (len >=  SPI_PRG_MAX_NUM) ? SPI_PRG_MAX_NUM : len;

#define SPI_TSREAD_DMYCNT 10

/* SPI configuration setting(SPI internal register address=0x08) for each mode
 SPI_DIVMSG  SPI_BREAKCODE  SPI_CONFIG_SET
 ON				OFF				0x00		: XCS_RESET=OFF , TRANSFER BREAK=DISABLE
 ON				ON				0x02		: XCS_RESET=OFF , TRANSFER BREAK=ENABLE
 OFF			OFF				0x03(0x01)  : XCS_RESET=ON  , TRANSFER BREAK=DISABLE
 OFF			ON				0x03        : XCS_RESET=ON  , TRANSFER BREAK=ENABLE  */

#define SPI_CONFIG_SET (0x03)
#define CMDBUF_POS(x) (void *)(x)
#define CMDBUF_LEN(x) (x+4)

// Normal Read command
#define SPI_READ_COMMAND  0x0b

#define BUFLEN_ALIGN(size) (size)

/******************************************************************************
 * global
 ******************************************************************************/
#define mtxLock()
#define mtxUnlock()

/******************************************************************************
 * data
 ******************************************************************************/
static struct of_device_id spi_dt_match[] = {
	{
		.compatible = "sni,tmm3spi" //Compatible node must match
	},
	{ },
};
MODULE_DEVICE_TABLE(of, spi_dt_match);

static struct spi_driver mn8855x_spi_driver = {
		.driver = {
				.name = "553spi",
				.owner = THIS_MODULE,
				.of_match_table = spi_dt_match,
		},
		.probe = tuner_drv_spi_probe,
		.remove = tuner_drv_spi_remove,
};


struct spi_drvdata *g_spi_drvdata = NULL;

int tuner_spi_sync(struct spi_device *spi, struct spi_message *message)
{
	int ret;
	__gpio_set_value(g_spi_drvdata->gpio_cs , 0);
    	ret = spi_sync(spi, message);
	__gpio_set_value(g_spi_drvdata->gpio_cs , 1);
	return ret;
}

/******************************************************************************
 * Variable
 ******************************************************************************/
static int g_edge_mode = 0;

/******************************************************************************
 * code area
 ******************************************************************************/
/**************************************************************************//**
 * Set TPM register
 *
 * The TPM (register of the tuner device) control the I/F port of
 * the tuner device.
 * TPM must be set to 0x02 when the Data-PATH (TS I/F) use SPI and
 * Control-PATH use I2C.
 *
 * @date	2014.09.10
 *
 * @author K.Okawa (KXDA3)
 *
 * @retval 0			Normal end
 * @retval <0			error
 ******************************************************************************/
int tuner_drv_hw_tsif_set_tpm(void)
{
	int ret = 0;
	uint8_t buf = 0x00;

	/* TPM[4:0] (PINCNT0[4:0]) */
	ret = tuner_drv_hw_read_reg(Main2, 0x00, 1, &buf);
	if (ret) {
		pr_err("Read PINCNT0, failed.\n");
		return ret;
	}
	if ((buf & 0x1F) != 0x0a) { /* NOT Diver Mode */

		buf = 0x02; /* CPATH:I2C, DPATH:SPI(slave-IF) */

		ret = tuner_drv_hw_write_reg(Main2, 0x00, 1, &buf);
		if (ret) {
			pr_err("write PINCNT0.TPM, failed.\n");
			return ret;
		}
	}
	return ret;
}

/**************************************************************************//**
 * Register the TS I/F driver
 *
 * @date	2013.12.10
 *
 * @author T.Abe (FSI)
 * @author K.Okawa (KXDA3)
 *
 * @retval 0					Normal end
 * @retval <0					error
 ******************************************************************************/
int tuner_drv_hw_tsif_register(void)
{
	int ret = 0;

	pr_debug("%s\n", __FUNCTION__);

	ret = spi_register_driver(&mn8855x_spi_driver);
	if (ret) {
		pr_err("spi_register_driver() failed.\n");
		return ret;
	}

	return ret;
}

/**************************************************************************//**
 * Configure the SPI-Slave I/F of the tuner device.
 *
 * @date	2013.12.10
 *
 * @author T.Abe (FSI)
 * @author K.Okawa (KXDA3)
 *
 * @retval 0					Normal end
 * @retval <0					error
 *
 * @param [in]
 ******************************************************************************/
int tuner_drv_hw_tsif_config(struct _tsif_cntxt *tc)
{
	int ret = 0;
	TUNER_DATA_TSIF *tsif = NULL;
	TUNER_DATA_EVENT iberint;
	int i=0;

	uint8_t tx[12] = { SPI_BREAKCODE_PATTERN, 0x03, 0x00, 0x08, 0x00, SPI_CONFIG_SET, 0x00, 0x00, 0x00 };
	struct spi_message msg;
	struct spi_transfer xfer;
	uint8_t pbuf_max_size = 0;
	uint8_t byte_order_set = 0;

	ret = tuner_drv_hw_tsif_set_tpm();
	if (ret) {
		pr_err("tuner_drv_hw_tsif_set_tpm() failed.\n");
		return ret;
	}

	if (tc == NULL || tc->tsif == NULL) {
		pr_err("illegal arguments.\n");
		return -EINVAL;
	}

	tsif = (TUNER_DATA_TSIF *) tc->tsif;

	/* configure the SPI(slave) I/F sub-system of Tuner device */
	memset(&xfer, 0, sizeof(struct spi_transfer));
	spi_message_init(&msg);

	xfer.tx_buf = (void *) tx;
	xfer.len = 4 + 8;
	xfer.bits_per_word = DTV_SPI_BITS_PER_WORD_VALUE; //8;
	
	spi_message_add_tail(&xfer, &msg);
	mtxLock();
	ret = tuner_spi_sync(g_spi_drvdata->spi, &msg);
	mtxUnlock();
	if (ret) {
		pr_err("spi_sync() failed. (return:%d).\n", ret);
		return ret;
	}
	
#ifdef TUNER_CONFIG_SPI_EDGE
	tuner_drv_spi_edge(1);
	ret = tuner_drv_spi_calibration();
	if (ret) {
		pr_err("tuner_drv_spi_calibration() failed. (return:%d).\n", ret);
		return ret;
	}
#endif

	// Water Line setting
	slvif_cfgregs[SLVIF_CFG_WATERLINE].param = ((uint8_t) (tsif->dwind[tc->bw]) << 3) | (tsif->thl[tc->bw]&0x7);

	// Byte order configuration
	if (tsif->spi_ts_bit_per_word == 32) {
		slvif_cfgregs[SLVIF_CFG_BYTEORDER].param = 0x40;
	} else {
		slvif_cfgregs[SLVIF_CFG_BYTEORDER].param = 0x00;
	}

	for (i = 0; slvif_cfgregs[i].bank != END_SLVCFG ; i++) {
		ret = tuner_drv_hw_rmw_reg(slvif_cfgregs[i].bank, slvif_cfgregs[i].adr, slvif_cfgregs[i].enabit, slvif_cfgregs[i].param);
		if (ret) {
			pr_err("TS slave-IF configuration, failed.\n");
			return ret;
		}
	}

	ret = tuner_drv_hw_read_reg(Main2, 0x62, 1, &pbuf_max_size);
	if (ret) {
		pr_err("Read PKTMSIZE register, failed.\n");
		return ret;
	}
	if ((pbuf_max_size & 0x0F) == 0) {
		pr_warn("PKTMSIZE.MEMSIZE0[3:0] != 0x3.\n");
	}

	// Interrupt setting
	/* IBERINT_F */
	iberint.pack = 0;
	iberint.set.mode = TUNER_EVENT_MODE_ADD;
	iberint.set.intdef1 = 0x80; /* IBERINT */
	iberint.set.intset1 = 0x09; /* NINTEN, INTMD = 1 */
	ret = tuner_drv_hw_setev(&iberint);
	if (ret) {
		pr_err("tuner_drv_setev(F) failed.\n");
		return ret;
	}

	//Check byte order configuration.
	ret = tuner_drv_hw_read_reg(Main2, 0x60, 1, &byte_order_set);
	if (ret) {
		pr_err("Main2 register read failed.\n");
		return ret;
	}
	
	return 0;
}

/**************************************************************************//**
 * Unregister the TS I/F driver
 *
 * @date	2014.08.21
 *
 * @author K.Okawa (KXDA3)
 *
 ******************************************************************************/
void tuner_drv_hw_tsif_unregister(void)
{
	spi_unregister_driver(&mn8855x_spi_driver);
}

/**************************************************************************//**
 * Get the DATAREADY flag
 *
 * This function return the DATAREADY flag of the Slave-I/F of
 * tuner device. DATAREADY flag contain OVER/UNER-Run indicator.
 * It is below there bit position.
 * OVER-Run is bit-2. UNDER-Run is bit-1, DATA-Ready is bit-0.
 *
 * @date	2014.08.27
 *
 * @author K.Okawa (KXDA3)
 *
 * @retval >=0		DATAREADY flag (casted from uint8_t)
 * @retval <0			error
 ******************************************************************************/
int tuner_drv_hw_tsif_get_dready(void)
{
	int ret = 0;
	uint8_t tx[12] = { SPI_BREAKCODE_PATTERN, 0x03, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	uint8_t rx[12] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	struct spi_message msg;
	struct spi_transfer xfer;

	memset(&xfer, 0, sizeof(struct spi_transfer));
	spi_message_init(&msg);

	xfer.tx_buf = CMDBUF_POS(tx);
	xfer.rx_buf = CMDBUF_POS(rx);
	xfer.len = CMDBUF_LEN(8);
	xfer.bits_per_word = DTV_SPI_BITS_PER_WORD_VALUE; 

	spi_message_add_tail(&xfer, &msg);
	mtxLock();
	ret = tuner_spi_sync(g_spi_drvdata->spi, &msg);
	mtxUnlock();
	if (ret) {
		pr_err("spi_sync() failed (rc:%d)\n", ret);
		return ret;
	}

	return (int) rx[11];
}

/**************************************************************************//**
 * Send the transaction command to synchronize slave I/F of tuner.
 *
 * This function send the packet synchronization command.
 * It initialize the read pointer and clear the FIFO buffer.
 *
 * @date	2014.08.27
 *
 * @author K.Okawa (KXDA3)
 *
 * @retval 0		normal
 * @retval <0		error
 ******************************************************************************/
int tuner_drv_hw_tsif_sync_pkt(void)
{
	int ret = 0;
	uint8_t tx[8] = { SPI_BREAKCODE_PATTERN, 0xd8, 0x00, 0x00, 0x00 };
	struct spi_message msg;
	struct spi_transfer xfer;

	memset(&xfer, 0, sizeof(struct spi_transfer));
	spi_message_init(&msg);

	xfer.tx_buf = CMDBUF_POS(tx);
	xfer.len = CMDBUF_LEN(4);
	xfer.bits_per_word = DTV_SPI_BITS_PER_WORD_VALUE; 

	spi_message_add_tail(&xfer, &msg);
	mtxLock();
	ret = tuner_spi_sync(g_spi_drvdata->spi, &msg);
	mtxUnlock();
	if (ret) {
		pr_err("spi_sync() failed (rc:%d)\n", ret);
		return ret;
	}

	return 0;
}

/**************************************************************************//**
 * Get the TS packets of the appointed number.
 *
 * @date	2014.08.26
 *
 * @author K.Okawa (KXDA3)
 *
 * @retval >=0		Normal end (number of the get packet)
 * @retval <0			error (refer the errno)
 *
 * @param [in] num			num of packets
 * @param [out] pktbuf		packet storage
 * @param [in] pktsize		packet size enumerator
 ******************************************************************************/
int tuner_drv_hw_tsif_get_pkts(struct _tsif_cntxt *tc)
{
	int ret;
	struct spi_message msg;
	struct spi_transfer xfer[2];
	int sum = 0;
	TUNER_DATA_TSIF *tsif = tc->tsif;
	unsigned int ts_rdelay = 0;

	/* TS packet size: 188Byte, num of TS packets:256 */
	/* Break code + Packet Read commnad */
	uint8_t tx[9 + 17] = { SPI_BREAKCODE_PATTERN, /* 4 byte length */
	SPI_READ_COMMAND, 0x00, 0x00, 0xFF, 0x00, /* 5 byte length command ex. 0x0b .. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* This 16 bytes is dummy cycles */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* for extend read command */
	0x00 }; /* Last byte is for additional cycle in edge mode and extend read is enabled */

#ifdef TUNER_CONFIG_SPI_EDGE
	ts_rdelay = ts_rdelay + 1;  //EDGE==1
#endif  //TUNER_CONFIG_SPI_EDGE

	if (!g_spi_drvdata) {
		pr_err("SPI I/F not active.\n");
		return -ENXIO;
	}
	if (!tc->pktbuf) {
		pr_err("TS buffer not found.\n");
		return -EINVAL;
	}
	if ((!(tc->tsif->ts_pkt_type)) == TUNER_DRV_TS_TSTAMP) {
		pr_err("not support the Time-Stamp TS");
		return -EINVAL;
	}
	
	memset(xfer, 0, sizeof(xfer));
	spi_message_init(&msg);

	tx[4 + 2] = (tc->ts_rxpkt_num - 1) >> 8;
	tx[4 + 3] = (tc->ts_rxpkt_num - 1) & 0xff;
	// break | read mode | edge mode ||  ts_rdelay(=edge delay+DMYCNT delay)
	//       |   nomral  |    off    ||   0
	//       |   nomral  |    on     ||   1
	//  on   |   nomral  |    off    ||   0
	//  on   |   nomral  |    on     ||   1
	//       |   extend  |    off    ||   DMYCNT
	//       |   extned  |    on     ||   DMYCNT+1
	//  on   |   extend  |    off    ||   DMYCNT
	//  on   |   extend  |    on     ||   DMYCNT+1
	xfer[0].tx_buf = CMDBUF_POS(tx);
	xfer[0].len = CMDBUF_LEN(5) + ts_rdelay;
	xfer[0].bits_per_word = DTV_SPI_BITS_PER_WORD_VALUE; //8;

	spi_message_add_tail(&xfer[0], &msg);

	xfer[1].rx_buf = (void *) (tc->pktbuf + tc->pwr);
	xfer[1].bits_per_word = tsif->spi_ts_bit_per_word;
	xfer[1].len = tc->ts_rx_size;
	spi_message_add_tail(&xfer[1], &msg);

	mtxLock();
	ret = tuner_spi_sync(g_spi_drvdata->spi, &msg);
	mtxUnlock();
	if (ret) {
		pr_err("spi_sync() failed (rc:%d)\n", ret);
		return ret;
	}

	tc->pwr += tc->ts_rx_size;

	if (tc->pwr == tc->ts_pktbuf_size) {
		tc->pwr = 0;
	}

	return sum;
}

/**************************************************************************//**
 * probe function called by spi_register_driver()
 *
 * @date	2013.12.10
 *
 * @author T.Abe (FSI)
 * @author K.Okawa (KXDA3)
 *
 * @retval 0			Normal end
 * @retval <0			error (refer the errno)
 *
 * @param [in] spidev	pointer to the "spi_device" structure
 ******************************************************************************/
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
static int tuner_drv_check_self(void)
{
        int ret = TUNER_CHECK_OK ;

        tuner_drv_power_control_startup();

        ret = tuner_drv_check_i2c_connection() ;

        if(ret == TUNER_DRV_OK){

                pr_err("tuner drv check self process ok.\n");

        }else{

                pr_err("tuner drv check self process failed.\n");

        }

        tuner_drv_power_control_endup() ;

        return ret ;
}

#endif

static int tuner_drv_spi_probe(struct spi_device *spidev)
{
	int ret = 0;
	struct spi_drvdata *drvdata= NULL;
	struct device_node *node = spidev->dev.of_node;
	uint32_t value = 0;

	if (g_spi_drvdata != NULL) {
		pr_err("SPI I/F not active.\n");
		return -EBUSY;
	}
	if (NULL == spidev) {
		pr_err("illegal argument.\n");
		return -EINVAL;
	}

	if (!(drvdata = kzalloc(sizeof(*drvdata), GFP_KERNEL))) {
		pr_err("memory allocation failed.\n");
		return -ENOMEM;
	}

	drvdata->spi = spidev;
	spin_lock_init(&drvdata->spi_lock);
	spi_set_drvdata(spidev, drvdata);
	g_spi_drvdata = drvdata;

	ret = spi_setup(spidev);
	if (ret) {
		pr_err("spi_setup() failed.\n");
		return ret;
	}

	ret = of_property_read_u32(node, "gpio_cs", &value);
	if(ret){
		pr_err("get gpio_power dts failed.\n");
		return ret;
	}
	g_spi_drvdata->gpio_cs = value;

	ret =gpio_request(g_spi_drvdata->gpio_cs ,"tuner_gpio_cs");
	if(ret != 0){
		pr_err("gpio request tuner_gpio_cs fail\n") ;
		return ret ;
	}

	ret = gpio_direction_output(g_spi_drvdata->gpio_cs ,0); 
	if(ret != 0){	
		pr_err("gpio tuner_nreset direction out fail\n");
	}

	__gpio_set_value(g_spi_drvdata->gpio_cs , 0);

#ifdef CONFIG_HUAWEI_HW_DEV_DCT

        INIT_DELAYED_WORK(&tuner_dbc_work, tuner_dbc_work_handler);

	schedule_delayed_work(&tuner_dbc_work,msecs_to_jiffies(TUNER_CHECK_DELAY_TIME));

#endif

	return ret;
}

#ifdef CONFIG_HUAWEI_HW_DEV_DCT

static void tuner_dbc_work_handler(struct work_struct *work){

        if(TUNER_CHECK_OK== tuner_drv_check_self()){

                set_hw_dev_flag(DEV_I2C_DTV);

                pr_info("tuner drv check self ok.\n");
        }else{
                pr_info("tuner drv check self failed.\n");
        }

}

#endif

/**************************************************************************//**
 * remove function called by spi_register_driver()
 *
 * @date	2013.12.10
 *
 * @author T.Abe (FSI)
 * @author K.Okawa (KXDA3)
 *
 * @retval 0			Normal end
 * @retval <0			error (refer the errno)
 *
 * @param [in] spidev	pointer to the "spi_device" structure
 ******************************************************************************/
static int tuner_drv_spi_remove(struct spi_device *spidev)
{

	gpio_free(g_spi_drvdata->gpio_cs);
	
	spi_set_drvdata(spidev, NULL);
	kfree(g_spi_drvdata);
	g_spi_drvdata = NULL;

	return 0;
}

/**************************************************************************//**
 * TS read Calibration
 *
 * @date	2014.12.02
 *
 * @author K.Fukuzaki (KXDA3)
 *
 * @retval 0		normal
 * @retval <0		error
 ******************************************************************************/
int tuner_drv_spi_calibration(void)
{
	int ret = 0;
	uint8_t tx1[7 + 32] = { SPI_BREAKCODE_PATTERN, 0x4b, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; //Calibration command
	uint8_t rx1[7 + 4 * 8] = { 0x00 }; //Read calib. result
	uint8_t tx2[12] = { SPI_BREAKCODE_PATTERN, 0x03, 0x00, 0x0D, 0x00, 0x00,
			0x00, 0x00, 0x00 }; //Delay set
	struct spi_message msg;
	struct spi_transfer xfer[3];
	int i,k;
	int sp, ep, pos;

	memset(xfer, 0, sizeof(xfer));
	spi_message_init(&msg);

	//Calibration Command
	xfer[0].tx_buf = CMDBUF_POS(tx1);
	xfer[0].rx_buf = CMDBUF_POS(rx1);
	xfer[0].len = CMDBUF_LEN((3+32));
	xfer[0].bits_per_word = DTV_SPI_BITS_PER_WORD_VALUE;
	
	spi_message_add_tail(&xfer[0], &msg);

	mtxLock();
	ret = tuner_spi_sync(g_spi_drvdata->spi, &msg);
	mtxUnlock();
	if (ret) {
		pr_err("spi_sync() failed (rc:%d)\n", ret);
		return ret;
	}

	//Calculation delay
	sp = 0;
	ep = 0;
	k = 0 ;
	for (i = 0; i < 8; i++) {
		k = (4 + 5 + (i * 4)) ;
		if (rx1[k] == 0x72) {
			if (sp == 0) {
				sp = i + 1;
			} else {
				ep = i + 1;
			}
		}
	}

	//Delay set
	pos = (int) ((sp + ep) / 2);
	tx2[4 + 4] = pos;

	memset(xfer, 0, sizeof(xfer));
	spi_message_init(&msg);
	xfer[2].tx_buf = CMDBUF_POS(tx2);
	xfer[2].len = CMDBUF_LEN(8);
	xfer[2].bits_per_word = DTV_SPI_BITS_PER_WORD_VALUE;
	spi_message_add_tail(&xfer[2], &msg);
	mtxLock();
	ret = tuner_spi_sync(g_spi_drvdata->spi, &msg);
	mtxUnlock();
	if (ret) {
		pr_err("spi_sync() failed (rc:%d)\n", ret);
		return ret;
	}

	if (sp == 0) {
		pr_err("spi_calibration() failed. sp = %d", sp);
		return -1;
	} else {
		return 0;
	}
	
}

/**************************************************************************//**
 * EDGE mode setting
 *
 * @date        2015.11.05
 *
 * @author Y.Uramoto (SOC)
 *
 * @retval 0            normal
 * @retval <0           error
 ******************************************************************************/
int tuner_drv_spi_edge(int ctrl)
{
	int ret = 0;
	uint8_t tx[12] = { SPI_BREAKCODE_PATTERN, 0x03, 0x00, 0x0c, 0x00, 0x00,
			0x00, 0x00, 0x00 };
	struct spi_message msg;
	struct spi_transfer xfer;

	memset(&xfer, 0, sizeof(struct spi_transfer));
	spi_message_init(&msg);

	tx[4 + 4] = (ctrl << 7) | SPI_TSREAD_DMYCNT;

	xfer.tx_buf = CMDBUF_POS(tx);
	xfer.len = CMDBUF_LEN(8);
	xfer.bits_per_word = DTV_SPI_BITS_PER_WORD_VALUE;
	spi_message_add_tail(&xfer, &msg);
	mtxLock();
	ret = tuner_spi_sync(g_spi_drvdata->spi, &msg);
	mtxUnlock();
	if (ret) {
		pr_err("spi_sync() return with %d", ret);
		return ret;
	}

	if(g_edge_mode != ctrl)
		g_edge_mode = ctrl;
	
	return 0;
}

/*******************************************************************************
 * Copyright (c) 2015 Socionext Inc.
 ******************************************************************************/
