#ifndef HISI_USB_OTG_ID_H
#define HISI_USB_OTG_ID_H

/* include platform head-file */
#if defined(CONFIG_DEC_USB)
#include "dwc_otg_dec.h"
#else
#error "Passable some error, It should be set a platform"
#endif

#include "dwc_otg_cil.h"

#define SAMPLE_DOING	(0)
#define SAMPLE_DONE		(1)

struct otg_gpio_id_dev {
	struct platform_device *pdev;
	struct notifier_block otg_nb;
	struct notifier_block pogopin_nb;
	unsigned int otg_adc_channel;
	int gpio;
	int irq;
	unsigned int fpga_flag;
	unsigned int sampling_time_optimize;
	struct work_struct  otg_intb_work;
};

#define TRUE 1
#define FALSE 0

#define hw_usb_dbg(format, arg...)    \
	do {                 \
		printk(KERN_INFO "[POGOPIN_DEBUG][USB_DEBUG][%s]"format, __func__, ##arg); \
	} while (0)
#define hw_usb_err(format, arg...)    \
	do {                 \
		printk(KERN_ERR "[POGOPIN_DEBUG][USB_DEBUG]"format, ##arg); \
	} while (0)

#endif

