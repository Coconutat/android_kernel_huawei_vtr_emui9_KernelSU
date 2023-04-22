/*
 * Copyright 2015 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * A copy of the GPL is available at
 * http://www.broadcom.com/licenses/GPLv2.php, or by writing to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * 477x tty char driver to support MCU_REQ pin toggling
 *
 */
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/serial.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>

#ifdef CONFIG_HWCONNECTIVITY
#include <huawei_platform/connectivity/hw_connectivity.h>
#endif

extern int get_gps_ic_type(void);

#define DTS_COMP_GPS_POWER_NAME "huawei,gps_power"
#define BUFFER_SIZE 16
#define PORT_NAME "/dev/ttyAMA3"
#define TTY_SIZE_MAX      (18446744073709551615UL)
#define USE_TIMER 1
#define GPS_IC_TYPE_4775 4775
//#define BCM_TTY_DEBUG_INFO 0
//#define BCM_TTY_DEBUG 0
/*
From EMUI4.1 to  EMUI 5.0 , the GpioDelayMs is changed from 50ms to 250ms,
so bcm suggest change the delay time from 400ms to 600 ms.
*/
//#define BCM_TTY_DELTA_MS_TO_TOGGLE_LOW 400
// KOM -- #define BCM_TTY_DELTA_MS_TO_TOGGLE_LOW 600
#define BCM_TTY_DELTA_MS_TO_TOGGLE_LOW 100
// KOM -- #define BCM_TTY_TIMER_IDLE_INTERVAL_MS 200
#define BCM_TTY_TIMER_IDLE_INTERVAL_MS 25
//#define BCM_TTY_MCU_REQ_RESP 1
int g_BCM_TTY_DELTA_MS_TO_TOGGLE_LOW = 600;
int g_BCM_TTY_TIMER_IDLE_INTERVAL_MS = 200;

#define GPSINFO(fmt, args...)	pr_info("[DRV_GPS, I %s:%d]" fmt "\n",  __func__,  __LINE__, ## args)
#define GPSDBG(fmt, args...)	pr_debug("[DRV_GPS, D %s:%d]" fmt "\n",  __func__,  __LINE__, ## args)
#define GPSERR(fmt, args...)	pr_err("[DRV_GPS, E %s:%d]" fmt "\n",  __func__, __LINE__, ## args)

#ifdef CONFIG_SENSORS_SSP_BBD
extern void bbd_parse_asic_data(unsigned char *pucData, unsigned short usLen, void (*to_gpsd)(unsigned char *packet, unsigned short len, void* priv), void* priv);
#endif
char gps_tty[16];
//--------------------------------------------------------------
//
//               Structs
//
//--------------------------------------------------------------
struct bcm_tty_priv
{
	struct file *tty;
	struct miscdevice misc;
	int mcu_req;
	//int mcu_resp;
	//int host_req;
};


#ifdef USE_TIMER

enum ttyalarmtimer_type {
	TXALARM_REALTIME,
	TXALARM_MONOTONIC,
	TXALARM_KERNEL_TIMER,
	TXALARM_NUMTYPE,
};

static struct txalarm_base {
	spinlock_t		lock;
	int 			total_write_request; // total write request counter since total_bytes_to_write = 0
	unsigned long  	total_bytes_to_write;
	struct timer_list timer_idle;
	struct timer_list timer_tty;
	unsigned long   first_write_jiffies;
	unsigned long   last_write_jiffies;
	unsigned long   delta_since_last_write_jiffies; // delta time since last write inside timer_idle_callback
	unsigned long   timer_idle_expiration_ms;
	void (*timer_idle_function)( unsigned long data );
	void            *priv_data;
	int             toggled_low;

} txalarm_bases[3];

#endif

struct bcm_tty_priv *priv;
/*
 *  Misc. functions
 * bcm4773_hello - wakeup chip by toggling mcu_req while monitoring mcu_resp to check if awake
 *
 */
typedef struct platform_device gps_tty_platform_device;

