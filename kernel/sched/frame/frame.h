#ifndef __FRAME_EXTERN_H
#define __FRAME_EXTERN_H

int set_frame_rate(int rate);
int set_frame_margin(int margin);
int set_frame_status(unsigned long status);

void update_frame_info_tick(struct related_thread_group *grp);

#ifdef CONFIG_TRACE_FRAME_SYSTRACE
#define FRAME_SYSTRACE	       trace_clock_set_rate
#else
#define FRAME_SYSTRACE(format, ...)
#endif /*  CONFIG_TRACE_FRAME_SYSTRACE	*/

#endif
