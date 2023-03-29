#ifndef __ERECOVERY_TRANSTATION_H_
#define __ERECOVERY_TRANSTATION_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ERECOVERY_TOTAL_BUF_SIZE                 8*1024

long erecovery_write_event_user(void __user *argp);
long erecovery_write_event_kernel(void *argp);
long erecovery_read_event(void __user *argp);


#ifdef __cplusplus
}
#endif
#endif
