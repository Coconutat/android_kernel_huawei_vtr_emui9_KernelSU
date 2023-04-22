#ifndef __LINUX_INPUTHUB_RDR_H__
#define __LINUX_INPUTHUB_RDR_H__
#include <linux/hisi/rdr_pub.h>
#include  <global_ddr_map.h>
#include "inputhub_bridge.h"

#define WD_INT  144

#define SENSORHUB_MODID     HISI_BB_MOD_IOM_START
#define SENSORHUB_USER_MODID     (HISI_BB_MOD_IOM_START + 1)
#define SENSORHUB_MODID_END HISI_BB_MOD_IOM_END
#define SENSORHUB_DUMP_BUFF_ADDR (HISI_RESERVED_SENSORHUB_SHARE_MEM_PHYMEM_BASE)
#define SENSORHUB_DUMP_BUFF_SIZE ((HISI_RESERVED_SENSORHUB_SHARE_MEM_PHYMEM_SIZE - IOMCU_CONFIG_SIZE))

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

#define SH_DUMP_INIT 0
#define SH_DUMP_START 1
#define SH_DUMP_FINISH 2

int rdr_sensorhub_init(void);
int write_ramdump_info_to_sharemem(void);
void __send_nmi(void);
void notify_rdr_thread(void);
#endif /*__LINUX_INPUTHUB_RDR_H__*/
