#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/notifier.h>
#include <net/genetlink.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include "../inputhub_api.h"
#include "../common.h"
#include "../shmem.h"
#include "hismart_ar.h"
#include <libhwsecurec/securec.h>


static ar_device_t  g_ar_dev;

static int ar_send_cmd_through_shmem(ar_hdr_t * head, char  *buf, size_t count)
{
	int ret;
	char *data;
	size_t len = count + sizeof(ar_hdr_t);
	if (len > MAX_CONFIG_SIZE || len < sizeof(ar_hdr_t)) {
		printk(KERN_ERR "hismart:[%s]:line[%d] shmem send size[%d] too large ,error\n",  __func__, __LINE__, (int)len);
		return -EINVAL;
	}
	data = (char *)kzalloc(len, GFP_KERNEL);
	if (!data) {
		printk(KERN_ERR "hismart:[%s]:line[%d] shmem send kzalloc[%d] fail\n", __func__, __LINE__, (int)len);
		return -ENOMEM;
	}
	memcpy_s(data, sizeof(ar_hdr_t), head, len - count);
	if (len > sizeof(ar_hdr_t))
		memcpy_s(data + sizeof(ar_hdr_t), count, buf, len - sizeof(ar_hdr_t));

#ifdef CONFIG_CONTEXTHUB_SHMEM
	ret = shmem_send(TAG_AR, (void *)data, len);
#else
	printk(KERN_ERR "hismart:[%s]:line[%d] shmem has not enable len[%d]\n", __func__, __LINE__, (int)len);
	ret = 0;
#endif
	kfree((void *)data);
	return ret;
}

static int ar_send_cmd_from_kernel(unsigned char cmd_tag, unsigned char cmd_type,
    unsigned int subtype, char  *buf, size_t count)
{
	unsigned int sub_cmd = 0;
	ar_hdr_t * ptr_hdr = (ar_hdr_t *)&sub_cmd;
	ptr_hdr->sub_cmd = (unsigned char)subtype;
	if (count > (MAX_PKT_LENGTH - CONTEXTHUB_HEADER_SIZE)) {
		return ar_send_cmd_through_shmem(ptr_hdr, buf, count);
	} else {
		return send_cmd_from_kernel(cmd_tag, cmd_type, sub_cmd, buf, count);
	}
}

static int ar_send_cmd_from_kernel_nolock(unsigned char cmd_tag, unsigned char cmd_type,
    unsigned int subtype, char  *buf, size_t count)
{
	unsigned int sub_cmd = 0;
	ar_hdr_t * ptr_hdr = (ar_hdr_t *)&sub_cmd;
	ptr_hdr->sub_cmd = (unsigned char)subtype;
	if (count > (MAX_PKT_LENGTH - CONTEXTHUB_HEADER_SIZE)) {
		return ar_send_cmd_through_shmem(ptr_hdr, buf, count);
	} else {
		return send_cmd_from_kernel_nolock(cmd_tag, cmd_type, sub_cmd, buf, count);
	}
}

static int ar_send_cmd_from_kernel_response(unsigned char cmd_tag, unsigned char cmd_type,
    unsigned int subtype, char  *buf, size_t count, struct read_info *rd)
{
	unsigned int sub_cmd = 0;
	ar_hdr_t * ptr_hdr = (ar_hdr_t *)&sub_cmd;
	ptr_hdr->sub_cmd = (unsigned char)subtype;
	return send_cmd_from_kernel_response(cmd_tag, cmd_type, sub_cmd, buf,  count, rd);
}

static void fifo_in(ar_data_buf_t *pdata, char *data, unsigned int len, unsigned int align)
{
	unsigned int deta;

	if ((!pdata) || (!data) || (!pdata->data_buf)) {
		pr_err("hismart:[%s]:pdata[%pK] data[%pK] parm is null\n", __func__, pdata, data);
		return ;
	}

	/*no enough space , just overwrite*/
	if ((pdata->data_count + len) > pdata->buf_size) {
		deta = pdata->data_count + len - pdata->buf_size;
		if (deta % align) {
			printk(KERN_ERR "hismart:[%s]:line[%d]fifo_in data not align\n", __func__, __LINE__);
			deta = (deta/align + 1)*align;
		}
		pdata->read_index = (pdata->read_index + deta)%pdata->buf_size;
	}
	/*copy data to flp pdr driver buffer*/
	if ((pdata->write_index + len) >  pdata->buf_size) {
		memcpy_s(pdata->data_buf + pdata->write_index ,pdata->buf_size, data, (size_t)(pdata->buf_size - pdata->write_index));
		memcpy_s(pdata->data_buf,pdata->buf_size, data + pdata->buf_size - pdata->write_index,
			(size_t)(len + pdata->write_index - pdata->buf_size));
	} else {
		memcpy_s(pdata->data_buf + pdata->write_index, pdata->buf_size - pdata->write_index, data , (size_t)len);
	}

	pdata->write_index = (pdata->write_index + len)%pdata->buf_size;
	pdata->data_count = (pdata->write_index - pdata->read_index + pdata->buf_size)%pdata->buf_size;
	/*if buf is full*/
	if (!pdata->data_count) {
		pdata->data_count = pdata->buf_size;
	}
}

static int fifo_out(ar_data_buf_t *pdata,  unsigned char *data, unsigned int count)
{
	if ((!pdata) || (!data) || (!pdata->data_buf)) {
		pr_err("hismart:[%s]:pdata[%pK] data[%pK] parm is null\n", __func__, pdata, data);
		return -EINVAL;
	}

	if (count  > pdata->data_count) {
		count = pdata->data_count;
	}

	/*copy data to user buffer*/
	if ((pdata->read_index + count) >  pdata->buf_size) {
		memcpy_s(data, count, pdata->data_buf + pdata->read_index, (size_t)(pdata->buf_size - pdata->read_index));
		memcpy_s(data + pdata->buf_size - pdata->read_index, count,
		pdata->data_buf, (size_t)(count + pdata->read_index - pdata->buf_size));
	} else {
		memcpy_s(data, pdata->buf_size - pdata->read_index, pdata->data_buf + pdata->read_index, (size_t)count);
	}
	pdata->read_index = (pdata->read_index + count)%pdata->buf_size;
	pdata->data_count -= count;
	return 0;
}

static unsigned int fifo_len(ar_data_buf_t *pdata)
{
	return pdata->data_count;
}

/* why *cur use g_ar_dev.ar_devinfo.cfg, because cts open can be used directly.*/
static unsigned int  context_cfg_set_to_iomcu(unsigned int context_max,
	context_iomcu_cfg_t *cur, context_iomcu_cfg_t *old_set)
{
	unsigned int i;
	unsigned char *ultimate_data = (unsigned char *)cur->context_list;
	memset_s((void*)cur, sizeof(context_iomcu_cfg_t), 0 ,sizeof(context_iomcu_cfg_t));
	cur->report_interval = old_set->report_interval < 1?1:old_set->report_interval;
	for(i = 0;i < context_max; i++) {
		if(old_set->context_list[i].head.event_type) {
			memcpy_s((void*)ultimate_data, sizeof(ar_context_cfg_header_t),
				(void*)&old_set->context_list[i], (unsigned long)sizeof(ar_context_cfg_header_t));
			ultimate_data += sizeof(ar_context_cfg_header_t);
			if(old_set->context_list[i].head.len && old_set->context_list[i].head.len <= CONTEXT_PRIVATE_DATA_MAX) {
				memcpy_s((void*)ultimate_data, CONTEXT_PRIVATE_DATA_MAX,
					old_set->context_list[i].buf, (unsigned long)old_set->context_list[i].head.len);
				ultimate_data += old_set->context_list[i].head.len;
			}
			cur->context_num++;
		}
	}

	return (unsigned int)(ultimate_data - (unsigned char*)cur);
}

/*lint -e661 -e662 -e826*/
/*Multi-instance scenarios,different buff data can impact on business,
but the kernel does not pay attention to this matter,APP service deal with this thing*/
static void ar_multi_instance(ar_device_t*  ar_dev, context_iomcu_cfg_t *config)
{
	int count;
	struct list_head *pos;
	ar_port_t *port;
	list_for_each(pos, &ar_dev->list) {
		port = container_of(pos, ar_port_t, list);
		if (port->channel_type & FLP_AR_DATA) {
			for (count= 0; count < AR_STATE_BUTT; count++) {
					config->context_list[count].head.event_type |=
					port->ar_config.context_list[count].head.event_type;
				}

			config->report_interval  =
			config->report_interval <= port->ar_config.report_interval ?
			config->report_interval:port->ar_config.report_interval;
		}
	}
}
/*lint -e715*/

static int ar_stop(unsigned int delay)
{
	int count;
	context_dev_info_t * devinfo = &g_ar_dev.ar_devinfo;

	if (!(FLP_AR_DATA & g_ar_dev.service_type)) {
		return 0;
	}

	if (devinfo->usr_cnt) {
		size_t len;
		context_iomcu_cfg_t *pcfg = (context_iomcu_cfg_t *)kzalloc(sizeof(context_iomcu_cfg_t), GFP_KERNEL);
		if (NULL == pcfg) {
			pr_err("hismart:[%s]:line[%d] kzalloc error\n", __func__, __LINE__);
			return -ENOMEM;
		}

		for (count = 0; count < AR_STATE_BUTT; count++) {
			pcfg->context_list[count].head.context = (unsigned int)count;
		}
		pcfg->report_interval = ~0;
		ar_multi_instance(&g_ar_dev, pcfg);
		len = context_cfg_set_to_iomcu(AR_STATE_BUTT, &devinfo->cfg, pcfg);
		kfree((void*)pcfg);
		if (0 == devinfo->cfg.context_num) {
			pr_err("hismart:[%s]:line[%d] context_cfg_set_to_iomcu context_num:0 error\n", __func__, __LINE__);
			return -EINVAL;
		}
	#ifdef CONFIG_INPUTHUB_20
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_AR_START_REQ,
		(char *)&devinfo->cfg, len);
	#else
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, CMD_FLP_AR_START_REQ,
		(char *)&devinfo->cfg, len);
	#endif
	} else {
		unsigned int delayed = 0;
	#ifdef CONFIG_INPUTHUB_20
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_AR_STOP_REQ, (char *)&delayed, sizeof(delayed));
	#else
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, CMD_FLP_AR_STOP_REQ, (char *)&delayed, sizeof(delayed));
	#endif
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CLOSE_REQ, 0, NULL, (size_t)0);
		 g_ar_dev.service_type &= ~FLP_AR_DATA;
	}

	return 0;
}
/*lint +e661 +e662 +e826 +e715*/
static void data_buffer_exit(ar_data_buf_t *buf)
{
	if (buf->data_buf) {
		kfree((void*)buf->data_buf);
		buf->data_buf = NULL;
	}
}

