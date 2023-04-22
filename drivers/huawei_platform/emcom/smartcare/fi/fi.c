

#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <net/udp.h>
#include <net/sock.h>
#include <linux/timer.h>
#include <linux/version.h>
#include "../../emcom_netlink.h"
#include "../../emcom_utils.h"
#include <huawei_platform/emcom/smartcare/fi/fi_utils.h>

fi_ctx g_fi_ctx;


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,1)
	static const struct nf_hook_ops fi_nfhooks[] = {
#else
	static struct nf_hook_ops fi_nfhooks[] = {
#endif
	{
		.hook        = fi_hook_out,                 /* 回调函数 */
		.pf          = PF_INET,                     /* nf协议链 */
		.hooknum     = NF_INET_LOCAL_OUT,           /* 表示本机发出的报文 */
		.priority    = NF_IP_PRI_FILTER - 1,        /* 优先级 */
	},
	{
		.hook        = fi_hook_in,                  /* 回调函数 */
		.pf          = PF_INET,                     /* nf协议链 */
		.hooknum     = NF_INET_LOCAL_IN,            /* 表示目标地址是本机的报文 */
		.priority    = NF_IP_PRI_FILTER - 1,        /* 优先级 */
	},
};


void fi_mem_used(uint32_t memlen)
{
	g_fi_ctx.memused += memlen;
}


void fi_mem_de_used(uint32_t memlen)
{
	g_fi_ctx.memused -= memlen;
}


void fi_free(void *ptr, uint32_t size)
{
	kfree(ptr);
	fi_mem_de_used(size);
	return;
}


void *fi_malloc(uint32_t size)
{
	void *ptr;

	ptr = kmalloc(size, GFP_ATOMIC);
	if (!ptr)
	{
		return NULL;
	}

	fi_mem_used(size);
	memset(ptr, 0, size);

	return ptr;
}


void fi_register_nf_hook(void)
{
	int ret = 0;

	/* 加锁 */
	mutex_lock(&(g_fi_ctx.nf_mutex));

	/* 如果已经存在钩子函数，就不再注册 */
	if (g_fi_ctx.nf_exist == FI_TRUE)
	{
		mutex_unlock(&(g_fi_ctx.nf_mutex));
		return;
	}

	/* 注册netfilter钩子函数 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,1)
	ret = nf_register_net_hooks(&init_net, fi_nfhooks, ARRAY_SIZE(fi_nfhooks));
#else
	ret = nf_register_hooks(fi_nfhooks, ARRAY_SIZE(fi_nfhooks));
#endif
	if (ret == 0)
	{
		/* 如果注册成功，需要设置标记 */
		g_fi_ctx.nf_exist = FI_TRUE;

		/* 有钩子就有定时器 */
		g_fi_ctx.tm.expires = jiffies + HZ / FI_TIMER_INTERVAL;
		add_timer(&g_fi_ctx.tm);
	}

	/* 尽快释放锁 */
	mutex_unlock(&(g_fi_ctx.nf_mutex));

	/* 释放锁之后再记录日志 */
	if (ret)
	{
		FI_LOGE(" : FI register nf hooks failed, ret=%d", ret);
	}
	else
	{
		FI_LOGD(" : FI register nf hooks successfully. FI version %s %s.", __DATE__, __TIME__);
	}

	return;
}


void fi_unregister_nf_hook(void)
{
	/* 加锁 */
	mutex_lock(&(g_fi_ctx.nf_mutex));

	/* 如果不存在钩子函数，直接返回*/
	if (g_fi_ctx.nf_exist == FI_FALSE)
	{
		mutex_unlock(&(g_fi_ctx.nf_mutex));
		return;
	}

	/* 删除netfilter钩子函数 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,1)
	nf_unregister_net_hooks(&init_net, fi_nfhooks, ARRAY_SIZE(fi_nfhooks));
#else
	nf_unregister_hooks(fi_nfhooks, ARRAY_SIZE(fi_nfhooks));
#endif

	/* 删除定时器, 定时器与钩子函数同时存在同时删除 */
	del_timer(&g_fi_ctx.tm);

	g_fi_ctx.nf_exist = FI_FALSE;

	mutex_unlock(&(g_fi_ctx.nf_mutex));

	FI_LOGD(" : FI unregister nf hooks successfully.");

	return;
}


