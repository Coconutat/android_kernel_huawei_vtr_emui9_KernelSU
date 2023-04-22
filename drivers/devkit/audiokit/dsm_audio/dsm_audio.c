#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/string.h>
#include "dsm_audio.h"

static struct dsm_dev dsm_audio = {
	.name = DSM_AUDIO_NAME,
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = DSM_AUDIO_BUF_SIZE,
};

static struct dsm_dev dsm_smartpa = {
	.name = DSM_SMARTPA_NAME,
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = DSM_SMARTPA_BUF_SIZE,
};

static struct dsm_dev dsm_anc_hs = {
	.name = DSM_ANC_HS_NAME,
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = DSM_ANC_HS_BUF_SIZE,
};

struct dsm_audio_client {
	struct dsm_client * dsm_client;
	char * dsm_str_info_buffer;
};

//lint -save -e578 -e605  -e715 -e528 -e753
static struct dsm_audio_client *dsm_audio_client_table = NULL;
static struct dsm_dev *dsm_dev_table[AUDIO_DEVICE_MAX] = {&dsm_audio, &dsm_smartpa, &dsm_anc_hs};

static int audio_dsm_register(struct dsm_audio_client *dsm_client, struct dsm_dev *dsm_audio)
{
	if(NULL == dsm_audio || NULL == dsm_client )
		return -EINVAL;

	dsm_client->dsm_client = dsm_register_client(dsm_audio);
	if(NULL == dsm_client->dsm_client) {
		dsm_client->dsm_client = dsm_find_client((char *)dsm_audio->name);
		if(NULL == dsm_client->dsm_client) {
			dsm_loge("dsm_audio_client register failed!\n");
			return -ENOMEM;
		} else {
			dsm_loge("dsm_audio_client find in dsm_server\n");
		}
	}

    dsm_client->dsm_str_info_buffer = kzalloc(dsm_audio->buff_size, GFP_KERNEL);
    if (!dsm_client->dsm_str_info_buffer) {
        dsm_loge("dsm audio %s malloc buffer failed!\n", dsm_audio->name);
        return -ENOMEM;
    }

	return 0;
}
//lint -restore
static int audio_dsm_init(void)
{
#ifdef CONFIG_HUAWEI_DSM
	int i = 0;
	int ret = 0;


	if (!dsm_audio_client_table) {
		dsm_audio_client_table = kzalloc(sizeof(struct dsm_audio_client) * AUDIO_DEVICE_MAX, GFP_KERNEL);
		if (!dsm_audio_client_table) {
			dsm_loge("dsm_audio_client table malloc failed!\n");
			return -ENOMEM;
		}
	}

	for(i = 0; i < AUDIO_DEVICE_MAX; i++) {
		if (NULL != dsm_audio_client_table[i].dsm_client || NULL == dsm_dev_table[i]) {
			continue;
		}
		ret = audio_dsm_register(dsm_audio_client_table + i, dsm_dev_table[i]);
		if(ret) {
			dsm_loge("dsm dev %s register failed %d\n",dsm_dev_table[i]->name, ret);
		}
	}
#endif
	return 0;
}
static void audio_dsm_deinit(void)
{
#ifdef CONFIG_HUAWEI_DSM
	int i;
    if (!dsm_audio_client_table)
        return;
	for (i = 0; i < AUDIO_DEVICE_MAX; i++) {
		if (dsm_audio_client_table[i].dsm_str_info_buffer) {
			kfree(dsm_audio_client_table[i].dsm_str_info_buffer);
			dsm_audio_client_table[i].dsm_str_info_buffer = NULL;
		}
	}
    kfree(dsm_audio_client_table);
    dsm_audio_client_table = NULL;
#endif
}

//lint -save -e578 -e605  -e715 -e528 -e753
int audio_dsm_report_num(enum audio_device_type dev_type, int error_no, unsigned int mesg_no)
{
#ifdef CONFIG_HUAWEI_DSM
	int err = 0;

	if(NULL == dsm_audio_client_table[dev_type].dsm_client) {
		dsm_loge("dsm_audio_client did not register!\n");
		return -EINVAL;
	}

	err = dsm_client_ocuppy(dsm_audio_client_table[dev_type].dsm_client);
	if(0 != err) {
		dsm_loge("user buffer is busy!\n");
		return -EBUSY;
	}

	dsm_logi("report error_no=0x%x, mesg_no=0x%x!\n",
			error_no, mesg_no);
	dsm_client_record(dsm_audio_client_table[dev_type].dsm_client, "Message code = 0x%x.\n", mesg_no);
	dsm_client_notify(dsm_audio_client_table[dev_type].dsm_client, error_no);
#endif
	return 0;
}

int audio_dsm_report_info(enum audio_device_type dev_type, int error_no, char *fmt, ...)
{
	int ret = 0;
#ifdef CONFIG_HUAWEI_DSM
	int err = 0;
	va_list args;

    dsm_logi("begin,errorno %d,dev_type %d ",error_no, dev_type);
	if(NULL == dsm_audio_client_table[dev_type].dsm_client) {
		dsm_loge("dsm_audio_client did not register!\n");
		ret = -EINVAL;
		goto out;
	}

	va_start(args, fmt);
	ret = vsnprintf(dsm_audio_client_table[dev_type].dsm_str_info_buffer, dsm_dev_table[dev_type]->buff_size, fmt, args);
	va_end(args);
    dsm_logi("begin,errorno %d,dev_type %d ",error_no, dev_type);

	err = dsm_client_ocuppy(dsm_audio_client_table[dev_type].dsm_client);
	if(0 != err) {
		dsm_loge("user buffer is busy!\n");
		ret = -EBUSY;
		goto out;
	}

	dsm_logi("report dsm_error_no = %d, %s\n",
			error_no, dsm_audio_client_table[dev_type].dsm_str_info_buffer);
	dsm_client_record(dsm_audio_client_table[dev_type].dsm_client, "%s\n", dsm_audio_client_table[dev_type].dsm_str_info_buffer);
	dsm_client_notify(dsm_audio_client_table[dev_type].dsm_client, error_no);
out:
#endif
	return ret;
}

EXPORT_SYMBOL(audio_dsm_report_num);
EXPORT_SYMBOL(audio_dsm_report_info);

subsys_initcall_sync(audio_dsm_init);
module_exit(audio_dsm_deinit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Huawei dsm audio");
MODULE_AUTHOR("<penghongxing@huawei.com>");
//lint -restore
