#include "bsp_trace.h"
#include "bsp_slice.h"
#include "bsp_hds_ind.h"
#include "bsp_hds_service.h"
#include <osl_types.h>


extern s32 bsp_transreport(TRANS_IND_STRU *pstData);

void bsp_trace_test0(u32 data1, u32 data2,u32 data3,u32 data4,u32 data5)
{
   u64 data = 4294967300;
   bsp_trace(BSP_LOG_LEVEL_NOTICE, BSP_MODU_LOG,"%d,%lld,%d,%d,%d,%d\n",data1,data,data2,data3,data4,data5);
}

void bsp_trace_test1(u32 data1, u32 data, u32 data2,u32 data3,u32 data4,u32 data5)
{
   bsp_trace(BSP_LOG_LEVEL_FATAL, BSP_MODU_LOG,"%d,%d,%d,%d,%d,%d\n",data1,data,data2,data3,data4,data5);
}

void bsp_trace_test2(u32 data1, u32 data2)
{
   u64 data = 4294967300;
   bsp_trace(BSP_LOG_LEVEL_FATAL, BSP_MODU_LOG,"data1:%d, data:%lld, data2:%d\n",data1,(u64)data,data2);
}

/*fail*/
void bsp_trace_test3(u64 data)
{
    u64 data1;
    data1 = data;
    bsp_trace(BSP_LOG_LEVEL_FATAL, BSP_MODU_LOG,"data:%lld\n", (u64)data1);
}

void bsp_print_report_test0(void)
{
    bsp_trace(BSP_LOG_LEVEL_FATAL, BSP_MODU_LOG,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa!\n");
}
void bsp_print_report_test1(void)
{
    bsp_trace(BSP_LOG_LEVEL_WARNING, BSP_MODU_LOG,"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n");
}
void bsp_print_report_test2(void)
{
    bsp_trace(BSP_LOG_LEVEL_DEBUG, BSP_MODU_DLOCK,"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc\n");
}

/*252个z,4个a,4个b,4个c:zz..zzzaaaa(256)|bbbbcccc*/
void bsp_print_report_test3(void)
{
    bsp_trace(BSP_LOG_LEVEL_FATAL, BSP_MODU_LOG,"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzaaaabbbbcccc\n");
}

typedef struct
{
    u32 temperater;
    u32 time;
}trans_report_stru;

int bsp_trans_report_test0(void)
{
    trans_report_stru trans = {0,0};
    TRANS_IND_STRU pstdata = {0};
    u64 beftime = 0,afttime = 0;
    int ret;

    trans.temperater = 9;
    trans.time = 10;

    // ( 31-24:modemid,23-16:modeid )
    pstdata.ulModule = 0x01000000;
    pstdata.ulMsgId = 0x00001234;
    pstdata.ulPid = 0x8003;
    pstdata.ulLength= sizeof(trans_report_stru);
    pstdata.pData = (void*)(&trans);

    bsp_slice_getcurtime(&beftime);
    ret = bsp_transreport(&pstdata);
    bsp_slice_getcurtime(&afttime);
    printk(KERN_ERR"beftime=%lld,afttime=%lld!\n",beftime,afttime);
    if (HDS_TRANS_RE_SUCC != ret)
    {
        printk("trans data report test fail!\n");
        return ret;
    }

    return ret;
}

typedef struct
{
    u32 temperater;
    char data[1000];
}trans_report_stru_info;

int bsp_trans_report_test1(void)
{
    trans_report_stru_info trans = {0};
    TRANS_IND_STRU pstdata = {0};
    u64 beftime = 0,afttime = 0;
    int ret;

    trans.temperater = 25;
    memcpy(trans.data,"I am happy!!!", (unsigned long)13);

    pstdata.ulModule = 0x01010000;      /*31-24:modemid(0x1),23-16:modeid(0x1)*/
    pstdata.ulMsgId = 0x00005701;       /*18-0:消息id(0x5701)*/
    pstdata.ulPid = 0x8003;             /*0x8003,代表BSP*/
    pstdata.ulLength= sizeof(trans_report_stru_info);
    pstdata.pData = (void*)(&trans);

    bsp_slice_getcurtime(&beftime);
    ret = bsp_transreport(&pstdata);
    bsp_slice_getcurtime(&afttime);
    printk(KERN_ERR"beftime=%lld,afttime=%lld!\n",beftime,afttime);

    if (HDS_TRANS_RE_SUCC != ret)
    {
        printk(KERN_ERR"trans data report test fail!\n");
        return ret;
    }
    return ret;
}

int bsp_trans_report_test2(void)
{
    void *pstdata = NULL;
    int ret;
    ret = bsp_transreport((TRANS_IND_STRU *)pstdata);
    return ret;
}






