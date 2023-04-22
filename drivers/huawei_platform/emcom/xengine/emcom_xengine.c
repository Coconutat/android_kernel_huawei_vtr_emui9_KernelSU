#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/time.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <net/ip.h>
#include <net/netlink.h>
#include <net/inet_connection_sock.h>
#include <net/tcp_states.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <uapi/linux/netlink.h>
#include <linux/kthread.h>
#include <linux/netdevice.h>
#include <linux/spinlock.h>
#include <net/pkt_sched.h>
#include <net/sch_generic.h>
#include "../emcom_netlink.h"
#include "../emcom_utils.h"
#include <huawei_platform/emcom/network_evaluation.h>

#ifdef CONFIG_HUAWEI_BASTET
#include <huawei_platform/net/bastet/bastet_utils.h>
#endif
#include <huawei_platform/emcom/emcom_xengine.h>
#include <linux/version.h>
#include <asm/uaccess.h>


#undef HWLOG_TAG
#define HWLOG_TAG emcom_xengine
HWLOG_REGIST();
MODULE_LICENSE("GPL");


#define     EMCOM_MAX_ACC_APP  (5)
#define     EMCOM_UID_ACC_AGE_MAX  (1000)

#define     EMCOM_SPEED_CTRL_BASE_WIN_SIZE   (10000)


#ifdef CONFIG_HUAWEI_BASTET_COMM
	extern int bastet_comm_keypsInfo_write(uint32_t ulState);
#endif

struct Emcom_Xengine_acc_app_info     g_CurrentUids[EMCOM_MAX_ACC_APP];
struct Emcom_Xengine_speed_ctrl_info  g_SpeedCtrlInfo;

struct sk_buff_head g_UdpSkbList;
struct timer_list   g_UdpSkb_timer;
uid_t  g_UdpRetranUid;
bool   g_Emcom_udptimerOn = false;
#ifdef CONFIG_SMART_MP
struct Emcom_Xengine_SmartMpInfo {
	struct list_head list;
	uint32_t uid;
	uint32_t mark;
};

static LIST_HEAD(smartmp_list);
#endif
uid_t g_FastSynUid;
#define FAST_SYN_COUNT (5)
#define EMCOM_UDPRETRAN_NODELAY
#define UDPTIMER_DELAY  (4)
#define EMCOM_MAX_UDP_SKB  (20)
#define MIN_JIFFIE         1
struct Emcom_Xengine_netem_skb_cb {
	psched_time_t    time_to_send;
	ktime_t          tstamp_save;
};

struct mutex g_Mpip_mutex;
struct  Emcom_Xengine_mpip_config g_MpipUids[EMCOM_MAX_MPIP_APP];/* The uid of bind to Mpip Application */
bool    g_MpipStart               = false;/* The uid of bind to Mpip Application */
char    g_Ifacename[IFNAMSIZ]     = {0};/* The uid of bind to Mpip Application */
static uint8_t g_SocketIndex      = 0;


void Emcom_Xengine_Mpip_Init(void);
/******************************************************************************
   6 º¯ÊýÊµÏÖ
******************************************************************************/
static inline bool invalid_uid(uid_t uid)
{
	/* if uid less than 10000, it is not an Android apk */
	return (uid < UID_APP);
}

static inline bool invalid_SpeedCtrlSize(uint32_t grade)
{
	/* the speed control grade bigger than 10000 */
	return (grade < EMCOM_SPEED_CTRL_BASE_WIN_SIZE);
}



static inline struct Emcom_Xengine_netem_skb_cb *Emcom_Xengine_netem_skb_cb(struct sk_buff *skb)
{
	/* we assume we can use skb next/prev/tstamp as storage for rb_node */
	qdisc_cb_private_validate(skb, sizeof(struct Emcom_Xengine_netem_skb_cb));
	return (struct Emcom_Xengine_netem_skb_cb *)qdisc_skb_cb(skb)->data;
}

#ifndef EMCOM_UDPRETRAN_NODELAY

static void Emcom_Xengine_setUdpTimerCb(struct sk_buff *skb)
{
	struct Emcom_Xengine_netem_skb_cb *cb;
	unsigned long now;
	now = jiffies;
	cb = Emcom_Xengine_netem_skb_cb(skb);
	/* translate to jiffies */
	cb->time_to_send = now + UDPTIMER_DELAY*HZ/MSEC_PER_SEC;
}
#endif


int Emcom_Xengine_udpretran_clear(void)
{
	g_UdpRetranUid = UID_INVALID_APP;
	skb_queue_purge(&g_UdpSkbList);
	if(g_Emcom_udptimerOn)
	{
		del_timer(&g_UdpSkb_timer);
		g_Emcom_udptimerOn = false;
	}
	return 0;
}


