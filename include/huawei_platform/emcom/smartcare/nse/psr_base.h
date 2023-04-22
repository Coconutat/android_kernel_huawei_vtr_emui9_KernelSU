

#ifndef __PSR_BASE_H__
#define __PSR_BASE_H__

#define PSR_HTTP_PROTOCOL	1
#define PSR_DNS_PROTOCOL	2
#define PSR_MAX_RESULT_STR_LEN	PSR_MAX_UINT8
#define PSR_RESULT_LEN		640
#define PSR_CTX_DATA_BLOCK_LEN	24
#define PSR_BASE_KMALLOC_SIZE	(128 * 1024)    /* 128K */

#define MAX_HOST_LEN		64  //host & refererhost
#define MAX_URI_LEN		128
#define MAX_REFERER_LEN		128
#define MAX_LOCATION_LEN	128
#define MAX_CONTENTTYPE_LEN	64

/* error code of PSR module */
enum {
	PSR_RET_BEGIN		= PSR_EN_INVALID,
	PSR_RET_SUCCESS		= 0,			/* successful */
	PSR_RET_FAILURE,					/* failed */
	PSR_RET_INVALID_PARAM,				/* invalid parameter */
	PSR_RET_ALLOC_FAILED	= 39,		/* aloc memory failed */
	PSR_RET_MEM_INVALID,				/* invalid memory */
};

/* HTTP Methods */
enum {
	PSR_METHOD_INVALID	= PSR_EN_INVALID,
	PSR_METHOD_GET		= 1,
	PSR_METHOD_POST		= 2,
	PSR_METHOD_PUT		= 3,
	PSR_METHOD_DELETE	= 4,
	PSR_METHOD_HEAD		= 5,
	PSR_METHOD_CONNECT	= 6,
	PSR_METHOD_TRACE	= 7,
	PSR_METHOD_OPTIONS	= 8,
	PSR_METHOD_PROPFIND	= 9,
	PSR_METHOD_PROPPATCH	= 10,
	PSR_METHOD_MKCOL	= 11,
	PSR_METHOD_COPY		= 12,
	PSR_METHOD_MOVE		= 13,
	PSR_METHOD_LOCK		= 14,
	PSR_METHOD_UNLOCK	= 15,
	PSR_METHOD_PATCH	= 16,
	PSR_METHOD_BPROPFIND	= 17,
	PSR_METHOD_BPROPPATCH	= 18,
	PSR_METHOD_BCOPY	= 19,
	PSR_METHOD_BDELETE	= 20,
	PSR_METHOD_BMOVE	= 21,
	PSR_METHOD_NOTIFY	= 22,
	PSR_METHOD_POLL		= 23,
	PSR_METHOD_SEARCH	= 24,
	PSR_METHOD_SUBSCRIBE	= 25,
	PSR_METHOD_UNSUBSCRIBE	= 26,
	PSR_METHOD_ORDERPATCH	= 27,
	PSR_METHOD_ACL		= 28,
	PSR_METHOD_UPDATE	= 29,
	PSR_METHOD_MERGE	= 30,
	PSR_METHOD_BIND		= 31,
	PSR_METHOD_REBIND	= 32,
	PSR_METHOD_UNBIND	= 33,
	PSR_METHOD_REPORT	= 34,
	PSR_METHOD_MKACTIVITY	= 35,
	PSR_METHOD_CHECKOUT	= 36,
	PSR_METHOD_MSEARCH	= 37,
	PSR_METHOD_PURGE	= 38,
	PSR_METHOD_MKCALENDAR	= 39,
	PSR_METHOD_LINK		= 40,
	PSR_METHOD_UNLINK	= 41,
	PSR_METHOD_END,

	PSR_METHOD_BOTTOM	= PSR_EN_BUTT
};

