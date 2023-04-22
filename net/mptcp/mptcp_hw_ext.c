
#include <net/inet_common.h>
#include <net/inet6_hashtables.h>
#include <net/ipv6.h>
#include <net/ip6_checksum.h>
#include <net/mptcp.h>
#include <net/mptcp_v4.h>
#if IS_ENABLED(CONFIG_IPV6)
#include <net/ip6_route.h>
#include <net/mptcp_v6.h>
#endif
#include <net/sock.h>
#include <net/tcp.h>
#include <net/tcp_states.h>
#include <linux/fdtable.h>
#include <linux/inet.h>
#ifdef CONFIG_HUAWEI_XENGINE
#include <huawei_platform/emcom/emcom_xengine.h>
#endif

#define ENABLE_BY_UID                   0
#define ENABLE_BY_UID_DIP_DPORT         1
#define ENABLE_BY_UID_DIP_DPORT_ISP     2
#define ENABLE_BY_PID_FD                3
#define ENABLE_BY_USR_SWITCH            4
#define ENABLE_BY_FIRST_PATH            5

#define MPTCP_HW_EXT_INVALID_GID        0xF

struct mptcp_hw_ext_uid_conf {
	uint32_t flags;
	struct mptcp_hw_ext_conf_value values;
};

struct mptcp_hw_ext_dport_node {
	struct list_head dport_node;
	struct mptcp_hw_dport_range *range;
	uint32_t range_num;
	uint32_t flags;
	struct mptcp_hw_ext_conf_value values;
	char port_key[MPTCP_HW_EXT_PORT_KEY_MAX_LEN];
};

struct mptcp_hw_ext_dip_node {
	struct list_head dip_node;
	uint32_t dip;
	uint8_t gid;
	uint8_t carrier_id;
	uint8_t deploy_info;
	struct list_head dport_list;
};

struct mptcp_hw_ext_dip_list {
	struct list_head list;
};

struct mptcp_hw_ext_uid {
	struct list_head list;
	int32_t uid;
	bool usr_switch; /*true or false by user */
	char prim_iface[IFNAMSIZ];
	uint8_t key_type;/* distinguish the types, by uid/uid_dip_dport/uid_dip_dport_isp */
	uint8_t proxy_fail_option; /* proxy continuous failure count from tcp fin option, only valid if proxy is set */
	uint8_t proxy_fail_noresponse; /* proxy continuous failure count from app server, only valid if proxy is set */
	uint8_t proxy_fail_syn_timeout; /* proxy continuous failure count from syn timeout when connect to proxy, only valid if proxy is set */
	void *config; /* 1) if by uid, this config point to one mptcp_hw_ext_conf_value
				 2) if by uid_dip_dport, this config point to one mptcp_hw_ext_dip_list */
	void *isp; /* one 4*5*4 */
	struct mptcp_hw_ext_first_path first_path;
};

static LIST_HEAD(mptcp_hw_ext_uid_list);
static DEFINE_RWLOCK(mptcp_hw_ext_lock);

struct mptcp_hw_intf_carrier {
	u_int8_t num_intf_carrier;
	struct mptcp_hw_carrier_info mptcp_carrier_info[MPTCP_IF_CARRIER_NUM_MAX];
};

struct mptcp_hw_intf_carrier mptcp_intf_carrier = {
	.num_intf_carrier = 0,
};

struct mptcp_hw_ext_carrier {
	uint8_t dst_addr_cnt;
	uint8_t dst_addr_idx;
	__be32 dst_addr[MPTCP_HW_EXT_MAX_IP_IN_CARRIER];
};

struct mptcp_hw_ext_gid {
	struct mptcp_hw_ext_carrier carrier_info[MPTCP_HW_EXT_MAX_CARRIER_ID];
};

struct mptcp_hw_ext_uid_gid_carrier_info {
	struct mptcp_hw_ext_gid mptcp_hw_ext_gid_addr_tbl[MPTCP_HW_EXT_MAX_GID];
};

#ifdef CONFIG_HUAWEI_XENGINE

static void xengine_report_uid_fallback(int32_t uid, int32_t reason)
{
	struct mptcp_hw_ext_proxy_fallback report;
	(void)memset(&report, 0, sizeof(report));

	report.uid = uid;
	report.reason = reason;
	Emcom_Xengine_MptcpProxyFallback(&report, sizeof(report));
}
#endif


static struct mptcp_hw_ext_uid *mptcp_hw_ext_uid_in_list(int32_t uid)
{
	struct mptcp_hw_ext_uid *node;

	list_for_each_entry(node, &mptcp_hw_ext_uid_list, list) {
		if (node->uid == uid)
			return node;
	}

	return NULL;
}


static struct mptcp_hw_ext_uid_gid_carrier_info *
	mptcp_hw_ext_get_isp_info_by_uid(int32_t uid)
{
	struct mptcp_hw_ext_uid *uid_node;
	uid_node = mptcp_hw_ext_uid_in_list(uid);
	if (NULL == uid_node)
		return NULL;

	if (uid_node->key_type != ENABLE_BY_UID_DIP_DPORT_ISP)
		return NULL;

	return (struct mptcp_hw_ext_uid_gid_carrier_info *)(uid_node->isp);
}


static bool mptcp_hw_ext_dport_valid(
		      struct mptcp_hw_dport_range *dport_range, int *pvalid_add)
{
	int i, j;
	int valid_add = 0;

	if (!dport_range)
		return false;

	for (i = 0; i < MPTCP_PORT_RANGE_NUM_MAX; i++) {
		if (!dport_range[i].dport_start)
			break;

		if (dport_range[i].dport_start > dport_range[i].dport_end)
			return false;

		valid_add++;
		for (j = i + 1; j < MPTCP_PORT_RANGE_NUM_MAX; j++) {
			if (!dport_range[j].dport_start)
				break;

			if (dport_range[j].dport_start > dport_range[j].dport_end)
				return false;

			if (!(dport_range[i].dport_start > dport_range[j].dport_end ||
			      dport_range[i].dport_end < dport_range[j].dport_start)) {
				pr_err("%s: port range %u~%u is partial match with %u~%u\n",
					__func__,
					dport_range[i].dport_start, dport_range[i].dport_end,
					dport_range[j].dport_start, dport_range[j].dport_end);
				return false;
			}
		}
	}

	if (valid_add == 0)
		return false;

	if (pvalid_add)
		*pvalid_add = valid_add;

	return true;
}


static bool mptcp_hw_validate_conf_values_common(uint32_t value_type,
				union mptcp_hw_ext_value *val, char *if_name)
{
	if (!val)
		return false;

	switch (value_type) {
	case MPTCP_HW_CONF_VALUE_SCHEDULER: {
		__kernel_size_t sched_len;

		sched_len = strnlen(val->sched_name, MPTCP_SCHED_NAME_MAX);
		if ((sched_len == 0) || (sched_len == MPTCP_SCHED_NAME_MAX)) {
			pr_err("%s: sched_name len invalid\n", __func__);
			return false;
		}

		if (!mptcp_sched_check_exist(val->sched_name)) {
			pr_err("%s: sched_name is not registed\n", __func__);
			return false;
		}
	}
	/* lint - fallthrough */
	case MPTCP_HW_CONF_VALUE_SND_RCV_BUF:
	{
		size_t len;

		if (if_name) {
			len = strnlen(if_name, IFNAMSIZ);
			if ((len == 0) || (len == IFNAMSIZ)) {
				pr_err("%s: ifname len invalid\n", __func__);
				return false;
			}
		} else
			return false;
		break;
	}
	case MPTCP_HW_CONF_VALUE_SCHED_PARAMS:
	default:
		break;
	}

	return true;
}


static int mptcp_hw_validate_set_conf_values_ipport(
	struct mptcp_hw_ext *config, uint8_t key_type, uint32_t value_type,
	int *valid_add)
{
	struct mptcp_hw_dip_dport *dip_dport = &(config->conf.dip_dport);
	union mptcp_hw_ext_value *val = &(config->conf_value);

	if (0 == strnlen(dip_dport->port_key, MPTCP_HW_EXT_PORT_KEY_MAX_LEN)) {
		pr_err("%s: port_key len is 0\n", __func__);
		return -EINVAL;
	}

	if (key_type == ENABLE_BY_UID_DIP_DPORT_ISP) {
		if (config->conf_value.i_val.gid >= MPTCP_HW_EXT_MAX_GID ||
			config->conf_value.i_val.carrier_id >= MPTCP_HW_EXT_MAX_CARRIER_ID ||
			config->conf_value.i_val.deploy_info != MPTCP_HW_EXT_DEPLOY_LOADBALANCE) {
			pr_err("%s: gid:%u carrier_id:%u deploy_info:%u invalid\n",
				__func__, config->conf_value.i_val.gid,
				config->conf_value.i_val.carrier_id,
				config->conf_value.i_val.deploy_info);
			return -EINVAL;
		}
	}

	switch (value_type) {
	case MPTCP_HW_CONF_VALUE_MPTCP: {
			if (mptcp_hw_ext_dport_valid(dip_dport->dport_range,
					valid_add) == false) {
				pr_err("%s: Invalid port range\n", __func__);
				return -EINVAL;
			}
		}
		break;
	case MPTCP_HW_CONF_VALUE_SCHEDULER:
	case MPTCP_HW_CONF_VALUE_SND_RCV_BUF:
	case MPTCP_HW_CONF_VALUE_SCHED_PARAMS: {
			if (!mptcp_hw_validate_conf_values_common(value_type, val,
					config->if_name)) {
				pr_err("%s: mptcp_hw_validate_conf_values_common failed\n", __func__);
				return -EINVAL;
			}
		}
		break;
	default:
		pr_err("%s: flags=%u, not supported\n", __func__, value_type);
		return -EINVAL;
	}

	return 0;
}


static int mptcp_hw_validate_set_conf_values(uint32_t value_type,
				union mptcp_hw_ext_value *val, char *if_name, bool is_add)
{
	switch (value_type) {
	case MPTCP_HW_CONF_VALUE_MPTCP:
		return 0;

	case MPTCP_HW_CONF_VALUE_SUBFLOW_PRIO: {
		if (!is_add) {
			pr_err("%s not support del operation for subflow prio\n", __func__);
			return -EINVAL;
		}
		return 0;
	}

	case MPTCP_HW_CONF_VALUE_PROXY_INFO:
		if (is_add) {
			if (0 == val->proxy_info.proxy_ip || 0 == val->proxy_info.proxy_port)
				return -EINVAL;
		}
		return 0;

	case MPTCP_HW_CONF_VALUE_SCHEDULER:
	case MPTCP_HW_CONF_VALUE_SND_RCV_BUF:
	case MPTCP_HW_CONF_VALUE_SCHED_PARAMS: {
		if (!mptcp_hw_validate_conf_values_common(value_type, val, if_name)) {
			pr_err("%s: mptcp_hw_validate_conf_values_common failed\n", __func__);
			return -EINVAL;
		}
		return 0;
	}

	default:
		pr_err("%s: flags=%u, not supported\n", __func__, value_type);
		return -EINVAL;
	}
}


static void mptcp_hw_del_conf_values(uint32_t flags, uint32_t *oflags,
				     struct mptcp_hw_ext_conf_value *value,
				     char *if_name)
{
	switch (flags) {
	case MPTCP_HW_CONF_VALUE_SCHEDULER: {
		(void)memset(value->sched.sched_name, 0, MPTCP_SCHED_NAME_MAX);
		(void)memset(value->sched.if_name, 0, IFNAMSIZ);
		*oflags &= ~flags;
		break;
	}

	case MPTCP_HW_CONF_VALUE_PROXY_INFO: {
		value->proxy_info.proxy_ip = 0;
		value->proxy_info.proxy_port = 0;
		*oflags &= ~flags;
		break;
	}

	default:
		*oflags &= ~flags;
	}

	if (!*oflags) {
		mptcp_debug("%s: config has some problem, can causing memory leak", __func__);
	}

	return;
}


static int mptcp_hw_set_conf_values(uint32_t value_type,
				    struct mptcp_hw_ext_conf_value *dstval,
				    union mptcp_hw_ext_value *srcval,
				    char *if_name)
{
	switch (value_type) {
	case MPTCP_HW_CONF_VALUE_SCHEDULER: {
		mptcp_debug("%s: if_name:%s sched_name:%s\n",
			__func__, if_name, srcval->sched_name);
		(void)strncpy(dstval->sched.if_name, if_name, IFNAMSIZ);
		dstval->sched.if_name[IFNAMSIZ - 1] = '\0';
		(void)strncpy(dstval->sched.sched_name, srcval->sched_name,
			MPTCP_SCHED_NAME_MAX);
		dstval->sched.sched_name[MPTCP_SCHED_NAME_MAX - 1] = '\0';
		mptcp_info("%s: sched_if_name:%s sched_name:%s\n", __func__,
			dstval->sched.if_name,
			dstval->sched.sched_name);
		break;
	}

	case MPTCP_HW_CONF_VALUE_SCHED_PARAMS: {
		dstval->sched_params = srcval->sched_params;
		mptcp_info("%s: rtt_thres:%u sched_params:%u\n", __func__,
			srcval->sched_params, dstval->sched_params);
		break;
	}

	case MPTCP_HW_CONF_VALUE_PROXY_INFO: {
		char ip_str[INET_ADDRSTRLEN] = {0};
		dstval->proxy_info.proxy_ip = srcval->proxy_info.proxy_ip;
		dstval->proxy_info.proxy_port = srcval->proxy_info.proxy_port;
		mptcp_info("%s: proxy_ip:%s proxy_port:%u\n", __func__,
			anonymousIPv4addr(srcval->proxy_info.proxy_ip, ip_str, INET_ADDRSTRLEN),
			dstval->proxy_info.proxy_port);
		break;
	}

	default:
		break;
	}

	return 0;
}


