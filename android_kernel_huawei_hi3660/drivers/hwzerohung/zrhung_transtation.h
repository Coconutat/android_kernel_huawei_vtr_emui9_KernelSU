#ifndef __ZRHUNG_TRANSTATION_H_
#define __ZRHUNG_TRANSTATION_H_

#ifdef __cplusplus
extern "C" {
#endif

#define HTRANS_TOTAL_BUF_SIZE 				64*1024

long htrans_write_event(void __user *argp);
long htrans_write_event_kernel(void *argp);
long htrans_read_event(void __user *argp);
long htrans_read_lastword(void __user *argp);

#ifdef __cplusplus
}
#endif
#endif /* __ZRHUNG_TRANSTATION_H_ */
