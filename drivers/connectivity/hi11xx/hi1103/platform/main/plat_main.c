

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#define HISI_LOG_TAG "[plat_init_etc]"
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "plat_main.h"
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "board.h"
#include "oneimage.h"
#include "plat_pm.h"
#include "hw_bfg_ps.h"
#include "oal_kernel_file.h"
#include "hisi_ini.h"
#endif

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
#include "oal_pcie_linux.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_PLAT_MAIN_C

extern int plat_exception_reset_init_etc(void);
extern int plat_exception_reset_exit_etc(void);
extern int low_power_init_etc(void);
extern void  low_power_exit_etc(void);
extern int hw_ps_init_etc(void);
extern void hw_ps_exit_etc(void);

#ifdef CONFIG_HI110X_GPS_REFCLK
extern int hi_gps_plat_init_etc(void);
extern void hi_gps_plat_exit_etc(void);
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
extern int hw_misc_connectivity_init_etc(void);
extern void hw_misc_connectivity_exit_etc(void);
#endif

#ifdef CONFIG_HI1102_PLAT_HW_CHR
extern int chr_miscdevs_init_etc(void);
extern void chr_miscdevs_exit_etc(void);
#endif


oal_int32  plat_init_etc(oal_void)
{
    oal_int32   l_return   = OAL_FAIL;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef HI110X_DRV_VERSION
    OAL_IO_PRINT("HI110X_DRV_VERSION: %s\r\n", HI110X_DRV_VERSION);
    OAL_IO_PRINT("HI110X_DRV compileTime: %s, %s\r\n", __DATE__,__TIME__);
#endif
    if(false == is_my_chip_etc())
    {
        return OAL_SUCC;
    }
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    l_return = hi110x_board_init_etc();
    if (OAL_SUCC != l_return)
    {
        OAL_IO_PRINT("plat_init_etc: hi110x_board_init_etc fail\r\n");
        goto board_init_fail;
    }
#endif

#ifdef CONFIG_HI1102_PLAT_HW_CHR
    l_return = chr_miscdevs_init_etc();
    if (OAL_SUCC != l_return)
    {
        OAL_IO_PRINT("plat_init_etc: chr_miscdev_init return error code: %d\r\n", l_return);

        goto chr_miscdevs_init_fail;
    }
#endif

    l_return = plat_exception_reset_init_etc();
    if (OAL_SUCC != l_return)
    {
        OAL_IO_PRINT("plat_init_etc: plat_exception_reset_init_etc fail\r\n");
        goto plat_exception_rst_init_fail;
    }

    l_return= oal_wifi_platform_load_dev();
    if(OAL_SUCC != l_return)
    {
        OAL_IO_PRINT("oal_wifi_platform_load_dev fail:%d\r\n",l_return);
        goto wifi_load_sdio_fail;
    }
#ifdef CONFIG_HWCONNECTIVITY
    l_return = hw_misc_connectivity_init_etc();
    if (OAL_SUCC != l_return)
    {
        OAL_IO_PRINT("hw_misc_connectivity_init_etc fail:%d\r\n",l_return);
        goto hw_misc_connectivity_init_fail;
    }
#endif
    l_return = oal_main_init_etc();
    if (OAL_SUCC != l_return)
    {
        OAL_IO_PRINT("plat_init_etc: oal_main_init_etc return error code: %d\r\n", l_return);
        goto oal_main_init_fail;
    }

    l_return = oam_main_init_etc();
    if (OAL_SUCC != l_return)
    {
        OAL_IO_PRINT("plat_init_etc: oam_main_init_etc return error code: %d\r\n", l_return);
        goto oam_main_init_fail;
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    l_return = sdt_drv_main_init_etc();
    if (OAL_SUCC != l_return)
    {
        OAL_IO_PRINT("plat_init_etc: sdt_drv_main_init_etc return error code: %d\r\n", l_return);
        goto sdt_drv_main_init_fail;
    }
#endif

    l_return = frw_main_init_etc();
    if (OAL_SUCC != l_return)
    {
        OAL_IO_PRINT("plat_init_etc: frw_main_init_etc return error code: %d\r\n", l_return);
        goto frw_main_init_fail;
    }

#if defined(_PRE_PLAT_FEATURE_HI110X_PCIE) && defined(CONFIG_ARCH_SD56XX)
    OAL_IO_PRINT("plat_init_etc:: platform_main_init finish!\r\n");
    return OAL_SUCC;
#endif

    l_return = low_power_init_etc();
    if (OAL_SUCC != l_return)
    {
        OAL_IO_PRINT("plat_init_etc: low_power_init_etc return error code: %d\r\n", l_return);
        goto low_power_init_fail;
    }

    l_return = hw_ps_init_etc();
    if (OAL_SUCC != l_return)
    {
        OAL_IO_PRINT("plat_init_etc: hw_ps_init_etc return error code: %d\r\n", l_return);
        goto hw_ps_init_fail;
    }

#ifdef CONFIG_HI110X_GPS_REFCLK
    l_return = hi_gps_plat_init_etc();
    if (OAL_SUCC != l_return)
    {
        OAL_IO_PRINT("plat_init_etc: hi_gps_plat_init_etc fail\r\n");
        goto gps_plat_init_fail;
    }
#endif

#ifdef CONFIG_HI110X_GPS_SYNC
    l_return = gnss_sync_init();
    if (OAL_SUCC != l_return)
    {
        OAL_IO_PRINT("plat_init_etc: gnss_sync_init fail\r\n");
        goto gps_sync_init_fail;
    }
#endif

#ifdef HAVE_HISI_NFC
        /* 读取nfc低电log数据,然后下电，若此前初始化异常，不会倒出NFC低电log*/
        save_nfc_lowpower_log_etc();
#endif

    /*启动完成后，输出打印*/
    OAL_IO_PRINT("plat_init_etc:: platform_main_init finish!\r\n");

    return OAL_SUCC;

#ifdef CONFIG_HI110X_GPS_SYNC
gps_sync_init_fail:
    hi_gps_plat_exit_etc();
#endif
#ifdef CONFIG_HI110X_GPS_REFCLK
gps_plat_init_fail:
    hw_ps_exit_etc();
#endif
hw_ps_init_fail:
    low_power_exit_etc();
low_power_init_fail:
    frw_main_exit_etc();
frw_main_init_fail:
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    sdt_drv_main_exit_etc();
sdt_drv_main_init_fail:
#endif
    oam_main_exit_etc();
oam_main_init_fail:
    oal_main_exit_etc();
oal_main_init_fail:
    /*异常关闭电源*/
    #ifdef HAVE_HISI_NFC
    hi_wlan_power_off_etc();
    #endif
#ifdef CONFIG_HWCONNECTIVITY
    hw_misc_connectivity_exit_etc();
hw_misc_connectivity_init_fail:
#endif
    oal_wifi_platform_unload_dev();
wifi_load_sdio_fail:
    plat_exception_reset_exit_etc();
plat_exception_rst_init_fail:

#ifdef CONFIG_HI1102_PLAT_HW_CHR
    chr_miscdevs_exit_etc();
chr_miscdevs_init_fail:
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
     hi110x_board_exit_etc();
#endif
board_init_fail:

          CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_SDIO_INIT);
    
    return l_return;
}


