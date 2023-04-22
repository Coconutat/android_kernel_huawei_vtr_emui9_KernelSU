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

#include <hisi_noc.h>
#include <hisi_noc_info.h>

#define ERR_CODE_NR  8
#define OPC_NR      10

#define NOC_CFG_SYS_BUS_ID              0
#define NOC_VCODEC_BUS_ID               1
#define NOC_VIVO_BUS_ID                 2
#define NOC_FCM_BUS_ID                  3
#define NOC_NPU_BUS_ID                  4


#define CFG_INITFLOW_ARRAY_SIZE         32
#define CFG_TARGETFLOW_ARRAY_SIZE       64
#define VCODEC_INITFLOW_ARRAY_SIZE      8
#define VCODEC_TARGETFLOW_ARRAY_SIZE    16
#define VIVO_INITFLOW_ARRAY_SIZE        32
#define VIVO_TARGETFLOW_ARRAY_SIZE      16
#define FCM_INITFLOW_ARRAY_SIZE        	2
#define FCM_TARGETFLOW_ARRAY_SIZE     	4
#define NPU_INITFLOW_ARRAY_SIZE        	16
#define NPU_TARGETFLOW_ARRAY_SIZE      	16

static char *cfg_initflow_array[] = {
	"Audio(noc_asp_mst)",                 /*0 */
	"ACPU(noc_cpu2sysbus)",               /*1 */
	"DJTAG(noc_djtag_mst)",               /*2 */
	"DMAC(noc_dmac_mst_rd)",              /*3 */
	"DMAC(noc_dmac_mst_wr)",              /*4 */
	"EMMC(noc_emmc_mst)",                 /*5 */
	"IOMCU(noc_iomcu_ahb_mst)",           /*6 */
	"IOMCU-DMA(noc_iomcu_dma_mst)",       /*7 */
	"TZP(noc_iomcu_tzp_ahb_mst)",         /*8 */
	"IPF(noc_ipf)",                       /*9 */
	"LPM3(noc_lpmcu_mst)",                /*A */
	"MEDIA-CFG(noc_media2cfg)",           /*B */
	"MODEM(noc_modem_mst)",               /*C */
	"NPU2CFG(noc_npu2cfg_mst)",           /*D */
	"PERF_STAT_DEBUG(noc_perf_stat)",     /*E */
	"SD3(noc_sd3)",                       /*F */
	"SDIO(noc_sdio)",                     /*10 */
	"SECURITY-P(noc_sec)",                /*11*/
	"SOCP_DEBUG(noc_socp)",               /*12*/
	"CORESIGHT(noc_top_cssys)",           /*13*/
	"UFS(noc_ufs_mst_rd)",                /*14*/
	"UFS(noc_ufs_mst_wr)",                /*15*/
	"USB3(noc_usb3otg)",                  /*16*/
	"VCODEC(noc_vcodec2cfg)",             /*17*/

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
	"CFG-MEDIA(noc_cfg2media)",                   /*9 */
	"CFG-VCODEC(noc_cfg2vcodec)",                 /*A */
	"DMA controller(noc_dmac_cfg)",               /*B */
	"EMMC(noc_emmc_ahb_cfg)",                     /*C */
	"GPU(noc_gpu_cfg)",                           /*D */
	"HISEE(noc_hisee_cfg)",                       /*E */
	"SSI(noc_hkadc_ssi)",                         /*F */
	"IOMCU(noc_iomcu_ahb_slv)",                   /*10*/
	"IOMCU(noc_iomcu_apb_slv)",                   /*11*/
	"LPM3(noc_lpmcu_slv)",                        /*12*/
	"MMC0BUS(noc_mmc0bus_apb_cfg)",               /*13*/
	"MMC1BUS(noc_mmc1bus_apb_cfg)",               /*14*/
	"MODEM(noc_modem_cfg)",                       /*15*/
	"NPU(noc_npu_cfg)",                           /*16*/
	"DDRC(noc_sys2ddrc)",                         /*17*/
	"CFG(noc_sys_peri0_cfg)",                     /*18*/
	"CFG(noc_sys_peri1_cfg)",                     /*19*/
	"DMA(noc_sys_peri2_cfg)",                     /*1A*/
	"DMA(noc_sys_peri3_cfg)",                     /*1B*/
	"CORESIGHT(noc_top_cssys_slv)",               /*1C*/
	"UFS-CFG(noc_ufs_cfg)",                       /*1D*/
	"USB3(noc_usb3otg_cfg)",                      /*1E*/
	"Service_target(sysbus_service_target)",      /*1F*/
	"RESERVED",                                   /*20*/
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
};

static char *vcodec_initflow_array[] = {
	"VCODEC(noc_vcodec_cfg)",
	"IVP(noc_ivp32_mst)",
	"IVP(noc_ivp_core_rd)",
	"IVP(noc_ivp_core_wr)",
	"IVP(noc_ivp_idma_rd)",
	"IVP(noc_ivp_idma_wr)",
	"VDEC(noc_vdec)",
	"VENC(noc_venc)",
};

