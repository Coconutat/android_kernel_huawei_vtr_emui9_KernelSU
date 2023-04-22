#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/hisi/hisi_rproc.h>
#include <linux/hisi/util.h>
#include <linux/hisi/ipc_msg.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <asm/compiler.h>
#include <hisee_mntn_plat.h>
#include <linux/delay.h>
#include "hisee_mntn.h"
#include "../mntn_filesys.h"
#include "../../hisee/hisi_hisee.h"
#include <dsm/dsm_pub.h>
#include <mntn_subtype_exception.h>

#define HISEE_MNTN_LPM3_STR		"lpm3"
#define HISEE_MNTN_ATF_STR		"atf"
/*lint -e785*/
struct rdr_exception_info_s hisee_excetption_info[] = {
	{
		.e_modid            = (u32)MODID_HISEE_EXC_SENSOR_CTRL,
		.e_modid_end        = (u32)MODID_HISEE_EXC_SENSOR_CTRL,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_SENSOR_CTRL,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE SENSOR",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_SIC,
		.e_modid_end        = (u32)MODID_HISEE_EXC_SIC,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_SIC,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE SIC",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_MED_ROM,
		.e_modid_end        = (u32)MODID_HISEE_EXC_MED_ROM,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_MED_ROM,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE ROM",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_MED_RAM,
		.e_modid_end        = (u32)MODID_HISEE_EXC_MED_RAM,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_MED_RAM,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE RAM",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_OTPC,
		.e_modid_end        = (u32)MODID_HISEE_EXC_OTPC,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_OTPC,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE OTPC",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_HARD,
		.e_modid_end        = (u32)MODID_HISEE_EXC_HARD,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_HARD,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE HARD",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_IPC_MAILBOX,
		.e_modid_end        = (u32)MODID_HISEE_EXC_IPC_MAILBOX,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_IPC_MAILBOX,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE IPC MAILBOX",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_MPU,
		.e_modid_end        = (u32)MODID_HISEE_EXC_MPU,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_MPU,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE MPU",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_BUS,
		.e_modid_end        = (u32)MODID_HISEE_EXC_BUS,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_BUS,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE RPMB",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_TIMER,
		.e_modid_end        = (u32)MODID_HISEE_EXC_TIMER,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_TIMER,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE TIMER",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_SEC_EXTERN,
		.e_modid_end        = (u32)MODID_HISEE_EXC_SEC_EXTERN,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_SEC_EXTERN,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE SEC EXTERN",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_WDG,
		.e_modid_end        = (u32)MODID_HISEE_EXC_WDG,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_WDG,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE WDG",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_SYSALARM,
		.e_modid_end        = (u32)MODID_HISEE_EXC_SYSALARM,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_SYSALARM,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE SYSALARM",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_NV_COUNTER,
		.e_modid_end        = (u32)MODID_HISEE_EXC_NV_COUNTER,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_NV_COUNTER,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE NV_COUNTER",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_SWP,
		.e_modid_end        = (u32)MODID_HISEE_EXC_SWP,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_SWP,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE SWP",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_COS,
		.e_modid_end        = (u32)MODID_HISEE_EXC_COS,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_COS,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE COS",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_BB,
		.e_modid_end        = (u32)MODID_HISEE_EXC_BB,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_BB,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE BB",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_MNTN_COS,
		.e_modid_end        = (u32)MODID_HISEE_EXC_MNTN_COS,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_MNTN_COS,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE MNTN_COS",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_MNTN_COS_RESET,
		.e_modid_end        = (u32)MODID_HISEE_EXC_MNTN_COS_RESET,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = 0,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_MNTN_COS_RESET,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE COS",
		.e_desc             = "HISEE NORMAL RESET",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_LIBC,
		.e_modid_end        = (u32)MODID_HISEE_EXC_LIBC,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_LIBC,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE LIBC",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_NVM,
		.e_modid_end        = (u32)MODID_HISEE_EXC_NVM,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_NVM,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE NVM",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_SECENG_TRNG,
		.e_modid_end        = (u32)MODID_HISEE_EXC_SECENG_TRNG,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_SECENG_TRNG,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE SECENG TRNG",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_SECENG_TRIM,
		.e_modid_end        = (u32)MODID_HISEE_EXC_SECENG_TRIM,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_SECENG_TRIM,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE SECENG TRIM",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_SECENG_SCE,
		.e_modid_end        = (u32)MODID_HISEE_EXC_SECENG_SCE,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_SECENG_SCE,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE SECENG SCE",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_SECENG_RSA,
		.e_modid_end        = (u32)MODID_HISEE_EXC_SECENG_RSA,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_SECENG_RSA,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE SECENG RSA",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_SECENG_SM2,
		.e_modid_end        = (u32)MODID_HISEE_EXC_SECENG_SM2,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_SECENG_SM2,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE SECENG SM2",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_SECENG_KM,
		.e_modid_end        = (u32)MODID_HISEE_EXC_SECENG_KM,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_SECENG_KM,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE SECENG KM",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_SECENG_SCRAMBLING,
		.e_modid_end        = (u32)MODID_HISEE_EXC_SECENG_SCRAMBLING,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_SECENG_SCRAMBLING,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE SECENG SCRAMBLING",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_BOTTOM,
		.e_modid_end        = (u32)MODID_HISEE_EXC_BOTTOM,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_BOTTOM,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE BOTTOM",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_ALARM0,
		.e_modid_end        = (u32)MODID_HISEE_EXC_ALARM0,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_ALARM0,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE ALARM0",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_ALARM1,
		.e_modid_end        = (u32)MODID_HISEE_EXC_ALARM1,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_ALARM1,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE ALARM1",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_AS2AP_IRQ,
		.e_modid_end        = (u32)MODID_HISEE_EXC_AS2AP_IRQ,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_AS2AP_IRQ,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE AS2AP IRQ",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_DS2AP_IRQ,
		.e_modid_end        = (u32)MODID_HISEE_EXC_DS2AP_IRQ,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_DS2AP_IRQ,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE DS2AP IRQ",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_SENC2AP_IRQ,
		.e_modid_end        = (u32)MODID_HISEE_EXC_SENC2AP_IRQ,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_SENC2AP_IRQ,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE SENC2AP IRQ",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_SENC2AP_IRQ0,
		.e_modid_end        = (u32)MODID_HISEE_EXC_SENC2AP_IRQ0,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_SENC2AP_IRQ0,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE SENC2AP IRQ0",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_SENC2AP_IRQ1,
		.e_modid_end        = (u32)MODID_HISEE_EXC_SENC2AP_IRQ1,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_SENC2AP_IRQ1,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE SENC2AP IRQ1",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_LOCKUP,
		.e_modid_end        = (u32)MODID_HISEE_EXC_LOCKUP,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_LOCKUP,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE LOCKUP",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_EH2H_SLV,
		.e_modid_end        = (u32)MODID_HISEE_EXC_EH2H_SLV,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_EH2H_SLV,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE EH2H SLV",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_TSENSOR1,
		.e_modid_end        = (u32)MODID_HISEE_EXC_TSENSOR1,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_TSENSOR1,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE TSENSOR1",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_SIMULATE_EXC_RPMB_KO,
		.e_modid_end        = (u32)MODID_SIMULATE_EXC_RPMB_KO,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = SIMULATE_EXC_RPMB_KO,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "FAIL TO RESET RPMB",
		.e_desc             = "HISEE",
	},
	{
		.e_modid            = (u32)MODID_HISEE_EXC_UNKNOWN,
		.e_modid_end        = (u32)MODID_HISEE_EXC_UNKNOWN,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority  = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_HISEE,
		.e_reset_core_mask  = RDR_HISEE,
		.e_from_core        = RDR_HISEE,
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
		.e_exce_type        = HISEE_S_EXCEPTION,
		.e_exce_subtype     = EXC_UNKNOWN,
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,
		.e_from_module      = "HISEE UNKNOWN",
		.e_desc             = "HISEE",
	}
};
/*lint +e785*/

