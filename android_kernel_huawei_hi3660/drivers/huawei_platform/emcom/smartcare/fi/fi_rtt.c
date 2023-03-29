#include <net/sock.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <linux/list.h>
#include "../../emcom_netlink.h"
#include "../../emcom_utils.h"
#include <huawei_platform/emcom/smartcare/fi/fi_utils.h>

static char hyxd_data[] = {0x00, 0x00, 0x00, 0x00, 0x51, 0x00, 0x40, 0x00};


uint32_t fi_get_srtt(uint32_t prev_srtt, uint32_t cur_rtt, int alpha)
{
	if (prev_srtt == 0)
	{
		prev_srtt = cur_rtt;
	}

	return (uint32_t)(((FI_PERCENT - alpha) * prev_srtt +
	                   (alpha * cur_rtt)) / FI_PERCENT);
}


void fi_rtt_send(uint32_t appid)
{
	fi_report_rtt report = {0};
	fi_app_info *appinfo = g_fi_ctx.appinfo + appid;
	fi_gamectx *gamectx = g_fi_ctx.gamectx + appid;

	/* 还没算出rtt 或者 游戏已退出 */
	if (!(gamectx->final_rtt) || !(appinfo->valid))
	{
		return;
	}

	/* 上报rtt到native */
	if ((appinfo->switchs & FI_SWITCH_RTT) != 0)
	{
		report.uid = appinfo->uid;
		report.rtt = gamectx->final_rtt;
		report.apptype = FI_APP_TYPE_GAME;

		emcom_send_msg2daemon(NETLINK_EMCOM_KD_SMARTCARE_FI_REPORT_RTT, &report, sizeof(report));

		FI_LOGD(" : FI send rtt to daemon, cmd=%d, rtt=%u, uid=%u."
		        " (btrtt=%d, btsrtt=%d, hbrtt=%d, hbsrtt=%d)",
				NETLINK_EMCOM_KD_SMARTCARE_FI_REPORT_RTT, report.rtt, report.uid,
				gamectx->rtt, gamectx->battlertt, gamectx->hbrtt, gamectx->hbsrtt);
	}

	return;
}


void fi_rtt_status(uint32_t appid, uint32_t status)
{
	fi_app_info *appinfo;
	fi_report_status report = {0};

	/* 上报游戏状态 */
	appinfo = g_fi_ctx.appinfo + appid;
	if ((appinfo->switchs & FI_SWITCH_STATUS) != 0)
	{
		report.uid = appinfo->uid;
		report.status = status;
		report.apptype = FI_APP_TYPE_GAME;

		emcom_send_msg2daemon(NETLINK_EMCOM_KD_SMARTCARE_FI_REPORT_APP_STATUS, &report, sizeof(report));

		FI_LOGD(" : FI send status to daemon, appid=%u, uid=%u, status=%u.", appid, report.uid, status);
	}

	return;
}


void fi_reset_gamectx(fi_gamectx *gamectx)
{
	fi_gamectx tmpctx;
	fi_mpctx *mpctx;

	mpctx = g_fi_ctx.mpctx + gamectx->appid;

	memcpy(&tmpctx, gamectx, sizeof(fi_gamectx));
	memset(gamectx, 0, sizeof(fi_gamectx));
	memset(mpctx, 0, sizeof(fi_mpctx));

	/* 保留部分值 */
	gamectx->appid = tmpctx.appid;
	gamectx->battle_flow_port = tmpctx.battle_flow_port;
	gamectx->cliip = tmpctx.cliip;
	gamectx->hb_flow_port = tmpctx.hb_flow_port;
}


void fi_rtt_battle_pktnum_cal(fi_gamectx *gamectx, fi_app_info *appinfo)
{
	/* 当游戏在后台期间，不统计连续没有报文的时间 */
	if (appinfo->appstatus != GAME_SDK_STATE_BACKGROUND)
	{
		/* 记录没有上行报文所持续的时间 */
		if (gamectx->uplinkpktnum == 0)
		{
			gamectx->nouplinkpkt++;
		}
		else
		{
			gamectx->nouplinkpkt = 0;
		}

		/* 记录没有下行报文所持续的时间 */
		if (gamectx->downlinkpktnum == 0)
		{
			gamectx->nodownlinkpkt++;
		}
		else
		{
			gamectx->nodownlinkpkt = 0;
		}
	}

	/* 通过pre***pktnum来判断游戏状态，因为这个是一整秒的报文总数 */
	gamectx->preuplinkpktnum = gamectx->uplinkpktnum;
	gamectx->predownlinkpktnum = gamectx->downlinkpktnum;
	gamectx->uplinkpktnum = 0;
	gamectx->downlinkpktnum = 0;
}


void fi_rtt_timer(void)
{
	int i;
	fi_gamectx  *gamectx;
	fi_app_info *appinfo;

	for (i = 0; i < FI_APPID_MAX; i++)
	{
		gamectx = g_fi_ctx.gamectx + i;
		if (!FI_APPID_VALID(gamectx->appid))
		{
			continue;
		}

		/* 得到app的配置, valid为假说明游戏未加载 */
		appinfo = g_fi_ctx.appinfo + gamectx->appid;
		if (appinfo->valid == FI_FALSE)
		{
			continue;
		}

		/* 计算对战报文在上一秒的数量 */
		fi_rtt_battle_pktnum_cal(gamectx, appinfo);

		FI_LOGD(" : FI timer appid=%d, uplinkpps=%u, nouplinkpkt=%u,"
		        " downlinkpps=%u, nodownlinkpkt=%u.",
		        i, gamectx->preuplinkpktnum, gamectx->nouplinkpkt,
		        gamectx->predownlinkpktnum, gamectx->nodownlinkpkt);

		/* 游戏还没算出rtt或者不在对战中 */
		if (!(gamectx->updatetime) || !FI_BATTLING(gamectx->appstatus))
		{
			continue;
		}

		/* 判定对战是否结束 */
		if (fi_rtt_judge_battle_stop(gamectx, appinfo) == FI_TRUE)
		{
			continue;
		}

		/* 上报rtt */
		fi_rtt_send(gamectx->appid);
	}

	return;
}


uint32_t fi_rtt_get_le_u32(uint8_t *data)
{
	uint32_t value = 0;

	value = data[3];
	value = (value << 8) + data[2];
	value = (value << 8) + data[1];
	value = (value << 8) + data[0];

	return value;
}


