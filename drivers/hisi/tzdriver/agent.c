/*******************************************************************************
* All rights reserved, Copyright (C) huawei LIMITED 2012
*------------------------------------------------------------------------------
* File Name   : tc_client_driver.c
* Description :
* Platform	  :
* Author	  : qiqingchao
* Version	  : V1.0
* Date		  : 2012.12.10
* Notes	:
*
*------------------------------------------------------------------------------
* Modifications:
*	Date		Author			Modifications
*******************************************************************************/
/*******************************************************************************
 * This source code has been made available to you by HUAWEI on an
 * AS-IS basis. Anyone receiving this source code is licensed under HUAWEI
 * copyrights to use it in any way he or she deems fit, including copying it,
 * modifying it, compiling it, and redistributing it either with or without
 * modifications. Any person who transfers this source code or any derivative
 * work must include the HUAWEI copyright notice and this paragraph in
 * the transferred software.
*******************************************************************************/

#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/module.h>
#include <linux/atomic.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "teek_client_constants.h"
#include "teek_ns_client.h"
#include "smc.h"
#include "agent.h"
#include "mem.h"
#include "tui.h"
#include "securec.h"
#include "tc_ns_log.h"
#include "mailbox_mempool.h"

#define HASH_FILE_MAX_SIZE 8192
#define AGENT_BUFF_SIZE (4*1024)
#define AGENT_MAX	32
#define MAX_PATH_SIZE 512

static struct list_head tee_agent_list;

struct __agent_control {
	spinlock_t lock;
	struct list_head agent_list;
};
static struct __agent_control agent_control;

#define TEE_SECE_AGENT_ID   0x53656345 //npu agent id
#define TEE_FACE_AGENT1_ID   0x46616365 //face agent id
#define TEE_FACE_AGENT2_ID   0x46616345 //face agent id
#define SYSTEM_UID 1000

typedef struct ca_info {
	char path[MAX_PATH_SIZE];
	uint32_t uid;
	uint32_t agent_id;
} CA_INFO;
CA_INFO allowed_ext_agent_ca[] = {
	{
		"/vendor/bin/hiaiserver",
		1000,
		TEE_SECE_AGENT_ID,
	},
	{
		"/vendor/bin/hw/vendor.huawei.hardware.biometrics.hwfacerecognize@1.1-service",
		1000,
		TEE_FACE_AGENT1_ID,
	},
	{
		"/vendor/bin/hw/vendor.huawei.hardware.biometrics.hwfacerecognize@1.1-service",
		1000,
		TEE_FACE_AGENT2_ID,
	},
};

int is_allowed_agent_ca(CA_INFO *ca, bool check_agent_id_flag)
{
	uint32_t i;
	CA_INFO *tmp_ca = allowed_ext_agent_ca;
	if (!check_agent_id_flag) {
		for(i = 0; i < sizeof(allowed_ext_agent_ca)/sizeof(CA_INFO); i++) {
			if ((0 == memcmp(ca->path, tmp_ca->path, MAX_PATH_SIZE)) && (ca->uid == tmp_ca->uid))
			{
				return 0;
			}
			tmp_ca++;
		}
	} else {
		for(i = 0; i < sizeof(allowed_ext_agent_ca)/sizeof(CA_INFO); i++) {
			if ((0 == memcmp(ca->path, tmp_ca->path, MAX_PATH_SIZE))
					&& (ca->uid == tmp_ca->uid)
					&& (ca->agent_id == tmp_ca->agent_id))
			{
				return 0;
			}
			tmp_ca++;
		}
	}
	tlogd("ca-uid is %u, ca_path is %s, agent id is %x\n", ca->uid, ca->path, ca->agent_id);
	return -1;
}