oal_void plat_exit_etc(oal_void)
{
#ifdef CONFIG_HI110X_GPS_SYNC
    gnss_sync_exit();
#endif
#ifdef CONFIG_HI110X_GPS_REFCLK
    hi_gps_plat_exit_etc();
#endif

    hw_ps_exit_etc();
    low_power_exit_etc();
    frw_main_exit_etc();

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    sdt_drv_main_exit_etc();
#endif

    oam_main_exit_etc();

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef CONFIG_HWCONNECTIVITY
    hw_misc_connectivity_exit_etc();
#endif
#endif

    oal_main_exit_etc();

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    hi110x_board_exit_etc();
#endif

#ifdef CONFIG_HI1102_PLAT_HW_CHR
    chr_miscdevs_exit_etc();
#endif

    oal_wifi_platform_unload_dev();

    plat_exception_reset_exit_etc();
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    ini_cfg_exit_etc();
#endif
    return;
}

/*lint -e578*//*lint -e19*/
#if defined(_PRE_PRODUCT_ID_HI110X_HOST) && !defined(CONFIG_HI110X_KERNEL_MODULES_BUILD_SUPPORT) && defined(_PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT)
oal_int32 g_plat_init_flag_etc = 0;
oal_int32 g_plat_init_ret_etc;
/*built-in*/
OAL_STATIC ssize_t  plat_sysfs_set_init(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    char            mode[128] = {0};

    if (NULL == buf)
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == attr)
    {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == dev)
    {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if ((sscanf(buf, "%20s", mode) != 1))
    {
        OAL_IO_PRINT("set value one param!\n");
        return -OAL_EINVAL;
    }

    if(sysfs_streq("init", mode))
    {
        /*init*/
        if(0 == g_plat_init_flag_etc)
        {
            g_plat_init_ret_etc = plat_init_etc();
            g_plat_init_flag_etc = 1;
        }
        else
        {
            OAL_IO_PRINT("double init!\n");
        }
    }
    else
    {
        OAL_IO_PRINT("invalid input:%s\n",mode);
    }

    return count;
}

