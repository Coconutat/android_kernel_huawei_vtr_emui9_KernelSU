/*
* NoC. (NoC Mntn Module.)
*
* Copyright (c) 2016 Huawei Technologies CO., Ltd.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

/*****************************************************************************
  1 头文件包含
 *****************************************************************************/
#include <linux/io.h>
#include <linux/string.h>

#include "hisi_noc.h"
#include "hisi_noc_info.h"
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG HISI_NOC_TAG

#define ERR_CODE_NR  8
#define OPC_NR      10

#define NOC_CFG_SYS_BUS_ID              0
#define NOC_VCODEC_BUS_ID               1
#define NOC_VIVO_BUS_ID                 2

#define CFG_INITFLOW_ARRAY_SIZE         32
#define CFG_TARGETFLOW_ARRAY_SIZE       64
#define VCODEC_INITFLOW_ARRAY_SIZE      4
#define VCODEC_TARGETFLOW_ARRAY_SIZE    16
#define VIVO_INITFLOW_ARRAY_SIZE        32
#define VIVO_TARGETFLOW_ARRAY_SIZE      16

static char *cfg_initflow_array[] = {
	"Audio(noc_asp_mst)",                 /*0 */
	"ACPU(noc_cci2sysbus)",               /*1 */
	"DJTAG(noc_djtag_mst)",               /*2 */
	"DMA-Controller(noc_dmac_mst)",       /*3 */
	"DPCTRL(noc_dpctrl)",                 /*4 */
	"IOMCU(noc_iomcu_ahb_mst)",           /*5 */
	"IOMCU-DMA(noc_iomcu_dma_mst)",       /*6 */
	"TZP(noc_iomcu_tzp_ahb_mst)",         /*7 */
	"IPF(noc_ipf)",                       /*8 */
	"LPM3(noc_lpmcu_mst)",                /*9 */
	"MEDIA-CFG(noc_media2cfg)",           /*A */
	"MODEM(noc_modem_mst)",               /*B */
	"PCIE(noc_pcie)",                     /*C */
	"PERF_STAT_DEBUG(noc_perf_stat)",     /*D */
	"PIMON(noc_pi_mon)",                  /*E */
	"SD3(noc_sd3)",                       /*F */
	"SDIO(noc_sdio)",                     /*10*/
	"SECURITY-P(noc_sec_p)",              /*11*/
	"SECURITY-S(noc_sec_s)",              /*12*/
	"SOCP_DEBUG(noc_socp)",               /*13*/
	"CORESIGHT(noc_top_cssys)",           /*14*/
	"UFS(noc_ufs_mst)",                   /*15*/
	"USB3(noc_usb3otg)",                  /*16*/
};

static char *cfg_targetflow_array[] = {
	"AOBUS(aobus_service_target)",                /*0 */
	"Audio(aspbus_service_target)",               /*1 */
	"Service_target(cfgbus_service_target)",      /*2 */
	"Service_target(dmadebugbus_service_target)", /*3 */
	"Service_target(mmc0bus_service_target)",     /*4 */
	"Service_target(mmc1bus_service_target)",     /*5 */
	"Service_target(modembus_service_target)",    /*6 */
	"AON(noc_aon_apb_slv)",                       /*7 */
	"Audio(noc_asp_cfg)",                         /*8 */
	"CCI(noc_cci_cfg)",                           /*9 */
	"CFG-MEDIA(noc_cfg2media)",                   /*A */
	"CFG-VCODEC(noc_cfg2vcodec)",                 /*B */
	"DMA controller(noc_dmac_cfg)",               /*C */
	"GIC(noc_gic)",                               /*D */
	"GPU(noc_gpu_cfg)",                           /*E */
	"HISEE(noc_hisee_cfg)",                       /*F */
	"SSI(noc_hkadc_ssi)",                         /*10*/
	"IOMCU(noc_iomcu_ahb_slv)",                   /*11*/
	"IOMCU(noc_iomcu_apb_slv)",                   /*12*/
	"LPM3(noc_lpmcu_slv)",                        /*13*/
	"MMC0BUS(noc_mmc0bus_apb_cfg)",               /*14*/
	"MMC1BUS(noc_mmc1bus_apb_cfg)",               /*15*/
	"MODEM(noc_modem_cfg)",                       /*16*/
	"PCIE(noc_pcie_cfg)",                         /*17*/
	"DDRC(noc_sys2ddrc)",                         /*18*/
	"CFG(noc_sys_peri0_cfg)",                     /*19*/
	"CFG(noc_sys_peri1_cfg)",                     /*1A*/
	"DMA(noc_sys_peri2_cfg)",                     /*1B*/
	"DMA(noc_sys_peri3_cfg)",                     /*1C*/
	"CORESIGHT(noc_top_cssys_slv)",               /*1D*/
	"UFS-CFG(noc_ufs_cfg)",                       /*1E*/
	"USB3(noc_usb3otg_cfg)",                      /*1F*/
	"Service_target(sysbus_service_target)",      /*20*/
	"RESERVED",                                   /*21*/
	"RESERVED",                                   /*22*/
	"RESERVED",                                   /*23*/
	"RESERVED",                                   /*24*/
	"RESERVED",                                   /*25*/
	"RESERVED",                                   /*26*/
	"RESERVED",                                   /*27*/
	"RESERVED",                                   /*28*/
	"RESERVED",                                   /*29*/
	"RESERVED",                                   /*2A*/
	"RESERVED",                                   /*2B*/
	"RESERVED",                                   /*2C*/
};

