/*
 * patching addresses
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */

#include <linux/slab.h>
#include "patch_addr.h"
#include "util.h"

int oases_patch_addr_init(struct oases_patch_addr *addr, int count)
{
    int size = 0;

    /* old insn, address + value */
    size += count * (sizeof(void *) + sizeof(u32));
    /* new insn, value */
    size += count * (sizeof(u32));
    /* 1 insn = 1 PLT, address + value */
    size += count * 4 * (sizeof(void *) + sizeof(u32));
    addr->data = kzalloc(size, GFP_KERNEL);
    if (addr->data == NULL) {
        oases_error("could not allocate %d\n", size);
        return -ENOMEM;
    }
    /* carefully setup each field */
    addr->i_key = 0;
    addr->addrs_key = addr->data;
    addr->old_insns_key = (u32 *)(addr->addrs_key + count);
    addr->new_insns_key = (u32 *)(addr->old_insns_key + count);
    addr->i_plt = 0;
    addr->addrs_plt = (void **)(addr->new_insns_key + count);
    addr->new_insns_plt = (u32 *)(addr->addrs_plt + count * 4);
    return 0;
}

void oases_patch_addr_free(struct oases_patch_addr *addr)
{
    if (addr->data) {
        kfree(addr->data);
        addr->data = NULL;
    }
}