uint32_t fi_rtt_get_be_u32(uint8_t *data)
{
	uint32_t value = 0;

	value = data[0];
	value = (value << 8) + data[1];
	value = (value << 8) + data[2];
	value = (value << 8) + data[3];

	return value;
}


void fi_rtt_update_integral(fi_gamectx *gamectx, int32_t newrtt, int64_t curms)
{
	int64_t cursec = FI_MS2SEC(curms);
	int32_t avgrtt;

	if (curms <= gamectx->updatetime)
	{
		return;
	}

	if (cursec == FI_MS2SEC(gamectx->updatetime))
	{
		gamectx->rtt_integral += gamectx->rtt * (curms - gamectx->updatetime);
	}
	else
	{
		gamectx->rtt_integral += gamectx->rtt * (cursec * FI_MS - gamectx->updatetime);
		avgrtt = gamectx->rtt_integral / FI_MS / (cursec - FI_MS2SEC(gamectx->updatetime));

		/* 上行报文很少的场景: 两局对战之间, 角色阵亡, 游戏在后台 */
		/* 上行报文很少时下行报文通常也很少, RTT计算的会不准, 所以这时不更新RTT */
		if ((gamectx->preuplinkpktnum >= FI_BATTLE_ONGOING) || (avgrtt < gamectx->battlertt))
		{
			gamectx->srtt = fi_get_srtt(gamectx->srtt, FI_MIN(avgrtt, FI_MAX_ORG_RTT), FI_SRTT_VAR);
			gamectx->battlertt = FI_MIN(gamectx->srtt, FI_MAX_RTT);
		}

		gamectx->rtt_integral = gamectx->rtt * (curms - cursec * FI_MS);
	}

	gamectx->updatetime = curms;
	gamectx->rtt = FI_MIN(newrtt, FI_MAX_ORG_RTT);

	return;
}


void fi_rtt_reset_study(fi_pkt *pktinfo, fi_flowctx *flowctx)
{
	flowctx->studystarttime = pktinfo->msec;
	flowctx->uppktnum = 0;
	flowctx->downpktnum = 0;
	flowctx->uppktbytes = 0;
}


int fi_rtt_study_by_port(fi_pkt *pktinfo, fi_flowctx *flowctx, fi_gamectx *gamectx)
{
	uint16_t srvport = FI_SRVPORT(pktinfo);

	if (srvport == gamectx->battle_flow_port)
	{
		flowctx->flowtype = FI_FLOWTYPE_BATTLE;
		FI_LOGD(" : FI learned a battle flow %u,%u by port %u.",
		        pktinfo->sport, pktinfo->dport, gamectx->battle_flow_port);

		/* 发现新的对战流，判断是否为断线重连 */
		fi_rtt_judge_reconn(pktinfo, gamectx);
		gamectx->cliip = FI_CLIIP(pktinfo);

		return FI_SUCCESS;
	}
	else if (srvport == gamectx->hb_flow_port)
	{
		flowctx->flowtype = FI_FLOWTYPE_HB;
		FI_LOGD(" : FI learned a heartbeat flow %u,%u by port %u.",
		        pktinfo->sport, pktinfo->dport, gamectx->hb_flow_port);
		return FI_SUCCESS;
	}
	else
	{
		return FI_FAILURE;
	}

	return FI_FAILURE;
}


void fi_rtt_judge_reconn(fi_pkt *pktinfo, fi_gamectx *gamectx)
{
	fi_app_info *appinfo = g_fi_ctx.appinfo + gamectx->appid;
	int reconn = FI_FALSE;

	FI_LOGD(" : FI nouplinkpkt=%u, nodownlinkpkt=%u, appstatus=%u, cliip=%08x.",
		    gamectx->nouplinkpkt, gamectx->nodownlinkpkt,
		    gamectx->appstatus, gamectx->cliip);

	/* 不在对战中不报断线重连 */
	if (!FI_BATTLING(gamectx->appstatus))
	{
		return;
	}

	/* 防止过于频繁的报断线重连 */
	if (pktinfo->msec - gamectx->reconntime < FI_RECONN_LIMIT)
	{
		return;
	}

	do
	{
		/* 如果发生wifi-lte链路切换，则不管在前台还是后台都报断线重连 */
		if ((gamectx->cliip != FI_CLIIP(pktinfo)) || (gamectx->final_rtt > FI_RECONN_THRESH))
		{
			reconn = FI_TRUE;
			break;
		}

		/* 游戏不在后台且连续多秒有上行却没有下行报文，则认为是断线重连 */
		if ((appinfo->appstatus != GAME_SDK_STATE_BACKGROUND) &&
		    (gamectx->nouplinkpkt < FI_NO_DOWNLINK_TIME) &&
		    (gamectx->nodownlinkpkt >= FI_NO_DOWNLINK_TIME))
		{
			reconn = FI_TRUE;
			break;
		}
	} while (0);

	if (reconn)
	{
		fi_rtt_status(gamectx->appid, FI_STATUS_BATTLE_RECONN);
		gamectx->reconntime = pktinfo->msec;
	}

	return;
}


void fi_rtt_judge_qqfc(fi_pkt *pktinfo, fi_flowctx *flowctx, fi_gamectx *gamectx)
{
	int64_t  timediff;      /* msec */
	uint16_t up_pps;

	timediff = FI_MAX(pktinfo->msec - flowctx->studystarttime, 1);

	up_pps = flowctx->uppktnum * FI_MS / timediff;
	FI_LOGD(" : FI judge qqfc, up: %u/s, down: %u/s, flow: %u,%u, times: %u",
	        up_pps, flowctx->downpktnum, pktinfo->sport, pktinfo->dport, flowctx->btflow_times);

	/* 识别QQ飞车的对战流 */
	if ((up_pps >= FI_QQFC_UP_PPS_MIN) && (up_pps <= FI_QQFC_UP_PPS_MAX))
	{
		flowctx->btflow_times++;
	}
	else
	{
		flowctx->btflow_times = 0;
	}

	/* 设置对战流标记 */
	if (flowctx->btflow_times >= FI_BATTLE_FLOW_CON)
	{
		flowctx->flowtype = FI_FLOWTYPE_BATTLE;
		gamectx->battle_flow_port = FI_SRVPORT(pktinfo);
		gamectx->cliip = FI_CLIIP(pktinfo);
		FI_LOGD(" : FI learned qqfc battle flow %u,%u, battle port %u.",
			    pktinfo->sport, pktinfo->dport, gamectx->battle_flow_port);
	}

	return;
}


