


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_11K

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_net.h"
#include "mac_frame.h"
#include "mac_resource.h"
#include "mac_ie.h"
#include "mac_vap.h"
#include "mac_user.h"
#include "frw_ext_if.h"
#include "hal_ext_if.h"
#include "mac_resource.h"
#include "wlan_types.h"
#include "dmac_main.h"
#include "dmac_ext_if.h"
#include "dmac_chan_mgmt.h"
#include "dmac_tx_bss_comm.h"
#include "dmac_scan.h"
#include "dmac_11k.h"
#include "dmac_alg.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_11K_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

/*****************************************************************************
  3 函数实现
*****************************************************************************/


oal_uint32 dmac_rrm_exit_quiet(oal_void *para)
{
    dmac_vap_stru       *pst_dmac_vap;
    mac_rrm_info_stru   *pst_rrm_info;
    dmac_user_stru      *pst_dmac_user;

    pst_dmac_vap = (dmac_vap_stru *)para;
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_exit_quiet::pst_dmac_vap null.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_rrm_info = pst_dmac_vap->pst_rrm_info;
    if (WLAN_VAP_MODE_BSS_STA == pst_dmac_vap->st_vap_base_info.en_vap_mode)
    {
        pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user((oal_uint16)(pst_dmac_vap->st_vap_base_info.uc_assoc_vap_id));
        if (OAL_PTR_NULL == pst_dmac_user)
        {
            OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_exit_quiet::pst_dmac_user null.");
            return OAL_ERR_CODE_PTR_NULL;
        }
        /* 恢复user，恢复该user的每一个tid */
        dmac_user_resume(pst_dmac_user);
    }

    /* 恢复硬件发送 */
    dmac_chan_enable_machw_tx(&pst_dmac_vap->st_vap_base_info);

    /* quiet period为0，表示不使用周期性quiet interval，静默结束后，设为IDLE状态 */
    if (pst_rrm_info->uc_quiet_period)
    {
        /* 退出quiet后对period开始倒计时，直到下一个quiet interval所在的tbtt中断到来，循环处理MAC_QUIET_STATE_PRD_DEC状态 */
        pst_rrm_info->en_quiet_state = MAC_QUIET_STATE_PRD_DEC;
    }
    else
    {
        pst_rrm_info->en_quiet_state = MAC_QUIET_STATE_IDLE;
    }
    return OAL_SUCC;
}


oal_uint32 dmac_rrm_enter_quiet(oal_void *para)
{
    mac_device_stru         *pst_mac_device;
    dmac_vap_stru           *pst_dmac_vap;
    mac_rrm_info_stru       *pst_rrm_info;
    dmac_user_stru          *pst_dmac_user = OAL_PTR_NULL;

    pst_dmac_vap = (dmac_vap_stru *)para;
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_enter_quiet::pst_dmac_vap null.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{dmac_rrm_enter_quiet::dev null, dev id[%d].}", pst_dmac_vap->st_vap_base_info.uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (WLAN_VAP_MODE_BSS_STA == pst_dmac_vap->st_vap_base_info.en_vap_mode)
    {
        pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user((oal_uint16)(pst_dmac_vap->st_vap_base_info.uc_assoc_vap_id));
        if (OAL_PTR_NULL == pst_dmac_user)
        {
            OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{dmac_rrm_enter_quiet::dmac user null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }
        /* pause tid */
        dmac_user_pause(pst_dmac_user);
    }
    pst_rrm_info = pst_dmac_vap->pst_rrm_info;

    /* 关闭硬件发送功能，禁止硬件回ack和cts */
    dmac_chan_disable_machw_tx(&(pst_dmac_vap->st_vap_base_info));

    /* 进入quiet状态，启动quiet超时定时器 */
    pst_rrm_info->en_quiet_state = MAC_QUIET_STATE_QUIET;
    FRW_TIMER_CREATE_TIMER(&pst_rrm_info->st_quiet_timer,
                               dmac_rrm_exit_quiet,
                               pst_rrm_info->us_quiet_duration,
                               pst_dmac_vap,
                               OAL_FALSE,
                               OAM_MODULE_ID_DMAC,
                               pst_mac_device->ul_core_id);
    OAM_WARNING_LOG1(0, OAM_SF_RRM, "{dmac_rrm_enter_quiet::ENTER quiet, count = [%d].",pst_rrm_info->uc_quiet_count);
    return OAL_SUCC;
}


oal_uint32 dmac_rrm_handle_quiet(dmac_vap_stru  *pst_dmac_vap)
{
    OAL_STATIC oal_uint8        uc_count = 0;
    OAL_STATIC oal_uint32       ul_tbtt_timer = 0;
    mac_device_stru             *pst_mac_device;
    mac_rrm_info_stru           *pst_rrm_info;

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_handle_quiet::pst_dmac_vap null.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{dmac_rrm_handle_quiet::dev null, dev id[%d].}", pst_dmac_vap->st_vap_base_info.uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_rrm_info = pst_dmac_vap->pst_rrm_info;
    switch(pst_rrm_info->en_quiet_state)
    {
        case MAC_QUIET_STATE_INIT:
        case MAC_QUIET_STATE_IDLE:
            break;
        /* beacon中quiet ie有更新时，进入START状态 */
        case MAC_QUIET_STATE_START:
            uc_count = pst_rrm_info->uc_quiet_count;
            ul_tbtt_timer = 0;
            /* START阶段，删除已注册的quiet和offset定时器 */
            if (pst_rrm_info->st_offset_timer.en_is_registerd)
            {
                FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_rrm_info->st_offset_timer);
            }
            if (pst_rrm_info->st_quiet_timer.en_is_registerd)
            {
                FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_rrm_info->st_quiet_timer);
            }
            /* quiet count为0时，表示该TBTT内需要静默，启动offset定时器，超时后进入quiet阶段 */
            if (0 != uc_count)
            {
                uc_count--;
                pst_rrm_info->en_quiet_state = MAC_QUIET_STATE_COUNT;
            }
            else
            {

                FRW_TIMER_CREATE_TIMER(&pst_rrm_info->st_offset_timer,
                       dmac_rrm_enter_quiet,
                       pst_rrm_info->us_quiet_offset,
                       pst_dmac_vap,
                       OAL_FALSE,
                       OAM_MODULE_ID_DMAC,
                       pst_mac_device->ul_core_id);

            }
            if (pst_rrm_info->uc_quiet_period)
            {
                ul_tbtt_timer++;
            }
            break;
        case MAC_QUIET_STATE_COUNT:
            if (0 != uc_count)
            {
                uc_count--;
                pst_rrm_info->en_quiet_state = MAC_QUIET_STATE_COUNT;
            }
            else
            {
                FRW_TIMER_CREATE_TIMER(&pst_rrm_info->st_offset_timer,
                       dmac_rrm_enter_quiet,
                       pst_rrm_info->us_quiet_offset,
                       pst_dmac_vap,
                       OAL_FALSE,
                       OAM_MODULE_ID_DMAC,
                       pst_mac_device->ul_core_id);

            }
            break;
        case MAC_QUIET_STATE_PRD_DEC:
            ul_tbtt_timer++;
            /* quiet period等于0不会进PRD_DEC状态 */
            if(0 == (ul_tbtt_timer % pst_rrm_info->uc_quiet_period))
            {

                FRW_TIMER_CREATE_TIMER(&pst_rrm_info->st_offset_timer,
                       dmac_rrm_enter_quiet,
                       pst_rrm_info->us_quiet_offset,
                       pst_dmac_vap,
                       OAL_FALSE,
                       OAM_MODULE_ID_DMAC,
                       pst_mac_device->ul_core_id);

                ul_tbtt_timer = 0;
            }
            break;
        default:
            break;
    }
    return OAL_SUCC;
}