static bool bcm477x_hello(struct bcm_tty_priv *priv, unsigned total_bytes_in_queue, unsigned total_write_request)
{
	if(priv == NULL)
	{
		GPSERR(" priv is NOT init ");
		return false;
	}
#ifdef BCM_TTY_DEBUG
	GPSINFO("[SSPBBD]gpstty driver bcm477x_hello is coming %d total_bytes_in_queue=%d total_write_request=%d", priv->mcu_req, total_bytes_in_queue, total_write_request);
#endif

	gpio_set_value(priv->mcu_req, 1);
	//printk("[SSPBBD]gpstty driver to set mcu_req is 1\n" );

	// With hardware flow control this is not needed but this can be keep for debug purposes to make sure
#ifdef BCM_TTY_MCU_REQ_RESP
	// mcu req resp pin is toggled properly
	while (!gpio_get_value(priv->mcu_resp))
	{
		if (count++ > 100) {
			gpio_set_value(priv->mcu_req, 0);
			GPSINFO("MCU_REQ_RESP timeout. MCU_RESP(gpio%d) not responding to MCU_REQ(gpio%d)",
					priv->mcu_resp, priv->mcu_req);
			return false;
		}

		mdelay(1);

		/*if awake, done */
		if (gpio_get_value(priv->mcu_resp)) break;

		if (count%20==0 && retries++ < 3) {
			gpio_set_value(priv->mcu_req, 0);
			mdelay(1);
			gpio_set_value(priv->mcu_req, 1);
			mdelay(1);
		}
	}
#endif
	return true;
}


/*
 * bcm4773_bye - set mcu_req low to let chip go to sleep
 *
 */
static void bcm477x_bye(struct bcm_tty_priv *priv)
{
	if(priv == NULL)
	{
		GPSERR(" priv is NOT init ");
		return;
	}
#ifdef BCM_TTY_DEBUG
	GPSINFO("[SSPBBD]gpstty driver bcm477x_bye is coming");
#endif

	gpio_set_value(priv->mcu_req, 0);
}



static void raz_timer(void)
{
#ifdef USE_TIMER

#ifdef BCM_TTY_DEBUG
	GPSINFO( "[SSPBBD]gpstty driver raz_timer");
#endif

	txalarm_bases[TXALARM_KERNEL_TIMER].total_write_request = 0;
	txalarm_bases[TXALARM_KERNEL_TIMER].total_bytes_to_write   = 0;
	txalarm_bases[TXALARM_KERNEL_TIMER].first_write_jiffies   = 0;
	txalarm_bases[TXALARM_KERNEL_TIMER].last_write_jiffies   = 0;
	txalarm_bases[TXALARM_KERNEL_TIMER].delta_since_last_write_jiffies   = 0;
	txalarm_bases[TXALARM_KERNEL_TIMER].timer_idle_expiration_ms = 0;
	txalarm_bases[TXALARM_KERNEL_TIMER].toggled_low = 1;
#endif
}

