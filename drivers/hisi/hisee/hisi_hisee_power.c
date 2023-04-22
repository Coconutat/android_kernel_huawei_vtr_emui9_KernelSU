#include <asm/compiler.h>
#include <linux/compiler.h>
#include <linux/fd.h>
#include <linux/tty.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/atomic.h>
#include <linux/notifier.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/hisi/ipc_msg.h>
#include <linux/hisi/hisi_rproc.h>
#include <linux/hisi/kirin_partition.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include "soc_acpu_baseaddr_interface.h"
#include "soc_sctrl_interface.h"
#include "hisi_hisee.h"
#include "hisi_hisee_fs.h"
#include "hisi_hisee_power.h"
#include "hisi_hisee_chip_test.h"

#define POWER_VOTE_OFF_STATUS	        0x0
#define POWER_VOTE_UNIT_MASK		0xFF
#define MAX_COUNT			0xFF
#define NO_VOTE_MAX_COUNT		0xFFFFFFFF


hisee_power_vote_status g_power_vote_status;
unsigned int g_power_vote_cnt;
unsigned int g_cos_id;

/*保存当前启动运行的cos镜像id*/
unsigned int g_runtime_cosid = COS_IMG_ID_0;

hisee_power_vote_record g_vote_record_method;
/*whether we need to pre enable clk before powering on hisee:
0->needn't ; 1->need*/
unsigned int g_power_pre_enable_clk = false;

static bool g_daemon_created = false;
static int g_unhandled_timer_cnt = 0;
static struct semaphore g_hisee_poweroff_sem;
static struct mutex g_poweron_timeout_mutex;
static struct list_head g_unhandled_timer_list;

extern int hisee_mntn_can_power_up_hisee(void);
extern u32 hisee_mntn_get_vote_val_lpm3(void);
extern u32 hisee_mntn_get_vote_val_atf(void);
extern int hisee_mntn_collect_vote_value_cmd(void);
void hisee_power_ctrl_init(void)
{
	g_vote_record_method = HISEE_POWER_VOTE_RECORD_CNT;
	g_power_vote_status.value = POWER_VOTE_OFF_STATUS;
	g_power_vote_cnt = 0;
	mutex_init(&g_poweron_timeout_mutex);
	g_unhandled_timer_cnt = 0;
	INIT_LIST_HEAD(&g_unhandled_timer_list);
}

static int hisee_powerctrl_paras_check(unsigned int vote_process, hisee_power_operation op_type, unsigned int op_cosid, int power_cmd)
{
	if ((HISEE_POWER_VOTE_RECORD_PRO == g_vote_record_method) && (vote_process >= MAX_POWER_PROCESS_ID)) {
		pr_err("%s(): vote_process=%d invalid\n",  __func__, vote_process);
		return HISEE_INVALID_PARAMS;
	}

	if (op_type < HISEE_POWER_OFF || op_type >= HISEE_POWER_MAX_OP) {
		pr_err("%s(): op_type=%x invalid\n",  __func__, op_type);
		return HISEE_INVALID_PARAMS;
	}

	if (op_cosid >= MAX_COS_IMG_ID) {
		pr_err("%s(): cosid=%d invalid\n",  __func__, op_cosid);
		return HISEE_INVALID_PARAMS;
	}

	if ((power_cmd != HISEE_POWER_CMD_ON) && (power_cmd != HISEE_POWER_CMD_OFF)) {
		pr_err("%s(): power_cmd=%d invalid\n",  __func__, power_cmd);
		return HISEE_INVALID_PARAMS;
	}

	if (((op_type != HISEE_POWER_OFF) && (power_cmd == HISEE_POWER_CMD_OFF))
		|| ((op_type == HISEE_POWER_OFF) && (power_cmd != HISEE_POWER_CMD_OFF))) {
		pr_err("%s(): power_cmd and op_type is not match.\n",  __func__);
		return HISEE_INVALID_PARAMS;
	}
	return HISEE_OK;
}

