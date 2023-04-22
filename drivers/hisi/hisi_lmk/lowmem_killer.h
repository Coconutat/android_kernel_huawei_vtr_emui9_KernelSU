#ifndef __HISI_LOWMEM_H
#define __HISI_LOWMEM_H

#ifdef CONFIG_HISI_LOWMEM
int hisi_lowmem_tune(int *other_free, int *other_file,
		     struct shrink_control *sc);
#else
static inline int hisi_lowmem_tune(int *other_free, int *other_file,
		     struct shrink_control *sc)
{
	return 0;
}
#endif

#ifdef CONFIG_HISI_LOWMEM_DBG

void hisi_lowmem_dbg(short oom_score_adj);
void hisi_lowmem_dbg_timeout(struct task_struct *p, struct task_struct *tsk);

#else

static inline void hisi_lowmem_dbg(short oom_score_adj)
{
}

static inline void hisi_lowmem_dbg_timeout(struct task_struct *p,
					   struct task_struct *leader)
{
}

#endif

#endif /* __HISI_LOWMEM_H */