static struct mptcp_hw_ext_dip_node *mptcp_hw_ext_get_node_by_dip(
	struct mptcp_hw_ext_dip_list *dip_list, uint32_t dip)
{
	struct mptcp_hw_ext_dip_node *node, *next;

	list_for_each_entry_safe(node, next, &(dip_list->list), dip_node) {
		if (node->dip == dip)
			return node;
	}

	return NULL;
}


static bool mptcp_hw_ext_dport_enable_valid(
	struct mptcp_hw_ext_dip_node *node,
	struct mptcp_hw_ext *config, uint8_t key_type, uint32_t port_to_add)
{
	int i;
	struct mptcp_hw_ext_dport_node *dport_node;
	struct mptcp_hw_dport_range *dport_range = config->conf.dip_dport.dport_range;
	union mptcp_hw_ext_value *value = &(config->conf_value);
	char *port_key = config->conf.dip_dport.port_key;

	if (!node || 0 == strnlen(port_key, MPTCP_HW_EXT_PORT_KEY_MAX_LEN) || !value)
		return false;

	if (key_type == ENABLE_BY_UID_DIP_DPORT_ISP) {
		if (config->conf_value.i_val.gid != node->gid ||
			config->conf_value.i_val.carrier_id != node->carrier_id ||
			config->conf_value.i_val.deploy_info != node->deploy_info) {
			pr_err("%d isp info not same with enable\n", __LINE__);
			return false;
		}
	}

	for (i = 0; i < MPTCP_PORT_RANGE_NUM_MAX; i++) {
		int j;

		if (!dport_range[i].dport_start)
			break;
		list_for_each_entry(dport_node, &(node->dport_list), dport_node) {
			if ((0 == strncmp(port_key, dport_node->port_key, MPTCP_HW_EXT_PORT_KEY_MAX_LEN)))
				return false;

			for (j = 0; j < dport_node->range_num; j++) {
				if (!(dport_range[i].dport_start > dport_node->range[j].dport_end ||
				      dport_range[i].dport_end < dport_node->range[j].dport_start)) {
					pr_err("%s: port range %u~%u is partial match with exist %u~%u\n", __func__,
						dport_range[i].dport_start, dport_range[i].dport_end,
						dport_node->range[j].dport_start, dport_node->range[j].dport_end);
					return false;
				}
			}
		}
	}

	return true;
}


static bool mptcp_hw_ext_dport_update_config_valid(
	struct mptcp_hw_ext_dip_node *node, struct mptcp_hw_ext *config)
{
	struct mptcp_hw_ext_dport_node *dport_node, *dport_next;

	if (!node || 0 == strnlen(config->conf.dip_dport.port_key, MPTCP_HW_EXT_PORT_KEY_MAX_LEN))
		return false;

	list_for_each_entry_safe(dport_node, dport_next, &(node->dport_list), dport_node) {\
		if ((0 == strncmp(dport_node->port_key, config->conf.dip_dport.port_key,
				MPTCP_HW_EXT_PORT_KEY_MAX_LEN)))
			return true;
	}


	return false;
}



static bool mptcp_hw_ext_dport_match_with_exist(
	struct mptcp_hw_ext_dip_node *node,
	struct mptcp_hw_ext *config, uint8_t key_type, uint32_t value_type,
	uint32_t port_to_add)
{
	if (value_type == MPTCP_HW_CONF_VALUE_MPTCP)
		return mptcp_hw_ext_dport_enable_valid(node, config, key_type, port_to_add);
	else
		return mptcp_hw_ext_dport_update_config_valid(node, config);
}


static struct mptcp_hw_ext_dport_node *mptcp_hw_ext_get_node_by_dport(
	struct list_head *dport_list, uint16_t dport)
{
	struct mptcp_hw_ext_dport_node *dport_node;
	int i;

	list_for_each_entry(dport_node, dport_list, dport_node) {
		for (i = 0; i < dport_node->range_num; i++) {
			if (dport_node->range[i].dport_start <= dport &&
				dport_node->range[i].dport_end >= dport)
				return dport_node;
		}
	}

	return NULL;
}


static void mptcp_hw_ext_set_uid_gid_carrier_ip(__be32 dip,
	union mptcp_hw_ext_value *value,
	struct mptcp_hw_ext_uid_gid_carrier_info *node)
{
	uint8_t dst_addr_cnt;
	char ip_str[INET_ADDRSTRLEN];
	/* input parameters already validated in the function
	mptcp_hw_validate_set_conf_values, so skipping here */
	mptcp_debug("%s: dst %s carrier_id%u\n", __func__,
			anonymousIPv4addr(dip, ip_str, INET_ADDRSTRLEN),
		    value->i_val.carrier_id);

	dst_addr_cnt = node->mptcp_hw_ext_gid_addr_tbl[value->i_val.gid]\
						.carrier_info[value->i_val.carrier_id].dst_addr_cnt;

	node->mptcp_hw_ext_gid_addr_tbl[value->i_val.gid]\
		.carrier_info[value->i_val.carrier_id].dst_addr[dst_addr_cnt++] = dip;

	node->mptcp_hw_ext_gid_addr_tbl[value->i_val.gid]\
		.carrier_info[value->i_val.carrier_id].dst_addr_cnt = dst_addr_cnt;
}


static struct file *__fget_by_pid(pid_t pid, unsigned int fd, fmode_t mask)
{
	struct task_struct *task;
	struct file *file;
	struct files_struct *files;

	rcu_read_lock();

	task = find_task_by_vpid(pid);
	if (!task) {
		rcu_read_unlock();
		return NULL;
	}

	files = task->files;

loop:
	file = fcheck_files(files, fd);
	if (file) {
		/* File object ref couldn't be taken.
		 * dup2() atomicity guarantee is the reason
		 * we loop to catch the new file (or NULL pointer)
		 */
		if (file->f_mode & mask)
			file = NULL;
		else if (!get_file_rcu(file))
			goto loop;
	}
	rcu_read_unlock();

	return file;
}

static struct file *fget_by_pid_fd(pid_t pid, unsigned int fd)
{
	return __fget_by_pid(pid, fd, 0);
}


static void mptcp_hw_ext_update_subflow_buf_info(struct tcp_sock *tp,
	__s32 snd_buf, __s32 rcv_buf, char *if_name)
{
	uint8_t i = 0;

	for (i = 0; i < MAX_SUPPORT_SUBFLOW_NUM; i++) {
		if ((strnlen(if_name, IFNAMSIZ) == strnlen(tp->hw_subflows[i].if_name, IFNAMSIZ))
				&& (0 == strncmp(if_name, tp->hw_subflows[i].if_name, IFNAMSIZ))) {
			tp->hw_subflows[i].snd_buf = snd_buf;
			tp->hw_subflows[i].rcv_buf = rcv_buf;
			return;
		}

		if (0 == strnlen(tp->hw_subflows[i].if_name, IFNAMSIZ)) {
			(void)strncpy(tp->hw_subflows[i].if_name, if_name, strnlen(if_name, IFNAMSIZ));
			tp->hw_subflows[i].if_name[IFNAMSIZ - 1] = '\0';
			tp->hw_subflows[i].snd_buf = snd_buf;
			tp->hw_subflows[i].rcv_buf = rcv_buf;
			return;
		}
	}
}


static void mptcp_hw_ext_update_subflow_prio_at_meta(struct tcp_sock *tp,
	char *if_name, uint8_t low_prio)
{
	uint8_t i = 0;
	for (i = 0; i < MAX_SUPPORT_SUBFLOW_NUM; i++) {
		if ((strnlen(if_name, IFNAMSIZ) == strnlen(tp->hw_subflows[i].if_name, IFNAMSIZ)) &&
				(0 == strncmp(if_name, tp->hw_subflows[i].if_name, IFNAMSIZ))) {
			tp->hw_subflows[i].low_prio = low_prio;
			return;
		}

		if (0 == strnlen(tp->hw_subflows[i].if_name, IFNAMSIZ)) {
			(void)strncpy(tp->hw_subflows[i].if_name, if_name, strnlen(if_name, IFNAMSIZ));
			tp->hw_subflows[i].low_prio = low_prio;
			return;
		}
	}
}


static void mptcp_hw_ext_mod_subflow_prio(struct sock *meta_sk)
{
	struct tcp_sock *meta_tp = tcp_sk(meta_sk), *subtp = NULL;
	struct sock *subsk = NULL;
	struct mptcp_cb *mpcb = meta_tp->mpcb;
	uint8_t i = 0;

	if (NULL == mpcb)
		return;

	mptcp_for_each_sk(mpcb, subsk) {
		subtp = tcp_sk(subsk);

		if (subtp->mptcp)
			subtp->mptcp->send_mp_other_prio = 1;

		for (i = 0; i < MAX_SUPPORT_SUBFLOW_NUM; i++) {
			if (meta_tp->hw_subflows[i].if_name[0] == '\0')
				break;

			if (mptcp_is_subflow_from_iface(subsk, meta_tp->hw_subflows[i].if_name)) {
				mptcp_debug("%s has flow on if_name:%s\n", __func__,
					meta_tp->hw_subflows[i].if_name);
				if (subtp->mptcp && subtp->mptcp->low_prio != meta_tp->hw_subflows[i].low_prio) {
					subtp->mptcp->send_mp_prio = 1;
					subtp->mptcp->low_prio = meta_tp->hw_subflows[i].low_prio;
				}
			}
		}
	}
}


static int mptcp_hw_ext_set_config_by_pid_fd(struct mptcp_hw_ext *config,
	uint32_t value_type)
{
	int ret = -EBADF;
	int err = 0;
	struct file *file;
	struct socket *sock;
	struct sock *sk;
	struct tcp_sock *tp;
	pid_t pid = config->conf.pid_fd.pid;
	int fd = config->conf.pid_fd.fd;
	union mptcp_hw_ext_value *value = &(config->conf_value);

	mptcp_info("%s: pid_fd:%d_%d flags:%u\n", __func__,
		pid, fd, value_type);

	file = fget_by_pid_fd(pid, fd);
	if (!file) {
		pr_err("%s: get file fail by pid_fd:%d_%d\n",
			__func__, pid, fd);
		return ret;
	}

	sock = sock_from_file(file, &err);
	if (!sock) {
		pr_err("%s get socket fail by pid_fd:%d_%d\n",
			__func__, pid, fd);
		fput_by_pid(pid, file);
		return err;
	}

	sk = sock->sk;
	if (!sk) {
		pr_err("%s: get sock fail by pid_fd:%d_%d\n",
			__func__, pid, fd);
		fput_by_pid(pid, file);
		return ret;
	}

	write_unlock_bh(&mptcp_hw_ext_lock);

	local_bh_disable();
	bh_lock_sock(sk);
	tp = tcp_sk(sk);

	if (!tp) {
		pr_err("%s: get tcp_sock fail by pid_fd:%d_%d\n",
			__func__, pid, fd);
		goto exit;
	}

	switch (value_type) {
	case MPTCP_HW_CONF_VALUE_MPTCP: {
		if ((value->i_val.mp_capability != TRUE
				&& value->i_val.mp_capability != FALSE)
			|| (sk->sk_state != TCP_CLOSE)) {
			pr_err("%s: status not allow to set config\n",
				__func__);
			ret = -EPERM;
			goto exit;
		}

		if (value->i_val.mp_capability) {
			struct mptcp_hw_ext_uid *uid_node;

			tp->mptcp_cap_flag = MPTCP_CAP_PID_FD;
			read_lock_bh(&mptcp_hw_ext_lock);
			uid_node = mptcp_hw_ext_uid_in_list(sock_i_uid(sk).val);
			if (uid_node)
				tp->user_switch = uid_node->usr_switch;
			read_unlock_bh(&mptcp_hw_ext_lock);
		} else
			tp->mptcp_cap_flag = 0;
		break;
	}

	case MPTCP_HW_CONF_VALUE_SND_RCV_BUF: {
		if (value->buf.snd_buf) {
			/* copied from SO_SNDBUF and SO_RCVBUF */
			int32_t val = value->buf.snd_buf;

			val = min_t(u32, val, sysctl_wmem_max);
			value->buf.snd_buf = max_t(int, val * 2, SOCK_MIN_SNDBUF);
		}

		if (value->buf.rcv_buf) {
			/* copied from SO_SNDBUF and SO_RCVBUF */
			int32_t val = value->buf.rcv_buf;

			val = min_t(u32, val, sysctl_rmem_max);
			value->buf.rcv_buf = max_t(int, val * 2, SOCK_MIN_RCVBUF);
		}

		mptcp_hw_ext_update_subflow_buf_info(tp, value->buf.snd_buf,
			value->buf.rcv_buf, config->if_name);
		break;
    }

	case MPTCP_HW_CONF_VALUE_SCHEDULER: {
		if (sk->sk_state != TCP_CLOSE) {
			ret = -EPERM;
			goto exit;
		}

		err = mptcp_set_scheduler(sk, value->sched_name);
		if (err) {
			pr_err("%s: set scheduler fail, err:%d\n", __func__, err);
			ret = err;
			goto exit;
		}
		(void)strncpy(tp->prim_iface, config->if_name, IFNAMSIZ);
		tp->prim_iface[IFNAMSIZ - 1] = '\0';
		break;
	}

	case MPTCP_HW_CONF_VALUE_SCHED_PARAMS: {
		*(u32 *)tp->mptcp_sched_params = value->sched_params;
		break;
	}

	case MPTCP_HW_CONF_VALUE_SUBFLOW_PRIO: {
		uint8_t i = 0;

		for (i = 0; i < MAX_SUPPORT_SUBFLOW_NUM; i++) {
			if (value->subflow_prio[i].if_name[0] == '\0')
				break;

			mptcp_hw_ext_update_subflow_prio_at_meta(tp, value->subflow_prio[i].if_name,
				value->subflow_prio[i].low_prio);
		}

		mptcp_hw_ext_mod_subflow_prio(sk);
		break;
	}

	default:
		break;
	}

	mptcp_debug("%s: pid_fd:%d_%d flag:%u user_switch:%u\n", __func__, pid, fd,
		value_type, tp->user_switch);
	ret = 0;
exit:
	bh_unlock_sock(sk);
	local_bh_enable();
	write_lock_bh(&mptcp_hw_ext_lock);
	fput_by_pid(pid, file);

	return ret;
}