void timer_idle_callback( unsigned long data )
{
#ifdef USE_TIMER
	struct txalarm_base *base = NULL;
	enum  ttyalarmtimer_type type = TXALARM_KERNEL_TIMER;
	unsigned long flags;
#ifdef BCM_TTY_DEBUG
	unsigned long j = jiffies;
#endif
	unsigned long delta_ms = 0;
	struct bcm_tty_priv *priv = NULL;

	base = &txalarm_bases[type];

	spin_lock_irqsave(&base->lock, flags);

	priv = (struct bcm_tty_priv *)base->priv_data;
	base->delta_since_last_write_jiffies = jiffies - base->last_write_jiffies;
	delta_ms = base->delta_since_last_write_jiffies * 1000 / HZ;   /* jiffies to milliseconds */

	// if delta_since_last_write_jiffies >= BCM_TTY_DELTA_MS_TO_TOGGLE_LOW then we toggle MCU_REQ low
	if (delta_ms >= g_BCM_TTY_DELTA_MS_TO_TOGGLE_LOW && base->toggled_low==0)
	{
		bcm477x_bye(priv);
		base->toggled_low = 1;
		raz_timer();
		mod_timer(&base->timer_idle, jiffies + msecs_to_jiffies(g_BCM_TTY_TIMER_IDLE_INTERVAL_MS * 1));
        // KOM -- __DEBUG__
        //GPSINFO( "[SSPBBD]gpstty driver timer_idle_callback %d jiffies delta %d jiffies => %d ms", (unsigned long)j, base->delta_since_last_write_jiffies, delta_ms);
	}
	else
    {
		mod_timer(&base->timer_idle, jiffies + msecs_to_jiffies(g_BCM_TTY_TIMER_IDLE_INTERVAL_MS * 1));
	}

	spin_unlock_irqrestore(&base->lock, flags);

#ifdef USE_TIMER
#ifdef BCM_TTY_DEBUG
	GPSINFO( "[SSPBBD]gpstty driver timer_idle_callback %d jiffies delta %d jiffies => %d ms", (unsigned long)j, base->delta_since_last_write_jiffies, delta_ms);
	GPSINFO( "[SSPBBD]gpstty driver timer_idle_callback exit");
#endif
#endif

#endif
}

// idle timer arm as a 100 ms timer to monitor interval since last write to check for end of transmission with an offset >=100 ms
static void alarm_idle_init(enum ttyalarmtimer_type type,
		void (*function)(unsigned long ),unsigned long timer_expiration_ms)

{
#ifdef USE_TIMER
	struct txalarm_base *base = &txalarm_bases[type];
	unsigned long flags;

#ifdef BCM_TTY_DEBUG
	GPSINFO( "[SSPBBD] alarm_idle_init enter");
#endif

	spin_lock_irqsave(&base->lock, flags);

	base->timer_idle_function = function;
	//base->type  = type;
	base->timer_idle_expiration_ms = timer_expiration_ms;

	// my_timer, my_timer.function, my_timer.data
	setup_timer( &base->timer_idle, base->timer_idle_function, (long)base );

	spin_unlock_irqrestore(&base->lock, flags);

#ifdef BCM_TTY_DEBUG
	GPSINFO( "[SSPBBD]gpstty driver alarm_idle_init exit");
#endif

#endif
}

static void alarm_idle_arm(void)
{
#ifdef USE_TIMER
	unsigned long j = jiffies;
	enum  ttyalarmtimer_type type = TXALARM_KERNEL_TIMER;
	struct txalarm_base *base = &txalarm_bases[type];
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&base->lock, flags);

	ret = mod_timer( &base->timer_idle, j + msecs_to_jiffies(base->timer_idle_expiration_ms));

	spin_unlock_irqrestore(&base->lock, flags);
#endif
}

static void config_timer(struct bcm_tty_priv *priv)
{
#ifdef USE_TIMER
	if(priv == NULL)
	{
		GPSERR(" priv is NOT init ");
		return;
	}
#ifdef BCM_TTY_DEBUG
	GPSINFO( "[SSPBBD]gpstty driver config_timer");
#endif

	raz_timer();

	txalarm_bases[TXALARM_KERNEL_TIMER].priv_data = (void*)priv;
	spin_lock_init(&txalarm_bases[TXALARM_KERNEL_TIMER].lock);
#endif
}

/**
 * timer_idle_try_to_cancel - Tries to cancel timer_idle
 */
static int timer_idle_try_to_cancel(void)
{
	int ret = 0;
#ifdef USE_TIMER
	enum  ttyalarmtimer_type type = TXALARM_KERNEL_TIMER;
	struct txalarm_base *base = &txalarm_bases[type];

#ifdef BCM_TTY_DEBUG
	GPSINFO( "[SSPBBD]gpstty driver txalarm_try_to_cancel");
#endif

        /*to void timer null point*/
	ret = del_timer_sync( &base->timer_idle );

#ifdef BCM_TTY_DEBUG
	if (ret)
		GPSINFO("[SSPBBD]gpstty driver timer_idle still in use...");
	else
		GPSINFO("[SSPBBD]gpstty driver timer_idle deleted");
#endif

#endif
	return ret;
}


