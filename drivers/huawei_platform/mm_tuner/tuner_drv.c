/**************************************************************************//**
 *
 *  @file		tuner_drv.c
 *
 *  @brief		The implementation that is independence in physical I/F.
 *
 *  @data		2011.07.25
 *
 *  @author	K.Kitamura(*)
 *  @author	K.Okawa(KXDA3)
 *
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
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/

/******************************************************************************
 * include
 ******************************************************************************/
#include "tuner_drv.h"
#include "tuner_drv_hw.h"
#include "version.h"
#include <linux/i2c.h>

#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
//For Timeout procedure
#include <linux/jiffies.h>
#include <linux/timer.h>

/******************************************************************************
 * variables
 ******************************************************************************/

/******************************************************************************
 * data
 ******************************************************************************/
struct _mmtuner_cntxt g_cnt;	/* mm_tuner driver context */

static struct _tsif_cntxt	g_tscnt;	/* TS I/F context */

struct devone_data;

/** @brief TS packet size.
 *
 * Array index of this constant array is
 * equivalent to ::_ts_pkt_type (enumeration type).
 */
static const size_t g_ts_pkt_size[3] = { 188, 204, 192 };

static const struct _mmtuner_version mmtuner_version = {
		MMTUNER_DEVICE,
		MMTUNER_MAJOR, MMTUNER_MINOR, MMTUNER_HOTFIX,
		MMTUNER_RC, MMTUNER_DESC
};

tuner_cpathid_t cpath_id = DVR_MASTER;

/******************************************************************************
 * function
 ******************************************************************************/
static ssize_t tuner_module_entry_read(struct file* FIle, char __user * Buffer,
		size_t Count, loff_t* OffsetPosition);
static ssize_t tuner_module_entry_write(struct file* FIle, const char __user * Buffer,
		size_t Count, loff_t* OffsetPosition);
static unsigned int tuner_module_entry_poll(struct file *file,
		struct poll_table_struct *poll_tbl);

static long tuner_module_entry_ioctl( struct file *file,
		unsigned int uCommand,
		unsigned long uArgument );

static int tuner_module_entry_open(struct inode* Inode, struct file* FIle);
static int tuner_module_entry_close(struct inode* Inode, struct file* FIle);
static int tuner_probe(struct platform_device *pdev);
static int __exit tuner_remove(struct platform_device *pdev);
static int tuner_pm_suspend(struct device *dev);
static int tuner_pm_resume(struct device *dev);
static int tuner_drv_read_regs(tuner_rwreg_t *rw, int num);
static int tuner_drv_write_regs(tuner_rwreg_t *rw, int num);

static int tuner_drv_tsif_set_cntxt(struct _tsif_cntxt *tc);
//static int __inline tuner_drv_bwseg(enum _bw_seg *pbw);
static int  tuner_drv_bwseg(enum _bw_seg *pbw);
static int tuner_drv_tsif_start(void);
static int tuner_drv_tsif_stop(void);
static int tuner_drv_tsif_pktsize(void);
static int tuner_drv_tsif_blksize(void);
static int tuner_drv_tsif_th(void *arg);

static void tsread_timer_handler(unsigned long data);
static struct timer_list tsread_timer;

/* entry point */
static struct file_operations TunerFileOperations = {
		.owner = THIS_MODULE,
		.read = tuner_module_entry_read,
		.write = tuner_module_entry_write,
		.poll = tuner_module_entry_poll,
		.unlocked_ioctl = tuner_module_entry_ioctl,
		.open = tuner_module_entry_open,
		.release = tuner_module_entry_close
};

static const struct dev_pm_ops mmtuner_driver_pm_ops = {
		.suspend = tuner_pm_suspend,
		.resume = tuner_pm_resume,
};

static struct platform_driver mmtuner_driver = {
		.probe = tuner_probe,
		.remove = tuner_remove,
		.driver = {
				.name = TUNER_CONFIG_DRIVER_NAME,
				.owner = THIS_MODULE,
				.pm = &mmtuner_driver_pm_ops,
		}
};

static struct platform_device *mmtuner_device = NULL;
static struct class *device_class = NULL;

/******************************************************************************
 * code area
 ******************************************************************************/

/**************************************************************************//**
 * probe control of a driver
 *
 * @date		2011.08.02
 *
 * @author		K.Kitamura(*)
 *
 * @retval		0		normal
 * @retval		<0		error
 *
 * @param [in] pdev	pointer to the structure "platform_device"
 ******************************************************************************/
static int tuner_probe(struct platform_device *pdev)
{

	/* register the driver */
	if (register_chrdev(TUNER_CONFIG_DRV_MAJOR, TUNER_CONFIG_DRIVER_NAME,
			&TunerFileOperations)) {
		pr_err("register_chrdev() failed (Major:%d).\n",
				TUNER_CONFIG_DRV_MAJOR);
		return -EINVAL;
	}

	return 0;
}

/**************************************************************************//**
 * remove control of a driver
 *
 * @date		2011.08.02
 *
 * @author		K.Kitamura(*)
 *
 * @retval		0	normal
 * @retval		<0	error
 *
 * @param [in] pdev	pointer to the structure "platform_device"
 ******************************************************************************/
static int __exit tuner_remove(struct platform_device *pdev)
{

	/* un-register driver */
	unregister_chrdev(TUNER_CONFIG_DRV_MAJOR, TUNER_CONFIG_DRIVER_NAME);

	return 0;
}

/*************************************************************************//***
 * suspend control of a driver
 *
 * @date		2014.02.21
 *
 * @author		T.Abe(FSI)
 *
 * @retval		0	normal
 * @retval		<0	error
 *
 * @param [in] dev	device
 ******************************************************************************/