oal_uint32 dmac_rrm_parse_quiet(dmac_vap_stru  *pst_dmac_vap, oal_netbuf_stru *pst_netbuf)
{
    oal_uint16                   us_frame_len;
    oal_bool_enum_uint8          en_is_update = OAL_FALSE;
    dmac_rx_ctl_stru            *pst_rx_ctrl;
    mac_rx_ctl_stru             *pst_rx_info;
    oal_uint8                   *puc_payload;
    mac_rrm_info_stru           *pst_rrm_info;
    mac_quiet_frm_stru          *pst_quiet_info;

    pst_rx_ctrl         = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_rx_info         = (mac_rx_ctl_stru *)(&(pst_rx_ctrl->st_rx_info));
    us_frame_len        = pst_rx_info->us_frame_len - pst_rx_info->bit_mac_header_len; /* 帧体长度 */
    puc_payload         = OAL_NETBUF_PAYLOAD(pst_netbuf);

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_parse_quiet::pst_dmac_vap null.");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_rrm_info = pst_dmac_vap->pst_rrm_info;

    /* Quiet IE */
    pst_quiet_info = (mac_quiet_frm_stru *)mac_find_ie(MAC_EID_QUIET, puc_payload + MAC_DEVICE_BEACON_OFFSET, us_frame_len - MAC_DEVICE_BEACON_OFFSET);
    if (OAL_PTR_NULL == pst_quiet_info)
    {
        pst_rrm_info->en_quiet_state = MAC_QUIET_STATE_IDLE;
        return OAL_ERR_CODE_PTR_NULL;
    }

    /***************************************************************************/
    /*                   Quiet IE                                              */
    /* ----------------------------------------------------------------------- */
    /* |Element ID|length|Quiet Count|Quiet Period|Quiet Duration|Quiet Offset|*/
    /* ----------------------------------------------------------------------- */
    /* |1         |1     |1          |1           |2             |2           |*/
    /* ----------------------------------------------------------------------- */
    /***************************************************************************/
    if (MAC_QUIET_IE_LEN != pst_quiet_info->uc_len)
    {
        OAM_WARNING_LOG1(0, OAM_SF_RRM, "{dmac_rrm_parse_quiet::illegal quiet IE len[%d].", pst_quiet_info->uc_len);
        return OAL_FAIL;
    }

    if (pst_rrm_info->uc_quiet_count != pst_quiet_info->uc_quiet_count)
    {
        pst_rrm_info->uc_quiet_count = pst_quiet_info->uc_quiet_count;
        en_is_update = OAL_TRUE;
    }

    if (pst_rrm_info->uc_quiet_period != pst_quiet_info->uc_quiet_period)
    {
        pst_rrm_info->uc_quiet_period = pst_quiet_info->uc_quiet_period;
        en_is_update = OAL_TRUE;
    }

    if (pst_rrm_info->us_quiet_duration != pst_quiet_info->us_quiet_duration)
    {
        pst_rrm_info->us_quiet_duration = pst_quiet_info->us_quiet_duration;
        en_is_update = OAL_TRUE;
    }

    if (pst_rrm_info->us_quiet_offset != pst_quiet_info->us_quiet_offset)
    {
        pst_rrm_info->us_quiet_offset = pst_quiet_info->us_quiet_offset;
        en_is_update = OAL_TRUE;
    }

    if(en_is_update)
    {
        pst_rrm_info->en_quiet_state = MAC_QUIET_STATE_START;
        OAM_WARNING_LOG2(0, OAM_SF_RRM, "{dmac_rrm_parse_quiet::QUIET IE UPDATE. old_count[%d], new_count[%d].", pst_rrm_info->uc_quiet_count,pst_quiet_info->uc_quiet_count);
    }

    return OAL_SUCC;
}


oal_uint32 dmac_rrm_proc_pwr_constraint(dmac_vap_stru  *pst_dmac_vap, oal_netbuf_stru *pst_netbuf)
{
    oal_uint16                   us_frame_len;
    oal_uint8                   *puc_payload;
    dmac_rx_ctl_stru            *pst_rx_ctrl;
    mac_rx_ctl_stru             *pst_rx_info;
    mac_regclass_info_stru      *pst_regdom_info;
    mac_pwr_constraint_frm_stru *pst_pwr_constraint;
    mac_rrm_info_stru           *pst_rrm_info;

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_RRM, "{dmac_rrm_proc_pwr_constraint::pst_dmac_vap is NULL.");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_rrm_info        = pst_dmac_vap->pst_rrm_info;

    pst_rx_ctrl         = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_rx_info         = (mac_rx_ctl_stru *)(&(pst_rx_ctrl->st_rx_info));
    us_frame_len        = pst_rx_info->us_frame_len - pst_rx_info->bit_mac_header_len; /* 帧体长度 */
    puc_payload         = OAL_NETBUF_PAYLOAD(pst_netbuf);

    /* Power Constraint IE */
    pst_pwr_constraint = (mac_pwr_constraint_frm_stru *)mac_find_ie(MAC_EID_PWRCNSTR, puc_payload + MAC_DEVICE_BEACON_OFFSET, us_frame_len - MAC_DEVICE_BEACON_OFFSET);
    if (OAL_PTR_NULL == pst_pwr_constraint)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*************************************************************************/
    /*                   Power Constraint IE                                 */
    /* --------------------------------------------------------------------- */
    /* |Element ID         |   length            |Local Power Constraint    |*/
    /* --------------------------------------------------------------------- */
    /* |1                  |1                    |1                         |*/
    /* --------------------------------------------------------------------- */
    /*************************************************************************/
    if (MAC_PWR_CONSTRAINT_LEN != pst_pwr_constraint->uc_len)
    {
        OAM_WARNING_LOG1(0, OAM_SF_RRM, "{dmac_rrm_proc_pwr_constraint::illegal pwr constraint IE len[%d].", pst_pwr_constraint->uc_len);
        return OAL_FAIL;
    }

    /* 值变化时才设置本地最大功率 */
    if (pst_rrm_info->uc_local_pwr_constraint != pst_pwr_constraint->uc_local_pwr_constraint)
    {
        pst_rrm_info->uc_local_pwr_constraint = pst_pwr_constraint->uc_local_pwr_constraint;

        if (OAL_PTR_NULL == pst_dmac_vap)
        {
            OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_parse_quiet::pst_dmac_vap null.");
            return OAL_ERR_CODE_PTR_NULL;
        }

        /* 设置管制域最大功率以控制TPC发送最大功率*/
        pst_regdom_info = mac_get_channel_num_rc_info(pst_dmac_vap->st_vap_base_info.st_channel.en_band, pst_dmac_vap->st_vap_base_info.st_channel.uc_chan_number);
        if (OAL_PTR_NULL == pst_regdom_info)
        {
            OAM_ERROR_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{dmac_rrm_proc_pwr_constraint::this channel isnot support by this country.freq_band = %d,cur_ch_num = %d}",
                                pst_dmac_vap->st_vap_base_info.st_channel.en_band, pst_dmac_vap->st_vap_base_info.st_channel.uc_chan_number);
        }
        else
        {
            if (0 == pst_rrm_info->uc_ori_max_pwr_flush_flag)
            {
                /* 要使功率约束生效，需要修改管制域最大功率，先将原始最大功率保存 */
                pst_rrm_info->uc_ori_max_reg_pwr = pst_regdom_info->uc_max_reg_tx_pwr;
                pst_rrm_info->uc_ori_max_pwr_flush_flag++;
            }
            /* 根据power constraint设置当前功率为最大功率减去pwr constraint */
            if (pst_rrm_info->uc_ori_max_reg_pwr > pst_rrm_info->uc_local_pwr_constraint)
            {
                pst_regdom_info->uc_max_reg_tx_pwr = pst_rrm_info->uc_ori_max_reg_pwr - pst_rrm_info->uc_local_pwr_constraint;
                /* 使之生效 */
                dmac_alg_cfg_channel_notify(&pst_dmac_vap->st_vap_base_info, CH_BW_CHG_TYPE_SCAN);
            }
        }
    }
    return OAL_SUCC;
}


oal_uint16  dmac_rrm_gen_random16(oal_uint32 ul_val, oal_uint8 us_rst_flag)
{
    OAL_STATIC oal_uint32 ul_rand = 0;
    if (0 != us_rst_flag)
    {
        ul_rand = ul_val;
    }
    ul_rand = ul_rand * 1664525L + 1013904223L;
    return (oal_uint16) (ul_rand >> 16);
}


oal_void dmac_rrm_get_link_req_info(dmac_vap_stru *pst_dmac_vap, oal_uint8 *puc_link_req_frame)
{
    oal_uint8       uc_offset = 0;
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_get_link_req_info::pst_dmac_vap null.");
        return;
    }
    /***********************************************************************************/
    /*                    Link Measurement Request Frame - Frame Body                  */
    /* ------------------------------------------------------------------------------- */
    /* |Category |Action |Dialog Token| Tansmit Power Used |Max Transmit Power|Opt Sub|*/
    /* --------------------------------------------------------------------------------*/
    /* |1        |1      | 1          |  1                 |       1          | var   |*/
    /* ------------------------------------------------------------------------------- */
    /*                                                                                 */
    /***********************************************************************************/
    /* 从link measurement request中获得tx pwr，保存下来 */
    uc_offset += MAC_ACTION_OFFSET_ACTION + 1;
    pst_dmac_vap->pst_rrm_info->uc_link_dialog_token    = puc_link_req_frame[uc_offset++];
    pst_dmac_vap->pst_rrm_info->c_link_tx_pwr_used      = (oal_int8)puc_link_req_frame[uc_offset++];
    pst_dmac_vap->pst_rrm_info->c_link_max_tx_pwr       = (oal_int8)puc_link_req_frame[uc_offset++];
}


