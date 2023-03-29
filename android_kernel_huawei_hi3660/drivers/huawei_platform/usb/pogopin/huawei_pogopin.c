#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/usb/huawei_pogopin.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/hisi/usb/hisi_usb.h>
#include "hisi_scharger_v300.h"

#define HIGH 1
#define LOW  0
#define TRUE 1
#define FALSE 0

extern struct hi6523_device_info *g_hi6523_dev;
struct pogopin_device_ops  *g_pogopin_device_ops = NULL;
struct cc_detect_ops *cc_detect_ops = NULL;
struct device *pogopin_dev;
static struct class *pogopin_class;
static u32 pogopin_support = 0;
struct completion dev_off_completion;

static inline void __pogopin_switch_buck_boost(struct pogopin_device_info *di, int value){
	gpio_set_value(di->buck_boost_gpio, value);
}
static inline void __pogopin_switch_power(struct pogopin_device_info *di, int value){
	gpio_set_value(di->power_switch_gpio,value);
	gpio_set_value(di->switch_power_gpio,value);
}
static inline void __pogopin_switch_dpdm(struct pogopin_device_info *di, int value){
	gpio_set_value(di->usb_switch_gpio,value);
}

static int get_connect_status(void){
	int power_switch_value = -1, switch_power_value = -1, usb_switch_value = -1, buck_boost_value = -1, pogopin_int_value = -1;
	power_switch_value = gpio_get_value(g_pogopin_device_ops->di->power_switch_gpio);
	switch_power_value = gpio_get_value(g_pogopin_device_ops->di->switch_power_gpio);
	usb_switch_value = gpio_get_value(g_pogopin_device_ops->di->usb_switch_gpio);
	buck_boost_value = gpio_get_value(g_pogopin_device_ops->di->buck_boost_gpio);
	pogopin_int_value = gpio_get_value(g_pogopin_device_ops->di->pogopin_int_gpio);
	hw_pogopin_dbg("power_switch = %d switch_power = %d usb_switch = %d buck_boost = %d pogopin_int = %d\n",
					power_switch_value, switch_power_value, usb_switch_value, buck_boost_value, pogopin_int_value);
	return 0;
}

static int get_interface_status(void)
{
	int pogopin_int_value = -1, typec_int_value = -1;
	pogopin_int_value = gpio_get_value(g_pogopin_device_ops->di->pogopin_int_gpio);
	typec_int_value = gpio_get_value(g_pogopin_device_ops->di->typec_int_gpio);
	hw_pogopin_dbg("pogopin_int = %d, typec_int = %d\n", pogopin_int_value, typec_int_value);
	if((HIGH == pogopin_int_value) && (HIGH == typec_int_value)){
		return NO_INTERFACE;
	}else if((HIGH == pogopin_int_value) && (LOW == typec_int_value)){
		return TYPEC_INTERFACE;
	}else if((LOW == pogopin_int_value) && (HIGH == typec_int_value)){
		return POGOPIN_INTERFACE;
	}else if((LOW == pogopin_int_value) && (LOW == typec_int_value)){
		return POGOPIN_AND_TYPEC;
	}
	return 0;
}

static void pogopin_charge(void){
	__pogopin_switch_power(g_pogopin_device_ops->di, LOW);
	__pogopin_switch_buck_boost(g_pogopin_device_ops->di, LOW);
	__pogopin_switch_dpdm(g_pogopin_device_ops->di, HIGH);
	return;
}
void pogopin_otg(void){
	__pogopin_switch_power(g_pogopin_device_ops->di, HIGH);
	__pogopin_switch_buck_boost(g_pogopin_device_ops->di, HIGH);
	__pogopin_switch_dpdm(g_pogopin_device_ops->di, HIGH);
	return;
}
void typec_charge_otg(void){
	__pogopin_switch_power(g_pogopin_device_ops->di, HIGH);
	__pogopin_switch_buck_boost(g_pogopin_device_ops->di, LOW);
	__pogopin_switch_dpdm(g_pogopin_device_ops->di, LOW);
	return;
}

