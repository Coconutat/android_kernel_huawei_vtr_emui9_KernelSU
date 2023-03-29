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

#include <product_config.h>
#include <mdrv_public.h>
#include <mdrv_memory.h>
#include <mdrv_misc_comm.h>
#include <mdrv_int_comm.h>
#include <mdrv_errno.h>
#include <bsp_trace.h>
#include <bsp_dump.h>
#include <bsp_om_enum.h>
#include <bsp_shared_ddr.h>
#include <bsp_sram.h>

/*lint -save -e778 -e835*/

#define hwadp_printf(fmt, ...) printk(fmt , ##__VA_ARGS__)

extern BSP_DDR_SECT_INFO_S g_ddr_info[BSP_DDR_SECT_TYPE_BUTTOM];

#ifndef HI_BBP_CDMA_BASE_ADDR
#define HI_BBP_CDMA_BASE_ADDR 0
#endif

#ifndef HI_BBP_CDMA_ON_BASE_ADDR
#define HI_BBP_CDMA_ON_BASE_ADDR 0
#endif

#ifndef HI_BBP_GLB_ON_BASE_ADDR
#define HI_BBP_GLB_ON_BASE_ADDR 0
#endif

#ifndef HI_BBP_GSDR_BASE_ADDR
#define HI_BBP_GSDR_BASE_ADDR 0
#endif

int bsp_show_ip_addr(void)
{
    unsigned int i;

    for(i = 0; i < BSP_IP_TYPE_BUTTOM; i++)
    {
        hwadp_printf("ip_addr[%d] = 0x%pK\n", i, mdrv_misc_get_ip_baseaddr((BSP_IP_TYPE_E)i));
    }
	return 0;
}
EXPORT_SYMBOL(bsp_show_ip_addr);

int bsp_show_irq_num(void)
{
    unsigned int i;

    for(i = 0; i < BSP_INT_TYPE_BUTTOM; i++)
    {
        hwadp_printf("irq_num[%d] = 0x%X\n", i, mdrv_int_get_num((BSP_INT_TYPE_E)i));
    }
	return 0;
}
EXPORT_SYMBOL(bsp_show_irq_num);

/*****************************************************************************
 函 数 名  : BSP_DDR_ShowSectInfo
 功能描述  : 打印DDR内存段信息
 输入参数  : 无
 输出参数  : 无
 返回值    ：无
*****************************************************************************/
int BSP_DDR_ShowSectInfo(void)
{
    BSP_DDR_SECT_TYPE_E     enSectTypeIndex = (BSP_DDR_SECT_TYPE_E)0;

    hwadp_printf("\ntype       paddr      vaddr      size       attr\n");
    for(; enSectTypeIndex < BSP_DDR_SECT_TYPE_BUTTOM; enSectTypeIndex++)
    {
          hwadp_printf("0x%-8.8x 0x%8pK 0x%8pK 0x%-8.8x 0x%-8.8x\n", \
          g_ddr_info[enSectTypeIndex].enSectType, \
          g_ddr_info[enSectTypeIndex].pSectPhysAddr, \
          g_ddr_info[enSectTypeIndex].pSectVirtAddr, \
          g_ddr_info[enSectTypeIndex].ulSectSize, \
          g_ddr_info[enSectTypeIndex].enSectAttr);
    }
	return 0;
}
EXPORT_SYMBOL(BSP_DDR_ShowSectInfo);


struct show_mem_info{
    const char *name;
    unsigned long value;
    unsigned int  size;
};

