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
  head info
*******************************************************************************/
#include <linux/io.h>
#include <linux/string.h>

#include <hisi_noc.h>
#include <hisi_noc_info.h>

#define ERR_CODE_NR  8
#define OPC_NR      10

#define NOC_CFG_SYS_BUS_ID              0
#define NOC_VCODEC_BUS_ID               1
#define NOC_VIVO_BUS_ID                 2
#define NOC_NPU_BUS_ID                  3

#define CFG_INITFLOW_ARRAY_SIZE         32
#define CFG_TARGETFLOW_ARRAY_SIZE       64
#define VCODEC_INITFLOW_ARRAY_SIZE      16
#define VCODEC_TARGETFLOW_ARRAY_SIZE    16
#define VIVO_INITFLOW_ARRAY_SIZE        32
#define VIVO_TARGETFLOW_ARRAY_SIZE      16
#define NPU_INITFLOW_ARRAY_SIZE        32
#define NPU_TARGETFLOW_ARRAY_SIZE      32

static char *cfg_initflow_array[] = {
	"noc_ao_tcp_mst",		      /*0 */
	"Audio(noc_asp_mst)",		      /*1 */
	"ACPU(noc_cci2sysbus)",		      /*2 */
	"DJTAG(noc_djtag_mst)",		      /*3 */
	"DMA-Controller-rd(noc_dmac_mst_rd)", /*4 */
	"DMA-Controller-wr(noc_dmac_mst_wr)", /*5 */
	"DPCTRL(noc_dpctrl)",		      /*6 */
	"noc_fd_mst",			      /*7 */
	"IOMCU(noc_iomcu_ahb_mst)",	   /*8 */
	"IOMCU-DMA(noc_iomcu_dma_mst)",       /*9 */
	"TZP(noc_iomcu_tzp_ahb_mst)",	 /*A */
	"IPF(noc_ipf)",			      /*B */
	"noc_ipu2cfg",			      /*C */
	"noc_latency_mon",		      /*D */
	"LPM3(noc_lpmcu_mst)",		      /*E */
	"MODEM(noc_modem_mst)",		      /*F */
	"PCIE0_rd(noc_pcie0_rd)",	     /*10*/
	"PCIE0_wr(noc_pcie0_wr)",	     /*11*/
	"PCIE1_rd(noc_pcie1_rd)",	     /*12*/
	"PCIE1_wr(noc_pcie1_wr)",	     /*13*/
	"PERF_STAT_DEBUG(noc_perf_stat)",     /*14*/
	"SD3(noc_sd3)",			      /*15*/
	"SDIO(noc_sdio)",		      /*16*/
	"SECURITY-P(noc_sec)",		      /*17*/
	"SOCP_DEBUG(noc_socp)",		      /*18*/
	"CORESIGHT(noc_top_cssys)",	   /*19*/
	"UFS(noc_ufs_mst_rd)",		      /*1A*/
	"UFS(noc_ufs_mst_wr)",		      /*1B*/
	"USB3_wr(noc_usb3otg_rd)",	    /*1C*/
	"USB3_rd(noc_usb3otg_wr)",	    /*1D*/
	"noc_vcodec2cfg",		      /*1E*/
	"noc_vivo2cfg",			      /*1F*/
};

static char *cfg_targetflow_array[] = {
	"AOBUS(aobus_service_target)",		      /*0 */
	"Audio(aspbus_service_target)",		      /*1 */
	"Service_target(cfgbus_service_target)",      /*2 */
	"Service_target(dmadebugbus_service_target)", /*3 */
	"fdbus_service_target",			      /*4 */
	"hsdtbus_service_target",		      /*5 */
	"Service_target(mmc0bus_service_target)",     /*6 */
	"Service_target(modembus_service_target)",    /*7 */
	"noc_ao_tcp_cfg",			      /*8 */
	"AON(noc_aon_apb_slv)",			      /*9 */
	"Audio(noc_asp_cfg)",			      /*A */
	"noc_cfg2ipu",				      /*B */
	"noc_cfg2syscache",			      /*C*/
	"CFG-VCODEC(noc_cfg2vcodec)",		      /*D */
	"noc_cfg2vivo",				      /*E*/
	"DMA controller(noc_dmac_cfg)",		      /*F*/
	"noc_fd_cfg",				      /*10*/
	"GPU(noc_gpu_cfg)",			      /*11*/
	"HISEE(noc_hisee_cfg_0)",		      /*12*/
	"SSI(noc_hkadc_ssi_0)",			      /*13*/
	"noc_hsdtbus_apb_cfg",			      /*14*/
	"IOMCU(noc_iomcu_ahb_slv)",		      /*15*/
	"IOMCU(noc_iomcu_apb_slv)",		      /*16*/
	"LPM3(noc_lpmcu_slv)",			      /*17*/
	"noc_mad_cfg",				      /*18*/
	"MMC0BUS(noc_mmc0bus_apb_cfg)",		      /*19*/
	"MODEM(noc_modem_cfg)",			      /*1A*/
	"PCIE_0(noc_pcie0_cfg)",		      /*1B*/
	"PCIE_1(noc_pcie1_cfg)",		      /*1C*/
	"DDRC(noc_sys2ddrc)",			      /*1D*/
	"CFG(noc_sys_peri0_cfg)",		      /*1E*/
	"CFG(noc_sys_peri1_cfg)",		      /*1F*/
	"DMA(noc_sys_peri2_cfg)",		      /*20*/
	"DMA(noc_sys_peri3_cfg)",		      /*21*/
	"CORESIGHT(noc_top_cssys_slv)",		      /*22*/
	"noc_usb3otg_cfg",			      /*23*/
	"Service_target(sysbus_service_target)",      /*24*/
	"RESERVED",				      /*25*/
	"RESERVED",				      /*26*/
	"RESERVED",				      /*27*/
	"RESERVED",				      /*28*/
	"RESERVED",				      /*29*/
	"RESERVED",				      /*30*/
	"RESERVED",				      /*31*/
	"RESERVED",				      /*32*/
	"RESERVED",				      /*33*/
	"RESERVED",				      /*34*/
	"RESERVED",				      /*35*/
	"RESERVED",				      /*36*/
	"RESERVED",				      /*37*/
	"RESERVED",				      /*38*/
	"RESERVED",				      /*39*/
	"RESERVED",				      /*3A*/
	"RESERVED",				      /*3B*/
	"RESERVED",				      /*3C*/
	"RESERVED",				      /*3D*/
	"RESERVED",				      /*3E*/
	"RESERVED",				      /*3F*/
};