/* HTTP Location */
enum {
	PSR_URI_TYPE_INVALID	= PSR_EN_INVALID,
	PSR_URI_TYPE_CGI_BIN	= 0,
	PSR_URI_TYPE_7Z		= 1,
	PSR_URI_TYPE_ACE	= 2,
	PSR_URI_TYPE_APK	= 3,
	PSR_URI_TYPE_APP	= 4,
	PSR_URI_TYPE_ARJ	= 5,
	PSR_URI_TYPE_BIN	= 6,
	PSR_URI_TYPE_BZ		= 7,
	PSR_URI_TYPE_BZ2	= 8,
	PSR_URI_TYPE_CAB	= 9,
	PSR_URI_TYPE_CVS	= 10,
	PSR_URI_TYPE_DAT	= 11,
	PSR_URI_TYPE_DEB	= 12,
	PSR_URI_TYPE_DOC	= 13,
	PSR_URI_TYPE_DOCX	= 14,
	PSR_URI_TYPE_DWG	= 15,
	PSR_URI_TYPE_ELF	= 16,
	PSR_URI_TYPE_EXE	= 17,
	PSR_URI_TYPE_GZ		= 18,
	PSR_URI_TYPE_IPA	= 19,
	PSR_URI_TYPE_ISO	= 20,
	PSR_URI_TYPE_JAD	= 21,
	PSR_URI_TYPE_JAR	= 22,
	PSR_URI_TYPE_LZH	= 23,
	PSR_URI_TYPE_MPKG	= 24,
	PSR_URI_TYPE_MRP	= 25,
	PSR_URI_TYPE_PATCH	= 26,
	PSR_URI_TYPE_PDF	= 27,
	PSR_URI_TYPE_PKG	= 28,
	PSR_URI_TYPE_PPT	= 29,
	PSR_URI_TYPE_PPTX	= 30,
	PSR_URI_TYPE_PRC	= 31,
	PSR_URI_TYPE_PXL	= 32,
	PSR_URI_TYPE_RAR	= 33,
	PSR_URI_TYPE_RSC	= 34,
	PSR_URI_TYPE_SIS	= 35,
	PSR_URI_TYPE_SISX	= 36,
	PSR_URI_TYPE_TAR	= 37,
	PSR_URI_TYPE_TGZ	= 38,
	PSR_URI_TYPE_UMD	= 40,
	PSR_URI_TYPE_UND	= 41,
	PSR_URI_TYPE_UUE	= 42,
	PSR_URI_TYPE_WPS	= 43,
	PSR_URI_TYPE_XAP	= 44,
	PSR_URI_TYPE_XLS	= 45,
	PSR_URI_TYPE_XLSX	= 46,
	PSR_URI_TYPE_ZIP	= 47,
	PSR_URI_TYPE_3G2	= 201,
	PSR_URI_TYPE_3GP	= 202,
	PSR_URI_TYPE_ASF	= 203,
	PSR_URI_TYPE_ASFV1	= 204,
	PSR_URI_TYPE_ASX	= 205,
	PSR_URI_TYPE_AVI	= 206,
	PSR_URI_TYPE_DIV	= 207,
	PSR_URI_TYPE_DIVX	= 208,
	PSR_URI_TYPE_F4V	= 209,
	PSR_URI_TYPE_FLV	= 210,
	PSR_URI_TYPE_M3U8	= 211,
	PSR_URI_TYPE_M4V	= 212,
	PSR_URI_TYPE_MKV	= 213,
	PSR_URI_TYPE_MOV	= 214,
	PSR_URI_TYPE_MP4	= 215,
	PSR_URI_TYPE_MPEG	= 216,
	PSR_URI_TYPE_MPG	= 217,
	PSR_URI_TYPE_MVB	= 218,
	PSR_URI_TYPE_OGM	= 219,
	PSR_URI_TYPE_QT		= 220,
	PSR_URI_TYPE_RM		= 222,
	PSR_URI_TYPE_RMVB	= 223,
	PSR_URI_TYPE_RV		= 224,
	PSR_URI_TYPE_SWF	= 225,
	PSR_URI_TYPE_TS		= 226,
	PSR_URI_TYPE_WEBM	= 227,
	PSR_URI_TYPE_WMV	= 228,
	PSR_URI_TYPE_M2TS	= 229,
	PSR_URI_TYPE_MGP	= 230,
	PSR_URI_TYPE_AA3	= 301,
	PSR_URI_TYPE_AAC	= 302,
	PSR_URI_TYPE_AIF	= 303,
	PSR_URI_TYPE_AIFF	= 304,
	PSR_URI_TYPE_AMR	= 305,
	PSR_URI_TYPE_APE	= 306,
	PSR_URI_TYPE_DLS	= 307,
	PSR_URI_TYPE_M4A	= 308,
	PSR_URI_TYPE_M4B	= 309,
	PSR_URI_TYPE_M4P	= 310,
	PSR_URI_TYPE_MID	= 311,
	PSR_URI_TYPE_MIDI	= 312,
	PSR_URI_TYPE_MMF	= 313,
	PSR_URI_TYPE_MP3	= 314,
	PSR_URI_TYPE_MPA	= 315,
	PSR_URI_TYPE_OGG	= 316,
	PSR_URI_TYPE_OMA	= 317,
	PSR_URI_TYPE_OMG	= 318,
	PSR_URI_TYPE_RA		= 319,
	PSR_URI_TYPE_RAM	= 320,
	PSR_URI_TYPE_WAV	= 321,
	PSR_URI_TYPE_WAX	= 322,
	PSR_URI_TYPE_WMA	= 323,
	PSR_URI_TYPE_ASP	= 401,
	PSR_URI_TYPE_BMP	= 402,
	PSR_URI_TYPE_CSS	= 403,
	PSR_URI_TYPE_GIF	= 404,
	PSR_URI_TYPE_HTM	= 405,
	PSR_URI_TYPE_HTML	= 406,
	PSR_URI_TYPE_JPEG	= 407,
	PSR_URI_TYPE_JPG	= 408,
	PSR_URI_TYPE_JS		= 409,
	PSR_URI_TYPE_JSON	= 410,
	PSR_URI_TYPE_JSP	= 411,
	PSR_URI_TYPE_PHP	= 412,
	PSR_URI_TYPE_PNG	= 413,
	PSR_URI_TYPE_XHTML	= 414,
	PSR_URI_TYPE_XML	= 415,
	PSR_URI_TYPE_TXT	= 416,
	PSR_URI_TYPE_URI_END	= 500,
	PSR_URI_TYPE_END,