struct show_mem_info global_ddr_infos[]={
    {"DDR_GU_ADDR         ", MDDR_FAMA(DDR_GU_ADDR         ), DDR_GU_SIZE         },
    {"DDR_TLPHY_IMAGE_ADDR", MDDR_FAMA(DDR_TLPHY_IMAGE_ADDR), DDR_TLPHY_IMAGE_SIZE},
    {"DDR_MCORE_ADDR      ", MDDR_FAMA(DDR_MCORE_ADDR      ), DDR_MCORE_SIZE      },
    {"DDR_MCORE_DTS_ADDR  ", MDDR_FAMA(DDR_MCORE_DTS_ADDR  ), DDR_MCORE_DTS_SIZE  },
    {"DDR_CBBE_IMAGE_ADDR ", MDDR_FAMA(DDR_CBBE_IMAGE_ADDR ), DDR_CBBE_IMAGE_SIZE },
    {"DDR_LPHY_SDR_ADDR   ", MDDR_FAMA(DDR_LPHY_SDR_ADDR   ), DDR_LPHY_SDR_SIZE   },
    {"DDR_LCS_ADDR        ", MDDR_FAMA(DDR_LCS_ADDR        ), DDR_LCS_SIZE        },
    {"DDR_SEC_SHARED_ADDR ", MDDR_FAMA(DDR_SEC_SHARED_ADDR ), DDR_SEC_SHARED_SIZE },
    {"DDR_ACORE_ADDR      ", MDDR_FAMA(DDR_ACORE_ADDR      ), DDR_ACORE_SIZE      },
    {"DDR_ACORE_DTS_ADDR  ", MDDR_FAMA(DDR_ACORE_DTS_ADDR  ), DDR_ACORE_DTS_SIZE  },
    {"DDR_SHARED_MEM_ADDR ", MDDR_FAMA(DDR_SHARED_MEM_ADDR ), DDR_SHARED_MEM_SIZE },
    {"DDR_MNTN_ADDR       ", MDDR_FAMA(DDR_MNTN_ADDR       ), DDR_MNTN_SIZE       },
    {"DDR_SOCP_ADDR       ", MDDR_FAMA(DDR_SOCP_ADDR       ), DDR_SOCP_SIZE       },
    {"DDR_HIFI_ADDR       ", MDDR_FAMA(DDR_HIFI_ADDR       ), DDR_HIFI_SIZE       },
};
/*****************************************************************************
 函 数 名  : show_global_ddr_status
 功能描述  : DDR内存段信息打印
 输入参数  : 无
 输出参数  : 无
 返回值    ：无
*****************************************************************************/
int show_global_ddr_status(void)
{
    int i,num;
    /* name phy_addr size */
    char* str_format ="%-30s %10lx %10x \n";

    hwadp_printf("%-30s %10s %10s (hex)\n", "name", "phy_addr", "size");
    num = sizeof(global_ddr_infos)/sizeof(global_ddr_infos[0]);
    for(i = 0; i < num; ++i){
        hwadp_printf(str_format,global_ddr_infos[i].name, global_ddr_infos[i].value, global_ddr_infos[i].size);
    }
    return 0;
}

struct show_mem_info sram_infos[]={

    {"SMALL_SECTIONS " , SRAM_OFFSET_SMALL_SECTIONS , SRAM_SIZE_SMALL_SECTIONS },
    {"MCU_RESERVE    " , SRAM_OFFSET_MCU_RESERVE    , SRAM_SIZE_MCU_RESERVE    },
    {"ICC            " , SRAM_OFFSET_ICC            , SRAM_SIZE_ICC            },
    {"TLDSP_SHARED   " , SRAM_OFFSET_TLDSP_SHARED   , SRAM_SIZE_TLDSP_SHARED   },
    {"GU_MAC_HEADER  " , SRAM_OFFSET_GU_MAC_HEADER  , SRAM_SIZE_GU_MAC_HEADER  },
    {"DYNAMIC_SEC    " , SRAM_OFFSET_DYNAMIC_SEC    , SRAM_SIZE_DYNAMIC_SEC    },
};