fi_app_info *fi_find_appinfo(uint32_t uid)
{
	int i;

	if (uid == 0)
	{
		return NULL;
	}

	/* 查找uid */
	for (i = 0; i < FI_APPID_MAX; i++)
	{
		if (g_fi_ctx.appinfo[i].uid == uid)
		{
			return &(g_fi_ctx.appinfo[i]);
		}
	}

	return NULL;
}


static uint32_t fi_find_appid(uint32_t uid)
{
	int i;

	if (uid == 0)
	{
		return FI_APPID_NULL;
	}

	/* 只在appidmin和appidmax的范围内查找，避免全表遍历，提升性能 */
	for (i = g_fi_ctx.appidmin; i <= g_fi_ctx.appidmax; i++)
	{
		if ((g_fi_ctx.appinfo[i].uid == uid) && (g_fi_ctx.appinfo[i].valid))
		{
			return g_fi_ctx.appinfo[i].appid;
		}
	}

	return FI_APPID_NULL;
}


static inline int fi_pkt_check(struct sk_buff *skb, int dir)
{
	struct iphdr *iph = ip_hdr(skb);

	/* 只关注ip报文 */
	if (!iph || skb->len <= FI_MIN_IP_PKT_LEN)
	{
		return FI_FAILURE;
	}

	/* 只关注ipv4的报文 */
	if (iph->version != FI_IP_VER_4)
	{
		return FI_FAILURE;
	}

	/* 不关注环回接口的报文 */
	if ((*(uint8_t *)&(iph->saddr) == FI_LOOP_ADDR) ||
	    (*(uint8_t *)&(iph->daddr) == FI_LOOP_ADDR))
	{
		return FI_FAILURE;
	}

	if (iph->protocol == FI_IPPROTO_UDP)
	{
		if (skb->len < sizeof(struct udphdr) + sizeof(struct iphdr))
		{
			return FI_FAILURE;
		}
	}
	else if (iph->protocol == FI_IPPROTO_TCP)
	{
		if (skb->len < sizeof(struct tcphdr) + sizeof(struct iphdr))
		{
			return FI_FAILURE;
		}
	}
	else
	{
		return FI_FAILURE;
	}

	return FI_SUCCESS;
}


static inline int fi_parse_mptcp(fi_pkt *pktinfo, struct tcphdr *tcph, uint32_t hdrlen)
{
	uint8_t   optkind = 0;      /* tcp选型的类型 */
	uint8_t   optlen  = 0;      /* tcp选项的长度 */
	uint32_t  leftlen = hdrlen - sizeof(struct tcphdr);
	uint8_t  *leftdata = (uint8_t *)tcph + sizeof(struct tcphdr);
	uint32_t *seqack;
	fi_mptcp_dss *mptcpdss;

	while (leftlen > FI_TCP_OPT_HDR_LEN)
	{
		optkind = *leftdata;
		optlen  = *(leftdata + 1);

		leftdata += FI_TCP_OPT_HDR_LEN;
		leftlen  -= FI_TCP_OPT_HDR_LEN;

		/* 找到mptcp选项 */
		if (optkind == FI_TCP_OPT_MPTCP)
		{
			break;
		}
	}

	/* 不是mptcp报文 */
	if (optkind != FI_TCP_OPT_MPTCP)
	{
		return FI_SUCCESS;
	}

	/* 开始解析mptcp选项, 首先检查数据长度 */
	if ((leftlen + 2 < optlen) || (optlen < FI_MPTCP_DSS_MINLEN))
	{
		/* 忽略选项错误 */
		return FI_SUCCESS;
	}

	/* 必须是dss选型 */
	mptcpdss = (fi_mptcp_dss *)leftdata;
	if (mptcpdss->subtype != FI_MPTCP_SUBTYPE_DSS)
	{
		return FI_SUCCESS;
	}

	/* 扣除dss首部长度 */
	leftdata += sizeof(fi_mptcp_dss);
	leftlen  -= sizeof(fi_mptcp_dss);

	/* 再次计算并检查dss应该有的长度 */
	optlen = sizeof(uint32_t) * (mptcpdss->seq8 + mptcpdss->seqpre +
	                             mptcpdss->ack8 + mptcpdss->ackpre);
	if (leftlen < optlen)
	{
		return FI_SUCCESS;
	}

	/* 到这里，说明一定可以提取到seq或ack，但有的报文不含seq，所以先置零 */
	pktinfo->seq = 0;
	pktinfo->ack = 0;
	pktinfo->mptcp = FI_TRUE;

	/* 提取ack */
	if (mptcpdss->ackpre && (leftlen >= sizeof(uint32_t)))
	{
		seqack = (uint32_t *)leftdata;
		pktinfo->ack = ntohl(*seqack);

		leftdata += sizeof(uint32_t);
		leftlen  -= sizeof(uint32_t);

		/* 如果是8字节的ack，只取低4字节就行了 */
		if (mptcpdss->ack8 && (leftlen >= sizeof(uint32_t)))
		{
			seqack = (uint32_t *)leftdata;
			pktinfo->ack = ntohl(*seqack);

			leftdata += sizeof(uint32_t);
			leftlen  -= sizeof(uint32_t);
		}
	}

	/* 提取seq */
	if (mptcpdss->seqpre && (leftlen >= sizeof(uint32_t)))
	{
		seqack = (uint32_t *)leftdata;
		pktinfo->seq = ntohl(*seqack);

		leftdata += sizeof(uint32_t);
		leftlen  -= sizeof(uint32_t);

		/* 如果是8字节的seq，只取低4字节就行了 */
		if (mptcpdss->seq8 && (leftlen >= sizeof(uint32_t)))
		{
			seqack = (uint32_t *)leftdata;
			pktinfo->seq = ntohl(*seqack);
		}
	}

	FI_LOGD(" : FI mptcp, seq=%u, ack=%u, flow: %u,%u.",
	        pktinfo->seq, pktinfo->ack, pktinfo->sport, pktinfo->dport);

	return FI_SUCCESS;
}


