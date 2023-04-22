/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/fs.h>
#include <osl_malloc.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>
#include <of.h>
#include <osl_thread.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include "bsp_hds_service.h"
#include "bsp_hds_ind.h"
#include "bsp_slice.h"
#include "bsp_trace.h"

print_send_buff* g_print_sendbuf = NULL;
trans_send_buff* g_trans_sendbuf = NULL;
LOG_SRC_CFG_STRU g_logSrcCfg = {0};
u32 g_print_init_state=PRINTLOG_CHN_UNINIT;
u32 g_trans_init_state=TRANSLOG_CHN_UNINIT;
unsigned long long g_log_dma_test_mask=0xffffffffULL;
u32 g_printlog_pkgnum = 0;
u32 g_Translog_pkgnum = 0;
u32 g_printlog_transId = 0;
u32 g_translog_transId = 0;
print_report_hook g_bsp_print_hook = NULL;
hds_lock_ctrl_info g_hds_lock_ctrl;
u64 g_dma_hds_mask = (u64)(-1);

static inline void * bsp_MemPhyToVirt(u8 *pucCurPhyAddr, u8 *pucPhyStart, u8 *pucVirtStart, u32 ulBufLen)
{
    if((pucCurPhyAddr < pucPhyStart) || (pucCurPhyAddr >= (pucPhyStart+ulBufLen)))
    {
        printk(KERN_ERR "bsp_MemPhyToVirt fail!\n");
        return HDS_NULL;
    }
    return (void *)((pucCurPhyAddr - pucPhyStart) + pucVirtStart);
}

void bsp_print_level_cfg(u32 *level)
{
    if((*level) >= BSP_LOG_LEVEL_ERROR)
    {
        (*level) = 1;
    }
    else if(BSP_LOG_LEVEL_WARNING == (*level))
    {
        (*level) =  2;
    }
    else if(BSP_LOG_LEVEL_NOTICE == (*level))
    {
        (*level) =  3;
    }
    else if((BSP_LOG_LEVEL_INFO == (*level)) || (BSP_LOG_LEVEL_DEBUG == (*level)))
    {
        (*level) =  4;
    }
}


s32 bsp_log_write_socp_chan(u8* data,u32 len)
{

    SOCP_BUFFER_RW_STRU wbuf = {NULL,};
    if( HDS_OK != bsp_socp_get_write_buff(SOCP_CODER_SRC_LOG_IND, &wbuf))
    {
       printk(KERN_ERR "get print buffer fail,chan=0x%x,len=0x%x\n", SOCP_CODER_SRC_LOG_IND, len);
       return HDS_ERR;
    }

    wbuf.pBuffer = (char *)bsp_MemPhyToVirt((u8*)(wbuf.pBuffer), g_logSrcCfg.pucPhyStart, g_logSrcCfg.pucVirtStart, g_logSrcCfg.ulBufLen);

    if((wbuf.u32Size >= len) && (NULL != wbuf.pBuffer))
    {
        memcpy((void*)(wbuf.pBuffer), data, (unsigned long)len);
        LOG_FLUSH_CACHE(wbuf.pBuffer, (unsigned long)len);
        bsp_socp_write_done(SOCP_CODER_SRC_LOG_IND, len);
    }
    else if(wbuf.u32Size + wbuf.u32RbSize >= len)
    {
        wbuf.pRbBuffer = (char *)bsp_MemPhyToVirt((u8*)(wbuf.pRbBuffer), g_logSrcCfg.pucPhyStart, g_logSrcCfg.pucVirtStart, g_logSrcCfg.ulBufLen);
        if((NULL != wbuf.pBuffer) && (NULL != wbuf.pRbBuffer))
        {
            memcpy((void*)(wbuf.pBuffer),data,(unsigned long)(wbuf.u32Size));
            LOG_FLUSH_CACHE(wbuf.pBuffer, (unsigned long)(wbuf.u32Size));
            memcpy((void*)(wbuf.pRbBuffer), data+wbuf.u32Size, (unsigned long)(len-wbuf.u32Size));
            LOG_FLUSH_CACHE(wbuf.pRbBuffer, (unsigned long)(len-wbuf.u32Size));

            bsp_socp_write_done(SOCP_CODER_SRC_LOG_IND, len);
        }
    }
    else
    {
        printk(KERN_ERR"socp buffer not enough!\n");
        return HDS_ERR;
    }

    return HDS_OK;
}

