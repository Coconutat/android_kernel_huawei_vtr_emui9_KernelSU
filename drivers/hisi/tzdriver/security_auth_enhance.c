
#include <linux/sched.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/err.h>
#include <linux/crc32.h>
#include "security_auth_enhance.h"
#include "tc_ns_client.h"
#include "tc_ns_log.h"
#include "teek_client_type.h"
#include "securec.h"
#include "teek_client_constants.h"
#include "teek_client_id.h"
#include "securectype.h"
#include "gp_ops.h"

#define GLOBAL_CMD_ID_SSA		0x2DCB /**SSA cmd_id 11723**/
#define GLOBAL_CMD_ID_MT		0x2DCC /**MT cmd_id 11724**/
#define GLOBAL_CMD_ID_MT_UPDATE 0x2DCD /**MT_IPDATE cmd_id 11725**/
#define TEEC_PENDING2_AGENT		0xFFFF2001
#define UUID_PREFIX_MEANS_GLOBAL 1 /**when uuid[0] is 1, means global **/


static bool is_token_empty(uint8_t *token, uint32_t token_len)
{
	uint32_t i;

	for (i = 0; i < token_len; i++) {
		if (*(token + i))
			return false;
	}
	return true;
}

static int32_t scrambling_timestamp(void *in, void *out,
						uint32_t data_len,
						void *key,
						uint32_t key_len)
{
	uint32_t i;

	if (!in || !out || !key) {
		tloge("bad parameters, input_data is null.\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (data_len == 0 || data_len > SECUREC_MEM_MAX_LEN
				|| key_len == 0
				|| key_len > SECUREC_MEM_MAX_LEN) {
		tloge("bad parameters, data_len is %d, scrambling_len is %d\n",
					data_len, key_len);
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	for (i = 0; i < data_len; i++) {
		*((uint8_t *)out + i) =
			*((uint8_t *)in + i) ^ *((uint8_t *)key + i % key_len);
	}

	return TEEC_SUCCESS;
}

static int32_t descrambling_timestamp(uint8_t *in_token_buf,
		struct session_secure_info *secure_info, uint8_t flag) {
	uint64_t timestamp = 0;

	if ((!in_token_buf) || (!secure_info)) {
		tloge("invalid params!\n");
		return -EINVAL;
	}

	if (scrambling_timestamp(&in_token_buf[TIMESTAMP_BUFFER_INDEX],
						&timestamp,
						TIMESTAMP_LEN_DEFAULT,
						secure_info->scrambling,
						SCRAMBLING_KEY_LEN)) {
		tloge("descrambling_timestamp failed\n");
		return -EFAULT;
	}

	if (INC == flag) {
		timestamp++;
	} else if (DEC == flag) {
		timestamp--;
	} else {
		tloge("flag error !, 0x%x\n", flag);
		return -EFAULT;
	}
	tlogd("timestamp is %lld.\n", timestamp);

	if (scrambling_timestamp(&timestamp,
				&in_token_buf[TIMESTAMP_BUFFER_INDEX],
				TIMESTAMP_LEN_DEFAULT,
				secure_info->scrambling,
				SCRAMBLING_KEY_LEN)) {
		tloge("descrambling_timestamp failed\n");
		return -EFAULT;
	}

	return EOK;
}

int32_t update_timestamp(TC_NS_SMC_CMD *cmd)
{
	TC_NS_Session *session = NULL;
	struct session_secure_info *secure_info = NULL;
	unsigned char *uuid = NULL;
	uint8_t *token_buffer = NULL;
	phys_addr_t phy_addr;
	bool global;
	bool filterFlag;
	bool needCheckFlag;

	if (!cmd) {
		tloge("cmd is NULL, error!");
		return TEEC_ERROR_BAD_PARAMETERS;
	}
	/*if cmd is agent ,not check uuid. and sometime uuid canot access it */
	filterFlag = (0 != cmd->agent_id) || (TEEC_PENDING2_AGENT == cmd->ret_val);
	if (filterFlag) {
		return TEEC_SUCCESS;
	}

	phy_addr = (phys_addr_t)(cmd->uuid_h_phys) << 32 | (cmd->uuid_phys);
	if (!phy_addr)
		return TEEC_ERROR_BAD_PARAMETERS;

	uuid = phys_to_virt(phy_addr);
	if (!uuid) {
		tloge("uuid is NULL, error!\n");
		return TEEC_ERROR_GENERIC;
	}
	global = (UUID_PREFIX_MEANS_GLOBAL == uuid[0]) ? true : false;
	needCheckFlag = (!global) && (0 == cmd->agent_id) && (TEEC_PENDING2_AGENT != cmd->ret_val);
	if (needCheckFlag) {
		token_buffer = phys_to_virt((phys_addr_t)(cmd->token_h_phys) << 32
				| (cmd->token_phys));
		if (!token_buffer || is_token_empty(token_buffer, TOKEN_BUFFER_LEN)) {
			tloge("token is NULL or token is empyt, error!\n");
			return TEEC_ERROR_GENERIC;
		}

		session = tc_find_session2(cmd->dev_file_id,
						cmd->context_id, uuid+1);
		if (NULL == session) {
			tlogd("tc_find_session_key find session FAILURE.\n");
			return TEEC_ERROR_GENERIC;
		}
		secure_info = &session->secure_info;

		if (EOK != descrambling_timestamp(token_buffer, secure_info, INC)) {
			put_session_struct(session);
			tloge("update token_buffer error.\n");
			return TEEC_ERROR_GENERIC;
		}

		put_session_struct(session);
		token_buffer[SYNC_INDEX] = UN_SYNCED;
	} else {
		tlogd("global cmd or agent, do not update timestamp.");
	}

	return TEEC_SUCCESS;
}

int32_t sync_timestamp(TC_NS_SMC_CMD *cmd, uint8_t *token, unsigned char *uuid, bool global)
{
	TC_NS_Session *session = NULL;

	if (!cmd || !token || !uuid) {
		tloge("parameters is NULL, error!\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (GLOBAL_CMD_ID_OPEN_SESSION == cmd->cmd_id && global) {
		tlogd("OpenSession would not need sync timestamp.\n");
		return TEEC_SUCCESS;
	}

	if (token[SYNC_INDEX] == UN_SYNCED) {
		tlogd("flag is UN_SYNC, to sync timestamp! \n");
		session = tc_find_session2(cmd->dev_file_id,
						cmd->context_id, uuid);
		if (NULL == session) {
			tlogd("sync_timestamp find session FAILURE. \n");
			return TEEC_ERROR_GENERIC;
		}

		if (EOK != descrambling_timestamp(token, &session->secure_info, DEC)) {
			put_session_struct(session);
			tloge("sync token_buffer error.\n");
			return TEEC_ERROR_GENERIC;
		}
		put_session_struct(session);
		return TEEC_SUCCESS;
	} else if (token[SYNC_INDEX] == IS_SYNCED) {
		/*nothing to do*/
	} else {
		tloge("sync flag error! 0x%x, \n", token[SYNC_INDEX]);
	}

	return TEEC_ERROR_GENERIC;
}

/*scrambling operation and pid*/
static void scrambling_operation(TC_NS_SMC_CMD *cmd,
	uint32_t scrambler)
{
	if (!cmd)
		return;

	if (0 != cmd->operation_phys || 0 != cmd->operation_h_phys) {
		cmd->operation_phys = cmd->operation_phys ^ scrambler;
		cmd->operation_h_phys = cmd->operation_h_phys ^ scrambler;
	}

	cmd->pid = cmd->pid ^ scrambler;
	cmd->real_pid = cmd->real_pid ^ scrambler;
}

static int32_t verify_cmdbuff(TC_NS_SMC_CMD *cmd, uint32_t scrambler)
{
	uint32_t crc = 0;
	uint32_t chksum = 0;

	if (!cmd) {
		tloge("cmd is NULL, error!\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	crc = crc32(0, (const uint8_t *)cmd, CHKSUM_LENGTH);
	chksum = crc ^ scrambler;
	if (chksum != cmd->chksum) {
		tloge("verify failed, id=0x%x, 0x%x\n", cmd->cmd_id,
			cmd->chksum);
		return TEEC_ERROR_GENERIC;
	}

	return TEEC_SUCCESS;
}

static bool agent_msg(uint32_t cmd_id)
{
	bool agent = GLOBAL_CMD_ID_SSA == cmd_id
			|| GLOBAL_CMD_ID_MT == cmd_id
			|| GLOBAL_CMD_ID_MT_UPDATE == cmd_id;

	return agent;
}

/*calculate cmd checksum and scrambling operation*/
int32_t update_chksum(TC_NS_SMC_CMD *cmd)
{
	TC_NS_Session *session = NULL;
	struct session_secure_info *secure_info = NULL;
	uint8_t *uuid = NULL;
	uint32_t scrambler = 0;
	uint32_t scrambler_oper = 0;
	uint32_t crc = 0;
	bool global = false;

	if (!cmd) {
		tloge("cmd is NULL, error!\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}
	/*if cmd is agent ,not check uuid. and sometime uuid canot access it */
	if (0 != cmd->agent_id || TEEC_PENDING2_AGENT == cmd->ret_val) {
		return TEEC_SUCCESS;
	}

	cmd->chksum = 0;
	if (agent_msg(cmd->cmd_id)) {
		tlogd("SSA cmd, no need to update_chksum.\n");
		return TEEC_SUCCESS;
	}

	uuid = phys_to_virt((phys_addr_t)(cmd->uuid_h_phys) << 32
		| (cmd->uuid_phys));
	if (!uuid) {
		tloge("uuid is NULL, error!\n");
		return TEEC_ERROR_GENERIC;
	}

	global = (UUID_PREFIX_MEANS_GLOBAL == uuid[0]) ? true : false;
	/*cmd is invoke command*/
	if (!global && (0 == cmd->agent_id)
		&& (TEEC_PENDING2_AGENT != cmd->ret_val)) {
		session = tc_find_session2(cmd->dev_file_id,
			cmd->context_id, uuid + 1);
		if (session) {
			secure_info = &session->secure_info;
			scrambler = secure_info->scrambling[SCRAMBLING_CMDCRC];
			crc = crc32(0, (const uint8_t *)cmd, CHKSUM_LENGTH);
			cmd->chksum = crc ^ scrambler;
			scrambler_oper = secure_info->scrambling[SCRAMBLING_OPERATION];
			scrambling_operation(cmd, scrambler_oper);
			put_session_struct(session);
		}
	}

	return TEEC_SUCCESS;
}

int32_t verify_chksum(TC_NS_SMC_CMD *cmd)
{
	TC_NS_Session *session = NULL;
	struct session_secure_info *secure_info = NULL;
	uint8_t *uuid = NULL;
	uint32_t scrambler = 0;
	bool global = false;
	bool chk_flag = false;

	if (!cmd) {
		tloge("cmd is NULL, error!\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (agent_msg(cmd->cmd_id)) {
		tlogd("SSA cmd, no need to update_chksum.\n");
		return TEEC_SUCCESS;
	}

	uuid = phys_to_virt((phys_addr_t)(cmd->uuid_h_phys) << 32
		| (cmd->uuid_phys));
	if (!uuid) {
		tloge("uuid is NULL, error!\n");
		return TEEC_ERROR_GENERIC;
	}

	global = (UUID_PREFIX_MEANS_GLOBAL == uuid[0]) ? true : false;
	/*cmd is invoke command*/
	chk_flag = !global && GLOBAL_CMD_ID_CLOSE_SESSION != cmd->cmd_id
		&& GLOBAL_CMD_ID_KILL_TASK != cmd->cmd_id;
	if (chk_flag && (0 == cmd->agent_id)) {
		session = tc_find_session2(cmd->dev_file_id,
			cmd->context_id, uuid + 1);
		if (session) {
			secure_info = &session->secure_info;
			scrambler = secure_info->scrambling[SCRAMBLING_CMDCRC];
			put_session_struct(session);
			if (TEEC_SUCCESS != verify_cmdbuff(cmd, scrambler)) {
				tloge("verify_chksum invoke failed.\n");
				return TEEC_ERROR_GENERIC;
			}
		}
	}

	return TEEC_SUCCESS;
}