static inline int fi_pkt_parse(fi_pkt *pktinfo, struct sk_buff *skb, int dir)
{
	struct iphdr  *iph;
	struct udphdr *udph;
	struct tcphdr *tcph;
	unsigned int   iphlen = 0;      /* ip header len */
	unsigned int   l4hlen = 0;      /* tcp/udp header len */

	/* 已在上文做过基本检查，报文一定为IP报文，长度一定大于20 */
	iph    = ip_hdr(skb);
	iphlen = iph->ihl * FI_IP_HDR_LEN_BASE;

	/* UDP报文解析 */
	if (iph->protocol == FI_IPPROTO_UDP)
	{
		udph   = udp_hdr(skb);
		l4hlen = sizeof(struct udphdr);

		/* 检查skb中报文的长度 */
		if (skb->len < iphlen + l4hlen + skb->data_len)
		{
			return FI_FAILURE;
		}

		pktinfo->data = (uint8_t *)udph + l4hlen;
		pktinfo->len = skb->len - iphlen - l4hlen;
		pktinfo->bufdatalen = skb->len - skb->data_len - iphlen - l4hlen;
		pktinfo->sport = ntohs(udph->source);
		pktinfo->dport = ntohs(udph->dest);
	}
	/* 不是UDP则一定是TCP */
	else
	{
		tcph   = tcp_hdr(skb);
		l4hlen = tcph->doff * FI_TCP_HDR_LEN_BASE;

		/* 检查skb中报文的长度 */
		if (skb->len < iphlen + l4hlen + skb->data_len)
		{
			return FI_FAILURE;
		}

		pktinfo->data = (uint8_t *)tcph + l4hlen;
		pktinfo->len  = skb->len - iphlen - l4hlen;
		pktinfo->bufdatalen = skb->len - skb->data_len - iphlen - l4hlen;
		pktinfo->sport = ntohs(tcph->source);
		pktinfo->dport = ntohs(tcph->dest);
		pktinfo->seq = ntohl(tcph->seq);
		pktinfo->ack = ntohl(tcph->ack_seq);

		if (fi_parse_mptcp(pktinfo, tcph, l4hlen) != FI_SUCCESS)
		{
			return FI_FAILURE;
		}
	}

	/* 只关注端口大于1023的 */
	if ((pktinfo->sport < FI_BATTLE_START_PORT_MIN) ||
	    (pktinfo->dport < FI_BATTLE_START_PORT_MIN))
	{
		return FI_FAILURE;
	}

	pktinfo->proto = iph->protocol;
	pktinfo->dir   = dir;
	pktinfo->sip   = ntohl(iph->saddr);
	pktinfo->dip   = ntohl(iph->daddr);

	pktinfo->msec = jiffies_to_msecs(jiffies);

	return FI_SUCCESS;
}


void fi_timer_callback(unsigned long arg)
{
	/* 老化流表 */
	fi_flow_age();

	/* 定期发送rtt, 更新游戏状态 */
	fi_rtt_timer();

	/* 定时器下次触发的时间 */
	mod_timer(&g_fi_ctx.tm, jiffies + HZ / FI_TIMER_INTERVAL);

	return;
}


