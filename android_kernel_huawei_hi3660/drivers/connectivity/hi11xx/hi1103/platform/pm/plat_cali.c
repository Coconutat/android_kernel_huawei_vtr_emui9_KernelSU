

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "plat_firmware.h"
#include "plat_cali.h"
#include "plat_debug.h"
#include "plat_type.h"
#include "board.h"
#include "plat_pm.h"
#include "hisi_ini.h"
#include <linux/version.h>
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
#define HISI_NVRAM_SUPPORT
#endif


/*****************************************************************************
  2 宏定义
*****************************************************************************/

/*****************************************************************************
  3 全局变量定义
*****************************************************************************/

/*保存校准数据的buf*/
oal_uint8 *g_pucCaliDataBuf_etc      = NULL;  /* 03 wifi校准数据 */
oal_uint8  g_uc_netdev_is_open_etc   = OAL_FALSE;
oal_uint32 g_ul_cali_update_channel_info = 0;


/*add for hi1103 bfgx*/
struct completion g_st_cali_recv_done;
oal_uint8 *g_pucBfgxCaliDataBuf = NULL;

bfgx_ini_cmd g_ast_bfgx_ini_config_cmd[BFGX_BT_CUST_INI_SIZE/4] =
{
    {"bt_maxpower"                      , 0},
    {"bt_edrpow_offset"                 , 0},
    {"bt_blepow_offset"                 , 0},
    {"bt_cali_txpwr_pa_ref_num"         , 0},
    {"bt_cali_txpwr_pa_ref_band1"       , 0},
    {"bt_cali_txpwr_pa_ref_band2"       , 0},
    {"bt_cali_txpwr_pa_ref_band3"       , 0},
    {"bt_cali_txpwr_pa_ref_band4"       , 0},
    {"bt_cali_txpwr_pa_ref_band5"       , 0},
    {"bt_cali_txpwr_pa_ref_band6"       , 0},
    {"bt_cali_txpwr_pa_ref_band7"       , 0},
    {"bt_cali_txpwr_pa_ref_band8"       , 0},
    {"bt_cali_txpwr_pa_fre1"            , 0},
    {"bt_cali_txpwr_pa_fre2"            , 0},
    {"bt_cali_txpwr_pa_fre3"            , 0},
    {"bt_cali_txpwr_pa_fre4"            , 0},
    {"bt_cali_txpwr_pa_fre5"            , 0},
    {"bt_cali_txpwr_pa_fre6"            , 0},
    {"bt_cali_txpwr_pa_fre7"            , 0},
    {"bt_cali_txpwr_pa_fre8"            , 0},
    {"bt_cali_bt_tone_amp_grade"        , 0},
    {"bt_rxdc_band"                     , 0},
    {"bt_dbb_scaling_saturation"        , 0},
    {"bt_productline_upccode_search_max", 0},
    {"bt_productline_upccode_search_min", 0},
    {"bt_dynamicsarctrl_bt"             , 0},
    {"bt_powoffsbt"                     , 0},
    {"bt_elna_2g_bt"                    , 0},
    {"bt_rxisobtelnabyp"                , 0},
    {"bt_rxgainbtelna"                  , 0},
    {"bt_rxbtextloss"                   , 0},
    {"bt_elna_on2off_time_ns"           , 0},
    {"bt_elna_off2on_time_ns"           , 0},
    {"bt_hipower_mode"                  , 0},
    {"bt_fem_control"                   , 0},
    {"bt_feature_32k_clock"             , 0},
    {"bt_feature_log"                   , 0},
    {"bt_cali_swtich_all"               , 0},
    {"bt_ant_num_bt"                    , 0},
    {"bt_power_level_control"           , 0},
    {"bt_country_code"                  , 0},
    {"bt_reserved1"                     , 0},
    {"bt_reserved2"                     , 0},
    {"bt_reserved3"                     , 0},
    {"bt_reserved4"                     , 0},
    {"bt_reserved5"                     , 0},
    {"bt_reserved6"                     , 0},
    {"bt_reserved7"                     , 0},
    {"bt_reserved8"                     , 0},
    {"bt_reserved9"                     , 0},
    {"bt_reserved10"                    , 0},

    {NULL, 0}
};

int32 g_aul_bfgx_cust_ini_data[BFGX_BT_CUST_INI_SIZE/4] = {0};

/*****************************************************************************
  4 函数实现
*****************************************************************************/