static void Emcom_Xengine_UdpTimer_handler(unsigned long pac)
{
	struct sk_buff *skb;
	unsigned long now;
	struct Emcom_Xengine_netem_skb_cb *cb;
	int jiffie_n;

	/* anyway, send out the first skb */
    if(!skb_queue_empty(&g_UdpSkbList))
	{
		skb = skb_dequeue(&g_UdpSkbList);
		if(skb)
		{
			dev_queue_xmit(skb);
			EMCOM_LOGD("Emcom_Xengine_UdpTimer_handler send skb\n");
		}
	}

	skb = skb_peek(&g_UdpSkbList);
	if(!skb)
	{
		goto timer_off;
		return;
	}
	cb = Emcom_Xengine_netem_skb_cb(skb);
	now = jiffies;
	/* if remaining time is little than 1 jiffie, send out */
	while(cb->time_to_send <= now + MIN_JIFFIE)
	{
		EMCOM_LOGD("Emcom_Xengine_UdpTimer_handler send another skb\n");
		skb = skb_dequeue(&g_UdpSkbList);
		if(skb)
		{
			dev_queue_xmit(skb);
		}
		skb = skb_peek(&g_UdpSkbList);
		if(!skb)
		{
			goto timer_off;
			return;
		}
		cb = Emcom_Xengine_netem_skb_cb(skb);
		now = jiffies;
	}
	/* set timer based on next skb cb */
	now = jiffies;
	jiffie_n = cb->time_to_send - now;

	if(jiffie_n < MIN_JIFFIE)
	{
		jiffie_n = MIN_JIFFIE;
	}
	EMCOM_LOGD("Emcom_Xengine_UdpTimer_handler modify timer hz %d\n", jiffie_n);
	mod_timer(&g_UdpSkb_timer, jiffies + jiffie_n);
	g_Emcom_udptimerOn = true;
	return;

timer_off:
	g_Emcom_udptimerOn = false;
}


void Emcom_Xengine_Init(void)
{
	uint8_t  index;
	for( index = 0; index < EMCOM_MAX_ACC_APP; index ++)
	{
		g_CurrentUids[index].lUid = UID_INVALID_APP;
		g_CurrentUids[index].ulAge = 0;
	}
	g_SpeedCtrlInfo.lUid = UID_INVALID_APP;
	g_SpeedCtrlInfo.ulSize = 0;
	spin_lock_init(&g_SpeedCtrlInfo.stLocker);
	g_UdpRetranUid = UID_INVALID_APP;
	g_Emcom_udptimerOn = false;
	skb_queue_head_init(&g_UdpSkbList);
	init_timer(&g_UdpSkb_timer);
	g_UdpSkb_timer.function = Emcom_Xengine_UdpTimer_handler;
	mutex_init(&g_Mpip_mutex);
	Emcom_Xengine_Mpip_Init();
	g_FastSynUid = UID_INVALID_APP;
}


void Emcom_Xengine_Mpip_Init(void)
{
	uint8_t  uIndex;
	mutex_lock(&g_Mpip_mutex);
	for( uIndex = 0; uIndex < EMCOM_MAX_MPIP_APP; uIndex ++)
	{
		g_MpipUids[uIndex].lUid = UID_INVALID_APP;
		g_MpipUids[uIndex].ulType = EMCOM_XENGINE_MPIP_TYPE_BIND_NEW;
	}
	mutex_unlock(&g_Mpip_mutex);
}


bool Emcom_Xengine_IsAccUid(uid_t lUid)
{
	uint8_t  index;
	for( index = 0; index < EMCOM_MAX_ACC_APP; index ++)
	{
		if( lUid == g_CurrentUids[index].lUid )
		{
			return true;
		}
	}

	return false;
}

#if defined(CONFIG_PPPOLAC) || defined(CONFIG_PPPOPNS)


bool Emcom_Xengine_Hook_Ul_Stub(struct sock *pstSock)
{
	uid_t lSockUid = 0;
	bool  bFound   = false;

	if(( NULL == pstSock ) )
	{
		EMCOM_LOGD("Emcom_Xengine_Hook_Ul_Stub param invalid\n");
		return false;
	}

	/**
	 * if uid equals current acc uid, accelerate it,else stop it
	 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 10)
	lSockUid = sock_i_uid(pstSock).val;
#else
	lSockUid = sock_i_uid(pstSock);
#endif

	if( invalid_uid ( lSockUid ))
	{
		return false;
	}

	bFound = Emcom_Xengine_IsAccUid ( lSockUid );

	return bFound;
}
#endif



int Emcom_Xengine_clear(void)
{
	uint8_t  index;
	for( index = 0; index < EMCOM_MAX_ACC_APP; index ++)
	{
		g_CurrentUids[index].lUid = UID_INVALID_APP;
		g_CurrentUids[index].ulAge = 0;
	}
	mutex_lock(&g_Mpip_mutex);
	for( index = 0; index < EMCOM_MAX_MPIP_APP; index ++)
	{
		g_MpipUids[index].lUid = UID_INVALID_APP;
		g_MpipUids[index].ulType = EMCOM_XENGINE_MPIP_TYPE_BIND_NEW;
	}
	memset(g_Ifacename, 0, sizeof(char)*IFNAMSIZ);
	g_MpipStart = false;
	mutex_unlock(&g_Mpip_mutex);
	Emcom_Xengine_udpretran_clear();
	EMCOM_XENGINE_SetSpeedCtrl(g_SpeedCtrlInfo, UID_INVALID_APP, 0);
	g_FastSynUid = UID_INVALID_APP;
	return 0;
}



int Emcom_Xengine_StartAccUid(uint8_t *pdata, uint16_t len)
{
	uid_t              uid;
	uint8_t            index;
	uint8_t            ucIdleIndex;
	uint8_t            ucOldIndex;
	uint8_t            ucOldAge;
	bool               bFound;
	/*input param check*/
	if( NULL == pdata )
	{
		EMCOM_LOGE("Emcom_Xengine_StartAccUid:data is null");
		return -EINVAL;
	}

	/*check len is invalid*/
	if(len != sizeof(uid_t))
	{
		EMCOM_LOGI("Emcom_Xengine_StartAccUid: len:%d is illegal", len);
		return -EINVAL;
	}

	uid =*(uid_t *)pdata;

	/*check uid*/
	if (invalid_uid(uid))
		return -EINVAL;

	EMCOM_LOGD("Emcom_Xengine_StartAccUid: uid:%d ready to added", uid);
	ucIdleIndex = EMCOM_MAX_ACC_APP;
	ucOldIndex  = EMCOM_MAX_ACC_APP;
	ucOldAge    = 0;
	bFound  = false;

	/*check whether has the same uid, and  record the first idle position and the oldest position*/
	for( index = 0; index < EMCOM_MAX_ACC_APP; index ++)
	{
		if( UID_INVALID_APP == g_CurrentUids[index].lUid )
		{
			if( EMCOM_MAX_ACC_APP == ucIdleIndex )
			{
				ucIdleIndex  = index;
			}
		}
		else if( uid == g_CurrentUids[index].lUid )
		{
			g_CurrentUids[index].ulAge = 0;
			bFound = true;
		}
		else
		{
			g_CurrentUids[index].ulAge ++;
			if( g_CurrentUids[index].ulAge > ucOldAge )
			{
				ucOldAge    = g_CurrentUids[index].ulAge;
				ucOldIndex  = index ;
			}

		}
	}

	/*remove the too old acc uid*/
	if(ucOldAge  > EMCOM_UID_ACC_AGE_MAX )
	{
		EMCOM_LOGD("Emcom_Xengine_StartAccUid: uid:%d added too long, remove it", g_CurrentUids[ucOldIndex].lUid );
		g_CurrentUids[ucOldIndex].ulAge = 0;
		g_CurrentUids[ucOldIndex].lUid  = UID_INVALID_APP;
	}

	EMCOM_LOGD("Emcom_Xengine_StartAccUid: ucIdleIndex=%d,ucOldIndex=%d,ucOldAge=%d",ucIdleIndex, ucOldIndex,ucOldAge);

	/*if has already added, return*/
	if(bFound)
	{
		EMCOM_LOGD("Emcom_Xengine_StartAccUid: uid:%d already added", uid);
		return 0;
	}

	/*if it is new uid, and has idle position , add it*/
	if( ucIdleIndex < EMCOM_MAX_ACC_APP )
	{
		EMCOM_LOGD("Emcom_Xengine_StartAccUid: uid:%d added", uid);
		g_CurrentUids[ucIdleIndex].ulAge = 0;
		g_CurrentUids[ucIdleIndex].lUid = uid;
		return 0;
	}


	/*if it is new uid, and acc list if full , replace the oldest*/
	if( ucOldIndex < EMCOM_MAX_ACC_APP )
	{
		EMCOM_LOGD("Emcom_Xengine_StartAccUid: uid:%d replace the oldest uid:%d", uid,g_CurrentUids[ucOldIndex].lUid);
		g_CurrentUids[ucOldIndex].ulAge = 0;
		g_CurrentUids[ucOldIndex].lUid = uid;
		return 0;
	}

	return 0;
}