static char *vcodec_initflow_array[] = {
	"noc_cfg2vcodec",
	"noc_eps_ahb_mst",
	"noc_eps_axi_mst_rd",
	"noc_eps_axi_mst_wr",
	"noc_hiface_core",
	"noc_ivp32_mst",
	"noc_ivp_core_rd",
	"noc_ivp_core_wr",
	"noc_ivp_idma_rd",
	"noc_ivp_idma_wr",
	"VDEC(noc_vdec)",
	"VENC(noc_venc)",
	"noc_venc2",
	"RESERVED",
	"RESERVED",
	"RESERVED",
};

static char *vcodec_targetflow_array[] = {
	"hiface_service_target",
	"ivp_service_target",
	"noc_eps_cfg",
	"noc_hiface_core_cfg",
	"noc_ivp32_cfg",
	"CRG-CFG2(noc_vcodec2cfg_cfg)",
	"CRG-CFG1(noc_vcodec_crg_cfg)",
	"DDRC0(noc_vcodecbus_ddrc0)",
	"DDRC0(noc_vcodecbus_ddrc1)",
	"VDEC(noc_vdec_cfg)",
	"VENC2(noc_venc2_cfg)",
	"VENC(noc_venc_cfg)",
	"service_target(vcdoecbus_service_target)",
	"service_target(vdec_service_target)",
	"service_target(venc2_service_target)",
	"service_target(venc_service_target)",
};

static char *vivo_initflow_array[] = {
	"CFG-MEDIA(noc_cfg2vivo)",
	"DSS0-RD(noc_dss0_rd)",
	"DSS0-WR(noc_dss0_wr)",
	"DSS1-RD(noc_dss1_rd)",
	"DSS1-WR(noc_dss1_wr)",
	"noc_idi2axi_wr",
	"noc_ipp_rd",
	"noc_ipp_wr",
	"ISP1-RD(noc_isp1_rd)",
	"ISP1-WR(noc_isp1_wr)",
	"ISP-CPU-PER(noc_isp_cpu_per)",
	"ISP_CPU-RD(noc_isp_cpu_rd)",
	"ISP-CPU-WR(noc_isp_cpu_wr)",
	"ISP-RD(noc_isp_rd)",
	"ISP-WR(noc_isp_wr)",
	"MEDIA-COM-RD(noc_media_com_rd)",
	"MEDIA-COM-WR(noc_media_com_wr)",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
};

static char *vivo_targetflow_array[] = {
	"service_target(dss_service_target)",
	"service_target(isp_service_target)",
	"noc_dss_cfg",
	"noc_ipp_cfg(noc_jpeg_fd_cfg_0)",
	"ISP_CFG(noc_isp_cfg)",
	"noc_vivo2cfg",
	"noc_vivo_crg_cfg",
	"DDRC0_RD(noc_vivobus_ddrc0_rd)",
	"DDRC0_WR(noc_vivobus_ddrc0_wr)",
	"DDRC1_RD(noc_vivobus_ddrc1_rd)",
	"DDRC1_WR(noc_vivobus_ddrc1_wr)",
	"service_target(vivo_service_target)",
};

static char *npu_initflow_array[] = {
	"noc_aicore_mst0_rd",
	"noc_aicore_mst0_wr",
	"noc_aicore_mst1_rd",
	"noc_aicore_mst1_wr",
	"noc_aicore_smmu_rd",
	"noc_aicore_smmu_wr",
	"noc_aicpu_mst_rd",
	"noc_aicpu_mst_wr",
	"noc_sys2npubus_cfg_rd",
	"noc_sys2npubus_cfg_wr",
	"noc_sysdma_mst0_rd",
	"noc_sysdma_mst0_wr",
	"noc_sysdma_mst1_rd",
	"noc_sysdma_mst1_wr",
	"noc_sysdma_smmu_rd",
	"noc_sysdma_smmu_wr",
	"noc_tssys_mst_rd",
	"noc_tssys_mst_wr",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
};

