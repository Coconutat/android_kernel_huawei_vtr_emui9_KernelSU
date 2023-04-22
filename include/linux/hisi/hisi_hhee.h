/*
 * hisi_hhee.h
 *
 * Hisi HHEE function interface.
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __HISI_HHEE_H__
#define __HISI_HHEE_H__
/*
 * HHEE general communication and test driver
 *
 */
#include <linux/arm-smccc.h>

#define HHEE_LKM_UPDATE                 0xC6001082
#define ARM_STD_HVC_VERSION             0x8400ff03
/*
 * Defines for HHEE HVC function ids.
 */
#define HHEE_HVC_VERSION                0xC9000000
#define HHEE_HVC_NOTIFY                 0xC9000001
#define HKIP_HVC_RO_REGISTER            0xC6001030
#define HKIP_HVC_RO_MOD_REGISTER        0xC6001032
#define HKIP_HVC_RO_MOD_UNREGISTER      0xC6001033
#define HKIP_HVC_ROWM_REGISTER          0xC6001040
#define HKIP_HVC_ROWM_SET_BIT           0xC6001050
#define HHEE_HVC_LIVEPATCH              0xC6001088
#define HHEE_HVC_TOKEN                  0xC6001089
#define HHEE_RETURN_STAGE2_INFO         0xC60000fe
/*
 * Defines for el2 register read
 */
#define HHEE_HCR_EL2                    0xC9000002
#define HHEE_VTTBR_EL2                  0xC9000003

/*
 * Defines for security level settings
 */
#define HHEE_PROT_LVL                   0xC9000004
#define HHEE_PERMISIVE                  0xC9000005
#define HHEE_TEXT_BOUNDARIES            0xC9000006
#define HHEE_RET_TO_USER                0xC9000007

/*
 * Defines for logging functionality
 */
#define HHEE_INIT_LOGBUF                0xC9000008
#define HHEE_LOGBUF_INFO                0xC9000009
#define HHEE_CRASHLOG_INFO              0xC900000A
#define HHEE_MONITORLOG_INFO            0xC900000B
#define HHEE_MONITORLOG_RESET_COUNTERS  0xc900000C
#define HHEE_RETURN_FUNCTION_ADDRESS    0xc900000D
#define HHEE_RETURN_VAR_ADDRESS         0xc900000E
#define HHEE_IRQ_INFOBUF                0xC900000F
#define HHEE_MSG_HANDLE_GET             0xC9000010
#define HHEE_PMFBUFLOG_INFO             0xC9000011

#define HHEE_UNKNOWN_HVC                0xC90000ff

/*
 * Defines for psci
 */
#define PSCI_VERSION                    0x84000000
#define PSCI_CPU_SUSPEND_AARCH32        0x84000001
#define PSCI_CPU_SUSPEND_AARCH64        0xc4000001
#define PSCI_CPU_OFF                    0x84000002
#define PSCI_CPU_ON_AARCH32             0x84000003
#define PSCI_CPU_ON_AARCH64             0xc4000003
#define PSCI_AFFINITY_INFO_AARCH32      0x84000004
#define PSCI_AFFINITY_INFO_AARCH64      0xc4000004
#define PSCI_MIG_AARCH32                0x84000005
#define PSCI_MIG_AARCH64                0xc4000005
#define PSCI_MIG_INFO_TYPE              0x84000006
#define PSCI_MIG_INFO_UP_CPU_AARCH32    0x84000007
#define PSCI_MIG_INFO_UP_CPU_AARCH64    0xc4000007
#define PSCI_SYSTEM_OFF                 0x84000008
#define PSCI_SYSTEM_RESET               0x84000009
#define PSCI_FEATURES                   0x8400000A


#define HHEE_ENABLE    1
#define HHEE_DISABLE   0

#define HHEE_MSG_ID_CRASH    0
#define HHEE_MSG_ID_HKIP     1
#define HHEE_MSG_ID_HKIP_TEST    2
#define HHEE_MSG_ID_MAX      8

typedef int (*hhee_callback_t)(unsigned int len, void *buf);

int hhee_msg_init(void);
int hhee_msg_register_handler(unsigned int id, hhee_callback_t fn);
int hhee_check_enable(void);

static inline struct arm_smccc_res hhee_fn_hvc(unsigned long function_id,
			unsigned long arg0, unsigned long arg1,
			unsigned long arg2)
{
	struct arm_smccc_res res;

	arm_smccc_hvc(function_id, arg0, arg1, arg2, 0ul, 0ul, 0ul, 0ul, &res);
	return res;
}

#endif