static int timer_idle_cancel(void)
{
#ifdef USE_TIMER
#ifdef BCM_TTY_DEBUG
	GPSINFO( "[SSPBBD]gpstty driver timer_idle_cancel");
#endif

	for (;;)
	{
		int ret = timer_idle_try_to_cancel();
		if (ret >= 0)
		{
			return ret;
		}
		cpu_relax();
	}
#endif
}

//--------------------------------------------------------------
//
//               Misc. functions
//
//--------------------------------------------------------------
static int bcm_tty_config(struct file *f)
{
	struct termios termios;
	mm_segment_t fs;
	long ret;
	if(f == NULL)
	{
		GPSERR(" f is NOT init ");
		return -1;
	}
	/* Change address limit */
	fs = get_fs();
	set_fs(KERNEL_DS);

	/* Get termios */
	ret = f->f_op->unlocked_ioctl(f, TCGETS, (unsigned long)&termios);
	if (ret) {
		GPSERR(" TCGETS failed, err=%ld", ret);
		set_fs(fs);
		return -1;
	}

	termios.c_iflag = 0;
	termios.c_oflag = 0;
	termios.c_lflag = 0;
	termios.c_cflag = CRTSCTS | CLOCAL | CS8 | CREAD;
	termios.c_cc[VMIN ] = 0;
	termios.c_cc[VTIME] = 1;	/* 100ms timeout */
	termios.c_cflag |= B921600;	//TODO: How to change baud

	/* Set termios */
	ret = f->f_op->unlocked_ioctl(f, TCSETS, (unsigned long)&termios);
	if (ret) {
		GPSERR(" TCSETS failed, err=%ld", ret);
		set_fs(fs);
		return -1;
	}

	/* Restore address limit */
	set_fs(fs);

	return 0;
}


static int bcm_tty_config_close(struct file *f)
{
	struct termios termios;
	mm_segment_t fs;
	long ret;
	if(f == NULL)
	{
		GPSERR(" f is NOT init ");
		return -1;
	}
	/* Change address limit */
	fs = get_fs();
	set_fs(KERNEL_DS);

	/* Get termios */
	ret = f->f_op->unlocked_ioctl(f, TCGETS, (unsigned long)&termios);
	if (ret) {
		GPSERR(" TCGETS failed, err=%ld", ret);
		set_fs(fs);
		return -1;
	}

	termios.c_iflag = 0;
	termios.c_oflag = 0;
	termios.c_lflag = 0;
	termios.c_cflag &= ~CRTSCTS;
	termios.c_cc[VMIN ] = 0;
	termios.c_cc[VTIME] = 1;
	termios.c_cflag |= B921600;
	/* Set termios */
	ret = f->f_op->unlocked_ioctl(f, TCSETS, (unsigned long)&termios);
	if (ret) {
		GPSERR(" TCSETS failed, err=%ld", ret);
		set_fs(fs);
		return -1;
	}

	/* Restore address limit */
	set_fs(fs);
	return 0;
}