oal_int32 get_cali_count_etc(oal_uint32 *count)
{
    if (NULL == count)
    {
        PS_PRINT_ERR("count is NULL\n");
        return -EFAIL;
    }

    *count = g_ul_cali_update_channel_info;

    PS_PRINT_WARNING("cali update info is [%d]\r\n", g_ul_cali_update_channel_info);

    return SUCC;
}

#ifdef HISI_NVRAM_SUPPORT

oal_int32 bfgx_nv_data_init(void)
{
    int32  l_ret = INI_FAILED;
    int8  *pst_buf;
    uint32 ul_len;

    oal_uint8 bt_cal_nvram_tmp[OAL_BT_NVRAM_DATA_LENGTH];

    l_ret = read_conf_from_nvram_etc(bt_cal_nvram_tmp, OAL_BT_NVRAM_DATA_LENGTH,
                                    OAL_BT_NVRAM_NUMBER, OAL_BT_NVRAM_NAME);
    if (l_ret != INI_SUCC)
    {
        PS_PRINT_ERR("bfgx_nv_data_init::BT read NV error!");
        return INI_FAILED;
    }

    pst_buf = bfgx_get_nv_data_buf(&ul_len);
    if (NULL == pst_buf)
    {
        PS_PRINT_ERR("get bfgx nv buf fail!");
        return INI_FAILED;
    }
    if(ul_len < OAL_BT_NVRAM_DATA_LENGTH)
    {
        PS_PRINT_ERR("get bfgx nv buf size %d, NV data size is %d!", ul_len, OAL_BT_NVRAM_DATA_LENGTH);
        return INI_FAILED;
    }

    OS_MEM_CPY(pst_buf, bt_cal_nvram_tmp, OAL_BT_NVRAM_DATA_LENGTH);
    PS_PRINT_INFO("bfgx_nv_data_init SUCCESS");
    return INI_SUCC;
}
#endif


int32 hi1102_get_bfgx_cali_data(oal_uint8 *buf, oal_uint32 *len, oal_uint32 buf_len)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
    oal_cali_param_stru *pst_cali_data         = NULL;
    oal_cali_param_addition_stru cali_addition = {0x00};
    oal_uint32 bfgx_cali_data_len;
    oal_uint32 bfgx_cali_addition_len;
    oal_int32  wifi_5g_enable_info = WIFI_MODE_5G;
    oal_int32  result;

    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == buf)
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -EFAIL;
    }

    if (NULL == len)
    {
        PS_PRINT_ERR("len is NULL\n");
        return -EFAIL;
    }

    bfgx_cali_data_len     = sizeof(oal_bfgn_cali_param_stru);
    bfgx_cali_addition_len = sizeof(oal_cali_param_addition_stru);
    if (buf_len < (bfgx_cali_data_len + bfgx_cali_addition_len))
    {
        PS_PRINT_ERR("buf_len[%d] is smaller than struct size[%d]\n", buf_len, bfgx_cali_data_len + bfgx_cali_addition_len);
        return -EFAIL;
    }

    if (NULL == g_pucCaliDataBuf_etc)
    {
        PS_PRINT_ERR("g_pucCaliDataBuf_etc is NULL\n");
        return -EFAIL;
    }

    pst_cali_data = (oal_cali_param_stru *)g_pucCaliDataBuf_etc;
    OS_MEM_CPY(buf, (oal_uint8 *)&(pst_cali_data->st_bfgn_cali_data), bfgx_cali_data_len);
    *len = bfgx_cali_data_len;

    /**********************************************************************************
    |----------------------------------------------------------------------------------|
    |   oal_cali_param_stru          : 源数据结构        |            216 byte         |
    |----------------------------------------------------------------------------------|
    |   oal_cali_param_addition_stru : 追加数据结构      |             40 byte         |
    |----------------------------------------------------------------------------------|
    ************************************************************************************/

    /******************************* WIFI 5G使能检查标志位 ******************************/
    result = get_cust_conf_int32_etc(INI_MODU_WIFI, CHECK_5G_ENABLE, &wifi_5g_enable_info);
    if (0 > result)
    {
        PS_PRINT_WARNING("host get wifi 5g enable info fail\n");
        /* 读取失败,默认为5G */
        wifi_5g_enable_info = WIFI_MODE_5G;
    }

    if (WIFI_MODE_2G == wifi_5g_enable_info)
    {
        cali_addition.ul_wifi_2_4g_only = WIFI_2_4G_ONLY;
    }
    else
    {
        cali_addition.ul_wifi_2_4g_only = 0;
    }

    /******************************** bfgx异常处理结束检查标志位 *********************************/
    if (is_bfgx_exception_etc())
    {
        cali_addition.ul_excep_reboot = SYS_EXCEP_REBOOT;
    }
    else
    {
        cali_addition.ul_excep_reboot = 0;
    }

    OS_MEM_CPY((buf + *len), (oal_uint8 *)(&cali_addition), bfgx_cali_addition_len);
    *len += bfgx_cali_addition_len;
