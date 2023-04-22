#ifndef __FAKE_HI6XXX_MBHC_H__
#define __FAKE_HI6XXX_MBHC_H__
#include <linux/debugfs.h>
#include <linux/hisi/hi6xxx/hi6xxx_mbhc.h>

extern void fake_init_headset_debugfs (struct hi6xxx_mbhc_priv *priv);
extern void fake_deinit_dfs_dir(void);
#endif

