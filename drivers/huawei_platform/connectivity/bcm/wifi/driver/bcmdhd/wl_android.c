/*
 * Linux cfg80211 driver - Android related functions
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
 * $Id: wl_android.c 470703 2014-04-16 02:25:28Z $
 */

#include <linux/module.h>
#include <linux/netdevice.h>
#include <net/netlink.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#include <wl_android.h>
#include <wldev_common.h>
#include <wlioctl.h>
#include <bcmutils.h>
#include <linux_osl.h>
#include <dhd_dbg.h>
#include <dngl_stats.h>
#include <dhd.h>
#include <proto/bcmip.h>
#ifdef PNO_SUPPORT
#include <dhd_pno.h>
#endif
#ifdef BCMSDIO
#include <bcmsdbus.h>
#endif
#ifdef WL_CFG80211
#include <wl_cfg80211.h>
#endif
#include <hw_wifi.h>

#include <wl_dbg.h>
#define WL_ERROR(x) printk x
#define WL_TRACE(x)

/*
 * Android private command strings, PLEASE define new private commands here
 * so they can be updated easily in the future (if needed)
 */

#define CMD_START		"START"
#define CMD_STOP		"STOP"
#define CMD_RECONNECT		"RECONNECT"
#define	CMD_SCAN_ACTIVE		"SCAN-ACTIVE"
#define	CMD_SCAN_PASSIVE	"SCAN-PASSIVE"
#define CMD_RSSI		"RSSI"
#define CMD_LINKSPEED		"LINKSPEED"
#define CMD_RXFILTER_START	"RXFILTER-START"
#define CMD_RXFILTER_STOP	"RXFILTER-STOP"
#define CMD_RXFILTER_ADD	"RXFILTER-ADD"
#define CMD_RXFILTER_REMOVE	"RXFILTER-REMOVE"
#define CMD_BTCOEXSCAN_START	"BTCOEXSCAN-START"
#define CMD_BTCOEXSCAN_STOP	"BTCOEXSCAN-STOP"
#define CMD_BTCOEXMODE		"BTCOEXMODE"
#define CMD_SETSUSPENDOPT	"SETSUSPENDOPT"
#define CMD_SETSUSPENDMODE      "SETSUSPENDMODE"
#define CMD_P2P_DEV_ADDR	"P2P_DEV_ADDR"
#define CMD_SETFWPATH		"SETFWPATH"
#define CMD_SETBAND		"SETBAND"
#define CMD_GETBAND		"GETBAND"
#define CMD_COUNTRY		"COUNTRY"
#define CMD_P2P_SET_NOA		"P2P_SET_NOA"
#if !defined WL_ENABLE_P2P_IF
#define CMD_P2P_GET_NOA			"P2P_GET_NOA"
#endif /* WL_ENABLE_P2P_IF */
#define CMD_P2P_SD_OFFLOAD		"P2P_SD_"
#define CMD_P2P_SET_PS		"P2P_SET_PS"
#define CMD_SET_AP_WPS_P2P_IE 		"SET_AP_WPS_P2P_IE"
#define CMD_SETROAMMODE 	"SETROAMMODE"
#define CMD_SETIBSSBEACONOUIDATA	"SETIBSSBEACONOUIDATA"
#define CMD_MIRACAST		"MIRACAST"

#if defined(WL_SUPPORT_AUTO_CHANNEL)
#define CMD_GET_BEST_CHANNELS	"GET_BEST_CHANNELS"
#endif /* WL_SUPPORT_AUTO_CHANNEL */

#define CMD_KEEP_ALIVE		"KEEPALIVE"

/* CCX Private Commands */

#ifdef PNO_SUPPORT
#define CMD_PNOSSIDCLR_SET	"PNOSSIDCLR"
#define CMD_PNOSETUP_SET	"PNOSETUP "
#define CMD_PNOENABLE_SET	"PNOFORCE"
#define CMD_PNODEBUG_SET	"PNODEBUG"
#define CMD_WLS_BATCHING	"WLS_BATCHING"
#endif /* PNO_SUPPORT */

#define CMD_OKC_SET_PMK		"SET_PMK"
#define CMD_OKC_ENABLE		"OKC_ENABLE"

#define	CMD_HAPD_MAC_FILTER	"HAPD_MAC_FILTER"
#define CMD_DRV_BUS_STATUS      "DRV_BUS_STATUS"



#define CMD_ROAM_OFFLOAD			"SETROAMOFFLOAD"
#ifdef BCM_BLOCK_DATA_FRAME
#define CMD_SET_BLOCKFRAME			"SETBLOCKFRAME"
#endif

#ifdef CONFIG_HW_VOWIFI
#define CMD_GET_MODE	"VOWIFI_DETECT GET MODE"
#define CMD_SET_MODE	"VOWIFI_DETECT SET MODE"
#define CMD_GET_PERIOD	"VOWIFI_DETECT GET PERIOD"
#define CMD_SET_PERIOD	"VOWIFI_DETECT SET PERIOD"
#define CMD_GET_LOW_THRESHOLD	"VOWIFI_DETECT GET LOW_THRESHOLD"
#define CMD_SET_LOW_THRESHOLD	"VOWIFI_DETECT SET LOW_THRESHOLD"
#define CMD_GET_HIGH_THRESHOLD	"VOWIFI_DETECT GET HIGH_THRESHOLD"
#define CMD_SET_HIGH_THRESHOLD	"VOWIFI_DETECT SET HIGH_THRESHOLD"
#define CMD_GET_TRIGGER_COUNT 	"VOWIFI_DETECT GET TRIGGER_COUNT"
#define CMD_SET_TRIGGER_COUNT 	"VOWIFI_DETECT SET TRIGGER_COUNT"
#define VOWIFI_IS_SUPPORT   "VOWIFI_DETECT VOWIFI_IS_SUPPORT"

#define CMD_VOWIFI_MODE		"vowifi_mode"
#define CMD_VOWIFI_PERIOD   "vowifi_rssi_period"
#define CMD_VOWIFI_LOW_THRES	"vowifi_low_thres"
#define CMD_VOWIFI_HIGH_THRES	"vowifi_high_thres"
#define CMD_VOWIFI_TRIGGER_COUNT	"vowifi_trigger_count"
#endif

#ifdef HW_SET_PM
#define CMD_POWER_LOCK		"POWER_LOCK"
#endif

#ifdef BRCM_RSDB
#define CMD_CAPAB_RSDB      "GET_CAPAB_RSDB"
#endif

#ifdef HW_DOZE_PKT_FILTER
#define CMD_FILTER_SWITCH   "FILTER"
#endif

#ifndef HUAWEI_ANDROID_EXTENSION
#define HUAWEI_ANDROID_EXTENSION
#endif

#ifdef HUAWEI_ANDROID_EXTENSION
#define AUTO_RECOVERY_AFTER_ROAMING          0       //send disassoc after roaming
#define AUTO_RECOVERY_BEFORE_ASSOC_COMPLETE  1       //send disassoc before assoc_complete
#define AUTO_RECOVERY_BY_MANUAL              2       //send disasoc by manual

#define CMD_AP_SET_CFG "AP_SET_CFG"
#define CMD_AP_GET_STA_LIST "AP_GET_STA_LIST"
#define CMD_AP_SET_MAC_FLTR "AP_SET_MAC_FLTR"
#define CMD_AP_STA_DISASSOC "AP_STA_DISASSOC"
#define CMD_SET_TX_POWER "SET_TX_POWER"
#define CMD_SET_DISASSOC_ROAMING_BSSID "SET_DISASSOC_ROAMING_BSSID"
#endif

/* miracast related definition */
#define MIRACAST_MODE_OFF	0
#define MIRACAST_MODE_SOURCE	1
#define MIRACAST_MODE_SINK	2

#ifndef MIRACAST_AMPDU_SIZE
#define MIRACAST_AMPDU_SIZE	8
#endif

#ifndef MIRACAST_MCHAN_ALGO
#define MIRACAST_MCHAN_ALGO     1
#endif

#ifndef MIRACAST_MCHAN_BW
#define MIRACAST_MCHAN_BW       25
#endif

static LIST_HEAD(miracast_resume_list);
static u8 miracast_cur_mode;

struct io_cfg {
	s8 *iovar;
	s32 param;
	u32 ioctl;
	void *arg;
	u32 len;
	struct list_head list;
};

typedef struct _android_wifi_priv_cmd {
	char *buf;
	int used_len;
	int total_len;
} android_wifi_priv_cmd;

#ifdef CONFIG_COMPAT
typedef struct _compat_android_wifi_priv_cmd {
	compat_caddr_t buf;
	int used_len;
	int total_len;
} compat_android_wifi_priv_cmd;
#endif /* CONFIG_COMPAT */

#if defined(BCMFW_ROAM_ENABLE)
#define CMD_SET_ROAMPREF	"SET_ROAMPREF"

#define MAX_NUM_SUITES		10
#define WIDTH_AKM_SUITE		8
#define JOIN_PREF_RSSI_LEN		0x02
#define JOIN_PREF_RSSI_SIZE		4	/* RSSI pref header size in bytes */
#define JOIN_PREF_WPA_HDR_SIZE		4 /* WPA pref header size in bytes */
#define JOIN_PREF_WPA_TUPLE_SIZE	12	/* Tuple size in bytes */
#define JOIN_PREF_MAX_WPA_TUPLES	16
#define MAX_BUF_SIZE		(JOIN_PREF_RSSI_SIZE + JOIN_PREF_WPA_HDR_SIZE +	\
				           (JOIN_PREF_WPA_TUPLE_SIZE * JOIN_PREF_MAX_WPA_TUPLES))
#endif /* BCMFW_ROAM_ENABLE */


/**
 * Extern function declarations (TODO: move them to dhd_linux.h)
 */
int dhd_net_bus_devreset(struct net_device *dev, uint8 flag);
int dhd_dev_init_ioctl(struct net_device *dev);
#ifdef WL_CFG80211
int wl_cfg80211_get_p2p_dev_addr(struct net_device *net, struct ether_addr *p2pdev_addr);
int wl_cfg80211_set_btcoex_dhcp(struct net_device *dev, dhd_pub_t *dhd, char *command);
#else
int wl_cfg80211_get_p2p_dev_addr(struct net_device *net, struct ether_addr *p2pdev_addr)
{ return 0; }
int wl_cfg80211_set_p2p_noa(struct net_device *net, char* buf, int len)
{ return 0; }
int wl_cfg80211_get_p2p_noa(struct net_device *net, char* buf, int len)
{ return 0; }
int wl_cfg80211_set_p2p_ps(struct net_device *net, char* buf, int len)
{ return 0; }
#endif /* WK_CFG80211 */


#ifdef ENABLE_4335BT_WAR
extern int bcm_bt_lock(int cookie);
extern void bcm_bt_unlock(int cookie);
static int lock_cookie_wifi = 'W' | 'i'<<8 | 'F'<<16 | 'i'<<24;	/* cookie is "WiFi" */
#endif /* ENABLE_4335BT_WAR */

extern bool ap_fw_loaded;
#if defined(CUSTOMER_HW2)
extern char iface_name[IFNAMSIZ];
#endif

/**
 * Local (static) functions and variables
 */

/* Initialize g_wifi_on to 1 so dhd_bus_start will be called for the first
 * time (only) in dhd_open, subsequential wifi on will be handled by
 * wl_android_wifi_on
 */
int g_wifi_on = TRUE;

/**
 * Local (static) function definitions
 */
static int wl_android_get_link_speed(struct net_device *net, char *command, int total_len)
{
	int link_speed;
	int bytes_written;
	int error;

	error = wldev_get_link_speed(net, &link_speed);
	if (error)
		return -1;

	/* Convert Kbps to Android Mbps */
	link_speed = link_speed / 1000;
	bytes_written = snprintf(command, total_len, "LinkSpeed %d", link_speed);
	DHD_INFO(("%s: command result is %s\n", __FUNCTION__, command));
	return bytes_written;
}

static int wl_android_get_rssi(struct net_device *net, char *command, int total_len)
{
	wlc_ssid_t ssid = {0};
	int rssi;
	int bytes_written = 0;
	int error;

	error = wldev_get_rssi(net, &rssi);
	if (error)
		return -1;

	error = wldev_get_ssid(net, &ssid);
	if (error)
		return -1;
	if ((ssid.SSID_len == 0) || (ssid.SSID_len > DOT11_MAX_SSID_LEN)) {
		DHD_ERROR(("%s: wldev_get_ssid failed\n", __FUNCTION__));
#ifdef BCM_PATCH_2016_12_2017_01
	} else if (total_len <= (int)ssid.SSID_len) {
		return -ENOMEM;
#endif
	} else {
		memcpy(command, ssid.SSID, ssid.SSID_len);
		bytes_written = ssid.SSID_len;
	}
#ifdef BCM_PATCH_2016_12_2017_01
	if ((total_len - bytes_written) < (int)(strlen(" rssi -XXX") + 1))
		return -ENOMEM;
	bytes_written += snprintf(&command[bytes_written],
		total_len - bytes_written, " rssi %d", rssi);
	command[bytes_written] = '\0';
#else
	bytes_written += snprintf(&command[bytes_written], total_len, " rssi %d", rssi);
#endif

	DHD_INFO(("%s: command result is %s (%d)\n", __FUNCTION__, command, bytes_written));
	return bytes_written;
}

static int wl_android_set_suspendopt(struct net_device *dev, char *command, int total_len)
{
	int suspend_flag;
	int ret_now;
	int ret = 0;

		suspend_flag = *(command + strlen(CMD_SETSUSPENDOPT) + 1) - '0';

		if (suspend_flag != 0)
			suspend_flag = 1;
		ret_now = net_os_set_suspend_disable(dev, suspend_flag);

		if (ret_now != suspend_flag) {
			if (!(ret = net_os_set_suspend(dev, ret_now, 1)))
				DHD_INFO(("%s: Suspend Flag %d -> %d\n",
					__FUNCTION__, ret_now, suspend_flag));
			else
				DHD_ERROR(("%s: failed %d\n", __FUNCTION__, ret));
		}
	return ret;
}

