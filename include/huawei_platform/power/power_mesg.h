#ifndef POWER_MESG
#define POWER_MESG

#include <linux/device.h>
#include <linux/power/hisi/hisi_bci_battery.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/power_mesg_type.h>
#include <huawei_platform/power/power_genl.h>

int board_runtime_register(int rt_step, int free_rt, struct notifier_block *nb);
int power_easy_send(power_mesg_node_t *node, unsigned char cmd,
                    unsigned char version, void *data, unsigned len);
int power_easy_node_register(power_mesg_node_t *node);

#endif