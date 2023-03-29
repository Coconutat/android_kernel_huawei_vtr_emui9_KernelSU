/***************************************************************************//**
 *
 *  @file		tuner_drv.h
 *
 *  @brief		common header file for mm_tuner55x driver
 *
 *  @data		2011.07.25
 *
 *  @author	K.Kitamura(*)
 *  @author	K.Okawa(KXDA3)
 *
 ****************************************************************************//*
 * Copyright (c) 2015 Socionext Inc.
 *******************************************************************************
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
#ifndef _TUNER_DRV_H
#define _TUNER_DRV_H

#ifdef DEBUG
#include <linux/kernel.h>
#define __file__		(strrchr( __FILE__, '/') +1)
#define pr_fmt(fmt)	TUNER_CONFIG_DRIVER_NAME " %s(%d): " fmt, __file__, __LINE__
#else
#define pr_fmt(fmt)	TUNER_CONFIG_DRIVER_NAME " %s: " fmt, __FUNCTION__
#endif

#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/mutex.h>

#include "tuner_drv_sys.h"

/**
 * Structure providing the version of the tuner driver runtime
 */
struct _mmtuner_version {
	/** target device identifier
	 *
	 * Device id means two digits of bottom of type name.
	 */
	const uint8_t device;

	/** driver major version */
	const uint8_t major;

	/** driver minor version */
	const uint8_t minor;

	/** driver tinny version */
	const uint8_t hotfix;

	/** driver release candidate suffix string, e.g. "-rc4". */
	const char *rc;

	/** driver description */
	const char *describe;
};

/**
 * MMTUNER context structure that contain some parameters and
 * handle of the kernel threads.
 */
struct _mmtuner_cntxt {
	/**
	 * wait queue for the poll/select system call */
	wait_queue_head_t poll_waitq;
	/**
	 * exclusive control to access poll_flag */
	spinlock_t poll_lock;
	/**
	 * status flag for poll/select */
	uint32_t poll_flag;

	/**
	 * ID of kernel thread that handle interrupts (IRQ) from tuner device */
	struct task_struct *irqth_id;
	/**
	 * IRQ kernel thread wake up flag */
	uint32_t irqth_flag;
	/**
	 * IRQ kernel thread wait queue */
	wait_queue_head_t irqth_waitq;

	/**
	 * to record the IRQ (event) factor */
	TUNER_DATA_EVENT ev;

	/**
	 * multiple open count */
	uint32_t opcnt;

	uint32_t gpio_power;

	uint32_t gpio_power_v18;

	uint32_t gpio_nreset;
	
	uint32_t spi_cs_gpio;

	uint32_t i2c_driver_flag;
};

struct _tsif_cntxt {
	/* TS I/F Thread */
	struct task_struct *tsifth_id;		/* Kernel thread ID */
	bool tsifth_wait;						/* Wait indicator */
	uint32_t tsifth_flag;				/* wake up flag */
	wait_queue_head_t tsifth_waitq;		/* wait queue */

	/* TS packet buffer */
	uint8_t *pktbuf;						/* pointer to TS packet buffer */
	uint32_t pwr;							/* write position of TS packet buffer */
	uint32_t prd;							/* read position of TS packet buffer */
	uint32_t ovf;							/* packet buffer overflow counter */

	enum _bw_seg bw;						/* reception SEGment system */
	size_t ts_rxpkt_num;					/* num of packets a RX transaction */
	size_t ts_rx_size;					/* RX transaction byte size */
	size_t ts_pktbuf_size;				/* TS receive buffer size */

	/* read (TS data read) */
	uint32_t tsread_flag;					/* wake up flag */
	wait_queue_head_t tsread_waitq;			/* wait queue */

	struct _tuner_data_tsif *tsif;
};

/* flag bit position of IRQ kernel thread */
#define TUNER_IRQTH_NONE			0x00000000			/* initial flag */
#define TUNER_IRQKTH_CATCHIRQ	0x00000001			/* interrupt flag position */
#define TUNER_KTH_END			0x80000000			/* end flag position */

/* flag bit position of TS buffering thread */
#define TUNER_TSIFTH_NONE		0x00000000			/* initial flag */
#define TUNER_TSIFTH_ACTIVE		0x00000001			/* thread is working */
#define TUNER_TSIFTH_END			0x80000000			/* end flag position */

#define TUNER_TSIFTH_FIFOERROR_MAX	(10)

/* TS I/F thread retry & wait time */
#define TUNER_TSIFTH_SLEEP_RETRY	(500)
#define TUNER_TSIFTH_SLEEP_MIN		(100)
#define TUNER_TSIFTH_SLEEP_MAX		(200)

/* flag bit position of TS reading */
#define TUNER_TSREAD_WAIT		0x00000000		/* wait */
#define TUNER_TSREAD_IDLE		0x00000001		/* idle (initial) */
#define TUNER_TSREAD_ACTIVE		0x00000002		/* active (reading) */
#define TUNER_TSREAD_TIMEOUT	0x00000004		/* Timeout */
#define TUNER_TSREAD_END			0x80000000		/* end */


/* 4 : Use both IL=2 and IL=4 , then packet buffer is 1.0Mb (default)*/
/* 2 : USE ONLY IL=2 , then expand packet buffer to 2.7Mb */
/* Please set TUNER_TSPKTBUF_MODE to 2 when you don't need to Interleave mode 4 */
#define TUNER_TSPKTBUF_MODE     4

/* Maxium packet buffer size is defined by MAX_TSPKTBUF_SIZE.
   Also it will be limited by MAX_TSPKTBUF_BANK. The size of
   packet buffer will be calculated by the below,
   maxbank = TUNER_MAX_TSPKTBUF_SIZE/lowerlimit;
   if ( maxbank > TUNER_MAX_TSPKTBUF_BANK ) maxbank = TUNER_MAX_TSPKTBUF_BANK;
   packet buffer size =  tc->ts_rx_size) * maxbank; */
#define TUNER_MAX_TSPKTBUF_SIZE 2097152
#define TUNER_MAX_TSPKTBUF_BANK 48
#define TUNER_NO_INCR_ADDR 0xF6

#define DRIVER_LOAD 1
#define DRIVER_UNLOAD 0
#define TUNER_DELAY 10

#ifndef TUNER_CONFIG_IRQ_PC_LINUX
extern irqreturn_t tuner_interrupt( int irq, void *dev_id );
#else  /* TUNER_CONFIG_IRQ_PC_LINUX */
extern int tuner_interrupt( void );
#endif /* TUNER_CONFIG_IRQ_PC_LINUX */

extern int tuner_drv_hw_i2c_register(void);

extern void tuner_drv_hw_i2c_unregister(void);

extern int tuner_drv_start(void);
extern void tuner_drv_end(void);
#endif /* _TUNER_DRV_H */
/*******************************************************************************
 * Copyright (c) 2015 Socionext Inc.
 ******************************************************************************/