int fi_rtt_judge_hb(fi_pkt *pktinfo, fi_flowctx *flowctx, fi_gamectx *gamectx)
{
	uint32_t timediff;      /* msec->sec */
	int ret = FI_FAILURE;

	/* 第一个报文, 记录开始学习的时间后返回 */
	if (flowctx->hbstudytime == 0)
	{
		flowctx->hbstudytime = (uint32_t)(pktinfo->msec);
		return FI_FAILURE;
	}

	/* 收集20个报文之后开始判断是否为心跳流 */
	if (flowctx->hbdownnum < FI_HB_STUDY_NUM)
	{
		return FI_FAILURE;
	}

	timediff = ((uint32_t)(pktinfo->msec) - flowctx->hbstudytime) / FI_MS;

	FI_LOGD(" : FI judge hb, hbupnum: %u, hbdownnum: %u, time: %u, flow: %u,%u",
	        flowctx->hbupnum, flowctx->hbdownnum, timediff, pktinfo->sport, pktinfo->dport);

	/* 三个条件判断心跳流: 上下行报文个数差值, pps, 报文间隔 */
	if ((FI_ABS_SUB(flowctx->hbupnum, flowctx->hbdownnum) < FI_HB_PKT_SUB) &&
	    (flowctx->hbupnum <= (FI_HB_MAX_PPS * timediff)) &&
	    (timediff <= (FI_HB_MAX_TIME_GAP * flowctx->hbupnum)))
	{
		/* 标记流类型为心跳流 */
		flowctx->flowtype = FI_FLOWTYPE_HB;
		gamectx->hb_flow_port = FI_SRVPORT(pktinfo);
		FI_LOGD(" : FI learned a heartbeat flow %u,%u, heartbeat port %u.",
		        pktinfo->sport, pktinfo->dport, gamectx->hb_flow_port);
		ret = FI_SUCCESS;
	}
	/* 该函数如果用于心跳流校验，这里重置流类型 */
	else
	{
		flowctx->flowtype = FI_FLOWTYPE_INIT;
		gamectx->hb_flow_port = 0;
		FI_LOGD(" : FI heartbeat flow verification failed, flow: %u,%u.",
		        pktinfo->sport, pktinfo->dport);
	}

	/* 一轮统计结束之后，清除统计信息 */
	flowctx->hbstudytime = 0;
	flowctx->hbupnum = 0;
	flowctx->hbdownnum = 0;

	return ret;
}


void fi_rtt_judge_battle_flow(fi_pkt *pktinfo, fi_flowctx *flowctx, fi_gamectx *gamectx)
{
	int64_t  timediff;      /* msec */
	uint32_t avgpktlen;
	uint16_t uppktpersec;
	uint16_t downpktpersec;

	timediff = FI_MAX(pktinfo->msec - flowctx->studystarttime, 1);

	uppktpersec = flowctx->uppktnum * FI_MS / timediff;
	downpktpersec = flowctx->downpktnum * FI_MS / timediff;
	avgpktlen = flowctx->uppktbytes / FI_MAX(flowctx->uppktnum, 1);

	FI_LOGD(" : FI judge battle: up: %u/s, down: %u/s, avg_pkt_len: %u, times: %u, flow: %u,%u",
	        uppktpersec, downpktpersec, avgpktlen, flowctx->btflow_times, pktinfo->sport, pktinfo->dport);

	if ((uppktpersec >= FI_BATTLE_UP_PKT_PER_SEC) &&
	    (downpktpersec >= FI_BATTLE_DOWN_PKT_PER_SEC) &&
	    (avgpktlen < FI_AUDIO_BATTLE_BOUNDARY))
	{
		flowctx->btflow_times++;
	}
	else
	{
		flowctx->btflow_times = 0;
	}

	/* 标记流类型为对战流 */
	if (flowctx->btflow_times >= FI_BATTLE_FLOW_CON)
	{
		flowctx->flowtype = FI_FLOWTYPE_BATTLE;
		gamectx->battle_flow_port = FI_SRVPORT(pktinfo);
		gamectx->cliip = FI_CLIIP(pktinfo);
		FI_LOGD(" : FI learned a battle flow %u,%u, battle port %u.",
			    pktinfo->sport, pktinfo->dport, gamectx->battle_flow_port);
	}

	return;
}


void fi_rtt_judge_battle_start(fi_pkt *pktinfo, fi_flowctx *flowctx, fi_gamectx *gamectx)
{
	int64_t  timediff;      /* msec */
	uint16_t uppktpersec;
	uint16_t downpktpersec;

	timediff = FI_MAX(pktinfo->msec - flowctx->studystarttime, 1);
	uppktpersec = flowctx->uppktnum * FI_MS / timediff;
	downpktpersec = flowctx->downpktnum * FI_MS / timediff;

	/* 需要考虑对战中间休息的状态 */
	if ((FI_MIN(uppktpersec, downpktpersec) >= FI_BATTLE_UP_PKT_PER_SEC) &&
	    (FI_MAX(uppktpersec, downpktpersec) >= FI_BATTLE_DOWN_PKT_PER_SEC))
	{
		flowctx->battle_times++;
	}
	else
	{
		flowctx->battle_times = 0;
	}

	/* 连续多次满足条件才判断为对战开始 */
	if (flowctx->battle_times >= FI_BATTLE_START_THRESH)
	{
		gamectx->appstatus |= FI_STATUS_BATTLE_START;
		FI_LOGD(" : FI learned battle start status by flow %u,%u.",
		        pktinfo->sport, pktinfo->dport);

		/* 上报游戏状态: 对战开始 */
		fi_rtt_status(gamectx->appid, FI_STATUS_BATTLE_START);
	}

	return;
}