int Emcom_Xengine_StopAccUid(uint8_t *pdata, uint16_t len)
{
	uid_t              uid;
	uint8_t            index;

	/*input param check*/
	if( NULL == pdata )
	{
		EMCOM_LOGE("Emcom_Xengine_StopAccUid:data is null");
		return -EINVAL;
	}

	/*check len is invalid*/
	if(len != sizeof(uid_t))
	{
		EMCOM_LOGI("Emcom_Xengine_StopAccUid: len: %d is illegal", len);
		return -EINVAL;
	}

	uid =*(uid_t *)pdata;

	/*check uid*/
	if (invalid_uid(uid))
		return -EINVAL;

	/*remove specify uid*/
	for( index = 0; index < EMCOM_MAX_ACC_APP; index ++)
	{
		if( uid == g_CurrentUids[index].lUid )
		{
			g_CurrentUids[index].ulAge = 0;
			g_CurrentUids[index].lUid  = UID_INVALID_APP;
			EMCOM_LOGD("Emcom_Xengine_StopAccUid:lUid:%d",uid);
			break;
		}
	}

	return 0;
}


int Emcom_Xengine_SetSpeedCtrlInfo(uint8_t *pdata, uint16_t len)
{
	struct Emcom_Xengine_speed_ctrl_data* pSpeedCtrlInfo;
	uid_t              lUid;
	uint32_t           ulSize;

	/*input param check*/
	if( NULL == pdata )
	{
		EMCOM_LOGE("Emcom_Xengine_SetSpeedCtrlInfo:data is null");
		return -EINVAL;
	}

	/*check len is invalid*/
	if(len != sizeof(struct Emcom_Xengine_speed_ctrl_data))
	{
		EMCOM_LOGI("Emcom_Xengine_SetSpeedCtrlInfo: len:%d is illegal", len);
		return -EINVAL;
	}

	pSpeedCtrlInfo = (struct Emcom_Xengine_speed_ctrl_data *)pdata;
	lUid = pSpeedCtrlInfo->lUid;
	ulSize = pSpeedCtrlInfo->ulSize;

	/* if uid and size is zero, clear the speed control info */
	if(!lUid && !ulSize)
	{
		EMCOM_LOGD("Emcom_Xengine_SetSpeedCtrlInfo: clear speed ctrl state");
		EMCOM_XENGINE_SetSpeedCtrl(g_SpeedCtrlInfo, UID_INVALID_APP, 0);
		return 0;
	}

	/*check uid*/
	if (invalid_uid(lUid))
	{
		EMCOM_LOGI("Emcom_Xengine_SetSpeedCtrlInfo: uid:%d is illegal", lUid);
		return -EINVAL;
	}

	/*check size*/
	if (invalid_SpeedCtrlSize(ulSize))
	{
		EMCOM_LOGI("Emcom_Xengine_SetSpeedCtrlInfo: size:%d is illegal", ulSize);
		return -EINVAL;
	}

	EMCOM_LOGD("Emcom_Xengine_SetSpeedCtrlInfo: uid:%d size:%d", lUid, ulSize);
	EMCOM_XENGINE_SetSpeedCtrl(g_SpeedCtrlInfo, lUid, ulSize);
	return 0;
}