static char *vcodec_initflow_array[] = {
	"ICS(noc_ics)",
	"VCODEC(noc_vcodec_cfg)",
	"VDEC(noc_vdec)",
	"VENC(noc_venc)",
};

static char *vcodec_targetflow_array[] = {
	"service_target(ics_service_target)",
	"ICS-CFG(noc_ics_cfg)",
	"ICS-MMU(noc_ics_mmu_cfg)",
	"CRG-CFG(noc_vcodec_crg_cfg)",
	"DDRC0(noc_vcodecbus_ddrc0)",
	"DDRC0(noc_vcodecbus_ddrc1)",
	"VDEC(noc_vdec_cfg)",
	"VENC(noc_venc_cfg)",
	"service_target(vcdoecbus_service_target)",
	"service_target(vdec_service_target)",
	"service_target(venc_service_target)",
};

static char *vivo_initflow_array[] = {
	"CFG-MEDIA(noc_cfg2media)",
	"DSS0-RD(noc_dss0_rd)",
	"DSS0-WR(noc_dss0_wr)",
	"DSS1-RD(noc_dss1_rd)",
	"DSS1-WR(noc_dss1_wr)",
	"ISP1-RD(noc_isp1_rd)",
	"ISP1-WR(noc_isp1_wr)",
	"ISP-CPU-PER(noc_isp_cpu_per)",
	"ISP_CPU-RD(noc_isp_cpu_rd)",
	"ISP-CPU-WR(noc_isp_cpu_wr)",
	"ISP-RD(noc_isp_rd)",
	"ISP-WR(noc_isp_wr)",
	"IVP32(noc_ivp32_mst)",
	"JPEG-FD-RD(noc_jpeg_fd_rd)",
	"JPEG-FD-WR(noc_jpeg_fd_wr)",
	"MEDIA-COM-RD(noc_media_com_rd)",
	"MEDIA-COM-WR(noc_media_com_wr)",
};

static char *vivo_targetflow_array[] = {
	"service_target(dss_service_target)",
	"service_target(isp_service_target)",
	"service_target(ivp32bus_service_target)",
	"service_target(noc_dss_cfg)",
	"ISP_CFG(noc_isp_cfg)",
	"IVP32-CFG(noc_ivp32_cfg)",
	"JPEG-FD-CFG(noc_jpeg_fd_cfg)",
	"MEDIA-CFG(noc_media2cfg)",
	"MEDIA-CRG(noc_media_crg_cfg)",
	"DDRC0_RD(noc_vivobus_ddrc0_rd)",
	"DDRC0_WR(noc_vivobus_ddrc0_wr)",
	"DDRC1_RD(noc_vivobus_ddrc1_rd)",
	"DDRC1_WR(noc_vivobus_ddrc1_wr)",
	"service_target(vivo_service_target)",
};

/*
 * unsigned int  opc      : 4;  bit[1-4]  : Transaction操作类型：
 * 0-->RD：INCR的读操作；
 * 1-->RDW：WRAP的读操作；
 * 2-->RDL：Exclusive 读操作；
 * 3-->RDX：Lock read；
 * 4-->WR：INCR写操作；
 * 5-->WRW：WRAP写操作；
 * 6-->WRC：Exclusive写操作；
 * 8-->PRE：FIXED BURST；
 * 9-->URG：urgency packet，Error Probe不会检测到此类packet
 * 其它-->Reserveed：非法操作
 */
static char *opc_array[] = {
	"RD:  INCR READ",
	"RDW: WRAP READ",
	"RDL: EXCLUSIVE READ",
	"RDX: LOCK READ",
	"WR:  INCR WRITE",
	"WRW: WRAP WRITE",
	"WRC：EXCLUSIVE WRITE",
	"Reversed",
	"PRE: FIXED BURST",
	"Abnormal"
};