static int tuner_pm_suspend(struct device *dev)
{

	/* TODO: implement suspend scheme of the tuner device */
	if(dev == NULL){
		pr_err("tuner pm suspend failed.\n");
	}
	
	return 0;
}

/*************************************************************************//***
 * resume control of a driver
 *
 * @date		2014.02.21
 *
 * @author		T.Abe(FSI)
 *
 * @retval		0	normal
 * @retval		<0	error
 *
 * @param [in] dev	device
 ******************************************************************************/
static int tuner_pm_resume(struct device *dev)
{
	int ret ;

	if(dev == NULL){
		pr_err("tuner pm resume.\n");
	}

	/* TODO: implement the resume scheme of the tuner device */
	ret = tuner_drv_hw_tsif_set_tpm();
	if (ret) {
		pr_err("write TPM bits failed.\n");
		return ret;
	}
	
	return ret;
}

/**************************************************************************//**
 * open control of a driver
 *
 * @date		2011.08.02
 * @author		K.Kitamura(*)
 *
 * @retval		0	normal
 * @retval		<0	error
 *
 * @param [in] Inode, FIle
 ******************************************************************************/
static int tuner_module_entry_open(struct inode* Inode, struct file* FIle)
{

	if(Inode == NULL || FIle == NULL){
		pr_err("tuner module entry open\n");
	}
	
	if (g_cnt.opcnt > 0) {
		pr_err("It's being used.\n");
		return -EBUSY;
	}

	g_cnt.opcnt++;

	return 0;
}

/**************************************************************************//**
 * close control of a driver
 *
 * @date		2011.08.02
 * @author		K.Kitamura(*)
 *
 * @retval		0	normal
 * @retval		<0	error
 *
 * @param [in] Inode, FIle	regular argument for linux system call
 ******************************************************************************/
static int tuner_module_entry_close(struct inode* Inode, struct file* FIle)
{
	struct devone_data *dev = NULL;

	if (g_cnt.opcnt == 0) {	/* not open */
		pr_err("NOT IN USE.\n");
		return -ENODEV;
	} else {
		g_cnt.opcnt--;
	}

	if (g_cnt.opcnt == 0) { /* All logical device is closed. */
		if (FIle == NULL) {
			return -EINVAL;
		}
		dev = FIle->private_data;
		if (dev) {
			kfree(dev);
		}
	}

	return 0;
}

/**************************************************************************//**
 * @brief "read" system call of mm_tuner device
 *
 * The "read" system-call acquires the TS data stream of
 * the designated byte size.
 *
 * @date		2014.08.05
 * @author		K.Okawa(KXDA3)
 *
 * @retval >=0		normal (byte size)
 * @retval <0			error
 ******************************************************************************/
static ssize_t tuner_module_entry_read(struct file * FIle, char __user * Buffer,
		size_t Count, loff_t * OffsetPosition)
{
	size_t size = 0;
	int ret = 0;
	size_t copy_size = 0;
	size_t able_size = 0 ;

	if (!Buffer) {
		pr_err("arg. \"Buffer\" is Null\n");
		return -EINVAL;
	}
	if (Count <= 0) {
		pr_err("arg. \"Count\" is illegal (%zu)\n", Count);
		return -EINVAL;
	}

	if (g_tscnt.tsifth_wait) {
		pr_warn("TS I/F thread is NOT active (waiting...)\n");
		return -EAGAIN;
	}
	if (g_tscnt.tsread_flag != TUNER_TSREAD_IDLE) {
		pr_warn("read (TS) system call is executed in duplicate\n");
		return -EBUSY;
	} else {
		g_tscnt.tsread_flag = TUNER_TSREAD_ACTIVE;
	}

	g_tscnt.ovf = 0;	/* clear the over-flow flag of the packet buffer */

	do {
		able_size = 0;

		/* TS FIFO is empty */
		if (g_tscnt.prd == g_tscnt.pwr) {

			/* TS-Read timer */
			init_timer(&tsread_timer);
			tsread_timer.expires = jiffies + msecs_to_jiffies(TUNER_CONFIG_TSREAD_TIMEOUT);
			tsread_timer.data = (unsigned long)jiffies;
			tsread_timer.function = tsread_timer_handler;
			add_timer(&tsread_timer);

			g_tscnt.tsread_flag = TUNER_TSREAD_WAIT;
			usleep_range(100, 500);
			//lint -save -e578
			wait_event_interruptible(g_tscnt.tsread_waitq, g_tscnt.tsread_flag);
			//lint -restore
			
			del_timer(&tsread_timer);
			if (g_tscnt.tsread_flag != TUNER_TSREAD_IDLE) {
				pr_warn("Stop reading TS data.\n");
				pr_debug("TS-Read status flag: 0x%08x.\n", g_tscnt.tsread_flag);
				break;
			} else {
				g_tscnt.tsread_flag = TUNER_TSREAD_ACTIVE;
			}
			continue;
		}

		/* readable data size of TS FIFO */
		able_size = (g_tscnt.pwr > g_tscnt.prd) ?
				(g_tscnt.pwr - g_tscnt.prd) :
				(g_tscnt.ts_pktbuf_size - g_tscnt.prd);

		/* decide copy_size */
		copy_size = ((size + able_size) > Count) ?
				(Count - size) :
				able_size;
		ret = copy_to_user(
				Buffer + size,
				g_tscnt.pktbuf + g_tscnt.prd,
				copy_size);
		if (ret) {
			pr_err("copy_to_user() failed.\n");
			return -EFAULT;
		}

		/* increment read position */
		g_tscnt.prd += copy_size;
		if (g_tscnt.prd == g_tscnt.ts_pktbuf_size) {
			g_tscnt.prd = 0;
		}

		/* increment total copied size */
		size += copy_size;

	} while (size != Count);

	g_tscnt.tsread_flag  = TUNER_TSREAD_IDLE;

	return size;
	
}

