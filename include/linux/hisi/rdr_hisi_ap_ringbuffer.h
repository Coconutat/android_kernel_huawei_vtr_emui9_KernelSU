/*
 * record the data to rdr. (RDR: kernel run data recorder.)
 * This file wraps the ring buffer.
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <mntn_public_interface.h>

#ifdef CONFIG_HISI_BB
int hisiap_ringbuffer_init(struct hisiap_ringbuffer_s *q, u32 bytes,
			   u32 fieldcnt, const char *keys);
void hisiap_ringbuffer_write(struct hisiap_ringbuffer_s *q, u8 *element);
int  hisiap_ringbuffer_read(struct hisiap_ringbuffer_s *q, u8 *element, u32 len);
int  hisiap_is_ringbuffer_full(const void *buffer_addr);
void get_ringbuffer_start_end(struct hisiap_ringbuffer_s *q, u32 *start, u32 *end);
bool is_ringbuffer_empty(struct hisiap_ringbuffer_s *q);
bool is_ringbuffer_invalid(u32 field_count, u32 len, struct hisiap_ringbuffer_s *q);
#else
static inline int hisiap_ringbuffer_init(struct hisiap_ringbuffer_s *q, u32 bytes,
			   u32 fieldcnt, const char *keys){return 0;}
static inline void hisiap_ringbuffer_write(struct hisiap_ringbuffer_s *q, u8 *element){ return;}
static inline int  hisiap_ringbuffer_read(struct hisiap_ringbuffer_s *q, u8 *element, u32 len){return 0;}
static inline int  hisiap_is_ringbuffer_full(const void *buffer_addr){return 0;}
static inline void get_ringbuffer_start_end(struct hisiap_ringbuffer_s *q, u32 *start, u32 *end){return;}
static inline bool is_ringbuffer_empty(struct hisiap_ringbuffer_s *q){return true;}
static inline bool is_ringbuffer_invalid(u32 field_count, u32 len, struct hisiap_ringbuffer_s *q){return true;}
#endif