static int hisee_power_ctrl(hisee_power_operation op_type, unsigned int op_cosid, int power_cmd)
{
	int ret;
	u64 cmd_to_atf;

	/* check para */
	if ((HISEE_POWER_CMD_ON != power_cmd) && (HISEE_POWER_CMD_OFF != power_cmd)) {
		pr_err("%s() para check failed.\n", __func__);
		ret = HISEE_INVALID_PARAMS;
		goto end;
	}

	if (HISEE_POWER_CMD_ON == power_cmd) {
		cmd_to_atf = (u64)CMD_HISEE_POWER_ON;
		if (true == g_power_pre_enable_clk) {
			ret = clk_prepare_enable(g_hisee_data.hisee_clk);
			if (ret < 0) {
				pr_err("%s() clk_prepare_enable failed ret=%d.\n", __func__, ret);
				ret = HISEE_BULK_CLK_ENABLE_ERROR;
				goto end;
			}
		}
	} else {
		cmd_to_atf = (u64)CMD_HISEE_POWER_OFF;
	}

	/* send power command to atf */
	ret = atfd_hisee_smc((u64)HISEE_FN_MAIN_SERVICE_CMD, cmd_to_atf, (u64)op_type, (u64)op_cosid);
	if (HISEE_OK != ret)
		pr_err("%s(): power_cmd=%d to atf failed, ret=%d\n",  __func__, power_cmd, ret);
	else if (HISEE_POWER_CMD_OFF == power_cmd) {
		if (true == g_power_pre_enable_clk) {
			clk_disable_unprepare(g_hisee_data.hisee_clk);
		}
	}

end:
	set_errno_and_return(ret);
}

int smx_process(hisee_power_operation op_type, unsigned int op_cosid, int power_cmd)
{
	int ret = SMX_PROCESS_UNSUPPORT;
	u64 cmd_to_atf;

	/* check para */
	if ((SMX_PROCESS_STEP1_CMD != power_cmd) && (SMX_PROCESS_STEP2_CMD != power_cmd)) {
		pr_err("%s() para check failed.\n", __func__);
		return SMX_PROCESS_INVALID_PARAMS;
	}

	if (SMX_PROCESS_STEP1_CMD == power_cmd) {
		cmd_to_atf = (u64)CMD_SMX_PROCESS_STEP1;
		if (true == g_power_pre_enable_clk) {
			ret = clk_prepare_enable(g_hisee_data.hisee_clk);
			if (ret < 0) {
				pr_err("%s() clk_prepare_enable failed ret=%d.\n", __func__, ret);
				return SMX_PROCESS_CLK_ENABLE_ERROR;
			}
		}
	} else {
		cmd_to_atf = (u64)CMD_SMX_PROCESS_STEP2;
	}

	/* send power command to atf */
	ret = atfd_hisee_smc((u64)SMX_PROCESS_FN_MAIN_SERVICE_CMD, cmd_to_atf, (u64)op_type, (u64)op_cosid);
	if (SMX_PROCESS_SUPPORT_BUT_ERROR == ret) {
		pr_err("%s(): power_cmd=%d to atf failed, ret=%d\n",  __func__, power_cmd, ret);
	} else if (SMX_PROCESS_STEP2_CMD == power_cmd) {
		if (true == g_power_pre_enable_clk) {
			clk_disable_unprepare(g_hisee_data.hisee_clk);
		}
	}
	return ret;
}

