/*
 * Linux DHD Bus Module for PCIE
 *
 * Copyright (C) 1999-2014, Broadcom Corporation
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: dhd_pcie.h 473468 2014-04-29 07:30:27Z $
 */


#ifndef dhd_pcie_h
#define dhd_pcie_h

#include <bcmpcie.h>
#include <hnd_cons.h>
#ifndef BCM_PCIE_UPDATE
#ifdef MSM_PCIE_LINKDOWN_RECOVERY
#if defined (CONFIG_ARCH_MSM)
#if defined (CONFIG_64BIT)
#include <linux/msm_pcie.h>
#else
#include <mach/msm_pcie.h>
#endif
#endif
#endif /* MSM_PCIE_LINKDOWN_RECOVERY */
#else  /* BCM_PCIE_UPDATE */
#ifdef SUPPORT_LINKDOWN_RECOVERY
#ifdef CONFIG_ARCH_MSM
#ifdef CONFIG_PCI_MSM
#include <linux/msm_pcie.h>
#else
#include <mach/msm_pcie.h>
#endif
#endif
#endif /* MSM_PCIE_LINKDOWN_RECOVERY */
#endif /* BCM_PCIE_UPDATE */
#ifdef CONFIG_ARCH_HISI
#include <linux/hisi/pcie-kirin-api.h>
#endif
/* defines */

#define PCMSGBUF_HDRLEN 0
#define DONGLE_REG_MAP_SIZE (32 * 1024)
#define DONGLE_TCM_MAP_SIZE (4096 * 1024)
#define DONGLE_MIN_MEMSIZE (128 *1024)
#ifdef DHD_DEBUG
#define DHD_PCIE_SUCCESS 0
#define DHD_PCIE_FAILURE 1
#endif /* DHD_DEBUG */
#define	REMAP_ENAB(bus)			((bus)->remap)
#define	REMAP_ISADDR(bus, a)		(((a) >= ((bus)->orig_ramsize)) && ((a) < ((bus)->ramsize)))

#ifdef BCM_PCIE_UPDATE
#ifdef SUPPORT_LINKDOWN_RECOVERY
#ifdef CONFIG_ARCH_MSM
#define struct_pcie_notify		struct msm_pcie_notify
#define struct_pcie_register_event	struct msm_pcie_register_event
#endif /* CONFIG_ARCH_MSM */
#ifdef CONFIG_ARCH_HISI
#define struct_pcie_notify	struct kirin_pcie_notify
#endif
#endif /* SUPPORT_LINKDOWN_RECOVERY */

/*
 * Router with 4366 can have 128 stations and 16 BSS,
 * hence (128 stations x 4 access categories for ucast) + 16 bc/mc flowrings
 */
#define MAX_DHD_TX_FLOWS	320

#define PCIE_LINK_DOWN		0xFFFFFFFF
#define DHD_INVALID 		-1
/* user defined data structures */
/* Device console log buffer state */
#define CONSOLE_LINE_MAX	192
#define CONSOLE_BUFFER_MAX	(8 * 1024)
#else
#define MAX_DHD_TX_FLOWS	256
#define PCIE_LINK_DOWN		0xFFFFFFFF
#define DHD_INVALID 		-1
/* Device console log buffer state */
#define CONSOLE_LINE_MAX	192
#define CONSOLE_BUFFER_MAX	2024
#endif /* BCM_PCIE_UPDATE */

#ifndef MAX_CNTL_D3ACK_TIMEOUT
#define MAX_CNTL_D3ACK_TIMEOUT 2
#endif /* MAX_CNTL_D3ACK_TIMEOUT */
#ifdef DEVICE_TX_STUCK_DETECT
#define DEVICE_TX_STUCK_CKECK_TIMEOUT	1000 /* 1 sec */
#define DEVICE_TX_STUCK_TIMEOUT		10000 /* 10 secs */
#define DEVICE_TX_STUCK_WARN_DURATION (DEVICE_TX_STUCK_TIMEOUT / DEVICE_TX_STUCK_CKECK_TIMEOUT)
#define DEVICE_TX_STUCK_DURATION (DEVICE_TX_STUCK_WARN_DURATION * 2)
#endif /* DEVICE_TX_STUCK_DETECT */

