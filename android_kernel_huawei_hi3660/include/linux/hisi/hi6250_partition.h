#ifndef _HI6250_PARTITION_H_
#define _HI6250_PARTITION_H_

#include "hisi_partition.h"
#include "partition_def.h"

static const struct partition partition_table_emmc[] =
{
    {PART_XLOADER,           0,         256,      EMMC_BOOT_MAJOR_PART},
    {PART_PTABLE,            0,         512,      EMMC_USER_PART}, /* ptable           512K */
    {PART_VRL,               512,       256,      EMMC_USER_PART}, /* VRL              256K p1*/
    {PART_VRL_BACKUP,        768,       256,      EMMC_USER_PART}, /* VRL backup       256K p2*/
    {PART_FW_LPM3,           1024,      256,      EMMC_USER_PART}, /* mcuimage         256K p3*/
    {PART_FRP,               1280,      768,      EMMC_USER_PART}, /* frp              768K p4*/
    {PART_FASTBOOT,          2*1024,    4*1024,   EMMC_USER_PART}, /* fastboot         4M   p5*/
    {PART_MODEMNVM_FACTORY,  6*1024,    4*1024,   EMMC_USER_PART}, /* modemnvm factory 4M   p6*/
    {PART_NVME,              10*1024,   6*1024,   EMMC_USER_PART}, /* nvme             6M   p7*/
    {PART_OEMINFO,           16*1024,   64*1024,  EMMC_USER_PART}, /* oeminfo          64M  p8*/
    {PART_RESERVED3,         80*1024,   4*1024,   EMMC_USER_PART}, /* reserved3        4M   p9*/
    {PART_MODEMNVM_BACKUP,   84*1024,   4*1024,   EMMC_USER_PART}, /* modemnvm backup  4M   p10*/
    {PART_MODEMNVM_IMG,      88*1024,   8*1024,   EMMC_USER_PART}, /* modemnvm img     8M   p11*/
    {PART_MODEMNVM_SYSTEM,   96*1024,   4*1024,   EMMC_USER_PART}, /* modemnvm system  4M   p12*/
    {PART_SECURE_STORAGE,    100*1024,  32*1024,  EMMC_USER_PART}, /* secure storage   32M  p13*/
    {PART_RESERVED4,         132*1024,  2*1024,   EMMC_USER_PART}, /* reserved4        2M   p14*/
    {PART_RESERVED5,         134*1024,  2*1024,   EMMC_USER_PART}, /* reserved5        2M   p15*/
    {PART_PERSIST,           136*1024,  2*1024,   EMMC_USER_PART}, /* persist          2M   p16*/
#ifdef SECDOG_SUPPORT_RSA_2048
    {PART_MODEM_SECURE,      138*1024,  14*1024,  EMMC_USER_PART}, /* modem_secure     14M  p17*/
#else
    {PART_RESERVED1,         138*1024,  14*1024,  EMMC_USER_PART}, /* reserved1        14M  p17*/
#endif
    {PART_MODEM_OM,          152*1024,  32*1024,  EMMC_USER_PART}, /* modem om         32M  p18*/
    {PART_SPLASH2,           184*1024,  64*1024,  EMMC_USER_PART}, /* splash2          64M  p19*/
    {PART_MISC,              248*1024,  2*1024,   EMMC_USER_PART}, /* misc             2M   p20*/
    {PART_MODEMNVM_UPDATE,   250*1024,  24*1024,  EMMC_USER_PART}, /* modemnvm update  24M  p21*/
    {PART_RESERVED2,         274*1024,  52*1024,  EMMC_USER_PART}, /* reserved2        52M  p22*/
    {PART_PRELOAD,           326*1024,  8*1024,   EMMC_USER_PART}, /* preload          8M   p23*/
    {PART_TEEOS,             334*1024,  4*1024,   EMMC_USER_PART}, /* teeos            4M   p24*/
    {PART_TRUSTFIRMWARE,     338*1024,  2*1024,   EMMC_USER_PART}, /* trustfirmware    2M   p25*/
    {PART_SENSORHUB,         340*1024,  16*1024,  EMMC_USER_PART}, /* sensorhub        16M  p26*/
    {PART_FW_HIFI,           356*1024,  12*1024,  EMMC_USER_PART}, /* hifi             12M  p27*/
    {PART_ERECOVERY_KERNEL,  368*1024,  24*1024,  EMMC_USER_PART},/* erecovery_kernel  24M  p28*/
    {PART_ERECOVERY_RAMDISK, 392*1024,  32*1024,  EMMC_USER_PART},/* erecovery_ramdisk 32M  p29*/
    {PART_ERECOVERY_VENDOR,  424*1024,  16*1024,  EMMC_USER_PART},/* erecovery_vendor 16M   p30*/
    {PART_KERNEL,            440*1024,  24*1024,  EMMC_USER_PART},/* kernel          24M    p31*/
    {PART_ENG_SYSTEM,      464*1024,  12*1024,  EMMC_USER_PART},/* eng_system        12M    p32*/
    {PART_RECOVERY_RAMDISK,  476*1024,  32*1024,  EMMC_USER_PART},/* recovery_ramdisk 32M   p33*/
    {PART_RECOVERY_VENDOR,   508*1024,  16*1024,  EMMC_USER_PART},/* recovery_vendor 16M    p34*/
    {PART_DTS,               524*1024,  28*1024,  EMMC_USER_PART}, /* dtimage          28M  p35*/
    {PART_DTO,               552*1024,  4*1024,  EMMC_USER_PART}, /* dtoimage          4M   p36*/
    {PART_MODEM_FW,          556*1024,  96*1024,  EMMC_USER_PART}, /* modem_fw         96M  p37*/
    {PART_RECOVERY_VBMETA, 652*1024,  1*1024,   EMMC_USER_PART}, /* recovery_vbmeta   1M    p38*/
    {PART_ERECOVERY_VBMETA,653*1024,  1*1024,   EMMC_USER_PART}, /* erecovery_vbmeta  1M    p39*/
    {PART_ENG_VENDOR,      654*1024,  12*1024,   EMMC_USER_PART}, /* eng_vendor     12M     p40*/
    {PART_DFX,               666*1024,  16*1024,  EMMC_USER_PART}, /* dfx              16M  p41*/
    {PART_VBMETA,            682*1024,  4*1024,   EMMC_USER_PART}, /* PART_VBMETA      4M   p42*/
    {PART_CACHE,             686*1024,  128*1024, EMMC_USER_PART}, /* cache            128M p43*/
    {PART_ODM,               814*1024,  128*1024, EMMC_USER_PART}, /* odm              128M p44*/
    {PART_HISITEST0,         942*1024,  2*1024,   EMMC_USER_PART}, /* hisitest0        2M   p45*/
    {PART_HISITEST1,         944*1024,  2*1024,   EMMC_USER_PART}, /* hisitest1        2M   p46*/
    {PART_HISITEST2,         946*1024,  4*1024,   EMMC_USER_PART}, /* hisitest2        4M   p47*/
#if (defined(CONFIG_MARKET_OVERSEA) || defined(CONFIG_MARKET_INTERNAL) || defined(CONFIG_MARKET_16G_OVERSEA) || defined(CONFIG_MARKET_16G_INTERNAL))
    {PART_PATCH,             950*1024,  32*1024,  EMMC_USER_PART}, /* patch            32M  p48*/
    {PART_BOOTFAIL_INFO,     982*1024,  2*1024,   EMMC_USER_PART}, /* bootfail_info    2M   p49*/
    {PART_RRECORD,           984*1024,  16*1024,  EMMC_USER_PART}, /* rrecord          16M  p50*/
    {PART_RESERVED9,         1000*1024,  24*1024,  EMMC_USER_PART}, /* reserved9       24M  p51*/
#endif
#ifdef CONFIG_MARKET_OVERSEA
#ifdef CONFIG_PARTITION_ROMUPGRADE_HI6250
    {PART_SYSTEM,            1024*1024, 4688*1024,EMMC_USER_PART}, /* system           4688M p52*/
    {PART_CUST,              5712*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p53*/
    {PART_VERSION,           5904*1024, 32*1024,  EMMC_USER_PART}, /* version          32M   p54*/
    {PART_VENDOR,            5936*1024, 784*1024, EMMC_USER_PART}, /* vendor           784M  p55*/
    {PART_PRODUCT,           6720*1024, 192*1024, EMMC_USER_PART}, /* product          192M  p56*/
    {PART_USERDATA,          6912*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p57*/
#else
    {PART_SYSTEM,            1024*1024, 3536*1024,EMMC_USER_PART}, /* system           3536M p52*/
    {PART_CUST,              4560*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p53*/
    {PART_VERSION,           4752*1024, 32*1024,  EMMC_USER_PART}, /* version          32M   p54*/
    {PART_VENDOR,            4784*1024, 784*1024, EMMC_USER_PART}, /* vendor           784M  p55*/
    {PART_PRODUCT,           5568*1024, 192*1024, EMMC_USER_PART}, /* product          192M  p56*/
    {PART_USERDATA,          5760*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p57*/
#endif
#elif defined CONFIG_MARKET_INTERNAL
#ifdef CONFIG_PARTITION_ROMUPGRADE_HI6250
    {PART_SYSTEM,            1024*1024, 3552*1024,EMMC_USER_PART}, /* system           3552M p52*/
    {PART_CUST,              4576*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p53*/
    {PART_VERSION,           4768*1024, 32*1024,  EMMC_USER_PART}, /* version          32M   p54*/
    {PART_VENDOR,            4800*1024, 784*1024, EMMC_USER_PART}, /* vendor           784M  p55*/
    {PART_PRODUCT,           5584*1024, 192*1024, EMMC_USER_PART}, /* product          192M  p56*/
    {PART_USERDATA,          5776*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p57*/
#else
    {PART_SYSTEM,            1024*1024, 3088*1024,EMMC_USER_PART}, /* system           3088M p52*/
    {PART_CUST,              4112*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p53*/
    {PART_VERSION,           4304*1024, 32*1024,  EMMC_USER_PART}, /* version          32M   p54*/
    {PART_VENDOR,            4336*1024, 784*1024, EMMC_USER_PART}, /* vendor           784M  p55*/
    {PART_PRODUCT,           5120*1024, 192*1024, EMMC_USER_PART}, /* product          192M  p56*/
    {PART_USERDATA,          5312*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p57*/
#endif
#elif defined CONFIG_MARKET_16G_OVERSEA
    {PART_SYSTEM,            1024*1024, 3008*1024,EMMC_USER_PART}, /* system           3008M p52*/
    {PART_CUST,              4032*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p53*/
    {PART_VERSION,           4224*1024, 32*1024,  EMMC_USER_PART}, /* version          32M   p54*/
    {PART_VENDOR,            4256*1024, 784*1024, EMMC_USER_PART}, /* vendor           784M  p55*/
    {PART_PRODUCT,           5040*1024, 192*1024, EMMC_USER_PART}, /* product          192M  p56*/
    {PART_USERDATA,          5232*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p57*/
#elif defined CONFIG_MARKET_16G_INTERNAL
    {PART_SYSTEM,            1024*1024, 2720*1024,EMMC_USER_PART}, /* system           2720M p52*/
    {PART_CUST,              3744*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p53*/
    {PART_VERSION,           3936*1024, 32*1024,  EMMC_USER_PART}, /* version          32M   p54*/
    {PART_VENDOR,            3968*1024, 784*1024, EMMC_USER_PART}, /* vendor           784M  p55*/
    {PART_PRODUCT,           4752*1024, 192*1024, EMMC_USER_PART}, /* product          192M  p56*/
    {PART_USERDATA,          4944*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p57*/
#elif defined CONFIG_MARKET_BERLIN_OVERSEA
    {PART_PATCH,             950*1024,  32*1024,  EMMC_USER_PART}, /* patch             32M   p48*/
    {PART_BOOTFAIL_INFO,     982*1024,  2*1024,   EMMC_USER_PART}, /* bootfail_info     2M    p49*/
    {PART_RRECORD,           984*1024,  16*1024,  EMMC_USER_PART}, /* rrecord           16M   p50*/
    {PART_RESERVED9,         1000*1024,  8*1024,  EMMC_USER_PART}, /* reserved9         8M    p51*/
    {PART_SYSTEM,            1008*1024, 3008*1024,EMMC_USER_PART}, /* system            3008M p52*/
    {PART_CUST,              4016*1024, 192*1024, EMMC_USER_PART}, /* cust              192M  p53*/
    {PART_VERSION,           4208*1024, 32*1024,  EMMC_USER_PART}, /* version           24M   p54*/
    {PART_VENDOR,            4240*1024, 608*1024, EMMC_USER_PART}, /* vendor            608M  p55*/
    {PART_PRODUCT,           4848*1024, 192*1024, EMMC_USER_PART}, /* product           200M  p56*/
    {PART_USERDATA,          5040*1024, 4096*1024,EMMC_USER_PART}, /* userdata          4096M p57*/
#elif defined CONFIG_MARKET_BERLIN_INTERNAL
    {PART_PATCH,             950*1024,  32*1024,  EMMC_USER_PART}, /* patch             32M   p48*/
    {PART_BOOTFAIL_INFO,     982*1024,  2*1024,   EMMC_USER_PART}, /* bootfail_info     2M    p49*/
    {PART_RRECORD,           984*1024,  16*1024,  EMMC_USER_PART}, /* rrecord           16M   p50*/
    {PART_RESERVED9,         1000*1024,  8*1024,  EMMC_USER_PART}, /* reserved9         8M    p51*/
    {PART_SYSTEM,            1008*1024, 2688*1024,EMMC_USER_PART}, /* system            2688M p52*/
    {PART_CUST,              3696*1024, 192*1024, EMMC_USER_PART}, /* cust              192M  p53*/
    {PART_VERSION,           3888*1024, 32*1024,  EMMC_USER_PART}, /* version           32M   p54*/
    {PART_VENDOR,            3920*1024, 608*1024, EMMC_USER_PART}, /* vendor            608M  p55*/
    {PART_PRODUCT,           4528*1024, 192*1024, EMMC_USER_PART}, /* product           192M  p56*/
    {PART_USERDATA,          4720*1024, 4096*1024,EMMC_USER_PART}, /* userdata          4096M p57*/
#elif defined CONFIG_MARKET_FULL_OVERSEA
    {PART_PATCH,             950*1024,  32*1024,  EMMC_USER_PART}, /* patch             32M   p48*/
    {PART_BOOTFAIL_INFO,     982*1024,  2*1024,   EMMC_USER_PART}, /* bootfail_info     2M    p49*/
    {PART_RRECORD,           984*1024,  16*1024,  EMMC_USER_PART}, /* rrecord           16M   p50*/
    {PART_RESERVED9,         1000*1024, 8*1024,  EMMC_USER_PART}, /* reserved9          8M    p51*/
    {PART_SYSTEM,            1008*1024, 5632*1024,EMMC_USER_PART}, /* system            5632M p52*/
    {PART_CUST,              6640*1024, 192*1024, EMMC_USER_PART}, /* cust              192M  p53*/
    {PART_VERSION,           6832*1024, 32*1024,  EMMC_USER_PART}, /* version           32M   p54*/
    {PART_VENDOR,            6864*1024, 784*1024, EMMC_USER_PART}, /* vendor            784M  p55*/
    {PART_PRODUCT,           7648*1024, 192*1024, EMMC_USER_PART}, /* product           192M  p56*/
    {PART_USERDATA,          7840*1024, 4096*1024,EMMC_USER_PART}, /* userdata          4096M p57*/
#elif defined CONFIG_MARKET_FULL_INTERNAL
    {PART_PATCH,             950*1024,  32*1024,  EMMC_USER_PART}, /* patch             32M   p48*/
    {PART_BOOTFAIL_INFO,     982*1024,  2*1024,   EMMC_USER_PART}, /* bootfail_info     2M    p49*/
    {PART_RRECORD,           984*1024,  16*1024,  EMMC_USER_PART}, /* rrecord           16M   p50*/
    {PART_RESERVED9,         1000*1024,  8*1024,  EMMC_USER_PART}, /* reserved9         8M    p51*/
    {PART_SYSTEM,            1008*1024, 4752*1024,EMMC_USER_PART}, /* system            4752M p52*/
    {PART_CUST,              5760*1024, 192*1024, EMMC_USER_PART}, /* cust              192M  p53*/
    {PART_VERSION,           5952*1024, 32*1024,  EMMC_USER_PART}, /* version           32M   p54*/
    {PART_VENDOR,            5984*1024, 784*1024, EMMC_USER_PART}, /* vendor            784M  p55*/
    {PART_PRODUCT,           6768*1024, 192*1024, EMMC_USER_PART}, /* product           192M  p56*/
    {PART_USERDATA,          6960*1024, 4096*1024,EMMC_USER_PART}, /* userdata          4096M p57*/
#else
#ifdef CONFIG_VENDORIMAGE_FILE_SYSTEM_TYPE
    {PART_PATCH,             950*1024,  32*1024,  EMMC_USER_PART}, /* patch             32M   p48*/
    {PART_BOOTFAIL_INFO,     982*1024,  2*1024,   EMMC_USER_PART}, /* bootfail_info     2M    p49*/
    {PART_RRECORD,           984*1024,  16*1024,  EMMC_USER_PART}, /* rrecord           16M   p50*/
    {PART_RESERVED9,         1000*1024,  8*1024,  EMMC_USER_PART}, /* reserved9         8M    p51*/
    {PART_SYSTEM,            1008*1024, 1984*1024,EMMC_USER_PART}, /* system            1984M p52*/
    {PART_CUST,              2992*1024, 192*1024, EMMC_USER_PART}, /* cust              192M  p53*/
    {PART_VERSION,           3184*1024, 32*1024,  EMMC_USER_PART}, /* version           32M   p54*/
    {PART_VENDOR,            3216*1024, 608*1024, EMMC_USER_PART}, /* vendor            608M  p55*/
    {PART_PRODUCT,           3824*1024, 192*1024, EMMC_USER_PART}, /* product           192M  p56*/
    {PART_USERDATA,          4016*1024, 4096*1024,EMMC_USER_PART}, /* userdata          4096M p57*/
#else
    {PART_PATCH,             950*1024,  32*1024,  EMMC_USER_PART}, /* patch             32M   p48*/
    {PART_BOOTFAIL_INFO,     982*1024,  2*1024,   EMMC_USER_PART}, /* bootfail_info     2M    p49*/
    {PART_RRECORD,           984*1024,  16*1024,  EMMC_USER_PART}, /* rrecord           16M   p50*/
    {PART_RESERVED9,         1000*1024,  8*1024,  EMMC_USER_PART}, /* reserved9         8M    p51*/
    {PART_SYSTEM,            1008*1024, 1984*1024,EMMC_USER_PART}, /* system            1984M p52*/
    {PART_CUST,              2992*1024, 192*1024, EMMC_USER_PART}, /* cust              192M  p53*/
    {PART_VERSION,           3184*1024, 32*1024,  EMMC_USER_PART}, /* version           32M   p54*/
    {PART_VENDOR,            3216*1024, 608*1024, EMMC_USER_PART}, /* vendor            608M  p55*/
    {PART_PRODUCT,           3824*1024, 192*1024, EMMC_USER_PART}, /* product           192M  p56*/
    {PART_USERDATA,          4016*1024, 4096*1024,EMMC_USER_PART}, /* userdata          4096M p57*/
#endif
#endif
    {"0", 0, 0, 0},                                       /* */
};

#endif
