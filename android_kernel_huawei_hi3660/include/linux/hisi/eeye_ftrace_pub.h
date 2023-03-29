
#ifndef _EEYE_FTRACE_PUB_H
#define	_EEYE_FTRACE_PUB_H


#ifdef CONFIG_HISI_EAGLE_EYE
extern void flush_ftrace_buffer_cache(void);
extern int save_eeye_ftrace_buffer(int reason);
extern void eeye_set_fb_reason(int reason);
#else
static inline void flush_ftrace_buffer_cache(void) { return; }
static inline int save_eeye_ftrace_buffer(int reason) { return 0; }
static inline void eeye_set_fb_reason(int reason) { return; }
#endif

#endif /*_EEYE_FTRACE_PUB_H*/

