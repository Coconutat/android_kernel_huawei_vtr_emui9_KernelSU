


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oam_main.h"
#include "oam_log.h"
#include "oam_event.h"
#include "oam_alarm.h"
#include "oam_trace.h"
#include "oam_statistics.h"
#if (!defined(_PRE_PRODUCT_ID_HI110X_DEV))
#include "oam_config.h"
#endif
#include "oam_linux_netlink.h"
#include "oam_ext_if.h"
#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
#include "oal_hcc_host_if.h"
#endif
#include "plat_pm_wlan.h"

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
/* OAM模块统一使用的全局操作变量上下文，包括OAM其他子模块全局上下文 */
oam_mng_ctx_stru    g_st_oam_mng_ctx_etc;

/* 打印类型函数定义 */
OAL_STATIC oal_print_func g_pa_oam_print_type_func[OAM_OUTPUT_TYPE_BUTT]=
{
    oam_print_to_console_etc,   /* OAM_OUTPUT_TYPE_CONSOLE 控制台输出 */
    oam_print_to_file_etc,      /* OAM_OUTPUT_TYPE_FS 写到文件系统 */
    oam_print_to_sdt_etc,       /* OAM_OUTPUT_TYPE_SDT 输出到SDT,上报字符串不宜大于2048 */
};

/* 用于和SDT工具交互的全局变量 */
oam_sdt_func_hook_stru          g_st_oam_sdt_func_hook_etc;
oam_wal_func_hook_stru          g_st_oam_wal_func_hook_etc;
oam_sdt_stat_info_stru          g_st_sdt_stat_info_etc;