void cc_detect_register_ops(struct cc_detect_ops *ops)
{
	if (!ops) {
		hw_pogopin_dbg("%s ops or pogopin_device is NULL\n", __func__);
		return;
	}
	cc_detect_ops = ops;
}

void pogopin_set_fcp_support(bool value)
{
	struct hi6523_device_info *di = g_hi6523_dev;
	if (NULL == di) {
		hw_pogopin_dbg("%s hi6523_device_info is NULL!\n", __func__);
		return;
	}
	di->param_dts.fcp_support = value;
}

#ifdef CONFIG_SYSFS
#define POGOPIN_SYSFS_FIELD(_name, n, m, store)                \
{                                                   \
	.attr = __ATTR(_name, m, pogopin_sysfs_show, store),    \
	.name = POGOPIN_SYSFS_##n,          \
}

#define POGOPIN_SYSFS_FIELD_RW(_name, n)               \
	POGOPIN_SYSFS_FIELD(_name, n, S_IWUSR | S_IRUGO, pogopin_sysfs_store)

#define POGOPIN_SYSFS_FIELD_RO(_name, n)               \
	POGOPIN_SYSFS_FIELD(_name, n, S_IRUGO, NULL)

static ssize_t pogopin_sysfs_show(struct device *dev,
					struct device_attribute *attr, char *buf);
static ssize_t pogopin_sysfs_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count);

struct pogopin_sysfs_field_info {
	struct device_attribute attr;
	u8 name;
};

static struct pogopin_sysfs_field_info pogopin_sysfs_field_tbl[] = {
	POGOPIN_SYSFS_FIELD_RO(interface_type, INTERFACE_TYPE),
};

static struct attribute *pogopin_sysfs_attrs[ARRAY_SIZE(pogopin_sysfs_field_tbl) + 1];

static const struct attribute_group pogopin_sysfs_attr_group = {
	.attrs = pogopin_sysfs_attrs,
};


/**********************************************************
*  Function:       pogopin_sysfs_init_attrs
*  Description:    initialize pogopin_sysfs_attrs[] for charge attribute
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void pogopin_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(pogopin_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		pogopin_sysfs_attrs[i] = &pogopin_sysfs_field_tbl[i].attr.attr;
	}
	pogopin_sysfs_attrs[limit] = NULL;	/* Has additional entry for this */
}

/**********************************************************
*  Function:       pogopin_sysfs_field_lookup
*  Description:    get the current device_attribute from pogopin_sysfs_field_tbl by attr's name
*  Parameters:   name:device attribute name
*  return value:  pogopin_sysfs_field_tbl[]
**********************************************************/
static struct pogopin_sysfs_field_info *pogopin_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(pogopin_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
	if (!strncmp(name, pogopin_sysfs_field_tbl[i].attr.attr.name,strlen(name)))
		break;
	}
	if (i >= limit)
		return NULL;

	return &pogopin_sysfs_field_tbl[i];
}

/**********************************************************
*  Function:       pogopin_sysfs_show
*  Description:    show the value for all charge device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*  return value:  0-sucess or others-fail
**********************************************************/


static ssize_t pogopin_sysfs_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct pogopin_sysfs_field_info *info = NULL;

	info = pogopin_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	switch (info->name) {
		case POGOPIN_SYSFS_INTERFACE_TYPE:
			return snprintf(buf, PAGE_SIZE, "%u\n", get_interface_status());
		default:
			hw_pogopin_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
			break;
	}
	return 0;
}

/**********************************************************
*  Function:       pogopin_sysfs_store
*  Description:    set the value for pogopin_data's node which is can be written
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*                      count:unused
*  return value:  0-sucess or others-fail
**********************************************************/

