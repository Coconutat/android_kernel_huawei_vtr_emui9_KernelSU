


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "wlan_spec.h"
#include "mac_resource.h"
#if (defined(_PRE_PRODUCT_ID_HI110X_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151))
#include "dmac_vap.h"
#include "dmac_user.h"
#endif
#if (defined(_PRE_PRODUCT_ID_HI110X_HOST) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151))
#include "hmac_vap.h"
#include "hmac_user.h"
#endif


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_RESOURCE_ROM_C


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
extern dmac_vap_stru g_ast_dmac_vap[WLAN_VAP_SUPPORT_MAX_NUM_LIMIT];
extern dmac_user_stru g_ast_dmac_user[MAC_RES_MAX_USER_LIMIT];
extern mac_res_user_idx_size_stru g_st_dmac_user_idx_size[MAC_RES_MAX_USER_LIMIT];
extern mac_res_user_cnt_size_stru g_st_dmac_user_cnt_size[MAC_RES_MAX_USER_LIMIT];

dmac_vap_stru *g_pst_dmac_vap = &g_ast_dmac_vap[0];
dmac_user_stru *g_pst_dmac_user = &g_ast_dmac_user[0];
mac_res_user_idx_size_stru *g_pst_dmac_user_idx_size = &g_st_dmac_user_idx_size[0];
mac_res_user_cnt_size_stru *g_pst_dmac_user_cnt_size = &g_st_dmac_user_cnt_size[0];
#endif

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
extern hmac_vap_stru g_ast_hmac_vap[WLAN_VAP_SUPPORT_MAX_NUM_LIMIT];
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

#define MAC_VAP_STRUCT_SIZE   (OAL_SIZEOF(hmac_vap_stru) + OAL_SIZEOF(dmac_vap_stru) - OAL_SIZEOF(mac_vap_stru))
#define MAC_RES_VAP_SIZE    ((MAC_VAP_STRUCT_SIZE) - ((MAC_VAP_STRUCT_SIZE) & (OAL_MEM_INFO_SIZE-1)) + OAL_MEM_INFO_SIZE)
#define MAC_USER_STRUCT_SIZE  (OAL_SIZEOF(hmac_user_stru) + OAL_SIZEOF(dmac_user_stru) - OAL_SIZEOF(mac_user_stru))
#define MAC_RES_USER_SIZE   ((MAC_USER_STRUCT_SIZE) - ((MAC_USER_STRUCT_SIZE) & (OAL_MEM_INFO_SIZE-1)) + OAL_MEM_INFO_SIZE)

typedef struct
{
   oal_uint8       uc_vap[MAC_RES_VAP_SIZE];
}mac_res_mem_vap_stru;

typedef struct
{
   oal_uint8       uc_user[MAC_RES_USER_SIZE];
}mac_res_mem_user_stru;

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151) && (_PRE_TEST_MODE != _PRE_TEST_MODE_UT)
mac_res_mem_vap_stru *g_pst_mac_res_vap = OAL_PTR_NULL;
#else
mac_res_mem_vap_stru g_ast_mac_res_vap[WLAN_VAP_SUPPORT_MAX_NUM_LIMIT];
#endif

#endif

/* 后续放入定制化来刷 */
/* 1个chip支持的最大关联用户数 */
oal_uint16      g_us_max_asoc_user_etc = WLAN_ASSOC_USER_MAX_NUM;
/* 1个chip支持的最大激活用户数 */
oal_uint8      g_uc_max_active_user = WLAN_ACTIVE_USER_MAX_NUM;

#ifdef _PRE_WLAN_WEB_CMD_COMM
oal_uint8 g_hw_addr[6] = {0x05, 0x04, 0x03, 0x02, 0x01, 0x00};
oal_uint64 g_mac_bmp = 0;
#define MAC_RES_MAC_ADDR_NO     (WLAN_DEVICE_SUPPORT_MAX_NUM_SPEC * WLAN_AP_STA_COEXIST_VAP_NUM)
#endif

mac_res_stru  *g_pst_mac_res = &g_st_mac_res_etc;

