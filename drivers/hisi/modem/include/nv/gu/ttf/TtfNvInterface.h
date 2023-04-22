

#ifndef __TTFNVINTERFACE_H__
#define __TTFNVINTERFACE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 Include Headfile
*****************************************************************************/
#include "vos.h"

/*****************************************************************************
  2 Macro
*****************************************************************************/

#define TTF_MEM_MAX_POOL_NUM                (5)
#define TTF_MEM_MAX_CLUSTER_NUM             (8)

#define FC_UL_RATE_MAX_LEV                  (11)
#define TTF_MEM_POOL_NV_NUM                 (TTF_MEM_MAX_POOL_NUM + 1)
#define FC_ACPU_DRV_ASSEM_NV_LEV            (4)

#define BASTET_HPRTODCH_SUPPORT             (0x01)
#define BASTET_CHNL_LPM_SUPPORT             (0x02)
#define BASTET_ASPEN_SUPPORT                (0x04)

#define NV_PLATFORM_MAX_RAT_NUM             (7)               /* 接入技术最大值 */

#define NV_PLATFORM_MAX_MODEM_NUM           (8)

/*****************************************************************************
  3 Massage Declare
*****************************************************************************/


/*****************************************************************************
  4 Enum
*****************************************************************************/

enum FC_MEM_THRESHOLD_LEV_ENUM
{
    FC_MEM_THRESHOLD_LEV_1              = 0,
    FC_MEM_THRESHOLD_LEV_2,
    FC_MEM_THRESHOLD_LEV_3,
    FC_MEM_THRESHOLD_LEV_4,
    FC_MEM_THRESHOLD_LEV_5,
    FC_MEM_THRESHOLD_LEV_6,
    FC_MEM_THRESHOLD_LEV_7,
    FC_MEM_THRESHOLD_LEV_8,
    FC_MEM_THRESHOLD_LEV_BUTT           = 8
};
typedef VOS_UINT32  FC_MEM_THRESHOLD_LEV_ENUM_UINT32;

enum FC_ACPU_DRV_ASSEM_LEV_ENUM
{
    FC_ACPU_DRV_ASSEM_LEV_1             = 0,
    FC_ACPU_DRV_ASSEM_LEV_2,
    FC_ACPU_DRV_ASSEM_LEV_3,
    FC_ACPU_DRV_ASSEM_LEV_4,
    FC_ACPU_DRV_ASSEM_LEV_5             = 4,
    FC_ACPU_DRV_ASSEM_LEV_BUTT          = 5
};
typedef VOS_UINT32  FC_ACPU_DRV_ASSEM_LEV_ENUM_UINT32;

enum RATIO_RESET_TYPE_ENUM
{
    TTF_NODE_RESET_TYPE                 = 0,
    PS_QNODE_RESET_TYPE                 = 1,
    RATIO_RESET_TYPE_BUTT               = 2
};
typedef VOS_UINT32  RATIO_RESET_TYPE_ENUM_UINT32;

/*****************************************************************************
 枚举名    : TTF_BOOL_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : TTF统一布尔类型枚举定义
*****************************************************************************/
enum TTF_BOOL_ENUM
{
    TTF_FALSE                            = 0,
    TTF_TRUE                             = 1,

    TTF_BOOL_BUTT
};
typedef VOS_UINT8   TTF_BOOL_ENUM_UINT8;

/*****************************************************************************
  5 STRUCT
*****************************************************************************/
/*****************************************************************************
*                                                                            *
*                           参数设置消息结构                                 *
*                                                                            *
******************************************************************************/

/*****************************************************************************
 结构名    : TTF_MEM_POOL_CFG_NV_STRU
 DESCRIPTION: 数据面内存池划分
*****************************************************************************/
typedef struct
{
    VOS_UINT8                      ucClusterCnt;                                /* 内存池档位个数，Range:[0,8] */
    VOS_UINT8                      aucReserved[1];
    VOS_UINT16                     ausBlkSize[TTF_MEM_MAX_CLUSTER_NUM];         /* 本级数的大小(byte) */
    VOS_UINT16                     ausBlkCnt[TTF_MEM_MAX_CLUSTER_NUM];          /* 本级个数 */
}TTF_MEM_POOL_CFG_NV_STRU;

/*****************************************************************************
 结构名    : TTF_MEM_SOLUTION_CFG_NV_STRU
 DESCRIPTION: 数据面内存池划分
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucPoolCnt;                              /* 内存池个数，Range:[0,6] */
    VOS_UINT8                           ucPoolMask;                             /* 对应内存池是否生效掩码位，bit来标示，1- 生效， 0- 不生效*/
    TTF_MEM_POOL_CFG_NV_STRU            astTtfMemPoolCfgInfo[TTF_MEM_POOL_NV_NUM];  /* 各内存池配置 */
    VOS_UINT8                           aucReserve[2];
}TTF_MEM_SOLUTION_CFG_NV_STRU;