void mptcp_hw_get_ifname_fr_sock(struct sock *sk, char *ifname)
{
	struct inet_sock *inet = inet_sk(sk);
	struct net *net = sock_net(sk);
	struct net_device *dev;
	struct in_device *in_dev;
	__be32 saddr;

	ifname[0] = 0;

	if (sk->sk_bound_dev_if) {
		(void)netdev_get_name(net, ifname, sk->sk_bound_dev_if);
		return;
	}

	saddr = inet->inet_rcv_saddr;
	if (!saddr)
		saddr = inet->inet_saddr;

	/* search based on saddr */
	rcu_read_lock();
	for_each_netdev_rcu(net, dev) {
		in_dev = __in_dev_get_rcu(dev);
		if (!in_dev)
			continue;

		for_ifa(in_dev) {
			if (saddr == ifa->ifa_address) {
				(void)strncpy(ifname, dev->name, IFNAMSIZ);
				ifname[IFNAMSIZ - 1] = '\0';
				break;
			}
		}
		endfor_ifa(in_dev);
	}
	rcu_read_unlock();
}


static int mptcp_hw_try_get_throughput_from_delete_subflow(
	const struct sock *meta_sk,
	struct mptcp_hw_ext_subflow_info *value,
	char *if_name)
{
	const struct tcp_sock *meta_tp = tcp_sk(meta_sk);
	u64 sent = 0;
	u64 recv = 0;
	struct mptcp_deleted_flow_info *deleted;
	int ret = -EINVAL;

	if (meta_tp->mpcb == NULL) {
		pr_err("%s mpcb null\n", __func__);
		return ret;
	}

	list_for_each_entry(deleted, &meta_tp->mpcb->deleted_flow_info, node) {
		if (strncmp(deleted->ifname, if_name, IFNAMSIZ) == 0) {
			sent += deleted->tcpi_bytes_acked;
			recv += deleted->tcpi_bytes_received;
			ret = 0;
		}
	}

	value->total_send_bytes = sent;
	value->total_recv_bytes = recv;
	return ret;
}


static int mptcp_get_subflow_info(const struct sock *meta_sk,
			   struct mptcp_hw_ext_subflow_info *value,
			   char *if_name)
{
	const struct tcp_sock *meta_tp = tcp_sk(meta_sk);
	struct sock *sk;
	char sk_ifname[IFNAMSIZ];
	struct tcp_sock *tp;
	u64 sent = 0;
	u64 recv = 0;

	if (meta_tp->mpcb == NULL) {
		pr_err("%s mpcb null\n", __func__);
		return -EINVAL;
	}

	mptcp_for_each_sk(meta_tp->mpcb, sk) {
		mptcp_hw_get_ifname_fr_sock(sk, sk_ifname);
		if (if_name && strncmp(sk_ifname, if_name, IFNAMSIZ))
			continue;

		tp = tcp_sk(sk);
		tcp_get_info(sk, &value->subflow);
		value->loc_id = tp->mptcp->loc_id;

		sent = value->subflow.tcpi_bytes_acked;
		recv = value->subflow.tcpi_bytes_received;

		(void)mptcp_hw_try_get_throughput_from_delete_subflow(meta_sk,
			value, if_name);

		value->total_send_bytes += sent;
		value->total_recv_bytes += recv;
		return 0;
	}

	pr_err("%s: can't find if_name:%s\n", __func__, if_name);
	return -EINVAL;
}


static int mptcp_hw_ext_pid_fd_get_mptcp_info(pid_t pid, int fd,
					      union mptcp_hw_ext_value *value,
					      char *if_name)
{
	int ret;
	int err;
	struct file *file;
	struct socket *sock;
	struct sock *sk;
	struct tcp_sock *tp;
	__kernel_size_t len;

	len = strnlen(if_name, IFNAMSIZ);
	if ((len == 0) || (len == IFNAMSIZ)) {
		pr_err("%s: invalid param if_name, len:%u\n",
			__func__, (unsigned int)len);
		return -EINVAL;
	}

	file = fget_by_pid_fd(pid, fd);
	if (!file) {
		pr_err("%s: can't get file fd by pid_fd:%d_%d\n",
			__func__, pid, fd);
		return -EBADF;
	}

	sock = sock_from_file(file, &err);
	if (!sock) {
		pr_err("%s: can't get socket by pid_fd:%d_%d\n",
			__func__, pid, fd);
		fput_by_pid(pid, file);
		return err;
	}

	sk = sock->sk;
	if (!sk) {
		pr_err("%s can't get sock by pid_fd:%d_%d\n",
			__func__, pid, fd);
		fput_by_pid(pid, file);
		return -EBADF;
	}

	local_bh_disable();
	bh_lock_sock(sk);
	tp = tcp_sk(sk);
	if ((!tp) || !mptcp(tp)) {
		pr_err("%s: can't get tcp_sock by pid_fd:%d_%d\n",
			__func__, pid, fd);
		bh_unlock_sock(sk);
		local_bh_enable();
		fput_by_pid(pid, file);
		return -EINVAL;
	}

	ret = mptcp_get_subflow_info(sk, &value->mp_info, if_name);
	if (ret < 0) {
		mptcp_debug("%s: pid:%d fd:%d\n", __func__, pid, fd);
		ret = mptcp_hw_try_get_throughput_from_delete_subflow(sk,
			&value->mp_info, if_name);
	}
	bh_unlock_sock(sk);
	local_bh_enable();
	fput_by_pid(pid, file);

	return ret;
}


static struct mptcp_hw_ext_dip_node *mptcp_hw_ext_get_node_by_uid_dip(
	int32_t uid, uint32_t dip)
{
	struct mptcp_hw_ext_uid *uid_node;
	struct mptcp_hw_ext_dip_list *dip_list;
	struct mptcp_hw_ext_dip_node *dip_node;

	uid_node = mptcp_hw_ext_uid_in_list(uid);
	if (NULL == uid_node)
		return NULL;

	if (uid_node->key_type != ENABLE_BY_UID_DIP_DPORT
		&& uid_node->key_type != ENABLE_BY_UID_DIP_DPORT_ISP)
		return NULL;

	dip_list = (struct mptcp_hw_ext_dip_list *)(uid_node->config);
	dip_node = mptcp_hw_ext_get_node_by_dip(dip_list, dip);
	if (NULL == dip_node)
		return NULL;

	return dip_node;

}


static struct mptcp_hw_ext_dport_node *mptcp_hw_ext_get_node_by_uid_dip_dport(
	int32_t uid, uint32_t dip, uint16_t dport)
{
	struct mptcp_hw_ext_dip_node *dip_node;
	struct mptcp_hw_ext_dport_node *dport_node;
	dip_node = mptcp_hw_ext_get_node_by_uid_dip(uid, dip);
	if (NULL == dip_node)
		return NULL;

	dport_node = mptcp_hw_ext_get_node_by_dport(&(dip_node->dport_list), dport);
	return dport_node;
}


static int mptcp_hw_ext_add_new_dport_node(struct list_head *dport_list,
	struct mptcp_hw_ext *config, uint32_t value_type, uint32_t port_to_add)
{
	int ret;
	int dport_range_len;
	int malloc_len;
	struct mptcp_hw_ext_dport_node *dport_node;

	dport_range_len = port_to_add * sizeof(struct mptcp_hw_dport_range);
	malloc_len = sizeof(struct mptcp_hw_ext_dport_node) + dport_range_len;
	dport_node = (struct mptcp_hw_ext_dport_node *)kzalloc(malloc_len, GFP_ATOMIC);
	if (NULL == dport_node) {
		pr_err("%d kzalloc mptcp_hw_ext_dport_node fail\n", __LINE__);
		return -ENOMEM;
	}
	dport_node->range_num = port_to_add;
	dport_node->range = (struct mptcp_hw_dport_range *)&dport_node[1];
	(void)memcpy(dport_node->range, config->conf.dip_dport.dport_range, dport_range_len);
	dport_node->flags = value_type;
	(void)strncpy(dport_node->port_key, config->conf.dip_dport.port_key, MPTCP_HW_EXT_PORT_KEY_MAX_LEN);
	dport_node->port_key[MPTCP_HW_EXT_PORT_KEY_MAX_LEN - 1] = '\0';

	/* set config for dport_node; */
	ret = mptcp_hw_set_conf_values(value_type, &(dport_node->values), &(config->conf_value),
				       config->if_name);
	/* note, there could not fail */
	if (0 != ret) {
		kfree(dport_node);
		return ret;
	}

	list_add_tail(&(dport_node->dport_node), dport_list);
	return 0;
}


static int mptcp_hw_ext_add_new_dip_node(struct list_head *dip_list,
	struct mptcp_hw_ext *config,
	uint8_t key_type, uint32_t value_type, uint32_t port_to_add)
{
	uint32_t dip = config->conf.dip_dport.dip;

	struct mptcp_hw_ext_dip_node *dip_node;
	dip_node = (struct mptcp_hw_ext_dip_node *)kzalloc(sizeof(struct mptcp_hw_ext_dip_node), GFP_ATOMIC);
	if (NULL == dip_node) {
		pr_err("%d kzalloc mptcp_hw_ext_dip_node fail\n", __LINE__);
		return -ENOMEM;
	}
	INIT_LIST_HEAD(&(dip_node->dport_list));
	dip_node->dip = dip;

	if (key_type == ENABLE_BY_UID_DIP_DPORT_ISP) {
		dip_node->gid = config->conf_value.i_val.gid;
		dip_node->carrier_id = config->conf_value.i_val.carrier_id;
		dip_node->deploy_info = config->conf_value.i_val.deploy_info;
	}

	if (0 != mptcp_hw_ext_add_new_dport_node(&(dip_node->dport_list), config,
			value_type, port_to_add)) {
		kfree(dip_node);
		return -ENOMEM;
	}

	list_add_tail(&(dip_node->dip_node), dip_list);

	return 0;
}


static int mptcp_hw_ext_add_new_uid_node(struct mptcp_hw_ext *config,
	uint8_t key_type, uint32_t value_type, uint32_t port_to_add)
{
	int32_t uid;
	int ret;

	struct mptcp_hw_ext_uid *uid_node;
	uid_node = (struct mptcp_hw_ext_uid *)kzalloc(sizeof(struct mptcp_hw_ext_uid), GFP_ATOMIC);
	if (NULL == uid_node) {
		pr_err("%d kzalloc mptcp_hw_ext_uid node fail\n", __LINE__);
		return -ENOMEM;
	}
	uid_node->key_type = key_type;

	switch (key_type) {
	case ENABLE_BY_UID: {
			struct mptcp_hw_ext_uid_conf *conf;
			uid = config->conf.uid;
			uid_node->config = kzalloc(sizeof(struct mptcp_hw_ext_uid_conf), GFP_ATOMIC);
			if (NULL == uid_node->config) {
				pr_err("%d kzalloc mptcp_hw_ext_uid_conf fail\n", __LINE__);
				kfree(uid_node);
				return -ENOMEM;
			}
			uid_node->isp = NULL;

			conf = (struct mptcp_hw_ext_uid_conf *)(uid_node->config);
			conf->flags = value_type;
			/* set config for uid_node */
			ret = mptcp_hw_set_conf_values(value_type, &(conf->values),
				&(config->conf_value), config->if_name);
			if (0 != ret) {
				kfree(uid_node->config);
				kfree(uid_node);
				return ret;
			}
		}
		break;
	case ENABLE_BY_UID_DIP_DPORT:
	case ENABLE_BY_UID_DIP_DPORT_ISP: {
			struct mptcp_hw_ext_dip_list *dip_list;
			uid = config->conf.dip_dport.uid;
			uid_node->config = kzalloc(sizeof(struct mptcp_hw_ext_dip_list), GFP_ATOMIC);
			if (NULL == uid_node->config) {
				pr_err("%d kzalloc mptcp_hw_ext_dip_list fail\n", __LINE__);
				kfree(uid_node);
				return -ENOMEM;
			}
			uid_node->isp = NULL;

			dip_list = (struct mptcp_hw_ext_dip_list *)(uid_node->config);
			INIT_LIST_HEAD(&(dip_list->list));
			ret = mptcp_hw_ext_add_new_dip_node(&(dip_list->list), config,
				key_type, value_type, port_to_add);
			if (0 != ret) {
				kfree(uid_node->config);
				kfree(uid_node);
				return ret;
			}

			if (key_type == ENABLE_BY_UID_DIP_DPORT_ISP) {
				struct mptcp_hw_ext_uid_gid_carrier_info *isp;
				uid_node->isp = kzalloc(sizeof(struct mptcp_hw_ext_uid_gid_carrier_info), GFP_ATOMIC);
				if (NULL == uid_node->isp) {
					pr_err("%d kzalloc mptcp_hw_ext_uid_gid_carrier_info fail\n", __LINE__);
					kfree(uid_node->config);
					kfree(uid_node);
					return -ENOMEM;
				}

				isp = (struct mptcp_hw_ext_uid_gid_carrier_info *)(uid_node->isp);
				mptcp_hw_ext_set_uid_gid_carrier_ip(config->conf.dip_dport.dip,
					&(config->conf_value), isp);
			}
		}
		break;
	case ENABLE_BY_USR_SWITCH:
		uid = config->conf.uid;
		uid_node->usr_switch = true;
		uid_node->config = NULL;
		uid_node->isp = NULL;
		break;

	case ENABLE_BY_FIRST_PATH: {
		uid = config->conf.uid;
		(void)strncpy(uid_node->first_path.if_name, config->if_name, IFNAMSIZ);
		uid_node->first_path.timeout = config->conf_value.first_path_timeout;
		mptcp_info("%s: uid:%d if_name:%s timeout:%u\n", __func__, uid,
					uid_node->first_path.if_name,
					uid_node->first_path.timeout);
		break;
	}

	default:
		pr_err("%d unsupport key_type:%u\n", __LINE__, key_type);
		kfree(uid_node);
		return -EINVAL;
	}

	uid_node->uid = uid;

	list_add(&(uid_node->list), &mptcp_hw_ext_uid_list);
	return 0;
}