static int get_ca_path_and_uid(struct task_struct *ca_task, CA_INFO *ca)
{
	char *path;
	const struct cred *cred = NULL;
	int message_size;
	int ret = 0;
	char *tpath;

	if (NULL == ca_task || NULL == ca) {
		TCERR("task info is NULL\n");
		return -EPERM;
	}

	cred = get_task_cred(ca_task); /*lint !e838 */
	if (NULL == cred) {
		TCERR("cred is NULL\n");
		return -EPERM;

	}

	tpath = kmalloc(MAX_PATH_SIZE, GFP_KERNEL);
	if (NULL == tpath) {
		TCERR("tpath kmalloc fail\n");
		put_cred(cred);
		return -EPERM;
	}

	path = get_process_path(ca_task, tpath);
	if (IS_ERR_OR_NULL(path)) {
		ret = -ENOMEM;
		goto end;
	}

	message_size = snprintf_s(ca->path, MAX_PATH_SIZE - 1,
			MAX_PATH_SIZE - 1, "%s", path);
	ca->uid = cred->uid.val;
	tlogd("ca_task->comm is %s, path is %s, ca uid is %u\n", ca_task->comm, path, cred->uid.val);

	if (message_size > 0)
		ret = 0;
	else
		ret = -1;

end:
	kfree(tpath);
	put_cred(cred);
	return ret;
}

int check_ext_agent_access(struct task_struct *ca_task)
{
	int ret = 0;
	CA_INFO agent_ca = {"", 0, 0};

	ret = get_ca_path_and_uid(ca_task, &agent_ca);
	if (ret) {
		tloge("get cp path or uid failed\n");
		return ret;
	}

	ret = is_allowed_agent_ca(&agent_ca, 0);
	return ret;
}

int check_ext_agent_access_with_agent_id(struct task_struct *ca_task, uint32_t agent_id)
{
	int ret = 0;
	CA_INFO agent_ca = {"", 0, 0};

	ret = get_ca_path_and_uid(ca_task, &agent_ca);
	if (ret) {
		tloge("get cp path or uid failed\n");
		return ret;
	}
	agent_ca.agent_id = agent_id;
	ret = is_allowed_agent_ca(&agent_ca, 1);
	return ret;
}

int TC_NS_set_nativeCA_hash(unsigned long arg)
{
	int ret = 0;
	TC_NS_SMC_CMD smc_cmd = { 0 };
	uint8_t *inbuf = (uint8_t *)arg;
	uint32_t buflen = 0;
	uint8_t *buftotee = NULL;
	struct mb_cmd_pack *mb_pack;

	if (NULL == inbuf)
		return -1;

	if (TC_NS_get_uid() != 0) {
		tloge("It is a fake tee agent\n");
		return -EACCES;
	}

	if (copy_from_user(&buflen, inbuf, sizeof(buflen))) {
		tloge("copy from user failed\n");
		return -EFAULT;
	}
	if (buflen > HASH_FILE_MAX_SIZE) {
		tloge("ERROR: file size[0x%x] too big\n", buflen);
		return -1;
	}

	buftotee = mailbox_alloc(buflen, 0);
	if (!buftotee) {
		tloge("failed to alloc memory!\n");
		return -1; /*lint !e429 */
	}

	if (copy_from_user(buftotee, inbuf, buflen)) { /*lint !e613 !e668 */
		tloge("copy from user failed\n");
		mailbox_free(buftotee);
		return -EFAULT;
	}

	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack) {
		tloge("alloc cmd pack failed\n");
		mailbox_free(buftotee);
		return -ENOMEM;
	}

	mb_pack->operation.paramTypes = TEE_PARAM_TYPE_VALUE_INPUT |
				(TEE_PARAM_TYPE_VALUE_INPUT << 4);
	mb_pack->operation.params[0].value.a = (unsigned int)virt_to_phys(buftotee);
	mb_pack->operation.params[0].value.b = (unsigned int)(virt_to_phys(buftotee) >> 32); /*lint !e572*/
	mb_pack->operation.params[1].value.a = buflen;

	mb_pack->uuid[0] = 1;
	smc_cmd.uuid_phys = virt_to_phys(mb_pack->uuid);
	smc_cmd.uuid_h_phys = virt_to_phys(mb_pack->uuid) >> 32; /*lint !e572*/
	smc_cmd.cmd_id = GLOBAL_CMD_ID_SET_CA_HASH;
	smc_cmd.operation_phys = virt_to_phys(&mb_pack->operation);
	smc_cmd.operation_h_phys = virt_to_phys(&mb_pack->operation) >> 32; /*lint !e572*/

	ret = TC_NS_SMC(&smc_cmd, 0);

	mailbox_free(buftotee);
	mailbox_free(mb_pack);

	return ret;
}