oal_uint8 g_auc_bcast_addr_etc[WLAN_MAC_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#ifdef _PRE_DEBUG_MODE  /* 调试特性默认开关状态 */
oal_uint32          g_aul_debug_feature_switch_etc[OAM_DEBUG_TYPE_BUTT] =
{
    OAL_SWITCH_OFF,   /* OAM_DEBUG_TYPE_ECHO_REG */
};
#endif

/* 特性列表 */
oam_software_feature_stru   gst_oam_feature_list_etc[OAM_SOFTWARE_FEATURE_BUTT] =
{
    /*特性宏ID                  特性名缩写*/
    /* 0 */
    {OAM_SF_SCAN,               "scan"},
    {OAM_SF_AUTH,               "auth"},
    {OAM_SF_ASSOC,              "assoc"},
    {OAM_SF_FRAME_FILTER,       "ff"},
    {OAM_SF_WMM,                "wmm"},

    /* 5 */
    {OAM_SF_DFS,                "dfs"},
    {OAM_SF_NETWORK_MEASURE,    "nm"},
    {OAM_SF_ENTERPRISE_VO,      "ev"},
    {OAM_SF_HOTSPOTROAM,        "roam"},
    {OAM_SF_NETWROK_ANNOUNCE,   "11u"},

    /* 10 */
    {OAM_SF_NETWORK_MGMT,       "11k"},
    {OAM_SF_NETWORK_PWS,        "pws"},
    {OAM_SF_PROXYARP,           "proxyarp"},
    {OAM_SF_TDLS,               "tdls"},
    {OAM_SF_CALIBRATE,          "cali"},

    /* 15 */
    {OAM_SF_EQUIP_TEST,         "equip"},
    {OAM_SF_CRYPTO,             "crypto"},
    {OAM_SF_WPA,                "wpa"},
    {OAM_SF_WEP,                "wep"},
    {OAM_SF_WPS,                "wps"},

    /* 20 */
    {OAM_SF_PMF,                "pmf"},
    {OAM_SF_WAPI,               "wapi"},
    {OAM_SF_BA,                 "ba"},
    {OAM_SF_AMPDU,              "ampdu"},
    {OAM_SF_AMSDU,              "amsdu"},

    /* 25 */
    {OAM_SF_STABILITY,          "dfr"},
    {OAM_SF_TCP_OPT,            "tcp"},
    {OAM_SF_ACS,                "acs"},
    {OAM_SF_AUTORATE,           "autorate"},
    {OAM_SF_TXBF,               "txbf"},

    /* 30 */
    {OAM_SF_DYN_RECV,           "weak"},
    {OAM_SF_VIVO,               "vivo"},
    {OAM_SF_MULTI_USER,         "muser"},
    {OAM_SF_MULTI_TRAFFIC,      "mtraff"},
    {OAM_SF_ANTI_INTF,          "anti_intf"},

    /* 35 */
    {OAM_SF_EDCA,               "edca"},
    {OAM_SF_SMART_ANTENNA,      "ani"},
    {OAM_SF_TPC,                "tpc"},
    {OAM_SF_TX_CHAIN,           "txchain"},
    {OAM_SF_RSSI,               "rssi"},

    /* 40 */
    {OAM_SF_WOW,                "wow"},
    {OAM_SF_GREEN_AP,           "green"},
    {OAM_SF_PWR,                "pwr"},
    {OAM_SF_SMPS,               "smps"},
    {OAM_SF_TXOP,               "txop"},

    /* 45 */
    {OAM_SF_WIFI_BEACON,        "beacon"},
    {OAM_SF_KA_AP,              "alive"},
    {OAM_SF_MULTI_VAP,          "mvap"},
    {OAM_SF_2040,               "2040"},
    {OAM_SF_DBAC,               "dbac"},

    /* 50 */
    {OAM_SF_PROXYSTA,           "proxysta"},
    {OAM_SF_UM,                 "um"},
    {OAM_SF_P2P,                "p2p"},
    {OAM_SF_M2U,                "m2u"},
    {OAM_SF_IRQ,                "irq"},

    /* 55 */
    {OAM_SF_TX,                 "tx"},
    {OAM_SF_RX,                 "rx"},
    {OAM_SF_DUG_COEX,           "dugcoex"},
    {OAM_SF_CFG,                "cfg"},
    {OAM_SF_FRW,                "frw"},

    /* 60 */
    {OAM_SF_KEEPALIVE,          "keepalive"},
    {OAM_SF_COEX,               "coex"},
    {OAM_SF_HS20,               "hs20"},
    {OAM_SF_MWO_DET,            "mwodet"},
    {OAM_SF_CCA_OPT,            "cca"},

    /* 65 */
    {OAM_SF_ROAM,               "roam"},
    {OAM_SF_DFT,                "dft"},
    {OAM_SF_DFR,                "dfr"},
    {OAM_SF_RRM,                "rrm"},
    {OAM_SF_VOWIFI,             "vowifi"},

    /* 70 */
    {OAM_SF_OPMODE,             "opmode"},
    {OAM_SF_M2S,                "m2s"},
    {OAM_SF_DBDC,               "dbdc"},
    {OAM_SF_HILINK,             "hilink"},
    {OAM_SF_WDS,                "wds"},

    /* 75 */
    {OAM_SF_WMMAC,              "wmmac"},
    {OAM_SF_USER_EXTEND,        "ue"},
    {OAM_SF_PKT_CAP,            "pktcap"},
    {OAM_SF_SOFT_CRYPTO,        "crypto"},
    {OAM_SF_CAR,                "car"},                /* 限速特性  */
    {OAM_SF_11AX,               "11ax"},
    {OAM_SF_CSA,                "csa"},
    {OAM_SF_QOS,                "qos"},
    {OAM_SF_RESERVE4,           "rev4"},
    {OAM_SF_RESERVE5,           "rev5"},
    {OAM_SF_RESERVE6,           "rev6"},
    {OAM_SF_RESERVE7,           "rev7"},
    {OAM_SF_RESERVE8,           "rev8"},
    {OAM_SF_RESERVE9,           "rev9"},
    {OAM_SF_RESERVE10,          "rev10"},
    {OAM_SF_CONN,               "conn"},
    {OAM_SF_CHAN,               "chnn"},
    {OAM_SF_CUSTOM,             "custom"},

    {OAM_SF_ANY,                "any"},
};

/*****************************************************************************
  3 函数实现
*****************************************************************************/

oal_uint32  oam_print_etc(oal_int8 *pc_string)
{
    oam_output_type_enum_uint8 en_output_type;
    oal_uint32                 ul_rslt;

    ul_rslt = oam_get_output_type_etc(&en_output_type);
    if (OAL_SUCC != ul_rslt)
    {
        return ul_rslt;
    }

    ul_rslt = g_pa_oam_print_type_func[en_output_type](pc_string);
    if (OAL_SUCC != ul_rslt)
    {
        return ul_rslt;
    }

    return OAL_SUCC;
}


oal_uint32 oam_print_to_console_etc(oal_int8 *pc_string)
{
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_string))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_IO_PRINT("%s\r\n", pc_string);

    return OAL_SUCC;
}