/*****************************************************************************
 结构名    : FC_CFG_CPU_STRU
 DESCRIPTION: FC_CFG_CPU结构,CPU流控的门限和配置值
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulCpuOverLoadVal;                       /*Range:[0,100]*//* CPU流控门限 */
    VOS_UINT32                          ulCpuUnderLoadVal;                      /*Range:[0,100]*//* CPU解除流控门限 */
    VOS_UINT32                          ulSmoothTimerLen;                       /*Range:[2,1000]*//* CPU流控平滑次数，单位:CPU监控周期 */
    VOS_UINT32                          ulStopAttemptTimerLen;                  /* CPU引发R接口流控后，数传中断时间较长，启动定时器，尝试提前解除，单位: 毫秒，0表示不使用 */
    VOS_UINT32                          ulUmUlRateThreshold;                    /* 空口上行速率门限，高于此门限，认为是数传引起的CPU负载高，需要流控 */
    VOS_UINT32                          ulUmDlRateThreshold;                    /* 空口下行速率门限，高于此门限，认为是数传引起的CPU负载高，需要流控 */
    VOS_UINT32                          ulRmRateThreshold;                      /* E5形态下， WIFI/USB入口处速率门限， 高于此门限，认为是数传引起的CPU负载高，需要流控，单位bps */
} FC_CFG_CPU_STRU;

/*****************************************************************************
 结构名    : FC_CFG_MEM_THRESHOLD_STRU
 DESCRIPTION: FC_CFG_MEM_THRESHOLD结构,MEM流控的门限和配置值
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulSetThreshold;                         /* 启动流控门限 单位字节 */
    VOS_UINT32                          ulStopThreshold;                        /* 停止流控门限 单位字节 */
} FC_CFG_MEM_THRESHOLD_STRU;

/*****************************************************************************
 结构名    : FC_CFG_MEM_THRESHOLD_CST_STRU
 DESCRIPTION: FC_CFG_MEM_THRESHOLD_CST结构,MEM流控的门限和配置值
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulSetThreshold;                         /*Range:[0,4096]*//* 启动流控门限 单位字节 */
    VOS_UINT32                          ulStopThreshold;                        /*Range:[0,4096]*//* 停止流控门限 单位字节 */
} FC_CFG_MEM_THRESHOLD_CST_STRU;


/*****************************************************************************
 结构名    : FC_CFG_UM_UL_RATE_STRU
 DESCRIPTION: FC_CFG_UM_UL_RATE结构,空口上行速率档位
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucRateCnt;                              /*Range:[0,11]*//* 上行速率档位个数，最多支持11个档位设置，但是优先级最高为FC_PRI_9，所以使用档位时只有前9档生效 */
    VOS_UINT8                           aucRsv[1];
    VOS_UINT16                          ausRate[FC_UL_RATE_MAX_LEV];            /* 上行速率限制，取值范围[0,65535]，单位bps */
} FC_CFG_UM_UL_RATE_STRU;

/*****************************************************************************
 结构名    : FC_CFG_NV_STRU
 DESCRIPTION: FC_CFG_NV对应的NV结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulFcEnbaleMask;                         /* 流控使能标识 */
    FC_CFG_CPU_STRU                     stFcCfgCpuA;                            /* A核CPU流控门限 */
    VOS_UINT32                          ulFcCfgMemThresholdCnt;                 /* Range:[0,8]*/
    FC_CFG_MEM_THRESHOLD_STRU           stFcCfgMem[FC_MEM_THRESHOLD_LEV_BUTT];  /* A核内存流控门限 */
    FC_CFG_MEM_THRESHOLD_CST_STRU       stFcCfgCst;                             /* CSD业务流控门限 */
    FC_CFG_MEM_THRESHOLD_STRU           stFcCfgGprsMemSize;                     /* G模内存总量流控门限 */
    FC_CFG_MEM_THRESHOLD_STRU           stFcCfgGprsMemCnt;                      /* G模内存块数流控门限 */
    FC_CFG_CPU_STRU                     stFcCfgCpuC;                            /* C核CPU流控门限 */
    FC_CFG_UM_UL_RATE_STRU              stFcCfgUmUlRateForCpu;                  /* C核CPU流控上行速率档位配置 */
    FC_CFG_UM_UL_RATE_STRU              stFcCfgUmUlRateForTmp;                  /* C核温度流控上行行速率档位配置 */
    FC_CFG_MEM_THRESHOLD_STRU           stFcCfgCdmaMemSize;                     /* X模内存总量流控门限 */
    FC_CFG_MEM_THRESHOLD_STRU           stFcCfgCdmaMemCnt;                      /* X模内存块数流控门限 */
} FC_CFG_NV_STRU;