int fi_rtt_judge_battle_stop(fi_gamectx *gamectx, fi_app_info *appinfo)
{
	/* 如果游戏在后台，则不报对战结束，直到游戏被杀死后者调到前台 */
	if (appinfo->appstatus == GAME_SDK_STATE_BACKGROUND)
	{
		return FI_FALSE;
	}

	/* QQ飞车使用单独的逻辑 */
	if (gamectx->appid == FI_APPID_QQFC)
	{
		/* 判断qqfc是否断线: 超过n秒有上行但没下行 */
		if (gamectx->nodownlinkpkt > (gamectx->nouplinkpkt + FI_APP_DISCONN))
		{
			gamectx->applocalstatus |= FI_STATUS_DISCONN;
			FI_LOGD(" : FI disconn, nouplinkpkt=%u, nodownlinkpkt=%u.",
				    gamectx->nouplinkpkt, gamectx->nodownlinkpkt);
		}

		/* 发现大量上行说明逃离断线状态 */
		if (gamectx->preuplinkpktnum >= FI_QQFC_UP_PPS_MIN)
		{
			gamectx->applocalstatus = 0;
			FI_LOGD(" : FI reconn, preuplinkpktnum=%u, predownlinkpktnum=%u.",
				    gamectx->preuplinkpktnum, gamectx->predownlinkpktnum);
		}

		/* 断线的时间小于设定的阈值，不报对战结束 */
		if (FI_DISCONN(gamectx->applocalstatus) &&
		    (gamectx->nouplinkpkt < FI_APP_DISCONN_STOP))
	    {
			return FI_FALSE;
	    }
	}

	/* 对战流超过一段时间没有上下行报文则认为对战结束 */
	if ((gamectx->nouplinkpkt >= FI_BATTLE_STOP_THRESH) &&
	    (gamectx->nodownlinkpkt >= FI_BATTLE_STOP_THRESH))
	{
		/* 发送对战结束消息 */
		fi_rtt_status(gamectx->appid, FI_STATUS_BATTLE_STOP);

		/* 清除用于计算rtt的缓存, 设置游戏状态为对战未开始 */
		fi_flow_lock();
		fi_reset_gamectx(gamectx);
		fi_flow_unlock();

		return FI_TRUE;
	}

	return FI_FALSE;
}


void fi_rtt_judge_flow_type(fi_pkt *pktinfo, fi_flowctx *flowctx, fi_gamectx *gamectx)
{
	do
	{
		/* 通过历史端口进行流类型判断 */
		if (fi_rtt_study_by_port(pktinfo, flowctx, gamectx) == FI_SUCCESS)
		{
			break;
		}

		/* 太长时间没学到结果，流类型设置为unknown */
		if (pktinfo->msec - flowctx->flowstarttime > FI_MAX_STUDY_TIME)
		{
			flowctx->flowtype = FI_FLOWTYPE_UNKNOWN;
			break;
		}

		switch (gamectx->appid)
		{
			/* QQ飞车使用单独的学习规则 */
			case FI_APPID_QQFC:
			{
				fi_rtt_judge_qqfc(pktinfo, flowctx, gamectx);
				break;
			}
			case FI_APPID_HYXD:
			case FI_APPID_BH3:
			{
				/* do nothing */
				break;
			}
			default:
			{
				/* 判断是否为对战流 */
				fi_rtt_judge_battle_flow(pktinfo, flowctx, gamectx);
				break;
			}
		}
	} while (0);

	return;
}


void fi_rtt_flow_study(fi_pkt *pktinfo, fi_flowctx *flowctx, fi_gamectx *gamectx)
{
	/* 流类型自学习不需要用tcp */
	if (pktinfo->proto == FI_IPPROTO_TCP)
	{
		return;
	}

	/* 收集统计信息 */
	if (pktinfo->dir == FI_DIR_UP)
	{
		flowctx->hbupnum++;
		flowctx->uppktnum++;
		flowctx->uppktbytes += pktinfo->len;
	}
	else
	{
		/* 心跳流的下行可能掺杂了其它报文，这里过滤一下 */
		if (pktinfo->len < FI_HB_PKT_LEN)
		{
			flowctx->hbdownnum++;
		}
		flowctx->downpktnum++;
	}

	/* 判断是否为心跳流 */
	if (flowctx->flowtype == FI_FLOWTYPE_INIT)
	{
		if (fi_rtt_judge_hb(pktinfo, flowctx, gamectx) == FI_SUCCESS)
		{
			/* 如果为心跳流就不做对战流的判断 */
			return;
		}
	}

	/* 对战流每间隔n秒进行一次判断, 不到n秒直接返回 */
	if (pktinfo->msec - flowctx->studystarttime < FI_STUDY_INTERVAL)
	{
		return;
	}

	/* 判断流类型 */
	if (flowctx->flowtype == FI_FLOWTYPE_INIT)
	{
		fi_rtt_judge_flow_type(pktinfo, flowctx, gamectx);
	}

	/* 判断对战是否开始 */
	if (!FI_BATTLING(gamectx->appstatus))
	{
		/* QQ飞车、崩坏3 识别出对战流意味着对战开始 */
		if ((gamectx->appid == FI_APPID_QQFC) ||
		    (gamectx->appid == FI_APPID_BH3))
		{
			if (flowctx->flowtype == FI_FLOWTYPE_BATTLE)
			{
				gamectx->appstatus |= FI_STATUS_BATTLE_START;
				FI_LOGD(" : FI learned battle start status by battle flow %u,%u.",
						pktinfo->sport, pktinfo->dport);

				/* 上报游戏状态: 对战开始 */
				fi_rtt_status(gamectx->appid, FI_STATUS_BATTLE_START);
			}
		}
		else
		{
			fi_rtt_judge_battle_start(pktinfo, flowctx, gamectx);
		}
	}

	/* 每间隔n秒清除统计信息 */
	fi_rtt_reset_study(pktinfo, flowctx);

	return;
}