static ssize_t pogopin_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct pogopin_sysfs_field_info *info = NULL;

	info = pogopin_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	switch (info->name) {
		case POGOPIN_SYSFS_INTERFACE_TYPE:
			break;
		default:
			hw_pogopin_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
			break;
	}
	return count;
}

/**********************************************************
*  Function:       pogopin_sysfs_create_group
*  Description:    create the pogopin device sysfs group
*  Parameters:   di:pogopin_device_info
*  return value:  0-sucess or others-fail
**********************************************************/
static int pogopin_sysfs_create_group(struct pogopin_device_info *di)
{
	pogopin_sysfs_init_attrs();
	return sysfs_create_group(&di->pdev->dev.kobj, &pogopin_sysfs_attr_group);
}

/**********************************************************
*  Function:       pogopin_sysfs_remove_group
*  Description:    remove the pogopin device sysfs group
*  Parameters:   di:pogopin_device_info
*  return value:  NULL
**********************************************************/
static inline void pogopin_sysfs_remove_group(struct pogopin_device_info *di)
{
	sysfs_remove_group(&di->pdev->dev.kobj, &pogopin_sysfs_attr_group);
}
#else
static int pogopin_sysfs_create_group(struct charge_device_info *di)
{
	return 0;
}

static inline void pogopin_sysfs_remove_group(struct charge_device_info *di)
{
}
#endif

int is_pogopin_support(void)
{
	return pogopin_support;
}

static struct pogopin_device_ops pogopin_device_ops = {
	.enable_pogopin_otg = pogopin_otg,
	.enable_pogopin_charge = pogopin_charge,
	.typec_charge_otg = typec_charge_otg,
	.get_connect_status = get_connect_status,
	.get_interface_status = get_interface_status,
};

static void pogopin_switch_work(struct work_struct *work)
{
	struct pogopin_device_info *di = g_pogopin_device_ops->di;
	int pogopin_gpio_value = -1;
	int buck_boost_gpio_value;
	unsigned long timeout;

	if((NULL == g_pogopin_device_ops) || (NULL == cc_detect_ops)){
		hw_pogopin_err("pogopin_device error");
		return;
	}
	buck_boost_gpio_value = gpio_get_value(di->buck_boost_gpio);
	pogopin_gpio_value = gpio_get_value(di->pogopin_int_gpio);
	hw_pogopin_dbg("%s-%d: buck_boost_gpio_value = %d pogopin_gpio_value = %d\n",__func__, __LINE__, buck_boost_gpio_value, pogopin_gpio_value);
	if(!buck_boost_gpio_value){
		if(LOW == pogopin_gpio_value){
			g_pogopin_device_ops->di->current_int_status = g_pogopin_device_ops->get_interface_status();
			hw_pogopin_dbg("%s current_int_status = %d\n", __func__, g_pogopin_device_ops->di->current_int_status);
			hw_pogopin_dbg("%s-%d pogopin charger insert\n", __func__, __LINE__);
			pogopin_set_fcp_support(FALSE);
			hw_pogopin_dbg("1.cutoff CC\n");
			cc_detect_ops->typec_detect_disable(TRUE);
			hw_pogopin_dbg("2.waitfor typec off\n");
			if(g_pogopin_device_ops->di->current_int_status == POGOPIN_AND_TYPEC){
				//timeout = wait_for_completion_timeout(&dev_off_completion, msecs_to_jiffies(500));
				//hw_pogopin_dbg("%s  timeout = %d\n", __func__, timeout);
				msleep(500);
			}else{
				timeout = wait_for_completion_timeout(&dev_off_completion, msecs_to_jiffies(50));
				hw_pogopin_dbg("%s  timeout = %ld\n", __func__, timeout);
			}
			hw_pogopin_dbg("3.start pogopin charger\n");
			pogopin_charge();
			hisi_usb_otg_bc_again();
			reinit_completion(&dev_off_completion);
		}else if(HIGH == pogopin_gpio_value){
			hw_pogopin_dbg("%s-%d pogopin charger remove\n", __func__, __LINE__);
			pogopin_set_fcp_support(TRUE);
			hw_pogopin_dbg("1.cutoff pogopin charger\n");
			typec_charge_otg();
			hw_pogopin_dbg("2.waitfor pogopin charger off\n");
			timeout = wait_for_completion_timeout(&dev_off_completion, msecs_to_jiffies(500));
			hw_pogopin_dbg("%s  timeout = %ld\n", __func__, timeout);
			hw_pogopin_dbg("3.enable CC\n");
			cc_detect_ops->typec_detect_disable(FALSE);
			reinit_completion(&dev_off_completion);
			g_pogopin_device_ops->di->current_int_status = g_pogopin_device_ops->get_interface_status();
		}
	}
	return;
}

