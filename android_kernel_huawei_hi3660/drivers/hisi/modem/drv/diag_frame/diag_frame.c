#include <linux/slab.h>
#include "bsp_slice.h"
#include "bsp_diag_frame.h"


//#define HISI_HEADER_MAGIC               (0x48495349) /*HISI*/
void bsp_diag_fill_socp_head(diag_socp_head_stru * socp_packet, u32 len)
{
    socp_packet->u32HisiMagic   = DIAG_SOCP_HEAD_MAGIC;
    socp_packet->u32DataLen     = len;

    return;
}


void bsp_diag_frame_head_init(diag_frame_head_stru* diag_frame)
{
    u64   auctime = 0;
    diag_frame->stService.sid8b       = DIAG_FRAME_MSP_SID_DIAG_SERVICE;
    diag_frame->stService.mdmid3b     = 0;
    diag_frame->stService.rsv1b       = 0;
    diag_frame->stService.ssid4b      = DIAG_FRAME_SSID_APP_CPU;
    diag_frame->stService.sessionid8b = DIAG_FRAME_MSP_SERVICE_SESSION_ID;
    diag_frame->stService.mt2b        = DIAG_FRAME_MT_IND;
    diag_frame->stService.index4b     = 0;
    diag_frame->stService.eof1b       = 0;
    diag_frame->stService.ff1b        = 0;
    diag_frame->stService.MsgTransId  = 0;

    bsp_slice_getcurtime(&auctime);
    /* coverity[secure_coding] */
    memcpy(diag_frame->stService.aucTimeStamp,&auctime, sizeof(diag_frame->stService.aucTimeStamp));

    diag_frame->stID.cmdid19b         = 0;
    diag_frame->stID.sec5b            = 0;
    diag_frame->stID.mode4b           = 0;
    diag_frame->stID.pri4b            = 0;
    diag_frame->u32MsgLen             = 0;
    return;
}
