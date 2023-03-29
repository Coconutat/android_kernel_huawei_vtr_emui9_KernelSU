#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <linux/if.h>
#include <huawei_platform/net/bastet/bastet_comm.h>
#include <huawei_platform/net/bastet/bastet_utils.h>
#include <huawei_platform/net/bastet/bastet.h>

#define BASTET_MODEM_DEV "/dev/bastet_modem"

#define DEV_OPEN_MAX_RETRY 60

struct buffer {
	struct list_head list;
	uint8_t data[BST_MAX_READ_PAYLOAD];
};

extern void ind_hisi_com(void *info, u32 len);

extern void ind_modem_reset(uint8_t *value, uint32_t len);

#ifdef CONFIG_HW_CROSSLAYER_OPT
extern void aspen_crosslayer_recovery(void *info, int length);
#endif

#ifdef CONFIG_HUAWEI_EMCOM
extern void Emcom_Ind_Modem_Support(u8 ucState);
#endif


static struct file *dev_filp;

static int kernel_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
        mm_segment_t oldfs = get_fs();
        int error = -ENOTTY;

        if (!filp || !filp->f_op || !filp->f_op->unlocked_ioctl)
                goto out;

        set_fs(KERNEL_DS);
        error = filp->f_op->unlocked_ioctl(filp, cmd, arg);
        set_fs(oldfs);

        if (error == -ENOIOCTLCMD)
                error = -ENOTTY;
 out:
        return error;
}

#ifdef CONFIG_HUAWEI_BASTET_COMM
int get_modem_rab_id(struct bst_modem_rab_id *info)
{
	union bst_rab_id_ioc_arg ioc_arg = {};
	int ret;

	if (!info) {
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(dev_filp)) {
		return -EBUSY;
	}

	memcpy(ioc_arg.in, cur_netdev_name, IFNAMSIZ);
	ioc_arg.in[IFNAMSIZ - 1] = '\0';

	ret = kernel_ioctl(dev_filp, BST_MODEM_IOC_GET_MODEM_RAB_ID, (unsigned long)&ioc_arg);
	if (ret >= 0) {
		info->modem_id = ioc_arg.out.modem_id;
		info->rab_id = ioc_arg.out.rab_id;
	}

	return ret;
}
#endif /* CONFIG_HUAWEI_BASTET_COMM */

int bastet_modem_dev_write(uint8_t *msg, uint32_t len)
{
	loff_t offset = 0;
	ssize_t ret;

	if (!msg) {
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(dev_filp)) {
		return -EBUSY;
	}

	BASTET_LOGI("len %u", len);

	if (BASTET_DEBUG) {
		print_hex_dump(KERN_ERR, "bstmsg:", 0, 16, 1, (void *)msg, len, 1);
	}

	ret = kernel_write(dev_filp, msg, len, offset);
	BASTET_LOGI("ret %ld", ret);

	return ret;
}

int bastet_comm_write(uint8_t *msg, uint32_t len, uint32_t type)
{
	bst_acom_msg *pMsg = NULL;
	uint8_t *buf;
	uint32_t ulLength;
	int ret;

	if (len > BST_MAX_WRITE_PAYLOAD) {
		BASTET_LOGE("write length over BST_MAX_WRITE_PAYLOAD!");
		return -EINVAL;
	}

	buf = kmalloc(sizeof(bst_acom_msg) + BST_MAX_WRITE_PAYLOAD, GFP_KERNEL);
	if (NULL == buf) {
		return -ENOMEM;
	}

	pMsg = (bst_acom_msg *)buf;
	pMsg->enMsgType = type;
	pMsg->ulLen     = len;
	memcpy(pMsg->aucValue, msg, len);

	ulLength = sizeof(bst_acom_msg) + len; // sub sizeof(aucValue[4]) ?

	ret = bastet_modem_dev_write((uint8_t *)pMsg, ulLength);
	kfree(buf);

	return ret;
}

#ifdef CONFIG_HUAWEI_EMCOM
int bastet_comm_keypsInfo_write(uint32_t ulState)
{
	bst_key_psinfo key_ps_info = {};
	int ret;

	BASTET_LOGI("state: %d", ulState);

	key_ps_info.enMsgType = BST_ACORE_CORE_MSG_TYPE_EMCOM_KEY_PSINFO;
	key_ps_info.enState = ulState;

	ret = bastet_modem_dev_write((uint8_t *)&key_ps_info, sizeof(key_ps_info));
	return ret;
}
#endif /* CONFIG_HUAWEI_EMCOM */

