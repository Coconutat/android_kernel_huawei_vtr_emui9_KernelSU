/*
 *hisi_mm.h
 *memory mapping related head file
 *Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 */
#ifndef HISI_MM_H
#define HISI_MM_H

void create_mapping_late(phys_addr_t phys, unsigned long virt,
                                phys_addr_t size, pgprot_t prot);

#endif