static int mptcp_hw_ext_update_uid_node_config(
	struct mptcp_hw_ext_uid *uid_node, struct mptcp_hw_ext *config,
	uint8_t key_type, uint32_t value_type, uint32_t port_to_add)
{
	int ret;

	switch (key_type) {
	case ENABLE_BY_UID: {
			struct mptcp_hw_ext_uid_conf *conf;
			if (NULL == uid_node->config) {
				/*
					a. user switch can create this uid node before enable
					b. the value_type of user switch is MPTCP_HW_CONF_VALUE_MPTCP
					b. server_proxy info can be set before enable
				 */
				if (value_type != MPTCP_HW_CONF_VALUE_MPTCP &&
					value_type != MPTCP_HW_CONF_VALUE_PROXY_INFO)
					return -EINVAL;

				uid_node->config = kzalloc(sizeof(struct mptcp_hw_ext_uid_conf), GFP_ATOMIC);
				if (NULL == uid_node->config) {
					pr_err("%d kzalloc mptcp_hw_ext_uid_conf fail\n", __LINE__);
					return -ENOMEM;
				}
				uid_node->key_type = key_type;
				uid_node->isp = NULL;
			}

			conf = (struct mptcp_hw_ext_uid_conf *)(uid_node->config);
			if ((conf->flags != MPTCP_HW_CONF_VALUE_PROXY_INFO) &&
				(conf->flags != 0) &&
				value_type == MPTCP_HW_CONF_VALUE_MPTCP) {
				pr_err("can't enable twice\n");
				return -EINVAL;
			}
			conf->flags |= value_type;
			/* update config value */
			ret = mptcp_hw_set_conf_values(value_type, &(conf->values),
				&(config->conf_value), config->if_name);
			if (ret) {
				pr_err("%d update config fail with uid:%d\n", __LINE__,
					config->conf.uid);
				return ret;
			}
		}
		break;
	case ENABLE_BY_UID_DIP_DPORT:
	case ENABLE_BY_UID_DIP_DPORT_ISP: {
			struct mptcp_hw_ext_dip_list *dip_list;
			struct mptcp_hw_ext_dip_node *dip_node;
			struct mptcp_hw_ext_dport_node *dport_node;
			uint32_t dip = config->conf.dip_dport.dip;
			char ip_str[INET_ADDRSTRLEN];

			if (NULL == uid_node->config) {
				if (value_type != MPTCP_HW_CONF_VALUE_MPTCP)
					return -EINVAL;

				uid_node->config = kzalloc(sizeof(struct mptcp_hw_ext_dip_list), GFP_ATOMIC);
				if (NULL == uid_node->config) {
					pr_err("%d kzalloc mptcp_hw_ext_dip_list fail\n", __LINE__);
					return -ENOMEM;
				}
				uid_node->isp = NULL;
				uid_node->key_type = key_type;

				if (ENABLE_BY_UID_DIP_DPORT_ISP == key_type) {
					uid_node->isp = kzalloc(sizeof(struct mptcp_hw_ext_uid_gid_carrier_info), GFP_ATOMIC);
					if (NULL == uid_node->isp) {
						pr_err("%d kzalloc mptcp_hw_ext_uid_gid_carrier_info fail\n", __LINE__);
						kfree(uid_node->config);
						return -ENOMEM;
					}
				}
				dip_list = (struct mptcp_hw_ext_dip_list *)(uid_node->config);
				INIT_LIST_HEAD(&(dip_list->list));
			} else
				dip_list = (struct mptcp_hw_ext_dip_list *)(uid_node->config);

			/* check this ip exists in dip_list or not
			if not, need check the flag is ENABLE or not, if not, return directly */
			dip_node = mptcp_hw_ext_get_node_by_dip(dip_list, dip);
			if (NULL == dip_node) {
				if (value_type != MPTCP_HW_CONF_VALUE_MPTCP) {
					pr_err("%d please make sure enable this node before you set config\n",
						__LINE__);
					return -EINVAL;
				}

				mptcp_debug("%d need create one node for dip:%s\n", __LINE__,
					anonymousIPv4addr(dip, ip_str, INET_ADDRSTRLEN));
				ret = mptcp_hw_ext_add_new_dip_node(&(dip_list->list), config,
					key_type, value_type, port_to_add);
				if (0 != ret)
					return ret;

				if (key_type == ENABLE_BY_UID_DIP_DPORT_ISP)
					mptcp_hw_ext_set_uid_gid_carrier_ip(config->conf.dip_dport.dip,
						&(config->conf_value),
						(struct mptcp_hw_ext_uid_gid_carrier_info *)(uid_node->isp));
				return 0;
			}

			/* check the port range exists in dport_node or not, if not, need add */
			if (mptcp_hw_ext_dport_match_with_exist(dip_node, config, key_type,
					value_type, port_to_add) == false) {
				pr_err("%s: valid param failed\n", __func__);
				return -EINVAL;
			}

			if (value_type == MPTCP_HW_CONF_VALUE_MPTCP) {
				mptcp_debug("%d need create need one port node for port_str:%s\n",
					__LINE__, config->conf.dip_dport.port_key);
				ret = mptcp_hw_ext_add_new_dport_node(&(dip_node->dport_list),
					config, value_type, port_to_add);
				if (0 != ret)
					return ret;

				return 0;
			}

			/* update config for dport_node */
			list_for_each_entry(dport_node, &(dip_node->dport_list), dport_node) {
				if ((0 != strncmp(dport_node->port_key, config->conf.dip_dport.port_key,
						MPTCP_HW_EXT_PORT_KEY_MAX_LEN)))
					continue;

				dport_node->flags |= value_type;
				/* note here, fail shoud not happen here */
				(void)mptcp_hw_set_conf_values(value_type, &(dport_node->values), &(config->conf_value),
					       config->if_name);
				break;

			}
		}
		break;
	case ENABLE_BY_USR_SWITCH:
		uid_node->usr_switch = !strncmp(config->if_name, "all", IFNAMSIZ);
		if (!uid_node->usr_switch) {
			(void)strncpy(uid_node->prim_iface, config->if_name, IFNAMSIZ);
			uid_node->prim_iface[IFNAMSIZ - 1] = '\0';
		}
		break;

	case ENABLE_BY_FIRST_PATH: {
		uid_node->first_path.timeout = config->conf_value.first_path_timeout;
		(void)strncpy(uid_node->first_path.if_name, config->if_name, IFNAMSIZ);
		uid_node->first_path.if_name[IFNAMSIZ - 1] = '\0';
		mptcp_info("%s: if_name:%s timeout:%u\n", __func__, uid_node->first_path.if_name,
			uid_node->first_path.timeout);
		break;
	}

	default:
		pr_err("%d unsupport key_type:%u\n", __LINE__, key_type);
		return -EINVAL;
	}

	return 0;
}


static int mptcp_hw_ext_chk_param_valid_before_use(
	struct mptcp_hw_ext *config, uint8_t key_type, uint32_t value_type,
	uint32_t *port_to_add)
{
	int ret = 0;
	bool is_add = true;

	if (NULL == port_to_add)
		is_add = false;

	switch (key_type) {
	case ENABLE_BY_UID:
	case ENABLE_BY_PID_FD: {
			ret = mptcp_hw_validate_set_conf_values(value_type,
				&(config->conf_value), config->if_name, is_add);
			if (ret) {
				pr_err("%s: validate param fail with uid type\n", __func__);
				return ret;
			}
		}
		break;
	case ENABLE_BY_UID_DIP_DPORT:
	case ENABLE_BY_UID_DIP_DPORT_ISP: {
			ret = mptcp_hw_validate_set_conf_values_ipport(config, key_type,
				value_type, port_to_add);
			if (ret) {
				pr_err("%s: validate param fail with uid_dip_dport type\n", __func__);
				return ret;
			}
		}
		break;

	case ENABLE_BY_USR_SWITCH:
		break;

	default:
		pr_err("unsupport key_type:%u\n", key_type);
		return -EINVAL;
	}

	return ret;
}


static void mptcp_hw_ext_reset_dport_list(struct list_head *dport_list)
{
	struct mptcp_hw_ext_dport_node *node, *next;

	list_for_each_entry_safe(node, next, dport_list, dport_node) {
		list_del_init(&(node->dport_node));
		kfree(node);
	}
}


static void mptcp_hw_ext_reset_dip_list(struct list_head *dip_list)
{
	struct mptcp_hw_ext_dip_node *node, *next;
	list_for_each_entry_safe(node, next, dip_list, dip_node) {
		list_del_init(&(node->dip_node));
		mptcp_hw_ext_reset_dport_list(&(node->dport_list));
		kfree(node);
	}
}


static void mptcp_hw_user_switch_sock_update(int32_t uid, bool on, const char *prim_iface)
{
	const struct hlist_nulls_node *node;
	struct tcp_sock *meta_tp;
	int32_t i;

	write_unlock_bh(&mptcp_hw_ext_lock);
	for (i = 0; i < MPTCP_HASH_SIZE; i++) {
		rcu_read_lock_bh();
		hlist_nulls_for_each_entry_rcu(meta_tp, node, &tk_hashtable[i],
					       tk_table) {
			struct sock *meta_sk = (struct sock *)meta_tp;
			struct mptcp_cb *mpcb;

			bh_lock_sock(meta_sk);
			mpcb = meta_tp->mpcb;
			if (mpcb && meta_sk->sk_uid.val == uid && meta_tp->user_switch != on) {
				if (mpcb->pm_ops->user_switch &&
				    !sock_flag(meta_sk, SOCK_DEAD) &&
				    meta_sk->sk_state == TCP_ESTABLISHED) {
					if (sock_owned_by_user(meta_sk)) {
						(void)strncpy(meta_tp->prim_iface, prim_iface, IFNAMSIZ);
						meta_tp->prim_iface[IFNAMSIZ - 1] = '\0';
						meta_tp->user_switch = on;
						if (!test_and_set_bit(MPTCP_USER_SWTCH_DEFERRED,
								      &meta_tp->tsq_flags))
							sock_hold(meta_sk);
					} else
						mpcb->pm_ops->user_switch(meta_sk, on, prim_iface);
				}
			}
			bh_unlock_sock(meta_sk);
		}
		rcu_read_unlock_bh();
	}
	write_lock_bh(&mptcp_hw_ext_lock);
}


static void mptcp_hw_ext_reset_rules(void)
{
	struct mptcp_hw_ext_uid *node, *next;

	list_for_each_entry_safe(node, next, &mptcp_hw_ext_uid_list, list) {
		list_del_init(&node->list);
		switch (node->key_type) {
		case ENABLE_BY_UID: {
				struct mptcp_hw_ext_uid_conf *conf =
					(struct mptcp_hw_ext_uid_conf *)(node->config);
				if (conf)
					kfree(conf);
			}
			break;
		case ENABLE_BY_UID_DIP_DPORT:
		case ENABLE_BY_UID_DIP_DPORT_ISP: {
				struct mptcp_hw_ext_dip_list *dip_list =
					(struct mptcp_hw_ext_dip_list *)(node->config);
				if (dip_list) {
					mptcp_hw_ext_reset_dip_list(&(dip_list->list));
					kfree(dip_list);
				}
				if (node->isp)
					kfree(node->isp);
			}
			break;
		case ENABLE_BY_USR_SWITCH:
		case ENABLE_BY_FIRST_PATH:
		default:
			break;
		}

		if (node->usr_switch && (node->key_type == ENABLE_BY_UID ||
		    node->key_type == ENABLE_BY_UID_DIP_DPORT ||
		    node->key_type == ENABLE_BY_UID_DIP_DPORT_ISP))
			mptcp_hw_user_switch_sock_update(node->uid, false, NULL);
		kfree(node);
	}
}