void Emcom_Xengine_SpeedCtrl_WinSize(struct sock *pstSock, uint32_t *pstSize)
{
	uid_t lSockUid = 0;
	uid_t lUid = 0;
	uint32_t ulSize = 0;

	if( NULL == pstSock )
	{
		EMCOM_LOGD("Emcom_Xengine_Hook_Ul_Stub param invalid\n");
		return;
	}

	if( NULL == pstSize )
	{
		EMCOM_LOGD(" Emcom_Xengine_SpeedCtrl_WinSize window size invalid\n");
		return;
	}

	EMCOM_XENGINE_GetSpeedCtrlUid(g_SpeedCtrlInfo, lUid);
	if( invalid_uid ( lUid ))
	{
		return;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 10)
	lSockUid = sock_i_uid(pstSock).val;
#else
	lSockUid = sock_i_uid(pstSock);
#endif

	if( invalid_uid ( lSockUid ))
	{
		return;
	}

	EMCOM_XENGINE_GetSpeedCtrlInfo(g_SpeedCtrlInfo, lUid, ulSize);
	/* check uid */
	if( lSockUid == lUid)
	{
		return;
	}

	if (ulSize)
	{
		*pstSize = g_SpeedCtrlInfo.ulSize < *pstSize ? g_SpeedCtrlInfo.ulSize : *pstSize;
	}

}




int Emcom_Xengine_Config_MPIP(uint8_t *pdata, uint16_t len)
{
	uint8_t            uIndex;
	uint8_t            *ptemp;
	uint8_t            ulength;
	/*The empty updated list means clear the Mpip App Uid list*/
	EMCOM_LOGD("The Mpip list will be update to empty.");

	/*Clear the Mpip App Uid list*/
	mutex_lock(&g_Mpip_mutex);
	for( uIndex = 0; uIndex < EMCOM_MAX_MPIP_APP; uIndex ++)
	{
		g_MpipUids[uIndex].lUid = UID_INVALID_APP;
		g_MpipUids[uIndex].ulType = EMCOM_XENGINE_MPIP_TYPE_BIND_NEW;
	}
	mutex_unlock(&g_Mpip_mutex);
	if((NULL == pdata) || (0 == len))
	{
		/*pdata == NULL or len == 0 is ok, just return*/
		return 0;
	}
	ptemp = pdata;
	ulength = len/sizeof(struct Emcom_Xengine_mpip_config);
	if(EMCOM_MAX_MPIP_APP < ulength )
	{
		EMCOM_LOGE("The length of received MPIP APP uid list is error.");
		return -EINVAL;
	}
	mutex_lock(&g_Mpip_mutex);
	for(uIndex = 0; uIndex < ulength; uIndex++)
	{
		g_MpipUids[uIndex].lUid = *(uid_t *)ptemp;
		g_MpipUids[uIndex].ulType = *(uint32_t*)(ptemp + sizeof(uid_t));
		EMCOM_LOGD("The Mpip config [%d] is: lUid %d and type %d.",uIndex, g_MpipUids[uIndex].lUid, g_MpipUids[uIndex].ulType);
		ptemp += sizeof(struct Emcom_Xengine_mpip_config);
	}
	mutex_unlock(&g_Mpip_mutex);

	return 0;
}


int Emcom_Xengine_Clear_Mpip_Config(uint8_t *pdata, uint16_t len)
{
	uint8_t            uIndex;
	uint8_t            *ptemp;
	uint8_t            ulength;
	/*The empty updated list means clear the Mpip App Uid list*/
	EMCOM_LOGD("The Mpip list will be update to empty.");

	/*Clear the Mpip App Uid list*/
	mutex_lock(&g_Mpip_mutex);
	for( uIndex = 0; uIndex < EMCOM_MAX_MPIP_APP; uIndex ++)
	{
		g_MpipUids[uIndex].lUid = UID_INVALID_APP;
		g_MpipUids[uIndex].ulType = EMCOM_XENGINE_MPIP_TYPE_BIND_NEW;
	}
	mutex_unlock(&g_Mpip_mutex);

	return 0;
}






int Emcom_Xengine_StartMPIP(char *pdata, uint16_t len)
{
	/*input param check*/
	if( (NULL == pdata) || (0 == len) || (IFNAMSIZ < len) )
	{
	    EMCOM_LOGE("MPIP interface name or length %d is error", len);
		return -EINVAL;
	}
	mutex_lock(&g_Mpip_mutex);
	memcpy (g_Ifacename, pdata, len);
	g_MpipStart = true;
	mutex_unlock(&g_Mpip_mutex);
	EMCOM_LOGD("Mpip is :%d to start.", g_MpipStart);
	return 0;
}




int Emcom_Xengine_StopMPIP(uint8_t *pdata, uint16_t len)
{
	mutex_lock(&g_Mpip_mutex);
	g_MpipStart = false;
	mutex_unlock(&g_Mpip_mutex);
	EMCOM_LOGD("MPIP function is :%d, ready to stop", g_MpipStart);

	return 0;
}


int Emcom_Xengine_IsMpipBindUid(uid_t lUid)
{
	int ret = -1;
	uint8_t  uIndex;
	mutex_lock(&g_Mpip_mutex);
	for( uIndex = 0; uIndex < EMCOM_MAX_MPIP_APP; uIndex ++)
	{
		if( lUid == g_MpipUids[uIndex].lUid )
		{
			mutex_unlock(&g_Mpip_mutex);
			ret = uIndex;
			return ret;
		}
	}
	mutex_unlock(&g_Mpip_mutex);

	return ret;
}


