 /*
  * hisilicon efuse driver, hisi_efuse.c
  *
  * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
  *
  */
#include <asm/compiler.h>
#include <linux/compiler.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/cpumask.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/memory.h>
#include <linux/proc_fs.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/smp.h>
#include <linux/hisi/hisi_efuse.h>
#include <linux/dma-mapping.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <global_ddr_map.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/kconfig.h>
#include "hisi_flash_hisee_otp.h"

#define EFUSE_DEV_NAME                    "efuse"
#define EFUSE_CLOCK_VOLT                  "efuse_volt_hold"

#define EFUSE_GROUP_MAX_STRING            "efuse_mem_attr_group_max"
#define EFUSE_MEM_START_ADDRESS           "hisi,share-memory-efuse"
#define EFUSE_ATTR_STRING                 "hisilicon, efuse"

#define EFUSE_FN_GET_DIEID                (0xc5000001)
#define EFUSE_FN_GET_CHIPID               (0xc5000002)
#define EFUSE_FN_SET_CHIPID               (0xc5000003)
#define EFUSE_FN_GET_AUTHKEY              (0xc5000004)
#define EFUSE_FN_SET_AUTHKEY              (0xc5000005)
#define EFUSE_FN_GET_SECDBG               (0xc5000006)
#define EFUSE_FN_SET_SECDBG               (0xc5000007)
#define EFUSE_FN_GET_THERMAL              (0xc5000008)
/*lint -e750 -esym(750,*)*/
#define EFUSE_FN_TEST_DISPLAY             (0xc5000009)
/*lint -e750 +esym(750,*)*/
#define EFUSE_FN_GET_KCE                  (0xc500000B)
#define EFUSE_FN_SET_KCE                  (0xc500000C)
#define EFUSE_FN_GET_FREQ                 (0xc500000E)


#define EFUSE_FN_SET_HISEE                (0xc5000013)
#define EFUSE_FN_GET_HISEE                (0xc5000014)
#define EFUSE_FN_GET_DESKEW               (0xc5000015)
/* 20 bytes be enough for these efuse field now, need to make
 * larger if the length of efuse field more than 20!
 */
#define EFUSE_BUFFER_MAX_BYTES            (20)

/* define SECURE DEBUG MODE value */
#define ARMDEBUG_CTRL_ENABLE_MODE         (0x0)
#define ARMDEBUG_CTRL_PASSWD_MODE         (0x1)
#define ARMDEBUG_CTRL_CERT_MODE           (0x2)
#define ARMDEBUG_CTRL_EFUSE_MODE          (0x3)

#ifndef ARRAY_LEN
#define ARRAY_LEN(a)                      (sizeof(a) / (sizeof(*a)))
#endif

#define GET_GROUP_NUM(width_bit)          (((width_bit) + 31) / 32)
#define GET_BYTE_NUM(width_bit)           (GET_GROUP_NUM(width_bit) * 4)

#define GET_BITS_WIDTH(attr_id) ({\
	g_efusec.efuse_attrs_from_dts[(u32)attr_id].bits_width;\
})

// cppcheck-suppress *
#define check_efuse_module_ready() \
do { \
	if (g_efusec.is_init_success != EFUSE_MODULE_INIT_SUCCESS) { \
		pr_err("%s: efuse module is not ready now.\n", __func__);\
		return -ENODEV; \
	} \
} while (0)

//CC-suppress
#define EFUSE_CHECK_READ_CHIPID() \
		if (bytes > EFUSE_CHIPID_LENGTH_BYTES){ \
			pr_err("%s:efuse_chipid_length is too large", __func__); \
			ret = -EFAULT; \
			break; \
		}

#define EFUSE_CHECK_AUTHKEY_LENGTH() \
		if (bytes > EFUSE_AUTHKEY_LENGTH_BYTES){ \
			pr_err("%s:efuse_authkey_length is too large", __func__); \
			ret = -EFAULT; \
			break; \
		}

#define EFUSE_CHECK_SECDBG_LENGTH() \
		if (bytes > EFUSE_SECDBG_LENGTH_BYTES) \
		{ \
			pr_err("%s:efuse_secdbg_length is too large", __func__); \
			ret = -EFAULT; \
			break; \
		}