/*****************************************************************************
 结构名    : CPULOAD_CFG_STRU
 DESCRIPTION: CPULOAD_CFG对应的NV结构,A核CPU占用率监测任务定时检测时长
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulMonitorTimerLen;
} CPULOAD_CFG_STRU;

/*****************************************************************************
 结构名    : FC_CPU_DRV_ASSEM_PARA_STRU
 DESCRIPTION: 根据CPU LOAD动态高速驱动组包参数
*****************************************************************************/
typedef struct
{
    VOS_UINT8                          ucHostOutTimeout;    /* PC驱动组包时延 */
    VOS_UINT8                          ucEthTxMinNum;       /* UE驱动下行组包个数 */
    VOS_UINT8                          ucEthTxTimeout;      /* UE驱动下行组包时延 */
    VOS_UINT8                          ucEthRxMinNum;       /* UE驱动上行组包个数 */
    VOS_UINT8                          ucEthRxTimeout;      /* UE驱动上行组包时延 */
    VOS_UINT8                          ucCdsGuDlThres;      /* 已废弃 */
    VOS_UINT8                          aucRsv[2];
}FC_DRV_ASSEM_PARA_STRU;

/*****************************************************************************
 结构名    : FC_CPU_DRV_ASSEM_PARA_STRU
 DESCRIPTION: 根据CPU LOAD动态高速驱动组包参数
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulCpuLoad;                              /* CPU负载,Range:[0,100] */
    FC_DRV_ASSEM_PARA_STRU              stDrvAssemPara;
}FC_CPU_DRV_ASSEM_PARA_STRU;

/*****************************************************************************
 结构名    : FC_CPU_DRV_ASSEM_PARA_NV_STRU
 DESCRIPTION: FC_CPU_DRV_ASSEM对应的NV结构，根据CPU LOAD动态高速驱动组包参数
*****************************************************************************/
typedef struct
{
    VOS_UINT8                              ucEnableMask;                        /* 使能控制，0x0:不使能，0x1:使能 */
    VOS_UINT8                              ucSmoothCntUpLev;                    /* 向上调整平滑系数 */
    VOS_UINT8                              ucSmoothCntDownLev;                  /* 向下调整平滑系数 */
    VOS_UINT8                              ucRsv;
    FC_CPU_DRV_ASSEM_PARA_STRU             stCpuDrvAssemPara[FC_ACPU_DRV_ASSEM_NV_LEV]; /* CPU配置参数 */
}FC_CPU_DRV_ASSEM_PARA_NV_STRU;

/*****************************************************************************
 结构名    : WTTF_MACDL_WATERMARK_LEVEL_STRU
 DESCRIPTION: WTTF_MACDL_BBMST_WATER_LEVEL对应的NV结构,BBP译码中断水线等级结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulWaterLevelOne;                        /* 水线界别1 */
    VOS_UINT32                          ulWaterLevelTwo;                        /* 水线界别2 */
    VOS_UINT32                          ulWaterLevelThree;                      /* 水线界别3 */
    VOS_UINT32                          ulWaterLevelFour;                       /* 水线界别4,预留 */
} WTTF_MACDL_WATERMARK_LEVEL_STRU;

/*****************************************************************************
 结构名    : WTTF_MACDL_BBPMST_TB_HEAD_STRU
 DESCRIPTION: WTTF_MACDL_BBMST_TB_HEAD对应的NV结构,BBP译码中断水线等级结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulTBHeadNum;                /* TB头部块个数 */
    VOS_UINT32                          ulTBHeadReserved;           /* TB头部块预留的TB块数，预留3帧，V9R1 30块，V3R3 15块  */
} WTTF_MACDL_BBPMST_TB_HEAD_STRU;

/*****************************************************************************
 结构名    : WTTF_SRB_NOT_SEND_THRESHOLD_STRU
 DESCRIPTION: WTTF_SRB_NOT_SEND_THRESHOLD_STRU对应的NV结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulWttfSrbNotSendThreshold;                /* SRB不发送测量报告的缓存buffer大小门限 */
} WTTF_SRB_NOT_SEND_THRESHOLD_STRU;


/*****************************************************************************
 结构名    : NF_EXT_NV_STRU
 DESCRIPTION: NETFILTER_HOOK_MASK对应的NV结构,设置勾包点的NV项，预留5种掩码组合:
             ulNvValue1~ulNvValue5, 每个掩码取值范围为0-FFFFFFFF,
             其中掩码位为1则代表该掩码位对应的钩子函数可能会被注册到内核中
*****************************************************************************/
typedef struct
{
    VOS_UINT32          ulNetfilterPara1;                                       /* 钩子函数掩码参数1 */
    VOS_UINT32          ulNetfilterPara2;                                       /* 钩子函数掩码参数2 */
    VOS_UINT32          ulNetfilterPara3;                                       /* 钩子函数掩码参数3 */
    VOS_UINT32          ulNetfilterPara4;                                       /* 钩子函数掩码参数4 */
    VOS_UINT32          ulNetfilterPara5;                                       /* 钩子函数掩码参数5 */
}NF_EXT_NV_STRU;

