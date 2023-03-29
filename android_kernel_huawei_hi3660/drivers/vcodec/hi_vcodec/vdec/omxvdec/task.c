

#include "task.h"
#include "decoder.h"
#include "processor.h"

/*================ EXTERN VALUE ================*/
extern HI_U32   g_ExtraDispNum;
extern HI_U32   g_ClientReservedNum;


/*============== INTERNAL FUNCTION =============*/
static inline const HI_PCHAR task_show_state(eTASK_STATE state)
{
    switch (state)
    {
       case TASK_STATE_INVALID:
            return "invalid";

       case TASK_STATE_SLEEPING:
            return "sleeping";

       case TASK_STATE_ONCALL:
            return "oncall";

       case TASK_STATE_RUNNING:
            return "running";

       default:
            return "unkown";
    }
}

static HI_S32 task_alloc_channel_mem(OMXVDEC_CHAN_CTX *pchan)
{
    HI_S32 i;
    HI_S32 ret;
    HI_U32 mem_offset;
    HI_U32 total_frame_num;
    HI_CHAR buf_name[20];
    VDEC_CHAN_FRAME_STORES stFsParam;

    memset(&stFsParam, 0, sizeof(stFsParam)); /* unsafe_function_ignore: memset */

    pchan->dfs_alloc_flag = DFS_WAIT_CLEAR;

    /* Wait for pp to hanlde remain frame */
    ret = processor_ctrl_inst(pchan, PP_CMD_WAIT_HANDLE, HI_NULL);
    if(ret != HI_SUCCESS)
    {
        OmxPrint(OMX_FATAL, "%s wait processor handle remain frame failed!\n", __func__);
        return HI_FAILURE;
    }

    /* Try to release old buffer */
    omxvdec_release_mem(&pchan->decoder_vdh_buf);

    pchan->dfs_alloc_flag = DFS_WAIT_BIND;

    total_frame_num = pchan->seq_info.ref_frame_num + g_ExtraDispNum;

    pchan->decoder_vdh_buf.u8IsSecure = (HI_TRUE==pchan->is_tvp)? 1 : 0;
    pchan->decoder_vdh_buf.u32Size    = total_frame_num * (pchan->seq_info.ref_frame_size + pchan->seq_info.ref_pmv_size);
    snprintf(buf_name, sizeof(buf_name), "Chan%d_VDH", pchan->channel_id);
    ret = VDEC_MEM_AllocAndMap(buf_name, OMXVDEC_ZONE, &pchan->decoder_vdh_buf);
    if(ret != HI_SUCCESS)
    {
        OmxPrint(OMX_FATAL, "%s alloc vdh mem(size = %d) failed!\n", __func__, pchan->decoder_vdh_buf.u32Size);
        return HI_FAILURE;
    }

    /* Record allocated buffer message */
    stFsParam.TotalFrameNum = total_frame_num;
    mem_offset = 0;
    for(i=0; i<(HI_S32)stFsParam.TotalFrameNum; i++)
    {
        //Frame buffer info
        stFsParam.Node[i].FrmPhyAddr = pchan->decoder_vdh_buf.u32StartPhyAddr + mem_offset;
        stFsParam.Node[i].FrmVirAddr = (HI_VIRT_ADDR_T)((HI_U8 *)pchan->decoder_vdh_buf.pStartVirAddr + mem_offset);
        stFsParam.Node[i].FrmLength  = pchan->seq_info.ref_frame_size;
        mem_offset += pchan->seq_info.ref_frame_size;

        //Pmv buffer info
        stFsParam.Node[i].PmvPhyAddr = pchan->decoder_vdh_buf.u32StartPhyAddr + mem_offset;
        stFsParam.Node[i].PmvVirAddr = (HI_VIRT_ADDR_T)((HI_U8 *)pchan->decoder_vdh_buf.pStartVirAddr + mem_offset);
        stFsParam.Node[i].PmvLength  = pchan->seq_info.ref_pmv_size;
        mem_offset += pchan->seq_info.ref_pmv_size;
    }

    /* Bind buffer to decoder */
    ret = decoder_ctrl_inst(pchan, DEC_CMD_BIND_MEM, (HI_VOID*)&stFsParam);
    if(ret != HI_SUCCESS)
    {
        OmxPrint(OMX_ERR, "%s bind mem to inst failed!\n", __func__);
        omxvdec_release_mem(&pchan->decoder_vdh_buf);
        return HI_FAILURE;
    }

    pchan->dfs_alloc_flag = DFS_WAIT_ACTIVATE;

    ret = channel_activate_inst(pchan);
    if (ret != HI_SUCCESS)
    {
        OmxPrint(OMX_ERR, "%s activate inst failed!\n", __func__);
        omxvdec_release_mem(&pchan->decoder_vdh_buf);
        return HI_FAILURE;
    }

    pchan->dfs_alloc_flag = DFS_ALREADY_ALLOC;

    return HI_SUCCESS;
}

