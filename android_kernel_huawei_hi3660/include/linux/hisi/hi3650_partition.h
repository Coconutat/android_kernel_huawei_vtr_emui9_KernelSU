#ifndef _HI3650_PARTITION_H_
#define _HI3650_PARTITION_H
_
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
    {PART_SPLASH,            80*1024,   8*1024,   EMMC_USER_PART}, /* splash           8M   p9*/
    {PART_MODEMNVM_BACKUP,   88*1024,   4*1024,   EMMC_USER_PART}, /* modemnvm backup  4M   p10*/
    {PART_MODEMNVM_IMG,      92*1024,   8*1024,   EMMC_USER_PART}, /* modemnvm img     8M   p11*/
    {PART_MODEMNVM_SYSTEM,   100*1024,  4*1024,   EMMC_USER_PART}, /* modemnvm system  4M   p12*/
    {PART_SECURE_STORAGE,    104*1024,  32*1024,  EMMC_USER_PART}, /* secure storage   32M  p13*/
    {PART_3RDMODEMNVM,       136*1024,  16*1024,  EMMC_USER_PART}, /* 3rdmodemnvm      16M  p14*/
    {PART_3RDMODEMNVMBKP,    152*1024,  16*1024,  EMMC_USER_PART}, /* 3rdmodemnvmback  16M  p15*/
    {PART_PERSIST,           168*1024,  2*1024,   EMMC_USER_PART}, /* persist          2M   p16*/
    {PART_RESERVED1,         170*1024,  14*1024,  EMMC_USER_PART}, /* reserved1        14M  p17*/
    {PART_MODEM_OM,          184*1024,  32*1024,  EMMC_USER_PART}, /* modem om         32M  p18*/
    {PART_SPLASH2,           216*1024,  64*1024,  EMMC_USER_PART}, /* splash2          64M  p19*/
    {PART_MISC,              280*1024,  2*1024,   EMMC_USER_PART}, /* misc             2M   p20*/
    {PART_MODEMNVM_UPDATE,   282*1024,  24*1024,  EMMC_USER_PART}, /* modemnvm update  24M  p21*/
    {PART_RECOVERY2,         306*1024,  64*1024,  EMMC_USER_PART}, /* recovery2        64M  p22*/
    {PART_RESERVED2,         370*1024,  64*1024,  EMMC_USER_PART}, /* reserved2        64M  p23*/
    {PART_TEEOS,             434*1024,  4*1024,   EMMC_USER_PART}, /* teeos            4M   p24*/
    {PART_TRUSTFIRMWARE,     438*1024,  2*1024,   EMMC_USER_PART}, /* trustfirmware    2M   p25*/
    {PART_SENSORHUB,         440*1024,  16*1024,  EMMC_USER_PART}, /* sensorhub        16M  p26*/
    {PART_FW_HIFI,           456*1024,  12*1024,  EMMC_USER_PART}, /* hifi             12M  p27*/
    {PART_BOOT,              468*1024,  32*1024,  EMMC_USER_PART}, /* boot             32M  p28*/
    {PART_RECOVERY,          500*1024,  64*1024,  EMMC_USER_PART}, /* recovery         64M  p29*/
    {PART_DTS,               564*1024,  64*1024,  EMMC_USER_PART}, /* dtimage          64M  p30*/
    {PART_MODEM,             628*1024,  64*1024,  EMMC_USER_PART}, /* modem            64M  p31*/
    {PART_MODEM_DSP,         692*1024,  16*1024,  EMMC_USER_PART}, /* modem_dsp        16M  p32*/
    {PART_MODEM_DTB,         708*1024,  8*1024,   EMMC_USER_PART}, /* modem_dtb        8M   p33*/
    {PART_DFX,               716*1024,  16*1024,  EMMC_USER_PART}, /* dfx              16M  p34*/
    {PART_3RDMODEM,          732*1024,  64*1024,  EMMC_USER_PART}, /* 3rdmodem         64M  p35*/
    {PART_CACHE,             796*1024,  256*1024, EMMC_USER_PART}, /* cache            256M p36*/
    {PART_HISITEST0,         1052*1024, 2*1024,   EMMC_USER_PART}, /* hisitest0        2M   p37*/
    {PART_HISITEST1,         1054*1024, 2*1024,   EMMC_USER_PART}, /* hisitest1        2M   p38*/
