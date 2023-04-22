#ifndef ITRUSTEE_H_INCLUDED
#define ITRUSTEE_H_INCLUDED

void wakeup_tc_siq(void);
bool is_tee_hungtask(struct task_struct *t);

#endif