static char *npu_targetflow_array[] = {
	"aicore_bus_service_target",
	"aicpu_bus_service_target",
	"noc_aicore_cfg",
	"noc_aicoresmmu_cfg",
	"noc_aicpu_cfg",
	"noc_l2buffer_slv0_rd",
	"noc_l2buffer_slv0_wr",
	"noc_l2buffer_slv1_rd",
	"noc_l2buffer_slv1_wr",
	"noc_npu2cfgbus",
	"noc_npubus_cfg",
	"noc_npubus_ddrc0_rd",
	"noc_npubus_ddrc0_wr",
	"noc_npubus_ddrc1_rd",
	"noc_npubus_ddrc1_wr",
	"noc_npugic_cfg",
	"noc_tssys_slv",
	"npubus_service_target",
	"ts_bus_service_target",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
	"RESERVED",
};
/*
 *operation code
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
	"URG:urgency packet"
};

/*
 * errcode
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
	{0x03, 0x00, 0x0, 0xfe270000},/*aobus_service_target*/
	{0x03, 0x01, 0x0, 0xfe230000},/*aspbus_service_target*/
	{0x03, 0x02, 0x0, 0xfe250000},/*cfgbus_service_target*/
	{0x03, 0x03, 0x0, 0xfe260000},/*dmadebugbus_service_target*/
	{0x03, 0x04, 0x0, 0xfe280000},/*fdbus_service_target*/
	{0x03, 0x05, 0x0, 0xfe210000},/*hsdtbus_service_target*/
	{0x03, 0x06, 0x0, 0xfe200000},/*mmc0bus_service_target*/
	{0x03, 0x07, 0x0, 0xfe220000},/*modembus_service_target*/
	{0x03, 0x08, 0x0, 0xfa980000},/*noc_ao_tcp_cfg*/
	{0x03, 0x09, 0x0, 0xfa880000},/*noc_aon_apb_slv*/
	{0x03, 0x0A, 0x0, 0xfa400000},/*noc_asp_cfg*/
	{0x03, 0x0B, 0x0, 0xe4000000},/*noc_cfg2ipu*/
	{0x03, 0x0C, 0x0, 0xfe300000},/*noc_cfg2syscache*/
	{0x03, 0x0D, 0x0, 0xe9000000},/*CFG-VCODEC(noc_cfg2vcodec)*/
	{0x03, 0x0E, 0x0, 0xe8000000}, /*noc_cfg2vivo"*/
	{0x03, 0x0F, 0x0, 0xfa000000}, /*DMA controller(noc_dmac_cfg)*/
	{0x03, 0x10, 0x0, 0xfac00000}, /*noc_fd_cfg*/
	{0x03, 0x11, 0x0, 0xfe140000}, /*GPU(noc_gpu_cfg)*/
	{0x03, 0x12, 0x0, 0xfa230000}, /*HISEE(noc_hisee_cfg_0)*/
	{0x03, 0x12, 0x1, 0xfa220000}, /*HISEE(noc_hisee_cfg_0)*/
	{0x03, 0x13, 0x0, 0xfe104000}, /*SSI(noc_hkadc_ssi_0)*/
	{0x03, 0x14, 0x0, 0xf8000000}, /*noc_hsdtbus_apb_cfg*/
	{0x03, 0x15, 0x0, 0xfa600000}, /*IOMCU(noc_iomcu_ahb_slv)*/
	{0x03, 0x16, 0x0, 0xfa800000}, /*IOMCU(noc_iomcu_apb_slv)*/
	{0x03, 0x17, 0x0, 0xffe00000}, /*LPM3(noc_lpmcu_slv)*/
	{0x03, 0x17, 0x1, 0xfc000000},/*LPM3(noc_lpmcu_slv)*/
	{0x03, 0x18, 0x0, 0xfa900000}, /*noc_mad_cfg*/
	{0x03, 0x19, 0x0, 0xf8500000}, /*MMC0BUS(noc_mmc0bus_apb_cfg)*/
	{0x03, 0x1A, 0x0, 0xe0000000}, /*MODEM(noc_modem_cfg)*/
	{0x03, 0x1B, 0x0, 0xf0000000}, /*PCIE_0(noc_pcie0_cfg)*/
	{0x03, 0x1C, 0x0, 0xf4000000}, /*PCIE_1(noc_pcie1_cfg)*/
	{0x03, 0x1D, 0x0, 0xc0000000}, /*DDRC(noc_sys2ddrc)*/
	{0x03, 0x1D, 0x1, 0x80000000}, /*DDRC(noc_sys2ddrc)*/
	{0x03, 0x1D, 0x2, 0x0}, /*DDRC(noc_sys2ddrc)*/
	{0x01, 0x1D, 0x3, 0x100000000}, /*DDRC(noc_sys2ddrc)*/
	{0x01, 0x1D, 0x4, 0x200000000}, /*DDRC(noc_sys2ddrc)*/
	{0x01, 0x1D, 0x5, 0x400000000}, /*DDRC(noc_sys2ddrc)*/
	{0x01, 0x1D, 0x6, 0x800000000}, /*DDRC(noc_sys2ddrc)*/
	{0x03, 0x1E, 0x0, 0xfe000000}, /*CFG(noc_sys_peri0_cfg)*/
	{0x03, 0x1F, 0x0, 0xfe100000}, /*CFG(noc_sys_peri1_cfg)*/
	{0x03, 0x20, 0x0, 0xfa080000}, /*DMA(noc_sys_peri2_cfg)*/
	{0x03, 0x21, 0x0, 0xfa040000}, /*DMA(noc_sys_peri3_cfg)*/
	{0x03, 0x22, 0x0, 0xf9000000}, /*CORESIGHT(noc_top_cssys_slv)*/
	{0x03, 0x23, 0x0, 0xf8400000}, /*noc_usb3otg_cfg*/
	{0x03, 0x24, 0x0, 0xfe240000}, /*Service_target(sysbus_service_target)*/
	{0x03, 0x24, 0x1, 0x0}, /*Service_target(sysbus_service_target)*/
};