void Emcom_Xengine_Mpip_Bind2Device(struct sock *pstSock)
{
	int iFound             = -1;
	uint8_t  uIndex        = 0;
	uid_t lSockUid         = 0;
	struct net *net        = NULL;
	struct net_device *dev = NULL;

	if(NULL == pstSock)
	{
		EMCOM_LOGE(" param invalid.\n");
		return;
	}

	if(!g_MpipStart)
	{
		return;
	}
	/**
	 * if uid equals current bind uid, bind 2 device
	 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 10)
	lSockUid = sock_i_uid(pstSock).val;
#else
	lSockUid = sock_i_uid(pstSock);
#endif

	if( invalid_uid ( lSockUid ))
	{
		return;
	}

	net = sock_net(pstSock);
	iFound = Emcom_Xengine_IsMpipBindUid( lSockUid );
	if(iFound != -1)
	{
		rcu_read_lock();
		dev = dev_get_by_name_rcu(net, g_Ifacename);
		if(dev)
		{
            uIndex = dev->ifindex;
		}
		rcu_read_unlock();
		if ((!dev) || (!test_bit(__LINK_STATE_START, &dev->state)))
		{
			g_MpipStart = false;
			emcom_send_msg2daemon(NETLINK_EMCOM_KD_XENIGE_DEV_FAIL, NULL, 0);
			EMCOM_LOGE(" get dev fail or dev is not up.\n");
			return;
		}

		if(g_MpipUids[iFound].ulType == EMCOM_XENGINE_MPIP_TYPE_BIND_RANDOM)
		{
			if(g_SocketIndex % 2 == 0)
			{
				lock_sock(pstSock);
				pstSock->sk_bound_dev_if = uIndex;
				sk_dst_reset(pstSock);
				release_sock(pstSock);
			}
			g_SocketIndex++;
			g_SocketIndex = g_SocketIndex % 2;
		}
		else
		{
			lock_sock(pstSock);
			pstSock->sk_bound_dev_if = uIndex;
			sk_dst_reset(pstSock);
			release_sock(pstSock);
		}
	}
}



int Emcom_Xengine_RrcKeep( void )
{
#ifdef CONFIG_HUAWEI_BASTET
	post_indicate_packet(BST_IND_RRC_KEEP,NULL,0);
#endif
	return 0;
}




int Emcom_Send_KeyPsInfo(uint8_t *pdata, uint16_t len)
{
	uint32_t            ulState;

	/*input param check*/
	if( NULL == pdata )
	{
		EMCOM_LOGE("Emcom_Send_KeyPsInfo:data is null");
		return -EINVAL;
	}

	/*check len is invalid*/
	if( len < sizeof( uint32_t ) )
	{
		EMCOM_LOGE("Emcom_Send_KeyPsInfo: len: %d is illegal", len);
		return -EINVAL;
	}

	ulState =*(uint32_t *)pdata;

	if( true != Emcom_Is_Modem_Support() )
	{
		EMCOM_LOGI( "Emcom_Send_KeyPsInfo: modem not support" );
		return -EINVAL;
	}

#ifdef CONFIG_HUAWEI_BASTET_COMM
	bastet_comm_keypsInfo_write( ulState );
#endif
	return 0;
}



static inline bool Emcom_Xengine_isWlan(struct sk_buff *skb)
{
	const char *delim = "wlan0";
	int len = strlen(delim);
	if(!skb->dev)
	{
		return false;
	}

	if (strncmp(skb->dev->name, delim, len))
	{
		return false;
	}

	return true;
}



void Emcom_Xengine_UdpEnqueue(struct sk_buff *skb)
{
	struct sock *sk;
	struct sk_buff *skb2;
	uid_t lSockUid = UID_INVALID_APP;
	/* invalid g_UdpRetranUid means UDP retran is closed */

	if(invalid_uid(g_UdpRetranUid))
	{
		return;
	}

	if((!skb))
	{
		EMCOM_LOGE("Emcom_Xengine_UdpEnqueue skb null");
		return;
	}
	if(g_UdpSkbList.qlen >= EMCOM_MAX_UDP_SKB)
	{
		EMCOM_LOGE("Emcom_Xengine_UdpEnqueue max skb");
		return;
	}

	sk = skb_to_full_sk(skb);
	if (unlikely(!sk))
	{
		EMCOM_LOGE("Emcom_Xengine_UdpEnqueue sk null");
		return;
	}

	if (unlikely(!sk->sk_socket))
	{
		EMCOM_LOGE("Emcom_Xengine_UdpEnqueue sk_socket null");
		return;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 10)
	lSockUid = sock_i_uid(sk).val;
#else
	lSockUid = sock_i_uid(sk);
#endif
	if(lSockUid == g_UdpRetranUid)
	{
		if(!Emcom_Xengine_isWlan(skb))
		{
			EMCOM_LOGD("Emcom_Xengine_UdpEnqueue not wlan");
			Emcom_Xengine_udpretran_clear();
			return;
		}
		if(sk->sk_socket->type == SOCK_DGRAM)
		{
			skb2 = skb_copy(skb, GFP_ATOMIC);
			if(unlikely(!skb2))
			{
				EMCOM_LOGE("Emcom_Xengine_UdpEnqueue skb2 null");
				return;
			}
#ifdef EMCOM_UDPRETRAN_NODELAY
			dev_queue_xmit(skb2);
			return;
#else
			skb_queue_tail(&g_UdpSkbList,skb2);
			Emcom_Xengine_setUdpTimerCb(skb2);
			if(!g_Emcom_udptimerOn)
			{
				skb2 = skb_peek(&g_UdpSkbList);
				if(!skb2)
				{
					EMCOM_LOGE("Emcom_Xengine_UdpEnqueue peek skb2 null");
					return;
				}
				g_Emcom_udptimerOn = true;
				g_UdpSkb_timer.expires = jiffies + UDPTIMER_DELAY*HZ/MSEC_PER_SEC;
				EMCOM_LOGD("Emcom_Xengine_UdpEnqueue: jiffie %d",UDPTIMER_DELAY*HZ/MSEC_PER_SEC);
				add_timer(&g_UdpSkb_timer);
			}
#endif
		}
	}
}



