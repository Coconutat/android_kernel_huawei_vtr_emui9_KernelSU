/*
 *
 * Copyright (C) 2015 Hisilicon
 * License terms: GNU General Public License (GPL) version 2
 *
 */
#include <linux/version.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/mutex.h>

#include <soc_ocbc_mbx_interface.h>
#include <soc_acpu_baseaddr_interface.h>



#define OCBC_MODULE_ERR_PRINT(args...)	pr_err(args)

#define OCBC_ERR	-1
#define OCBC_OK		0


static DEFINE_MUTEX(ocbc_mbx_mutex);
static int ocbc_mmbuf_state;
static void __iomem *ocbc_mbx0_base;

int ocbc_mmbuf_request(void)
{
	int timeout;
	void __iomem *pe_ocb_m_dat_addr;
	void __iomem *pe_ocb_m_rdy_addr;
	void __iomem *pe_ocb_m_ovf_addr;
	void __iomem *ocb_pe_m_dat_addr;
	void __iomem *ocb_pe_m_rdy_addr;


	mutex_lock(&ocbc_mbx_mutex);

	if (0 != ocbc_mmbuf_state) {
		OCBC_MODULE_ERR_PRINT("pe->ocbc msg Re-Request\n");
		mutex_unlock(&ocbc_mbx_mutex);
		return OCBC_OK;
	}

	/* addresses of pe->ocb registeres ioremap */
	if (NULL == ocbc_mbx0_base) {
		OCBC_MODULE_ERR_PRINT("%s[%d]: ioremap NULL!\n", __func__, __LINE__);
		mutex_unlock(&ocbc_mbx_mutex);
		return OCBC_ERR;
	}

	pe_ocb_m_dat_addr = SOC_OCBC_MBX_PE_OCB_MBOX_ADDR(ocbc_mbx0_base);
	pe_ocb_m_rdy_addr = SOC_OCBC_MBX_PE_OCB_MBOX_RDY_ADDR(ocbc_mbx0_base);
	pe_ocb_m_ovf_addr = SOC_OCBC_MBX_PE_OCB_MBOX_OVF_ADDR(ocbc_mbx0_base);

	/* addresses of ocb->pe registeres ioremap */
	ocb_pe_m_dat_addr = SOC_OCBC_MBX_OCB_PE_MBOX_ADDR(ocbc_mbx0_base);
	ocb_pe_m_rdy_addr = SOC_OCBC_MBX_OCB_PE_MBOX_RDY_ADDR(ocbc_mbx0_base);

	/* wait 30ms until pe->ocbc mbx idle */
	timeout = 3000;
	while ((0 != (readl(pe_ocb_m_rdy_addr) & 0x01)) && (0 != timeout)) {
		timeout--;
		udelay(10);
	}
	if (0 == timeout) {
		OCBC_MODULE_ERR_PRINT("pe->ocbc mbx busy!\n");
		mutex_unlock(&ocbc_mbx_mutex);
		return OCBC_ERR;
	}

	/* clear overflow interrupt */
	if (0 != (readl(pe_ocb_m_ovf_addr) & 0x01)) {
		OCBC_MODULE_ERR_PRINT("pe->ocbc mbx overflow happened!\n");
		writel(0x01, pe_ocb_m_ovf_addr);
	}

	/*
	 * request mmbuf msg
	 *
	 * ocbc mbx message configuration
	 * [15:0]: Indicate the target OCBs
	 *    bit0 - mmbuf
	 *
	 * [23:16]: Attribute (oneshot)
	 *    Bit16：req
	 *    Bit17：release
	 *    Bit23：Deinit
	 *
	 * [27:24]: CMD
	 *    0x00: Binding request mode (default)
	 *    0x02: Parallel request mode
	 *
	 */
	writel(0x10001, pe_ocb_m_dat_addr);

	/* wait 200ms until receive ocbc->pe msg */
	timeout = 20000;
	while ((0 == readl(ocb_pe_m_rdy_addr)) && (0 != timeout)) {
		timeout--;
		udelay(10);
	}
	if (0 == timeout) {
		OCBC_MODULE_ERR_PRINT("ocbc not reply within timeout period!\n");
		mutex_unlock(&ocbc_mbx_mutex);
		return OCBC_ERR;
	}

	/* check msg and clear ocbc->pe rdy */
	if (0 == (readl(ocb_pe_m_dat_addr) & 0x01)) {
		OCBC_MODULE_ERR_PRINT("ocbc reply msg error!\n");
		mutex_unlock(&ocbc_mbx_mutex);
		return OCBC_ERR;
	}
	writel(0x01, ocb_pe_m_rdy_addr);

	ocbc_mmbuf_state = 1;

	mutex_unlock(&ocbc_mbx_mutex);

	return OCBC_OK;
}
EXPORT_SYMBOL_GPL(ocbc_mmbuf_request);

