#ifndef _HIFI_USB_PHY_H_
#define _HIFI_USB_PHY_H_

#ifdef CONFIG_USB_DWC3_FEB
#include <linux/mfd/hisi_pmic.h>

static inline void hisi_usb_ldo23_on(void)
{
	uint32_t temp;
	temp = hisi_pmic_reg_read(0x11D);
	temp &= 0xF7;
	hisi_pmic_reg_write(0x11D, temp);
}

static inline void hisi_usb_ldo23_auto(void)
{
	uint32_t temp;
	temp = hisi_pmic_reg_read(0x11D);
	temp |= 0x8;
	hisi_pmic_reg_write(0x11D, temp);
}

static inline void hisi_usb_ldo5_on(void)
{
	uint32_t temp;
	temp = hisi_pmic_reg_read(0x11D);
	temp &= 0xBF;
	hisi_pmic_reg_write(0x11D, temp);
}

static inline void hisi_usb_ldo5_auto(void)
{
	uint32_t temp;
	temp = hisi_pmic_reg_read(0x11D);
	temp |= 0x40;
	hisi_pmic_reg_write(0x11D, temp);
}

static inline void hisi_usb_ldo30_on(void)
{
	uint32_t temp;
	temp = hisi_pmic_reg_read(0x11D);
	temp &= 0xFE;
	hisi_pmic_reg_write(0x11D, temp);
}

static inline void hisi_usb_ldo30_off(void)
{
	uint32_t temp;
	temp = hisi_pmic_reg_read(0x11D);
	temp |= 0x1;
	hisi_pmic_reg_write(0x11D, temp);
}
static inline __u32 hisi_usb_phy_regu_reg_val(void)
{
	return hisi_pmic_reg_read(0x11D);
}
#else
static inline void hisi_usb_ldo23_on(void){}
static inline void hisi_usb_ldo23_auto(void){}
static inline void hisi_usb_ldo5_on(void){}
static inline void hisi_usb_ldo5_auto(void){}
static inline void hisi_usb_ldo30_on(void){}
static inline void hisi_usb_ldo30_off(void){}
static inline __u32 hisi_usb_phy_regu_reg_val(void){return 0xffffffff;}
#endif

#endif
