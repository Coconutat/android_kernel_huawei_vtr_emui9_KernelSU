#ifndef ONEWIRE_PHY_PL061_GPIO_H
#define ONEWIRE_PHY_PL061_GPIO_H

#include <linux/delay.h>
#include <linux/timex.h>
#include <asm/arch_timer.h>
#include <hisi_gpio.h>
#include "onewire_phy_common.h"

/* PL061 */
#define PL061_DIR_OFFSET                    0x400
#define PL061_DATA_OFFSET                   2

/* device tree bind */
#define GPIO_CHIP_PHANDLE_INDEX             0
#define GPIO_REG_PROPERTY_SIZE              4
#define ONEWIRE_GPIO_OFFSET_INDEX           1
#define ADDRESS_HIGH32BIT                   0
#define ADDRESS_LOW32BIT                    1
#define LENGTH_HIGH32BIT                    2
#define LENGTH_LOW32BIT                     3
#define SHIFT_32                            32
#define GPIO_INDEX                          0

#endif