static int hisee_set_power_vote_status(u32 vote_process, hisee_power_cmd power_cmd)
{
	u32 shift = 0;
	u64 current_count = 0;
	int ret = HISEE_OK;

	if (HISEE_POWER_VOTE_RECORD_PRO == g_vote_record_method) {
		/* the 3 means every vote status unit is 8 bits */
		shift = (vote_process - COS_PROCESS_WALLET) << (u32)3;
		current_count = (g_power_vote_status.value >> shift) & POWER_VOTE_UNIT_MASK;
		g_power_vote_status.value &= ~((u64)POWER_VOTE_UNIT_MASK << shift);

		if (HISEE_POWER_CMD_ON == power_cmd) {
			if (MAX_COUNT == current_count) {
				pr_err("Vote is the maximum number.\n");
				ret = HISEE_ERROR;
			} else {
				current_count++;
			}
			g_power_vote_status.value |= current_count << shift;
		} else {
			if (0 < current_count) {
				current_count--;
			} else {
				pr_err("Vote is zero already.\n");
			}
			g_power_vote_status.value |= current_count << shift;
		}
		pr_err("VoteStatus:%lx.\n",  g_power_vote_status.value);
	} else {
		if (HISEE_POWER_CMD_ON == power_cmd) {
			if (g_power_vote_cnt < NO_VOTE_MAX_COUNT)
				g_power_vote_cnt++;
			else {
				pr_err("Vote count is already max.\n");
				ret = HISEE_ERROR;
			}
		} else {
			if (g_power_vote_cnt > 0)
				g_power_vote_cnt--;
			else {
				pr_err("Vote is already zero.\n");
			}
		}

		pr_err("VoteCnt:%x.\n",  g_power_vote_cnt);
	}

	return ret;
}

hisee_power_status hisee_get_power_status(void)
{
	if (HISEE_POWER_VOTE_RECORD_PRO == g_vote_record_method) {
		if ((POWER_VOTE_OFF_STATUS != g_power_vote_status.value)) {
			return HISEE_POWER_STATUS_ON;
		}
	} else {
		if (g_power_vote_cnt > 0) {
			return HISEE_POWER_STATUS_ON;
		}
	}

	return HISEE_POWER_STATUS_OFF;
}

static int hisee_power_vote(u32 vote_process, hisee_power_operation op_type,
				unsigned int op_cosid, hisee_power_cmd power_cmd)
{
	int ret = HISEE_OK;
	hisee_power_status power_status;

	pr_err("VoteIn:%x,%x,%x,%x\n",  vote_process, op_type, op_cosid, power_cmd);
	ret = hisee_powerctrl_paras_check(vote_process, op_type, op_cosid, power_cmd);
	if (HISEE_OK != ret) {
		pr_err("%s(): para check failed.\n",  __func__);
		set_errno_and_return(HISEE_INVALID_PARAMS);
	}

	power_status = hisee_get_power_status();

	if ((HISEE_POWER_STATUS_OFF == power_status) &&
		(HISEE_POWER_CMD_ON == power_cmd)) {
		/* from the off status to on status */
		ret = hisee_power_ctrl(op_type, op_cosid, power_cmd);
		if (HISEE_OK != ret) {
			pr_err("%s(): hisee power on failed.\n",  __func__);
			goto exit;
		}
			ret = hisee_set_power_vote_status(vote_process, power_cmd);
			if (HISEE_OK != ret) {
				pr_err("%s(): status is off, hisee power on vote failed.\n",  __func__);
			goto exit;
		}
	} else if ((HISEE_POWER_STATUS_ON == power_status) && /*lint !e456*/
		(HISEE_POWER_CMD_ON == power_cmd)) {
		/* hisee is on, so just vote */
		ret = hisee_set_power_vote_status(vote_process, power_cmd);
		if (HISEE_OK != ret) {
			pr_err("%s(): status is on, hisee power on vote failed.\n",  __func__);
			goto exit;
		}
	} else if ((HISEE_POWER_STATUS_ON == power_status) &&
		(HISEE_POWER_CMD_OFF == power_cmd)) {
		/* vote then check is the vote_status if off,
		 * if the status is off, then power_off the hisee.
		 */
		ret = hisee_set_power_vote_status(vote_process, power_cmd);
		if (HISEE_OK != ret) {
			pr_err("%s(): status is on, hisee power off vote failed.\n",  __func__);
			goto exit;
		}
		power_status = hisee_get_power_status();
		if (HISEE_POWER_STATUS_OFF == power_status) {
			ret = hisee_power_ctrl(op_type, op_cosid, power_cmd);
			if (HISEE_OK != ret) {
				/* recover the vote if power off failed. */
				ret = hisee_set_power_vote_status(vote_process, HISEE_POWER_CMD_ON);
				if (HISEE_OK != ret) {
					pr_err("%s(): hisee power off failed, vote the on status failed.\n",  __func__);
				}
				pr_err("%s(): hisee power off failed.\n",  __func__);
				goto exit;
			}
		}
	} else if ((HISEE_POWER_STATUS_OFF == power_status) &&
		(HISEE_POWER_CMD_OFF == power_cmd)) {
		/* hisee is already off */
		pr_err("%s(): hisee is already off.\n",  __func__);
		ret = HISEE_OK;
	} else {
		pr_err("%s(): input power status(0x%x) or cmd(0x%x) error.\n",  __func__, power_status, power_cmd);
		ret = HISEE_INVALID_PARAMS;
	}

exit:
	set_errno_and_return(ret);/*lint !e454 !e456*/
}/*lint !e454 !e456*/