static int get_GPS_TTY_Port(void)
{
	struct device_node *np = NULL;
	int ret = 0;
	char *gps_tty_port_str = NULL;
    
	memset(gps_tty,0,sizeof(gps_tty));
	np = of_find_compatible_node(NULL, NULL, DTS_COMP_GPS_POWER_NAME);
	if (!np) 
	{
		GPSERR("%s, can't find node %s\n", __func__,DTS_COMP_GPS_POWER_NAME);
		return -1;
	}
	ret = of_property_read_string(np, "broadcom_config,tty_port",(const char **)&gps_tty_port_str);
	if (ret) 
	{
		GPSINFO("get broadcom_config,tty_port fail ret=%d\n",ret);
		snprintf(gps_tty, sizeof(gps_tty),"/dev/%s", "ttyAMA3");
		return 0;
	}
	snprintf(gps_tty, sizeof(gps_tty),"/dev/%s", gps_tty_port_str);
	GPSINFO("get gps_tty=%s\n",gps_tty);
	return 0;
	
}
//--------------------------------------------------------------
//
//               File Operations
//
//--------------------------------------------------------------
static int bcm_tty_open(struct inode *inode, struct file *filp)
{
    struct bcm_tty_priv *priv = NULL;
#ifdef USE_TIMER
	enum  ttyalarmtimer_type type = TXALARM_KERNEL_TIMER;
#endif
	if(inode == NULL || filp == NULL)
	{
		GPSERR(" pointer is NOT init ");
		return -1;
	}
	priv = container_of(filp->private_data, struct bcm_tty_priv, misc);
	/* Initially, file->private_data points device itself and we can get our priv structs from it. */
	GPSINFO("++");
    if(get_GPS_TTY_Port()<0)
	{
		GPSERR("get_GPS_TTY_Port failed !");
		return -1;
	}
	if(priv->tty)
	{
		GPSERR("-- has opened, open failed !");
		return -1;
	}

	/* Open tty */
	priv->tty = filp_open(gps_tty, O_RDWR, 0);
	if (IS_ERR(priv->tty)) {
		int ret = (int)PTR_ERR(priv->tty);
		GPSERR(" can not open %s, error=%d", gps_tty, ret);
		return ret;
	}

	/* Config tty */
	if (bcm_tty_config(priv->tty)) {
		GPSERR(" can not change %s setting.", gps_tty);
		return -EIO;
	}

	filp->private_data = priv;


#ifdef USE_TIMER
	raz_timer();
	alarm_idle_init(type, timer_idle_callback, g_BCM_TTY_TIMER_IDLE_INTERVAL_MS);
	alarm_idle_arm();
#endif

#ifdef BCM_TTY_DEBUG_INFO
	GPSINFO("be called --");
#endif

	return 0;
}

static int bcm_tty_release(struct inode *inode, struct file *filp)
{
    struct bcm_tty_priv *priv = NULL;
    struct file *tty = NULL;
    int ret = 0;
	if(inode == NULL || filp == NULL)
	{
		GPSERR(" pointer is NOT init ");
		return -1;
	}
	priv = (struct bcm_tty_priv*) filp->private_data;
    if (NULL == priv)
    {
        return -1;
    }
	tty = priv->tty;

	GPSINFO("++");
	priv->tty = NULL;
	ret = filp_close(tty, 0);
#ifdef USE_TIMER
	timer_idle_cancel();
#endif
	GPSINFO("--");

	return ret;
}

static ssize_t bcm_tty_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    struct bcm_tty_priv *priv  = NULL;
    struct file *tty = NULL;
    ssize_t len;

	if(buf == NULL || filp == NULL || ppos ==NULL)
	{
		GPSERR(" pointer is NOT init ");
		return 0;
	}
	priv = (struct bcm_tty_priv*) filp->private_data;
    if (NULL == priv)
    {
        return 0;
    }
    tty = priv->tty;
    if (NULL == tty)
    {
        return 0;
    }

	len = tty->f_op->read(tty, buf, size, ppos);

	return len;
}

static ssize_t bcm_tty_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    unsigned long j = jiffies;
    struct bcm_tty_priv *priv = NULL;
    struct file *tty = NULL;
    ssize_t ret = 0;
#ifdef USE_TIMER
	unsigned long flags;
	struct txalarm_base *base = NULL;
	enum  ttyalarmtimer_type type = TXALARM_KERNEL_TIMER;
#endif
	if(buf == NULL || filp == NULL || ppos ==NULL)
	{
		GPSERR(" pointer is NOT init ");
		return 0;
	}
	priv = (struct bcm_tty_priv*) filp->private_data;
	tty = priv->tty;
	