/*for translation from original irq no to exception type that module id*/
hisee_exc_trans_s hisee_exc_trans[] = {
	{HISEE_MNTN_IRQ_GROUP_ALARM0, MODID_HISEE_EXC_ALARM0},
	{HISEE_MNTN_IRQ_GROUP_ALARM1, MODID_HISEE_EXC_ALARM1},
	{HISEE_MNTN_IRQ_GROUP_AS2AP, MODID_HISEE_EXC_AS2AP_IRQ},
	{HISEE_MNTN_IRQ_GROUP_DS2AP, MODID_HISEE_EXC_DS2AP_IRQ},
	{HISEE_MNTN_IRQ_GROUP_SENC2AP, MODID_HISEE_EXC_SENC2AP_IRQ},
	{HISEE_MNTN_IRQ_GROUP_SENC2AP_IRQ0, MODID_HISEE_EXC_SENC2AP_IRQ0},
	{HISEE_MNTN_IRQ_GROUP_SENC2AP_IRQ1, MODID_HISEE_EXC_SENC2AP_IRQ1},
	{HISEE_MNTN_IRQ_GROUP_LOCKUP, MODID_HISEE_EXC_LOCKUP},
	{HISEE_MNTN_IRQ_GROUP_EH2H_SLV, MODID_HISEE_EXC_EH2H_SLV},
	{HISEE_MNTN_IRQ_GROUP_TSENSOR1, MODID_HISEE_EXC_TSENSOR1},
	/*Please add your new member above!!!!*/
};
static cosimage_version_info curr_ver_mntn = {0};
static cosimage_version_info misc_version_mntn = {0};
static unsigned int hisee_lcs_mode_mntn = 0;