#define EFUSE_CHECK_DIEID_LENGTH() \
		if (bytes > EFUSE_DIEID_LENGTH_BYTES) \
		{ \
			pr_err("%s:efuse_dieid_length is too large", __func__); \
			ret = -EFAULT; \
			break; \
		}

#define EFUSE_CHECK_SLTFINISHFLAG_LENGTH() \
		if (bytes > EFUSE_SLTFINISHFLAG_LENGTH_BYTES) \
		{ \
			pr_err("%s:efuse_sltfinishflag_length is too large", __func__); \
			ret = -EFAULT; \
			break; \
		}

static struct clk *pefuse_clk;
static efusec_data_t g_efusec;

static char *g_efusec_attr[] = {
	"efuse_mem_attr_huk",
	"efuse_mem_attr_scp",
	"efuse_mem_attr_authkey",
	"efuse_mem_attr_chipid",
	"efuse_mem_attr_tsensor_calibration",
	"efuse_mem_attr_huk_rd_disable",
	"efuse_mem_attr_authkey_rd_disable",
	"efuse_mem_attr_dbg_class_ctrl",
	"efuse_mem_attr_dieid",
	"efuse_mem_attr_max"
};

/*
 *Function: vote_efuse_volt --efuse voltage hold at 0.75v when writing
 *Input:    void
 *return:   OK --SUCC,
 *          ERROR --FAIL,
 *          -EFAULT --addr is NULL
 *History:
 *1.    2017.6.15   Creat
 */
static s32 vote_efuse_volt(void)
{
	s32 ret = ERROR;

	if (IS_ERR_OR_NULL(pefuse_clk)) {
		pr_err("%s: pefuse_clk is NULL.\n", __func__);
		return -EFAULT;
	}
	ret = clk_prepare_enable(pefuse_clk);
	if (ret != OK) {
		pr_err("%s: clk_prepare_enable fail.\n", __func__);
		return ERROR;
	}
	return OK;
}

/*
 *Function: restore_efuse_volt --restore peri voltage
 *Input:    void
 *return:   OK --SUCC,
 *          -EFAULT --addr is NULL
 *History:
 *1.    2017.6.15   Creat
 */
static s32 restore_efuse_volt(void)
{
	if (IS_ERR_OR_NULL(pefuse_clk)) {
		pr_err("%s: pefuse_clk is NULL.\n", __func__);
		return -EFAULT;
	}
	clk_disable_unprepare(pefuse_clk);
	return OK;
}

static inline s32 atfd_hisi_service_efusec_smc(u64 _function_id,
					       u64 _arg0,
					       u64 _arg1,
					       u64 _arg2)
{
	register u64 function_id asm("x0") = _function_id;
	register u64 arg0 asm("x1") = _arg0;
	register u64 arg1 asm("x2") = _arg1;
	register u64 arg2 asm("x3") = _arg2;

	asm volatile(
		__asmeq("%0", "x0")
		__asmeq("%1", "x1")
		__asmeq("%2", "x2")
		__asmeq("%3", "x3")
		"smc    #0\n"
		: "+r" (function_id)
		: "r" (arg0), "r" (arg1), "r" (arg2));

	return (int)function_id;
}

s32 get_efuse_hisee_value(unsigned char *buf, u32 size, u32 timeout)
{
	s32 ret;

	if (!buf || (size > EFUSE_HISEE_LENGTH_BYTES)) {
		pr_err("%s: buf is NULL.\n", __func__);
		return -EFAULT;
	}
	check_efuse_module_ready();
	BUG_ON(in_interrupt());

	mutex_lock(&g_efusec.efuse_mutex);
	ret = g_efusec.atf_fn(EFUSE_FN_GET_HISEE, (u64)g_efusec.paddr,
			      size, timeout);
	if (ret == OK)
		memmove((void *)buf, (void *)g_efusec.vaddr, size);

	mutex_unlock(&g_efusec.efuse_mutex);

	pr_info("%s: ret=%d.\n", __func__, ret);
	return ret;
}

