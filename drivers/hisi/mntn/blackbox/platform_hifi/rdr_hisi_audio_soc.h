/*
 * audio soc rdr.
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __RDR_HISI_SOC_H__
#define __RDR_HISI_SOC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifndef RESET_CBFUNC_PRIO_LEVEL_LOWT
#define RESET_CBFUNC_PRIO_LEVEL_LOWT    (0)
#endif

#ifndef RESET_CBFUNC_PRIO_LEVEL_HIGH
#define RESET_CBFUNC_PRIO_LEVEL_HIGH    (49)
#endif

#define RESET_CBFUN_IGNORE_NAME         "NONAME"
#define BSP_RESET_OK                    0
#define BSP_RESET_ERROR                 (-1)
#define DRV_MODULE_NAME_LEN             9  /* exclude '\0' */
#define HIFI_SEC_DDR_MAX_NUM            (100)

#ifdef CONFIG_HISI_FAMA
#define ICC_SHARE_FAMA_ADDR_OFFSET      (HISI_RESERVED_MODEM_SHARE_PHYMEM_BASE_FAMA - HISI_RESERVED_MODEM_SHARE_PHYMEM_HIFI_ICC_BASE)
#else
#define ICC_SHARE_FAMA_ADDR_OFFSET      0
#endif

struct drv_hifi_sec_info {
	unsigned int type;
	unsigned int src_addr;
	unsigned int des_addr;
	unsigned int size;
};

struct drv_hifi_sec_ddr_head {
	unsigned int sections_num;
	struct drv_hifi_sec_info sections[HIFI_SEC_DDR_MAX_NUM];
};

/*** for icc begin***/
#define ICC_CHN_ID_MAX 16
#define ICC_STAT_MSG_NUM   1000

#define ICC_DEBUG_PROTECT_WORD1 0xCDCDDCDC
#define ICC_DEBUG_PROTECT_WORD2 0xEFEFFEFE

struct icc_channel_fifo {
	unsigned int magic;     /* fifo魔数，标识通道fifo的状态 */
	unsigned int size;      /* fifo大小 */
	unsigned int write;     /* fifo读指针 */
	unsigned int read;      /* fifo写指针 */
	unsigned char data[4];  /* fifo的context */
};

struct icc_channel {
	unsigned int id;               /* 通道id */
	unsigned char name[4];          /* 通道名称 */
	unsigned int send_seq_num;     /* 下一个发送数据包的序列号 */
	unsigned int recv_seq_num;     /* 上一个接收数据包的序列号 */
	unsigned int ready_recv;       /* 接收FIFO的状态: 标识当前FIFO是否可用 */
	unsigned int ipc_send_irq_id;  /* 通道私有的发送数据使用的ipc中断 */
	unsigned int ipc_recv_irq_id;  /* 通道私有的接收数据使用的ipc中断 */
	unsigned int func_size;        /* 接收向量(子通道)大小 */
	unsigned int fifo_recv;        /* 接收fifo的指针 */
	unsigned int fifo_send;        /* 发送fifo的指针 */
};


struct icc_control {
	unsigned int cpu_id;                      /* 当前核cpu id */
	unsigned int channel_num;                 /* 通道数目 */
	unsigned char channels[ICC_CHN_ID_MAX*4];  /* icc_channel的结构体指针数组 */
};


struct icc_uni_msg_info {
	unsigned short channel_id;
	unsigned short packetlen;
	unsigned short msg_id;
	unsigned short drop;
	unsigned short fifo_read_pos;
	unsigned short fifo_write_pos;
	unsigned int time;
	unsigned int sender_pid;
	unsigned int receiver_pid;
};

struct icc_msg_fifo {
	unsigned int front;
	unsigned int rear;
	unsigned int size;
	struct icc_uni_msg_info msg[ICC_STAT_MSG_NUM];
};

struct icc_msg_info {
	struct icc_msg_fifo send;
	struct icc_msg_fifo recv;
};

struct icc_dbg {
	unsigned int protectword1;
	unsigned int total_length;
	unsigned int heap_base_offset;
	struct icc_control icc_ctrl;
	struct icc_channel icc_channels[2];
	unsigned int state;
	unsigned int ipc_int_cnt;
	struct icc_msg_info msg_stat;
	unsigned int protectword2;
};
/*** for icc end***/

/*用于区分在Modem/HIFI复位前还是复位后调用回调函数*/
enum DRV_RESET_CALLCBFUN_MOMENT {
	DRV_RESET_CALLCBFUN_RESET_BEFORE,   /*Modem/HIFI 复位前*/
	DRV_RESET_CALLCBFUN_RESET_AFTER,    /*Modem/HIFI 复位后*/
	DRV_RESET_CALLCBFUN_RESETING,       /*Modem/HIFI 复位中*/
	DRV_RESET_CALLCBFUN_MOEMENT_INVALID
};

