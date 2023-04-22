/*
 * Hisilicon IPP Head File
 *
 * Copyright (c) 2018 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LINUX_HISI_HIPP_H_
#define _LINUX_HISI_HIPP_H_

#include <linux/scatterlist.h>

enum hipp_secmode_type_e {
    NONTRUS = 0,
    TRUSTED = 1,
    PROTECT = 2,
    MAX_SECMODE
};

enum hipp_clocklevel_type_e {
    CLKLVL_TURBO    = 0,
    CLKLVL_NORMINAL = 1,
    CLKLVL_SVS      = 2,
    CLKLVL_DISCLOCK = 3,
    MAX_CLOCK_LEVEL
};

enum hipp_com_type_e {
    HISI_CPE_UNIT   = 0,
    HISI_ORB_UNIT   = 1,
    HISI_JPEGE_UNIT = 2,
    HISI_JPEGD_UNIT = 3,
    HISI_FD_UNIT    = 4,
    HISI_HIFD_UNIT  = 5,
    MAX_IPP_COM
};

struct hipp_commntn_s {
    unsigned long long ifirst_ts;   /* Driver TimeStamp for First Time in */
    unsigned long long xfirst_ts;   /* Driver TimeStamp for First Time x */
    unsigned long long ilast_ts;    /* Driver TimeStamp for Last Time in */
    unsigned long long xlast_ts;    /* Driver TimeStamp for Last Time x */
    unsigned int icount;            /* in Counting */
    unsigned int xcount;            /* x Counting */ 
};

struct hipp_common_s {
    int initialized;                                    /* Driver Initialize */
    unsigned int type;                                  /* hipp_smmu_type_e */
    unsigned int mode;                                  /* Security Attribution */
    const char *name;                                   /* Name */
    int (*enable_smmu)(struct hipp_common_s *drv);      /* CPE/ORB/JPEGen/JPEGde/FD Smmu Enable Handler */
    int (*disable_smmu)(struct hipp_common_s *drv);     /* CPE/ORB/JPEGen/JPEGde/FD Smmu Disable Handler */
    int (*setsid_smmu)(struct hipp_common_s *drv,       /* CPE/ORB/JPEGen/JPEGde/FD Smmu Set SID Handler */
                        unsigned int sid, unsigned int mode);
    int (*lock_usecase)(struct hipp_common_s *drv);     /* CPE/ORB/JPEGen/JPEGde/FD Usecase Lock Handler */
    int (*unlock_usecase)(struct hipp_common_s *drv);   /* CPE/ORB/JPEGen/JPEGde/FD Usecase unLock Handler */
    int (*enable_jpgclk)(struct hipp_common_s *drv, unsigned int dvfs);    /* CPE/ORB/JPEGen/JPEGde/FD Clock Enable Handler */
    int (*disable_jpgclk)(struct hipp_common_s *drv, unsigned int dvfs);   /* CPE/ORB/JPEGen/JPEGde/FD Clock Disable Handler */
    int (*set_jpgclk)(struct hipp_common_s *drv, unsigned int dvfs);   /* CPE/ORB/JPEGen/JPEGde/FD Clock Disable Handler */
    int (*dump)(struct hipp_common_s *drv);             /* CPE/ORB/JPEGen/JPEGde/FD Smmu Common Handler */
    unsigned int (*shrf2da)(struct hipp_common_s *drv, unsigned int sharefd, unsigned int* size); /* ORB sharefd to va Handler */
	void *comdev;                                       /**/
    void *priv;                                         /**/
    struct hipp_commntn_s mntn_smmu;                    /* Hipp Common Mntn Smmu */
    struct hipp_commntn_s mntn_lock;                    /* Hipp Common Mntn Usecase Lock */
    struct hipp_commntn_s mntn_clck;                    /* Hipp Common Mntn Jpeg Clock */
};

struct hipp_common_s *hipp_register(int type, int mode);
int hipp_unregister(int type);
void hipp_trusted_unmap(struct hipp_common_s *drv, unsigned int da, size_t size);

#endif /* _LINUX_HISI_HIPP_H_ */
