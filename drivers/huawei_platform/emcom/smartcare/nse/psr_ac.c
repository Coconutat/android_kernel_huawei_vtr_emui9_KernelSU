

#include <linux/types.h>
#include <linux/string.h>
#include "huawei_platform/emcom/smartcare/nse/psr_pub.h"
#include "huawei_platform/emcom/smartcare/nse/psr_base.h"
#include "huawei_platform/emcom/smartcare/nse/psr_ac.h"
#include "huawei_platform/emcom/smartcare/nse/psr_init.h"
#include "huawei_platform/emcom/smartcare/nse/psr_char.h"
#include "huawei_platform/emcom/smartcare/nse/psr_proc.h"

static uint32_t psr_ac_mem_init(psr_malloc_func malloc_func,
				struct psr_ac_pattern *pattern_list,
				struct psr_ac **ac_data)
{
	uint32_t max_state = 1;
	struct psr_ac *ac_data_tmp = NULL;

	*ac_data = NULL;

	ac_data_tmp = malloc_func(sizeof(struct psr_ac));
	if (!ac_data_tmp) {
		return PSR_RET_ALLOC_FAILED;
	}

	ac_data_tmp->pattern = pattern_list;

	while (pattern_list) {
		max_state += pattern_list->len;
		pattern_list = pattern_list->next;
	}

	if ((max_state > PSR_AC_WARN_MAX_STATE) || (1 == max_state)) {
		return PSR_RET_FAILURE;
	}

	ac_data_tmp->max_state_num = (uint16_t)max_state;

	ac_data_tmp->match_list = malloc_func(max_state *
					      sizeof(struct psr_ac_pattern*));
	if (!ac_data_tmp->match_list) {
		return PSR_RET_ALLOC_FAILED;
	}

	ac_data_tmp->next_state = malloc_func(max_state * sizeof(void **));
	if (!ac_data_tmp->next_state) {
		return PSR_RET_ALLOC_FAILED;
	}

	*ac_data = ac_data_tmp;
	return 0;
}

static uint32_t psr_ac_build_charset(struct psr_ac *ac_data, bool case_sensitive)
{
	struct psr_ac_pattern *pattern;
	uint32_t i, sum;
	uint8_t *str;

	for (pattern = ac_data->pattern; pattern; pattern = pattern->next) {
		if (case_sensitive)
			str = pattern->org_pattern;
		else
			str = pattern->cap_pattern;

		for (i = 0; i < pattern->len; i++)
			ac_data->charset[str[i]] = 1;
	}

	sum = 2;
	for (i = 0; i < PSR_CHAR_ALPHA_SIZE; i++) {
		if (1 == ac_data->charset[i]) {
			if (sum > PSR_MAX_UINT8) {
				return PSR_RET_FAILURE;
			}
			ac_data->charset[i] = (uint8_t)sum++;
		} else {
			ac_data->charset[i] = 1;
		}
	}
	ac_data->charset_size = sum;

	return 0;
}

static uint32_t psr_ac_add_pattern(psr_malloc_func malloc_func,
				   bool case_sensitive,
				   struct psr_ac_pattern *pattern_list,
				   struct psr_ac *ac_data)
{
	void **next;
	void **state;
	uint32_t len;
	uint8_t *str;
	void ***next_state;

	next_state = ac_data->next_state;
	state = next_state[PSR_AC_DEFAULT_STATE];

	if (case_sensitive)
		str = pattern_list->org_pattern;
	else
		str = pattern_list->cap_pattern;

	for (len = pattern_list->len; len > 0; str++, len--) {
		next = (void **)PSR_AC_STATE_GOTO(state, *str, ac_data->charset);

		if (!next)
			break;
		state = next;
	}

	for (; len > 0; str++, len--) {
		ac_data->state_num++;
		next = malloc_func(ac_data->charset_size * sizeof(void *));
		if (!next) {
			return PSR_RET_ALLOC_FAILED;
		}

		PSR_AC_STATE_GOTO(state, *str, ac_data->charset) = next;

		next_state[ac_data->state_num] = next;
		state = next;
	}

	pattern_list->next = (struct psr_ac_pattern *)state[0];
	state[0] = (void *)pattern_list;

	return 0;
}

static struct psr_ac_pattern *psr_ac_cpy(psr_malloc_func malloc_func,
					 const struct psr_ac_pattern *pattern)
{
	struct psr_ac_pattern *dest;

	dest = malloc_func(sizeof(struct psr_ac_pattern));
	if (!dest) {
		return NULL;
	}

	memcpy(dest, pattern, sizeof(struct psr_ac_pattern));

	dest->next = NULL;
	return dest;
}

static void psr_ac_queue_init(void ***head, struct psr_ac_queue *queue)
{
	queue->head = head;
	queue->tail = head;
	queue->cnt = 0;
}

static void psr_ac_queue_add_node(struct psr_ac_queue *queue, void **state)
{
	*(queue->head) = state;
	queue->head++;
	queue->cnt++;
}

static void **psr_ac_queue_fetch(struct psr_ac_queue *queue)
{
	void **state = NULL;

	if (queue->cnt) {
		state = *(queue->tail);
		queue->tail++;
		queue->cnt--;
	}

	return state;
}