/* the process_id can only be 0-3 */
int hisee_get_cosid_processid(void *buf, unsigned int *cos_id, unsigned int *process_id)
{
	int ret = HISEE_OK;
	unsigned char *arg_vector = (unsigned char *)buf;

	if (NULL == buf) {
		/* if no args, use default imgid 0. */
		*cos_id = COS_IMG_ID_0;
		*process_id = COS_PROCESS_CHIP_TEST;
	} else if (HISEE_CHAR_NEWLINE == *(char *)buf || '\0' == *(char *)buf) {
		/* if no args, use default imgid 0. */
		*cos_id = COS_IMG_ID_0;
		*process_id = COS_PROCESS_UNKNOWN;
	/* parameter's length must be more than or equal to 3: one blank, one cos id and one process id. */
	} else if (3 <= (strlen((char *)arg_vector)) && (HISEE_CHAR_SPACE == arg_vector[0]) &&
		('0' <= arg_vector[1]) && ('0' + MAX_COS_IMG_ID > arg_vector[1]) &&
		('0' <= arg_vector[2]) && ('0' + MAX_POWER_PROCESS_ID > arg_vector[2])) {
		*cos_id = arg_vector[1] - '0';
		*process_id = arg_vector[2] - '0';
		if (COS_PROCESS_UNKNOWN == *process_id) {
			pr_err("%s(): input process_id is unknow.", __func__);
			ret = HISEE_INVALID_PARAMS;
		}
	} else {
		pr_err("%s(): input cos_id process_id error", __func__);
		ret = HISEE_INVALID_PARAMS;
	}

	pr_info("hisee:%s():cos_id is %d\n", __func__, *cos_id);
	return ret;
}

int hisee_poweron_booting_func(void *buf, int para)
{
	int ret;
	unsigned int cos_id = COS_IMG_ID_0;
	unsigned int process_id = 0;

	mutex_lock(&g_hisee_data.hisee_mutex);

	ret = hisee_get_cosid_processid(buf, &cos_id, &process_id);
	if (HISEE_OK != ret) {
		pr_err("%s() hisee_get_cosid failed ret=%d\n", __func__, ret);
		goto end;
	}
	/*don't power up hisee, if current is dm mode and cos has not been upgraded,
	 *of there will be many hisee exception log reporting to apr.
	 *COS_FLASH is the specific cos_flash image, bypass the judgement*/
	if (HISEE_ERROR == hisee_mntn_can_power_up_hisee()) {
		ret = HISEE_ERROR;
		goto end;
	}

	if ((HISEE_POWER_VOTE_RECORD_PRO == g_vote_record_method)
				&& ((MAX_POWER_PROCESS_ID <= process_id) || (COS_PROCESS_TIMEOUT == process_id))) {
		ret = HISEE_INVALID_PARAMS;
		pr_err("%s() process_id: %u error ret=%d\n", __func__, process_id, ret);
		goto end;
	}

	if (HISEE_POWER_ON_BOOTING_MISC != para)
		para = HISEE_POWER_ON_BOOTING;

	ret = hisee_power_vote(process_id, (hisee_power_operation)para, cos_id, HISEE_POWER_CMD_ON);
	if (HISEE_OK != ret) {
		pr_err("%s() hisee_power_vote failed ret=%d\n", __func__, ret);
		goto end;
	}
	/*record the current cosid in booting phase*/
	g_runtime_cosid = cos_id;

end:
	mutex_unlock(&g_hisee_data.hisee_mutex);
	check_and_print_result_with_cosid();
	set_errno_and_return(ret);
}/*lint !e715*/

