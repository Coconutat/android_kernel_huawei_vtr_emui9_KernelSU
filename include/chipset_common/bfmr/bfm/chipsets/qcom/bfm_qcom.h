#ifndef BFM_QCOM_H
#define BFM_QCOM_H
#include <chipset_common/bfmr/public/bfmr_public.h>

struct boot_log_struct {
  u32 boot_magic;        /* must be initialized in slb1 */

  u32 boot_stage;        /* must be initialized in every stage,and set it's val via  set_boot_stage */
  u32 last_boot_stage;

  u32 boot_error_no;     /* set this via set_boot_error */
  u32 last_boot_error_no;

  u32 rtc_time;
  u32 ats_1[2];
  u32 rcv_method;
  u32 rcv_result;
  u8 isUserPerceptiable;
  u8 last_isUserPerceptiable;
  u8 isSystemRooted;
  u8 last_isSystemRooted;
};


struct bootlog_inject_struct {
  u32 flag;
  u32 inject_boot_fail_no;
};

#define BOOT_TIMER_INTERVAL  (10000)
#define BOOT_TOO_LONG_TIME   (1000*60*30)
#define BOOT_TOO_LONG_TIME_EXACT   (1000*60*3)
#define BFM_STAGE_CODE              0x00000001
#define BFM_ERROR_CODE              0x00000002
#define BFM_TIMER_EN_CODE           0x00000003
#define BFM_TIMER_SET_CODE          0x00000004
#define BFM_CAN_NOT_CALL_TRY_TO_RECOVERY  (0xFFFFFFFF)


/* IMEM ALLOCATION */
/* SHARED_IMEM_UNUSED_SPACE_BASE (0x146bfe1C) */
//#define HWBOOT_LOG_INFO_START_BASE 0x08600B1C 
#define HWBOOT_LOG_INFO_SIZE (0x100)

#define HWBOOT_MAGIC_NUMBER   *((u32 *)("BOOT"))
#define HWBOOT_FAIL_INJECT_MAGIC  0x12345678

//#define BOOT_LOG_CHECK_SUM_SIZE  (sizeof(struct boot_log_struct) - 16)
#define BOOT_LOG_CHECK_SUM_SIZE ((int)((u8 *)(&(((struct boot_log_struct *)0)->hash_code)) - (u8 *)0))


/* export interface to bfm_core */
void qcom_set_boot_stage(bfmr_detail_boot_stage_e stage);
u32 qcom_get_boot_stage(void);
int qcom_set_boot_fail_flag(bfmr_bootfail_errno_e bootfail_errno);
int qcom_hwboot_fail_init(void);
int kmsg_print_to_ddr(char *buf, int size);
unsigned long long bfm_hctosys(unsigned long long current_secs);

/* export interface to kernel */
void hwboot_fail_init_struct(void);
void hwboot_clear_magic(void);
bool check_bootfail_inject(u32 err_code);

#endif