static uint32_t psr_ac_build_nfa(psr_malloc_func malloc_func,
				 const struct psr_ac *ac_data)
{
	uint16_t chr;
	struct psr_ac_queue queue;
	uint16_t fail_node[PSR_CHAR_ALPHA_SIZE] = {0};
	uint16_t num = 0;
	void **state, **next, **fail, **fail_n;
	struct psr_ac_pattern *pattern, *list;

	psr_ac_queue_init((void ***)(void *)(ac_data->match_list), &queue);

	fail = (state = (ac_data->next_state)[PSR_AC_DEFAULT_STATE]);
	for (chr = 2; chr < ac_data->charset_size; chr++) {
		next = (void **)state[chr];
		if (next) {
			psr_ac_queue_add_node(&queue, next);

			PSR_AC_STATE_FAIL(next) = (void *)fail;
		} else {
			state[chr] = (void *)fail;
		}
	}

	PSR_AC_STATE_FAIL(state) = (void *)fail;

	while (queue.cnt > 0) {
		state = psr_ac_queue_fetch(&queue);
		fail = (void **)PSR_AC_STATE_FAIL(state);

		for (chr = 2; chr < ac_data->charset_size; chr++) {
			next = (void **)state[chr];

			if (!next) {
				fail_node[num++] = chr;
				continue;
			}

			psr_ac_queue_add_node(&queue, next);
			fail_n =(void **)fail[chr];
			next[1] = (void *)fail_n;

			list = (struct psr_ac_pattern *)fail_n[0];
			for (; list; list = list->next) {
				pattern = psr_ac_cpy(malloc_func, list);
				if (!pattern) {
					return PSR_RET_ALLOC_FAILED;
				}

				pattern->next = (struct psr_ac_pattern *)next[0];
				next[0] = (void *)pattern;
			}
		}

		for (chr = 0; chr < num; chr++)
			state[fail_node[chr]] = fail[fail_node[chr]];

		num = 0;
	}

	return 0;
}

static void psr_ac_update_state(const struct psr_ac *ac_data)
{
	uint32_t s;
	void ***state = ac_data->next_state;

	for (s = PSR_AC_DEFAULT_STATE; s <= ac_data->state_num; s++) {
		PSR_AC_STATE_FAIL(state[s]) = state[PSR_AC_DEFAULT_STATE];
		ac_data->match_list[s] = (struct psr_ac_pattern *)state[s][0];
	}
}

uint32_t psr_ac_compile(psr_malloc_func malloc_func, bool case_sensitive,
			struct psr_ac_pattern *pattern_list,
			struct psr_ac **ac_data)
{
	struct psr_ac *ac_data_tmp = NULL;
	uint32_t ret;
	struct psr_ac_pattern *pattern, *next_pattern;
	uint8_t buf[PSR_CHAR_ALPHA_SIZE];
	uint8_t input;

	ret = psr_ac_mem_init(malloc_func, pattern_list, &ac_data_tmp);
	if (ret) {
		return ret;
	}

	ret = psr_ac_build_charset(ac_data_tmp, case_sensitive);
	if (ret) {
		return ret;
	}

	ac_data_tmp->next_state[PSR_AC_DEFAULT_STATE] =
		malloc_func(ac_data_tmp->charset_size * sizeof(void *));
	if (!ac_data_tmp->next_state[PSR_AC_DEFAULT_STATE]) {
		return PSR_RET_ALLOC_FAILED;
	}

	for (pattern = ac_data_tmp->pattern; pattern; pattern = next_pattern) {
		next_pattern = pattern->next;
		ret = psr_ac_add_pattern(malloc_func, case_sensitive,
					 pattern, ac_data_tmp);
		if (ret) {
			return ret;
		}
	}

	ret = psr_ac_build_nfa(malloc_func, ac_data_tmp);
	if (ret) {
		return ret;
	}

	if (!case_sensitive) {
		memcpy(buf, ac_data_tmp->charset, PSR_CHAR_ALPHA_SIZE);

		for (input = 'a'; input <= 'z'; input++)
			ac_data_tmp->charset[input] = buf[input ^ 0x20];
	}

	psr_ac_update_state(ac_data_tmp);

	*ac_data = ac_data_tmp;
	return 0;
}

void psr_str_to_upper(uint8_t *dest, uint32_t dest_len, const char *src,
		      uint32_t src_len)
{
	uint32_t i;

	memcpy(dest, src, src_len);

	for (i = 0; i < src_len; i++)
		PSR_TO_UPPER(dest[i]);
}

uint32_t psr_ac_search(struct psr_match_pattern *pattern_buf,
		       const struct psr_ac *ac_data, uint8_t *text,
		       uint32_t text_len, void ***cur_state, uint32_t *offset,
		       struct psr_match_pattern **pattern)
{
	uint32_t i;
	uint8_t *text_end, *in_str;
	void **state;
	uint8_t const *charset_map  = ac_data->charset;
	struct psr_match_pattern *pattern_list = *pattern;
	struct psr_match_pattern *node;

	node = pattern_buf;
	if (!node) {
		return PSR_RET_FAILURE;
	}

	if (!*cur_state)
		state = ac_data->next_state[0];
	else
		state = *cur_state;

	text_end = text + text_len;
	in_str = text + (*offset);

	if (PSR_MAX_LOOP_CNT < ((uint32_t)(text_end - in_str)))
		return PSR_RET_FAILURE;

	for (; in_str < text_end; in_str++) {
		i = charset_map[in_str[0]];
		state = (void **)state[i];

		if (state[0]) {
			node->pattern = (struct psr_ac_pattern *)state[0];
			node->next = pattern_list;
			node->offset = (uint32_t)((in_str + 1) - text);

			pattern_list = node;

			*offset = (uint32_t)((in_str + 1) - text);
			*cur_state = state;
			*pattern = pattern_list;

			return 0;
		}
	}

	*offset = text_len;
	*cur_state = state;
	*pattern = pattern_list;
	return PSR_RET_FAILURE;
}