static u32 hisee_exception_modid;
static u32	 g_log_out_offset = 0;
static u32	 g_vote_val_lpm3 = 0;
static u32	 g_vote_val_atf = 0;
static int	 g_need_run_flag = HISEE_OK;/*flag that make sure only run this function after last running is over*/
static int	 g_rpmb_status_flag = HISEE_OK;/*whether rpmb is ok: 0->ok; !0->ko*/
static struct notifier_block hisee_ipc_block;
static struct rdr_register_module_result hisee_info;
static void *hisee_mntn_addr;
static dma_addr_t hisee_log_phy;
static void __iomem *hisee_log_addr;
static struct task_struct *hisee_mntn_thread = NULL;
static struct task_struct *hisee_mntn_print_verinfo = NULL;
static DECLARE_COMPLETION(hisee_mntn_complete);
static DECLARE_COMPLETION(hisee_pwrdebug_complete);
static struct ipc_msg g_msg;
static hisee_mntn_state	g_hisee_mntn_state = HISEE_STATE_INVALID;
static struct dsm_dev dsm_hisee = {
	.name = "dsm_hisee",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = HISEE_DMD_BUFF_SIZE,
};

static struct dsm_client *hisee_dclient = NULL;

extern int32_t hisee_exception_to_reset_rpmb(void);
/********************************************************************
Description:	Only for hisi hisee, to call directly to record an exception.
input:	data£ºdata to save
output:	NA
return:	NA
********************************************************************/
void rdr_hisee_call_to_record_exc(int data)
{
	hisee_exception_modid = (u32)MODID_SIMULATE_EXC_RPMB_KO;
	g_hisee_mntn_state = HISEE_STATE_HISEE_EXC;
	g_rpmb_status_flag = data;
	complete(&hisee_mntn_complete);
}
/********************************************************************
Description:	send msg to lpm3 by rproc
input:	u32 data0, u32 data1, u32 data2
output:
return:	NA
********************************************************************/
static int hisee_mntn_send_msg_to_lpm3(u32 *p_data, u32 len)
{
	int	ret = -1;

	if (NULL == p_data)
		return ret;

	ret = RPROC_ASYNC_SEND(HISI_RPROC_LPM3_MBX17,
		(mbox_msg_t *)p_data, (rproc_msg_len_t)len);
	if (ret != 0)
		pr_err("%s:RPROC_ASYNC_SEND failed! return 0x%x, msg:(%x)\n",
			__func__, ret, *p_data);
	return ret;
}
/********************************************************************
description:  translate the irq number to the exception type defined by kernel,
		so kernel can know what exception it is.
input: irq_no, irq number id.
output: NA
return: exception type
********************************************************************/
static unsigned int translate_exc_type(u32 irq_no)
{
	unsigned long i;
	u32 module_id = (u32)MODID_HISEE_EXC_UNKNOWN;

	for (i = 0; i < sizeof(hisee_exc_trans) /
		sizeof(hisee_exc_trans_s); i++)
		if (irq_no == hisee_exc_trans[i].irq_value) {
			module_id = hisee_exc_trans[i].module_value;
			break;
		}

	return module_id;
}

/********************************************************************
Description:	kenrel send msg to ATF
input:	NA
output:	NA
return:	NA
********************************************************************/
/*lint -e715*/
noinline u64 atfd_hisi_service_hisee_mntn_smc(u64 _function_id,
	u64 _arg0,
	u64 _arg1,
	u64 _arg2)
{
	register u64 function_id asm("x0") = _function_id;
	register u64 arg0 asm("x1") = _arg0;
	register u64 arg1 asm("x2") = _arg1;
	register u64 arg2 asm("x3") = _arg2;
	asm volatile(
		__asmeq("%0", "x0")
		__asmeq("%1", "x1")
		__asmeq("%2", "x2")
		__asmeq("%3", "x3")
		"smc    #0\n"
		: "+r" (function_id)
		: "r" (arg0), "r" (arg1), "r" (arg2));

	return (u64)function_id;
}