/*****************************************************************************
  3 函数实现
*****************************************************************************/
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
SIZE_OF_SIZE1_NOT_LARGER_THAN_SIZE2_BY_NAME(MAC_RES_VAP_SIZE, (OAL_SIZEOF(hmac_vap_stru) + OAL_SIZEOF(dmac_vap_stru) - OAL_SIZEOF(mac_vap_stru)),MAC_RES_VAP_SIZE)
SIZE_OF_SIZE1_NOT_LARGER_THAN_SIZE2_BY_NAME(MAC_RES_USER_SIZE, (OAL_SIZEOF(hmac_user_stru) + OAL_SIZEOF(dmac_user_stru) - OAL_SIZEOF(mac_user_stru)),MAC_RES_USER_SIZE)
#endif


oal_uint32  mac_res_check_spec_etc(oal_void)
{
    oal_uint32  ul_ret = OAL_SUCC;


#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    if ((OAL_SIZEOF(hmac_vap_stru) + OAL_SIZEOF(dmac_vap_stru) - OAL_SIZEOF(mac_vap_stru)) > MAC_RES_VAP_SIZE)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{mac_res_check_spec_etc::vap_stru is over limit! vap_stru[%d], MAC_RES_VAP_SIZE[%d]}",
                      (OAL_SIZEOF(hmac_vap_stru) + OAL_SIZEOF(dmac_vap_stru) - OAL_SIZEOF(mac_vap_stru)),
                       MAC_RES_VAP_SIZE);
        OAL_IO_PRINT("{mac_res_check_spec_etc::vap_stru is over limit! hmac_vap[%d], dmac_vap[%d], mac_vap[%d], MAC_RES_VAP_SIZE[%d].\r\n}",
                     OAL_SIZEOF(hmac_vap_stru), OAL_SIZEOF(dmac_vap_stru), OAL_SIZEOF(mac_vap_stru), MAC_RES_VAP_SIZE);
        ul_ret = OAL_FAIL;
    }

    if ((OAL_SIZEOF(hmac_user_stru) + OAL_SIZEOF(dmac_user_stru) - OAL_SIZEOF(mac_user_stru)) > MAC_RES_USER_SIZE)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{mac_res_check_spec_etc::user_stru is over limit! user_stru[%d], MAC_RES_USER_SIZE[%d]}",
                       (OAL_SIZEOF(hmac_user_stru) + OAL_SIZEOF(dmac_user_stru) - OAL_SIZEOF(mac_user_stru)), MAC_RES_USER_SIZE);
        OAL_IO_PRINT("{mac_res_check_spec_etc::user_stru is over limit! hmac_user[%d], dmac_user[%d], mac_user[%d], MAC_RES_USER_SIZE[%d].\r\n}",
                     OAL_SIZEOF(hmac_user_stru), OAL_SIZEOF(dmac_user_stru), OAL_SIZEOF(mac_user_stru), MAC_RES_USER_SIZE);
        ul_ret = OAL_FAIL;
    }
#endif
    return ul_ret;
}


#if (defined(_PRE_PRODUCT_ID_HI110X_DEV))

oal_void  mac_res_vap_init(oal_void)
{
    oal_uint32 ul_loop;

    oal_uint ul_one_vap_size;

    g_pst_mac_res->st_vap_res.us_hmac_priv_size = 0;
    ul_one_vap_size = 0;
    for (ul_loop = 0; ul_loop < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; ul_loop++)
    {
        g_pst_mac_res->st_vap_res.past_vap_info[ul_loop] = (oal_uint8 *)g_pst_dmac_vap + ul_one_vap_size;
        OAL_MEMZERO(g_pst_mac_res->st_vap_res.past_vap_info[ul_loop], OAL_SIZEOF(dmac_vap_stru));

        ul_one_vap_size += OAL_SIZEOF(dmac_vap_stru);
                /* 初始化对应的引用计数值为0 */
        g_pst_mac_res->st_vap_res.auc_user_cnt[ul_loop] = 0;
    }
}