static void mptcp_hw_ext_reset_rules_by_uid(int32_t uid)
{
	struct mptcp_hw_ext_uid *node, *next;

	list_for_each_entry_safe(node, next, &mptcp_hw_ext_uid_list, list) {
		if (node->uid == uid) {
			list_del_init(&node->list);
			switch (node->key_type) {
			case ENABLE_BY_UID: {
					struct mptcp_hw_ext_uid_conf *conf =
						(struct mptcp_hw_ext_uid_conf *)(node->config);
					if (conf)
						kfree(conf);
				}
				break;
			case ENABLE_BY_UID_DIP_DPORT:
			case ENABLE_BY_UID_DIP_DPORT_ISP: {
					struct mptcp_hw_ext_dip_list *dip_list =
						(struct mptcp_hw_ext_dip_list *)(node->config);
					if (dip_list) {
						mptcp_hw_ext_reset_dip_list(&(dip_list->list));
						kfree(dip_list);
					}
					if (node->isp)
						kfree(node->isp);
				}
				break;
			case ENABLE_BY_USR_SWITCH:
			case ENABLE_BY_FIRST_PATH:
			default:
				break;
			}
			if (node->usr_switch && (node->key_type == ENABLE_BY_UID ||
			    node->key_type == ENABLE_BY_UID_DIP_DPORT ||
			    node->key_type == ENABLE_BY_UID_DIP_DPORT_ISP))
				mptcp_hw_user_switch_sock_update(node->uid, false, NULL);
			kfree(node);
		}
	}
}


static void mptcp_hw_ext_delete_uid_gid_carrier_ip(
	struct mptcp_hw_ext_uid_gid_carrier_info *isp,
	uint32_t dip, uint8_t gid, uint8_t carrier_id)
{
	uint8_t dst_addr_cnt;
	uint8_t i, j;
	uint32_t *paddr;

	dst_addr_cnt = isp->mptcp_hw_ext_gid_addr_tbl[gid]\
						.carrier_info[carrier_id].dst_addr_cnt;
	if (0 == dst_addr_cnt)
		return;

	paddr = &isp->mptcp_hw_ext_gid_addr_tbl[gid].carrier_info[carrier_id].dst_addr[0];

	for (i = 0; i < dst_addr_cnt; i++, paddr++) {
		if (*paddr == dip) {
			for (j = i; j < (dst_addr_cnt - 1); j++) {
				*paddr = *(paddr + 1);
				paddr++;
			}
			*paddr = 0;
			isp->mptcp_hw_ext_gid_addr_tbl[gid]\
						.carrier_info[carrier_id].dst_addr_cnt--;

			if (isp->mptcp_hw_ext_gid_addr_tbl[gid]\
						.carrier_info[carrier_id].dst_addr_idx >=
				isp->mptcp_hw_ext_gid_addr_tbl[gid]\
						.carrier_info[carrier_id].dst_addr_cnt)
				isp->mptcp_hw_ext_gid_addr_tbl[gid].carrier_info[carrier_id].dst_addr_idx
							= 0;
			break;
		}
	}

	if (i >= dst_addr_cnt) {
		mptcp_debug("%d can't find ip in isp array\n", __LINE__);
		return;
	}

	return;
}


static void mptcp_hw_ext_delete_uid_node(
	struct mptcp_hw_ext_uid *uid_node)
{
	list_del(&(uid_node->list));
	if (uid_node->config) {
		kfree(uid_node->config);
		uid_node->config = NULL;
	}
	if (uid_node->isp) {
		kfree(uid_node->isp);
		uid_node->isp = NULL;
	}

	if (uid_node->usr_switch)
		mptcp_hw_user_switch_sock_update(uid_node->uid, false, NULL);
	kfree(uid_node);
}


static void mptcp_hw_ext_delete_dip_node(struct mptcp_hw_ext_uid *uid_node,
	struct mptcp_hw_ext_dip_list *dip_list,
	struct mptcp_hw_ext_dip_node *dip_node)
{
	if (uid_node->isp)
		mptcp_hw_ext_delete_uid_gid_carrier_ip(uid_node->isp, dip_node->dip,
			dip_node->gid, dip_node->carrier_id);

	list_del_init(&dip_node->dip_node);
	kfree(dip_node);
	if (list_empty(&dip_list->list))
		mptcp_hw_ext_delete_uid_node(uid_node);
}


static void mptcp_hw_ext_delete_dport_node(
	struct mptcp_hw_ext_dport_node *dport_node)
{
	list_del_init(&(dport_node->dport_node));
	kfree(dport_node);
}


static bool mptcp_hw_ext_try_to_delete_dport_node(struct list_head *dport_list,
	struct mptcp_hw_ext *config)
{
	struct mptcp_hw_ext_dport_node *pdport_node;
	struct mptcp_hw_ext_dport_node *del_node = NULL;
	char *dport_key = config->conf.dip_dport.port_key;
	bool ret = false;

	list_for_each_entry(pdport_node, dport_list, dport_node) {
		if (0 == strncmp(pdport_node->port_key, dport_key, MPTCP_HW_EXT_PORT_KEY_MAX_LEN)) {
			del_node = pdport_node;
			break;
		}
	}

	if (del_node) {
		mptcp_hw_ext_delete_dport_node(del_node);

		ret = true;
	}
	return ret;
}


static bool mptcp_hw_ext_try_to_delete_config_in_dport_node(
	struct list_head *dport_list,
	struct mptcp_hw_ext *config, uint32_t value_type)
{
	struct mptcp_hw_ext_dport_node *pdport_node;
	char *dport_key = config->conf.dip_dport.port_key;
	bool ret = false;

	list_for_each_entry(pdport_node, dport_list, dport_node) {
		if (0 != strncmp(pdport_node->port_key, dport_key, MPTCP_HW_EXT_PORT_KEY_MAX_LEN))
			continue;

		ret = true;

		mptcp_hw_del_conf_values(value_type, &pdport_node->flags,
			&pdport_node->values, config->if_name);
		break;
	}

	return ret;
}


static int mptcp_hw_ext_delete_exist_uid_config(
	struct mptcp_hw_ext_uid *uid_node, struct mptcp_hw_ext *config,
	uint32_t value_type)
{
	switch (uid_node->key_type) {
	case ENABLE_BY_UID: {
			struct mptcp_hw_ext_uid_conf *conf =
				(struct mptcp_hw_ext_uid_conf *)(uid_node->config);
			if (value_type == MPTCP_HW_CONF_VALUE_MPTCP) {
				list_del_init(&uid_node->list);
				if (conf)
					kfree(conf);

				if (uid_node->usr_switch)
					mptcp_hw_user_switch_sock_update(uid_node->uid, false, NULL);
				mptcp_debug("%d delete config node with uid:%d\n", __LINE__,
					uid_node->uid);
				kfree(uid_node);
				return 0;
			}

			mptcp_hw_del_conf_values(value_type, &(conf->flags),
				&(conf->values), config->if_name);

			mptcp_debug("%d delete config flag:%u with uid:%d\n", __LINE__,
				value_type, uid_node->uid);
		}
		break;
	case ENABLE_BY_UID_DIP_DPORT:
	case ENABLE_BY_UID_DIP_DPORT_ISP: {
			struct mptcp_hw_ext_dip_list *dip_list;
			struct mptcp_hw_ext_dip_node *dip_node;
			struct mptcp_hw_dip_dport *dip_dport = &(config->conf.dip_dport);
			uint32_t dip = dip_dport->dip;
			char ip_str[INET_ADDRSTRLEN];
			dip_list = (struct mptcp_hw_ext_dip_list *)(uid_node->config);

			if (NULL == dip_list)
				return -EINVAL;

			/* check this ip exists in dip_list or not, if not, return directly */
			dip_node = mptcp_hw_ext_get_node_by_dip(dip_list, dip);
			if (NULL == dip_node) {
				pr_err("%d not exist in dip_list with dip:%s\n", __LINE__,
					anonymousIPv4addr(dip, ip_str, INET_ADDRSTRLEN));
				return -EINVAL;
			}

			if (value_type == MPTCP_HW_CONF_VALUE_MPTCP) {
				if (false == mptcp_hw_ext_try_to_delete_dport_node(
					&(dip_node->dport_list), config))
					return -EINVAL;
				else {
					if (list_empty(&dip_node->dport_list))
						mptcp_hw_ext_delete_dip_node(uid_node, dip_list, dip_node);
				}

				return 0;
			}

			/* try to delete config in dport_node */
			if (false == mptcp_hw_ext_try_to_delete_config_in_dport_node(
				&(dip_node->dport_list), config, value_type))
				return -EINVAL;

			return 0;
		}
		break;
	case ENABLE_BY_FIRST_PATH:
	case ENABLE_BY_USR_SWITCH: {
		mptcp_debug("%s: delete node with key_type:%u uid:%d\n",
			__func__, uid_node->key_type, uid_node->uid);
		list_del_init(&(uid_node->list));
		kfree(uid_node);
		return 0;
	}
	default:
		pr_err("%d unsupport key_type:%u\n", __LINE__, uid_node->key_type);
		return -EINVAL;
	}

	return 0;
}


static uint8_t mptcp_get_carrier_info_of_iface(const char *iface_name)
{
	int i;
	uint8_t carrier_id = MPTCP_HW_EXT_CARRIER_INVALID;

	if ((iface_name == NULL) || (strnlen(iface_name, IFNAMSIZ) == 0))
		return MPTCP_HW_EXT_CARRIER_INVALID;

	for (i = 0; i < mptcp_intf_carrier.num_intf_carrier; i++) {
		if (!strncmp(mptcp_intf_carrier.mptcp_carrier_info[i].if_name, iface_name, IFNAMSIZ)) {
			carrier_id = mptcp_intf_carrier.mptcp_carrier_info[i].carrier_id;
			break;
		}
	}

	return carrier_id;
}


uint8_t mptcp_hw_get_carrier_info_of_iface(const char *iface_name)
{
	uint8_t carrier_id;

	read_lock_bh(&mptcp_hw_ext_lock);
	carrier_id = mptcp_get_carrier_info_of_iface(iface_name);
	read_unlock_bh(&mptcp_hw_ext_lock);

	return carrier_id;
}


static __be32
mptcp_get_dst_addr_from_info(int32_t uid, uint8_t gid,
	uint8_t carrier_id, uint8_t deploy_mode)
{
	struct mptcp_hw_ext_uid_gid_carrier_info *node;
	if ((gid >= MPTCP_HW_EXT_MAX_GID) || (carrier_id >= MPTCP_HW_EXT_CARRIER_INVALID)
		|| (deploy_mode >= MPTCP_HW_EXT_DEPLOY_INVALID)) {
		pr_err("%s: Invalid gid:%u carrier_id:%u deploy_mode:%u not found \n",
			__func__, gid, carrier_id, deploy_mode);
		return 0;
	}

	node = mptcp_hw_ext_get_isp_info_by_uid(uid);
	if (NULL == node) {
		pr_err("%s: can't get right info by uid:%d\n", __func__, uid);
		return 0;
	}

	if (deploy_mode == MPTCP_HW_EXT_DEPLOY_LOADBALANCE) {
		if (node->mptcp_hw_ext_gid_addr_tbl[gid].carrier_info[carrier_id].dst_addr_idx
			>= node->mptcp_hw_ext_gid_addr_tbl[gid].carrier_info[carrier_id].dst_addr_cnt)
			node->mptcp_hw_ext_gid_addr_tbl[gid].carrier_info[carrier_id].dst_addr_idx
						= 0;

		return node->mptcp_hw_ext_gid_addr_tbl[gid].carrier_info[carrier_id]\
			.dst_addr[node->mptcp_hw_ext_gid_addr_tbl[gid]\
				.carrier_info[carrier_id].dst_addr_idx++];
	}

	return 0;
}


int mptcp_get_raddr_carrier_info(int32_t uid, __be32 raddr)
{
	int i, j, k;
	struct mptcp_hw_ext_uid_gid_carrier_info *node;

	read_lock_bh(&mptcp_hw_ext_lock);
	node = mptcp_hw_ext_get_isp_info_by_uid(uid);
	if (NULL == node) {
		pr_err("%s: can't find right node by uid:%d\n", __func__, uid);
		read_unlock_bh(&mptcp_hw_ext_lock);
		return -1;
	}

	for (i = 0; i < MPTCP_HW_EXT_MAX_GID; i++) {
		for (j = 0; j < MPTCP_HW_EXT_MAX_CARRIER_ID; j++) {
			for (k = 0; k < node->mptcp_hw_ext_gid_addr_tbl[i].carrier_info[j].dst_addr_cnt; k++) {
				if (node->mptcp_hw_ext_gid_addr_tbl[i].carrier_info[j].dst_addr[k] == raddr) {
					read_unlock_bh(&mptcp_hw_ext_lock);
					return j;
				}
			}
		}
	}

	read_unlock_bh(&mptcp_hw_ext_lock);
	return -1;
}


static void mptcp_set_sec_addr_for_sk(struct sock *sk, int32_t uid,
	struct mptcp_hw_ext_dip_node *node, __be16 dport)
{
	int match_idx = -1;
	uint8_t carrier_id;
	__be32 rem_addr;
	union inet_addr addr;
	struct mptcp_cb *mpcb;
	char ip_str[INET_ADDRSTRLEN];

	if (mptcp_is_subflow_from_iface(sk, mptcp_intf_carrier.mptcp_carrier_info[0].if_name))
		match_idx = 1;
	else
		match_idx = 0;

	carrier_id = mptcp_get_carrier_info_of_iface(
			mptcp_intf_carrier.mptcp_carrier_info[match_idx].if_name);
	if (carrier_id == MPTCP_HW_EXT_CARRIER_INVALID) {
		mptcp_debug("%s: Carrier Id not found for the secondary interface \n",
			__func__);
		return;
	}

	if (carrier_id == mptcp_get_carrier_info_of_iface(mptcp_intf_carrier.mptcp_carrier_info[!match_idx].if_name)) {
		mptcp_debug("%s: Carrier Id %d is same with %s\n", __func__, carrier_id,
			    mptcp_intf_carrier.mptcp_carrier_info[!match_idx].if_name);
		return;
	}

