#ifndef __RDR_HISI_PLATFORM_H__
#define __RDR_HISI_PLATFORM_H__
#include <linux/mntn/rdr_types.h>
#include <linux/mntn/rdr_pub.h>
#include <linux/mntn/mntn_public_interface.h>

#ifdef BBOX_UT_TEST
#define BBOX_STATIC
#define BBOX_STATIC_INLINE
#define BBOX_LOFF_T long long
#define BBOX_VOLATILE
#else
#define BBOX_STATIC static
#define BBOX_STATIC_INLINE static inline
#define BBOX_LOFF_T loff_t
#define BBOX_VOLATILE volatile
#endif

#ifdef CONFIG_GCOV_KERNEL
#define KBOX_KMAP kmap
#define KBOX_KUNMAP kunmap
#define KBOX_PANIC(x) 
#else
#define KBOX_KMAP kmap_atomic
#define KBOX_KUNMAP kunmap_atomic
#define KBOX_PANIC(x) panic(x)
#endif

/* AP dump memory area*/
#define AP_DUMP_MEM_MODU_NOC_SIZE   0x1000
#define AP_DUMP_MEM_MODU_DDR_SIZE   0x2000
#define AP_DUMP_MEM_MODU_TMC_SIZE   0x20000 /* 128k for TMC */
#define AP_DUMP_MEM_MODU_GAP_SIZE   0x100   /*256 byte space as the gap, adding modules need before this */
#define AP_DUMP_MEM_MODU_KBOX_SIZE  0x80000 /* 512k for KBOX */

/* Confirm this definition is same as in the kernel/drivers/hisi/modem/drv/om/dump/rdr_adp.h.*/
#define RDR_MODEM_NOC_MOD_ID 0x82000030
#define RDR_MODEM_DMSS_MOD_ID 0x82000031
#define RDR_AUDIO_NOC_MODID 0x84000021

/*PMU register mask*/
#define PMU_RESET_REG_MASK PMU_RESET_VALUE_USED

/*record log length*/
#define LOG_PATH_LEN    96
#define DEST_LOG_PATH_LEN    (LOG_PATH_LEN+10)
#define NEXT_LOG_PATH_LEN    (LOG_PATH_LEN+30)


typedef enum {
    MODID_AP_START            = HISI_BB_MOD_AP_START,
    MODID_AP_S_PANIC          = 0x80000001,
    MODID_AP_S_NOC            = 0x80000002,
    MODID_AP_S_PMU            = 0x80000003,
    MODID_AP_S_DDRC_SEC       = 0x80000004,
    MODID_AP_S_SMPL           = 0x80000005,
    MODID_AP_S_MAILBOX        = 0x80000008,
    MODID_AP_S_SCHARGER       = 0x80000009,
    MODID_AP_S_F2FS           = 0x8000000a,
    MODID_AP_END              = HISI_BB_MOD_AP_END
} modid_ap;

typedef enum {
    MODU_NOC,
    MODU_DDR,
    MODU_TMC,
    MODU_GAP,	/*256 byte space as the gap, adding modules need before this */
    MODU_KBOX,
    MODU_MAX
} dump_mem_module;

#ifdef CONFIG_HISI_CORESIGHT_TRACE
#define     ETR_DUMP_NAME       "etr_dump.ad"
#endif

typedef int (*rdr_hisiap_dump_func_ptr) (void *dump_addr, unsigned int size);

#ifdef CONFIG_HISI_BB

extern int g_bbox_fpga_flag;

void save_module_dump_mem(void);
void regs_dump(void);
void last_task_stack_dump(void);
int register_module_dump_mem_func(rdr_hisiap_dump_func_ptr func,
                                                                        char *module_name, dump_mem_module modu);
unsigned long long get_pmu_reset_reg(void);

#else
static inline void save_module_dump_mem(void) {}
static inline void regs_dump(void) {}
static inline void last_task_stack_dump(void) {}
static inline int register_module_dump_mem_func(rdr_hisiap_dump_func_ptr func,
                                                                                         char *module_name, dump_mem_module modu){return -1;}
static inline bool unsigned long long get_pmu_reset_reg(void){return 0;};

#endif

#endif