static char *vcodec_targetflow_array[] = {
	"IVP(ivp_service_target)",
	"IVP(noc_ivp32_cfg)",
	"CRG-CFG(noc_vcodec2cfg_cfg)",
	"CRG-CFG(noc_vcodec_crg_cfg)",
	"DDRC0(noc_vcodecbus_ddrc0)",
	"VDEC(noc_vdec_cfg)",
	"VENC(noc_venc_cfg)",
	"service_target(vcdoecbus_service_target)",
	"service_target(vdec_service_target)",
	"service_target(venc_service_target)",
};

static char *vivo_initflow_array[] = {
	"CFG-MEDIA(noc_cfg2media)",
	"DSS0-RD(noc_dss0_rd)",
	"DSS1-RD(noc_dss1_rd)",
	"DSS1-WR(noc_dss1_wr)",
	"ISP1-RD(noc_isp1_rd)",
	"ISP1-WR(noc_isp1_wr)",
	"ISP-CPU-PER(noc_isp_cpu_per)",
	"ISP_CPU-RD(noc_isp_cpu_rd)",
	"ISP-CPU-WR(noc_isp_cpu_wr)",
	"ISP-RD(noc_isp_rd)",
	"ISP-WR(noc_isp_wr)",
	"JPEG-FD-RD(noc_jpeg_fd_rd)",
	"JPEG-FD-WR(noc_jpeg_fd_wr)",
};

static char *vivo_targetflow_array[] = {
	"service_target(dss_service_target)",
	"service_target(isp_service_target)",
	"service_target(noc_dss_cfg)",
	"ISP_CFG(noc_isp_cfg)",
	"JPEG-FD-CFG(noc_jpeg_fd_cfg)",
	"MEDIA-CFG(noc_media2cfg)",
	"MEDIA-CRG(noc_media_crg_cfg)",
	"DDRC0_RD(noc_vivobus_ddrc0_rd)",
	"DDRC0_WR(noc_vivobus_ddrc0_wr)",
	"DDRC1_RD(noc_vivobus_ddrc1_rd)",
	"DDRC1_WR(noc_vivobus_ddrc1_wr)",
	"service_target(vivo_service_target)",
};

/* fcm_bus */
static char *fcm_initflow_array[] = {
	"APB(noc_fcm_debugAPB_mst)",
	"PP(noc_fcm_pp_mst)",
};

static char *fcm_targetflow_array[] = {
	"FCM2GIC(noc_fcm2gic_cfg)",
	"FCM2SYS(noc_fcm2sys_cfg)",
	"FCM2SYS(noc_fcm2sys_service_cfg)",
	"RESERVED",
};

/* npu_bus */
static char *npu_initflow_array[] = {
	"AICORE(noc_aicore_tbu_rd)",
	"AICORE(noc_aicore_tbu_wr)",
	"NPUCPU(noc_npu_cpu_m0_rd)",
	"NPUCPU(noc_npu_cpu_m0_wr)",
	"NPUCPU(noc_npu_cpu_m1_rd)",
	"NPUCPU(noc_npu_cpu_m1_wr)",
	"SMMU(noc_smmu_tcu_rd)",
	"SMMU(noc_smmu_tcu_wr)",
	"SYS2NPU(noc_sys2npubus_cfg_rd)",
	"SYS2NPU(noc_sys2npubus_cfg_wr)",
	"SYSDMA(noc_sysdma_tbu_rd)",
	"SYSDMA(noc_sysdma_tbu_wr)",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
};