	rem_addr = mptcp_get_dst_addr_from_info(uid, node->gid, carrier_id, node->deploy_info);
	if (rem_addr == 0) {
		mptcp_debug("%s: rem_addr is 0", __func__);
		return;
	}

	if (rem_addr == sk->sk_daddr) {
		mptcp_debug("%s: dst IP: %s already exists\n", __func__,
			anonymousIPv4addr(rem_addr, ip_str, INET_ADDRSTRLEN));
		return;
	}

	mptcp_debug("%s: dst IP: %s added to local list \n", __func__,
		anonymousIPv4addr(rem_addr, ip_str, INET_ADDRSTRLEN));

	addr.in.s_addr = rem_addr;
	mpcb = tcp_sk(sk)->mpcb;

	if (mpcb->pm_ops->add_raddr)
		mpcb->pm_ops->add_raddr(mpcb, &addr, AF_INET, dport,
					fullmesh_get_rem_locid(mpcb));

	mptcp_path_array_check(mptcp_meta_sk(sk));

}


void mptcp_add_rem_addr_for_sk(struct sock *meta_sk)
{
	struct tcp_sock *meta_tp = tcp_sk(meta_sk);
	struct mptcp_hw_ext_dip_node *dip_node;
	struct mptcp_hw_ext_dport_node *dport_node;
	struct sock *sk;

	if (meta_tp->mptcp_cap_flag != MPTCP_CAP_UID_DIP_DPORT)
		return;

	read_lock_bh(&mptcp_hw_ext_lock);
	dip_node = mptcp_hw_ext_get_node_by_uid_dip(sock_i_uid(meta_sk).val,
		meta_sk->sk_daddr);
	if (NULL == dip_node) {
		read_unlock_bh(&mptcp_hw_ext_lock);
		return;
	}

	dport_node = mptcp_hw_ext_get_node_by_dport(&(dip_node->dport_list),
		ntohs(meta_sk->sk_dport));
	if (dport_node) {
		mptcp_for_each_sk(meta_tp->mpcb, sk) {
			mptcp_set_sec_addr_for_sk(sk, sock_i_uid(meta_sk).val, dip_node,
				meta_sk->sk_dport);
			break;
		}
	}
	read_unlock_bh(&mptcp_hw_ext_lock);
}


char *anonymousIPv4addr(__be32 ip, char *buf, size_t size)
{
	char *pos;

	if ((NULL == buf) || (0 == size))
		return "NULL";

	(void)memset(buf, 0, size);
	if ((size < INET_ADDRSTRLEN) ||
			(snprintf(buf, INET_ADDRSTRLEN, "%pI4", &ip) <= 0)) {
		buf[0] = '\0';
		return buf;
	}

	pos = strrchr(buf, '.');
	if (NULL == pos) {
		buf[0] = '\0';
		return buf;
	}

	strncpy(pos + 1, "***", 3);
	*(pos + 4) = '\0';

	return buf;
}


static bool mptcp_hw_ext_match_key_type(uint32_t value_type,
	uint8_t old_key_type, uint8_t new_key_type, bool is_add)
{
	bool ret = true;
	switch (old_key_type) {
	case ENABLE_BY_USR_SWITCH:
	case ENABLE_BY_FIRST_PATH:
		break;

	case ENABLE_BY_UID: {
		if (is_add && (ENABLE_BY_UID_DIP_DPORT == new_key_type ||
			ENABLE_BY_UID_DIP_DPORT_ISP == new_key_type))
			ret = false;

		if (!is_add && new_key_type != ENABLE_BY_UID)
			ret = false;

		break;
	}

	case ENABLE_BY_UID_DIP_DPORT:
	case ENABLE_BY_UID_DIP_DPORT_ISP: {
		if (ENABLE_BY_UID == new_key_type) {
			ret = false;
			break;
		}

		if (is_add && MPTCP_HW_CONF_VALUE_MPTCP == value_type) {
			if (old_key_type != new_key_type)
				ret = false;
		}

		break;
	}
	default:
		break;
    }

    return ret;
}


static int mptcp_hw_ext_add_config(struct mptcp_hw_ext *config,
	uint8_t key_type, uint32_t value_type)
{
	int32_t uid;
	int ret;
	uint32_t port_to_add;

	struct mptcp_hw_ext_uid *uid_node;

	/* 1.  distinguish DIP_DPORT_ISP from DIP_DPORT here */
	if (key_type == ENABLE_BY_UID_DIP_DPORT && value_type == MPTCP_HW_CONF_VALUE_MPTCP) {
		if (config->conf_value.i_val.gid != MPTCP_HW_EXT_INVALID_GID)
			key_type = ENABLE_BY_UID_DIP_DPORT_ISP;
	}

	/* 2. check param valid */
	ret = mptcp_hw_ext_chk_param_valid_before_use(config, key_type,
		value_type, &port_to_add);
	if (0 != ret)
		return ret;

	switch (key_type) {
	case ENABLE_BY_UID:
		uid = config->conf.uid;
		break;
	case ENABLE_BY_UID_DIP_DPORT:
	case ENABLE_BY_UID_DIP_DPORT_ISP:
		uid = config->conf.dip_dport.uid;
		break;
	case ENABLE_BY_PID_FD:
		return mptcp_hw_ext_set_config_by_pid_fd(config, value_type);
	default:
		pr_err("unsupport key_type:%u\n", key_type);
		return -EINVAL;
	}

	/* 3. check the node existed or not, if not, create one new node */
	uid_node = mptcp_hw_ext_uid_in_list(uid);
	if (uid_node) {
		/* 4.1 if existed,  check key type */
		if (false == mptcp_hw_ext_match_key_type(value_type, uid_node->key_type,
				key_type, true)) {
			pr_err("%d key_type different from last time with uid:%d\n", __LINE__, uid);
			return -EINVAL;
		}

		if (0 != mptcp_hw_ext_update_uid_node_config(uid_node, config, key_type,
				value_type, port_to_add)) {
			pr_err("%d update uid_node config with uid:%d fail\n", __LINE__, uid);
			return -EINVAL;
		}

		return 0;
	}

	/* 4. 2.1 server_proxy info can be set before enable, only support by uid now */
	if (key_type == ENABLE_BY_UID &&
			(value_type == MPTCP_HW_CONF_VALUE_PROXY_INFO))
		goto add_new_uid_node;

	/*
	4.2.2 if not existed, check flag is ENABLE or not, if not, return directly
	note, the value_type of user switch is MPTCP_HW_CONF_VALUE_MPTCP
	*/
	if (value_type != MPTCP_HW_CONF_VALUE_MPTCP) {
		pr_err("%d please make sure you have enable mp for this node before setting config\n", __LINE__);
		return -EINVAL;
	}

add_new_uid_node:
	if (0 != mptcp_hw_ext_add_new_uid_node(config, key_type, value_type, port_to_add)) {
		pr_err("%d add new uid_node with uid:%d fail\n", __LINE__, uid);
		return -EINVAL;
	}

	return 0;
}



static int mptcp_hw_ext_del_config(struct mptcp_hw_ext *config,
	uint8_t key_type, uint32_t value_type)
{
	int32_t uid;
	struct mptcp_hw_ext_uid *uid_node;
	int ret;

	/* check param valid firstly */
	ret = mptcp_hw_ext_chk_param_valid_before_use(config, key_type,
		value_type, NULL);
	if (0 != ret)
		return ret;

	switch (key_type) {
	case ENABLE_BY_UID:
		uid = config->conf.uid;
		break;
	case ENABLE_BY_UID_DIP_DPORT:
	case ENABLE_BY_UID_DIP_DPORT_ISP:
		uid = config->conf.dip_dport.uid;
		break;
	case ENABLE_BY_PID_FD:
		return mptcp_hw_ext_set_config_by_pid_fd(config, value_type);
	default:
		return -EINVAL;
	}

	/* try to find the node, if existed, then try to delete the config */
	uid_node = mptcp_hw_ext_uid_in_list(uid);
	if (NULL == uid_node) {
		pr_err("%d the node with uid:%d not exist\n", __LINE__, uid);
		return -EINVAL;
	}

	if (false == mptcp_hw_ext_match_key_type(value_type, uid_node->key_type,
				key_type, false)) {
		pr_err("%d key_type different from add with uid:%d\n", __LINE__, uid);
		return -EINVAL;
	}

	ret = mptcp_hw_ext_delete_exist_uid_config(uid_node, config, value_type);
	if (ret)
		return ret;

	return 0;
}


static int mptcp_hw_ext_set_interface_isp_info(struct mptcp_hw_ext *ext)
{
	uint8_t i;
	for (i = 0; i < mptcp_intf_carrier.num_intf_carrier; i++) {
		if (strncmp(mptcp_intf_carrier.mptcp_carrier_info[i]
				    .if_name,
			    ext->conf.carrier_info.if_name,
			    IFNAMSIZ) == 0){
			mptcp_info("%s: update interface %s carrier_id from %d to %d\n",
				__func__,
				ext->conf.carrier_info.if_name,
				mptcp_intf_carrier.mptcp_carrier_info[i].carrier_id,
				ext->conf.carrier_info.carrier_id);
			mptcp_intf_carrier.mptcp_carrier_info[i].carrier_id =
				ext->conf.carrier_info.carrier_id;
			return 0;
		}
	}

	if (mptcp_intf_carrier.num_intf_carrier < MPTCP_IF_CARRIER_NUM_MAX) {
		mptcp_info("%s: add interface %s carrier_id %d\n",
			__func__,
			ext->conf.carrier_info.if_name,
			ext->conf.carrier_info.carrier_id);
		mptcp_intf_carrier.mptcp_carrier_info
			[mptcp_intf_carrier.num_intf_carrier++] =
			ext->conf.carrier_info;
		return 0;
	} else {
		pr_err(
			"%s: limit:%u same interface not found and max interface configured\n",
			__func__, mptcp_intf_carrier.num_intf_carrier);
		return -EINVAL;
	}
}


static int mptcp_hw_chck_action_type(uint32_t flag,
				      union mptcp_hw_ext_value *value)
{
	int action_is_add = 0;

	switch (flag) {
	case MPTCP_HW_CONF_VALUE_MPTCP: {
		action_is_add = (value->i_val.mp_capability) ? 1 : 0;
		break;
	}

	case MPTCP_HW_CONF_VALUE_SND_RCV_BUF: {
		if (value->buf.snd_buf || value->buf.rcv_buf)
			action_is_add = 1;
		break;
	}

	case MPTCP_HW_CONF_VALUE_SCHEDULER: {
		if (value->sched_name[0] != 0)
			action_is_add = 1;
		break;
	}

	case MPTCP_HW_CONF_VALUE_SCHED_PARAMS: {
		if (value->sched_params)
			action_is_add = 1;
		break;
	}

	case MPTCP_HW_CONF_VALUE_PROXY_INFO: {
		if (value->proxy_info.proxy_ip)
			action_is_add = 1;
		break;
	}

	case MPTCP_HW_CONF_VALUE_SUBFLOW_PRIO: {
		if (value->subflow_prio[0].if_name[0] != '\0')
			action_is_add = 1;
		break;
	}

	default:
		break;
	}

	return action_is_add;
}


static int mptcp_hw_user_switch(struct mptcp_hw_ext *ext)
{
	struct mptcp_hw_ext_uid *uid_node;
	int32_t uid = ext->conf.uid;
	bool on;
	int ret = 0;

	on = !strncmp(ext->if_name, "all", IFNAMSIZ);
	mptcp_info("%s: set uid %d switch %d\n", __func__, uid, on);
	uid_node = mptcp_hw_ext_uid_in_list(uid);
	if (uid_node) {
		if (uid_node->key_type == ENABLE_BY_USR_SWITCH && !on) {
			ret = mptcp_hw_ext_delete_exist_uid_config(uid_node, ext, 0);
			if (ret != 0) {
				pr_err("%s: mptcp_hw_ext_delete_exist_uid_config uid %d failed\n", __func__, uid);
				return ret;
			}
		} else {
			(void)mptcp_hw_ext_update_uid_node_config(uid_node, ext,
				ENABLE_BY_USR_SWITCH, 0, 0);
		}
	} else {
		if (on) {
			ret = mptcp_hw_ext_add_new_uid_node(ext, ENABLE_BY_USR_SWITCH, 0, 0);
			if (ret != 0) {
				pr_err("%s: mptcp_hw_ext_add_new_uid_node uid %d failed\n", __func__, uid);
				return ret;
			}
		}
	}

	mptcp_hw_user_switch_sock_update(uid, on, ext->if_name);

	return ret;
}


static int mptcp_hw_ext_set_first_path_info(struct mptcp_hw_ext *ext)
{
	size_t len;
	struct mptcp_hw_ext_uid *uid_node;
	int32_t uid = ext->conf.uid;
	int ret;

	len = strnlen(ext->if_name, IFNAMSIZ);
	if ((len == 0) || (len == IFNAMSIZ) || 0 == ext->conf_value.first_path_timeout || 0 >= uid) {
		pr_err("%s: ifname len or timeout or uid invalid\n", __func__);
		return -EINVAL;
	}

	uid_node = mptcp_hw_ext_uid_in_list(uid);

	if (uid_node) {
		ret = mptcp_hw_ext_update_uid_node_config(uid_node, ext, ENABLE_BY_FIRST_PATH, 0, 0);
		if (ret != 0) {
			pr_err("%s: mptcp_hw_ext_update_uid_node_config uid %d failed\n", __func__, uid);
			return ret;
		}
	} else {
		ret = mptcp_hw_ext_add_new_uid_node(ext, ENABLE_BY_FIRST_PATH, 0, 0);
		if (ret != 0) {
			pr_err("%s: mptcp_hw_ext_add_new_uid_node uid %d failed\n", __func__, uid);
			return ret;
		}
	}

	return ret;
}