/********************************************************************
Description: save hisee print log data in file
input:	NA
output:	NA
return:	NA
********************************************************************/
void rdr_hisee_save_print_log(void)
{
	u32 tmpsize;
	char path[HISEE_MNTN_PATH_MAXLEN] = {0};
	char	*p_timestr = rdr_get_timestamp();
	int ret;
	hlog_header *hisee_log_head = (hlog_header *)hisee_log_addr;

	pr_err(" ====================================\n");
	pr_err(" save hisee print log now\n");
	pr_err(" ====================================\n");

	snprintf(path, (size_t)HISEE_MNTN_PATH_MAXLEN, "%s", PATH_ROOT);
	/*return if there is no free space in hisi_logs*/
	tmpsize = (u32)rdr_dir_size(path, (bool)true);
	if (tmpsize > rdr_get_logsize()) {
		pr_err("hisi_logs dir is full!!\n");
		return;
	}
	atfd_hisi_service_hisee_mntn_smc((u64)HISEE_MNTN_ID,
		(u64)HISEE_SMC_LOG_OUT, (u64)g_log_out_offset, (u64)0x0);

	snprintf(path, (size_t)HISEE_MNTN_PATH_MAXLEN, "%s%s", PATH_ROOT, HISEE_PRINTLOG_FLIENAME);

	/*save current time in hisee_printlog*/
	/*lint -e124*/
	if (NULL != p_timestr) {
		ret = mntn_filesys_write_log(path,
				(void *)(p_timestr),
				(unsigned int)strlen(p_timestr),
				HISEE_FILE_PERMISSION);
		pr_err(" save hisee print log time, time is %s, ret = %d\n", p_timestr, ret);
	}
	/* save hisee log to data/hisi_logs/time/hisee_printlog */
	ret = mntn_filesys_write_log(path,
			(void *)(hisee_log_addr + sizeof(hlog_header)),
			(unsigned int)hisee_log_head->real_size,
			HISEE_FILE_PERMISSION);
	/*lint +e124*/
	pr_err(" ====================================\n");
	pr_err(" save hisee print log end, ret = %d\n", ret);
	pr_err(" ====================================\n");
	/*lint +e124*/
}
/********************************************************************
Description: judge the current misc is new or not
input:	void *arg
output:	NA
return:	HISEE_OK, new misc; other value: old misc
********************************************************************/
int rdr_hisee_judge_new_misc(void)
{
	int ret = HISEE_OK;
	return ret;
}
/********************************************************************
Description: hisee mntn thread
input:	void *arg
output:	NA
return:	NA
********************************************************************/
int rdr_hisee_thread(void *arg)
{
	unsigned int msg[2];

	msg[0] = LPM3_HISEE_MNTN;
	msg[1] = HISEE_RESET;

	while (!kthread_should_stop()) {
		wait_for_completion(&hisee_mntn_complete);
		switch (g_hisee_mntn_state) {
		case HISEE_STATE_HISEE_EXC:
			/*Needn't call hisee_exception_to_reset_rpmb here, because it has been called if exception modid = MODID_SIMULATE_EXC_RPMB_KO*/
			if ((u32)MODID_SIMULATE_EXC_RPMB_KO != hisee_exception_modid)
				g_rpmb_status_flag = hisee_exception_to_reset_rpmb();

			hisee_mntn_print_cos_info();
			/*new req from huanghuijin: record only for new misc!!!*/
			if (HISEE_OK == rdr_hisee_judge_new_misc()) {
				g_vote_val_atf = (u32)atfd_hisi_service_hisee_mntn_smc((u64)HISEE_MNTN_ID,
					(u64)HISEE_SMC_GET_VOTE, (u64)0, (u64)0);
				pr_err("fi[0x%x] fv[0x%x] ss[0x%x] sc[0x%x] vote lpm3[0x%x] vote atf[0x%x] rpmb %d\n", g_msg.data[2], g_msg.data[3], g_msg.data[4], g_msg.data[5], g_msg.data[6], g_vote_val_atf, g_rpmb_status_flag);
				pr_err("msg.data[0] = %d msg.data[1] = %d\n", g_msg.data[0], g_msg.data[1]);
				rdr_system_error(hisee_exception_modid, 0, 0);
			} else {
				/*To tell lpm3 reset hisee, do nothing else*/
				hisee_mntn_send_msg_to_lpm3(msg, 2);
			}
			break;
		case HISEE_STATE_LOG_OUT:
			rdr_hisee_save_print_log();
			break;
		default:
			break;
		}
		g_hisee_mntn_state = HISEE_STATE_READY;
	}
	return 0;
}
/********************************************************************
Description: hisee mntn thread for printing cos ver info
input:	void *arg
output:	NA
return:	NA
********************************************************************/
int hisee_mntn_printverinfo_thread(void *arg)
{
	u32	 try_max = 0;

	while (!kthread_should_stop()) {
		if (HISEE_STATE_READY == g_hisee_mntn_state) {
			if (HISEE_OK == access_hisee_image_partition((char *)&curr_ver_mntn, SW_VERSION_READ_TYPE)
				|| try_max > HISEE_MNTN_PRINT_COS_VER_MAXTRY) {
				/*get misc version*/
				if (HISEE_OK != access_hisee_image_partition((char *)&misc_version_mntn, MISC_VERSION_READ_TYPE)) {
					pr_err("%s:fail to get misc ver\n", __func__);
				}
				hisee_mntn_print_cos_info();
	/*this pdswipe init must be put here, to make sure that is after fs ready*/
				kthread_stop(hisee_mntn_print_verinfo);
				break;
			}
			pr_err("%s:msg is NULL!\n", __func__);
			hisee_mdelay(HISEE_MNTN_PRINT_COS_VER_MS);
			try_max++;
		}
	}
	return 0;
}
/********************************************************************
Description:	receive ipc message
input:	msg£ºipc message
output:	NA
return:	NA
********************************************************************/
/*lint -e715*/
int rdr_hisee_msg_handler(struct notifier_block *nb,
	unsigned long action,
	void *msg)
{
	struct ipc_msg *p_ipcmsg;

	if (NULL == msg) {
		pr_err("%s:msg is NULL!\n", __func__);
		return 0;
	}

	p_ipcmsg = (struct ipc_msg *)msg;

	switch (p_ipcmsg->data[0]) {
	case HISEE_VOTE_RES:
		g_vote_val_lpm3 = p_ipcmsg->data[6];
		pr_err("%s:vote val from lpm3 is 0x%x!\n", __func__, g_vote_val_lpm3);

		complete(&hisee_pwrdebug_complete);
		break;
	case HISEE_LOG_OUT:
		g_log_out_offset = p_ipcmsg->data[1];
		g_hisee_mntn_state = HISEE_STATE_LOG_OUT;

		complete(&hisee_mntn_complete);
		break;
	case HISEE_EXCEPTION:
		hisee_exception_modid = p_ipcmsg->data[1] + MODID_HISEE_START;
		if (hisee_exception_modid >= (u32)MODID_HISEE_EXC_BOTTOM)
			hisee_exception_modid = (u32)MODID_HISEE_EXC_UNKNOWN;

		g_hisee_mntn_state = HISEE_STATE_HISEE_EXC;
		g_vote_val_lpm3 = p_ipcmsg->data[6];

		g_msg = *p_ipcmsg;
		complete(&hisee_mntn_complete);
		break;
	case HISEE_IRQ:
		hisee_exception_modid = translate_exc_type(p_ipcmsg->data[1]);
		g_hisee_mntn_state = HISEE_STATE_HISEE_EXC;
		g_vote_val_lpm3 = p_ipcmsg->data[6];

		g_msg = *p_ipcmsg;
		complete(&hisee_mntn_complete);
		break;
	case HISEE_TIME:
		pr_err("%s:sync time with hisee, mark value is:%d\n",
				__func__, p_ipcmsg->data[1]);
		break;
	default:
		/*nothing to do, other modules' msg*/
		/* pr_err("not ipc msg for hisee %x, %x\n", p_ipcmsg->data[0], p_ipcmsg->data[1]); */
		break;
	}

	return 0;
}
/*lint +e715*/
/********************************************************************
Description: record dmd info
input:	dmd_no, dmd no
		dmd_info, info str of dmd
output:	NA
return:	0, if ok; !0, if err
********************************************************************/
int hisee_mntn_record_dmd_info(long dmd_no, const char *dmd_info)
{
	if (dmd_no < HISEE_DMD_START || dmd_no >= HISEE_DMD_END
		|| dmd_info == NULL || NULL == hisee_dclient) {
		pr_err("%s: para error: %lx\n", __func__, dmd_no);
		return -1;
	}

	pr_err("%s: dmd no: %lx - %s", __func__, dmd_no, dmd_info);
	if (!dsm_client_ocuppy(hisee_dclient)) {
		dsm_client_record(hisee_dclient, "DMD info:%s", dmd_info);
		dsm_client_notify(hisee_dclient, dmd_no);
	}
	return 0;
}
/********************************************************************
Description: notify lpm3 reset hisee
input:	modid:module id
		etype:exception type
		coreid:core id
output:	NA
return:	NA
********************************************************************/
void rdr_hisee_reset_common(u32 modid, u32 etype, u64 coreid)
{
	unsigned int msg[2];

	msg[0] = LPM3_HISEE_MNTN;
	msg[1] = HISEE_RESET;

	pr_err(" ====================================\n");
	pr_err(" modid:          [0x%x]\n", modid);
	pr_err(" coreid:         [0x%llx]\n", coreid);
	pr_err(" exce tpye:      [0x%x]\n", etype);
	pr_err(" ====================================\n");

	hisee_mntn_send_msg_to_lpm3(msg, 2);
}