#ifdef CONFIG_VENDORIMAGE_FILE_SYSTEM_TYPE
    {PART_PATCH,             1056*1024,  32*1024,  EMMC_USER_PART}, /* patch            32M  p39*/
    {PART_BOOTFAIL_INFO,     1088*1024,  2*1024,   EMMC_USER_PART}, /* bootfail info    2M   p40*/
    {PART_RRECORD,           1090*1024,  16*1024,  EMMC_USER_PART}, /* rrecord          16M  p41*/
    {PART_RESERVED3,         1106*1024,  14*1024,  EMMC_USER_PART}, /* rrecord          14M  p42*/

#ifdef CONFIG_MARKET_OVERSEA
    {PART_SYSTEM,            1120*1024, 3008*1024,EMMC_USER_PART}, /* system           3008M p43*/
    {PART_CUST,              4128*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p44*/
    {PART_VERSION,           4320*1024, 32*1024,  EMMC_USER_PART}, /* version          32M    p45*/
    {PART_VENDOR,            4352*1024, 608*1024, EMMC_USER_PART}, /* vendor           608M  p46*/
    {PART_PRODUCT,           4960*1024, 192*1024, EMMC_USER_PART}, /* product          192M   p47*/
    {PART_HISITEST2,         5152*1024, 4*1024,   EMMC_USER_PART}, /* hisitest2        4M    p48*/
    {PART_USERDATA,          5156*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p49*/
#elif defined CONFIG_MARKET_INTERNAL
    {PART_SYSTEM,            1120*1024, 2688*1024,EMMC_USER_PART}, /* system           2688M p43*/
    {PART_CUST,              3808*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p44*/
    {PART_VERSION,           4000*1024, 32*1024,  EMMC_USER_PART}, /* version          32M    p45*/
    {PART_VENDOR,            4032*1024, 608*1024, EMMC_USER_PART}, /* vendor           608M  p46*/
    {PART_PRODUCT,           4640*1024, 192*1024, EMMC_USER_PART}, /* product          192M   p47*/
    {PART_HISITEST2,         4832*1024, 4*1024,   EMMC_USER_PART}, /* hisitest2        4M    p48*/
    {PART_USERDATA,          4836*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p49*/
#else
    {PART_SYSTEM,            1120*1024, 1984*1024,EMMC_USER_PART}, /* system           1984M p43*/
    {PART_CUST,              3104*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p44*/
    {PART_VERSION,           3296*1024, 32*1024,   EMMC_USER_PART}, /* version         32M    p45*/
    {PART_VENDOR,            3328*1024, 608*1024, EMMC_USER_PART}, /* vendor           608M  p46*/
    {PART_PRODUCT,           3936*1024, 192*1024,  EMMC_USER_PART}, /* product         192M   p47*/
    {PART_HISITEST2,         4128*1024, 4*1024,   EMMC_USER_PART}, /* hisitest2        4M    p48*/
    {PART_USERDATA,          4132*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p49*/
#endif
#else
#ifdef CONFIG_MARKET_OVERSEA
    {PART_SYSTEM,            1056*1024, 3584*1024,EMMC_USER_PART}, /* system           3584M p39*/
    {PART_CUST,              4640*1024, 512*1024, EMMC_USER_PART}, /* cust             512M  p40*/
    {PART_HISITEST2,         5152*1024, 4*1024,   EMMC_USER_PART}, /* hisitest2        4M    p41*/
    {PART_USERDATA,          5156*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p42*/
#elif defined CONFIG_MARKET_INTERNAL
    {PART_SYSTEM,            1056*1024, 3264*1024,EMMC_USER_PART}, /* system           3264M p39*/
    {PART_CUST,              4320*1024, 512*1024, EMMC_USER_PART}, /* cust             512M  p40*/
    {PART_HISITEST2,         4832*1024, 4*1024,   EMMC_USER_PART}, /* hisitest2        4M    p41*/
    {PART_USERDATA,          4836*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p42*/
#else
    {PART_SYSTEM,            1056*1024, 2560*1024,EMMC_USER_PART}, /* system           2560M p39*/
    {PART_CUST,              3616*1024, 512*1024, EMMC_USER_PART}, /* cust             512M  p40*/
    {PART_HISITEST2,         4128*1024, 4*1024,   EMMC_USER_PART}, /* hisitest2        4M    p41*/
    {PART_USERDATA,          4132*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p42*/
#endif
#endif
    {"0", 0, 0, 0},                                        /* total 8228M*/
};

#endif
