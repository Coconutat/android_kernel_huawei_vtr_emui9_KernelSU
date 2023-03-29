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

#include "hisi_noc_info.h"
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG HISI_NOC_TAG

#define ERR_CODE_NR  8
#define OPC_NR      10

#define CFG_INITFLOW_ARRAY_SIZE         32
#define CFG_TARGETFLOW_ARRAY_SIZE       48
#define VIVO_INITFLOW_ARRAY_SIZE        16
#define VIVO_TARGETFLOW_ARRAY_SIZE      8
#define VCODEC_INITFLOW_ARRAY_SIZE      4
#define VCODEC_TARGETFLOW_ARRAY_SIZE    8

static char *cfg_initflow_array[] = {
	"A7(noc_a7tocfg)",
	"Audio(noc_asp_mst)",
	"ACPU(noc_cci2sysbus)",
	"DJTAG(noc_djtag_mst)",
	"DMA Controller(noc_dmac_mst)",
	"EMMC51(noc_emmc51)",
	"HISEE(noc_hisee_mst)",
	"SensorHub(noc_iomcu_ahb_mst)",
	"DMA-SensorHub(noc_iomcu_dma_mst)",
	"IPF(noc_ipf)",
	"IVP32(noc_ivp32_mst)",
	"LPM3(noc_lpmcu_mst)",
	"MODEM(noc_modem_mst)",
	"OCB(noc_ocb_mcu_mst)",
	"PCIE(noc_pcie)",
	"PERF_STAT_DEBUG(noc_perf_stat)",
	"SD3(noc_sd3)",
	"SDIO(noc_sdio)",
	"SECURITY(noc_sec_p)",
	"SECURITY(noc_sec_s)",
	"SOCP_DEBUG(noc_socp)",
	"CORESIGHT(noc_top_cssys)",
	"UFS(noc_ufs_mst)",
	"USB3(noc_usb3otg)",
};

static char *cfg_targetflow_array[] = {
	"AOBUS(aobus_service_target)",
	"Audio(aspbus_service_target)",
	"Service_target(cfgbus_service_target)",
	"Service_target(dmabus_service_target)",
	"Service_target(ivp32bus_service_target)",
	"Service_target(mmc0bus_service_target)",
	"Service_target(mmc1bus_service_target)",
	"Service_target(modembus_service_target)",
	"AO-MMBUF(noc_aobus_mmbuf)",
	"Audio(noc_asp_cfg)",
	"CCI(noc_cci_cfg)",
	"Vivo(noc_cfg2vivo)",
	"DMA controller(noc_dmac_cfg)",
	"EMMC51(noc_emmc51_cfg)",
	"GIC(noc_gic)",
	"GPU(noc_gpu_cfg)",
	"HISEE(noc_hisee_cfg)",
	"SSI(noc_hkadc_ssi)",
	"IOMCU(noc_iomcu_ahb_slv)",
	"IOMCU(noc_iomcu_apb_slv)",
	"IVP32(noc_ivp32_cfg)",
	"LPM3(noc_lpmcu_slv)",
	"MMC0BUS(noc_mmc0bus_apb_cfg)",
	"MMC1BUS(noc_mmc1bus_apb_cfg)",
	"MODEM(noc_modem_cfg)",
	"OCB(noc_ocb_cfg)",
	"PCIE(noc_pcie_cfg)",
	"DDRC(noc_sys2ddrc)",
	"HKMEM(noc_sys2hkmem)",
	"CFG(noc_sys_peri0_cfg)",
	"CFG(noc_sys_peri1_cfg)",
	"DMA(noc_sys_peri2_cfg)",
	"DMA(noc_sys_peri3_cfg)",
	"CORESIGHT(noc_top_cssys_slv)",
	"UFS-CFG(noc_ufs_cfg)",
	"USB3(noc_usb3otg_cfg)",
	"Service_target(sysbus_service_target)",
	"UFS-BUS(ufsbus_service_target)",
};

static char *vcodec_initflow_array[] = {
	"VCODEC(noc_vcodec_cfg)",
	"VDEC(noc_vdec)",
	"VENC(noc_venc)",
	"RESERVED",
};

static char *vcodec_targetflow_array[] = {
	"DDRC0(noc_vcodecbus_ddrc0)",
	"DDRC0(noc_vcodecbus_ddrc1)",
	"VDEC(noc_vdec_cfg)",
	"VENC(noc_venc_cfg)",
	"service_target (vcodec_bus_service_target)",
	"service_target (vdec_service_target)",
	"venc_service_target",
};

