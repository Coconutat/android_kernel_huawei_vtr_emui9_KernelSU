
 
#include "processor.h"


/*================ EXTERN VALUE ================*/
extern HI_U32   g_ExtraDispNum;
extern HI_U32   g_ClientReservedNum;
extern HI_BOOL  g_SaveYuvEnable;
extern HI_CHAR  g_SavePath[];
extern HI_CHAR  g_SaveName[];


/*================== MACRO ===================*/
#define NO_NEW_IMAGE               (HI_SUCCESS+1)
#define REALID_WITH_EOF_OFFSET(id) (id%100+2)
#define INVALID_IMAGE_ID           (0xFFFFFFFF)
#define IMAGE_ID_OFFSET            (2)

#define CHECK_PROCESSOR_ID(id)   VDEC_ASSERT_RETURN((id)<0 || (id)>=MAX_CHANNEL_NUM)

/*================ DEFINITION ================*/
typedef enum{
    BPP_THREAD_INVALID = 0,
    BPP_THREAD_SLEEPING,
    BPP_THREAD_ONCALL,
    BPP_THREAD_RUNNING,
    BPP_THREAD_EXIT,
    BPP_THREAD_BUTT,
}BPP_THREAD_STATE_E;

typedef enum{
    BPP_INST_INVALID = 0,
    BPP_INST_IDLE,
    BPP_INST_WORK,
    BPP_INST_SUSPEND,
    BPP_INST_BUTT,
}BPP_INST_STATE_E;

typedef struct{
    HI_U32              image_id;
    HI_U32              chroma_offset;
    HI_U32              frame_width;
    HI_U32              frame_height;
    HI_U32              disp_width;
    HI_U32              disp_height;
    HI_U32              left_offset;
    HI_U32              right_offset;
    HI_U32              top_offset;
    HI_U32              bottom_offset;
    HI_U32              stride;
    HI_U32              format;
    HI_U32              is_cur_last;
    HI_U32              is_last_frame;
    HI_U32              ActCRC[2];
    HI_S64              timestamp; 
}BPP_IMAGE_INFO_S;

typedef struct{
    HI_BOOL             is_valid;
    HI_BOOL             is_eof;
    HI_U32              frame_phy_addr;
    HI_U32              frame_size;
    BPP_IMAGE_INFO_S    image;    
}BPP_VIDEO_FRAME_S;

typedef struct {
    HI_U32              inst_id;
    BPP_INST_STATE_E    inst_state;
    BPP_THREAD_STATE_E  thread_state;
    wait_queue_head_t   task_wait;
    struct task_struct *pThread;
    OMXVDEC_CHAN_CTX   *pMaster;
    MEM_BUFFER_S        mem_buf;
    BPP_VIDEO_FRAME_S   stFrame;
}BPP_CONTEXT_S;


/*================ INTERNAL VALUE ================*/
HI_U8 g_BPPInit = 0;
struct mutex gBPPMut;
BPP_CONTEXT_S *gpBPPContext[MAX_CHANNEL_NUM];


/*============== INTERNAL FUNCTION ==============*/
static inline const HI_PCHAR processor_show_thread_state(BPP_THREAD_STATE_E state)
{
    switch (state)
    {
       case BPP_THREAD_INVALID:
            return "INVALID";

       case BPP_THREAD_SLEEPING:
            return "SLEEPING";

       case BPP_THREAD_ONCALL:
            return "ONCALL";

       case BPP_THREAD_RUNNING:
            return "RUNNING";

       case BPP_THREAD_EXIT:
            return "EXIT";
            
       default:
            return "UNKOWN";
    }
}

static inline const HI_PCHAR processor_show_eof_state(ePRC_EOF_STATE state)
{
    switch (state)
    {
        case PRC_EOF_INIT_STATE:
            return "INIT STATE";

        case PRC_EOF_WAIT_SLOT:
            return "WAIT SLOT";
            
        case PRC_EOF_GOT_SLOT:
            return "GOT SLOT";
            
        case PRC_EOF_REPORT_SUCCESS:
            return "REPORT SUCCESS";
            
        case PRC_EOF_REPORT_FAILED:
            return "REPORT FAILED";

        default:
            return "INVALID STATE";
    }
}

static inline const HI_PCHAR processor_show_progressive(HI_U32 flag)
{
    /* CodecType Relative */
    HI_PCHAR s;
    switch (flag)
    {
        case 0:
           s = "Progressive";
           break;

        case 1:
           s = "Interlaced";
           break;

        case 2:
           s = "Infered Progressive";
           break;

        case 3:
           s = "Infered Interlaced";
           break;

        default:
           s = "unknow";
           break;
    }

    return s; /* [false alarm] */
}

static inline const HI_PCHAR processor_show_field_valid(HI_U32 flag)
{
    /* CodecType Relative */
    HI_PCHAR s;
    switch (flag)
    {
        case 0:
           s = "Both Field Invalid";
           break;

        case 1:
           s = "Top Field Valid";
           break;

        case 2:
           s = "Bottom Field Valid";
           break;

        case 3:
           s = "Both Field Valid";
           break;

        default:
           s = "unknow";
           break;
    }

    return s; /* [false alarm] */
}

static inline const HI_PCHAR processor_show_field_first(HI_U32 flag)
{
    /* CodecType Relative */
    HI_PCHAR s;
    switch (flag)
    {
        case 0:
           s = "Bottom Field First";
           break;

        case 1:
           s = "Top Field First";
           break;

        default:
           s = "unknow";
           break;
    }

    return s; /* [false alarm] */
}

static inline HI_U32 processor_calculate_data_len(HI_U32 YPlanarSize, HI_U32 ColorFormat) 
{
    switch(ColorFormat)
    {
        default:
            OmxPrint(OMX_ERR, "%s unkown format %d, set as NV12!\n", __func__, ColorFormat);
        case OMX_PIX_FMT_NV12:/*lint !e616 */
        case OMX_PIX_FMT_NV21:
        case OMX_PIX_FMT_YUV420Planar:
        case OMX_PIX_FMT_YUV420Tile:
        {
            return (YPlanarSize * 3)/2;
        }
    }
}