void TC_NS_send_event_response_all(void)
{
	struct __smc_event_data *event_data = NULL;

	if (TC_NS_send_event_response(AGENT_FS_ID))
		tloge("failed to send response to AGENT_FS_ID\n");
	event_data = find_event_control(AGENT_FS_ID);
	if (event_data) {
		event_data->agent_alive = 0;
		put_agent_event(event_data);
	}
	if (TC_NS_send_event_response(AGENT_MISC_ID))
		tloge("failed to send response to AGENT_MISC_ID\n");
	event_data = find_event_control(AGENT_MISC_ID);
	if (event_data) {
		event_data->agent_alive = 0;
		put_agent_event(event_data);
	}
	if (TC_NS_send_event_response(AGENT_SOCKET_ID))
		tloge("failed to send response to AGENT_SOCKET_ID\n");
	event_data = find_event_control(AGENT_SOCKET_ID);
	if (event_data) {
		event_data->agent_alive = 0;
		put_agent_event(event_data);
	}

}


struct __smc_event_data *find_event_control(unsigned int agent_id)
{
	struct __smc_event_data *event_data = NULL, *tmp_data = NULL;
	unsigned long flags;

	spin_lock_irqsave(&agent_control.lock, flags);
	list_for_each_entry(event_data, &agent_control.agent_list, head) {/*lint !e64 !e826*/
		if (event_data->agent_id == agent_id) {
			tmp_data = event_data;
			get_agent_event(event_data);
			break;
		}
	}
	spin_unlock_irqrestore(&agent_control.lock, flags);

	return tmp_data;
}
static void free_event_control(unsigned int agent_id)
{
	struct __smc_event_data *event_data = NULL;
	struct __smc_event_data *tmp_event = NULL;
	unsigned long flags;
	spin_lock_irqsave(&agent_control.lock, flags);
	list_for_each_entry_safe(event_data, tmp_event,
							&agent_control.agent_list, head) {
		if (event_data->agent_id == agent_id) {
			list_del(&event_data->head);
			break;
		}
	}
	spin_unlock_irqrestore(&agent_control.lock, flags);
	if (event_data) {
		/* Release the share memory obtained in TC_NS_register_agent() */
		put_sharemem_struct(event_data->buffer);
		put_agent_event(event_data);
	}
}


int agent_process_work(TC_NS_SMC_CMD *smc_cmd, unsigned int agent_id)
{
	struct __smc_event_data *event_data;
	int ret = TEEC_SUCCESS;

	if (NULL == smc_cmd) {
		/*TODO: if return, the pending task in S can't be resumed!! */
		tloge("agent %u not exist\n", agent_id);
		return TEEC_ERROR_GENERIC;
	}
	tlogd("agent_id=0x%x\n", agent_id);
	/* TODO: needs lock */
	event_data = find_event_control(agent_id);
	if ( NULL == event_data || 0 == event_data->agent_alive) {
		/*TODO: if return, the pending task in S can't be resumed!! */
		tloge("agent %u not exist\n", agent_id);
		put_agent_event(event_data);
		return TEEC_ERROR_GENERIC;/*lint !e570*/
	}
	tlogd("agent_process_work: returning client command");

	/* store tui working device for terminate tui
	 * when this device is closed.
	 */
	if (agent_id == TEE_TUI_AGENT_ID){
		tloge("TEE_TUI_AGENT_ID: pid-%d", current->pid);
		set_tui_caller_info(smc_cmd->dev_file_id, current->pid);}

	event_data->ret_flag = 1;
	/* Wake up the agent that will process the command */
	tlogd("agent_process_work: wakeup the agent");
	wake_up(&event_data->wait_event_wq);


	tlogd("agent 0x%x request, goto sleep, pe->run=%d\n",
		agent_id, atomic_read(&event_data->ca_run));
	/* we need to wait agent work done even if CA receive a signal */
	wait_event(event_data->ca_pending_wq, atomic_read(&event_data->ca_run));
	atomic_set(&event_data->ca_run, 0);
	put_agent_event(event_data);

	return ret;
}