/* vcodec_bus */
static const ROUTE_ID_ADDR_STRU vcodec_routeid_addr_tbl[] = {
	/* Init_flow  Targ_flow  Targ_subrange  Init_localAddress*/
	/* ---------------------------------------------------*/
	{0x00, 0x00, 0x0, 0xe93d0000}, /*hiface_service_target*/ 
	{0x00, 0x01, 0x0, 0xe9380000}, /*ivp_service_target*/
	{0x00, 0x02, 0x0, 0xe9400000}, /*noc_eps_cfg"*/
	{0x00, 0x03, 0x0, 0xe9500000}, /*noc_hiface_core_cfg*/
	{0x00, 0x04, 0x0, 0xe9000000}, /*noc_ivp32_cfg*/
	{0x01, 0x05, 0x0, 0x0}, /*CRG-CFG2(noc_vcodec2cfg_cfg)*/
	{0x00, 0x06, 0x0, 0xe9300000}, /*CRG-CFG1(noc_vcodec_crg_cfg)*/
	{0x02, 0x07, 0x0, 0x0}, /*DDRC0(noc_vcodecbus_ddrc0)*/
	{0x02, 0x07, 0x1, 0x0}, /*DDRC0(noc_vcodecbus_ddrc0)*/
	{0x02, 0x07, 0x2, 0x0}, /*DDRC0(noc_vcodecbus_ddrc0)*/
	{0x02, 0x07, 0x3, 0x0}, /*DDRC0(noc_vcodecbus_ddrc0)*/
	{0x02, 0x07, 0x4, 0x0}, /*DDRC0(noc_vcodecbus_ddrc0)*/
	{0x02, 0x07, 0x5, 0x0}, /*DDRC0(noc_vcodecbus_ddrc0)*/
	{0x02, 0x07, 0x6, 0x0}, /*DDRC0(noc_vcodecbus_ddrc0)*/
	{0x02, 0x07, 0x7, 0x0}, /*DDRC0(noc_vcodecbus_ddrc0)*/
	{0x02, 0x08, 0x0, 0x80}, /*DDRC0(noc_vcodecbus_ddrc1)*/
	{0x02, 0x08, 0x1, 0x100}, /*DDRC0(noc_vcodecbus_ddrc1)*/
	{0x02, 0x08, 0x2, 0x200}, /*DDRC0(noc_vcodecbus_ddrc1)*/
	{0x02, 0x08, 0x3, 0x400}, /*DDRC0(noc_vcodecbus_ddrc1)*/
	{0x00, 0x09, 0x0, 0xe9200000}, /*VDEC(noc_vdec_cfg)*/
	{0x00, 0x0A, 0x0, 0xe92c0000}, /*VENC2(noc_venc2_cfg)*/
	{0x00, 0x0B, 0x0, 0xe9280000}, /*VENC(noc_venc_cfg)*/
	{0x00, 0x0C, 0x0, 0xe9390000}, /*service_target(vcdoecbus_service_target)*/
	{0x00, 0x0C, 0x1, 0x0}, /*service_target(vcdoecbus_service_target)*/
	{0x00, 0x0D, 0x0, 0xe93a0000}, /*service_target(vdec_service_target)*/
	{0x00, 0x0E, 0x0, 0xe93c0000}, /*service_target(venc2_service_target)*/
	{0x00, 0x0F, 0x0, 0xe93b0000}, /*service_target(venc_service_target)*/
};

/* vivo_bus */
static const ROUTE_ID_ADDR_STRU vivo_routeid_addr_tbl[] = {
	/* Init_flow  Targ_flow  Targ_subrange Init_localAddress */
	/* ----------------------------------------------------- */
	{0x00, 0x00, 0x0, 0xe8680000},/*dss_service_target*/
	{0x00, 0x01, 0x0, 0xe8690000},/*isp_service_target*/
	{0x00, 0x02, 0x0, 0xe8400000},/*noc_dss_cfg*/
	{0x00, 0x03, 0x0, 0xe8000000},/*noc_jpeg_fd_cfg_0*/
	{0x00, 0x04, 0x0, 0xe8200000},/*noc_isp_cfg*/
	{0X0A, 0x05, 0x0, 0x0},/*noc_vivo2cfg*/
	{0x00, 0x06, 0x0, 0xe8600000},/*noc_vivo_crg_cfg*/
	{0x01, 0x07, 0x0, 0x0}, /*noc_vivobus_ddrc0_rd*/
	{0x01, 0x07, 0x1, 0x0}, /*noc_vivobus_ddrc0_rd*/
	{0x01, 0x07, 0x2, 0x0}, /*noc_vivobus_ddrc0_rd*/
	{0x01, 0x07, 0x3, 0x0}, /*noc_vivobus_ddrc0_rd*/
	{0x01, 0x07, 0x4, 0x0}, /*noc_vivobus_ddrc0_rd*/
	{0x01, 0x07, 0x5, 0x0}, /*noc_vivobus_ddrc0_rd*/
	{0x01, 0x07, 0x6, 0x0}, /*noc_vivobus_ddrc0_rd*/
	{0x01, 0x07, 0x7, 0x0}, /*noc_vivobus_ddrc0_rd*/
	{0x02, 0x08, 0x0, 0x0}, /*noc_vivobus_ddrc0_wr*/
	{0x02, 0x08, 0x1, 0x0}, /*noc_vivobus_ddrc0_wr*/
	{0x02, 0x08, 0x2, 0x0}, /*noc_vivobus_ddrc0_wr*/
	{0x02, 0x08, 0x3, 0x0}, /*noc_vivobus_ddrc0_wr*/
	{0x02, 0x08, 0x4, 0x0}, /*noc_vivobus_ddrc0_wr*/
	{0x02, 0x08, 0x5, 0x0}, /*noc_vivobus_ddrc0_wr*/
	{0x02, 0x08, 0x6, 0x0}, /*noc_vivobus_ddrc0_wr*/
	{0x02, 0x08, 0x7, 0x0}, /*noc_vivobus_ddrc0_wr*/
	{0x01, 0x09, 0x0, 0x80}, /*noc_vivobus_ddrc1_rd*/
	{0x01, 0x09, 0x1, 0x100}, /*noc_vivobus_ddrc1_rd*/
	{0x01, 0x09, 0x2, 0x200}, /*noc_vivobus_ddrc1_rd*/
	{0x01, 0x09, 0x3, 0x400}, /*noc_vivobus_ddrc1_rd*/
	{0x02, 0x0A, 0x0, 0x80}, /*noc_vivobus_ddrc1_wr*/
	{0x02, 0x0A, 0x1, 0x100}, /*noc_vivobus_ddrc1_wr*/
	{0x02, 0x0A, 0x2, 0x200}, /*noc_vivobus_ddrc1_wr*/
	{0x02, 0x0A, 0x3, 0x400}, /*noc_vivobus_ddrc1_wr*/
	{0x00, 0x0B, 0x0, 0xe86a0000},/*vivo_service_target*/
	{0x00, 0x0B, 0x1, 0x0},/*vivo_service_target*/
};