int ocbc_mmbuf_release(void)
{
	int timeout;
	void __iomem *pe_ocb_m_dat_addr;
	void __iomem *pe_ocb_m_rdy_addr;
	void __iomem *pe_ocb_m_ovf_addr;


	mutex_lock(&ocbc_mbx_mutex);

	if (0 == ocbc_mmbuf_state) {
		OCBC_MODULE_ERR_PRINT("pe->ocbc msg Re-Release\n");
		mutex_unlock(&ocbc_mbx_mutex);
		return OCBC_OK;
	}

	/* addresses of pe->ocb registeres ioremap */
	if (NULL == ocbc_mbx0_base) {
		OCBC_MODULE_ERR_PRINT("%s[%d]: ioremap NULL!\n", __func__, __LINE__);
		mutex_unlock(&ocbc_mbx_mutex);
		return OCBC_ERR;
	}

	pe_ocb_m_dat_addr = SOC_OCBC_MBX_PE_OCB_MBOX_ADDR(ocbc_mbx0_base);
	pe_ocb_m_rdy_addr = SOC_OCBC_MBX_PE_OCB_MBOX_RDY_ADDR(ocbc_mbx0_base);
	pe_ocb_m_ovf_addr = SOC_OCBC_MBX_PE_OCB_MBOX_OVF_ADDR(ocbc_mbx0_base);

	/* wait 30ms until pe->ocbc mbx idle */
	timeout = 3000;
	while ((0 != (readl(pe_ocb_m_rdy_addr) & 0x01)) && (0 != timeout)) {
		timeout--;
		udelay(10);
	}
	if (0 == timeout) {
		OCBC_MODULE_ERR_PRINT("pe->ocbc mbx busy!\n");
		mutex_unlock(&ocbc_mbx_mutex);
		return OCBC_ERR;
	}

	/* clear overflow interrupt */
	if (0 != (readl(pe_ocb_m_ovf_addr) & 0x01)) {
		OCBC_MODULE_ERR_PRINT("pe->ocbc mbx overflow happened!\n");
		writel(0x01, pe_ocb_m_ovf_addr);
	}

	/*
	 * release mmbuf msg
	 *
	 * ocbc mbx message configuration
	 * [15:0]: Indicate the target OCBs
	 *    bit0 - mmbuf
	 *
	 * [23:16]: Attribute (oneshot)
	 *    Bit16：req
	 *    Bit17：release
	 *    Bit23：Deinit
	 *
	 * [27:24]: CMD
	 *    0x00: Binding request mode (default)
	 *    0x02: Parallel request mode
	 *
	 */
	writel(0x20001, pe_ocb_m_dat_addr);

	ocbc_mmbuf_state = 0;

	mutex_unlock(&ocbc_mbx_mutex);

	return OCBC_OK;
}
EXPORT_SYMBOL_GPL(ocbc_mmbuf_release);

static __init int ocbc_mmbuf_init(void)
{
	ocbc_mbx0_base = ioremap(SOC_ACPU_OCBC_MBOX_0_BASE_ADDR, SZ_4K);
	if (NULL == ocbc_mbx0_base) {
		OCBC_MODULE_ERR_PRINT("%s: ioremap fail!\n", __func__);
		return OCBC_ERR;
	}

	return 0;
}

arch_initcall(ocbc_mmbuf_init);
