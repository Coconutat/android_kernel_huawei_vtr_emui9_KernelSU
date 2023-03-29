

#ifndef __PSR_PROC_H__
#define __PSR_PROC_H__

#define PSR_GET_HEADER_END(_i, _pos, _left, _lf_num, _line_end, _len, _cpy_len)	\
	do {								\
		for ((_i) = 0; (_i) < (_left); (_i)++) {		\
			if (LF == (_pos)[(_i)]) {			\
				(_lf_num)++;				\
				(_line_end) = true;			\
				break;					\
			}						\
			if (CR == (_pos)[(_i)]) {			\
				(_line_end) = true;			\
				break;					\
			}						\
		}							\
		(_len) = (_line_end) ? (_i) + 1 : (_i);			\
		(_cpy_len) = (_i);					\
		if (((_cpy_len) < (_left)) && (_cpy_len)) {		\
			for ((_i) = (_cpy_len); (_i) != 0; (_i)--)	\
			{						\
				if ((' ' != (_pos)[(_i) - 1])		\
					&& ('\t' != (_pos)[(_i) - 1]))	\
					break;				\
			}						\
			(_cpy_len) = (_i);				\
		}							\
	} while(0)

#define PSR_UPDATE_PAYLOAD(_payload, _leftlen, _size)	\
	do {						\
		(_payload) += (_size);			\
		(_leftlen) -= (_size);			\
	} while(0)

#define PSR_SKIP_BLANK(_pos, _remain)					\
	do {								\
		while ((_remain) > 0) {					\
			if ((' ' == (_pos)[0])				\
				|| ('\t' == (_pos)[0]))			\
				PSR_UPDATE_PAYLOAD((_pos), (_remain), 1); \
			else						\
				break;					\
		}							\
	} while (0)

#ifdef __GNUC__
# define LIKELY(X) __builtin_expect(!!(X), 1)
# define UNLIKELY(X) __builtin_expect(!!(X), 0)
#else
# define LIKELY(X) (X)
# define UNLIKELY(X) (X)
#endif

#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define PSR_MAX_LEN			65535
#define PSR_CTX_MAGIC		0x5A
#define PSR_RESULT_MAGIC	0xA5
#define PSR_MAX_LOOP_CNT	65535
#define PSR_MAX_HTTP_MAIN_LOOP_CNT	100

#define PSR_UP    0
#define PSR_DOWN  1
#define PSR_HTTP_ST_LEN (sizeof(struct psr_base_result) +	\
			sizeof(struct psr_http_result))

#define PSR_US2UC_MAGIC(_us) (((_us) & 0xFF) ^ (((_us) >> 8) & 0xFF))

#define PSR_GET_RESULT_MAGIC(_base) (PSR_US2UC_MAGIC((_base).valid_len) ^ \
			PSR_US2UC_MAGIC((_base).total_len) ^ \
			(_base).proto ^ PSR_RESULT_MAGIC)

extern struct psr_mem_pool *psr_global_mem;

#endif/* __PSR_PROC_H__ */