/* npu_bus */
static const ROUTE_ID_ADDR_STRU npu_routeid_addr_tbl[] = {
	/* Init_flow  Targ_flow  Targ_subrange Init_localAddress */
	/* ----------------------------------------------------- */
	{0x06, 0x00, 0x0, 0xe4d10000}, /*aicore_bus_service_target*/
	{0x06, 0x01, 0x0, 0xe4d00000}, /*aicpu_bus_service_target*/
	{0x06, 0x02, 0x0, 0xe5000000}, /*noc_aicore_cfg*/
	{0x06, 0x03, 0x0, 0xe5800000}, /*noc_aicoresmmu_cfg*/
	{0x06, 0x04, 0x0, 0xe4c00000}, /*noc_aicpu_cfg*/
	{0x06, 0x05, 0x0, 0xe4800000}, /*noc_l2buffer_slv0_rd*/
	{0x06, 0x05, 0x1, 0xe4800000}, /*noc_l2buffer_slv0_rd*/
	{0x06, 0x05, 0x2, 0xe4800000}, /*noc_l2buffer_slv0_rd*/
	{0x06, 0x05, 0x3, 0xe4800000}, /*noc_l2buffer_slv0_rd*/
	{0x01, 0x06, 0x0, 0xe4800000}, /*noc_l2buffer_slv0_wr*/
	{0x01, 0x06, 0x1, 0xe4800000}, /*noc_l2buffer_slv0_wr*/
	{0x01, 0x06, 0x2, 0xe4800000}, /*noc_l2buffer_slv0_wr*/
	{0x01, 0x06, 0x3, 0xe4800000}, /*noc_l2buffer_slv0_wr*/
	{0x06, 0x07, 0x0, 0xe4800080}, /*noc_l2buffer_slv1_rd*/
	{0x06, 0x07, 0x1, 0xe4800100}, /*noc_l2buffer_slv1_rd*/
	{0x06, 0x07, 0x2, 0xe4800200}, /*noc_l2buffer_slv1_rd*/
	{0x06, 0x07, 0x3, 0xe4800400}, /*noc_l2buffer_slv1_rd*/
	{0x01, 0x08, 0x0, 0xe4800080}, /*noc_l2buffer_slv1_wr*/
	{0x01, 0x08, 0x1, 0xe4800100}, /*noc_l2buffer_slv1_wr*/
	{0x01, 0x08, 0x2, 0xe4800200}, /*noc_l2buffer_slv1_wr*/
	{0x01, 0x08, 0x3, 0xe4800400}, /*noc_l2buffer_slv1_wr*/
	{0x06, 0x09, 0x0, 0xe0000000}, /*noc_npu2cfgbus*/
	{0x06, 0x0A, 0x0, 0xe5e00000}, /*noc_npubus_cfg*/
	{0x00, 0x0B, 0x0, 0x0}, /*noc_npubus_ddrc0_rd*/
	{0x00, 0x0B, 0x1, 0x0}, /*noc_npubus_ddrc0_rd*/
	{0x00, 0x0B, 0x2, 0x0}, /*noc_npubus_ddrc0_rd*/
	{0x00, 0x0B, 0x3, 0x0}, /*noc_npubus_ddrc0_rd*/
	{0x00, 0x0B, 0x4, 0x0}, /*noc_npubus_ddrc0_rd*/
	{0x00, 0x0B, 0x5, 0x0}, /*noc_npubus_ddrc0_rd*/
	{0x00, 0x0B, 0x6, 0x0}, /*noc_npubus_ddrc0_rd*/
	{0x00, 0x0B, 0x7, 0x0}, /*noc_npubus_ddrc0_rd*/
	{0x01, 0x0C, 0x0, 0x0}, /*noc_npubus_ddrc0_wr*/
	{0x01, 0x0C, 0x1, 0x0}, /*noc_npubus_ddrc0_wr*/
	{0x01, 0x0C, 0x2, 0x0}, /*noc_npubus_ddrc0_wr*/
	{0x01, 0x0C, 0x3, 0x0}, /*noc_npubus_ddrc0_wr*/
	{0x01, 0x0C, 0x4, 0x0}, /*noc_npubus_ddrc0_wr*/
	{0x01, 0x0C, 0x5, 0x0}, /*noc_npubus_ddrc0_wr*/
	{0x01, 0x0C, 0x6, 0x0}, /*noc_npubus_ddrc0_wr*/
	{0x01, 0x0C, 0x7, 0x0}, /*noc_npubus_ddrc0_wr*/
	{0x00, 0x0D, 0x0, 0x80}, /*noc_npubus_ddrc1_rd*/
	{0x00, 0x0D, 0x1, 0x100}, /*noc_npubus_ddrc1_rd*/
	{0x00, 0x0D, 0x2, 0x200}, /*noc_npubus_ddrc1_rd*/
	{0x00, 0x0D, 0x3, 0x400}, /*noc_npubus_ddrc1_rd*/
	{0x01, 0x0E, 0x0, 0x80}, /*noc_npubus_ddrc1_wr*/
	{0x01, 0x0E, 0x1, 0x100}, /*noc_npubus_ddrc1_wr*/
	{0x01, 0x0E, 0x2, 0x200}, /*noc_npubus_ddrc1_wr*/
	{0x01, 0x0E, 0x3, 0x400}, /*noc_npubus_ddrc1_wr*/
	{0x06, 0x0F, 0x0, 0xe5c00000}, /*noc_npugic_cfg*/
	{0x06, 0x10, 0x0, 0xe4000000}, /*noc_tssys_slv*/
	{0x08, 0x11, 0x0, 0xe4d30000}, /*npubus_service_target*/
	{0x08, 0x11, 0x1, 0x0}, /*npubus_service_target*/
	{0x08, 0x11, 0x2, 0x0}, /*npubus_service_target*/
	{0x08, 0x11, 0x3, 0x0}, /*npubus_service_target*/
	{0x08, 0x11, 0x4, 0x0}, /*npubus_service_target*/
	{0x08, 0x11, 0x5, 0x0}, /*npubus_service_target*/
	{0x08, 0x11, 0x6, 0x0}, /*npubus_service_target*/
	{0x08, 0x11, 0x7, 0x0}, /*npubus_service_target*/
	{0x08, 0x11, 0x8, 0x0}, /*npubus_service_target*/
	{0x06, 0x12, 0x0, 0xe4d20000}, /*ts_bus_service_target*/
};