#endif

    return SUCC;
}


int32 hi1103_get_bfgx_cali_data(oal_uint8 *buf, oal_uint32 *len, oal_uint32 buf_len)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    oal_uint32 bfgx_cali_data_len = 0;

    PS_PRINT_INFO("%s\n", __func__);

    if (unlikely(NULL == buf))
    {
        PS_PRINT_ERR("buf is NULL\n");
        return -EFAIL;
    }

    if (unlikely(NULL == len))
    {
        PS_PRINT_ERR("len is NULL\n");
        return -EFAIL;
    }

    if (unlikely(NULL == g_pucBfgxCaliDataBuf))
    {
        PS_PRINT_ERR("g_pucBfgxCaliDataBuf is NULL\n");
        return -EFAIL;
    }

    #ifdef HISI_NVRAM_SUPPORT
    if (OAL_SUCC != bfgx_nv_data_init())
    {
       PS_PRINT_ERR("bfgx nv data init fail!\n");
    }
    #endif

    bfgx_cali_data_len = sizeof(bfgx_cali_data_stru);
    if (buf_len < bfgx_cali_data_len)
    {
        PS_PRINT_ERR("bfgx cali buf len[%d] is smaller than struct size[%d]\n", buf_len, bfgx_cali_data_len);
        return -EFAIL;
    }

    OS_MEM_CPY(buf, g_pucBfgxCaliDataBuf, bfgx_cali_data_len);
    *len = bfgx_cali_data_len;
#endif

    return SUCC;
}


int32 get_bfgx_cali_data_etc(oal_uint8 *buf, oal_uint32 *len, oal_uint32 buf_len)
{
    oal_int32 l_subchip_type = get_hi110x_subchip_type();

    switch (l_subchip_type)
    {
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
        case BOARD_VERSION_HI1102:
            return hi1102_get_bfgx_cali_data(buf, len, buf_len);
#elif (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
        case BOARD_VERSION_HI1103:
            return hi1103_get_bfgx_cali_data(buf, len, buf_len);
#endif
        default:
            PS_PRINT_ERR("subchip type error! subchip=%d\n", l_subchip_type);
            return -EFAIL;
    }
}


void *get_cali_data_buf_addr_etc(void)
{
    return g_pucCaliDataBuf_etc;
}

EXPORT_SYMBOL(get_cali_data_buf_addr_etc);
EXPORT_SYMBOL(g_uc_netdev_is_open_etc);
EXPORT_SYMBOL(g_ul_cali_update_channel_info);



void *wifi_get_bfgx_rc_data_buf_addr(uint32 *pul_len)
{
    bfgx_cali_data_stru *pst_bfgx_cali_buf = NULL;

    if (NULL == g_pucBfgxCaliDataBuf)
    {
        return NULL;
    }

    pst_bfgx_cali_buf = (bfgx_cali_data_stru *)g_pucBfgxCaliDataBuf;
    *pul_len = sizeof(pst_bfgx_cali_buf->auc_wifi_rc_code_data);

    PS_PRINT_INFO("wifi cali size is %d\n", *pul_len);

    return pst_bfgx_cali_buf->auc_wifi_rc_code_data;
}

EXPORT_SYMBOL(wifi_get_bfgx_rc_data_buf_addr);


void *wifi_get_bt_cali_data_buf(uint32 *pul_len)
{
    bfgx_cali_data_stru *pst_bfgx_cali_data_buf;

    if (NULL == g_pucBfgxCaliDataBuf)
    {
        return NULL;
    }

    pst_bfgx_cali_data_buf = (bfgx_cali_data_stru *)g_pucBfgxCaliDataBuf;

    *pul_len = sizeof(pst_bfgx_cali_data_buf->auc_wifi_cali_for_bt_data);

    PS_PRINT_INFO("bfgx wifi cali data for bt buf size is %d\n", *pul_len);

    return pst_bfgx_cali_data_buf->auc_wifi_cali_for_bt_data;
}

EXPORT_SYMBOL(wifi_get_bt_cali_data_buf);