/*
 * unsigned int  errcode  : 3;  bit[8-10] : 错误类型
 * 0-->被访问slave返回Error Response；
 * 1-->master访问了总线的Reserve地址空间；
 * 2-->master发送了slave不支持的Burst类型（当前总线不会出现这种场景）；
 * 3-->master访问了掉电区域；
 * 4-->访问权限报错；
 * 5-->master访问时，收到Firewall的Hide Error Response；
 * 6-->被访问slave TimeOut，并返回Error Response；
 * 7-->none
 */
static char *err_code_array[] = {
	"Slave:  returns Error response",
	"Master: access reserved memory space",
	"Master: send the illigel type burst to slave",
	"Master: access the powerdown area",
	"Master: Permission error",
	"Master: received Hide Error Response from Firewall",
	"Master: accessed slave timeout and returned Error reponse",
	"None"
};

static const ROUTE_ID_ADDR_STRU cfgsys_routeid_addr_tbl[] = {
	/*Init_flow_bit   Targ_flow_bit    Targ subrange  Init localAddress*/
	/*-----------------------------------------------------------------*/
	{0x02, 0x00, 0x0, 0xe9870000},/*aobus_service_target*/
	{0x02, 0x01, 0x0, 0xe9830000},/*aspbus_service_target*/
	{0x02, 0x02, 0x0, 0xe9800000},/*cfgbus_service_target*/
	{0x02, 0x03, 0x0, 0xe9860000},/*dmadebugbus_service_target*/
	{0x02, 0x04, 0x0, 0xe9890000},/*mmc0bus_service_target*/
	{0x02, 0x05, 0x0, 0xe9880000},/*mmc1bus_service_target*/
	{0x02, 0x06, 0x0, 0xe9820000},/*modembus_service_target*/
	{0x02, 0x07, 0x0, 0xfff20000},/*noc_aon_apb_slv*/
	{0x02, 0x07, 0x1, 0xfff00000},/*noc_aon_apb_slv*/
	{0x02, 0x08, 0x0, 0xe7f00000},/*noc_asp_cfg*/
	{0x02, 0x08, 0x1, 0xe8000000},/*noc_asp_cfg*/
	{0x02, 0x09, 0x0, 0xe8100000},/*noc_cci_cfg*/
	{0x02, 0x0A, 0x0, 0xe8300000},/*noc_cfg2media*/
	{0x02, 0x0A, 0x1, 0xe8c00000},/*noc_cfg2media*/
	{0x02, 0x0A, 0x2, 0xe8400000},/*noc_cfg2media*/
	{0x02, 0x0B, 0x0, 0xe8940000},/*noc_cfg2vcodec*/
	{0x02, 0x0B, 0x1, 0xe8900000},/*noc_cfg2vcodec*/
	{0x02, 0x0B, 0x2, 0xe8800000},/*noc_cfg2vcodec*/
	{0x02, 0x0B, 0x3, 0xff400000},/*noc_cfg2vcodec*/
	{0x02, 0x0C, 0x0, 0xfdf30000},/*noc_dmac_cfg*/
	{0x02, 0x0D, 0x0, 0xe82b0000},/*noc_gic*/
	{0x02, 0x0E, 0x0, 0xe82c0000},/*noc_gpu_cfg*/
	{0x02, 0x0F, 0x0, 0xf0e00000},/*noc_hisee_cfg*/
	{0x02, 0x10, 0x0, 0xe82b8000},/*noc_hkadc_ssi*/
	{0x02, 0x11, 0x0, 0xf0800000},/*noc_iomcu_ahb_slv*/
	{0x02, 0x11, 0x1, 0xf0000000},/*noc_iomcu_ahb_slv*/
	{0x02, 0x12, 0x0, 0xffd00000},/*noc_iomcu_apb_slv*/
	{0x02, 0x13, 0x0, 0xea900000},/*noc_lpmcu_slv*/
	{0x02, 0x13, 0x1, 0xffe00000},/*noc_lpmcu_slv*/
	{0x02, 0x13, 0x2, 0xed000000},/*noc_lpmcu_slv*/
	{0x02, 0x13, 0x3, 0xec000000},/*noc_lpmcu_slv*/
	{0x02, 0x14, 0x0, 0xff340000},/*noc_mmc0bus_apb_cfg*/
	{0x02, 0x15, 0x0, 0xfc000000},/*noc_mmc1bus_apb_cfg*/
	{0x02, 0x16, 0x0, 0xe0000000},/*noc_modem_cfg*/
	{0x02, 0x17, 0x0, 0xf4000000},/*noc_pcie_cfg*/
	{0x02, 0x18, 0x0, 0xc0000000},/*noc_sys2ddrc*/
	{0x02, 0x18, 0x1, 0x80000000},/*noc_sys2ddrc*/
	{0x02, 0x18, 0x2, 0x0},/*noc_sys2ddrc*/
	{0x02, 0x18, 0x3, 0x100000000},/*noc_sys2ddrc*/
	{0x02, 0x18, 0x4, 0x200000000},/*noc_sys2ddrc*/
	{0x02, 0x19, 0x0, 0xe8a00000},/*noc_sys_peri0_cfg*/
	{0x02, 0x1A, 0x0, 0xe8960000},/*noc_sys_peri1_cfg*/
	{0x02, 0x1A, 0x1, 0xe8980000},/*noc_sys_peri1_cfg*/
	{0x02, 0x1B, 0x0, 0xff000000},/*noc_sys_peri2_cfg*/
	{0x02, 0x1C, 0x0, 0xfdf00000},/*noc_sys_peri3_cfg*/
	{0x02, 0x1D, 0x0, 0xfe000000},/*noc_top_cssys_slv*/
	{0x02, 0x1E, 0x0, 0xff3e0000},/*noc_ufs_cfg*/
	{0x02, 0x1E, 0x1, 0xff3c0000},/*noc_ufs_cfg*/
	{0x02, 0x1F, 0x0, 0xff200000},/*noc_usb3otg_cfg*/
	{0x02, 0x1F, 0x1, 0xff100000},/*noc_usb3otg_cfg*/
	{0x02, 0x20, 0x0, 0xe9840000},/*sysbus_service_target*/
	{0x02, 0x20, 0x1, 0x0},/*sysbus_service_target*/
};