/*Record information of callback functions*/
typedef int (*pdrv_reset_cbfun)(enum DRV_RESET_CALLCBFUN_MOMENT eparam, int userdata);
struct sreset_mgr_callbackfunc {
	char name[DRV_MODULE_NAME_LEN + 1];
	int priolevel;
	pdrv_reset_cbfun cbfun;
	int userdata;
};

/*Link list unit of managing all callback functions*/
struct sreset_mgr_lli {
	struct sreset_mgr_callbackfunc cbfuninfo;
	struct sreset_mgr_lli *pnext;
};

#define OM_LOG_INNER_MAX_NUM (300)
#define INNERLOG_ONE_INFO_LEN (124)
#define UCOM_MEM_DYN_NODE_MAX_NUM (959)
#define MEM_DYN_STATUS_ONE_INFO_LEN (307)
#define MEM_DYN_NODE_ONE_INFO_LEN (167)
#define COMPILE_TIME_BUFF_SIZE (24)
#define RDR_FALG_OFFSET (HIFI_FLAG_DATA_ADDR - HIFI_DUMP_BIN_ADDR)
#define PARSE_INNERLOG_SIZE (OM_LOG_INNER_MAX_NUM * INNERLOG_ONE_INFO_LEN)
#define PARSE_MEM_DYN_LOG_SIZE (MEM_DYN_STATUS_ONE_INFO_LEN*UCOM_MEM_DYN_TYPE_BUTT + MEM_DYN_NODE_ONE_INFO_LEN*UCOM_MEM_DYN_NODE_MAX_NUM)

/*Innerlog*/
struct innerlog_record
{
	unsigned int enlogid;
	unsigned int time_stamp;
	unsigned short fileid;
	unsigned short lineid;
	unsigned int value1;
	unsigned int value2;
	unsigned int value3;
};

struct innerlog_reset
{
	unsigned int cnt;
	unsigned int time_stamp;
};

struct innerlog_obj
{
	unsigned int curr_idx;
	struct innerlog_record records[OM_LOG_INNER_MAX_NUM];
	struct innerlog_reset wait_reset;
};

#define SOCHIFI_ORIGINAL_INNERLOG_SIZE  (sizeof(struct innerlog_obj))

/*mem_dyn*/
enum UCOM_MEM_DYN_OM_ENABLE_ENUM {
    UCOM_MEM_DYN_OM_ENABLE_NO = 0,
    UCOM_MEM_DYN_OM_ENABLE_YES,
    UCOM_MEM_DYN_OM_ENABLE_BUTT,
};

enum UCOM_MEM_DYN_TYPE_ENUM {
	UCOM_MEM_DYN_TYPE_DDR = 0,
	UCOM_MEM_DYN_TYPE_TCM,
	UCOM_MEM_DYN_TYPE_OCB,	/* memory on OCB (On Chip Buffer, another name is sram) */
	UCOM_MEM_DYN_TYPE_USB_160K,
	UCOM_MEM_DYN_TYPE_USB_96K,
	UCOM_MEM_DYN_TYPE_BUTT,
};

typedef struct {
	unsigned int addr;
	unsigned int size;
	unsigned short fileid;
	unsigned short lineid;
} mem_dyn_blk_stru;

typedef struct {
	unsigned int time_stamp;
	unsigned int curr_used_rate;
} mem_dyn_trace_stru;

struct mem_dyn_node {
	mem_dyn_blk_stru blk;
	unsigned int next;     //struct mem_dyn_node *
	unsigned int prev;     //struct mem_dyn_node *
};

struct mem_dyn_list {
	struct mem_dyn_node head;
	struct mem_dyn_node tail;
	unsigned int len;
};

struct mem_dyn_status {
	unsigned int enable;
	unsigned int total_size;
	unsigned int used_rate;
	mem_dyn_trace_stru mem_trace;
};

struct mem_dyn_ctrl {
	struct mem_dyn_status status[UCOM_MEM_DYN_TYPE_BUTT];
	struct mem_dyn_list freelist[UCOM_MEM_DYN_TYPE_BUTT];
	struct mem_dyn_list unfreelist[UCOM_MEM_DYN_TYPE_BUTT];
	struct mem_dyn_list idlelist;
	struct mem_dyn_node nodes[UCOM_MEM_DYN_NODE_MAX_NUM];
};

#define SOCHIFI_ORIGINAL_DYNMEM_SIZE (sizeof(struct mem_dyn_ctrl))

int  rdr_audio_soc_init(void);
void rdr_audio_soc_exit(void);

void rdr_audio_soc_dump(u32 modid, char *pathname, pfn_cb_dump_done pfb);
void rdr_audio_soc_reset(u32 modid, u32 etype, u64 coreid);

/*HIFI复位前后的回调注册函数*/
int hifireset_regcbfunc(const char *pname, pdrv_reset_cbfun pcbfun, int userdata, int priolevel);
int hifireset_runcbfun(enum DRV_RESET_CALLCBFUN_MOMENT eparam);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

