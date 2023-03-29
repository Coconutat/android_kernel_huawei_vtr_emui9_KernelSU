/*
 *
 * All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses. You may choose this file to be licensed under the terms
 * of the GNU General Public License (GPL) Version 2 or the 2-clause
 * BSD license listed below:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */



/******************************************************************************
   2 外部函数变量声明
******************************************************************************/
#include "product_config.h"
/******************************************************************************
   1 头文件包含
******************************************************************************/
#include "PPP/Inc/ppp_public.h"
#include "PPP/Inc/pppid.h"
#include "PPP/Inc/layer.h"
#include "PPP/Inc/ppp_mbuf.h"
#include "PPP/Inc/hdlc.h"
#include "PPP/Inc/throughput.h"
#include "PPP/Inc/proto.h"
#include "PPP/Inc/ppp_fsm.h"
#include "PPP/Inc/lcp.h"
#include "PPP/Inc/async.h"
#include "PPP/Inc/auth.h"
#include "PPP/Inc/ipcp.h"
#include "PPP/Inc/link.h"
#include "PPP/Inc/pap.h"
#include "PPP/Inc/ppp_init.h"
#include "PPP/Inc/ppp_input.h"
#include "PPP/Inc/ppp_atcmd.h"
#include "NVIM_Interface.h"
#include "TtfNvInterface.h"
#include "NasNvInterface.h"
#include "TafNvInterface.h"

/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
/*lint -e767  原因简述: 打点日志文件宏ID定义 */
#define    THIS_FILE_ID        PS_FILE_ID_PPP_INIT_C
/*lint +e767  */

/******************************************************************************
   PPP任务优先级高于modem_send高，比modem_recv任务低
******************************************************************************/
#define PPP_TASK_PRI                                  143

extern VOS_VOID PPP_ProcDataNotify(VOS_VOID);
extern VOS_UINT32  PPP_Snd1stDataNotify(VOS_VOID);
extern VOS_VOID PPP_ProcAsFrmDataInd(struct MsgCB * pMsg);




/******************************************************************************
   3 私有定义
******************************************************************************/


/******************************************************************************
   4 全局变量定义
******************************************************************************/
/* 保存从NV项中读取的WINS特性开关值*/
VOS_UINT8  g_ucPppConfigWins = WINS_CONFIG_ENABLE;

PPP_ENTITY_INFO_STRU                g_stPppEntInfo;

/* 队列自旋锁 */
extern      VOS_SPINLOCK            g_stPppASpinLock;

/******************************************************************************
   5 函数实现
******************************************************************************/

VOS_VOID PPP_UpdateWinsConfig(VOS_UINT8 ucWins)
{
    if ((WINS_CONFIG_DISABLE != ucWins) && (WINS_CONFIG_ENABLE != ucWins))
    {
        /* ucWins值无效 */
        return;
    }

    /* 更新g_ucPppConfigWins */
    g_ucPppConfigWins = ucWins;

    return;
}


VOS_VOID PPP_DataQInit(VOS_VOID)
{
    PPP_ZC_QUEUE_STRU                    *pstDataQ;


    pstDataQ    = &(g_PppDataQCtrl.stDataQ);

    PSACORE_MEM_SET(&g_PppDataQCtrl, sizeof(g_PppDataQCtrl), 0, sizeof(g_PppDataQCtrl));

    PPP_ZC_QUEUE_INIT(pstDataQ);

    return;
}



VOS_VOID PPP_ClearDataQ(VOS_VOID)
{
    PPP_ZC_STRU    *pstMem;

    for(;;)
    {
        if ( 0 == PPP_ZC_GET_QUEUE_LEN(&g_PppDataQCtrl.stDataQ) )
        {
            break;
        }

        pstMem  = (PPP_ZC_STRU *)PPP_ZC_DEQUEUE_HEAD(&g_PppDataQCtrl.stDataQ);

        PPP_MemFree(pstMem);
    }

    PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL,
                                  "PPP, PPP_ClearDataQ, NORMAL, Clear Stat Info\n");

    return;
} /* PPP_ClearDataQ */



