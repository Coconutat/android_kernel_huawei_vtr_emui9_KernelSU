/*
 * Broadcom Dongle Host Driver (DHD), Linux-specific network interface
 * Basically selected code segments from usb-cdc.c and usb-rndis.c
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
 * $Id: dhd_linux.c 477711 2014-05-14 08:45:17Z $
 */

#include <typedefs.h>
#include <linuxver.h>
#include <osl.h>
#ifdef SHOW_LOGTRACE
#include <linux/syscalls.h>
#include <event_log.h>
#endif /* SHOW_LOGTRACE */


#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/rtnetlink.h>
#include <linux/etherdevice.h>
#include <linux/random.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/ethtool.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/ip.h>
#include <linux/reboot.h>
#include <linux/notifier.h>
#include <net/addrconf.h>
#ifdef ENABLE_ADAPTIVE_SCHED
#include <linux/cpufreq.h>
#endif /* ENABLE_ADAPTIVE_SCHED */

#ifdef CONFIG_HWCONNECTIVITY
#include <huawei_platform/connectivity/hw_connectivity.h>
#endif

#include <asm/uaccess.h>
#include <asm/unaligned.h>

#include <epivers.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <bcmdevs.h>

#include <proto/ethernet.h>
#include <proto/bcmevent.h>
#include <proto/vlan.h>
#include <proto/bcmudp.h>
#include <proto/bcmdhcp.h>
#ifdef DHD_L2_FILTER
#include <proto/bcmicmp.h>
#endif
#include <proto/802.3.h>

#include <dngl_stats.h>
#include <dhd_linux_wq.h>
#include <dhd.h>
#include <dhd_linux.h>
#ifdef PCIE_FULL_DONGLE
#include <dhd_flowring.h>
#endif
#include <dhd_bus.h>
#include <dhd_proto.h>
#include <dhd_dbg.h>
#include <dhd_debug.h>
#include <hw_wifi.h>
#ifdef CONFIG_HW_WIFI_FREQ_CTRL_FLAG
#include "hw_wifi_freq_ctrl.h"
#endif
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif
#ifdef WL_CFG80211
#include <wl_cfg80211.h>
#endif
#ifdef PNO_SUPPORT
#include <dhd_pno.h>
#endif
#ifdef RTT_SUPPORT
#include <dhd_rtt.h>
#endif

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#ifdef DHD_WMF
#include <dhd_wmf_linux.h>
#endif /* DHD_WMF */

#ifdef DHDTCPACK_SUPPRESS
#include <dhd_ip.h>
#endif /* DHDTCPACK_SUPPRESS */

#if defined(DHD_TRACE_WAKE_LOCK)
#include <linux/sysfs.h>
#include <linux/kobject.h>
#endif

#ifdef WLMEDIA_HTSF
#include <linux/time.h>
#include <htsf.h>

#define HTSF_MINLEN 200    /* min. packet length to timestamp */
#define HTSF_BUS_DELAY 150 /* assume a fix propagation in us  */
#define TSMAX  1000        /* max no. of timing record kept   */
#define NUMBIN 34

static uint32 tsidx = 0;
static uint32 htsf_seqnum = 0;
uint32 tsfsync;
struct timeval tsync;
static uint32 tsport = 5010;

typedef struct histo_ {
	uint32 bin[NUMBIN];
} histo_t;

#if !ISPOWEROF2(DHD_SDALIGN)
#error DHD_SDALIGN is not a power of 2!
#endif

static histo_t vi_d1, vi_d2, vi_d3, vi_d4;
#endif /* WLMEDIA_HTSF */

#if defined(GSCAN_SUPPORT) && defined(HW_WIFI_DRIVER_NORMALIZE)
int g_gscan_maxap_per_scan = GSCAN_MAX_AP_PER_SCAN;
#endif

#if defined(SOFTAP)
extern bool ap_cfg_running;
extern bool ap_fw_loaded;
#endif

#ifdef SET_RANDOM_MAC_SOFTAP
#ifndef CONFIG_DHD_SET_RANDOM_MAC_VAL
#define CONFIG_DHD_SET_RANDOM_MAC_VAL	0x001A11
#endif
static u32 vendor_oui = CONFIG_DHD_SET_RANDOM_MAC_VAL;
#endif

#ifdef ENABLE_ADAPTIVE_SCHED
#define DEFAULT_CPUFREQ_THRESH		1000000	/* threshold frequency : 1000000 = 1GHz */
#ifndef CUSTOM_CPUFREQ_THRESH
#define CUSTOM_CPUFREQ_THRESH	DEFAULT_CPUFREQ_THRESH
#endif /* CUSTOM_CPUFREQ_THRESH */
#endif /* ENABLE_ADAPTIVE_SCHED */

/* enable HOSTIP cache update from the host side when an eth0:N is up */
#define AOE_IP_ALIAS_SUPPORT 1

#ifdef NVRAM_FROM_DTS
#define NV_PATH_MAX_LEN  129
#endif
#ifdef HW_WIFI_DRIVER_NORMALIZE
#define FW_PATH_MAX_LEN       (129)
#define CHIP_NAME_MAX_LEN       (17)
#endif /* HW_WIFI_DRIVER_NORMALIZE */

#ifdef BCM_FD_AGGR
#include <bcm_rpc.h>
#include <bcm_rpc_tp.h>
#endif
#ifdef PROP_TXSTATUS
#include <wlfc_proto.h>
#include <dhd_wlfc.h>
#endif

#include <wl_android.h>

/* Maximum STA per radio */
#define DHD_MAX_STA     32


const uint8 wme_fifo2ac[] = { 0, 1, 2, 3, 1, 1 };
const uint8 prio2fifo[8] = { 1, 0, 0, 1, 2, 2, 3, 3 };
#define WME_PRIO2AC(prio)  wme_fifo2ac[prio2fifo[(prio)]]

#ifdef ARP_OFFLOAD_SUPPORT
void aoe_update_host_ipv4_table(dhd_pub_t *dhd_pub, u32 ipa, bool add, int idx);
static int dhd_inetaddr_notifier_call(struct notifier_block *this,
	unsigned long event, void *ptr);
static struct notifier_block dhd_inetaddr_notifier = {
	.notifier_call = dhd_inetaddr_notifier_call
};
/* to make sure we won't register the same notifier twice, otherwise a loop is likely to be
 * created in kernel notifier link list (with 'next' pointing to itself)
 */
static bool dhd_inetaddr_notifier_registered = FALSE;
#endif /* ARP_OFFLOAD_SUPPORT */

#ifdef CONFIG_IPV6
static int dhd_inet6addr_notifier_call(struct notifier_block *this,
	unsigned long event, void *ptr);
static struct notifier_block dhd_inet6addr_notifier = {
	.notifier_call = dhd_inet6addr_notifier_call
};
/* to make sure we won't register the same notifier twice, otherwise a loop is likely to be
 * created in kernel notifier link list (with 'next' pointing to itself)
 */
static bool dhd_inet6addr_notifier_registered = FALSE;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)) && defined(CONFIG_PM_SLEEP)
#include <linux/suspend.h>
volatile bool dhd_mmc_suspend = FALSE;
DECLARE_WAIT_QUEUE_HEAD(dhd_dpc_wait);
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)) && defined(CONFIG_PM_SLEEP) */

#if defined(OOB_INTR_ONLY)
extern void dhd_enable_oob_intr(struct dhd_bus *bus, bool enable);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27))
static void dhd_hang_process(void *dhd_info, void *event_data, u8 event);
#endif
#ifdef	HW_PCIE_STABILITY
static void dhd_hang_work_handler(struct work_struct *work);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
MODULE_LICENSE("GPL v2");
#endif /* LinuxVer */

#include <dhd_bus.h>

#ifdef BCM_FD_AGGR
#define DBUS_RX_BUFFER_SIZE_DHD(net)	(BCM_RPC_TP_DNGL_AGG_MAX_BYTE)
#else
#ifndef PROP_TXSTATUS
#define DBUS_RX_BUFFER_SIZE_DHD(net)	(net->mtu + net->hard_header_len + dhd->pub.hdrlen)
#else
#define DBUS_RX_BUFFER_SIZE_DHD(net)	(net->mtu + net->hard_header_len + dhd->pub.hdrlen + 128)
#endif
#endif /* BCM_FD_AGGR */

#ifdef PROP_TXSTATUS
extern bool dhd_wlfc_skip_fc(void);
extern void dhd_wlfc_plat_init(void *dhd);
extern void dhd_wlfc_plat_deinit(void *dhd);
#endif /* PROP_TXSTATUS */

#ifdef HW_KERNEL_4_0_ADAPTATION
extern void set_wifi_open_err_code(int err_num);
#endif

#if LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 15)
const char *
print_tainted()
{
	return "";
}
#endif	/* LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 15) */

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(DHD_USE_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif /* defined(CONFIG_HAS_EARLYSUSPEND) && defined(DHD_USE_EARLYSUSPEND) */


#ifdef PKT_FILTER_SUPPORT
extern void dhd_pktfilter_offload_set(dhd_pub_t * dhd, char *arg);
extern void dhd_pktfilter_offload_enable(dhd_pub_t * dhd, char *arg, int enable, int master_mode);
extern void dhd_pktfilter_offload_delete(dhd_pub_t *dhd, int id);
#endif

#if defined(PKT_FILTER_SUPPORT) && defined(BCM_APF)
static int __dhd_apf_add_filter(struct net_device *ndev, uint32 filter_id,
	u8* program, uint32 program_len);
static int __dhd_apf_config_filter(struct net_device *ndev, uint32 filter_id,
	uint32 mode, uint32 enable);
static int __dhd_apf_delete_filter(struct net_device *ndev, uint32 filter_id);
#endif /* PKT_FILTER_SUPPORT && BCM_APF */

#ifdef READ_MACADDR
extern int dhd_read_macaddr(struct dhd_info *dhd);
#else
static inline int dhd_read_macaddr(struct dhd_info *dhd) { return 0; }
#endif
#ifdef WRITE_MACADDR
extern int dhd_write_macaddr(struct ether_addr *mac);
#else
static inline int dhd_write_macaddr(struct ether_addr *mac) { return 0; }
#endif




static int dhd_reboot_callback(struct notifier_block *this, unsigned long code, void *unused);
static struct notifier_block dhd_reboot_notifier = {
	.notifier_call = dhd_reboot_callback,
	.priority = 1,
};

#ifdef BCMPCIE
static int is_reboot = 0;
#endif /* BCMPCIE */

typedef struct dhd_dump {
	uint8 *buf;
	int bufsize;
} dhd_dump_t;

typedef struct dhd_if_event {
	struct list_head	list;
	wl_event_data_if_t	event;
	char			name[IFNAMSIZ+1];
	uint8			mac[ETHER_ADDR_LEN];
} dhd_if_event_t;

/* Interface control information */
typedef struct dhd_if {
	struct dhd_info *info;			/* back pointer to dhd_info */
	/* OS/stack specifics */
	struct net_device *net;
	int				idx;			/* iface idx in dongle */
	uint			subunit;		/* subunit */
	uint8			mac_addr[ETHER_ADDR_LEN];	/* assigned MAC address */
	bool			set_macaddress;
	bool			set_multicast;
	uint8			bssidx;			/* bsscfg index for the interface */
	bool			attached;		/* Delayed attachment when unset */
	bool			txflowcontrol;	/* Per interface flow control indicator */
	char			name[IFNAMSIZ+1]; /* linux interface name */
#ifdef  BRCM_RSDB	
	char			dngl_name[IFNAMSIZ+1]; /* corresponding dongle interface name */
#endif	
	struct net_device_stats stats;
#ifdef DHD_WMF
	dhd_wmf_t		wmf;		/* per bsscfg wmf setting */
#endif /* DHD_WMF */
#ifdef PCIE_FULL_DONGLE
	struct list_head sta_list;		/* sll of associated stations */
#if !defined(BCM_GMAC3)
	spinlock_t	sta_list_lock;		/* lock for manipulating sll */
#endif /* ! BCM_GMAC3 */
#endif /* PCIE_FULL_DONGLE */
	uint32  ap_isolate;			/* ap-isolation settings */
} dhd_if_t;

#ifdef WLMEDIA_HTSF
typedef struct {
	uint32 low;
	uint32 high;
} tsf_t;

typedef struct {
	uint32 last_cycle;
	uint32 last_sec;
	uint32 last_tsf;
	uint32 coef;     /* scaling factor */
	uint32 coefdec1; /* first decimal  */
	uint32 coefdec2; /* second decimal */
} htsf_t;

typedef struct {
	uint32 t1;
	uint32 t2;
	uint32 t3;
	uint32 t4;
} tstamp_t;

static tstamp_t ts[TSMAX];
static tstamp_t maxdelayts;
static uint32 maxdelay = 0, tspktcnt = 0, maxdelaypktno = 0;

#endif  /* WLMEDIA_HTSF */

struct ipv6_work_info_t {
	uint8			if_idx;
	char			ipv6_addr[16];
	unsigned long		event;
};

/* When Perimeter locks are deployed, any blocking calls must be preceeded
 * with a PERIM UNLOCK and followed by a PERIM LOCK.
 * Examples of blocking calls are: schedule_timeout(), down_interruptible(),
 * wait_event_timeout().
 */

/* Local private structure (extension of pub) */
typedef struct dhd_info {
	dhd_pub_t pub;
	dhd_if_t *iflist[DHD_MAX_IFS]; /* for supporting multiple interfaces */

	void *adapter;			/* adapter information, interrupt, fw path etc. */
	char fw_path[PATH_MAX];		/* path to firmware image */
	char nv_path[PATH_MAX];		/* path to nvram vars file */
#ifdef BCM_PCIE_UPDATE
	/* serialize dhd iovars */
	struct mutex dhd_iovar_mutex;
#endif
	struct semaphore proto_sem;
#ifdef PROP_TXSTATUS
	spinlock_t	wlfc_spinlock;

#endif /* PROP_TXSTATUS */
#ifdef WLMEDIA_HTSF
	htsf_t  htsf;
#endif
	wait_queue_head_t ioctl_resp_wait;
	wait_queue_head_t d3ack_wait;
#ifdef BCM_PCIE_UPDATE
	wait_queue_head_t dhd_bus_busy_state_wait;
#ifdef DHD_DEVWAKE_EARLY
	struct mutex ifdel_mutex[DHD_MAX_IFS]; /* for protecting multiple interfaces deletion */
	wait_queue_head_t devwake_wait;
#endif /* DHD_DEVWAKE_EARLY */
#endif
	uint32	default_wd_interval;

	struct timer_list timer;
	bool wd_timer_valid;
	struct tasklet_struct tasklet;
	spinlock_t	sdlock;
	spinlock_t	txqlock;
	spinlock_t	dhd_lock;

	struct mutex	sdmutex;
	tsk_ctl_t	thr_dpc_ctl;
	tsk_ctl_t	thr_wdt_ctl;

	tsk_ctl_t	thr_rxf_ctl;
	spinlock_t	rxf_lock;
	bool		rxthread_enabled;

	/* Wakelocks */
#if defined(CONFIG_HAS_WAKELOCK) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27))
	struct wake_lock wl_wifi;   /* Wifi wakelock */
	struct wake_lock wl_rxwake; /* Wifi rx wakelock */
	struct wake_lock wl_ctrlwake; /* Wifi ctrl wakelock */
	struct wake_lock wl_wdwake; /* Wifi wd wakelock */
#ifdef BCM_PCIE_UPDATE
	struct wake_lock wl_evtwake; /* Wifi event wakelock */
#endif
#ifdef BCMPCIE_OOB_HOST_WAKE
	struct wake_lock wl_intrwake; /* Host wakeup wakelock */
#endif /* BCMPCIE_OOB_HOST_WAKE */
#ifdef DHD_USE_SCAN_WAKELOCK
	struct wake_lock wl_scanwake;  /* Wifi scan wakelock */
#endif /* DHD_USE_SCAN_WAKELOCK */
#endif /* CONFIG_HAS_WAKELOCK && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27) */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25))
	/* net_device interface lock, prevent race conditions among net_dev interface
	 * calls and wifi_on or wifi_off
	 */
#ifdef HW_WIFI_OPEN_STOP
	struct mutex dhd_open_stop_if_mutex;
#endif
	struct mutex dhd_net_if_mutex;
	struct mutex dhd_suspend_mutex;
#endif
#if defined(PKT_FILTER_SUPPORT) && defined(BCM_APF)
	struct mutex dhd_apf_mutex;
#endif /* PKT_FILTER_SUPPORT && BCM_APF */
	spinlock_t wakelock_spinlock;
#ifdef BCM_PCIE_UPDATE
	spinlock_t wakelock_evt_spinlock;
	uint32 wakelock_event_counter;
#endif
	uint32 wakelock_counter;
	int wakelock_wd_counter;
	int wakelock_rx_timeout_enable;
	int wakelock_ctrl_timeout_enable;
	bool waive_wakelock;
	uint32 wakelock_before_waive;

	/* Thread to issue ioctl for multicast */
	wait_queue_head_t ctrl_wait;
	atomic_t pend_8021x_cnt;
	dhd_attach_states_t dhd_state;
#ifdef SHOW_LOGTRACE
	dhd_event_log_t event_data;
#endif /* SHOW_LOGTRACE */

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(DHD_USE_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif /* CONFIG_HAS_EARLYSUSPEND && DHD_USE_EARLYSUSPEND */

#if !defined(CONFIG_HAS_EARLYSUSPEND) && defined(HW_AP_POWERSAVE)
	struct notifier_block fb_notifier;
#endif

#ifdef ARP_OFFLOAD_SUPPORT
	u32 pend_ipaddr;
#endif /* ARP_OFFLOAD_SUPPORT */
#ifdef BCM_FD_AGGR
	void *rpc_th;
	void *rpc_osh;
	struct timer_list rpcth_timer;
	bool rpcth_timer_active;
	bool fdaggr;
#endif
#ifdef DHDTCPACK_SUPPRESS
	spinlock_t	tcpack_lock;
#endif /* DHDTCPACK_SUPPRESS */
	void			*dhd_deferred_wq;
#ifdef DEBUG_CPU_FREQ
	struct notifier_block freq_trans;
	int __percpu *new_freq;
#endif
	unsigned int unit;
	struct notifier_block pm_notifier;
#ifdef SAR_SUPPORT
	struct notifier_block sar_notifier;
	s32 sar_enable;
#endif
#ifdef DHD_TPUT_MONITOR
	unsigned long last_jiffies;
	struct timer_list tput_timer;
	tsk_ctl_t tput_tsk;
#endif /* DHD_TPUT_MONITOR */
#ifdef DHD_DEVWAKE_EARLY
	tsk_ctl_t devwake_tsk;
#endif /* DHD_DEVWAKE_EARLY */
#ifdef DHD_TRACE_WAKE_LOCK
	struct kobject dhd_kobj;
#endif
} dhd_info_t;

#ifdef HW_WIFI_OPEN_STOP
static void dhd_open_stop_if_mutex_lock(dhd_info_t *dhd);
static void dhd_open_stop_if_mutex_unlock(dhd_info_t *dhd);
#endif
#define DHDIF_FWDER(dhdif)      FALSE

/* Flag to indicate if we should download firmware on driver load */
uint dhd_download_fw_on_driverload = TRUE;

/* Flag to indicate if driver is initialized */
uint dhd_driver_init_done = FALSE;

/* Definitions to provide path to the firmware and nvram
 * example nvram_path[MOD_PARAM_PATHLEN]="/projects/wlan/nvram.txt"
 */
char firmware_path[MOD_PARAM_PATHLEN];
char nvram_path[MOD_PARAM_PATHLEN];
char region[MOD_PARAM_PATHLEN];
#if defined(SOFTAP) && defined(HW_SOFTAP_MANAGEMENT)
char g_fw_path[MOD_PARAM_PATHLEN];
#endif
/* backup buffer for firmware and nvram path */
char fw_bak_path[MOD_PARAM_PATHLEN];
char nv_bak_path[MOD_PARAM_PATHLEN];

/* information string to keep firmware, chio, cheip version info visiable from log */
char info_string[MOD_PARAM_INFOLEN];
module_param_string(info_string, info_string, MOD_PARAM_INFOLEN, 0444);
int op_mode = 0;
int disable_proptx = 0;
module_param(op_mode, int, 0644);
extern int wl_control_wl_start(struct net_device *dev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)) && defined(BCMLXSDMMC)
struct semaphore dhd_registration_sem;
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)) */

/* deferred handlers */
static void dhd_ifadd_event_handler(void *handle, void *event_info, u8 event);
static void dhd_ifdel_event_handler(void *handle, void *event_info, u8 event);
static void dhd_set_mac_addr_handler(void *handle, void *event_info, u8 event);
static void dhd_set_mcast_list_handler(void *handle, void *event_info, u8 event);
#ifdef CONFIG_IPV6
static void dhd_inet6_work_handler(void *dhd_info, void *event_data, u8 event);
#endif

#ifdef WL_CFG80211
extern void dhd_netdev_free(struct net_device *ndev);
#endif /* WL_CFG80211 */

/* Error bits */
module_param(dhd_msg_level, int, 0);

#ifdef ARP_OFFLOAD_SUPPORT
/* ARP offload enable */
uint dhd_arp_enable = TRUE;
module_param(dhd_arp_enable, uint, 0);

/* ARP offload agent mode : Enable ARP Host Auto-Reply and ARP Peer Auto-Reply */

uint dhd_arp_mode = ARP_OL_AGENT | ARP_OL_PEER_AUTO_REPLY;

module_param(dhd_arp_mode, uint, 0);
#endif /* ARP_OFFLOAD_SUPPORT */

/* Disable Prop tx */
module_param(disable_proptx, int, 0644);
/* load firmware and/or nvram values from the filesystem */
module_param_string(firmware_path, firmware_path, MOD_PARAM_PATHLEN, 0660);
module_param_string(nvram_path, nvram_path, MOD_PARAM_PATHLEN, 0660);
module_param_string(region, region, MOD_PARAM_PATHLEN, 0660);
/* Watchdog interval */

/* extend watchdog expiration to 2 seconds when DPC is running */
#define WATCHDOG_EXTEND_INTERVAL (2000)

uint dhd_watchdog_ms = CUSTOM_DHD_WATCHDOG_MS;
module_param(dhd_watchdog_ms, uint, 0);

#if defined(DHD_DEBUG)
/* Console poll interval */
uint dhd_console_ms = 0;
module_param(dhd_console_ms, uint, 0664);
#endif /* defined(DHD_DEBUG) */


uint dhd_slpauto = TRUE;
module_param(dhd_slpauto, uint, 0);

#ifdef PKT_FILTER_SUPPORT
/* Global Pkt filter enable control */
uint dhd_pkt_filter_enable = TRUE;
module_param(dhd_pkt_filter_enable, uint, 0);
#endif

/* Pkt filter init setup */
uint dhd_pkt_filter_init = 0;
module_param(dhd_pkt_filter_init, uint, 0);

/* Pkt filter mode control */
uint dhd_master_mode = TRUE;
module_param(dhd_master_mode, uint, 0);

int dhd_watchdog_prio = 0;
module_param(dhd_watchdog_prio, int, 0);

/* DPC thread priority */
int dhd_dpc_prio = CUSTOM_DPC_PRIO_SETTING;
module_param(dhd_dpc_prio, int, 0);

/* RX frame thread priority */
int dhd_rxf_prio = CUSTOM_RXF_PRIO_SETTING;
module_param(dhd_rxf_prio, int, 0);

#ifdef  BRCM_RSDB
int passive_channel_skip = 0;
module_param(passive_channel_skip, int, (S_IRUSR|S_IWUSR));
#endif
#if !defined(BCMDHDUSB)
extern int dhd_dongle_ramsize;
module_param(dhd_dongle_ramsize, int, 0);
#endif /* BCMDHDUSB */

/* Keep track of number of instances */
static int dhd_found = 0;
static int instance_base = 0; /* Starting instance number */
module_param(instance_base, int, 0644);
#ifdef DHD_TRACE_WAKE_LOCK
/* Functions to manage sysfs interface for dhd */
static int dhd_sysfs_init(dhd_info_t *dhd);
static void dhd_sysfs_exit(dhd_info_t *dhd);
#endif
/* DHD Perimiter lock only used in router with bypass forwarding. */
#define DHD_PERIM_RADIO_INIT()              do { /* noop */ } while (0)
#define DHD_PERIM_LOCK_TRY(unit, flag)      do { /* noop */ } while (0)
#define DHD_PERIM_UNLOCK_TRY(unit, flag)    do { /* noop */ } while (0)
#ifndef BCM_PCIE_UPDATE
#define DHD_PERIM_LOCK_ALL()                do { /* noop */ } while (0)
#define DHD_PERIM_UNLOCK_ALL()              do { /* noop */ } while (0)
#endif
#ifdef PCIE_FULL_DONGLE
#if defined(BCM_GMAC3)
#define DHD_IF_STA_LIST_LOCK_INIT(ifp)      do { /* noop */ } while (0)
#define DHD_IF_STA_LIST_LOCK(ifp, flags)    ({ BCM_REFERENCE(flags); })
#define DHD_IF_STA_LIST_UNLOCK(ifp, flags)  ({ BCM_REFERENCE(flags); })
#else /* ! BCM_GMAC3 */
#define DHD_IF_STA_LIST_LOCK_INIT(ifp) spin_lock_init(&(ifp)->sta_list_lock)
#define DHD_IF_STA_LIST_LOCK(ifp, flags) \
	spin_lock_irqsave(&(ifp)->sta_list_lock, (flags))
#define DHD_IF_STA_LIST_UNLOCK(ifp, flags) \
	spin_unlock_irqrestore(&(ifp)->sta_list_lock, (flags))
#endif /* ! BCM_GMAC3 */
#endif /* PCIE_FULL_DONGLE */

/* Control fw roaming */
uint dhd_roam_disable = 0;

/* Control radio state */
uint dhd_radio_up = 1;

/* Network inteface name */
char iface_name[IFNAMSIZ] = {'\0'};
module_param_string(iface_name, iface_name, IFNAMSIZ, 0);

/* The following are specific to the SDIO dongle */

/* IOCTL response timeout */
int dhd_ioctl_timeout_msec = IOCTL_RESP_TIMEOUT;

/* Idle timeout for backplane clock */
int dhd_idletime = DHD_IDLETIME_TICKS;
module_param(dhd_idletime, int, 0);

/* Use polling */
#ifdef HW_WIFI_POLL_ENABLE
uint dhd_poll = TRUE;
#else
uint dhd_poll = FALSE;
#endif
module_param(dhd_poll, uint, 0);

/* Use interrupts */
uint dhd_intr = TRUE;
module_param(dhd_intr, uint, 0);

/* SDIO Drive Strength (in milliamps) */
uint dhd_sdiod_drive_strength = 6;
module_param(dhd_sdiod_drive_strength, uint, 0);

#ifdef BCMSDIO
/* Tx/Rx bounds */
extern uint dhd_txbound;
extern uint dhd_rxbound;
module_param(dhd_txbound, uint, 0);
module_param(dhd_rxbound, uint, 0);

/* Deferred transmits */
extern uint dhd_deferred_tx;
module_param(dhd_deferred_tx, uint, 0);

#ifdef BCMDBGFS
extern void dhd_dbgfs_init(dhd_pub_t *dhdp);
extern void dhd_dbgfs_remove(void);
#endif /* BCMDBGFS */

#endif /* BCMSDIO */


#ifdef SDTEST
/* Echo packet generator (pkts/s) */
uint dhd_pktgen = 0;
module_param(dhd_pktgen, uint, 0);

/* Echo packet len (0 => sawtooth, max 2040) */
uint dhd_pktgen_len = 0;
module_param(dhd_pktgen_len, uint, 0);
#endif /* SDTEST */


extern char dhd_version[];

int dhd_net_bus_devreset(struct net_device *dev, uint8 flag);
static void dhd_net_if_lock_local(dhd_info_t *dhd);
static void dhd_net_if_unlock_local(dhd_info_t *dhd);
static void dhd_suspend_lock(dhd_pub_t *dhdp);
static void dhd_suspend_unlock(dhd_pub_t *dhdp);

#ifdef WLMEDIA_HTSF
void htsf_update(dhd_info_t *dhd, void *data);
tsf_t prev_tsf, cur_tsf;

uint32 dhd_get_htsf(dhd_info_t *dhd, int ifidx);
static int dhd_ioctl_htsf_get(dhd_info_t *dhd, int ifidx);
static void dhd_dump_latency(void);
static void dhd_htsf_addtxts(dhd_pub_t *dhdp, void *pktbuf);
static void dhd_htsf_addrxts(dhd_pub_t *dhdp, void *pktbuf);
static void dhd_dump_htsfhisto(histo_t *his, char *s);
#endif /* WLMEDIA_HTSF */

/* Monitor interface */
int dhd_monitor_init(void *dhd_pub);
int dhd_monitor_uninit(void);


static void dhd_dpc(ulong data);
/* forward decl */
extern int dhd_wait_pend8021x(struct net_device *dev);
void dhd_os_wd_timer_extend(void *bus, bool extend);

#ifdef TOE
#ifndef BDC
#error TOE requires BDC
#endif /* !BDC */
static int dhd_toe_get(dhd_info_t *dhd, int idx, uint32 *toe_ol);
static int dhd_toe_set(dhd_info_t *dhd, int idx, uint32 toe_ol);
#endif /* TOE */
#if defined (BCM_PATCH_FOR_ETHERTYPE_SECURITY)
static int dhd_wl_host_event(dhd_info_t *dhd, int *ifidx, void *pktdata, uint pktlen,
                             wl_event_msg_t *event_ptr, void **data_ptr);
#else
static int dhd_wl_host_event(dhd_info_t *dhd, int *ifidx, void *pktdata,
                             wl_event_msg_t *event_ptr, void **data_ptr);
#endif /*BCM_PATCH_FOR_ETHERTYPE_SECURITY*/
#ifdef DHD_UNICAST_DHCP
static const uint8 llc_snap_hdr[SNAP_HDR_LEN] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00};
static int dhd_get_pkt_ip_type(dhd_pub_t *dhd, void *skb, uint8 **data_ptr,
	int *len_ptr, uint8 *prot_ptr);
static int dhd_get_pkt_ether_type(dhd_pub_t *dhd, void *skb, uint8 **data_ptr,
	int *len_ptr, uint16 *et_ptr, bool *snap_ptr);

static int dhd_convert_dhcp_broadcast_ack_to_unicast(dhd_pub_t *pub, void *pktbuf, int ifidx);
#endif /* DHD_UNICAST_DHCP */
#ifdef DHD_L2_FILTER
static int dhd_l2_filter_block_ping(dhd_pub_t *pub, void *pktbuf, int ifidx);
#endif
#if defined(CONFIG_PM_SLEEP)
static int dhd_pm_callback(struct notifier_block *nfb, unsigned long action, void *ignored)
{
	int ret = NOTIFY_DONE;
	bool suspend = FALSE;
	dhd_info_t *dhdinfo = (dhd_info_t*)container_of(nfb, struct dhd_info, pm_notifier);

	BCM_REFERENCE(dhdinfo);
	switch (action) {
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		suspend = TRUE;
		break;
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
		suspend = FALSE;
		break;
	}

#if defined(SUPPORT_P2P_GO_PS)
#ifdef PROP_TXSTATUS
	if (suspend) {
		DHD_OS_WAKE_LOCK_WAIVE(&dhdinfo->pub);
		dhd_wlfc_suspend(&dhdinfo->pub);
		DHD_OS_WAKE_LOCK_RESTORE(&dhdinfo->pub);
	} else
		dhd_wlfc_resume(&dhdinfo->pub);
#endif
#endif /* defined(SUPPORT_P2P_GO_PS) */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)) && (LINUX_VERSION_CODE <= \
	KERNEL_VERSION(2, 6, 39))
	dhd_mmc_suspend = suspend;
	smp_mb();
#endif

	return ret;
}

static struct notifier_block dhd_pm_notifier = {
	.notifier_call = dhd_pm_callback,
	.priority = 10
};
/* to make sure we won't register the same notifier twice, otherwise a loop is likely to be
 * created in kernel notifier link list (with 'next' pointing to itself)
 */
static bool dhd_pm_notifier_registered = FALSE;

extern int register_pm_notifier(struct notifier_block *nb);
extern int unregister_pm_notifier(struct notifier_block *nb);
#endif /* CONFIG_PM_SLEEP */

/* Request scheduling of the bus rx frame */
static void dhd_sched_rxf(dhd_pub_t *dhdp, void *skb);
static void dhd_os_rxflock(dhd_pub_t *pub);
static void dhd_os_rxfunlock(dhd_pub_t *pub);

/** priv_link is the link between netdev and the dhdif and dhd_info structs. */
typedef struct dhd_dev_priv {
	dhd_info_t * dhd; /* cached pointer to dhd_info in netdevice priv */
	dhd_if_t   * ifp; /* cached pointer to dhd_if in netdevice priv */
	int          ifidx; /* interface index */
} dhd_dev_priv_t;

#define DHD_DEV_PRIV_SIZE       (sizeof(dhd_dev_priv_t))
#define DHD_DEV_PRIV(dev)       ((dhd_dev_priv_t *)DEV_PRIV(dev))
#define DHD_DEV_INFO(dev)       (((dhd_dev_priv_t *)DEV_PRIV(dev))->dhd)
#define DHD_DEV_IFP(dev)        (((dhd_dev_priv_t *)DEV_PRIV(dev))->ifp)
#define DHD_DEV_IFIDX(dev)      (((dhd_dev_priv_t *)DEV_PRIV(dev))->ifidx)

#if defined(DHD_OF_SUPPORT)
extern int dhd_wlan_init(void);
extern void dhd_wlan_exit(void);
#endif /* defined(DHD_OF_SUPPORT) */


/** Clear the dhd net_device's private structure. */
static inline void
dhd_dev_priv_clear(struct net_device * dev)
{
	dhd_dev_priv_t * dev_priv;
	ASSERT(dev != (struct net_device *)NULL);
	dev_priv = DHD_DEV_PRIV(dev);
	dev_priv->dhd = (dhd_info_t *)NULL;
	dev_priv->ifp = (dhd_if_t *)NULL;
	dev_priv->ifidx = DHD_BAD_IF;
}

/** Setup the dhd net_device's private structure. */
static inline void
dhd_dev_priv_save(struct net_device * dev, dhd_info_t * dhd, dhd_if_t * ifp,
                  int ifidx)
{
	dhd_dev_priv_t * dev_priv;
	ASSERT(dev != (struct net_device *)NULL);
	dev_priv = DHD_DEV_PRIV(dev);
	dev_priv->dhd = dhd;
	dev_priv->ifp = ifp;
	dev_priv->ifidx = ifidx;
}
#ifdef SAR_SUPPORT
static int dhd_sar_callback(struct notifier_block *nfb, unsigned long action, void *data)
{
	dhd_info_t *dhd = (dhd_info_t*)container_of(nfb, struct dhd_info, sar_notifier);
	s32 sar_enable;
	s32 txpower;
	int ret;

	if (dhd->pub.busstate == DHD_BUS_DOWN || dhd->pub.up == 0) {
		DHD_ERROR(("%s Not ready, Bus state %d firmware state %d\n",
		       __FUNCTION__, dhd->pub.busstate, dhd->pub.up));
		return NOTIFY_BAD;
	}

	if (data) {
		/* if data != NULL then we expect that the notifier passed
		 * the exact value of max tx power in quarters of dB.
		 * qtxpower variable allows us to overwrite TX power.
		 */
		txpower = *(s32*)data;
		if (txpower == -1 || txpower >= 127)
			txpower = 127; /* Max val of 127 qdbm */
		else
			txpower |= WL_TXPWR_OVERRIDE;

		txpower = htod32(txpower);

		ret = dhd_iovar(&dhd->pub, 0, "qtxpower", (char *)&txpower,
				sizeof(txpower), NULL, 0, TRUE);
		if (ret < 0)
			DHD_ERROR(("%s wl qtxpower failed %d\n", __FUNCTION__, ret));
	} else {
		/* '1' means activate sarlimit and '0' means back to normal
		 *  state (deactivate sarlimit)
		 */
		sar_enable = action ? 1 : 0;
		if (dhd->sar_enable == sar_enable)
			return NOTIFY_DONE;
		ret = dhd_iovar(&dhd->pub, 0, "sar_enable",
				(char *)&sar_enable, sizeof(sar_enable),
				NULL, 0, TRUE);
		if (ret < 0)
			DHD_ERROR(("%s wl sar_enable %d failed %d\n", __FUNCTION__, sar_enable, ret));
		else
			dhd->sar_enable = sar_enable;
	}

	return NOTIFY_DONE;
}

static bool dhd_sar_notifier_registered = FALSE;

extern int register_notifier_by_sar(struct notifier_block *nb);
extern int unregister_notifier_by_sar(struct notifier_block *nb);
#endif

#ifdef PCIE_FULL_DONGLE

/** Dummy objects are defined with state representing bad|down.
 * Performance gains from reducing branch conditionals, instruction parallelism,
 * dual issue, reducing load shadows, avail of larger pipelines.
 * Use DHD_XXX_NULL instead of (dhd_xxx_t *)NULL, whenever an object pointer
 * is accessed via the dhd_sta_t.
 */

/* Dummy dhd_info object */
dhd_info_t dhd_info_null = {
#if defined(BCM_GMAC3)
	.fwdh = FWDER_NULL,
#endif
	.pub = {
	         .info = &dhd_info_null,
#ifdef DHDTCPACK_SUPPRESS
	         .tcpack_sup_mode = TCPACK_SUP_REPLACE,
#endif /* DHDTCPACK_SUPPRESS */
	         .up = FALSE, .busstate = DHD_BUS_DOWN
	}
};
#define DHD_INFO_NULL (&dhd_info_null)
#define DHD_PUB_NULL  (&dhd_info_null.pub)

/* Dummy netdevice object */
struct net_device dhd_net_dev_null = {
	.reg_state = NETREG_UNREGISTERED
};
#define DHD_NET_DEV_NULL (&dhd_net_dev_null)

/* Dummy dhd_if object */
dhd_if_t dhd_if_null = {
#if defined(BCM_GMAC3)
	.fwdh = FWDER_NULL,
#endif
#ifdef WMF
	.wmf = { .wmf_enable = TRUE },
#endif
	.info = DHD_INFO_NULL,
	.net = DHD_NET_DEV_NULL,
	.idx = DHD_BAD_IF
};
#define DHD_IF_NULL  (&dhd_if_null)

#define DHD_STA_NULL ((dhd_sta_t *)NULL)

/** Interface STA list management. */

/** Fetch the dhd_if object, given the interface index in the dhd. */
static inline dhd_if_t *dhd_get_ifp(dhd_pub_t *dhdp, uint32 ifidx);

/** Alloc/Free a dhd_sta object from the dhd instances' sta_pool. */
static void dhd_sta_free(dhd_pub_t *pub, dhd_sta_t *sta);
static dhd_sta_t * dhd_sta_alloc(dhd_pub_t * dhdp);

/* Delete a dhd_sta or flush all dhd_sta in an interface's sta_list. */
static void dhd_if_del_sta_list(dhd_if_t * ifp);
static void	dhd_if_flush_sta(dhd_if_t * ifp);

/* Construct/Destruct a sta pool. */
static int dhd_sta_pool_init(dhd_pub_t *dhdp, int max_sta);
static void dhd_sta_pool_fini(dhd_pub_t *dhdp, int max_sta);
static void dhd_sta_pool_clear(dhd_pub_t *dhdp, int max_sta);


/* Return interface pointer */
static inline dhd_if_t *dhd_get_ifp(dhd_pub_t *dhdp, uint32 ifidx)
{
	ASSERT(ifidx < DHD_MAX_IFS);
	if (ifidx >= DHD_MAX_IFS) {
		return NULL;
	}
	return dhdp->info->iflist[ifidx];
}
#ifndef BCM_PCIE_UPDATE
/** Reset a dhd_sta object and free into the dhd pool. */
static void
dhd_sta_free(dhd_pub_t * dhdp, dhd_sta_t * sta)
{
	int prio;

	ASSERT((sta != DHD_STA_NULL) && (sta->idx != ID16_INVALID));

	ASSERT((dhdp->staid_allocator != NULL) && (dhdp->sta_pool != NULL));
	id16_map_free(dhdp->staid_allocator, sta->idx);
	for (prio = 0; prio < (int)NUMPRIO; prio++)
		sta->flowid[prio] = FLOWID_INVALID;
	sta->ifp = DHD_IF_NULL; /* dummy dhd_if object */
	sta->ifidx = DHD_BAD_IF;
	bzero(sta->ea.octet, ETHER_ADDR_LEN);
	INIT_LIST_HEAD(&sta->list);
	sta->idx = ID16_INVALID; /* implying free */
}
#else
static void
dhd_sta_free(dhd_pub_t * dhdp, dhd_sta_t * sta)
{
	int prio;

	ASSERT((sta != DHD_STA_NULL) && (sta->idx != ID16_INVALID));

	ASSERT((dhdp->staid_allocator != NULL) && (dhdp->sta_pool != NULL));

	/*
	 * Flush and free all packets in all flowring's queues belonging to sta.
	 * Packets in flow ring will be flushed later.
	 */
	for (prio = 0; prio < (int)NUMPRIO; prio++) {
		uint16 flowid = sta->flowid[prio];

		if (flowid != FLOWID_INVALID) {
			unsigned long flags;
			flow_queue_t * queue = dhd_flow_queue(dhdp, flowid);
			flow_ring_node_t * flow_ring_node;

#ifdef DHDTCPACK_SUPPRESS
			/* Clean tcp_ack_info_tbl in order to prevent access to flushed pkt,
			 * when there is a newly coming packet from network stack.
			 */
			dhd_tcpack_info_tbl_clean(dhdp);
#endif /* DHDTCPACK_SUPPRESS */

			flow_ring_node = dhd_flow_ring_node(dhdp, flowid);
			DHD_FLOWRING_LOCK(flow_ring_node->lock, flags);
			flow_ring_node->status = FLOW_RING_STATUS_STA_FREEING;

			if (!DHD_FLOW_QUEUE_EMPTY(queue)) {
				void * pkt;
				while ((pkt = dhd_flow_queue_dequeue(dhdp, queue)) != NULL) {
					PKTFREE(dhdp->osh, pkt, TRUE);
				}
			}

			DHD_FLOWRING_UNLOCK(flow_ring_node->lock, flags);
			ASSERT(DHD_FLOW_QUEUE_EMPTY(queue));
		}

		sta->flowid[prio] = FLOWID_INVALID;
	}

	id16_map_free(dhdp->staid_allocator, sta->idx);
	sta->ifp = DHD_IF_NULL; /* dummy dhd_if object */
	sta->ifidx = DHD_BAD_IF;
	bzero(sta->ea.octet, ETHER_ADDR_LEN);
	INIT_LIST_HEAD(&sta->list);
	sta->idx = ID16_INVALID; /* implying free */
}
#endif
/** Allocate a dhd_sta object from the dhd pool. */
static dhd_sta_t *
dhd_sta_alloc(dhd_pub_t * dhdp)
{
	uint16 idx;
	dhd_sta_t * sta;
	dhd_sta_pool_t * sta_pool;

	ASSERT((dhdp->staid_allocator != NULL) && (dhdp->sta_pool != NULL));

	idx = id16_map_alloc(dhdp->staid_allocator);
	if (idx == ID16_INVALID) {
		DHD_ERROR(("%s: cannot get free staid\n", __FUNCTION__));
		return DHD_STA_NULL;
	}

	sta_pool = (dhd_sta_pool_t *)(dhdp->sta_pool);
	sta = &sta_pool[idx];

	ASSERT((sta->idx == ID16_INVALID) &&
	       (sta->ifp == DHD_IF_NULL) && (sta->ifidx == DHD_BAD_IF));
	sta->idx = idx; /* implying allocated */

	return sta;
}

/** Delete all STAs in an interface's STA list. */
static void
dhd_if_del_sta_list(dhd_if_t *ifp)
{
	dhd_sta_t *sta, *next;
	unsigned long flags;

	DHD_IF_STA_LIST_LOCK(ifp, flags);

	list_for_each_entry_safe(sta, next, &ifp->sta_list, list) {
#if defined(BCM_GMAC3)
		if (ifp->fwdh) {
			/* Remove sta from WOFA forwarder. */
			fwder_deassoc(ifp->fwdh, (uint16 *)(sta->ea.octet), (wofa_t)sta);
		}
#endif /* BCM_GMAC3 */
		list_del(&sta->list);
		dhd_sta_free(&ifp->info->pub, sta);
	}

	DHD_IF_STA_LIST_UNLOCK(ifp, flags);

	return;
}

/** Router/GMAC3: Flush all station entries in the forwarder's WOFA database. */
static void
dhd_if_flush_sta(dhd_if_t * ifp)
{
#if defined(BCM_GMAC3)

	if (ifp && (ifp->fwdh != FWDER_NULL)) {
		dhd_sta_t *sta, *next;
		unsigned long flags;

		DHD_IF_STA_LIST_LOCK(ifp, flags);

		list_for_each_entry_safe(sta, next, &ifp->sta_list, list) {
			/* Remove any sta entry from WOFA forwarder. */
			fwder_flush(ifp->fwdh, (wofa_t)sta);
		}

		DHD_IF_STA_LIST_UNLOCK(ifp, flags);
	}
#endif /* BCM_GMAC3 */
}

/** Construct a pool of dhd_sta_t objects to be used by interfaces. */
static int
dhd_sta_pool_init(dhd_pub_t *dhdp, int max_sta)
{
	int idx, sta_pool_memsz;
	dhd_sta_t * sta;
	dhd_sta_pool_t * sta_pool;
	void * staid_allocator;

	ASSERT(dhdp != (dhd_pub_t *)NULL);
	ASSERT((dhdp->staid_allocator == NULL) && (dhdp->sta_pool == NULL));

	/* dhd_sta objects per radio are managed in a table. id#0 reserved. */
	staid_allocator = id16_map_init(dhdp->osh, max_sta, 1);
	if (staid_allocator == NULL) {
		DHD_ERROR(("%s: sta id allocator init failure\n", __FUNCTION__));
		return BCME_ERROR;
	}

	/* Pre allocate a pool of dhd_sta objects (one extra). */
	sta_pool_memsz = ((max_sta + 1) * sizeof(dhd_sta_t)); /* skip idx 0 */
	sta_pool = (dhd_sta_pool_t *)MALLOC(dhdp->osh, sta_pool_memsz);
	if (sta_pool == NULL) {
		DHD_ERROR(("%s: sta table alloc failure\n", __FUNCTION__));
		id16_map_fini(dhdp->osh, staid_allocator);
		return BCME_ERROR;
	}

	dhdp->sta_pool = sta_pool;
	dhdp->staid_allocator = staid_allocator;

	/* Initialize all sta(s) for the pre-allocated free pool. */
	bzero((uchar *)sta_pool, sta_pool_memsz);
	for (idx = max_sta; idx >= 1; idx--) { /* skip sta_pool[0] */
		sta = &sta_pool[idx];
		sta->idx = id16_map_alloc(staid_allocator);
		ASSERT(sta->idx <= max_sta);
	}
	/* Now place them into the pre-allocated free pool. */
	for (idx = 1; idx <= max_sta; idx++) {
		sta = &sta_pool[idx];
#ifdef BCM_PCIE_UPDATE
		int prio = 0;
		for (prio = 0; prio < (int)NUMPRIO; prio++) {
			sta->flowid[prio] = FLOWID_INVALID; /* Flow rings do not exist */
		}
#endif
		dhd_sta_free(dhdp, sta);
	}

	return BCME_OK;
}

/** Destruct the pool of dhd_sta_t objects.
 * Caller must ensure that no STA objects are currently associated with an if.
 */
static void
dhd_sta_pool_fini(dhd_pub_t *dhdp, int max_sta)
{
	dhd_sta_pool_t * sta_pool = (dhd_sta_pool_t *)dhdp->sta_pool;

	if (sta_pool) {
		int idx;
		int sta_pool_memsz = ((max_sta + 1) * sizeof(dhd_sta_t));
		for (idx = 1; idx <= max_sta; idx++) {
			ASSERT(sta_pool[idx].ifp == DHD_IF_NULL);
			ASSERT(sta_pool[idx].idx == ID16_INVALID);
		}
		MFREE(dhdp->osh, dhdp->sta_pool, sta_pool_memsz);
		dhdp->sta_pool = NULL;
	}

	id16_map_fini(dhdp->osh, dhdp->staid_allocator);
	dhdp->staid_allocator = NULL;
}



/* Clear the pool of dhd_sta_t objects for built-in type driver */
static void
dhd_sta_pool_clear(dhd_pub_t *dhdp, int max_sta)
{
	int idx, sta_pool_memsz;
	dhd_sta_t * sta;
	dhd_sta_pool_t * sta_pool;
	void *staid_allocator;

	if (!dhdp) {
		DHD_ERROR(("%s: dhdp is NULL\n", __FUNCTION__));
		return;
	}

	sta_pool = (dhd_sta_pool_t *)dhdp->sta_pool;
	staid_allocator = dhdp->staid_allocator;

	if (!sta_pool) {
		DHD_ERROR(("%s: sta_pool is NULL\n", __FUNCTION__));
		return;
	}

	if (!staid_allocator) {
		DHD_ERROR(("%s: staid_allocator is NULL\n", __FUNCTION__));
		return;
	}

	/* clear free pool */
	sta_pool_memsz = ((max_sta + 1) * sizeof(dhd_sta_t));
	bzero((uchar *)sta_pool, sta_pool_memsz);

	/* dhd_sta objects per radio are managed in a table. id#0 reserved. */
	id16_map_clear(staid_allocator, max_sta, 1);

	/* Initialize all sta(s) for the pre-allocated free pool. */
	for (idx = max_sta; idx >= 1; idx--) { /* skip sta_pool[0] */
		sta = &sta_pool[idx];
		sta->idx = id16_map_alloc(staid_allocator);
		ASSERT(sta->idx <= max_sta);
	}
	/* Now place them into the pre-allocated free pool. */
	for (idx = 1; idx <= max_sta; idx++) {
		sta = &sta_pool[idx];
#ifdef BCM_PCIE_UPDATE
		int prio = 0;
		for (prio = 0; prio < (int)NUMPRIO; prio++) {
			sta->flowid[prio] = FLOWID_INVALID; /* Flow rings do not exist */
		}
#endif
		dhd_sta_free(dhdp, sta);
	}
}


/** Find STA with MAC address ea in an interface's STA list. */
dhd_sta_t *
dhd_find_sta(void *pub, int ifidx, void *ea)
{
	dhd_sta_t *sta, *next;
	dhd_if_t *ifp;
	unsigned long flags;

	ASSERT(ea != NULL);
	ifp = dhd_get_ifp((dhd_pub_t *)pub, ifidx);
	if (ifp == NULL)
		return DHD_STA_NULL;

	DHD_IF_STA_LIST_LOCK(ifp, flags);

	list_for_each_entry_safe(sta, next, &ifp->sta_list, list) {
		if (!memcmp(sta->ea.octet, ea, ETHER_ADDR_LEN)) {
			DHD_IF_STA_LIST_UNLOCK(ifp, flags);
			return sta;
		}
	}

	DHD_IF_STA_LIST_UNLOCK(ifp, flags);

	return DHD_STA_NULL;
}

/** Add STA into the interface's STA list. */
dhd_sta_t *
dhd_add_sta(void *pub, int ifidx, void *ea)
{
	dhd_sta_t *sta;
	dhd_if_t *ifp;
	unsigned long flags;

	ASSERT(ea != NULL);
	ifp = dhd_get_ifp((dhd_pub_t *)pub, ifidx);
	if (ifp == NULL)
		return DHD_STA_NULL;

	sta = dhd_sta_alloc((dhd_pub_t *)pub);
	if (sta == DHD_STA_NULL) {
		DHD_ERROR(("%s: Alloc failed\n", __FUNCTION__));
		return DHD_STA_NULL;
	}

	memcpy(sta->ea.octet, ea, ETHER_ADDR_LEN);

	/* link the sta and the dhd interface */
	sta->ifp = ifp;
	sta->ifidx = ifidx;
	INIT_LIST_HEAD(&sta->list);

	DHD_IF_STA_LIST_LOCK(ifp, flags);

	list_add_tail(&sta->list, &ifp->sta_list);

#if defined(BCM_GMAC3)
	if (ifp->fwdh) {
		ASSERT(ISALIGNED(ea, 2));
		/* Add sta to WOFA forwarder. */
		fwder_reassoc(ifp->fwdh, (uint16 *)ea, (wofa_t)sta);
	}
#endif /* BCM_GMAC3 */

	DHD_IF_STA_LIST_UNLOCK(ifp, flags);

	return sta;
}

/** Delete STA from the interface's STA list. */
void
dhd_del_sta(void *pub, int ifidx, void *ea)
{
	dhd_sta_t *sta, *next;
	dhd_if_t *ifp;
	unsigned long flags;

	ASSERT(ea != NULL);
	ifp = dhd_get_ifp((dhd_pub_t *)pub, ifidx);
	if (ifp == NULL)
		return;

	DHD_IF_STA_LIST_LOCK(ifp, flags);

	list_for_each_entry_safe(sta, next, &ifp->sta_list, list) {
		if (!memcmp(sta->ea.octet, ea, ETHER_ADDR_LEN)) {
#if defined(BCM_GMAC3)
			if (ifp->fwdh) { /* Found a sta, remove from WOFA forwarder. */
				ASSERT(ISALIGNED(ea, 2));
				fwder_deassoc(ifp->fwdh, (uint16 *)ea, (wofa_t)sta);
			}
#endif /* BCM_GMAC3 */
			list_del(&sta->list);
			dhd_sta_free(&ifp->info->pub, sta);
		}
	}

	DHD_IF_STA_LIST_UNLOCK(ifp, flags);

	return;
}

/** Add STA if it doesn't exist. Not reentrant. */
dhd_sta_t*
dhd_findadd_sta(void *pub, int ifidx, void *ea)
{
	dhd_sta_t *sta;

	sta = dhd_find_sta(pub, ifidx, ea);

	if (!sta) {
		/* Add entry */
		sta = dhd_add_sta(pub, ifidx, ea);
	}

	return sta;
}
#else
static inline void dhd_if_flush_sta(dhd_if_t * ifp) { }
static inline void dhd_if_del_sta_list(dhd_if_t *ifp) {}
static inline int dhd_sta_pool_init(dhd_pub_t *dhdp, int max_sta) { return BCME_OK; }
static inline void dhd_sta_pool_fini(dhd_pub_t *dhdp, int max_sta) {}
dhd_sta_t *dhd_findadd_sta(void *pub, int ifidx, void *ea) { return NULL; }
void dhd_del_sta(void *pub, int ifidx, void *ea) {}
#endif /* PCIE_FULL_DONGLE */


/* Returns dhd iflist index correspondig the the bssidx provided by apps */
int dhd_bssidx2idx(dhd_pub_t *dhdp, uint32 bssidx)
{
	dhd_if_t *ifp;
	int i;

	ASSERT(bssidx < DHD_MAX_IFS);
	ASSERT(dhdp);

	dhd_info_t *dhd = dhdp->info;

	for (i = 0; i < DHD_MAX_IFS; i++) {
		ifp = dhd->iflist[i];
		if (ifp && (ifp->bssidx == bssidx)) {
			DHD_TRACE(("Index manipulated for %s from %d to %d\n",
				ifp->name, bssidx, i));
			break;
		}
	}
	return i;
}

static inline int dhd_rxf_enqueue(dhd_pub_t *dhdp, void* skb)
{
	uint32 store_idx;
	uint32 sent_idx;

	if (!skb) {
		DHD_ERROR(("dhd_rxf_enqueue: NULL skb!!!\n"));
		return BCME_ERROR;
	}

	dhd_os_rxflock(dhdp);
	store_idx = dhdp->store_idx;
	sent_idx = dhdp->sent_idx;
	if (dhdp->skbbuf[store_idx] != NULL) {
		/* Make sure the previous packets are processed */
		dhd_os_rxfunlock(dhdp);
#ifdef RXF_DEQUEUE_ON_BUSY
		DHD_TRACE(("dhd_rxf_enqueue: pktbuf not consumed %p, store idx %d sent idx %d\n",
			skb, store_idx, sent_idx));
		return BCME_BUSY;
#else /* RXF_DEQUEUE_ON_BUSY */
		DHD_ERROR(("dhd_rxf_enqueue: pktbuf not consumed %p, store idx %d sent idx %d\n",
			skb, store_idx, sent_idx));
		/* removed msleep here, should use wait_event_timeout if we
		 * want to give rx frame thread a chance to run
		 */
#if defined(WAIT_DEQUEUE)
		OSL_SLEEP(1);
#endif
		return BCME_ERROR;
#endif /* RXF_DEQUEUE_ON_BUSY */
	}
	DHD_TRACE(("dhd_rxf_enqueue: Store SKB %p. idx %d -> %d\n",
		skb, store_idx, (store_idx + 1) & (MAXSKBPEND - 1)));
	dhdp->skbbuf[store_idx] = skb;
	dhdp->store_idx = (store_idx + 1) & (MAXSKBPEND - 1);
	dhd_os_rxfunlock(dhdp);

	return BCME_OK;
}

static inline void* dhd_rxf_dequeue(dhd_pub_t *dhdp)
{
	uint32 store_idx;
	uint32 sent_idx;
	void *skb;

	dhd_os_rxflock(dhdp);

	store_idx = dhdp->store_idx;
	sent_idx = dhdp->sent_idx;
	skb = dhdp->skbbuf[sent_idx];

	if (skb == NULL) {
		dhd_os_rxfunlock(dhdp);
		DHD_ERROR(("dhd_rxf_dequeue: Dequeued packet is NULL, store idx %d sent idx %d\n",
			store_idx, sent_idx));
		return NULL;
	}

	dhdp->skbbuf[sent_idx] = NULL;
	dhdp->sent_idx = (sent_idx + 1) & (MAXSKBPEND - 1);

	DHD_TRACE(("dhd_rxf_dequeue: netif_rx_ni(%p), sent idx %d\n",
		skb, sent_idx));

	dhd_os_rxfunlock(dhdp);

	return skb;
}

int dhd_process_cid_mac(dhd_pub_t *dhdp, bool prepost)
{
	dhd_info_t *dhd = (dhd_info_t *)dhdp->info;

	if (prepost) { /* pre process */
		dhd_read_macaddr(dhd);
	} else { /* post process */
		dhd_write_macaddr(&dhd->pub.mac);
	}

	return 0;
}

#if defined(PKT_FILTER_SUPPORT) && !defined(GAN_LITE_NAT_KEEPALIVE_FILTER)
static bool
_turn_on_arp_filter(dhd_pub_t *dhd, int op_mode)
{
	bool _apply = FALSE;
	/* In case of IBSS mode, apply arp pkt filter */
	if (op_mode & DHD_FLAG_IBSS_MODE) {
		_apply = TRUE;
		goto exit;
	}
	/* In case of P2P GO or GC, apply pkt filter to pass arp pkt to host */
	if ((dhd->arp_version == 1) &&
		(op_mode & (DHD_FLAG_P2P_GC_MODE | DHD_FLAG_P2P_GO_MODE))) {
		_apply = TRUE;
		goto exit;
	}

exit:
	return _apply;
}
#endif /* PKT_FILTER_SUPPORT && !GAN_LITE_NAT_KEEPALIVE_FILTER */

void dhd_set_packet_filter(dhd_pub_t *dhd)
{
#ifdef PKT_FILTER_SUPPORT
	int i;

	DHD_TRACE(("%s: enter\n", __FUNCTION__));
	if (dhd_pkt_filter_enable) {
		for (i = 0; i < dhd->pktfilter_count; i++) {
			dhd_pktfilter_offload_set(dhd, dhd->pktfilter[i]);
		}
	}
#endif /* PKT_FILTER_SUPPORT */
}

void dhd_enable_packet_filter(int value, dhd_pub_t *dhd)
{
#ifdef PKT_FILTER_SUPPORT
	int i;

	DHD_TRACE(("%s: enter, value = %d\n", __FUNCTION__, value));
	/* 1 - Enable packet filter, only allow unicast packet to send up */
	/* 0 - Disable packet filter */
	if (dhd_pkt_filter_enable && (!value ||
	    (dhd_support_sta_mode(dhd) && !dhd->dhcp_in_progress)))
	    {
		for (i = 0; i < dhd->pktfilter_count; i++) {
#ifndef GAN_LITE_NAT_KEEPALIVE_FILTER
			if (value && (i == DHD_ARP_FILTER_NUM) &&
				!_turn_on_arp_filter(dhd, dhd->op_mode)) {
				DHD_TRACE(("Do not turn on ARP white list pkt filter:"
					"val %d, cnt %d, op_mode 0x%x\n",
					value, i, dhd->op_mode));
				continue;
			}
#endif /* !GAN_LITE_NAT_KEEPALIVE_FILTER */
			dhd_pktfilter_offload_enable(dhd, dhd->pktfilter[i],
				value, dhd_master_mode);
		}
#ifdef HW_FILTER_MDNS
		dhd_iovar(dhd, 0, "pkt_filter_mdns", (char *)&value, sizeof(value), NULL, 0,TRUE);
#endif /* PKT_FILTER_SUPPORT */
#ifdef HW_DOZE_PKT_FILTER
		if(!value) {
			hw_clear_filters();
		}
#endif /* HW_DOZE_PKT_FILTER */
	}
#endif /* PKT_FILTER_SUPPORT */
}
#ifdef DHD_TPUT_MONITOR
#ifdef HW_TPUT_CPU
#define TPUT_COUNT_HIGH 4
#define TPUT_COUNT_LOW 2
void dhd_irq_affinity_set(dhd_if_t *ifp, ulong tx_speed, ulong rx_speed,
int *count, int *flag_high, int *flag_low)
{
	if(ifp == NULL || count == NULL || flag_high == NULL || flag_low == NULL)
		return;

	/* Above DHD_TPUT_THRESHOLD(200Mbps) is large cpu */
	if (tx_speed >= DHD_TPUT_THRESHOLD_HIGH ||
		rx_speed >= DHD_TPUT_THRESHOLD_HIGH) {
		(*count)++;
		if (*count == TPUT_COUNT_HIGH && !(*flag_high)) {
			DHD_ERROR(("%s: affinity cpu%d\n", __FUNCTION__,DPC_LARGE_CPUCORE));
			dhd_irq_affinity_enable(ifp->net, DPC_LARGE_CPUCORE);
			*flag_high = TRUE;
			*flag_low = FALSE;
		}
		if (*count > TPUT_COUNT_HIGH) *count = TPUT_COUNT_HIGH;

	/* above DHD_TPUT_THRESHOLD_LOW(20Mbps) is small cpu(cpu1)
	 * count = TPUT_COUNT_LOW is speedtest from 0 to DHD_TPUT_THRESHOLD_LOW
	 * flag_high is speedtest from DHD_TPUT_THRESHOLD to DHD_TPUT_THRESHOLD_LOW
	 */
	} else if (tx_speed >= DHD_TPUT_THRESHOLD_LOW ||
		rx_speed >= DHD_TPUT_THRESHOLD_LOW) {
		(*count)++;
		if ((*count == TPUT_COUNT_LOW || *flag_high) && !(*flag_low)) {
			DHD_ERROR(("%s: affinity cpu%d\n", __FUNCTION__,DPC_SMALL_CPUCORE));
			dhd_irq_affinity_enable(ifp->net, DPC_SMALL_CPUCORE);
			*flag_low = TRUE;
			*flag_high = FALSE;
		}
		if (*count > TPUT_COUNT_LOW) *count = TPUT_COUNT_LOW;
	} else {
		(*count)--;
		if (*count == 0 && (*flag_low || *flag_high)) {
			DHD_ERROR(("%s: affinity PRIMARY_CPUCORE\n", __FUNCTION__));
			dhd_irq_affinity_enable(ifp->net, PRIMARY_CPUCORE);
			*flag_high = FALSE;
			*flag_low = FALSE;
		}
		if (*count < 0) *count = 0;
	}
}
#endif
static int
dhd_tput_monitor_thread(void *data)
{
	tsk_ctl_t *tsk = (tsk_ctl_t *)data;
	dhd_info_t *dhd = (dhd_info_t *)tsk->parent;
	dhd_if_t *ifp = dhd->iflist[0];
	ulong tx_bytes = 0;
	ulong rx_bytes = 0;
	ulong pre_jiffies = 0;
	ulong passed_msec = 0;
	ulong pre_tx_bytes = 0;
	ulong pre_rx_bytes = 0;
	int count = 0;
#ifndef HW_TPUT_CPU
	bool flag = FALSE;
#else
	int flag_high = FALSE;
	int flag_low = FALSE;
	ulong tx_speed = 0;
	ulong rx_speed = 0;
#endif
	DAEMONIZE("dhd_tput_monitor");

	complete(&tsk->completed);
	while (1) {
		if (down_interruptible(&tsk->sema) == 0) {
			SMP_RD_BARRIER_DEPENDS();
			if (tsk->terminated)
				break;
			passed_msec = jiffies_to_msecs(jiffies - pre_jiffies);
			if (ifp && (passed_msec >= DHD_TPUT_CHECK_INTERVAL * 1000)) {
				pre_jiffies = jiffies;
				tx_bytes = ifp->stats.tx_bytes;
				rx_bytes = ifp->stats.rx_bytes;
#ifndef HW_TPUT_CPU
				DHD_TRACE(("%s: check, tx %lu, rx %lu, time %lu threshold=%lu\n",
					__FUNCTION__,
					tx_bytes - pre_tx_bytes, rx_bytes - pre_rx_bytes,
					passed_msec,
					(ulong)DHD_TPUT_THRESHOLD * passed_msec / 1000));
				if ((tx_bytes - pre_tx_bytes) >=
					((ulong)DHD_TPUT_THRESHOLD * passed_msec / 1000) ||
				    (rx_bytes - pre_rx_bytes) >=
					((ulong)DHD_TPUT_THRESHOLD * passed_msec / 1000)) {
					count++;
					if (count == 2 && !flag) {
						DHD_ERROR(("%s: affinity enable\n", __FUNCTION__));
						dhd_irq_affinity_enable(ifp->net, true);
						flag = TRUE;
					}
					if (count > 2) count = 2;
				} else {
					count--;
					if (count == 0 && flag) {
						DHD_ERROR(("%s: affinity disable\n", __FUNCTION__));
						dhd_irq_affinity_enable(ifp->net, false);
						flag = FALSE;
					}
					if (count < 0) count = 0;
				}
#else
				tx_speed = (tx_bytes - pre_tx_bytes)*1000/passed_msec;
				rx_speed = (rx_bytes - pre_rx_bytes)*1000/passed_msec;

				DHD_TRACE(("%s: check, tx %lu, rx %lu, time %lu, threshold %lu ~ %lu\n",__FUNCTION__,
					tx_speed, rx_speed, passed_msec, DHD_TPUT_THRESHOLD_LOW, DHD_TPUT_THRESHOLD_HIGH));
				//set cpu
				dhd_irq_affinity_set(ifp, tx_speed, rx_speed, &count , &flag_high, &flag_low);
#endif
				pre_tx_bytes = tx_bytes;
				pre_rx_bytes = rx_bytes;
			}
#ifndef HW_TPUT_CPU
			if (flag)
#else
			if (flag_high || flag_low)
#endif
				mod_timer(&dhd->tput_timer,
					jiffies + msecs_to_jiffies(DHD_TPUT_CHECK_INTERVAL * 1000));
		} else {
			break;
		}
	}
	complete_and_exit(&tsk->completed, 0);
}

void
dhd_tput_monitor_func(ulong data)
{
	dhd_info_t *dhd = (dhd_info_t *)data;
	if (dhd && dhd->tput_tsk.thr_pid >= 0) {
		if (time_after_eq(jiffies,
			dhd->last_jiffies + msecs_to_jiffies(DHD_TPUT_CHECK_INTERVAL * 1000)) ||
			time_after(dhd->last_jiffies, jiffies)) {
			dhd->last_jiffies = jiffies;
			up(&dhd->tput_tsk.sema);
		}
	}
	return;
}

static void
dhd_tput_monitor_init(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd = (dhd_info_t *)dhdp->info;

	//init values
	DHD_TRACE(("%s: tput_monitor init\n", __FUNCTION__));
	mod_timer(&dhd->tput_timer, jiffies + msecs_to_jiffies(DHD_TPUT_CHECK_INTERVAL * 1000));
}

static void
dhd_tput_monitor_release(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd = (dhd_info_t *)dhdp->info;
	DHD_TRACE(("%s: tput_monitor release\n", __FUNCTION__));
	del_timer_sync(&dhd->tput_timer);
}
#endif /* DHD_TPUT_MONITOR */
#ifdef DHD_DEVWAKE_EARLY
#define DEVWAKE_BUS_BUSY_MASK (DHD_BUS_BUSY_IN_TX | DHD_BUS_BUSY_IN_DPC |\
	DHD_BUS_BUSY_IN_WD | DHD_BUS_BUSY_IN_IOVAR)

extern int dhd_wlan_dev_wake(int on);
static void
dhd_devwake_thread(void *data)
{
	tsk_ctl_t *tsk = (tsk_ctl_t *)data;
	dhd_info_t *dhd = (dhd_info_t *)tsk->parent;
	int timeleft;
	unsigned long flags;

	DAEMONIZE("dhd_devwake");

	complete(&tsk->completed);
	while (1) {
		if (!binary_sema_down(tsk)) {
			SMP_RD_BARRIER_DEPENDS();
			if (tsk->terminated)
				break;
			/* sync state */
			if (!dhd->pub.devwake_enable)
				continue;

			/* set running as true */
			spin_lock_irqsave(&tsk->spinlock, flags);
			tsk->is_running = true;
			spin_unlock_irqrestore(&tsk->spinlock, flags);

			DHD_TRACE(("%s: deepsleep=%d\n", __FUNCTION__, dhd->pub.deepsleep));
			if (dhd->pub.deepsleep) {
				DHD_TRACE(("%s: disable deep-sleep\n", __FUNCTION__));
				/* wake up */
				dhd_wlan_dev_wake(1);
				/* wait until deepsleep become 0 */
				timeleft = dhd_os_devwake_wait(&(dhd->pub), &(dhd->pub.deepsleep));
				if (timeleft == 0) {
					dhd->pub.deepsleep = 0;
					dhd->pub.devwake_err++;
					DHD_ERROR(("%s: dev wake timeout, set deepsleep 0 and devwake_err is %d\n", __FUNCTION__, dhd->pub.devwake_err));
					if (dhd->pub.devwake_err > 3) {
#ifdef HW_WIFI_DMD_LOG
						hw_wifi_dsm_client_notify(DSM_WIFI_SET_CIPHER_ERROR, "%s: deep-sleep function disable\n", __FUNCTION__);
#endif
						/* force to disable devwake */
						DHD_ERROR(("%s: err > 3, disable devwake.\n", __FUNCTION__));
						dhd->pub.devwake_enable = 0;
						dhd->pub.devwake_err = 0;
						/* deassert dev wake pin. */
						dhd_wlan_dev_wake(0);
						dhd_net_if_lock_local(dhd);
						dhd_bus_start_queue(dhd->pub.bus);
						dhd_net_if_unlock_local(dhd);
						/* send hang to remove nv item */
						dhd->pub.devwake_timeout = 1;
						dhd_os_send_hang_message(&dhd->pub);
						break;
					}
				} else {
					/* reset error count */
					dhd->pub.devwake_err = 0;
				}
			}
			/* sync state */
			if (dhd->pub.devwake_enable) {
				dhd_net_if_lock_local(dhd);
				dhd_bus_start_queue(dhd->pub.bus);
				dhd_net_if_unlock_local(dhd);
			} else {
				DHD_ERROR(("%s: devwake disabled. Don't start queue again.\n", __FUNCTION__));
			}

			if (!wait_event_interruptible(dhd->dhd_bus_busy_state_wait,
				!(dhd->pub.dhd_bus_busy_state & DEVWAKE_BUS_BUSY_MASK))) {
				DHD_TRACE(("%s: dhd_bus_busy_state=%x, deepsleep=%d\n", __FUNCTION__,
					dhd->pub.dhd_bus_busy_state, dhd->pub.deepsleep));
				/* release dev wake */
				if (!dhd->pub.deepsleep)
					dhd_wlan_dev_wake(0);
			} else {
				DHD_ERROR(("%s: interrupt by signal\n", __FUNCTION__));
			}

			/* set running as false */
			spin_lock_irqsave(&tsk->spinlock, flags);
			tsk->is_running = false;
			spin_unlock_irqrestore(&tsk->spinlock, flags);
		} else {
			DHD_ERROR(("%s: down sleep is interrupted. Thread is going to end!\n", __FUNCTION__));
			break;
		}
	}
	DHD_ERROR(("%s: Thread is end! Disable devwake.\n", __FUNCTION__));
	dhd->pub.devwake_enable = FALSE;
	smp_mb();
	complete_and_exit(&tsk->completed, 0);
}

static void
dhd_devwake_init(dhd_pub_t *dhdp)
{
	DHD_TRACE(("%s: enter\n", __FUNCTION__));
	dhd_wlan_dev_wake(0);
	dhdp->deepsleep = 1;
	dhdp->devwake_err = 0;
	/* disable devwake in default */
	dhdp->devwake_enable = FALSE;
}

void
dhd_devwake_release(dhd_pub_t *dhdp)
{
	unsigned long flags;

	DHD_ERROR(("%s: deepsleep=%d\n", __FUNCTION__, dhdp->deepsleep));
	dhdp->devwake_enable = FALSE;
	smp_mb();
	dhdp->devwake_err = 0;
	DHD_GENERAL_LOCK(dhdp, flags);
	if (dhdp->dhd_bus_busy_state & DHD_BUS_BUSY_IN_TX) {
		/* We have added DHD_BUS_BUSY_IN_TX for waiting deepsleep disabled.
		   Since we abort the devwake process here, we have to clean
                   DHD_BUS_BUSY_IN_TX here also. It is ok because the TX packet has
		   not been really sent. */
		DHD_ERROR(("%s: Tx abort, clean DHD_BUS_BUSY_IN_TX\n", __FUNCTION__));
		dhdp->dhd_bus_busy_state &= ~DHD_BUS_BUSY_IN_TX;
		/* Reset for next wifi on */
		//dhd_bus_start_queue(dhdp->bus);
	}
	DHD_GENERAL_UNLOCK(dhdp, flags);

}
#endif /* DHD_DEVWAKE_EARLY */

static int
dhd_set_prune_event_mask(int value, dhd_pub_t *dhd)
{
	int ret = 0;
	char eventmask[WL_EVENTING_MASK_LEN];
	char iovbuf[WL_EVENTING_MASK_LEN + 12]; /*  Room for "event_msgs" + '\0' + bitvec  */

	/* Read event_msgs mask */
	bcm_mkiovar("event_msgs", eventmask, WL_EVENTING_MASK_LEN, iovbuf, sizeof(iovbuf));
	if ((ret  = dhd_wl_ioctl_cmd(dhd, WLC_GET_VAR, iovbuf, sizeof(iovbuf), FALSE, 0)) < 0) {
			DHD_ERROR(("%s read Event mask failed %d\n", __FUNCTION__, ret));
			goto done;
	}
	bcopy(iovbuf, eventmask, WL_EVENTING_MASK_LEN);
	DHD_ERROR(("%s prune set bit %d \n", __FUNCTION__, value));
	if(value) {
		setbit(eventmask, WLC_E_PRUNE);
	}else {
		clrbit(eventmask, WLC_E_PRUNE);
	}
	/* Write updated Event mask */
	bcm_mkiovar("event_msgs", eventmask, WL_EVENTING_MASK_LEN, iovbuf, sizeof(iovbuf));
	if ((ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0)) < 0) {
			DHD_ERROR(("%s Set Event mask failed %d\n", __FUNCTION__, ret));
			goto done;
	}
done:
	return ret;
}



#ifdef HW_READ_FW_LOG
extern void dhd_read_console_ex(dhd_pub_t *dhd);
#endif
#ifdef HW_CHINAMOBILE_SPEEDLIMIT
unsigned long recv_pkt = 0;
static int early_suspended = 0;
static int pm2_sleep_ret = 0;
#endif
#ifdef HW_SHARE_WIFI_FILTER_MANAGE
extern bool g_force_stop_filter;
#endif /* HW_SHARE_WIFI_FILTER_MANAGE */
static int dhd_set_suspend(int value, dhd_pub_t *dhd)
{
#ifndef SUPPORT_PM2_ONLY
	int power_mode = PM_MAX;
#endif /* SUPPORT_PM2_ONLY */
	/* wl_pkt_filter_enable_t	enable_parm; */
	int bcn_li_dtim = 0; /* Default bcn_li_dtim in resume mode is 0 */
#ifndef ENABLE_FW_ROAM_SUSPEND
	uint roamvar = 1;
#endif /* ENABLE_FW_ROAM_SUSPEND */
	uint nd_ra_filter = 0;
	int lpas = 0;
	int dtim_period = 0;
	int bcn_interval = 0;
	int bcn_to_dly = 0;
	int bcn_timeout = CUSTOM_BCN_TIMEOUT_SETTING;
	int ret = 0;
#ifdef BCM_PATCH_GC_WAKE_BY_NOA
	int host_suspend = 0;
#endif

	if (!dhd)
		return -ENODEV;

	DHD_TRACE(("%s: enter, value = %d in_suspend=%d\n",
		__FUNCTION__, value, dhd->in_suspend));

	dhd_suspend_lock(dhd);

#ifdef CUSTOM_SET_CPUCORE
	DHD_TRACE(("%s set cpucore(suspend%d)\n", __FUNCTION__, value));
	/* set specific cpucore */
	dhd_set_cpucore(dhd, TRUE);
#endif /* CUSTOM_SET_CPUCORE */
#ifdef HW_CUSTOM_BCN_TIMEOUT
	bcn_timeout = wifi_platform_get_bcn_timeout(dhd->info->adapter, bcn_timeout);
	DHD_TRACE(("%s: bcn_timeout: %d \n", __FUNCTION__, bcn_timeout));
#endif /* HW_CUSTOM_BCN_TIMEOUT */
	if (dhd->up) {
		if (value && dhd->in_suspend) {
#ifdef PKT_FILTER_SUPPORT
				dhd->early_suspended = 1;
#endif
				dhd_set_prune_event_mask(0, dhd);
				/* Kernel suspended */
				DHD_ERROR(("%s: force extra Suspend setting \n", __FUNCTION__));
#ifdef HW_WATCHDOG_MS
				dhd_os_wd_timer(dhd, 0);
#endif
#ifdef CUSTOM_SET_SHORT_DWELL_TIME
				dhd_set_short_dwell_time(dhd, TRUE);
#endif
#ifndef SUPPORT_PM2_ONLY
				dhd_wl_ioctl_cmd(dhd, WLC_SET_PM, (char *)&power_mode,
				                 sizeof(power_mode), TRUE, 0);
#endif /* SUPPORT_PM2_ONLY */
#ifdef HW_CHINAMOBILE_SPEEDLIMIT
				//screen off
				early_suspended = 1;
				pm2_sleep_ret = 10;
				dhd_iovar(dhd, 0, "pm2_sleep_ret", (char *)&pm2_sleep_ret, sizeof(pm2_sleep_ret), NULL, 0, TRUE);
#endif

				/* Enable packet filter, only allow unicast packet to send up */
#ifdef HW_SHARE_WIFI_FILTER_MANAGE
				if (false == g_force_stop_filter)
					dhd_enable_packet_filter(1, dhd);
#else
				dhd_enable_packet_filter(1, dhd);
#endif /* HW_SHARE_WIFI_FILTER_MANAGE */
#ifdef BCM_APF
				DHD_ERROR(("%s: dhd_dev_apf_enable_filter \n", __FUNCTION__));
				dhd_dev_apf_enable_filter(dhd_linux_get_primary_netdev(dhd));
#endif /* BCM_APF */

				/* If DTIM skip is set up as default, force it to wake
				 * each third DTIM for better power savings.  Note that
				 * one side effect is a chance to miss BC/MC packet.
				 */
				bcn_li_dtim = dhd_get_suspend_bcn_li_dtim(dhd, &dtim_period, &bcn_interval);
				dhd_iovar(dhd, 0, "bcn_li_dtim",
					  (char *)&bcn_li_dtim,
					  sizeof(bcn_li_dtim),
					  NULL, 0, TRUE);
				if (bcn_li_dtim * dtim_period * bcn_interval >= MIN_DTIM_FOR_ROAM_THRES_EXTEND) {
					/*
					* Increase max roaming threshold from 2 secs to 8 secs
					* the real roam threshold is MIN(max_roam_threshold, bcn_timeout/2)
					* In 4359 chip max roaming threshold is min(8s, bcn_timeout-2)
					*/
#if defined(BCMSDIO)
					lpas = 1;
					dhd_iovar(dhd, 0, "lpas", (char *)&lpas,
						  sizeof(lpas), NULL, 0, TRUE);

					bcn_to_dly = 1;
					/*
					* if bcn_to_dly is 1, the real roam threshold is MIN(max_roam_threshold, bcn_timeout -1);
					* notify link down event after roaming procedure complete if we hit bcn_timeout
					* while we are in roaming progress.
					*
					*/
					dhd_iovar(dhd, 0, "bcn_to_dly",
						  (char *)&bcn_to_dly,
						  sizeof(bcn_to_dly), NULL, 0,
						  TRUE);
#endif
					/* Increase beacon timeout to 6 secs */
					bcn_timeout = (bcn_timeout < BCN_TIMEOUT_IN_SUSPEND) ?
										BCN_TIMEOUT_IN_SUSPEND : bcn_timeout;
					dhd_iovar(dhd, 0, "bcn_timeout",
						  (char *)&bcn_timeout,
						  sizeof(bcn_timeout),
						  NULL, 0, TRUE);
				}


#ifndef ENABLE_FW_ROAM_SUSPEND
				/* Disable firmware roaming during suspend */
				dhd_iovar(dhd, 0, "roam_off", (char *)&roamvar,
					  sizeof(roamvar), NULL, 0, TRUE);

#endif /* ENABLE_FW_ROAM_SUSPEND */
				if (FW_SUPPORTED(dhd, ndoe)) {
					/* enable IPv6 RA filter in  firmware during suspend */
					nd_ra_filter = 1;
					ret = dhd_iovar(dhd, 0,
							"nd_ra_filter_enable",
							(char *)&nd_ra_filter,
							sizeof(nd_ra_filter),
							NULL, 0, TRUE);
					if (ret < 0)
						DHD_ERROR(("nd_ra_filter :%d\n",
							   ret));
				}
				dhd_dev_get_drop_pkt(dhd, 0);
				dhd_os_suppress_logging(dhd, TRUE);
#ifdef BCM_PATCH_GC_WAKE_BY_NOA
				host_suspend = 1;
				dhd_iovar(dhd,0,"host_suspend", (char *) &host_suspend, sizeof(host_suspend), NULL, 0, TRUE);
#endif
			} else {
#ifdef HW_READ_FW_LOG
				dhd_read_console_ex(dhd);
#endif
#ifdef PKT_FILTER_SUPPORT
				dhd->early_suspended = 0;
#endif
				/* Kernel resumed  */
				DHD_ERROR(("%s: Remove extra suspend setting \n", __FUNCTION__));
				dhd_dev_get_drop_pkt(dhd, 1);
#ifdef HW_WATCHDOG_MS
				dhd_os_wd_timer(dhd, dhd_watchdog_ms);
#endif
#ifdef CUSTOM_SET_SHORT_DWELL_TIME
				dhd_set_short_dwell_time(dhd, FALSE);
#endif
#ifndef SUPPORT_PM2_ONLY
				power_mode = PM_FAST;
				dhd_wl_ioctl_cmd(dhd, WLC_SET_PM, (char *)&power_mode,
				                 sizeof(power_mode), TRUE, 0);
#endif /* SUPPORT_PM2_ONLY */
#ifdef HW_CHINAMOBILE_SPEEDLIMIT
				//screen on
				early_suspended = 0;
				pm2_sleep_ret = 200;
				dhd_iovar(dhd, 0, "pm2_sleep_ret", (char *)&pm2_sleep_ret, sizeof(pm2_sleep_ret), NULL, 0, TRUE);
#endif
#ifdef PKT_FILTER_SUPPORT
				/* disable pkt filter */
				dhd_enable_packet_filter(0, dhd);
#ifdef BCM_APF
				DHD_ERROR(("%s: dhd_dev_apf_disable_filter \n", __FUNCTION__));
				dhd_dev_apf_disable_filter(dhd_linux_get_primary_netdev(dhd));
#endif /* BCM_APF */
#endif /* PKT_FILTER_SUPPORT */

				/* restore pre-suspend setting */
				dhd_iovar(dhd, 0, "bcn_li_dtim",
					  (char *)&bcn_li_dtim,
					  sizeof(bcn_li_dtim), NULL, 0, TRUE);
#if defined(BCMSDIO)
				dhd_iovar(dhd, 0, "lpas", (char *)&lpas,
					  sizeof(lpas), NULL, 0, TRUE);
				dhd_iovar(dhd, 0, "bcn_to_dly",
					  (char *)&bcn_to_dly,
					  sizeof(bcn_to_dly), NULL, 0, TRUE);
#endif
				dhd_iovar(dhd, 0, "bcn_timeout",
					  (char *)&bcn_timeout,
					  sizeof(bcn_timeout), NULL, 0, TRUE);

#ifndef ENABLE_FW_ROAM_SUSPEND
				roamvar = dhd_roam_disable;
				dhd_iovar(dhd, 0, "roam_off",
					  (char *)&roamvar, sizeof(roamvar),
					  NULL, 0, TRUE);
#endif /* ENABLE_FW_ROAM_SUSPEND */
				if (FW_SUPPORTED(dhd, ndoe)) {
					/* disable IPv6 RA filter in  firmware during suspend */
					nd_ra_filter = 0;
					ret = dhd_iovar(dhd, 0,
							"nd_ra_filter_enable",
							(char *)&nd_ra_filter,
							sizeof(nd_ra_filter),
							NULL, 0, TRUE);
					if (ret < 0)
						DHD_ERROR(("nd_ra_filter: %d\n",
							   ret));
				}
				dhd_os_suppress_logging(dhd, FALSE);
#ifdef BCM_PATCH_GC_WAKE_BY_NOA
				host_suspend = 0;
				dhd_iovar(dhd,0,"host_suspend", (char *) &host_suspend, sizeof(host_suspend), NULL, 0, TRUE);
#endif
				dhd_set_prune_event_mask(1, dhd);
			}
	}
	dhd_suspend_unlock(dhd);

	return 0;
}

static int dhd_suspend_resume_helper(struct dhd_info *dhd, int val, int force)
{
	dhd_pub_t *dhdp = &dhd->pub;
	int ret = 0;

	DHD_OS_WAKE_LOCK(dhdp);
	DHD_PERIM_LOCK(dhdp);

	/* Set flag when early suspend was called */
	dhdp->in_suspend = val;
	if ((force || !dhdp->suspend_disable_flag) &&
		dhd_support_sta_mode(dhdp))
	{
		ret = dhd_set_suspend(val, dhdp);
	}

	DHD_PERIM_UNLOCK(dhdp);
	DHD_OS_WAKE_UNLOCK(dhdp);
	return ret;
}

#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(DHD_USE_EARLYSUSPEND)
static void dhd_early_suspend(struct early_suspend *h)
{
	struct dhd_info *dhd = container_of(h, struct dhd_info, early_suspend);
	DHD_TRACE_HW4(("%s: enter\n", __FUNCTION__));

	if (dhd)
		dhd_suspend_resume_helper(dhd, 1, 0);
}

static void dhd_late_resume(struct early_suspend *h)
{
	struct dhd_info *dhd = container_of(h, struct dhd_info, early_suspend);
	DHD_TRACE_HW4(("%s: enter\n", __FUNCTION__));

	if (dhd)
		dhd_suspend_resume_helper(dhd, 0, 0);
}
#endif /* CONFIG_HAS_EARLYSUSPEND && DHD_USE_EARLYSUSPEND */

#if !defined(CONFIG_HAS_EARLYSUSPEND) && defined(HW_AP_POWERSAVE)
static int dhd_fb_notify(struct notifier_block *nb, unsigned long action, void *data) {
	int                 *blank = NULL;
	struct fb_event     *event = data;
	struct dhd_info     *dhd = NULL;

	if (NULL == nb || NULL == event || FB_EVENT_BLANK != action) {
		return NOTIFY_OK;
	}
	dhd = container_of(nb, struct dhd_info, fb_notifier);
	blank = event->data;

	if (NULL != dhd && NULL != blank) {
		hw_dhd_fb_notify(*blank, action, &dhd->pub);
	}

	return NOTIFY_OK;
}
#endif

/*
 * Generalized timeout mechanism.  Uses spin sleep with exponential back-off until
 * the sleep time reaches one jiffy, then switches over to task delay.  Usage:
 *
 *      dhd_timeout_start(&tmo, usec);
 *      while (!dhd_timeout_expired(&tmo))
 *              if (poll_something())
 *                      break;
 *      if (dhd_timeout_expired(&tmo))
 *              fatal();
 */

#ifdef CONFIG_PARTIALRESUME
static unsigned int dhd_get_ipv6_stat(u8 type)
{
	static unsigned int ra = 0;
	static unsigned int na = 0;
	static unsigned int other = 0;

	switch (type) {
	case NDISC_ROUTER_ADVERTISEMENT:
		ra++;
		return ra;
	case NDISC_NEIGHBOUR_ADVERTISEMENT:
		na++;
		return na;
	default:
		other++;
		break;
	}
	return other;
}
#endif

static int dhd_rx_suspend_again(struct sk_buff *skb)
{
#ifdef CONFIG_PARTIALRESUME
	u8 *pptr = skb_mac_header(skb);

	if (pptr &&
	    (memcmp(pptr, "\x33\x33\x00\x00\x00\x01", ETHER_ADDR_LEN) == 0) &&
	    (ntoh16(skb->protocol) == ETHER_TYPE_IPV6)) {
		u8 type = 0;
#define ETHER_ICMP6_TYPE	54
#define ETHER_ICMP6_DADDR	38
		if (skb->len > ETHER_ICMP6_TYPE)
			type = pptr[ETHER_ICMP6_TYPE];
		if ((type == NDISC_NEIGHBOUR_ADVERTISEMENT) &&
		    (ipv6_addr_equal(&in6addr_linklocal_allnodes,
		    (const struct in6_addr *)(pptr + ETHER_ICMP6_DADDR)))) {
			HW_PRINT_LO("%s: Suspend, type = %d [%u]\n", __func__,
				type, dhd_get_ipv6_stat(type));
			return 0;
		} else {
			HW_PRINT_LO("%s: Resume, type = %d [%u]\n", __func__,
				type, dhd_get_ipv6_stat(type));
		}
#undef ETHER_ICMP6_TYPE
#undef ETHER_ICMP6_DADDR
	}
#endif
	return DHD_PACKET_TIMEOUT_MS;
}

void
dhd_timeout_start(dhd_timeout_t *tmo, uint usec)
{
	tmo->limit = usec;
	tmo->increment = 0;
	tmo->elapsed = 0;
	tmo->tick = jiffies_to_usecs(1);
}

int
dhd_timeout_expired(dhd_timeout_t *tmo)
{
	/* Does nothing the first call */
	if (tmo->increment == 0) {
		tmo->increment = 1;
		return 0;
	}

	if (tmo->elapsed >= tmo->limit)
		return 1;

	/* Add the delay that's about to take place */
	tmo->elapsed += tmo->increment;

	if ((!CAN_SLEEP()) || tmo->increment < tmo->tick) {
		OSL_DELAY(tmo->increment);
		tmo->increment *= 2;
		if (tmo->increment > tmo->tick)
			tmo->increment = tmo->tick;
	} else {
		wait_queue_head_t delay_wait;
		DECLARE_WAITQUEUE(wait, current);
		init_waitqueue_head(&delay_wait);
		add_wait_queue(&delay_wait, &wait);
		set_current_state(TASK_INTERRUPTIBLE);
		(void)schedule_timeout(1);
		remove_wait_queue(&delay_wait, &wait);
		set_current_state(TASK_RUNNING);
	}

	return 0;
}

int
dhd_net2idx(dhd_info_t *dhd, struct net_device *net)
{
	int i = 0;

	ASSERT(dhd);
	while (i < DHD_MAX_IFS) {
		if (dhd->iflist[i] && dhd->iflist[i]->net && (dhd->iflist[i]->net == net))
			return i;
		i++;
	}

	return DHD_BAD_IF;
}

struct net_device * dhd_idx2net(void *pub, int ifidx)
{
	struct dhd_pub *dhd_pub = (struct dhd_pub *)pub;
	struct dhd_info *dhd_info;

	if (!dhd_pub || ifidx < 0 || ifidx >= DHD_MAX_IFS)
		return NULL;
	dhd_info = dhd_pub->info;
	if (dhd_info && dhd_info->iflist[ifidx])
		return dhd_info->iflist[ifidx]->net;
	return NULL;
}

int
dhd_ifname2idx(dhd_info_t *dhd, char *name)
{
	int i = DHD_MAX_IFS;

	ASSERT(dhd);

	if (name == NULL || *name == '\0' || dhd == NULL)
		return 0;

	while (--i > 0)
#ifdef  BRCM_RSDB
		if (dhd->iflist[i] && !strncmp(dhd->iflist[i]->dngl_name, name, IFNAMSIZ))
#else
		if (dhd->iflist[i] && !strncmp(dhd->iflist[i]->name, name, IFNAMSIZ))
#endif
				break;

	DHD_TRACE(("%s: return idx %d for \"%s\"\n", __FUNCTION__, i, name));

	return i;	/* default - the primary interface */
}

int
dhd_ifidx2hostidx(dhd_info_t *dhd, int ifidx)
{
	int i = DHD_MAX_IFS;

	ASSERT(dhd);

	if (ifidx < 0 || ifidx >= DHD_MAX_IFS) {
		DHD_TRACE(("%s: ifidx %d out of range\n", __FUNCTION__, ifidx));
		return 0;	/* default - the primary interface */
	}

	while (--i > 0)
		if (dhd->iflist[i] && (dhd->iflist[i]->idx == ifidx))
				break;

	DHD_TRACE(("%s: return hostidx %d for ifidx %d\n", __FUNCTION__, i, ifidx));

	return i;	/* default - the primary interface */
}

char *
dhd_ifname(dhd_pub_t *dhdp, int ifidx)
{
	dhd_info_t *dhd = (dhd_info_t *)dhdp->info;

	ASSERT(dhd);

	if (ifidx < 0 || ifidx >= DHD_MAX_IFS) {
		DHD_ERROR(("%s: ifidx %d out of range\n", __FUNCTION__, ifidx));
		return "<if_bad>";
	}

	if (dhd->iflist[ifidx] == NULL) {
		DHD_ERROR(("%s: null i/f %d\n", __FUNCTION__, ifidx));
		return "<if_null>";
	}

	if (dhd->iflist[ifidx]->net)
		return dhd->iflist[ifidx]->net->name;

	return "<if_none>";
}

uint8 *
dhd_bssidx2bssid(dhd_pub_t *dhdp, int idx)
{
	int i;
	dhd_info_t *dhd = (dhd_info_t *)dhdp;

	ASSERT(dhd);
	for (i = 0; i < DHD_MAX_IFS; i++)
	if (dhd->iflist[i] && dhd->iflist[i]->bssidx == idx)
		return dhd->iflist[i]->mac_addr;

	return NULL;
}


static void
_dhd_set_multicast_list(dhd_info_t *dhd, int ifidx)
{
	struct net_device *dev;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)
	struct netdev_hw_addr *ha;
#else
	struct dev_mc_list *mclist;
#endif
	uint32 allmulti, cnt;

	wl_ioctl_t ioc;
	char *buf, *bufp;
	uint buflen;
	int ret;

			//ASSERT(dhd && dhd->iflist[ifidx]);
			if (NULL == dhd || NULL == dhd->iflist[ifidx]) {
				DHD_ERROR(("_dhd_set_multicast_list dhd or dhd->iflist[ifidx] is NULL\n"));
				return;
			}
			dev = dhd->iflist[ifidx]->net;
			if (!dev)
				return;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
			netif_addr_lock_bh(dev);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)
			cnt = netdev_mc_count(dev);
#else
			cnt = dev->mc_count;
#endif /* LINUX_VERSION_CODE */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
			netif_addr_unlock_bh(dev);
#endif

			/* Determine initial value of allmulti flag */
	allmulti = (dev->flags & IFF_ALLMULTI) ? TRUE : FALSE;

	/* Send down the multicast list first. */


	buflen = sizeof("mcast_list") + sizeof(cnt) + (cnt * ETHER_ADDR_LEN);
	if (!(bufp = buf = MALLOC(dhd->pub.osh, buflen))) {
		DHD_ERROR(("%s: out of memory for mcast_list, cnt %d\n",
		           dhd_ifname(&dhd->pub, ifidx), cnt));
		return;
	}

	strncpy(bufp, "mcast_list", buflen - 1);
	bufp[buflen - 1] = '\0';
	bufp += strlen("mcast_list") + 1;

	cnt = htol32(cnt);
	memcpy(bufp, &cnt, sizeof(cnt));
	bufp += sizeof(cnt);


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
			netif_addr_lock_bh(dev);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)
			netdev_for_each_mc_addr(ha, dev) {
				if (!cnt)
					break;
				memcpy(bufp, ha->addr, ETHER_ADDR_LEN);
				bufp += ETHER_ADDR_LEN;
				cnt--;
	}
#else
	for (mclist = dev->mc_list; (mclist && (cnt > 0));
		cnt--, mclist = mclist->next) {
				memcpy(bufp, (void *)mclist->dmi_addr, ETHER_ADDR_LEN);
				bufp += ETHER_ADDR_LEN;
			}
#endif /* LINUX_VERSION_CODE */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
			netif_addr_unlock_bh(dev);
#endif

	memset(&ioc, 0, sizeof(ioc));
	ioc.cmd = WLC_SET_VAR;
	ioc.buf = buf;
	ioc.len = buflen;
	ioc.set = TRUE;

	ret = dhd_wl_ioctl(&dhd->pub, ifidx, &ioc, ioc.buf, ioc.len);
	if (ret < 0) {
		DHD_ERROR(("%s: set mcast_list failed, cnt %d\n",
			dhd_ifname(&dhd->pub, ifidx), cnt));
		allmulti = cnt ? TRUE : allmulti;
	}

	MFREE(dhd->pub.osh, buf, buflen);

	/* Now send the allmulti setting.  This is based on the setting in the
	 * net_device flags, but might be modified above to be turned on if we
	 * were trying to set some addresses and dongle rejected it...
	 */

	allmulti = htol32(allmulti);
	ret = dhd_iovar(&dhd->pub, ifidx, "allmulti", (char *)&allmulti,
			sizeof(allmulti), NULL, 0, TRUE);
	if (ret < 0) {
		DHD_ERROR(("%s: set allmulti %d failed\n",
		           dhd_ifname(&dhd->pub, ifidx), ltoh32(allmulti)));
	}

	/* Finally, pick up the PROMISC flag as well, like the NIC driver does */

	allmulti = (dev->flags & IFF_PROMISC) ? TRUE : FALSE;

	allmulti = htol32(allmulti);

	memset(&ioc, 0, sizeof(ioc));
	ioc.cmd = WLC_SET_PROMISC;
	ioc.buf = &allmulti;
	ioc.len = sizeof(allmulti);
	ioc.set = TRUE;

	ret = dhd_wl_ioctl(&dhd->pub, ifidx, &ioc, ioc.buf, ioc.len);
	if (ret < 0) {
		DHD_ERROR(("%s: set promisc %d failed\n",
		           dhd_ifname(&dhd->pub, ifidx), ltoh32(allmulti)));
	}
}

int
_dhd_set_mac_address(dhd_info_t *dhd, int ifidx, uint8 *addr)
{
	int ret;

	ret = dhd_iovar(&dhd->pub, ifidx, "cur_etheraddr", (char *)addr,
			ETHER_ADDR_LEN, NULL, 0, TRUE);
	if (ret < 0) {
		DHD_ERROR(("%s: set cur_etheraddr failed\n", dhd_ifname(&dhd->pub, ifidx)));
	} else {
		memcpy(dhd->iflist[ifidx]->net->dev_addr, addr, ETHER_ADDR_LEN);
		if (ifidx == 0)
			memcpy(dhd->pub.mac.octet, addr, ETHER_ADDR_LEN);
	}

	return ret;
}

#ifdef SOFTAP
struct net_device *ap_net_dev;
#endif

static void
dhd_ifadd_event_handler(void *handle, void *event_info, u8 event)
{
	dhd_info_t *dhd = handle;
	dhd_if_event_t *if_event = event_info;
	struct net_device *ndev;
	int ifidx, bssidx;
	int ret;
#if 1 && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
	struct wireless_dev *vwdev, *primary_wdev;
	struct net_device *primary_ndev;
#endif /* OEM_ANDROID && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0)) */

	if (event != DHD_WQ_WORK_IF_ADD) {
		DHD_ERROR(("%s: unexpected event \n", __FUNCTION__));
		return;
	}

	if (!dhd) {
		DHD_ERROR(("%s: dhd info not available \n", __FUNCTION__));
		return;
	}

	if (!if_event) {
		DHD_ERROR(("%s: event data is null \n", __FUNCTION__));
		return;
	}

	dhd_net_if_lock_local(dhd);
	DHD_OS_WAKE_LOCK(&dhd->pub);
	DHD_PERIM_LOCK(&dhd->pub);

	ifidx = if_event->event.ifidx;
	bssidx = if_event->event.bssidx;
	DHD_TRACE(("%s: registering if with ifidx %d\n", __FUNCTION__, ifidx));

#ifndef  BRCM_RSDB
	ndev = dhd_allocate_if(&dhd->pub, ifidx, if_event->name,
		if_event->mac, bssidx, TRUE);
#else
	ndev = dhd_allocate_if(&dhd->pub, ifidx, if_event->name,
		if_event->mac, bssidx, TRUE, if_event->name);
#endif

	if (!ndev) {
		DHD_ERROR(("%s: net device alloc failed  \n", __FUNCTION__));
		goto done;
	}

#if 1 && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
	vwdev = kzalloc(sizeof(*vwdev), GFP_KERNEL);
	if (unlikely(!vwdev)) {
		DHD_ERROR(("Could not allocate wireless device\n"));
		goto done;
	}
	primary_ndev = dhd->pub.info->iflist[0]->net;
	primary_wdev = ndev_to_wdev(primary_ndev);
	vwdev->wiphy = primary_wdev->wiphy;
	vwdev->iftype = if_event->event.role;
	vwdev->netdev = ndev;
	ndev->ieee80211_ptr = vwdev;
	SET_NETDEV_DEV(ndev, wiphy_dev(vwdev->wiphy));
	DHD_ERROR(("virtual interface(%s) is created\n", if_event->name));
#endif /* OEM_ANDROID && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0)) */

	DHD_PERIM_UNLOCK(&dhd->pub);
	ret = dhd_register_if(&dhd->pub, ifidx, TRUE);
	DHD_PERIM_LOCK(&dhd->pub);
	if (ret != BCME_OK) {
		DHD_ERROR(("%s: dhd_register_if failed\n", __FUNCTION__));
		dhd_remove_if(&dhd->pub, ifidx, TRUE);
#ifdef BCM_PCIE_UPDATE
		goto done;
#endif
	}
#ifdef PCIE_FULL_DONGLE
#ifndef BCM_PCIE_UPDATE
	/* Turn on AP isolation in the firmware for interfaces operating in AP mode */
	if (FW_SUPPORTED((&dhd->pub), ap) && !(DHD_IF_ROLE_STA(if_event->event.role))) {
		uint32 var_int =  1;
		dhd_iovar(&dhd->pub, ifidx, "ap_isolate", (char *)&var_int,
			  sizeof(var_int), NULL, 0, TRUE);
	}
#else
	/* Turn on AP isolation in the firmware for interfaces operating in AP mode */
	if (FW_SUPPORTED((&dhd->pub), ap) && (if_event->event.role != WLC_E_IF_ROLE_STA)) {
		char iovbuf[WLC_IOCTL_SMLEN];
		uint32 var_int =  1;

		memset(iovbuf, 0, sizeof(iovbuf));
		bcm_mkiovar("ap_isolate", (char *)&var_int, 4, iovbuf, sizeof(iovbuf));
		ret = dhd_wl_ioctl_cmd(&dhd->pub, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, ifidx);

		if (ret != BCME_OK) {
			DHD_ERROR(("%s: Failed to set ap_isolate to dongle\n", __FUNCTION__));
			dhd_remove_if(&dhd->pub, ifidx, TRUE);
		}
	}
#endif
#endif /* PCIE_FULL_DONGLE */
done:
	MFREE(dhd->pub.osh, if_event, sizeof(dhd_if_event_t));

	DHD_PERIM_UNLOCK(&dhd->pub);
	DHD_OS_WAKE_UNLOCK(&dhd->pub);
	dhd_net_if_unlock_local(dhd);
}

static void
dhd_ifdel_event_handler(void *handle, void *event_info, u8 event)
{
	dhd_info_t *dhd = handle;
	int ifidx;
	dhd_if_event_t *if_event = event_info;


	if (event != DHD_WQ_WORK_IF_DEL) {
		DHD_ERROR(("%s: unexpected event \n", __FUNCTION__));
		return;
	}

	if (!dhd) {
		DHD_ERROR(("%s: dhd info not available \n", __FUNCTION__));
		return;
	}

	if (!if_event) {
		DHD_ERROR(("%s: event data is null \n", __FUNCTION__));
		return;
	}

	dhd_net_if_lock_local(dhd);
	DHD_OS_WAKE_LOCK(&dhd->pub);
	DHD_PERIM_LOCK(&dhd->pub);

	ifidx = if_event->event.ifidx;
	DHD_TRACE(("Removing interface with idx %d\n", ifidx));

	dhd_remove_if(&dhd->pub, ifidx, TRUE);

	MFREE(dhd->pub.osh, if_event, sizeof(dhd_if_event_t));

	DHD_PERIM_UNLOCK(&dhd->pub);
	DHD_OS_WAKE_UNLOCK(&dhd->pub);
	dhd_net_if_unlock_local(dhd);
}

static void
dhd_set_mac_addr_handler(void *handle, void *event_info, u8 event)
{
	dhd_info_t *dhd = handle;
	dhd_if_t *ifp = event_info;

	if (event != DHD_WQ_WORK_SET_MAC) {
		DHD_ERROR(("%s: unexpected event \n", __FUNCTION__));
	}

	if (!dhd) {
		DHD_ERROR(("%s: dhd info not available \n", __FUNCTION__));
		return;
	}

	dhd_net_if_lock_local(dhd);
	DHD_OS_WAKE_LOCK(&dhd->pub);
	DHD_PERIM_LOCK(&dhd->pub);

#ifdef SOFTAP
	{
		unsigned long flags;
		bool in_ap = FALSE;
		DHD_GENERAL_LOCK(&dhd->pub, flags);
		in_ap = (ap_net_dev != NULL);
		DHD_GENERAL_UNLOCK(&dhd->pub, flags);

		if (in_ap)  {
			DHD_ERROR(("attempt to set MAC for %s in AP Mode, blocked. \n",
			           ifp->net->name));
			goto done;
		}
	}
#endif /* SOFTAP */

	if (ifp == NULL || !dhd->pub.up) {
		DHD_ERROR(("%s: interface info not available/down \n", __FUNCTION__));
		goto done;
	}

	DHD_ERROR(("%s: MACID is overwritten\n", __FUNCTION__));
	ifp->set_macaddress = FALSE;
	if (_dhd_set_mac_address(dhd, ifp->idx, ifp->mac_addr) == 0)
		DHD_INFO(("%s: MACID is overwritten\n",	__FUNCTION__));
	else
		DHD_ERROR(("%s: _dhd_set_mac_address() failed\n", __FUNCTION__));

done:
	DHD_PERIM_UNLOCK(&dhd->pub);
	DHD_OS_WAKE_UNLOCK(&dhd->pub);
	dhd_net_if_unlock_local(dhd);
}

static void
dhd_set_mcast_list_handler(void *handle, void *event_info, u8 event)
{
	dhd_info_t *dhd = handle;
	dhd_if_t *ifp = event_info;
	int ifidx;

	if (event != DHD_WQ_WORK_SET_MCAST_LIST) {
		DHD_ERROR(("%s: unexpected event \n", __FUNCTION__));
		return;
	}

	if (!dhd) {
		DHD_ERROR(("%s: dhd info not available \n", __FUNCTION__));
		return;
	}

	dhd_net_if_lock_local(dhd);
	DHD_OS_WAKE_LOCK(&dhd->pub);
	DHD_PERIM_LOCK(&dhd->pub);

#ifdef SOFTAP
	{
		bool in_ap = FALSE;
		unsigned long flags;
		DHD_GENERAL_LOCK(&dhd->pub, flags);
		in_ap = (ap_net_dev != NULL);
		DHD_GENERAL_UNLOCK(&dhd->pub, flags);

		if (in_ap)  {
			DHD_ERROR(("set MULTICAST list for %s in AP Mode, blocked. \n",
			           ifp->net->name));
			ifp->set_multicast = FALSE;
			goto done;
		}
	}
#endif /* SOFTAP */

	if (ifp == NULL || !dhd->pub.up) {
		DHD_ERROR(("%s: interface info not available/down \n", __FUNCTION__));
		goto done;
	}

	ifidx = ifp->idx;


	_dhd_set_multicast_list(dhd, ifidx);
	DHD_INFO(("%s: set multicast list for if %d\n", __FUNCTION__, ifidx));

done:
	DHD_PERIM_UNLOCK(&dhd->pub);
	DHD_OS_WAKE_UNLOCK(&dhd->pub);
	dhd_net_if_unlock_local(dhd);
}

static int
dhd_set_mac_address(struct net_device *dev, void *addr)
{
	int ret = 0;

	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	struct sockaddr *sa = (struct sockaddr *)addr;
	int ifidx;
	dhd_if_t *dhdif;

	ifidx = dhd_net2idx(dhd, dev);
	if (ifidx == DHD_BAD_IF)
		return -1;

	dhdif = dhd->iflist[ifidx];

	dhd_net_if_lock_local(dhd);
	memcpy(dhdif->mac_addr, sa->sa_data, ETHER_ADDR_LEN);
	dhdif->set_macaddress = TRUE;
	dhd_net_if_unlock_local(dhd);
#ifdef HW_DEFERRED_WORK_BUGFIX
	ret = dhd_deferred_schedule_work(dhd->dhd_deferred_wq, (void *)dhdif, DHD_WQ_WORK_SET_MAC,
			dhd_set_mac_addr_handler, DHD_WORK_PRIORITY_LOW);
	if (ret)
		DHD_ERROR(("dhd_set_mac_address failed\n"));
#else
	dhd_deferred_schedule_work(dhd->dhd_deferred_wq, (void *)dhdif, DHD_WQ_WORK_SET_MAC,
		dhd_set_mac_addr_handler, DHD_WORK_PRIORITY_LOW);
#endif
	return ret;
}

static void
dhd_set_multicast_list(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	int ifidx;
#ifdef HW_DEFERRED_WORK_BUGFIX
	int ret = 0;
#endif

	ifidx = dhd_net2idx(dhd, dev);
	if (ifidx == DHD_BAD_IF)
		return;

	dhd->iflist[ifidx]->set_multicast = TRUE;
#ifdef HW_DEFERRED_WORK_BUGFIX
	ret = dhd_deferred_schedule_work(dhd->dhd_deferred_wq, (void *)dhd->iflist[ifidx],
			DHD_WQ_WORK_SET_MCAST_LIST, dhd_set_mcast_list_handler, DHD_WORK_PRIORITY_LOW);
	if (ret)
		DHD_ERROR(("dhd_set_multicast_list failed\n"));
#else
	dhd_deferred_schedule_work(dhd->dhd_deferred_wq, (void *)dhd->iflist[ifidx],
		DHD_WQ_WORK_SET_MCAST_LIST, dhd_set_mcast_list_handler, DHD_WORK_PRIORITY_LOW);
#endif
}

#ifdef PROP_TXSTATUS
int
dhd_os_wlfc_block(dhd_pub_t *pub)
{
	dhd_info_t *di = (dhd_info_t *)(pub->info);
	ASSERT(di != NULL);
	spin_lock_bh(&di->wlfc_spinlock);
	return 1;
}

int
dhd_os_wlfc_unblock(dhd_pub_t *pub)
{
	dhd_info_t *di = (dhd_info_t *)(pub->info);

	ASSERT(di != NULL);
	spin_unlock_bh(&di->wlfc_spinlock);
	return 1;
}

#endif /* PROP_TXSTATUS */

#if defined(DHD_8021X_DUMP)
void
dhd_tx_dump(osl_t *osh, void *pkt)
{
	uint8 *dump_data;
	uint16 protocol;

	dump_data = PKTDATA(osh, pkt);
	protocol = (dump_data[12] << 8) | dump_data[13];

	if (protocol == ETHER_TYPE_802_1X) {
		DHD_ERROR(("ETHER_TYPE_802_1X [TX]: ver %d, type %d, replay %d\n",
			dump_data[14], dump_data[15], dump_data[30]));
	}
}
#endif /* DHD_8021X_DUMP */

int BCMFASTPATH
dhd_sendpkt(dhd_pub_t *dhdp, int ifidx, void *pktbuf)
{
	int ret = BCME_OK;
	dhd_info_t *dhd = (dhd_info_t *)(dhdp->info);
	struct ether_header *eh = NULL;
#ifdef PCIE_FULL_DONGLE
	unsigned long flags;
	DHD_GENERAL_LOCK(dhdp, flags);
#endif
	/* Reject if down */
	if (!dhdp->up || (dhdp->busstate == DHD_BUS_DOWN)) {
		/* free the packet here since the caller won't */
		PKTFREE(dhdp->osh, pktbuf, TRUE);
#ifdef PCIE_FULL_DONGLE
		DHD_GENERAL_UNLOCK(dhdp, flags);
#endif
		return -ENODEV;
	}

#ifdef PCIE_FULL_DONGLE
	if (dhdp->busstate == DHD_BUS_SUSPEND) {
		DHD_ERROR(("%s : pcie is still in suspend state!!\n", __FUNCTION__));
		PKTFREE(dhdp->osh, pktbuf, TRUE);
		DHD_GENERAL_UNLOCK(dhdp, flags);
		return -EBUSY;
	}
	DHD_GENERAL_UNLOCK(dhdp, flags);
#endif /* PCIE_FULL_DONGLE */

#ifdef BCM_BLOCK_DATA_FRAME
	if (dhdp->blockframe_flag) {
		uint8 *pktdata = (uint8 *)PKTDATA(dhdp->osh, pktbuf);
		uint16 cur_ether_type;
		eh = (struct ether_header *)pktdata;
	        cur_ether_type	= ntoh16(eh->ether_type);

		if ((cur_ether_type != ETHER_TYPE_802_1X) && (cur_ether_type != ETHER_TYPE_BRCM )) {
			DHD_ERROR(("%s : Drop all of the frames when connecting process!\n", __FUNCTION__));
			PKTFREE(dhdp->osh, pktbuf, TRUE);
			return -EBUSY;
		}

	}
#endif

#ifdef DHD_UNICAST_DHCP
	/* if dhcp_unicast is enabled, we need to convert the */
	/* broadcast DHCP ACK/REPLY packets to Unicast. */
	if (dhdp->dhcp_unicast) {
	    dhd_convert_dhcp_broadcast_ack_to_unicast(dhdp, pktbuf, ifidx);
	}
#endif /* DHD_UNICAST_DHCP */
	/* Update multicast statistic */
	if (PKTLEN(dhdp->osh, pktbuf) >= ETHER_HDR_LEN) {
		uint8 *pktdata = (uint8 *)PKTDATA(dhdp->osh, pktbuf);
		eh = (struct ether_header *)pktdata;

		if (ETHER_ISMULTI(eh->ether_dhost))
			dhdp->tx_multicast++;
		if (ntoh16(eh->ether_type) == ETHER_TYPE_802_1X) {
			DBG_EVENT_LOG(dhdp, WIFI_EVENT_DRIVER_EAPOL_FRAME_TRANSMIT_REQUESTED);
			atomic_inc(&dhd->pend_8021x_cnt);
		}
#ifdef DHD_DHCP_DUMP
		if (ntoh16(eh->ether_type) == ETHER_TYPE_IP) {
			uint16 dump_hex;
			uint16 source_port;
			uint16 dest_port;
			uint16 udp_port_pos;
			uint8 *ptr8 = (uint8 *)&pktdata[ETHER_HDR_LEN];
			uint8 ip_header_len = (*ptr8 & 0x0f)<<2;

			udp_port_pos = ETHER_HDR_LEN + ip_header_len;
			source_port = (pktdata[udp_port_pos] << 8) | pktdata[udp_port_pos+1];
			dest_port = (pktdata[udp_port_pos+2] << 8) | pktdata[udp_port_pos+3];
			if (source_port == 0x0044 || dest_port == 0x0044) {
				dump_hex = (pktdata[udp_port_pos+249] << 8) |
					pktdata[udp_port_pos+250];
				if (dump_hex == 0x0101) {
					DHD_ERROR(("DHCP - DISCOVER [TX]\n"));
				} else if (dump_hex == 0x0102) {
					DHD_ERROR(("DHCP - OFFER [TX]\n"));
				} else if (dump_hex == 0x0103) {
					DHD_ERROR(("DHCP - REQUEST [TX]\n"));
				} else if (dump_hex == 0x0105) {
					DHD_ERROR(("DHCP - ACK [TX]\n"));
				} else {
					DHD_ERROR(("DHCP - 0x%X [TX]\n", dump_hex));
				}
			} else if (source_port == 0x0043 || dest_port == 0x0043) {
				DHD_ERROR(("DHCP - BOOTP [RX]\n"));
			}
		}
#endif /* DHD_DHCP_DUMP */
	} else {
			PKTFREE(dhd->pub.osh, pktbuf, TRUE);
			return BCME_ERROR;
	}

#ifdef DHDTCPACK_SUPPRESS
	/* If this packet has replaced another packet and got freed, just return */
	if (dhd_tcpack_suppress(dhdp, pktbuf))
		return ret;
#endif /* DHDTCPACK_SUPPRESS */

	/* Look into the packet and update the packet priority */
#ifndef PKTPRIO_OVERRIDE
	if (PKTPRIO(pktbuf) == 0)
#endif
		pktsetprio(pktbuf, FALSE);


#ifdef PCIE_FULL_DONGLE
	/*
	 * Lkup the per interface hash table, for a matching flowring. If one is not
	 * available, allocate a unique flowid and add a flowring entry.
	 * The found or newly created flowid is placed into the pktbuf's tag.
	 */
	ret = dhd_flowid_update(dhdp, ifidx, dhdp->flow_prio_map[(PKTPRIO(pktbuf))], pktbuf);
	if (ret != BCME_OK) {
		DHD_ERROR(("%s: dhd_flowid_update failed, pkt dropped.\n", __FUNCTION__));
		PKTCFREE(dhd->pub.osh, pktbuf, TRUE);
		return ret;
	}
#endif

#ifdef PROP_TXSTATUS
	if (dhd_wlfc_is_supported(dhdp)) {
		/* store the interface ID */
		DHD_PKTTAG_SETIF(PKTTAG(pktbuf), ifidx);

		/* store destination MAC in the tag as well */
		DHD_PKTTAG_SETDSTN(PKTTAG(pktbuf), eh->ether_dhost);

		/* decide which FIFO this packet belongs to */
		if (ETHER_ISMULTI(eh->ether_dhost))
			/* one additional queue index (highest AC + 1) is used for bc/mc queue */
			DHD_PKTTAG_SETFIFO(PKTTAG(pktbuf), AC_COUNT);
		else
			DHD_PKTTAG_SETFIFO(PKTTAG(pktbuf), WME_PRIO2AC(PKTPRIO(pktbuf)));
	} else
#endif /* PROP_TXSTATUS */
	/* If the protocol uses a data header, apply it */
	dhd_prot_hdrpush(dhdp, ifidx, pktbuf);

	/* Use bus module to send data frame */
#ifdef WLMEDIA_HTSF
	dhd_htsf_addtxts(dhdp, pktbuf);
#endif
#if defined(DHD_8021X_DUMP)
	dhd_tx_dump(dhdp->osh, pktbuf);
#endif
#ifdef PROP_TXSTATUS
	{
		if (dhd_wlfc_commit_packets(dhdp, (f_commitpkt_t)dhd_bus_txdata,
			dhdp->bus, pktbuf, TRUE) == WLFC_UNSUPPORTED) {
			/* non-proptxstatus way */
#ifdef BCMPCIE
			ret = dhd_bus_txdata(dhdp->bus, pktbuf, (uint8)ifidx);
#else
			ret = dhd_bus_txdata(dhdp->bus, pktbuf);
#endif /* BCMPCIE */
		}
	}
#else
#ifdef BCMPCIE
	ret = dhd_bus_txdata(dhdp->bus, pktbuf, (uint8)ifidx);
#else
	ret = dhd_bus_txdata(dhdp->bus, pktbuf);
#endif /* BCMPCIE */
#endif /* PROP_TXSTATUS */

	return ret;
}

int BCMFASTPATH
dhd_start_xmit(struct sk_buff *skb, struct net_device *net)
{
	int ret;
	uint datalen;
	void *pktbuf;
	dhd_info_t *dhd = DHD_DEV_INFO(net);
	dhd_if_t *ifp = NULL;
	int ifidx;
#ifdef PCIE_FULL_DONGLE
	unsigned long flags;
#endif
#ifdef WLMEDIA_HTSF
	uint8 htsfdlystat_sz = dhd->pub.htsfdlystat_sz;
#else
	uint8 htsfdlystat_sz = 0;
#endif
#ifdef DHD_WMF
	struct ether_header *eh;
	uint8 *iph;
#endif /* DHD_WMF */
#ifdef CONFIG_HUAWEI_XENGINE
	struct sock *sk;
#endif
	DHD_TRACE(("%s: Enter\n", __FUNCTION__));
#ifdef PCIE_FULL_DONGLE
	if (dhd->pub.busstate == DHD_BUS_SUSPEND) {
		DHD_GENERAL_LOCK(&dhd->pub, flags);
		dhd->pub.dhd_bus_busy_state &= ~DHD_BUS_BUSY_IN_TX;
		dhd_os_busbusy_wake(&dhd->pub);
		DHD_GENERAL_UNLOCK(&dhd->pub, flags);
		DHD_ERROR(("%s : pcie is still in suspend state!!\n", __FUNCTION__));
		dev_kfree_skb_any(skb);
		ifp = DHD_DEV_IFP(net);
		ifp->stats.tx_dropped++;
		dhd->pub.tx_dropped++;
		return NETDEV_TX_OK;
	}
	DHD_GENERAL_LOCK(&dhd->pub, flags);
	dhd->pub.dhd_bus_busy_state |= DHD_BUS_BUSY_IN_TX;
	DHD_GENERAL_UNLOCK(&dhd->pub, flags);
#endif

	DHD_OS_WAKE_LOCK(&dhd->pub);
	DHD_PERIM_LOCK_TRY(DHD_FWDER_UNIT(dhd), TRUE);

	/* Reject if down */
#ifdef PCIE_FULL_DONGLE
	if (dhd->pub.hang_was_sent || dhd->pub.busstate == DHD_BUS_DOWN ||
		dhd->pub.busstate == DHD_BUS_DOWN_IN_PROGRESS) {
#else
	if (dhd->pub.busstate == DHD_BUS_DOWN || dhd->pub.hang_was_sent) {
#endif
		DHD_ERROR(("%s: xmit rejected pub.up=%d busstate=%d \n",
			__FUNCTION__, dhd->pub.up, dhd->pub.busstate));
		netif_stop_queue(net);
		/* Send Event when bus down detected during data session */
#ifdef PCIE_FULL_DONGLE
		if (dhd->pub.up && !dhd->pub.hang_was_sent) {
#else
		if (dhd->pub.up) {
#endif
			DHD_ERROR(("%s: Event HANG sent up\n", __FUNCTION__));
			net_os_send_hang_message(net);
		}
#ifdef PCIE_FULL_DONGLE
		DHD_GENERAL_LOCK(&dhd->pub, flags);
		dhd->pub.dhd_bus_busy_state &= ~DHD_BUS_BUSY_IN_TX;
		dhd_os_busbusy_wake(&dhd->pub);
		DHD_GENERAL_UNLOCK(&dhd->pub, flags);
#endif /* PCIE_FULL_DONGLE */

		DHD_PERIM_UNLOCK_TRY(DHD_FWDER_UNIT(dhd), TRUE);
		DHD_OS_WAKE_UNLOCK(&dhd->pub);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 20))
		return -ENODEV;
#else
		return NETDEV_TX_BUSY;
#endif
	}

	ifp = DHD_DEV_IFP(net);
	ifidx = DHD_DEV_IFIDX(net);

	ASSERT(ifidx == dhd_net2idx(dhd, net));
	ASSERT((ifp != NULL) && (ifp == dhd->iflist[ifidx]));

	if (ifidx == DHD_BAD_IF) {
		DHD_ERROR(("%s: bad ifidx %d\n", __FUNCTION__, ifidx));
		netif_stop_queue(net);

#ifdef PCIE_FULL_DONGLE
		DHD_GENERAL_LOCK(&dhd->pub, flags);
		dhd->pub.dhd_bus_busy_state &= ~DHD_BUS_BUSY_IN_TX;
		dhd_os_busbusy_wake(&dhd->pub);
		DHD_GENERAL_UNLOCK(&dhd->pub, flags);
#endif /* PCIE_FULL_DONGLE */

		DHD_PERIM_UNLOCK_TRY(DHD_FWDER_UNIT(dhd), TRUE);
		DHD_OS_WAKE_UNLOCK(&dhd->pub);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 20))
		return -ENODEV;
#else
		return NETDEV_TX_BUSY;
#endif
	}
#ifdef DHD_DEVWAKE_EARLY
	if (dhd_os_devwake_check_deepsleep(&dhd->pub)) {
		DHD_PERIM_UNLOCK_TRY(DHD_FWDER_UNIT(dhd), TRUE);
                DHD_OS_WAKE_UNLOCK(&dhd->pub);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 20))
		return -ENODEV;
#else
		return NETDEV_TX_BUSY;
#endif
	}
#endif /* DHD_DEVWAKE_EARLY */

	/* re-align socket buffer if "skb->data" is odd address */
	if (((unsigned long)(skb->data)) & 0x1) {
		unsigned char *data = skb->data;
		uint32 length = skb->len;
		PKTPUSH(dhd->pub.osh, skb, 1);
		memmove(skb->data, data, length);
		PKTSETLEN(dhd->pub.osh, skb, length);
	}

	datalen  = PKTLEN(dhd->pub.osh, skb);

	/* Make sure there's enough room for any header */

	if (skb_headroom(skb) < dhd->pub.hdrlen + htsfdlystat_sz) {
		struct sk_buff *skb2;

		DHD_INFO(("%s: insufficient headroom\n",
		          dhd_ifname(&dhd->pub, ifidx)));
		dhd->pub.tx_realloc++;

		skb2 = skb_realloc_headroom(skb, dhd->pub.hdrlen + htsfdlystat_sz);

		dev_kfree_skb(skb);
		if ((skb = skb2) == NULL) {
			DHD_ERROR(("%s: skb_realloc_headroom failed\n",
			           dhd_ifname(&dhd->pub, ifidx)));
			ret = -ENOMEM;
			goto done;
		}
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
	skb_orphan(skb);
#endif


	/* Convert to packet */
	if (!(pktbuf = PKTFRMNATIVE(dhd->pub.osh, skb))) {
		DHD_ERROR(("%s: PKTFRMNATIVE failed\n",
		           dhd_ifname(&dhd->pub, ifidx)));
		dev_kfree_skb_any(skb);
		ret = -ENOMEM;
		goto done;
	}
#ifdef WLMEDIA_HTSF
	if (htsfdlystat_sz && PKTLEN(dhd->pub.osh, pktbuf) >= ETHER_ADDR_LEN) {
		uint8 *pktdata = (uint8 *)PKTDATA(dhd->pub.osh, pktbuf);
		struct ether_header *eh = (struct ether_header *)pktdata;

		if (!ETHER_ISMULTI(eh->ether_dhost) &&
			(ntoh16(eh->ether_type) == ETHER_TYPE_IP)) {
			eh->ether_type = hton16(ETHER_TYPE_BRCM_PKTDLYSTATS);
		}
	}
#endif
#ifdef DHD_WMF
	eh = (struct ether_header *)PKTDATA(dhd->pub.osh, pktbuf);
	iph = (uint8 *)eh + ETHER_HDR_LEN;

	/* WMF processing for multicast packets
	 * Only IPv4 packets are handled
	 */
	if (ifp->wmf.wmf_enable && (ntoh16(eh->ether_type) == ETHER_TYPE_IP) &&
		(IP_VER(iph) == IP_VER_4) && (ETHER_ISMULTI(eh->ether_dhost) ||
		((IPV4_PROT(iph) == IP_PROT_IGMP) && dhd->pub.wmf_ucast_igmp))) {
#if defined(DHD_IGMP_UCQUERY) || defined(DHD_UCAST_UPNP)
		void *sdu_clone;
		bool ucast_convert = FALSE;
#ifdef DHD_UCAST_UPNP
		uint32 dest_ip;

		dest_ip = ntoh32(*((uint32 *)(iph + IPV4_DEST_IP_OFFSET)));
		ucast_convert = dhd->pub.wmf_ucast_upnp && MCAST_ADDR_UPNP_SSDP(dest_ip);
#endif /* DHD_UCAST_UPNP */
#ifdef DHD_IGMP_UCQUERY
		ucast_convert |= dhd->pub.wmf_ucast_igmp_query &&
			(IPV4_PROT(iph) == IP_PROT_IGMP) &&
			(*(iph + IPV4_HLEN(iph)) == IGMPV2_HOST_MEMBERSHIP_QUERY);
#endif /* DHD_IGMP_UCQUERY */
		if (ucast_convert) {
			dhd_sta_t *sta;
			unsigned long flags;
#ifdef PCIE_FULL_DONGLE
			DHD_GENERAL_LOCK(&dhd->pub, flags);
			dhd->pub.dhd_bus_busy_state &= ~DHD_BUS_BUSY_IN_TX;
			dhd_os_busbusy_wake(&dhd->pub);
			DHD_GENERAL_UNLOCK(&dhd->pub, flags);
#endif /* PCIE_FULL_DONGLE */
			DHD_IF_STA_LIST_LOCK(ifp, flags);

			/* Convert upnp/igmp query to unicast for each assoc STA */
			list_for_each_entry(sta, &ifp->sta_list, list) {
				if ((sdu_clone = PKTDUP(dhd->pub.osh, pktbuf)) == NULL) {
					DHD_IF_STA_LIST_UNLOCK(ifp, flags);
					DHD_PERIM_UNLOCK_TRY(DHD_FWDER_UNIT(dhd), TRUE);
					DHD_OS_WAKE_UNLOCK(&dhd->pub);
					return (WMF_NOP);
				}
				dhd_wmf_forward(ifp->wmf.wmfh, sdu_clone, 0, sta, 1);
			}

			DHD_IF_STA_LIST_UNLOCK(ifp, flags);
			DHD_PERIM_UNLOCK_TRY(DHD_FWDER_UNIT(dhd), TRUE);
			DHD_OS_WAKE_UNLOCK(&dhd->pub);

			PKTFREE(dhd->pub.osh, pktbuf, TRUE);
			return NETDEV_TX_OK;
		} else
#endif /* defined(DHD_IGMP_UCQUERY) || defined(DHD_UCAST_UPNP) */
		{
			/* There will be no STA info if the packet is coming from LAN host
			 * Pass as NULL
			 */
			ret = dhd_wmf_packets_handle(&dhd->pub, pktbuf, NULL, ifidx, 0);
			switch (ret) {
			case WMF_TAKEN:
			case WMF_DROP:
				/* Either taken by WMF or we should drop it.
				 * Exiting send path
				 */
#ifdef PCIE_FULL_DONGLE
				DHD_GENERAL_LOCK(&dhd->pub, flags);
				dhd->pub.dhd_bus_busy_state &= ~DHD_BUS_BUSY_IN_TX;
				dhd_os_busbusy_wake(&dhd->pub);
				DHD_GENERAL_UNLOCK(&dhd->pub, flags);
#endif /* PCIE_FULL_DONGLE */
				DHD_PERIM_UNLOCK_TRY(DHD_FWDER_UNIT(dhd), TRUE);
				DHD_OS_WAKE_UNLOCK(&dhd->pub);
				return NETDEV_TX_OK;
			default:
				/* Continue the transmit path */
				break;
			}
		}
	}
#endif /* DHD_WMF */

#ifdef CONFIG_HUAWEI_XENGINE
	/*for high priority data ,set  priority of skbuff as DSCP_EF£¨2E£©*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 23))
	sk = skb_to_full_sk((struct sk_buff*)pktbuf);
#else
	sk = ((struct sk_buff*)pktbuf)->sk;
#endif
	if( NULL !=sk )
	{
		if(sk->acc_state)
			PKTSETPRIO(pktbuf, DSCP_EF);
	}
#endif

	ret = dhd_sendpkt(&dhd->pub, ifidx, pktbuf);

done:
	if (ret) {
		ifp->stats.tx_dropped++;
		dhd->pub.tx_dropped++;
	}
	else {
		dhd->pub.tx_packets++;
		ifp->stats.tx_packets++;
		ifp->stats.tx_bytes += datalen;
	}
#ifdef PCIE_FULL_DONGLE
	DHD_GENERAL_LOCK(&dhd->pub, flags);
	dhd->pub.dhd_bus_busy_state &= ~DHD_BUS_BUSY_IN_TX;
	dhd_os_busbusy_wake(&dhd->pub);
	DHD_GENERAL_UNLOCK(&dhd->pub, flags);
#endif /* PCIE_FULL_DONGLE */

	DHD_PERIM_UNLOCK_TRY(DHD_FWDER_UNIT(dhd), TRUE);
	DHD_OS_WAKE_UNLOCK(&dhd->pub);

	/* Return ok: we always eat the packet */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 20))
	return 0;
#else
	return NETDEV_TX_OK;
#endif
}


void
dhd_txflowcontrol(dhd_pub_t *dhdp, int ifidx, bool state)
{
	struct net_device *net;
	dhd_info_t *dhd = dhdp->info;
	int i = 0;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	ASSERT(dhd);

	if (ifidx == ALL_INTERFACES) {
		/* Flow control on all active interfaces */
		dhdp->txoff = state;
		for (i = 0; i < DHD_MAX_IFS; i++) {
#ifdef DHD_DEVWAKE_EARLY
			if (mutex_trylock(&dhd->ifdel_mutex[i])) {
#endif
				if (dhd->iflist[i]) {
					net = dhd->iflist[i]->net;
					if (net) {
						if (state == ON)
							netif_stop_queue(net);
						else
							netif_wake_queue(net);
					}
				}
#ifdef DHD_DEVWAKE_EARLY
				mutex_unlock(&dhd->ifdel_mutex[i]);
			} else {
				DHD_ERROR(("%s: intf(%d) is removing, don't allowed to access.\n",
					__FUNCTION__, i));
			}
#endif
		}
	}
	else {
#ifdef DHD_DEVWAKE_EARLY
		if (mutex_trylock(&dhd->ifdel_mutex[ifidx])) {
#endif
			if (dhd->iflist[ifidx]) {
				net = dhd->iflist[ifidx]->net;
				if (net) {
					if (state == ON)
						netif_stop_queue(net);
					else
						netif_wake_queue(net);
				}
			}
#ifdef DHD_DEVWAKE_EARLY
			mutex_unlock(&dhd->ifdel_mutex[ifidx]);
		} else {
			DHD_ERROR(("%s: intf(%d) is removing, don't allowed to access.\n",
				__FUNCTION__, i));
		}
#endif
	}
}

#ifdef DHD_RX_DUMP
typedef struct {
	uint16 type;
	const char *str;
} PKTTYPE_INFO;

static const PKTTYPE_INFO packet_type_info[] =
{
	{ ETHER_TYPE_IP, "IP" },
	{ ETHER_TYPE_ARP, "ARP" },
	{ ETHER_TYPE_BRCM, "BRCM" },
	{ ETHER_TYPE_802_1X, "802.1X" },
	{ ETHER_TYPE_WAI, "WAPI" },
	{ 0, ""}
};

static const char *_get_packet_type_str(uint16 type)
{
	int i;
	int n = sizeof(packet_type_info)/sizeof(packet_type_info[1]) - 1;

	for (i = 0; i < n; i++) {
		if (packet_type_info[i].type == type)
			return packet_type_info[i].str;
	}

	return packet_type_info[n].str;
}
#endif /* DHD_RX_DUMP */


#ifdef DHD_WMF
bool
dhd_is_rxthread_enabled(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd = dhdp->info;

	return dhd->rxthread_enabled;
}
#endif /* DHD_WMF */

void
dhd_rx_frame(dhd_pub_t *dhdp, int ifidx, void *pktbuf, int numpkt, uint8 chan,
	     int pkt_wake, wake_counts_t *wcp)
{
	dhd_info_t *dhd = (dhd_info_t *)dhdp->info;
	struct sk_buff *skb;
	uchar *eth;
	uint len;
	void *data, *pnext = NULL;
	int i;
	dhd_if_t *ifp;
	wl_event_msg_t event;
	int tout_rx = 0;
	int tout_ctrl = 0;
	void *skbhead = NULL;
	void *skbprev = NULL;
	uint16 protocol;
#if defined(DHD_RX_DUMP) || defined(DHD_8021X_DUMP) || defined(DHD_WAKE_STATUS)
	char *dump_data;
#endif /* DHD_RX_DUMP || DHD_8021X_DUMP || DHD_WAKE_STATUS */

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	for (i = 0; pktbuf && i < numpkt; i++, pktbuf = pnext) {
		struct ether_header *eh;

		pnext = PKTNEXT(dhdp->osh, pktbuf);
		PKTSETNEXT(dhdp->osh, pktbuf, NULL);

		ifp = dhd->iflist[ifidx];
		if (ifp == NULL) {
			DHD_ERROR(("%s: ifp is NULL. drop packet\n",
				__FUNCTION__));
			PKTCFREE(dhdp->osh, pktbuf, FALSE);
			continue;
		}

		eh = (struct ether_header *)PKTDATA(dhdp->osh, pktbuf);

		/* Dropping only data packets before registering net device to avoid kernel panic */
#ifndef PROP_TXSTATUS_VSDB
		if ((!ifp->net || ifp->net->reg_state != NETREG_REGISTERED) &&
			(ntoh16(eh->ether_type) != ETHER_TYPE_BRCM)) {
#else
		if ((!ifp->net || ifp->net->reg_state != NETREG_REGISTERED || !dhd->pub.up) &&
			(ntoh16(eh->ether_type) != ETHER_TYPE_BRCM)) {
#endif /* PROP_TXSTATUS_VSDB */
			DHD_ERROR(("%s: net device is NOT registered yet. drop packet\n",
			__FUNCTION__));
			PKTCFREE(dhdp->osh, pktbuf, FALSE);
			continue;
		}


#ifdef PROP_TXSTATUS
		if (dhd_wlfc_is_header_only_pkt(dhdp, pktbuf)) {
			/* WLFC may send header only packet when
			there is an urgent message but no packet to
			piggy-back on
			*/
			PKTCFREE(dhdp->osh, pktbuf, FALSE);
			continue;
		}
#endif
#ifdef DHD_L2_FILTER
		/* If block_ping is enabled drop the ping packet */
		if (dhdp->block_ping) {
			if (dhd_l2_filter_block_ping(dhdp, pktbuf, ifidx) == BCME_OK) {
				PKTFREE(dhdp->osh, pktbuf, FALSE);
				continue;
			}
		}
#endif
#ifdef DHD_WMF
		/* WMF processing for multicast packets */
		if (ifp->wmf.wmf_enable && (ETHER_ISMULTI(eh->ether_dhost))) {
			dhd_sta_t *sta;
			int ret;

			sta = dhd_find_sta(dhdp, ifidx, (void *)eh->ether_shost);
			ret = dhd_wmf_packets_handle(dhdp, pktbuf, sta, ifidx, 1);
			switch (ret) {
				case WMF_TAKEN:
					/* The packet is taken by WMF. Continue to next iteration */
					continue;
				case WMF_DROP:
					/* Packet DROP decision by WMF. Toss it */
					DHD_ERROR(("%s: WMF decides to drop packet\n",
						__FUNCTION__));
					PKTCFREE(dhdp->osh, pktbuf, FALSE);
					continue;
				default:
					/* Continue the transmit path */
					break;
			}
		}
#endif /* DHD_WMF */
#ifdef DHDTCPACK_SUPPRESS
		dhd_tcpdata_info_get(dhdp, pktbuf);
#endif
		skb = PKTTONATIVE(dhdp->osh, pktbuf);

		ifp = dhd->iflist[ifidx];
		if (ifp == NULL)
			ifp = dhd->iflist[0];

		ASSERT(ifp);
		skb->dev = ifp->net;

#ifdef PCIE_FULL_DONGLE
		if ((DHD_IF_ROLE_AP(dhdp, ifidx) || DHD_IF_ROLE_P2PGO(dhdp, ifidx)) &&
			(!ifp->ap_isolate)) {
			eh = (struct ether_header *)PKTDATA(dhdp->osh, pktbuf);
			if (ETHER_ISUCAST(eh->ether_dhost)) {
				if (dhd_find_sta(dhdp, ifidx, (void *)eh->ether_dhost)) {
						dhd_sendpkt(dhdp, ifidx, pktbuf);
					continue;
				}
			} else {
				void *npktbuf = PKTDUP(dhdp->osh, pktbuf);
#ifndef HW_PCIE_STABILITY
				dhd_sendpkt(dhdp, ifidx, npktbuf);
#else
				if(npktbuf) {
					dhd_sendpkt(dhdp, ifidx, npktbuf);
				} else {
					PKTFREE(dhdp->osh, pktbuf, FALSE);
					continue;
				}
#endif
			}
		}
#endif /* PCIE_FULL_DONGLE */

		/* Get the protocol, maintain skb around eth_type_trans()
		 * The main reason for this hack is for the limitation of
		 * Linux 2.4 where 'eth_type_trans' uses the 'net->hard_header_len'
		 * to perform skb_pull inside vs ETH_HLEN. Since to avoid
		 * coping of the packet coming from the network stack to add
		 * BDC, Hardware header etc, during network interface registration
		 * we set the 'net->hard_header_len' to ETH_HLEN + extra space required
		 * for BDC, Hardware header etc. and not just the ETH_HLEN
		 */
		eth = skb->data;
		len = skb->len;
		protocol = (skb->data[12] << 8) | skb->data[13];

		if (protocol == ETHER_TYPE_802_1X) {
			DBG_EVENT_LOG(dhdp, WIFI_EVENT_DRIVER_EAPOL_FRAME_RECEIVED);
		}
#if defined(DHD_RX_DUMP) || defined(DHD_8021X_DUMP) || defined(DHD_DHCP_DUMP) \
	|| defined(DHD_WAKE_STATUS)
		dump_data = skb->data;
#endif /* DHD_RX_DUMP || DHD_8021X_DUMP || DHD_DHCP_DUMP */
#ifdef DHD_8021X_DUMP
		if (protocol == ETHER_TYPE_802_1X) {
			DHD_ERROR(("ETHER_TYPE_802_1X [RX]: "
				"ver %d, type %d, replay %d\n",
				dump_data[14], dump_data[15],
				dump_data[30]));
		}
#endif /* DHD_8021X_DUMP */
#ifdef HW_CHINAMOBILE_SPEEDLIMIT
		if(early_suspended) {
		    if (protocol == ETHER_TYPE_IP) {
		          recv_pkt++;
		       if (recv_pkt >= 100) {
		          recv_pkt = 0;
		          osl_sleep(100);
		       }
		    }
		}
#endif
#ifdef DHD_DHCP_DUMP
		if (protocol != ETHER_TYPE_BRCM && protocol == ETHER_TYPE_IP) {
			uint16 dump_hex;
			uint16 source_port;
			uint16 dest_port;
			uint16 udp_port_pos;
			uint8 *ptr8 = (uint8 *)&dump_data[ETHER_HDR_LEN];
			uint8 ip_header_len = (*ptr8 & 0x0f)<<2;

			udp_port_pos = ETHER_HDR_LEN + ip_header_len;
			source_port = (dump_data[udp_port_pos] << 8) | dump_data[udp_port_pos+1];
			dest_port = (dump_data[udp_port_pos+2] << 8) | dump_data[udp_port_pos+3];
			if (source_port == 0x0044 || dest_port == 0x0044) {
				dump_hex = (dump_data[udp_port_pos+249] << 8) |
					dump_data[udp_port_pos+250];
				if (dump_hex == 0x0101) {
					DHD_ERROR(("DHCP - DISCOVER [RX]\n"));
				} else if (dump_hex == 0x0102) {
					DHD_ERROR(("DHCP - OFFER [RX]\n"));
				} else if (dump_hex == 0x0103) {
					DHD_ERROR(("DHCP - REQUEST [RX]\n"));
				} else if (dump_hex == 0x0105) {
					DHD_ERROR(("DHCP - ACK [RX]\n"));
				} else {
					DHD_ERROR(("DHCP - 0x%X [RX]\n", dump_hex));
				}
			} else if (source_port == 0x0043 || dest_port == 0x0043) {
				DHD_ERROR(("DHCP - BOOTP [RX]\n"));
			}
		}
#endif /* DHD_DHCP_DUMP */
#if defined(DHD_RX_DUMP)
		DHD_ERROR(("RX DUMP - %s\n", _get_packet_type_str(protocol)));
		if (protocol != ETHER_TYPE_BRCM) {
			if (dump_data[0] == 0xFF) {
				DHD_ERROR(("%s: BROADCAST\n", __FUNCTION__));

				if ((dump_data[12] == 8) &&
					(dump_data[13] == 6)) {
					DHD_ERROR(("%s: ARP %d\n",
						__FUNCTION__, dump_data[0x15]));
				}
			} else if (dump_data[0] & 1) {
				DHD_ERROR(("%s: MULTICAST: " MACDBG "\n",
					__FUNCTION__, MAC2STRDBG(dump_data)));
			}
#ifdef DHD_RX_FULL_DUMP
			{
				int k;
				for (k = 0; k < skb->len; k++) {
					DHD_ERROR(("%02X ", dump_data[k]));
					if ((k & 15) == 15)
						DHD_ERROR(("\n"));
				}
				DHD_ERROR(("\n"));
			}
#endif /* DHD_RX_FULL_DUMP */
		}
#endif /* DHD_RX_DUMP */

		skb->protocol = eth_type_trans(skb, skb->dev);

		if (skb->pkt_type == PACKET_MULTICAST) {
			dhd->pub.rx_multicast++;
			ifp->stats.multicast++;
		}

		skb->data = eth;
		skb->len = len;

#ifdef WLMEDIA_HTSF
		dhd_htsf_addrxts(dhdp, pktbuf);
#endif
		/* Strip header, count, deliver upward */
		skb_pull(skb, ETH_HLEN);

		/* Process special event packets and then discard them */
		memset(&event, 0, sizeof(event));
		if (ntoh16(skb->protocol) == ETHER_TYPE_BRCM) {
#ifdef BCM_PATCH_CVE_2016_0801
			int ret_event;

			ret_event = dhd_wl_host_event(dhd, &ifidx,
#else
			dhd_wl_host_event(dhd, &ifidx,
#endif /* BCM_PATCH_CVE_2016_0801 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 22)
			skb_mac_header(skb),
#else
			skb->mac.raw,
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 22) */
#if defined (BCM_PATCH_FOR_ETHERTYPE_SECURITY)
#ifdef BCM_PATCH_CVE_2016_0801
			len,
#else
            len - 2,
#endif /* BCM_PATCH_CVE_2016_0801 */
#endif /*BCM_PATCH_FOR_ETHERTYPE_SECURITY*/
			&event,
			&data);
#ifdef BCM_PATCH_CVE_2016_0801
#if defined (BCM_PATCH_FOR_ETHERTYPE_SECURITY)
			if (ret_event != BCME_OK) {
#ifdef DHD_USE_STATIC_CTRLBUF
				PKTFREE_STATIC(dhdp->osh, pktbuf, FALSE);
#else
				PKTFREE(dhdp->osh, pktbuf, FALSE);
#endif /* DHD_USE_STATIC_CTRLBUF */
				continue;
			}
#endif /*BCM_PATCH_FOR_ETHERTYPE_SECURITY*/
#endif /* BCM_PATCH_CVE_2016_0801 */
			wl_event_to_host_order(&event);
			if (!tout_ctrl)
				tout_ctrl = DHD_PACKET_TIMEOUT_MS;
#ifdef HW_WIFI_WAKEUP_SRC_PARSE
			if(g_wifi_firstwake){
				printk(WIFI_WAKESRC_TAG"event type:%d, flags:%x, status:%x\n",
					event.event_type, event.flags, event.status);
				g_wifi_firstwake = FALSE;
			}
#endif

#if defined(PNO_SUPPORT)
			if (event.event_type == WLC_E_PFN_NET_FOUND) {
				/* enforce custom wake lock to garantee that Kernel not suspended */
				tout_ctrl = CUSTOM_PNO_EVENT_LOCK_xTIME * DHD_PACKET_TIMEOUT_MS;
			}
#endif /* PNO_SUPPORT */

#ifdef DHD_WAKE_STATUS
			if (unlikely(pkt_wake)) {
				wcp->rcwake++;
#ifdef DHD_WAKE_EVENT_STATUS
				if (event.event_type < WLC_E_LAST)
					wcp->rc_event[event.event_type]++;
#endif
				pkt_wake = 0;
			}
#endif

#ifdef DHD_DONOT_FORWARD_BCMEVENT_AS_NETWORK_PKT
#ifdef DHD_USE_STATIC_CTRLBUF
			PKTFREE_STATIC(dhdp->osh, pktbuf, FALSE);
#else
			PKTFREE(dhdp->osh, pktbuf, FALSE);
#endif /* DHD_USE_STATIC_CTRLBUF */
			continue;
#endif /* DHD_DONOT_FORWARD_BCMEVENT_AS_NETWORK_PKT */
		} else {
			if (dhd_rx_suspend_again(skb) != 0) {
				if (skb->dev->ieee80211_ptr && skb->dev->ieee80211_ptr->ps == false)
					tout_rx = CUSTOM_DHCP_LOCK_xTIME * DHD_PACKET_TIMEOUT_MS;
				else
					tout_rx = DHD_PACKET_TIMEOUT_MS;
			}
#ifdef PROP_TXSTATUS
			dhd_wlfc_save_rxpath_ac_time(dhdp, (uint8)PKTPRIO(skb));
#endif /* PROP_TXSTATUS */

#ifdef DHD_WAKE_STATUS
			if (unlikely(pkt_wake)) {
				wcp->rxwake++;
#ifdef DHD_WAKE_RX_STATUS
#define ETHER_ICMP6_HEADER	20
#define ETHER_IPV6_SADDR (ETHER_ICMP6_HEADER + 2)
#define ETHER_IPV6_DAADR (ETHER_IPV6_SADDR + IPV6_ADDR_LEN)
#define ETHER_ICMPV6_TYPE (ETHER_IPV6_DAADR + IPV6_ADDR_LEN)
				if (ntoh16(skb->protocol) == ETHER_TYPE_ARP) /* ARP */
					wcp->rx_arp++;
				if (dump_data[0] == 0xFF) { /* Broadcast */
					wcp->rx_bcast++;
				} else if (dump_data[0] & 0x01) { /* Multicast */
					wcp->rx_mcast++;
					if (ntoh16(skb->protocol) == ETHER_TYPE_IPV6) {
						wcp->rx_multi_ipv6++;
						if ((skb->len > ETHER_ICMP6_HEADER) &&
						    (dump_data[ETHER_ICMP6_HEADER] == IPPROTO_ICMPV6)) {
							wcp->rx_icmpv6++;
							if (skb->len > ETHER_ICMPV6_TYPE) {
								switch (dump_data[ETHER_ICMPV6_TYPE]) {
								case NDISC_ROUTER_ADVERTISEMENT:
									wcp->rx_icmpv6_ra++;
									break;
								case NDISC_NEIGHBOUR_ADVERTISEMENT:
									wcp->rx_icmpv6_na++;
									break;
								case NDISC_NEIGHBOUR_SOLICITATION:
									wcp->rx_icmpv6_ns++;
									break;
								}
							}
						}
					} else if (dump_data[2] == 0x5E) {
						wcp->rx_multi_ipv4++;
					} else {
						wcp->rx_multi_other++;
					}
				} else { /* Unicast */
					wcp->rx_ucast++;
				}
#undef ETHER_ICMP6_HEADER
#undef ETHER_IPV6_SADDR
#undef ETHER_IPV6_DAADR
#undef ETHER_ICMPV6_TYPE
#endif
				pkt_wake = 0;
			}
#endif
		}

		ASSERT(ifidx < DHD_MAX_IFS && dhd->iflist[ifidx]);
		ifp = dhd->iflist[ifidx];

		if (ifp->net)
			ifp->net->last_rx = jiffies;

		if (ntoh16(skb->protocol) != ETHER_TYPE_BRCM) {
			dhdp->dstats.rx_bytes += skb->len;
			dhdp->rx_packets++; /* Local count */
			ifp->stats.rx_bytes += skb->len;
			ifp->stats.rx_packets++;
		}

#ifdef HW_WIFI_WAKEUP_SRC_PARSE
		if(g_wifi_firstwake){
			parse_packet(skb);
			g_wifi_firstwake = FALSE;
		}
#endif
#ifdef HW_DNS_DHCP_PARSE
		if (ntoh16(skb->protocol) == ETH_P_IP) {
			hw_parse_special_ipv4_packet(skb->data - ETH_HLEN, skb->len + ETH_HLEN);
		}
#endif
		if (in_interrupt()) {
			netif_rx(skb);
		} else {
			if (dhd->rxthread_enabled) {
				if (!skbhead)
					skbhead = skb;
				else
					PKTSETNEXT(dhdp->osh, skbprev, skb);
				skbprev = skb;
			} else {

				/* If the receive is not processed inside an ISR,
				 * the softirqd must be woken explicitly to service
				 * the NET_RX_SOFTIRQ.	In 2.6 kernels, this is handled
				 * by netif_rx_ni(), but in earlier kernels, we need
				 * to do it manually.
				 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
				netif_rx_ni(skb);
#else
				ulong flags;
				netif_rx(skb);
				local_irq_save(flags);
				RAISE_RX_SOFTIRQ();
				local_irq_restore(flags);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0) */
			}
		}
	}

	if (dhd->rxthread_enabled && skbhead)
		dhd_sched_rxf(dhdp, skbhead);

	DHD_OS_WAKE_LOCK_RX_TIMEOUT_ENABLE(dhdp, tout_rx);
	DHD_OS_WAKE_LOCK_CTRL_TIMEOUT_ENABLE(dhdp, tout_ctrl);
#ifdef PCIE_FULL_DONGLE	
	DHD_OS_WAKE_LOCK_TIMEOUT(dhdp);
#endif
#ifdef CONFIG_PARTIALRESUME
	if (tout_rx || tout_ctrl)
		wifi_process_partial_resume(dhd->adapter,
					    WIFI_PR_VOTE_FOR_RESUME);
#endif
}

void
dhd_event(struct dhd_info *dhd, char *evpkt, int evlen, int ifidx)
{
	/* Linux version has nothing to do */
	return;
}

void
dhd_txcomplete(dhd_pub_t *dhdp, void *txp, bool success)
{
	dhd_info_t *dhd = (dhd_info_t *)(dhdp->info);
	struct ether_header *eh;
	uint16 type;

	dhd_prot_hdrpull(dhdp, NULL, txp, NULL, NULL);

	eh = (struct ether_header *)PKTDATA(dhdp->osh, txp);
	type  = ntoh16(eh->ether_type);

	if (type == ETHER_TYPE_802_1X)
		atomic_dec(&dhd->pend_8021x_cnt);

}

static struct net_device_stats *
dhd_get_stats(struct net_device *net)
{
	dhd_info_t *dhd = DHD_DEV_INFO(net);
	dhd_if_t *ifp;
	int ifidx;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	ifidx = dhd_net2idx(dhd, net);
	if (ifidx == DHD_BAD_IF) {
		DHD_ERROR(("%s: BAD_IF\n", __FUNCTION__));

		memset(&net->stats, 0, sizeof(net->stats));
		return &net->stats;
	}

	ifp = dhd->iflist[ifidx];
	ASSERT(dhd && ifp);

	if (dhd->pub.up) {
		/* Use the protocol to get dongle stats */
		dhd_prot_dstats(&dhd->pub);
	}
	return &ifp->stats;
}

static int
dhd_watchdog_thread(void *data)
{
	tsk_ctl_t *tsk = (tsk_ctl_t *)data;
	dhd_info_t *dhd = (dhd_info_t *)tsk->parent;
	/* This thread doesn't need any user-level access,
	 * so get rid of all our resources
	 */
	if (dhd_watchdog_prio > 0) {
		struct sched_param param;
		param.sched_priority = (dhd_watchdog_prio < MAX_RT_PRIO)?
			dhd_watchdog_prio:(MAX_RT_PRIO-1);
		setScheduler(current, SCHED_FIFO, &param);
	}

	while (1)
		if (down_interruptible (&tsk->sema) == 0) {
#ifndef PCIE_FULL_DONGLE
			unsigned long flags;
#endif
			unsigned long jiffies_at_start = jiffies;
			unsigned long time_lapse;

			SMP_RD_BARRIER_DEPENDS();
			if (tsk->terminated) {
				break;
			}

			if (dhd->pub.dongle_reset == FALSE) {
				DHD_TIMER(("%s:\n", __FUNCTION__));

				/* Call the bus module watchdog */
				if (dhd->pub.up)
					dhd_bus_watchdog(&dhd->pub);
#ifndef PCIE_FULL_DONGLE
				DHD_GENERAL_LOCK(&dhd->pub, flags);
#endif
				/* Count the tick for reference */
				dhd->pub.tickcnt++;
				time_lapse = jiffies - jiffies_at_start;

				/* Reschedule the watchdog */
				if (dhd->wd_timer_valid) {
					mod_timer(&dhd->timer,
					    jiffies +
					    msecs_to_jiffies(dhd_watchdog_ms) -
					    min(msecs_to_jiffies(dhd_watchdog_ms), time_lapse));
				}
#ifndef PCIE_FULL_DONGLE
				DHD_GENERAL_UNLOCK(&dhd->pub, flags);
#endif
			}
		} else {
			break;
	}

	complete_and_exit(&tsk->completed, 0);
}

static void dhd_watchdog(ulong data)
{
	dhd_info_t *dhd = (dhd_info_t *)data;
	unsigned long flags;

	if (dhd->pub.dongle_reset) {
		return;
	}
#ifdef PCIE_FULL_DONGLE
	DHD_GENERAL_LOCK(&dhd->pub, flags);
	if (dhd->pub.busstate == DHD_BUS_SUSPEND) {
		DHD_ERROR(("%s wd while suspend in progress \n", __FUNCTION__));
		DHD_GENERAL_UNLOCK(&dhd->pub, flags);
		return;
	}
	DHD_GENERAL_UNLOCK(&dhd->pub, flags);
#endif /* PCIE_FULL_DONGLE */

	if (dhd->thr_wdt_ctl.thr_pid >= 0) {
		up(&dhd->thr_wdt_ctl.sema);
		return;
	}

	/* Call the bus module watchdog */
	if (dhd->pub.up)
		dhd_bus_watchdog(&dhd->pub);

	DHD_GENERAL_LOCK(&dhd->pub, flags);
	/* Count the tick for reference */
	dhd->pub.tickcnt++;

	/* Reschedule the watchdog */
	if (dhd->wd_timer_valid)
		mod_timer(&dhd->timer, jiffies + msecs_to_jiffies(dhd_watchdog_ms));
	DHD_GENERAL_UNLOCK(&dhd->pub, flags);

}

#ifdef ENABLE_ADAPTIVE_SCHED
static void
dhd_sched_policy(int prio)
{
	struct sched_param param;
	if (cpufreq_quick_get(0) <= CUSTOM_CPUFREQ_THRESH) {
		param.sched_priority = 0;
		setScheduler(current, SCHED_NORMAL, &param);
	} else {
		if (get_scheduler_policy(current) != SCHED_FIFO) {
			param.sched_priority = (prio < MAX_RT_PRIO)? prio : (MAX_RT_PRIO-1);
			setScheduler(current, SCHED_FIFO, &param);
		}
	}
}
#endif /* ENABLE_ADAPTIVE_SCHED */
#ifdef DEBUG_CPU_FREQ
static int dhd_cpufreq_notifier(struct notifier_block *nb, unsigned long val, void *data)
{
	dhd_info_t *dhd = container_of(nb, struct dhd_info, freq_trans);
	struct cpufreq_freqs *freq = data;
	if (dhd) {
		if (!dhd->new_freq)
			goto exit;
		if (val == CPUFREQ_POSTCHANGE) {
			DHD_ERROR(("cpu freq is changed to %u kHZ on CPU %d\n",
				freq->new, freq->cpu));
			*per_cpu_ptr(dhd->new_freq, freq->cpu) = freq->new;
		}
	}
exit:
	return 0;
}
#endif /* DEBUG_CPU_FREQ */
static int
dhd_dpc_thread(void *data)
{
	tsk_ctl_t *tsk = (tsk_ctl_t *)data;
	dhd_info_t *dhd = (dhd_info_t *)tsk->parent;

	/* This thread doesn't need any user-level access,
	 * so get rid of all our resources
	 */
	if (dhd_dpc_prio > 0)
	{
		struct sched_param param;
		param.sched_priority = (dhd_dpc_prio < MAX_RT_PRIO)?dhd_dpc_prio:(MAX_RT_PRIO-1);
		setScheduler(current, SCHED_FIFO, &param);
	}

#ifdef CUSTOM_DPC_CPUCORE
	set_cpus_allowed_ptr(current, cpumask_of(CUSTOM_DPC_CPUCORE));
#endif
#ifdef CUSTOM_SET_CPUCORE
	dhd->pub.current_dpc = current;
#endif /* CUSTOM_SET_CPUCORE */

	/* Run until signal received */
	while (1) {
		if (!binary_sema_down(tsk)) {
#ifdef ENABLE_ADAPTIVE_SCHED
			dhd_sched_policy(dhd_dpc_prio);
#endif /* ENABLE_ADAPTIVE_SCHED */
			SMP_RD_BARRIER_DEPENDS();
			if (tsk->terminated) {
				break;
			}
#ifdef HW_SDIO_DPC_SUSPEND
			if(hw_skip_dpc_in_suspend()) {
				DHD_OS_WAKE_UNLOCK(&dhd->pub);
				continue;
			}
#endif
			/* Call bus dpc unless it indicated down (then clean stop) */
			if (dhd->pub.busstate != DHD_BUS_DOWN) {
#ifndef PCIE_FULL_DONGLE
				dhd_os_wd_timer_extend(&dhd->pub, TRUE);
#endif
				while (dhd_bus_dpc(dhd->pub.bus)) {
					/* process all data */
				}
#ifndef PCIE_FULL_DONGLE
				dhd_os_wd_timer_extend(&dhd->pub, FALSE);
#endif
				DHD_OS_WAKE_UNLOCK(&dhd->pub);

			} else {
				if (dhd->pub.up)
					dhd_bus_stop(dhd->pub.bus, TRUE);
				DHD_OS_WAKE_UNLOCK(&dhd->pub);
			}
		}
		else
			break;
	}

	complete_and_exit(&tsk->completed, 0);
}
#ifdef BCMSDIO
#define HW_SDIO_FREQ_ADJUST_PEROID     2000
#define PRIMARY_CPUCORE 0
#define DPC_CPUCORE 4
#define DHD_TPUT_THRESHOLD (150*1024*1024/8) // 150Mbps
extern int brcm_mmc_irq;
void
dhdsdio_interrupt_set_cpucore(dhd_info_t *dhd, bool set)
{
	struct cpumask rxf_cpu_mask;
	struct cpumask irq_cpu_mask;
	int dpc_cpumask = PRIMARY_CPUCORE;
	if (dhd == NULL)
	{
		return;
	}
	if (set)
	{
		cpumask_setall(&rxf_cpu_mask);
		/* set wifi rxf thread to CPU5~7  dpc&irq to cpu 4*/
		cpumask_clear_cpu(0, &rxf_cpu_mask);
		cpumask_clear_cpu(1, &rxf_cpu_mask);
		cpumask_clear_cpu(2, &rxf_cpu_mask);
		cpumask_clear_cpu(3, &rxf_cpu_mask);
		cpumask_clear_cpu(4, &rxf_cpu_mask);
		dpc_cpumask = DPC_CPUCORE;
		irq_cpu_mask = *cpumask_of(DPC_CPUCORE);
	}
	else
	{
		cpumask_setall(&rxf_cpu_mask);
		cpumask_setall(&irq_cpu_mask);
		/* set wifi rx thread to CPU1~3   dpc to cpu 0 irq to cpu0~7*/
		cpumask_clear_cpu(0, &rxf_cpu_mask);
		cpumask_clear_cpu(4, &rxf_cpu_mask);
		cpumask_clear_cpu(5, &rxf_cpu_mask);
		cpumask_clear_cpu(6, &rxf_cpu_mask);
		cpumask_clear_cpu(7, &rxf_cpu_mask);
		dpc_cpumask = PRIMARY_CPUCORE;
	}
	set_cpus_allowed_ptr(dhd->thr_rxf_ctl.p_task, &rxf_cpu_mask);
	set_cpus_allowed_ptr(dhd->thr_dpc_ctl.p_task, cpumask_of(dpc_cpumask));
	irq_set_affinity_hint(brcm_mmc_irq, &irq_cpu_mask);
	return;
}
#endif
static int
dhd_rxf_thread(void *data)
{
	tsk_ctl_t *tsk = (tsk_ctl_t *)data;
	dhd_info_t *dhd = (dhd_info_t *)tsk->parent;
#if defined(WAIT_DEQUEUE)
#define RXF_WATCHDOG_TIME 250 /* BARK_TIME(1000) /  */
	ulong watchdogTime = OSL_SYSUPTIME(); /* msec */
#endif
	dhd_pub_t *pub = &dhd->pub;
#ifdef BCMSDIO
	dhd_if_t *ifp = dhd->iflist[0];
	ulong rx_bytes = 0;
 	ulong pre_rx_bytes = 0;
	ulong pre_jiffies = 0;
	ulong passed_msec = 0;
	bool flag = FALSE;
#endif
	/* This thread doesn't need any user-level access,
	 * so get rid of all our resources
	 */
	if (dhd_rxf_prio > 0)
	{
		struct sched_param param;
		param.sched_priority = (dhd_rxf_prio < MAX_RT_PRIO)?dhd_rxf_prio:(MAX_RT_PRIO-1);
		setScheduler(current, SCHED_FIFO, &param);
	}

	DAEMONIZE("dhd_rxf");
	/* DHD_OS_WAKE_LOCK is called in dhd_sched_dpc[dhd_linux.c] down below  */

	/*  signal: thread has started */
	complete(&tsk->completed);
#ifdef CUSTOM_SET_CPUCORE
	dhd->pub.current_rxf = current;
#endif /* CUSTOM_SET_CPUCORE */

	/* Run until signal received */
	while (1) {
		if (down_interruptible(&tsk->sema) == 0) {
			void *skb;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
			ulong flags;
#endif
#ifdef ENABLE_ADAPTIVE_SCHED
			dhd_sched_policy(dhd_rxf_prio);
#endif /* ENABLE_ADAPTIVE_SCHED */

			SMP_RD_BARRIER_DEPENDS();

			if (tsk->terminated) {
				break;
			}
#ifdef BCMSDIO
			passed_msec = jiffies_to_msecs(jiffies - pre_jiffies);
			if (ifp && (passed_msec >= HW_SDIO_FREQ_ADJUST_PEROID)) {
			    pre_jiffies = jiffies;
				rx_bytes = ifp->stats.rx_bytes;
				if ((rx_bytes - pre_rx_bytes) >=
					((ulong)DHD_TPUT_THRESHOLD * passed_msec / 1000)){
					if (!flag){
						dhdsdio_interrupt_set_cpucore(dhd,true);
						flag = TRUE;
					}
				}
				else
				{
					if (flag){
						dhdsdio_interrupt_set_cpucore(dhd,false);
						flag = FALSE;
					}
				}
				pre_rx_bytes = rx_bytes;
			}
#endif
			skb = dhd_rxf_dequeue(pub);

			if (skb == NULL) {
				continue;
			}
			while (skb) {
				void *skbnext = PKTNEXT(pub->osh, skb);
				PKTSETNEXT(pub->osh, skb, NULL);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
				netif_rx_ni(skb);
#else
				netif_rx(skb);
				local_irq_save(flags);
				RAISE_RX_SOFTIRQ();
				local_irq_restore(flags);

#endif
				skb = skbnext;
			}
#if defined(WAIT_DEQUEUE)
			if (OSL_SYSUPTIME() - watchdogTime > RXF_WATCHDOG_TIME) {
				OSL_SLEEP(1);
				watchdogTime = OSL_SYSUPTIME();
			}
#endif

			DHD_OS_WAKE_UNLOCK(pub);
		}
		else
			break;
	}

	complete_and_exit(&tsk->completed, 0);
}

#ifdef BCMPCIE
void dhd_dpc_enable(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd;

	if (!dhdp || !dhdp->info)
		return;
	dhd = dhdp->info;

	if (atomic_read(&dhd->tasklet.count) ==  1)
		tasklet_enable(&dhd->tasklet);
}
#endif /* BCMPCIE */

#ifdef BCMPCIE
void dhd_dpc_kill(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd;

	if (!dhdp)
		return;

	dhd = dhdp->info;

	if(!dhd)
		return;

	tasklet_kill(&dhd->tasklet);
	DHD_ERROR(("%s: tasklet disabled\n",__FUNCTION__));
}

void
dhd_dpc_tasklet_kill(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd;

	if (!dhdp) {
		return;
	}

	dhd = dhdp->info;

	if (!dhd) {
		return;
	}

	if (dhd->thr_dpc_ctl.thr_pid < 0) {
		tasklet_kill(&dhd->tasklet);
	}
}
#endif
#ifndef BCM_PCIE_UPDATE
static int isresched = 0;
#endif
static void
dhd_dpc(ulong data)
{
	dhd_info_t *dhd;

	dhd = (dhd_info_t *)data;

	/* this (tasklet) can be scheduled in dhd_sched_dpc[dhd_linux.c]
	 * down below , wake lock is set,
	 * the tasklet is initialized in dhd_attach()
	 */
	/* Call bus dpc unless it indicated down (then clean stop) */
#ifndef BCM_PCIE_UPDATE
	if (dhd->pub.busstate != DHD_BUS_DOWN) {
		isresched = dhd_bus_dpc(dhd->pub.bus);
		if (isresched)
			tasklet_schedule(&dhd->tasklet);
		else
			DHD_OS_WAKE_UNLOCK(&dhd->pub);
	} else {
		dhd_bus_stop(dhd->pub.bus, TRUE);
		DHD_OS_WAKE_UNLOCK(&dhd->pub);
	}
#else
	if (dhd->pub.busstate != DHD_BUS_DOWN) {
		if (dhd_bus_dpc(dhd->pub.bus)) {
			tasklet_schedule(&dhd->tasklet);
		}
	} else {
		dhd_bus_stop(dhd->pub.bus, TRUE);
	}
#endif
}

void
dhd_sched_dpc(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd = (dhd_info_t *)dhdp->info;

	if (dhd->thr_dpc_ctl.thr_pid >= 0) {
		DHD_OS_WAKE_LOCK(dhdp);
		/* If the semaphore does not get up,
		* wake unlock should be done here
		*/
		if (!binary_sema_up(&dhd->thr_dpc_ctl)) {
			DHD_OS_WAKE_UNLOCK(dhdp);
		}
		return;
	} else {
#ifndef BCM_PCIE_UPDATE
		if (!test_bit(TASKLET_STATE_SCHED, &dhd->tasklet.state) && !isresched) {
			DHD_OS_WAKE_LOCK(dhdp);
			tasklet_schedule(&dhd->tasklet);
		}
#else
		tasklet_schedule(&dhd->tasklet);
#endif
	}
}

static void
dhd_sched_rxf(dhd_pub_t *dhdp, void *skb)
{
	dhd_info_t *dhd = (dhd_info_t *)dhdp->info;
#ifdef RXF_DEQUEUE_ON_BUSY
	int ret = BCME_OK;
	int retry = 2;
#endif /* RXF_DEQUEUE_ON_BUSY */

	DHD_OS_WAKE_LOCK(dhdp);

	DHD_TRACE(("dhd_sched_rxf: Enter\n"));
#ifdef RXF_DEQUEUE_ON_BUSY
	do {
		ret = dhd_rxf_enqueue(dhdp, skb);
		if (ret == BCME_OK || ret == BCME_ERROR)
			break;
		else
			OSL_SLEEP(50); /* waiting for dequeueing */
	} while (retry-- > 0);

	if (retry <= 0 && ret == BCME_BUSY) {
		void *skbp = skb;

		while (skbp) {
			void *skbnext = PKTNEXT(dhdp->osh, skbp);
			PKTSETNEXT(dhdp->osh, skbp, NULL);
			netif_rx_ni(skbp);
			skbp = skbnext;
		}
		DHD_ERROR(("send skb to kernel backlog without rxf_thread\n"));
	}
	else {
		if (dhd->thr_rxf_ctl.thr_pid >= 0) {
			up(&dhd->thr_rxf_ctl.sema);
		}
	}
#else /* RXF_DEQUEUE_ON_BUSY */
	do {
		if (dhd_rxf_enqueue(dhdp, skb) == BCME_OK)
			break;
	} while (1);
	if (dhd->thr_rxf_ctl.thr_pid >= 0) {
		up(&dhd->thr_rxf_ctl.sema);
	}
	return;
#endif /* RXF_DEQUEUE_ON_BUSY */
}

#ifdef TOE
/* Retrieve current toe component enables, which are kept as a bitmap in toe_ol iovar */
static int
dhd_toe_get(dhd_info_t *dhd, int ifidx, uint32 *toe_ol)
{
	char buf[32];
	int ret;

	memset(buf, 0, sizeof(buf));
	ret = dhd_iovar(&dhd->pub, ifidx, "toe_ol", NULL, 0, (char *)&buf,
			sizeof(buf), FALSE);
	if (ret == -EIO) {
		DHD_ERROR(("%s: toe not supported by device\n",
			   dhd_ifname(&dhd->pub, ifidx)));
		return -EOPNOTSUPP;
	} else if (ret < 0) {
		DHD_INFO(("%s: could not get toe_ol: ret=%d\n", dhd_ifname(&dhd->pub, ifidx), ret));
		return ret;
	}

	memcpy(toe_ol, buf, sizeof(uint32));
	return 0;
}

/* Set current toe component enables in toe_ol iovar, and set toe global enable iovar */
static int
dhd_toe_set(dhd_info_t *dhd, int ifidx, uint32 toe_ol)
{
	int toe, ret;

	/* Set toe_ol as requested */
	ret = dhd_iovar(&dhd->pub, ifidx, "toe_ol", (char *)&toe_ol,
			sizeof(toe_ol), NULL, 0, TRUE);
	if (ret < 0) {
		DHD_ERROR(("%s: could not set toe_ol: ret=%d\n",
			dhd_ifname(&dhd->pub, ifidx), ret));
		return ret;
	}

	/* Enable toe globally only if any components are enabled. */
	toe = (toe_ol != 0);
	ret = dhd_iovar(&dhd->pub, ifidx, "toe", (char *)&toe, sizeof(toe),
			NULL, 0, TRUE);
	if (ret < 0) {
		DHD_ERROR(("%s: could not set toe: ret=%d\n", dhd_ifname(&dhd->pub, ifidx), ret));
		return ret;
	}

	return 0;
}
#endif /* TOE */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
static void
dhd_ethtool_get_drvinfo(struct net_device *net, struct ethtool_drvinfo *info)
{
	dhd_info_t *dhd = DHD_DEV_INFO(net);

	snprintf(info->driver, sizeof(info->driver), "wl");
	snprintf(info->version, sizeof(info->version), "%lu", dhd->pub.drv_version);
}

struct ethtool_ops dhd_ethtool_ops = {
	.get_drvinfo = dhd_ethtool_get_drvinfo
};
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24) */


#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 4, 2)
static int
dhd_ethtool(dhd_info_t *dhd, void *uaddr)
{
	struct ethtool_drvinfo info;
	char drvname[sizeof(info.driver)];
	uint32 cmd;
#ifdef TOE
	struct ethtool_value edata;
	uint32 toe_cmpnt, csum_dir;
	int ret;
#endif

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	/* all ethtool calls start with a cmd word */
	if (copy_from_user(&cmd, uaddr, sizeof (uint32)))
		return -EFAULT;

	switch (cmd) {
	case ETHTOOL_GDRVINFO:
		/* Copy out any request driver name */
		if (copy_from_user(&info, uaddr, sizeof(info)))
			return -EFAULT;
		strncpy(drvname, info.driver, sizeof(info.driver));
		drvname[sizeof(info.driver)-1] = '\0';

		/* clear struct for return */
		memset(&info, 0, sizeof(info));
		info.cmd = cmd;

		/* if dhd requested, identify ourselves */
		if (strcmp(drvname, "?dhd") == 0) {
			snprintf(info.driver, sizeof(info.driver), "dhd");
			strncpy(info.version, EPI_VERSION_STR, sizeof(info.version) - 1);
			info.version[sizeof(info.version) - 1] = '\0';
		}

		/* otherwise, require dongle to be up */
		else if (!dhd->pub.up) {
			DHD_ERROR(("%s: dongle is not up\n", __FUNCTION__));
			return -ENODEV;
		}

		/* finally, report dongle driver type */
		else if (dhd->pub.iswl)
			snprintf(info.driver, sizeof(info.driver), "wl");
		else
			snprintf(info.driver, sizeof(info.driver), "xx");

		snprintf(info.version, sizeof(info.version), "%lu", dhd->pub.drv_version);
		if (copy_to_user(uaddr, &info, sizeof(info)))
			return -EFAULT;
		DHD_CTL(("%s: given %*s, returning %s\n", __FUNCTION__,
		         (int)sizeof(drvname), drvname, info.driver));
		break;

#ifdef TOE
	/* Get toe offload components from dongle */
	case ETHTOOL_GRXCSUM:
	case ETHTOOL_GTXCSUM:
		if ((ret = dhd_toe_get(dhd, 0, &toe_cmpnt)) < 0)
			return ret;

		csum_dir = (cmd == ETHTOOL_GTXCSUM) ? TOE_TX_CSUM_OL : TOE_RX_CSUM_OL;

		edata.cmd = cmd;
		edata.data = (toe_cmpnt & csum_dir) ? 1 : 0;

		if (copy_to_user(uaddr, &edata, sizeof(edata)))
			return -EFAULT;
		break;

	/* Set toe offload components in dongle */
	case ETHTOOL_SRXCSUM:
	case ETHTOOL_STXCSUM:
		if (copy_from_user(&edata, uaddr, sizeof(edata)))
			return -EFAULT;

		/* Read the current settings, update and write back */
		if ((ret = dhd_toe_get(dhd, 0, &toe_cmpnt)) < 0)
			return ret;

		csum_dir = (cmd == ETHTOOL_STXCSUM) ? TOE_TX_CSUM_OL : TOE_RX_CSUM_OL;

		if (edata.data != 0)
			toe_cmpnt |= csum_dir;
		else
			toe_cmpnt &= ~csum_dir;

		if ((ret = dhd_toe_set(dhd, 0, toe_cmpnt)) < 0)
			return ret;

		/* If setting TX checksum mode, tell Linux the new mode */
		if (cmd == ETHTOOL_STXCSUM) {
			if (edata.data)
				dhd->iflist[0]->net->features |= NETIF_F_IP_CSUM;
			else
				dhd->iflist[0]->net->features &= ~NETIF_F_IP_CSUM;
		}

		break;
#endif /* TOE */

	default:
		return -EOPNOTSUPP;
	}

	return 0;
}
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 4, 2) */

static bool dhd_check_hang(struct net_device *net, dhd_pub_t *dhdp, int error)
{
	dhd_info_t *dhd;
	int dump_len = 0;
	if (!dhdp) {
		DHD_ERROR(("%s: dhdp is NULL\n", __FUNCTION__));
		return FALSE;
	}

	if (!dhdp->up)
		return FALSE;

	dhd = (dhd_info_t *)dhdp->info;
#if !defined(BCMPCIE)
	if (dhd->thr_dpc_ctl.thr_pid < 0) {
		DHD_ERROR(("%s : skipped due to negative pid - unloading?\n", __FUNCTION__));
		return FALSE;
	}
#endif
	if (error == -ETIMEDOUT && dhdp->busstate != DHD_BUS_DOWN) {
		if (dhd_os_socram_dump(net, &dump_len) == BCME_OK) {
			dhd_dbg_send_urgent_evt(dhdp, NULL, 0);
		}
	}
	if ((error == -ETIMEDOUT) || (error == -EREMOTEIO) ||
		((dhdp->busstate == DHD_BUS_DOWN) && (!dhdp->dongle_reset))) {
		DHD_ERROR(("%s: Event HANG send up due to  re=%d te=%d e=%d s=%d\n", __FUNCTION__,
			dhdp->rxcnt_timeout, dhdp->txcnt_timeout, error, dhdp->busstate));
		net_os_send_hang_message(net);
		return TRUE;
	}
	return FALSE;
}

int dhd_ioctl_process(dhd_pub_t *pub, int ifidx, dhd_ioctl_t *ioc, void *data_buf)
{
	int bcmerror = BCME_OK;
	int buflen = 0;
	struct net_device *net;

	net = dhd_idx2net(pub, ifidx);
	if (!net) {
		bcmerror = BCME_BADARG;
		goto done;
	}

	if (data_buf)
		buflen = MIN(ioc->len, DHD_IOCTL_MAXLEN);

	/* check for local dhd ioctl and handle it */
	if (ioc->driver == DHD_IOCTL_MAGIC) {
		bcmerror = dhd_ioctl((void *)pub, ioc, data_buf, buflen);
		if (bcmerror)
			pub->bcmerror = bcmerror;
		goto done;
	}

	/* send to dongle (must be up, and wl). */
	if (pub->busstate != DHD_BUS_DATA) {
		bcmerror = BCME_DONGLE_DOWN;
		goto done;
	}

	if (!pub->iswl) {
		bcmerror = BCME_DONGLE_DOWN;
		goto done;
	}

	/*
	 * Flush the TX queue if required for proper message serialization:
	 * Intercept WLC_SET_KEY IOCTL - serialize M4 send and set key IOCTL to
	 * prevent M4 encryption and
	 * intercept WLC_DISASSOC IOCTL - serialize WPS-DONE and WLC_DISASSOC IOCTL to
	 * prevent disassoc frame being sent before WPS-DONE frame.
	 */
	if (ioc->cmd == WLC_SET_KEY ||
	    (ioc->cmd == WLC_SET_VAR && data_buf != NULL &&
	     strncmp("wsec_key", data_buf, 9) == 0) ||
	    (ioc->cmd == WLC_SET_VAR && data_buf != NULL &&
	     strncmp("bsscfg:wsec_key", data_buf, 15) == 0) ||
	    ioc->cmd == WLC_DISASSOC)
		dhd_wait_pend8021x(net);

#ifdef WLMEDIA_HTSF
	if (data_buf) {
		/*  short cut wl ioctl calls here  */
		if (strcmp("htsf", data_buf) == 0) {
			dhd_ioctl_htsf_get(dhd, 0);
			return BCME_OK;
		}

		if (strcmp("htsflate", data_buf) == 0) {
			if (ioc->set) {
				memset(ts, 0, sizeof(tstamp_t)*TSMAX);
				memset(&maxdelayts, 0, sizeof(tstamp_t));
				maxdelay = 0;
				tspktcnt = 0;
				maxdelaypktno = 0;
				memset(&vi_d1.bin, 0, sizeof(uint32)*NUMBIN);
				memset(&vi_d2.bin, 0, sizeof(uint32)*NUMBIN);
				memset(&vi_d3.bin, 0, sizeof(uint32)*NUMBIN);
				memset(&vi_d4.bin, 0, sizeof(uint32)*NUMBIN);
			} else {
				dhd_dump_latency();
			}
			return BCME_OK;
		}
		if (strcmp("htsfclear", data_buf) == 0) {
			memset(&vi_d1.bin, 0, sizeof(uint32)*NUMBIN);
			memset(&vi_d2.bin, 0, sizeof(uint32)*NUMBIN);
			memset(&vi_d3.bin, 0, sizeof(uint32)*NUMBIN);
			memset(&vi_d4.bin, 0, sizeof(uint32)*NUMBIN);
			htsf_seqnum = 0;
			return BCME_OK;
		}
		if (strcmp("htsfhis", data_buf) == 0) {
			dhd_dump_htsfhisto(&vi_d1, "H to D");
			dhd_dump_htsfhisto(&vi_d2, "D to D");
			dhd_dump_htsfhisto(&vi_d3, "D to H");
			dhd_dump_htsfhisto(&vi_d4, "H to H");
			return BCME_OK;
		}
		if (strcmp("tsport", data_buf) == 0) {
			if (ioc->set) {
				memcpy(&tsport, data_buf + 7, 4);
			} else {
				DHD_ERROR(("current timestamp port: %d \n", tsport));
			}
			return BCME_OK;
		}
	}
#endif /* WLMEDIA_HTSF */

	if ((ioc->cmd == WLC_SET_VAR || ioc->cmd == WLC_GET_VAR) &&
		data_buf != NULL && strncmp("rpc_", data_buf, 4) == 0) {
#ifdef BCM_FD_AGGR
		bcmerror = dhd_fdaggr_ioctl(pub, ifidx, (wl_ioctl_t *)ioc, data_buf, buflen);
#else
		bcmerror = BCME_UNSUPPORTED;
#endif
		goto done;
	}
	bcmerror = dhd_wl_ioctl(pub, ifidx, (wl_ioctl_t *)ioc, data_buf, buflen);

done:
	dhd_check_hang(net, pub, bcmerror);

	return bcmerror;
}

static int
dhd_ioctl_entry(struct net_device *net, struct ifreq *ifr, int cmd)
{
	dhd_info_t *dhd = DHD_DEV_INFO(net);
	dhd_ioctl_t ioc;
	int bcmerror = 0;
	int ifidx;
	int ret;
	void *local_buf = NULL;
	u16 buflen = 0;

	DHD_OS_WAKE_LOCK(&dhd->pub);
	DHD_PERIM_LOCK(&dhd->pub);

	/* Interface up check for built-in type */
	if (!dhd_download_fw_on_driverload && dhd->pub.up == 0) {
		DHD_ERROR(("%s: Interface is down \n", __FUNCTION__));
		DHD_PERIM_UNLOCK(&dhd->pub);
		DHD_OS_WAKE_UNLOCK(&dhd->pub);
		return BCME_NOTUP;
	}

	/* send to dongle only if we are not waiting for reload already */
	if (dhd->pub.hang_was_sent) {
		DHD_ERROR(("%s: HANG was sent up earlier\n", __FUNCTION__));
		DHD_OS_WAKE_LOCK_CTRL_TIMEOUT_ENABLE(&dhd->pub, DHD_EVENT_TIMEOUT_MS);
		DHD_OS_WAKE_UNLOCK(&dhd->pub);
		return OSL_ERROR(BCME_DONGLE_DOWN);
	}

	ifidx = dhd_net2idx(dhd, net);
	DHD_TRACE(("%s: ifidx %d, cmd 0x%04x\n", __FUNCTION__, ifidx, cmd));

	if (ifidx == DHD_BAD_IF) {
		DHD_ERROR(("%s: BAD IF\n", __FUNCTION__));
		DHD_PERIM_UNLOCK(&dhd->pub);
		DHD_OS_WAKE_UNLOCK(&dhd->pub);
		return -1;
	}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 4, 2)
	if (cmd == SIOCETHTOOL) {
		ret = dhd_ethtool(dhd, (void*)ifr->ifr_data);
		DHD_PERIM_UNLOCK(&dhd->pub);
		DHD_OS_WAKE_UNLOCK(&dhd->pub);
		return ret;
	}
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 4, 2) */

	if (cmd == SIOCDEVPRIVATE+1) {
		ret = wl_android_priv_cmd(net, ifr, cmd);
		dhd_check_hang(net, &dhd->pub, ret);
		DHD_OS_WAKE_UNLOCK(&dhd->pub);
		return ret;
	}

	if (cmd != SIOCDEVPRIVATE) {
		DHD_PERIM_UNLOCK(&dhd->pub);
		DHD_OS_WAKE_UNLOCK(&dhd->pub);
		return -EOPNOTSUPP;
	}

	memset(&ioc, 0, sizeof(ioc));

#ifdef CONFIG_COMPAT
	if (is_compat_task()) {
		compat_wl_ioctl_t compat_ioc;
		if (copy_from_user(&compat_ioc, ifr->ifr_data, sizeof(compat_wl_ioctl_t))) {
			bcmerror = BCME_BADADDR;
			goto done;
		}
		ioc.cmd = compat_ioc.cmd;
#if defined(CONFIG_COMPAT) && defined(HW_SOFTAP_MANAGEMENT_BUG)
		if(ioc.cmd & WLC_SPEC_FLAG) {
			memset(&ioc, 0, sizeof(ioc));
			/* Copy the ioc control structure part of ioctl request */
			if (copy_from_user(&ioc, ifr->ifr_data, sizeof(wl_ioctl_t))) {
				bcmerror = BCME_BADADDR;
				goto done;
			}
			ioc.cmd &= ~WLC_SPEC_FLAG; /* Clear the FLAG */

			/* To differentiate between wl and dhd read 4 more byes */
			if ((copy_from_user(&ioc.driver, (char *)ifr->ifr_data + sizeof(wl_ioctl_t),
				sizeof(uint)) != 0)) {
				bcmerror = BCME_BADADDR;
				goto done;
			}

		}else { /* ioc.cmd & WLC_SPEC_FLAG */
#endif
			ioc.buf = compat_ptr(compat_ioc.buf);
			ioc.len = compat_ioc.len;
			ioc.set = compat_ioc.set;
			ioc.used = compat_ioc.used;
			ioc.needed = compat_ioc.needed;
			/* To differentiate between wl and dhd read 4 more byes */
			if ((copy_from_user(&ioc.driver, (char *)ifr->ifr_data + sizeof(compat_wl_ioctl_t),
				sizeof(uint)) != 0)) {
				bcmerror = BCME_BADADDR;
				goto done;
#if defined(CONFIG_COMPAT) && defined(HW_SOFTAP_MANAGEMENT_BUG)
			}
#endif
		} /* ioc.cmd & WLC_SPEC_FLAG */
	} else
#endif /* CONFIG_COMPAT */
	{
		/* Copy the ioc control structure part of ioctl request */
		if (copy_from_user(&ioc, ifr->ifr_data, sizeof(wl_ioctl_t))) {
			bcmerror = BCME_BADADDR;
			goto done;
		}
#if defined(CONFIG_COMPAT) && defined(HW_SOFTAP_MANAGEMENT_BUG)
		ioc.cmd &= ~WLC_SPEC_FLAG; /* make sure it was clear when it isn't a compat task*/
#endif

		/* To differentiate between wl and dhd read 4 more byes */
		if ((copy_from_user(&ioc.driver, (char *)ifr->ifr_data + sizeof(wl_ioctl_t),
			sizeof(uint)) != 0)) {
			bcmerror = BCME_BADADDR;
			goto done;
		}
	}

	if (!capable(CAP_NET_ADMIN)) {
		bcmerror = BCME_EPERM;
		goto done;
	}

	if (ioc.len > 0) {
		buflen = MIN(ioc.len, DHD_IOCTL_MAXLEN);
		if (!(local_buf = MALLOC(dhd->pub.osh, buflen+1))) {
			bcmerror = BCME_NOMEM;
			goto done;
		}

		DHD_PERIM_UNLOCK(&dhd->pub);
		if (copy_from_user(local_buf, ioc.buf, buflen)) {
			DHD_PERIM_LOCK(&dhd->pub);
			bcmerror = BCME_BADADDR;
			goto done;
		}
		DHD_PERIM_LOCK(&dhd->pub);

		*(char *)(local_buf + buflen) = '\0';
	}

	bcmerror = dhd_ioctl_process(&dhd->pub, ifidx, &ioc, local_buf);

	if (!bcmerror && buflen && local_buf && ioc.buf) {
		DHD_PERIM_UNLOCK(&dhd->pub);
		if (copy_to_user(ioc.buf, local_buf, buflen))
			bcmerror = -EFAULT;
		DHD_PERIM_LOCK(&dhd->pub);
	}

done:
	if (local_buf)
		MFREE(dhd->pub.osh, local_buf, buflen+1);

	DHD_PERIM_UNLOCK(&dhd->pub);
	DHD_OS_WAKE_UNLOCK(&dhd->pub);

	return OSL_ERROR(bcmerror);
}

static int
dhd_stop_no_lock(struct net_device *net)
{
	int ifidx = 0;
	dhd_info_t *dhd = DHD_DEV_INFO(net);
	DHD_OS_WAKE_LOCK(&dhd->pub);
	DHD_PERIM_LOCK(&dhd->pub);
	HW_PRINT((WIFI_TAG"Enter: %s\n", __FUNCTION__));
	DHD_TRACE(("%s: Enter %p\n", __FUNCTION__, net));
	if (dhd->pub.up == 0) {
		goto exit;
	}
#ifdef HW_SOFTAP_BUGFIX
	hw_reset_beacon_interval(net);
#endif
	dhd_if_flush_sta(DHD_DEV_IFP(net));

	ifidx = dhd_net2idx(dhd, net);
	BCM_REFERENCE(ifidx);
#ifdef DHD_TPUT_MONITOR
	dhd_tput_monitor_release(&dhd->pub);
#endif /* DHD_TPUT_MONITOR */
#ifdef DHD_DEVWAKE_EARLY
	dhd_devwake_release(&dhd->pub);
#endif /* DHD_DEVWAKE_EARLY */

	/* Set state and stop OS transmissions */
	netif_stop_queue(net);
	dhd->pub.up = 0;

#ifdef WL_CFG80211
#ifdef WL_TEM_CTRL
	DHD_TRACE(("%s: call wl_hw_tem_ctrl_event_report\n", __FUNCTION__));
	wl_hw_tem_ctrl_event_report();
#endif
#if defined WL_TEM_CTRL || defined WL_TIM_EVENT
	wifi_exit_proc();
#endif
	if (ifidx == 0) {
		wl_cfg80211_down(NULL);

		/*
		 * For CFG80211: Clean up all the left over virtual interfaces
		 * when the primary Interface is brought down. [ifconfig wlan0 down]
		 */
		if (!dhd_download_fw_on_driverload) {
			if ((dhd->dhd_state & DHD_ATTACH_STATE_ADD_IF) &&
				(dhd->dhd_state & DHD_ATTACH_STATE_CFG80211)) {
				int i;
				dhd_if_t *ifp;

				dhd_net_if_lock_local(dhd);
				for (i = 1; i < DHD_MAX_IFS; i++)
					dhd_remove_if(&dhd->pub, i, FALSE);

				/* remove sta list for primary interface */
				ifp = dhd->iflist[0];
				if (ifp && ifp->net) {
					dhd_if_del_sta_list(ifp);
				}
#ifdef PCIE_FULL_DONGLE
				/* Initialize STA info list */
				INIT_LIST_HEAD(&ifp->sta_list);
#endif
#ifdef ARP_OFFLOAD_SUPPORT
				if (dhd_inetaddr_notifier_registered) {
					dhd_inetaddr_notifier_registered = FALSE;
					unregister_inetaddr_notifier(&dhd_inetaddr_notifier);
				}
#endif /* ARP_OFFLOAD_SUPPORT */
#ifdef CONFIG_IPV6
				if (dhd_inet6addr_notifier_registered) {
					dhd_inet6addr_notifier_registered = FALSE;
					unregister_inet6addr_notifier(&dhd_inet6addr_notifier);
				}
#endif /* CONFIG_IPV6 */

				dhd_net_if_unlock_local(dhd);
			}
#ifdef HW_PCIE_STABILITY
			cancel_work_sync(dhd->dhd_deferred_wq);
#endif
		}
	}
#endif /* WL_CFG80211 */

#ifdef PROP_TXSTATUS
	dhd_wlfc_cleanup(&dhd->pub, NULL, 0);
#endif

#ifdef BCM_APF
    DHD_ERROR(("%s: dhd_dev_apf_delete_filter \n", __FUNCTION__));
	dhd_dev_apf_delete_filter(net);
#endif /* BCM_APF */

	/* Stop the protocol module */
	dhd_prot_stop(&dhd->pub);

	OLD_MOD_DEC_USE_COUNT;
exit:
#if defined(WL_CFG80211)
	if (ifidx == 0 && !dhd_download_fw_on_driverload)
		wl_android_wifi_off(net, TRUE);
#endif
	dhd->pub.rxcnt_timeout = 0;
	dhd->pub.txcnt_timeout = 0;

	dhd->pub.hang_was_sent = 0;

	/* Clear country spec for for built-in type driver */
#ifndef CUSTOM_COUNTRY_CODE
	if (!dhd_download_fw_on_driverload) {
		dhd->pub.dhd_cspec.country_abbrev[0] = 0x00;
		dhd->pub.dhd_cspec.rev = 0;
		dhd->pub.dhd_cspec.ccode[0] = 0x00;
	}
#endif
#ifdef CONFIG_HW_WIFI_FREQ_CTRL_FLAG
	hw_wifi_freq_ctrl_destroy();
#endif

	DHD_PERIM_UNLOCK(&dhd->pub);
	DHD_OS_WAKE_UNLOCK(&dhd->pub);
#ifdef BCM_PCIE_UPDATE
	/* Destroy wakelock */
	if (!dhd_download_fw_on_driverload &&
		(dhd->dhd_state & DHD_ATTACH_STATE_WAKELOCKS_INIT)) {
		dhd_wk_lock_stats_dump(&dhd->pub);
		DHD_OS_WAKE_LOCK_DESTROY(dhd);
		dhd->dhd_state &= ~DHD_ATTACH_STATE_WAKELOCKS_INIT;
	}
#endif
	HW_PRINT((WIFI_TAG"Exit: %s\n", __FUNCTION__));

	return 0;
}

static int
dhd_stop(struct net_device *net)
{
	dhd_info_t *dhd = DHD_DEV_INFO(net);
	HW_PRINT((WIFI_TAG"Enter: %s\n", __FUNCTION__));
#ifdef HW_WIFI_OPEN_STOP
	dhd_open_stop_if_mutex_lock(dhd);
#endif

	dhd_stop_no_lock(net);

#ifdef HW_WIFI_OPEN_STOP
	dhd_open_stop_if_mutex_unlock(dhd);
#endif
	HW_PRINT((WIFI_TAG"Exit: %s\n", __FUNCTION__));
	return 0;
}

#if defined(WL_CFG80211) && (defined(USE_INITIAL_2G_SCAN) || \
	defined(USE_INITIAL_SHORT_DWELL_TIME))
extern bool g_first_broadcast_scan;
#endif /* OEM_ANDROID && WL_CFG80211 && (USE_INITIAL_2G_SCAN || USE_INITIAL_SHORT_DWELL_TIME) */

#ifdef WL11U
static int dhd_interworking_enable(dhd_pub_t *dhd)
{
	uint32 enable = TRUE;
	int ret;

	ret = dhd_iovar(dhd, 0, "interworking", (char *)&enable, sizeof(enable),
			NULL, 0, TRUE);
	if (ret < 0)
		DHD_ERROR(("%s: enableing interworking failed, ret=%d\n", __FUNCTION__, ret));

	if (ret == BCME_OK) {
		/* basic capabilities for HS20 REL2 */
		uint32 cap = WL_WNM_BSSTRANS | WL_WNM_NOTIF;

		ret = dhd_iovar(dhd, 0, "wnm", (char *)&cap, sizeof(cap), NULL,
				0, TRUE);
		if (ret < 0)
			DHD_ERROR(("%s: failed to set WNM info, ret=%d\n", __FUNCTION__, ret));
	}

	return ret;
}
#endif /* WL11u */

static int
dhd_open(struct net_device *net)
{
	dhd_info_t *dhd = DHD_DEV_INFO(net);
#ifdef TOE
	uint32 toe_ol;
#endif
	int ifidx;
	int32 ret = 0;
#ifdef HW_PCIE_STABILITY
	uint32 flags;
#endif /* HW_PCIE_STABILITY */
#ifdef BCM_PCIE_UPDATE	
	if (!dhd_download_fw_on_driverload && !dhd_driver_init_done) {
		DHD_ERROR(("%s: WLAN driver is not initialized\n", __FUNCTION__));
		return -1;
	}
#ifdef HW_PCIE_STABILITY
	/* set bus to osl for linkdown checking later */
	osl_set_bus_handle(dhd->pub.osh, dhd->pub.bus);
#endif /* BCMPCIE */
	/* Init wakelock */
	if (!dhd_download_fw_on_driverload &&
		!(dhd->dhd_state & DHD_ATTACH_STATE_WAKELOCKS_INIT)) {
		DHD_OS_WAKE_LOCK_INIT(dhd);
		dhd->dhd_state |= DHD_ATTACH_STATE_WAKELOCKS_INIT;
	}
#endif
	DHD_ERROR(("%s enter\n", __FUNCTION__));
#ifdef HW_WIFI_OPEN_STOP
	dhd_open_stop_if_mutex_lock(dhd);
#endif
#if (defined(CONFIG_BCMDHD_PCIE) && defined(HW_WIFI_DMD_LOG) && defined(CONFIG_ARCH_HISI))
	char dmdbuf[DHD_PCIE_BUF_SIZE + 1] = {0};
#endif

	DHD_OS_WAKE_LOCK(&dhd->pub);
	DHD_PERIM_LOCK(&dhd->pub);
#ifdef HW_PCIE_STABILITY
	DHD_GENERAL_LOCK(&dhd->pub, flags);
	dhd->pub.dhd_bus_busy_state |= DHD_BUS_BUSY_IN_OPEN;
	DHD_GENERAL_UNLOCK(&dhd->pub, flags);
#endif /* HW_PCIE_STABILITY */
	dhd->pub.dongle_trap_occured = 0;
	dhd->pub.hang_was_sent = 0;
#ifdef BCM_PCIE_UPDATE
	dhd->pub.hang_reason = 0;
#ifdef DHD_DEVWAKE_EARLY
	dhd_devwake_init(&dhd->pub);
#endif /* DHD_DEVWAKE_EARLY */
#endif
#ifdef	HW_PCIE_STABILITY
	INIT_WORK(&dhd->pub.hang_work, dhd_hang_work_handler);
#endif
#if !defined(WL_CFG80211)
	/*
	 * Force start if ifconfig_up gets called before START command
	 *  We keep WEXT's wl_control_wl_start to provide backward compatibility
	 *  This should be removed in the future
	 */
	ret = wl_control_wl_start(net);
	if (ret != 0) {
		DHD_ERROR(("%s: failed with code %d\n", __FUNCTION__, ret));
		ret = -1;
		goto exit;
	}

#endif

	ifidx = dhd_net2idx(dhd, net);
	DHD_TRACE(("%s: ifidx %d\n", __FUNCTION__, ifidx));

	if (ifidx < 0) {
		DHD_ERROR(("%s: Error: called with invalid IF\n", __FUNCTION__));
		ret = -1;
		goto exit;
	}

	if (!dhd->iflist[ifidx]) {
		DHD_ERROR(("%s: Error: called when IF already deleted\n", __FUNCTION__));
		ret = -1;
		goto exit;
	}

	if (ifidx == 0) {
		atomic_set(&dhd->pend_8021x_cnt, 0);
#if defined(WL_CFG80211)
		if (!dhd_download_fw_on_driverload) {
			DHD_ERROR(("\n%s\n", dhd_version));
#if defined(USE_INITIAL_2G_SCAN) || defined(USE_INITIAL_SHORT_DWELL_TIME)
			g_first_broadcast_scan = TRUE;
#endif /* USE_INITIAL_2G_SCAN || USE_INITIAL_SHORT_DWELL_TIME */
			ret = wl_android_wifi_on(net);
			if (ret != 0) {
				DHD_ERROR(("%s : wl_android_wifi_on failed (%d)\n",
					__FUNCTION__, ret));
				ret = -1;
				goto exit;
			}
		}
#endif

		if (dhd->pub.busstate != DHD_BUS_DATA) {
#ifdef HW_FW_HALT_PATCH
			DHD_ERROR(("%s: current busstate is %d\n", __FUNCTION__, dhd->pub.busstate));
			if(dhd->pub.hang_was_sent) {
				DHD_ERROR(("%s: error occurred, do not try to bring up bus\n",__FUNCTION__));
				ret = -1;
				goto exit;
			}
#endif
			/* try to bring up bus */
			DHD_PERIM_UNLOCK(&dhd->pub);
			ret = dhd_bus_start(&dhd->pub);
			DHD_PERIM_LOCK(&dhd->pub);
			if (ret) {
				DHD_ERROR(("%s: failed with code %d\n", __FUNCTION__, ret));
				ret = -1;
				goto exit;
			}
		}

		/* dhd_sync_with_dongle has been called in dhd_bus_start or wl_android_wifi_on */
		memcpy(net->dev_addr, dhd->pub.mac.octet, ETHER_ADDR_LEN);

#ifdef TOE
		/* Get current TOE mode from dongle */
		if (dhd_toe_get(dhd, ifidx, &toe_ol) >= 0 && (toe_ol & TOE_TX_CSUM_OL) != 0)
			dhd->iflist[ifidx]->net->features |= NETIF_F_IP_CSUM;
		else
			dhd->iflist[ifidx]->net->features &= ~NETIF_F_IP_CSUM;
#endif /* TOE */

#if defined(WL_CFG80211)
		if (unlikely(wl_cfg80211_up(NULL))) {
			DHD_ERROR(("%s: failed to bring up cfg80211\n", __FUNCTION__));
			ret = -1;
			goto exit;
		}
#endif /* WL_CFG80211 */
		if (!dhd_download_fw_on_driverload) {
#ifdef ARP_OFFLOAD_SUPPORT
				dhd->pend_ipaddr = 0;
				if (!dhd_inetaddr_notifier_registered) {
					dhd_inetaddr_notifier_registered = TRUE;
					register_inetaddr_notifier(&dhd_inetaddr_notifier);
				}
#endif /* ARP_OFFLOAD_SUPPORT */
#ifdef CONFIG_IPV6
				if (!dhd_inet6addr_notifier_registered) {
					dhd_inet6addr_notifier_registered = TRUE;
					register_inet6addr_notifier(&dhd_inet6addr_notifier);
				}
#endif /* CONFIG_IPV6 */
		}
	}

	/* Allow transmit calls */
	netif_start_queue(net);
	dhd->pub.up = 1;
	OLD_MOD_INC_USE_COUNT;

#ifdef BCMDBGFS
	dhd_dbgfs_init(&dhd->pub);
#endif
#ifdef DHD_TPUT_MONITOR
	dhd_tput_monitor_init(&dhd->pub);
#endif /* DHD_TPUT_MONITOR */

exit:
#ifdef HW_PCIE_STABILITY
	DHD_GENERAL_LOCK(&dhd->pub, flags);
	dhd->pub.dhd_bus_busy_state &= ~DHD_BUS_BUSY_IN_OPEN;
	dhd_os_busbusy_wake(&dhd->pub);
	DHD_GENERAL_UNLOCK(&dhd->pub, flags);
#endif /* HW_PCIE_STABILITY */

	if (ret){
#ifdef HW_WIFI_OPEN_STOP
		DHD_ERROR(("%s: open wifi fail, goto dhd_stop, ret:%d\n", __FUNCTION__, ret));
#ifdef HW_WIFI_DMD_LOG
#if (defined(CONFIG_BCMDHD_PCIE) && defined(CONFIG_ARCH_HISI))
		dsm_pcie_dump_reginfo(dmdbuf, DHD_PCIE_BUF_SIZE);
		hw_wifi_dsm_client_notify(DSM_WIFI_OPEN_ERROR,
				"open wifi fail, goto dhd_stop, ret:%d %s\n", ret, dmdbuf);
#else
		hw_wifi_dsm_client_notify(DSM_WIFI_OPEN_ERROR,
		             "%s: open wifi fail, ret:%d, %s\n", __FUNCTION__, ret, hw_dmd_get_trace_log());
#endif /* CONFIG_BCMDHD_PCIE */
#endif
		dhd_stop_no_lock(net);
#else
		dhd_stop(net);
#endif
	}
	DHD_PERIM_UNLOCK(&dhd->pub);
	DHD_OS_WAKE_UNLOCK(&dhd->pub);


#ifdef HW_WIFI_OPEN_STOP
	dhd_open_stop_if_mutex_unlock(dhd);
#endif
	DHD_ERROR(("%s exit with ret: %d\n", __FUNCTION__, ret));
#ifdef HW_OTP_CHECK
	if (!ret) {
		hw_check_chip_otp(&dhd->pub);
	}
#endif
	return ret;
}

int dhd_do_driver_init(struct net_device *net)
{
	dhd_info_t *dhd = NULL;

	if (!net) {
		DHD_ERROR(("Primary Interface not initialized \n"));
		return -EINVAL;
	}


	/*  && defined(OEM_ANDROID) && defined(BCMSDIO) */
	dhd = DHD_DEV_INFO(net);

	/* If driver is already initialized, do nothing
	 */
	if (dhd->pub.busstate == DHD_BUS_DATA) {
		DHD_TRACE(("Driver already Inititalized. Nothing to do"));
		return 0;
	}

	if (dhd_open(net) < 0) {
		DHD_ERROR(("Driver Init Failed \n"));
		return -1;
	}

	return 0;
}

int
dhd_event_ifadd(dhd_info_t *dhdinfo, wl_event_data_if_t *ifevent, char *name, uint8 *mac)
{
#ifdef HW_DEFERRED_WORK_BUGFIX
	int ret = 0;
#endif

#ifdef WL_CFG80211
	if (wl_cfg80211_notify_ifadd(ifevent->ifidx, name, mac, ifevent->bssidx) == BCME_OK)
		return BCME_OK;
#endif

	/* handle IF event caused by wl commands, SoftAP, WEXT and
	 * anything else. This has to be done asynchronously otherwise
	 * DPC will be blocked (and iovars will timeout as DPC has no chance
	 * to read the response back)
	 */
	if (ifevent->ifidx > 0) {
		dhd_if_event_t *if_event = MALLOC(dhdinfo->pub.osh, sizeof(dhd_if_event_t));
#ifdef BCM_PCIE_UPDATE
		if (if_event == NULL) {
			DHD_ERROR(("dhd_event_ifadd: Failed MALLOC, malloced %d bytes",
				MALLOCED(dhdinfo->pub.osh)));
			return BCME_NOMEM;
		}
#endif

		memcpy(&if_event->event, ifevent, sizeof(if_event->event));
		memcpy(if_event->mac, mac, ETHER_ADDR_LEN);
		strncpy(if_event->name, name, IFNAMSIZ);
		if_event->name[IFNAMSIZ - 1] = '\0';
#ifdef HW_DEFERRED_WORK_BUGFIX
		ret = dhd_deferred_schedule_work(dhdinfo->dhd_deferred_wq, (void *)if_event,
				DHD_WQ_WORK_IF_ADD, dhd_ifadd_event_handler, DHD_WORK_PRIORITY_LOW);
		if (ret)
			DHD_ERROR(("dhd_event_ifadd failed\n"));
#else
		dhd_deferred_schedule_work(dhdinfo->dhd_deferred_wq, (void *)if_event,
			DHD_WQ_WORK_IF_ADD, dhd_ifadd_event_handler, DHD_WORK_PRIORITY_LOW);
#endif
	}

	return BCME_OK;
}

int
dhd_event_ifdel(dhd_info_t *dhdinfo, wl_event_data_if_t *ifevent, char *name, uint8 *mac)
{
	dhd_if_event_t *if_event;
#ifdef HW_DEFERRED_WORK_BUGFIX
	int ret = 0;
#endif

#ifdef WL_CFG80211
	if (wl_cfg80211_notify_ifdel(ifevent->ifidx, name, mac, ifevent->bssidx) == BCME_OK)
		return BCME_OK;
#endif /* WL_CFG80211 */

	/* handle IF event caused by wl commands, SoftAP, WEXT and
	 * anything else
	 */
	if_event = MALLOC(dhdinfo->pub.osh, sizeof(dhd_if_event_t));
#ifdef BCM_PCIE_UPDATE
	if (if_event == NULL) {
		DHD_ERROR(("dhd_event_ifdel: malloc failed for if_event, malloced %d bytes",
			MALLOCED(dhdinfo->pub.osh)));
		return BCME_NOMEM;
	}
#endif
	memcpy(&if_event->event, ifevent, sizeof(if_event->event));
	memcpy(if_event->mac, mac, ETHER_ADDR_LEN);
	strncpy(if_event->name, name, IFNAMSIZ);
	if_event->name[IFNAMSIZ - 1] = '\0';
#ifdef HW_DEFERRED_WORK_BUGFIX
	ret = dhd_deferred_schedule_work(dhdinfo->dhd_deferred_wq, (void *)if_event, DHD_WQ_WORK_IF_DEL,
			dhd_ifdel_event_handler, DHD_WORK_PRIORITY_LOW);
	if (ret) {
		DHD_ERROR(("dhd_event_ifdel failed\n"));
		return BCME_ERROR;
	}
#else
	dhd_deferred_schedule_work(dhdinfo->dhd_deferred_wq, (void *)if_event, DHD_WQ_WORK_IF_DEL,
		dhd_ifdel_event_handler, DHD_WORK_PRIORITY_LOW);
#endif

	return BCME_OK;
}

/* unregister and free the existing net_device interface (if any) in iflist and
 * allocate a new one. the slot is reused. this function does NOT register the
 * new interface to linux kernel. dhd_register_if does the job
 */
#ifndef  BRCM_RSDB
struct net_device*
dhd_allocate_if(dhd_pub_t *dhdpub, int ifidx, char *name,
	uint8 *mac, uint8 bssidx, bool need_rtnl_lock)
#else
struct net_device*
dhd_allocate_if(dhd_pub_t *dhdpub, int ifidx, char *name,
	uint8 *mac, uint8 bssidx, bool need_rtnl_lock, char *dngl_name)
#endif
{
	dhd_info_t *dhdinfo = (dhd_info_t *)dhdpub->info;
	dhd_if_t *ifp;

	ASSERT(dhdinfo && (ifidx < DHD_MAX_IFS));
	if (NULL == dhdinfo)
		return;

	ifp = dhdinfo->iflist[ifidx];

	if (ifp != NULL) {
		if (ifp->net != NULL) {
			DHD_ERROR(("%s: free existing IF %s\n", __FUNCTION__, ifp->net->name));
#ifdef HW_WIFI_ALLOC_IF_PANIC
			if ((strcmp(ifp->net->name, "wlan0") == 0)
					&& name && (strcmp(name, "wlan0") != 0)) {
				DHD_ERROR(("not allow to release main interface, new name:%s\n", name));
				return NULL;
			}
#else
			dhd_dev_priv_clear(ifp->net); /* clear net_device private */
#endif
#ifdef HW_NETDEVICE_PANIC
			hw_unregister_breakpoint(ifidx, ifp->net->name);
#endif
			/* in unregister_netdev case, the interface gets freed by net->destructor
			 * (which is set to free_netdev)
			 */
			if (ifp->net->reg_state == NETREG_UNINITIALIZED) {
				free_netdev(ifp->net);
			} else {
				netif_stop_queue(ifp->net);
				if (need_rtnl_lock)
					unregister_netdev(ifp->net);
				else
					unregister_netdevice(ifp->net);
			}
			ifp->net = NULL;
		}
	} else {
		ifp = MALLOC(dhdinfo->pub.osh, sizeof(dhd_if_t));
		if (ifp == NULL) {
			DHD_ERROR(("%s: OOM - dhd_if_t(%zu)\n", __FUNCTION__, sizeof(dhd_if_t)));
			return NULL;
		}
	}

	memset(ifp, 0, sizeof(dhd_if_t));
	ifp->info = dhdinfo;
	ifp->idx = ifidx;
	ifp->bssidx = bssidx;
	if (mac != NULL)
		memcpy(&ifp->mac_addr, mac, ETHER_ADDR_LEN);

	/* Allocate etherdev, including space for private structure */
	ifp->net = alloc_etherdev(DHD_DEV_PRIV_SIZE);
	if (ifp->net == NULL) {
		DHD_ERROR(("%s: OOM - alloc_etherdev(%zu)\n", __FUNCTION__, sizeof(dhdinfo)));
		goto fail;
	}

	/* Setup the dhd interface's netdevice private structure. */
	dhd_dev_priv_save(ifp->net, dhdinfo, ifp, ifidx);

	if (name && name[0]) {
		strncpy(ifp->net->name, name, IFNAMSIZ);
		ifp->net->name[IFNAMSIZ - 1] = '\0';
	}
#ifdef WL_CFG80211
	if (ifidx == 0)
		ifp->net->destructor = free_netdev;
	else
		ifp->net->destructor = dhd_netdev_free;
#else
	ifp->net->destructor = free_netdev;
#endif /* WL_CFG80211 */
	strncpy(ifp->name, ifp->net->name, IFNAMSIZ);
	ifp->name[IFNAMSIZ - 1] = '\0';
	dhdinfo->iflist[ifidx] = ifp;
#ifdef HW_NETDEVICE_PANIC
	DHD_ERROR(("%s alloc netdevice %p\n", __func__, ifp->net));
	if (ifidx > 0) {
		hw_register_breakpoint(ifp->net, ifidx, ifp->name);
	}
#endif
#ifdef  BRCM_RSDB
/* initialize the dongle provided if name */
        if (dngl_name)
                strncpy(ifp->dngl_name, dngl_name, IFNAMSIZ);
        else
                strncpy(ifp->dngl_name, name, IFNAMSIZ);
#endif
#ifdef PCIE_FULL_DONGLE
	/* Initialize STA info list */
	INIT_LIST_HEAD(&ifp->sta_list);
	DHD_IF_STA_LIST_LOCK_INIT(ifp);
#endif /* PCIE_FULL_DONGLE */

	return ifp->net;

fail:
	if (ifp != NULL) {
		if (ifp->net != NULL) {
			dhd_dev_priv_clear(ifp->net);
			free_netdev(ifp->net);
			ifp->net = NULL;
		}
		MFREE(dhdinfo->pub.osh, ifp, sizeof(*ifp));
		ifp = NULL;
	}
	dhdinfo->iflist[ifidx] = NULL;
	return NULL;
}

/* unregister and free the the net_device interface associated with the indexed
 * slot, also free the slot memory and set the slot pointer to NULL
 */
int
dhd_remove_if(dhd_pub_t *dhdpub, int ifidx, bool need_rtnl_lock)
{
	dhd_info_t *dhdinfo = (dhd_info_t *)dhdpub->info;
	dhd_if_t *ifp;
#ifdef DHD_DEVWAKE_EARLY
	mutex_lock(&dhdinfo->ifdel_mutex[ifidx]);
#endif
	ifp = dhdinfo->iflist[ifidx];
	if (ifp != NULL) {
		if (ifp->net != NULL) {
			DHD_ERROR(("deleting interface '%s' idx %d\n", ifp->net->name, ifp->idx));

#ifdef HW_NETDEVICE_PANIC
			hw_unregister_breakpoint(ifidx, ifp->net->name);
#endif
			/* in unregister_netdev case, the interface gets freed by net->destructor
			 * (which is set to free_netdev)
			 */
			if (ifp->net->reg_state == NETREG_UNINITIALIZED) {
				free_netdev(ifp->net);
			} else {
				netif_stop_queue(ifp->net);



				if (need_rtnl_lock)
					unregister_netdev(ifp->net);
				else
					unregister_netdevice(ifp->net);
			}
			ifp->net = NULL;
		}
#ifdef DHD_WMF
		dhd_wmf_cleanup(dhdpub, ifidx);
#endif /* DHD_WMF */

		dhd_if_del_sta_list(ifp);

		dhdinfo->iflist[ifidx] = NULL;
		MFREE(dhdinfo->pub.osh, ifp, sizeof(*ifp));
#ifdef BCM_PCIE_UPDATE
		ifp = NULL;
#endif
	}
#ifdef DHD_DEVWAKE_EARLY
	mutex_unlock(&dhdinfo->ifdel_mutex[ifidx]);
#endif
	return BCME_OK;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31))
static struct net_device_ops dhd_ops_pri = {
	.ndo_open = dhd_open,
	.ndo_stop = dhd_stop,
	.ndo_get_stats = dhd_get_stats,
	.ndo_do_ioctl = dhd_ioctl_entry,
	.ndo_start_xmit = dhd_start_xmit,
	.ndo_set_mac_address = dhd_set_mac_address,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0))
	.ndo_set_rx_mode = dhd_set_multicast_list,
#else
	.ndo_set_multicast_list = dhd_set_multicast_list,
#endif
};

static struct net_device_ops dhd_ops_virt = {
	.ndo_get_stats = dhd_get_stats,
	.ndo_do_ioctl = dhd_ioctl_entry,
	.ndo_start_xmit = dhd_start_xmit,
	.ndo_set_mac_address = dhd_set_mac_address,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0))
	.ndo_set_rx_mode = dhd_set_multicast_list,
#else
	.ndo_set_multicast_list = dhd_set_multicast_list,
#endif
};
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)) */

#ifdef DEBUGGER
extern void debugger_init(void *bus_handle);
#endif


#ifdef SHOW_LOGTRACE
#define DEFAULT_LOG_STR_PATH "/vendor/firmware/logstrs.bin"
static char logstrs_path[MOD_PARAM_PATHLEN] = DEFAULT_LOG_STR_PATH;

module_param_string(logstrs_path, logstrs_path, MOD_PARAM_PATHLEN, 0660);

int
dhd_init_logstrs_array(dhd_event_log_t *temp)
{
	struct file *filep = NULL;
	struct kstat stat;
	mm_segment_t fs;
	char *raw_fmts =  NULL;
	int logstrs_size = 0;
	gfp_t kflags;
	logstr_header_t *hdr = NULL;
	uint32 *lognums = NULL;
	char *logstrs = NULL;
	int ram_index = 0;
	char **fmts;
	int num_fmts = 0;
	uint32 i = 0;
	int error = 0;

	if (temp->fmts && temp->raw_fmts) {
		return BCME_OK;
	}
	kflags = in_atomic() ? GFP_ATOMIC : GFP_KERNEL;

	/* Save previous address limit first and then change to KERNEL_DS address limit */
	fs = get_fs();
	set_fs(KERNEL_DS);

	filep = filp_open(logstrs_path, O_RDONLY, 0);
	if (IS_ERR(filep)) {
		DHD_ERROR(("Failed to open the file logstrs.bin in %s, %s\n",  __FUNCTION__, logstrs_path));
		goto fail;
	}
	error = vfs_stat(logstrs_path, &stat);
	if (error) {
		DHD_ERROR(("Failed in %s to find file stat\n", __FUNCTION__));
		goto fail;
	}
	logstrs_size = (int) stat.size;

	raw_fmts = kmalloc(logstrs_size, kflags);
	if (raw_fmts == NULL) {
		DHD_ERROR(("Failed to allocate raw_fmts memory\n"));
		goto fail;
	}
	if (vfs_read(filep, raw_fmts, logstrs_size, &filep->f_pos) !=	logstrs_size) {
		DHD_ERROR(("Error: Log strings file read failed\n"));
		goto fail;
	}

	/* Remember header from the logstrs.bin file */
	hdr = (logstr_header_t *) (raw_fmts + logstrs_size -
		sizeof(logstr_header_t));

	if (hdr->log_magic == LOGSTRS_MAGIC) {
		/*
		* logstrs.bin start with header.
		*/
		num_fmts =	hdr->rom_logstrs_offset / sizeof(uint32);
		ram_index = (hdr->ram_lognums_offset -
			hdr->rom_lognums_offset) / sizeof(uint32);
		lognums = (uint32 *) &raw_fmts[hdr->rom_lognums_offset];
		logstrs = (char *)	 &raw_fmts[hdr->rom_logstrs_offset];
	} else {
		/*
		 * Legacy logstrs.bin format without header.
		 */
		num_fmts = *((uint32 *) (raw_fmts)) / sizeof(uint32);
		if (num_fmts == 0) {
			/* Legacy ROM/RAM logstrs.bin format:
			  *  - ROM 'lognums' section
			  *   - RAM 'lognums' section
			  *   - ROM 'logstrs' section.
			  *   - RAM 'logstrs' section.
			  *
			  * 'lognums' is an array of indexes for the strings in the
			  * 'logstrs' section. The first uint32 is 0 (index of first
			  * string in ROM 'logstrs' section).
			  *
			  * The 4324b5 is the only ROM that uses this legacy format. Use the
			  * fixed number of ROM fmtnums to find the start of the RAM
			  * 'lognums' section. Use the fixed first ROM string ("Con\n") to
			  * find the ROM 'logstrs' section.
			  */
			#define NUM_4324B5_ROM_FMTS	186
			#define FIRST_4324B5_ROM_LOGSTR "Con\n"
			ram_index = NUM_4324B5_ROM_FMTS;
			lognums = (uint32 *) raw_fmts;
			num_fmts =	ram_index;
			logstrs = (char *) &raw_fmts[num_fmts << 2];
			while (strncmp(FIRST_4324B5_ROM_LOGSTR, logstrs, 4)) {
				num_fmts++;
				logstrs = (char *) &raw_fmts[num_fmts << 2];
			}
		} else {
				/* Legacy RAM-only logstrs.bin format:
				 *	  - RAM 'lognums' section
				 *	  - RAM 'logstrs' section.
				 *
				 * 'lognums' is an array of indexes for the strings in the
				 * 'logstrs' section. The first uint32 is an index to the
				 * start of 'logstrs'. Therefore, if this index is divided
				 * by 'sizeof(uint32)' it provides the number of logstr
				 *	entries.
				 */
				ram_index = 0;
				lognums = (uint32 *) raw_fmts;
				logstrs = (char *)	&raw_fmts[num_fmts << 2];
			}
	}
	fmts = kmalloc(num_fmts  * sizeof(char *), kflags);
	if (fmts == NULL) {
		DHD_ERROR(("Failed to allocate fmts memory"));
		goto fail;
	}

	for (i = 0; i < num_fmts; i++) {
		/* ROM lognums index into logstrs using 'rom_logstrs_offset' as a base
		* (they are 0-indexed relative to 'rom_logstrs_offset').
		*
		* RAM lognums are already indexed to point to the correct RAM logstrs (they
		* are 0-indexed relative to the start of the logstrs.bin file).
		*/
		if (i == ram_index) {
			logstrs = raw_fmts;
		}
		fmts[i] = &logstrs[lognums[i]];
	}
	temp->fmts = fmts;
	temp->raw_fmts = raw_fmts;
	temp->num_fmts = num_fmts;
	filp_close(filep, NULL);
	set_fs(fs);
	return 0;
fail:
	if (raw_fmts) {
		kfree(raw_fmts);
		raw_fmts = NULL;
	}
	if (!IS_ERR(filep))
		filp_close(filep, NULL);

	/* Restore previous address limit */
	set_fs(fs);

	temp->fmts = NULL;
	return -1;
}
#endif /* SHOW_LOGTRACE */

#ifdef BCM_BLOCK_DATA_FRAME
static void dhd_clear_blockframe_flag(ulong data)
{
	dhd_pub_t *dhd = (dhd_pub_t *)data;
	if (dhd) {
		DHD_ERROR(("%s: block frame time out, clear flag\n", __func__));
		dhd->blockframe_flag = 0;
	}
}

void dhd_set_blockframe(dhd_pub_t *dhd, int enable, int timer, int ifindex)
{
	dhd->blockframe_flag = enable;

	if (enable){
#ifdef PROP_TXSTATUS
		dhd_wlfc_cleanup(dhd, NULL, 0);
#endif
#ifdef PCIE_FULL_DONGLE
		dhd_flow_rings_delete(dhd, ifindex);
#endif
		osl_sleep(20);

	/* timer is defaults to 1000 */
		if (timer <= 0)
			timer = 1000;
		mod_timer(&dhd->block_timer, jiffies + msecs_to_jiffies(timer));
	}
	else
		del_timer_sync(&dhd->block_timer);
}
#endif
#ifdef HW_PCIE_NOCLOG
dhd_pub_t *dhdp_global;
#endif
dhd_pub_t *
dhd_attach(osl_t *osh, struct dhd_bus *bus, uint bus_hdrlen)
{
	dhd_info_t *dhd = NULL;
	struct net_device *net = NULL;
	char if_name[IFNAMSIZ] = {'\0'};
	uint32 bus_type = -1;
	uint32 bus_num = -1;
	uint32 slot_num = -1;
	wifi_adapter_info_t *adapter = NULL;
#ifdef DHD_DEVWAKE_EARLY
	int i;
#endif
#ifdef HW_WIFI_DRIVER_NORMALIZE
	char  chip_type_string[CHIP_NAME_MAX_LEN];
#endif /* HW_WIFI_DRIVER_NORMALIZE */

	dhd_attach_states_t dhd_state = DHD_ATTACH_STATE_INIT;
	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	/* will implement get_ids for DBUS later */
#if defined(BCMSDIO)
	dhd_bus_get_ids(bus, &bus_type, &bus_num, &slot_num);
#endif
	adapter = dhd_wifi_platform_get_adapter(bus_type, bus_num, slot_num);

	/* Allocate primary dhd_info */
	dhd = wifi_platform_prealloc(adapter, DHD_PREALLOC_DHD_INFO, sizeof(dhd_info_t));
	if (dhd == NULL) {
		dhd = MALLOC(osh, sizeof(dhd_info_t));
		if (dhd == NULL) {
			DHD_ERROR(("%s: OOM - alloc dhd_info\n", __FUNCTION__));
			goto fail;
		}
	}
	memset(dhd, 0, sizeof(dhd_info_t));
	dhd_state |= DHD_ATTACH_STATE_DHD_ALLOC;

	dhd->unit = dhd_found + instance_base; /* do not increment dhd_found, yet */

	dhd->pub.osh = osh;
	dhd->adapter = adapter;
#ifdef HW_WIFI_DRIVER_NORMALIZE
    memset(chip_type_string, 0x0, sizeof(chip_type_string));
	wifi_platform_get_chip_type(adapter, chip_type_string, sizeof(chip_type_string)-1);
	if ( chip_type_string[0] != '\0' ) {
		if(0 == strncmp(chip_type_string, "bcm43455", strlen("bcm43455"))){
	        g_gscan_maxap_per_scan = GSCAN_MAX_AP_PER_SCAN_43455;
	    }
	}
	DHD_ERROR(("gscan max ap per scan is %d \n", g_gscan_maxap_per_scan));
#endif /* HW_WIFI_DRIVER_NORMALIZE */		
#ifndef HW_SKIP_ATTACH_GET_MAC
#ifdef GET_CUSTOM_MAC_ENABLE
	wifi_platform_get_mac_addr(dhd->adapter, dhd->pub.mac.octet);
#endif /* GET_CUSTOM_MAC_ENABLE */
#endif
#ifdef CUSTOM_FORCE_NODFS_FLAG
	dhd->pub.dhd_cflags |= WLAN_PLAT_NODFS_FLAG;
	dhd->pub.force_country_change = TRUE;
#endif
#ifdef CUSTOM_COUNTRY_CODE
	get_customized_country_code(dhd->adapter,
		dhd->pub.dhd_cspec.country_abbrev, &dhd->pub.dhd_cspec,
		dhd->pub.dhd_cflags);
#endif /* CUSTOM_COUNTRY_CODE */

	dhd->pub.short_dwell_time = -1;

	dhd->thr_dpc_ctl.thr_pid = DHD_PID_KT_TL_INVALID;
	dhd->thr_wdt_ctl.thr_pid = DHD_PID_KT_INVALID;

	/* Initialize thread based operation and lock */
	mutex_init(&dhd->sdmutex);

	/* Some DHD modules (e.g. cfg80211) configures operation mode based on firmware name.
	 * This is indeed a hack but we have to make it work properly before we have a better
	 * solution
	 */
	dhd_update_fw_nv_path(dhd);

	/* Link to info module */
	dhd->pub.info = dhd;


	/* Link to bus module */
	dhd->pub.bus = bus;
	dhd->pub.hdrlen = bus_hdrlen;

#ifdef HW_SDIO_QUALITY_STATISTIC
	sdio_quality_init(&dhd->pub);
#endif

	/* Set network interface name if it was provided as module parameter */
	if (iface_name[0]) {
		int len;
		char ch;
		strncpy(if_name, iface_name, IFNAMSIZ);
		if_name[IFNAMSIZ - 1] = 0;
		len = strlen(if_name);
		ch = if_name[len - 1];
		if ((ch > '9' || ch < '0') && (len < IFNAMSIZ - 2))
			strcat(if_name, "%d");
	}
#ifdef DHD_DEVWAKE_EARLY
	/* init all delete mutex protection of all ifs */
	for (i = 0; i < DHD_MAX_IFS; i++) {
		DHD_ERROR(("%s: init ifdel_mutex(%d)\n", __FUNCTION__, i));
		mutex_init(&dhd->ifdel_mutex[i]);
	}
#endif
#ifndef  BRCM_RSDB
	net = dhd_allocate_if(&dhd->pub, 0, if_name, NULL, 0, TRUE);
#else
	net = dhd_allocate_if(&dhd->pub, 0, if_name, NULL, 0, TRUE, NULL);
#endif
	if (net == NULL)
		goto fail;
	dhd_state |= DHD_ATTACH_STATE_ADD_IF;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 31))
	net->open = NULL;
#else
	net->netdev_ops = NULL;
#endif
#ifdef BCM_PCIE_UPDATE
	mutex_init(&dhd->dhd_iovar_mutex);
#endif
	sema_init(&dhd->proto_sem, 1);

#ifdef PROP_TXSTATUS
	spin_lock_init(&dhd->wlfc_spinlock);

	dhd->pub.skip_fc = dhd_wlfc_skip_fc;
	dhd->pub.plat_init = dhd_wlfc_plat_init;
	dhd->pub.plat_deinit = dhd_wlfc_plat_deinit;
#ifdef WLFC_STATE_PREALLOC
	dhd->pub.wlfc_state = MALLOC(dhd->pub.osh, sizeof(athost_wl_status_info_t));
	if (dhd->pub.wlfc_state == NULL)
		DHD_ERROR(("%s: wlfc_state prealloc failed\n", __FUNCTION__));
#endif /* WLFC_STATE_PREALLOC */
#endif /* PROP_TXSTATUS */

	/* Initialize other structure content */
	init_waitqueue_head(&dhd->ioctl_resp_wait);
	init_waitqueue_head(&dhd->d3ack_wait);
	init_waitqueue_head(&dhd->ctrl_wait);
#ifdef BCM_PCIE_UPDATE
#ifdef DHD_DEVWAKE_EARLY
	init_waitqueue_head(&dhd->devwake_wait);
#endif /* DHD_DEVWAKE_EARLY */
	init_waitqueue_head(&dhd->dhd_bus_busy_state_wait);
	dhd->pub.dhd_bus_busy_state = 0;
#endif
	/* Initialize the spinlocks */
	spin_lock_init(&dhd->sdlock);
	spin_lock_init(&dhd->txqlock);
	spin_lock_init(&dhd->dhd_lock);
	spin_lock_init(&dhd->rxf_lock);
#if defined(RXFRAME_THREAD)
	dhd->rxthread_enabled = TRUE;
#endif /* defined(RXFRAME_THREAD) */

#ifdef DHDTCPACK_SUPPRESS
	spin_lock_init(&dhd->tcpack_lock);
#endif /* DHDTCPACK_SUPPRESS */

	/* Initialize Wakelock stuff */
	spin_lock_init(&dhd->wakelock_spinlock);
#ifndef BCM_PCIE_UPDATE
	dhd->wakelock_counter = 0;
#else
	spin_lock_init(&dhd->wakelock_evt_spinlock);
	DHD_OS_WAKE_LOCK_INIT(dhd);
#endif
	dhd->wakelock_wd_counter = 0;
#ifndef BCM_PCIE_UPDATE
	dhd->wakelock_rx_timeout_enable = 0;
	dhd->wakelock_ctrl_timeout_enable = 0;
#endif
#ifdef CONFIG_HAS_WAKELOCK
#ifndef BCM_PCIE_UPDATE
	wake_lock_init(&dhd->wl_wifi, WAKE_LOCK_SUSPEND, "wlan_wake");
	wake_lock_init(&dhd->wl_rxwake, WAKE_LOCK_SUSPEND, "wlan_rx_wake");
	wake_lock_init(&dhd->wl_ctrlwake, WAKE_LOCK_SUSPEND, "wlan_ctrl_wake");
#endif
	wake_lock_init(&dhd->wl_wdwake, WAKE_LOCK_SUSPEND, "wlan_wd_wake");
#endif /* CONFIG_HAS_WAKELOCK */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25))
#ifdef HW_WIFI_OPEN_STOP
	mutex_init(&dhd->dhd_open_stop_if_mutex);
#endif
	mutex_init(&dhd->dhd_net_if_mutex);
	mutex_init(&dhd->dhd_suspend_mutex);
#endif
#if defined(PKT_FILTER_SUPPORT) && defined(BCM_APF)
	mutex_init(&dhd->dhd_apf_mutex);
#endif /* PKT_FILTER_SUPPORT && BCM_APF */
	dhd_state |= DHD_ATTACH_STATE_WAKELOCKS_INIT;

	/* Attach and link in the protocol */
	if (dhd_prot_attach(&dhd->pub) != 0) {
		DHD_ERROR(("dhd_prot_attach failed\n"));
		goto fail;
	}
	dhd_state |= DHD_ATTACH_STATE_PROT_ATTACH;

#ifdef WL_CFG80211
	/* Attach and link in the cfg80211 */
	if (unlikely(wl_cfg80211_attach(net, &dhd->pub))) {
		DHD_ERROR(("wl_cfg80211_attach failed\n"));
		goto fail;
	}

	dhd_monitor_init(&dhd->pub);
	dhd_state |= DHD_ATTACH_STATE_CFG80211;
#endif
	/* attach debug support */
	if (dhd_os_dbg_attach(&dhd->pub)) {
		DHD_ERROR(("%s debug module attach failed\n", __FUNCTION__));
		goto fail;
	}

	if (dhd_sta_pool_init(&dhd->pub, DHD_MAX_STA) != BCME_OK) {
		DHD_ERROR(("%s: Initializing %u sta\n", __FUNCTION__, DHD_MAX_STA));
		goto fail;
	}

	/* Set up the watchdog timer */
	init_timer(&dhd->timer);
	dhd->timer.data = (ulong)dhd;
	dhd->timer.function = dhd_watchdog;
	dhd->default_wd_interval = dhd_watchdog_ms;
#ifdef BCM_BLOCK_DATA_FRAME
	init_timer(&dhd->pub.block_timer);
	dhd->pub.block_timer.data = (ulong)&dhd->pub;
	dhd->pub.block_timer.function = dhd_clear_blockframe_flag;
#endif
	if (dhd_watchdog_prio >= 0) {
		/* Initialize watchdog thread */
		PROC_START(dhd_watchdog_thread, dhd, &dhd->thr_wdt_ctl, 0, "dhd_watchdog_thread");
#ifdef BCM_PCIE_UPDATE
		if (dhd->thr_wdt_ctl.thr_pid < 0) {
			goto fail;
		}
#endif
	} else {
		dhd->thr_wdt_ctl.thr_pid = -1;
	}

#ifdef DEBUGGER
	debugger_init((void *) bus);
#endif

	/* Set up the bottom half handler */
	if (dhd_dpc_prio >= 0) {
		/* Initialize DPC thread */
		PROC_START(dhd_dpc_thread, dhd, &dhd->thr_dpc_ctl, 0, "dhd_dpc");
#ifdef BCM_PCIE_UPDATE
		if (dhd->thr_dpc_ctl.thr_pid < 0) {
			goto fail;
		}
#endif
	} else {
		/*  use tasklet for dpc */
		tasklet_init(&dhd->tasklet, dhd_dpc, (ulong)dhd);
		dhd->thr_dpc_ctl.thr_pid = -1;
	}

	if (dhd->rxthread_enabled) {
		bzero(&dhd->pub.skbbuf[0], sizeof(void *) * MAXSKBPEND);
		/* Initialize RXF thread */
		PROC_START(dhd_rxf_thread, dhd, &dhd->thr_rxf_ctl, 0, "dhd_rxf");
#ifdef BCM_PCIE_UPDATE
		if (dhd->thr_rxf_ctl.thr_pid < 0) {
			goto fail;
		}
#endif
	}
#ifdef DHD_DEVWAKE_EARLY
	dhd->devwake_tsk.thr_pid = -1;
	PROC_START(dhd_devwake_thread, dhd, &dhd->devwake_tsk, 0, "dhd_devwake_thread");
	if (dhd->devwake_tsk.thr_pid < 0) {
		goto fail;
	}
#endif /* DHD_DEVWAKE_EARLY */
#ifdef DHD_TPUT_MONITOR
	dhd->tput_tsk.thr_pid = -1;
	init_timer(&dhd->tput_timer);
	dhd->tput_timer.data = (ulong)dhd;
	dhd->tput_timer.function = dhd_tput_monitor_func;
	PROC_START(dhd_tput_monitor_thread, dhd, &dhd->tput_tsk, 0, "dhd_tput_monitor_thread");
#endif /* DHD_TPUT_MONITOR */

	dhd_state |= DHD_ATTACH_STATE_THREADS_CREATED;

#if defined(CONFIG_PM_SLEEP)
	if (!dhd_pm_notifier_registered) {
		dhd_pm_notifier_registered = TRUE;
		register_pm_notifier(&dhd_pm_notifier);
	}
#endif /* CONFIG_PM_SLEEP */
#ifdef SAR_SUPPORT
	dhd->sar_notifier.notifier_call = dhd_sar_callback;
	if (!dhd_sar_notifier_registered) {
		dhd_sar_notifier_registered = TRUE;
		dhd->sar_enable = 1;		/* unknown state value */
		register_notifier_by_sar(&dhd->sar_notifier);
	}
#endif /* SAR_SUPPORT */
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(DHD_USE_EARLYSUSPEND)
	dhd->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 20;
	dhd->early_suspend.suspend = dhd_early_suspend;
	dhd->early_suspend.resume = dhd_late_resume;
	register_early_suspend(&dhd->early_suspend);
	dhd_state |= DHD_ATTACH_STATE_EARLYSUSPEND_DONE;
#endif /* CONFIG_HAS_EARLYSUSPEND && DHD_USE_EARLYSUSPEND */

#if !defined(CONFIG_HAS_EARLYSUSPEND) && defined(HW_AP_POWERSAVE)
	INIT_WORK(&dhd->pub.ap_suspend_work, hw_suspend_work_handler);
	dhd->fb_notifier.notifier_call = dhd_fb_notify;
	fb_register_client(&dhd->fb_notifier);
#endif

#ifdef ARP_OFFLOAD_SUPPORT
	dhd->pend_ipaddr = 0;
	if (!dhd_inetaddr_notifier_registered) {
		dhd_inetaddr_notifier_registered = TRUE;
		register_inetaddr_notifier(&dhd_inetaddr_notifier);
	}
#endif /* ARP_OFFLOAD_SUPPORT */
#ifdef CONFIG_IPV6
	if (!dhd_inet6addr_notifier_registered) {
		dhd_inet6addr_notifier_registered = TRUE;
		register_inet6addr_notifier(&dhd_inet6addr_notifier);
	}
#endif
	dhd->dhd_deferred_wq = dhd_deferred_work_init((void *)dhd);
#ifdef DEBUG_CPU_FREQ
	dhd->new_freq = alloc_percpu(int);
	dhd->freq_trans.notifier_call = dhd_cpufreq_notifier;
	cpufreq_register_notifier(&dhd->freq_trans, CPUFREQ_TRANSITION_NOTIFIER);
#endif
#ifdef DHDTCPACK_SUPPRESS
#ifdef BCMSDIO
	dhd_tcpack_suppress_set(&dhd->pub, TCPACK_SUP_REPLACE);
#elif defined(BCMPCIE)
	dhd_tcpack_suppress_set(&dhd->pub, TCPACK_SUP_REPLACE);
#else
	dhd_tcpack_suppress_set(&dhd->pub, TCPACK_SUP_OFF);
#endif /* BCMSDIO */
#endif /* DHDTCPACK_SUPPRESS */

	dhd_state |= DHD_ATTACH_STATE_DONE;
	dhd->dhd_state = dhd_state;

	dhd_found++;
#ifdef CONFIG_HW_WLANFTY_STATUS
	hw_wlanfty_attach(&dhd->pub);
#endif
#ifdef HW_DOZE_PKT_FILTER
	hw_attach_dhd_pub_t(&dhd->pub);
#endif
#ifdef DHD_TRACE_WAKE_LOCK
	(void)dhd_sysfs_init(dhd);
#endif
#ifdef HW_PCIE_NOCLOG
	dhdp_global = &dhd->pub;
#endif
	return &dhd->pub;

fail:
	if (dhd_state >= DHD_ATTACH_STATE_DHD_ALLOC) {
		DHD_TRACE(("%s: Calling dhd_detach dhd_state 0x%x &dhd->pub %p\n",
			__FUNCTION__, dhd_state, &dhd->pub));
		dhd->dhd_state = dhd_state;
		dhd_detach(&dhd->pub);
		dhd_free(&dhd->pub);
	}

	return NULL;
}

int dhd_get_fw_mode(dhd_info_t *dhdinfo)
{
	if (strstr(dhdinfo->fw_path, "_apsta") != NULL)
		return DHD_FLAG_HOSTAP_MODE;
	if (strstr(dhdinfo->fw_path, "_p2p") != NULL)
		return DHD_FLAG_P2P_MODE;
	if (strstr(dhdinfo->fw_path, "_ibss") != NULL)
		return DHD_FLAG_IBSS_MODE;
	if (strstr(dhdinfo->fw_path, "_mfg") != NULL)
		return DHD_FLAG_MFG_MODE;

	return DHD_FLAG_STA_MODE;
}
#ifdef NVRAM_FROM_DTS
#define REGION_CHINA  "CN"
const char suffix_oversea[13]="_oversea.txt";
#endif
bool dhd_update_fw_nv_path(dhd_info_t *dhdinfo)
{
	int fw_len;
	int nv_len;
	const char *fw = NULL;
	const char *nv = NULL;
	wifi_adapter_info_t *adapter = dhdinfo->adapter;

#ifdef NVRAM_FROM_DTS
	char  nv_path_string[NV_PATH_MAX_LEN];
	char  nv_path_string_bak[NV_PATH_MAX_LEN];
	void * image = NULL;
#endif
#ifdef HW_WIFI_DRIVER_NORMALIZE
    char  fw_path_string[FW_PATH_MAX_LEN];
#endif /* HW_WIFI_DRIVER_NORMALIZE */


	/* Update firmware and nvram path. The path may be from adapter info or module parameter
	 * The path from adapter info is used for initialization only (as it won't change).
	 *
	 * The firmware_path/nvram_path module parameter may be changed by the system at run
	 * time. When it changes we need to copy it to dhdinfo->fw_path. Also Android private
	 * command may change dhdinfo->fw_path. As such we need to clear the path info in
	 * module parameter after it is copied. We won't update the path until the module parameter
	 * is changed again (first character is not '\0')
	 */

	/* set default firmware and nvram path for built-in type driver */
	if (!dhd_download_fw_on_driverload) {
#ifdef HW_WIFI_DRIVER_NORMALIZE
	    memset(fw_path_string, 0x0, sizeof(fw_path_string));
	    wifi_platform_get_fw_path(adapter, fw_path_string, sizeof(fw_path_string)-1);
		if ( fw_path_string[0] != '\0' ) {
		    fw = fw_path_string;
		}
#else
#ifdef CONFIG_BCMDHD_FW_PATH
		fw = CONFIG_BCMDHD_FW_PATH;
#endif /* CONFIG_BCMDHD_FW_PATH */
#ifdef CONFIG_BCMDHD_NVRAM_PATH
		nv = CONFIG_BCMDHD_NVRAM_PATH;
#endif /* CONFIG_BCMDHD_NVRAM_PATH */

#endif /*HW_WIFI_DRIVER_NORMALIZE*/
	}

	/* check if we need to initialize the path */
	if (dhdinfo->fw_path[0] == '\0') {
		if (adapter && adapter->fw_path && adapter->fw_path[0] != '\0')
			fw = adapter->fw_path;

	}
	if (dhdinfo->nv_path[0] == '\0') {
		if (adapter && adapter->nv_path && adapter->nv_path[0] != '\0')
			nv = adapter->nv_path;
	}

#ifdef NVRAM_FROM_DTS
	memset(nv_path_string, 0x0, sizeof(nv_path_string));
	wifi_platform_get_nvram_path(adapter, nv_path_string, sizeof(nv_path_string)-1);
	if ( nv_path_string[0] != '\0' ) {
		DHD_ERROR(("%s:get auto nvram_path=%s\n", __FUNCTION__, nv_path_string));
		nv = nv_path_string;

		/* Overseas version to modify nvpath ,region is not null and region is not CN(CHINA)*/
		if ((region[0] != '\0')&&(strcmp(region, REGION_CHINA) != 0)) {
			/* According to DTS nvpath to modify foreign nvpath.
			* If oversea nvpath load failure,reload DTS nvpath */
			strncpy(nv_path_string_bak, nv_path_string, sizeof(nv_path_string));
			nv_path_string_bak[NV_PATH_MAX_LEN - 1] = '\0';
			nv_len = strlen(nv_path_string_bak);
#ifdef HW_LP_OVERSEA
			hw_set_region_oversea(1);
#endif //HW_LP_OVERSEA
			/*change suffix from .txt to _oversea.txt*/
			if (nv_path_string_bak[nv_len-4] == '.') {
				nv_path_string_bak[nv_len-4] = '\0';
				strncat(nv_path_string_bak, suffix_oversea, sizeof(suffix_oversea));
				DHD_ERROR(("%s: modify nvram_path oversea=%s\n", __FUNCTION__, nv_path_string_bak));

				/* whether nvram file exists*/
				image = dhd_os_open_image(nv_path_string_bak);
				if (image == NULL) {/*file not exist*/
				       DHD_ERROR(("%s:nvram file is not exist,reload Init nvram file:%s\n", __FUNCTION__,nv_path_string_bak));
				} else {/*file exist*/
				       nv = nv_path_string_bak;
				       dhd_os_close_image(image);
				}
			} else {
				DHD_ERROR(("%s:File encoding format is not correct,reload Init nvram file\n", __FUNCTION__));
			}
#ifdef HW_LP_OVERSEA
		} else {
			hw_set_region_oversea(0);
		}
#else
		}
#endif //HW_LP_OVERSEA
	}
#endif

	if ( fw_bak_path[0] != '\0' ) {
		fw = fw_bak_path;
	}
	if ( nv_bak_path[0] != '\0' ) {
		nv = nv_bak_path;
	}

	/* Use module parameter if it is valid, EVEN IF the path has not been initialized
	 *
	 * TODO: need a solution for multi-chip, can't use the same firmware for all chips
	 */
	if (firmware_path[0] != '\0') {
		fw = firmware_path;
		memset(fw_bak_path, 0, sizeof(fw_bak_path));
		strncpy(fw_bak_path, firmware_path, MOD_PARAM_PATHLEN);
		fw_bak_path[MOD_PARAM_PATHLEN - 1] = '\0';
#if defined(SOFTAP) && defined(HW_SOFTAP_MANAGEMENT)
		memset(g_fw_path, 0, sizeof(g_fw_path));
		strncpy(g_fw_path, firmware_path, sizeof(g_fw_path)-1);
#endif
	}

	if (nvram_path[0] != '\0') {
		nv = nvram_path;
		strncpy(nv_bak_path, nvram_path, MOD_PARAM_PATHLEN);
		nv_bak_path[MOD_PARAM_PATHLEN-1] = '\0';
	}

	if (fw && fw[0] != '\0') {
		fw_len = strlen(fw);
		if (fw_len >= sizeof(dhdinfo->fw_path)) {
			DHD_ERROR(("fw path len exceeds max len of dhdinfo->fw_path\n"));
			return FALSE;
		}
		strncpy(dhdinfo->fw_path, fw, sizeof(dhdinfo->fw_path));
		if (dhdinfo->fw_path[fw_len-1] == '\n')
		       dhdinfo->fw_path[fw_len-1] = '\0';
	}
	if (nv && nv[0] != '\0') {
		nv_len = strlen(nv);
		if (nv_len >= sizeof(dhdinfo->nv_path)) {
			DHD_ERROR(("nvram path len exceeds max len of dhdinfo->nv_path\n"));
			return FALSE;
		}
		strncpy(dhdinfo->nv_path, nv, sizeof(dhdinfo->nv_path));
		if (dhdinfo->nv_path[nv_len-1] == '\n')
		       dhdinfo->nv_path[nv_len-1] = '\0';
	}

	/* clear the path in module parameter */
#ifndef BCM_PCIE_UPDATE
	firmware_path[0] = '\0';
	nvram_path[0] = '\0';
#else
	if (dhd_download_fw_on_driverload) {
		firmware_path[0] = '\0';
		nvram_path[0] = '\0';
	}
#endif
#ifndef BCMEMBEDIMAGE
	/* fw_path and nv_path are not mandatory for BCMEMBEDIMAGE */
	if (dhdinfo->fw_path[0] == '\0') {
		DHD_ERROR(("firmware path not found\n"));
		return FALSE;
	}
	if (dhdinfo->nv_path[0] == '\0') {
		DHD_ERROR(("nvram path not found\n"));
		return FALSE;
	}

	DHD_ERROR(("%s:dhdinfo->fw_path=%s\n", __FUNCTION__, dhdinfo->fw_path ));
	DHD_ERROR(("%s:dhdinfo->nv_path=%s\n", __FUNCTION__, dhdinfo->nv_path ));
#if defined(SOFTAP) && defined(HW_SOFTAP_MANAGEMENT)
	DHD_ERROR(("%s:g_fw_path=%s\n", __FUNCTION__, g_fw_path));
#endif
#endif /* BCMEMBEDIMAGE */

	return TRUE;
}


int
dhd_bus_start(dhd_pub_t *dhdp)
{
	int ret = -1;
	dhd_info_t *dhd = (dhd_info_t*)dhdp->info;
	unsigned long flags;

	ASSERT(dhd);
	if (NULL == dhd)
		return;

	DHD_TRACE(("Enter %s:\n", __FUNCTION__));
#ifdef HW_WIFI_DMD_LOG
	hw_dmd_trace_log("Enter %s:", __FUNCTION__);
#endif

	DHD_PERIM_LOCK(dhdp);

	/* try to download image and nvram to the dongle */
	if  (dhd->pub.busstate == DHD_BUS_DOWN && dhd_update_fw_nv_path(dhd)) {
#ifdef BCM_PCIE_UPDATE
		/* Indicate FW Download has not yet done */
		dhd->pub.is_fw_download_done = FALSE;
#endif
		DHD_INFO(("%s download fw %s, nv %s\n", __FUNCTION__, dhd->fw_path, dhd->nv_path));
		ret = dhd_bus_download_firmware(dhd->pub.bus, dhd->pub.osh,
		                                dhd->fw_path, dhd->nv_path);
		if (ret < 0) {
			DHD_ERROR(("%s: failed to download firmware %s\n",
			          __FUNCTION__, dhd->fw_path));
#ifdef HW_WIFI_DMD_LOG
			hw_dmd_trace_log("failed to download firmware %s", (NULL == dhd)? "null": dhd->fw_path);
			hw_wifi_dsm_client_notify(DSM_WIFI_SDIO_FIRMWARE_DL_ERROR,
			          "dhdsdio_download_firmware failed\n");
#endif
			DHD_PERIM_UNLOCK(dhdp);
			return ret;
		}
#ifdef BCM_PCIE_UPDATE
		/* Indicate FW Download has succeeded */
		dhd->pub.is_fw_download_done = TRUE;
#endif
#ifdef HW_WIFI_DMD_LOG
		hw_dmd_trace_log("%s: download firmware ok", __FUNCTION__);
#endif
	}
#ifdef SHOW_LOGTRACE
	dhd_init_logstrs_array(&dhd->event_data);
#endif /* SHOW_LOGTRACE */
	if (dhd->pub.busstate != DHD_BUS_LOAD) {
		DHD_PERIM_UNLOCK(dhdp);
		return -ENETDOWN;
	}

	dhd_os_sdlock(dhdp);

	/* Start the watchdog timer */
	dhd->pub.tickcnt = 0;
	dhd_os_wd_timer(&dhd->pub, dhd_watchdog_ms);

	/* Bring up the bus */
	if ((ret = dhd_bus_init(&dhd->pub, FALSE)) != 0) {

		DHD_ERROR(("%s, dhd_bus_init failed %d\n", __FUNCTION__, ret));
		dhd_os_sdunlock(dhdp);
		DHD_PERIM_UNLOCK(dhdp);
#ifdef HW_WIFI_DMD_LOG
		hw_dmd_trace_log("dhd_bus_init failed");
#endif
		return ret;
	}
#if defined(OOB_INTR_ONLY) || defined(BCMPCIE_OOB_HOST_WAKE)
#if defined(BCMPCIE_OOB_HOST_WAKE)
	dhd_os_sdunlock(dhdp);
#endif /* BCMPCIE_OOB_HOST_WAKE */
	/* Host registration for OOB interrupt */
	if (dhd_bus_oob_intr_register(dhdp)) {
		/* deactivate timer and wait for the handler to finish */
#if !defined(BCMPCIE_OOB_HOST_WAKE)
		DHD_GENERAL_LOCK(&dhd->pub, flags);
		dhd->wd_timer_valid = FALSE;
		DHD_GENERAL_UNLOCK(&dhd->pub, flags);
		del_timer_sync(&dhd->timer);

		DHD_ERROR(("%s Host failed to register for OOB\n", __FUNCTION__));
		dhd_os_sdunlock(dhdp);
#endif /* BCMPCIE_OOB_HOST_WAKE */
		DHD_PERIM_UNLOCK(dhdp);
		DHD_OS_WD_WAKE_UNLOCK(&dhd->pub);
		DHD_ERROR(("%s Host failed to register for OOB\n", __FUNCTION__));
#ifdef HW_WIFI_DMD_LOG
		hw_dmd_trace_log("Host failed to register for OOB");
#endif
		return -ENODEV;
	}

#if defined(BCMPCIE_OOB_HOST_WAKE)
	dhd_os_sdlock(dhdp);
	dhd_bus_oob_intr_set(dhdp, TRUE);
#else
	/* Enable oob at firmware */
	dhd_enable_oob_intr(dhd->pub.bus, TRUE);
#endif /* BCMPCIE_OOB_HOST_WAKE */
#endif
#ifndef BCM_PCIE_UPDATE 
#ifdef PCIE_FULL_DONGLE
	{
		uint8 txpush = 0;
		uint32 num_flowrings; /* includes H2D common rings */
		num_flowrings = dhd_bus_max_h2d_queues(dhd->pub.bus, &txpush);
		DHD_ERROR(("%s: Initializing %u flowrings\n", __FUNCTION__,
			num_flowrings));
		if ((ret = dhd_flow_rings_init(&dhd->pub, num_flowrings)) != BCME_OK) {
			DHD_PERIM_UNLOCK(dhdp);
#ifdef HW_WIFI_DMD_LOG
			hw_dmd_trace_log("Initializing flowrings failed");
#endif
			return ret;
		}
	}
#endif /* PCIE_FULL_DONGLE */

	/* Do protocol initialization necessary for IOCTL/IOVAR */
	dhd_prot_init(&dhd->pub);
#else
#ifdef PCIE_FULL_DONGLE
	{
		/* max_h2d_rings includes H2D common rings */
		uint32 max_h2d_rings = dhd_bus_max_h2d_queues(dhd->pub.bus);

		DHD_ERROR(("%s: Initializing %u h2drings\n", __FUNCTION__,
			max_h2d_rings));
		if ((ret = dhd_flow_rings_init(&dhd->pub, max_h2d_rings)) != BCME_OK) {
			dhd_os_sdunlock(dhdp);
			DHD_PERIM_UNLOCK(dhdp);
#ifdef HW_WIFI_DMD_LOG
			hw_dmd_trace_log("Initializing flowrings failed");
#endif
			return ret;
		}
	}
#endif /* PCIE_FULL_DONGLE */

	/* Do protocol initialization necessary for IOCTL/IOVAR */
#ifdef PCIE_FULL_DONGLE
	dhd_os_sdunlock(dhdp);
#endif /* PCIE_FULL_DONGLE */
	ret = dhd_prot_init(&dhd->pub);
	if (unlikely(ret) != BCME_OK) {
		DHD_PERIM_UNLOCK(dhdp);
		DHD_OS_WD_WAKE_UNLOCK(&dhd->pub);
		return ret;
	}
#ifdef PCIE_FULL_DONGLE
	dhd_os_sdlock(dhdp);
#endif /* PCIE_FULL_DONGLE */
#endif /* BCM_PCIE_UPDATE */

	/* If bus is not ready, can't come up */
	if (dhd->pub.busstate != DHD_BUS_DATA) {
		DHD_GENERAL_LOCK(&dhd->pub, flags);
		dhd->wd_timer_valid = FALSE;
		DHD_GENERAL_UNLOCK(&dhd->pub, flags);
		del_timer_sync(&dhd->timer);
		DHD_ERROR(("%s failed bus is not ready\n", __FUNCTION__));
		dhd_os_sdunlock(dhdp);
		DHD_PERIM_UNLOCK(dhdp);
		DHD_OS_WD_WAKE_UNLOCK(&dhd->pub);
#ifdef HW_WIFI_DMD_LOG
		hw_dmd_trace_log("failed bus is not ready");
#endif
		return -ENODEV;
	}

	dhd_os_sdunlock(dhdp);

	/* Bus is ready, query any dongle information */
	if ((ret = dhd_sync_with_dongle(&dhd->pub)) < 0) {
#ifdef BCM_PCIE_UPDATE
		DHD_GENERAL_LOCK(&dhd->pub, flags);
		dhd->wd_timer_valid = FALSE;
		DHD_GENERAL_UNLOCK(&dhd->pub, flags);
		del_timer_sync(&dhd->timer);
		DHD_ERROR(("%s failed to sync with dongle\n", __FUNCTION__));
		DHD_OS_WD_WAKE_UNLOCK(&dhd->pub);
#endif
		DHD_PERIM_UNLOCK(dhdp);
#ifdef HW_WIFI_DMD_LOG
		hw_dmd_trace_log("dhd_sync_with_dongle failed");
#endif
		return ret;
	}

#ifdef ARP_OFFLOAD_SUPPORT
	if (dhd->pend_ipaddr) {
#ifdef AOE_IP_ALIAS_SUPPORT
		aoe_update_host_ipv4_table(&dhd->pub, dhd->pend_ipaddr, TRUE, 0);
#endif /* AOE_IP_ALIAS_SUPPORT */
		dhd->pend_ipaddr = 0;
	}
#endif /* ARP_OFFLOAD_SUPPORT */

	DHD_PERIM_UNLOCK(dhdp);
	return 0;
}
#ifdef WLTDLS
int _dhd_tdls_enable(dhd_pub_t *dhd, bool tdls_on, bool auto_on, struct ether_addr *mac)
{
	uint32 tdls = tdls_on;
	int ret = 0;
	uint32 tdls_auto_op = 0;
	uint32 tdls_idle_time = CUSTOM_TDLS_IDLE_MODE_SETTING;
	int32 tdls_rssi_high = CUSTOM_TDLS_RSSI_THRESHOLD_HIGH;
	int32 tdls_rssi_low = CUSTOM_TDLS_RSSI_THRESHOLD_LOW;
	BCM_REFERENCE(mac);
	if (!FW_SUPPORTED(dhd, tdls))
		return BCME_ERROR;

	if (dhd->tdls_enable == tdls_on)
		goto auto_mode;
	ret = dhd_iovar(dhd, 0, "tdls_enable", (char *)&tdls, sizeof(tdls),
			NULL, 0, TRUE);
	if (ret < 0) {
		DHD_ERROR(("%s: tdls %d failed %d\n", __FUNCTION__, tdls, ret));
		goto exit;
	}
	dhd->tdls_enable = tdls_on;
auto_mode:

	tdls_auto_op = auto_on;
	ret = dhd_iovar(dhd, 0, "tdls_auto_op", (char *)&tdls_auto_op,
			sizeof(tdls_auto_op), NULL, 0, TRUE);
	if (ret < 0) {
		DHD_ERROR(("%s: tdls_auto_op failed %d\n", __FUNCTION__, ret));
		goto exit;
	}

	if (tdls_auto_op) {
		ret = dhd_iovar(dhd, 0, "tdls_idle_time",
				(char *)&tdls_idle_time, sizeof(tdls_idle_time),
				NULL, 0, TRUE);
		if (ret < 0) {
			DHD_ERROR(("%s: tdls_idle_time failed %d\n", __FUNCTION__, ret));
			goto exit;
		}
		ret = dhd_iovar(dhd, 0, "tdls_rssi_high",
				(char *)&tdls_rssi_high, sizeof(tdls_rssi_high),
				NULL, 0, TRUE);
		if (ret < 0) {
			DHD_ERROR(("%s: tdls_rssi_high failed %d\n", __FUNCTION__, ret));
			goto exit;
		}
		ret = dhd_iovar(dhd, 0, "tdls_rssi_low", (char *)&tdls_rssi_low,
				sizeof(tdls_rssi_low), NULL, 0, TRUE);
		if (ret < 0) {
			DHD_ERROR(("%s: tdls_rssi_low failed %d\n", __FUNCTION__, ret));
			goto exit;
		}
	}

exit:
	return ret;
}
int dhd_tdls_enable(struct net_device *dev, bool tdls_on, bool auto_on, struct ether_addr *mac)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	int ret = 0;
	if (dhd)
		ret = _dhd_tdls_enable(&dhd->pub, tdls_on, auto_on, mac);
	else
		ret = BCME_ERROR;
	return ret;
}
#ifdef  BRCM_RSDB
int
dhd_tdls_set_mode(dhd_pub_t *dhd, bool wfd_mode)
{
	char iovbuf[WLC_IOCTL_SMLEN];
	int ret = 0;
	bool auto_on = false;
	uint32 mode =  wfd_mode;

#ifdef ENABLE_TDLS_AUTO_MODE
	if (wfd_mode) {
		auto_on = false;
	} else {
		auto_on = true;
	}
#else
	auto_on = false;
#endif /* ENABLE_TDLS_AUTO_MODE */
	ret = _dhd_tdls_enable(dhd, false, auto_on, NULL);
	if (ret < 0) {
		DHD_ERROR(("Disable tdls_auto_op failed. %d\n", ret));
		return ret;
	}


	bcm_mkiovar("tdls_wfd_mode", (char *)&mode, sizeof(mode),
			iovbuf, sizeof(iovbuf));
	if (((ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf,
			sizeof(iovbuf), TRUE, 0)) < 0) &&
			(ret != BCME_UNSUPPORTED)) {
		DHD_ERROR(("%s: tdls_wfd_mode faile_wfd_mode %d\n", __FUNCTION__, ret));
		return ret;
	}

	ret = _dhd_tdls_enable(dhd, true, auto_on, NULL);
	if (ret < 0) {
		DHD_ERROR(("enable tdls_auto_op failed. %d\n", ret));
		return ret;
	}

	dhd->tdls_mode = mode;
	return ret;
}
#endif
#ifdef PCIE_FULL_DONGLE
void dhd_tdls_update_peer_info(struct net_device *dev, bool connect, uint8 *da)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	dhd_pub_t *dhdp =  (dhd_pub_t *)&dhd->pub;
	tdls_peer_node_t *cur = dhdp->peer_tbl.node;
	tdls_peer_node_t *new = NULL, *prev = NULL;
	dhd_if_t *dhdif;
	uint8 sa[ETHER_ADDR_LEN];
	int ifidx = dhd_net2idx(dhd, dev);

	if (ifidx == DHD_BAD_IF)
		return;

	dhdif = dhd->iflist[ifidx];
	memcpy(sa, dhdif->mac_addr, ETHER_ADDR_LEN);

	if (connect) {
		while (cur != NULL) {
			if (!memcmp(da, cur->addr, ETHER_ADDR_LEN)) {
				DHD_ERROR(("%s: TDLS Peer exist already %d\n",
					__FUNCTION__, __LINE__));
				return;
			}
			cur = cur->next;
		}

		new = MALLOC(dhdp->osh, sizeof(tdls_peer_node_t));
		if (new == NULL) {
			DHD_ERROR(("%s: Failed to allocate memory\n", __FUNCTION__));
			return;
		}
		memcpy(new->addr, da, ETHER_ADDR_LEN);
		new->next = dhdp->peer_tbl.node;
		dhdp->peer_tbl.node = new;
		dhdp->peer_tbl.tdls_peer_count++;

	} else {
		while (cur != NULL) {
			if (!memcmp(da, cur->addr, ETHER_ADDR_LEN)) {
				dhd_flow_rings_delete_for_peer(dhdp, ifidx, da);
				if (prev)
					prev->next = cur->next;
				else
					dhdp->peer_tbl.node = cur->next;
				MFREE(dhdp->osh, cur, sizeof(tdls_peer_node_t));
				dhdp->peer_tbl.tdls_peer_count--;
				return;
			}
			prev = cur;
			cur = cur->next;
		}
		DHD_ERROR(("%s: TDLS Peer Entry Not found\n", __FUNCTION__));
	}
}
#endif /* PCIE_FULL_DONGLE */
#endif

bool dhd_is_concurrent_mode(dhd_pub_t *dhd)
{
	if (!dhd)
		return FALSE;

	if (dhd->op_mode & DHD_FLAG_CONCURR_MULTI_CHAN_MODE)
		return TRUE;
	else if ((dhd->op_mode & DHD_FLAG_CONCURR_SINGLE_CHAN_MODE) ==
		DHD_FLAG_CONCURR_SINGLE_CHAN_MODE)
		return TRUE;
	else
		return FALSE;
}
#if !defined(AP) && defined(WLP2P)
/* From Android JerryBean release, the concurrent mode is enabled by default and the firmware
 * name would be fw_bcmdhd.bin. So we need to determine whether P2P is enabled in the STA
 * firmware and accordingly enable concurrent mode (Apply P2P settings). SoftAP firmware
 * would still be named as fw_bcmdhd_apsta.
 */
uint32
dhd_get_concurrent_capabilites(dhd_pub_t *dhd)
{
	int32 ret = 0;
	char buf[WLC_IOCTL_SMLEN];
	bool mchan_supported = FALSE;
	/* if dhd->op_mode is already set for HOSTAP and Manufacturing
	 * test mode, that means we only will use the mode as it is
	 */
	if (dhd->op_mode & (DHD_FLAG_HOSTAP_MODE | DHD_FLAG_MFG_MODE))
		return 0;
	if (FW_SUPPORTED(dhd, vsdb)) {
		mchan_supported = TRUE;
	}
	if (!FW_SUPPORTED(dhd, p2p)) {
		DHD_TRACE(("Chip does not support p2p\n"));
		return 0;
	}
	else {
		/* Chip supports p2p but ensure that p2p is really implemented in firmware or not */
		memset(buf, 0, sizeof(buf));
		ret = dhd_iovar(dhd, 0, "p2p", NULL, 0, (char *)&buf,
				sizeof(buf), FALSE);
		if (ret < 0) {
			DHD_ERROR(("%s: Get P2P failed (error=%d)\n", __FUNCTION__, ret));
			return 0;
		} else {
			if (buf[0] == 1) {
				/* By default, chip supports single chan concurrency,
				* now lets check for mchan
				*/
				ret = DHD_FLAG_CONCURR_SINGLE_CHAN_MODE;
				if (mchan_supported)
					ret |= DHD_FLAG_CONCURR_MULTI_CHAN_MODE;
#ifdef  BRCM_RSDB
				if (FW_SUPPORTED(dhd, rsdb)) {
					ret |= DHD_FLAG_RSDB_MODE;
				}
				if (FW_SUPPORTED(dhd, mp2p)) {
					ret |= DHD_FLAG_MP2P_MODE;
				}
#endif
#if defined(WL_ENABLE_P2P_IF) || defined(WL_CFG80211_P2P_DEV_IF)
				/* For customer_hw4, although ICS,
				* we still support concurrent mode
				*/
				return ret;
#else
				return 0;
#endif
			}
		}
	}
	return 0;
}
#endif

#ifdef CONFIG_HW_ABS
static int dhd_ant_init(dhd_pub_t *dhd)
{
	int err = BCME_OK;
	wl_ant_qual_event_t ant_event_param;

	DHD_ERROR(("%s enter \n", __FUNCTION__));

	memset(&ant_event_param, 0, sizeof(wl_ant_qual_event_t));
	/*
	 * Set the threshold for the ant event when STA is in MIMO mode
	 * When the rssi of core1 is lower than 'thresh_high_core1', the delta of
	 * rssi with core0 is higher than 'delta_high' and it persist 'hyst_time_low'
	 * seconds in this situation, it will report ant_event. And we switch to SISO
	 * mode
	 */
	ant_event_param.thresh_low_core1 = htod32(-78);
	ant_event_param.delta_low = htod32(20);
	ant_event_param.hyst_time_low = htod32(2);

	/*
	 * Set the threshold for the ant event when STA is in SISO mode
	 * When the rssi of core1 is higher than 'thresh_high_core1', the delta of
	 * rssi with core0 is lower than 'delta_high' and it persist 'hyst_time_high'
	 * seconds in this situation, it will report ant_event.
	 * Turn off this ant event by set 'hyst_time_high' to zero because we don't
	 * switch to MIMO accroding to it temporarily.
	 */
	ant_event_param.thresh_high_core1 = htod32(-82);
	ant_event_param.delta_high = htod32(10);
	ant_event_param.hyst_time_high = 0;

	err = dhd_iovar(dhd, 0, "ant_event", (char *)&ant_event_param, sizeof(wl_ant_qual_event_t), NULL, 0, TRUE);
	if (err == BCME_UNSUPPORTED) {
		DHD_ERROR(("Current firmware doesn't support antenna event\n"));
	}
	if (err != BCME_OK)
		DHD_ERROR(("dhd_ant_init failed, err: %d", err));
	return err;
}
#endif

#ifdef HW_WIFI_FIRST_SCAN_OPTIMIZE
bool g_dhd_init_flag = FALSE;
#endif
#ifdef CONFIG_HW_ABS
extern bool g_abs_enabled;
#endif
int
dhd_preinit_ioctls(dhd_pub_t *dhd)
{
	int ret = 0;
	char eventmask[WL_EVENTING_MASK_LEN];
	char iovbuf[WL_EVENTING_MASK_LEN + 12];	/*  Room for "event_msgs" + '\0' + bitvec  */
	uint32 buf_key_b4_m4 = 1;
	uint8 msglen;
	eventmsgs_ext_t *eventmask_msg;
	char iov_buf[WLC_IOCTL_SMLEN];
	int ret2 = 0;
#if defined(CUSTOM_AMPDU_BA_WSIZE)
	uint32 ampdu_ba_wsize = 0;
#endif
#if defined(CUSTOM_AMPDU_MPDU)
	int32 ampdu_mpdu = 0;
#endif
#if defined(CUSTOM_AMPDU_RELEASE)
	int32 ampdu_release = 0;
#endif
#ifdef HW_PATCH_DISABLE_VOICE_AMPDU
	struct ampdu_tid_control atc;
#endif

#if defined(BCMSDIO)
#ifdef PROP_TXSTATUS
	int wlfc_enable = TRUE;
#ifndef DISABLE_11N
	uint32 hostreorder = 1;
#endif /* DISABLE_11N */
#endif /* PROP_TXSTATUS */
#endif
#ifdef PCIE_FULL_DONGLE
	uint32 wl_ap_isolate;
#ifdef HW_DISABLE_HOSTREORDER
	uint32 hostreorder = 0;
#endif
#endif /* PCIE_FULL_DONGLE */

#ifdef DHD_ENABLE_LPC
	uint32 lpc = 1;
#endif /* DHD_ENABLE_LPC */
	uint power_mode = PM_FAST;
	uint32 dongle_align = DHD_SDALIGN;
#if defined(BCMSDIO)
	uint32 glom = CUSTOM_GLOM_SETTING;
#endif /* defined(BCMSDIO) */
#if defined(CUSTOMER_HW2) && defined(USE_WL_CREDALL)
	uint32 credall = 1;
#endif
	uint bcn_timeout = CUSTOM_BCN_TIMEOUT_SETTING;
	uint retry_max = 3;
#if defined(ARP_OFFLOAD_SUPPORT)
	int arpoe = 1;
#endif
	char buf[WLC_IOCTL_SMLEN];
	char *ptr;
	uint32 listen_interval = CUSTOM_LISTEN_INTERVAL; /* Default Listen Interval in Beacons */
#ifdef ROAM_ENABLE
	uint roamvar = 0;
	int roam_trigger[2] = {CUSTOM_ROAM_TRIGGER_SETTING, WLC_BAND_ALL};
	int roam_scan_period[2] = {10, WLC_BAND_ALL};
	int roam_delta[2] = {CUSTOM_ROAM_DELTA_SETTING, WLC_BAND_ALL};
#ifdef ROAM_AP_ENV_DETECTION
	int roam_env_mode = AP_ENV_INDETERMINATE;
#endif /* ROAM_AP_ENV_DETECTION */
#ifdef FULL_ROAMING_SCAN_PERIOD_60_SEC
	int roam_fullscan_period = 60;
#else /* FULL_ROAMING_SCAN_PERIOD_60_SEC */
	int roam_fullscan_period = 120;
#endif /* FULL_ROAMING_SCAN_PERIOD_60_SEC */
#else
#ifdef DISABLE_BUILTIN_ROAM
	uint roamvar = 1;
#endif /* DISABLE_BUILTIN_ROAM */
#endif /* ROAM_ENABLE */

#if defined(HW_SUPPORT_SHORT_GI)
	/*
	sgi_tx:
	-1: auto
	0: off
	1: on

	sgi_rx:
	0:off
	1: on when ht20
	2: on when ht40
	3: on both ht20 and ht40
	*/
	int sgi_tx = -1;
	int sgi_rx = 7;
#endif
#if defined(SOFTAP)
	uint dtim = 1;
#endif
#if (defined(AP) && !defined(WLP2P)) || (!defined(AP) && defined(WL_CFG80211))
	uint32 mpc = 0; /* Turn MPC off for AP/APSTA mode */
	struct ether_addr p2p_ea;
#endif

#if (defined(AP) || defined(WLP2P)) && !defined(SOFTAP_AND_GC)
	uint32 apsta = 1; /* Enable APSTA mode */
#elif defined(SOFTAP_AND_GC)
	uint32 apsta = 0;
	int ap_mode = 1;
#endif /* (defined(AP) || defined(WLP2P)) && !defined(SOFTAP_AND_GC) */
#ifdef GET_CUSTOM_MAC_ENABLE
	struct ether_addr ea_addr;
#endif /* GET_CUSTOM_MAC_ENABLE */

#ifdef DISABLE_11N
	uint32 nmode = 0;
#endif /* DISABLE_11N */

#if defined(DISABLE_11AC)
	uint32 vhtmode = 0;
#endif /* DISABLE_11AC */
#ifdef USE_WL_TXBF
	uint32 txbf = 1;
#endif /* USE_WL_TXBF */
#ifdef USE_WL_FRAMEBURST
	uint32 frameburst = 1;
#endif /* USE_WL_FRAMEBURST */
#ifdef SUPPORT_2G_VHT
	uint32 vht_features = 0x3; /* 2G enable | rates all */
#endif /* SUPPORT_2G_VHT */
#ifdef CUSTOM_PSPRETEND_THR
	uint32 pspretend_thr = CUSTOM_PSPRETEND_THR;
#endif
#ifdef MAX_AP_CLIENT_CNT
	uint32 max_assoc = MAX_AP_CLIENT_CNT;
#endif

#ifdef PKT_FILTER_SUPPORT
	dhd_pkt_filter_enable = TRUE;
#ifdef BCM_APF
	dhd->apf_set = FALSE;
#endif /* BCM_APF */
#endif /* PKT_FILTER_SUPPORT */
#ifdef WLTDLS
	dhd->tdls_enable = FALSE;
#ifdef  BRCM_RSDB
	dhd_tdls_set_mode(dhd, false);
#endif
#endif /* WLTDLS */
#ifdef DONGLE_ENABLE_ISOLATION
	dhd->dongle_isolation = TRUE;
#endif /* DONGLE_ENABLE_ISOLATION */
	dhd->suspend_bcn_li_dtim = CUSTOM_SUSPEND_BCN_LI_DTIM;
	DHD_TRACE(("Enter %s\n", __FUNCTION__));
	dhd->op_mode = 0;
	/* clear AP flags */
	dhd->dhd_cflags &= ~WLAN_PLAT_AP_FLAG;
	if ((!op_mode && dhd_get_fw_mode(dhd->info) == DHD_FLAG_MFG_MODE) ||
		(op_mode == DHD_FLAG_MFG_MODE)) {
		/* Check and adjust IOCTL response timeout for Manufactring firmware */
		dhd_os_set_ioctl_resp_timeout(MFG_IOCTL_RESP_TIMEOUT);
		DHD_ERROR(("%s : Set IOCTL response time for Manufactring Firmware\n",
			__FUNCTION__));
	}
	else {
		dhd_os_set_ioctl_resp_timeout(IOCTL_RESP_TIMEOUT);
		DHD_INFO(("%s : Set IOCTL response time.\n", __FUNCTION__));
	}
#ifdef  BRCM_RSDB
	/* Set RSDB mode */
	{
	    wl_config_t cfg;

	    if(dhd->rsdb_mode == FALSE) {
	        cfg.config = dhd->rsdb_mode;
	        cfg.config = htod32(cfg.config);
			bcm_mkiovar("rsdb_mode", (void *)&cfg, sizeof(wl_config_t), buf, sizeof(buf));
			ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, buf, sizeof(buf), TRUE, 0);
			if (ret < 0) {
				DHD_ERROR(("%s: can't set RSDB mode , error=%d\n", __FUNCTION__, ret));
			} else
				DHD_ERROR(("%s: set RSDB mode => %d\n", __FUNCTION__, dhd->rsdb_mode));

	    }
	}
#endif
#ifdef GET_CUSTOM_MAC_ENABLE
	ret = wifi_platform_get_mac_addr(dhd->info->adapter, ea_addr.octet);
	if (!ret) {
		ret = dhd_iovar(dhd, 0, "cur_etheraddr", (char *)&ea_addr,
				ETHER_ADDR_LEN, NULL, 0, TRUE);
		if (ret < 0) {
			DHD_ERROR(("%s: can't set MAC address , error=%d\n", __FUNCTION__, ret));
			return BCME_NOTUP;
		}
		memcpy(dhd->mac.octet, ea_addr.octet, ETHER_ADDR_LEN);
	} else {
#endif /* GET_CUSTOM_MAC_ENABLE */
		/* Get the default device MAC address directly from firmware */
		memset(buf, 0, sizeof(buf));
		ret = dhd_iovar(dhd, 0, "cur_etheraddr", NULL, 0, (char *)&buf,
				sizeof(buf), FALSE);
		if (ret < 0) {
			DHD_ERROR(("%s: can't get MAC address , error=%d\n", __FUNCTION__, ret));
			return BCME_NOTUP;
		}
		/* Update public MAC address after reading from Firmware */
		memcpy(dhd->mac.octet, buf, ETHER_ADDR_LEN);

#ifdef GET_CUSTOM_MAC_ENABLE
	}
#endif /* GET_CUSTOM_MAC_ENABLE */
	/* get a capabilities from firmware */
	ret = dhd_iovar(dhd, 0, "cap", NULL, 0, (char *)&dhd->fw_capabilities,
			sizeof(dhd->fw_capabilities), FALSE);
	if (ret < 0) {
		DHD_ERROR(("%s: Get Capability failed (error=%d)\n",
			__FUNCTION__, ret));
		return 0;
	}
	if ((!op_mode && dhd_get_fw_mode(dhd->info) == DHD_FLAG_HOSTAP_MODE) ||
		(op_mode == DHD_FLAG_HOSTAP_MODE)) {
#ifdef SET_RANDOM_MAC_SOFTAP
		uint rand_mac;
#endif
		dhd->op_mode = DHD_FLAG_HOSTAP_MODE;
#if defined(ARP_OFFLOAD_SUPPORT)
			arpoe = 0;
#endif
#ifdef PKT_FILTER_SUPPORT
			dhd_pkt_filter_enable = FALSE;
#endif
#ifdef SET_RANDOM_MAC_SOFTAP
		SRANDOM32((uint)jiffies);
		rand_mac = RANDOM32();
		iovbuf[0] = (unsigned char)(vendor_oui >> 16) | 0x02;	/* locally administered bit */
		iovbuf[1] = (unsigned char)(vendor_oui >> 8);
		iovbuf[2] = (unsigned char)vendor_oui;
		iovbuf[3] = (unsigned char)(rand_mac & 0x0F) | 0xF0;
		iovbuf[4] = (unsigned char)(rand_mac >> 8);
		iovbuf[5] = (unsigned char)(rand_mac >> 16);

		ret = dhd_iovar(dhd, 0, "cur_etheraddr", (char *)&iovbuf,
				ETHER_ADDR_LEN, NULL, 0, TRUE);
		if (ret < 0) {
			DHD_ERROR(("%s: can't set MAC address , error=%d\n", __FUNCTION__, ret));
		} else
			memcpy(dhd->mac.octet, iovbuf, ETHER_ADDR_LEN);
#endif /* SET_RANDOM_MAC_SOFTAP */
#if !defined(AP) && defined(WL_CFG80211)
		/* Turn off MPC in AP mode */
		ret = dhd_iovar(dhd, 0, "mpc", (char *)&mpc, sizeof(mpc), NULL,
				0, TRUE);
		if (ret < 0)
			DHD_ERROR(("%s mpc for HostAPD failed  %d\n",
				   __FUNCTION__, ret));
#endif
#ifdef MAX_AP_CLIENT_CNT
		ret = dhd_iovar(dhd, 0, "maxassoc", (char *)&max_assoc,
				sizeof(max_assoc), NULL, 0, TRUE);
		if (ret < 0)
			DHD_ERROR(("%s maxassoc for HostAPD failed  %d\n",
				   __FUNCTION__, ret));
#endif
		/* set AP flag for specific country code of SOFTAP */
		dhd->dhd_cflags |= WLAN_PLAT_AP_FLAG;
	} else if ((!op_mode && dhd_get_fw_mode(dhd->info) == DHD_FLAG_MFG_MODE) ||
		(op_mode == DHD_FLAG_MFG_MODE)) {
#if defined(ARP_OFFLOAD_SUPPORT)
		arpoe = 0;
#endif /* ARP_OFFLOAD_SUPPORT */
#ifdef PKT_FILTER_SUPPORT
		dhd_pkt_filter_enable = FALSE;
#endif /* PKT_FILTER_SUPPORT */
		dhd->op_mode = DHD_FLAG_MFG_MODE;
	} else {
		uint32 concurrent_mode = 0;
		if ((!op_mode && dhd_get_fw_mode(dhd->info) == DHD_FLAG_P2P_MODE) ||
			(op_mode == DHD_FLAG_P2P_MODE)) {
#if defined(ARP_OFFLOAD_SUPPORT)
			arpoe = 0;
#endif
#ifdef PKT_FILTER_SUPPORT
			dhd_pkt_filter_enable = FALSE;
#endif
			dhd->op_mode = DHD_FLAG_P2P_MODE;
		} else if ((!op_mode && dhd_get_fw_mode(dhd->info) == DHD_FLAG_IBSS_MODE) ||
			(op_mode == DHD_FLAG_IBSS_MODE)) {
			dhd->op_mode = DHD_FLAG_IBSS_MODE;
		} else
			dhd->op_mode = DHD_FLAG_STA_MODE;
#if !defined(AP) && defined(WLP2P)
		if (dhd->op_mode != DHD_FLAG_IBSS_MODE &&
			(concurrent_mode = dhd_get_concurrent_capabilites(dhd))) {
#if defined(ARP_OFFLOAD_SUPPORT)
			arpoe = 1;
#endif
			dhd->op_mode |= concurrent_mode;
		}

		/* Check if we are enabling p2p */
		if (dhd->op_mode & DHD_FLAG_P2P_MODE) {
			ret = dhd_iovar(dhd, 0, "apsta", (char *)&apsta,
					sizeof(apsta), NULL, 0, TRUE);
			if (ret < 0)
				DHD_ERROR(("%s APSTA for P2P failed ret= %d\n", __FUNCTION__, ret));

#if defined(SOFTAP_AND_GC)
		if ((ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_AP,
			(char *)&ap_mode, sizeof(ap_mode), TRUE, 0)) < 0) {
				DHD_ERROR(("%s WLC_SET_AP failed %d\n", __FUNCTION__, ret));
		}
#endif
			memcpy(&p2p_ea, &dhd->mac, ETHER_ADDR_LEN);
			ETHER_SET_LOCALADDR(&p2p_ea);
			ret = dhd_iovar(dhd, 0, "p2p_da_override",
					(char *)&p2p_ea, sizeof(p2p_ea), NULL,
					0, TRUE);
			if (ret < 0)
				DHD_ERROR(("%s p2p_da_override ret= %d\n", __FUNCTION__, ret));
			else
				DHD_INFO(("dhd_preinit_ioctls: p2p_da_override succeeded\n"));
		}
#else
	(void)concurrent_mode;
#endif
	}

	DHD_ERROR(("Firmware up: op_mode=0x%04x, MAC="MACDBG"\n",
		dhd->op_mode, MAC2STRDBG(dhd->mac.octet)));
	/* get a ccode and revision for the country code */
	get_customized_country_code(dhd->info->adapter, dhd->dhd_cspec.country_abbrev,
			&dhd->dhd_cspec, dhd->dhd_cflags);
	/* Set Country code  */
	if (dhd->dhd_cspec.ccode[0] != 0) {
		ret = dhd_iovar(dhd, 0, "country", (char *)&dhd->dhd_cspec,
				sizeof(dhd->dhd_cspec), NULL, 0, TRUE);
		if (ret < 0)
			DHD_ERROR(("%s: country code setting failed\n", __FUNCTION__));
	}

#if defined(DISABLE_11AC)
	bcm_mkiovar("vhtmode", (char *)&vhtmode, 4, iovbuf, sizeof(iovbuf));
	if ((ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0)) < 0)
		DHD_ERROR(("%s wl vhtmode 0 failed %d\n", __FUNCTION__, ret));
#endif /* DISABLE_11AC */

	/* Set Listen Interval */
	ret = dhd_iovar(dhd, 0, "assoc_listen", (char *)&listen_interval,
			sizeof(listen_interval), NULL, 0, TRUE);
	if (ret < 0)
		DHD_ERROR(("%s assoc_listen failed %d\n", __FUNCTION__, ret));

#if defined(ROAM_ENABLE) || defined(DISABLE_BUILTIN_ROAM)
	/* Disable built-in roaming to allowed ext supplicant to take care of roaming */
	dhd_iovar(dhd, 0, "roam_off", (char *)&roamvar, sizeof(roamvar),
		  NULL, 0, TRUE);
#endif /* ROAM_ENABLE || DISABLE_BUILTIN_ROAM */
#if defined(ROAM_ENABLE)
	if ((ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_ROAM_TRIGGER, roam_trigger,
		sizeof(roam_trigger), TRUE, 0)) < 0)
		DHD_ERROR(("%s: roam trigger set failed %d\n", __FUNCTION__, ret));
	if ((ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_ROAM_SCAN_PERIOD, roam_scan_period,
		sizeof(roam_scan_period), TRUE, 0)) < 0)
		DHD_ERROR(("%s: roam scan period set failed %d\n", __FUNCTION__, ret));
	if ((dhd_wl_ioctl_cmd(dhd, WLC_SET_ROAM_DELTA, roam_delta,
		sizeof(roam_delta), TRUE, 0)) < 0)
		DHD_ERROR(("%s: roam delta set failed %d\n", __FUNCTION__, ret));
	ret = dhd_iovar(dhd, 0, "fullroamperiod", (char *)&roam_fullscan_period,
			sizeof(roam_fullscan_period), NULL, 0, TRUE);
	if (ret < 0)
		DHD_ERROR(("%s: roam fullscan period set failed %d\n", __FUNCTION__, ret));
#ifdef ROAM_AP_ENV_DETECTION
	if (roam_trigger[0] == WL_AUTO_ROAM_TRIGGER) {
		if (dhd_iovar(dhd, 0, "roam_env_detection",
			      (char *)&roam_env_mode, sizeof(roam_env_mode),
			      NULL, 0, TRUE) == BCME_OK)
			dhd->roam_env_detection = TRUE;
		else
			dhd->roam_env_detection = FALSE;
	}
#endif /* ROAM_AP_ENV_DETECTION */
#endif /* ROAM_ENABLE */

#ifdef WLTDLS
	/* by default TDLS on and auto mode off */
	_dhd_tdls_enable(dhd, true, false, NULL);
#endif /* WLTDLS */

#ifdef DHD_ENABLE_LPC
	/* Set lpc 1 */
	ret = dhd_iovar(dhd, 0, "lpc", (char *)&lpc, sizeof(lpc), NULL, 0,
			TRUE);
	if (ret < 0) {
		if (ret != BCME_NOTDOWN) {
			DHD_ERROR(("%s Set lpc fail  %d\n", __FUNCTION__, ret));
		} else {
			u32 wl_down = 1;

			ret = dhd_wl_ioctl_cmd(dhd, WLC_DOWN, (char *)&wl_down,
					       sizeof(wl_down), TRUE, 0);
			DHD_ERROR(("%s lpc fail WL_DOWN : %d, lpc = %d\n",
				   __FUNCTION__, ret, lpc));

			ret = dhd_iovar(dhd, 0, "lpc", (char *)&lpc,
					sizeof(lpc), NULL, 0, TRUE);
			if (ret < 0)
				DHD_ERROR(("%s Set lpc ret --> %d\n",
					   __FUNCTION__, ret));
		}
	}
#endif /* DHD_ENABLE_LPC */

	/* Set PowerSave mode */
	dhd_wl_ioctl_cmd(dhd, WLC_SET_PM, (char *)&power_mode, sizeof(power_mode), TRUE, 0);

#ifdef HW_LP_OVERSEA
	hw_set_pmlock(dhd);
#endif

	/* Match Host and Dongle rx alignment */
#if defined(BCMSDIO)
	dhd_iovar(dhd, 0, "bus:txglomalign", (char *)&dongle_align,
		  sizeof(dongle_align), NULL, 0, TRUE);
#endif
#if defined(CUSTOMER_HW2) && defined(USE_WL_CREDALL)
	/* enable credall to reduce the chance of no bus credit happened. */
	dhd_iovar(dhd, 0, "bus:credall", (char *)&credall,
		  sizeof(credall), NULL, 0, TRUE);
#endif

#if defined(BCMSDIO)
	if (glom != DEFAULT_GLOM_VALUE) {
		DHD_INFO(("%s set glom=0x%X\n", __FUNCTION__, glom));
		dhd_iovar(dhd, 0, "bus:txglom", (char *)&glom,
			  sizeof(glom), NULL, 0, TRUE);
	}
#endif /* defined(BCMSDIO) */

	/* Setup timeout if Beacons are lost and roam is off to report link down */
#ifdef HW_CUSTOM_BCN_TIMEOUT
	bcn_timeout = (uint) wifi_platform_get_bcn_timeout(dhd->info->adapter, (int) bcn_timeout);
	DHD_TRACE(("%s: bcn_timeout: %d \n", __FUNCTION__, bcn_timeout));
#endif /* HW_CUSTOM_BCN_TIMEOUT */
	dhd_iovar(dhd, 0, "bcn_timeout", (char *)&bcn_timeout,
		  sizeof(bcn_timeout), NULL, 0, TRUE);
	/* Setup assoc_retry_max count to reconnect target AP in dongle */
	dhd_iovar(dhd, 0, "assoc_retry_max", (char *)&retry_max,
		  sizeof(retry_max), NULL, 0, TRUE);

#if defined(AP) && !defined(WLP2P)
	/* Turn off MPC in AP mode */
	dhd_iovar(dhd, 0, "mpc", (char *)&mpc, sizeof(mpc), NULL, 0, TRUE);
	dhd_iovar(dhd, 0, "apsta", (char *)&apsta, sizeof(apsta), NULL,
		  0, TRUE);

#endif /* defined(AP) && !defined(WLP2P) */


#if defined(SOFTAP)
	if (ap_fw_loaded == TRUE) {
		dhd_wl_ioctl_cmd(dhd, WLC_SET_DTIMPRD, (char *)&dtim, sizeof(dtim), TRUE, 0);
	}
#endif

#if defined(KEEP_ALIVE)
	{
	/* Set Keep Alive : be sure to use FW with -keepalive */
	int res;

#if defined(SOFTAP)
	if (ap_fw_loaded == FALSE)
#endif
		if (!(dhd->op_mode &
			(DHD_FLAG_HOSTAP_MODE | DHD_FLAG_MFG_MODE))) {
			if ((res = dhd_keep_alive_onoff(dhd)) < 0)
				DHD_ERROR(("%s set keeplive failed %d\n",
				__FUNCTION__, res));
		}
	}
#endif /* defined(KEEP_ALIVE) */

#ifdef USE_WL_TXBF
	ret = dhd_iovar(dhd, 0, "txbf", (char *)&txbf, sizeof(txbf), NULL, 0,
			TRUE);
	if (ret < 0)
		DHD_ERROR(("%s Set txbf failed  %d\n", __FUNCTION__, ret));

#endif /* USE_WL_TXBF */
#ifdef USE_WL_FRAMEBURST
	/* Set frameburst to value */
	if ((ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_FAKEFRAG, (char *)&frameburst,
		sizeof(frameburst), TRUE, 0)) < 0) {
		DHD_ERROR(("%s Set frameburst failed  %d\n", __FUNCTION__, ret));
	}
#endif /* USE_WL_FRAMEBURST */
#if defined(CUSTOM_AMPDU_BA_WSIZE)
	/* Set ampdu ba wsize to 64 or 16 */
#ifdef CUSTOM_AMPDU_BA_WSIZE
	ampdu_ba_wsize = CUSTOM_AMPDU_BA_WSIZE;
#endif
	if (ampdu_ba_wsize != 0) {
		ret = dhd_iovar(dhd, 0, "ampdu_ba_wsize",
				(char *)&ampdu_ba_wsize, sizeof(ampdu_ba_wsize),
				NULL, 0, TRUE);
		if (ret < 0)
			DHD_ERROR(("%s Set ampdu_ba_wsize to %d failed  %d\n",
				__FUNCTION__, ampdu_ba_wsize, ret));
	}
#endif


#if defined(CUSTOM_AMPDU_MPDU)
	ampdu_mpdu = CUSTOM_AMPDU_MPDU;
	if (ampdu_mpdu != 0 && (ampdu_mpdu <= ampdu_ba_wsize)) {
		ret = dhd_iovar(dhd, 0, "ampdu_mpdu", (char *)&ampdu_mpdu,
				sizeof(ampdu_mpdu), NULL, 0, TRUE);
		if (ret < 0)
			DHD_ERROR(("%s Set ampdu_mpdu to %d failed  %d\n",
				__FUNCTION__, CUSTOM_AMPDU_MPDU, ret));
	}
#endif /* CUSTOM_AMPDU_MPDU */

#if defined(CUSTOM_AMPDU_RELEASE)
	ampdu_release = CUSTOM_AMPDU_RELEASE;
	if (ampdu_release != 0 && (ampdu_release <= ampdu_ba_wsize)) {
		ret = dhd_iovar(dhd, 0, "ampdu_release", (char *)&ampdu_release,
				sizeof(ampdu_release), NULL, 0, TRUE);
		if (ret < 0)
			DHD_ERROR(("%s Set ampdu_release to %d failed  %d\n",
				__FUNCTION__, CUSTOM_AMPDU_RELEASE, ret));
	}
#endif /* CUSTOM_AMPDU_RELEASE */

#ifdef HW_PATCH_DISABLE_VOICE_AMPDU
	/* not enable AMPDU for Qos-voice frame like iphone, because
		1, voice frame care more about real-time than througput
		2, some router like DIR625 has compatible issue with video&voice ADDBA request
	*/
	DHD_ERROR(("%s set ampdu_tid.\n", __FUNCTION__));
	atc.enable = 0;
	atc.tid = 6;
	bcm_mkiovar("ampdu_tid", (void *)&atc, sizeof(atc), buf, sizeof(buf));
	ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, buf, sizeof(buf), TRUE, 0);
	if (ret < 0) {
		DHD_ERROR(("%s set ampdu_tid 6 failed %d\n", __FUNCTION__, ret));
	}
	atc.tid = 7;
	bcm_mkiovar("ampdu_tid", (void *)&atc, sizeof(atc), buf, sizeof(buf));
	ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, buf, sizeof(buf), TRUE, 0);
	if (ret < 0) {
		DHD_ERROR(("%s set ampdu_tid 7 failed %d\n", __FUNCTION__, ret));
	}
#endif

#ifdef SUPPORT_2G_VHT
	u32 wl_down = 1;
	if ((ret = dhd_wl_ioctl_cmd(dhd, WLC_DOWN,(char *)&wl_down, sizeof(wl_down), TRUE, 0)) < 0) {
		DHD_ERROR(("%s WLC_DOWN set failed %d\n", __FUNCTION__, ret));
	}
	bcm_mkiovar("vht_features", (char *)&vht_features, 4, iovbuf, sizeof(iovbuf));
	if ((ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0)) < 0) {
		DHD_ERROR(("%s vht_features set failed %d\n", __FUNCTION__, ret));
	}
#endif /* SUPPORT_2G_VHT */

#if defined(HW_SUPPORT_SHORT_GI)
	bcm_mkiovar("sgi_tx", (char *)&sgi_tx, 4, iovbuf, sizeof(iovbuf));
	if ((ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR,
		iovbuf, sizeof(iovbuf), TRUE, 0)) < 0) {
		DHD_ERROR(("%s set sgi_tx failed ret= %d\n", __FUNCTION__, ret));
	}
	bcm_mkiovar("sgi_rx", (char *)&sgi_rx, 4, iovbuf, sizeof(iovbuf));
	if ((ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR,
		iovbuf, sizeof(iovbuf), TRUE, 0)) < 0) {
		DHD_ERROR(("%s set sgi_rx failed ret= %d\n", __FUNCTION__, ret));
	}
#endif
#ifdef CUSTOM_PSPRETEND_THR
	/* Turn off MPC in AP mode */
	ret = dhd_iovar(dhd, 0, "pspretend_threshold", (char *)&pspretend_thr,
			sizeof(pspretend_thr), NULL, 0, TRUE);
	if (ret < 0)
		DHD_ERROR(("%s pspretend_threshold for HostAPD failed  %d\n",
			__FUNCTION__, ret));
#endif
	ret = dhd_iovar(dhd, 0, "buf_key_b4_m4", (char *)&buf_key_b4_m4,
			sizeof(buf_key_b4_m4), NULL, 0, TRUE);
	if (ret < 0)
		DHD_ERROR(("%s buf_key_b4_m4 set failed %d\n", __FUNCTION__, ret));
	/* Read event_msgs mask */

	ret = dhd_iovar(dhd, 0, "event_msgs", NULL, 0, (char *)&iovbuf,
			sizeof(iovbuf), FALSE);
	if (ret < 0) {
		DHD_ERROR(("%s read Event mask failed %d\n", __FUNCTION__, ret));
		goto done;
	}
	bcopy(iovbuf, eventmask, WL_EVENTING_MASK_LEN);
	/* Setup event_msgs */
#ifdef CONFIG_HW_VOWIFI
	setbit(eventmask, WLC_E_RSSI);
#endif
	setbit(eventmask, WLC_E_SET_SSID);
	setbit(eventmask, WLC_E_PRUNE);
	setbit(eventmask, WLC_E_AUTH);
	setbit(eventmask, WLC_E_AUTH_IND);
	setbit(eventmask, WLC_E_ASSOC);
	setbit(eventmask, WLC_E_REASSOC);
	setbit(eventmask, WLC_E_REASSOC_IND);
	setbit(eventmask, WLC_E_DEAUTH);
	setbit(eventmask, WLC_E_DEAUTH_IND);
	setbit(eventmask, WLC_E_DISASSOC_IND);
	setbit(eventmask, WLC_E_DISASSOC);
	setbit(eventmask, WLC_E_JOIN);
	setbit(eventmask, WLC_E_START);
	setbit(eventmask, WLC_E_ASSOC_IND);
	setbit(eventmask, WLC_E_PSK_SUP);
	setbit(eventmask, WLC_E_LINK);
	setbit(eventmask, WLC_E_NDIS_LINK);
	setbit(eventmask, WLC_E_MIC_ERROR);
	setbit(eventmask, WLC_E_ASSOC_REQ_IE);
	setbit(eventmask, WLC_E_ASSOC_RESP_IE);
#ifndef WL_CFG80211
	setbit(eventmask, WLC_E_PMKID_CACHE);
	setbit(eventmask, WLC_E_TXFAIL);
#endif
	setbit(eventmask, WLC_E_JOIN_START);
	setbit(eventmask, WLC_E_SCAN_COMPLETE);
#ifdef WLMEDIA_HTSF
	setbit(eventmask, WLC_E_HTSFSYNC);
#endif /* WLMEDIA_HTSF */
#ifdef PNO_SUPPORT
	setbit(eventmask, WLC_E_PFN_NET_FOUND);
	setbit(eventmask, WLC_E_PFN_BEST_BATCHING);
	setbit(eventmask, WLC_E_PFN_BSSID_NET_FOUND);
	setbit(eventmask, WLC_E_PFN_BSSID_NET_LOST);
#endif /* PNO_SUPPORT */
	/* enable dongle roaming event */
	setbit(eventmask, WLC_E_ROAM);
	setbit(eventmask, WLC_E_BSSID);
#ifdef WLTDLS
	setbit(eventmask, WLC_E_TDLS_PEER_EVENT);
#endif /* WLTDLS */
#ifdef RTT_SUPPORT
	setbit(eventmask, WLC_E_PROXD);
#endif /* RTT_SUPPORT */
#ifdef WL_CFG80211
	setbit(eventmask, WLC_E_ESCAN_RESULT);
#ifdef  BRCM_RSDB
	setbit(eventmask, WLC_E_AP_STARTED);
#endif
	if (dhd->op_mode & DHD_FLAG_P2P_MODE) {
		setbit(eventmask, WLC_E_ACTION_FRAME_RX);
		setbit(eventmask, WLC_E_P2P_DISC_LISTEN_COMPLETE);
	}
#endif /* WL_CFG80211 */
#ifdef SHOW_LOGTRACE
	setbit(eventmask, WLC_E_TRACE);
#else
	clrbit(eventmask, WLC_E_TRACE);
#endif
#if defined(DHD_DEBUG) && defined(BCM_PATCH_DEBUG_BEACON_LOSS)
	setbit(eventmask, WLC_E_BEACON_RX);
#endif
#ifdef HW_DFX_TXFAIL_EVENT
	/* For ARP/DHCP tx frame status check */
	setbit(eventmask, WLC_E_TXFAIL_THRESH);
#endif
#ifdef EAPOL_PKT_PRIO
#ifdef CONFIG_BCMDHD_PCIE
	dhd_update_flow_prio_map(dhd, DHD_FLOW_PRIO_LLR_MAP);
#endif /* CONFIG_BCMDHD_PCIE */
#endif /* EAPOL_PKT_PRIO */
	/* Write updated Event mask */
	ret = dhd_iovar(dhd, 0, "event_msgs", eventmask, sizeof(eventmask),
			NULL, 0, TRUE);
	if (ret < 0) {
		DHD_ERROR(("%s Set Event mask failed %d\n", __FUNCTION__, ret));
		goto done;
	}
	/* make up event mask ext message iovar for event larger than 128 */
	msglen = ROUNDUP(WLC_E_LAST, NBBY)/NBBY + EVENTMSGS_EXT_STRUCT_SIZE;
	eventmask_msg = (eventmsgs_ext_t*)kmalloc(msglen, GFP_KERNEL);
	if (eventmask_msg == NULL) {
		DHD_ERROR(("failed to allocate %d bytes for event_msg_ext\n", msglen));
		return BCME_NOMEM;
	}
	bzero(eventmask_msg, msglen);
	eventmask_msg->ver = EVENTMSGS_VER;
	eventmask_msg->len = ROUNDUP(WLC_E_LAST, NBBY)/NBBY;
	/* Read event_msgs_ext mask */
	ret2 = dhd_iovar(dhd, 0, "event_msgs_ext", (char *)eventmask_msg,
			 msglen, (char *)&iov_buf, sizeof(iov_buf), FALSE);

	if (ret2 != BCME_UNSUPPORTED)
		ret = ret2;
	if (ret2 == 0) { /* event_msgs_ext must be supported */
		bcopy(iov_buf, eventmask_msg, msglen);
		setbit(eventmask_msg->mask, WLC_E_RSSI_LQM);
#ifdef GSCAN_SUPPORT
		setbit(eventmask_msg->mask, WLC_E_PFN_GSCAN_FULL_RESULT);
		setbit(eventmask_msg->mask, WLC_E_PFN_SCAN_COMPLETE);
#ifndef BCM_PATCH_SECURITY_2017_07
		setbit(eventmask_msg->mask, WLC_E_PFN_SWC);
#endif
		setbit(eventmask_msg->mask, WLC_E_PFN_SSID_EXT);
		setbit(eventmask_msg->mask, WLC_E_ROAM_EXP_EVENT);
#endif /* GSCAN_SUPPORT */
#ifdef BT_WIFI_HANDOVER
		setbit(eventmask_msg->mask, WLC_E_BT_WIFI_HANDOVER_REQ);
#endif /* BT_WIFI_HANDOVER */
#ifdef BRCM_RSDB
		setbit(eventmask_msg->mask, WLC_E_SDB_TRANSITION);
#endif
#ifdef CONFIG_HW_ABS
		setbit(eventmask_msg->mask, WLC_E_ANT_EVENT);
#endif
#ifdef CONFIG_HW_GET_EXT_SIG
		setbit(eventmask_msg->mask, WLC_E_RRM);
#endif
#ifdef WL_TEM_CTRL
		setbit(eventmask_msg->mask, WLC_E_TEM_CTRL_EVENT);
#endif
#if defined WL_TEM_CTRL || defined WL_TIM_EVENT
		wifi_init_proc();
#endif
#ifdef WL_TIM_EVENT
		//setbit(eventmask_msg->mask, WLC_E_TIM_EVENT);
#endif

		/* Write updated Event mask */
		eventmask_msg->ver = EVENTMSGS_VER;
		eventmask_msg->command = EVENTMSGS_SET_MASK;
		eventmask_msg->len = ROUNDUP(WLC_E_LAST, NBBY)/NBBY;
		ret = dhd_iovar(dhd, 0, "event_msgs_ext", (char *)eventmask_msg,
				msglen, NULL, 0, TRUE);
		if (ret < 0) {
			DHD_ERROR(("%s write event mask ext failed %d\n", __FUNCTION__, ret));
			kfree(eventmask_msg);
			goto done;
		}
	} else if (ret2 < 0 && ret2 != BCME_UNSUPPORTED) {
		DHD_ERROR(("%s read event mask ext failed %d\n", __FUNCTION__, ret2));
		kfree(eventmask_msg);
		goto done;
	} /* unsupported is ok */
	kfree(eventmask_msg);
#ifdef HW_SCAN_DEFAULT_TIME
	/* Init the scan time when turn on wifi */
	dhd->short_dwell_time = -1;
#endif
	dhd_set_short_dwell_time(dhd, FALSE);
#ifdef HW_WIFI_FIRST_SCAN_OPTIMIZE
	g_dhd_init_flag = FALSE;
#endif

#ifdef ARP_OFFLOAD_SUPPORT
	/* Set and enable ARP offload feature for STA only  */
#if defined(SOFTAP)
	if (arpoe && !ap_fw_loaded) {
#else
	if (arpoe) {
#endif
		dhd_arp_offload_enable(dhd, TRUE);
		dhd_arp_offload_set(dhd, dhd_arp_mode);
	} else {
		dhd_arp_offload_enable(dhd, FALSE);
		dhd_arp_offload_set(dhd, 0);
	}
	dhd_arp_enable = arpoe;
#endif /* ARP_OFFLOAD_SUPPORT */
#ifdef HW_DOZE_PKT_FILTER
	memset(dhd->pktfilter, 0, sizeof(dhd->pktfilter));
#endif
#ifdef PKT_FILTER_SUPPORT
	/* Setup default defintions for pktfilter , enable in suspend */
	dhd->pktfilter_count = 6;
	/* Setup filter to allow only unicast */
	dhd->pktfilter[DHD_UNICAST_FILTER_NUM] = "100 0 0 0 0x01 0x00";
	dhd->pktfilter[DHD_BROADCAST_FILTER_NUM] = NULL;
	dhd->pktfilter[DHD_MULTICAST4_FILTER_NUM] = NULL;
	dhd->pktfilter[DHD_MULTICAST6_FILTER_NUM] = NULL;
	dhd->pktfilter[DHD_MDNS_FILTER_NUM] = NULL;
	/* apply APP pktfilter */
	dhd->pktfilter[DHD_ARP_FILTER_NUM] = "105 0 0 12 0xFFFF 0x0806";


#if defined(SOFTAP)
	if (ap_fw_loaded) {
		dhd_enable_packet_filter(0, dhd);
	}
#endif /* defined(SOFTAP) */
	dhd_set_packet_filter(dhd);
#endif /* PKT_FILTER_SUPPORT */
#ifdef DISABLE_11N
	ret = dhd_iovar(dhd, 0, "nmode", (char *)&nmode, sizeof(nmode), NULL,
			0, TRUE);
	if (ret < 0)
		DHD_ERROR(("%s wl nmode 0 failed %d\n", __FUNCTION__, ret));
#ifdef WRONG_ACTION_PATCH
	else
	    dhd->nmode_disable = TRUE;
#endif
#endif /* DISABLE_11N */
	/* query for 'ver' to get version info from firmware */
	ret = dhd_iovar(dhd, 0, "ver", NULL, 0, (char *)&buf, sizeof(buf),
			FALSE);
	if (ret == BCME_OK) {
		ptr = buf;
		bcmstrtok(&ptr, "\n", 0);
		/* Print fw version info */
		DHD_ERROR(("Firmware version = %s\n", buf));

		dhd_set_version_info(dhd, buf);
	} else {
		DHD_ERROR(("%s failed %d\n", __FUNCTION__, ret));
	}
#if defined(BCMSDIO)
	dhd_txglom_enable(dhd, TRUE);
#endif /* defined(BCMSDIO) */

#if defined(BCMSDIO)
#ifdef PROP_TXSTATUS
	if (disable_proptx ||
#ifdef PROP_TXSTATUS_VSDB
		/* enable WLFC only if the firmware is VSDB when it is in STA mode */
#ifndef HW_SOFTAP_CANNOT_BE_CONNECTTED
		(dhd->op_mode != DHD_FLAG_HOSTAP_MODE &&
		 dhd->op_mode != DHD_FLAG_IBSS_MODE) ||
#else
		(dhd->op_mode != DHD_FLAG_IBSS_MODE) ||
#endif /* HW_SOFTAP_CANNOT_BE_CONNECTTED */
#endif /* PROP_TXSTATUS_VSDB */
		FALSE) {
		wlfc_enable = FALSE;
	}
#ifndef DISABLE_11N
	ret2 = dhd_iovar(dhd, 0, "ampdu_hostreorder", (char *)&hostreorder,
			 sizeof(hostreorder), NULL, 0, TRUE);
	if (ret2 < 0) {
		DHD_ERROR(("%s wl ampdu_hostreorder failed %d\n", __FUNCTION__, ret2));
		if (ret2 != BCME_UNSUPPORTED)
			ret = ret2;
		if (ret2 != BCME_OK)
			hostreorder = 0;
	}
#endif /* DISABLE_11N */

	if (wlfc_enable)
		dhd_wlfc_init(dhd);
#ifndef DISABLE_11N
	else if (hostreorder)
		dhd_wlfc_hostreorder_init(dhd);
#endif /* DISABLE_11N */
#endif /* PROP_TXSTATUS */
#endif /* BCMSDIO || BCMBUS */
#ifdef PCIE_FULL_DONGLE
#ifdef HW_DISABLE_HOSTREORDER
		bcm_mkiovar("ampdu_hostreorder", (char *)&hostreorder, 4, iovbuf, sizeof(iovbuf));
		if ((ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0)) < 0) {
			DHD_ERROR(("%s wl ampdu_hostreorder failed %d\n", __FUNCTION__, ret));
		}
#endif /* HW_DISABLE_HOSTREORDER */
	/* For FD we need all the packets at DHD to handle intra-BSS forwarding */
	if (FW_SUPPORTED(dhd, ap)) {
		wl_ap_isolate = AP_ISOLATE_SENDUP_ALL;
		ret = dhd_iovar(dhd, 0, "ap_isolate", (char *)&wl_ap_isolate,
				sizeof(wl_ap_isolate), NULL, 0, TRUE);
		if (ret < 0)
			DHD_ERROR(("%s failed %d\n", __FUNCTION__, ret));
	}
#endif /* PCIE_FULL_DONGLE */
#ifdef PNO_SUPPORT
	if (!dhd->pno_state) {
		dhd_pno_init(dhd);
	}
#endif
#ifdef CONFIG_HW_GET_EXT_SIG
{
#ifndef HW_RRM_DISABLE
	dot11_rrm_cap_ie_t rrm_cap, *reply;
	bcm_mkiovar("rrm", (char *)&(rrm_cap), sizeof(rrm_cap), iovbuf, sizeof(iovbuf));
	if ((ret2 = dhd_wl_ioctl_cmd(dhd, WLC_GET_VAR, iovbuf, sizeof(iovbuf), FALSE, 0)) < 0) {
		DHD_ERROR(("%s failed to get rrm cap %d\n",
			__FUNCTION__, ret2));
	} else {
		reply = (dot11_rrm_cap_ie_t *)iovbuf;
		DHD_ERROR(("%s: rrm cap = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
		__FUNCTION__, reply->cap[0], reply->cap[1], reply->cap[2], reply->cap[3],reply->cap[4]));
	}

	memcpy(&rrm_cap, reply, sizeof(rrm_cap));
	rrm_cap.cap[1] |= 0x2;
#else
	dot11_rrm_cap_ie_t rrm_cap;
	memset(&rrm_cap, 0, sizeof(rrm_cap));
#endif

	bcm_mkiovar("rrm", (char *)&(rrm_cap), sizeof(rrm_cap), iovbuf, sizeof(iovbuf));
	if ((dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), FALSE, 0)) < 0) {
		DHD_ERROR(("%s failed to set rrm\n", __FUNCTION__));
	}
}
#endif
#ifdef RTT_SUPPORT
	if (!dhd->rtt_state) {
		ret = dhd_rtt_init(dhd);
		if (ret < 0) {
			DHD_ERROR(("%s failed to initialize RTT\n", __FUNCTION__));
		}
	}
#endif
#ifdef HW_SOFTAP_THROUGHPUT_OPTIMIZE
{
	uint val = 0;
	val = 1;
	if(dhd_wl_ioctl_cmd(dhd, WLC_UP, (char *)&val, sizeof(val), TRUE, 0)< 0)
		DHD_ERROR(("%s: set WLC_UP error. \n", __FUNCTION__));
	val = 1;
	if(dhd_wl_ioctl_cmd(dhd, WLC_SET_CWMIN, (char *)&val, sizeof(val), TRUE, 0) <0)
		DHD_ERROR(("%s: set WLC_SET_CWMIN error. \n", __FUNCTION__));
	val = 255;
	if(dhd_wl_ioctl_cmd(dhd, WLC_SET_CWMAX, (char *)&val, sizeof(val), TRUE, 0)<0)
		DHD_ERROR(("%s: set WLC_SET_CWMAX error. \n", __FUNCTION__));
	val = 1;
	if(dhd_wl_ioctl_cmd(dhd, WLC_DOWN, (char *)&val, sizeof(val), TRUE, 0)<0)
		DHD_ERROR(("%s: set WLC_DOWN error. \n", __FUNCTION__));
	printf(" cwmin/max set.\n");
	HW_PRINT((WIFI_TAG"throughput optimize\n"));
}
#endif
#ifdef WL11U
	dhd_interworking_enable(dhd);
#endif /* WL11U */

#ifdef CONFIG_HW_ABS
	if(true == g_abs_enabled)
		dhd_ant_init(dhd);
#endif

done:
	return ret;
}


int
dhd_iovar(dhd_pub_t *pub, int ifidx, char *name, char *param_buf,
	  uint param_len, char *res_buf, uint res_len, int set)
{
	char *buf;
	int input_len;
	wl_ioctl_t ioc;
	int ret;

	if (res_len > WLC_IOCTL_MAXLEN || param_len > WLC_IOCTL_MAXLEN)
		return BCME_BADARG;

	input_len = strlen(name) + 1 + param_len;
	if (input_len > WLC_IOCTL_MAXLEN)
		return BCME_BADARG;
	buf = NULL;
	if (set) {
		if (res_buf || res_len != 0) {
			DHD_ERROR(("%s: SET wrong arguemnet\n", __FUNCTION__));
			return BCME_BADARG;
		}
		buf = kzalloc(input_len, GFP_KERNEL);
		if (!buf) {
			DHD_ERROR(("%s: mem alloc failed\n", __FUNCTION__));
			return BCME_NOMEM;
		}
		ret = bcm_mkiovar(name, param_buf, param_len, buf, input_len);
		if (!ret) {
			ret = BCME_NOMEM;
			goto exit;
		}

		ioc.cmd = WLC_SET_VAR;
		ioc.buf = buf;
		ioc.len = input_len;
		ioc.set = set;

		ret = dhd_wl_ioctl(pub, ifidx, &ioc, ioc.buf, ioc.len);

	} else {
		if (!res_buf || res_len == 0) {
			DHD_ERROR(("%s: GET failed. resp_buf NULL or len:0\n",
				   __FUNCTION__));
			return BCME_NOMEM;
		}
		if (res_len < (uint)input_len) {
			DHD_INFO(("%s: res_len(%d) < input_len(%d)\n",
				  __FUNCTION__, res_len, input_len));
			buf = kzalloc(input_len, GFP_KERNEL);
			if (!buf) {
				DHD_ERROR(("%s: mem alloc failed\n",
					   __FUNCTION__));
				return BCME_NOMEM;
			}
			ret = bcm_mkiovar(name, param_buf, param_len, buf,
					  input_len);
			if (!ret) {
				ret = BCME_NOMEM;
				goto exit;
			}

			ioc.cmd = WLC_GET_VAR;
			ioc.buf = buf;
			ioc.len = input_len;
			ioc.set = set;

			ret = dhd_wl_ioctl(pub, ifidx, &ioc, ioc.buf, ioc.len);

			if (ret == BCME_OK)
				memcpy(res_buf, buf, res_len);
		} else {
			memset(res_buf, 0, res_len);
			ret = bcm_mkiovar(name, param_buf, param_len, res_buf,
					  res_len);
			if (!ret) {
				ret = BCME_NOMEM;
				goto exit;
			}

			ioc.cmd = WLC_GET_VAR;
			ioc.buf = res_buf;
			ioc.len = res_len;
			ioc.set = set;

			ret = dhd_wl_ioctl(pub, ifidx, &ioc, ioc.buf, ioc.len);
		}
	}
exit:
	kfree(buf);
	return ret;
}

int
dhd_getiovar(dhd_pub_t *pub, int ifidx, char *name, char *cmd_buf,
	uint cmd_len, char **resptr, uint resp_len)
{
	int len = resp_len;
	int ret;
	char *buf = *resptr;
	wl_ioctl_t ioc;
	if (resp_len > WLC_IOCTL_MAXLEN)
		return BCME_BADARG;

	memset(buf, 0, resp_len);

	bcm_mkiovar(name, cmd_buf, cmd_len, buf, len);

	memset(&ioc, 0, sizeof(ioc));

	ioc.cmd = WLC_GET_VAR;
	ioc.buf = buf;
	ioc.len = len;
	ioc.set = 0;

	ret = dhd_wl_ioctl(pub, ifidx, &ioc, ioc.buf, ioc.len);

	return ret;
}

int
dhd_wl_ioctl_get_intiovar(dhd_pub_t *dhd_pub, char *name, uint *pval,
	int cmd, uint8 set, int ifidx)
{
	char iovbuf[WLC_IOCTL_SMLEN];
	int ret = -1;

	/* memset(iovbuf, 0, sizeof(iovbuf)); */
	if (bcm_mkiovar(name, NULL, 0, iovbuf, sizeof(iovbuf))) {
		ret = dhd_wl_ioctl_cmd(dhd_pub, cmd, iovbuf, sizeof(iovbuf), set, ifidx);
		if (!ret) {
			*pval = ltoh32(*((uint*)iovbuf));
		} else {
			DHD_ERROR(("%s: get int iovar %s failed, ERR %d\n",
				__FUNCTION__, name, ret));
		}
	} else {
		DHD_ERROR(("%s: mkiovar %s failed\n",
			__FUNCTION__, name));
	}

	return ret;
}

int
dhd_wl_ioctl_set_intiovar(dhd_pub_t *dhd_pub, char *name, uint val,
	int cmd, uint8 set, int ifidx)
{
	char iovbuf[WLC_IOCTL_SMLEN];
	int ret = -1;
	int lval = htol32(val);

	/* memset(iovbuf, 0, sizeof(iovbuf)); */
	if (bcm_mkiovar(name, (char*)&lval, sizeof(lval), iovbuf, sizeof(iovbuf))) {
		ret = dhd_wl_ioctl_cmd(dhd_pub, cmd, iovbuf, sizeof(iovbuf), set, ifidx);
		if (ret) {
			DHD_ERROR(("%s: set int iovar %s failed, ERR %d\n",
				__FUNCTION__, name, ret));
		}
	} else {
		DHD_ERROR(("%s: mkiovar %s failed\n",
			__FUNCTION__, name));
	}

	return ret;
}


int dhd_change_mtu(dhd_pub_t *dhdp, int new_mtu, int ifidx)
{
	struct dhd_info *dhd = dhdp->info;
	struct net_device *dev = NULL;

	ASSERT(dhd && dhd->iflist[ifidx]);
	dev = dhd->iflist[ifidx]->net;
	ASSERT(dev);

	if (netif_running(dev)) {
		DHD_ERROR(("%s: Must be down to change its MTU", dev->name));
		return BCME_NOTDOWN;
	}

#define DHD_MIN_MTU 1500
#define DHD_MAX_MTU 1752

	if ((new_mtu < DHD_MIN_MTU) || (new_mtu > DHD_MAX_MTU)) {
		DHD_ERROR(("%s: MTU size %d is invalid.\n", __FUNCTION__, new_mtu));
		return BCME_BADARG;
	}

	dev->mtu = new_mtu;
	return 0;
}

#ifdef ARP_OFFLOAD_SUPPORT
/* add or remove AOE host ip(s) (up to 8 IPs on the interface)  */
void
aoe_update_host_ipv4_table(dhd_pub_t *dhd_pub, u32 ipa, bool add, int idx)
{
	u32 ipv4_buf[MAX_IPV4_ENTRIES]; /* temp save for AOE host_ip table */
	int i;
	int ret;

	bzero(ipv4_buf, sizeof(ipv4_buf));

	/* display what we've got */
	ret = dhd_arp_get_arp_hostip_table(dhd_pub, ipv4_buf, sizeof(ipv4_buf), idx);
	DHD_ARPOE(("%s: hostip table read from Dongle:\n", __FUNCTION__));
#ifdef AOE_DBG
	dhd_print_buf(ipv4_buf, 32, 4); /* max 8 IPs 4b each */
#endif
	/* now we saved hoste_ip table, clr it in the dongle AOE */
	dhd_aoe_hostip_clr(dhd_pub, idx);

	if (ret) {
		DHD_ERROR(("%s failed\n", __FUNCTION__));
		return;
	}

	for (i = 0; i < MAX_IPV4_ENTRIES; i++) {
		if (add && (ipv4_buf[i] == 0)) {
				ipv4_buf[i] = ipa;
				add = FALSE; /* added ipa to local table  */
				DHD_ARPOE(("%s: Saved new IP in temp arp_hostip[%d]\n",
				__FUNCTION__, i));
		} else if (ipv4_buf[i] == ipa) {
			ipv4_buf[i]	= 0;
			DHD_ARPOE(("%s: removed IP:%x from temp table %d\n",
				__FUNCTION__, ipa, i));
		}

		if (ipv4_buf[i] != 0) {
			/* add back host_ip entries from our local cache */
			dhd_arp_offload_add_ip(dhd_pub, ipv4_buf[i], idx);
			DHD_ARPOE(("%s: added IP:%x to dongle arp_hostip[%d]\n\n",
				__FUNCTION__, ipv4_buf[i], i));
		}
	}
#ifdef AOE_DBG
	/* see the resulting hostip table */
	dhd_arp_get_arp_hostip_table(dhd_pub, ipv4_buf, sizeof(ipv4_buf), idx);
	DHD_ARPOE(("%s: read back arp_hostip table:\n", __FUNCTION__));
	dhd_print_buf(ipv4_buf, 32, 4); /* max 8 IPs 4b each */
#endif
}

/*
 * Notification mechanism from kernel to our driver. This function is called by the Linux kernel
 * whenever there is an event related to an IP address.
 * ptr : kernel provided pointer to IP address that has changed
 */
static int dhd_inetaddr_notifier_call(struct notifier_block *this,
	unsigned long event,
	void *ptr)
{
	struct in_ifaddr *ifa = (struct in_ifaddr *)ptr;

	dhd_info_t *dhd;
	dhd_pub_t *dhd_pub;
	int idx;

	if (!dhd_arp_enable)
		return NOTIFY_DONE;
	if (!ifa || !(ifa->ifa_dev->dev))
		return NOTIFY_DONE;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31))
	/* Filter notifications meant for non Broadcom devices */
	if ((ifa->ifa_dev->dev->netdev_ops != &dhd_ops_pri) &&
	    (ifa->ifa_dev->dev->netdev_ops != &dhd_ops_virt)) {
#if defined(WL_ENABLE_P2P_IF)
		if (!wl_cfgp2p_is_ifops(ifa->ifa_dev->dev->netdev_ops))
#endif /* WL_ENABLE_P2P_IF */
			return NOTIFY_DONE;
	}
#endif /* LINUX_VERSION_CODE */

	dhd = DHD_DEV_INFO(ifa->ifa_dev->dev);
	if (!dhd)
		return NOTIFY_DONE;

	dhd_pub = &dhd->pub;

	if (dhd_pub->arp_version == 1) {
		idx = 0;
	}
	else {
		for (idx = 0; idx < DHD_MAX_IFS; idx++) {
			if (dhd->iflist[idx] && dhd->iflist[idx]->net == ifa->ifa_dev->dev)
			break;
		}
		if (idx < DHD_MAX_IFS)
			DHD_TRACE(("ifidx : %p %s %d\n", dhd->iflist[idx]->net,
				dhd->iflist[idx]->name, dhd->iflist[idx]->idx));
		else {
			DHD_ERROR(("Cannot find ifidx for(%s) set to 0\n", ifa->ifa_label));
			idx = 0;
		}
	}

	switch (event) {
		case NETDEV_UP:
			DHD_ARPOE(("%s: [%s] Up IP: 0x%x\n",
				__FUNCTION__, ifa->ifa_label, ifa->ifa_address));

			if (dhd->pub.busstate != DHD_BUS_DATA) {
				DHD_ERROR(("%s: bus not ready, exit\n", __FUNCTION__));
				if (dhd->pend_ipaddr) {
					DHD_ERROR(("%s: overwrite pending ipaddr: 0x%x\n",
						__FUNCTION__, dhd->pend_ipaddr));
				}
				dhd->pend_ipaddr = ifa->ifa_address;
				break;
			}

#ifdef AOE_IP_ALIAS_SUPPORT
			DHD_ARPOE(("%s:add aliased IP to AOE hostip cache\n",
				__FUNCTION__));
			aoe_update_host_ipv4_table(dhd_pub, ifa->ifa_address, TRUE, idx);
#endif /* AOE_IP_ALIAS_SUPPORT */
			break;

		case NETDEV_DOWN:
			DHD_ARPOE(("%s: [%s] Down IP: 0x%x\n",
				__FUNCTION__, ifa->ifa_label, ifa->ifa_address));
			dhd->pend_ipaddr = 0;
#ifdef AOE_IP_ALIAS_SUPPORT
			DHD_ARPOE(("%s:interface is down, AOE clr all for this if\n",
				__FUNCTION__));
			aoe_update_host_ipv4_table(dhd_pub, ifa->ifa_address, FALSE, idx);
#else
			dhd_aoe_hostip_clr(&dhd->pub, idx);
			dhd_aoe_arp_clr(&dhd->pub, idx);
#endif /* AOE_IP_ALIAS_SUPPORT */
			break;

		default:
			DHD_ARPOE(("%s: do noting for [%s] Event: %lu\n",
				__func__, ifa->ifa_label, event));
			break;
	}
	return NOTIFY_DONE;
}
#endif /* ARP_OFFLOAD_SUPPORT */

#ifdef CONFIG_IPV6
/* Neighbor Discovery Offload: defered handler */
static void
dhd_inet6_work_handler(void *dhd_info, void *event_data, u8 event)
{
	struct ipv6_work_info_t *ndo_work = (struct ipv6_work_info_t *)event_data;
	dhd_pub_t	*pub = &((dhd_info_t *)dhd_info)->pub;
	int		ret;

	if (event != DHD_WQ_WORK_IPV6_NDO) {
		DHD_ERROR(("%s: unexpected event \n", __FUNCTION__));
		return;
	}

	if (!ndo_work) {
		DHD_ERROR(("%s: ipv6 work info is not initialized \n", __FUNCTION__));
		return;
	}

	if (!pub) {
		DHD_ERROR(("%s: dhd pub is not initialized \n", __FUNCTION__));
		return;
	}

	if (ndo_work->if_idx) {
		DHD_ERROR(("%s: idx %d \n", __FUNCTION__, ndo_work->if_idx));
		return;
	}

	if(dhd_get_fw_mode(pub->info) == DHD_FLAG_HOSTAP_MODE){
		DHD_ERROR(("%s: hostap mode not allowed enable ndoe \n", __FUNCTION__));
		return;
	}

	switch (ndo_work->event) {
		case NETDEV_UP:
			DHD_TRACE(("%s: Enable NDO and add ipv6 into table \n ", __FUNCTION__));
			ret = dhd_ndo_enable(pub, TRUE);
			if (ret < 0) {
				DHD_ERROR(("%s: Enabling NDO Failed %d\n", __FUNCTION__, ret));
			}

			ret = dhd_ndo_add_ip(pub, &ndo_work->ipv6_addr[0], ndo_work->if_idx);
			if (ret < 0) {
				DHD_ERROR(("%s: Adding host ip for NDO failed %d\n",
					__FUNCTION__, ret));
			}
			break;
		case NETDEV_DOWN:
			DHD_TRACE(("%s: clear ipv6 table \n", __FUNCTION__));
			ret = dhd_ndo_remove_ip(pub, ndo_work->if_idx);
			if (ret < 0) {
				DHD_ERROR(("%s: Removing host ip for NDO failed %d\n",
					__FUNCTION__, ret));
				goto done;
			}

			ret = dhd_ndo_enable(pub, FALSE);
			if (ret < 0) {
				DHD_ERROR(("%s: disabling NDO Failed %d\n", __FUNCTION__, ret));
				goto done;
			}
			break;
		default:
			DHD_ERROR(("%s: unknown notifier event \n", __FUNCTION__));
			break;
	}
done:
	/* free ndo_work. alloced while scheduling the work */
	kfree(ndo_work);

	return;
}

/*
 * Neighbor Discovery Offload: Called when an interface
 * is assigned with ipv6 address.
 * Handles only primary interface
 */
static int dhd_inet6addr_notifier_call(struct notifier_block *this,
	unsigned long event,
	void *ptr)
{
	dhd_info_t *dhd;
	dhd_pub_t *dhd_pub;
	struct inet6_ifaddr *inet6_ifa = ptr;
	struct in6_addr *ipv6_addr = &inet6_ifa->addr;
	struct ipv6_work_info_t *ndo_info;
	int idx = 0; /* REVISIT */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31))
	/* Filter notifications meant for non Broadcom devices */
	if (inet6_ifa->idev->dev->netdev_ops != &dhd_ops_pri) {
			return NOTIFY_DONE;
	}
#endif /* LINUX_VERSION_CODE */

	dhd = DHD_DEV_INFO(inet6_ifa->idev->dev);
	if (!dhd)
		return NOTIFY_DONE;

	if (dhd->iflist[idx] && dhd->iflist[idx]->net != inet6_ifa->idev->dev)
		return NOTIFY_DONE;
	dhd_pub = &dhd->pub;
	if (!FW_SUPPORTED(dhd_pub, ndoe))
		return NOTIFY_DONE;

	ndo_info = (struct ipv6_work_info_t *)kzalloc(sizeof(struct ipv6_work_info_t), GFP_ATOMIC);
	if (!ndo_info) {
		DHD_ERROR(("%s: ipv6 work alloc failed\n", __FUNCTION__));
		return NOTIFY_DONE;
	}

	ndo_info->event = event;
	ndo_info->if_idx = idx;
	memcpy(&ndo_info->ipv6_addr[0], ipv6_addr, IPV6_ADDR_LEN);

	/* defer the work to thread as it may block kernel */
	dhd_deferred_schedule_work(dhd->dhd_deferred_wq, (void *)ndo_info, DHD_WQ_WORK_IPV6_NDO,
		dhd_inet6_work_handler, DHD_WORK_PRIORITY_LOW);
	return NOTIFY_DONE;
}
#endif /* #ifdef CONFIG_IPV6 */

int
dhd_register_if(dhd_pub_t *dhdp, int ifidx, bool need_rtnl_lock)
{
	dhd_info_t *dhd = (dhd_info_t *)dhdp->info;
	dhd_if_t *ifp;
	struct net_device *net = NULL;
	int err = 0;
	uint8 temp_addr[ETHER_ADDR_LEN] = { 0x00, 0x90, 0x4c, 0x11, 0x22, 0x33 };

	DHD_TRACE(("%s: ifidx %d\n", __FUNCTION__, ifidx));

	ASSERT(dhd && dhd->iflist[ifidx]);
	if (NULL == dhd)
                return;

	ifp = dhd->iflist[ifidx];
	net = ifp->net;
	ASSERT(net && (ifp->idx == ifidx));
	if (NULL == net)
		return;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 31))
	ASSERT(!net->open);
	net->get_stats = dhd_get_stats;
	net->do_ioctl = dhd_ioctl_entry;
	net->hard_start_xmit = dhd_start_xmit;
	net->set_mac_address = dhd_set_mac_address;
	net->set_multicast_list = dhd_set_multicast_list;
	net->open = net->stop = NULL;
#else
	ASSERT(!net->netdev_ops);
	net->netdev_ops = &dhd_ops_virt;
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 31) */

	/* Ok, link into the network layer... */
	if (ifidx == 0) {
		/*
		 * device functions for the primary interface only
		 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 31))
		net->open = dhd_open;
		net->stop = dhd_stop;
#else
		net->netdev_ops = &dhd_ops_pri;
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 31) */
		if (!ETHER_ISNULLADDR(dhd->pub.mac.octet))
			memcpy(temp_addr, dhd->pub.mac.octet, ETHER_ADDR_LEN);
	} else {
		/*
		 * We have to use the primary MAC for virtual interfaces
		 */
		memcpy(temp_addr, ifp->mac_addr, ETHER_ADDR_LEN);
		/*
		 * Android sets the locally administered bit to indicate that this is a
		 * portable hotspot.  This will not work in simultaneous AP/STA mode,
		 * nor with P2P.  Need to set the Donlge's MAC address, and then use that.
		 */
		if (!memcmp(temp_addr, dhd->iflist[0]->mac_addr,
			ETHER_ADDR_LEN)) {
			DHD_ERROR(("%s interface [%s]: set locally administered bit in MAC\n",
			__func__, net->name));
			temp_addr[0] |= 0x02;
		}
	}

	net->hard_header_len = ETH_HLEN + dhd->pub.hdrlen;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
	net->ethtool_ops = &dhd_ethtool_ops;
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24) */

	dhd->pub.rxsz = DBUS_RX_BUFFER_SIZE_DHD(net);

	memcpy(net->dev_addr, temp_addr, ETHER_ADDR_LEN);

	if (ifidx == 0)
		printf("%s\n", dhd_version);

	if (need_rtnl_lock)
		err = register_netdev(net);
	else
		err = register_netdevice(net);

	if (err != 0) {
		DHD_ERROR(("couldn't register the net device [%s], err %d\n", net->name, err));
		goto fail;
	}



	printf("Register interface [%s]  MAC: "MACDBG"\n\n", net->name,
		MAC2STRDBG(net->dev_addr));

#if defined(BCMLXSDMMC) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27))
	if (ifidx == 0) {
#ifdef BCMLXSDMMC
		up(&dhd_registration_sem);
#endif
		if (!dhd_download_fw_on_driverload) {
#ifdef BCMSDIO
			dhd_net_bus_devreset(net, TRUE);
			dhd_net_bus_suspend(net);
#endif /* BCMSDIO */
			wifi_platform_set_power(dhdp->info->adapter, FALSE, WIFI_TURNOFF_DELAY);
		}
	}
#endif /* OEM_ANDROID && BCMLXSDMMC && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)) */

#if defined(BCMPCIE)
	if (ifidx == 0) {
		if (!dhd_download_fw_on_driverload) {
			dhd_net_bus_devreset(net, TRUE);
			wifi_platform_set_power(dhdp->info->adapter, FALSE, WIFI_TURNOFF_DELAY);
		}
	}
#endif /* BCMPCIE */
#ifdef HW_NETDEVICE_PANIC
	if (ifidx == 0) {
		hw_register_breakpoint(net, ifidx, net->name);
	}
#endif
	return 0;

fail:
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 31)
	net->open = NULL;
#else
	net->netdev_ops = NULL;
#endif
	return err;
}

void
dhd_bus_detach(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	if (dhdp) {
		dhd = (dhd_info_t *)dhdp->info;
		if (dhd) {

			/*
			 * In case of Android cfg80211 driver, the bus is down in dhd_stop,
			 *  calling stop again will cuase SD read/write errors.
			 */
			if (dhd->pub.busstate != DHD_BUS_DOWN) {
				/* Stop the protocol module */
				dhd_prot_stop(&dhd->pub);

				/* Stop the bus module */
				dhd_bus_stop(dhd->pub.bus, TRUE);
			}

#if defined(OOB_INTR_ONLY) || defined(BCMPCIE_OOB_HOST_WAKE)
			dhd_bus_oob_intr_unregister(dhdp);
#endif
		}
	}
}


void dhd_detach(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd;
	unsigned long flags;
	int timer_valid = FALSE;
#ifdef BCM_PCIE_UPDATE
	struct net_device *dev;
#endif
	if (!dhdp)
		return;

	dhd = (dhd_info_t *)dhdp->info;
	if (!dhd)
		return;
#ifdef BCM_PCIE_UPDATE
	dev = dhd->iflist[0]->net;

	if (dev) {
		rtnl_lock();
		if (dev->flags & IFF_UP) {
			/* If IFF_UP is still up, it indicates that
			 * "ifconfig wlan0 down" hasn't been called.
			 * So invoke dev_close explicitly here to
			 * bring down the interface.
			 */
			DHD_TRACE(("IFF_UP flag is up. Enforcing dev_close from detach \n"));
			dev_close(dev);
		}
		rtnl_unlock();
	}
#endif
	DHD_TRACE(("%s: Enter state 0x%x\n", __FUNCTION__, dhd->dhd_state));

#ifdef HW_SDIO_QUALITY_STATISTIC
	if (dhd->pub.sdio_quality) {
		sdio_quality_deinit(&dhd->pub);
	}
#endif

	dhd->pub.up = 0;
	if (!(dhd->dhd_state & DHD_ATTACH_STATE_DONE)) {
		/* Give sufficient time for threads to start running in case
		 * dhd_attach() has failed
		 */
		OSL_SLEEP(100);
	}

	if (dhd->dhd_state & DHD_ATTACH_STATE_PROT_ATTACH) {
#ifndef BCM_PCIE_UPDATE
#ifdef PCIE_FULL_DONGLE
		dhd_flow_rings_deinit(dhdp);
#endif
		dhd_bus_detach(dhdp);

		if (dhdp->prot)
			dhd_prot_detach(dhdp);
#else /* BCM_PCIE_UPDATE */
			dhd_bus_detach(dhdp);
#ifdef BCMPCIE
		if (is_reboot == SYS_RESTART) {
			extern bcmdhd_wifi_platdata_t *dhd_wifi_platdata;
			if (dhd_wifi_platdata && !dhdp->dongle_reset) {
				dhdpcie_bus_clock_stop(dhdp->bus);
				wifi_platform_set_power(dhd_wifi_platdata->adapters,
					FALSE, WIFI_TURNOFF_DELAY);
			}
		}
#endif /* BCMPCIE */
#ifndef PCIE_FULL_DONGLE
		if (dhdp->prot)
			dhd_prot_detach(dhdp);
#endif
#endif /* BCM_PCIE_UPDATE */
	}
#ifdef PROP_TXSTATUS
#ifdef WLFC_STATE_PREALLOC
	MFREE(dhd->pub.osh, dhd->pub.wlfc_state, sizeof(athost_wl_status_info_t));
#endif /* WLFC_STATE_PREALLOC */
#endif /* PROP_TXSTATUS */

#ifdef ARP_OFFLOAD_SUPPORT
	if (dhd_inetaddr_notifier_registered) {
		dhd_inetaddr_notifier_registered = FALSE;
		unregister_inetaddr_notifier(&dhd_inetaddr_notifier);
	}
#endif /* ARP_OFFLOAD_SUPPORT */
#ifdef CONFIG_IPV6
	if (dhd_inet6addr_notifier_registered) {
		dhd_inet6addr_notifier_registered = FALSE;
		unregister_inet6addr_notifier(&dhd_inet6addr_notifier);
	}
#endif /* CONFIG_IPV6 && IPV6_NDO_SUPPORT */
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(DHD_USE_EARLYSUSPEND)
	if (dhd->dhd_state & DHD_ATTACH_STATE_EARLYSUSPEND_DONE) {
		if (dhd->early_suspend.suspend)
			unregister_early_suspend(&dhd->early_suspend);
	}
#endif /* CONFIG_HAS_EARLYSUSPEND && DHD_USE_EARLYSUSPEND */

	/* delete all interfaces, start with virtual  */
	if (dhd->dhd_state & DHD_ATTACH_STATE_ADD_IF) {
		int i = 1;
		dhd_if_t *ifp;

		/* Cleanup virtual interfaces */
		dhd_net_if_lock_local(dhd);
		for (i = 1; i < DHD_MAX_IFS; i++) {
			if (dhd->iflist[i])
				dhd_remove_if(&dhd->pub, i, TRUE);
		}
		dhd_net_if_unlock_local(dhd);

		/*  delete primary interface 0 */
		ifp = dhd->iflist[0];
		ASSERT(ifp);
		ASSERT(ifp->net);
		if (ifp && ifp->net) {



			/* in unregister_netdev case, the interface gets freed by net->destructor
			 * (which is set to free_netdev)
			 */
			if (ifp->net->reg_state == NETREG_UNINITIALIZED) {
				free_netdev(ifp->net);
			} else {
#ifdef BCM_PCIE_UPDATE
				netif_tx_disable(ifp->net);
#endif
				unregister_netdev(ifp->net);
			}
			ifp->net = NULL;
#ifdef DHD_WMF
			dhd_wmf_cleanup(dhdp, 0);
#endif /* DHD_WMF */

			dhd_if_del_sta_list(ifp);

			MFREE(dhd->pub.osh, ifp, sizeof(*ifp));
			dhd->iflist[0] = NULL;
		}
	}

	/* Clear the watchdog timer */
	DHD_GENERAL_LOCK(&dhd->pub, flags);
	timer_valid = dhd->wd_timer_valid;
	dhd->wd_timer_valid = FALSE;
	DHD_GENERAL_UNLOCK(&dhd->pub, flags);
	if (timer_valid)
		del_timer_sync(&dhd->timer);

	if (dhd->dhd_state & DHD_ATTACH_STATE_THREADS_CREATED) {
		if (dhd->thr_wdt_ctl.thr_pid >= 0) {
			PROC_STOP(&dhd->thr_wdt_ctl);
		}

		if (dhd->rxthread_enabled && dhd->thr_rxf_ctl.thr_pid >= 0) {
			PROC_STOP(&dhd->thr_rxf_ctl);
		}

		if (dhd->thr_dpc_ctl.thr_pid >= 0) {
			PROC_STOP(&dhd->thr_dpc_ctl);
		} else {
			tasklet_kill(&dhd->tasklet);
		}
#ifdef DHD_DEVWAKE_EARLY
		if (dhd->devwake_tsk.thr_pid >= 0) {
			PROC_STOP(&dhd->devwake_tsk);
		}
#endif /* DHD_DEVWAKE_EARLY */
#ifdef DHD_TPUT_MONITOR
		if (dhd->tput_tsk.thr_pid >= 0) {
			PROC_STOP(&dhd->tput_tsk);
		}
#endif /* DHD_TPUT_MONITOR */
	}
#ifdef WL_CFG80211
	if (dhd->dhd_state & DHD_ATTACH_STATE_CFG80211) {
		wl_cfg80211_detach(NULL);
		dhd_monitor_uninit();
	}
#endif
	/* free deferred work queue */
	dhd_deferred_work_deinit(dhd->dhd_deferred_wq);
	dhd->dhd_deferred_wq = NULL;

	if (dhdp->dbg)
		dhd_os_dbg_detach(dhdp);
#ifdef SHOW_LOGTRACE
	if (dhd->event_data.fmts)
		kfree(dhd->event_data.fmts);
	if (dhd->event_data.raw_fmts)
		kfree(dhd->event_data.raw_fmts);
#endif /* SHOW_LOGTRACE */

#ifdef PNO_SUPPORT
	if (dhdp->pno_state)
		dhd_pno_deinit(dhdp);
#endif
#ifdef RTT_SUPPORT
	if (dhdp->rtt_state)
		dhd_rtt_deinit(dhdp);
#endif
#if defined(CONFIG_PM_SLEEP)
	if (dhd_pm_notifier_registered) {
		unregister_pm_notifier(&dhd_pm_notifier);
		dhd_pm_notifier_registered = FALSE;
	}
#endif /* CONFIG_PM_SLEEP */
#ifdef SAR_SUPPORT
	if (dhd_sar_notifier_registered) {
		unregister_notifier_by_sar(&dhd->sar_notifier);
		dhd_sar_notifier_registered = FALSE;
	}
#endif /* SAR_SUPPORT */
#ifdef DEBUG_CPU_FREQ
		if (dhd->new_freq)
			free_percpu(dhd->new_freq);
		dhd->new_freq = NULL;
		cpufreq_unregister_notifier(&dhd->freq_trans, CPUFREQ_TRANSITION_NOTIFIER);
#endif

#if ((defined CONFIG_HAS_WAKELOCK) && (defined BCM_PCIE_UPDATE))
	wake_lock_destroy(&dhd->wl_wdwake);
#endif

	if (dhd->dhd_state & DHD_ATTACH_STATE_WAKELOCKS_INIT) {
		DHD_TRACE(("wd wakelock count:%d\n", dhd->wakelock_wd_counter));
#ifdef CONFIG_HAS_WAKELOCK
#ifndef BCM_PCIE_UPDATE
		dhd->wakelock_counter = 0;
#endif
		dhd->wakelock_wd_counter = 0;
#ifndef BCM_PCIE_UPDATE
		dhd->wakelock_rx_timeout_enable = 0;
		dhd->wakelock_ctrl_timeout_enable = 0;
		wake_lock_destroy(&dhd->wl_wifi);
		wake_lock_destroy(&dhd->wl_rxwake);
		wake_lock_destroy(&dhd->wl_ctrlwake);
		wake_lock_destroy(&dhd->wl_wdwake);
#endif

#endif /* CONFIG_HAS_WAKELOCK */
#ifdef BCM_PCIE_UPDATE
		DHD_OS_WAKE_LOCK_DESTROY(dhd);
#endif
	}

	if (dhdp->soc_ram) {
		MFREE(dhdp->osh, dhdp->soc_ram, dhdp->soc_ram_length);
	}
#ifdef DHDTCPACK_SUPPRESS
	/* This will free all MEM allocated for TCPACK SUPPRESS */
	dhd_tcpack_suppress_set(&dhd->pub, TCPACK_SUP_OFF);
#endif /* DHDTCPACK_SUPPRESS */
#ifdef BCM_PCIE_UPDATE
#ifdef PCIE_FULL_DONGLE
		dhd_flow_rings_deinit(dhdp);
		if (dhdp->prot)
			dhd_prot_detach(dhdp);
#endif

#ifdef DHD_TRACE_WAKE_LOCK
	dhd_sysfs_exit(dhd);
#endif
	dhd->pub.is_fw_download_done = FALSE;
#endif
#ifdef CONFIG_HW_WLANFTY_STATUS
	hw_dhd_wlanfty_detach();
#endif
#ifdef HW_DOZE_PKT_FILTER
	hw_detach_dhd_pub_t();
#endif
	dhd_unregister_handle();
}


void
dhd_free(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd;
	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	if (dhdp) {
		int i;
		for (i = 0; i < ARRAYSIZE(dhdp->reorder_bufs); i++) {
			if (dhdp->reorder_bufs[i]) {
				reorder_info_t *ptr;
				uint32 buf_size = sizeof(struct reorder_info);

				ptr = dhdp->reorder_bufs[i];

				buf_size += ((ptr->max_idx + 1) * sizeof(void*));
				DHD_REORDER(("free flow id buf %d, maxidx is %d, buf_size %d\n",
					i, ptr->max_idx, buf_size));

				MFREE(dhdp->osh, dhdp->reorder_bufs[i], buf_size);
				dhdp->reorder_bufs[i] = NULL;
			}
		}

		dhd_sta_pool_fini(dhdp, DHD_MAX_STA);

		dhd = (dhd_info_t *)dhdp->info;
		/* If pointer is allocated by dhd_os_prealloc then avoid MFREE */
		if (dhd &&
			dhd != (dhd_info_t *)dhd_os_prealloc(dhdp, DHD_PREALLOC_DHD_INFO, 0, FALSE))
			MFREE(dhd->pub.osh, dhd, sizeof(*dhd));
		dhd = NULL;

		if (dhdp->soc_ram) {
                        memset(dhdp->soc_ram, 0, dhdp->soc_ram_length);
		}

	}

}
void
dhd_clear(dhd_pub_t *dhdp)
{
	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	if (dhdp) {
#ifdef PCIE_FULL_DONGLE
		int i;
		for (i = 0; i < ARRAYSIZE(dhdp->reorder_bufs); i++) {
			if (dhdp->reorder_bufs[i]) {
				reorder_info_t *ptr;
				uint32 buf_size = sizeof(struct reorder_info);
				ptr = dhdp->reorder_bufs[i];
				buf_size += ((ptr->max_idx + 1) * sizeof(void*));
				DHD_REORDER(("free flow id buf %d, maxidx is %d, buf_size %d\n",
					i, ptr->max_idx, buf_size));

				MFREE(dhdp->osh, dhdp->reorder_bufs[i], buf_size);
				dhdp->reorder_bufs[i] = NULL;
			}
		}
		dhd_sta_pool_clear(dhdp, DHD_MAX_STA);
#endif

		if (dhdp->soc_ram) {
			memset(dhdp->soc_ram, 0, dhdp->soc_ram_length);
		}
	}

}

static void
dhd_module_cleanup(void)
{
	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	dhd_bus_unregister();

	wl_android_exit();

	dhd_wifi_platform_unregister_drv();
}

static void __exit
dhd_module_exit(void)
{
	dhd_module_cleanup();
	unregister_reboot_notifier(&dhd_reboot_notifier);
#if defined(DHD_OF_SUPPORT)
	dhd_wlan_exit();
#endif /* defined(DHD_OF_SUPPORT) */
}
#ifdef HW_PCIE_NOCLOG
typedef void (* WIFI_DUMP_FUNC) (void);
extern void register_wifi_dump_func(WIFI_DUMP_FUNC func);
extern void dhd_dump_local(void);
#endif
static int __init
dhd_module_init(void)
{
	int err;
	int retry = POWERUP_MAX_RETRY;
#if (defined(CONFIG_BCMDHD_PCIE) && defined(HW_WIFI_DMD_LOG) && defined(CONFIG_ARCH_HISI))
	char dmdbuf[DHD_PCIE_BUF_SIZE + 1] = {0};
#endif

	DHD_ERROR(("%s in\n", __FUNCTION__));
#ifdef CONFIG_HWCONNECTIVITY
	//For OneTrack, we need check it's the right chip type or not.
	//If it's not the right chip type, don't init the driver
	if(!isMyConnectivityChip(CHIP_TYPE_BCM)) {
		DHD_ERROR(("wifi-dhd chip type is not match, skip driver init"));
		return -EINVAL;
	} else {
		DHD_INFO(("wifi-dhd chip type is matched with Broadcom, continue"));
	}
#endif

#if defined(DHD_OF_SUPPORT)
	err = dhd_wlan_init();
	if(err) {
		DHD_ERROR(("%s: failed in dhd_wlan_init.",__FUNCTION__));
		return err;
	}
#endif /* defined(DHD_OF_SUPPORT) */


	DHD_PERIM_RADIO_INIT();
#ifdef HW_WIFI_DMD_LOG
	hw_register_wifi_dsm_client();
#endif
#ifdef HW_PCIE_NOCLOG
	register_wifi_dump_func(dhd_dump_local);
#endif

	if (firmware_path[0] != '\0') {
		strncpy(fw_bak_path, firmware_path, MOD_PARAM_PATHLEN);
		fw_bak_path[MOD_PARAM_PATHLEN-1] = '\0';
	}

	if (nvram_path[0] != '\0') {
		strncpy(nv_bak_path, nvram_path, MOD_PARAM_PATHLEN);
		nv_bak_path[MOD_PARAM_PATHLEN-1] = '\0';
	}

	do {
		err = dhd_wifi_platform_register_drv();
		if (!err) {
			register_reboot_notifier(&dhd_reboot_notifier);
			break;
		}
		else {
			DHD_ERROR(("%s: Failed to load the driver, try cnt %d\n",
				__FUNCTION__, retry));
			strncpy(firmware_path, fw_bak_path, MOD_PARAM_PATHLEN);
			firmware_path[MOD_PARAM_PATHLEN-1] = '\0';
			strncpy(nvram_path, nv_bak_path, MOD_PARAM_PATHLEN);
			nvram_path[MOD_PARAM_PATHLEN-1] = '\0';
		}
	} while (retry--);

	if (err) {
		DHD_ERROR(("%s: Failed to load driver max retry reached**\n", __FUNCTION__));
	} else {
		dhd_register_handle();
#ifdef BCM_PCIE_UPDATE
		if (!dhd_download_fw_on_driverload) {
			dhd_driver_init_done = TRUE;
		}
#endif
	}

	DHD_ERROR(("%s out\n", __FUNCTION__));
#ifdef HW_WIFI_DMD_LOG
	DHD_ERROR(("0---- %s in, return = %d\n", __FUNCTION__, err));
	if(0 != err)
    {
        set_wifi_open_err_code(err);
#ifdef HW_WIFI_DMD_LOG
#if (defined(CONFIG_BCMDHD_PCIE) && defined(CONFIG_ARCH_HISI))
		dsm_pcie_dump_reginfo(dmdbuf, DHD_PCIE_BUF_SIZE);
		hw_wifi_dsm_client_notify(DSM_WIFI_MODULE_INIT_ERROR,
				"init wifi driver failed in dhd_module_init %s\n", dmdbuf);
#else
		hw_wifi_dsm_client_notify(DSM_WIFI_MODULE_INIT_ERROR,
		          "init wifi driver failed in dhd_module_init %s\n", hw_dmd_get_trace_log());
#endif /* CONFIG_BCMDHD_PCIE */
#endif
		DHD_ERROR(("1--- %s in, return = %d\n", __FUNCTION__, err));
    }
#endif

	return err;
}

static int
dhd_reboot_callback(struct notifier_block *this, unsigned long code, void *unused)
{
	DHD_TRACE(("%s: code = %ld\n", __FUNCTION__, code));
	if (code == SYS_RESTART) {
#ifdef BCMPCIE
		is_reboot = code;
#endif /* BCMPCIE */
	}

	return NOTIFY_DONE;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
#if defined(CONFIG_DEFERRED_INITCALLS)
deferred_module_init(dhd_module_init);
#elif defined(USE_LATE_INITCALL_SYNC)
late_initcall_sync(dhd_module_init);
#else
late_initcall(dhd_module_init);
#endif /* USE_LATE_INITCALL_SYNC */
#else
module_init(dhd_module_init);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0) */

module_exit(dhd_module_exit);

/*
 * OS specific functions required to implement DHD driver in OS independent way
 */
int
dhd_os_proto_block(dhd_pub_t *pub)
{
	dhd_info_t * dhd = (dhd_info_t *)(pub->info);

	if (dhd) {
		DHD_PERIM_UNLOCK(pub);

		down(&dhd->proto_sem);

		DHD_PERIM_LOCK(pub);
		return 1;
	}

	return 0;
}

int
dhd_os_proto_unblock(dhd_pub_t *pub)
{
	dhd_info_t * dhd = (dhd_info_t *)(pub->info);

	if (dhd) {
		up(&dhd->proto_sem);
		return 1;
	}

	return 0;
}
#ifdef BCM_PCIE_UPDATE
void
dhd_os_dhdiovar_lock(dhd_pub_t *pub)
{
	dhd_info_t * dhd = (dhd_info_t *)(pub->info);

	if (dhd) {
		mutex_lock(&dhd->dhd_iovar_mutex);
	}
}

void
dhd_os_dhdiovar_unlock(dhd_pub_t *pub)
{
	dhd_info_t * dhd = (dhd_info_t *)(pub->info);

	if (dhd) {
		mutex_unlock(&dhd->dhd_iovar_mutex);
	}
}
#endif
unsigned int
dhd_os_get_ioctl_resp_timeout(void)
{
	return ((unsigned int)dhd_ioctl_timeout_msec);
}

void
dhd_os_set_ioctl_resp_timeout(unsigned int timeout_msec)
{
	dhd_ioctl_timeout_msec = (int)timeout_msec;
}
#ifndef BCM_PCIE_UPDATE
int
dhd_os_ioctl_resp_wait(dhd_pub_t *pub, uint *condition, bool *pending)
#else
int 
dhd_os_ioctl_resp_wait(dhd_pub_t * pub, uint * condition)
#endif
{
	dhd_info_t * dhd = (dhd_info_t *)(pub->info);
	int timeout;

	/* Convert timeout in millsecond to jiffies */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27))
	timeout = msecs_to_jiffies(dhd_ioctl_timeout_msec);
#else
	timeout = dhd_ioctl_timeout_msec * HZ / 1000;
#endif

	DHD_PERIM_UNLOCK(pub);

	timeout = wait_event_timeout(dhd->ioctl_resp_wait, (*condition), timeout);

	DHD_PERIM_LOCK(pub);

	return timeout;
}

int
dhd_os_ioctl_resp_wake(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);

	wake_up(&dhd->ioctl_resp_wait);
	return 0;
}
#ifndef BCM_PCIE_UPDATE
int
dhd_os_d3ack_wait(dhd_pub_t *pub, uint *condition, bool *pending)
#else
int
dhd_os_d3ack_wait(dhd_pub_t *pub, uint *condition)
#endif
{
	dhd_info_t * dhd = (dhd_info_t *)(pub->info);
	int timeout;

	/* Convert timeout in millsecond to jiffies */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27))
	timeout = msecs_to_jiffies(dhd_ioctl_timeout_msec);
#else
	timeout = dhd_ioctl_timeout_msec * HZ / 1000;
#endif

	DHD_PERIM_UNLOCK(pub);
	timeout = wait_event_timeout(dhd->d3ack_wait, (*condition), timeout);
	DHD_PERIM_LOCK(pub);

	return timeout;
}

int
dhd_os_d3ack_wake(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);

	wake_up(&dhd->d3ack_wait);
	return 0;
}
#ifdef BCM_PCIE_UPDATE
#ifdef DHD_DEVWAKE_EARLY
bool
dhd_os_devwake_check_deepsleep(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);

	/* NOTE: aleady called inside general spin lock.
	   Don't use spin lock here */
	if (!pub->devwake_enable) {
		return false;
	}

	if (!pub->deepsleep) {
		return false;
	}

	if (dhd->devwake_tsk.thr_pid >= 0) {
		if (!binary_sema_task_is_running(&dhd->devwake_tsk)) {
			dhd_bus_stop_queue(dhd->pub.bus);
		}

		if (binary_sema_up(&dhd->devwake_tsk)) {
			DHD_TRACE(("%s: schedule dev wake thread+++\n", __FUNCTION__));
		}
	}

	return true;
}

int
dhd_os_devwake_wait(dhd_pub_t *pub, uint *condition)
{
	dhd_info_t * dhd = (dhd_info_t *)(pub->info);
	int timeout;

	/* Convert timeout in millsecond to jiffies */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27))
	timeout = msecs_to_jiffies(DHD_DEVWAKE_WAIT_MS);
#else
	timeout = DHD_DEVWAKE_WAIT_MS * HZ / 1000;
#endif

	DHD_PERIM_UNLOCK(pub);

	/* wait until deepsleep become 0 */
	timeout = wait_event_timeout(dhd->devwake_wait, !(*condition), timeout);

	DHD_PERIM_LOCK(pub);

	return timeout;
}

int
dhd_os_devwake_intr(dhd_pub_t *pub, uint deepsleep)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);

	pub->deepsleep = deepsleep;
	smp_mb();
	if (!deepsleep) {
		if (waitqueue_active(&dhd->devwake_wait)) {
			wake_up(&dhd->devwake_wait);
			DHD_TRACE(("%s: wake up devwake_wait\n", __FUNCTION__));
		}
	}
	return 0;
}

#endif /* DHD_DEVWAKE_EARLY */

int
dhd_os_busbusy_wait_negation(dhd_pub_t *pub, uint *condition)
{
	dhd_info_t * dhd = (dhd_info_t *)(pub->info);
	int timeout;

	/* Wait for bus usage contexts to gracefully exit within some timeout value
	 * Set time out to little higher than dhd_ioctl_timeout_msec,
	 * so that IOCTL timeout should not get affected.
	 */
	/* Convert timeout in millsecond to jiffies */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27))
	timeout = msecs_to_jiffies(DHD_BUS_BUSY_TIMEOUT);
#else
	timeout = DHD_BUS_BUSY_TIMEOUT * HZ / 1000;
#endif

	timeout = wait_event_timeout(dhd->dhd_bus_busy_state_wait, !(*condition), timeout);

	return timeout;
}

int INLINE
dhd_os_busbusy_wake(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);
	/* Call wmb() to make sure before waking up the other event value gets updated */
	OSL_SMP_WMB();
	wake_up(&dhd->dhd_bus_busy_state_wait);
	return 0;
}
#endif
void
dhd_os_wd_timer_extend(void *bus, bool extend)
{
	dhd_pub_t *pub = bus;
	dhd_info_t *dhd = (dhd_info_t *)pub->info;

	if (extend)
		dhd_os_wd_timer(bus, WATCHDOG_EXTEND_INTERVAL);
	else
		dhd_os_wd_timer(bus, dhd->default_wd_interval);
}


void
dhd_os_wd_timer(void *bus, uint wdtick)
{
	dhd_pub_t *pub = bus;
	dhd_info_t *dhd = (dhd_info_t *)pub->info;
	unsigned long flags;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	if (!dhd) {
		DHD_ERROR(("%s: dhd NULL\n", __FUNCTION__));
		return;
	}
#ifdef BCM_PCIE_UPDATE
	DHD_OS_WD_WAKE_LOCK(pub);
#endif
	DHD_GENERAL_LOCK(pub, flags);

	/* don't start the wd until fw is loaded */
	if (pub->busstate == DHD_BUS_DOWN) {
		DHD_GENERAL_UNLOCK(pub, flags);
		if (!wdtick)
			DHD_OS_WD_WAKE_UNLOCK(pub);
		return;
	}

	/* Totally stop the timer */
	if (!wdtick && dhd->wd_timer_valid == TRUE) {
		dhd->wd_timer_valid = FALSE;
		DHD_GENERAL_UNLOCK(pub, flags);
		del_timer_sync(&dhd->timer);
		DHD_OS_WD_WAKE_UNLOCK(pub);
		return;
	}
#ifdef HW_WIFI_WAKELOCK_BUGFIX
	if(dhd->pub.up == 0){
		DHD_GENERAL_UNLOCK(pub, flags);
		DHD_OS_WD_WAKE_UNLOCK(pub);
		return;
	}
#endif

	if (wdtick) {
		DHD_OS_WD_WAKE_LOCK(pub);
		dhd_watchdog_ms = (uint)wdtick;
		/* Re arm the timer, at last watchdog period */
		mod_timer(&dhd->timer, jiffies + msecs_to_jiffies(dhd_watchdog_ms));
		dhd->wd_timer_valid = TRUE;
	}
	DHD_GENERAL_UNLOCK(pub, flags);
#ifdef BCM_PCIE_UPDATE
	DHD_OS_WD_WAKE_UNLOCK(pub);
#endif
}

void *
dhd_os_open_image(char *filename)
{
	struct file *fp;

	fp = filp_open(filename, O_RDONLY, 0);
	/*
	 * 2.6.11 (FC4) supports filp_open() but later revs don't?
	 * Alternative:
	 * fp = open_namei(AT_FDCWD, filename, O_RD, 0);
	 * ???
	 */
	 if (IS_ERR(fp))
		 fp = NULL;

	 return fp;
}

int
dhd_os_get_image_block(char *buf, int len, void *image)
{
	struct file *fp = (struct file *)image;
	int rdlen;

	if (!image)
		return 0;

	rdlen = kernel_read(fp, fp->f_pos, buf, len);
	if (rdlen > 0)
		fp->f_pos += rdlen;

	return rdlen;
}

void
dhd_os_close_image(void *image)
{
	if (image)
		filp_close((struct file *)image, NULL);
}

void
dhd_os_sdlock(dhd_pub_t *pub)
{
	dhd_info_t *dhd;

	dhd = (dhd_info_t *)(pub->info);

	if (dhd_dpc_prio >= 0)
		mutex_lock(&dhd->sdmutex);
	else
		spin_lock_bh(&dhd->sdlock);
}

void
dhd_os_sdunlock(dhd_pub_t *pub)
{
	dhd_info_t *dhd;

	dhd = (dhd_info_t *)(pub->info);

	if (dhd_dpc_prio >= 0)
		mutex_unlock(&dhd->sdmutex);
	else
		spin_unlock_bh(&dhd->sdlock);
}

void
dhd_os_sdlock_txq(dhd_pub_t *pub)
{
	dhd_info_t *dhd;

	dhd = (dhd_info_t *)(pub->info);
	spin_lock_bh(&dhd->txqlock);
}

void
dhd_os_sdunlock_txq(dhd_pub_t *pub)
{
	dhd_info_t *dhd;

	dhd = (dhd_info_t *)(pub->info);
	spin_unlock_bh(&dhd->txqlock);
}

void
dhd_os_sdlock_rxq(dhd_pub_t *pub)
{
}

void
dhd_os_sdunlock_rxq(dhd_pub_t *pub)
{
}

static void
dhd_os_rxflock(dhd_pub_t *pub)
{
	dhd_info_t *dhd;

	dhd = (dhd_info_t *)(pub->info);
	spin_lock_bh(&dhd->rxf_lock);

}

static void
dhd_os_rxfunlock(dhd_pub_t *pub)
{
	dhd_info_t *dhd;

	dhd = (dhd_info_t *)(pub->info);
	spin_unlock_bh(&dhd->rxf_lock);
}

#ifdef DHDTCPACK_SUPPRESS
void
dhd_os_tcpacklock(dhd_pub_t *pub)
{
	dhd_info_t *dhd;

	dhd = (dhd_info_t *)(pub->info);
	spin_lock_bh(&dhd->tcpack_lock);

}

void
dhd_os_tcpackunlock(dhd_pub_t *pub)
{
	dhd_info_t *dhd;

	dhd = (dhd_info_t *)(pub->info);
	spin_unlock_bh(&dhd->tcpack_lock);
}
#endif /* DHDTCPACK_SUPPRESS */

uint8* dhd_os_prealloc(dhd_pub_t *dhdpub, int section, uint size, bool kmalloc_if_fail)
{
	uint8* buf;
	gfp_t flags = CAN_SLEEP() ? GFP_KERNEL: GFP_ATOMIC;

	buf = (uint8*)wifi_platform_prealloc(dhdpub->info->adapter, section, size);
	if (buf == NULL && kmalloc_if_fail)
		buf = kmalloc(size, flags);

	return buf;
}

void dhd_os_prefree(dhd_pub_t *dhdpub, void *addr, uint size)
{
}

#if defined (BCM_PATCH_FOR_ETHERTYPE_SECURITY)
static int
dhd_wl_host_event(dhd_info_t *dhd, int *ifidx, void *pktdata, uint pktlen,
	wl_event_msg_t *event, void **data)
#else
static int
dhd_wl_host_event(dhd_info_t *dhd, int *ifidx, void *pktdata,
	wl_event_msg_t *event, void **data)
#endif/*BCM_PATCH_FOR_ETHERTYPE_SECURITY*/
{
	int bcmerror = 0;
	ASSERT(dhd != NULL);

#ifdef SHOW_LOGTRACE
#if defined (BCM_PATCH_FOR_ETHERTYPE_SECURITY)
		bcmerror = wl_host_event(&dhd->pub, ifidx, pktdata, pktlen, event, data, &dhd->event_data);
#else
		bcmerror = wl_host_event(&dhd->pub, ifidx, pktdata, event, data, &dhd->event_data);
#endif/*BCM_PATCH_FOR_ETHERTYPE_SECURITY*/
#else
#if defined (BCM_PATCH_FOR_ETHERTYPE_SECURITY)
		bcmerror = wl_host_event(&dhd->pub, ifidx, pktdata, pktlen, event, data, NULL);
#else
		bcmerror = wl_host_event(&dhd->pub, ifidx, pktdata, event, data, NULL);
#endif/*BCM_PATCH_FOR_ETHERTYPE_SECURITY*/
#endif /* SHOW_LOGTRACE */

	if (bcmerror != BCME_OK)
		return (bcmerror);

#ifdef WL_CFG80211
	ASSERT(dhd->iflist[*ifidx] != NULL);
	ASSERT(dhd->iflist[*ifidx]->net != NULL);
	if (dhd->iflist[*ifidx]->net)
		wl_cfg80211_event(dhd->iflist[*ifidx]->net, event, *data);
#endif /* defined(WL_CFG80211) */

	return (bcmerror);
}

/* send up locally generated event */
void
dhd_sendup_event(dhd_pub_t *dhdp, wl_event_msg_t *event, void *data)
{
	switch (ntoh32(event->event_type)) {

	default:
		break;
	}
}

#ifdef LOG_INTO_TCPDUMP
void
dhd_sendup_log(dhd_pub_t *dhdp, void *data, int data_len)
{
	struct sk_buff *p, *skb;
	uint32 pktlen;
	int len;
	dhd_if_t *ifp;
	dhd_info_t *dhd;
	uchar *skb_data;
	int ifidx = 0;
	struct ether_header eth;

	pktlen = sizeof(eth) + data_len;
	dhd = dhdp->info;

	if ((p = PKTGET(dhdp->osh, pktlen, FALSE))) {
		ASSERT(ISALIGNED((uintptr)PKTDATA(dhdp->osh, p), sizeof(uint32)));

		bcopy(&dhdp->mac, &eth.ether_dhost, ETHER_ADDR_LEN);
		bcopy(&dhdp->mac, &eth.ether_shost, ETHER_ADDR_LEN);
		ETHER_TOGGLE_LOCALADDR(&eth.ether_shost);
		eth.ether_type = hton16(ETHER_TYPE_BRCM);

		bcopy((void *)&eth, PKTDATA(dhdp->osh, p), sizeof(eth));
		bcopy(data, PKTDATA(dhdp->osh, p) + sizeof(eth), data_len);
		skb = PKTTONATIVE(dhdp->osh, p);
		skb_data = skb->data;
		len = skb->len;

		ifidx = dhd_ifname2idx(dhd, "wlan0");
		ifp = dhd->iflist[ifidx];
		if (ifp == NULL)
			 ifp = dhd->iflist[0];

		ASSERT(ifp);
		skb->dev = ifp->net;
		skb->protocol = eth_type_trans(skb, skb->dev);
		skb->data = skb_data;
		skb->len = len;

		/* Strip header, count, deliver upward */
		skb_pull(skb, ETH_HLEN);

		/* Send the packet */
		if (in_interrupt()) {
			netif_rx(skb);
		} else {
			netif_rx_ni(skb);
		}
	}
	else {
		/* Could not allocate a sk_buf */
		DHD_ERROR(("%s: unable to alloc sk_buf", __FUNCTION__));
	}
}
#endif /* LOG_INTO_TCPDUMP */

void dhd_wait_for_event(dhd_pub_t *dhd, bool *lockvar)
{
#if defined(BCMSDIO) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	struct dhd_info *dhdinfo =  dhd->info;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27))
	int timeout = msecs_to_jiffies(IOCTL_RESP_TIMEOUT);
#else
	int timeout = (IOCTL_RESP_TIMEOUT / 1000) * HZ;
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)) */

	dhd_os_sdunlock(dhd);
	wait_event_timeout(dhdinfo->ctrl_wait, (*lockvar == FALSE), timeout);
	dhd_os_sdlock(dhd);
#endif /* defined(BCMSDIO) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)) */
	return;
}

void dhd_wait_event_wakeup(dhd_pub_t *dhd)
{
#if defined(BCMSDIO) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	struct dhd_info *dhdinfo =  dhd->info;
	if (waitqueue_active(&dhdinfo->ctrl_wait))
		wake_up(&dhdinfo->ctrl_wait);
#endif
	return;
}

#if defined(BCMSDIO) || defined(BCMPCIE)
int
dhd_net_bus_devreset(struct net_device *dev, uint8 flag)
{
	int ret = 0;

	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	if (flag == TRUE) {
		/* Issue wl down command before resetting the chip */
		if (dhd_wl_ioctl_cmd(&dhd->pub, WLC_DOWN, NULL, 0, TRUE, 0) < 0) {
			DHD_TRACE(("%s: wl down failed\n", __FUNCTION__));
		}
#ifdef PROP_TXSTATUS
		if (dhd->pub.wlfc_enabled) {
			dhd_wlfc_deinit(&dhd->pub);
		}
#endif /* PROP_TXSTATUS */
#ifdef PNO_SUPPORT
		if (dhd->pub.pno_state) {
			dhd_pno_deinit(&dhd->pub);
		}
#endif /* PNO_SUPPORT */
#ifdef RTT_SUPPORT
		if (dhd->pub.rtt_state) {
			dhd_rtt_deinit(&dhd->pub);
		}
#endif /* RTT_SUPPORT */
	}
#ifdef BCM_BLOCK_DATA_FRAME
	else {
		/*  reset connecting flag to avoid block data frames */
		dhd->pub.blockframe_flag = 0;
	}
#endif
#ifdef BCMSDIO
	if (!flag) {
		dhd_update_fw_nv_path(dhd);
		/* update firmware and nvram path to sdio bus */
		dhd_bus_update_fw_nv_path(dhd->pub.bus,
			dhd->fw_path, dhd->nv_path);
	}
#endif /* BCMSDIO */
	ret = dhd_bus_devreset(&dhd->pub, flag);
	if (ret) {
		DHD_ERROR(("%s: dhd_bus_devreset: %d\n", __FUNCTION__, ret));
		return ret;
	}
	return ret;
}

#ifdef BCMSDIO
int
dhd_net_bus_suspend(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	return dhd_bus_suspend(&dhd->pub);
}

int
dhd_net_bus_resume(struct net_device *dev, uint8 stage)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	return dhd_bus_resume(&dhd->pub, stage);
}

#ifdef HW_WIFI_SDIO_LOWPOWER
int
dhd_net_bus_power_up(struct net_device *dev)
{
    dhd_info_t *dhd = DHD_DEV_INFO(dev);
    return dhd_bus_power_up(&dhd->pub);
}


int
dhd_net_bus_power_down(struct net_device *dev)
{
    dhd_info_t *dhd = DHD_DEV_INFO(dev);
    return dhd_bus_power_down(&dhd->pub);
}
#endif

#endif /* BCMSDIO */
#endif /* BCMSDIO || BCMPCIE */

int net_os_set_suspend_disable(struct net_device *dev, int val)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	int ret = 0;

	if (dhd) {
		ret = dhd->pub.suspend_disable_flag;
		dhd->pub.suspend_disable_flag = val;
	}
	return ret;
}

int net_os_set_suspend(struct net_device *dev, int val, int force)
{
	int ret = 0;
	dhd_info_t *dhd = DHD_DEV_INFO(dev);

	if (dhd) {
#if defined(CONFIG_HAS_EARLYSUSPEND) && defined(DHD_USE_EARLYSUSPEND)
		ret = dhd_set_suspend(val, &dhd->pub);
#else
		ret = dhd_suspend_resume_helper(dhd, val, force);
#endif
#ifdef WL_CFG80211
		wl_cfg80211_update_power_mode(dev);
#endif
	}
	return ret;
}

int net_os_set_suspend_bcn_li_dtim(struct net_device *dev, int val)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);

	if (dhd)
		dhd->pub.suspend_bcn_li_dtim = val;

	return 0;
}

#ifdef PKT_FILTER_SUPPORT
int net_os_rxfilter_add_remove(struct net_device *dev, int add_remove, int num)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	char *filterp = NULL;
	int filter_id = 0;
	int ret = 0;

	if (!dhd || (num == DHD_UNICAST_FILTER_NUM))
		return ret;
	if (num >= dhd->pub.pktfilter_count)
		return -EINVAL;
	switch (num) {
		case DHD_BROADCAST_FILTER_NUM:
			filterp = "101 0 0 0 0xFFFFFFFFFFFF 0xFFFFFFFFFFFF";
			filter_id = 101;
			break;
		case DHD_MULTICAST4_FILTER_NUM:
			filterp = "102 0 0 0 0xFFFFFF 0x01005E";
			filter_id = 102;
			break;
		case DHD_MULTICAST6_FILTER_NUM:
#ifdef BCM_PATCH_BLOCK_IPV6_PACKET
			/* customer want to use NO IPV6 packets only */
			return ret;
#else
			filterp = "103 0 0 0 0xFFFF 0x3333";
			filter_id = 103;
			break;
#endif /* BCM_PATCH_BLOCK_IPV6_PACKET */
		case DHD_MDNS_FILTER_NUM:
			filterp = "104 0 0 0 0xFFFFFFFFFFFF 0x01005E0000FB";
			filter_id = 104;
			break;
		default:
			return -EINVAL;
	}

	/* Add filter */
	if (add_remove) {
		dhd->pub.pktfilter[num] = filterp;
		dhd_pktfilter_offload_set(&dhd->pub, dhd->pub.pktfilter[num]);
	} else { /* Delete filter */
		if (dhd->pub.pktfilter[num] != NULL) {
			dhd_pktfilter_offload_delete(&dhd->pub, filter_id);
			dhd->pub.pktfilter[num] = NULL;
		}
	}
	return ret;
}

int dhd_os_enable_packet_filter(dhd_pub_t *dhdp, int val)

{
	int ret = 0;

	/* Packet filtering is set only if we still in early-suspend and
	 * we need either to turn it ON or turn it OFF
	 * We can always turn it OFF in case of early-suspend, but we turn it
	 * back ON only if suspend_disable_flag was not set
	*/
	if (dhdp && dhdp->up) {
		if (dhdp->in_suspend) {
			if (!val || (val && !dhdp->suspend_disable_flag))
				dhd_enable_packet_filter(val, dhdp);
		}
	}
	return ret;
}

/* function to enable/disable packet for Network device */
int net_os_enable_packet_filter(struct net_device *dev, int val)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);

	return dhd_os_enable_packet_filter(&dhd->pub, val);
}
#endif /* PKT_FILTER_SUPPORT */

int
dhd_dev_init_ioctl(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	int ret;

	if ((ret = dhd_sync_with_dongle(&dhd->pub)) < 0)
		goto done;

done:
	return ret;
}
int dhd_dev_get_feature_set(struct net_device *dev)
{
	dhd_info_t *ptr = *(dhd_info_t **)netdev_priv(dev);
	dhd_pub_t *dhd = (&ptr->pub);
	int feature_set = 0;

	if (!dhd)
		return feature_set;

	if (FW_SUPPORTED(dhd, sta))
		feature_set |= WIFI_FEATURE_INFRA;
	if (FW_SUPPORTED(dhd, dualband))
		feature_set |= WIFI_FEATURE_INFRA_5G;
	if (FW_SUPPORTED(dhd, p2p))
		feature_set |= WIFI_FEATURE_P2P;
	if (dhd->op_mode & DHD_FLAG_HOSTAP_MODE)
		feature_set |= WIFI_FEATURE_SOFT_AP;
	if (FW_SUPPORTED(dhd, tdls))
		feature_set |= WIFI_FEATURE_TDLS;
	if (FW_SUPPORTED(dhd, vsdb))
		feature_set |= WIFI_FEATURE_TDLS_OFFCHANNEL;
	if (FW_SUPPORTED(dhd, nan)) {
		feature_set |= WIFI_FEATURE_NAN;
		/* NAN is essentail for d2d rtt */
#ifdef HW_WIFI_RTT_DISABLE
#else
		if (FW_SUPPORTED(dhd, rttd2d))
			feature_set |= WIFI_FEATURE_D2D_RTT;
#endif /* HW_WIFI_RTT_DISABLE */
	}
#ifdef HW_WIFI_RTT_DISABLE
#else
#ifdef RTT_SUPPORT
	feature_set |= WIFI_FEATURE_D2AP_RTT;
#endif /* RTT_SUPPORT */
#endif /* HW_WIFI_RTT_DISABLE */
#ifdef LINKSTAT_SUPPORT
	feature_set |= WIFI_FEATURE_LINKSTAT;
#endif /* LINKSTAT_SUPPORT */
	/* Supports STA + STA always */
	feature_set |= WIFI_FEATURE_ADDITIONAL_STA;
#ifdef PNO_SUPPORT
	if (dhd_is_pno_supported(dhd)) {
		feature_set |= WIFI_FEATURE_PNO;
		feature_set |= WIFI_FEATURE_BATCH_SCAN;
#ifdef GSCAN_SUPPORT
		feature_set |= WIFI_FEATURE_GSCAN;
		feature_set |= WIFI_FEATURE_HAL_EPNO;
#endif /* GSCAN_SUPPORT */
	}
	if (FW_SUPPORTED(dhd, rssi_mon)) {
		feature_set |= WIFI_FEATUE_RSSI_MONITOR;
	}
#endif /* PNO_SUPPORT */
#ifdef WL11U
	feature_set |= WIFI_FEATURE_HOTSPOT;
#endif /* WL11U */
#ifdef BCM_BSSID_BLACKLIST
feature_set |= WIFI_FEATURE_CONTROL_ROAMING;
#endif
	return feature_set;
}

int *dhd_dev_get_feature_set_matrix(struct net_device *dev, int *num)
{
	int feature_set_full, mem_needed;
	int *ret;

	*num = 0;
	mem_needed = sizeof(int) * MAX_FEATURE_SET_CONCURRRENT_GROUPS;
	ret = (int *) kmalloc(mem_needed, GFP_KERNEL);

	 if (!ret) {
		DHD_ERROR(("%s: failed to allocate %d bytes\n", __FUNCTION__,
		mem_needed));
		return ret;
	 }

	feature_set_full = dhd_dev_get_feature_set(dev);

	ret[0] = (feature_set_full & WIFI_FEATURE_INFRA) |
	         (feature_set_full & WIFI_FEATURE_INFRA_5G) |
	         (feature_set_full & WIFI_FEATURE_NAN) |
	         (feature_set_full & WIFI_FEATURE_D2D_RTT) |
	         (feature_set_full & WIFI_FEATURE_D2AP_RTT) |
	         (feature_set_full & WIFI_FEATURE_PNO) |
	         (feature_set_full & WIFI_FEATURE_HAL_EPNO) |
	         (feature_set_full & WIFI_FEATUE_RSSI_MONITOR) |
	         (feature_set_full & WIFI_FEATURE_BATCH_SCAN) |
	         (feature_set_full & WIFI_FEATURE_GSCAN) |
	         (feature_set_full & WIFI_FEATURE_HOTSPOT) |
	         (feature_set_full & WIFI_FEATURE_ADDITIONAL_STA) |
#ifdef BCM_BSSID_BLACKLIST
	         (feature_set_full & WIFI_FEATURE_EPR) |
	         (feature_set_full & WIFI_FEATURE_CONTROL_ROAMING);
#else
	         (feature_set_full & WIFI_FEATURE_EPR);
#endif

	ret[1] = (feature_set_full & WIFI_FEATURE_INFRA) |
	         (feature_set_full & WIFI_FEATURE_INFRA_5G) |
	         (feature_set_full & WIFI_FEATUE_RSSI_MONITOR) |
	         /* Not yet verified NAN with P2P */
	         /* (feature_set_full & WIFI_FEATURE_NAN) | */
	         (feature_set_full & WIFI_FEATURE_P2P) |
	         (feature_set_full & WIFI_FEATURE_D2AP_RTT) |
	         (feature_set_full & WIFI_FEATURE_D2D_RTT) |
	         (feature_set_full & WIFI_FEATURE_EPR);

	ret[2] = (feature_set_full & WIFI_FEATURE_INFRA) |
	         (feature_set_full & WIFI_FEATURE_INFRA_5G) |
	         (feature_set_full & WIFI_FEATUE_RSSI_MONITOR) |
	         (feature_set_full & WIFI_FEATURE_NAN) |
	         (feature_set_full & WIFI_FEATURE_D2D_RTT) |
	         (feature_set_full & WIFI_FEATURE_D2AP_RTT) |
	         (feature_set_full & WIFI_FEATURE_TDLS) |
	         (feature_set_full & WIFI_FEATURE_TDLS_OFFCHANNEL) |
	         (feature_set_full & WIFI_FEATURE_EPR);
	*num = MAX_FEATURE_SET_CONCURRRENT_GROUPS;

	return ret;
}

int
dhd_dev_set_nodfs(struct net_device *dev, u32 nodfs)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);

	if (nodfs)
		dhd->pub.dhd_cflags |= WLAN_PLAT_NODFS_FLAG;
	else
		dhd->pub.dhd_cflags &= ~WLAN_PLAT_NODFS_FLAG;
	dhd->pub.force_country_change = TRUE;
	return 0;
}

#ifdef PNO_SUPPORT
/* Linux wrapper to call common dhd_pno_stop_for_ssid */
int
dhd_dev_pno_stop_for_ssid(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);

	return (dhd_pno_stop_for_ssid(&dhd->pub));
}
/* Linux wrapper to call common dhd_pno_set_for_ssid */
int
dhd_dev_pno_set_for_ssid(struct net_device *dev, wlc_ssid_ext_t* ssids_local, int nssid,
	uint16  scan_fr, int pno_repeat, int pno_freq_expo_max, uint16 *channel_list, int nchan)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);

	return (dhd_pno_set_for_ssid(&dhd->pub, ssids_local, nssid, scan_fr,
		pno_repeat, pno_freq_expo_max, channel_list, nchan));
}

/* Linux wrapper to call common dhd_pno_enable */
int
dhd_dev_pno_enable(struct net_device *dev, int enable)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);

	return (dhd_pno_enable(&dhd->pub, enable));
}

/* Linux wrapper to call common dhd_pno_set_for_hotlist */
int
dhd_dev_pno_set_for_hotlist(struct net_device *dev, wl_pfn_bssid_t *p_pfn_bssid,
	struct dhd_pno_hotlist_params *hotlist_params)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	return (dhd_pno_set_for_hotlist(&dhd->pub, p_pfn_bssid, hotlist_params));
}
/* Linux wrapper to call common dhd_dev_pno_stop_for_batch */
int
dhd_dev_pno_stop_for_batch(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	return (dhd_pno_stop_for_batch(&dhd->pub));
}
/* Linux wrapper to call common dhd_dev_pno_set_for_batch */
int
dhd_dev_pno_set_for_batch(struct net_device *dev, struct dhd_pno_batch_params *batch_params)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	return (dhd_pno_set_for_batch(&dhd->pub, batch_params));
}
/* Linux wrapper to call common dhd_dev_pno_get_for_batch */
int
dhd_dev_pno_get_for_batch(struct net_device *dev, char *buf, int bufsize)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	return (dhd_pno_get_for_batch(&dhd->pub, buf, bufsize, PNO_STATUS_NORMAL));
}
#endif /* PNO_SUPPORT */

#ifdef GSCAN_SUPPORT
#ifdef BCM_PATCH_GSCAN
int
dhd_dev_set_epno(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	if (!dhd) {
		return BCME_ERROR;
	}
	return dhd_pno_set_epno(&dhd->pub);
}

int
dhd_dev_flush_fw_epno(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	if (!dhd) {
		return BCME_ERROR;
	}
	return dhd_pno_flush_fw_epno(&dhd->pub);
}
#endif

/* Linux wrapper to call common dhd_pno_set_cfg_gscan */
int
dhd_dev_pno_set_cfg_gscan(struct net_device *dev, dhd_pno_gscan_cmd_cfg_t type,
#ifdef BCM_PATCH_GSCAN
 void *buf, bool flush)
#else
 void *buf, uint8 flush)
#endif
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_pno_set_cfg_gscan(&dhd->pub, type, buf, flush));
}

/* Linux wrapper to call common dhd_pno_get_gscan */
void *
dhd_dev_pno_get_gscan(struct net_device *dev, dhd_pno_gscan_cmd_cfg_t type,
                      void *info, uint32 *len)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_pno_get_gscan(&dhd->pub, type, info, len));
}

/* Linux wrapper to call common dhd_wait_batch_results_complete */
int dhd_dev_wait_batch_results_complete(struct net_device *dev)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_wait_batch_results_complete(&dhd->pub));
}

/* Linux wrapper to call common dhd_pno_lock_batch_results */
int
dhd_dev_pno_lock_access_batch_results(struct net_device *dev)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_pno_lock_batch_results(&dhd->pub));
}
/* Linux wrapper to call common dhd_pno_unlock_batch_results */
void
dhd_dev_pno_unlock_access_batch_results(struct net_device *dev)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_pno_unlock_batch_results(&dhd->pub));
}

/* Linux wrapper to call common dhd_pno_initiate_gscan_request */
int dhd_dev_pno_run_gscan(struct net_device *dev, bool run, bool flush)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_pno_initiate_gscan_request(&dhd->pub, run, flush));
}

/* Linux wrapper to call common dhd_pno_enable_full_scan_result */
int dhd_dev_pno_enable_full_scan_result(struct net_device *dev, bool real_time_flag)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_pno_enable_full_scan_result(&dhd->pub, real_time_flag));
}
#ifndef BCM_PATCH_SECURITY_2017_07
/* Linux wrapper to call common dhd_handle_swc_evt */
void * dhd_dev_swc_scan_event(struct net_device *dev, const void  *data, int *send_evt_bytes)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_handle_swc_evt(&dhd->pub, data, send_evt_bytes));
}
#endif
/* Linux wrapper to call common dhd_handle_hotlist_scan_evt */
void * dhd_dev_hotlist_scan_event(struct net_device *dev,
      const void  *data, int *send_evt_bytes, hotlist_type_t type)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_handle_hotlist_scan_evt(&dhd->pub, data, send_evt_bytes, type));
}
#ifndef BCM_PATCH_SECURITY_2017_07
/* Linux wrapper to call common dhd_process_full_gscan_result */
void * dhd_dev_process_full_gscan_result(struct net_device *dev,
const void  *data, int *send_evt_bytes)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_process_full_gscan_result(&dhd->pub, data, send_evt_bytes));
}
#else
void * dhd_dev_process_full_gscan_result(struct net_device *dev,
const void  *data, uint32 len, int *send_evt_bytes)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_process_full_gscan_result(&dhd->pub, data, len, send_evt_bytes));
}
#endif
void dhd_dev_gscan_hotlist_cache_cleanup(struct net_device *dev, hotlist_type_t type)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	dhd_gscan_hotlist_cache_cleanup(&dhd->pub, type);

	return;
}

int dhd_dev_gscan_batch_cache_cleanup(struct net_device *dev)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_gscan_batch_cache_cleanup(&dhd->pub));
}

/* Linux wrapper to call common dhd_retreive_batch_scan_results */
int dhd_dev_retrieve_batch_scan(struct net_device *dev)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_retreive_batch_scan_results(&dhd->pub));
}
/* Linux wrapper to call common dhd_pno_process_epno_result */
void * dhd_dev_process_epno_result(struct net_device *dev,
			const void  *data, uint32 event, int *send_evt_bytes)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_pno_process_epno_result(&dhd->pub, data, event, send_evt_bytes));
}

int dhd_dev_set_lazy_roam_cfg(struct net_device *dev,
             wlc_roam_exp_params_t *roam_param)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);
	wl_roam_exp_cfg_t roam_exp_cfg;
	int err;

	if (!roam_param) {
		return BCME_BADARG;
	}

	DHD_ERROR(("a_band_boost_thr %d a_band_penalty_thr %d\n",
	      roam_param->a_band_boost_threshold, roam_param->a_band_penalty_threshold));
	DHD_ERROR(("a_band_boost_factor %d a_band_penalty_factor %d cur_bssid_boost %d\n",
	      roam_param->a_band_boost_factor, roam_param->a_band_penalty_factor,
	      roam_param->cur_bssid_boost));
	DHD_ERROR(("alert_roam_trigger_thr %d a_band_max_boost %d\n",
	      roam_param->alert_roam_trigger_threshold, roam_param->a_band_max_boost));

	memcpy(&roam_exp_cfg.params, roam_param, sizeof(*roam_param));
	roam_exp_cfg.version = ROAM_EXP_CFG_VERSION;
	roam_exp_cfg.flags = ROAM_EXP_CFG_PRESENT;
	if (dhd->pub.lazy_roam_enable) {
		roam_exp_cfg.flags |= ROAM_EXP_ENABLE_FLAG;
	}
	err = dhd_iovar(&dhd->pub, 0, "roam_exp_params",
			(char *)&roam_exp_cfg, sizeof(roam_exp_cfg), NULL, 0,
			TRUE);
	if (err < 0) {
		DHD_ERROR(("%s : Failed to execute roam_exp_params %d\n", __FUNCTION__, err));
	}
	return err;
}

int dhd_dev_lazy_roam_enable(struct net_device *dev, uint32 enable)
{
	int err;
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);
	wl_roam_exp_cfg_t roam_exp_cfg;

	memset(&roam_exp_cfg, 0, sizeof(roam_exp_cfg));
	roam_exp_cfg.version = ROAM_EXP_CFG_VERSION;
	if (enable) {
		roam_exp_cfg.flags = ROAM_EXP_ENABLE_FLAG;
	}

	err = dhd_iovar(&dhd->pub, 0, "roam_exp_params",
			(char *)&roam_exp_cfg, sizeof(roam_exp_cfg), NULL, 0,
			TRUE);
	if (err < 0) {
		DHD_ERROR(("%s : Failed to execute roam_exp_params %d\n", __FUNCTION__, err));
	} else {
		dhd->pub.lazy_roam_enable = (enable != 0);
	}
	return err;
}
int dhd_dev_set_lazy_roam_bssid_pref(struct net_device *dev,
       wl_bssid_pref_cfg_t *bssid_pref, uint32 flush)
{
	int err;
	int len;
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	bssid_pref->version = BSSID_PREF_LIST_VERSION;
	/* By default programming bssid pref flushes out old values */
	bssid_pref->flags = (flush && !bssid_pref->count) ? ROAM_EXP_CLEAR_BSSID_PREF: 0;
	len = sizeof(wl_bssid_pref_cfg_t);
	len += (bssid_pref->count - 1) * sizeof(wl_bssid_pref_list_t);
	err = dhd_iovar(&dhd->pub, 0, "roam_exp_bssid_pref",
			(char *)bssid_pref, len, NULL, 0, TRUE);
	if (err != BCME_OK) {
		DHD_ERROR(("%s : Failed to execute roam_exp_bssid_pref %d\n", __FUNCTION__, err));
	}
	return err;
}
int dhd_dev_set_blacklist_bssid(struct net_device *dev, maclist_t *blacklist,
    uint32 len, uint32 flush)
{
	int err;
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);
	int macmode;

	if (blacklist) {
		err = dhd_wl_ioctl_cmd(&(dhd->pub), WLC_SET_MACLIST, (char *)blacklist,
						 len, TRUE, 0);
		if (err != BCME_OK) {
			DHD_ERROR(("%s : WLC_SET_MACLIST failed %d\n", __FUNCTION__, err));
			return err;
		}
	}
	/* By default programming blacklist flushes out old values */
	macmode = (flush && !blacklist) ? WLC_MACMODE_DISABLED : WLC_MACMODE_DENY;
	err = dhd_wl_ioctl_cmd(&(dhd->pub), WLC_SET_MACMODE, (char *)&macmode,
	              sizeof(macmode), TRUE, 0);
	if (err != BCME_OK) {
		DHD_ERROR(("%s : WLC_SET_MACMODE failed %d\n", __FUNCTION__, err));
	}
	return err;
}
int dhd_dev_set_whitelist_ssid(struct net_device *dev, wl_ssid_whitelist_t *ssid_whitelist,
    uint32 len, uint32 flush)
{
	int err;
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);
	wl_ssid_whitelist_t whitelist_ssid_flush;

	if (!ssid_whitelist) {
		if (flush) {
			ssid_whitelist = &whitelist_ssid_flush;
			ssid_whitelist->ssid_count = 0;
		} else {
			DHD_ERROR(("%s : Nothing to do here\n", __FUNCTION__));
			return BCME_BADARG;
		}
	}
	ssid_whitelist->version = SSID_WHITELIST_VERSION;
	ssid_whitelist->flags = flush ? ROAM_EXP_CLEAR_SSID_WHITELIST : 0;
	err = dhd_iovar(&dhd->pub, 0, "roam_exp_ssid_whitelist",
			(char *)ssid_whitelist, len, NULL, 0, TRUE);
	if (err != BCME_OK) {
		DHD_ERROR(("%s : Failed to execute roam_exp_bssid_pref %d\n", __FUNCTION__, err));
	}
	return err;
}

void * dhd_dev_process_anqpo_result(struct net_device *dev,
			const void  *data, uint32 event, int *send_evt_bytes)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_pno_process_anqpo_result(&dhd->pub, data, event, send_evt_bytes));
}
#endif /* GSCAN_SUPPORT */

int dhd_dev_set_rssi_monitor_cfg(struct net_device *dev, int start,
             int8 max_rssi, int8 min_rssi)
{
	int err;
	wl_rssi_monitor_cfg_t rssi_monitor;
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	rssi_monitor.version = RSSI_MONITOR_VERSION;
	rssi_monitor.max_rssi = max_rssi;
	rssi_monitor.min_rssi = min_rssi;
	rssi_monitor.flags = start ? 0: RSSI_MONITOR_STOP;
	err = dhd_iovar(&dhd->pub, 0, "rssi_monitor", (char *)&rssi_monitor,
			sizeof(rssi_monitor), NULL, 0, TRUE);
	if (err != BCME_OK) {
		DHD_ERROR(("%s : Failed to execute rssi_monitor %d\n", __FUNCTION__, err));
	}
	return err;
}

bool dhd_dev_is_legacy_pno_enabled(struct net_device *dev)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_is_legacy_pno_enabled(&dhd->pub));
}

int dhd_dev_cfg_rand_mac_oui(struct net_device *dev, uint8 *oui)
{
#ifdef BCM_SUPPORT_RAND_MAC
	int err;
	wl_pfn_macaddr_cfg_t cfg;
#endif
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);
	dhd_pub_t *dhdp = &dhd->pub;

	if (!dhdp || !oui) {
		DHD_ERROR(("NULL POINTER : %s\n",
			__FUNCTION__));
		return BCME_ERROR;
	}
	if (ETHER_ISMULTI(oui)) {
		DHD_ERROR(("Expected unicast OUI\n"));
		return BCME_ERROR;
	} else {
		uint8 *rand_mac_oui = dhdp->rand_mac_oui;
		memcpy(rand_mac_oui, oui, DOT11_OUI_LEN);
		DHD_ERROR(("Random MAC OUI to be used - %02x:%02x:%02x\n", rand_mac_oui[0],
			rand_mac_oui[1], rand_mac_oui[2]));
#ifdef BCM_SUPPORT_RAND_MAC
		/*
		 * rand_mac_oui is used for PNO scan, do not use random mac scan if it is all zero
		 * cfg.macaddr is used for normal scan in disconnected state
		 */
		memset(&cfg.macaddr, 0, ETHER_ADDR_LEN);
		memcpy(&cfg.macaddr, oui, DOT11_OUI_LEN);
		cfg.version = WL_PFN_MACADDR_CFG_VER;
		if (ETHER_ISNULLADDR(&cfg.macaddr))
			cfg.flags = 0;
		else
			cfg.flags = (WL_PFN_MAC_OUI_ONLY_MASK | WL_PFN_SET_MAC_UNASSOC_MASK);

		err = dhd_iovar(dhdp, 0, "scan_macaddr", (char *)&cfg, sizeof(cfg), NULL, 0, TRUE);
		if (err < 0) {
			DHD_ERROR(("%s : failed to execute scan_macaddr %d\n", __FUNCTION__, err));
		}
#endif
	}
	return BCME_OK;
}

int dhd_set_rand_mac_oui(dhd_pub_t *dhd)
{
	int err;
	wl_pfn_macaddr_cfg_t cfg;
	uint8 *rand_mac_oui = dhd->rand_mac_oui;

	memset(&cfg.macaddr, 0, ETHER_ADDR_LEN);
	memcpy(&cfg.macaddr, rand_mac_oui, DOT11_OUI_LEN);
	cfg.version = WL_PFN_MACADDR_CFG_VER;
	if (ETHER_ISNULLADDR(&cfg.macaddr))
		cfg.flags = 0;
	else
		cfg.flags = (WL_PFN_MAC_OUI_ONLY_MASK | WL_PFN_SET_MAC_UNASSOC_MASK);

	DHD_ERROR(("Setting rand mac oui to FW - %02x:%02x:%02x\n", rand_mac_oui[0],
	    rand_mac_oui[1], rand_mac_oui[2]));

	err = dhd_iovar(dhd, 0, "pfn_macaddr", (char *)&cfg, sizeof(cfg), NULL,
			0, TRUE);
	if (err < 0) {
		DHD_ERROR(("%s : failed to execute pfn_macaddr %d\n", __FUNCTION__, err));
	}
	return err;
}

#ifdef RTT_SUPPORT
/* Linux wrapper to call common dhd_pno_set_cfg_gscan */
int
dhd_dev_rtt_set_cfg(struct net_device *dev, void *buf)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_rtt_set_cfg(&dhd->pub, buf));
}
int
dhd_dev_rtt_cancel_cfg(struct net_device *dev, struct ether_addr *mac_list, int mac_cnt)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_rtt_stop(&dhd->pub, mac_list, mac_cnt));
}

int
dhd_dev_rtt_register_noti_callback(struct net_device *dev, void *ctx, dhd_rtt_compl_noti_fn noti_fn)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_rtt_register_noti_callback(&dhd->pub, ctx, noti_fn));
}
int
dhd_dev_rtt_unregister_noti_callback(struct net_device *dev, dhd_rtt_compl_noti_fn noti_fn)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_rtt_unregister_noti_callback(&dhd->pub, noti_fn));
}

int
dhd_dev_rtt_capability(struct net_device *dev, rtt_capabilities_t *capa)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

	return (dhd_rtt_capability(&dhd->pub, capa));
}
#endif /* RTT_SUPPORT */

#if defined(KEEP_ALIVE)
#define TEMP_BUF_SIZE 512
#define FRAME_SIZE 300
int
dhd_dev_start_mkeep_alive(dhd_pub_t *dhd_pub, u8 mkeep_alive_id, u8 *ip_pkt, u16 ip_pkt_len,
	u8* src_mac, u8* dst_mac, u32 period_msec)
{
	const int 		ETHERTYPE_LEN = 2;
	char			*pbuf;
	const char		*str;
	wl_mkeep_alive_pkt_t mkeep_alive_pkt = {0};
	wl_mkeep_alive_pkt_t *mkeep_alive_pktp;
	int				buf_len;
	int				str_len;
	int 			res = BCME_ERROR;
	int 			len_bytes = 0;
	int 			i;

	/* ether frame to have both max IP pkt (256 bytes) and ether header */
	char 			*pmac_frame = NULL;
	char 			*pmac_frame_begin = NULL;

	/*
	 * The mkeep_alive packet is for STA interface only; if the bss is configured as AP,
	 * dongle shall reject a mkeep_alive request.
	 */
	if (!dhd_support_sta_mode(dhd_pub))
		return res;

	DHD_TRACE(("%s execution\n", __FUNCTION__));

	if ((pbuf = kzalloc(TEMP_BUF_SIZE, GFP_KERNEL)) == NULL) {
		DHD_ERROR(("failed to allocate buf with size %d\n", TEMP_BUF_SIZE));
		res = BCME_NOMEM;
		return res;
	}

	if ((pmac_frame = kzalloc(FRAME_SIZE, GFP_KERNEL)) == NULL) {
		DHD_ERROR(("failed to allocate mac_frame with size %d\n", FRAME_SIZE));
		res = BCME_NOMEM;
		goto exit;
	}
	pmac_frame_begin = pmac_frame;

	/*
	 * Get current mkeep-alive status.
	 */
	res = dhd_iovar(dhd_pub, 0, "mkeep_alive", &mkeep_alive_id,
			sizeof(mkeep_alive_id), pbuf, TEMP_BUF_SIZE, FALSE);
	if (res < 0) {
		DHD_ERROR(("%s: Get mkeep_alive failed (error=%d)\n", __FUNCTION__, res));
		goto exit;
	} else {
		/* Check available ID whether it is occupied */
		mkeep_alive_pktp = (wl_mkeep_alive_pkt_t *) pbuf;
		if (dtoh32(mkeep_alive_pktp->period_msec != 0)) {
			DHD_ERROR(("%s: Get mkeep_alive failed, ID %u is in use.\n",
				__FUNCTION__, mkeep_alive_id));

			/* Current occupied ID info */
			DHD_ERROR(("%s: mkeep_alive\n", __FUNCTION__));
			DHD_ERROR(("   Id    : %d\n"
				   "   Period: %d msec\n"
				   "   Length: %d\n"
				   "   Packet: 0x",
				mkeep_alive_pktp->keep_alive_id,
				dtoh32(mkeep_alive_pktp->period_msec),
				dtoh16(mkeep_alive_pktp->len_bytes)));

			for (i = 0; i < mkeep_alive_pktp->len_bytes; i++) {
				DHD_ERROR(("%02x", mkeep_alive_pktp->data[i]));
			}
			DHD_ERROR(("\n"));

			res = BCME_NOTFOUND;
			goto exit;
		}
	}

	/* Request the specified ID */
	memset(&mkeep_alive_pkt, 0, sizeof(wl_mkeep_alive_pkt_t));
	memset(pbuf, 0, TEMP_BUF_SIZE);
	str = "mkeep_alive";
	str_len = strlen(str);
	strncpy(pbuf, str, str_len);
	pbuf[str_len] = '\0';

	mkeep_alive_pktp = (wl_mkeep_alive_pkt_t *) (pbuf + str_len + 1);
	mkeep_alive_pkt.period_msec = htod32(period_msec);
	buf_len = str_len + 1;
	mkeep_alive_pkt.version = htod16(WL_MKEEP_ALIVE_VERSION);
	mkeep_alive_pkt.length = htod16(WL_MKEEP_ALIVE_FIXED_LEN);

	/* ID assigned */
	mkeep_alive_pkt.keep_alive_id = mkeep_alive_id;

	buf_len += WL_MKEEP_ALIVE_FIXED_LEN;

	/*
	 * Build up Ethernet Frame
	 */

	/* Mapping dest mac addr */
	memcpy(pmac_frame, dst_mac, ETHER_ADDR_LEN);
	pmac_frame += ETHER_ADDR_LEN;

	/* Mapping src mac addr */
	memcpy(pmac_frame, src_mac, ETHER_ADDR_LEN);
	pmac_frame += ETHER_ADDR_LEN;

	/* Mapping Ethernet type (ETHERTYPE_IP: 0x0800) */
	*(pmac_frame++) = 0x08;
	*(pmac_frame++) = 0x00;

	/* Mapping IP pkt */
	memcpy(pmac_frame, ip_pkt, ip_pkt_len);
	pmac_frame += ip_pkt_len;

	/*
	 * Length of ether frame (assume to be all hexa bytes)
	 *     = src mac + dst mac + ether type + ip pkt len
	 */
	len_bytes = ETHER_ADDR_LEN*2 + ETHERTYPE_LEN + ip_pkt_len;
	memcpy(mkeep_alive_pktp->data, pmac_frame_begin, len_bytes);
	buf_len += len_bytes;
	mkeep_alive_pkt.len_bytes = htod16(len_bytes);

	/*
	 * Keep-alive attributes are set in local variable (mkeep_alive_pkt), and
	 * then memcpy'ed into buffer (mkeep_alive_pktp) since there is no
	 * guarantee that the buffer is properly aligned.
	 */
	memcpy((char *)mkeep_alive_pktp, &mkeep_alive_pkt, WL_MKEEP_ALIVE_FIXED_LEN);

	res = dhd_wl_ioctl_cmd(dhd_pub, WLC_SET_VAR, pbuf, buf_len, TRUE, 0);
exit:
	kfree(pmac_frame_begin);
	kfree(pbuf);
	return res;
}

int
dhd_dev_stop_mkeep_alive(dhd_pub_t *dhd_pub, u8 mkeep_alive_id)
{
	char			*pbuf;
	wl_mkeep_alive_pkt_t	mkeep_alive_pkt;
	wl_mkeep_alive_pkt_t	*mkeep_alive_pktp;
	int			res = BCME_ERROR;
	int 			i;

	/*
	 * The mkeep_alive packet is for STA interface only; if the bss is configured as AP,
	 * dongle shall reject a mkeep_alive request.
	 */
	if (!dhd_support_sta_mode(dhd_pub))
		return res;

	DHD_TRACE(("%s execution\n", __FUNCTION__));

	/*
	 * Get current mkeep-alive status. Skip ID 0 which is being used for NULL pkt.
	 */
	if ((pbuf = kzalloc(TEMP_BUF_SIZE, GFP_KERNEL)) == NULL) {
		DHD_ERROR(("failed to allocate buf with size %d\n", TEMP_BUF_SIZE));
		return res;
	}

	res = dhd_iovar(dhd_pub, 0, "mkeep_alive", &mkeep_alive_id,
			sizeof(mkeep_alive_id), pbuf, TEMP_BUF_SIZE, FALSE);
	if (res < 0) {
		DHD_ERROR(("%s: Get mkeep_alive failed (error=%d)\n", __FUNCTION__, res));
		goto exit;
	} else {
		/* Check occupied ID */
		mkeep_alive_pktp = (wl_mkeep_alive_pkt_t *) pbuf;
		DHD_INFO(("%s: mkeep_alive\n", __FUNCTION__));
		DHD_INFO(("   Id    : %d\n"
			  "   Period: %d msec\n"
			  "   Length: %d\n"
			  "   Packet: 0x",
			mkeep_alive_pktp->keep_alive_id,
			dtoh32(mkeep_alive_pktp->period_msec),
			dtoh16(mkeep_alive_pktp->len_bytes)));

		for (i = 0; i < mkeep_alive_pktp->len_bytes; i++) {
			DHD_INFO(("%02x", mkeep_alive_pktp->data[i]));
		}
		DHD_INFO(("\n"));
	}

	/* Make it stop if available */
	if (dtoh32(mkeep_alive_pktp->period_msec != 0)) {
		DHD_INFO(("stop mkeep_alive on ID %d\n", mkeep_alive_id));
		memset(&mkeep_alive_pkt, 0, sizeof(wl_mkeep_alive_pkt_t));

		mkeep_alive_pkt.period_msec = 0;
		mkeep_alive_pkt.version = htod16(WL_MKEEP_ALIVE_VERSION);
		mkeep_alive_pkt.length = htod16(WL_MKEEP_ALIVE_FIXED_LEN);
		mkeep_alive_pkt.keep_alive_id = mkeep_alive_id;

		res = dhd_iovar(dhd_pub, 0, "mkeep_alive",
				(char *)&mkeep_alive_pkt,
				WL_MKEEP_ALIVE_FIXED_LEN, NULL, 0, TRUE);
	} else {
		DHD_ERROR(("%s: ID %u does not exist.\n", __FUNCTION__, mkeep_alive_id));
		res = BCME_NOTFOUND;
	}
exit:
	kfree(pbuf);
	return res;
}
#endif /* defined(KEEP_ALIVE) */

#if defined(PKT_FILTER_SUPPORT) && defined(BCM_APF)
static void _dhd_apf_lock_local(dhd_info_t *dhd)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25))
	if (dhd) {
		mutex_lock(&dhd->dhd_apf_mutex);
	}
#endif
}

static void _dhd_apf_unlock_local(dhd_info_t *dhd)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25))
	if (dhd) {
		mutex_unlock(&dhd->dhd_apf_mutex);
	}
#endif
}

static int
__dhd_apf_add_filter(struct net_device *ndev, uint32 filter_id,
	u8* program, uint32 program_len)
{
	dhd_info_t *dhd = DHD_DEV_INFO(ndev);
	dhd_pub_t *dhdp = &dhd->pub;
	wl_pkt_filter_t * pkt_filterp;
	wl_apf_program_t *apf_program;
	char *buf;
	u32 cmd_len, buf_len;
	int ifidx, ret;
	gfp_t kflags;
	char cmd[] = "pkt_filter_add";

	ifidx = dhd_net2idx(dhd, ndev);
	if (ifidx == DHD_BAD_IF) {
		DHD_ERROR(("%s: bad ifidx\n", __FUNCTION__));
		return -ENODEV;
	}

	cmd_len = sizeof(cmd);
#ifdef BCM_PATCH_CVE_2016_8455
	/* check if the program_len is more than the expected len */
	if ((program_len > WL_APF_PROGRAM_MAX_SIZE) ||
		(program == NULL)){
		DHD_ERROR(("%s Invalid program_len: %d, program: %pK\n", __func__, program_len, program));
		return -EINVAL;
	}
#endif
	buf_len = cmd_len + WL_PKT_FILTER_FIXED_LEN +
		WL_APF_PROGRAM_FIXED_LEN + program_len;

	kflags = in_atomic() ? GFP_ATOMIC : GFP_KERNEL;
	buf = kzalloc(buf_len, kflags);
	if (unlikely(!buf)) {
		DHD_ERROR(("%s: MALLOC failure, %d bytes\n", __FUNCTION__, buf_len));
		return -ENOMEM;
	}

	memcpy(buf, cmd, cmd_len);

	pkt_filterp = (wl_pkt_filter_t *) (buf + cmd_len);
	pkt_filterp->id = htod32(filter_id);
	pkt_filterp->negate_match = htod32(FALSE);
	pkt_filterp->type = htod32(WL_PKT_FILTER_TYPE_APF_MATCH);

	apf_program = &pkt_filterp->u.apf_program;
	apf_program->version = htod16(WL_APF_INTERNAL_VERSION);
	apf_program->instr_len = htod16(program_len);
	memcpy(apf_program->instrs, program, program_len);

	ret = dhd_wl_ioctl_cmd(dhdp, WLC_SET_VAR, buf, buf_len, TRUE, ifidx);
	if (unlikely(ret)) {
		DHD_ERROR(("%s: failed to add APF filter, id=%d, ret=%d\n",
			__FUNCTION__, filter_id, ret));
	}

	if (buf) {
		kfree(buf);
	}
	return ret;
}

static int
__dhd_apf_config_filter(struct net_device *ndev, uint32 filter_id,
	uint32 mode, uint32 enable)
{
	dhd_info_t *dhd = DHD_DEV_INFO(ndev);
	dhd_pub_t *dhdp = &dhd->pub;
	wl_pkt_filter_enable_t * pkt_filterp;
	char *buf;
	u32 cmd_len, buf_len;
	int ifidx, ret;
	gfp_t kflags;
	char cmd[] = "pkt_filter_enable";
#ifdef HW_SHARE_WIFI_FILTER_MANAGE
	if (true == g_force_stop_filter) {
		DHD_ERROR(("Cannot enable apf because filter is force to stop."));
		return 0;
	}
#endif
	ifidx = dhd_net2idx(dhd, ndev);
	if (ifidx == DHD_BAD_IF) {
		DHD_ERROR(("%s: bad ifidx\n", __FUNCTION__));
		return -ENODEV;
	}

	cmd_len = sizeof(cmd);
	buf_len = cmd_len + sizeof(*pkt_filterp);

	kflags = in_atomic() ? GFP_ATOMIC : GFP_KERNEL;
	buf = kzalloc(buf_len, kflags);
	if (unlikely(!buf)) {
		DHD_ERROR(("%s: MALLOC failure, %d bytes\n", __FUNCTION__, buf_len));
		return -ENOMEM;
	}

	memcpy(buf, cmd, cmd_len);

	pkt_filterp = (wl_pkt_filter_enable_t *) (buf + cmd_len);
	pkt_filterp->id = htod32(filter_id);
	pkt_filterp->enable = htod32(enable);

	ret = dhd_wl_ioctl_cmd(dhdp, WLC_SET_VAR, buf, buf_len, TRUE, ifidx);
	if (unlikely(ret)) {
		DHD_ERROR(("%s: failed to enable APF filter, id=%d, ret=%d\n",
			__FUNCTION__, filter_id, ret));
		goto exit;
	}

	ret = dhd_wl_ioctl_set_intiovar(dhdp, "pkt_filter_mode", htod32(mode),
		WLC_SET_VAR, TRUE, ifidx);
	if (unlikely(ret)) {
		DHD_ERROR(("%s: failed to set APF filter mode, id=%d, ret=%d\n",
			__FUNCTION__, filter_id, ret));
	}

exit:
	if (buf) {
		kfree(buf);
	}
	return ret;
}

static int
__dhd_apf_delete_filter(struct net_device *ndev, uint32 filter_id)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(ndev);
	dhd_pub_t *dhdp = &dhd->pub;
	int ifidx, ret;

	ifidx = dhd_net2idx(dhd, ndev);
	if (ifidx == DHD_BAD_IF) {
		DHD_ERROR(("%s: bad ifidx\n", __FUNCTION__));
		return -ENODEV;
	}

	ret = dhd_wl_ioctl_set_intiovar(dhdp, "pkt_filter_delete",
		htod32(filter_id), WLC_SET_VAR, TRUE, ifidx);
	if (unlikely(ret)) {
		DHD_ERROR(("%s: failed to delete APF filter, id=%d, ret=%d\n",
			__FUNCTION__, filter_id, ret));
	}

	return ret;
}

void dhd_apf_lock(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	_dhd_apf_lock_local(dhd);
}

void dhd_apf_unlock(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	_dhd_apf_unlock_local(dhd);
}

void
dhd_dev_get_drop_pkt(dhd_pub_t *dhd, uint32 enable)
{
	int ret,i;
	uint32 filter_id = PKT_FILTER_APF_ID;
	char smbuf[WLC_IOCTL_SMLEN]={0};
	wl_pkt_filter_stats_t *filter_stat;

	bcm_mkiovar("pkt_filter_stats", (char *)&(filter_id), 4, smbuf, WLC_IOCTL_SMLEN);
	ret = dhd_wl_ioctl_cmd(dhd, WLC_GET_VAR, smbuf, WLC_IOCTL_SMLEN, FALSE, 0);
	if (ret < 0) {
		DHD_ERROR(("%s: failed to get apf filter stats ret=%d\n", __FUNCTION__,ret));
		return ;
	}

	filter_stat = (wl_pkt_filter_stats_t *)smbuf;
	DHD_ERROR(("scr on: %d,forwarded:%d,discarded:%d\n", enable,filter_stat->num_pkts_forwarded,filter_stat->num_pkts_discarded));

}

int
dhd_dev_apf_get_version(struct net_device *ndev, uint32 *version)
{
	dhd_info_t *dhd = DHD_DEV_INFO(ndev);
	dhd_pub_t *dhdp = &dhd->pub;
	int ifidx, ret;

	if (!FW_SUPPORTED(dhdp, apf)) {
		DHD_ERROR(("%s: firmware doesn't support APF\n", __FUNCTION__));

		/*
		 * Notify Android framework that APF is not supported by setting
		 * version as zero.
		 */
		*version = 0;
		return BCME_OK;
	}

	ifidx = dhd_net2idx(dhd, ndev);
	if (ifidx == DHD_BAD_IF) {
		DHD_ERROR(("%s: bad ifidx\n", __FUNCTION__));
		return -ENODEV;
	}

	ret = dhd_wl_ioctl_get_intiovar(dhdp, "apf_ver", version,
		WLC_GET_VAR, FALSE, ifidx);
	if (unlikely(ret)) {
		DHD_ERROR(("%s: failed to get APF version, ret=%d\n",
			__FUNCTION__, ret));
	}

	return ret;
}

int
dhd_dev_apf_get_max_len(struct net_device *ndev, uint32 *max_len)
{
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(ndev);
	dhd_pub_t *dhdp = &dhd->pub;
	int ifidx, ret;

	if (!FW_SUPPORTED(dhdp, apf)) {
		DHD_ERROR(("%s: firmware doesn't support APF\n", __FUNCTION__));
		*max_len = 0;
		return BCME_OK;
	}

	ifidx = dhd_net2idx(dhd, ndev);
	if (ifidx == DHD_BAD_IF) {
		DHD_ERROR(("%s bad ifidx\n", __FUNCTION__));
		return -ENODEV;
	}

	ret = dhd_wl_ioctl_get_intiovar(dhdp, "apf_size_limit", max_len,
		WLC_GET_VAR, FALSE, ifidx);
	if (unlikely(ret)) {
		DHD_ERROR(("%s: failed to get APF size limit, ret=%d\n",
			__FUNCTION__, ret));
	}

	return ret;
}

int
dhd_dev_apf_add_filter(struct net_device *ndev, u8* program,
	uint32 program_len)
{
	dhd_info_t *dhd = DHD_DEV_INFO(ndev);
	dhd_pub_t *dhdp = &dhd->pub;
	int ret;

	DHD_APF_LOCK(ndev);

	/* delete, if filter already exists */
	if (dhdp->apf_set) {
		ret = __dhd_apf_delete_filter(ndev, PKT_FILTER_APF_ID);
		if (unlikely(ret)) {
			goto exit;
		}
		dhdp->apf_set = FALSE;
	}

	ret = __dhd_apf_add_filter(ndev, PKT_FILTER_APF_ID, program, program_len);
	if (ret) {
		goto exit;
	}
	dhdp->apf_set = TRUE;

	if (dhdp->in_suspend && dhdp->apf_set) {
		/* Driver is still in (early) suspend state, enable APF filter back */
		ret = __dhd_apf_config_filter(ndev, PKT_FILTER_APF_ID,
			PKT_FILTER_MODE_FORWARD_ON_MATCH, TRUE);
	}
exit:
	DHD_APF_UNLOCK(ndev);

	return ret;
}

int
dhd_dev_apf_enable_filter(struct net_device *ndev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(ndev);
	dhd_pub_t *dhdp = &dhd->pub;
	int ret = 0;

	DHD_APF_LOCK(ndev);

	if (dhdp->apf_set) {
		ret = __dhd_apf_config_filter(ndev, PKT_FILTER_APF_ID,
			PKT_FILTER_MODE_FORWARD_ON_MATCH, TRUE);
	}

	DHD_APF_UNLOCK(ndev);

	return ret;
}

int
dhd_dev_apf_disable_filter(struct net_device *ndev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(ndev);
	dhd_pub_t *dhdp = &dhd->pub;
	int ret = 0;

	DHD_APF_LOCK(ndev);

	if (dhdp->apf_set) {
		ret = __dhd_apf_config_filter(ndev, PKT_FILTER_APF_ID,
			PKT_FILTER_MODE_FORWARD_ON_MATCH, FALSE);
	}

	DHD_APF_UNLOCK(ndev);

	return ret;
}

int
dhd_dev_apf_delete_filter(struct net_device *ndev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(ndev);
	dhd_pub_t *dhdp = &dhd->pub;
	int ret = 0;

	DHD_APF_LOCK(ndev);

	if (dhdp->apf_set) {
		ret = __dhd_apf_delete_filter(ndev, PKT_FILTER_APF_ID);
		if (!ret) {
			dhdp->apf_set = FALSE;
		}
	}

	DHD_APF_UNLOCK(ndev);

	return ret;
}
#endif /* PKT_FILTER_SUPPORT && BCM_APF */
#ifdef HW_PCIE_STABILITY
enum {
	DEVICE_LINK_MIN = 0,
	DEVICE_LINK_UP = 1,
	DEVICE_LINK_ABNORMAL = 2,
	DEVICE_LINK_MAX= 3,
};
extern int pcie_ep_link_ltssm_notify(u32 rc_id, u32 link_status);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27))
static void dhd_hang_process(void *dhd_info, void *event_info, u8 event)
{
	dhd_info_t *dhd;
	struct net_device *dev;

	dhd = (dhd_info_t *)dhd_info;
	dev = dhd->iflist[0]->net;

	if (dev) {
#ifdef HW_PCIE_STABILITY
		smp_mb();
		if (!dhd->pub.hang_report)
			return;
		int ret = 0;
		ret = pcie_ep_link_ltssm_notify(0, DEVICE_LINK_ABNORMAL);
		DHD_ERROR(("%s: notify pcie host wifi chip crash, ret %d\n", __FUNCTION__, ret));
#endif
#ifndef NO_AUTO_RECOVERY
		rtnl_lock();
		dev_close(dev);
		rtnl_unlock();
#endif

#if defined(WL_CFG80211)
		wl_cfg80211_hang(dev, WLAN_REASON_UNSPECIFIED);
#endif
#ifdef CONFIG_HW_WLANFTY_STATUS
		set_wlanfty_status(WLANFTY_STATUS_RECOVERY);
#endif
	}
}
#ifdef	HW_PCIE_STABILITY
extern void dhdpcie_bus_intr_disable(struct dhd_bus *bus);
static void dhd_hang_work_handler(struct work_struct *work)
{
	dhd_pub_t *dhdp;

	dhdp = container_of(work, dhd_pub_t, hang_work);

	if (!dhdp) {
		DHD_ERROR(("%s: dhdp is NULL\n", __FUNCTION__));
		return;
	}

	dhd_hang_process(dhdp->info, NULL, 0);
}
#endif
int dhd_os_send_hang_message(dhd_pub_t *dhdp)
{
	int ret = 0;
	if (dhdp) {
#ifdef HW_PCIE_STABILITY
        unsigned long flags;
        bool disable_intr = false;

        DHD_GENERAL_LOCK(dhdp, flags);
        if (dhdp->busstate == DHD_BUS_DATA) {
                disable_intr = true;
        }
        /* Enforce bus down to stop any future traffic */
        dhdp->busstate = DHD_BUS_DOWN;
        DHD_GENERAL_UNLOCK(dhdp, flags);

        if (disable_intr) {
                DHD_ERROR(("%s: disable PCIe interrupts.\n", __FUNCTION__));
                dhdpcie_bus_intr_disable(dhdp->bus);
        }
#endif /* BCMPCIE */
#ifndef HW_PCIE_STABILITY
		if (!dhdp->hang_was_sent) {
#else
		if(dhdp->hang_report && !dhdp->hang_was_sent) {
#endif
			dhdp->hang_was_sent = 1;
#ifdef HW_PCIE_STABILITY
			/* co-work with dev_close() */
			schedule_work(&dhdp->hang_work);
#else
#ifdef HW_DEFERRED_WORK_BUGFIX
			ret = dhd_deferred_schedule_work(dhdp->info->dhd_deferred_wq, (void *)dhdp,
					DHD_WQ_WORK_HANG_MSG, dhd_hang_process, DHD_WORK_PRIORITY_HIGH);
			if (ret) {
				dhdp->hang_was_sent = 0;
				DHD_ERROR(("dhd_os_send_hang_message failed\n"));
			}
#else
			dhd_deferred_schedule_work(dhdp->info->dhd_deferred_wq, (void *)dhdp,
				DHD_WQ_WORK_HANG_MSG, dhd_hang_process, DHD_WORK_PRIORITY_HIGH);
#endif
#endif
		}
	}
	return ret;
}

int net_os_send_hang_message(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	int ret = 0;

	if (dhd) {
		/* Report FW problem when enabled */
		if (dhd->pub.hang_report) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27))
			ret = dhd_os_send_hang_message(&dhd->pub);
#else
			ret = wl_cfg80211_hang(dev, WLAN_REASON_UNSPECIFIED);
#endif
		} else {
			DHD_ERROR(("%s: FW HANG ignored (for testing purpose) and not sent up\n",
				__FUNCTION__));
			/* Enforce bus down to stop any future traffic */
			dhd->pub.busstate = DHD_BUS_DOWN;
		}
	}
	return ret;
}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27) && OEM_ANDROID */


int dhd_net_wifi_platform_set_power(struct net_device *dev, bool on, unsigned long delay_msec)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	return wifi_platform_set_power(dhd->adapter, on, delay_msec);
}

bool dhd_force_country_change(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);

	if (dhd && dhd->pub.up)
		return dhd->pub.force_country_change;
	return FALSE;
}

void dhd_get_customized_country_code(struct net_device *dev, char *country_iso_code,
	wl_country_t *cspec)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	get_customized_country_code(dhd->adapter, country_iso_code, cspec,
				    dhd->pub.dhd_cflags);
}

void dhd_bus_country_set(struct net_device *dev, wl_country_t *cspec, bool notify)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	if (dhd && dhd->pub.up) {
		memcpy(&dhd->pub.dhd_cspec, cspec, sizeof(wl_country_t));
		dhd->pub.force_country_change = FALSE;
#ifdef WL_CFG80211
		wl_update_wiphybands(NULL, notify);
#endif
	}
}

void dhd_bus_band_set(struct net_device *dev, uint band)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	if (dhd && dhd->pub.up) {
#ifdef WL_CFG80211
		wl_update_wiphybands(NULL, true);
#endif
	}
}

int dhd_net_set_fw_path(struct net_device *dev, char *fw)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);

	if (!fw || fw[0] == '\0')
		return -EINVAL;

	strncpy(dhd->fw_path, fw, sizeof(dhd->fw_path) - 1);
	dhd->fw_path[sizeof(dhd->fw_path)-1] = '\0';

#if defined(SOFTAP)
	if (strstr(fw, "apsta") != NULL) {
		DHD_INFO(("GOT APSTA FIRMWARE\n"));
		ap_fw_loaded = TRUE;
	} else {
		DHD_INFO(("GOT STA FIRMWARE\n"));
		ap_fw_loaded = FALSE;
	}
#endif
	return 0;
}

void dhd_net_if_lock(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	dhd_net_if_lock_local(dhd);
}

void dhd_net_if_unlock(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	dhd_net_if_unlock_local(dhd);
}

static void dhd_net_if_lock_local(dhd_info_t *dhd)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25))
	if (dhd)
		mutex_lock(&dhd->dhd_net_if_mutex);
#endif
}

static void dhd_net_if_unlock_local(dhd_info_t *dhd)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25))
	if (dhd)
		mutex_unlock(&dhd->dhd_net_if_mutex);
#endif
}

#ifdef HW_WIFI_OPEN_STOP
static void dhd_open_stop_if_mutex_lock(dhd_info_t *dhd)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)) && 1
   if (dhd)
       mutex_lock(&dhd->dhd_open_stop_if_mutex);
#endif
}
static void dhd_open_stop_if_mutex_unlock(dhd_info_t *dhd)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)) && 1
   if (dhd)
       mutex_unlock(&dhd->dhd_open_stop_if_mutex);
#endif
}
#endif

static void dhd_suspend_lock(dhd_pub_t *pub)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25))
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);
	if (dhd)
		mutex_lock(&dhd->dhd_suspend_mutex);
#endif
}

static void dhd_suspend_unlock(dhd_pub_t *pub)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25))
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);
	if (dhd)
		mutex_unlock(&dhd->dhd_suspend_mutex);
#endif
}

unsigned long dhd_os_general_spin_lock(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);
	unsigned long flags = 0;

	if (dhd)
		spin_lock_irqsave(&dhd->dhd_lock, flags);

	return flags;
}

void dhd_os_general_spin_unlock(dhd_pub_t *pub, unsigned long flags)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);

	if (dhd)
		spin_unlock_irqrestore(&dhd->dhd_lock, flags);
}

/* Linux specific multipurpose spinlock API */
void *
dhd_os_spin_lock_init(osl_t *osh)
{
	/* Adding 4 bytes since the sizeof(spinlock_t) could be 0 */
	/* if CONFIG_SMP and CONFIG_DEBUG_SPINLOCK are not defined */
	/* and this results in kernel asserts in internal builds */
	spinlock_t * lock = MALLOC(osh, sizeof(spinlock_t) + 4);
	if (lock)
		spin_lock_init(lock);
	return ((void *)lock);
}
void
dhd_os_spin_lock_deinit(osl_t *osh, void *lock)
{
#ifdef BCM_PCIE_UPDATE
	if (lock)
#endif
		MFREE(osh, lock, sizeof(spinlock_t) + 4);
}
unsigned long
dhd_os_spin_lock(void *lock)
{
	unsigned long flags = 0;

	if (lock)
		spin_lock_irqsave((spinlock_t *)lock, flags);

	return flags;
}
void
dhd_os_spin_unlock(void *lock, unsigned long flags)
{
	if (lock)
		spin_unlock_irqrestore((spinlock_t *)lock, flags);
}

static int
dhd_get_pend_8021x_cnt(dhd_info_t *dhd)
{
	return (atomic_read(&dhd->pend_8021x_cnt));
}

#define MAX_WAIT_FOR_8021X_TX	100

int
dhd_wait_pend8021x(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	int timeout = msecs_to_jiffies(10);
	int ntimes = MAX_WAIT_FOR_8021X_TX;
	int pend = dhd_get_pend_8021x_cnt(dhd);

	while (ntimes && pend) {
		if (pend) {
			set_current_state(TASK_INTERRUPTIBLE);
			DHD_PERIM_UNLOCK(&dhd->pub);
			schedule_timeout(timeout);
			DHD_PERIM_LOCK(&dhd->pub);
			set_current_state(TASK_RUNNING);
			ntimes--;
		}
		pend = dhd_get_pend_8021x_cnt(dhd);
	}
	if (ntimes == 0)
	{
		atomic_set(&dhd->pend_8021x_cnt, 0);
		DHD_ERROR(("%s: TIMEOUT\n", __FUNCTION__));
	}
	return pend;
}

#ifdef DHD_DEBUG
int
write_to_file(dhd_pub_t *dhd, uint8 *buf, int size)
{
	int ret = 0;
	struct file *fp;
	mm_segment_t old_fs;
	loff_t pos = 0;

	/* change to KERNEL_DS address limit */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	/* open file to write */
	fp = filp_open("/data/misc/wifi/mem_dump", O_WRONLY|O_CREAT, 0640);
	if (!fp) {
		printf("%s: open file error\n", __FUNCTION__);
		ret = -1;
		goto exit;
	}

	/* Write buf to file */
	fp->f_op->write(fp, buf, size, &pos);

exit:
	/* free buf before return */
	MFREE(dhd->osh, buf, size);
	/* close file before return */
	if (fp)
		filp_close(fp, current->files);
	/* restore previous address limit */
	set_fs(old_fs);

	return ret;
}
#endif /* DHD_DEBUG */

int dhd_os_wake_lock_timeout(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);
	unsigned long flags;
	int ret = 0;

	if (dhd) {
		spin_lock_irqsave(&dhd->wakelock_spinlock, flags);
		ret = dhd->wakelock_rx_timeout_enable > dhd->wakelock_ctrl_timeout_enable ?
			dhd->wakelock_rx_timeout_enable : dhd->wakelock_ctrl_timeout_enable;
#ifdef CONFIG_HAS_WAKELOCK
		if (dhd->wakelock_rx_timeout_enable)
			wake_lock_timeout(&dhd->wl_rxwake,
				msecs_to_jiffies(dhd->wakelock_rx_timeout_enable));
		if (dhd->wakelock_ctrl_timeout_enable)
			wake_lock_timeout(&dhd->wl_ctrlwake,
				msecs_to_jiffies(dhd->wakelock_ctrl_timeout_enable));
#endif
		dhd->wakelock_rx_timeout_enable = 0;
		dhd->wakelock_ctrl_timeout_enable = 0;
		spin_unlock_irqrestore(&dhd->wakelock_spinlock, flags);
	}
	return ret;
}

int net_os_wake_lock_timeout(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	int ret = 0;

	if (dhd)
		ret = dhd_os_wake_lock_timeout(&dhd->pub);
	return ret;
}

int dhd_os_wake_lock_rx_timeout_enable(dhd_pub_t *pub, int val)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);
	unsigned long flags;

	if (dhd) {
		spin_lock_irqsave(&dhd->wakelock_spinlock, flags);
		if (val > dhd->wakelock_rx_timeout_enable)
			dhd->wakelock_rx_timeout_enable = val;
		spin_unlock_irqrestore(&dhd->wakelock_spinlock, flags);
	}
	return 0;
}

int dhd_os_wake_lock_ctrl_timeout_enable(dhd_pub_t *pub, int val)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);
	unsigned long flags;

	if (dhd) {
		spin_lock_irqsave(&dhd->wakelock_spinlock, flags);
		if (val > dhd->wakelock_ctrl_timeout_enable)
			dhd->wakelock_ctrl_timeout_enable = val;
		spin_unlock_irqrestore(&dhd->wakelock_spinlock, flags);
	}
	return 0;
}

int dhd_os_wake_lock_ctrl_timeout_cancel(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);
	unsigned long flags;

	if (dhd) {
		spin_lock_irqsave(&dhd->wakelock_spinlock, flags);
		dhd->wakelock_ctrl_timeout_enable = 0;
#ifdef CONFIG_HAS_WAKELOCK
		if (wake_lock_active(&dhd->wl_ctrlwake))
			wake_unlock(&dhd->wl_ctrlwake);
#endif
		spin_unlock_irqrestore(&dhd->wakelock_spinlock, flags);
	}
	return 0;
}

int net_os_wake_lock_rx_timeout_enable(struct net_device *dev, int val)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	int ret = 0;

	if (dhd)
		ret = dhd_os_wake_lock_rx_timeout_enable(&dhd->pub, val);
	return ret;
}

int net_os_wake_lock_ctrl_timeout_enable(struct net_device *dev, int val)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	int ret = 0;

	if (dhd)
		ret = dhd_os_wake_lock_ctrl_timeout_enable(&dhd->pub, val);
	return ret;
}


#if defined(DHD_TRACE_WAKE_LOCK)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0))
#include <linux/hashtable.h>
#else
#include <linux/hash.h>
#endif /* KERNEL_VER >= KERNEL_VERSION(3, 7, 0) */


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0))
/* Define 2^5 = 32 bucket size hash table */
DEFINE_HASHTABLE(wklock_history, 5);
#else
/* Define 2^5 = 32 bucket size hash table */
struct hlist_head wklock_history[32] = { [0 ... 31] = HLIST_HEAD_INIT };
#endif /* KERNEL_VER >= KERNEL_VERSION(3, 7, 0) */

int trace_wklock_onoff = 1;
unsigned long long wklock_hold_ms = 0;
unsigned long wklock_start_jiffies = 0;
unsigned long wklock_stop_jiffies = 0;

typedef enum dhd_wklock_type {
	DHD_WAKE_LOCK,
	DHD_WAKE_UNLOCK,
	DHD_WAIVE_LOCK,
	DHD_RESTORE_LOCK
} dhd_wklock_t;

struct wk_trace_record {
	unsigned long long jiffies;		/* The last update time */
	unsigned long addr;	            /* Address of the instruction */
	dhd_wklock_t lock_type;         /* lock_type */
	unsigned long long counter;		/* counter information */
	struct hlist_node wklock_node;  /* hash node */
};


static struct wk_trace_record *find_wklock_entry(unsigned long addr)
{
	struct wk_trace_record *wklock_info;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0))
	hash_for_each_possible(wklock_history, wklock_info, wklock_node, addr)
#else
	struct hlist_node *entry;
	int index = hash_long(addr, ilog2(ARRAY_SIZE(wklock_history)));
	hlist_for_each_entry(wklock_info, entry, &wklock_history[index], wklock_node)
#endif /* KERNEL_VER >= KERNEL_VERSION(3, 7, 0) */
	{
		if (wklock_info->addr == addr) {
			return wklock_info;
		}
	}
	return NULL;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0))
#define HASH_ADD(hashtable, node, key) \
	do { \
		hash_add(hashtable, node, key); \
	} while (0);
#else
#define HASH_ADD(hashtable, node, key) \
	do { \
		int index = hash_long(key, ilog2(ARRAY_SIZE(hashtable))); \
		hlist_add_head(node, &hashtable[index]); \
	} while (0);
#endif /* KERNEL_VER < KERNEL_VERSION(3, 7, 0) */

#define SET_WKLOCK_START_TIME() wklock_start_jiffies = jiffies 
#define SET_WKLOCK_STOP_TIME() \
       do {\
               wklock_stop_jiffies = jiffies;\
               if (time_after(wklock_stop_jiffies, wklock_start_jiffies)) {\
                       if (wklock_stop_jiffies>wklock_start_jiffies) \
                               wklock_hold_ms += jiffies_to_msecs(wklock_stop_jiffies-wklock_start_jiffies);\
                       else\
                               wklock_hold_ms += jiffies_to_msecs(wklock_stop_jiffies+(MAX_JIFFY_OFFSET-wklock_start_jiffies)+1);\
               }\
       } while(0);


#define STORE_WKLOCK_RECORD(wklock_type) \
	do { \
		struct wk_trace_record *wklock_info = NULL; \
		unsigned long func_addr = (unsigned long)__builtin_return_address(0); \
		wklock_info = find_wklock_entry(func_addr); \
		if (wklock_info) { \
			wklock_info->jiffies = jiffies; \
			if (wklock_type == DHD_WAIVE_LOCK || wklock_type == DHD_RESTORE_LOCK) { \
				wklock_info->counter = dhd->wakelock_counter; \
			} else { \
				wklock_info->counter++; \
			} \
		} else { \
			wklock_info = kzalloc(sizeof(*wklock_info), GFP_ATOMIC); \
			if (!wklock_info) {\
				printk("Can't allocate wk_trace_record \n"); \
			} else { \
				wklock_info->jiffies = jiffies; \
				wklock_info->addr = func_addr; \
				wklock_info->lock_type = wklock_type; \
				if (wklock_type == DHD_WAIVE_LOCK || \
						wklock_type == DHD_RESTORE_LOCK) { \
					wklock_info->counter = dhd->wakelock_counter; \
				} else { \
					wklock_info->counter++; \
				} \
				HASH_ADD(wklock_history, &wklock_info->wklock_node, func_addr); \
			} \
		} \
	} while (0);

static inline void dhd_wk_lock_rec_dump(void)
{
	int bkt;
	struct wk_trace_record *wklock_info;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0))
	hash_for_each(wklock_history, bkt, wklock_info, wklock_node)
#else
	struct hlist_node *entry = NULL;
	int max_index = ARRAY_SIZE(wklock_history);
	for (bkt = 0; bkt < max_index; bkt++)
		hlist_for_each_entry(wklock_info, entry, &wklock_history[bkt], wklock_node)
#endif /* KERNEL_VER >= KERNEL_VERSION(3, 7, 0) */
		{
			DHD_ERROR(("Jiffies[%llu] ", wklock_info->jiffies));
			switch (wklock_info->lock_type) {
				case DHD_WAKE_LOCK:
					DHD_ERROR(("wakelock lock : %pS  lock_counter : %llu\n",
						(void *)wklock_info->addr, wklock_info->counter));
					break;
				case DHD_WAKE_UNLOCK:
					DHD_ERROR(("wakelock unlock : %pS, unlock_counter : %llu\n",
						(void *)wklock_info->addr, wklock_info->counter));
					break;
				case DHD_WAIVE_LOCK:
					DHD_ERROR(("wakelock waive : %pS  before_waive : %llu\n",
						(void *)wklock_info->addr, wklock_info->counter));
					break;
				case DHD_RESTORE_LOCK:
					DHD_ERROR(("wakelock restore : %pS, after_waive : %llu\n",
						(void *)wklock_info->addr, wklock_info->counter));
					break;
			}
		}
}

static void dhd_wk_lock_trace_init(struct dhd_info *dhd)
{
	unsigned long flags;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0))
	int i;
#endif /* KERNEL_VER >= KERNEL_VERSION(3, 7, 0) */
	wklock_hold_ms = 0;
    wklock_start_jiffies = 0;
	spin_lock_irqsave(&dhd->wakelock_spinlock, flags);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0))
	hash_init(wklock_history);
#else
	for (i = 0; i < ARRAY_SIZE(wklock_history); i++)
		INIT_HLIST_HEAD(&wklock_history[i]);
#endif /* KERNEL_VER >= KERNEL_VERSION(3, 7, 0) */
	spin_unlock_irqrestore(&dhd->wakelock_spinlock, flags);
}

static void dhd_wk_lock_trace_deinit(struct dhd_info *dhd)
{
	int bkt;
	struct wk_trace_record *wklock_info;
	struct hlist_node *tmp;
	unsigned long flags;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0))
	struct hlist_node *entry = NULL;
	int max_index = ARRAY_SIZE(wklock_history);
#endif /* KERNEL_VER >= KERNEL_VERSION(3, 7, 0) */

	spin_lock_irqsave(&dhd->wakelock_spinlock, flags);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0))
	hash_for_each_safe(wklock_history, bkt, tmp, wklock_info, wklock_node)
#else
	for (bkt = 0; bkt < max_index; bkt++)
		hlist_for_each_entry_safe(wklock_info, entry, tmp,
			&wklock_history[bkt], wklock_node)
#endif /* KERNEL_VER >= KERNEL_VERSION(3, 7, 0)) */
		{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0))
			hash_del(&wklock_info->wklock_node);
#else
			hlist_del_init(&wklock_info->wklock_node);
#endif /* KERNEL_VER >= KERNEL_VERSION(3, 7, 0)) */
			kfree(wklock_info);
		}
	spin_unlock_irqrestore(&dhd->wakelock_spinlock, flags);
}

void dhd_wk_lock_stats_dump(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd = (dhd_info_t *)(dhdp->info);
	unsigned long flags;

	DHD_ERROR((KERN_ERR"DHD Printing wl_wake Lock/Unlock Record \r\n"));
	spin_lock_irqsave(&dhd->wakelock_spinlock, flags);
	dhd_wk_lock_rec_dump();
	spin_unlock_irqrestore(&dhd->wakelock_spinlock, flags);
	DHD_ERROR((KERN_ERR"Wlan_wake wakelock counter is %u. Hold ms = %llu\n", dhd->wakelock_counter, wklock_hold_ms));
	DHD_ERROR((KERN_ERR"Event wakelock counter %u\n", dhd->wakelock_event_counter));
	//start time
    SET_WKLOCK_START_TIME(); 
}
#else
#define STORE_WKLOCK_RECORD(wklock_type)
#endif /* ! DHD_TRACE_WAKE_LOCK */

int dhd_os_wake_lock(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);
	unsigned long flags;
	int ret = 0;

	if (dhd) {
		spin_lock_irqsave(&dhd->wakelock_spinlock, flags);
		if (dhd->wakelock_counter == 0 && !dhd->waive_wakelock) {
#ifdef CONFIG_HAS_WAKELOCK
			wake_lock(&dhd->wl_wifi);
#elif defined(BCMSDIO) && (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36))
			dhd_bus_dev_pm_stay_awake(pub);
#endif
		}
#ifdef DHD_TRACE_WAKE_LOCK
		if (trace_wklock_onoff) {
			STORE_WKLOCK_RECORD(DHD_WAKE_LOCK);
		}
#endif /* DHD_TRACE_WAKE_LOCK */
		dhd->wakelock_counter++;
		ret = dhd->wakelock_counter;
		spin_unlock_irqrestore(&dhd->wakelock_spinlock, flags);
	}
	return ret;
}
#ifdef BCM_PCIE_UPDATE
int dhd_event_wake_lock(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);
	unsigned long flags;
	int ret = 0;

	if (dhd) {
		spin_lock_irqsave(&dhd->wakelock_evt_spinlock, flags);
		if (dhd->wakelock_event_counter == 0) {
#ifdef CONFIG_HAS_WAKELOCK
			wake_lock(&dhd->wl_evtwake);
#elif defined(BCMSDIO) && (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36))
			dhd_bus_dev_pm_stay_awake(pub);
#endif
		}
		dhd->wakelock_event_counter++;
		ret = dhd->wakelock_event_counter;
		spin_unlock_irqrestore(&dhd->wakelock_evt_spinlock, flags);
	}

	return ret;
}
#endif
int net_os_wake_lock(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	int ret = 0;

	if (dhd)
		ret = dhd_os_wake_lock(&dhd->pub);
	return ret;
}

int dhd_os_wake_unlock(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);
	unsigned long flags;
	int ret = 0;

	dhd_os_wake_lock_timeout(pub);
	if (dhd) {
		spin_lock_irqsave(&dhd->wakelock_spinlock, flags);
		if (dhd->wakelock_counter > 0) {
			dhd->wakelock_counter--;
#ifdef DHD_TRACE_WAKE_LOCK
			if (trace_wklock_onoff) {
				STORE_WKLOCK_RECORD(DHD_WAKE_UNLOCK);
			}
#endif /* DHD_TRACE_WAKE_LOCK */
			if (dhd->wakelock_counter == 0 && !dhd->waive_wakelock) {
#ifdef CONFIG_HAS_WAKELOCK
				wake_unlock(&dhd->wl_wifi);
#elif defined(BCMSDIO) && (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36))
				dhd_bus_dev_pm_relax(pub);
#endif
#ifdef DHD_TRACE_WAKE_LOCK
				SET_WKLOCK_STOP_TIME();
#endif
			}
			ret = dhd->wakelock_counter;
		}
		spin_unlock_irqrestore(&dhd->wakelock_spinlock, flags);
	}
	return ret;
}
#ifdef BCM_PCIE_UPDATE
int dhd_event_wake_unlock(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);
	unsigned long flags;
	int ret = 0;

	if (dhd) {
		spin_lock_irqsave(&dhd->wakelock_evt_spinlock, flags);

		if (dhd->wakelock_event_counter > 0) {
			dhd->wakelock_event_counter--;
			if (dhd->wakelock_event_counter == 0) {
#ifdef CONFIG_HAS_WAKELOCK
				wake_unlock(&dhd->wl_evtwake);
#elif defined(BCMSDIO) && (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36))
				dhd_bus_dev_pm_relax(pub);
#endif
			}
			ret = dhd->wakelock_event_counter;
		}
		spin_unlock_irqrestore(&dhd->wakelock_evt_spinlock, flags);
	}
	return ret;
}
#endif
int dhd_os_check_wakelock(dhd_pub_t *pub)
{
#if defined(CONFIG_HAS_WAKELOCK) || (defined(BCMSDIO) && (LINUX_VERSION_CODE > \
	KERNEL_VERSION(2, 6, 36)))
	dhd_info_t *dhd;

	if (!pub)
		return 0;
	dhd = (dhd_info_t *)(pub->info);
#endif /* CONFIG_HAS_WAKELOCK || BCMSDIO */

#ifdef CONFIG_HAS_WAKELOCK
	/* Indicate to the SD Host to avoid going to suspend if internal locks are up */
	if (dhd && (wake_lock_active(&dhd->wl_wifi) ||
		(wake_lock_active(&dhd->wl_wdwake))))
		return 1;
#elif defined(BCMSDIO) && (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36))
	if (dhd && (dhd->wakelock_counter > 0) && dhd_bus_dev_pm_enabled(pub))
		return 1;
#endif
	return 0;
}

#ifndef BCM_PCIE_UPDATE
int dhd_os_check_wakelock_all(dhd_pub_t *pub)
{
#if defined(CONFIG_HAS_WAKELOCK) || \
	(defined(BCMSDIO) && (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36)))
	dhd_info_t *dhd;

	if (!pub)
		return 0;
	dhd = (dhd_info_t *)(pub->info);
#endif /* CONFIG_HAS_WAKELOCK || BCMSDIO */

#ifdef CONFIG_HAS_WAKELOCK
	/* Indicate to the SD Host to avoid going to suspend if internal locks are up */
	if (dhd && (wake_lock_active(&dhd->wl_wifi) ||
		wake_lock_active(&dhd->wl_wdwake) ||
		wake_lock_active(&dhd->wl_rxwake) ||
		wake_lock_active(&dhd->wl_ctrlwake))) {
		return 1;
	}
#elif defined(BCMSDIO) && (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36))
	if (dhd && (dhd->wakelock_counter > 0) && dhd_bus_dev_pm_enabled(pub))
		return 1;
#endif
	return 0;
}
#else
int
dhd_os_check_wakelock_all(dhd_pub_t *pub)
{
#ifdef CONFIG_HAS_WAKELOCK
	int l1, l2, l3, l4, l7;
	int l5 = 0, l6 = 0;
	int c, lock_active;
#endif /* CONFIG_HAS_WAKELOCK */
#if defined(CONFIG_HAS_WAKELOCK) || (defined(BCMSDIO) && (LINUX_VERSION_CODE > \
	KERNEL_VERSION(2, 6, 36)))
	dhd_info_t *dhd;

	if (!pub) {
		return 0;
	}
	dhd = (dhd_info_t *)(pub->info);
	if (!dhd) {
		return 0;
	}
#endif /* CONFIG_HAS_WAKELOCK || BCMSDIO */

#ifdef CONFIG_HAS_WAKELOCK
	c = dhd->wakelock_counter;
	l1 = wake_lock_active(&dhd->wl_wifi);
	l2 = wake_lock_active(&dhd->wl_wdwake);
	l3 = wake_lock_active(&dhd->wl_rxwake);
	l4 = wake_lock_active(&dhd->wl_ctrlwake);
#ifdef BCMPCIE_OOB_HOST_WAKE
	l5 = wake_lock_active(&dhd->wl_intrwake);
#endif /* BCMPCIE_OOB_HOST_WAKE */
#ifdef DHD_USE_SCAN_WAKELOCK
	l6 = wake_lock_active(&dhd->wl_scanwake);
#endif /* DHD_USE_SCAN_WAKELOCK */
	l7 = wake_lock_active(&dhd->wl_evtwake);
	lock_active = (l1 || l2 || l3 || l4 || l5 || l6 || l7);

	/* Indicate to the Host to avoid going to suspend if internal locks are up */
	if (dhd && lock_active) {
		DHD_ERROR(("%s wakelock c-%d wl-%d wd-%d rx-%d "
			"ctl-%d intr-%d scan-%d evt-%d\n",
			__FUNCTION__, c, l1, l2, l3, l4, l5, l6, l7));
		return 1;
	}
#elif defined(BCMSDIO) && (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36))
	if (dhd && (dhd->wakelock_counter > 0) && dhd_bus_dev_pm_enabled(pub)) {
		return 1;
	}
#endif /* CONFIG_HAS_WAKELOCK */
	return 0;
}
#endif /* BCM_PCIE_UPDATE */
int net_os_wake_unlock(struct net_device *dev)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);
	int ret = 0;

	if (dhd)
		ret = dhd_os_wake_unlock(&dhd->pub);
	return ret;
}

int dhd_os_wd_wake_lock(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);
	unsigned long flags;
	int ret = 0;

	if (dhd) {
		spin_lock_irqsave(&dhd->wakelock_spinlock, flags);
#ifdef CONFIG_HAS_WAKELOCK
		/* if wakelock_wd_counter was never used : lock it at once */
		if (!dhd->wakelock_wd_counter)
			wake_lock(&dhd->wl_wdwake);
#endif
		dhd->wakelock_wd_counter++;
		ret = dhd->wakelock_wd_counter;
		spin_unlock_irqrestore(&dhd->wakelock_spinlock, flags);
	}
	return ret;
}

int dhd_os_wd_wake_unlock(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);
	unsigned long flags;
	int ret = 0;

	if (dhd) {
		spin_lock_irqsave(&dhd->wakelock_spinlock, flags);
		if (dhd->wakelock_wd_counter) {
			dhd->wakelock_wd_counter = 0;
#ifdef CONFIG_HAS_WAKELOCK
			wake_unlock(&dhd->wl_wdwake);
#endif
		}
		spin_unlock_irqrestore(&dhd->wakelock_spinlock, flags);
	}
	return ret;
}

#ifdef BCMPCIE_OOB_HOST_WAKE
void
dhd_os_oob_irq_wake_lock_timeout(dhd_pub_t *pub, int val)
{
#ifdef CONFIG_HAS_WAKELOCK
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);

	if (dhd) {
		wake_lock_timeout(&dhd->wl_intrwake, msecs_to_jiffies(val));
	}
#endif /* CONFIG_HAS_WAKELOCK */
}

void
dhd_os_oob_irq_wake_unlock(dhd_pub_t *pub)
{
#ifdef CONFIG_HAS_WAKELOCK
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);

	if (dhd) {
		/* if wl_intrwake is active, unlock it */
		if (wake_lock_active(&dhd->wl_intrwake)) {
			wake_unlock(&dhd->wl_intrwake);
		}
	}
#endif /* CONFIG_HAS_WAKELOCK */
}
#endif /* BCMPCIE_OOB_HOST_WAKE */

#ifdef DHD_USE_SCAN_WAKELOCK
void
dhd_os_scan_wake_lock_timeout(dhd_pub_t *pub, int val)
{
#ifdef CONFIG_HAS_WAKELOCK
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);

	if (dhd) {
		wake_lock_timeout(&dhd->wl_scanwake, msecs_to_jiffies(val));
	}
#endif /* CONFIG_HAS_WAKELOCK */
}

void
dhd_os_scan_wake_unlock(dhd_pub_t *pub)
{
#ifdef CONFIG_HAS_WAKELOCK
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);

	if (dhd) {
		/* if wl_scanwake is active, unlock it */
		if (wake_lock_active(&dhd->wl_scanwake)) {
			wake_unlock(&dhd->wl_scanwake);
		}
	}
#endif /* CONFIG_HAS_WAKELOCK */
}
#endif /* DHD_USE_SCAN_WAKELOCK */

/* waive wakelocks for operations such as IOVARs in suspend function, must be closed
 * by a paired function call to dhd_wakelock_restore. returns current wakelock counter
 */
int dhd_os_wake_lock_waive(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);
	unsigned long flags;
	int ret = 0;

	if (dhd) {
		spin_lock_irqsave(&dhd->wakelock_spinlock, flags);
		/* dhd_wakelock_waive/dhd_wakelock_restore must be paired */
		if (dhd->waive_wakelock == FALSE) {
#ifdef DHD_TRACE_WAKE_LOCK
			if (trace_wklock_onoff) {
				STORE_WKLOCK_RECORD(DHD_WAIVE_LOCK);
			}
#endif /* DHD_TRACE_WAKE_LOCK */
			/* record current lock status */
			dhd->wakelock_before_waive = dhd->wakelock_counter;
			dhd->waive_wakelock = TRUE;
		}
		ret = dhd->wakelock_wd_counter;
		spin_unlock_irqrestore(&dhd->wakelock_spinlock, flags);
	}
	return ret;
}

int dhd_os_wake_lock_restore(dhd_pub_t *pub)
{
	dhd_info_t *dhd = (dhd_info_t *)(pub->info);
	unsigned long flags;
	int ret = 0;

	if (!dhd)
		return 0;

	spin_lock_irqsave(&dhd->wakelock_spinlock, flags);
	/* dhd_wakelock_waive/dhd_wakelock_restore must be paired */
	if (!dhd->waive_wakelock)
		goto exit;

	dhd->waive_wakelock = FALSE;
	/* if somebody else acquires wakelock between dhd_wakelock_waive/dhd_wakelock_restore,
	 * we need to make it up by calling wake_lock or pm_stay_awake. or if somebody releases
	 * the lock in between, do the same by calling wake_unlock or pm_relax
	 */
#ifdef DHD_TRACE_WAKE_LOCK
	if (trace_wklock_onoff) {
		STORE_WKLOCK_RECORD(DHD_RESTORE_LOCK);
	}
#endif /* DHD_TRACE_WAKE_LOCK */

	if (dhd->wakelock_before_waive == 0 && dhd->wakelock_counter > 0) {
#ifdef CONFIG_HAS_WAKELOCK
		wake_lock(&dhd->wl_wifi);
#elif defined(BCMSDIO) && (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36))
		dhd_bus_dev_pm_stay_awake(&dhd->pub);
#endif
#ifdef DHD_TRACE_WAKE_LOCK
		SET_WKLOCK_START_TIME(); 
#endif
	} else if (dhd->wakelock_before_waive > 0 && dhd->wakelock_counter == 0) {
#ifdef CONFIG_HAS_WAKELOCK
		wake_unlock(&dhd->wl_wifi);
#elif defined(BCMSDIO) && (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 36))
		dhd_bus_dev_pm_relax(&dhd->pub);
#endif
#ifdef DHD_TRACE_WAKE_LOCK
		SET_WKLOCK_STOP_TIME();
#endif
	}
	dhd->wakelock_before_waive = 0;
exit:
	ret = dhd->wakelock_wd_counter;
	spin_unlock_irqrestore(&dhd->wakelock_spinlock, flags);
	return ret;
}
#ifdef BCM_PCIE_UPDATE
void dhd_os_wake_lock_init(struct dhd_info *dhd)
{
	DHD_TRACE(("%s: initialize wake_lock_counters\n", __FUNCTION__));
	dhd->wakelock_event_counter = 0;
	dhd->wakelock_counter = 0;
	dhd->wakelock_rx_timeout_enable = 0;
	dhd->wakelock_ctrl_timeout_enable = 0;
#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_init(&dhd->wl_wifi, WAKE_LOCK_SUSPEND, "wlan_wake");
	wake_lock_init(&dhd->wl_rxwake, WAKE_LOCK_SUSPEND, "wlan_rx_wake");
	wake_lock_init(&dhd->wl_ctrlwake, WAKE_LOCK_SUSPEND, "wlan_ctrl_wake");
	wake_lock_init(&dhd->wl_evtwake, WAKE_LOCK_SUSPEND, "wlan_evt_wake");
#ifdef BCMPCIE_OOB_HOST_WAKE
	wake_lock_init(&dhd->wl_intrwake, WAKE_LOCK_SUSPEND, "wlan_oob_irq_wake");
#endif /* BCMPCIE_OOB_HOST_WAKE */
#ifdef DHD_USE_SCAN_WAKELOCK
	wake_lock_init(&dhd->wl_scanwake, WAKE_LOCK_SUSPEND, "wlan_scan_wake");
#endif /* DHD_USE_SCAN_WAKELOCK */
#endif /* CONFIG_HAS_WAKELOCK */
#ifdef DHD_TRACE_WAKE_LOCK
	dhd_wk_lock_trace_init(dhd);
#endif /* DHD_TRACE_WAKE_LOCK */
}

void dhd_os_wake_lock_destroy(struct dhd_info *dhd)
{
	DHD_TRACE(("%s: deinit wake_lock_counters\n", __FUNCTION__));
#ifdef CONFIG_HAS_WAKELOCK
	dhd->wakelock_event_counter = 0;
	dhd->wakelock_counter = 0;
	dhd->wakelock_rx_timeout_enable = 0;
	dhd->wakelock_ctrl_timeout_enable = 0;
	wake_lock_destroy(&dhd->wl_wifi);
	wake_lock_destroy(&dhd->wl_rxwake);
	wake_lock_destroy(&dhd->wl_ctrlwake);
	wake_lock_destroy(&dhd->wl_evtwake);
#ifdef BCMPCIE_OOB_HOST_WAKE
	wake_lock_destroy(&dhd->wl_intrwake);
#endif /* BCMPCIE_OOB_HOST_WAKE */
#ifdef DHD_USE_SCAN_WAKELOCK
	wake_lock_destroy(&dhd->wl_scanwake);
#endif /* DHD_USE_SCAN_WAKELOCK */
#ifdef DHD_TRACE_WAKE_LOCK
	dhd_wk_lock_trace_deinit(dhd);
#endif /* DHD_TRACE_WAKE_LOCK */
#endif /* CONFIG_HAS_WAKELOCK */
}
#endif
bool dhd_os_check_if_up(dhd_pub_t *pub)
{
	if (!pub)
		return FALSE;
	return pub->up;
}

int dhd_os_get_wake_irq(dhd_pub_t *pub)
{
	if (!pub)
		return -1;
	return wifi_platform_get_wake_irq(pub->info->adapter);
}

/* function to collect firmware, chip id and chip version info */
void dhd_set_version_info(dhd_pub_t *dhdp, char *fw)
{
	int i;

	i = snprintf(info_string, sizeof(info_string),
		"  Driver: %s\n  Firmware: %s ", EPI_VERSION_STR, fw);

	if (!dhdp)
		return;

#ifdef HW_WIFI_DRIVER_NORMALIZE
#if defined(BCMSDIO) || defined(BCMPCIE)
	i = snprintf(&info_string[i], sizeof(info_string) - i,
		"\n  Chip: %x Rev %x Pkg %x", dhd_bus_chip_id(dhdp),
		dhd_bus_chiprev_id(dhdp), dhd_bus_chippkg_id(dhdp));
#endif
#else
	i = snprintf(&info_string[i], sizeof(info_string) - i,
		"\n  Chip: %x Rev %x Pkg %x", dhd_bus_chip_id(dhdp),
		dhd_bus_chiprev_id(dhdp), dhd_bus_chippkg_id(dhdp));
#endif /* HW_WIFI_DRIVER_NORMALIZE */
}
int dhd_ioctl_entry_local(struct net_device *net, wl_ioctl_t *ioc, int cmd)
{
	int ifidx;
	int ret = 0;
	dhd_info_t *dhd = NULL;

	if (!net || !DEV_PRIV(net)) {
		DHD_ERROR(("%s invalid parameter\n", __FUNCTION__));
		return -EINVAL;
	}

	dhd = DHD_DEV_INFO(net);
	if (!dhd)
		return -EINVAL;

	ifidx = dhd_net2idx(dhd, net);
	if (ifidx == DHD_BAD_IF) {
		DHD_ERROR(("%s bad ifidx\n", __FUNCTION__));
		return -ENODEV;
	}

	DHD_OS_WAKE_LOCK(&dhd->pub);
	DHD_PERIM_LOCK(&dhd->pub);

	ret = dhd_wl_ioctl(&dhd->pub, ifidx, ioc, ioc->buf, ioc->len);
	dhd_check_hang(net, &dhd->pub, ret);

	DHD_PERIM_UNLOCK(&dhd->pub);
	DHD_OS_WAKE_UNLOCK(&dhd->pub);

	return ret;
}

bool dhd_os_check_hang(dhd_pub_t *dhdp, int ifidx, int ret)
{
	struct net_device *net;

	net = dhd_idx2net(dhdp, ifidx);
	if (!net) {
		DHD_ERROR(("%s : Invalid index : %d\n", __FUNCTION__, ifidx));
		return -EINVAL;
	}

	return dhd_check_hang(net, dhdp, ret);
}

/* Return instance */
int dhd_get_instance(dhd_pub_t *dhdp)
{
	return dhdp->info->unit;
}

void dhd_set_short_dwell_time(dhd_pub_t *dhd, int set)
{
	int scan_assoc_time = DHD_SCAN_ASSOC_ACTIVE_TIME;
	int scan_unassoc_time = DHD_SCAN_UNASSOC_ACTIVE_TIME;
	int scan_passive_time = DHD_SCAN_PASSIVE_TIME;
#ifdef HW_SCAN_DEFAULT_TIME
	int scan_nprobes = DHD_SCAN_N_PROBES;
#endif

	DHD_TRACE(("%s: Enter: %d\n", __FUNCTION__, set));
	if (dhd->short_dwell_time != set) {
		if (set) {
			scan_unassoc_time = DHD_SCAN_UNASSOC_ACTIVE_TIME_PS;
		}
		dhd_wl_ioctl_cmd(dhd, WLC_SET_SCAN_UNASSOC_TIME,
				(char *)&scan_unassoc_time,
				sizeof(scan_unassoc_time), TRUE, 0);
		if (dhd->short_dwell_time == -1) {
			dhd_wl_ioctl_cmd(dhd, WLC_SET_SCAN_CHANNEL_TIME,
					(char *)&scan_assoc_time,
					sizeof(scan_assoc_time), TRUE, 0);
			dhd_wl_ioctl_cmd(dhd, WLC_SET_SCAN_PASSIVE_TIME,
					(char *)&scan_passive_time,
					sizeof(scan_passive_time), TRUE, 0);
#ifdef HW_SCAN_DEFAULT_TIME
			dhd_wl_ioctl_cmd(dhd, WLC_SET_SCAN_NPROBES,
					(char *)&scan_nprobes,
					sizeof(scan_nprobes), TRUE, 0);
#endif
		}
		dhd->short_dwell_time = set;
	}
}

#ifdef CUSTOM_SET_SHORT_DWELL_TIME
void net_set_short_dwell_time(struct net_device *dev, int set)
{
	dhd_info_t *dhd = DHD_DEV_INFO(dev);

	dhd_set_short_dwell_time(&dhd->pub, set);
}
#endif

#ifdef PROP_TXSTATUS

void dhd_wlfc_plat_init(void *dhd)
{
	return;
}

void dhd_wlfc_plat_deinit(void *dhd)
{
	return;
}

bool dhd_wlfc_skip_fc(void)
{
	return FALSE;
}
#endif /* PROP_TXSTATUS */

#ifdef BCMDBGFS

#include <linux/debugfs.h>

extern uint32 dhd_readregl(void *bp, uint32 addr);
extern uint32 dhd_writeregl(void *bp, uint32 addr, uint32 data);

typedef struct dhd_dbgfs {
	struct dentry	*debugfs_dir;
	struct dentry	*debugfs_mem;
	dhd_pub_t 	*dhdp;
	uint32 		size;
} dhd_dbgfs_t;

dhd_dbgfs_t g_dbgfs;

static int
dhd_dbg_state_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t
dhd_dbg_state_read(struct file *file, char __user *ubuf,
                       size_t count, loff_t *ppos)
{
	ssize_t rval;
	uint32 tmp;
	loff_t pos = *ppos;
	size_t ret;

	if (pos < 0)
		return -EINVAL;
	if (pos >= g_dbgfs.size || !count)
		return 0;
	if (count > g_dbgfs.size - pos)
		count = g_dbgfs.size - pos;

	/* Basically enforce aligned 4 byte reads. It's up to the user to work out the details */
	tmp = dhd_readregl(g_dbgfs.dhdp->bus, file->f_pos & (~3));

	ret = copy_to_user(ubuf, &tmp, 4);
	if (ret == count)
		return -EFAULT;

	count -= ret;
	*ppos = pos + count;
	rval = count;

	return rval;
}


static ssize_t
dhd_debugfs_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
	loff_t pos = *ppos;
	size_t ret;
	uint32 buf;

	if (pos < 0)
		return -EINVAL;
	if (pos >= g_dbgfs.size || !count)
		return 0;
	if (count > g_dbgfs.size - pos)
		count = g_dbgfs.size - pos;

	ret = copy_from_user(&buf, ubuf, sizeof(uint32));
	if (ret == count)
		return -EFAULT;

	/* Basically enforce aligned 4 byte writes. It's up to the user to work out the details */
	dhd_writeregl(g_dbgfs.dhdp->bus, file->f_pos & (~3), buf);

	return count;
}


loff_t
dhd_debugfs_lseek(struct file *file, loff_t off, int whence)
{
	loff_t pos = -1;

	switch (whence) {
		case 0:
			pos = off;
			break;
		case 1:
			pos = file->f_pos + off;
			break;
		case 2:
			pos = g_dbgfs.size - off;
	}
	return (pos < 0 || pos > g_dbgfs.size) ? -EINVAL : (file->f_pos = pos);
}

static const struct file_operations dhd_dbg_state_ops = {
	.read   = dhd_dbg_state_read,
	.write	= dhd_debugfs_write,
	.open   = dhd_dbg_state_open,
	.llseek	= dhd_debugfs_lseek
};

static void dhd_dbg_create(void)
{
	if (g_dbgfs.debugfs_dir) {
		g_dbgfs.debugfs_mem = debugfs_create_file("mem", 0644, g_dbgfs.debugfs_dir,
			NULL, &dhd_dbg_state_ops);
	}
}

void dhd_dbgfs_init(dhd_pub_t *dhdp)
{
	int err;
#ifdef BCMDBGFS_MEM
	g_dbgfs.dhdp = dhdp;
	g_dbgfs.size = 0x20000000; /* Allow access to various cores regs */
#endif
	g_dbgfs.debugfs_dir = debugfs_create_dir("dhd", 0);
	if (IS_ERR(g_dbgfs.debugfs_dir)) {
		err = PTR_ERR(g_dbgfs.debugfs_dir);
		g_dbgfs.debugfs_dir = NULL;
		return;
	}

	dhd_dbg_create();

	return;
}

void dhd_dbgfs_remove(void)
{
#ifdef BCMDBGFS_MEM
	debugfs_remove(g_dbgfs.debugfs_mem);
#endif
	debugfs_remove(g_dbgfs.debugfs_dir);

	bzero((unsigned char *) &g_dbgfs, sizeof(g_dbgfs));

}
#endif /* BCMDBGFS */

#ifdef WLMEDIA_HTSF
static
void dhd_htsf_addtxts(dhd_pub_t *dhdp, void *pktbuf)
{
	dhd_info_t *dhd = (dhd_info_t *)(dhdp->info);
	struct sk_buff *skb;
	uint32 htsf = 0;
	uint16 dport = 0, oldmagic = 0xACAC;
	char *p1;
	htsfts_t ts;

	/*  timestamp packet  */

	p1 = (char*) PKTDATA(dhdp->osh, pktbuf);

	if (PKTLEN(dhdp->osh, pktbuf) > HTSF_MINLEN) {
/*		memcpy(&proto, p1+26, 4);  	*/
		memcpy(&dport, p1+40, 2);
/* 	proto = ((ntoh32(proto))>> 16) & 0xFF;  */
		dport = ntoh16(dport);
	}

	/* timestamp only if  icmp or udb iperf with port 5555 */
/*	if (proto == 17 && dport == tsport) { */
	if (dport >= tsport && dport <= tsport + 20) {

		skb = (struct sk_buff *) pktbuf;

		htsf = dhd_get_htsf(dhd, 0);
		memset(skb->data + 44, 0, 2); /* clear checksum */
		memcpy(skb->data+82, &oldmagic, 2);
		memcpy(skb->data+84, &htsf, 4);

		memset(&ts, 0, sizeof(htsfts_t));
		ts.magic  = HTSFMAGIC;
		ts.prio   = PKTPRIO(pktbuf);
		ts.seqnum = htsf_seqnum++;
		ts.c10    = get_cycles();
		ts.t10    = htsf;
		ts.endmagic = HTSFENDMAGIC;

		memcpy(skb->data + HTSF_HOSTOFFSET, &ts, sizeof(ts));
	}
}

static void dhd_dump_htsfhisto(histo_t *his, char *s)
{
	int pktcnt = 0, curval = 0, i;
	for (i = 0; i < (NUMBIN-2); i++) {
		curval += 500;
		printf("%d ",  his->bin[i]);
		pktcnt += his->bin[i];
	}
	printf(" max: %d TotPkt: %d neg: %d [%s]\n", his->bin[NUMBIN-2], pktcnt,
		his->bin[NUMBIN-1], s);
}

static
void sorttobin(int value, histo_t *histo)
{
	int i, binval = 0;

	if (value < 0) {
		histo->bin[NUMBIN-1]++;
		return;
	}
	if (value > histo->bin[NUMBIN-2])  /* store the max value  */
		histo->bin[NUMBIN-2] = value;

	for (i = 0; i < (NUMBIN-2); i++) {
		binval += 500; /* 500m s bins */
		if (value <= binval) {
			histo->bin[i]++;
			return;
		}
	}
	histo->bin[NUMBIN-3]++;
}

static
void dhd_htsf_addrxts(dhd_pub_t *dhdp, void *pktbuf)
{
	dhd_info_t *dhd = (dhd_info_t *)dhdp->info;
	struct sk_buff *skb;
	char *p1;
	uint16 old_magic;
	int d1, d2, d3, end2end;
	htsfts_t *htsf_ts;
	uint32 htsf;

	skb = PKTTONATIVE(dhdp->osh, pktbuf);
	p1 = (char*)PKTDATA(dhdp->osh, pktbuf);

	if (PKTLEN(osh, pktbuf) > HTSF_MINLEN) {
		memcpy(&old_magic, p1+78, 2);
		htsf_ts = (htsfts_t*) (p1 + HTSF_HOSTOFFSET - 4);
	}
	else
		return;

	if (htsf_ts->magic == HTSFMAGIC) {
		htsf_ts->tE0 = dhd_get_htsf(dhd, 0);
		htsf_ts->cE0 = get_cycles();
	}

	if (old_magic == 0xACAC) {

		tspktcnt++;
		htsf = dhd_get_htsf(dhd, 0);
		memcpy(skb->data+92, &htsf, sizeof(uint32));

		memcpy(&ts[tsidx].t1, skb->data+80, 16);

		d1 = ts[tsidx].t2 - ts[tsidx].t1;
		d2 = ts[tsidx].t3 - ts[tsidx].t2;
		d3 = ts[tsidx].t4 - ts[tsidx].t3;
		end2end = ts[tsidx].t4 - ts[tsidx].t1;

		sorttobin(d1, &vi_d1);
		sorttobin(d2, &vi_d2);
		sorttobin(d3, &vi_d3);
		sorttobin(end2end, &vi_d4);

		if (end2end > 0 && end2end >  maxdelay) {
			maxdelay = end2end;
			maxdelaypktno = tspktcnt;
			memcpy(&maxdelayts, &ts[tsidx], 16);
		}
		if (++tsidx >= TSMAX)
			tsidx = 0;
	}
}

uint32 dhd_get_htsf(dhd_info_t *dhd, int ifidx)
{
	uint32 htsf = 0, cur_cycle, delta, delta_us;
	uint32    factor, baseval, baseval2;
	cycles_t t;

	t = get_cycles();
	cur_cycle = t;

	if (cur_cycle >  dhd->htsf.last_cycle)
		delta = cur_cycle -  dhd->htsf.last_cycle;
	else {
		delta = cur_cycle + (0xFFFFFFFF -  dhd->htsf.last_cycle);
	}

	delta = delta >> 4;

	if (dhd->htsf.coef) {
		/* times ten to get the first digit */
	        factor = (dhd->htsf.coef*10 + dhd->htsf.coefdec1);
		baseval  = (delta*10)/factor;
		baseval2 = (delta*10)/(factor+1);
		delta_us  = (baseval -  (((baseval - baseval2) * dhd->htsf.coefdec2)) / 10);
		htsf = (delta_us << 4) +  dhd->htsf.last_tsf + HTSF_BUS_DELAY;
	}
	else {
		DHD_ERROR(("-------dhd->htsf.coef = 0 -------\n"));
	}

	return htsf;
}

static void dhd_dump_latency(void)
{
	int i, max = 0;
	int d1, d2, d3, d4, d5;

	printf("T1       T2       T3       T4           d1  d2   t4-t1     i    \n");
	for (i = 0; i < TSMAX; i++) {
		d1 = ts[i].t2 - ts[i].t1;
		d2 = ts[i].t3 - ts[i].t2;
		d3 = ts[i].t4 - ts[i].t3;
		d4 = ts[i].t4 - ts[i].t1;
		d5 = ts[max].t4-ts[max].t1;
		if (d4 > d5 && d4 > 0)  {
			max = i;
		}
		printf("%08X %08X %08X %08X \t%d %d %d   %d i=%d\n",
			ts[i].t1, ts[i].t2, ts[i].t3, ts[i].t4,
			d1, d2, d3, d4, i);
	}

	printf("current idx = %d \n", tsidx);

	printf("Highest latency %d pkt no.%d total=%d\n", maxdelay, maxdelaypktno, tspktcnt);
	printf("%08X %08X %08X %08X \t%d %d %d   %d\n",
	maxdelayts.t1, maxdelayts.t2, maxdelayts.t3, maxdelayts.t4,
	maxdelayts.t2 - maxdelayts.t1,
	maxdelayts.t3 - maxdelayts.t2,
	maxdelayts.t4 - maxdelayts.t3,
	maxdelayts.t4 - maxdelayts.t1);
}


static int
dhd_ioctl_htsf_get(dhd_info_t *dhd, int ifidx)
{
	char buf[32];
	int ret;
	uint32 s1, s2;

	struct tsf {
		uint32 low;
		uint32 high;
	} tsf_buf;

	s1 = dhd_get_htsf(dhd, 0);
	ret = dhd_iovar(&dhd->pub, ifidx, "tsf", NULL, 0, buf, sizeof(buf),
			FALSE);
	if (ret < 0) {
		if (ret == -EIO) {
			DHD_ERROR(("%s: tsf is not supported by device\n",
				dhd_ifname(&dhd->pub, ifidx)));
			return -EOPNOTSUPP;
		}
		return ret;
	}
	s2 = dhd_get_htsf(dhd, 0);

	memcpy(&tsf_buf, buf, sizeof(tsf_buf));
	printf(" TSF_h=%04X lo=%08X Calc:htsf=%08X, coef=%d.%d%d delta=%d ",
		tsf_buf.high, tsf_buf.low, s2, dhd->htsf.coef, dhd->htsf.coefdec1,
		dhd->htsf.coefdec2, s2-tsf_buf.low);
	printf("lasttsf=%08X lastcycle=%08X\n", dhd->htsf.last_tsf, dhd->htsf.last_cycle);
	return 0;
}

void htsf_update(dhd_info_t *dhd, void *data)
{
	static ulong  cur_cycle = 0, prev_cycle = 0;
	uint32 htsf, tsf_delta = 0;
	uint32 hfactor = 0, cyc_delta, dec1 = 0, dec2, dec3, tmp;
	ulong b, a;
	cycles_t t;

	/* cycles_t in inlcude/mips/timex.h */

	t = get_cycles();

	prev_cycle = cur_cycle;
	cur_cycle = t;

	if (cur_cycle > prev_cycle)
		cyc_delta = cur_cycle - prev_cycle;
	else {
		b = cur_cycle;
		a = prev_cycle;
		cyc_delta = cur_cycle + (0xFFFFFFFF - prev_cycle);
	}

	if (data == NULL)
		printf(" tsf update ata point er is null \n");

	memcpy(&prev_tsf, &cur_tsf, sizeof(tsf_t));
	memcpy(&cur_tsf, data, sizeof(tsf_t));

	if (cur_tsf.low == 0) {
		DHD_INFO((" ---- 0 TSF, do not update, return\n"));
		return;
	}

	if (cur_tsf.low > prev_tsf.low)
		tsf_delta = (cur_tsf.low - prev_tsf.low);
	else {
		DHD_INFO((" ---- tsf low is smaller cur_tsf= %08X, prev_tsf=%08X, \n",
		 cur_tsf.low, prev_tsf.low));
		if (cur_tsf.high > prev_tsf.high) {
			tsf_delta = cur_tsf.low + (0xFFFFFFFF - prev_tsf.low);
			DHD_INFO((" ---- Wrap around tsf coutner  adjusted TSF=%08X\n", tsf_delta));
		}
		else
			return; /* do not update */
	}

	if (tsf_delta)  {
		hfactor = cyc_delta / tsf_delta;
		tmp  = 	(cyc_delta - (hfactor * tsf_delta))*10;
		dec1 =  tmp/tsf_delta;
		dec2 =  ((tmp - dec1*tsf_delta)*10) / tsf_delta;
		tmp  = 	(tmp   - (dec1*tsf_delta))*10;
		dec3 =  ((tmp - dec2*tsf_delta)*10) / tsf_delta;

		if (dec3 > 4) {
			if (dec2 == 9) {
				dec2 = 0;
				if (dec1 == 9) {
					dec1 = 0;
					hfactor++;
				}
				else {
					dec1++;
				}
			}
			else
				dec2++;
		}
	}

	if (hfactor) {
		htsf = ((cyc_delta * 10)  / (hfactor*10+dec1)) + prev_tsf.low;
		dhd->htsf.coef = hfactor;
		dhd->htsf.last_cycle = cur_cycle;
		dhd->htsf.last_tsf = cur_tsf.low;
		dhd->htsf.coefdec1 = dec1;
		dhd->htsf.coefdec2 = dec2;
	}
	else {
		htsf = prev_tsf.low;
	}
}

#endif /* WLMEDIA_HTSF */

#ifdef CUSTOM_SET_CPUCORE
void dhd_set_cpucore(dhd_pub_t *dhd, int set)
{
	int e_dpc = 0, e_rxf = 0, retry_set = 0;

	if (!(dhd->chan_isvht80)) {
		DHD_ERROR(("%s: chan_status(%d) cpucore!!!\n", __FUNCTION__, dhd->chan_isvht80));
		return;
	}

	if (DPC_CPUCORE) {
		do {
			if (set == TRUE) {
				e_dpc = set_cpus_allowed_ptr(dhd->current_dpc,
					cpumask_of(DPC_CPUCORE));
			} else {
				e_dpc = set_cpus_allowed_ptr(dhd->current_dpc,
					cpumask_of(PRIMARY_CPUCORE));
			}
			if (retry_set++ > MAX_RETRY_SET_CPUCORE) {
				DHD_ERROR(("%s: dpc(%d) invalid cpu!\n", __FUNCTION__, e_dpc));
				return;
			}
			if (e_dpc < 0)
				OSL_SLEEP(1);
		} while (e_dpc < 0);
	}
	if (RXF_CPUCORE) {
		do {
			if (set == TRUE) {
				e_rxf = set_cpus_allowed_ptr(dhd->current_rxf,
					cpumask_of(RXF_CPUCORE));
			} else {
				e_rxf = set_cpus_allowed_ptr(dhd->current_rxf,
					cpumask_of(PRIMARY_CPUCORE));
			}
			if (retry_set++ > MAX_RETRY_SET_CPUCORE) {
				DHD_ERROR(("%s: rxf(%d) invalid cpu!\n", __FUNCTION__, e_rxf));
				return;
			}
			if (e_rxf < 0)
				OSL_SLEEP(1);
		} while (e_rxf < 0);
	}
#ifdef DHD_OF_SUPPORT
	interrupt_set_cpucore(set);
#endif /* DHD_OF_SUPPORT */
	DHD_TRACE(("%s: set(%d) cpucore success!\n", __FUNCTION__, set));

	return;
}
#endif /* CUSTOM_SET_CPUCORE */

/* Get interface specific ap_isolate configuration */
int dhd_get_ap_isolate(dhd_pub_t *dhdp, uint32 idx)
{
	dhd_info_t *dhd = dhdp->info;
	dhd_if_t *ifp;

	ASSERT(idx < DHD_MAX_IFS);

	ifp = dhd->iflist[idx];

	return ifp->ap_isolate;
}

/* Set interface specific ap_isolate configuration */
int dhd_set_ap_isolate(dhd_pub_t *dhdp, uint32 idx, int val)
{
	dhd_info_t *dhd = dhdp->info;
	dhd_if_t *ifp;

	ASSERT(idx < DHD_MAX_IFS);

	ifp = dhd->iflist[idx];

	ifp->ap_isolate = val;

	return 0;
}

static void
dhd_mem_dump_to_file(void *handle, void *event_info, u8 event)
{
	dhd_info_t *dhd = handle;
	dhd_dump_t *dump = event_info;

	if (!dhd) {
		DHD_ERROR(("%s: dhd is NULL\n", __FUNCTION__));
		return;
	}

	if (!dump) {
		DHD_ERROR(("%s: dump is NULL\n", __FUNCTION__));
		return;
	}

	if (dhd->pub.memdump_enabled == DUMP_MEMFILE) {
		write_to_file(&dhd->pub, dump->buf, dump->bufsize);
		DHD_ERROR(("%s: writing SoC_RAM dump to the file failed\n", __FUNCTION__));
	}

	if (dhd->pub.memdump_enabled == DUMP_MEMFILE_BUGON) {
		BUG_ON(1);
	}
	MFREE(dhd->pub.osh, dump, sizeof(dhd_dump_t));
}

void dhd_schedule_memdump(dhd_pub_t *dhdp, uint8 *buf, uint32 size)
{
#ifdef HW_DEFERRED_WORK_BUGFIX
	int ret = 0;
#endif
	dhd_dump_t *dump = NULL;
	dump = (dhd_dump_t *)MALLOC(dhdp->osh, sizeof(dhd_dump_t));
	if (dump == NULL) {
		DHD_ERROR(("%s: dhd dump memory allocation failed\n", __FUNCTION__));
		return;
	}
	dump->buf = buf;
	dump->bufsize = size;

#ifdef HW_DEFERRED_WORK_BUGFIX
	ret = dhd_deferred_schedule_work(dhdp->info->dhd_deferred_wq, (void *)dump,
			DHD_WQ_WORK_SOC_RAM_DUMP, dhd_mem_dump_to_file, DHD_WORK_PRIORITY_HIGH);
	if (ret)
		DHD_ERROR(("dhd_schedule_memdump failed\n"));
#else
	dhd_deferred_schedule_work(dhdp->info->dhd_deferred_wq, (void *)dump,
		DHD_WQ_WORK_SOC_RAM_DUMP, dhd_mem_dump_to_file, DHD_WORK_PRIORITY_HIGH);
#endif
}
int dhd_os_socram_dump(struct net_device *dev, uint32 *dump_size)
{
	int ret = BCME_OK;
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);
	dhd_pub_t *dhdp = &dhd->pub;

	if (dhdp->busstate == DHD_BUS_DOWN) {
		return BCME_ERROR;
	}
	ret = dhd_common_socram_dump(dhdp);
	if (ret == BCME_OK) {
		*dump_size = dhdp->soc_ram_length;
	}
	return ret;
}

int dhd_os_get_socram_dump(struct net_device *dev, char **buf, uint32 *size)
{
	int ret = BCME_OK;
	int orig_len = 0;
	dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);
	dhd_pub_t *dhdp = &dhd->pub;
	if (buf == NULL)
		return BCME_ERROR;
	orig_len = *size;
	if (dhdp->soc_ram) {
		if (orig_len >= dhdp->soc_ram_length) {
			memcpy(*buf, dhdp->soc_ram, dhdp->soc_ram_length);
			/* reset the storage of dump */
			memset(dhdp->soc_ram, 0, dhdp->soc_ram_length);
			*size = dhdp->soc_ram_length;
			dhdp->soc_ram_length = 0;
		} else {
			ret = BCME_BUFTOOSHORT;
			DHD_ERROR(("The length of the buffer is too short"
				" to save the memory dump with %d\n", dhdp->soc_ram_length));
		}
	} else {
		DHD_ERROR(("socram_dump is not ready to get\n"));
		ret = BCME_NOTREADY;
	}
	return ret;
}

int dhd_os_get_version(struct net_device *dev, bool dhd_ver, char **buf, uint32 size)
{
	int ret = BCME_OK;
	memset(*buf, 0, size);
	if (dhd_ver) {
		strncpy(*buf, dhd_version, size - 1);
	} else {
	    if(NULL != strstr(info_string, "Firmware: "))
		strncpy(*buf, strstr(info_string, "Firmware: "), size - 1);
	}
	return ret;
}

#ifdef DHD_WMF
/* Returns interface specific WMF configuration */
dhd_wmf_t* dhd_wmf_conf(dhd_pub_t *dhdp, uint32 idx)
{
	dhd_info_t *dhd = dhdp->info;
	dhd_if_t *ifp;

	ASSERT(idx < DHD_MAX_IFS);

	ifp = dhd->iflist[idx];
	return &ifp->wmf;
}
#endif /* DHD_WMF */
#ifdef DHD_TPUT_MONITOR
int dhd_irq_affinity_enable(struct net_device *net, int enable)
{
	dhd_info_t *dhd = DHD_DEV_INFO(net);

	dhdpcie_interrupt_set_cpucore(&dhd->pub, enable);

	return 0;
}
#endif /* DHD_TPUT_MONITOR */
/* ----------------------------------------------------------------------------
 * Infrastructure code for sysfs interface support for DHD
 *
 * What is sysfs interface?
 * https://www.kernel.org/doc/Documentation/filesystems/sysfs.txt
 *
 * Why sysfs interface?
 * This is the Linux standard way of changing/configuring Run Time parameters
 * for a driver. We can use this interface to control "linux" specific driver
 * parameters.
 *
 * -----------------------------------------------------------------------------
 */
#if defined(DHD_TRACE_WAKE_LOCK)
/* Function to show the history buffer */
static ssize_t
show_wklock_trace(struct dhd_info *dev, char *buf)
{
	ssize_t ret = 0;
	dhd_info_t *dhd = (dhd_info_t *)dev;

	buf[ret] = '\n';
	buf[ret+1] = 0;

	dhd_wk_lock_stats_dump(&dhd->pub);
	return ret+1;
}

/* Function to enable/disable wakelock trace */
static ssize_t
wklock_trace_onoff(struct dhd_info *dev, const char *buf, size_t count)
{
	unsigned long onoff;
	unsigned long flags;
	dhd_info_t *dhd = (dhd_info_t *)dev;

	onoff = bcm_strtoul(buf, NULL, 10);
	if (onoff != 0 && onoff != 1) {
		return -EINVAL;
	}

	spin_lock_irqsave(&dhd->wakelock_spinlock, flags);
	trace_wklock_onoff = onoff;
	spin_unlock_irqrestore(&dhd->wakelock_spinlock, flags);
	if (trace_wklock_onoff) {
		printk("ENABLE WAKLOCK TRACE\n");
	} else {
		printk("DISABLE WAKELOCK TRACE\n");
	}

	return (ssize_t)(onoff+1);
}

#if defined(HW_WATCHDOG_MS)
static int dhd_fw_log_event_enable(struct dhd_info *dev, bool enable)
{
       dhd_info_t *dhd = (dhd_info_t *)dev;
       dhd_pub_t *dhdp;
       char eventmask[WL_EVENTING_MASK_LEN];
       char iovbuf[WL_EVENTING_MASK_LEN + 12]; /*  Room for "event_msgs" + '\0' + bitvec  */
       int ret;

       dhdp = &dhd->pub;

       /* Read event_msgs mask */
       bcm_mkiovar("event_msgs", eventmask, WL_EVENTING_MASK_LEN, iovbuf, sizeof(iovbuf));
       if ((ret  = dhd_wl_ioctl_cmd(dhdp, WLC_GET_VAR, iovbuf, sizeof(iovbuf), FALSE, 0)) < 0) {
               DHD_ERROR(("%s read Event mask failed %d\n", __FUNCTION__, ret));
               return ret;
       }
       bcopy(iovbuf, eventmask, WL_EVENTING_MASK_LEN);

       if (enable) {
               setbit(eventmask, WLC_E_EXTLOG_MSG);
       } else {
               clrbit(eventmask, WLC_E_EXTLOG_MSG);
       }

       /* Write updated Event mask */
       bcm_mkiovar("event_msgs", eventmask, WL_EVENTING_MASK_LEN, iovbuf, sizeof(iovbuf));
       if ((ret = dhd_wl_ioctl_cmd(dhdp, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0)) < 0) {
               DHD_ERROR(("%s Set Event mask failed %d\n", __FUNCTION__, ret));
               return ret;
       }

       return 0;
}
unsigned long hw_watchdog_time = 0;
/* Function to show dhd_watchdog_time */
static ssize_t
show_watchdog_time(struct dhd_info *dev, char *buf)
{
	ssize_t ret = 0;
	if(NULL == dev || NULL == buf)
		return -EINVAL;

	DHD_ERROR(("%s: read dhd_watchdog_ms %d\n", __FUNCTION__, hw_watchdog_time));
	ret = sprintf(buf, "%d\n", hw_watchdog_time);

	return ret;
}
/* Function to enable/disable dhd_watchdog_ms */
#define time_200ms 200
static ssize_t
dhd_watchdog_time_write(struct dhd_info *dev, const char *buf, size_t count)
{
	unsigned long time;
	dhd_info_t *dhd = (dhd_info_t *)dev;
	//Null pointer judgment and avoid buf is too large
	if(NULL == dev || NULL == buf || NULL == dhd || strlen(buf) > 4 || dhd->pub.up == 0)
		return -EINVAL;

	DHD_ERROR(("%s: input buf %s \n", __FUNCTION__, buf));
	time = bcm_strtoul(buf, NULL, 10);
	//In order to think security problem, write dead 200 ms
	if (time > 0) {
#ifdef HW_TIMER
		DHD_ERROR(("%s: Open the firmware log and enable watchdog thread during 200ms\n", __FUNCTION__));
		dhd_watchdog_ms = time_200ms;
		dhd_console_ms = time_200ms;
		dhd_os_wd_timer(dhd, time_200ms);
#endif
		hw_watchdog_time = time_200ms;
		dhd_fw_log_event_enable(dev, TRUE);
		DHD_ERROR(("%s: Enable firmware log report full event function! \n", __FUNCTION__));

	} else {
#ifdef HW_TIMER
		dhd_watchdog_ms= 0;
		dhd_console_ms= 0;
		dhd_os_wd_timer(dhd, 0);
		DHD_ERROR(("%s: Close the firmware log and disable watchdog thread\n", __FUNCTION__));
#endif
		hw_watchdog_time = 0;
		dhd_fw_log_event_enable(dev, FALSE);
		DHD_ERROR(("%s: Disable firmware log report full event function! \n", __FUNCTION__));
	}

	return (ssize_t)(time+1);
}
#endif

/*
 * Generic Attribute Structure for DHD.
 * If we have to add a new sysfs entry under /sys/bcm-dhd/, we have
 * to instantiate an object of type dhd_attr,  populate it with
 * the required show/store functions (ex:- dhd_attr_cpumask_primary)
 * and add the object to default_attrs[] array, that gets registered
 * to the kobject of dhd (named bcm-dhd).
 */

struct dhd_attr {
	struct attribute attr;
	ssize_t(*show)(struct dhd_info *, char *);
	ssize_t(*store)(struct dhd_info *, const char *, size_t count);
};

#if defined(DHD_TRACE_WAKE_LOCK)
static struct dhd_attr dhd_attr_wklock =
	__ATTR(wklock_trace, 0660, show_wklock_trace, wklock_trace_onoff);
#endif /* defined(DHD_TRACE_WAKE_LOCK */
#if defined(HW_WATCHDOG_MS)
static struct dhd_attr dhd_attr_watchdog =
	__ATTR(dhd_watchdog_time, 0660, show_watchdog_time, dhd_watchdog_time_write);
#endif

/* Attribute object that gets registered with "bcm-dhd" kobject tree */
static struct attribute *default_attrs[] = {
#if defined(DHD_TRACE_WAKE_LOCK)
	&dhd_attr_wklock.attr,
#endif
#if defined(HW_WATCHDOG_MS)
	&dhd_attr_watchdog.attr,
#endif
	NULL
};

#define to_dhd(k) container_of(k, struct dhd_info, dhd_kobj)
#define to_attr(a) container_of(a, struct dhd_attr, attr)

/*
 * bcm-dhd kobject show function, the "attr" attribute specifices to which
 * node under "bcm-dhd" the show function is called.
 */
static ssize_t dhd_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	dhd_info_t *dhd = to_dhd(kobj);
	struct dhd_attr *d_attr = to_attr(attr);
	int ret;

	if (d_attr->show)
		ret = d_attr->show(dhd, buf);
	else
		ret = -EIO;

	return ret;
}


/*
 * bcm-dhd kobject show function, the "attr" attribute specifices to which
 * node under "bcm-dhd" the store function is called.
 */
static ssize_t dhd_store(struct kobject *kobj, struct attribute *attr,
	const char *buf, size_t count)
{
	dhd_info_t *dhd = to_dhd(kobj);
	struct dhd_attr *d_attr = to_attr(attr);
	int ret;

	if (d_attr->store)
		ret = d_attr->store(dhd, buf, count);
	else
		ret = -EIO;

	return ret;

}

static struct sysfs_ops dhd_sysfs_ops = {
	.show = dhd_show,
	.store = dhd_store,
};

static struct kobj_type dhd_ktype = {
	.sysfs_ops = &dhd_sysfs_ops,
	.default_attrs = default_attrs,
};

/* Create a kobject and attach to sysfs interface */
static int dhd_sysfs_init(dhd_info_t *dhd)
{
	int ret = -1;

	if (dhd == NULL) {
		DHD_ERROR(("%s(): dhd is NULL \r\n", __FUNCTION__));
		return ret;
	}

	/* Initialize the kobject */
	ret = kobject_init_and_add(&dhd->dhd_kobj, &dhd_ktype, NULL, "bcm-dhd");
	if (ret) {
		kobject_put(&dhd->dhd_kobj);
		DHD_ERROR(("%s(): Unable to allocate kobject \r\n", __FUNCTION__));
		return ret;
	}

	/*
	 * We are always responsible for sending the uevent that the kobject
	 * was added to the system.
	 */
	kobject_uevent(&dhd->dhd_kobj, KOBJ_ADD);

	return ret;
}

/* Done with the kobject and detach the sysfs interface */
static void dhd_sysfs_exit(dhd_info_t *dhd)
{
	if (dhd == NULL) {
		DHD_ERROR(("%s(): dhd is NULL \r\n", __FUNCTION__));
		return;
	}

	/* Releae the kobject */
	kobject_put(&dhd->dhd_kobj);
}
#endif /* DHD_TRACE_WAKE_LOCK */

#ifdef DHD_UNICAST_DHCP
static int
dhd_get_pkt_ether_type(dhd_pub_t *pub, void *pktbuf,
	uint8 **data_ptr, int *len_ptr, uint16 *et_ptr, bool *snap_ptr)
{
	uint8 *frame = PKTDATA(pub->osh, pktbuf);
	int length = PKTLEN(pub->osh, pktbuf);
	uint8 *pt;			/* Pointer to type field */
	uint16 ethertype;
	bool snap = FALSE;
	/* Process Ethernet II or SNAP-encapsulated 802.3 frames */
	if (length < ETHER_HDR_LEN) {
		DHD_ERROR(("dhd: %s: short eth frame (%d)\n",
		           __FUNCTION__, length));
		return BCME_ERROR;
	} else if (ntoh16_ua(frame + ETHER_TYPE_OFFSET) >= ETHER_TYPE_MIN) {
		/* Frame is Ethernet II */
		pt = frame + ETHER_TYPE_OFFSET;
	} else if (length >= ETHER_HDR_LEN + SNAP_HDR_LEN + ETHER_TYPE_LEN &&
	           !bcmp(llc_snap_hdr, frame + ETHER_HDR_LEN, SNAP_HDR_LEN)) {
		pt = frame + ETHER_HDR_LEN + SNAP_HDR_LEN;
		snap = TRUE;
	} else {
		DHD_INFO(("DHD: %s: non-SNAP 802.3 frame\n",
		           __FUNCTION__));
		return BCME_ERROR;
	}

	ethertype = ntoh16_ua(pt);

	/* Skip VLAN tag, if any */
	if (ethertype == ETHER_TYPE_8021Q) {
		pt += VLAN_TAG_LEN;

		if ((pt + ETHER_TYPE_LEN) > (frame + length)) {
			DHD_ERROR(("dhd: %s: short VLAN frame (%d)\n",
			          __FUNCTION__, length));
			return BCME_ERROR;
		}

		ethertype = ntoh16_ua(pt);
	}

	*data_ptr = pt + ETHER_TYPE_LEN;
	*len_ptr = length - (pt + ETHER_TYPE_LEN - frame);
	*et_ptr = ethertype;
	*snap_ptr = snap;
	return BCME_OK;
}

static int
dhd_get_pkt_ip_type(dhd_pub_t *pub, void *pktbuf,
	uint8 **data_ptr, int *len_ptr, uint8 *prot_ptr)
{
	struct ipv4_hdr *iph;		/* IP frame pointer */
	int iplen;			/* IP frame length */
	uint16 ethertype, iphdrlen, ippktlen;
	uint16 iph_frag;
	uint8 prot;
	bool snap;

	if (dhd_get_pkt_ether_type(pub, pktbuf, (uint8 **)&iph,
	    &iplen, &ethertype, &snap) != 0)
		return BCME_ERROR;

	if (ethertype != ETHER_TYPE_IP) {
		return BCME_ERROR;
	}

	/* We support IPv4 only */
	if (iplen < IPV4_OPTIONS_OFFSET || (IP_VER(iph) != IP_VER_4)) {
		return BCME_ERROR;
	}

	/* Header length sanity */
	iphdrlen = IPV4_HLEN(iph);

	/*
	 * Packet length sanity; sometimes we receive eth-frame size bigger
	 * than the IP content, which results in a bad tcp chksum
	 */
	ippktlen = ntoh16(iph->tot_len);
	if (ippktlen < iplen) {

		DHD_INFO(("%s: extra frame length ignored\n",
		          __FUNCTION__));
		iplen = ippktlen;
	} else if (ippktlen > iplen) {
		DHD_ERROR(("dhd: %s: truncated IP packet (%d)\n",
		           __FUNCTION__, ippktlen - iplen));
		return BCME_ERROR;
	}

	if (iphdrlen < IPV4_OPTIONS_OFFSET || iphdrlen > iplen) {
		DHD_ERROR(("DHD: %s: IP-header-len (%d) out of range (%d-%d)\n",
		           __FUNCTION__, iphdrlen, IPV4_OPTIONS_OFFSET, iplen));
		return BCME_ERROR;
	}

	/*
	 * We don't handle fragmented IP packets.  A first frag is indicated by the MF
	 * (more frag) bit and a subsequent frag is indicated by a non-zero frag offset.
	 */
	iph_frag = ntoh16(iph->frag);

	if ((iph_frag & IPV4_FRAG_MORE) || (iph_frag & IPV4_FRAG_OFFSET_MASK) != 0) {
		DHD_INFO(("DHD:%s: IP fragment not handled\n",
		           __FUNCTION__));
		return BCME_ERROR;
	}

	prot = IPV4_PROT(iph);

	*data_ptr = (((uint8 *)iph) + iphdrlen);
	*len_ptr = iplen - iphdrlen;
	*prot_ptr = prot;
	return BCME_OK;
}

/** check the packet type, if it is DHCP ACK/REPLY, convert into unicast packet	*/
static
int dhd_convert_dhcp_broadcast_ack_to_unicast(dhd_pub_t *pub, void *pktbuf, int ifidx)
{
	dhd_sta_t* stainfo;
	uint8 *eh = PKTDATA(pub->osh, pktbuf);
	uint8 *udph;
	uint8 *dhcp;
	uint8 *chaddr;
	int udpl;
	int dhcpl;
	uint16 port;
	uint8 prot;

	if (!ETHER_ISMULTI(eh + ETHER_DEST_OFFSET))
	    return BCME_ERROR;
	if (dhd_get_pkt_ip_type(pub, pktbuf, &udph, &udpl, &prot) != 0)
		return BCME_ERROR;
	if (prot != IP_PROT_UDP)
		return BCME_ERROR;
	/* check frame length, at least UDP_HDR_LEN */
	if (udpl < UDP_HDR_LEN) {
		DHD_ERROR(("DHD: %s: short UDP frame, ignored\n",
		    __FUNCTION__));
		return BCME_ERROR;
	}
	port = ntoh16_ua(udph + UDP_DEST_PORT_OFFSET);
	/* only process DHCP packets from server to client */
	if (port != DHCP_PORT_CLIENT)
		return BCME_ERROR;

	dhcp = udph + UDP_HDR_LEN;
	dhcpl = udpl - UDP_HDR_LEN;

	if (dhcpl < DHCP_CHADDR_OFFSET + ETHER_ADDR_LEN) {
		DHD_ERROR(("DHD: %s: short DHCP frame, ignored\n",
		    __FUNCTION__));
		return BCME_ERROR;
	}
	/* only process DHCP reply(offer/ack) packets */
	if (*(dhcp + DHCP_TYPE_OFFSET) != DHCP_TYPE_REPLY)
		return BCME_ERROR;
	chaddr = dhcp + DHCP_CHADDR_OFFSET;
	stainfo = dhd_find_sta(pub, ifidx, chaddr);
	if (stainfo) {
		bcopy(chaddr, eh + ETHER_DEST_OFFSET, ETHER_ADDR_LEN);
		return BCME_OK;
	}
	return BCME_ERROR;
}
#endif /* DHD_UNICAST_DHD */

#ifdef DHD_PKTID_AUDIT_ENABLED
void
dhd_pktid_audit_fail_cb(dhd_pub_t *dhdp)
{
	DHD_ERROR(("%s: Got Pkt Id Audit failure \n", __FUNCTION__));
}
#endif /* DHD_PKTID_AUDIT_ENABLED */

#ifdef DHD_L2_FILTER
/* Check if packet type is ICMP ECHO */
static
int dhd_l2_filter_block_ping(dhd_pub_t *pub, void *pktbuf, int ifidx)
{
	struct bcmicmp_hdr *icmph;
	int udpl;
	uint8 prot;

	if (dhd_get_pkt_ip_type(pub, pktbuf, (uint8 **)&icmph, &udpl, &prot) != 0)
		return BCME_ERROR;
	if (prot == IP_PROT_ICMP) {
		if (icmph->type == ICMP_TYPE_ECHO_REQUEST)
			return BCME_OK;
	}
	return BCME_ERROR;
}
#endif /* DHD_L2_FILTER */
struct net_device *
dhd_linux_get_primary_netdev(dhd_pub_t *dhdp)
{
	dhd_info_t *dhd = dhdp->info;

	if (dhd->iflist[0] && dhd->iflist[0]->net)
		return dhd->iflist[0]->net;
	else
		return NULL;
}

#ifdef HW_WIFI_SHUTDOWN
#ifdef PCIE_FULL_DONGLE
extern void dhdpcie_shutdown(dhd_pub_t *dhd_pub); 
#endif
void wifi_plat_dev_drv_shutdown(struct platform_device *pdev)
{
    dhd_pub_t *dhd_pub = NULL;
    dhd_info_t *dhd_info = NULL;
    dhd_if_t *dhd_if = NULL;

    DHD_ERROR(("##> %s\n", __FUNCTION__));
    dhd_pub = wl_cfg80211_get_dhdp();

    if (dhd_os_check_if_up(dhd_pub)){
        /*
         * here not close wifi, only disable interrupt, or it will cause panic sometime
         * because of not synchronization with supplicant
         */
#ifndef BCMPCIE
        dhd_bus_suspend(dhd_pub);
#else
#ifdef PCIE_FULL_DONGLE
		dhdpcie_shutdown(dhd_pub);
#endif
	dhd_info = (dhd_info_t *)dhd_pub->info;
	dhd_if = dhd_info->iflist[0];
	ASSERT(dhd_if);
	ASSERT(dhd_if->net);
	if (dhd_if && dhd_if->net) {
 		dhd_stop(dhd_if->net);
	}
#endif
    }
}
#endif


dhd_pub_t *hw_get_dhd_pub(struct net_device *dev) {
	if(dev) {
		dhd_info_t *dhd = *(dhd_info_t **)netdev_priv(dev);

		if (dhd)
			return &(dhd->pub);
	}
	return NULL;
}

#ifdef HW_DOZE_PKT_FILTER

static char huawei_pktfilter[HW_PKT_FILTER_MAX_NUM][HW_PKT_FILTER_LEN];

int net_hw_set_filter_enable(dhd_pub_t *pub, int on) {
    int idx;

    DHD_ERROR(("%s: enter\n", __FUNCTION__));
#ifdef HW_WIFI_FILTER_WIFI_LOCK
    DHD_OS_WAKE_LOCK(pub);
#endif
    for (idx = HW_PKT_FILTER_MIN_IDX; idx < HW_PKT_FILTER_MAX_IDX; idx++) {
        if(pub->pktfilter[idx] != NULL)
            dhd_pktfilter_offload_enable(pub, pub->pktfilter[idx], on, dhd_master_mode);
    }
#ifdef HW_WIFI_FILTER_WIFI_LOCK
    DHD_OS_WAKE_UNLOCK(pub);
#endif
    return 0;
}

int net_hw_add_filter_items(dhd_pub_t *pub, hw_wifi_filter_item *items, int count) {
    int i, id, idx, on = 1;
    char *filterp = NULL;

    DHD_ERROR(("%s: enter\n", __FUNCTION__));
    if (count > HW_PKT_FILTER_MAX_NUM)
        return -EINVAL;

    // pktfilter_count is for default filter set, should below hw filter id
    if (pub->pktfilter_count >= HW_PKT_FILTER_MIN_IDX) {
        DHD_ERROR(("%s: too much whitelist pkt filter\n", __FUNCTION__));
        return -EINVAL;
    }

    //clear previous filters
    //net_hw_clear_filters(pub);
#ifdef HW_WIFI_FILTER_WIFI_LOCK
    DHD_OS_WAKE_LOCK(pub);
#endif
    for (i = 0; i < count; i++) {
        id = HW_PKT_FILTER_ID_BASE + i;
        idx = HW_PKT_FILTER_MIN_IDX + i;

        if (NULL == pub->pktfilter[idx] || 0 == items->filter_cnt) {
            if (NULL != pub->pktfilter[idx]) {
                dhd_pktfilter_offload_delete(pub, id);
                pub->pktfilter[idx] = NULL;
            }
            filterp =  huawei_pktfilter[i];
            memset(filterp, 0, HW_PKT_FILTER_LEN);

            snprintf(filterp, HW_PKT_FILTER_LEN, HW_FILTER_PATTERN,
                    id, items->protocol, (items->port & 0x00ff), ((items->port >> 8) & 0xff));
            DHD_ERROR(("%s\n", filterp));
            pub->pktfilter[idx] = filterp;
            dhd_pktfilter_offload_set(pub, pub->pktfilter[idx]);
            dhd_pktfilter_offload_enable(pub, pub->pktfilter[idx], on, dhd_master_mode);
        }
        items++;
    }
#ifdef HW_WIFI_FILTER_WIFI_LOCK
    DHD_OS_WAKE_UNLOCK(pub);
#endif
    return 0;
}

int net_hw_clear_filters(dhd_pub_t *pub) {
    int i, id;

    DHD_ERROR(("%s: enter\n", __FUNCTION__));
#ifdef HW_WIFI_FILTER_WIFI_LOCK
    DHD_OS_WAKE_LOCK(pub);
#endif
    for (i = HW_PKT_FILTER_MIN_IDX; i < HW_PKT_FILTER_MAX_IDX; i++) {
        id = HW_PKT_FILTER_ID_BASE + i - HW_PKT_FILTER_MIN_IDX;
        if (NULL != pub->pktfilter[i]) {
            dhd_pktfilter_offload_delete(pub, id);
            pub->pktfilter[i] = NULL;
        }
    }
#ifdef HW_WIFI_FILTER_WIFI_LOCK
    DHD_OS_WAKE_UNLOCK(pub);
#endif
    return 0;
}

#endif