oal_uint32 dmac_rrm_fill_basic_rm_rpt_action(dmac_vap_stru *pst_dmac_vap)
{
    oal_uint8                       *puc_mac_header;
    oal_uint8                       *puc_payload;
    mac_rrm_info_stru               *pst_rrm_info;
    mac_user_stru                   *pst_mac_user;

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_fill_basic_rm_rpt_action::pst_dmac_vap NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_rrm_info = pst_dmac_vap->pst_rrm_info;
    if (OAL_PTR_NULL == pst_rrm_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_fill_basic_rm_rpt_action::pst_rrm_info NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请管理帧内存 */
    pst_rrm_info->pst_rm_rpt_mgmt_buf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_LARGE_NETBUF_SIZE, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_rrm_info->pst_rm_rpt_mgmt_buf)
    {
       OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{dmac_rrm_send_rm_bcn_rpt_action::pst_mgmt_buf null.}");

       return OAL_ERR_CODE_PTR_NULL;
    }

    oal_set_netbuf_prev(pst_rrm_info->pst_rm_rpt_mgmt_buf, OAL_PTR_NULL);
    oal_set_netbuf_next(pst_rrm_info->pst_rm_rpt_mgmt_buf,OAL_PTR_NULL);

    /*************************************************************************/
    /*                        Management Frame Format                        */
    /* --------------------------------------------------------------------  */
    /* |Frame Control|Duration|DA|SA|BSSID|Sequence Control|Frame Body|FCS|  */
    /* --------------------------------------------------------------------  */
    /* | 2           |2       |6 |6 |6    |2               |0 - 2312  |4  |  */
    /* --------------------------------------------------------------------  */
    /*                                                                       */
    /*************************************************************************/

    /*************************************************************************/
    /*                Set the fields in the frame header                     */
    /*************************************************************************/
    puc_mac_header = oal_netbuf_header(pst_rrm_info->pst_rm_rpt_mgmt_buf);

    /* 帧控制字段全为0，除了type和subtype */
    mac_hdr_set_frame_control(puc_mac_header, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);

    /* 设置分片序号为0 */
    mac_hdr_set_fragment_number(puc_mac_header, 0);

    pst_mac_user = mac_res_get_mac_user(pst_dmac_vap->st_vap_base_info.uc_assoc_vap_id);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_ERROR_LOG1(0, OAM_SF_TX, "{dmac_rrm_fill_basic_rm_rpt_action::pst_mac_user[%d] null.", pst_dmac_vap->st_vap_base_info.uc_assoc_vap_id);
        oal_netbuf_free(pst_rrm_info->pst_rm_rpt_mgmt_buf);
        pst_rrm_info->pst_rm_rpt_mgmt_buf = OAL_PTR_NULL;
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置地址1，用户地址 */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR1_OFFSET, pst_mac_user->auc_user_mac_addr);

    /* 设置地址2为自己的MAC地址 */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR2_OFFSET, pst_dmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);

    /* 地址3，为VAP自己的MAC地址 */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR3_OFFSET, pst_dmac_vap->st_vap_base_info.auc_bssid);

    /*************************************************************************/
    /*                    Radio Measurement Report Frame - Frame Body        */
    /* --------------------------------------------------------------------- */
    /* |Category |Action |Dialog Token| Measurement Report Elements         |*/
    /* --------------------------------------------------------------------- */
    /* |1        |1      | 1          |  var                                 */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    puc_payload                                     = oal_netbuf_data(pst_rrm_info->pst_rm_rpt_mgmt_buf);
#else
    puc_payload                                     = puc_mac_header + MAC_80211_FRAME_LEN;
#endif
    pst_rrm_info->pst_rm_rpt_action                 = (mac_action_rm_rpt_stru *)puc_payload;
    pst_rrm_info->pst_rm_rpt_action->uc_category    = MAC_ACTION_CATEGORY_RADIO_MEASURMENT;
    pst_rrm_info->pst_rm_rpt_action->uc_action_code = MAC_RM_ACTION_RADIO_MEASUREMENT_REPORT;
    pst_rrm_info->pst_rm_rpt_action->uc_dialog_token =   pst_rrm_info->st_bcn_req_info.uc_dialog_token;

    pst_rrm_info->us_rm_rpt_action_len = 3 + MAC_80211_FRAME_LEN;
    return OAL_SUCC;
}