/**************************************************************************//**
 * write control of a driver
 *
 * @caution	The previous version of mm_tuner driver have "write"
 * 				system call to write registers continuously.
 * 				But, ioctl(TUNER_IOCTL_CNTSET) is implemented in this
 * 				version.
 *
 * @date		2011.10.31
 * @author		K.Okawa(KXD14)
 *
 * @retval		0			normal
 * @retval		-ENOSYS	(not implemented)
 *
 * @param [in] FIle, Buffer, Count, OffsetPosition
 *		These are regular argument of the system call "write"
 ******************************************************************************/
static ssize_t tuner_module_entry_write(struct file* FIle,
		const char __user * Buffer, size_t Count, loff_t* OffsetPosition)
{
	return -ENOSYS;
}

/**************************************************************************//**
 * ioctl system call
 *
 * @date		2011.08.02
 *
 * @author		K.Kitamura(*)
 * @author		K.Okawa(KXDA3)
 *
 * @retval 0			normal
 * @retval <0			error
 ******************************************************************************/
static long tuner_module_entry_ioctl(struct file *file,
		unsigned int uCommand, unsigned long uArgument)
{
	int ret = 0;
	int retval = 0;
	tuner_rwreg_t rw;
	tuner_event_t ev;
	uint8_t *buf;
	uint32_t ver;
	uint8_t data;
	tuner_cpathid_t cid;

	switch (uCommand) {
	/* get a parameter of the register of Tuner */
	case TUNER_IOCTL_VALGET:
		if (copy_from_user(&rw, (tuner_rwreg_t __user *)uArgument, sizeof(tuner_rwreg_t))) {
			pr_err("copy_from_user() failed.\n");
			return -EFAULT;
		}

		ret = tuner_drv_read_regs(&rw, 1);
		if (!ret) {
			if (copy_to_user((tuner_rwreg_t __user *)uArgument, &rw, sizeof(tuner_rwreg_t))) {
				pr_err("copy_to_user() failed.\n");
				return -EFAULT;
			}
		} else {
			pr_err("read a register, failed.\n");
			return ret;
		}

		return ret;
	/* write a parameters to the register of Tuner */
	case TUNER_IOCTL_VALSET:
		if (copy_from_user(&rw, (tuner_rwreg_t __user *)uArgument, sizeof(tuner_rwreg_t))) {
			pr_err("copy_from_user() failed.\n");
			return -EFAULT;
		}
		ret = tuner_drv_write_regs(&rw, 1);
		if (ret) {
			pr_err("write a register, failed.\n");
			return ret;
		}
		return 0;
	/* write registers continuously */
	case TUNER_IOCTL_CNTSET:
		if (copy_from_user(&rw, (tuner_rwreg_t __user *)uArgument, sizeof(tuner_rwreg_t))) {
			pr_err("copy_from_user() failed.\n");
			return -EFAULT;
		}
		buf = (uint8_t *)kmalloc(rw.cont.len, GFP_KERNEL);
		if (!buf) {
			pr_err("memory allocation failed.\n");
			return -ENOMEM;
		}
		if (copy_from_user(buf, (void __user *)rw.cont.buf, rw.cont.len)) {
			pr_err("copy_from_user() failed.\n");
			retval = -EFAULT;
		} else {

			ret = tuner_drv_hw_write_reg((int)rw.cont.bank + (int)cpath_id, rw.cont.adr, rw.cont.len, buf);
			if (ret) {
				pr_err("write continuously, failed.\n");
				retval = ret;
			}
		}
		kfree(buf);
		return retval;
	/* read registers continuously */
	case TUNER_IOCTL_CNTGET:
		if (copy_from_user(&rw, (tuner_rwreg_t __user *)uArgument, sizeof(tuner_rwreg_t))) {
			pr_err("copy_from_user() failed.\n");
			return -EFAULT;
		}
		buf = (uint8_t *)kmalloc(rw.cont.len, GFP_KERNEL);
		if (!buf) {
			pr_err("memory allocation failed.\n");
			return -ENOMEM;
		}
		ret = tuner_drv_hw_read_reg((int )rw.cont.bank + (int)cpath_id, rw.cont.adr, rw.cont.len, buf);
		if (ret) {
			pr_err("read continuously, failed.\n");
			retval = ret;
		} else {
			if (copy_to_user((void __user *)rw.cont.buf, buf, rw.cont.len)) {
				pr_err("copy_to_user() failed.\n");
				retval = -EFAULT;
			}
		}
		kfree(buf);
		return retval;
	/* get the interrupt factor and status */
	case TUNER_IOCTL_EVENT_GET:
		if(copy_to_user((tuner_event_t __user *)uArgument, &g_cnt.ev, sizeof(tuner_event_t))) {
			pr_err("copy_to_user() failed.\n");
			return -EFAULT;
		}

		/* initialize the variables for the interrupt information */
		g_cnt.ev.pack = 0;
		return 0;
	/* be available some interrupts */
	case TUNER_IOCTL_EVENT_SET:
		if (copy_from_user(&ev, (tuner_event_t __user *)uArgument, sizeof(tuner_event_t))) {
			pr_err("copy_from_user() failed.\n");
			return -EFAULT;
		}
		ret = tuner_drv_hw_setev(&ev);
		if (ret) {
			pr_err("tuner_drv_setev() failed.\n");
			return ret;
		}
		return 0;
	/* be disable some interrupts */
	case TUNER_IOCTL_EVENT_REL:
		if (copy_from_user(&ev, (tuner_event_t __user *)uArgument,	sizeof(tuner_event_t))) {
			pr_err("copy_from_user() failed.\n");
			return -EFAULT;
		}
		ret = tuner_drv_hw_relev(&ev);
		if (ret) {
			pr_err("tuner_drv_relev() failed.\n");
			return ret;
		}
		return 0;
	/* start to receive TS data from Tuner */
	case TUNER_IOCTL_TSIF_START: 
		if (!g_tscnt.tsifth_wait) {
			pr_warn("TS data buffering had already been started.\n");
			return -EBUSY;
		}
		if (g_tscnt.tsif != NULL) {
			kfree(g_tscnt.tsif);
			g_tscnt.tsif = NULL;
		}
		g_tscnt.tsif = (tuner_tsif_t *)kmalloc(sizeof(tuner_tsif_t), GFP_KERNEL);
		if (g_tscnt.tsif == NULL) {
			pr_err("memory allocation failed.\n");
			return -ENOMEM;
		}
		if (copy_from_user(g_tscnt.tsif, (tuner_tsif_t __user *)uArgument, sizeof(tuner_tsif_t))) {
			pr_err("copy_from_user() failed.\n");
			retval = -EFAULT;
		} else {
			ret = tuner_drv_tsif_start();
			if (ret) {
				pr_err("tuner_drv_tsif_start() failed.\n");
				retval = ret;
			} else {
				return 0;
			}
		}
		kfree(g_tscnt.tsif);
		g_tscnt.tsif = NULL;
		return retval;
	/* stop to receive TS data from Tuner */
	case TUNER_IOCTL_TSIF_STOP:
		ret = tuner_drv_tsif_stop() ;
		if(ret){
			pr_err("tuner_drv_tsif_stop() failed.\n");		
		}

		return ret ;
	/* return byte num of a TS packet */
	case TUNER_IOCTL_TSIF_PKTSIZE:
		ret = tuner_drv_tsif_pktsize();
		if (ret < 0) {
			pr_err("tuner_drv_tsif_pktsize() failed.\n");
			return ret;
		}
		if (copy_to_user((unsigned int __user *)uArgument, &ret, sizeof(unsigned int))) {
			pr_err("copy_to_user() failed.\n");
			return -EFAULT;
		}
		return 0;
	case TUNER_IOCTL_GETVER:
		ver =	(mmtuner_version.device << 24) |
				(mmtuner_version.major << 16) |
				(mmtuner_version.minor << 8) |
				mmtuner_version.hotfix;
//		put_user(ver, (unsigned int __user *)uArgument);
		if (copy_to_user((unsigned int __user *)uArgument, &ver, sizeof(unsigned int))) {
			pr_err("copy_to_user() failed.\n");
			return -EFAULT;
		}
		
		ret = tuner_drv_hw_read_reg(Sub, 0xFF, 1, &data);
		if (ret) {
			pr_err("Read CHIPID, failed.\n");
			return -EFAULT;
		}
		if (data != MMTUNER_DEVICE) {
			pr_err("Unexpected CHIPRD (0x%02x).\n", data);
			return -ENXIO;
		}
		return 0;
	case TUNER_IOCTL_TSIF_BLKSIZE:
		ret = tuner_drv_tsif_blksize();
		if (ret < 0) {
			pr_err("tuner_drv_tsif_blksize() failed.\n");
			return ret;
		}
		if (copy_to_user((int __user *)uArgument, &ret, sizeof(int))) {
			pr_err("copy_to_user() failed.\n");
			return -EFAULT;
		}
		return 0;
	case TUNER_IOCTL_CPATHID:
		if (copy_from_user(&cid, (tuner_cpathid_t __user *)uArgument, sizeof(tuner_cpathid_t))) {
			pr_err("copy_from_user() failed.\n");
			return -EFAULT;
		}

       	ret = tuner_drv_hw_set_id(cid);
		if (ret) {
		  	pr_err("set id, failed.\n");
			retval = ret;
		}
		return retval;
	/* return byte num of a TS packet */
	case TUNER_IOCTL_ENABLEPOWER:
		tuner_drv_power_control_startup() ;
		return 0;
	case TUNER_IOCTL_DISABLEPOWER:
		tuner_drv_power_control_endup() ;
		return 0;
	default:
		pr_err("illegal ioctl request.\n");
		return -EINVAL;
	}
}