int fi_rtt_cal_tcprtt(fi_pkt *pktinfo, fi_flowctx *flowctx, fi_gamectx *gamectx)
{
	int32_t rawrtt = 0;   /* 根据seq/ack算出的最原始的rtt */
	int32_t newrtt = 0;   /* 在rawrtt的基础上经过min max修正过的rtt */
	char *logstr = "";

	/* 上行报文，仅仅记录seq */
	if (pktinfo->dir == FI_DIR_UP)
	{
		/* seq等于0说明上一个计算周期已经结束, 重新开始一个新的周期 */
		if (flowctx->seq == 0)
		{
			/* 有负载的报文才会有ack */
			if (pktinfo->len > 0)
			{
				/* 记录期望收到的ack和当前时间戳 */
				flowctx->seq = pktinfo->seq + pktinfo->len;
				flowctx->uppkttime = pktinfo->msec;
				FI_LOGD(" : FI save seq, seq=%u, nextseq=%u.",
				        pktinfo->seq, flowctx->seq);
			}
			return FI_SUCCESS;
		}

		/* 很长时间没有收到ack，根据上行报文跟新rtt */
		if (pktinfo->msec - gamectx->updatetime < FI_UPPKT_WAIT)
		{
			return FI_FAILURE;
		}

		/* 健壮性考虑, 太长时间没有收到ack, 重新选取一个报文 */
		if ((flowctx->uppkttime > 0) &&
		    (pktinfo->msec - flowctx->uppkttime > FI_ACK_MAX_WAIT))
		{
			FI_LOGD(" : FI reset tcprtt ctx.");
			flowctx->seq = 0;
			flowctx->uppkttime = 0;
			return FI_FAILURE;
		}

		/* 长时间没有收到ack, 则对rtt进行修正 */
		newrtt = FI_RANGE(pktinfo->msec - flowctx->uppkttime, FI_MIN_RTT, FI_MAX_ORG_RTT);
		logstr = "uplink pkt";
	}
	else
	{
		/* 判断是否为有效的ack */
		if (!(flowctx->uppkttime) || (pktinfo->ack - flowctx->seq > (uint32_t)FI_ACK_MAX))
		{
			return FI_FAILURE;
		}

		/* 根据时间差值计算原始rtt */
		rawrtt = pktinfo->msec - flowctx->uppkttime;
		gamectx->rawrtt = fi_get_srtt(gamectx->rawrtt, rawrtt, FI_SRTT_VAR);
		newrtt = FI_RANGE(rawrtt, FI_MIN_RTT, FI_MAX_ORG_RTT);

		FI_LOGD(" : FI get ack, appid=%u, rawrtt=%d, seq=%u, ack=%u, flow %u,%u.",
		        gamectx->appid, rawrtt, flowctx->seq, pktinfo->ack,
		        pktinfo->sport, pktinfo->dport);

		/* 这一轮rtt计算已结束，重置状态，准备下一轮 */
		flowctx->seq = 0;
		flowctx->uppkttime = 0;
		logstr = "ack";
	}

	/* 计算最终rtt，需要对原始rtt进行平滑 */
	fi_rtt_update_integral(gamectx, newrtt, pktinfo->msec);

	FI_LOGD(" : FI update rtt by tcp %s, appid=%u, newrtt=%d, battlertt=%d.",
			logstr, gamectx->appid, newrtt, gamectx->battlertt);

	return FI_SUCCESS;
}


int fi_rtt_cal_mptcp(fi_pkt *pktinfo, fi_flowctx *flowctx, fi_gamectx *gamectx)
{
	int32_t rawrtt = 0;   /* 根据seq/ack算出的最原始的rtt */
	int32_t newrtt = 0;   /* 在rawrtt的基础上经过min max修正过的rtt */
	fi_mpctx *mpctx = g_fi_ctx.mpctx + gamectx->appid;

	/* 上行报文，仅仅记录seq */
	if (pktinfo->dir == FI_DIR_UP)
	{
		/* 记下当前seq已经到哪里了，避免对重传报文计算rtt */
		mpctx->preseq = FI_MAX(mpctx->preseq, pktinfo->seq);

		/* seq等于0说明上一个计算周期已经结束, 重新开始一个新的周期 */
		if ((mpctx->seq == 0) && (pktinfo->len > 0) &&
		    ((pktinfo->seq >= mpctx->preseq) ||
		    (FI_ABS_SUB(pktinfo->seq, mpctx->preseq) > FI_RETRANS_PKT)))
		{
			/* 记录期望收到的ack和当前时间戳 */
			mpctx->seq = pktinfo->seq + pktinfo->len;
			mpctx->uppkttime = pktinfo->msec;
			mpctx->preseq = mpctx->seq;
			FI_LOGD(" : FI save seq, seq=%u, nextseq=%u, preseq=%u.",
			        pktinfo->seq, mpctx->seq, mpctx->preseq);
		}

		/* 健壮性考虑, 太长时间没有收到ack, 重新选取一个报文 */
		if ((mpctx->uppkttime > 0) &&
		    (pktinfo->msec - mpctx->uppkttime > FI_ACK_MAX_WAIT))
		{
			FI_LOGD(" : FI reset mptcp ctx.");
			memset(mpctx, 0, sizeof(fi_mpctx));
		}

		return FI_SUCCESS;
	}
	else
	{
		/* 记下当前seq已经到哪里了，避免对重传报文计算rtt */
		mpctx->preseq = FI_MAX(mpctx->preseq, pktinfo->ack);

		/* 判断是否为有效的ack */
		if (!(mpctx->uppkttime) || (pktinfo->ack - mpctx->seq > (uint32_t)FI_ACK_MAX))
		{
			return FI_FAILURE;
		}

		/* 根据时间差值计算原始rtt */
		rawrtt = pktinfo->msec - mpctx->uppkttime;
		gamectx->rawrtt = fi_get_srtt(gamectx->rawrtt, rawrtt, FI_SRTT_VAR);
		newrtt = FI_RANGE(rawrtt, FI_MIN_RTT, FI_MAX_ORG_RTT);

		FI_LOGD(" : FI get ack, rawrtt=%d, seq=%u, ack=%u, flow %u,%u.",
		        rawrtt, mpctx->seq, pktinfo->ack, pktinfo->sport, pktinfo->dport);

		/* 这一轮rtt计算已结束，重置状态，准备下一轮 */
		mpctx->seq = 0;
		mpctx->uppkttime = 0;
	}

	/* 计算最终rtt，需要对原始rtt进行平滑 */
	fi_rtt_update_integral(gamectx, newrtt, pktinfo->msec);

	FI_LOGD(" : FI update rtt by mptcp, appid=%u, newrtt=%d, battlertt=%d.",
			gamectx->appid, newrtt, gamectx->battlertt);

	return FI_SUCCESS;
}


