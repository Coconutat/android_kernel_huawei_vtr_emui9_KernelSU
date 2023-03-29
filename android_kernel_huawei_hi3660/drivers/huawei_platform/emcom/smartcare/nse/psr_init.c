

#include <linux/slab.h>
#include <linux/types.h>
#include "huawei_platform/emcom/smartcare/nse/psr_pub.h"
#include "huawei_platform/emcom/smartcare/nse/psr_base.h"
#include "huawei_platform/emcom/smartcare/nse/psr_ac.h"
#include "huawei_platform/emcom/smartcare/nse/psr_init.h"
#include "huawei_platform/emcom/smartcare/nse/psr_char.h"

struct psr_cont_mem *psr_global_cont_mem;
struct psr_mem_pool *psr_global_mem;

#define AC_NODE(_id, _str) {_id, sizeof(_str) - 1, _str}

struct psr_ac_node psr_ac_methods[] = {
	AC_NODE(PSR_METHOD_DELETE,		"DELETE "),
	AC_NODE(PSR_METHOD_GET,			"GET "),
	AC_NODE(PSR_METHOD_HEAD,		"HEAD "),
	AC_NODE(PSR_METHOD_POST,		"POST "),
	AC_NODE(PSR_METHOD_PUT,			"PUT "),
	AC_NODE(PSR_METHOD_CONNECT,		"CONNECT "),
	AC_NODE(PSR_METHOD_OPTIONS,		"OPTIONS "),
	AC_NODE(PSR_METHOD_TRACE,		"TRACE "),
	AC_NODE(PSR_METHOD_COPY,		"COPY "),
	AC_NODE(PSR_METHOD_LOCK,		"LOCK "),
	AC_NODE(PSR_METHOD_MKCOL,		"MKCOL "),
	AC_NODE(PSR_METHOD_MOVE,		"MOVE "),
	AC_NODE(PSR_METHOD_PROPFIND,		"PROPFIND "),
	AC_NODE(PSR_METHOD_PROPPATCH,		"PROPPATCH "),
	AC_NODE(PSR_METHOD_SEARCH,		"SEARCH "),
	AC_NODE(PSR_METHOD_UNLOCK,		"UNLOCK "),
	AC_NODE(PSR_METHOD_BIND,		"BIND "),
	AC_NODE(PSR_METHOD_REBIND,		"REBIND "),
	AC_NODE(PSR_METHOD_UNBIND,		"UNBIND "),
	AC_NODE(PSR_METHOD_ACL,			"ACL "),
	AC_NODE(PSR_METHOD_REPORT,		"REPORT "),
	AC_NODE(PSR_METHOD_MKACTIVITY,		"MKACTIVITY "),
	AC_NODE(PSR_METHOD_CHECKOUT,		"CHECKOUT "),
	AC_NODE(PSR_METHOD_MERGE,		"MERGE "),
	AC_NODE(PSR_METHOD_MSEARCH,		"MSEARCH "),
	AC_NODE(PSR_METHOD_NOTIFY,		"NOTIFY "),
	AC_NODE(PSR_METHOD_SUBSCRIBE,		"SUBSCRIBE "),
	AC_NODE(PSR_METHOD_UNSUBSCRIBE,		"UNSUBSCRIBE "),
	AC_NODE(PSR_METHOD_PATCH,		"PATCH "),
	AC_NODE(PSR_METHOD_PURGE,		"PURGE "),
	AC_NODE(PSR_METHOD_MKCALENDAR,		"MKCALENDAR "),
	AC_NODE(PSR_METHOD_LINK,		"LINK "),
	AC_NODE(PSR_METHOD_UNLINK,		"UNLINK "),
	AC_NODE(PSR_METHOD_BPROPFIND,		"BPROPFIND "),
	AC_NODE(PSR_METHOD_BPROPPATCH,		"BPROPPATCH "),
	AC_NODE(PSR_METHOD_BCOPY,		"BCOPY "),
	AC_NODE(PSR_METHOD_BDELETE,		"BDELETE "),
	AC_NODE(PSR_METHOD_BMOVE,		"BMOVE "),
	AC_NODE(PSR_METHOD_POLL,		"POLL "),
	AC_NODE(PSR_METHOD_ORDERPATCH,		"ORDERPATCH "),
	AC_NODE(PSR_METHOD_UPDATE,		"UPDATE ")
};

