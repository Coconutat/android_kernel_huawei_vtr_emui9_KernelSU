#ifndef __ZRHUNG_WP_SOCHALT_H_
#define __ZRHUNG_WP_SOCHALT_H_

#include <chipset_common/hwzrhung/zrhung.h>

#ifdef __cplusplus
extern "C" {
#endif

int wp_get_sochalt(zrhung_write_event* we);
void get_sr_position_from_fastboot(char *dst, unsigned int max_dst_size);
void zrhung_get_longpress_event(void);

#ifdef __cplusplus
}
#endif
#endif /* __ZRHUNG_WP_SOCHALT_H_ */