static int  ar_stop_cmd(ar_port_t *ar_port, unsigned long arg)
{
	unsigned int delay = 0;
	int ret = 0;
	context_dev_info_t * devinfo = &g_ar_dev.ar_devinfo;

	mutex_lock(&g_ar_dev.lock);
	if (!(FLP_AR_DATA & ar_port->channel_type) || !(FLP_AR_DATA & g_ar_dev.service_type)) {
		pr_err("hismart:[%s]:line[%d] had stopped[%x][%x]\n", __func__, __LINE__, ar_port->channel_type, g_ar_dev.service_type);
		ret =  -EIO;
		goto AR_STOP_ERR;
	}

	if (copy_from_user(&delay, (void *)arg, sizeof(unsigned int))) {
		pr_err("hismart:[%s]:line[%d] delay copy_from_user error\n", __func__, __LINE__);
		ret =  -EFAULT;
		goto AR_STOP_ERR;
	}
	printk(HISI_AR_DEBUG "hismart:[%s]:line[%d] delay[%u]\n", __func__, __LINE__, delay);
	memset_s((void *)&ar_port->ar_config, sizeof(context_iomcu_cfg_t), 0, sizeof(context_iomcu_cfg_t));
	devinfo->usr_cnt--;

	ar_port->channel_type &= (~FLP_AR_DATA);
	ret = ar_stop(delay);
	if (ret) {
		pr_err("hismart:[%s]:line[%d] ar_stop ret [%d] error\n", __func__, __LINE__, ret);
		ret = 0;/*Because the channel_type flag has been removed, the business has been closed for the application layer*/
	}

	data_buffer_exit(&ar_port->ar_buf);
AR_STOP_ERR:
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}

static int  env_close_cmd(ar_port_t *ar_port, int force)
{
	int i;

	if (!(FLP_ENVIRONMENT & ar_port->channel_type) || !(FLP_ENVIRONMENT & g_ar_dev.service_type)) {
		pr_info("hismart:[%s]:line[%d] channel_type[0x%x] service_type[0x%x]\n", __func__, __LINE__, ar_port->channel_type, g_ar_dev.service_type);
		return 0;
	}

	if (ENV_CLOSE_NORMAL == force) {
		for(i = 0;i<AR_ENVIRONMENT_END;i++) {
			if (AR_ENVIRONMENT_END != g_ar_dev.envdev_priv.env_init[i].context)
				return 0;
		}
	}

	printk(HISI_AR_DEBUG "hismart:[%s]:line[%d]\n", __func__, __LINE__);
	memset_s((void*)&g_ar_dev.env_devinfo, sizeof(context_dev_info_t), 0, sizeof(context_dev_info_t));
	memset_s((void*)&g_ar_dev.envdev_priv, sizeof(envdev_priv_t), 0, sizeof(envdev_priv_t));
	for(i = 0;i<AR_ENVIRONMENT_END;i++) {
		g_ar_dev.envdev_priv.env_init[i].context = AR_ENVIRONMENT_END;
	}
	ar_port->channel_type &= (~FLP_ENVIRONMENT);
	ar_send_cmd_from_kernel(TAG_ENVIRONMENT, CMD_CMN_CLOSE_REQ, 0, NULL, (size_t)0);
	g_ar_dev.service_type &= ~FLP_ENVIRONMENT;
	data_buffer_exit(&ar_port->env_buf);
	return 0;
}

static int data_buffer_init(ar_data_buf_t *buf, unsigned int buf_sz)
{
	int ret = 0;
	buf->buf_size = buf_sz;
	buf->read_index = 0 ;
	buf->write_index = 0;
	buf->data_count = 0;
	buf->data_buf = (char *)kzalloc((size_t)buf->buf_size, GFP_KERNEL);
	if (!buf->data_buf) {
		pr_err("hismart:[%s]:line[%d] data_buf kzalloc err\n", __func__, __LINE__);
		return -ENOMEM;
	}

	return ret;
}

/*devinfo->cfg.context_list :The order just like HAL layers,set of One app*/
/*config :ar_port->config,out parm, natural order ,Global group*/
static int create_ar_port_cfg(context_iomcu_cfg_t *config,
context_dev_info_t* devinfo, unsigned int context_max)
{
	unsigned int i;
	context_config_t  *plist = (context_config_t  *)devinfo->cfg.context_list;
	memset_s(config, sizeof(context_iomcu_cfg_t), 0, sizeof(context_iomcu_cfg_t));
	config->report_interval = devinfo->cfg.report_interval;
	for (i = 0; i < context_max; i++)
		config->context_list[i].head.context = i;

	for(i = 0; i < devinfo->cfg.context_num; i++,plist++) {
		if (plist->head.context >= context_max || plist->head.event_type >= AR_STATE_MAX ||plist->head.len > CONTEXT_PRIVATE_DATA_MAX) {
			pr_err("hismart:[%s]:line[%d] EPERM ERR c[%u]e[%u]len[%u]\n", __func__, __LINE__, plist->head.context,
			plist->head.event_type, plist->head.len);
			return -EPERM;
		}

		memcpy_s((void*)&config->context_list[plist->head.context], sizeof(context_config_t), (void*)plist, sizeof(context_config_t));
	}
	return 0;
}
/*lint -e838 -e438*/
static int context_config(context_iomcu_cfg_t *config, context_dev_info_t* devinfo,
	unsigned long arg, unsigned int context_max)
{
	int ret = 0;
	context_hal_config_t hal_cfg_hdr;
	unsigned long usr_len;
	if (copy_from_user((void *)&hal_cfg_hdr, (void __user *)arg, sizeof(context_hal_config_t))) {
		pr_err("hismart:[%s]:line[%d] copy_from_user context_hal_config_t err\n", __func__, __LINE__);
		return -EIO;
	}

	if (0 == hal_cfg_hdr.context_num ||hal_cfg_hdr.context_num > context_max) {
		pr_err("hismart:[%s]:line[%d] num[%d]max[%d] is invalid\n", __func__, __LINE__,
			hal_cfg_hdr.context_num, context_max);
		return  -EINVAL;
	}

	memset_s((void*)&devinfo->cfg, sizeof(context_iomcu_cfg_t), 0, sizeof(context_iomcu_cfg_t));
	devinfo->cfg.report_interval = hal_cfg_hdr.report_interval;
	devinfo->cfg.context_num = hal_cfg_hdr.context_num;
	usr_len = sizeof(context_config_t) * hal_cfg_hdr.context_num;
	if (usr_len > sizeof(devinfo->cfg.context_list)) {
		pr_err("hismart:[%s]:line[%d] usr_len[%lu] bufsize[%lu]err\n", __func__, __LINE__,
			usr_len, sizeof(devinfo->cfg.context_list));
		return  -ENOMEM;
	}

	if (copy_from_user((void *)devinfo->cfg.context_list, (const void __user *)hal_cfg_hdr.context_addr, usr_len)) {
		pr_err("hismart:[%s]:line[%d] copy_from_user error context list\n", __func__, __LINE__);
		return -EDOM;
	}

	ret = create_ar_port_cfg(config, devinfo, context_max);
	if (ret)
		pr_err("hismart:[%s]:line[%d] create_ar_port_cfg error\n", __func__, __LINE__);
	return ret;
}

/*lint +e838 +e438*/
static int ar_config_to_iomcu(ar_port_t *ar_port)
{
	int ret = 0;
	context_dev_info_t* devinfo = &g_ar_dev.ar_devinfo;
	if (devinfo->usr_cnt) {
		context_iomcu_cfg_t *pcfg = (context_iomcu_cfg_t *)kzalloc(sizeof(context_iomcu_cfg_t), GFP_KERNEL);
		if (NULL == pcfg) {
			pr_err("hismart:[%s]:line[%d] kzalloc error\n", __func__, __LINE__);
			ret = -ENOMEM;
			goto CFG_IOMCU_FIN;
		}
		memcpy_s(pcfg, sizeof(context_iomcu_cfg_t), &ar_port->ar_config, sizeof(context_iomcu_cfg_t));
		ar_multi_instance(&g_ar_dev, pcfg);
		devinfo->cfg_sz = context_cfg_set_to_iomcu(AR_STATE_BUTT, &devinfo->cfg, pcfg);
		kfree((void*)pcfg);
		if (0 == devinfo->cfg.context_num) {
			pr_err("hismart:[%s]:line[%d] context_cfg_set_to_iomcu context_num:0 error\n", __func__, __LINE__);
			ret = -ERANGE;
			goto CFG_IOMCU_FIN;
		}
	#ifdef CONFIG_INPUTHUB_20
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_AR_START_REQ,
		(char *)&devinfo->cfg, (size_t)devinfo->cfg_sz);
	#else
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, CMD_FLP_AR_START_REQ,
		(char *)&devinfo->cfg, (size_t)devinfo->cfg_sz);
	#endif
	} else {
		struct read_info rd;
		memset_s((void*)&rd, sizeof(struct read_info), 0,sizeof(struct read_info));
		devinfo->cfg_sz = context_cfg_set_to_iomcu(AR_STATE_BUTT, &devinfo->cfg, &ar_port->ar_config);
		if (0 == devinfo->cfg.context_num) {
			pr_err("hismart:[%s]:line[%d] context_cfg_set_to_iomcu context_num:0 error\n", __func__, __LINE__);
			ret = -EDOM;
			goto CFG_IOMCU_FIN;
		}

		ret = ar_send_cmd_from_kernel_response(TAG_AR, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0, &rd);
		if (ret) {
			pr_err("hismart:[%s]:line[%d] hub not support context, open ar app fail[%d]\n", __func__, __LINE__, ret);
			goto CFG_IOMCU_FIN;
		}
#ifdef CONFIG_INPUTHUB_20
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_AR_START_REQ,
		(char *)&devinfo->cfg, (size_t)devinfo->cfg_sz);