int Emcom_Xengine_StartUdpReTran(uint8_t *pdata, uint16_t len)
{
	uid_t              uid;

	/*input param check*/
	if( NULL == pdata )
	{
		EMCOM_LOGE("Emcom_Xengine_StartUdpReTran:data is null");
		return -EINVAL;
	}

	/*check len is invalid*/
	if(len != sizeof(uid_t))
	{
		EMCOM_LOGI("Emcom_Xengine_StartUdpReTran: len: %d is illegal", len);
		return -EINVAL;
	}

	uid =*(uid_t *)pdata;
	/*check uid*/
	if (invalid_uid(uid))
	{
		EMCOM_LOGE("Emcom_Xengine_StartUdpReTran: uid is invalid %d", uid);
		return -EINVAL;
	}
	EMCOM_LOGI("Emcom_Xengine_StartUdpReTran: uid: %d ", uid);
	g_UdpRetranUid = uid;
	return 0;
}


int Emcom_Xengine_StopUdpReTran(uint8_t *pdata, uint16_t len)
{
	Emcom_Xengine_udpretran_clear();
	return 0;
}



void Emcom_Xengine_FastSyn(struct sock *pstSock)
{
	uid_t lSockUid = 0;
	struct inet_connection_sock *icsk;

	if( NULL == pstSock )
	{
		EMCOM_LOGD(" Emcom_Xengine_FastSyn param invalid\n");
		return;
	}
	if( pstSock->sk_state != TCP_SYN_SENT )
	{
		return;
	}

	if( invalid_uid ( g_FastSynUid ))
	{
		return;
	}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 10)
	lSockUid = sock_i_uid(pstSock).val;
#else
	lSockUid = sock_i_uid(pstSock);
#endif

	if( lSockUid != g_FastSynUid )
	{
		return;
	}

	icsk = inet_csk(pstSock);
	if( icsk->icsk_retransmits <= FAST_SYN_COUNT )
	{
		icsk->icsk_rto = TCP_TIMEOUT_INIT;
	}
}

int Emcom_Xengine_StartFastSyn(uint8_t *pdata, uint16_t len)
{
	uid_t              uid;

	/*input param check*/
	if( NULL == pdata )
	{
		EMCOM_LOGE(" Emcom_Xengine_StartFastSyn:data is null");
		return -EINVAL;
	}

	/*check len is invalid*/
	if(len != sizeof(uid_t))
	{
		EMCOM_LOGI(" Emcom_Xengine_StartFastSyn: len: %d is illegal", len);
		return -EINVAL;
	}

	uid =*(uid_t *)pdata;
	/*check uid*/
	if (invalid_uid(uid))
	{
		EMCOM_LOGE(" Emcom_Xengine_StartFastSyn: uid is invalid %d", uid);
		return -EINVAL;
	}
	EMCOM_LOGI(" Emcom_Xengine_StartFastSyn: uid: %d ", uid);
	g_FastSynUid = uid;
	return 0;
}

int Emcom_Xengine_StopFastSyn(uint8_t *pdata, uint16_t len)
{
	g_FastSynUid = UID_INVALID_APP;
	return 0;
}

#ifdef CONFIG_SMART_MP
static bool Emcom_Xengine_SmartMpUidInlist(uint32_t uid)
{
	struct Emcom_Xengine_SmartMpInfo *node;

	rcu_read_lock();
	list_for_each_entry_rcu(node, &smartmp_list, list) {
		if (node->uid == uid) {
			rcu_read_unlock();
			return true;
		}
	}
	rcu_read_unlock();

	return false;
}

static uint32_t Emcom_Xengine_SmartMpGetUidByMark(uint32_t mark)
{
	struct Emcom_Xengine_SmartMpInfo *node;
	uint32_t uid;

	if (!mark)
		return 0;

	rcu_read_lock();
	list_for_each_entry_rcu(node, &smartmp_list, list) {
		if (node->mark == mark) {
			uid = node->uid;
			rcu_read_unlock();
			return uid;
		}
	}
	rcu_read_unlock();

	return 0;
}

static void Emcom_Xengine_SmartMpAdd(uint32_t uid, uint32_t mark)
{
	struct Emcom_Xengine_SmartMpInfo *node;

	if (!uid || !mark) {
		EMCOM_LOGI("Para wrong: uid=%u mark=%u\n", uid, mark);
		return;
	}

	if (Emcom_Xengine_SmartMpUidInlist(uid)) {
		EMCOM_LOGI("uid=%u is already in the list\n", uid);
		return;
	}

	node = kmalloc(sizeof(*node), GFP_ATOMIC);
	if (!node) {
		EMCOM_LOGE("kmalloc fail\n");
		return;
	}

	node->uid = uid;
	node->mark = mark;
	EMCOM_LOGD("add uid=%u mark=%u\n", uid, mark);
	list_add_rcu(&node->list, &smartmp_list);
}

