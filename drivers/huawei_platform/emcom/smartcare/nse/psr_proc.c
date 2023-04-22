

#include <linux/string.h>
#include <linux/types.h>
#include "huawei_platform/emcom/smartcare/nse/psr_pub.h"
#include "huawei_platform/emcom/smartcare/nse/psr_base.h"
#include "huawei_platform/emcom/smartcare/nse/psr_ac.h"
#include "huawei_platform/emcom/smartcare/nse/psr_init.h"
#include "huawei_platform/emcom/smartcare/nse/psr_char.h"
#include "huawei_platform/emcom/smartcare/nse/psr_dns.h"
#include "huawei_platform/emcom/smartcare/nse/psr_proc.h"

static const int8_t psr_hex_tbl[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

static uint32_t psr_get_block(uint8_t type, struct psr_result **result,
			      struct psr_http_tlv **block)
{
	uint32_t len = sizeof(struct psr_base_result) +
		       sizeof(struct psr_http_result);
	struct psr_http_tlv *block_tmp;
	uint32_t while_loop = 0;

	*block = NULL;
	if (PSR_GET_BIT((*result)->un.http.block_bit, type)) {
		while (len < (*result)->base.valid_len) {
			block_tmp = (struct psr_http_tlv *)((void *)(*result) +
							    len);

			if (while_loop++ > PSR_MAX_LOOP_CNT) {
				PSR_LOG(PSR_RET_FAILURE);
				return PSR_RET_FAILURE;
			}

			if (type == block_tmp->type) {
				*block = block_tmp;
				break;
			}
			len += (sizeof(struct psr_http_tlv) + block_tmp->len);
		}
	}

	if (!*block) {
		if (((*result)->base.valid_len + sizeof(struct psr_http_tlv)) >
			(*result)->base.total_len) {
			return PSR_RET_MEM_INVALID;
		}

		*block = (struct psr_http_tlv *)((void *)(*result) +
						 (*result)->base.valid_len);
		(*block)->len = 0;
		(*block)->type = type;
		(*result)->base.valid_len += sizeof(struct psr_http_tlv);
		PSR_SET_BIT((*result)->un.http.block_bit, type);
	}

	return 0;
}

static uint32_t psr_proc_http_method(struct psr_msg *msg, struct psr_pkt *pkt,
				     struct psr_ctx_data *ctx_data,
				     struct psr_result *result)
{
	uint32_t ret;
	struct psr_match_pattern  pattern_tmp = {0};
	struct psr_match_pattern *match_pattern;
	void **cur_state = ctx_data->ac_state;
	uint32_t offset = 0;
	bool done = false;
	struct psr_ac_pattern *pattern;
	struct psr_ac_node *node;
	uint32_t while_loop = 0;

	while ((offset < pkt->remain) && (!done)) {
		match_pattern = NULL;
		ret = psr_ac_search(&pattern_tmp,
				    msg->mem_pool->ac_info.method_ac, pkt->pos,
				    pkt->remain, &cur_state, &offset,
				    &match_pattern);
		if (ret)
			break;

		if (match_pattern) {
			pattern = match_pattern->pattern;
			while (pattern) {
				node = (struct psr_ac_node *)(pattern->ac_node);

				if (while_loop++ > PSR_MAX_LOOP_CNT) {
					PSR_LOG(PSR_RET_FAILURE);
					return PSR_RET_FAILURE;
				}

				if (ctx_data->state.method_state.scan_len ==
				    (pattern->len - offset)) {
					result->un.http.method = node->id;
					done = true;

					break;
				}
				pattern = pattern->next;
			}
		}
		else if (while_loop++ > PSR_MAX_LOOP_CNT) {
			PSR_LOG(PSR_RET_FAILURE);
			return PSR_RET_FAILURE;
		}
	}

	PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, offset);

	if ((offset + ctx_data->state.method_state.scan_len) <
	    PSR_MAX_METHOD_SCAN_LEN)
		ctx_data->state.method_state.scan_len += offset;
	else
		ctx_data->state.method_state.scan_len = PSR_MAX_METHOD_SCAN_LEN;

	if (done) {
		ctx_data->ac_state = NULL;
		ctx_data->state.val = 0;
		ctx_data->psr_state = PSR_STATE_URI;
	}
	else {
		ctx_data->ac_state = cur_state;
	}

	return 0;
}

static uint32_t  psr_proc_http_uri(struct psr_msg *msg, struct psr_pkt *pkt,
				   struct psr_ctx_data *ctx_data,
				   struct psr_result *result)
{
	int32_t cpy_len = 0;
	bool done = false;
	struct psr_http_tlv *block;
	uint32_t pos;
	uint32_t ret;

	for (pos = 0; pos < pkt->remain; pos++) {
		if ((' ' == pkt->pos[pos]) || ('\t' == pkt->pos[pos])) {
			done = true;
			pos++;
			break;
		}
	}

	ret = psr_get_block(PSR_TYPE_URI, &result, &block);
	if (ret)
		return ret;

	cpy_len = done ? (pos - 1) : pos;

	if ((cpy_len + block->len) >= MAX_URI_LEN)
		cpy_len = MAX_URI_LEN - block->len;

	if (cpy_len > 0) {
		if (result->base.valid_len + cpy_len > result->base.total_len) {
			return PSR_RET_MEM_INVALID;
		}

		memcpy(block->val + block->len, pkt->pos, cpy_len);
	} else if (cpy_len < 0) {
		return PSR_RET_FAILURE;
	}

	block->len += cpy_len;
	result->base.valid_len += cpy_len;

	PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, pos);

	if (done) {
		PSR_SET_BIT(ctx_data->blk_bit_finish, PSR_TYPE_URI);
		ctx_data->ac_state = NULL;
		ctx_data->state.val = 0;
		ctx_data->psr_state = PSR_STATE_REQ_HTTP_VERSION;

		ctx_data->valid_len = result->base.valid_len;
	}

	return 0;
}

static uint32_t  psr_proc_http_req_ver(struct psr_msg *msg, struct psr_pkt *pkt,
				       struct psr_ctx_data *ctx_data,
				       struct psr_result*result)
{
	uint32_t ret;
	struct psr_match_pattern pattern_tmp = {0};
	struct psr_match_pattern *match_pattern;
	void **cur_state = ctx_data->ac_state;
	uint32_t offset = 0;
	bool done = false;
	struct psr_ac_pattern *pattern;
	struct psr_ac_node *node;
	uint32_t while_loop = 0;

	while ((offset < pkt->remain) && (!done)) {
		match_pattern = NULL;
		ret = psr_ac_search(&pattern_tmp,
				    msg->mem_pool->ac_info.version_ac,
				    pkt->pos, pkt->remain, &cur_state,
				    &offset, &match_pattern);
		if (ret)
			break;

		if  (match_pattern) {
			pattern = match_pattern->pattern;
			while (pattern) {
				node = (struct psr_ac_node *)(pattern->ac_node);

				if (while_loop++ > PSR_MAX_LOOP_CNT) {
					PSR_LOG(PSR_RET_FAILURE);
					return PSR_RET_FAILURE;
				}

				if (PSR_HTTP_VERSION_INNER_FL_END == node->id) {
					done = true;

					if (!ctx_data->state.version.got) {
						result->un.http.version =
							PSR_HTTP_VERSION_OTHER;
					}
					break;
				}

				if (!ctx_data->state.version.got) {
					result->un.http.version = node->id;
					ctx_data->state.version.got = true;
					break;
				}
				pattern = pattern->next;
			}
		}
		else if (while_loop++ > PSR_MAX_LOOP_CNT) {
			PSR_LOG(PSR_RET_FAILURE);
			return PSR_RET_FAILURE;
		}
	}

	PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, offset);

	if (done) {
		ctx_data->ac_state = NULL;
		ctx_data->state.val = 0;
		ctx_data->psr_state = PSR_STATE_HEADER;
	} else {
		ctx_data->ac_state = cur_state;
	}

	return 0;
}

static uint32_t  psr_proc_http_rsp_ver(struct psr_msg *msg, struct psr_pkt *pkt,
				       struct psr_ctx_data *ctx_data,
				       struct psr_result *result)
{
	uint32_t ret;
	struct psr_match_pattern pattern_tmp = {0};
	struct psr_match_pattern *match_pattern;
	void **cur_state = ctx_data->ac_state;
	uint32_t offset = 0;
	bool done = false;
	struct psr_ac_pattern *pattern;
	struct psr_ac_node *node;
	uint32_t while_loop = 0;

	while ((offset < pkt->remain) && (!done)) {
		match_pattern = NULL;
		ret = psr_ac_search(&pattern_tmp,
				    msg->mem_pool->ac_info.version_ac,
				    pkt->pos, pkt->remain, &cur_state,
				    &offset, &match_pattern);
		if (ret)
			break;

		if (match_pattern) {
			pattern = match_pattern->pattern;
			while (pattern) {
				node = (struct psr_ac_node*)(pattern->ac_node);

				if (while_loop++ > PSR_MAX_LOOP_CNT) {
					PSR_LOG(PSR_RET_FAILURE);
					return PSR_RET_FAILURE;
				}

				if (PSR_HTTP_VERSION_INNER_OVER == node->id) {
					done = true;
					break;
				}
				pattern = pattern->next;
			}
		}
		else if (while_loop++ > PSR_MAX_LOOP_CNT) {
			PSR_LOG(PSR_RET_FAILURE);
			return PSR_RET_FAILURE;
		}

	}

	PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, offset);
	if (done) {
		ctx_data->ac_state = NULL;
		ctx_data->state.val = 0;
		ctx_data->psr_state = PSR_STATE_RSP_CODE;
	} else {
		ctx_data->ac_state = cur_state;
	}

	return 0;
}

static uint32_t  psr_proc_http_header(struct psr_msg *msg, struct psr_pkt *pkt,
				      struct psr_ctx_data *ctx_data,
			       	      struct psr_result *result)
{
	uint32_t ret;
	struct psr_match_pattern pattern_tmp = {0};
	struct psr_match_pattern *match_pattern;
	void **cur_state = ctx_data->ac_state;
	uint32_t offset = 0;
	bool done = false;
	uint32_t scan_len = ctx_data->state.header.scan_len;
	struct psr_ac_pattern *pattern;
	struct psr_ac_node *node;
	uint32_t while_loop = 0;

	while ((offset < pkt->remain) && (!done)) {
		match_pattern = NULL;
		ret = psr_ac_search(&pattern_tmp,
				    msg->mem_pool->ac_info.header_ac,
				    pkt->pos, pkt->remain, &cur_state,
				    &offset, &match_pattern);
		if (ret)
			break;

		if (match_pattern) {
			pattern = match_pattern->pattern;
			while (pattern) {
				node = (struct psr_ac_node*)(pattern->ac_node);

				if (while_loop++ > PSR_MAX_LOOP_CNT) {
					PSR_LOG(PSR_RET_FAILURE);
					return PSR_RET_FAILURE;
				}

				if (PSR_TYPE_HEADER_END == node->id) {
					PSR_UPDATE_PAYLOAD(pkt->pos,
							   pkt->remain,
							   offset);
					ctx_data->ac_state = NULL;
					ctx_data->state.val = 0;
					ctx_data->psr_state =
						PSR_STATE_BODY_START;
					return 0;
				}

				if ((PSR_TYPE_HEADER_LINE_END) == node->id) {
					scan_len = 0;
					PSR_UPDATE_PAYLOAD(pkt->pos,
							   pkt->remain,
							   offset);
					offset = 0;
					pattern = pattern->next;
					continue;
				}

				if ((scan_len + offset) == node->len) {
					ctx_data->state.header.type = node->id;
					ctx_data->state.header.got = true;
					done = true;
					break;
				}

				pattern = pattern->next;
			}
		}
		else if (while_loop++ > PSR_MAX_LOOP_CNT) {
			PSR_LOG(PSR_RET_FAILURE);
			return PSR_RET_FAILURE;
		}
	}

	PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, offset);

	if (ctx_data->state.header.got) {
		ctx_data->ac_state = NULL;
		ctx_data->psr_state = PSR_STATE_HEADER_VALUE;
	} else {
		ctx_data->ac_state = cur_state;
		scan_len += offset;
		if (scan_len >= PSR_MAX_HEADER_LEN)
			ctx_data->state.header.scan_len = PSR_MAX_HEADER_LEN;
		else
			ctx_data->state.header.scan_len = scan_len;
	}
	return 0;
}

static void psr_proc_http_header_val_end(struct psr_msg *msg,
					 struct psr_pkt *pkt,
					 struct psr_ctx_data *ctx_data,
					 struct psr_result *result)
{
	uint32_t i;

	for (i = 0; i < pkt->remain; i++) {
		if (LF == pkt->pos[i]) {
			ctx_data->state.header.lf_num++;
			if (2 == ctx_data->state.header.lf_num) {
				ctx_data->ac_state = NULL;
				ctx_data->state.val = 0;
				ctx_data->psr_state = PSR_STATE_BODY_START;
				i++;
				break;
			}
		} else if (1 == ctx_data->state.header.lf_num) {
			if (('\t' != pkt->pos[i]) && (' '!= pkt->pos[i]) &&
			    (CR != pkt->pos[i])) {
				ctx_data->ac_state = NULL;
				ctx_data->state.val = 0;
				ctx_data->psr_state = PSR_STATE_HEADER;
				break;
			}
		}
	}

	PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, i);
}

static uint32_t psr_proc_http_header_str(uint8_t str_type, uint8_t max_len,
					 struct psr_msg *msg,
					 struct psr_pkt *pkt,
					 struct psr_ctx_data *ctx_data,
					 struct psr_result *result)
{
	uint32_t i;
	int32_t cpy_len;
	int32_t line_len;
	struct psr_http_tlv *block;
	uint32_t ret;

	PSR_GET_HEADER_END(i, pkt->pos, pkt->remain,
			   ctx_data->state.header.lf_num,
			   ctx_data->state.header.line_end, line_len, cpy_len);

	if ((!PSR_GET_BIT(ctx_data->blk_bit_finish, str_type)) &&
	     (cpy_len > 0)) {
		ret = psr_get_block(str_type, &result, &block);
		if (ret)
			return ret;

		if ((cpy_len + block->len) >= max_len)
			cpy_len = max_len - block->len;

		if (cpy_len > 0) {
			if ((result->base.valid_len + cpy_len) >
			     result->base.total_len) {
				return PSR_RET_MEM_INVALID;
			}

			memcpy(block->val + block->len, pkt->pos, cpy_len);
		} else if (cpy_len < 0) {
			return PSR_RET_FAILURE;
		}

		block->len  += cpy_len;
		result->base.valid_len += cpy_len;
	}

	if (ctx_data->state.header.line_end) {
		PSR_SET_BIT(ctx_data->blk_bit_finish, str_type);

		ctx_data->valid_len = result->base.valid_len;
	}

	PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, line_len);

	return 0;
}

static uint32_t psr_proc_http_header_ref(struct psr_msg *msg,
					 struct psr_pkt *pkt,
					 struct psr_ctx_data *ctx_data,
					 struct psr_result *result)
{
	uint32_t i;
	int32_t cpy_len;
	int32_t line_len;
	struct psr_http_tlv *block;
	struct psr_match_pattern pattern_tmp = {0};
	struct psr_match_pattern *match_pattern;
	void **cur_state = ctx_data->ac_state;
	uint32_t offset = 0;
	struct psr_ac_pattern *pattern;
	struct psr_ac_node *node;
	uint32_t step = ctx_data->state.header.get_val_step;
	uint32_t ret;
	uint32_t while_loop = 0;

