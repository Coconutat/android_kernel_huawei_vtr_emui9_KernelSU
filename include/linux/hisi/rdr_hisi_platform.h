#ifndef __RDR_HISI_PLATFORM_H__
#define __RDR_HISI_PLATFORM_H__
#include <linux/hisi/rdr_types.h>
#include <linux/hisi/rdr_pub.h>
#include <mntn_public_interface.h>

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
	MODID_AP_S_COMBINATIONKEY = 0x80000006,
	MODID_AP_S_SUBPMU         = 0x80000007,
	MODID_AP_S_MAILBOX        = 0x80000008,
	MODID_AP_S_SCHARGER       = 0x80000009,
	MODID_AP_S_F2FS           = 0x8000000a,
	MODID_AP_S_BL31_PANIC     = 0x8000000b,
	MODID_AP_S_RESUME_SLOWY   = 0x8000000c,
	MODID_CHARGER_S_WDT       = 0x8000000d,
	MODID_AP_S_HHEE_PANIC     = 0x8000000e,
	MODID_AP_S_WDT             = 0x8000000f,
	MODID_AP_S_L3CACHE_ECC     = 0x8000001f,
	MODID_AP_S_PANIC_SOFTLOCKUP = 0x80000020,
	MODID_AP_S_PANIC_OTHERCPU_HARDLOCKUP = 0x80000021,
	MODID_AP_S_PANIC_SP805_HARDLOCKUP = 0x80000022,

	/* Exceptions for Huawei Device Co. Ltd. */
	MODID_AP_S_VENDOR_BEGIN = 0x80100000,
	MODID_AP_S_VENDOR_END   = 0x801fffff,

	MODID_AP_END              = HISI_BB_MOD_AP_END
} modid_ap;

typedef enum {
	MODU_NOC,
	MODU_DDR,
	MODU_TMC,
	MODU_GAP,	/*256 byte space as the gap, adding modules need before this */
	MODU_MAX
} dump_mem_module;

#ifdef CONFIG_HISI_CORESIGHT_TRACE
#define		ETR_DUMP_NAME		"etr_dump.ad"
#endif

typedef int (*rdr_hisiap_dump_func_ptr) (void *dump_addr, unsigned int size);

#ifdef CONFIG_HISI_BB

extern int g_bbox_fpga_flag;

void save_module_dump_mem(void);
void regs_dump(void);
void hisiap_nmi_notify_lpm3(void);
int register_module_dump_mem_func(rdr_hisiap_dump_func_ptr func,
				  char *module_name, dump_mem_module modu);
void set_exception_info(unsigned long address);
bool rdr_get_ap_init_done(void);
unsigned long long get_pmu_reset_reg(void);
void set_reboot_reason(unsigned int reboot_reason);
unsigned int get_reboot_reason(void);
int rdr_press_key_to_fastboot(struct notifier_block *nb,
		unsigned long event, void *buf);
void rdr_long_press_powerkey(void);
unsigned long long get_pmu_subtype_reg(void);
void set_subtype_exception(unsigned int subtype, bool save_value);
unsigned int get_subtype_exception(void);
char *rdr_get_subtype_name(u32 e_exce_type,u32 subtype);
char *rdr_get_category_name(u32 e_exce_type, u32 subtype);
u32 rdr_get_exec_subtype_value(void);
char *rdr_get_exec_subtype(void);

#else
static inline void save_module_dump_mem(void) {}
static inline void regs_dump(void) {}
static inline void hisiap_nmi_notify_lpm3(void) {}
static inline void set_exception_info(unsigned long address){}
static inline int register_module_dump_mem_func(rdr_hisiap_dump_func_ptr func,
				  char *module_name, dump_mem_module modu){return -1;}
static inline bool rdr_get_ap_init_done(void){return 0;}
static inline unsigned long long get_pmu_reset_reg(void){return 0;}
static inline void set_reboot_reason(unsigned int reboot_reason) {}
static inline unsigned int get_reboot_reason(void){return 0;}
static inline int rdr_press_key_to_fastboot(struct notifier_block *nb,
		unsigned long event, void *buf){return 0;}
static inline void rdr_long_press_powerkey(void){}
static inline unsigned long long get_pmu_subtype_reg(void){return 0;}
static inline void set_subtype_exception(unsigned int subtype, bool save_value){}
static inline unsigned int get_subtype_exception(void) {return 0;}
static inline char *rdr_get_subtype_name(u32 e_exce_type,u32 subtype) {return NULL;}
static inline char *rdr_get_category_name(u32 e_exce_type, u32 subtype) {return NULL;}
static inline u32 rdr_get_exec_subtype_value(void) { return 0;}
static inline char *rdr_get_exec_subtype(void) { return NULL;}
#endif

#ifdef CONFIG_HISI_IRQ_REGISTER
void irq_register_hook(struct pt_regs *reg);
void show_irq_register(void);
#else
static inline void irq_register_hook(struct pt_regs *reg) {}
static inline void show_irq_register(void) {}
#endif

#ifdef CONFIG_HISI_REENTRANT_EXCEPTION
void reentrant_exception(void);
#else
static inline void reentrant_exception(void) {}
#endif

#ifdef CONFIG_HISI_BB_DEBUG
void last_task_stack_dump(void);
#else
static inline void last_task_stack_dump(void) {}
#endif

#endif