s32 set_efuse_hisee_value(unsigned char *buf, u32 size, u32 timeout)
{
	s32 ret;
	s32 vote_efuse_volt_flag = ERROR;

	if (!buf || (size > EFUSE_HISEE_LENGTH_BYTES)) {
		pr_err("%s: buf is NULL.\n", __func__);
		return -EFAULT;
	}
	check_efuse_module_ready();
	BUG_ON(in_interrupt());

	mutex_lock(&g_efusec.efuse_mutex);
	vote_efuse_volt_flag = vote_efuse_volt();
	memmove((void *)g_efusec.vaddr, (void *)buf, size);
	ret = g_efusec.atf_fn(EFUSE_FN_SET_HISEE, (u64)g_efusec.paddr,
			      size, timeout);
	if (vote_efuse_volt_flag == OK)
		restore_efuse_volt();

	mutex_unlock(&g_efusec.efuse_mutex);

	pr_info("%s: ret=%d.\n", __func__, ret);
	return ret;
}

s32 get_efuse_dieid_value(unsigned char *buf, u32 size, u32 timeout)
{
	s32 ret;

	if (!buf || (size > EFUSE_DIEID_LENGTH_BYTES)) {
		pr_err("%s: buf is NULL.\n", __func__);
		return -EFAULT;
	}
	check_efuse_module_ready();
	BUG_ON(in_interrupt());

	mutex_lock(&g_efusec.efuse_mutex);
	ret = g_efusec.atf_fn(EFUSE_FN_GET_DIEID, (u64)g_efusec.paddr,
			     (u64)size, (u64)timeout);
	if (ret == OK)
		memmove((void *)buf, (void *)g_efusec.vaddr, size);

	mutex_unlock(&g_efusec.efuse_mutex);

	pr_info("%s: ret=%d.\n", __func__, ret);
	return ret;
}



/*****************************************************************************
函 数 名  : get_efuse_deskew_value
功能描述  : 获取boston平台1756位efuse的bit值
输入参数  : unsigned char *buf(接收返回值的内存指针)
            u32 size_t(接收返回值的内存大小)
            u32 timeout  		(获取bit值超时时间，单位毫秒，一般为1000)
输出参数  : 无
返 回 值  : s32
			成功    0
			失败其他值
*****************************************************************************/
s32 get_efuse_deskew_value(unsigned char *buf, u32 size, u32 timeout)
{
	s32 ret;

	if ((!buf) ||(size > EFUSE_DESKEW_LENGTH_BYTES)){
		pr_err("%s: buf is NULL.\n", __func__);
		return -EFAULT;
	}
	check_efuse_module_ready();
	BUG_ON(in_interrupt());

	mutex_lock(&g_efusec.efuse_mutex);
	ret = g_efusec.atf_fn(EFUSE_FN_GET_DESKEW, (u64)g_efusec.paddr,
			     (u64)size, (u64)timeout);
	if (ret == OK)
		memmove((void *)buf, (void *)g_efusec.vaddr, size);/* [false alarm]:memmove is safe here */
	mutex_unlock(&g_efusec.efuse_mutex);

	pr_info("%s: ret=%d.\n", __func__, ret);
	return ret;
}

s32 get_efuse_chipid_value(unsigned char *buf, u32 size, u32 timeout)
{
	s32 ret;

	if (!buf || (size > EFUSE_CHIPID_LENGTH_BYTES)) {
		pr_err("%s: buf is NULL.\n", __func__);
		return -EFAULT;
	}
	check_efuse_module_ready();
	BUG_ON(in_interrupt());

	mutex_lock(&g_efusec.efuse_mutex);
	ret = g_efusec.atf_fn(EFUSE_FN_GET_CHIPID, (u64)g_efusec.paddr,
			      (u64)size, (u64)timeout);
	if (ret == OK)
		memmove((void *)buf, (void *)g_efusec.vaddr, size);

	mutex_unlock(&g_efusec.efuse_mutex);

	pr_info("%s: ret=%d.\n", __func__, ret);
	return ret;
}

static s32 set_efuse_chipid_value(unsigned char *buf, u32 size, u32 timeout)
{
	s32 ret;
	s32 vote_efuse_volt_flag = ERROR;

	if (!buf || (size > EFUSE_CHIPID_LENGTH_BYTES)) {
		pr_err("%s: buf is NULL.\n", __func__);
		return -EFAULT;
	}
	check_efuse_module_ready();
	BUG_ON(in_interrupt());

	mutex_lock(&g_efusec.efuse_mutex);
	vote_efuse_volt_flag = vote_efuse_volt();
	memmove((void *)g_efusec.vaddr, (void *)buf, size);
	ret = g_efusec.atf_fn(EFUSE_FN_SET_CHIPID, (u64)g_efusec.paddr,
			      (u64)size, (u64)timeout);
	if (vote_efuse_volt_flag == OK)
		restore_efuse_volt();

	mutex_unlock(&g_efusec.efuse_mutex);

	pr_info("%s: ret=%d.\n", __func__, ret);
	return ret;
}

