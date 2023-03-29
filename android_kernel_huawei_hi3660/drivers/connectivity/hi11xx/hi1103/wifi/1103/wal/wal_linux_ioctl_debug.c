


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#ifdef _PRE_WLAN_CFGID_DEBUG

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_profiling.h"
#include "oal_kernel_file.h"
#include "oal_cfg80211.h"

#include "oam_ext_if.h"
#include "frw_ext_if.h"

#include "wlan_spec.h"
#include "wlan_types.h"

#include "mac_vap.h"
#include "mac_resource.h"
#include "mac_regdomain.h"
#include "mac_ie.h"

#include "hmac_ext_if.h"
#include "hmac_chan_mgmt.h"

#include "wal_main.h"
#include "wal_ext_if.h"
#include "wal_config.h"
#include "wal_regdb.h"
#include "wal_linux_scan.h"
#include "wal_linux_ioctl.h"
#include "wal_linux_bridge.h"
#include "wal_linux_flowctl.h"
#include "wal_linux_atcmdsrv.h"
#include "wal_linux_event.h"
#include "hmac_resource.h"
#include "hmac_p2p.h"

#ifdef _PRE_WLAN_FEATURE_P2P
#include "wal_linux_cfg80211.h"
#endif

#include "wal_dfx.h"


#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#include "oal_hcc_host_if.h"
#include "plat_cali.h"
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/notifier.h>
#include <linux/inetdevice.h>
#include <net/addrconf.h>
#endif
#include "hmac_arp_offload.h"
#endif


#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
#include "hmac_auto_adjust_freq.h"
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM
#include "hmac_roam_main.h"
#endif //_PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

//#include "dmac_alg_if.h"
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#endif

#ifdef _PRE_MEM_TRACE
#include "mem_trace.h"
#endif

#if defined (WIN32) && (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
#include "hisi_customize_wifi_hi110x.h"
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "board.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_IOCTL_DEBUG_C
#define MAX_PRIV_CMD_SIZE   4096

/*****************************************************************************
  2 结构体定义
*****************************************************************************/

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#ifdef _PRE_WLAN_FEATURE_DFR
extern  hmac_dfr_info_stru    g_st_dfr_info_etc;
#endif //_PRE_WLAN_FEATURE_DFR
extern OAL_CONST oal_int8 * pauc_tx_dscr_param_name_etc[];
extern OAL_CONST oal_int8 * pauc_tx_pow_param_name[];
extern OAL_CONST wal_ioctl_alg_cfg_stru g_ast_alg_cfg_map_etc[];
extern OAL_CONST wal_ioctl_dyn_cali_stru g_ast_dyn_cali_cfg_map[];
extern OAL_CONST wal_ioctl_tlv_stru   g_ast_set_tlv_table[];
/*****************************************************************************
  3 函数实现
*****************************************************************************/

OAL_STATIC oal_uint32  wal_hipriv_global_log_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_int32                   l_switch_val;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;

    /* 获取开关状态值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_global_log_switch::error code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    if ((0 != oal_strcmp("0", ac_name)) && (0 != oal_strcmp("1", ac_name)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_global_log_switch::invalid switch value}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    l_switch_val = oal_atoi(ac_name);

    return oam_log_set_global_switch_etc((oal_switch_enum_uint8)l_switch_val);
}


OAL_STATIC oal_uint32  wal_hipriv_vap_log_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    mac_vap_stru               *pst_mac_vap;
    oal_int32                   l_switch_val;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_vap_log_switch::null pointer.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取开关状态值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_vap_log_switch::error code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    if ((0 != oal_strcmp("0", ac_name)) && (0 != oal_strcmp("1", ac_name)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_vap_log_switch::invalid switch value}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    l_switch_val = oal_atoi(ac_name);

    return oam_log_set_vap_switch_etc(pst_mac_vap->uc_vap_id, (oal_switch_enum_uint8)l_switch_val);
}


OAL_STATIC oal_uint32  wal_hipriv_feature_log_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    mac_vap_stru                       *pst_mac_vap;
    oam_feature_enum_uint8              en_feature_id;
    oal_uint8                           uc_switch_vl;
    oal_uint32                          ul_off_set;
    oal_int8                            ac_param[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                          ul_ret;
    oam_log_level_enum_uint8            en_log_lvl;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        wal_msg_write_stru          st_write_msg;
#endif

    /* OAM log模块的开关的命令: hipriv "Hisilicon0[vapx] feature_log_switch {feature_name} {0/1}"
       1-2(error与warning)级别日志以vap级别为维度；
    */

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_feature_log_switch::null pointer.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取特性名称 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_param, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 提供特性名的帮助信息 */
    if ('?' == ac_param[0])
    {
        OAL_IO_PRINT("please input abbr feature name. \r\n");
        oam_show_feature_list_etc();
        return OAL_SUCC;
    }

    /* 获取特性ID */
    ul_ret = oam_get_feature_id_etc((oal_uint8 *)ac_param, &en_feature_id);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_feature_log_switch::invalid feature name}\r\n");
        return ul_ret;
    }

    /* 获取开关值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_param, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 获取INFO级别开关状态 */
    if ((0 != oal_strcmp("0", ac_param)) && (0 != oal_strcmp("1", ac_param)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_feature_log_switch::invalid switch value}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    uc_switch_vl = (oal_uint8)oal_atoi(ac_param);

     /* 关闭INFO日志级别时，恢复成默认的日志级别 */
    en_log_lvl = (OAL_SWITCH_ON == uc_switch_vl) ? OAM_LOG_LEVEL_INFO : OAM_LOG_DEFAULT_LEVEL;
    ul_ret = oam_log_set_feature_level_etc(pst_mac_vap->uc_vap_id, en_feature_id, en_log_lvl) ;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_FEATURE_LOG, OAL_SIZEOF(oal_int32));
    *((oal_uint16 *)(st_write_msg.auc_value)) = ((en_feature_id<<8) | en_log_lvl);
    ul_ret |= (oal_uint32)wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_feature_log_switch::return err code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
#endif

    return ul_ret;

}


OAL_STATIC oal_uint32  wal_hipriv_log_ratelimit(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oam_ratelimit_stru                  st_ratelimit;
    oam_ratelimit_type_enum_uint8       en_ratelimit_type;
    oal_uint32                          ul_off_set;
    oal_int8                            ac_param[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                          ul_ret;

    /* OAM log printk流控配置命令: hipriv "Hisilicon0[vapx] {log_ratelimit} {printk(0)/sdt(1)}{switch(0/1)} {interval} {burst}" */

    st_ratelimit.en_ratelimit_switch    = OAL_SWITCH_OFF;
    st_ratelimit.ul_interval            = OAM_RATELIMIT_DEFAULT_INTERVAL;
    st_ratelimit.ul_burst               = OAM_RATELIMIT_DEFAULT_BURST;

    /* 获取限速类型 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_param, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }
    pc_param += ul_off_set;

    en_ratelimit_type =  (oam_ratelimit_type_enum_uint8)oal_atoi(ac_param);

    /* 获取开关状态 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_param, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }
    pc_param += ul_off_set;

    st_ratelimit.en_ratelimit_switch =  (oal_switch_enum_uint8)oal_atoi(ac_param);

    if (OAL_SWITCH_ON == st_ratelimit.en_ratelimit_switch)
    {
        /* 获取interval值 */
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_param, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            return ul_ret;
        }
        pc_param += ul_off_set;

        st_ratelimit.ul_interval = (oal_uint32)oal_atoi(ac_param);

        /* 获取burst值 */
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_param, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            return ul_ret;
        }
        pc_param += ul_off_set;

        st_ratelimit.ul_burst = (oal_uint32)oal_atoi(ac_param);
    }

    return oam_log_set_ratelimit_param_etc(en_ratelimit_type, &st_ratelimit);
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32  wal_hipriv_log_lowpower(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    /* OAM event模块的开关的命令: hipriv "Hisilicon0 log_pm 0 | 1"
        此处将解析出"1"或"0"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_log_lowpower::wal_get_cmd_one_arg_etc return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对event模块进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else if (0 == (oal_strcmp("2", ac_name)))
    {
        l_tmp = 2;
    }
    else if (0 == (oal_strcmp("3", ac_name)))
    {
        l_tmp = 3;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_log_lowpower::the log switch command is error [%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_LOG_PM, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_event_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_pm_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    /* OAM event模块的开关的命令: hipriv "Hisilicon0 wal_hipriv_pm_switch 0 | 1"
        此处将解析出"1"或"0"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pm_switch::wal_get_cmd_one_arg_etc return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对event模块进行不同的设置 */
    l_tmp = (oal_uint8)oal_atoi(ac_name);

    if (l_tmp < 0 || l_tmp > 5)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pm_switch::the log switch command is error [%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_PM_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pm_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_hipriv_power_test(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    /* OAM event模块的开关的命令: hipriv "Hisilicon0 wal_hipriv_power_test 0 | 1"
        此处将解析出"1"或"0"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pm_switch::wal_get_cmd_one_arg_etc return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对event模块进行不同的设置 */
    l_tmp = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_POWER_TEST, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pm_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}

#endif




extern oal_bool_enum_uint8 g_en_tas_switch_en;
OAL_STATIC oal_uint32  wal_hipriv_set_tx_pow_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    mac_cfg_set_tx_pow_param_stru   *pst_set_tx_pow_param;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret = OAL_SUCC;
    wal_dscr_param_enum_uint8        en_param_index;
    oal_uint8                        uc_value = 0;
#ifdef _PRE_WLAN_FEATURE_TPC_OPT
    oal_uint8                        auc_sar_ctrl_params[CUS_NUM_OF_SAR_PARAMS];
    oal_uint8                       *puc_sar_ctrl_params;
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    mac_device_stru                 *pst_mac_device;
    mac_cfg_tas_pwr_ctrl_stru        st_tas_pow_ctrl_params;
#endif
#endif

    /* 命令格式: hipriv "vap0 set_tx_pow rf_reg_ctl 0/1" ,   0:不使能, 1:使能            */
    /* 命令格式: hipriv "vap0 set_tx_pow fix_level 0/1/2/3"  设置数据帧功率等级, 仅data0 */
    /* 命令格式: hipriv "vap0 set_tx_pow mag_level 0/1/2/3"  设置管理帧功率等级          */
    /* 命令格式: hipriv "vap0 set_tx_pow ctl_level 0/1/2/3"  设置控制帧功率等级          */
    /* 命令格式: hipriv "vap0 set_tx_pow amend <value>"      修正upc code                */
    /* 命令格式: hipriv "vap0 set_tx_pow no_margin"          功率不留余量设置, 仅51用    */
    /* 命令格式: hipriv "vap0 set_tx_pow show_log"           显示功率表日志              */
    /* 命令格式: hipriv "vap0 set_tx_pow sar_level 0/1/2/3"  设置降sar等级               */
    /* 命令格式: hipriv "vap0 set_tx_pow tas_pwr_ctrl 0/1 0/1" tas功率控制              */

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_TX_POW, OAL_SIZEOF(mac_cfg_set_tx_pow_param_stru));

    /* 解析并设置配置命令参数 */
    pst_set_tx_pow_param = (mac_cfg_set_tx_pow_param_stru *)(st_write_msg.auc_value);
    OAL_MEMZERO(pst_set_tx_pow_param, OAL_SIZEOF(mac_cfg_set_tx_pow_param_stru));

    /* 获取描述符字段设置命令字符串 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_tx_pow_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 解析是设置哪一个字段 */
    for (en_param_index = 0; en_param_index < WAL_TX_POW_PARAM_BUTT; en_param_index++)
    {
        if(!oal_strcmp(pauc_tx_pow_param_name[en_param_index], ac_arg))
        {
            break;
        }
    }

    /* 检查命令是否打错 */
    if (WAL_TX_POW_PARAM_BUTT == en_param_index)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_tx_pow_param::no such param for tx pow!}\r\n");
        return OAL_FAIL;
    }

    pst_set_tx_pow_param->en_type = en_param_index;

    /* 获取下一个参数 */
    pc_param += ul_off_set;
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_tx_pow_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
    }
    else
    {
        uc_value = (oal_uint8)oal_atoi(ac_arg);
    }
    pst_set_tx_pow_param->auc_value[0] = uc_value;

    /*lint -e571 */
    /* 参数校验 */
    switch(en_param_index)
    {
        case WAL_TX_POW_PARAM_SET_RF_REG_CTL:
        case WAL_TX_POW_PARAM_SET_NO_MARGIN:
            if (uc_value >= 2)
            {
                //OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_tx_pow_param::input val out of range [%d]!}\r\n", uc_value);
                return OAL_ERR_CODE_INVALID_CONFIG;
            }
            break;
        case WAL_TX_POW_PARAM_SET_FIX_LEVEL:
        case WAL_TX_POW_PARAM_SET_MAG_LEVEL:
        case WAL_TX_POW_PARAM_SET_CTL_LEVEL:
            if (uc_value > 4)
            {
                //OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_tx_pow_param::input pow level val out of range [%d]!}\r\n", uc_value);
                return OAL_ERR_CODE_INVALID_CONFIG;
            }
            break;
        case WAL_TX_POW_PARAM_SET_SHOW_LOG:
        case WAL_TX_POW_PARAM_SET_AMEND:
            break;
        case WAL_TX_POW_PARAM_SET_SAR_LEVEL:
            OAM_WARNING_LOG1(0, OAM_SF_TPC, "{wal_hipriv_set_tx_pow_param::input reduce SAR level [%d]!}\r\n", uc_value);
        #ifdef _PRE_WLAN_FEATURE_TPC_OPT
            if ((uc_value > 3) || (uc_value == 0))
            {
                OAM_ERROR_LOG0(0, OAM_SF_TPC, "{wal_hipriv_set_tx_pow_param::reduce SAR STOP!}\r\n");
                oal_memset(auc_sar_ctrl_params, 0xFF, OAL_SIZEOF(auc_sar_ctrl_params));
                puc_sar_ctrl_params = auc_sar_ctrl_params;
            }
            else
            {
                puc_sar_ctrl_params = wal_get_reduce_sar_ctrl_params(uc_value);
            }
            if (puc_sar_ctrl_params)
            {
                oal_memcopy(pst_set_tx_pow_param->auc_value, puc_sar_ctrl_params, OAL_SIZEOF(auc_sar_ctrl_params));
            }
        #endif
            break;

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
        case WAL_TX_POW_PARAM_TAS_POW_CTRL:
            pst_mac_device = mac_res_get_dev_etc(0);
            /* 如果非单VAP,则不处理 */
            if (mac_device_calc_up_vap_num_etc(pst_mac_device) > 1)
            {
                OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_tx_pow_param::WAL_TX_POW_PARAM_TAS_POW_CTRL more than 1 vap");
                return OAL_FAIL;
            }

            st_tas_pow_ctrl_params.uc_core_idx = uc_value;
            /* 获取下一个参数 */
            pc_param += ul_off_set;
            ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_tx_pow_param::TAS pwr ctrl core or needimproved lost return err_code [%d]!}\r\n");
                return OAL_ERR_CODE_INVALID_CONFIG;
            }
            st_tas_pow_ctrl_params.en_need_improved = (oal_uint8)oal_atoi(ac_arg);
            oal_memcopy(pst_set_tx_pow_param->auc_value, &st_tas_pow_ctrl_params, OAL_SIZEOF(mac_cfg_tas_pwr_ctrl_stru));
            OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_hipriv_set_tx_pow_param::WAL_TX_POW_PARAM_TAS_POW_CTRL core[%d] improved_flag[%d]!}\r\n",
                             st_tas_pow_ctrl_params.uc_core_idx, st_tas_pow_ctrl_params.en_need_improved);
            break;

        case WAL_TX_POW_PARAM_TAS_RSSI_MEASURE:
            if (WLAN_RF_CHANNEL_ONE == uc_value)
            {
                /* 当前TAS方案只支持天线0测量 */
                OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_hipriv_set_tx_pow_param::WAL_TX_POW_PARAM_TAS_RSSI_MEASURE core[%d] is not supported!", uc_value);
                return OAL_FAIL;
            }
            break;
        case WAL_TX_POW_PARAM_TAS_ANT_SWITCH:
            if (OAL_TRUE == g_en_tas_switch_en)
            {
                /* 0:默认态 1:tas态 */
                OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_tx_pow_param::CMD_SET_MEMO_CHANGE antIndex[%d].}", uc_value);
                l_ret = board_wifi_tas_set(uc_value);
            }
            return (oal_uint32)l_ret;
#endif

        default:
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_tx_pow_param::error input type!}\r\n");
            //OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_hipriv_set_tx_pow_param::input type[%d], val[%d]!}\r\n", en_param_index, uc_value);
            return OAL_FAIL;
    }
    /*lint +e571 */


    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_tx_pow_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_tx_pow_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_ucast_data_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_set_dscr_param_stru     *pst_set_dscr_param;
    wal_dscr_param_enum_uint8        en_param_index;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];


    /* 解析并设置配置命令参数 */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /* 获取描述符字段设置命令字符串 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_data_dscr_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 解析是设置哪一个字段 */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++)
    {
        if(!oal_strcmp(pauc_tx_dscr_param_name_etc[en_param_index], ac_arg))
        {
            break;
        }
    }

    /* 检查命令是否打错 */
    if (WAL_DSCR_PARAM_BUTT == en_param_index)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_data_dscr_param::CMD ERR!cmd type::wlan0 set_ucast_data CMD VALUE!}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->ta: set tx rts antenna![0] or [1]}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->ra: set rx cts/ack/ba antenna![0] or [1]}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->cc: set channel code![0][BCC] or [1][LDPC]}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->data0: set tx dscr rate0 4 bytes!}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->data1: set tx dscr rate1 4 bytes!}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->data2: set tx dscr rate2 4 bytes!}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->data3: set tx dscr rate3 4 bytes!}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->power: set lpf/pa/upc/dac!}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->shortgi: set tx dscr short gi or long gi![0] or [1] }");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->preamble: set tx dscr preamble mode![0] or [1]}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->rtscts: set tx dscr enable rts or not![0] or [1]}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->lsigtxop: set tx dscr enable lsigtxop or not![0] or [1]}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->smooth: set rx channel matrix with smooth or not![0] or [1]}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->snding: set sounding mode![0][NON]-[1][NDP]-[2][STAGGERD]-[3][LEGACY]}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->txbf: set txbf mode![0][NON]-[1][EXPLICIT]-[2][LEGACY]}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->stbc: set STBC or not![0] or [1]}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->rd_ess: expand spatial stream}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->dyn_bw: set rts/cts dynamic signaling or not![0][STATIC]-[1][DYNAMIC]}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->dyn_bw_exist: set tx dscr dynamic when no ht exist!set[1]] }");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->ch_bw_exist: set tx dscr dynamic when no ht exist!set[1]}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->rate: set 11a/b/g rate as rate table}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->mcs: set 11n rate as rate table mcs index}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->mcsac: set 11ac rate as rate table mcs index}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->nss: set 11ac nss mode [1][SINGLE]-[2][DOUBLE]-[3][TRIBLE]-[4][QUAD]}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->bw: set tx dscr channel bw![20][20M]-[40][40M]-[d40][40MDUP]-[80][80M]-[d80][80MDUP]-[160][160M]-[d160][160MDUP]-[80_80][80+80M]}");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD-->txchain: set tx dscr txchain![1]CH0-[2]CH1-[3]DOUBLE}");
        return OAL_FAIL;
    }

    pst_set_dscr_param->uc_function_index = en_param_index;

    /*配置速率、空间流数、带宽*/
    if(en_param_index >= WAL_DSCR_PARAM_RATE && en_param_index <= WAL_DSCR_PARAM_BW)
    {
        ul_ret = wal_hipriv_process_rate_params(pst_net_dev, pc_param, pst_set_dscr_param);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_data_dscr_param::wal_hipriv_process_ucast_params return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
    }
    else
    {
        /* 解析要设置为多大的速率 */
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_data_dscr_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
        pc_param += ul_off_set;
        pst_set_dscr_param->l_value = oal_strtol(ac_arg, OAL_PTR_NULL, 0);
    }

    /* 单播数据帧描述符设置 tpye = MAC_VAP_CONFIG_UCAST_DATA */
    pst_set_dscr_param->en_type = MAC_VAP_CONFIG_UCAST_DATA;
    OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_data_dscr_param::en_param_index [%d]!,value[%d]}\r\n", pst_set_dscr_param->uc_function_index,pst_set_dscr_param->l_value);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_data_dscr_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}



OAL_STATIC oal_uint32  wal_hipriv_set_bcast_data_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_set_dscr_param_stru     *pst_set_dscr_param;
    wal_dscr_param_enum_uint8        en_param_index;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    /* 解析并设置配置命令参数 */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /* 获取描述符字段设置命令字符串 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_bcast_data_dscr_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 解析是设置哪一个字段 */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++)
    {
        if(!oal_strcmp(pauc_tx_dscr_param_name_etc[en_param_index], ac_arg))
        {
            break;
        }
    }

    /* 检查命令是否打错 */
    if (WAL_DSCR_PARAM_BUTT == en_param_index)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_bcast_data_dscr_param::no such param for tx dscr!}\r\n");
        return OAL_FAIL;
    }
    pst_set_dscr_param->uc_function_index = en_param_index;

    /*配置速率、空间流数、带宽*/
    if(en_param_index >= WAL_DSCR_PARAM_RATE && en_param_index <= WAL_DSCR_PARAM_BW)
    {
        ul_ret = wal_hipriv_process_rate_params(pst_net_dev, pc_param, pst_set_dscr_param);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_bcast_data_dscr_param::wal_hipriv_process_ucast_params return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
    }
    else
    {
        /* 解析要设置为多大的速率 */
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_bcast_data_dscr_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
        pc_param += ul_off_set;
        pst_set_dscr_param->l_value = oal_strtol(ac_arg, OAL_PTR_NULL, 0);
    }

    /* 广播数据帧描述符设置 tpye = MAC_VAP_CONFIG_BCAST_DATA */
    pst_set_dscr_param->en_type = MAC_VAP_CONFIG_BCAST_DATA;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_bcast_data_dscr_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_set_ucast_mgmt_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_set_dscr_param_stru     *pst_set_dscr_param;
    wal_dscr_param_enum_uint8        en_param_index;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                        uc_band;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    /* 解析并设置配置命令参数 */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /***************************************************************************
             sh hipriv.sh "vap0 set_ucast_mgmt data0 2 8389137"
    ***************************************************************************/
    /* 解析data0 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_mgmt_dscr_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 解析是设置哪一个字段 */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++)
    {
        if(!oal_strcmp(pauc_tx_dscr_param_name_etc[en_param_index], ac_arg))
        {
            break;
        }
    }

    /* 检查命令是否打错 */
    if (WAL_DSCR_PARAM_BUTT == en_param_index)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_mgmt_dscr_param::no such param for tx dscr!}\r\n");
        return OAL_FAIL;
    }

    pst_set_dscr_param->uc_function_index = en_param_index;

    /* 解析要设置为哪个频段的单播管理帧 2G or 5G*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_mgmt_dscr_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    uc_band = (oal_uint8)oal_atoi(ac_arg);

    /* 单播管理帧描述符设置 tpye = MAC_VAP_CONFIG_UCAST_MGMT 2为2G,否则为5G  */
    if (WLAN_BAND_2G == uc_band)
    {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_UCAST_MGMT_2G;
    }
    else
    {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_UCAST_MGMT_5G;
    }

    /*配置速率、空间流数、带宽*/
    if(en_param_index >= WAL_DSCR_PARAM_RATE && en_param_index <= WAL_DSCR_PARAM_BW)
    {
        ul_ret = wal_hipriv_process_rate_params(pst_net_dev, pc_param, pst_set_dscr_param);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_mgmt_dscr_param::wal_hipriv_process_ucast_params return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
    }
    else
    {
        /* 解析要设置为多大的速率 */
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_mgmt_dscr_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
        pc_param += ul_off_set;
        pst_set_dscr_param->l_value = oal_strtol(ac_arg, OAL_PTR_NULL, 0);
    }

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ucast_mgmt_dscr_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_set_mbcast_mgmt_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_set_dscr_param_stru     *pst_set_dscr_param;
    wal_dscr_param_enum_uint8        en_param_index;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                        uc_band;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    /* 解析并设置配置命令参数 */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /***************************************************************************
             sh hipriv.sh "vap0 set_mcast_mgmt data0 5 8389137"
    ***************************************************************************/
    /* 解析data0 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mbcast_mgmt_dscr_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 解析是设置哪一个字段 */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++)
    {
        if(!oal_strcmp(pauc_tx_dscr_param_name_etc[en_param_index], ac_arg))
        {
            break;
        }
    }

    /* 检查命令是否打错 */
    if (WAL_DSCR_PARAM_BUTT == en_param_index)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mbcast_mgmt_dscr_param::no such param for tx dscr!}\r\n");
        return OAL_FAIL;
    }

    pst_set_dscr_param->uc_function_index = en_param_index;

    /* 解析要设置为哪个频段的单播管理帧 2G or 5G*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mbcast_mgmt_dscr_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    uc_band = (oal_uint8)oal_atoi(ac_arg);

    /* 单播管理帧描述符设置 tpye = MAC_VAP_CONFIG_UCAST_MGMT 2为2G,否则为5G  */
    if (WLAN_BAND_2G == uc_band)
    {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_MBCAST_MGMT_2G;
    }
    else
    {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_MBCAST_MGMT_5G;
    }

    /*配置速率、空间流数、带宽*/
    if(en_param_index >= WAL_DSCR_PARAM_RATE && en_param_index <= WAL_DSCR_PARAM_BW)
    {
        ul_ret = wal_hipriv_process_rate_params(pst_net_dev, pc_param, pst_set_dscr_param);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mbcast_mgmt_dscr_param::wal_hipriv_process_ucast_params return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
    }
    else
    {
        /* 解析要设置为多大的速率 */
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mbcast_mgmt_dscr_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
        pc_param += ul_off_set;

        pst_set_dscr_param->l_value = oal_strtol(ac_arg, OAL_PTR_NULL, 0);
    }

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mbcast_mgmt_dscr_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}

#ifdef _PRE_WLAN_FEATURE_11D

oal_uint32  wal_hipriv_set_rd_by_ie_switch_etc(oal_net_device_stru *pst_net_dev,oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_switch_enum_uint8           *pst_set_rd_by_ie_switch;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_switch_enum_uint8            en_rd_by_ie_switch = OAL_SWITCH_OFF;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RD_IE_SWITCH, OAL_SIZEOF(oal_switch_enum_uint8));

    /* 解析并设置配置命令参数 */
    pst_set_rd_by_ie_switch = (oal_switch_enum_uint8 *)(st_write_msg.auc_value);

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rd_by_ie_switch_etc::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    en_rd_by_ie_switch = (oal_uint8)oal_atoi(ac_name);
    *pst_set_rd_by_ie_switch = en_rd_by_ie_switch;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_switch_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rd_by_ie_switch_etc::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


oal_uint32  wal_hipriv_set_nss(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_tx_comp_stru             *pst_set_nss_param;
    oal_int32                        l_nss;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                        l_idx = 0;
    wal_msg_stru                    *pst_rsp_msg = OAL_PTR_NULL;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_NSS, OAL_SIZEOF(mac_cfg_tx_comp_stru));

    /* 解析并设置配置命令参数 */
    pst_set_nss_param = (mac_cfg_tx_comp_stru *)(st_write_msg.auc_value);

    /* 获取速率值字符串 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_nss::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 输入命令合法性检测 */
    while ('\0' != ac_arg[l_idx])
    {

        if (isdigit(ac_arg[l_idx]))
        {
            l_idx++;
            continue;
        }
        else
        {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_nss::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* 解析要设置为多大的值 */
    l_nss = oal_atoi(ac_arg);

    if (l_nss < WAL_HIPRIV_NSS_MIN || l_nss > WAL_HIPRIV_NSS_MAX)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_nss::input val out of range [%d]!}\r\n", l_nss);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_set_nss_param->uc_param = (oal_uint8)(l_nss - 1);
    pst_set_nss_param->en_protocol_mode = WLAN_VHT_PHY_PROTOCOL_MODE;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_comp_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_nss::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
        /* 读取返回的错误码 */
    ul_ret = wal_check_and_release_msg_resp_etc(pst_rsp_msg);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_DFR, "{wal_hipriv_set_freq fail, err code[%u]!}\r\n", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  wal_hipriv_set_rfch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_tx_comp_stru             *pst_set_rfch_param;
    oal_uint8                        uc_ch;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int8                         c_ch_idx;
    wal_msg_stru                    *pst_rsp_msg = OAL_PTR_NULL;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RFCH, OAL_SIZEOF(mac_cfg_tx_comp_stru));

    /* 解析并设置配置命令参数 */
    pst_set_rfch_param = (mac_cfg_tx_comp_stru *)(st_write_msg.auc_value);

    /* 获取速率值字符串 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rfch::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 解析要设置为多大的值 */
    uc_ch = 0;
    for (c_ch_idx = 0; c_ch_idx < WAL_HIPRIV_CH_NUM; c_ch_idx++)
    {
        if ('0' == ac_arg[c_ch_idx])
        {
            continue;
        }
        else if ('1' == ac_arg[c_ch_idx])
        {
            uc_ch += (oal_uint8)(1 << (WAL_HIPRIV_CH_NUM - c_ch_idx - 1));
        }
        /* 输入数据有非01数字，或数字少于4位，异常 */
        else
        {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_rfch::input err!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* 输入参数多于四位，异常 */
    if ('\0' != ac_arg[c_ch_idx])
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_rfch::input err!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_set_rfch_param->uc_param = uc_ch;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_comp_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rfch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    /* 读取返回的错误码 */
    ul_ret = wal_check_and_release_msg_resp_etc(pst_rsp_msg);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_DFR, "{wal_hipriv_set_rfch fail, err code[%u]!}\r\n", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_get_thruput(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                        uc_stage;
    oal_int32                        l_idx = 0;

    /* 获取参数 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_thruput::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 输入命令合法性检测 */
    while ('\0' != ac_arg[l_idx])
    {
        if (isdigit(ac_arg[l_idx]))
        {
            l_idx++;
            continue;
        }
        else
        {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_get_thruput::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* 将命令参数值字符串转化为整数 */
    uc_stage = (oal_uint8)oal_atoi(ac_arg);

    *(oal_uint8 *)(st_write_msg.auc_value) = uc_stage;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_THRUPUT, OAL_SIZEOF(oal_uint8));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_thruput::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_set_freq_skew(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int8                    ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint16                  us_len;
    oal_uint16                  i;
    oal_uint32                  ul_ret;
    oal_uint32                  ul_off_set;
    mac_cfg_freq_skew_stru      *pst_freq_skew;

    /*             命令格式: hipriv "Hisilicon0 set_freq_skew <>"
     * <idx chn T0Int20M T0Frac20M T1Int20M T1Frac20M T0Int40M T0Frac40M T1Int40M T1Frac40M>
     */
    pst_freq_skew = (mac_cfg_freq_skew_stru*)st_write_msg.auc_value;

    /* 索引值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_freq_skew::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pst_freq_skew->us_idx = (oal_uint16)oal_atoi(ac_arg);

    /* 信道 */
    pc_param += ul_off_set;
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_freq_skew::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pst_freq_skew->us_chn = (oal_uint16)oal_atoi(ac_arg);

    /* 获取8个校正数据 */
    for (i = 0; i < WAL_HIPRIV_FREQ_SKEW_ARG_NUM; i++)
    {
        pc_param += ul_off_set;
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_freq_skew::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
        pst_freq_skew->as_corr_data[i] = (oal_int16)oal_atoi(ac_arg);
    }

    us_len = OAL_SIZEOF(mac_cfg_freq_skew_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_FREQ_SKEW, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_freq_skew::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



oal_uint32  wal_hipriv_adjust_ppm(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int8                    ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint16                  us_len;
    oal_uint32                  ul_ret;
    oal_uint32                  ul_off_set;
    mac_cfg_adjust_ppm_stru     *pst_adjust_ppm;

    /* 命令格式: hipriv "Hisilicon0 adjust_ppm ppm band clock" */
    pst_adjust_ppm = (mac_cfg_adjust_ppm_stru*)st_write_msg.auc_value;

    /* ppm */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_adjust_ppm::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    if ('-' == ac_arg[0])
    {
        pst_adjust_ppm->c_ppm_val = -(oal_int8)oal_atoi(ac_arg+1);
    }
    else
    {
        pst_adjust_ppm->c_ppm_val = (oal_int8)oal_atoi(ac_arg);
    }


    /* clock */
    pc_param += ul_off_set;
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
#ifdef _PRE_WLAN_PRODUCT_1151V200
        /* 51V200默认是40M */
        pst_adjust_ppm->uc_clock_freq = 40;
#else
        /* 此参数不配置，采用默认时钟配置，5G 26M 2G 40M */
        pst_adjust_ppm->uc_clock_freq = 0;
#endif
    }
    else
    {
        pst_adjust_ppm->uc_clock_freq = (oal_uint8)oal_atoi(ac_arg);
    }

    us_len = OAL_SIZEOF(mac_cfg_adjust_ppm_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADJUST_PPM, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_adjust_ppm::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_event_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    /* OAM event模块的开关的命令: hipriv "Hisilicon0 event_switch 0 | 1"
        此处将解析出"1"或"0"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_event_switch::wal_get_cmd_one_arg_etc return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对event模块进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_event_switch::the log switch command is error [%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_EVENT_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_event_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



#ifdef _PRE_WLAN_NARROW_BAND

OAL_STATIC oal_uint32 wal_hipriv_narrow_bw(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_int32                           l_ret;
    mac_cfg_narrow_bw_stru             *pst_nrw_bw;
    oal_uint32                          ul_ret;
    oal_uint32                          ul_off_set;
    oal_int8                            ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_NARROW_BW, OAL_SIZEOF(mac_cfg_narrow_bw_stru));

     /* 解析并设置配置命令参数 */
    pst_nrw_bw = (mac_cfg_narrow_bw_stru *)(st_write_msg.auc_value);

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_narrow_bw::get switch  [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param = pc_param + ul_off_set;

    pst_nrw_bw->en_open = (oal_uint8)oal_atoi(ac_arg);
#if 0
    if (en_tx_flag > HAL_ALWAYS_TX_RF)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_always_tx::input should be 0 or 1.}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_set_bcast_param->uc_param = en_tx_flag;
#endif

    /* 窄带打开模式下强制关闭ampdu amsdu聚合和若干扰免疫算法 */
    if(OAL_TRUE == pst_nrw_bw->en_open)
    {
        wal_hipriv_alg_cfg_etc(pst_net_dev, "anti_inf_unlock_en 0");
        wal_hipriv_ampdu_tx_on(pst_net_dev, "0");
        wal_hipriv_amsdu_tx_on(pst_net_dev, "0");
    }
    else
    {
        wal_hipriv_alg_cfg_etc(pst_net_dev, "anti_inf_unlock_en 1");
        wal_hipriv_ampdu_tx_on(pst_net_dev, "1");
        wal_hipriv_amsdu_tx_on(pst_net_dev, "1");
    }


    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_narrow_bw::get switch  [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param = pc_param + ul_off_set;

    if (0 == (oal_strcmp("1m", ac_arg)))
    {
        pst_nrw_bw->en_bw = NARROW_BW_1M;
    }
    else if (0 == (oal_strcmp("5m", ac_arg)))
    {
        pst_nrw_bw->en_bw = NARROW_BW_5M;
    }
    else if (0 == (oal_strcmp("10m", ac_arg)))
    {
        pst_nrw_bw->en_bw = NARROW_BW_10M;
    }
    else
    {
        pst_nrw_bw->en_bw = NARROW_BW_BUTT;
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_narrow_bw::bw should be 1/5/10 m");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_narrow_bw_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif


OAL_STATIC oal_uint32  wal_hipriv_ota_beacon_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_int32                           l_param;
    oal_uint32                          ul_off_set = 0;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32                           l_ret;
    oal_uint32                          ul_ret;

    /* OAM ota模块的开关的命令: hipriv "Hisilicon0 ota_beacon_switch 0 | 1"
    */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ota_beacon_switch::wal_get_cmd_one_arg_etc fails!}\r\n");
        return ul_ret;
    }
    l_param = oal_atoi((const oal_int8 *)ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_OTA_BEACON_SWITCH, OAL_SIZEOF(oal_uint32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_param;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ota_beacon_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_ota_rx_dscr_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_int32                           l_param;
    oal_uint32                          ul_off_set = 0;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_int32                           l_ret;
    oal_uint32                          ul_ret;

    /* OAM ota模块的开关的命令: hipriv "Hisilicon0 ota_rx_dscr_switch 0 | 1"
    */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ota_rx_dscr_switch::wal_get_cmd_one_arg_etc fails!}\r\n");
        return ul_ret;
    }

    /* 解析参数 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param + ul_off_set, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ota_rx_dscr_switch::wal_get_cmd_one_arg_etc fails!}\r\n");
        return ul_ret;
    }
    l_param = oal_atoi((const oal_int8 *)ac_name);


    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_OTA_RX_DSCR_SWITCH, OAL_SIZEOF(oal_uint32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_param;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ota_rx_dscr_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_ether_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    mac_cfg_eth_switch_param_stru   st_eth_switch_param;

    /* "vap0 ether_switch user_macaddr oam_ota_frame_direction_type_enum(帧方向) 0|1(开关)" */
    OAL_MEMZERO(&st_eth_switch_param, OAL_SIZEOF(mac_cfg_eth_switch_param_stru));

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, st_eth_switch_param.auc_user_macaddr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ether_switch::wal_hipriv_get_mac_addr_etc return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 获取以太网帧方向 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ether_switch::wal_get_cmd_one_arg_etc return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_eth_switch_param.en_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* 获取开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ether_switch::wal_get_cmd_one_arg_etc return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    st_eth_switch_param.en_switch = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ETH_SWITCH, OAL_SIZEOF(st_eth_switch_param));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_eth_switch_param,
                OAL_SIZEOF(st_eth_switch_param));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_eth_switch_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ether_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_80211_ucast_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    mac_cfg_80211_ucast_switch_stru st_80211_ucast_switch;

    /* sh hipriv.sh "vap0 80211_uc_switch user_macaddr 0|1(帧方向tx|rx) 0|1(帧类型:管理帧|数据帧)
                                                       0|1(帧内容开关) 0|1(CB开关) 0|1(描述符开关)"
    */
    OAL_MEMZERO(&st_80211_ucast_switch, OAL_SIZEOF(mac_cfg_80211_ucast_switch_stru));

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, st_80211_ucast_switch.auc_user_macaddr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::wal_hipriv_get_mac_addr_etc return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 获取80211帧方向 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::get 80211 ucast frame direction return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧类型 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::get ucast frame type return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_frame_type = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧内容打印开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::get ucast frame content switch  return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_frame_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧CB字段打印开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::get ucast frame cb switch return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_cb_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取描述符打印开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::get ucast frame dscr switch return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_dscr_switch = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_80211_UCAST_SWITCH, OAL_SIZEOF(st_80211_ucast_switch));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_80211_ucast_switch,
                OAL_SIZEOF(st_80211_ucast_switch));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_80211_ucast_switch),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_TXOPPS

OAL_STATIC oal_uint32  wal_hipriv_set_txop_ps_machw(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    mac_txopps_machw_param_stru     st_txopps_machw_param = {0};

    /* sh hipriv.sh "stavap_name txopps_hw_en 0|1(txop_ps_en) 0|1(condition1) 0|1(condition2)" */

    /* 获取txop ps使能开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_TXOP, "{wal_hipriv_set_txop_ps_machw::get machw txop_ps en return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_txopps_machw_param.en_machw_txopps_en = (oal_switch_enum_uint8)oal_atoi(ac_name);

    /* 获取txop ps condition1使能开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_TXOP, "{wal_hipriv_set_txop_ps_machw::get machw txop_ps condition1 return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_txopps_machw_param.en_machw_txopps_condition1= (oal_switch_enum_uint8)oal_atoi(ac_name);

    /* 获取txop ps condition2使能开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_TXOP, "{wal_hipriv_set_txop_ps_machw::get machw txop_ps condition2 return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_txopps_machw_param.en_machw_txopps_condition2 = (oal_switch_enum_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TXOP_PS_MACHW, OAL_SIZEOF(st_txopps_machw_param));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_txopps_machw_param,
                OAL_SIZEOF(st_txopps_machw_param));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_txopps_machw_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_TXOP, "{wal_hipriv_set_txop_ps_machw::return err code[%d]!}\r\n", ul_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  wal_hipriv_set_80211_mcast_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    mac_cfg_80211_mcast_switch_stru st_80211_mcast_switch = {0};

    OAL_MEMZERO((oal_uint8*)&st_write_msg, OAL_SIZEOF(st_write_msg));
    /* sh hipriv.sh "Hisilicon0 80211_mc_switch 0|1(帧方向tx|rx) 0|1(帧类型:管理帧|数据帧)
                                                0|1(帧内容开关) 0|1(CB开关) 0|1(描述符开关)"
    */

    /* 获取80211帧方向 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_mcast_switch::get 80211 mcast frame direction return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_mcast_switch.en_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧类型 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_mcast_switch::get mcast frame type return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_mcast_switch.en_frame_type = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧内容打印开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_mcast_switch::get mcast frame content switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_mcast_switch.en_frame_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧CB字段打印开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_mcast_switch::get mcast frame cb switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_mcast_switch.en_cb_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取描述符打印开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_mcast_switch::get mcast frame dscr switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_mcast_switch.en_dscr_switch = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_80211_MCAST_SWITCH, OAL_SIZEOF(st_80211_mcast_switch));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_80211_mcast_switch,
                OAL_SIZEOF(st_80211_mcast_switch));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_80211_mcast_switch),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_mcast_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_all_80211_ucast(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    mac_cfg_80211_ucast_switch_stru st_80211_ucast_switch;
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;


    /* sh hipriv.sh "Hisilicon0 80211_uc_all 0|1(帧方向tx|rx) 0|1(帧类型:管理帧|数据帧)
                                             0|1(帧内容开关) 0|1(CB开关) 0|1(描述符开关)"
    */

    OAL_MEMZERO(&st_80211_ucast_switch, OAL_SIZEOF(mac_cfg_80211_ucast_switch_stru));

    /* 获取80211帧方向 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_80211_ucast::get 80211 ucast frame direction return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧类型 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_80211_ucast::get ucast frame type return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_frame_type = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧内容打印开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_80211_ucast::get ucast frame content switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_frame_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧CB字段打印开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_80211_ucast::get ucast frame cb switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_cb_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取描述符打印开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_80211_ucast::get ucast frame dscr switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_80211_ucast_switch.en_dscr_switch = (oal_uint8)oal_atoi(ac_name);


    /* 设置广播mac地址 */
    oal_memcopy(st_80211_ucast_switch.auc_user_macaddr, BROADCAST_MACADDR, WLAN_MAC_ADDR_LEN);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_80211_UCAST_SWITCH, OAL_SIZEOF(st_80211_ucast_switch));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_80211_ucast_switch,
                OAL_SIZEOF(st_80211_ucast_switch));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_80211_ucast_switch),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_all_ucast_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_all_ether_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_uint16                      us_user_num;
    oal_uint8                       uc_frame_direction;
    oal_uint8                       uc_switch;

    /* sh hipriv.sh "Hisilicon0 ether_all 0|1(帧方向tx|rx) 0|1(开关)" */

    /* 获取以太网帧方向 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_ether_switch::get eth frame direction return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_ether_switch::get eth type return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_switch = (oal_uint8)oal_atoi(ac_name);

    /* 设置开关 */
    for (us_user_num = 0; us_user_num < WLAN_USER_MAX_USER_LIMIT; us_user_num++)
    {
        oam_report_eth_frame_set_switch_etc(us_user_num, uc_switch, uc_frame_direction);
    }

    /* 同时设置广播arp dhcp帧的上报开关 */
    oam_report_dhcp_arp_set_switch_etc(uc_switch);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_dhcp_arp_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint8                       uc_switch;

    /* sh hipriv.sh "Hisilicon0 dhcp_arp_switch 0|1(开关)" */

    /* 获取帧方向 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dhcp_arp_switch::get switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_switch = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DHCP_ARP, OAL_SIZEOF(oal_uint32));
    *((oal_int32 *)(st_write_msg.auc_value)) = (oal_uint32)uc_switch;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dhcp_arp_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32  wal_hipriv_report_vap_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_uint32                      ul_flag_value;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;

    /* sh hipriv.sh "wlan0 report_vap_info  flags_value" */

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_vap_info::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    ul_flag_value = (oal_uint32)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REPORT_VAP_INFO, OAL_SIZEOF(ul_flag_value));

    /* 填写消息体，参数 */
    *(oal_uint32 *)(st_write_msg.auc_value) = ul_flag_value;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(ul_flag_value),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_vap_info::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

OAL_STATIC oal_uint32 wal_get_tone_tran_para(oal_int8 **pc_param, oal_int8 *ac_value,mac_phy_debug_switch_stru *pst_phy_debug_switch)
{
    oal_uint32 ul_ret = 0;
    oal_uint32 ul_off_set = 0;

    if (0 == oal_strcmp("help", ac_value))
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_get_tone_tran_para:tone tansmit command.!!}\r\n");
        return OAL_SUCC;
    }

    pst_phy_debug_switch->st_tone_tran.uc_tone_tran_switch = (oal_uint8)oal_atoi(ac_value);
    if(pst_phy_debug_switch->st_tone_tran.uc_tone_tran_switch == 1)
    {
        ul_ret = wal_get_cmd_one_arg_etc(*pc_param, ac_value, &ul_off_set);
        if ((OAL_SUCC != ul_ret) || (ul_off_set == 0))
        {
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "{wal_get_tone_tran_para:chain index is illegal'!!}\r\n");
            return ul_ret;
        }
        *pc_param += ul_off_set;
        pst_phy_debug_switch->st_tone_tran.uc_chain_idx = (oal_uint16)oal_atoi(ac_value);

        if ((0 != pst_phy_debug_switch->st_tone_tran.uc_chain_idx) && (1 != pst_phy_debug_switch->st_tone_tran.uc_chain_idx))
        {
            OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_get_tone_tran_para:chain index[%d] is invalid'!!}\r\n", pst_phy_debug_switch->st_tone_tran.uc_chain_idx);
            return OAL_FAIL;
        }

        /*  获取数据长度  */
        ul_ret = wal_get_cmd_one_arg_etc(*pc_param, ac_value, &ul_off_set);
        if ((OAL_SUCC != ul_ret) || (ul_off_set == 0) ||(0 == oal_atoi(ac_value)))
        {
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "{wal_get_tone_tran_para:tone data len is illegal'!!}\r\n");
            return ul_ret;
        }
        *pc_param += ul_off_set;
        pst_phy_debug_switch->st_tone_tran.us_data_len = (oal_uint16)oal_atoi(ac_value);
    }
    return OAL_SUCC;
}


ITCM_EXT_T OAL_STATIC oal_uint32 wal_hipriv_set_phy_debug_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set    = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int8                        ac_value[WAL_HIPRIV_CMD_VALUE_MAX_LEN];
    mac_phy_debug_switch_stru       st_phy_debug_switch;
    oal_uint32                      ul_ret        = 0;
    oal_int32                       l_ret         = 0;
    oal_bool_enum_uint8             en_cmd_updata = OAL_FALSE;
    oal_uint8                       uc_data_cnt;              //单次命令的最大计数

    /* sh hipriv.sh "wlan0 phy_debug snr 0|1(关闭|打开) rssi 0|1(关闭|打开) trlr 1234a count N(每个N个报文打印一次)" */

    OAL_MEMZERO(&st_phy_debug_switch, OAL_SIZEOF(st_phy_debug_switch));

    st_phy_debug_switch.ul_rx_comp_isr_interval = 10;  //如果没有设置，则默认10个包打印一次，命令码可以更新
    st_phy_debug_switch.uc_trlr_sel_num = 1;           //mpw2上rx hder占用一个，因此这里从1开始计数
    st_phy_debug_switch.uc_force_work_switch = 0xff;
    st_phy_debug_switch.st_tone_tran.uc_tone_tran_switch = 0xF;     /*  默认单音发送不处于发送/关闭状态  */
    st_phy_debug_switch.uc_dfr_reset_switch = 0xff;
    st_phy_debug_switch.uc_extlna_chg_bypass_switch = 0xff;
    st_phy_debug_switch.uc_edca_param_switch = 0x0;

    do
    {
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if ((OAL_SUCC != ul_ret) && (0 != ul_off_set))
        {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_phy_debug_switch::cmd format err, ret:%d;!!}\r\n", ul_ret);
            return ul_ret;
        }
        pc_param += ul_off_set;

        if (OAL_FALSE == en_cmd_updata)
        {
            en_cmd_updata = OAL_TRUE;
        }
        else if (0 == ul_off_set)
        {
            break;
        }

        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
        if ((OAL_SUCC != ul_ret) || ((!isdigit(ac_value[0])) && (0 != oal_strcmp("help", ac_value))))
        {
            OAL_IO_PRINT("CMD format::sh hipriv.sh 'wlan0 phy_debug [rssi 0|1] [snr 0|1] [trlr xxxxx] [vect yyyyy] [count N] [edca 0-3(tid_no)] [aifsn N] [cwmin N] [cwmax N] [txoplimit N]'");
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 phy_debug [rssi 0|1] [snr 0|1] [trlr xxxxx] [vect yyyyy] [count N] [edca 0-3(tid_no)] [aifsn N] [cwmin N] [cwmax N] [txoplimit N]'!!}\r\n");
            return ul_ret;
        }
        pc_param += ul_off_set;
        ul_off_set = 0;

        if (0 == oal_strcmp("rssi", ac_name))
        {
            if (0 == oal_strcmp("help", ac_value))
            {
                OAM_WARNING_LOG0(0, OAM_SF_CFG, "{Print the rssi of rx packets, reported from rx dscr of MAC, range [-128 ~ +127] dBm.!!}\r\n");
                return OAL_SUCC;
            }

            st_phy_debug_switch.en_rssi_debug_switch = ((oal_uint8)oal_atoi(ac_value)) & OAL_TRUE;
        }
        else if (0 == oal_strcmp("tsensor", ac_name))
        {
            if (0 == oal_strcmp("help", ac_value))
            {
                OAM_WARNING_LOG0(0, OAM_SF_CFG, "{Print the code of T-sensor.!!}\r\n");
                return OAL_SUCC;
            }
            st_phy_debug_switch.en_tsensor_debug_switch = ((oal_uint8)oal_atoi(ac_value)) & OAL_TRUE;
        }
        else if (0 == oal_strcmp("snr", ac_name))
        {
            if (0 == oal_strcmp("help", ac_value))
            {
                OAM_WARNING_LOG0(0, OAM_SF_CFG, "{Print snr values of two rx ants, 11B not included, reported from rx dscr of MAC, range [-10 ~ +55] dBm.!!}\r\n");
                return OAL_SUCC;
            }

            st_phy_debug_switch.en_snr_debug_switch = ((oal_uint8)oal_atoi(ac_value)) & OAL_TRUE;
        }
        else if (0 == oal_strcmp("evm", ac_name))
        {
            if (0 == oal_strcmp("help", ac_value))
            {
                OAM_WARNING_LOG0(0, OAM_SF_CFG, "{Print snr values of two rx ants, 11B not included, reported from rx dscr of MAC, range [-10 ~ +55] dBm.!!}\r\n");
                return OAL_SUCC;
            }

            st_phy_debug_switch.en_evm_debug_switch = ((oal_uint8)oal_atoi(ac_value)) & OAL_TRUE;
        }
        else if ((0 == oal_strcmp("trlr", ac_name)) || (0 == oal_strcmp("vect", ac_name)))
        {
            uc_data_cnt = 0;
            while (ac_value[uc_data_cnt] != '\0')
            {
                /* 输入参数合法性检查 */
                if (st_phy_debug_switch.uc_trlr_sel_num >= WAL_PHY_DEBUG_TEST_WORD_CNT)
                {
                    OAM_ERROR_LOG1(0, OAM_SF_CFG, "{Param input illegal, cnt [%d] reached 4!!}\r\n", st_phy_debug_switch.uc_trlr_sel_num);
                    return OAL_ERR_CODE_ARRAY_OVERFLOW;
                }

                if (0 == oal_strcmp("help", ac_value))
                {
                    OAM_WARNING_LOG0(0, OAM_SF_CFG, "{Sel range::trailer [0~b/B], vector [0~7], eg: trlr 1234, sum of both is less than 4.!!}\r\n");
                    return OAL_SUCC;
                }

                if(isdigit(ac_value[uc_data_cnt]))
                {
                    st_phy_debug_switch.auc_trlr_sel_info[st_phy_debug_switch.uc_trlr_sel_num] = (oal_uint8)(ac_value[uc_data_cnt] - '0');
                }
                else if (('a' == ac_value[uc_data_cnt]) || ('b' == ac_value[uc_data_cnt]))
                {
                    st_phy_debug_switch.auc_trlr_sel_info[st_phy_debug_switch.uc_trlr_sel_num] = (oal_uint8)(ac_value[uc_data_cnt] - 'a' + 10);
                }
                else if (('A' == ac_value[uc_data_cnt]) || ('B' == ac_value[uc_data_cnt]))
                {
                    st_phy_debug_switch.auc_trlr_sel_info[st_phy_debug_switch.uc_trlr_sel_num] = (oal_uint8)(ac_value[uc_data_cnt] - 'A' + 10);
                }
                else
                {
                    OAM_ERROR_LOG0(0, OAM_SF_CFG, "{param input illegal, should be [0-b/B].!!}\r\n");
                    return OAL_ERR_CODE_INVALID_CONFIG;
                }

                if (0 == oal_strcmp("vect", ac_name))
                {
                    if (ac_value[uc_data_cnt] > '7')
                    {
                        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{param input illegal, vect should be [0-7].!!}\r\n");
                        return OAL_ERR_CODE_INVALID_CONFIG;
                    }
                }
                else
                {
                    /* vector bit4为0，trailer的bit4置1, 设置寄存器可以一并带上 */
                    st_phy_debug_switch.auc_trlr_sel_info[st_phy_debug_switch.uc_trlr_sel_num] |= 0x10;
                }

                uc_data_cnt++;
                st_phy_debug_switch.uc_trlr_sel_num++;
            }

            /* 若输入正常，则打开trailer开关 */
            if (uc_data_cnt != 0)
            {
                st_phy_debug_switch.en_trlr_debug_switch = OAL_TRUE;
            }
        }
        else if (0 == oal_strcmp("count", ac_name))
        {
            if (0 == oal_strcmp("help", ac_value))
            {
                OAM_WARNING_LOG0(0, OAM_SF_CFG, "{Set the interval of print (packets), range [0 ~ 2^32].!!}\r\n");
                return OAL_SUCC;
            }

            st_phy_debug_switch.ul_rx_comp_isr_interval = (oal_uint32)oal_atoi(ac_value);
        }
        else if(0 == oal_strcmp("iq_cali", ac_name))
        {
            if (0 == oal_strcmp("help", ac_value))
            {
                OAM_WARNING_LOG0(0, OAM_SF_CFG, "{debug TRX IQ CALI manually!!}\r\n");
                return OAL_SUCC;
            }
            st_phy_debug_switch.uc_iq_cali_switch = (oal_uint8)oal_atoi(ac_value);
        }
        else if(0 == oal_strcmp("tone_tran", ac_name))
        {
            ul_ret = wal_get_tone_tran_para(&pc_param, ac_value, &st_phy_debug_switch);
            if(ul_ret != OAL_SUCC)
            {
                return ul_ret;
            }
        }
        else if (0 == oal_strcmp("pdet", ac_name))
        {
            st_phy_debug_switch.en_pdet_debug_switch = ((oal_uint8)oal_atoi(ac_value)) & OAL_TRUE;
        }
        else if (0 == oal_strcmp("force_work", ac_name))
        {
            st_phy_debug_switch.uc_force_work_switch = ((oal_uint8)oal_atoi(ac_value));
        }
        else if (0 == oal_strcmp("dfr_reset", ac_name))
        {
            st_phy_debug_switch.uc_dfr_reset_switch = ((oal_uint8)oal_atoi(ac_value));
        }
        else if (0 == oal_strcmp("fsm_info", ac_name))
        {
            st_phy_debug_switch.uc_fsm_info_switch = ((oal_uint8)oal_atoi(ac_value)) & OAL_TRUE;
        }
        else if(0 == oal_strcmp("report_radar", ac_name))
        {
            st_phy_debug_switch.uc_report_radar_switch = OAL_TRUE;
        }
        else if(0 == oal_strcmp("extlna_bypass", ac_name))
        {
            st_phy_debug_switch.uc_extlna_chg_bypass_switch = ((oal_uint8)oal_atoi(ac_value));
        }
        else if(0 == oal_strcmp("edca", ac_name))
        {
            st_phy_debug_switch.uc_edca_param_switch |= ((oal_uint8)oal_atoi(ac_value)) <<4;
        }
        else if(0 == oal_strcmp("aifsn", ac_name))
        {
            st_phy_debug_switch.uc_edca_param_switch |= (oal_uint8)BIT3;
            st_phy_debug_switch.uc_edca_aifsn= (oal_uint8)oal_atoi(ac_value);
        }
        else if(0 == oal_strcmp("cwmin", ac_name))
        {
            st_phy_debug_switch.uc_edca_param_switch |= (oal_uint8)BIT2;
            st_phy_debug_switch.uc_edca_cwmin= (oal_uint8)oal_atoi(ac_value);
        }
        else if(0 == oal_strcmp("cwmax", ac_name))
        {
            st_phy_debug_switch.uc_edca_param_switch |= (oal_uint8)BIT1;
            st_phy_debug_switch.uc_edca_cwmax= (oal_uint8)oal_atoi(ac_value);
        }
        else if(0 == oal_strcmp("txoplimit", ac_name))
        {
            st_phy_debug_switch.uc_edca_param_switch |= (oal_uint8)BIT0;
            st_phy_debug_switch.us_edca_txoplimit= (oal_uint16)oal_atoi(ac_value);
        }
        else
        {
            OAL_IO_PRINT("CMD format::sh hipriv.sh 'wlan0 phy_debug [rssi 0|1] [snr 0|1] [trlr xxxxx] [vect yyyyy] [count N] [edca 0-3(tid_no)] [aifsn N] [cwmin N] [cwmax N] [txoplimit N]'");
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 phy_debug [rssi 0|1] [snr 0|1] [trlr xxxx] [vect yyyy] [count N] [edca 0-3(tid_no)] [aifsn N] [cwmin N] [cwmax N] [txoplimit N]'!!}\r\n");
            return OAL_FAIL;
        }
    }while(*pc_param != '\0');

    /* 将打印总开关保存到 */
    st_phy_debug_switch.en_debug_switch = st_phy_debug_switch.en_rssi_debug_switch | st_phy_debug_switch.en_snr_debug_switch | st_phy_debug_switch.en_trlr_debug_switch \
                                           | st_phy_debug_switch.uc_iq_cali_switch | st_phy_debug_switch.en_tsensor_debug_switch | st_phy_debug_switch.en_evm_debug_switch;


    OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_phy_debug_switch:: phy_debug switch [%d].}\r\n", st_phy_debug_switch.en_debug_switch);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PHY_DEBUG_SWITCH, OAL_SIZEOF(st_phy_debug_switch));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_phy_debug_switch,
                OAL_SIZEOF(st_phy_debug_switch));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_phy_debug_switch),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_phy_debug_switch::return err code[%d]!}", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC  OAL_INLINE oal_void protocol_debug_cmd_format_info(void)
{
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{CMD format::sh hipriv.sh 'wlan0 protocol_debug\
    [band_force_switch 0|1|2(20M|40M+|40M-)]\
    [2040_ch_swt_prohi 0|1]\
    [40_intol 0|1]'!!}\r\n");
    OAM_WARNING_LOG0(0, OAM_SF_ANY,
    "{[csa 0(csa mode) 1(csa channel) 10(csa cnt) 1(debug  flag,0:normal channel channel,1:only include csa ie 2:cannel debug)]\
    [2040_user_switch 0|1]'!!}\r\n");
    OAM_WARNING_LOG0(0, OAM_SF_ANY,
    "[lsig 0|1]\
    '!!}\r\n");
}


OAL_STATIC oal_uint32 wal_protocol_debug_parase_csa_cmd(oal_int8 *pc_param,mac_protocol_debug_switch_stru *pst_debug_info,oal_uint32 *pul_offset)
{

    oal_uint32                          ul_ret        = 0;
    oal_int8                            ac_value[WAL_HIPRIV_CMD_VALUE_MAX_LEN];
    oal_uint32                          ul_off_set    = 0;
    oal_uint8                           uc_value;

    *pul_offset = 0;
    /*解析csa mode*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG,"{wal_protocol_debug_parase_csa_cmd::get csa mode error,return.}");
        return ul_ret;
    }
    uc_value = (oal_uint8)oal_atoi(ac_value);
    if(uc_value > 1)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG,"{wal_protocol_debug_parase_csa_cmd::csa mode = [%d] invalid,return.}",uc_value);
        return OAL_FAIL;
    }
    *pul_offset += ul_off_set;
    pst_debug_info->st_csa_debug_bit3.en_mode = uc_value;
    pc_param += ul_off_set;
    ul_off_set = 0;

    /*解析csa channel*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG,"{wal_protocol_debug_parase_csa_cmd::get csa channel error,return.}");
        return ul_ret;
    }
    *pul_offset += ul_off_set;
    pst_debug_info->st_csa_debug_bit3.uc_channel = (oal_uint8)oal_atoi(ac_value);
    pc_param += ul_off_set;
    ul_off_set = 0;

    /*解析bandwidth*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG,"{wal_protocol_debug_parase_csa_cmd::get bandwidth error,return.}");
        return ul_ret;
    }
    uc_value = (oal_uint8)oal_atoi(ac_value);
    if(uc_value >= WLAN_BAND_WIDTH_BUTT)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG,"{wal_protocol_debug_parase_csa_cmd::invalid bandwidth = %d,return.}",uc_value);
        return OAL_FAIL;
    }
    *pul_offset += ul_off_set;
    pst_debug_info->st_csa_debug_bit3.en_bandwidth= uc_value;
    pc_param += ul_off_set;
    ul_off_set = 0;

    /*解析csa cnt*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG,"{wal_protocol_debug_parase_csa_cmd::get csa cnt error,return.}");
        return ul_ret;
    }
    uc_value = (oal_uint8)oal_atoi(ac_value);
    if(uc_value >= 255)
    {
        uc_value = 255;
    }
    *pul_offset += ul_off_set;
    pst_debug_info->st_csa_debug_bit3.uc_cnt = uc_value;
    pc_param += ul_off_set;
    ul_off_set = 0;

    /*解析debug flag*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG,"{wal_protocol_debug_parase_csa_cmd::get debug flag error,return.}");
        return ul_ret;
    }
    *pul_offset += ul_off_set;
    uc_value = (oal_uint8)oal_atoi(ac_value);
    if(uc_value >= MAC_CSA_FLAG_BUTT)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG,"{wal_protocol_debug_parase_csa_cmd::invalid debug flag = %d,return.}",uc_value);
        return OAL_FAIL;
    }
    pst_debug_info->st_csa_debug_bit3.en_debug_flag = (mac_csa_flag_enum_uint8)oal_atoi(ac_value);
    pc_param += ul_off_set;
    ul_off_set = 0;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_set_protocol_debug_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_uint32                          ul_off_set    = 0;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int8                            ac_value[WAL_HIPRIV_CMD_VALUE_MAX_LEN];
    mac_protocol_debug_switch_stru      st_protocol_debug;
    oal_uint32                          ul_ret        = 0;
    oal_int32                           l_ret         = 0;
    oal_bool_enum_uint8                 en_cmd_updata = OAL_FALSE;

    /* sh hipriv.sh "wlan0 protocol_debug band_force_switch 0|1|2(20|40-|40+) 2040_ch_swt_prohi 0|1(关闭|打开) 2040_intolerant 0|1(关闭|打开)" */
    OAL_MEMZERO(&st_protocol_debug, OAL_SIZEOF(st_protocol_debug));

    do
    {
        /*获取命令关键字*/
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if ((OAL_SUCC != ul_ret) && (0 != ul_off_set))
        {
            protocol_debug_cmd_format_info();
            return ul_ret;
        }
        pc_param += ul_off_set;

        if (OAL_FALSE == en_cmd_updata)
        {
            en_cmd_updata = OAL_TRUE;
        }
        else if (0 == ul_off_set)
        {
            break;
        }

        /*命令分类*/
        if (0 == oal_strcmp("band_force_switch", ac_name))
        {
            /*取命令配置值*/
            ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
            if ((OAL_SUCC != ul_ret) || ('0' > ac_value[0]) || ('9' < ac_value[0]))
            {
                protocol_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            /*填写结构体*/
            st_protocol_debug.en_band_force_switch_bit0 = ((oal_uint8)oal_atoi(ac_value));
            st_protocol_debug.ul_cmd_bit_map |= BIT0;
        }
        else if (0 == oal_strcmp("2040_ch_swt_prohi", ac_name))
        {
            /*取命令配置值*/
            ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
            if ((OAL_SUCC != ul_ret) || ('0' > ac_value[0]) || ('9' < ac_value[0]))
            {
                protocol_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            /*填写结构体*/
            st_protocol_debug.en_2040_ch_swt_prohi_bit1 = ((oal_uint8)oal_atoi(ac_value)) & BIT0;
            st_protocol_debug.ul_cmd_bit_map |= BIT1;
        }
        else if (0 == oal_strcmp("40_intol", ac_name))
        {
            /*取命令配置值*/
            ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
            if ((OAL_SUCC != ul_ret) || ('0' > ac_value[0]) || ('9' < ac_value[0]))
            {
                protocol_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            /*填写结构体*/
            st_protocol_debug.en_40_intolerant_bit2 = ((oal_uint8)oal_atoi(ac_value)) & BIT0;
            st_protocol_debug.ul_cmd_bit_map |= BIT2;
        }
        else if(0 == oal_strcmp("csa",ac_name))
        {
            ul_ret = wal_protocol_debug_parase_csa_cmd(pc_param,&st_protocol_debug,&ul_off_set);
            if(OAL_SUCC != ul_ret)
            {
                protocol_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            st_protocol_debug.ul_cmd_bit_map |= BIT3;
        }
#ifdef _PRE_WLAN_FEATURE_HWBW_20_40
        else if (0 == oal_strcmp("2040_user_switch", ac_name))
        {
            /*取命令配置值*/
            ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
            if ((OAL_SUCC != ul_ret) || ('0' > ac_value[0]) || ('1' < ac_value[0]) || ('\0' != ac_value[1]))
            {
                protocol_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            /*填写结构体*/
            st_protocol_debug.en_2040_user_switch_bit4 = ((oal_uint8)oal_atoi(ac_value)) & BIT0;
            st_protocol_debug.ul_cmd_bit_map |= BIT4;
        }
#endif
        else if (0 == oal_strcmp("lsig", ac_name))
        {
            /*取命令配置值*/
            ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
            if ((OAL_SUCC != ul_ret) || ('0' > ac_value[0]) || ('9' < ac_value[0]))
            {
                protocol_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            /*填写结构体*/
            st_protocol_debug.en_lsigtxop_bit5 = ((oal_uint8)oal_atoi(ac_value)) & BIT0;
            st_protocol_debug.ul_cmd_bit_map |= BIT5;
        }
#ifdef _PRE_WLAN_FEATURE_11AX
        else if(0 == oal_strcmp("11ax", ac_name))
        {
            st_protocol_debug.ul_cmd_bit_map |= BIT6;
        }
#endif
        else
        {
            protocol_debug_cmd_format_info();
            return OAL_FAIL;
        }
    }while(*pc_param != '\0');

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_protocol_debug_info::ul_cmd_bit_map: 0x%08x.}", st_protocol_debug.ul_cmd_bit_map);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PROTOCOL_DBG, OAL_SIZEOF(st_protocol_debug));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_protocol_debug,
                OAL_SIZEOF(st_protocol_debug));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_protocol_debug),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_protocol_debug_info::return err code[%d]!}", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32 wal_hipriv_set_pm_debug_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    mac_vap_stru                   *pst_mac_vap;
    oal_uint32                      ul_ret        = 0;
    oal_int32                       l_ret         = 0;
    oal_uint32                      ul_offset;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int8                        ac_value[WAL_HIPRIV_CMD_VALUE_MAX_LEN];
    mac_pm_debug_cfg_stru          *pst_pm_debug_cfg;
    oal_uint8                       uc_switch;
    oal_bool_enum_uint8             en_cmd_updata = OAL_FALSE;

    /* sh hipriv.sh "wlan0 pm_debug srb " */

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if(OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_pm_debug_switch::pst_mac_vap is null!}");
        return OAL_FAIL;
    }

    pst_pm_debug_cfg = (mac_pm_debug_cfg_stru*)(st_write_msg.auc_value);

    OAL_MEMZERO(pst_pm_debug_cfg, OAL_SIZEOF(mac_pm_debug_cfg_stru));

    do
    {
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_offset);
        if ((OAL_SUCC != ul_ret) && (0 != ul_offset))
        {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_pm_debug_switch::cmd format err, ret:%d;!!}\r\n", ul_ret);
            return ul_ret;
        }
        pc_param += ul_offset;

        if (OAL_FALSE == en_cmd_updata)
        {
            en_cmd_updata = OAL_TRUE;
        }
        else if (0 == ul_offset)
        {
            break;
        }

        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_offset);
        if ((OAL_SUCC != ul_ret) || ((!isdigit(ac_value[0])) && (0 != oal_strcmp("help", ac_value))))
        {
            OAL_IO_PRINT("CMD format::sh hipriv.sh 'wlan0 pm_debug [srb 0|1]'\r\n");
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 pm_debug [srb 0|1]'!!}\r\n");
            return ul_ret;
        }
        pc_param += ul_offset;

        if (0 == oal_strcmp("srb", ac_name))
        {
            if (0 == oal_strcmp("help", ac_value))
            {
                OAM_WARNING_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 pm_debug srb [0|1]'}");
                return OAL_SUCC;
            }

            uc_switch = (oal_uint8)oal_atoi(ac_value);
            if (uc_switch > 1)
            {
                OAM_ERROR_LOG1(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 pm_debug srb [0|1]', input[%d]!!}", uc_switch);
                return OAL_FAIL;
            }
            pst_pm_debug_cfg->ul_cmd_bit_map |= BIT(MAC_PM_DEBUG_SISO_RECV_BCN);
            pst_pm_debug_cfg->uc_srb_switch = uc_switch;

            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_hipriv_set_pm_debug_switch::siso recv beacon switch[%d].}", uc_switch);
        }
        else if (0 == oal_strcmp("dto", ac_name))
        {
            if (0 == oal_strcmp("help", ac_value))
            {
                OAM_WARNING_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 pm_debug dto [0|1]'}");
                return OAL_SUCC;
            }

            uc_switch = (oal_uint8)oal_atoi(ac_value);
            if (uc_switch > 1)
            {
                OAM_ERROR_LOG1(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 pm_debug dto [0|1]', input[%d]!!}", uc_switch);
                return OAL_FAIL;
            }
            pst_pm_debug_cfg->ul_cmd_bit_map |= BIT(MAC_PM_DEBUG_DYN_TBTT_OFFSET);
            pst_pm_debug_cfg->uc_dto_switch = uc_switch;

            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_hipriv_set_pm_debug_switch::dyn tbtt offset switch[%d].}", uc_switch);
        }
        else if (0 == oal_strcmp("nfi", ac_name))
        {
            if (0 == oal_strcmp("help", ac_value))
            {
                OAM_WARNING_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 pm_debug nfi [0|1]'}");
                return OAL_SUCC;
            }

            uc_switch = (oal_uint8)oal_atoi(ac_value);
            if (uc_switch > 1)
            {
                OAM_ERROR_LOG1(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 pm_debug nfi [0|1]', input[%d]!!}", uc_switch);
                return OAL_FAIL;
            }
            pst_pm_debug_cfg->ul_cmd_bit_map |= BIT(MAC_PM_DEBUG_NO_PS_FRM_INT);
            pst_pm_debug_cfg->uc_nfi_switch = uc_switch;

            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_hipriv_set_pm_debug_switch::no ps frm int switch[%d].}", uc_switch);
        }
        else if (0 == oal_strcmp("apf", ac_name))
        {
            if (0 == oal_strcmp("help", ac_value))
            {
                OAM_WARNING_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 pm_debug apf [0|1]'}");
                return OAL_SUCC;
            }

            uc_switch = (oal_uint8)oal_atoi(ac_value);
            if (uc_switch > 1)
            {
                OAM_ERROR_LOG1(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 pm_debug apf [0|1]', input[%d]!!}", uc_switch);
                return OAL_FAIL;
            }
            pst_pm_debug_cfg->ul_cmd_bit_map |= BIT(MAC_PM_DEBUG_APF);
            pst_pm_debug_cfg->uc_apf_switch = uc_switch;

            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_hipriv_set_pm_debug_switch::apf switch[%d].}", uc_switch);
        }
        else if (0 == oal_strcmp("ao", ac_name))
        {
            if (0 == oal_strcmp("help", ac_value))
            {
                OAM_WARNING_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 pm_debug ao [0|1]'}");
                return OAL_SUCC;
            }

            uc_switch = (oal_uint8)oal_atoi(ac_value);
            if (uc_switch > 1)
            {
                OAM_ERROR_LOG1(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 pm_debug ao [0|1]', input[%d]!!}", uc_switch);
                return OAL_FAIL;
            }
            pst_pm_debug_cfg->ul_cmd_bit_map |= BIT(MAC_PM_DEBUG_AO);
            pst_pm_debug_cfg->uc_ao_switch = uc_switch;

            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_hipriv_set_pm_debug_switch::arp offload switch[%d].}", uc_switch);
        }
        else
        {
            OAL_IO_PRINT("CMD format::sh hipriv.sh 'wlan0 pm_debug [srb 0|1] [dto 0|1] [nfi 0|1] [apf 0|1] [ao 0|1]'");
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 pm_debug [srb 0|1] [dto 0|1] [nfi 0|1] [apf 0|1]'!!}");
            return OAL_FAIL;
        }
    }while(*pc_param != '\0');

    /***************************************************************************
                               抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PM_DEBUG_SWITCH, OAL_SIZEOF(mac_pm_debug_cfg_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_pm_debug_cfg_stru),
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
       OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dbdc_debug_switch::return err code[%d]!}", l_ret);
       return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_DBDC

OAL_STATIC oal_uint32 wal_hipriv_set_dbdc_debug_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set    = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int8                        ac_value[WAL_HIPRIV_CMD_VALUE_MAX_LEN];
    mac_dbdc_debug_switch_stru      st_dbdc_debug_switch;
    mac_vap_stru                   *pst_mac_vap;
    oal_uint32                      ul_ret        = 0;
    oal_int32                       l_ret         = 0;
    oal_uint8                       uc_dst_hal_dev_id;
    oal_uint8                       en_dbdc_enable;
    oal_uint8                       en_fast_scan_enable;
    oal_bool_enum_uint8             en_cmd_updata = OAL_FALSE;

    /* sh hipriv.sh "wlan0 dbdc_debug change_hal_dev 0|1(hal 0|hal 1)" */

    OAL_MEMZERO(&st_dbdc_debug_switch, OAL_SIZEOF(st_dbdc_debug_switch));

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if(OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_dbdc_debug_switch::pst_mac_vap is null!}");
        return OAL_FAIL;
    }

    do
    {
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if ((OAL_SUCC != ul_ret) && (0 != ul_off_set))
        {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_dbdc_debug_switch::cmd format err, ret:%d;!!}\r\n", ul_ret);
            return ul_ret;
        }
        pc_param += ul_off_set;

        if (OAL_FALSE == en_cmd_updata)
        {
            en_cmd_updata = OAL_TRUE;
        }
        else if (0 == ul_off_set)
        {
            break;
        }

        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
        if ((OAL_SUCC != ul_ret) || ((!isdigit(ac_value[0])) && (0 != oal_strcmp("help", ac_value))))
        {
            OAL_IO_PRINT("CMD format::sh hipriv.sh 'wlan0 dbdc_debug [change_hal_dev 0|1]'\r\n");
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 dbdc_debug [change_hal_dev 0|1]'!!}\r\n");
            return ul_ret;
        }
        pc_param += ul_off_set;
        ul_off_set = 0;

        if (0 == oal_strcmp("change_hal_dev", ac_name))
        {
            if (0 == oal_strcmp("help", ac_value))
            {
                OAM_WARNING_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 dbdc_debug [change_hal_dev 0|1] .!!}");
                return OAL_SUCC;
            }

            uc_dst_hal_dev_id = (oal_uint8)oal_atoi(ac_value);
            if (uc_dst_hal_dev_id > 1)
            {
                OAM_ERROR_LOG1(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 dbdc_debug [change_hal_dev 0|1],input[%d]'!!}", uc_dst_hal_dev_id);
                return OAL_FAIL;
            }
            st_dbdc_debug_switch.ul_cmd_bit_map |= BIT(MAC_DBDC_CHANGE_HAL_DEV);
            st_dbdc_debug_switch.uc_dst_hal_dev_id = ((oal_uint8)oal_atoi(ac_value));

            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_hipriv_set_dbdc_debug_switch::change to hal device[%d].}", st_dbdc_debug_switch.uc_dst_hal_dev_id);
        }
        else if (0 == oal_strcmp("dbdc_enable", ac_name))
        {
            if (0 == oal_strcmp("help", ac_value))
             {
                 OAM_WARNING_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 dbdc_debug dbdc_enable 0|1] .!!}");
                 return OAL_SUCC;
             }

             en_dbdc_enable = (oal_uint8)oal_atoi(ac_value);
             if (en_dbdc_enable > 1)
             {
                 OAM_ERROR_LOG1(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 dbdc_debug [change_hal_dev 0|1],input[%d]'!!}", en_dbdc_enable);
                 return OAL_FAIL;
             }
             st_dbdc_debug_switch.ul_cmd_bit_map |= BIT(MAC_DBDC_SWITCH);
             st_dbdc_debug_switch.uc_dbdc_enable = ((oal_uint8)oal_atoi(ac_value));

             OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_hipriv_set_dbdc_debug_switch::dbdc enable[%d].}", st_dbdc_debug_switch.uc_dbdc_enable);
        }

        else if (0 == oal_strcmp("fast_scan", ac_name))
        {
            if (0 == oal_strcmp("help", ac_value))
             {
                 OAM_WARNING_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 dbdc_debug fast_scan 0|1] .!!}");
                 return OAL_SUCC;
             }

             en_fast_scan_enable = (oal_uint8)oal_atoi(ac_value);
             if (en_fast_scan_enable > 1)
             {
                 OAM_ERROR_LOG1(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 dbdc_debug [fast_scan 0|1],input[%d]'!!}", en_fast_scan_enable);
                 return OAL_FAIL;
             }
             st_dbdc_debug_switch.ul_cmd_bit_map |= BIT(MAC_FAST_SCAN_SWITCH);
             st_dbdc_debug_switch.en_fast_scan_enable = ((oal_uint8)oal_atoi(ac_value));

             OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_hipriv_set_dbdc_debug_switch::fast_scan enable[%d].}", st_dbdc_debug_switch.en_fast_scan_enable);
        }
        else if (0 == oal_strcmp("dbdc_status", ac_name))
        {
            if (0 == oal_strcmp("help", ac_value))
             {
                 OAM_WARNING_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 dbdc_debug dbdc_status 1' !!}");
                 return OAL_SUCC;
             }

             st_dbdc_debug_switch.ul_cmd_bit_map |= BIT(MAC_DBDC_STATUS);
             st_dbdc_debug_switch.uc_dbdc_status = ((oal_uint8)oal_atoi(ac_value));

             OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_hipriv_set_dbdc_debug_switch::dbdc_status enquiry.}");
        }
        else
        {
            OAL_IO_PRINT("CMD format::sh hipriv.sh 'wlan0 dbdc_debug [change_hal_dev 0|1]'");
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 dbdc_debug [change_hal_dev 0|1]'!!}");
            return OAL_FAIL;
        }
    }while(*pc_param != '\0');

    /***************************************************************************
                            抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DBDC_DEBUG_SWITCH, OAL_SIZEOF(st_dbdc_debug_switch));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_dbdc_debug_switch,
                OAL_SIZEOF(st_dbdc_debug_switch));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_dbdc_debug_switch),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dbdc_debug_switch::return err code[%d]!}", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_MEM_TRACE

OAL_STATIC oal_uint32 wal_hipriv_mem_trace_info_show(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_ret;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_mode;
    oal_uint32                      ul_fileid = ~0;
    oal_uint32                      ul_line = 0;

    /* 获取开关 sh hipriv.sh "Hisilicon0 mem_trace_show 0 (file_id,line) | 1 | 2"*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }

    ul_mode = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);
    if(0 == ul_mode)
    {
        pc_param += ul_off_set;
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC == ul_ret)
        {
            ul_fileid = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);

            pc_param += ul_off_set;
            ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC == ul_ret)
            {
                ul_line = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);
            }
        }
    }

    mem_trace_info_show(ul_mode,ul_fileid,ul_line);
    return OAL_SUCC;
}

#endif


OAL_STATIC oal_uint32  wal_hipriv_set_probe_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    mac_cfg_probe_switch_stru       st_probe_switch;

    /* sh hipriv.sh "Hisilicon0 probe_switch 0|1(帧方向tx|rx) 0|1(帧内容开关)
                                             0|1(CB开关) 0|1(描述符开关)"
    */

    /* 获取帧方向 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_probe_switch::get probe direction return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_probe_switch.en_frame_direction = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧内容打印开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_probe_switch::get probe frame content switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_probe_switch.en_frame_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取帧CB字段打印开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_probe_switch::get probe frame cb switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_probe_switch.en_cb_switch = (oal_uint8)oal_atoi(ac_name);

    /* 获取描述符打印开关 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_probe_switch::get probe frame dscr switch return err_code[%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    st_probe_switch.en_dscr_switch = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PROBE_SWITCH, OAL_SIZEOF(st_probe_switch));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_probe_switch,
                OAL_SIZEOF(st_probe_switch));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_probe_switch),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_80211_ucast_switch::return err code[%d]!}\r\n", ul_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_get_mpdu_num(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    mac_cfg_get_mpdu_num_stru       st_param;

    /* sh hipriv.sh "vap_name mpdu_num user_macaddr" */

    OAL_MEMZERO(&st_param, OAL_SIZEOF(mac_cfg_get_mpdu_num_stru));

    /* 获取用户mac地址 */
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, st_param.auc_user_macaddr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_MPDU_NUM, OAL_SIZEOF(st_param));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_param,
                OAL_SIZEOF(st_param));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_all_ota(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                       l_param;
    wal_msg_write_stru              st_write_msg;

    /* 获取开关 sh hipriv.sh "Hisilicon0 set_all_ota 0|1"*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }

    l_param = oal_atoi((const oal_int8 *)ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_ALL_OTA, OAL_SIZEOF(oal_uint32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_param;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_all_ota::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_oam_output(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;
    oal_int32                   l_ret;

    /* OAM log模块的开关的命令: hipriv "Hisilicon0 log_level 0~3"
        此处将解析出"1"或"0"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_oam_output::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对log模块进行不同的设置 取值:oam_output_type_enum_uint8 */
    l_tmp = oal_atoi(ac_name);
    if (l_tmp >= OAM_OUTPUT_TYPE_BUTT)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_oam_output::output type invalid [%d]!}\r\n", l_tmp);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_OAM_OUTPUT_TYPE,  OAL_SIZEOF(oal_int32));
    /* 设置配置命令参数 */
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_oam_output::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_ampdu_start(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                 st_write_msg;
    oal_uint32                         ul_off_set;
    oal_int8                           ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                         ul_ret;
    oal_int32                          l_ret;
    mac_cfg_ampdu_start_param_stru    *pst_ampdu_start_param;
    mac_cfg_ampdu_start_param_stru     st_ampdu_start_param;  /* 临时保存获取的use的信息 */
    oal_uint32                         ul_get_addr_idx;

    /*
        设置AMPDU开启的配置命令: hipriv "Hisilicon0  ampdu_start xx xx xx xx xx xx(mac地址) tidno ack_policy"
    */

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_start::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    OAL_MEMZERO((oal_uint8*)&st_ampdu_start_param, OAL_SIZEOF(st_ampdu_start_param));
    oal_strtoaddr(ac_name, st_ampdu_start_param.auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取tid */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_start::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    if (OAL_STRLEN(ac_name) > 2)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_start::the ampdu start command is erro [%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    st_ampdu_start_param.uc_tidno = (oal_uint8)oal_atoi(ac_name);
    if (st_ampdu_start_param.uc_tidno >= WLAN_TID_MAX_NUM)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_start::the ampdu start command is error! uc_tidno is  [%d]!}\r\n", st_ampdu_start_param.uc_tidno);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMPDU_START, OAL_SIZEOF(mac_cfg_ampdu_start_param_stru));

    /* 设置配置命令参数 */
    pst_ampdu_start_param = (mac_cfg_ampdu_start_param_stru *)(st_write_msg.auc_value);
    for (ul_get_addr_idx = 0; ul_get_addr_idx < WLAN_MAC_ADDR_LEN; ul_get_addr_idx++)
    {
        pst_ampdu_start_param->auc_mac_addr[ul_get_addr_idx] = st_ampdu_start_param.auc_mac_addr[ul_get_addr_idx];
    }

    pst_ampdu_start_param->uc_tidno = st_ampdu_start_param.uc_tidno;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ampdu_start_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_start::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_auto_ba_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    /* 设置自动开始BA会话的开关:hipriv "vap0  auto_ba 0 | 1" 该命令针对某一个VAP */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_auto_ba_switch::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对AUTO BA进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_auto_ba_switch::the auto ba switch command is error[%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AUTO_BA_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_auto_ba_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_profiling_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    /* 设置自动开始BA会话的开关:hipriv "vap0  profiling 0 | 1" 该命令针对某一个VAP */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_profiling_switch::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对AUTO BA进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_profiling_switch::the profiling switch command is error[%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PROFILING_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_profiling_switch::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_addba_req(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_addba_req_param_stru   *pst_addba_req_param;
    mac_cfg_addba_req_param_stru    st_addba_req_param;     /* 临时保存获取的addba req的信息 */
    oal_uint32                      ul_get_addr_idx;

    /*
        设置AMPDU关闭的配置命令:
        hipriv "Hisilicon0 addba_req xx xx xx xx xx xx(mac地址) tidno ba_policy buffsize timeout"
    */

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    OAL_MEMZERO((oal_uint8*)&st_addba_req_param, OAL_SIZEOF(st_addba_req_param));
    oal_strtoaddr(ac_name, st_addba_req_param.auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取tid */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    if (OAL_STRLEN(ac_name) > 2)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::the addba req command is error[%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    st_addba_req_param.uc_tidno = (oal_uint8)oal_atoi(ac_name);
    if (st_addba_req_param.uc_tidno >= WLAN_TID_MAX_NUM)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::the addba req command is error!uc_tidno is [%d]!}\r\n", st_addba_req_param.uc_tidno);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pc_param = pc_param + ul_off_set;

    /* 获取ba_policy */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    st_addba_req_param.en_ba_policy = (oal_uint8)oal_atoi(ac_name);
    if (MAC_BA_POLICY_IMMEDIATE != st_addba_req_param.en_ba_policy)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::the ba policy is not correct! ba_policy is[%d]!}\r\n", st_addba_req_param.en_ba_policy);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pc_param = pc_param + ul_off_set;

    /* 获取buffsize */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    st_addba_req_param.us_buff_size = (oal_uint16)oal_atoi(ac_name);

    pc_param = pc_param + ul_off_set;

    /* 获取timeout时间 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    st_addba_req_param.us_timeout = (oal_uint16)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADDBA_REQ, OAL_SIZEOF(mac_cfg_addba_req_param_stru));

    /* 设置配置命令参数 */
    pst_addba_req_param = (mac_cfg_addba_req_param_stru *)(st_write_msg.auc_value);
    for (ul_get_addr_idx = 0; ul_get_addr_idx < WLAN_MAC_ADDR_LEN; ul_get_addr_idx++)
    {
        pst_addba_req_param->auc_mac_addr[ul_get_addr_idx] = st_addba_req_param.auc_mac_addr[ul_get_addr_idx];
    }

    pst_addba_req_param->uc_tidno     = st_addba_req_param.uc_tidno;
    pst_addba_req_param->en_ba_policy = st_addba_req_param.en_ba_policy;
    pst_addba_req_param->us_buff_size = st_addba_req_param.us_buff_size;
    pst_addba_req_param->us_timeout   = st_addba_req_param.us_timeout;


    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_addba_req_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addba_req::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_delba_req(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_delba_req_param_stru   *pst_delba_req_param;
    mac_cfg_delba_req_param_stru    st_delba_req_param;     /* 临时保存获取的addba req的信息 */
    oal_uint32                      ul_get_addr_idx;

    /*
        设置AMPDU关闭的配置命令:
        hipriv "Hisilicon0 delba_req xx xx xx xx xx xx(mac地址) tidno direction reason_code"
    */

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    OAL_MEMZERO((oal_uint8*)&st_delba_req_param, OAL_SIZEOF(st_delba_req_param));
    oal_strtoaddr(ac_name, st_delba_req_param.auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取tid */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    if (OAL_STRLEN(ac_name) > 2)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::the delba_req req command is error[%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    st_delba_req_param.uc_tidno = (oal_uint8)oal_atoi(ac_name);
    if (st_delba_req_param.uc_tidno >= WLAN_TID_MAX_NUM)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::the delba_req req command is error! uc_tidno is[%d]!}\r\n", st_delba_req_param.uc_tidno);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pc_param = pc_param + ul_off_set;

    /* 获取direction */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    st_delba_req_param.en_direction = (oal_uint8)oal_atoi(ac_name);
    if (st_delba_req_param.en_direction >= MAC_BUTT_DELBA)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::the direction is not correct! direction is[%d]!}\r\n", st_delba_req_param.en_direction);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DELBA_REQ, OAL_SIZEOF(mac_cfg_delba_req_param_stru));

    /* 设置配置命令参数 */
    pst_delba_req_param = (mac_cfg_delba_req_param_stru *)(st_write_msg.auc_value);
    for (ul_get_addr_idx = 0; ul_get_addr_idx < WLAN_MAC_ADDR_LEN; ul_get_addr_idx++)
    {
        pst_delba_req_param->auc_mac_addr[ul_get_addr_idx] = st_delba_req_param.auc_mac_addr[ul_get_addr_idx];
    }

    pst_delba_req_param->uc_tidno       = st_delba_req_param.uc_tidno;
    pst_delba_req_param->en_direction   = st_delba_req_param.en_direction;
    pst_delba_req_param->en_trigger     = MAC_DELBA_TRIGGER_COMM;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_delba_req_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delba_req::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_WMMAC

OAL_STATIC oal_uint32  wal_hipriv_addts_req(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                   st_write_msg;
    oal_uint32                           ul_off_set;
    oal_int8                             ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                           ul_ret;
    oal_int32                            l_ret;
    mac_cfg_wmm_tspec_stru_param_stru    *pst_addts_req_param;
    mac_cfg_wmm_tspec_stru_param_stru    st_addts_req_param;     /* 临时保存获取的addts req的信息 */

    /*
    设置发送ADDTS REQ配置命令:
    hipriv "vap0 addts_req tid direction psb up nominal_msdu_size maximum_data_rate
            minimum_data_rate mean_data_rate peak_data_rate minimum_phy_rate surplus_bandwidth_allowance"
   */

 /***********************************************************************************************
 TSPEC字段:
          --------------------------------------------------------------------------------------
          |TS Info|Nominal MSDU Size|Max MSDU Size|Min Serv Itvl|Max Serv Itvl|
          ---------------------------------------------------------------------------------------
 Octets:  | 3     |  2              |   2         |4            |4            |
          ---------------------------------------------------------------------------------------
          | Inactivity Itvl | Suspension Itvl | Serv Start Time |Min Data Rate | Mean Data Rate |
          ---------------------------------------------------------------------------------------
 Octets:  |4                | 4               | 4               |4             |  4             |
          ---------------------------------------------------------------------------------------
          |Peak Data Rate|Burst Size|Delay Bound|Min PHY Rate|Surplus BW Allowance  |Medium Time|
          ---------------------------------------------------------------------------------------
 Octets:  |4             |4         | 4         | 4          |  2                   |2          |
          ---------------------------------------------------------------------------------------

 TS info字段:
          ---------------------------------------------------------------------------------------
          |Reserved |TSID |Direction |1 |0 |Reserved |PSB |UP |Reserved |Reserved |Reserved |
          ---------------------------------------------------------------------------------------
   Bits:  |1        |4    |2         |  2  |1        |1   |3  |2        |1        |7        |
          ----------------------------------------------------------------------------------------

 ***********************************************************************************************/

    /* 获取tid，取值范围0~7 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addts_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    if (OAL_STRLEN(ac_name) > 2)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addts_req::the addba req command is error[%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    st_addts_req_param.ts_info.bit_tsid= (oal_uint16)oal_atoi(ac_name);
    if (WLAN_TID_MAX_NUM <= st_addts_req_param.ts_info.bit_tsid)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addts_req::the addts req command is error!uc_tidno is [%d]!}\r\n", st_addts_req_param.ts_info.bit_tsid);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pc_param = pc_param + ul_off_set;

    /* 获取direction 00:uplink 01:downlink 10:reserved 11:Bi-directional */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addts_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    st_addts_req_param.ts_info.bit_direction= (oal_uint16)oal_atoi(ac_name);
    if (MAC_WMMAC_DIRECTION_RESERVED == st_addts_req_param.ts_info.bit_direction)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addts_req::the direction is not correct! direction is[%d]!}\r\n", st_addts_req_param.ts_info.bit_direction);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pc_param = pc_param + ul_off_set;

    /* 获取PSB，1表示U-APSD，0表示legacy */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addts_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    st_addts_req_param.ts_info.bit_apsd= (oal_uint16)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取UP */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addts_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    st_addts_req_param.ts_info.bit_user_prio= (oal_uint16)oal_atoi(ac_name);

    pc_param = pc_param + ul_off_set;

    /* 获取Nominal MSDU Size ,第一位为1 */
    /*
        ------------
        |fixed|size|
        ------------
 bits:  |1    |15  |
        ------------
    */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addts_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    st_addts_req_param.us_norminal_msdu_size = (oal_uint16)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取maximum MSDU size */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addts_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    st_addts_req_param.us_max_msdu_size = (oal_uint16)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;


    /* 获取minimum data rate */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addts_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    st_addts_req_param.ul_min_data_rate = (oal_uint32)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取mean data rate */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addts_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    st_addts_req_param.ul_mean_data_rate = (oal_uint32)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取peak data rate */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addts_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    st_addts_req_param.ul_peak_data_rate = (oal_uint32)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取minimum PHY Rate */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addts_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    st_addts_req_param.ul_min_phy_rate = (oal_uint32)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取surplus bandwidth allowance */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addts_req::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    st_addts_req_param.us_surplus_bw = (oal_uint16)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADDTS_REQ, OAL_SIZEOF(mac_cfg_wmm_tspec_stru_param_stru));

    /* 设置配置命令参数 */
    pst_addts_req_param = (mac_cfg_wmm_tspec_stru_param_stru *)(st_write_msg.auc_value);

    pst_addts_req_param->ts_info.bit_tsid        = st_addts_req_param.ts_info.bit_tsid;
    pst_addts_req_param->ts_info.bit_direction   = st_addts_req_param.ts_info.bit_direction;
    pst_addts_req_param->ts_info.bit_apsd        = st_addts_req_param.ts_info.bit_apsd;
    pst_addts_req_param->ts_info.bit_user_prio   = st_addts_req_param.ts_info.bit_user_prio;
    pst_addts_req_param->us_norminal_msdu_size   = st_addts_req_param.us_norminal_msdu_size;
    pst_addts_req_param->us_max_msdu_size        = st_addts_req_param.us_max_msdu_size;
    pst_addts_req_param->ul_min_data_rate        = st_addts_req_param.ul_min_data_rate;
    pst_addts_req_param->ul_mean_data_rate       = st_addts_req_param.ul_mean_data_rate;
    pst_addts_req_param->ul_peak_data_rate       = st_addts_req_param.ul_peak_data_rate;
    pst_addts_req_param->ul_min_phy_rate         = st_addts_req_param.ul_min_phy_rate;
    pst_addts_req_param->us_surplus_bw           = st_addts_req_param.us_surplus_bw;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_wmm_tspec_stru_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_addts_req::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_delts(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                   st_write_msg;
    oal_uint32                           ul_off_set;
    oal_int8                             ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                           ul_ret;
    oal_int32                            l_ret;
    mac_cfg_wmm_tspec_stru_param_stru   *pst_delts_param;
    mac_cfg_wmm_tspec_stru_param_stru    st_delts_param;

    /* 设置删除TS的配置命令: hipriv "Hisilicon0 delts tidno" */

    /* 获取tsid */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delts::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    st_delts_param.ts_info.bit_tsid = (oal_uint8)oal_atoi(ac_name);
    if (st_delts_param.ts_info.bit_tsid >= WLAN_TID_MAX_NUM)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delts::the delts command is error! tsid is[%d]!}\r\n", st_delts_param.ts_info.bit_tsid);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DELTS, OAL_SIZEOF(mac_cfg_wmm_tspec_stru_param_stru));

    /* 设置配置命令参数 */
    pst_delts_param = (mac_cfg_wmm_tspec_stru_param_stru *)(st_write_msg.auc_value);
    OAL_MEMZERO(pst_delts_param, OAL_SIZEOF(mac_cfg_wmm_tspec_stru_param_stru));

    pst_delts_param->ts_info.bit_tsid      = st_delts_param.ts_info.bit_tsid;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_wmm_tspec_stru_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delts::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_wmmac_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                   st_write_msg;
    oal_uint32                           ul_off_set;
    oal_int8                             ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                           ul_ret;
    oal_int32                            l_ret;
    oal_uint8                            uc_wmmac_switch;
    mac_cfg_wmm_ac_param_stru            st_wmm_ac_param;

    /* 设置删除TS的配置命令: hipriv "vap0 wmmac_switch 1/0(使能) 0|1(WMM_AC认证使能) AC xxx(limit_medium_time)" */

    OAL_MEMZERO(&st_wmm_ac_param, OAL_SIZEOF(mac_cfg_wmm_ac_param_stru));
    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wmmac_switch::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    uc_wmmac_switch = (oal_uint8)oal_atoi(ac_name);
    if (uc_wmmac_switch != OAL_FALSE)
    {
        uc_wmmac_switch = OAL_TRUE;
    }
    st_wmm_ac_param.en_wmm_ac_switch = uc_wmmac_switch;

    /* 获取auth flag*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wmmac_switch::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    st_wmm_ac_param.en_auth_flag = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* timeout period ms*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wmmac_switch::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    st_wmm_ac_param.us_timeout_period = (oal_uint16)oal_atoi(ac_name);
    pc_param += ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wmmac_switch::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    st_wmm_ac_param.uc_factor = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    /* 设置配置命令参数 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_WMMAC_SWITCH, OAL_SIZEOF(st_wmm_ac_param));
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_wmm_ac_param,
                OAL_SIZEOF(st_wmm_ac_param));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_wmm_ac_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_delts::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_reassoc_req(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REASSOC_REQ, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_reassoc_req::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif //#ifdef _PRE_WLAN_FEATURE_WMMAC


OAL_STATIC oal_uint32  wal_hipriv_mem_info(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    oal_int8                     auc_token[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_mem_pool_id_enum_uint8   en_pool_id;
    oal_uint32                   ul_off_set;
    oal_uint32                   ul_ret;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_cfg_net_dev) || OAL_UNLIKELY(OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_mem_info::pst_net_dev or pc_param null ptr error [%d] [%d]!}\r\n", pst_cfg_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取内存池ID */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, auc_token, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_mem_info::wal_get_cmd_one_arg_etc return error code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    en_pool_id = (oal_mem_pool_id_enum_uint8)oal_atoi(auc_token);

    /* 打印内存池信息 */
    oal_mem_info_etc(en_pool_id);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_mem_leak(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    oal_int8                     auc_token[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_mem_pool_id_enum_uint8   en_pool_id;
    oal_uint32                   ul_off_set;
    oal_uint32                   ul_ret;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_cfg_net_dev) || OAL_UNLIKELY(OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_mem_leak::pst_net_dev or pc_param null ptr error [%d] [%d]!}\r\n", pst_cfg_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取内存池ID */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, auc_token, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_mem_leak::wal_get_cmd_one_arg_etc return error code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    en_pool_id = (oal_mem_pool_id_enum_uint8)oal_atoi(auc_token);
    if (en_pool_id > OAL_MEM_POOL_ID_SDT_NETBUF)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_mem_leak::mem pool id exceeds,en_pool_id[%d]!}\r\n", en_pool_id);
        return OAL_SUCC;
    }

    /* 检查内存池泄漏内存块 */
    oal_mem_leak_etc(en_pool_id);

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32  wal_hipriv_device_mem_leak(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                st_write_msg;
    oal_uint32                        ul_off_set;
    oal_int8                          ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                        ul_ret;
    oal_int32                         l_ret;
    oal_uint8                         uc_pool_id;
    mac_device_pool_id_stru          *pst_pool_id_param;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_device_mem_leak::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_pool_id = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEVICE_MEM_LEAK, OAL_SIZEOF(mac_device_pool_id_stru));

    /* 设置配置命令参数 */
    pst_pool_id_param = (mac_device_pool_id_stru *)(st_write_msg.auc_value);
    pst_pool_id_param->uc_pool_id   = uc_pool_id;

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_device_pool_id_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_device_mem_leak::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_memory_info(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    oal_uint32                        ul_off_set;
    oal_uint32                        ul_ret;
    oal_int32                         l_ret;
    oal_uint8                         uc_meminfo_type = MAC_MEMINFO_BUTT;
    oal_int8                          ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    wal_msg_write_stru                st_write_msg;
    mac_cfg_meminfo_stru             *pst_meminfo_param;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{CMD format::sh hipriv.sh 'wlan0 memoryinfo [all|dscr|netbuff|user|vap|sdio_sch_q] [pool_usage/pool_debug 0|1,..7<pool_id>]'!}\r\n");
        return ul_ret;
    }

    if(0 == (oal_strcmp("host", ac_name)))
    {
        oal_mem_print_pool_info_etc();
        return OAL_SUCC;
    }
    else if(0 == (oal_strcmp("device", ac_name)))
    {
        hcc_print_device_mem_info_etc();
        return OAL_SUCC;
    }
    else if(0 == (oal_strcmp("all", ac_name)))
    {
        uc_meminfo_type = MAC_MEMINFO_ALL;
    }
    else if(0 == (oal_strcmp("dscr", ac_name)))
    {
        uc_meminfo_type = MAC_MEMINFO_DSCR;
    }
    else if(0 == (oal_strcmp("netbuff", ac_name)))
    {
        uc_meminfo_type = MAC_MEMINFO_NETBUFF;
    }
    else if(0 == (oal_strcmp("user", ac_name)))
    {
        uc_meminfo_type = MAC_MEMINFO_USER;
    }
    else if(0 == (oal_strcmp("vap", ac_name)))
    {
        uc_meminfo_type = MAC_MEMINFO_VAP;
    }
    else if(0 == (oal_strcmp("sdio_sch_q", ac_name)))
    {
        uc_meminfo_type = MAC_MEMINFO_SDIO_TRX;
    }
    else if(0 == (oal_strcmp("pool_usage", ac_name)))
    {
        uc_meminfo_type = MAC_MEMINFO_POOL_INFO;
    }
    else if(0 == (oal_strcmp("pool_debug", ac_name)))
    {
        uc_meminfo_type = MAC_MEMINFO_POOL_DBG;
    }
    else if(0 == (oal_strcmp("sample_alloc", ac_name)))
    {
        uc_meminfo_type = MAC_MEMINFO_SAMPLE_ALLOC;
    }
    else if(0 == (oal_strcmp("sample_free", ac_name)))
    {
        uc_meminfo_type = MAC_MEMINFO_SAMPLE_FREE;
    }
    else
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_memory_info::wal_get_cmd_one_arg_etc::second arg:: please check input!}\r\n");
        return OAL_FAIL;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEVICE_MEM_INFO, OAL_SIZEOF(mac_cfg_meminfo_stru));
    pst_meminfo_param = (mac_cfg_meminfo_stru *)(st_write_msg.auc_value);
    pst_meminfo_param->uc_meminfo_type = uc_meminfo_type;
    /* host和device mempool个数不一致 ，这里用0xff给个default值 */
    pst_meminfo_param->uc_object_index = 0xff;

    if ((MAC_MEMINFO_POOL_INFO == uc_meminfo_type) || (MAC_MEMINFO_POOL_DBG == uc_meminfo_type) || (MAC_MEMINFO_SAMPLE_ALLOC == uc_meminfo_type))
    {
        pc_param = pc_param + ul_off_set;

        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        /* 没有后续参数不退出 */
        if (OAL_SUCC == ul_ret)
        {
            pst_meminfo_param->uc_object_index = (oal_uint8)oal_atoi(ac_name);
        }
    }

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_meminfo_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_memory_info::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif


OAL_STATIC oal_uint32  wal_hipriv_beacon_chain_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_net_dev) || (OAL_PTR_NULL == pc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_beacon_chain_switch::pst_net_dev or pc_param null ptr error %d, %d!}\r\n",  pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* beacon通道(0/1/2)切换开关的命令: hipriv "vap0 beacon_chain_switch 0 | 1 | 2"
        此处将解析出"0"或"1"或"2"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_beacon_chain_switch::wal_get_cmd_one_arg_etc return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，配置不同的通道 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else if (0 == (oal_strcmp("2", ac_name)))
    {
        l_tmp = 2;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_beacon_chain_switch::the beacon chain switch command is error %d!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_BEACON_CHAIN_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_beacon_chain_switch::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_BTCOEX

OAL_STATIC oal_uint32 wal_hipriv_btcoex_status_print(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;

    OAL_MEMZERO((oal_uint8*)&st_write_msg, OAL_SIZEOF(st_write_msg));

    /* sh hipriv.sh "vap_name coex_print" */

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_BTCOEX_STATUS_PRINT, OAL_SIZEOF(oal_uint32));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_status_print::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32 wal_hipriv_btcoex_preempt_type(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    mac_btcoex_preempt_mgr_stru *pst_btcoex_preempt_mgr;
    wal_msg_write_stru          st_write_msg;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    OAL_MEMZERO((oal_uint8*)&st_write_msg, OAL_SIZEOF(st_write_msg));

    /*设置配置命令参数 */
    pst_btcoex_preempt_mgr = (mac_btcoex_preempt_mgr_stru*)st_write_msg.auc_value;

    /* 1.获取第一个参数: mode */
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_preempt_type::wal_get_cmd_one_arg1 return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    pst_btcoex_preempt_mgr->uc_cfg_preempt_mode = (oal_uint8)oal_atoi(ac_name);
    if(pst_btcoex_preempt_mgr->uc_cfg_preempt_mode > 5)
    {
        OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_preempt_type:: pst_btcoex_preempt_mgr->uc_cfg_preempt_mode [%d]!}\r\n",
            pst_btcoex_preempt_mgr->uc_cfg_preempt_mode);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 2.获取第2个参数 */
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_preempt_type::wal_get_cmd_one_arg2 return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 参数放宽配置，约束配置行为 */
    pst_btcoex_preempt_mgr->uc_cfg_preempt_type = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_BTCOEX_PREEMPT_TYPE, OAL_SIZEOF(mac_btcoex_preempt_mgr_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_btcoex_preempt_mgr_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_preempt_type::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_btcoex_set_perf_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    mac_btcoex_mgr_stru            *pst_btcoex_mgr;
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /* st_write_msg作清零操作 */
    oal_memset(&st_write_msg, 0, OAL_SIZEOF(wal_msg_write_stru));

    /*设置配置命令参数 */
    pst_btcoex_mgr = (mac_btcoex_mgr_stru*)st_write_msg.auc_value;

    /* 1.获取第一个参数: mode */
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg1 return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    pst_btcoex_mgr->uc_cfg_btcoex_mode = (oal_uint8)oal_atoi(ac_name);

    if(0 == pst_btcoex_mgr->uc_cfg_btcoex_mode)
    {

    }
    else if(1 == pst_btcoex_mgr->uc_cfg_btcoex_mode)
    {
        /* 偏移，取下一个参数 */
        pc_param = pc_param + ul_off_set;

        /* 2.获取第二个参数*/
        ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg2 return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }

        pst_btcoex_mgr->uc_cfg_btcoex_type = (oal_uint8)oal_atoi(ac_name);
        if(2 < pst_btcoex_mgr->uc_cfg_btcoex_type)
        {
             OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param:: pst_btcoex_mgr->uc_cfg_btcoex_type error [%d], [0/1/2]!}\r\n",
                 pst_btcoex_mgr->uc_cfg_btcoex_type);
             return OAL_ERR_CODE_INVALID_CONFIG;
        }

        /* 偏移，取下一个参数 */
        pc_param = pc_param + ul_off_set;

        if(0 == pst_btcoex_mgr->uc_cfg_btcoex_type)
        {
            /* 3.获取第三个参数 */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                 OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg3 return err_code [%d]!}\r\n", ul_ret);
                 return ul_ret;
            }

            pst_btcoex_mgr->pri_data.rx_size.en_btcoex_nss = (wlan_nss_enum_uint8)oal_atoi(ac_name);

            /* 偏移，取下一个参数 */
            pc_param = pc_param + ul_off_set;

            /* 获取第四个参数 */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg4 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_btcoex_mgr->pri_data.threhold.uc_20m_low = (oal_uint8)oal_atoi(ac_name);

            /* 偏移，取下一个参数 */
            pc_param = pc_param + ul_off_set;

            /* 获取第五个参数 */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg5 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_btcoex_mgr->pri_data.threhold.uc_20m_high  = (oal_uint8)oal_atoi(ac_name);

            /* 偏移，取下一个参数 */
            pc_param = pc_param + ul_off_set;

            /* 获取第六个参数: */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg6 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_btcoex_mgr->pri_data.threhold.uc_40m_low = (oal_uint8)oal_atoi(ac_name);

            /* 偏移，取下一个参数 */
            pc_param = pc_param + ul_off_set;

            /* 获取第七个参数: */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg7 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_btcoex_mgr->pri_data.threhold.us_40m_high = (oal_uint16)oal_atoi(ac_name);
        }
        else if(1 == pst_btcoex_mgr->uc_cfg_btcoex_type)
        {
            /* 3.获取第三个参数 */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                 OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg3 return err_code [%d]!}\r\n", ul_ret);
                 return ul_ret;
            }

            pst_btcoex_mgr->pri_data.rx_size.en_btcoex_nss = (wlan_nss_enum_uint8)oal_atoi(ac_name);

            /* 偏移，取下一个参数 */
            pc_param = pc_param + ul_off_set;

            /* 获取第四个参数 */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg3 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_btcoex_mgr->pri_data.rx_size.uc_grade = (oal_uint8)oal_atoi(ac_name);

            /* 偏移，取下一个参数 */
            pc_param = pc_param + ul_off_set;

            /* 获取第五个参数 */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg4 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_btcoex_mgr->pri_data.rx_size.uc_rx_size0  = (oal_uint8)oal_atoi(ac_name);

            /* 偏移，取下一个参数 */
            pc_param = pc_param + ul_off_set;

            /* 获取第六个参数: */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg5 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_btcoex_mgr->pri_data.rx_size.uc_rx_size1 = (oal_uint8)oal_atoi(ac_name);

            /* 偏移，取下一个参数 */
            pc_param = pc_param + ul_off_set;

            /* 获取第七个参数: */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg6 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_btcoex_mgr->pri_data.rx_size.uc_rx_size2 = (oal_uint8)oal_atoi(ac_name);

            /* 偏移，取下一个参数 */
            pc_param = pc_param + ul_off_set;

            /* 获取第八个参数: */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg7 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_btcoex_mgr->pri_data.rx_size.uc_rx_size3 = (oal_uint8)oal_atoi(ac_name);
        }
        else
        {
            /* 3.获取第三个参数 */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                 OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg3 return err_code [%d]!}\r\n", ul_ret);
                 return ul_ret;
            }

            pst_btcoex_mgr->pri_data.rssi_param.en_rssi_limit_on = (oal_bool_enum_uint8)oal_atoi(ac_name);

            /* 偏移，取下一个参数 */
            pc_param = pc_param + ul_off_set;

            /* 获取第四个参数: */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg6 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_btcoex_mgr->pri_data.rssi_param.en_rssi_log_on = (oal_bool_enum_uint8)oal_atoi(ac_name);

            /* 偏移，取下一个参数 */
            pc_param = pc_param + ul_off_set;

            /* 获取第五个参数 */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg3 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_btcoex_mgr->pri_data.rssi_param.uc_cfg_rssi_detect_cnt = (oal_uint8)oal_atoi(ac_name);

            /* 偏移，取下一个参数 */
            pc_param = pc_param + ul_off_set;

            /* 获取第六个参数 */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg4 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_btcoex_mgr->pri_data.rssi_param.c_cfg_rssi_detect_m2s_th = (oal_int8)oal_atoi(ac_name);

            /* 偏移，取下一个参数 */
            pc_param = pc_param + ul_off_set;

            /* 获取第七个参数: */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::wal_get_cmd_one_arg5 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_btcoex_mgr->pri_data.rssi_param.c_cfg_rssi_detect_s2m_th = (oal_int8)oal_atoi(ac_name);
        }
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param::pst_btcoex_mgr->uc_cfg_btcoex_mode err_code [%d]!}\r\n",
            pst_btcoex_mgr->uc_cfg_btcoex_mode);
        return OAL_FAIL;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_BTCOEX_SET_PERF_PARAM, OAL_SIZEOF(mac_btcoex_mgr_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_btcoex_mgr_stru),
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_btcoex_set_perf_param:: return err code = [%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE

OAL_STATIC oal_uint32  wal_hipriv_dev_customize_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;
    oal_uint32                   ul_tmp = 0;
    oal_uint32                   ul_ret;
    oal_uint32                   ul_off_set;
    oal_int8                     ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    hwifi_get_cfg_params();

    OAL_MEMZERO((oal_uint8*)&st_write_msg, OAL_SIZEOF(st_write_msg));
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_COEX, "{wal_hipriv_dev_customize_info::wal_get_cmd_one_arg_etc return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令 */
    if (0 == (oal_strcmp("1", ac_name)))
    {
        ul_tmp = 1;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SHOW_DEV_CUSTOMIZE_INFOS, OAL_SIZEOF(oal_int32));
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dev_customize_info::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}
#endif

#ifdef _PRE_WLAN_ONLINE_DPD

OAL_STATIC oal_uint32  wal_hipriv_dpd_cfg(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DPD, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_reg_write::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_dpd_start(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DPD_START, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_reg_write::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif


oal_uint32  wal_hipriv_set_txpower(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_ret;
    oal_int32                   l_pwer;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_val[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;
    oal_int32                   l_idx = 0;
    wal_msg_stru                *pst_rsp_msg = OAL_PTR_NULL;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_val, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_txpower::wal_get_cmd_one_arg_etc vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 输入命令合法性检测 */
    while ('\0' != ac_val[l_idx])
    {
        if (isdigit(ac_val[l_idx]))
        {
            l_idx++;
            continue;
        }
        else
        {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_txpower::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    l_pwer = oal_atoi(ac_val);

    if (l_pwer > WLAN_MAX_TXPOWER*10 || l_pwer < 0)   /* 参数异常: 功率限制大于1W */
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_txpower::invalid argument!}");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TX_POWER, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_pwer;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_txpower::return err code %d!}", l_ret);
        return (oal_uint32)l_ret;
    }
    /* 读取返回的错误码 */
    ul_ret = wal_check_and_release_msg_resp_etc(pst_rsp_msg);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_DFR, "{wal_hipriv_set_bw fail, err code[%u]!}\r\n", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44))

OAL_STATIC oal_uint32  wal_ioctl_set_beacon_interval(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_beacon_interval;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_beacon_interval[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;
    oal_int32                   l_ret;
    mac_vap_stru                *pst_mac_vap;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if(OAL_UNLIKELY(NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{wal_ioctl_set_beacon_interval::can't get mac vap from netdevice priv data!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设备在up状态不允许配置，必须先down */
    if (pst_mac_vap->en_vap_state != MAC_VAP_STATE_INIT)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_ioctl_set_beacon_interval::device is busy, please down it firs %d!}\r\n", pst_mac_vap->en_vap_state);
        return OAL_FAIL;
    }

    /* pc_param指向新创建的net_device的name, 将其取出存放到ac_name中 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_beacon_interval, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_ioctl_set_beacon_interval::wal_get_cmd_one_arg_etc vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    l_beacon_interval = oal_atoi(ac_beacon_interval);
    OAM_INFO_LOG1(0, OAM_SF_ANY, "{wal_ioctl_set_beacon_interval::l_beacon_interval = %d!}\r\n", l_beacon_interval);

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_BEACON_INTERVAL, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_beacon_interval;

    /* 发送消息 */
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_ioctl_set_beacon_interval::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_start_vap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    OAM_ERROR_LOG0(0, OAM_SF_CFG, "DEBUG:: priv start enter.");
    wal_netdev_open_etc(pst_net_dev,OAL_FALSE);
    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  wal_hipriv_amsdu_start(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_amsdu_start_param_stru  *pst_amsdu_start_param;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    /* 填写消息 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMSDU_START, OAL_SIZEOF(mac_cfg_amsdu_start_param_stru));

    /* 解析并设置配置命令参数 */
    pst_amsdu_start_param = (mac_cfg_amsdu_start_param_stru *)(st_write_msg.auc_value);

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_start::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, pst_amsdu_start_param->auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_start::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_amsdu_start_param->uc_amsdu_max_num     = (oal_uint8)oal_atoi(ac_name);

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_start::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_amsdu_start_param->us_amsdu_max_size    = (oal_uint16)oal_atoi(ac_name);

    /* 发送消息 */
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_amsdu_start_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_start::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_list_ap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LIST_AP, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_list_ap::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_list_sta(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LIST_STA, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_list_sta::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_list_channel(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LIST_CHAN, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_list_channel::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32 wal_hipriv_set_regdomain_pwr_priv(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;
    oal_int32                   l_ret = OAL_SUCC;
    oal_uint32                  ul_pwr;
    wal_msg_write_stru          st_write_msg;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "wal_hipriv_set_regdomain_pwr, get arg return err %d", ul_ret);
        return ul_ret;
    }

    ul_pwr = (oal_uint32)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REGDOMAIN_PWR, OAL_SIZEOF(oal_int32));

    ((mac_cfg_regdomain_max_pwr_stru *)st_write_msg.auc_value)->uc_pwr        = (oal_uint8)ul_pwr;
    ((mac_cfg_regdomain_max_pwr_stru *)st_write_msg.auc_value)->en_exceed_reg = OAL_TRUE;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);
    if (OAL_SUCC != l_ret)
    {
        OAM_WARNING_LOG1(0,OAM_SF_CFG,"{wal_hipriv_set_regdomain_pwr::wal_send_cfg_event_etc fail.return err code %d}",l_ret);
    }

    return (oal_uint32)l_ret;
}


OAL_STATIC oal_uint32  wal_hipriv_start_scan(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;
#ifdef _PRE_WLAN_FEATURE_P2P
    oal_uint8                    uc_is_p2p0_scan;
#endif  /* _PRE_WLAN_FEATURE_P2P */

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_START_SCAN, OAL_SIZEOF(oal_int32));

#ifdef _PRE_WLAN_FEATURE_P2P
    uc_is_p2p0_scan = (oal_memcmp(pst_net_dev->name, "p2p0", OAL_STRLEN("p2p0")) == 0)?1:0;
    //uc_is_p2p0_scan = (pst_net_dev->ieee80211_ptr->iftype == NL80211_IFTYPE_P2P_DEVICE)?1:0;
    st_write_msg.auc_value[0] = uc_is_p2p0_scan;
#endif  /* _PRE_WLAN_FEATURE_P2P */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_start_scan::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_start_join(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_int32                        l_ret;
    oal_uint32                       ul_ret;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_START_JOIN, OAL_SIZEOF(oal_int32));

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_start_join::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 将要关联AP的编号复制到事件msg中，AP编号是数字的ASSCI码，不超过4个字节 */
    oal_memcopy((oal_int8 *)st_write_msg.auc_value, (oal_int8 *)ac_name, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_start_join::return err codereturn err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_start_deauth(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_START_DEAUTH, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_start_deauth::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_dump_timer(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_TIEMR, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_timer::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_kick_user(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_kick_user_param_stru   *pst_kick_user_param;
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0,0,0,0,0,0};

    /* 去关联1个用户的命令 hipriv "vap0 kick_user xx:xx:xx:xx:xx:xx" */

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_kick_user::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_KICK_USER, OAL_SIZEOF(mac_cfg_kick_user_param_stru));

    /* 设置配置命令参数 */
    pst_kick_user_param = (mac_cfg_kick_user_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_kick_user_param->auc_mac_addr, auc_mac_addr);

    /* 填写去关联reason code */
    pst_kick_user_param->us_reason_code = MAC_UNSPEC_REASON;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_kick_user_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_kick_user::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_pause_tid(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_pause_tid_param_stru   *pst_pause_tid_param;
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0,0,0,0,0,0};
    oal_uint8                       uc_tid;
    /* 去关联1个用户的命令 hipriv "vap0 kick_user xx xx xx xx xx xx" */

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pause_tid::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pause_tid::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_tid = (oal_uint8)oal_atoi(ac_name);

    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PAUSE_TID, OAL_SIZEOF(mac_cfg_pause_tid_param_stru));

    /* 设置配置命令参数 */
    pst_pause_tid_param = (mac_cfg_pause_tid_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_pause_tid_param->auc_mac_addr, auc_mac_addr);
    pst_pause_tid_param->uc_tid = uc_tid;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pause_tid::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pst_pause_tid_param->uc_is_paused = (oal_uint8)oal_atoi(ac_name);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_pause_tid_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pause_tid::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL

OAL_STATIC oal_uint32  wal_hipriv_get_hipkt_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru  st_write_msg;
    oal_int32        l_ret           = OAL_SUCC;

    // sh hipriv.sh "wlan0 get_hipkt_stat"

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_HIPKT_STAT, OAL_SIZEOF(oal_uint8));

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_hipkt_stat:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_set_flowctl_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru  st_write_msg;
    oal_uint32       ul_ret          = OAL_SUCC;
    oal_uint32       ul_off_set      = 0;
    oal_int32        l_ret           = OAL_SUCC;
    oal_int8         ac_param[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    mac_cfg_flowctl_param_stru st_flowctl_param;
    mac_cfg_flowctl_param_stru *pst_param;

    // sh hipriv.sh "Hisilicon0 set_flowctl_param 0/1/2/3 20 20 40"
    // 0/1/2/3 分别代表be,bk,vi,vo

    /* 获取队列类型参数 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_param, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_flowctl_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    st_flowctl_param.uc_queue_type = (oal_uint8)oal_atoi(ac_param);

    /* 设置队列对应的每次调度报文个数 */
    pc_param += ul_off_set;
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_param, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_flowctl_param::wal_get_cmd_one_arg_etc burst_limit return err_code %d!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }
    st_flowctl_param.us_burst_limit = (oal_uint16)oal_atoi(ac_param);

    /* 设置队列对应的流控low_waterline */
    pc_param += ul_off_set;
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_param, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_flowctl_param::wal_get_cmd_one_arg_etc low_waterline return err_code %d!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }
    st_flowctl_param.us_low_waterline= (oal_uint16)oal_atoi(ac_param);


    /* 设置队列对应的流控high_waterline */
    pc_param += ul_off_set;
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_param, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_flowctl_param::wal_get_cmd_one_arg_etc high_waterline return err_code %d!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }
    st_flowctl_param.us_high_waterline = (oal_uint16)oal_atoi(ac_param);

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_FLOWCTL_PARAM, OAL_SIZEOF(mac_cfg_flowctl_param_stru));
    pst_param = (mac_cfg_flowctl_param_stru *)(st_write_msg.auc_value);

    pst_param->uc_queue_type = st_flowctl_param.uc_queue_type;
    pst_param->us_burst_limit= st_flowctl_param.us_burst_limit;
    pst_param->us_low_waterline = st_flowctl_param.us_low_waterline;
    pst_param->us_high_waterline = st_flowctl_param.us_high_waterline;


    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_flowctl_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_flowctl_param:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_get_flowctl_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru  st_write_msg;
    oal_int32        l_ret           = OAL_SUCC;

    // sh hipriv.sh "Hisilicon0 get_flowctl_stat"

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_FLOWCTL_STAT, OAL_SIZEOF(oal_uint8));

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_flowctl_stat:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}
#endif



OAL_STATIC oal_uint32  wal_hipriv_event_queue_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return frw_event_queue_info_etc();
}


OAL_STATIC oal_uint32  wal_hipriv_set_user_vip(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_user_vip_param_stru    *pst_user_vip_param;
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint8                       uc_vip_flag;

    /* 设置用户为vip用户: 0 代表非VIP用户，1代表VIP用户
       sh hipriv.sh "vap0 set_user_vip xx xx xx xx xx xx 0|1" */

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_user_vip::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_user_vip::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_vip_flag = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_USER_VIP, OAL_SIZEOF(mac_cfg_pause_tid_param_stru));

    /* 设置配置命令参数 */
    pst_user_vip_param = (mac_cfg_user_vip_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_user_vip_param->auc_mac_addr, auc_mac_addr);
    pst_user_vip_param->uc_vip_flag = uc_vip_flag;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_user_vip_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_user_vip::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_set_vap_host(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint8                       uc_host_flag;

    /* 设置vap的host flag: 0 代表guest vap, 1代表host vap
       sh hipriv.sh "vap0 set_host 0|1" */

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_vap_host::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_host_flag = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_VAP_HOST, OAL_SIZEOF(oal_uint8));

    /* 设置配置命令参数 */
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_host_flag;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_vap_host::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_send_bar(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_pause_tid_param_stru   *pst_pause_tid_param;
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0,0,0,0,0,0};
    oal_uint8                       uc_tid;

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_bar::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, auc_mac_addr);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_bar::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_tid = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_BAR, OAL_SIZEOF(mac_cfg_pause_tid_param_stru));

    /* 设置配置命令参数 */
    pst_pause_tid_param = (mac_cfg_pause_tid_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_pause_tid_param->auc_mac_addr, auc_mac_addr);
    pst_pause_tid_param->uc_tid = uc_tid;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_pause_tid_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_bar::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


oal_uint32  wal_hipriv_amsdu_tx_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_ampdu_tx_on_param_stru *pst_aggr_tx_on_param;
    oal_uint8                       uc_aggr_tx_on;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_tx_on::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_aggr_tx_on = (oal_uint8)oal_atoi(ac_name);


    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMSDU_TX_ON, OAL_SIZEOF(mac_cfg_ampdu_tx_on_param_stru));

    /* 设置配置命令参数 */
    pst_aggr_tx_on_param = (mac_cfg_ampdu_tx_on_param_stru *)(st_write_msg.auc_value);
    pst_aggr_tx_on_param->uc_aggr_tx_on = uc_aggr_tx_on;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ampdu_tx_on_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_amsdu_tx_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_frag_threshold(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                       ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    mac_cfg_frag_threshold_stru    *pst_threshold;
    oal_uint32                      ul_threshold = 0;

    /* 获取分片门限 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_frag_threshold::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    ul_threshold = (oal_uint16)oal_atoi(ac_name);
    pc_param += ul_off_set;

    if ((WLAN_FRAG_THRESHOLD_MIN > ul_threshold) ||
        (WLAN_FRAG_THRESHOLD_MAX < ul_threshold))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_frag_threshold::ul_threshold value error [%d]!}\r\n", ul_threshold);
        return OAL_FAIL;
    }

    pst_threshold = (mac_cfg_frag_threshold_stru *)(st_write_msg.auc_value);
    pst_threshold->ul_frag_threshold = ul_threshold;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(mac_cfg_frag_threshold_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FRAG_THRESHOLD_REG, us_len);

    l_cfg_rst = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_frag_threshold::return err code [%d]!}\r\n", l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_wmm_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_open_wmm = 0;

    /* 获取设定的值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wmm_switch::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_open_wmm = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;


    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_open_wmm;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_WMM_SWITCH, us_len);

    l_cfg_rst = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wmm_switch::return err code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_hide_ssid(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_hide_ssid = 0;

    /* 获取设定的值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_hide_ssid::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_hide_ssid = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;


    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_hide_ssid;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_HIDE_SSID, us_len);

    l_cfg_rst = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_hide_ssid::return err code [%d]!}\r\n", l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


oal_uint32  wal_hipriv_ampdu_tx_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_ampdu_tx_on_param_stru *pst_aggr_tx_on_param;
    oal_uint8                       uc_aggr_tx_on;
    oal_uint8                       uc_snd_type = 0;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_tx_on::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param += ul_off_set;

    uc_aggr_tx_on = (oal_uint8)oal_atoi(ac_name);

    /* 只有硬件聚合需要配置第二参数 */
    if ((uc_aggr_tx_on & BIT3) || (uc_aggr_tx_on & BIT2) || (uc_aggr_tx_on & BIT1))
    {
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
             OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_tx_on::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
             return ul_ret;
        }

        uc_snd_type = (oal_uint8)oal_atoi(ac_name);
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMPDU_TX_ON, OAL_SIZEOF(mac_cfg_ampdu_tx_on_param_stru));

    /* 设置配置命令参数 */
    pst_aggr_tx_on_param = (mac_cfg_ampdu_tx_on_param_stru *)(st_write_msg.auc_value);
    pst_aggr_tx_on_param->uc_aggr_tx_on = uc_aggr_tx_on;
    if (1 < uc_snd_type)
    {
        pst_aggr_tx_on_param->uc_snd_type = 1;
        pst_aggr_tx_on_param->en_aggr_switch_mode = AMPDU_SWITCH_BY_BA_LUT;
    }
    else
    {
        pst_aggr_tx_on_param->uc_snd_type = uc_snd_type;
        pst_aggr_tx_on_param->en_aggr_switch_mode = AMPDU_SWITCH_BY_DEL_BA;
    }

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ampdu_tx_on_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ampdu_tx_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATUER_PCIE_TEST

OAL_STATIC oal_uint32  wal_hipriv_pcie_test(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set=0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_pcie_test_stru         *pst_pcie_test_param;
    oal_int32                       ul_num=3;
    oal_uint32                      uc_tmp[3];

    for (l_ret = 0; l_ret < ul_num; l_ret++)
    {
        pc_param = pc_param + ul_off_set;
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
             OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pcie_test::wal_hipriv_pcie_test return err_code [%d]!}\r\n", ul_ret);
             return ul_ret;
        }
        uc_tmp[l_ret] = (oal_uint32)oal_atoi(ac_name);
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PCIE_TEST, OAL_SIZEOF(mac_cfg_pcie_test_stru));

    /* 设置配置命令参数 */
    pst_pcie_test_param = (mac_cfg_pcie_test_stru *)(st_write_msg.auc_value);
    pst_pcie_test_param->us_burst = (oal_uint16)uc_tmp[0];
    if(uc_tmp[1]>1 || uc_tmp[2]>1)
    {
        uc_tmp[1] = 0;
        uc_tmp[2] = 0;
    }
    pst_pcie_test_param->uc_wdata = uc_tmp[1];
    pst_pcie_test_param->uc_rdata = uc_tmp[2];
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_pcie_test_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pcie_test::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  wal_hipriv_reset_device(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RESET_HW,us_len );

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_reset_device::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_reset_operate(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    if (OAL_UNLIKELY(WAL_MSG_WRITE_MAX_LEN <= OAL_STRLEN(pc_param)))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_reset_operate:: pc_param overlength is %d}\n", OAL_STRLEN(pc_param));
        return OAL_FAIL;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);


    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RESET_HW_OPERATE, us_len);

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_reset_operate::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_UAPSD

OAL_STATIC oal_uint32  wal_hipriv_uapsd_debug(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    if (OAL_UNLIKELY(WAL_MSG_WRITE_MAX_LEN <= OAL_STRLEN(pc_param)))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_uapsd_debug:: pc_param overlength is %d}\n", OAL_STRLEN(pc_param));
        return OAL_FAIL;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_UAPSD_DEBUG,us_len );

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_uapsd_debug::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_DFT_STAT

OAL_STATIC oal_uint32  wal_hipriv_set_phy_stat_en(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oam_stats_phy_node_idx_stru     st_phy_stat_node_idx;
    oal_uint8                       uc_loop;

    /* sh hipriv.sh "Hisilicon0 phy_stat_en idx1 idx2 idx3 idx4" */
    for (uc_loop = 0; uc_loop < OAM_PHY_STAT_NODE_ENABLED_MAX_NUM; uc_loop++)
    {
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
             OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_phy_stat_en::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
             return ul_ret;
        }

        st_phy_stat_node_idx.auc_node_idx[uc_loop] = (oal_uint8)oal_atoi(ac_name);

        /* 检查参数是否合法，参数范围是1~16 */
        if (st_phy_stat_node_idx.auc_node_idx[uc_loop] < OAM_PHY_STAT_ITEM_MIN_IDX
            || st_phy_stat_node_idx.auc_node_idx[uc_loop] > OAM_PHY_STAT_ITEM_MAX_IDX)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY,
                             "{wal_hipriv_set_phy_stat_en::stat_item_idx invalid! should between 1 and 16!}.",
                             st_phy_stat_node_idx.auc_node_idx[uc_loop]);
            return OAL_ERR_CODE_INVALID_CONFIG;
        }

        pc_param += ul_off_set;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PHY_STAT_EN, OAL_SIZEOF(st_phy_stat_node_idx));

    /* 填写消息体，参数 */
    oal_memcopy(st_write_msg.auc_value, &st_phy_stat_node_idx, OAL_SIZEOF(st_phy_stat_node_idx));

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_phy_stat_node_idx),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_phy_stat_en::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_dbb_env_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint8                       uc_param;

    /* sh hipriv.sh "Hisilicon0 dbb_env_param 0|1" */

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dbb_env_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_param = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DBB_ENV_PARAM, OAL_SIZEOF(uc_param));

    /* 填写消息体，参数 */
    st_write_msg.auc_value[0] = uc_param;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(uc_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dbb_env_param::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_usr_queue_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_usr_queue_param_stru    st_usr_queue_param;

    /* sh hipriv.sh "vap_name usr_queue_stat XX:XX:XX:XX:XX:XX 0|1" */
    OAL_MEMZERO((oal_uint8*)&st_write_msg, OAL_SIZEOF(st_write_msg));
    OAL_MEMZERO((oal_uint8*)&st_usr_queue_param, OAL_SIZEOF(st_usr_queue_param));

    /* 获取用户mac地址 */
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, st_usr_queue_param.auc_user_macaddr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_usr_queue_stat::wal_hipriv_get_mac_addr_etc return [%d].}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_usr_queue_stat::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    st_usr_queue_param.uc_param = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_USR_QUEUE_STAT, OAL_SIZEOF(st_usr_queue_param));

    /* 填写消息体，参数 */
    oal_memcopy(st_write_msg.auc_value, &st_usr_queue_param, OAL_SIZEOF(st_usr_queue_param));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_usr_queue_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_usr_queue_stat::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_report_vap_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint8                       uc_param;

    /* sh hipriv.sh "vap_name vap _stat  0|1" */

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_vap_stat::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_param = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_VAP_STAT, OAL_SIZEOF(uc_param));

    /* 填写消息体，参数 */
    st_write_msg.auc_value[0] = uc_param;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(uc_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_vap_stat::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_report_all_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /* sh hipriv.sh "Hisilicon0 reprt_all_stat type(phy/machw/mgmt/irq/all)  0|1" */
    /* 获取repot类型 */
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));
    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';
    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);
    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALL_STAT, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_all_stat::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  wal_hipriv_set_ampdu_aggr_num(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru       st_write_msg;
    oal_uint32               ul_off_set;
    oal_int8                 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    mac_cfg_aggr_num_stru    st_aggr_num_ctl = {0};
    oal_uint32               ul_ret;
    oal_int32                l_ret;

    OAL_MEMZERO((oal_uint8*)&st_write_msg, OAL_SIZEOF(st_write_msg));
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_aggr_num::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param += ul_off_set;

    st_aggr_num_ctl.uc_aggr_num_switch = (oal_uint8)oal_atoi(ac_name);
    if (0 == st_aggr_num_ctl.uc_aggr_num_switch)
    {
        /* 不指定聚合个数时，聚合个数恢复为0 */
        st_aggr_num_ctl.uc_aggr_num = 0;
    }
    else
    {
        /* 获取聚合个数 */
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_aggr_num::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }

        st_aggr_num_ctl.uc_aggr_num = (oal_uint8)oal_atoi(ac_name);

        /* 超过聚合最大限制判断 */
        if (st_aggr_num_ctl.uc_aggr_num > WLAN_AMPDU_TX_MAX_NUM)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_aggr_num::exceed max aggr num [%d]!}\r\n", st_aggr_num_ctl.uc_aggr_num);
            return ul_ret;
        }
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_AGGR_NUM, OAL_SIZEOF(st_aggr_num_ctl));

    /* 填写消息体，参数 */
    oal_memcopy(st_write_msg.auc_value, &st_aggr_num_ctl, OAL_SIZEOF(st_aggr_num_ctl));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_aggr_num_ctl),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_aggr_num::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_stbc_cap(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_value;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_cfg_net_dev) || (OAL_PTR_NULL == pc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_set_stbc_cap::pst_cfg_net_dev or pc_param null ptr error %d, %d!}\r\n", pst_cfg_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* STBC设置开关的命令: hipriv "vap0 set_stbc_cap 0 | 1"
            此处将解析出"1"或"0"存入ac_name
    */

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_stbc_cap::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    /* 针对解析出的不同命令，设置TDLS禁用开关 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        ul_value = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        ul_value = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_stbc_cap::the set stbc command is error %d!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_STBC_CAP, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_value;

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_stbc_cap::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_set_ldpc_cap(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_value;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_cfg_net_dev) || (OAL_PTR_NULL == pc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_set_ldpc_cap::pst_cfg_net_dev or pc_param null ptr error %d, %d!}\r\n", pst_cfg_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* LDPC设置开关的命令: hipriv "vap0 set_ldpc_cap 0 | 1"
            此处将解析出"1"或"0"存入ac_name
    */

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ldpc_cap::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    /* 针对解析出的不同命令，设置TDLS禁用开关 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        ul_value = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        ul_value = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ldpc_cap::the set ldpc command is error %d!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_LDPC_CAP, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_value;

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ldpc_cap::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_txbf_cap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_value;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_net_dev) || (OAL_PTR_NULL == pc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_set_txbf_cap::v or pc_param null ptr error %d, %d!}\r\n", pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /************************************************************
      TXBF设置开关的命令: sh hipriv "vap0 set_txbf_cap 0 | 1 | 2 |3"
             bit0表示RX(bfee)能力     bit1表示TX(bfer)能力
    *************************************************************/

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if ((OAL_SUCC != ul_ret) || (0 == (oal_strcmp("help", ac_name))))
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_txbf_cap::set txbf cap [bit0 indicates RX(bfee), bit1 TX(bfer)], ret[%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    ul_value = (oal_uint32)oal_atoi(ac_name);

    if(ul_value > 3)
    {
         OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_txbf_cap::set txbf cap [0 ~ 3]!}\r\n", ul_value);
         return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TXBF_SWITCH, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_value;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_txbf_cap::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


oal_uint32  wal_hipriv_dump_rx_dscr_etc(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_value;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_rx_dscr_etc::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    ul_value = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_RX_DSCR, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_value;

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_rx_dscr_etc::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_dump_tx_dscr(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_value;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_tx_dscr::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    ul_value = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_TX_DSCR, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_value;

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_tx_dscr::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_dump_memory(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_addr[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int8                        ac_len[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_len;
    oal_uint32                      ul_addr;
    mac_cfg_dump_memory_stru       *pst_cfg;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_addr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_memory::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pc_param += ul_off_set;
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_len, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_memory::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    /* 地址字符串转成16位地址 */
    ul_addr = (oal_uint32)oal_strtol(ac_addr, 0, 16);
    ul_len  = (oal_uint32)oal_atoi(ac_len);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_MEMORY, OAL_SIZEOF(mac_cfg_dump_memory_stru));

    /* 设置配置命令参数 */

    pst_cfg = (mac_cfg_dump_memory_stru *)(st_write_msg.auc_value);

    pst_cfg->ul_addr = ul_addr;
    pst_cfg->ul_len  = ul_len;

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_dump_memory_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_memory::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_show_tx_dscr_addr(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
#ifdef _PRE_DEBUG_MODE
    oal_uint32                   ul_mem_idx;
    oal_uint16                   us_tx_dscr_idx;
    oal_mempool_tx_dscr_addr     *pst_tx_dscr_addr;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_cfg_net_dev) || OAL_UNLIKELY(OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_show_tx_dscr_addr::pst_net_dev or pc_param null ptr error [%d] [%d]!}\r\n", pst_cfg_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_tx_dscr_addr = oal_mem_get_tx_dscr_addr_etc();
    if (OAL_PTR_NULL == pst_tx_dscr_addr)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_show_tx_dscr_addr::pst_tx_dscr_addr is NULL!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_IO_PRINT("Allocated addr\n");
    for (ul_mem_idx = 0; ul_mem_idx < pst_tx_dscr_addr->us_tx_dscr_cnt; ul_mem_idx++)
    {
        OAL_IO_PRINT("[%d]0x%x\t", ul_mem_idx, (oal_uint32)pst_tx_dscr_addr->ul_tx_dscr_addr[ul_mem_idx]);
    }
    OAL_IO_PRINT("\n");

    OAL_IO_PRINT("Released addr\n");
    for (ul_mem_idx = 0; ul_mem_idx < OAL_TX_DSCR_ITEM_NUM; ul_mem_idx++)
    {
        if (0 != pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_released_addr)
        {
            OAL_IO_PRINT("Addr:0x%x\tFileId:%d\tLineNum:%d\tTimeStamp:%u\n",
                        (oal_uint32)pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_released_addr,
                        pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_release_file_id,
                        pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_release_line_num,
                        pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_release_ts);
        }
    }

    OAL_IO_PRINT("Tx complete int:\n");
    for (ul_mem_idx = 0; ul_mem_idx < OAL_TX_DSCR_ITEM_NUM; ul_mem_idx++)
    {
        if (0 != pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_tx_dscr_in_up_intr)
        {
            OAL_IO_PRINT("Up tx addr:0x%x\tts:%u  |  Dn tx addr:0x%x\tts:%u\n",
                        pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_tx_dscr_in_up_intr,
                        pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_up_intr_ts,
                        pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_tx_dscr_in_dn_intr,
                        pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_dn_intr_ts);
            OAL_IO_PRINT("tx dscr in q[%d] mpdu_num[%d]:", pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].uc_q_num,
                                                            pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].uc_mpdu_num);
            for (us_tx_dscr_idx = 0; us_tx_dscr_idx < OAL_MAX_TX_DSCR_CNT_IN_LIST && 0 != pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_tx_dscr_in_q[us_tx_dscr_idx]; us_tx_dscr_idx++)
            {
                OAL_IO_PRINT("0x%x\t", pst_tx_dscr_addr->ast_tx_dscr_info[ul_mem_idx].ul_tx_dscr_in_q[us_tx_dscr_idx]);
            }
            OAL_IO_PRINT("\n-------------------------------------------\n");
        }
    }
#endif
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_dump_ba_bitmap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_mpdu_ampdu_tx_param_stru *pst_aggr_tx_on_param;
    oal_uint8                       uc_tid;
    oal_uint8                       auc_ra_addr[WLAN_MAC_ADDR_LEN] = {0};

    /* 获取tid */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_ba_bitmap::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_tid = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取MAC地址字符串 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_ba_bitmap::get mac err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* 地址字符串转地址数组 */
    oal_strtoaddr(ac_name, auc_ra_addr);
    pc_param += ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_BA_BITMAP, OAL_SIZEOF(mac_cfg_mpdu_ampdu_tx_param_stru));

    /* 设置配置命令参数 */
    pst_aggr_tx_on_param = (mac_cfg_mpdu_ampdu_tx_param_stru *)(st_write_msg.auc_value);
    pst_aggr_tx_on_param->uc_tid        = uc_tid;
    oal_set_mac_addr(pst_aggr_tx_on_param->auc_ra_mac, auc_ra_addr);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_mpdu_ampdu_tx_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_ba_bitmap::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_packet_xmit(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_mpdu_ampdu_tx_param_stru *pst_aggr_tx_on_param;
    oal_uint8                       uc_packet_num;
    oal_uint8                       uc_tid;
    oal_uint16                      uc_packet_len;
    oal_uint8                       auc_ra_addr[WLAN_MAC_ADDR_LEN] = {0};

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_tid = (oal_uint8)oal_atoi(ac_name);
    if(uc_tid >= WLAN_TID_MAX_NUM)
    {
         return OAL_FAIL;
    }
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    uc_packet_num = (oal_uint8)oal_atoi(ac_name);

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_packet_len = (oal_uint16)oal_atoi(ac_name);
    if(uc_packet_len < 30)
    {
        return OAL_FAIL;
    }
    pc_param += ul_off_set;

    /* 获取MAC地址字符串 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::get mac err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* 地址字符串转地址数组 */
    oal_strtoaddr(ac_name, auc_ra_addr);
    pc_param += ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PACKET_XMIT, OAL_SIZEOF(mac_cfg_mpdu_ampdu_tx_param_stru));

    /* 设置配置命令参数 */
    pst_aggr_tx_on_param = (mac_cfg_mpdu_ampdu_tx_param_stru *)(st_write_msg.auc_value);
    pst_aggr_tx_on_param->uc_packet_num = uc_packet_num;
    pst_aggr_tx_on_param->uc_tid        = uc_tid;
    pst_aggr_tx_on_param->us_packet_len = uc_packet_len;
    oal_set_mac_addr(pst_aggr_tx_on_param->auc_ra_mac, auc_ra_addr);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_mpdu_ampdu_tx_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
OAL_STATIC oal_uint32  wal_hipriv_alg(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru  st_write_msg;  //FIXME : st_write_msg can only carry bytes less than 48
    oal_int32           l_ret;
    oal_uint32          ul_off_set;
    oal_int8            ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int8           *pc_tmp = (oal_int8 *)pc_param;
    oal_uint16          us_config_len;
    oal_uint16          us_param_len;

    mac_ioctl_alg_config_stru   st_alg_config;

    st_alg_config.uc_argc = 0;
    while(OAL_SUCC == wal_get_cmd_one_arg_etc(pc_tmp, ac_arg, &ul_off_set))
    {
        st_alg_config.auc_argv_offset[st_alg_config.uc_argc] = (oal_uint8)((oal_uint8)(pc_tmp - pc_param) + (oal_uint8)ul_off_set - (oal_uint8)OAL_STRLEN(ac_arg));
        pc_tmp += ul_off_set;
        st_alg_config.uc_argc++;

        if(st_alg_config.uc_argc > DMAC_ALG_CONFIG_MAX_ARG)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_alg::wal_hipriv_alg error, argc too big [%d]!}\r\n", st_alg_config.uc_argc);
            return OAL_FAIL;
        }
    }

    if(0 == st_alg_config.uc_argc)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_alg::argc=0!}\r\n");
        return OAL_FAIL;
    }

    us_param_len = (oal_uint16)OAL_STRLEN(pc_param);
    if(us_param_len > WAL_MSG_WRITE_MAX_LEN - 1 - OAL_SIZEOF(mac_ioctl_alg_config_stru) )
    {
        return OAL_FAIL;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    us_config_len = OAL_SIZEOF(mac_ioctl_alg_config_stru) + us_param_len+ 1;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG, us_config_len);
    oal_memcopy(st_write_msg.auc_value, &st_alg_config, OAL_SIZEOF(mac_ioctl_alg_config_stru));
    oal_memcopy(st_write_msg.auc_value + OAL_SIZEOF(mac_ioctl_alg_config_stru), pc_param, us_param_len + 1);
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_config_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_alg::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND

OAL_STATIC oal_uint32  wal_hipriv_waveapp_32plus_user_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;
    oal_uint32                   ul_ret;
    oal_uint32                   ul_off_set;
    oal_int8                     ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                    uc_enable_flag;
    oal_uint8                   *puc_value;

    /* hipriv "Hisilicon0 waveapp_enable 1" */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_waveapp_32plus_user_enable::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_enable_flag = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_WAVEAPP_32PLUS_USER_ENABLE, OAL_SIZEOF(oal_uint8));

    puc_value = (oal_uint8 *)(st_write_msg.auc_value);
    *puc_value = uc_enable_flag;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_waveapp_32plus_user_enable::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  wal_hipriv_show_stat_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)||defined(_PRE_PRODUCT_ID_HI110X_HOST)

    oam_stats_report_info_to_sdt_etc(OAM_OTA_TYPE_DEV_STAT_INFO);
    oam_stats_report_info_to_sdt_etc(OAM_OTA_TYPE_VAP_STAT_INFO);
#endif
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_show_vap_pkt_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;

    /***************************************************************************
                                 抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_VAP_PKT_STAT, OAL_SIZEOF(oal_uint32));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_vap_pkt_stat::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_intf_det_log(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                      st_write_msg;
    oal_uint32                              ul_off_set;
    oal_int8                                ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                              ul_ret;
    mac_ioctl_alg_intfdet_log_param_stru   *pst_alg_intfdet_log_param = OAL_PTR_NULL;
    wal_ioctl_alg_cfg_stru                  st_alg_cfg;
    oal_uint8                               uc_map_index = 0;
    oal_int32                               l_ret;
    //oal_bool_enum_uint8                     en_stop_flag = OAL_FALSE;

    pst_alg_intfdet_log_param = (mac_ioctl_alg_intfdet_log_param_stru *)(st_write_msg.auc_value);

    /* 获取配置参数名称 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_intf_det_log::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param = pc_param + ul_off_set;

    /* 寻找匹配的命令 */
    st_alg_cfg = g_ast_alg_cfg_map_etc[0];
    while(OAL_PTR_NULL != st_alg_cfg.pc_name)
    {
        if (0 == oal_strcmp(st_alg_cfg.pc_name, ac_name))
        {
            break;
        }
        st_alg_cfg = g_ast_alg_cfg_map_etc[++uc_map_index];
    }

    /* 没有找到对应的命令，则报错 */
    if( OAL_PTR_NULL == st_alg_cfg.pc_name)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_intf_det_log::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }

    /* 记录命令对应的枚举值 */
    pst_alg_intfdet_log_param->en_alg_cfg = g_ast_alg_cfg_map_etc[uc_map_index].en_alg_cfg;

    /* 区分获取特定帧功率和统计日志命令处理:获取功率只需获取帧名字 */
    if (MAC_ALG_CFG_INTF_DET_STAT_LOG_START == pst_alg_intfdet_log_param->en_alg_cfg)
    {
        /* 获取配置参数名称 */
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_intf_det_log::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }

        /* 记录参数 */
        pst_alg_intfdet_log_param->us_value = (oal_uint16)oal_atoi(ac_name);
        //en_stop_flag = OAL_TRUE;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG_PARAM, OAL_SIZEOF(mac_ioctl_alg_intfdet_log_param_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_alg_intfdet_log_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_clear_stat_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)||defined(_PRE_PRODUCT_ID_HI110X_HOST)
    oam_stats_clear_stat_info_etc();
#endif
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_user_stat_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)||defined(_PRE_PRODUCT_ID_HI110X_HOST)

    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;

    /* sh hipriv.sh "Hisilicon0 usr_stat_info usr_id" */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_user_stat_info::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    l_tmp = oal_atoi(ac_name);
    if ((l_tmp < 0) || (l_tmp >= WLAN_ACTIVE_USER_MAX_NUM))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_user_stat_info::user id invalid [%d]!}\r\n", l_tmp);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    oam_stats_report_usr_info_etc((oal_uint16)l_tmp);
#endif
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_timer_start(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint8                       uc_timer_switch;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_timer_start::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);

         return ul_ret;
    }

    uc_timer_switch = (oal_uint8)oal_atoi(ac_name);
    if (uc_timer_switch >= 2)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_timer_start::invalid choicee [%d]!}\r\n", uc_timer_switch);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TIMER_START, OAL_SIZEOF(oal_uint8));

    /* 设置配置命令参数 */
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_timer_switch;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_timer_start::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_show_profiling(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
#ifdef _PRE_PROFILING_MODE

    wal_msg_write_stru                st_write_msg;
    oal_uint32                        ul_off_set;
    oal_int8                          ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                        ul_ret;
    oal_int32                         l_ret;
    mac_cfg_show_profiling_param_stru *pst_show_profiling_param = {0};
    oal_int8                          uc_show_profiling_type;
    oal_int8                          uc_show_level;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_profiling::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_show_profiling_type = (oal_uint8)oal_atoi(ac_name);

    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_profiling::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_show_level = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SHOW_PROFILING, OAL_SIZEOF(mac_cfg_show_profiling_param_stru));

    /* 设置配置命令参数 */
    pst_show_profiling_param = (mac_cfg_show_profiling_param_stru *)(st_write_msg.auc_value);
    pst_show_profiling_param->uc_show_profiling_type = uc_show_profiling_type;
    pst_show_profiling_param->uc_show_level = uc_show_level;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_show_profiling_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_profiling::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

#endif

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_profiling_packet_add(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
#ifdef _PRE_PROFILING_MODE
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint8                       ul_value;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set;

    l_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != l_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_profiling::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", l_ret);
         return l_ret;
    }
    ul_value = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                           抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PROFILING_PACKET_ADD, OAL_SIZEOF(oal_int8));

    /* 设置配置命令参数 */
    *((oal_uint8 *)(st_write_msg.auc_value)) = ul_value;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                         WAL_MSG_TYPE_WRITE,
                         WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int8),
                         (oal_uint8 *)&st_write_msg,
                         OAL_FALSE,
                         OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_profiling_packet_add::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

#endif

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_DFT_STAT

OAL_STATIC oal_uint32  wal_hipriv_clear_vap_stat_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint8                   uc_vap_id;

    if (OAL_PTR_NULL == OAL_NET_DEV_PRIV(pst_net_dev))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_clear_vap_stat_info::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_vap_id = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;
    oam_stats_clear_vap_stat_info_etc(uc_vap_id);

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_DFR

OAL_STATIC oal_uint32 wal_hipriv_test_dfr_start(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                       ul_cfg_rst;
    oal_wireless_dev_stru           *pst_wdev;
    mac_wiphy_priv_stru             *pst_wiphy_priv;
    mac_device_stru                 *pst_mac_device;

    pst_wdev = OAL_NETDEVICE_WDEV(pst_net_dev);
    if(OAL_PTR_NULL == pst_wdev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_test_dfr_start::pst_wdev is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_wiphy_priv  = (mac_wiphy_priv_stru *)oal_wiphy_priv(pst_wdev->wiphy);
    if (OAL_PTR_NULL == pst_wiphy_priv)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_test_dfr_start::pst_wiphy_priv is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_device  = pst_wiphy_priv->pst_mac_device;
    if(OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_test_dfr_start::pst_mac_device is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }
    if ((!g_st_dfr_info_etc.bit_device_reset_enable) || g_st_dfr_info_etc.bit_device_reset_process_flag)
    {
        OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_hipriv_test_dfr_start::now DFR disabled or in DFR process, enable=%d, reset_flag=%d}",
            g_st_dfr_info_etc.bit_device_reset_enable, g_st_dfr_info_etc.bit_device_reset_process_flag);
        return OAL_ERR_CODE_RESET_INPROGRESS;
    }

    g_st_dfr_info_etc.bit_device_reset_enable       = OAL_TRUE;
    g_st_dfr_info_etc.bit_device_reset_process_flag = OAL_FALSE;
    g_st_dfr_info_etc.ul_netdev_num                 = 0;
    OAL_MEMZERO((oal_uint8 *)(g_st_dfr_info_etc.past_netdev), OAL_SIZEOF(g_st_dfr_info_etc.past_netdev[0]) * WLAN_VAP_SUPPORT_MAX_NUM_LIMIT);

    ul_cfg_rst = wal_dfr_excp_rx_etc(pst_mac_device->uc_device_id, 0);
    if (OAL_UNLIKELY(OAL_SUCC != ul_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_test_dfr_start::wal_send_cfg_event_etc return err_code [%d]!}\r\n", ul_cfg_rst);
      return ul_cfg_rst;
    }

    return OAL_SUCC;
}

#endif //_PRE_WLAN_FEATURE_DFR


OAL_STATIC oal_uint32  wal_hipriv_ar_log(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                      st_write_msg;
    oal_uint32                              ul_off_set;
    oal_int8                                ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                              ul_ret;
    mac_ioctl_alg_ar_log_param_stru         *pst_alg_ar_log_param = OAL_PTR_NULL;
    wal_ioctl_alg_cfg_stru                  st_alg_cfg;
    oal_uint8                               uc_map_index = 0;
    oal_int32                               l_ret;
    oal_bool_enum_uint8                     en_stop_flag = OAL_FALSE;

    pst_alg_ar_log_param = (mac_ioctl_alg_ar_log_param_stru *)(st_write_msg.auc_value);

    /* 获取配置参数名称 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ar_log::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;

    /* 寻找匹配的命令 */
    st_alg_cfg = g_ast_alg_cfg_map_etc[0];
    while(OAL_PTR_NULL != st_alg_cfg.pc_name)
    {
        if (0 == oal_strcmp(st_alg_cfg.pc_name, ac_name))
        {
            break;
        }
        st_alg_cfg = g_ast_alg_cfg_map_etc[++uc_map_index];
    }

    /* 没有找到对应的命令，则报错 */
    if( OAL_PTR_NULL == st_alg_cfg.pc_name)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ar_log::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }

    /* 记录命令对应的枚举值 */
    pst_alg_ar_log_param->en_alg_cfg = g_ast_alg_cfg_map_etc[uc_map_index].en_alg_cfg;

    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, pst_alg_ar_log_param->auc_mac_addr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ar_log::wal_hipriv_get_mac_addr_etc failed!}\r\n");
        return ul_ret;
    }
    pc_param += ul_off_set;

    while ((' ' == *pc_param) || ('\0' == *pc_param))
    {
        if ('\0' == *pc_param)
        {
            en_stop_flag = OAL_TRUE;
            break;
        }
        ++ pc_param;
    }

    /* 获取业务类型值 */
    if (OAL_TRUE != en_stop_flag)
    {
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
             OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ar_log::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
             return ul_ret;
        }

        pst_alg_ar_log_param->uc_ac_no = (oal_uint8)oal_atoi(ac_name);
        pc_param = pc_param + ul_off_set;

        en_stop_flag = OAL_FALSE;
        while ((' ' == *pc_param) || ('\0' == *pc_param))
        {
            if ('\0' == *pc_param)
            {
                en_stop_flag = OAL_TRUE;
                break;
            }
            ++ pc_param;
        }

        if (OAL_TRUE != en_stop_flag)
        {
            /* 获取参数配置值 */
            ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                 OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ar_log::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
                 return ul_ret;
            }

            /* 记录参数配置值 */
            pst_alg_ar_log_param->us_value = (oal_uint16)oal_atoi(ac_name);
        }
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG_PARAM, OAL_SIZEOF(mac_ioctl_alg_ar_log_param_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_alg_ar_log_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}

OAL_STATIC oal_uint32  wal_hipriv_txbf_log(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                      st_write_msg;
    oal_uint32                              ul_off_set;
    oal_int8                                ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                              ul_ret;
    mac_ioctl_alg_txbf_log_param_stru      *pst_alg_txbf_log_param = OAL_PTR_NULL;
    wal_ioctl_alg_cfg_stru                  st_alg_cfg;
    oal_uint8                               uc_map_index = 0;
    oal_int32                               l_ret;
    oal_bool_enum_uint8                     en_stop_flag = OAL_FALSE;
    pst_alg_txbf_log_param = (mac_ioctl_alg_txbf_log_param_stru *)(st_write_msg.auc_value);
    /* 获取配置参数名称 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_txbf_log::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    /* 寻找匹配的命令 */
    st_alg_cfg = g_ast_alg_cfg_map_etc[0];
    while(OAL_PTR_NULL != st_alg_cfg.pc_name)
    {
        if (0 == oal_strcmp(st_alg_cfg.pc_name, ac_name))
        {
            break;
        }
        st_alg_cfg = g_ast_alg_cfg_map_etc[++uc_map_index];
    }
    /* 没有找到对应的命令，则报错 */
    if( OAL_PTR_NULL == st_alg_cfg.pc_name)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ar_log::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }
    /* 记录命令对应的枚举值 */
    pst_alg_txbf_log_param->en_alg_cfg = g_ast_alg_cfg_map_etc[uc_map_index].en_alg_cfg;
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, pst_alg_txbf_log_param->auc_mac_addr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_txbf_log::wal_hipriv_get_mac_addr_etc failed!}\r\n");
        return ul_ret;
    }
    pc_param += ul_off_set;
    while ((' ' == *pc_param))
    {
        ++ pc_param;
        if ('\0' == *pc_param)
        {
            en_stop_flag = OAL_TRUE;
            break;
        }
    }
    /* 获取参数配置值 */
    if (OAL_TRUE != en_stop_flag)
    {
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
             OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_txbf_log::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
             return ul_ret;
        }

        pst_alg_txbf_log_param->uc_ac_no = (oal_uint8)oal_atoi(ac_name);
        pc_param = pc_param + ul_off_set;

        en_stop_flag = OAL_FALSE;
        while ((' ' == *pc_param) || ('\0' == *pc_param))
        {
            if ('\0' == *pc_param)
            {
                en_stop_flag = OAL_TRUE;
                break;
            }
            ++ pc_param;
        }

        if (OAL_TRUE != en_stop_flag)
        {
            /* 获取参数配置值 */
            ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                 OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_txbf_log::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
                 return ul_ret;
            }

            /* 记录参数配置值 */
            pst_alg_txbf_log_param->us_value = (oal_uint16)oal_atoi(ac_name);
        }
    }
    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG_PARAM, OAL_SIZEOF(mac_ioctl_alg_txbf_log_param_stru));
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_alg_txbf_log_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_ar_test(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                      st_write_msg;
    oal_uint32                              ul_offset;
    oal_int8                                ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                              ul_ret;
    mac_ioctl_alg_ar_test_param_stru       *pst_alg_ar_test_param = OAL_PTR_NULL;
    wal_ioctl_alg_cfg_stru                  st_alg_cfg;
    oal_uint8                               uc_map_index = 0;
    oal_int32                               l_ret;

    pst_alg_ar_test_param = (mac_ioctl_alg_ar_test_param_stru *)(st_write_msg.auc_value);

    /* 获取配置参数名称 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_offset);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ar_test::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_offset;

    /* 寻找匹配的命令 */
    st_alg_cfg = g_ast_alg_cfg_map_etc[0];
    while(OAL_PTR_NULL != st_alg_cfg.pc_name)
    {
        if (0 == oal_strcmp(st_alg_cfg.pc_name, ac_name))
        {
            break;
        }
        st_alg_cfg = g_ast_alg_cfg_map_etc[++uc_map_index];
    }

    /* 没有找到对应的命令，则报错 */
    if( OAL_PTR_NULL == st_alg_cfg.pc_name)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ar_test::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }

    /* 记录命令对应的枚举值 */
    pst_alg_ar_test_param->en_alg_cfg = g_ast_alg_cfg_map_etc[uc_map_index].en_alg_cfg;


    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, pst_alg_ar_test_param->auc_mac_addr, &ul_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_ar_test::wal_hipriv_get_mac_addr_etc failed!}\r\n");
        return ul_ret;
    }
    pc_param += ul_offset;

    /* 获取参数配置值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_offset);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ar_test::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    /* 记录参数配置值 */
    pst_alg_ar_test_param->us_value = (oal_uint16)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ALG_PARAM, OAL_SIZEOF(mac_ioctl_alg_ar_test_param_stru));
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_alg_ar_test_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP

OAL_STATIC oal_uint32  wal_hipriv_set_edca_opt_weight_sta(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru  st_write_msg;
    oal_uint8           uc_weight       = 0;
    oal_uint8          *puc_value       = 0;
    oal_uint32          ul_ret          = OAL_SUCC;
    oal_int32           l_ret           = OAL_SUCC;
    oal_uint32          ul_off_set      = 0;
    mac_vap_stru       *pst_mac_vap     = OAL_PTR_NULL;
    oal_int8            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    // sh hipriv.sh "vap0 set_edca_weight_sta 1"
    if (OAL_PTR_NULL == OAL_NET_DEV_PRIV(pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_edca_opt_weight_sta::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取mac_vap */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG0(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap:: only AP_MODE support}");
        return OAL_FAIL;
    }

    /* 获取参数值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /*lint -e734*/
    uc_weight = (oal_uint32)oal_atoi(ac_name);
   /*lint +e734*/
    /* 最大权重为3 */
    if (uc_weight > 3)
    {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "wal_hipriv_set_edca_opt_weight_sta: valid value is between 0 and %d", 3);
        return OAL_FAIL;
    }

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_EDCA_OPT_WEIGHT_STA, OAL_SIZEOF(oal_uint8));
    puc_value = (oal_uint8 *)(st_write_msg.auc_value);
    *puc_value = uc_weight;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_weight_sta:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_set_edca_opt_switch_ap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru  st_write_msg;
    oal_uint8        uc_flag         = 0;
    oal_uint8       *puc_value       = 0;
    oal_uint32       ul_ret          = OAL_SUCC;
    oal_uint32       ul_off_set      = 0;
    oal_int32        l_ret           = OAL_SUCC;
    mac_vap_stru    *pst_mac_vap     = OAL_PTR_NULL;
    oal_int8         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    // sh hipriv.sh "vap0 set_edca_switch_ap 1/0"
    if (OAL_PTR_NULL == OAL_NET_DEV_PRIV(pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_edca_opt_switch_ap::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取mac_vap */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode)
    {
       OAM_WARNING_LOG0(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap:: only AP_MODE support}");
       return OAL_FAIL;
    }

    /* 获取配置参数 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_flag = (oal_uint8)oal_atoi(ac_name);

    /* 非法配置参数 */
    if (uc_flag > 1)
    {
        OAM_WARNING_LOG0(0, OAM_SF_EDCA, "wal_hipriv_set_edca_opt_cycle_ap, invalid config, should be 0 or 1");
        return OAL_SUCC;
    }

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_EDCA_OPT_SWITCH_AP, OAL_SIZEOF(oal_uint8));
    puc_value = (oal_uint8 *)(st_write_msg.auc_value);
    *puc_value = uc_flag;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_switch_ap:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_edca_opt_cycle_ap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru  st_write_msg;
    oal_uint32       ul_cycle_ms     = 0;
    oal_uint32      *pul_value       = 0;
    oal_uint32       ul_ret          = OAL_SUCC;
    oal_int32        l_ret           = OAL_SUCC;
    oal_uint32       ul_off_set      = 0;
    mac_vap_stru    *pst_mac_vap     = OAL_PTR_NULL;
    oal_int8         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    // sh hipriv.sh "vap0 set_edca_cycle_ap 200"
    if (OAL_PTR_NULL == OAL_NET_DEV_PRIV(pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_edca_opt_cycle_ap::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取mac_vap */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG0(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap:: only AP_MODE support}");
        return OAL_FAIL;
    }

    /* 获取参数值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    ul_cycle_ms = (oal_uint32)oal_atoi(ac_name);

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_EDCA_OPT_CYCLE_AP, OAL_SIZEOF(oal_uint32));
    pul_value = (oal_uint32 *)(st_write_msg.auc_value);
    *pul_value = ul_cycle_ms;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_edca_opt_cycle_ap:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_hipriv_set_default_key(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    mac_setdefaultkey_param_stru  st_payload_params  = {0};
    oal_uint32                     ul_off_set;
    oal_int8                       ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                      l_ret;
    oal_uint32                     ul_ret;
    wal_msg_write_stru             st_write_msg;

    /*1.1 入参检查*/
    if ((OAL_PTR_NULL == pst_net_dev)|| (OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_set_default_key::Param Check ERROR,pst_netdev, pst_params %d, %d!}\r\n",pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }


    /* 获取key_index*/
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0,OAM_SF_ANY,"{wal_hipriv_test_add_key::wal_get_cmd_one_arg_etc fail.err code[%u]}",ul_ret);
        return ul_ret;
    }
    st_payload_params.uc_key_index = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取en_unicast*/
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0,OAM_SF_ANY,"{wal_hipriv_test_add_key::wal_get_cmd_one_arg_etc fail.err code[%u]}",ul_ret);
        return ul_ret;
    }
    st_payload_params.en_unicast = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取multicast*/
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0,OAM_SF_ANY,"{wal_hipriv_test_add_key::wal_get_cmd_one_arg_etc fail.err code[%u]}",ul_ret);
        return ul_ret;
    }
    st_payload_params.en_multicast = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;


    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    /*3.2 填写 msg 消息体 */
    oal_memcopy(st_write_msg.auc_value, &st_payload_params, OAL_SIZEOF(mac_setdefaultkey_param_stru));
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEFAULT_KEY, OAL_SIZEOF(mac_setdefaultkey_param_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_setdefaultkey_param_stru),
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_default_key::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
      return (oal_uint32)l_ret;
    }
    return OAL_SUCC;


}


OAL_STATIC oal_uint32 wal_hipriv_test_add_key(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru             st_write_msg;
    mac_addkey_param_stru          st_payload_params;
    oal_int8                       ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                      l_ret;
    oal_uint32                     ul_ret;
    oal_uint32                     ul_off_set;
    oal_uint8                      auc_key[OAL_WPA_KEY_LEN] = {0};
    oal_int8                      *pc_key;
    oal_uint32                     ul_char_index;
    oal_uint16                     us_len;
    wal_msg_stru                  *pst_rsp_msg;

    /*1.1 入参检查*/
    if ((OAL_PTR_NULL == pst_net_dev)|| (OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_test_add_key::Param Check ERROR,pst_netdev, pst_params %d, %d!}\r\n",pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    /*xxx(cipher) xx(en_pairwise) xx(key_len) xxx(key_index) xxxx:xx:xx:xx:xx:xx...(key 小于32字节) xx:xx:xx:xx:xx:xx(目的地址)  */

    oal_memset(&st_payload_params, 0, OAL_SIZEOF(st_payload_params));
    oal_memset(&st_payload_params.st_key, 0, OAL_SIZEOF(mac_key_params_stru));
    st_payload_params.st_key.seq_len = 6;
    OAL_MEMZERO(st_payload_params.auc_mac_addr, WLAN_MAC_ADDR_LEN);

    /* 获取cipher*/
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0,OAM_SF_ANY,"{wal_hipriv_test_add_key::wal_get_cmd_one_arg_etc fail.err code[%u]}",ul_ret);
        return ul_ret;
    }
    st_payload_params.st_key.cipher = (oal_uint32)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;


    /* 获取en_pairwise*/
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0,OAM_SF_ANY,"{wal_hipriv_test_add_key::wal_get_cmd_one_arg_etc fail.err code[%u]}",ul_ret);
        return ul_ret;
    }
    st_payload_params.en_pairwise = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取key_len */
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0,OAM_SF_ANY,"{wal_hipriv_test_add_key::wal_get_cmd_one_arg_etc fail.err code[%u]}",ul_ret);
        return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    st_payload_params.st_key.key_len = (oal_uint8)oal_atoi(ac_name);
    if ((st_payload_params.st_key.key_len > OAL_WPA_KEY_LEN) || (st_payload_params.st_key.key_len < 0) )
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_test_add_key::Param Check ERROR! key_len[%x]  }\r\n",
                      (oal_int32)st_payload_params.st_key.key_len);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* 获取key_index */
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0,OAM_SF_ANY,"{wal_hipriv_test_add_key::wal_get_cmd_one_arg_etc fail.err code[%u]}",ul_ret);
        return ul_ret;
    }
    st_payload_params.uc_key_index = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取key */
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0,OAM_SF_ANY,"{wal_hipriv_test_add_key::wal_get_cmd_one_arg_etc fail.err code[%u]}",ul_ret);
        return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    pc_key = ac_name;
    /* 16进制转换 */
    for (ul_char_index = 0; ul_char_index < ul_off_set; ul_char_index++)
    {
        if ('-' == *pc_key)
        {
            pc_key++;
            if (0 != ul_char_index)
            {
                ul_char_index--;
            }

            continue;
        }

        auc_key[ul_char_index/2] = (oal_uint8)(auc_key[ul_char_index/2] * 16 * (ul_char_index % 2) + oal_strtohex(pc_key));
        pc_key++;
    }
    oal_memcopy(st_payload_params.st_key.auc_key, auc_key, (oal_uint32)st_payload_params.st_key.key_len);


    /* 获取目的地址 */
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0,OAM_SF_ANY,"{wal_hipriv_test_add_key::wal_get_cmd_one_arg_etc fail.err code[%u]}",ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, st_payload_params.auc_mac_addr);

    OAM_INFO_LOG3(0, OAM_SF_ANY, "{wal_hipriv_test_add_key::key_len:%d, seq_len:%d, cipher:0x%08x!}\r\n",
                  st_payload_params.st_key.key_len, st_payload_params.st_key.seq_len, st_payload_params.st_key.cipher);


    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    /*3.2 填写 msg 消息体 */
    us_len = (oal_uint32)OAL_SIZEOF(mac_addkey_param_stru);
    oal_memcopy((oal_int8*)st_write_msg.auc_value, (oal_int8*)&st_payload_params, (oal_uint32)us_len);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_KEY, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_TRUE,
                             &pst_rsp_msg);
    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_enable_pmf::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
      return (oal_uint32)l_ret;
    }

    if (OAL_SUCC != wal_check_and_release_msg_resp_etc(pst_rsp_msg))
    {
        OAM_WARNING_LOG0(0,OAM_SF_ANY,"wal_hipriv_test_add_key::wal_check_and_release_msg_resp_etc fail");
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_set_thruput_bypass(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_ret;
    oal_int32                        l_cfg_rst;
    oal_uint16                       us_len;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_off_set = 0;
    oal_uint8                        uc_bypass_type = 0;
    oal_uint8                        uc_value = 0;
    mac_cfg_set_thruput_bypass_stru *pst_set_bypass;

    /* 获取设定mib名称 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_thruput_bypass::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_bypass_type = (oal_uint8)oal_atoi(ac_name);

    /* 获取设定置 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_thruput_bypass::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }
    pc_param += ul_off_set;
    uc_value = (oal_uint8)oal_atoi(ac_name);

    pst_set_bypass = (mac_cfg_set_thruput_bypass_stru *)(st_write_msg.auc_value);
    pst_set_bypass->uc_bypass_type   = uc_bypass_type;
    pst_set_bypass->uc_value = uc_value;
    us_len = OAL_SIZEOF(mac_cfg_set_thruput_bypass_stru);
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_THRUPUT_BYPASS, us_len);

    l_cfg_rst = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_thruput_bypass::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_tlv_cmd(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                        l_ret;
    oal_uint32                       ul_ret;
    wal_ioctl_tlv_stru               st_tlv_cfg;
    mac_cfg_set_tlv_stru            *pst_set_tlv;
    wal_msg_write_stru               st_write_msg;
    oal_uint8                        uc_len;
    oal_uint8                        uc_cmd_type = 0;
    oal_uint16                       us_cfg_id;
    oal_uint32                       ul_value = 0;
    oal_uint8                        uc_map_index = 0;

    /* 获取配置参数名称 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_tlv_cmd::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param += ul_off_set;

    /* 寻找匹配的命令 */
    st_tlv_cfg = g_ast_set_tlv_table[0];
    while(OAL_PTR_NULL != st_tlv_cfg.pc_name)
    {
        if (0 == oal_strcmp(st_tlv_cfg.pc_name, ac_name))
        {
            break;
        }
        st_tlv_cfg = g_ast_set_tlv_table[++uc_map_index];
    }

    /* 没有找到对应的命令，则报错 */
    if( OAL_PTR_NULL == st_tlv_cfg.pc_name)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_tlv_cmd::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }

    /* 记录命令对应的枚举值 */
    us_cfg_id = g_ast_set_tlv_table[uc_map_index].en_tlv_cfg_id;

    /* 获取设定mib名称 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_tlv_cmd::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    pc_param += ul_off_set;
    uc_cmd_type = (oal_uint8)oal_atoi(ac_name);

    /* 获取设定置 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_tlv_cmd::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    ul_value = (oal_uint32)oal_atoi(ac_name);

    uc_len = OAL_SIZEOF(mac_cfg_set_tlv_stru);

    pst_set_tlv = (mac_cfg_set_tlv_stru*)(st_write_msg.auc_value);
    pst_set_tlv->uc_cmd_type   = uc_cmd_type;
    pst_set_tlv->us_cfg_id     = us_cfg_id;
    pst_set_tlv->uc_len        = uc_len;
    pst_set_tlv->ul_value      = ul_value;

    OAM_WARNING_LOG4(0, OAM_SF_ANY, "{wal_hipriv_set_tlv_cmd:: cfg id[%d] cfg len[%d] cmd type[%d], set val[%d]!}",
                        us_cfg_id, uc_len, uc_cmd_type, ul_value);
    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, us_cfg_id, uc_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + uc_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_tlv_cmd::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_set_val_cmd(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                        l_ret;
    oal_uint32                       ul_ret;
    wal_ioctl_tlv_stru               st_tlv_cfg;
    mac_cfg_set_tlv_stru            *pst_set_tlv;
    wal_msg_write_stru               st_write_msg;
    oal_uint8                        uc_len;
    oal_uint16                       us_cfg_id;
    oal_uint32                       ul_value = 0;
    oal_uint8                        uc_map_index = 0;

    /* 获取配置参数名称 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_val_cmd::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param += ul_off_set;

    /* 寻找匹配的命令 */
    st_tlv_cfg = g_ast_set_tlv_table[0];
    while(OAL_PTR_NULL != st_tlv_cfg.pc_name)
    {
        if (0 == oal_strcmp(st_tlv_cfg.pc_name, ac_name))
        {
            break;
        }
        st_tlv_cfg = g_ast_set_tlv_table[++uc_map_index];
    }

    /* 没有找到对应的命令，则报错 */
    if( OAL_PTR_NULL == st_tlv_cfg.pc_name)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_val_cmd::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }

    /* 记录命令对应的枚举值 */
    us_cfg_id = g_ast_set_tlv_table[uc_map_index].en_tlv_cfg_id;

    /* 获取设定值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_val_cmd::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    pc_param += ul_off_set;
    ul_value = (oal_uint32)oal_atoi(ac_name);

    uc_len = OAL_SIZEOF(mac_cfg_set_tlv_stru);

    pst_set_tlv = (mac_cfg_set_tlv_stru*)(st_write_msg.auc_value);
    pst_set_tlv->uc_cmd_type   = 0xFF;
    pst_set_tlv->us_cfg_id     = us_cfg_id;
    pst_set_tlv->uc_len        = uc_len;
    pst_set_tlv->ul_value      = ul_value;

    OAM_WARNING_LOG3(0, OAM_SF_ANY, "{wal_hipriv_set_val_cmd:: cfg id[%d], cfg len[%d],set_val[%d]!}", us_cfg_id, uc_len, ul_value);
    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, us_cfg_id, uc_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + uc_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_val_cmd::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}

#ifdef _PRE_WLAN_DFT_STAT

OAL_STATIC oal_uint32  wal_hipriv_performance_log_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_ret;
    oal_int32                        l_cfg_rst;
    oal_uint16                       us_len;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_off_set = 0;
    oal_uint8                        uc_performance_switch_type = 0;
    oal_uint8                        uc_value = 0;
    mac_cfg_set_performance_log_switch_stru *pst_set_performance_log_switch;

    /* 获取设定mib名称 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_performance_log_switch::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    uc_performance_switch_type = (oal_uint8)oal_atoi(ac_name);

    /* 获取设定置 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_performance_log_switch::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }
    pc_param += ul_off_set;
    uc_value = (oal_uint8)oal_atoi(ac_name);

    pst_set_performance_log_switch = (mac_cfg_set_performance_log_switch_stru *)(st_write_msg.auc_value);
    pst_set_performance_log_switch->uc_performance_log_switch_type   = uc_performance_switch_type;
    pst_set_performance_log_switch->uc_value = uc_value;
    us_len = OAL_SIZEOF(mac_cfg_set_performance_log_switch_stru);
    OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_hipriv_performance_log_switch::uc_performance_switch_type = %d, uc_value = %d!}\r\n", uc_performance_switch_type,uc_value);
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_PERFORMANCE_LOG_SWITCH, us_len);

    l_cfg_rst = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_performance_log_switch::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_cfg_rst);
        return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  wal_hipriv_set_auto_protection(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint32                      ul_auto_protection_flag = 0;

    /* 获取mib名称 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_auto_protection::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    ul_auto_protection_flag = (oal_uint32)oal_atoi(ac_name);

    us_len = OAL_SIZEOF(oal_uint32);
    *(oal_uint32 *)(st_write_msg.auc_value) = ul_auto_protection_flag;
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_AUTO_PROTECTION, us_len);

    l_cfg_rst = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_auto_protection::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_WAPI

#ifdef _PRE_WAPI_DEBUG
OAL_STATIC oal_uint32  wal_hipriv_show_wapi_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    mac_vap_stru                    *pst_mac_vap;
    wal_msg_write_stru               st_write_msg;
    oal_int32                        l_ret;
    mac_cfg_user_info_param_stru    *pst_user_info_param;
    oal_uint8                        auc_mac_addr[6] = {0};    /* 临时保存获取的use的mac地址信息 */
    oal_uint8                        uc_char_index;
    oal_uint16                       us_user_idx;
    //OAL_IO_PRINT("wal_hipriv_show_wapi_info::enter\r\n");
    /* 去除字符串的空格 */
    pc_param++;

    /* 获取mac地址,16进制转换 */
    for (uc_char_index = 0; uc_char_index < 12; uc_char_index++)
    {
        if (':' == *pc_param)
        {
            pc_param++;
            if (0 != uc_char_index)
            {
                uc_char_index--;
            }

            continue;
        }

        auc_mac_addr[uc_char_index/2] =
        (oal_uint8)(auc_mac_addr[uc_char_index/2] * 16 * (uc_char_index % 2) +
                                        oal_strtohex(pc_param));
        pc_param++;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_WAPI_INFO, OAL_SIZEOF(mac_cfg_user_info_param_stru));

    /* 根据mac地址找用户 */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);

    l_ret = (oal_int32)mac_vap_find_user_by_macaddr_etc(pst_mac_vap, auc_mac_addr, &us_user_idx);
    if (OAL_SUCC != l_ret)
    {
        //OAL_IO_PRINT("wal_hipriv_show_wapi_info::no such user\r\n");
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_user_info::no such user!}\r\n");
        return OAL_FAIL;
    }

    /* 设置配置命令参数 */
    pst_user_info_param              = (mac_cfg_user_info_param_stru *)(st_write_msg.auc_value);
    pst_user_info_param->us_user_idx = us_user_idx;
    //OAL_IO_PRINT("wal_hipriv_show_wapi_info::us_user_idx %u\r\n", us_user_idx);

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_hipriv_show_wapi_info::us_user_idx %u", us_user_idx);
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_user_info_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_user_info::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif /* #ifdef WAPI_DEBUG_MODE */

#endif /* #ifdef _PRE_WLAN_FEATURE_WAPI */


#ifdef _PRE_WLAN_FEATURE_FTM
OAL_STATIC  OAL_INLINE oal_void ftm_debug_cmd_format_info(void)
{
    OAM_WARNING_LOG0(0, OAM_SF_FTM,
    "{CMD format::hipriv.sh wlan0 ftm_debug \
                  enable_ftm_initiator[0|1] \
                  enable[0|1] \
                  cali[0|1] \
                  enable_ftm_resp[0|1] \
                  set_ftm_time t1[] t2[] t3[] t4[] \
                  enable_ftm_resp[0|1] \
                  enable_ftm_range[0|1] \
                  get_cali \
                  !!}\r\n");
    OAM_WARNING_LOG0(0, OAM_SF_FTM,
    "{CMD format::hipriv.sh wlan0 ftm_debug \
                  send_iftmr channel[] single_burst[0|1] ftms_per_burst[n] measure_req[0|1] asap[0|1] bssid[xx:xx:xx:xx:xx:xx] \
                  send_ftm bssid[xx:xx:xx:xx:xx:xx] \
                  send_range_req mac[] num_rpt[0-65535] delay[] ap_cnt[] bssid[] channel[] bssid[] channel[] ...\
                  set_location type[] mac[] mac[] mac[] \
                  !!}\r\n");

}


OAL_STATIC oal_uint32 ftm_debug_parase_iftmr_cmd(oal_int8 *pc_param, mac_ftm_debug_switch_stru *pst_debug_info, oal_uint32 *pul_offset)
{

    oal_uint32                          ul_ret        = 0;
    oal_int8                            ac_value[WAL_HIPRIV_CMD_VALUE_MAX_LEN];
    oal_uint32                          ul_off_set    = 0;

    *pul_offset = 0;
    /*解析channel*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_iftmr_cmd::get iftmr mode error,return.}");
        return ul_ret;
    }

    *pul_offset += ul_off_set;
    pst_debug_info->st_send_iftmr_bit1.uc_channel_num = (oal_uint8)oal_atoi(ac_value);
    pc_param += ul_off_set;
    ul_off_set = 0;

    /*解析burst*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_iftmr_cmd::get burst error,return.}");
        return ul_ret;
    }
    *pul_offset += ul_off_set;
    pst_debug_info->st_send_iftmr_bit1.uc_burst_num  = (oal_uint8)oal_atoi(ac_value);
    pc_param += ul_off_set;
    ul_off_set = 0;

    /*解析ftms_per_burst*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_iftmr_cmd::get uc_ftms_per_burst error,return.}");
        return ul_ret;
    }
    *pul_offset += ul_off_set;
    pst_debug_info->st_send_iftmr_bit1.uc_ftms_per_burst  = (oal_uint8)oal_atoi(ac_value);
    pc_param += ul_off_set;
    ul_off_set = 0;

    /*解析measure_req*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_iftmr_cmd::get measure_req error,return.}");
        return ul_ret;
    }
    *pul_offset += ul_off_set;
    pst_debug_info->st_send_iftmr_bit1.measure_req  = (oal_uint8)oal_atoi(ac_value);
    pc_param += ul_off_set;
    ul_off_set = 0;

    /*解析asap*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_iftmr_cmd::get measure_req error,return.}");
        return ul_ret;
    }
    *pul_offset += ul_off_set;
    pst_debug_info->st_send_iftmr_bit1.en_asap  = (oal_uint8)oal_atoi(ac_value);
    pc_param += ul_off_set;
    ul_off_set = 0;

    /*解析bssid*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_iftmr_cmd::No bssid,set the associated bssid.}");
        *pul_offset += ul_off_set;
        oal_memset(pst_debug_info->st_send_iftmr_bit1.auc_bssid, 0, OAL_SIZEOF(pst_debug_info->st_send_iftmr_bit1.auc_bssid));
        pc_param += ul_off_set;
        ul_off_set = 0;

        return OAL_SUCC;
    }
    *pul_offset += ul_off_set;
    oal_strtoaddr(ac_value, pst_debug_info->st_send_iftmr_bit1.auc_bssid);
    pc_param += ul_off_set;
    ul_off_set = 0;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 ftm_debug_parase_ftm_cmd(oal_int8 *pc_param, mac_ftm_debug_switch_stru *pst_debug_info, oal_uint32 *pul_offset)
{

    oal_uint32                          ul_ret        = 0;
    oal_int8                            ac_value[WAL_HIPRIV_CMD_VALUE_MAX_LEN];
    oal_uint32                          ul_off_set    = 0;

    *pul_offset = 0;
    /*解析mac*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_ftm_cmd::ger mac error.}");
        return OAL_FAIL;
    }
    *pul_offset += ul_off_set;
    oal_strtoaddr(ac_value, pst_debug_info->st_send_ftm_bit4.auc_mac);
    pc_param += ul_off_set;
    ul_off_set = 0;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 ftm_debug_parase_ftm_time_cmd(oal_int8 *pc_param, mac_ftm_debug_switch_stru *pst_debug_info, oal_uint32 *pul_offset)
{

    oal_uint32                          ul_ret        = 0;
    oal_int8                            ac_value[WAL_HIPRIV_CMD_VALUE_MAX_LEN];
    oal_uint32                          ul_off_set    = 0;

    *pul_offset = 0;
    /*解析t1*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_ftm_time_cmd::get correct time1 error,return.}");
        return ul_ret;
    }

    *pul_offset += ul_off_set;
    pst_debug_info->st_ftm_time_bit6.ul_ftm_correct_time1 = (oal_uint32)oal_atoi(ac_value);
    pc_param += ul_off_set;
    ul_off_set = 0;

    /*解析t2*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_ftm_time_cmd::get correct time2 error,return.}");
        return ul_ret;
    }
    *pul_offset += ul_off_set;
    pst_debug_info->st_ftm_time_bit6.ul_ftm_correct_time2 = (oal_uint32)oal_atoi(ac_value);
    pc_param += ul_off_set;
    ul_off_set = 0;

    /*解析t3*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_ftm_time_cmd::get correct time3 error,return.}");
        return ul_ret;
    }
    *pul_offset += ul_off_set;
    pst_debug_info->st_ftm_time_bit6.ul_ftm_correct_time3 = (oal_uint32)oal_atoi(ac_value);
    pc_param += ul_off_set;
    ul_off_set = 0;

    /*解析t4*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_ftm_time_cmd::get correct time4 error,return.}");
        return ul_ret;
    }
    *pul_offset += ul_off_set;
    pst_debug_info->st_ftm_time_bit6.ul_ftm_correct_time4 = (oal_uint32)oal_atoi(ac_value);
    pc_param += ul_off_set;
    ul_off_set = 0;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 ftm_debug_parase_range_req_cmd(oal_int8 *pc_param, mac_ftm_debug_switch_stru *pst_debug_info, oal_uint32 *pul_offset)
{

    oal_uint32                          ul_ret        = 0;
    oal_int8                            ac_value[WAL_HIPRIV_CMD_VALUE_MAX_LEN];
    oal_uint32                          ul_off_set    = 0;
    oal_uint8                           uc_ap_cnt;

    *pul_offset = 0;

    /*解析mac*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_range_req_cmd::mac error!.}");
        return ul_ret;
    }
    *pul_offset += ul_off_set;
    oal_strtoaddr(ac_value, pst_debug_info->st_send_range_req_bit7.auc_mac);
    pc_param += ul_off_set;
    ul_off_set = 0;

    /*解析num_rpt*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_range_req_cmd::get num_rpt error,return.}");
        return ul_ret;
    }
    *pul_offset += ul_off_set;
    pst_debug_info->st_send_range_req_bit7.us_num_rpt = (oal_uint16)oal_atoi(ac_value);
    pc_param += ul_off_set;
    ul_off_set = 0;

    /*解析ap_cnt*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_range_req_cmd::get ap_cnt error,return.}");
        return ul_ret;
    }
    *pul_offset += ul_off_set;
    pst_debug_info->st_send_range_req_bit7.uc_minimum_ap_count  = (oal_uint8)oal_atoi(ac_value);
    pc_param += ul_off_set;
    ul_off_set = 0;

    for(uc_ap_cnt = 0; uc_ap_cnt < pst_debug_info->st_send_range_req_bit7.uc_minimum_ap_count; uc_ap_cnt++)
    {
        /*解析bssid*/
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
        if(OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_range_req_cmd::bssid error!.}");
            return ul_ret;
        }
        *pul_offset += ul_off_set;
        oal_strtoaddr(ac_value, pst_debug_info->st_send_range_req_bit7.aauc_bssid[uc_ap_cnt]);
        pc_param += ul_off_set;
        ul_off_set = 0;

        /*解析channel*/
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
        if(OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_range_req_cmd::get channel error,return.}");
            return ul_ret;
        }

        *pul_offset += ul_off_set;
        pst_debug_info->st_send_range_req_bit7.auc_channel[uc_ap_cnt] = (oal_uint8)oal_atoi(ac_value);
        pc_param += ul_off_set;
        ul_off_set = 0;

    }
    return OAL_SUCC;
}
#if 0

OAL_STATIC oal_uint32 ftm_debug_parase_location_cmd(oal_int8 *pc_param, mac_ftm_debug_switch_stru *pst_debug_info, oal_uint32 *pul_offset)
{

    oal_uint32                          ul_ret        = 0;
    oal_int8                            ac_value[WAL_HIPRIV_CMD_VALUE_MAX_LEN];
    oal_uint32                          ul_off_set    = 0;
    oal_uint8                           uc_ap_cnt;
    mac_location_stru                  *pst_location = &(pst_debug_info->st_location_bit10);

    *pul_offset = 0;

    /*解析type*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_location_cmd::get num_rpt error,return.}");
        return ul_ret;
    }
    *pul_offset += ul_off_set;
    pst_location->en_location_type = (oal_uint8)oal_atoi(ac_value);
    pc_param += ul_off_set;
    ul_off_set = 0;

    for(uc_ap_cnt = 0; uc_ap_cnt < MAX_REPEATER_NUM; uc_ap_cnt++)
    {
        /*解析mac*/
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
        if(OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_location_cmd::No bssid,set zero.}");
            *pul_offset += ul_off_set;
            OAL_MEMZERO(pst_location->auc_location_ap[uc_ap_cnt], WLAN_MAC_ADDR_LEN);
            pc_param += ul_off_set;
            ul_off_set = 0;

            continue;
        }
        *pul_offset += ul_off_set;
        oal_strtoaddr(ac_value, pst_location->auc_location_ap[uc_ap_cnt]);
        pc_param += ul_off_set;
        ul_off_set = 0;
    }

    return OAL_SUCC;
}
#endif

OAL_STATIC oal_uint32 ftm_debug_parase_m2s_cmd(oal_int8 *pc_param, mac_ftm_debug_switch_stru *pst_debug_info, oal_uint32 *pul_offset)
{

    oal_uint32                          ul_ret        = 0;
    oal_int8                            ac_value[WAL_HIPRIV_CMD_VALUE_MAX_LEN];
    oal_uint32                          ul_off_set    = 0;
    ftm_m2s_stru                       *pst_m2s = &(pst_debug_info->st_m2s_bit11);

    *pul_offset = 0;

    /*解析m2s*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_m2s_cmd::get m2s error,return.}");
        return ul_ret;
    }
    *pul_offset += ul_off_set;
    pst_m2s->en_is_mimo = (oal_uint8)oal_atoi(ac_value);
    pc_param += ul_off_set;
    ul_off_set = 0;

    /*解析tx_chain*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_FTM,"{ftm_debug_parase_m2s_cmd::get tx_chain error,return.}");
        return ul_ret;
    }
    *pul_offset += ul_off_set;
    pst_m2s->uc_tx_chain_selection = (oal_uint8)oal_atoi(ac_value);
    pc_param += ul_off_set;
    ul_off_set = 0;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_ftm(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_uint32                          ul_off_set    = 0;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int8                            ac_value[WAL_HIPRIV_CMD_VALUE_MAX_LEN];
    mac_ftm_debug_switch_stru           st_ftm_debug;
    oal_uint32                          ul_ret        = 0;
    oal_int32                           l_ret         = 0;

    OAL_MEMZERO(&st_ftm_debug, OAL_SIZEOF(st_ftm_debug));

    do
    {
        /*获取命令关键字*/
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if ((OAL_SUCC != ul_ret) && (0 != ul_off_set))
        {
            ftm_debug_cmd_format_info();
            return ul_ret;
        }
        pc_param += ul_off_set;

        if (0 == ul_off_set)
        {
            break;
        }

        /*命令分类*/
        if (0 == oal_strcmp("enable_ftm_initiator", ac_name))
        {
            /*取命令配置值*/
            ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
            if ((OAL_SUCC != ul_ret) || ('0' > ac_value[0]) || ('9' < ac_value[0]))
            {
                ftm_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            /*填写结构体*/
            st_ftm_debug.en_ftm_initiator_bit0 = ((oal_uint8)oal_atoi(ac_value));
            st_ftm_debug.ul_cmd_bit_map |= BIT0;
        }
        else if(0 == oal_strcmp("send_iftmr", ac_name))
        {
            ul_ret = ftm_debug_parase_iftmr_cmd(pc_param,&st_ftm_debug,&ul_off_set);
            if(OAL_SUCC != ul_ret)
            {
                ftm_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            st_ftm_debug.ul_cmd_bit_map |= BIT1;
        }
        else if(0 == oal_strcmp("enable", ac_name))
        {
            ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
            if ((OAL_SUCC != ul_ret) || ('0' > ac_value[0]) || ('9' < ac_value[0]))
            {
                ftm_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            /*填写结构体*/
            st_ftm_debug.en_enable_bit2 = ((oal_uint8)oal_atoi(ac_value));
            st_ftm_debug.ul_cmd_bit_map |= BIT2;
        }
        else if(0 == oal_strcmp("cali", ac_name))
        {
            ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
            if ((OAL_SUCC != ul_ret) || ('0' > ac_value[0]) || ('9' < ac_value[0]))
            {
                ftm_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            /*填写结构体*/
            st_ftm_debug.en_cali_bit3 = ((oal_uint8)oal_atoi(ac_value));
            st_ftm_debug.ul_cmd_bit_map |= BIT3;
        }
        else if(0 == oal_strcmp("send_ftm", ac_name))
        {
            ul_ret = ftm_debug_parase_ftm_cmd(pc_param,&st_ftm_debug,&ul_off_set);
            if(OAL_SUCC != ul_ret)
            {
                ftm_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            st_ftm_debug.ul_cmd_bit_map |= BIT4;
        }
        else if (0 == oal_strcmp("enable_ftm_resp", ac_name))
        {
            /*取命令配置值*/
            ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
            if ((OAL_SUCC != ul_ret) || ('0' > ac_value[0]) || ('9' < ac_value[0]))
            {
                ftm_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            /*填写结构体*/
            st_ftm_debug.en_ftm_resp_bit5 = ((oal_uint8)oal_atoi(ac_value));
            st_ftm_debug.ul_cmd_bit_map |= BIT5;
        }
        else if(0 == oal_strcmp("set_ftm_time", ac_name))
        {
            ul_ret = ftm_debug_parase_ftm_time_cmd(pc_param, &st_ftm_debug, &ul_off_set);
            if(OAL_SUCC != ul_ret)
            {
                ftm_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            st_ftm_debug.ul_cmd_bit_map |= BIT6;
        }
        else if(0 == oal_strcmp("send_range_req", ac_name))
        {
            ul_ret = ftm_debug_parase_range_req_cmd(pc_param,&st_ftm_debug,&ul_off_set);
            if(OAL_SUCC != ul_ret)
            {
                ftm_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            st_ftm_debug.ul_cmd_bit_map |= BIT7;
        }
        else if (0 == oal_strcmp("enable_ftm_range", ac_name))
        {
            /*取命令配置值*/
            ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_value, &ul_off_set);
            if ((OAL_SUCC != ul_ret) || ('0' > ac_value[0]) || ('9' < ac_value[0]))
            {
                ftm_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            /*填写结构体*/
            st_ftm_debug.en_ftm_range_bit8 = ((oal_uint8)oal_atoi(ac_value));
            st_ftm_debug.ul_cmd_bit_map |= BIT8;
        }
        else if(0 == oal_strcmp("get_cali", ac_name))
        {
            st_ftm_debug.ul_cmd_bit_map |= BIT9;
        }
        else if(0 == oal_strcmp("set_location", ac_name))
        {
            /*ul_ret = ftm_debug_parase_location_cmd(pc_param, &st_ftm_debug, &ul_off_set);
            if(OAL_SUCC != ul_ret)
            {
                ftm_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            st_ftm_debug.ul_cmd_bit_map |= BIT10;*/
        }
        else if(0 == oal_strcmp("set_ftm_m2s", ac_name))
        {
            ul_ret = ftm_debug_parase_m2s_cmd(pc_param, &st_ftm_debug, &ul_off_set);
            if(OAL_SUCC != ul_ret)
            {
                ftm_debug_cmd_format_info();
                return ul_ret;
            }
            pc_param += ul_off_set;
            ul_off_set = 0;
            st_ftm_debug.ul_cmd_bit_map |= BIT11;
        }
        else
        {
            ftm_debug_cmd_format_info();
            return OAL_FAIL;
        }
    }while(*pc_param != '\0');

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_ftm::ul_cmd_bit_map: 0x%08x.}", st_ftm_debug.ul_cmd_bit_map);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FTM_DBG, OAL_SIZEOF(st_ftm_debug));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,
                (const oal_void *)&st_ftm_debug,
                OAL_SIZEOF(st_ftm_debug));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_ftm_debug),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_FTM, "{wal_hipriv_ftm::return err code[%d]!}", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  wal_hipriv_send_2040_coext(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    oal_uint32                      ul_off_set = 0;
    mac_cfg_set_2040_coexist_stru   *pst_2040_coexist;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    pst_2040_coexist = (mac_cfg_set_2040_coexist_stru*)st_write_msg.auc_value;
    /* 获取mib名称 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_2040_coext::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_2040_coexist->ul_coext_info = (oal_uint32)oal_atoi(ac_name);

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_2040_coext::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_2040_coexist->ul_channel_report = (oal_uint32)oal_atoi(ac_name);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_2040_COEXT,
                    OAL_SIZEOF(mac_cfg_set_2040_coexist_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_2040_coexist_stru),
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_2040_coext::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
      return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_2040_coext_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    if (OAL_UNLIKELY(WAL_MSG_WRITE_MAX_LEN <= OAL_STRLEN(pc_param)))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_2040_coext_info:: pc_param overlength is %d}\n", OAL_STRLEN(pc_param));
        oal_print_hex_dump((oal_uint8 *)pc_param, WAL_MSG_WRITE_MAX_LEN, 32, "wal_hipriv_2040_coext_info: param is overlong:");
        return OAL_FAIL;
    }

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_2040_COEXT_INFO, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_2040_coext_info::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
      return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_get_version(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_VERSION, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_version::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY

OAL_STATIC oal_uint32  wal_hipriv_set_opmode_notify(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_value = 0;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_net_dev) || (OAL_PTR_NULL == pc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_OPMODE, "{wal_hipriv_set_opmode_notify::pst_net_dev or pc_param null ptr error %d, %d!}\r\n",  pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置工作模式通知能力: 0关闭 | 1开始  此处将解析出"1"或"0"存入ac_name */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_OPMODE, "{wal_hipriv_set_opmode_notify::wal_get_cmd_one_arg_etc return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，配置不同的通道 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        uc_value = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        uc_value = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_OPMODE, "{wal_hipriv_set_opmode_notify::opmode notify command is error [%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_value;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_OPMODE_NOTIFY, OAL_SIZEOF(oal_uint8));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_OPMODE, "{wal_hipriv_set_opmode_notify::wal_hipriv_reset_device return err code = [%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_get_user_nssbw(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_add_user_param_stru    *pst_add_user_param;
    mac_cfg_add_user_param_stru     st_add_user_param;  /* 临时保存获取的use的信息 */
    oal_uint32                      ul_get_addr_idx;

    /* 获取用户带宽和空间流信息: hipriv "vap0 add_user xx xx xx xx xx xx(mac地址)" */
    OAL_MEMZERO((oal_void *)&st_add_user_param, OAL_SIZEOF(mac_cfg_add_user_param_stru));
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_user::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, st_add_user_param.auc_mac_addr);

    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    /* 设置配置命令参数 */
    pst_add_user_param = (mac_cfg_add_user_param_stru *)(st_write_msg.auc_value);
    for (ul_get_addr_idx = 0; ul_get_addr_idx < WLAN_MAC_ADDR_LEN; ul_get_addr_idx++)
    {
        pst_add_user_param->auc_mac_addr[ul_get_addr_idx] = st_add_user_param.auc_mac_addr[ul_get_addr_idx];
    }

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_USER_RSSBW, OAL_SIZEOF(mac_cfg_add_user_param_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_add_user_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_add_user::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_M2S


oal_uint32  wal_hipriv_set_m2s_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    mac_m2s_mgr_stru               *pst_m2s_mgr;
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};

    /* st_write_msg作清零操作 */
    oal_memset(&st_write_msg, 0, OAL_SIZEOF(wal_msg_write_stru));

    /*设置配置命令参数 */
    pst_m2s_mgr = (mac_m2s_mgr_stru*)st_write_msg.auc_value;

    /* 1.获取第一个参数: mode */
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2S, "{wal_hipriv_set_m2s_switch::wal_get_cmd_one_arg1 return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    pst_m2s_mgr->en_cfg_m2s_mode = (mac_m2s_mode_enum_uint8)oal_atoi(ac_name);

    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    switch (pst_m2s_mgr->en_cfg_m2s_mode)
    {
        /* mimo-siso切换参数查询 */
        case MAC_M2S_MODE_QUERY:
            /* 抛事件dmac打印全局管理参数即可 */
            break;

        case MAC_M2S_MODE_MSS:
            /* 1.获取第二参数: 期望切换到的状态 */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_M2S, "{wal_hipriv_set_m2s_switch::wal_get_cmd_one_arg2 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_m2s_mgr->pri_data.mss_mode.en_mss_on = (oal_uint8)oal_atoi(ac_name);
            if(pst_m2s_mgr->pri_data.mss_mode.en_mss_on >= 2)
            {
                OAM_WARNING_LOG1(0, OAM_SF_M2S, "{wal_hipriv_set_m2s_switch::pst_m2s_mgr->pri_data.mss_mode.en_mss_on error [%d] (0 or 1)!}\r\n",
                    pst_m2s_mgr->pri_data.mss_mode.en_mss_on);
                return OAL_ERR_CODE_INVALID_CONFIG;
            }
            break;

        case MAC_M2S_MODE_DELAY_SWITCH:
        case MAC_M2S_MODE_SW_TEST:
        case MAC_M2S_MODE_HW_TEST:
            /* 1.获取第二参数: 期望切换到的状态 */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_M2S, "{wal_hipriv_set_m2s_switch::wal_get_cmd_one_arg2 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_m2s_mgr->pri_data.test_mode.uc_m2s_state = (oal_uint8)oal_atoi(ac_name);
            if(pst_m2s_mgr->pri_data.test_mode.uc_m2s_state >= MAC_M2S_COMMAND_STATE_BUTT)
            {
                OAM_WARNING_LOG1(0, OAM_SF_M2S, "{wal_hipriv_set_m2s_switch::pst_m2s_mgr->pri_data.test_mode.uc_m2s_state error [%d] (<=4)!}\r\n",
                    pst_m2s_mgr->pri_data.test_mode.uc_m2s_state);
                return OAL_ERR_CODE_INVALID_CONFIG;
            }

            /* 偏移，取下一个参数 */
            pc_param = pc_param + ul_off_set;

            /* 2.获取第三个参数:主路还是辅路 0为主路 1为辅路 暂时不使用 空缺TBD */
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                 OAM_WARNING_LOG1(0, OAM_SF_M2S, "{wal_hipriv_set_m2s_switch::wal_get_cmd_one_arg3 return err_code [%d]!}\r\n", ul_ret);
                 return ul_ret;
            }

            pst_m2s_mgr->pri_data.test_mode.uc_master_id = (oal_uint8)oal_atoi(ac_name);
            if(pst_m2s_mgr->pri_data.test_mode.uc_master_id >= HAL_DEVICE_ID_BUTT)
            {
                OAM_WARNING_LOG1(0, OAM_SF_M2S, "{wal_hipriv_set_m2s_switch:: pst_m2s_mgr->pri_data.test_mode.uc_master_id error [%d] 0/1!}\r\n",
                    pst_m2s_mgr->pri_data.test_mode.uc_master_id);
                return OAL_ERR_CODE_INVALID_CONFIG;
            }

            if(MAC_M2S_MODE_HW_TEST == pst_m2s_mgr->en_cfg_m2s_mode)
            {
                /* 硬切换测试模式采用默认软切换配置 */
                pst_m2s_mgr->pri_data.test_mode.en_m2s_type = WLAN_M2S_TYPE_HW;
            }
            else
            {
                /* 业务切换，软切换测试模式采用默认软切换配置 */
                pst_m2s_mgr->pri_data.test_mode.en_m2s_type = WLAN_M2S_TYPE_SW;
            }

            /* 标识业务类型 */
            if(MAC_M2S_MODE_DELAY_SWITCH == pst_m2s_mgr->en_cfg_m2s_mode)
            {
                pst_m2s_mgr->pri_data.test_mode.uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_COMMAND;
            }
            else
            {
                pst_m2s_mgr->pri_data.test_mode.uc_trigger_mode = WLAN_M2S_TRIGGER_MODE_TEST;
            }
            break;

        case MAC_M2S_MODE_RSSI:
            /* 1.获取第二个参数*/
            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_M2S, "{wal_hipriv_set_m2s_switch::wal_get_cmd_one_arg2 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_m2s_mgr->pri_data.rssi_mode.uc_opt = (oal_uint8)oal_atoi(ac_name);

            /* 偏移，取下一个参数 */
            pc_param = pc_param + ul_off_set;

            ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(0, OAM_SF_M2S, "{wal_hipriv_set_m2s_switch::wal_get_cmd_one_arg2 return err_code [%d]!}\r\n", ul_ret);
                return ul_ret;
            }

            pst_m2s_mgr->pri_data.rssi_mode.c_value = (oal_int8)oal_atoi(ac_name);

            break;
        default:
            OAM_WARNING_LOG1(0, OAM_SF_M2S, "{wal_hipriv_set_m2s_switch: en_cfg_m2s_mode[%d] error!}", pst_m2s_mgr->en_cfg_m2s_mode);
            return OAL_FAIL;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_M2S_SWITCH, OAL_SIZEOF(mac_m2s_mgr_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_m2s_mgr_stru),
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);
    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2S, "{wal_hipriv_set_m2s_stitch::wal_hipriv_reset_device return err code = [%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  wal_hipriv_set_vap_nss(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_vap_nss = 0;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_vap_nss::wal_get_cmd_one_arg_etc fails!}\r\n");
        return ul_ret;
    }

    pc_param += ul_off_set;
    uc_vap_nss = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_vap_nss;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_VAP_NSS, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_vap_nss::wal_hipriv_reset_device return err code = [%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)


OAL_STATIC oal_uint32  wal_hipriv_blacklist_add(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_off_set = 0;
    mac_blacklist_stru             *pst_blklst;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_blacklist_add:wal_get_cmd_one_arg_etc fail!}\r\n");
        return ul_ret;
    }
    OAL_MEMZERO((oal_uint8*)&st_write_msg, OAL_SIZEOF(st_write_msg));
    pst_blklst = (mac_blacklist_stru*)(st_write_msg.auc_value);

    oal_strtoaddr(ac_name, pst_blklst->auc_mac_addr); /* 将字符 ac_name 转换成数组 mac_add[6] */

    us_len = OAL_SIZEOF(mac_blacklist_stru);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_BLACK_LIST, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_blacklist_add:wal_send_cfg_event_etc return[%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_blacklist_del(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_off_set = 0;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_blacklist_add:wal_get_cmd_one_arg_etc fail!}\r\n");
        return ul_ret;
    }
    OAL_MEMZERO((oal_uint8*)&st_write_msg, OAL_SIZEOF(st_write_msg));

    oal_strtoaddr(ac_name, st_write_msg.auc_value); /* 将字符 ac_name 转换成数组 mac_add[6] */

    us_len = OAL_MAC_ADDR_LEN; /* OAL_SIZEOF(oal_uint8); */

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEL_BLACK_LIST, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_blacklist_add:wal_send_cfg_event_etc return[%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_blacklist_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data_etc(pst_net_dev,pc_param,WLAN_CFGID_BLACKLIST_MODE);
}

OAL_STATIC oal_uint32  wal_hipriv_blacklist_show(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    OAL_MEMZERO(&st_write_msg,OAL_SIZEOF(wal_msg_write_stru));

    us_len = 4; /* OAL_SIZEOF(oal_uint32) */
    *(oal_uint32 *)(st_write_msg.auc_value) = 1;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_BLACKLIST_SHOW, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_blacklist_show:wal_send_cfg_event_etc return [%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_hipriv_set_abl_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data_etc(pst_net_dev,pc_param,WLAN_CFGID_AUTOBLACKLIST_ON);
}

OAL_STATIC oal_uint32  wal_hipriv_set_abl_aging(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data_etc(pst_net_dev,pc_param,WLAN_CFGID_AUTOBLACKLIST_AGING);
}

OAL_STATIC oal_uint32  wal_hipriv_set_abl_threshold(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data_etc(pst_net_dev,pc_param,WLAN_CFGID_AUTOBLACKLIST_THRESHOLD);
}

OAL_STATIC oal_uint32  wal_hipriv_set_abl_reset(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data_etc(pst_net_dev,pc_param,WLAN_CFGID_AUTOBLACKLIST_RESET);
}
#endif
#ifdef _PRE_WLAN_FEATURE_ISOLATION

OAL_STATIC oal_uint32  wal_hipriv_set_isolation_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data_etc(pst_net_dev,pc_param,WLAN_CFGID_ISOLATION_MODE);
}

OAL_STATIC oal_uint32  wal_hipriv_set_isolation_type(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_bss = 0;
    oal_uint8                       uc_isolation = 0;
    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_isolation_type:wal_get_cmd_one_arg_etc fail!}\r\n");
        return ul_ret;
    }

    uc_bss = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_isolation_type::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    uc_isolation = (oal_uint8)oal_atoi(ac_name);

    OAL_MEMZERO((oal_uint8*)&st_write_msg, OAL_SIZEOF(st_write_msg));

    us_len = 2; /* OAL_SIZEOF(oal_uint8); */
    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ISOLATION_TYPE, us_len);

    /* 设置配置命令参数 */
    st_write_msg.auc_value[0] = uc_bss;
    st_write_msg.auc_value[1] = uc_isolation;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_isolation_type:wal_send_cfg_event_etc return[%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_hipriv_set_isolation_fwd(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data_etc(pst_net_dev,pc_param,WLAN_CFGID_ISOLATION_FORWARD);
}

OAL_STATIC oal_uint32  wal_hipriv_set_isolation_clear(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data_etc(pst_net_dev,pc_param,WLAN_CFGID_ISOLATION_CLEAR);
}

OAL_STATIC oal_uint32  wal_hipriv_set_isolation_show(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    return wal_hipriv_send_cfg_uint32_data_etc(pst_net_dev,pc_param,WLAN_CFGID_ISOLATION_SHOW);
}
#endif  /* _PRE_WLAN_FEATURE_CUSTOM_SECURITY */


OAL_STATIC oal_uint32  wal_hipriv_vap_classify_en(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_val = 0xff;
    wal_msg_write_stru              st_write_msg;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC == ul_ret)
    {
        ul_val = (oal_uint32)oal_atoi(ac_name);
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_VAP_CLASSIFY_EN, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_val;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);
    if (OAL_SUCC != l_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_send_cfg_event_etc fail.return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_vap_classify_tid(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_val = 0xff;
    wal_msg_write_stru              st_write_msg;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC == ul_ret)
    {
        ul_val = (oal_uint32)oal_atoi(ac_name);
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_VAP_CLASSIFY_TID, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_val;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);
    if (OAL_SUCC != l_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_send_cfg_event_etc fail.return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_STA_PM

OAL_STATIC oal_uint32  wal_hipriv_sta_psm_param(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_uint32                          ul_off_set;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                          ul_ret;
    oal_int32                           l_ret;
    oal_uint16                          us_beacon_timeout;
    oal_uint16                          us_tbtt_offset;
    oal_uint16                          us_ext_tbtt_offset;
    oal_uint16                          us_dtim3_on;
    mac_cfg_ps_param_stru               *pst_ps_para;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_psm_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    us_beacon_timeout = (oal_uint16)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_psm_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    us_tbtt_offset = (oal_uint16)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_psm_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    us_ext_tbtt_offset = (oal_uint16)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_psm_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    us_dtim3_on = (oal_uint16)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_PSM_PARAM, OAL_SIZEOF(mac_cfg_ps_param_stru));

    /* 设置配置命令参数 */
    pst_ps_para = (mac_cfg_ps_param_stru *)(st_write_msg.auc_value);
    pst_ps_para->us_beacon_timeout      = us_beacon_timeout;
    pst_ps_para->us_tbtt_offset         = us_tbtt_offset;
    pst_ps_para->us_ext_tbtt_offset     = us_ext_tbtt_offset;
    pst_ps_para->us_dtim3_on            = us_dtim3_on;

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ps_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_sta_psm_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



oal_uint32  wal_hipriv_sta_pm_on(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_uint32                          ul_off_set;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                          ul_ret;
    oal_int32                           l_ret;
    oal_uint8                           uc_sta_pm_open;
    mac_cfg_ps_open_stru               *pst_sta_pm_open;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_sta_pm_open::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_sta_pm_open = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_STA_PM_ON, OAL_SIZEOF(mac_cfg_ps_open_stru));

    /* 设置配置命令参数 */
    pst_sta_pm_open = (mac_cfg_ps_open_stru *)(st_write_msg.auc_value);
    /* MAC_STA_PM_SWITCH_ON / MAC_STA_PM_SWITCH_OFF */
    pst_sta_pm_open->uc_pm_enable      = uc_sta_pm_open;
    pst_sta_pm_open->uc_pm_ctrl_type   = MAC_STA_PM_CTRL_TYPE_CMD;

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_ps_open_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_sta_pm_open::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_hipriv_p2p_test(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_p2p_test::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param   += ul_off_set;

    /* 针对解析出的不同命令，对log模块进行不同的设置 */
    if (0 == (oal_strcmp("del_intf", ac_name)))
    {
        oal_uint32              ul_del_intf = 0;
        mac_vap_stru           *pst_mac_vap;
        hmac_device_stru       *pst_hmac_device;

        /* 获取参数 */
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_p2p_test::wal_get_cmd_one_arg_etc 1 return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
        pc_param   += ul_off_set;
        if (0 == (oal_strcmp("0", ac_name)))
        {
            ul_del_intf = 0;
        }
        else if (0 == (oal_strcmp("1", ac_name)))
        {
            ul_del_intf = 1;
        }
        else
        {
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_hipriv_p2p_test ::wrong parm.ul_del_intf\r\n}");
            return OAL_FAIL;
        }

        pst_mac_vap     = OAL_NET_DEV_PRIV(pst_net_dev);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_p2p_test::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
            return OAL_ERR_CODE_PTR_NULL;
        }
        pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
        if (pst_hmac_device == OAL_PTR_NULL)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
                        "{wal_hipriv_p2p_test::pst_hmac_device is null !device_id[%d]}",
                        pst_mac_vap->uc_device_id);
            return OAL_FAIL;
        }

        if (ul_del_intf == 1)
        {
            hmac_set_p2p_status_etc(&pst_hmac_device->ul_p2p_intf_status, P2P_STATUS_IF_DELETING);
        }
        else
        {
            hmac_clr_p2p_status_etc(&pst_hmac_device->ul_p2p_intf_status, P2P_STATUS_IF_DELETING);
        }

        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{wal_hipriv_p2p_test ::ctrl[%d], len:%d!}\r\n",
                        ul_del_intf, pst_hmac_device->ul_p2p_intf_status);

        OAL_SMP_MB();
        OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&pst_hmac_device->st_netif_change_event);

    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_p2p_test::the log switch command is error [%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    #if 0
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_p2p_test::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
    #endif

    return OAL_SUCC;
}
#endif  /* _PRE_DEBUG_MODE */
#endif  /* _PRE_WLAN_FEATURE_P2P */

#ifdef _PRE_WLAN_TCP_OPT

OAL_STATIC oal_uint32  wal_hipriv_get_tcp_ack_stream_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;


    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_TCP_ACK_STREAM_INFO, OAL_SIZEOF(oal_uint32));


    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);
    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_arpoffload_info::wal_send_cfg_event_etc fail.return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_tcp_tx_ack_opt_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_val = 0xff;
    wal_msg_write_stru              st_write_msg;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tcp_tx_ack_opt_enable::wal_get_cmd_one_arg_etc vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    pc_param += ul_off_set;

    ul_val = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TX_TCP_ACK_OPT_ENALBE, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_val;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);

    if (OAL_SUCC != l_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_arpoffload_info::wal_send_cfg_event_etc fail.return err code [%ud]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_tcp_rx_ack_opt_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_val = 0xff;
    wal_msg_write_stru              st_write_msg;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tcp_rx_ack_opt_enable::wal_get_cmd_one_arg_etc vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_val = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);


    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RX_TCP_ACK_OPT_ENALBE, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_val;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);
    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_arpoffload_info::wal_send_cfg_event_etc fail.return err code [%d]!}\r\n", l_ret);
         return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_hipriv_tcp_tx_ack_limit(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_val = 0xff;
    wal_msg_write_stru              st_write_msg;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tcp_tx_ack_limit::wal_get_cmd_one_arg_etc vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_val = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TX_TCP_ACK_OPT_LIMIT, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_val;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);
    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_arpoffload_info::wal_send_cfg_event_etc fail.return err code [%d]!}\r\n", l_ret);
         return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_hipriv_tcp_rx_ack_limit(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_val = 0xff;
    wal_msg_write_stru              st_write_msg;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_tcp_tx_ack_limit::wal_get_cmd_one_arg_etc vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_val = (oal_uint32)oal_atoi((const oal_int8 *)ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RX_TCP_ACK_OPT_LIMIT, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_val;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);
    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_arpoffload_info::wal_send_cfg_event_etc fail.return err code [%d]!}\r\n", l_ret);
         return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

OAL_STATIC oal_uint32  wal_hipriv_enable_2040bss(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_uint8                       uc_2040bss_switch;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_enable_2040bss::wal_get_cmd_one_arg_etc return err_code %d!}\r\n", ul_ret);
         return ul_ret;
    }

    if ((0 != oal_strcmp("0", ac_name)) && (0 != oal_strcmp("1", ac_name)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{wal_hipriv_enable_2040bss::invalid parameter.}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    uc_2040bss_switch = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_2040BSS_ENABLE, OAL_SIZEOF(oal_uint8));
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_2040bss_switch;  /* 设置配置命令参数 */

    ul_ret = (oal_uint32)wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_enable_2040bss::return err code %d!}\r\n", ul_ret);
    }

    return ul_ret;
}
#endif /* _PRE_WLAN_FEATURE_20_40_80_COEXIST */

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

OAL_STATIC oal_uint32  wal_hipriv_set_txrx_chain(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_chain = 0;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_txrx_chain::wal_get_cmd_one_arg_etc fails!}\r\n");
        return ul_ret;
    }

    pc_param += ul_off_set;
    uc_chain = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_chain;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_TXRX_CHAIN, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_txrx_chain::wal_hipriv_reset_device return err code = [%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_set_2g_txrx_path(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_2g_path = 0;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_txrx_chain::wal_get_cmd_one_arg_etc fails!}\r\n");
        return ul_ret;
    }

    pc_param += ul_off_set;
    uc_2g_path = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_2g_path;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_2G_TXRX_PATH, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    wal_hipriv_wait_rsp(pst_net_dev, pc_param);
#endif

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_2g_txrx_path::wal_hipriv_reset_device return err code = [%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN

OAL_STATIC oal_uint32  wal_hipriv_set_tx_classify_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru  st_write_msg;
    oal_uint8        uc_flag         = 0;
    oal_uint8       *puc_value       = 0;
    oal_uint32       ul_ret          = OAL_SUCC;
    oal_uint32       ul_off_set      = 0;
    oal_int32        l_ret           = OAL_SUCC;
    oal_int8         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    // sh hipriv.sh "p2p-p2p0-0 set_tx_classify_switch 1/0"

    /* 获取配置参数 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_tx_classify_switch::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_flag = (oal_uint8)oal_atoi(ac_name);
    /* 非法配置参数 */
    if (uc_flag > 1)
    {
        OAM_WARNING_LOG0(0, OAM_SF_EDCA, "wal_hipriv_set_tx_classify_switch::invalid config, should be 0 or 1");
        return OAL_SUCC;
    }

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TX_CLASSIFY_LAN_TO_WLAN_SWITCH, OAL_SIZEOF(oal_uint8));
    puc_value = (oal_uint8 *)(st_write_msg.auc_value);
    *puc_value = uc_flag;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_EDCA, "{wal_hipriv_set_tx_classify_switch:: return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif  /* _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN */


#ifdef _PRE_WLAN_FEATURE_AP_PM
OAL_STATIC oal_uint32  wal_hipriv_wifi_enable(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    /* OAM log模块的开关的命令: hipriv "Hisilicon0 enable 0 | 1"
        此处将解析出"1"或"0"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wifi_enable::wal_get_cmd_one_arg_etc return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对log模块进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
       OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_wifi_enable::command param is error!}\r\n");
       return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_WIFI_EN, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    wal_hipriv_wait_rsp(pst_cfg_net_dev, pc_param);
#endif

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wifi_enable::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_pm_info(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PM_INFO, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pm_info::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


oal_uint32  wal_hipriv_pm_enable(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;


    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wifi_enable::wal_get_cmd_one_arg_etc return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对log模块进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
       OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_wifi_enable::command param is error!}\r\n");
       return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PM_EN, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    wal_hipriv_wait_rsp(pst_cfg_net_dev, pc_param);
#endif

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pm_enable::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}
#endif

#ifdef _PRE_WLAN_CHIP_TEST

OAL_STATIC oal_uint32  wal_hipriv_beacon_offload_test(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    wal_msg_write_stru                  st_write_msg;
    oal_uint8                           *uc_param;
    oal_uint32                          ul_off_set = 0;
    oal_int32                           l_ret;
    oal_uint32                          ul_ret;
    oal_uint8                           i;

    uc_param = (oal_uint8*)st_write_msg.auc_value;

    /* hipriv "Hisilicon0 beacon_offload_test param0 param1 param2 param3", */
    for (i=0; i<4; i++)
    {
        pc_param += ul_off_set;
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_beacon_offload_test::wal_get_cmd_one_arg_etc param[%d] fails!}", i);
            return ul_ret;
        }
        *uc_param = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);
        uc_param++;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_BEACON_OFFLOAD_TEST, OAL_SIZEOF(wal_specific_event_type_param_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

oal_uint32  wal_hipriv_pci_reg_write(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_bus_chip_stru   *pst_bus_chip = OAL_PTR_NULL;
    oal_int8            *pc_token;
    oal_int8            *pc_end;
    oal_int8            *pc_ctx;
    oal_int8            *pc_sep = " ";
    oal_int32            ul_addr;
    oal_uint32           ul_val = 0;
    oal_uint8            uc_pci_device_id = 0;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_dev || OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_write::pst_net_dev or pc_param null ptr error [%d] [%d]!}\r\n", pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取芯片ID */
    pc_token = oal_strtok((oal_int8 *)pc_param, pc_sep, &pc_ctx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_write::pci_device_id null ptr error [%d]!}\r\n", pc_token);
        return OAL_FAIL;
    }
    uc_pci_device_id = (oal_uint8)oal_strtol(pc_token, &pc_end, 10);

    /* 获取寄存器地址 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_write::pul_addr null ptr errorr [%d]!}\r\n", pc_token);
        return OAL_FAIL;
    }

    ul_addr = (oal_int32)oal_strtol(pc_token, &pc_end, 16);

    /* 获取需要写入的值 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_write::ul_val null ptr error [%d]!}\r\n", pc_token);
        return OAL_FAIL;
    }

    ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 16);

    /* OAL_IO_PRINT("pci_reg_write: addr = 0x%x, val = 0x%x.\n", ul_addr, ul_val); */
    //OAL_SPRINTF(ac_buf, OAL_SIZEOF(ac_buf), "pci_reg_write: addr = 0x%08x, val = 0x%08x.\n", ul_addr, ul_val);
    //oam_print_etc(ac_buf);

    oal_bus_get_chip_instance(&pst_bus_chip, uc_pci_device_id);
    if(OAL_PTR_NULL == pst_bus_chip)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_write::pst_bus_chip is null.}\r\n");
        return OAL_FAIL;
    }

    oal_pci_write_config_dword(pst_bus_chip->pst_pci_device, ul_addr, ul_val);

    OAL_IO_PRINT("pci_reg_write succ: addr = 0x%x, val = 0x%x.\n", ul_addr, ul_val);

    return OAL_SUCC;
}


oal_uint32  wal_hipriv_pci_reg_read(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_bus_chip_stru   *pst_bus_chip = OAL_PTR_NULL;
    oal_int8       *pc_token;
    oal_int8       *pc_end;
    oal_int8       *pc_ctx;
    oal_int8       *pc_sep = " ";
    oal_int32       ul_addr;
    oal_uint32      ul_val = 0;
    oal_uint8       uc_pci_device_id = 0;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_dev || OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_read::pst_net_dev or pc_param null ptr error [%d] [%d]!}\r\n", pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取芯片ID */
    pc_token = oal_strtok((oal_int8 *)pc_param, pc_sep, &pc_ctx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_read::pci_device_id null ptr error [%d]!}\r\n", pc_token);
        return OAL_FAIL;
    }
    /* 读取那个芯片的pcie */
    uc_pci_device_id = (oal_uint8)oal_strtol(pc_token, &pc_end, 10);

    /* 获取寄存器地址 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (NULL == pc_token)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_read::ul_addr null ptr error [%d]!}\r\n", pc_token);
        return OAL_FAIL;
    }

    ul_addr = (oal_int32)oal_strtol(pc_token, &pc_end, 16);

    oal_bus_get_chip_instance(&pst_bus_chip, uc_pci_device_id);
    if(OAL_PTR_NULL == pst_bus_chip)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pci_reg_read::pst_bus_chip is null，uc_pci_device_id[0].}\r\n", uc_pci_device_id);
        return OAL_FAIL;
    }

    oal_pci_read_config_dword(pst_bus_chip->pst_pci_device, ul_addr, &ul_val);

    //OAL_SPRINTF(ac_buf, OAL_SIZEOF(ac_buf), "pci_reg_read: addr=0x%08x, val=0x%08x.\n", ul_addr, ul_val);
    //oam_print_etc(ac_buf);
    OAL_IO_PRINT("pci_reg_read succ:addr = 0x%x, val = 0x%x\n", ul_addr, ul_val);

    return OAL_SUCC;
}


oal_uint32  wal_hipriv_5115_reg_write(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_bus_chip_stru   *pst_bus_chip = OAL_PTR_NULL;
    oal_int8             *pc_token;
    oal_int8             *pc_end;
    oal_int8             *pc_ctx;
    oal_int8             *pc_sep = " ";
    oal_uint8             uc_pci_device_id = 0;

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_uint              ul_flag;
#endif

    struct wal_reg_write_stru
    {
        oal_int8     *pc_reg_type;
        oal_uint32    ul_addr;
        oal_uint32    ul_val;
    }st_reg_write = {0};

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_dev || OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_write::pst_net_dev or pc_param null ptr error [%d] [%d]!}\r\n", pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取芯片ID */
    pc_token = oal_strtok((oal_int8 *)pc_param, pc_sep, &pc_ctx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_write::pci_device_id null ptr error [%d]!}\r\n", pc_token);
        return OAL_FAIL;
    }
    uc_pci_device_id = (oal_uint8)oal_strtol(pc_token, &pc_end, 10);

      /* 参数检查 */
    /*lint -e960*/
    if ((0 != oal_strcmp(pc_token, "0")) && (0 != oal_strcmp(pc_token, "1")))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_write::err pci_device_id!}\r\n");
        return OAL_FAIL;
    }

    /* 获取要读取的寄存器类型 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (OAL_PTR_NULL == pc_token)
    {
        return OAL_FAIL;
    }

      /* 参数检查 */
    if ((0 != oal_strcmp(pc_token, "sys")) && (0 != oal_strcmp(pc_token, "pcie")))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_write::err reg typer!}\r\n");
        return OAL_FAIL;
    }
    /*lint +e960*/
    st_reg_write.pc_reg_type = pc_token;

    /* 获取寄存器地址 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (OAL_PTR_NULL == pc_token)
    {
        return OAL_FAIL;
    }

    st_reg_write.ul_addr = (oal_uint32)oal_strtol(pc_token, &pc_end, 16);

    /* 获取需要写入的值 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (OAL_PTR_NULL == pc_token)
    {
        return OAL_FAIL;
    }

    st_reg_write.ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 16);

    //OAL_IO_PRINT("write 5115/5610 reg, addr = 0x%x, val = 0x%x.\n", st_reg_write.ul_addr, st_reg_write.ul_val);
    //OAL_SPRINTF(ac_buf, OAL_SIZEOF(ac_buf), "write 5115/5610 reg, addr = 0x%08x, val = 0x%08x.\n", st_reg_write.ul_addr, st_reg_write.ul_val);
    //oam_print_etc(ac_buf);

    oal_bus_get_chip_instance(&pst_bus_chip, uc_pci_device_id);
    if(OAL_PTR_NULL == pst_bus_chip)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_write::pst_bus_chip is null.}\r\n");
        return OAL_FAIL;
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if ((0 == oal_strcmp(st_reg_write.pc_reg_type, "sys")))   /* sys ctl */
    {
    #ifdef _PRE_WLAN_FEATURE_PCIE_ADAPT_5116
        /* chip id 1 */
        if (1 == uc_pci_device_id)
        {
            oal_writel(st_reg_write.ul_val, g_pst_5115_sys_ctl1 + (st_reg_write.ul_addr - OAL_PCIE_SYS_BASE_PHYS - OAL_PCIE_PORT_OFFSET));
        }
        else
    #endif
        {
            oal_writel(st_reg_write.ul_val, g_pst_5115_sys_ctl + (st_reg_write.ul_addr - OAL_PCIE_SYS_BASE_PHYS));
        }
    }
    else
    {
        oal_irq_save(&ul_flag, OAL_5115IRQ_WH5RW);

        /* 配置工作模式，写cpu侧 */
        oal_pcie_dbi_enable(uc_pci_device_id);

        if(0 == uc_pci_device_id)
        {
            oal_writel(st_reg_write.ul_val, pst_bus_chip->p_pci_dbi_base + (st_reg_write.ul_addr - OAL_DBI_BASE_ADDR_0));
        }
        else /* chip id 1 */
        {
            oal_writel(st_reg_write.ul_val, pst_bus_chip->p_pci_dbi_base + (st_reg_write.ul_addr - OAL_DBI_BASE_ADDR_1));
        }

        /* 配置工作模式，恢复写wifi侧 */
        oal_pcie_dbi_disable(uc_pci_device_id);

        oal_irq_restore(&ul_flag, OAL_5115IRQ_WH5RW);
    }
#endif

    OAL_IO_PRINT("write 5115/5610 reg succ, addr = 0x%x, val = 0x%x.\n", st_reg_write.ul_addr, st_reg_write.ul_val);

    return OAL_SUCC;
}


oal_uint32  wal_hipriv_5115_reg_read(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_bus_chip_stru   *pst_bus_chip = OAL_PTR_NULL;
    oal_int8             *pc_token;
    oal_int8             *pc_end;
    oal_int8             *pc_ctx;
    oal_int8             *pc_sep = " ";
    oal_uint8            uc_pci_device_id = 0;

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_uint              ul_flag;
#endif

    struct wal_reg_info_stru
    {
        oal_int8     *pc_reg_type;
        oal_uint32    ul_addr;
        oal_uint32    ul_val;
    }st_reg_info = {0};

     /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_dev || OAL_PTR_NULL == pc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_read::pst_net_dev or pc_param null ptr error [%d] [%d]!}\r\n", pst_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取芯片ID */
    pc_token = oal_strtok((oal_int8 *)pc_param, pc_sep, &pc_ctx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_write::pci_device_id null ptr error [%d]!}\r\n", pc_token);
        return OAL_FAIL;
    }
    uc_pci_device_id = (oal_uint8)oal_strtol(pc_token, &pc_end, 10);

      /* 参数检查 */
    /*lint -e960*/
    if ((0 != oal_strcmp(pc_token, "0")) && (0 != oal_strcmp(pc_token, "1")))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_write::err pci_device_id!}\r\n");
        return OAL_FAIL;
    }

    /* 获取要读取的寄存器类型 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (OAL_PTR_NULL == pc_token)
    {
        return OAL_FAIL;
    }

    /* 参数检查 */
    if ((0 != oal_strcmp(pc_token, "sys")) && (0 != oal_strcmp(pc_token, "pcie")))
    {
        return OAL_FAIL;
    }
    /*lint +e960*/

    st_reg_info.pc_reg_type = pc_token;

    /* 获取地址 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (OAL_PTR_NULL == pc_token)
    {
        return OAL_FAIL;
    }

    st_reg_info.ul_addr = (oal_uint32)oal_strtol(pc_token, &pc_end, 16);

    oal_bus_get_chip_instance(&pst_bus_chip, uc_pci_device_id);
    if(OAL_PTR_NULL == pst_bus_chip)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_5115_reg_write::pst_bus_chip is null.}\r\n");
        return OAL_FAIL;
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if ((0 == oal_strcmp(st_reg_info.pc_reg_type, "sys")))   /* sys ctl */
    {
    #ifdef _PRE_WLAN_FEATURE_PCIE_ADAPT_5116
        if(1 == uc_pci_device_id)
        {
            st_reg_info.ul_val = oal_readl(g_pst_5115_sys_ctl1 + (st_reg_info.ul_addr - OAL_PCIE_SYS_BASE_PHYS - OAL_PCIE_PORT_OFFSET));
        }
        else
    #endif
        {
            st_reg_info.ul_val = oal_readl(g_pst_5115_sys_ctl + (st_reg_info.ul_addr - OAL_PCIE_SYS_BASE_PHYS));
        }
    }
    else   /* pcie0 */
    {
        oal_irq_save(&ul_flag, OAL_5115IRQ_WH5RR);

        /* 配置工作模式，读cpu侧 */
        oal_pcie_dbi_enable (uc_pci_device_id);
        if(0 == uc_pci_device_id)
        {
            st_reg_info.ul_val = oal_readl(pst_bus_chip->p_pci_dbi_base + (st_reg_info.ul_addr - OAL_DBI_BASE_ADDR_0));
        }
        else
        {
            st_reg_info.ul_val = oal_readl(pst_bus_chip->p_pci_dbi_base + (st_reg_info.ul_addr - OAL_DBI_BASE_ADDR_1));
        }
        /* 配置工作模式，恢复读wifi侧 */
        oal_pcie_dbi_disable (uc_pci_device_id);

        oal_irq_restore(&ul_flag, OAL_5115IRQ_WH5RR);
    }
#endif

    //OAL_SPRINTF(ac_buf, OAL_SIZEOF(ac_buf), "read 5115/5610 reg, addr = 0x%08x, val = 0x%08x.\n", st_reg_info.ul_addr, st_reg_info.ul_val);
    //oam_print_etc(ac_buf);
    OAL_IO_PRINT("read 5115 reg succ, addr = 0x%x, val = 0x%x.\n", st_reg_info.ul_addr, st_reg_info.ul_val);

    return OAL_SUCC;
}
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */

#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
#ifdef _PRE_WLAN_FEATURE_DFR

#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_hipriv_dfr_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;
    oal_uint32                   ul_ret;
    oal_uint32                   ul_off_set;
    oal_int8                     ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                    uc_enable_flag;
    oal_uint8                   *puc_value;

    /* hipriv "Hisilicon0 dfr_enable 1" */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pause_tid::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_enable_flag = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGIG_DFR_ENABLE, OAL_SIZEOF(oal_uint8));

    puc_value = (oal_uint8 *)(st_write_msg.auc_value);
    *puc_value = uc_enable_flag;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dfr_enable::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_trig_pcie_reset(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TRIG_PCIE_RESET, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_trig_pcie_reset::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_trig_loss_tx_comp(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;
    oal_uint32                   ul_ret;
    oal_uint32                   ul_off_set;
    oal_int8                     ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                   ul_loss_cnt;
    oal_uint32                  *pul_value;

    /* hipriv "Hisilicon0 loss_tx_comp 1" */

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_pause_tid::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    ul_loss_cnt = (oal_uint32)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_TRIG_LOSS_TX_COMP, OAL_SIZEOF(oal_int32));

    pul_value = (oal_uint32 *)(st_write_msg.auc_value);
    *pul_value = ul_loss_cnt;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_trig_loss_tx_comp::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}
#endif
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_WEB_CFG_FIXED_RATE

OAL_STATIC oal_uint32  wal_hipriv_set_mode_ucast_data_dscr_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_set_dscr_param_stru     *pst_set_dscr_param;
    wal_dscr_param_enum_uint8        en_param_index;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_DSCR, OAL_SIZEOF(mac_cfg_set_dscr_param_stru));

    /* 解析并设置配置命令参数 */
    pst_set_dscr_param = (mac_cfg_set_dscr_param_stru *)(st_write_msg.auc_value);

    /* 获取描述符字段设置命令字符串 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mode_ucast_data_dscr_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 解析配置的协议模式 */
    if (!oal_strcmp("11ac", ac_arg))
    {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_VHT_UCAST_DATA;
    }
    else if (!oal_strcmp("11n", ac_arg))
    {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_HT_UCAST_DATA;
    }
    else if (!oal_strcmp("11ag", ac_arg))
    {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_11AG_UCAST_DATA;
    }
    else if (!oal_strcmp("11b", ac_arg))
    {
        pst_set_dscr_param->en_type = MAC_VAP_CONFIG_11B_UCAST_DATA;
    }
    else
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mode_ucast_data_dscr_param:: no such param for protocol_mode!}\r\n");
        return OAL_FAIL;
    }

    /* 获取描述符字段设置命令字符串 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mode_ucast_data_dscr_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    /* 解析是设置哪一个字段 */
    for (en_param_index = 0; en_param_index < WAL_DSCR_PARAM_BUTT; en_param_index++)
    {
        if(!oal_strcmp(pauc_tx_dscr_param_name_etc[en_param_index], ac_arg))
        {
            break;
        }
    }

    /* 检查命令是否打错 */
    if (WAL_DSCR_PARAM_BUTT == en_param_index)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_mode_ucast_data_dscr_param::no such param for tx dscr!}\r\n");
        return OAL_FAIL;
    }

    pst_set_dscr_param->uc_function_index = en_param_index;

    /* 解析要设置为多大的值 */
    pst_set_dscr_param->l_value = oal_strtol(pc_param, OAL_PTR_NULL, 0);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_dscr_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mode_ucast_data_dscr_param::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}
#endif

#ifdef _PRE_DEBUG_MODE_USER_TRACK

OAL_STATIC oal_uint32  wal_hipriv_report_thrput_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_usr_thrput_stru         st_usr_thrput;

    /* sh hipriv.sh "vap_name thrput_stat  XX:XX:XX:XX:XX;XX 0|1" */

    /* 获取用户mac地址 */
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, st_usr_thrput.auc_user_macaddr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_thrput_stat::wal_hipriv_get_mac_addr_etc return [%d].}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_thrput_stat::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    st_usr_thrput.uc_param = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_USR_THRPUT_STAT, OAL_SIZEOF(st_usr_thrput));

    /* 填写消息体，参数 */
    oal_memcopy(st_write_msg.auc_value, &st_usr_thrput, OAL_SIZEOF(st_usr_thrput));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_usr_thrput),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_thrput_stat::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif

#ifdef _PRE_DEBUG_MODE

oal_uint32 wal_hipriv_set_rxch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_int8                         c_ch_idx = 0;
    oal_uint8                        uc_rxch = 0;
    oal_uint32                       ul_off_set = 0;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    wal_msg_write_stru               st_write_msg;
    wal_msg_stru                    *pst_rsp_msg = OAL_PTR_NULL;

    /* 获取接收通道设置 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rfch::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 解析要设置为多大的值 */
    uc_rxch = 0;
    for (c_ch_idx = 0; c_ch_idx < WAL_HIPRIV_CH_NUM; c_ch_idx++)
    {
        if ('0' == ac_arg[c_ch_idx])
        {
            continue;
        }
        else if ('1' == ac_arg[c_ch_idx])
        {
            uc_rxch += (oal_uint8)(1 << (WAL_HIPRIV_CH_NUM - c_ch_idx - 1));
        }
        /* 输入数据有非01数字，或数字少于4位，异常 */
        else
        {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_rfch::input err!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* 输入参数多于四位，异常 */
    if ('\0' != ac_arg[c_ch_idx])
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_rfch::input err!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    *(oal_uint8 *)(st_write_msg.auc_value) = uc_rxch;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RXCH, OAL_SIZEOF(oal_uint8));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rfch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

        /* 读取返回的错误码 */
    ul_ret = wal_check_and_release_msg_resp_etc(pst_rsp_msg);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_DFR, "{wal_hipriv_set_rfch fail, err code[%u]!}\r\n", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_dync_txpower(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                        uc_dync_power_flag;
    oal_int32                        l_idx = 0;

    /* 获取动态功率校准开关标志 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dync_txpower::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 输入命令合法性检测 */
    while ('\0' != ac_arg[l_idx])
    {
        if (isdigit(ac_arg[l_idx]))
        {
            l_idx++;
            continue;
        }
        else
        {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_dync_txpower::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* 将命令参数值字符串转化为整数 */
    uc_dync_power_flag = (oal_uint8)oal_atoi(ac_arg);

    *(oal_uint8 *)(st_write_msg.auc_value) = uc_dync_power_flag;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DYNC_TXPOWER, OAL_SIZEOF(oal_uint8));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dync_txpower::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_dync_pow_debug_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_debug_switch = 0;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_dync_pow_debug_switch::wal_get_cmd_one_arg_etc fails!}\r\n");
        return ul_ret;
    }

    pc_param += ul_off_set;
    uc_debug_switch = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_debug_switch;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DYNC_POW_DEBUG, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    wal_hipriv_wait_rsp(pst_net_dev, pc_param);
#endif

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_dync_pow_debug_switch::wal_hipriv_reset_device return err code = [%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_report_ampdu_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_ampdu_stat_stru         st_ampdu_param;

    /* sh hipriv.sh "vap_name ampdu_stat XX:XX:XX:XX:XX:XX tid_no 0|1" */

    /* 获取用户mac地址 */
    OAL_MEMZERO(&st_ampdu_param, OAL_SIZEOF(mac_cfg_ampdu_stat_stru));
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, st_ampdu_param.auc_user_macaddr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_ampdu_stat::wal_hipriv_get_mac_addr_etc return [%d].}", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_ampdu_stat::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param += ul_off_set;

    st_ampdu_param.uc_tid_no = (oal_uint8)oal_atoi(ac_name);
    if (st_ampdu_param.uc_tid_no > WLAN_TID_MAX_NUM)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_ampdu_stat::input tid_no invalid. tid_no = [%d]!}\r\n", st_ampdu_param.uc_tid_no);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_ampdu_stat::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    st_ampdu_param.uc_param = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REPORT_AMPDU_STAT, OAL_SIZEOF(st_ampdu_param));

    /* 填写消息体，参数 */
    oal_memcopy(st_write_msg.auc_value, &st_ampdu_param, OAL_SIZEOF(st_ampdu_param));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_ampdu_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_report_ampdu_stat::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_get_tx_comp_cnt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32      ul_ret          = OAL_SUCC;
    oal_uint32      ul_off_set      = 0;
    oal_uint8       uc_stat_flag    = 0;
    oal_int8        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    mac_vap_stru    *pst_mac_vap;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if(NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_get_tx_comp_cnt::pst_mac_vap is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 统计发送完成中断是否丢失(关闭聚合) sh hipriv.sh "Hisilicon0 tx_comp_cnt 0|1",
       0表示清零统计次数， 1表示显示统计次数并且清零",
    */

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_get_tx_comp_cnt::wal_get_cmd_one_arg_etc fails!}\r\n");
        return ul_ret;
    }
    uc_stat_flag = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    if( 0 == uc_stat_flag)
    {
        g_ast_tx_complete_stat_etc[pst_mac_vap->uc_device_id].ul_tx_data_num            = 0;
        g_ast_tx_complete_stat_etc[pst_mac_vap->uc_device_id].ul_tx_mgnt_num            = 0;
        g_ast_tx_complete_stat_etc[pst_mac_vap->uc_device_id].ul_tx_complete_total_num  = 0;
        g_ast_tx_complete_stat_etc[pst_mac_vap->uc_device_id].ul_tx_complete_uh1_num    = 0;
        g_ast_tx_complete_stat_etc[pst_mac_vap->uc_device_id].ul_tx_complete_uh2_num    = 0;
        g_ast_tx_complete_stat_etc[pst_mac_vap->uc_device_id].ul_tx_complete_bh1_num    = 0;
        g_ast_tx_complete_stat_etc[pst_mac_vap->uc_device_id].ul_tx_complete_bh2_num    = 0;
        g_ast_tx_complete_stat_etc[pst_mac_vap->uc_device_id].ul_tx_complete_bh3_num    = 0;

    }
    else
    {
        OAL_IO_PRINT("ul_tx_data_num = %d\n", g_ast_tx_complete_stat_etc[pst_mac_vap->uc_device_id].ul_tx_data_num);
        OAL_IO_PRINT("ul_tx_mgnt_num = %d\n", g_ast_tx_complete_stat_etc[pst_mac_vap->uc_device_id].ul_tx_mgnt_num);
        OAL_IO_PRINT("ul_tx_complete_total_num = %d\n", g_ast_tx_complete_stat_etc[pst_mac_vap->uc_device_id].ul_tx_complete_total_num);
        OAL_IO_PRINT("ul_tx_complete_uh1_num = %d\n", g_ast_tx_complete_stat_etc[pst_mac_vap->uc_device_id].ul_tx_complete_uh1_num);
        OAL_IO_PRINT("ul_tx_complete_uh2_num = %d\n", g_ast_tx_complete_stat_etc[pst_mac_vap->uc_device_id].ul_tx_complete_uh2_num);
        OAL_IO_PRINT("ul_tx_complete_bh1_num = %d\n", g_ast_tx_complete_stat_etc[pst_mac_vap->uc_device_id].ul_tx_complete_bh1_num);
        OAL_IO_PRINT("ul_tx_complete_bh2_num = %d\n", g_ast_tx_complete_stat_etc[pst_mac_vap->uc_device_id].ul_tx_complete_bh2_num);
        OAL_IO_PRINT("ul_tx_complete_bh3_num = %d\n", g_ast_tx_complete_stat_etc[pst_mac_vap->uc_device_id].ul_tx_complete_bh3_num);
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_set_debug_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                          ul_off_set = 0;
    oal_uint8                           uc_debug_type = 0;
    oal_int8                            ac_debug_type[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                          ul_ret;
    oal_uint8                           uc_idx;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_debug_type, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAL_IO_PRINT("Error:wal_hipriv_set_debug_switch[%d] wal_get_cmd_one_arg_etc return error code[%d]! \n", __LINE__, ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;

    if (ac_debug_type[0] == '?')
    {
        OAL_IO_PRINT("debug_switch <debug_type> <switch_state> \n");
        OAL_IO_PRINT("               0      0/1     -- when set register echo read value \n");
        return OAL_SUCC;
    }

    uc_debug_type = (oal_uint8)oal_atoi((const oal_int8 *)ac_debug_type);
    if (uc_debug_type >= MAX_DEBUG_TYPE_NUM)
    {
        OAL_IO_PRINT("Info: <debug_type> should be less than %d. \n", MAX_DEBUG_TYPE_NUM);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_debug_type, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAL_IO_PRINT("Error:wal_hipriv_set_debug_switch[%d] wal_get_cmd_one_arg_etc return error code[%d]! \n", __LINE__, ul_ret);
        return ul_ret;
    }

    g_aul_debug_feature_switch_etc[uc_debug_type] = (oal_uint32)oal_atoi((const oal_int8 *)ac_debug_type);
    if ((g_aul_debug_feature_switch_etc[uc_debug_type] != OAL_SWITCH_ON) && (g_aul_debug_feature_switch_etc[uc_debug_type] != OAL_SWITCH_OFF))
    {
        OAL_IO_PRINT("Error:switch_value must be 0 or 1. \n");
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    OAL_IO_PRINT("<debug_type>   <switch_value> \n");
    for (uc_idx = 0; uc_idx < MAX_DEBUG_TYPE_NUM; uc_idx++)
    {
        OAL_IO_PRINT("  %d          %d \n", uc_idx, g_aul_debug_feature_switch_etc[uc_idx]);
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_rx_filter_val(oal_int8                **pc_param,
                                                hmac_cfg_rx_filter_stru *pst_rx_filter_val)
{
    oal_uint32                          ul_off_set = 0;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                          ul_ret;

    ul_ret = wal_get_cmd_one_arg_etc(*pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_filter_val::wal_get_cmd_one_arg_etc return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    *pc_param += ul_off_set;

    pst_rx_filter_val->uc_dev_mode = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);
    if (pst_rx_filter_val->uc_dev_mode > 1)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_filter_val::st_rx_filter_val.uc_dev_mode is exceed.[%d]}\r\n", pst_rx_filter_val->uc_dev_mode);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    ul_ret = wal_get_cmd_one_arg_etc(*pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_rx_filter_val::wal_get_cmd_one_arg_etc return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    *pc_param += ul_off_set;

    pst_rx_filter_val->uc_vap_mode = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    if (pst_rx_filter_val->uc_vap_mode >= WLAN_VAP_MODE_BUTT)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_hipriv_rx_filter_val::uc_dev_mode is exceed! uc_dev_mode = [%d].}\r\n", pst_rx_filter_val->uc_vap_mode);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    ul_ret = wal_get_cmd_one_arg_etc(*pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_filter_val::wal_get_cmd_one_arg_etc return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }
    *pc_param += ul_off_set;
    pst_rx_filter_val->uc_vap_status = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    if (pst_rx_filter_val->uc_vap_status >= MAC_VAP_STATE_BUTT)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{wal_hipriv_rx_filter_val::uc_dev_mode is exceed! uc_dev_mode = [%d].}\r\n", pst_rx_filter_val->uc_vap_status);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_rx_filter_val(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                          ul_ret;
    oal_int32                           l_ret;
    hmac_cfg_rx_filter_stru             st_rx_filter_val = {0};
    wal_msg_write_stru                  st_write_msg;
    oal_int8             *pc_token;
    oal_int8             *pc_end;
    oal_int8             *pc_ctx;
    oal_int8             *pc_sep = " ";

    ul_ret = wal_hipriv_rx_filter_val(&pc_param, &st_rx_filter_val);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_filter_val::wal_hipriv_rx_filter_val return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    /* 0--写某一VAP状态的帧过滤值 */
    st_rx_filter_val.uc_write_read = 0;

    /* 获取需要写入的值 */
    pc_token = oal_strtok(pc_param, pc_sep, &pc_ctx);
    if (NULL == pc_token)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_rx_filter_val::pc_token is null}\r\n");
        return OAL_FAIL;
    }

    st_rx_filter_val.ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 16);


    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RX_FILTER_VAL, OAL_SIZEOF(hmac_cfg_rx_filter_stru));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value, &st_rx_filter_val, OAL_SIZEOF(hmac_cfg_rx_filter_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(hmac_cfg_rx_filter_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_filter_val::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_get_rx_filter_val(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                          ul_ret;
    oal_int32                           l_ret;
    hmac_cfg_rx_filter_stru             st_rx_filter_val = {0};
    wal_msg_write_stru                  st_write_msg;

    ul_ret = wal_hipriv_rx_filter_val(&pc_param, &st_rx_filter_val);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_filter_val::wal_hipriv_rx_filter_val return err_code[%d]}\r\n", ul_ret);
        return ul_ret;
    }

    /* 1--读某一VAP状态的帧过滤值 */
    st_rx_filter_val.uc_write_read = 1;
    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RX_FILTER_VAL, OAL_SIZEOF(hmac_cfg_rx_filter_stru));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value, &st_rx_filter_val, OAL_SIZEOF(hmac_cfg_rx_filter_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(hmac_cfg_rx_filter_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_filter_val::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_rx_filter_en(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_rx_filter_en = 0;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_vap_nss::wal_get_cmd_one_arg_etc fails!}\r\n");
        return ul_ret;
    }

    pc_param += ul_off_set;
    uc_rx_filter_en = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);

    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_rx_filter_en;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RX_FILTER_EN, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_rx_filter_en::wal_hipriv_reset_device return err code = [%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_get_rx_filter_en(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_RX_FILTER_EN, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_version::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_scan_test(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[15];
    oal_int8                        ac_scan_type[15];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint8                       uc_bandwidth;
    wal_msg_write_stru              st_write_msg;
    mac_ioctl_scan_test_config_stru *pst_scan_test;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_scan_type, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "wal_hipriv_scan_test: get first arg fail.");
        return OAL_FAIL;
    }

    pc_param = pc_param + ul_off_set;
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "wal_hipriv_scan_test: get second arg fail.");
        return OAL_FAIL;
    }
    uc_bandwidth = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                            抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SCAN_TEST, OAL_SIZEOF(mac_ioctl_scan_test_config_stru));

    /* 设置配置命令参数 */
    pst_scan_test = (mac_ioctl_scan_test_config_stru *)(st_write_msg.auc_value);
    oal_memcopy(pst_scan_test->ac_scan_type, ac_scan_type, sizeof(ac_scan_type));
    pst_scan_test->en_bandwidth = uc_bandwidth;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                      WAL_MSG_TYPE_WRITE,
                      WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_ioctl_scan_test_config_stru),
                      (oal_uint8 *)&st_write_msg,
                      OAL_FALSE,
                      OAL_PTR_NULL);
    if (OAL_SUCC != l_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_send_cfg_event_etc fail.return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}
#endif

#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_hipriv_freq_adjust(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_int8                       *pc_token;
    oal_int8                       *pc_end;
    oal_int8                       *pc_ctx;
    oal_int8                       *pc_sep = " ";
    mac_cfg_freq_adjust_stru        st_freq_adjust_ctl;

    /* 获取整数分频 */
    pc_token = oal_strtok(pc_param, pc_sep, &pc_ctx);
    if (NULL == pc_token)
    {
        return OAL_FAIL;
    }

    st_freq_adjust_ctl.us_pll_int = (oal_uint16)oal_strtol(pc_token, &pc_end, 16);

    /* 获取小数分频 */
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (NULL == pc_token)
    {
        return OAL_FAIL;
    }

    st_freq_adjust_ctl.us_pll_frac = (oal_uint16)oal_strtol(pc_token, &pc_end, 16);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FREQ_ADJUST, OAL_SIZEOF(st_freq_adjust_ctl));

    /* 填写消息体，参数 */
    oal_memcopy(st_write_msg.auc_value, &st_freq_adjust_ctl, OAL_SIZEOF(st_freq_adjust_ctl));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_freq_adjust_ctl),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_freq_adjust::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_SUPPORT_ACS

OAL_STATIC oal_uint32  wal_hipriv_acs(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru    st_write_msg;
    oal_uint16            us_len;
    oal_int32             l_ret;

    if (OAL_UNLIKELY(WAL_MSG_WRITE_MAX_LEN <= OAL_STRLEN(pc_param)))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_acs:: pc_param overlength is %d}\n", OAL_STRLEN(pc_param));
        oal_print_hex_dump((oal_uint8 *)pc_param, WAL_MSG_WRITE_MAX_LEN, 32, "wal_hipriv_acs: param is overlong:");
        return OAL_FAIL;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ACS_CONFIG, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_acs::return err code [%d]!}\r\n", l_ret);

        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_hipriv_chan_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru    st_write_msg;
    oal_int32             l_ret;
    oal_int8              *pc_token;
    oal_int8              *pc_end;
    oal_int8              *pc_ctx;
    oal_int8              *pc_sep = " ";
    oal_uint8              uc_idx;
    oal_int32              l_scan_mode;
    oal_uint8              uc_chan_num = 0;

    mac_chan_stat_param_stru        st_param;

    pc_token = oal_strtok(pc_param, pc_sep, &pc_ctx);
    if (NULL == pc_token)
    {
        return OAL_FAIL;
    }

    st_param.us_duration_ms = (oal_uint16)oal_strtol(pc_token, &pc_end, 10);

    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (NULL == pc_token)
    {
        return OAL_FAIL;
    }

    st_param.en_probe = oal_strtol(pc_token, &pc_end, 10) ? OAL_TRUE: OAL_FALSE;
    st_param.uc_chan_cnt = 0;


    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (NULL == pc_token)
    {
        return OAL_FAIL;
    }

    l_scan_mode = oal_strtol(pc_token, &pc_end, 10);;

    if (l_scan_mode == 0 || l_scan_mode == 2)
    {
        for (uc_idx = 0; uc_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_idx++)
        {
            if(OAL_SUCC == mac_is_channel_idx_valid_etc(WLAN_BAND_2G, uc_idx))
            {
                mac_get_channel_num_from_idx_etc(WLAN_BAND_2G, uc_idx, &uc_chan_num);
                st_param.auc_channels[st_param.uc_chan_cnt++] = uc_chan_num;
            }
        }
    }

    if (l_scan_mode == 1 || l_scan_mode == 2)
    {
        for (uc_idx = 0; uc_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_idx++)
        {
            if(OAL_SUCC == mac_is_channel_idx_valid_etc(WLAN_BAND_5G, uc_idx))
            {
                mac_get_channel_num_from_idx_etc(WLAN_BAND_5G, uc_idx, &uc_chan_num);
                st_param.auc_channels[st_param.uc_chan_cnt++] = uc_chan_num;
            }
        }
    }
    else
    {
        for (uc_idx = 0; uc_idx < WLAN_MAX_CHANNEL_NUM; uc_idx++)
        {
            pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
            if (NULL == pc_token)
            {
                break;
            }
            st_param.auc_channels[st_param.uc_chan_cnt++]= (oal_uint8)oal_strtol(pc_token, &pc_end, 10);
        }
    }

    if (st_param.uc_chan_cnt == 0)
    {
        return OAL_FAIL;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, &st_param, OAL_SIZEOF(st_param));

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CHAN_STAT, OAL_SIZEOF(st_param));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_param),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_chan_stat::return err code [%d]!}\r\n", l_ret);

        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


#ifdef _PRE_WLAN_FEATURE_BAND_STEERING

OAL_STATIC oal_uint32  wal_hipriv_bsd(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru    st_write_msg;
    oal_uint16            us_len;
    oal_int32             l_ret;

    if (OAL_UNLIKELY(WAL_MSG_WRITE_MAX_LEN <= OAL_STRLEN(pc_param)))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_bsd:: pc_param overlength is %d}\n", OAL_STRLEN(pc_param));
        oal_print_hex_dump((oal_uint8 *)pc_param, WAL_MSG_WRITE_MAX_LEN, 32, "wal_hipriv_bsd: param is overlong:");
        return OAL_FAIL;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_BSD_CONFIG, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_bsd::return err code [%d]!}\r\n", l_ret);

        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


#ifdef _PRE_WLAN_FEATURE_11V

OAL_STATIC oal_uint32  wal_hipriv_11v_cfg_wl_mgmt(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_uint8                   uc_debug_switch;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_BSSTRANSITION, "{wal_hipriv_11v_cfg_wl_mgmt::wal_get_cmd_one_arg_etc return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }
    /* 参数0为关闭能力位 1位打开能力位，其他参数不支持 */
    uc_debug_switch = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);
    switch(uc_debug_switch)
    {
    case 0:
    case 1:
        break;
    default:
        OAM_WARNING_LOG0(0, OAM_SF_BSSTRANSITION, "{wal_hipriv_11v_cfg_wl_mgmt::command param is error!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_11V_WL_MGMT_SWITCH, OAL_SIZEOF(oal_uint8));
    st_write_msg.auc_value[0] = uc_debug_switch;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_BSSTRANSITION, "{wal_hipriv_11v_cfg_wl_mgmt::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_hipriv_11v_tx_request(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru    st_write_msg;
	oal_int32			  l_ret	= 0;
    oal_uint32            ul_ret	= 0;
    oal_uint32            ul_off_set;
    oal_int8              ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8             auc_mac_addr[WLAN_MAC_ADDR_LEN];

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_BSSTRANSITION, "{wal_hipriv_11v_tx_request::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, auc_mac_addr);
    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_11V_TX_REQUEST, OAL_SIZEOF(auc_mac_addr));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,auc_mac_addr,WLAN_MAC_ADDR_LEN);
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(auc_mac_addr),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_BSSTRANSITION, "{wal_hipriv_11v_tx_request::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
    return OAL_SUCC;
}
#endif  // _PRE_DEBUG_MODE
#endif  //_PRE_WLAN_FEATURE_11V

#if (defined(_PRE_WLAN_FEATURE_11V) && defined(_PRE_DEBUG_MODE)) || defined(_PRE_WLAN_FEATURE_11V_ENABLE)

OAL_STATIC oal_uint32  wal_hipriv_11v_cfg_bsst(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_uint8                   uc_debug_switch;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_11v_cfg_bsst::wal_get_cmd_one_arg_etc return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }
    /* 参数0为关闭能力位 1位打开能力位，其他参数不支持 */
    uc_debug_switch = (oal_uint8)oal_atoi((const oal_int8 *)ac_name);
    switch(uc_debug_switch)
    {
    case 0:
    case 1:
        break;
    default:
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_hipriv_11v_cfg_wl_mgmt::command param is error!}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_11V_BSST_SWITCH, OAL_SIZEOF(oal_uint8));
    st_write_msg.auc_value[0] = uc_debug_switch;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_11v_cfg_bsst::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_11v_tx_query(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru    st_write_msg;
    oal_int32             l_ret	= 0;
	oal_uint32			  ul_ret	= 0;
    oal_uint32            ul_off_set;
    oal_int8              ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8             auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_11v_tx_query::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, auc_mac_addr);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_11V_TX_QUERY, OAL_SIZEOF(auc_mac_addr));

    /* 设置配置命令参数 */
    oal_memcopy(st_write_msg.auc_value,auc_mac_addr,WLAN_MAC_ADDR_LEN);
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(auc_mac_addr),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);
    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_11v_tx_query::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  wal_hipriv_set_priv_flag(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                           ul_off_set;
    oal_int8                             ac_flag[2];
    oal_uint32                           ul_ret;
    oal_int32                            l_ret;
    wal_msg_write_stru                   st_write_msg;
    oal_bool_enum_uint8                 *pen_priv_flag;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_flag, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_priv_flag: get first arg fail.");
        return OAL_FAIL;
    }

    /***************************************************************************
                            抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_PRIV_FLAG, OAL_SIZEOF(oal_bool_enum_uint8));

    /* 设置配置命令参数 */
    pen_priv_flag = (oal_bool_enum_uint8 *)(st_write_msg.auc_value);
   *pen_priv_flag = (oal_bool_enum_uint8)oal_atoi(ac_flag);

    if((*pen_priv_flag != 0)&&(*pen_priv_flag != 1))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_priv_flag: Inavlid flag,should be 0 or 1!");
        return OAL_FAIL;
    }

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_hipriv_set_priv_flag:: priv_flag= %d.", *pen_priv_flag);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);
    if (OAL_SUCC != l_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_priv_flag::wal_send_cfg_event_etc fail.return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_bw_fixed(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                           ul_off_set;
    oal_int8                             ac_flag[2];
    oal_uint32                           ul_ret;
    oal_int32                            l_ret;
    mac_vap_stru                        *pst_mac_vap;
    wal_msg_write_stru                   st_write_msg;
    oal_bool_enum_uint8                 *puc_bw_fixed_flag;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_net_dev) || (OAL_PTR_NULL == pc_param)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_bw_fixed::pst_net_dev/p_param null ptr error!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if(OAL_UNLIKELY(NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{wal_hipriv_set_bw_fixed::can't get mac vap from netdevice priv data!}");
        return OAL_FAIL;
    }

    /* 设备在up状态不允许配置，必须先down */
    if (pst_mac_vap->en_vap_state != MAC_VAP_STATE_INIT)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_set_bw_fixed::device is busy, please down it first %d!}\r\n", pst_mac_vap->en_vap_state);
        return OAL_FAIL;
    }

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_flag, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_hipriv_set_bw_fixed: get first arg fail.");
        return OAL_FAIL;
    }

    /***************************************************************************
                            抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_BW_FIXED, OAL_SIZEOF(oal_uint8));

    /* 设置配置命令参数 */
    puc_bw_fixed_flag = (oal_uint8 *)(st_write_msg.auc_value);
   *puc_bw_fixed_flag = (oal_uint8)oal_atoi(ac_flag);

    *puc_bw_fixed_flag = !!(*puc_bw_fixed_flag);

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_hipriv_set_bw_fixed:: bw_fixed = %d.", *puc_bw_fixed_flag);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                       WAL_MSG_TYPE_WRITE,
                       WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                       (oal_uint8 *)&st_write_msg,
                       OAL_FALSE,
                       OAL_PTR_NULL);
    if (OAL_SUCC != l_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_bw_fixed::wal_send_cfg_event_etc fail.return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



#ifdef _PRE_WLAN_PERFORM_STAT

OAL_STATIC oal_uint32  wal_hipriv_stat_tid_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_total_offset = 0;
    mac_cfg_stat_param_stru        *pst_stat_param;

    /* vap0 stat_tid_thrpt xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数) */
    if (OAL_PTR_NULL == OAL_NET_DEV_PRIV(pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_thrpt::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_STAT, OAL_SIZEOF(mac_cfg_stat_param_stru));
    pst_stat_param = (mac_cfg_stat_param_stru *)(st_write_msg.auc_value);

    pst_stat_param->en_stat_type    = MAC_STAT_TYPE_TID_THRPT;
    pst_stat_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_thrpt::wal_hipriv_get_mac_addr_etc failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_stat_param->auc_mac_addr, auc_mac_addr);

    /* 获取tidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_thrpt::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    pst_stat_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /* 获取统计周期 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_thrpt::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_period = (oal_uint16)oal_atoi(ac_name);

    /* 获取统计次数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_thrpt::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pst_stat_param->us_stat_num = (oal_uint16)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_stat_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_thrpt::wal_hipriv_stat_tid_thrpt return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_stat_user_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_total_offset = 0;
    mac_cfg_stat_param_stru        *pst_stat_param;

    /* vap0 stat_user_thrpt xx xx xx xx xx xx(mac地址) stat_period(统计周期ms) stat_num(统计次数) */
    if (OAL_PTR_NULL == OAL_NET_DEV_PRIV(pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_user_thrpt::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }


    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_STAT, OAL_SIZEOF(mac_cfg_stat_param_stru));
    pst_stat_param = (mac_cfg_stat_param_stru *)(st_write_msg.auc_value);

    pst_stat_param->en_stat_type    = MAC_STAT_TYPE_USER_THRPT;
    pst_stat_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_user_thrpt::wal_hipriv_get_mac_addr_etc failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_stat_param->auc_mac_addr, auc_mac_addr);

    /* 获取统计周期 */
    pc_param = pc_param + ul_total_offset;
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_user_thrpt::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_period = (oal_uint16)oal_atoi(ac_name);

    /* 获取统计次数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_user_thrpt::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_num = (oal_uint16)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_stat_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_user_thrpt::wal_hipriv_stat_tid_thrpt return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_stat_user_bsd(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_total_offset = 0;
    mac_cfg_stat_param_stru        *pst_stat_param;

    /* vap0 stat_user_thrpt xx xx xx xx xx xx(mac地址) stat_period(统计周期ms) stat_num(统计次数) */

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_STAT, OAL_SIZEOF(mac_cfg_stat_param_stru));
    pst_stat_param = (mac_cfg_stat_param_stru *)(st_write_msg.auc_value);

    pst_stat_param->en_stat_type    = MAC_STAT_TYPE_USER_BSD;
    pst_stat_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_user_bsd::wal_hipriv_get_mac_addr_etc failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_stat_param->auc_mac_addr, auc_mac_addr);

    /* 获取统计周期 */
    pc_param = pc_param + ul_total_offset;
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_user_bsd::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_period = (oal_uint16)oal_atoi(ac_name);

    /* 获取统计次数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_user_bsd::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_num = (oal_uint16)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_stat_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_user_thrpt::wal_hipriv_stat_tid_thrpt return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_stat_vap_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_stat_param_stru        *pst_stat_param;

    /* vap0 stat_vap_thrpt stat_period(统计周期ms) stat_num(统计次数) */
    if (OAL_PTR_NULL == OAL_NET_DEV_PRIV(pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_vap_thrpt::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_STAT, OAL_SIZEOF(mac_cfg_stat_param_stru));
    pst_stat_param = (mac_cfg_stat_param_stru *)(st_write_msg.auc_value);

    pst_stat_param->en_stat_type    = MAC_STAT_TYPE_VAP_THRPT;
    pst_stat_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取统计周期 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_vap_thrpt::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_period = (oal_uint16)oal_atoi(ac_name);

    /* 获取统计次数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_vap_thrpt::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_num = (oal_uint16)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_stat_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_vap_thrpt::wal_hipriv_stat_tid_thrpt return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_stat_tid_per(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_stat_param_stru        *pst_stat_param;
    oal_uint32                      ul_total_offset = 0;

    /* vap0 stat_tid_per xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数) */
    if (OAL_PTR_NULL == OAL_NET_DEV_PRIV(pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_per::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_STAT, OAL_SIZEOF(mac_cfg_stat_param_stru));
    pst_stat_param = (mac_cfg_stat_param_stru *)(st_write_msg.auc_value);

    pst_stat_param->en_stat_type    = MAC_STAT_TYPE_TID_PER;
    pst_stat_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_per::wal_hipriv_get_mac_addr_etc failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_stat_param->auc_mac_addr, auc_mac_addr);

    /* 获取tidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_per::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pst_stat_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /* 获取统计周期 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_per::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_period = (oal_uint16)oal_atoi(ac_name);

    /* 获取统计次数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_per::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_num = (oal_uint16)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_stat_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_per::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_stat_tid_delay(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_stat_param_stru        *pst_stat_param;
    oal_uint32                      ul_total_offset = 0;

    /* vap0 stat_tid_delay xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数) */
    if (OAL_PTR_NULL == OAL_NET_DEV_PRIV(pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_STAT, OAL_SIZEOF(mac_cfg_stat_param_stru));
    pst_stat_param = (mac_cfg_stat_param_stru *)(st_write_msg.auc_value);

    pst_stat_param->en_stat_type    = MAC_STAT_TYPE_TID_DELAY;
    pst_stat_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::wal_hipriv_get_mac_addr_etc failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_stat_param->auc_mac_addr, auc_mac_addr);

    /* 获取tidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pst_stat_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /* 获取统计周期 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_period = (oal_uint16)oal_atoi(ac_name);

    /* 获取统计次数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_stat_param->us_stat_num = (oal_uint16)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_stat_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_stat_tid_delay::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_display_tid_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_display_param_stru     *pst_display_param;
    oal_uint32                      ul_total_offset = 0;

    /* vap0 stat_tid_thrpt xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数) */
    if (OAL_PTR_NULL == OAL_NET_DEV_PRIV(pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_tid_thrpt::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_DISPLAY, OAL_SIZEOF(mac_cfg_display_param_stru));
    pst_display_param = (mac_cfg_display_param_stru *)(st_write_msg.auc_value);

    pst_display_param->en_stat_type    = MAC_STAT_TYPE_TID_THRPT;
    pst_display_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_tid_thrpt::wal_hipriv_get_mac_addr_etc failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_display_param->auc_mac_addr, auc_mac_addr);

    /* 获取tidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_thrpt::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pst_display_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_display_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_thrpt::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_display_user_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_display_param_stru     *pst_display_param;
    oal_uint32                      ul_total_offset = 0;

    /* vap0 stat_user_thrpt xx xx xx xx xx xx(mac地址) stat_period(统计周期ms) stat_num(统计次数) */
    if (OAL_PTR_NULL == OAL_NET_DEV_PRIV(pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_user_thrpt::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_DISPLAY, OAL_SIZEOF(mac_cfg_display_param_stru));
    pst_display_param = (mac_cfg_display_param_stru *)(st_write_msg.auc_value);

    pst_display_param->en_stat_type    = MAC_STAT_TYPE_USER_THRPT;
    pst_display_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_user_thrpt::wal_hipriv_get_mac_addr_etc failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_display_param->auc_mac_addr, auc_mac_addr);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_display_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_user_thrpt::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_display_vap_thrpt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_display_param_stru     *pst_display_param;

    /* vap0 stat_vap_thrpt stat_period(统计周期ms) stat_num(统计次数) */
    if (OAL_PTR_NULL == OAL_NET_DEV_PRIV(pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_vap_thrpt::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_DISPLAY, OAL_SIZEOF(mac_cfg_display_param_stru));
    pst_display_param = (mac_cfg_display_param_stru *)(st_write_msg.auc_value);

    pst_display_param->en_stat_type    = MAC_STAT_TYPE_VAP_THRPT;
    pst_display_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_display_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_vap_thrpt::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_display_tid_per(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_display_param_stru     *pst_display_param;
    oal_uint32                      ul_total_offset = 0;

    /* vap0 stat_tid_per xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数) */
    if (OAL_PTR_NULL == OAL_NET_DEV_PRIV(pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_tid_per::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_DISPLAY, OAL_SIZEOF(mac_cfg_display_param_stru));
    pst_display_param = (mac_cfg_display_param_stru *)(st_write_msg.auc_value);

    pst_display_param->en_stat_type    = MAC_STAT_TYPE_TID_PER;
    pst_display_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_tid_per::wal_hipriv_get_mac_addr_etc failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_display_param->auc_mac_addr, auc_mac_addr);

    /* 获取tidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_per::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pst_display_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_display_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_per::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_display_tid_delay(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_display_param_stru     *pst_display_param;
    oal_uint32                      ul_total_offset = 0;

    /* vap0 stat_tid_delay xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数) */
    if (OAL_PTR_NULL == OAL_NET_DEV_PRIV(pst_net_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_tid_delay::OAL_NET_DEV_PRIV(pst_net_dev) is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请事件内存 */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PFM_DISPLAY, OAL_SIZEOF(mac_cfg_display_param_stru));
    pst_display_param = (mac_cfg_display_param_stru *)(st_write_msg.auc_value);

    pst_display_param->en_stat_type    = MAC_STAT_TYPE_TID_DELAY;
    pst_display_param->uc_vap_id       = ((mac_vap_stru *)OAL_NET_DEV_PRIV(pst_net_dev))->uc_vap_id;

    /* 获取mac地址 */
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, auc_mac_addr, &ul_total_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_display_tid_delay::wal_hipriv_get_mac_addr_etc failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_display_param->auc_mac_addr, auc_mac_addr);

    /* 获取tidno */
    pc_param = pc_param + ul_total_offset;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_delay::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pst_display_param->uc_tidno = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_display_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_display_tid_delay::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_DAQ

OAL_STATIC oal_uint32  wal_hipriv_data_acq(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;
    if (OAL_UNLIKELY(WAL_MSG_WRITE_MAX_LEN <= OAL_STRLEN(pc_param)))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_data_acq:: pc_param overlength is %d}\n", OAL_STRLEN(pc_param));
        return OAL_FAIL;
    }

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    while (' ' == *pc_param)
    {
        ++pc_param;
    }
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DATA_ACQ, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
       OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_data_acq::return err code[%d]!}\r\n", l_ret);
      return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_PSD_ANALYSIS

OAL_STATIC oal_uint32  wal_hipriv_set_psd(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_value;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_cfg_net_dev) || (OAL_PTR_NULL == pc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_set_psd::pst_cfg_net_dev or pc_param null ptr error %d, %d!}\r\n", pst_cfg_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* PSD设置开关的命令: hipriv "vap0 set_psd 0 | 1"
            此处将解析出"1"或"0"存入ac_name
    */

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_psd::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    if (0 == (oal_strcmp("0", ac_name)))
    {
        ul_value = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        ul_value = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_psd::the set psd command is error %d!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_PSD, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_value;

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_psd::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_hipriv_cfg_psd(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    oal_int32                  l_ret;
    oal_bool_enum_uint8        en_enable;
    wal_msg_write_stru         st_write_msg;

    /* PSD设置开关的命令: hipriv "vap0 cfg_psd */

    en_enable = 1;
    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CFG_PSD, OAL_SIZEOF(oal_uint32));

    /* 设置配置命令参数 */
    *((oal_bool_enum_uint8 *)(st_write_msg.auc_value)) = en_enable;

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_cfg_psd::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


#endif
#ifdef _PRE_WLAN_FEATURE_CSI

OAL_STATIC oal_uint32  wal_hipriv_set_csi(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_csi_param_stru         *pst_cfg_csi_param;
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0,0,0,0,0,0};
    oal_uint8                       uc_ta_check;
    oal_uint8                       uc_csi_en;



    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_cfg_net_dev) || (OAL_PTR_NULL == pc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_hipriv_set_csi::pst_cfg_net_dev or pc_param null ptr error %d, %d!}\r\n", pst_cfg_net_dev, pc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* PSD设置开关的命令: hipriv "vap0 set_csi ta ta_check csi_en"
       TA:被测量CSI的mac地址，为0表示不使能
       TA_check: 为1时，TA有效，表示每次采集CSI信息时需比对ta。
       csi_en:   为1时，表示使能CSI采集
    */

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_csi::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    oal_strtoaddr(ac_name, auc_mac_addr);

    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_csi::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_ta_check = (oal_uint8)oal_atoi(ac_name);

    pc_param = pc_param + ul_off_set;
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    uc_csi_en = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_CSI, OAL_SIZEOF(mac_cfg_csi_param_stru));

    /* 设置配置命令参数 */
    pst_cfg_csi_param = (mac_cfg_csi_param_stru*)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_cfg_csi_param->auc_mac_addr, auc_mac_addr);
    pst_cfg_csi_param->en_ta_check = uc_ta_check;
    pst_cfg_csi_param->en_csi = uc_csi_en;

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_csi_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_csi::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

OAL_STATIC oal_uint32  wal_hipriv_lpm_soc_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_lpm_soc_set_stru       *pst_set_para;
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;

    /* SOC节能测试模式配置, hipriv "Hisilicon0 lpm_soc_mode 0|1|2|3|4(总线gating|PCIE RD BY PASS|mem precharge|PCIE L0-S|PCIE L1-0)
                        0|1(disable|enable) pcie_idle(PCIE低功耗空闲时间1~7us) "*/

    pst_set_para = (mac_cfg_lpm_soc_set_stru*)(st_write_msg.auc_value);
    /* 设置配置命令参数 */
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* 获取测试模式*/
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_soc_mode::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pst_set_para->en_mode= (mac_lpm_soc_set_enum_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取开启还是关闭*/
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_soc_mode::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_set_para->uc_on_off = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取PCIE空闲时间配置*/
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_soc_mode::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_set_para->uc_pcie_idle = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_SOC_MODE,OAL_SIZEOF(mac_cfg_lpm_soc_set_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_soc_set_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_soc_mode::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}

#ifdef _PRE_WLAN_CHIP_TEST

OAL_STATIC oal_uint32  wal_hipriv_lpm_chip_state(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set;
    mac_cfg_lpm_sleep_para_stru     *pst_set_para;

    pst_set_para = (mac_cfg_lpm_sleep_para_stru*)(st_write_msg.auc_value);
    /* 设置配置命令参数 */
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    pst_set_para->uc_pm_switch = (mac_lpm_state_enum_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取定时睡眠参数*/
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_set_para->us_sleep_ms = (oal_uint16)oal_atoi(ac_name);


    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_CHIP_STATE,OAL_SIZEOF(mac_cfg_lpm_sleep_para_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_sleep_para_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_chip_state::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_lpm_psm_param(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_lpm_psm_param_stru     *pst_psm_para;
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];


    /* psm节能寄存器配置, hipriv "Hisilicon0 lpm_psm_param 0|1(ps off|ps on) 0|1(DTIM|listen intval) xxx(listen interval值) xxx(TBTT offset)"*/

    pst_psm_para = (mac_cfg_lpm_psm_param_stru*)(st_write_msg.auc_value);
    /* 设置配置命令参数 */
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* 获取节能是否开启*/
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_psm_para->uc_psm_on = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;


    /* 获取是DTIM唤醒还是listen interval唤醒 */
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_psm_para->uc_psm_wakeup_mode = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取listen interval的值 */
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_psm_para->us_psm_listen_interval = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取TBTT中断提前量的值 */
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_psm_para->us_psm_tbtt_offset = (oal_uint8)oal_atoi(ac_name);
    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_PSM_PARAM,OAL_SIZEOF(mac_cfg_lpm_psm_param_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_psm_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_psm_param::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_lpm_smps_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set;
    oal_uint8                       uc_smps_mode;

    wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);

    uc_smps_mode = (oal_uint8)oal_atoi(ac_name);
    if (uc_smps_mode >= 3)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_smps_mode::invalid choice [%d]!}\r\n", uc_smps_mode);

        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }
    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    /* 设置配置命令参数 */
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_smps_mode;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_SMPS_MODE,OAL_SIZEOF(oal_uint8));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_smps_mode::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_lpm_smps_stub(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_lpm_smps_stub_stru     *pst_smps_stub;
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /*smps ap发包打桩, hipriv "vap0 lpm_smps_stub 0|1|2(off|单流|双流) 0|1(是否发RTS)*/
    /*设置配置命令参数 */
    pst_smps_stub = (mac_cfg_lpm_smps_stub_stru*)(st_write_msg.auc_value);
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* 获取桩类型*/
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_smps_stub->uc_stub_type = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* RTS */
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_smps_stub->uc_rts_en= (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_SMPS_STUB,OAL_SIZEOF(mac_cfg_lpm_smps_stub_stru) );

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_smps_stub_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_smps_stub::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_lpm_txopps_set(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_lpm_txopps_set_stru    *pst_txopps_set;
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];


    /* txop ps节能寄存器配置, hipriv "Hisilicon0 lpm_txopps_set 0|1(off|on|debug) 0|1(contion1 off|on) 0|1(condition2 off|on)"*/
    /* 设置配置命令参数 */
    pst_txopps_set = (mac_cfg_lpm_txopps_set_stru*)(st_write_msg.auc_value);
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* 获取节能是否开启*/
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_txopps_set->uc_txop_ps_on = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;


    /* 获取condition1 */
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_txopps_set->uc_conditon1 = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取condition2*/
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_txopps_set->uc_conditon2 = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_TXOP_PS_SET,OAL_SIZEOF(mac_cfg_lpm_txopps_set_stru) );

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_txopps_set_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_txopps_set::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_lpm_txopps_tx_stub(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_int32                           l_ret;
    mac_cfg_lpm_txopps_tx_stub_stru    *pst_txopps_tx_stub;
    oal_uint32                          ul_off_set = 0;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /* txop ps发包测试打桩条件, hipriv "vap0 lpm_txopps_tx_stub 0|1(off|on) xxx(第几个包打桩)"*/
    /* 设置配置命令参数 */
    pst_txopps_tx_stub = (mac_cfg_lpm_txopps_tx_stub_stru*)(st_write_msg.auc_value);
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* 获取桩类型*/
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_txopps_tx_stub->uc_stub_on = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取第几个报文打桩 */
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_txopps_tx_stub->us_begin_num = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;


    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_TXOP_TX_STUB,OAL_SIZEOF(mac_cfg_lpm_txopps_tx_stub_stru) );

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_txopps_tx_stub_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_txopps_tx_stub::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_lpm_tx_data(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_lpm_tx_data_stru       *pst_lpm_tx_data;
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /* 测试发包, hipriv "vap0 lpm_tx_data xxx(个数) xxx(长度) xx:xx:xx:xx:xx:xx(目的mac) xxx(AC类型)"*/
    pst_lpm_tx_data = (mac_cfg_lpm_tx_data_stru*)(st_write_msg.auc_value);
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* 获取发包个数*/
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_lpm_tx_data->us_num= (oal_uint16)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取发包长度 */
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_lpm_tx_data->us_len = (oal_uint16)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取目的地址 */
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    oal_strtoaddr(ac_name, pst_lpm_tx_data->auc_da);
    pc_param = pc_param + ul_off_set;

    /* 获取发包AC类型 */
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_lpm_tx_data->uc_ac = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;
    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_TX_DATA,OAL_SIZEOF(mac_cfg_lpm_tx_data_stru) );

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_tx_data_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_tx_data::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_lpm_tx_probe_request(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_lpm_tx_data_stru       *pst_lpm_tx_data;
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /* 测试发包, hipriv "vap0 lpm_tx_probe_request 0|1(被动|主动) xx:xx:xx:xx:xx:xx(主动模式下BSSID)"*/
    pst_lpm_tx_data = (mac_cfg_lpm_tx_data_stru*)(st_write_msg.auc_value);
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* 获取主动or被动probe request*/
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_lpm_tx_data->uc_positive = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /* 获取bssid */
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    oal_strtoaddr(ac_name, pst_lpm_tx_data->auc_da);
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_LPM_TX_PROBE_REQUEST,OAL_SIZEOF(mac_cfg_lpm_tx_data_stru) );

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_lpm_tx_data_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_tx_probe_request::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


oal_uint32  wal_hipriv_remove_user_lut(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_remove_lut_stru        *pst_param;          /* 这里复用删除用户配置命令的结构体 */
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0,0,0,0,0,0};
    mac_vap_stru                   *pst_mac_vap;
    oal_uint16                      us_user_idx;


    /* 删除恢复用户lut表, hipriv "vap0 remove_lut xx:xx:xx:xx:xx:xx(mac地址) 0|1(恢复/删除)" */
    pst_param = (mac_cfg_remove_lut_stru *)(st_write_msg.auc_value);

    /* 获取MAC地址字符串 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_remove_user_lut::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
    }

    /* 地址字符串转地址数组 */
    oal_strtoaddr(ac_name, auc_mac_addr);

    /* 获取 恢复/删除 标识 */
    pc_param += ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_remove_user_lut::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    pst_param->uc_is_remove = (oal_uint8)oal_atoi(ac_name);

    /* 根据mac地址找用户 */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);

    ul_ret = mac_vap_find_user_by_macaddr_etc(pst_mac_vap, auc_mac_addr, &us_user_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_remove_user_lut::no such user!}\r\n");

        return ul_ret;
    }

    pst_param->us_user_idx = us_user_idx;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_REMOVE_LUT, OAL_SIZEOF(mac_cfg_kick_user_param_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_kick_user_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_remove_user_lut::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

OAL_STATIC oal_uint32 wal_hipriv_parase_send_frame_body(oal_int8 *p_str_body,oal_uint8 uc_str_body_len,oal_uint8 *p_char_body,oal_uint8 *p_char_body_len)
{
    oal_uint8       uc_index;
    oal_uint8       uc_value = 0;
    oal_uint8       uc_hvalue = 0;
    oal_uint8       uc_char_body_len = 0;
    if(OAL_PTR_NULL == p_str_body || OAL_PTR_NULL == p_char_body || OAL_PTR_NULL == p_char_body_len)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_parase_send_frame_body::param is NULL,return!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if(0 != (uc_str_body_len % 2) )
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_parase_send_frame_body::uc_str_body_len=%d invalid,return!}\r\n",uc_str_body_len);
        return OAL_FAIL;
    }

    for(uc_index = 0;uc_index < uc_str_body_len;uc_index++)
    {
        if(p_str_body[uc_index] >= '0' && p_str_body[uc_index] <= '9')
        {
            uc_value = (oal_uint8)(p_str_body[uc_index] - '0');
        }
        else if(p_str_body[uc_index] >= 'A' && p_str_body[uc_index] <= 'F')
        {
            uc_value = (oal_uint8)(p_str_body[uc_index] - 'A' + 10);
        }
        else
        {
            OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_hipriv_parase_send_frame_body::p_str_body[%d]=%c invalid,return!}\r\n",uc_index,(oal_uint8)p_str_body[uc_index]);
            return OAL_FAIL;
        }

        if(0 == (uc_index % 2))
        {
            uc_hvalue = ((uc_value << 4)&0XF0);
        }
        else if(1 == (uc_index % 2))
        {
            p_char_body[uc_char_body_len] = uc_value + uc_hvalue;
            uc_value = 0;
            uc_char_body_len ++;
            if(uc_char_body_len > MAC_TEST_INCLUDE_FRAME_BODY_LEN)
            {
                OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_hipriv_parase_send_frame_body::uc_char_body_len =%d over_max = %d!}\r\n",
                    uc_char_body_len,MAC_TEST_INCLUDE_FRAME_BODY_LEN);
                return OAL_FAIL;
            }
        }
        else
        {
            uc_value = 0;
        }
    }

    *p_char_body_len = uc_char_body_len;
    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_send_frame(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_uint32                          ul_offset;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                          ul_ret;
    mac_cfg_send_frame_param_stru      *pst_test_send_frame;
    oal_uint8                           auc_mac_addr[WLAN_MAC_ADDR_LEN];

    mac_test_frame_type_enum_uint8      en_frame_type;
    oal_uint8                           uc_pkt_num = 0;
    oal_int8                            ac_str_frame_body[MAC_TEST_INCLUDE_FRAME_BODY_LEN * 2] = {0};
    oal_uint8                           uc_str_frame_body_len;
    oal_uint8                           ac_frame_body[MAC_TEST_INCLUDE_FRAME_BODY_LEN] = {0};
    oal_uint8                           uc_frame_body_len = 0;

    /* 获取帧类型 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_offset);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_frame::get frame type err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    en_frame_type = (mac_test_frame_type_enum_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_offset;

    /* 获取帧数目 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_offset);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_frame::get frame num err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_pkt_num = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_offset;

    /* 获取MAC地址字符串 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_offset);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_frame::get mac err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* 地址字符串转地址数组 */
    oal_strtoaddr(ac_name, auc_mac_addr);
    pc_param += ul_offset;

    if(MAC_TEST_MGMT_ACTION == en_frame_type ||MAC_TEST_MGMT_BEACON_INCLUDE_IE == en_frame_type)
    {
        ul_ret = wal_get_cmd_one_arg_etc(pc_param,ac_str_frame_body,&ul_offset);
        if(OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_frame::get frame body err_code = [%d]!}\r\n", ul_ret);
            return ul_ret;
        }

        pc_param += ul_offset;
        uc_str_frame_body_len = (oal_uint8)(OAL_STRLEN(ac_str_frame_body));
        ul_ret = wal_hipriv_parase_send_frame_body(ac_str_frame_body,uc_str_frame_body_len,ac_frame_body,&uc_frame_body_len);
        if(OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_frame::parase_send_frame_body err_code = [%d]!}\r\n", ul_ret);
            return ul_ret;
        }

    }

    /***************************************************************************
                                 抛事件到dmac层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_FRAME, OAL_SIZEOF(mac_cfg_send_frame_param_stru));

    /* 设置配置命令参数 */
    pst_test_send_frame = (mac_cfg_send_frame_param_stru *)(st_write_msg.auc_value);
    OAL_MEMZERO(pst_test_send_frame, OAL_SIZEOF(mac_cfg_send_frame_param_stru));
    oal_set_mac_addr(pst_test_send_frame->auc_mac_ra, auc_mac_addr);
    pst_test_send_frame->en_frame_type = en_frame_type;
    pst_test_send_frame->uc_pkt_num = uc_pkt_num;
    pst_test_send_frame->uc_frame_body_length = uc_frame_body_len;
    oal_memcopy(pst_test_send_frame->uc_frame_body, ac_frame_body,uc_frame_body_len);
    OAM_WARNING_LOG3(0, OAM_SF_ANY, "{wal_hipriv_send_frame:: frame_type = [%d] send_times = %d body_len=%d!}\r\n",
        pst_test_send_frame->en_frame_type,pst_test_send_frame->uc_pkt_num,pst_test_send_frame->uc_frame_body_length);
    ul_ret = (oal_uint32)wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_send_frame_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_frame::wal_send_cfg_event_etc return err_code [%d]!}\r\n", ul_ret);
        return (oal_uint32)ul_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_CHIP_TEST

OAL_STATIC oal_uint32  wal_hipriv_set_rx_pn(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                       ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       auc_mac_addr[OAL_MAC_ADDR_LEN];
    mac_vap_stru                   *pst_mac_vap;
    oal_uint16                      us_user_idx;
    mac_cfg_set_rx_pn_stru         *pst_rx_pn;
    oal_uint16                      us_pn = 0;
    /* 获取MAC地址字符串 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_pn::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 地址字符串转地址数组 */
    oal_strtoaddr(ac_name, auc_mac_addr);
    pc_param += ul_off_set;

    /* 获取pn号 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_rx_pn::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    us_pn = (oal_uint16)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* 根据mac地址找用户 */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    ul_ret = mac_vap_find_user_by_macaddr_etc(pst_mac_vap, auc_mac_addr, &us_user_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_set_rx_pn::no such user!}\r\n");

        return ul_ret;
    }

    pst_rx_pn = (mac_cfg_set_rx_pn_stru *)(st_write_msg.auc_value);
    pst_rx_pn->us_rx_pn = us_pn;
    pst_rx_pn->us_user_idx = us_user_idx;
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(mac_cfg_set_rx_pn_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RX_PN_REG, us_len);

    l_cfg_rst = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_hipriv_set_rx_pn::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_soft_retry(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_software_retry = 0;
    oal_uint8                       uc_retry_test = 0;
    mac_cfg_set_soft_retry_stru    *pst_soft_retry;
    /* 是否为test所设的值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_soft_retry::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_retry_test = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* 获取设定的值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_soft_retry::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_software_retry = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    pst_soft_retry = (mac_cfg_set_soft_retry_stru *)(st_write_msg.auc_value);
    pst_soft_retry->uc_retry_test = uc_retry_test;
    pst_soft_retry->uc_software_retry = uc_software_retry;
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(mac_cfg_set_soft_retry_stru);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_SOFT_RETRY, us_len);

    l_cfg_rst = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_soft_retry::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_open_addr4(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_open_addr4 = 0;

    /* 获取设定的值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_open_addr4::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_open_addr4 = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;


    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_open_addr4;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_OPEN_ADDR4, us_len);

    l_cfg_rst = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_open_addr4::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_open_wmm_test(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_open_wmm = 0;

    /* 获取设定的值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_open_wmm_test::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_open_wmm = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;


    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_open_wmm;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_OPEN_WMM_TEST, us_len);

    l_cfg_rst = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_open_wmm_test::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_chip_test_open(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_chip_test_open = 0;

    /* 获取设定的值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_chip_test_open::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_chip_test_open = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;


    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_chip_test_open;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CHIP_TEST_OPEN, us_len);

    l_cfg_rst = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_chip_test_open::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_coex(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint32                      ul_mac_ctrl = 0;
    oal_uint32                      ul_rf_ctrl  = 0;
    mac_cfg_coex_ctrl_param_stru   *pst_coex_ctrl;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_coex::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    ul_mac_ctrl = (oal_uint32)oal_atoi(ac_name);
    pc_param   += ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_coex::wal_get_cmd_2nd_arg return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    ul_rf_ctrl = (oal_uint32)oal_atoi(ac_name);
    pc_param  += ul_off_set;


    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/

    pst_coex_ctrl = (mac_cfg_coex_ctrl_param_stru *)(st_write_msg.auc_value);
    pst_coex_ctrl->ul_mac_ctrl = ul_mac_ctrl;
    pst_coex_ctrl->ul_rf_ctrl  = ul_rf_ctrl;

    us_len = OAL_SIZEOF(mac_cfg_coex_ctrl_param_stru);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_COEX, us_len);

    l_cfg_rst = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_coex::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_dfx(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dfx::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对log模块进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dfx::the log switch command is error [%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DFX_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_dfx::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC  oal_uint32 wal_hipriv_test_send_action(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;
    oal_int32                       l_cfg_rst;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    mac_cfg_send_action_param_stru  st_action_param;

    OAL_MEMZERO(&st_action_param, OAL_SIZEOF(mac_cfg_send_action_param_stru));

    /* 获取uc_category设定的值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_test_send_action::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    st_action_param.uc_category = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /* 获取目的地址 */
    ul_ret = wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_test_send_action::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, st_action_param.auc_mac_da);
    pc_param = pc_param + ul_off_set;
    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, &st_action_param, OAL_SIZEOF(mac_cfg_send_action_param_stru));
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_ACTION, OAL_SIZEOF(mac_cfg_send_action_param_stru));

    l_cfg_rst = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_send_action_param_stru),
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_test_send_action::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;

}



OAL_STATIC oal_uint32  wal_hipriv_send_pspoll(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_PSPOLL, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_pspoll::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_send_nulldata(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru           st_write_msg;
    oal_int32                    l_ret;
    mac_cfg_tx_nulldata_stru    *pst_tx_nulldata;
    oal_uint32                   ul_off_set = 0;
    oal_int8                     ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    oal_uint32                   ul_ret;

    pst_tx_nulldata = (mac_cfg_tx_nulldata_stru *)st_write_msg.auc_value;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_nulldata::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_tx_nulldata->l_is_psm = oal_atoi((const oal_int8 *)ac_name);

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_nulldata::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_tx_nulldata->l_is_qos = oal_atoi((const oal_int8 *)ac_name);

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_nulldata::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param += ul_off_set;
    pst_tx_nulldata->l_tidno = oal_atoi((const oal_int8 *)ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_NULLDATA, OAL_SIZEOF(mac_cfg_tx_nulldata_stru));
    oal_memcopy((oal_void *)st_write_msg.auc_value,
                (const oal_void *)pst_tx_nulldata,
                OAL_SIZEOF(mac_cfg_tx_nulldata_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_tx_nulldata_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_nulldata::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_clear_all_stat(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;

    /***************************************************************************
                                 抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CLEAR_ALL_STAT, OAL_SIZEOF(oal_uint32));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_clear_all_stat::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#ifdef  _PRE_WLAN_FEATURE_P2P

OAL_STATIC oal_uint32   wal_parse_ops_param(oal_int8 *pc_param, mac_cfg_p2p_ops_param_stru *pst_p2p_ops_param)
{
    oal_uint32                  ul_ret;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_off_set;
    oal_int32                   l_ct_window;

    /* 解析第一个参数，是否使能OPS 节能 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::wal_get_cmd_one_arg_etc 1 return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param   += ul_off_set;

    if (0 == (oal_strcmp("0", ac_name)))
    {
        pst_p2p_ops_param->en_ops_ctrl  = OAL_FALSE;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        pst_p2p_ops_param->en_ops_ctrl = OAL_TRUE;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::the log switch command[%c] is error!}",(oal_uint8)ac_name[0]);
        OAL_IO_PRINT("{wal_parse_ops_param::the log switch command is error [%6s....]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* 解析第二个参数，OPS 节能CT Window */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    OAL_IO_PRINT("wal_parse_ops_param:ct window %s\r\n", ac_name);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::wal_get_cmd_one_arg_etc 2 return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    l_ct_window = oal_atoi(ac_name);
    if (l_ct_window < 0 || l_ct_window > 255)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::ct window out off range [%d]!}\r\n", l_ct_window);
        return OAL_FAIL;
    }
    else
    {
        pst_p2p_ops_param->uc_ct_window = (oal_uint8)l_ct_window;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32   wal_parse_noa_param(oal_int8 *pc_param, mac_cfg_p2p_noa_param_stru *pst_p2p_noa_param)
{
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_off_set;
    oal_int32                   l_count;
    oal_uint32                  ul_ret;


    /* 解析第一个参数，start_time */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_parse_noa_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param   += ul_off_set;

    pst_p2p_noa_param->ul_start_time = (oal_uint32)oal_atoi(ac_name);

    /* 解析第二个参数，dulration */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_noa_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param   += ul_off_set;

    pst_p2p_noa_param->ul_duration = (oal_uint32)oal_atoi(ac_name);

    /* 解析第三个参数，interval */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_noa_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param   += ul_off_set;

    pst_p2p_noa_param->ul_interval = (oal_uint32)oal_atoi(ac_name);

    /* 解析第四个参数，count */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_noa_param::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param   += ul_off_set;

    l_count = oal_atoi(ac_name);

    if (l_count < 0 || l_count > 255)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::ct window out off range [%d]!}\r\n", l_count);
        return OAL_FAIL;
    }
    else
    {
        pst_p2p_noa_param->uc_count  = (oal_uint8)l_count;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_p2p_ps(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    mac_cfg_p2p_ops_param_stru  st_p2p_ops_param;
    mac_cfg_p2p_noa_param_stru  st_p2p_noa_param;
    mac_cfg_p2p_stat_param_stru st_p2p_stat_param;
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;
    oal_uint16                  us_len;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_set_p2p_ps::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    pc_param   += ul_off_set;

    /* 针对解析出的不同命令，对log模块进行不同的设置 */
    if (0 == (oal_strcmp("ops", ac_name)))
    {
        /* 设置P2P OPS 节能参数 */
        ul_ret = wal_parse_ops_param(pc_param, &st_p2p_ops_param);
        if (OAL_SUCC != ul_ret)
        {
            return ul_ret;
        }
        OAM_INFO_LOG2(0, OAM_SF_CFG, "{wal_hipriv_set_p2p_ps ops::ctrl[%d], ct_window[%d]!}\r\n",
                        st_p2p_ops_param.en_ops_ctrl,
                        st_p2p_ops_param.uc_ct_window);
        us_len = OAL_SIZEOF(mac_cfg_p2p_ops_param_stru);
        WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_P2P_PS_OPS, OAL_SIZEOF(mac_cfg_p2p_ops_param_stru));
        oal_memcopy(st_write_msg.auc_value, &st_p2p_ops_param, OAL_SIZEOF(mac_cfg_p2p_ops_param_stru));
    }
    else if (0 == (oal_strcmp("noa", ac_name)))
    {
        /* 设置P2P NOA 节能参数 */
        ul_ret = wal_parse_noa_param(pc_param, &st_p2p_noa_param);
        if (OAL_SUCC != ul_ret)
        {
            return ul_ret;
        }
        OAM_INFO_LOG4(0, OAM_SF_CFG, "{wal_hipriv_set_p2p_ps noa::start_time[%d], duration[%d], interval[%d], count[%d]!}\r\n",
                        st_p2p_noa_param.ul_start_time,
                        st_p2p_noa_param.ul_duration,
                        st_p2p_noa_param.ul_interval,
                        st_p2p_noa_param.uc_count);
        us_len = OAL_SIZEOF(mac_cfg_p2p_noa_param_stru);
        WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_P2P_PS_NOA, OAL_SIZEOF(mac_cfg_p2p_noa_param_stru));
        oal_memcopy(st_write_msg.auc_value, &st_p2p_noa_param, OAL_SIZEOF(mac_cfg_p2p_noa_param_stru));
    }
    else if (0 == (oal_strcmp("statistics", ac_name)))
    {
        /* 获取P2P节能统计 */
        /* 解析参数，查看节能统计 */
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_parse_ops_param::wal_get_cmd_one_arg_etc 1 return err_code [%d]!}\r\n", ul_ret);
            return ul_ret;
        }
        pc_param   += ul_off_set;
        if (0 == (oal_strcmp("0", ac_name)))
        {
            st_p2p_stat_param.uc_p2p_statistics_ctrl = 0;
        }
        else if (0 == (oal_strcmp("1", ac_name)))
        {
            st_p2p_stat_param.uc_p2p_statistics_ctrl = 1;
        }
        else
        {
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_hipriv_set_p2p_ps statistics::wrong parm\r\n}");
            return OAL_FAIL;
        }

        us_len = OAL_SIZEOF(mac_cfg_p2p_stat_param_stru);
        WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_P2P_PS_STAT, us_len);
        oal_memcopy(st_write_msg.auc_value, &st_p2p_stat_param, us_len);
        OAM_INFO_LOG2(0, OAM_SF_CFG, "{wal_hipriv_set_p2p_ps statistics::ctrl[%d], len:%d!}\r\n",
                        st_p2p_stat_param.uc_p2p_statistics_ctrl, us_len);
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_p2p_ps::the log switch command is error [%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_p2p_ps::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_GREEN_AP

OAL_STATIC oal_uint32  wal_hipriv_set_gap_free_ratio(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    mac_cfg_free_ratio_set_stru     *pst_set_para;
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /* hipriv "Hisilicon0 set_free_ratio 0|1 val" */

    pst_set_para = (mac_cfg_free_ratio_set_stru*)(st_write_msg.auc_value);
    /* 设置配置命令参数 */
    OAL_MEMZERO(ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN);

    /* 获取测试模式*/
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_set_para->en_mode= (mac_lpm_soc_set_enum_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /* 获取开启还是关闭*/
    wal_get_cmd_one_arg_etc((oal_int8*)pc_param, ac_name, &ul_off_set);
    pst_set_para->uc_th_value = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FREE_RATIO_SET,OAL_SIZEOF(mac_cfg_free_ratio_set_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_free_ratio_set_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_lpm_soc_mode::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)

OAL_STATIC oal_uint32 wal_hipriv_enable_pmf(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{

    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_ret;

    oal_int32                       l_cfg_rst;
    oal_uint16                      us_len;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_off_set = 0;
    oal_uint8                       uc_chip_test_open = 0;

    /* 获取设定的值 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_enable_pmf::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    uc_chip_test_open = (oal_uint8)oal_atoi(ac_name);
    pc_param += ul_off_set;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    us_len = OAL_SIZEOF(oal_uint8);
    *(oal_uint8 *)(st_write_msg.auc_value) = uc_chip_test_open;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PMF_ENABLE, us_len);

    l_cfg_rst = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_cfg_rst))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_enable_pmf::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_cfg_rst);
      return (oal_uint32)l_cfg_rst;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA

OAL_STATIC oal_uint32  wal_hipriv_set_oma(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_set_oma_param_stru     *pst_set_oma_param;
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];

    /* 设置Proxy STA 的OMA地址命令 sh hipriv.sh "vap0 set_vma xx xx xx xx xx xx" */

    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_PROXYSTA, "{wal_hipriv_set_oma::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, auc_mac_addr);

    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_OMA, OAL_SIZEOF(mac_cfg_set_oma_param_stru));

    /* 设置配置命令参数 */
    pst_set_oma_param = (mac_cfg_set_oma_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_set_oma_param->auc_mac_addr, auc_mac_addr);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_oma_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_oma::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_proxysta_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    /* proxysta模块的开关的命令: hipriv "Hisilicon0 proxysta_switch 0 | 1"
        此处将解析出"1"或"0"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }

    /* 针对解析出的不同命令，对proxysta模块进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PROXYSTA_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_DFT_REG

OAL_STATIC oal_uint32  wal_hipriv_dump_reg(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oam_reg_type_enum_uint8         en_reg_type = 0;
    oal_uint32                      ul_reg_subtype = 0;
    oal_uint8                       uc_flag = 0;
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;


    if ((0 != oal_strcmp(ac_name, "soc"))
    && (0 != oal_strcmp(ac_name, "mac"))
    && (0 != oal_strcmp(ac_name, "phy"))
    && (0 != oal_strcmp(ac_name, "abb"))
    && (0 != oal_strcmp(ac_name, "rf")))
    {
        return OAL_FAIL;
    }

    if (0 == oal_strcmp(ac_name, "soc"))
    {
        en_reg_type = OAM_REG_SOC;
    }
    if (0 == oal_strcmp(ac_name, "mac"))
    {
        en_reg_type = OAM_REG_MAC;
    }
    if (0 == oal_strcmp(ac_name, "phy"))
    {
        en_reg_type = OAM_REG_PHY;
    }
    if (0 == oal_strcmp(ac_name, "abb"))
    {
        en_reg_type = OAM_REG_ABB;
    }
    if (0 == oal_strcmp(ac_name, "rf"))
    {
        en_reg_type = OAM_REG_RF;
    }

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    ul_reg_subtype = (oal_uint32)oal_atoi(ac_name);

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_flag = (oal_uint8)oal_atoi(ac_name);

    oam_reg_set_flag_etc(en_reg_type, ul_reg_subtype, uc_flag);
    OAL_IO_PRINT("dump reg: regtype %u, subtype %u,uc flag %u\n",
                        en_reg_type,
                        ul_reg_subtype,
                        uc_flag);
    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_hipriv_dump_reg_evt(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oam_reg_evt_enum_uint32         en_evt_type = 0;
    oal_uint32                      ul_tick = 0;
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    /* 获取事件类型 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg_evt::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;


    if ((0 != oal_strcmp(ac_name, "tbtt"))
    && (0 != oal_strcmp(ac_name, "rx"))
    && (0 != oal_strcmp(ac_name, "tx"))
    && (0 != oal_strcmp(ac_name, "prd"))
    && (0 != oal_strcmp(ac_name, "usr"))
    && (0 != oal_strcmp(ac_name, "all"))
    && (0 != oal_strcmp(ac_name, "err")))
    {
        return OAL_FAIL;
    }

    if (0 == oal_strcmp(ac_name, "tbtt"))
    {
        en_evt_type = OAM_REG_EVT_TBTT;
    }
    if (0 == oal_strcmp(ac_name, "rx"))
    {
        en_evt_type = OAM_REG_EVT_RX;
    }
    if (0 == oal_strcmp(ac_name, "tx"))
    {
        en_evt_type = OAM_REG_EVT_TX;
    }
    if (0 == oal_strcmp(ac_name, "prd"))
    {
        en_evt_type = OAM_REG_EVT_PRD;
    }
    if (0 == oal_strcmp(ac_name, "usr"))
    {
        en_evt_type = OAM_REG_EVT_USR;
    }
    if (0 == oal_strcmp(ac_name, "err"))
    {
        en_evt_type = OAM_REG_EVT_ERR;
    }
    if (0 == oal_strcmp(ac_name, "all"))
    {
        en_evt_type = OAM_REG_EVT_BUTT;
    }

    /* 获取tick */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg_evt::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    ul_tick = (oal_uint32)oal_atoi(ac_name);

    oam_reg_set_evt_etc(en_evt_type, ul_tick);

    if(OAM_REG_EVT_USR != en_evt_type)
    {
        return OAL_SUCC;
    }

    /***************************************************************************
                             抛事件到wal层触发数据刷新和上报
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DUMP_REG, 0);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg_evt::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_dump_reg_addr(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oam_reg_type_enum_uint8         en_reg_type = 0;
    oal_uint8                       uc_flag = 0;
    oal_uint32                      ul_addr_start = 0;
    oal_uint32                      ul_addr_end = 0;
    oal_int8                       *pc_end;
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg_addr::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* 偏移，取下一个参数 */
    pc_param = pc_param + ul_off_set;

    if ((0 != oal_strcmp(ac_name, "soc"))
    && (0 != oal_strcmp(ac_name, "mac"))
    && (0 != oal_strcmp(ac_name, "phy"))
    && (0 != oal_strcmp(ac_name, "abb"))
    && (0 != oal_strcmp(ac_name, "rf")))
    {
        return OAL_FAIL;
    }

    if (0 == oal_strcmp(ac_name, "soc"))
    {
        en_reg_type = OAM_REG_SOC;
    }
    if (0 == oal_strcmp(ac_name, "mac"))
    {
        en_reg_type = OAM_REG_MAC;
    }
    if (0 == oal_strcmp(ac_name, "phy"))
    {
        en_reg_type = OAM_REG_PHY;
    }
    if (0 == oal_strcmp(ac_name, "abb"))
    {
        en_reg_type = OAM_REG_ABB;
    }
    if (0 == oal_strcmp(ac_name, "rf"))
    {
        en_reg_type = OAM_REG_RF;
    }

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg_addr::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    //ul_addr_start = (oal_uint32)oal_atoi(ac_name);
    ul_addr_start = (oal_uint32)oal_strtol(ac_name, &pc_end, 16);

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg_addr::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    //ul_addr_end = (oal_uint32)oal_atoi(ac_name);
    ul_addr_end = (oal_uint32)oal_strtol(ac_name, &pc_end, 16);

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dump_reg_addr::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_flag = (oal_uint8)oal_atoi(ac_name);

    if((ul_addr_start % 4)
        || (ul_addr_end % 4)
        || (ul_addr_start > ul_addr_end))
    {
        OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_hipriv_dump_reg_addr::start %u, end %u Err [%d] [%d]!}\r\n", ul_addr_start, ul_addr_end);
        return OAL_FAIL;
    }
    oam_reg_set_flag_addr_etc(en_reg_type, ul_addr_start, ul_addr_end, uc_flag);
    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_dump_reg_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oam_reg_info_etc();
    return OAL_SUCC;
}
#endif

#if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)

OAL_STATIC oal_uint32  wal_hipriv_m2u_tid_set(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_uint8                        uc_m2u_tid_num;
    mac_set_m2u_tid_num_stru        *pst_m2u_set_tid_param;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_tid_set::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_m2u_tid_num = (oal_uint8)oal_atoi(ac_name);
    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_M2U_TID_SET, OAL_SIZEOF(mac_set_m2u_tid_num_stru));

    /* 设置配置命令参数 */
    pst_m2u_set_tid_param = (mac_set_m2u_tid_num_stru *)(st_write_msg.auc_value);
    pst_m2u_set_tid_param->uc_m2u_tid_num = uc_m2u_tid_num;
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_set_m2u_tid_num_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_snoop_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_m2u_snoop_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_uint8                        uc_m2u_snoop_on;
    oal_uint8                        uc_m2u_mcast_mode;
    mac_cfg_m2u_snoop_on_param_stru *pst_m2u_snoop_on_param;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_snoop_on::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_m2u_snoop_on = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_snoop_on::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_m2u_mcast_mode = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_M2U_SNOOP_ON, OAL_SIZEOF(mac_cfg_m2u_snoop_on_param_stru));

    /* 设置配置命令参数 */
    pst_m2u_snoop_on_param = (mac_cfg_m2u_snoop_on_param_stru *)(st_write_msg.auc_value);
    pst_m2u_snoop_on_param->uc_m2u_snoop_on   = uc_m2u_snoop_on;
    pst_m2u_snoop_on_param->uc_m2u_mcast_mode = uc_m2u_mcast_mode;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_m2u_snoop_on_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_snoop_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_m2u_add_deny_table(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_add_m2u_deny_table_stru     *pst_m2u_deny_table_param;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_add_deny_table::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_ADD_M2U_DENY_TABLE, OAL_SIZEOF(mac_add_m2u_deny_table_stru));

    /* 设置配置命令参数 */
    pst_m2u_deny_table_param = (mac_add_m2u_deny_table_stru *)(st_write_msg.auc_value);
    if (ul_off_set <= 16)
    {
       pst_m2u_deny_table_param->ip_type = OAL_FALSE ;
       pst_m2u_deny_table_param->ul_deny_group_ipv4_addr = oal_in_aton((oal_uint8 *)ac_name);
    }
    else
    {
       pst_m2u_deny_table_param->ip_type = OAL_TRUE;
       oal_strtoipv6(ac_name, pst_m2u_deny_table_param->ul_deny_group_ipv6_addr);
    }

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_add_m2u_deny_table_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_add_deny_table::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_m2u_del_deny_table(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_del_m2u_deny_table_stru     *pst_m2u_deny_table_param;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_del_deny_table::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }


    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_DEL_M2U_DENY_TABLE, OAL_SIZEOF(mac_del_m2u_deny_table_stru));

    /* 设置配置命令参数 */
    pst_m2u_deny_table_param = (mac_del_m2u_deny_table_stru *)(st_write_msg.auc_value);
    if (ul_off_set <= 16)
    {
       pst_m2u_deny_table_param->ip_type = OAL_FALSE ;
       pst_m2u_deny_table_param->ul_deny_group_ipv4_addr = oal_in_aton((oal_uint8 *)ac_name);
    }
    else
    {
       pst_m2u_deny_table_param->ip_type = OAL_TRUE;
       oal_strtoipv6(ac_name, pst_m2u_deny_table_param->ul_deny_group_ipv6_addr);
    }

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_del_m2u_deny_table_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_del_deny_table::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_m2u_cfg_deny_table(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_uint8                        uc_m2u_clear_deny_table;
    oal_uint8                        uc_m2u_show_deny_table;
    mac_clg_m2u_deny_table_stru     *pst_m2u_deny_table_param;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_cfg_deny_table::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_m2u_clear_deny_table = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_cfg_deny_table::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_m2u_show_deny_table = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_M2U_DENY_TABLE, OAL_SIZEOF(mac_clg_m2u_deny_table_stru));

    /* 设置配置命令参数 */
    pst_m2u_deny_table_param = (mac_clg_m2u_deny_table_stru *)(st_write_msg.auc_value);
    pst_m2u_deny_table_param->uc_m2u_clear_deny_table   = uc_m2u_clear_deny_table;
    pst_m2u_deny_table_param->uc_m2u_show_deny_table    = uc_m2u_show_deny_table;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_clg_m2u_deny_table_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_snoop_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_hipriv_m2u_show_snoop_table(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_uint8                        uc_m2u_show_snoop_table;
    mac_show_m2u_snoop_table_stru   *pst_m2u_show_snoop_table_param;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_cfg_deny_table::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_m2u_show_snoop_table = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_SHOW_M2U_SNOOP_TABLE, OAL_SIZEOF(mac_show_m2u_snoop_table_stru));

    /* 设置配置命令参数 */
    pst_m2u_show_snoop_table_param = (mac_show_m2u_snoop_table_stru *)(st_write_msg.auc_value);
    pst_m2u_show_snoop_table_param->uc_m2u_show_snoop_table   = uc_m2u_show_snoop_table;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_show_m2u_snoop_table_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_snoop_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_hipriv_m2u_show_snoop_deny_table(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                    st_write_msg;
    oal_uint32                            ul_off_set;
    oal_int8                              ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                            ul_ret;
    oal_int32                             l_ret;
    oal_uint8                             uc_m2u_show_snoop_deny_table;
    mac_show_m2u_snoop_deny_table_stru   *pst_m2u_show_snoop_deny_table_param;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_show_snoop_deny_table::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_m2u_show_snoop_deny_table = (oal_uint8)oal_atoi(ac_name);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_SHOW_M2U_SNOOP_DENY_TABLE, OAL_SIZEOF(mac_show_m2u_snoop_deny_table_stru));
    pst_m2u_show_snoop_deny_table_param = (mac_show_m2u_snoop_deny_table_stru *)(st_write_msg.auc_value);
    pst_m2u_show_snoop_deny_table_param->uc_m2u_show_snoop_deny_table = uc_m2u_show_snoop_deny_table;
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_show_m2u_snoop_deny_table_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_snoop_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_hipriv_igmp_packet_xmit(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_cfg_mpdu_ampdu_tx_param_stru *pst_aggr_tx_on_param;
    oal_uint8                       uc_packet_num;
    oal_uint8                       uc_tid;
    oal_uint16                      uc_packet_len;
    oal_uint8                       auc_ra_addr[WLAN_MAC_ADDR_LEN];

    /*设置报文的tid*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_tid = (oal_uint8)oal_atoi(ac_name);
    if (OAL_UNLIKELY(uc_tid >= WLAN_TID_MAX_NUM))
    {
        OAM_WARNING_LOG0(0, OAM_SF_M2U, "{wal_hipriv_packet_xmit::uc_tid >= WLAN_TID_MAX_NUM}\r\n");
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }
    pc_param = pc_param + ul_off_set;

    /*设置报文的个数*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param = pc_param + ul_off_set;
    uc_packet_num = (oal_uint8)oal_atoi(ac_name);
    if (OAL_UNLIKELY(uc_packet_num > WAL_HIPRIV_HT_MCS_MAX))
    {
        OAM_WARNING_LOG0(0, OAM_SF_M2U, "{wal_hipriv_packet_xmit::uc_packet_num is more than 32}\r\n");
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    /*设置报文的长度*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_packet_xmit::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    uc_packet_len = (oal_uint16)oal_atoi(ac_name);
    if (OAL_UNLIKELY(uc_packet_len <= WAL_IWPRIV_IGMP_MIN_LEN))
    {
        OAM_WARNING_LOG0(0, OAM_SF_M2U, "{wal_hipriv_packet_xmit::uc_packet_len is less than 50}\r\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    pc_param += ul_off_set;

    /* 获取MAC地址字符串 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_packet_xmit::get mac err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    /* 地址字符串转地址数组 */
    OAL_MEMZERO(auc_ra_addr,WLAN_MAC_ADDR_LEN);
    oal_strtoaddr(ac_name, auc_ra_addr);
    pc_param += ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_IGMP_PACKET_XMIT, OAL_SIZEOF(mac_cfg_mpdu_ampdu_tx_param_stru));

    /* 设置配置命令参数 */
    pst_aggr_tx_on_param = (mac_cfg_mpdu_ampdu_tx_param_stru *)(st_write_msg.auc_value);
    pst_aggr_tx_on_param->uc_packet_num = uc_packet_num;
    pst_aggr_tx_on_param->uc_tid        = uc_tid;
    pst_aggr_tx_on_param->us_packet_len = uc_packet_len;
    oal_set_mac_addr(pst_aggr_tx_on_param->auc_ra_mac, auc_ra_addr);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_mpdu_ampdu_tx_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_xmit::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_HERA_MCAST

OAL_STATIC oal_uint32  wal_hipriv_m2u_frequency_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_uint8                        uc_m2u_frequency_on;
    mac_cfg_m2u_frequency_on_param_stru *pst_m2u_frequency_on_param;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_frequency_on::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_m2u_frequency_on = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_M2U_FREQUENCY_ON, OAL_SIZEOF(mac_cfg_m2u_snoop_on_param_stru));

    /* 设置配置命令参数 */
    pst_m2u_frequency_on_param = (mac_cfg_m2u_frequency_on_param_stru *)(st_write_msg.auc_value);
    pst_m2u_frequency_on_param->uc_m2u_frequency_on   = uc_m2u_frequency_on;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_m2u_frequency_on_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_frequency_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_m2u_adaptive_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_uint8                        uc_m2u_adaptive_on;
    oal_uint32                       ul_threshold_time;
    oal_uint8                        uc_adaptive_num;
    mac_cfg_m2u_adaptive_on_param_stru *pst_m2u_adaptive_on_param;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_adaptive_on::wal_get_cmd_one_arg_etc param1 return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_m2u_adaptive_on = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_adaptive_on::wal_get_cmd_one_arg_etc param2 return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_adaptive_num = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_m2u_adaptive_on::wal_get_cmd_one_arg_etc param3 return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    ul_threshold_time = (oal_uint32)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_M2U_ADAPTIVE_ON, OAL_SIZEOF(mac_cfg_m2u_snoop_on_param_stru));

    /* 设置配置命令参数 */
    pst_m2u_adaptive_on_param = (mac_cfg_m2u_adaptive_on_param_stru *)(st_write_msg.auc_value);
    pst_m2u_adaptive_on_param->uc_m2u_adaptive_on   = uc_m2u_adaptive_on;
    pst_m2u_adaptive_on_param->ul_threshold_time    = ul_threshold_time;
    pst_m2u_adaptive_on_param->uc_adaptive_num      = uc_adaptive_num;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_m2u_adaptive_on_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_M2U, "{wal_hipriv_m2u_adaptive_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_QOS_ENHANCE

OAL_STATIC oal_uint32  wal_hipriv_qos_enhance_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_uint8                        uc_qos_enhance_on;
    oal_int32                        l_idx = 0;
    mac_cfg_qos_enhance_on_param_stru *pst_qos_enhance_on_param;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_qos_enhance_on::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    /* 输入命令合法性检测 */
    while ('\0' != ac_name[l_idx])
    {
        if (isdigit(ac_name[l_idx]))
        {
            l_idx++;
            continue;
        }
        else
        {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_qos_enhance_on::input illegal!}");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    uc_qos_enhance_on = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_QOS_ENHANCE_ON, OAL_SIZEOF(mac_cfg_qos_enhance_on_param_stru));

    /* 设置配置命令参数 */
    pst_qos_enhance_on_param = (mac_cfg_qos_enhance_on_param_stru *)(st_write_msg.auc_value);
    pst_qos_enhance_on_param->uc_qos_enhance_on   = uc_qos_enhance_on;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_qos_enhance_on_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_QOS, "{wal_hipriv_qos_enhance_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP

OAL_STATIC oal_uint32  wal_hipriv_proxyarp_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_bool_enum_uint8              en_proxyarp_on;
    mac_proxyarp_en_stru            *pst_proxyarp_on_param;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proxyarp_on::get cmd  err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    en_proxyarp_on = (oal_uint8)oal_atoi(ac_name);
    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PROXYARP_EN, OAL_SIZEOF(mac_proxyarp_en_stru));

    /* 设置配置命令参数 */
    pst_proxyarp_on_param = (mac_proxyarp_en_stru *)(st_write_msg.auc_value);
    pst_proxyarp_on_param->en_proxyarp = en_proxyarp_on;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_proxyarp_en_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proxyarp_on::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32  wal_hipriv_proxyarp_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_int32                        l_ret;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PROXYARP_INFO, OAL_SIZEOF(mac_cfg_m2u_snoop_on_param_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_proxyarp_info::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif /* #ifdef _PRE_DEBUG_MODE */

#endif /* #ifdef _PRE_WLAN_FEATURE_PROXY_ARP */

#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_hipriv_get_smps_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_SMPS_INFO, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_smps_info::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif
#endif

#ifdef _PRE_WLAN_PROFLING_MIPS

OAL_STATIC oal_uint32  wal_hipriv_set_mips(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                       l_mips_type;
    oal_int32                       l_switch;
    wal_msg_write_stru              st_write_msg;
    oal_mips_type_param_stru       *pst_mips_type_param;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }
    l_mips_type = oal_atoi((const oal_int8 *)ac_name);

    ul_ret = wal_get_cmd_one_arg_etc(pc_param + ul_off_set, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }
    l_switch = oal_atoi((const oal_int8 *)ac_name);


    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_MIPS, OAL_SIZEOF(oal_mips_type_param_stru));
    pst_mips_type_param = (oal_mips_type_param_stru *)st_write_msg.auc_value;
    pst_mips_type_param->l_mips_type = l_mips_type;
    pst_mips_type_param->l_switch    = l_switch;


    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_mips_type_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_mips::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_show_mips(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                       l_mips_type;
    wal_msg_write_stru              st_write_msg;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }
    l_mips_type = oal_atoi((const oal_int8 *)ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SHOW_MIPS, OAL_SIZEOF(oal_uint32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_mips_type;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_mips::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32  wal_hipriv_resume_rx_intr_fifo(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru                  st_write_msg;
    oal_uint32                          ul_off_set;
    oal_int8                            ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                          ul_ret;
    oal_int32                           l_ret;
    mac_cfg_resume_rx_intr_fifo_stru   *pst_param;
    oal_uint8                           uc_is_on;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_resume_rx_intr_fifo::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_is_on = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_RESUME_RX_INTR_FIFO, OAL_SIZEOF(mac_cfg_resume_rx_intr_fifo_stru));

    /* 设置配置命令参数 */
    pst_param = (mac_cfg_resume_rx_intr_fifo_stru *)(st_write_msg.auc_value);
    pst_param->uc_is_on = uc_is_on;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_resume_rx_intr_fifo_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_resume_rx_intr_fifo::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32  wal_hipriv_set_ampdu_mmss(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru       st_write_msg;
    oal_uint32               ul_off_set;
    oal_int8                 ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    mac_cfg_ampdu_mmss_stru  st_ampdu_mmss_cfg;
    oal_uint32               ul_ret;
    oal_int32                l_ret;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_mmss::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pc_param += ul_off_set;

    st_ampdu_mmss_cfg.uc_mmss_val = (oal_uint8)oal_atoi(ac_name);
    if (st_ampdu_mmss_cfg.uc_mmss_val > 7)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_mmss::mmss invilid [%d]!}\r\n", st_ampdu_mmss_cfg.uc_mmss_val);
        OAL_IO_PRINT("{wal_hipriv_set_ampdu_mmss::mmss invilid [%d]!}\r\n", st_ampdu_mmss_cfg.uc_mmss_val);

        return OAL_FAIL;
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_AMPDU_MMSS, OAL_SIZEOF(st_ampdu_mmss_cfg));

    /* 填写消息体，参数 */
    oal_memcopy(st_write_msg.auc_value, &st_ampdu_mmss_cfg, OAL_SIZEOF(st_ampdu_mmss_cfg));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_ampdu_mmss_cfg),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_ampdu_mmss::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD

oal_uint32 wal_hipriv_arp_offload_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                     ul_ret;
    oal_int32                      l_ret;
    oal_uint32                     ul_off_set;
    oal_switch_enum_uint8          en_switch;
    oal_int8                       ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    wal_msg_write_stru             st_write_msg;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "wal_hipriv_arp_offload_enable return err_code: %d", ul_ret);
        return ul_ret;
    }
    en_switch = (oal_switch_enum_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ENABLE_ARP_OFFLOAD, OAL_SIZEOF(oal_switch_enum_uint8));
    *(oal_switch_enum_uint8 *)(st_write_msg.auc_value) = en_switch;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_switch_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_hipriv_arp_offload_enable::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


oal_uint32 wal_hipriv_show_arpoffload_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_uint8                        uc_show_ip_addr;
    oal_uint8                        uc_show_arpoffload_info;
    mac_cfg_arpoffload_info_stru     *pst_arpoffload_info;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_arpoffload_info::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }

    uc_show_ip_addr = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_show_arpoffload_info::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    uc_show_arpoffload_info = (oal_uint8)oal_atoi(ac_name);
    pc_param = pc_param + ul_off_set;

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SHOW_ARPOFFLOAD_INFO, OAL_SIZEOF(mac_cfg_arpoffload_info_stru));

    /* 设置配置命令参数 */
    pst_arpoffload_info = (mac_cfg_arpoffload_info_stru *)(st_write_msg.auc_value);
    pst_arpoffload_info->uc_show_ip_addr            = uc_show_ip_addr;
    pst_arpoffload_info->uc_show_arpoffload_info    = uc_show_arpoffload_info;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_arpoffload_info_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_hipriv_show_arpoffload_info::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM

OAL_STATIC oal_uint32 wal_hipriv_roam_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                 ul_ret;
    oal_int32                  l_ret;
    oal_uint32                 ul_off_set;
    oal_bool_enum_uint8        en_enable;
    oal_int8                   ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    wal_msg_write_stru         st_write_msg;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "roam enable type return err_code [%d]", ul_ret);
        return ul_ret;
    }
    en_enable = (oal_bool_enum_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ROAM_ENABLE, OAL_SIZEOF(oal_uint32));
    *((oal_bool_enum_uint8 *)(st_write_msg.auc_value)) = en_enable;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_roam_enable::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32 wal_hipriv_roam_org(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                 ul_ret;
    oal_int32                  l_ret;
    oal_uint32                 ul_off_set;
    oal_uint8                  uc_org;
    oal_int8                   ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    wal_msg_write_stru         st_write_msg;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "roam org type return err_code[%d]", ul_ret);
        return ul_ret;
    }
    uc_org = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ROAM_ORG, OAL_SIZEOF(oal_uint32));
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_org;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{wal_hipriv_roam_org::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32 wal_hipriv_roam_band(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                 ul_ret;
    oal_int32                  l_ret;
    oal_uint32                 ul_off_set;
    oal_uint8                  uc_band;
    oal_int8                   ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    wal_msg_write_stru         st_write_msg;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "roam band type return err_code[%d]", ul_ret);
        return ul_ret;
    }
    uc_band = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ROAM_BAND, OAL_SIZEOF(oal_uint32));
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_band;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{wal_hipriv_roam_band::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32 wal_hipriv_roam_start(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_int32                  l_ret;
    oal_uint32                 ul_ret;
    oal_uint8                  uc_scan_type = 0;
    oal_bool_enum_uint8        en_current_bss_ignore = OAL_FALSE;
    wal_msg_write_stru         st_write_msg;
    mac_cfg_set_roam_start_stru *pst_roam_start;

    oal_uint32                 ul_off_set;
    oal_uint8                  uc_param;
    oal_int8                   ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_ERR_CODE_PTR_NULL == ul_ret)
    {
        /* 默认扫描+漫游 */
        uc_scan_type = 0;
        en_current_bss_ignore = OAL_FALSE;
    }
    else if (OAL_SUCC == ul_ret)
    {
        /* 指定漫游时刻是否搭配扫描操作 */
        uc_scan_type  = (oal_uint8)oal_atoi(ac_name);

        pc_param += ul_off_set;
        /* 获取是否可以关联到自己的参数 */
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if (OAL_ERR_CODE_PTR_NULL == ul_ret)
        {
            /* 默认不能关联到自己 */
            en_current_bss_ignore = OAL_FALSE;
        }
        else if (OAL_SUCC == ul_ret)
        {
            uc_param = (oal_uint8)oal_atoi(ac_name);
            en_current_bss_ignore = (uc_param > 0 ) ? OAL_TRUE : OAL_FALSE;
        }
        else
        {
            OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{wal_hipriv_roam_start::roam_start return err_code [%d]!}\r\n", ul_ret);
            return (oal_uint32)ul_ret;
        }
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{wal_hipriv_roam_start::roam_start cfg_cmd error[%d]}", ul_ret);
        return ul_ret;
    }

    OAM_WARNING_LOG2(0, OAM_SF_ROAM, "{wal_hipriv_roam_start::roam_start uc_scan_type[%d], en_current_bss_ignore[%d]}",
        uc_scan_type, en_current_bss_ignore);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    oal_memset(&st_write_msg, 0, OAL_SIZEOF(wal_msg_write_stru));
    pst_roam_start = (mac_cfg_set_roam_start_stru *)(st_write_msg.auc_value);
    pst_roam_start->uc_scan_type = uc_scan_type;
    pst_roam_start->en_current_bss_ignore = en_current_bss_ignore;
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ROAM_START, OAL_SIZEOF(mac_cfg_set_roam_start_stru));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_set_roam_start_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{wal_hipriv_roam_enable::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_hipriv_roam_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_int32                  l_ret;
    oal_bool_enum_uint8        en_enable;
    wal_msg_write_stru         st_write_msg;

    en_enable = 1;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ROAM_INFO, OAL_SIZEOF(oal_uint32));
    *((oal_bool_enum_uint8 *)(st_write_msg.auc_value)) = en_enable;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{wal_hipriv_roam_enable::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif //_PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_HILINK_DEBUG

OAL_STATIC oal_uint32  wal_hipriv_fbt_set_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_fbt_set_mode::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_fbt_set_mode::the value is invalid[%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FBT_SET_MODE, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_fbt_set_mode::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}

OAL_STATIC oal_uint32  wal_hipriv_fbt_scan_list_clear(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;


    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FBT_SCAN_LIST_CLEAR, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_fbt_monitor_list_clear::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_hipriv_fbt_scan_specified_sta(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                      ul_ret;
    oal_int32                       l_ret;
    mac_fbt_scan_sta_addr_stru    *pst_specified_sta_param;
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];


    /* 获取mac地址 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_HILINK, "{wal_hipriv_fbt_monitor_specified_sta::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }
    oal_strtoaddr(ac_name, auc_mac_addr);
    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FBT_SCAN_SPECIFIED_STA, OAL_SIZEOF(mac_fbt_scan_sta_addr_stru));

    /* 设置配置命令参数 */
    pst_specified_sta_param = (mac_fbt_scan_sta_addr_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_specified_sta_param->auc_mac_addr, auc_mac_addr);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_fbt_scan_sta_addr_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_fbt_monitor_specified_sta::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_hipriv_fbt_start_scan(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint32                      ul_ret;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_hilink_scan_params         *pst_mac_cfg_fbt_scan_params;
    oal_hilink_scan_params          st_mac_cfg_fbt_scan_params;

    pst_mac_cfg_fbt_scan_params = (oal_hilink_scan_params *)pc_param;
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_del_user::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    OAL_MEMZERO((oal_uint8*)&st_mac_cfg_fbt_scan_params, OAL_SIZEOF(oal_hilink_scan_params));
    oal_strtoaddr(ac_name, st_mac_cfg_fbt_scan_params.auc_mac);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FBT_START_SCAN, OAL_SIZEOF(oal_hilink_scan_params));

    pst_mac_cfg_fbt_scan_params = (oal_hilink_scan_params *)(st_write_msg.auc_value);
    oal_memcopy(pst_mac_cfg_fbt_scan_params->auc_mac, st_mac_cfg_fbt_scan_params.auc_mac, WLAN_MAC_ADDR_LEN);
    pst_mac_cfg_fbt_scan_params->en_is_on    = 1;
    pst_mac_cfg_fbt_scan_params->ul_channel  = 0;
    pst_mac_cfg_fbt_scan_params->ul_interval = 5000;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_hilink_scan_params),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_fbt_monitor_list_clear::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}

OAL_STATIC oal_uint32  wal_hipriv_fbt_print_scan_list(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;


    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FBT_PRINT_SCAN_LIST, OAL_SIZEOF(oal_int32));

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_fbt_print_monitor_list::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_fbt_scan_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_fbt_scan_enable::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    if (0 == (oal_strcmp("0", ac_name)))
    {
        l_tmp = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        l_tmp = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_fbt_scan_enable::the value is invalid[%d]!}\r\n", ac_name);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FBT_SCAN_ENABLE, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_fbt_scan_enable::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}



OAL_STATIC oal_uint32  wal_hipriv_fbt_scan_interval(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;
    oal_int32                   l_idx = 0;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_fbt_scan_interval::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 输入命令合法性检测 */
    while ('\0' != ac_name[l_idx])
    {
        if (isdigit(ac_name[l_idx]))
        {
            l_idx++;
            continue;
        }
        else
        {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_fbt_scan_interval::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    l_tmp =oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FBT_SCAN_INTERVAL, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_fbt_scan_interval::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_fbt_scan_channel(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;
    oal_int32                   l_idx = 0;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_fbt_scan_channel::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 输入命令合法性检测 */
    while ('\0' != ac_name[l_idx])
    {
        if (isdigit(ac_name[l_idx]))
        {
            l_idx++;
            continue;
        }
        else
        {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_fbt_scan_channel::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    l_tmp =oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FBT_SCAN_CHANNEL, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_fbt_scan_channel::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  wal_hipriv_fbt_scan_report_period(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_tmp;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;
    oal_int32                   l_idx = 0;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_fbt_scan_report_period::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 输入命令合法性检测 */
    while ('\0' != ac_name[l_idx])
    {
        if (isdigit(ac_name[l_idx]))
        {
            l_idx++;
            continue;
        }
        else
        {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_fbt_scan_report_period::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    l_tmp =oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_FBT_SCAN_REPORT_PERIOD, OAL_SIZEOF(oal_int32));
    *((oal_int32 *)(st_write_msg.auc_value)) = l_tmp;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_fbt_scan_report_period::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;

}

#endif

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST

oal_uint32  wal_hipriv_chip_check(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_int32                   l_ret;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint32                  ul_ret;
    oal_int32                   l_idx = 0;
    oal_switch_enum_uint8       en_chip_check_flag;
    wal_msg_stru                *pst_rsp_msg = OAL_PTR_NULL;

    /* 获取芯片自检开关参数 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_chip_check::wal_get_cmd_one_arg_etc vap name return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 输入命令合法性检测 */
    while ('\0' != ac_arg[l_idx])
    {
        if (isdigit(ac_arg[l_idx]))
        {
            l_idx++;
            continue;
        }
        else
        {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_chip_check::input illegal!}");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    en_chip_check_flag = (oal_uint8)oal_atoi(ac_arg);

    /***************************************************************************
                                    抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_CHIP_CHECK_SWITCH, OAL_SIZEOF(oal_uint8));
    *((oal_uint8 *)(st_write_msg.auc_value)) = en_chip_check_flag;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_chip_check::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    /* 读取返回的错误码 */
    ul_ret = wal_check_and_release_msg_resp_etc(pst_rsp_msg);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_DFR, "{wal_hipriv_chip_check fail, err code[%u]!}\r\n", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  wal_hipriv_send_cw_signal(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_uint8                        uc_param;
    oal_int8                         ac_arg[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                        l_idx = 0;
    wal_msg_stru                    *pst_rsp_msg = OAL_PTR_NULL;

    /* 获取速率值字符串 */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_arg, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_cw_signal::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 输入命令合法性检测 */
    while ('\0' != ac_arg[l_idx])
    {
        if (isdigit(ac_arg[l_idx]))
        {
            l_idx++;
            continue;
        }
        else
        {
            l_idx++;
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_send_cw_signal::input illegal!}\r\n");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
    }

    /* 解析要设置为多大的值 */
    uc_param = (oal_uint8)oal_atoi(ac_arg);

    if (uc_param > 2)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_cw_signal::input val out of range [%d]!}\r\n", uc_param);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_CW_SIGNAL, OAL_SIZEOF(oal_uint8));
    *((oal_uint8 *)(st_write_msg.auc_value)) = uc_param;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_cw_signal::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
    /* 读取返回的错误码 */
    ul_ret = wal_check_and_release_msg_resp_etc(pst_rsp_msg);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_DFR, "{wal_hipriv_set_freq fail, err code[%u]!}\r\n", ul_ret);
        return ul_ret;
    }
//#endif  /* _PRE_WLAN_CHIP_TEST */
    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI

oal_uint32  wal_hipriv_dyn_cali_cfg(oal_net_device_stru *pst_net_dev, oal_int8 *puc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_uint32                      ul_off_set;
    oal_int8                        ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    mac_ioctl_dyn_cali_param_stru  *pst_dyn_cali_param = OAL_PTR_NULL;
    wal_ioctl_dyn_cali_stru         st_cyn_cali_cfg;
    oal_uint32                      ul_ret;
    oal_uint8                       uc_map_index = 0;
    oal_int32                       l_send_event_ret;

    pst_dyn_cali_param = (mac_ioctl_dyn_cali_param_stru *)(st_write_msg.auc_value);
    OAL_MEMZERO(pst_dyn_cali_param, OAL_SIZEOF(mac_ioctl_dyn_cali_param_stru));

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_net_dev) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_dyn_cali_cfg::pst_cfg_net_dev or puc_param null ptr error }\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = wal_get_cmd_one_arg_etc(puc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dyn_cali_cfg::wal_get_cmd_one_arg_etc return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    /* 寻找匹配的命令 */
    st_cyn_cali_cfg = g_ast_dyn_cali_cfg_map[0];
    while (OAL_PTR_NULL != st_cyn_cali_cfg.pc_name)
    {
        if (0 == oal_strcmp(st_cyn_cali_cfg.pc_name, ac_name))
        {
            break;
        }
        st_cyn_cali_cfg = g_ast_dyn_cali_cfg_map[++uc_map_index];
    }

    /* 没有找到对应的命令，则报错 */
    if (OAL_PTR_NULL == st_cyn_cali_cfg.pc_name)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_hipriv_dyn_cali_cfg::invalid alg_cfg command!}\r\n");
        return OAL_FAIL;
    }

    /* 记录命令对应的枚举值 */
    pst_dyn_cali_param->en_dyn_cali_cfg = g_ast_dyn_cali_cfg_map[uc_map_index].en_dyn_cali_cfg;
    /* 获取参数配置值 */
    ul_ret = wal_get_cmd_one_arg_etc(puc_param + ul_off_set, ac_name, &ul_off_set);
    if (OAL_SUCC == ul_ret)
    {
         /* 记录参数配置值 */
         pst_dyn_cali_param->us_value = (oal_uint16)oal_atoi(ac_name);
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_dyn_cali_cfg::wal_get_cmd_one_arg_etc [%d]!}\r\n", pst_dyn_cali_param->us_value);
    }

    /***************************************************************************
                             抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DYN_CALI_CFG, OAL_SIZEOF(oal_uint32));

    l_send_event_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_send_event_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_cyn_cali_set_dscr_interval::wal_send_cfg_event_etc return err code [%d]!}\r\n", l_send_event_ret);
        return (oal_uint32)l_send_event_ret;
    }
    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_USER_EXTEND

OAL_STATIC oal_uint32 wal_hipriv_user_extend_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    oal_uint32                 ul_ret;
    oal_int32                  l_ret;
    oal_uint32                 ul_off_set;
    oal_bool_enum_uint8        en_enable;
    oal_int8                   ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    wal_msg_write_stru         st_write_msg;

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_USER_EXTEND, "wal_hipriv_user_extend_enable::cmd string not correct, ret=%d.", ul_ret);
        return ul_ret;
    }
    en_enable = (oal_bool_enum_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_USER_EXTEND_ENABLE, OAL_SIZEOF(oal_uint32));
    *((oal_bool_enum_uint8 *)(st_write_msg.auc_value)) = en_enable;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_hipriv_user_extend_enable::wal_send_cfg_event_etc return err code[%d]!\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_hipriv_get_all_reg_value(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_ALL_REG_VALUE, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    wal_hipriv_wait_rsp(pst_net_dev, pc_param);
#endif

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
      OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_all_reg_value::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
      return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_get_cali_data(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));

    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';

    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_GET_CALI_DATA, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (oal_uint8 *)&st_write_msg,
                             OAL_FALSE,
                             OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_get_cali_data::wal_send_cfg_event_etc return err_code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE

OAL_STATIC oal_uint32  wal_hipriv_packet_capture(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru          st_write_msg;
    oal_uint32                  ul_capture_switch;
    oal_uint32                  ul_off_set;
    oal_int8                    ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_int32                   l_ret;
    oal_uint32                  ul_ret;

    /*
       抓包功能的开关的命令: hipriv "Hisilicon0 packet_capture 0 | 1 | 2 | 3 | 4"
       此处将解析出"0 ~ 4"存入ac_name
    */
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_packet_capture::wal_get_cmd_one_arg_etc return err_code %d!}\r\n", ul_ret);
        return ul_ret;
    }

    /* 针对解析出的不同命令，对抓包模块进行不同的设置 */
    if (0 == (oal_strcmp("0", ac_name)))
    {
        ul_capture_switch = 0;
    }
    else if (0 == (oal_strcmp("1", ac_name)))
    {
        ul_capture_switch = 1;
    }
    else if (0 == (oal_strcmp("2", ac_name)))
    {
        ul_capture_switch = 2;
    }
    else if (0 == (oal_strcmp("3", ac_name)))
    {
        ul_capture_switch = 3;
    }
    else if (0 == (oal_strcmp("4", ac_name)))
    {
        ul_capture_switch = 4;
    }
    else
    {
       OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_packet_capture::command param is error!}\r\n");
       return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
                              抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_PACKET_CAPTURE_SWITCH, OAL_SIZEOF(oal_int32));
    *((oal_uint32 *)(st_write_msg.auc_value)) = ul_capture_switch;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_cfg_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_wifi_enable::return err code %d!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_11K_STAT

OAL_STATIC oal_uint32  wal_hipriv_query_stat_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_int32                        l_ret;
    oal_uint16                       us_len;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    oal_memcopy(st_write_msg.auc_value, pc_param, OAL_STRLEN(pc_param));
    st_write_msg.auc_value[OAL_STRLEN(pc_param)] = '\0';
    us_len = (oal_uint16)(OAL_STRLEN(pc_param) + 1);
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_QUERY_STAT_INFO, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_query_stat_info::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN

OAL_STATIC oal_uint32  wal_hipriv_send_radio_meas_req(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set = 0;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    mac_cfg_radio_meas_info_stru    *pst_radio_meas_cfg;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];
    oal_uint8                        auc_mac_addr[WLAN_MAC_ADDR_LEN];    /* MAC地址 */

    /* vap0 send_radio_meas_req [action_type] [对端mac地址] [meas type] [meas duration][chn_num] [BCN meas_type] [BCN mac addr]*/

    /*sh hipriv.sh "vap0 send_radio_meas_req 0 AA:BB:CC:11:22:26 5 100 149 1 ff:ff:ff:ff:ff:ff"*/

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SEND_RADIO_MEAS_REQ, OAL_SIZEOF(mac_cfg_radio_meas_info_stru));
    pst_radio_meas_cfg = (mac_cfg_radio_meas_info_stru *)(st_write_msg.auc_value);
    OAL_MEMZERO(pst_radio_meas_cfg, OAL_SIZEOF(mac_cfg_radio_meas_info_stru));

    /* action type*/
    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_radio_meas_req::action_type return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    pst_radio_meas_cfg->uc_action_type = (oal_uint8)oal_atoi(ac_name);

    /* 获取mac地址 */
    pc_param = pc_param + ul_off_set;
    ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, auc_mac_addr, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_send_radio_meas_req::wal_hipriv_get_mac_addr_etc failed!}\r\n");
        return ul_ret;
    }
    oal_set_mac_addr(pst_radio_meas_cfg->auc_mac_addr, auc_mac_addr);

    OAM_WARNING_LOG4(0, OAM_SF_ANY, "{wal_hipriv_send_radio_meas_req::action_type=%d,mac_addr=%x:ff:ff:ff:%x:%x}",
        pst_radio_meas_cfg->uc_action_type,
        pst_radio_meas_cfg->auc_mac_addr[0],
        pst_radio_meas_cfg->auc_mac_addr[4],
        pst_radio_meas_cfg->auc_mac_addr[5]);

    if(MAC_RM_ACTION_RADIO_MEASUREMENT_REQUEST == pst_radio_meas_cfg->uc_action_type)
    {
        /* meas type*/
        pc_param = pc_param + ul_off_set;
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
             OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_radio_meas_req::means_type return err_code [%d]!}\r\n", ul_ret);
             return ul_ret;
        }
        pst_radio_meas_cfg->uc_means_type = (oal_uint8)oal_atoi(ac_name);

        /* meas duration*/
        pc_param = pc_param + ul_off_set;
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
             OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_radio_meas_req:: meas_duration return err_code [%d]!}\r\n", ul_ret);
             return ul_ret;
        }
        pst_radio_meas_cfg->us_duration = (oal_uint16)oal_atoi(ac_name);

        /* channum*/
        pc_param = pc_param + ul_off_set;
        ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
        if (OAL_SUCC != ul_ret)
        {
             OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_radio_meas_req::channum return err_code [%d]!}\r\n", ul_ret);
             return ul_ret;
        }
        pst_radio_meas_cfg->uc_channum = (oal_uint8)oal_atoi(ac_name);

        OAM_WARNING_LOG3(0, OAM_SF_ANY, "{wal_hipriv_send_radio_meas_req::us_duration=%d, means_type=%d, channum=%d}",
            pst_radio_meas_cfg->us_duration,
            pst_radio_meas_cfg->uc_means_type,
            pst_radio_meas_cfg->uc_channum);

        if(RM_RADIO_MEAS_BCN == pst_radio_meas_cfg->uc_means_type)
        {
            /* means_mode for bcn*/
            pc_param = pc_param + ul_off_set;
            ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                 OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_radio_meas_req::bcn_mode return err_code [%d]!}\r\n", ul_ret);
                 return ul_ret;
            }
            pst_radio_meas_cfg->uc_bcn_mode= (oal_uint8)oal_atoi(ac_name);

            /* 获取BSSID */
            pc_param = pc_param + ul_off_set;
            ul_ret = wal_hipriv_get_mac_addr_etc(pc_param, auc_mac_addr, &ul_off_set);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_hipriv_send_radio_meas_req::bssid failed!}\r\n");
                return ul_ret;
            }
            oal_set_mac_addr(pst_radio_meas_cfg->auc_bssid, auc_mac_addr);

            OAM_WARNING_LOG4(0, OAM_SF_ANY, "{wal_hipriv_send_radio_meas_req::bcn_mode=%d,bssid=%x:ff:ff:ff:%x:%x}",
                pst_radio_meas_cfg->uc_bcn_mode,
                pst_radio_meas_cfg->auc_bssid[3],
                pst_radio_meas_cfg->auc_bssid[4],
                pst_radio_meas_cfg->auc_bssid[5]);
        }
    }

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_radio_meas_info_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_send_neighbor_req::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_hipriv_set_11k_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param)
{
    wal_msg_write_stru               st_write_msg;
    oal_uint32                       ul_off_set = 0;
    oal_uint32                       ul_ret;
    oal_int32                        l_ret;
    oal_switch_enum_uint8            en_11k_switch;
    oal_int8                         ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN];

    /* vap0 11k_swithc 0/1*/

    ul_ret = wal_get_cmd_one_arg_etc(pc_param, ac_name, &ul_off_set);
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_11k_switch::en_11k_switch return err_code [%d]!}\r\n", ul_ret);
         return ul_ret;
    }
    en_11k_switch = (oal_uint8)oal_atoi(ac_name);

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_11K_SWITCH, OAL_SIZEOF(oal_uint8));
    *((oal_uint8 *)(st_write_msg.auc_value)) = en_11k_switch;  /* 设置配置命令参数 */

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_hipriv_set_11k_switch::return err code [%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }
    return OAL_SUCC;
}

#endif

OAL_CONST wal_hipriv_cmd_entry_stru  g_ast_hipriv_cmd_debug_etc[] =
{
    /***********************调试命令***********************/
    /* 设置TLV类型通用命令: hipriv "wlan0 set_tlv xxx 0 0"  */
    {"set_tlv",                 wal_hipriv_set_tlv_cmd},
    /* 设置Value类型通用命令: hipriv "wlan0 set_val xxx 0"  */
    {"set_val",                 wal_hipriv_set_val_cmd},

#ifdef _PRE_WLAN_FEATURE_AP_PM
    {"pm_info",                 wal_hipriv_pm_info},                /* 输出低功耗PM信息 hipriv "Hisilicon0 pm_info"*/
    {"pm_enable",               wal_hipriv_pm_enable},                /* 输出低功耗PM信息 hipriv "Hisilicon0 pm_enable 0|1"*/
    {"enable",                  wal_hipriv_wifi_enable},            /* 开启或关闭wifi: hipriv "Hisilicon0 enable 0|1" */
#endif
    {"destroy",                 wal_hipriv_del_vap_etc},                /* 删除vap私有命令为: hipriv "vap0 destroy" */

    {"global_log_switch",       wal_hipriv_global_log_switch},      /* 全局日志开关:  hipriv "Hisilicon0 global_log_switch 0 | 1*/
    {"log_switch",              wal_hipriv_vap_log_switch},         /* VAP级别的日志开关: hipriv "Hisilicon0{VAPx} log_switch 0 | 1"，该命令针对所有的VAP */
    {"feature_log_switch",      wal_hipriv_feature_log_switch},     /* 特性的INFO级别日志开关 hipriv "VAPX feature_name {0/1}"   */
    {"log_ratelimit",           wal_hipriv_log_ratelimit},          /* 特性的INFO级别日志开关 hipriv "Hisilicon0 log_ratelimit {type} {switch} {interval} {burst}"   */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {"log_pm",                  wal_hipriv_log_lowpower},           /* log低功耗模式: hipriv "Hisilicon0 log_pm 0 | 1"，log pm模式开关 */
    {"pm_switch",               wal_hipriv_pm_switch},              /* log低功耗模式: hipriv "Hisilicon0 pm_switch 0 | 1"，log pm模式开关 */
    {"power_test",              wal_hipriv_power_test},              /* log低功耗模式: hipriv "Hisilicon0 power_test 0 | 1"，功耗测试模式开 */
#endif
    {"event_switch",            wal_hipriv_event_switch},           /* OAM event模块的开关的命令: hipriv "Hisilicon0 event_switch 0 | 1"，该命令针对所有的VAP */
#if 0
    {"ota_switch",              wal_hipriv_ota_switch},             /* 设置某一种具体的ota类型开关: hipriv "Hisilicon0 ota_switch ota_type(oam_ota_type_enum_uint8) 0 | 1"，该命令针对所有的VAP */
#endif


#ifdef _PRE_WLAN_NARROW_BAND
    {"narrow_bw",               wal_hipriv_narrow_bw},                /*Start DPD Calibration*/
#endif

#ifdef _PRE_WLAN_CHIP_TEST
    {"beacon_offload_test",     wal_hipriv_beacon_offload_test},    /* 手动设置host sleep状态，仅用于测试: hipriv "Hisilicon0 host_sleep 0 | 1" */
#endif
    {"ota_beacon_on",           wal_hipriv_ota_beacon_switch},      /* 设置是否上报beacon帧开关: hipriv "Hisilicon0 ota_beacon_switch 0 | 1"，该命令针对所有的VAP */
    {"ota_switch",              wal_hipriv_ota_rx_dscr_switch},  /* 设置是否上报接收描述符帧开关: hipriv "Hisilicon0 ota_rx_dscr_switch 0 | 1"，该命令针对所有的VAP */
    {"oam_output",              wal_hipriv_oam_output},             /* 设置oam模块的信息打印位置命令:hipriv "Hisilicon0 oam_output 0~4 (oam_output_type_enum_uint8)"，该命令针对所有的VAP */
    {"ampdu_start",             wal_hipriv_ampdu_start},            /* 设置AMPDU开启的配置命令: hipriv "vap0  ampdu_start xx xx xx xx xx xx(mac地址) tidno" 该命令针对某一个VAP */
    {"auto_ba",                 wal_hipriv_auto_ba_switch},         /* 设置自动开始BA会话的开关:hipriv "vap0  auto_ba 0 | 1" 该命令针对某一个VAP */
    {"profiling_switch",        wal_hipriv_profiling_switch},       /* 设置性能测试的开关:hipriv "vap0  profiling_switch 0 | 1"  */
    {"addba_req",               wal_hipriv_addba_req},              /* 设置建立BA会话的配置命令:hipriv "vap0 addba_req xx xx xx xx xx xx(mac地址) tidno ba_policy buffsize timeout" 该命令针对某一个VAP */
    {"delba_req",               wal_hipriv_delba_req},              /* 设置删除BA会话的配置命令: hipriv "vap0 delba_req xx xx xx xx xx xx(mac地址) tidno direction" 该命令针对某一个VAP */
#ifdef _PRE_WLAN_FEATURE_WMMAC
    {"addts_req",               wal_hipriv_addts_req},              /* 设置建立TS，即发送ADDTS REQ的配置命令:hipriv "vap0 addts_req tid direction apsd up nominal_msdu_size max_msdu_size                                                                       minimum_data_rate mean_data_rate peak_data_rate minimum_phy_rate surplus_bandwidth_allowance" 该命令针对某一个VAP */
    {"delts",                   wal_hipriv_delts},                  /* 设置删除TS，即发送DELTS的配置命令: hipriv "vap0 tidno" 该命令针对某一个VAP */
    {"reassoc_req",             wal_hipriv_reassoc_req},            /*  发送重关联请求帧: hipriv "vap0 reassoc_req"*/
    {"wmmac_switch",            wal_hipriv_wmmac_switch},           /* 设置WMMAC开关，配置命令: hipriv "vap0 wmmac_switch 1/0(使能) 0|1(WMM_AC认证使能) xxx(timeout_period) factor" 整个Device */
#endif
    {"meminfo",                 wal_hipriv_mem_info},               /* 打印内存池信息: hipriv "Hisilicon0 meminfo poolid" */
    {"memleak",                 wal_hipriv_mem_leak},               /* 打印内存池信息: hipriv "Hisilicon0 memleak poolid" */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {"devicememleak",           wal_hipriv_device_mem_leak},         /* 打印内存池信息: hipriv "Hisilicon0 devicememleak poolid" */
    {"memoryinfo",              wal_hipriv_memory_info},             /* 打印内存池信息: hipriv "Hisilicon0 memoryinfo host/device" */
#endif
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
#ifdef _PRE_WLAN_CHIP_TEST
    {"pciregwrite",             wal_hipriv_pci_reg_write},          /* 写pci内部寄存器值: hipriv "Hisilicon0 pciregwrite <chip id>(芯片ID, 即PCIE编号) <addr>(寄存器地址) <val>(写入的4字节值)" */
    {"pciregread",              wal_hipriv_pci_reg_read},           /* 读pci内部寄存器值: hipriv "Hisilicon0 pciregread <chip id>(芯片ID, 即PCIE编号) <addr>(寄存器地址)" */
    {"regw5115",                wal_hipriv_5115_reg_write},         /* 写5115侧sys ctl or pci寄存器: hipriv "Hisilicon0 regw5115 <chip id>(芯片ID, 即PCIE编号) sys|pcie <addr>(寄存器地址) <val>(写入的4字节值)" */
    {"regr5115",                wal_hipriv_5115_reg_read},          /* 读5115侧sys ctl or pci寄存器: hipriv "Hisilicon0 regr5115 <chip id>(芯片ID, 即PCIE编号) sys|pcie <addr>(寄存器地址) */
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
#ifdef _PRE_WLAN_FEATURE_DFR
#ifdef _PRE_DEBUG_MODE
    {"dfr_enable",              wal_hipriv_dfr_enable},             /* 使能dfr开关 hipriv "Hisilicon0 dfr_enable 0|1" */
	{"loss_tx_comp",            wal_hipriv_trig_loss_tx_comp},      /* 触发丢失发送完成中断,1表示连续丢失发送完成中断数量， hipriv "Hisilicon0 loss_tx_comp 1" */
    {"pcie_reset",              wal_hipriv_trig_pcie_reset},        /* 触发pcie异常中断 pcie reset: hipriv "Hisilicon0 pcie_reset" */
#endif
#endif
#endif
    {"beacon_chain_switch",     wal_hipriv_beacon_chain_switch},    /* 设置beacon帧发送策略配置命令: hipriv "vap0 beacon_chain_switch 0/1" 目前采取的是单通道模式(使用通道0)，0表示关闭双路轮流发送，1表示开启，该命令针对某一个VAP */
#if 0
    {"tdls_prohi",              wal_hipriv_tdls_prohibited},                       /* 设置tdls prohibited策略配置命令: hipriv "vap0  tdls_prohi 0/1" 0表示不禁用，1表示禁用 */
    {"tdls_chaswi_prohi",       wal_hipriv_tdls_channel_switch_prohibited},        /* 设置tdls channel switch prohibited策略配置命令: hipriv "vap0 tdls_chaswi_prohi 0/1" 0表示不禁用，1表示禁用 */
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    {"coex_print",              wal_hipriv_btcoex_status_print},    /* 打印共存维测信息，sh hipriv.sh "coex_print" */
    {"coex_preempt_type",       wal_hipriv_btcoex_preempt_type},    /* 配置preempt_type，sh hipriv.sh "vap0 coex_preempt_type  0/1(硬件或者软件) 0-3/0-1"  0 noframe 1 self-cts 2 nulldata 3 qosnull  软件ps打开或者关闭 2 slot提前量 */
    {"set_coex_perf_param",     wal_hipriv_btcoex_set_perf_param},    /* 配置BA删建门限，sh hipriv.sh "vap0 set_coex_perf_param  0/1/2/3" */
#endif
    {"protocol_debug",          wal_hipriv_set_protocol_debug_info}, /* 设置打印phy维测的相关信息，sh hipriv.sh "wlan0 protocol_debug [band_force_switch 0|1|2] [2040_ch_swt_prohi 0|1] [40_intol 0|1]" */

    {"set_tx_pow",              wal_hipriv_set_tx_pow_param},    /* 配置发送功率参数: hipriv "vap0 set_tx_pow <param name> <value>" */

    {"set_ucast_data", wal_hipriv_set_ucast_data_dscr_param},    /* 打印描述符信息: hipriv "vap0 set_ucast_data <param name> <value>" */
    {"set_bcast_data", wal_hipriv_set_bcast_data_dscr_param},    /* 打印描述符信息: hipriv "vap0 set_bcast_data <param name> <value>" */
    {"set_ucast_mgmt", wal_hipriv_set_ucast_mgmt_dscr_param},    /* 打印描述符信息: hipriv "vap0 set_ucast_mgmt <param name> <value>" */
    {"set_mbcast_mgmt",wal_hipriv_set_mbcast_mgmt_dscr_param},   /* 打印描述符信息: hipriv "vap0 set_mbcast_mgmt <param name> <value>" */
#ifdef _PRE_WLAN_FEATURE_WEB_CFG_FIXED_RATE
    {"set_mode_ucast_data",     wal_hipriv_set_mode_ucast_data_dscr_param},         /* 设置指定模式单播数据帧描述符: hipriv "vap0 set_mode_ucast_data <protocol_mode> <param name> <value>" */
#endif
    {"nss",                     wal_hipriv_set_nss  },               /* 设置HT模式下的空间流个数:   hipriv "vap0 nss   <value>" */
    {"txch",                    wal_hipriv_set_rfch },               /* 设置发射通道:               hipriv "vap0 rfch  <value>" */
    {"get_thruput",             wal_hipriv_get_thruput},             /* 获取芯片的吞吐量数据        hipriv "vap0 get_thruput >" */
#ifdef _PRE_DEBUG_MODE
    {"rxch",                    wal_hipriv_set_rxch},                /* 设置常收接收通道            hipriv "vap0 rxch <value:0001/0010/0011>" */
    {"dync_txpower",            wal_hipriv_dync_txpower},            /* 设置动态功率校准开关        hipriv "Hisilicon0 dync_txpower 0/1" 0:关闭 1:打开 */
	{"get_cali_data",           wal_hipriv_get_cali_data},           /* 获取1151软件校准所有数据    hipriv.sh "Hisilicon0 get_cali_data" */
    {"dync_txpower_dbg",        wal_hipriv_dync_pow_debug_switch},   /* 设置动态校准维测开关        hipriv "Hisilicon0 dync_txpower_dbg 0/1" 参数 0:关闭, 1:打开 */
#endif
    {"set_freq_skew",           wal_hipriv_set_freq_skew},          /* 设置频偏数据                 hipriv "Hisilicon0 set_freq_skew
                                                                     * <idx chn T0Int20M T0Frac20M T1Int20M T1Frac20M T0Int40M T0Frac40M T1Int40M T1Frac40M>" */
    {"adjust_ppm",              wal_hipriv_adjust_ppm},             /* 设置ppm         hipriv "Hisilicon0 adjust_ppm ppm clock" */
    {"amsdu_start",             wal_hipriv_amsdu_start},            /* 打印寄存器信息: hipriv "vap0 amsdu_start xx xx xx xx xx xx(mac地址10进制oal_atoi) <max num> <max size> " */
    {"list_ap",                 wal_hipriv_list_ap},                /* 打印STA扫描到的AP列表: hipriv "sta0 list_ap" */
    {"list_sta",                wal_hipriv_list_sta},               /* 打印AP关联的STA列表: hipriv "sta0 list_sta" */
    {"start_scan",              wal_hipriv_start_scan},             /* 触发sta扫描: hipriv "sta0 start_scan" */
    {"start_join",              wal_hipriv_start_join},             /* 触发sta加入并认证关联: hipriv "sta0 start_join 1" 1表示扫描到的AP在device写数组下标号*/
    {"start_deauth",            wal_hipriv_start_deauth},           /* 触发sta去认证: hipriv "vap0 start_deauth" */
    {"dump_timer",              wal_hipriv_dump_timer},             /* 打印所有timer的维测信息 hipriv "vap0 dump_timer" */
    {"kick_user",               wal_hipriv_kick_user},              /* 删除1个用户 hipriv "vap0 kick_user xx xx xx xx xx xx(mac地址)" */
    {"pause_tid",               wal_hipriv_pause_tid},              /* 暂停指定用户的指定tid hipriv "vap0 pause_tid xx xx xx xx xx xx(mac地址) tid_num 0\1" */
    {"set_user_vip",            wal_hipriv_set_user_vip},           /* 设置某个用户为VIP或者非VIP，sh hipriv.sh "vap0 set_user_vip xx xx xx xx xx xx(mac地址) 0\1" */
    {"set_vap_host",            wal_hipriv_set_vap_host},           /* 设置某个vap为host或者guest vap: sh hipriv.sh "vap0 st_vap_host 0\1" */
    {"ampdu_tx_on",             wal_hipriv_ampdu_tx_on},            /* 开启或关闭ampdu发送功能 hipriv "vap0 ampdu_tx_on 0\1" */
    {"amsdu_tx_on",             wal_hipriv_amsdu_tx_on},            /* 开启或关闭ampdu发送功能 hipriv "vap0 amsdu_tx_on 0\1" */
    {"send_bar",                wal_hipriv_send_bar},               /* 指定tid发送bar hipriv "vap0 send_bar A6C758662817(mac地址) tid_num" */
#ifdef _PRE_WLAN_FEATUER_PCIE_TEST
    {"pcie_test",               wal_hipriv_pcie_test},              /* PCIE test: 0-15(总线测试粒度0-15) 0/1(读) 0/1(写): hipriv "vap0 pcie_test 0-15 0/1 0/1" */
#endif
    {"packet_xmit",             wal_hipriv_packet_xmit},            /* 向目标STA/AP发送数据帧: hipriv "vap0 packet_xmit (tid_no) (报文个数) (报文长度) (RA MAC)" */
    {"dump_ba_bitmap",          wal_hipriv_dump_ba_bitmap},         /* 打印发送ba的bitmap hipriv "vap0 dump_ba_bitmap (tid_no) (RA)" */
    {"wifi_stat_info",          wal_hipriv_show_stat_info},         /* 获取所有维测统计信息: hipriv "Hisilicon0 wifi_stat_info" */
    {"vap_pkt_stat",            wal_hipriv_show_vap_pkt_stat},      /* 获取某一个vap下的收发包统计信息: sh hipriv.sh "vap_name vap_pkt_stat" */
    {"clear_stat_info",         wal_hipriv_clear_stat_info},        /* 清零所有维测统计信息: hipriv "Hisilicon0 clear_stat_info" */
    {"usr_stat_info",           wal_hipriv_user_stat_info},         /* 上报某个user下的维测统计信息: sh hipriv.sh "Hisilicon0 usr_stat_info usr_id" */
    {"timer_start",             wal_hipriv_timer_start},            /* 开启5115硬件定时器: hipriv "Hisilicon0 timer_start 0/1" */
    {"show_profiling",          wal_hipriv_show_profiling},         /* 开启5115硬件定时器: hipriv "Hisilicon0 show_profiling 0/1/2(0是rx 1是tx 2是chipstart) 0/1/2 (0是分段数据 1是分段数据和每个节点数据 2是打印分段数据和每个节点数据，包括错误的)" */
    {"profiling_packet_add",    wal_hipriv_profiling_packet_add},   /* 开启5115硬件定时器: hipriv "Hisilicon0 profiling_packet_add" 一次发包结束，进行下一次统计，将packet num 加1 */
   // {"reset_hw",                wal_hipriv_reset_device},           /* 复位硬件phy&mac: hipriv "Hisilicon0 reset_hw 0|1|2|3(all|phy|mac|debug) 0|1(reset phy reg) 0|1(reset mac reg) */
    {"reset_hw",                wal_hipriv_reset_device},           /* 复位硬件phy&mac: hipriv "Hisilicon0 reset_hw 0|1|2|3|4|5|6|8|9|10|11
                                                                                                                    (all|phy|mac|debug|mac_tsf|mac_cripto|mac_non_cripto|phy_AGC|phy_HT_optional|phy_VHT_optional|phy_dadar )
                                                                                                                    0|1(reset phy reg) 0|1(reset mac reg) */
    {"reset_operate",           wal_hipriv_reset_operate},          /* 复位硬件phy&mac: hipriv "Hisilicon0 reset_hw 0|1|2|3(all|phy|mac|debug) 0|1(reset phy reg) 0|1(reset mac reg) */
    {"dump_rx_dscr",            wal_hipriv_dump_rx_dscr_etc},           /* dump出来接收描述符队列，hipriv "Hisilicon0 dump_rx_dscr 0|1", 0:高优先级队列 1:普通优先级队列  */
    {"dump_tx_dscr",            wal_hipriv_dump_tx_dscr},           /* dump出来发送描述符队列，hipriv "Hisilicon0 dump_tx_dscr value", value取值0~3代表AC发送队列，4代表管理帧 */
    {"dump_memory",             wal_hipriv_dump_memory},            /* dump内存， hipriv "Hisilicon0 dump_memory 0xabcd len" */
    {"show_tx_dscr_addr",       wal_hipriv_show_tx_dscr_addr},      /* 打印内存池中所有发送描述符地址 hipriv "Hisilicon0 show_tx_dscr_addr" */
    {"list_channel",            wal_hipriv_list_channel},           /* 支持信道列表， hipriv "Hisilicon0 list_channel" */
    {"set_regdomain_pwr_p",     wal_hipriv_set_regdomain_pwr_priv}, /* 设置管制域最大发送功率(可以突破管制域的限制)，hipriv "Hisilicon0 set_regdomain_pwr_priv 20",单位dBm */
    {"event_queue",             wal_hipriv_event_queue_info},       /* 打印事件队列信息，将打印出每一个非空事件队列中事件的个数，以及每一个事件头信息, hipriv "Hisilicon0 event_queue" */
    {"frag_threshold",          wal_hipriv_frag_threshold},         /* 设置分片门限的配置命令: hipriv "vap0 frag_threshold (len)" 该命令针对某一个VAP */
    {"wmm_switch",              wal_hipriv_wmm_switch},             /* 动态开启或者关闭wmm hipriv "vap0 wmm_switch 0|1"(0不使能，1使能)  */
    {"hide_ssid",               wal_hipriv_hide_ssid},              /*  隐藏ssid功能开启或者关闭 wmm hipriv "Hisilicon0 hide_ssid 0|1"(0不使能，1使能)  */
    {"ether_switch",            wal_hipriv_set_ether_switch},       /* 设置以太网帧上报的开关，sh hipriv.sh "vap0 ether_switch user_macaddr oam_ota_frame_direction_type_enum(帧方向) 0|1(开关)" */
    {"80211_uc_switch",         wal_hipriv_set_80211_ucast_switch}, /* 设置80211单播帧上报的开关，sh hipriv.sh "vap0 80211_uc_switch user_macaddr 0|1(帧方向tx|rx) 0|1(帧类型:管理帧|数据帧) 0|1(帧内容开关) 0|1(CB开关) 0|1(描述符开关)" */
    {"80211_mc_switch",         wal_hipriv_set_80211_mcast_switch}, /* 设置80211组播\广播帧上报的开关，sh hipriv.sh "Hisilicon0 80211_mc_switch 0|1(帧方向tx|rx) 0|1(帧类型:管理帧|数据帧) 0|1(帧内容开关) 0|1(CB开关) 0|1(描述符开关)" */
    {"probe_switch",            wal_hipriv_set_probe_switch},       /* 设置probe req与rsp上报的开关，sh hipriv.sh "Hisilicon0 probe_switch 0|1(帧方向tx|rx) 0|1(帧内容开关) 0|1(CB开关) 0|1(描述符开关)" */
    {"phy_debug",               wal_hipriv_set_phy_debug_switch},    /* 设置打印phy维测的相关信息，sh hipriv.sh "Hisilicon0 phy_debug [snr 0|1] [rssi 0|1] [trlr 0|1] [count N]" */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {"report_vap_info",         wal_hipriv_report_vap_info},        /* 根据标记位上报vap的对应信息 sh hipriv.sh "wlan0 report_vap_info 1" */
#endif
    {"mpdu_num",                wal_hipriv_get_mpdu_num},           /* 获取device下和每一个tid下当前mpdu个数，sh hipriv.sh "vap_name mpdu_num user_macaddr" */
    {"set_all_ota",             wal_hipriv_set_all_ota},            /* 设置所有ota上报，如果为1，则所有类型帧的cb描述符都报，如果为0，什么都不报，sh hipriv.sh "Hisilicon0 set_all_ota 0|1" */
    {"80211_uc_all",            wal_hipriv_set_all_80211_ucast},    /* 设置所有用户的单播开关，sh hipriv.sh "Hisilicon0 80211_uc_all 0|1(帧方向tx|rx) 0|1(帧类型:管理帧|数据帧) 0|1(帧内容开关) 0|1(CB开关) 0|1(描述符开关)" */
    {"ether_all",               wal_hipriv_set_all_ether_switch},   /* 设置所有用户的以太网开关，sh hipriv.sh "Hisilicon0 ether_all 0|1(帧方向tx|rx) 0|1(开关)" */
    {"dhcp_arp_switch",         wal_hipriv_set_dhcp_arp_switch},    /* 设置发送广播arp和dhcp开关，sh hipriv.sh "Hisilicon0 dhcp_arp_switch 0|1(开关)" */
#ifdef _PRE_DEBUG_MODE_USER_TRACK
    {"thrput_stat",             wal_hipriv_report_thrput_stat},     /* 上报或者停止上报反应user实时吞吐统计信息: sh hipriv.sh "vap_name thrput_stat  XX:XX:XX:XX:XX;XX 0|1" */
#endif
#ifdef _PRE_WLAN_DFT_STAT
    {"clear_vap_stat_info",     wal_hipriv_clear_vap_stat_info},    /* 清零指定VAP的统计信息: hipriv "vap_name clear_vap_stat_info" */
#endif
#ifdef _PRE_WLAN_FEATURE_TXOPPS
    {"txopps_hw_en",            wal_hipriv_set_txop_ps_machw},      /* 设置mac txop ps使能寄存器，sh hipriv.sh "stavap_name txopps_hw_en 0|1(txop_ps_en) 0|1(condition1) 0|1(condition2)" */
#endif
#ifdef _PRE_WLAN_FEATURE_UAPSD
    {"uapsd_debug",             wal_hipriv_uapsd_debug},            /* uapsd维测信息，sh hipriv "vap0 uapsd_debug 0|1|2(单用户|all user|清空统计计数器) xx:xx:xx:xx:xx:xx(mac地址)" */
#endif
#ifdef _PRE_WLAN_DFT_STAT
    {"phy_stat_en",             wal_hipriv_set_phy_stat_en},        /* 设置phy统计使能节点编号，一次可以设置4个，参数范围1~16，sh hipriv.sh "Hisilicon0 phy_stat_en idx1 idx2 idx3 idx4" */

    {"dbb_env_param",           wal_hipriv_dbb_env_param},          /* 上报或者停止上报空口环境类参数信息: sh hipriv.sh "Hisilicon0 dbb_env_param 0|1" */
    {"usr_queue_stat",          wal_hipriv_usr_queue_stat},         /* 上报或者清零用户队列统计信息: sh hipriv.sh "vap_name usr_queue_stat XX:XX:XX:XX:XX:XX 0|1" */
    {"vap_stat",                wal_hipriv_report_vap_stat},        /* 上报或者停止上报vap吞吐统计信息: sh hipriv.sh "vap_name vap _stat  0|1" */
    {"reprt_all_stat",          wal_hipriv_report_all_stat},        /* 上报或者清零所有维测统计信息: sh hipriv.sh "Hisilicon0 reprt_all_stat type(phy/machw/mgmt/irq/all)  0|1" */
#endif
#ifdef _PRE_DEBUG_MODE
    {"ampdu_stat",             wal_hipriv_report_ampdu_stat},      /* 上报或者清零ampdu维测统计信息: sh hipriv.sh "vap_name ampdu_stat XX:XX:XX:XX:XX:XX tid_no 0|1" */
#endif

    {"ampdu_aggr_num",          wal_hipriv_set_ampdu_aggr_num},     /* 设置AMPDU聚合个数: sh hipriv.sh "Hisilicon0 ampdu_aggr_num aggr_num_switch aggr_num" ,aggr_num_switch非0时，aggr_num有效 */

#ifdef _PRE_DEBUG_MODE
    {"freq_adjust",             wal_hipriv_freq_adjust},            /* 频偏调整配置命令: sh hipriv.sh "Hisilicon0 freq_adjust pll_int pll_frac" ,pll_int整数分频系数，pll_frac小数分频系数 */
#endif

    {"set_stbc_cap",            wal_hipriv_set_stbc_cap},           /* 设置STBC能力 */
    {"set_ldpc_cap",            wal_hipriv_set_ldpc_cap},           /* 设置LDPC能力 */
    {"set_txbf_cap",            wal_hipriv_set_txbf_cap},           /* 开启或关闭txbf的 接收bit0/发送bit1 能力 hipriv "vap0 alg_txbf_switch 0|1|2|3" */
#ifdef _PRE_WLAN_FEATURE_STA_PM
    {"set_psm_para",            wal_hipriv_sta_psm_param},          /* sh hipriv.sh 'wlan0 set_psm_para 100 40 */
    {"set_sta_pm_on",           wal_hipriv_sta_pm_on},              /* sh hipriv.sh 'wlan0 set_sta_pm_on xx xx xx xx */
#endif
    {"lpm_soc_mode",            wal_hipriv_lpm_soc_mode},           /* 睡眠或唤醒芯片, hipriv "Hisilicon0 lpm_soc_mode 0|1|2|3|4(总线gating|PCIE RD BY PASS|mem precharge|PCIE L0-S|PCIE L1-0) 0|1(disable|enable)" */
#ifdef _PRE_WLAN_CHIP_TEST
    {"lpm_chip_state",          wal_hipriv_lpm_chip_state},         /* 睡眠或唤醒芯片, hipriv "Hisilicon0 lpm_chip_state 0|1|2(0:soft sleep，1:gpio sleep,2:work)" */
    {"lpm_psm_param",           wal_hipriv_lpm_psm_param},          /* psm节能寄存器配置, hipriv "Hisilicon0 lpm_psm_param 0|1|2(ps off|ps on|debug) 0|1(DTIM|listen intval) xxx(listen interval值) xxx(TBTT offset)"*/
    {"lpm_smps_mode",           wal_hipriv_lpm_smps_mode},          /* smps节能模式配置, hipriv "Hisilicon0 lpm_smps_mode 0|1|2(off|static|dynamic)"*/
    {"lpm_smps_stub",           wal_hipriv_lpm_smps_stub},          /* smps ap发包打桩, hipriv "vap0 lpm_smps_stub 0|1|2(off|单流|双流) 0|1(是否发RTS)"*/
    {"lpm_txopps_set",          wal_hipriv_lpm_txopps_set},         /* txop ps节能模式配置, hipriv "Hisilicon0 lpm_txopps_set 0|1(off|on|debug) 0|1(contion1 off|on) 0|1(condition2 off|on)"*/
    {"lpm_txopps_tx_stub",      wal_hipriv_lpm_txopps_tx_stub},     /* txop ps发包测试打桩条件, hipriv "vap0 lpm_txopps_tx_stub 0|1|2(off|address|partial AID) xxx(第几个包打桩)"*/
    {"lpm_tx_data",             wal_hipriv_lpm_tx_data},            /* 测试发包, hipriv "vap0 lpm_tx_data xxx(个数) xxx(长度) xx:xx:xx:xx:xx:xx(目的mac) xxx(AC类型)"*/
    {"lpm_tx_probe_req",        wal_hipriv_lpm_tx_probe_request},   /* 测试发包, hipriv "vap0 lpm_tx_probe_req 0|1(被动|主动) xx:xx:xx:xx:xx:xx(主动模式下BSSID)"*/
    {"remove_lut",              wal_hipriv_remove_user_lut},        /* 删除恢复用户lut表, hipriv "vap0 remove_lut xx:xx:xx:xx:xx:xx(mac地址 16进制) 0|1(恢复/删除)" */
#endif
    {"send_frame",              wal_hipriv_send_frame},               /* 指定tid发送bar hipriv "vap0 send_frame (type) (num) (目的mac)" */
#ifdef _PRE_WLAN_CHIP_TEST
    {"set_rx_pn",               wal_hipriv_set_rx_pn},               /* 设置RX_PN_LUT_CONFIG寄存器 */
    {"set_sft_retry",           wal_hipriv_set_soft_retry},           /* 设置software_retry 描述符 hipriv "Hisilicon0 set_sft_retry 0|1(0不使能，1使能)"  */
    {"open_addr4",              wal_hipriv_open_addr4},             /* 设置mac头进入4地址 hipriv "Hisilicon0 open_addr4 0|1(0不使能，1使能)  */
    {"open_wmm_test",           wal_hipriv_open_wmm_test},         /* 设置芯片验证开关 hipriv "Hisilicon0 open_wmm_test 0|1|2|3  ()  */
    {"chip_test",               wal_hipriv_chip_test_open},         /* 设置芯片验证开关 hipriv "Hisilicon0 chip_test 0|1(0不使能，1使能)  */
    {"coex_ctrl",               wal_hipriv_set_coex},               /* 设置共存控制开关 hipriv "Hisilicon0 coex_ctrl xxx(mac ctrl值) xxx(rf ctrl值))  */
    {"dfx_en",                  wal_hipriv_set_dfx},                /* 设置DFX特性开关 sh hipriv.sh "Hisilicon0 dfx_en 0|1  */
    {"clear_all_stat",          wal_hipriv_clear_all_stat},         /* 清除中断和管理帧统计信息 hipriv "Hisilicon0 clear_all_stat" */
    {"send_action",             wal_hipriv_test_send_action},     /* 发送action帧接口 sh hipriv.sh "vap0 send_action XX(category) xx:xx:xx:xx:xx:xx(目的地址 16进制) " */
    {"send_pspoll",             wal_hipriv_send_pspoll},            /* sta发ps-poll给ap，sh hipriv "vap0 send_pspoll" */
    {"send_nulldata",           wal_hipriv_send_nulldata},          /* sta发null data给ap，通知节能状态，sh hipriv "vap0 send_nulldata 0|1(是否进入节能) 0|1(是否发qosnull) tid_no" */
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    {"enable_pmf",              wal_hipriv_enable_pmf},     /* 设置chip test中强制使能pmf能力 (用于关联之后)sh hipriv.sh "vap0 enable_pmf 0|1|2(0不使能，1 enable, 2强制)  */
#endif
#ifdef _PRE_WLAN_FEATURE_GREEN_AP
    {"set_free_ratio",          wal_hipriv_set_gap_free_ratio},     /* 设置green ap的free ratio门限 hipriv "Hisilicon0 set_free_ratio 0|1 val" */
#endif
    {"set_default_key",         wal_hipriv_set_default_key},     /* 设置chip test中强制使能pmf能力 (用于关联之后)sh hipriv.sh "vap0 set_default_key x(key_index) 0|1(en_unicast) 0|1(multicast)"  */
    {"add_key",                 wal_hipriv_test_add_key},         /* chip test配置add key操作的私有配置命令接口
                          sh hipriv.sh "xxx(cipher) xx(en_pairwise) xx(key_len) xxx(key_index) xxxx:xx:xx:xx:xx:xx...(key 小于32字节) xx:xx:xx:xx:xx:xx(目的地址)  */


#ifdef _PRE_WLAN_FEATURE_DFR
    {"dfr_start",              wal_hipriv_test_dfr_start},  /* dfr功能打桩触发接口sh hipriv.sh "vap0 dfr_start 0(dfr子功能:0-device异常复位 )"*/
#endif //_PRE_WLAN_FEATURE_DFR
    /* 算法相关的命令 */
    {"alg_ar_log",              wal_hipriv_ar_log},                 /* autorate算法日志参数配置:*/
    {"alg_ar_test",             wal_hipriv_ar_test},                /* autorate算法系统测试命令 */
    {"alg",                     wal_hipriv_alg},                    /* alg */

#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
    {"waveapp_enable",          wal_hipriv_waveapp_32plus_user_enable},  /* 使能waveapp 32以上多用户测试开关 hipriv "Hisilicon0 waveapp_enable 0|1" */
#endif

    {"alg_txbf_log",            wal_hipriv_txbf_log},               /* txbf算法日志参数配置:*/
    {"alg_intf_det_log",        wal_hipriv_intf_det_log},           /* 干扰检测算法日志参数配置:*/
#ifdef _PRE_SUPPORT_ACS
    {"acs",                     wal_hipriv_acs},
    {"chan_stat",               wal_hipriv_chan_stat},
#endif

    {"start_priv",              wal_hipriv_set_priv_flag},/* 只是依赖ifconfig up来启动AP时，需要先通过此接口设置一个标记*/
    {"bw_fixed",                wal_hipriv_set_bw_fixed},        /* 设置ap模式的vap带宽固定 */
#ifdef _PRE_WLAN_PERFORM_STAT
    /* 性能统计命令 */
    {"stat_tid_thrpt",          wal_hipriv_stat_tid_thrpt},        /* 统计指定tid的吞吐量: hipriv "vap0 stat_tid_thrpt xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数)" */
    {"stat_user_thrpt",         wal_hipriv_stat_user_thrpt},       /* 统计指定user的吞吐量: hipriv "vap0 stat_user_thrpt xx xx xx xx xx xx(mac地址) stat_period(统计周期ms) stat_num(统计次数)" */
    {"stat_user_bsd",           wal_hipriv_stat_user_bsd},          /* 统计指定user的吞吐量,rssi: hipriv "vap0 stat_user_bsd xx xx xx xx xx xx(mac地址) stat_period(统计周期ms) stat_num(统计次数)" */
    {"stat_vap_thrpt",          wal_hipriv_stat_vap_thrpt},        /* 统计指定tid的吞吐量: hipriv "vap0 stat_vap_thrpt stat_period(统计周期ms) stat_num(统计次数)" */
    {"stat_tid_per",            wal_hipriv_stat_tid_per},          /* 统计指定tid的per: hipriv "vap0 stat_tid_per xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数)" */
    {"stat_tid_delay",          wal_hipriv_stat_tid_delay},        /* 统计指定tid的delay: hipriv "vap0 stat_tid_delay xx xx xx xx xx xx(mac地址) tid_num stat_period(统计周期ms) stat_num(统计次数)" */

    /* 性能显示命令 */
    {"dspl_tid_thrpt",          wal_hipriv_display_tid_thrpt},      /* 统计指定tid的吞吐量: hipriv "vap0 dspl_tid_thrpt xx xx xx xx xx xx(mac地址)" */
    {"dspl_user_thrpt",         wal_hipriv_display_user_thrpt},     /* 统计指定user的吞吐量: hipriv "vap0 dspl_user_thrpt xx xx xx xx xx xx(mac地址)" */
    {"dspl_vap_thrpt",          wal_hipriv_display_vap_thrpt},      /* 统计指定tid的吞吐量: hipriv "vap0 dspl_vap_thrpt" */
    {"dspl_tid_per",            wal_hipriv_display_tid_per},        /* 统计指定tid的per: hipriv "vap0 dspl_tid_per xx xx xx xx xx xx(mac地址) tid_num" */
    {"dspl_tid_delay",          wal_hipriv_display_tid_delay},      /* 统计指定tid的delay: hipriv "vap0 dspl_tid_delay xx xx xx xx xx xx(mac地址) tid_num" */
#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    {"set_edca_weight_sta",        wal_hipriv_set_edca_opt_weight_sta},       /* STA edca参数调整权重 */
    {"set_edca_switch_ap",         wal_hipriv_set_edca_opt_switch_ap},        /* 是否开启edca优化机制 */
    {"set_edca_cycle_ap",          wal_hipriv_set_edca_opt_cycle_ap},         /* 设置edca参数调整的周期 */
#endif

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
    {"get_hipkt_stat",             wal_hipriv_get_hipkt_stat},                /* 获取高优先级报文的统计情况 */
    {"set_flowctl_param",          wal_hipriv_set_flowctl_param},             /* 设置流控相关参数 */
    {"get_flowctl_stat",           wal_hipriv_get_flowctl_stat},              /* 获取流控相关状态信息 */
#endif

#ifdef _PRE_DEBUG_MODE
    /* 维测命令:设置某个值的某个类型*/
    {"debug_switch",            wal_hipriv_set_debug_switch},        /* 设置某一种具体的debug类型开关: hipriv "Hisilicon0 debug_switch debug_type debug_value"，该命令针对设备级别调试使用 */
    {"tx_comp_cnt",             wal_hipriv_get_tx_comp_cnt},         /* 统计发送完成中断是否丢失(关闭聚合) hipriv "Hisilicon0 tx_comp_cnt 0|1", 0表示清零统计次数， 1表示显示统计次数并且清零 */
    {"set_rx_filter_val",       wal_hipriv_set_rx_filter_val},       /* 设置接收帧过滤各状态下的配置值:hipriv "Hisilicon0 set_rx_filter_val 0-Normal/1-Repeater mode status value" */
    {"get_rx_filter_val",       wal_hipriv_get_rx_filter_val},       /* 设置接收帧过滤各状态下的配置值:hipriv "Hisilicon0 get_rx_filter_val 0-Normal/1-Repeater mode status" */
    {"set_rx_filter_en",        wal_hipriv_set_rx_filter_en},        /* 读取接收帧过滤各状态下的配置值:hipriv "Hisilicon0 set_rx_filter_en 0-打开/1-关闭 */
    {"get_rx_filter_en",        wal_hipriv_get_rx_filter_en},        /* 读取接收帧过滤各状态下的配置值:hipriv "Hisilicon0 get_rx_filter_en */
    {"get_all_regs",            wal_hipriv_get_all_reg_value},               /* 获取所有寄存器的值: hipriv "Hisilicon0 get_all_regs" */
#endif

    {"thruput_bypass",          wal_hipriv_set_thruput_bypass},        /* 设置thruput bypass维测点 */
    {"auto_protection",         wal_hipriv_set_auto_protection},       /* 设置自动保护开关 */

    /* 共存维测相关 */
    {"send_2040_coext",         wal_hipriv_send_2040_coext},           /* 发送20/40共存管理帧: hipriv "Hisilicon0 send_2040_coext coext_info chan_report" */
    {"2040_coext_info",         wal_hipriv_2040_coext_info},           /* 打印vap的所有20/40共存参数信息: hipriv "vap0 2040_coext_info" */
    {"get_version",             wal_hipriv_get_version},               /* 获取软件版本: hipriv "vap0 get_version" */

#ifdef _PRE_WLAN_FEATURE_FTM
    /* hipriv.sh "wlan0 ftm_debug ---------------------------------------------------------------------------------------------------------*/
    /* -------------------------- enable_ftm_initiator [0|1]*/
    /* -------------------------- send_iftmr channel[] burst[0|1] ftms_per_burst[n] measure_req[0|1] asap[0|1] bssid[xx:xx:xx:xx:xx:xx]*/
    /* -------------------------- enable [0|1]*/
    /* -------------------------- cali [0|1]*/
    /* -------------------------- send_ftm bssid[xx:xx:xx:xx:xx:xx]*/
    /* -------------------------- set_ftm_time t1[] t2[] t3[] t4[]*/
    /* -------------------------- enable_ftm_resp [0|1]*/
    /* -------------------------- send_range_req mac[] num_rpt[] delay[] ap_cnt[] bssid[] channel[] bssid[] channel[] ...*/
    /* -------------------------- enable_ftm_range [0|1]*/
    /* -------------------------- get_cali*/
    /* -------------------------- set_location type[] mac[] mac[] mac[]*/
    /* -------------------------- set_ftm_m2s en_is_mimo[] uc_tx_chain_selection[]*/
    {"ftm_debug",              wal_hipriv_ftm},
#endif

#ifdef _PRE_WLAN_FEATURE_DAQ
    {"data_acq",                wal_hipriv_data_acq},                  /* 获取软件版本: hipriv "Hisilicon0 data_acq 0/1/2/3/4 (length num depth) (channel mode data_th bit) (2) () ()" */
#endif

#ifdef _PRE_WLAN_FEATURE_PSD_ANALYSIS
    {"set_psd",                 wal_hipriv_set_psd},                  /* 使能PSD采集: hipriv "Hisilicon0 set_psd 0/1"，使能之前先config  */
    {"cfg_psd",                 wal_hipriv_cfg_psd},                  /* 使能PSD采集: hipriv "Hisilicon0 cfg_psd" */
#endif
#ifdef _PRE_WLAN_FEATURE_CSI
    {"set_csi",                 wal_hipriv_set_csi},                  /* 使能CSI上报: hipriv "Hisilicon0 set_csi xx xx xx xx xx xx(mac地址) ta_check csi_en" */
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    {"set_oma",                 wal_hipriv_set_oma},                   /* 设置Proxy STA的oma地址" */
    {"proxysta_switch",         wal_hipriv_proxysta_switch},           /* proxysta模块的开关的命令: hipriv "Hisilicon0 proxysta_switch 0 | 1"，该命令针对所有的VAP */
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
    {"set_opmode_notify",       wal_hipriv_set_opmode_notify},                /* 设置VAP工作模式通知: hipriv "vap0 set_opmode_notify 0/1"  0-不支持; 1-支持 */
    {"get_user_nssbw",          wal_hipriv_get_user_nssbw},                   /* 设置添加用户的配置命令: hipriv "vap0 get_user_nssbw xx xx xx xx xx xx(mac地址) "  该命令针对某一个VAP */
#endif

#ifdef _PRE_WLAN_FEATURE_M2S
    {"set_m2s_switch",          wal_hipriv_set_m2s_switch},     /* mimo和siso切换: hipriv "Hisilicon0 set_m2s_switch  0/1/2/3/4/5...(参数查询/配置模式/resv/软切换或硬切换测试模式) */
#endif

#ifdef _PRE_WLAN_DFT_REG
    {"dump_reg",                 wal_hipriv_dump_reg},                 /* 设置需要读取的寄存器，hipriv "Hisilicon0 dump_reg phy/mac/soc/abb/rf (subtype-数字) (flag 0/1)"*/
    {"dump_reg_evt",             wal_hipriv_dump_reg_evt},             /* 设置触发寄存器读取的事件和事件跳数，hipriv "Hisilicon0 dump_reg_evt tx/rx/tbtt/prd (tick-数字)"*/
    {"dump_reg_addr",            wal_hipriv_dump_reg_addr},            /* 显示寄存器读取相关调试信息 */

    {"dump_reg_info",            wal_hipriv_dump_reg_info},            /* 显示寄存器读取相关调试信息 */
#endif

    {"set_vap_nss",              wal_hipriv_set_vap_nss},               /* 设置VAP的空间流个数:hipriv "vap0 set_vap_nss <value>" */

#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)

    {"blacklist_add",           wal_hipriv_blacklist_add},          /* 1 */
    {"blacklist_del",           wal_hipriv_blacklist_del},          /* 2 */
    {"blacklist_mode",          wal_hipriv_set_blacklist_mode},     /* 3 */
    {"blacklist_show",          wal_hipriv_blacklist_show},         /* 4 wal_config_blacklist_show */
    {"abl_on",                  wal_hipriv_set_abl_on},             /* 5 */
    {"abl_aging",               wal_hipriv_set_abl_aging},          /* 6 */
    {"abl_threshold",           wal_hipriv_set_abl_threshold},      /* 7 */
    {"abl_reset",               wal_hipriv_set_abl_reset},          /* 8 wal_config_set_autoblacklist_reset_time */
#endif
#ifdef _PRE_WLAN_FEATURE_ISOLATION
    {"isolation_mode",          wal_hipriv_set_isolation_mode},     /* 9 */
    {"isolation_type",          wal_hipriv_set_isolation_type},     /* 10 */
    {"isolation_fwd",           wal_hipriv_set_isolation_fwd},      /* 11 */
    {"isolation_clear",         wal_hipriv_set_isolation_clear},    /* 12 wal_config_set_isolation_clear */
    {"isolation_show",          wal_hipriv_set_isolation_show},     /* 13 wal_config_isolation_show */

#endif
#if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)
    {"m2u_snoop_on",               wal_hipriv_m2u_snoop_on},               /* 开启或关闭snoop开关功能 hipriv "vap0 m2u_snoop_on 0\1" */
    {"m2u_add_deny_table",         wal_hipriv_m2u_add_deny_table},         /* 增加组播组黑名单 hipriv "vap0 m2u_add_deny_table 224.1.1.1" */
    {"m2u_del_deny_table",         wal_hipriv_m2u_del_deny_table},         /* 删除组播组黑名单 hipriv "vap0 m2u_del_deny_table 224.1.1.1" */
    {"m2u_cfg_deny_table",         wal_hipriv_m2u_cfg_deny_table},         /* 清空组播组黑名单 hipriv "vap0 m2u_cfg_deny_table 1 0" */
    {"m2u_show_snoop_table",       wal_hipriv_m2u_show_snoop_table},       /* 打印组播组 hipriv "vap0 m2u_show_snoop_table 1" */
    {"m2u_igmp_pkt_xmit",          wal_hipriv_igmp_packet_xmit},           /* 向目标STA/AP发送数据帧: hipriv "vap0 m2u_igmp_pkt_xmit (tid_no) (报文个数) (报文长度) (RA MAC)" */
    {"m2u_tid_set",                wal_hipriv_m2u_tid_set},                /*设置组播转单播报文使用的tid: hipriv "vap0 m2u_tid_set 0~7"*/
    {"m2u_show_snoop_deny_table",  wal_hipriv_m2u_show_snoop_deny_table},  /* 打印组播黑名单 hipriv "vap0 m2u_show_snoop_deny_table 1" */
#endif
#ifdef _PRE_WLAN_FEATURE_HERA_MCAST
	{"m2u_adaptive_on",            wal_hipriv_m2u_adaptive_on},            /* 开启或关闭配网模式识别开关 hipriv "vap0 m2u_adaptive_on 0\1" */
	{"m2u_frequency_on",           wal_hipriv_m2u_frequency_on},           /* 开启或关闭异频组播转发开关 hipriv "vap0 m2u_frequency_on 0\1" */
#endif

#ifdef _PRE_WLAN_FEATURE_QOS_ENHANCE
    {"qos_enhance_on",           wal_hipriv_qos_enhance_on},           /* 开启或关闭异频组播转发开关 hipriv "vap0 qos_enhance_on 0\1" */
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
    {"proxyarp_on",            wal_hipriv_proxyarp_on},           /* ?????proxyarp???? hipriv "vap0 proxyarp_on 0\1" */
#ifdef _PRE_DEBUG_MODE
    {"proxyarp_info",          wal_hipriv_proxyarp_info},         /* ??proxyarp???? hipriv "vap0 proxyarp_info 0\1" */
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif/* #ifdef _PRE_WLAN_FEATURE_PROXY_ARP */

#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE
    {"smps_info",              wal_hipriv_get_smps_info},         /* ??proxyarp???? hipriv "vap0 smps_info 0\1" */
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif/* #ifdef _PRE_WLAN_FEATURE_SMPS */

    {"vap_classify_en",         wal_hipriv_vap_classify_en},        /* device级别配置命令 设置基于vap的业务分类是否使能 hipriv "Hisilicon0 vap_classify_en 0/1" */
    {"vap_classify_tid",        wal_hipriv_vap_classify_tid},       /* 设置vap的流等级 hipriv "vap0 classify_tid 0~7" */

#ifdef _PRE_DEBUG_MODE
    {"scan_test",               wal_hipriv_scan_test},              /* 扫描模块测试命令 hipriv "Hisilicon0 scan_test param1 param2" param1取值'2g' '5g' 'all' 1~14, 36~196; param2取值对应wlan_channel_bandwidth_enum_uint8 */
#endif
#ifdef _PRE_WLAN_PROFLING_MIPS
    {"set_mips",             wal_hipriv_set_mips},            /* 设置某流程的MIPS统计开关，sh hipriv.sh "Hisilicon0 set_mips wal_mips_param_enum 0|1" */
    {"show_mips",            wal_hipriv_show_mips},           /* 打印某流程的MIPS统计结果，sh hipriv.sh "Hisilicon0 show_mips wal_mips_param_enum" */
#endif
    {"txpower",         wal_hipriv_set_txpower},                 /* 设置最大发送功率，要求功率值按照扩大10倍来输入，例如最大功率要限制为20，输入200 */
#if  (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44))
    {"essid",           wal_ioctl_set_essid_etc},                   /* 设置AP ssid */
    {"bintval",         wal_ioctl_set_beacon_interval},         /* 设置AP beacon 周期 */
    {"up",              wal_hipriv_start_vap},
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44)) */

#ifdef _PRE_WLAN_FEATURE_11D
    {"set_rd_by_ie_switch",      wal_hipriv_set_rd_by_ie_switch_etc},   /*设置是否根据关联ap更新国家码信息 hipriv "Hisilicon0 set_rd_by_ie_switch 0/1"*/
#endif
#ifdef  _PRE_WLAN_FEATURE_P2P
#ifdef _PRE_WLAN_CHIP_TEST
    {"p2p_ps",                  wal_hipriv_set_p2p_ps},         /* 设置P2P 节能 sh hipriv.sh "vap0 p2p_ps noa/ops params */
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
                                                                /* sh hipriv.sh "vap0 p2p_ps ops 0/1(0不使能，1使能) [0~255] 设置OPS 节能下ct_window 参数 */
                                                                /* sh hipriv.sh "vap0 p2p_ps noa start_time duration interval count 设置NOA 节能参数 */
                                                                /* sh hipriv.sh "vap0 p2p_ps statistics 0/1(0 清空统计，1查看统计) P2P 中断统计 */
#ifdef _PRE_DEBUG_MODE
    {"p2p_test",                wal_hipriv_p2p_test},
#endif
#endif  /* _PRE_WLAN_FEATURE_P2P */

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    {"resume_rx_intr_fifo",     wal_hipriv_resume_rx_intr_fifo},    /* 使能恢复rx intr fifo命令，默认不是能 hipriv "Hisilicon0 resume_rxintr_fifo 0|1" 1使能 */
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    {"ampdu_mmss",              wal_hipriv_set_ampdu_mmss},         /* 设置AMPDU MMSS : sh hipriv.sh "vap0 ampdu_mmss 0~7" */
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
    {"arp_offload_enable",      wal_hipriv_arp_offload_enable},    /* ARP/ND处理下移和广播/组播过滤开关:sh hipriv.sh "wlan0 arp_offload_enable  0/1(0关闭，1打开)" */
    {"show_arpoffload_info",    wal_hipriv_show_arpoffload_info},          /* 显示Device侧记录的IP地址:sh hipriv.sh "wlan0 show_ip_addr" */
#endif

#ifdef _PRE_WLAN_TCP_OPT
    {"get_tcp_ack_stream_info",                  wal_hipriv_get_tcp_ack_stream_info},         /* 显示TCP ACK 过滤统计值 sh hipriv.sh "vap0 get_tx_ack_stream_info*/
    {"tcp_tx_ack_opt_enable",                  wal_hipriv_tcp_tx_ack_opt_enable},         /*设置发送TCP ACK优化使能  sh hipriv.sh "vap0 tcp_tx_ack_opt_enable 0 | 1*/
    {"tcp_rx_ack_opt_enable",                  wal_hipriv_tcp_rx_ack_opt_enable},         /* 设置接收TCP ACK优化使能 sh hipriv.sh "vap0 tcp_rx_ack_opt_enable 0 | 1*/
    {"tcp_tx_ack_opt_limit",                  wal_hipriv_tcp_tx_ack_limit},         /* 设置发送TCP ACK LIMIT sh hipriv.sh "vap0 tcp_tx_ack_opt_limit X*/
    {"tcp_rx_ack_opt_limit",                  wal_hipriv_tcp_rx_ack_limit},         /* 设置接收TCP ACKLIMIT  sh hipriv.sh "vap0 tcp_tx_ack_opt_limit X*/

#endif

#ifdef _PRE_WLAN_FEATURE_WAPI
#ifdef _PRE_WAPI_DEBUG
    {"wapi_info",                             wal_hipriv_show_wapi_info},   /*wapi hipriv "vap0 wal_hipriv_show_wapi_info "*/
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif /* #ifdef _PRE_WLAN_FEATURE_WAPI */


#ifdef _PRE_WLAN_DFT_STAT
    {"performance_log_debug",          wal_hipriv_performance_log_switch},        /* 设置性能打印控制开关 sh hipriv.sh "wlan0 performance_log_debug X Y,*/
                                                                                    /*其中X是打印点，见oal_performance_log_switch_enum定义，Y是使能开关,0关闭，1打开。*/                                                                                                                                                        /*X=255时，配置所有的打印开关*/
                                                                                    /*使用说明:                                                     */
                                                                                    /*sh hipriv.sh "wlan0 performance_log_debug 0 0 :关闭聚合打印   */
                                                                                    /*sh hipriv.sh "wlan0 performance_log_debug 0 1 :打开聚合打印   */
                                                                                    /*sh hipriv.sh "wlan0 performance_log_debug 1 0 :打印性能统计   */
                                                                                    /*sh hipriv.sh "wlan0 performance_log_debug 1 1 :聚合统计清0    */
                                                                                    /*sh hipriv.sh "wlan0 performance_log_debug 255 0 :清除所有控制开关*/
                                                                                    /*sh hipriv.sh "wlan0 performance_log_debug 255 1 :设置所有控制开关*/

#endif
#ifdef _PRE_WLAN_FEATURE_ROAM
    {"roam_enable",      wal_hipriv_roam_enable},   /* 设置漫游开关 */
    {"roam_org",         wal_hipriv_roam_org},      /* 设置漫游正交 */
    {"roam_band",        wal_hipriv_roam_band},     /* 设置漫游频段 */
    {"roam_start",       wal_hipriv_roam_start},    /* 漫游测试命令 sh hipriv.sh "wlan0 roam_start 0|1|2|3|4 0/1" 0或者参数缺失表示漫游前不扫描, 1|2|3|4表示扫描+漫游*/
                                                    /* 第二个参数0或者参数缺失表示禁止漫游到自己, 1表示漫游到自己*/
    {"roam_info",        wal_hipriv_roam_info},     /* 漫游信息打印 */
#endif  //_PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    {"2040bss_enable",   wal_hipriv_enable_2040bss}, /* 设置20/40 bss使能: hipriv "Hisilicon0 2040bss_enable 0|1" 0表示20/40 bss判断关闭，1表示使能 */
#endif

#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
    {"dyn_cali",        wal_hipriv_dyn_cali_cfg},                   /*  动态校准参数配置 sh hipriv "wlan0 dyn_cali   "*/
#endif
#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN
    {"set_tx_classify_switch",        wal_hipriv_set_tx_classify_switch},       /* 设置业务识别功能开关: sh hipriv.sh "p2p-p2p0-0 set_tx_classify_switch 1/0"(1打开，0关闭，开关默认开启) */
#endif  /* _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN */

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    {"set_txrx_chain",   wal_hipriv_set_txrx_chain},  /* 设置收发通道: hipriv "wlan0 set_txrx_chain 0/1/2" 第二个参数 0:通道0, 1:通道1, 2:双通道 */
    {"set_2g_path",      wal_hipriv_set_2g_txrx_path},/* 设置2g收发通路: hipriv "wlan0 set_2g_path 0/1" 参数 0:617通路, 1:420通路 */
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK_DEBUG
    {"fbt_set_mode",                   wal_hipriv_fbt_set_mode},              /*FBT 模式设置hipriv.sh "wlan0 fbt_set_mode 0|1"*/
    {"fbt_scan_list_clear",            wal_hipriv_fbt_scan_list_clear},       /*清除扫描列表hipriv.sh "wlan0 fbt_scan_list_clear"*/
    {"fbt_scan_specified_sta",         wal_hipriv_fbt_scan_specified_sta},    /*侦听指定用户hipriv.sh "wlan0 fbt_scan_specified_sta aa:bb:cc:dd:ee:ff"*/
    {"fbt_start_scan",                 wal_hipriv_fbt_start_scan},            /*触发侦听hipriv.sh "wlan0 fbt_start_scan"*/
    {"fbt_print_scan_list",            wal_hipriv_fbt_print_scan_list},       /*打印侦听列表hipriv.sh "wlan0 fbt_print_scan_list"*/
    {"fbt_scan_enable",                wal_hipriv_fbt_scan_enable},           /*FBT 侦听开关设置hipriv.sh "wlan0 fbt_scan_enable 0|1"*/
    {"fbt_scan_interval",              wal_hipriv_fbt_scan_interval},         /*FBT 侦听时长设置hipriv.sh "wlan0 fbt_scan_interval [0,*]"*/
    {"fbt_scan_channel",               wal_hipriv_fbt_scan_channel},          /*FBT 侦听信道设置hipriv.sh "wlan0 fbt_scan_channel ** "*/
    {"fbt_scan_report_period",         wal_hipriv_fbt_scan_report_period},    /*FBT 侦听上报周期设置hipriv.sh "wlan0 fbt_scan_report_period [0,*]"*/
#endif

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    {"chip_check",      wal_hipriv_chip_check},                  /* 芯片自检 */
    {"cfg_cw_signal",   wal_hipriv_send_cw_signal},               /* 发送单音信号:         hipriv "wlan0 cfg_cw_signal   <value>" */
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    {"custom_info",     wal_hipriv_dev_customize_info},            /* 打印定制化信息 */
#endif
#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
    {"user_extend_enable",  wal_hipriv_user_extend_enable}, /* 芯片级用户扩展开关 */
#endif

#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
    {"packet_capture",  wal_hipriv_packet_capture},              /* 开启或关闭抓包功能: hipriv "Hisilicon0 packet_capture 0|1" */
#endif
#ifdef _PRE_WLAN_11K_STAT
    {"query_stat_info", wal_hipriv_query_stat_info},     /*打印统计信息*/

#endif
#ifdef _PRE_WLAN_ONLINE_DPD
    {"dpd_cfg",     wal_hipriv_dpd_cfg},            /* 打印定制化信息 */
    {"dpd_start",     wal_hipriv_dpd_start},            /* 打印定制化信息 */
#endif

#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
    {"bsd",  wal_hipriv_bsd},              /* bsd特性配置:
                                                开关:    hipriv "Hisilicon0 bsd sw 0|1"
                                                阀值配置:hipriv "vap[x] bsd thr_th []"  //@todo...
                                            */
#endif
#ifdef _PRE_WLAN_FEATURE_11V
    {"11v_cfg_wl_mgmt",  wal_hipriv_11v_cfg_wl_mgmt},                 /* 11v wireless mgmt特性配置 sh hipriv.sh "vap0 11v_cfg_wl_mgmt 0|1"
                                                                        0:关闭11V特性；1:打开11V特性*/
#endif
#if (defined(_PRE_WLAN_FEATURE_11V) && defined(_PRE_DEBUG_MODE)) || defined(_PRE_WLAN_FEATURE_11V_ENABLE)
    {"11v_cfg_bsst",  wal_hipriv_11v_cfg_bsst},                       /* 11v bss transition特性配置 sh hipriv.sh "vap0 11v_cfg_bsst 0|1"
                                                                        0:关闭11V特性；1:打开11V特性*/
    {"11v_tx_query",  wal_hipriv_11v_tx_query},                     /* 11v特性配置:  触发sta发送11v Query帧: hipriv "vap[x] 11v_tx_query [mac-addr]"   */
#endif
#if defined(_PRE_WLAN_FEATURE_11V) && defined(_PRE_DEBUG_MODE)
    {"11v_tx_request",  wal_hipriv_11v_tx_request},                 /* 11v特性配置:  触发ap发送11v Request帧: hipriv "vap[x] 11v_tx_request [mac-addr]" */
#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    {"send_radio_meas_req", wal_hipriv_send_radio_meas_req},     /*radio measurement request*/
    {"11k_switch",          wal_hipriv_set_11k_switch},          /*11k flag*/
#endif
#ifdef _PRE_WLAN_FEATURE_DBDC
    {"dbdc_debug",          wal_hipriv_set_dbdc_debug_switch},    /* DBDC特性开关，sh hipriv.sh "wlan0 dbdc_debug [change_hal_dev 0|1] " */
#endif

#ifdef _PRE_MEM_TRACE
    {"mem_trace_show",          wal_hipriv_mem_trace_info_show},
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {"pm_debug",          wal_hipriv_set_pm_debug_switch},    /* 低功耗debug命令 */
#endif
};

oal_uint32 wal_hipriv_get_debug_cmd_size_etc(oal_void)
{
    return OAL_ARRAY_SIZE(g_ast_hipriv_cmd_debug_etc);
}

#endif
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

