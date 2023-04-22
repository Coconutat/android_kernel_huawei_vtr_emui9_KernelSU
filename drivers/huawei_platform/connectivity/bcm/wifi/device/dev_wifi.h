/*
 * hisi_wifi_power.h
 *
 * Copyright (C) 2010 Texas Instruments Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _HISI_WIFI_POWER_H
#define _HISI_WIFI_POWER_H

#include <linux/clk.h>

#define WLAN_MAC_NV_NAME        "MACWLAN"
#define WLAN_MAC_LEN            6
#define NV_WLAN_NUM             193
#define WLAN_VALID_SIZE      17
#define NV_WLAN_VALID_SIZE   12
#define NVRAM_PATH_LEN       (129)
#ifdef HW_WIFI_DRIVER_NORMALIZE
#define FW_PATH_LEN       (129)
#define CHIP_NAME_LEN       (17)
#endif /* HW_WIFI_DRIVER_NORMALIZE */


#ifdef CONFIG_DHD_USE_STATIC_BUF
#define NV_NAME_LENGTH 8     /*NV name maximum length*/
#define NVE_NV_DATA_SIZE 104 /*NV data maximum length*/

#define NV_WRITE 0 /*NV write operation*/
#define NV_READ 1  /*NV read  operation*/


struct bcm_nve_info_user {
	uint32_t nv_operation;
	uint32_t nv_number;
	char nv_name[NV_NAME_LENGTH];
	uint32_t valid_size;
	u_char nv_data[NVE_NV_DATA_SIZE];
};




typedef struct wifi_mem_prealloc_struct {
    void *mem_ptr;
    unsigned long size;
} wifi_mem_prealloc_t;

enum dhd_prealloc_index {
    DHD_PREALLOC_PROT = 0,
    DHD_PREALLOC_RXBUF,
    DHD_PREALLOC_DATABUF,
    DHD_PREALLOC_OSL_BUF,
    DHD_PREALLOC_SKB_BUF,
    DHD_PREALLOC_WIPHY_ESCAN0 = 5,
    DHD_PREALLOC_WIPHY_ESCAN1 = 6,
    DHD_PREALLOC_DHD_INFO = 7,
    DHD_PREALLOC_DHD_WLFC_INFO = 8,
    DHD_PREALLOC_IF_FLOW_LKUP = 9,
    DHD_PREALLOC_FLOWRING = 10,
#ifdef CONFIG_BCMDHD_PCIE
    DHD_PREALLOC_PKTID_MAP = 13,
    DHD_PREALLOC_PKTID_MAP_IOCTL = 14,
    DHD_PREALLOC_IF_FLOW_TBL = 15,
#endif
    DHD_PREALLOC_MAX
};

#define STATIC_BUF_MAX_NUM  20

#ifdef HW_SOFTAP_THROUGHPUT_OPTIMIZE
#define STATIC_BUF_SIZE (PAGE_SIZE*4)
#define PREALLOC_WLAN_BUF_NUM       320
#else
#define STATIC_BUF_SIZE (PAGE_SIZE*2)
#define PREALLOC_WLAN_BUF_NUM       160
#endif

#define DHD_PREALLOC_WIPHY_ESCAN0_SIZE  (64 * 1024)
#ifndef CONFIG_BCMDHD_PCIE
#define DHD_PREALLOC_DHD_INFO_SIZE      (24 * 1024)
#else
#define DHD_PREALLOC_DHD_INFO_SIZE      (26 * 1024)
#endif
#ifdef CONFIG_64BIT
#define DHD_PREALLOC_IF_FLOW_LKUP_SIZE  (20 * 1024 * 2)
#else
#define DHD_PREALLOC_IF_FLOW_LKUP_SIZE  (20 * 1024)
#endif
#ifdef BCM_ALLOC_STATIC_10K
#define DHD_PREALLOC_IF_FLOW_TBL_SIZE	(10 * 1024)
#endif
#define DHD_PREALLOC_OSL_BUF_SIZE      (STATIC_BUF_MAX_NUM * STATIC_BUF_SIZE)

#if defined(CONFIG_64BIT)
#define WLAN_DHD_IF_FLOW_LKUP_SIZE  (64 * 1024)
#else
#define WLAN_DHD_IF_FLOW_LKUP_SIZE  (20 * 1024)
#endif /* CONFIG_64BIT */

#define PREALLOC_WLAN_SEC_NUM       2

#define PREALLOC_WLAN_SECTION_HEADER    24

#define DHD_SKB_1PAGE_BUFSIZE   (PAGE_SIZE*1)
#define DHD_SKB_2PAGE_BUFSIZE   (PAGE_SIZE*2)
#define DHD_SKB_4PAGE_BUFSIZE   (PAGE_SIZE*4)