/*lint -save -e550 */
s32 bsp_transreport(TRANS_IND_STRU *pstData)
{
    s32 ret;
    u64 auctime;
    u32 socp_packet_len;
    u32 diag_packet_len;
    u32 trans_packet_len;
    diag_trans_head_stru *trans_head;
    diag_frame_head_stru *diag_head;
    diag_socp_head_stru  *socp_head;
    unsigned long lock_flag = 0;

    /*判断工具连接状态、开关状态*/
    if(0 == g_translog_conn)
    {
       printk(KERN_ERR"hids not conn(%d)!\n",g_translog_conn);
       return HDS_TRANS_SW_ERR;
    }

    /*入参检查*/
    if((NULL == pstData)||(NULL == pstData->pData)||(0 == pstData->ulLength) || ((pstData->ulLength) > (TRANSLOG_MAX_HIDS_BUFF_LEN - 1)))
    {
       printk(KERN_ERR"pstdata err!\n");
       return HDS_ERR;
    }

    trans_head = &g_trans_sendbuf->trans_head;
    diag_head  = &g_trans_sendbuf->diag_head;
    socp_head  = &g_trans_sendbuf->socp_head;

    trans_packet_len = pstData->ulLength + DIAG_TRANS_HEAD_SIZE;
    diag_packet_len  = trans_packet_len + DIAG_FRAME_HEAD_SIZE;
    socp_packet_len  = diag_packet_len  + DIAG_SOCP_HEAD_SIZE;

    spin_lock_irqsave(&g_hds_lock_ctrl.trans_lock,lock_flag);

    memcpy(g_trans_sendbuf->data,pstData->pData, (unsigned long)(pstData->ulLength));

    /*fill trans head*/
    trans_head->ulModule   = 0x8003;            /*Pid,0x8003代表BSP*/
    trans_head->ulMsgId    = pstData->ulMsgId;
    trans_head->ulNo       = g_Translog_pkgnum++;
    /*fill diag head*/
    bsp_slice_getcurtime(&auctime);
    memcpy(diag_head->stService.aucTimeStamp,&auctime, sizeof(diag_head->stService.aucTimeStamp));
    /*pstData->ulModule:31-24bit,代表mdmid3b,主副卡*/
    diag_head->stService.mdmid3b = ((pstData->ulModule) & 0xff000000)>>24;
    diag_head->stService.MsgTransId  = g_translog_transId;

    diag_head->stID.pri4b    = DIAG_FRAME_MSG_TYPE_BSP;
    diag_head->stID.mode4b   = ((pstData->ulModule) & 0x00ff0000)>>16;
    diag_head->stID.sec5b    = DIAG_FRAME_MSG_STRUCT;
    diag_head->stID.cmdid19b = ((pstData->ulMsgId) & 0x7ffff);       /*pstData->ulMsgId:18-0bit,代表cmdid19b*/
    diag_head->u32MsgLen     = trans_packet_len;

    /*fill socp head*/
    socp_head->u32DataLen = diag_packet_len;

    ret = bsp_log_write_socp_chan((u8*)g_trans_sendbuf, socp_packet_len);
    if(ret)
    {
        printk(KERN_ERR"write transdata to socp fail!\n");
        spin_unlock_irqrestore(&g_hds_lock_ctrl.trans_lock,lock_flag);
        return ret;
    }

    /* trans_id++, only if send data successfully*/
    g_translog_transId++;

    spin_unlock_irqrestore(&g_hds_lock_ctrl.trans_lock,lock_flag);
    return HDS_TRANS_RE_SUCC;
}
/*lint -restore +e550 */


