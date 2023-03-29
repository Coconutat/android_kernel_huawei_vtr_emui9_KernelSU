

#ifndef __PSR_INIT_H__
#define __PSR_INIT_H__

/* continuous memory block */
struct psr_cont_mem {
	uint8_t		*base;
	uint8_t		*cur;
	uint32_t	total_len;
	uint32_t	remain;
};

/* memory block list */
struct psr_mem_buf_list {
	struct psr_mem_buf_list	*next;
	uint8_t			*base;
	uint32_t		total_len;
	uint8_t			reserved[4];
	uint8_t			info[0]; /* unused */
};

struct psr_ac_mem {
	struct psr_ac	*method_ac;
	struct psr_ac	*header_ac;
	struct psr_ac	*refer_ac;
	struct psr_ac	*version_ac;
	struct psr_ac	*encoding_ac;
};

struct psr_ac_node {
	uint32_t	id;
	uint32_t	len;
	char		*pattern;
};

#endif/* __PSR_INIT_H__ */