/**************************************************************************//**
 * poll control of a driver
 *
 * @date		2011.08.23
 * @author		M.Takahashi(*)
 *
 * @retval		0	normal
 * @retval		<0	error
 ******************************************************************************/
static unsigned int tuner_module_entry_poll(struct file *file,
		struct poll_table_struct *poll_tbl)
{
	unsigned long tuner_flags = 0;
	unsigned int tuner_mask = 0;

	/* initialize */
	tuner_mask = 0;

	/* wait */
	poll_wait(file, &g_cnt.poll_waitq, poll_tbl);

	/* disable the interrupt */
	//lint -save -e550 -e666
	spin_lock_irqsave(&g_cnt.poll_lock, tuner_flags);
	//lint -restore
	
	/* release */
	if (g_cnt.poll_flag == 0x01) {
		tuner_mask = (POLLIN | POLLRDNORM);
	}

	g_cnt.poll_flag = 0x00;

	/* enable the interrupt */
	spin_unlock_irqrestore(&g_cnt.poll_lock, tuner_flags);

	return tuner_mask;
}

/**************************************************************************//**
 * @brief Read some registers.
 *
 * Repeat single read transaction.
 *
 * @date		2014.07.28
 * @date		2015.06.10
 *
 * @author		K.Okawa(IoT1)
 *
 * @param [in] arg	Address to a argument of ioctl() in unsigned long.
 * @num [in] num		Repeat count.
 *
 * @returns	0 on success.
 * @returns	Negative on error.
 ******************************************************************************/