OAL_STATIC ssize_t  plat_sysfs_get_init(struct device *dev, struct device_attribute *attr, char*buf)
{
    int ret = 0;

    if (NULL == buf)
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == attr)
    {
        OAL_IO_PRINT("attr is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if (NULL == dev)
    {
        OAL_IO_PRINT("dev is null r failed!%s\n",__FUNCTION__);
        return 0;
    }

    if(1 == g_plat_init_flag_etc)
    {
        if(OAL_SUCC == g_plat_init_ret_etc)
        {
            ret +=  snprintf(buf + ret , PAGE_SIZE-ret, "running\n");
        }
        else
        {
            ret +=  snprintf(buf + ret , PAGE_SIZE-ret, "boot failed ret=%d\n", g_plat_init_ret_etc);
        }
    }
    else
    {
        ret +=  snprintf(buf + ret , PAGE_SIZE-ret, "uninit\n");
    }

    return ret;
}
OAL_STATIC DEVICE_ATTR(plat, S_IRUGO | S_IWUSR, plat_sysfs_get_init, plat_sysfs_set_init);
OAL_STATIC struct attribute *plat_init_sysfs_entries[] = {
        &dev_attr_plat.attr,
        NULL
};

OAL_STATIC struct attribute_group plat_init_attribute_group = {
        .attrs = plat_init_sysfs_entries,
};

oal_int32  plat_sysfs_init_etc(oal_void)
{
    oal_int32 ret = 0;
    oal_uint32 ul_rslt;
    oal_kobject*     pst_root_boot_object = NULL;

    if(false == is_hisi_chiptype_etc(BOARD_VERSION_HI1103))
    {
        return OAL_SUCC;
    }

    /*110X 驱动build in，内存池初始化上移到内核完成，保证大片内存申请成功*/
    ul_rslt = oal_mem_init_pool_etc();
    if (ul_rslt != OAL_SUCC)
    {
        OAL_IO_PRINT("oal_main_init_etc: oal_mem_init_pool_etc return error code: %d", ul_rslt);
        return -OAL_EFAIL;
    }

    OAL_IO_PRINT("mem pool init succ\n");

    pst_root_boot_object = oal_get_sysfs_root_boot_object_etc();
    if(NULL == pst_root_boot_object)
    {
        OAL_IO_PRINT("[E]get root boot sysfs object failed!\n");
        return -OAL_EBUSY;
    }

    ret = sysfs_create_group(pst_root_boot_object,&plat_init_attribute_group);
    if (ret)
    {
        OAL_IO_PRINT("sysfs create plat boot group fail.ret=%d\n",ret);
        ret = -OAL_ENOMEM;
        return ret;
    }

    return ret;
}

oal_void  plat_sysfs_exit_etc(oal_void)
{
    /*need't exit,built-in*/
    return;
}
oal_module_init(plat_sysfs_init_etc);
oal_module_exit(plat_sysfs_exit_etc);
#else
//#ifdef CONFIG_HI1102_PLATFORM_MODULE
oal_module_init(plat_init_etc);
oal_module_exit(plat_exit_etc);
#endif





#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