#ifdef USE_TIMER
	base = &txalarm_bases[type];
	// get priv from txalarm_bases
	priv = (struct bcm_tty_priv *)base->priv_data;

	spin_lock_irqsave(&base->lock, flags);
	if(size > TTY_SIZE_MAX){
		size = TTY_SIZE_MAX;
	}
	base->total_bytes_to_write+=size;
	base->total_write_request++;

	bcm477x_hello(priv, base->total_bytes_to_write, base->total_write_request);
	base->toggled_low = 0;
	base->last_write_jiffies = j;
	spin_unlock_irqrestore(&base->lock, flags);
#else
	bcm477x_hello(priv, 0, 0);
#endif

	ret = tty->f_op->write(tty, buf, size, ppos);

    // KOM -- __DEBUG__
    //GPSINFO( "[SSPBBD]gpstty driver, written %d of %d", ret,size);

#ifndef USE_TIMER
	// KOM -- bcm477x_bye(priv);
#endif
	return ret;
}

static unsigned int bcm_tty_poll(struct file *filp, poll_table *wait)
{
    struct bcm_tty_priv *priv = NULL;
    struct file *tty = NULL;
	if(filp == NULL || wait == NULL)
	{
		GPSERR(" pointer is NOT init ");
		return 0;
	}
	priv = (struct bcm_tty_priv*) filp->private_data;
	tty = priv->tty;

	return tty->f_op->poll(tty, wait);
}

int bcm4774_suspend(gps_tty_platform_device *p, pm_message_t state)
{
	struct tty_struct *ttypoint;
	struct file *ttyfile;

	if((NULL == priv) || (NULL == priv->tty) || (NULL == priv->tty->private_data))
	{
		GPSERR(" priv is NOT init ");
		return 0;
	}

	ttyfile = priv->tty;
	ttypoint = ((struct tty_file_private *)ttyfile->private_data)->tty;
	if((NULL == ttypoint) || (NULL == ttypoint->port))
	{
		GPSERR(" ttypoint is NULL ");
		return 0;
	}
	/* Disable auto uart flow control by hardware, enable manual uart flow control by register */
	if (bcm_tty_config_close(ttyfile)) {
		GPSERR("can not change %s setting.", gps_tty);
		return 0;
	}
	/* Disable  uart RTS via register: pull high (register =0 means pull high) */
	tty_port_lower_dtr_rts(ttypoint->port);
	GPSINFO(" entry ++ :%s",  ttypoint->name);
	return 0;
}

int bcm4774_resume(gps_tty_platform_device *p)
{
	struct tty_struct *ttypoint;
	struct file *ttyfile;

	if((NULL == priv) || (NULL == priv->tty) || (NULL == priv->tty->private_data))
	{
		GPSERR(" priv is NOT init ");
		return 0;
	}

	ttyfile = priv->tty;
	ttypoint = ((struct tty_file_private *)ttyfile->private_data)->tty;
	if((NULL == ttypoint) || (NULL == ttypoint->port))
	{
		GPSERR(" ttypoint is NULL ");
		return 0;
	}

    tty_port_raise_dtr_rts(ttypoint->port);

	/* Enable auto uart flow control by hardware, manual uart flow control by register is disabled */
	if (bcm_tty_config(ttyfile)) {
		GPSERR(" can not change %s setting.", gps_tty);
		return 0;
	}

	GPSINFO(" -- :%s", ttypoint->name);
	return 0;
}

static long bcm_tty_ioctl( struct file *filp,
        unsigned int cmd, unsigned long arg)
{
#ifdef BCM_TTY_DEBUG
    GPSINFO("[SSPBBD]gpstty driver bcm_tty_ioctl is coming");
#endif
    return 0;
}

static const struct file_operations bcm_tty_fops = {
	.owner          =  THIS_MODULE,
	.open           =  bcm_tty_open,
	.release        =  bcm_tty_release,
	.read           =  bcm_tty_read,
	.write          =  bcm_tty_write,
	.poll           =  bcm_tty_poll,
	.unlocked_ioctl = bcm_tty_ioctl,
};

//--------------------------------------------------------------
//
//               Module init/exit
//
//--------------------------------------------------------------