#define OFFSET_SRAM_USB_ASHELL               offsetof(SRAM_SMALL_SECTIONS, SRAM_USB_ASHELL             )
#define OFFSET_UART_INFORMATION              offsetof(SRAM_SMALL_SECTIONS, UART_INFORMATION            )
#define OFFSET_SRAM_DICC                     offsetof(SRAM_SMALL_SECTIONS, SRAM_DICC                   )
#define OFFSET_SRAM_DSP_DRV                  offsetof(SRAM_SMALL_SECTIONS, SRAM_DSP_DRV                )
#define OFFSET_SRAM_PCIE_INFO                offsetof(SRAM_SMALL_SECTIONS, SRAM_PCIE_INFO              )
#define OFFSET_SRAM_SEC_ROOTCA               offsetof(SRAM_SMALL_SECTIONS, SRAM_SEC_ROOTCA             )
#define OFFSET_SRAM_WDT_AM_FLAG              offsetof(SRAM_SMALL_SECTIONS, SRAM_WDT_AM_FLAG            )
#define OFFSET_SRAM_WDT_CM_FLAG              offsetof(SRAM_SMALL_SECTIONS, SRAM_WDT_CM_FLAG            )
#define OFFSET_SRAM_BUCK3_ACORE_ONOFF_FLAG   offsetof(SRAM_SMALL_SECTIONS, SRAM_BUCK3_ACORE_ONOFF_FLAG )
#define OFFSET_SRAM_BUCK3_CCORE_ONOFF_FLAG   offsetof(SRAM_SMALL_SECTIONS, SRAM_BUCK3_CCORE_ONOFF_FLAG )
#define OFFSET_SRAM_CUR_CPUFREQ_PROFILE      offsetof(SRAM_SMALL_SECTIONS, SRAM_CUR_CPUFREQ_PROFILE    )
#define OFFSET_SRAM_MAX_CPUFREQ_PROFILE      offsetof(SRAM_SMALL_SECTIONS, SRAM_MAX_CPUFREQ_PROFILE    )
#define OFFSET_SRAM_MIN_CPUFREQ_PROFILE      offsetof(SRAM_SMALL_SECTIONS, SRAM_MIN_CPUFREQ_PROFILE    )
#define OFFSET_SRAM_REBOOT_INFO              offsetof(SRAM_SMALL_SECTIONS, SRAM_REBOOT_INFO            )
#define OFFSET_SRAM_TEMP_PROTECT             offsetof(SRAM_SMALL_SECTIONS, SRAM_TEMP_PROTECT           )
#define OFFSET_SRAM_DLOAD                    offsetof(SRAM_SMALL_SECTIONS, SRAM_DLOAD                  )
#define OFFSET_SRAM_SEC_SHARE                offsetof(SRAM_SMALL_SECTIONS, SRAM_SEC_SHARE              )
#define OFFSET_SRAM_DSP_MNTN_INFO            offsetof(SRAM_SMALL_SECTIONS, SRAM_DSP_MNTN_INFO          )
#define OFFSET_SRAM_DFS_DDRC_CFG             offsetof(SRAM_SMALL_SECTIONS, SRAM_DFS_DDRC_CFG           )
#define OFFSET_SRAM_DUMP_POWER_OFF_FLAG      offsetof(SRAM_SMALL_SECTIONS, SRAM_DUMP_POWER_OFF_FLAG    )
#define OFFSET_SRAM_PM_CHECK_ADDR            offsetof(SRAM_SMALL_SECTIONS, SRAM_PM_CHECK_ADDR          )
#define OFFSET_SRAM_CDSP_MNTN_INFO           offsetof(SRAM_SMALL_SECTIONS, SRAM_CDSP_MNTN_INFO         )
#define OFFSET_SRAM_CDSP_DRV                 offsetof(SRAM_SMALL_SECTIONS, SRAM_CDSP_DRV               )