VOS_UINT16 PPP_GetMruConfig(VOS_VOID)
{
    VOS_UINT16                      usPppConfigMru  = DEF_MRU;
    VOS_UINT32                      ulRslt;
    PPP_CONFIG_MRU_TYPE_NV_STRU     stPppConfigMruType;


    PSACORE_MEM_SET(&stPppConfigMruType, sizeof(PPP_CONFIG_MRU_TYPE_NV_STRU), 0x00, sizeof(PPP_CONFIG_MRU_TYPE_NV_STRU));


    /* 为客户定制PPP Default MRU而使用NV_Item，由于此NV结构为16bit，考虑到字节对齐因素，
       长度固定写sizeof(VOS_UINT16) */
    ulRslt = NV_ReadEx(MODEM_ID_0, en_NV_Item_PPP_CONFIG_MRU_Type, &stPppConfigMruType, sizeof(VOS_UINT16));
    usPppConfigMru  = stPppConfigMruType.usPppConfigType;

    if (NV_OK != ulRslt)
    {
        usPppConfigMru = DEF_MRU;
        PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_WARNING,
                     "Warning: Read en_NV_Item_PPP_DEFAULT_MRU_Type Error!");
    }

    if (usPppConfigMru > MAX_MRU)
    {
        usPppConfigMru = MAX_MRU;
    }

    if (usPppConfigMru < MIN_MRU)
    {
        usPppConfigMru = MIN_MRU;
    }

    PPP_MNTN_LOG1(PS_PID_APP_PPP, 0, PS_PRINT_INFO,
        "[INFO] PPP -- NV config MRU : <1>", (VOS_INT32)usPppConfigMru);

    return usPppConfigMru;
}


VOS_UINT8 PPP_GetWinsConfig(VOS_VOID)
{
    WINS_CONFIG_STRU    stWins;

    /* 初始化 */
    PSACORE_MEM_SET(&stWins, sizeof(stWins), 0x0, sizeof(stWins));

    /* 定制需求,读取WINS特性控制开关NV项 */

    /* 若读取en_NV_Item_WINS失败,则默认WINS设置为使能 */
    if(NV_OK != NV_ReadEx(MODEM_ID_0, en_NV_Item_WINS_Config, &stWins, sizeof(WINS_CONFIG_STRU)))
    {
        PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_WARNING, "Warning: Read en_NV_Item_WINS_Config Error!");
        return WINS_CONFIG_ENABLE;
    }

    /* 若en_NV_Item_WINS未设置，则默认WINS设置为使能 */
    if (0 == stWins.ucStatus)
    {
        return WINS_CONFIG_ENABLE;
    }

    /* 若en_NV_Item_WINS设置值无效，则默认WINS设置为使能 */
    if ((WINS_CONFIG_ENABLE != stWins.ucWins) && (WINS_CONFIG_DISABLE != stWins.ucWins))
    {
        return WINS_CONFIG_ENABLE;
    }

    PPP_MNTN_LOG1(PS_PID_APP_PPP, 0, PS_PRINT_INFO,
                  "PPP_GetWinsConfig,[INFO] PPP -- NV config WINS : %d", (VOS_INT32)stWins.ucWins);

    return stWins.ucWins;
}