#else
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, CMD_FLP_AR_START_REQ,
		(char *)&devinfo->cfg, (size_t)devinfo->cfg_sz);
#endif
	}

	printk(HISI_AR_DEBUG "hismart:[%s]:line[%d] interval[%u] size[%d]\n", __func__, __LINE__, devinfo->cfg.report_interval, devinfo->cfg_sz);
CFG_IOMCU_FIN:
	return ret;
}
/*lint -e715*/
static int ar_config_cmd(ar_port_t *ar_port, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	context_dev_info_t* devinfo = &g_ar_dev.ar_devinfo;

	mutex_lock(&g_ar_dev.lock);
	if(!(FLP_AR_DATA & ar_port->channel_type)) {
		ret = data_buffer_init(&ar_port->ar_buf,
			(unsigned int)(AR_STATE_BUTT * (sizeof(context_event_t) + CONTEXT_PRIVATE_DATA_MAX)));
		if (ret){
			pr_err("hismart:[%s]:line[%d] data_buffer_init err[%d]\n", __func__, __LINE__, ret);
			goto AR_CFG_FIN;
		}
	}

	ret = context_config(&ar_port->ar_config, devinfo, arg, AR_STATE_BUTT);
	if (ret) {
		pr_err("hismart:[%s]:line[%d]context_config err\n", __func__, __LINE__);
		goto AR_CONFIG_ERR;
	}

	ret = ar_config_to_iomcu(ar_port);
	if (ret) {
		pr_err("hismart:[%s]:line[%d] ar_config_to_iomcu err[%d]\n", __func__, __LINE__, ret);
		goto AR_CONFIG_ERR;
	}

	if (!(FLP_AR_DATA & ar_port->channel_type)) {
		devinfo->usr_cnt++;
		ar_port->channel_type |= FLP_AR_DATA;
		g_ar_dev.service_type |= FLP_AR_DATA;
	}

	mutex_unlock(&g_ar_dev.lock);
	return 0;

AR_CONFIG_ERR:
	data_buffer_exit(&ar_port->ar_buf);
AR_CFG_FIN:
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}
/*lint +e715*/
/*lint -e838*/
static int env_open_cmd(ar_port_t *ar_port)
{
	int ret = 0;
	struct read_info rd;
	mutex_lock(&g_ar_dev.lock);
	if (FLP_ENVIRONMENT & ar_port->channel_type) {
		goto ENV_INIT_ERR;
	}

	ret = data_buffer_init(&ar_port->env_buf,
		(unsigned int)(AR_ENVIRONMENT_END * (sizeof(context_event_t) + CONTEXT_PRIVATE_DATA_MAX)));
	if (ret){
		pr_err("hismart:[%s]:line[%d] kfifo_alloc err[%d]\n", __func__, __LINE__, ret);
		goto ENV_INIT_ERR;
	}

	memset_s((void*)&rd, sizeof(struct read_info), 0,sizeof(struct read_info));
	ret = ar_send_cmd_from_kernel_response(TAG_ENVIRONMENT, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0, &rd);
	if (ret) {
		pr_err("hismart:[%s]:line[%d] hub not support env, open env app[%d]\n", __func__, __LINE__, ret);
		goto ENV_INIT_ERR1;
	}
	ar_port->channel_type |= FLP_ENVIRONMENT;
	g_ar_dev.service_type |= FLP_ENVIRONMENT;
	g_ar_dev.envdev_priv.env_port = ar_port;
	mutex_unlock(&g_ar_dev.lock);
	return ret;
ENV_INIT_ERR1:
	data_buffer_exit(&ar_port->env_buf);
ENV_INIT_ERR:
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}

void data_dump(void)
{
#ifdef HISI_AR_DATA_DEBUG
	int i;
	envdev_priv_t *envdev_priv = &g_ar_dev.envdev_priv;
	for (i = 0;i < AR_ENVIRONMENT_END;i++) {
		printk(HISI_AR_DEBUG"hismart:[%s]:line[%d] Num[%d] ctx[0x%x]ENctx[0x%x]type[%d]\n", __func__, __LINE__, i, envdev_priv->env_init[i].context,
		envdev_priv->env_enable[i].head.context, envdev_priv->env_enable[i].head.event_type);
	}
#endif
}

static int env_enable_cmd(ar_port_t *ar_port, unsigned long arg)
{
	int ret = 0;
	env_enable_t *env_enable = g_ar_dev.envdev_priv.env_enable;
	env_enable_t enable;

	mutex_lock(&g_ar_dev.lock);

	if (!(FLP_ENVIRONMENT & ar_port->channel_type) || !(FLP_ENVIRONMENT & g_ar_dev.service_type)) {
		pr_err("hismart:[%s]:line[%d] channel[0x%x] service[0x%x] err,you must init first\n", __func__, __LINE__, ar_port->channel_type, g_ar_dev.service_type);
		ret =  -EINVAL;
		goto ENV_EN_FIN;
	}

	if (copy_from_user((void *)&enable, (const void __user *)arg, sizeof(env_enable_t))) {
		pr_err("hismart:[%s]:line[%d] copy_from_user error\n", __func__, __LINE__);
		ret =  -EDOM;
		goto ENV_EN_FIN;
	}

	if (CONTEXT_PRIVATE_DATA_MAX < enable.head.len){
		pr_err("hismart:[%s]:line[%d] priv data len err[%u]\n", __func__, __LINE__, enable.head.len);
		ret =  -EINVAL;
		goto ENV_EN_FIN;
	}

	if (AR_STATE_MAX <= enable.head.event_type) {
		pr_err("hismart:[%s]:line[%d] event_type err[%u]\n", __func__, __LINE__, enable.head.event_type);
		ret =  -EINVAL;
		goto ENV_EN_FIN;
	}

	if (AR_ENVIRONMENT_END <= enable.head.context) {
		pr_err("hismart:[%s]:line[%d] context error[%d]\n", __func__, __LINE__, enable.head.context);
		ret =  -EACCES;
		goto ENV_EN_FIN;
	}

	printk(HISI_AR_DEBUG"hismart:[%s]:line[%d] e[%u]c[%u]r[%u]len[%u]\n", __func__, __LINE__, enable.head.event_type, enable.head.context, enable.head.report_interval, enable.head.len);

	memcpy_s((void*)&env_enable[enable.head.context], sizeof(env_enable_t), &enable, sizeof(env_enable_t));
#ifdef CONFIG_INPUTHUB_20
	ret = ar_send_cmd_from_kernel(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ, SUB_CMD_ENVIRONMENT_ENABLE_REQ,
	(char *)&enable, sizeof(env_enable_t));
#else
	ret = ar_send_cmd_from_kernel(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ, CMD_ENVIRONMENT_ENABLE_REQ,
	(char *)&enable, sizeof(env_enable_t));
#endif
	data_dump();
ENV_EN_FIN:
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}

static int  env_disable_cmd(ar_port_t *ar_port, unsigned long arg)
{
	int ret = 0;
	env_disable_cmd_t dis_cmd;
	env_enable_t *env_enable = g_ar_dev.envdev_priv.env_enable;
	mutex_lock(&g_ar_dev.lock);
	if (!(FLP_ENVIRONMENT & ar_port->channel_type) || !(FLP_ENVIRONMENT & g_ar_dev.service_type)) {
		pr_err("hismart:[%s]:line[%d] Env has not open, channel[0x%x] service[0x%x]\n", __func__, __LINE__, ar_port->channel_type, g_ar_dev.service_type);
		ret = -EPERM;
		goto ENV_DIS_ERR;
	}

	if (copy_from_user((void *)&dis_cmd,  (void __user *)arg, sizeof(env_disable_cmd_t))) {
		pr_err("hismart:[%s]:line[%d] copy_from_user error \n", __func__, __LINE__);
		ret = -EDOM;
		goto ENV_DIS_ERR;
	}

	if (AR_STATE_MAX <= dis_cmd.event_type){
		pr_err("hismart:[%s]:line[%d] event_type error [%u]\n", __func__, __LINE__, dis_cmd.event_type);
		ret = -EINVAL;
		goto ENV_DIS_ERR;
	}

	if (AR_ENVIRONMENT_END <= dis_cmd.context || AR_ENVIRONMENT_BEGIN > dis_cmd.context) {
		pr_err("hismart:[%s]:line[%d] context error [%x]\n", __func__, __LINE__, dis_cmd.context);
		ret = -EINVAL;
		goto ENV_DIS_ERR;
	}

	if (0 == env_enable[dis_cmd.context].head.event_type) {
		pr_err("hismart:[%s]:line[%d] you must enable first\n", __func__, __LINE__);
		ret = -EIO;
		goto ENV_DIS_ERR;
	}

	env_enable[dis_cmd.context].head.event_type &= 0x03&(~dis_cmd.event_type);
#ifdef CONFIG_INPUTHUB_20
	ret = ar_send_cmd_from_kernel(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ, SUB_CMD_ENVIRONMENT_DISABLE_REQ, (char *)&dis_cmd, sizeof(env_disable_cmd_t));
#else
	ret = ar_send_cmd_from_kernel(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ, CMD_ENVIRONMENT_DISABLE_REQ, (char *)&dis_cmd, sizeof(env_disable_cmd_t));
#endif
	data_dump();
ENV_DIS_ERR:
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}