/*
 * Function:	  is_agent_alive
 * Description:   This function check if the special agent is launched.
 * Used For HDCP key.
 * e.g. If sfs agent is not alive,
 * you can not do HDCP key write to SRAM.
 * Parameters:	 agent_id.
 * Return:		1:agent is alive
 *				0:agent not exsit.
 */
int is_agent_alive(unsigned int agent_id)
{
	struct __smc_event_data* event_data;
	event_data = find_event_control(agent_id);
	if (event_data) {
		put_agent_event(event_data);
		return 1;
	}
	else
		return 0;
}

int TC_NS_wait_event(unsigned int agent_id)
{
	int ret = -EINVAL;
	struct __smc_event_data *event_data;

	if ((TC_NS_get_uid() != 0) && check_ext_agent_access_with_agent_id(current, agent_id)) {
		tloge("It is a fake tee agent\n");
		return -EACCES;
	}

	event_data = find_event_control(agent_id);
	tlogi("agent %u waits for command\n", agent_id);
	if (event_data) {
		/* wait event will return either 0 or -ERESTARTSYS so just
		 * return it further to the ioctl handler */
		ret = wait_event_interruptible(event_data->wait_event_wq,/*lint !e774 !e845 !e712 !e40*/
					       event_data->ret_flag);
		put_agent_event(event_data);
	} else {
	     return -EINVAL;
	}

	return ret;
}


int TC_NS_sync_sys_time(TC_NS_Time *tc_ns_time)
{
	TC_NS_SMC_CMD smc_cmd = { 0 };
	int ret = 0;
	TC_NS_Time tmp_tc_ns_time = {0};
	struct mb_cmd_pack *mb_pack = NULL;

	if (!tc_ns_time) {
		tloge("tc_ns_time is NULL input buffer\n");
		return -EINVAL;
	}
	if (TC_NS_get_uid() != 0) {
		tloge("It is a fake tee agent\n");
		return TEEC_ERROR_GENERIC;
	}
	if (copy_from_user(&tmp_tc_ns_time, tc_ns_time, sizeof(TC_NS_Time))) {
		tloge("copy from user failed\n");
		return -EFAULT;
	}

	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack) {
		tloge("alloc mb pack failed\n");
		return -ENOMEM;
	}

	mb_pack->uuid[0] = 1;
	smc_cmd.uuid_phys = virt_to_phys(mb_pack->uuid);
	smc_cmd.uuid_h_phys = virt_to_phys(mb_pack->uuid) >> 32; /*lint !e572*/
	smc_cmd.cmd_id = GLOBAL_CMD_ID_ADJUST_TIME;
	smc_cmd.err_origin = (unsigned int)tmp_tc_ns_time.seconds;
	smc_cmd.ret_val = (unsigned int)tmp_tc_ns_time.millis;

	ret = TC_NS_SMC(&smc_cmd, 0);
	if (ret)
		tloge("tee adjust time failed, return error %x\n", ret);

	mailbox_free(mb_pack);

	return ret;
}

int TC_NS_send_event_response(unsigned int agent_id)
{
	struct __smc_event_data *event_data = find_event_control(agent_id);
	unsigned int ret;

	if (TC_NS_get_uid() != 0 && check_ext_agent_access_with_agent_id(current, agent_id)) {
		tloge("It is a fake tee agent\n");
		put_agent_event(event_data);
		return -1;
	}

	tlogd("agent %u sends answer back\n", agent_id);
	if (event_data && event_data->ret_flag) {
		event_data->send_flag = 1;
		event_data->ret_flag = 0;
		/* Send the command back to the TA session waiting for it */
		tlogi("agent 0x%x wakeup ca\n", agent_id);
		atomic_set(&event_data->ca_run, 1);
		/* make sure reset working_ca before wakeup CA */
		wake_up(&event_data->ca_pending_wq);
		ret = 0;
		put_agent_event(event_data);

		return ret;
	}
	put_agent_event(event_data);
	return -EINVAL;
}