/*****************************************************************************
 Prototype      : PPP_StatusPrint()
 Description    :
 Input          : VOS_VOID
 Output         :
 Return Value   : VOID
 Calls          :
 Called By      :
 History        :
  1.Date        : 2007-08-28
    Author      :
    Modification:
*****************************************************************************/
VOS_VOID PPP_StatusPrint(VOS_VOID)
{
    VOS_UINT32  ulIndex;
    struct lcp *lcp;

    PS_PRINTF(" lcp Info \n");
    for(ulIndex = 0; ulIndex < PPP_MAX_ID_NUM; ulIndex++)
    {
        lcp = &((pgPppLink + ulIndex)->lcp);

        /* 输出到Shell */
        PS_PRINTF(" %d: %s [%s]\n", ulIndex, lcp->fsm.name, State2Nam(lcp->fsm.state));
        PS_PRINTF(" his side: MRU %d, ACCMAP %08lx, PROTOCOMP %s, ACFCOMP %s,\n"
                    "           MAGIC %08lx, MRRU %u, SHORTSEQ %s, REJECT %04x\n",
                    lcp->his_mru, (VOS_UINT32)lcp->his_accmap,
                    lcp->his_protocomp ? "on" : "off",
                    lcp->his_acfcomp ? "on" : "off",
                    (VOS_UINT32)lcp->his_magic, lcp->his_mrru,
                    lcp->his_shortseq ? "on" : "off", lcp->his_reject);
        PS_PRINTF(" my  side: MRU %d, ACCMAP %08lx, PROTOCOMP %s, ACFCOMP %s,\n"
                    "           MAGIC %08lx, MRRU %u, SHORTSEQ %s, REJECT %04x\n",
                    lcp->want_mru, (VOS_UINT32)lcp->want_accmap,
                    lcp->want_protocomp ? "on" : "off",
                    lcp->want_acfcomp ? "on" : "off",
                    (VOS_UINT32)lcp->want_magic, lcp->want_mrru,
                    lcp->want_shortseq ? "on" : "off", lcp->my_reject);

        if (lcp->cfg.mru)
            PS_PRINTF("\n Defaults: MRU = %d (max %d), ",
                      lcp->cfg.mru, lcp->cfg.max_mru);
        else
            PS_PRINTF("\n Defaults: MRU = any (max %d), ",
                      lcp->cfg.max_mru);
        if (lcp->cfg.mtu)
            PS_PRINTF("MTU = %d (max %d), ",
                      lcp->cfg.mtu, lcp->cfg.max_mtu);
        else
            PS_PRINTF("MTU = any (max %d), ", lcp->cfg.max_mtu);
        PS_PRINTF("ACCMAP = %08lx\n", (VOS_UINT32)lcp->cfg.accmap);
        PS_PRINTF("           LQR period = %us, ",
                    lcp->cfg.lqrperiod);
        PS_PRINTF("Open Mode = %s",
                    lcp->cfg.openmode == OPEN_PASSIVE ? "passive" : "active");
        if (lcp->cfg.openmode > 0)
            PS_PRINTF(" (delay %ds)", lcp->cfg.openmode);
        PS_PRINTF("\n           FSM retry = %us, max %u Config"
                    " REQ%s, %u Term REQ%s\n", lcp->cfg.fsm.timeout,
                    lcp->cfg.fsm.maxreq, lcp->cfg.fsm.maxreq == 1 ? "" : "s",
                    lcp->cfg.fsm.maxtrm, lcp->cfg.fsm.maxtrm == 1 ? "" : "s");
        PS_PRINTF("    Ident: %s\n", lcp->cfg.ident);
        PS_PRINTF("\n Negotiation:\n");
        PS_PRINTF("           ACFCOMP =   %s\n",
                    command_ShowNegval(lcp->cfg.acfcomp));
        PS_PRINTF("           CHAP05 =    %s\n",
                    command_ShowNegval(lcp->cfg.chap05));
        PS_PRINTF("           CHAP80 =    %s\n",
                    command_ShowNegval(lcp->cfg.chap80nt));
        PS_PRINTF("           LANMan =    %s\n",
                    command_ShowNegval(lcp->cfg.chap80lm));
        PS_PRINTF("           CHAP81 =    %s\n",
                    command_ShowNegval(lcp->cfg.chap81));
        PS_PRINTF("           LQR =       %s\n",
                    command_ShowNegval(lcp->cfg.lqr));
        PS_PRINTF("           LCP ECHO =  %s\n",
                      lcp->cfg.echo ? "enabled" : "disabled");
        PS_PRINTF("           PAP =       %s\n",
                    command_ShowNegval(lcp->cfg.pap));
        PS_PRINTF("           PROTOCOMP = %s\n",
                    command_ShowNegval(lcp->cfg.protocomp));
    }

    return;
}