static s32 get_efuse_authkey_value(unsigned char *buf, u32 size, u32 timeout)
{
	s32 ret;

	if (!buf || (size > EFUSE_AUTHKEY_LENGTH_BYTES)) {
		pr_err("%s: buf is NULL.\n", __func__);
		return -EFAULT;
	}

	check_efuse_module_ready();
	BUG_ON(in_interrupt());

	mutex_lock(&g_efusec.efuse_mutex);
	ret = g_efusec.atf_fn(EFUSE_FN_GET_AUTHKEY, (u64)g_efusec.paddr,
			      (u64)size, (u64)timeout);
	if (ret == OK)
		memmove((void *)buf, (void *)g_efusec.vaddr, size);

	mutex_unlock(&g_efusec.efuse_mutex);

	pr_info("%s: ret=%d.\n", __func__, ret);
	return ret;
}

static s32 set_efuse_authkey_value(unsigned char *buf, u32 size, u32 timeout)
{
	s32 ret;
	s32 vote_efuse_volt_flag = ERROR;

	if (!buf || (size > EFUSE_AUTHKEY_LENGTH_BYTES)) {
		pr_err("%s: buf is NULL.\n", __func__);
		return -EFAULT;
	}
	check_efuse_module_ready();
	BUG_ON(in_interrupt());

	mutex_lock(&g_efusec.efuse_mutex);
	vote_efuse_volt_flag = vote_efuse_volt();
	memmove((void *)g_efusec.vaddr, (void *)buf, size);
	ret = g_efusec.atf_fn(EFUSE_FN_SET_AUTHKEY, (u64)g_efusec.paddr,
			      (u64)size, (u64)timeout);

	if (vote_efuse_volt_flag == OK)
		restore_efuse_volt();

	mutex_unlock(&g_efusec.efuse_mutex);

	pr_info("%s: ret=%d.\n", __func__, ret);
	return ret;
}

static s32 get_efuse_securitydebug_value(unsigned char *buf, u32 size,
					 u32 timeout)
{
	s32 ret;

	if (!buf || (size > EFUSE_SECDBG_LENGTH_BYTES)) {
		pr_err("%s: buf is NULL.\n", __func__);
		return -EFAULT;
	}
	check_efuse_module_ready();
	BUG_ON(in_interrupt());
	mutex_lock(&g_efusec.efuse_mutex);

	ret = g_efusec.atf_fn(EFUSE_FN_GET_SECDBG, (u64)g_efusec.paddr,
			      (u64)size, (u64)timeout);
	if (ret == OK)
		memmove((void *)buf, (void *)g_efusec.vaddr, size);

	mutex_unlock(&g_efusec.efuse_mutex);
	pr_info("%s: ret=%d\n", __func__, ret);

	return ret;
}

/* check secret_mode is disabled mode */
static bool efuse_secdebug_is_disabled(u32 secret_mode)
{
	if (secret_mode == ARMDEBUG_CTRL_ENABLE_MODE ||
	    secret_mode == ARMDEBUG_CTRL_PASSWD_MODE ||
	    secret_mode == ARMDEBUG_CTRL_CERT_MODE ||
	    secret_mode == ARMDEBUG_CTRL_EFUSE_MODE) {
		return true;
	}
	return false;
}

/* check secure debug efuse is disabled */
s32 efuse_check_secdebug_disable(bool *b_disabled)
{
	u32 sec_debug_value;
	s32 ret;

	if (!b_disabled)
		return -EINVAL;

	sec_debug_value = ARMDEBUG_CTRL_ENABLE_MODE;

	ret = get_efuse_securitydebug_value((unsigned char *)&sec_debug_value,
					    sizeof(sec_debug_value),
					    EFUSE_TIMEOUT_1000MS);

	if (ret != OK)
		return ERROR;

	*b_disabled = efuse_secdebug_is_disabled(sec_debug_value);
	return OK;
}