oal_uint32  mac_res_user_init_etc(oal_void)
{
    oal_uint32      ul_loop;
    oal_uint        ul_one_user_info_size;

    if ((OAL_PTR_NULL == g_pst_dmac_user) || (OAL_PTR_NULL == g_pst_dmac_user_idx_size) || (OAL_PTR_NULL == g_pst_dmac_user_cnt_size))
    {
        OAM_ERROR_LOG0(0, OAM_SF_UM, "{mac_res_user_init_etc:input para null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 内存初始清0 */
    OAL_MEMZERO(g_pst_dmac_user, (OAL_SIZEOF(dmac_user_stru) * MAC_RES_MAX_USER_LIMIT));
    OAL_MEMZERO(g_pst_dmac_user_idx_size, (OAL_SIZEOF(oal_uint) * MAC_RES_MAX_USER_LIMIT));
    OAL_MEMZERO(g_pst_dmac_user_cnt_size, (OAL_SIZEOF(oal_uint8) * MAC_RES_MAX_USER_LIMIT));

    g_pst_mac_res->st_user_res.pul_idx        = (oal_uint *)g_pst_dmac_user_idx_size;
    g_pst_mac_res->st_user_res.puc_user_cnt   = (oal_uint8 *)g_pst_dmac_user_cnt_size;

    g_pst_mac_res->st_user_res.us_hmac_priv_size = 0;

    ul_one_user_info_size = 0;
    for (ul_loop = 0; ul_loop < MAC_RES_MAX_USER_LIMIT; ul_loop++)
    {
        /* 初始化对应的引用计数值为0 */
        g_pst_mac_res->st_user_res.past_user_info[ul_loop]  = (oal_uint8 *)g_pst_dmac_user + ul_one_user_info_size;
        ul_one_user_info_size += OAL_SIZEOF(dmac_user_stru);
    }

    return OAL_SUCC;
}


oal_uint32  mac_res_exit_etc(void)
{
    oal_uint ul_loop;

    OAL_MEM_FREE((g_pst_mac_res->st_user_res.past_user_info[0]), OAL_TRUE);

    for (ul_loop = 0; ul_loop < MAC_RES_MAX_USER_LIMIT; ul_loop++)
    {
       g_pst_mac_res->st_user_res.past_user_info[ul_loop]      = OAL_PTR_NULL;
    }

    g_pst_mac_res->st_user_res.pul_idx            = OAL_PTR_NULL;
    g_pst_mac_res->st_user_res.puc_user_cnt       = OAL_PTR_NULL;

    return OAL_SUCC;
}

oal_uint32  mac_res_init_etc(oal_void)
{
    oal_uint        ul_loop;
    oal_uint32      ul_ret;

    OAL_MEMZERO(g_pst_mac_res, OAL_SIZEOF(mac_res_stru));
    /***************************************************************************
            初始化DEV的资源管理内容
    ***************************************************************************/
    oal_queue_set(&(g_pst_mac_res->st_dev_res.st_queue),
                  g_pst_mac_res->st_dev_res.aul_idx,
                  MAC_RES_MAX_DEV_NUM);

    ul_ret = mac_res_check_spec_etc();
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{mac_res_init_etc::mac_res_init_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    for (ul_loop = 0; ul_loop < MAC_RES_MAX_DEV_NUM; ul_loop++)
    {
        /* 初始值保存的是对应数组下标值加1 */
        oal_queue_enqueue(&(g_pst_mac_res->st_dev_res.st_queue), (oal_void *)(ul_loop + 1));

        /* 初始化对应的引用计数值为0 */
        g_pst_mac_res->st_dev_res.auc_user_cnt[ul_loop] = 0;
    }

    /***************************************************************************
            初始化VAP的资源管理内容
    ***************************************************************************/
    mac_res_vap_init();
    /***************************************************************************
            初始化USER的资源管理内容
            初始化HASH桶的资源管理内容
    ***************************************************************************/
    ul_ret = mac_res_user_init_etc();
    if (OAL_SUCC != ul_ret)
    {
        MAC_ERR_LOG1(0, "mac_res_init_etc: mac_res_user_init_etc return err code", ul_ret);
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{mac_res_init_etc::mac_res_user_init_etc failed[%d].}", ul_ret);

        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  mac_res_free_mac_user_etc(oal_uint16 us_idx)
{
    if (OAL_UNLIKELY(us_idx >= MAC_RES_MAX_USER_LIMIT))
    {
        return OAL_FAIL;
    }

    if(0 == g_pst_mac_res->st_user_res.puc_user_cnt[us_idx])
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY,"mac_res_free_mac_user_etc::cnt==0! idx:%d",us_idx);
        oal_dump_stack();
        return OAL_ERR_CODE_USER_RES_CNT_ZERO;
    }

    (g_pst_mac_res->st_user_res.puc_user_cnt[us_idx])--;

    if (0 != g_pst_mac_res->st_user_res.puc_user_cnt[us_idx])
    {
        return OAL_SUCC;
    }

    return OAL_SUCC;
}


oal_uint32  mac_res_free_mac_vap_etc(oal_uint32 ul_idx)
{
    if (OAL_UNLIKELY(ul_idx >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT))
    {
        return OAL_FAIL;
    }

    if(0 == g_pst_mac_res->st_vap_res.auc_user_cnt[ul_idx])
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY,"mac_res_free_mac_vap_etc::cnt==0! idx:%d",ul_idx);
        oal_dump_stack();
        return OAL_SUCC;
    }

    (g_pst_mac_res->st_vap_res.auc_user_cnt[ul_idx])--;

    if (0 != g_pst_mac_res->st_vap_res.auc_user_cnt[ul_idx])
    {
        return OAL_SUCC;
    }

    return OAL_SUCC;
}

#else


oal_void  mac_res_vap_init(oal_void)
{
    oal_uint ul_loop;

    oal_queue_set(&(g_pst_mac_res->st_vap_res.st_queue),
                  g_pst_mac_res->st_vap_res.aul_idx,
                  (oal_uint8)WLAN_VAP_SUPPORT_MAX_NUM_LIMIT);
    g_pst_mac_res->st_vap_res.us_hmac_priv_size = (oal_uint16)OAL_OFFSET_OF(hmac_vap_stru, st_vap_base_info);


#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151) && (_PRE_TEST_MODE != _PRE_TEST_MODE_UT)
    g_pst_mac_res_vap = (mac_res_mem_vap_stru *)oal_memalloc(WLAN_VAP_SUPPORT_MAX_NUM_LIMIT * OAL_SIZEOF(mac_res_mem_vap_stru));
    if(OAL_PTR_NULL == g_pst_mac_res_vap)
    {
        OAL_IO_PRINT("[file = %s, line = %d], mac_res_vap_init, memory allocation g_pst_mac_res_vap %u bytes fail!\n",
                        __FILE__, __LINE__, (oal_uint32)(WLAN_VAP_SUPPORT_MAX_NUM_LIMIT * OAL_SIZEOF(mac_res_mem_vap_stru)));
        return;
    }
    oal_memset(g_pst_mac_res_vap, 0, WLAN_VAP_SUPPORT_MAX_NUM_LIMIT * OAL_SIZEOF(mac_res_mem_vap_stru));
#endif

    for (ul_loop = 0; ul_loop < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; ul_loop++)
    {
          #if (defined(_PRE_PRODUCT_ID_HI110X_HOST))
          g_pst_mac_res->st_vap_res.past_vap_info[ul_loop] = (oal_void*)&g_ast_hmac_vap[ul_loop];
          OAL_MEMZERO(g_pst_mac_res->st_vap_res.past_vap_info[ul_loop], OAL_SIZEOF(hmac_vap_stru));
          #else
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151) && (_PRE_TEST_MODE != _PRE_TEST_MODE_UT)
          g_pst_mac_res->st_vap_res.past_vap_info[ul_loop] = (oal_void*)&g_pst_mac_res_vap[ul_loop];
#else
          g_pst_mac_res->st_vap_res.past_vap_info[ul_loop] = (oal_void*)&g_ast_mac_res_vap[ul_loop];
#endif
		  OAL_MEMZERO(g_pst_mac_res->st_vap_res.past_vap_info[ul_loop], OAL_SIZEOF(mac_res_mem_vap_stru));
          #endif

          /* 初始值保存的是对应数组下标值加1 */
          oal_queue_enqueue(&(g_pst_mac_res->st_vap_res.st_queue), (oal_void *)(ul_loop + 1));
          /* 初始化对应的引用计数值为0 */
          g_pst_mac_res->st_vap_res.auc_user_cnt[ul_loop] = 0;
     }

}