int fi_rtt_cal_hyxd(fi_pkt *pktinfo, fi_flowctx *flowctx, fi_gamectx *gamectx)
{
	int32_t newrtt = 0;

	/* 需要访问负载，所以用bufdatalen */
	if (pktinfo->bufdatalen < FI_HYXD_MIN_LEN)
	{
		return FI_FAILURE;
	}

	/* 上行报文，取出seq记录下来 */
	if (pktinfo->dir == FI_DIR_UP)
	{
		/* 筛选携带数据的上行报文(非纯ack报文) */
		if (memcmp(pktinfo->data, hyxd_data, sizeof(hyxd_data)))
		{
			return FI_FAILURE;
		}

		/* 打上对战流标记，以后通过该标记过滤报文 */
		flowctx->flowtype = FI_FLOWTYPE_BATTLE;

		/* 取出seq号，只记录一个seq号 */
		if (flowctx->seq == 0)
		{
			flowctx->seq = fi_rtt_get_le_u32(pktinfo->data + FI_HYXD_SEQ_OFFSET);
			flowctx->uppkttime = pktinfo->msec;
		}

		flowctx->uppktnum++;

		/* 当长时间没有下行报文时，用上行报文来更新rtt */
		if ((pktinfo->msec - flowctx->uppkttime < FI_UPPKT_WAIT) ||
		    (flowctx->uppktnum <= FI_HB_LOST_TOLERANCE))
		{
			return FI_FAILURE;
		}

		newrtt = FI_RANGE(pktinfo->msec - flowctx->uppkttime, FI_MIN_RTT, FI_MAX_ORG_RTT);
	}
	else
	{
		uint32_t ack;

		/* 没有已记录的seq, 则无法配对seq - ack */
		if (flowctx->seq == 0)
		{
			return FI_FAILURE;
		}

		/* 取出ack号 */
		ack = fi_rtt_get_le_u32(pktinfo->data + FI_HYXD_ACK_OFFSET);
		if (ack - flowctx->seq > (uint32_t)FI_ACK_MAX)
		{
			return FI_FAILURE;
		}

		/* 计算原始rtt */
		newrtt = FI_RANGE(pktinfo->msec - flowctx->uppkttime, FI_MIN_RTT, FI_MAX_ORG_RTT);

		/* 这一轮rtt计算已结束，重置状态，准备下一轮 */
		flowctx->seq = 0;
		flowctx->uppkttime = 0;
		flowctx->uppktnum = 0;
	}

	/* 计算最终rtt，需要对原始rtt进行平滑 */
	fi_rtt_update_integral(gamectx, newrtt, pktinfo->msec);

	FI_LOGD(" : FI update hyxd rtt, appid=%u, rtt=%d.",
	        gamectx->appid, gamectx->battlertt);

	return FI_SUCCESS;
}


int fi_rtt_cal_battle(fi_pkt *pktinfo, fi_flowctx *flowctx, fi_gamectx *gamectx)
{
	char *logstr = "";

	/* 上行报文 */
	if (pktinfo->dir == FI_DIR_UP)
	{
		/* Wait for downlink data to start. */
		if (gamectx->downlast == 0)
		{
			return FI_FAILURE;
		}

		/* Use uplink pkt as a timer */
		if (pktinfo->msec - gamectx->downlast < FI_UPPKT_WAIT)
		{
			return FI_FAILURE;
		}

		/* For too long, there is no uplink pkt, update RTT. */
		gamectx->rtt = FI_RANGE(pktinfo->msec - gamectx->downlast,
		                        FI_MIN_RTT, FI_MAX_ORG_RTT);
		logstr = "uplink pkt";
	}
	else
	{
		/* Save the timestamp of the first downlink pkt. */
		if (gamectx->downlast == 0)
		{
			gamectx->downlast = pktinfo->msec;
			return FI_FAILURE;
		}

		/* Calculate RTT by adjacent downlink pkts */
		gamectx->rtt = FI_RANGE((pktinfo->msec - gamectx->downlast),
		                         FI_MIN_RTT, FI_MAX_ORG_RTT);
		gamectx->downlast = pktinfo->msec;
		logstr = "downlink pkt";
	}

	/* 计算最终rtt，需要对原始rtt进行平滑 */
	fi_rtt_update_integral(gamectx, gamectx->rtt, pktinfo->msec);

	FI_LOGD(" : FI update rtt by battle flow %s, appid=%u, rtt=%d.",
	        logstr, gamectx->appid, gamectx->battlertt);

	return FI_SUCCESS;
}


void fi_rtt_cal_hb(fi_pkt *pktinfo, fi_flowctx *flowctx, fi_gamectx *gamectx)
{
	/* 上行报文记下时间戳 */
	if (pktinfo->dir == FI_DIR_UP)
	{
		/* 统计连续的上行报文的个数 */
		flowctx->hbupnum++;

		/* 一连串的上行报文中, 记下最后一个上行报文的时间 */
		flowctx->uppkttime = pktinfo->msec;
	}
	/* 下行报文根据时间戳的差值计算rtt */
	else
	{
		flowctx->hbdownnum++;

		/* 前面没有心跳请求 */
		if (flowctx->uppkttime == 0)
		{
			return;
		}

		/* 收到心跳响应 */
		gamectx->hbrtt = FI_RANGE(pktinfo->msec - flowctx->uppkttime, FI_MIN_RTT, FI_MAX_RTT);
		gamectx->hbsrtt = fi_get_srtt(gamectx->hbsrtt, gamectx->hbrtt, FI_HB_SRTT_VAR);
		FI_LOGD(" : FI update rtt by heartbeat, appid=%u, hbrtt=%d, hbsrtt=%d.",
				gamectx->appid, gamectx->hbrtt, gamectx->hbsrtt);

		/* 本轮计算结束 */
		flowctx->uppkttime = 0;
	}

	/* 校验心跳流是否正确 */
	fi_rtt_judge_hb(pktinfo, flowctx, gamectx);
	if (flowctx->flowtype != FI_FLOWTYPE_HB)
	{
		gamectx->hbrtt= 0;
		gamectx->hbsrtt = 0;
	}

	return;
}


