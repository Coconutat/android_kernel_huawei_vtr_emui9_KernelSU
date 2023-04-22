
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_MEM_TRACE

#include "oal_list.h"
#include "oal_schedule.h"
#include "mem_trace.h"


//note: 如下两个宏定义值需要根据实际需要跟踪的模块来修改，并遵守基本的约束
/**
 @brief 此宏定义表示某模块在运行时需要跟踪的动态申请的内存节点的最大值
        为了模块内的逻辑运算，此值必须定义为2的N次方且(N>=5)
*/
#define MEM_TRACE_TBL_SIZE              8192    //需要根据常驻内存的上水位调整

/**
 @brief 此宏定义表示某模块在运行时需要跟踪的动态申请的内存节点的入口的最大值
*/
#define MEM_TRACE_OWNER_TBL_SIZE        64
/*
本模块消耗的内存资源 = 44*MEM_TRACE_TBL_SIZE
                       + 20
                       + 20*MEM_TRACE_OWNER_TBL_SIZE
                       + 8*(MEM_TRACE_TBL_SIZE>>5)
                       + sizeof(oal_spin_lock) = 178K
*/


//每个被跟踪的内存块的信息
typedef struct
{
    oal_ulong               ul_addr;        //内存块地址
    oal_uint32              ul_time;        //内存申请时间戳
    oal_uint32              ul_fileid;      //申请该内存块的文件ID
    oal_uint32              ul_linenum;     //申请该内存块的代码行号
    oal_uint32              ul_last_trace_fileid;  //该内存块最近一次的跟踪点
    oal_uint32              ul_last_trace_line;    //该内存块最近一次的跟踪点
    oal_uint32              ul_last_trace_time;    //该内存块最近一次的跟踪时间戳
    oal_dlist_head_stru     st_list_entry;
    oal_dlist_head_stru     st_hash_list_entry;
    oal_dlist_head_stru     st_owner_list_entry;
} mem_trace_node;

//内存块的入口信息
typedef struct _mem_node_owner
{
    oal_uint32              ul_fileid;      //申请该内存块的文件ID
    oal_uint32              ul_linenum;     //申请该内存块的代码行号
    oal_dlist_head_stru     st_owner_list_head;
    oal_uint32              ul_cnt;
} mem_node_owner;


#define MEM_TRACE_HASH_TBL_SIZE         (MEM_TRACE_TBL_SIZE >> 5)
#define MEM_TRACE_HASH_TBL_SIZE_MASK    (MEM_TRACE_HASH_TBL_SIZE - 1)


//内存跟踪模块的管理
typedef struct
{
    mem_trace_node          ast_mem_trace_tbl[MEM_TRACE_TBL_SIZE];
    oal_dlist_head_stru     st_free_list_head;
    oal_dlist_head_stru     st_used_list_head;
    oal_dlist_head_stru     ast_hast_table[MEM_TRACE_HASH_TBL_SIZE];      //加速搜索节点的速度
    mem_node_owner          ast_owner[MEM_TRACE_OWNER_TBL_SIZE];
    oal_spin_lock_stru      st_spin_lock;
    oal_uint32              ul_cnt;
    oal_uint32              ul_node_shortage; // 发生过node不够用的情形
} mem_trace_mgmt;


OAL_STATIC mem_trace_mgmt     g_st_mem_trace_mgmt;


OAL_STATIC OAL_INLINE mem_trace_mgmt *mem_trace_get_mgr(oal_void)
{
    return &g_st_mem_trace_mgmt;
}


oal_void mem_trace_init(oal_void)
{
    oal_uint32      i;
    mem_trace_mgmt *pst_mgr = mem_trace_get_mgr();

    OAL_MEMZERO((oal_void*)pst_mgr, OAL_SIZEOF(mem_trace_mgmt));
    oal_spin_lock_init(&pst_mgr->st_spin_lock);
    oal_dlist_init_head(&pst_mgr->st_free_list_head);
    oal_dlist_init_head(&pst_mgr->st_used_list_head);
    pst_mgr->ul_cnt = 0;

    for (i = 0; i < MEM_TRACE_HASH_TBL_SIZE; i++)
    {
        oal_dlist_init_head(&pst_mgr->ast_hast_table[i]);
    }

    //初始化时将所有节点都加入free链表
    for (i = 0; i < MEM_TRACE_TBL_SIZE; i++)
    {
        oal_dlist_add_tail(&pst_mgr->ast_mem_trace_tbl[i].st_list_entry, &pst_mgr->st_free_list_head);
    }

    for (i = 0; i < MEM_TRACE_OWNER_TBL_SIZE; i++)
    {
        pst_mgr->ast_owner[i].ul_cnt = 0;
        pst_mgr->ast_owner[i].ul_fileid = ~0;
        pst_mgr->ast_owner[i].ul_linenum = 0;
        oal_dlist_init_head(&pst_mgr->ast_owner[i].st_owner_list_head);
    }

    OAL_IO_PRINT("mem_trace_init done!\r\n");
}


