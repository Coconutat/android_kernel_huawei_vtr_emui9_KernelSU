/*
 * Huawei Touchscreen Driver
 *
 * Copyright (C) 2017 Huawei Device Co.Ltd
 * License terms: GNU General Public License (GPL) version 2
 *
 */

int thp_init_sysfs(struct thp_core_data *cd);
void thp_sysfs_release(struct thp_core_data *cd);
int is_tp_detected(void);
int thp_set_prox_switch_status(bool enable);
bool thp_get_prox_switch_status(void);