int hisee_poweron_upgrade_func(void *buf, int para)
{
	int ret;
	unsigned int hisee_lcs_mode = 0;
	unsigned int cos_id = COS_IMG_ID_0;
	unsigned int process_id = 0;

	ret = get_hisee_lcs_mode(&hisee_lcs_mode);
	if (HISEE_OK != ret) {
		pr_err("%s() get_hisee_lcs_mode failed,ret=%d\n", __func__, ret);
		set_errno_and_return(ret);
	}

	if (HISEE_SM_MODE_MAGIC == hisee_lcs_mode)
	{
		para = HISEE_POWER_ON_UPGRADE_SM;
	} else {
		para = HISEE_POWER_ON_UPGRADE;
	}
	mutex_lock(&g_hisee_data.hisee_mutex);

	ret = hisee_get_cosid_processid(buf, &cos_id, &process_id);
	if (HISEE_OK != ret) {
		pr_err("%s() hisee_get_cosid failed ret=%d\n", __func__, ret);
		goto end;
	}

	if ((HISEE_POWER_VOTE_RECORD_PRO == g_vote_record_method)
				&& (COS_PROCESS_UPGRADE != process_id)
				&& (COS_PROCESS_UNKNOWN != process_id)) {
		ret = HISEE_INVALID_PARAMS;
		pr_err("%s() process_id error ret=%d\n", __func__, ret);
		goto end;
	}

	/* To fit chiptest, change unkonw process id to upgrade id. */
	process_id = COS_PROCESS_UPGRADE;

	/*Record the cosid for check usr mismatch operation.*/
	g_cos_id = cos_id;

	ret = hisee_power_vote(process_id, (hisee_power_operation)para, cos_id, HISEE_POWER_CMD_ON);
	if (HISEE_OK != ret) {
		pr_err("%s() hisee_power_vote failed ret=%d\n", __func__, ret);
		goto end;
	}

end:
	mutex_unlock(&g_hisee_data.hisee_mutex);
	check_and_print_result_with_cosid();
	set_errno_and_return(ret);
}/*lint !e715*/

int hisee_poweroff_func(void *buf, int para)
{
	int ret;
	unsigned int cos_id = COS_IMG_ID_0;
	unsigned int process_id = 0;

	mutex_lock(&g_hisee_data.hisee_mutex);

	ret = hisee_get_cosid_processid(buf, &cos_id,&process_id);
	if (HISEE_OK != ret) {
		pr_err("%s() hisee_get_cosid failed ret=%d\n", __func__, ret);
		goto end;
	}
	if ((HISEE_POWER_VOTE_RECORD_PRO == g_vote_record_method)
				&& ((MAX_POWER_PROCESS_ID <= process_id) || (COS_PROCESS_TIMEOUT == process_id))) {
		ret = HISEE_INVALID_PARAMS;
		pr_err("%s() process_id error ret=%d\n", __func__, ret);
		goto end;
	}


	ret = hisee_power_vote(process_id, HISEE_POWER_OFF, cos_id, HISEE_POWER_CMD_OFF);
	if (HISEE_OK != ret) {
		pr_err("%s() hisee_power_vote failed ret=%d\n", __func__, ret);
		goto end;
	}

end:
	mutex_unlock(&g_hisee_data.hisee_mutex);
	check_and_print_result_with_cosid();
	set_errno_and_return(ret);/*lint !e1058*/
}/*lint !e715*/