struct noc_mid_info noc_mid_PHOE_es[] = {
	/*Bus ID,     init_flow       ,mask   ,mid_va,        mid name */
	{0, 0x02, 0x1C0, 0x0, "AP_CPU0"}, /*noc_cci2sysbus,*/
	{0, 0x02, 0x1C0, 0x40, "AP_CPU1"},   /*noc_cci2sysbus,*/
	{0, 0x02, 0x1C0, 0x80, "AP_CPU2"},  /*noc_cci2sysbus,*/
	{0, 0x02, 0x1C0, 0xC0, "AP_CPU3"},  /*noc_cci2sysbus,*/
	{0, 0x02, 0x1C0, 0x100, "AP_CPU4"},  /*noc_cci2sysbus,*/
	{0, 0x02, 0x1C0, 0x140, "AP_CPU5"},  /*noc_cci2sysbus,*/
	{0, 0x02, 0x1C0, 0x180, "AP_CPU6"},  /*noc_cci2sysbus,*/
	{0, 0x02, 0x1C0, 0x1C0, "AP_CPU7"},  /*noc_cci2sysbus,*/
	{0, 0x0E, 0x003f, 0x00, "LPMCU"},
	{0, 0x08, 0x003f, 0x01, "IOMCU_M7"},/*noc_iomcu_ahb_mst*/
	{0, 0x12, 0x003f, 0x02, "PCIe_1"},
	{0, 0x14, 0x003f, 0x03, "PERF_STAT"},
	{0, 0x0B, 0x003f, 0x04, "IPF"},
	{0, 0x03, 0x003f, 0x05, "DJTAG_M"},
	{0, 0x09, 0x003f, 0x06, "IOMCU_DMA"},
	{0, 0x1A, 0x003f, 0x07, "UFS"},
	{0, 0x15, 0x003f, 0x08, "SD"},
	{0, 0x16, 0x003f, 0x09, "SDIO"},
	{0, 0x17, 0x003f, 0x0A, "CC712"},
	{0, 0x07, 0x003f, 0x0B, "FD_UL"},
	{0, 0x06, 0x003f, 0x0C, "DPC"},
	{0, 0x18, 0x003f, 0x0D, "SOCP"},
	{0, 0x1C, 0x003f, 0x0E, "USB31OTG"},
	{0, 0x19, 0x003f, 0x0F, "TOP_CSSYS"},
	{0, 0x04, 0x003f, 0x10, "DMAC"},
	{0, 0x01, 0x003f, 0x11, "ASP_hifi"},
	{0, 0x10, 0x003f, 0x12, "PCIE_0"},
	{0, 0xFF, 0x003f, 0x13, "ATGC"},
	{0, 0x01, 0x003f, 0x14, "ASP_dma_usb_hdmi"},
	{0, 0x0D, 0x003f, 0x15, "LatMon"},
	{0, 0x0F, 0x003f, 0x24, "DFC"},  /*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x16, "CIPHER"},  /*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x36, "HDLC"},  /*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x17, "CICOM0"},  /*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x37, "CICOM1"},  /*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x18, "NXDSP"},  /*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x19, "TL_BBP_DMA_TCM"},/*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x39, "TL_BBP_DMA_DDR"},/*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x1A, "GU_BBP_MST_TCM"},    /*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x3A, "GU_BBP_MST_DDR"},    /*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x1B, "EDMA0"},  /*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x3B, "EDMA1"},  /*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x1C, "HARQ_L"},    /*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x3C, "HARQ_H"},    /*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x1D, "UPACC"}, /*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x3D, "RSR_ACC"}, /*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x1E, "CIPHER_WRITE_THOUGH"},   /*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x3F, "CCPU_CFG"},   /*noc_modem_mst,*/
	{0, 0x0F, 0x003f, 0x1F, "CCPU_L2C"},   /*noc_modem_mst,*/
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
	{2, 0xFF, 0x003f, 0x1E, "MEDIA_COMMON_CMDLIST"},/*MEDIA_COMMON*/
	{2, 0xFF, 0x003f, 0x1F, "MEDIA_COMMON_RCH_WCH"},/*MEDIA_COMMON*/
	{1, 0x0B, 0x003f, 0x20, "VENC0"},/*VENC0*/
	{1, 0x0B, 0x003f, 0x21, "VENC1"},/*VENC1*/
	{1, 0x0A, 0x003f, 0x22, "VDEC1"},/*VDEC1*/
	{1, 0x0A, 0x003f, 0x23, "VDEC2"},/*VDEC2*/
	{1, 0x0A, 0x003f, 0x24, "VDEC3"},/*VDEC3*/
	{1, 0x0A, 0x003f, 0x25, "VDEC4"},/*VDEC4*/
	{1, 0x0A, 0x003f, 0x26, "VDEC5"},/*VDEC5*/
	{1, 0x0A, 0x003f, 0x27, "VDEC6"},/*VDEC6*/
	{1, 0xFF, 0x003f, 0x28, "VENC2"},/*VENC2_0*/
	{2, 0xFF, 0x003f, 0x29, "DSP_CORE_OR_IVP_ATGM"},/*IVP32_DSP*/
	{2, 0xFF, 0x003f, 0x2A, "IVP32_DSP DSP_DMA"},/*IVP32_DSP*/
	{2, 0xFF, 0x003f, 0x2B, "ISP_A7_CFG"},/*ISP*/
	{2, 0xFF, 0x003f, 0x2C, "IPP_SUBSYS_JPGDEC"},/*IPP_SUBSYS*/
	{2, 0xFF, 0x003f, 0x2D, "IPP_SUBSYS_FD"},/*IPP_SUBSYS*/
	{2, 0xFF, 0x003f, 0x2E, "IPP_SUBSYS_CPE"},/*CPE*/
	{2, 0xFF, 0x003f, 0x2F, "IPP_SUBSYS_SLAM"},/*SLAM*/
	{3, 0xFF, 0x003f, 0x30, "ATGM_NPU"},/*NPU*/
	{3, 0xFF, 0x003f, 0x31, "NPU"},/*NPU*/
	{2, 0xFF, 0x003f, 0x34, "IVP32_DSP DSP_CORE_DATA"},
	{1, 0xFF, 0x003f, 0x35, "VENC2_1"},
	{2, 0xFF, 0x003f, 0x38, "FCM_M0"},
	{2, 0xFF, 0x003f, 0x39, "FCM_M1"},
	{2, 0xFF, 0x003f, 0x3A, "GPU0_not_DRM"},
	{2, 0xFF, 0x003f, 0x3B, "GPU0_DRM"},
	{2, 0xFF, 0x003f, 0x3C, "IDI2AXI"},
	{2, 0xFF, 0x003f, 0x3D, "HiEPS_MMU_PTW"},
	{2, 0xFF, 0x003f, 0x3E, "HiEPS_ARC"},
	{2, 0xFF, 0x003f, 0x3F, "HiEPS_SCE"},
};