/*****************************************************************************
 结构名    : EXT_TEBS_FLAG_NV_STRU
 DESCRIPTION: NV_Ext_Tebs_Flag对应的NV结构,BO扩展功能，根据当前的BO计算一个放大的BO用于提高SG值
*****************************************************************************/
typedef struct
{
    VOS_UINT32          ulExtTebsFlag;                                          /* 使能开关.0x0:不使能,0x1:使能 */
}EXT_TEBS_FLAG_NV_STRU;

/*****************************************************************************
 结构名    : TCP_ACK_DELETE_FLG_NV_STRU
 DESCRIPTION: NV_TCP_ACK_Delete_Flg对应的NV结构，删除SDU队列中缓存的旧的TCP ACK的IP包功能
*****************************************************************************/
typedef struct
{
    VOS_UINT32          ulTcpAckDeleteFlg;                                      /* 使能开关.0x0:不使能,0x1:使能 */
}TCP_ACK_DELETE_FLG_NV_STRU;

/*****************************************************************************
 结构名    : HUAWEI_IRAN_OPEN_PAGE_I_NV_STRU
 DESCRIPTION: HUAWEI_IRAN_OPEN_PAGE_I_NV结构,XID定制开关
*****************************************************************************/
typedef struct
{
    VOS_UINT16          usHuaweiIranOpenPageI;                                  /* Range:[0,1]
                                                                                   0: UE按照最大协商值向网络发起XID协商
                                                                                   1: 在LLC ADM模式下不发起XID协商，在LLC ABM模式下按照协议规定默认协商值发起XID协商 */
    VOS_UINT8           aucReserve[2];
}HUAWEI_IRAN_OPEN_PAGE_I_NV_STRU;

/*****************************************************************************
 结构名    : GCF_TYPE_CONTENT_NV_STRU
 DESCRIPTION: GCFTypeContent对应的NV结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32          ulGcfTypeContent;                                       /* 使能开关.0x0:不使能,0x1:使能 */
}GCF_TYPE_CONTENT_NV_STRU;

/*****************************************************************************
 结构名    : W_RF8960_BER_TEST_NV_STRU
 DESCRIPTION: W_RF8960_BER_Test对应的NV结构,为Aglient8960仪器没有做重建导致无法进行FR BER测试操作而打桩
*****************************************************************************/
typedef struct
{
    VOS_UINT32          ulRlc8960RFBerTestFlag;                                 /* 使能开关.0x0:不使能,0x1:使能 */
}W_RF8960_BER_TEST_NV_STRU;

/*****************************************************************************
 结构名    : LAPDM_RAND_BIT_NV_STRU
 DESCRIPTION: LAPDM_RAND_BIT对应的NV结构,lapdm随机BIT功能开关
*****************************************************************************/
typedef struct
{
    VOS_UINT16          usLapdmRandBit;                                         /* 使能开关.0x0:不使能,0x1:使能 */
    VOS_UINT8           aucReserve[2];
}LAPDM_RAND_BIT_NV_STRU;

/*****************************************************************************
 结构名    : CBS_W_DRX_SWITCH_NV_STRU
 DESCRIPTION: CBS_W_DRX_Switch对应的NV结构,配置W模CBS是否启动DRX功能
*****************************************************************************/
typedef struct
{
    VOS_UINT32          ulCbsWDrxSwitch;                                        /* 使能开关.0x0:不使能,0x1:使能 */
}CBS_W_DRX_SWITCH_NV_STRU;

/*****************************************************************************
 结构名    : CBS_W_WAIT_NEW_CBS_MSG_TIMER_NV_STRU
 DESCRIPTION: CBS_W_WaitNewCBSMsgTimer对应的NV结构，W模CBS功能简化版本定时器时长
*****************************************************************************/
typedef struct
{
    VOS_UINT32          ulCbsWWaitNewCbsMsgTimer;                               /* W模CBS功能简化版本定时器时长，单位为ms */
}CBS_W_WAIT_NEW_CBS_MSG_TIMER_NV_STRU;

/*****************************************************************************
 结构名    : CBS_W_WAIT_SHED_MSG_TIMER_NV_STRU
 DESCRIPTION: CBS_W_WaitShedMsgTimer对应的NV结构，不使用
*****************************************************************************/
typedef struct
{
    VOS_UINT32          ulCbsWWaitShedMsgTimer;
}CBS_W_WAIT_SHED_MSG_TIMER_NV_STRU;

/*****************************************************************************
 结构名    : FC_QOS_STRU
 DESCRIPTION: FC_QOS_STRU 打桩使用结构,目前代码中已经不使用，为了保持NV结构不变化
*****************************************************************************/

typedef struct
{
    VOS_UINT32          ulULKBitRate;
    VOS_UINT32          ulDLKBitRate;
}FC_QOS_STRU;