/****************************************************************************//**
 * @brief      : rdr_hisee_reset
 * @param[in]  : modid
 * @param[in]  : etype
 * @param[in]  : coreid
 * @return     : void
 * @note       :
********************************************************************************/
void rdr_hisee_reset(u32 modid, u32 etype, u64 coreid)
{
	if ((u32)MODID_SIMULATE_EXC_PD_SWIPE != hisee_exception_modid)
		rdr_hisee_reset_common(modid, etype, coreid);
	else
		pr_err("Hisee exception happed in pd swipe, so needn't reset hisee now\n");
}
/********************************************************************
Description: get the pointer to the name str of mod
input:	modid: module id
output:	NA
return:	pointer to the name str of mod
********************************************************************/
static char *hisee_mntn_get_mod_name_str(u32 modid)
{
	u32	i;
	char	*p_name = NULL;

	for (i = 0; i < (sizeof(hisee_excetption_info) / sizeof(hisee_excetption_info[0]));i++) {
		if (hisee_excetption_info[i].e_modid == modid) {
			p_name = (char *)(hisee_excetption_info[i].e_from_module);
			break;
		}
	}
	return p_name;
}
/********************************************************************
Description: save hisee log to file system
input:	modid: module id
		etype:exception type
		coreid: core id
		pathname: log path
		pfn_cb: callback function
output:	NA
return:	NA
********************************************************************/
void rdr_hisee_dump_common(u32 modid,
	u64 coreid,
	char *pathname,
	pfn_cb_dump_done pfn_cb)
{
	char path[HISEE_MNTN_PATH_MAXLEN] = {0};
	char debug_cont[HISEE_MNTN_PATH_MAXLEN] = {0};
	int ret;
	hlog_header *hisee_log_head = (hlog_header *)hisee_log_addr;
	char	*p_name_str = hisee_mntn_get_mod_name_str(modid);

	if (NULL == pathname || NULL == hisee_log_head) {
		pr_err("%s:pointer is NULL !!\n",  __func__);
		return;
	}
	atfd_hisi_service_hisee_mntn_smc((u64)HISEE_MNTN_ID,
		(u64)HISEE_SMC_GET_LOG, (u64)0x0, (u64)0x0);
	snprintf(path, (unsigned long)HISEE_MNTN_PATH_MAXLEN, "%s/%s", pathname, HISEE_LOG_FLIENAME);

	/* save hisee log to data/hisi_logs/time/hisee_log */
	/*lint -e124*/
	ret = mntn_filesys_write_log(path,
			(void *)(hisee_log_addr + sizeof(hlog_header)),
			(unsigned int)hisee_log_head->real_size,
			HISEE_FILE_PERMISSION);
	/*lint +e124*/
	if (0 == ret)
		pr_err("%s:hisee log save fail\n", __func__);

	/*lint -e124*/
	if (NULL != p_name_str) {
		ret = mntn_filesys_write_log(path,
				(void *)p_name_str,
				(unsigned int)MODULE_NAME_LEN,
				HISEE_FILE_PERMISSION);
		pr_err("mod name:      [%s]\n", p_name_str);
	}
	else {
		ret = mntn_filesys_write_log(path,
				(void *)(&modid),
				(unsigned int)sizeof(modid),
				HISEE_FILE_PERMISSION);
	}
	/*lint +e124*/
	pr_err(" ====================================\n");
	if (0 == ret)
		pr_err("%s:hisee mod id save fail\n", __func__);
	/*save fi fv ss and sv in hisee_log*/
	memset((void *)debug_cont, 0, sizeof(debug_cont));
	snprintf(debug_cont, (unsigned long)HISEE_MNTN_PATH_MAXLEN, "fi[0x%x] fv[0x%x] ss[0x%x] sc[0x%x] vote lpm3[0x%x] vote atf[0x%x] rpmb %d\n",
									g_msg.data[2], g_msg.data[3], g_msg.data[4], g_msg.data[5], g_msg.data[6], g_vote_val_atf, g_rpmb_status_flag);
	/*lint -e124*/
	ret = mntn_filesys_write_log(path,
			(void *)debug_cont,
			(unsigned int)HISEE_MNTN_PATH_MAXLEN,
			HISEE_FILE_PERMISSION);
	/*lint +e124*/
	if (0 == ret)
		pr_err("%s:fifvsssc save fail\n", __func__);

	/* save to 8M */
	memcpy(hisee_mntn_addr, hisee_log_addr, (unsigned long)hisee_info.log_len);
	memset(&g_msg, 0, sizeof(struct ipc_msg));
	if (pfn_cb)
		pfn_cb(modid, coreid);

}