static char *npu_targetflow_array[] = {
	"AICORE_CFG(noc_aicore_cfg)",
	"L2BUFFER(noc_l2buffer_slv0_rd)",
	"L2BUFFER(noc_l2buffer_slv0_wr)",
	"L2BUFFER(noc_l2buffer_slv1_rd)",
	"L2BUFFER(noc_l2buffer_slv1_wr)",
	"NPU2_CFG(noc_npu2cfgbus)",
	"NPUCPU_CFG(noc_npu_cpu_cfg)",
	"NPU_CFG(noc_npubus_cfg)",
	"DDRC(noc_npubus_ddrc0_rd)",
	"DDRC(noc_npubus_ddrc0_wr)",
	"NPUGIC(noc_npugic_cfg)",
	"Service_target(npubus_service_target)",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
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
	"WRC: EXCLUSIVE WRITE",
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
	{0x01, 0x08, 0x0, 0xe7f00000},/*noc_asp_cfg*/
	{0x01, 0x08, 0x1, 0xe8000000},/*noc_asp_cfg*/
	{0x02, 0x09, 0x0, 0xe8300000},/*noc_cfg2media*/
	{0x02, 0x09, 0x1, 0xe8400000},/*noc_cfg2media*/
	{0x02, 0x0A, 0x0, 0xe9000000},/*noc_cfg2vcodec*/
	{0x02, 0x0B, 0x0, 0xfdf30000},/*noc_dmac_cfg*/
	{0x02, 0x0C, 0x0, 0xf4000000},/*noc_emmc_ahb_cfg*/
	{0x02, 0x0D, 0x0, 0xe82c0000},/*noc_gpu_cfg*/
	{0x02, 0x0E, 0x0, 0xf0e00000},/*noc_hisee_cfg*/
	{0x02, 0x0F, 0x0, 0xe82b8000},/*noc_hkadc_ssi*/
	{0x02, 0x10, 0x0, 0xffb00000},/*noc_iomcu_ahb_slv*/
	{0x02, 0x10, 0x1, 0xffc00000},/*noc_iomcu_ahb_slv*/
	{0x02, 0x11, 0x0, 0xffd00000},/*noc_iomcu_apb_slv*/
	{0x02, 0x12, 0x0, 0xea980000},/*noc_lpmcu_slv*/
	{0x02, 0x12, 0x1, 0xea900000},/*noc_lpmcu_slv*/
	{0x02, 0x12, 0x2, 0xffe00000},/*noc_lpmcu_slv*/
	{0x02, 0x12, 0x3, 0xec000000},/*noc_lpmcu_slv*/
	{0x02, 0x13, 0x0, 0xff340000},/*noc_mmc0bus_apb_cfg*/
	{0x02, 0x14, 0x0, 0xfc000000},/*noc_mmc1bus_apb_cfg*/
	{0x02, 0x15, 0x0, 0xe0000000},/*noc_modem_cfg*/
	{0x02, 0x16, 0x0, 0xe4000000},/*noc_npu_cfg*/
	{0x00, 0x17, 0x0, 0xc0000000},/*noc_sys2ddrc*/
	{0x02, 0x17, 0x1, 0x80000000},/*noc_sys2ddrc*/
	{0x02, 0x17, 0x2, 0x0},/*noc_sys2ddrc*/
	{0x00, 0x17, 0x3, 0x100000000},/*noc_sys2ddrc*/
	{0x00, 0x17, 0x4, 0x200000000},/*noc_sys2ddrc*/
	{0x02, 0x18, 0x0, 0xe8a00000},/*noc_sys_peri0_cfg*/
	{0x02, 0x19, 0x0, 0xe8960000},/*noc_sys_peri1_cfg*/
	{0x02, 0x19, 0x1, 0xe8980000},/*noc_sys_peri1_cfg*/
	{0x02, 0x1A, 0x0, 0xff000000},/*noc_sys_peri2_cfg*/
	{0x02, 0x1B, 0x0, 0xfdf00000},/*noc_sys_peri3_cfg*/
	{0x02, 0x1C, 0x0, 0xfe000000},/*noc_top_cssys_slv*/
	{0x02, 0x1D, 0x0, 0xff3e0000},/*noc_ufs_cfg*/
	{0x02, 0x1D, 0x1, 0xff3c0000},/*noc_ufs_cfg*/
	{0x02, 0x1E, 0x0, 0xff200000},/*noc_usb3otg_cfg*/
	{0x02, 0x1E, 0x1, 0xff100000},/*noc_usb3otg_cfg*/
	{0x02, 0x1F, 0x0, 0xe9840000},/*sysbus_service_target*/
	{0x02, 0x1F, 0x1, 0x0},/*sysbus_service_target*/
};

/* vcodec_bus */
static const ROUTE_ID_ADDR_STRU vcodec_routeid_addr_tbl[] = {
	/* Init_flow  Targ_flow  Targ_subrange  Init_localAddress*/
	/* ---------------------------------------------------*/
	{0x00, 0x00, 0x0, 0xe9380000},/*ivp_service_target*/
	{0x00, 0x01, 0x0, 0xe9000000},/*noc_ivp32_cfg*/
	{0x01, 0x02, 0x0, 0x0},/*noc_vcodec2cfg_cfg*/
	{0x00, 0x03, 0x0, 0xe9300000},/*noc_vcodec_crg_cfg*/
	{0x00, 0x04, 0x0, 0xe8880000},/*noc_vcodecbus_ddrc0*/
	{0x00, 0x05, 0x0, 0xe9200000},/*noc_vdec_cfg*/
	{0x00, 0x06, 0x0, 0xe9280000},/*noc_venc_cfg*/
	{0x00, 0x07, 0x0, 0xe9390000},/*vcdoecbus_service_target*/
	{0x00, 0x07, 0x1, 0x0},/*vcdoecbus_service_target*/
	{0x00, 0x08, 0x0, 0xe93a0000},/*vdec_service_target*/
	{0x00, 0x09, 0x1, 0xe93b0000},/*venc_service_target*/
};