int mptcp_hw_ext_cmd_get_sockopt(struct mptcp_hw_ext *ext)
{
	unsigned int key_type;
	unsigned int action;
	unsigned int value_type;

	if (ext->cmd >= MPTCP_HW_EXT_CMD_MAX_INVALID ||
		((ext->cmd & 0xFFFFFF) >= MPTCP_HW_EXT_MAX_VALUE_TYPE)) {
		pr_err("%s cmd exceeds the maximum value\n", __func__);
		return -ENOTSUPP;
	}

	key_type = mptcp_hw_ext_get_cmd_type(ext->cmd);
	mptcp_hw_ext_get_conf_type(ext->cmd, value_type);
	action = mptcp_hw_chck_action_type(value_type, &ext->conf_value);

	mptcp_debug("%s: cmd_type:%u, cfg_type:%u\n", __func__, key_type,
		    value_type);
	switch (key_type) {
	case MPTCP_HW_CONF_KEY_PID_FD: {
		switch (value_type) {
		case MPTCP_HW_CONF_VALUE_MPTCP_INFO:
			return mptcp_hw_ext_pid_fd_get_mptcp_info(
				ext->conf.pid_fd.pid, ext->conf.pid_fd.fd,
				&ext->conf_value, ext->if_name);

		default:
			pr_err("%s: unsupported cmd %d value_type:%u\n", __func__,
				ext->cmd, value_type);
			return -ENOTSUPP;
		}
	}

	default:
		pr_err("%s: unsupported cmd %d\n", __func__,
			ext->cmd);
		return -ENOTSUPP;
	}
}
EXPORT_SYMBOL(mptcp_hw_ext_cmd_get_sockopt);



int mptcp_hw_ext_cmd_set_sockopt(struct mptcp_hw_ext *ext)
{
	unsigned int key_type;
	unsigned int action;
	uint32_t value_type;
	int ret = -ENOTSUPP;

	if (ext->cmd >= MPTCP_HW_EXT_CMD_MAX_INVALID ||
		((ext->cmd & 0xFFFFFF) >= MPTCP_HW_EXT_MAX_VALUE_TYPE)) {
		pr_err("%s cmd exceeds the maximum value\n", __func__);
		return ret;
	}

	key_type = mptcp_hw_ext_get_cmd_type(ext->cmd);
	mptcp_hw_ext_get_conf_type(ext->cmd, value_type);
	action = mptcp_hw_chck_action_type(value_type, &ext->conf_value);

	mptcp_info("%s: key_type:%u value_type:%u action:%u\n", __func__,
		    key_type, value_type, action);
	write_lock_bh(&mptcp_hw_ext_lock);
	switch (key_type) {
	case MPTCP_HW_CONF_KEY_UID: {
		if (action)
			ret = mptcp_hw_ext_add_config(ext, ENABLE_BY_UID, value_type);
		else
			ret = mptcp_hw_ext_del_config(ext, ENABLE_BY_UID, value_type);
		break;
	}

	case MPTCP_HW_CONF_KEY_IP_PORT: {
		if (action)
			ret = mptcp_hw_ext_add_config(ext, ENABLE_BY_UID_DIP_DPORT, value_type);
		else
			ret = mptcp_hw_ext_del_config(ext, ENABLE_BY_UID_DIP_DPORT, value_type);
		break;
	}


	case MPTCP_HW_CONF_KEY_PID_FD: {
		if (action)
			ret = mptcp_hw_ext_add_config(ext, ENABLE_BY_PID_FD, value_type);
		else
			ret = mptcp_hw_ext_del_config(ext, ENABLE_BY_PID_FD, value_type);
		break;
	}

	case MPTCP_HW_CONF_KEY_RESET: {
		if (ext->conf.uid != 0)
			mptcp_hw_ext_reset_rules_by_uid(ext->conf.uid);
		else
			mptcp_hw_ext_reset_rules();
		ret = 0;
		break;
	}

	case MPTCP_HW_CONF_KEY_CARRIER_INFO_SET: {
		ret = mptcp_hw_ext_set_interface_isp_info(ext);
		break;
	}

	case MPTCP_HW_CONF_KEY_CARRIER_INFO_RESET: {
		mptcp_debug("%s: reset all carrier_id\n", __func__);
		mptcp_intf_carrier.num_intf_carrier = 0;
		(void)memset(mptcp_intf_carrier.mptcp_carrier_info, 0,
		       sizeof(mptcp_intf_carrier.mptcp_carrier_info));
		ret = 0;
		break;
	}

	case MPTCP_HW_CONF_KEY_UID_SWITCH: {
		ret = mptcp_hw_user_switch(ext);
		break;
	}

	case MPTCP_HW_CONF_KEY_UID_FIRST_PATH: {
		ret = mptcp_hw_ext_set_first_path_info(ext);
		break;
	}

	default:
		pr_err("%s: unsupported cmd %d\n", __func__, ext->cmd);
		ret = -ENOTSUPP;
		break;
	}

	write_unlock_bh(&mptcp_hw_ext_lock);
	return ret;
}
EXPORT_SYMBOL(mptcp_hw_ext_cmd_set_sockopt);


static bool is_ipv4_private_address(__be32 addr)
{
	return (ipv4_is_private_10(addr) || ipv4_is_private_172(addr) || ipv4_is_private_192(addr));
}


static bool check_ip_addrss_for_mptcp_available(struct sockaddr *addr)
{
	if (addr->sa_family == AF_INET) {
		struct sockaddr_in *usin = (struct sockaddr_in *)addr;
		return !ipv4_is_loopback(usin->sin_addr.s_addr) && !ipv4_is_multicast(usin->sin_addr.s_addr)
			&& !ipv4_is_zeronet(usin->sin_addr.s_addr) && !ipv4_is_lbcast(usin->sin_addr.s_addr);
	}
#if IS_ENABLED(CONFIG_IPV6)
	else if (addr->sa_family == AF_INET6) {
		struct sockaddr_in6 *usin6 = (struct sockaddr_in6 *)addr;
		return !ipv6_addr_loopback(&usin6->sin6_addr) && !ipv6_addr_is_multicast(&usin6->sin6_addr);
	}
#endif

	return true;
}


static bool check_ip_addrss_public_available(struct sockaddr *addr)
{
	struct sockaddr_in *usin = (struct sockaddr_in *)addr;
	if (usin->sin_family == AF_INET) {
		return !ipv4_is_linklocal_169(usin->sin_addr.s_addr) && !is_ipv4_private_address(usin->sin_addr.s_addr);
	} else if (usin->sin_family == AF_INET6) {
		struct sockaddr_in6 *usin6 = (struct sockaddr_in6 *)addr;
		int addr_type = ipv6_addr_type(&usin6->sin6_addr);
		if (addr_type & IPV6_ADDR_MAPPED) {
			__be32 s_addr = usin6->sin6_addr.s6_addr32[3];
			char src_ip_str[INET_ADDRSTRLEN];

			mptcp_debug("%s: sin_family=%u ip=%s\n",  __func__, usin->sin_family,
				anonymousIPv4addr(s_addr, src_ip_str, INET_ADDRSTRLEN));

			return !ipv4_is_linklocal_169(s_addr) && !is_ipv4_private_address(s_addr);
		}
	}

	return false;
}


static void mptcp_enable_first_path(struct sock *sk, char *first_path_iface)
{
	if (!sk->sk_bound_dev_if && !sk->sk_rcv_saddr) {
		struct net_device *net_dev = dev_get_by_name(sock_net(sk), first_path_iface);
		if (net_dev) {
			unsigned int flags = dev_get_flags(net_dev);
			if (flags & IFF_RUNNING) {
				sk->sk_bound_dev_if = net_dev->ifindex;
				sk_dst_reset(sk);
				mptcp_debug("%s: enable first_path, iface:%s, flags=%x.\n", __func__, first_path_iface, flags);
			}
			dev_put(net_dev);
		}
	}
}


static void mptcp_enable_first_path_search_uid(struct sock *sk)
{
	struct mptcp_hw_ext_uid *uid_node;
	int32_t uid;

	uid = sock_i_uid(sk).val;

	read_lock_bh(&mptcp_hw_ext_lock);
	uid_node = mptcp_hw_ext_uid_in_list(uid);

	if (NULL == uid_node) {
		read_unlock_bh(&mptcp_hw_ext_lock);
		return;
	}
	if (uid_node->first_path.if_name[0] != '\0') {
		mptcp_enable_first_path(sk, uid_node->first_path.if_name);
	}

	read_unlock_bh(&mptcp_hw_ext_lock);
}


static int32_t in_mptcp_uid_list(struct sock *sk, struct sockaddr_in *daddr,
			struct mptcp_hw_ext_conf_value *values,
			struct mptcp_hw_ext_first_path *first_path)
{
	struct mptcp_hw_ext_uid *uid_node;
	uint8_t key_type;
	int32_t flags = 0;
	struct sock *meta_sk;
	int32_t uid;
	struct tcp_sock *tp = tcp_sk(sk);

	meta_sk = tp->meta_sk ? tp->meta_sk : sk;
	uid = sock_i_uid(meta_sk).val;
	uid_node = mptcp_hw_ext_uid_in_list(uid);
	if (NULL == uid_node)
		return flags;

	key_type = uid_node->key_type;
	switch (key_type) {
	case ENABLE_BY_UID: {
			struct mptcp_hw_ext_uid_conf *conf;
			conf = (struct mptcp_hw_ext_uid_conf *)(uid_node->config);
			if (conf && (conf->flags & MPTCP_HW_CONF_VALUE_MPTCP)) {
				flags = conf->flags;
				*values = conf->values;
				tp->mptcp_cap_flag = MPTCP_CAP_UID;
				tp->user_switch = uid_node->usr_switch;
			}
		}
		break;
	case ENABLE_BY_UID_DIP_DPORT:
	case ENABLE_BY_UID_DIP_DPORT_ISP: {
			struct mptcp_hw_ext_dip_list *dip_list;
			struct mptcp_hw_ext_dip_node *dip_node;
			struct mptcp_hw_ext_dport_node *dport_node;

			dip_list = (struct mptcp_hw_ext_dip_list *)(uid_node->config);
			if (NULL == dip_list)
				return flags;

			dip_node = mptcp_hw_ext_get_node_by_dip(dip_list, daddr->sin_addr.s_addr);
			if (NULL == dip_node)
				return flags;

			dport_node = mptcp_hw_ext_get_node_by_dport(&(dip_node->dport_list),
				ntohs(daddr->sin_port));
			if (NULL == dport_node)
				return flags;

			if (dport_node->flags & MPTCP_HW_CONF_VALUE_MPTCP) {
				flags = dport_node->flags;
				*values = dport_node->values;
				tp->user_switch = uid_node->usr_switch;
				tp->mptcp_cap_flag = MPTCP_CAP_UID_DIP_DPORT;
			}
		}
		break;
	case ENABLE_BY_USR_SWITCH:
	default:
		break;
	}

	if (flags && uid_node->first_path.if_name[0] != '\0') {
		*first_path = uid_node->first_path;
	}

	return flags;
}


void mptcp_init_tcp_sock(struct sock *sk,
				       struct sockaddr_in *daddr, int addr_len)
{
	int32_t flags = 0, err;
	struct mptcp_hw_ext_conf_value value;
	struct mptcp_hw_ext_first_path first_path;
	struct tcp_sock *tp = tcp_sk(sk);

	if (!tp || mptcp_init_failed || (sysctl_mptcp_enabled != MPTCP_SYSCTL)) {
		mptcp_debug("%s: mp not open\n", __func__);
		return;
	}

	if (daddr->sin_family == AF_INET) {
		if (addr_len < sizeof(struct sockaddr_in)) {
			mptcp_debug("%s: AF_INET addr_len is %d\n", __func__, addr_len);
			return;
		}
	} else if (daddr->sin_family == AF_INET6) {
		if (addr_len < SIN6_LEN_RFC2133) {
			mptcp_debug("%s: AF_INET6 addr_len is %d\n", __func__, addr_len);
			return;
		}
	} else {
		mptcp_debug("%s: unsupport sin_family %u\n", __func__, daddr->sin_family);
		return;
	}

	if (!check_ip_addrss_for_mptcp_available((struct sockaddr *)daddr)) {
		mptcp_debug("%s: sk addrss is not available\n", __func__);
		return;
	}

	if (tp->mptcp_cap_flag) {
		mptcp_debug("%s: enable mptcp by pid_fd\n", __func__);
		mptcp_enable_sock(sk);
		if (sock_flag(sk, SOCK_MPTCP))
			mptcp_enable_first_path_search_uid(sk);
		return;
	}

	first_path.if_name[0] = '\0';
	read_lock_bh(&mptcp_hw_ext_lock);
	flags = in_mptcp_uid_list(sk, daddr, &value, &first_path);
	read_unlock_bh(&mptcp_hw_ext_lock);
	mptcp_debug("%s: flags:%d user_switch:%d\n", __func__, flags, tp->user_switch);

	if (0 == tp->mptcp_cap_flag)
		return;

	mptcp_enable_sock(sk);
	if (sock_flag(sk, SOCK_MPTCP)) {
		sk->sk_userlocks |= SOCK_RCVBUF_LOCK;
		sk->sk_rcvbuf = 4 * sysctl_rmem_max;
	}