/****************************************************************************//**
 * @brief      : rdr_hisee_dump
 * @param[in]  : modid
 * @param[in]  : etype
 * @param[in]  : coreid
 * @param[in]  : pathname
 * @param[in]  : pfn_cb
 * @return     : void
 * @note       :
********************************************************************************/
void rdr_hisee_dump(u32 modid,
	u32 etype,
	u64 coreid,
	char *pathname,
	pfn_cb_dump_done pfn_cb)
{
	if (NULL == pathname) {
		pr_err("%s:pointer is NULL !!\n",  __func__);
		return;
	}
	pr_err(" ====================================\n");
	pr_err(" modid:          [0x%x]\n",   modid);
	pr_err(" coreid:         [0x%llx]\n", coreid);
	pr_err(" exce tpye:      [0x%x]\n",   etype);
	pr_err(" path name:      [%s]\n",     pathname);
	rdr_hisee_dump_common(modid, coreid, pathname, pfn_cb);
}

/********************************************************************
Description:	register hisee dump and reset function
input:	NA
output:	NA
return:	NA
********************************************************************/
static int hisee_register_core(void)
{
	int ret;
	struct rdr_module_ops_pub s_soc_ops;

	s_soc_ops.ops_dump = rdr_hisee_dump;
	s_soc_ops.ops_reset = rdr_hisee_reset;
	/* register hisee core dump and reset function */
	ret = rdr_register_module_ops((u64)RDR_HISEE, &s_soc_ops, &hisee_info);
	if (ret < 0) {
		pr_err("%s:hisee core register fail, ret = [%d]\n",
			__func__, ret);
		return ret;
	};
	return 0;
}