static int hisee_poweroff_daemon_body(void *arg)
{
	int ret;
	timer_entry_list *cursor = NULL, *next = NULL;

	for (;;) {
		if (down_timeout(&g_hisee_poweroff_sem, (long)HISEE_THREAD_WAIT_TIMEOUT)) {
			mutex_lock(&g_poweron_timeout_mutex);
			if (0 == g_unhandled_timer_cnt) {
				/* exit this thread if wait sema timeout and theres no timer to be handled */
				g_daemon_created = false;
				mutex_unlock(&g_poweron_timeout_mutex);
				return 0;
			} else {
				mutex_unlock(&g_poweron_timeout_mutex);
				continue;
			}
		}
		/* got the sema */
		mutex_lock(&g_poweron_timeout_mutex);
		if (g_unhandled_timer_cnt > 0) {
			g_unhandled_timer_cnt--;
			if (0 == g_unhandled_timer_cnt) {
				unsigned int process_id = COS_PROCESS_TIMEOUT;
				unsigned int cos_id = HISEE_DEFAULT_COSID;	/* the default cos */
				ret = hisee_power_vote(process_id, HISEE_POWER_OFF,
				cos_id, HISEE_POWER_CMD_OFF);
				if (HISEE_OK != ret)
					pr_err("%s  hisee timeout poweroff failed, ret=%d\n", __func__, ret);
				else
					pr_err("%s  hisee timeout poweroff success!\n", __func__);
			}
		}
		/*lint -e{613,529,438,64,826}*/
		list_for_each_entry_safe(cursor, next, &g_unhandled_timer_list, list) {
			if (atomic_read(&(cursor->handled))) {
				list_del(&(cursor->list));
				kfree(cursor);
			}
		}
		mutex_unlock(&g_poweron_timeout_mutex);
	}
}/*lint !e715*/

static int create_hisee_poweroff_daemon(void)
{
	struct task_struct *hisee_poweroff_daemon;

	/* create semaphore for daemon to wait poweroff signal */
	sema_init(&g_hisee_poweroff_sem, 0);

	hisee_poweroff_daemon = kthread_run(hisee_poweroff_daemon_body, NULL, "hisee_poweroff_daemon");
	if (IS_ERR(hisee_poweroff_daemon)) {
		pr_err("hisee err create hisee_poweroff_daemon failed\n");
		return HISEE_THREAD_CREATE_ERROR;
	}

	g_daemon_created = true;

	return HISEE_OK;
}

static void poweroff_handle(unsigned long arg)
{
	timer_entry_list *p_timer_entry = (timer_entry_list *)arg;

	atomic_set(&(p_timer_entry->handled), 1);/*lint !e1058*/

	up(&g_hisee_poweroff_sem);

	return;
}

static int parse_arg_get_id(void *buf, unsigned int *id)
{
	char *p = (char *)buf;

	if ((NULL == buf) || (NULL == id))
		return HISEE_INVALID_PARAMS;

	while (' ' == *p) p++;/* bypass blank */
	if (('\0' == *p) || ('\n' == *p)) {
		*id = NFC_SERVICE;
		return HISEE_OK;
	} else if (('0' + NFC_SERVICE <= *p) && ('0' + MAX_TIMEOUT_ID > *p)) {
		*id = *p - '0';
		return HISEE_OK;
	} else {
		pr_err("%s(): input timeout id : %c error\n", __func__, *p);
		return HISEE_INVALID_PARAMS;
	}
}

static int parse_arg_get_timeout(void *buf, int para, unsigned int *time, unsigned int *id)
{
	char *p = (char *)buf;
	char *cmd = p;
	char timeout[TIMEOUT_MAX_LEN + 1] = {0};/* 1bit terminated char */
	unsigned int i = 0;
	int ret = HISEE_OK;

	/* interface for direct call */
	if ((NULL == buf) || (NULL == time) || (NULL == id)) {
		if (para <= 0) {
		      return HISEE_INVALID_PARAMS;
		}
		return para;
	}
	/* called through powerctrl_cmd */
	while ('\0' != *p && ' ' != *p) {
		p++;/* bypass cmd name. */
	}
	if ('\0' == *p)
	      return HISEE_INVALID_PARAMS;
	while (' ' == *p) {
		p++;/* bypass blank */
	}
	cmd = p;
	/* extract timeout value */
	while (('\n' != *p) && (' ' != *p) &&
		('\0' != *p) && (i <= TIMEOUT_MAX_LEN)) {
		timeout[i++] = *p++;
	}
	if (TIMEOUT_MAX_LEN < i) {
		pr_err("Timeout value overflow:%s\n", cmd);
		return HISEE_INVALID_PARAMS;
	}
	/* if there is other para(id), there will be a blank */
	if (' ' == *p) {
		timeout[i] = '\0';
		if (kstrtouint(timeout, 0, time)) {
			return HISEE_INVALID_PARAMS;
		}
		ret = parse_arg_get_id(p, id);
	} else {
		if (kstrtouint(cmd, 0, time)) {/*its ok that cmd end with new line*/
			return HISEE_INVALID_PARAMS;
		}
		*id = NFC_SERVICE;
	}
	return ret;
}