oal_uint32  mac_res_user_init_etc(oal_void)
{
    oal_uint        ul_loop;
    oal_void       *p_user_info = OAL_PTR_NULL;
    oal_void       *p_idx       = OAL_PTR_NULL;
    oal_void       *p_user_cnt  = OAL_PTR_NULL;
    oal_uint        ul_one_user_info_size;

    /***************************************************************************
            初始化USER的资源管理内容
    ***************************************************************************/
    /* 动态申请用户资源池相关内存 */
    #if (defined(_PRE_PRODUCT_ID_HI110X_HOST))
    p_user_info = oal_memalloc(OAL_SIZEOF(hmac_user_stru) * MAC_RES_MAX_USER_LIMIT);
    #else
    p_user_info = oal_memalloc(OAL_SIZEOF(mac_res_mem_user_stru) * MAC_RES_MAX_USER_LIMIT);
    #endif
    p_idx       = oal_memalloc(OAL_SIZEOF(oal_uint) * MAC_RES_MAX_USER_LIMIT);
    p_user_cnt  = oal_memalloc(OAL_SIZEOF(oal_uint8) * MAC_RES_MAX_USER_LIMIT);
    if ((OAL_PTR_NULL == p_user_info) || (OAL_PTR_NULL == p_idx) || (OAL_PTR_NULL == p_user_cnt))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_res_user_init_etc::param null.}");

        if (OAL_PTR_NULL != p_user_info)
        {
            oal_free(p_user_info);
        }

        if (OAL_PTR_NULL != p_idx)
        {
            oal_free(p_idx);
        }

        if (OAL_PTR_NULL != p_user_cnt)
        {
            oal_free(p_user_cnt);
        }

        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }


    /* 内存初始清0 */
    #if (defined(_PRE_PRODUCT_ID_HI110X_HOST))
    OAL_MEMZERO(p_user_info, (OAL_SIZEOF(hmac_user_stru) * MAC_RES_MAX_USER_LIMIT));
    #else
    OAL_MEMZERO(p_user_info, (OAL_SIZEOF(mac_res_mem_user_stru) * MAC_RES_MAX_USER_LIMIT));
    #endif

    OAL_MEMZERO(p_idx,       (OAL_SIZEOF(oal_uint) * MAC_RES_MAX_USER_LIMIT));
    OAL_MEMZERO(p_user_cnt,  (OAL_SIZEOF(oal_uint8) * MAC_RES_MAX_USER_LIMIT));

    g_pst_mac_res->st_user_res.pul_idx        = p_idx;
    g_pst_mac_res->st_user_res.puc_user_cnt   = p_user_cnt;

    oal_queue_set_16(&(g_pst_mac_res->st_user_res.st_queue),
                  g_pst_mac_res->st_user_res.pul_idx,
                  (oal_uint16)MAC_RES_MAX_USER_LIMIT);
    g_pst_mac_res->st_user_res.us_hmac_priv_size = 0;

    ul_one_user_info_size = 0;
    g_pst_mac_res->st_user_res.past_user_info[0] = p_user_info;
    for (ul_loop = 0; ul_loop < MAC_RES_MAX_USER_LIMIT; ul_loop++)
    {
        /* 初始值保存的是对应数组下标值加1 */
        oal_queue_enqueue_16(&(g_pst_mac_res->st_user_res.st_queue), (oal_void *)(ul_loop + 1));

        /* 初始化对应的引用位置 */
        g_pst_mac_res->st_user_res.past_user_info[ul_loop]  = (oal_uint8 *)p_user_info + ul_one_user_info_size;
        #if (defined(_PRE_PRODUCT_ID_HI110X_HOST))
        ul_one_user_info_size += OAL_SIZEOF(hmac_user_stru);
        #else
        ul_one_user_info_size += OAL_SIZEOF(mac_res_mem_user_stru);
        #endif

    }

    return OAL_SUCC;
}