/* vivo_bus */
static const ROUTE_ID_ADDR_STRU vivo_routeid_addr_tbl[] = {
	/* Init_flow  Targ_flow  Targ_subrange Init_localAddress */
	/* ----------------------------------------------------- */
	{0x00, 0x00, 0x0, 0xe87a0000},/*dss_service_target*/
	{0x00, 0x01, 0x0, 0xe87b0000},/*isp_service_target*/
	{0x00, 0x02, 0x0, 0xe8600000},/*noc_dss_cfg*/
	{0x00, 0x03, 0x0, 0xe8400000},/*noc_isp_cfg*/
	{0x00, 0x04, 0x0, 0xe8300000},/*noc_jpeg_fd_cfg*/
	{0x06, 0x05, 0x0, 0x0},/*noc_media2cfg*/
	{0x00, 0x06, 0x0, 0xe87fe000},/*noc_media_crg_cfg*/
	{0x01, 0x07, 0x0, 0x0},/*noc_vivobus_ddrc0_rd*/
	{0x01, 0x07, 0x1, 0x0},/*noc_vivobus_ddrc0_rd*/
	{0x01, 0x07, 0x2, 0x0},/*noc_vivobus_ddrc0_rd*/
	{0x01, 0x07, 0x3, 0x0},/*noc_vivobus_ddrc0_rd*/
	{0x01, 0x07, 0x4, 0x0},/*noc_vivobus_ddrc0_rd*/
	{0x01, 0x07, 0x5, 0x0},/*noc_vivobus_ddrc0_rd*/
	{0x01, 0x07, 0x6, 0x0},/*noc_vivobus_ddrc0_rd*/
	{0x01, 0x07, 0x7, 0x0},/*noc_vivobus_ddrc0_rd*/
	{0x01, 0x08, 0x0, 0x0},/*noc_vivobus_ddrc0_wr*/
	{0x01, 0x08, 0x1, 0x0},/*noc_vivobus_ddrc0_wr*/
	{0x01, 0x08, 0x2, 0x0},/*noc_vivobus_ddrc0_wr*/
	{0x01, 0x08, 0x3, 0x0},/*noc_vivobus_ddrc0_wr*/
	{0x01, 0x08, 0x4, 0x0},/*noc_vivobus_ddrc0_wr*/
	{0x01, 0x08, 0x5, 0x0},/*noc_vivobus_ddrc0_wr*/
	{0x01, 0x08, 0x6, 0x0},/*noc_vivobus_ddrc0_wr*/
	{0x01, 0x08, 0x7, 0x0},/*noc_vivobus_ddrc0_wr*/
	{0x01, 0x09, 0x0, 0x80},/*noc_vivobus_ddrc1_rd*/
	{0x01, 0x09, 0x1, 0x100},/*noc_vivobus_ddrc1_rd*/
	{0x01, 0x09, 0x2, 0x200},/*noc_vivobus_ddrc1_rd*/
	{0x01, 0x09, 0x3, 0x400},/*noc_vivobus_ddrc1_rd*/
	{0x01, 0x0A, 0x0, 0x80},/*noc_vivobus_ddrc1_wr*/
	{0x01, 0x0A, 0x1, 0x100},/*noc_vivobus_ddrc1_wr*/
	{0x01, 0x0A, 0x2, 0x200},/*noc_vivobus_ddrc1_wr*/
	{0x01, 0x0A, 0x3, 0x400},/*noc_vivobus_ddrc1_wr*/
	{0x00, 0x0B, 0x0, 0xe87d0000},/*vivo_service_target*/
	{0x00, 0x0B, 0x1, 0x0},/*vivo_service_target*/

};

/* fcm_bus */
static const ROUTE_ID_ADDR_STRU fcm_routeid_addr_tbl[] = {
	/* Init_flow  Targ_flow  Targ_subrange Init_localAddress */
	/* ----------------------------------------------------- */
	{0x00, 0x00, 0x0, 0xea000000},/*noc_fcm2gic_cfg*/
	{0x01, 0x01, 0x0, 0xe0000000},/*noc_fcm2sys_cfg*/
	{0x00, 0x02, 0x0, 0xea200000},/*noc_fcm2sys_service_cfg*/
	{0x00, 0x02, 0x1, 0x0},/*noc_fcm2sys_service_cfg*/
};

