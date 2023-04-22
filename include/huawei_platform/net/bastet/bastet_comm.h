/*
 * Copyright (c) Huawei Technologies Co., Ltd. 1998-2014. All rights reserved.
 *
 * File name: bastet_dev.h
 * Description: Provide kernel device information for bastet.
 * Author: Pengyu  ID: 00188486
 * Version: 0.1
 * Date: 2014/06/21
 *
 */

#ifndef _BASTET_COMM_H
#define _BASTET_COMM_H

#include "bastet_dev.h"

#ifdef CONFIG_HW_CROSSLAYER_OPT
#include <net/tcp_crosslayer.h>
#endif


#define BST_MODEM_IOC_MAGIC					'j'

#define BST_MODEM_IOC_GET_MODEM_RAB_ID			_IOWR(BST_MODEM_IOC_MAGIC, 1, struct bastet_modem_rab_id)
#define BST_MODEM_IOC_GET_MODEM_RESET			_IOWR(BST_MODEM_IOC_MAGIC, 2, int32_t)

#ifdef CONFIG_HW_CROSSLAYER_OPT
#define BST_ASPEN_INFO_PKT_DROP             0xc0000001
#define BST_ASPEN_PKT_DROP_SIZE             128

typedef struct
{
    uint32_t                          ulAspenInfoType;
    uint32_t                          ulPktNum;
    struct aspen_cdn_info             stPkt[BST_ASPEN_PKT_DROP_SIZE];
} bst_aspen_pkt_drop;
#endif

struct bastet_modem_rab_id {
    uint16_t modem_id;
    uint16_t rab_id;
};

enum BST_ACORE_CORE_MSG_TYPE_ENUM
{
    BST_ACORE_CORE_MSG_TYPE_DSPP,
    BST_ACORE_CORE_MSG_TYPE_ASPEN,
    BST_ACORE_CORE_MSG_TYPE_EMCOM_SUPPORT,
    BST_ACORE_CORE_MSG_TYPE_EMCOM_KEY_PSINFO,
    BST_ACORE_CORE_MSG_TYPE_RESET_INFO,
    BST_ACORE_CORE_MSG_TYPE_BUTT
};
typedef uint32_t BST_ACORE_CORE_MSG_TYPE_ENUM_UINT32;

typedef struct
{
    BST_ACORE_CORE_MSG_TYPE_ENUM_UINT32 enMsgType;
    uint8_t                             aucValue[0];
} bst_common_msg;

typedef struct
{
    BST_ACORE_CORE_MSG_TYPE_ENUM_UINT32 enMsgType;
    uint32_t                            ulLen;
    uint8_t                             aucValue[4];
} bst_acom_msg;

typedef struct
{
    BST_ACORE_CORE_MSG_TYPE_ENUM_UINT32 enMsgType;
    uint8_t                             enState;
    uint8_t                             aucreverse[3];
} bst_emcom_support_msg;

typedef struct
{
    BST_ACORE_CORE_MSG_TYPE_ENUM_UINT32 enMsgType;
    uint32_t                            enState;
} bst_key_psinfo;

#ifdef CONFIG_HUAWEI_BASTET_COMM
union bst_rab_id_ioc_arg
{
	char in[IFNAMSIZ];
	struct bst_modem_rab_id out;
};
#endif

void reg_ccore_reset_notify(void);
void unreg_ccore_reset_notify(void);

#endif /* _BASTET_COMM_H */