struct psr_ac_node psr_ac_headers[] = {
	AC_NODE(PSR_TYPE_HOST,			"Host"),
	AC_NODE(PSR_TYPE_LOCATION,		"Location"),
	AC_NODE(PSR_TYPE_CONTENT_TYPE,		"Content-Type"),
	AC_NODE(PSR_TYPE_REFER_HOST,		"referer"),
	AC_NODE(PSR_TYPE_REFER_HOST,		"referrer"),

	AC_NODE(PSR_TYPE_TRAN_ENCODING,		"Transfer-Encoding"),
	AC_NODE(PSR_TYPE_CONTENT_LEN,		"Content-Length"),

	AC_NODE(PSR_TYPE_HEADER_LINE_END,	"\n"),
	AC_NODE(PSR_TYPE_HEADER_LINE_END,	"\r\n"),
	AC_NODE(PSR_TYPE_HEADER_END,		"\n\n"),
	AC_NODE(PSR_TYPE_HEADER_END,		"\r\n\r\n")
};

struct psr_ac_node psr_ac_refer[] = {
	AC_NODE(PSR_TYPE_REFER_HOST,		"http://")
};

struct psr_ac_node psr_ac_version[] = {
	AC_NODE(PSR_HTTP_VERSION_INNER_FL_END,	"\r\n"),
	AC_NODE(PSR_HTTP_VERSION_INNER_FL_END,	"\n"),
	AC_NODE(PSR_HTTP_VERSION_INNER_OVER,	" "),
	AC_NODE(PSR_HTTP_VERSION_INNER_0_9,	"HTTP/0.9"),
	AC_NODE(PSR_HTTP_VERSION_INNER_1_0,	"HTTP/1.0"),
	AC_NODE(PSR_HTTP_VERSION_INNER_1_1,	"HTTP/1.1")
};

struct psr_ac_node psr_ac_encoding[] = {
	AC_NODE(PSR_HTTP_ENCODING_FL_END,	"\r\n"),
	AC_NODE(PSR_HTTP_ENCODING_FL_END,	"\n"),
	AC_NODE(PSR_HTTP_ENCODING_CHUNKED,	"chunked")
};

static void* psr_alloc_global_mem(uint32_t mem_size)
{
	uint8_t *buf;
	struct psr_cont_mem *mem_buf = psr_global_cont_mem;
	mem_size = ((mem_size + 7) >> 3) << 3;

	if (mem_size > mem_buf->remain)
		return NULL;

	buf = mem_buf->cur;
	mem_buf->cur += mem_size;
	mem_buf->remain -= mem_size;

	return buf;
}

static uint32_t  psr_build_one_ac(struct psr_ac_node *node_list, uint32_t cnt,
				  struct psr_ac **ac_data)
{
	uint32_t ret, i;
	struct psr_ac_pattern *pattern_tmp = NULL;
	struct psr_ac_node *node;
	struct psr_ac_pattern *pattern_list = NULL;
	uint32_t mem_size = sizeof(struct psr_ac_pattern);

	for (i = 0; i < cnt; i++) {
		node = node_list + i;
		pattern_tmp = psr_alloc_global_mem(mem_size);
		if (!pattern_tmp) {
			return PSR_RET_ALLOC_FAILED;
		}
		pattern_tmp->next = NULL;
		pattern_tmp->behavior = PSR_AC_BEHAVIOR_BREAK;
		pattern_tmp->len = node->len;
		pattern_tmp->org_pattern = psr_alloc_global_mem(node->len + 1);
		if (!pattern_tmp->org_pattern) {
			return PSR_RET_ALLOC_FAILED;
		}

		memcpy(pattern_tmp->org_pattern, node->pattern, node->len);

		pattern_tmp->org_pattern[node->len] = '\0';

		pattern_tmp->cap_pattern = psr_alloc_global_mem(node->len + 1);
		if (!pattern_tmp->cap_pattern) {
			return PSR_RET_ALLOC_FAILED;
		}

		psr_str_to_upper(pattern_tmp->cap_pattern, pattern_tmp->len,
				 node->pattern, node->len);

		pattern_tmp->cap_pattern[node->len] = '\0';

		pattern_tmp->ac_node = node;
		pattern_tmp->next = pattern_list;
		pattern_list = pattern_tmp;
	}