/* vcodec_bus */
static const ROUTE_ID_ADDR_STRU vcodec_routeid_addr_tbl[] = {
	/* Init_flow  Targ_flow  Targ_subrange  Init_localAddress*/
	/* ---------------------------------------------------*/
	{0x01, 0x00, 0x0, 0xe8950000},/*ics_service_target*/
	{0x01, 0x01, 0x0, 0xff400000},/*noc_ics_cfg*/
	{0x01, 0x02, 0x0, 0xff480000},/*noc_ics_mmu_cfg*/
	{0x01, 0x03, 0x0, 0xe8900000},/*noc_vcodec_crg_cfg*/
	{0x01, 0x04, 0x0, 0x0},/*noc_vcodecbus_ddrc0*/
	{0x00, 0x04, 0x1, 0x10000000000},/*noc_vcodecbus_ddrc0*/
	{0x00, 0x05, 0x0, 0x8000000000},/*noc_vcodecbus_ddrc1*/
	{0x00, 0x05, 0x1, 0x18000000000},/*noc_vcodecbus_ddrc1*/
	{0x00, 0x06, 0x0, 0xe8800000},/*noc_vdec_cfg*/
	{0x00, 0x07, 0x0, 0xe8880000},/*noc_venc_cfg*/
	{0x00, 0x08, 0x0, 0xe8920000},/*vcdoecbus_service_target*/
	{0x00, 0x08, 0x1, 0x0},/*vcdoecbus_service_target*/
	{0x00, 0x09, 0x0, 0xe8930000},/*vdec_service_target*/
	{0x00, 0x0A, 0x0, 0xe8940000},/*venc_service_target*/
};

