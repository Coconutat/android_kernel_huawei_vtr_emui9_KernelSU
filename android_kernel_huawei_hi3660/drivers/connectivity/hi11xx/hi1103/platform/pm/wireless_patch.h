/*
 *
 * Copyright (C), 2001-2021, Huawei Tech. Co., Ltd.
 *
 * File Name     : wireless_patch.h
 * Version       : Initial Draft
 * Created       : 2012/11/1
 * Last Modified :
 * Description   : wireless_patch.c header file
 * Function List :
 * History       :
 * 1.Date        : 2012/11/1
 *   Modification: Created file
 *
 */

#ifndef __WIRELESS_PATCH_H__
#define __WIRELESS_PATCH_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*
 * 1 Other Header File Including
 */


/*
 * 2 Macro Definition
 */

#include "hw_bfg_ps.h"
#include "plat_debug.h"
#include "plat_firmware.h"

#define OS_INIT_WAITQUEUE_HEAD(wq)                                  init_waitqueue_head(wq)
#define OS_WAIT_EVENT_INTERRUPTIBLE(wq, condition)                  wait_event_interruptible(wq, condition)
#define OS_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(wq, condition, time)    wait_event_interruptible_timeout(wq, condition, time)
#define OS_WAITQUEUE_ACTIVE(wq)                                     waitqueue_active(wq)
#define OS_WAKE_UP_INTERRUPTIBLE(wq)                                wake_up_interruptible(wq)

#define SOH                         0x01       /* 开始字符 */
#define EOT                         0x04       /* 发送完成 */
#define ACK                         0x06       /* 正确接收应答 */
#define NAK                         0x15       /* 校验错误重新发送，通讯开始时用于接收方协商累加校验 */
#define CAN                         0x18       /* 结束下载 */

#define PATCH_INTEROP_TIMEOUT        (HZ)

#define DATA_BUF_LEN                128

#define CFG_PACH_LEN                64

#ifdef UT_TEST
#define UART_CFG_FILE               "./config/uart_cfg"
#define SDIO_CFG_FILE               "./config/sdio_cfg"
#else
#define UART_CFG_FILE               "/system/vendor/firmware/uart_cfg"
#define SDIO_CFG_FILE               "/system/vendor/firmware/sdio_cfg"
#define SDIO_RECOVER_CFG_FILE       "/system/vendor/firmware/sdio_recover_cfg"
#define SDIO_RECOVER_FLAG_FILE      "/data/hwlogdir/recoverflag"
#endif

#define PLL_CMD_KEYWORD             "PLL"
#define BRT_CMD_KEYWORD             "BAUDRATE"

#define QUIT_CMD                    "QUIT"

#define CMD_LEN                     32
#define PARA_LEN                    128

#define UART_KEY                    "UART"
#define FILE_TYPE_RW_KEY            "ADDR_FILE_RW"
#define FILE_TYPE_CMD_KEY           "ADDR_FILE_"
#define NUM_TYPE_CMD_KEY            "PARA_"
#define QUIT_TYPE_CMD_KEY           "QUIT"


#define PATCH_SEND_CAN_SDIO         do{patch_send_char(CAN, NO_RESPONSE, ENUM_INFO_SDIO);}while(0)
#define PATCH_SEND_EOT_SDIO         do{patch_send_char(EOT, WAIT_RESPONSE, ENUM_INFO_SDIO);}while(0)
#define PATCH_SEND_A_SDIO           do{patch_send_char('a', NO_RESPONSE, ENUM_INFO_SDIO);}while(0)
#define PATCH_SEND_N_SDIO           do{patch_send_char('n', NO_RESPONSE, ENUM_INFO_SDIO);}while(0)