void *bfgx_get_cali_data_buf(uint32 *pul_len)
{
    bfgx_cali_data_stru *pst_bfgx_cali_data_buf;

    if (NULL == g_pucBfgxCaliDataBuf)
    {
        return NULL;
    }

    pst_bfgx_cali_data_buf = (bfgx_cali_data_stru *)g_pucBfgxCaliDataBuf;

    *pul_len = sizeof(pst_bfgx_cali_data_buf->auc_bfgx_data);

    PS_PRINT_INFO("bfgx bt cali data buf size is %d\n", *pul_len);

    return pst_bfgx_cali_data_buf->auc_bfgx_data;
}


void *bfgx_get_nv_data_buf(uint32 *pul_len)
{
    bfgx_cali_data_stru *pst_bfgx_cali_data_buf;

    if (NULL == g_pucBfgxCaliDataBuf)
    {
        return NULL;
    }

    pst_bfgx_cali_data_buf = (bfgx_cali_data_stru *)g_pucBfgxCaliDataBuf;

    *pul_len = sizeof(pst_bfgx_cali_data_buf->auc_nv_data);

    PS_PRINT_INFO("bfgx nv buf size is %d\n", *pul_len);

    return pst_bfgx_cali_data_buf->auc_nv_data;
}


void *bfgx_get_cust_ini_data_buf(uint32 *pul_len)
{
    bfgx_cali_data_stru *pst_bfgx_cali_data_buf;

    if (NULL == g_pucBfgxCaliDataBuf)
    {
        return NULL;
    }

    pst_bfgx_cali_data_buf = (bfgx_cali_data_stru *)g_pucBfgxCaliDataBuf;

    *pul_len = sizeof(pst_bfgx_cali_data_buf->auc_bt_cust_ini_data);

    PS_PRINT_INFO("bfgx cust ini buf size is %d\n", *pul_len);

    return pst_bfgx_cali_data_buf->auc_bt_cust_ini_data;
}


void plat_bfgx_cali_data_test_etc(void)
{
    bfgx_cali_data_stru *pst_cali_data = NULL;
    oal_uint32 *p_test = NULL;
    oal_uint32 count;
    oal_uint32 i;

    pst_cali_data = (bfgx_cali_data_stru *)bfgx_get_cali_data_buf(&count);
    if (NULL == pst_cali_data)
    {
        PS_PRINT_ERR("get_cali_data_buf_addr_etc failed\n");
        return;
    }

    p_test = (oal_uint32 *)pst_cali_data;
    count  = count / sizeof(oal_uint32);

    for (i = 0; i < count; i++)
    {
        p_test[i] = i;
    }

    return;
}


oal_int32 cali_data_buf_malloc_etc(void)
{
    oal_uint8 *buffer = NULL;
    oal_uint32 ul_buffer_len;

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    ul_buffer_len = OAL_DOUBLE_CALI_DATA_STRU_LEN;
#else
    ul_buffer_len = RF_CALI_DATA_BUF_LEN;
#endif

    buffer = (oal_uint8 *)OS_KZALLOC_GFP(ul_buffer_len);

    if (NULL == buffer)
    {
        PS_PRINT_ERR("malloc for g_pucCaliDataBuf_etc fail\n");
        return -EFAIL;
    }
    g_pucCaliDataBuf_etc = buffer;
    OAL_MEMZERO(g_pucCaliDataBuf_etc, ul_buffer_len);

    buffer = (oal_uint8 *)OS_KZALLOC_GFP(BFGX_CALI_DATA_BUF_LEN);
    if (NULL == buffer)
    {
        OS_MEM_KFREE(g_pucCaliDataBuf_etc);
        g_pucCaliDataBuf_etc = NULL;
        PS_PRINT_ERR("malloc for g_pucBfgxCaliDataBuf fail\n");
        return -EFAIL;
    }
    g_pucBfgxCaliDataBuf = buffer;

    init_completion(&g_st_cali_recv_done);

    //plat_bfgx_cali_data_test_etc();

    return SUCC;
}


void cali_data_buf_free_etc(void)
{
    if (NULL != g_pucCaliDataBuf_etc)
    {
        OS_MEM_KFREE(g_pucCaliDataBuf_etc);
    }
    g_pucCaliDataBuf_etc = NULL;

    if (NULL != g_pucBfgxCaliDataBuf)
    {
        OS_MEM_KFREE(g_pucBfgxCaliDataBuf);
    }
    g_pucBfgxCaliDataBuf = NULL;
}


int32 wait_bfgx_cali_data(void)
{
#define WAIT_BFGX_CALI_DATA_TIME (2000)
    uint64 timeleft;

    timeleft = wait_for_completion_timeout(&g_st_cali_recv_done, msecs_to_jiffies(WAIT_BFGX_CALI_DATA_TIME));
    if (!timeleft)
    {
        PS_PRINT_ERR("wait bfgx cali data timeout\n");
        return -ETIMEDOUT;
    }

    return 0;
}