static int wl_android_set_suspendmode(struct net_device *dev, char *command, int total_len)
{
	int ret = 0;

#if !defined(CONFIG_HAS_EARLYSUSPEND) || !defined(DHD_USE_EARLYSUSPEND)
	int suspend_flag;

	suspend_flag = *(command + strlen(CMD_SETSUSPENDMODE) + 1) - '0';
	if (suspend_flag != 0)
		suspend_flag = 1;

	if (!(ret = net_os_set_suspend(dev, suspend_flag, 0)))
		DHD_INFO(("%s: Suspend Mode %d\n", __FUNCTION__, suspend_flag));
	else
		DHD_ERROR(("%s: failed %d\n", __FUNCTION__, ret));
#endif

	return ret;
}

#ifdef HW_DOZE_PKT_FILTER
#define IS_NUMBER(c) ((c) >= '0' && (c) <= '9')
static int wl_android_set_doze_filter(struct net_device *dev, char *command)
{
	int ret = -1;
	int enable;
	char c;
	DHD_ERROR(("cmd:%s\n",command));
	if (NULL == command) {
		DHD_ERROR(("doze filer cmd is null"));
		return ret;
	}
	/* format:FILTER 1*/
	c = *(command + strlen(CMD_FILTER_SWITCH) + 1);

	if (!IS_NUMBER(c)) {
		DHD_ERROR(("doze filer cmd is error"));
		return ret;
	}
	enable = c - '0';

	ret = hw_set_net_filter_enable(enable);
	return ret;
}
#endif

static int wl_android_get_band(struct net_device *dev, char *command, int total_len)
{
	uint band;
	int bytes_written;
	int error;

	error = wldev_get_band(dev, &band);
	if (error)
		return -1;
	bytes_written = snprintf(command, total_len, "Band %d", band);
	return bytes_written;
}


#ifdef PNO_SUPPORT
#define PNO_PARAM_SIZE 50
#define VALUE_SIZE 50
#define LIMIT_STR_FMT  ("%49s %49s")
static int
wls_parse_batching_cmd(struct net_device *dev, char *command, int total_len)
{
	int err = BCME_OK;
	uint i, tokens;
	char *pos, *pos2, *token, *token2, *delim;
	char param[PNO_PARAM_SIZE], value[VALUE_SIZE];
	struct dhd_pno_batch_params batch_params;
	DHD_PNO(("%s: command=%s, len=%d\n", __FUNCTION__, command, total_len));
	if (total_len < strlen(CMD_WLS_BATCHING)) {
		DHD_ERROR(("%s argument=%d less min size\n", __FUNCTION__, total_len));
		err = BCME_ERROR;
		goto exit;
	}
	pos = command + strlen(CMD_WLS_BATCHING) + 1;
	memset(&batch_params, 0, sizeof(struct dhd_pno_batch_params));

	if (!strncmp(pos, PNO_BATCHING_SET, strlen(PNO_BATCHING_SET))) {
		pos += strlen(PNO_BATCHING_SET) + 1;
		while ((token = strsep(&pos, PNO_PARAMS_DELIMETER)) != NULL) {
			memset(param, 0, sizeof(param));
			memset(value, 0, sizeof(value));
			if (token == NULL || !*token)
				break;
			if (*token == '\0')
				continue;
			delim = strchr(token, PNO_PARAM_VALUE_DELLIMETER);
			if (delim != NULL)
				*delim = ' ';

			tokens = sscanf(token, LIMIT_STR_FMT, param, value);
			if (!strncmp(param, PNO_PARAM_SCANFREQ, strlen(PNO_PARAM_SCANFREQ))) {
				batch_params.scan_fr = simple_strtol(value, NULL, 0);
				DHD_PNO(("scan_freq : %d\n", batch_params.scan_fr));
			} else if (!strncmp(param, PNO_PARAM_BESTN, strlen(PNO_PARAM_BESTN))) {
				batch_params.bestn = simple_strtol(value, NULL, 0);
				DHD_PNO(("bestn : %d\n", batch_params.bestn));
			} else if (!strncmp(param, PNO_PARAM_MSCAN, strlen(PNO_PARAM_MSCAN))) {
				batch_params.mscan = simple_strtol(value, NULL, 0);
				DHD_PNO(("mscan : %d\n", batch_params.mscan));
			} else if (!strncmp(param, PNO_PARAM_CHANNEL, strlen(PNO_PARAM_CHANNEL))) {
				i = 0;
				pos2 = value;
				tokens = sscanf(value, "<%s>", value);
				if (tokens != 1) {
					err = BCME_ERROR;
					DHD_ERROR(("%s : invalid format for channel"
					" <> params\n", __FUNCTION__));
					goto exit;
				}
				while ((token2 = strsep(&pos2,
					PNO_PARAM_CHANNEL_DELIMETER)) != NULL) {
					if (token2 == NULL || !*token2)
						break;
					if (*token2 == '\0')
						continue;
					if (*token2 == 'A' || *token2 == 'B') {
						batch_params.band = (*token2 == 'A')?
							WLC_BAND_5G : WLC_BAND_2G;
						DHD_PNO(("band : %s\n",
							(*token2 == 'A')? "A" : "B"));
					} else {
#ifdef BCM_PATCH_CVE_2016_3869
						if ((batch_params.nchan >= WL_NUMCHANNELS) ||
						    (i >= WL_NUMCHANNELS)) {
							DHD_ERROR(("Too many nchan %d\n",
								batch_params.nchan));
							err = BCME_BUFTOOSHORT;
							goto exit;
						}
#endif
						batch_params.chan_list[i++] =
							simple_strtol(token2, NULL, 0);
						batch_params.nchan++;
						DHD_PNO(("channel :%d\n",
							batch_params.chan_list[i-1]));
					}
				 }
			} else if (!strncmp(param, PNO_PARAM_RTT, strlen(PNO_PARAM_RTT))) {
				batch_params.rtt = simple_strtol(value, NULL, 0);
				DHD_PNO(("rtt : %d\n", batch_params.rtt));
			} else {
				DHD_ERROR(("%s : unknown param: %s\n", __FUNCTION__, param));
				err = BCME_ERROR;
				goto exit;
			}
		}
		err = dhd_dev_pno_set_for_batch(dev, &batch_params);
		if (err < 0) {
			DHD_ERROR(("failed to configure batch scan\n"));
		} else {
			memset(command, 0, total_len);
			err = sprintf(command, "%d", err);
		}
	} else if (!strncmp(pos, PNO_BATCHING_GET, strlen(PNO_BATCHING_GET))) {
		err = dhd_dev_pno_get_for_batch(dev, command, total_len);
		if (err < 0) {
			DHD_ERROR(("failed to getting batching results\n"));
		} else {
			err = strlen(command);
		}
	} else if (!strncmp(pos, PNO_BATCHING_STOP, strlen(PNO_BATCHING_STOP))) {
		err = dhd_dev_pno_stop_for_batch(dev);
		if (err < 0) {
			DHD_ERROR(("failed to stop batching scan\n"));
		} else {
			memset(command, 0, total_len);
			err = sprintf(command, "OK");
		}
	} else {
		DHD_ERROR(("%s : unknown command\n", __FUNCTION__));
		err = BCME_ERROR;
		goto exit;
	}
exit:
	return err;
}
#ifndef WL_SCHED_SCAN
static int wl_android_set_pno_setup(struct net_device *dev, char *command, int total_len)
{
	wlc_ssid_ext_t ssids_local[MAX_PFN_LIST_COUNT];
	int res = -1;
	int nssid = 0;
	cmd_tlv_t *cmd_tlv_temp;
	char *str_ptr;
	int tlv_size_left;
	int pno_time = 0;
	int pno_repeat = 0;
	int pno_freq_expo_max = 0;

#ifdef PNO_SET_DEBUG
	int i;
	char pno_in_example[] = {
		'P', 'N', 'O', 'S', 'E', 'T', 'U', 'P', ' ',
		'S', '1', '2', '0',
		'S',
		0x05,
		'd', 'l', 'i', 'n', 'k',
		'S',
		0x04,
		'G', 'O', 'O', 'G',
		'T',
		'0', 'B',
		'R',
		'2',
		'M',
		'2',
		0x00
		};
#endif /* PNO_SET_DEBUG */
	DHD_PNO(("%s: command=%s, len=%d\n", __FUNCTION__, command, total_len));

	if (total_len < (strlen(CMD_PNOSETUP_SET) + sizeof(cmd_tlv_t))) {
		DHD_ERROR(("%s argument=%d less min size\n", __FUNCTION__, total_len));
		goto exit_proc;
	}
#ifdef PNO_SET_DEBUG
	memcpy(command, pno_in_example, sizeof(pno_in_example));
	total_len = sizeof(pno_in_example);
#endif
	str_ptr = command + strlen(CMD_PNOSETUP_SET);
	tlv_size_left = total_len - strlen(CMD_PNOSETUP_SET);

	cmd_tlv_temp = (cmd_tlv_t *)str_ptr;
	memset(ssids_local, 0, sizeof(ssids_local));

	if ((cmd_tlv_temp->prefix == PNO_TLV_PREFIX) &&
		(cmd_tlv_temp->version == PNO_TLV_VERSION) &&
		(cmd_tlv_temp->subtype == PNO_TLV_SUBTYPE_LEGACY_PNO)) {

		str_ptr += sizeof(cmd_tlv_t);
		tlv_size_left -= sizeof(cmd_tlv_t);

		if ((nssid = wl_iw_parse_ssid_list_tlv(&str_ptr, ssids_local,
			MAX_PFN_LIST_COUNT, &tlv_size_left)) <= 0) {
			DHD_ERROR(("SSID is not presented or corrupted ret=%d\n", nssid));
			goto exit_proc;
		} else {
			if ((str_ptr[0] != PNO_TLV_TYPE_TIME) || (tlv_size_left <= 1)) {
				DHD_ERROR(("%s scan duration corrupted field size %d\n",
					__FUNCTION__, tlv_size_left));
				goto exit_proc;
			}
			str_ptr++;
			pno_time = simple_strtoul(str_ptr, &str_ptr, 16);
			DHD_PNO(("%s: pno_time=%d\n", __FUNCTION__, pno_time));

			if (str_ptr[0] != 0) {
				if ((str_ptr[0] != PNO_TLV_FREQ_REPEAT)) {
					DHD_ERROR(("%s pno repeat : corrupted field\n",
						__FUNCTION__));
					goto exit_proc;
				}
				str_ptr++;
				pno_repeat = simple_strtoul(str_ptr, &str_ptr, 16);
				DHD_PNO(("%s :got pno_repeat=%d\n", __FUNCTION__, pno_repeat));
				if (str_ptr[0] != PNO_TLV_FREQ_EXPO_MAX) {
					DHD_ERROR(("%s FREQ_EXPO_MAX corrupted field size\n",
						__FUNCTION__));
					goto exit_proc;
				}
				str_ptr++;
				pno_freq_expo_max = simple_strtoul(str_ptr, &str_ptr, 16);
				DHD_PNO(("%s: pno_freq_expo_max=%d\n",
					__FUNCTION__, pno_freq_expo_max));
			}
		}
	} else {
		DHD_ERROR(("%s get wrong TLV command\n", __FUNCTION__));
		goto exit_proc;
	}

	res = dhd_dev_pno_set_for_ssid(dev, ssids_local, nssid, pno_time, pno_repeat,
		pno_freq_expo_max, NULL, 0);
exit_proc:
	return res;
}
#endif /* !WL_SCHED_SCAN */
#endif /* PNO_SUPPORT  */

static int wl_android_get_p2p_dev_addr(struct net_device *ndev, char *command, int total_len)
{
	int ret;
	int bytes_written = 0;

	ret = wl_cfg80211_get_p2p_dev_addr(ndev, (struct ether_addr*)command);
	if (ret)
		return 0;
	bytes_written = sizeof(struct ether_addr);
	return bytes_written;
}


int
wl_android_set_ap_mac_list(struct net_device *dev, int macmode, struct maclist *maclist)
{
	int i, j, match;
	int ret	= 0;
	char mac_buf[MAX_NUM_OF_ASSOCLIST *
		sizeof(struct ether_addr) + sizeof(uint)] = {0};
	struct maclist *assoc_maclist = (struct maclist *)mac_buf;

	/* set filtering mode */
	if ((ret = wldev_ioctl(dev, WLC_SET_MACMODE, &macmode, sizeof(macmode), true)) != 0) {
		DHD_ERROR(("%s : WLC_SET_MACMODE error=%d\n", __FUNCTION__, ret));
		return ret;
	}
	if (macmode != MACLIST_MODE_DISABLED) {
		/* set the MAC filter list */
		if ((ret = wldev_ioctl(dev, WLC_SET_MACLIST, maclist,
			sizeof(int) + sizeof(struct ether_addr) * maclist->count, true)) != 0) {
			DHD_ERROR(("%s : WLC_SET_MACLIST error=%d\n", __FUNCTION__, ret));
			return ret;
		}
		/* get the current list of associated STAs */
		assoc_maclist->count = MAX_NUM_OF_ASSOCLIST;
		if ((ret = wldev_ioctl(dev, WLC_GET_ASSOCLIST, assoc_maclist,
			sizeof(mac_buf), false)) != 0) {
			DHD_ERROR(("%s : WLC_GET_ASSOCLIST error=%d\n", __FUNCTION__, ret));
			return ret;
		}
		/* do we have any STA associated?  */
		if (assoc_maclist->count) {
			/* iterate each associated STA */
			for (i = 0; i < assoc_maclist->count; i++) {
				match = 0;
				/* compare with each entry */
				for (j = 0; j < maclist->count; j++) {
					DHD_INFO(("%s : associated="MACDBG " list="MACDBG "\n",
					__FUNCTION__, MAC2STRDBG(assoc_maclist->ea[i].octet),
					MAC2STRDBG(maclist->ea[j].octet)));
					if (memcmp(assoc_maclist->ea[i].octet,
						maclist->ea[j].octet, ETHER_ADDR_LEN) == 0) {
						match = 1;
						break;
					}
				}
				/* do conditional deauth */
				/*   "if not in the allow list" or "if in the deny list" */
				if ((macmode == MACLIST_MODE_ALLOW && !match) ||
					(macmode == MACLIST_MODE_DENY && match)) {
					scb_val_t scbval;

					scbval.val = htod32(1);
					memcpy(&scbval.ea, &assoc_maclist->ea[i],
						ETHER_ADDR_LEN);
					if ((ret = wldev_ioctl(dev,
						WLC_SCB_DEAUTHENTICATE_FOR_REASON,
						&scbval, sizeof(scb_val_t), true)) != 0)
						DHD_ERROR(("%s WLC_SCB_DEAUTHENTICATE error=%d\n",
							__FUNCTION__, ret));
				}
			}
		}
	}
	return ret;
}

