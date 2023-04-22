#ifndef _PROXY_HCD_DEBUGFS_H_
#define _PROXY_HCD_DEBUGFS_H_

#include "proxy-hcd.h"

#ifdef CONFIG_HISI_DEBUG_FS
int phcd_debugfs_init(struct proxy_hcd *phcd);
void phcd_debugfs_exit(struct proxy_hcd *phcd);
#else
static inline int phcd_debugfs_init(struct proxy_hcd *phcd){return 0;}
static inline void phcd_debugfs_exit(struct proxy_hcd *phcd){}
#endif /* CONFIG_HISI_DEBUG_FS */

#endif