oal_void mem_trace_exit(oal_void)
{
    //nothing todo
}


OAL_STATIC mem_node_owner *mem_trace_owner_search(oal_uint32  ul_fileid, oal_uint32  ul_linenum)
{
    mem_trace_mgmt *pst_mgr   = mem_trace_get_mgr();
    mem_node_owner *pst_owner;

    for (pst_owner = pst_mgr->ast_owner + 0; pst_owner < pst_mgr->ast_owner + MEM_TRACE_OWNER_TBL_SIZE; pst_owner++)
    {
        // 找到空闲owner、或者既有owner
        if (((pst_owner->ul_fileid ==  0xffffffff) && (pst_owner->ul_linenum == 0))
        ||  ((pst_owner->ul_fileid == ul_fileid) && (pst_owner->ul_linenum == ul_linenum)))
        {
            return pst_owner;
        }
    }

    return OAL_PTR_NULL;
}


oal_void __mem_trace_add_node(oal_ulong   ul_addr,
                              oal_uint32  ul_fileid,
                              oal_uint32  ul_linenum)
{
    mem_trace_node*      pst_mem_trace_node;
    oal_dlist_head_stru* pst_entry;
    oal_uint             ul_irq_save;
    mem_node_owner*      pst_owner;
    mem_trace_mgmt      *pst_mgr   = mem_trace_get_mgr();

    oal_spin_lock_irq_save(&pst_mgr->st_spin_lock, &ul_irq_save);
    if (oal_dlist_is_empty(&pst_mgr->st_free_list_head))
    {
        OAL_IO_PRINT("mem trace mod not initialized or the trace space not enough!!!\n");
        pst_mgr->ul_node_shortage = OAL_TRUE;
        oal_spin_unlock_irq_restore(&pst_mgr->st_spin_lock, &ul_irq_save);
        return;
    }
    // 从free链表头部取；向free链表尾部回收，以确保free链表中节点历史。
    pst_entry = oal_dlist_delete_head(&(pst_mgr->st_free_list_head));
    oal_dlist_add_tail(pst_entry, &(pst_mgr->st_used_list_head));

    pst_mem_trace_node = OAL_DLIST_GET_ENTRY(pst_entry, mem_trace_node, st_list_entry);
    pst_mem_trace_node->ul_addr = ul_addr;
    pst_mem_trace_node->ul_time = (oal_uint32)OAL_TIME_JIFFY;
    pst_mem_trace_node->ul_last_trace_time = pst_mem_trace_node->ul_time;
    pst_mem_trace_node->ul_fileid = ul_fileid;
    pst_mem_trace_node->ul_linenum = ul_linenum;
    pst_mem_trace_node->ul_last_trace_fileid = ul_fileid;
    pst_mem_trace_node->ul_last_trace_line = ul_linenum;

    oal_dlist_add_tail(&(pst_mem_trace_node->st_hash_list_entry), &(pst_mgr->ast_hast_table[ul_addr & MEM_TRACE_HASH_TBL_SIZE_MASK]));
    pst_mgr->ul_cnt++;
    pst_owner = mem_trace_owner_search(ul_fileid, ul_linenum);

    if (pst_owner)
    {
        pst_owner->ul_fileid = ul_fileid;
        pst_owner->ul_linenum = ul_linenum;
        pst_owner->ul_cnt++;
        oal_dlist_add_tail(&(pst_mem_trace_node->st_owner_list_entry), &pst_owner->st_owner_list_head);
    }
    else
    {
        OAL_WARN_ON(1);
    }

    oal_spin_unlock_irq_restore(&pst_mgr->st_spin_lock, &ul_irq_save);
}