oal_uint32 dmac_rrm_start_scan_for_bcn_req(dmac_vap_stru *pst_dmac_vap)
{
    mac_device_stru     *pst_mac_device;
    mac_scan_req_stru   *pst_scan_req;

    if (OAL_PTR_NULL == pst_dmac_vap->pst_rrm_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_start_scan_for_bcn_req::pst_rrm_info null.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_scan_req = (mac_scan_req_stru *)pst_dmac_vap->pst_rrm_info->pst_scan_req;

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{dmac_rrm_start_scan_for_bcn_req::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    dmac_scan_handle_scan_req_entry(pst_mac_device, pst_dmac_vap, pst_scan_req);

    return OAL_SUCC;
}


oal_void dmac_rrm_encap_meas_rpt(dmac_vap_stru *pst_dmac_vap)
{
    mac_rrm_info_stru       *pst_rrm_info;
    mac_meas_rpt_bcn_stru   *pst_meas_rpt_bcn;
    mac_bcn_rpt_stru        *pst_bcn_rpt_item;
    mac_meas_rpt_ie_stru    *pst_meas_rpt_ie;
    mac_meas_rpt_bcn_item_stru *pst_meas_rpt_bcn_item;
    oal_dlist_head_stru         *pst_meas_rpt_node      = OAL_PTR_NULL;
    oal_uint8               uc_curr_beacon_item_len;

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_encap_meas_rpt_basic::pst_dmac_vap null.");
        return;
    }

    pst_rrm_info = pst_dmac_vap->pst_rrm_info;
    if (OAL_PTR_NULL == pst_rrm_info || OAL_PTR_NULL == pst_rrm_info->pst_meas_rpt_ie)
    {
        OAM_ERROR_LOG1(0, OAM_SF_RRM, "{dmac_rrm_encap_meas_rpt_basic::pst_rrm_info[0x%x] or pst_meas_rpt_ie is null.", pst_rrm_info);
        return;
    }

    pst_meas_rpt_ie = pst_rrm_info->pst_meas_rpt_ie;

    /*************************************************************************/
    /*                   Measurement Report IE - Frame Body         */
    /* --------------------------------------------------------------------- */
    /* |Element ID |Length |Meas Token| Meas Rpt Mode | Meas Type | Meas Rpt|*/
    /* --------------------------------------------------------------------- */
    /* |1          |1      | 1        |  1            | 1         | var      */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    do {
        pst_meas_rpt_ie->uc_eid       = MAC_EID_MEASREP;
        pst_meas_rpt_ie->uc_token     = pst_rrm_info->st_bcn_req_info.uc_meas_token;
        pst_meas_rpt_ie->uc_rpttype   = pst_rrm_info->st_bcn_req_info.uc_meas_type;
        OAL_MEMZERO(&(pst_meas_rpt_ie->st_rptmode), OAL_SIZEOF(mac_meas_rpt_mode_stru));
        pst_meas_rpt_ie->uc_len       = 3;

        pst_rrm_info->us_rm_rpt_action_len          += MAC_MEASUREMENT_RPT_FIX_LEN;

        if (oal_dlist_is_empty(&(pst_rrm_info->st_meas_rpt_list)))
        {
            break;
        }

        pst_meas_rpt_node = pst_rrm_info->st_meas_rpt_list.pst_next;
        pst_meas_rpt_bcn = OAL_DLIST_GET_ENTRY(pst_meas_rpt_node, mac_meas_rpt_bcn_stru, st_dlist_head);
        if (OAL_PTR_NULL == pst_meas_rpt_bcn->pst_meas_rpt_bcn_item)
        {
            break;
        }
        pst_meas_rpt_bcn_item = pst_meas_rpt_bcn->pst_meas_rpt_bcn_item;

        uc_curr_beacon_item_len = MAC_BEACON_RPT_FIX_LEN + pst_meas_rpt_bcn->ul_rpt_detail_act_len;

        if (pst_rrm_info->us_rm_rpt_action_len + uc_curr_beacon_item_len > WLAN_LARGE_NETBUF_SIZE)
        {
            return;
        }

        pst_bcn_rpt_item = (mac_bcn_rpt_stru *)(pst_meas_rpt_ie->auc_meas_rpt);
        pst_bcn_rpt_item->uc_optclass = pst_meas_rpt_bcn_item->uc_optclass;
        pst_bcn_rpt_item->uc_channum  = pst_meas_rpt_bcn_item->uc_channum;
        oal_memcopy(pst_bcn_rpt_item->auc_act_meas_start_time, pst_meas_rpt_bcn_item->auc_act_meas_start_time, 8);
        pst_bcn_rpt_item->us_duration = pst_meas_rpt_bcn_item->us_duration;
        pst_bcn_rpt_item->bit_condensed_phy_type = pst_meas_rpt_bcn_item->bit_condensed_phy_type;
        pst_bcn_rpt_item->bit_rpt_frm_type = pst_meas_rpt_bcn_item->bit_rpt_frm_type;
        pst_bcn_rpt_item->uc_rcpi = pst_meas_rpt_bcn_item->uc_rcpi;
        pst_bcn_rpt_item->uc_rsni = pst_meas_rpt_bcn_item->uc_rsni;
        oal_memcopy(pst_bcn_rpt_item->auc_bssid, pst_meas_rpt_bcn_item->auc_bssid, 6);
        pst_bcn_rpt_item->uc_antenna_id = pst_meas_rpt_bcn_item->uc_antenna_id;
        pst_bcn_rpt_item->ul_parent_tsf = pst_meas_rpt_bcn_item->ul_parent_tsf;

        /* Reported Frame Body EID&Len */
        if (pst_meas_rpt_bcn->ul_rpt_detail_act_len)
        {
            *(pst_bcn_rpt_item->auc_subelm) = 1;
            *(pst_bcn_rpt_item->auc_subelm + 1) = pst_meas_rpt_bcn->ul_rpt_detail_act_len;
            oal_memcopy((pst_bcn_rpt_item->auc_subelm + 2), pst_meas_rpt_bcn->puc_rpt_detail_data, pst_meas_rpt_bcn->ul_rpt_detail_act_len);
            uc_curr_beacon_item_len += 2;
        }
        pst_meas_rpt_ie->uc_len += uc_curr_beacon_item_len;
        pst_rrm_info->us_rm_rpt_action_len    += uc_curr_beacon_item_len;

        /* 下一个Measurement Report的位置 */
        pst_meas_rpt_ie = (mac_meas_rpt_ie_stru *)((oal_uint8 *)pst_meas_rpt_ie + pst_meas_rpt_ie->uc_len + 2);

        oal_dlist_delete_entry(pst_meas_rpt_node);
        OAL_MEM_FREE(pst_meas_rpt_bcn->puc_rpt_detail_data, OAL_TRUE);
        OAL_MEM_FREE(pst_meas_rpt_bcn_item, OAL_TRUE);
        OAL_MEM_FREE(pst_meas_rpt_bcn, OAL_TRUE);
        pst_meas_rpt_bcn->puc_rpt_detail_data = OAL_PTR_NULL;
        pst_meas_rpt_bcn_item = OAL_PTR_NULL;
        pst_meas_rpt_bcn = OAL_PTR_NULL;

    } while (!oal_dlist_is_empty(&(pst_rrm_info->st_meas_rpt_list)));
}


oal_void dmac_rrm_extract_bcn_rpt_fix_field(  dmac_vap_stru *pst_dmac_vap,
                                                        mac_meas_rpt_bcn_item_stru *pst_meas_rpt_bcn_item,
                                                        dmac_rx_ctl_stru *pst_rx_ctrl,
                                                        oal_uint8 *puc_addr
                                                    )
{
    oal_uint32                  ul_act_meas_lo;
    mac_rrm_info_stru           *pst_rrm_info;
    pst_rrm_info = pst_dmac_vap->pst_rrm_info;

    pst_meas_rpt_bcn_item->uc_eid       = MAC_EID_MEASREP;
    pst_meas_rpt_bcn_item->uc_token     = pst_rrm_info->st_bcn_req_info.uc_meas_token;
    pst_meas_rpt_bcn_item->uc_rpttype   = pst_rrm_info->st_bcn_req_info.uc_meas_type;

    pst_meas_rpt_bcn_item->uc_optclass  = pst_rrm_info->st_bcn_req_info.uc_opt_class;
    pst_meas_rpt_bcn_item->uc_channum   = pst_rx_ctrl->st_rx_info.uc_channel_number;
    /* Actual Meas Start Time */
    oal_memcopy(pst_meas_rpt_bcn_item->auc_act_meas_start_time, pst_rrm_info->auc_act_meas_start_time, 8);

    /* Meas Duration */
    if (1 == pst_rrm_info->st_bcn_req_info.st_mr_mode.bit_duration_mandatory)
    {
        pst_meas_rpt_bcn_item->us_duration = pst_rrm_info->st_bcn_req_info.us_meas_duration;
    }
    else
    {
        ul_act_meas_lo = *(oal_uint32 *)&(pst_meas_rpt_bcn_item->auc_act_meas_start_time);
        pst_meas_rpt_bcn_item->us_duration = (oal_uint16)((oal_uint32)(pst_meas_rpt_bcn_item->ul_parent_tsf + 1<<32 - ul_act_meas_lo) >> 10);
    }

    /* Rpt Frm Info */
    pst_meas_rpt_bcn_item->bit_rpt_frm_type        = 0;                /* Beacon/Probe rsp */
    pst_meas_rpt_bcn_item->bit_condensed_phy_type  = PHY_TYPE_OFDM;    /* need modify */

    pst_meas_rpt_bcn_item->uc_rcpi                 = (pst_rx_ctrl->st_rx_statistic.c_rssi_dbm + 110) << 1;
    pst_meas_rpt_bcn_item->uc_rsni                 = 0;

    /* BSSID */
    oal_set_mac_addr(pst_meas_rpt_bcn_item->auc_bssid, puc_addr);
    pst_meas_rpt_bcn_item->uc_antenna_id           = 1;
    /* Parent TSF */
    hal_vap_tsf_get_32bit(pst_dmac_vap->pst_hal_vap, (oal_uint32 *)&(pst_meas_rpt_bcn_item->ul_parent_tsf));
}


void dmac_rrm_extract_bcn_rpt_detail(dmac_vap_stru *pst_dmac_vap,
                                                oal_uint8  *puc_frame_body,
                                                oal_uint16   us_frame_len,
                                                mac_meas_rpt_bcn_stru  *pst_meas_rpt_bcn)
{
    oal_uint8                   *puc_rpt_detail_data;
    oal_int32                   l_tmp_len;
    oal_uint8                   uc_fix_len;
    mac_rrm_info_stru           *pst_rrm_info;
    oal_uint8                   uc_element_num;
    oal_uint8                   *puc_req_elements;
    oal_uint8                    uc_element_idx;
    oal_uint8                   *puc_rx_frame;
    oal_bool_enum_uint8         en_found_ie = OAL_FALSE;

    pst_rrm_info = pst_dmac_vap->pst_rrm_info;

    /* 填充Report Frame Body */
    if (0 == pst_rrm_info->st_bcn_req_info.uc_rpt_detail)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_extract_bcn_rpt_detail::rpt detail is 0}");
        return;
    }
    /* report detail内容要放在Meas Rpt中，最长不能超过Meas rpt的长度 */
    puc_rpt_detail_data = (oal_uint8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, MAC_MAX_RPT_DETAIL_LEN, OAL_TRUE);
    if (OAL_PTR_NULL == puc_rpt_detail_data)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_encap_bcn_rpt_detail::puc_rpt_detail_data NULL}");
        return;
    }

    pst_meas_rpt_bcn->puc_rpt_detail_data = puc_rpt_detail_data;

    /* 定长field拷贝 */
    uc_fix_len = MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;
    oal_memcopy(puc_rpt_detail_data, puc_frame_body, uc_fix_len);
    puc_rpt_detail_data += uc_fix_len;
    pst_meas_rpt_bcn->ul_rpt_detail_act_len = uc_fix_len;

    /* 要寻找的IE个数及IEID */
    puc_req_elements = pst_rrm_info->st_bcn_req_info.puc_reqinfo_ieid;
    uc_element_num = pst_rrm_info->st_bcn_req_info.uc_req_ie_num;

    puc_rx_frame = puc_frame_body + uc_fix_len;
    l_tmp_len = us_frame_len - MAC_80211_FRAME_LEN - uc_fix_len;

    /* 存在有多个相同IE的场景，要找全 */
    while (l_tmp_len > MAC_IE_HDR_LEN )
    {
        /* 沿着被搜索对象，依次核对是否有被搜索EID */
        for (uc_element_idx = 0; uc_element_idx < uc_element_num; uc_element_idx++)
        {
            if (puc_rx_frame[0] == puc_req_elements[uc_element_idx])
            {
                en_found_ie = OAL_TRUE;
                break;
            }
        }

        if (OAL_TRUE == en_found_ie)
        {
            if (pst_meas_rpt_bcn->ul_rpt_detail_act_len + 2 + puc_rx_frame[1] > MAC_MAX_RPT_DETAIL_LEN)
            {
                OAM_WARNING_LOG1(0, OAM_SF_RRM, "{dmac_rrm_encap_bcn_rpt_detail::rpt detail over len[%d]}", pst_meas_rpt_bcn->ul_rpt_detail_act_len + 2 + puc_rx_frame[1]);
                break;
            }
            oal_memcopy(puc_rpt_detail_data, puc_rx_frame, 2 + puc_rx_frame[1]);
            puc_rpt_detail_data += 2 + puc_rx_frame[1];
            pst_meas_rpt_bcn->ul_rpt_detail_act_len += 2 + puc_rx_frame[1];
            en_found_ie = OAL_FALSE;
        }

        l_tmp_len   -= puc_rx_frame[1] + MAC_IE_HDR_LEN;
        puc_rx_frame += puc_rx_frame[1] + MAC_IE_HDR_LEN;

        if ((l_tmp_len < MAC_IE_HDR_LEN) || (l_tmp_len < (MAC_IE_HDR_LEN + puc_rx_frame[1])))
        {
            break;
        }
    }
}

