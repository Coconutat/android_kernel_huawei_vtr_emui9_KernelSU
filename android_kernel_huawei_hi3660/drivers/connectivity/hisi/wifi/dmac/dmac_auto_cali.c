


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_RF_CALI

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "dmac_auto_cali.h"
#include "hal_ext_if.h"
#include "dmac_main.h"
#include "dmac_config.h"
#include "dmac_device.h"


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_AUTO_CALI_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
oal_workqueue_stru  *g_auto_cali_workqueue;           /* 校准工作队列全局变量 */

/*****************************************************************************
  3 函数实现
*****************************************************************************/

oal_void dmac_rf_auto_cali(oal_work_stru *pst_work)
{
    mac_device_stru     *pst_mac_device;

    pst_mac_device = OAL_CONTAINER_OF(pst_work, mac_device_stru, auto_cali_work);

    //hal_rf_auto_cali(pst_mac_device->pst_device_stru);
    /* 停止PA和PHY的工作 */
    hal_disable_machw_phy_and_pa(pst_mac_device->pst_device_stru);

    /* 初始化MAC硬件 */
    hal_initialize_machw(pst_mac_device->pst_device_stru);

    /* 初始化PHY */
    hal_initialize_phy(pst_mac_device->pst_device_stru);

    /* 单芯片修改chip_id导致pcie定时获取id错误，需要关掉 */
#if (_PRE_TARGET_PRODUCT_TYPE_E5 == _PRE_CONFIG_TARGET_PRODUCT)
    #ifdef _PRE_DEBUG_MODE
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_mac_device->st_exception_report_timer));
    #endif
#endif

    /* 初始化RF系统 */
    hal_rf_auto_cali(pst_mac_device->pst_device_stru);

    /* 使能pa */
    hal_enable_machw_phy_and_pa(pst_mac_device->pst_device_stru);

    /* 打开定时器 */
#if (_PRE_TARGET_PRODUCT_TYPE_E5 == _PRE_CONFIG_TARGET_PRODUCT)
    #ifdef _PRE_DEBUG_MODE
        FRW_TIMER_CREATE_TIMER(&(pst_mac_device->st_exception_report_timer),
                        dmac_device_exception_report_timeout_fn,
                        MAC_EXCEPTION_TIME_OUT,
                        pst_mac_device->pst_device_stru,
                        OAL_TRUE,
                        OAM_MODULE_ID_MAC,
                        pst_mac_device->ul_core_id);
    #endif
#endif

}


oal_uint32 dmac_config_auto_cali(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru     *pst_device = OAL_PTR_NULL;

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "dmac_config_auto_cali::pst_dmac_vap null.\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);

    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "dmac_config_auto_cali::pst_device null.\n");

        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_INIT_WORK(&pst_device->auto_cali_work, dmac_rf_auto_cali);

    oal_queue_work(g_auto_cali_workqueue, &(pst_device->auto_cali_work));

    return OAL_SUCC;
}


oal_uint32  dmac_auto_cali_init(oal_void)
{
    g_auto_cali_workqueue = OAL_CREATE_SINGLETHREAD_WORKQUEUE("auto_cali_queue");

    if (OAL_PTR_NULL == g_auto_cali_workqueue)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CALIBRATE, "dmac_auto_cali_init: create auto cali workqueue failed.\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    return OAL_SUCC;
}


oal_uint32  dmac_auto_cali_exit(oal_void)
{
    /* 删除工作队列 */
    oal_destroy_workqueue(g_auto_cali_workqueue);

    return OAL_SUCC;
}

/*lint -e578*//*lint -e19*/
oal_module_license("GPL");
/*lint +e578*//*lint +e19*/

#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