static s32 set_efuse_securitydebug_value(unsigned char *buf,
					 u32 size, u32 timeout)
{
	s32 ret;
	s32 vote_efuse_volt_flag = ERROR;

	if (!buf || (size > EFUSE_SECDBG_LENGTH_BYTES)) {
		pr_err("%s: buf is NULL.\n", __func__);
		return -EFAULT;
	}

	check_efuse_module_ready();
	BUG_ON(in_interrupt());

	mutex_lock(&g_efusec.efuse_mutex);

	vote_efuse_volt_flag = vote_efuse_volt();

	memmove((void *)g_efusec.vaddr, (void *)buf, sizeof(u32));
	ret = g_efusec.atf_fn(EFUSE_FN_SET_SECDBG, (u64)g_efusec.paddr,
			      (u64)size, (u64)timeout);
	if (vote_efuse_volt_flag == OK)
		restore_efuse_volt();

	/* start hisee otp1 write task, if secure debug is disabled */
	if (ret == OK && efuse_secdebug_is_disabled(*(u32 *)buf) == true) {
		pr_err("debug efuse set %x start otp\n", (*(u32 *)buf));
		/* start otp1 flash stask for hisee pinstall task */
		creat_flash_otp_thread();
	}

	mutex_unlock(&g_efusec.efuse_mutex);
	pr_info("%s: ret=%d.\n", __func__, ret);

	return ret;
}

s32 get_efuse_thermal_value(unsigned char *buf, u32 size, u32 timeout)
{
	s32 ret;

	if (!buf || (size > EFUSE_THERMAL_LENGTH_BYTES)) {
		pr_err("%s: buf is NULL.\n", __func__);
		return -EFAULT;
	}
	check_efuse_module_ready();
	BUG_ON(in_interrupt());

	mutex_lock(&g_efusec.efuse_mutex);
	ret = g_efusec.atf_fn(EFUSE_FN_GET_THERMAL, (u64)g_efusec.paddr,
			      (u64)size, (u64)timeout);
	if (ret == OK)
		memmove((void *)buf, (void *)g_efusec.vaddr, size);

	mutex_unlock(&g_efusec.efuse_mutex);

	pr_info("%s: ret=%d.\n", __func__, ret);
	return ret;
}

s32 get_efuse_freq_value(unsigned char *buf, u32 size)
{
	s32 ret;

	if (!buf || (size > EFUSE_FREQ_LENGTH_BYTES)) {
		pr_err("%s: buf is NULL.\n", __func__);
		return -EFAULT;
	}
	check_efuse_module_ready();
	BUG_ON(in_interrupt());

	mutex_lock(&g_efusec.efuse_mutex);
	ret = g_efusec.atf_fn(EFUSE_FN_GET_FREQ, (u64)g_efusec.paddr,
			      sizeof(u32), (u64)0);
	if (ret == OK)
		memmove((void *)buf, (void *)g_efusec.vaddr, size);

	mutex_unlock(&g_efusec.efuse_mutex);

	pr_info("%s: ret=%d.\n", __func__, ret);
	return ret;
}

s32 get_efuse_kce_value(unsigned char *buf, u32 size,
			u32 timeout)
{
	s32 ret = OK;

	check_efuse_module_ready();
	pr_info("%s: ret=%d.\n", __func__, ret);
	return ret;
}

s32 set_efuse_kce_value(unsigned char *buf, u32 size, u32 timeout)
{
	s32 ret;
	s32 vote_efuse_volt_flag = ERROR;

	if (!buf || (size != EFUSE_KCE_LENGTH_BYTES)) {
		pr_err("%s: buf is NULL.\n", __func__);
		return -EFAULT;
	}
	check_efuse_module_ready();
	BUG_ON(in_interrupt());

	mutex_lock(&g_efusec.efuse_mutex);
	vote_efuse_volt_flag = vote_efuse_volt();
	memmove((void *)g_efusec.vaddr, (void *)buf, size);

	ret = g_efusec.atf_fn(EFUSE_FN_SET_KCE, (u64)g_efusec.paddr,
			      (u64)size, (u64)timeout);
	if (vote_efuse_volt_flag == OK)
		restore_efuse_volt();

	mutex_unlock(&g_efusec.efuse_mutex);

	pr_info("%s: ret=%d.\n", __func__, ret);
	return ret;
}


/*
 * Function name:efusec_ioctl.
 * Discription:complement read/write efuse by terms of sending command-words.
 * return value:
 *          @ 0 - success.
 *          @ -1- failure.
 */
