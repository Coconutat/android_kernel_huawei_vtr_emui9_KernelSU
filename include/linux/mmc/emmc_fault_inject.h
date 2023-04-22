#ifndef _EMMC_FAULT_INJECT_H
#define _EMMC_FAULT_INJECT_H

#include <linux/mmc/host.h>
#include <linux/mmc/cmdq_hci.h>


/*macro begin*/


/*enum begin*/
enum mmcdbg_err_inject_scenario {
	ERR_INJECT_LEGACY_INTR,
	ERR_INJECT_CMDQ_INTR,
	ERR_INJECT_CMDQ_TIMEOUT,
	ERR_INJECT_MAX_ERR_SCENARIOS,
};

/*struct begin*/
struct cmdq_inject_para {
	/*member for interrupt*/
	bool in_intr_inject;
	u32 status;
	u32 err_info;
	u32 fake_dbl;

	bool need_reset;
};

struct cmdq_host;

#ifdef CONFIG_EMMC_FAULT_INJECT
extern bool g_mmc_reset_status;

void mmc_fault_inject_fs_setup(void);
void mmc_fault_inject_fs_remove(void);
bool mmcdbg_error_inject_dispatcher(struct mmc_host *mmc,
			enum mmcdbg_err_inject_scenario err_scenario,
			u32 success_value, u32 *ret_value, bool not_inj);
bool mmcdbg_cq_timeout_inj(struct mmc_request *mrq,
	enum mmcdbg_err_inject_scenario usecase);

void mmcdbg_cmdq_inj_fill_status(struct cmdq_host *cq_host,
			u32 *status);
void mmcdbg_cmdq_inj_fill_errinfo(struct cmdq_host *cq_host,
			u32 *errinfo);
void mmcdbg_cmdq_inj_fake_dbl(struct cmdq_host *cq_host,
			u32 *dbl);

bool mmcdbg_cmdq_inj_need_reset(struct mmc_host *mmc);

void mmcdbg_cmdq_inj_clear_reset(struct mmc_host *mmc);
bool mmc_is_reset(void);
#else
static inline bool mmcdbg_error_inject_dispatcher(struct mmc_host *mmc,
			enum mmcdbg_err_inject_scenario err_scenario,
			int success_value, int *ret_value, bool not_inj)
{
	return false;
}

static bool mmcdbg_cq_timeout_inj(struct mmc_request *mrq,
	enum mmcdbg_err_inject_scenario usecase)
{
	return false;
}
static inline void mmc_fault_inject_fs_setup(void)
{
}
static inline void mmc_fault_inject_fs_remove(void)
{
}
static void mmcdbg_cmdq_inj_fill_status(struct cmdq_host *cq_host,
			u32 *status)
{
}
static void mmcdbg_cmdq_inj_fill_errinfo(struct cmdq_host *cq_host,
			u32 *errinfo)
{
}
void mmcdbg_cmdq_inj_fake_dbl(struct cmdq_host *cq_host,
			u32 *dbl)
{
}


bool mmcdbg_cmdq_inj_need_reset(struct mmc_host *mmc)
{
	return false;
}

void mmcdbg_cmdq_inj_clear_reset(struct mmc_host *mmc);


#endif /* End of CONFIG_SCSI_UFS_FAULT_INJECT */

#endif /* End of Header */