oal_uint32  mac_res_exit_etc(void)
{
    oal_uint ul_loop;
    oal_free(g_pst_mac_res->st_user_res.past_user_info[0]);
    oal_free(g_pst_mac_res->st_user_res.pul_idx);
    oal_free(g_pst_mac_res->st_user_res.puc_user_cnt);
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151) && (_PRE_TEST_MODE != _PRE_TEST_MODE_UT)
	oal_free(g_pst_mac_res_vap);
#endif
	for (ul_loop = 0; ul_loop < MAC_RES_MAX_USER_LIMIT; ul_loop++)
    {
       g_pst_mac_res->st_user_res.past_user_info[ul_loop]      = OAL_PTR_NULL;
    }
    g_pst_mac_res->st_user_res.pul_idx            = OAL_PTR_NULL;
    g_pst_mac_res->st_user_res.puc_user_cnt       = OAL_PTR_NULL;

    return OAL_SUCC;
}



oal_uint32  mac_res_init_etc(oal_void)
{
    oal_uint        ul_loop;
    oal_uint32      ul_ret;

    OAL_MEMZERO(g_pst_mac_res, OAL_SIZEOF(mac_res_stru));
    /***************************************************************************
            初始化DEV的资源管理内容
    ***************************************************************************/
    oal_queue_set(&(g_pst_mac_res->st_dev_res.st_queue),
                  g_pst_mac_res->st_dev_res.aul_idx,
                  MAC_RES_MAX_DEV_NUM);

    ul_ret = mac_res_check_spec_etc();
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{mac_res_init_etc::mac_res_user_init_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    for (ul_loop = 0; ul_loop < MAC_RES_MAX_DEV_NUM; ul_loop++)
    {
        /* 初始值保存的是对应数组下标值加1 */
        oal_queue_enqueue(&(g_pst_mac_res->st_dev_res.st_queue), (oal_void *)(ul_loop + 1));

        /* 初始化对应的引用计数值为0 */
        g_pst_mac_res->st_dev_res.auc_user_cnt[ul_loop] = 0;
    }

    /*lint -e413*/
    /***************************************************************************
            初始化VAP的资源管理内容
    ***************************************************************************/
    mac_res_vap_init();

    /***************************************************************************
            初始化USER的资源管理内容
            初始化HASH桶的资源管理内容
    ***************************************************************************/
    ul_ret = mac_res_user_init_etc();
    if (OAL_SUCC != ul_ret)
    {
        MAC_ERR_LOG1(0, "mac_res_init_etc: mac_res_user_init_etc return err code", ul_ret);
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{mac_res_init_etc::mac_res_user_init_etc failed[%d].}", ul_ret);

        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  mac_res_free_mac_user_etc(oal_uint16 us_idx)
{
    if (OAL_UNLIKELY(us_idx >= MAC_RES_MAX_USER_LIMIT))
    {
        return OAL_FAIL;
    }

    if(0 == g_pst_mac_res->st_user_res.puc_user_cnt[us_idx])
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY,"mac_res_free_mac_user_etc::cnt==0! idx:%d",us_idx);
        oal_dump_stack();
        return OAL_ERR_CODE_USER_RES_CNT_ZERO;
    }

    (g_pst_mac_res->st_user_res.puc_user_cnt[us_idx])--;

    if (0 != g_pst_mac_res->st_user_res.puc_user_cnt[us_idx])
    {
        return OAL_SUCC;
    }

    /* 入队索引值需要加1操作 */
    oal_queue_enqueue_16(&(g_pst_mac_res->st_user_res.st_queue), (oal_void *)((oal_uint)us_idx + 1));

    return OAL_SUCC;
}


oal_uint32  mac_res_free_mac_vap_etc(oal_uint32 ul_idx)
{
    if (OAL_UNLIKELY(ul_idx >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT))
    {
        return OAL_FAIL;
    }

    if(0 == g_pst_mac_res->st_vap_res.auc_user_cnt[ul_idx])
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY,"mac_res_free_mac_vap_etc::cnt==0! idx:%d",ul_idx);
        oal_dump_stack();
        return OAL_SUCC;
    }

    (g_pst_mac_res->st_vap_res.auc_user_cnt[ul_idx])--;

    if (0 != g_pst_mac_res->st_vap_res.auc_user_cnt[ul_idx])
    {
        return OAL_SUCC;
    }

    /* 入队索引值需要加1操作 */
    oal_queue_enqueue(&(g_pst_mac_res->st_vap_res.st_queue), (oal_void *)((oal_uint)ul_idx + 1));

    return OAL_SUCC;
}