/*****************************************************************************
 Prototype      : PppStop
 Description    : TAF PPP模块中的结束函数,该函数负责在系统重新启动时释放PPP模
                  块向系统申请的资源
 Input          : ---
 Output         : ---
 Return Value   : ---VOS_UINT32
 Calls          : ---
 Called By      : ---

 History        : ---
  1.Date        : 2005-11-18
    Author      : ---
    Modification: Created function
*****************************************************************************/
#define PPP_FREE(point)\
    if(point!=VOS_NULL_PTR)         \
    {                                   \
        /*释放申请的全局所用的空间*/    \
        VOS_MemFree(PS_PID_APP_PPP, point);   \
        point = VOS_NULL_PTR;  \
    }\

VOS_VOID PppStop(VOS_VOID)
{
    VOS_INT32 i;

    /*对于系统所用到的throughout结构都释放掉它申请的内存*/
    for(i = 0;i < PPP_MAX_ID_NUM; i++)
    {
        throughput_destroy(&((pgPppLink + i)->stats.total));
    }

    PPP_FREE(pgPppLink)
    PPP_FREE(pgPppId)

    return;
}


VOS_VOID PPP_EntInit(VOS_VOID)
{
    VOS_UINT32                  ulRslt;
    NV_TTF_PPP_CONFIG_STRU      stPppConfig;

    PSACORE_MEM_SET(&g_stPppEntInfo, sizeof(PPP_ENTITY_INFO_STRU), 0x0, sizeof(PPP_ENTITY_INFO_STRU));

    PSACORE_MEM_SET(&stPppConfig, sizeof(NV_TTF_PPP_CONFIG_STRU), 0x0, sizeof(NV_TTF_PPP_CONFIG_STRU));

    ulRslt = NV_ReadEx(MODEM_ID_0, en_NV_Item_PPP_CONFIG, &stPppConfig, (VOS_UINT32)sizeof(NV_TTF_PPP_CONFIG_STRU));
    if (NV_OK != ulRslt)
    {
        g_stPppEntInfo.enChapEnable                 = TTF_TRUE;
        g_stPppEntInfo.enPapEnable                  = TTF_TRUE;
        g_stPppEntInfo.usLcpEchoMaxLostCnt          = 5;
        g_stPppEntInfo.usQueneMaxCnt                = 1500;

        return;
    }

    g_stPppEntInfo.enChapEnable                 = stPppConfig.enChapEnable;
    g_stPppEntInfo.enPapEnable                  = stPppConfig.enPapEnable;
    g_stPppEntInfo.usLcpEchoMaxLostCnt          = stPppConfig.usLcpEchoMaxLostCnt;
    g_stPppEntInfo.usQueneMaxCnt                = stPppConfig.usQueneMaxCnt;

    return;
}