oal_void dmac_rrm_get_bcn_info_from_rx(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru  *pst_netbuf)
{
    oal_uint8                    *puc_ssid_ie = OAL_PTR_NULL;
    oal_uint8                     uc_ssid_len = 0;
    oal_uint8                   uc_tmp_len;
    dmac_rx_ctl_stru           *pst_rx_ctrl;
    mac_ieee80211_frame_stru   *pst_frame_hdr;
    oal_uint8                  *puc_frame_body;
    oal_uint16                  us_frame_len;
    //oal_uint8                  *puc_rpt_frm_len_field;
    mac_rrm_info_stru           *pst_rrm_info;
    mac_meas_rpt_bcn_stru       *pst_meas_rpt_bcn;
    mac_meas_rpt_bcn_item_stru  *pst_meas_rpt_bcn_item = OAL_PTR_NULL;
    oal_dlist_head_stru         *pst_meas_rpt_entry      = OAL_PTR_NULL;
    mac_meas_rpt_bcn_stru       *pst_meas_rpt_bcn_entry;

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_get_bcn_info_from_rx::pst_dmac_vap NULL!}");
        return;
    }

    pst_rrm_info = pst_dmac_vap->pst_rrm_info;
    if (OAL_PTR_NULL == pst_rrm_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_get_bcn_info_from_rx::pst_rrm_info NULL!}");
        return;
    }

    /* 获取该buffer的控制信息 */
    pst_rx_ctrl = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    if (OAL_PTR_NULL == pst_rx_ctrl)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_get_bcn_info_from_rx::pst_rx_ctrl NULL!}");
        return;
    }
    /* 获取帧信息 */
    pst_frame_hdr  = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr(&(pst_rx_ctrl->st_rx_info));
    puc_frame_body = MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctrl->st_rx_info), pst_netbuf);

    us_frame_len   = (oal_uint16)oal_netbuf_get_len(pst_netbuf);

    /*************************************************************************/
    /*                   Beacon Report - Frame Body                          */
    /* --------------------------------------------------------------------- */
    /* |oper class|chn num|Actual Meas Start Time|Meas Duration|Rpt Frm Info|*/
    /* --------------------------------------------------------------------- */
    /* |1         |1      |8                     |2            | 1          |*/
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* | RCPI | RSNI | BSSID | Antenna ID | Parent TSF| Optional Subelements|*/
    /* --------------------------------------------------------------------- */
    /* |1     |1     |6      |1           |4          | Var                 |*/
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    /* BSSID过滤 */
    if (!ETHER_IS_BROADCAST(pst_rrm_info->st_bcn_req_info.auc_bssid))
    {
        /* 请求中的bssid不是wildcard，则需要bssid匹配，不匹配时不上报 */
        if (MEMCMP(pst_rrm_info->st_bcn_req_info.auc_bssid, pst_frame_hdr->auc_address3, WLAN_MAC_ADDR_LEN))
        {
            return;
        }
    }

    /* 判断该BSSID的信息是否已经保存过，若已保存过，则不再保存 更新? */
    if (!oal_dlist_is_empty(&(pst_rrm_info->st_meas_rpt_list)))
    {
        OAL_DLIST_SEARCH_FOR_EACH(pst_meas_rpt_entry, &(pst_rrm_info->st_meas_rpt_list))
        {
            pst_meas_rpt_bcn_entry = OAL_DLIST_GET_ENTRY(pst_meas_rpt_entry, mac_meas_rpt_bcn_stru, st_dlist_head);
            if (0 == MEMCMP(pst_meas_rpt_bcn_entry->pst_meas_rpt_bcn_item->auc_bssid, pst_frame_hdr->auc_address3, WLAN_MAC_ADDR_LEN))
            {
                return;
            }
        }
    }

    /* SSID过滤，若请求中ssid长度为0，则不过滤 */
    if (0 != pst_rrm_info->st_bcn_req_info.uc_ssid_len)
    {
        /* 查找ssid的ie */
        uc_tmp_len = MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;
        puc_ssid_ie = mac_find_ie(MAC_EID_SSID, (puc_frame_body + uc_tmp_len), (oal_int32)(us_frame_len - uc_tmp_len));
        if ((OAL_PTR_NULL != puc_ssid_ie) && (puc_ssid_ie[1] < WLAN_SSID_MAX_LEN))
        {
            /* 获取ssid ie的长度 */
            uc_ssid_len = puc_ssid_ie[1];
            puc_ssid_ie += 2; /* 指向ssid字符串 */
        }
        else
        {
            OAM_WARNING_LOG0(0, OAM_SF_RRM, "{dmac_rrm_get_bcn_info_from_rx::find ssid fail in beacon");
            return;
        }

        /* ssid不匹配的不处理 */
        if ((pst_rrm_info->st_bcn_req_info.uc_ssid_len != uc_ssid_len )
        || (0 != MEMCMP(pst_rrm_info->st_bcn_req_info.puc_ssid, puc_ssid_ie, pst_rrm_info->st_bcn_req_info.uc_ssid_len)))
        {
            return;
        }
    }

    pst_meas_rpt_bcn = (mac_meas_rpt_bcn_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(mac_meas_rpt_bcn_stru), OAL_TRUE);
    if (OAL_PTR_NULL == pst_meas_rpt_bcn)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_get_bcn_info_from_rx::pst_meas_rpt_bcn NULL}");
        return;
    }

    OAL_MEMZERO(pst_meas_rpt_bcn, OAL_SIZEOF(mac_meas_rpt_bcn_stru));

    pst_meas_rpt_bcn_item = (mac_meas_rpt_bcn_item_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(mac_meas_rpt_bcn_item_stru), OAL_TRUE);
    if (OAL_PTR_NULL == pst_meas_rpt_bcn_item)
    {
        OAL_MEM_FREE(pst_meas_rpt_bcn, OAL_TRUE);
        pst_meas_rpt_bcn = OAL_PTR_NULL;
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_get_bcn_info_from_rx::pst_meas_rpt_bcn_item NULL}");
        return;
    }

    pst_meas_rpt_bcn->pst_meas_rpt_bcn_item = pst_meas_rpt_bcn_item;

    dmac_rrm_extract_bcn_rpt_fix_field(pst_dmac_vap, pst_meas_rpt_bcn_item, pst_rx_ctrl, pst_frame_hdr->auc_address3);

    dmac_rrm_extract_bcn_rpt_detail(pst_dmac_vap, puc_frame_body, us_frame_len, pst_meas_rpt_bcn);

    oal_dlist_add_tail(&(pst_meas_rpt_bcn->st_dlist_head), &(pst_rrm_info->st_meas_rpt_list));
}

oal_void dmac_rrm_prepare_basic_scan_params(dmac_vap_stru *pst_dmac_vap, mac_bcn_req_stru *pst_bcn_req, mac_scan_req_stru   *pst_scan_req)
{
    if (OAL_PTR_NULL == pst_dmac_vap || OAL_PTR_NULL == pst_bcn_req || OAL_PTR_NULL == pst_scan_req)
    {
        OAM_ERROR_LOG3(0, OAM_SF_RRM, "{dmac_rrm_prepare_basic_scan_params::input para NULL, dmac_vap[%x], bcn_req[%x], scan_req[%x]}",
        pst_dmac_vap, pst_bcn_req, pst_scan_req);
        return;
    }
    pst_scan_req->en_bss_type = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    pst_scan_req->en_scan_mode = WLAN_SCAN_MODE_RRM_BEACON_REQ;
    oal_set_mac_addr(pst_scan_req->auc_bssid, pst_bcn_req->auc_bssid);
    pst_scan_req->uc_bssid_num = 1;
    pst_scan_req->uc_vap_id = pst_dmac_vap->st_vap_base_info.uc_vap_id;
    pst_scan_req->uc_scan_func = MAC_SCAN_FUNC_BSS;
    pst_scan_req->uc_max_scan_count_per_channel = 1;
    pst_scan_req->uc_max_send_probe_req_count_per_channel = WLAN_DEFAULT_SEND_PROBE_REQ_COUNT_PER_CHANNEL;
    pst_scan_req->uc_probe_delay = 0;
    oal_set_mac_addr(pst_scan_req->auc_sour_mac_addr, pst_dmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);
}

oal_uint32 dmac_rrm_meas_bcn(dmac_vap_stru *pst_dmac_vap, mac_bcn_req_stru *pst_bcn_req, mac_scan_req_stru   *pst_scan_req)
{
    mac_rrm_info_stru               *pst_rrm_info;
    mac_vap_rrm_trans_req_info_stru  st_trans_req_info;

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_meas_bcn::pst_dmac_vap NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_rrm_info = pst_dmac_vap->pst_rrm_info;

    /* 根据模式进行测量 */
    switch (pst_bcn_req->en_mode)
    {
        /* Passive:触发扫描，不发probe req，测量收到的Beacon和probe rsp */
        case RM_BCN_REQ_MEAS_MODE_PASSIVE:
            if (OAL_FALSE == pst_dmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.en_dot11RMBeaconPassiveMeasurementActivated)
            {
                if ( OAL_SUCC != dmac_rrm_fill_basic_rm_rpt_action(pst_dmac_vap))
                {
                    return OAL_FAIL;
                }

                OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "passive measurement mode not enable");
                dmac_rrm_encap_meas_rpt(pst_dmac_vap);
                pst_rrm_info->pst_meas_rpt_ie->st_rptmode.bit_incapable = 1;
                dmac_rrm_send_rm_rpt_action(pst_dmac_vap);
                return OAL_FAIL;
            }

            dmac_rrm_prepare_basic_scan_params(pst_dmac_vap, pst_bcn_req, pst_scan_req);

            pst_scan_req->en_scan_type = WLAN_SCAN_TYPE_PASSIVE;
            /* 测量时间, 需要足够长的时间以保证可以收到指定beacon */
            pst_scan_req->us_scan_time = 1200;//WLAN_DEFAULT_PASSIVE_SCAN_TIME;

            dmac_rrm_start_scan_for_bcn_req(pst_dmac_vap);

            break;
        /* Active:触发扫描，发probe req，测量收到的Beacon和probe rsp */
        case RM_BCN_REQ_MEAS_MODE_ACTIVE:
            if (OAL_FALSE == pst_dmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.en_dot11RMBeaconActiveMeasurementActivated)
            {
                if ( OAL_SUCC != dmac_rrm_fill_basic_rm_rpt_action(pst_dmac_vap))
                {
                    return OAL_FAIL;
                }
                OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "active measurement mode not enable");
                dmac_rrm_encap_meas_rpt(pst_dmac_vap);
                pst_rrm_info->pst_meas_rpt_ie->st_rptmode.bit_incapable = 1;
                dmac_rrm_send_rm_rpt_action(pst_dmac_vap);
                return OAL_FAIL;
            }

            dmac_rrm_prepare_basic_scan_params(pst_dmac_vap, pst_bcn_req, pst_scan_req);
            pst_scan_req->en_scan_type = WLAN_SCAN_TYPE_ACTIVE;
            /* 测量时间 TBD*/
            pst_scan_req->us_scan_time = WLAN_DEFAULT_ACTIVE_SCAN_TIME;

            dmac_rrm_start_scan_for_bcn_req(pst_dmac_vap);
            break;
        /* Table:上报保存的扫描结果 */
        case RM_BCN_REQ_MEAS_MODE_TABLE:
            if (OAL_FALSE == pst_dmac_vap->bit_bcn_table_switch)
            {
                OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "dmac_rrm_meas_bcn:table mode is shutdown!");
                return OAL_FAIL;
            }

            if (OAL_FALSE == pst_dmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.en_dot11RMBeaconTableMeasurementActivated)
            {
                if ( OAL_SUCC != dmac_rrm_fill_basic_rm_rpt_action(pst_dmac_vap))
                {
                    return OAL_FAIL;
                }
                OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "table measurement mode not enable");
                dmac_rrm_encap_meas_rpt(pst_dmac_vap);
                pst_rrm_info->pst_meas_rpt_ie->st_rptmode.bit_incapable = 1;
                dmac_rrm_send_rm_rpt_action(pst_dmac_vap);
                return OAL_FAIL;
            }
            oal_memcopy(st_trans_req_info.auc_ssid, pst_rrm_info->st_bcn_req_info.puc_ssid, pst_rrm_info->st_bcn_req_info.uc_ssid_len);
            st_trans_req_info.us_ssid_len = (oal_uint16)pst_rrm_info->st_bcn_req_info.uc_ssid_len;
            st_trans_req_info.uc_action_dialog_token = pst_rrm_info->st_bcn_req_info.uc_dialog_token;
            st_trans_req_info.uc_meas_token = pst_rrm_info->st_bcn_req_info.uc_meas_token;
            st_trans_req_info.uc_oper_class = pst_rrm_info->st_bcn_req_info.uc_opt_class;
            dmac_send_sys_event(&(pst_dmac_vap->st_vap_base_info), WLAN_CFGID_REQ_SAVE_BSS_INFO, OAL_SIZEOF(st_trans_req_info), (oal_uint8 *)&st_trans_req_info);

            break;
        default:
            break;

    }
    return OAL_SUCC;
}