/**********************************************************
*  Function:       pogopin_int_handler
*  Description:    callback function for pogopin fault irq in charging
*  Parameters:   irq:pogopin insert/remove interrupt
*                      _di:pogopin_device_info
*  return value:  IRQ_HANDLED-success or others
**********************************************************/
static irqreturn_t pogopin_int_handler(int irq, void *_di)
{
	struct pogopin_device_info *di = _di;
	int gpio_value = -1;

	if(NULL == di){
		hw_pogopin_err("pogopin_device error");
		return IRQ_HANDLED;
	}
	disable_irq_nosync(di->pogopin_int_irq);
	gpio_value = gpio_get_value(di->pogopin_int_gpio);

	if (LOW == gpio_value){  //Pogopin insert
		irq_set_irq_type(di->pogopin_int_irq, IRQF_TRIGGER_HIGH | IRQF_NO_SUSPEND);
	} else { //Pogopin remove
		irq_set_irq_type(di->pogopin_int_irq, IRQF_TRIGGER_LOW | IRQF_NO_SUSPEND);
	}
	enable_irq(di->pogopin_int_irq);
	schedule_work(&di->work);
	return IRQ_HANDLED;
}

static int set_direction_request_irq(struct pogopin_device_info *di)
{
	int ret = -1;
	int gpio_value = -1;

	if(NULL == di){
		hw_pogopin_err("pogopin_device error");
		return ret;
	}
	//GPIO 方向设置
	ret = gpio_direction_output(di->usb_switch_gpio, 0);
	if (ret < 0) {
		hw_pogopin_err("Could not set usb_switch_gpio direction.\n");
		goto out;
	}
	ret = gpio_direction_output(di->power_switch_gpio, 0);
	if (ret < 0) {
		hw_pogopin_err("Could not set power_switch_gpio direction.\n");
		goto out;
	}
	ret = gpio_direction_output(di->switch_power_gpio, 0);
	if (ret < 0) {
		hw_pogopin_err("Could not set switch_power_gpio direction.\n");
		goto out;
	}
	ret = gpio_direction_output(di->buck_boost_gpio, 0);
	if (ret < 0) {
		hw_pogopin_err("Could not set buck_boost_gpio direction.\n");
		goto out;
	}

	ret = gpio_direction_input(di->pogopin_int_gpio);
	if (ret < 0) {
		hw_pogopin_err("Could not set pogopin_int_gpio direction.\n");
		goto out;
	}

	ret = gpio_direction_input(di->typec_int_gpio);
	if (ret < 0) {
		hw_pogopin_err("Could not set typec_int_gpio direction.\n");
		goto out;
	}
	//获取中断号
	di->pogopin_int_irq = gpio_to_irq(di->pogopin_int_gpio);
	if (di->pogopin_int_irq < 0) {
		hw_pogopin_err("could not map pogopin_int_gpio to irq\n");
		ret = di->pogopin_int_irq;
		goto out;
	}
	hw_pogopin_dbg("%s-%d di->pogopin_int_irq = %d\n ", __func__, __LINE__, di->pogopin_int_irq);

	//注册中断
	gpio_value = gpio_get_value(di->pogopin_int_gpio);

	if (0 == gpio_value){  //Pogopin insert
		ret = request_irq(di->pogopin_int_irq, pogopin_int_handler,
			IRQF_TRIGGER_HIGH | IRQF_NO_SUSPEND , "pogopin_int_irq", di); // IRQF_TRIGGER_HIGH
		if (ret) {
			hw_pogopin_err("could not request pogopin_int_irq\n");
			di->pogopin_int_irq = -1;
			goto out;
		}
	} else { //Pogopin remove
		ret = request_irq(di->pogopin_int_irq, pogopin_int_handler,
			IRQF_TRIGGER_LOW | IRQF_NO_SUSPEND , "pogopin_int_irq", di); // IRQF_TRIGGER_LOW
		if (ret) {
			hw_pogopin_err("could not request pogopin_int_irq\n");
			di->pogopin_int_irq = -1;
			goto out;
		}
	}

	return 0;

out:
	return ret;
}