int TC_NS_register_agent(TC_NS_DEV_File *dev_file, unsigned int agent_id,
			 TC_NS_Shared_MEM *shared_mem)
{
	TC_NS_SMC_CMD smc_cmd = { 0 };
	struct __smc_event_data *event_data = NULL;
	int ret = 0;
	int find_flag = 0;
	unsigned long flags;
	struct mb_cmd_pack *mb_pack = NULL;

	if ((TC_NS_get_uid() != 0) && check_ext_agent_access_with_agent_id(current, agent_id)) {
		tloge("It is a fake tee agent\n");
		ret = TEEC_ERROR_GENERIC;
		goto error;
	}

	spin_lock_irqsave(&agent_control.lock, flags);
	list_for_each_entry(event_data, &agent_control.agent_list, head) {
		if (event_data->agent_id == agent_id) {
			find_flag = 1;
			break;
		}
	}
	spin_unlock_irqrestore(&agent_control.lock, flags);

	if (find_flag) {
		if ((AGENT_FS_ID == agent_id) || (AGENT_MISC_ID == agent_id)
		    || (AGENT_SOCKET_ID == agent_id)) {
			event_data->ret_flag = 0;
			event_data->send_flag = 0;
			event_data->owner = dev_file;
			event_data->agent_alive = 1;
			atomic_set(&event_data->usage, 1);
			init_waitqueue_head(&(event_data->wait_event_wq));
			init_waitqueue_head(&(event_data->send_response_wq));
			init_waitqueue_head(&(event_data->ca_pending_wq));
			atomic_set(&event_data->ca_run, 0);
			ret = TEEC_SUCCESS;
		} else
			ret = TEEC_ERROR_GENERIC;
		goto error;
	}

	if (!shared_mem) {
		tloge("shared mem is not exist\n");
		ret = TEEC_ERROR_GENERIC;
		goto error;
	}

	/* Obtain this share memory which SHALL be released in TC_NS_unregister_agent() */
	get_sharemem_struct(shared_mem);

	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack) {
		tloge("alloc mailbox failed\n");
		ret = TEEC_ERROR_GENERIC;
		put_sharemem_struct(shared_mem);
		goto error;
	}

	mb_pack->operation.paramTypes = TEE_PARAM_TYPE_VALUE_INPUT | (TEE_PARAM_TYPE_VALUE_INPUT << 4);
	mb_pack->operation.params[0].value.a = virt_to_phys(shared_mem->kernel_addr);
	mb_pack->operation.params[0].value.b = virt_to_phys(shared_mem->kernel_addr) >> 32; /*lint !e572*/
	mb_pack->operation.params[1].value.a = shared_mem->len;

	mb_pack->uuid[0] = 1;
	smc_cmd.uuid_phys = virt_to_phys(mb_pack->uuid);
	smc_cmd.uuid_h_phys = virt_to_phys(mb_pack->uuid) >> 32; /*lint !e572*/
	smc_cmd.cmd_id = GLOBAL_CMD_ID_REGISTER_AGENT;
	smc_cmd.operation_phys = virt_to_phys(&mb_pack->operation);
	smc_cmd.operation_h_phys = virt_to_phys(&mb_pack->operation) >> 32; /*lint !e572*/
	smc_cmd.agent_id = agent_id;

	ret = TC_NS_SMC(&smc_cmd, 0);

	if (ret == TEEC_SUCCESS) {
		event_data =
			kzalloc(sizeof(struct __smc_event_data), GFP_KERNEL);
		if (!event_data) {
			ret = -ENOMEM;
			put_sharemem_struct(shared_mem);
			goto error;
		}
		event_data->agent_id = agent_id;
		mutex_init(&event_data->work_lock);
		event_data->ret_flag = 0;
		event_data->send_flag = 0;
		event_data->buffer = shared_mem;
		event_data->owner = dev_file;
		event_data->agent_alive = 1;
		init_waitqueue_head(&(event_data->wait_event_wq));
		init_waitqueue_head(&(event_data->send_response_wq));
		INIT_LIST_HEAD(&event_data->head);
		init_waitqueue_head(&(event_data->ca_pending_wq));
		atomic_set(&event_data->ca_run, 0);

		spin_lock_irqsave(&agent_control.lock, flags);
		list_add_tail(&event_data->head, &agent_control.agent_list);
		atomic_set(&event_data->usage, 1);
		spin_unlock_irqrestore(&agent_control.lock, flags);
	} else {
		/* release share mem when sending smc failure. */
		put_sharemem_struct(shared_mem);
	}