	if (pattern_list) {
		ret = psr_ac_compile(psr_alloc_global_mem, false, pattern_list,
				     ac_data);
		if (ret)
			return ret;
	}

	return 0;
}

static uint32_t  psr_build_ac(struct psr_ac_mem  *pac_info)
{
	uint32_t ret;
	uint32_t item_size = sizeof(struct psr_ac_node);

	ret = psr_build_one_ac(psr_ac_methods,
			       sizeof(psr_ac_methods) / item_size,
		    	       &(pac_info->method_ac));
	if (ret)
		return ret;

	ret = psr_build_one_ac(psr_ac_headers,
			       sizeof(psr_ac_headers) / item_size,
			       &(pac_info->header_ac));
	if (ret)
		return ret;

	ret = psr_build_one_ac(psr_ac_refer, sizeof(psr_ac_refer) / item_size,
			       &(pac_info->refer_ac));
	if (ret)
		return ret;

	ret = psr_build_one_ac(psr_ac_version,
			       sizeof(psr_ac_version) / item_size,
			       &(pac_info->version_ac));
	if (ret)
		return ret;

	ret = psr_build_one_ac(psr_ac_encoding,
			       sizeof(psr_ac_encoding) / item_size,
			       &(pac_info->encoding_ac));
	if (ret)
		return ret;

	return 0;
}

static void  psr_ins_deinit(struct psr_mem_pool **pmem_pool)
{
	struct psr_mem_buf_list *mem_node;
	struct psr_mem_buf_list *mem_node_next;

	mem_node = (*pmem_pool)->mem_list;
	while (mem_node)
	{
		mem_node_next = mem_node->next;
		kfree(mem_node);
		mem_node = mem_node_next;
	}

	*pmem_pool = NULL;
}

static int __init psr_init(void)
{
	struct psr_mem_pool *mem_pool;
	uint32_t ret;
	uint32_t total_len;
	struct psr_mem_buf_list *mem_node;
	uint8_t *buf;

	if (psr_global_mem) {
		return PSR_RET_FAILURE;
	}

	buf = (uint8_t*)kmalloc(PSR_BASE_KMALLOC_SIZE, GFP_KERNEL);
	if (!buf) {
		return PSR_RET_ALLOC_FAILED;
	}

	memset(buf, 0, PSR_BASE_KMALLOC_SIZE);

	mem_node = (struct psr_mem_buf_list*)(void*)buf;
	mem_node->next = NULL;
	mem_node->base = buf + sizeof(struct psr_mem_buf_list);
	mem_node->total_len = PSR_BASE_KMALLOC_SIZE -
			      sizeof(struct psr_mem_buf_list);

	total_len = mem_node->total_len;
	buf = mem_node->base;

	mem_pool = (struct psr_mem_pool *)(void*)buf;
	mem_pool->cont_mem.base = buf + sizeof(struct psr_mem_pool);
	mem_pool->cont_mem.cur = buf + sizeof(struct psr_mem_pool);
	mem_pool->cont_mem.total_len = total_len - sizeof(struct psr_mem_pool);
	mem_pool->cont_mem.remain = total_len - sizeof(struct psr_mem_pool);

	mem_pool->mem_list = mem_node;
	psr_global_cont_mem = &(mem_pool->cont_mem);

	ret = psr_build_ac(&(mem_pool->ac_info));
	if (ret) {
		psr_ins_deinit(&mem_pool);
		return ret;
	}

	psr_global_mem = mem_pool;

	return 0;
}
late_initcall(psr_init);

uint32_t psr_deinit(void)
{
	if (!psr_global_mem) {
		return PSR_RET_FAILURE;
	}

	psr_ins_deinit(&psr_global_mem);
	return 0;
}