static long efusec_ioctl(struct file *file, u32 cmd, unsigned long arg)
{
	s32 ret = OK;
	void __user *argp = (void __user *)arg;
	u32 i;
	u32 bits_width, bytes;

	u32 read_buf[EFUSE_BUFFER_MAX_BYTES / sizeof(u32)] = { 0 };
	unsigned char *buffer = (unsigned char *)read_buf;

	pr_info("%s: cmd=0x%x.\n", __func__, cmd);
	if (!argp) {
		ret = -EFAULT;
		return ret;
	}

	switch (cmd) {
	case HISI_EFUSE_READ_CHIPID:
		bits_width = GET_BITS_WIDTH(efuse_mem_attr_chipid);
		bytes = GET_BYTE_NUM(bits_width);

		ret = get_efuse_chipid_value(&buffer[0], bytes,
					     EFUSE_TIMEOUT_1000MS);
		if (ret != OK) {
			pr_err("%s: get_efuse_chipid_value failed,ret=%d\n",
			       __func__, ret);
			ret = -EFAULT;
			break;
		}

		EFUSE_CHECK_READ_CHIPID();
		if (copy_to_user(argp, &buffer[0], bytes))
			ret = -EFAULT;

		break;

	case HISI_EFUSE_WRITE_CHIPID:
		bits_width = GET_BITS_WIDTH(efuse_mem_attr_chipid);
		bytes = GET_BYTE_NUM(bits_width);
		if (bytes > EFUSE_CHIPID_LENGTH_BYTES) {
			pr_err("%s:efuse_chipid_length is too large", __func__);
			ret = -EFAULT;
			break;
		}

		if (copy_from_user(&buffer[0], argp, bytes)) {
			ret = -EFAULT;
			break;
		}

		ret = set_efuse_chipid_value(&buffer[0], bytes,
					     EFUSE_TIMEOUT_1000MS);
		if (ret != OK) {
			pr_err("%s: efuse_set_chipID,ret=%d\n", __func__, ret);
			ret = -EFAULT;
		}

		break;

	case HISI_EFUSE_READ_CHIPIDLEN:
		bits_width = GET_BITS_WIDTH(efuse_mem_attr_chipid);
		if (copy_to_user(argp, &bits_width, sizeof(bits_width)))
			ret = -EFAULT;

		break;

	case HISI_EFUSE_READ_AUTHKEY:
		bits_width = GET_BITS_WIDTH(efuse_mem_attr_authkey);
		bytes = GET_BYTE_NUM(bits_width);
		ret = get_efuse_authkey_value(&buffer[0], bytes,
					      EFUSE_TIMEOUT_1000MS);
		if (ret != OK) {
			pr_err("%s: get_efuse_authkey_value failed, ret=%d\n",
			       __func__, ret);
			ret = -EFAULT;
			break;
		}

		EFUSE_CHECK_AUTHKEY_LENGTH();
		if (copy_to_user(argp, &buffer[0], bytes))
			ret = -EFAULT;
		break;

	case HISI_EFUSE_WRITE_AUTHKEY:
		bits_width = GET_BITS_WIDTH(efuse_mem_attr_authkey);
		bytes = GET_BYTE_NUM(bits_width);
		if (bytes > EFUSE_AUTHKEY_LENGTH_BYTES) {
			pr_err("%s:efuse_authkey_len is too large", __func__);
			ret = -EFAULT;
			break;
		}

		if (copy_from_user(&buffer[0], argp, bytes)) {
			ret = -EFAULT;
			break;
		}

		ret = set_efuse_authkey_value(&buffer[0], bytes,
					      EFUSE_TIMEOUT_1000MS);
		if (ret != OK) {
			pr_err("%s: set_efuse_authkey_value failed,ret=%d\n",
			       __func__, ret);
			ret = -EFAULT;
		}

		break;

	case HISI_EFUSE_READ_DEBUGMODE:
		bits_width = GET_BITS_WIDTH(efuse_mem_attr_huk_rd_disable);
		bytes = GET_BYTE_NUM(bits_width);
		ret = get_efuse_securitydebug_value(&buffer[0], bytes,
						    EFUSE_TIMEOUT_1000MS);
		if (ret != OK) {
			pr_err("%s: efuse_get_SecureDebugMode failed,ret=%d\n",
			       __func__, ret);
			ret = -EFAULT;
			break;
		}

		EFUSE_CHECK_SECDBG_LENGTH();
		/*send back to user */
		if (copy_to_user(argp, &buffer[0], bytes)) {
			ret = -EFAULT;
			break;
		}

		break;

	case HISI_EFUSE_WRITE_DEBUGMODE:
		bits_width = GET_BITS_WIDTH(efuse_mem_attr_huk_rd_disable);
		bytes = GET_BYTE_NUM(bits_width);
		if (bytes > EFUSE_SECDBG_LENGTH_BYTES) {
			pr_err("%s:efuse_secdbg_length is too large", __func__);
			ret = -EFAULT;
			break;
		}

		/*get data from user */
		if (copy_from_user(&buffer[0], argp, bytes)) {
			ret = -EFAULT;
			break;
		}

		ret = set_efuse_securitydebug_value(&buffer[0], bytes,
						    EFUSE_TIMEOUT_1000MS);
		if (ret != OK) {
			pr_err("%s: efuse_set_SecureDebugMode failed,ret=%d\n",
			       __func__, ret);
			ret = -EFAULT;
		}

		break;

	case HISI_EFUSE_READ_DIEID:
		bits_width = GET_BITS_WIDTH(efuse_mem_attr_dieid);
		bytes = GET_BYTE_NUM(bits_width);
		ret = get_efuse_dieid_value(&buffer[0], bytes,
					    EFUSE_TIMEOUT_1000MS);
		if (ret != OK) {
			pr_err("%s: get_efuse_dieid_value failed,ret=%d\n",
			       __func__, ret);
			ret = -EFAULT;
			break;
		}

		EFUSE_CHECK_DIEID_LENGTH();
		if (copy_to_user(argp, &buffer[0], bytes))
			ret = -EFAULT;

		break;


	default:
		pr_err("[EFUSE][%s] Unknown command!\n", __func__);
		ret = -ENOTTY;
		break;
	}

	return ret;
}