error:
	if (mb_pack)
		mailbox_free(mb_pack);
	return ret; /*lint !e429 */
}


int TC_NS_unregister_agent(unsigned int agent_id)
{
	struct __smc_event_data *event_data = NULL;
	int ret = 0;
	TC_NS_SMC_CMD smc_cmd = { 0 };
	struct mb_cmd_pack *mb_pack = NULL;

	if ((TC_NS_get_uid() != 0) && (TC_NS_get_uid() != SYSTEM_UID)) {
		tloge("It is a fake tee agent\n");
		return TEEC_ERROR_GENERIC;
	}
	if (AGENT_FS_ID == agent_id || AGENT_MISC_ID == agent_id ||
	    TEE_RPMB_AGENT_ID == agent_id || AGENT_SOCKET_ID == agent_id ||
	    TEE_TUI_AGENT_ID == agent_id) {
		tloge("system agent is not allowed to unregister  agent_id=0x%x\n", agent_id);
		return TEEC_ERROR_GENERIC;
	}
	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack) {
		tloge("alloc mailbox failed\n");
		return TEEC_ERROR_GENERIC;
	}
	event_data = find_event_control(agent_id);
	if (!event_data) {
		mailbox_free(mb_pack);
		tloge("agent is not found\n");
		return TEEC_ERROR_GENERIC;
	}

	mb_pack->operation.paramTypes = TEE_PARAM_TYPE_VALUE_INPUT | (TEE_PARAM_TYPE_VALUE_INPUT << 4);
	mb_pack->operation.params[0].value.a = virt_to_phys(event_data->buffer->kernel_addr);
	mb_pack->operation.params[0].value.b = virt_to_phys(event_data->buffer->kernel_addr) >> 32; /*lint !e572*/
	mb_pack->operation.params[1].value.a = SZ_4K;

	mb_pack->uuid[0] = 1;
	smc_cmd.uuid_phys = virt_to_phys(mb_pack->uuid);
	smc_cmd.uuid_h_phys = virt_to_phys(mb_pack->uuid) >> 32; /*lint !e572*/
	smc_cmd.cmd_id = GLOBAL_CMD_ID_UNREGISTER_AGENT;
	smc_cmd.operation_phys = virt_to_phys(&mb_pack->operation);
	smc_cmd.operation_h_phys = virt_to_phys(&mb_pack->operation) >> 32; /*lint !e572*/
	smc_cmd.agent_id = agent_id;

	mutex_lock(&event_data->work_lock);
	tlogd("Unregistering agent %u\n", agent_id);
	ret = TC_NS_SMC(&smc_cmd, 0);
	mutex_unlock(&event_data->work_lock);
	if (ret == TEEC_SUCCESS) {
    		free_event_control(agent_id);
	}
	put_agent_event(event_data);

	mailbox_free(mb_pack);

	return ret;
}

bool TC_NS_is_system_agent_client(TC_NS_DEV_File *dev_file)
{
	struct __smc_event_data *event_data = NULL;
	struct __smc_event_data *tmp = NULL;
	bool system_agent = false;
	unsigned long flags;

	if (!dev_file)
		return system_agent;

	spin_lock_irqsave(&agent_control.lock, flags);
	list_for_each_entry_safe(event_data, tmp,
				 &agent_control.agent_list, head) {
		if (event_data->owner == dev_file) {
			system_agent = true;
			break;
		}
	}
	spin_unlock_irqrestore(&agent_control.lock, flags);

	return system_agent;
}

