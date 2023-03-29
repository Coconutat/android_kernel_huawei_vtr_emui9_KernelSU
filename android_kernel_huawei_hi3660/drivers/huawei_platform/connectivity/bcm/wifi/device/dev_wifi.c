/*
 * Copyright (c) 2012-2013 Huawei Ltd.
 *
 * Author: 
 *
 * This	program	is free	software; you can redistribute it and/or modify
 * it under	the	terms of the GNU General Public	License	version	2 as
 * published by	the	Free Software Foundation.
 */

/* dev_wifi.c
 */

/*=========================================================================
 *
 * histoty
 *
 *=========================================================================
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/err.h>
#include <linux/random.h>
#include <linux/skbuff.h>
#include <linux/wlan_plat.h>
#include <linux/gpio.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/ctype.h>
#include <linux/export.h>
#include <linux/mmc/dw_mmc.h>
#include <linux/module.h>
#ifdef CONFIG_HWCONNECTIVITY
#include <huawei_platform/connectivity/hw_connectivity.h>
#endif
#include <huawei_platform/log/hw_log.h>

#include "dev_wifi.h"
#include "hw_driver_register.h"
#include <linux/mtd/hisi_nve_interface.h>


#define HW_FAC_GPIO_VAL_LOW     20700
#define HW_FAC_GPIO_VAL_HIG     20701

#define DHD_ERROR_VAL   0x0001

#define DTS_COMP_WIFI_POWER_NAME "hisilicon,bcm_wifi"

#define WIFI_VCC_NAME "wifi_vccio"
static struct regulator *wifi_vccio = NULL;

static struct WIFI_VCC_CMD_S vcc_get_cmd = {WIFI_REGU_GET, &wifi_vccio, 0, 0, 0};
static struct WIFI_VCC_CMD_S vcc_put_cmd = {WIFI_REGU_PUT, &wifi_vccio, 0, 0, 0};
static struct WIFI_VCC_CMD_S vcc_enable_cmd = {WIFI_REGU_ENABLE, &wifi_vccio, 0, 0, 10};
static struct WIFI_VCC_CMD_S vcc_disable_cmd = {WIFI_REGU_DISABLE, &wifi_vccio, 0, 0, 10};
static struct WIFI_VCC_CMD_S vcc_setvol_cmd = {WIFI_REGU_SET_VOLTAGE, &wifi_vccio, 1800000, 1800000, 0};

#define HWLOG_TAG       dev_wifi

struct wifi_host_s *wifi_host;
struct dev_wifi_handle dev_handle;

#if defined(HW_WIFI_FACTORY_MODE)
struct clk *g_pmic_clk = NULL;
int init_wifi_pmic_clock(struct platform_device *pdev);
#endif

#ifdef CONFIG_DHD_USE_STATIC_BUF
void *wlan_static_prot = NULL;
void *wlan_static_scan_buf0 = NULL;
void *wlan_static_scan_buf1 = NULL;
void *wlan_static_dhd_info_buf = NULL;
void *wlan_static_if_flow_lkup = NULL;
void *wlan_static_osl_buf = NULL;
void *wlan_static_dhd_prealloc_pktid_map = NULL;
#ifdef BCM_ALLOC_STATIC_10K
void *wlan_static_if_flow_tbl = NULL;
#endif
static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];
unsigned char g_wifimac[WLAN_MAC_LEN] = {0x00,0x00,0x00,0x00,0x00,0x00};
static wifi_mem_prealloc_t wifi_mem_array[PREALLOC_WLAN_SEC_NUM] = {
	{ NULL, (WLAN_SECTION_SIZE_1 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_2 + PREALLOC_WLAN_SECTION_HEADER) }
};
#endif /* CONFIG_DHD_USE_STATIC_BUF */

HWLOG_REGIST();

#ifdef CONFIG_DHD_USE_STATIC_BUF

MODULE_LICENSE("GPL v2");

void register_dev_wifi_handle (struct dev_wifi_handle org) {
    memcpy(&dev_handle, &org, sizeof(struct dev_wifi_handle));
}
void unregister_dev_wifi_handle(void) {
    memset(&dev_handle, 0, sizeof(struct dev_wifi_handle));
}
EXPORT_SYMBOL(register_dev_wifi_handle);
EXPORT_SYMBOL(unregister_dev_wifi_handle);
void *wifi_mem_prealloc(int section, unsigned long size) {
    int mem_idx = 0;
    if (section == DHD_PREALLOC_PROT) {
        if (size > DHD_PREALLOC_PROT_SIZE) {
            printk("bcmdhd request prot(%lu) is bigger than static size(%ld).\n",
                size, DHD_PREALLOC_PROT_SIZE);
            return NULL;
        }
        return wlan_static_prot;
    }

    if (section == DHD_PREALLOC_SKB_BUF)
        return wlan_static_skb;

    if (section == DHD_PREALLOC_WIPHY_ESCAN0)
        return wlan_static_scan_buf0;

    if (section == DHD_PREALLOC_WIPHY_ESCAN1)
        return wlan_static_scan_buf1;

    if (section == DHD_PREALLOC_OSL_BUF) {
        if (size > DHD_PREALLOC_OSL_BUF_SIZE) {
            hwlog_err("request OSL_BUF(%lu) is bigger than static size(%ld).\n",
                size, DHD_PREALLOC_OSL_BUF_SIZE);
            return NULL;
        }
        return wlan_static_osl_buf;
    }

    if (section == DHD_PREALLOC_DHD_INFO) {
        if (size > DHD_PREALLOC_DHD_INFO_SIZE) {
            hwlog_err("request DHD_INFO size(%lu) is bigger than static size(%d).\n",
                size, DHD_PREALLOC_DHD_INFO_SIZE);
            return NULL;
        }
        return wlan_static_dhd_info_buf;
    }
    if (section == DHD_PREALLOC_IF_FLOW_LKUP)  {
        if (size > DHD_PREALLOC_IF_FLOW_LKUP_SIZE) {
            hwlog_err("request DHD_IF_FLOW_LKUP size(%lu) is bigger than static size(%d).\n",
                size, DHD_PREALLOC_IF_FLOW_LKUP_SIZE);
            return NULL;
        }

        return wlan_static_if_flow_lkup;
    }
#ifdef CONFIG_BCMDHD_PCIE
	if (section == DHD_PREALLOC_PKTID_MAP) {
		if (size > WLAN_DHD_PREALLOC_PKTID_MAP) {
			hwlog_err("request DHD_PREALLOC_PKTID_MAP size(%lu) is bigger"
				" than static size(%d).\n",
				size, WLAN_DHD_PREALLOC_PKTID_MAP);
			return NULL;
		}
		hwlog_err("request DHD_PREALLOC_PKTID_MAP size(%lu) ok, static size(%d).\n",
			size, WLAN_DHD_PREALLOC_PKTID_MAP);
		return wlan_static_dhd_prealloc_pktid_map;
	}
#ifdef BCM_ALLOC_STATIC_10K
	if (section == DHD_PREALLOC_IF_FLOW_TBL)  {
		if (size > DHD_PREALLOC_IF_FLOW_TBL_SIZE) {
			hwlog_err("request DHD_IF_FLOW_TBL size(%lu) > static size(%d).\n",
				size, DHD_PREALLOC_IF_FLOW_TBL_SIZE);
			return NULL;
		}
		hwlog_err("request DHD_IF_FLOW_TBL size(%lu) ok, static size(%d).\n",
			size, DHD_PREALLOC_IF_FLOW_TBL_SIZE);
		return wlan_static_if_flow_tbl;
	}
#endif
#endif
    if (section > DHD_PREALLOC_PROT && section <= (DHD_PREALLOC_PROT + PREALLOC_WLAN_SEC_NUM)) {
        mem_idx = section - DHD_PREALLOC_PROT - 1;
        if (wifi_mem_array[mem_idx].size < size) {
            hwlog_err("section:%d prealloc is not enough(size:%lu).\n", section, size);
            return NULL;
        }
        return wifi_mem_array[mem_idx].mem_ptr;
    }

    if ((section < 0) || (section > DHD_PREALLOC_MAX))
        hwlog_err("request section id(%d) is out of max index %d\n",
                section, DHD_PREALLOC_MAX);

    return NULL;
}