static int tuner_drv_read_regs(tuner_rwreg_t *rw, int num)
{
	int i = 0;
	int ret = 0;

	for (i = 0; i < num; i++) {
		ret = tuner_drv_hw_read_reg(
				rw[i].sngl.bank + (int)cpath_id,
				rw[i].sngl.adr,
				1,
				&(rw[i].sngl.param));
		if (ret) {
			pr_err("copy_to_user() failed.\n");
			return ret;
		}
	}
	return 0;
}

/**************************************************************************//**
 * write some registers of the tuner device.
 *
 * (no description)
 *
 * @date		2014.07.28
 * @author		K.Okawa(KXDA3)
 *
 * @retval 0			normal
 * @retval -EINVAL	error
 ******************************************************************************/
static int tuner_drv_write_regs(tuner_rwreg_t *rw, int num)
{
	int i = 0;
	int ret = 0;

	for (i = 0; i < num; i++) {
		ret = tuner_drv_hw_rmw_reg(
				rw->sngl.bank + (int)cpath_id,
				rw->sngl.adr,
				rw->sngl.enabit,
				rw->sngl.param);
		if (ret) {
			return ret;
		}
	}

	return 0;
}
/**************************************************************************//**
 * TS I/F (buffering) thread function
 *
 * @caption
 * 	Original is "tuner_tsbufferring_thread" implemented by
 * 	T.Abe(FSI).
 *
 * @date	2014.08.21
 *
 * @author K.Okawa (KXDA3)
 *
 * @retval 0			Normal end
 * @retval <0			error (refer the errno)
 *
 * @param [in] arg	pointer to the TUNER_DATA_TSIF structure
 ******************************************************************************/
static int tuner_drv_tsif_th(void *arg)
{
	int ret = 0;
	int retval = 0;
	struct sched_param param;
	mm_segment_t oldfs;
	int dataready = 0;
	int fifo_err = 0;
	int maxwaitcnt=8;
	int waitcnt = 0;

	/* set the priority of thread */
	param.sched_priority = TUNER_CONFIG_TSBTH_PRI;
	//lint -save -e501
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	//lint -restore
	ret = sched_setscheduler(g_tscnt.tsifth_id, SCHED_FIFO, &param);
	set_fs(oldfs);

	waitcnt = maxwaitcnt;

	/* thread main loop */
	while (1) {
		//lint -save -e578
		wait_event_interruptible(g_tscnt.tsifth_waitq, g_tscnt.tsifth_flag);
		//lint -restore
		/* TS buffering */
		if (g_tscnt.tsifth_flag & TUNER_TSIFTH_ACTIVE) {
			ret = tuner_drv_hw_tsif_get_dready();
			if (ret < 0) {
				pr_debug("get DATAREADY, failed.\n");
				retval = ret;
				goto out;
			} else {
				dataready = (uint8_t)ret;
			}
			if (dataready & 0x06) {	/* OVER/UNDER-Run */
				if (fifo_err++ >= TUNER_TSIFTH_FIFOERROR_MAX) {
					pr_err("FIFO is in serious status!!!\n");
					pr_warn("TS I/F thread go to waiting.\n");

					g_tscnt.tsifth_flag = TUNER_TSIFTH_NONE;
					g_tscnt.tsifth_wait = true;
					fifo_err = 0;
					continue;
				}
				pr_warn("OVER/UNER-Run(0x%02x) (%d).\n", dataready, fifo_err);
				dataready = 0x00;
				ret = tuner_drv_hw_tsif_sync_pkt();
				if (ret) {
					pr_err("tuner_drv_hw_tsif_sync_pkt() failed.\n");
					retval = ret;
					goto out;
				}
				waitcnt=maxwaitcnt;
			}
			//set usleep time(time is given by vendor)
			if (!(dataready & 0x01)) { /* not DATAREADY */
				switch (waitcnt) {
				case 0:
					usleep_range(500, 1000);
					break;
				case 1:
					usleep_range(1000, 1500);
					break;
				case 2:
					usleep_range(1500, 2000);
					break;
				case 3:
					usleep_range(2000, 2500);
					break;
				case 4:
					usleep_range(2500, 3000);
					break;
				case 5:
					usleep_range(3000, 3500);
					break;
				case 6:
					usleep_range(3500, 4000);
					break;
				case 7:
					usleep_range(4000, 4500);
					break;
				case 8:
					usleep_range(4500, 5000);
					break;
				default:
					usleep_range(2000, 3000);
					waitcnt = 7;
					break;
				}
				if (waitcnt > 0) waitcnt--;
				continue;
			}
			waitcnt=maxwaitcnt;

			/* TS buffering */
			fifo_err = 0;
			ret = tuner_drv_hw_tsif_get_pkts(&g_tscnt);
			if (ret < 0) {
				pr_err("Receive/Store TS data, failed\n");
				retval = ret;
				goto out;
			}

			/* TS packet buffer over-flow detection */
			if (g_tscnt.pwr <= g_tscnt.prd &&
					g_tscnt.prd < g_tscnt.pwr + g_tscnt.ts_rx_size) {
				if (g_tscnt.tsread_flag) {
					g_tscnt.ovf ++;
				}
				if ((g_tscnt.ovf % 100) == 1) {
					pr_info("packet buffer over flow (%d)", g_tscnt.ovf);
				}
			}

			/* wake up the TS read */
			if (g_tscnt.tsread_flag == TUNER_TSREAD_WAIT) {
				g_tscnt.tsread_flag = TUNER_TSREAD_IDLE;
				if (waitqueue_active(&g_tscnt.tsread_waitq)) {
					wake_up_interruptible(&g_tscnt.tsread_waitq);
				}
			}
		}

		/* check the stop request to the TS I/F thread */
		if (g_tscnt.tsifth_flag & TUNER_TSIFTH_END) {
			pr_debug("Caught the stop request.\n");
			g_tscnt.tsifth_flag = TUNER_TSIFTH_NONE;
			break;
		}
	} /* while */

out:
	return retval;
}