	if (sock_flag(sk, SOCK_MPTCP) && first_path.if_name[0] != '\0')
		mptcp_enable_first_path(sk, first_path.if_name);

	if (flags & MPTCP_HW_CONF_VALUE_SCHEDULER) {
		err = mptcp_set_scheduler(sk, value.sched.sched_name);
		if (err) {
			pr_err("set scheduler failed, scheduler name:%s\n",
				value.sched.sched_name);
			return;
		}
		(void)strncpy(tp->prim_iface, value.sched.if_name, IFNAMSIZ);

		if (flags & MPTCP_HW_CONF_VALUE_SCHED_PARAMS) {
			*(u32 *)tp->mptcp_sched_params = value.sched_params;
			mptcp_debug("%s rtt_sched:%u sched_param:%u\n",
				__func__, value.sched_params,
				*(u32 *)tp->mptcp_sched_params);
		}
	}

	if (flags & MPTCP_HW_CONF_VALUE_PROXY_INFO && check_ip_addrss_public_available((struct sockaddr *)daddr)) {
		struct sockaddr_in *usvr_addr = (struct sockaddr_in *)&tp->server_addr;
		usvr_addr->sin_addr.s_addr = value.proxy_info.proxy_ip;
		usvr_addr->sin_port = htons(value.proxy_info.proxy_port);
	}
}


void mptcp_init_sub_sock(struct sock *sk, bool master_sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct tcp_sock *meta_tp;
	struct sock *meta_sk;
	uint8_t i;

	if (!tp || mptcp_init_failed || (sysctl_mptcp_enabled != MPTCP_SYSCTL))
		return;

	meta_sk = tp->meta_sk;
	meta_tp = tcp_sk(meta_sk);

	/* Set the primary interface type required for scheduler */
	if (mptcp_is_subflow_from_iface(sk, meta_tp->prim_iface))
		tp->mptcp_sched_prim_intf = 1;

	for (i = 0; i < MAX_SUPPORT_SUBFLOW_NUM; i++) {
		if (0 == strnlen(meta_tp->hw_subflows[i].if_name, IFNAMSIZ)) {
			continue;
		}

		if (mptcp_is_subflow_from_iface(sk, meta_tp->hw_subflows[i].if_name)) {
			if (meta_tp->hw_subflows[i].snd_buf) {
				sk->sk_userlocks |= SOCK_SNDBUF_LOCK;
				sk->sk_sndbuf = meta_tp->hw_subflows[i].snd_buf;
				sk->sk_write_space(sk);
			}

			if (meta_tp->hw_subflows[i].rcv_buf) {
				sk->sk_userlocks |= SOCK_RCVBUF_LOCK;
				sk->sk_rcvbuf = meta_tp->hw_subflows[i].rcv_buf;
			}

			if (tp->mptcp)
				mptcp_debug("%s %d match if_name:%s at_meta_low_prio:%u tp_low_prio:%u\n",
					__func__, __LINE__,
					meta_tp->hw_subflows[i].if_name,
					meta_tp->hw_subflows[i].low_prio, tp->mptcp->low_prio);

			mptcp_hw_ext_mod_subflow_prio(tp->meta_sk);

			mptcp_debug("enable snd: %u rcv: %u buff\n",
				meta_tp->hw_subflows[i].snd_buf,
				meta_tp->hw_subflows[i].rcv_buf);
		}
	}

	if (master_sk && meta_tp->user_switch &&
	    meta_tp->mptcp_cap_flag == MPTCP_CAP_UID_DIP_DPORT) {
		int32_t uid = sock_i_uid(tp->meta_sk).val;
		struct mptcp_hw_ext_dip_node *dip_node;

		read_lock_bh(&mptcp_hw_ext_lock);
		dip_node = mptcp_hw_ext_get_node_by_uid_dip(uid, meta_sk->sk_daddr);
		if (dip_node)
			mptcp_set_sec_addr_for_sk(sk, uid, dip_node,
						  meta_sk->sk_dport);
		read_unlock_bh(&mptcp_hw_ext_lock);
	}
}


int32_t mptcp_hw_ext_get_port_key(struct sock *sk, char *port_key, size_t len)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct mptcp_hw_ext_dport_node *dport_node;
	int32_t ret = -EINVAL;

	if (!sk || sk->sk_family != AF_INET || sk->sk_state != TCP_ESTABLISHED) {
		mptcp_debug("%s: Invalid sk\n", __func__);
		return ret;
	}

	if (!port_key || len <= 0) {
		mptcp_debug("%s: Invalid key or len\n", __func__);
		return ret;
	}

	if (tp->mptcp_cap_flag != MPTCP_CAP_UID_DIP_DPORT) {
		mptcp_debug("%s: Invalid mptcp_cap_flag %d\n", __func__,
			    tp->mptcp_cap_flag);
		return ret;
	}

	read_lock_bh(&mptcp_hw_ext_lock);
	dport_node = mptcp_hw_ext_get_node_by_uid_dip_dport(sock_i_uid(sk).val,
		sk->sk_daddr, ntohs(sk->sk_dport));
	if (dport_node) {
		(void)strncpy(port_key, dport_node->port_key, len);
		port_key[len - 1] = '\0';
		ret = 0;
	} else {
		mptcp_debug("%s: Invalid dip_node or dip_port_node\n", __func__);
	}
	read_unlock_bh(&mptcp_hw_ext_lock);

	return ret;
}
EXPORT_SYMBOL(mptcp_hw_ext_get_port_key);


bool mptcp_hw_ext_get_switch_by_uid(int32_t uid)
{
	bool is_on = false;
	struct mptcp_hw_ext_uid *uid_node;

	read_lock_bh(&mptcp_hw_ext_lock);
	uid_node = mptcp_hw_ext_uid_in_list(uid);
	if (uid_node)
		is_on = uid_node->usr_switch;
	read_unlock_bh(&mptcp_hw_ext_lock);

	return is_on;
}


int mptcp_is_subflow_from_iface(struct sock *sk, const char *iface_name)
{
	struct net_device *net_dev;
	struct in_device *in_dev;
	struct in_ifaddr *addr;
	struct net *net = sock_net(sk);

	net_dev = dev_get_by_name(net, iface_name);
	if (!net_dev)
		return 0;

	in_dev = (struct in_device *)net_dev->ip_ptr;
	for (addr = in_dev->ifa_list; addr; addr = addr->ifa_next) {
		if (sk->sk_rcv_saddr == addr->ifa_address) {
			dev_put(net_dev);
			return 1;
		}
	}

	dev_put(net_dev);
	return 0;
}
EXPORT_SYMBOL(mptcp_is_subflow_from_iface);


void mptcp_proxy_syn_options(const struct sock *sk, struct tcp_out_options *opts,
	unsigned int *remaining)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	struct sockaddr_in *server_addr = (struct sockaddr_in *) (&(tp->server_addr));

	if (server_addr->sin_addr.s_addr == 0 || server_addr->sin_port == 0) {
		return;
	}
	if (*remaining < PROXY_MPTCP_OP_LEN_OPT_SERVER_ADDR4) {
		(void)mptcp_proxy_fallback((struct sock *)sk, MPTCP_FALLBACK_PROXY_OPTLEN_OFR, true);
		tcp_write_err((struct sock *)sk);

		mptcp_debug("%s: not enough space to add new option.\n", __func__);
		return;
	}
	opts->options |= OPTION_MPTCP_PROXY;
	*remaining -= PROXY_MPTCP_OP_LEN_OPT_SERVER_ADDR4;
}


void mptcp_proxy_options_write(__be32 *ptr, struct tcp_sock *tp,
	const struct tcp_out_options *opts, struct sk_buff *skb)
{
	if (unlikely(OPTION_MPTCP_PROXY & opts->options)) {
		struct sockaddr_in *server_addr;
		struct mptcp_proxy_server_addr *mp_proxy_addr = (struct mptcp_proxy_server_addr *)ptr;
		mp_proxy_addr->kind = TCPOPT_PROXY_MPTCP;
		mp_proxy_addr->len = MPTCP_PROXY_OP_LEN_OPT_SERVER_ADDR4;

		server_addr = (struct sockaddr_in *) (&(tp->server_addr));
		mp_proxy_addr->addr.s_addr = ntohl(server_addr->sin_addr.s_addr);
		mp_proxy_addr->port = ntohs(server_addr->sin_port);
		ptr += MPTCP_PROXY_OP_LEN_OPT_SERVER_ADDR4 >> 2;
	}
}


void mptcp_proxy_rewrite_dst_addr(struct sock *sk, struct sockaddr *uaddr)
{
	const struct tcp_sock *tp = tcp_sk(sk);
	if (sock_flag(sk, SOCK_MPTCP) && mptcp_doit(sk) && is_master_tp(tp) && check_ip_addrss_public_available(uaddr)) {
		struct sockaddr_in *server_addr = (struct sockaddr_in *) (&(tp->server_addr));

		if (server_addr->sin_addr.s_addr != 0 && server_addr->sin_port != 0) {
			struct sockaddr tmp_server_addr;
			struct sockaddr_in *tmp_usin = (struct sockaddr_in *)(&tmp_server_addr);
			struct sockaddr_in *usin = (struct sockaddr_in *)uaddr;
			char src_ip_str[INET_ADDRSTRLEN];
			char dst_ip_str[INET_ADDRSTRLEN];

			mptcp_info("%s: rewrite src_addr:%s:%d to dst_addr:%s:%d\n",
				    __func__,
				    anonymousIPv4addr(usin->sin_addr.s_addr, src_ip_str, INET_ADDRSTRLEN),
				    ntohs(usin->sin_port),
				    anonymousIPv4addr(server_addr->sin_addr.s_addr, dst_ip_str, INET_ADDRSTRLEN),
				    ntohs(server_addr->sin_port));

			(void)memcpy((void *)tmp_usin, (const void *)server_addr, sizeof(struct sockaddr));
			(void)memcpy((void *)server_addr, (const void *)usin, sizeof(struct sockaddr));
			usin->sin_addr.s_addr = tmp_usin->sin_addr.s_addr;
			usin->sin_port = tmp_usin->sin_port;
		}
	}
}


int mptcp_proxy_fallback(struct sock *sk, int reason, bool is_add)
{
	struct mptcp_hw_ext_uid *node;
	struct tcp_sock *tp = tcp_sk(sk);
	struct sockaddr_in *server_addr = (struct sockaddr_in *) (&(tp->server_addr));
	int32_t uid;
	struct mptcp_hw_ext_uid_conf *conf;
	int32_t need_fallback = MPTCP_FALLBACK_UNDO;

	if (!server_addr->sin_addr.s_addr || !server_addr->sin_port) {
		mptcp_debug("%s: server_addr is not set\n", __func__);
		return MPTCP_FALLBACK_UNDO;
	}

	uid = sk->sk_uid.val;
	write_lock_bh(&mptcp_hw_ext_lock);
	node = mptcp_hw_ext_uid_in_list(uid);
	if (!node || node->key_type != ENABLE_BY_UID) {
		mptcp_debug("%s: node is not found\n", __func__);
		write_unlock_bh(&mptcp_hw_ext_lock);
		return MPTCP_FALLBACK_PEND;
	}

	conf = (struct mptcp_hw_ext_uid_conf *)(node->config);
	if (!conf || !(conf->flags & MPTCP_HW_CONF_VALUE_PROXY_INFO)) {
		mptcp_debug("%s: conf is not found\n", __func__);
		write_unlock_bh(&mptcp_hw_ext_lock);
		return MPTCP_FALLBACK_PEND;
	}

	if (is_add) {
		need_fallback = MPTCP_FALLBACK_PEND;
		if (reason == MPTCP_FALLBACK_PROXY_SYN_TIMEOUT) {
			node->proxy_fail_syn_timeout++;
			if (node->proxy_fail_syn_timeout >=
			    MPTCP_FALLBACK_PROXY_FAIL_SYN_TIMEOUT_THRH)
				need_fallback = MPTCP_FALLBACK_AT_ONCE;
		} else if (reason == MPTCP_FALLBACK_PROXY_NORESPONSE) {
			node->proxy_fail_noresponse++;
			if (node->proxy_fail_noresponse >=
			    MPTCP_FALLBACK_PROXY_FAIL_NORESPONSE_THRH)
				need_fallback = MPTCP_FALLBACK_AT_ONCE;
		} else if (reason <= MPTCP_FALLBACK_PROXY_INTERNAL_ERROR_MAX) {
			node->proxy_fail_option++;
			if (node->proxy_fail_option >=
			    MPTCP_FALLBACK_PROXY_FAIL_OPTION_THRH)
				need_fallback = MPTCP_FALLBACK_AT_ONCE;
		} else
			need_fallback = MPTCP_FALLBACK_AT_ONCE;

		if (need_fallback == MPTCP_FALLBACK_AT_ONCE) {
			mptcp_info("%s: uid %d will fallback reason %d\n",
				__func__, uid, reason);

			list_del_init(&node->list);
			kfree(conf);
			kfree(node);
#ifdef CONFIG_HUAWEI_XENGINE
			xengine_report_uid_fallback(uid, reason);
#endif
		}
	} else {
		if (reason == MPTCP_FALLBACK_PROXY_SYN_TIMEOUT)
			node->proxy_fail_syn_timeout = 0;
		else if (reason == MPTCP_FALLBACK_PROXY_NORESPONSE)
			node->proxy_fail_noresponse = 0;
		else if (reason <= MPTCP_FALLBACK_PROXY_INTERNAL_ERROR_MAX)
			node->proxy_fail_option = 0;
	}
	write_unlock_bh(&mptcp_hw_ext_lock);
	return need_fallback;
}

