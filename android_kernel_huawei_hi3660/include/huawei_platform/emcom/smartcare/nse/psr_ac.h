

#ifndef __PSR_AC_H__
#define __PSR_AC_H__

#define PSR_AC_DEFAULT_STATE    0

#define PSR_AC_WARN_MAX_STATE   (2048)

enum {
	PSR_AC_BEHAVIOR_BEGIN		= PSR_EN_INVALID,

	PSR_AC_BEHAVIOR_BREAK		= 0,
	PSR_AC_BEHAVIOR_CONTINUE	= 1,

	PSR_AC_BEHAVIOR_END,
	PSR_AC_BEHAVIOR_BOTTOM		= PSR_EN_BUTT
};

struct psr_ac_pattern {
	struct psr_ac_pattern	*next;
	uint8_t			*cap_pattern;	/* capital pattern */
	uint8_t			*org_pattern;	/* orginal pattern */
	uint32_t		len;		/* pattern length */
	uint8_t			behavior;	/* PSR_AC_BEHAVIOR_E */
	uint8_t			reserved[3];
	void			*ac_node;	/* psr_ac_node */
};

struct psr_ac {
	struct psr_ac_pattern	*pattern;	/* pattern list */
	uint16_t		max_state_num;
	uint16_t		state_num;
	uint32_t		charset_size;
	uint8_t			charset[256];
	struct psr_ac_pattern	**match_list;
	void			***next_state;
};

struct psr_ac_queue {
	void			***head;
	void			***tail;
	uint32_t		cnt;
	uint8_t			reserved[4];
};

struct psr_match_pattern {
	struct psr_match_pattern	*next;
	struct psr_ac_pattern		*pattern;
	uint32_t			offset;
	uint8_t				reserved[4];
};

#define PSR_AC_STATE_GOTO(state, str, charset) (state)[charset[(str)]]
#define PSR_AC_STATE_FAIL(state) (state)[1]

typedef void* (*psr_malloc_func)(uint32_t  uiMemSize);

extern void psr_str_to_upper(uint8_t *dest, uint32_t dest_len,
			     const char *src, uint32_t src_len);
extern uint32_t psr_ac_compile(psr_malloc_func malloc_func, bool case_sensitive,
			       struct psr_ac_pattern *pattern_list,
			       struct psr_ac **ac_data);
extern uint32_t psr_ac_search(struct psr_match_pattern *pattern_buf,
			      const struct psr_ac *ac_data, uint8_t *text,
			      uint32_t text_len, void ***cur_state,
			      uint32_t *offset,
			      struct psr_match_pattern **pattern);

#endif /* __PSR_AC_H__ */