static char *vivo_initflow_array[] = {
	"A7_RD(noc_a7tovivobus_rd)",
	"A7_WR (noc_a7tovivobus_wr)",
	"DSS0_RD(noc_dss0_rd)",
	"DSS0_WR(noc_dss0_wr)",
	"DSS1_RD (noc_dss1_rd)",
	"DSS1_WR(noc_dss1_wr)",
	"ISP1_RD(noc_isp1_rd)",
	"ISP1_WR(noc_isp1_wr)",
	"ISP_RD(noc_isp_rd)",
	"ISP_WR(noc_isp_wr)",
	"VIVO(noc_vivo_cfg)",
};


static char *vivo_targetflow_array[VIVO_TARGETFLOW_ARRAY_SIZE] = {
	"service_target (dss_service_target)",
	"service_target (isp_service_target)",
	"service_target (noc_dss_cfg)",
	"ISP(noc_isp_cfg)",
	"DDRC(noc_vivobus_ddrc0_rd)",
	"DDRC (noc_vivobus_ddrc0_wr)",
	"DDRC (noc_vivobus_ddrc1_rd)",
	"DDRC (noc_vivobus_ddrc1_wr)"
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
	{0x02, 0x00, 0x0, 0xe9870000},
	{0x02, 0x01, 0x0, 0xe9830000},
	{0x02, 0x02, 0x0, 0xe9800000},
	{0x02, 0x03, 0x0, 0xe9860000},
	{0x02, 0x04, 0x0, 0xe9850000},
	{0x02, 0x05, 0x0, 0xe9890000},
	{0x02, 0x06, 0x0, 0xe9880000},
	{0x02, 0x07, 0x0, 0xe9820000},
	{0x02, 0x08, 0x0, 0xea840000},
	{0x02, 0x08, 0x1, 0xea800000},
	{0x02, 0x09, 0x0, 0xe8000000},
	{0x02, 0x0A, 0x0, 0xe8100000},
	{0x02, 0x0B, 0x0, 0xe8400000},
	{0x02, 0x0C, 0x0, 0xfdf30000},
	{0x02, 0x0D, 0x0, 0xff3fb000},
	{0x02, 0x0E, 0x0, 0xe82b0000},
	{0x02, 0x0F, 0x0, 0xe82c0000},
	{0x02, 0x10, 0x0, 0xf0e00000},
	{0x02, 0x11, 0x0, 0xe82b8000},
	{0x02, 0x12, 0x0, 0xf0800000},
	{0x02, 0x12, 0x1, 0xf0000000},
	{0x02, 0x13, 0x0, 0xffd00000},
	{0x02, 0x14, 0x0, 0xe8c00000},
	{0x02, 0x15, 0x0, 0xea900000},
	{0x02, 0x15, 0x1, 0xffe00000},
	{0x02, 0x15, 0x2, 0xed000000},
	{0x02, 0x15, 0x3, 0xec000000},
	{0x02, 0x16, 0x0, 0xff37e000},
	{0x02, 0x17, 0x0, 0xff3fc000},
	{0x02, 0x17, 0x1, 0xff3e0000},
	{0x02, 0x17, 0x2, 0xf3f00000},
	{0x02, 0x18, 0x0, 0xe0000000},
	{0x02, 0x19, 0x0, 0xf1100000},
	{0x02, 0x1A, 0x0, 0xf4000000},
	{0x02, 0x1A, 0x1, 0xf8000000},
	{0x02, 0x1B, 0x0, 0xc0000000},
	{0x02, 0x1B, 0x1, 0x80000000},
	{0x02, 0x1B, 0x2, 0x0},
	{0x02, 0x1B, 0x3, 0x100000000},
	{0x02, 0x1B, 0x4, 0x200000000},
	{0x02, 0x1C, 0x0, 0xea000000},
	{0x02, 0x1C, 0x1, 0x0},
	{0x02, 0x1D, 0x0, 0xe8a00000},
	{0x02, 0x1E, 0x0, 0xe8800000},
	{0x02, 0x1F, 0x0, 0xff000000},
	{0x02, 0x20, 0x0, 0xfdf00000},
	{0x02, 0x21, 0x0, 0xfe000000},
	{0x02, 0x22, 0x0, 0xff3b0000},
	{0x02, 0x23, 0x0, 0xff200000},
	{0x02, 0x23, 0x1, 0xff100000},
	{0x02, 0x24, 0x0, 0xe9840000},
	{0x02, 0x25, 0x0, 0xe9810000},
};