/******************************************************************************
 Prototype       : PPP_BindToCpu
 Description     : 绑定Task到指定CPU上面
 Input           :
 Output          : NONE
 Return Value    : PS_SUCC   --- 成功
                   PS_FAIL   --- 失败
 History         :
   1.Date        : 2016-06-16
     Author      :
     Modification:
******************************************************************************/
VOS_VOID PPP_BindToCpu(VOS_VOID)
{
    VOS_LONG                ret;
    pid_t                   target_pid;
    VOS_INT                 cpu;

    /* 获取当前线程的Pid */
    target_pid = current->pid;

    /* 获取当前线程的affinity */
    ret = sched_getaffinity(target_pid, &(g_stPppEntInfo.orig_mask));
    if (ret < 0)
    {
        PS_PRINTF("warning: unable to get cpu affinity\n");
        return;
    }

    PSACORE_MEM_SET(&(g_stPppEntInfo.curr_mask), cpumask_size(), 0, cpumask_size());

    /* 设置当前线程的affinity */
    /*lint -e{713,732} */
    for_each_cpu(cpu, &(g_stPppEntInfo.orig_mask))
    {
        /* 去绑定CPU0 */
        if ((0 < cpu) && (cpumask_test_cpu(cpu, &(g_stPppEntInfo.orig_mask))))
        {
            cpumask_set_cpu((unsigned int)cpu, &(g_stPppEntInfo.curr_mask));
        }
    }

    if (0 == cpumask_weight(&(g_stPppEntInfo.curr_mask)))
    {
        cpumask_set_cpu(0, &(g_stPppEntInfo.curr_mask));
        return;
    }

    ret = sched_setaffinity(target_pid, &(g_stPppEntInfo.curr_mask));
    if (ret < 0)
    {
        PS_PRINTF("warning: unable to set cpu affinity\n");
        return;
    }

    return;
}


VOS_VOID PppMsgTimerProc( struct MsgCB * pMsg )
{
    REL_TIMER_MSG  *pPsMsg  = (REL_TIMER_MSG *)pMsg;
    VOS_UINT16      usPppId = (VOS_UINT16)(pPsMsg->ulPara);

    if (TIMER_PPP_LCP_ECHO_MSG == pPsMsg->ulName)
    {
        LcpSendEcho(PPP_LINK(usPppId));
    }
    else if (TIMER_PPP_PHASE_MSG == pPsMsg->ulName)
    {
        /*判断是那个阶段启动的定时器*/
        switch(PPP_LINK(usPppId)->phase)
        {
            /*如果是LCP阶段启动的定时器*/
            case PHASE_ESTABLISH:
                FsmTimeout(&(PPP_LINK(usPppId)->lcp.fsm));
                break;

            /*如果是终止阶段启动的定时器*/
            case PHASE_TERMINATE:
                FsmTimeout(&(PPP_LINK(usPppId)->lcp.fsm));
                break;

            /*如果是认证阶段启动的定时器*/
            case PHASE_AUTHENTICATE:
                AuthTimeout(PPP_LINK(usPppId));
                break;

            /*如果是IPCP阶段启动的定时器*/
            case PHASE_NETWORK:
                FsmTimeout(&(PPP_LINK(usPppId)->ipcp.fsm));
                break;

            default:
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_WARNING,"unknow phase!\r\n");
                break;
        }
    }
    else if (TIMER_PDP_ACT_PENDING == pPsMsg->ulName)
    {
        /*如果是IPCP阶段待PDP激活的定时器,则处理待处理的IPCP帧*/
        if (VOS_NULL_PTR != PPP_LINK(usPppId)->ipcp.pstIpcpPendFrame)
        {
            fsm_Input(&(PPP_LINK(usPppId)->ipcp.fsm), PPP_LINK(usPppId)->ipcp.pstIpcpPendFrame);
            PPP_LINK(usPppId)->ipcp.pstIpcpPendFrame = VOS_NULL_PTR;
        }
        PPP_LINK(usPppId)->ipcp.hIpcpPendTimer = VOS_NULL_PTR;
    }
    else if (TIMER_TERMINATE_PENDING == pPsMsg->ulName)
    {
        PPP_LINK(usPppId)->lcp.hLcpCloseTimer = VOS_NULL_PTR;
        PPP_ProcPppDisconnEvent(usPppId);
    }
    else
    {
        PPP_MNTN_LOG1(PS_PID_APP_PPP, 0, PS_PRINT_WARNING,"Unknow Timer!\n", pPsMsg->ulName);
    }

    return;
}


