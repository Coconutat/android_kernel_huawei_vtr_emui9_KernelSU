

#include <linux/types.h>
#include <linux/string.h>
#include "huawei_platform/emcom/smartcare/nse/psr_pub.h"
#include "huawei_platform/emcom/smartcare/nse/psr_base.h"
#include "huawei_platform/emcom/smartcare/nse/psr_ac.h"
#include "huawei_platform/emcom/smartcare/nse/psr_init.h"
#include "huawei_platform/emcom/smartcare/nse/psr_char.h"
#include "huawei_platform/emcom/smartcare/nse/psr_dns.h"
#include "huawei_platform/emcom/smartcare/nse/psr_proc.h"


extern struct psr_mem_pool *psr_global_mem;

static uint32_t psr_proc_dns_header(struct psr_msg *msg, struct psr_pkt *pkt,
				    struct psr_ctx_data *ctx_data,
				    struct psr_result *result)
{
	uint8_t *pos = pkt->pos;
	uint32_t remain = pkt->remain;

	if (remain < 12) {
		return PSR_RET_FAILURE;
	}

	result->un.dns.query_id = (pos[0] << 8) + pos[1];
	PSR_UPDATE_PAYLOAD(pos, remain, 2);

	result->un.dns.is_req = (pos[0] & 0x80) ? false : true;
	PSR_UPDATE_PAYLOAD(pos, remain, 2);

	result->un.dns.query_cnt = (pos[0] << 8) + pos[1];
	PSR_UPDATE_PAYLOAD(pos, remain, 2);

	result->un.dns.answer_cnt = (pos[0] << 8) + pos[1];
	PSR_UPDATE_PAYLOAD(pos, remain, 2);

	result->un.dns.auth_cnt = (pos[0] << 8) + pos[1];
	PSR_UPDATE_PAYLOAD(pos, remain, 2);

	result->un.dns.additional_cnt = (pos[0] << 8) + pos[1];
	PSR_UPDATE_PAYLOAD(pos, remain, 2);

	PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, 12);

	return 0;
}

static uint32_t psr_proc_dns_get_name_inner(struct psr_msg *msg,
					    struct psr_pkt *pkt,
					    uint16_t offset,
					    struct psr_ctx_data *ctx_data,
					    uint16_t *no_recu_len,
					    uint16_t remain,
					    struct psr_result *result,
					    struct psr_dns_tlv *tlv,
					    uint8_t recu_cnt)
{
	uint32_t   ret, i;
	uint8_t   *pos;
	uint32_t   left;
	uint8_t    ref_flag;
	uint8_t    cur_len;
	uint16_t   len = 0;
	uint16_t   next_offset;
	uint32_t   while_loop = 0;

	if (offset < pkt->len) {
		pos = pkt->payload + offset;
		left = pkt->len - offset;
	} else {
		return PSR_RET_FAILURE;
	}

	do {
		if (while_loop++ > PSR_MAX_LOOP_CNT) {
			PSR_LOG(PSR_RET_FAILURE);
			return PSR_RET_FAILURE;
		}

		ref_flag = pos[0] >> 6;

		if (PSR_DNS_DOMAIN_NAME_COMPRESSION == ref_flag) {
			if (left < 2) {
				return PSR_RET_FAILURE;
			}

			next_offset = ((pos[0] & 0x3f) << 8) + pos[1];
			if (next_offset >= offset) {
				return PSR_RET_FAILURE;
			}

			len += 2;

			if (recu_cnt >= PSR_MAX_RECU_DEPTH) {
				return PSR_RET_MEM_INVALID;
			}
			ret = psr_proc_dns_get_name_inner(msg, pkt, next_offset,
							  ctx_data, &len,
							  remain, result, tlv,
							  recu_cnt + 1);
			if (!*no_recu_len)
				*no_recu_len = len;

			return ret;
		}
		else if (PSR_DNS_DOMAIN_NAME_NO_COMPRESSION == ref_flag) {
			cur_len = pos[0];
			if (cur_len) {
				if ((1 + cur_len) > left) {
					return PSR_RET_FAILURE;
				}

				if (tlv->len) {
					if (remain < 1) {
						return PSR_RET_MEM_INVALID;
					}
					tlv->val[tlv->len] = '.';
					remain--;
					tlv->len++;
					len++;
				}

				if (remain < cur_len) {
					return PSR_RET_MEM_INVALID;
				}

				for (i = 0; i < cur_len; i++)
					tlv->val[(uint32_t)(tlv->len) + i] = pos[i + (uint32_t)1];

				remain -= cur_len;
			} else {
				if (!*no_recu_len)
					*no_recu_len = tlv->len + 2;

				break;
			}
			tlv->len += cur_len;
			PSR_UPDATE_PAYLOAD(pos, left, cur_len + 1);
			len += cur_len;
		} else {
			return PSR_RET_FAILURE;
		}
	} while (true);

	return 0;
}

static uint32_t  psr_proc_dns_get_name(struct psr_msg *msg, struct psr_pkt *pkt,
				       struct psr_ctx_data *ctx_data,
				       struct psr_result *result)
{
	uint32_t ret;
	uint16_t no_recu_len = 0;
	struct psr_dns_tlv *tlv;
	uint16_t remain;

	remain = result->base.total_len - result->base.valid_len;

	if (remain <= sizeof(struct psr_dns_tlv)) {
		return PSR_RET_MEM_INVALID;
	}