oal_void __mem_trace_delete_node(oal_ulong   ul_addr,
                                 oal_uint32  ul_fileid,
                                 oal_uint32  ul_linenum)
{
    oal_dlist_head_stru* pst_entry;
    oal_dlist_head_stru* pst_entry_tmp;
    mem_trace_node*      pst_mem_trace_node;
    oal_bool_enum_uint8  en_match_flag = OAL_FALSE;
    oal_uint             ul_irq_save;
    mem_node_owner*      pst_owner;
    mem_trace_mgmt      *pst_mgr = mem_trace_get_mgr();

    oal_spin_lock_irq_save(&pst_mgr->st_spin_lock, &ul_irq_save);

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_entry_tmp, &(pst_mgr->ast_hast_table[ul_addr & MEM_TRACE_HASH_TBL_SIZE_MASK]))
    {
        pst_mem_trace_node = OAL_DLIST_GET_ENTRY(pst_entry, mem_trace_node, st_hash_list_entry);

        if (pst_mem_trace_node->ul_addr == ul_addr)
        {
            oal_dlist_delete_entry(pst_entry);      //从hash链表删除
            pst_mem_trace_node->ul_last_trace_time = (oal_uint32)OAL_TIME_JIFFY;
            pst_mem_trace_node->ul_last_trace_fileid = ul_fileid;
            pst_mem_trace_node->ul_last_trace_line   = ul_linenum;
            en_match_flag = OAL_TRUE;
            break;
        }
    }

    if (en_match_flag)
    {
        pst_mgr->ul_cnt--;
        oal_dlist_delete_entry(&(pst_mem_trace_node->st_list_entry));  //从used 链表删除
        oal_dlist_add_tail(&(pst_mem_trace_node->st_list_entry), &pst_mgr->st_free_list_head);  //增加到free链表
        pst_owner = mem_trace_owner_search(pst_mem_trace_node->ul_fileid, pst_mem_trace_node->ul_linenum);

        if (pst_owner)
        {
            oal_dlist_delete_entry(&(pst_mem_trace_node->st_owner_list_entry));  //从owner链表删除
            pst_owner->ul_cnt--;
        }
        else
        {
             OAL_IO_PRINT("%s error:pst_owner is null\n", __FUNCTION__);
        }
    }
    else if (!pst_mgr->ul_node_shortage)
    {
        //释放一个没有登记的内存
        oal_bool_enum_uint8  en_ever_del = OAL_FALSE;
        OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &pst_mgr->st_free_list_head)
        {
            pst_mem_trace_node = OAL_DLIST_GET_ENTRY(pst_entry, mem_trace_node, st_list_entry);
            if (pst_mem_trace_node->ul_addr == ul_addr)
            {
                OAL_IO_PRINT("0x%lx add@[%d:%d:%u] del@[%d:%d:%u]\n",
                        ul_addr,
                        pst_mem_trace_node->ul_fileid, pst_mem_trace_node->ul_linenum,
                        pst_mem_trace_node->ul_time,
                        pst_mem_trace_node->ul_last_trace_fileid,
                        pst_mem_trace_node->ul_last_trace_line,
                        pst_mem_trace_node->ul_last_trace_time);
                en_ever_del = OAL_TRUE;
            }
        }

        if (en_ever_del)
        {
            OAL_IO_PRINT("maybe double-free 0x%lx@[%d:%d] time=%u, see last above\n---cut---\n", ul_addr, ul_fileid, ul_linenum, (oal_uint32)OAL_TIME_JIFFY);
        }
        else
        {
            OAL_IO_PRINT("delete node not registered, addr=0x%lx)\n", ul_addr);
            OAL_WARN_ON(1);
        }
    }

    oal_spin_unlock_irq_restore(&pst_mgr->st_spin_lock, &ul_irq_save);
    //OAL_IO_PRINT("mem_trace_delete_node::addr=0x%x\r\n",ul_addr);
}