/* vivo_bus */
static const ROUTE_ID_ADDR_STRU vivo_routeid_addr_tbl[] = {
	/* Init_flow  Targ_flow  Targ_subrange Init_localAddress */
	/* ----------------------------------------------------- */
	{0x0A, 0x00, 0x0, 0xe86c0000},/*dss_service_target*/
	{0x0A, 0x01, 0x0, 0xe86d0000},/*isp_service_target*/
	{0x0A, 0x02, 0x0, 0xe86e0000},/*ivp32bus_service_target*/
	{0x0A, 0x03, 0x0, 0xe8780000},/*noc_dss_cfg*/
	{0x0A, 0x03, 0x1, 0xe8680000},/*noc_dss_cfg*/
	{0x0A, 0x03, 0x2, 0xe8600000},/*noc_dss_cfg*/
	{0x0A, 0x03, 0x3, 0xe8700000},/*noc_dss_cfg*/
	{0x0A, 0x04, 0x0, 0xe8400000},/*noc_isp_cfg*/
	{0x0A, 0x05, 0x0, 0xe8c00000},/*noc_ivp32_cfg*/
	{0x0A, 0x06, 0x0, 0xe8300000},/*noc_jpeg_fd_cfg*/
	{0x00, 0x07, 0x0, 0x0},/*noc_media2cfg*/
	{0x00, 0x08, 0x0, 0xe87ff000},/*noc_media_crg_cfg*/
	{0x01, 0x09, 0x0, 0x0},/*noc_vivobus_ddrc0_rd*/
	{0x01, 0x09, 0x1, 0x10000000000},/*noc_vivobus_ddrc0_rd*/
	{0x01, 0x0A, 0x0, 0x0},/*noc_vivobus_ddrc0_wr*/
	{0x01, 0x0A, 0x1, 0x10000000000},/*noc_vivobus_ddrc0_wr*/
	{0x00, 0x0B, 0x0, 0x8000000000},/*noc_vivobus_ddrc1_rd*/
	{0x00, 0x0B, 0x1, 0x18000000000},/*noc_vivobus_ddrc1_rd*/
	{0x01, 0x0C, 0x0, 0x8000000000},/*noc_vivobus_ddrc1_wr*/
	{0x01, 0x0C, 0x1, 0x18000000000},/*noc_vivobus_ddrc1_wr*/
	{0x01, 0x0D, 0x0, 0xe86f0000},/*vivo_service_target*/
	{0x01, 0x0D, 0x1, 0x0},/*vivo_service_target*/
};