/* npu_bus */
static const ROUTE_ID_ADDR_STRU npu_routeid_addr_tbl[] = {
	/* Init_flow  Targ_flow  Targ_subrange Init_localAddress */
	/* ----------------------------------------------------- */
	{0x00, 0x01, 0x0, 0xe4800000},/*noc_l2buffer_slv0_rd*/
	{0x00, 0x01, 0x1, 0xe4800000},/*noc_l2buffer_slv0_rd*/
	{0x00, 0x01, 0x2, 0xe4800000},/*noc_l2buffer_slv0_rd*/
	{0x00, 0x01, 0x3, 0xe4800000},/*noc_l2buffer_slv0_rd*/
	{0x00, 0x02, 0x0, 0xe4800000},/*noc_l2buffer_slv0_wr*/
	{0x00, 0x02, 0x1, 0xe4800000},/*noc_l2buffer_slv0_wr*/
	{0x00, 0x02, 0x2, 0xe4800000},/*noc_l2buffer_slv0_wr*/
	{0x00, 0x02, 0x3, 0xe4800000},/*noc_l2buffer_slv0_wr*/
	{0x00, 0x03, 0x0, 0xe4800080},/*noc_l2buffer_slv1_rd*/
	{0x00, 0x03, 0x1, 0xe4800100},/*noc_l2buffer_slv1_rd*/
	{0x00, 0x03, 0x2, 0xe4800200},/*noc_l2buffer_slv1_rd*/
	{0x00, 0x03, 0x3, 0xe4800400},/*noc_l2buffer_slv1_rd*/
	{0x00, 0x04, 0x0, 0xe4800080},/*noc_l2buffer_slv1_wr*/
	{0x00, 0x04, 0x1, 0xe4800100},/*noc_l2buffer_slv1_wr*/
	{0x00, 0x04, 0x2, 0xe4800200},/*noc_l2buffer_slv1_wr*/
	{0x00, 0x04, 0x3, 0xe4800400},/*noc_l2buffer_slv1_wr*/
	{0x00, 0x06, 0x0, 0xe4000000},/*noc_npu_cpu_cfg*/
	{0x00, 0x08, 0x0, 0x0},/*noc_npubus_ddrc0_rd*/
	{0x02, 0x00, 0x0, 0xe5000000},/*noc_aicore_cfg*/
	{0x02, 0x05, 0x0, 0xe0000000},/*noc_npu2cfgbus*/
	{0x02, 0x07, 0x0, 0xe5e00000},/*noc_npubus_cfg*/
	{0x02, 0x0A, 0x0, 0xe5c00000},/*noc_npugic_cfg*/
	{0x02, 0x0B, 0x0, 0xe4d30000},/*npubus_service_target*/
	{0x02, 0x0B, 0x1, 0x0},/*npubus_service_target*/
	{0x03, 0x09, 0x0, 0x0},/*noc_npubus_ddrc0_wr*/
};

