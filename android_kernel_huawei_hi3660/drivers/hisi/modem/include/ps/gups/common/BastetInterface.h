
#ifndef __BASTETINTERFACE_H__
#define __BASTETINTERFACE_H__

/*****************************************************************************
 * 1 其他头文件包含                                                          *
*****************************************************************************/
#include "vos.h"

#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif
#endif

#pragma pack(4)

/*****************************************************************************
 * 2 宏定义                                                                  *
*****************************************************************************/
/* Bastet模块丢包消息类型 */
#define BST_ASPEN_INFO_PKT_DROP             0xc0000001
/* 上报给A核丢包数组大小 */
#define BST_ASPEN_PKT_DROP_SIZE             (128u)

/* 丢包位置类型信息 */
#define BASTET_ASPEN_PKT_DROP_TYPE_CDS      0x10
#define BASTET_ASPEN_PKT_DROP_TYPE_PDCP     0x20
#define BASTET_ASPEN_PKT_DROP_TYPE_WRLC     0x40

#define BASTET_ASPEN_PKT_DROP_TYPE_MASK     (0xF0)

/*****************************************************************************
 * 3 Massage Declare                                                         *
*****************************************************************************/

/*****************************************************************************
 * 4 枚举定义                                                                *
*****************************************************************************/
enum BST_ACORE_CORE_MSG_TYPE_ENUM
{
    BST_ACORE_CORE_MSG_TYPE_DSPP,
    BST_ACORE_CORE_MSG_TYPE_ASPEN,
    BST_ACORE_CORE_MSG_TYPE_BUTT
};
typedef VOS_UINT32 BST_ACORE_CORE_MSG_TYPE_ENUM_UINT32;

/*****************************************************************************
 * 5 类/结构定义                                                             *
*****************************************************************************/
typedef struct
{
    BST_ACORE_CORE_MSG_TYPE_ENUM_UINT32 enMsgType;
    const VOS_UINT8                    *pucData;
} BST_ACOM_DATA_STRU;

typedef struct
{
    VOS_MSG_HEADER
    BST_ACORE_CORE_MSG_TYPE_ENUM_UINT32 enMsgType;
    VOS_UINT32                          ulLen;
    VOS_UINT8                           aucValue[4];
} BST_ACOM_MSG_STRU;

typedef struct
{
    VOS_UINT32                          ulTcpSeq;
    VOS_UINT16                          usSrcPort;
    VOS_UINT16                          usDropType;
} BST_ASPEN_TCP_INFO_STRU;

typedef struct
{
    VOS_UINT32                          ulAspenInfoType;
    VOS_UINT32                          ulPktNum;                           /* 丢包个数 */
    BST_ASPEN_TCP_INFO_STRU             stPkt[BST_ASPEN_PKT_DROP_SIZE];     /* 丢包数组(一次最大传输128个丢包信息) */
} BST_ASPEN_PKT_DROP_STRU;

/*****************************************************************************
 * 6 UNION定义                                                               *
*****************************************************************************/

/*****************************************************************************
 * 7 全局变量声明                                                            *
*****************************************************************************/

/*****************************************************************************
 * 8 函数声明                                                             *
*****************************************************************************/

/*****************************************************************************
 * 9 OTHERS定义                                                              *
*****************************************************************************/

#pragma pack()

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __BASTETINTERFACE_H__ */
