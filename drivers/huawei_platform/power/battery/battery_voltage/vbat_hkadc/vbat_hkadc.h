#ifndef _VBAT_HKADC_H_
#define _VBAT_HKADC_H_

#define VBAT_HKADC_RETRY_TIMES      (3)
#define VBAT_HKADC_COEF_MULTIPLE    (1000)

struct vbat_hkadc_info {
	struct platform_device *pdev;
	struct device *dev;
	int adc_channel;
	int offset_channel;
	int offset_value;
	int coef;
};

#endif /* end of _VBAT_HKADC_H_ */