static void Emcom_Xengine_SmartMpDel(uint32_t uid)
{
	struct Emcom_Xengine_SmartMpInfo *node;

	rcu_read_lock();
	list_for_each_entry_rcu(node, &smartmp_list, list) {
		if (node->uid == uid ) {
			rcu_read_unlock();
			list_del_rcu(&node->list);
			EMCOM_LOGD("del uid=%u\n", uid);
			kfree(node);
			return;
		}
	}
	rcu_read_unlock();
}

static void Emcom_Xengine_SmartMpClear(void)
{
	struct Emcom_Xengine_SmartMpInfo *node;

	while (node = list_first_or_null_rcu(&smartmp_list, struct Emcom_Xengine_SmartMpInfo, list)) {
		list_del_rcu(&node->list);
		EMCOM_LOGD("del uid=%u\n", node->uid);
		kfree(node);
	}
}

static void Emcom_Xengine_StartSmartMp(uint8_t *pdata, uint16_t len)
{
	uint32_t *p;
	uint32_t uid, mark;

	/*input param check*/
	if( NULL == pdata )
	{
		EMCOM_LOGE("Emcom_Xengine_StartSmartMp: pdata is null");
		return;
	}

	if (len != 2*sizeof(uint32_t)) {
		EMCOM_LOGI("Emcom_Xengine_StartSmartMp: len %d is illegal", len);
		return;
	}

	p = (uint32_t *)pdata;
	uid = *p;
	p++;
	mark = *p;
	Emcom_Xengine_SmartMpAdd(uid, mark);
}

static void Emcom_Xengine_StopSmartMp(uint8_t *pdata, uint16_t len)
{
	/*input param check*/
	if( NULL == pdata )
	{
		EMCOM_LOGE("Emcom_Xengine_StopSmartMp: pdata is null");
		return;
	}

	if (len != sizeof(uint32_t)) {
		EMCOM_LOGI("Emcom_Xengine_StopSmartMp: len %d is illegal", len);
		return;
	}

	Emcom_Xengine_SmartMpDel(*(uint32_t *)pdata);
}

bool Emcom_Xengine_CheckUidAccount(const struct sk_buff *skb, uint32_t *uid, const struct sock *alternate_sk, int proto)
{
	uint32_t new_uid;
	const struct sock *full_sk;
	const struct sock *smart_mp_sk;

	if (!skb || !uid)
		return false;

	if (proto != IPPROTO_UDP)
		return true;

	if (Emcom_Xengine_SmartMpUidInlist(*uid)) {
		return false;
	} else {
		full_sk = skb_to_full_sk(skb);
		smart_mp_sk = full_sk ? full_sk : alternate_sk;
		new_uid = Emcom_Xengine_SmartMpGetUidByMark(skb->mark);
		if (!new_uid && smart_mp_sk && sk_fullsock(smart_mp_sk))
			new_uid = Emcom_Xengine_SmartMpGetUidByMark(smart_mp_sk->sk_mark);

		if (new_uid)
			*uid = new_uid;
	}
	return true;
}
EXPORT_SYMBOL(Emcom_Xengine_CheckUidAccount);

bool Emcom_Xengine_CheckIfaceAccount(const struct sock *sk, int proto)
{
	if (!sk || proto != IPPROTO_UDP
	    || !Emcom_Xengine_SmartMpUidInlist(sk->sk_uid.val))
		return true;
	else
		return false;
}
EXPORT_SYMBOL(Emcom_Xengine_CheckIfaceAccount);

bool Emcom_Xengine_SmartMpEnable(void)
{
	if (list_first_or_null_rcu(&smartmp_list, struct Emcom_Xengine_SmartMpInfo, list))
		return true;
	else
		return false;
}
EXPORT_SYMBOL(Emcom_Xengine_SmartMpEnable);

void Emcom_Xengine_SmartMpOnDK_Connect(void)
{
	Emcom_Xengine_SmartMpClear();
}
EXPORT_SYMBOL(Emcom_Xengine_SmartMpOnDK_Connect);
#endif
#ifdef CONFIG_MPTCP
void Emcom_Xengine_MptcpSocketClosed(void *data, int len)
{
	emcom_send_msg2daemon(NETLINK_EMCOM_KD_MPTCP_SOCKET_CLOSED, data, len);
}
EXPORT_SYMBOL(Emcom_Xengine_MptcpSocketClosed);

void Emcom_Xengine_MptcpSocketSwitch(void *data, int len)
{
	emcom_send_msg2daemon(NETLINK_EMCOM_KD_MPTCP_SOCKET_SWITCH, data, len);
}
EXPORT_SYMBOL(Emcom_Xengine_MptcpSocketSwitch);

void Emcom_Xengine_MptcpProxyFallback(void *data, int len)
{
	emcom_send_msg2daemon(NETLINK_EMCOM_KD_MPTCP_PROXY_FALLBACK, data, len);
}
EXPORT_SYMBOL(Emcom_Xengine_MptcpProxyFallback);
#endif


