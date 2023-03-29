

#ifndef __TASK_H__
#define __TASK_H__

#include "omxvdec.h"
#include "channel.h"

HI_S32 task_create_thread(OMXVDEC_ENTRY* vdec);

HI_S32 task_destroy_thread(OMXVDEC_ENTRY* vdec);

HI_S32 task_cancel_inst(OMXVDEC_CHAN_CTX * pchan);

HI_VOID task_proc_entry(struct seq_file *p, OMXVDEC_TASK *ptask);


#endif