#ifdef CONFIG_HW_CROSSLAYER_OPT
static void bastet_aspen_pkt_drop_proc(uint8_t *msg, uint32_t len)
{
	bst_aspen_pkt_drop *aspen_msg = (bst_aspen_pkt_drop *)msg;

	if (NULL == aspen_msg) {
		BASTET_LOGE("aspen msg is empty");
		return;
	}

	if (len < (sizeof(*aspen_msg) - sizeof(aspen_msg->stPkt))) {
		BASTET_LOGE("aspen msg size too small %u", len);
		return;
	}

	switch(aspen_msg->ulAspenInfoType)
	{
		case BST_ASPEN_INFO_PKT_DROP:
		{
			if (len != (sizeof(bst_aspen_pkt_drop) - (BST_ASPEN_PKT_DROP_SIZE - aspen_msg->ulPktNum) * sizeof(struct aspen_cdn_info))) {
				BASTET_LOGE("aspen msg size wrong %u", len);
				break;
			}
			aspen_crosslayer_recovery((void *)aspen_msg->stPkt, (int)aspen_msg->ulPktNum);
			break;
		}
		default:
		{
			BASTET_LOGE("aspen info type is wrong %u", aspen_msg->ulAspenInfoType);
			break;
		}
	}
}
#endif /* CONFIG_HW_CROSSLAYER_OPT */

static int handle_event(uint8_t *msg, uint32_t len)
{
	bst_common_msg *bst_msg;

	if (msg == NULL) {
		BASTET_LOGE("msg is null");
		return 0;
	}

	if (len < sizeof(bst_common_msg)) {
		BASTET_LOGE("msg len error %u", len);
		return 0;
	}

	bst_msg = (bst_common_msg *)msg;

	BASTET_LOGI("bst msg type %u len %u", bst_msg->enMsgType, len);

	switch(bst_msg->enMsgType)
	{
	#ifdef CONFIG_HW_CROSSLAYER_OPT
		case BST_ACORE_CORE_MSG_TYPE_ASPEN:
		{
			bst_acom_msg *acom_msg = (bst_acom_msg *)msg;
			long hdrlen = (uint8_t *)(acom_msg->aucValue) - (uint8_t *)acom_msg;

			if (len != hdrlen + acom_msg->ulLen) {
				BASTET_LOGE("aspen msg len error %u %u", len, acom_msg->ulLen);
				break;
			}
			bastet_aspen_pkt_drop_proc(acom_msg->aucValue, acom_msg->ulLen);
			break;
		}
	#endif
		case BST_ACORE_CORE_MSG_TYPE_DSPP:
		{
			bst_acom_msg *acom_msg = (bst_acom_msg *)msg;
			long hdrlen = (uint8_t *)(acom_msg->aucValue) - (uint8_t *)acom_msg;

			if (len != hdrlen + acom_msg->ulLen) {
				BASTET_LOGE("dspp msg len error %u %u", len, acom_msg->ulLen);
				break;
			}
			ind_hisi_com(acom_msg->aucValue, acom_msg->ulLen);
			break;
		}
	#ifdef CONFIG_HUAWEI_EMCOM
		case BST_ACORE_CORE_MSG_TYPE_EMCOM_SUPPORT:
		{
			bst_emcom_support_msg *emcom_support_msg = (bst_emcom_support_msg *)msg;
			if (len != sizeof(bst_emcom_support_msg)) {
				BASTET_LOGE("emcom support msg len error %u %lu", len, sizeof(bst_emcom_support_msg));
				break;
			}
			Emcom_Ind_Modem_Support(emcom_support_msg->enState);
			break;
		}
	#endif
		case BST_ACORE_CORE_MSG_TYPE_RESET_INFO:
		{
			bst_acom_msg *acom_msg = (bst_acom_msg *)msg;
			long hdrlen = (uint8_t *)(acom_msg->aucValue) - (uint8_t *)acom_msg;

			if (len != hdrlen + acom_msg->ulLen) {
				BASTET_LOGE("reset info msg len error %u %u", len, acom_msg->ulLen);
				break;
			}
			ind_modem_reset(acom_msg->aucValue, acom_msg->ulLen);
			break;
		}
		default:
			BASTET_LOGE("bst msg type error %u", bst_msg->enMsgType);
			break;
	}
	return 0;
}

static void free_buffers(struct list_head *buffers)
{
	struct buffer *buffer_entry, *tmp;

	if (!buffers) {
		return;
	}

	list_for_each_entry_safe(buffer_entry, tmp, buffers, list) {
		list_del(&buffer_entry->list);
		kfree(buffer_entry);
	}
}

static void copy_from_buffers(uint8_t *data, struct list_head *buffers, uint32_t total)
{
	struct buffer *buffer_entry;
	uint8_t *pdata = data;
	int remain = total;
	int size;

	if (!data || !buffers) {
		return;
	}

	list_for_each_entry(buffer_entry, buffers, list) {
		if (remain <= 0)
			break;
		size = remain > BST_MAX_READ_PAYLOAD ? BST_MAX_READ_PAYLOAD : remain;
		memcpy(pdata, buffer_entry->data, size);
		pdata += size;
		remain -= size;
	}
}