/********************************************************************
Description:	register hisee exception
input:	hisee_einfo: hisee exception list
output:
return:	NA
********************************************************************/
static void hisee_register_exception(void)
{
	u32 ret;
	unsigned long i;

	for (i = 0; i < sizeof(hisee_excetption_info) /
		sizeof(struct rdr_exception_info_s); i++) {
		/* error return 0, ok return modid */
		ret = rdr_register_exception(&hisee_excetption_info[i]);
		if (!ret)
			pr_err("register hisee exception fail hisee_einfo[%lu]\n", i);
	}
}

/********************************************************************
Description:	hisee mntn initialization
input:	NA
output:	NA
return:	NA
********************************************************************/
static int hisee_mntn_prepare_logbuf(struct platform_device *pdev)
{
	hisee_mntn_addr = hisi_bbox_map(
		(phys_addr_t)hisee_info.log_addr, (size_t)hisee_info.log_len);
	if (NULL == hisee_mntn_addr) {
		pr_err("%s:memory map fail\n", __func__);
		return -EFAULT;
	}
	/*lint -e747 -esym(747,*)*/
	hisee_log_addr = dma_alloc_coherent(&pdev->dev,
		(size_t)hisee_info.log_len, &hisee_log_phy, GFP_KERNEL);
	/*lint -e747 +esym(747,*)*/
	if (!hisee_log_addr) {
		pr_err("%s:memory alloc fail\n", __func__);
		return -ENOMEM;
	}
	pr_info("%s : v:%pK, phy : %llx\n", __func__,
		hisee_log_addr, (u64)hisee_log_phy);
	return 0;
}

/********************************************************************
Description:	update the cos version info in local variable
input:	NA
output:	NA
return:	NA
********************************************************************/
void hisee_mntn_update_local_ver_info(void)
{
	curr_ver_mntn.magic = 0;
	if (HISEE_OK != access_hisee_image_partition((char *)&curr_ver_mntn, SW_VERSION_READ_TYPE)) {
		pr_err("%s:Fail to update ver info\n", __func__);
	} else {
		hisee_mntn_print_cos_info();
	}
}
/********************************************************************
Description:	print cos info
input:	NA
output:	NA
return:	NA
********************************************************************/
void hisee_mntn_print_cos_info(void)
{
	int	i;

	pr_err("%s:%x %llx", __func__, curr_ver_mntn.magic, (u64)curr_ver_mntn.img_timestamp.value);
	for (i = 0;i < HISEE_SUPPORT_COS_FILE_NUMBER;i++) {
		pr_err(" %d", curr_ver_mntn.img_version_num[i]);
	}
	pr_err("\n");

	/*print misc info*/
	pr_err("misc info: %s:%x %llx", __func__, misc_version_mntn.magic, (u64)misc_version_mntn.img_timestamp.value);
	for (i = 0;i < HISEE_SUPPORT_COS_FILE_NUMBER;i++) {
		pr_err(" %d", misc_version_mntn.img_version_num[i]);
	}
	pr_err("\n");
}


/********************************************************************
Description:	don't allow to power up hisee if cur phone is dm and just power up bootng
input:	NA
output:	NA
return:	HISEE_OK: allow to power; HISEE_ERROR: don't allow to power up hisee
********************************************************************/

extern bool g_hisee_is_fpga;
int hisee_mntn_can_power_up_hisee(void)
{
	int ret = HISEE_OK;

	if (g_hisee_is_fpga) {
	      return ret;
	}

	/*get the current mode*/
	if (0 == hisee_lcs_mode_mntn) {
		if (HISEE_OK != get_hisee_lcs_mode(&hisee_lcs_mode_mntn)) {
			pr_err("%s:failt to get lcs mode\n", __func__);
		}
	}
	if (HISEE_DM_MODE_MAGIC == hisee_lcs_mode_mntn) {
		if (0 == curr_ver_mntn.magic) {
			pr_err("%s:cos hasn't been upgraded\n", __func__);
			ret = HISEE_ERROR;
		}
	}
	return ret;
}
/********************************************************************
Description:	update local variable of lcs mode
input:	NA
output:	NA
return:	void
********************************************************************/
void hisee_mntn_update_local_lcsmode_val(void)
{
	hisee_lcs_mode_mntn = 0;
}
/********************************************************************
Description:	get the vote value in lpm3
input:	NA
output:	NA
return:	vote value
********************************************************************/
u32 hisee_mntn_get_vote_val_lpm3(void)
{
	return g_vote_val_lpm3;
}
/********************************************************************
Description:	get the vote value in atf
input:	NA
output:	NA
return:	vote value
********************************************************************/
u32 hisee_mntn_get_vote_val_atf(void)
{
	return g_vote_val_atf;
}

