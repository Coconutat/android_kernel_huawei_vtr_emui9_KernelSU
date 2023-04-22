//**********************************************************************************
// FILE DESCRIPTION
// Copyright (C) 2018, Huawei Technologies Co., Ltd. All Rights Reserved.
//**********************************************************************************

#ifndef __BBOX_HDC_H_
#define __BBOX_HDC_H_

enum bbox_dataset_type {
    BBOX_DATASET_FULL   = 1, // full set
    BBOX_DATASET_MIN    = 2, // mini set
    BBOX_DATASET_DDR    = 3, // ddr dump
    BBOX_DATASET_NOFLASH= 4, // no flash data
    BBOX_DATASET_NODDR  = 5, // no ddr data
    BBOX_DATASET_DMA    = 6, // dma dump
};

/* host侧获取数据的模式，可以以位操作组合 */
enum bbox_fetch_pattern {
    BBOX_FETCH_NULL         = 1 << 0,
    BBOX_FETCH_FLASH_SINGLE = 1 << 1,
    BBOX_FETCH_FLASH_MULTI  = 1 << 2,
    BBOX_FETCH_DDR          = 1 << 3,
};

/* enum type in bbox_hdc_header_t */
enum bbox_hdc_type {
    BBOX_HDC_EXCEPTION_SREPORT = 0x1,
    BBOX_HDC_EXCEPTION_MREPORT,
    BBOX_HDC_RESTART_NOTIFY,
    BBOX_HDC_DDR_DUMP_UPLOAD,
    BBOX_HDC_FLASH_UPLOAD,
    BBOX_HDC_ACK,
    BBOX_HDC_ACK_WITH_DUMP_TYPE,
    BBOX_HDC_ACK_MULTI,
    BBOX_HDC_NAK,
};

/* enum type of segment flag */
enum bbox_hdc_seg_flag {
    BBOX_HDC_SEG_STRT = 1, //the first segment pkt
    BBOX_HDC_SEG_CONT = 2, //middle segment pkts
    BBOX_HDC_SEG_FINL = 3, //the last segment pkt
};

typedef struct bbox_time {
    u64 tv_sec;
    u64 tv_usec;
}bbox_time_t;

typedef struct bbox_hdc_header {
    u32 type;  //header type
    u32 len;   //hole message length
} bbox_hdc_header_t;

typedef struct bbox_exception_upload_hdr {
    bbox_hdc_header_t header;
    bbox_time_t tm; // time stamp
    u32 modid;      // exception id
    u32 type;       // data set type
    u64 coreid;     // module id
    u32 etype;      // exception reason
    u32 len;        // the length of data below
    u8 segflg;      // segment flag for multi-part pkg
    u8 pad[7];      // padding
    u8 data[0];     // data
} bbox_exception_upload_hdr_t;

typedef struct bbox_restart_notify_hdr {
    bbox_hdc_header_t header;
    u32 reset_type;
} bbox_restart_notify_hdr_t;

typedef struct bbox_reply_hdr {
    bbox_hdc_header_t header;
    u8 pattern; //bb_hagent_fetch_pattern
    u8 pad[7];  //align
} bbox_reply_hdr_t;

#endif /* __BBOX_HDC_H_ */