static int get_more_data(uint8_t *firstdata)
{
	int32_t ret = 0;
	uint32_t len = 0;
	struct buffer *buf;
	uint8_t *data = NULL;
	loff_t offset = 0;
	struct list_head buffers = LIST_HEAD_INIT(buffers);

	while (1) {
		buf = kmalloc(sizeof(*buf), GFP_KERNEL); // GFP_ATOMIC ?
		if (NULL == buf) {
			free_buffers(&buffers);
			return -ENOMEM;
		}

		ret = kernel_read(dev_filp, offset, buf->data, BST_MAX_READ_PAYLOAD);
		BASTET_LOGI("read %d", ret);

		if (ret > 0 && ret < BST_MAX_READ_PAYLOAD) {
			if (BASTET_DEBUG) {
				print_hex_dump(KERN_ERR, "bstmsg:", 0, 16, 1, (void *)buf, ret, 1);
			}
			len += ret;
			list_add(&buf->list, &buffers);
			break;
		} else if (BST_MAX_READ_PAYLOAD == ret) {
			if (BASTET_DEBUG) {
				print_hex_dump(KERN_ERR, "bstmsg:", 0, 16, 1, (void *)buf, ret, 1);
			}
			len += ret;
			list_add(&buf->list, &buffers);
		} else if (0 == ret) {
			kfree(buf);
			break;
		} else {
			free_buffers(&buffers);
			kfree(buf);
			return -EINVAL;
		}
	}

	if (0 == len) {
		handle_event(firstdata, BST_MAX_READ_PAYLOAD);
		return 0;
	}

	data = (uint8_t *)kmalloc(len + BST_MAX_READ_PAYLOAD, GFP_KERNEL);
	if (NULL == data) {
		free_buffers(&buffers);
		return -EINVAL;
	}

	memcpy(data, firstdata, BST_MAX_READ_PAYLOAD);
	copy_from_buffers(data + BST_MAX_READ_PAYLOAD, &buffers, len);
	free_buffers(&buffers);

	handle_event(data, len + BST_MAX_READ_PAYLOAD);

	if (data != NULL) {
		kfree(data);
	}

	return 0;
}

static int get_event(void)
{
	int32_t size = 0;
	int ret = 0;
	uint8_t *buf;
	loff_t offset = 0;

	if (IS_ERR_OR_NULL(dev_filp)) {
		return -EINVAL;
	}

	buf = kmalloc(BST_MAX_READ_PAYLOAD, GFP_KERNEL);
	if (NULL == buf) {
		return -ENOMEM;
	}

	size = kernel_read(dev_filp, offset, buf, BST_MAX_READ_PAYLOAD);
	BASTET_LOGI("read %d", size);

	if (size > 0 && size < BST_MAX_READ_PAYLOAD) {
		if (BASTET_DEBUG) {
			print_hex_dump(KERN_ERR, "bstmsg:", 0, 16, 1, (void *)buf, size, 1);
		}
		handle_event(buf, size);
	} else if (BST_MAX_READ_PAYLOAD == size) {
		if (BASTET_DEBUG) {
			print_hex_dump(KERN_ERR, "bstmsg:", 0, 16, 1, (void *)buf, size, 1);
		}
		ret = get_more_data(buf);
	} else {
		BASTET_LOGI("read error %d", size);
		ret = -EINVAL;
	}

	kfree(buf);
	return ret;
}

static int bastet_modem_dev_open(void)
{
	struct file *filp = NULL;
	int try = DEV_OPEN_MAX_RETRY;

	while (try--) {
		filp = filp_open(BASTET_MODEM_DEV, O_RDWR, 0);

		if (IS_ERR(filp)) {
			BASTET_LOGI("open err=%ld", PTR_ERR(filp));

			msleep(1000);
			continue;
		} else {
			BASTET_LOGI("open succ");
			break;
		}
	}

	if (IS_ERR(filp))
		return PTR_ERR(filp);

	dev_filp = filp;
	return 0;
}

static int bastet_modem_reader(void *data)
{
	int err;

	err = bastet_modem_dev_open();
	if (err < 0) {
		BASTET_LOGE("error open bastet modem dev");
		return 0;
	}

	while (!kthread_should_stop()) {
		//wait_for_event(); // poll in kernel

		err = get_event();
		if (err < 0) {
			BASTET_LOGI("get_event err=%d", err);
			break;
		}
	}

	BASTET_LOGI("kthread exit");
	return 0;
}

int bastet_comm_init(void)
{
	struct task_struct *task;

	BASTET_LOGI("bastet_comm_init");

	task = kthread_run(bastet_modem_reader, NULL, "bastet_modem_rd");
	if (IS_ERR(task)) {
		BASTET_LOGE("create reader thread failed err=%ld", PTR_ERR(task));
		return -1;
	}

	return 0;
}