struct noc_mid_info noc_mid_ORLA[] = {
	/*Bus ID,     init_flow       ,mask   ,mid_va,        mid name */
	{0, 0x01, 0x3800, 0x0, "AP_CPU0"}, /*noc_cci2sysbus,*/
	{0, 0x01, 0x3800, 0x800, "AP_CPU1"},   /*noc_cci2sysbus,*/
	{0, 0x01, 0x3800, 0x1000, "AP_CPU2"},  /*noc_cci2sysbus,*/
	{0, 0x01, 0x3800, 0x1800, "AP_CPU3"},  /*noc_cci2sysbus,*/
	{0, 0x01, 0x3800, 0x2000, "AP_CPU4"},  /*noc_cci2sysbus,*/
	{0, 0x01, 0x3800, 0x2800, "AP_CPU5"},  /*noc_cci2sysbus,*/
	{0, 0x01, 0x3800, 0x3000, "AP_CPU6"},  /*noc_cci2sysbus,*/
	{0, 0x01, 0x3800, 0x3800, "AP_CPU7"},  /*noc_cci2sysbus,*/
	{0, 0x0A, 0x003f, 0x00, "LPMCU"},
	{0, 0x06, 0x003f, 0x01, "IOMCU_M7"},/*noc_iomcu_ahb_mst*/
	{0, 0x0E, 0x003f, 0x03, "PERF_STAT"},
	{0, 0x09, 0x003f, 0x04, "IPF"},
	{0, 0x02, 0x003f, 0x05, "DJTAG_M"},
	{0, 0x07, 0x003f, 0x06, "IOMCU_DMA"},/*noc_iomcu_dma_mst*/
	{0, 0x07, 0x003f, 0x01, "IOMCU_M7"}, /*noc_iomcu_dma_mst*/
	{0, 0x14, 0x003f, 0x07, "UFS"},
	{0, 0x0F, 0x003f, 0x08, "SD"},
	{0, 0x10, 0x003f, 0x09, "SDIO"},
	{0, 0x11, 0x003f, 0x0A, "SEC_CC712"},
	{0, 0x12, 0x003f, 0x0D, "SOCP"},
	{0, 0x15, 0x003f, 0x0E, "USB20OTG"},
	{0, 0x13, 0x003f, 0x0F, "TOP_CSSYS"},
	{0, 0x03, 0x003f, 0x10, "DMAC"},
	{0, 0x00, 0x003f, 0x11, "ASP"},
	{0, 0x05, 0x003f, 0x12, "EMMC"},
	{0, 0xFF, 0x003f, 0x13, "ATGC"},
	{0, 0x00, 0x003f, 0x14, "DMA"},
	{0, 0x0C, 0x003f, 0x24, "DFC"},  /*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x16, "CIPHER"},  /*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x36, "HDLC"},  /*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x17, "CICOM0"},  /*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x37, "CICOM1"},  /*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x18, "NXDSP"},  /*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x19, "TL_BBP_DMA_TCM"},/*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x39, "TL_BBP_DMA_DDR"},/*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x1A, "GU_BBP_MST_TCM"},    /*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x3A, "GU_BBP_MST_DDR"},    /*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x1B, "EDMA0"},  /*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x3B, "EDMA1"},  /*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x1C, "HARQ_L"},    /*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x3C, "HARQ_H"},    /*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x1D, "UPACC"}, /*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x3D, "RSR_ACC"}, /*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x1E, "CIPHER_WRITE_THOUGH"},   /*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x3F, "CCPU_CFG"},   /*noc_modem_mst,*/
	{0, 0x0C, 0x003f, 0x1F, "CCPU_L2C"},   /*noc_modem_mst,*/
	{2, 0xFF, 0x003f, 0x00, "ISP_1"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x01, "ISP_1"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x02, "ISP_1"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x03, "ISP_1"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x04, "ISP_1"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x05, "ISP_1"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x06, "ISP_1"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x07, "ISP_1"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x08, "ISP_2"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x09, "ISP_2"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x0A, "ISP_2"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x0B, "ISP_2"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x0C, "ISP_2"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x0D, "ISP_2"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x0E, "ISP_2"},/*ISP_CORE*/
	{2, 0xFF, 0x003f, 0x0F, "ISP_JPEG"},/*ISP_JPEG*/
	{2, 0xFF, 0x003f, 0x10, "cmdlist"},/*dss*/
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
	{2, 0xFF, 0x003f, 0x1C, "ISP_R8_OR_ISP_ATGM"},/*ISP*/
	{2, 0xFF, 0x003f, 0x1D, "IPP_SUBSYS_JPGENC"},/*TPEG_FD_SUBSYS*/
	{1, 0x04, 0x003f, 0x20, "VENC1"},/*VENC1*/
	{1, 0x04, 0x003f, 0x21, "VENC2"},/*VENC2*/
	{1, 0x03, 0x003f, 0x22, "VDEC_OR_VDEC_ATGM"},/*VDEC1*/
	{1, 0x03, 0x003f, 0x23, "VDEC2"},/*VDEC2*/
	{1, 0x03, 0x003f, 0x24, "VDEC3"},/*VDEC3*/
	{1, 0x03, 0x003f, 0x25, "VDEC4"},/*VDEC4*/
	{1, 0x03, 0x003f, 0x26, "VDEC5"},/*VDEC5*/
	{1, 0x03, 0x003f, 0x27, "VDEC6"},/*VDEC6*/
	{2, 0x0C, 0x003f, 0x29, "DSP_CORE_OR_IVP_ATGM"},/*IVP32_DSP*/
	{2, 0x0C, 0x003f, 0x2A, "IVP32_DSP DSP_DMA"},/*IVP32_DSP*/
	{2, 0xFF, 0x003f, 0x2B, "ISP_A7_CFG"},/*ISP*/
	{2, 0xFF, 0x003f, 0x2C, "CMDLIST"},/*IPP_SUBSYS*/
	{2, 0xFF, 0x003f, 0x2D, "ORB"},/*IPP_SUBSYS*/
	{4, 0xFF, 0x003f, 0x30, "NPU_0"},/*NPU*/
	{4, 0xFF, 0x003f, 0x31, "NPU_1"},/*NPU*/
	{4, 0xFF, 0x003f, 0x32, "NPU_2"},/*NPU*/
	{4, 0xFF, 0x003f, 0x33, "NPU_3"},/*NPU*/
	{1, 0x01, 0x003f, 0x34, "DSP_CORE_DATA"},/*DSP_CORE_DATA*/
	{3, 0xFF, 0x003f, 0x38, "FCM_M0"},/*DSP_CORE_DATA*/
	{3, 0xFF, 0x003f, 0x39, "FCM_M1"},/*DSP_CORE_DATA*/
};

struct noc_sec_info noc_sec_ORLA[] = {
	/*mask value info  sec_mode*/
	{0x03, 0x00, "trusted", "TZMP2 NSAID"},/*TZMP2 NSAID*/
	{0x03, 0x01, "non-trusted", "TZMP2 NSAID"},/*TZMP2 NSAID*/
	{0x03, 0x02, "protected", "TZMP2 NSAID"},/*TZMP2 NSAID*/
	{0x03, 0x03, "ACPU", "TZMP2 NSAID"},/*TZMP2 NSAID*/
	{0x04, 0x00, "secure", "secure"},/*secure*/
	{0x04, 0x04, "non-secure", "secure"},/*secure*/
};