oal_uint32 dmac_rrm_get_bcn_rpt_channels(mac_bcn_req_stru *pst_bcn_req, mac_ap_chn_rpt_stru **ppst_ap_chn_rpt, oal_uint8 uc_ap_chn_rpt_num, mac_scan_req_stru *pst_scan_req)
{
    oal_uint8                       uc_chan_idx;
    oal_uint8                       uc_chan_num;
    oal_uint8                       uc_chan_avail_idx = 0;
    oal_uint8                       uc_ap_chan_num;
    oal_uint8                       uc_ap_chan_idx;
    oal_uint8                       uc_ap_chan_rpt_count;
    wlan_channel_band_enum_uint8    en_chan_band;
    oal_uint8                       uc_chan_count;

    /* 对应指定用例进行打桩 */
    if (12 == pst_bcn_req->uc_optclass)
    {
        en_chan_band = WLAN_BAND_2G;
        uc_chan_count = (oal_uint8)MAC_CHANNEL_FREQ_2_BUTT;
    }
    else
    {
        en_chan_band = WLAN_BAND_5G;
        uc_chan_count = (oal_uint8)MAC_CHANNEL_FREQ_5_BUTT;
    }

    /* 当前operating class下的所有chan，先不处理regclass，使用2.4G信道总集 */
    if (0 == pst_bcn_req->uc_channum)
    {
        /* 根据channel_bitmap解析出对应的信道号集合 */
        for (uc_chan_idx = 0; uc_chan_idx < uc_chan_count; uc_chan_idx++)
        {
            mac_get_channel_num_from_idx(en_chan_band, uc_chan_idx, &uc_chan_num);
            pst_scan_req->ast_channel_list[uc_chan_avail_idx].uc_chan_number = uc_chan_num;
            pst_scan_req->ast_channel_list[uc_chan_avail_idx].en_band        = en_chan_band;
            pst_scan_req->ast_channel_list[uc_chan_avail_idx++].uc_idx       = uc_chan_idx;
        }
        pst_scan_req->uc_channel_nums = uc_chan_avail_idx;
    }
    /* 当前operating class与AP chan rpt的交集 */
    else if (255 == pst_bcn_req->uc_channum)
    {
        if (0 == uc_ap_chn_rpt_num)
        {
            OAM_WARNING_LOG0(0, OAM_SF_RRM, "dmac_rrm_get_bcn_rpt_channels: channum is 255,but NO ap_chan_rpt ie");
            return OAL_FAIL;
        }

        for (uc_ap_chan_rpt_count = 0; uc_ap_chan_rpt_count < uc_ap_chn_rpt_num; uc_ap_chan_rpt_count++)
        {
            /* 请求的信道个数，根据长度计算 */
            uc_ap_chan_num = ppst_ap_chn_rpt[uc_ap_chan_rpt_count]->uc_length - 1;

            for (uc_chan_idx = 0; uc_chan_idx < uc_ap_chan_num; uc_chan_idx++)
            {
                uc_chan_num = *(&(ppst_ap_chn_rpt[uc_ap_chan_rpt_count]->auc_chan[0]) + uc_chan_idx);
                en_chan_band = mac_get_band_by_channel_num(uc_chan_num);
                mac_get_channel_idx_from_num(en_chan_band, uc_chan_num, &uc_ap_chan_idx);
                pst_scan_req->ast_channel_list[uc_chan_avail_idx].uc_chan_number = uc_chan_num;
                pst_scan_req->ast_channel_list[uc_chan_avail_idx].en_band        = en_chan_band;
                pst_scan_req->ast_channel_list[uc_chan_avail_idx++].uc_idx       = uc_ap_chan_idx;
            }
        }
        pst_scan_req->uc_channel_nums = uc_chan_avail_idx;
    }
    /* 当前chan num */
    else
    {
        uc_chan_num = pst_bcn_req->uc_channum;
        pst_scan_req->uc_channel_nums = 1;
        mac_get_channel_idx_from_num(en_chan_band, uc_chan_num, &uc_ap_chan_idx);
        /* 临时调试，需要更新管制类表格 */
        pst_scan_req->ast_channel_list[0].uc_chan_number = uc_chan_num;
        pst_scan_req->ast_channel_list[0].uc_idx         = uc_ap_chan_idx;
        pst_scan_req->ast_channel_list[0].en_band        = en_chan_band;
    }
    return OAL_SUCC;
}