/*lint -e785 */
static struct genl_family ar_genl_family = {
    .id         = GENL_ID_GENERATE,
    .name       = AR_GENL_NAME,
    .version    = TASKAR_GENL_VERSION,
    .maxattr    = AR_GENL_ATTR_MAX,
};
/*lint +e785 */
static struct sk_buff *context_gnetlink_alloc(ar_port_t *ar_port,unsigned int count, unsigned char cmd_type, unsigned char **data)
{
	ar_data_buf_t *pdata = &ar_port->ar_buf;
	void *msg_header;
	struct sk_buff *skb;
	static unsigned int flp_event_seqnum = 0;

	if(!ar_port->portid) {
		pr_err("hismart:[%s]:line[%d] no portid error\n", __func__, __LINE__);
		return NULL;
	}

	skb = genlmsg_new((size_t)count, GFP_ATOMIC);
	if (!skb) {
		pr_err("hismart:[%s]:line[%d] genlmsg_new error\n", __func__, __LINE__);
		return NULL;
	}

	msg_header = genlmsg_put(skb, 0, flp_event_seqnum++, &ar_genl_family, 0, cmd_type);
	if (!msg_header) {
		pr_err("hismart:[%s]:line[%d] genlmsg_put error\n", __func__, __LINE__);
		nlmsg_free(skb);
		return NULL;
	}

	*data = nla_reserve_nohdr(skb, (int)count);
	if (NULL == *data) {
		pr_err("hismart:[%s]:line[%d] nla_reserve_nohdr error\n", __func__, __LINE__);
		nlmsg_free(skb);
		return NULL;
	}

	pdata->data_len = count;

	return skb;
}
/*lint -e826*/
static int context_gnetlink_send(ar_port_t *ar_port, struct sk_buff *skb)
{
	ar_data_buf_t *pdata =  &ar_port->ar_buf;
	struct nlmsghdr *nlh;
	int result;
	if (!skb) {
		return -EINVAL;
	}
	nlh = nlmsg_hdr(skb);
	nlh->nlmsg_len = pdata->data_len + GENL_HDRLEN + NLMSG_HDRLEN;
	result = genlmsg_unicast(&init_net, skb, ar_port->portid);
	if (result)
		pr_err("hismart:[%s]:line[%d] ar:genlmsg_unicast %d", __func__, __LINE__, result);

	return result;
}

static int ar_data_report(ar_data_buf_t *pdata, ar_port_t *ar_port)
{
	unsigned char *data = NULL;
	struct sk_buff *skb;
	int ret = 0;
	if (0 == fifo_len(pdata))
		goto AR_DATA_REPO_FIN;

	skb = context_gnetlink_alloc(ar_port, pdata->data_count, AR_GENL_CMD_AR_DATA, &data);
	if (!data || !skb) {
		pr_err("hismart:[%s]:line[%d] context_gnetlink_alloc ERR\n", __func__, __LINE__);
		ret = -EBUSY;
		goto AR_DATA_REPO_FIN;
	}

	ret = fifo_out(pdata, data, pdata->data_count);
	if (ret) {
		nlmsg_free(skb);
		pr_err("hismart:[%s]:line[%d] fifo_out ERR[%d]\n", __func__, __LINE__, ret);
		goto AR_DATA_REPO_FIN;
	}

	ret = context_gnetlink_send(ar_port, skb);
AR_DATA_REPO_FIN:
	return ret;
}

static int get_ar_data_from_mcu(const pkt_header_t *head)
{
	unsigned int i, port_etype;
	ar_data_buf_t *pdata;
	const ar_data_req_t *pd = (const ar_data_req_t *)head;
	unsigned char *pcd;
	unsigned char *data_tail = (unsigned char *)head + sizeof(pkt_header_t) + head->length;
	context_event_t * pevent;
	int ret = 0;
	ar_port_t *ar_port;
	struct list_head *pos;

	if(0 == pd->context_num || AR_STATE_BUTT < pd->context_num) {
		pr_err("hismart:[%s]:line[%d] context_num [%d]err\n", __func__, __LINE__, pd->context_num);
		return -EBUSY;
	}

	mutex_lock(&g_ar_dev.lock);
	printk(HISI_AR_DEBUG"hismart:[%s]:line[%d] num[%d]\n", __func__, __LINE__, pd->context_num);
	list_for_each(pos, &g_ar_dev.list) {
		ar_port = container_of(pos, ar_port_t, list);
		if (!(FLP_AR_DATA & ar_port->channel_type) || !ar_port->portid)
			continue;

		pdata = &ar_port->ar_buf;
		pcd = (unsigned char *)pd->context_event;
		for (i = 0; i < pd->context_num; i++) {
			if(pcd >= data_tail){
				pr_err("hismart:[%s]:line[%d] break[%d][%pK]\n", __func__, __LINE__, i, pcd);
				break;
			}
			pevent = (context_event_t *)pcd;
			if(AR_STATE_BUTT <= pevent->context) {
				pr_err("hismart:[%s]:line[%d] pevent->context[%d] too large\n", __func__, __LINE__, pevent->context);
				break;
			}
			port_etype = ar_port->ar_config.context_list[pevent->context].head.event_type;
			if((port_etype & pevent->event_type) || ((FLP_AR_DATA & ar_port->flushing) && (!pevent->event_type))) {
				if (pevent->buf_len <= CONTEXT_PRIVATE_DATA_MAX){
					fifo_in(pdata, (char *)pevent,
					(unsigned int)(sizeof(context_event_t) + pevent->buf_len), (unsigned int)1);
				} else {
					pr_err("hismart:[%s]:line[%d] type[%d]len[%d] private data too large\n", __func__, __LINE__,pevent->event_type,pevent->buf_len);
					break;
				}

				if(!pevent->event_type)
					ar_port->flushing &= ~FLP_AR_DATA;
			}

			pcd += sizeof(context_event_t) + pevent->buf_len;
		}

		(void)ar_data_report(pdata, ar_port);
	}
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}

static int get_env_data_from_mcu(const pkt_header_t *head)
{
	unsigned int i, port_etype;
	ar_data_buf_t *pdata;
	const ar_data_req_t *pd = (const ar_data_req_t *)head;
	unsigned char *pcd;
	unsigned char *data_tail = (unsigned char *)head + sizeof(pkt_header_t) + head->length;
	context_event_t * pevent;
	struct sk_buff *skb;
	int ret = 0;
	ar_port_t *ar_port = g_ar_dev.envdev_priv.env_port;
	unsigned char *data = NULL;

	if(0 == pd->context_num || AR_ENVIRONMENT_END < pd->context_num) {
		pr_err("hismart:[%s]:line[%d] context_num [%d]err\n", __func__, __LINE__, pd->context_num);
		return -EBUSY;
	}

	printk(HISI_AR_DEBUG"hismart:[%s]:line[%d] num[%d]\n", __func__, __LINE__, pd->context_num);

	mutex_lock(&g_ar_dev.lock);

	if(NULL == ar_port) {
		pr_err("hismart:[%s]:line[%d] env_port must init first\n", __func__, __LINE__);
		goto ENV_DATA_FIN;
	}

	pdata = &ar_port->env_buf;
	pcd = (unsigned char *)pd->context_event;
	for (i = 0; i < pd->context_num; i++) {
		if(pcd >= data_tail)
			goto ENV_DATA_FIN;

		pevent = (context_event_t *)pcd;
		if(AR_ENVIRONMENT_END <= pevent->context) {
			pr_err("hismart:[%s]:line[%d] pevent->context[%d]idx[%d]\n", __func__, __LINE__, pevent->context,i);
			goto ENV_DATA_FIN;
		}

		if (pevent->buf_len > CONTEXT_PRIVATE_DATA_MAX) {
			pr_err("hismart:[%s]:line[%d] type[%d]len[%d] private data too large.idx[%d]\n", __func__, __LINE__, pevent->event_type,pevent->buf_len,i);
			goto ENV_DATA_FIN;
		}

		port_etype = g_ar_dev.envdev_priv.env_enable[pevent->context].head.event_type;
		printk(HISI_AR_DEBUG "hismart:[%s]:line[%d] num[%u]type[%u]ctt[%u]msec[%llu]len[%u] channel_type[0x%x] confident[%u][%u]\n", __func__, __LINE__,
			pd->context_num, pevent->event_type, pevent->context, pevent->msec, pevent->buf_len,
			ar_port->channel_type, pevent->confident, port_etype);
		if(port_etype & pevent->event_type)
			fifo_in(pdata, (char *)pevent,
				(unsigned int)(sizeof(context_event_t) + pevent->buf_len), (unsigned int)1);

		pcd += sizeof(context_event_t) + pevent->buf_len;
	}

	if (0 == fifo_len(pdata))
		goto ENV_DATA_FIN;
	skb = context_gnetlink_alloc(ar_port, pdata->data_count, AR_GENL_CMD_ENV_DATA, &data);
	if (!data || !skb) {
		pr_err("hismart:[%s]:line[%d] context_gnetlink_alloc ERR\n", __func__, __LINE__);
		ret = -EBUSY;
		goto ENV_DATA_FIN;
	}

	ret = fifo_out(pdata, data, pdata->data_count);
	if (ret) {
		pr_err("hismart:[%s]:line[%d] fifo_out ERR\n", __func__, __LINE__);
		nlmsg_free(skb);
		goto ENV_DATA_FIN;
	}
	HISI_AR_DEBUG_DUMP_OPT(HISI_AR_DEBUG_DUMP, "env: ", DUMP_PREFIX_ADDRESS,
		16, 1, data, pdata->data_count, true);
	context_gnetlink_send(ar_port, skb);
ENV_DATA_FIN:
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}