struct show_mem_info sram_small_section_infos[]={
    {"USB_ASHELL_ADDR             " , OFFSET_SRAM_USB_ASHELL              , 0 },
    {"UART_INFO_ADDR              " , OFFSET_UART_INFORMATION             , 0 },
    {"MEMORY_AXI_DICC_ADDR        " , OFFSET_SRAM_DICC                    , 0 },
    {"DSP_DRV_ADDR                " , OFFSET_SRAM_DSP_DRV                 , 0 },
    {"PCIE_INFO_ADDR              " , OFFSET_SRAM_PCIE_INFO               , 0 },
    {"SEC_ROOTCA_ADDR             " , OFFSET_SRAM_SEC_ROOTCA              , 0 },
    {"WDT_AM_FLAG_ADDR            " , OFFSET_SRAM_WDT_AM_FLAG             , 0 },
    {"WDT_CM_FLAG_ADDR            " , OFFSET_SRAM_WDT_CM_FLAG             , 0 },
    {"BUCK3_ACORE_ONOFF_FLAG_ADDR " , OFFSET_SRAM_BUCK3_ACORE_ONOFF_FLAG  , 0 },
    {"BUCK3_CCORE_ONOFF_FLAG_ADDR " , OFFSET_SRAM_BUCK3_CCORE_ONOFF_FLAG  , 0 },
    {"CUR_CPUFREQ_PROFILE_ADDR    " , OFFSET_SRAM_CUR_CPUFREQ_PROFILE     , 0 },
    {"MAX_CPUFREQ_PROFILE_ADDR    " , OFFSET_SRAM_MAX_CPUFREQ_PROFILE     , 0 },
    {"MIN_CPUFREQ_PROFILE_ADDR    " , OFFSET_SRAM_MIN_CPUFREQ_PROFILE     , 0 },
    {"REBOOT_ADDR                 " , OFFSET_SRAM_REBOOT_INFO             , 0 },
    {"TEMP_PROTECT_ADDR           " , OFFSET_SRAM_TEMP_PROTECT            , 0 },
    {"DLOAD_ADDR                  " , OFFSET_SRAM_DLOAD                   , 0 },
    {"SEC_SHARE                   " , OFFSET_SRAM_SEC_SHARE               , 0 },
    {"DSP_MNTN_INFO_ADDR          " , OFFSET_SRAM_DSP_MNTN_INFO           , 0 },
    {"DFS_DDRC_CFG_ADDR           " , OFFSET_SRAM_DFS_DDRC_CFG            , 0 },
    {"DUMP_POWER_OFF_FLAG_ADDR    " , OFFSET_SRAM_DUMP_POWER_OFF_FLAG     , 0 },
    {"PM_CHECK_ADDR               " , OFFSET_SRAM_PM_CHECK_ADDR           , 0 },
    {"CDSP_MNTN_INFO_ADDR         " , OFFSET_SRAM_CDSP_MNTN_INFO          , 0 },
    {"CDSP_DRV_ADDR               " , OFFSET_SRAM_CDSP_DRV                , 0 },
};
/*****************************************************************************
 函 数 名  : show_sram_status
 功能描述  : SRAM内存段信息打印
 输入参数  : 无
 输出参数  : 无
 返回值    ：无
*****************************************************************************/
int show_sram_status(void)
{
    int i,num;
    /* "name", "offset", "size" */
    char* str_format ="%-30s %10x %10x\n";
    /* "name", "offset" */
    char* str_format2 ="%-30s %10x \n";

    hwadp_printf("sram phy_addr %10pK, virt_addr %10pK, size 0x%x (hex)\n", SRAM_V2P((unsigned long)SRAM_BASE_ADDR),SRAM_BASE_ADDR,HI_SRAM_SIZE);
    hwadp_printf("%-30s %10s %10s \n", "name", "offset", "size");
    num = sizeof(sram_infos)/sizeof(sram_infos[0]);
    for(i = 0; i < num; ++i){
        hwadp_printf(str_format,sram_infos[i].name, sram_infos[i].value, sram_infos[i].size);
    }

    hwadp_printf("\n detailed info of SMALL_SECTIONS:offset:0x%x size:0x%x \n",SRAM_OFFSET_SMALL_SECTIONS , SRAM_SIZE_SMALL_SECTIONS );
    hwadp_printf("%-30s %10s\n", "name", "offset");
    num = sizeof(sram_small_section_infos)/sizeof(sram_small_section_infos[0]);
    for(i = 0; i < num; ++i){
        hwadp_printf(str_format2,sram_small_section_infos[i].name, sram_small_section_infos[i].value);
    }
    return 0;
}

