#ifndef __HISI_PERIPHERAL_TM_H
#define __HISI_PERIPHERAL_TM_H
#include <linux/thermal.h>
#include <linux/hisi/hisi_adc.h>
#define NTC_TEMP_MIN_VALUE	(-40)
#define NTC_TEMP_MAX_VALUE	(125)
#define ADC_RTMP_DEFAULT_VALUE	(44)

#ifdef CONFIG_HISI_THERMAL_CONTEXTHUB
struct contexthub_chip {
	void __iomem *share_addr;
	struct hw_chan_table adc_table[0];
};
#endif

struct periph_tsens_tm_device_sensor {
	struct thermal_zone_device	*tz_dev;
	enum thermal_device_mode	mode;
	unsigned int			sensor_num;
	struct work_struct		work;
	const char			*ntc_name;
	int				chanel;/*hw*/
	int				state;
#ifdef CONFIG_HISI_THERMAL_TRIP
	s32				temp_throttling;
	s32				temp_shutdown;
	s32				temp_below_vr_min;
	s32				temp_over_skin;
#endif
};

struct hisi_peripheral_tm_chip {
	struct platform_device		*pdev;
	struct device			*dev;
	struct notifier_block		nb;
	struct work_struct			tsens_work;
	struct delayed_work		tsens_periph_tm_work;
	int				average_period;
#ifdef CONFIG_HISI_THERMAL_CONTEXTHUB
	struct contexthub_chip*  chub;
#endif
	int				tsens_num_sensor;
	struct periph_tsens_tm_device_sensor sensor[0];
};

extern int hisi_peripheral_ntc_2_temp(struct periph_tsens_tm_device_sensor *chip, int *temp,int ntc);
extern int hisi_peripheral_temp_2_ntc(struct periph_tsens_tm_device_sensor *chip, int temp,u16 *ntc);
int hisi_peripheral_get_table_info(const char* ntc_name, unsigned long* dest, enum hkadc_table_id* table_id);
#endif