/* vivo_bus */
static const ROUTE_ID_ADDR_STRU vivo_routeid_addr_tbl[] = {
	/* Init_flow  Targ_flow  Targ_subrange Init_localAddress */
	/* ----------------------------------------------------- */
	{0x0A, 0x00, 0x0, 0xe86c0000},
	{0x0A, 0x00, 0x1, 0x0},
	{0x0A, 0x01, 0x0, 0xe86d0000},
	{0x0A, 0x02, 0x0, 0xe8600000},
	{0x0A, 0x03, 0x0, 0xe8400000},
	{0x00, 0x04, 0x0, 0x0},
	{0x00, 0x04, 0x1, 0x0},
	{0x01, 0x05, 0x0, 0x0},
	{0x01, 0x05, 0x1, 0x0},
	{0x00, 0x06, 0x0, 0x0},
	{0x00, 0x06, 0x1, 0x0},
	{0x01, 0x07, 0x0, 0x0},
	{0x01, 0x07, 0x1, 0x0},
};

/* vcodec_bus */
static const ROUTE_ID_ADDR_STRU vcodec_routeid_addr_tbl[] = {
	/* Init_flow  Targ_flow  Targ_subrange : Init_mapping ,
	Init_localAddress , Targ_mapping , Targ_localAddress */
	/* ---------------------------------------------------*/
	{0x01, 0x00, 0x0, 0x0},
	{0x01, 0x00, 0x1, 0x0},
	{0x01, 0x01, 0x0, 0x0},
	{0x01, 0x01, 0x1, 0x0},
	{0x00, 0x02, 0x0, 0xe8800000},
	{0x00, 0x03, 0x0, 0xe8900000},
	{0x00, 0x04, 0x0, 0xe8920000},
	{0x00, 0x04, 0x1, 0x0},
	{0x00, 0x05, 0x0, 0xe8930000},
	{0x00, 0x06, 0x0, 0xe8940000},
};

struct noc_mid_info noc_mid_hi3660[] = {
	/*Bus ID,     init_flow       ,mask   ,mid_va,        mid name */
	{0, 2, 0x3800, 0x0, "AP_CPU0"}, /*noc_cci2sysbus,*/
	{0, 2, 0x3800, 0x800, "AP_CPU1"},   /*noc_cci2sysbus,*/
	{0, 2, 0x3800, 0x1000, "AP_CPU2"},  /*noc_cci2sysbus,*/
	{0, 2, 0x3800, 0x1800, "AP_CPU3"},  /*noc_cci2sysbus,*/
	{0, 2, 0x3800, 0x2000, "AP_CPU4"},  /*noc_cci2sysbus,*/
	{0, 2, 0x3800, 0x2800, "AP_CPU5"},  /*noc_cci2sysbus,*/
	{0, 2, 0x3800, 0x3000, "AP_CPU6"},  /*noc_cci2sysbus,*/
	{0, 2, 0x3800, 0x3800, "AP_CPU7"},  /*noc_cci2sysbus,*/
	{0, 12, 0x007f, 0x02, "HDLC"},  /*noc_modem_mst,*/
	{0, 12, 0x007f, 0x14, "CBBE16"},  /*noc_modem_mst,*/
	{0, 12, 0x007f, 0x15, "TLBBE16"},  /*noc_modem_mst,*/
	{0, 12, 0x007f, 0x16, "BBP_DMA_0"},/*noc_modem_mst,*/
	{0, 12, 0x007f, 0x17, "BBP_DMA_1"},/*noc_modem_mst,*/
	{0, 12, 0x007f, 0x18, "GU_BBP_MST_0"},    /*noc_modem_mst,*/
	{0, 12, 0x007f, 0x19, "GU_BBP_MST_1"},    /*noc_modem_mst,*/
	{0, 12, 0x007f, 0x1A, "CCPU_CFG"},   /*noc_modem_mst,*/
	{0, 12, 0x007f, 0x1B, "EDMA0"},  /*noc_modem_mst,*/
	{0, 12, 0x007f, 0x1C, "CIPHER"},    /*noc_modem_mst,*/
	{0, 12, 0x007f, 0x1D, "EDMA1"}, /*noc_modem_mst,*/
	{0, 12, 0x007f, 0x1E, "UPACC"}, /*noc_modem_mst,*/
	{0, 12, 0x007f, 0x1F, "CICOM0_OR_CICOM1"},   /*noc_modem_mst,*/
	{0, 12, 0x007f, 0x62, "CCPU_L2C"},   /*noc_modem_mst,*/
	{0, 10, 0x007f, 0x23, "DSP_CORE"},  /*ivp32,*/
};