/*****************************************************************************
 结构名    : CPU_FLOW_CTRL_CONFIG_NV_STRU
 DESCRIPTION: CPU_FLOW_CTRL_CONFIG_STRU对应的NV结构，已废弃，打桩提供
*****************************************************************************/
typedef struct
{
    VOS_UINT32          ulFuncMask;
    VOS_UINT32          ulCpuOverLoadVal;
    VOS_UINT32          ulCpuUnderLoadVal;
    FC_QOS_STRU         astQos1[2];
    FC_QOS_STRU         astQos2[2];
    FC_QOS_STRU         stUmtsEhsUlLimitForDlHighRate;
    FC_QOS_STRU         stUmtsHslULimitForDlHighRate;
    FC_QOS_STRU         stUlLimitForDlLowRate;
    VOS_UINT32          ulRItfDlkBitRate;
    VOS_UINT32          ulRItfRate;
    VOS_UINT32          ulWaitQosTimeLen;
    VOS_UINT32          ulSmoothTimerLen;
    VOS_UINT32          ulRItfSetTimerLen;
    VOS_UINT32          ulCpuFlowCtrlEnable;
    VOS_UINT32          aulRsv[2];
}CPU_FLOW_CTRL_CONFIG_NV_STRU;

/*****************************************************************************
 结构名    : R_ITF_FLOW_CTRL_CONFIG_STRU
 DESCRIPTION: R_ITF_FLOW_CTRL_CONFIG_STRU对应的NV结构,R接口流控使能。
*****************************************************************************/
typedef struct
{
    VOS_UINT32              ulRateDismatchUsbEnable;                            /* USB的R接口流控功能使能开关.0x0:不使能,0x1:使能 */
    VOS_UINT32              ulRateDismatchWifiEnable;                           /* WIFI的R接口流控功能使能开关.0x0:不使能,0x1:使能 */
} R_ITF_FLOW_CTRL_CONFIG_STRU;

/*****************************************************************************
 结构名    : TFC_POWER_FUN_ENABLE_NV_STRU
 DESCRIPTION: TFC_POWER_FUN_ENABLE对应的NV结构,使能TFC功率估计功能
*****************************************************************************/
typedef struct
{
    VOS_UINT32          ulTfcPowerFunEnable;                                    /* TFC功率估计功能使能开关.0x0:不使能,0x1:使能 */
}TFC_POWER_FUN_ENABLE_NV_STRU;

/*****************************************************************************
结构名    : PPP_CONFIG_MRU_TYPE_NV_STRU
DESCRIPTION: PPP_CONFIG_MRU_Type对应的NV结构,默认协商MTU参数，用于PPP LCP协商
*****************************************************************************/
typedef struct
{
    VOS_UINT16                           usPppConfigType;   /* 默认MRU大小,Range:[296,1500]*/
    VOS_UINT8                            aucReserve[2];
}PPP_CONFIG_MRU_TYPE_NV_STRU;

/*****************************************************************************
 结构名    : FC_CDS_DL_CONFIG_STRU
 协议表格  :
 ASN.1描述 :
 DESCRIPTION: 定义CDS下行丢包流控配置结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32          ulDiscardThres;         /* CDS下行队列丢包门限 */
    VOS_UINT32          ulDiscardRate;          /* 丢包率 */
} FC_CDS_DL_CONFIG_STRU;

/*****************************************************************************
 结构名    : QOS_FC_CONFIG_STRU
 协议表格  :
 ASN.1描述 :
 DESCRIPTION: 定义QOS流控配置结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32          ulPktCntLimit;              /* 触发QoS流控包数 */
    VOS_UINT32          ulTimerLen;                 /* 触发QoS流控时长 */
    VOS_UINT32          ulRandomDiscardRate;        /* 随机丢包率 */
    VOS_UINT32          ulDiscardRate;              /* 丢包率 */
    VOS_UINT32          ulWarningThres;             /* 警告阈值，除必须保留的承载外全部置为丢包状态 */
    VOS_UINT32          ulDiscardThres;             /* 丢包阈值，从低优先级承载开始置承载为丢包状态 */
    VOS_UINT32          ulRandomDiscardThres;       /* 随机丢包阈值，从低优先级承载开始置承载为随机丢包状态 */
    VOS_UINT32          ulRestoreThres;             /* 恢复阈值，从高优先级承载开始逐渐恢复承载数传 */
} QOS_FC_CONFIG_STRU;

/*****************************************************************************
 结构名    : FLOWCTRL_CDS_CONFIG_STRU
 协议表格  :
 ASN.1描述 :
 DESCRIPTION: 保存C核CDS模块流控功能的配置信息
*****************************************************************************/
typedef struct
{
    VOS_UINT32              ulFcEnableMask;         /* 流控开关 */
                                                    /* bit0 QoS流控是否使能 */
                                                    /* bit1 最高优先级承载是否流控 */
                                                    /* bit2 CDS下行流控是否使能 */
                                                    /* bit3 最高优先级承载是否不丢包 */
    QOS_FC_CONFIG_STRU      stQosFcConfig;          /* QOS流控配置结构 */
    FC_CDS_DL_CONFIG_STRU   stFcCdsDlConfig;        /* CDS下行丢包流控配置结构 */
}FLOWCTRL_CDS_CONFIG_STRU;