static bool fi_streq(char *data, uint32_t datalen, char *str)
{
	uint32_t strLen = strlen(str);

	/* data字符串后有多余的'\0' */
	if (datalen >= strLen + 1)
	{
		if (!memcmp(data, str, strLen + 1))
		{
			return FI_TRUE;
		}
	}
	/* data不是以'\0'结尾 */
	else if (datalen == strLen)
	{
		if (!memcmp(data, str, strLen))
		{
			return FI_TRUE;
		}
	}
	else
	{
		return FI_FALSE;
	}

	return FI_FALSE;
}


static void fi_appid_add(uint32_t appid)
{
	if ((g_fi_ctx.appidmin == 0) || (appid < g_fi_ctx.appidmin))
	{
		g_fi_ctx.appidmin = appid;
	}

	if ((g_fi_ctx.appidmax == 0) || (appid > g_fi_ctx.appidmax))
	{
		g_fi_ctx.appidmax = appid;
	}
}


static void fi_appid_shrink(void)
{
	uint32_t i;

	/* appidmin向前移 */
	for (i = g_fi_ctx.appidmin; (i <= g_fi_ctx.appidmax) && (i < FI_APPID_MAX); i++)
	{
		/* valid为假表示该游戏已退出 */
		if (!(g_fi_ctx.appinfo[i].valid))
		{
			g_fi_ctx.appidmin = i;
		}
		else
		{
			break;
		}
	}

	/* appidmax向后移 */
	for (i = g_fi_ctx.appidmax; (i >= g_fi_ctx.appidmin) && (i > 0); i--)
	{
		/* valid为假表示该游戏已退出 */
		if (!(g_fi_ctx.appinfo[i].valid))
		{
			g_fi_ctx.appidmax = i;
		}
		else
		{
			break;
		}
	}

	/* 说明已经没有appid了 */
	i = g_fi_ctx.appidmin;
	if ((g_fi_ctx.appidmin == g_fi_ctx.appidmax) && !(g_fi_ctx.appinfo[i].valid))
	{
		g_fi_ctx.appidmin = 0;
		g_fi_ctx.appidmax = 0;
	}

	return;
}


static uint32_t fi_appname_to_appid(char *nameptr, uint32_t datalen)
{
	uint32_t appid = FI_APPID_NULL;

	if (fi_streq(nameptr, datalen, FI_APP_NAME_WZRY))
	{
		appid = FI_APPID_WZRY;
	}
	else if (fi_streq(nameptr, datalen, FI_APP_NAME_CJZC))
	{
		appid = FI_APPID_CJZC;
	}
	else if (fi_streq(nameptr, datalen, FI_APP_NAME_QJCJ))
	{
		appid = FI_APPID_QJCJ;
	}
	else if (fi_streq(nameptr, datalen, FI_APP_NAME_HYXD))
	{
		appid = FI_APPID_HYXD;
	}
	else if (fi_streq(nameptr, datalen, FI_APP_NAME_HYXD_HW))
	{
		appid = FI_APPID_HYXD;
	}
	else if (fi_streq(nameptr, datalen, FI_APP_NAME_CYHX))
	{
		appid = FI_APPID_CYHX;
	}
	else if (fi_streq(nameptr, datalen, FI_APP_NAME_QQFC))
	{
		appid = FI_APPID_QQFC;
	}
	else if (fi_streq(nameptr, datalen, FI_APP_NAME_BH3) ||
	         fi_streq(nameptr, datalen, FI_APP_NAME_BH3_2) ||
	         fi_streq(nameptr, datalen, FI_APP_NAME_BH3_3) ||
	         fi_streq(nameptr, datalen, FI_APP_NAME_BH3_4) ||
	         fi_streq(nameptr, datalen, FI_APP_NAME_BH3_5))
	{
		appid = FI_APPID_BH3;
	}
	else if (fi_streq(nameptr, datalen, FI_APP_NAME_UU))
	{
		appid = FI_APPID_UU;
	}
	else if (fi_streq(nameptr, datalen, FI_APP_NAME_XUNYOU))
	{
		appid = FI_APPID_XUNYOU;
	}
	else
	{
		appid = FI_APPID_NULL;
	}

	return appid;
}