static int ar_state_shmem(const pkt_header_t *head)
{
	ar_state_t *staus = (ar_state_t *)(head + 1);
	if (head->length < (uint16_t)sizeof(unsigned int) || (head->length > sizeof(ar_state_shmen_t))) {
		pr_err("hismart:[%s]:line[%d] get state len is invalid[%d]\n", __func__, __LINE__, (int)head->length);
		goto STATE_SHMEM_ERROR;
	}

	if (0 == staus->context_num || GET_STATE_NUM_MAX < staus->context_num) {
		pr_err("hismart:[%s]:line[%d] context_num err[%d]\n", __func__, __LINE__, staus->context_num);
		goto STATE_SHMEM_ERROR;
	}

	g_ar_dev.activity_shemem.user_len = (unsigned int)(head->length - sizeof(unsigned int));
	memcpy_s(g_ar_dev.activity_shemem.context_event, sizeof(ar_state_shmen_t) - sizeof(int), staus->context_event, g_ar_dev.activity_shemem.user_len);
STATE_SHMEM_ERROR:
	complete(&g_ar_dev.get_complete);
	return 0;
}

/*lint +e826*/
/*lint -e737*/
static int state_v2_cmd(struct read_info *rd, unsigned long arg)
{
	int ret = 0;
	unsigned long  usr_len;
	ar_state_t* date_report;

	usr_len = rd->data_length - sizeof(unsigned int);
	if ((0 == usr_len) || (usr_len > (MAX_PKT_LENGTH - sizeof(ar_state_t)))) {
		pr_err("hismart:[%s]:line[%d] length err[%lu] buflen_limit[%lu]\n", __func__, __LINE__, usr_len, MAX_PKT_LENGTH - sizeof(ar_state_t));
		ret = -EINVAL;
		goto STATE_V2_ERR;
	}

	date_report = (ar_state_t*)rd->data;
	if (0 == date_report->context_num || GET_STATE_NUM_MAX < date_report->context_num) {
		pr_err("hismart:[%s]:line[%d] context_num err[%d]\n", __func__, __LINE__, date_report->context_num);
		ret = -EPERM;
		goto STATE_V2_ERR;
	}

	if (copy_to_user((void *)arg, date_report->context_event, usr_len)) {
		pr_err("hismart:[%s]:line[%d] [STATE]copy_to_user ERR\n", __func__, __LINE__);
		ret = -EFAULT;
		goto STATE_V2_ERR;
	}

	return (int)usr_len;
STATE_V2_ERR:
	return ret;
}
/*lint +e737*/
static int ar_state_v2_cmd(ar_port_t *ar_port, unsigned long arg)
{
	int ret = 0;
	mutex_lock(&g_ar_dev.state_lock);
	if (!(FLP_AR_DATA & ar_port->channel_type) || !(FLP_AR_DATA & g_ar_dev.service_type)) {
		pr_err("hismart:[%s]:line[%d] channel[0x%x] service[0x%x]\n", __func__, __LINE__, ar_port->channel_type, g_ar_dev.service_type);
		goto AR_STATE_V2_ERR;
	}

	memset_s(&g_ar_dev.activity_shemem, sizeof(ar_state_shmen_t), 0, sizeof(ar_state_shmen_t));
	reinit_completion(&g_ar_dev.get_complete);

#ifdef CONFIG_INPUTHUB_20
	ret = ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ,
		SUB_CMD_FLP_AR_GET_STATE_REQ, NULL, (size_t)0);
#else
	ret = ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ,
		CMD_FLP_AR_GET_STATE_REQ, NULL, (size_t)0);
#endif
	if (ret) {
		pr_err("hismart:[%s]:line[%d] ar_send_cmd_from_kernel ERR[%d]\n", __func__, __LINE__, ret);
		ret = -EFAULT;
		goto AR_STATE_V2_ERR;
	}

	if (!wait_for_completion_timeout(&g_ar_dev.get_complete, msecs_to_jiffies(2000))) {
		pr_err("hismart:[%s]:line[%d] Get state timeout\n", __func__, __LINE__);
		ret = -EFAULT;
		goto AR_STATE_V2_ERR;
	}

	if (g_ar_dev.activity_shemem.user_len > (unsigned int)(sizeof(ar_state_shmen_t) - sizeof(int))) {
		pr_err("hismart:[%s]:line[%d] get len[%d] ERR\n", __func__, __LINE__, (int)g_ar_dev.activity_shemem.user_len);
		ret = -EFAULT;
		goto AR_STATE_V2_ERR;
	}

	if (copy_to_user((void *)arg, g_ar_dev.activity_shemem.context_event, g_ar_dev.activity_shemem.user_len)) {
		pr_err("hismart:[%s]:line[%d] [STATE]copy_to_user ERR\n", __func__, __LINE__);
		ret = -EFAULT;
		goto AR_STATE_V2_ERR;
	}

	ret = (int)g_ar_dev.activity_shemem.user_len;



AR_STATE_V2_ERR:
	mutex_unlock(&g_ar_dev.state_lock);
	return ret;
}

static int env_state_cmd(ar_port_t *ar_port, unsigned long arg)
{
	struct read_info rd;
	int ret = 0;

	mutex_lock(&g_ar_dev.lock);

	if (!(FLP_ENVIRONMENT & ar_port->channel_type) || !(FLP_ENVIRONMENT & g_ar_dev.service_type)) {
		pr_err("hismart:[%s]:line[%d] channel[0x%x] service[0x%x]\n", __func__, __LINE__, ar_port->channel_type, g_ar_dev.service_type);
		ret =  -EFAULT;
		goto ENV_STATE_ERR;
	}

	memset_s((void*)&rd,sizeof(struct read_info), 0,sizeof(struct read_info));
#ifdef CONFIG_INPUTHUB_20
	ret = ar_send_cmd_from_kernel_response(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ,
		SUB_CMD_ENVIRONMENT_GET_STATE_REQ, NULL, (size_t)0, &rd);
#else
	ret = ar_send_cmd_from_kernel_response(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ,
		CMD_ENVIRONMENT_GET_STATE_REQ, NULL, (size_t)0, &rd);
#endif
	if (ret) {
		pr_err("hismart:[%s]:line[%d] ar_send_cmd_from_kernel_response err[%d]\n", __func__, __LINE__, rd.errno);
		goto ENV_STATE_ERR;
	}

	ret = state_v2_cmd(&rd, arg);
	if (ret < 0) {
		pr_err("hismart:[%s]:line[%d] state_v2_cmd err[%d]\n", __func__, __LINE__, ret);
		goto ENV_STATE_ERR;
	}

ENV_STATE_ERR:
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}

static int  env_init_cmd(ar_port_t *ar_port, unsigned long arg)
{
	env_init_t env_init;
	envdev_priv_t *envdev_priv = &g_ar_dev.envdev_priv;

	int ret = env_open_cmd(ar_port);
	if (ret) {
		pr_err("hismart:[%s]:line[%d] env_open_cmd err\n", __func__, __LINE__);
		return ret;
	}

	mutex_lock(&g_ar_dev.lock);
	memset_s((void*)&env_init, sizeof(env_init_t), 0, sizeof(env_init_t));
	if (copy_from_user((void *)&env_init,  (void __user *)arg, sizeof(env_init_t))) {
		pr_err("hismart:[%s]:line[%d] copy_from_user error env_init\n", __func__, __LINE__);
		ret = -EDOM;
		goto ENV_INIT_ERR;
	}

	if (AR_ENVIRONMENT_END <= env_init.context || AR_ENVIRONMENT_BEGIN > env_init.context) {
		pr_err("hismart:[%s]:line[%d] context too big[%d]\n", __func__, __LINE__, env_init.context);
		ret = -EINVAL;
		goto ENV_INIT_ERR;
	}

	memcpy_s((void*)&envdev_priv->env_init[env_init.context],sizeof(env_init_t), &env_init, sizeof(env_init_t));
#ifdef CONFIG_INPUTHUB_20
	ret = ar_send_cmd_from_kernel(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ, SUB_CMD_ENVIRONMENT_INIT_REQ,
		(char *)&env_init, sizeof(env_init_t));
#else
	ret = ar_send_cmd_from_kernel(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ, CMD_ENVIRONMENT_INIT_REQ,
		(char *)&env_init, sizeof(env_init_t));
#endif
	data_dump();
ENV_INIT_ERR:
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}

static int  env_exit_cmd(ar_port_t *ar_port, unsigned long arg)
{
	int ret = 0;
	env_init_t env_init;
	envdev_priv_t *envdev_priv = &g_ar_dev.envdev_priv;
	mutex_lock(&g_ar_dev.lock);
	if (!(FLP_ENVIRONMENT & ar_port->channel_type) || !(FLP_ENVIRONMENT & g_ar_dev.service_type)) {
		pr_err("hismart:[%s]:line[%d] env has not open channel[0x%x] service[0x%x]\n", __func__, __LINE__, ar_port->channel_type, g_ar_dev.service_type);
		ret = -EPERM;
		goto ENV_EXIT_ERR;
	}
	memset_s((void*)&env_init,sizeof(env_init_t), 0, sizeof(env_init_t));
	if (copy_from_user((void *)&env_init,  (void __user *)arg, sizeof(env_init_t))) {
		pr_err("hismart:[%s]:line[%d] copy_from_user error env_init\n", __func__, __LINE__);
		ret = -EDOM;
		goto ENV_EXIT_ERR;
	}

	if (AR_ENVIRONMENT_END <= env_init.context || AR_ENVIRONMENT_BEGIN > env_init.context) {
		pr_err("hismart:[%s]:line[%d] context too big[%d]\n", __func__, __LINE__, env_init.context);
		ret = -EINVAL;
		goto ENV_EXIT_ERR;
	}

	envdev_priv->env_enable[env_init.context].head.event_type = 0;
	envdev_priv->env_init[env_init.context].context = AR_ENVIRONMENT_END;
#ifdef CONFIG_INPUTHUB_20
	ar_send_cmd_from_kernel(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ, SUB_CMD_ENVIRONMENT_EXIT_REQ, (char *)&env_init, sizeof(env_init_t));
#else
	ar_send_cmd_from_kernel(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ, CMD_ENVIRONMENT_EXIT_REQ, (char *)&env_init, sizeof(env_init_t));
#endif
	env_close_cmd(ar_port, ENV_CLOSE_NORMAL);
	data_dump();
ENV_EXIT_ERR:
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}