/*****************************************************************************
结构名    : BMC_CBS_MSG_READ_NV_STRU
DESCRIPTION: BMC_CBS_MSG_READ_NV_STRU对应的NV结构.控制是否读取消息类型为Reading Advised和Reading optional CBS消息
*****************************************************************************/
typedef struct
{
    VOS_UINT8               ucDisableReadAdvised;       /*Range:[0,1]*//*当调度消息中消息描述类型为advised时，对应DRX周期内消息是否接受； PS_FALSE为不接受，PS_TRUE为接受*/
    VOS_UINT8               ucDisableReadOptional;      /*Range:[0,1]*//*当调度消息中消息描述类型为optional时，对应DRX周期内消息是否接受；PS_FALSE为不接受，PS_TRUE为接受*/
    VOS_UINT8               ucDisableRepetitionMsg;     /*Range:[0,1]*//*当调度消息中消息描述类型为Repetition msg时，对应DRX周期内消息是否接受；PS_FALSE为不接受，PS_TRUE为接受*/
    VOS_UINT8               ucDisableOldMsg;            /*Range:[0,1]*//*当调度消息中消息描述类型为old msg时，对应DRX周期内消息是否接受；PS_FALSE为不接受，PS_TRUE为接受*/
}BMC_CBS_MSG_READ_NV_STRU;


/*****************************************************************************
结构名    : NV_MODEM_RF_SHARE_CFG_STRU
DESCRIPTION: NV_MODEM_RF_SHARE_CFG对应的NV结构
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usSupportFlag;      /* Range:[0,2]，值从小到大分别表示不支持DSDS,支持DSDS1.0,支持DSDS2.0 */
    VOS_UINT16                          usGSMRFID;          /* GSM接入模式RFID信息 */
    VOS_UINT16                          usWCDMARFID;        /* WCDMA接入模式RFID信息 */
    VOS_UINT16                          usTDSRFID;          /* TD-SCDMA接入模式RFID信息 */
    VOS_UINT16                          usLTERFID;          /* LTE接入模式RFID信息 */
    VOS_UINT16                          usCDMARFID;         /* CMDA接入模式RFID信息 */
    VOS_UINT16                          usEVDORFID;         /* CDMA EVDO接入模式RFID信息 */
    VOS_UINT16                          usReserved;
} NV_MODEM_RF_SHARE_CFG_STRU;


/*****************************************************************************
结构名    : NV_MODEM_RF_SHARE_CFG_EX_STRU
DESCRIPTION: NV_MODEM_RF_SHARE_CFG_EX_STRU对应的NV结构,DSDS功能开启以及各接入使用的RFID
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulProfileTypeId;     /* 读取en_NV_Item_TRI_MODE_FEM_PROFILE_ID获取ulProfileId,
                                                               对应于取下面组数中哪一套配置 */
    NV_MODEM_RF_SHARE_CFG_STRU          astNvModemRfShareCfg[8];                /* Modem对应的RF资源配置信息 */
}NV_MODEM_RF_SHARE_CFG_EX_STRU;


enum NV_PLATFORM_RAT_TYPE_ENUM
{
    NV_PLATFORM_RAT_GSM,                                                       /*GSM接入技术 */
    NV_PLATFORM_RAT_WCDMA,                                                     /* WCDMA接入技术 */
    NV_PLATFORM_RAT_LTE,                                                       /* LTE接入技术 */
    NV_PLATFORM_RAT_TDS,                                                       /* TDS接入技术 */
    NV_PLATFORM_RAT_1X,                                                        /* CDMA-1X接入技术 */
    NV_PLATFORM_RAT_EVDO,                                                      /* CDMA-EV_DO接入技术 */

    NV_PLATFORM_RAT_BUTT
};
typedef VOS_UINT16 NV_PLATFORM_RAT_TYPE_ENUM_UINT16;


typedef struct
{
    VOS_UINT16                           usRatNum;                          /* 支持的接入技术的数目 */
    NV_PLATFORM_RAT_TYPE_ENUM_UINT16     aenRatList[NV_PLATFORM_MAX_RAT_NUM];  /* 接入技术列表 */
}NV_PLATAFORM_RAT_CAPABILITY_STRU;


typedef struct
{
    VOS_UINT8                           ucActiveFlg;        /* 是否激活功能 */
    VOS_UINT8                           ucHookFlg;          /* Bastet钩包模式 */
    VOS_UINT8                           aucSubFun[2];
}BASTET_SUPPORT_FLG_STRU;


enum NV_CTTF_BOOL_ENUM
{
    NV_CTTF_BOOL_FALSE,                /* 条件为真 */
    NV_CTTF_BOOL_TRUE,               /* 条件为假 */
    NV_CTTF_BOOL_BUTT
};
typedef VOS_UINT8 NV_CTTF_BOOL_ENUM_UINT8;