	tlv = (struct psr_dns_tlv *)((void *)(result)  + result->base.valid_len);
	tlv->type = PSR_DNS_ITEM_QUERY;

	remain -= sizeof(struct psr_dns_tlv);
	ret = psr_proc_dns_get_name_inner(msg, pkt,
					  (uint16_t)(pkt->len - pkt->remain),
					  ctx_data, &no_recu_len, remain,
					  result, tlv, 0);
	if (ret)
		return ret;

	result->base.valid_len += (tlv->len + sizeof(struct psr_dns_tlv));
	PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, no_recu_len);

	return ret;
}

static uint32_t psr_proc_dns_query(struct psr_msg *msg, struct psr_pkt *pkt,
				   struct psr_ctx_data *ctx_data,
				   struct psr_result *result)
{
	uint32_t  ret;
	uint32_t  i;
	uint32_t  while_loop = 0;

	for (i = 0; i < result->un.dns.query_cnt; i++) {
		ret = psr_proc_dns_get_name(msg, pkt, ctx_data, result);
		if (ret)
			return ret;

		if (pkt->remain < 4) {
			return PSR_RET_FAILURE;
		}
		PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, 2 + 2);

		if (while_loop++ > PSR_MAX_LOOP_CNT) {
			PSR_LOG(PSR_RET_FAILURE);
			return PSR_RET_FAILURE;
		}
	}

	return 0;
}

static uint32_t psr_proc_dns_rsp_rdata(struct psr_msg *msg, struct psr_pkt *pkt,
				       uint16_t rdata_cnt,
				       struct psr_ctx_data *ctx_data,
				       struct psr_result *result)
{
	uint32_t  i, j;
	uint16_t  type;
	uint16_t  data_len, tlv_len;
	struct psr_dns_tlv *tlv;
	uint32_t while_loop = 0;

	for (i = 0; i < rdata_cnt; i++) {
		if (pkt->remain < 12) {
			return PSR_RET_FAILURE;
		}

		if (PSR_DNS_DOMAIN_NAME_COMPRESSION == pkt->pos[0] >> 6) {
			PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, 2);
		} else {
			return PSR_RET_FAILURE;
		}

		type = (pkt->pos[0] << 8) + pkt->pos[1];

		PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, 2 + 2 + 4);

		data_len = (pkt->pos[0] << 8) + pkt->pos[1];
		if (data_len > pkt->remain) {
			return PSR_RET_FAILURE;
		} else if ((data_len + 2) > pkt->remain) {
		}

		PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, 2);

		if ((PSR_DNS_QTYPE_A == type) || (PSR_DNS_QTYPE_AAAA == type)) {
			tlv_len = sizeof(struct psr_dns_tlv) + data_len;
			if ((result->base.valid_len + tlv_len) >
			    result->base.total_len) {
				return PSR_RET_MEM_INVALID;
			}

			tlv = (struct psr_dns_tlv*)((void*)(result) +
						result->base.valid_len);

			if (PSR_DNS_QTYPE_A == type)
				tlv->type = PSR_DNS_ITEM_IP_V4;
			else
				tlv->type = PSR_DNS_ITEM_IP_V6;

			tlv->len = data_len;
			for (j = 0; j < data_len; j++)
				tlv->val[j] = pkt->pos[j];

			result->base.valid_len += tlv_len;
		}

		PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, data_len);

		if (while_loop++ > PSR_MAX_LOOP_CNT) {
			PSR_LOG(PSR_RET_FAILURE);
			return PSR_RET_FAILURE;
		}
	}

	return 0;
}

uint32_t psr_ins_dns(uint16_t proto, uint32_t direction, uint64_t ts,
		     struct psr_pkt_node *pkt_node,
		     struct psr_ctx_data *ctx_data, struct psr_result *result)
{
	uint32_t ret = 0;
	struct psr_pkt cur_node;
	struct psr_msg msg;

	msg.mem_pool = psr_global_mem;
	msg.ctx_data = ctx_data;
	msg.result = result;
	msg.pkt_node = pkt_node;
	msg.cur_node = &cur_node;
	msg.direction = direction;

	result->base.proto = PSR_DNS_PROTOCOL;
	result->base.valid_len = sizeof(struct psr_base_result) +
				 sizeof(struct psr_dns_result);

	if ((!pkt_node->pkt_len) || (!pkt_node->pkt_data)) {
		return PSR_RET_INVALID_PARAM;
	}

	cur_node.pos = pkt_node->pkt_data;
	cur_node.payload = pkt_node->pkt_data;
	cur_node.len = pkt_node->pkt_len;
	cur_node.remain = pkt_node->pkt_len;

	do {
		ret = psr_proc_dns_header(&msg, &cur_node, ctx_data, result);
		if (ret)
			break;

		ret = psr_proc_dns_query(&msg, &cur_node, ctx_data, result);
		if (ret)
			break;

		ret = psr_proc_dns_rsp_rdata(&msg, &cur_node,
					     result->un.dns.answer_cnt,
					     ctx_data, result);
		if (ret)
			break;

		ret = psr_proc_dns_rsp_rdata(&msg, &cur_node,
					     result->un.dns.auth_cnt,
					     ctx_data, result);
		if (ret)
			break;

		ret = psr_proc_dns_rsp_rdata(&msg, &cur_node,
					     result->un.dns.additional_cnt,
					     ctx_data, result);
		if (ret)
			break;

		result->un.dns.pkt_time = ts;

		result->un.dns.finished = 1;
		result->base.need_send = 1;
	} while (0);

	return ret;
}
