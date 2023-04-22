

#ifndef __PSR_CHAR_H__
#define __PSR_CHAR_H__

#define PSR_CHAR_ALPHA_SIZE		256

#define PSR_TO_UPPER(_char)				\
	do {						\
		if ((_char >= 'a') && (_char <= 'z'))	\
			_char -= 0x20;			\
	} while(0)

#define PSR_MAX_METHOD_SCAN_LEN PSR_MAX_UINT8
#define PSR_MAX_HEADER_LEN 31

#define PSR_SET_BIT(_val, _type) ((_val) |= (0x01 << (_type - 1)))
#define PSR_GET_BIT(_val, _type) ((_val) & (0x01 << (_type - 1)))

enum {
	PSR_TYPE_INVALID		= PSR_EN_INVALID,

	PSR_TYPE_HOST			= 1,
	PSR_TYPE_URI			= 2,
	PSR_TYPE_REFER_HOST		= 3,
	PSR_TYPE_REFER_URI		= 4,
	PSR_TYPE_CONTENT_TYPE		= 5,
	PSR_TYPE_LOCATION		= 6,

	PSR_TYPE_PKT_LEN		= 7,
	PSR_TYPE_TRAN_ENCODING,
	PSR_TYPE_METHOD,
	PSR_TYPE_CONTENT_LEN,
	PSR_TYPE_HEADER_VALUE_START,
	PSR_TYPE_HEADER_LINE_END,
	PSR_TYPE_HEADER_END,

	PSR_TYPE_END,
	PSR_TYPE_BOTTOM			= PSR_EN_BUTT
};

enum {
	PSR_HTTP_VERSION_INNER_INVALID	= PSR_EN_INVALID,
	PSR_HTTP_VERSION_INNER_FL_END	= 0,
	PSR_HTTP_VERSION_INNER_0_9	= PSR_HTTP_VERSION_0_9,
	PSR_HTTP_VERSION_INNER_1_0	= PSR_HTTP_VERSION_1_0,
	PSR_HTTP_VERSION_INNER_1_1	= PSR_HTTP_VERSION_1_1,
	PSR_HTTP_VERSION_INNER_OTHER	= PSR_HTTP_VERSION_OTHER,
	PSR_HTTP_VERSION_INNER_OVER	= 5,
	PSR_HTTP_VERSION_INNER_END,

	PSR_HTTP_VERSION_INNERBOTTOM	= PSR_EN_BUTT
};

enum {
	PSR_HTTP_ENCODING_INVALID	= PSR_EN_INVALID,
	PSR_HTTP_ENCODING_FL_END	= 0,
	PSR_HTTP_ENCODING_CHUNKED	= 1,
	PSR_HTTP_ENCODING_END,

	PSR_HTTP_ENCODING_BOTTOM	= PSR_EN_BUTT
};

struct psr_mem_pool {
	struct psr_ac_mem	ac_info;
	struct psr_cont_mem	cont_mem;
	struct psr_mem_buf_list	*mem_list;
};

struct psr_pkt {
	uint8_t		*payload;
	uint8_t		*pos;
	uint32_t	len;
	uint32_t	remain;
};

struct psr_msg {
	struct psr_mem_pool	*mem_pool;
	struct psr_ctx_data	*ctx_data;
	struct psr_result	*result;
	struct psr_pkt_node	*pkt_node;
	struct psr_pkt		*cur_node;
	uint32_t		direction;
};


/* Parser state of HTTP */
enum {
	PSR_STATE_REQ_INIT			= 0,
	PSR_STATE_RSP_INIT			= 1,
	PSR_STATE_METHOD,
	PSR_STATE_URI,
	PSR_STATE_REQ_HTTP_VERSION,
	PSR_STATE_RSP_HTTP_VERSION,
	PSR_STATE_RSP_CODE,
	PSR_STATE_HEADER,
	PSR_STATE_HEADER_VALUE,
	PSR_STATE_BODY_START,
	PSR_STATE_CHUNK_SIZE_START,
	PSR_STATE_CHUNK_SIZE,
	PSR_STATE_CHUNK_PARAMS,
	PSR_STATE_CHUNK_SIZE_INTER,
	PSR_STATE_CHUNK_DATA,
	PSR_STATE_CHUNK_DATA_INTER,
	PSR_STATE_CHUNK_DATA_DONE,
	PSR_STATE_CHK_CONTENT_LEN,
	PSR_STATE_DONE
};

#define CR                  '\r'
#define LF                  '\n'
#define IS_NUM(c)           ((c) >= '0' && (c) <= '9')

#endif /* __PSR_CHAR_H__ */