VOS_UINT32    APP_PPP_PidInit(enum VOS_INIT_PHASE_DEFINE InitPhase )
{
    VOS_INT32               i;



    switch( InitPhase )
    {
    case   VOS_IP_LOAD_CONFIG:
            /*定制需求,通过NV项,获取用户配置MRU以及WINS协商开关*/
            /*读取NV项,获取用户配置MRU*/
            g_usPppConfigMru = PPP_GetMruConfig();

            /*读取NV项,获取用户配置WINS使能开关*/
            g_ucPppConfigWins = PPP_GetWinsConfig();

            /*向系统申请分配一块全局所用的link数组的空间*/
            /*lint -e433*/
            pgPppLink = (struct link *)VOS_MemAlloc(PS_PID_APP_PPP, STATIC_MEM_PT, sizeof(struct link)*PPP_MAX_ID_NUM);
            /*lint +e433*/
            if (VOS_NULL_PTR == pgPppLink)
            {
                /*输出错误信息*/
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_ERROR,"APP_PPP_PidInit, malloc of  memory fail\r\n");
                return VOS_ERR;
            }

            /*向系统申请分配一块全局所用的TAF_PPP_PUBLIC_STRU的空间*/
            pgPppId = (PPP_ID *)VOS_MemAlloc(PS_PID_APP_PPP, STATIC_MEM_PT, sizeof(PPP_ID)*PPP_MAX_ID_NUM_ALLOC);
            if (VOS_NULL_PTR == pgPppId)
            {
                /*释放申请的全局所用的TAF_PPP_PUBLIC_STRU的空间*/
                VOS_MemFree(PS_PID_APP_PPP, pgPppLink);
                pgPppLink = VOS_NULL_PTR;


                /*输出错误信息*/
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_ERROR,"APP_PPP_PidInit, malloc of  memory fail\r\n");
                return VOS_ERR;
            }

            /*对于系统所用到的所有数据结构都初始化*/
            for(i = 0;i < PPP_MAX_ID_NUM; i++)
            {
                link_Init((pgPppLink + i));
            }

            PppIdInit();

            /*初始化PPP的数据队列*/
            PPP_DataQInit();

            /* 初始化自旋锁 */
            PPP_InitSpinLock();

            PPP_EntInit();



            break;

    case   VOS_IP_FARMALLOC:
    case   VOS_IP_INITIAL:
    case   VOS_IP_ENROLLMENT:
    case   VOS_IP_LOAD_DATA:
    case   VOS_IP_FETCH_DATA:
    case   VOS_IP_STARTUP:
    case   VOS_IP_RIVAL:
    case   VOS_IP_KICKOFF:
    case   VOS_IP_STANDBY:
    case   VOS_IP_BROADCAST_STATE:
    case   VOS_IP_RESTART:
    case   VOS_IP_BUTT:
           break;
    }

    return VOS_OK;
}


VOS_VOID APP_PPP_EventProc(VOS_UINT32 ulEvent)
{

    if (ulEvent & PPP_RCV_DATA_EVENT)
    {
        PPP_ProcDataNotify();
    }


    return;
}



VOS_VOID APP_PPP_MsgProc( struct MsgCB * pMsg )
{
    PPP_MSG    *pPsMsg  = (PPP_MSG *)pMsg;

    if(pMsg == VOS_NULL_PTR)
    {
        return;
    }


    /*如果是定时器发来的消息*/
    if (VOS_PID_TIMER == pMsg->ulSenderPid)
    {
        PppMsgTimerProc(pMsg);


        return ;
    }

    /*如果是接收到从TE发送来的数据帧*/
    switch(pPsMsg->ulMsgType)
    {
        case PPP_DATA_PROC_NOTIFY:
            PPP_ProcDataNotify();
            break;

        case PPP_AT_CTRL_OPERATION:
            PPP_ProcAtCtrlOper(pMsg);
            break;

        case PPP_HDLC_PROC_AS_FRM_PACKET_IND:
            PPP_ProcAsFrmDataInd(pMsg);
            break;



        default:
            break;
    }


    return;
}


