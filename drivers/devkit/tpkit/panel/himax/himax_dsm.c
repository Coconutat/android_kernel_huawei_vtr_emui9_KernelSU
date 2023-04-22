/*
 *add dsm log
 */
#include "himax_ic.h"
#include <dsm/dsm_pub.h>
#include <linux/gpio.h>

#ifdef CONFIG_HUAWEI_DSM


ssize_t hmx_dsm_record_basic_err_info( void )
{
	ssize_t size = 0;
	ssize_t total_size = 0;

	/* record irq and reset gpio status */
	TS_LOG_DEBUG("%s: record irq and reset gpio!\n", __func__);
	size = dsm_client_record(hmx_tp_dclient,
				"[irq gpio]   num:%d, irq gpio status:%d, [reset gpio] num:%d, reset gpio status:%d\n",
				hmx_tp_dsm_info.irq_gpio, gpio_get_value(hmx_tp_dsm_info.irq_gpio),
				hmx_tp_dsm_info.rst_gpio, gpio_get_value(hmx_tp_dsm_info.rst_gpio));
	total_size += size;

	TS_LOG_ERR("[irq gpio]   num:%d, irq gpio status:%d, [reset gpio] num:%d, reset gpio status:%d\n",
				hmx_tp_dsm_info.irq_gpio, gpio_get_value(hmx_tp_dsm_info.irq_gpio),
				hmx_tp_dsm_info.rst_gpio, gpio_get_value(hmx_tp_dsm_info.rst_gpio));

	return total_size;
}

/* report error infomation */
ssize_t hmx_dsm_record_common_err_info(char * err_info)
{

	ssize_t size = 0;
	ssize_t total_size = 0;
	if(NULL == err_info) {
		return total_size;
	}
	TS_LOG_ERR("%s: entry!\n", __func__);

	/* err number */
	size = dsm_client_record(hmx_tp_dclient, "%s\n", err_info);
	total_size += size;

	return total_size;
}

/* i2c error infomation: err number, register infomation */
ssize_t hmx_dsm_record_i2c_err_info( int err_numb )
{
	ssize_t size = 0;
	ssize_t total_size = 0;

	TS_LOG_ERR("%s: entry!\n", __func__);

	/* err number */
	size = dsm_client_record(hmx_tp_dclient, "i2c err number:%d\n", err_numb );
	total_size += size;

	return total_size;
}
/* fw err infomation: err number */
ssize_t hmx_dsm_record_fw_err_info( int err_numb )
{
	ssize_t size = 0;
	ssize_t total_size = 0;

	TS_LOG_ERR("%s: entry!\n", __func__);

	/* err number */
	size = dsm_client_record(hmx_tp_dclient, "fw related fail, retval is %d\n", err_numb);
	total_size += size;

	/* fw err status */
	size = dsm_client_record(hmx_tp_dclient, "fw err status is %d\n", hmx_tp_dsm_info.UPDATE_status);
	total_size += size;

	return total_size;
}

ssize_t hmx_dsm_record_esd_err_info( int err_numb )
{

	ssize_t size = 0;
	ssize_t total_size = 0;

	TS_LOG_INFO("%s: entry!\n", __func__);

	/* err number */
	size = dsm_client_record(hmx_tp_dclient, "esd err number:%d\n", err_numb );
	total_size += size;

	return total_size;
}

ssize_t hmx_dsm_record_rawdata_err_info( int err_numb )
{
	ssize_t size = 0;
	ssize_t total_size = 0;
	TS_LOG_INFO("%s: entry!\n", __func__);
	size = dsm_client_record(hmx_tp_dclient, "rawdata error is:%d\n", err_numb );
	total_size += size;
	return total_size;
}
/* tp report err according to err type */
int hmx_tp_report_dsm_err(int type, int err_numb)
{
	TS_LOG_ERR("%s: entry! type:%d\n", __func__, type);

	if( NULL == hmx_tp_dclient )
	{
		TS_LOG_ERR("%s: there is not tp_dclient!\n", __func__);
		return TP_DCLIENT_NO_EXIST;
	}

	/* try to get permission to use the buffer */
	if(dsm_client_ocuppy(hmx_tp_dclient))
	{
		/* buffer is busy */
		TS_LOG_ERR("%s: buffer is busy!\n", __func__);
		return BUFFER_BUSY;
	}

	/* tp report err according to err type */
	switch(type)
	{
		case DSM_TP_I2C_RW_ERROR_NO:
			/* report tp basic infomation */
			hmx_dsm_record_basic_err_info();
			/* report i2c infomation */
			hmx_dsm_record_i2c_err_info(err_numb);
			break;
		case DSM_TP_FWUPDATE_ERROR_NO:
			/* report tp basic infomation */
			hmx_dsm_record_basic_err_info();
			/* report fw infomation */
			hmx_dsm_record_fw_err_info(err_numb);
			break;
		case DSM_TP_RAWDATA_ERROR_NO:
			/* report tp basic infomation */
			hmx_dsm_record_basic_err_info();
			/* report error infomation */
			hmx_dsm_record_common_err_info("work error");
			break;
		case DSM_TP_ESD_ERROR_NO:
			/* report tp basic infomation */
			hmx_dsm_record_basic_err_info();
			/* report esd infomation */
			hmx_dsm_record_esd_err_info(err_numb);
			break;

		default:
			break;
	}
	dsm_client_notify(hmx_tp_dclient, type);

	return NO_ERR;
}
#endif/*CONFIG_HUAWEI_DSM*/