/**************************************************************************//**
 * Set TS I/F context
 *
 * This function detect active OFDM circuit, and calculate suitable
 * memory size for handling TS data.
 * After execution, the following member variables of "_tsif_context"
 * structure are update.
 *		bw: active OFDM circuit
 *		ts_packet_size: byte num of an TS packet
 *		ts_record_size: RX transfer size a transaction.
 *		ts_pktbuf_size: buffer size stored by TS I/F thread
 *
 * @date	2014.08.22
 *
 * @author K.Okawa (KXDA3)
 *
 * @retval 0			Normal end
 * @retval <0			error
 *
 * @param [out] ptscnt	pointer to TS I/F context structure
 ******************************************************************************/
static int tuner_drv_tsif_set_cntxt(struct _tsif_cntxt *tc)
{
	int ret = 0;
	uint8_t pbuf_max_size;
	uint16_t pktbuf_size;
	uint32_t lowerlimit;
	uint32_t maxbank=48;
	uint32_t rxsize;

#if TUNER_TSPKTBUF_MODE == 2
        const uint16_t bufsize_tbl[8] = {1915,1851,1787,1659,1403, 891, 635, 379};
#else
	const uint16_t bufsize_tbl[8] = { 695, 631, 567, 439, 183, 695, 695, 695};
#endif

	if (tc == NULL || tc->tsif == NULL) {
		pr_err("illegal argument.\n");
		return -EINVAL;
	}
	if (!tc->tsifth_wait) {
		pr_warn("TS buffering have been already started.\n");
		return -EINPROGRESS;
	}

	/* detect the reception segment system */
	ret = tuner_drv_bwseg(&tc->bw);
	if (ret) {
		pr_err("tuner_drv_bwseg(), failed.\n");
		return ret;
	}

	/* TS record size a RX transaction */
	/* Readable packet number, when DATAREADY is high. */

	//Check packet buffer size configuration.
	ret = tuner_drv_hw_read_reg(2, 0x62, 1, &pbuf_max_size);
	if (ret) {
		pr_err("Main2 register read failed.\n");
		return ret;
	}

	pktbuf_size = bufsize_tbl[(pbuf_max_size>>4)&0x7];

	// Calculate hardware lower limit value
	switch((tc->tsif->thl[tc->bw]&0x7)) {
	case 0 :
	case 6 :
	case 7 :
		lowerlimit = 16;
		break;
	default :
		lowerlimit = (pktbuf_size)>>(6-(tc->tsif->thl[tc->bw]&0x7));
		break;
	}

	rxsize = lowerlimit;
	// re-calculate lowerlimit when use fixed rxsize
	if ( tc->tsif->thl[tc->bw] >= 0x10 ) {
		rxsize = 4<<(tc->tsif->thl[tc->bw]>>4);
		if ( rxsize > lowerlimit ) rxsize = lowerlimit;
	}

	tc->ts_rxpkt_num = rxsize&0x07fc;

	tc->ts_rx_size = tc->ts_rxpkt_num * g_ts_pkt_size[tc->tsif->ts_pkt_type];

	/* TS buffer size for the TS I/F thread */
	maxbank = TUNER_MAX_TSPKTBUF_SIZE/tc->ts_rx_size;
	if ( maxbank > TUNER_MAX_TSPKTBUF_BANK ) maxbank = TUNER_MAX_TSPKTBUF_BANK;

	tc->ts_pktbuf_size = (tc->ts_rx_size) * maxbank;

	if ( (tc->bw == 0 && tc->ts_rx_size < (64*g_ts_pkt_size[tc->tsif->ts_pkt_type]) ) ||
	     (tc->bw == 2 && tc->ts_rx_size < (32*g_ts_pkt_size[tc->tsif->ts_pkt_type]) ) ) {
	        pr_warn("Looks waterline of interrput is too low. Please confirm it.\n");
	}

	return 0;
}

/**************************************************************************//**
 * Start to receive TS data.
 *
 * This function activate to receive TS data.
 * The "tsifth" which is the kernel thread to receive and store
 * the TS data from the tuner device had been dispatched and wait.
 *
 * @date		2013.12.06
 *
 * @author		S.Sasaki(FSI)
 * @author		K.Okawa(KXDA3)
 *
 * @retval 0			normal
 * @retval <0			error
 ******************************************************************************/