	if (!step) {
		while ((offset < pkt->remain) && (!step)) {
			match_pattern = NULL;
			ret = psr_ac_search(&pattern_tmp,
					    msg->mem_pool->ac_info.refer_ac,
					    pkt->pos, pkt->remain, &cur_state,
					    &offset, &match_pattern);
			if (ret)
				break;

			if (match_pattern) {
				pattern = match_pattern->pattern;
				while (pattern) {
					node = (struct psr_ac_node*)
						(pattern->ac_node);

					if (while_loop++ > PSR_MAX_LOOP_CNT) {
						PSR_LOG(PSR_RET_FAILURE);
						return PSR_RET_FAILURE;
					}

					if (PSR_TYPE_REFER_HOST == node->id) {
						step++;
						break;
					}
					pattern = pattern->next;
				}
			}
			else if (while_loop++ > PSR_MAX_LOOP_CNT) {
				PSR_LOG(PSR_RET_FAILURE);
				return PSR_RET_FAILURE;
			}
		}

		if (!step)
			ctx_data->ac_state = cur_state;
		else
			ctx_data->ac_state = NULL;

		PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, offset);
	}

	if (1 == step) {
		for (i = 0; i < pkt->remain; i++) {
			if ('/' == pkt->pos[i]) {
				step++;
				i++;
				break;
			}
		}

		cpy_len = (2 == step) ? i - 1 : i;

		if ((cpy_len > 0) && (!PSR_GET_BIT(ctx_data->blk_bit_finish,
						   PSR_TYPE_REFER_HOST))) {
			ret = psr_get_block(PSR_TYPE_REFER_HOST, &result,
					    &block);
			if (ret)
				return ret;

			if ((cpy_len + block->len) >= MAX_HOST_LEN)
				cpy_len = MAX_HOST_LEN - block->len;

			if (cpy_len > 0) {
				if (result->base.valid_len + cpy_len >
					result->base.total_len) {
					return PSR_RET_MEM_INVALID;
				}
				memcpy(block->val + block->len, pkt->pos,
				       cpy_len);
			} else if (cpy_len < 0) {
				return PSR_RET_FAILURE;
			}

			block->len  += cpy_len;
			result->base.valid_len += cpy_len;
		}

		if (2 == step)
			PSR_SET_BIT(ctx_data->blk_bit_finish,
				    PSR_TYPE_REFER_HOST);

		PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, cpy_len);
	}

	if (2 == step) {
		PSR_GET_HEADER_END(i, pkt->pos, pkt->remain,
				   ctx_data->state.header.lf_num,
				   ctx_data->state.header.line_end,
				   line_len, cpy_len);

		if ((cpy_len > 0) && (!PSR_GET_BIT(ctx_data->blk_bit_finish,
						   PSR_TYPE_REFER_URI))) {
			ret = psr_get_block(PSR_TYPE_REFER_URI, &result, &block);
			if (ret)
				return ret;

			if ((cpy_len + block->len) >= MAX_REFERER_LEN)
				cpy_len = MAX_REFERER_LEN - block->len;

			if (cpy_len > 0) {
				if (result->base.valid_len + cpy_len >
					result->base.total_len) {
					return PSR_RET_MEM_INVALID;
				}
				memcpy(block->val + block->len,pkt->pos,
				       cpy_len);
			} else if (cpy_len < 0) {
				return PSR_RET_FAILURE;
			}

			block->len  += cpy_len;
			result->base.valid_len += cpy_len;
		}

		if (ctx_data->state.header.line_end) {
			PSR_SET_BIT(ctx_data->blk_bit_finish,
				    PSR_TYPE_REFER_URI);

			ctx_data->valid_len = result->base.valid_len;
		}

		PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, line_len);
	}

	ctx_data->state.header.get_val_step = step;

	return 0;
}

static uint32_t psr_proc_http_content_len(struct psr_msg *msg,
					  struct psr_pkt *pkt,
					  struct psr_ctx_data *ctx_data,
					  struct psr_result *result)
{
	uint32_t i = 0;
	uint8_t chr;

	if (!ctx_data->state.header.data_end) {
		for (i = 0; i < pkt->remain; i++) {
			chr = pkt->pos[i];
			if (IS_NUM(chr)) {
				if (ctx_data->content_len >
				    ((PSR_MAX_UINT32 - 10) / 10)) {
					return PSR_RET_FAILURE;
				}
				ctx_data->content_len *= 10;
				ctx_data->content_len += (chr - '0');
			} else {
				ctx_data->state.header.data_end = true;
				break;
			}
		}

		PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, i);
	}

	for (i = 0; i < pkt->remain; i++) {
		if (LF == pkt->pos[i]) {
			ctx_data->state.header.lf_num++;
			ctx_data->state.header.line_end = true;
			i++;
			break;
		}

		if (CR == pkt->pos[i]) {
			ctx_data->state.header.line_end = true;
			i++;
			break;
		}
	}

	PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, i);
	return 0;
}

static uint32_t psr_proc_http_tran_encoding(struct psr_msg *msg,
					    struct psr_pkt *pkt,
					    struct psr_ctx_data *ctx_data,
					    struct psr_result *result)
{
	uint32_t ret;
	struct psr_match_pattern pattern_tmp = {0};
	struct psr_match_pattern *match_pattern;
	void **cur_state = ctx_data->ac_state;
	uint32_t offset = 0;
	struct psr_ac_pattern *pattern;
	struct psr_ac_node *node;
	uint32_t while_loop = 0;

	while ((offset < pkt->remain) && (!ctx_data->state.header.line_end)) {
		match_pattern = NULL;
		ret = psr_ac_search(&pattern_tmp,
				    msg->mem_pool->ac_info.encoding_ac,
				    pkt->pos, pkt->remain, &cur_state, &offset,
				    &match_pattern);
		if (ret)
			break;

		if (match_pattern) {
			pattern = match_pattern->pattern;
			while (pattern) {
				node = (struct psr_ac_node*)(pattern->ac_node);

				if (while_loop++ > PSR_MAX_LOOP_CNT) {
					PSR_LOG(PSR_RET_FAILURE);
					return PSR_RET_FAILURE;
				}

				if (PSR_HTTP_ENCODING_CHUNKED == node->id) {
					ctx_data->chunked = true;
					PSR_UPDATE_PAYLOAD(pkt->pos,
							   pkt->remain,
							   offset);
					offset = 0;
					break;
				} else if (PSR_HTTP_ENCODING_FL_END ==
					   node->id) {
					ctx_data->state.header.lf_num++;
					ctx_data->state.header.line_end = true;
					ctx_data->ac_state = NULL;
					break;
				}

				pattern = pattern->next;
			}
		}
		else if (while_loop++ > PSR_MAX_LOOP_CNT) {
			PSR_LOG(PSR_RET_FAILURE);
			return PSR_RET_FAILURE;
		}

	}

	PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, offset);

	if (!ctx_data->state.header.line_end)
		ctx_data->ac_state = cur_state;

	return 0;
}

static uint32_t  psr_set_http_pkt_len(struct psr_ctx_data *ctx_data, struct psr_result *result)
{
	uint32_t ret;
	struct psr_http_tlv *block;

	ret = psr_get_block(PSR_TYPE_PKT_LEN, &result, &block);
	if (ret){
		return ret;
	}

	if (result->base.valid_len + sizeof(ctx_data->pkt_len) > result->base.total_len) {
		return PSR_RET_MEM_INVALID;
	}

	memcpy(block->val + block->len, (uint8_t*)&ctx_data->pkt_len, sizeof(ctx_data->pkt_len));

	block->len += sizeof(ctx_data->pkt_len);
	result->base.valid_len += sizeof(ctx_data->pkt_len);
	return 0;
}

static uint32_t  psr_proc_http_header_val(struct psr_msg *msg,
					  struct psr_pkt *pkt,
					  struct psr_ctx_data *ctx_data,
					  struct psr_result *result)
{
	uint32_t ret = PSR_RET_FAILURE;

	PSR_SKIP_BLANK(pkt->pos, pkt->remain);
	if (!pkt->remain)
		return 0;

	if (!(ctx_data->state.header.over_colon)) {
		if (':' == pkt->pos[0]) {
			ctx_data->state.header.over_colon = true;
			PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, 1);
		} else {
			return PSR_RET_FAILURE;
		}
	}

	PSR_SKIP_BLANK(pkt->pos, pkt->remain);
	if (!pkt->remain)
		return 0;

	if (!ctx_data->state.header.line_end) {
		switch (ctx_data->state.header.type) {
		case PSR_TYPE_HOST:
			ret = psr_proc_http_header_str(PSR_TYPE_HOST,
						       MAX_HOST_LEN,
						       msg, pkt,
						       ctx_data, result);
			break;

		case PSR_TYPE_REFER_HOST:
			ret = psr_proc_http_header_ref(msg, pkt, ctx_data,
						       result);
			break;

		case PSR_TYPE_LOCATION:
			ret = psr_proc_http_header_str(PSR_TYPE_LOCATION,
						       MAX_LOCATION_LEN,
						       msg,
						       pkt, ctx_data, result);
			break;

		case PSR_TYPE_CONTENT_TYPE:
			ret = psr_proc_http_header_str(PSR_TYPE_CONTENT_TYPE,
						       MAX_CONTENTTYPE_LEN,
						       msg, pkt, ctx_data,
						       result);
			break;

		case PSR_TYPE_TRAN_ENCODING:
			ret = psr_proc_http_tran_encoding(msg, pkt, ctx_data,
							  result);
			break;

		case PSR_TYPE_CONTENT_LEN:
			ret = psr_proc_http_content_len(msg, pkt, ctx_data,
							result);
			break;

		default:
			break;
		}

		if (ret)
			return ret;
	}

	if (!pkt->remain)
		return 0;

	if (ctx_data->state.header.line_end)
		psr_proc_http_header_val_end(msg, pkt, ctx_data, result);

	return 0;
}

static uint32_t psr_proc_http_body(struct psr_msg *msg, struct psr_pkt *pkt,
				   struct psr_ctx_data *ctx_data,
				   struct psr_result *result)
{
	if (ctx_data->chunked) {
		ctx_data->psr_state = PSR_STATE_CHUNK_SIZE_START;
		return 0;
	}

	if (ctx_data->content_len) {
		ctx_data->psr_state = PSR_STATE_CHK_CONTENT_LEN;
		return 0;
	}

	ctx_data->psr_state = PSR_STATE_DONE;
	return 0;
}

static uint32_t psr_proc_http_chk_size_start(struct psr_msg *msg,
					     struct psr_pkt *pkt,
					     struct psr_ctx_data *ctx_data,
					     struct psr_result *result)
{
	int8_t    hex_val;

	if (pkt->remain > 0) {
		hex_val = psr_hex_tbl[pkt->pos[0]];
		if (UNLIKELY(-1 == hex_val)) {
			return PSR_RET_FAILURE;
		}
		ctx_data->content_len = hex_val;

		pkt->pos++;
		pkt->remain--;
		ctx_data->psr_state = PSR_STATE_CHUNK_SIZE;
	}

	return 0;
}

static uint32_t psr_proc_http_chk_size(struct psr_msg *msg,
				       struct psr_pkt *pkt,
				       struct psr_ctx_data *ctx_data,
				       struct psr_result *result)
{
	uint32_t i;
	int8_t hex_val;
	int8_t chr_val;

	for (i = 0; i < pkt->remain; i++) {
		chr_val = pkt->pos[i];
		hex_val = psr_hex_tbl[(uint8_t)chr_val];
		if (UNLIKELY(-1 == hex_val)) {
			if (CR == chr_val) {
				ctx_data->psr_state = PSR_STATE_CHUNK_SIZE_INTER;
				i++;
			} else if (LF == chr_val) {
				if (!ctx_data->content_len)
					ctx_data->psr_state = PSR_STATE_DONE;
				else
					ctx_data->psr_state =
						PSR_STATE_CHUNK_DATA;
				i++;
			} else if ((';' == chr_val) || (' ' == chr_val)) {
				ctx_data->psr_state = PSR_STATE_CHUNK_PARAMS;
				i++;
			} else {
				return PSR_RET_FAILURE;
			}

			break;
		}

		if (UNLIKELY(((PSR_MAX_UINT16 - 16) >> 1) <
		    ctx_data->content_len)) {
			return PSR_RET_FAILURE;
		}

		ctx_data->content_len = (ctx_data->content_len << 4) + hex_val;
	}

	PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, i);

	return 0;
}

static uint32_t psr_proc_http_chk_param(struct psr_msg *msg, struct psr_pkt *pkt,
					struct psr_ctx_data *ctx_data,
					struct psr_result *result)
{
	if (pkt->remain > 0) {
		if (CR == pkt->pos[0]) {
			ctx_data->psr_state = PSR_STATE_CHUNK_SIZE_INTER;
		} else if (LF == pkt->pos[0]) {
			if (!ctx_data->content_len)
				ctx_data->psr_state = PSR_STATE_DONE;
			else
				ctx_data->psr_state = PSR_STATE_CHUNK_DATA;
		} else {
			return PSR_RET_FAILURE;
		}

		PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, 1);
	}

	return 0;
}

static uint32_t psr_proc_http_chk_size_inter(struct psr_msg *msg,
					     struct psr_pkt *pkt,
					     struct psr_ctx_data *ctx_data,
					     struct psr_result *result)
{
	if (pkt->remain > 0) {
		if (LF == pkt->pos[0]) {
			if (!ctx_data->content_len)
				ctx_data->psr_state = PSR_STATE_DONE;
			else
				ctx_data->psr_state = PSR_STATE_CHUNK_DATA;
		} else {
			return PSR_RET_FAILURE;
		}

		PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, 1);
	}

	return 0;
}

static uint32_t psr_proc_http_chk_data(struct psr_msg *msg, struct psr_pkt *pkt,
				       struct psr_ctx_data *ctx_data,
				       struct psr_result *result)
{
	uint32_t  skip_len;
	skip_len = MIN(ctx_data->content_len, pkt->remain);
	PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, skip_len);

	ctx_data->content_len -= skip_len;
	if (!ctx_data->content_len)
		ctx_data->psr_state = PSR_STATE_CHUNK_DATA_INTER;

	return 0;
}

static uint32_t psr_proc_http_chk_data_inter(struct psr_msg *msg,
					     struct psr_pkt *pkt,
					     struct psr_ctx_data *ctx_data,
					     struct psr_result *result)
{
	if (pkt->remain > 0) {
		if (CR == pkt->pos[0]) {
			ctx_data->psr_state = PSR_STATE_CHUNK_DATA_DONE;
		} else if (LF == pkt->pos[0]) {
			ctx_data->psr_state = PSR_STATE_CHUNK_SIZE_START;
		} else {
			return PSR_RET_FAILURE;
		}

		PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, 1);
	}

	return 0;
}

static uint32_t psr_proc_http_chk_data_done(struct psr_msg *msg,
					    struct psr_pkt *pkt,
					    struct psr_ctx_data *ctx_data,
					    struct psr_result *result)
{
	if (pkt->remain > 0) {
		if (LF == pkt->pos[0]) {
			ctx_data->psr_state = PSR_STATE_CHUNK_SIZE_START;
		} else {
			return PSR_RET_FAILURE;
		}

		PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, 1);
	}

	return 0;
}

static uint32_t psr_proc_http_chk_cont_len(struct psr_msg *msg,
					   struct psr_pkt *pkt,
					   struct psr_ctx_data *ctx_data,
					   struct psr_result *result)
{
	uint32_t  skip_len;

	skip_len = MIN(ctx_data->content_len, pkt->remain);
	PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, skip_len);

	ctx_data->content_len -= skip_len;
	if (!ctx_data->content_len)
		ctx_data->psr_state = PSR_STATE_DONE;

	return 0;
}

static uint32_t psr_proc_http_done(struct psr_msg *msg, struct psr_pkt *pkt,
				   struct psr_ctx_data *ctx_data,
				   struct psr_result *result)
{
	/* TODO: pipeline */
	pkt->remain = 0;
	return 0;
}

static uint32_t psr_proc_http_rsp_code(struct psr_msg *msg, struct psr_pkt *pkt,
				       struct psr_ctx_data *ctx_data,
				       struct psr_result *result)
{
	uint32_t i;
	uint32_t rsp_code = result->un.http.rsp_code;
	uint8_t chr_val;

	if (!ctx_data->state.rsp_code.got) {
		for (i = 0; i < pkt->remain; i++) {
			chr_val = pkt->pos[i];
			if (IS_NUM(chr_val)) {
				rsp_code = rsp_code * 10 + (chr_val - '0');
				if (rsp_code > PSR_MAX_HTTP_RSP_CODE) {
					return PSR_RET_FAILURE;
				}
			} else {
				ctx_data->state.rsp_code.got = true;
				break;
			}
		}

		result->un.http.rsp_code = rsp_code;
		PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, i);
	}

	for (i = 0; i < pkt->remain; i++) {
		if (LF == pkt->pos[i]) {
			ctx_data->state.val = 0;
			ctx_data->ac_state = NULL;
			ctx_data->psr_state = PSR_STATE_HEADER;
			break;
		}
	}

	PSR_UPDATE_PAYLOAD(pkt->pos, pkt->remain, i);

	return 0;
}

static uint32_t psr_ins_http(struct psr_ac_mem *pac_info, uint32_t direction,
			     struct psr_pkt_node *pkt_node,
			     struct psr_ctx_data *ctx_data,
			     struct psr_result *result)
{
	uint32_t ret = 0;
	struct psr_pkt cur_node;
	struct psr_msg msg;
	uint16_t diff_len;
	uint32_t while_loop = 0;

	msg.mem_pool = psr_global_mem;
	msg.ctx_data = ctx_data;
	msg.result = result;
	msg.pkt_node = pkt_node;
	msg.cur_node = &cur_node;
	msg.direction = direction;

	result->base.proto = PSR_HTTP_PROTOCOL;