typedef struct
{
    NV_CTTF_BOOL_ENUM_UINT8             enSupportFlg;                   /* 是否支持通过NV项配置cProbeInitialAdjust。NV_CTTF_BOOL_FALSE:不支持；NV_CTTF_BOOL_TRUE支持，默认不支持 */
    VOS_INT8                            cProbeInitialAdjust;            /* cProbeInitialAdjust的值 */
    VOS_UINT8                           aucSubFun[2];
}NV_CTTF_PROBE_INIT_POWER_CTRL_STRU;



typedef struct
{
    VOS_UINT16                          usMru;                  /* PPP帧最大接收单元长度 */
    VOS_UINT8                           ucReserved;
    VOS_UINT8                           ucCaveEnable;           /* PPP接入鉴权CAVE算法开关，0不支持，1支持 */
    VOS_UINT32                          ulPppInactTimerLen;     /* MAX PPP Inactive Timer时长，单位s */
}TTF_PPPC_NVIM_CONFIG_OPTIONS_STRU;


typedef struct
{
    VOS_UINT32                          ulHrpdRfAllocSwitchMask;    /* 每个bit为1表示使能，0表示不使能, bit 0:signaling alloc rf, bit 1:ppp in access auth alloc rf */
}NV_HRPD_RF_ALLOC_SWITCH_MASK_STRU;



typedef struct
{
    NV_CTTF_BOOL_ENUM_UINT8             enResetEnable;                  /* TTF_Node主动复位时能 */
    VOS_UINT8                           ucFailPercent;             /* 申请失败比例门限，达到时主动复位 */
    VOS_UINT16                          usTotalStat;               /* 节点申请统计总次数 */
}NV_RATIO_RESET_CTRL_STRU;


typedef struct
{
    NV_RATIO_RESET_CTRL_STRU                  astNvResetCtrl[RATIO_RESET_TYPE_BUTT];
}NV_NODE_RESET_CTRL_STRU;

/*****************************************************************************
 结构名    : TTF_CICOM_IP_ENT_NVIM_STRU
 DESCRIPTION: TTF_CICOM_IP_ENT_NVIM_STRU对应的NV结构,CIOCM个数
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucCicomIPNumber;         /* CIOCM个数 */
    VOS_UINT8                           aucRsv[3];

    VOS_UINT8                           aucModemIdToCicomIndex[NV_PLATFORM_MAX_MODEM_NUM];   /* 数组下标为Modem id,数值代表对应使用的CIOCM Index，0~ucCicomIPNumber-1 */
}TTF_CICOM_IP_ENT_NVIM_STRU;

/*****************************************************************************
 结构名    : NV_TTF_PPP_CONFIG_STRU
 DESCRIPTION: en_NV_Item_PPP_CONFIG对应的NV结构
*****************************************************************************/
typedef struct
{
    TTF_BOOL_ENUM_UINT8                 enChapEnable;           /* 是否使能Chap鉴权 */
    TTF_BOOL_ENUM_UINT8                 enPapEnable;            /* 是否使能Pap鉴权 */
    VOS_UINT16                          usLcpEchoMaxLostCnt;    /* 发送LcpEchoRequest允许丢弃的最大个数 */

    VOS_UINT16                          usQueneMaxCnt;          /* 队列最大允许个数 */
    VOS_UINT8                           aucRsv[2];
}NV_TTF_PPP_CONFIG_STRU;

/*****************************************************************************
 结构名    : NV_TTF_SEQ_OUT_OF_ORDER_COMPATIBLE_STRU
 DESCRIPTION: en_NV_Item_TTF_SEQ_OUT_OF_ORDER_COMPATIBLE对应的NV结构
*****************************************************************************/
typedef struct
{
    TTF_BOOL_ENUM_UINT8                 enWcdmaEnable;                  /*Range:[0,1]*//* 是否使能WCDMA兼容 */
    TTF_BOOL_ENUM_UINT8                 enLAPDmEnable;                  /*Range:[0,1]*//* 是否使能LAPDm兼容 */
    VOS_UINT8                           ucWcdmaRlcAckOutOfSeqScope;     /*Range:[0,24]*//* W模RLC接收ACK的LSN允许的乱序范围 */
    VOS_UINT8                           ucWcdmaRlcErrStatusPduCnt;      /*Range:[0,4]*//* W模RLC连续接收错误PDU的个数 */
}NV_TTF_SEQ_OUT_OF_ORDER_COMPATIBLE_STRU;

/*****************************************************************************
 结构名    : NV_WTTF_CSPS_RLC_PS_NOT_RPT_DATALINK_LOSS_STRU
 DESCRIPTION: en_NV_Item_WTTF_CSPS_RLC_PS_NOT_RPT_DATALINK_LOSS对应的NV结构，是否使能CS+PS时，PS RB不上报不可恢复错
*****************************************************************************/
typedef struct
{
    TTF_BOOL_ENUM_UINT8                 enEnable;                       /*Range:[0,1]*//* 是否使能CS+PS时，PS RB不上报不可恢复错 */
    VOS_UINT8                           ucRsv1;
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
}NV_WTTF_CSPS_RLC_PS_NOT_RPT_DATALINK_LOSS_STRU;


