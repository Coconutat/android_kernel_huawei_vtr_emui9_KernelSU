#ifndef _LTC7820_H_
#define _LTC7820_H_

#define LTC7820_CHIP_ENABLE               (1)
#define LTC7820_CHIP_DISABLE              (0)
#define LTC7820_FREQ_ENABLE               (1)
#define LTC7820_FREQ_DISABLE              (0)

#define LTC7820_INIT_FINISH               (1)
#define LTC7820_NOT_INIT                  (0)
#define LTC7820_ENABLE_INTERRUPT_NOTIFY   (1)
#define LTC7820_DISABLE_INTERRUPT_NOTIFY  (0)

struct ltc7820_device_info {
	struct platform_device *pdev;
	struct device *dev;
	struct device_node *dev_node;
	struct work_struct irq_work;
	struct nty_data nty_data;
	int gpio_en;
	int gpio_freq;
	int gpio_int;
	int irq_int;
	int chip_already_init;
	int device_id;
};

#endif /* end of _LTC7820_H_ */