struct noc_sec_info noc_sec_hi3660[] = {
	/*mask value info  sec_mode*/
	{0x01, 0x00, "secure", "secure"},/*secure*/
	{0x01, 0x01, "non-secure", "secure"},/*secure*/
};

struct noc_dump_reg noc_dump_reg_list_hi3660[] = {
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
const struct noc_busid_initflow hisi_filter_initflow_hi3660[] = {
	/* Bus ID, init_flow, coreid*/
	{0, 9, RDR_CP},	/*ipf*/
	{0, 20, RDR_CP},	/*socp*/
	{0, 7, RDR_IOM3},
	{0, 8, RDR_IOM3},
	{0, 1, RDR_HIFI},
	{ARRAY_END_FLAG, 0, RDR_AP},	/*end*/
};

const struct noc_bus_info noc_buses_info_hi3660[] = {
	[0] = {
		.name = "cfg_sys_noc_bus",
		.initflow_mask = ((0x1f) << 18),
		.targetflow_mask = ((0x3f) << 12),
		.targ_subrange_mask = ((0x7) << 9),
		.seq_id_mask = (0x3FFF),
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
		.p_noc_mid_info = noc_mid_hi3660,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_hi3660),
		.p_noc_sec_info = noc_sec_hi3660,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_hi3660),
	},
	[1] = {
		.name = "vcodec_bus",
		.initflow_mask = ((0x3) << 12),
		.targetflow_mask = ((0x7) << 9),
		.targ_subrange_mask = ((0x1) << 8),
		.seq_id_mask = (0x1FFF),
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
		.p_noc_mid_info = noc_mid_hi3660,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_hi3660),
		.p_noc_sec_info = noc_sec_hi3660,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_hi3660),
	},
	[2] = {
		.name = "vivo_bus",
		.initflow_mask = ((0xf) << 12),
		.targetflow_mask = ((0x7) << 9),
		.targ_subrange_mask = ((0x1) << 8),
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
		.p_noc_mid_info = noc_mid_hi3660,
		.noc_mid_info_size = ARRAY_SIZE_NOC(noc_mid_hi3660),
		.p_noc_sec_info = noc_sec_hi3660,
		.noc_sec_info_size = ARRAY_SIZE_NOC(noc_sec_hi3660),
	}
};

/* hisi_noc_get_array_size - get static array size
 * @bus_info_size : bus info array size
 * @dump_list_size: dump list array size
 */
void hisi_noc_get_array_size_hi3660(unsigned int *bus_info_size, unsigned int *dump_list_size)
{
	if ((NULL == bus_info_size)||(NULL == dump_list_size))
		return;

	*bus_info_size  = ARRAY_SIZE_NOC(noc_buses_info_hi3660);
	*dump_list_size = ARRAY_SIZE_NOC(noc_dump_reg_list_hi3660);
}

/* hisi_noc_clock_enable - check noc clock state : on or off
 * @noc_dev : hisi noc device poiter
 * @node: hisi noc node poiter
 *
 * If clock enable, return 1, else return 0;
 */
unsigned int hisi_noc_clock_enable_hi3660(struct hisi_noc_device *noc_dev,
				     struct noc_node *node)
{
    void __iomem *clock_reg = NULL;
	unsigned int reg_value;
	unsigned int i;
	unsigned int ret = 1;

	if ((NULL == noc_dev)||(NULL == node))
		return 0;

    for (i = 0; i < HISI_NOC_CLOCK_MAX; i++) {
		if (0xFFFFFFFF == node->crg_clk[i].offset)
			continue;

		clock_reg =
		    (void __iomem *)((unsigned long)noc_dev->pcrgctrl_base +
					node->crg_clk[i].offset);
		reg_value = readl_relaxed(clock_reg);

		/* Clock is enabled */
		if (reg_value & (1U << node->crg_clk[i].mask_bit))
			continue;
		else {
			ret = 0;
			break;
		}
	}

	if (noc_dev->noc_property->noc_debug)
		pr_err("%s: clock_reg = 0x%pK\n", __func__, clock_reg);

    return ret;
}