static int  env_data_cmd(ar_port_t *ar_port, unsigned long arg)
{
	int ret = 0;
	void *data;
	env_data_download_t download;
	mutex_lock(&g_ar_dev.lock);
	if (!(FLP_ENVIRONMENT & ar_port->channel_type) || !(FLP_ENVIRONMENT & g_ar_dev.service_type)) {
		pr_err("hismart:[%s]:line[%d] env has not open channel[0x%x] service[0x%x]\n", __func__, __LINE__, ar_port->channel_type, g_ar_dev.service_type);
		ret = -EPERM;
		goto ENV_DATA_FIN;
	}

	if (copy_from_user((void *)&download,  (void __user *)arg, sizeof(env_data_download_t))) {
		pr_err("hismart:[%s]:line[%d] copy_from_user  download error \n", __func__, __LINE__);
		ret = -EDOM;
		goto ENV_DATA_FIN;
	}

	if(download.len >DATABASE_DATALEN || !download.len) {
		pr_err("hismart:[%s]:line[%d] copy_from_user len to big[%d] \n", __func__, __LINE__, download.len);
		ret = -E2BIG;
		goto ENV_DATA_FIN;
	}

	data = kzalloc(download.len, GFP_KERNEL);
	if(!data) {
		pr_err("hismart:[%s]:line[%d] malloc ENOMEM\n", __func__, __LINE__);
		ret = -ENOMEM;
		goto ENV_DATA_FIN;
	}

	if (copy_from_user(data,  (void __user *)download.bufaddr, download.len)) {
		pr_err("hismart:[%s]:line[%d] copy_from_user error [%pK]len[%u]\n", __func__, __LINE__, download.bufaddr, download.len);
		ret = -EDOM;
		goto ENV_DATA_ERR;
	}
#ifdef CONFIG_CONTEXTHUB_SHMEM
	ret = shmem_send(TAG_ENVIRONMENT, data, download.len);
	if(ret)
		pr_err("hismart:[%s]:line[%d] shmem_send error \n", __func__, __LINE__);
#endif

ENV_DATA_ERR:
	kfree((void*)data);
ENV_DATA_FIN:
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}

static int  env_stop_cmd(ar_port_t *ar_port)
{
	int ret;
	mutex_lock(&g_ar_dev.lock);
	ret = env_close_cmd(ar_port, ENV_CLOSE_FORCE);
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}

static int ar_flush_cmd(ar_port_t *ar_port)
{
	int ret = 0;
	mutex_lock(&g_ar_dev.lock);
	if (!(FLP_AR_DATA & ar_port->channel_type) || !(FLP_AR_DATA & g_ar_dev.service_type)) {
		pr_err("hismart:[%s]:line[%d] activity has not start channel[0x%x] service[0x%x]\n", __func__, __LINE__, ar_port->channel_type, g_ar_dev.service_type);
		ret = -EPERM;
		goto AR_FLUSH_ERR;
	}
#ifdef CONFIG_INPUTHUB_20
	ret = ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ,
		SUB_CMD_FLP_AR_FLUSH_REQ, NULL, (size_t)0);
#else
	ret = ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ,
		CMD_FLP_AR_FLUSH_REQ, NULL, (size_t)0);
#endif
	if (ret) {
		pr_err("hismart:[%s]:line[%d] ar_send_cmd_from_kernel ERR[%d]\n", __func__, __LINE__, ret);
		ret = -EFAULT;
		goto AR_FLUSH_ERR;
	}

	ar_port->flushing |= FLP_AR_DATA;
AR_FLUSH_ERR:
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}

static bool ar_check_cmd(unsigned int cmd, int type)
{
	switch (type) {
	case FLP_AR_DATA:
		if (((cmd & FLP_IOCTL_TAG_MASK) == FLP_IOCTL_TAG_GPS) ||
		((cmd & FLP_IOCTL_TAG_MASK) == FLP_IOCTL_TAG_AR)) {
			return true;
		}
	break ;
	case FLP_ENVIRONMENT:
		if ((cmd & FLP_IOCTL_TAG_MASK) == FLP_IOCTL_TAG_AR)
			return true;
	break;
	default : break ;
	}
	return 0;
}

/*lint -e845*/
static int flp_ar_ioctl(ar_port_t *ar_port, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	switch (cmd & FLP_IOCTL_CMD_MASK) {
	case FLP_IOCTL_AR_CONFIG(0):
		ret =  ar_config_cmd(ar_port, cmd, arg);
	break;
	case FLP_IOCTL_AR_STOP(0):
		ret = ar_stop_cmd(ar_port, arg);
	break;
	case FLP_IOCTL_AR_FLUSH(0):
		ret = ar_flush_cmd(ar_port);
	break;
	case FLP_IOCTL_AR_STATE_V2(0):
		ret = ar_state_v2_cmd(ar_port, arg);
	break;
	default:
		printk(KERN_ERR "hismart:[%s]:line[%d] flp_ar_ioctrl input cmd[0x%x] error\n", __func__, __LINE__, cmd);
		return -EFAULT;
	}
	return ret;
}

static int flp_env_ioctl(ar_port_t *ar_port, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	if (!ar_check_cmd(cmd, FLP_ENVIRONMENT))
			return -EPERM;

	switch (cmd & FLP_IOCTL_CMD_MASK) {
	case FLP_IOCTL_ENV_INIT(0):
		ret = env_init_cmd(ar_port, arg);
	break;
	case FLP_IOCTL_ENV_ENABLE(0):
		ret = env_enable_cmd(ar_port, arg);
	break;
	case FLP_IOCTL_ENV_STATE(0):
		ret = env_state_cmd(ar_port, arg);
	break;
	case FLP_IOCTL_ENV_DATA(0):
		ret = env_data_cmd(ar_port, arg);
	break;
	case FLP_IOCTL_ENV_DISABLE(0):
		ret = env_disable_cmd(ar_port, arg);
	break;
	case FLP_IOCTL_ENV_EXIT(0):
		ret = env_exit_cmd(ar_port, arg);
	break;
	case FLP_IOCTL_ENV_STOP(0):
		ret = env_stop_cmd(ar_port);
	break;
	default:
		printk(KERN_ERR "hismart:[%s]:line[%d] flp_ar_ioctrl input cmd[0x%x] error\n", __func__, __LINE__, cmd);
		return -EFAULT;
	}
	return ret;
}

static int ar_common_ioctl_open_service(void)
{
	struct read_info rd;
	int ret =0, i;

	memset_s((void*)&rd, sizeof(struct read_info), 0,sizeof(struct read_info));
	pr_info("hismart:[%s]:line[%d] service[%x] AR user cnt[%d]\n", __func__, __LINE__, g_ar_dev.service_type, g_ar_dev.ar_devinfo.usr_cnt);
	if (g_ar_dev.service_type & FLP_AR_DATA && g_ar_dev.ar_devinfo.usr_cnt) {
		ret = ar_send_cmd_from_kernel_response(TAG_AR, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0, &rd);
		if(0 == ret && g_ar_dev.ar_devinfo.cfg_sz && g_ar_dev.ar_devinfo.cfg.context_num)
		#ifdef CONFIG_INPUTHUB_20
			ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_AR_START_REQ,
			(char *)&g_ar_dev.ar_devinfo.cfg, (size_t)g_ar_dev.ar_devinfo.cfg_sz);
		#else
			ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, CMD_FLP_AR_START_REQ,
			(char *)&g_ar_dev.ar_devinfo.cfg, (size_t)g_ar_dev.ar_devinfo.cfg_sz);
		#endif
	}

	if (g_ar_dev.service_type & FLP_ENVIRONMENT) {
		ret = ar_send_cmd_from_kernel_response(TAG_ENVIRONMENT, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0, &rd);
		if(ret) {
			pr_err("hismart:[%s]:line[%d] ar_send_cmd_from_kernel_response ret[%d]\n",__func__, __LINE__, ret);
			return ret;
		}

		for(i = 0; i<AR_ENVIRONMENT_END; i++) {
			if(AR_ENVIRONMENT_END != g_ar_dev.envdev_priv.env_init[i].context)
			#ifdef CONFIG_INPUTHUB_20
				ar_send_cmd_from_kernel(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ, SUB_CMD_ENVIRONMENT_INIT_REQ,
					(char *)&g_ar_dev.envdev_priv.env_init[i], sizeof(env_init_t));
			#else
				ar_send_cmd_from_kernel(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ, CMD_ENVIRONMENT_INIT_REQ,
					(char *)&g_ar_dev.envdev_priv.env_init[i], sizeof(env_init_t));
			#endif
			if (g_ar_dev.envdev_priv.env_enable[i].head.event_type)
			#ifdef CONFIG_INPUTHUB_20
				ar_send_cmd_from_kernel(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ, SUB_CMD_ENVIRONMENT_ENABLE_REQ,
					(char *)&g_ar_dev.envdev_priv.env_enable[i], sizeof(env_enable_t));
			#else
				ar_send_cmd_from_kernel(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ, CMD_ENVIRONMENT_ENABLE_REQ,
					(char *)&g_ar_dev.envdev_priv.env_enable[i], sizeof(env_enable_t));
			#endif
		}
	}

	return ret;
}