s32 bsp_printreport(char *logdata,u32 level,u32 module_id)
{
    s32 ret;
    u32 prelen, datalen;
    u32 socp_packet_len;
    u32 diag_packet_len;
    u32 print_packet_len;
    u64   auctime = 0;
    diag_print_head_stru *print_head;
    diag_frame_head_stru *diag_head;
    diag_socp_head_stru  *socp_head;

    /*入参检查*/
    if((NULL == logdata)||(level > g_printlog_level))
    {
       printk(KERN_ERR"logdata or level err!\n");
       return HDS_ERR;
    }

    prelen = (u32)snprintf((char *)(g_print_sendbuf->data), (unsigned long)PRINTLOG_MAX_FILENAME_LEN,"%s:%d[%d]", "module", module_id, 0);
    if(prelen > PRINTLOG_MAX_FILENAME_LEN)
    {
       printk(KERN_ERR"print prelen err!\n");
       return HDS_ERR;
    }

    datalen = (u32)snprintf((char *)(g_print_sendbuf->data+prelen), (unsigned long)PRINTLOG_MAX_BUFF_LEN,"%s", logdata);/* [false alarm]:fortify */
    if(datalen > PRINTLOG_MAX_BUFF_LEN)
    {
       printk(KERN_ERR"print datalen err!\n");
       return HDS_ERR;
    }

    datalen = datalen + prelen;

    print_head = &g_print_sendbuf->print_head;
    diag_head = &g_print_sendbuf->diag_head;
    socp_head = &g_print_sendbuf->socp_head;

    print_packet_len = datalen + DIAG_PRINT_HEAD_SIZE;
    diag_packet_len  = print_packet_len + DIAG_FRAME_HEAD_SIZE;
    socp_packet_len  = diag_packet_len  + DIAG_SOCP_HEAD_SIZE;

    bsp_print_level_cfg(&level);
    //printk(KERN_ERR "level = %d!\n",level);

    /*fill print head*/
    /* 1:error, 2:warning, 3:normal, 4:info */
    /* (0|ERROR|WARNING|NORMAL|INFO|0|0|0) */
    print_head->u32level = (0x80000000) >> level;
    print_head->u32module = 0x8003;
    print_head->u32no = g_printlog_pkgnum++;

    /*fill diag head*/
    bsp_slice_getcurtime(&auctime);
    memcpy(diag_head->stService.aucTimeStamp,&auctime, sizeof(diag_head->stService.aucTimeStamp));
    diag_head->stService.MsgTransId  = g_printlog_transId;
    diag_head->stID.sec5b    = DIAG_FRAME_MSG_PRINT;
    diag_head->stID.mode4b   = DIAG_FRAME_MODE_COMM;
    diag_head->stID.pri4b    = DIAG_FRAME_MSG_TYPE_BSP;
    diag_head->u32MsgLen     = print_packet_len;

    /*fill socp head*/
    socp_head->u32DataLen   = diag_packet_len;

    ret = bsp_log_write_socp_chan((u8*)g_print_sendbuf, socp_packet_len);
    if(ret)
    {
        printk(KERN_ERR"write printdata to socp fail!\n");
        return ret;
    }

    /* trans_id++, only if send data successfully*/
    g_printlog_transId++;

    return HDS_PRINT_RE_SUCC;
}

/*lint -save -e550 */
int bsp_trace_to_hids(u32 module_id, u32 level, char* print_buff)
{
    unsigned long lock_flag;
    int ret = HDS_PRINT_SW_ERR;
    static bool print_flag = false;

    spin_lock_irqsave(&g_hds_lock_ctrl.trace_lock,lock_flag);
    if (print_flag)
    {
        printk(KERN_ERR "print data report recursion!\n");
        spin_unlock_irqrestore(&g_hds_lock_ctrl.trace_lock,lock_flag);
        return HDS_PRINT_RECURSION;
    }
    print_flag = true;

    /*判断工具连接状态、开关状态*/
    if((1 == g_printlog_conn)&&(1 == g_printlog_enable))
    {
        ret = bsp_printreport(print_buff,level,module_id);

        if (HDS_PRINT_RE_SUCC != ret)
        {
            printk(KERN_ERR "print data report fail,ret = 0x%x!\n",ret);
        }
        print_flag = false;
        spin_unlock_irqrestore(&g_hds_lock_ctrl.trace_lock,lock_flag);
        return ret;
    }
    else
    {
        //printk(KERN_ERR "hids disconnect(0x%x) or print switch off(0x%x)!\n",g_printlog_conn, g_printlog_enable);
        print_flag = false;
        spin_unlock_irqrestore(&g_hds_lock_ctrl.trace_lock,lock_flag);
        return ret;
    }
}
/*lint -restore +e550 */

