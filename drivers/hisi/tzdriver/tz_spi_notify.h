#ifndef _TZ_SPI_NOTIFY_H_
#define _TZ_SPI_NOTIFY_H_

int tz_spi_init(struct device *class_dev, struct device_node *np);
void tz_spi_exit(void);

int TC_NS_TST_CMD(TC_NS_DEV_File *dev_id, void *argp);
#endif