struct show_mem_info shared_ddr_infos[]={
    {"HIFI_MBX              " , SHM_OFFSET_HIFI_MBX            ,  SHM_SIZE_HIFI_MBX            },
    {"HIFI                  " , SHM_OFFSET_HIFI                ,  SHM_SIZE_HIFI                },
    {"TLPHY                 " , SHM_OFFSET_TLPHY               ,  SHM_SIZE_TLPHY               },
    {"TEMPERATURE           " , SHM_OFFSET_TEMPERATURE         ,  SHM_SIZE_TEMPERATURE         },
    {"DDM_LOAD              " , SHM_OFFSET_DDM_LOAD            ,  SHM_SIZE_DDM_LOAD            },
    {"MEM_APPA9_PM_BOOT     " , SHM_OFFSET_APPA9_PM_BOOT       ,  SHM_SIZE_MEM_APPA9_PM_BOOT   },
    {"MEM_MDMA9_PM_BOOT     " , SHM_OFFSET_MDMA9_PM_BOOT       ,  SHM_SIZE_MEM_MDMA9_PM_BOOT   },
    {"TENCILICA_MULT_BAND   " , SHM_OFFSET_TENCILICA_MULT_BAND ,  SHM_SIZE_TENCILICA_MULT_BAND },
    {"ICC                   " , SHM_OFFSET_ICC                 ,  SHM_SIZE_ICC                 },
    {"IPF                   " , SHM_OFFSET_IPF                 ,  SHM_SIZE_IPF                 },
    {"PSAM                  " , SHM_OFFSET_PSAM                ,  SHM_SIZE_PSAM                },
    {"WAN                   " , SHM_OFFSET_WAN                 ,  SHM_SIZE_WAN                 },
    {"NV                    " , SHM_OFFSET_NV                  ,  SHM_SIZE_NV                  },
    {"M3_MNTN               " , SHM_OFFSET_M3_MNTN             ,  SHM_SIZE_M3_MNTN             },
    {"TIMESTAMP             " , SHM_OFFSET_TIMESTAMP           ,  SHM_SIZE_TIMESTAMP           },
    {"IOS                   " , SHM_OFFSET_IOS                 ,  SHM_SIZE_IOS                 },
    {"RESTORE_AXI           " , SHM_OFFSET_RESTORE_AXI         ,  SHM_SIZE_RESTORE_AXI         },
    {"PMU                   " , SHM_OFFSET_PMU                 ,  SHM_SIZE_PMU                 },
    {"PTABLE                " , SHM_OFFSET_PTABLE              ,  SHM_SIZE_PTABLE              },
    {"CCORE_RESET           " , SHM_OFFSET_CCORE_RESET         ,  SHM_SIZE_CCORE_RESET         },
    {"PM_OM                 " , SHM_OFFSET_PM_OM               ,  SHM_SIZE_PM_OM               },
    {"M3PM                  " , SHM_OFFSET_M3PM                ,  SHM_SIZE_M3PM                },
    {"SLICE_MEM             " , SHM_OFFSET_SLICE_MEM           ,  SHM_SIZE_SLICE_MEM           },
    {"OSA_LOG               " , SHM_OFFSET_OSA_LOG             ,  SHM_SIZE_OSA_LOG             },
    {"WAS_LOG               " , SHM_OFFSET_WAS_LOG             ,  SHM_SIZE_WAS_LOG             },
    {"SRAM_BAK              " , SHM_OFFSET_SRAM_BAK            ,  SHM_SIZE_SRAM_BAK            },
    {"SRAM_TO_DDR           " , SHM_OFFSET_SRAM_TO_DDR         ,  SHM_SIZE_SRAM_TO_DDR         },
    {"M3RSRACC_BD           " , SHM_OFFSET_M3RSRACC_BD         ,  SHM_SIZE_M3RSRACC_BD         },
    {"SIM_MEMORY            " , SHM_OFFSET_SIM_MEMORY          ,  SHM_SIZE_SIM_MEMORY          },
    {"MEMMGR                " , SHM_OFFSET_MEMMGR              ,  SHM_SIZE_MEMMGR              },
};