	/* remove the complete TLVs which have been sent */
	if (ctx_data->valid_len > result->base.valid_len) {
		if (result->base.need_send) {
			diff_len = ctx_data->valid_len - result->base.valid_len;
			memcpy(result->un.http.tlv, result->un.http.tlv +
			       result->base.valid_len, diff_len);

			result->base.valid_len = diff_len + PSR_HTTP_ST_LEN;
			ctx_data->valid_len = diff_len + PSR_HTTP_ST_LEN;
		} else {
			result->base.valid_len = ctx_data->valid_len;
		}
	} else if (result->base.need_send) {
		result->base.valid_len = PSR_HTTP_ST_LEN;
		ctx_data->valid_len = PSR_HTTP_ST_LEN;
	}
	result->base.need_send = 0;

	do {
		if ((!pkt_node->pkt_len) || (!pkt_node->pkt_data)) {
			return PSR_RET_INVALID_PARAM;
		}

		cur_node.pos = pkt_node->pkt_data;
		cur_node.payload = pkt_node->pkt_data;
		cur_node.len = pkt_node->pkt_len;
		cur_node.remain = pkt_node->pkt_len;

		while (cur_node.remain > 0) {
			switch (ctx_data->psr_state) {
			case PSR_STATE_REQ_INIT:
				ctx_data->ac_state = NULL;
				ctx_data->state.val = 0;
				ctx_data->psr_state = PSR_STATE_METHOD;
				result->un.http.version = 0;
				result->base.valid_len = PSR_HTTP_ST_LEN;
				result->un.http.block_bit = 0;
				result->un.http.method = 0;
				break;

			case PSR_STATE_RSP_INIT:
				ctx_data->ac_state = NULL;
				ctx_data->state.val = 0;
				ctx_data->psr_state = PSR_STATE_RSP_HTTP_VERSION;
				result->un.http.rsp_code = 0;
				break;

			case PSR_STATE_METHOD:
				ret = psr_proc_http_method(&msg, &cur_node,
							   ctx_data, result);
				break;

			case PSR_STATE_URI:
				ret = psr_proc_http_uri(&msg, &cur_node,
							ctx_data, result);
				break;

			case PSR_STATE_REQ_HTTP_VERSION:
				ret = psr_proc_http_req_ver(&msg, &cur_node,
							    ctx_data, result);
				break;

			case PSR_STATE_RSP_HTTP_VERSION:
				ret = psr_proc_http_rsp_ver(&msg, &cur_node,
							    ctx_data, result);
				break;

			case PSR_STATE_RSP_CODE:
				ret = psr_proc_http_rsp_code(&msg, &cur_node,
							     ctx_data, result);
				break;

			case PSR_STATE_HEADER:
				ret = psr_proc_http_header(&msg, &cur_node,
							   ctx_data, result);
				break;

			case PSR_STATE_HEADER_VALUE:
				ret = psr_proc_http_header_val(&msg, &cur_node,
							       ctx_data, result);
				break;

			case PSR_STATE_BODY_START:
				ret = psr_proc_http_body(&msg, &cur_node,
							 ctx_data, result);
				break;

			case PSR_STATE_CHUNK_SIZE_START:
				ret = psr_proc_http_chk_size_start(&msg,
								   &cur_node,
								   ctx_data,
								   result);
				break;

			case PSR_STATE_CHUNK_SIZE:
				ret = psr_proc_http_chk_size(&msg, &cur_node,
							     ctx_data, result);
				break;

			case PSR_STATE_CHUNK_PARAMS:
				ret = psr_proc_http_chk_param(&msg, &cur_node,
							      ctx_data, result);
				break;

			case PSR_STATE_CHUNK_SIZE_INTER:
				ret = psr_proc_http_chk_size_inter(&msg,
								   &cur_node,
								   ctx_data,
								   result);
				break;

			case PSR_STATE_CHUNK_DATA:
				ret = psr_proc_http_chk_data(&msg, &cur_node,
							     ctx_data, result);
				break;

			case PSR_STATE_CHUNK_DATA_INTER:
				ret = psr_proc_http_chk_data_inter(&msg,
								   &cur_node,
								   ctx_data,
								   result);
				break;

			case PSR_STATE_CHUNK_DATA_DONE:
				ret = psr_proc_http_chk_data_done(&msg,
								  &cur_node,
								  ctx_data,
								  result);
				break;

			case PSR_STATE_CHK_CONTENT_LEN:
				ret = psr_proc_http_chk_cont_len(&msg,
								 &cur_node,
								 ctx_data,
								 result);
				break;

			case PSR_STATE_DONE:
				ret = psr_proc_http_done(&msg, &cur_node,
							 ctx_data, result);
				break;

			default:
				ctx_data->failed = 1;
				PSR_LOG(PSR_RET_FAILURE);
				return PSR_RET_FAILURE;
			}

			if (ret)
				return ret;

			if ((while_loop++ > PSR_MAX_HTTP_MAIN_LOOP_CNT) ||
				(cur_node.remain > PSR_MAX_LEN)) {
				ctx_data->failed = 1;
				PSR_LOG(PSR_RET_FAILURE);
				return PSR_RET_FAILURE;
			}
		}

		pkt_node = pkt_node->next;
	} while (pkt_node);

	if ((PSR_STATE_BODY_START == ctx_data->psr_state) &&
	    (!ctx_data->content_len) && (!ctx_data->chunked))
		ctx_data->psr_state = PSR_STATE_DONE;

	if ((PSR_STATE_DONE == ctx_data->psr_state) && (PSR_DOWN == direction))
		result->un.http.rsp_finish = 1;