static int ar_common_ioctl_close_service(void)
{
	unsigned int data = 0;
	if ((g_ar_dev.service_type & FLP_AR_DATA) && (g_ar_dev.ar_devinfo.usr_cnt)) {
	#ifdef CONFIG_INPUTHUB_20
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_AR_STOP_REQ,
			(char *)&data, sizeof(int));
	#else
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, CMD_FLP_AR_STOP_REQ,
			(char *)&data, sizeof(int));
	#endif
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CLOSE_REQ, 0, NULL, (size_t)0);
	}

	if (g_ar_dev.service_type & FLP_ENVIRONMENT)
		ar_send_cmd_from_kernel(TAG_ENVIRONMENT, CMD_CMN_CLOSE_REQ, 0, NULL, (size_t)0);
	return 0;
}
/*lint -e455*/
static int ar_common_ioctl(ar_port_t *ar_port, unsigned int cmd, unsigned long arg)
{
	unsigned int data = 0;
	int ret = 0;

	if (FLP_IOCTL_COMMON_RELEASE_WAKELOCK != cmd) {
		if (copy_from_user(&data, (void *)arg, sizeof(unsigned int))) {
			printk(KERN_ERR "hismart:[%s]:line[%d] flp_ioctl copy_from_user error[%d]\n", __func__, __LINE__, ret);
			return -EFAULT;
		}
	}

	switch (cmd) {
	case FLP_IOCTL_COMMON_SLEEP:
		printk(HISI_AR_DEBUG "hismart:[%s]:line[%d] start timer %d\n", __func__, __LINE__, data);
		/*if timer is running just delete it ,then restart it*/
		hisi_softtimer_delete(&ar_port->sleep_timer);
		ret = hisi_softtimer_modify(&ar_port->sleep_timer, data);
		if (!ret)
		hisi_softtimer_add(&ar_port->sleep_timer);
	break ;
	case FLP_IOCTL_COMMON_AWAKE_RET:
		ar_port->need_awake = data;
	break ;
	case FLP_IOCTL_COMMON_SETPID:
		ar_port->portid = data;
	break ;
	case FLP_IOCTL_COMMON_CLOSE_SERVICE:
		mutex_lock(&g_ar_dev.lock);
		g_ar_dev.denial_sevice = data;
		if(g_ar_dev.denial_sevice)
			ar_common_ioctl_close_service();
		else
			ar_common_ioctl_open_service();
		mutex_unlock(&g_ar_dev.lock);
	break;
	case FLP_IOCTL_COMMON_HOLD_WAKELOCK:
		ar_port->need_hold_wlock = data;
	break ;
	case FLP_IOCTL_COMMON_RELEASE_WAKELOCK:
		if (ar_port->need_hold_wlock)
			wake_unlock(&ar_port->wlock);
	break;
	default:
		printk(KERN_ERR "hismart:[%s]:line[%d] ar_common_ioctl input cmd[0x%x] error\n", __func__, __LINE__, cmd);
		return -EFAULT;
	}
	return 0;
}
/*lint +e455*/
static long ar_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	ar_port_t *ar_port  = (ar_port_t *)file->private_data;
	if (!ar_port) {
		printk(KERN_ERR "hismart:[%s]:line[%d] flp_ioctl parameter error\n", __func__, __LINE__);
		return -EINVAL;
	}
	printk(HISI_AR_DEBUG "hismart:[%s]:line[%d] cmd[0x%x]\n\n", __func__, __LINE__, cmd&0x0FFFF);
	mutex_lock(&g_ar_dev.lock);
	if ((g_ar_dev.denial_sevice) && (cmd != FLP_IOCTL_COMMON_CLOSE_SERVICE)) {
		mutex_unlock(&g_ar_dev.lock);
		return 0;
	}
	mutex_unlock(&g_ar_dev.lock);

	switch (cmd & FLP_IOCTL_TYPE_MASK) {
	case FLP_IOCTL_TYPE_AR:
		if (!ar_check_cmd(cmd, FLP_AR_DATA))
			return -EPERM;
		return (long)flp_ar_ioctl(ar_port, cmd, arg);
	case FLP_IOCTL_TYPE_ENV:
		return (long)flp_env_ioctl(ar_port, cmd, arg);
	case FLP_IOCTL_TYPE_COMMON:
		return (long)ar_common_ioctl(ar_port, cmd, arg);
	default:
		printk(KERN_ERR "hismart:[%s]:line[%d] flp_ioctl input cmd[0x%x] error\n", __func__, __LINE__, cmd);
		return -EFAULT;
	}
}

static void  ar_service_recovery(void)
{
	int i;
	envdev_priv_t *envdev_priv = &g_ar_dev.envdev_priv;

	if (g_ar_dev.service_type & FLP_AR_DATA) {
		ar_send_cmd_from_kernel_nolock(TAG_AR, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0);
	#ifdef CONFIG_INPUTHUB_20
		ar_send_cmd_from_kernel_nolock(TAG_AR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_AR_START_REQ,
		(char *)&g_ar_dev.ar_devinfo.cfg, (size_t)g_ar_dev.ar_devinfo.cfg_sz);
	#else
		ar_send_cmd_from_kernel_nolock(TAG_AR, CMD_CMN_CONFIG_REQ, CMD_FLP_AR_START_REQ,
		(char *)&g_ar_dev.ar_devinfo.cfg, (size_t)g_ar_dev.ar_devinfo.cfg_sz);
	#endif
	}

	if (g_ar_dev.service_type & FLP_ENVIRONMENT) {
		ar_send_cmd_from_kernel_nolock(TAG_ENVIRONMENT, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0);
		for(i = 0; i< AR_ENVIRONMENT_END; i++) {
			if (AR_ENVIRONMENT_END != envdev_priv->env_init[i].context)
			#ifdef CONFIG_INPUTHUB_20
				ar_send_cmd_from_kernel_nolock(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ, SUB_CMD_ENVIRONMENT_INIT_REQ,
					(char *)&envdev_priv->env_init[i], sizeof(env_init_t));
			#else
				ar_send_cmd_from_kernel_nolock(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ, CMD_ENVIRONMENT_INIT_REQ,
					(char *)&envdev_priv->env_init[i], sizeof(env_init_t));
			#endif
		}

		for(i = 0; i< AR_ENVIRONMENT_END; i++) {
			if(envdev_priv->env_enable[i].head.event_type)
			#ifdef CONFIG_INPUTHUB_20
				ar_send_cmd_from_kernel_nolock(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ, SUB_CMD_ENVIRONMENT_ENABLE_REQ,
					(char *)&envdev_priv->env_enable[i], sizeof(env_enable_t));
			#else
				ar_send_cmd_from_kernel_nolock(TAG_ENVIRONMENT, CMD_CMN_CONFIG_REQ, CMD_ENVIRONMENT_ENABLE_REQ,
					(char *)&envdev_priv->env_enable[i], sizeof(env_enable_t));
			#endif
		}
	}
	printk(HISI_AR_DEBUG "hismart:[%s]:line[%d] service_type[%d]\n", __func__, __LINE__, g_ar_dev.service_type);
}
/*lint -e715*/
static int ar_notifier(struct notifier_block *nb,
            unsigned long action, void *data)
{
	switch (action) {
	case IOM3_RECOVERY_3RD_DOING:
		ar_service_recovery();
	break;
	default:break;
	}
	return 0;
}
/*lint +e715*/
extern int register_iom3_recovery_notifier(struct notifier_block *nb);
static struct notifier_block ar_reboot_notify = {
    .notifier_call = ar_notifier,
    .priority = -1,
};

static void ar_timerout_work(struct work_struct *wk)
{
	unsigned char *data = NULL;
	struct sk_buff *skb;
	ar_port_t *ar_port  = container_of(wk, ar_port_t, work);
	pr_info("hismart:[%s]:line[%d]\n", __func__, __LINE__);
	mutex_lock(&g_ar_dev.lock);
	skb = context_gnetlink_alloc(ar_port, 0, ar_port->work_para, &data);
	if (skb)
		context_gnetlink_send(ar_port, skb);
	mutex_unlock(&g_ar_dev.lock);
}

static void ar_sleep_timeout(unsigned long data)
{
	ar_port_t *ar_port = (ar_port_t *)data;
	pr_info("hismart:[%s]:line[%d]\n", __func__, __LINE__);
	if (ar_port) {
		ar_port->work_para = AR_GENL_CMD_NOTIFY_TIMEROUT;
		if (ar_port->portid) {
			queue_work(system_power_efficient_wq, &ar_port->work);
		} else {
			pr_info("hismart:[%s]:line[%d] not set pid, timer out but not report\n", __func__, __LINE__);
		}
		if (ar_port->need_hold_wlock) {
			pr_info("hismart:[%s]:line[%d] wake_lock_timeout 2\n", __func__, __LINE__);
			wake_lock_timeout(&ar_port->wlock, (long)(2 * HZ));
		}
	}
	return;
}

static int ar_open(struct inode *inode, struct file *filp)/*lint -e715*/
{
	int ret = 0;
	ar_port_t *ar_port;
	struct list_head *pos;
	int count = 0;

	mutex_lock(&g_ar_dev.lock);
	list_for_each(pos, &g_ar_dev.list) {
		count++;
	}

	if(count > 32) {
		pr_err("hismart:[%s]:line[%d] ar_open clinet limit\n", __func__, __LINE__);
		ret = -EACCES;
		goto ar_open_ERR;
	}

	ar_port  = (ar_port_t *)kzalloc(sizeof(ar_port_t), GFP_KERNEL);
	if (!ar_port) {
		pr_err("hismart:[%s]:line[%d] no mem\n", __func__, __LINE__);
		ret = -ENOMEM;
		goto ar_open_ERR;
	}
	INIT_LIST_HEAD(&ar_port->list);
	hisi_softtimer_create(&ar_port->sleep_timer, ar_sleep_timeout, (unsigned long)ar_port, 0);
	INIT_WORK(&ar_port->work, ar_timerout_work);

	list_add_tail(&ar_port->list, &g_ar_dev.list);
	wake_lock_init(&ar_port->wlock, WAKE_LOCK_SUSPEND, "hisi_ar");
	filp->private_data = ar_port;
	printk(HISI_AR_DEBUG "hismart:[%s]:line[%d] v1.4 enter\n", __func__, __LINE__);

ar_open_ERR:
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}