int TC_NS_unregister_agent_client(TC_NS_DEV_File *dev_file)
{
	struct __smc_event_data *event_data = NULL;
	struct __smc_event_data	*tmp = NULL;
	unsigned int agent_id[AGENT_MAX] = {0};
	unsigned int i = 0;
	unsigned int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&agent_control.lock, flags);
	list_for_each_entry_safe(event_data, tmp,
				 &agent_control.agent_list, head) {
		if ((event_data->owner == dev_file)
				&& (i < AGENT_MAX))
			agent_id[i++] = event_data->agent_id;
	}
	spin_unlock_irqrestore(&agent_control.lock, flags);

	for (i = 0; i < AGENT_MAX; i++) {
		if (agent_id[i]) {
			if(TC_NS_unregister_agent(agent_id[i])) {
				TCERR("TC_NS_unregister_agent[%d] failed\n",agent_id[i]);
				ret |= 1;
			}
		}
	}

	return ret;
}
static int def_tee_agent_work(void *instance)
{
	int ret = 0;
	struct tee_agent_kernel_ops *agent_instance;
	agent_instance = (struct tee_agent_kernel_ops *)instance;

	while (!kthread_should_stop()) {
		tlogd("%s agent loop++++\n", agent_instance->agent_name);
		ret = TC_NS_wait_event(agent_instance->agent_id);
		if (ret) {
			tloge("%s wait event fail\n",
			      agent_instance->agent_name);
			break;
		}

		if (agent_instance->tee_agent_work) {
			ret = agent_instance->tee_agent_work(agent_instance);
			if (ret) {
			    tloge("%s agent work fail\n",
				  agent_instance->agent_name);
			}
		}

		ret = TC_NS_send_event_response(agent_instance->agent_id);
		if (ret) {
			tloge("%s send event response fail\n",
			      agent_instance->agent_name);
			break;
		}
		tlogd("%s agent loop----\n", agent_instance->agent_name);
	}

	return ret;
}


static int def_tee_agent_run(struct tee_agent_kernel_ops *agent_instance)
{
	TC_NS_Shared_MEM *shared_mem = NULL;
	TC_NS_DEV_File dev = {0};
	int ret = 0;
	int page_order = 8;

	/*1. Allocate agent buffer */
	shared_mem = tc_mem_allocate((size_t)(unsigned)(AGENT_BUFF_SIZE * page_order), true);
	while ((IS_ERR(shared_mem)) && (page_order > 0)) {
		page_order /= 2;
		shared_mem = tc_mem_allocate((size_t)(unsigned)(AGENT_BUFF_SIZE * page_order), true);
	}
	if (IS_ERR(shared_mem)) {
		tloge("allocate agent buffer fail\n");
		ret = PTR_ERR(shared_mem);
		goto out;
	}

	atomic_set(&shared_mem->usage, 1);
	agent_instance->agent_buffer = shared_mem;

	/*2. Register agent buffer to TEE */
	ret = TC_NS_register_agent(&dev, agent_instance->agent_id, shared_mem);
	if (ret) {
		tloge("register agent buffer fail,ret =0x%x\n", ret);
		ret = -1;
		goto out;
	}

	/*3. Creat thread to run agent */
	agent_instance->agent_thread =
		kthread_run(def_tee_agent_work, (void *)agent_instance, "agent_%s",
			    agent_instance->agent_name);
	if (IS_ERR(agent_instance->agent_thread)) {
		tloge("kthread creat fail\n");
		ret = PTR_ERR(agent_instance->agent_thread);
		agent_instance->agent_thread = NULL;
		goto out;
	}
	return 0;

out:
	if (!IS_ERR_OR_NULL(shared_mem)) {
		tc_mem_free(shared_mem); /*lint !e668 */
		agent_instance->agent_buffer = NULL;
	}

	return ret;
}


static int def_tee_agent_stop(struct tee_agent_kernel_ops *agent_instance)
{
	int ret;
	if (TC_NS_send_event_response(agent_instance->agent_id))
		tloge("failed to send response for agent %d\n", agent_instance->agent_id);
	ret = TC_NS_unregister_agent(agent_instance->agent_id);
	if (0 != ret)
		tloge("failed to unregister agent %d\n", agent_instance->agent_id);
	if (!IS_ERR_OR_NULL(agent_instance->agent_thread))
		kthread_stop(agent_instance->agent_thread);

	/* release share mem obtained in def_tee_agent_run() */
	put_sharemem_struct(agent_instance->agent_buffer);

	return 0;
}