static int init_wifi_mem(void) {
    int i;
    int j;

    hwlog_info("init_wifi_mem.\n");
    for (i = 0; i < DHD_SKB_1PAGE_BUF_NUM; i++) {
        wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_1PAGE_BUFSIZE);
        if (!wlan_static_skb[i]) {
            goto err_skb_alloc;
        }
    }

    for (i = DHD_SKB_1PAGE_BUF_NUM; i < WLAN_SKB_1_2PAGE_BUF_NUM; i++) {
        wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_2PAGE_BUFSIZE);
        if (!wlan_static_skb[i]) {
            goto err_skb_alloc;
        }
    }

#if !defined(CONFIG_BCMDHD_PCIE)
    wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_4PAGE_BUFSIZE);
    if (!wlan_static_skb[i]) {
        goto err_skb_alloc;
    }
#endif /* !CONFIG_BCMDHD_PCIE */

    wlan_static_prot = kmalloc(DHD_PREALLOC_PROT_SIZE, GFP_KERNEL);
    if (!wlan_static_prot) {
        hwlog_err("Failed to alloc wlan_static_prot\n");
        goto err_mem_alloc;
    }

    wlan_static_osl_buf = kmalloc(DHD_PREALLOC_OSL_BUF_SIZE, GFP_KERNEL);
    if (!wlan_static_osl_buf) {
        hwlog_err("Failed to alloc wlan_static_osl_buf\n");
        goto err_mem_alloc;
    }

    wlan_static_scan_buf0 = kmalloc(DHD_PREALLOC_WIPHY_ESCAN0_SIZE, GFP_KERNEL);
    if (!wlan_static_scan_buf0) {
        hwlog_err("Failed to alloc wlan_static_scan_buf0\n");
        goto err_mem_alloc;
    }

    wlan_static_dhd_info_buf = kmalloc(DHD_PREALLOC_DHD_INFO_SIZE, GFP_KERNEL);
    if (!wlan_static_dhd_info_buf) {
        hwlog_err("Failed to alloc wlan_static_dhd_info_buf\n");
        goto err_mem_alloc;
    }

#ifdef CONFIG_BCMDHD_PCIE
    wlan_static_if_flow_lkup = kmalloc(DHD_PREALLOC_IF_FLOW_LKUP_SIZE, GFP_KERNEL);
    if (!wlan_static_if_flow_lkup) {
        hwlog_err("Failed to alloc wlan_static_if_flow_lkup\n");
        goto err_mem_alloc;
    }

    wlan_static_dhd_prealloc_pktid_map = kmalloc(WLAN_DHD_PREALLOC_PKTID_MAP, GFP_KERNEL);
    if (!wlan_static_dhd_prealloc_pktid_map) {
        hwlog_err("[WLAN] Failed to alloc wlan_static_dhd_prealloc_pktid_map\n");
            goto err_mem_alloc;
    }
#ifdef BCM_ALLOC_STATIC_10K
    wlan_static_if_flow_tbl = kmalloc(DHD_PREALLOC_IF_FLOW_TBL_SIZE, GFP_KERNEL);
    if (!wlan_static_if_flow_tbl) {
	hwlog_err("Failed to alloc wlan_static_if_flow_tbl\n");
	goto err_mem_alloc;
    }
#endif
#endif /* CONFIG_BCMDHD_PCIE */

#if !defined(CONFIG_BCMDHD_PCIE)
    for (i = 0; i < PREALLOC_WLAN_SEC_NUM; i++) {
        wifi_mem_array[i].mem_ptr = kzalloc(wifi_mem_array[i].size,
            GFP_KERNEL);
        if (wifi_mem_array[i].mem_ptr == NULL) {
            hwlog_err("%s: alloc mem_ptr is error(%d).\n", __func__, i);
            goto err_sec_alloc;
        }
    }
#endif /* !defined(CONFIG_BCMDHD_PCIE) */
    return 0;

err_sec_alloc:
    for (i = 0; i < PREALLOC_WLAN_SEC_NUM; i++) {
        if (wifi_mem_array[i].mem_ptr != NULL) {
            kfree(wifi_mem_array[i].mem_ptr);
            wifi_mem_array[i].mem_ptr = NULL;
        } else {
            break;
        }
    }

err_mem_alloc:

    if (wlan_static_prot)
        kfree(wlan_static_prot);

    if (wlan_static_dhd_info_buf)
        kfree(wlan_static_dhd_info_buf);

    if (wlan_static_scan_buf1)
        kfree(wlan_static_scan_buf1);

    if (wlan_static_scan_buf0)
        kfree(wlan_static_scan_buf0);

    if (wlan_static_osl_buf)
        kfree(wlan_static_osl_buf);

#ifdef CONFIG_BCMDHD_PCIE
    if (wlan_static_if_flow_lkup)
        kfree(wlan_static_if_flow_lkup);
    if (wlan_static_dhd_prealloc_pktid_map)
        kfree(wlan_static_dhd_prealloc_pktid_map);
#ifdef BCM_ALLOC_STATIC_10K
	if (wlan_static_if_flow_tbl)
		kfree(wlan_static_if_flow_tbl);
#endif
#endif
    hwlog_err("Failed to mem_alloc for WLAN\n");

    i = WLAN_SKB_BUF_NUM;

err_skb_alloc:
    hwlog_err("Failed to skb_alloc for WLAN\n");
    for (j = 0; j < i; j++) {
        dev_kfree_skb(wlan_static_skb[j]);
    }

    return -ENOMEM;
}

