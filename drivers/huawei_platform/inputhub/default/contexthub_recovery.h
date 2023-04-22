#ifndef __LINUX_RECOVERY_H__
#define __LINUX_RECOVERY_H__
#include <linux/hisi/rdr_pub.h>
#include  <iomcu_ddr_map.h>
#include "contexthub_boot.h"
#include "protocol.h"

#define IOM3_RECOVERY_UNINIT	(0)
#define IOM3_RECOVERY_IDLE		(IOM3_RECOVERY_UNINIT + 1)
#define IOM3_RECOVERY_START	(IOM3_RECOVERY_IDLE + 1)
#define IOM3_RECOVERY_MINISYS	(IOM3_RECOVERY_START + 1)
#define IOM3_RECOVERY_DOING	(IOM3_RECOVERY_MINISYS + 1)
#define IOM3_RECOVERY_3RD_DOING	(IOM3_RECOVERY_DOING + 1)
#define IOM3_RECOVERY_FAILED	(IOM3_RECOVERY_3RD_DOING + 1)

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

#define I2C_0_RST_VAL	(BIT(3))

#define IOM3_REC_NEST_MAX		(5)

#define WD_INT  144

#define SENSORHUB_MODID          HISI_BB_MOD_IOM_START
#define SENSORHUB_USER_MODID     (HISI_BB_MOD_IOM_START + 1)
#define SENSORHUB_MODID_END      HISI_BB_MOD_IOM_END
#define SENSORHUB_DUMP_BUFF_ADDR DDR_LOG_BUFF_ADDR_AP
#define SENSORHUB_DUMP_BUFF_SIZE DDR_LOG_BUFF_SIZE

/*#define SCIOMCUCTRL  0x598*/

#define REG_UNLOCK_KEY  0x1ACCE551
#define WDOGCTRL		0x08
#define WDOGINTCLR	0x0C
#define WDOGLOCK	0xC00

#define PATH_MAXLEN         128
#define HISTORY_LOG_SIZE 256
#define HISTORY_LOG_MAX  0x80000	/*512k */
#define ROOT_UID		0
#define SYSTEM_GID		1000
#define DIR_LIMIT		0770
#define FILE_LIMIT		0660
#define SH_DMP_DIR  "/data/log/sensorhub-log/"
#define SH_DMP_FS  "/data/lost+found"
#define SH_DMP_HISTORY_FILE "history.log"
#define DATATIME_MAXLEN     24	/* 14+8 +2, 2: '-'+'\0' */
#define MAX_DUMP_CNT     32

#define SH_DUMP_INIT 0
#define SH_DUMP_START 1
#define SH_DUMP_FINISH 2

typedef enum
{
    SH_FAULT_HARDFAULT = 0,
    SH_FAULT_BUSFAULT,
    SH_FAULT_USAGEFAULT,
    SH_FAULT_MEMFAULT,
    SH_FAULT_NMIFAULT,
    SH_FAULT_ASSERT,
    SH_FAULT_INTERNELFAULT = 16,
    SH_FAULT_IPC_RX_TIMEOUT,
    SH_FAULT_IPC_TX_TIMEOUT,
    SH_FAULT_RESET,
    SH_FAULT_USER_DUMP,
    SH_FAULT_RESUME,
    SH_FAULT_REDETECT,
    SH_FAULT_PANIC,
    SH_FAULT_NOC,
    SH_FAULT_EXP_BOTTOM,
} exp_source_t;

typedef struct
{
    uint32_t cnt;
    uint32_t len;
    uint32_t tuncate;
} dump_zone_head_t;

int thp_prox_event_report(int value[], int length);
extern int iom3_need_recovery(int modid, exp_source_t f);
extern int recovery_init(void);
extern int register_iom3_recovery_notifier(struct notifier_block *nb);
extern int iom3_rec_sys_callback(const pkt_header_t *head);

#endif //__LINUX_RECOVERY_H__
