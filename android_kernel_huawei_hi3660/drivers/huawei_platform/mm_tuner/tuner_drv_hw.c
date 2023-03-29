/**************************************************************************//**
 *
 *  @file		tuner_drv_hw.c
 *
 *  @brief		The HW Wrapping Layer for Tmm Tuner Driver
 *
 *  @data		2011.07.25
 *
 *  @author	K.Kitamura(*)
 *  @author	K.Okawa(KXDA3)
 ***************************************************************************//*
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

/******************************************************************************
 * include
 ******************************************************************************/
#include "tuner_drv.h"
#include "tuner_drv_hw.h"

#include <linux/irq.h>
#include <linux/gpio.h>

/******************************************************************************
 * data
 ******************************************************************************/
static bool g_tuner_irq_flag = false;

	/* Configuration registers list for slave i/f. */
	/* Don't Edit from here */
struct snglreg slvif_cfgregs[] = {
	{ Main2, 0x6B, 0x20, 0x20 },	/* #0  EXINTSET.SLVINTEN = 1 */
	{ Main1, 0xE2, 0xF0, 0x90 },	/* #1  INTSET5.ISEGSEL[3:0] = 9 */
	{ Main1, 0xD9, 0x1F, 0x01 },	/* #2  STM_SYNCSEL[4:0] DOSET4[4:0] = 1 */
	{ Main2, 0x66, 0xFF, 0x00 },	/* #3  PKTWLSET1.DATA_WINDOW0[4:0], PKTWLSET2.LOWER_LEVEL0[2:0] */
	{ Main2, 0x60, 0x40, 0x00 },	/* #4  PKTBUFCTL.ECONFIG[1:0] Set byte order of slave i/f */
	{ Main2, 0x65, 0x10, 0x10 },	/* #5  PKTSYNCC3[7:4] NULLOFF2=ON */
	{ Main2, 0x70, 0x46, 0x00 },	/* #6  IFPWDSET1[6].[2:1] PKTBUF0,INTGEN0,PKTPACK0 ON*/
	{ END_SLVCFG, 0x00, 0x00, 0x00 }		/* End Mark. Do not remove this line */
};

extern struct _mmtuner_cntxt g_cnt;

extern struct spi_drvdata * g_spi_drvdata;

/******************************************************************************
 * code area
 ******************************************************************************/

/**************************************************************************//**
 * interruption registration control of a driver
 *
 * @date		2011.08.26
 * @author		M.Takahashi(*)
 *
 * @retval 	0	normal
 * @retval		<0	error
 ******************************************************************************/
int tuner_drv_hw_reqirq(void)
{
	/* the sub-system of IRQ has been already activated */
	if (g_tuner_irq_flag == true) {
//		pr_debug("IRQ (#%d) is already active, so do nothing\n", TUNER_CONFIG_INT);
		return 0;
	}

	/* IRQ status flag: on */
	g_tuner_irq_flag = true;

	return 0;
}

/**************************************************************************//**
 * interruption registration release control of a driver
 *
 * @date		2011.08.26
 * @author		M.Takahashi(*)
 ******************************************************************************/
void tuner_drv_hw_freeirq(void)
{
	if (g_tuner_irq_flag == false) {
		/* IRQ line is not active */
//		pr_debug("IRQ (#%d) is not active, so do nothing.\n", TUNER_CONFIG_INT);
		return;
	}
	
	/* flag off */
	g_tuner_irq_flag = false;
}

/**************************************************************************//**
 * Write masked bits of the Register. (Read and Modified Write)
 *
 * @date	2016.03.03
 *
 * @author K.Okawa (IoT-Sol.)
 *
 * @retval 0					Normal end
 * @retval <0					error (refer the errno)
 *
 * @param [in] bank	register bank enumerator
 * @param [in] adr	start address for continuous write
 * @param [in] mask	continuous write length
 * @param [in] wd		write data
 ******************************************************************************/
int tuner_drv_hw_rmw_reg(enum _reg_bank bank, uint8_t adr, uint8_t mask, uint8_t wd)
{
	int ret = 0;
	uint8_t data = 0;

	if (mask == 0x00) {
		pr_warn("%s(): Bitmask is 0x00, so write nothing.\n", __FUNCTION__);
		goto _out;
	}
	if (mask == 0xff) {
		data = wd;
	} else {
		ret = tuner_drv_hw_read_reg(bank, adr, 1, &data);
		if (ret) {
			return ret;
		}
		data = (data & ~mask) | wd;
		pr_debug("%s(): R,M(m:0x%02x,d:0x%02x),W(0x%02x).\n", __FUNCTION__, mask, wd, data);
	}
	ret = tuner_drv_hw_write_reg(bank, adr, 1, &data);
	if (ret) {
		return ret;
	}

_out:
	return 0;
}



/**************************************************************************//**
 * @brief Set the event (interrupt) condition.
 *
 * This function set some specified interrupt (event) conditions,
 * and, be enable the interrupt sub system.
 *
 * @date		2014.08.01
 * @author		K.Okawa(KXDA3)
 *
 * @retval 0			normal
 * @retval <0			error
 ******************************************************************************/