/*
 * HAPD_MAC_FILTER mac_mode mac_cnt mac_addr1 mac_addr2
 *
 */
static int
wl_android_set_mac_address_filter(struct net_device *dev, const char* str)
{
	int i;
	int ret = 0;
	int macnum = 0;
	int macmode = MACLIST_MODE_DISABLED;
	struct maclist *list;
	char eabuf[ETHER_ADDR_STR_LEN];
#ifdef HW_WIFI_SECURITY_PATCH
	char *tstr = NULL;
#endif

	/* string should look like below (macmode/macnum/maclist) */
	/*   1 2 00:11:22:33:44:55 00:11:22:33:44:ff  */

	/* get the MAC filter mode */
	macmode = bcm_atoi(strsep((char**)&str, " "));

	if (macmode < MACLIST_MODE_DISABLED || macmode > MACLIST_MODE_ALLOW) {
		DHD_ERROR(("%s : invalid macmode %d\n", __FUNCTION__, macmode));
		return -1;
	}

	macnum = bcm_atoi(strsep((char**)&str, " "));
	if (macnum < 0 || macnum > MAX_NUM_MAC_FILT) {
		DHD_ERROR(("%s : invalid number of MAC address entries %d\n",
			__FUNCTION__, macnum));
		return -1;
	}
	/* allocate memory for the MAC list */
	list = (struct maclist*)kmalloc(sizeof(int) +
		sizeof(struct ether_addr) * macnum, GFP_KERNEL);
	if (!list) {
		DHD_ERROR(("%s : failed to allocate memory\n", __FUNCTION__));
		return -1;
	}
	/* prepare the MAC list */
	list->count = htod32(macnum);
	bzero((char *)eabuf, ETHER_ADDR_STR_LEN);
	for (i = 0; i < list->count; i++) {
#ifdef HW_WIFI_SECURITY_PATCH
		tstr = strsep((char**)&str, " ");
		if (NULL == tstr) {
			kfree(list);
			return -1;
		}
		strncpy(eabuf, tstr, ETHER_ADDR_STR_LEN - 1);
#endif
		if (!(ret = bcm_ether_atoe(eabuf, &list->ea[i]))) {
			DHD_ERROR(("%s : mac parsing err index=%d, addr=%s\n",
				__FUNCTION__, i, eabuf));
			list->count--;
			break;
		}
		DHD_INFO(("%s : %d/%d MACADDR=%s", __FUNCTION__, i, list->count, eabuf));
	}
	/* set the list */
	if ((ret = wl_android_set_ap_mac_list(dev, macmode, list)) != 0)
		DHD_ERROR(("%s : Setting MAC list failed error=%d\n", __FUNCTION__, ret));

	kfree(list);

	return 0;
}

/**
 * Global function definitions (declared in wl_android.h)
 */

int wl_android_wifi_on(struct net_device *dev)
{
	int ret = 0;
	int retry = POWERUP_MAX_RETRY;

	DHD_ERROR(("%s in\n", __FUNCTION__));
	if (!dev) {
		DHD_ERROR(("%s: dev is null\n", __FUNCTION__));
		return -EINVAL;
	}

	dhd_net_if_lock(dev);
	if (!g_wifi_on) {
		do {
#ifdef HW_WIFI_SDIO_LOWPOWER
			dhd_net_bus_power_up(dev);
#endif
			dhd_net_wifi_platform_set_power(dev, TRUE, WIFI_TURNON_DELAY);
#ifdef BCMSDIO
			ret = dhd_net_bus_resume(dev, 0);
#endif
			if (ret == 0)
				break;
			DHD_ERROR(("\nfailed to power up wifi chip, retry again (%d left) **\n\n",
				retry+1));
			dhd_net_wifi_platform_set_power(dev, FALSE, WIFI_TURNOFF_DELAY);
#ifdef HW_WIFI_SDIO_LOWPOWER
			dhd_net_bus_power_down(dev);
#endif
		} while (retry-- > 0);
		if (ret != 0) {
			DHD_ERROR(("\nfailed to power up wifi chip, max retry reached **\n\n"));
			goto exit;
		}
#if defined(BCMSDIO) || defined(BCMPCIE)
		ret = dhd_net_bus_devreset(dev, FALSE);
#ifdef BCMSDIO
		dhd_net_bus_resume(dev, 1);
#endif
#endif /* BCMSDIO || BCMPCIE */
#ifndef BCMPCIE
		if (!ret) {
			if (dhd_dev_init_ioctl(dev) < 0) {
#ifdef HW_WIFI_DMD_LOG
				hw_wifi_dsm_client_notify(DSM_WIFI_DHD_DEV_INIT_IOCTL_ERROR,
				           "dhd_dev_init_ioctl failed\n");
#endif
				ret = -EFAULT;
			}
		}
#endif
	if (!ret)
		g_wifi_on = TRUE;
	}

exit:
	dhd_net_if_unlock(dev);
	DHD_ERROR(("%s out with ret: %d\n", __FUNCTION__, ret));
	return ret;
}

int wl_android_wifi_off(struct net_device *dev, bool on_failure)
{
	int ret = 0;

	DHD_ERROR(("%s in\n", __FUNCTION__));
	if (!dev) {
		DHD_TRACE(("%s: dev is null\n", __FUNCTION__));
		return -EINVAL;
	}

	dhd_net_if_lock(dev);
	if (g_wifi_on || on_failure) {
#if defined(BCMSDIO) || defined(BCMPCIE)
		ret = dhd_net_bus_devreset(dev, TRUE);
#ifdef BCMSDIO
		dhd_net_bus_suspend(dev);
#endif
#endif /* BCMSDIO || BCMPCIE */
		dhd_net_wifi_platform_set_power(dev, FALSE, WIFI_TURNOFF_DELAY);
#ifdef HW_WIFI_SDIO_LOWPOWER
		dhd_net_bus_power_up(dev);
#endif
		g_wifi_on = FALSE;
	}
	dhd_net_if_unlock(dev);

	return ret;
}

static int wl_android_set_fwpath(struct net_device *net, char *command, int total_len)
{
	if ((strlen(command) - strlen(CMD_SETFWPATH)) > MOD_PARAM_PATHLEN)
		return -1;
	return dhd_net_set_fw_path(net, command + strlen(CMD_SETFWPATH) + 1);
}


static int
wl_android_set_pmk(struct net_device *dev, char *command, int total_len)
{
	uchar pmk[33];
	int error = 0;
	char smbuf[WLC_IOCTL_SMLEN];
#ifdef OKC_DEBUG
	int i = 0;
#endif

	bzero(pmk, sizeof(pmk));
	memcpy((char *)pmk, command + strlen("SET_PMK "), 32);
	error = wldev_iovar_setbuf(dev, "okc_info_pmk", pmk, 32, smbuf, sizeof(smbuf), NULL);
	if (error) {
		DHD_ERROR(("Failed to set PMK for OKC, error = %d\n", error));
	}
#ifdef OKC_DEBUG
	DHD_ERROR(("PMK is "));
	for (i = 0; i < 32; i++)
		DHD_ERROR(("%02X ", pmk[i]));

	DHD_ERROR(("\n"));
#endif
	return error;
}

static int
wl_android_okc_enable(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	char okc_enable = 0;

	okc_enable = command[strlen(CMD_OKC_ENABLE) + 1] - '0';
	error = wldev_iovar_setint(dev, "okc_enable", okc_enable);
	if (error) {
		DHD_ERROR(("Failed to %s OKC, error = %d\n",
			okc_enable ? "enable" : "disable", error));
	}

	wldev_iovar_setint(dev, "ccx_enable", 0);

	return error;
}