/* poweron hisee and add a timer to poweroff hisee _msecs_ ms later */
int hisee_poweron_timeout_func(void *buf, int para)
{
	int ret = HISEE_OK;
	struct timer_list *p_timer;
	timer_entry_list *p_timer_entry;
	unsigned int msecs = 0;
	unsigned int id = 0;
	ret = parse_arg_get_timeout(buf, para, &msecs, &id);
	if (HISEE_OK != ret) {
		pr_err("%s()  invalid para, timeout not specified or not larger than 0\n", __func__);
		set_errno_and_return(HISEE_INVALID_PARAMS);/*lint !e1058*/
	}

	mutex_lock(&g_poweron_timeout_mutex);

	if (!g_daemon_created) {
		ret = create_hisee_poweroff_daemon();
		if (HISEE_OK != ret)
			goto end;
	}

	p_timer_entry = (timer_entry_list *)kzalloc(sizeof(timer_entry_list), GFP_KERNEL);
	if (NULL == p_timer_entry) {
		pr_err("%s()  timer kmalloc failed\n", __func__);
		ret = HISEE_NO_RESOURCES;
		goto end;
	}
	atomic_set(&(p_timer_entry->handled), 0);/*lint !e1058*/

	p_timer = &(p_timer_entry->timer);
	init_timer(p_timer);
	p_timer->function = poweroff_handle;
	p_timer->data     = (unsigned long)p_timer_entry;
	p_timer->expires  = jiffies + msecs_to_jiffies(msecs) + 1; /*+1 makes timeout >= msecs*/

	if (0 == g_unhandled_timer_cnt) {
		unsigned int process_id = COS_PROCESS_TIMEOUT;
		unsigned int cos_id = HISEE_DEFAULT_COSID;/* the default cos */
		ret = hisee_power_vote(process_id, HISEE_POWER_ON_BOOTING,
				cos_id, HISEE_POWER_CMD_ON);
		if (HISEE_OK != ret) {
			int ret_tmp;
			ret_tmp = hisee_power_vote(process_id, HISEE_POWER_OFF,
				cos_id, HISEE_POWER_CMD_OFF);

			kfree(p_timer_entry);
			pr_err("%s()  hisee poweron booting failed, ret=%d. abort poweron_timeout\n", __func__, ret);
			if (HISEE_OK != ret_tmp) pr_err("%s()  also poweroff failed, ret=%d\n", __func__, ret_tmp);
			goto end;
		}
			/*record the current cosid in booting phase*/
		g_runtime_cosid = cos_id;
	}

	add_timer(p_timer);
	list_add(&(p_timer_entry->list), &g_unhandled_timer_list);
	g_unhandled_timer_cnt++;
end:
	pr_err("%s():id is %u!\n", __func__, id);
	mutex_unlock(&g_poweron_timeout_mutex);
	set_errno_and_return(ret);/*lint !e1058*/
}/*lint !e715*/