struct noc_mid_info noc_mid_kirin970[] = {
	/*Bus ID,     init_flow       ,mask   ,mid_va,        mid name */
	{0, 0x01, 0x3800, 0x0, "AP_CPU0"}, /*noc_cci2sysbus,*/
	{0, 0x01, 0x3800, 0x800, "AP_CPU1"},   /*noc_cci2sysbus,*/
	{0, 0x01, 0x3800, 0x1000, "AP_CPU2"},  /*noc_cci2sysbus,*/
	{0, 0x01, 0x3800, 0x1800, "AP_CPU3"},  /*noc_cci2sysbus,*/
	{0, 0x01, 0x3800, 0x2000, "AP_CPU4"},  /*noc_cci2sysbus,*/
	{0, 0x01, 0x3800, 0x2800, "AP_CPU5"},  /*noc_cci2sysbus,*/
	{0, 0x01, 0x3800, 0x3000, "AP_CPU6"},  /*noc_cci2sysbus,*/
	{0, 0x01, 0x3800, 0x3800, "AP_CPU7"},  /*noc_cci2sysbus,*/
	{0, 0x09, 0x003f, 0x00, "LPMCU"},
	{0, 0x05, 0x003f, 0x01, "IOMCU_M7"},
	{0, 0x0E, 0x003f, 0x02, "PI_MON"},
	{0, 0x0D, 0x003f, 0x03, "PERF_STAT"},
	{0, 0x08, 0x003f, 0x04, "IPF"},
	{0, 0x02, 0x003f, 0x05, "DJTAG_M"},
	{0, 0x06, 0x003f, 0x06, "IOMCU_DMA"},
	{0, 0x15, 0x003f, 0x07, "UFS"},
	{0, 0x0F, 0x003f, 0x08, "SD"},
	{0, 0x10, 0x003f, 0x09, "SDIO"},
	{0, 0x12, 0x003f, 0x0A, "SEC-S"},
	{0, 0x11, 0x003f, 0x0B, "SEC-P"},
	{0, 0x04, 0x003f, 0x0C, "DPC"},
	{0, 0x13, 0x003f, 0x0D, "SOCP"},
	{0, 0x16, 0x003f, 0x0E, "USB31OTG"},
	{0, 0x14, 0x003f, 0x0F, "TOP_CSSYS"},
	{0, 0x03, 0x003f, 0x10, "DMAC"},
	{0, 0x00, 0x003f, 0x11, "ASP"},
	{0, 0x0c, 0x003f, 0x12, "PCIE_0"},
	{0, 0x0B, 0x003f, 0x24, "DFC"},  /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x16, "CIPHER"},  /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x36, "HDLC"},  /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x17, "CICOM0"},  /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x37, "CICOM1"},  /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x18, "BBE16_0"},  /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x38, "BBE16_C"},  /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x19, "BBP_DMA_0"},/*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x39, "BBP_DMA_1"},/*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x1A, "GU_BBP_MST_0"},    /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x3A, "GU_BBP_MST_1"},    /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x1B, "EDMA0"},  /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x3B, "EDMA1"},  /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x1C, "HARQ_L"},    /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x3C, "HARQ_H"},    /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x1D, "UPACC"}, /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x3D, "RSR_ACC"}, /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x1E, "CIPHER_WRITE_THOUGH"},   /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x3F, "CCPU_CFG"},   /*noc_modem_mst,*/
	{0, 0x0B, 0x003f, 0x1F, "CCPU_L2C"},   /*noc_modem_mst,*/
	{2, 0xFF, 0x003f, 0x00, "ISP"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x01, "ISP"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x02, "ISP"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x03, "ISP"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x04, "ISP"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x05, "ISP"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x06, "ISP"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x07, "ISP"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x08, "ISP1"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x09, "ISP1"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x0A, "ISP1"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x0B, "ISP1"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x0C, "ISP1"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x0D, "ISP1"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x0E, "ISP1"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x0F, "ISP1"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x10, "dss_wr_ch2"},/*dss*/
	{2, 0xFF, 0x003f, 0x11, "dss_wr_ch1"},/*dss*/
	{2, 0xFF, 0x003f, 0x12, "dss_wr_ch0"},/*dss*/
	{2, 0xFF, 0x003f, 0x13, "dss_rd_ch8"},/*dss*/
	{2, 0xFF, 0x003f, 0x14, "dss_rd_ch7"},/*dss*/
	{2, 0xFF, 0x003f, 0x15, "dss_rd_ch6"},/*dss*/
	{2, 0xFF, 0x003f, 0x16, "dss_rd_ch5"},/*dss*/
	{2, 0xFF, 0x003f, 0x17, "dss_rd_ch4"},/*dss*/
	{2, 0xFF, 0x003f, 0x18, "dss_rd_ch3"},/*dss*/
	{2, 0xFF, 0x003f, 0x19, "dss_rd_ch2"},/*dss*/
	{2, 0xFF, 0x003f, 0x1A, "dss_rd_ch1"},/*dss*/
	{2, 0xFF, 0x003f, 0x1B, "dss_rd_ch0_or_DSS_ATGM"},/*dss*/
	{2, 0xFF, 0x003f, 0x1C, "ISP_R8_OR_ATGM"},/*dss*/
	{2, 0xFF, 0x003f, 0x1D, "JPEG_FD_SUBSYS"},/*dss*/
	{2, 0xFF, 0x003f, 0x1E, "MEDIA_COMMON_CMDLIST"},/*MEDIA_COMMON*/
	{2, 0xFF, 0x003f, 0x1F, "MEDIA_COMMON_RCH_WCH"},/*MEDIA_COMMON*/
	{1, 0x03, 0x003f, 0x20, "VENC1"},/*VENC*/
	{1, 0x03, 0x003f, 0x21, "VENC2"},/*VENC*/
	{1, 0x02, 0x003f, 0x22, "VDEC1"},/*VDEC*/
	{1, 0x02, 0x003f, 0x23, "VENC2"},/*VDEC*/
	{1, 0x02, 0x003f, 0x24, "VDEC3"},/*VDEC*/
	{1, 0x02, 0x003f, 0x25, "VENC4"},/*VDEC*/
	{1, 0x02, 0x003f, 0x26, "VDEC5"},/*VDEC*/
	{1, 0x02, 0x003f, 0x27, "VENC6"},/*VDEC*/
	{1, 0x00, 0x003f, 0x28, "ICS"},/*ICS*/
	{2, 0x0C, 0x003f, 0x29, "DSP_CORE_OR_IVP_ATGM"},/*IVP*/
	{2, 0x0C, 0x003f, 0x2A, "DSP_DMA"},/*IVP*/
	{2, 0xFF, 0x003f, 0x2B, "ISP_A7_CFG"},/*ISP*/
};

struct noc_sec_info noc_sec_kirin970[] = {
	/*mask value info  sec_mode*/
	{0x03, 0x00, "trusted", "TZMP2 NSAID"},/*TZMP2 NSAID*/
	{0x03, 0x01, "non-trusted", "TZMP2 NSAID"},/*TZMP2 NSAID*/
	{0x03, 0x02, "protected", "TZMP2 NSAID"},/*TZMP2 NSAID*/
	{0x03, 0x03, "ACPU", "TZMP2 NSAID"},/*TZMP2 NSAID*/
	{0x04, 0x00, "secure", "secure"},/*secure*/
	{0x04, 0x04, "non-secure", "secure"},/*secure*/
};