struct noc_sec_info noc_sec_PHOE_es[] = {
	/*mask value info  sec_mode*/
	{0x07, 0x00, "trusted", "TZMP2 NSAID"},/*TZMP2 NSAID*/
	{0x07, 0x01, "non-trusted", "TZMP2 NSAID"},/*TZMP2 NSAID*/
	{0x07, 0x02, "protected", "TZMP2 NSAID"},/*TZMP2 NSAID*/
	{0x07, 0x03, "ACPU", "TZMP2 NSAID"},/*TZMP2 NSAID*/
	{0x07, 0x04, "enhanced", "TZMP2 NSAID"},/*TZMP2 NSAID*/
	{0x07, 0x05, "others_illegal", "TZMP2 NSAID"},/*TZMP2 NSAID*/
	{0x08, 0x00, "secure", "secure"},/*secure*/
	{0x08, 0x08, "non-secure", "secure"},/*secure*/
};

struct noc_dump_reg noc_dump_reg_list_PHOE_es[] = {
	/* Table.1 NoC Register Dump: control and state */
	{"PCTRL", (void *)NULL, 0x160},     // PCTRL_nopendingtrans
	{"PCTRL", (void *)NULL, 0x164},     // PCTRL_nopendingtrans
	{"PCTRL", (void *)NULL, 0x168},     // PCTRL_nopendingtrans
	{"PCTRL", (void *)NULL, 0x16c},     // PCTRL_nopendingtrans
	{"PCTRL", (void *)NULL, 0x170},     // PCTRL_nopendingtrans
	{"PCTRL", (void *)NULL, 0x174},     // PCTRL_nopendingtrans
	{"PCTRL", (void *)NULL, 0x178},     // PCTRL_nopendingtrans
	{"PCTRL", (void *)NULL, 0x17c},     // PCTRL_nopendingtrans
	{"PCTRL", (void *)NULL, 0x180},     // PCTRL_nopendingtrans
	{"PCTRL", (void *)NULL, 0x094},     // PCTRL_interrupt
	{"SCTRL", (void *)NULL, 0x384},     // SCTRL_ao_nopendingtrans
	{"SCTRL", (void *)NULL, 0x380},     // SCTRL_ao_interrupt
	{"CRGPERIPH", (void *)NULL, 0x12C}, // CRGPERIPH_PERI_CTRL3
	{"PMCTRL", (void *)NULL, 0x3A0},    // PMCTRL_PERI_INT0_MASK},
	{"PMCTRL", (void *)NULL, 0x3A8},    // PMCTRL_PERI_INT1_MASK},
	{"PMCTRL", (void *)NULL, 0x3A4},    // PMCTRL_PERI_INT0_MASK},
	{"PMCTRL", (void *)NULL, 0x3AC},    // PMCTRL_PERI_INT1_MASK},
	{"SCTRL", (void *)NULL, 0x314},     // SCTRL_SCPERCTRL5},
	{"SCTRL", (void *)NULL, 0x3D0},     // SCTRL_SC_SECOND_INT_MASK},
	{"SCTRL", (void *)NULL, 0x3D8},     // SCTRL_SC_SECOND_INT_OUT},
	{"SCTRL", (void *)NULL, 0x380},     // SCTRL_SCPERSTATUS8},
	{"PMCTRL", (void *)NULL, 0x380},    // PMCTRL_NOC_POWER_IDLEREQ},
	{"PMCTRL", (void *)NULL, 0x384},    // PMCTRL_NOC_POWER_IDLEACK},
	{"PMCTRL", (void *)NULL, 0x388},    // PMCTRL_NOC_POWER_IDLE},
	{"PMCTRL", (void *)NULL, 0x38C},    // PMCTRL_NOC_POWER_IDLEREQ1},
	{"PMCTRL", (void *)NULL, 0x390},    // PMCTRL_NOC_POWER_IDLEACK1},
	{"PMCTRL", (void *)NULL, 0x394},    // PMCTRL_NOC_POWER_IDLE1},
	{"PMCTRL", (void *)NULL, 0x398},
	{"PMCTRL", (void *)NULL, 0x39c},
	{"PMCTRL", (void *)NULL, 0x2f0},
	{"SCTRL", (void *)NULL, 0x31C}, // SCTRL_SCPERCTRL7
	{"SCTRL", (void *)NULL, 0x37C},

};