typedef struct
{
    NV_CTTF_BOOL_ENUM_UINT8             enTpeEnable;             /* TPE使能 */
    VOS_UINT8                           ucCorrectPercent;        /* 修正因子:若超时，再等百分比的时间 */
    VOS_UINT8                           aucReserved[2];
}NV_TTF_TPE_CTRL_STRU;

/*****************************************************************************
 结构名    : NV_TTF_CORE_BIND_CONFIG_STRU
 DESCRIPTION: NV_TTF_CORE_BIND_CONFIG_STRU对应的NV结构
             绑core0 设置第0个bit为1，绑core1 设置第1个bit为1
             绑core2 设置第2个bit为1，绑core3 设置第3个bit为1
             多核一起绑定时，mask需要做或操作
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucLlcFidCoreMask;          /* Llc绑定在哪个核上 */
    VOS_UINT8                           ucGrmFidCoreMask;          /* Grm绑定在哪个核上 */
    VOS_UINT8                           ucMacRlcUlFidCoreMask;     /* MacRlcUl绑定在哪个核上 */
    VOS_UINT8                           ucMacRlcDlFidCoreMask;     /* MacRlcDl绑定在哪个核上 */
}NV_TTF_CORE_BIND_CONFIG_STRU;

/*****************************************************************************
 结构名    : NV_CTTF_FID_CORE_BIND_CONFIG_STRU
 DESCRIPTION: NV_CTTF_FID_CORE_BIND_CONFIG_STRU对应的NV结构, CoreMask的每一个bit位对应一个核
             bit0对应Core0，bit1对应Core1
             bit2对应Core2，bit3对应Core3
             CoreMask等于0为非法，超出范围也非法
             可以同时绑定到多个核上
             举例:
             uc1XFwdFidCoreMask设置为1，表示MSPS_FID_CTTF_1X_FWD绑定在核0
             uc1XFwdFidCoreMask设置为2，表示MSPS_FID_CTTF_1X_FWD绑定在核1
             uc1XFwdFidCoreMask设置为4，表示MSPS_FID_CTTF_1X_FWD绑定在核2
             uc1XFwdFidCoreMask设置为8，表示MSPS_FID_CTTF_1X_FWD绑定在核3
             uc1XFwdFidCoreMask设置为3，表示MSPS_FID_CTTF_1X_FWD绑定在核0和核1
             uc1XFwdFidCoreMask设置为11，表示MSPS_FID_CTTF_1X_FWD绑定在核0和核1和核3
             uc1XFwdFidCoreMask设置为15，表示MSPS_FID_CTTF_1X_FWD绑定在核0和核1和核2和核3
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           uc1XFwdFidCoreMask;       /* MSPS_FID_CTTF_1X_FWD绑核参数 */
    VOS_UINT8                           uc1XRevFidCoreMask;       /* MSPS_FID_CTTF_1X_REV绑核参数 */
    VOS_UINT8                           ucHrpdFwdFidCoreMask;     /* CTTF_FID_HRPD_FWD绑核参数 */
    VOS_UINT8                           ucHrpdRevFidCoreMask;     /* CTTF_FID_HRPD_REV绑核参数 */
}NV_CTTF_FID_CORE_BIND_CONFIG_STRU;



enum NV_ACTIVE_MODEM_MODE_ENUM
{
    NV_ACTIVE_SINGLE_MODEM              = 0x00,
    NV_ACTIVE_MULTI_MODEM               = 0x01,
    NV_ACTIVE_MODEM_MODE_BUTT
};
typedef VOS_UINT8 NV_ACTIVE_MODEM_MODE_ENUM_UINT8;


typedef struct
{
    NV_ACTIVE_MODEM_MODE_ENUM_UINT8     enActiveModem;
    VOS_UINT8                           aucReserve[3];
}NV_DSDS_ACTIVE_MODEM_MODE_STRU;


typedef struct
{
    TTF_BOOL_ENUM_UINT8         enEnable;               /* 功能是否打开,取值0和1 */
    VOS_UINT8                   ucFreq;                 /* 流控频率，单位:10ms */
    VOS_UINT16                  usFlowCtrlThreshold;    /* 数据包老化时间，单位:10ms */ 
}NV_TTF_RLC_FLOW_CONTROL_STRU;

/*****************************************************************************
  6 UNION
*****************************************************************************/


/*****************************************************************************
  7 Extern Global Variable
*****************************************************************************/


/*****************************************************************************
  8 Fuction Extern
*****************************************************************************/


/*****************************************************************************
  9 OTHERS
*****************************************************************************/
















#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of TtfNvInterface.h */
