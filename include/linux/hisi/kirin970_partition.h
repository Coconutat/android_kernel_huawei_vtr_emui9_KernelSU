#ifndef _KIRIN970_PARTITION_H_
#define _KIRIN970_PARTITION_H_

#include "hisi_partition.h"
#include "partition_def.h"

static const struct partition partition_table_emmc[] =
{
  {PART_XLOADER_A,        0,         2*1024,        EMMC_BOOT_MAJOR_PART},
  {PART_XLOADER_B,        0,         2*1024,        EMMC_BOOT_BACKUP_PART},
  {PART_PTABLE,           0,         512,           EMMC_USER_PART},/* ptable          512K */
  {PART_FRP,              512,       512,           EMMC_USER_PART},/* frp             512K   p1*/
  {PART_PERSIST,          1024,      2048,          EMMC_USER_PART},/* persist         2048K  p2*/
  {PART_RESERVED1,        3072,      5120,          EMMC_USER_PART},/* reserved1       5120K  p3*/
  {PART_RESERVED6,        8*1024,    512,           EMMC_USER_PART},/* reserved6       512K   p4*/
  {PART_VRL,              8704,      512,           EMMC_USER_PART},/* vrl             512K   p5*/
  {PART_VRL_BACKUP,       9216,      512,           EMMC_USER_PART},/* vrl backup      512K   p6*/
  {PART_MODEM_SECURE,     9728,      8704,          EMMC_USER_PART},/* modem_secure    8704k  p7*/
  {PART_NVME,             18*1024,   5*1024,        EMMC_USER_PART},/* nvme            5M     p8*/
  {PART_CTF,              23*1024,   1*1024,        EMMC_USER_PART},/* PART_CTF        1M     p9*/
  {PART_OEMINFO,          24*1024,   64*1024,       EMMC_USER_PART},/* oeminfo         64M    p10*/
  {PART_SECURE_STORAGE,   88*1024,   32*1024,       EMMC_USER_PART},/* secure storage  32M    p11*/
  {PART_MODEM_OM,         120*1024,  32*1024,       EMMC_USER_PART},/* modem om        32M    p12*/
  {PART_MODEMNVM_FACTORY, 152*1024,  16*1024,       EMMC_USER_PART},/* modemnvmfactory 16M    p13*/
  {PART_MODEMNVM_BACKUP,  168*1024,  16*1024,       EMMC_USER_PART},/* modemnvmbackup  16M    p14*/
  {PART_MODEMNVM_IMG,     184*1024,  20*1024,       EMMC_USER_PART},/* modemnvmimg     20M    p15*/
  {PART_MODEMNVM_SYSTEM,  204*1024,  16*1024,       EMMC_USER_PART},/* modemnvmsystem  16M    p16*/
  {PART_HISEE_ENCOS,      220*1024,  4*1024,        EMMC_USER_PART},/* hisee_encos     4M     p17*/
  {PART_VERITYKEY,        224*1024,  1*1024,        EMMC_USER_PART},/* reserved2       32M    p18*/
  {PART_DDR_PARA,         225*1024,  1*1024,        EMMC_USER_PART},/* DDR_PARA        1M     p19*/
  {PART_RESERVED2,        226*1024,  27*1024,       EMMC_USER_PART},/* reserved2       32M    p20*/
  {PART_SPLASH2,          253*1024,  80*1024,       EMMC_USER_PART},/* splash2         80M    p21*/
  {PART_BOOTFAIL_INFO,    333*1024,  2*1024,        EMMC_USER_PART},/* bootfail info   2MB    p22*/
  {PART_MISC,             335*1024,  2*1024,        EMMC_USER_PART},/* misc            2M     p23*/
  {PART_DFX,              337*1024,  16*1024,       EMMC_USER_PART},/* dfx             16M    p24*/
  {PART_RRECORD,          353*1024,  16*1024,       EMMC_USER_PART},/* rrecord         16M    p25*/
  {PART_FW_LPM3_A,        369*1024,  256,           EMMC_USER_PART},/* mcuimage        256K   p26*/
  {PART_RESERVED3_A,      378112,    3840,          EMMC_USER_PART},/* reserved3A      3840KB p27*/
  {PART_HDCP_A,           373*1024,  1*1024,        EMMC_USER_PART},/* HDCP_A          1M     p28*/
  {PART_HISEE_IMG_A,      374*1024,  4*1024,        EMMC_USER_PART},/* hisee_img_a     4M     p29*/
  {PART_HHEE_A,           378*1024,  4*1024,        EMMC_USER_PART},/* RESERVED10      4M     p30*/
  {PART_HISEE_FS_A,       382*1024,  8*1024,        EMMC_USER_PART},/* hisee_fs        8M     p31*/
  {PART_FASTBOOT_A,       390*1024,  12*1024,       EMMC_USER_PART},/* fastboot        12M    p32*/
  {PART_VECTOR_A,         402*1024,  4*1024,        EMMC_USER_PART},/* avs vector      4M     p33*/
  {PART_ISP_BOOT_A,       406*1024,  2*1024,        EMMC_USER_PART},/* isp_boot        2M     p34*/
  {PART_ISP_FIRMWARE_A,   408*1024,  14*1024,       EMMC_USER_PART},/* isp_firmware    14M    p35*/
  {PART_FW_HIFI_A,        422*1024,  12*1024,       EMMC_USER_PART},/* hifi            12M    p36*/
  {PART_TEEOS_A,          434*1024,  8*1024,        EMMC_USER_PART},/* teeos           8M     p37*/
  {PART_SENSORHUB_A,      442*1024,  16*1024,       EMMC_USER_PART},/* sensorhub       16M    p38*/
#ifdef CONFIG_SANITIZER_ENABLE
  {PART_ERECOVERY_KERNEL_A,  458*1024,  24*1024,    EMMC_USER_PART},/* erecovery_kernel  24M  p39*/
  {PART_ERECOVERY_RAMDISK_A, 482*1024,  32*1024,    EMMC_USER_PART},/* erecovery_ramdisk 32M  p40*/
  {PART_ERECOVERY_VENDOR_A,  514*1024,  8*1024,     EMMC_USER_PART},/* erecovery_vendor  8M   p41*/
  {PART_KERNEL_A,            522*1024,  32*1024,    EMMC_USER_PART},/* kernel            32M  p42*/
#else
  {PART_ERECOVERY_KERNEL_A,  458*1024,  24*1024,    EMMC_USER_PART},/* erecovery_kernel  24M  p39*/
  {PART_ERECOVERY_RAMDISK_A, 482*1024,  32*1024,    EMMC_USER_PART},/* erecovery_ramdisk 32M  p40*/
  {PART_ERECOVERY_VENDOR_A,  514*1024,  16*1024,    EMMC_USER_PART},/* erecovery_vendor  16M  p41*/
  {PART_KERNEL_A,            530*1024,  24*1024,    EMMC_USER_PART},/* kernel            24M  p42*/
#endif
  {PART_ENG_SYSTEM_A,        554*1024,  12*1024,    EMMC_USER_PART},/* eng_system        12M  p43*/
  {PART_RECOVERY_RAMDISK_A,  566*1024,  32*1024,    EMMC_USER_PART},/* recovery_ramdisk  32M  p44*/
  {PART_RECOVERY_VENDOR_A,   598*1024,  16*1024,    EMMC_USER_PART},/* recovery_vendor   16M  p45*/
  {PART_DTS_A,               614*1024,  21*1024,    EMMC_USER_PART},/* dtimage           21M  p46*/
  {PART_DTO_A,               635*1024,  7*1024,     EMMC_USER_PART},/* dtoimage          7M   p47*/
  {PART_TRUSTFIRMWARE_A,     642*1024,  2*1024,     EMMC_USER_PART},/* trustfirmware     2M   p48*/
  {PART_MODEM_FW_A,          644*1024,  56*1024,    EMMC_USER_PART},/* modem_fw          56M  p49*/
  {PART_ENG_VENDOR_A,        700*1024,  12*1024,    EMMC_USER_PART},/* eng_vendor        12M  p50*/
  {PART_RECOVERY_VBMETA_A,   712*1024,  2*1024,     EMMC_USER_PART},/* recovery_vbmeta_a  2M  p51*/
  {PART_ERECOVERY_VBMETA_A,  714*1024,  2*1024,     EMMC_USER_PART},/* erecovery_vbmeta_a 2M  p52*/
  {PART_VBMETA_A,            716*1024,  4*1024,     EMMC_USER_PART},/* PART_VBMETA_A      4M  p53*/
  {PART_MODEMNVM_UPDATE_A,   720*1024,  16*1024,    EMMC_USER_PART},/* modemnvm update   16M  p54*/
  {PART_MODEMNVM_CUST_A,     736*1024,  40*1024,    EMMC_USER_PART},/* modemnvm cust     40M  p55*/
  {PART_PATCH_A,             776*1024,  32*1024,    EMMC_USER_PART},/* patch             32M  p56*/
#ifdef CONFIG_FACTORY_MODE
  {PART_VERSION_A,        808*1024,  32*1024,       EMMC_USER_PART},/* version         32M    p57*/
  {PART_VENDOR_A,         840*1024,  952*1024,      EMMC_USER_PART},/* vendor          952M   p58*/
  {PART_PRELOAD_A,        1792*1024, 8*1024,        EMMC_USER_PART},/* preload         8M     p59*/
  {PART_CUST_A,           1800*1024, 192*1024,      EMMC_USER_PART},/* cust            192M   p60*/
  {PART_ODM_A,            1992*1024, 144*1024,      EMMC_USER_PART},/* odm             144M   p61*/
  {PART_CACHE,            2136*1024, 128*1024,      EMMC_USER_PART},/* cache           128M   p62*/
  {PART_PRODUCT_A,        2264*1024, 592*1024,      EMMC_USER_PART},/* product         592M   p63*/
  {PART_SYSTEM_A,         2856*1024, 5040*1024,       EMMC_USER_PART},/* system        5040M  p64*/
  {PART_RESERVED5,        7896*1024, 128*1024,        EMMC_USER_PART},/* reserved5     128M   p65*/
  {PART_HIBENCH_DATA,     8024*1024, 512*1024,        EMMC_USER_PART},/* hibench_data  512M   p66*/
  {PART_USERDATA,         8536*1024, (4UL)*1024*1024, EMMC_USER_PART},/* userdata      4G     p67*/
#else
 #ifdef CONFIG_NEW_PRODUCT_P
  {PART_CACHE,           808*1024,   128*1024,      EMMC_USER_PART},/* cache           128M   p57*/
  {PART_VENDOR_A,        936*1024,   1232*1024,     EMMC_USER_PART},/* vendor          1232M  p58*/
  {PART_ODM_A,           2168*1024,  192*1024,      EMMC_USER_PART},/* odm             192M   p59*/
  {PART_CUST_A,          2360*1024,  192*1024,      EMMC_USER_PART},/* cust            192M   p60*/
  #ifdef CONFIG_MARKET_INTERNAL
  {PART_SYSTEM_A,        2552*1024,  4784*1024,       EMMC_USER_PART},/* system        4784M  p61*/
  {PART_PRODUCT_A,       7336*1024,  1472*1024,       EMMC_USER_PART},/* product       1472M  p62*/
  {PART_VERSION_A,       8808*1024,  576*1024,        EMMC_USER_PART},/* version       576M   p63*/
  {PART_PRELOAD_A,       9384*1024,  1144*1024,       EMMC_USER_PART},/* preload       1144M  p64*/
  {PART_USERDATA,       10528*1024, (4UL)*1024*1024,  EMMC_USER_PART},/* userdata      4G     p65*/
  #else
  {PART_SYSTEM_A,        2552*1024,  5848*1024,       EMMC_USER_PART},/* system        5848M  p61*/
  {PART_PRODUCT_A,       8400*1024,  1328*1024,       EMMC_USER_PART},/* product       1328M  p62*/
  {PART_VERSION_A,       9728*1024,  576*1024,        EMMC_USER_PART},/* version       576M   p63*/
  {PART_PRELOAD_A,       10304*1024, 1144*1024,       EMMC_USER_PART},/* preload       1144M  p64*/
  {PART_USERDATA,        11448*1024, (4UL)*1024*1024, EMMC_USER_PART},/* userdata      4G     p65*/
  #endif
 #else
  {PART_VERSION_A,        808*1024,  32*1024,       EMMC_USER_PART},/* version         32M    p57*/
  {PART_VENDOR_A,         840*1024,  952*1024,      EMMC_USER_PART},/* vendor          952M   p58*/
  {PART_PRELOAD_A,        1792*1024, 8*1024,        EMMC_USER_PART},/* preload         8M     p59*/
  {PART_CUST_A,           1800*1024, 192*1024,      EMMC_USER_PART},/* cust            192M   p60*/
  {PART_ODM_A,            1992*1024, 144*1024,      EMMC_USER_PART},/* odm             144M   p61*/
  {PART_CACHE,            2136*1024, 128*1024,      EMMC_USER_PART},/* cache           128M   p62*/
  {PART_PRODUCT_A,        2264*1024, 592*1024,      EMMC_USER_PART},/* product         592M   p63*/
  #ifdef CONFIG_MARKET_INTERNAL
  {PART_SYSTEM_A,         2856*1024, 4160*1024,       EMMC_USER_PART},/* system        4160M  p64*/
  {PART_USERDATA,         7016*1024, (4UL)*1024*1024, EMMC_USER_PART},/* userdata      4G     p65*/
  #else
  {PART_SYSTEM_A,         2856*1024, 5040*1024,       EMMC_USER_PART},/* system        5040M  p64*/
  {PART_USERDATA,         7896*1024, (4UL)*1024*1024, EMMC_USER_PART},/* userdata      4G     p65*/
  #endif
 #endif
#endif
  {"0", 0, 0, 0},                                        /* total 11896M*/
};
#ifdef CONFIG_HISI_STORAGE_UFS_PARTITION
static const struct partition partition_table_ufs[] =
{
  {PART_XLOADER_A,        0,         2*1024,        UFS_PART_0},
  {PART_XLOADER_B,        0,         2*1024,        UFS_PART_1},
  {PART_PTABLE,           0,         512,           UFS_PART_2},/* ptable          512K */
  {PART_FRP,              512,       512,           UFS_PART_2},/* frp             512K   p1*/
  {PART_PERSIST,          1024,      2048,          UFS_PART_2},/* persist         2048K  p2*/
  {PART_RESERVED1,        3072,      5120,          UFS_PART_2},/* reserved1       5120K  p3*/
  {PART_PTABLE_LU3,       0,         512,           UFS_PART_3},/* ptable_lu3      512K   p0*/
  {PART_VRL,              512,       512,           UFS_PART_3},/* vrl             512K   p1*/
  {PART_VRL_BACKUP,       1024,      512,           UFS_PART_3},/* vrl backup      512K   p2*/
  {PART_MODEM_SECURE,     1536,      8704,          UFS_PART_3},/* modem_secure    8704k  p3*/
  {PART_NVME,             10*1024,   5*1024,        UFS_PART_3},/* nvme            5M     p4*/
  {PART_CTF,              15*1024,   1*1024,        UFS_PART_3},/* PART_CTF        1M     p5*/
  {PART_OEMINFO,          16*1024,   64*1024,       UFS_PART_3},/* oeminfo         64M    p6*/
  {PART_SECURE_STORAGE,   80*1024,   32*1024,       UFS_PART_3},/* secure storage  32M    p7*/
  {PART_MODEM_OM,         112*1024,  32*1024,       UFS_PART_3},/* modem om        32M    p8*/
  {PART_MODEMNVM_FACTORY, 144*1024,  16*1024,       UFS_PART_3},/* modemnvmfactory 16M    p9*/
  {PART_MODEMNVM_BACKUP,  160*1024,  16*1024,       UFS_PART_3},/* modemnvmbackup  16M    p10*/
  {PART_MODEMNVM_IMG,     176*1024,  20*1024,       UFS_PART_3},/* modemnvmimg     20M    p11*/
  {PART_MODEMNVM_SYSTEM,  196*1024,  16*1024,       UFS_PART_3},/* modemnvmsystem  16M    p12*/
  {PART_HISEE_ENCOS,      212*1024,  4*1024,        UFS_PART_3},/* hisee_encos     4M     p13*/
  {PART_VERITYKEY,        216*1024,  1*1024,        UFS_PART_3},/* reserved2       32M    p14*/
  {PART_DDR_PARA,         217*1024,  1*1024,        UFS_PART_3},/* DDR_PARA        1M     p15*/
  {PART_RESERVED2,        218*1024,  27*1024,       UFS_PART_3},/* reserved2       32M    p16*/
  {PART_SPLASH2,          245*1024,  80*1024,       UFS_PART_3},/* splash2         80M    p17*/
  {PART_BOOTFAIL_INFO,    325*1024,  2*1024,        UFS_PART_3},/* bootfail info   2MB    p18*/
  {PART_MISC,             327*1024,  2*1024,        UFS_PART_3},/* misc            2M     p19*/
  {PART_DFX,              329*1024,  16*1024,       UFS_PART_3},/* dfx             16M    p20*/
  {PART_RRECORD,          345*1024,  16*1024,       UFS_PART_3},/* rrecord         16M    p21*/
  {PART_FW_LPM3_A,        361*1024,  256,           UFS_PART_3},/* mcuimage        256K   p22*/
  {PART_RESERVED3_A,      369920,    3840,          UFS_PART_3},/* reserved3A      3840K  p23*/
  {PART_HDCP_A,           365*1024,  1*1024,        UFS_PART_3},/* HDCP_A          1M     p24*/
  {PART_HISEE_IMG_A,      366*1024,  4*1024,        UFS_PART_3},/* hisee_img_a     4M     p25*/
  {PART_HHEE_A,           370*1024,  4*1024,        UFS_PART_3},/* RESERVED10      4M     p26*/
  {PART_HISEE_FS_A,       374*1024,  8*1024,        UFS_PART_3},/* hisee_fs        8M     p27*/
  {PART_FASTBOOT_A,       382*1024,  12*1024,       UFS_PART_3},/* fastboot        12M    p28*/
  {PART_VECTOR_A,         394*1024,  4*1024,        UFS_PART_3},/* avs vector      4M     p29*/
  {PART_ISP_BOOT_A,       398*1024,  2*1024,        UFS_PART_3},/* isp_boot        2M     p30*/
  {PART_ISP_FIRMWARE_A,   400*1024,  14*1024,       UFS_PART_3},/* isp_firmware    14M    p31*/
  {PART_FW_HIFI_A,        414*1024,  12*1024,       UFS_PART_3},/* hifi            12M    p32*/
  {PART_TEEOS_A,          426*1024,  8*1024,        UFS_PART_3},/* teeos           8M     p33*/
  {PART_SENSORHUB_A,      434*1024,  16*1024,       UFS_PART_3},/* sensorhub       16M    p34*/
#ifdef CONFIG_SANITIZER_ENABLE
  {PART_ERECOVERY_KERNEL_A,  450*1024,  24*1024,    UFS_PART_3},/* erecovery_kernel  24M  p35*/
  {PART_ERECOVERY_RAMDISK_A, 474*1024,  32*1024,    UFS_PART_3},/* erecovery_ramdisk 32M  p36*/
  {PART_ERECOVERY_VENDOR_A,  506*1024,  8*1024,     UFS_PART_3},/* erecovery_vendor  8M   p37*/
  {PART_KERNEL_A,            514*1024,  32*1024,    UFS_PART_3},/* kernel            32M  p38*/
#else
  {PART_ERECOVERY_KERNEL_A,  450*1024,  24*1024,    UFS_PART_3},/* erecovery_kernel  24M  p35*/
  {PART_ERECOVERY_RAMDISK_A, 474*1024,  32*1024,    UFS_PART_3},/* erecovery_ramdisk 32M  p36*/
  {PART_ERECOVERY_VENDOR_A,  506*1024,  16*1024,    UFS_PART_3},/* erecovery_vendor  16M  p37*/
  {PART_KERNEL_A,            522*1024,  24*1024,    UFS_PART_3},/* kernel            24M  p38*/
#endif
  {PART_ENG_SYSTEM_A,        546*1024,  12*1024,    UFS_PART_3},/* eng_system        12M  p39*/
  {PART_RECOVERY_RAMDISK_A,  558*1024,  32*1024,    UFS_PART_3},/* recovery_ramdisk  32M  p40*/
  {PART_RECOVERY_VENDOR_A,   590*1024,  16*1024,    UFS_PART_3},/* recovery_vendor   16M  p41*/
  {PART_DTS_A,               606*1024,  21*1024,    UFS_PART_3},/* dtimage           21M  p42*/
  {PART_DTO_A,               627*1024,  7*1024,     UFS_PART_3},/* dtoimage          7M   p43*/
  {PART_TRUSTFIRMWARE_A,     634*1024,  2*1024,     UFS_PART_3},/* trustfirmware     2M   p44*/
  {PART_MODEM_FW_A,          636*1024,  56*1024,    UFS_PART_3},/* modem_fw          56M  p45*/
  {PART_ENG_VENDOR_A,        692*1024,  12*1024,    UFS_PART_3},/* eng_vendor        12M  p46*/
  {PART_RECOVERY_VBMETA_A,   704*1024,  2*1024,     UFS_PART_3},/* recovery_vbmeta_a  2M  p47*/
  {PART_ERECOVERY_VBMETA_A,  706*1024,  2*1024,     UFS_PART_3},/* erecovery_vbmeta_a 2M  p48*/
  {PART_VBMETA_A,            708*1024,  4*1024,     UFS_PART_3},/* VBMETA_A          4M   p49*/
  {PART_MODEMNVM_UPDATE_A,   712*1024,  16*1024,    UFS_PART_3},/* modemnvm update   16M  p50*/
  {PART_MODEMNVM_CUST_A,     728*1024,  40*1024,    UFS_PART_3},/* modemnvm cust     64M  p51*/
  {PART_PATCH_A,             768*1024,  32*1024,    UFS_PART_3},/* patch             32M  p52*/
#ifdef CONFIG_FACTORY_MODE
  {PART_VERSION_A,        800*1024,   32*1024,         UFS_PART_3},/* version      32M    p53*/
  {PART_VENDOR_A,         832*1024,   952*1024,        UFS_PART_3},/* vendor       952M   p54*/
  {PART_PRELOAD_A,        1784*1024,  8*1024,          UFS_PART_3},/* preload      8M     p55*/
  {PART_CUST_A,           1792*1024,  192*1024,        UFS_PART_3},/* cust         192M   p56*/
  {PART_ODM_A,            1984*1024,  144*1024,        UFS_PART_3},/* odm          144M   p57*/
  {PART_CACHE,            2128*1024,  128*1024,        UFS_PART_3},/* cache        128M   p58*/
  {PART_PRODUCT_A,        2256*1024,  592*1024,        UFS_PART_3},/* product      592M   p59*/
  {PART_SYSTEM_A,         2848*1024,  5040*1024,       UFS_PART_3},/* system       5040M  p60*/
  {PART_RESERVED5,        7888*1024,  128*1024,        UFS_PART_3},/* reserved5    128M   p61*/
  {PART_HIBENCH_DATA,     8016*1024,  512*1024,        UFS_PART_3},/* hibench_data 512M   p62*/
  {PART_USERDATA,         8528*1024,  (4UL)*1024*1024, UFS_PART_3},/* userdata     4G     p63*/
#else
 #ifdef CONFIG_NEW_PRODUCT_P
  {PART_CACHE,           800*1024,   128*1024,      UFS_PART_3},/* cache           128M   p53*/
  {PART_VENDOR_A,        928*1024,   1232*1024,     UFS_PART_3},/* vendor          1232M  p54*/
  {PART_ODM_A,           2160*1024,  192*1024,      UFS_PART_3},/* odm             192M   p55*/
  {PART_CUST_A,          2352*1024,  192*1024,      UFS_PART_3},/* cust            192M   p56*/
   #ifdef CONFIG_MARKET_INTERNAL
  {PART_SYSTEM_A,        2544*1024,  4784*1024,       UFS_PART_3},/* system        4784M  p57*/
  {PART_PRODUCT_A,       7328*1024,  1472*1024,       UFS_PART_3},/* product       1472M  p58*/
  {PART_VERSION_A,       8800*1024,  576*1024,        UFS_PART_3},/* version       576M   p59*/
  {PART_PRELOAD_A,       9376*1024,  1144*1024,       UFS_PART_3},/* preload       1144M  p60*/
  {PART_USERDATA,       10520*1024, (4UL)*1024*1024,  UFS_PART_3},/* userdata      4G     p61*/
   #else
  {PART_SYSTEM_A,        2544*1024,  5848*1024,       UFS_PART_3},/* system        5848M  p57*/
  {PART_PRODUCT_A,       8392*1024,  1328*1024,       UFS_PART_3},/* product       1328M  p58*/
  {PART_VERSION_A,       9720*1024,  576*1024,        UFS_PART_3},/* version       576M   p59*/
  {PART_PRELOAD_A,       10296*1024, 1144*1024,       UFS_PART_3},/* preload       1144M  p60*/
  {PART_USERDATA,        11440*1024, (4UL)*1024*1024, UFS_PART_3},/* userdata      4G     p61*/
   #endif
 #else
  {PART_VERSION_A,        800*1024,   32*1024,      UFS_PART_3},/* version         32M    p53*/
  {PART_VENDOR_A,         832*1024,   952*1024,     UFS_PART_3},/* vendor          952M   p54*/
  {PART_PRELOAD_A,        1784*1024,  8*1024,       UFS_PART_3},/* preload         8M     p55*/
  {PART_CUST_A,           1792*1024,  192*1024,     UFS_PART_3},/* cust            192M   p56*/
  {PART_ODM_A,            1984*1024,  144*1024,     UFS_PART_3},/* odm             144M   p57*/
  #ifdef CONFIG_HISI_CACHE_OVERSEA
  {PART_CACHE,            2128*1024, 1300*1024,     UFS_PART_3},/* cache           1300M  p58*/
  {PART_PRODUCT_A,        3428*1024, 592*1024,      UFS_PART_3},/* product         592M   p59*/
   #ifdef CONFIG_MARKET_INTERNAL
  {PART_SYSTEM_A,         4020*1024, 4160*1024,       UFS_PART_3},/* system        4160M  p60*/
  {PART_USERDATA,         8180*1024, (4UL)*1024*1024, UFS_PART_3},/* userdata      4G     p61*/
   #else
  {PART_SYSTEM_A,         4020*1024, 5040*1024,       UFS_PART_3},/* system        5040M  p60*/
  {PART_USERDATA,         9060*1024, (4UL)*1024*1024, UFS_PART_3},/* userdata      4G     p61*/
   #endif
  #else
  {PART_CACHE,            2128*1024, 128*1024,        UFS_PART_3},/* cache         128M   p58*/
  {PART_PRODUCT_A,        2256*1024, 592*1024,        UFS_PART_3},/* product       592M   p59*/
   #ifdef CONFIG_MARKET_INTERNAL
  {PART_SYSTEM_A,         2848*1024, 4160*1024,       UFS_PART_3},/* system        4160M  p60*/
  {PART_USERDATA,         7008*1024, (4UL)*1024*1024, UFS_PART_3},/* userdata      4G     p61*/
   #else
  {PART_SYSTEM_A,         2848*1024, 5040*1024,       UFS_PART_3},/* system        5040M  p60*/
  {PART_USERDATA,         7888*1024, (4UL)*1024*1024, UFS_PART_3},/* userdata      4G     p61*/
   #endif
  #endif
 #endif
#endif
  {"0", 0, 0, 0},
};
#endif

#endif