/*
 * if noc_happend's initflow is in the hisi_modemnoc_initflow,
 * firstly save log, then system reset.
 */
const struct noc_busid_initflow hisi_filter_initflow_PHOE_es[] = {
	/* Bus ID, init_flow, coreid*/
	{0, 11, RDR_CP},	/*ipf*/
	{0, 24, RDR_CP},	/*socp*/
	{0, 8, RDR_IOM3},  /* iomcu core */
	{0, 9, RDR_IOM3},  /* iomcu dma */
	{ARRAY_END_FLAG, 0, RDR_AP},	/*end*/
};

const struct noc_bus_info noc_buses_info_PHOE_es[] = {
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
		.p_noc_mid_info = noc_mid_PHOE_es,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_PHOE_es),
		.p_noc_sec_info = noc_sec_PHOE_es,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_PHOE_es),
	},
	[1] = {
		.name = "vcodec_bus",
		.initflow_mask = ((0xf) << 16),
		.targetflow_mask = ((0xf) << 12),
		.targ_subrange_mask = ((0x7) << 9),
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
		.p_noc_mid_info = noc_mid_PHOE_es,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_PHOE_es),
		.p_noc_sec_info = noc_sec_PHOE_es,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_PHOE_es),
	},
	[2] = {
		.name = "vivo_bus",
		.initflow_mask = ((0x1f) << 16),
		.targetflow_mask = ((0xf) << 12),
		.targ_subrange_mask = ((0x7) << 9),
		.seq_id_mask = (0x1FF),
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
		.p_noc_mid_info = noc_mid_PHOE_es,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_PHOE_es),
		.p_noc_sec_info = noc_sec_PHOE_es,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_PHOE_es),
	},
	[3] = {
		.name = "npu_noc_bus",
		.initflow_mask = ((0x1f) << 18),
		.targetflow_mask = ((0x1f) << 13),
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
		.p_noc_mid_info = noc_mid_PHOE_es,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_PHOE_es),
		.p_noc_sec_info = noc_sec_PHOE_es,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_PHOE_es),
	}
};

/* hisi_noc_get_array_size - get static array size
 * @bus_info_size : bus info array size
 * @dump_list_size: dump list array size
 */
void hisi_noc_get_array_size_PHOE_es(unsigned int *bus_info_size, unsigned int *dump_list_size)
{
	if ((NULL == bus_info_size)||(NULL == dump_list_size))
		return;

	*bus_info_size  = ARRAY_SIZE_NOC(noc_buses_info_PHOE_es);
	*dump_list_size = ARRAY_SIZE_NOC(noc_dump_reg_list_PHOE_es);
}

/*
 * hisi_noc_clock_enable - check noc clock state : on or off
 * @noc_dev : hisi noc device poiter
 * @node: hisi noc node poiter
 *
 * If clock enable, return 1, else return 0;
 */
unsigned int hisi_noc_clock_enable_PHOE_es(struct hisi_noc_device *noc_dev,
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
