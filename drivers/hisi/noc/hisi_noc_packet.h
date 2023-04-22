/*
* NoC. (NoC Mntn Module.)
*
* Copyright (c) 2016 Huawei Technologies CO., Ltd.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#ifndef __HISI_NOC_PACKET
#define __HISI_NOC_PACKET

#include "hisi_noc.h"

#define PACKET_MAINCTL                 (0x0008)/*(0x2008-0x2000)(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_MAINCTL_ADDR(0)-PACKET_BASE)*/
#define PACKET_CFGCTL                  (0x000c)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_CFGCTL_ADDR(0)-PACKET_BASE)*/
#define PACKET_FILTERLUT               (0x0014)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_FILTERLUT_ADDR(0)-PACKET_BASE)*/
#define PACKET_STATPERIOD              (0x0024)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_STATPERIOD_ADDR(0)-PACKET_BASE)*/
#define PACKET_STATALARMMAX            (0x0030)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_STATALARMMAX_ADDR(0)-PACKET_BASE)*/
#define PACKET_STATALARMCLR            (0x0038)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_STATALARMCLR_ADDR(0)-PACKET_BASE)*/

#define PACKET_F0_ROUTEIDBASE          (0x0044)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_F0_ROUTEIDBASE_ADDR(0)-PACKET_BASE)*/
#define PACKET_F0_ROUTEIDMASK          (0x0048)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_F0_ROUTEIDMASK_ADDR(0)-PACKET_BASE)*/
#define PACKET_F0_ADDRBASE             (0x004c)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_F0_ADDRBASE_ADDR(0)-PACKET_BASE)*/
#define PACKET_F0_WINDOWSIZE           (0x0054)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_F0_WINDOWSIZE_ADDR(0)-PACKET_BASE)*/
#define PACKET_F0_SECURITYBASE         (0x0058)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_F0_SECURITYBASE_ADDR(0)-PACKET_BASE)*/
#define PACKET_F0_SECURITYMASK         (0x005c)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_F0_SECURITYMASK_ADDR(0)-PACKET_BASE)*/
#define PACKET_F0_OPCODE               (0x0060)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_F0_OPCODE_ADDR(0)-PACKET_BASE)*/
#define PACKET_F0_STATUS               (0x0064)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_F0_STATUS_ADDR(0)-PACKET_BASE)*/
#define PACKET_F0_LENGTH               (0x0068)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_F0_LENGTH_ADDR(0)-PACKET_BASE)*/
#define PACKET_F0_URGENCY              (0x006c)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_F0_URGENCY_ADDR(0)-PACKET_BASE)*/
#define PACKET_F0_USERBASE             (0x0070)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_F0_USERBASE_ADDR(0)-PACKET_BASE)*/
#define PACKET_F0_USERMASK             (0x0074)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_F0_USERMASK_ADDR(0)-PACKET_BASE)*/

#define PACKET_F1_ROUTEIDBASE          (0x0080)
#define PACKET_F1_ROUTEIDMASK          (0x0084)
#define PACKET_F1_ADDRBASE             (0x0088)
#define PACKET_F1_WINDOWSIZE           (0x0090)
#define PACKET_F1_SECURITYMASK         (0x0098)
#define PACKET_F1_OPCODE               (0x009C)
#define PACKET_F1_STATUS               (0x00A0)
#define PACKET_F1_LENGTH               (0x00A4)
#define PACKET_F1_URGENCY              (0x00A8)
#define PACKET_F1_USERMASK             (0x00B0)

#define PACKET_COUNTERS_0_SRC          (0x0138)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_COUNTERS_0_SRC_ADDR(0)-PACKET_BASE)*/
#define PACKET_COUNTERS_0_ALARMMODE    (0x013c)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_COUNTERS_0_ALARMMODE_ADDR(0)-PACKET_BASE)*/
#define PACKET_COUNTERS_0_VAL          (0x0140)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_COUNTERS_0_VAL_ADDR(0)-PACKET_BASE)*/

#define PACKET_COUNTERS_1_SRC          (0x014c)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_COUNTERS_1_SRC_ADDR(0)-PACKET_BASE)*/
#define PACKET_COUNTERS_1_ALARMMODE    (0x0150)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_COUNTERS_1_ALARMMODE_ADDR(0)-PACKET_BASE)*/
#define PACKET_COUNTERS_1_VAL          (0x0154)/*(SOC_CFG_SYS_NOC_BUS_SYSBUS_DDRC_PACKET_COUNTERS_1_VAL_ADDR(0)-PACKET_BASE)*/

#define PACKET_COUNTERS_2_SRC          (0x0160)
#define PACKET_COUNTERS_2_ALARMMODE    (0x0164)
#define PACKET_COUNTERS_2_VAL          (0x0168)

#define PACKET_COUNTERS_3_SRC          (0x0174)
#define PACKET_COUNTERS_3_ALARMMODE    (0x0178)
#define PACKET_COUNTERS_3_VAL          (0x017c)

#define PACKET_STARTGO                 (0x0028)

void enable_packet_probe_by_name(const char *name);
void disable_packet_probe_by_name(const char *name);
void config_packet_probe(const char *name, const struct packet_configration *packet_cfg);

void noc_packet_probe_hanlder(const struct noc_node *node, void __iomem *base);
void enable_packet_probe(const struct noc_node *node, void __iomem *base);
void disable_packet_probe(void __iomem *base);
void noc_set_bit(void __iomem *base, unsigned int offset, unsigned int bit);
void noc_clear_bit(void __iomem *base, unsigned int offset, unsigned int bit);
void init_packet_info(struct noc_node *node);

#endif