/* user defined data structures */
#ifdef DHD_DEBUG
typedef struct dhd_console {
	 uint		count;	/* Poll interval msec counter */
	 uint		log_addr;		 /* Log struct address (fixed) */
	 hnd_log_t	 log;			 /* Log struct (host copy) */
	 uint		 bufsize;		 /* Size of log buffer */
	 uint8		 *buf;			 /* Log buffer (host copy) */
	 uint		 last;			 /* Last buffer read index */
} dhd_console_t;
#endif /* DHD_DEBUG */
typedef struct ring_sh_info {
	uint32 ring_mem_addr;
	uint32 ring_state_w;
	uint32 ring_state_r;
} ring_sh_info_t;

typedef struct dhd_bus {
	dhd_pub_t	*dhd;
	struct pci_dev  *dev;		/* pci device handle */
	dll_t       const_flowring; /* constructed list of tx flowring queues */
#ifdef DEVICE_TX_STUCK_DETECT
	/* Flag to enable/disable device tx stuck monitor by DHD IOVAR dev_tx_stuck_monitor */
	uint32 dev_tx_stuck_monitor;
	/* Stores the timestamp (msec) of the last device Tx stuck check */
	unsigned long  device_tx_stuck_check;
#endif /* DEVICE_TX_STUCK_DETECT */

	si_t		*sih;			/* Handle for SI calls */
	char		*vars;			/* Variables (from CIS and/or other) */
	uint		varsz;			/* Size of variables buffer */
	uint32		sbaddr;			/* Current SB window pointer (-1, invalid) */
	sbpcieregs_t	*reg;			/* Registers for PCIE core */

	uint		armrev;			/* CPU core revision */
	uint		ramrev;			/* SOCRAM core revision */
	uint32		ramsize;		/* Size of RAM in SOCRAM (bytes) */
	uint32		orig_ramsize;		/* Size of RAM in SOCRAM (bytes) */
	uint32		srmemsize;		/* Size of SRMEM */

	uint32		bus;			/* gSPI or SDIO bus */
	uint32		intstatus;		/* Intstatus bits (events) pending */
	bool		dpc_sched;		/* Indicates DPC schedule (intrpt rcvd) */
	bool		fcstate;		/* State of dongle flow-control */

	uint16		cl_devid;		/* cached devid for dhdsdio_probe_attach() */
	char		*fw_path;		/* module_param: path to firmware image */
	char		*nv_path;		/* module_param: path to nvram vars file */
	char		*nvram_params;		/* user specified nvram params. */
	int		nvram_params_len;

	struct pktq	txq;			/* Queue length used for flow-control */

	uint		rxlen;			/* Length of valid data in buffer */


	bool		intr;			/* Use interrupts */
	bool		ipend;			/* Device interrupt is pending */
	bool		intdis;			/* Interrupts disabled by isr */
	uint		intrcount;		/* Count of device interrupt callbacks */
	uint		lastintrs;		/* Count as of last watchdog timer */

#ifdef DHD_DEBUG
	dhd_console_t	console;		/* Console output polling support */
	uint		console_addr;		/* Console address from shared struct */
#endif /* DHD_DEBUG */

	bool		alp_only;		/* Don't use HT clock (ALP only) */

	bool		remap;		/* Contiguous 1MB RAM: 512K socram + 512K devram
					 * Available with socram rev 16
					 * Remap region not DMA-able
					 */
	uint32		resetinstr;
	uint32		dongle_ram_base;

	ulong		shared_addr;
	pciedev_shared_t	*pcie_sh;
	bool bus_flowctrl;
	ioctl_comp_resp_msg_t	ioct_resp;
	uint32		dma_rxoffset;
	volatile char	*regs;		/* pci device memory va */
	volatile char	*tcm;		/* pci device memory va */
	uint32		tcm_size;
#if defined(CONFIG_ARCH_MSM) && defined(CONFIG_64BIT)
	uint32		bar1_win_base;
	uint32		bar1_win_mask;
#endif
	osl_t		*osh;
	uint32		nvram_csm;	/* Nvram checksum */
	uint16		pollrate;
	uint16  polltick;

	uint32  *pcie_mb_intr_addr;
	void    *pcie_mb_intr_osh;
	bool	sleep_allowed;

	wake_counts_t	wake_counts;

	/* version 3 shared struct related info start */
	ring_sh_info_t	ring_sh[BCMPCIE_COMMON_MSGRINGS + MAX_DHD_TX_FLOWS];
	uint8	h2d_ring_count;
	uint8	d2h_ring_count;
	uint32  ringmem_ptr;
	uint32  ring_state_ptr;

	uint32 d2h_dma_scratch_buffer_mem_addr;

	uint32 h2d_mb_data_ptr_addr;
	uint32 d2h_mb_data_ptr_addr;
	/* version 3 shared struct related info end */

	uint32 def_intmask;
	bool	ltrsleep_on_unload;
	uint	wait_for_d3_ack;
#ifndef BCM_PCIE_UPDATE
	uint8	txmode_push;
#endif
	uint32 max_sub_queues;
#ifdef BCM_PCIE_UPDATE
	uint32	rw_index_sz;
#endif
	bool	db1_for_mb;
	bool	suspended;
#ifndef BCM_PCIE_UPDATE
#ifdef MSM_PCIE_LINKDOWN_RECOVERY
	struct msm_pcie_register_event pcie_event;
	bool islinkdown;
#endif /* MSM_PCIE_LINKDOWN_RECOVERY */
#else
	bool	device_wake_state;
	bool	irq_registered;

#ifdef SUPPORT_LINKDOWN_RECOVERY
#ifdef CONFIG_ARCH_MSM
	uint8 no_cfg_restore;
	struct_pcie_register_event pcie_event;
#endif /* CONFIG_ARCH_MSM */
#ifdef CONFIG_ARCH_HISI
	struct kirin_pcie_register_event pcie_event;
#endif	
#endif /* SUPPORT_LINKDOWN_RECOVERY */
	uint32 d3_inform_cnt;
	uint32 d0_inform_cnt;
	uint32 d0_inform_in_use_cnt;
	uint8 force_suspend;
	uint32 d3_ack_war_cnt;
	uint8 is_linkdown;
	uint32 pci_d3hot_done;
#endif /*BCM_PCIE_UPDATE*/

} dhd_bus_t;