void fi_rtt_cal_uu(fi_pkt *pktinfo, fi_flowctx *flowctx, fi_gamectx *gamectx)
{
	static uint32_t uu_seq[FI_UU_CACHE_NUM] = {0};
	static int64_t  uu_time[FI_UU_CACHE_NUM] = {0};
	static uint32_t curcache = 0;
	uint32_t i;
	int32_t newrtt = 0;

	/* UU只使用tcp报文 */
	if (pktinfo->proto != FI_IPPROTO_TCP)
	{
		return;
	}

	/* UU加速器不同于tcprtt的是，不能使用负载为0的报文 */
	if (pktinfo->len == 0)
	{
		return;
	}

	/* 上行报文，仅仅记录seq和时间戳 */
	if (pktinfo->dir == FI_DIR_UP)
	{
		uu_seq[curcache] = pktinfo->seq + pktinfo->len;
		uu_time[curcache] = pktinfo->msec;
		curcache = (curcache + 1) % FI_UU_CACHE_NUM;
		return;
	}

	/* 剩下的就是下行报文，遍历上行缓存，计算rtt */
	for (i = 0; i < FI_UU_CACHE_NUM; i++)
	{
		/* 有效的缓存, 有效的ack */
		if (uu_seq[curcache] && (pktinfo->ack - uu_seq[curcache] < (uint32_t)FI_ACK_MAX))
		{
			/* 最后一个算的的rtt肯定比之前的小，覆盖之前的 */
			newrtt = FI_RANGE(pktinfo->msec - uu_time[curcache], FI_MIN_RTT, FI_MAX_ORG_RTT);
			uu_seq[curcache] = 0;
			uu_time[curcache] = 0;
		}

		curcache = (curcache + 1) % FI_UU_CACHE_NUM;
	}

	if (newrtt == 0)
	{
		return;
	}

	/* 计算最终rtt，需要对原始rtt进行平滑 */
	gamectx->rtt = newrtt;
	gamectx->srtt = fi_get_srtt(gamectx->srtt, newrtt, FI_SRTT_VAR);
	gamectx->battlertt = FI_MIN(gamectx->srtt, FI_MAX_RTT);

	FI_LOGD(" : FI update uu rtt, newrtt=%d, battlertt=%d.", newrtt, gamectx->battlertt);

	return;
}


void fi_rtt_cal_bh3(fi_pkt *pktinfo, fi_flowctx *flowctx, fi_gamectx *gamectx)
{
	fi_flow_bh3 *bh3cache;
	uint32_t newrtt = 0;
	uint16_t seq = 0;
	uint16_t ack = 0;
	uint16_t verify = 0;
	int i;

	/* 上行报文，取出seq记录下来 */
	if (pktinfo->dir == FI_DIR_UP)
	{
		/* 带seq的报文长度固定为12, 访问负载需要用bufdatalen */
		if (pktinfo->bufdatalen != FI_BH3_UP_LEN)
		{
			return;
		}

		/* 固定位置出现关键字 */
		if (pktinfo->data[FI_BH3_KEY_OFFSET_UP] != FI_BH3_KEY_WORD)
		{
			return;
		}

		/* 提取seq和verify */
		seq = ntohs(*(uint16_t *)(pktinfo->data + FI_BH3_SEQ_OFFSET_UP));
		verify = ntohs(*(uint16_t *)(pktinfo->data + FI_BH3_VRF_OFFSET_UP));

		/* 发现更大的seq, 则覆盖小的seq, 如果是相等的seq则在其后面找地方保存 */
		for (i = 0; i < FI_BH3_SEQ_CACHE_NUM; i++)
		{
			bh3cache = flowctx->flow_bh3 + i;
			if (seq > bh3cache->seq)
			{
				bh3cache->seq = seq;
				bh3cache->verify = verify;
				bh3cache->time = (uint32_t)pktinfo->msec;
				FI_LOGD(" : FI bh3 uplink, seq=%04x, verify=%04x, time=%u.",
				        seq, verify, bh3cache->time);
				break;
			}
		}

		/* 清理缓存中seq比当前seq小的项目 */
		for (i++; (i < FI_BH3_SEQ_CACHE_NUM) && (flowctx->flow_bh3[i].seq > 0); i++)
		{
			flowctx->flow_bh3[i].seq = 0;
		}

		return;
	}
	else
	{
		/* 带seq的报文长度固定为14, 访问负载需要用bufdatalen */
		if (pktinfo->bufdatalen != FI_BH3_DOWN_LEN)
		{
			return;
		}

		/* 固定位置出现关键字 */
		if (pktinfo->data[FI_BH3_KEY_OFFSET_DOWN] != FI_BH3_KEY_WORD)
		{
			return;
		}

		/* 提取ack和verify */
		ack = ntohs(*(uint16_t *)(pktinfo->data + FI_BH3_ACK_OFFSET_DOWN));
		verify = ntohs(*(uint16_t *)(pktinfo->data + FI_BH3_VRF_OFFSET_DOWN));

		FI_LOGD(" : FI bh3 downlink, ack=%04x, verify=%04x.", ack, verify);

		/* 在缓存中找配对的seq */
		for (i = 0; i < FI_BH3_SEQ_CACHE_NUM; i++)
		{
			bh3cache = flowctx->flow_bh3 + i;
			if ((ack == bh3cache->seq) && (verify == bh3cache->verify))
			{
				newrtt = (uint32_t)pktinfo->msec - bh3cache->time;
				bh3cache->seq = 0;
				FI_LOGD(" : FI bh3 downlink, ack=%04x, verify=%04x, rawrtt=%u.",
				        ack, verify, newrtt);
				break;
			}
		}

		/* 没找到 */
		if (i == FI_BH3_SEQ_CACHE_NUM)
		{
			return;
		}

		/* 没有专门的对战流识别算法，这里打上对战流标记 */
		flowctx->flowtype = FI_FLOWTYPE_BATTLE;

		/* 对原始rtt进行修正 */
		newrtt = FI_RANGE(newrtt, FI_MIN_RTT, FI_MAX_ORG_RTT);

		/* 直接计算srtt, 不使用积分平均 */
		gamectx->rtt = newrtt;
		gamectx->srtt = fi_get_srtt(gamectx->srtt, newrtt, FI_SRTT_VAR);
		gamectx->battlertt = FI_MIN(gamectx->srtt, FI_MAX_RTT);
		gamectx->updatetime = pktinfo->msec;

		FI_LOGD(" : FI bh3, rtt=%d, srtt=%d, btrtt=%d.",
				gamectx->rtt, gamectx->srtt, gamectx->battlertt);
	}

	return;
}