static void fi_proc_applaunch(void *data, int32_t len)
{
	fi_msg_applaunch *msg;
	fi_app_info *appinfo;
	uint32_t appid;

	/* 参数检查 */
	if ((!data) || (len < sizeof(fi_msg_applaunch)))
	{
		FI_LOGE(" : FI illegal data length %d in FI app launch", len);
		return;
	}

	msg  = (fi_msg_applaunch *)data;
	len -= sizeof(fi_msg_applaunch);

	/* 将字符串形式的appname转化为appid */
	appid = fi_appname_to_appid(msg->appname, len);
	if (!FI_APPID_VALID(appid))
	{
		FI_LOGE(" : FI unknown app name %s, len %u.", msg->appname, len);
		return;
	}

	fi_appid_add(appid);

	/* 创建nf钩子, 如果已经存在就不再增加 */
	fi_register_nf_hook();

	/* 开始赋值 */
	appinfo = &g_fi_ctx.appinfo[appid];
	appinfo->appid = appid;
	appinfo->uid   = msg->uid;
	appinfo->valid = FI_TRUE;
	appinfo->switchs = msg->switchs;

	FI_LOGI(" : FI set app info appid=%u, uid=%u, switch=%08x",
			appinfo->appid, appinfo->uid, appinfo->switchs);

	return;
}


static void fi_proc_appstatus(void *data, int32_t len)
{
	fi_msg_appstatus *msg;
	fi_app_info *appinfo;
	uint32_t appid;

	/* 参数检查 */
	if ((!data) || (len < sizeof(fi_msg_appstatus)))
	{
		FI_LOGE(" : FI illegal data length %d in FI set app status", len);
		return;
	}

	msg = (fi_msg_appstatus *)data;
	len -= sizeof(fi_msg_appstatus);

	appid = fi_appname_to_appid(msg->appname, len);
	if (!FI_APPID_VALID(appid))
	{
		FI_LOGE(" : FI unknown app name %s, len %u.", msg->appname, len);
		return;
	}

	FI_LOGI(" : FI recv app status appid=%u, appstatus=%u", appid, msg->appstatus);

	/* 需要更新一下uid，记录app当前的状态 */
	appinfo = &g_fi_ctx.appinfo[appid];
	appinfo->uid = msg->uid;
	appinfo->appstatus = msg->appstatus;

	/* 如果是游戏退出 */
	if (msg->appstatus == GAME_SDK_STATE_DIE)
	{
		fi_gamectx *gamectx;

		/* 不要清除uid, 其它内核线程可能还会用到, 将标记置为false即可 */
		appinfo->valid = FI_FALSE;

		gamectx = g_fi_ctx.gamectx + appid;

		/* 发送对战结束 */
		if (FI_BATTLING(gamectx->appstatus))
		{
			fi_rtt_status(gamectx->appid, FI_STATUS_BATTLE_STOP);
		}

		/* 清除rtt缓存 */
		memset(gamectx, 0, sizeof(fi_gamectx));

		/* 收缩appid范围 */
		fi_appid_shrink();

		/* 如果没有appid, 则删除nf钩子 */
		if (FI_HAS_NO_APPID(g_fi_ctx))
		{
			fi_unregister_nf_hook();
		}

		FI_LOGI(" : FI clear cache because of app exit, uid=%u", msg->uid);
	}

	return;
}


void fi_reflect_status(int32_t event, uint8_t *data, uint16_t len)
{
	fi_msg_appstatus *msg;
	fi_report_status report = {0};
	uint32_t status = 0;

	if ((!data) || (len < sizeof(fi_msg_appstatus)))
	{
		return;
	}

	msg = (fi_msg_appstatus *)data;

	/* 如果是游戏加载，消息类型固定为GAME_SDK_STATE_FOREGROUND */
	if (event == NETLINK_EMCOM_DK_SMARTCARE_FI_APP_LAUNCH)
	{
		status = GAME_SDK_STATE_FOREGROUND;
	}
	else if (event == NETLINK_EMCOM_DK_SMARTCARE_FI_APP_STATUS)
	{
		status = msg->appstatus;
	}
	else
	{
		return;
	}

	/* 上报游戏状态 */
	report.uid = msg->uid;
	report.status = status;
	report.apptype = FI_APP_TYPE_GAME;
	emcom_send_msg2daemon(NETLINK_EMCOM_KD_SMARTCARE_FI_REPORT_APP_STATUS, &report, sizeof(report));

	FI_LOGD(" : FI reflect status to daemon, uid=%u, status=%u.", report.uid, status);

	return;
}