int hisee_suspend(struct platform_device *pdev, struct pm_message state)
{
	timer_entry_list *cursor = NULL, *next = NULL;
	pr_err("hisi_hisee_suspend: +\n");
	mutex_lock(&g_poweron_timeout_mutex);

	/*lint -e{64,826,838} */
	list_for_each_entry_safe(cursor, next, &g_unhandled_timer_list, list) {
		list_del(&(cursor->list));
		del_timer_sync(&(cursor->timer));
		kfree(cursor);
	}

	sema_init(&g_hisee_poweroff_sem, 0);
	g_unhandled_timer_cnt = 0;

	mutex_unlock(&g_poweron_timeout_mutex);

	mutex_lock(&g_hisee_data.hisee_mutex);

	pr_err("hisi_hisee_suspend: %lx, vote_cnt = %x\n",
		g_power_vote_status.value, g_power_vote_cnt);

	if ((g_power_vote_cnt > 0) || (POWER_VOTE_OFF_STATUS != g_power_vote_status.value)) {
		g_power_vote_status.value = POWER_VOTE_OFF_STATUS;
		g_power_vote_cnt = 0;
		if (HISEE_OK != hisee_power_ctrl(HISEE_POWER_OFF, 0, HISEE_POWER_CMD_OFF))
			pr_err("hisi_hisee_suspend: power_off failed\n");
	}

	mutex_unlock(&g_hisee_data.hisee_mutex);

	pr_err("hisi_hisee_suspend: -\n");
	return HISEE_OK;
}/*lint !e715*/

int wait_hisee_ready(hisee_state ready_state, unsigned int timeout_ms)
{
	hisee_state state = HISEE_STATE_MAX;
	unsigned int unit = 20;
	unsigned int cnt;

	timeout_ms = timeout_ms < unit ? unit : timeout_ms;
	cnt = timeout_ms / unit;
	do {
		state = (hisee_state)atfd_hisee_smc((u64)HISEE_FN_MAIN_SERVICE_CMD, (u64)CMD_GET_STATE, (u64)0, (u64)0);
		if (ready_state == state) {
			if (HISEE_STATE_COS_READY == ready_state) /*add log for check*/
				pr_err("%s cost about %dms\n", __func__, timeout_ms - cnt*unit);
			return HISEE_OK;
		}
		hisee_mdelay(unit);
		cnt--;
	} while (cnt);

	pr_err("%s fail, ready state is %d, timeout is %d ms!\n",
            __func__, ready_state, timeout_ms);
	return HISEE_WAIT_READY_TIMEOUT;
}


/** check whether the hisee is ready
 * @buf: output, return the hisee ready status
 */
ssize_t hisee_check_ready_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	hisee_state state;
	int ret;
	u32	vote_lpm3;
	u32	vote_atf;

	if (NULL == buf) {
		pr_err("%s buf paramters is null\n", __func__);
		set_errno_and_return(HISEE_INVALID_PARAMS);/*lint !e1058*/
	}
	state = (hisee_state)atfd_hisee_smc((u64)HISEE_FN_MAIN_SERVICE_CMD, (u64)CMD_GET_STATE, (u64)0, (u64)0);
	if (HISEE_STATE_COS_READY == state) {
		snprintf(buf, (u64)3, "%d,", 0);
		strncat(buf, "cos ready", (unsigned long)strlen("cos ready"));
	} else if (HISEE_STATE_POWER_DOWN == state
				|| HISEE_STATE_POWER_UP == state
				|| HISEE_STATE_MISC_READY == state
				|| HISEE_STATE_POWER_DOWN_DOING == state
				|| HISEE_STATE_POWER_UP_DOING == state) {
		snprintf(buf, (u64)3, "%d,", 1);
		strncat(buf, "cos unready", (unsigned long)strlen("cos unready"));
	} else {
		snprintf(buf, (u64)4, "%d,", -1);
		strncat(buf, "failed", (unsigned long)strlen("failed"));
	}
	if (HISEE_STATE_COS_READY != state) {
		hisee_mntn_collect_vote_value_cmd();
		vote_lpm3 = hisee_mntn_get_vote_val_lpm3();
		vote_atf = hisee_mntn_get_vote_val_atf();
		pr_err("%s(): votes:lpm3 0x%08x atf 0x%08x kernel 0x%lx\n", __func__, vote_lpm3, vote_atf, g_power_vote_status.value);
	}
	pr_err("%s(): state=%d, %s\n", __func__, (int)state, buf);
	return (ssize_t)strlen(buf);
}/*lint !e715*/