static struct tee_agent_kernel_ops def_tee_agent_ops = {
	.agent_name = "default",
	.agent_id = 0,
	.tee_agent_init = NULL,
	.tee_agent_run = def_tee_agent_run,
	.tee_agent_work = NULL,
	.tee_agent_exit = NULL,
	.tee_agent_stop = def_tee_agent_stop,
	.tee_agent_crash_work = NULL,
	.list = LIST_HEAD_INIT(def_tee_agent_ops.list)
};


static int tee_agent_kernel_init(void)
{
	struct tee_agent_kernel_ops *agent_ops = NULL;
	int ret = 0;

	list_for_each_entry(agent_ops, &tee_agent_list, list) {
		/* Check the agent validity */
		if ((0 == agent_ops->agent_id)
		    || (NULL == agent_ops->agent_name)
		    || (NULL == agent_ops->tee_agent_work)) {
			tloge("agent is invalid\n");
			continue;
		}
		tlogd("ready to init %s agent, id=0x%x\n",
			agent_ops->agent_name, agent_ops->agent_id);

		/* Initialize the agent */
		if (agent_ops->tee_agent_init)
			ret = agent_ops->tee_agent_init(agent_ops);
		else if (def_tee_agent_ops.tee_agent_init)
			ret = def_tee_agent_ops.tee_agent_init(agent_ops);
		else
		    tlogw("agent id %d has no init function\n", agent_ops->agent_id);
		if (ret) {
			tloge("tee_agent_init %s failed\n",
			      agent_ops->agent_name);
			continue;
		}

		/* Run the agent */
		if (agent_ops->tee_agent_run)
			ret = agent_ops->tee_agent_run(agent_ops);
		else if (def_tee_agent_ops.tee_agent_run)
			ret = def_tee_agent_ops.tee_agent_run(agent_ops);
		else
		    tlogw("agent id %d has no run function\n", agent_ops->agent_id);
		if (ret) {
			tloge("tee_agent_run %s failed\n",
			      agent_ops->agent_name);
			if (agent_ops->tee_agent_exit)
				agent_ops->tee_agent_exit(agent_ops);
			continue;
		}
	}

	return 0;
}


static int tee_agent_kernel_exit(void)
{
	struct tee_agent_kernel_ops *agent_ops = NULL;

	list_for_each_entry(agent_ops, &tee_agent_list, list) {

		/* Stop the agent */
		if (agent_ops->tee_agent_stop)
			agent_ops->tee_agent_stop(agent_ops);
		else if (def_tee_agent_ops.tee_agent_stop)
			def_tee_agent_ops.tee_agent_stop(agent_ops);
		else
		    tlogw("agent id %d has no stop function\n", agent_ops->agent_id);
		/* Uninitialize the agent */
		if (agent_ops->tee_agent_exit)
			agent_ops->tee_agent_exit(agent_ops);
		else if (def_tee_agent_ops.tee_agent_exit)
			def_tee_agent_ops.tee_agent_exit(agent_ops);
		else
		    tlogw("agent id %d has no exit function\n", agent_ops->agent_id);
	}
	return 0;
}


int tee_agent_clear_work(TC_NS_ClientContext *context,
			 unsigned int dev_file_id)
{
	struct tee_agent_kernel_ops *agent_ops = NULL;

	list_for_each_entry(agent_ops, &tee_agent_list, list) {
		if (agent_ops->tee_agent_crash_work)
			agent_ops->tee_agent_crash_work(agent_ops, context,
							dev_file_id);
	}
	return 0;
}


int tee_agent_kernel_register(struct tee_agent_kernel_ops *new_agent)
{
	if (NULL == new_agent)
		return -1;
	INIT_LIST_HEAD(&new_agent->list);
	list_add_tail(&new_agent->list, &tee_agent_list);
	return 0;
}


int agent_init(void)
{
	spin_lock_init(&agent_control.lock);
	INIT_LIST_HEAD(&agent_control.agent_list);

	INIT_LIST_HEAD(&tee_agent_list);
	rpmb_agent_register();
	tee_agent_kernel_init();
	return 0;
}


int agent_exit(void)
{
	tee_agent_kernel_exit();
	return 0;
}