static void free_irqs(struct pogopin_device_info *di)
{
	free_irq(di->pogopin_int_irq, di);
	return;
}

static int parse_and_request_gpios(struct pogopin_device_info *di, struct device_node *np)
{
	int ret = -1;

	if(NULL == di){
		hw_pogopin_err("pogopin_device error");
		return ret;
	}

	if(of_property_read_u32(np, "pogopin_support", &pogopin_support)){
		hw_pogopin_err("get pogopin_support fail!\n");
	}
	hw_pogopin_dbg("pogopin_support = %d\n", pogopin_support);

	if(!pogopin_support){
		return ret;
	}
	// usb 切换GPIO
	di->usb_switch_gpio = of_get_named_gpio(np, "usb_switch_gpio", 0);
	if (!gpio_is_valid(di->usb_switch_gpio)) {
		hw_pogopin_err("usb_switch_gpio is not valid\n");
		ret = -EINVAL;
		goto of_get_named_gpio_usb_switch_failed;
	}
	hw_pogopin_dbg("%s-%d di->usb_switch_gpio = %d\n ", __func__, __LINE__, di->usb_switch_gpio);

	//GPIO 请求
	ret = gpio_request(di->usb_switch_gpio, "usb_switch");
	if (ret) {
		hw_pogopin_err("could not request usb_switch_gpio\n");
		goto gpio_request_usb_switch_failed;
	}

	//power 切换GPIO
	di->power_switch_gpio = of_get_named_gpio(np, "power_switch_gpio", 0);
	if (!gpio_is_valid(di->power_switch_gpio)) {
		hw_pogopin_err("power_switch_gpio is not valid\n");
		ret = -EINVAL;
		goto of_get_named_gpio_power_switch_failed;
	}
	hw_pogopin_dbg("%s-%d di->power_switch_gpio = %d\n", __func__, __LINE__, di->power_switch_gpio);

	di->switch_power_gpio = of_get_named_gpio(np, "switch_power_gpio", 0);
	if (!gpio_is_valid(di->switch_power_gpio)) {
		hw_pogopin_err("switch_power_gpio is not valid\n");
		ret = -EINVAL;
		goto of_get_named_gpio_switch_power_failed;
	}
	hw_pogopin_dbg("%s-%d di->switch_power_gpio = %d\n", __func__, __LINE__, di->switch_power_gpio);

	//GPIO 请求
	ret = gpio_request(di->power_switch_gpio, "power_switch");
	if (ret) {
		hw_pogopin_err("could not request power_switch_gpio\n");
		goto gpio_request_power_switch_failed;
	}

	ret = gpio_request(di->switch_power_gpio, "switch_power");
	if (ret) {
		hw_pogopin_err("could not request switch_power_gpio\n");
		goto gpio_request_switch_power_failed;
	}
	//buck 切换GPIO
	di->buck_boost_gpio = of_get_named_gpio(np, "buck_boost_gpio", 0);
	if (!gpio_is_valid(di->buck_boost_gpio)) {
		hw_pogopin_err("buck_boost_gpio is not valid\n");
		ret = -EINVAL;
		goto of_get_named_gpio_buck_boost_failed;
	}
	hw_pogopin_dbg("%s-%d di->buck_boost_gpio = %d\n", __func__, __LINE__, di->buck_boost_gpio);

	//GPIO 请求
	ret = gpio_request(di->buck_boost_gpio, "buck_boost");
	if (ret) {
		hw_pogopin_err("could not request buck_boost_gpio\n");
		goto gpio_request_buck_boost_failed;
	}

	//pogopin 中断 GPIO
	di->pogopin_int_gpio = of_get_named_gpio(np, "pogopin_int_gpio", 0);
	if (!gpio_is_valid(di->pogopin_int_gpio)) {
		hw_pogopin_err("pogopin_int_gpio is not valid\n");
		ret = -EINVAL;
		goto of_get_named_gpio_pogopin_int_failed;
	}
	hw_pogopin_dbg("%s-%d di->pogopin_int_gpio = %d\n", __func__, __LINE__, di->pogopin_int_gpio);

	//GPIO 请求
	ret = gpio_request(di->pogopin_int_gpio, "pogopin_int");
	if (ret) {
		hw_pogopin_err("could not request pogopin_int_gpio\n");
		goto gpio_request_pogopin_int_failed;
	}

	//typec 中断 GPIO
	di->typec_int_gpio = of_get_named_gpio(np, "typec_int_gpio", 0);
	if (!gpio_is_valid(di->typec_int_gpio)) {
		hw_pogopin_err("typec_int_gpio is not valid\n");
		ret = -EINVAL;
		goto of_get_named_gpio_typec_int_failed;
	}
	hw_pogopin_dbg("%s-%d di->typec_int_gpio = %d\n", __func__, __LINE__, di->typec_int_gpio);

	//GPIO 请求
	ret = gpio_request(di->typec_int_gpio, "typec_int");
	if (ret) {
		hw_pogopin_err("could not request typec_int_gpio\n");
		goto gpio_request_typec_int_failed;
	}

	return 0;

gpio_request_typec_int_failed:
of_get_named_gpio_typec_int_failed:
	gpio_free(di->pogopin_int_gpio);
gpio_request_pogopin_int_failed:
of_get_named_gpio_pogopin_int_failed:
	gpio_free(di->buck_boost_gpio);
gpio_request_buck_boost_failed:
of_get_named_gpio_buck_boost_failed:
	gpio_free(di->switch_power_gpio);
	gpio_free(di->power_switch_gpio);
gpio_request_switch_power_failed:
gpio_request_power_switch_failed:
of_get_named_gpio_switch_power_failed:
of_get_named_gpio_power_switch_failed:
	gpio_free(di->usb_switch_gpio);
gpio_request_usb_switch_failed:
of_get_named_gpio_usb_switch_failed:

	return ret;
}
static void free_gpios(struct pogopin_device_info *di)
{
	if(NULL == di){
		hw_pogopin_err("pogopin_device error");
		return;
	}
	gpio_free(di->typec_int_gpio);
	gpio_free(di->pogopin_int_gpio);
	gpio_free(di->buck_boost_gpio);
	gpio_free(di->switch_power_gpio);
	gpio_free(di->power_switch_gpio);
	gpio_free(di->usb_switch_gpio);
	return;
}
/**********************************************************
*  Function:       pogopin_probe
*  Description:    pogopin module probe
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/

static int pogopin_probe(struct platform_device *pdev)
{
	int ret = -1;
	struct device_node *np = NULL;
	struct device* dev = NULL;
	struct pogopin_device_info *di;

	hw_pogopin_dbg("%s-%d \n", __func__, __LINE__);
	if (NULL == pdev) {
		hw_pogopin_err("%s The pdev is NULL !\n", __func__);
		return ret;
	}

	//分配内存
	di = (struct pogopin_device_info *)devm_kzalloc(&pdev->dev, sizeof(*di), GFP_KERNEL);
	if (NULL == di) {
		hw_pogopin_err("%s alloc pogopin_dev failed! not enough memory\n", __func__);
		return ret;
	}

	platform_set_drvdata(pdev, di);
	di->pdev = pdev;

	dev = &pdev->dev;
	np = dev->of_node;

	ret = parse_and_request_gpios(di, np);
	if (ret) {
		hw_pogopin_err("parse_and_request_gpios failed\n");
		goto parse_and_request_gpios_failed;
	}
	ret = set_direction_request_irq(di);
	if (ret){
		hw_pogopin_err("set_direction_request_irq failed\n");
		goto set_direction_request_irq_failed;
	}

	g_pogopin_device_ops = &pogopin_device_ops;
	g_pogopin_device_ops->di = di;

	INIT_WORK(&di->work, pogopin_switch_work);
	init_completion(&dev_off_completion);
	g_pogopin_device_ops->di->current_int_status = g_pogopin_device_ops->get_interface_status();

	pogopin_int_handler(di->pogopin_int_irq, di);
	get_connect_status();

	ret = pogopin_sysfs_create_group(di);
	if (ret){
		hw_pogopin_err("can't create pogopin sysfs entries\n");
	}
	pogopin_class = class_create(THIS_MODULE, "hw_pogopin");
	if (IS_ERR(pogopin_class)) {
		pr_info("Unable to create pogopin class; errno = %ld\n",PTR_ERR(pogopin_class));
		goto class_create_failed;
	}

	if (pogopin_dev == NULL)
		pogopin_dev = device_create(pogopin_class, NULL, 0, NULL, "pogopin");
	ret = sysfs_create_link(&pogopin_dev->kobj, &di->pdev->dev.kobj, "pogopin_data");
	if (ret) {
		hw_pogopin_err("create link to pogopin_data fail.\n");
		goto sysfs_create_link_failed;
	}

	return 0;
sysfs_create_link_failed:
class_create_failed:
	pogopin_sysfs_remove_group(di);
	free_irqs(di);
set_direction_request_irq_failed:
	free_gpios(di);
parse_and_request_gpios_failed:
	devm_kfree(&pdev->dev, di);
	return ret;
}

/**********************************************************
*  Function:       pogopin_remove
*  Description:    pogopin module remove
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/
static int pogopin_remove(struct platform_device *pdev)
{
	struct pogopin_device_info *di;
	int ret = -1;
	hw_pogopin_dbg("%s-%d\n", __func__, __LINE__);
	g_pogopin_device_ops = NULL;
	if (NULL == pdev) {
		hw_pogopin_err("%s pdev is NULL!\n", __func__);
		return -ENODEV;
	}
	di = platform_get_drvdata(pdev);
	if(NULL == di){
		hw_pogopin_err("pogopin_device error");
		return ret;
	}
	pogopin_sysfs_remove_group(di);
	free_irqs(di);
	free_gpios(di);
	devm_kfree(&pdev->dev, di);
	return 0;
}


static struct of_device_id pogopin_match_table[] = {
	{
	 .compatible = "huawei,pogopin",
	 .data = NULL,
	},
	{
	},
};

static struct platform_driver pogopin_driver = {
	.probe = pogopin_probe,
	.remove = pogopin_remove,
	.driver = {
		   .name = "huawei,pogopin",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(pogopin_match_table),
	},
};

/**********************************************************
*  Function:       pogopin_init
*  Description:    pogopin module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/

static int __init pogopin_init(void)
{
	return platform_driver_register(&pogopin_driver);
}

/**********************************************************
*  Function:       pogopin_exit
*  Description:    pogopin module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit pogopin_exit(void)
{
	platform_driver_unregister(&pogopin_driver);
	return;
}

late_initcall(pogopin_init);
module_exit(pogopin_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("huawei pogopin module driver");
MODULE_AUTHOR("HUAWEI Inc");
