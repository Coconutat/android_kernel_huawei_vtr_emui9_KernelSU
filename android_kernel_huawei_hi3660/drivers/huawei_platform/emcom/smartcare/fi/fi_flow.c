

#include <linux/types.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/ip.h>
#include "../../emcom_netlink.h"
#include "../../emcom_utils.h"
#include <huawei_platform/emcom/smartcare/fi/fi_utils.h>

fi_flow g_fi_flow;


uint32_t fi_flow_node_num(void)
{
	return atomic_read(&g_fi_flow.nodenum);
}


void fi_flow_lock(void)
{
	spin_lock_bh(&(g_fi_flow.flow_lock));
}


void fi_flow_unlock(void)
{
	spin_unlock_bh(&(g_fi_flow.flow_lock));
}


inline uint32_t fi_flow_hash(uint32_t sip, uint32_t dip, uint32_t sport, uint32_t dport)
{
	uint32_t v1, v2, h1;

	v1 = sip^dip;
	v2 = sport^dport;

	h1 = v1<<8;
	h1 ^= v1>>4;
	h1 ^= v1>>12;
	h1 ^= v1>>16;
	h1 ^= v2<<6;
	h1 ^= v2<<10;
	h1 ^= v2<<14;
	h1 ^= v2<<7;

	return h1 & FI_FLOW_TABLE_MASK;
}


fi_flow_node *fi_flow_find(fi_pkt *pktinfo, fi_flow_head *head)
{
	fi_flow_node *node;

	list_for_each_entry(node, &(head->list), list)
	{
		if (FI_FLOW_SAME(node, pktinfo))
		{
			return node;
		}
	}

	return NULL;
}


fi_flow_node *fi_flow_add(fi_pkt *pktinfo, fi_flow_head *head)
{
	fi_flow_node *newnode;

	/* 流规模超出上限 */
	if (atomic_read(&g_fi_flow.nodenum) > FI_FLOW_NODE_LIMIT)
	{
		FI_LOGD(" : FI flow node out of limit");
		return NULL;
	}

	newnode = (fi_flow_node *)fi_malloc(sizeof(fi_flow_node));
	if (newnode == NULL)
	{
		FI_LOGD(" : FI failed to malloc for new fi flow node");
		return NULL;
	}

	newnode->sip = pktinfo->sip;
	newnode->dip = pktinfo->dip;
	newnode->sport = pktinfo->sport;
	newnode->dport = pktinfo->dport;
	newnode->updatetime = jiffies_to_msecs(jiffies);

	list_add(&(newnode->list), &(head->list));
	atomic_inc(&g_fi_flow.nodenum);

	FI_LOGD(" : FI new flow node, port %u,%u.", pktinfo->sport, pktinfo->dport);

	return newnode;
}


fi_flow_node *fi_flow_get(fi_pkt *pktinfo, fi_flow_head *head, uint32_t addflow)
{
	fi_flow_node *node;

	node = fi_flow_find(pktinfo, head);
	if (node != NULL)
	{
		node->updatetime = jiffies_to_msecs(jiffies);
		return node;
	}

	if (addflow)
	{
		return fi_flow_add(pktinfo, head);
	}
	else
	{
		return NULL;
	}
}


inline fi_flow_head *fi_flow_header(uint32_t index)
{
	index = index & FI_FLOW_TABLE_MASK;

	return &(g_fi_flow.flow_table[index]);
}


void fi_flow_init(void)
{
	int i;

	memset(&g_fi_flow, 0, sizeof(g_fi_flow));

	for (i = 0; i < FI_FLOW_TABLE_SIZE; i++)
	{
		INIT_LIST_HEAD(&(g_fi_flow.flow_table[i].list));
	}

	spin_lock_init(&g_fi_flow.flow_lock);

	return;
}


void fi_flow_age(void)
{
	int i;
	fi_flow_head *head;
	fi_flow_node *node;
	fi_flow_node *tmp;
	uint32_t curtime = jiffies_to_msecs(jiffies);

	for (i = 0; i < FI_FLOW_TABLE_SIZE; i++)
	{
		head = g_fi_flow.flow_table + i;
		if (list_empty(&(head->list)))
		{
			continue;
		}

		fi_flow_lock();
		list_for_each_entry_safe(node, tmp, &(head->list), list)
		{
			if (curtime - node->updatetime > FI_FLOW_AGING_TIME)
			{
				list_del(&(node->list));
				fi_free(node, sizeof(fi_flow_node));
				atomic_dec(&g_fi_flow.nodenum);
			}
		}
		fi_flow_unlock();
	}
	return;
}


void fi_flow_clear(void)
{
	int i;
	fi_flow_head *head;
	fi_flow_node *node;
	fi_flow_node *tmp;

	/* 遍历hash桶 */
	for (i = 0; i < FI_FLOW_TABLE_SIZE; i++)
	{
		head = g_fi_flow.flow_table + i;

		/* 遍历链表, 释放流结点 */
		if (list_empty(&(head->list)))
		{
			continue;
		}

		fi_flow_lock();
		list_for_each_entry_safe(node, tmp, &(head->list), list)
		{
			list_del(&(node->list));
			fi_free(node, sizeof(fi_flow_node));
			atomic_dec(&g_fi_flow.nodenum);
		}
		fi_flow_unlock();
	}

	return;
}