static HI_S32 processor_stop_thread(BPP_CONTEXT_S *pBppContext) 
{
    HI_S32 count;

    for (count=0; count<100; count++)
    {
        if (BPP_THREAD_RUNNING == pBppContext->thread_state)
        {
            msleep(10);
        }
        else
        {
            break;
        }
    }

    if (count >= 100)
    {
        OmxPrint(OMX_ERR, "processor stop failed!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static HI_S32 processor_report_size_change_normal(OMXVDEC_CHAN_CTX *pchan, BPP_VIDEO_FRAME_S *pInFrame)
{
    OMXVDEC_SEQ_INFO SeqInfo;
    HI_U32 new_size = pInFrame->frame_size;
    HI_U32 new_stride = ALIGN_UP(pInFrame->image.frame_width, DEFAULT_ALIGN_STEP);

    pchan->out_width      = pInFrame->image.frame_width;
    pchan->out_height     = pInFrame->image.frame_height;
    pchan->out_stride     = new_stride;

    SeqInfo.dec_width     = pInFrame->image.frame_width;
    SeqInfo.dec_height    = pInFrame->image.frame_height;
    SeqInfo.disp_width    = pInFrame->image.disp_width;
    SeqInfo.disp_height   = pInFrame->image.disp_height;
    SeqInfo.stride        = new_stride;
    SeqInfo.frame_size    = new_size;
    SeqInfo.min_frame_num = g_ExtraDispNum;
    SeqInfo.max_frame_num = g_ExtraDispNum + g_ClientReservedNum;
    
    pchan->recfg_flag = 1;

    OmxPrint(OMX_INFO, "Normal mode report new resolution: width %d, stride %d, height %d, frame_size = %d, need_num = %d, ref_num = %d\n",
             SeqInfo.dec_width, SeqInfo.stride, SeqInfo.dec_height, SeqInfo.frame_size, SeqInfo.max_frame_num, SeqInfo.min_frame_num);
    
    /* Report to allocate frame buffer outside */
    message_queue(pchan->msg_queue, VDEC_EVT_REPORT_SEQ_INFO_CHG, VDEC_S_SUCCESS, (HI_VOID *)&SeqInfo);

    return HI_SUCCESS;
}

static HI_S32 processor_report_size_change_bypass(OMXVDEC_CHAN_CTX *pchan)
{
    OMXVDEC_SEQ_INFO SeqInfo;
    SEQ_INFO_S *pSeqInfo = &pchan->seq_info;

    pchan->out_width      = pSeqInfo->dec_width;
    pchan->out_height     = pSeqInfo->dec_height;
    pchan->out_stride     = pSeqInfo->stride;

    SeqInfo.dec_width     = pSeqInfo->dec_width;
    SeqInfo.dec_height    = pSeqInfo->dec_height;
    SeqInfo.disp_width    = pSeqInfo->disp_width;
    SeqInfo.disp_height   = pSeqInfo->disp_height;
    SeqInfo.stride        = pSeqInfo->stride;
    SeqInfo.frame_size    = pSeqInfo->ref_frame_size;
    SeqInfo.min_frame_num = pSeqInfo->ref_frame_num + g_ExtraDispNum;
    SeqInfo.max_frame_num = pSeqInfo->ref_frame_num + g_ExtraDispNum + g_ClientReservedNum;
    
    pchan->recfg_flag = 1;
    pchan->dfs_alloc_flag = DFS_WAIT_INSERT;

    OmxPrint(OMX_INFO, "Bypass mode report new resolution: dec_width %d, dec_height %d,  disp_width %d, disp_height %d, stride %d, frame_size = %d, need_num = %d, ref_num = %d\n",
             SeqInfo.dec_width, SeqInfo.dec_height, SeqInfo.disp_width, SeqInfo.disp_height, SeqInfo.stride, SeqInfo.frame_size, SeqInfo.max_frame_num, SeqInfo.min_frame_num);
    
    /* Report to allocate frame buffer outside */
    message_queue(pchan->msg_queue, VDEC_EVT_REPORT_SEQ_INFO_CHG, VDEC_S_SUCCESS, (HI_VOID *)&SeqInfo);

    return HI_SUCCESS;
}

static HI_S32 processor_get_frame(OMXVDEC_CHAN_CTX *pchan, BPP_VIDEO_FRAME_S *pframe, HI_U32 expect_size)
{
    HI_S32 ret = HI_FAILURE;
    unsigned long flags;
    OMXVDEC_BUF_S *pbuf = HI_NULL;
    
    spin_lock_irqsave(&pchan->yuv_lock, flags);
    if (1 == pchan->recfg_flag)
    {
        OmxPrint(OMX_VPSS, "%s size changed, reconfiguring!\n", __func__);
        goto EXIT;
    }
    
    if (list_empty(&pchan->yuv_queue))
    {
        OmxPrint(OMX_VPSS, "%s List is empty!\n", __func__);
        goto EXIT;
    }

    list_for_each_entry(pbuf, &pchan->yuv_queue, list)
    {
        if (BUF_STATE_QUEUED == pbuf->status && pbuf->buf_len >= expect_size)
        {
            pbuf->status           = BUF_STATE_USING;
            pframe->frame_phy_addr = pbuf->phy_addr + pbuf->offset;
            pframe->frame_size     = pbuf->buf_len;

            pchan->yuv_use_cnt++;
            ret = HI_SUCCESS;
            
            channel_sync_frame_buffer(pchan, pbuf->phy_addr, EXTBUF_TAKEN);
            
            OmxPrint(OMX_OUTBUF, "got frame: phy addr = 0x%08x\n", pframe->frame_phy_addr);
            break;
        }
    }

EXIT:
    spin_unlock_irqrestore(&pchan->yuv_lock, flags);

    return ret;
}

static HI_S32 processor_release_frame(OMXVDEC_CHAN_CTX *pchan, BPP_VIDEO_FRAME_S *pframe)
{
    unsigned long flags;
    HI_S32 is_find      = 0;
    HI_S32 is_del       = 0;
    HI_U32 phyaddr      = 0;
    OMXVDEC_BUF_S *pbuf = HI_NULL;
    OMXVDEC_BUF_S *ptmp = HI_NULL;
    OMXVDEC_BUF_DESC user_buf;

    memset(&user_buf, 0, sizeof(OMXVDEC_BUF_DESC)); /* unsafe_function_ignore: memset */

    phyaddr = pframe->frame_phy_addr;

    /* for we del list during, so use safe means */
    spin_lock_irqsave(&pchan->yuv_lock, flags);
    if (list_empty(&pchan->yuv_queue))
    {
        spin_unlock_irqrestore(&pchan->yuv_lock, flags);
        printk(KERN_ALERT "%s: list is empty\n", __func__);
        return 0;
    }

    list_for_each_entry_safe(pbuf, ptmp, &pchan->yuv_queue, list)
    {
        if (phyaddr == (pbuf->phy_addr + pbuf->offset))
        {
            if (pbuf->status != BUF_STATE_USING)
            {
                printk(KERN_ALERT "%s: buffer(0x%08x) flags(%d) confused!\n",__func__, phyaddr, pbuf->status);
            }

            if (pchan->output_flush_pending || pchan->pause_pending)
            {
                list_del(&pbuf->list);
                is_del = 1;
                pbuf->status = BUF_STATE_IDLE;
                channel_sync_frame_buffer(pchan, pbuf->phy_addr, EXTBUF_DEQUE);
            }
            else
            {
                pbuf->status = BUF_STATE_QUEUED;
                channel_sync_frame_buffer(pchan, pbuf->phy_addr, EXTBUF_QUEUE);
            }

            is_find = 1;
            pchan->yuv_use_cnt = (pchan->yuv_use_cnt > 0) ? (pchan->yuv_use_cnt-1) : 0;
            break;
        }
    }

    if (!is_find)
    {
        spin_unlock_irqrestore(&pchan->yuv_lock, flags);
        printk(KERN_ALERT "%s: buffer(0x%08x) not in queue!\n",__func__,  phyaddr);
        return HI_FAILURE;
    }

    if (1 == is_del)
    {
        user_buf.dir = PORT_DIR_OUTPUT;
        user_buf.bufferaddr  = pbuf->user_vaddr;
        user_buf.buffer_len  = pbuf->buf_len;
        user_buf.client_data = pbuf->client_data;
        user_buf.data_len = 0;
        user_buf.timestamp = 0;

        message_queue(pchan->msg_queue, VDEC_MSG_RESP_OUTPUT_DONE, VDEC_S_SUCCESS, (HI_VOID *)&user_buf);

        if (0 == pchan->yuv_use_cnt)
        {
            if (pchan->output_flush_pending)
            {
                message_queue(pchan->msg_queue, VDEC_MSG_RESP_FLUSH_OUTPUT_DONE, VDEC_S_SUCCESS, HI_NULL);
                pchan->output_flush_pending = 0;
            }

            if (pchan->pause_pending)
            {
                message_queue(pchan->msg_queue, VDEC_MSG_RESP_PAUSE_DONE, VDEC_S_SUCCESS, HI_NULL);
                pchan->pause_pending = 0;
            }
        }

        //OmxPrint(OMX_OUTBUF, "bpp release frame: phy addr = 0x%08x (delete)\n", phyaddr);
    }
/*
    else
    {
        OmxPrint(OMX_OUTBUF, "bpp release frame: phy addr = 0x%08x (requeue)\n", phyaddr);
    }
*/
    spin_unlock_irqrestore(&pchan->yuv_lock, flags);

    return HI_SUCCESS;
}

static HI_S32 processor_report_frame(OMXVDEC_CHAN_CTX *pchan, BPP_VIDEO_FRAME_S *pstFrame)
{
    unsigned long flags;
    HI_U32 phyaddr      = 0;
    HI_S32 is_find      = 0;
    OMXVDEC_BUF_S *pbuf = HI_NULL;
    OMXVDEC_BUF_S *ptmp = HI_NULL;
    OMXVDEC_BUF_DESC user_buf;

    phyaddr = pstFrame->frame_phy_addr;

    /* for we del list during, so use safe means */
    spin_lock_irqsave(&pchan->yuv_lock, flags);
    if (list_empty(&pchan->yuv_queue))
    {
        spin_unlock_irqrestore(&pchan->yuv_lock, flags);
        printk(KERN_ALERT "%s: list is empty\n", __func__);
        return HI_FAILURE;
    }

    list_for_each_entry_safe(pbuf, ptmp, &pchan->yuv_queue, list)
    {
        if (phyaddr == (pbuf->phy_addr + pbuf->offset))
        {
            if (pbuf->status != BUF_STATE_USING && pbuf->status != BUF_STATE_WAIT_REPORT)
            {
                printk(KERN_ALERT "%s: buffer(0x%08x) flags(%d) confused!\n", __func__, phyaddr, pbuf->status);
            }
            
            is_find = 1;
            pbuf->status = BUF_STATE_IDLE;
            list_del(&pbuf->list);

            channel_sync_frame_buffer(pchan, pbuf->phy_addr, EXTBUF_DEQUE);

            pchan->yuv_use_cnt = (pchan->yuv_use_cnt > 0) ? (pchan->yuv_use_cnt-1) : 0;
            break;
        }
    }

    if (!is_find)
    {
        spin_unlock_irqrestore(&pchan->yuv_lock, flags);
        printk(KERN_ALERT "%s: buffer(0x%08x) not in queue!\n", __func__,  phyaddr);
        return HI_FAILURE;
    }

    /* let out msg to inform application */
    memset(&user_buf, 0, sizeof(OMXVDEC_BUF_DESC)); /* unsafe_function_ignore: memset */
    memset(&pbuf->ext_info, 0, sizeof(EXT_INFO)); /* unsafe_function_ignore: memset */
    
    user_buf.dir                     = PORT_DIR_OUTPUT;
    user_buf.buffer_type             = pbuf->buf_type;
    user_buf.bufferaddr              = pbuf->user_vaddr;
    user_buf.buffer_len              = pbuf->buf_len;
    user_buf.client_data             = pbuf->client_data;
    user_buf.flags                   = VDEC_BUFFERFLAG_ENDOFFRAME;
    user_buf.phyaddr                 = pstFrame->frame_phy_addr;
    user_buf.out_frame.phyaddr_Y     = pstFrame->frame_phy_addr; 
    user_buf.out_frame.phyaddr_C     = pstFrame->frame_phy_addr + pstFrame->image.chroma_offset; 
    user_buf.out_frame.stride        = pstFrame->image.stride; 
    user_buf.out_frame.frame_width   = pstFrame->image.frame_width;
    user_buf.out_frame.frame_height  = pstFrame->image.frame_height;
    user_buf.out_frame.disp_width    = pstFrame->image.disp_width;
    user_buf.out_frame.disp_height   = pstFrame->image.disp_height;
    user_buf.out_frame.left_offset   = pstFrame->image.left_offset;
    user_buf.out_frame.right_offset  = pstFrame->image.right_offset;
    user_buf.out_frame.top_offset    = pstFrame->image.top_offset;
    user_buf.out_frame.bottom_offset = pstFrame->image.bottom_offset;
    user_buf.out_frame.format        = pstFrame->image.format;
    user_buf.out_frame.save_yuv      = g_SaveYuvEnable;

    if (1 == pchan->spec_mode)
    {
        memcpy(user_buf.out_frame.act_crc, pstFrame->image.ActCRC, sizeof(user_buf.out_frame.act_crc)); /* unsafe_function_ignore: memcpy */
    }

    if (HI_TRUE == user_buf.out_frame.save_yuv)
    {
        snprintf(user_buf.out_frame.save_path, sizeof(user_buf.out_frame.save_path),
                "%s/%s.yuv", g_SavePath, g_SaveName);
                user_buf.out_frame.save_path[PATH_LEN-1] = '\0';
    }

    if (pstFrame->is_eof == HI_TRUE)
    {
        /* last frame */
        user_buf.timestamp = 0;
        user_buf.data_len  = 0;
        user_buf.flags    |= VDEC_BUFFERFLAG_EOS;
        pchan->eof_send_count++;

        // OmxPrint(OMX_INFO, "report last frame, phyaddr = %x, timestamp = %lld\n", phyaddr, user_buf.timestamp);
    }
    else
    {
        if (pchan->output_flush_pending)
        {
            user_buf.timestamp = 0;
            user_buf.data_len  = 0;
            // OmxPrint(OMX_OUTBUF, "output flush pending, unrelease buffer num: %d\n", pchan->yuv_use_cnt);
        }
        else
        {
            user_buf.timestamp = pstFrame->image.timestamp; 
            user_buf.data_len  = MIN(pstFrame->frame_size, user_buf.buffer_len);
            
            pbuf->ext_info.PP_EXT.image_id   = (pstFrame->image.image_id != INVALID_IMAGE_ID)? (pstFrame->image.image_id + IMAGE_ID_OFFSET): 0;
            pchan->out_frame_num++;
        }
    }

    pbuf->act_len = user_buf.data_len;
    // OmxPrint(OMX_PTS, "Put Time Stamp: %lld\n", user_buf.timestamp);
    
    message_queue(pchan->msg_queue, VDEC_MSG_RESP_OUTPUT_DONE, VDEC_S_SUCCESS, (HI_VOID *)&user_buf);

    if (0 == pchan->yuv_use_cnt)
    {
        if (pchan->output_flush_pending)
        {
           message_queue(pchan->msg_queue, VDEC_MSG_RESP_FLUSH_OUTPUT_DONE, VDEC_S_SUCCESS, HI_NULL);
           pchan->output_flush_pending = 0;
        }

        if (pchan->pause_pending)
        {
           message_queue(pchan->msg_queue, VDEC_MSG_RESP_PAUSE_DONE, VDEC_S_SUCCESS, HI_NULL);
           pchan->pause_pending = 0;
        }
    }
    
    spin_unlock_irqrestore(&pchan->yuv_lock, flags);
    // OmxPrint(OMX_OUTBUF, "bpp report frame: phy addr 0x%08x, data_len %d\n", phyaddr, user_buf.data_len);

    return HI_SUCCESS;
}

static HI_S32 processor_try_send_eof(OMXVDEC_CHAN_CTX *pchan)
{
    HI_S32 ret;
    BPP_VIDEO_FRAME_S stLastFrame;

    memset(&stLastFrame, 0, sizeof(stLastFrame)); /* unsafe_function_ignore: memset */

    ret = processor_get_frame(pchan, &stLastFrame, 0);
    if (HI_SUCCESS == ret)
    {
        pchan->processor_eof_state = PRC_EOF_GOT_SLOT;
        stLastFrame.is_eof = HI_TRUE;
        ret = processor_report_frame(pchan, &stLastFrame);
        if (HI_SUCCESS == ret)
        {
            OmxPrint(OMX_INFO, "Processor report eof frame.\n");
            pchan->processor_eof_state = PRC_EOF_REPORT_SUCCESS;
        }
        else
        {
            OmxPrint(OMX_ERR, "Processor report eof frame failed!\n");
            processor_release_frame(pchan, &stLastFrame);
            pchan->processor_eof_state = PRC_EOF_REPORT_FAILED;
        }
    }
    else
    {
        OmxPrint(OMX_INFO, "No free frame slot for eof frame, wait for a while.\n");
        pchan->processor_eof_state = PRC_EOF_WAIT_SLOT;
    }

    return ret;
}

static HI_S32 processor_get_image(OMXVDEC_CHAN_CTX *pchan, BPP_VIDEO_FRAME_S *pstFrame)
{
    HI_S32 ret;
    IMAGE  stImage;

    if(pchan->reset_pending)
    {
        OmxPrint(OMX_VPSS, "%s channel reset pending\n", __func__);
        return HI_FAILURE;
    }
    
    ret = pchan->image_ops.read_image(pchan->decoder_id, &stImage);
    if(ret != HI_SUCCESS)
    {
        OmxPrint(OMX_VPSS, "bpp read image failed\n");
        return NO_NEW_IMAGE;
    }

    pstFrame->image.chroma_offset = stImage.top_chrom_phy_addr - stImage.top_luma_phy_addr;    
    pstFrame->image.frame_width   = stImage.image_width;
    pstFrame->image.frame_height  = stImage.image_height;
    pstFrame->image.disp_width    = stImage.disp_width;
    pstFrame->image.disp_height   = stImage.disp_height;
    pstFrame->image.left_offset   = stImage.left_offset;
    pstFrame->image.right_offset  = stImage.right_offset;
    pstFrame->image.top_offset       = stImage.top_offset;
    pstFrame->image.bottom_offset = stImage.bottom_offset;
    pstFrame->image.stride           = stImage.image_stride/16;
    pstFrame->image.format        = stImage.format;
    pstFrame->image.timestamp     = (HI_S64)stImage.PTS;
    pstFrame->image.image_id      = stImage.image_id;
    pstFrame->image.is_cur_last   = stImage.is_cur_last;
    pstFrame->image.is_last_frame = stImage.last_frame;

    pstFrame->frame_phy_addr      = stImage.top_luma_phy_addr;
    pstFrame->frame_size          = processor_calculate_data_len(pstFrame->image.chroma_offset, pchan->color_format); 

    if (1 == pchan->spec_mode)
    {
        memcpy(pstFrame->image.ActCRC, stImage.ActualCRC, sizeof(pstFrame->image.ActCRC)); /* unsafe_function_ignore: memcpy */
    }

    pchan->pic_format = stImage.format;
    pchan->last_processor_got_image_id = REALID_WITH_EOF_OFFSET(stImage.image_id);

    return HI_SUCCESS;
}

static HI_S32 processor_release_image(OMXVDEC_CHAN_CTX *pchan, BPP_VIDEO_FRAME_S *pstFrame)
{
    HI_S32 ret;
    IMAGE stImage;

    memset(&stImage, 0, sizeof(stImage)); /* unsafe_function_ignore: memset */
    stImage.luma_phy_addr        = pstFrame->frame_phy_addr;
    stImage.top_luma_phy_addr    = pstFrame->frame_phy_addr;
    stImage.image_id             = pstFrame->image.image_id;
    stImage.image_id_1           = -1;

    ret = pchan->image_ops.release_image(pchan->decoder_id, &stImage);
    if(ret != HI_SUCCESS)
    {
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static HI_S32 processor_ctrl_release_image(OMXVDEC_CHAN_CTX *pchan, OMXVDEC_BUF_S *pBuf)
{
    HI_S32 ret;
    BPP_VIDEO_FRAME_S stFrame;
    
    memset(&stFrame, 0x00, sizeof(stFrame)); /* unsafe_function_ignore: memset */
    stFrame.frame_phy_addr = pBuf->phy_addr;
    stFrame.image.image_id = pBuf->ext_info.PP_EXT.image_id - IMAGE_ID_OFFSET;
        
    ret = processor_release_image(pchan, &stFrame);
    if(ret != HI_SUCCESS)
    {
        return HI_FAILURE;
    }
    
    return HI_SUCCESS;
}

static HI_S32 processor_ctrl_thread_wake_up(OMXVDEC_CHAN_CTX *pchan)
{
    BPP_CONTEXT_S *pBppContext = HI_NULL;
    
    CHECK_PROCESSOR_ID(pchan->processor_id);

    pBppContext = gpBPPContext[pchan->processor_id];
    VDEC_ASSERT_RETURN(pBppContext == HI_NULL);

    if (BPP_THREAD_SLEEPING == pBppContext->thread_state)
    {
        pBppContext->thread_state = BPP_THREAD_ONCALL;
        wake_up(&pBppContext->task_wait);
    }

    return HI_SUCCESS;
}

static HI_S32 processor_ctrl_send_eof(OMXVDEC_CHAN_CTX *pchan)
{
    return processor_try_send_eof(pchan);
}

static HI_S32 processor_ctrl_is_buffer_usable(OMXVDEC_CHAN_CTX *pchan, HI_U32 phy_addr)
{
    unsigned long  flags;
    HI_S32         is_find = 0;
    OMXVDEC_BUF_S *pbuf    = HI_NULL;

    spin_lock_irqsave(&pchan->yuv_lock, flags);
    if (list_empty(&pchan->yuv_queue))
    {
        spin_unlock_irqrestore(&pchan->yuv_lock, flags);
        return 0;
    }

    list_for_each_entry(pbuf, &pchan->yuv_queue, list)
    {
        if (phy_addr == pbuf->phy_addr)
        {
            if (BUF_STATE_QUEUED == pbuf->status)
            {
                is_find = 1;
                break;
            }
        }
    }
    spin_unlock_irqrestore(&pchan->yuv_lock, flags);

    return is_find;
}

static HI_S32 processor_ctrl_set_buffer_wait_report(OMXVDEC_CHAN_CTX *pchan, HI_U32 phy_addr)
{    
    HI_S32 ret = HI_FAILURE;
    unsigned long flags;
    OMXVDEC_BUF_S *pbuf = HI_NULL;
    
    spin_lock_irqsave(&pchan->yuv_lock, flags);
    if (list_empty(&pchan->yuv_queue))
    {
        OmxPrint(OMX_VPSS, "%s List is empty!\n", __func__);
        goto EXIT;
    }

    list_for_each_entry(pbuf, &pchan->yuv_queue, list)
    {
        if (phy_addr == pbuf->phy_addr)
        {
            if (BUF_STATE_QUEUED == pbuf->status)
            {
                pbuf->status = BUF_STATE_WAIT_REPORT;
                pchan->yuv_use_cnt++;
                ret = HI_SUCCESS;
                
                channel_sync_frame_buffer(pchan, pbuf->phy_addr, EXTBUF_TAKEN);
                break;
            }
            else
            {
                OmxPrint(OMX_ERR, "%s: buffer(0x%08x) flags(%d) confused!\n",__func__, phy_addr, pbuf->status);
            }
        }
    }

EXIT:
    spin_unlock_irqrestore(&pchan->yuv_lock, flags);
    return ret;
}

static HI_S32 processor_ctrl_wait_handle_remain_frame(OMXVDEC_CHAN_CTX *pchan)
{
    HI_S32 count;
    BPP_CONTEXT_S *pBppContext = HI_NULL;

    CHECK_PROCESSOR_ID(pchan->processor_id);

    pBppContext = gpBPPContext[pchan->processor_id];
	VDEC_ASSERT_RETURN(pBppContext == HI_NULL);

    for (count=0; count<400; count++)
    {
        if (pBppContext->stFrame.is_valid != HI_FALSE)
        {
            msleep(5);
        }
        else
        {
            break;
        }
    }

    if (count >= 100)
    {
        OmxPrint(OMX_ERR, "processor wait handle remain frame timeout!\n");
        pBppContext->stFrame.is_valid = HI_FALSE;
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static HI_BOOL processor_need_handle_eof(OMXVDEC_CHAN_CTX *pchan, BPP_VIDEO_FRAME_S *pCurFrame)
{
    if (pchan->processor_eof_state == PRC_EOF_INIT_STATE)
    {
        if (pCurFrame->image.is_last_frame == 1)
        {
            return HI_TRUE;
        }

        if (DEC_EOF_REPORT_FAILED == pchan->decoder_eof_state)
        {
            return HI_TRUE;
        }

        if (DEC_EOF_REPORT_ID == pchan->decoder_eof_state)
        {
            OmxPrint(OMX_INFO, "check image id: report %d, last handle %d\n", pchan->last_frame_image_id, pchan->last_processor_got_image_id);
            if (pchan->last_frame_image_id == pchan->last_processor_got_image_id)
            {
                return HI_TRUE;
            }
        }
    }

    return HI_FALSE;
}

static HI_BOOL processor_is_size_changed(OMXVDEC_CHAN_CTX *pchan, BPP_VIDEO_FRAME_S *pInFrame)
{
    HI_U32 new_stride = ALIGN_UP(pInFrame->image.frame_width, DEFAULT_ALIGN_STEP);

    if (pchan->out_width  != pInFrame->image.frame_width
     || pchan->out_height != pInFrame->image.frame_height
     || pchan->out_stride != new_stride)
    {
        OmxPrint(OMX_INFO, "%s: width %d -> %d, height %d -> %d, stride %d -> %d\n", __func__, pchan->out_width, pInFrame->image.frame_width, pchan->out_height, pInFrame->image.frame_height, pchan->out_stride, new_stride);
        return HI_TRUE;
    }

    if (pchan->output_buf_size < pInFrame->frame_size)
    {
        OmxPrint(OMX_INFO, "%s: buf_size %d -> %d", __func__, pchan->output_buf_size, pInFrame->frame_size);
        return HI_TRUE;
    }

    return HI_FALSE;
}

static HI_S32 processor_restore_frame_without_gap(BPP_VIDEO_FRAME_S *pSrcFrame, BPP_VIDEO_FRAME_S *pDstFrame)
{
    HI_U32 i;
    HI_S32 ret;
    HI_U8 *pSrcYAddr = HI_NULL;
    HI_U8 *pSrcCAddr = HI_NULL;
    HI_U8* pDstAddr  = HI_NULL;
    HI_U32 Width, Height, SrcStride, SrcDataLen, DstStride, DstDataLen;

    Width     = pSrcFrame->image.frame_width;
    Height    = pSrcFrame->image.frame_height;
    SrcStride = pSrcFrame->image.stride;
    DstStride = ALIGN_UP(Width, DEFAULT_ALIGN_STEP);

    SrcDataLen = SrcStride * Height * 3 / 2;
    if (SrcDataLen > pSrcFrame->frame_size)
    {
        OmxPrint(OMX_ERR, "%s SrcDataLen %d > SrcFrameSize %d\n", __func__, SrcDataLen, pSrcFrame->frame_size);
        return HI_FAILURE;
    }

    DstDataLen = DstStride * Height * 3 / 2;
    if (DstDataLen > pDstFrame->frame_size)
    {
        OmxPrint(OMX_ERR, "%s DstDataLen %d > DstFrameSize %d\n", __func__, DstDataLen, pDstFrame->frame_size);
        return HI_FAILURE;
    }

    ret = VDEC_MEM_GetVirAddr_FromPhyAddr(&pSrcYAddr, pSrcFrame->frame_phy_addr, pSrcFrame->frame_size);
    if (ret == HI_FAILURE)
    {
        OmxPrint(OMX_ERR, "%s get src vir from phy 0x%x failed!\n", __func__, pSrcFrame->frame_phy_addr);
        return HI_FAILURE;
    }
    pSrcCAddr = pSrcYAddr + pSrcFrame->image.chroma_offset;

    ret = VDEC_MEM_GetVirAddr_FromPhyAddr(&pDstAddr, pDstFrame->frame_phy_addr, pDstFrame->frame_size);
    if (ret == HI_FAILURE)
    {
        OmxPrint(OMX_ERR, "%s get dst vir from phy 0x%x failed!\n", __func__, pDstFrame->frame_phy_addr);
        return HI_FAILURE;
    }

    memcpy(&pDstFrame->image, &pSrcFrame->image, sizeof(BPP_IMAGE_INFO_S)); /* unsafe_function_ignore: memcpy */

    /*restore y*/
    for (i=0; i<Height; i++)
    {
        memcpy(pDstAddr, pSrcYAddr, Width); /* unsafe_function_ignore: memcpy */
        pSrcYAddr += SrcStride;
        pDstAddr  += DstStride;
    }

    /*restore uv*/
    for (i=0; i<Height/2; i++)
    {
        memcpy(pDstAddr, pSrcCAddr, Width); /* unsafe_function_ignore: memcpy */
        pSrcCAddr += SrcStride;
        pDstAddr  += DstStride;
    }

    pDstFrame->image.image_id      = INVALID_IMAGE_ID;
    pDstFrame->image.stride        = DstStride;
    pDstFrame->image.chroma_offset = DstStride * Height;
    return HI_SUCCESS;
}

static HI_S32 processor_work_in_normal_mode(OMXVDEC_CHAN_CTX *pchan, BPP_CONTEXT_S *pBppContext)
{
    HI_S32 ret;
    BPP_VIDEO_FRAME_S *pstInFrame = HI_NULL;
    BPP_VIDEO_FRAME_S  stOutFrame;

    memset(&stOutFrame, 0x0, sizeof(BPP_VIDEO_FRAME_S)); /* unsafe_function_ignore: memset */

    if (HI_TRUE == pchan->is_tvp)
    {
        printk(KERN_ALERT "%s chan %d is tvp, should not work in normal mode!\n", __func__, pchan->channel_id);
        return HI_FAILURE; 
    }

    // get src image
    pstInFrame = &pBppContext->stFrame;
    if (pstInFrame->is_valid != HI_TRUE)
    {
        ret = processor_get_image(pchan, pstInFrame);
        if (ret != HI_SUCCESS)
        {
            printk(KERN_ALERT "%s get image failed!\n", __func__);
            goto EXIT;
        }

        pstInFrame->is_valid = HI_TRUE;

        // check if size changed
        if (HI_TRUE == processor_is_size_changed(pchan, pstInFrame))
        {
            processor_report_size_change_normal(pchan, pstInFrame);
            return HI_FAILURE;
        }
    }
    
    // get dst frame
    ret = processor_get_frame(pchan, &stOutFrame, pstInFrame->frame_size);
    if (ret != HI_SUCCESS)
    {
        OmxPrint(OMX_VPSS, "%s get output frame failed!\n", __func__);
        return HI_FAILURE;
    }

    // restore src image into dst frame
    ret = processor_restore_frame_without_gap(pstInFrame, &stOutFrame);
    if (ret != HI_SUCCESS)
    {
        printk(KERN_ALERT "%s converse frame failed!\n", __func__);
    }
    
    // release src image
    ret = processor_release_image(pchan, pstInFrame);
    if (ret != HI_SUCCESS)
    {
        OmxPrint(OMX_WARN, "%s release image failed!\n", __func__);
    }

    // report dst frame
    ret = processor_report_frame(pchan, &stOutFrame);
    if (ret != HI_SUCCESS)
    {
        printk(KERN_ALERT "%s report frame failed!\n", __func__);
    }
    pstInFrame->is_valid = HI_FALSE;
    
EXIT:
    if (HI_TRUE == processor_need_handle_eof(pchan, pstInFrame))
    {
        processor_try_send_eof(pchan);
    }
        
    return ret;
}

static HI_S32 processor_work_in_bypass_mode(OMXVDEC_CHAN_CTX *pchan, BPP_CONTEXT_S *pBppContext)
{
    HI_S32 ret;
    HI_BOOL have_more_image = HI_FALSE;
    BPP_VIDEO_FRAME_S stFrame;

    // get image
    memset(&stFrame, 0, sizeof(BPP_VIDEO_FRAME_S)); /* unsafe_function_ignore: memset */
    ret = processor_get_image(pchan, &stFrame);
    if (ret != HI_SUCCESS)
    {
        OmxPrint(OMX_VPSS, "%s get image failed!\n", __func__);
        goto EXIT;
    }

    // report frame
    ret = processor_report_frame(pchan, &stFrame);
    if (ret != HI_SUCCESS)
    {
        OmxPrint(OMX_ERR, "%s report frame failed!\n", __func__);
        processor_release_image(pchan, &stFrame);
    }

    // no need next try if is cur last image
    have_more_image = (1 == stFrame.image.is_cur_last)? HI_FALSE: HI_TRUE;

EXIT:
    if (HI_FALSE == have_more_image)
    {
        if (DFS_WAIT_CLEAR == pchan->dfs_alloc_flag)
        {
            processor_report_size_change_bypass(pchan);
        }
        
        if (HI_TRUE == processor_need_handle_eof(pchan, &stFrame))
        {
            processor_try_send_eof(pchan);
        }
    }
    
    return (have_more_image == HI_TRUE)? HI_SUCCESS: HI_FAILURE;
}

static HI_S32 processor_do_one_work(OMXVDEC_CHAN_CTX *pchan, BPP_CONTEXT_S *pBppContext)
{
    if (PATH_MODE_NORMAL == pchan->path_mode)
    {
        // Under normal path mode, output frame without gap.
        return processor_work_in_normal_mode(pchan, pBppContext);
    }
    else if (PATH_MODE_NATIVE == pchan->path_mode)
    {
        return processor_work_in_bypass_mode(pchan, pBppContext);
    }
    else
    {
        OmxPrint(OMX_ERR, "%s unkown path mode %d\n", __func__, pchan->path_mode);
        return HI_FAILURE;
    }
}

static HI_S32 processor_thread(HI_VOID* param)
{
    HI_S32 ret = HI_SUCCESS;
    OMXVDEC_CHAN_CTX *pchan       = HI_NULL;
    BPP_CONTEXT_S    *pBppContext = (BPP_CONTEXT_S *)param;    

    VDEC_ASSERT_RETURN(pBppContext == HI_NULL);
    VDEC_ASSERT_RETURN(pBppContext->pMaster == HI_NULL);   

    pchan = pBppContext->pMaster;

    while (!kthread_should_stop())
    {   
        pBppContext->thread_state = BPP_THREAD_SLEEPING;
        ret = wait_event_timeout(pBppContext->task_wait, (BPP_THREAD_ONCALL == pBppContext->thread_state), (msecs_to_jiffies(100))); /*lint !e666*/
        pBppContext->thread_state = BPP_THREAD_RUNNING;      

        if (BPP_INST_WORK == pBppContext->inst_state && CHAN_STATE_WORK == pchan->channel_state)
        {
            do {    
                ret = processor_do_one_work(pchan, pBppContext);
                if (ret != HI_SUCCESS)
                {
                    //OmxPrint(OMX_VPSS, "chan %d processor_do_one_work failed\n", pchan->processor_id);
                    break;
                }     
            }while(1);
        }
    }   

    pBppContext->thread_state = BPP_THREAD_EXIT;

    return ret;
}

/*=============== EXPORT FUNCTION ==============*/
HI_S32 processor_probe(HI_VOID)
{
    VDEC_INIT_MUTEX(&gBPPMut);

    return HI_SUCCESS;
}

HI_S32 processor_init(HI_VOID)
{
    VDEC_DOWN_INTERRUPTIBLE(&gBPPMut);

    memset(gpBPPContext, 0, MAX_CHANNEL_NUM * sizeof(BPP_CONTEXT_S*)); /* unsafe_function_ignore: memset */ /* [false alarm] */
    g_BPPInit = 1;

    VDEC_UP_INTERRUPTIBLE(&gBPPMut);

    return HI_SUCCESS;
}

HI_S32 processor_exit(HI_VOID)
{
    HI_U32 i;
    HI_S32 ret = HI_FAILURE;

    VDEC_DOWN_INTERRUPTIBLE(&gBPPMut);

    for(i=0; i<MAX_CHANNEL_NUM; i++)
    {
        if(gpBPPContext[i] != HI_NULL && gpBPPContext[i]->inst_state != BPP_INST_INVALID)
        {
            ret = processor_release_inst(gpBPPContext[i]->pMaster);
            if (ret != HI_SUCCESS)
            {
                OmxPrint(OMX_FATAL, "%s release inst %d failed!\n", __func__, i);
            }
        }
    }
    g_BPPInit = 0;

    VDEC_UP_INTERRUPTIBLE(&gBPPMut);
    
    return HI_SUCCESS;
}

HI_S32 processor_suspend(HI_VOID)
{
    HI_S32 i;
    HI_S32 ret;
    BPP_CONTEXT_S *pBppContext = HI_NULL;
    
    VDEC_DOWN_INTERRUPTIBLE(&gBPPMut);

    if (g_BPPInit == 1)
    {
        for (i=0; i<MAX_CHANNEL_NUM; i++)
        {
            pBppContext = gpBPPContext[i];
            if (pBppContext == HI_NULL)
            {
                continue;
            }

            if (pBppContext->inst_state != BPP_INST_WORK)
            {
                continue;
            }

            pBppContext->inst_state = BPP_INST_SUSPEND;
            ret = processor_stop_thread(pBppContext);
            if (ret != HI_SUCCESS)
            {
                OmxPrint(OMX_ERR, "%s stop inst %d thread failed!\n", __func__, i);
            }
        }
    }

    VDEC_UP_INTERRUPTIBLE(&gBPPMut);

    return HI_SUCCESS;
}

HI_S32 processor_resume(HI_VOID)
{
    HI_S32 i;
    BPP_CONTEXT_S *pBppContext = HI_NULL;
    
    VDEC_DOWN_INTERRUPTIBLE(&gBPPMut);
    
    if (g_BPPInit == 1)
    {
        for (i=0; i<MAX_CHANNEL_NUM; i++)
        {
            pBppContext = gpBPPContext[i];
            if (pBppContext == HI_NULL)
            {
                continue;
            }

            if (pBppContext->inst_state != BPP_INST_SUSPEND)
            {
                continue;
            }
            pBppContext->inst_state = BPP_INST_WORK;
        }
    }

    VDEC_UP_INTERRUPTIBLE(&gBPPMut);

    return HI_SUCCESS;
}

HI_S32 processor_create_inst(OMXVDEC_CHAN_CTX *pchan, HI_U32 color_format)
{
    HI_U32  i = 0;
    HI_S32  ret = HI_FAILURE;
    HI_CHAR name_array[20];
    MEM_BUFFER_S mem_buf;
    BPP_CONTEXT_S *pBppContext = HI_NULL;

    VDEC_DOWN_INTERRUPTIBLE(&gBPPMut);

    pchan->color_format = color_format;
    pchan->processor_id = -1;

    for (i=0; i<MAX_CHANNEL_NUM; i++)
    {
        if (gpBPPContext[i] == HI_NULL)
        {
            break;
        }
    }
    
    if (i >= MAX_CHANNEL_NUM)
    {
        printk(KERN_ALERT "%s no free context slot!\n", __func__);
        ret = HI_FAILURE;
    }
    else
    {
        /* alloc mem for bpp context */
        memset(&mem_buf, 0, sizeof(MEM_BUFFER_S)); /* unsafe_function_ignore: memset */
        mem_buf.u8IsCached = 1;
        mem_buf.u32Size = sizeof(BPP_CONTEXT_S);
        snprintf(name_array, sizeof(name_array), "Chan%d_BPP", pchan->channel_id); /* unsafe_function_ignore: snprintf */
        ret = VDEC_MEM_AllocAndMap(name_array, OMXVDEC_ZONE, &mem_buf);
        if (ret != HI_SUCCESS)
        {
            VDEC_UP_INTERRUPTIBLE(&gBPPMut);
            printk(KERN_ALERT "%s alloc memory for bpp context failed.\n", __func__);
            return HI_FAILURE;
        }
        memset(mem_buf.pStartVirAddr, 0, mem_buf.u32Size); /* unsafe_function_ignore: memset */
        
        pBppContext = (BPP_CONTEXT_S *)mem_buf.pStartVirAddr;
        memcpy(&pBppContext->mem_buf, &mem_buf, sizeof(MEM_BUFFER_S)); /* unsafe_function_ignore: memcpy */

        pBppContext->inst_state = BPP_INST_IDLE;
        pBppContext->inst_id    = i;
        pBppContext->pMaster    = pchan;

        init_waitqueue_head(&pBppContext->task_wait);
        snprintf(name_array, sizeof(name_array), "BPP_Thread_%d", i); /* unsafe_function_ignore: snprintf */
        pBppContext->pThread = kthread_create(processor_thread, (HI_VOID *)pBppContext, name_array);
        if (HI_NULL == pBppContext->pThread || IS_ERR(pBppContext->pThread))
        {
            VDEC_MEM_UnmapAndRelease(&pBppContext->mem_buf);
            printk(KERN_ALERT "Create processor_thread %d failed!\n", i);
            ret = HI_FAILURE;
        }
        else
        {
            gpBPPContext[i] = pBppContext;
            pchan->processor_id = pBppContext->inst_id;
            wake_up_process(pBppContext->pThread);
            ret = HI_SUCCESS;
        }
    }

    VDEC_UP_INTERRUPTIBLE(&gBPPMut);

    return ret;
}

HI_S32 processor_release_inst(OMXVDEC_CHAN_CTX *pchan)
{
    BPP_CONTEXT_S *pBppContext = HI_NULL;

    CHECK_PROCESSOR_ID(pchan->processor_id);
    
    VDEC_DOWN_INTERRUPTIBLE(&gBPPMut);
        
    pBppContext = gpBPPContext[pchan->processor_id];
    if (pBppContext == HI_NULL)
    {
        VDEC_UP_INTERRUPTIBLE(&gBPPMut);
        printk(KERN_ALERT "%s: gpBPPContext[%d] = NULL!\n", __func__, pchan->processor_id);
        return HI_FAILURE;
    }
    
    pBppContext->pMaster    = HI_NULL;
    pBppContext->inst_state = BPP_INST_INVALID;
    
    if (pBppContext->pThread != HI_NULL)
    {
        if (pBppContext->thread_state != BPP_THREAD_EXIT)
        {
            kthread_stop(pBppContext->pThread);
        }
        
        if (pBppContext->thread_state != BPP_THREAD_EXIT)
        {
            printk(KERN_ALERT "FATAL: BPP %d thread state %d should be EXIT by now!\n", pchan->processor_id, pBppContext->thread_state);
        }
        pBppContext->pThread = HI_NULL;
    }
    
    VDEC_MEM_UnmapAndRelease(&pBppContext->mem_buf);
    gpBPPContext[pchan->processor_id] = HI_NULL;

    VDEC_UP_INTERRUPTIBLE(&gBPPMut);
        
    return HI_SUCCESS;
}

HI_S32 processor_start_inst(OMXVDEC_CHAN_CTX *pchan)
{
    BPP_CONTEXT_S *pBppContext = HI_NULL;

    CHECK_PROCESSOR_ID(pchan->processor_id);
        
    pBppContext = gpBPPContext[pchan->processor_id];
    VDEC_ASSERT_RETURN(pBppContext == HI_NULL);
    
    pBppContext->inst_state = BPP_INST_WORK;

    return HI_SUCCESS;
}

HI_S32 processor_stop_inst(OMXVDEC_CHAN_CTX *pchan)
{
    HI_S32 ret;
    BPP_CONTEXT_S *pBppContext = HI_NULL;
    
    CHECK_PROCESSOR_ID(pchan->processor_id);
    
    pBppContext = gpBPPContext[pchan->processor_id];
    VDEC_ASSERT_RETURN(pBppContext == HI_NULL);

    pBppContext->inst_state = BPP_INST_IDLE;
    
    ret = processor_stop_thread(pBppContext);
    if (ret != HI_SUCCESS)
    {
        OmxPrint(OMX_ERR, "processor_stop_thread failed!\n");
        return HI_FAILURE;
    }
            
    return HI_SUCCESS;
}

HI_S32 processor_reset_inst(OMXVDEC_CHAN_CTX *pchan)
{
    BPP_CONTEXT_S *pBppContext = HI_NULL;
    
    CHECK_PROCESSOR_ID(pchan->processor_id);

    pBppContext = gpBPPContext[pchan->processor_id];
    VDEC_ASSERT_RETURN(pBppContext == HI_NULL);

    memset(&pBppContext->stFrame, 0, sizeof(BPP_VIDEO_FRAME_S)); /* unsafe_function_ignore: memset */

    return HI_SUCCESS;
}

HI_S32 processor_ctrl_inst(OMXVDEC_CHAN_CTX *pchan, PROCESSOR_CMD_E eCmd, HI_VOID *pParam)
{
    HI_S32 ret;
    
    VDEC_ASSERT_RETURN(pchan == HI_NULL);
    
    switch(eCmd)
    {
        case PP_CMD_RELEASE_IMAGE:
        {
            VDEC_ASSERT_RETURN(pParam == HI_NULL);
            ret = processor_ctrl_release_image(pchan, (OMXVDEC_BUF_S *)pParam);
            break;    
        }

        case PP_CMD_THREAD_WAKE_UP:
        { 
            ret = processor_ctrl_thread_wake_up(pchan);
            break;    
        }

        case PP_CMD_INFO_EOF_SLOT:
        {
            ret = processor_ctrl_send_eof(pchan);
            break;    
        }

        case PP_CMD_IS_BUF_USABLE:
        {
            VDEC_ASSERT_RETURN(pParam == HI_NULL);
            ret = processor_ctrl_is_buffer_usable(pchan, *(UADDR *)pParam);
            break;    
        }
        
        case PP_CMD_REPORT_BUF_OUT:
        {
            VDEC_ASSERT_RETURN(pParam == HI_NULL);
            ret = processor_ctrl_set_buffer_wait_report(pchan, *(UADDR *)pParam);
            break;    
        }
        
		case PP_CMD_WAIT_HANDLE:
	    {
			ret = processor_ctrl_wait_handle_remain_frame(pchan);
			break;
		}

        default:
        {
            printk(KERN_ALERT "%s unkown command id %d\n", __func__, eCmd);
            return HI_FAILURE;    
        }    
    }

    return ret;
}

HI_VOID processor_proc_entry(OMXVDEC_CHAN_CTX *pchan, struct seq_file *p)
{
    HI_U32 source_format = (pchan->pic_format & 0x300)>>8;
    HI_U32 field_valid   = (pchan->pic_format & 0xC00)>>10;
    HI_U32 field_first   = (pchan->pic_format & 0x3000)>>12;
    BPP_CONTEXT_S *pBppContext = gpBPPContext[pchan->processor_id];
        
    PROC_PRINT(p, "%-20s :%d\n",   "Processor",       pchan->processor_id);

    if (pBppContext != HI_NULL)
    {
    PROC_PRINT(p, "%-20s :%s\n",   "ThreadState",     processor_show_thread_state(pBppContext->thread_state));
    }
    else
    {
    PROC_PRINT(p, "%-20s :%s\n",   "ThreadState",     "Bpp context null!!");
    }
    
    PROC_PRINT(p, "%-20s :%d\n",   "OutBufUse",       pchan->yuv_use_cnt);
    PROC_PRINT(p, "%-20s :%d\n",   "OutFrameNum",     pchan->out_frame_num);
    PROC_PRINT(p, "%-20s :%s\n",   "P/I",             processor_show_progressive(source_format));
    if (1 == source_format || 3 == source_format)
    {
    PROC_PRINT(p, "%-20s :%s\n",   "FieldValid",      processor_show_field_valid(field_valid));
    PROC_PRINT(p, "%-20s :%s\n",   "FieldFirst",      processor_show_field_first(field_first));
    }
    PROC_PRINT(p, "%-20s :%s\n",   "EOFState",        processor_show_eof_state(pchan->processor_eof_state));
    PROC_PRINT(p, "\n");
}