/********************************************************************
Description:	to get vote value from lpm3 and atf
input:	NA
output:	NA
return:	vote value
********************************************************************/
int hisee_mntn_collect_vote_value_cmd(void)
{
	int	ret = HISEE_OK;
	unsigned int msg[4];
	unsigned long time_left = 0;

	if (HISEE_ERROR == g_need_run_flag) {
		ret = HISEE_ERROR;
	} else {
		g_need_run_flag = HISEE_ERROR;
		msg[0] = LPM3_HISEE_MNTN;
		msg[1] = HISEE_GET_VOTE;
		hisee_mntn_send_msg_to_lpm3(msg, 2);
		g_vote_val_atf = (u32)atfd_hisi_service_hisee_mntn_smc((u64)HISEE_MNTN_ID,
		(u64)HISEE_SMC_GET_VOTE, (u64)0, (u64)0);
		/*wait for lpm3 reply*/
		time_left = wait_for_completion_timeout(&hisee_pwrdebug_complete, msecs_to_jiffies(1000));
		if (0 == time_left) {
			pr_err("%s: no time is left!!\n", __func__);
		}
		g_need_run_flag = HISEE_OK;
		pr_err("%s: vote_val from atf is 0x%x!\n", __func__, g_vote_val_atf);
	}
	return ret;
}


/********************************************************************
Description:	hisee mntn initialization
input:	NA
output:	NA
return:	NA
********************************************************************/
static int hisee_mntn_probe(struct platform_device *pdev)
{
	int ret;


	ret = of_reserved_mem_device_init(&pdev->dev);
	if (0 != ret) {
		pr_err("%s: init failed, ret.%d\n", __func__, ret);
		return ret;
	}

	/* register hisee exception */
	hisee_register_exception();

	/* register hisee dump and reset function */
	ret = hisee_register_core();
	if (ret < 0)
		return ret;

	ret = hisee_mntn_prepare_logbuf(pdev);
	if (ret < 0)
		return ret;
	/* initialization mailbox */
	hisee_ipc_block.next = NULL;
	hisee_ipc_block.notifier_call = rdr_hisee_msg_handler;
	ret = RPROC_MONITOR_REGISTER(HISI_RPROC_LPM3_MBX0, &hisee_ipc_block);
	if (ret != 0) {
		pr_err("%s:RPROC_MONITOR_REGISTER failed\n", __func__);
		return ret;
	}
	init_completion(&hisee_mntn_complete);
	hisee_mntn_thread = kthread_run(rdr_hisee_thread, NULL, "hisee_mntn");
	if (!hisee_mntn_thread)
		pr_err("create hisee mntn thread faild.\n");

	atfd_hisi_service_hisee_mntn_smc((u64)HISEE_MNTN_ID,
		(u64)HISEE_SMC_INIT, hisee_log_phy, (u64)hisee_info.log_len);

	hisee_mntn_print_verinfo = kthread_run(hisee_mntn_printverinfo_thread, NULL, "hiseeprint_mntn");
	if (!hisee_mntn_print_verinfo)
		pr_err("fail to create the thread that prints cos ver info\n");

	if (!hisee_dclient) {
		hisee_dclient = dsm_register_client(&dsm_hisee);
	}

	g_hisee_mntn_state = HISEE_STATE_READY;

	pr_err("exit %s\n", __func__);
	return 0;
}

static int hisee_mntn_remove(struct platform_device *pdev)
{
	/*lint -e747 -esym(747,*)*/
	dma_free_coherent(&pdev->dev, (size_t)hisee_info.log_len,
		&hisee_log_phy, GFP_KERNEL);
	/*lint -e747 +esym(747,*)*/
	if (NULL != hisee_mntn_thread) {
		kthread_stop(hisee_mntn_thread);
		hisee_mntn_thread = NULL;
	}
	if (NULL != hisee_mntn_print_verinfo) {
		kthread_stop(hisee_mntn_print_verinfo);
		hisee_mntn_print_verinfo = NULL;
	}
	return 0;
}
/*lint -e785*/
static const struct of_device_id hisee_mntn_match[] = {
	{.compatible = "hisee-mntn"},
	{}
};

static struct platform_driver hisee_mntn_driver = {
	.probe = hisee_mntn_probe,
	.remove = hisee_mntn_remove,
	.driver = {
		   .name = "hisee-mntn",
		   .of_match_table = hisee_mntn_match,
	},
};
/*lint +e785*/

static int __init hisee_mntn_init(void)
{
	/*lint -e64 -esym(64,*)*/
	return platform_driver_register(&hisee_mntn_driver);
	/*lint -e64 +esym(64,*)*/
}

static void __exit hisee_mntn_exit(void)
{
	platform_driver_unregister(&hisee_mntn_driver);
}
/*lint -e528 -esym(528,*)*/
module_init(hisee_mntn_init);
module_exit(hisee_mntn_exit);
/*lint -e528 +esym(528,*)*/
/*lint -e753 -esym(753,*)*/
MODULE_LICENSE("GPL");
/*lint -e753 +esym(753,*)*/

