#ifndef _COMMSTAT_H
#define _COMMSTAT_H

void inet_save_comm_stat(struct socket *sock, int tx, int len);
int comm_stat_init(void);

#endif	/* _COMMSTAT_H */