#endif


 oal_uint32  mac_res_alloc_dmac_dev_etc(oal_uint8    *puc_dev_idx)
{
    oal_uint  ul_dev_idx_temp;

    if (OAL_UNLIKELY(OAL_PTR_NULL == puc_dev_idx))
    {
        OAL_IO_PRINT("mac_res_alloc_dmac_dev_etc: OAL_PTR_NULL == pul_dev_idx");
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_res_alloc_dmac_dev_etc::puc_dev_idx null.}");

        return OAL_FAIL;
    }

    ul_dev_idx_temp = (oal_uint)oal_queue_dequeue(&(g_pst_mac_res->st_dev_res.st_queue));

    /* 0为无效值 */
    if (0 == ul_dev_idx_temp)
    {
        OAL_IO_PRINT("mac_res_alloc_dmac_dev_etc: 0 == ul_dev_idx_temp");
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_res_alloc_dmac_dev_etc::ul_dev_idx_temp=0.}");

        return OAL_FAIL;
    }

    *puc_dev_idx = (oal_uint8)(ul_dev_idx_temp - 1);

    (g_pst_mac_res->st_dev_res.auc_user_cnt[ul_dev_idx_temp - 1])++;

    return OAL_SUCC;
}

oal_uint32  mac_res_free_dev_etc(oal_uint32 ul_dev_idx)
{
    if (OAL_UNLIKELY(ul_dev_idx >= MAC_RES_MAX_DEV_NUM))
    {
        MAC_ERR_LOG(0, "mac_res_free_dev_etc: ul_dev_idx >= MAC_RES_MAX_DEV_NUM");
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{mac_res_free_dev_etc::invalid ul_dev_idx[%d].}", ul_dev_idx);

        return OAL_FAIL;
    }

    if(0 == g_pst_mac_res->st_dev_res.auc_user_cnt[ul_dev_idx])
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY,"mac_res_free_dev_etc::cnt==0! idx:%d",ul_dev_idx);
        oal_dump_stack();
        return OAL_SUCC;
    }

    (g_pst_mac_res->st_dev_res.auc_user_cnt[ul_dev_idx])--;

    if (0 != g_pst_mac_res->st_dev_res.auc_user_cnt[ul_dev_idx])
    {
        return OAL_SUCC;
    }

    /* 入队索引值需要加1操作 */
    oal_queue_enqueue(&(g_pst_mac_res->st_dev_res.st_queue), (oal_void *)((oal_uint)ul_dev_idx + 1));

    return OAL_SUCC;
}