/*deinit wifi buf*/
int deinit_wifi_mem(void) {
#ifdef CONFIG_DHD_USE_STATIC_BUF
    int i;

    hwlog_info("deinit_wifi_mem.\n");
    for (i = 0; i < DHD_SKB_1PAGE_BUF_NUM; i++) {
        if (wlan_static_skb[i])
            dev_kfree_skb(wlan_static_skb[i]);
    }

    for (i = DHD_SKB_1PAGE_BUF_NUM; i < WLAN_SKB_1_2PAGE_BUF_NUM; i++) {
        if (wlan_static_skb[i])
            dev_kfree_skb(wlan_static_skb[i]);
    }

#if !defined(CONFIG_BCMDHD_PCIE)
    if (wlan_static_skb[i])
        dev_kfree_skb(wlan_static_skb[i]);
#endif /* !CONFIG_BCMDHD_PCIE */

    if (wlan_static_prot)
        kfree(wlan_static_prot);

    if (wlan_static_osl_buf)
        kfree(wlan_static_osl_buf);

    if (wlan_static_scan_buf0)
        kfree(wlan_static_scan_buf0);

    if (wlan_static_dhd_info_buf)
        kfree(wlan_static_dhd_info_buf);

    if (wlan_static_scan_buf1)
        kfree(wlan_static_scan_buf1);

#ifdef CONFIG_BCMDHD_PCIE
    if (wlan_static_if_flow_lkup)
        kfree(wlan_static_if_flow_lkup);
    if (wlan_static_dhd_prealloc_pktid_map)
        kfree(wlan_static_dhd_prealloc_pktid_map);
#ifdef BCM_ALLOC_STATIC_10K
    if (wlan_static_if_flow_tbl)
		kfree(wlan_static_if_flow_tbl);
#endif
#endif

    for (i = 0; i < PREALLOC_WLAN_SEC_NUM; i++) {
        if (wifi_mem_array[i].mem_ptr != NULL) {
            kfree(wifi_mem_array[i].mem_ptr);
            wifi_mem_array[i].mem_ptr = NULL;
        }
    }
#endif /* CONFIG_BCMDHD_USE_STATIC_BUF */
    return 0;
}

#endif /* CONFIG_DHD_USE_STATIC_BUF */

static void read_from_global_buf(unsigned char * buf)
{
	memcpy(buf,g_wifimac,WLAN_MAC_LEN);
	hwlog_info("get MAC from g_wifimac: mac=%02x:%02x:**:**:%02x:%02x\n",buf[0],buf[1],buf[4],buf[5]);
	return;
}

static int char2_byte( char* strori, char* outbuf )
{
	int i = 0;
	int temp = 0;
	int sum = 0;
	char *ptemp;
	char tempchar[20]={0};

	ptemp = tempchar;

	for (i = 0; i< WLAN_VALID_SIZE;i++){
		if(strori[i]!=':'){
			*ptemp = strori[i];
			 ptemp++;
		}
	}

	for( i = 0; i < NV_WLAN_VALID_SIZE; i++ ){

		switch (tempchar[i]) {
			case '0' ... '9':
				temp = tempchar[i] - '0';
				break;
			case 'a' ... 'f':
				temp = tempchar[i] - 'a' + 10;
				break;
			case 'A' ... 'F':
				temp = tempchar[i] - 'A' + 10;
				break;
			default:
				return 0;
		}
		sum += temp;
		if( i % 2 == 0 ){
			outbuf[i/2] |= temp << 4;
		}
		else{
			outbuf[i/2] |= temp;
		}
	}
	return sum;
}