struct noc_dump_reg noc_dump_reg_list_kirin970[] = {
	/* Table.1 NoC Register Dump */
	{"PCTRL", (void *)NULL, 0x050},//PCTRL_PERI_CTRL19},
	{"PCTRL", (void *)NULL, 0x094},//PCTRL_PERI_STAT0},
	{"PCTRL", (void *)NULL, 0x09c},//PCTRL_PERI_STAT2},
	{"PCTRL", (void *)NULL, 0x0A0},//PCTRL_PERI_STAT3},
	{"SCTRL", (void *)NULL, 0x0B4},//SCTRL_SCINT_MASK1},
	{"PMCTRL", (void *)NULL, 0x3A0},//PMCTRL_PERI_INT0_MASK},
	{"SCTRL", (void *)NULL, 0x378},//SCTRL_SCPERSTATUS6},
	{"SCTRL", (void *)NULL, 0x0B8},//SCTRL_SCINT_STAT1},
	{"SCTRL", (void *)NULL, 0x314},//SCTRL_SCPERCTRL5},
	{"SCTRL", (void *)NULL, 0x31C},//SCTRL_SCPERCTRL7},
	{"SCTRL", (void *)NULL, 0x58C},//SCTRL_SCIOMCUSTAT},
	{"PMCTRL", (void *)NULL, 0x380},//PMCTRL_NOC_POWER_IDLEREQ},
	{"PMCTRL", (void *)NULL, 0x384},//PMCTRL_NOC_POWER_IDLEACK},
	{"PMCTRL", (void *)NULL, 0x388},//PMCTRL_NOC_POWER_IDLE},
	{"PMCTRL", (void *)NULL, 0x3A4},//PMCTRL_PERI_INT0_STAT},
	{"CRGPERIPH", (void *)NULL, 0x12C},//CRGPERIPH_PERI_CTRL3},

	/* Table.2 NoC Register Dump */
	{"CRGPERIPH", (void *)NULL, 0x008},//CRGPERIPH_PERCLKEN0},
	{"CRGPERIPH", (void *)NULL, 0x028},//CRGPERIPH_PERCLKEN2},
	{"CRGPERIPH", (void *)NULL, 0x038},//CRGPERIPH_PERCLKEN3},
	{"CRGPERIPH", (void *)NULL, 0x048},//CRGPERIPH_PERCLKEN4},
	{"CRGPERIPH", (void *)NULL, 0x104},//CRGPERIPH_CLKDIV23},
	{"CRGPERIPH", (void *)NULL, 0x428},//CRGPERIPH_PERCLKEN7},
	{"SCTRL", (void *)NULL, 0x198},//SCTRL_SCPERCLKEN2},
	{"SCTRL", (void *)NULL, 0x17C},//SCTRL_SCPERSTAT1},

	/* Table.3 NoC Register Dump */
	{"CRGPERIPH", (void *)NULL, 0x068},//CRGPERIPH_PERRSTSTAT0},
	{"CRGPERIPH", (void *)NULL, 0x08C},//CRGPERIPH_PERRSTSTAT3},
	{"CRGPERIPH", (void *)NULL, 0x098},//CRGPERIPH_PERRSTSTAT4},
	{"SCTRL", (void *)NULL, 0x214},//SCTRL_SCPERRSTSTAT1},
};

/*
 * if noc_happend's initflow is in the hisi_modemnoc_initflow,
 * firstly save log, then system reset.
 */
const struct noc_busid_initflow hisi_filter_initflow_kirin970[] = {
	/* Bus ID, init_flow, coreid*/
	{0, 8, RDR_CP},	/*ipf*/
	{0, 19, RDR_CP},	/*socp*/
	{0,5,RDR_IOM3},
	{0,6,RDR_IOM3},
	{0, 0, RDR_HIFI},
	{ARRAY_END_FLAG, 0, RDR_AP},	/*end*/
};