oal_uint32   oam_print_to_file_etc(oal_int8 *pc_string)
{
#ifdef _PRE_WIFI_DMT

    oal_file_stru            *f_file_ret;                                 /* 用于保存写文件后的返回值 */
    oal_file_stru            *f_event_file;
    oal_int32                 l_rslt;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_string))
    {
        OAM_IO_PRINTK("null param. \r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    f_event_file = oal_file_open_append(g_st_oam_mng_ctx_etc.ac_file_path);
    if (OAL_UNLIKELY(OAL_FILE_FAIL == f_event_file))
    {
        OAM_IO_PRINTK("open file failed. \r\n");
        return OAL_ERR_CODE_OPEN_FILE_FAIL;
    }

    f_file_ret = oal_file_write(f_event_file, pc_string, (OAL_STRLEN(pc_string) + 1));
    if (OAL_FILE_FAIL == f_file_ret)
    {
        l_rslt = oal_file_close(f_event_file);
        if (0 != l_rslt)
        {
            OAM_IO_PRINTK("close file failed. \r\n");
            return OAL_ERR_CODE_CLOSE_FILE_FAIL;
        }

        OAM_IO_PRINTK("write file failed. \r\n");
        return OAL_ERR_CODE_WRITE_FILE_FAIL;
    }

    l_rslt = oal_file_close(f_event_file);

    if (0 != l_rslt)
    {
        OAM_IO_PRINTK("close file failed. \r\n");
        return OAL_ERR_CODE_CLOSE_FILE_FAIL;
    }
#endif
    return OAL_SUCC;
}


oal_uint32 oam_print_to_sdt_etc(oal_int8 *pc_string)
{
    oal_netbuf_stru                *pst_skb;
    oal_uint32                      ul_ret       = OAL_SUCC;
    oal_uint16                      us_strlen;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_string))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_UNLIKELY(OAL_PTR_NULL == g_st_oam_sdt_func_hook_etc.p_sdt_report_data_func))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 上报sdt字符串以'0'结束 */
    us_strlen = (oal_uint16)OAL_STRLEN(pc_string);

    us_strlen = (us_strlen > OAM_REPORT_MAX_STRING_LEN) ? OAM_REPORT_MAX_STRING_LEN : us_strlen;

    pst_skb = oam_alloc_data2sdt_etc(us_strlen);
    if (OAL_PTR_NULL == pst_skb)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* COPY打印的内容 */
    oal_memset(oal_netbuf_data(pst_skb), 0, us_strlen);
    oal_memcopy(oal_netbuf_data(pst_skb), pc_string, (oal_uint32)us_strlen);

    /* 下发至sdt接收队列，若队列满则串口输出 */
    ul_ret = oam_report_data2sdt_etc(pst_skb, OAM_DATA_TYPE_STRING, OAM_PRIMID_TYPE_OUTPUT_CONTENT);

    return ul_ret;
}