void Emcom_Xengine_EvtProc(int32_t event, uint8_t *pdata, uint16_t len)
{
	switch(event)
	{
		case NETLINK_EMCOM_DK_START_ACC:
			EMCOM_LOGD("emcom netlink receive acc start\n");
			Emcom_Xengine_StartAccUid(pdata,len);
			break;
		case NETLINK_EMCOM_DK_STOP_ACC:
			EMCOM_LOGD("emcom netlink receive acc stop\n");
			Emcom_Xengine_StopAccUid(pdata,len);
			break;
		case NETLINK_EMCOM_DK_CLEAR:
			EMCOM_LOGD("emcom netlink receive clear info\n");
			Emcom_Xengine_clear();
			break;
		case NETLINK_EMCOM_DK_RRC_KEEP:
			EMCOM_LOGD("emcom netlink receive rrc keep\n");
			Emcom_Xengine_RrcKeep();
			break;
		case NETLINK_EMCOM_DK_KEY_PSINFO:
			EMCOM_LOGD("emcom netlink receive psinfo\n");
			Emcom_Send_KeyPsInfo(pdata,len);
			break;
		case NETLINK_EMCOM_DK_SPEED_CTRL:
			EMCOM_LOGD("emcom netlink receive speed control uid\n");
			Emcom_Xengine_SetSpeedCtrlInfo(pdata,len);
			break;
		case NETLINK_EMCOM_DK_START_UDP_RETRAN:
			EMCOM_LOGD("emcom netlink receive wifi udp start\n");
			Emcom_Xengine_StartUdpReTran(pdata,len);
			break;
		case NETLINK_EMCOM_DK_STOP_UDP_RETRAN:
			EMCOM_LOGD("emcom netlink receive wifi udp stop\n");
			Emcom_Xengine_StopUdpReTran(pdata,len);
			break;
		case NETLINK_EMCOM_DK_CONFIG_MPIP:
			EMCOM_LOGD("emcom netlink receive btm config start\n");
			Emcom_Xengine_Config_MPIP(pdata,len);
			break;
		case NETLINK_EMCOM_DK_CLEAR_MPIP:
			EMCOM_LOGD("emcom netlink receive clear mpip config\n");
			Emcom_Xengine_Clear_Mpip_Config(pdata,len);
			break;
		case NETLINK_EMCOM_DK_START_MPIP:
			EMCOM_LOGD("emcom netlink receive btm start\n");
			Emcom_Xengine_StartMPIP(pdata,len);
			break;
		case NETLINK_EMCOM_DK_STOP_MPIP:
			EMCOM_LOGD("emcom netlink receive btm stop\n");
			Emcom_Xengine_StopMPIP(pdata,len);
			break;
		case NETLINK_EMCOM_DK_START_FAST_SYN:
			EMCOM_LOGD("emcom netlink receive fast syn start\n");
			Emcom_Xengine_StartFastSyn(pdata, len);
			break;
		case NETLINK_EMCOM_DK_STOP_FAST_SYN:
			EMCOM_LOGD("emcom netlink receive fast syn stop\n");
			Emcom_Xengine_StopFastSyn(pdata, len);
			break;
#ifdef CONFIG_SMART_MP
		case NETLINK_EMCOM_DK_START_SMARTMP:
			EMCOM_LOGD("emcom netlink receive smartmp start\n");
			Emcom_Xengine_StartSmartMp(pdata,len);
			break;
		case NETLINK_EMCOM_DK_STOP_SMARTMP:
			EMCOM_LOGD("emcom netlink receive smartmp stop\n");
			Emcom_Xengine_StopSmartMp(pdata,len);
			break;
		case NETLINK_EMCOM_DK_KNL_INIT_SMARTMP:
			EMCOM_LOGD("emcom netlink receive smartmp init\n");
			emcom_send_msg2daemon(NETLINK_EMCOM_KD_SMART_MP_ENABLED, NULL, 0);
			break;
#endif
		default:
			EMCOM_LOGI("emcom Xengine unsupport packet, the type is %d.\n", event);
			break;
	}
}


int Emcom_Xengine_SetProxyUid(struct sock *sk, char __user *optval, int optlen)
{
    uid_t uid = 0;
    int ret = 0;

    ret = -EINVAL;
    if (optlen < 0)
        goto out;

    ret = -EFAULT;
    if (copy_from_user(&uid, optval, optlen))
        goto out;

    lock_sock(sk);
    sk->sk_uid.val = uid;
    release_sock(sk);
    EMCOM_LOGD("hicom set proxy uid, uid: %u", sk->sk_uid.val);

    ret = 0;

out:

    return ret;
}

int Emcom_Xengine_GetProxyUid(struct sock *sk, char __user *optval, int __user *optlen, int len)
{
    uid_t uid = 0;
    int ret = 0;

    if (len < 0)
    {
        return -EINVAL;
    }

    uid = sk->sk_uid.val;

    ret = -EFAULT;
    if (copy_to_user(optval, &uid, len))
        goto out;

    ret = -EFAULT;
    if (put_user(len, optlen))
        goto out;

    EMCOM_LOGD(" hicom get proxy uid, uid: %u", uid);

    ret = 0;

out:

    return ret;
}


int Emcom_Xengine_SetSockFlag(struct sock *sk, char __user *optval, int optlen)
{
    int ret = 0;
    int hicom_flag = 0;

    ret = -EINVAL;
    if (optlen < 0)
        goto out;

    ret = -EFAULT;
    if (copy_from_user(&hicom_flag, optval, optlen))
        goto out;

    lock_sock(sk);
    sk->hicom_flag = hicom_flag;
    release_sock(sk);

    EMCOM_LOGD(" hicom set proxy flag, uid: %u, flag: %x", sk->sk_uid.val, sk->hicom_flag);

    ret = 0;

out:

    return ret;
}

void Emcom_Xengine_NotifySockError(struct sock *sk)
{
    if (sk->hicom_flag == HICOM_SOCK_FLAG_FINTORST) {
        EMCOM_LOGD(" hicom change fin to rst, uid: %u, flag: %x", sk->sk_uid.val, sk->hicom_flag);
        sk->sk_err = ECONNRESET;
        sk->sk_error_report(sk);
    }

    return;
}