#define PATCH_SEND_CAN_UART         do{patch_send_char(CAN, NO_RESPONSE, ENUM_INFO_UART);}while(0)
#define PATCH_SEND_EOT_UART         do{patch_send_char(EOT, WAIT_RESPONSE, ENUM_INFO_UART);}while(0)
#define PATCH_SEND_A_UART           do{patch_send_char('a', NO_RESPONSE, ENUM_INFO_UART);}while(0)
#define PATCH_SEND_N_UART           do{patch_send_char('n', NO_RESPONSE, ENUM_INFO_UART);}while(0)

/*
#define CMD_LINE_SIGN               0x3B
*/

#define  MSG_FORM_DRV_G             'G'
#define  MSG_FORM_DRV_C             'C'
#define  MSG_FORM_DRV_A             'a'
#define  MSG_FORM_DRV_N             'n'


/*
#define  HOST_DEV_TIMEOUT           3
*/


//#define RECV_BUF_LEN                32
//#define READ_CFG_BUF_LEN            1024
#define READ_PATCH_BUF_LEN          1024 * 32
#define READ_DATA_BUF_LEN           1024 * 50
#define READ_DATA_REALLOC_BUF_LEN   1024 * 4

#define XMODE_DATA_LEN              1024

#define XMODEM_PAK_LEN              XMODE_DATA_LEN + 5

#define WLAN_PINDISABLE                 1
#define BFG_PINDISABLE                  2
#define WLAN_PINSTATEENABLE             4
#define BFG_PINSTATEENABLE              8
#define PINENABLE                       0
#define PINSTATE_NUM                    2
#define PINSTATE_SWITCH_MASK            ((1 << PINSTATE_NUM) - 1)
#define PINSTATE_ENABLE_MASK            (PINSTATE_SWITCH_MASK << PINSTATE_NUM)
#define PINSTATE_MASK                   (PINSTATE_SWITCH_MASK | PINSTATE_ENABLE_MASK)

#define MIN(x, y) (x) > (y)? (y) : (x)

/*
 * 3 Enum Type Definition
 */
enum PATCH_INFO_TYPE_ENUM
{
    ENUM_INFO_SDIO    =  0,        /* SDIO 接口下载patch */
    ENUM_INFO_UART,                /* uart接口下载patch */

    ENUM_INFO_TOTAL                /* 接口总数 */

};
#if 0
enum PATCH_CFG_CMD_ENUM
{
    ERROR_TYPE_CMD  = 0,           /* 错误的命令 */
    FILE_TYPE_CMD,                 /* 下载配置文件命令 */
    NUM_TYPE_CMD,                  /* 下载配置参数命令 */
    QUIT_TYPE_CMD                  /* 退出命令 */
};
#endif

enum PATCH_WAIT_RESPONSE_ENUM
{
    NO_RESPONSE  = 0,               /* 不等待device响应 */
    WAIT_RESPONSE                   /* 等待device响应 */
};

/*
 * 4 Global Variable Declaring
 */


/*
 * 5 Message Header Definition
 */


/*
 * 6 Message Definition
 */


/*
 * 7 STRUCT Type Definition
 */
typedef wait_queue_head_t           OS_WAIT_QUEUE_HEAD_T_STRU;

typedef struct file                 OS_KERNEL_FILE_STRU;

/*
typedef struct cmd_type_st
{
    int32       cmd_type;
    uint8       cmd_name[CMD_LEN];
    uint8       cmd_para[PARA_LEN];

}CMD_TYPE_STUR;
*/

typedef struct patch_globals
{
    uint8               auc_Cfgpath[CFG_PACH_LEN];
    uint8               auc_CfgVersion[VERSION_LEN];
    uint8               auc_DevVersion[VERSION_LEN];

    uint8               auc_Recvbuf1[RECV_BUF_LEN];
    int32               l_Recvbuf1_len;

    uint8               auc_Recvbuf2[RECV_BUF_LEN];
    int32               l_Recvbuf2_len;

    int32               l_count;
    struct cmd_type_st      *pst_cmd;

    OS_WAIT_QUEUE_HEAD_T_STRU  *pst_wait;
}PATCH_GLOBALS_STUR;


