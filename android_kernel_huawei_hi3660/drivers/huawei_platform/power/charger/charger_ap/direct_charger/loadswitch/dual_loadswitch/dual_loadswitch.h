#ifndef _DUAL_LOADSWITCH_H_
#define _DUAL_LOADSWITCH_H_

#include <huawei_platform/power/direct_charger.h>

struct dual_loadswitch_info {
	struct platform_device *pdev;
	struct device *dev;
};

int loadswitch_main_ops_register(struct loadswitch_ops *);
int loadswitch_aux_ops_register(struct loadswitch_ops *);

#endif /* end of _DUAL_LOADSWITCH_H_ */