static HI_S32 task_report_channel_mem(OMXVDEC_CHAN_CTX *pchan)
{
    HI_S32 ret;

    /* Dynamic select vcodec freq */
    if (HI_FAILURE == channel_dynamic_freq_sel(pchan))
    {
        OmxPrint(OMX_ERR, "%s select vcodec freq failed\n", __func__);
    }

    pchan->dfs_alloc_flag = DFS_WAIT_CLEAR;

    ret = processor_ctrl_inst(pchan, PP_CMD_THREAD_WAKE_UP, HI_NULL);
    if (ret != HI_SUCCESS)
    {
        OmxPrint(OMX_ERR, "%s call processor_ctrl_inst failed\n", __func__);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static HI_S32 task_info_channel_mem(OMXVDEC_CHAN_CTX *pchan)
{
    HI_S32 ret;

    pchan->dfs_alloc_flag = DFS_WAIT_BIND;

    ret = channel_bind_mem_to_inst(pchan);
    if (ret != HI_SUCCESS)
    {
        OmxPrint(OMX_ERR, "%s bind mem to inst failed!\n", __func__);
        message_queue(pchan->msg_queue, VDEC_ERR_DYNAMIC_ALLOC_FAIL, HI_FAILURE, NULL);/*lint !e570 */
        return ret;
    }

    pchan->dfs_alloc_flag = DFS_WAIT_ACTIVATE;

    ret = channel_activate_inst(pchan);
    if (ret != HI_SUCCESS)
    {
        OmxPrint(OMX_ERR, "%s activate inst failed!\n", __func__);
        message_queue(pchan->msg_queue, VDEC_ERR_DYNAMIC_ALLOC_FAIL, HI_FAILURE, NULL);/*lint !e570 */
        return ret;
    }

    pchan->dfs_alloc_flag = DFS_ALREADY_ALLOC;

    return HI_SUCCESS;
}

static HI_S32 task_memory_manager(OMXVDEC_CHAN_CTX *pchan)
{
    HI_S32 ret;
    HI_U32 new_buf_num = 0;
    SEQ_INFO_S *pSeqInfo = &pchan->seq_info;

    if (PATH_MODE_NORMAL == pchan->path_mode)
    {
        OmxPrint(OMX_INFO, "Normal path need alloc: dec_width %d, dec_height %d, disp_width %d, disp_height %d, stride %d, frame_num %d, frame_size %d, pmv_size %d\n",
                 pSeqInfo->dec_width, pSeqInfo->dec_height, pSeqInfo->disp_width, pSeqInfo->disp_height, pSeqInfo->stride, pSeqInfo->ref_frame_num, pSeqInfo->ref_frame_size, pSeqInfo->ref_pmv_size);

        ret = task_alloc_channel_mem(pchan);
    }
    else
    {
            new_buf_num = pSeqInfo->ref_frame_num + g_ExtraDispNum + g_ClientReservedNum;

        if (pchan->out_width      != pSeqInfo->dec_width
         || pchan->out_height     != pSeqInfo->dec_height
         || pchan->out_stride     != pSeqInfo->stride
         || pchan->output_buf_num  < new_buf_num
         || pchan->output_buf_size < pSeqInfo->ref_frame_size)
        {
            OmxPrint(OMX_INFO, "REPORT_DEC_SIZE_CHG: width(%d->%d), height(%d->%d), stride=(%d->%d), frame_num(%d->%d), frame_size(%d->%d)\n",
                     pchan->out_width, pSeqInfo->dec_width, pchan->out_height, pSeqInfo->dec_height, pchan->out_stride, pSeqInfo->stride, pchan->output_buf_num, new_buf_num, pchan->output_buf_size, pSeqInfo->ref_frame_size);

            ret = task_report_channel_mem(pchan);
        }
        else
        {
            OmxPrint(OMX_INFO, "No need to REPORT_DEC_SIZE_CHG: width(%d), height(%d), stride=(%d), frame_num(%d, %d), frame_size(%d, %d)\n",
                     pchan->out_width, pchan->out_height, pchan->out_stride, pchan->output_buf_num, new_buf_num, pchan->output_buf_size, pSeqInfo->ref_frame_size);

            ret = task_info_channel_mem(pchan);
        }
    }

    if (ret != HI_SUCCESS)
    {
        OmxPrint(OMX_FATAL, "Mem manager for chan %d failed\n", pchan->channel_id); /*lint!e 774*/
        message_queue(pchan->msg_queue, VDEC_ERR_DYNAMIC_ALLOC_FAIL, HI_FAILURE, NULL); /*lint !e570 */
    }

    return ret;
}


HI_S32 task_func_entry(HI_VOID* param)
{
    HI_S32            ret     = HI_SUCCESS;
    OMXVDEC_ENTRY    *omxvdec = (OMXVDEC_ENTRY*)param;
    OMXVDEC_TASK     *task    = &(omxvdec->task);

    task->pserve_chan = HI_NULL;

    while(!kthread_should_stop())
    {
        task->task_state = TASK_STATE_SLEEPING;
        ret = wait_event_timeout(task->task_wait, (TASK_STATE_ONCALL == task->task_state), (msecs_to_jiffies(500))); /*lint !e666*/
        task->task_state = TASK_STATE_RUNNING;

        // if is not wake up by time out
        if (ret > 0)
        {
            do {
                task->pserve_chan = channel_find_inst_need_wake_up(omxvdec);
                if (HI_NULL == task->pserve_chan)
                {
                    break;
                }

                ret = task_memory_manager(task->pserve_chan);
                if (ret != HI_SUCCESS)
                {
                    task->pserve_chan = HI_NULL;
                    break;
                }
            }while(1);
        }
    }

    task->task_state = TASK_STATE_INVALID;

    return ret;
}


/*============== EXPORT FUNCTION =============*/
HI_S32 task_create_thread(OMXVDEC_ENTRY* omxvdec)
{
    OMXVDEC_TASK *task = HI_NULL;

    if (HI_NULL == omxvdec)
    {
        OmxPrint(OMX_FATAL, "%s ppTask = NULL!\n", __func__);
        return HI_FAILURE;
    }
    task = &(omxvdec->task);

    init_waitqueue_head(&task->task_wait);
    task->task_thread = kthread_create(task_func_entry, (HI_VOID *)omxvdec, "OmxVdecTask");

    if (HI_NULL == task->task_thread)
    {
        OmxPrint(OMX_FATAL, "%s create task failed!\n", __func__);
        return HI_FAILURE;
    }
    else if (IS_ERR(task->task_thread))
    {
        OmxPrint(OMX_FATAL, "%s create task failed, error no = %ld!\n", __func__, PTR_ERR(task->task_thread));
        task->task_thread = HI_NULL;
        return HI_FAILURE;
    }
    wake_up_process(task->task_thread);

    return HI_SUCCESS;
}

HI_S32 task_cancel_inst(OMXVDEC_CHAN_CTX *pchan)
{
    OMXVDEC_TASK *task;
	HI_S32 ret         = HI_FAILURE;
    HI_S32 wait_count  = 100;

    task = &(pchan->vdec->task);

    do
    {
        if (task->pserve_chan != pchan)
        {
            ret = HI_SUCCESS;
            break;
        }
        else
        {
            msleep(5);
            wait_count--;
        }
    }while(wait_count > 0);

    return ret;
}

HI_S32 task_destroy_thread(OMXVDEC_ENTRY* omxvdec)
{
    OMXVDEC_TASK *task = HI_NULL;

    if (HI_NULL == omxvdec)
    {
        OmxPrint(OMX_FATAL, "%s omxvdec = NULL!\n", __func__);
        return HI_FAILURE;
    }
    task = &(omxvdec->task);

    if(task->task_thread != HI_NULL)
    {
        task->task_state = TASK_STATE_ONCALL;
        wake_up(&task->task_wait);
        kthread_stop(task->task_thread);
        task->task_thread = HI_NULL;
    }

    return HI_SUCCESS;
}

HI_VOID task_proc_entry(struct seq_file *p, OMXVDEC_TASK *ptask)
{
    PROC_PRINT(p, "%-20s :%s\n", "TaskState",         task_show_state(ptask->task_state));
}