struct show_mem_info shared_ddr_slice_infos[]={
    {"MEMMGR_FLAG    " , SHM_OFFSET_MEMMGR_FLAG      ,  SHM_SIZE_MEMMGR_FLAG      },
    {"SYNC           " , SHM_OFFSET_SYNC             ,  SHM_SIZE_SYNC             },
    {"AT_FLAG        " , SHM_OFFSET_AT_FLAG          ,  SHM_SIZE_AT_FLAG          },
    {"CSHELL_FLAG    " , SHM_OFFSET_CHSELL_FLAG      ,  SHM_SIZE_CSHELL_FLAG      },
    {"DSP_FLAG       " , SHM_OFFSET_DSP_FLAG         ,  SHM_SIZE_DSP_FLAG         },
    {"CDSP_FLAG      " , SHM_OFFSET_CDSP_FLAG        ,  SHM_SIZE_CDSP_FLAG        },
    {"CCORE_FIQ      " , SHM_OFFSET_CCORE_FIQ        ,  SHM_SIZE_CCORE_FIQ        },
    {"LOADM          " , SHM_OFFSET_LOADM            ,  SHM_SIZE_LOADM            },
    {"MEMREPAIR      " , SHM_OFFSET_MEMREPAIR        ,  SHM_SIZE_MEMREPAIR        },
    {"NAND_SPEC      " , SHM_OFFSET_NAND_SPEC        ,  SHM_SIZE_NAND_SPEC        },
    {"VERSION        " , SHM_OFFSET_VERSION          ,  SHM_SIZE_VERSION          },
    {"UART_FLAG      " , SHM_OFFSET_UART_FLAG        ,  SHM_SIZE_UART_FLAG        },
    {"M3PM_LOG       " , SHM_OFFSET_M3PM_LOG         ,  SHM_SIZE_M3PM_LOG         },
    {"PAN_RPC        " , SHM_OFFSET_PAN_RPC          ,  SHM_SIZE_PAN_RPC          },
    {"MODEM_SR_STAMP " , SHM_OFFSET_MODEM_SR_STAMP   ,  SHM_SIZE_MODEM_SR_STAMP   },
    {"TSENSOR_STAMP  " , SHM_OFFSET_TSENSOR_STAMP    ,  SHM_SIZE_TSENSOR_STAMP    },
    {"RFFE_VIA_FLAG  " , SHM_OFFSET_RFFE_VIA_LP_FLAG ,  SHM_SIZE_RFFE_VIA_LP_FLAG },
    {"SEC_SOC_ID     " , SHM_OFFSET_SEC_SOC_ID       ,  SHM_SIZE_SEC_SOC_ID       },
};
/*****************************************************************************
 函 数 名  : show_shared_ddr_status
 功能描述  : 共享内存内存段信息打印
 输入参数  : 无
 输出参数  : 无
 返回值    ：无
*****************************************************************************/
int show_shared_ddr_status(void)
{
    int i,num;
    /* "name", "offset", "size" */
    char* str_format ="%-30s %10x %10x\n";

    hwadp_printf("shared_ddr phy_addr %10pK virt_addr %10pK size:0x%x (hex)\n", SHD_DDR_V2P((unsigned long)SHM_BASE_ADDR), SHM_BASE_ADDR,DDR_SHARED_MEM_SIZE);
    hwadp_printf("%-30s %10s %10s\t\n", "name", "offset", "size");
    num = sizeof(shared_ddr_infos)/sizeof(shared_ddr_infos[0]);
    for(i = 0; i < num; ++i){
        hwadp_printf(str_format,shared_ddr_infos[i].name, shared_ddr_infos[i].value, shared_ddr_infos[i].size);
    }

    hwadp_printf("\n detailed info of SLICE_MEM offset:0x%x size:0x%x \n", SHM_OFFSET_SLICE_MEM , SHM_SIZE_SLICE_MEM );
    hwadp_printf("%-30s %10s %10s\t\n", "name", "offset", "size");
    num = sizeof(shared_ddr_slice_infos)/sizeof(shared_ddr_slice_infos[0]);
    for(i = 0; i < num; ++i){
        hwadp_printf(str_format,shared_ddr_slice_infos[i].name, shared_ddr_slice_infos[i].value, shared_ddr_slice_infos[i].size);
    }
    return 0;
}
EXPORT_SYMBOL(show_global_ddr_status);
EXPORT_SYMBOL(show_sram_status);
EXPORT_SYMBOL(show_shared_ddr_status);

int show_hpm_temp(void)
{
	return 0;
}
EXPORT_SYMBOL(show_hpm_temp);
/*lint -restore */ 


