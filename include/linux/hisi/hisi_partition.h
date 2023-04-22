/*
*分区表公共头文件
*/
#ifndef _HISI_PARTITION_TABLE_H_
#define _HISI_PARTITION_TABLE_H_

enum {
	EMMC_USER_PART = 0x0,
	EMMC_BOOT_MAJOR_PART = 0x1,
	EMMC_BOOT_BACKUP_PART = 0x2,
};

enum {
	UFS_PART_0 = 0x0,
	UFS_PART_1 = 0x1,
	UFS_PART_2 = 0x2,
	UFS_PART_3 = 0x3,
};

#define PART_NAMELEN                    (36)

#define MAX_UFS_LUN			(8)
#define UFS_OFFSET_128KB		(128*1024)
#define UFS_PTABLE_SIZE_17KB		(17*1024)

#define UNIT_USING_COUNT       		(4)


#if ((defined CONFIG_HISI_PARTITION_HI3650) || (defined CONFIG_HISI_PARTITION_HI6250))

/*ptable*/
#define SECBOOT_PTN_PTABLE_START        (0)
#define SECBOOT_PTN_PTABLE_SIZE         (512 * 1024)
#define SECBOOT_PTN_PTABLE_FLAGS	(EMMC_USER_PART)
/*xloader*/
#define SECBOOT_PTN_XLOADER_START	(0)
#define SECBOOT_PTN_XLOADER_SIZE	(256 * 1024)
#define SECBOOT_PTN_XLOADER_FLAGS	(EMMC_BOOT_MAJOR_PART)
/*vrl*/
#define SECBOOT_PTN_VRL_START           (512 * 1024)
#define SECBOOT_PTN_VRL_SIZE		(256 * 1024)
#define SECBOOT_PTN_VRL_FLAGS           (EMMC_USER_PART)
/*vrl backup*/
#define SECBOOT_PTN_VRL_BACKUP_START    (768 * 1024)
#define SECBOOT_PTN_VRL_BACKUP_SIZE	(256 * 1024)
#define SECBOOT_PTN_VRL_BACKUP_FLAGS    (EMMC_USER_PART)
/*fastboot*/
#define SECBOOT_PTN_FASTBOOT_START	(2048 * 1024)
#define SECBOOT_PTN_FASTBOOT_SIZE	(4096 * 1024)
#define SECBOOT_PTN_FASTBOOT_FLAGS      (EMMC_USER_PART)

#endif