int wl_android_set_roam_mode(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int mode = 0;

	if (sscanf(command, "%*s %d", &mode) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = wldev_iovar_setint(dev, "roam_off", mode);
	if (error) {
		DHD_ERROR(("%s: Failed to set roaming Mode %d, error = %d\n",
		__FUNCTION__, mode, error));
		return -1;
	}
	else
		DHD_ERROR(("%s: succeeded to set roaming Mode %d, error = %d\n",
		__FUNCTION__, mode, error));
	return 0;
}

int wl_android_set_ibss_beacon_ouidata(struct net_device *dev, char *command, int total_len)
{
	char ie_buf[VNDR_IE_MAX_LEN];
	char *ioctl_buf = NULL;
	char hex[] = "XX";
	char *pcmd = NULL;
	int ielen = 0, datalen = 0, idx = 0, tot_len = 0;
	vndr_ie_setbuf_t *vndr_ie = NULL;
	s32 iecount;
	uint32 pktflag;
	u16 kflags = in_atomic() ? GFP_ATOMIC : GFP_KERNEL;
	s32 err = BCME_OK;

	/* Check the VSIE (Vendor Specific IE) which was added.
	 *  If exist then send IOVAR to delete it
	 */
	if (wl_cfg80211_ibss_vsie_delete(dev) != BCME_OK) {
		return -EINVAL;
	}

	pcmd = command + strlen(CMD_SETIBSSBEACONOUIDATA) + 1;
	for (idx = 0; idx < DOT11_OUI_LEN; idx++) {
		hex[0] = *pcmd++;
		hex[1] = *pcmd++;
		ie_buf[idx] =  (uint8)simple_strtoul(hex, NULL, 16);
	}
	pcmd++;
	while ((*pcmd != '\0') && (idx < VNDR_IE_MAX_LEN)) {
		hex[0] = *pcmd++;
		hex[1] = *pcmd++;
		ie_buf[idx++] =  (uint8)simple_strtoul(hex, NULL, 16);
		datalen++;
	}
	tot_len = sizeof(vndr_ie_setbuf_t) + (datalen - 1);
	vndr_ie = (vndr_ie_setbuf_t *) kzalloc(tot_len, kflags);
	if (!vndr_ie) {
		WL_ERR(("IE memory alloc failed\n"));
		return -ENOMEM;
	}
	/* Copy the vndr_ie SET command ("add"/"del") to the buffer */
	strncpy(vndr_ie->cmd, "add", VNDR_IE_CMD_LEN - 1);
	vndr_ie->cmd[VNDR_IE_CMD_LEN - 1] = '\0';

	/* Set the IE count - the buffer contains only 1 IE */
	iecount = htod32(1);
	memcpy((void *)&vndr_ie->vndr_ie_buffer.iecount, &iecount, sizeof(s32));

	/* Set packet flag to indicate that BEACON's will contain this IE */
	pktflag = htod32(VNDR_IE_BEACON_FLAG | VNDR_IE_PRBRSP_FLAG);
	memcpy((void *)&vndr_ie->vndr_ie_buffer.vndr_ie_list[0].pktflag, &pktflag,
		sizeof(u32));
	/* Set the IE ID */
	vndr_ie->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.id = (uchar) DOT11_MNG_PROPR_ID;

	memcpy(&vndr_ie->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.oui, &ie_buf,
		DOT11_OUI_LEN);
	memcpy(&vndr_ie->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.data,
		&ie_buf[DOT11_OUI_LEN], datalen);

	ielen = DOT11_OUI_LEN + datalen;
	vndr_ie->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.len = (uchar) ielen;

	ioctl_buf = kmalloc(WLC_IOCTL_MEDLEN, GFP_KERNEL);
	if (!ioctl_buf) {
		WL_ERR(("ioctl memory alloc failed\n"));
		if (vndr_ie) {
			kfree(vndr_ie);
		}
		return -ENOMEM;
	}
	memset(ioctl_buf, 0, WLC_IOCTL_MEDLEN);	/* init the buffer */
	err = wldev_iovar_setbuf(dev, "ie", vndr_ie, tot_len, ioctl_buf, WLC_IOCTL_MEDLEN, NULL);


	if (err != BCME_OK) {
		err = -EINVAL;
		if (vndr_ie) {
			kfree(vndr_ie);
		}
	}
	else {
		/* do NOT free 'vndr_ie' for the next process */
		wl_cfg80211_ibss_vsie_set_buffer(vndr_ie, tot_len);
	}

	if (ioctl_buf) {
		kfree(ioctl_buf);
	}

	return err;
}

#if defined(BCMFW_ROAM_ENABLE)
static int
wl_android_set_roampref(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	char smbuf[WLC_IOCTL_SMLEN];
	uint8 buf[MAX_BUF_SIZE];
	uint8 *pref = buf;
	char *pcmd;
#ifdef BCM_PATCH_SECURITY_2017_04
	uint num_ucipher_suites;
	uint num_akm_suites;
#else
	int num_ucipher_suites = 0;
	int num_akm_suites = 0;
#endif
	wpa_suite_t ucipher_suites[MAX_NUM_SUITES];
	wpa_suite_t akm_suites[MAX_NUM_SUITES];
	int num_tuples = 0;
	int total_bytes = 0;
	int total_len_left;
	int i, j;
	char hex[] = "XX";

	pcmd = command + strlen(CMD_SET_ROAMPREF) + 1;
	total_len_left = total_len - strlen(CMD_SET_ROAMPREF) + 1;

	num_akm_suites = simple_strtoul(pcmd, NULL, 16);
#ifdef BCM_PATCH_SECURITY_2017_04
	if (num_akm_suites > MAX_NUM_SUITES) {
		WL_ERR(("wrong num_akm_suites:%d.\n", num_akm_suites));
		return BCME_ERROR;
	}
#endif
	/* Increment for number of AKM suites field + space */
	pcmd += 3;
	total_len_left -= 3;

	/* check to make sure pcmd does not overrun */
	if (total_len_left < (num_akm_suites * WIDTH_AKM_SUITE))
		return -1;

	memset(buf, 0, sizeof(buf));
	memset(akm_suites, 0, sizeof(akm_suites));
	memset(ucipher_suites, 0, sizeof(ucipher_suites));

	/* Save the AKM suites passed in the command */
	for (i = 0; i < num_akm_suites; i++) {
		/* Store the MSB first, as required by join_pref */
		for (j = 0; j < 4; j++) {
			hex[0] = *pcmd++;
			hex[1] = *pcmd++;
			buf[j] = (uint8)simple_strtoul(hex, NULL, 16);
		}
		memcpy((uint8 *)&akm_suites[i], buf, sizeof(uint32));
	}

	total_len_left -= (num_akm_suites * WIDTH_AKM_SUITE);
	num_ucipher_suites = simple_strtoul(pcmd, NULL, 16);
#ifdef BCM_PATCH_SECURITY_2017_04
	if (num_ucipher_suites > MAX_NUM_SUITES) {
		WL_ERR(("wrong num_ucipher_suites:%d.\n", num_ucipher_suites));
		return BCME_ERROR;
	}
#endif
	/* Increment for number of cipher suites field + space */
	pcmd += 3;
	total_len_left -= 3;

	if (total_len_left < (num_ucipher_suites * WIDTH_AKM_SUITE))
		return -1;

	/* Save the cipher suites passed in the command */
	for (i = 0; i < num_ucipher_suites; i++) {
		/* Store the MSB first, as required by join_pref */
		for (j = 0; j < 4; j++) {
			hex[0] = *pcmd++;
			hex[1] = *pcmd++;
			buf[j] = (uint8)simple_strtoul(hex, NULL, 16);
		}
		memcpy((uint8 *)&ucipher_suites[i], buf, sizeof(uint32));
	}

	/* Join preference for RSSI
	 * Type	  : 1 byte (0x01)
	 * Length : 1 byte (0x02)
	 * Value  : 2 bytes	(reserved)
	 */
	*pref++ = WL_JOIN_PREF_RSSI;
	*pref++ = JOIN_PREF_RSSI_LEN;
	*pref++ = 0;
	*pref++ = 0;

	/* Join preference for WPA
	 * Type	  : 1 byte (0x02)
	 * Length : 1 byte (not used)
	 * Value  : (variable length)
	 *		reserved: 1 byte
	 *      count	: 1 byte (no of tuples)
	 *		Tuple1	: 12 bytes
	 *			akm[4]
	 *			ucipher[4]
	 *			mcipher[4]
	 *		Tuple2	: 12 bytes
	 *		Tuplen	: 12 bytes
	 */
	num_tuples = num_akm_suites * num_ucipher_suites;
	if (num_tuples != 0) {
		if (num_tuples <= JOIN_PREF_MAX_WPA_TUPLES) {
			*pref++ = WL_JOIN_PREF_WPA;
			*pref++ = 0;
			*pref++ = 0;
			*pref++ = (uint8)num_tuples;
			total_bytes = JOIN_PREF_RSSI_SIZE + JOIN_PREF_WPA_HDR_SIZE +
				(JOIN_PREF_WPA_TUPLE_SIZE * num_tuples);
		} else {
			DHD_ERROR(("%s: Too many wpa configs for join_pref \n", __FUNCTION__));
			return -1;
		}
	} else {
		/* No WPA config, configure only RSSI preference */
		total_bytes = JOIN_PREF_RSSI_SIZE;
	}

	/* akm-ucipher-mcipher tuples in the format required for join_pref */
	for (i = 0; i < num_ucipher_suites; i++) {
		for (j = 0; j < num_akm_suites; j++) {
			memcpy(pref, (uint8 *)&akm_suites[j], WPA_SUITE_LEN);
			pref += WPA_SUITE_LEN;
			memcpy(pref, (uint8 *)&ucipher_suites[i], WPA_SUITE_LEN);
			pref += WPA_SUITE_LEN;
			/* Set to 0 to match any available multicast cipher */
			memset(pref, 0, WPA_SUITE_LEN);
			pref += WPA_SUITE_LEN;
		}
	}

	prhex("join pref", (uint8 *)buf, total_bytes);
	error = wldev_iovar_setbuf(dev, "join_pref", buf, total_bytes, smbuf, sizeof(smbuf), NULL);
	if (error) {
		DHD_ERROR(("Failed to set join_pref, error = %d\n", error));
	}
	return error;
}
#endif /* defined(BCMFW_ROAM_ENABLE */

static int
wl_android_iolist_add(struct net_device *dev, struct list_head *head, struct io_cfg *config)
{
	struct io_cfg *resume_cfg;
	s32 ret;

	resume_cfg = kzalloc(sizeof(struct io_cfg), GFP_KERNEL);
	if (!resume_cfg)
		return -ENOMEM;

	if (config->iovar) {
		ret = wldev_iovar_getint(dev, config->iovar, &resume_cfg->param);
		if (ret) {
			DHD_ERROR(("%s: Failed to get current %s value\n",
				__FUNCTION__, config->iovar));
			goto error;
		}

		ret = wldev_iovar_setint(dev, config->iovar, config->param);
		if (ret) {
			DHD_ERROR(("%s: Failed to set %s to %d\n", __FUNCTION__,
				config->iovar, config->param));
			goto error;
		}

		resume_cfg->iovar = config->iovar;
	} else {
		resume_cfg->arg = kzalloc(config->len, GFP_KERNEL);
		if (!resume_cfg->arg) {
			ret = -ENOMEM;
			goto error;
		}
		ret = wldev_ioctl(dev, config->ioctl, resume_cfg->arg, config->len, false);
		if (ret) {
			DHD_ERROR(("%s: Failed to get ioctl %d\n", __FUNCTION__,
				config->ioctl));
			goto error;
		}
		ret = wldev_ioctl(dev, config->ioctl + 1, config->arg, config->len, true);
		if (ret) {
			DHD_ERROR(("%s: Failed to set %s to %d\n", __FUNCTION__,
				config->iovar, config->param));
			goto error;
		}
		if (config->ioctl + 1 == WLC_SET_PM)
			wl_cfg80211_update_power_mode(dev);
		resume_cfg->ioctl = config->ioctl;
		resume_cfg->len = config->len;
	}

	list_add(&resume_cfg->list, head);

	return 0;
error:
	kfree(resume_cfg->arg);
	kfree(resume_cfg);
	return ret;
}

static void
wl_android_iolist_resume(struct net_device *dev, struct list_head *head)
{
	struct io_cfg *config;
	struct list_head *cur, *q;
	s32 ret = 0;

	list_for_each_safe(cur, q, head) {
		config = list_entry(cur, struct io_cfg, list);
		if (config->iovar) {
			if (!ret)
				ret = wldev_iovar_setint(dev, config->iovar,
					config->param);
		} else {
			if (!ret)
				ret = wldev_ioctl(dev, config->ioctl + 1,
					config->arg, config->len, true);
			if (config->ioctl + 1 == WLC_SET_PM)
				wl_cfg80211_update_power_mode(dev);
			kfree(config->arg);
		}
		list_del(cur);
		kfree(config);
	}
}

static int
wl_android_set_miracast(struct net_device *dev, char *command, int total_len)
{
	int mode, val = 0;
	int ret = 0;
	struct io_cfg config;

	if (sscanf(command, "%*s %d", &mode) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	DHD_INFO(("%s: enter miracast mode %d\n", __FUNCTION__, mode));

	if (miracast_cur_mode == mode)
		return 0;

	wl_android_iolist_resume(dev, &miracast_resume_list);
	miracast_cur_mode = MIRACAST_MODE_OFF;

	switch (mode) {
	case MIRACAST_MODE_SOURCE:
		/* setting mchan_algo to platform specific value */
		config.iovar = "mchan_algo";

		ret = wldev_ioctl(dev, WLC_GET_BCNPRD, &val, sizeof(int), false);
		if (!ret && val > 100) {
			config.param = 0;
			DHD_ERROR(("%s: Connected station's beacon interval: "
				"%d and set mchan_algo to %d \n",
				__FUNCTION__, val, config.param));
		}
		else {
			config.param = MIRACAST_MCHAN_ALGO;
		}
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		if (ret)
			goto resume;

		/* setting mchan_bw to platform specific value */
		config.iovar = "mchan_bw";
		config.param = MIRACAST_MCHAN_BW;
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		if (ret)
			goto resume;

		/* setting apmdu to platform specific value */
		config.iovar = "ampdu_mpdu";
		config.param = MIRACAST_AMPDU_SIZE;
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		if (ret)
			goto resume;
		/* FALLTROUGH */
		/* Source mode shares most configurations with sink mode.
		 * Fall through here to avoid code duplication
		 */
	case MIRACAST_MODE_SINK:
		/* disable internal roaming */
		config.iovar = "roam_off";
		config.param = 1;
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		if (ret)
			goto resume;
		/* tunr off pm */
		val = 0;
		config.iovar = NULL;
		config.ioctl = WLC_GET_PM;
		config.arg = &val;
		config.len = sizeof(int);
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		if (ret)
			goto resume;

		break;
	case MIRACAST_MODE_OFF:
	default:
		break;
	}
	miracast_cur_mode = mode;

	return 0;

resume:
	DHD_ERROR(("%s: turnoff miracast mode because of err%d\n", __FUNCTION__, ret));
	wl_android_iolist_resume(dev, &miracast_resume_list);
	return ret;
}


int wl_keep_alive_set(struct net_device *dev, char* extra, int total_len)
{
	wl_mkeep_alive_pkt_t	mkeep_alive_pkt;
	int ret;
	uint period_msec = 0;
	char *buf;

	if (extra == NULL)
	{
		 DHD_ERROR(("%s: extra is NULL\n", __FUNCTION__));
		 return -1;
	}
	if (sscanf(extra, "%d", &period_msec) != 1)
	{
		 DHD_ERROR(("%s: sscanf error. check period_msec value\n", __FUNCTION__));
		 return -EINVAL;
	}
	DHD_ERROR(("%s: period_msec is %d\n", __FUNCTION__, period_msec));
	memset(&mkeep_alive_pkt, 0, sizeof(wl_mkeep_alive_pkt_t));

	mkeep_alive_pkt.period_msec = period_msec;
	mkeep_alive_pkt.version = htod16(WL_MKEEP_ALIVE_VERSION);
	mkeep_alive_pkt.length = htod16(WL_MKEEP_ALIVE_FIXED_LEN);

	/* Setup keep alive zero for null packet generation */
	mkeep_alive_pkt.keep_alive_id = 0;
	mkeep_alive_pkt.len_bytes = 0;

	buf = kzalloc(WLC_IOCTL_SMLEN, GFP_KERNEL);
	if (!buf) {
		DHD_ERROR(("%s: buffer alloc failed\n", __FUNCTION__));
		return BCME_NOMEM;
	}

	ret = wldev_iovar_setbuf(dev, "mkeep_alive", (char *)&mkeep_alive_pkt,
				 WL_MKEEP_ALIVE_FIXED_LEN, buf, WLC_IOCTL_SMLEN,
				 NULL);
	if (ret < 0)
		DHD_ERROR(("%s:keep_alive set failed:%d\n", __FUNCTION__, ret));
	else
		DHD_TRACE(("%s:keep_alive set ok\n", __FUNCTION__));

	kfree(buf);
	return ret;
}

#ifdef HW_WIFI_GET_DRIVER_BUS_STATUS
extern dhd_pub_t *hw_get_dhd_pub(struct net_device *dev);
int wl_android_get_drv_bus_status(struct net_device *net)
{
	dhd_pub_t *pub = NULL;

	pub = hw_get_dhd_pub(net);

	if ((pub) && (pub->busstate) != DHD_BUS_DOWN)
		return 0;

	return -1;
}
#endif

#ifdef CONFIG_HW_VOWIFI
static int
wl_android_set_vowifi(struct net_device *dev, char *buf, int total_len,
char *command, int value)
{
	int error = 0;
	int bytes_written;

	WL_ERR(("%s:vowifi command = %s value = %d\n", __FUNCTION__,command,value));
	error = wldev_iovar_setint(dev, command, value);
	if (error) {
		DHD_ERROR(("Failed to vowifi value = %d, error = %d\n",
			value , error));
		return error;
	}

	//modify return buf
	if (strnicmp(command, CMD_VOWIFI_MODE, strlen(CMD_VOWIFI_MODE)) == 0) {
		bytes_written = snprintf(buf, total_len, "%s %d", CMD_SET_MODE, value);
	}
	else if (strnicmp(command, CMD_VOWIFI_PERIOD, strlen(CMD_VOWIFI_PERIOD)) == 0) {
		bytes_written = snprintf(buf, total_len, "%s %d", CMD_SET_PERIOD, value);
	}
	else if (strnicmp(command, CMD_VOWIFI_LOW_THRES, strlen(CMD_VOWIFI_LOW_THRES)) == 0) {
		bytes_written = snprintf(buf, total_len, "%s %d", CMD_SET_LOW_THRESHOLD, value);
	}
	else if (strnicmp(command, CMD_VOWIFI_HIGH_THRES, strlen(CMD_VOWIFI_HIGH_THRES)) == 0) {
		bytes_written = snprintf(buf, total_len, "%s %d", CMD_SET_HIGH_THRESHOLD, value);
	}
	else if (strnicmp(command, CMD_VOWIFI_TRIGGER_COUNT, strlen(CMD_VOWIFI_TRIGGER_COUNT)) == 0) {
		bytes_written = snprintf(buf, total_len, "%s %d", CMD_SET_TRIGGER_COUNT, value);
	}

	DHD_ERROR(("%s: command result is %s (%d)\n", __FUNCTION__, command, bytes_written));
	return bytes_written;
}
static int
wl_android_get_vowifi(struct net_device *dev,char *buf, int total_len, char *command)
{
	int error,bytes_written,val;

	error = wldev_iovar_getint(dev, command, &val);
	WL_ERR(("%s:vowifi get val = %d\n", __FUNCTION__, val));
	if (unlikely(error)) {
			WL_ERR(("could not get vowifi_mode (%d)\n", error));
			return error;
	}
	//modify return buf
	if (strnicmp(command, CMD_VOWIFI_MODE, strlen(CMD_VOWIFI_MODE)) == 0) {
		bytes_written = snprintf(buf, total_len, "%s %d", CMD_GET_MODE, val);
	}
	else if (strnicmp(command, CMD_VOWIFI_PERIOD, strlen(CMD_VOWIFI_PERIOD)) == 0) {
		bytes_written = snprintf(buf, total_len, "%s %d", CMD_GET_PERIOD, val);
	}
	else if (strnicmp(command, CMD_VOWIFI_LOW_THRES, strlen(CMD_VOWIFI_LOW_THRES)) == 0) {
		bytes_written = snprintf(buf, total_len, "%s %d", CMD_GET_LOW_THRESHOLD, val);
	}
	else if (strnicmp(command, CMD_VOWIFI_HIGH_THRES, strlen(CMD_VOWIFI_HIGH_THRES)) == 0) {
		bytes_written = snprintf(buf, total_len, "%s %d", CMD_GET_HIGH_THRESHOLD, val);
	}
	else if (strnicmp(command, CMD_VOWIFI_TRIGGER_COUNT, strlen(CMD_VOWIFI_TRIGGER_COUNT)) == 0) {
		bytes_written = snprintf(buf, total_len, "%s %d", CMD_GET_TRIGGER_COUNT, val);
	}

	DHD_ERROR(("%s: command result is %s (%d)\n", __FUNCTION__, command, bytes_written));
	return bytes_written;

}
s32 wl_android_vowifi_issupport(
	struct net_device *dev, s8 *pval, int total_len)
{
	s32 error,val,bytes_written;

	error = wldev_iovar_getint(dev, CMD_VOWIFI_MODE, &val);
	if (unlikely(error)) {
		WL_ERR(("VOWIFi is not supprot error = %d\n", error));
		bytes_written = snprintf(pval, total_len, "false");
	} else {
		bytes_written = snprintf(pval, total_len, "true");
	}
	DHD_ERROR(("%s: command result is %s \n", __FUNCTION__, pval));

	return bytes_written;
}
#endif

#ifdef HUAWEI_ANDROID_EXTENSION
#if defined(SOFTAP) && defined(HW_SOFTAP_MANAGEMENT)
#define PTYPE_STRING 0
#define PTYPE_INTDEC 1
#define PTYPE_INTHEX 2
#define PTYPE_STR_HEX 3
#define SSID_LEN	33
#define SEC_LEN		16
#define KEY_LEN		65
#define PROFILE_OFFSET	32
struct ap_profile {
	uint8	ssid[SSID_LEN];
	uint8	sec[SEC_LEN];
	uint8	key[KEY_LEN];
	uint32	channel;
	uint32	preamble;
	uint32	max_scb;
	uint32  closednet;
	char country_code[WLC_CNTRY_BUF_SZ];
};

#define MACLIST_MODE_DISABLED  0
#define MACLIST_MODE_DENY              1
#define MACLIST_MODE_ALLOW             2
struct mflist {
       uint count;
       struct ether_addr ea[16];
};

struct mac_list_set {
       uint32  mode;
       struct mflist mac_list;
};

#define MAX_WX_STRING 80
#define MAC_FILT_MAX 8
#define htod32(i) (i)
#define WL_SOFTAP(x) printk x;

static int
dev_wlc_ioctl(
	struct net_device *dev,
	int cmd,
	void *arg,
	int len
)
{
	struct ifreq ifr;
	wl_ioctl_t ioc;
	mm_segment_t fs;
	int ret;

	memset(&ioc, 0, sizeof(ioc));
#if defined(CONFIG_COMPAT) && defined(HW_SOFTAP_MANAGEMENT_BUG)
		ioc.cmd = cmd | WLC_SPEC_FLAG;
#else
		ioc.cmd = cmd;
#endif
	ioc.buf = arg;
	ioc.len = len;

	strncpy(ifr.ifr_name, dev->name, sizeof(ifr.ifr_name) - 1);
	ifr.ifr_data = (caddr_t) &ioc;

	fs = get_fs();
	set_fs(get_ds());
#if defined(WL_USE_NETDEV_OPS)
	ret = dev->netdev_ops->ndo_do_ioctl(dev, &ifr, SIOCDEVPRIVATE);
#else
	ret = dev->do_ioctl(dev, &ifr, SIOCDEVPRIVATE);
#endif
	set_fs(fs);

	return ret;
}

/*
set named driver variable to int value and return error indication
calling example: dev_wlc_intvar_set(dev, "arate", rate)
*/
static int
dev_wlc_intvar_set(
	struct net_device *dev,
	char *name,
	int val)
{
	char buf[WLC_IOCTL_SMLEN];
	uint len;

	val = htod32(val);
	len = bcm_mkiovar(name, (char *)(&val), sizeof(val), buf, sizeof(buf));
	ASSERT(len);

	return (dev_wlc_ioctl(dev, WLC_SET_VAR, buf, len));
}

static int
dev_iw_iovar_setbuf(
	struct net_device *dev,
	char *iovar,
	void *param,
	int paramlen,
	void *bufptr,
	int buflen)
{
	int iolen;

	iolen = bcm_mkiovar(iovar, param, paramlen, bufptr, buflen);
	ASSERT(iolen);
	BCM_REFERENCE(iolen);

	return (dev_wlc_ioctl(dev, WLC_SET_VAR, bufptr, iolen));
}

static int
dev_iw_write_cfg0_bss_var(struct net_device *dev, int val)
{
	struct {
		int cfg;
		int val;
	} bss_setbuf;
	int bss_set_res;
	char smbuf[WLC_IOCTL_SMLEN];
	memset(smbuf, 0, sizeof(smbuf));
	bss_setbuf.cfg = 0;
	bss_setbuf.val = val;
	bss_set_res = dev_iw_iovar_setbuf(dev, "bss",
		&bss_setbuf, sizeof(bss_setbuf), smbuf, sizeof(smbuf));
	WL_TRACE(("%s: bss_set_result:%d set with %d\n", __FUNCTION__, bss_set_res, val));
	return bss_set_res;
}

#ifdef HW_SOFTAP_REASON_CODE
/* 802.11-2012 Table 8-36--Reason codes */
/* Previous authentication no longer valid */
#define REASON_CODE_NO_VALID    2
/* Disassociated due to inactivity */
#define REASON_CODE_INACTIVITY  4
/* Disassociated beause AP is unable to handle all currently associated STAs */
#define REASON_CODE_UN_HANDLE   5
/* Disassociated because sending STA is leaving (or has left) BSS */
#define REASON_CODE_LEAVING     8

/* 802.11-2012 Table 8-37--Status codes */
/* Association denied because AP is unable to handle additional associated STAs */
#define STATUS_CODE_LIMIT       17
/* The request has been declined */
#define STATUS_CODE_DECLINED    37

/* deny auth request from STAs outside of ACL list */
#define HW_DENY_REASON_ACL      STATUS_CODE_DECLINED
#endif

void check_error(int res, const char *msg, const char *func, int line)
{
       if (res != 0)
               WL_ERROR(("%s, %d function:%s, line:%d\n", msg, res, func, line));
}

static int
hex2num(char c)
{
       if (c >= '0' && c <= '9')
               return c - '0';
       if (c >= 'a' && c <= 'f')
               return c - 'a' + 10;
       if (c >= 'A' && c <= 'F')
               return c - 'A' + 10;
       return -1;
}
static int
hstr_2_buf(const char *txt, u8 *buf, int len)
{
       int i;
       for (i = 0; i < len; i++) {
               int a, b;
               a = hex2num(*txt++);
               if (a < 0)
                       return -1;
               b = hex2num(*txt++);
               if (b < 0)
                       return -1;
               *buf++ = (a << 4) | b;
       }
       return 0;
}

static int
get_parameter_from_string(
                        char **str_ptr, const char *token,
                        int param_type, void  *dst, int param_max_len)
{
        char int_str[7] = "0";
        int parm_str_len;
        char  *param_str_begin;
        char  *param_str_end;
        char  *orig_str = *str_ptr;
        if ((*str_ptr) && !strncmp(*str_ptr, token, strlen(token))) {
                strsep(str_ptr, "=,");
                param_str_begin = *str_ptr;
                strsep(str_ptr, "=,");
                if (*str_ptr == NULL) {
                        parm_str_len = strlen(param_str_begin);
                } else {
                        param_str_end = *str_ptr-1;
                        parm_str_len = param_str_end - param_str_begin;
                }
                WL_TRACE((" 'token:%s', len:%d, ", token, parm_str_len));
                if (parm_str_len > param_max_len) {
                        WL_ERROR((" WARNING: extracted param len:%d is > MAX:%d\n",
                                parm_str_len, param_max_len));
                        parm_str_len = param_max_len;
                }
                switch (param_type) {
                        case PTYPE_INTDEC: {
                                int *pdst_int = dst;
                                char *eptr;
                                if (parm_str_len > sizeof(int_str))
                                         parm_str_len = sizeof(int_str);
                                memcpy(int_str, param_str_begin, parm_str_len);
                                *pdst_int = simple_strtoul(int_str, &eptr, 10);
                                WL_TRACE((" written as integer:%d\n",  *pdst_int));
                        }
                        break;
                        case PTYPE_STR_HEX: {
                                u8 *buf = dst;
                                param_max_len = param_max_len >> 1;
                                hstr_2_buf(param_str_begin, buf, param_max_len);
                                dhd_print_buf(buf, param_max_len, 0);
                        }
                        break;
                        default:
                                memcpy(dst, param_str_begin, parm_str_len);
                                if (parm_str_len == param_max_len) {
                                         *((char *)dst + parm_str_len - 1) = 0;
                                } else {
                                         *((char *)dst + parm_str_len) = 0;
                                }
                                WL_ERROR((" written as a string:%s\n", (char *)dst));
                        break;
                }
                return 0;
        } else {
                WL_ERROR(("\n %s: ERROR: can't find token:%s in str:%s \n",
                        __FUNCTION__, token, orig_str));
         return -1;
        }
}

static int
init_ap_profile_from_string(char *param_str, struct ap_profile *ap_cfg)
{
	char *str_ptr = param_str;
#ifdef HW_MEM_OVERFLOW_BUGFIX
	char sub_cmd[SSID_LEN + 1];
#else
	char sub_cmd[16];
#endif
	int ret = 0;
	memset(sub_cmd, 0, sizeof(sub_cmd));
	memset(ap_cfg, 0, sizeof(struct ap_profile));
	if (get_parameter_from_string(&str_ptr, "ASCII_CMD=",
		PTYPE_STRING, sub_cmd, SSID_LEN) != 0) {
	 return -1;
	}
	if (strncmp(sub_cmd, "AP_CFG", 6)) {
	   WL_ERROR(("ERROR: sub_cmd:%s != 'AP_CFG'!\n", sub_cmd));
		return -1;
	}
	get_parameter_from_string(&str_ptr, "SSID=", PTYPE_STRING, ap_cfg->ssid, SSID_LEN);
	get_parameter_from_string(&str_ptr, "SEC=", PTYPE_STRING,  ap_cfg->sec, SEC_LEN);
	get_parameter_from_string(&str_ptr, "KEY=", PTYPE_STRING,  ap_cfg->key, KEY_LEN);
	get_parameter_from_string(&str_ptr, "CHANNEL=", PTYPE_INTDEC, &ap_cfg->channel, 5);
	get_parameter_from_string(&str_ptr, "PREAMBLE=", PTYPE_INTDEC, &ap_cfg->preamble, 5);
	get_parameter_from_string(&str_ptr, "MAX_SCB=", PTYPE_INTDEC,  &ap_cfg->max_scb, 5);
	get_parameter_from_string(&str_ptr, "HIDDEN=",
		PTYPE_INTDEC,  &ap_cfg->closednet, 5);
	get_parameter_from_string(&str_ptr, "COUNTRY=",
		PTYPE_STRING,  &ap_cfg->country_code, 3);
	return ret;
}

#ifndef AP_ONLY
static int last_auto_channel = 6;
#endif

static int
get_softap_auto_channel(struct net_device *dev, struct ap_profile *ap)
{
	int chosen = 0;
	wl_uint32_list_t request;
	int retry = 0;
	int ret = 0;
	int res = 0;
	request.count = htod32(0);
	ret = dev_wlc_ioctl(dev, WLC_START_CHANNEL_SEL, &request, sizeof(request));
	if (ret < 0) {
		WL_ERROR(("can't start auto channel scan\n"));
		goto fail;
	}
	get_channel_retry:
	bcm_mdelay(350);
	ret = dev_wlc_ioctl(dev, WLC_GET_CHANNEL_SEL, &chosen, sizeof(chosen));
		if (ret < 0 || dtoh32(chosen) == 0) {
			if (retry++ < 15) {
				goto get_channel_retry;
			} else {
				if (ret < 0) {
					WL_ERROR(("can't get auto channel sel, err = %d, "
					          "chosen = 0x%04X\n", ret, (uint16)chosen));
					goto fail;
				} else {
					ap->channel = (uint16)last_auto_channel;
					WL_ERROR(("auto channel sel timed out. we get channel %d\n",
						ap->channel));
				}
			}
		}
		if (chosen) {
			ap->channel = (uint16)chosen & 0x00FF;
			WL_ERROR(("%s: Got auto channel = %d, attempt:%d\n",
				__FUNCTION__, ap->channel, retry));
		}
#ifndef AP_ONLY
	if (!res || !ret)
		last_auto_channel = ap->channel;
#endif
fail :
	if (ret < 0) {
		WL_TRACE(("%s: return value %d\n", __FUNCTION__, ret));
		return ret;
	}
	return res;
}

static int
set_ap_cfg_hw(struct net_device *dev, struct ap_profile *ap)
{
#ifndef HW_SOFTAP_ENABLE_BW_80
	int channel = 0;
#endif
	int max_assoc = 8;
	int res = 0;
	int mpc = 0;
#ifndef HW_SOFTAP_ENABLE_BW_80
	int updown = 0;
#endif
	if (!dev) {
		WL_ERROR(("%s: dev is null\n", __FUNCTION__));
		return -1;
	}

	WL_ERROR(("wl_iw: set ap profile:\n"));
	WL_ERROR(("	ssid = '%s'\n", ap->ssid));
	WL_ERROR(("	security = '%s'\n", ap->sec));
	if (ap->key[0] != '\0')
		WL_ERROR(("	key = '%s'\n", ap->key));
	WL_ERROR(("	channel = %d\n", ap->channel));
	WL_ERROR(("	max scb = %d\n", ap->max_scb));
	mpc = 0;
	if ((res = dev_wlc_intvar_set(dev, "mpc", mpc))) {
		WL_ERROR(("%s fail to set mpc\n", __FUNCTION__));
		goto fail;
	}
	OSL_DELAY(50*1000);
#ifndef HW_SOFTAP_ENABLE_BW_80
	if ((res = dev_iw_write_cfg0_bss_var(dev, 0)) < 0) {
		WL_ERROR(("%s fail to set bss down\n", __FUNCTION__));
		goto fail;
	}
	OSL_DELAY(50*1000);
	if ((ap->channel == 0) && (get_softap_auto_channel(dev, ap) < 0)) {
		ap->channel = 1;
		WL_ERROR(("%s auto channel failed, use channel=%d\n",
		          __FUNCTION__, ap->channel));
	}
	OSL_DELAY(50*1000);
	if ((res = dev_iw_write_cfg0_bss_var(dev, 1)) < 0) {
		WL_ERROR(("%s fail to set bss up\n", __FUNCTION__));
		goto fail;
	}
	OSL_DELAY(50*1000);
	updown = 1;
	if ((res = dev_wlc_ioctl(dev, WLC_DOWN, &updown, sizeof(updown))) < 0) {
		WL_ERROR(("%s fail to set down err =%d\n", __FUNCTION__, res));
		goto fail;
	}
	OSL_DELAY(50*1000);
	WL_SOFTAP(("%s: use channel=%d\n", __FUNCTION__, ap->channel));
	channel = ap->channel;
	if ((res = dev_wlc_ioctl(dev, WLC_SET_CHANNEL, &channel, sizeof(channel)))) {
		WL_ERROR(("%s fail to set channel\n", __FUNCTION__));
	}
	OSL_DELAY(50*1000);
	updown = 1;
	if ((res = dev_wlc_ioctl(dev, WLC_UP, &updown, sizeof(updown))) < 0) {
		WL_ERROR(("%s fail to set up err =%d\n", __FUNCTION__, res));
		goto fail;
	}
	OSL_DELAY(50*1000);
#endif
	max_assoc = ap->max_scb;
	if ((res = dev_wlc_intvar_set(dev, "maxassoc", max_assoc))) {
		WL_ERROR(("%s fail to set maxassoc\n", __FUNCTION__));
		goto fail;
	}
fail:
	WL_ERROR(("%s exit with %d\n", __FUNCTION__, res));
	return res;
}

static int
wl_android_set_ap_config(struct net_device *dev, char *command, int total_len)
{
	int res = 0;
	struct ap_profile ap_cfg;
	memset(&ap_cfg, 0, sizeof(struct ap_profile));
	if ((res = init_ap_profile_from_string(command, &ap_cfg)) < 0) {
		WL_ERROR(("%s failed to parse %d\n", __FUNCTION__, res));
		return -1;
	}

	if ((res = set_ap_cfg_hw(dev, &ap_cfg)) < 0)
		WL_ERROR(("%s failed to set_ap_cfg %d\n", __FUNCTION__, res));
	return res;
}

static int
get_assoc_sta_list(struct net_device *dev, char *buf, int len)
{
	WL_TRACE(("%s: dev_wlc_ioctl(dev:%p, cmd:%d, buf:%p, len:%d)\n",
		__FUNCTION__, dev, WLC_GET_ASSOCLIST, buf, len));
	return dev_wlc_ioctl(dev, WLC_GET_ASSOCLIST, buf, len);
}

static int wl_android_get_assoc_list(struct net_device *dev, char *command, int total_len)
{
	int i, ret = 0;
	char mac_buf[256];
	struct maclist *sta_maclist = (struct maclist *)mac_buf;
	char mac_lst[384];
	char *p_mac_str;
	char *p_mac_str_end;
	int bytes_written = 0;
	if ((!dev) || (!command)) {
		return -EINVAL;
	}

	memset(sta_maclist, 0, sizeof(mac_buf));
	sta_maclist->count = 8;

	if ((ret = get_assoc_sta_list(dev, mac_buf, sizeof(mac_buf))) < 0) {
		WL_ERROR(("%s: sta list ioctl error:%d\n", __FUNCTION__, ret));
		goto func_exit;
	}

	WL_ERROR(("%s: got %d stations\n", __FUNCTION__, sta_maclist->count));
	memset(mac_lst, 0, sizeof(mac_lst));
	p_mac_str = mac_lst;
	p_mac_str_end = &mac_lst[sizeof(mac_lst)-1];
	for (i = 0; i < 8; i++) {
		struct ether_addr * id = &sta_maclist->ea[i];
		if (!ETHER_ISNULLADDR(id->octet)) {
			if ((p_mac_str_end - p_mac_str) <= 36) {
				WL_ERROR(("%s: mac list buf is < 36 for item[%i] item\n",
					__FUNCTION__, i));
				break;
			}
			p_mac_str += snprintf(p_mac_str, MAX_WX_STRING,
			"%02X:%02X:%02X:%02X:%02X:%02X\n",
			id->octet[0], id->octet[1], id->octet[2],
			id->octet[3], id->octet[4], id->octet[5]);
		}
	}

	if ((total_len) < (int)(strlen(mac_lst)+1))
		return -ENOMEM;
	bytes_written = snprintf(command, total_len, "%s", mac_lst);
	return bytes_written;
func_exit:
	return ret;
}


static int
set_ap_mac_list(struct net_device *dev, void *buf)
{
	struct mac_list_set *mac_list_set = (struct mac_list_set *)buf;
	struct maclist *maclist = (struct maclist *)&mac_list_set->mac_list;
	int length;
	uint i;
	int mac_mode = mac_list_set->mode;
	int ioc_res = 0;
	if (mac_mode == MACLIST_MODE_DISABLED) {
		ioc_res = dev_wlc_ioctl(dev, WLC_SET_MACMODE, &mac_mode, sizeof(mac_mode));
		check_error(ioc_res, "ioctl ERROR:", __FUNCTION__, __LINE__);
		WL_SOFTAP(("%s: MAC filtering disabled\n", __FUNCTION__));
	} else {
		scb_val_t scbval;
		char mac_buf[256] = {0};
		struct maclist *assoc_maclist = (struct maclist *) mac_buf;
#ifdef HW_SOFTAP_REASON_CODE
		int reasonCode = HW_DENY_REASON_ACL;
#endif
		ioc_res = dev_wlc_ioctl(dev, WLC_SET_MACMODE, &mac_mode, sizeof(mac_mode));
		check_error(ioc_res, "ioctl ERROR:", __FUNCTION__, __LINE__);
		length = sizeof(maclist->count) + maclist->count*ETHER_ADDR_LEN;
		dev_wlc_ioctl(dev, WLC_SET_MACLIST, maclist, length);
		WL_SOFTAP(("%s: applied MAC List, mode:%d, length %d:\n",
			__FUNCTION__, mac_mode, length));
		for (i = 0; i < maclist->count; i++)
			WL_SOFTAP(("mac %d: %02X:%02X:%02X:%02X:%02X:%02X\n",
				i, maclist->ea[i].octet[0], maclist->ea[i].octet[1],
				maclist->ea[i].octet[2],
				maclist->ea[i].octet[3], maclist->ea[i].octet[4],
				maclist->ea[i].octet[5]));
#ifdef HW_SOFTAP_REASON_CODE
		/* set ACL deny reason */
		WL_SOFTAP(("%s: set deny reason %d\n", __FUNCTION__, reasonCode));
		ioc_res = dev_wlc_intvar_set(dev, "mac_deny_reason", reasonCode);
		check_error(ioc_res, "ioctl ERROR:", __FUNCTION__, __LINE__);
#endif
		assoc_maclist->count = 8;
		ioc_res = dev_wlc_ioctl(dev, WLC_GET_ASSOCLIST, assoc_maclist, 256);
		check_error(ioc_res, "ioctl ERROR:", __FUNCTION__, __LINE__);
		WL_SOFTAP((" Cur assoc clients:%d\n", assoc_maclist->count));
		if (assoc_maclist->count)
			for (i = 0; i < assoc_maclist->count; i++) {
				uint j;
				bool assoc_mac_matched = FALSE;
				WL_SOFTAP(("\n Cheking assoc STA: "));
				dhd_print_buf(&assoc_maclist->ea[i], 6, 7);
				WL_SOFTAP(("with the b/w list:"));
				for (j = 0; j < maclist->count; j++)
					if (!bcmp(&assoc_maclist->ea[i], &maclist->ea[j],
						ETHER_ADDR_LEN)) {
						assoc_mac_matched = TRUE;
						break;
					}
				if (((mac_mode == MACLIST_MODE_ALLOW) && !assoc_mac_matched) ||
					((mac_mode == MACLIST_MODE_DENY) && assoc_mac_matched)) {
					WL_SOFTAP(("b-match or w-mismatch,"
								" do deauth/disassoc \n"));
							scbval.val = htod32(1);
							bcopy(&assoc_maclist->ea[i], &scbval.ea,
							ETHER_ADDR_LEN);
							ioc_res = dev_wlc_ioctl(dev,
								WLC_SCB_DEAUTHENTICATE_FOR_REASON,
								&scbval, sizeof(scb_val_t));
							check_error(ioc_res,
								"ioctl ERROR:",
								__FUNCTION__, __LINE__);
				} else {
					WL_SOFTAP((" no b/w list hits, let it be\n"));
				}
		} else {
			WL_SOFTAP(("No ASSOC CLIENTS\n"));
		}
	}
	WL_SOFTAP(("%s iocres:%d\n", __FUNCTION__, ioc_res));
	return ioc_res;
}

static int wl_android_set_mac_filters(struct net_device *dev, char *command, int total_len)
{
	int i, ret = -1;
	int mac_cnt = 0;
	int mac_mode = 0;
	struct ether_addr *p_ea;
	struct mac_list_set mflist_set;
	char *str_ptr;

	memset(&mflist_set, 0, sizeof(mflist_set));
	str_ptr = command;
	if (get_parameter_from_string(&str_ptr, "MAC_MODE=",
		PTYPE_INTDEC, &mac_mode, 4) != 0) {
		WL_ERROR(("ERROR: 'MAC_MODE=' token is missing\n"));
		goto exit_proc;
	}
	p_ea = &mflist_set.mac_list.ea[0];
	if (get_parameter_from_string(&str_ptr, "MAC_CNT=",
		PTYPE_INTDEC, &mac_cnt, 4) != 0) {
		WL_ERROR(("ERROR: 'MAC_CNT=' token param is missing \n"));
		goto exit_proc;
	}
	if (mac_cnt < 0 || mac_cnt > MAC_FILT_MAX) {
		WL_ERROR(("ERROR: number of MAC filters > MAX\n"));
		goto exit_proc;
	}
	for (i=0; i< mac_cnt; i++)
	if (get_parameter_from_string(&str_ptr, "MAC=",
		PTYPE_STR_HEX, &p_ea[i], 12) != 0) {
		WL_ERROR(("ERROR: MAC_filter[%d] is missing !\n", i));
		goto exit_proc;
	}
	WL_ERROR(("MAC_MODE=:%d, MAC_CNT=%d, MACs:..\n", mac_mode, mac_cnt));
	for (i = 0; i < mac_cnt; i++) {
	   WL_ERROR(("mac_filt[%d]:", i));
	   dhd_print_buf(&p_ea[i], 6, 0);
	}
	mflist_set.mode = mac_mode;
	mflist_set.mac_list.count = mac_cnt;
	set_ap_mac_list(dev, &mflist_set);
	ret = 0;

	exit_proc:
	return ret;
}

static int wl_iw_softap_deassoc_stations(struct net_device *dev, u8 *mac)
{
	int i;
	int res = 0;
	char mac_buf[128] = {0};
	char z_mac[6] = {0, 0, 0, 0, 0, 0};
	char *sta_mac;
	struct maclist *assoc_maclist = (struct maclist *) mac_buf;
	bool deauth_all = FALSE;
	if (mac == NULL) {
		deauth_all = TRUE;
		sta_mac = z_mac;
	} else {
		sta_mac = mac;
	}
	memset(assoc_maclist, 0, sizeof(mac_buf));
	assoc_maclist->count = 8;
	res = dev_wlc_ioctl(dev, WLC_GET_ASSOCLIST, assoc_maclist, 128);
	if (res != 0) {
		WL_ERROR(("%s: Error:%d Couldn't get ASSOC List\n", __FUNCTION__, res));
		return res;
	}
	if (assoc_maclist->count)
		for (i = 0; i < assoc_maclist->count; i++) {
		scb_val_t scbval;
		scbval.val = htod32(1);
		bcopy(&assoc_maclist->ea[i], &scbval.ea, ETHER_ADDR_LEN);
		if (deauth_all || (memcmp(&scbval.ea, sta_mac, ETHER_ADDR_LEN) == 0))  {
			WL_ERROR(("%s, deauth STA:%d \n", __FUNCTION__, i));
			res |= dev_wlc_ioctl(dev, WLC_SCB_DEAUTHENTICATE_FOR_REASON,
				&scbval, sizeof(scb_val_t));
		}
	} else WL_SOFTAP(("%s: No Stations \n", __FUNCTION__));
	if (res != 0) {
		WL_ERROR(("%s: Error:%d\n", __FUNCTION__, res));
	} else if (assoc_maclist->count) {
		bcm_mdelay(200);
	}
	return res;
}

static int wl_android_set_ap_sta_disassoc(struct net_device *dev, char *command, int total_len)
{
	int res = 0;
	char sta_mac[6] = {0, 0, 0, 0, 0, 0};
	char *str_ptr = command;

	if (get_parameter_from_string(&str_ptr,
		"MAC=", PTYPE_STR_HEX, sta_mac, 12) == 0) {
		res = wl_iw_softap_deassoc_stations(dev, sta_mac);
	} else  {
		WL_ERROR(("ERROR: STA_MAC= token not found\n"));
      }

	return res;
}
#endif

#define SAR_PARAMETER1  1001
#define SAR_PARAMETER2  1002
#define SAR_PARAMETER3  1003
#define SAR_DISABLE 1000
static int
wl_iw_set_sar_enable(
       struct net_device *dev,
       int sar_enable)
{
       int error = -EINVAL;

       if (sar_enable == SAR_DISABLE) {
		error = dev_wlc_intvar_set(dev, "sar_enable", 0);
		WL_ERROR(("%s enter sar_disable  \n", __FUNCTION__));
       } else if(sar_enable == SAR_PARAMETER1) {
		error = dev_wlc_intvar_set(dev, "sar_enable", 1);
		WL_ERROR(("%s enter sar_enable parameter1 \n",__FUNCTION__));
       } else if(sar_enable == SAR_PARAMETER2) {
                error = dev_wlc_intvar_set(dev, "sar_enable", 2);
                WL_ERROR(("%s enter sar_enable parameter2 \n",__FUNCTION__));
       } else if(sar_enable == SAR_PARAMETER3) {
                error = dev_wlc_intvar_set(dev, "sar_enable", 3);
                WL_ERROR(("%s enter sar_enable parameter3 \n",__FUNCTION__));
       } else {
		WL_ERROR(("Sar parameter is invalid\n"));
       }

       return error;
}

static int
wl_iw_set_txpow_dbm(
	struct net_device *dev,
	int txpwrdbm)
{
	int error;
	txpwrdbm *= 4;
	if (txpwrdbm < 0 || txpwrdbm>127) {
		txpwrdbm = 127;
	}
	error = dev_wlc_intvar_set(dev, "qtxpower", txpwrdbm);
	return error;
}

static int
wl_android_set_txpow(struct net_device *dev, char *command, int total_len)
{
	int error, disable;
	int value = 0;
	int disabled = 0;
	WL_TRACE(("%s: SIOCSIWTXPOW\n", dev->name));

	if (sscanf(command, "%d %d", &disabled, &value) != 2)
	{
		 DHD_ERROR(("%s: sscanf error. check txpow value\n", __FUNCTION__));
		 return -EINVAL;
	}
	/* Make sure radio is off or on as far as software is concerned */
	disable = disabled ? WL_RADIO_SW_DISABLE : 0;
	disable += WL_RADIO_SW_DISABLE << 16;

	disable = htod32(disable);
	if ((error = dev_wlc_ioctl(dev, WLC_SET_RADIO, &disable, sizeof(disable))))
		return error;

	/* If Radio is off, nothing more to do */
	if (disable & WL_RADIO_SW_DISABLE)
		return 0;

	/* handle dbm */
	if(value >= SAR_DISABLE)
		error = wl_iw_set_sar_enable(dev, value);
	else
		error = wl_iw_set_txpow_dbm(dev, value);

	return error;
}

static int
wl_android_set_disassoc_roaming_bssid(struct net_device *dev, char *command, int total_len)
{
	int error;
	int mode = AUTO_RECOVERY_BY_MANUAL;
    WL_TRACE(("%s: wl_android_set_disassoc_roaming_bssid\n", dev->name));
	scb_val_t scbval;
	memset(&scbval, 0, sizeof(scb_val_t));
	if(!command)
	{
		DHD_ERROR(("%s: invalid mode vale\n", __FUNCTION__));
		return -EINVAL;
	}
	if (sscanf(command, "%d", &mode) != 1)
	{
		 DHD_ERROR(("%s: sscanf error. check wl_android_set_disassoc_roaming_bssid value\n", __FUNCTION__));
		 return -EINVAL;
	}
	if((mode < AUTO_RECOVERY_AFTER_ROAMING) || (mode > AUTO_RECOVERY_BY_MANUAL))
	{
		 DHD_ERROR(("%s: mode error. mode vale is %d\n", __FUNCTION__, mode));
		 return -EINVAL;
	}
	DHD_ERROR(("%s: mode vale is %d\n", __FUNCTION__, mode));
	scbval.val = mode;
	if ((error = wldev_ioctl(dev, WLC_SCB_DEAUTHENTICATE_FOR_REASON, &scbval, sizeof(scb_val_t), true)) != 0){
	     DHD_ERROR(("%s WLC_SCB_DEAUTHENTICATE error=%d\n", __FUNCTION__, error));
	}
	return error;
}

#endif
#ifdef HW_SHARE_WIFI_FILTER_MANAGE
bool g_force_stop_filter = false;
#endif
int wl_android_priv_cmd(struct net_device *net, struct ifreq *ifr, int cmd)
{
#define PRIVATE_COMMAND_MAX_LEN	8192
	int ret = 0;
#ifdef CONFIG_HW_VOWIFI
	int value = 0;
#endif
	char *command = NULL;
	int bytes_written = 0;
	android_wifi_priv_cmd priv_cmd;
	dhd_pub_t * dhd_pub = hw_get_dhd_pub(net);

	net_os_wake_lock(net);
#ifdef BCM_PATCH_CVE_2016_2475
	if (!capable(CAP_NET_ADMIN)) {
		ret = -EPERM;
		goto exit;
	}
#endif
	if (!ifr || !ifr->ifr_data) {
		ret = -EINVAL;
		goto exit;
	}

#ifdef CONFIG_COMPAT
	if (is_compat_task()) {
		compat_android_wifi_priv_cmd compat_priv_cmd;
		if (copy_from_user(&compat_priv_cmd, ifr->ifr_data,
			sizeof(compat_android_wifi_priv_cmd))) {
			ret = -EFAULT;
			goto exit;

		}
		priv_cmd.buf = compat_ptr(compat_priv_cmd.buf);
		priv_cmd.used_len = compat_priv_cmd.used_len;
		priv_cmd.total_len = compat_priv_cmd.total_len;
	} else
#endif /* CONFIG_COMPAT */
	{
		if (copy_from_user(&priv_cmd, ifr->ifr_data, sizeof(android_wifi_priv_cmd))) {
			ret = -EFAULT;
			goto exit;
		}
	}
	if ((priv_cmd.total_len > PRIVATE_COMMAND_MAX_LEN) || (priv_cmd.total_len < 0)) {
		DHD_ERROR(("%s: too long priavte command\n", __FUNCTION__));
		ret = -EINVAL;
		goto exit;
	}
	command = kmalloc((priv_cmd.total_len + 1), GFP_KERNEL);
	if (!command)
	{
		DHD_ERROR(("%s: failed to allocate memory\n", __FUNCTION__));
		ret = -ENOMEM;
		goto exit;
	}
	if (copy_from_user(command, priv_cmd.buf, priv_cmd.total_len)) {
		ret = -EFAULT;
		goto exit;
	}
	command[priv_cmd.total_len] = '\0';

	DHD_INFO(("%s: Android private cmd \"%s\" on %s\n", __FUNCTION__, command, ifr->ifr_name));

	if (strnicmp(command, CMD_START, strlen(CMD_START)) == 0) {
		DHD_INFO(("%s, Received regular START command\n", __FUNCTION__));
		bytes_written = wl_android_wifi_on(net);
	}
	else if (strnicmp(command, CMD_SETFWPATH, strlen(CMD_SETFWPATH)) == 0) {
		bytes_written = wl_android_set_fwpath(net, command, priv_cmd.total_len);
	}
#ifdef HW_WIFI_GET_DRIVER_BUS_STATUS
	if (strnicmp(command, CMD_DRV_BUS_STATUS, strlen(CMD_DRV_BUS_STATUS)) == 0) {
		DHD_INFO(("%s, Received Driver Status Command\n", __FUNCTION__));
		ret = wl_android_get_drv_bus_status(net);
		goto exit;
	}
#endif

	if (!g_wifi_on) {
		DHD_ERROR(("%s: Ignore private cmd \"%s\" - iface %s is down\n",
			__FUNCTION__, command, ifr->ifr_name));
		ret = 0;
		goto exit;
	}

	if (strnicmp(command, CMD_STOP, strlen(CMD_STOP)) == 0) {
		bytes_written = wl_android_wifi_off(net, FALSE);
	}
	else if (strnicmp(command, CMD_SCAN_ACTIVE, strlen(CMD_SCAN_ACTIVE)) == 0) {
		/* TBD: SCAN-ACTIVE */
	}
	else if (strnicmp(command, CMD_SCAN_PASSIVE, strlen(CMD_SCAN_PASSIVE)) == 0) {
		/* TBD: SCAN-PASSIVE */
	}
	else if (strnicmp(command, CMD_RSSI, strlen(CMD_RSSI)) == 0) {
		bytes_written = wl_android_get_rssi(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_LINKSPEED, strlen(CMD_LINKSPEED)) == 0) {
		bytes_written = wl_android_get_link_speed(net, command, priv_cmd.total_len);
	}
#ifdef PKT_FILTER_SUPPORT
	else if (strnicmp(command, CMD_RXFILTER_START, strlen(CMD_RXFILTER_START)) == 0) {
#ifdef HW_SHARE_WIFI_FILTER_MANAGE
		g_force_stop_filter = false;
		if (dhd_pub && dhd_pub->in_suspend)
			dhd_dev_apf_enable_filter(net);
#endif /* HW_SHARE_WIFI_FILTER_MANAGE */
		bytes_written = net_os_enable_packet_filter(net, 1);
	}
	else if (strnicmp(command, CMD_RXFILTER_STOP, strlen(CMD_RXFILTER_STOP)) == 0) {
#ifdef HW_SHARE_WIFI_FILTER_MANAGE
		g_force_stop_filter = true;
		if (dhd_pub && !dhd_pub->in_suspend)
			dhd_dev_apf_enable_filter(net);
#endif
		bytes_written = net_os_enable_packet_filter(net, 0);
	}
	else if (strnicmp(command, CMD_RXFILTER_ADD, strlen(CMD_RXFILTER_ADD)) == 0) {
		int filter_num = *(command + strlen(CMD_RXFILTER_ADD) + 1) - '0';
		bytes_written = net_os_rxfilter_add_remove(net, TRUE, filter_num);
	}
	else if (strnicmp(command, CMD_RXFILTER_REMOVE, strlen(CMD_RXFILTER_REMOVE)) == 0) {
		int filter_num = *(command + strlen(CMD_RXFILTER_REMOVE) + 1) - '0';
		bytes_written = net_os_rxfilter_add_remove(net, FALSE, filter_num);
	}
#endif /* PKT_FILTER_SUPPORT */
	else if (strnicmp(command, CMD_BTCOEXSCAN_START, strlen(CMD_BTCOEXSCAN_START)) == 0) {
		/* TBD: BTCOEXSCAN-START */
	}
	else if (strnicmp(command, CMD_BTCOEXSCAN_STOP, strlen(CMD_BTCOEXSCAN_STOP)) == 0) {
		/* TBD: BTCOEXSCAN-STOP */
	}
	else if (strnicmp(command, CMD_BTCOEXMODE, strlen(CMD_BTCOEXMODE)) == 0) {
#ifdef WL_CFG80211
		void *dhdp = wl_cfg80211_get_dhdp();
		if (NULL != dhdp) {
			bytes_written = wl_cfg80211_set_btcoex_dhcp(net, dhdp, command);
		}
#else
#ifdef PKT_FILTER_SUPPORT
		uint mode = *(command + strlen(CMD_BTCOEXMODE) + 1) - '0';

		if (mode == 1)
			net_os_enable_packet_filter(net, 0); /* DHCP starts */
		else
			net_os_enable_packet_filter(net, 1); /* DHCP ends */
#endif /* PKT_FILTER_SUPPORT */
#endif /* WL_CFG80211 */
	}
	else if (strnicmp(command, CMD_SETSUSPENDOPT, strlen(CMD_SETSUSPENDOPT)) == 0) {
		bytes_written = wl_android_set_suspendopt(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETSUSPENDMODE, strlen(CMD_SETSUSPENDMODE)) == 0) {
		bytes_written = wl_android_set_suspendmode(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETBAND, strlen(CMD_SETBAND)) == 0) {
		uint band = *(command + strlen(CMD_SETBAND) + 1) - '0';
		bytes_written = wldev_set_band(net, band);
	}
	else if (strnicmp(command, CMD_GETBAND, strlen(CMD_GETBAND)) == 0) {
		bytes_written = wl_android_get_band(net, command, priv_cmd.total_len);
	}
#ifdef CONFIG_HW_VOWIFI
	else if (strnicmp(command, CMD_GET_MODE, strlen(CMD_GET_MODE)) == 0) {
		bytes_written = wl_android_get_vowifi(net, command, priv_cmd.total_len, CMD_VOWIFI_MODE);
	}
	else if (strnicmp(command, CMD_SET_MODE, strlen(CMD_SET_MODE)) == 0) {
		sscanf(command + strlen(CMD_SET_MODE) + 1, "%d", &value);
		bytes_written = wl_android_set_vowifi(net, command, priv_cmd.total_len, CMD_VOWIFI_MODE, value);
	}
	else if (strnicmp(command, CMD_GET_PERIOD, strlen(CMD_GET_PERIOD)) == 0) {
		bytes_written = wl_android_get_vowifi(net, command, priv_cmd.total_len, CMD_VOWIFI_PERIOD);
	}
	else if (strnicmp(command, CMD_SET_PERIOD, strlen(CMD_SET_PERIOD)) == 0) {
		sscanf(command + strlen(CMD_SET_PERIOD) + 1, "%d", &value);
		bytes_written = wl_android_set_vowifi(net, command, priv_cmd.total_len, CMD_VOWIFI_PERIOD, value);
	}
	else if (strnicmp(command, CMD_GET_LOW_THRESHOLD, strlen(CMD_GET_LOW_THRESHOLD)) == 0) {
		bytes_written = wl_android_get_vowifi(net, command, priv_cmd.total_len, CMD_VOWIFI_LOW_THRES);
	}
	else if (strnicmp(command, CMD_SET_LOW_THRESHOLD, strlen(CMD_SET_LOW_THRESHOLD)) == 0) {
		sscanf(command + strlen(CMD_SET_LOW_THRESHOLD) + 1, "%d", &value);
		bytes_written = wl_android_set_vowifi(net, command, priv_cmd.total_len, CMD_VOWIFI_LOW_THRES, value);
	}
	else if (strnicmp(command, CMD_GET_HIGH_THRESHOLD, strlen(CMD_GET_HIGH_THRESHOLD)) == 0) {
		bytes_written = wl_android_get_vowifi(net, command, priv_cmd.total_len, CMD_VOWIFI_HIGH_THRES);
	}
	else if (strnicmp(command, CMD_SET_HIGH_THRESHOLD, strlen(CMD_SET_HIGH_THRESHOLD)) == 0) {
		sscanf(command + strlen(CMD_SET_HIGH_THRESHOLD) + 1, "%d", &value);
		bytes_written = wl_android_set_vowifi(net, command, priv_cmd.total_len, CMD_VOWIFI_HIGH_THRES, value);
	}
	else if (strnicmp(command, CMD_GET_TRIGGER_COUNT, strlen(CMD_GET_TRIGGER_COUNT)) == 0) {
		bytes_written = wl_android_get_vowifi(net, command, priv_cmd.total_len, CMD_VOWIFI_TRIGGER_COUNT);
	}
	else if (strnicmp(command, CMD_SET_TRIGGER_COUNT, strlen(CMD_SET_TRIGGER_COUNT)) == 0) {
		sscanf(command + strlen(CMD_SET_TRIGGER_COUNT) + 1, "%d", &value);
		bytes_written = wl_android_set_vowifi(net, command, priv_cmd.total_len, CMD_VOWIFI_TRIGGER_COUNT, value);
	}
	else if (strnicmp(command, VOWIFI_IS_SUPPORT, strlen(VOWIFI_IS_SUPPORT)) == 0) {
		bytes_written = wl_android_vowifi_issupport(net, command, priv_cmd.total_len);
	}
#endif

#ifdef WL_CFG80211
	/* CUSTOMER_SET_COUNTRY feature is define for only GGSM model */
	else if (strnicmp(command, CMD_COUNTRY, strlen(CMD_COUNTRY)) == 0) {
		char *country_code = command + strlen(CMD_COUNTRY) + 1;
		printf(KERN_ERR "input country code %s.\n", country_code);
		bytes_written = wldev_set_country(net, country_code, true, true);
	}
#endif /* WL_CFG80211 */

#ifdef HW_DOZE_PKT_FILTER
	else if (strnicmp(command, CMD_FILTER_SWITCH, strlen(CMD_FILTER_SWITCH)) == 0) {
		wl_android_set_doze_filter(net,command);
	}
#endif

#ifdef PNO_SUPPORT
	else if (strnicmp(command, CMD_PNOSSIDCLR_SET, strlen(CMD_PNOSSIDCLR_SET)) == 0) {
		bytes_written = dhd_dev_pno_stop_for_ssid(net);
	}
#ifndef WL_SCHED_SCAN
	else if (strnicmp(command, CMD_PNOSETUP_SET, strlen(CMD_PNOSETUP_SET)) == 0) {
		bytes_written = wl_android_set_pno_setup(net, command, priv_cmd.total_len);
	}
#endif /* !WL_SCHED_SCAN */
	else if (strnicmp(command, CMD_PNOENABLE_SET, strlen(CMD_PNOENABLE_SET)) == 0) {
		int enable = *(command + strlen(CMD_PNOENABLE_SET) + 1) - '0';
		bytes_written = (enable)? 0 : dhd_dev_pno_stop_for_ssid(net);
	}
	else if (strnicmp(command, CMD_WLS_BATCHING, strlen(CMD_WLS_BATCHING)) == 0) {
		bytes_written = wls_parse_batching_cmd(net, command, priv_cmd.total_len);
	}
#endif /* PNO_SUPPORT */
	else if (strnicmp(command, CMD_P2P_DEV_ADDR, strlen(CMD_P2P_DEV_ADDR)) == 0) {
		bytes_written = wl_android_get_p2p_dev_addr(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_P2P_SET_NOA, strlen(CMD_P2P_SET_NOA)) == 0) {
		int skip = strlen(CMD_P2P_SET_NOA) + 1;
		bytes_written = wl_cfg80211_set_p2p_noa(net, command + skip,
			priv_cmd.total_len - skip);
	}
#if !defined WL_ENABLE_P2P_IF
	else if (strnicmp(command, CMD_P2P_GET_NOA, strlen(CMD_P2P_GET_NOA)) == 0) {
		bytes_written = wl_cfg80211_get_p2p_noa(net, command, priv_cmd.total_len);
	}
#endif /* WL_ENABLE_P2P_IF */
	else if (strnicmp(command, CMD_P2P_SET_PS, strlen(CMD_P2P_SET_PS)) == 0) {
		int skip = strlen(CMD_P2P_SET_PS) + 1;
		bytes_written = wl_cfg80211_set_p2p_ps(net, command + skip,
			priv_cmd.total_len - skip);
	}
#ifdef WL_CFG80211
	else if (strnicmp(command, CMD_SET_AP_WPS_P2P_IE,
		strlen(CMD_SET_AP_WPS_P2P_IE)) == 0) {
		int skip = strlen(CMD_SET_AP_WPS_P2P_IE) + 3;
		bytes_written = wl_cfg80211_set_wps_p2p_ie(net, command + skip,
			priv_cmd.total_len - skip, *(command + skip - 2) - '0');
	}
#endif /* WL_CFG80211 */
	else if (strnicmp(command, CMD_OKC_SET_PMK, strlen(CMD_OKC_SET_PMK)) == 0)
		bytes_written = wl_android_set_pmk(net, command, priv_cmd.total_len);
	else if (strnicmp(command, CMD_OKC_ENABLE, strlen(CMD_OKC_ENABLE)) == 0)
		bytes_written = wl_android_okc_enable(net, command, priv_cmd.total_len);
#if defined(WL_SUPPORT_AUTO_CHANNEL)
	else if (strnicmp(command, CMD_GET_BEST_CHANNELS,
		strlen(CMD_GET_BEST_CHANNELS)) == 0) {
		bytes_written = wl_cfg80211_get_best_channels(net, command,
			priv_cmd.total_len);
	}
#endif /* WL_SUPPORT_AUTO_CHANNEL */
	else if (strnicmp(command, CMD_HAPD_MAC_FILTER, strlen(CMD_HAPD_MAC_FILTER)) == 0) {
		int skip = strlen(CMD_HAPD_MAC_FILTER) + 1;
		wl_android_set_mac_address_filter(net, (const char*)command+skip);
	}
	else if (strnicmp(command, CMD_SETROAMMODE, strlen(CMD_SETROAMMODE)) == 0)
		bytes_written = wl_android_set_roam_mode(net, command, priv_cmd.total_len);
#if defined(BCMFW_ROAM_ENABLE)
	else if (strnicmp(command, CMD_SET_ROAMPREF, strlen(CMD_SET_ROAMPREF)) == 0) {
		bytes_written = wl_android_set_roampref(net, command, priv_cmd.total_len);
	}
#endif /* BCMFW_ROAM_ENABLE */
	else if (strnicmp(command, CMD_MIRACAST, strlen(CMD_MIRACAST)) == 0)
		bytes_written = wl_android_set_miracast(net, command, priv_cmd.total_len);
	else if (strnicmp(command, CMD_SETIBSSBEACONOUIDATA, strlen(CMD_SETIBSSBEACONOUIDATA)) == 0)
		bytes_written = wl_android_set_ibss_beacon_ouidata(net,
		command, priv_cmd.total_len);
	else if (strnicmp(command, CMD_KEEP_ALIVE, strlen(CMD_KEEP_ALIVE)) == 0) {
		int skip = strlen(CMD_KEEP_ALIVE) + 1;
		bytes_written = wl_keep_alive_set(net, command + skip, priv_cmd.total_len - skip);
	}
	else if (strnicmp(command, CMD_ROAM_OFFLOAD, strlen(CMD_ROAM_OFFLOAD)) == 0) {
		int enable = *(command + strlen(CMD_ROAM_OFFLOAD) + 1) - '0';
		bytes_written = wl_cfg80211_enable_roam_offload(net, enable);
	}
#ifdef HW_SET_PM
	else if(strnicmp(command, CMD_POWER_LOCK, strlen(CMD_POWER_LOCK)) == 0) {
		int enable = *(command + strlen(CMD_POWER_LOCK) + 1) - '0';
		bytes_written = wl_cfg80211_powerlock(net, enable);
	}
#endif
#ifdef BCM_BLOCK_DATA_FRAME
	else if (strnicmp(command, CMD_SET_BLOCKFRAME, strlen(CMD_SET_BLOCKFRAME)) == 0) {
		int skip = strlen(CMD_SET_BLOCKFRAME) + 1;
		bytes_written = wl_cfg80211_set_blockframe(net, command + skip, priv_cmd.total_len - skip);
	}
#endif
#ifdef BRCM_RSDB
	else if (strnicmp(command, CMD_CAPAB_RSDB, strlen(CMD_CAPAB_RSDB)) == 0) {
		dhd_pub = hw_get_dhd_pub(net);
		if (NULL != dhd_pub && priv_cmd.total_len > 0) {
			bytes_written = snprintf(command, priv_cmd.total_len, "RSDB:%d", dhd_pub->rsdb_mode);
		}
	}
#endif
#ifdef HUAWEI_ANDROID_EXTENSION
        else if (strnicmp(command, CMD_AP_SET_CFG, strlen(CMD_AP_SET_CFG)) == 0) {
            int skip = strlen(CMD_AP_SET_CFG) + 1;
            bytes_written = wl_android_set_ap_config(net, command+skip, priv_cmd.total_len-skip);
        }
        else if (strnicmp(command, CMD_AP_GET_STA_LIST, strlen(CMD_AP_GET_STA_LIST)) == 0) {
            bytes_written = wl_android_get_assoc_list(net, command, priv_cmd.total_len);
        }
       else if (strnicmp(command, CMD_AP_SET_MAC_FLTR, strlen(CMD_AP_SET_MAC_FLTR)) == 0) {
            int skip = strlen(CMD_AP_SET_MAC_FLTR) + 1;
            bytes_written = wl_android_set_mac_filters(net, command+skip, priv_cmd.total_len-skip);
        }
        else if (strnicmp(command, CMD_AP_STA_DISASSOC, strlen(CMD_AP_STA_DISASSOC)) == 0) {
            int skip = strlen(CMD_AP_STA_DISASSOC) + 1;
            bytes_written = wl_android_set_ap_sta_disassoc(net, command+skip, priv_cmd.total_len-skip);
        }
        else if (strnicmp(command, CMD_SET_TX_POWER, strlen(CMD_SET_TX_POWER)) == 0) {
            int skip = strlen(CMD_SET_TX_POWER) + 1;
            bytes_written = wl_android_set_txpow(net, command+skip, priv_cmd.total_len-skip);
        }
        else if (strnicmp(command, CMD_SET_DISASSOC_ROAMING_BSSID, strlen(CMD_SET_DISASSOC_ROAMING_BSSID)) == 0) {
            int skip = strlen(CMD_SET_DISASSOC_ROAMING_BSSID) + 1;
            bytes_written = wl_android_set_disassoc_roaming_bssid(net, command+skip, priv_cmd.total_len-skip);
        }
#endif
	else {
		DHD_ERROR(("Unknown PRIVATE command %s - ignored\n", command));
		snprintf(command, 3, "OK");
		bytes_written = strlen("OK");
	}

	if (bytes_written >= 0) {
		if ((bytes_written == 0) && (priv_cmd.total_len > 0))
			command[0] = '\0';
		if (bytes_written >= priv_cmd.total_len) {
			DHD_ERROR(("%s: bytes_written = %d\n", __FUNCTION__, bytes_written));
			bytes_written = priv_cmd.total_len;
		} else {
			bytes_written++;
		}
		priv_cmd.used_len = bytes_written;
		if (copy_to_user(priv_cmd.buf, command, bytes_written)) {
			DHD_ERROR(("%s: failed to copy data to user buffer\n", __FUNCTION__));
			ret = -EFAULT;
		}
	}
	else {
		ret = bytes_written;
	}

exit:
	net_os_wake_unlock(net);
	if (command) {
		kfree(command);
	}

	return ret;
}

int wl_android_init(void)
{
	int ret = 0;

#ifdef ENABLE_INSMOD_NO_FW_LOAD
	dhd_download_fw_on_driverload = FALSE;
#endif /* ENABLE_INSMOD_NO_FW_LOAD */
#if defined(CUSTOMER_HW2)
	if (!iface_name[0]) {
		memset(iface_name, 0, IFNAMSIZ);
		bcm_strncpy_s(iface_name, IFNAMSIZ, "wlan", IFNAMSIZ);
	}
#endif


	return ret;
}

int wl_android_exit(void)
{
	int ret = 0;
	struct io_cfg *cur, *q;


	list_for_each_entry_safe(cur, q, &miracast_resume_list, list) {
		list_del(&cur->list);
		kfree(cur);
	}

	return ret;
}

void wl_android_post_init(void)
{

#ifdef ENABLE_4335BT_WAR
	bcm_bt_unlock(lock_cookie_wifi);
	printf("%s: btlock released\n", __FUNCTION__);
#endif /* ENABLE_4335BT_WAR */

	if (!dhd_download_fw_on_driverload)
		g_wifi_on = FALSE;
}