/* xmodem每包数据的结构，CRC校验*/
typedef struct xmodem_crc_pkt
{
    int8  Head;                             /* 开始字符 */
    int8  PacketNum;                        /* 包序号 */
    int8  PacketAnt;                        /* 包序号补码*/
    int8  packet_data[XMODE_DATA_LEN];      /* 数据 */
    int8  CRCValue_H;                       /* CRC校验码高位 */
    int8  CRCValue_L;                       /* CRC校验码低位 */
}XMODEM_CRC_PKT_STRU;


/* xmodem每包数据的结构，CRC校验*/
typedef struct xmodem_head_pkt
{
    int8  Head;                             /* 开始字符 */
    int8  PacketNum;                        /* 包序号 */
    int8  PacketAnt;                        /* 包序号补码*/
}XMODEM_HEAD_PKT_STRU;


typedef struct ringbuf
{
    uint8  *pbufstart;
    uint8  *pbufend;
    uint8  *phead;
    uint8  *ptail;
}RINGBUF_STRU;


/*
 * 8 UNION Type Definition
 */


/*
 * 9 OTHERS Definition
 */


/*
 * 10 Function Declare
 */

extern uint16 patch_Crc16 (int8 *ptr, uint16 l_count);
extern uint8 *patch_del_space(uint8 *string, int32 *len);
extern int32 patch_device_respond(int32 type);
extern int32 patch_download_export(uint8 *keyword, int32 type);
extern int32 patch_download_patch(int32 type);
extern int32 patch_down_file(uint8 *puc_file, int32 type);
extern int32 patch_execute_cmd(int32 cmd_type, uint8 *cmd_name, uint8 *cmd_para,int32 type);
extern int32 patch_exit(void);
extern int32 patch_file_type(uint8 *Key, uint8 *Value, int32 type);
extern int32 patch_get_cfg(uint8 *cfg, int32 type);
extern int32 create_recover_flag(void);
//extern int32 patch_init(void);
extern int32 patch_int_para_send(uint8 *name, uint8 *Value, int32 type);
extern void *patch_malloc_cmd_buf(uint8 *buf, int32 type);
extern int32 patch_number_type(uint8 *Key, uint8 *Value, int32 type);
extern int32 patch_parse_cfg(uint8 *buf, int32 buf_len, int32 type);
extern int32 patch_parse_cmd(uint8 *buf, uint8 *cmd_name, uint8 *cmd_para);
extern int32 patch_quit_type(int32 type);
extern int32 patch_read_cfg(uint8 *cfg_path, uint8 *read_buf);
extern int32 patch_read_patch(int8 *buf, int32 len, OS_KERNEL_FILE_STRU *fp);
extern int32 patch_recv_mem(OS_KERNEL_FILE_STRU *fp, int32 len, int32 type);
extern int32 patch_send(uint8 *data, int32 len, uint8 expect, int32 type);
extern int32 patch_send_char(int8 num, int32 wait, int32 type);
extern int32 patch_send_data(uint8 *data, int32 len, uint8 expect, int32 type);
extern int32 patch_send_file(uint8 *data, int32 len, uint8 expect, int32 type);
extern int32 patch_string_to_num(uint8 *string);
extern int32 patch_wait_g_form_dev(int32 type);
extern int32 patch_xmodem_send(uint8 *data, int32 len, uint8 expect);
extern int32 read_msg_t(uint8 *data, int32 len, int32 type);
extern int32 recv_expect_result_t(uint8 expect, int32 type);
extern int32 send_msg_t(uint8 *data, int32 len, int32 type);
extern int32 uart_recv_data(const uint8 *data, int32 len);
extern int32 pm_change_patch_state(void);
extern int32 is_file_exist(char * filename);
extern int32 patch_init(int32 type);
extern void ps_patch_to_nomal(void);


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of oam_kernel.h */