const struct noc_bus_info noc_buses_info_kirin970[] = {
	[0] = {
		.name = "cfg_sys_noc_bus",
		.initflow_mask = ((0x1f) << 18),
		.targetflow_mask = ((0x3f) << 12),
		.targ_subrange_mask = ((0x7) << 9),
		.seq_id_mask = (0x1FF),
		.opc_mask = ((0xf) << 1),
		.err_code_mask = ((0x7) << 8),
		.opc_array = opc_array,
		.opc_array_size = OPC_NR,
		.err_code_array = err_code_array,
		.err_code_array_size = ERR_CODE_NR,
		.initflow_array = cfg_initflow_array,
		.initflow_array_size = CFG_INITFLOW_ARRAY_SIZE,
		.targetflow_array = cfg_targetflow_array,
		.targetflow_array_size = CFG_TARGETFLOW_ARRAY_SIZE,
		.routeid_tbl = cfgsys_routeid_addr_tbl,
		.routeid_tbl_size = ARRAY_SIZE_NOC(cfgsys_routeid_addr_tbl),
		.p_noc_mid_info = noc_mid_kirin970,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_kirin970),
		.p_noc_sec_info = noc_sec_kirin970,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_kirin970),
	},
	[1] = {
		.name = "vcodec_bus",
		.initflow_mask = ((0x3) << 13),
		.targetflow_mask = ((0xf) << 9),
		.targ_subrange_mask = ((0x1) << 8),
		.seq_id_mask = (0xFF),
		.opc_mask = ((0xf) << 1),
		.err_code_mask = ((0x7) << 8),
		.opc_array = opc_array,
		.opc_array_size = OPC_NR,
		.err_code_array = err_code_array,
		.err_code_array_size = ERR_CODE_NR,
		.initflow_array = vcodec_initflow_array,
		.initflow_array_size = VCODEC_INITFLOW_ARRAY_SIZE,
		.targetflow_array = vcodec_targetflow_array,
		.targetflow_array_size = VCODEC_TARGETFLOW_ARRAY_SIZE,
		.routeid_tbl = vcodec_routeid_addr_tbl,
		.routeid_tbl_size = ARRAY_SIZE_NOC(vcodec_routeid_addr_tbl),
		.p_noc_mid_info = noc_mid_kirin970,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_kirin970),
		.p_noc_sec_info = noc_sec_kirin970,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_kirin970),
	},
	[2] = {
		.name = "vivo_bus",
		.initflow_mask = ((0x1f) << 14),
		.targetflow_mask = ((0xf) << 10),
		.targ_subrange_mask = ((0x3) << 8),
		.seq_id_mask = (0xFF),
		.opc_mask = ((0xf) << 1),
		.err_code_mask = ((0x7) << 8),
		.opc_array = opc_array,
		.opc_array_size = OPC_NR,
		.err_code_array = err_code_array,
		.err_code_array_size = ERR_CODE_NR,
		.initflow_array = vivo_initflow_array,
		.initflow_array_size = VIVO_INITFLOW_ARRAY_SIZE,
		.targetflow_array = vivo_targetflow_array,
		.targetflow_array_size = VIVO_TARGETFLOW_ARRAY_SIZE,
		.routeid_tbl = vivo_routeid_addr_tbl,
		.routeid_tbl_size = ARRAY_SIZE_NOC(vivo_routeid_addr_tbl),
		.p_noc_mid_info = noc_mid_kirin970,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_kirin970),
		.p_noc_sec_info = noc_sec_kirin970,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_kirin970),
	}
};

/* hisi_noc_get_array_size - get static array size
 * @bus_info_size : bus info array size
 * @dump_list_size: dump list array size
 */
void hisi_noc_get_array_size_kirin970(unsigned int *bus_info_size, unsigned int *dump_list_size)
{
	if ((NULL == bus_info_size)||(NULL == dump_list_size))
		return;

	*bus_info_size  = ARRAY_SIZE_NOC(noc_buses_info_kirin970);
	*dump_list_size = ARRAY_SIZE_NOC(noc_dump_reg_list_kirin970);
}

/*
 * hisi_noc_clock_enable - check noc clock state : on or off
 * @noc_dev : hisi noc device poiter
 * @node: hisi noc node poiter
 *
 * If clock enable, return 1, else return 0;
 */
unsigned int hisi_noc_clock_enable_kirin970(struct hisi_noc_device *noc_dev,
				     struct noc_node *node)
{
	void __iomem *reg_base = NULL;
	unsigned int reg_value;
	unsigned int i;
	unsigned int ret = 1;

	if ((NULL == noc_dev)||(NULL == node))
		return 0;

	if (noc_dev->pcrgctrl_base != NULL)
		reg_base = noc_dev->pcrgctrl_base;
	else {
		pr_err("%s: bus id and clock domain error!\n", __func__);
		return 0;
	}

	for (i = 0; i < HISI_NOC_CLOCK_MAX; i++) {
		if (0xFFFFFFFF == node->crg_clk[i].offset)
			continue;

		reg_value = readl_relaxed((u8 __iomem *)reg_base + node->crg_clk[i].offset);
		/* Clock is enabled */
		if (reg_value & (1U << node->crg_clk[i].mask_bit))
			continue;
		else {
			ret = 0;
			break;
		}
	}

	if (noc_dev->noc_property->noc_debug)
		pr_err("%s: clock_reg = 0x%pK\n", __func__, reg_base);

	return ret;
}
