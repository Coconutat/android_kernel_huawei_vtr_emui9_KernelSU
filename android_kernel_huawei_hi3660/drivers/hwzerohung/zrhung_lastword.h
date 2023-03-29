#ifndef __ZRHUNG_LASTWORD_H_
#define __ZRHUNG_LASTWORD_H_

#ifdef __cplusplus
extern "C" {
#endif

int hlastword_init(void);
int hlastword_read(void* buf, uint32_t len);
int hlastword_write(void* buf, uint32_t len);

#ifdef __cplusplus
}
#endif
#endif /* __ZRHUNG_LASTWORD_H_ */