#ifdef CONFIG_BCMDHD_PCIE
#define WLAN_SECTION_SIZE_0 (PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_1 0
#define WLAN_SECTION_SIZE_2 0
#define WLAN_SECTION_SIZE_3 (PREALLOC_WLAN_BUF_NUM * 1024)

#define DHD_SKB_1PAGE_BUF_NUM   0
#define DHD_SKB_2PAGE_BUF_NUM   64
#define DHD_SKB_4PAGE_BUF_NUM   0
#define WLAN_DHD_PREALLOC_PKTID_MAP	(170 * 1024)
#else
#define WLAN_SECTION_SIZE_0 (PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_1 (PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_2 (PREALLOC_WLAN_BUF_NUM * 512)
#define WLAN_SECTION_SIZE_3 (PREALLOC_WLAN_BUF_NUM * 1024)

#define DHD_SKB_1PAGE_BUF_NUM   8
#define DHD_SKB_2PAGE_BUF_NUM   8
#define DHD_SKB_4PAGE_BUF_NUM   1
#endif /* CONFIG_BCMDHD_PCIE */

#define WLAN_SKB_1_2PAGE_BUF_NUM    ((DHD_SKB_1PAGE_BUF_NUM) + \
        (DHD_SKB_2PAGE_BUF_NUM))
#define WLAN_SKB_BUF_NUM    ((WLAN_SKB_1_2PAGE_BUF_NUM) + \
        (DHD_SKB_4PAGE_BUF_NUM))
#define DHD_PREALLOC_PROT_SIZE      WLAN_SECTION_SIZE_0
#endif /* CONFIG_DHD_USE_STATIC_BUF */

#define MAC_ADDRESS_FILE "/data/misc/wifi/macwifi"
struct wifi_host_s {
	struct regulator *vdd;
	struct clk *clk;
	struct pinctrl *pctrl;					
	struct pinctrl_state *pins_normal;
	struct pinctrl_state *pins_idle;
	char chip_fpga;
	bool bEnable;
	int enable;
	int wifi_wakeup_irq;
#ifdef DHD_DEVWAKE_EARLY
	int dev_wake;
#endif
	char nvram_path[NVRAM_PATH_LEN];
#ifdef HW_WIFI_DRIVER_NORMALIZE
	char fw_path[FW_PATH_LEN];
	char chip_type[CHIP_NAME_LEN];
#endif /* HW_WIFI_DRIVER_NORMALIZE */
#ifdef HW_CUSTOM_BCN_TIMEOUT
	int bcn_timeout;
#endif /* HW_CUSTOM_BCN_TIMEOUT */
	unsigned char macAddr[WLAN_MAC_LEN];
};

enum WIFI_SUPPLY_OP_TYPE {
	WIFI_REGU_GET = 0,
	WIFI_REGU_PUT = 1,
	WIFI_REGU_ENABLE = 2,
	WIFI_REGU_DISABLE = 3,
	WIFI_REGU_SET_VOLTAGE = 4,
};

struct WIFI_VCC_CMD_S {
	int optype;
	struct regulator **regulator;
	int min_uv;
	int max_uv;
	int waitms;
};

#ifdef CONFIG_LLT_TEST
struct UT_TEST {
	void (*read_from_global_buf)(unsigned char *);
	int (*char2_byte)( char *, char *);
	int (*read_from_mac_file)(unsigned char *);
	ssize_t (*show_wifi_open_state)(struct device *,struct device_attribute *, char *);

	ssize_t (*restore_wifi_open_state)(struct device *, struct device_attribute *,const char *, size_t );
    ssize_t (*show_wifi_debug_level)(struct device *,struct device_attribute *, char *);
    ssize_t (*restore_wifi_debug_level)(struct device *, struct device_attribute *,const char *, size_t );
    ssize_t (*show_wifi_wrong_action_flag)(struct device *,struct device_attribute *, char *);
	ssize_t (*restore_wifi_arp_timeout)(struct device *, struct device_attribute *,const char *, size_t );
    ssize_t (*restore_wifi_wrong_action_debug)(struct device *, struct device_attribute *,const char *, size_t ) ;
	int (*bcm_wifi_get_nvram_path)(char *, int );
#ifdef HW_WIFI_DRIVER_NORMALIZE
	int (*bcm_wifi_get_fw_path)(char *, int );
    int (*bcm_wifi_get_chip_type)(char *, int );	
#endif /* HW_WIFI_DRIVER_NORMALIZE */
#ifdef HW_CUSTOM_BCN_TIMEOUT
	int (*bcm_wifi_get_bcn_timeout)(void);
#endif /* HW_CUSTOM_BCN_TIMEOUT */
	int (*wifi_power_init)(void);
	int (*bcm_wifi_reset)(int);
};
#endif


#define K3V3_WIFI_VDD_VOLTAGE   1800000

extern int hi_sdio_detectcard_to_core(int val);
extern void hi_sdio_set_power(int val);

#endif /*_K3V2_WIFI_POWER_H*/