s32 bsp_socp_log_chan_cfg(void)
{
    SOCP_CODER_SRC_CHAN_S EncSrcAttr;
    dma_addr_t  ulAddress = 0;
    u8 *p;
    struct device dev;

    /* coverity[secure_coding] */
    memset(&dev,0,sizeof(dev));
    dma_set_mask_and_coherent(&dev, g_dma_hds_mask);
    of_dma_configure(&dev, NULL);
    p =(u8 *) dma_alloc_coherent(&dev, (unsigned long)LOG_SRC_BUF_LEN, &ulAddress, GFP_KERNEL);

    if(HDS_NULL == p)
    {
        printk(KERN_ERR"log src chan malloc fail!\n");
        return HDS_ERR;
    }

    EncSrcAttr.eDataType   = SOCP_DATA_TYPE_0;
    EncSrcAttr.eDataTypeEn = SOCP_DATA_TYPE_EN;
    EncSrcAttr.eDebugEn    = SOCP_ENC_DEBUG_DIS;
    EncSrcAttr.eMode       = SOCP_ENCSRC_CHNMODE_CTSPACKET;
    EncSrcAttr.ePriority   = SOCP_CHAN_PRIORITY_2;
    EncSrcAttr.u32BypassEn = 0;
    EncSrcAttr.u32DestChanID = SOCP_CODER_DST_OM_IND;

    EncSrcAttr.sCoderSetSrcBuf.pucInputStart = (u8*)ulAddress;
    EncSrcAttr.sCoderSetSrcBuf.pucInputEnd = (u8*)(ulAddress +LOG_SRC_BUF_LEN - 1);

    g_logSrcCfg.pucPhyStart = (u8*)ulAddress;
    g_logSrcCfg.pucVirtStart = (void*)LOG_PHYS_TO_VIRT(ulAddress);
    g_logSrcCfg.ulBufLen = LOG_SRC_BUF_LEN;

    /*调用SOCP接口进行编码源通道配置*/
    if(HDS_OK != bsp_socp_coder_set_src_chan(SOCP_CODER_SRC_LOG_IND, &EncSrcAttr))
    {

        printk(KERN_ERR"log src channel set failed!\n");
        return HDS_ERR;
    }

    /*启动编码*/
    bsp_socp_start(SOCP_CODER_SRC_LOG_IND);
    return HDS_OK;
}



int __init bsp_hds_init(void)
{
    int ret;

    bsp_hds_service_init();

    /*上报LOG的SOCP通道*/
    ret=bsp_socp_log_chan_cfg();
    if(ret)
    {
        printk(KERN_ERR"bsplog src chan fail!\n");
        return HDS_ERR;
    }

    spin_lock_init(&g_hds_lock_ctrl.trace_lock);
    g_bsp_print_hook = (print_report_hook)bsp_trace_to_hids;

    /*为print数据申请buffer*/
    g_print_sendbuf = (print_send_buff*)osl_malloc((unsigned int)sizeof(print_send_buff));
    if(NULL == g_print_sendbuf)
    {
        printk(KERN_ERR"printbuf malloc fail!\n");
        return HDS_ERR;
    }
    bsp_diag_frame_head_init(&g_print_sendbuf->diag_head);
    bsp_diag_fill_socp_head(&g_print_sendbuf->socp_head,0);

    g_print_init_state=PRINTLOG_CHN_INIT;

    spin_lock_init(&g_hds_lock_ctrl.trans_lock);
    /*为结构化消息数据申请buffer*/
    g_trans_sendbuf = (trans_send_buff*)osl_malloc((unsigned int)sizeof(trans_send_buff));
    if(NULL == g_trans_sendbuf)
    {
        printk(KERN_ERR"transbuf malloc fail!\n");
        return HDS_ERR;
    }
    bsp_diag_frame_head_init(&g_trans_sendbuf->diag_head);
    bsp_diag_fill_socp_head(&g_trans_sendbuf->socp_head,0);

    g_trans_init_state=TRANSLOG_CHN_INIT;

    printk(KERN_ERR"hds_init ok!\n");
    return HDS_OK;
}

/*lint --e{528}*/
module_init(bsp_hds_init);