oal_void dmac_rrm_parse_beacon_req(dmac_vap_stru *pst_dmac_vap, mac_meas_req_ie_stru  *pst_meas_req_ie)
{
    mac_bcn_req_stru               *pst_bcn_req;
    mac_ap_chn_rpt_stru            *apst_ap_chn_rpt[MAC_11K_SUPPORT_AP_CHAN_RPT_NUM];
    oal_uint8                       uc_ap_chn_rpt_count = 0;
    oal_int8                        c_bcn_req_sub_len;
    oal_uint8                      *puc_ap_ch_rpt_search_addr;
    oal_int8                        c_ap_ch_rpt_search_len;
    oal_uint8                      *puc_rpt_detail_search_addr;
    oal_uint8                      *puc_reporting_detail;
    oal_uint8                      *puc_reqinfo_search_addr;
    oal_uint8                      *puc_ssid_search_addr;
    oal_uint8                      *puc_ssid_sub_element;
    oal_uint8                      *puc_reqinfo;
    mac_scan_req_stru               st_scan_req;
    mac_rrm_info_stru               *pst_rrm_info;

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_parse_beacon_req::pst_dmac_vap NULL!}");
        return;
    }

    pst_rrm_info = pst_dmac_vap->pst_rrm_info;

    OAL_MEMZERO(&st_scan_req, OAL_SIZEOF(mac_scan_req_stru));
    pst_rrm_info->pst_scan_req = &st_scan_req;

    pst_bcn_req = (mac_bcn_req_stru *)&(pst_meas_req_ie->auc_meas_req[0]);

    pst_bcn_req->us_random_ivl = OAL_NTOH_16(pst_bcn_req->us_random_ivl);
    pst_bcn_req->us_duration = OAL_NTOH_16(pst_bcn_req->us_duration);

    oal_memcopy(pst_rrm_info->st_bcn_req_info.auc_bssid, pst_bcn_req->auc_bssid, WLAN_MAC_ADDR_LEN);

    /*************************************************************************/
    /*                    Beacon Request                                     */
    /* --------------------------------------------------------------------- */
    /* |Operating Class |Channel Number |Rand Interval| Meas Duration       |*/
    /* --------------------------------------------------------------------- */
    /* |1               |1              | 2           | 2                   |*/
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* |Meas Mode       |BSSID          |Optional Subelements               |*/
    /* --------------------------------------------------------------------- */
    /* |1               |6              | var                               |*/
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    /* 从Subelements中获取被测信道集合,AP Channel Report可能会有多个 */
    c_bcn_req_sub_len = pst_meas_req_ie->uc_len - 16;
    if (c_bcn_req_sub_len <= 0)
    {
        dmac_rrm_get_bcn_rpt_channels(pst_bcn_req, apst_ap_chn_rpt,  uc_ap_chn_rpt_count, &st_scan_req);
    }
    else
    {
        puc_ap_ch_rpt_search_addr = pst_bcn_req->auc_subelm;
        c_ap_ch_rpt_search_len = c_bcn_req_sub_len;
        do {
            apst_ap_chn_rpt[uc_ap_chn_rpt_count] = (mac_ap_chn_rpt_stru *)mac_find_ie(MAC_EID_AP_CHAN_REPORT, puc_ap_ch_rpt_search_addr, c_ap_ch_rpt_search_len);
            if (OAL_PTR_NULL == apst_ap_chn_rpt[uc_ap_chn_rpt_count])
            {
                break;
            }
            puc_ap_ch_rpt_search_addr = (oal_uint8 *)apst_ap_chn_rpt[uc_ap_chn_rpt_count] + 2 + apst_ap_chn_rpt[uc_ap_chn_rpt_count]->uc_length;
            c_ap_ch_rpt_search_len = c_bcn_req_sub_len - ((oal_uint8 *)apst_ap_chn_rpt[uc_ap_chn_rpt_count] - pst_bcn_req->auc_subelm) - (2 + apst_ap_chn_rpt[uc_ap_chn_rpt_count]->uc_length);
            uc_ap_chn_rpt_count++;
            if (MAC_11K_SUPPORT_AP_CHAN_RPT_NUM == uc_ap_chn_rpt_count)
            {
                OAM_WARNING_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "ap chan rpt num is larger than 8, truncate the later ones");
                break;
            }
        }while (c_ap_ch_rpt_search_len > 0);

        dmac_rrm_get_bcn_rpt_channels(pst_bcn_req, apst_ap_chn_rpt,  uc_ap_chn_rpt_count, &st_scan_req);

        /* 获取SSID */
        puc_ssid_search_addr = pst_bcn_req->auc_subelm;
        puc_ssid_sub_element = mac_find_ie(0, puc_ssid_search_addr, c_bcn_req_sub_len);
        if (OAL_PTR_NULL != puc_ssid_sub_element)
        {
            pst_rrm_info->st_bcn_req_info.uc_ssid_len = *(puc_ssid_sub_element + 1);
            if (0 != pst_rrm_info->st_bcn_req_info.uc_ssid_len)
            {
                if (pst_rrm_info->st_bcn_req_info.uc_ssid_len > (WLAN_SSID_MAX_LEN - 1))
                {
                    OAM_ERROR_LOG1(0, OAM_SF_RRM, "{dmac_rrm_parse_beacon_req::ssid_len(%d) invalid}", pst_rrm_info->st_bcn_req_info.uc_ssid_len);
                    return;
                }
                pst_rrm_info->st_bcn_req_info.puc_ssid = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, pst_rrm_info->st_bcn_req_info.uc_ssid_len, OAL_TRUE);
                if (OAL_PTR_NULL == pst_rrm_info->st_bcn_req_info.puc_ssid)
                {
                    OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_parse_beacon_req::memalloc ssid fail}");
                    return;
                }

                oal_memcopy(pst_rrm_info->st_bcn_req_info.puc_ssid, (puc_ssid_sub_element + 2), pst_rrm_info->st_bcn_req_info.uc_ssid_len);
            }
        }

        /* 获取Reporting detail */
        puc_rpt_detail_search_addr = pst_bcn_req->auc_subelm;
        puc_reporting_detail = mac_find_ie(2, puc_rpt_detail_search_addr, c_bcn_req_sub_len);
        if (OAL_PTR_NULL != puc_reporting_detail)
        {
            pst_rrm_info->st_bcn_req_info.uc_rpt_detail = *(puc_reporting_detail + 2);
        }

        /* 获取ReqInfo */
        puc_reqinfo_search_addr = pst_bcn_req->auc_subelm;
        puc_reqinfo = mac_find_ie(MAC_EID_REQINFO, puc_reqinfo_search_addr, c_bcn_req_sub_len);
        if (OAL_PTR_NULL != puc_reqinfo)
        {
            pst_rrm_info->st_bcn_req_info.uc_req_ie_num = *(puc_reqinfo + 1);
            pst_rrm_info->st_bcn_req_info.puc_reqinfo_ieid = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, pst_rrm_info->st_bcn_req_info.uc_req_ie_num, OAL_TRUE);
            if (OAL_PTR_NULL == pst_rrm_info->st_bcn_req_info.puc_reqinfo_ieid)
            {
                OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_parse_beacon_req::memalloc reqinfo_ieid fail}");
                return;
            }
            oal_memcopy(pst_rrm_info->st_bcn_req_info.puc_reqinfo_ieid, (puc_reqinfo + 2), pst_rrm_info->st_bcn_req_info.uc_req_ie_num);
        }
    }

    pst_rrm_info->st_bcn_req_info.uc_opt_class = pst_bcn_req->uc_optclass;
    pst_rrm_info->st_bcn_req_info.us_meas_duration = pst_bcn_req->us_duration;

    dmac_rrm_meas_bcn(pst_dmac_vap, pst_bcn_req, &st_scan_req);
}


oal_uint32 dmac_rrm_send_rm_rpt_action(dmac_vap_stru* pst_dmac_vap)
{
    mac_tx_ctl_stru                 *pst_tx_ctl;
    oal_netbuf_stru                 *pst_mgmt_buf;
    oal_uint32                      ul_ret = OAL_SUCC;

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_send_rm_rpt_action::pst_dmac_vap NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_PTR_NULL == pst_dmac_vap->pst_rrm_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_send_rm_rpt_action::pst_rrm_info NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mgmt_buf = pst_dmac_vap->pst_rrm_info->pst_rm_rpt_mgmt_buf;
    if (OAL_PTR_NULL == pst_mgmt_buf)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_send_rm_rpt_action::pst_mgmt_buf NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填写netbuf的cb字段，供发送管理帧和发送完成接口使用 */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_mgmt_buf);
    OAL_MEMZERO(pst_tx_ctl, sizeof(mac_tx_ctl_stru));

    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)  =(oal_uint8)pst_dmac_vap->st_vap_base_info.uc_assoc_vap_id;
    mac_set_cb_ac(pst_tx_ctl, WLAN_WME_AC_MGMT);

    /* 调用发送管理帧接口 */
    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_mgmt_buf, pst_dmac_vap->pst_rrm_info->us_rm_rpt_action_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_mgmt_buf);
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{dmac_rrm_send_link_meas_rpt_action::tx link meas rpt action failed.}");
        return ul_ret;
    }

    return ul_ret;
}


oal_void dmac_rrm_encap_and_send_bcn_rpt(dmac_vap_stru* pst_dmac_vap)
{
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{dmac_rrm_encap_and_send_bcn_rpt::pst_dmac_vap is NULL}");
        return;
    }

    do {
        if ( OAL_SUCC != dmac_rrm_fill_basic_rm_rpt_action(pst_dmac_vap))
        {
            break;
        }
        pst_dmac_vap->pst_rrm_info->pst_meas_rpt_ie   = (mac_meas_rpt_ie_stru *)pst_dmac_vap->pst_rrm_info->pst_rm_rpt_action->auc_rpt_ies;

        dmac_rrm_encap_meas_rpt(pst_dmac_vap);

        dmac_rrm_send_rm_rpt_action(pst_dmac_vap);
    }while (!oal_dlist_is_empty(&(pst_dmac_vap->pst_rrm_info->st_meas_rpt_list)));
}

oal_void dmac_rrm_proc_rm_request(dmac_vap_stru* pst_dmac_vap, oal_netbuf_stru *pst_netbuf)
{
    mac_action_rm_req_stru          *pst_rm_req;
    mac_meas_req_ie_stru            *pst_meas_req_ie;
    dmac_rx_ctl_stru                *pst_rx_ctrl;
    oal_uint16                      us_framebody_len;
    mac_rrm_info_stru               *pst_rrm_info;
    oal_uint8                      *puc_data;

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_proc_rm_request::pst_dmac_vap NULL!}");
        return;
    }

    pst_rrm_info = pst_dmac_vap->pst_rrm_info;
    if (OAL_PTR_NULL == pst_rrm_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_proc_rm_request::pst_rrm_info is NULL}");
        return;
    }

    /* 获取帧体指针 */
    puc_data = OAL_NETBUF_PAYLOAD(pst_netbuf);

    pst_rx_ctrl      = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    if (OAL_PTR_NULL == pst_rx_ctrl)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_proc_rm_request::pst_rx_ctrl is NULL}");
        return;
    }
    us_framebody_len = pst_rx_ctrl->st_rx_info.us_frame_len - pst_rx_ctrl->st_rx_info.bit_mac_header_len;

    /*************************************************************************/
    /*                    Radio Measurement Request Frame - Frame Body       */
    /* --------------------------------------------------------------------- */
    /* |Category |Action |Dialog Token| Number of Repetitions|Meas Req Eles |*/
    /* --------------------------------------------------------------------- */
    /* |1        |1      | 1          | 2                    |var            */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    pst_rm_req = (mac_action_rm_req_stru *)(puc_data + MAC_ACTION_OFFSET_ACTION + 1);

    pst_rrm_info->st_bcn_req_info.uc_dialog_token = pst_rm_req->uc_dialog_token;

    /* 重复测试次数暂不处理 */
    pst_rrm_info->st_bcn_req_info.us_repetition = OAL_NTOH_16(pst_rm_req->us_num_rpt);

    /* 是否有Meas Req */
    if (us_framebody_len <= 5)
    {
        /* 如果没有MR IE，也回一个不带Meas Rpt的Radio Meas Rpt */
        /* 申请管理帧内存并填充头部信息 */
        if ( OAL_SUCC != dmac_rrm_fill_basic_rm_rpt_action(pst_dmac_vap))
        {
            return;
        }
        dmac_rrm_send_rm_rpt_action(pst_dmac_vap);
        return;
    }

    pst_meas_req_ie = (mac_meas_req_ie_stru *)&(pst_rm_req->auc_req_ies[0]);

    /*************************************************************************/
    /*                    Measurement Request IE                             */
    /* --------------------------------------------------------------------- */
    /* |Element ID |Length |Meas Token| Meas Req Mode|Meas Type  | Meas Req |*/
    /* --------------------------------------------------------------------- */
    /* |1          |1      | 1        | 1            |1          |var       |*/
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    /* TO BE DONE:可能有多个Measurement Req IEs */
    if (MAC_EID_MEASREQ == pst_meas_req_ie->uc_eid)
    {
        /* Req中不允许发送对应的report */
        if ((1 == pst_meas_req_ie->st_reqmode.bit_enable) && (0 == pst_meas_req_ie->st_reqmode.bit_rpt))
        {
            OAM_WARNING_LOG0(0, OAM_SF_RRM, "{rpt now allowed!}");
            return;
        }

        pst_rrm_info->st_bcn_req_info.uc_meas_token = pst_meas_req_ie->uc_token;
        pst_rrm_info->st_bcn_req_info.uc_meas_type  = pst_meas_req_ie->uc_reqtype;

        /* 只处理beacon req */
        if (5 != pst_meas_req_ie->uc_reqtype)
        {
            if ( OAL_SUCC != dmac_rrm_fill_basic_rm_rpt_action(pst_dmac_vap))
            {
                oal_netbuf_free(pst_rrm_info->pst_rm_rpt_mgmt_buf);
                pst_rrm_info->pst_rm_rpt_mgmt_buf = OAL_PTR_NULL;
                return;
            }
            pst_rrm_info->pst_meas_rpt_ie                           = (mac_meas_rpt_ie_stru *)pst_rrm_info->pst_rm_rpt_action->auc_rpt_ies;
            pst_rrm_info->pst_meas_rpt_ie->st_rptmode.bit_incapable = 1;

            dmac_rrm_encap_meas_rpt(pst_dmac_vap);
            dmac_rrm_send_rm_rpt_action(pst_dmac_vap);
            return;
        }

        /* 处理Beacon req */
        dmac_rrm_parse_beacon_req(pst_dmac_vap, pst_meas_req_ie);
    }
    /* MR IE错误，不回，报错 */
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_RRM, "{Error Request, Expect Measurement Request, but got EID[%d]!}", pst_meas_req_ie->uc_eid);
    }
    return;
}