oal_uint32 oam_upload_log_to_sdt_etc(oal_int8 *pc_string)
{
    oal_netbuf_stru        *pst_skb;
    oal_uint32              ul_ret      = OAL_SUCC;

    if (OAL_UNLIKELY(OAL_PTR_NULL == g_st_oam_sdt_func_hook_etc.p_sdt_report_data_func))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_PTR_NULL == pc_string)
    {
        OAL_IO_PRINT("oam_upload_log_to_sdt_etc::pc_string is null!\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_skb = oam_alloc_data2sdt_etc(OAL_SIZEOF(oam_log_info_stru));
    if (OAL_PTR_NULL == pst_skb)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* COPY打印的内容 */
    oal_memcopy(oal_netbuf_data(pst_skb), pc_string, OAL_SIZEOF(oam_log_info_stru));

    /* 下发至sdt接收队列，若队列满则串口输出 */
    ul_ret = oam_report_data2sdt_etc(pst_skb, OAM_DATA_TYPE_LOG, OAM_PRIMID_TYPE_OUTPUT_CONTENT);

    return ul_ret;
}

oal_uint32 oam_upload_device_log_to_sdt_etc(oal_uint8 *pc_string, oal_uint16 len)
{
    oal_netbuf_stru        *pst_skb;
    oal_uint32              ul_ret      = OAL_SUCC;

    if (OAL_PTR_NULL == pc_string)
    {
        OAL_IO_PRINT("oam_upload_log_to_sdt_etc::pc_string is null!\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_skb = oam_alloc_data2sdt_etc(len);
    if (OAL_PTR_NULL == pst_skb)
    {
        OAL_IO_PRINT("alloc netbuf stru failed!\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* COPY打印的内容 */
    oal_memcopy(oal_netbuf_data(pst_skb), pc_string, len);

    /* 下发至sdt接收队列，若队列满则串口输出 */
    ul_ret = oam_report_data2sdt_etc(pst_skb, OAM_DATA_TYPE_DEVICE_LOG, OAM_PRIMID_TYPE_OUTPUT_CONTENT);

    return ul_ret;
}


oal_uint32 oam_send_device_data2sdt_etc(oal_uint8* pc_string, oal_uint16 len)
{
    oal_uint32 ul_ret = OAL_SUCC;
    if (NULL == pc_string)
    {
        return OAL_EFAIL;
    }

    ul_ret = oam_upload_device_log_to_sdt_etc(pc_string, len);

    return ul_ret;
}

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
oal_int32 oam_rx_post_action_function_etc(struct hcc_handler * hcc, oal_uint8 stype,
                                             hcc_netbuf_stru* pst_hcc_netbuf, oal_uint8 *pst_context)
{
    oal_uint8   *puc_data;
    OAL_REFERENCE(pst_context);
    OAL_REFERENCE(hcc);

    if(OAL_WARN_ON(NULL == pst_hcc_netbuf))
    {
         oal_print_hi11xx_log(HI11XX_LOG_ERR,"%s error: pst_hcc_netbuf is null",__FUNCTION__);
         return OAL_FAIL;
    };

    puc_data = oal_netbuf_data(pst_hcc_netbuf->pst_netbuf);
    if((DUMP_REG == stype) || (DUMP_MEM == stype))
    {
       //AL_IO_PRINT("receive dump msg  = %d\n", stype);
        exception_bcpu_dump_recv_etc(puc_data,pst_hcc_netbuf->pst_netbuf);
        oal_netbuf_free(pst_hcc_netbuf->pst_netbuf);
        return OAL_SUCC;
    }

    /* 调用OAM接口*/
    oam_send_device_data2sdt_etc(puc_data, (oal_uint16)pst_hcc_netbuf->len);

    oal_netbuf_free(pst_hcc_netbuf->pst_netbuf);
    return OAL_SUCC;
}
oal_int32 chr_rx_post_action_function_etc(struct hcc_handler * hcc, oal_uint8 stype,
                                             hcc_netbuf_stru* pst_hcc_netbuf, oal_uint8 *pst_context)
{
    oal_uint8 *puc_data;
    OAL_REFERENCE(pst_context);
    OAL_REFERENCE(hcc);

    if(OAL_WARN_ON(NULL == pst_hcc_netbuf))
    {
         oal_print_hi11xx_log(HI11XX_LOG_ERR,"%s error: pst_hcc_netbuf is null",__FUNCTION__);
         return OAL_FAIL;
    };

#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
    if(OAL_TRUE==wlan_pm_wkup_src_debug_get())
    {
        OAL_IO_PRINT("wifi_wake_src:rx chr error!\n");
        wlan_pm_wkup_src_debug_set(OAL_FALSE);
    }
#endif

    puc_data = oal_netbuf_data(pst_hcc_netbuf->pst_netbuf);
#ifdef _PRE_CONFIG_HW_CHR
    chr_dev_exception_callback_etc(puc_data, (oal_uint16)pst_hcc_netbuf->len);
#endif
    oal_netbuf_free(pst_hcc_netbuf->pst_netbuf);
    return OAL_SUCC;
}

#endif


oal_uint32  oam_get_output_type_etc(oam_output_type_enum_uint8 *pen_output_type)
{

    *pen_output_type = g_st_oam_mng_ctx_etc.en_output_type;

    return OAL_SUCC;
}


oal_uint32  oam_set_output_type_etc(oam_output_type_enum_uint8 en_output_type)
{
    if (en_output_type >= OAM_OUTPUT_TYPE_BUTT)
    {
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    g_st_oam_mng_ctx_etc.en_output_type = en_output_type;

    return OAL_SUCC;
}


oal_uint32  oam_set_file_path_etc(oal_int8 *pc_file_path, oal_uint32 ul_length)
{
#ifdef _PRE_WIFI_DMT
    oal_file_stru         *f_event_file;
	oal_uint8             *puc_file_path;

    if (OAL_PTR_NULL == pc_file_path)
	{
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (ul_length > OAM_FILE_PATH_LENGTH)
    {
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    puc_file_path = DmtStub_GetDebugFilePath();
    oal_memcopy(g_st_oam_mng_ctx_etc.ac_file_path, puc_file_path, strlen(puc_file_path));

    /* 以下操作是为了将上一次的日志文件清空 */
    f_event_file = oal_file_open_rw(g_st_oam_mng_ctx_etc.ac_file_path);
	if (OAL_FILE_FAIL == f_event_file)
    {
        return OAL_ERR_CODE_WRITE_FILE_FAIL;
    }

    if (0 != oal_file_close(f_event_file))
    {
        return OAL_ERR_CODE_CLOSE_FILE_FAIL;
    }

#else

    if (OAL_PTR_NULL == pc_file_path)
	{
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (ul_length > OAM_FILE_PATH_LENGTH)
    {
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    oal_memcopy(g_st_oam_mng_ctx_etc.ac_file_path, pc_file_path, ul_length);

#endif

    return OAL_SUCC;
}


oal_void  oam_dump_buff_by_hex_etc(oal_uint8 *puc_buff, oal_int32 l_len, oal_int32 l_num)
{
    oal_int32       l_loop;

    for (l_loop = 0; l_loop < l_len; l_loop++)
    {
        OAL_IO_PRINT("%02x ", puc_buff[l_loop]);

        if (0 == (l_loop + 1) % l_num)
        {
            OAL_IO_PRINT("\n");
        }
    }

    OAL_IO_PRINT("\n");
}


OAL_STATIC oal_void oam_drv_func_hook_init(oal_void)
{
    /* sdt侧对外钩子函数初始化 */
    g_st_oam_sdt_func_hook_etc.p_sdt_report_data_func       = OAL_PTR_NULL;
    g_st_oam_sdt_func_hook_etc.p_sdt_get_wq_len_func        = OAL_PTR_NULL;

    /* wal侧对外钩子函数初始化 */
    g_st_oam_wal_func_hook_etc.p_wal_recv_cfg_data_func     = OAL_PTR_NULL;
    g_st_oam_wal_func_hook_etc.p_wal_recv_mem_data_func     = OAL_PTR_NULL;
    g_st_oam_wal_func_hook_etc.p_wal_recv_reg_data_func     = OAL_PTR_NULL;
    g_st_oam_wal_func_hook_etc.p_wal_recv_global_var_func   = OAL_PTR_NULL;
}


oal_void oam_sdt_func_fook_register_etc(oam_sdt_func_hook_stru *pfun_st_oam_sdt_hook)
{
    g_st_oam_sdt_func_hook_etc.p_sdt_report_data_func = pfun_st_oam_sdt_hook->p_sdt_report_data_func;
    g_st_oam_sdt_func_hook_etc.p_sdt_get_wq_len_func  = pfun_st_oam_sdt_hook->p_sdt_get_wq_len_func;
}


oal_void oam_wal_func_fook_register_etc(oam_wal_func_hook_stru *pfun_st_oam_wal_hook)
{
    g_st_oam_wal_func_hook_etc.p_wal_recv_cfg_data_func     = pfun_st_oam_wal_hook->p_wal_recv_cfg_data_func;
    g_st_oam_wal_func_hook_etc.p_wal_recv_mem_data_func     = pfun_st_oam_wal_hook->p_wal_recv_mem_data_func;
    g_st_oam_wal_func_hook_etc.p_wal_recv_reg_data_func     = pfun_st_oam_wal_hook->p_wal_recv_reg_data_func;
    g_st_oam_wal_func_hook_etc.p_wal_recv_global_var_func   = pfun_st_oam_wal_hook->p_wal_recv_global_var_func;
#if defined(_PRE_WLAN_FEATURE_DATA_SAMPLE) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)
    g_st_oam_wal_func_hook_etc.p_wal_recv_sample_data_func  = pfun_st_oam_wal_hook->p_wal_recv_sample_data_func;
#endif
#ifdef _PRE_WLAN_RF_AUTOCALI
    g_st_oam_wal_func_hook_etc.p_wal_recv_autocali_data_func  = pfun_st_oam_wal_hook->p_wal_recv_autocali_data_func;
#endif
}


oal_uint32  oam_filter_data2sdt_etc(oam_data_type_enum_uint8 en_type)
{
    if (g_st_sdt_stat_info_etc.ul_wq_len < WLAN_SDT_MSG_FLT_HIGH_THD)
    {
        g_st_sdt_stat_info_etc.en_filter_switch = OAL_FALSE;
        return OAM_FLT_PASS;
    }
    else if ((g_st_sdt_stat_info_etc.ul_wq_len >= WLAN_SDT_MSG_FLT_HIGH_THD)
              && (g_st_sdt_stat_info_etc.ul_wq_len < WLAN_SDT_MSG_QUEUE_MAX_LEN))
    {
        /* 消息队列达到过滤上限，过滤非日志消息 */
        g_st_sdt_stat_info_etc.en_filter_switch = OAL_TRUE;
        return (((oal_uint8)OAM_DATA_TYPE_LOG) == en_type) ? OAM_FLT_PASS : OAM_FLT_DROP;/* [false alarm]:返回值为布尔值0或者1，不影响*/
    }

    /* 消息队列满全部过滤 */
    return OAM_FLT_DROP;
}


oal_netbuf_stru *oam_alloc_data2sdt_etc(oal_uint16  us_data_len)
{
    oal_netbuf_stru    *pst_netbuf = OAL_PTR_NULL;
#if ((_PRE_OS_VERSION_RAW != _PRE_OS_VERSION) && (_PRE_OS_VERSION_WIN32_RAW != _PRE_OS_VERSION))
    pst_netbuf = oal_mem_sdt_netbuf_alloc_etc(us_data_len+WLAN_SDT_SKB_RESERVE_LEN, OAL_TRUE);
    if (OAL_PTR_NULL == pst_netbuf)
    {
        return OAL_PTR_NULL;
    }

    oal_netbuf_reserve(pst_netbuf, WLAN_SDT_SKB_HEADROOM_LEN);

    oal_netbuf_put(pst_netbuf, us_data_len);
#endif
    return pst_netbuf;
}


oal_uint32 oam_report_data2sdt_etc(oal_netbuf_stru *pst_netbuf,
                               oam_data_type_enum_uint8 en_type,
                               oam_primid_type_enum_uint8 en_prim)
{
    /* 判断sdt发送消息队列是否已满，若满输出至串口 */
    if (OAL_LIKELY(OAL_PTR_NULL != g_st_oam_sdt_func_hook_etc.p_sdt_get_wq_len_func))
    {
        g_st_sdt_stat_info_etc.ul_wq_len = (oal_uint32)g_st_oam_sdt_func_hook_etc.p_sdt_get_wq_len_func();
    }

    if (OAM_FLT_PASS != oam_filter_data2sdt_etc(en_type))
    {
        OAM_SDT_STAT_INCR(ul_filter_cnt);
        oal_mem_sdt_netbuf_free_etc(pst_netbuf, OAL_TRUE);
        //oal_netbuf_free(pst_netbuf);

        /* Note: 目前上层函数仅仅使用该返回值打印warning信息而已*/
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
        if(0 != ((g_st_sdt_stat_info_etc.ul_filter_cnt) % OAM_DATA2SDT_FAIL_COUNT))
        {
            return OAL_SUCC;
        }
#endif
        return OAL_FAIL;
    }

    if (OAL_UNLIKELY(OAL_PTR_NULL == g_st_oam_sdt_func_hook_etc.p_sdt_report_data_func))
    {
        OAL_IO_PRINT("oam_report_data2sdt_etc p_sdt_report_data_func is NULL. \n");
        return OAL_FAIL;
    }

    g_st_oam_sdt_func_hook_etc.p_sdt_report_data_func(pst_netbuf, en_type, en_prim);

    return OAL_SUCC;
}


oal_void oam_sdt_func_fook_unregister_etc(oal_void)
{
    /* 函数指针赋值 */
    g_st_oam_sdt_func_hook_etc.p_sdt_report_data_func           = OAL_PTR_NULL;
    g_st_oam_sdt_func_hook_etc.p_sdt_get_wq_len_func            = OAL_PTR_NULL;
}


oal_void oam_wal_func_fook_unregister_etc(oal_void)
{
    /* 函数指针赋值 */
    g_st_oam_wal_func_hook_etc.p_wal_recv_cfg_data_func         = OAL_PTR_NULL;
    g_st_oam_wal_func_hook_etc.p_wal_recv_global_var_func       = OAL_PTR_NULL;
    g_st_oam_wal_func_hook_etc.p_wal_recv_mem_data_func         = OAL_PTR_NULL;
    g_st_oam_wal_func_hook_etc.p_wal_recv_reg_data_func         = OAL_PTR_NULL;
}

/*lint -save -e578 -e19 */
DEFINE_GET_BUILD_VERSION_FUNC(oam);
/*lint -restore*/


oal_int32  oam_main_init_etc(oal_void)
{
    oal_uint32 ul_rslt;

    OAL_RET_ON_MISMATCH(oam, -OAL_EFAIL);

#ifdef _PRE_WLAN_REPORT_PRODUCT_LOG
    oam_pdt_log_init();
#endif

    /* 初始化可维可测试FILE路径 */
    ul_rslt = oam_set_file_path_etc(WLAN_OAM_FILE_PATH, (OAL_STRLEN(WLAN_OAM_FILE_PATH) + 1));
    if (OAL_SUCC != ul_rslt)
    {
        OAL_IO_PRINT("oam_main_init_etc call oam_set_file_path_etc fail %d\n",ul_rslt);
        return -OAL_EFAIL;//lint !e527
    }

    /* 初始化可维可测输出方式 */
    ul_rslt = oam_set_output_type_etc(OAM_OUTPUT_TYPE_SDT);
    if (OAL_SUCC != ul_rslt)
    {
        OAL_IO_PRINT("oam_main_init_etc call oam_set_output_type_etc fail %d\n",ul_rslt);
        return -OAL_EFAIL;//lint !e527
    }

    /* 完成LOG模块的初始化操作 */
    ul_rslt = oam_log_init_etc();
    if (OAL_SUCC != ul_rslt)
    {
        OAL_IO_PRINT("oam_main_init_etc call oam_log_init_etc fail %d\n",ul_rslt);
        return -OAL_EFAIL;//lint !e527
    }

    /* 完成EVENT模块的初始化操作 */
    ul_rslt = oam_event_init_etc();
    if (OAL_SUCC != ul_rslt)
    {
        OAL_IO_PRINT("oam_main_init_etc call oam_event_init_etc fail %d\n",ul_rslt);
        return -OAL_EFAIL;//lint !e527
    }

    /* 初始化5115timer，用于代码中获取高精度时间戳 */
    oal_5115timer_init();

#ifdef _PRE_PROFILING_MODE
    /* 完成PROFILING模块的初始化操作 */
    ul_rslt = oam_profiling_init();
    if (OAL_SUCC != ul_rslt)
    {
        OAL_IO_PRINT("oam_main_init_etc call oam_profiling_init fail %d\n",ul_rslt);
        return -OAL_EFAIL;
    }
#endif

    /* 初始化oam模块的钩子函数 */
    oam_drv_func_hook_init();

    /* 统计模块初始化 */
    oam_statistics_init_etc();

/*TBD，待命令触发定制化初始操作*/
#if 0
    /* 从配置文件中获取配置信息，保存到OAM内部结构中 */
    for (ul_counter= 0; ul_counter < OM_MODULEID_BUTT; ul_counter++)
    {
        if (OAL_PTR_NULL != g_oam_customize_etc.customize_init[ul_counter])
        {
            g_oam_customize_etc.customize_init[ul_counter]();
        }
    }
#endif

#ifdef _PRE_WLAN_DFT_REG
    oam_reg_init_etc();
#endif

#if ((_PRE_OS_VERSION_RAW != _PRE_OS_VERSION) && (_PRE_OS_VERSION_WIN32_RAW != _PRE_OS_VERSION))
        /* ??netlink */
 #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    /* 所用的端口号在系统中已被占用，02无法使用这个接口，需要修改 TBD */
        ul_rslt = oam_netlink_kernel_create_etc();
        if (OAL_SUCC != ul_rslt)
        {
            OAL_IO_PRINT("oam_main_init_etc call oam_netlink_kernel_create_etc fail %d\n",ul_rslt);
            return -OAL_EFAIL;//lint !e527
        }
 #endif
#endif
#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
    hcc_rx_register_etc(hcc_get_110x_handler(),HCC_ACTION_TYPE_OAM,oam_rx_post_action_function_etc,NULL);
    hcc_rx_register_etc(hcc_get_110x_handler(),HCC_ACTION_TYPE_CHR,chr_rx_post_action_function_etc,NULL);
#endif
    return OAL_SUCC;
}


oal_void  oam_main_exit_etc(oal_void)
{

    /* 初始化5115timer，用于代码中获取高精度时间戳 */
    oal_5115timer_exit();

#ifdef _PRE_WLAN_DFT_REG
    oam_reg_exit_etc();
#endif
    /* 去注册全局变量读写总接口 */

#ifdef _PRE_WLAN_REPORT_PRODUCT_LOG
    oam_pdt_log_exit();
#endif

    /* 卸载成功后，输出打印 */
#if ((_PRE_OS_VERSION_RAW != _PRE_OS_VERSION) && (_PRE_OS_VERSION_WIN32_RAW != _PRE_OS_VERSION))
    #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
       /* 所用的端口号在系统中已被占用，02无法使用这个接口，需要修改 TBD */

        oam_netlink_kernel_release_etc();
    #endif
#endif

    return ;
}

/*lint -e578*//*lint -e19*/
#if (_PRE_PRODUCT_ID_HI1151==_PRE_PRODUCT_ID)
oal_module_init(oam_main_init_etc);
oal_module_exit(oam_main_exit_etc);
#endif
/*lint -e19*/
oal_module_symbol(oam_main_init_etc);
oal_module_symbol(oam_main_exit_etc);
oal_module_symbol(oam_send_device_data2sdt_etc);
oal_module_symbol(oam_set_file_path_etc);
oal_module_symbol(oam_set_output_type_etc);
oal_module_symbol(oam_get_output_type_etc);
oal_module_symbol(oam_print_etc);
oal_module_symbol(g_st_oam_mng_ctx_etc);
oal_module_symbol(oam_dump_buff_by_hex_etc);
oal_module_symbol(g_st_oam_sdt_func_hook_etc);
oal_module_symbol(g_st_oam_wal_func_hook_etc);
oal_module_symbol(oam_sdt_func_fook_register_etc);
oal_module_symbol(oam_sdt_func_fook_unregister_etc);
oal_module_symbol(oam_wal_func_fook_register_etc);
oal_module_symbol(oam_wal_func_fook_unregister_etc);
oal_module_symbol(oam_report_data2sdt_etc);
oal_module_symbol(g_st_sdt_stat_info_etc);
oal_module_symbol(oam_alloc_data2sdt_etc);
oal_module_symbol(gst_oam_feature_list_etc);
oal_module_symbol(g_auc_bcast_addr_etc);

#ifdef _PRE_DEBUG_MODE
oal_module_symbol(g_aul_debug_feature_switch_etc);
#endif

oal_module_license("GPL");







#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