static const struct file_operations efusec_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = efusec_ioctl,
};

static s32 efuse_get_atrr_from_dts(void)
{
	u32 i = 0;
	s32 ret = -EFAULT;
	u32 bit_width = 0;
	struct device_node *np = NULL;

	np = of_find_compatible_node(NULL, NULL, EFUSE_ATTR_STRING);
	if (!np) {
		pr_err("%s: No efusec compatible node found.\n", __func__);
		ret = -ENODEV;
		goto error0;
	}

	ret = of_property_read_u32(np, EFUSE_GROUP_MAX_STRING,
				   &g_efusec.efuse_group_max);
	if (ret != OK) {
		pr_err("%s %s in efusec compatible node\n",
		       EFUSE_GROUP_MAX_STRING,
		       (-EINVAL == ret) ? "not exist" : "has invalid value");
		goto error0;
	}

	/* currently, parse the dts configurations follow the strategy:
	 * 1.Assume the values in correspondings attribution is correct
	 *   both for Balong and K3V3+ platform, so the values in DTS
	 *   must be filled correctly;
	 * 2.Assume to parse the attributions one by one according to
	 *   g_efusec_attr[] until one does not exist, treat it as failure,
	 *   so these attributions contained in g_efusec_attr[] must be
	 *   configured in dts;
	 * 3.if the value are 0xffffffff, indicate the attribution is
	 *   reserved for future use, it's process depend on software.
	 *   A example is the dieid attribution.
	 */

	/* ARRAY_LEN(g_efusec_attr) - 1 : the -1 is for  efuse_mem_attr_max */
	for (i = 0; i < ARRAY_LEN(g_efusec_attr) - 1; i++) {
		ret = of_property_read_u32(np, g_efusec_attr[i], &bit_width);
		if (ret != OK) {
			pr_err("%s %s in efusec compatible node\n",
			       g_efusec_attr[i],
			       (-EINVAL == ret) ?
			       "does not exist" : "has invalid value");
			goto error0;
		}

		if (i < ARRAY_LEN(g_efusec.efuse_attrs_from_dts)) {
			g_efusec.efuse_attrs_from_dts[i].bits_width = bit_width;
		} else {
			pr_err("%s: i = 0x%x is out-of-bound\n", __func__, i);
			ret =  -EFAULT;
			goto error0;
		}

		pr_info("%d: %s in efusec compatible node value::%d\n",
			i, g_efusec_attr[i], bit_width);
	}

	pr_info("%s success\n", __func__);

error0:
	return ret;
}