int32 bfgx_cust_ini_init(void)
{
    int32  i;
    int32  l_ret = INI_FAILED;
    int32  l_cfg_value;
    int32  l_ori_val;
    int8  *pst_buf;
    uint32 ul_len;

    for(i = 0; i < BFGX_CFG_INI_BUTT; i++)
    {
        l_ori_val = g_ast_bfgx_ini_config_cmd[i].init_value;

        /* 获取ini的配置值 */
        l_ret = get_cust_conf_int32_etc(INI_MODU_DEV_BT, g_ast_bfgx_ini_config_cmd[i].name, &l_cfg_value);
        if (INI_FAILED == l_ret)
        {
            g_aul_bfgx_cust_ini_data[i] = l_ori_val;
            PS_PRINT_WARNING("bfgx read ini file failed cfg_id[%d],default value[%d]!", i, l_ori_val);
            continue;
        }

        g_aul_bfgx_cust_ini_data[i] = l_cfg_value;

        PS_PRINT_INFO("bfgx ini init [id:%d] [%s] changed from [%d]to[%d]", i, g_ast_bfgx_ini_config_cmd[i].name, l_ori_val, l_cfg_value);
    }

    pst_buf = bfgx_get_cust_ini_data_buf(&ul_len);
    if (NULL == pst_buf)
    {
        PS_PRINT_ERR("get cust ini buf fail!");
        return INI_FAILED;
    }

    OS_MEM_CPY(pst_buf, g_aul_bfgx_cust_ini_data, ul_len);

    return INI_SUCC;
}


int32 bfgx_customize_init(void)
{
    int32  ret = 0;

    /*申请用于保存校准数据的buffer*/
    ret = cali_data_buf_malloc_etc();
    if(OAL_SUCC != ret)
    {
        PS_PRINT_ERR("alloc cali data buf fail\n");
        return INI_FAILED;
    }

    ret = bfgx_cust_ini_init();
    if (OAL_SUCC != ret)
    {
       PS_PRINT_ERR("bfgx ini init fail!\n");
       cali_data_buf_free_etc();
       return INI_FAILED;
    }

#ifdef HISI_NVRAM_SUPPORT
    ret = bfgx_nv_data_init();
    if (OAL_SUCC != ret)
    {
       PS_PRINT_ERR("bfgx nv data init fail!\n");
       cali_data_buf_free_etc();
       return INI_FAILED;
    }
#endif

    return INI_SUCC;
}


oal_int32 bfgx_cali_data_init(void)
{
    int32  ret = 0;
    static uint32 cali_flag = 0;
    struct ps_core_s *ps_core_d = NULL;
    struct st_bfgx_data *pst_bfgx_data = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    PS_PRINT_INFO("%s\n", __func__);

    /*校准只在开机时执行一次，OAM可能被杀掉重启，所以加标志保护*/
    if (0 != cali_flag)
    {
        PS_PRINT_INFO("bfgx cali data has inited\n");
        return 0;
    }

    cali_flag++;

    if (NULL == pm_data)
    {
       PS_PRINT_ERR("pm_data is NULL!\n");
       return OAL_FAIL;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return OAL_FAIL;
    }

    mutex_lock(&pm_data->host_mutex);

    pst_bfgx_data = &ps_core_d->bfgx_info[BFGX_BT];
    if (POWER_STATE_OPEN == atomic_read(&pst_bfgx_data->subsys_state))
    {
        PS_PRINT_WARNING("%s has opened! ignore bfgx cali!\n", g_bfgx_subsys_name_etc[BFGX_BT]);
        goto open_fail;
    }

    ret = hw_bfgx_open(BFGX_BT);
    if (SUCC != ret)
    {
        PS_PRINT_ERR("bfgx cali, open bt fail\n");
        goto open_fail;
    }

    ret = wait_bfgx_cali_data();
    if (SUCC != ret)
    {
        goto timeout;
    }

    ret = hw_bfgx_close(BFGX_BT);
    if (SUCC != ret)
    {
        PS_PRINT_ERR("bfgx cali, clsoe bt fail\n");
        goto close_fail;
    }

    mutex_unlock(&pm_data->host_mutex);

    return ret;

timeout:
    hw_bfgx_close(BFGX_BT);
close_fail:
open_fail:
    mutex_unlock(&pm_data->host_mutex);
    return ret;
}