int tuner_drv_hw_setev(TUNER_DATA_EVENT *ev)
{
	int ret = 0;
	uint8_t buf[2] = { 0x00, 0x00 };

	if (ev->set.mode == TUNER_EVENT_MODE_ADD) {
		/* read INTDEF1 and INTDEF2 */
		ret = tuner_drv_hw_read_reg(Main1,	0xDC, 2, buf);
		if (ret) {
			pr_err("Read INTDEF1/2, failed\n");
			return ret;
		}
		buf[0] |= ev->set.intdef1;
		buf[1] |= ev->set.intdef2;
	} else {	/* Overwrite mode: TUNER_EVENT_MODE_OVW */
		buf[0] = ev->set.intdef1;
		buf[1] = ev->set.intdef2;
	}

	/* write INTDEF1 and INTDEF2 */
	ret = tuner_drv_hw_write_reg(Main1, 0xDC, 2, buf);
	if (ret) {
		pr_err("Write INTDEF1/2, fail.\n");
		return ret;
	}

	/* write INTSET1[3](NINTEN) and NITSET1[0](INTMD) */
	ret = tuner_drv_hw_rmw_reg(Main1, 0xDE, 0x09, ev->set.intset1);
	if (ret) {
		pr_err("Write INTSET1.NINTEN/INTMD, failed\n");
		return ret;
	}
	if ((buf[0] | (buf[1] & 0x0F)) != 0x00) {
		pr_debug("Enable system IRQ line.\n");
		ret = tuner_drv_hw_reqirq();
		if (ret) {
			pr_err("tuner_drv_hw_reqirq() failed.\n");
			return ret;
		}
	}

	return 0; /* normal exit */
}

/**************************************************************************//**
 * Clear the IRQ (interrupt) conditions.
 *
 * This function clear the specified interrupt conditions.
 * And, be disabled the IRQ sub-system, when the all interrupt conditions
 * are not active.
 *
 * @date		2014.08.05
 * @author		K.Okawa(KXDA3)
 *
 * @retval 0			normal
 * @retval <0			error
 ******************************************************************************/
int tuner_drv_hw_relev(TUNER_DATA_EVENT *ev)
{
	int ret = 0;
	uint8_t buf[2] = { 0x00, 0x00 };

	/* read INTDEF1/2 */
	ret = tuner_drv_hw_read_reg(Main1, 0xDC, 2, buf);
	if (ret) {
		pr_err("Read INTDEF1/2, failed.\n");
		return ret;
	}

	/* clear specified bits */
	buf[0] &= ~(ev->set.intdef1);
	buf[1] &= ~(ev->set.intdef2);

	/* write INTDEF1/2 */
	ret = tuner_drv_hw_write_reg(Main1, 0xDC, 2, buf);
	if (ret) {
		pr_debug("Write INTDEF1/2, failed.\n");
		return ret;
	}

	if ((buf[0] | (buf[1] & 0x0F)) == 0x00) {
		pr_debug("Disable system IRQ line.\n");
		tuner_drv_hw_freeirq();
	}

	return 0;	/* normal return */
}

#if 0

int tuner_drv_gpio_power_process(int type, int status)
{
	int ret = TUNER_DRV_OK; 

	int gpio = TUNER_DRV_GPIO_POWER_1V8 ;

	switch(type){
		case TUNER_DRV_GPIO_POWER :
			gpio = g_cnt.gpio_power; 
			break;
		case TUNER_DRV_GPIO_POWER_1V8 :
			gpio = g_cnt.gpio_power_v18 ;
			break;
		case TUNER_DRV_GPIO_POWER_NRST :
			gpio = g_cnt.gpio_nreset ;
			break;
		default :
			pr_err("gpio  type is error\n");
			return TUNER_DRV_ERROR ;
	}

#if 0
	ret = gpio_direction_output(gpio,status); 
	if(ret != TUNER_DRV_OK){	
		pr_err("gpio  tuner_power direction out fail\n");
		return ret ;
	}
#endif 

	__gpio_set_value(gpio, status);

	return ret ;

} 
#endif

void tuner_drv_power_control_startup(void)
{

	__gpio_set_value(g_cnt.gpio_nreset, TUNER_DRV_POWER_GPIO_DISABLE);

	mdelay(TUNER_DELAY);

	__gpio_set_value(g_cnt.gpio_power_v18, TUNER_DRV_POWER_GPIO_ENABLE);

	mdelay(TUNER_DELAY);

	__gpio_set_value(g_cnt.gpio_power, TUNER_DRV_POWER_GPIO_ENABLE);

	mdelay(TUNER_DELAY);

	__gpio_set_value(g_cnt.gpio_nreset, TUNER_DRV_POWER_GPIO_ENABLE);

	mdelay(TUNER_DELAY);

	__gpio_set_value(g_spi_drvdata->gpio_cs , 1);

	mdelay(TUNER_DELAY);

}

void tuner_drv_power_control_endup(void)
{

	__gpio_set_value(g_spi_drvdata->gpio_cs , 0);

	mdelay(TUNER_DELAY);

	__gpio_set_value(g_cnt.gpio_nreset, TUNER_DRV_POWER_GPIO_DISABLE);

	mdelay(TUNER_DELAY);

	__gpio_set_value(g_cnt.gpio_power, TUNER_DRV_POWER_GPIO_DISABLE);

	__gpio_set_value(g_cnt.gpio_power_v18, TUNER_DRV_POWER_GPIO_DISABLE);

}

void tuner_drv_power_control_init(void)
{

	__gpio_set_value(g_cnt.gpio_power_v18, TUNER_DRV_POWER_GPIO_DISABLE);

	__gpio_set_value(g_cnt.gpio_power, TUNER_DRV_POWER_GPIO_DISABLE);
	
	__gpio_set_value(g_cnt.gpio_nreset, TUNER_DRV_POWER_GPIO_DISABLE);
	
}


/*******************************************************************************
 * Copyright (c) 2015 Socionext Inc.
 ******************************************************************************/
