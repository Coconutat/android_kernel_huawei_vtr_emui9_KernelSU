/*
 *  drivers/misc/inputhub/inputhub_mcu.h
 *  Sensor Hub Channel Bridge
 *
 *  Copyright (C) 2013 Huawei, Inc.
 *  Author: huangjisong <inputhub@huawei.com>
 *
 */
#ifndef __LINUX_INPUTHUB_CMU_H__
#define __LINUX_INPUTHUB_CMU_H__
#include <linux/types.h>
#include  <global_ddr_map.h>
#include "rdr_sensorhub.h"
#include <huawei_platform/power/charging_core_sh.h>
#include <huawei_platform/power/huawei_charger_sh.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>

#define STARTUP_IOM3_CMD		(0x00070001)
#define RELOAD_IOM3_CMD		(0x0007030D)

/*for iom3 recovery debug used*/
#define IOMCU_CLK_SEL 0x00
#define IOMCU_CFG_DIV0 0x04
#define IOMCU_CFG_DIV1 0x08
#define CLKEN0_OFFSET    0x010
#define CLKSTAT0_OFFSET  0x18
#define CLKEN1_OFFSET        0x090
#define RSTEN0_OFFSET	   0x020
#define RSTDIS0_OFFSET  0x024
#define RSTSTAT0_OFFSET    0x028

#ifdef CONFIG_IOM3_RECOVERY
#define IOM3_RECOVERY_UNINIT	(0)
#define IOM3_RECOVERY_IDLE		(IOM3_RECOVERY_UNINIT + 1)
#define IOM3_RECOVERY_START	(IOM3_RECOVERY_IDLE + 1)
#define IOM3_RECOVERY_MINISYS	(IOM3_RECOVERY_START + 1)
#define IOM3_RECOVERY_DOING	(IOM3_RECOVERY_MINISYS + 1)
#define IOM3_RECOVERY_3RD_DOING	(IOM3_RECOVERY_DOING + 1)
#define IOM3_RECOVERY_FAILED	(IOM3_RECOVERY_3RD_DOING + 1)
#endif

#define IOMCU_CONFIG_SIZE  0x1000
#define IOMCU_CONFIG_START  (HISI_RESERVED_SENSORHUB_SHARE_MEM_PHYMEM_BASE + 0x80000 - IOMCU_CONFIG_SIZE)

#define WARN_LEVEL 2
#define INFO_LEVEL 3

typedef struct
{
    u32 mutex;
    u16 index;
    u16 pingpang;
    u32 buff;
    u32 ddr_log_buff_cnt;
    u32 ddr_log_buff_index;
    u32 ddr_log_buff_last_update_index;
} log_buff_t;

typedef struct WRONG_WAKEUP_MSG
{
    u32 flag;
    u64 time;
    u32 irq0;
    u32 irq1;
    u32 recvfromapmsg[4];
    u32 recvfromlpmsg[4];
    u32 sendtoapmsg[4];
    u32 sendtolpmsg[4];
    u32 recvfromapmsgmode;
    u32 recvfromlpmsgmode;
    u32 sendtoapmsgmode;
    u32 sendtolpmsgmode;
} wrong_wakeup_msg_t;

typedef enum DUMP_LOC
{
    DL_NONE = 0,
    DL_TCM,
    DL_EXT,
    DL_BOTTOM = DL_EXT,
} dump_loc_t;

enum DUMP_REGION
{
    DE_TCM_CODE,
    DE_DDR_CODE,
    DE_DDR_DATA,
    DE_BOTTOM = 16,
};

typedef struct DUMP_REGION_CONFIG
{
    u8 loc;
    u8 reserved[3];
} dump_region_config_t;

typedef struct DUMP_CONFIG
{
    u64 dump_addr;
    u32 dump_size;
    u64 ext_dump_addr;
    u32 ext_dump_size;
    u8 enable;
    u8 finish;
    u8 reason;
    dump_region_config_t elements[DE_BOTTOM];
} dump_config_t;

typedef struct modem_state {
	u32 AR_state;
	u32 MCU_state;
}modem_state_t;

typedef struct {
	u64 syscnt;
	u64 kernel_ns;
}timestamp_kernel_t;

typedef struct {
	u64 syscnt;
	u64 kernel_ns;
	u32 timer32k_cyc;
}timestamp_iomcu_base_t;

struct CONFIG_ON_DDR
{
    dump_config_t dump_config;
    log_buff_t LogBuffCBBackup;
    wrong_wakeup_msg_t WrongWakeupMsg;
    u32 log_level;
    float gyro_offset[3];
    modem_state_t modem_state;
    u8 SAR_open_status;
    timestamp_kernel_t timestamp_base;
    timestamp_iomcu_base_t timestamp_base_iomcu;
    u64 reserved;
    struct charge_core_info_sh g_core_info;
    struct charge_device_info_sh g_di;
    struct sensorhub_scene scenes;
    struct uscp_device_info_sh g_di_uscp;
    struct coul_core_info_sh g_di_coul_info_sh;
};

/*receive data from mcu,you should copy the buf each time.*/
/*extern size_t (*api_inputhub_mcu_recv)(const char *buf,unsigned long length);*/
extern int (*api_inputhub_mcu_recv) (const char* buf,
                                     unsigned int length);
extern int (*api_mculog_process) (const char* buf,
                                  unsigned int length);

int getSensorMcuMode(void);
int get_iomcu_power_state(void);
/*connect to mcu,register callback receive function.*/
int inputhub_mcu_connect(void);
int inputhub_mcu_disconnect(void);

/*send data to mcu, impliment multithread safe.*/
int inputhub_mcu_send(const char* buf, unsigned int length);

void write_timestamp_base_to_sharemem(void);
int iom3_need_recovery(int modid, exp_source_t f);
int is_sensorhub_disabled(void);
void restart_iom3(void);
void peri_used_request(void);
void peri_used_release(void);
#endif /* __LINUX_INPUTHUB_CMU_H__ */
