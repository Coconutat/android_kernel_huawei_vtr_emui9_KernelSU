#ifndef HIVIEW_SELINUX_H
#define HIVEIW_SELINUX_H
#include <linux/lsm_audit.h>

#define CONFIG_HUAWEI_HIVIEW_SELINUX
// #define CONFIG_HUAWEI_HIVIEW_SELINUX_PERFORMANCE

int hw_hiview_selinux_avc_audit(struct common_audit_data *cad);

#endif