static int __init bcm_tty_init(void)
{

	int ret;
	int mcu_req = 0;
	//int mcu_resp = 0;
	//int host_req = 0;
	/* Check GPIO# */
    struct device_node *np = NULL;

#ifdef CONFIG_HWCONNECTIVITY
    //For OneTrack, we need check it's the right chip type or not.
    //If it's not the right chip type, don't init the driver
    if (!isMyConnectivityChip(CHIP_TYPE_BCM)) {
        GPSERR("bcm tty chip type is not match, skip driver init");
        return -EINVAL;
    } else {
        GPSINFO("bcm tty chip type is matched with Broadcom, continue");
    }
#endif
    if (GPS_IC_TYPE_4775 == get_gps_ic_type()) {
        GPSINFO("set tty time parameter for 4775");
        g_BCM_TTY_DELTA_MS_TO_TOGGLE_LOW = 100;
        g_BCM_TTY_TIMER_IDLE_INTERVAL_MS = 25;
    }
	/*===================================================
	  We need folowing OF node in dts

	  bcm477x-gpio {
		  ssp-mcu-req = <some_gpio_number>
		  ssp-mcu-resp = <some_gpio_number>
		  ssp-host-req = <some_gpio_number>
	  }
	  ===================================================== */
	np = of_find_node_by_name(NULL, "gps_power");
	if (!np) {
		GPSERR("[SSPBBD]gpstty driver fail to find OF node huawei,gps_power");
		goto err_exit;
	}
	mcu_req = of_get_named_gpio(np,"huawei,mcu_req",0);
	//mcu_resp = of_get_named_gpio(np,"huawei,mcu_req_rsp",0);
	//host_req = of_get_named_gpio(np,"huawei,gps_hostwake",0);
#ifdef BCM_TTY_DEBUG_INFO
	GPSINFO("[SSPBBD]gpstty driver huawei,mcu_req=%d",  mcu_req);
#endif
	if (mcu_req<0) {
		GPSERR("[SSPBBD]: GPIO value not correct");
		goto err_exit;
	}

	/* Config GPIO */
	ret = gpio_request(mcu_req, "MCU REQ");
	if (ret)
	{
		GPSERR("MCU REQ gpio=%d, gpio_request fail. ret=%d", mcu_req, ret);
	}

	ret = gpio_direction_output(mcu_req, 0);
	if (ret) {
		GPSERR("gpio_direction_output %d failed, ret:0x%X", mcu_req, ret);
	}

	//gpio_request(mcu_resp, "MCU RESP");
	//gpio_direction_input(mcu_resp);

	/* Alloc */
	priv = (struct bcm_tty_priv*) kmalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		GPSERR("Failed to allocate \"gpstty\"");
		goto err_exit;
	}

	memset(priv, 0, sizeof(struct bcm_tty_priv));

	/* Init - gpios */
	//priv->host_req = host_req;
	priv->mcu_req  = mcu_req;
	//priv->mcu_resp = mcu_resp;

	/* Register misc device */
	priv->misc.minor = MISC_DYNAMIC_MINOR;
	priv->misc.name = "gpstty";
	priv->misc.fops = &bcm_tty_fops;

	/* Initialize alarm bases */
#ifdef USE_TIMER
	config_timer(priv);
#endif

	ret = misc_register(&priv->misc);
	if (ret) {
		GPSERR(" Failed to register gpstty. err=%d", ret);
		goto free_mem;
	}

	return 0;

free_mem:
	if (priv)
		kfree(priv);
err_exit:
	return -ENODEV;
}

static void __exit bcm_tty_exit(void)
{
	if(priv) {
		misc_deregister(&priv->misc);
		kfree(priv);
	}
}

module_init(bcm_tty_init);
module_exit(bcm_tty_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("BCM TTY Driver");
MODULE_AUTHOR("Broadcom Corporation");    ///< The author -- visible when you use modinfo
MODULE_VERSION("1.0");            ///< A version number to inform users