/*lint -e438*/
static int ar_release(struct inode *inode, struct file *file)/*lint -e715*/
{
	ar_port_t *ar_port  = (ar_port_t *)file->private_data;
	struct list_head    *pos;
	ar_port_t      *port;
	printk(HISI_AR_DEBUG"hismart:[%s]:line[%d]\n", __func__, __LINE__);
	if (!ar_port) {
		printk(KERN_ERR "hismart:[%s]:line[%d] flp_close parameter error\n", __func__, __LINE__);
		return -EINVAL;
	}

	hisi_softtimer_delete(&ar_port->sleep_timer);
	cancel_work_sync(&ar_port->work);
	wake_lock_destroy(&ar_port->wlock);

	mutex_lock(&g_ar_dev.lock);
	list_del(&ar_port->list);
/*if andriod vm restart, apk doesnot send stop cmd,just adjust it*/
	g_ar_dev.ar_devinfo.usr_cnt = 0;
	list_for_each(pos, &g_ar_dev.list) {
		port = container_of(pos, ar_port_t, list);
		if (port->channel_type & FLP_AR_DATA) {
			g_ar_dev.ar_devinfo.usr_cnt++ ;
		}
	}
	ar_stop(0);

	if (FLP_ENVIRONMENT & ar_port->channel_type)
		env_close_cmd(ar_port, ENV_CLOSE_FORCE);
	data_buffer_exit(&ar_port->ar_buf);
	kfree(ar_port);
	ar_port = NULL;
	file->private_data = NULL;
	mutex_unlock(&g_ar_dev.lock);
	return 0;
}
/*lint -e785*/
static const struct file_operations ar_fops = {
	.owner =          THIS_MODULE,
	.llseek =         no_llseek,
	.unlocked_ioctl = ar_ioctl,
	.open       =     ar_open,
	.release    =     ar_release,
};

static struct miscdevice ar_miscdev =
{
    .minor =    MISC_DYNAMIC_MINOR,
    .name =     "ar",
    .fops =     &ar_fops,
};
/*lint +e785*/
/*lint -e826*/
static int database_req(const pkt_header_t *head)
{
	unsigned char *data= NULL;
	env_database_head_t *dbase = (env_database_head_t *)(head + 1);
	struct sk_buff *skb;

	if (0 == dbase->len || 1024*1024 < dbase->len) {
		pr_err("hismart:[%s]:line[%d] len[%d] is err\n", __func__, __LINE__, dbase->len);
		return 0;
	}

	mutex_lock(&g_ar_dev.lock);
	if(NULL == g_ar_dev.envdev_priv.env_port) {
		pr_err("hismart:[%s]:line[%d] env_port must init first\n", __func__, __LINE__);
		goto DATABASE_REQ_FIN;
	}

	printk(HISI_AR_DEBUG"hismart:[%s]:line[%d] len[%d]\n", __func__, __LINE__, dbase->len);
	HISI_AR_DEBUG_DUMP_OPT(HISI_AR_DEBUG_DUMP, "dbase: ", DUMP_PREFIX_ADDRESS,
		16, 1, dbase, dbase->len, true);

	skb = context_gnetlink_alloc(g_ar_dev.envdev_priv.env_port, dbase->len, AR_GENL_CMD_ENV_DATABASE, &data);
	if (!data || !skb) {
		pr_err("hismart:[%s]:line[%d] context_gnetlink_alloc ERR\n", __func__, __LINE__);
		goto DATABASE_REQ_FIN;
	}

	memcpy_s(data,dbase->len, dbase, dbase->len);
	HISI_AR_DEBUG_DUMP_OPT(HISI_AR_DEBUG_DUMP, "data: ", DUMP_PREFIX_ADDRESS,
		16, 1, dbase, dbase->len, true);
	context_gnetlink_send(g_ar_dev.envdev_priv.env_port, skb);
DATABASE_REQ_FIN:
	mutex_unlock(&g_ar_dev.lock);
	return 0;
} /*lint !e715*/
/*lint +e826*/

static int __init ar_init(void)
{
	int ret = 0;
	int i;
	envdev_priv_t *envdev_priv = &g_ar_dev.envdev_priv;

	ret = get_contexthub_dts_status();
	if(ret)
		return ret;

	memset_s((void*)&g_ar_dev,sizeof(g_ar_dev), 0, sizeof(g_ar_dev));
	for(i = 0; i< AR_ENVIRONMENT_END; i++) {
		envdev_priv->env_init[i].context = AR_ENVIRONMENT_END;
	}

	ret = genl_register_family(&ar_genl_family);
	if (ret) {
		pr_err("hismart:[%s]:line[%d] genl_register_family err[%d]\n", __func__, __LINE__, ret);
		return ret ;
	}

	ret = misc_register(&ar_miscdev);
	if (ret){
		pr_err("hismart:[%s]:line[%d] cannot register hisi ar ret[%d]\n", __func__, __LINE__, ret);
		goto AR_PROBE_GEN;
	}

	INIT_LIST_HEAD(&g_ar_dev.list);
	mutex_init(&g_ar_dev.lock);
	mutex_init(&g_ar_dev.state_lock);
	init_completion(&g_ar_dev.get_complete);
#ifdef CONFIG_INPUTHUB_20
	register_mcu_event_notifier(TAG_AR, CMD_DATA_REQ, get_ar_data_from_mcu);
	register_mcu_event_notifier(TAG_ENVIRONMENT, SUB_CMD_ENVIRONMENT_DATABASE_REQ, database_req);
	register_mcu_event_notifier(TAG_ENVIRONMENT, CMD_DATA_REQ, get_env_data_from_mcu);
	register_mcu_event_notifier(TAG_AR, SUB_CMD_FLP_AR_SHMEM_STATE_REQ, ar_state_shmem);
#else
	register_mcu_event_notifier(TAG_AR, CMD_FLP_AR_DATA_REQ, get_ar_data_from_mcu);
	register_mcu_event_notifier(TAG_ENVIRONMENT, CMD_ENVIRONMENT_DATA_REQ, get_env_data_from_mcu);
	register_mcu_event_notifier(TAG_ENVIRONMENT, CMD_ENVIRONMENT_DATABASE_REQ, database_req);
	register_mcu_event_notifier(TAG_AR, CMD_FLP_AR_GET_STATE_RESP, ar_state_shmem);
#endif
	register_iom3_recovery_notifier(&ar_reboot_notify);
	return ret;

AR_PROBE_GEN:
	genl_unregister_family(&ar_genl_family);
	return ret;
}

static void ar_exit(void)
{
	int ret = 0;
	misc_deregister(&ar_miscdev);
#ifdef CONFIG_INPUTHUB_20
	ret |= unregister_mcu_event_notifier(TAG_AR, CMD_DATA_REQ, get_ar_data_from_mcu);
	ret |= unregister_mcu_event_notifier(TAG_ENVIRONMENT, SUB_CMD_ENVIRONMENT_DATABASE_REQ, database_req);
	ret |= unregister_mcu_event_notifier(TAG_ENVIRONMENT, CMD_DATA_REQ, get_env_data_from_mcu);
	ret |= unregister_mcu_event_notifier(TAG_AR, SUB_CMD_FLP_AR_SHMEM_STATE_REQ, ar_state_shmem);
#else
	ret |= unregister_mcu_event_notifier(TAG_AR, CMD_FLP_AR_DATA_REQ, get_ar_data_from_mcu);
	ret |= unregister_mcu_event_notifier(TAG_ENVIRONMENT, CMD_ENVIRONMENT_DATA_REQ, get_env_data_from_mcu);
	ret |= unregister_mcu_event_notifier(TAG_ENVIRONMENT, CMD_ENVIRONMENT_DATABASE_REQ, database_req);
	ret |= unregister_mcu_event_notifier(TAG_AR, CMD_FLP_AR_GET_STATE_RESP, ar_state_shmem);
#endif
	if(ret) {
		pr_err("hismart:[%s]:line[%d] ret[%d]err\n", __func__, __LINE__, ret);
	}
	return;
}
/*lint +e64*/

static int hisi_ar_pm_suspend(struct device *dev)
{
	return 0;
}

static int hisi_ar_pm_resume(struct device *dev)
{
	struct list_head    *pos;
	ar_port_t      *ar_port;
	unsigned int cnt = 0;
	mutex_lock(&g_ar_dev.lock);
	pr_info("hismart:[%s]:line[%d] in\n", __func__, __LINE__);
	list_for_each(pos, &g_ar_dev.list) {
		ar_port = container_of(pos, ar_port_t, list);
		if (ar_port->need_awake) {
			ar_port->work_para = AR_GENL_CMD_AWAKE_RET;
			queue_work(system_power_efficient_wq, &ar_port->work);
			cnt++;
		}
	}
	pr_info("hismart:[%s]:line[%d] cnt[%d]out\n", __func__, __LINE__, cnt);
	mutex_unlock(&g_ar_dev.lock);
	return 0;
}
/*lint -e785*/
struct dev_pm_ops hisi_ar_pm_ops = {
#ifdef  CONFIG_PM_SLEEP
    .suspend = hisi_ar_pm_suspend ,
    .resume  = hisi_ar_pm_resume ,
#endif
};

static int generic_ar_probe(struct platform_device *pdev)
{
	return ar_init();
}

static int generic_ar_remove(struct platform_device *pdev)
{
	ar_exit();
	return 0;
}

static const struct of_device_id generic_ar[] = {
    { .compatible = "hisilicon,hismart-ar" },
    {},
};
MODULE_DEVICE_TABLE(of, generic_ar);
/*lint -e64*/
static struct platform_driver generic_ar_platdrv = {
	.driver = {
		.name	= "hisi-ar",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(generic_ar),
		.pm = &hisi_ar_pm_ops,
	},
	.probe = generic_ar_probe,
	.remove  = generic_ar_remove,
};
/*lint +e64*/
/*lint +e785*/
/*lint -e64*/
static int __init hisi_ar_init(void)
{
	return platform_driver_register(&generic_ar_platdrv);
}
/*lint +e64*/
static void __exit hisi_ar_exit(void)
{
	platform_driver_unregister(&generic_ar_platdrv);
}
/*lint -e528 -esym(528,*) */
late_initcall_sync(hisi_ar_init);
module_exit(hisi_ar_exit);


