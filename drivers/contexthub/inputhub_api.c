#include <libhwsecurec/securec.h>
#include "inputhub_api.h"

#ifdef CONFIG_INPUTHUB_20
/*lint -e655*/
int send_cmd_format(unsigned char cmd_tag, unsigned char cmd_type,
    unsigned int subtype, char  *buf, size_t count, bool is_lock, struct read_info *rd)
{
	char auto_buffer[MAX_PKT_LENGTH] = {0};
	char *buffer;
	int ret = 0;

	write_info_t winfo;
	winfo.tag = cmd_tag;
	winfo.cmd = cmd_type;
	if (rd)
		buffer = rd->data;
	else
		buffer = auto_buffer;


	pr_info("[%s] : cmd_tag[0x%x] cmd[%d:0x%x]  count[%ld]\n",
		__func__, cmd_tag, cmd_type, subtype, count);

	if (CMD_CMN_CONFIG_REQ != cmd_type) {
		if (count > (MAX_PKT_LENGTH - sizeof(pkt_header_t))) {
			pr_err("%s cnt error[%ld]\n", __func__, count);
			return -1;
		}

		if (count)
			memcpy_s(buffer + sizeof(pkt_header_t), MAX_PKT_LENGTH - sizeof(pkt_header_t), buf, count);
		winfo.wr_buf = buffer + sizeof(pkt_header_t);
		winfo.wr_len = (unsigned short)count;
		ret = write_customize_cmd(&winfo, rd, is_lock);
	} else {
		unsigned int *subcmd = (unsigned int *)(((pkt_header_t *)buffer) + 1);
		*subcmd = subtype;
		if (count > (MAX_PKT_LENGTH - CONTEXTHUB_HEADER_SIZE)) {
			pr_err("%s cnt1 error[%ld]\n", __func__, count);
			return -1;
		}

		if (count)
			memcpy_s(buffer + CONTEXTHUB_HEADER_SIZE, MAX_PKT_LENGTH - CONTEXTHUB_HEADER_SIZE, buf, count);
		winfo.wr_buf = buffer + sizeof(pkt_header_t);
		winfo.wr_len = (unsigned short)(count + sizeof(unsigned int));
		ret = write_customize_cmd(&winfo, rd, is_lock);
	}

	if (ret)
		printk(KERN_ERR "%s error\n", __func__);

	return ret;
}

int send_cmd_from_kernel(unsigned char cmd_tag, unsigned char cmd_type,
    unsigned int subtype, char  *buf, size_t count)
{
	return send_cmd_format(cmd_tag, cmd_type, subtype, buf, count, true, NULL);
}

int send_cmd_from_kernel_nolock(unsigned char cmd_tag, unsigned char cmd_type,
    unsigned int subtype, char  *buf, size_t count)
{
	return send_cmd_format(cmd_tag, cmd_type, subtype, buf, count, false, NULL);
}

int send_cmd_from_kernel_response(unsigned char cmd_tag, unsigned char cmd_type,
    unsigned int subtype, char  *buf, size_t count, struct read_info *rd)
{
	return send_cmd_format(cmd_tag, cmd_type, subtype, buf, count, true, rd);
}
#else
/*lint -e655*/

static int adapter_unify(const void *buf, unsigned int length, struct read_info *rd)
{
	return inputhub_mcu_write_cmd_nolock(buf, length);
}

int send_cmd_format(unsigned char cmd_tag, unsigned char cmd_type,
    unsigned int subtype, char  *buf, size_t count, int (*adapter)(const void *buf, unsigned int length, struct read_info *rd), struct read_info *rd)
{
	char auto_buffer[MAX_PKT_LENGTH] = {0};
	char *buffer;
	int ret = 0;
	if (rd)
		buffer = rd->data;
	else
		buffer = auto_buffer;

	((pkt_header_t *)buffer)->tag = cmd_tag;
	((pkt_header_t *)buffer)->resp = rd?RESP:NO_RESP;
	((pkt_header_t *)buffer)->cmd = cmd_type;

	pr_info("[%s] : cmd_tag[0x%x] cmd[%d:0x%x]  count[%ld]\n", __func__, cmd_tag, cmd_type, subtype, count);

	if (CMD_CMN_CONFIG_REQ != cmd_type) {
		if (count > (MAX_PKT_LENGTH - sizeof(pkt_header_t))) {
			pr_err("%s cnt error[%ld]\n", __func__, count);
			return -1;
		}

		if (count)
			memcpy_s(buffer + sizeof(pkt_header_t), MAX_PKT_LENGTH - sizeof(pkt_header_t), buf, count);

		((pkt_header_t *)buffer)->length = (unsigned short)count;
		ret = adapter(buffer, (unsigned int)(count + sizeof(pkt_header_t)), rd);
	} else {
		unsigned int *subcmd = (unsigned int *)(((pkt_header_t *)buffer) + 1);
		*subcmd = subtype;

		if (count > (MAX_PKT_LENGTH - CONTEXTHUB_HEADER_SIZE)) {
			pr_err("%s cnt2 error[%ld]\n", __func__, count);
			return -1;
		}

		if (count)
			memcpy_s(buffer + CONTEXTHUB_HEADER_SIZE, MAX_PKT_LENGTH - CONTEXTHUB_HEADER_SIZE, buf, count);

		((pkt_header_t *)buffer)->length = (unsigned short)(count + sizeof(unsigned int));
		ret = adapter(buffer, (unsigned int)(count + CONTEXTHUB_HEADER_SIZE), rd);
	}

	if (ret)
		printk(KERN_ERR "%s error\n", __func__);

	return ret;
}

int send_cmd_from_kernel(unsigned char cmd_tag, unsigned char cmd_type,
    unsigned int subtype, char  *buf, size_t count)
{
	return  send_cmd_format(cmd_tag, cmd_type, subtype, buf,  count, inputhub_mcu_write_cmd_adapter,NULL);
}

int send_cmd_from_kernel_nolock(unsigned char cmd_tag, unsigned char cmd_type,
    unsigned int subtype, char  *buf, size_t count)
{
	return  send_cmd_format(cmd_tag, cmd_type, subtype, buf,  count, adapter_unify, NULL);
}

int send_cmd_from_kernel_response(unsigned char cmd_tag, unsigned char cmd_type,
    unsigned int subtype, char  *buf, size_t count, struct read_info *rd)
{
	return  send_cmd_format(cmd_tag, cmd_type, subtype, buf,  count, inputhub_mcu_write_cmd_adapter, rd);
}
#endif