/* get share memory between kernel and atf */
static s32 efuse_get_share_mem_from_dts(void)
{
	s32 ret = -EFAULT;
	struct device_node *np = NULL;
	u32 data[2] = { 0 };
	phys_addr_t shm_base = HISI_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE;

	np = of_find_compatible_node(NULL, NULL, EFUSE_MEM_START_ADDRESS);
	if (!np) {
		pr_err("%s: No share-memory-efuse compatible node found.\n",
		       __func__);
		goto error0;
	}

	ret = of_property_read_u32_array(np, "reg", &data[0], 2);
	if (ret != OK) {
		pr_err("reg %s in share-memory-efuse compatible node.\n",
		       (-EINVAL == ret) ? "not exist" : "has invalid value");
		goto error0;
	}

	/* data[0] is the offset of physical address
	 * data[1] is the size of share memory
	 */
	g_efusec.paddr = shm_base + data[0];
	if (data[1] < EFUSE_BUFFER_MAX_BYTES) {
		ret = -EFAULT;
		pr_err("%s: share memory is too small.\n", __func__);
		goto error0;
	}

	g_efusec.vaddr = (unsigned char *)ioremap(shm_base + data[0],
						  data[1]);
	if (!g_efusec.vaddr) {
		ret = -EFAULT;
		pr_err("%s: memory for g_efusec.vaddr failed.\n", __func__);
		goto error0;
	}

	pr_info("g_efusec.paddr=0x%lx, g_efusec.vaddr=0x%lx ",
		(unsigned long)g_efusec.paddr, (unsigned long)g_efusec.vaddr);

	pr_info("%s success\n", __func__);
error0:
	return ret;
}

static void efuse_set_atf_invoke(void)
{
	g_efusec.atf_fn = atfd_hisi_service_efusec_smc;
}

static s32 efuse_create_device_node(void)
{
	s32 ret = OK;
	s32 major = 0;
	struct class *pclass = NULL;
	struct device *pdevice = NULL;

	major = register_chrdev(0, EFUSE_DEV_NAME, &efusec_fops);
	if (major <= 0) {
		ret = -EFAULT;
		pr_err("%s: unable to get major.\n", __func__);
		goto error0;
	}

	pclass = class_create(THIS_MODULE, EFUSE_DEV_NAME);
	if (IS_ERR(pclass)) {
		ret = -EFAULT;
		pr_err("%s: class_create error.\n", __func__);
		goto error1;
	}

	pdevice = device_create(pclass, NULL, MKDEV(major, 0),
				NULL, EFUSE_DEV_NAME);
	if (IS_ERR(pdevice)) {
		ret = -EFAULT;
		pr_err("%s: device_create error.\n", __func__);
		goto error2;
	}

	pefuse_clk = devm_clk_get(pdevice, EFUSE_CLOCK_VOLT);
	if (IS_ERR_OR_NULL(pefuse_clk))
		pr_err("%s: devm_clk_get is NULL.\n", __func__);

	pr_info("%s success\n", __func__);
	return ret;

error2:
	class_destroy(pclass);
error1:
	unregister_chrdev(major, EFUSE_DEV_NAME);
error0:
	return ret;
}

static s32 __init hisi_efusec_init(void)
{
	s32 ret = 0;

	memset(&g_efusec, 0, sizeof(g_efusec));

	ret = efuse_get_atrr_from_dts();
	if (ret != OK)
		return ret;

	ret = efuse_get_share_mem_from_dts();
	if (ret != OK)
		return ret;

	efuse_set_atf_invoke();

	ret = efuse_create_device_node();
	if (ret != OK)
		return ret;

	mutex_init(&g_efusec.efuse_mutex);
	creat_flash_otp_init();

	g_efusec.is_init_success = EFUSE_MODULE_INIT_SUCCESS;

	pr_info("%s success\n", __func__);
	return ret;
}

rootfs_initcall(hisi_efusec_init);

MODULE_DESCRIPTION("Hisilicon efuse module");
MODULE_AUTHOR("lvtaolong@huawei.com.sh");
MODULE_LICENSE("GPL");