mac_chip_stru  *mac_res_get_mac_chip(oal_uint32 ul_chip_idx)
{
    if (OAL_UNLIKELY(ul_chip_idx >= WLAN_CHIP_MAX_NUM_PER_BOARD))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{mac_res_get_mac_chip::invalid ul_chip_idx[%d].}", ul_chip_idx);

        return OAL_PTR_NULL;
    }

    return &(g_pst_mac_board->ast_chip[ul_chip_idx]);
}


oal_uint16  mac_chip_get_max_asoc_user(oal_uint8 uc_chip_id)
{
    /* 关联单播最大用户数 */
#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
    mac_chip_stru      *pst_mac_chip;

    pst_mac_chip = mac_res_get_mac_chip(uc_chip_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_chip))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "mac_chip_get_max_asoc_user::pst_mac_chip ptr null.");
        return OAL_FAIL;
    }

    return (pst_mac_chip->st_user_extend.en_flag) ? WLAN_ASSOC_USER_MAX_NUM : (oal_uint16)g_uc_max_active_user;
#else
    return WLAN_ASSOC_USER_MAX_NUM;
#endif
}

oal_uint8  mac_chip_get_max_active_user(oal_void)
{
    /* 关联单播active最大用户数 */
    return g_uc_max_active_user;
}
oal_uint16  mac_board_get_max_user(oal_void)
{
    /* 整板单播和组播用户总数，整板单播和组播用户数直接取用宏值即可 */
    return MAC_RES_MAX_USER_LIMIT;
}