/*lint -e{838} */
VOS_VOID APP_PPP_FidTask(VOS_VOID)
{
    MsgBlock                           *pMsg          = VOS_NULL_PTR;
    VOS_UINT32                          ulEvent       = 0;
    VOS_UINT32                          ulTaskID      = 0;
    VOS_UINT32                          ulRtn         = VOS_ERR;
    VOS_UINT32                          ulEventMask   = 0;
    VOS_UINT32                          ulExpectEvent = 0;

    ulTaskID = VOS_GetCurrentTaskID();
    if (PS_NULL_UINT32 == ulTaskID)
    {
        PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_ERROR, "APP_PPP_FidTask: ERROR, TaskID is invalid.");
        return;
    }

    if (VOS_OK != VOS_CreateEvent(ulTaskID))
    {
        PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_ERROR, "APP_PPP_FidTask: ERROR, create event fail.");
        return;
    }

    PPP_BindToCpu();

    ulExpectEvent = PPP_RCV_DATA_EVENT | VOS_MSG_SYNC_EVENT;
    ulEventMask   = VOS_EVENT_ANY | VOS_EVENT_WAIT;

    g_stPppEntInfo.ulPppInitFlag    = 1;
    g_stPppEntInfo.ulPppTaskId      = ulTaskID;

    /*lint -e{716} */
    for(;;)
    {
        ulRtn = VOS_EventRead(ulExpectEvent, ulEventMask, 0, &ulEvent);
        if (VOS_OK != ulRtn)
        {
            PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_ERROR, "APP_PPP_FidTask: ERROR, read event error.");
            continue;
        }

        /* 事件处理 */
        if (VOS_MSG_SYNC_EVENT != ulEvent)
        {
            APP_PPP_EventProc(ulEvent);
            continue;
        }

        pMsg = (MsgBlock*)VOS_GetMsg(ulTaskID);
        if (VOS_NULL_PTR != pMsg)
        {
            if (PS_PID_APP_PPP == pMsg->ulReceiverPid)
            {
                APP_PPP_MsgProc(pMsg);
            }

            (VOS_VOID)VOS_FreeMsg(PS_PID_APP_PPP, pMsg);
        }
    }
}



VOS_UINT32 APP_PPP_FidInit(enum VOS_INIT_PHASE_DEFINE ip)
{
    VOS_UINT32                          ulRslt;

    switch (ip)
    {
        case VOS_IP_LOAD_CONFIG:
            /* 上行PID初始化 */
            ulRslt = VOS_RegisterPIDInfo(PS_PID_APP_PPP,
                                (Init_Fun_Type)APP_PPP_PidInit,
                                (Msg_Fun_Type)APP_PPP_MsgProc);
            if (VOS_OK != ulRslt)
            {
                PS_PRINTF("APP_PPP_FidInit, register PPP PID fail!\n");
                return VOS_ERR;
            }

            ulRslt = VOS_RegisterMsgTaskEntry(ACPU_FID_PPP, (VOS_VOIDFUNCPTR)APP_PPP_FidTask);
            if (VOS_OK != ulRslt)
            {
                PS_PRINTF("APP_PPP_FidInit, VOS_RegisterMsgTaskEntry fail!\n");
                return VOS_ERR;
            }

            /* 任务优先级 */
            ulRslt = VOS_RegisterTaskPrio(ACPU_FID_PPP, VOS_PRIORITY_P4);
            if( VOS_OK != ulRslt )
            {
                PS_PRINTF("APP_PPP_FidInit, register priority fail!\n");
                return VOS_ERR;
            }

            break;

        case VOS_IP_FARMALLOC:
        case VOS_IP_INITIAL:
        case VOS_IP_ENROLLMENT:
        case VOS_IP_LOAD_DATA:
        case VOS_IP_FETCH_DATA:
        case VOS_IP_STARTUP:
        case VOS_IP_RIVAL:
        case VOS_IP_KICKOFF:
        case VOS_IP_STANDBY:
        case VOS_IP_BROADCAST_STATE:
        case VOS_IP_RESTART:
        case VOS_IP_BUTT:
            break;
    }

    return VOS_OK;
}