static int tuner_drv_tsif_start(void)
{
	int ret = 0;
	int buffer_size = 0;

	if (!g_tscnt.tsifth_wait) {
		/* TS I/F active! */
		pr_warn("TS buffering had already been started.\n");
		return -EINPROGRESS;
	}

	ret = tuner_drv_tsif_set_cntxt(&g_tscnt);
	if (ret) {
		pr_err("tuner_drv_hw_tsif_set_cntxt() failed.\n");
		return ret;
	}

	if (g_tscnt.pktbuf) {
		kfree(g_tscnt.pktbuf);
		g_tscnt.pktbuf = NULL;
	}

	buffer_size = g_tscnt.ts_pktbuf_size;

	g_tscnt.pktbuf = (uint8_t *)kmalloc(buffer_size, GFP_KERNEL );

	if (g_tscnt.pktbuf == NULL) {
		pr_err("memory allocation failed.\n");
		return -ENOMEM;
	}

	memset(g_tscnt.pktbuf, 0, g_tscnt.ts_pktbuf_size);
	g_tscnt.pwr = g_tscnt.prd = 0;
	g_tscnt.ovf = 0;
	
	ret = tuner_drv_hw_tsif_config(&g_tscnt);
	if (ret) {
		pr_err("tuner_drv_hw_tsif_config() failed.\n");
		kfree(g_tscnt.pktbuf);
		return ret;
	}

	/* TS read operation is IDLE state. */
	g_tscnt.tsread_flag = TUNER_TSREAD_IDLE;

	ret = tuner_drv_hw_tsif_sync_pkt();
	if (ret) {
		pr_err("tuner_drv_hw_tsif_sync_pkt failed.\n");
		kfree(g_tscnt.pktbuf);
		return ret;
	}

	/* re-activate TS I/F */
	g_tscnt.tsifth_flag = TUNER_TSIFTH_ACTIVE;
	if (waitqueue_active(&g_tscnt.tsifth_waitq)) {
		wake_up_interruptible(&g_tscnt.tsifth_waitq);
	}
	g_tscnt.tsifth_wait = false;

	return ret;
}

/**************************************************************************//**
 * Stop receiving TS data.
 *
 * This function make the TS I/F thread wait status.
 *
 * @date		2014.08.21
 *
 * @author		S.Sasaki(FSI)
 * @author		K.Okawa(KXDA3)
 *
 * @retval 0			normal
 * @retval <0			error
 ******************************************************************************/
static int tuner_drv_tsif_stop(void)
{
	int ret = 0;
	uint32_t i = 0;

	if (g_tscnt.tsifth_wait) {
		/* TS I/F is NOT active! */
		pr_warn("TS buffering is not active.\n");
		return 0;
	}

	/* stop (be waiting status) TS I/F thread */
	g_tscnt.tsifth_flag = TUNER_TSIFTH_NONE;
	g_tscnt.tsifth_wait = true;

	/* confirm the status of the TS I/F thread */
	while (1) {
		/*
		 * NOTE
		 * It takes about 20-30[ms] to become able to detect the TS
		 * I/F thread changed in waiting state in the return value of
		 * the waitqueue_activate() function.
		 */
		if (waitqueue_active(&g_tscnt.tsifth_waitq)) {
			pr_warn("TS I/F thread is going to do the stop procedure.\n");
			break;
		}

		usleep_range(TUNER_TSIFTH_SLEEP_MIN, TUNER_TSIFTH_SLEEP_MAX);

		i++;
		if (TUNER_TSIFTH_SLEEP_RETRY <= i) {
			pr_crit("cannot stop TS I/F thread.\n");
			break;
		}
	}

	/* release the waiting in TS read operation */
	if (g_tscnt.tsread_flag == TUNER_TSREAD_WAIT) {
		g_tscnt.tsread_flag = TUNER_TSREAD_END;
		if (waitqueue_active(&g_tscnt.tsread_waitq)) {
			wake_up_interruptible( &g_tscnt.tsread_waitq);
		}
	}

	g_tscnt.pwr = g_tscnt.prd = 0;
	g_tscnt.ovf = 0;
	g_tscnt.ts_rxpkt_num = 0;
	g_tscnt.ts_rx_size = 0;
	g_tscnt.ts_pktbuf_size = 0;

	kfree(g_tscnt.pktbuf);
	kfree(g_tscnt.tsif);

	g_tscnt.pktbuf = NULL;
	g_tscnt.tsif = NULL;

	return ret;
}

/**************************************************************************//**
 * return TS packet size
 *
 * @date		2014.09.02
 *
 * @author		K.Okawa(KXDA3)
 *
 * @retval		>0	packet size
 * @retval		<0	error
 ******************************************************************************/
static int tuner_drv_tsif_pktsize(void)
{

	if (g_tscnt.tsifth_wait) {
		pr_warn("TS buffering is not started.\n");
		return -EAGAIN;
	}

	return (int)(g_ts_pkt_size[g_tscnt.tsif->ts_pkt_type]);
}

/**************************************************************************//**
 * @brief Call-back function for TS-Read Timeout.
 *
 * @date		2015.11.05
 *
 * @author		K.Okawa(IoT Sol.)
 ******************************************************************************/
static void tsread_timer_handler(unsigned long data)
{

	if (g_tscnt.tsread_flag != TUNER_TSREAD_WAIT) {
		pr_info("Timer is not to be restarted.");
		return;
	}

	g_tscnt.tsread_flag = TUNER_TSREAD_TIMEOUT;
	if (waitqueue_active(&g_tscnt.tsread_waitq)) {
		wake_up_interruptible(&g_tscnt.tsread_waitq);
	}
}

/**************************************************************************//**
 * return TS block count
 *
 * @date		2015.07.14
 *
 * @author		M.Sumida
 *
 * @retval		>0	block size
 * @retval		<0	error
 ******************************************************************************/
static int tuner_drv_tsif_blksize(void)
{

	if (g_tscnt.tsifth_wait) {
		pr_warn("TS buffering is not started.\n");
		return -EAGAIN;
	}

	return (int)(g_tscnt.ts_rxpkt_num);
}


/**************************************************************************//**
 * detect active OFDM block (BW13 or BW01)
 *
 * @date		2014.08.22
 *
 * @author		K.Okawa(KXDA3)
 *
 * @retval 0			normal
 * @retval <0			error
 ******************************************************************************/