oal_void __mem_trace_probe(oal_ulong  ul_addr,
                           oal_uint32 ul_probe_fileid,
                           oal_uint32 ul_probe_line)
{
    oal_dlist_head_stru* pst_entry;
    mem_trace_node*      pst_mem_trace_node;
    oal_bool_enum_uint8  en_match_flag = OAL_FALSE;
    oal_uint             ul_irq_save;
    mem_trace_mgmt      *pst_mgr = mem_trace_get_mgr();

    oal_spin_lock_irq_save(&pst_mgr->st_spin_lock, &ul_irq_save);

    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_mgr->ast_hast_table[ul_addr & MEM_TRACE_HASH_TBL_SIZE_MASK]))
    {
        pst_mem_trace_node = OAL_DLIST_GET_ENTRY(pst_entry, mem_trace_node, st_hash_list_entry);

        if (pst_mem_trace_node->ul_addr == ul_addr)
        {
            pst_mem_trace_node->ul_last_trace_fileid = ul_probe_fileid;
            pst_mem_trace_node->ul_last_trace_line = ul_probe_line;
            pst_mem_trace_node->ul_last_trace_time = (oal_uint32)OAL_TIME_JIFFY;
            en_match_flag = OAL_TRUE;
            break;
        }
    }

    oal_spin_unlock_irq_restore(&pst_mgr->st_spin_lock, &ul_irq_save);

    if (!en_match_flag)
    {
        if(!pst_mgr->ul_node_shortage)
        {
            //找不到该内存块
            OAL_IO_PRINT("the node(0x%p) maybe not register!\n",(void*)ul_addr);
            mem_trace_info_show(2,0,0);
            mem_trace_info_show(1,0,0);
            OAL_WARN_ON(1);
        }
    }

}



oal_void mem_trace_info_show(oal_uint32 ul_mode, oal_uint32  ul_fileid, oal_uint32 ul_line)
{
    oal_dlist_head_stru* pst_entry;
    mem_trace_node*      pst_mem_trace_node;
    oal_uint32           i;
    oal_uint             ul_irq_save;
    mem_node_owner*      pst_owner;
    mem_trace_mgmt      *pst_mgr = mem_trace_get_mgr();

    oal_spin_lock_irq_save(&pst_mgr->st_spin_lock, &ul_irq_save);

    OAL_IO_PRINT("==============show mem trace info(mode=%d) shortage=%d:==================\r\n", ul_mode, pst_mgr->ul_node_shortage);

    if (0 == ul_mode)
    {
        OAL_IO_PRINT("    Addr    | File_id | Line |    Time    | trace_file | trace_line | Last_time\r\n");
        pst_owner = mem_trace_owner_search(ul_fileid, ul_line);

        if (pst_owner)
        {
            if (pst_owner->ul_fileid != ul_fileid)
            {
                OAL_IO_PRINT("no mem trace owner find!\r\n");
            }
            else
            {
                OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_owner->st_owner_list_head))
                {
                    pst_mem_trace_node = OAL_DLIST_GET_ENTRY(pst_entry, mem_trace_node, st_owner_list_entry);
                    OAL_IO_PRINT("0x%p |%8u | %4u | %10u | %10u | %10u | %10u\r\n",
                                 (oal_void*)pst_mem_trace_node->ul_addr,
                                 pst_mem_trace_node->ul_fileid,
                                 pst_mem_trace_node->ul_linenum,
                                 pst_mem_trace_node->ul_time,
                                 pst_mem_trace_node->ul_last_trace_fileid,
                                 pst_mem_trace_node->ul_last_trace_line,
                                 pst_mem_trace_node->ul_last_trace_time);
                }
            }
        }
        else
        {
            OAL_IO_PRINT("%s error:pst_owner is null\n", __FUNCTION__);
        }      
    }
    else if (1 == ul_mode)
    {
        OAL_IO_PRINT(" File_id | Line | Cnt \r\n");

        for (i = 0; i < MEM_TRACE_OWNER_TBL_SIZE; i++)
        {
            if ((pst_mgr->ast_owner[i].ul_fileid == 0xffffffff)
                && (pst_mgr->ast_owner[i].ul_linenum == 0))
            {
                break;
            }
            else
            {
                OAL_IO_PRINT("%8u | %4u | %4u\r\n",
                             pst_mgr->ast_owner[i].ul_fileid,
                             pst_mgr->ast_owner[i].ul_linenum,
                             pst_mgr->ast_owner[i].ul_cnt);
            }
        }
    }
    else if (2 == ul_mode)
    {
        OAL_IO_PRINT(" current mem trace cnt:%d\r\n", pst_mgr->ul_cnt);
    }
    oal_spin_unlock_irq_restore(&pst_mgr->st_spin_lock, &ul_irq_save);

    OAL_IO_PRINT("========================End=====================\r\n");
}

oal_module_symbol(__mem_trace_add_node);
oal_module_symbol(__mem_trace_delete_node);
oal_module_symbol(__mem_trace_probe);
oal_module_symbol(mem_trace_info_show);

#endif/* #ifdef _PRE_MEM_TRACE */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