static int read_from_mac_file(unsigned char * buf)
{
	struct file* filp = NULL;
	mm_segment_t old_fs;
	int result = 0;
	int sum = 0;
	char buf1[20] = {0};
	char buf2[6] = {0};

	if (NULL == buf) {
		hwlog_err("%s: buf is NULL\n", __func__);
		return -1;
	}

	filp = filp_open(MAC_ADDRESS_FILE, O_RDONLY,0);
	if(IS_ERR(filp)){
		hwlog_err("open mac address file error\n");
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	filp->f_pos = 0;
	result = vfs_read(filp,buf1,WLAN_VALID_SIZE,&filp->f_pos);
	if(WLAN_VALID_SIZE != result){
		hwlog_err("read mac address file error\n");
		set_fs(old_fs);
		filp_close(filp,NULL);
		return -1;
	}

	sum = char2_byte(buf1,buf2);
	if (0 != sum){
		hwlog_err("get MAC from file: mac=%02x:%02x:**:**:%02x:%02x\n",buf2[0],buf2[1],buf2[4],buf2[5]);
		memcpy(buf,buf2,WLAN_MAC_LEN);
	}else{
		set_fs(old_fs);
		filp_close(filp,NULL);
		return -1;
	}

	set_fs(old_fs);
	filp_close(filp,NULL);

	return 0;
}

int char2byte( char* strori, char* outbuf )
{
    int i = 0;
    int temp = 0;
    int sum = 0;

    for( i = 0; i < 12; i++ )
    {
        switch (strori[i]) {
            case '0' ... '9':
                temp = strori[i] - '0';
                break;

            case 'a' ... 'f':
                temp = strori[i] - 'a' + 10;
                break;

            case 'A' ... 'F':
                temp = strori[i] - 'A' + 10;
                break;
        }

        sum += temp;
        if( i % 2 == 0 ){
            outbuf[i/2] |= temp << 4;
        }
        else{
            outbuf[i/2] |= temp;
        }
    }

    return sum;
}


/*****************************************************************************
 函 数 名  : bcm_wifi_get_mac_addr
 功能描述  : 从nvram中获取mac地址
             如果获取失败，则随机一个mac地址
 返 回 值  : 0成功，-1失败

*****************************************************************************/
int bcm_wifi_get_mac_addr(unsigned char *buf)
{
    struct bcm_nve_info_user st_info;
    int l_ret = -1;
    int l_sum = 0;

    if (NULL == buf) {
        hwlog_err("%s: k3v2_wifi_get_mac_addr failed\n", __func__);
        return -1;
    }
    memset(buf, 0, WLAN_MAC_LEN);


    memset(&st_info, 0, sizeof(st_info));
    st_info.nv_number  = NV_WLAN_NUM;   //nve item

    strncpy(st_info.nv_name, "MACWLAN", sizeof("MACWLAN"));

    st_info.valid_size = NV_WLAN_VALID_SIZE;
    st_info.nv_operation = NV_READ;

    if (0 != g_wifimac[0] || 0 != g_wifimac[1] || 0 != g_wifimac[2] || 0 != g_wifimac[3]|| 0 != g_wifimac[4] || 0 != g_wifimac[5]){
        read_from_global_buf(buf);
        return 0;
    }

    l_ret = hisi_nve_direct_access(&st_info);

    if (!l_ret)
    {
        l_sum = char2byte(st_info.nv_data, buf);
        if (0 != l_sum)
        {
            hwlog_info("get MAC from NV: mac=%02x:%02x:%02x:%02x:%02x:%02x\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
            memcpy(g_wifimac, buf, WLAN_MAC_LEN);
        }else{
            get_random_bytes(buf,WLAN_MAC_LEN);
            buf[0] = 0x0;
            hwlog_info("get MAC from Random: mac=%02x:%02x:%02x:%02x:%02x:%02x\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
            memcpy(g_wifimac,buf,WLAN_MAC_LEN);

        }
    }else{
        get_random_bytes(buf,WLAN_MAC_LEN);
        buf[0] = 0x0;
        hwlog_info("get MAC from Random: mac=%02x:%02x:%02x:%02x:%02x:%02x\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
        memcpy(g_wifimac,buf,WLAN_MAC_LEN);

    }

    return 0;
}


int bcm_wifi_power(int on)
{
	int ret = 0;
	hwlog_info("%s: on:%d\n", __func__, on);
	if (wifi_host->chip_fpga) {
		if (NULL == wifi_host) {
			hwlog_err("%s: wifi_host is null\n", __func__);
			return -1;
		}
	}

	if (on) {

		if (wifi_host->bEnable) {
			hwlog_err("%s: wifi had power on.\n", __func__);
			return ret;
		}
#if defined(CONFIG_BCMDHD_SDIO) && (defined(CONFIG_MMC_DW_HI6XXX) || defined(CONFIG_MMC_DW_HI3XXX))
        /* set LowerPower mode*/
        if (!(IS_ERR(wifi_host->pctrl)||IS_ERR(wifi_host->pins_idle))){
            ret = pinctrl_select_state(wifi_host->pctrl, wifi_host->pins_idle);
            if(ret)
                hwlog_err("%s: set LOWPOWER mode failed, ret:%d\n", __func__, ret);
        }
#endif
		ret = clk_prepare_enable(wifi_host->clk);

		if (ret < 0) {
		    hwlog_err("%s: clk_enable failed, ret:%d\n", __func__, ret);
		    return ret;
		}
           
		gpio_set_value(wifi_host->enable, 0);
		msleep(10);
		gpio_set_value(wifi_host->enable, 1);
		msleep(100);
#if defined(CONFIG_BCMDHD_SDIO) && (defined(CONFIG_MMC_DW_HI6XXX) || defined(CONFIG_MMC_DW_HI3XXX))
        /* set Normal mode*/
        if (!(IS_ERR(wifi_host->pctrl)||IS_ERR(wifi_host->pins_normal))){
            ret = pinctrl_select_state(wifi_host->pctrl, wifi_host->pins_normal);
            if(ret) hwlog_err("%s: set NORMAL mode failed, ret:%d\n", __func__, ret);
        }
#endif
		wifi_host->bEnable = true;
		//hi_sdio_set_power(on); 
	} else {
		//hi_sdio_set_power(on);
		//dump_stack();
#if defined(CONFIG_BCMDHD_SDIO) && (defined(CONFIG_MMC_DW_HI6XXX) || defined(CONFIG_MMC_DW_HI3XXX))
		/* set LowerPower mode*/
		if (!(IS_ERR(wifi_host->pctrl)||IS_ERR(wifi_host->pins_idle))){
            pinctrl_select_state(wifi_host->pctrl, wifi_host->pins_idle);
            if(ret) hwlog_err("%s: set LOWPOWER mode failed, ret:%d\n", __func__, ret);
        }

        msleep(10);
#endif
		gpio_set_value(wifi_host->enable, 0);
		msleep(10);
		#if 1
			clk_disable_unprepare(wifi_host->clk);
		#endif
			wifi_host->bEnable = false;

	}
	
	return ret;
}

static int bcm_wifi_reset(int on)
{
	hwlog_info("%s: on:%d.\n", __func__, on);
	if (on) {
		gpio_set_value(wifi_host->enable, 1);
	}
	else {
		gpio_set_value(wifi_host->enable, 0);
	}
	return 0;
}

static int bcm_wifi_get_nvram_path(char *val, int len)
{
    hwlog_info("%s: len:%d, val=%s.\n", __func__, len, val);
    if ( val == NULL || len <= 0 ) {
        hwlog_err("%s:val is null or len is invalid.\n", __func__);
        return -1;
    }

    strncpy(val, wifi_host->nvram_path, len);
    return 0;
}

#ifdef HW_WIFI_DRIVER_NORMALIZE
static int bcm_wifi_get_fw_path(char *val, int len)
{
    hwlog_info("%s: len:%d, val=%s.\n", __func__, len, val);
    if ( val == NULL || len <= 0 ) {
        hwlog_err("%s:val is null or len is invalid.\n", __func__);
        return -1;
    }

    strncpy(val, wifi_host->fw_path, len);
    return 0;
}

static int bcm_wifi_get_chip_type(char *val, int len)
{
    hwlog_info("%s: len:%d, val=%s.\n", __func__, len, val);
    if ( val == NULL || len <= 0 ) {
        hwlog_err("%s:val is null or len is invalid.\n", __func__);
        return -1;
    }

    strncpy(val, wifi_host->chip_type, len);
    return 0;
}
#endif /* HW_WIFI_DRIVER_NORMALIZE */

#ifdef HW_CUSTOM_BCN_TIMEOUT
static int bcm_wifi_get_bcn_timeout(void)
{
    // hwlog_info("%s, bcn_timeout: %d\n", __func__, wifi_host->bcn_timeout);
    return wifi_host ? wifi_host->bcn_timeout : -1;
}
#endif /* HW_CUSTOM_BCN_TIMEOUT */

#ifdef CONFIG_MMC_DW_HI3XXX
extern void dw_mci_sdio_card_detect_change(void);
#endif

int hi_sdio_detectcard_to_core(int val)
{
#ifdef CONFIG_MMC_DW_HI3XXX
    dw_mci_sdio_card_detect_change();
#endif
    hwlog_err("dw_mci_sdio_card_detect %d", val);

    return 0;
}
int dhd_msg_level = DHD_ERROR_VAL;
EXPORT_SYMBOL(dhd_msg_level);

static ssize_t show_wifi_debug_level(struct device *dev,
        struct device_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", dhd_msg_level);
}

static ssize_t restore_wifi_debug_level(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t size) {
    int value;
    if (sscanf(buf, "%d\n", &value) == 1) {
#ifdef WL_CFG80211
        hwlog_err("restore_wifi_debug_level\n");
		/* Enable DHD and WL logs in oneshot */
		if (value & DHD_WL_VAL2)
			wl_cfg80211_enable_trace(TRUE, value & (~DHD_WL_VAL2));
		else if (value & DHD_WL_VAL)
			wl_cfg80211_enable_trace(FALSE, WL_DBG_DBG);
		if (!(value & DHD_WL_VAL2))
#endif /* WL_CFG80211 */
        dhd_msg_level = value;
        return size;
    }
    return -1;
}

static ssize_t show_wifi_wrong_action_flag(struct device *dev,
        struct device_attribute *attr, char *buf) {
    if(dev_handle.wl_get_wrong_action_flag_handle == NULL) {
        hwlog_err("%s: handle has not been registered\n", __func__);
        return -1;
    }
    int has_wrong_action = dev_handle.wl_get_wrong_action_flag_handle();
    hwlog_info("%s has wrong action %d\n", __func__, has_wrong_action);
    return sprintf(buf, "%d\n", has_wrong_action);
}

static ssize_t restore_wifi_arp_timeout(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t size) {
    int value;
    if (sscanf(buf, "%d\n", &value) == 1) {
        if(value == 1) {
            hwlog_info("%s enter should invoke wrong action handler\n", __func__);
            if(dev_handle.wl_trigger_disable_nmode_handle == NULL) {
                hwlog_err("%s: handle has not been registered\n", __func__);
                return -1;
            }
                dev_handle.wl_trigger_disable_nmode_handle();
        }
        return size;
    }
    return -1;
}

static ssize_t restore_wifi_wrong_action_debug(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t size) {
    int value;
    if (sscanf(buf, "%d\n", &value) == 1) {
        if(value == 1) {
            hwlog_info("%s enter should invoke wrong action handler\n", __func__);
            if(dev_handle.wl_trigger_disable_nmode_handle == NULL) {
                hwlog_err("handle has not been registered\n");
                return -1;
            }
            dev_handle.wl_trigger_disable_nmode_handle();
        }
        return size;
    }
    return -1;
}

static DEVICE_ATTR(wifi_debug_level, S_IRUGO | S_IWUSR | S_IWGRP,
        show_wifi_debug_level, restore_wifi_debug_level);

static DEVICE_ATTR(wifi_wrong_action_flag, S_IRUGO,
        show_wifi_wrong_action_flag, NULL);

static DEVICE_ATTR(wifi_arp_timeout, S_IWUSR | S_IWGRP,
        NULL, restore_wifi_arp_timeout);

static DEVICE_ATTR(wifi_wrong_action_debug, S_IWUSR | S_IWGRP,
        NULL, restore_wifi_wrong_action_debug);

static struct attribute *attr_debug_attributes[] = {
    &dev_attr_wifi_debug_level.attr,
    NULL
};

static struct attribute *attr_arp_attributes[] = {
    &dev_attr_wifi_wrong_action_flag.attr,
    &dev_attr_wifi_arp_timeout.attr,
    &dev_attr_wifi_wrong_action_debug.attr,
    NULL
};

static const struct attribute_group attrgroup_debug_level = {
    .attrs = attr_debug_attributes,
};

static const struct attribute_group attrgroup_arp_timeout = {
	.attrs = attr_arp_attributes,
};

static int wifi_open_err_code = 0;
static ssize_t show_wifi_open_state(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", wifi_open_err_code);
}
static ssize_t restore_wifi_open_state(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	int value;
	if (sscanf(buf, "%d\n", &value) == 1) {
		wifi_open_err_code = value;
		return size;
	}
	return -1;
}

static ssize_t restore_wifi_factory_dbg(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t size) {
    int value = 0;
    int ret = -1;
    if (NULL == wifi_host) {
        hwlog_err("restore_wifi_factory_dbg: wifi_host is null\n");
        return ret;
    }

    if (sscanf(buf, "%d\n", &value) == 1) {
        if (HW_FAC_GPIO_VAL_LOW == value) {
            ret = bcm_wifi_power(0);
        } else if (HW_FAC_GPIO_VAL_HIG == value) {
            ret = bcm_wifi_power(1);
        } else {
            hwlog_err("restore_wifi_factory_dbg: value: %d\n", value);
        }
    }
    return ((ret >= 0) ? size: -1);
}

void set_wifi_open_err_code(int err_num)
{
    wifi_open_err_code = err_num;
}
EXPORT_SYMBOL(set_wifi_open_err_code);
static DEVICE_ATTR(wifi_open_state, S_IRUGO | S_IWUSR,
				   show_wifi_open_state, restore_wifi_open_state);

static DEVICE_ATTR(wifi_factory_dbg, S_IWUSR,
        NULL, restore_wifi_factory_dbg);

static struct attribute *wifi_state_attributes[] = {
	&dev_attr_wifi_open_state.attr,
    &dev_attr_wifi_factory_dbg.attr,
	NULL
};
static const struct attribute_group wifi_state = {
	.attrs = wifi_state_attributes,
};

struct wifi_platform_data bcm_wifi_control = {
	.set_power = bcm_wifi_power,
	.set_reset = bcm_wifi_reset,
#ifdef CONFIG_BCMDHD_SDIO
	.set_carddetect = hi_sdio_detectcard_to_core,
#endif
	.get_mac_addr = bcm_wifi_get_mac_addr,
	.mem_prealloc = wifi_mem_prealloc,
	.get_nvram_path = bcm_wifi_get_nvram_path,
#ifdef HW_WIFI_DRIVER_NORMALIZE
	.get_fw_path = bcm_wifi_get_fw_path,
	.get_chip_type = bcm_wifi_get_chip_type,
#endif /* HW_WIFI_DRIVER_NORMALIZE */
#ifdef HW_CUSTOM_BCN_TIMEOUT
	.get_bcn_timeout = bcm_wifi_get_bcn_timeout,
#endif /* HW_CUSTOM_BCN_TIMEOUT */
};

static struct resource bcm_wifi_resources[] = {
	[0] = {
	.name  = "bcmdhd_wlan_irq",
#if defined(CONFIG_BCMDHD_PCIE)
	.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE
			| IRQF_NO_SUSPEND,
#else
	.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL
                        | IRQF_NO_SUSPEND,
#endif
	},
};

static struct platform_device bcm_wifi_device = {
	.name = "bcmdhd_wlan",
	.id = 1,
	.num_resources = ARRAY_SIZE(bcm_wifi_resources),
	.resource = bcm_wifi_resources,
	.dev = {
		.platform_data = &bcm_wifi_control,
	},
};

int do_vcc_cmd(struct platform_device *pdev, struct WIFI_VCC_CMD_S *cm) {
	int ret = -1;

	if (cm == NULL) {
		hwlog_err("cm is null!\n");
		return ret;
	}

	if (cm->optype == WIFI_REGU_GET) {
		BUG_ON(pdev == NULL);

		*(cm->regulator) = devm_regulator_get(&pdev->dev, WIFI_VCC_NAME);
		if (IS_ERR(*(cm->regulator))) {
			goto error;
		}
	} else if (cm->optype == WIFI_REGU_PUT) {
		if (!IS_ERR(*(cm->regulator))) {
			devm_regulator_put(*(cm->regulator));
		}
	} else if (cm->optype == WIFI_REGU_ENABLE) {
		if (!IS_ERR(*(cm->regulator))) {
			if (regulator_enable(*(cm->regulator)) != 0) {
				goto error;
			}
		}
	} else if (cm->optype == WIFI_REGU_DISABLE) {
		if (!IS_ERR(*(cm->regulator))) {
			if (regulator_disable(*(cm->regulator)) != 0) {
				goto error;
			}
		}
	} else if (cm->optype == WIFI_REGU_SET_VOLTAGE) {
		if (!IS_ERR(*(cm->regulator))) {
			if (regulator_set_voltage(*(cm->regulator), cm->min_uv, cm->max_uv) != 0) {
				goto error;
			}
		}
	} else {
		hwlog_err("unsupported optype=%x\n", cm->optype);
		goto error;
	}

	if (cm->waitms > 0) {
		mdelay(cm->waitms);
	}

	return 0;

error:
	hwlog_err("failed to operate %x to %s regulator!\n", cm->optype, WIFI_VCC_NAME);
	return ret;
}


int do_wifi_vio(struct platform_device *pdev) {
	int vio_enable = 0;
	int ret = 0;
	bool ext_gpio_type = 0;
	hwlog_err("%s: not udp board, set vio.\n", __func__);
	vio_enable = of_get_named_gpio(pdev->dev.of_node, "wlan_enable,gpio_vio", 0);
	if (vio_enable < 0) {
		hwlog_err("%s: get vio_enable (%d) failed.\n", __func__, vio_enable);
		return -1;
	}
	ext_gpio_type = of_property_read_bool(pdev->dev.of_node, "ext_type");
	ret = gpio_request(vio_enable, NULL);
	if (ret < 0) {
		hwlog_err("%s: vio gpio_request failed, ret:%d.\n", __func__, ret);
		return -1;
	}

	gpio_direction_output(vio_enable, 1);
	if(!ext_gpio_type) {
		ret = gpio_get_value(vio_enable);
	} else {
		ret = gpio_get_value_cansleep(vio_enable);
	}
	if(ret <= 0) {
		hwlog_err("%s: gpio_get_value, vio_enable, ret:%d, pull on.\n", __func__, ret);
		if(!ext_gpio_type) {
			gpio_set_value(vio_enable, 1);
		} else {
			gpio_set_value_cansleep(vio_enable, 1);
		}
	} else {
		hwlog_err("%s: gpio_get_value, vio already on.\n", __func__);
	}
	gpio_free(vio_enable);
	return 0;
}

int do_wifi_vcc_enable(struct platform_device *pdev) {
	int ret = do_vcc_cmd(pdev, &vcc_get_cmd);
	if (ret) return ret;
	hwlog_err("%s enter\n", __func__);

	ret = do_vcc_cmd(pdev, &vcc_setvol_cmd);
	if (ret) {
		goto error;
	} else {
		ret = do_vcc_cmd(pdev, &vcc_enable_cmd);
		if (ret) {
			goto error;
		}
	}
	return 0;
error:
	do_vcc_cmd(pdev, &vcc_put_cmd);
	return ret;
}

int do_wifi_vcc_disable(struct platform_device *pdev) {
	int ret = do_vcc_cmd(pdev, &vcc_disable_cmd);
	ret |= do_vcc_cmd(pdev, &vcc_put_cmd);
	return ret;
}

int enable_wifi_supply (struct platform_device *pdev) {
	int ret = 0;
	u32 supplytype = 0;
	/* udp board doesn't have vio */
	bool udp = of_property_read_bool(pdev->dev.of_node,"udp_board");
	if (udp) {
		hwlog_err("%s: udp board doesn't have vio, do nothing.\n", __func__);
		return 0;
	}
	ret = of_property_read_u32(pdev->dev.of_node, "wifi_supply_type", &supplytype);
	if (ret < 0) {
		return do_wifi_vio(pdev);
	} else if (supplytype == 0) {
		hwlog_err("%s: IO power no need to set\n", __func__);
		return 0;
	} else if (supplytype == 1) {
		return do_wifi_vcc_enable(pdev);
	} else {
		return do_wifi_vio(pdev);
	}
}
#ifdef DHD_DEVWAKE_EARLY
static int gpio_wl_dev_wake = -1;
int dhd_wlan_dev_wake(int on)
{
	if(gpio_wl_dev_wake < 0) {
		hwlog_info("%s Enter: dev_wake is not support\n", __func__);
		return -EIO;
	}
	//pr_info("%s Enter: dev_wake %s\n", __func__, on ? "on" : "off");

	if (on) {
		if (gpio_direction_output(gpio_wl_dev_wake, 1)) {
			hwlog_err("%s: WL_DEV_WAKE didn't output high\n", __func__);
			return -EIO;
		}
		if (gpio_get_value(gpio_wl_dev_wake) == 0)
			hwlog_err("[%s] gpio didn't set high.\n", __func__);
	} else {
		if (gpio_direction_output(gpio_wl_dev_wake, 0)) {
			hwlog_err("%s: WL_DEV_WAKE didn't output low\n", __func__);
			return -EIO;
		}
		if (gpio_get_value(gpio_wl_dev_wake) != 0)
			hwlog_err("[%s] gpio didn't set low.\n", __func__);
	}

	return 0;

}
int devwake_gpio_is_support(void)
{
	return gpio_wl_dev_wake;
}
int dhd_wlan_dev_wake_value(void)
{
	if(gpio_wl_dev_wake < 0)
		return -EIO;
	else
		return gpio_get_value(gpio_wl_dev_wake);
}
EXPORT_SYMBOL(dhd_wlan_dev_wake_value);
EXPORT_SYMBOL(devwake_gpio_is_support);
EXPORT_SYMBOL(dhd_wlan_dev_wake);
#endif
#ifdef HW_PCIE_NOCLOG
int get_enable_gpio(void)
{
	if (NULL == wifi_host) {
		hwlog_err("%s: wifi_host is null\n", __func__);
		return -1;
	}
	return gpio_get_value(wifi_host->enable);
}
EXPORT_SYMBOL(get_enable_gpio);
#endif
#ifdef CONFIG_HW_ABS
bool g_abs_enabled;
EXPORT_SYMBOL(g_abs_enabled);
#endif
int  wifi_power_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np;
#if defined(CONFIG_BCM4343)
	enum of_gpio_flags gpio_flags;
#endif
	const char *nvpath = NULL;
#ifdef HW_WIFI_DRIVER_NORMALIZE
	const char *ictype = NULL;
	const char *fw = NULL;
#endif /* HW_WIFI_DRIVER_NORMALIZE */
#ifdef HW_CUSTOM_BCN_TIMEOUT
	u32 bcn_timeout = 0;
#endif /* HW_CUSTOM_BCN_TIMEOUT */

	memset(&dev_handle, 0, sizeof(struct dev_wifi_handle));

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_WIFI_POWER_NAME);	// should be the same as dts node compatible property
	if (np == NULL) {
		hwlog_err("Unable to find wifi_power\n");
		return -ENOENT;
	}
#ifdef CONFIG_DHD_USE_STATIC_BUF
	ret = init_wifi_mem();
	if (ret) {
		hwlog_err("%s: init_wifi_mem failed.\n", __func__);
		goto err_malloc_wifi_host;
	}
#endif

	wifi_host = kzalloc(sizeof(struct wifi_host_s), GFP_KERNEL);
	if (!wifi_host) {
		hwlog_err("%s: malloc wifi_host failed.\n", __func__);
		ret = -ENOMEM;
		goto err_malloc_wifi_host;
	}

	memset((void*)wifi_host, 0, sizeof(struct wifi_host_s));
	wifi_host->bEnable = false;

#if defined(CONFIG_BCM4343)
	wifi_host->clk = devm_clk_get(&pdev->dev, "CK32B");
#else
	wifi_host->clk = devm_clk_get(&pdev->dev, "clk_pmu32kb");
#endif
	if (IS_ERR(wifi_host->clk)) {
		dev_err(&pdev->dev, "Get wifi 32k clk failed\n");
		ret = -1;
		goto err_clk_get;
       }

#if defined(CONFIG_BCMDHD_SDIO) && (defined(CONFIG_MMC_DW_HI6XXX) || defined(CONFIG_MMC_DW_HI3XXX))
	wifi_host->pctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(wifi_host->pctrl))
	{
		hwlog_err("%s: iomux_lookup_block failed, and the value of wifi_host->pctrl is %lx\n", __func__,(unsigned long)wifi_host->pctrl);
		ret = -1;
		goto err_pinctrl_get;
	}

	wifi_host->pins_normal = pinctrl_lookup_state(wifi_host->pctrl, "default");
	if (IS_ERR(wifi_host->pins_normal))
	{
		hwlog_err("%s: pinctrl_lookup_state default failed, and the value of wifi_host->pins_normal is%lx\n", __func__,(unsigned long)wifi_host->pins_normal);
		ret = -1;
		goto err_pinctrl_get;
	}

	wifi_host->pins_idle = pinctrl_lookup_state(wifi_host->pctrl, "idle");
	if (IS_ERR(wifi_host->pins_idle))
	{
		hwlog_err("%s: pinctrl_lookup_state idle failed, and the value of wifi_host->pins_idle is %lx\n", __func__,(unsigned long)wifi_host->pins_idle);
		ret = -1;
		goto err_pinctrl_get;
	}

	ret = pinctrl_select_state(wifi_host->pctrl, wifi_host->pins_normal);
	if (ret <0) {
		hwlog_err("%s: pinctrl_select_state failed.\n", __func__);
		goto err_pinctrl_get;
	}
#endif

#if defined(CONFIG_BCM4343)
	wifi_host->enable = of_get_gpio_by_prop(np , "wlan-on,gpio-enable" , 0 , 0 , &gpio_flags);
	if (wifi_host->enable<0)
	{
		ret = -1;
		goto err_gpio_get;
	}

	wifi_host->wifi_wakeup_irq= of_get_gpio_by_prop(np , "wlan-irq,gpio-irq" , 0 , 0 , &gpio_flags);
	if (wifi_host->wifi_wakeup_irq<0)
	{
		ret = -1;
		goto err_gpio_get;
	}
#else
	wifi_host->enable = of_get_named_gpio(pdev->dev.of_node, "wlan-on,gpio-enable", 0);
	hwlog_err("%s: wifi_host->enable %d\n", __func__, wifi_host->enable);
	if (wifi_host->enable<0) {
		ret = -1;
		goto err_gpio_get;
	}
        //Register wlan IRQ
	wifi_host->wifi_wakeup_irq= of_get_named_gpio(pdev->dev.of_node, "wlan-irq,gpio-irq", 0);

	hwlog_err("%s: wifi_host->wifi_wakeup_irq %d\n", __func__, wifi_host->wifi_wakeup_irq);
	if (wifi_host->wifi_wakeup_irq<0) {
		ret = -1;
		goto err_gpio_get;
	}

	ret = enable_wifi_supply(pdev);
	if (ret < 0) {
		hwlog_err("%s: enable_wifi_supply failed, ret:%d.\n", __func__, ret);
		goto err_gpio_get;
	}
#ifdef DHD_DEVWAKE_EARLY
	wifi_host->dev_wake = of_get_named_gpio(pdev->dev.of_node, "wlan_wake,gpio_wake", 0);
	if (wifi_host->dev_wake >= 0) {
	   hwlog_err("%s: wl_dev_wake:%d.\n", __FUNCTION__, wifi_host->dev_wake);
	   gpio_wl_dev_wake = wifi_host->dev_wake;
	   ret = gpio_request_one(wifi_host->dev_wake,
	         GPIOF_OUT_INIT_HIGH, "wlan_wake,gpio_wake");
	   if (ret < 0) {
	      hwlog_err("%s: failed to configure output direction for GPIO %d err %d\n",
	                           __FUNCTION__, wifi_host->dev_wake, ret);
	   }
	} else {
	      hwlog_err("%s: wlan_wake,gpio_wake %d is not configured\n", __FUNCTION__, wifi_host->dev_wake);
	}
#endif
#endif

    /* set power gpio */
	ret = gpio_request(wifi_host->enable, "wifi_en_gpio");
	if (ret < 0) {
		hwlog_err("%s: gpio_request failed, ret:%d.\n", __func__,
			wifi_host->enable);
		goto err_enable_gpio_request;
	}
	gpio_direction_output(wifi_host->enable, 0);

	/* set apwake gpio */
	ret = gpio_request(wifi_host->wifi_wakeup_irq, "wifi_irq_gpio");
	if (ret < 0) {
		hwlog_err("%s: gpio_request failed, ret:%d.\n", __func__,
			wifi_host->wifi_wakeup_irq);
		goto err_irq_gpio_request;
	}
	gpio_direction_input(wifi_host->wifi_wakeup_irq);

	bcm_wifi_resources[0].start = gpio_to_irq(wifi_host->wifi_wakeup_irq);
	bcm_wifi_resources[0].end = gpio_to_irq(wifi_host->wifi_wakeup_irq) ;

	memset(wifi_host->nvram_path, 0x0, sizeof(wifi_host->nvram_path));
	ret = of_property_read_string(pdev->dev.of_node, "wifi_nvram_name", &nvpath );
	if ( ret < 0 || nvpath == NULL) {
		hwlog_err("%s: get nvrame_name failed, ret:%d.\n", __func__, ret);
	}
	else {
		hwlog_err("%s:  nvrame_name :%s.\n", __func__, nvpath);
		strncpy(wifi_host->nvram_path, nvpath, sizeof(wifi_host->nvram_path)-1);
	}
#ifdef HW_WIFI_DRIVER_NORMALIZE
    memset(wifi_host->fw_path, 0x0, sizeof(wifi_host->fw_path));
	ret = of_property_read_string(pdev->dev.of_node, "wifi_fw_name", &fw );
	if ( ret < 0 || fw == NULL) {
		hwlog_err("%s: get fw path, ret:%d.\n", __func__, ret);
	}
	else {
		hwlog_err("%s:  fw path :%s.\n", __func__, fw);
		strncpy(wifi_host->fw_path, fw, sizeof(wifi_host->fw_path)-1);
	}
	
	
    memset(wifi_host->chip_type, 0x0, sizeof(wifi_host->chip_type));
	ret = of_property_read_string(pdev->dev.of_node, "ic_type", &ictype );
	if ( ret < 0 || ictype == NULL) {
		hwlog_err("%s: get wlan_chip_type, ret:%d.\n", __func__, ret);
	}
	else {
		hwlog_err("%s:  wlan_chip_type :%s.\n", __func__, ictype);
		strncpy(wifi_host->chip_type, ictype, sizeof(wifi_host->chip_type)-1);
	}
#endif /* HW_WIFI_DRIVER_NORMALIZE */

#ifdef HW_CUSTOM_BCN_TIMEOUT
	ret = of_property_read_u32(pdev->dev.of_node, "bcn_timeout", &bcn_timeout);
	if (ret < 0) {
		hwlog_err("%s: get bcn_timeout failed, ret:%d.\n", __func__, ret);
		wifi_host->bcn_timeout = -1;
	} else {
		wifi_host->bcn_timeout = (int)bcn_timeout;
		hwlog_info("%s: get bcn_timeout, value:%d.\n", __func__, wifi_host->bcn_timeout);
	}
#endif /* HW_CUSTOM_BCN_TIMEOUT */

	ret = platform_device_register(&bcm_wifi_device);
	if (ret) {
		hwlog_err("%s: platform_device_register failed, ret:%d.\n",
			__func__, ret);
		goto err_platform_device_register;
	}
	ret = sysfs_create_group(&bcm_wifi_device.dev.kobj, &attrgroup_debug_level);
	if (ret) {
		hwlog_err("wifi_power_probe create debug level error ret =%d", ret);
	}

	ret = sysfs_create_group(&bcm_wifi_device.dev.kobj, &attrgroup_arp_timeout);
	if (ret) {
		hwlog_err("wifi_power_probe create arp trigger error ret =%d", ret);
	}

	ret = sysfs_create_group(&bcm_wifi_device.dev.kobj, &wifi_state);
	if (ret) {
		hwlog_err("wifi_power_probe sysfs_create_group error ret =%d", ret);
	}
#ifdef CONFIG_HW_ABS
	g_abs_enabled = of_property_read_bool(pdev->dev.of_node,"abs_enabled");
	hwlog_err("%s: abs_enabled: %d\n", __func__, g_abs_enabled);
#endif
#if defined(HW_WIFI_FACTORY_MODE)
	init_wifi_pmic_clock(pdev);
#endif
	return 0;
err_platform_device_register:
	gpio_free(wifi_host->wifi_wakeup_irq);
err_irq_gpio_request:
	gpio_free(wifi_host->enable);
err_enable_gpio_request:
err_gpio_get:
#if defined(CONFIG_BCMDHD_SDIO) && (defined(CONFIG_MMC_DW_HI6XXX) || defined(CONFIG_MMC_DW_HI3XXX))
err_pinctrl_get:
	pinctrl_put(wifi_host->pctrl);
#endif
#if 0
err_regulator_get:
#endif
err_clk_get:
	kfree(wifi_host);
	wifi_host = NULL;
err_malloc_wifi_host:
	//deinit_wifi_mem();
	return ret;
}

#if defined(HW_WIFI_FACTORY_MODE)
int init_wifi_pmic_clock(struct platform_device *pdev) {
	struct device *dev = &pdev->dev;

	g_pmic_clk = devm_clk_get(dev, "clk_wifi");
	if (IS_ERR_OR_NULL(g_pmic_clk)) {
		hwlog_err("devm_clk_get: clk_wifi not found!\n");
		g_pmic_clk = NULL;
		return -EFAULT;
	}
	return 0;
}

int hw_enable_pmic_clock(int enable) {
	int ret = 0;

	if (NULL != g_pmic_clk) {
		if (enable) {
			ret = clk_prepare_enable(g_pmic_clk);
			hwlog_err("hw_enable_pmic_clock: state:%d, ret:%d\n", enable, ret);
		} else {
			ret = 0;
			clk_disable_unprepare(g_pmic_clk);
			hwlog_err("hw_enable_pmic_clock: state:%d\n", enable);
		}
		return ret;
	}
	hwlog_err("hw_enable_pmic_clock: g_pmic_clk is NULL\n");
	return -EFAULT;
}
EXPORT_SYMBOL(hw_enable_pmic_clock);

#endif


static struct of_device_id wifi_power_match_table[] = {
	{ .compatible = DTS_COMP_WIFI_POWER_NAME, 
		.data = NULL,
        	},
	{ },
};

static struct platform_driver wifi_power_driver = {
	.driver		= {
		.name		= "bcm4334_power",
		.owner = THIS_MODULE,
	       	.of_match_table	= wifi_power_match_table,
	},
	.probe          = wifi_power_probe,
	
	
};

static int __init wifi_power_init(void)
{
	int ret;

#ifdef CONFIG_HWCONNECTIVITY
    //For OneTrack, we need check it's the right chip type or not.
    //If it's not the right chip type, don't init the driver
    if (!isMyConnectivityChip(CHIP_TYPE_BCM)) {
        hwlog_err("wifi chip type is not match, skip driver init\n");
        return -EINVAL;
    } else {
        hwlog_info("wifi chip type is matched with Broadcom, continue\n");
    }
#endif

	ret = platform_driver_register(&wifi_power_driver);
	if (ret)
		hwlog_err("%s: platform_driver_register failed, ret:%d.\n",
			__func__, ret);
	return ret;
}

static void __exit wifi_power_exit(void)
{
	platform_driver_unregister(&wifi_power_driver);
}
device_initcall(wifi_power_init);

#ifdef CONFIG_LLT_TEST

struct UT_TEST UT_dev_wifi = {
	.read_from_global_buf = read_from_global_buf,
	.char2_byte = char2_byte,
	.read_from_mac_file = read_from_mac_file,
	.show_wifi_open_state = show_wifi_open_state,
	.restore_wifi_open_state = restore_wifi_open_state,
	.show_wifi_debug_level = show_wifi_debug_level,
	.restore_wifi_debug_level = restore_wifi_debug_level,
	.show_wifi_wrong_action_flag = show_wifi_wrong_action_flag,
	.restore_wifi_arp_timeout = restore_wifi_arp_timeout,
	.restore_wifi_wrong_action_debug = restore_wifi_wrong_action_debug,
	.wifi_power_init = wifi_power_init,
	.bcm_wifi_get_nvram_path = bcm_wifi_get_nvram_path,
#ifdef HW_WIFI_DRIVER_NORMALIZE
	.bcm_wifi_get_fw_path = bcm_wifi_get_fw_path,
	.bcm_wifi_get_chip_type = bcm_wifi_get_chip_type,
#endif /* HW_WIFI_DRIVER_NORMALIZE */
#ifdef HW_CUSTOM_BCN_TIMEOUT
	.bcm_wifi_get_bcn_timeout = bcm_wifi_get_bcn_timeout,
#endif /* HW_CUSTOM_BCN_TIMEOUT */
	.bcm_wifi_reset = bcm_wifi_reset,

};
int Get_wifi_open_err_code()
{
	return wifi_open_err_code;
}

#endif