/*partition macro definition*/
#define PART_XLOADER                   "xloader"
#define PART_XLOADER_INSE           "xloader_inse"
#define PART_PTABLE                    "ptable"
#define PART_VRL                       "vrl"
#define PART_VRL_BACKUP                "vrl_backup"
#define PART_FW_LPM3                   "fw_lpm3"
#define PART_FRP                       "frp"
#define PART_FASTBOOT                  "fastboot"
#define PART_MODEMNVM_FACTORY          "modemnvm_factory"
#define PART_NVME                      "nvme"
#define PART_OEMINFO                   "oeminfo"
#define PART_MODEMNVM_BACKUP           "modemnvm_backup"
#define PART_MODEMNVM_IMG              "modemnvm_img"
#define PART_MODEMNVM_SYSTEM           "modemnvm_system"
#define PART_SECURE_STORAGE            "secure_storage"
#define PART_3RDMODEMNVM               "3rdmodemnvm"
#define PART_3RDMODEMNVMBKP            "3rdmodemnvmbkp"
#define PART_PERSIST                   "persist"
#define PART_MODEM_OM                  "modem_om"
#define PART_SPLASH                    "splash"
#define PART_SPLASH2                   "splash2"
#define PART_MISC                      "misc"
#define PART_MODEMNVM_UPDATE           "modemnvm_update"
#define PART_ERECOVERY_KERNEL          "erecovery_kernel"
#define PART_ERECOVERY_RAMDISK         "erecovery_ramdisk"
#define PART_ERECOVERY_VENDOR          "erecovery_vendor"
#define PART_TEEOS                     "teeos"
#define PART_TRUSTFIRMWARE             "trustfirmware"
#define PART_SENSORHUB                 "sensorhub"
#define PART_FW_HIFI                   "fw_hifi"
#define PART_KERNEL                    "kernel"
#define PART_RAMDISK                   "ramdisk"
#define PART_RECOVERY_RAMDISK          "recovery_ramdisk"
#define PART_RECOVERY_VENDOR           "recovery_vendor"
#define PART_DTS                       "dts"
#define PART_DTO                       "dto"
#define PART_MODEM                     "modem"
#define PART_MODEM_DSP                 "modem_dsp"
#define PART_MODEM_DTB                 "modem_dtb"
#define PART_MODEM_FW                  "modem_fw"
#define PART_DFX                       "dfx"
#define PART_3RDMODEM                  "3rdmodem"
#define PART_CACHE                     "cache"
#define PART_HISITEST0                 "hisitest0"
#define PART_HISITEST1                 "hisitest1"
#define PART_HISITEST2                 "hisitest2"
#define PART_SYSTEM                    "system"
#define PART_CUST                      "cust"
#define PART_ODM                       "odm"
#define PART_USERDATA                  "userdata"
#define PART_RESERVED                  "reserved"
#define PART_RESERVED0                 "reserved0"
#define PART_RESERVED1                 "reserved1"
#define PART_RESERVED2                 "reserved2"
#define PART_RESERVED3                 "reserved3"
#define PART_RESERVED4                 "reserved4"
#define PART_RESERVED5                 "reserved5"
#define PART_HIBENCH_IMG               "hibench_img"
#define PART_RESERVED6                 "reserved6"
#define PART_RESERVED7                 "reserved7"
#define PART_RESERVED8                 "reserved8"
#define PART_RESERVED9                 "reserved9"
#define PART_RESERVED10                 "reserved10"
#define PART_RESERVED11                 "reserved11"
#define PART_HHEE                 "hhee"
#define PART_HHEE_A                 "hhee_a"
#define PART_ISP_BOOT                  "isp_boot"
#define PART_ISP_FIRMWARE              "isp_firmware"
#define PART_ISP_DTS                   "isp_dts"
#define PART_HISEE_FS                  "hisee_fs"
#define PART_HISEE_FS_A                  "hisee_fs_a"
#define PART_HISEE_IMG                 "hisee_img"
#define PART_VECTOR                    "vector"
#define PART_VERSION                   "version"
#define PART_VENDOR                    "vendor"
#define PART_VERSION                   "version"
#define PART_PRODUCT                   "product"
#define PART_PRELOAD                   "preload"
/*AB*/
#define PART_PTABLE_LU2                "ptable_lu2"
#define PART_PTABLE_LU3                "ptable_lu3"
#define PART_PTABLE_LU4                "ptable_lu4"
#define PART_PTABLE_LU5                "ptable_lu5"
#define PART_PTABLE_LU6                "ptable_lu6"
#define PART_XLOADER_A                 "xloader_a"
#define PART_XLOADER_INSE_A           "xloader_inse_a"
#define PART_XLOADER_B                 "xloader_b"
#define PART_XLOADER_INSE_B           "xloader_inse_b"
#define PART_PTABLE_ENHANCED           "ptable_enhanced"
#define PART_PTABLE_PROTECT_A          "ptable_protect_a"
#define PART_PTABLE_PROTECT_B          "ptable_protect_b"
#define PART_PTABLE_NORMAL             "ptable_normal"
#define PART_FW_LPM3_A                 "fw_lpm3_a"
#define PART_FW_LPM3_B                 "fw_lpm3_b"
#define PART_FASTBOOT_A                "fastboot_a"
#define PART_FASTBOOT_B                "fastboot_b"
#define PART_ERECOVERY_KERNEL_A        "erecovery_kernel_a"
#define PART_ERECOVERY_KERNEL_B        "erecovery_kernel_b"
#define PART_ERECOVERY_RAMDISK_A       "erecovery_ramdisk_a"
#define PART_ERECOVERY_RAMDISK_B       "erecovery_ramdisk_b"
#define PART_ERECOVERY_VENDOR_A        "erecovery_vendor_a"
#define PART_ERECOVERY_VENDOR_B        "erecovery_vendor_b"
#define PART_TEEOS_A                   "teeos_a"
#define PART_TEEOS_B                   "teeos_b"
#define PART_TRUSTFIRMWARE_A           "trustfirmware_a"
#define PART_TRUSTFIRMWARE_B           "trustfirmware_b"
#define PART_SENSORHUB_A               "sensorhub_a"
#define PART_SENSORHUB_B               "sensorhub_b"
#define PART_FW_HIFI_A                 "fw_hifi_a"
#define PART_FW_HIFI_B                 "fw_hifi_b"
#define PART_KERNEL_A                  "kernel_a"
#define PART_KERNEL_B                  "kernel_b"
#define PART_RAMDISK_A                 "ramdisk_a"
#define PART_RAMDISK_B                 "ramdisk_b"
#define PART_RECOVERY_VENDOR_B         "recovery_vendor_b"
#define PART_RECOVERY_VENDOR_A         "recovery_vendor_a"
#define PART_RECOVERY_RAMDISK_A        "recovery_ramdisk_a"
#define PART_RECOVERY_RAMDISK_B        "recovery_ramdisk_b"
#define PART_MODEMNVM_UPDATE_A         "modemnvm_update_a"
#define PART_MODEMNVM_UPDATE_B         "modemnvm_update_b"
#define PART_DTS_A                     "dts_a"
#define PART_DTS_B                     "dts_b"
#define PART_DTO_A                     "dto_a"
#define PART_DTO_B                     "dto_b"
#define PART_MODEM_FW_A                "modem_fw_a"
#define PART_MODEM_FW_B                "modem_fw_b"
#define PART_SYSTEM_A                  "system_a"
#define PART_SYSTEM_B                  "system_b"
#define PART_CUST_A                    "cust_a"
#define PART_CUST_B                    "cust_b"
#define PART_ODM_A                     "odm_a"
#define PART_ODM_B                     "odm_b"
#define PART_RESERVED2_A               "reserved2_a"
#define PART_RESERVED2_B               "reserved2_b"
#define PART_RESERVED3_A               "reserved3_a"
#define PART_RESERVED3_B               "reserved3_b"
#define PART_RESERVED4_A               "reserved4_a"
#define PART_RESERVED4_B               "reserved4_b"
#define PART_ISP_BOOT_A                "isp_boot_a"
#define PART_ISP_BOOT_B                "isp_boot_b"
#define PART_ISP_FIRMWARE_A            "isp_firmware_a"
#define PART_ISP_FIRMWARE_B            "isp_firmware_b"
#define PART_ISP_DTS_A                 "isp_dts_a"
#define PART_ISP_DTS_B                 "isp_dts_b"
#define PART_HISEE_FS_A                "hisee_fs_a"
#define PART_HISEE_FS_B                "hisee_fs_b"
#define PART_HISEE_IMG_A               "hisee_img_a"
#define PART_HISEE_IMG_B               "hisee_img_b"
#define PART_VECTOR_A                  "vector_a"
#define PART_VECTOR_B                  "vector_b"
#define PART_VERSION_A                 "version_a"
#define PART_VERSION_B                 "version_b"
#define PART_VENDOR_A                  "vendor_a"
#define PART_VENDOR_B                  "vendor_b"
#define PART_PRODUCT_A                 "product_a"
#define PART_PRODUCT_B                 "product_b"
#define PART_BOOTFAIL_INFO             "bootfail_info"
#define PART_RRECORD                   "rrecord"
#define PART_PATCH                     "patch"
#define PART_PATCH_A                   "patch_a"
#define PART_PATCH_B                   "patch_b"
#define PART_MODEM_SECURE              "modem_secure"
#define PART_DDR_PARA                 "ddr_para"
#define PART_HISEE_ENCOS               "hisee_encos"
#define PART_HIBENCH_DATA             "hibench_data"
#define PART_MODEMNVM_CUST_A         "modemnvm_cust_a"
#define PART_HDCP                     "hdcp"
#define PART_HDCP_A                   "hdcp_a"
#define PART_VERITYKEY                "veritykey"
#define PART_CTF                      "certification"
#define PART_VBMETA_A                 "vbmeta_a"
#define PART_VBMETA                   "vbmeta"
#define PART_RECOVERY_VBMETA          "recovery_vbmeta"
#define PART_RECOVERY_VBMETA_A        "recovery_vbmeta_a"
#define PART_ERECOVERY_VBMETA         "erecovery_vbmeta"
#define PART_ERECOVERY_VBMETA_A       "erecovery_vbmeta_a"
#define PART_FLASH_AGEING             "flash_ageing"
#define PART_ENG_SYSTEM_A             "eng_system_a"
#define PART_ENG_VENDOR_A             "eng_vendor_a"
#define PART_ENG_SYSTEM               "eng_system"
#define PART_ENG_VENDOR               "eng_vendor"
#define PART_PRELOAD_A                "preload_a"
#define PART_PRELOAD_B                "preload_b"
#define PART_IVP                      "ivp"
#define PART_LOWPOWER_PARA            "lowpower_para"
#define PART_BATT_TP_PARA             "batt_tp_para"
#define PART_MODEM_PATCH_NV_A         "modem_patch_nv_a"
#define PART_MODEM_DRIVER_A           "modem_driver_a"
#define PART_HIEPS                    "hieps"
#define PART_HIEPS_A                  "hieps_a"
#define PART_HIEPS_B                  "hieps_b"
#endif