struct noc_dump_reg noc_dump_reg_list_ORLA[] = {
	/* Table.1 NoC Register Dump: control and state */
	{"PCTRL", (void *)NULL, 0x09C},//PCTRL_PERI_STAT2},
	{"PCTRL", (void *)NULL, 0x0A0},//PCTRL_PERI_STAT3},
	{"SCTRL", (void *)NULL, 0x37C},//SCTRL_SCPERSTATUS7},
	{"PCTRL", (void *)NULL, 0x094},//PCTRL_PERI_STAT0},
	{"SCTRL", (void *)NULL, 0x380},//SCTRL_SCPERSTATUS8},
	{"CRGPERIPH", (void *)NULL, 0x12C},//CRGPERIPH_PERI_CTRL3},
	{"PMCTRL", (void *)NULL, 0x3A0},//PMCTRL_PERI_INT0_MASK},
	{"PMCTRL", (void *)NULL, 0x3A8},//PMCTRL_PERI_INT1_MASK},
	{"PMCTRL", (void *)NULL, 0x3A4},//PMCTRL_PERI_INT0_MASK},
	{"PMCTRL", (void *)NULL, 0x3AC},//PMCTRL_PERI_INT1_MASK},
	{"SCTRL", (void *)NULL, 0x314},//SCTRL_SCPERCTRL5},
	{"SCTRL", (void *)NULL, 0x3D0},//SCTRL_SC_SECOND_INT_MASK},
	{"SCTRL", (void *)NULL, 0x3D8},//SCTRL_SC_SECOND_INT_OUT},
	{"SCTRL", (void *)NULL, 0x380},//SCTRL_SCPERSTATUS8},
	{"PMCTRL", (void *)NULL, 0x380},//PMCTRL_NOC_POWER_IDLEREQ},
	{"PMCTRL", (void *)NULL, 0x384},//PMCTRL_NOC_POWER_IDLEACK},
	{"PMCTRL", (void *)NULL, 0x388},//PMCTRL_NOC_POWER_IDLE},
	{"PMCTRL", (void *)NULL, 0x38C},//PMCTRL_NOC_POWER_IDLEREQ1},
	{"PMCTRL", (void *)NULL, 0x390},//PMCTRL_NOC_POWER_IDLEACK1},
	{"PMCTRL", (void *)NULL, 0x394},//PMCTRL_NOC_POWER_IDLE1},
	{"SCTRL", (void *)NULL, 0x31C},//SCTRL_SCPERCTRL7},
	{"SCTRL", (void *)NULL, 0x37C},//SCTRL_SCPERSTATUS7},

	/* Table.2 NoC Register Dump: CLK state*/
	{"CRGPERIPH", (void *)NULL, 0x008},//CRGPERIPH_PERCLKEN0},
	{"CRGPERIPH", (void *)NULL, 0x00C},//CRGPERIPH_PERCLKSTAT0},
	{"CRGPERIPH", (void *)NULL, 0x028},//CRGPERIPH_PERCLKEN2},
	{"CRGPERIPH", (void *)NULL, 0x038},//CRGPERIPH_PERCLKEN3},
	{"CRGPERIPH", (void *)NULL, 0x03C},//CRGPERIPH_PERCLKSTAT3},
	{"CRGPERIPH", (void *)NULL, 0x048},//CRGPERIPH_PERCLKEN4},
	{"CRGPERIPH", (void *)NULL, 0x058},//CRGPERIPH_PERCLKEN5},
	{"CRGPERIPH", (void *)NULL, 0x05C},//CRGPERIPH_PERCLKSTAT5},
	{"CRGPERIPH", (void *)NULL, 0x418},//CRGPERIPH_PERCLKEN6},
	{"CRGPERIPH", (void *)NULL, 0x41C},//CRGPERIPH_PERCLKSTAT6},
	{"CRGPERIPH", (void *)NULL, 0x428},//CRGPERIPH_PERCLKEN7},
	{"CRGPERIPH", (void *)NULL, 0x42C},//CRGPERIPH_PERCLKSTAT7},
	{"CRGPERIPH", (void *)NULL, 0x438},//CRGPERIPH_PERCLKEN8},
	{"CRGPERIPH", (void *)NULL, 0x43C},//CRGPERIPH_PERCLKSTAT8},
	{"CRGPERIPH", (void *)NULL, 0x448},//CRGPERIPH_PERCLKEN9},
	{"CRGPERIPH", (void *)NULL, 0x44C},//CRGPERIPH_PERCLKSTAT9},
	{"CRGPERIPH", (void *)NULL, 0x104},//CRGPERIPH_CLKDIV23},
	{"CRGPERIPH", (void *)NULL, 0x504},//CRGPERIPH_PERISTAT5},
	{"SCTRL", (void *)NULL, 0x178},//SCTRL_SCPERCLKEN1
	{"SCTRL", (void *)NULL, 0x17C},//SCTRL_SCPERSTAT1

	/* Table.3 NoC Register Dump: RESET state */
	{"CRGPERIPH", (void *)NULL, 0x068},//CRGPERIPH_PERRSTSTAT0},
	{"CRGPERIPH", (void *)NULL, 0x098},//CRGPERIPH_PERRSTSTAT4},
	{"CRGPERIPH", (void *)NULL, 0x0A4},//CRGPERIPH_PERRSTSTAT5},
	{"SCTRL", (void *)NULL, 0x214},//SCTRL_SCPERRSTSTAT1},
};