/* function declarations */

extern uint32* dhdpcie_bus_reg_map(osl_t *osh, ulong addr, int size);
extern int dhdpcie_bus_register(void);
extern void dhdpcie_bus_unregister(void);
extern bool dhdpcie_chipmatch(uint16 vendor, uint16 device);
#ifndef BCM_PCIE_UPDATE
extern struct dhd_bus* dhdpcie_bus_attach(osl_t *osh, volatile char* regs, volatile char* tcm);
#else
extern struct dhd_bus* dhdpcie_bus_attach(osl_t *osh,
	volatile char *regs, volatile char *tcm, void *pci_dev);
#endif
extern uint32 dhdpcie_bus_cfg_read_dword(struct dhd_bus *bus, uint32 addr, uint32 size);
extern void dhdpcie_bus_cfg_write_dword(struct dhd_bus *bus, uint32 addr, uint32 size, uint32 data);
extern void dhdpcie_bus_intr_disable(struct dhd_bus *bus);
extern void dhdpcie_bus_release(struct dhd_bus *bus);
extern int32 dhdpcie_bus_isr(struct dhd_bus *bus);
extern void dhdpcie_free_irq(dhd_bus_t *bus);
extern int dhdpcie_bus_suspend(struct  dhd_bus *bus, bool state);
#ifndef BCM_PCIE_UPDATE
extern int dhdpcie_pci_suspend_resume(struct pci_dev *dev, bool state);
#else
extern int dhdpcie_pci_suspend_resume(struct  dhd_bus *bus, bool state);
#ifndef BCMPCIE_OOB_HOST_WAKE
extern void dhdpcie_pme_active(osl_t *osh, bool enable);
extern bool dhdpcie_pme_cap(osl_t *osh);
#endif /* !BCMPCIE_OOB_HOST_WAKE */
#endif
extern int dhdpcie_start_host_pcieclock(dhd_bus_t *bus);
extern int dhdpcie_stop_host_pcieclock(dhd_bus_t *bus);
extern int dhdpcie_disable_device(dhd_bus_t *bus);
extern int dhdpcie_enable_device(dhd_bus_t *bus);
extern int dhdpcie_alloc_resource(dhd_bus_t *bus);
extern void dhdpcie_free_resource(dhd_bus_t *bus);
extern int dhdpcie_bus_request_irq(struct dhd_bus *bus);
#ifdef BCMPCIE_OOB_HOST_WAKE
extern int dhdpcie_oob_intr_register(dhd_bus_t *bus);
extern void dhdpcie_oob_intr_unregister(dhd_bus_t *bus);
extern void dhdpcie_oob_intr_set(dhd_bus_t *bus, bool enable);
#endif /* BCMPCIE_OOB_HOST_WAKE */

extern int dhd_buzzz_dump_dngl(dhd_bus_t *bus);
#ifdef DHD_WAKE_STATUS
int bcmpcie_get_total_wake(struct dhd_bus *bus);
int bcmpcie_set_get_wake(struct dhd_bus *bus, int flag);
#endif

#endif /* dhd_pcie_h */
