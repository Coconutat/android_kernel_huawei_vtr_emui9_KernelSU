#ifndef __ZRHUNG_CONFIG_H_
#define __ZRHUNG_CONFIG_H_

#include <linux/fs.h>

int hcfgk_set_cfg(struct file *file, void *arg);
int hcfgk_ioctl_get_cfg(struct file *file, void *arg);
int hcfgk_set_cfg_flag(struct file *file, void *arg);
int hcfgk_get_cfg_flag(struct file *file, void *arg);
int hcfgk_set_feature(struct file *file, void *arg);

#endif /* __ZRHUNG_CONFIG_H_ */