void fi_rtt_amend(fi_gamectx *gamectx, fi_gamectx *uuctx)
{
	switch (gamectx->appid)
	{
		case FI_APPID_CJZC:
		case FI_APPID_QJCJ:
		{
			/* 使用心跳rtt对最终rtt进行修正 */
			if (gamectx->hbsrtt)
			{
				gamectx->final_rtt = FI_RANGE(gamectx->battlertt +
				    gamectx->hbsrtt - FI_RTT_BASE, FI_MIN_RTT, FI_MAX_RTT);
			}
			else
			{
				gamectx->final_rtt = gamectx->battlertt;
			}

			break;
		}
		case FI_APPID_QQFC:
		{
			/* 如果正在使用UU，则用UU的rtt进行修正 */
			if ((gamectx->rawrtt < FI_GAME_UU_RTT) && (uuctx->battlertt > 0))
			{
				gamectx->final_rtt = uuctx->battlertt + FI_UU_BASE_RTT;
			}
			else
			{
				gamectx->final_rtt = gamectx->battlertt;
			}
			break;
		}
		default:
		{
			gamectx->final_rtt = gamectx->battlertt;
			break;
		}
	}

	return;
}


int fi_rtt_record_battle(fi_pkt *pktinfo, fi_gamectx *gamectx)
{
	/* 上行报文 */
	if (pktinfo->dir == FI_DIR_UP)
	{
		gamectx->uplinkpktnum++;
	}
	else
	{
		gamectx->downlinkpktnum++;
	}

	return FI_SUCCESS;
}


int fi_rtt_para_check(fi_pkt *pktinfo, fi_flowctx *flowctx, uint32_t appid)
{
	static int64_t  prepkttime = 0;

	if (!pktinfo || !flowctx || !FI_APPID_VALID(appid))
	{
		FI_LOGD(" : FI parameter error, appid=%u.", appid);
		return FI_FAILURE;
	}

	/* 报文时间混乱 */
	if (pktinfo->msec < prepkttime)
	{
		FI_LOGD(" : FI pkt timestamp error.");
		return FI_FAILURE;
	}

	/* 记录流开始时间 */
	if (flowctx->flowstarttime == 0)
	{
		flowctx->flowstarttime = pktinfo->msec;
	}

	/* 记录开始学习的时间 */
	if (flowctx->studystarttime == 0)
	{
		flowctx->studystarttime = pktinfo->msec;
	}

	return FI_SUCCESS;
}


int fi_rtt_filter_pkt(fi_pkt *pktinfo, uint32_t appid)
{
	int ret = FI_SUCCESS;

	/* 不同的游戏关注的报文类型不同 */
	switch (appid)
	{
		/* QQ飞车需要tcp和udp */
		case FI_APPID_QQFC:
		{
			break;
		}
		/* UU加速器仅需要tcp */
		case FI_APPID_UU:
		{
			if (pktinfo->proto == FI_IPPROTO_UDP)
			{
				ret = FI_FAILURE;
			}
			break;
		}
		default:
		{
			if (pktinfo->proto == FI_IPPROTO_TCP)
			{
				ret = FI_FAILURE;
			}
			break;
		}
	}

	return ret;
}


int fi_rtt_entrance(fi_pkt *pktinfo, fi_flowctx *flowctx, uint32_t appid)
{
	fi_gamectx *gamectx;

	/* 当前未支持uu和迅游，如果检测发现这两个加速器已启动，则直接返回 */
	if (g_fi_ctx.appinfo[FI_APPID_UU].valid || g_fi_ctx.appinfo[FI_APPID_XUNYOU].valid)
	{
		return FI_FAILURE;
	}

	/* 参数检查 */
	if (fi_rtt_para_check(pktinfo, flowctx, appid) != FI_SUCCESS)
	{
		return FI_FAILURE;
	}

	/* 根据游戏类型对报文进行筛选(是否需要TCP报文) */
	if (fi_rtt_filter_pkt(pktinfo, appid) != FI_SUCCESS)
	{
		return FI_FAILURE;
	}

	gamectx = g_fi_ctx.gamectx + appid;
	gamectx->appid = appid;

	/* UU加速器直接测算tcp rtt, 且仅仅只做这一件事 */
	if (appid == FI_APPID_UU)
	{
		fi_rtt_cal_uu(pktinfo, flowctx, gamectx);
		return FI_SUCCESS;
	}

	/* 流类型及游戏状态自学习 */
	if ((flowctx->flowtype == FI_FLOWTYPE_INIT) ||
	    !FI_BATTLING(gamectx->appstatus))
	{
		fi_rtt_flow_study(pktinfo, flowctx, gamectx);
	}

	/* 统计对战流每秒上下行报文个数 */
	if ((flowctx->flowtype == FI_FLOWTYPE_BATTLE) ||
	    (FI_SRVPORT(pktinfo) == gamectx->battle_flow_port))
	{
		fi_rtt_record_battle(pktinfo, gamectx);
	}

	/* 崩坏3直接尝试计算rtt */
	if (gamectx->appid == FI_APPID_BH3)
	{
		fi_rtt_cal_bh3(pktinfo, flowctx, gamectx);
		fi_rtt_amend(gamectx, g_fi_ctx.gamectx + FI_APPID_UU);
		return FI_SUCCESS;
	}

	/* 其它游戏，对战开始之后才测算rtt */
	if (!FI_BATTLING(gamectx->appstatus))
	{
		return FI_SUCCESS;
	}

	/* 不同的游戏采用不同的测算方案 */
	switch (appid)
	{
		case FI_APPID_HYXD:
		{
			fi_rtt_cal_hyxd(pktinfo, flowctx, gamectx);
			break;
		}
		case FI_APPID_QQFC:
		{
			/* QQ飞车使用TCP对战流计算rtt */
			if ((pktinfo->proto == FI_IPPROTO_TCP) &&
			    (FI_SRVPORT(pktinfo) == gamectx->battle_flow_port))
			{
				if (pktinfo->mptcp)
				{
					fi_rtt_cal_mptcp(pktinfo, flowctx, gamectx);
				}
				else
				{
					fi_rtt_cal_tcprtt(pktinfo, flowctx, gamectx);
				}
			}

			break;
		}
		default:
		{
			/* 根据对战流计算rtt */
			if (flowctx->flowtype == FI_FLOWTYPE_BATTLE)
			{
				fi_rtt_cal_battle(pktinfo, flowctx, gamectx);
			}

			/* 刺激战场和全军出击需要用心跳流辅助 */
			if (flowctx->flowtype == FI_FLOWTYPE_HB)
			{
				fi_rtt_cal_hb(pktinfo, flowctx, gamectx);
			}

			break;
		}
	}

	/* 用心跳rtt、uurtt对最终rtt进行辅助修正, 给最终的rtt赋值 */
	fi_rtt_amend(gamectx, g_fi_ctx.gamectx + FI_APPID_UU);

	return FI_SUCCESS;
}