//static int __inline tuner_drv_bwseg(enum _bw_seg *pbw)
static int  tuner_drv_bwseg(enum _bw_seg *pbw)
{
	int ret;
	uint8_t rd;
	uint8_t sys;

	ret = tuner_drv_hw_read_reg(Main1, 0x02, 1, &rd);	/* SYSSET */
	if (ret) {
		pr_debug("register SYSSET, read fail.\n");
		return ret;
	}
	sys = (rd & 0xC0) >> 6;	/* SYS is SYSSET[7:6]. */
	switch (sys) {
	case 0:
		*pbw = TUNER_DRV_BW13;
		break;
	case 1:
		*pbw = TUNER_DRV_BW1;
		break;
	case 3:
		*pbw = TUNER_DRV_BW3;
		break;
	default:
		pr_err("illegal SYS parameter.\n");
		return -EFAULT;
	}

	return 0;
}

/**************************************************************************//**
 * initialization control of a driver
 *
 * @date		2011.08.02
 * @author		K.Kitamura(*)
 *
 * @retval		0	normal
 * @retval		<0	error
 ******************************************************************************/
int tuner_drv_start(void)
{
	int retval = 0;
	int ret = 0;
	struct device *dev = NULL;

	/* register "mmtuner" driver */
	ret = platform_driver_register(&mmtuner_driver);
	if (ret != 0) {
		pr_err("platform_driver_register failed.\n");
		return ret;
	}

	/* memory allocation */
	mmtuner_device = platform_device_alloc(TUNER_CONFIG_DRIVER_NAME, -1);

	if (!mmtuner_device) {
		pr_err("platform_device_alloc() failed.\n");
		platform_driver_unregister(&mmtuner_driver);
		return -ENOMEM;
	}

	/* add device */
	ret = platform_device_add(mmtuner_device);
	if (ret) {
		pr_err("platform_device_add() failed.\n");
		retval = ret;
		goto out;
	}

	/* create the node of device */
	device_class = class_create(THIS_MODULE, TUNER_CONFIG_DRIVER_NAME);
	if (IS_ERR(device_class)) {
		pr_err("class_create() failed.\n");
		retval = PTR_ERR(device_class);
		goto out;
	}

	/* create the logical device */
	dev = device_create(device_class, NULL,
			MKDEV(TUNER_CONFIG_DRV_MAJOR, TUNER_CONFIG_DRV_MINOR), NULL,
			TUNER_CONFIG_DRIVER_NAME);
	if (IS_ERR(dev)) {
		pr_err("device_create() failed.\n");
		retval = PTR_ERR(dev);
		goto out;
	}

	/* register the TS I/F driver */
	ret = tuner_drv_hw_tsif_register();
	if (ret) {
		pr_err("tuner_drv_hw_tsif_register() failed.\n");
		retval = ret;
		goto out;
	}

	init_waitqueue_head(&g_cnt.poll_waitq);
	spin_lock_init(&g_cnt.poll_lock);
	g_cnt.poll_flag = 0x00;

	g_cnt.ev.pack = 0;
	g_cnt.opcnt = 0;

	/* dispatch a kernel thread to receive TS data */
	g_tscnt.tsifth_wait = true;
	g_tscnt.tsifth_flag = TUNER_TSIFTH_NONE;
	init_waitqueue_head(&g_tscnt.tsifth_waitq);
	g_tscnt.tsifth_id = kthread_create(tuner_drv_tsif_th, NULL, "tuner_drv_tsif_th");
	if (IS_ERR(g_tscnt.tsifth_id)) {
		pr_err("kthread_create() failed.\n");
		retval = PTR_ERR(g_tscnt.tsifth_id);
		g_tscnt.tsifth_id = NULL;
		goto out;
	}
	g_tscnt.pktbuf = NULL;
	g_tscnt.pwr = g_tscnt.prd = 0;
	g_tscnt.ovf = 0;

	wake_up_process(g_tscnt.tsifth_id);

	/* initialize the status flag and the wait-queue for Ts Buffering Thread */
	g_tscnt.tsread_flag = TUNER_TSREAD_IDLE;
	init_waitqueue_head(&g_tscnt.tsread_waitq);

	return 0;	/* normal */

out:
	platform_device_put(mmtuner_device);
	platform_driver_unregister(&mmtuner_driver);
	
	return retval;

}

void tuner_drv_end(void)
{
	/* exit the kernel thread (for TS) */
	g_tscnt.tsifth_flag = TUNER_TSIFTH_END;
	if (waitqueue_active(&g_tscnt.tsifth_waitq)) {
		wake_up_interruptible(&g_tscnt.tsifth_waitq);
	}
	g_tscnt.tsifth_wait = true;

	/* unregister the TS kernel thread */
	if (g_tscnt.tsifth_id) {
		kthread_stop(g_tscnt.tsifth_id);
	}

	/* execute the unregister scheme of TS I/F */
	tuner_drv_hw_tsif_unregister();

	/* Destroy device */
	device_destroy(device_class,
			MKDEV(TUNER_CONFIG_DRV_MAJOR, TUNER_CONFIG_DRV_MINOR));
	/* delete a entry of class */
	class_destroy(device_class);
	/* unregister the driver */
	platform_device_unregister(mmtuner_device);
	/* unregister the platform entry */
	platform_driver_unregister(&mmtuner_driver);

}

static int __init tuner_drv_init(void)
{
	int ret = 0 ;
	
	ret = tuner_drv_hw_i2c_register();
	if(ret) {
		pr_err("tuner drv init failed.\n");
	}

	return ret ;
}


static void __exit tuner_drv_uninit(void)
{
	tuner_drv_hw_i2c_unregister() ;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Socionext Inc.");
MODULE_DESCRIPTION("MM Tuner Driver");
//lint -save -e528 -esym(528,*)
module_init(tuner_drv_init);
module_exit(tuner_drv_uninit);
//lint -restore

/*******************************************************************************
 * Copyright (c) 2015 Socionext Inc.
 ******************************************************************************/