#ifdef _PRE_WLAN_WEB_CMD_COMM

oal_void  mac_res_set_hw_addr(oal_uint8 *puc_addr)
{
    oal_set_mac_addr(g_hw_addr, puc_addr);
    g_mac_bmp = 0;
}


oal_void  mac_res_set_mac_bitmap(oal_uint8 val)
{
    g_mac_bmp = g_mac_bmp | ((oal_uint64)((oal_uint64)1 << val));
}


oal_void  mac_res_clear_mac_bitmap(oal_uint8 *puc_addr)
{
    oal_uint8 uc_pos = 0;

    uc_pos = puc_addr[WLAN_MAC_ADDR_LEN - 1] - g_hw_addr[WLAN_MAC_ADDR_LEN - 1];
    g_mac_bmp = g_mac_bmp & (~(oal_uint64)((oal_uint64)1 << uc_pos));
    OAL_IO_PRINT("mac_res_clear_mac_bitmap:bitmap is %llx.\r\n", g_mac_bmp);
}


oal_int32 mac_res_get_mac_addr(oal_uint8 *puc_addr)
{
    oal_uint8 uc_pos = 0;

    OAL_IO_PRINT("mac_res_get_mac_addr:bitmap is %llx.\r\n", g_mac_bmp);
    for (; uc_pos < MAC_RES_MAC_ADDR_NO; uc_pos++)
    {
        if (0 == (g_mac_bmp & (oal_uint64)((oal_uint64)1 << uc_pos)))
        {
            oal_set_mac_addr(puc_addr, g_hw_addr);
            if (uc_pos > (255 - g_hw_addr[WLAN_MAC_ADDR_LEN - 1]))
            {
                if (0xff == puc_addr[WLAN_MAC_ADDR_LEN - 2])
                {
                    puc_addr[WLAN_MAC_ADDR_LEN - 3] += 1;
                }
                puc_addr[WLAN_MAC_ADDR_LEN - 2] += 1;
            }
            puc_addr[WLAN_MAC_ADDR_LEN - 1] += uc_pos;
            mac_res_set_mac_bitmap(uc_pos);
            return OAL_SUCC;
        }
    }

    oal_set_mac_addr_zero(puc_addr);
    OAM_ERROR_LOG0(0, OAM_SF_CFG, "{mac_res_get_mac_addr::mac resources out of range!}");
    return OAL_FAIL;
}
#endif

/*lint -e19*/
oal_module_symbol(g_pst_mac_res);
oal_module_symbol(g_us_max_asoc_user_etc);
oal_module_symbol(g_uc_max_active_user);
oal_module_symbol(mac_chip_get_max_asoc_user);
oal_module_symbol(mac_chip_get_max_active_user);
oal_module_symbol(mac_chip_get_max_multi_user);
oal_module_symbol(mac_board_get_max_user);
oal_module_symbol(mac_res_free_dev_etc);
oal_module_symbol(mac_res_alloc_hmac_dev_etc);
oal_module_symbol(mac_res_get_dev_etc);
oal_module_symbol(mac_res_get_mac_chip);
oal_module_symbol(mac_res_free_mac_user_etc);
oal_module_symbol(mac_res_free_mac_vap_etc);
/*lint +e19*/
#ifdef _PRE_WLAN_WEB_CMD_COMM
oal_module_symbol(mac_res_set_hw_addr);
oal_module_symbol(mac_res_get_mac_addr);
oal_module_symbol(mac_res_set_mac_bitmap);
oal_module_symbol(mac_res_clear_mac_bitmap);
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