	if (result->base.valid_len > ctx_data->valid_len) {
		ctx_data->valid_len += result->base.valid_len;
		result->base.valid_len = ctx_data->valid_len -
			result->base.valid_len;
		ctx_data->valid_len -= result->base.valid_len;
	}

	return ret;
}

uint8_t psr_proc(uint32_t proto, uint32_t direction, uint64_t ts,
		 uint32_t pkt_len, uint8_t *pkt_data, void *ctx_buf,
		 void *result_buf)
{
	uint32_t ret;
	struct psr_ctx_data *ctx_data;
	struct psr_result *result;
	struct psr_pkt_node pkt_node;
	uint8_t blk_bit_finish;

	if (!psr_global_mem) {
		return 0;
	}

	if ((!pkt_data) || (!ctx_buf) || (!result_buf) ||
		((PSR_UP != direction) && (PSR_DOWN != direction)) ||
		(pkt_len > PSR_MAX_LEN)) {
		PSR_LOG(PSR_RET_INVALID_PARAM);
		return 0;
	}

	ctx_data = (struct psr_ctx_data*)ctx_buf;
	result = (struct psr_result*)result_buf;
	pkt_node.next = NULL;
	pkt_node.pkt_data = pkt_data;
	pkt_node.pkt_len = pkt_len;
	result->base.total_len = PSR_RESULT_LEN;

	if (PSR_L4_PROTO_TCP == proto) {
		if ((ctx_data->mid_pkt_flag) &&
			((PSR_CTX_MAGIC != ctx_data->magic) ||
			(PSR_GET_RESULT_MAGIC(result->base) != result->base.magic))) {
			PSR_LOG(PSR_RET_MEM_INVALID);
			ctx_data->failed = 1;
			return 0;
		}

		result->un.http.direction = direction;
		/* first packet or another direction packet */
		if ((!ctx_data->mid_pkt_flag) ||
		    (direction != ctx_data->cur_direction)) {
			if (ctx_data->continued) {
				ctx_data->continued = 0;
			} else if (PSR_UP == direction) {
				/* ctx_data->failed cleared here */
				memset((uint8_t*)ctx_data, 0,
					sizeof(struct psr_ctx_data));
				ctx_data->valid_len = PSR_HTTP_ST_LEN;
				ctx_data->psr_state = PSR_STATE_REQ_INIT;

				memset((uint8_t*)result, 0, PSR_RESULT_LEN);
				result->base.total_len = PSR_RESULT_LEN;
				result->base.valid_len = PSR_HTTP_ST_LEN;
				result->un.http.direction = direction;
			} else {
				/* error occured in the stream,
				   remained packets should be ignored */
				if (ctx_data->failed)
					return 0;

				blk_bit_finish = ctx_data->blk_bit_finish;
				memset((uint8_t*)ctx_data, 0,
					sizeof(struct psr_ctx_data));
				ctx_data->psr_state = PSR_STATE_RSP_INIT;
				ctx_data->blk_bit_finish = blk_bit_finish;
				ctx_data->valid_len = PSR_HTTP_ST_LEN;
			}
		}

		/* error occured in the stream,
		   remained packets should be ignored */
		if (ctx_data->failed)
			return 0;

		/* first response packet */
		if (result->un.http.req_finish && result->base.need_send) {
			result->un.http.start_time = ts;
			result->un.http.end_time = result->un.http.start_time;
			result->un.http.req_finish = 0;
		/* } else if (direction == PSR_DOWN) { */
		} else if (result->un.http.start_time) {
			result->un.http.end_time = ts;
		}

		ret = psr_ins_http(&(psr_global_mem->ac_info), direction,
				   &pkt_node, ctx_data, result);

		if ((!ctx_data->mid_pkt_flag) ||
		    (direction != ctx_data->cur_direction)) {
			ctx_data->cur_direction = direction;
			ctx_data->mid_pkt_flag = 1;
			ctx_data->magic = PSR_CTX_MAGIC;

			if ((PSR_UP == direction) && (result->un.http.method)) {
				/* first request packet */
				result->un.http.start_time = ts;
				/* result->un.http.end_time = 0; */
				result->un.http.end_time =
					result->un.http.start_time;
			} else if (PSR_DOWN == direction) {
				/* first response packet */
				result->un.http.end_time = ts;
				if (!result->un.http.start_time)
					result->un.http.start_time = ts;

				/* ignore 100 continue packet */
				if (result->un.http.rsp_code != 100) {
					/* stop the parser process
					   if the up packet is invalid
					 */
					if (!result->un.http.method)
						ctx_data->failed = 1;
					else
						result->un.http.req_finish = 1;
				} else {
					ctx_data->continued = 1;
				}
			}
		}

		if (PSR_DOWN == direction){
			ctx_data->pkt_len += pkt_len;
			if (result->un.http.rsp_finish){
				ret |= psr_set_http_pkt_len(ctx_data, result);
			}
		}

		result->base.magic = PSR_GET_RESULT_MAGIC(result->base);

		if (result->un.http.req_finish || result->un.http.rsp_finish)
			result->base.need_send = 1;
	} else {
		memset((uint8_t*)result, 0, PSR_RESULT_LEN);
		result->base.total_len = PSR_RESULT_LEN;

		ret = psr_ins_dns(PSR_L4_PROTO_UDP, direction, ts, &pkt_node,
				  ctx_data, result);
	}

	if (ret) {
		result->base.need_send = 0;
		ctx_data->failed = 1;
		ctx_data->continued = 0;
		return 0;
	}

	if (result->base.need_send) {
		if ((PSR_L4_PROTO_TCP == proto) && (result->un.http.rsp_finish))
			return 2;
		else
			return 1;
	} else {
		return 0;
	}
}
EXPORT_SYMBOL(psr_proc);
