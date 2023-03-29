

#ifndef __MEM_TRACE_H__
#define __MEM_TRACE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if defined(_PRE_MEM_TRACE)
#include "oal_types.h"

/**
 @brief 此内存跟踪模块的初始化接口?
 @param[in] none
 @return    oal_void
 @note  在需要跟踪的内存节点开始进行动态申请之前被执行
*/
extern oal_void mem_trace_init(oal_void);

/**
 @brief 此内存跟踪模块的去初始化接口
 @param[in] none
 @return    oal_void
 @note  由于当前的实现中模块本身的内存资源都是静态申请的
        所以此接口暂无任何作用，后续如果此跟踪模块本身使用的内存资源
        需要动态申请，可以利用此接口来释放申请的内存资源
*/
extern oal_void mem_trace_exit(void);

/**
 @brief 显示内存跟踪模块中跟踪到的节点信息
 @param[in] ul_mode     模式0:根据输入的入口信息，显示该入口登记的所有内存节点信息
                        模式1:显示所有入口登记的内存节点总数
                        模式2:显示当前所有处于登记状态的节点总数

 @param[in] ul_fileid   申请该内存的入口信息:文件号
 @param[in] ul_linenum  申请该内存的入口信息:行号
 @return    oal_void
 @note  当前的实现是采用printk的方式将上述信息呈现给用户。
*/
extern oal_void mem_trace_info_show(oal_uint32 ul_mode,
                                    oal_uint32 ul_fileid,
                                    oal_uint32 ul_line);
#ifdef _PRE_SKB_TRACE
/**
 @brief 登记需要被跟踪的内存节点，包括内存节点的地址、申请该内存的入口信息(文件号+行号)
 @param[in] ul_addr     内存节点对应的地址
 @param[in] ul_fileid   申请该内存的入口信息:文件号
 @param[in] ul_linenum  申请该内存的入口信息:行号
 @return    oal_void
 @note  具体在解决某一特定问题时，需要修改相应的内存申请接口或者内存跟踪的入口
        比如需要跟踪某一个模块动态申请的skb，那么需要统一对该模块使用的skb动态申请接口进行统一的宏改造:

     _oal_netbuf_alloc()是原本的实现接口，只是修改了一下接口名称

     #ifdef _PRE_SKB_TRACE
     #define oal_netbuf_alloc(size,l_reserve,l_align)        \
     ({\
         oal_netbuf_stru *pst_netbuf;\
         pst_netbuf = _oal_netbuf_alloc(size,l_reserve,l_align);\
         if(pst_netbuf)\
         {\
             mem_trace_add_node((oal_size_t)pst_netbuf, THIS_FILE_ID, (oal_uint32)__LINE__);\
         }\
         pst_netbuf;\
     })
     #else
     #define oal_netbuf_alloc(size,l_reserve,l_align) _oal_netbuf_alloc(size,l_reserve,l_align)
     #endif
*/
extern oal_void __mem_trace_add_node(oal_ulong   ul_addr,
                                     oal_uint32  ul_fileid,
                                     oal_uint32  ul_linenum);

/**
 @brief 注销被跟踪的内存节点
 @param[in] ul_addr     内存节点对应的地址
 @return    oal_void
 @note  某模块认为该内存节点不需要继续跟踪的时候需要调用此接口注销掉该内存节点。
        通常是内存被释放时或者内存节点传递给下一个模块了。

        具体在解决某一特定问题时，需要修改相应的内存释放接口或者内存传递给下一个模块的出口
        比如需要跟踪某一个模块释放的skb，那么需要统一对该模块使用的skb释放接口进行统一的宏改造:

     _oal_netbuf_free()是原本的实现接口，只是修改了一下接口名称

     #ifdef _PRE_SKB_TRACE
     #define oal_netbuf_free(pst_netbuf)        \
     ({\
         mem_trace_delete_node((oal_size_t)pst_netbuf);\
         _oal_netbuf_free(pst_netbuf);\
     })
     #else
     #define oal_netbuf_free(pst_netbuf) _oal_netbuf_free(pst_netbuf)
     #endif
*/
extern oal_void __mem_trace_delete_node(oal_ulong   ul_addr,
                                        oal_uint32  ul_fileid,
                                        oal_uint32  ul_linenum);


/**
 @brief 被跟踪的内存节点的探针，可以在程序运行时跟踪该内存节点最后探测到的位置
 @param[in] ul_addr     内存节点对应的地址
 @param[in] ul_fileid   申请该内存的入口信息:文件号
 @param[in] ul_linenum  申请该内存的入口信息:行号
 @return    oal_void
 @note  通常在解决某一个实际问题时，在程序流程中不同的地方调用此接口用于探测某内存节点是否运行到该位置，
        从而可以确定程序流程，便于问题的定位

        比如在解决内存泄露时，通过mem_trace_info_show的打印，可以得知哪些内存节点没有被释放掉，也可以知道
        哪些入口申请的内存节点没有释放掉，那么可以在该入口开始的流程中增加探针，从而可以跟踪内存在运行时
        最后到达的位置
*/
extern oal_void __mem_trace_probe(oal_ulong  ul_addr,
                                  oal_uint32 ul_probe_fileid,
                                  oal_uint32 ul_probe_line);



#define mem_trace_add_node(ul_addr) \
        __mem_trace_add_node(ul_addr, THIS_FILE_ID, __LINE__)
#define mem_trace_delete_node(ul_addr) \
        __mem_trace_delete_node(ul_addr, THIS_FILE_ID, __LINE__)
#define mem_trace_probe(ul_addr) \
        __mem_trace_probe(ul_addr, THIS_FILE_ID, __LINE__)
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
#endif