/*
 * if noc_happend's initflow is in the hisi_modemnoc_initflow,
 * firstly save log, then system reset.
 */
const struct noc_busid_initflow hisi_filter_initflow_ORLA[] = {
	/* Bus ID, init_flow, coreid*/
	{0, 9, RDR_CP},	/*ipf*/
	{0, 18, RDR_CP},	/*socp*/
	{0, 6, RDR_IOM3},  /* iomcu core */
	{0, 7, RDR_IOM3},  /* iomcu dma */
	{ARRAY_END_FLAG, 0, RDR_AP},	/*end*/
};

const struct noc_bus_info noc_buses_info_ORLA[] = {
	[0] = {
		.name = "cfg_sys_noc_bus",
		.initflow_mask = ((0x1f) << 17),
		.targetflow_mask = ((0x1f) << 12),
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
		.p_noc_mid_info = noc_mid_ORLA,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_ORLA),
		.p_noc_sec_info = noc_sec_ORLA,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_ORLA),
	},
	[1] = {
		.name = "vcodec_bus",
		.initflow_mask = ((0x7) << 14),
		.targetflow_mask = ((0xf) << 10),
		.targ_subrange_mask = ((0x1) << 9),
		.seq_id_mask = (0x1FF),
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
		.p_noc_mid_info = noc_mid_ORLA,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_ORLA),
		.p_noc_sec_info = noc_sec_ORLA,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_ORLA),
	},
	[2] = {
		.name = "vivo_bus",
		.initflow_mask = ((0xf) << 15),
		.targetflow_mask = ((0xf) << 11),
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
		.p_noc_mid_info = noc_mid_ORLA,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_ORLA),
		.p_noc_sec_info = noc_sec_ORLA,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_ORLA),
	},
	[3] = {
		.name = "fcm_bus",
		.initflow_mask = ((0x1) << 8),
		.targetflow_mask = ((0x3) << 6),
		.targ_subrange_mask = ((0x1) << 5),
		.seq_id_mask = (0x1F),
		.opc_mask = ((0xf) << 1),
		.err_code_mask = ((0x7) << 8),
		.opc_array = opc_array,
		.opc_array_size = OPC_NR,
		.err_code_array = err_code_array,
		.err_code_array_size = ERR_CODE_NR,
		.initflow_array = fcm_initflow_array,
		.initflow_array_size = FCM_INITFLOW_ARRAY_SIZE,
		.targetflow_array = fcm_targetflow_array,
		.targetflow_array_size = FCM_TARGETFLOW_ARRAY_SIZE,
		.routeid_tbl = fcm_routeid_addr_tbl,
		.routeid_tbl_size = ARRAY_SIZE_NOC(fcm_routeid_addr_tbl),
		.p_noc_mid_info = noc_mid_ORLA,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_ORLA),
		.p_noc_sec_info = noc_sec_ORLA,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_ORLA),
	},
	[4] = {
		.name = "npu_bus",
		.initflow_mask = ((0xf) << 17),
		.targetflow_mask = ((0xf) << 13),
		.targ_subrange_mask = ((0xf) << 9),
		.seq_id_mask = (0x1FF),
		.opc_mask = ((0xf) << 1),
		.err_code_mask = ((0x7) << 8),
		.opc_array = opc_array,
		.opc_array_size = OPC_NR,
		.err_code_array = err_code_array,
		.err_code_array_size = ERR_CODE_NR,
		.initflow_array = npu_initflow_array,
		.initflow_array_size = NPU_INITFLOW_ARRAY_SIZE,
		.targetflow_array = npu_targetflow_array,
		.targetflow_array_size = NPU_TARGETFLOW_ARRAY_SIZE,
		.routeid_tbl = npu_routeid_addr_tbl,
		.routeid_tbl_size = ARRAY_SIZE_NOC(npu_routeid_addr_tbl),
		.p_noc_mid_info = noc_mid_ORLA,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_ORLA),
		.p_noc_sec_info = noc_sec_ORLA,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_ORLA),
	},
};

/* hisi_noc_get_array_size - get static array size
 * @bus_info_size : bus info array size
 * @dump_list_size: dump list array size
 */
void hisi_noc_get_array_size_ORLA(unsigned int *bus_info_size, unsigned int *dump_list_size)
{
	if ((NULL == bus_info_size)||(NULL == dump_list_size))
		return;

	*bus_info_size  = ARRAY_SIZE_NOC(noc_buses_info_ORLA);
	*dump_list_size = ARRAY_SIZE_NOC(noc_dump_reg_list_ORLA);
}

/*
 * hisi_noc_clock_enable - check noc clock state : on or off
 * @noc_dev : hisi noc device poiter
 * @node: hisi noc node poiter
 *
 * If clock enable, return 1, else return 0;
 */
unsigned int hisi_noc_clock_enable_ORLA(struct hisi_noc_device *noc_dev,
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