oal_uint32    dmac_rrm_encap_link_meas_rpt(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_netbuf, oal_netbuf_stru *pst_rx_netbuf)
{
    oal_uint8                     uc_offset = 0;
    oal_uint8                    *puc_mac_header;
    oal_uint8                    *puc_payload;
    mac_user_stru                *pst_mac_user;
    dmac_rx_ctl_stru             *pst_cb_ctrl;

    mac_regclass_info_stru       *pst_regdom_info;
    oal_uint8                     uc_tx_pow;

    /*************************************************************************/
    /*                        Management Frame Format                        */
    /* --------------------------------------------------------------------  */
    /* |Frame Control|Duration|DA|SA|BSSID|Sequence Control|Frame Body|FCS|  */
    /* --------------------------------------------------------------------  */
    /* | 2           |2       |6 |6 |6    |2               |0 - 2312  |4  |  */
    /* --------------------------------------------------------------------  */
    /*                                                                       */
    /*************************************************************************/

    /*************************************************************************/
    /*                Set the fields in the frame header                     */
    /*************************************************************************/
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_encap_link_meas_rpt::pst_dmac_vap NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    puc_mac_header = oal_netbuf_header(pst_netbuf);

    /* 帧控制字段全为0，除了type和subtype */
    mac_hdr_set_frame_control(puc_mac_header, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);

    /* 设置分片序号为0 */
    mac_hdr_set_fragment_number(puc_mac_header, 0);

    pst_mac_user = mac_res_get_mac_user(pst_dmac_vap->st_vap_base_info.uc_assoc_vap_id);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_ERROR_LOG1(0, OAM_SF_TX, "{dmac_rrm_fill_basic_rm_rpt_action::pst_mac_user[%d] null.", pst_dmac_vap->st_vap_base_info.uc_assoc_vap_id);

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置地址1，用户地址 */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR1_OFFSET, pst_mac_user->auc_user_mac_addr);

    /* 设置地址2为自己的MAC地址 */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR2_OFFSET, pst_dmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);

    /* 地址3，为VAP自己的MAC地址 */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR3_OFFSET, pst_dmac_vap->st_vap_base_info.auc_bssid);

    /*************************************************************************/
    /*                Set the fields in the frame body                     */
    /*************************************************************************/

    /*************************************************************************/
    /*                    Link Measurement Report Frame - Frame Body         */
    /* --------------------------------------------------------------------- */
    /* |Category |Action |Dialog Token| TPC Report Element|Rx Antenna ID     */
    /* --------------------------------------------------------------------- */
    /* |1        |1      | 1          |  4                |1                 */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* |Tx Antenna ID |RCPI |RSNI| Opt Subelement                            */
    /* --------------------------------------------------------------------- */
    /* |1             |1    | 1  |  Var                                      */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    puc_payload                                     = oal_netbuf_data(pst_netbuf);
#else
    puc_payload                                     = puc_mac_header + MAC_80211_FRAME_LEN;
#endif

    /* 设置Action的Category   */
    puc_payload[uc_offset++] = MAC_ACTION_CATEGORY_RADIO_MEASURMENT;

    /* 设置Link Measurement Report Action Field */
    puc_payload[uc_offset++] = MAC_RM_ACTION_LINK_MEASUREMENT_REPORT;

    /* Dialog Token,与LM Req中相同 */
    puc_payload[uc_offset++]= pst_dmac_vap->pst_rrm_info->uc_link_dialog_token;

    /* TPC Report，管理帧， 2G:1M  5G:6M，查找功率速率表g_auc_rate_pow_table_margin */
    /********************************************************************************
                -------------------------------------------------
                |ElementID  |Length  |TransmitPower  |LinkMargin|
                -------------------------------------------------
       Octets:  |1          |1       |1              |1         |
                -------------------------------------------------

    TransimitPower, 此帧的传送功率，以dBm为单位
    ********************************************************************************/
    puc_payload[uc_offset++] = MAC_EID_TPCREP;
    puc_payload[uc_offset++] = MAC_TPCREP_IE_LEN;

    /* Tx Pwr，根据power constraint值的改变而改变 */

    pst_regdom_info = mac_get_channel_num_rc_info(pst_dmac_vap->st_vap_base_info.st_channel.en_band, pst_dmac_vap->st_vap_base_info.st_channel.uc_chan_number);
    dmac_alg_get_mgmt_tx_pow(pst_mac_user, pst_dmac_vap->st_vap_base_info.st_channel.en_band, &uc_tx_pow);
    puc_payload[uc_offset++] = OAL_MIN(uc_tx_pow, OAL_MIN(pst_regdom_info->uc_max_reg_tx_pwr, pst_regdom_info->uc_max_tx_pwr));

    /* Margin */
    puc_payload[uc_offset++] = 0;

    /* Rx Antenna ID */
    puc_payload[uc_offset++]= 1;

    /* Tx Antenna ID */
    puc_payload[uc_offset++]= 1;

    pst_cb_ctrl = (dmac_rx_ctl_stru*)oal_netbuf_cb(pst_rx_netbuf);

    /* RCPI */
    puc_payload[uc_offset++]= (pst_cb_ctrl->st_rx_statistic.c_rssi_dbm + 110) << 1;

    /* RSNI */
    puc_payload[uc_offset++]= 0;

    return (oal_uint32)(uc_offset + MAC_80211_FRAME_LEN);
}


oal_void dmac_rrm_send_link_meas_rpt_action(dmac_vap_stru *pst_dmac_vap, oal_netbuf_stru *pst_rx_netbuf)
{
    oal_netbuf_stru        *pst_mgmt_buf;
    oal_uint16              us_mgmt_len;
    mac_tx_ctl_stru        *pst_tx_ctl;
    oal_uint32              ul_ret;

    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{dmac_rrm_send_link_meas_rpt_action::pst_dmac_vap null.}");
        return;
    }

    if (OAL_TRUE != pst_dmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.en_dot11RMLinkMeasurementActivated)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{dmac_rrm_send_link_meas_rpt_action::Link Meas not Activated.}");

        return;
    }

    /* 申请管理帧内存 */
    pst_mgmt_buf = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, WLAN_MGMT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_HIGH);
    if (OAL_PTR_NULL == pst_mgmt_buf)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{dmac_rrm_send_link_meas_rpt_action::pst_mgmt_buf null.}");

        return;
    }

    oal_set_netbuf_prev(pst_mgmt_buf, OAL_PTR_NULL);
    oal_set_netbuf_next(pst_mgmt_buf,OAL_PTR_NULL);
    OAL_MEM_NETBUF_TRACE(pst_mgmt_buf, OAL_TRUE);

    /* 封装 Link Measurement Report 帧 */
    us_mgmt_len = (oal_uint16)dmac_rrm_encap_link_meas_rpt(pst_dmac_vap, pst_mgmt_buf, pst_rx_netbuf);
    if (0 == us_mgmt_len)
    {
        oal_netbuf_free(pst_mgmt_buf);
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{dmac_rrm_send_link_meas_rpt_action::encap link meas rpt action failed.}");
        return;
    }

    /* 填写netbuf的cb字段，供发送管理帧和发送完成接口使用 */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_mgmt_buf);

    OAL_MEMZERO(pst_tx_ctl, sizeof(mac_tx_ctl_stru));
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)  =(oal_uint8)pst_dmac_vap->st_vap_base_info.uc_assoc_vap_id;
    mac_set_cb_ac(pst_tx_ctl, WLAN_WME_AC_MGMT);

    /* 调用发送管理帧接口 */
    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_mgmt_buf, us_mgmt_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_mgmt_buf);
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{dmac_rrm_send_link_meas_rpt_action::tx link meas rpt action failed.}");
        return;
    }
    return;
}

#endif//_PRE_WLAN_FEATURE_11K
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