void fi_event_process(int32_t event, uint8_t *pdata, uint16_t len)
{
	FI_LOGD(" : FI received cmd %d, datalen %u.", event, len);

	/* 根据不同的消息id进行不同的处理 */
	switch (event)
	{
		/* 游戏启动 */
		case NETLINK_EMCOM_DK_SMARTCARE_FI_APP_LAUNCH:
			fi_proc_applaunch(pdata, len);
			break;

		/* 游戏状态变化 */
		case NETLINK_EMCOM_DK_SMARTCARE_FI_APP_STATUS:
			fi_proc_appstatus(pdata, len);
			break;

		default:
			FI_LOGE(" : FI received unsupported message");
			break;
	}

	/* 将游戏状态变化消息同时发送到framework */
	fi_reflect_status(event, pdata, len);

	return;
}


static void fi_hook(struct sk_buff *skb, int dir)
{
	fi_flow_head *head;
	fi_flow_node *flow;
	fi_pkt pktinfo = {0};
	struct sock *sk;
	struct iphdr *iph = ip_hdr(skb);
	kuid_t   kuid = {0};
	uint32_t hash;
	uint32_t appid = 0;
	uint32_t addflow = FI_FALSE;

	if (unlikely(!skb))
	{
		return;
	}

	/* 过滤出ipv4 udp&tcp */
	if (fi_pkt_check(skb, dir) != FI_SUCCESS)
	{
		return;
	}

	/* 上行报文通过uid过滤，下行报文获取不到uid, 通过流表过滤 */
	if (dir == FI_DIR_UP)
	{
		sk = skb->sk;
		if (sk == NULL)
		{
			return;
		}

		/* tcp上行报文还需要检查当前连接状态 */
		if ((iph->protocol == FI_IPPROTO_TCP) &&
		    (sk->sk_state != TCP_ESTABLISHED))
		{
			return;
		}

		kuid = sock_i_uid(sk);

		/* 只处理游戏报文, 通过uid过滤 */
		appid = fi_find_appid(kuid.val);
		if (!FI_APPID_VALID(appid))
		{
			return;
		}

		addflow = FI_TRUE;
	}

	/* 报文解析, 只关注端口大于1023的 */
	if (fi_pkt_parse(&pktinfo, skb, dir) != FI_SUCCESS)
	{
		return;
	}

	hash = fi_flow_hash(pktinfo.sip, pktinfo.dip, pktinfo.sport, pktinfo.dport);
	head = fi_flow_header(hash);

	fi_flow_lock();

	/* 上行报文查找或创建流, 下行报文仅仅查找流 */
	flow = fi_flow_get(&pktinfo, head, addflow);
	if (flow == NULL)
	{
		fi_flow_unlock();
		return;
	}

	if (dir == FI_DIR_UP)
	{
		flow->flowctx.appid = appid;
	}
	else
	{
		appid = flow->flowctx.appid;
	}

	/* fi主入口 */
	fi_rtt_entrance(&pktinfo, &(flow->flowctx), appid);
	fi_flow_unlock();

	return;
}


unsigned int fi_hook_out(void *priv, struct sk_buff *skb,
            const struct nf_hook_state *state)
{
	fi_hook(skb, FI_DIR_UP);

	return NF_ACCEPT;
}


unsigned int fi_hook_in(void *priv, struct sk_buff *skb,
            const struct nf_hook_state *state)
{
	fi_hook(skb, FI_DIR_DOWN);

	return NF_ACCEPT;
}


void fi_para_init(void)
{
	fi_flow_init();
	memset(&g_fi_ctx, 0, sizeof(g_fi_ctx));

	mutex_init(&g_fi_ctx.appinfo_lock);
	mutex_init(&g_fi_ctx.nf_mutex);

	init_timer(&g_fi_ctx.tm);
	g_fi_ctx.tm.function = fi_timer_callback;
	g_fi_ctx.tm.data     = (unsigned long)"fi_timer";
}


void fi_init(void)
{
	/* 初始化模块运行参数 */
	fi_para_init();

	FI_LOGI(" : FI init kernel module successfully, mem used %lu",
	        sizeof(fi_ctx) + sizeof(fi_flow));

	return;
}


void fi_deinit(void)
{
	/* 删除nf注册的钩子函数, 删除定时器 */
	fi_unregister_nf_hook();

	/* 清空流表 */
	fi_flow_clear();

	FI_LOGI(" : FI deinit kernel module successfully");

	return;
}

EXPORT_SYMBOL(fi_event_process);
EXPORT_SYMBOL(fi_init);
EXPORT_SYMBOL(fi_deinit);