	PSR_URI_TYPE_BOTTOM	= PSR_EN_BUTT
};

enum {
	PSR_HTTP_VERSION_INVALID	= PSR_EN_INVALID,
	PSR_HTTP_VERSION_0_9		= 1,
	PSR_HTTP_VERSION_1_0		= 2,
	PSR_HTTP_VERSION_1_1		= 3,
	PSR_HTTP_VERSION_OTHER		= 4,
	PSR_HTTP_VERSION_END,

	PSR_HTTP_VERSION_BOTTOM		= PSR_EN_BUTT
};

enum {
	PSR_L4_PROTO_INVALID	= PSR_EN_INVALID,
	PSR_L4_PROTO_TCP	= 0,
	PSR_L4_PROTO_UDP	= 1,
	PSR_L4_PROTO_END,

	PSR_L4_PROTO_BOTTOM	= PSR_EN_BUTT
};

enum {
	PSR_DNS_ITEM_INVALID	= PSR_EN_INVALID,
	PSR_DNS_ITEM_QUERY	= 1,
	PSR_DNS_ITEM_IP_V4	= 2,
	PSR_DNS_ITEM_IP_V6	= 3,
	PSR_DNS_ITEM_END,

	PSR_DNS_ITEM_BOTTOM	= PSR_EN_BUTT
};

struct psr_base_result {
	uint16_t	valid_len;	/* valid length of TLV blocks */
	uint16_t	total_len;	/* total length of memory */
	uint8_t		proto;		/* protocol type */
	uint8_t		need_send;	/* result need to be send */
	uint8_t		magic;		/* magic */
	uint8_t		reserved;	/* for 8 bytes alignment */
};

struct psr_http_tlv {
	uint8_t		type;
	uint8_t		len;
	uint8_t		val[0];
};

struct psr_http_result {
	uint32_t	req_finish:1;
	uint32_t	rsp_finish:1;
	uint32_t	method:6;
	uint32_t	block_bit:8;
	uint32_t	rsp_code:10;
	uint32_t	version:3;
	uint32_t	direction:1;
	uint32_t	reserved:2;
	uint32_t	reserved2;
	uint64_t	start_time;
	uint64_t	end_time;
	uint8_t		tlv[0];
};

struct psr_dns_tlv {
	uint16_t	type;
	uint16_t	len;
	uint8_t		val[0];
};

struct psr_dns_result {
	uint16_t      query_id;
	uint8_t       is_req:1;
	uint8_t       finished:1;
	uint8_t       reserved1:6;
	uint8_t       reserved2;
	uint16_t      query_cnt;
	uint16_t      answer_cnt;
	uint16_t      auth_cnt;
	uint16_t      additional_cnt;
	uint32_t      reserved;
	uint64_t      pkt_time;
	uint8_t       tlv[0];
};

struct psr_result {
	struct psr_base_result		base;
	union {
		struct psr_http_result	http;
		struct psr_dns_result	dns;
	} un;
};

struct psr_state_method {
	uint32_t	scan_len:8;
	uint32_t	got:1;
	uint32_t	reserved:23;
};

struct psr_state_version {
	uint32_t	got:1;
	uint32_t	reserved:31;
};

struct psr_state_header {
	uint32_t	got:1;
	uint32_t	type:4;
	uint32_t	start_cpy_val:1;
	uint32_t	scan_len:5;
	uint32_t	over_colon:1;
	uint32_t	lf_num:2;
	uint32_t	line_end:1;
	uint32_t	data_end:1;
	uint32_t	scan_end_scan_len:2;
	uint32_t	get_val_step:3;
	uint32_t	reserved:11;
};

struct psr_state_rsp_code {
	uint32_t	got:1;
	uint32_t	line_end:1;
	uint32_t	reserved:30;
};

union psr_state {
        uint32_t			val;
        struct psr_state_method		method_state;
        struct psr_state_version	version;
        struct psr_state_header		header;
        struct psr_state_rsp_code	rsp_code;
};

struct psr_ctx_data {
	uint8_t		magic;
	uint8_t		psr_state;
	uint8_t		cur_direction:1;
	uint8_t		chunked:1;
	uint8_t		line_delimiter_len:2;
	uint8_t		mid_pkt_flag:1;	/* 0: first packet; 1: other */
	uint8_t		failed:1;	/* 0: normal; 1: parse failed  */
	uint8_t		continued:1;	/* 0: normal; 1: keep last buffer */
	uint8_t		reserved:1;
	uint8_t		blk_bit_finish;
	uint32_t	content_len;
	union psr_state	state;
	uint16_t	valid_len;
	uint16_t	reserved2;
	void		*ac_state;
	uint32_t	pkt_len;
	uint8_t		reserved1[4];
};

struct psr_pkt_node {
	struct psr_pkt_node	*next;
	uint8_t			*pkt_data;
	uint32_t		pkt_len;
	uint8_t			reserved[4];
};

#endif /* __PSR_BASE_H__ */
