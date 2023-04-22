#include "irda_driver.h"
#include <linux/mutex.h>

/*
* CONFIG_USE_CAMERA3_ARCH : the camera module build config
* du to the irda power suplly by camera power chip
*/
/*lint -e749*/

#define IRDA_DRIVER_COMPATIBLE_ID		"irda,config"
#define IRDA_CHIP_TYPE		"irda,chiptype"

#define IRDA_BUFF_SIZE			50
#define IRDA_SUCCESS			0
#define IRDA_ERROR			-1

#define HWLOG_TAG irda_chiptype
HWLOG_REGIST();


/**
*struct for the chip type.
*/
enum irda_chiptype {
	DEFAULT = 0,
	MAXIM_616,
	HI1102,
	HI64XX,
	OTHERS,
};
int g_chip_type;

struct irda_device {
	struct platform_device *pdev;
	struct device *dev;
};

extern struct class *irda_class;

static ssize_t chip_type_get(struct device *dev,
			struct device_attribute *attr, char *buf)
{

	hwlog_info("chiptype is :%d\n", g_chip_type);

	return snprintf(buf, IRDA_BUFF_SIZE, "%d\0", g_chip_type);
}

static ssize_t chiptype_set(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	int ret = -1;
	unsigned long state = 0;

	if (!kstrtol(buf, 10, &state)) {
		g_chip_type = (int)state;
	}
	return ret;
}

static DEVICE_ATTR(ir_chip_type, 0440, chip_type_get, chiptype_set);

static int irda_probe(struct platform_device *pdev)
{
	int ret;
	int power_type;
	struct device_node *np;
	struct irda_device *irda_dev;

	g_chip_type = 0;
	np = pdev->dev.of_node;
	if (np == NULL) {
		hwlog_err("default chip type is maxim\n");
		g_chip_type = MAXIM_616;
		return 0;
	}

	irda_dev =
	    (struct irda_device *)kzalloc(sizeof(struct irda_device),
					  GFP_KERNEL);
	if (NULL == irda_dev) {
		hwlog_err("Failed to allocate irda_dev\n");
		ret = -ENOMEM;
		goto error;
	}

	ret = of_property_read_u32(np, IRDA_CHIP_TYPE, &g_chip_type);
	if (ret) {
		hwlog_warn("Failed to get chipset type; ret:%d\n", ret);
		/*
		* set default chiptype as maxim;
		*/
		g_chip_type = MAXIM_616;
	}

	irda_dev->dev =
	    device_create(irda_class, NULL, MKDEV(0, 0), NULL, "%s",
			  "irda_chip");
	if (IS_ERR(irda_dev->dev)) {
		ret = PTR_ERR(irda_dev->dev);
		hwlog_err("Failed to create dev; ret:%d\n", ret);
		goto free_irda_dev;
	}

	ret = device_create_file(irda_dev->dev, &dev_attr_ir_chip_type);
	if (ret) {
		hwlog_err("Failed to create file; ret:%d\n", ret);
		goto free_dev;
	}

	dev_set_drvdata(irda_dev->dev, irda_dev);
	platform_set_drvdata(pdev, irda_dev);

	hwlog_info("platform device probe success\n");
	return 0;

free_dev:
	device_destroy(irda_class, irda_dev->dev->devt);
free_irda_dev:
	kfree(irda_dev);
error:
	return ret;
}

static int irda_remove(struct platform_device *pdev)
{
	struct irda_device *irda_dev = platform_get_drvdata(pdev);

	device_remove_file(irda_dev->dev, &dev_attr_ir_chip_type);
	device_destroy(irda_class, irda_dev->dev->devt);
	class_destroy(irda_class);
	kfree(irda_dev);

	return 0;
}

static const struct of_device_id irda_match_table[] = {
		{
			.compatible = IRDA_DRIVER_COMPATIBLE_ID,
			.data = NULL,
		},
		{
		},
};

MODULE_DEVICE_TABLE(of, irda_match_table);

static struct platform_driver irda_driver = {
	.probe = irda_probe,
	.remove = irda_remove,
	.driver = {
		.name = "irda",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(irda_match_table),
	},
};

int irda_chip_type_regist(void)
{
	return platform_driver_register(&irda_driver);
}

void irda_chip_type_unregist(void)
{
	platform_driver_unregister(&irda_driver);
}
